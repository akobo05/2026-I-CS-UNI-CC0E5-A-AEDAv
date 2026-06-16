#ifndef __HEAP_H__
#define __HEAP_H__

#include <iostream>
#include <string>
#include <sstream>
#include <stdexcept>
#include <mutex>
#include <shared_mutex>
#include <utility>
#include <tuple>
#include <functional>
#include "vector.h"
#include "../types.h"
#include "traits.h"
using namespace std;

template <typename T>
struct HeapNode {
    using value_type = T;
    T   m_data;
    Ref m_ref;
    HeapNode() : m_data(T()), m_ref(Ref()) {}
    HeapNode(T d, Ref r) : m_data(d), m_ref(r) {}
};

template <typename T> struct MinHeapTrait : BaseTrait<HeapNode<T>, less<T>>{};
template <typename T> struct MaxHeapTrait : BaseTrait<HeapNode<T>, greater<T>>{};

// Helper: traduce el Trait del Heap al Trait que el Vector base entiende.
template <typename HeapTraitT>
struct HeapVectorTrait : BaseTrait<
    VectorNode<typename HeapTraitT::value_type>,
    typename HeapTraitT::Comp>{};

template <typename Trait>
class Heap : public Vector<HeapVectorTrait<Trait>> {
public:
    using Base       = Vector<HeapVectorTrait<Trait>>;
    using value_type = typename Trait::value_type;
    using Comp       = typename Trait::Comp;
    using MySelf     = Heap<Trait>;

protected:
    Comp m_comp;
    void heapifyUp(Index i);
    void heapifyDown(Index i);

public:
    Heap();
    Heap(const Heap &other);
    Heap(Heap &&other) noexcept;
    Heap& operator=(const Heap &other);
    Heap& operator=(Heap &&other) noexcept;
    virtual ~Heap();

    void insert(value_type v, Ref r);
    std::tuple<value_type, Ref> extract();
    value_type peek() const;
    bool   isEmpty() const;
    std::string toString() const;

    friend std::ostream& operator<<(std::ostream& os, const Heap& h){
        return os << h.toString();
    }
    friend std::istream& operator>>(std::istream& is, Heap& h){
        char ch;
        if(!(is >> ch) || ch != '['){ is.clear(ios_base::failbit); return is; }
        value_type val; Ref ref; char comma, parenClose;
        while(is >> ch && ch != ']'){
            if(ch == '('){
                if(is >> val >> comma >> ref >> parenClose){
                    if(comma == ',' && parenClose == ')')
                        h.insert(val, ref);
                }
            }
        }
        return is;
    }

private:
    // PROBLEMA DE HERENCIA (intencional): Vector::push_back / pop_back rompen
    // el invariante de heap si se llaman directamente. Los ocultamos aqui.
    using Base::push_back;
    using Base::pop_back;
};

template <typename Trait>
Heap<Trait>::Heap() : Base() {}

template <typename Trait>
Heap<Trait>::Heap(const Heap &other) : Base(other), m_comp() {}

template <typename Trait>
Heap<Trait>::Heap(Heap &&other) noexcept : Base(std::move(other)), m_comp() {}

template <typename Trait>
Heap<Trait>& Heap<Trait>::operator=(const Heap &other){
    Base::operator=(other);
    return *this;
}

template <typename Trait>
Heap<Trait>& Heap<Trait>::operator=(Heap &&other) noexcept{
    Base::operator=(std::move(other));
    return *this;
}

template <typename Trait>
Heap<Trait>::~Heap() {}

template <typename Trait>
bool Heap<Trait>::isEmpty() const{
    return this->size() == 0;
}

template <typename Trait>
void Heap<Trait>::heapifyUp(Index i){
    auto& self = *this;
    while(i > 0){
        Index parent = (i - 1) / 2;
        value_type child_v  = self[i];
        value_type parent_v = self[parent];
        if(m_comp(child_v, parent_v)){
            self.swap(i, parent);
            i = parent;
        } else {
            break;
        }
    }
}

template <typename Trait>
void Heap<Trait>::insert(value_type v, Ref r){
    Base::push_back(v, r);
    heapifyUp(this->size() - 1);
}

template <typename Trait>
typename Heap<Trait>::value_type Heap<Trait>::peek() const{
    auto& self = *this;
    if(self.size() == 0) throw out_of_range("Heap::peek vacio");
    return self[0];
}

template <typename Trait>
void Heap<Trait>::heapifyDown(Index i){
    auto& self = *this;
    Index n = self.size();
    while(true){
        Index left  = 2 * i + 1;
        Index right = 2 * i + 2;
        Index best  = i;
        if(left  < n && m_comp(self[left],  self[best])) best = left;
        if(right < n && m_comp(self[right], self[best])) best = right;
        if(best == i) break;
        self.swap(i, best);
        i = best;
    }
}

template <typename Trait>
std::tuple<typename Heap<Trait>::value_type, Ref> Heap<Trait>::extract(){
    auto& self = *this;
    if(self.size() == 0) throw out_of_range("Heap::extract vacio");
    value_type top_v = self[0];
    Ref        top_r = self.refAt(0);
    Index      last  = self.size() - 1;
    self.swap(0, last);
    Base::pop_back();
    if(self.size() > 0) heapifyDown(0);
    return {top_v, top_r};
}

template <typename Trait>
std::string Heap<Trait>::toString() const{
    return Base::toString();
}

#endif // __HEAP_H__
