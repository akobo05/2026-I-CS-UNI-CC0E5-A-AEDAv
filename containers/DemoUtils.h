#ifndef __DEMO_UTILS_H__
#define __DEMO_UTILS_H__

#include <iostream>
#include <sstream>
#include <thread>
#include <vector>
#include <string>

inline void printSection(const std::string& t) { std::cout << "\n[" << t << "]\n"; }
inline void printHeader (const std::string& n) { std::cout << "\n             PRUEBAS " << n << "             \n"; }
inline void printFooter (const std::string& n) { std::cout << "\n             FIN "     << n << "             \n"; }

template <typename Container, typename InsertFn>
void testCopyMove(Container& original, InsertFn insertExtra) {
    printSection("Copy constructor");
    Container copia(original);
    insertExtra(copia);
    std::cout << "  Original : size=" << original.size() << "  " << original << "\n";
    std::cout << "  Copia    : size=" << copia.size()    << "  " << copia    << "\n";
    printSection("Move constructor");
    Container movida(std::move(copia));
    std::cout << "  Movida   : size=" << movida.size() << "  " << movida << "\n";
    std::cout << "  Fuente tras move: size=" << copia.size() << "\n";
}

template <typename Container>
void testIO(const Container& c) {
    printSection("operator<< / operator>>");
    std::ostringstream oss; oss << c;
    std::cout << "  Serializado : " << oss.str() << "\n";
    Container c2; std::istringstream iss(oss.str()); iss >> c2;
    std::cout << "  Deserializado: " << c2 << "\n";
}

template <typename Container, typename WorkerFn>
void testConcurrency(Container& c, WorkerFn worker, std::size_t nThreads,
                     std::size_t expected, const std::string& label = "size") {
    printSection("Concurrencia");
    std::vector<std::thread> ts; ts.reserve(nThreads);
    for (std::size_t i = 0; i < nThreads; ++i) ts.emplace_back(worker, std::ref(c), (int)i + 1);
    for (auto& t : ts) t.join();
    std::cout << "  " << label << " (esperado " << expected << "): " << c.size() << "\n";
    std::cout << (c.size() == expected ? "  EXITO - sin condiciones de carrera\n"
                                       : "  FALLO - corrupcion detectada\n");
}

#endif // __DEMO_UTILS_H__
