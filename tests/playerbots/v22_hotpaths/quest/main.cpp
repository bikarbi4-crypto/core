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
#include <random>
#include <cerrno>
#include "../allocations.h"

static std::vector<Api> apis() { return {v21_api(),v22_api()}; }
static std::vector<std::string> normalized(const Observation& o) { return o.trace; }
static unsigned checks=0;
static std::pair<int,int> Parse(Api api,const std::string& s,unsigned n) {
    try { return {api.parse(s,n),0}; }
    catch(const std::invalid_argument&) { return {0,1}; }
    catch(const std::out_of_range&) { return {0,2}; }
}
static void ParserContracts() {
    unsigned count=0;
    std::vector<std::string> inputs{"","0","reward","100,reward","{100,reward}","{100,}","{,0}","{+1,0}","{-1,3}","{+,0}","{-,0}","{{1,2},0}","{100,0,1}","{100,4}","{100,00}","{2147483647,0}","{2147483648,0}","{9999999999,3}","{00000000000,1}","{100, 1}","{1,1}x"};
    for(auto q:{0u,1u,100u,2147483647u,2147483648u,4294967295u})
        for(unsigned o=0;o<8;++o) inputs.push_back("{"+std::to_string(q)+","+std::to_string(o)+"}");
    std::mt19937 random(22);
    // Defined, balanced grammar only: malformed unmatched braces in the old
    // general parser can access past the input; V22 does not repair that code.
    for(unsigned i=0;i<25000;++i) {
        std::string q=std::to_string(random());
        if(i%5==0) q.insert(0,i%7,'0');
        if(i%11==0) q="-"+q;
        if(i%13==0) q="+"+q;
        std::string o=std::to_string(random()%8);
        if(i%17==0) o="reward";
        inputs.push_back(i%3?"{"+q+","+o+"}":q+","+o);
    }
    for(const auto& s:inputs) for(unsigned n=0;n<4;++n) {
        for(int initial:{0,EDOM,ERANGE}) {
            errno=initial;const auto before=Parse(v21_api(),s,n);const int beforeErrno=errno;
            errno=initial;const auto after=Parse(v22_api(),s,n);const int afterErrno=errno;
            if(before!=after || beforeErrno!=afterErrno) {
                std::cerr<<"Parser parity mismatch: "<<s<<" index="<<n<<" errno "<<beforeErrno<<" -> "<<afterErrno<<'\n';
                std::exit(2);
            }
            ++count;
        }
    }
    std::cout<<"parser result/exception parity: "<<count<<" inputs/positions/errno states passed\n";
}
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
                const bool once=true;
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
    ParserContracts();
    for(unsigned mode=0;mode<4;++mode){auto a=v21_api().edge(mode),b=v22_api().edge(mode);assert(a.result==b.result&&a.trace==b.trace);}
    for(unsigned objectives:{0u,1u,2u,4u})
    for(unsigned mask=0;mask<16;++mask)
    for(int kind=0;kind<3;++kind)
    for(int match:{-1,0,1}) {
        Case c; c.objectives=objectives; c.incompleteMask=mask; c.kind=kind; c.match=match;
        Check(c,5); c.refresh=true; Check(c,5);
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
            for(unsigned o=0;o<4;++o) assert(api.qualifier(q,o)==v21_api().qualifier(q,o));
    }
    for(unsigned n:{0u,1u,8u,64u}) { Case c; c.itemOnly=true; c.sharedCold=true; c.dropSize=n; Check(c); }
    for(unsigned n:{0u,1u,2u,4u}) for(auto api:apis()) {
        Case c; c.objectives=n;
        auto p=api.create(c); auto o=api.observe(p,0); api.destroy(p);
        std::cout<<"destination counts: "<<api.name<<" objectives="<<n<<" calls="<<o.destinations.size()<<" distances="<<o.distanceCalls<<'\n';
    }
    std::cout<<"quest parity: "<<checks<<" observations passed across V21 and V22; policy/reentry/exception/qualifier checks passed\n";
}
struct Bench { std::string name; Case c; };
static std::vector<Bench> Benchmarks() {
    std::vector<Bench> ret;
    for(unsigned n:{0u,1u,2u,4u}) for(int match:{-1,0,1}) {
        Case c; c.objectives=n; c.match=match;
        std::string label="quest-"+std::to_string(n)+"-"+(match<0?"miss":match==0?"first":"last");
        ret.push_back({label+"-warm",c}); c.refresh=true;
        ret.push_back({label+"-refresh",c});
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
static void OperationCounts() {
    std::cout<<"case,variant,allocations,bytes,lookup_gets,calculate_calls,quest_scans,destination_queries,distance_checks,rng_calls\n";
    for(auto bench:Benchmarks()) for(auto api:apis()) {
        auto p=api.create(bench.c);api.run(p,1);
#ifdef AUDIT_ALLOCATIONS
        Allocations::reset();Allocations::active=true;
        api.run(p,1);Allocations::active=false;
        std::cout<<bench.name<<','<<api.name<<','<<Allocations::allocations<<','<<Allocations::bytes<<",NA,NA,NA,NA,NA,0\n";
#else
        const auto o=api.observe(p,0);
        unsigned gets=0,calculates=0,quests=0;
        for(const auto& s:o.trace) { gets+=s.rfind("lookup:",0)==0;calculates+=s.rfind("calculate:",0)==0;quests+=s.rfind("template:",0)==0; }
        std::cout<<bench.name<<','<<api.name<<",NA,NA,"<<gets<<','<<calculates<<','<<quests<<','<<o.destinations.size()<<','<<o.distanceCalls<<','<<o.rngCalls<<'\n';
#endif
        api.destroy(p);
    }
}
static void ParserBenchmark() {
    assert(SetThreadAffinityMask(GetCurrentThread(),4));
    std::cout<<"input,position,variant,round,iterations,ns_per_call,allocations,bytes,checksum\n";
    for(const std::string s:{"{100,0}","{2147483647,3}","100", "{100,reward}","{+100,2}","{{100,1},2}","{100,00}","{2147483648,0}"})
    for(unsigned index:{0u,1u})for(unsigned round=0;round<9;++round)for(unsigned j=0;j<2;++j) {
        auto api=apis()[(j+round)%2];const unsigned count=1000;unsigned sum=0;
        Allocations::reset();Allocations::active=true;
        auto start=std::chrono::steady_clock::now();
        for(unsigned i=0;i<count;++i){auto v=Parse(api,s,index);sum+=unsigned(v.first)+unsigned(v.second);}
        auto ns=std::chrono::duration<double,std::nano>(std::chrono::steady_clock::now()-start).count()/count;
        Allocations::active=false;
        std::cout<<'"'<<s<<'"'<<','<<index<<','<<api.name<<','<<round<<','<<count<<','<<ns<<','<<double(Allocations::allocations)/count<<','<<double(Allocations::bytes)/count<<','<<sum<<'\n';
    }
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
            std::vector<unsigned> order{0,1}; std::rotate(order.begin(),order.begin()+round%2,order.end());
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
    if(argc>1 && std::string(argv[1])=="--parser-bench") ParserBenchmark();
    else if(argc>1 && std::string(argv[1])=="--operations") OperationCounts();
    else if(argc>1 && std::string(argv[1])=="--bench") Benchmark();
    else Contracts();
}
