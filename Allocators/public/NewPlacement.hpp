#pragma once

#include "Common.hpp"

inline void* operator new(std::size_t aSize, void* aWhere) noexcept { return aWhere; }
inline void* operator new[](std::size_t aSize, void* aWhere) noexcept { return aWhere; }
