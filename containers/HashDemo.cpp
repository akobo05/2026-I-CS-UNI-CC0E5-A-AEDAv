#include <iostream>
#include <cassert>
#include "avl.h"
using namespace std;

void DemoAVL(){
    AVL<AscendingAVLTrait<T1>> t;
    T1 valores[] = {30, 10, 20, 40, 50, 25};
    Ref refs[]   = {300, 100, 200, 400, 500, 250};
    for(Index i = 0; i < 6; ++i){
        bool ins = t.insert(valores[i], refs[i]);
        assert(ins);
    }
    assert(t.size() == 6);
    Ref out;
    assert(t.find(25, out)); assert(out == 250);
    assert(t.find(50, out)); assert(out == 500);
    assert(!t.find(99, out));
    cout << "DemoAVL insert+find OK: " << t << "\n";

    // Forzar rotaciones (insertar ordenadamente)
    AVL<AscendingAVLTrait<T1>> t2;
    for(T1 i = 1; i <= 7; ++i) t2.insert(i, Ref(i * 10));
    assert(t2.size() == 7);
    cout << "DemoAVL balanceo OK: " << t2 << "\n";

    // Duplicado: update ref, no insertar
    AVL<AscendingAVLTrait<T1>> t3;
    t3.insert(T1(5), Ref(50));
    bool dup = t3.insert(T1(5), Ref(999));
    assert(!dup);
    assert(t3.size() == 1);
    Ref out3;
    assert(t3.find(T1(5), out3));
    assert(out3 == 999);
    cout << "DemoAVL duplicado actualiza ref OK\n";

    // Remove
    AVL<AscendingAVLTrait<T1>> tr;
    for(T1 i = 1; i <= 7; ++i) tr.insert(i, Ref(i * 10));
    assert(tr.size() == 7);

    assert(tr.remove(T1(4)));   // nodo interno
    assert(tr.size() == 6);
    Ref tmpRef;
    assert(!tr.find(T1(4), tmpRef));

    assert(tr.remove(T1(1)));   // hoja
    assert(tr.size() == 5);

    assert(!tr.remove(T1(99))); // inexistente
    cout << "DemoAVL remove OK: " << tr << "\n";
}

// Placeholder — will be filled in Task 13+
void DemoHashTable(){}
