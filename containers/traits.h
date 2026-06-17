#ifndef __TRAITS_H__
#define __TRAITS_H__
#include <functional> // para less y greater
using namespace std;

template <typename _Node, typename _Comp>
struct BaseTrait{
    using Node       = _Node;
    using value_type = typename _Node::value_type;
    using ref_type   = typename _Node::ref_type;   // el trait expone value_type y ref
    using Comp       = _Comp;
};

template <typename _Node>
struct AscendingTrait : public BaseTrait<_Node, less<typename _Node::value_type>>{
};
template <typename _Node>
struct DescendingTrait : public BaseTrait<_Node, greater<typename _Node::value_type>>{
};

#endif // __TRAITS_H__