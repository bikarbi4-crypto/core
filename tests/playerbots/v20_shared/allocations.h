#pragma once
#include <cstddef>
#include <cstdint>
#ifdef SHARED_ALLOCATIONS
namespace SharedCounters {
inline thread_local bool active=false;
inline thread_local std::uint64_t allocations=0, bytes=0, aiConstructions=0;
inline void record(std::size_t n) {if(active){++allocations;bytes+=n;}}
}
#endif
