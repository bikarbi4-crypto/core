#pragma once
#include <cstdint>
struct Work { std::uint64_t copies=0,copiedEntries=0,registryEntries=0,mapScans=0,mapEntries=0,friendChecks=0,presenceEntries=0; };
inline thread_local Work work;
#ifdef PRESENCE_COUNT
#define PROBE(name) (++work.name)
#define COUNT(name, n) (work.name += (n))
#else
#define PROBE(name) ((void)0)
#define COUNT(name, n) ((void)0)
#endif
