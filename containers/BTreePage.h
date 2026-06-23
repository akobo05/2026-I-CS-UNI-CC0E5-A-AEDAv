#ifndef __BTREEPAGE_H__
#define __BTREEPAGE_H__

#include <vector>
#include <utility>
#include <iostream>
#include "../types.h"
#include "traits.h"

template <typename Trait> class BTree;

enum class bt_ErrorCode { ok, overflow, duplicate };

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

    // Comparaciones derivadas del comparador del Trait (m_comp da "menor que").
    Flag eq(const value_type& a, const value_type& b) const { return !m_comp(a, b) && !m_comp(b, a); }
    Flag lt(const value_type& a, const value_type& b) const { return m_comp(a, b); }

    // Búsqueda binaria: devuelve la posición de la clave o donde debería ir.
    Size locate(const value_type& key) const {
        Size first = 0, last = m_keyCount;
        while (first < last) {
            Size mid = (first + last) / 2;
            if (eq(key, m_keys[mid].m_data)) return mid;
            if (lt(m_keys[mid].m_data, key)) first = mid + 1;
            else                             last  = mid;
        }
        if (first < m_keyCount && !lt(m_keys[first].m_data, key)) return first;  // key <= keys[first]
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
    void setMaxKeysForChilds(Size order) { m_maxKeysForChilds = order; }

    Size freeCells()    const { return m_maxKeys - m_keyCount; }
    Flag isFull()       const { return m_keyCount >= m_maxKeys; }
    Flag isOverflow()   const { return m_keyCount >  m_maxKeys; }
    Size minKeys()      const { return 2 * m_maxKeys / 3; }
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
    // Solo se usa en la ruta de INSERT (el hijo desbordó). Intenta redistribuir
    // hacia un hermano con espacio; si no hay, devuelve false -> splitChild.
    Flag redistribute1(Size& pos) {
        Size fcLeft = freeCellsOnLeft(pos), fcRight = freeCellsOnRight(pos);
        if (!fcLeft && !fcRight && m_subPages[pos]->isFull()) return false;
        if (fcLeft > fcRight) redistributeR2L(pos); else redistributeL2R(pos);
        return true;
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

    bt_ErrorCode insert(const value_type& key, Ref ref) {
        Size pos = locate(key);
        if (pos < m_keyCount && eq(key, m_keys[pos].m_data) && m_unique)
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

    Flag search(const value_type& key, value_type& outValue, Ref& outRef) {
        Size pos = locate(key);
        if (pos >= m_keyCount)
            return m_subPages[pos] ? m_subPages[pos]->search(key, outValue, outRef) : false;
        if (eq(key, m_keys[pos].m_data)) {
            outValue = m_keys[pos].m_data;
            outRef   = m_keys[pos].m_ref;
            m_keys[pos].touch();
            return true;
        }
        if (lt(key, m_keys[pos].m_data) && m_subPages[pos])
            return m_subPages[pos]->search(key, outValue, outRef);
        return false;
    }

    // Fast-path de borrado: SOLO borra si la clave está en una hoja que no caerá
    // en underflow (o en la raíz). Devuelve false sin mutar si requiere rebuild.
    Flag removeIfSafe(const value_type& key, Entry& out) {
        Size pos = locate(key);
        Flag leaf = (m_subPages[0] == nullptr);
        if (pos < m_keyCount && eq(key, m_keys[pos].m_data)) {
            if (!leaf) return false;                                  // nodo interno -> rebuild
            if (!isRoot() && m_keyCount - 1 < minKeys()) return false; // haría underflow -> rebuild
            out = m_keys[pos];
            removeAt(m_keys, pos); --m_keyCount;
            return true;
        }
        if (leaf) return false;                                       // no está aquí -> rebuild lo maneja
        Page* c = m_subPages[pos];
        return c ? c->removeIfSafe(key, out) : false;
    }

    // Recorrido inorder, un solo template para CUALQUIER aridad de args extra.
    template <typename Func, typename... Args>
    void forEach(Level level, Func func, Args&&... args) {
        for (Size i = 0; i < m_keyCount; ++i) {
            if (m_subPages[i]) m_subPages[i]->forEach(level + 1, func, std::forward<Args>(args)...);
            func(m_keys[i], level, std::forward<Args>(args)...);
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
