#ifndef __HASHTABLE_H__
#define __HASHTABLE_H__

#include <iostream>
#include <istream>
#include <ostream>
#include <string>
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

    // Serializacion "clave => valor". El "=>" se lee como TOKEN (std::string),
    // no como char, para soportar simbolos multibyte.
    friend ostream& operator<<(ostream &os, const KVPair &kv){
        return os << kv.first << " => " << kv.second;
    }
    friend istream& operator>>(istream &is, KVPair &kv){
        string arrow;                         // lee el token "=>"
        is >> kv.first >> arrow >> kv.second;
        return is;
    }
};

// Trait de la HashTable: de aqui sale TODO (clave, valor, par, y el tipo de arbol).
// La clase HashTable no nombra AVL ni Node; los toma del Trait.
template <typename K, typename V>
struct HashTrait {
    using key_type    = K;
    using mapped_type = V;
    using kv_type     = KVPair<K, V>;
    using tree_type   = AVL<AscendingAVLTrait<kv_type>>;
};

// HashTable<Trait>: CONTIENE un arbol (composicion). Todo lo de los tipos viene del
// Trait. El arbol (AVL : BinaryTree) aporta insert/find/iterador/operator<</>>/
// copy/move. HashTable solo agrega operator[] y contains.
template <typename Trait>
class HashTable {
public:
    using key_type    = typename Trait::key_type;
    using mapped_type = typename Trait::mapped_type;
    using kv_type     = typename Trait::kv_type;
    using tree_type   = typename Trait::tree_type;

private:
    tree_type            m_tree;     // el arbol contenido (sale del Trait)
    mutable shared_mutex m_mtx;

public:
    HashTable() = default;
    ~HashTable() = default;

    HashTable(const HashTable &o){
        shared_lock<shared_mutex> lk(o.m_mtx);
        m_tree = o.m_tree;
    }
    HashTable(HashTable &&o) noexcept {
        unique_lock<shared_mutex> lk(o.m_mtx);
        m_tree = std::move(o.m_tree);
    }
    HashTable& operator=(const HashTable &o){
        if(this != &o){ std::scoped_lock lock(m_mtx, o.m_mtx); m_tree = o.m_tree; }
        return *this;
    }
    HashTable& operator=(HashTable &&o) noexcept {
        if(this != &o){ std::scoped_lock lock(m_mtx, o.m_mtx); m_tree = std::move(o.m_tree); }
        return *this;
    }

    // operator[]: si la clave existe devuelve referencia a su valor; si no, la
    // inserta con valor por defecto y devuelve la referencia (estilo std::map).
    mapped_type& operator[](const key_type &key){
        unique_lock<shared_mutex> lock(m_mtx);
        kv_type probe(key, mapped_type());
        kv_type *found = m_tree.find_data_nolock(probe);
        if(found) return found->second;
        m_tree.insert_nolock(probe, Ref());
        return m_tree.find_data_nolock(probe)->second;
    }

    bool contains(const key_type &key) const {
        shared_lock<shared_mutex> lock(m_mtx);
        Ref dummy;
        return m_tree.find_nolock(kv_type(key, mapped_type()), dummy);
    }

    Index size() const { return m_tree.size(); }
    bool  empty() const { return m_tree.empty(); }

    auto begin() const { return m_tree.begin(); }
    auto end()   const { return m_tree.end(); }

    // operator<< y operator>> REAPROVECHADOS del arbol contenido (delegan)
    friend std::ostream& operator<<(std::ostream &os, const HashTable &t){ return os << t.m_tree; }
    friend std::istream& operator>>(std::istream &is, HashTable &t){ return is >> t.m_tree; }
};

#endif // __HASHTABLE_H__
