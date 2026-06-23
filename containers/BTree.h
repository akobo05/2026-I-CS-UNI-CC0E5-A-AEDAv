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
#include <vector>
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

    class Iterator {
        std::vector<std::pair<Page*, Size>> m_stack;
        void pushPath(Page* page, Size idx) {
            while (page && page->m_keyCount > 0) {
                m_stack.push_back({page, idx});
                page = page->m_subPages[idx];
                idx = 0;
            }
        }
    public:
        Iterator() = default;
        explicit Iterator(Page* root) { pushPath(root, 0); }
        Entry& operator*() const { return m_stack.back().first->m_keys[m_stack.back().second]; }
        Iterator& operator++() {
            auto [page, idx] = m_stack.back();
            m_stack.pop_back();
            if (idx + 1 < page->m_keyCount) m_stack.push_back({page, idx + 1});
            if (Page* right = page->m_subPages[idx + 1]) pushPath(right, 0);
            return *this;
        }
        Flag operator==(const Iterator& o) const { return m_stack == o.m_stack; }
        Flag operator!=(const Iterator& o) const { return !(*this == o); }
    };
    Iterator begin() const { return Iterator(m_pRoot); }
    Iterator end()   const { return Iterator(); }

    // Variádicos: delegan en la raíz bajo UN shared_lock (recorrido atómico)
    template <typename Func, typename... Args>
    void forEach(Func func, Args&&... args) const {
        std::shared_lock<std::shared_mutex> lk(m_mtx);
        m_pRoot->forEach(0, func, std::forward<Args>(args)...);
    }
    template <typename Func, typename... Args>
    Entry* firstThat(Func func, Args&&... args) {
        std::unique_lock<std::shared_mutex> lk(m_mtx);   // devuelve Entry* mutable -> lock exclusivo
        return m_pRoot->firstThat(0, func, std::forward<Args>(args)...);
    }
    template <typename Func, typename... Args>
    void forEachPage(Func func, Args&&... args) const {
        std::shared_lock<std::shared_mutex> lk(m_mtx);
        m_pRoot->forEachPage(0, func, std::forward<Args>(args)...);
    }

    std::string toString() const {
        std::ostringstream os; os << *this; return os.str();
    }

    // V3: delimitadores como TOKENS std::string. Formato: "{ ( <data> , <ref> ) ... }"
    friend std::ostream& operator<<(std::ostream& os, const BTree& t) {
        os << "{";
        t.forEach([&os](Entry& e, Level){ os << " ( " << e.m_data << " , " << e.m_ref << " )"; });
        os << " }";
        return os;
    }
    friend std::istream& operator>>(std::istream& is, BTree& t) {
        std::string tok;
        if (!(is >> tok) || tok != "{") { is.clear(std::ios_base::failbit); return is; }
        while (is >> tok && tok != "}") {
            if (tok == "(") {
                value_type v; Ref r; std::string comma, close;
                if (is >> v >> comma >> r >> close)
                    if (comma == "," && close == ")")
                        t.insert(v, r);
            }
        }
        return is;
    }
};

#endif // __BTREE_H__
