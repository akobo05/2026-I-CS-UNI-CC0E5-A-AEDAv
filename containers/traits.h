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
struct AscendingTrait : public BaseTrait<_Node, less<typename _Node::value_type>>{
};
template <typename _Node>
struct DescendingTrait : public BaseTrait<_Node, greater<typename _Node::value_type>>{
};

// Trait del B-Tree: agrupa tipo de clave, de objeto y el COMPARADOR.
template <typename _Key, typename _ObjID = Ref, typename _Comp = less<_Key>>
struct BTreeTrait {
    using keyType   = _Key;
    using ObjIDType = _ObjID;
    using Comp      = _Comp;
};
template <typename _Key, typename _ObjID = Ref> using AscBTreeTrait  = BTreeTrait<_Key, _ObjID, less<_Key>>;
template <typename _Key, typename _ObjID = Ref> using DescBTreeTrait = BTreeTrait<_Key, _ObjID, greater<_Key>>;

#endif // __TRAITS_H__