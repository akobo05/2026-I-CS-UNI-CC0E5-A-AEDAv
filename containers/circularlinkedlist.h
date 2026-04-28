#ifndef __CIRCULARLINKEDLIST_H__
#define __CIRCULARLINKEDLIST_H__

#include "linkedlist.h"

template <typename T>
struct AscendingCLLTrait  : BaseTrait<T, less<T>,    LLNode<T>>{};

template <typename T>
struct DescendingCLLTrait : BaseTrait<T, greater<T>, LLNode<T>>{};

template <typename Container>
class CLLForwardIterator
    : public general_iterator<Container, CLLForwardIterator<Container>>{
public:
    using MySelf = CLLForwardIterator<Container>;
    using Parent = general_iterator<Container, MySelf>;
    using Node   = typename Container::Node;
private:
    Node *m_start;
public:
    CLLForwardIterator(Container *pContainer, Node *pNode)
        : Parent(pContainer, pNode), m_start(pNode) {}
    CLLForwardIterator(Container *pContainer, Node *pNode, bool /*sentinel*/)
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

template <typename Trait>
class CircularLinkedList : public LinkedList<Trait>{
public:
    using Base       = LinkedList<Trait>;
    using value_type = typename Trait::value_type;
    using Node       = typename Trait::Node;
    using MySelf     = CircularLinkedList<Trait>;

    using forward_iterator = CLLForwardIterator<MySelf>;
    friend forward_iterator;

protected:
    void internal_clear_cll();
    void internal_push_back_cll(const value_type &value, Ref ref);

public:
    CircularLinkedList() : Base() {}
    CircularLinkedList(const CircularLinkedList &other);
    CircularLinkedList(CircularLinkedList &&other) noexcept;
    CircularLinkedList& operator=(const CircularLinkedList &other);
    CircularLinkedList& operator=(CircularLinkedList &&other) noexcept;
    virtual ~CircularLinkedList();

    void   push_front(value_type value, Ref ref) override;
    std::tuple<value_type, Ref> pop_front() override;
    void   push_back(value_type value, Ref ref) override;
    std::tuple<value_type, Ref> pop_back() override;
    void   insert(const value_type &value, Ref ref) override;

    forward_iterator begin() { return forward_iterator(this, this->m_pRoot); }
    forward_iterator end()   { return forward_iterator(this, nullptr, true); }

    template <typename Func, typename... Args>
    void ForEach(Func func, Args &&... args){
        unique_lock<shared_mutex> lock(this->m_mtx);
        if(this->m_size == 0) return;
        for(auto& item : *this)
            func(item, std::forward<Args>(args)...);
    }

    template <typename Func, typename... Args>
    void circularForEach(size_t vueltas, Func func, Args &&... args){
        unique_lock<shared_mutex> lock(this->m_mtx);
        if(!this->m_pRoot || vueltas == 0) return;
        Node* act = this->m_pRoot;
        size_t pasos = this->m_size * vueltas;
        for(size_t i = 0; i < pasos; ++i){
            func(act->getDataRef(), std::forward<Args>(args)...);
            act = act->getNext();
        }
    }

    friend ostream& operator<<(ostream& os, const CircularLinkedList& list){
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
        if(list.m_pRoot)
            os << " ->root(" << list.m_pRoot->getData() << ")";
        return os;
    }

    friend istream& operator>>(istream& is, CircularLinkedList& list){
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
void CircularLinkedList<Trait>::internal_clear_cll(){
    if(this->m_tail) this->m_tail->setNext(nullptr);
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
void CircularLinkedList<Trait>::internal_push_back_cll(const value_type &value, Ref ref){
    Node* n = new Node(value, ref);
    if(this->m_size == 0){
        this->m_pRoot = this->m_tail = n;
    } else {
        this->m_tail->setNext(n);
        this->m_tail = n;
    }
    this->m_tail->setNext(this->m_pRoot);
    this->m_size++;
}

template <typename Trait>
CircularLinkedList<Trait>::CircularLinkedList(const CircularLinkedList &other) : Base() {
    shared_lock<shared_mutex> lock(other.m_mtx);
    if(!other.m_pRoot) return;
    Node* curr = other.m_pRoot;
    do {
        internal_push_back_cll(curr->getData(), curr->getRef());
        curr = curr->getNext();
    } while(curr != other.m_pRoot);
}

template <typename Trait>
CircularLinkedList<Trait>::CircularLinkedList(CircularLinkedList &&other) noexcept : Base() {
    unique_lock<shared_mutex> lock(other.m_mtx);
    this->m_pRoot = std::exchange(other.m_pRoot, nullptr);
    this->m_tail  = std::exchange(other.m_tail,  nullptr);
    this->m_size  = std::exchange(other.m_size,  0);
}

template <typename Trait>
CircularLinkedList<Trait>& CircularLinkedList<Trait>::operator=(const CircularLinkedList &other){
    if(this == &other) return *this;
    unique_lock<shared_mutex> lk_this(this->m_mtx);
    shared_lock<shared_mutex> lk_other(other.m_mtx);
    internal_clear_cll();
    if(!other.m_pRoot) return *this;
    Node* curr = other.m_pRoot;
    do {
        internal_push_back_cll(curr->getData(), curr->getRef());
        curr = curr->getNext();
    } while(curr != other.m_pRoot);
    return *this;
}

template <typename Trait>
CircularLinkedList<Trait>& CircularLinkedList<Trait>::operator=(CircularLinkedList &&other) noexcept{
    if(this == &other) return *this;
    unique_lock<shared_mutex> lk_this(this->m_mtx);
    unique_lock<shared_mutex> lk_other(other.m_mtx);
    internal_clear_cll();
    this->m_pRoot = std::exchange(other.m_pRoot, nullptr);
    this->m_tail  = std::exchange(other.m_tail,  nullptr);
    this->m_size  = std::exchange(other.m_size,  0);
    return *this;
}

template <typename Trait>
CircularLinkedList<Trait>::~CircularLinkedList(){
    unique_lock<shared_mutex> lock(this->m_mtx);
    internal_clear_cll();
}

template <typename Trait>
void CircularLinkedList<Trait>::push_front(value_type value, Ref ref){
    unique_lock<shared_mutex> lock(this->m_mtx);
    Node* n = new Node(value, ref, this->m_pRoot);
    if(this->m_size == 0){
        this->m_pRoot = this->m_tail = n;
    } else {
        this->m_pRoot = n;
    }
    this->m_tail->setNext(this->m_pRoot);
    this->m_size++;
}

template <typename Trait>
std::tuple<typename CircularLinkedList<Trait>::value_type, Ref>
CircularLinkedList<Trait>::pop_front(){
    unique_lock<shared_mutex> lock(this->m_mtx);
    if(!this->m_pRoot)
        throw out_of_range("CircularLinkedList::pop_front: lista vacia");
    Node* old = this->m_pRoot;
    auto result = std::make_tuple(old->getData(), old->getRef());
    if(this->m_size == 1){
        this->m_pRoot = this->m_tail = nullptr;
    } else {
        this->m_pRoot = old->getNext();
        this->m_tail->setNext(this->m_pRoot);
    }
    delete old;
    this->m_size--;
    return result;
}

template <typename Trait>
void CircularLinkedList<Trait>::push_back(value_type value, Ref ref){
    unique_lock<shared_mutex> lock(this->m_mtx);
    internal_push_back_cll(value, ref);
}

template <typename Trait>
std::tuple<typename CircularLinkedList<Trait>::value_type, Ref>
CircularLinkedList<Trait>::pop_back(){
    unique_lock<shared_mutex> lock(this->m_mtx);
    if(!this->m_tail)
        throw out_of_range("CircularLinkedList::pop_back: lista vacia");
    Node* old = this->m_tail;
    auto result = std::make_tuple(old->getData(), old->getRef());
    if(this->m_size == 1){
        this->m_pRoot = this->m_tail = nullptr;
    } else {
        Node* prev = this->m_pRoot;
        while(prev->getNext() != this->m_tail) prev = prev->getNext();
        this->m_tail = prev;
        this->m_tail->setNext(this->m_pRoot);
    }
    delete old;
    this->m_size--;
    return result;
}

template <typename Trait>
void CircularLinkedList<Trait>::insert(const value_type &value, Ref ref){
    unique_lock<shared_mutex> lock(this->m_mtx);
    if(this->m_size == 0){
        Node* n = new Node(value, ref);
        this->m_pRoot = this->m_tail = n;
        n->setNext(n);
        this->m_size++;
        return;
    }
    if(this->m_comp(value, this->m_pRoot->getDataRef())){
        Node* n = new Node(value, ref, this->m_pRoot);
        this->m_pRoot = n;
        this->m_tail->setNext(this->m_pRoot);
        this->m_size++;
        return;
    }
    Node* prev = this->m_pRoot;
    while(prev->getNext() != this->m_pRoot
          && !this->m_comp(value, prev->getNext()->getDataRef())){
        prev = prev->getNext();
    }
    Node* n = new Node(value, ref, prev->getNext());
    prev->setNext(n);
    if(prev == this->m_tail){
        this->m_tail = n;
        this->m_tail->setNext(this->m_pRoot);
    }
    this->m_size++;
}

#endif // __CIRCULARLINKEDLIST_H__
