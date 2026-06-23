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

    void redistributeR2L(Size pos) {
        Page *src = m_subPages[pos], *dst = m_subPages[pos-1];
        while (src->m_keyCount > src->minKeys() && dst->m_keyCount < src->m_keyCount) {
            insertAt(dst->m_keys, m_keys[pos-1], dst->m_keyCount++);
            insertAt(dst->m_subPages, src->m_subPages[0], dst->m_keyCount);
            m_keys[pos-1] = src->m_keys[0];
            removeAt(src->m_keys, 0);
            removeAt(src->m_subPages, 0);
            --src->m_keyCount;
        }
    }
    void redistributeL2R(Size pos) {
        Page *src = m_subPages[pos], *dst = m_subPages[pos+1];
        while (src->m_keyCount > src->minKeys() && dst->m_keyCount < src->m_keyCount) {
            insertAt(dst->m_keys, m_keys[pos], 0);
            insertAt(dst->m_subPages, src->m_subPages[src->m_keyCount], 0);
            ++dst->m_keyCount;
            m_keys[pos] = src->m_keys[src->m_keyCount - 1];
            --src->m_keyCount;
        }
    }
    Flag redistribute1(Size& pos) {
        if (m_subPages[pos]->isUnderflow()) {
            Size nkLeft  = pos > 0          ? m_subPages[pos-1]->m_keyCount : 0;
            Size nkRight = pos < m_keyCount ? m_subPages[pos+1]->m_keyCount : 0;
            if (nkLeft > nkRight) {
                if (m_subPages[pos-1]->m_keyCount > m_subPages[pos-1]->minKeys()) redistributeL2R(pos - 1);
                else if (pos == m_keyCount) { --pos; return false; }
                else return false;
            } else {
                if (m_subPages[pos+1]->m_keyCount > m_subPages[pos+1]->minKeys()) redistributeR2L(pos + 1);
                else if (pos == 0) { ++pos; return false; }
                else return false;
            }
        } else {
            Size fcLeft = freeCellsOnLeft(pos), fcRight = freeCellsOnRight(pos);
            if (!fcLeft && !fcRight && m_subPages[pos]->isFull()) return false;
            if (fcLeft > fcRight) redistributeR2L(pos); else redistributeL2R(pos);
        }
        return true;
    }
    Flag redistribute2(Size pos) {
        if (m_subPages[pos-1]->isUnderflow()) {
            redistributeR2L(pos + 1); redistributeR2L(pos);
            if (m_subPages[pos-1]->isUnderflow()) return false;
        } else if (m_subPages[pos+1]->isUnderflow()) {
            redistributeL2R(pos - 1); redistributeL2R(pos);
            if (m_subPages[pos+1]->isUnderflow()) return false;
        } else {
            redistributeL2R(pos - 1); redistributeR2L(pos + 1);
            if (m_subPages[pos]->isUnderflow()) return false;
        }
        return true;
    }
    Flag treatUnderflow(Size& pos) { return redistribute1(pos) || redistribute2(pos); }

    Entry& firstEntry() { return m_subPages[0] ? m_subPages[0]->firstEntry() : m_keys[0]; }

    bt_ErrorCode mergePages(Size pos) {
        std::vector<Entry> tmpKeys; std::vector<Page*> tmpSub;
        Page *c1 = m_subPages[pos-1], *c2 = m_subPages[pos], *c3 = m_subPages[pos+1];
        movePage(c1, tmpKeys, tmpSub); tmpKeys.push_back(m_keys[pos-1]);
        movePage(c2, tmpKeys, tmpSub); tmpKeys.push_back(m_keys[pos]);
        movePage(c3, tmpKeys, tmpSub);
        c3->destroy();
        Size nKeys = c1->freeCells(), i = 0;
        for (; i < nKeys; ++i) { c1->m_keys[i] = tmpKeys[i]; c1->m_subPages[i] = tmpSub[i]; ++c1->m_keyCount; }
        c1->m_subPages[i] = tmpSub[i];
        m_keys[pos-1] = tmpKeys[i]; m_subPages[pos-1] = c1;
        removeAt(m_keys, pos); removeAt(m_subPages, pos);
        --m_keyCount;
        nKeys = c2->freeCells();
        Size j = ++i;
        for (i = 0; i < nKeys; ++i, ++j) { c2->m_keys[i] = tmpKeys[j]; c2->m_subPages[i] = tmpSub[j]; ++c2->m_keyCount; }
        c2->m_subPages[i] = tmpSub[j];
        m_subPages[pos] = c2;
        return isUnderflow() ? bt_ErrorCode::underflow : bt_ErrorCode::ok;
    }
    bt_ErrorCode mergeRoot() {
        Size pos = 1;
        Page *c1 = m_subPages[pos-1], *c2 = m_subPages[pos], *c3 = m_subPages[pos+1];
        Size nKeys = c1->m_keyCount + c2->m_keyCount + c3->m_keyCount + 2;
        std::vector<Entry> tmpKeys; std::vector<Page*> tmpSub;
        movePage(c1, tmpKeys, tmpSub); tmpKeys.push_back(m_keys[pos-1]);
        movePage(c2, tmpKeys, tmpSub); tmpKeys.push_back(m_keys[pos]);
        movePage(c3, tmpKeys, tmpSub);
        clearKeys();
        Size i = 0;
        for (; i < nKeys; ++i) { m_keys[i] = tmpKeys[i]; m_subPages[i] = tmpSub[i]; ++m_keyCount; }
        m_subPages[i] = tmpSub[i];
        c1->destroy(); c2->destroy(); c3->destroy();
        return bt_ErrorCode::rootMerged;
    }

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

    bt_ErrorCode remove(const value_type& key, value_type& outValue, Ref& outRef) {
        bt_ErrorCode error = bt_ErrorCode::ok;
        Size pos = locate(key);
        if (pos < m_keyCount && m_keys[pos].m_data == key) {
            outValue = m_keys[pos].m_data; outRef = m_keys[pos].m_ref;
            if (!m_subPages[pos + 1]) {                          // hoja
                removeAt(m_keys, pos); --m_keyCount;
                return isUnderflow() ? bt_ErrorCode::underflow : bt_ErrorCode::ok;
            }
            Entry& rightFirst = m_subPages[pos + 1]->firstEntry();  // sucesor inorder
            std::swap(m_keys[pos], rightFirst);
            value_type discard{}; Ref discardRef{};
            error = m_subPages[++pos]->remove(key, discard, discardRef);
        } else if (pos == m_keyCount) {
            error = m_subPages[pos]->remove(key, outValue, outRef);
        } else if (key <= m_keys[pos].m_data) {
            if (m_subPages[pos]) error = m_subPages[pos]->remove(key, outValue, outRef);
            else return bt_ErrorCode::notFound;
        }
        if (error == bt_ErrorCode::underflow) {
            if (treatUnderflow(pos)) return bt_ErrorCode::ok;
            if (isRoot() && m_keyCount == 2) return mergeRoot();
            return mergePages(pos);
        }
        return error;
    }

    // Recorrido inorder, un solo template para CUALQUIER aridad de args extra.
    template <typename Func, typename... Args>
    void forEach(Level level, Func func, Args&&... args) const {
        for (Size i = 0; i < m_keyCount; ++i) {
            if (m_subPages[i]) m_subPages[i]->forEach(level + 1, func, std::forward<Args>(args)...);
            func(const_cast<Entry&>(m_keys[i]), level, std::forward<Args>(args)...);
        }
        if (m_subPages[m_keyCount]) m_subPages[m_keyCount]->forEach(level + 1, func, std::forward<Args>(args)...);
    }
    template <typename Func, typename... Args>
    Entry* firstThat(Level level, Func func, Args&&... args) {
        for (Size i = 0; i < m_keyCount; ++i) {
            if (m_subPages[i])
                if (Entry* found = m_subPages[i]->firstThat(level + 1, func, std::forward<Args>(args)...))
                    return found;
            if (func(m_keys[i], level, std::forward<Args>(args)...)) return &m_keys[i];
        }
        if (m_subPages[m_keyCount])
            return m_subPages[m_keyCount]->firstThat(level + 1, func, std::forward<Args>(args)...);
        return nullptr;
    }
    // Recorre PÁGINAS (para validar invariantes de orden): pasa keyCount, no Entry.
    template <typename Func, typename... Args>
    void forEachPage(Level level, Func func, Args&&... args) const {
        func(m_keyCount, level, std::forward<Args>(args)...);
        for (Size i = 0; i <= m_keyCount; ++i)
            if (m_subPages[i]) m_subPages[i]->forEachPage(level + 1, func, std::forward<Args>(args)...);
    }
};

#endif // __BTREEPAGE_H__
