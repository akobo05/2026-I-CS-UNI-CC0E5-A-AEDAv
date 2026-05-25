#include <iostream>
#include <cassert>
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

void DemoKVPair(){
    KVPair<T1, Type> a(5, 100);
    KVPair<T1, Type> b(7, 200);
    KVPair<T1, Type> c(5, 999);
    assert(a < b);
    assert(!(a < c));
    assert(a == c);
    KnuthMultiplicativeHash<T1> h;
    HashValue idx = h(T1(42), BucketCount(16));
    assert(idx >= 0 && idx < 16);
    cout << "DemoKVPair OK (hash de 42 mod 16 = " << idx << ")\n";
}

void DemoHashTable(){
    DemoKVPair();

    HashTable<DefaultHashTrait<T1, Type>> m;
    T1 k1 = 5; Type v1 = 3;
    m[k1] = v1;
    assert(m.size() == 1);
    assert(m[k1] == 3);
    T1 k2 = 17; Type v2 = 42;
    m[k2] = v2;
    assert(m.size() == 2);
    assert(m.contains(k2));
    assert(!m.contains(T1(99)));
    cout << "DemoHashTable basico OK (size=" << m.size() << ")\n";

    // copy + move ctors
    HashTable<DefaultHashTrait<T1, Type>> m_copy(m);
    assert(m_copy.size() == 2);
    assert(m_copy[T1(5)] == 3);
    HashTable<DefaultHashTrait<T1, Type>> m_moved(std::move(m_copy));
    assert(m_moved.size() == 2);
    assert(m_copy.size() == 0);
    cout << "DemoHashTable copy+move OK\n";

    // rehash con muchos inserts
    HashTable<DefaultHashTrait<T1, Type>> big;
    BucketCount initial_bc = big.bucket_count();
    for(T1 i = 0; i < 100; ++i) big[i] = Type(i * 10);
    assert(big.size() == 100);
    assert(big.bucket_count() > initial_bc);
    cout << "DemoHashTable rehash OK (buckets " << initial_bc
         << " -> " << big.bucket_count() << ")\n";
}
