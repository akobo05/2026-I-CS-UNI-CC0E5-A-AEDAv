#ifndef __BTREEPAGE_H__
#define __BTREEPAGE_H__

#include <vector>
#include <utility>
#include <iostream>
#include "../types.h"
#include "traits.h"

template <typename Trait> class BTree;

enum class bt_ErrorCode { ok, overflow, underflow, duplicate, notFound, rootMerged };

// Entry: dato + referencia + contador de accesos. Serializa con tokens (no char).
template <typename Value>
struct BTreeEntry {
    using value_type = Value;
    Value m_data;
    Ref   m_ref;
    Size  m_useCount;

    BTreeEntry() : m_data(Value{}), m_ref(Ref{}), m_useCount(0) {}
    BTreeEntry(const Value& data, Ref ref) : m_data(data), m_ref(ref), m_useCount(0) {}

    Size touch()          { return ++m_useCount; }
    Size useCount() const { return m_useCount; }
    Flag operator< (const BTreeEntry& o) const { return m_data <  o.m_data; }
    Flag operator> (const BTreeEntry& o) const { return m_data >  o.m_data; }
    Flag operator==(const BTreeEntry& o) const { return m_data == o.m_data; }

    // Delimitador " : " leído como token std::string (multibyte-safe)
    friend std::ostream& operator<<(std::ostream& os, const BTreeEntry& e) {
        return os << e.m_data << " : " << e.m_ref;
    }
    friend std::istream& operator>>(std::istream& is, BTreeEntry& e) {
        std::string sep;                       // consume el ":"
        return is >> e.m_data >> sep >> e.m_ref;
    }
};

// Página m-vías. Order y value_type salen del Trait. Sin native types, sin using namespace std.
template <typename Trait>
class BTreePage {
public:
    using value_type            = typename Trait::value_type;
    using Comp                  = typename Trait::Comp;
    static constexpr Size Order = Trait::Order;
    using Entry                 = BTreeEntry<value_type>;
    using Page                  = BTreePage<Trait>;

private:
    friend class BTree<Trait>;
    Comp m_comp{};
    std::vector<Entry> m_keys;
    std::vector<Page*> m_subPages;
    Size m_keyCount;
    Size m_maxKeys;
    Size m_maxKeysForChilds;
    Flag m_unique;

    void create() {
        m_keys.assign(m_maxKeys + 1, Entry{});
        m_subPages.assign(m_maxKeys + 2, nullptr);
        m_keyCount = 0;
    }
    void clearKeys() { m_keyCount = 0; }
    void reset() {
        for (Size i = 0; i <= m_keyCount; ++i) delete m_subPages[i];  // i <= keyCount: incluye hijo derecho
        m_subPages.assign(m_subPages.size(), nullptr);
        clearKeys();
    }
    void destroy() { reset(); delete this; }
    void setMaxKeysForChilds(Size order) { m_maxKeysForChilds = order; }

public:
    BTreePage(Size maxKeys, Flag unique = true)
        : m_keyCount(0), m_maxKeys(maxKeys), m_maxKeysForChilds(maxKeys), m_unique(unique) {
        create();
    }
    ~BTreePage() { reset(); }

    Size keyCount() const { return m_keyCount; }
};

#endif // __BTREEPAGE_H__
