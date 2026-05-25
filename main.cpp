#include "containers/vector.h"
#include "containers/linkedlist.h"
// g++ -std=c++2b main.cpp containers/vector.cpp -o main
void ListsDemo();
void DemoMinHeap();
void DemoMaxHeap();
void DemoAVL();
void DemoHashTable();
int main(){
    // DemoVector();
    //DemoConcurrentVector();
    ListsDemo();
    DemoMinHeap();
    DemoMaxHeap();
    DemoAVL();
    DemoHashTable();
    return 0;
}