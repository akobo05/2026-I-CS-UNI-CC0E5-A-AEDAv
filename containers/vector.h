#ifndef __VECTOR_H__
#define __VECTOR_H__

#include <iostream>
#include <string>
#include <sstream>
#include <shared_mutex>
#include <mutex>
#include <stdexcept>
#include <utility>
#include <functional>
#include "general_iterator.h"
#include "util.h"
#include "../types.h"
#include "traits.h"
using namespace std;

template <typename Container>
class vector_forward_iterator : public general_iterator<Container, vector_forward_iterator<Container>> {
public:
    using MySelf = vector_forward_iterator<Container>;
    using Parent = general_iterator<Container, MySelf>;
    using Parent::Parent;
    MySelf operator++() { this->m_pNode++; return *this; }
};

template <typename Container>
class vector_backward_iterator : public general_iterator<Container, vector_backward_iterator<Container>> {
public:
    using MySelf = vector_backward_iterator<Container>;
    using Parent = general_iterator<Container, MySelf>;
    using Parent::Parent;
    MySelf operator++() { this->m_pNode--; return *this; }
};

template <typename T>
class VectorNode{
public:
    using value_type = T;
private:
    T   m_data;
    Ref m_ref;
public:
    VectorNode() : m_data(T()), m_ref(Ref()) {}
    VectorNode(T data, Ref ref) : m_data(data), m_ref(ref) {}
    VectorNode(const VectorNode &other) : m_data(other.m_data), m_ref(other.m_ref) {}
    VectorNode(VectorNode &&other) : m_data(std::move(other.m_data)), m_ref(std::move(other.m_ref)) {}
    VectorNode& operator=(const VectorNode &other) {
        m_data = other.m_data; m_ref = other.m_ref; return *this;
    }
    VectorNode& operator=(VectorNode &&other) {
        m_data = std::move(other.m_data); m_ref = std::move(other.m_ref); return *this;
    }
    T    getData() const  { return m_data; }
    T&   getDataRef()     { return m_data; }
    void setData(T data)  { m_data = data; }
    Ref  getRef() const   { return m_ref; }
    Ref& getRefRef()      { return m_ref; }
    void setRef(Ref ref)  { m_ref = ref; }
};

template <typename T>
ostream& operator<<(ostream& os, VectorNode<T>& node){
    return os << "(" << node.getData() << "," << node.getRef() << ")";
}

template <typename T> struct AscendingVectorTrait  : BaseTrait<VectorNode<T>, less<T>>{};
template <typename T> struct DescendingVectorTrait : BaseTrait<VectorNode<T>, greater<T>>{};

template <typename Trait>
class Vector{
public:
    using value_type = typename Trait::value_type;
    using Node       = typename Trait::Node;
    using Comp       = typename Trait::Comp;
    using MySelf     = Vector<Trait>;
    using forward_iterator  = vector_forward_iterator<MySelf>;
    using backward_iterator = vector_backward_iterator<MySelf>;
    friend forward_iterator;
    friend backward_iterator;

protected:
    Index   m_capacity;
    Index   m_size;
    Node   *m_data;
    mutable shared_mutex m_mtx;
    void    resize_internal();
    void    internal_clear();

public:
    Vector(Index capacity = 10);
    Vector(const Vector &other);
    Vector(Vector &&other) noexcept;
    Vector& operator=(const Vector &other);
    Vector& operator=(Vector &&other) noexcept;
    virtual ~Vector();

    virtual void   push_back(value_type value, Ref ref);
    virtual void   pop_back();
    virtual value_type&       operator[](Index i);
    virtual const value_type& operator[](Index i) const;
    virtual Ref&   refAt(Index i);
    virtual void   swap(Index i, Index j);
    virtual Index  size() const;
    virtual std::string toString() const;

    forward_iterator begin() { return forward_iterator(this, m_data); }
    forward_iterator end()   { return forward_iterator(this, m_data + m_size); }
    backward_iterator rbegin() { return backward_iterator(this, m_data + m_size - 1); }
    backward_iterator rend()   { return backward_iterator(this, m_data - 1); }

    template <typename Func, typename... Args>
    void ForEach(Func func, Args &&... args){
        unique_lock<shared_mutex> lock(m_mtx);
        ::ForEach(begin(), end(), func, std::forward<Args>(args)...);
    }
    template <typename Func, typename... Args>
    void ReverseForEach(Func func, Args &&... args){
        unique_lock<shared_mutex> lock(m_mtx);
        if(m_size == 0) return;
        ::ForEach(rbegin(), rend(), func, std::forward<Args>(args)...);
    }

    friend ostream& operator<<(ostream& os, const Vector& v){
        return os << v.toString();
    }
    friend istream& operator>>(istream& is, Vector& v){
        char ch;
        if(!(is >> ch) || ch != '['){ is.clear(ios_base::failbit); return is; }
        value_type val; Ref ref; char comma, parenClose;
        while(is >> ch && ch != ']'){
            if(ch == '('){
                if(is >> val >> comma >> ref >> parenClose){
                    if(comma == ',' && parenClose == ')')
                        v.push_back(val, ref);
                }
            }
        }
        return is;
    }
};

template <typename Trait>
Vector<Trait>::Vector(Index capacity)
    : m_capacity(capacity), m_size(0), m_data(new Node[capacity]) {}

template <typename Trait>
Vector<Trait>::Vector(const Vector &other)
    : m_capacity(other.m_capacity), m_size(0), m_data(new Node[other.m_capacity])
{
    shared_lock<shared_mutex> lock(other.m_mtx);
    for(Index i = 0; i < other.m_size; ++i)
        m_data[i] = other.m_data[i];
    m_size = other.m_size;
}

template <typename Trait>
Vector<Trait>::Vector(Vector &&other) noexcept
    : m_capacity(0), m_size(0), m_data(nullptr)
{
    unique_lock<shared_mutex> lock(other.m_mtx);
    m_capacity = std::exchange(other.m_capacity, 0);
    m_size     = std::exchange(other.m_size, 0);
    m_data     = std::exchange(other.m_data, nullptr);
}

template <typename Trait>
Vector<Trait>& Vector<Trait>::operator=(const Vector &other){
    if(this == &other) return *this;
    unique_lock<shared_mutex> lk_this(m_mtx);
    shared_lock<shared_mutex> lk_other(other.m_mtx);
    internal_clear();
    m_capacity = other.m_capacity;
    m_data = new Node[m_capacity];
    for(Index i = 0; i < other.m_size; ++i) m_data[i] = other.m_data[i];
    m_size = other.m_size;
    return *this;
}

template <typename Trait>
Vector<Trait>& Vector<Trait>::operator=(Vector &&other) noexcept{
    if(this == &other) return *this;
    unique_lock<shared_mutex> lk_this(m_mtx);
    unique_lock<shared_mutex> lk_other(other.m_mtx);
    internal_clear();
    m_capacity = std::exchange(other.m_capacity, 0);
    m_size     = std::exchange(other.m_size, 0);
    m_data     = std::exchange(other.m_data, nullptr);
    return *this;
}

template <typename Trait>
Vector<Trait>::~Vector(){ delete[] m_data; }

template <typename Trait>
void Vector<Trait>::internal_clear(){
    delete[] m_data; m_data = nullptr; m_size = 0; m_capacity = 0;
}

template <typename Trait>
void Vector<Trait>::resize_internal(){
    m_capacity = (m_capacity < 10) ? m_capacity + 10 : m_capacity * 2;
    Node *new_data = new Node[m_capacity];
    for(Index i = 0; i < m_size; ++i) new_data[i] = m_data[i];
    delete[] m_data;
    m_data = new_data;
}

template <typename Trait>
void Vector<Trait>::push_back(value_type value, Ref ref){
    unique_lock<shared_mutex> lock(m_mtx);
    if(m_size == m_capacity) resize_internal();
    m_data[m_size++] = Node(value, ref);
}

template <typename Trait>
void Vector<Trait>::pop_back(){
    unique_lock<shared_mutex> lock(m_mtx);
    if(m_size == 0) throw out_of_range("Vector::pop_back vacio");
    --m_size;
}

template <typename Trait>
typename Vector<Trait>::value_type& Vector<Trait>::operator[](Index i){
    shared_lock<shared_mutex> lock(m_mtx);
    if(i < 0 || i >= m_size) throw out_of_range("Vector::operator[] fuera de rango");
    return m_data[i].getDataRef();
}

template <typename Trait>
const typename Vector<Trait>::value_type& Vector<Trait>::operator[](Index i) const{
    shared_lock<shared_mutex> lock(m_mtx);
    if(i < 0 || i >= m_size) throw out_of_range("Vector::operator[] const fuera de rango");
    return const_cast<Node*>(m_data)[i].getDataRef();
}

template <typename Trait>
Ref& Vector<Trait>::refAt(Index i){
    shared_lock<shared_mutex> lock(m_mtx);
    if(i < 0 || i >= m_size) throw out_of_range("Vector::refAt fuera de rango");
    return m_data[i].getRefRef();
}

template <typename Trait>
void Vector<Trait>::swap(Index i, Index j){
    unique_lock<shared_mutex> lock(m_mtx);
    if(i < 0 || j < 0 || i >= m_size || j >= m_size)
        throw out_of_range("Vector::swap fuera de rango");
    std::swap(m_data[i], m_data[j]);
}

template <typename Trait>
Index Vector<Trait>::size() const{
    shared_lock<shared_mutex> lock(m_mtx);
    return m_size;
}

template <typename Trait>
std::string Vector<Trait>::toString() const{
    shared_lock<shared_mutex> lock(m_mtx);
    ostringstream oss;
    oss << "[";
    for(Index i = 0; i < m_size; ++i){
        if(i > 0) oss << ",";
        oss << m_data[i];
    }
    oss << "]";
    return oss.str();
}

void DemoVector();
void DemoConcurrentVector();

#endif // __VECTOR_H__
