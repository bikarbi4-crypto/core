#include "api.h"
#include "inventory.h"
#include <algorithm>
#include <cassert>
#include <chrono>
#include <cstring>
#include <fstream>
#include <iostream>
#include <vector>
#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#endif

int main(int argc,char** argv)
{
    if (argc>1 && !strcmp(argv[1],"--repro"))
    {
        auto stale=v18_api.scenario(50,true,1,0,0);
        auto same=v18_api.scenario(51,true,1,0,0);
        auto late=v18_api.scenario(49,true,33,123,0);
        assert(stale.stack.size()==1 && same.result==0 && late.result==0);
        std::cout<<"V18 defects reproduced: stale stack="<<stale.stack.size()<<", surviving duplicate parent="<<same.result<<", late map operation="<<late.result<<"\n";
        return 0;
    }
#ifdef PMO_TRACE
    unsigned observations=0;
    for (bool on:{false,true})
        for (unsigned map:{0u,1u,33u,609u})
            for (unsigned instance:{0u,123u})
                for (unsigned flags:{0u,32u,64u,96u,128u,256u})
                    for (int test=0;test<scopeCount+2;++test)
                    {
                        auto a=v18_api.scenario(test,on,map,instance,flags);
                        auto b=v19_api.scenario(test,on,map,instance,flags);
                        assert(a.result==b.result && a.events==b.events && a.stats==b.stats && a.stack==b.stack);
                        assert(a.counters.work==b.counters.work);
                        if (!on)
                        {
                            assert(b.counters.starts==0 && b.counters.operations==0 && b.counters.clocks==0);
                            assert(b.counters.lookups==0 && b.counters.positions==0 && b.counters.actionNames==0 && b.counters.eventSources==0);
                            assert(b.maps==0 && b.stats.empty() && b.stack.empty());
                        }
                        ++observations;
                    }
    // Full ExecuteAction branch/early-return/reset/scope order parity.
    for (unsigned flags=0;flags<256;++flags)
        for (bool on:{false,true})
        {
            auto a=v18_api.scenario(scopeCount,on,33,123,flags);
            auto b=v19_api.scenario(scopeCount,on,33,123,flags);
            assert(a.result==b.result && a.events==b.events && a.stats==b.stats && a.stack==b.stack);
            ++observations;
        }
    for (int test:{53,54,55,56,59,60})
    {
        auto a=v18_api.scenario(test,true,1,0,0), b=v19_api.scenario(test,true,1,0,0);
        assert(a.events==b.events && a.stats==b.stats && a.stack==b.stack);
        ++observations;
    }
    for (unsigned flags:{0u,1u,2u,66u})
    {
        int test=(flags&2) ? 32 : 48;
        auto a=v18_api.scenario(test,true,1,0,flags), b=v19_api.scenario(test,true,1,0,flags);
        assert(a.result==b.result && a.events==b.events && a.stats==b.stats && a.stack==b.stack);
        ++observations;
    }
    auto types=v19_api.scenario(61,true,33,123,0);
    assert(types.stack.empty() && types.stats.size()==5);
    for(const auto& s:types.stats) assert(s.substr(s.find('=')+1)=="0,0,0,0");
    ++observations;
    for (unsigned map:{0u,1u,33u})
        for (unsigned instance:{0u,123u})
        {
            auto late=v19_api.scenario(49,true,map,instance,0);
            assert(late.result==1 && late.stack.empty() && late.stats.size()==1);
            auto stale=v19_api.scenario(50,true,map,instance,0);
            assert(stale.stack.empty());
            assert(stale.events.back()=="next body|next");
            auto same=v19_api.scenario(51,true,map,instance,0);
            assert(same.result==1 && same.stack.empty() && same.events.back()=="sibling body|same|sibling");
            auto nested=v19_api.scenario(52,true,map,instance,0);
            assert(nested.result==1 && nested.stack.empty());
            auto instances=v19_api.scenario(58,true,map,instance,0);
            assert(instances.result==4 && instances.stats.size()==4);
            for (unsigned f:{0u,1u}) assert(v19_api.scenario(57,true,map,instance,f).stack.empty());
            observations+=7;
        }
    v19_api.registry_threads();
    std::cout<<"PMO PASS: "<<observations<<" parity/toggle observations; "<<scopeCount<<" callsites; OFF counters zero; 512 concurrent map/instance registrations.\n";
#else
    if(argc!=3 || strcmp(argv[1],"--benchmark")) return 2;
#ifdef _WIN32
    assert(SetProcessAffinityMask(GetCurrentProcess(),4));
#endif
    std::ofstream out(argv[2]);
    out<<"case,enabled,map,instance,flags,round,variant,iterations,ns_per_call,checksum\n";
    // Exact IDs come from the generated inventory (recorded alongside CSV).
    std::vector<int> selected={0,1,2,3,4,25,26,32,34,38,40,41,42,44,45,46,47,48};
    for (int test:selected)
        for (bool on:{false,true})
            for (unsigned map:{1u,33u})
            {
                // Equal work counts in both variants. At least ~12ms even for
                // the faster variant; short OFF batches exaggerate noise.
                uint64_t n=on ? 30000 : 200000;
                for (auto api:{&v18_api,&v19_api})
                {
                    auto begin=std::chrono::steady_clock::now();
                    api->batch(test,on,map,123,0,n);
                    double elapsed=std::chrono::duration<double>(std::chrono::steady_clock::now()-begin).count();
                    if(elapsed<0.012) n=std::min<uint64_t>(20000000,uint64_t(n*0.015/std::max(elapsed,0.000001)));
                }
                for (int round=0;round<11;++round)
                {
                    const PmoApi* variants[2]={&v18_api,&v19_api};
                    if(round%2) std::swap(variants[0],variants[1]);
                    uint64_t prior=0;
                    for (auto api:variants)
                    {
                        auto start=std::chrono::steady_clock::now();
                        auto sum=api->batch(test,on,map,123,0,n);
                        auto stop=std::chrono::steady_clock::now();
                        if (prior) assert(prior==sum); prior=sum;
                        double ns=std::chrono::duration<double,std::nano>(stop-start).count()/n;
                        out<<test<<','<<on<<','<<map<<",123,0,"<<round<<','<<api->label<<','<<n<<','<<ns<<','<<sum<<'\n';
                    }
                }
            }
    std::cout<<"PMO benchmark complete; all paired work checksums match.\n";
#endif
}
