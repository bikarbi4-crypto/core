#include "variants.h"
static volatile unsigned sink=0;
template<class F> double Run(F f,unsigned n)
{
    auto const start=std::chrono::steady_clock::now();
    unsigned result=0;
    for(unsigned i=0;i<n;++i) result+=f();
    sink=result;
    return std::chrono::duration<double,std::nano>(std::chrono::steady_clock::now()-start).count()/n;
}
int main()
{
    std::cout<<"scene,repeat,variant,ns_per_call\n";
    for(unsigned scene:{0u,10u,22u,26u})
    {
        baseline::Setup(scene); candidate::Setup(scene);
        baseline::env.tracing=candidate::env.tracing=false;
        if(scene>=22)
        {
            baseline::map.list.insert(baseline::map.list.begin(),2000,{&baseline::other,nullptr});
            candidate::map.list.insert(candidate::map.list.begin(),2000,{&candidate::other,nullptr});
        }
        baseline::PlayerbotAI b; candidate::PlayerbotAI c;
        unsigned const n=scene>=22 ? 30000 : 1000000;
        for(unsigned rep=0;rep<7;++rep)
        {
            // Alternate baseline/OFF order to reduce monotonic warmup drift.
            double base,off;
            if(rep%2) { off=Run([&]{return c.AllowActive(candidate::ALL_ACTIVITY);},n); base=Run([&]{return b.AllowActive(baseline::ALL_ACTIVITY);},n); }
            else { base=Run([&]{return b.AllowActive(baseline::ALL_ACTIVITY);},n); off=Run([&]{return c.AllowActive(candidate::ALL_ACTIVITY);},n); }
            std::cout<<scene<<','<<rep<<",v20,"<<base<<'\n'<<scene<<','<<rep<<",diagnostic_off,"<<off<<'\n';
            for(bool timing:{false,true})
            {
                PresenceDiagnostics::Start(3600,timing,false);
                auto const on=Run([&]{return c.AllowActive(candidate::ALL_ACTIVITY);},n);
                PresenceDiagnostics::Stop("benchmark");
                std::cout<<scene<<','<<rep<<','<<(timing ? "diagnostic_timing" : "diagnostic_on")<<','<<on<<'\n';
            }
        }
    }
}
