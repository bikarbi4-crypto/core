#include "api.h"
#include <algorithm>
#include <cassert>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <numeric>
#include <set>
#include <tuple>
#define NOMINMAX
#include <windows.h>

static std::vector<Api> apis() { return {v171_api(),a_only_api(),b_only_api(),c_only_api(),v18_api()}; }
static std::vector<std::string> normalized(const Observation& o) {
    std::vector<std::string> ret; std::set<std::string> seen;
    for(const auto& s:o.trace) {
        if(s.rfind("dest:",0)==0 && !seen.insert(s).second) continue;
        ret.push_back(s);
    }
    return ret;
}
static unsigned checks=0;
static void Check(const Case& c,unsigned steps=4) {
    std::vector<Observation> control;
    for(auto api:apis()) {
        void* state=api.create(c);
        for(unsigned step=0;step<steps;++step) {
            auto o=api.observe(state,step);
            if(control.size()<=step) control.push_back(o);
            else {
                const auto& base=control[step];
                assert(base.result==o.result && base.rngCalls==o.rngCalls);
                if(normalized(base)!=normalized(o)) {
                    std::cerr<<"Trace differs "<<api.name<<" step "<<step<<"\n";
                    auto a=normalized(base),b=normalized(o);
                    for(unsigned i=0;i<std::max(a.size(),b.size());++i)
                        if(i>=a.size() || i>=b.size() || a[i]!=b[i]) std::cerr<<i<<" "<<(i<a.size()?a[i]:"END")<<" | "<<(i<b.size()?b[i]:"END")<<"\n";
                    std::abort();
                }
                const bool once=std::string(api.name)=="a_only" || std::string(api.name)=="v18";
                if(once) {
                    std::set<uint32_t> unique(o.destinations.begin(),o.destinations.end());
                    assert(unique.size()==o.destinations.size());
                    assert(o.destinations.size()<=base.destinations.size());
                    assert(o.distanceCalls<=base.distanceCalls);
                } else assert(o.destinations==base.destinations && o.distanceCalls==base.distanceCalls);
            }
            // Independent result oracle: only needed objectives can enable a
            // destination match; empty vendor lists intentionally mean true.
            bool need=c.objectives && (c.incompleteMask&((1u<<c.objectives)-1));
            if(c.kind==2) need=need && (c.dropSize || !c.vendorSize || !c.prototype || c.money>100);
            bool expected=c.itemOnly?(c.dropSize || !c.vendorSize || !c.prototype || c.money>100):
                c.state==0 && need && c.destinations && c.match>=0 && !c.unreachable;
            if(step<4) assert(o.result==expected);
            ++checks;
        }
        api.destroy(state);
    }
}
static void Contracts() {
    for(unsigned objectives:{0u,1u,2u,4u})
    for(unsigned mask=0;mask<16;++mask)
    for(int kind=0;kind<3;++kind)
    for(int match:{-1,0,1}) {
        Case c; c.objectives=objectives; c.incompleteMask=mask; c.kind=kind; c.match=match;
        Check(c,5);
    }
    for(unsigned quests:{1u,3u,20u})
    for(int state=0;state<5;++state)
    for(unsigned dests:{0u,1u,8u,64u})
    for(int match:{-1,0,1})
    for(bool pmo:{false,true}) {
        Case c; c.quests=quests; c.state=state; c.destinations=dests; c.match=match; c.monitor=pmo;
        Check(c);
        c.unreachable=true; Check(c);
    }
    for(unsigned drops:{0u,1u,8u,64u})
    for(unsigned vendors:{0u,1u,8u,64u})
    for(unsigned money:{0u,99u,100u,101u})
    for(bool proto:{false,true}) {
        Case c; c.itemOnly=true; c.dropSize=drops; c.vendorSize=vendors; c.money=money; c.prototype=proto;
        Check(c); c.itemOnly=false; c.kind=2; c.match=1; Check(c);
    }
    for(auto api:apis()) {
        api.policy();
        for(uint32_t q:{0u,1u,9u,100u,999999u,2147483647u,2147483648u,4294967295u})
            for(unsigned o=0;o<4;++o) assert(api.qualifier(q,o)==v171_api().qualifier(q,o));
    }
    for(unsigned n:{0u,1u,8u,64u}) { Case c; c.itemOnly=true; c.sharedCold=true; c.dropSize=n; Check(c); }
    for(unsigned n:{0u,1u,2u,4u}) for(auto api:apis()) {
        Case c; c.objectives=n;
        auto p=api.create(c); auto o=api.observe(p,0); api.destroy(p);
        std::cout<<"destination counts: "<<api.name<<" objectives="<<n<<" calls="<<o.destinations.size()<<" distances="<<o.distanceCalls<<'\n';
    }
    std::cout<<"quest parity: "<<checks<<" observations passed across V17.1, A, B, C and V18; policy/reentry/exception/qualifier checks passed\n";
}
struct Bench { std::string name; Case c; };
static std::vector<Bench> Benchmarks() {
    std::vector<Bench> ret;
    for(unsigned n:{0u,1u,2u,4u}) for(int match:{-1,0,1}) {
        Case c; c.objectives=n; c.match=match;
        ret.push_back({"quest-"+std::to_string(n)+"-"+(match<0?"miss":match==0?"first":"last"),c});
    }
    for(unsigned quests:{3u,20u}) { Case c; c.quests=quests; ret.push_back({"quests-"+std::to_string(quests)+"-miss",c}); }
    for(bool refresh:{false,true}) {
        Case c; c.kind=2; c.refresh=refresh; ret.push_back({refresh?"item-objectives-refresh":"item-objectives-warm",c});
    }
    for(unsigned n:{0u,1u,8u,64u}) for(bool vendor:{false,true}) {
        Case c; c.itemOnly=true; c.dropSize=vendor?0:n; c.vendorSize=vendor?n:0;
        ret.push_back({std::string(vendor?"vendor-":"drop-")+std::to_string(n),c});
    }
    for(unsigned n:{0u,1u,8u,64u}) { Case c; c.itemOnly=true; c.sharedCold=true; c.dropSize=n; ret.push_back({"drop-reset-"+std::to_string(n),c}); }
    return ret;
}
static volatile std::uint64_t sink=0;
static double Time(Api api,void* state,uint64_t n) {
    auto start=std::chrono::steady_clock::now(); sink+=api.run(state,n);
    return std::chrono::duration<double,std::nano>(std::chrono::steady_clock::now()-start).count()/double(n);
}
static void Benchmark() {
    assert(SetProcessAffinityMask(GetCurrentProcess(),4));
    std::cout<<"case,variant,round,iterations,ns_per_call,checksum\n";
    auto variants=apis();
    for(auto bench:Benchmarks()) {
        std::vector<uint64_t> counts(variants.size(),1);
        for(unsigned v=0;v<variants.size();++v) {
            auto p=variants[v].create(bench.c); variants[v].run(p,1);
            double ns=Time(variants[v],p,128); counts[v]=std::max<uint64_t>(8,static_cast<uint64_t>(12000000/std::max(1.0,ns)));
            variants[v].destroy(p);
        }
        // Same iteration count/work for all variants, rotating/reversing order.
        auto n=*std::max_element(counts.begin(),counts.end());
        for(unsigned round=0;round<9;++round) {
            std::vector<unsigned> order{0,1,2,3,4}; std::rotate(order.begin(),order.begin()+round%5,order.end());
            if(round%2) std::reverse(order.begin(),order.end());
            for(auto v:order) {
                auto api=variants[v]; auto p=api.create(bench.c); api.run(p,1);
                double ns=Time(api,p,n); api.destroy(p);
                std::cout<<bench.name<<','<<api.name<<','<<round<<','<<n<<','<<std::fixed<<std::setprecision(3)<<ns<<','<<sink<<'\n';
            }
        }
    }
}
int main(int argc,char** argv) {
    if(argc>1 && std::string(argv[1])=="--bench") Benchmark();
    else Contracts();
}
