#ifndef __TRAITS_H__
#define __TRAITS_H__
#include <functional> // para less y greater
#include "../types.h"

template <typename _Node, typename _Comp>
struct BaseTrait{
    using Node       = _Node;
    using value_type = typename _Node::value_type;
    using Comp       = _Comp;
};

template <typename _Node>
struct AscendingTrait : public BaseTrait<_Node, std::less<typename _Node::value_type>>{
};
template <typename _Node>
struct DescendingTrait : public BaseTrait<_Node, std::greater<typename _Node::value_type>>{
};

// --- B-Tree: el Order (grado) vive en el Trait, nunca hardcodeado ---
template <typename _Value, Size _Order, typename _Comp = std::less<_Value>>
struct BTreeTrait {
    using value_type            = _Value;
    using Comp                  = _Comp;
    static constexpr Size Order = _Order;
};

// Alias con nombre: comunican intención (no un literal numérico suelto)
template <typename _Value, typename _Comp = std::less<_Value>>
using Tree23Trait = BTreeTrait<_Value, 2, _Comp>;   // árbol 2-3
template <typename _Value, typename _Comp = std::less<_Value>>
using Tree34Trait = BTreeTrait<_Value, 3, _Comp>;   // árbol 3-4 (orden 3)

#endif // __TRAITS_H__