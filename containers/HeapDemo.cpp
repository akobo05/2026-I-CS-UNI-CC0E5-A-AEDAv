#include <iostream>
#include <cassert>
#include "heap.h"
using namespace std;

void DemoMinHeap(){
    Heap<MinHeapTrait<T1>> h;
    T1 valores[] = {50, 10, 30, 20, 40};
    Ref refs[]   = {500, 100, 300, 200, 400};
    for(Index i = 0; i < 5; ++i) h.insert(valores[i], refs[i]);
    assert(h.size() == 5);
    assert(h.peek() == 10);
    cout << "DemoMinHeap insert OK: peek=" << h.peek() << ", size=" << h.size() << "\n";
}

void DemoMaxHeap(){
    Heap<MaxHeapTrait<T1>> h;
    T1 valores[] = {50, 10, 30, 20, 40};
    Ref refs[]   = {500, 100, 300, 200, 400};
    for(Index i = 0; i < 5; ++i) h.insert(valores[i], refs[i]);
    assert(h.size() == 5);
    assert(h.peek() == 50);
    cout << "DemoMaxHeap insert OK: peek=" << h.peek() << ", size=" << h.size() << "\n";
}
