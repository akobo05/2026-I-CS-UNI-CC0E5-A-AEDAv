#include <iostream>
#include <sstream>
#include <string>
#include <cassert>
#include <thread>
#include <vector>
#include "avl.h"
#include "hashtable.h"
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

void DemoConcurrentHashTable();

void DemoKVPair(){
    KVPair<T1, Type> a(5, 100);
    KVPair<T1, Type> b(7, 200);
    KVPair<T1, Type> c(5, 999);
    assert(a < b);          // ordena por key: 5 < 7
    assert(!(a < c));       // misma key (5): no es menor
    assert(a == c);         // misma key (5): iguales aunque el valor difiera
    cout << "DemoKVPair OK (compara solo por clave)\n";
}

void DemoHashTable(){
    DemoKVPair();

    cout << "\n========== HashTable (contiene un AVL que hereda de BinaryTree) - Criterios ==========\n";

    // [operator[]] : auto-insert y sobrescritura
    HashTable<HashTrait<T1, Type>> m;
    m[T1(5)]  = Type(3);
    m[T1(17)] = Type(42);
    m[T1(5)]  = Type(100);              // misma key: sobrescribe, no inserta
    cout << "\n[operator[]]\n";
    cout << "  m[5]  = " << m[T1(5)]  << "   (sobrescrito, esperado 100)\n";
    cout << "  m[17] = " << m[T1(17)] << "\n";
    cout << "  size  = " << m.size()  << "   (esperado 2)\n";
    assert(m.size() == 2);
    assert(m[T1(5)] == 100);
    assert(m.contains(T1(17)));
    assert(!m.contains(T1(99)));

    // [for (const auto& [key, value] : m)]
    HashTable<HashTrait<T1, Type>> mr;
    for(T1 i = 0; i < 5; ++i) mr[i] = Type(i * 100);
    cout << "\n[for (const auto& [key, value] : m)]\n";
    Index count = 0; Type sum = 0;
    for(const auto& [key, value] : mr){
        cout << "  key=" << key << "  value=" << value << "\n";
        ++count; sum = sum + value;
    }
    assert(count == 5);
    assert(sum == Type(1000));

    // [operator<<]  (reaprovechado del AVL contenido)
    cout << "\n[operator<<]  (reaprovechado del AVL contenido)\n";
    cout << "  " << mr << "\n";

    // [operator>>]  (reaprovechado del AVL contenido)
    cout << "\n[operator>>]  (reaprovechado del AVL contenido)\n";
    stringstream ss;
    ss << mr;
    HashTable<HashTrait<T1, Type>> mr2;
    ss >> mr2;
    cout << "  serializado  : " << mr  << "\n";
    cout << "  deserializado: " << mr2 << "\n";
    cout << "  sizes iguales: " << (mr.size() == mr2.size() ? "SI" : "NO") << "\n";
    assert(mr.size() == mr2.size());
    assert(mr2[T1(3)] == 300);

    // [Copy constructor]  (reaprovecha la copia del AVL contenido)
    cout << "\n[Copy constructor]  (reaprovecha la copia del AVL contenido)\n";
    HashTable<HashTrait<T1, Type>> mcopy(mr);
    mcopy[T1(99)] = Type(999);          // modifico solo la copia
    cout << "  original (sin 99): size=" << mr.size()    << "  " << mr    << "\n";
    cout << "  copia    (con 99): size=" << mcopy.size() << "  " << mcopy << "\n";
    cout << "  independientes   : " << (!mr.contains(T1(99)) ? "SI" : "NO") << "\n";
    assert(mr.size() == 5);
    assert(mcopy.size() == 6);
    assert(!mr.contains(T1(99)));

    // [Move constructor]  (reaprovecha el move del AVL contenido)
    cout << "\n[Move constructor]  (reaprovecha el move del AVL contenido)\n";
    HashTable<HashTrait<T1, Type>> mmoved(std::move(mcopy));
    cout << "  movida          : size=" << mmoved.size() << "\n";
    cout << "  fuente tras move: size=" << mcopy.size()  << "   (esperado 0)\n";
    assert(mmoved.size() == 6);
    assert(mcopy.size() == 0);

    // ---------- Extras ----------
    cout << "\n---------- Extras ----------\n";

    // Claves string (el AVL ordena por la clave; KVPair compara strings)
    HashTable<HashTrait<std::string, T1>> ms;
    ms[std::string("hola")] = T1(1);
    ms[std::string("aeda")] = T1(3);
    ms[std::string("uni")]  = T1(7);
    ms[std::string("hola")] = ms[std::string("hola")] + T1(10);
    assert(ms.size() == 3);
    assert(ms[std::string("hola")] == T1(11));
    cout << "  string-key: " << ms << "\n";

    DemoConcurrentHashTable();
}

void DemoConcurrentHashTable(){
    HashTable<HashTrait<T1, Type>> m;
    auto worker = [&m](T1 base){
        for(T1 i = 0; i < 1000; ++i){
            T1 k = base * 1000 + i;
            m[k] = Type(k);
        }
    };
    std::vector<std::thread> ths;
    for(T1 t = 0; t < 5; ++t) ths.emplace_back(worker, t);
    for(auto &th : ths) th.join();
    assert(m.size() == 5000);
    cout << "  concurrencia: size=" << m.size() << " (5 hilos x 1000 ops) OK\n";
}
