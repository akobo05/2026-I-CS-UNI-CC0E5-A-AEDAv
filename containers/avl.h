#ifndef __AVL_H__
#define __AVL_H__

#include <mutex>
#include <shared_mutex>
#include <utility>
#include <functional>
#include "BinaryTree.h"
#include "../types.h"
#include "traits.h"
using namespace std;

// Nodo del AVL: ES-UN BinaryTreeNode (hereda m_data, m_ref, hijos) y agrega la altura.
template <typename T>
struct AVLNode : public BinaryTreeNode<T, AVLNode<T>> {
    Depth m_height;
    AVLNode() : BinaryTreeNode<T, AVLNode<T>>(), m_height(1) {}
    AVLNode(T d, Ref r) : BinaryTreeNode<T, AVLNode<T>>(d, r), m_height(1) {}
};

template <typename T> struct AscendingAVLTrait  : BaseTrait<AVLNode<T>, less<T>>{};
template <typename T> struct DescendingAVLTrait : BaseTrait<AVLNode<T>, greater<T>>{};

// AVL: ES-UN BinaryTree. Reaprovecha por herencia insert/find/iterador/operator<</>>/
// copy-move/mutex, y SOBREESCRIBE el hook virtual internal_insert para rebalancear.
// Agrega rotaciones, rebalanceo y remove.
template <typename Trait>
class AVL : public BinaryTree<Trait> {
public:
    using Base       = BinaryTree<Trait>;
    using value_type = typename Base::value_type;
    using Node       = typename Base::Node;
    using Comp       = typename Base::Comp;
    using MySelf     = AVL<Trait>;

protected:
    Depth height(Node *n) const { return n ? n->m_height : 0; }
    Depth balance(Node *n) const { return n ? height(n->child(0)) - height(n->child(1)) : 0; }
    void  updateHeight(Node *n){
        Depth lh = height(n->child(0));
        Depth rh = height(n->child(1));
        n->m_height = 1 + (lh > rh ? lh : rh);
    }
    Node* rotateLeft(Node *x){
        Node *y  = x->child(1);
        Node *T2 = y->child(0);
        y->child(0) = x;
        x->child(1) = T2;
        updateHeight(x);
        updateHeight(y);
        return y;
    }
    Node* rotateRight(Node *y){
        Node *x  = y->child(0);
        Node *T2 = x->child(1);
        x->child(1) = y;
        y->child(0) = T2;
        updateHeight(y);
        updateHeight(x);
        return x;
    }
    void rebalance(Node* &n){
        Depth bal = balance(n);
        if(bal > 1){                                        // pesa a la izquierda
            if(balance(n->child(0)) < 0)                    // caso LR
                n->child(0) = rotateLeft(n->child(0));
            n = rotateRight(n);                             // LL (y final del LR)
        } else if(bal < -1){                                // pesa a la derecha
            if(balance(n->child(1)) > 0)                    // caso RL
                n->child(1) = rotateRight(n->child(1));
            n = rotateLeft(n);                              // RR (y final del RL)
        }
    }

    // OVERRIDE del hook virtual: inserta como BST y luego rebalancea al volver.
    void internal_insert(Node* &pNode, const value_type &data, Ref ref, bool &inserted) override {
        if(!pNode){ pNode = new Node(data, ref); inserted = true; return; }
        bool less_v = this->m_comp(data, pNode->m_data);
        bool less_n = this->m_comp(pNode->m_data, data);
        if(!less_v && !less_n){ pNode->m_ref = ref; inserted = false; return; }  // duplicado
        internal_insert(pNode->child(less_v ? 0 : 1), data, ref, inserted);
        updateHeight(pNode);
        rebalance(pNode);
    }

    void internal_remove(Node* &pNode, const value_type &v, bool &removed){
        if(!pNode){ removed = false; return; }
        if(this->m_comp(v, pNode->m_data)){
            internal_remove(pNode->child(0), v, removed);
        } else if(this->m_comp(pNode->m_data, v)){
            internal_remove(pNode->child(1), v, removed);
        } else {
            removed = true;
            if(!pNode->child(0) || !pNode->child(1)){          // 0 o 1 hijo
                Node *child = pNode->child(0) ? pNode->child(0) : pNode->child(1);
                delete pNode;
                pNode = child;
            } else {                                            // 2 hijos: sucesor inorder
                Node *succ = pNode->child(1);
                while(succ->child(0)) succ = succ->child(0);
                pNode->m_data = succ->m_data;
                pNode->m_ref  = succ->m_ref;
                bool d = false;
                internal_remove(pNode->child(1), succ->m_data, d);
            }
        }
        if(!pNode) return;
        updateHeight(pNode);
        rebalance(pNode);
    }

public:
    AVL() : Base() {}

    AVL(const AVL &o) : Base() {
        shared_lock<shared_mutex> lk(o.m_mtx);
        o.internal_inorder(o.m_pRoot, [this](const Node &n){     // re-inserta -> AVL balanceado
            bool ins = false;
            internal_insert(this->m_pRoot, n.m_data, n.m_ref, ins);
            if(ins) ++this->m_size;
        });
    }
    AVL(AVL &&o) noexcept : Base(std::move(o)) {}                 // roba el arbol (ya balanceado)

    AVL& operator=(const AVL &o){
        if(this == &o) return *this;
        std::scoped_lock lock(this->m_mtx, o.m_mtx);
        this->internal_clear(this->m_pRoot); this->m_pRoot = nullptr; this->m_size = 0;
        o.internal_inorder(o.m_pRoot, [this](const Node &n){
            bool ins = false;
            internal_insert(this->m_pRoot, n.m_data, n.m_ref, ins);
            if(ins) ++this->m_size;
        });
        return *this;
    }
    AVL& operator=(AVL &&o) noexcept{
        Base::operator=(std::move(o));
        return *this;
    }
    virtual ~AVL() {}     // el destructor de BinaryTree libera los nodos

    bool remove(const value_type &v){
        unique_lock<shared_mutex> lk(this->m_mtx);
        return remove_nolock(v);
    }
    bool remove_nolock(const value_type &v){
        bool removed = false;
        internal_remove(this->m_pRoot, v, removed);
        if(removed) --this->m_size;
        return removed;
    }
};

#endif // __AVL_H__
