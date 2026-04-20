#include <iostream>
#include <sstream>
#include <thread>
#include "linkedlist.h"
using namespace std;

void LinkedListDemo(){
    cout << "---- LinkedListDemo (orden descendente) ----" << endl;
    LinkedList<DescendingLinkedListTrait<T1>> list;
    list.insert(1, 15);
    list.insert(2, 25);
    list.insert(3, 35);
    list.insert(4, 45);
    list.insert(5, 55);
    cout << "insert: " << list << endl;

    list.push_front(6, 66);
    list.push_back(0, 99);
    cout << "push_front(6) + push_back(0): " << list << endl;

    list.pop_front();
    list.pop_back();
    cout << "pop_front + pop_back: " << list << endl;

    cout << "operator[2]: " << list[2] << endl;
    cout << "size(): " << list.size() << endl;

    cout << "---- ForEach (imprime cada elemento) ----" << endl;
    list.ForEach([](T1& x){ cout << x << " "; });
    cout << endl;

    cout << "---- operator>> desde string ----" << endl;
    LinkedList<AscendingLinkedListTrait<T1>> lRead;
    istringstream iss("[(10,100),(3,33),(7,77),(1,11)]");
    iss >> lRead;
    cout << "leido (ascendente, insert respeta orden): " << lRead << endl;

    cout << "---- Big-Five ----" << endl;
    LinkedList<AscendingLinkedListTrait<T1>> lCopy(lRead);
    cout << "copy: " << lCopy << endl;

    LinkedList<AscendingLinkedListTrait<T1>> lMove(std::move(lCopy));
    cout << "move (src queda vacia -> size=" << lCopy.size() << "): " << lMove << endl;

    LinkedList<AscendingLinkedListTrait<T1>> lAssign;
    lAssign = lRead;
    cout << "copy=: " << lAssign << endl;

    LinkedList<AscendingLinkedListTrait<T1>> lMoveAssign;
    lMoveAssign = std::move(lAssign);
    cout << "move=: " << lMoveAssign << endl;
}

void ConcurrentLinkedListDemo(){
    cout << "---- ConcurrentLinkedListDemo ----" << endl;
    LinkedList<AscendingLinkedListTrait<T1>> list;

    auto worker = [&list](int base){
        for(int i = 0; i < 100; ++i)
            list.push_back(base + i, base * 1000 + i);
    };
    thread t1(worker, 0);
    thread t2(worker, 1000);
    thread t3(worker, 2000);
    thread t4(worker, 3000);
    t1.join(); t2.join(); t3.join(); t4.join();

    cout << "size() esperado 400, obtenido: " << list.size() << endl;
}

void ListsDemo(){
    LinkedListDemo();
    ConcurrentLinkedListDemo();
}
