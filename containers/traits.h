#ifndef __TRAITS_H__
#define __TRAITS_H__

template <typename T, typename _Comp, typename _Node>
struct BaseTrait{
    using value_type = T;
    using Comp       = _Comp;
    using Node       = _Node;
};

#endif // __TRAITS_H__
