#ifndef __TYPES_H__
#define __TYPES_H__

// C/C++
// typedef int Type;

// C++11, C++14, C++17, C++20, C++23 ...
using Type = int;

// T1 must be int for 32-bit architecture and long long for 64-bit architecture
// It must work for windows, linux, iOS, macOS, android, etc.

using T1 = int;

using Ref = long;

#include <cstddef>   // std::size_t
#include <string>    // std::string

using Size      = std::size_t;   // conteos / índices no negativos
using Level     = std::size_t;   // profundidad en recorridos
using Flag      = bool;          // resultados booleanos
using TypeBTree = std::string;   // value_type por defecto del demo (multibyte-safe)

#endif // __TYPES_H__

