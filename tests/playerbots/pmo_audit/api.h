#pragma once
#include <cstdint>
#include <string>
#include <vector>

struct PmoCounters
{
    uint64_t starts=0, operations=0, clocks=0, lookups=0, positions=0;
    uint64_t actionNames=0, eventSources=0, work=0;
};
struct PmoResult
{
    PmoCounters counters;
    std::vector<std::string> events;
    std::vector<std::string> stats;
    std::vector<std::string> stack;
    size_t maps=0;
    int result=0;
};
struct PmoApi
{
    const char* label;
    PmoResult (*scenario)(int test, bool enabled, unsigned map, unsigned instance, unsigned flags);
    uint64_t (*batch)(int test, bool enabled, unsigned map, unsigned instance, unsigned flags, uint64_t iterations);
    void (*registry_threads)();
};
extern const PmoApi v18_api;
extern const PmoApi v19_api;
