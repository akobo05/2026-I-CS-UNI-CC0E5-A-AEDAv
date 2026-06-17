#ifndef __HASHTABLE_H__
#define __HASHTABLE_H__

#include <iostream>
#include <istream>
#include <ostream>
#include <mutex>
#include <shared_mutex>
#include <functional>
#include "avl.h"
#include "../types.h"
#include "traits.h"
using namespace std;

// Par clave-valor. Se compara SOLO por la clave (first): el AVL ordena por key.
template <typename K, typename V>
struct KVPair {
    K first;
    V second;
    KVPair() : first(K()), second(V()) {}
    KVPair(const K &k, const V &v) : first(k), second(v) {}
    bool operator< (const KVPair &o) const { return first <  o.first; }
    bool operator> (const KVPair &o) const { return first >  o.first; }
    bool operator==(const KVPair &o) const { return first == o.first; }

    // Serializacion del par: "key=>value". La usan el operator<< / >> del arbol.
    friend ostream& operator<<(ostream &os, const KVPair &kv){
        return os << kv.first << "=>" << kv.second;
    }
    friend istream& operator>>(istream &is, KVPair &kv){
        char a, b;                 // los dos caracteres de "=>"
        is >> kv.first >> a >> b >> kv.second;
        return is;
    }
};

// HashTable: CONTIENE un AVL de pares ordenado por clave (composicion).
// El AVL (que a su vez hereda de BinaryTree) aporta insert/find/iterador/
// operator<</>>/copy/move. HashTable agrega operator[], contains y delega lo demas.
template <typename K, typename V>
class HashTable {
public:
    using key_type    = K;
    using mapped_type = V;
    using kv_type     = KVPair<K, V>;
    using Tree        = AVL<AscendingAVLTrait<kv_type>>;
    using Node        = typename Tree::Node;

private:
    Tree                 m_tree;     // el AVL contenido
    mutable shared_mutex m_mtx;

public:
    HashTable() = default;
    ~HashTable() = default;

    HashTable(const HashTable &o){
        shared_lock<shared_mutex> lk(o.m_mtx);
        m_tree = o.m_tree;                          // copia profunda del AVL
    }
    HashTable(HashTable &&o) noexcept {
        unique_lock<shared_mutex> lk(o.m_mtx);
        m_tree = std::move(o.m_tree);
    }
    HashTable& operator=(const HashTable &o){
        if(this != &o){
            std::scoped_lock lock(m_mtx, o.m_mtx);
            m_tree = o.m_tree;
        }
        return *this;
    }
    HashTable& operator=(HashTable &&o) noexcept {
        if(this != &o){
            std::scoped_lock lock(m_mtx, o.m_mtx);
            m_tree = std::move(o.m_tree);
        }
        return *this;
    }

    // operator[]: si la clave existe devuelve referencia a su valor; si no, la
    // inserta con valor por defecto y devuelve la referencia (estilo std::map).
    mapped_type& operator[](const key_type &key){
        unique_lock<shared_mutex> lock(m_mtx);      // un solo lock para buscar+insertar
        kv_type probe(key, mapped_type());
        kv_type *found = m_tree.find_data_nolock(probe);
        if(found) return found->second;
        m_tree.insert_nolock(probe, Ref());          // insert del AVL (rebalancea)
        return m_tree.find_data_nolock(probe)->second;
    }

    bool contains(const key_type &key) const {
        shared_lock<shared_mutex> lock(m_mtx);
        Ref dummy;
        return m_tree.find_nolock(kv_type(key, mapped_type()), dummy);
    }

    Index size() const { return m_tree.size(); }
    bool  empty() const { return m_tree.empty(); }

    // Recorrido: delega en el iterador in-order del AVL (-> for con structured binding)
    auto begin() const { return m_tree.begin(); }
    auto end()   const { return m_tree.end(); }

    // operator<< y operator>> REAPROVECHADOS del AVL (delegan en el arbol contenido)
    friend std::ostream& operator<<(std::ostream &os, const HashTable &t){
        return os << t.m_tree;
    }
    friend std::istream& operator>>(std::istream &is, HashTable &t){
        return is >> t.m_tree;
    }
};

#endif // __HASHTABLE_H__
