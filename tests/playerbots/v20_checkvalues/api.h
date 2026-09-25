#pragma once
#include <cstdint>
#include <string>
#include <vector>

struct Counts { uint64_t allocations=0, calculateAllocations=0, calculations=0, clock=0, pmo=0, lookups=0; };
struct Result {
    bool result=false, threw=false;
    Counts counts;
    std::vector<std::string> events;
    std::vector<uint64_t> state;
};
Result Scenario(bool candidate, bool enabled, unsigned size, unsigned stage, bool fallback, unsigned flags);
uint64_t Batch(bool candidate, bool enabled, unsigned size, unsigned stage, uint64_t iterations);
extern thread_local bool countAllocations;
extern thread_local unsigned calculateDepth;
extern thread_local Counts counters;
