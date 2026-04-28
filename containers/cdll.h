#ifndef __CDLL_H__
#define __CDLL_H__

#include "doublelinkedlist.h"

template <typename T>
struct AscendingCDLLTrait  : BaseTrait<T, less<T>,    DLLNode<T>>{};

template <typename T>
struct DescendingCDLLTrait : BaseTrait<T, greater<T>, DLLNode<T>>{};

template <typename Container>
class CDLLForwardIterator
    : public general_iterator<Container, CDLLForwardIterator<Container>>{
public:
    using MySelf = CDLLForwardIterator<Container>;
    using Parent = general_iterator<Container, MySelf>;
    using Node   = typename Container::Node;
private:
    Node *m_start;
public:
    CDLLForwardIterator(Container *pContainer, Node *pNode)
        : Parent(pContainer, pNode), m_start(pNode) {}
    CDLLForwardIterator(Container *pContainer, Node *pNode, bool /*sentinel*/)
        : Parent(pContainer, pNode), m_start(nullptr) {}
    MySelf& operator++(){
        if(this->m_pNode){
            this->m_pNode = this->m_pNode->getNext();
            if(this->m_pNode == m_start)
                this->m_pNode = nullptr;
        }
        return *this;
    }
    MySelf  operator++(int){ MySelf tmp = *this; ++(*this); return tmp; }
};

template <typename Container>
class CDLLBackwardIterator
    : public general_iterator<Container, CDLLBackwardIterator<Container>>{
public:
    using MySelf = CDLLBackwardIterator<Container>;
    using Parent = general_iterator<Container, MySelf>;
    using Node   = typename Container::Node;
private:
    Node *m_start;
public:
    CDLLBackwardIterator(Container *pContainer, Node *pNode)
        : Parent(pContainer, pNode), m_start(pNode) {}
    CDLLBackwardIterator(Container *pContainer, Node *pNode, bool /*sentinel*/)
        : Parent(pContainer, pNode), m_start(nullptr) {}
    MySelf& operator++(){
        if(this->m_pNode){
            this->m_pNode = this->m_pNode->getPrev();
            if(this->m_pNode == m_start)
                this->m_pNode = nullptr;
        }
        return *this;
    }
    MySelf  operator++(int){ MySelf tmp = *this; ++(*this); return tmp; }
};

template <typename Trait>
class CircularDoubleLinkedList : public DoubleLinkedList<Trait>{
public:
    using Base       = DoubleLinkedList<Trait>;
    using value_type = typename Trait::value_type;
    using Node       = typename Trait::Node;
    using MySelf     = CircularDoubleLinkedList<Trait>;

    using forward_iterator  = CDLLForwardIterator<MySelf>;
    using backward_iterator = CDLLBackwardIterator<MySelf>;
    friend forward_iterator;
    friend backward_iterator;

protected:
    void internal_clear_cdll();
    void internal_push_back_cdll(const value_type &value, Ref ref);

public:
    CircularDoubleLinkedList() : Base() {}
    CircularDoubleLinkedList(const CircularDoubleLinkedList &other);
    CircularDoubleLinkedList(CircularDoubleLinkedList &&other) noexcept;
    CircularDoubleLinkedList& operator=(const CircularDoubleLinkedList &other);
    CircularDoubleLinkedList& operator=(CircularDoubleLinkedList &&other) noexcept;
    virtual ~CircularDoubleLinkedList();

    void   push_front(value_type value, Ref ref) override;
    std::tuple<value_type, Ref> pop_front() override;
    void   push_back(value_type value, Ref ref) override;
    std::tuple<value_type, Ref> pop_back() override;
    void   insert(const value_type &value, Ref ref) override;

    forward_iterator  begin()  { return forward_iterator(this, this->m_pRoot); }
    forward_iterator  end()    { return forward_iterator(this, nullptr, true); }
    backward_iterator rbegin() { return backward_iterator(this, this->m_tail); }
    backward_iterator rend()   { return backward_iterator(this, nullptr, true); }

    template <typename Func, typename... Args>
    void ForEach(Func func, Args &&... args){
        unique_lock<shared_mutex> lock(this->m_mtx);
        if(this->m_size == 0) return;
        for(auto& item : *this)
            func(item, std::forward<Args>(args)...);
    }

    template <typename Func, typename... Args>
    void ReverseForEach(Func func, Args &&... args){
        unique_lock<shared_mutex> lock(this->m_mtx);
        for(auto it = rbegin(); it != rend(); ++it)
            func(*it, std::forward<Args>(args)...);
    }

    template <typename Func, typename... Args>
    void circularForEach(size_t vueltas, int direction, Func func, Args &&... args){
        unique_lock<shared_mutex> lock(this->m_mtx);
        if(!this->m_pRoot || vueltas == 0) return;
        Node* act = (direction >= 0) ? this->m_pRoot : this->m_tail;
        size_t pasos = this->m_size * vueltas;
        for(size_t i = 0; i < pasos; ++i){
            func(act->getDataRef(), std::forward<Args>(args)...);
            act = (direction >= 0) ? act->getNext() : act->getPrev();
        }
    }

    friend ostream& operator<<(ostream& os, const CircularDoubleLinkedList& list){
        shared_lock<shared_mutex> lock(list.m_mtx);
        os << "[";
        if(list.m_pRoot){
            Node* curr = list.m_pRoot;
            bool first = true;
            do {
                if(!first) os << ",";
                os << "(" << curr->getData() << "," << curr->getRef() << ")";
                first = false;
                curr = curr->getNext();
            } while(curr != list.m_pRoot);
        }
        os << "]";
        if(list.m_pRoot){
            os << " |bwd:[";
            Node* curr = list.m_tail;
            bool first = true;
            do {
                if(!first) os << ",";
                os << "(" << curr->getData() << "," << curr->getRef() << ")";
                first = false;
                curr = curr->getPrev();
            } while(curr != list.m_tail);
            os << "] ->root(" << list.m_pRoot->getData() << ")";
        }
        return os;
    }

    friend istream& operator>>(istream& is, CircularDoubleLinkedList& list){
        char ch;
        if(!(is >> ch) || ch != '['){
            is.clear(ios_base::failbit);
            return is;
        }
        value_type val;
        Ref ref;
        char comma, parenClose;
        while(is >> ch && ch != ']'){
            if(ch == '('){
                if(is >> val >> comma >> ref >> parenClose){
                    if(comma == ',' && parenClose == ')')
                        list.insert(val, ref);
                }
            }
        }
        return is;
    }
};

template <typename Trait>
void CircularDoubleLinkedList<Trait>::internal_clear_cdll(){
    if(this->m_tail)  this->m_tail->setNext(nullptr);
    if(this->m_pRoot) this->m_pRoot->setPrev(nullptr);
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
void CircularDoubleLinkedList<Trait>::internal_push_back_cdll(const value_type &value, Ref ref){
    Node* n = new Node(value, ref);
    if(this->m_size == 0){
        this->m_pRoot = this->m_tail = n;
    } else {
        n->setPrev(this->m_tail);
        this->m_tail->setNext(n);
        this->m_tail = n;
    }
    this->m_tail->setNext(this->m_pRoot);
    this->m_pRoot->setPrev(this->m_tail);
    this->m_size++;
}

template <typename Trait>
CircularDoubleLinkedList<Trait>::CircularDoubleLinkedList(const CircularDoubleLinkedList &other) : Base() {
    shared_lock<shared_mutex> lock(other.m_mtx);
    if(!other.m_pRoot) return;
    Node* curr = other.m_pRoot;
    do {
        internal_push_back_cdll(curr->getData(), curr->getRef());
        curr = curr->getNext();
    } while(curr != other.m_pRoot);
}

template <typename Trait>
CircularDoubleLinkedList<Trait>::CircularDoubleLinkedList(CircularDoubleLinkedList &&other) noexcept : Base() {
    unique_lock<shared_mutex> lock(other.m_mtx);
    this->m_pRoot = std::exchange(other.m_pRoot, nullptr);
    this->m_tail  = std::exchange(other.m_tail,  nullptr);
    this->m_size  = std::exchange(other.m_size,  0);
}

template <typename Trait>
CircularDoubleLinkedList<Trait>& CircularDoubleLinkedList<Trait>::operator=(const CircularDoubleLinkedList &other){
    if(this == &other) return *this;
    unique_lock<shared_mutex> lk_this(this->m_mtx);
    shared_lock<shared_mutex> lk_other(other.m_mtx);
    internal_clear_cdll();
    if(!other.m_pRoot) return *this;
    Node* curr = other.m_pRoot;
    do {
        internal_push_back_cdll(curr->getData(), curr->getRef());
        curr = curr->getNext();
    } while(curr != other.m_pRoot);
    return *this;
}

template <typename Trait>
CircularDoubleLinkedList<Trait>& CircularDoubleLinkedList<Trait>::operator=(CircularDoubleLinkedList &&other) noexcept{
    if(this == &other) return *this;
    unique_lock<shared_mutex> lk_this(this->m_mtx);
    unique_lock<shared_mutex> lk_other(other.m_mtx);
    internal_clear_cdll();
    this->m_pRoot = std::exchange(other.m_pRoot, nullptr);
    this->m_tail  = std::exchange(other.m_tail,  nullptr);
    this->m_size  = std::exchange(other.m_size,  0);
    return *this;
}

template <typename Trait>
CircularDoubleLinkedList<Trait>::~CircularDoubleLinkedList(){
    unique_lock<shared_mutex> lock(this->m_mtx);
    internal_clear_cdll();
}

template <typename Trait>
void CircularDoubleLinkedList<Trait>::push_front(value_type value, Ref ref){
    unique_lock<shared_mutex> lock(this->m_mtx);
    Node* n = new Node(value, ref);
    if(this->m_size == 0){
        this->m_pRoot = this->m_tail = n;
    } else {
        n->setNext(this->m_pRoot);
        this->m_pRoot->setPrev(n);
        this->m_pRoot = n;
    }
    this->m_tail->setNext(this->m_pRoot);
    this->m_pRoot->setPrev(this->m_tail);
    this->m_size++;
}

template <typename Trait>
std::tuple<typename CircularDoubleLinkedList<Trait>::value_type, Ref>
CircularDoubleLinkedList<Trait>::pop_front(){
    unique_lock<shared_mutex> lock(this->m_mtx);
    if(!this->m_pRoot)
        throw out_of_range("CDLL::pop_front: lista vacia");
    Node* old = this->m_pRoot;
    auto result = std::make_tuple(old->getData(), old->getRef());
    if(this->m_size == 1){
        this->m_pRoot = this->m_tail = nullptr;
    } else {
        this->m_pRoot = old->getNext();
        this->m_tail->setNext(this->m_pRoot);
        this->m_pRoot->setPrev(this->m_tail);
    }
    delete old;
    this->m_size--;
    return result;
}

template <typename Trait>
void CircularDoubleLinkedList<Trait>::push_back(value_type value, Ref ref){
    unique_lock<shared_mutex> lock(this->m_mtx);
    internal_push_back_cdll(value, ref);
}

template <typename Trait>
std::tuple<typename CircularDoubleLinkedList<Trait>::value_type, Ref>
CircularDoubleLinkedList<Trait>::pop_back(){
    unique_lock<shared_mutex> lock(this->m_mtx);
    if(!this->m_tail)
        throw out_of_range("CDLL::pop_back: lista vacia");
    Node* old = this->m_tail;
    auto result = std::make_tuple(old->getData(), old->getRef());
    if(this->m_size == 1){
        this->m_pRoot = this->m_tail = nullptr;
    } else {
        this->m_tail = old->getPrev();
        this->m_tail->setNext(this->m_pRoot);
        this->m_pRoot->setPrev(this->m_tail);
    }
    delete old;
    this->m_size--;
    return result;
}

template <typename Trait>
void CircularDoubleLinkedList<Trait>::insert(const value_type &value, Ref ref){
    unique_lock<shared_mutex> lock(this->m_mtx);
    if(this->m_size == 0){
        Node* n = new Node(value, ref);
        this->m_pRoot = this->m_tail = n;
        n->setNext(n);
        n->setPrev(n);
        this->m_size++;
        return;
    }
    if(this->m_comp(value, this->m_pRoot->getDataRef())){
        Node* n = new Node(value, ref);
        n->setNext(this->m_pRoot);
        this->m_pRoot->setPrev(n);
        this->m_pRoot = n;
        this->m_tail->setNext(this->m_pRoot);
        this->m_pRoot->setPrev(this->m_tail);
        this->m_size++;
        return;
    }
    Node* prev = this->m_pRoot;
    while(prev->getNext() != this->m_pRoot
          && !this->m_comp(value, prev->getNext()->getDataRef())){
        prev = prev->getNext();
    }
    Node* nxt = prev->getNext();
    Node* n   = new Node(value, ref);
    n->setNext(nxt);
    n->setPrev(prev);
    prev->setNext(n);
    nxt->setPrev(n);
    if(prev == this->m_tail){
        this->m_tail = n;
        this->m_tail->setNext(this->m_pRoot);
        this->m_pRoot->setPrev(this->m_tail);
    }
    this->m_size++;
}

#endif // __CDLL_H__
