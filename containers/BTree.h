#ifndef __BTREE_H__
#define __BTREE_H__

#include <iostream>
#include <sstream>
#include <string>
#include <tuple>
#include <stdexcept>
#include <shared_mutex>
#include <mutex>
#include <utility>
#include "../types.h"
#include "traits.h"
#include "BTreePage.h"

template <typename Trait>
class BTree {
public:
    using value_type            = typename Trait::value_type;
    using Comp                  = typename Trait::Comp;
    static constexpr Size Order = Trait::Order;
    using Page  = BTreePage<Trait>;
    using Entry = typename Page::Entry;

private:
    Page* m_pRoot;
    Level m_height;
    Flag  m_unique;
    Size  m_numKeys;
    mutable std::shared_mutex m_mtx;

    Page* deepCopy(Page* src) const {
        if (!src) return nullptr;
        Page* dst = new Page(src->m_maxKeys, src->m_unique);
        dst->m_maxKeysForChilds = src->m_maxKeysForChilds;
        dst->m_keyCount = src->m_keyCount;
        dst->m_keys     = src->m_keys;
        for (Size i = 0; i <= src->m_keyCount; ++i) dst->m_subPages[i] = deepCopy(src->m_subPages[i]);
        return dst;
    }

public:
    explicit BTree(Flag unique = true)
        : m_pRoot(new Page(2 * Order + 1, unique)), m_height(1), m_unique(unique), m_numKeys(0) {
        m_pRoot->setMaxKeysForChilds(Order);
    }
    BTree(const BTree& o) : m_pRoot(nullptr), m_height(1), m_unique(true), m_numKeys(0) {
        std::shared_lock<std::shared_mutex> lk(o.m_mtx);
        m_pRoot = deepCopy(o.m_pRoot); m_height = o.m_height; m_unique = o.m_unique; m_numKeys = o.m_numKeys;
    }
    BTree(BTree&& o) noexcept : m_pRoot(nullptr), m_height(1), m_unique(true), m_numKeys(0) {
        std::unique_lock<std::shared_mutex> lk(o.m_mtx);
        m_pRoot   = std::exchange(o.m_pRoot, nullptr);
        m_height  = std::exchange(o.m_height, 0);
        m_unique  = o.m_unique;
        m_numKeys = std::exchange(o.m_numKeys, 0);
    }
    BTree& operator=(const BTree& o) {
        if (this != &o) {
            std::unique_lock<std::shared_mutex> lk(m_mtx);
            std::shared_lock<std::shared_mutex> lo(o.m_mtx);
            delete m_pRoot;
            m_pRoot = deepCopy(o.m_pRoot); m_height = o.m_height; m_unique = o.m_unique; m_numKeys = o.m_numKeys;
        }
        return *this;
    }
    BTree& operator=(BTree&& o) noexcept {
        if (this != &o) {
            std::scoped_lock lk(m_mtx, o.m_mtx);
            delete m_pRoot;
            m_pRoot   = std::exchange(o.m_pRoot, nullptr);
            m_height  = std::exchange(o.m_height, 0);
            m_unique  = o.m_unique;
            m_numKeys = std::exchange(o.m_numKeys, 0);
        }
        return *this;
    }
    ~BTree() { delete m_pRoot; }

    Flag insert(const value_type& key, Ref ref) {
        std::unique_lock<std::shared_mutex> lk(m_mtx);
        auto error = m_pRoot->insert(key, ref);
        if (error == bt_ErrorCode::duplicate) return false;
        ++m_numKeys;
        if (error == bt_ErrorCode::overflow) { m_pRoot->splitRoot(); ++m_height; }
        return true;
    }
    std::tuple<value_type, Ref> remove(const value_type& key) {
        std::unique_lock<std::shared_mutex> lk(m_mtx);
        value_type outValue{}; Ref outRef{};
        auto error = m_pRoot->remove(key, outValue, outRef);
        if (error == bt_ErrorCode::notFound) throw std::runtime_error("BTree::remove - clave no encontrada");
        --m_numKeys;
        if (error == bt_ErrorCode::rootMerged) --m_height;
        return {outValue, outRef};
    }
    std::tuple<value_type, Ref> search(const value_type& key) const {
        std::shared_lock<std::shared_mutex> lk(m_mtx);
        value_type outValue{}; Ref outRef{};
        if (!m_pRoot->search(key, outValue, outRef)) throw std::runtime_error("BTree::search - clave no encontrada");
        return {outValue, outRef};
    }
    Flag contains(const value_type& key) const {
        std::shared_lock<std::shared_mutex> lk(m_mtx);
        value_type v{}; Ref r{};
        return m_pRoot->search(key, v, r);
    }
    Size  size()   const { std::shared_lock<std::shared_mutex> lk(m_mtx); return m_numKeys; }
    Level height() const { std::shared_lock<std::shared_mutex> lk(m_mtx); return m_height; }
    Size  order()  const { return Order; }
    Flag  empty()  const { return size() == 0; }
};

#endif // __BTREE_H__
