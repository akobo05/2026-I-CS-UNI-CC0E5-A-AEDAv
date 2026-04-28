#ifndef __DOUBLELINKEDLIST_H__
#define __DOUBLELINKEDLIST_H__

#include "linkedlist.h"

template <typename T>
class DLLNode : public LLNode<T, DLLNode<T>>{
    using Base = LLNode<T, DLLNode<T>>;
    using Node = DLLNode<T>;
private:
    Node *m_pPrev;
public:
    DLLNode() : Base(), m_pPrev(nullptr) {}
    DLLNode(T data, Ref ref, Node *next = nullptr, Node *prev = nullptr)
        : Base(data, ref, next), m_pPrev(prev) {}

    Node*  getPrev() const     { return m_pPrev; }
    void   setPrev(Node *prev) { m_pPrev = prev; }
    Node*& getPrevRef()        { return m_pPrev; }
};

template <typename T>
struct AscendingDLLTrait  : BaseTrait<T, less<T>,    DLLNode<T>>{};

template <typename T>
struct DescendingDLLTrait : BaseTrait<T, greater<T>, DLLNode<T>>{};

template <typename Container>
class DLLBackwardIterator
    : public general_iterator<Container, DLLBackwardIterator<Container>>{
public:
    using MySelf = DLLBackwardIterator<Container>;
    using Parent = general_iterator<Container, MySelf>;
    using Parent::Parent;
    MySelf& operator++()   { this->m_pNode = this->m_pNode->getPrev(); return *this; }
    MySelf  operator++(int){ MySelf tmp = *this; ++(*this); return tmp; }
};

template <typename Trait>
class DoubleLinkedList : public LinkedList<Trait>{
public:
    using Base       = LinkedList<Trait>;
    using value_type = typename Trait::value_type;
    using Node       = typename Trait::Node;
    using MySelf     = DoubleLinkedList<Trait>;

    using forward_iterator  = LinkedListForwardIterator<MySelf>;
    using backward_iterator = DLLBackwardIterator<MySelf>;
    friend forward_iterator;
    friend backward_iterator;

protected:
    void internal_clear_dll();
    void internal_push_back_dll(const value_type &value, Ref ref);

public:
    DoubleLinkedList() : Base() {}
    DoubleLinkedList(const DoubleLinkedList &other);
    DoubleLinkedList(DoubleLinkedList &&other) noexcept;
    DoubleLinkedList& operator=(const DoubleLinkedList &other);
    DoubleLinkedList& operator=(DoubleLinkedList &&other) noexcept;
    virtual ~DoubleLinkedList() {}

    void   push_front(value_type value, Ref ref) override;
    std::tuple<value_type, Ref> pop_front() override;
    void   push_back(value_type value, Ref ref) override;
    std::tuple<value_type, Ref> pop_back() override;
    void   insert(const value_type &value, Ref ref) override;

    forward_iterator  begin()  { return forward_iterator(this, this->m_pRoot); }
    forward_iterator  end()    { return forward_iterator(this, nullptr); }
    backward_iterator rbegin() { return backward_iterator(this, this->m_tail); }
    backward_iterator rend()   { return backward_iterator(this, nullptr); }

    template <typename Func, typename... Args>
    void ReverseForEach(Func func, Args &&... args){
        unique_lock<shared_mutex> lock(this->m_mtx);
        for(auto it = rbegin(); it != rend(); ++it)
            func(*it, std::forward<Args>(args)...);
    }

    friend ostream& operator<<(ostream& os, const DoubleLinkedList& list){
        shared_lock<shared_mutex> lock(list.m_mtx);
        os << "[";
        bool first = true;
        for(Node* curr = list.m_pRoot; curr; curr = curr->getNext()){
            if(!first) os << ",";
            os << "(" << curr->getData() << "," << curr->getRef() << ")";
            first = false;
        }
        os << "] |bwd:[";
        first = true;
        for(Node* curr = list.m_tail; curr; curr = curr->getPrev()){
            if(!first) os << ",";
            os << "(" << curr->getData() << "," << curr->getRef() << ")";
            first = false;
        }
        os << "]";
        return os;
    }
};

template <typename Trait>
void DoubleLinkedList<Trait>::internal_clear_dll(){
    Node* curr = this->m_pRoot;
    while(curr){
        Node* next = curr->getNext();
        delete curr;
        curr = next;
    }
    this->m_pRoot = nullptr;
    this->m_tail  = nullptr;
    this->m_size  = 0;
}

template <typename Trait>
void DoubleLinkedList<Trait>::internal_push_back_dll(const value_type &value, Ref ref){
    Node* n = new Node(value, ref, nullptr, this->m_tail);
    if(this->m_size == 0) this->m_pRoot = this->m_tail = n;
    else                { this->m_tail->setNext(n); this->m_tail = n; }
    this->m_size++;
}

template <typename Trait>
DoubleLinkedList<Trait>::DoubleLinkedList(const DoubleLinkedList &other) : Base() {
    shared_lock<shared_mutex> lock(other.m_mtx);
    for(Node* curr = other.m_pRoot; curr; curr = curr->getNext())
        internal_push_back_dll(curr->getData(), curr->getRef());
}

template <typename Trait>
DoubleLinkedList<Trait>::DoubleLinkedList(DoubleLinkedList &&other) noexcept : Base() {
    unique_lock<shared_mutex> lock(other.m_mtx);
    this->m_pRoot = std::exchange(other.m_pRoot, nullptr);
    this->m_tail  = std::exchange(other.m_tail,  nullptr);
    this->m_size  = std::exchange(other.m_size,  0);
}

template <typename Trait>
DoubleLinkedList<Trait>& DoubleLinkedList<Trait>::operator=(const DoubleLinkedList &other){
    if(this == &other) return *this;
    unique_lock<shared_mutex> lk_this(this->m_mtx);
    shared_lock<shared_mutex> lk_other(other.m_mtx);
    internal_clear_dll();
    for(Node* curr = other.m_pRoot; curr; curr = curr->getNext())
        internal_push_back_dll(curr->getData(), curr->getRef());
    return *this;
}

template <typename Trait>
DoubleLinkedList<Trait>& DoubleLinkedList<Trait>::operator=(DoubleLinkedList &&other) noexcept{
    if(this == &other) return *this;
    unique_lock<shared_mutex> lk_this(this->m_mtx);
    unique_lock<shared_mutex> lk_other(other.m_mtx);
    internal_clear_dll();
    this->m_pRoot = std::exchange(other.m_pRoot, nullptr);
    this->m_tail  = std::exchange(other.m_tail,  nullptr);
    this->m_size  = std::exchange(other.m_size,  0);
    return *this;
}

template <typename Trait>
void DoubleLinkedList<Trait>::push_front(value_type value, Ref ref){
    unique_lock<shared_mutex> lock(this->m_mtx);
    Node* n = new Node(value, ref, this->m_pRoot, nullptr);
    if(this->m_size == 0) this->m_tail = n;
    else                  this->m_pRoot->setPrev(n);
    this->m_pRoot = n;
    this->m_size++;
}

template <typename Trait>
std::tuple<typename DoubleLinkedList<Trait>::value_type, Ref>
DoubleLinkedList<Trait>::pop_front(){
    unique_lock<shared_mutex> lock(this->m_mtx);
    if(!this->m_pRoot)
        throw out_of_range("DoubleLinkedList::pop_front: lista vacia");
    Node* old = this->m_pRoot;
    auto result = std::make_tuple(old->getData(), old->getRef());
    this->m_pRoot = old->getNext();
    if(this->m_pRoot) this->m_pRoot->setPrev(nullptr);
    else              this->m_tail  = nullptr;
    delete old;
    this->m_size--;
    return result;
}

template <typename Trait>
void DoubleLinkedList<Trait>::push_back(value_type value, Ref ref){
    unique_lock<shared_mutex> lock(this->m_mtx);
    internal_push_back_dll(value, ref);
}

template <typename Trait>
std::tuple<typename DoubleLinkedList<Trait>::value_type, Ref>
DoubleLinkedList<Trait>::pop_back(){
    unique_lock<shared_mutex> lock(this->m_mtx);
    if(!this->m_tail)
        throw out_of_range("DoubleLinkedList::pop_back: lista vacia");
    Node* old = this->m_tail;
    auto result = std::make_tuple(old->getData(), old->getRef());
    this->m_tail = old->getPrev();
    if(this->m_tail) this->m_tail->setNext(nullptr);
    else             this->m_pRoot = nullptr;
    delete old;
    this->m_size--;
    return result;
}

template <typename Trait>
void DoubleLinkedList<Trait>::insert(const value_type &value, Ref ref){
    unique_lock<shared_mutex> lock(this->m_mtx);
    Node* prev = nullptr;
    Node* curr = this->m_pRoot;
    while(curr && !this->m_comp(value, curr->getDataRef())){
        prev = curr;
        curr = curr->getNext();
    }
    Node* n = new Node(value, ref, curr, prev);
    if(prev) prev->setNext(n);
    else     this->m_pRoot = n;
    if(curr) curr->setPrev(n);
    else     this->m_tail  = n;
    this->m_size++;
}

#endif // __DOUBLELINKEDLIST_H__
