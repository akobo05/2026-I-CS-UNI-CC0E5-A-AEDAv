#ifndef __BINARYTREE_H__
#define __BINARYTREE_H__

#include <iostream>
#include <istream>
#include <ostream>
#include <stack>
#include <stdexcept>
#include <mutex>
#include <shared_mutex>
#include <utility>
#include <functional>
#include "../types.h"
#include "traits.h"
using namespace std;

// Nodo base de arbol binario. El segundo parametro DerivedNode permite que los
// hijos sean del tipo derivado (p.ej. AVLNode), siguiendo el patron CRTP de nodo.
template <typename T, typename DerivedNode>
struct BinaryTreeNode {
    using value_type = T;
    using ref_type   = Ref;
    T   m_data;
    Ref m_ref;
    DerivedNode *m_pChild[2];   // 0 = izquierdo, 1 = derecho
    BinaryTreeNode() : m_data(T()), m_ref(Ref()), m_pChild{nullptr, nullptr} {}
    BinaryTreeNode(T d, Ref r) : m_data(d), m_ref(r), m_pChild{nullptr, nullptr} {}
    virtual ~BinaryTreeNode() {}
    DerivedNode*&       child(Index d)       { return m_pChild[d]; }
    DerivedNode* const& child(Index d) const { return m_pChild[d]; }
};

// Arbol binario de busqueda generico. Provee toda la maquinaria (insertar,
// buscar, recorrer, serializar, copiar/mover, concurrencia). La insercion es un
// HOOK VIRTUAL (internal_insert): la version base hace BST simple; las clases
// derivadas (AVL) la sobreescriben para balancear. Reutilizacion via herencia.
template <typename Trait>
class BinaryTree {
public:
    using value_type = typename Trait::value_type;
    using Node       = typename Trait::Node;
    using Comp       = typename Trait::Comp;
    using MySelf     = BinaryTree<Trait>;

protected:
    Node *m_pRoot;
    Index m_size;
    Comp  m_comp;
    mutable shared_mutex m_mtx;

    // HOOK VIRTUAL. Version base: insercion BST. AVL la sobreescribe (rebalance).
    virtual void internal_insert(Node* &pNode, const value_type &data, Ref ref, bool &inserted){
        if(!pNode){ pNode = new Node(data, ref); inserted = true; return; }
        bool less_v = m_comp(data, pNode->m_data);
        bool less_n = m_comp(pNode->m_data, data);
        if(!less_v && !less_n){ pNode->m_ref = ref; inserted = false; return; }  // duplicado
        internal_insert(pNode->child(less_v ? 0 : 1), data, ref, inserted);
    }

    void internal_clear(Node *n){
        if(!n) return;
        internal_clear(n->child(0));
        internal_clear(n->child(1));
        delete n;
    }
    Node* internal_find(Node *n, const value_type &v) const {
        if(!n) return nullptr;
        if(m_comp(v, n->m_data)) return internal_find(n->child(0), v);
        if(m_comp(n->m_data, v)) return internal_find(n->child(1), v);
        return n;
    }
    template <typename F>
    void internal_inorder(Node *n, F func) const {
        if(!n) return;
        internal_inorder(n->child(0), func);
        func(*n);
        internal_inorder(n->child(1), func);
    }

public:
    BinaryTree() : m_pRoot(nullptr), m_size(0) {}

    BinaryTree(const BinaryTree &o) : m_pRoot(nullptr), m_size(0) {
        shared_lock<shared_mutex> lk(o.m_mtx);
        o.internal_inorder(o.m_pRoot, [this](const Node &n){
            bool ins = false;
            internal_insert(m_pRoot, n.m_data, n.m_ref, ins);
            if(ins) ++m_size;
        });
    }
    BinaryTree(BinaryTree &&o) noexcept : m_pRoot(nullptr), m_size(0) {
        unique_lock<shared_mutex> lk(o.m_mtx);
        m_pRoot = std::exchange(o.m_pRoot, nullptr);
        m_size  = std::exchange(o.m_size, 0);
    }
    BinaryTree& operator=(const BinaryTree &o){
        if(this == &o) return *this;
        std::scoped_lock lock(m_mtx, o.m_mtx);
        internal_clear(m_pRoot); m_pRoot = nullptr; m_size = 0;
        o.internal_inorder(o.m_pRoot, [this](const Node &n){
            bool ins = false;
            internal_insert(m_pRoot, n.m_data, n.m_ref, ins);
            if(ins) ++m_size;
        });
        return *this;
    }
    BinaryTree& operator=(BinaryTree &&o) noexcept{
        if(this == &o) return *this;
        std::scoped_lock lock(m_mtx, o.m_mtx);
        internal_clear(m_pRoot);
        m_pRoot = std::exchange(o.m_pRoot, nullptr);
        m_size  = std::exchange(o.m_size, 0);
        return *this;
    }
    virtual ~BinaryTree(){
        unique_lock<shared_mutex> lk(m_mtx);
        internal_clear(m_pRoot);
        m_pRoot = nullptr;
    }

    // insert publico: dispara el HOOK VIRTUAL internal_insert (-> version del AVL si aplica)
    bool insert(const value_type &v, Ref r){
        unique_lock<shared_mutex> lk(m_mtx);
        return insert_nolock(v, r);
    }
    bool insert_nolock(const value_type &v, Ref r){
        bool inserted = false;
        internal_insert(m_pRoot, v, r, inserted);   // virtual: AVL rebalancea
        if(inserted) ++m_size;
        return inserted;
    }
    bool find(const value_type &v, Ref &outRef) const {
        shared_lock<shared_mutex> lk(m_mtx);
        return find_nolock(v, outRef);
    }
    bool find_nolock(const value_type &v, Ref &outRef) const {
        Node *n = internal_find(m_pRoot, v);
        if(!n) return false;
        outRef = n->m_ref;
        return true;
    }
    // Devuelve puntero al dato almacenado por clave, o nullptr (uso de HashTable::operator[]).
    value_type* find_data_nolock(const value_type &v){
        Node *n = internal_find(m_pRoot, v);
        return n ? &n->m_data : nullptr;
    }
    Index size() const { shared_lock<shared_mutex> lk(m_mtx); return m_size; }
    bool  empty() const { return size() == 0; }
    Node* getRoot() const { return m_pRoot; }

    template <typename F>
    void inorder(F func) const {
        shared_lock<shared_mutex> lk(m_mtx);
        internal_inorder(m_pRoot, func);
    }

    // Iterador in-order. operator* devuelve el DATO (value_type&) para soportar
    // range-for y structured binding directamente sobre el dato.
    class iterator {
    public:
        std::stack<Node*> m_stk;
        iterator() = default;
        void push_left(Node *n){ while(n){ m_stk.push(n); n = n->child(0); } }
        iterator(Node *root){ push_left(root); }
        value_type& operator*()  { return m_stk.top()->m_data; }
        value_type* operator->() { return &m_stk.top()->m_data; }
        iterator& operator++(){
            Node *n = m_stk.top(); m_stk.pop();
            push_left(n->child(1));
            return *this;
        }
        bool operator==(const iterator &o) const { return m_stk == o.m_stk; }
        bool operator!=(const iterator &o) const { return !(*this == o); }
    };
    iterator begin() const { return iterator(m_pRoot); }
    iterator end()   const { return iterator(); }

    friend std::ostream& operator<<(std::ostream &os, const BinaryTree &t){
        shared_lock<shared_mutex> lk(t.m_mtx);
        os << "{";
        bool first = true;
        t.internal_inorder(t.m_pRoot, [&](const Node &n){
            if(!first) os << ",";
            os << "(" << n.m_data << "," << n.m_ref << ")";
            first = false;
        });
        os << "}";
        return os;
    }
    friend std::istream& operator>>(std::istream &is, BinaryTree &t){
        char ch;
        if(!(is >> ch) || ch != '{'){ is.clear(ios_base::failbit); return is; }
        value_type v; Ref r; char comma, close;
        while(is >> ch && ch != '}'){
            if(ch == '('){
                if(is >> v >> comma >> r >> close)
                    if(comma == ',' && close == ')')
                        t.insert(v, r);   // virtual insert -> AVL rebalancea
            }
        }
        return is;
    }
};

#endif // __BINARYTREE_H__
