// btree.h

#ifndef BTREE_H
#define BTREE_H

#include <iostream>
#include <mutex>
#include <shared_mutex>
#include "BTreePage.h"
#include "bt_iterators.h"

#define DEFAULT_BTREE_ORDER 3

template <typename Trait>
class BTree
// this is the full version of the BTree
{
       using keyType   = typename Trait::keyType;
       using ObjIDType = typename Trait::ObjIDType;
       typedef CBTreePage <Trait> BTNode;// useful shorthand

public:
       //typedef ObjectInfo iterator;
       typedef typename BTNode::ObjectInfo      ObjectInfo;

public:
       BTree(T1 order = DEFAULT_BTREE_ORDER, flag unique = true);
       ~BTree();
       //int           Open (char * name, int mode);
       //int           Create (char * name, int mode);
       //int           Close ();
       flag            Insert (const keyType key, const T1 ObjID);
       flag            Remove (const keyType key, const T1 ObjID);
       ObjIDType       Search (const keyType key);
       Ref             size()  { shared_lock<shared_mutex> lock(m_mtx); return m_NumKeys; }
       Ref             height() { shared_lock<shared_mutex> lock(m_mtx); return m_Height; }
       Ref             GetOrder() { shared_lock<shared_mutex> lock(m_mtx); return m_Order; }

       // tipos que el iterador (general_iterator) necesita del contenedor
       using Page       = BTNode;
       using Node       = BTNode;
       using value_type = ObjectInfo;
       using iterator         = bt_inorder_iterator<BTree<Trait>, 1>;   // forward (Comp)
       using reverse_iterator = bt_inorder_iterator<BTree<Trait>, 0>;   // backward

       iterator begin() { return iterator(this, &m_Root, false); }
       iterator end()   { return iterator(this, &m_Root, true ); }
       reverse_iterator rbegin() { return reverse_iterator(this, &m_Root, false); }
       reverse_iterator rend()   { return reverse_iterator(this, &m_Root, true ); }

       // Un SOLO bucle (Traverse) sirve a ForEach y FirstThat (unificados)
       template <typename Pred>
       ObjectInfo* Traverse(Pred pred) {
               shared_lock<shared_mutex> lock(m_mtx);
               for( auto it = begin(); it != end(); ++it )
                       if( pred(*it, it.level()) )
                               return &(*it);
               return nullptr;
       }
       template <typename Func, typename... Args>
       void ForEach(Func func, Args&&... args) {
               Traverse([&](ObjectInfo &o, T1 lvl) { func(o, lvl, forward<Args>(args)...); return false; });
       }
       template <typename Func, typename... Args>
       ObjectInfo* FirstThat(Func func, Args&&... args) {
               return Traverse([&](ObjectInfo &o, T1 lvl) { return (flag)func(o, lvl, forward<Args>(args)...); });
       }

protected:
       BTNode          m_Root;
       T1              m_Height;  // height of tree
       T1              m_Order;   // order of tree
       Ref             m_NumKeys; // number of keys
       flag            m_Unique;  // Accept the elements only once ?
       mutable shared_mutex m_mtx; // control concurrente (igual que vector/linkedlist)
};

const T1 MaxHeight = 5;
template <typename Trait>
BTree<Trait>::BTree(T1 order, flag unique)
                               : m_Unique(unique),
                                 m_Order(order),
                                 m_Root(2 * order  + 1, unique),
                                 m_NumKeys(0)
{
       m_Root.SetMaxKeysForChilds(order);
       m_Height = 1;
}

template <typename Trait>
BTree<Trait>::~BTree()
{
}

template <typename Trait>
flag BTree<Trait>::Insert(const keyType key, const T1 ObjID)
{
       unique_lock<shared_mutex> lock(m_mtx);
       bt_ErrorCode error = m_Root.Insert(key, ObjID);
       if( error == bt_duplicate )
               return false;
       m_NumKeys++;
       if( error == bt_overflow )
       {
               m_Root.SplitRoot();
               m_Height++;
       }
       return true;
}

template <typename Trait>
flag BTree<Trait>::Remove (const keyType key, const T1 ObjID)
{
       unique_lock<shared_mutex> lock(m_mtx);
       bt_ErrorCode error = m_Root.Remove(key, ObjID);
       if( error == bt_duplicate || error == bt_nofound )
               return false;
       m_NumKeys--;

       if( error == bt_rootmerged )
               m_Height--;
       return true;
}

template <typename Trait>
typename BTree<Trait>::ObjIDType BTree<Trait>::Search (const keyType key)
{
       shared_lock<shared_mutex> lock(m_mtx);
       ObjIDType ObjID = -1;
       m_Root.Search(key, ObjID);
       return ObjID;
}


// operator<< : imprime el arbol via ForEach (reemplaza a Print)
template <typename Trait>
ostream& operator<<(ostream &os, BTree<Trait> &bt)
{
       bt.ForEach([&os](tagObjectInfo<Trait> &info, T1 level)
       {
               for( T1 i = 0; i < level; i++ )
                       os << "\t";
               os << info.key << "->" << info.ObjID << "\n";
       });
       return os;
}

// operator>> : lee claves del stream y las inserta (ObjID = orden de insercion)
template <typename Trait>
istream& operator>>(istream &is, BTree<Trait> &bt)
{
       typename Trait::keyType key;
       while( is >> key )
               bt.Insert(key, bt.size());
       return is;
}

#endif