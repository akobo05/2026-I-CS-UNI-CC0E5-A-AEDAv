#ifndef __LINKEDLIST_H__
#define __LINKEDLIST_H__

#include <iostream>
#include <cstddef> // size_t
#include <string>
#include <sstream>
#include <stdexcept>
#include <mutex>
#include <shared_mutex> // shared_mutex
#include "general_iterator.h"
#include "util.h"
#include "../types.h"
using namespace std;

// Forward iterator
template <typename Container>
class LinkedListForwardIterator : public general_iterator<Container, LinkedListForwardIterator<Container>>{
public:
    using MySelf = LinkedListForwardIterator<Container>;
    using Parent = general_iterator<Container, MySelf>;
    using Parent::Parent;
    MySelf operator++() { this->m_pNode = this->m_pNode->getNext(); return *this; }
};

// Linked List Node
template <typename T>
class LLNode{
    using Node = LLNode<T>;
private:
    T    m_data;
    Ref  m_ref;
    Node *m_next;
public:
    LLNode() : m_data(T()), m_ref(Ref()), m_next(nullptr) {}
    LLNode(T data, Ref ref) : m_data(data), m_ref(ref), m_next(nullptr) {}
    LLNode(T data, Ref ref, Node *next) : m_data(data), m_ref(ref), m_next(next) {}
    virtual ~LLNode() {}

    T      getData() const { return m_data; }
    T&     getDataRef()    { return m_data; }
    void   setData(T data) { m_data = data; }
    Ref    getRef() const  { return m_ref; }
    void   setRef(Ref ref) { m_ref = ref; }
    Node*  getNext() const { return m_next; }
    Node*& getNextRef()    { return m_next; }
    void   setNext(Node *next) { m_next = next; }
};

template <typename T>
ostream& operator<<(ostream& os, LLNode<T>& node){
    return os << "(" << node.getData() << "," << node.getRef() << ")";
}

template <typename T>
struct AscendingLinkedListTrait{
    using value_type = T;
    using Node = LLNode<T>;
    using Comp = less<T>;
};

template <typename T>
struct DescendingLinkedListTrait{
    using value_type = T;
    using Node = LLNode<T>;
    using Comp = greater<T>;
};

template <typename Trait>
class LinkedList{
public:
    using value_type = typename Trait::value_type;
    using Node       = typename Trait::Node;
    using Comp       = typename Trait::Comp;
    using MySelf     = LinkedList<Trait>;

    using forward_iterator = LinkedListForwardIterator<MySelf>;
    friend forward_iterator;

private:
    Node  *m_pRoot = nullptr;
    Node  *m_tail  = nullptr;
    size_t m_size  = 0;
    Comp   m_comp;
    mutable shared_mutex m_mtx;

    void internal_clear();
    void internal_push_back(const value_type &value, Ref ref);
    void internal_insert(Node* &pPrev, const value_type &value, Ref ref);

public:
    LinkedList() {}
    LinkedList(const LinkedList &other);
    LinkedList(LinkedList &&other);
    LinkedList& operator=(const LinkedList &other);
    LinkedList& operator=(LinkedList &&other);
    virtual ~LinkedList();

    virtual void    push_front(value_type value, Ref ref);
    virtual void    pop_front();
    virtual void    push_back(value_type value, Ref ref);
    virtual void    pop_back();
    virtual void    insert(const value_type &value, Ref ref);

    virtual value_type& operator[](size_t index);
    virtual size_t  size() const;
    virtual string  toString() const;

    forward_iterator begin() { return forward_iterator(this, m_pRoot); }
    forward_iterator end()   { return forward_iterator(this, nullptr); }

    template <typename Func, typename... Args>
    void ForEach(Func func, Args &&...  args){
        unique_lock<shared_mutex> lock(m_mtx);
        ::ForEach(begin(), end(), func, std::forward<Args>(args)... );
    }
};

template <typename Trait>
void LinkedList<Trait>::internal_clear(){
    Node* curr = m_pRoot;
    while(curr){
        Node* next = curr->getNext();
        delete curr;
        curr = next;
    }
    m_pRoot = nullptr;
    m_tail  = nullptr;
    m_size  = 0;
}

template <typename Trait>
void LinkedList<Trait>::internal_push_back(const value_type &value, Ref ref){
    Node* n = new Node(value, ref);
    if(m_size == 0) m_pRoot = m_tail = n;
    else          { m_tail->setNext(n); m_tail = n; }
    m_size++;
}

template <typename Trait>
void LinkedList<Trait>::internal_insert(Node* &pPrev, const value_type &value, Ref ref){
    if(!pPrev || m_comp(value, pPrev->getDataRef())){
        pPrev = new Node(value, ref, pPrev);
        m_size++;
        if(pPrev->getNext() == nullptr)
            m_tail = pPrev;
        return;
    }
    internal_insert(pPrev->getNextRef(), value, ref);
}

template <typename Trait>
LinkedList<Trait>::LinkedList(const LinkedList &other){
    shared_lock<shared_mutex> lock(other.m_mtx);
    for(Node* curr = other.m_pRoot; curr; curr = curr->getNext())
        internal_push_back(curr->getData(), curr->getRef());
}

template <typename Trait>
LinkedList<Trait>::LinkedList(LinkedList &&other){
    unique_lock<shared_mutex> lock(other.m_mtx);
    m_pRoot = other.m_pRoot;
    m_tail  = other.m_tail;
    m_size  = other.m_size;
    other.m_pRoot = nullptr;
    other.m_tail  = nullptr;
    other.m_size  = 0;
}

template <typename Trait>
LinkedList<Trait>& LinkedList<Trait>::operator=(const LinkedList &other){
    if(this == &other) return *this;
    unique_lock<shared_mutex> lk_this(m_mtx);
    shared_lock<shared_mutex> lk_other(other.m_mtx);
    internal_clear();
    for(Node* curr = other.m_pRoot; curr; curr = curr->getNext())
        internal_push_back(curr->getData(), curr->getRef());
    return *this;
}

template <typename Trait>
LinkedList<Trait>& LinkedList<Trait>::operator=(LinkedList &&other){
    if(this == &other) return *this;
    unique_lock<shared_mutex> lk_this(m_mtx);
    unique_lock<shared_mutex> lk_other(other.m_mtx);
    internal_clear();
    m_pRoot = other.m_pRoot;
    m_tail  = other.m_tail;
    m_size  = other.m_size;
    other.m_pRoot = nullptr;
    other.m_tail  = nullptr;
    other.m_size  = 0;
    return *this;
}

template <typename Trait>
LinkedList<Trait>::~LinkedList(){
    unique_lock<shared_mutex> lock(m_mtx);
    internal_clear();
}

template <typename Trait>
void LinkedList<Trait>::push_front(value_type value, Ref ref){
    unique_lock<shared_mutex> lock(m_mtx);
    m_pRoot = new Node(value, ref, m_pRoot);
    if(m_size == 0) m_tail = m_pRoot;
    m_size++;
}

template <typename Trait>
void LinkedList<Trait>::pop_front(){
    unique_lock<shared_mutex> lock(m_mtx);
    if(!m_pRoot) return;
    Node* old = m_pRoot;
    m_pRoot = m_pRoot->getNext();
    delete old;
    m_size--;
    if(m_size == 0) m_tail = nullptr;
}

template <typename Trait>
void LinkedList<Trait>::push_back(value_type value, Ref ref){
    unique_lock<shared_mutex> lock(m_mtx);
    internal_push_back(value, ref);
}

template <typename Trait>
void LinkedList<Trait>::pop_back(){
    unique_lock<shared_mutex> lock(m_mtx);
    if(!m_pRoot) return;
    if(m_pRoot == m_tail){
        delete m_pRoot;
        m_pRoot = m_tail = nullptr;
    } else {
        Node* prev = m_pRoot;
        while(prev->getNext() != m_tail) prev = prev->getNext();
        delete m_tail;
        m_tail = prev;
        m_tail->setNext(nullptr);
    }
    m_size--;
}

template <typename Trait>
void LinkedList<Trait>::insert(const value_type &value, Ref ref){
    unique_lock<shared_mutex> lock(m_mtx);
    internal_insert(m_pRoot, value, ref);
}

template <typename Trait>
typename LinkedList<Trait>::value_type& LinkedList<Trait>::operator[](size_t index){
    shared_lock<shared_mutex> lock(m_mtx);
    if(index >= m_size)
        throw out_of_range("LinkedList::operator[]: indice fuera de rango");
    Node* curr = m_pRoot;
    for(size_t i = 0; i < index; ++i)
        curr = curr->getNext();
    return curr->getDataRef();
}

template <typename Trait>
size_t LinkedList<Trait>::size() const{
    shared_lock<shared_mutex> lock(m_mtx);
    return m_size;
}

template <typename Trait>
string LinkedList<Trait>::toString() const{
    shared_lock<shared_mutex> lock(m_mtx);
    ostringstream oss;
    oss << "[";
    bool first = true;
    for(Node* curr = m_pRoot; curr; curr = curr->getNext()){
        if(!first) oss << ",";
        oss << "(" << curr->getData() << "," << curr->getRef() << ")";
        first = false;
    }
    oss << "]";
    return oss.str();
}

template <typename Trait>
ostream& operator<<(ostream& os, const LinkedList<Trait>& list){
    return os << list.toString();
}

template <typename Trait>
istream& operator>>(istream& is, LinkedList<Trait>& list){
    typename LinkedList<Trait>::value_type value;
    Ref ref;
    char ch;
    is >> ch;
    if((is >> ws).peek() == ']'){ is >> ch; return is; }
    while(true){
        is >> ch;
        is >> value >> ch >> ref >> ch;
        list.insert(value, ref);
        is >> ch;
        if(ch == ']') break;
    }
    return is;
}

void ListsDemo();

#endif // __LINKEDLIST_H__
