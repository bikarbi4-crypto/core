#include "api.h"
#include <algorithm>
#include <cassert>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <new>
#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#endif
thread_local bool countAllocations=false;
thread_local unsigned calculateDepth=0;
thread_local Counts counters;
#ifdef CHECKVALUES_COUNT_ALLOC
void* operator new(size_t n) {
    if(countAllocations) { ++counters.allocations; if(calculateDepth) ++counters.calculateAllocations; }
    if(void* p=std::malloc(n ? n : 1)) return p;
    throw std::bad_alloc();
}
void* operator new[](size_t n) { return ::operator new(n); }
void operator delete(void* p) noexcept { std::free(p); }
void operator delete[](void* p) noexcept { std::free(p); }
void operator delete(void* p,size_t) noexcept { std::free(p); }
void operator delete[](void* p,size_t) noexcept { std::free(p); }
#endif
int main(int argc,char** argv) {
    if(argc==3 && !std::strcmp(argv[1],"--benchmark")) {
#ifdef CHECKVALUES_COUNT_ALLOC
        std::cerr<<"Use the separate checkvalues_benchmark executable without allocator instrumentation.\n";
        return 2;
#endif
#ifdef _WIN32
        assert(SetProcessAffinityMask(GetCurrentProcess(),4));
#endif
        std::ofstream out(argv[2]);
        out<<"stage,size,pmo,round,variant,iterations,ns_per_call,checksum\n";
        for(unsigned stage:{0u,1u,2u,3u}) for(unsigned size:{0u,8u,64u,512u}) for(bool on:{false,true}) {
            uint64_t n=100;
            for(bool candidate:{false,true}) {
                auto start=std::chrono::steady_clock::now(); Batch(candidate,on,size,stage,n);
                double elapsed=std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count();
                if(elapsed<0.012) n=std::min<uint64_t>(1000000,uint64_t(n*0.016/std::max(elapsed,0.000001)));
            }
            for(unsigned round=0;round<9;++round) {
                uint64_t first=0;
                for(unsigned order=0;order<2;++order) {
                    bool candidate=(round+order)%2;
                    auto start=std::chrono::steady_clock::now(); auto sum=Batch(candidate,on,size,stage,n);
                    auto ns=std::chrono::duration<double,std::nano>(std::chrono::steady_clock::now()-start).count()/n;
                    if(order) assert(first==sum); else first=sum;
                    out<<stage<<','<<size<<','<<on<<','<<round<<','<<(candidate?"v20":"v19")<<','<<n<<','<<ns<<','<<sum<<'\n';
                }
            }
        }
        std::cout<<"CheckValues benchmark passed paired checksums.\n"; return 0;
    }
#ifndef CHECKVALUES_COUNT_ALLOC
    std::cerr<<"Use checkvalues_audit for parity/allocation checks.\n";
    return 2;
#endif
    std::ofstream allocations;
    if(argc==3 && !std::strcmp(argv[1],"--allocations")) allocations.open(argv[2]);
    if(allocations) allocations<<"pmo,size,stage,fallback,flags,v19_alloc,v20_alloc,v19_calc_alloc,v20_calc_alloc,calculations\n";
    unsigned observations=0;
    for(bool on:{false,true}) for(unsigned size:{0u,1u,8u,64u,512u})
        for(unsigned stage=0;stage<8;++stage) for(bool fallback:{false,true}) for(unsigned flags=0;flags<16;++flags) {
            auto a=Scenario(false,on,size,stage,fallback,flags), b=Scenario(true,on,size,stage,fallback,flags);
            assert(a.result==b.result && a.threw==b.threw && a.events==b.events && a.state==b.state);
            assert(a.counts.calculations==b.counts.calculations && a.counts.clock==b.counts.clock);
            assert(a.counts.pmo==b.counts.pmo && a.counts.lookups==b.counts.lookups);
            assert(a.counts.calculateAllocations==b.counts.calculateAllocations);
            if(fallback) assert(a.counts.allocations==b.counts.allocations);
            else assert(a.counts.allocations>=b.counts.allocations);
            if(!fallback && stage<4) assert(a.counts.allocations-b.counts.allocations>=6*size);
            if(!on) assert(a.counts.pmo==0);
            if(allocations) allocations<<on<<','<<size<<','<<stage<<','<<fallback<<','<<flags<<','<<a.counts.allocations<<','<<b.counts.allocations<<','<<a.counts.calculateAllocations<<','<<b.counts.calculateAllocations<<','<<b.counts.calculations<<'\n';
            ++observations;
        }
    std::cout<<"CheckValues PASS: "<<observations<<" observations; exact results/order/refresh/PMO names/cache states; fallback allocations identical.\n";
}
