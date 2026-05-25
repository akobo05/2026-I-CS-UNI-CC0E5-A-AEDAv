#include <iostream>
#include <fstream>
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

    // Range-for con structured binding (REQUISITO LITERAL)
    HashTable<DefaultHashTrait<T1, Type>> m_iter;
    for(T1 i = 0; i < 5; ++i) m_iter[i] = Type(i * 100);
    Index count_iter = 0;
    Type  sum = 0;
    for(const auto& [key, value] : m_iter){
        ++count_iter;
        sum = sum + value;
        (void)key;
    }
    assert(count_iter == 5);
    assert(sum == Type(0 + 100 + 200 + 300 + 400));
    cout << "DemoHashTable range-for OK (count=" << count_iter << ", sum=" << sum << ")\n";

    // Persistencia
    ofstream out("HashTable.txt");
    out << m_iter;
    out.close();
    HashTable<DefaultHashTrait<T1, Type>> m_loaded;
    ifstream in("HashTable.txt");
    in >> m_loaded;
    in.close();
    assert(m_loaded.size() == 5);
    assert(m_loaded[T1(3)] == 300);
    cout << "DemoHashTable persistencia OK: " << m_loaded << "\n";

    // Key de tipo no integral (string) — Knuth se apoya en std::hash
    HashTable<DefaultHashTrait<std::string, T1>> ms;
    std::string k_hola = "hola";
    std::string k_aeda = "aeda";
    std::string k_unip = "uni";
    ms[k_hola] = T1(1);
    ms[k_aeda] = T1(3);
    ms[k_unip] = T1(7);
    ms[k_hola] = ms[k_hola] + T1(10);   // update via operator[]
    assert(ms.size() == 3);
    assert(ms[k_hola] == T1(11));
    assert(ms.contains(k_aeda));
    assert(!ms.contains(std::string("noexiste")));
    cout << "DemoHashTable string-key OK: " << ms << "\n";

    DemoConcurrentHashTable();
}

void DemoConcurrentHashTable(){
    HashTable<DefaultHashTrait<T1, Type>> m;
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
    cout << "DemoConcurrentHashTable size=" << m.size()
         << " buckets=" << m.bucket_count() << " OK\n";
}
