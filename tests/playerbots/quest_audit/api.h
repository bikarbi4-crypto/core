#pragma once
#include <cstdint>
#include <string>
#include <vector>
struct Case {
    unsigned objectives=4, incompleteMask=15, quests=1, destinations=16;
    int match=-1; // -1 absent, 0 first, 1 last
    int kind=0; // 0 creature, 1 GO, 2 item
    int state=0; // 0 active incomplete, 1 complete, 2 inactive, 3 absent template, 4 not in log
    unsigned dropSize=8, vendorSize=8, money=101;
    bool prototype=true, refresh=false, monitor=false, unreachable=false;
    bool itemOnly=false, sharedCold=false;
};
struct Observation {
    bool result=false;
    std::vector<std::string> trace;
    std::vector<std::uint32_t> destinations;
    unsigned distanceCalls=0, rngCalls=0;
};
struct Api {
    const char* name;
    void* (*create)(const Case&);
    void (*destroy)(void*);
    std::uint64_t (*run)(void*,std::uint64_t);
    Observation (*observe)(void*,unsigned);
    void (*policy)();
    std::string (*qualifier)(std::uint32_t,unsigned);
};
Api v171_api(); Api a_only_api(); Api b_only_api(); Api c_only_api(); Api v18_api();
