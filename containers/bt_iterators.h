#ifndef BT_ITERATORS_H
#define BT_ITERATORS_H

#include <vector>
#include <utility>
#include "general_iterator.h"
#include "../types.h"

using namespace std;

// Iterador in-order para el B-Tree multivia.
// La posicion NO es un solo nodo: es una pila de marcos (pagina, indice de clave),
// el camino de la raiz a la clave actual. Reutiliza general_iterator (m_pContainer,
// operator=, ==) y solo aporta la pila y el operator++.
//   D = 1 -> forward  (orden del Comp, ascendente)
//   D = 0 -> backward (orden inverso)
template <typename Container, size_t D>
class bt_inorder_iterator
        : public general_iterator<Container, bt_inorder_iterator<Container, D> >
{
public:
       using Page       = typename Container::Page;        // CBTreePage<Trait>
       using ObjectInfo = typename Container::ObjectInfo;
       using MySelf     = bt_inorder_iterator<Container, D>;
       using Parent     = general_iterator<Container, MySelf>;

protected:
       vector<pair<Page *, long>> m_stack;   // (pagina, indice de la clave actual)

       // el hijo que sigue a la clave actual segun la direccion
       Page *NextChild(Page *p, long i) { return (D == 1) ? p->m_SubPages[i+1] : p->m_SubPages[i]; }
       long  NextIndex(long i)          { return (D == 1) ? i+1 : i-1; }
       flag  ValidIndex(Page *p, long j){ return j >= 0 && (size_t)j < p->m_KeyCount; }

       // baja por el extremo inicial (izq si forward, der si backward)
       void Descend(Page *p) {
               while( p && p->m_KeyCount > 0 ) {
                       long i = (D == 1) ? 0 : (long)p->m_KeyCount - 1;
                       m_stack.push_back({p, i});
                       p = (D == 1) ? p->m_SubPages[0] : p->m_SubPages[p->m_KeyCount];
               }
       }
       // m_pNode refleja la pagina actual (nullptr al terminar) para que == funcione
       void Sync() { this->m_pNode = m_stack.empty() ? nullptr : m_stack.back().first; }

public:
       bt_inorder_iterator(Container *pC, Page *pRoot, flag atEnd) : Parent(pC, nullptr) {
               if( !atEnd ) Descend(pRoot);
               Sync();
       }

       MySelf operator++() {
               if( m_stack.empty() ) return *this;
               Page *page = m_stack.back().first;
               long  i    = m_stack.back().second;
               Page *child = NextChild(page, i);
               m_stack.back().second = NextIndex(i);     // este marco avanza una posicion
               if( child )
                       Descend(child);                   // hay subarbol -> bajar a su extremo
               else                                      // hoja -> descartar marcos agotados
                       while( !m_stack.empty() && !ValidIndex(m_stack.back().first, m_stack.back().second) )
                               m_stack.pop_back();
               Sync();
               return *this;
       }

       // clave actual y su nivel (profundidad = tam. de la pila - 1)
       ObjectInfo &operator*() { return m_stack.back().first->m_Keys[ m_stack.back().second ]; }
       T1 level() const { return (T1)m_stack.size() - 1; }
};

#endif // BT_ITERATORS_H
