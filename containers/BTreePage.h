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

    // Búsqueda binaria: devuelve la posición de la clave o donde debería ir.
    Size locate(const value_type& key) const {
        Size first = 0, last = m_keyCount;
        while (first < last) {
            Size mid = (first + last) / 2;
            if (key == m_keys[mid].m_data) return mid;
            if (key >  m_keys[mid].m_data) first = mid + 1;
            else                           last  = mid;
        }
        if (first < m_keyCount && key <= m_keys[first].m_data) return first;
        return last;
    }

    template <typename Container, typename Item>
    static void insertAt(Container& c, const Item& item, Size pos) {
        for (Size i = c.size() - 1; i > pos; --i) c[i] = c[i - 1];
        c[pos] = item;
    }
    template <typename Container>
    static void removeAt(Container& c, Size pos) {
        for (Size i = pos + 1; i < c.size(); ++i) c[i - 1] = c[i];
    }

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

    Size freeCells()    const { return m_maxKeys - m_keyCount; }
    Flag isFull()       const { return m_keyCount >= m_maxKeys; }
    Flag isOverflow()   const { return m_keyCount >  m_maxKeys; }
    Size minKeys()      const { return 2 * m_maxKeys / 3; }
    Flag isUnderflow()  const { return m_keyCount <  minKeys(); }
    Flag isRoot()       const { return m_maxKeysForChilds != m_maxKeys; }
    Size freeCellsOnLeft (Size pos) const { return pos > 0          ? m_subPages[pos-1]->freeCells() : 0; }
    Size freeCellsOnRight(Size pos) const { return pos < m_keyCount ? m_subPages[pos+1]->freeCells() : 0; }

    Flag redistribute1(Size&) { return false; }   // STUB temporal; versión real en Task 7

    void movePage(Page* child, std::vector<Entry>& tmpKeys, std::vector<Page*>& tmpSub) {
        Size n = child->m_keyCount, i = 0;
        for (; i < n; ++i) { tmpKeys.push_back(child->m_keys[i]); tmpSub.push_back(child->m_subPages[i]); }
        tmpSub.push_back(child->m_subPages[i]);
        child->clearKeys();
    }

    void splitInto3(std::vector<Entry>& tmpKeys, std::vector<Page*>& tmpSub,
                    Page*& c1, Page*& c2, Page*& c3, Entry& e1, Entry& e2) {
        if (!c1) c1 = new Page(m_maxKeysForChilds, m_unique);
        c1->clearKeys();
        Size nKeys = (tmpKeys.size() - 2) / 3, i = 0;
        for (; i < nKeys; ++i) { c1->m_keys[i] = tmpKeys[i]; c1->m_subPages[i] = tmpSub[i]; ++c1->m_keyCount; }
        c1->m_subPages[i] = tmpSub[i];
        e1 = tmpKeys[i++];

        if (!c2) c2 = new Page(m_maxKeysForChilds, m_unique);
        c2->clearKeys();
        nKeys += (tmpKeys.size() - 2) / 3 + 1;
        Size j = 0;
        for (; i < nKeys; ++i, ++j) { c2->m_keys[j] = tmpKeys[i]; c2->m_subPages[j] = tmpSub[i]; ++c2->m_keyCount; }
        c2->m_subPages[j] = tmpSub[i];
        e2 = tmpKeys[i++];

        if (!c3) c3 = new Page(m_maxKeysForChilds, m_unique);
        c3->clearKeys();
        nKeys = tmpKeys.size();
        for (j = 0; i < nKeys; ++i, ++j) { c3->m_keys[j] = tmpKeys[i]; c3->m_subPages[j] = tmpSub[i]; ++c3->m_keyCount; }
        c3->m_subPages[j] = tmpSub[i];
    }

    void splitChild(Size pos) {
        Page *c1 = nullptr, *c2 = nullptr;
        if (pos > 0 && m_subPages[pos-1]->isFull()) { c1 = m_subPages[pos-1]; c2 = m_subPages[pos--]; }
        if (pos < m_keyCount && m_subPages[pos+1]->isFull()) { c1 = m_subPages[pos]; c2 = m_subPages[pos+1]; }
        if (!c1) {   // fallback cuando redistribute1 está en stub y ningún hermano está lleno
            if (pos < m_keyCount) { c1 = m_subPages[pos]; c2 = m_subPages[pos + 1]; }
            else                  { c2 = m_subPages[pos]; c1 = m_subPages[--pos]; }
        }

        std::vector<Entry> tmpKeys; std::vector<Page*> tmpSub;
        movePage(c1, tmpKeys, tmpSub);
        tmpKeys.push_back(m_keys[pos]);
        movePage(c2, tmpKeys, tmpSub);

        Page* c3 = nullptr; Entry e1, e2;
        splitInto3(tmpKeys, tmpSub, c1, c2, c3, e1, e2);

        m_keys[pos] = e1; m_subPages[pos] = c1;
        insertAt(m_keys, e2, pos + 1);
        insertAt(m_subPages, c2, pos + 1);
        ++m_keyCount;
        m_subPages[pos + 2] = c3;
    }

    Flag splitRoot() {
        Page *c1 = nullptr, *c2 = nullptr, *c3 = nullptr; Entry e1, e2;
        splitInto3(m_keys, m_subPages, c1, c2, c3, e1, e2);
        clearKeys();
        m_keys[0] = e1; m_subPages[0] = c1; ++m_keyCount;
        m_keys[1] = e2; m_subPages[1] = c2; ++m_keyCount;
        m_subPages[2] = c3;
        return true;
    }

public:
    BTreePage(Size maxKeys, Flag unique = true)
        : m_keyCount(0), m_maxKeys(maxKeys), m_maxKeysForChilds(maxKeys), m_unique(unique) {
        create();
    }
    ~BTreePage() { reset(); }

    Size keyCount() const { return m_keyCount; }
    Size locateForTest(const value_type& key) const { return locate(key); }

    bt_ErrorCode insert(const value_type& key, Ref ref) {
        Size pos = locate(key);
        if (pos < m_keyCount && m_keys[pos].m_data == key && m_unique)
            return bt_ErrorCode::duplicate;
        if (!m_subPages[pos]) {                       // hoja
            insertAt(m_keys, Entry(key, ref), pos);
            ++m_keyCount;
            return isOverflow() ? bt_ErrorCode::overflow : bt_ErrorCode::ok;
        }
        auto error = m_subPages[pos]->insert(key, ref);
        if (error == bt_ErrorCode::duplicate) return bt_ErrorCode::duplicate;
        if (error == bt_ErrorCode::overflow) {
            if (!redistribute1(pos)) splitChild(pos);
            return isOverflow() ? bt_ErrorCode::overflow : bt_ErrorCode::ok;
        }
        return isOverflow() ? bt_ErrorCode::overflow : bt_ErrorCode::ok;
    }

    void setMaxKeysForChildsForTest(Size o) { setMaxKeysForChilds(o); }
    void splitRootForTest() { splitRoot(); }

    Flag search(const value_type& key, value_type& outValue, Ref& outRef) {
        Size pos = locate(key);
        if (pos >= m_keyCount)
            return m_subPages[pos] ? m_subPages[pos]->search(key, outValue, outRef) : false;
        if (m_keys[pos].m_data == key) {
            outValue = m_keys[pos].m_data;
            outRef   = m_keys[pos].m_ref;
            m_keys[pos].touch();
            return true;
        }
        if (key < m_keys[pos].m_data && m_subPages[pos])
            return m_subPages[pos]->search(key, outValue, outRef);
        return false;
    }
};

#endif // __BTREEPAGE_H__
