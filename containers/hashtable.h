#ifndef __HASHTABLE_H__
#define __HASHTABLE_H__

#include <iostream>
#include <string>
#include <sstream>
#include <stdexcept>
#include <mutex>
#include <shared_mutex>
#include <utility>
#include <functional>
#include <type_traits>
#include "vector.h"
#include "avl.h"
#include "../types.h"
#include "traits.h"
using namespace std;

template <typename K, typename V>
struct KVPair {
    K first;
    V second;
    KVPair() : first(K()), second(V()) {}
    KVPair(const K &k, const V &v) : first(k), second(v) {}
    bool operator< (const KVPair &o) const { return first <  o.first; }
    bool operator> (const KVPair &o) const { return first >  o.first; }
    bool operator==(const KVPair &o) const { return first == o.first; }
    friend std::ostream& operator<<(std::ostream &os, const KVPair &kv){
        return os << kv.first << "=>" << kv.second;
    }
};

// Knuth multiplicativo. Para tipos integrales usa el key directo;
// para no integrales (string, etc.) delega en std::hash<K> y luego aplica Knuth.
template <typename K>
struct KnuthMultiplicativeHash {
    HashValue operator()(const K &key, BucketCount nBuckets) const {
        HashValue raw;
        if constexpr (std::is_integral_v<K>)
            raw = HashValue(key) * 2654435761L;
        else
            raw = HashValue(std::hash<K>{}(key)) * 2654435761L;
        if(raw < 0) raw = -raw;
        return raw % nBuckets;
    }
};

template <typename K, typename V>
struct DefaultHashTrait {
    using key_type    = K;
    using value_type  = V;
    using kv_type     = KVPair<K, V>;
    using hash_fn     = KnuthMultiplicativeHash<K>;
    using bucket_type = AVL<AscendingAVLTrait<kv_type>>;
    static constexpr BucketCount initial_buckets       = 16;
    static constexpr long        load_factor_max_x100  = 75;
    static constexpr BucketCount growth_factor          = 2;
};

template <typename HashTraitT>
struct HashBucketsVectorTrait : BaseTrait<
    VectorNode<typename HashTraitT::bucket_type>,
    less<typename HashTraitT::bucket_type>>{};

template <typename Trait>
class HashTable {
public:
    using key_type    = typename Trait::key_type;
    using value_type  = typename Trait::value_type;
    using kv_type     = typename Trait::kv_type;
    using bucket_type = typename Trait::bucket_type;
    using hash_fn     = typename Trait::hash_fn;
    using MySelf      = HashTable<Trait>;

private:
    using BucketsVec = Vector<HashBucketsVectorTrait<Trait>>;
    BucketsVec   m_buckets;
    BucketCount  m_count;
    hash_fn      m_hash;
    mutable shared_mutex m_mtx;

    HashValue bucket_index_of(const key_type &k) const {
        return m_hash(k, m_buckets.size());
    }
    void rehash_to(BucketCount newCount);
    void maybe_rehash();

public:
    HashTable();
    HashTable(const HashTable &other);
    HashTable(HashTable &&other) noexcept;
    HashTable& operator=(const HashTable &other);
    HashTable& operator=(HashTable &&other) noexcept;
    ~HashTable();

    value_type& operator[](const key_type &k);
    bool        contains(const key_type &k) const;
    bool        erase(const key_type &k);
    Index       size() const;
    bool        empty() const;
    BucketCount bucket_count() const;

    class iterator {
    public:
        HashTable *m_owner;
        Index m_bucket;
        typename bucket_type::iterator m_inner;

        iterator() : m_owner(nullptr), m_bucket(0) {}
        iterator(HashTable *owner, Index bucket, typename bucket_type::iterator inner)
            : m_owner(owner), m_bucket(bucket), m_inner(inner) {}

        void advance_to_next_nonempty(){
            BucketCount nb = m_owner->m_buckets.size();
            while(m_bucket < nb){
                bucket_type &b = m_owner->m_buckets[m_bucket];
                if(m_inner != b.end_nolock()) return;
                ++m_bucket;
                if(m_bucket < nb)
                    m_inner = m_owner->m_buckets[m_bucket].begin_nolock();
            }
            m_inner = typename bucket_type::iterator();
        }
        iterator& operator++(){
            ++m_inner;
            advance_to_next_nonempty();
            return *this;
        }
        kv_type& operator*()  { return (*m_inner).m_data; }
        kv_type* operator->() { return &(*m_inner).m_data; }
        bool operator==(const iterator &o) const {
            return m_owner == o.m_owner && m_bucket == o.m_bucket && m_inner == o.m_inner;
        }
        bool operator!=(const iterator &o) const { return !(*this == o); }
    };

    iterator begin() {
        shared_lock<shared_mutex> lock(m_mtx);
        if(m_buckets.size() == 0) return iterator(this, 0, typename bucket_type::iterator());
        iterator it(this, 0, m_buckets[0].begin_nolock());
        it.advance_to_next_nonempty();
        return it;
    }
    iterator end() {
        shared_lock<shared_mutex> lock(m_mtx);
        return iterator(this, m_buckets.size(), typename bucket_type::iterator());
    }

    friend std::ostream& operator<<(std::ostream &os, const HashTable &t){
        shared_lock<shared_mutex> lock(t.m_mtx);
        os << "[";
        bool first = true;
        for(BucketCount i = 0; i < t.m_buckets.size(); ++i){
            const bucket_type &b = t.m_buckets[i];
            b.inorder_nolock([&](const typename bucket_type::Node &n){
                if(!first) os << ",";
                os << "(" << n.m_data.first << "," << n.m_data.second << ")";
                first = false;
            });
        }
        os << "]";
        return os;
    }

    friend std::istream& operator>>(std::istream &is, HashTable &t){
        char ch;
        if(!(is >> ch) || ch != '['){ is.clear(ios_base::failbit); return is; }
        key_type k; value_type v; char comma, parenClose;
        while(is >> ch && ch != ']'){
            if(ch == '('){
                if(is >> k >> comma >> v >> parenClose){
                    if(comma == ',' && parenClose == ')')
                        t[k] = v;
                }
            }
        }
        return is;
    }
};

template <typename Trait>
HashTable<Trait>::HashTable() : m_buckets(Trait::initial_buckets), m_count(0) {
    for(BucketCount i = 0; i < Trait::initial_buckets; ++i)
        m_buckets.push_back(bucket_type(), Ref(i));
}

template <typename Trait>
HashTable<Trait>::HashTable(const HashTable &other) : m_buckets(other.m_buckets.size()), m_count(0) {
    shared_lock<shared_mutex> lock(other.m_mtx);
    for(BucketCount i = 0; i < other.m_buckets.size(); ++i)
        m_buckets.push_back(bucket_type(), Ref(i));
    for(BucketCount i = 0; i < other.m_buckets.size(); ++i){
        const bucket_type &src = other.m_buckets[i];
        src.inorder([this, i](const typename bucket_type::Node &n){
            m_buckets[i].insert_nolock(n.m_data, n.m_ref);
        });
    }
    m_count = other.m_count;
}

template <typename Trait>
HashTable<Trait>::HashTable(HashTable &&other) noexcept
    : m_buckets(std::move(other.m_buckets)), m_count(0)
{
    unique_lock<shared_mutex> lock(other.m_mtx);
    m_count = std::exchange(other.m_count, 0);
}

template <typename Trait>
HashTable<Trait>& HashTable<Trait>::operator=(const HashTable &other){
    if(this == &other) return *this;
    HashTable tmp(other);
    unique_lock<shared_mutex> lock(m_mtx);
    m_buckets = std::move(tmp.m_buckets);
    m_count   = tmp.m_count;
    return *this;
}

template <typename Trait>
HashTable<Trait>& HashTable<Trait>::operator=(HashTable &&other) noexcept{
    if(this == &other) return *this;
    unique_lock<shared_mutex> lk_this(m_mtx);
    unique_lock<shared_mutex> lk_other(other.m_mtx);
    m_buckets = std::move(other.m_buckets);
    m_count   = std::exchange(other.m_count, 0);
    return *this;
}

template <typename Trait>
HashTable<Trait>::~HashTable() {}

template <typename Trait>
Index HashTable<Trait>::size() const{
    shared_lock<shared_mutex> lock(m_mtx);
    return Index(m_count);
}

template <typename Trait>
bool HashTable<Trait>::empty() const{
    return size() == 0;
}

template <typename Trait>
BucketCount HashTable<Trait>::bucket_count() const{
    shared_lock<shared_mutex> lock(m_mtx);
    return m_buckets.size();
}

template <typename Trait>
void HashTable<Trait>::rehash_to(BucketCount newCount){
    BucketsVec newBuckets(newCount);
    for(BucketCount i = 0; i < newCount; ++i)
        newBuckets.push_back(bucket_type(), Ref(i));
    for(BucketCount i = 0; i < m_buckets.size(); ++i){
        bucket_type &src = m_buckets[i];
        src.inorder_nolock([&](const typename bucket_type::Node &n){
            HashValue idx = m_hash(n.m_data.first, newCount);
            newBuckets[idx].insert_nolock(n.m_data, n.m_ref);
        });
    }
    m_buckets = std::move(newBuckets);
}

template <typename Trait>
void HashTable<Trait>::maybe_rehash(){
    BucketCount nb = m_buckets.size();
    if((m_count + 1) * 100 > nb * Trait::load_factor_max_x100){
        rehash_to(nb * Trait::growth_factor);
    }
}

template <typename Trait>
typename HashTable<Trait>::value_type& HashTable<Trait>::operator[](const key_type &k){
    unique_lock<shared_mutex> lock(m_mtx);
    Ref dummy;
    HashValue idx = bucket_index_of(k);
    kv_type probe(k, value_type());
    if(m_buckets[idx].find_nolock(probe, dummy)){
        // existe — re-busca el nodo y retorna referencia al value
        bucket_type &bucket = m_buckets[idx];
        value_type *result = nullptr;
        bucket.inorder_nolock([&](typename bucket_type::Node &n){
            if(!(k < n.m_data.first) && !(n.m_data.first < k))
                result = &n.m_data.second;
        });
        if(result) return *result;
    }
    // no existe — chequea rehash ANTES de insertar
    maybe_rehash();
    idx = bucket_index_of(k);   // puede haber cambiado tras rehash
    m_buckets[idx].insert_nolock(kv_type(k, value_type()), Ref(0));
    ++m_count;
    bucket_type &bucket = m_buckets[idx];
    value_type *result = nullptr;
    bucket.inorder_nolock([&](typename bucket_type::Node &n){
        if(!(k < n.m_data.first) && !(n.m_data.first < k))
            result = &n.m_data.second;
    });
    if(!result) throw runtime_error("HashTable::operator[] insert fallo");
    return *result;
}

template <typename Trait>
bool HashTable<Trait>::contains(const key_type &k) const{
    shared_lock<shared_mutex> lock(m_mtx);
    HashValue idx = m_hash(k, m_buckets.size());
    Ref dummy;
    return const_cast<bucket_type&>(m_buckets[idx]).find_nolock(kv_type(k, value_type()), dummy);
}

template <typename Trait>
bool HashTable<Trait>::erase(const key_type &k){
    unique_lock<shared_mutex> lock(m_mtx);
    HashValue idx = bucket_index_of(k);
    bool removed = m_buckets[idx].remove_nolock(kv_type(k, value_type()));
    if(removed) --m_count;
    return removed;
}

#endif // __HASHTABLE_H__
