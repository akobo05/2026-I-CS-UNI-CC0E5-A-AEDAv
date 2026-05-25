#ifndef __AVL_H__
#define __AVL_H__

#include <iostream>
#include <string>
#include <sstream>
#include <stack>
#include <stdexcept>
#include <mutex>
#include <shared_mutex>
#include <utility>
#include <functional>
#include "util.h"
#include "../types.h"
#include "traits.h"
using namespace std;

template <typename T>
struct AVLNode {
    using value_type = T;
    T   m_data;
    Ref m_ref;
    AVLNode *m_pChild[2];   // 0=left, 1=right
    long m_height;
    AVLNode() : m_data(T()), m_ref(Ref()), m_pChild{nullptr,nullptr}, m_height(1) {}
    AVLNode(T d, Ref r) : m_data(d), m_ref(r), m_pChild{nullptr,nullptr}, m_height(1) {}
};

template <typename T> struct AscendingAVLTrait  : BaseTrait<AVLNode<T>, less<T>>{};
template <typename T> struct DescendingAVLTrait : BaseTrait<AVLNode<T>, greater<T>>{};

template <typename Trait>
class AVL {
public:
    using value_type = typename Trait::value_type;
    using Node       = typename Trait::Node;
    using Comp       = typename Trait::Comp;
    using MySelf     = AVL<Trait>;

private:
    Node *m_pRoot;
    Index m_size;
    Comp  m_comp;
    mutable shared_mutex m_mtx;

    long  height(Node *n) const { return n ? n->m_height : 0; }
    long  balance(Node *n) const { return n ? height(n->m_pChild[0]) - height(n->m_pChild[1]) : 0; }
    void  updateHeight(Node *n){
        long lh = height(n->m_pChild[0]);
        long rh = height(n->m_pChild[1]);
        n->m_height = 1 + (lh > rh ? lh : rh);
    }
    Node* rotateLeft(Node *x);
    Node* rotateRight(Node *y);
    Node* internal_insert(Node *n, const value_type &v, Ref r, bool &inserted);
    Node* internal_remove(Node *n, const value_type &v, bool &removed);
    Node* internal_find  (Node *n, const value_type &v) const;
    void  internal_clear (Node *n);
    template <typename F>
    void  internal_inorder(Node *n, F func) const;

public:
    AVL();
    AVL(const AVL &other);
    AVL(AVL &&other) noexcept;
    AVL& operator=(const AVL &other);
    AVL& operator=(AVL &&other) noexcept;
    virtual ~AVL();

    // API publica (toma su propio lock — uso standalone)
    bool insert(const value_type &v, Ref r);
    bool find  (const value_type &v, Ref &outRef) const;
    bool remove(const value_type &v);
    Index size() const;
    bool  empty() const;

    template <typename F>
    void  inorder(F func) const;

    // API *_nolock: el caller debe sostener un lock externo (uso interno por HashTable).
    bool  insert_nolock(const value_type &v, Ref r);
    bool  find_nolock  (const value_type &v, Ref &outRef) const;
    bool  remove_nolock(const value_type &v);
    Index size_nolock() const { return m_size; }
    template <typename F>
    void  inorder_nolock(F func) const;

    // Iterator in-order (sin lock interno — para uso bajo lock externo)
    class iterator {
    public:
        std::stack<Node*> m_stk;
        iterator() = default;
        void push_left(Node *n){ while(n){ m_stk.push(n); n = n->m_pChild[0]; } }
        iterator(Node *root){ push_left(root); }
        Node& operator*()  { return *m_stk.top(); }
        Node* operator->() { return  m_stk.top(); }
        iterator& operator++(){
            Node *n = m_stk.top(); m_stk.pop();
            push_left(n->m_pChild[1]);
            return *this;
        }
        bool operator==(const iterator &o) const { return m_stk == o.m_stk; }
        bool operator!=(const iterator &o) const { return !(*this == o); }
    };
    iterator begin_nolock() const { return iterator(m_pRoot); }
    iterator end_nolock()   const { return iterator(); }

    friend std::ostream& operator<<(std::ostream& os, const AVL& t){
        shared_lock<shared_mutex> lock(t.m_mtx);
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
};

template <typename Trait>
AVL<Trait>::AVL() : m_pRoot(nullptr), m_size(0) {}

template <typename Trait>
AVL<Trait>::AVL(const AVL &other) : m_pRoot(nullptr), m_size(0) {
    shared_lock<shared_mutex> lock(other.m_mtx);
    other.internal_inorder(other.m_pRoot, [this](const Node &n){
        bool ins = false;
        m_pRoot = internal_insert(m_pRoot, n.m_data, n.m_ref, ins);
        if(ins) ++m_size;
    });
}

template <typename Trait>
AVL<Trait>::AVL(AVL &&other) noexcept : m_pRoot(nullptr), m_size(0) {
    unique_lock<shared_mutex> lock(other.m_mtx);
    m_pRoot = std::exchange(other.m_pRoot, nullptr);
    m_size  = std::exchange(other.m_size, 0);
}

template <typename Trait>
AVL<Trait>& AVL<Trait>::operator=(const AVL &other){
    if(this == &other) return *this;
    unique_lock<shared_mutex> lk_this(m_mtx);
    shared_lock<shared_mutex> lk_other(other.m_mtx);
    internal_clear(m_pRoot); m_pRoot = nullptr; m_size = 0;
    other.internal_inorder(other.m_pRoot, [this](const Node &n){
        bool ins = false;
        m_pRoot = internal_insert(m_pRoot, n.m_data, n.m_ref, ins);
        if(ins) ++m_size;
    });
    return *this;
}

template <typename Trait>
AVL<Trait>& AVL<Trait>::operator=(AVL &&other) noexcept{
    if(this == &other) return *this;
    unique_lock<shared_mutex> lk_this(m_mtx);
    unique_lock<shared_mutex> lk_other(other.m_mtx);
    internal_clear(m_pRoot);
    m_pRoot = std::exchange(other.m_pRoot, nullptr);
    m_size  = std::exchange(other.m_size, 0);
    return *this;
}

template <typename Trait>
AVL<Trait>::~AVL(){
    unique_lock<shared_mutex> lock(m_mtx);
    internal_clear(m_pRoot);
    m_pRoot = nullptr;
}

template <typename Trait>
void AVL<Trait>::internal_clear(Node *n){
    if(!n) return;
    internal_clear(n->m_pChild[0]);
    internal_clear(n->m_pChild[1]);
    delete n;
}

template <typename Trait>
Index AVL<Trait>::size() const{
    shared_lock<shared_mutex> lock(m_mtx);
    return m_size;
}

template <typename Trait>
bool AVL<Trait>::empty() const{
    return size() == 0;
}

template <typename Trait>
template <typename F>
void AVL<Trait>::internal_inorder(Node *n, F func) const{
    if(!n) return;
    internal_inorder(n->m_pChild[0], func);
    func(*n);
    internal_inorder(n->m_pChild[1], func);
}

template <typename Trait>
template <typename F>
void AVL<Trait>::inorder(F func) const{
    shared_lock<shared_mutex> lock(m_mtx);
    internal_inorder(m_pRoot, func);
}

template <typename Trait>
template <typename F>
void AVL<Trait>::inorder_nolock(F func) const{
    internal_inorder(m_pRoot, func);
}

template <typename Trait>
typename AVL<Trait>::Node* AVL<Trait>::rotateLeft(Node *x){
    Node *y = x->m_pChild[1];
    Node *T2 = y->m_pChild[0];
    y->m_pChild[0] = x;
    x->m_pChild[1] = T2;
    updateHeight(x);
    updateHeight(y);
    return y;
}

template <typename Trait>
typename AVL<Trait>::Node* AVL<Trait>::rotateRight(Node *y){
    Node *x = y->m_pChild[0];
    Node *T2 = x->m_pChild[1];
    x->m_pChild[1] = y;
    y->m_pChild[0] = T2;
    updateHeight(y);
    updateHeight(x);
    return x;
}

template <typename Trait>
typename AVL<Trait>::Node* AVL<Trait>::internal_insert(Node *n, const value_type &v, Ref r, bool &inserted){
    if(!n){
        inserted = true;
        return new Node(v, r);
    }
    // duplicado: actualiza ref, no inserta
    bool less_v = m_comp(v, n->m_data);
    bool less_n = m_comp(n->m_data, v);
    if(!less_v && !less_n){
        n->m_ref = r;
        inserted = false;
        return n;
    }
    int branch = less_v ? 0 : 1;
    n->m_pChild[branch] = internal_insert(n->m_pChild[branch], v, r, inserted);
    updateHeight(n);

    long bal = balance(n);
    // LL
    if(bal > 1 && m_comp(v, n->m_pChild[0]->m_data))
        return rotateRight(n);
    // RR
    if(bal < -1 && m_comp(n->m_pChild[1]->m_data, v))
        return rotateLeft(n);
    // LR
    if(bal > 1 && m_comp(n->m_pChild[0]->m_data, v)){
        n->m_pChild[0] = rotateLeft(n->m_pChild[0]);
        return rotateRight(n);
    }
    // RL
    if(bal < -1 && m_comp(v, n->m_pChild[1]->m_data)){
        n->m_pChild[1] = rotateRight(n->m_pChild[1]);
        return rotateLeft(n);
    }
    return n;
}

template <typename Trait>
bool AVL<Trait>::insert_nolock(const value_type &v, Ref r){
    bool inserted = false;
    m_pRoot = internal_insert(m_pRoot, v, r, inserted);
    if(inserted) ++m_size;
    return inserted;
}

template <typename Trait>
bool AVL<Trait>::insert(const value_type &v, Ref r){
    unique_lock<shared_mutex> lock(m_mtx);
    return insert_nolock(v, r);
}

template <typename Trait>
typename AVL<Trait>::Node* AVL<Trait>::internal_find(Node *n, const value_type &v) const{
    if(!n) return nullptr;
    if(m_comp(v, n->m_data))      return internal_find(n->m_pChild[0], v);
    if(m_comp(n->m_data, v))      return internal_find(n->m_pChild[1], v);
    return n;
}

template <typename Trait>
bool AVL<Trait>::find_nolock(const value_type &v, Ref &outRef) const{
    Node *n = internal_find(m_pRoot, v);
    if(!n) return false;
    outRef = n->m_ref;
    return true;
}

template <typename Trait>
bool AVL<Trait>::find(const value_type &v, Ref &outRef) const{
    shared_lock<shared_mutex> lock(m_mtx);
    return find_nolock(v, outRef);
}

template <typename Trait>
typename AVL<Trait>::Node* AVL<Trait>::internal_remove(Node *n, const value_type &v, bool &removed){
    if(!n){ removed = false; return nullptr; }
    if(m_comp(v, n->m_data)){
        n->m_pChild[0] = internal_remove(n->m_pChild[0], v, removed);
    } else if(m_comp(n->m_data, v)){
        n->m_pChild[1] = internal_remove(n->m_pChild[1], v, removed);
    } else {
        // encontrado
        removed = true;
        if(!n->m_pChild[0] || !n->m_pChild[1]){
            Node *child = n->m_pChild[0] ? n->m_pChild[0] : n->m_pChild[1];
            if(!child){ delete n; return nullptr; }
            Node *tmp = child;
            *n = *tmp;        // copia data, ref, height, children
            delete tmp;
        } else {
            // dos hijos: sucesor inorder (minimo del subarbol derecho)
            Node *succ = n->m_pChild[1];
            while(succ->m_pChild[0]) succ = succ->m_pChild[0];
            n->m_data = succ->m_data;
            n->m_ref  = succ->m_ref;
            bool dummy = false;
            n->m_pChild[1] = internal_remove(n->m_pChild[1], succ->m_data, dummy);
        }
    }
    if(!n) return n;
    updateHeight(n);
    long bal = balance(n);
    // LL
    if(bal > 1 && balance(n->m_pChild[0]) >= 0) return rotateRight(n);
    // LR
    if(bal > 1 && balance(n->m_pChild[0]) <  0){
        n->m_pChild[0] = rotateLeft(n->m_pChild[0]);
        return rotateRight(n);
    }
    // RR
    if(bal < -1 && balance(n->m_pChild[1]) <= 0) return rotateLeft(n);
    // RL
    if(bal < -1 && balance(n->m_pChild[1]) >  0){
        n->m_pChild[1] = rotateRight(n->m_pChild[1]);
        return rotateLeft(n);
    }
    return n;
}

template <typename Trait>
bool AVL<Trait>::remove_nolock(const value_type &v){
    bool removed = false;
    m_pRoot = internal_remove(m_pRoot, v, removed);
    if(removed) --m_size;
    return removed;
}

template <typename Trait>
bool AVL<Trait>::remove(const value_type &v){
    unique_lock<shared_mutex> lock(m_mtx);
    return remove_nolock(v);
}

#endif // __AVL_H__
