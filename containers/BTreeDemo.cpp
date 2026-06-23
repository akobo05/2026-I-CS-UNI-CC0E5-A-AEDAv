#include <iostream>
#include <string>
#include <cctype>
#include <cassert>
#include "../types.h"
#include "traits.h"
#include "BTree.h"        // PascalCase: compila en Linux
#include "DemoUtils.h"

void DemoBTree() {
    using Trait = Tree34Trait<TypeBTree>;
    using BT    = BTree<Trait>;
    using Entry = BT::Entry;
    printHeader("BTREE (orden 3 / std::string)");

    BT bt;
    const std::string ks = "D1XJ2xTg8zKL9AhijOPQcEowRSp0NbW567BUfCqrs4FdtYZakHIuvGV3eMylmn";
    Size n = 0;
    for (char c : ks) if (bt.insert(std::string(1, c), (Ref)(n * n))) ++n;
    printSection("insert");
    std::cout << "  size=" << bt.size() << "  height=" << bt.height() << "  order=" << bt.order() << "\n";

    printSection("search");
    try { auto [v, r] = bt.search("Z"); std::cout << "  search('Z') -> valor=" << v << " ref=" << r << "\n"; }
    catch (const std::exception& e) { std::cout << "  " << e.what() << "\n"; }
    try { bt.search("!"); } catch (const std::exception& e) { std::cout << "  search('!') -> " << e.what() << "\n"; }

    printSection("forEach variadic (1 arg extra)");
    Size letras = 0;
    bt.forEach([](Entry& e, Level, Size& cnt){ if (!e.m_data.empty() && std::isalpha((unsigned char)e.m_data[0])) ++cnt; }, letras);
    std::cout << "  letras en el arbol: " << letras << "\n";

    printSection("firstThat variadic (1 arg extra)");
    if (Entry* hit = bt.firstThat([](Entry& e, Level, const std::string& t){ return e.m_data == t; }, std::string("M")))
        std::cout << "  firstThat('M') -> ref=" << hit->m_ref << "\n";

    printSection("remove");
    Size before = bt.size();
    auto [rv, rr] = bt.remove("A");
    std::cout << "  remove('A') -> valor=" << rv << " ref=" << rr << "  size " << before << " -> " << bt.size() << "\n";

    printSection("iterador inorder (range-for)");
    std::cout << "  claves: ";
    for (auto& e : bt) std::cout << e.m_data;
    std::cout << "\n";

    printSection("useCount");
    bt.search("B"); bt.search("B");
    if (Entry* b = bt.firstThat([](Entry& e, Level, const std::string& t){ return e.m_data == t; }, std::string("B")))
        std::cout << "  'B' useCount=" << b->useCount() << "\n";

    testIO(bt);                                            // operator<< / >> reutilizados
    testCopyMove(bt, [](BT& c){ c.insert("!", 999); });    // copy/move reutilizados

    printSection("Traits: arbol 2-3 con el MISMO motor");
    BTree<Tree23Trait<TypeBTree>> bt23;
    for (char c : std::string("MDFABZCEG")) bt23.insert(std::string(1, c), (Ref)c);
    std::cout << "  bt23 size=" << bt23.size() << "  order=" << bt23.order() << "  " << bt23 << "\n";

    testConcurrency(bt, [](BT& t, int id){
        for (int i = 0; i < 200; ++i) t.insert("k" + std::to_string(i), (Ref)id);
    }, 5, bt.size() + 200, "size");

    printFooter("BTREE");
}
