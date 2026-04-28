#include <iostream>
#include <fstream>
#include <sstream>
#include <thread>
#include <tuple>
#include "../types.h"
#include "linkedlist.h"
#include "doublelinkedlist.h"
#include "circularlinkedlist.h"
#include "cdll.h"

using namespace std;

template <typename Container>
void DemoList(Container& list, const string& fileName){
    list.insert(28, 15);
    list.insert(17, 25);
    list.insert(8,  35);
    list.insert(4,  45);
    list.insert(35, 55);
    cout << "  Tras 5 inserts: " << list << endl;

    ofstream os(fileName);
    os << list << endl;
    os.close();

    Container fresh;
    ifstream is(fileName);
    is >> fresh;
    is.close();
    cout << "  Releido de " << fileName << ": " << fresh << endl;
}

void LinkedListDemo(){
    cout << "\n========== LinkedList ==========" << endl;
    cout << "-- Ascendente --" << endl;
    LinkedList<AscendingLinkedListTrait<T1>> asc;
    DemoList(asc, "AscLL.txt");

    cout << "-- Descendente --" << endl;
    LinkedList<DescendingLinkedListTrait<T1>> desc;
    DemoList(desc, "DescLL.txt");

    cout << "-- pop_front / pop_back devolviendo (data, ref) --" << endl;
    auto [df, rf] = asc.pop_front();
    auto [db, rb] = asc.pop_back();
    cout << "  pop_front -> (" << df << "," << rf << ")" << endl;
    cout << "  pop_back  -> (" << db << "," << rb << ")" << endl;
    cout << "  resto: " << asc << endl;

    cout << "-- Big Five --" << endl;
    LinkedList<AscendingLinkedListTrait<T1>> a;
    a.push_back(1, 10); a.push_back(2, 20); a.push_back(3, 30);
    LinkedList<AscendingLinkedListTrait<T1>> b(a);
    LinkedList<AscendingLinkedListTrait<T1>> c(std::move(a));
    cout << "  copy ctor:  " << b << endl;
    cout << "  move ctor:  " << c << "  (origen vacio: size=" << a.size() << ")" << endl;
    LinkedList<AscendingLinkedListTrait<T1>> d; d = b;
    LinkedList<AscendingLinkedListTrait<T1>> e; e = std::move(b);
    cout << "  copy=:      " << d << endl;
    cout << "  move=:      " << e << "  (origen vacio: size=" << b.size() << ")" << endl;
}

void DoubleLinkedListDemo(){
    cout << "\n========== DoubleLinkedList ==========" << endl;
    cout << "-- Ascendente --" << endl;
    DoubleLinkedList<AscendingDLLTrait<T1>> asc;
    DemoList(asc, "AscDLL.txt");

    cout << "-- Descendente --" << endl;
    DoubleLinkedList<DescendingDLLTrait<T1>> desc;
    DemoList(desc, "DescDLL.txt");

    cout << "-- Iterador BACKWARD (rbegin -> rend) --" << endl;
    DoubleLinkedList<AscendingDLLTrait<T1>> dll;
    dll.push_back(1, 10); dll.push_back(2, 20); dll.push_back(3, 30); dll.push_back(4, 40);
    cout << "  forward : ";
    for(auto it = dll.begin();  it != dll.end();  ++it) cout << *it << " ";
    cout << "\n  backward: ";
    for(auto it = dll.rbegin(); it != dll.rend(); ++it) cout << *it << " ";
    cout << endl;

    cout << "-- ReverseForEach (metodo propio de DLL) --" << endl;
    cout << "  ";
    dll.ReverseForEach([](T1& x){ cout << x << " "; });
    cout << endl;

    cout << "-- pop_back O(1) (DLL aprovecha m_pPrev) --" << endl;
    auto [d, r] = dll.pop_back();
    cout << "  pop_back -> (" << d << "," << r << "), resto: " << dll << endl;

    cout << "-- Big Five DLL --" << endl;
    DoubleLinkedList<AscendingDLLTrait<T1>> x;
    x.push_back(7, 70); x.push_back(8, 80); x.push_back(9, 90);
    DoubleLinkedList<AscendingDLLTrait<T1>> y(x);
    DoubleLinkedList<AscendingDLLTrait<T1>> z(std::move(x));
    cout << "  copy ctor: " << y << endl;
    cout << "  move ctor: " << z << "  (origen vacio: size=" << x.size() << ")" << endl;
}

void CircularLinkedListDemo(){
    cout << "\n========== CircularLinkedList ==========" << endl;
    cout << "-- Ascendente --" << endl;
    CircularLinkedList<AscendingCLLTrait<T1>> asc;
    DemoList(asc, "AscCLL.txt");

    cout << "-- Verificar circularidad: dos vueltas con iterador propio --" << endl;
    cout << "  vuelta 1: ";
    for(auto it = asc.begin(); it != asc.end(); ++it) cout << *it << " ";
    cout << "\n  vuelta 2: ";
    for(auto it = asc.begin(); it != asc.end(); ++it) cout << *it << " ";
    cout << endl;

    cout << "-- circularForEach(3) (3 vueltas completas) --" << endl;
    cout << "  ";
    asc.circularForEach(3, [](T1& x){ cout << x << " "; });
    cout << endl;

    cout << "-- pop_front / pop_back --" << endl;
    auto [df, rf] = asc.pop_front();
    auto [db, rb] = asc.pop_back();
    cout << "  pop_front -> (" << df << "," << rf << ")" << endl;
    cout << "  pop_back  -> (" << db << "," << rb << ")" << endl;
    cout << "  resto: " << asc << endl;

    cout << "-- Big Five CLL --" << endl;
    CircularLinkedList<AscendingCLLTrait<T1>> a;
    a.push_back(1, 10); a.push_back(2, 20); a.push_back(3, 30);
    CircularLinkedList<AscendingCLLTrait<T1>> b(a);
    CircularLinkedList<AscendingCLLTrait<T1>> c(std::move(a));
    cout << "  copy ctor: " << b << endl;
    cout << "  move ctor: " << c << "  (origen vacio: size=" << a.size() << ")" << endl;
}

void CircularDoubleLinkedListDemo(){
    cout << "\n========== CircularDoubleLinkedList (CDLL) ==========" << endl;
    cout << "-- Ascendente --" << endl;
    CircularDoubleLinkedList<AscendingCDLLTrait<T1>> asc;
    DemoList(asc, "AscCDLL.txt");

    cout << "-- Descendente --" << endl;
    CircularDoubleLinkedList<DescendingCDLLTrait<T1>> desc;
    DemoList(desc, "DescCDLL.txt");

    cout << "-- Forward y Backward (con detencion en root/tail) --" << endl;
    CircularDoubleLinkedList<AscendingCDLLTrait<T1>> cdll;
    cdll.push_back(1, 10); cdll.push_back(2, 20); cdll.push_back(3, 30); cdll.push_back(4, 40);
    cout << "  forward : ";
    for(auto it = cdll.begin();  it != cdll.end();  ++it) cout << *it << " ";
    cout << "\n  backward: ";
    for(auto it = cdll.rbegin(); it != cdll.rend(); ++it) cout << *it << " ";
    cout << endl;

    cout << "-- ReverseForEach + circularForEach(2, dir) --" << endl;
    cout << "  ReverseForEach            : ";
    cdll.ReverseForEach([](T1& x){ cout << x << " "; });
    cout << "\n  circularForEach(2, +1) fwd: ";
    cdll.circularForEach(2, +1, [](T1& x){ cout << x << " "; });
    cout << "\n  circularForEach(2, -1) bwd: ";
    cdll.circularForEach(2, -1, [](T1& x){ cout << x << " "; });
    cout << endl;

    cout << "-- pop_back O(1) en CDLL (aprovecha prev del tail) --" << endl;
    auto [d, r] = cdll.pop_back();
    cout << "  pop_back -> (" << d << "," << r << "), resto: " << cdll << endl;

    cout << "-- Big Five CDLL --" << endl;
    CircularDoubleLinkedList<AscendingCDLLTrait<T1>> x;
    x.push_back(7, 70); x.push_back(8, 80); x.push_back(9, 90);
    CircularDoubleLinkedList<AscendingCDLLTrait<T1>> y(x);
    CircularDoubleLinkedList<AscendingCDLLTrait<T1>> z(std::move(x));
    cout << "  copy ctor: " << y << endl;
    cout << "  move ctor: " << z << "  (origen vacio: size=" << x.size() << ")" << endl;
}

void ConcurrentDemo(){
    cout << "\n========== Concurrencia (5 hilos x 1000 ops) ==========" << endl;

    LinkedList<AscendingLinkedListTrait<T1>> ll;
    auto wll = [&ll](int id){ for(int i = 0; i < 1000; ++i) ll.push_front(i, id); };
    thread t1(wll,1), t2(wll,2), t3(wll,3), t4(wll,4), t5(wll,5);
    t1.join(); t2.join(); t3.join(); t4.join(); t5.join();
    cout << "LinkedList               size=" << ll.size()
         << (ll.size() == 5000 ? "  OK" : "  FALLO") << endl;

    DoubleLinkedList<AscendingDLLTrait<T1>> dll;
    auto wdll = [&dll](int id){ for(int i = 0; i < 1000; ++i) dll.push_back(i, id); };
    thread u1(wdll,1), u2(wdll,2), u3(wdll,3), u4(wdll,4), u5(wdll,5);
    u1.join(); u2.join(); u3.join(); u4.join(); u5.join();
    cout << "DoubleLinkedList         size=" << dll.size()
         << (dll.size() == 5000 ? "  OK" : "  FALLO") << endl;

    CircularLinkedList<AscendingCLLTrait<T1>> cll;
    auto wcll = [&cll](int id){ for(int i = 0; i < 1000; ++i) cll.push_back(i, id); };
    thread v1(wcll,1), v2(wcll,2), v3(wcll,3), v4(wcll,4), v5(wcll,5);
    v1.join(); v2.join(); v3.join(); v4.join(); v5.join();
    cout << "CircularLinkedList       size=" << cll.size()
         << (cll.size() == 5000 ? "  OK" : "  FALLO") << endl;

    CircularDoubleLinkedList<AscendingCDLLTrait<T1>> cdll;
    auto wcdll = [&cdll](int id){ for(int i = 0; i < 1000; ++i) cdll.push_back(i, id); };
    thread w1(wcdll,1), w2(wcdll,2), w3(wcdll,3), w4(wcdll,4), w5(wcdll,5);
    w1.join(); w2.join(); w3.join(); w4.join(); w5.join();
    cout << "CircularDoubleLinkedList size=" << cdll.size()
         << (cdll.size() == 5000 ? "  OK" : "  FALLO") << endl;
}

void ListsDemo(){
    LinkedListDemo();
    DoubleLinkedListDemo();
    CircularLinkedListDemo();
    CircularDoubleLinkedListDemo();
    ConcurrentDemo();
    cout << "\n========== Fin de los demos ==========" << endl;
}
