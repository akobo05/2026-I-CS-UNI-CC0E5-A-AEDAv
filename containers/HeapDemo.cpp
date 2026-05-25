#include <iostream>
#include <fstream>
#include <thread>
#include <vector>
#include <cassert>
#include "heap.h"
using namespace std;

void DemoConcurrentHeap();

void DemoMinHeap(){
    Heap<MinHeapTrait<T1>> h;
    T1 valores[] = {50, 10, 30, 20, 40};
    Ref refs[]   = {500, 100, 300, 200, 400};
    for(Index i = 0; i < 5; ++i) h.insert(valores[i], refs[i]);
    assert(h.size() == 5);
    assert(h.peek() == 10);
    cout << "DemoMinHeap insert OK: peek=" << h.peek() << ", size=" << h.size() << "\n";

    // Verify extract orden ascendente
    T1 esperado[] = {10, 20, 30, 40, 50};
    for(Index i = 0; i < 5; ++i){
        auto [val, ref] = h.extract();
        assert(val == esperado[i]);
    }
    assert(h.size() == 0);
    cout << "DemoMinHeap extract orden ascendente OK\n";

    // Persistencia
    Heap<MinHeapTrait<T1>> h2;
    T1 valores2[] = {7, 3, 9, 1, 5};
    Ref refs2[]   = {70, 30, 90, 10, 50};
    for(Index i = 0; i < 5; ++i) h2.insert(valores2[i], refs2[i]);

    ofstream out("MinHeap.txt");
    out << h2;
    out.close();

    Heap<MinHeapTrait<T1>> h3;
    ifstream in("MinHeap.txt");
    in >> h3;
    in.close();
    assert(h3.size() == 5);
    assert(h3.peek() == 1);
    cout << "DemoMinHeap persistencia OK: " << h3 << "\n";

    DemoConcurrentHeap();
}

void DemoMaxHeap(){
    Heap<MaxHeapTrait<T1>> h;
    T1 valores[] = {50, 10, 30, 20, 40};
    Ref refs[]   = {500, 100, 300, 200, 400};
    for(Index i = 0; i < 5; ++i) h.insert(valores[i], refs[i]);
    assert(h.size() == 5);
    assert(h.peek() == 50);
    cout << "DemoMaxHeap insert OK: peek=" << h.peek() << ", size=" << h.size() << "\n";

    T1 esperado[] = {50, 40, 30, 20, 10};
    for(Index i = 0; i < 5; ++i){
        auto [val, ref] = h.extract();
        assert(val == esperado[i]);
    }
    cout << "DemoMaxHeap extract orden descendente OK\n";

    // Persistencia
    Heap<MaxHeapTrait<T1>> h2;
    T1 valores2[] = {7, 3, 9, 1, 5};
    Ref refs2[]   = {70, 30, 90, 10, 50};
    for(Index i = 0; i < 5; ++i) h2.insert(valores2[i], refs2[i]);
    ofstream out("MaxHeap.txt");
    out << h2;
    out.close();
    Heap<MaxHeapTrait<T1>> h3;
    ifstream in("MaxHeap.txt");
    in >> h3;
    in.close();
    assert(h3.peek() == 9);
    cout << "DemoMaxHeap persistencia OK: " << h3 << "\n";
}

void DemoConcurrentHeap(){
    Heap<MinHeapTrait<T1>> h;
    auto worker = [&h](T1 base){
        for(T1 i = 0; i < 1000; ++i)
            h.insert(base * 1000 + i, Ref(base * 1000 + i));
    };
    std::vector<std::thread> ths;
    for(T1 t = 0; t < 5; ++t) ths.emplace_back(worker, t);
    for(auto& th : ths) th.join();
    assert(h.size() == 5000);
    cout << "DemoConcurrentHeap size=" << h.size() << " OK\n";
}
