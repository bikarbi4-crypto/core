#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cstdint>
struct AuditCounters { std::uint64_t passes=0,candidateVisits=0,survivingVisits=0; };
static AuditCounters auditCounters;
#include "../grind_fixture.h"
static volatile uint64 sink=0;
struct Case { unsigned members,raw; const char* mode; };
void Prepare(FixtureWorld& world,const Case& c) {
    if(std::string(c.mode).rfind("filtered-",0)==0) {
        auto survivors=unsigned(std::stoul(std::string(c.mode).substr(9)));
        for(unsigned i=0;i<world.units.size();++i) world.units[i]->valid=i<survivors;
    }
}
struct Outcome {
    uint64 target;
    AuditCounters audit;
    FixtureCounters fixture;
    std::vector<std::string> messages;
    std::vector<std::pair<uint32,uint32>> rng;
};
template<class Value> Outcome Count(FixtureWorld& w) {
    activeWorld=&w; w.trace=true; w.rng=w.scenario.seed; w.counters={}; auditCounters={};
    w.messages.clear(); w.randomTrace.clear();
    Value value(&w.ai); auto* target=value.Calculate();
    return {target?target->guid.raw:0,auditCounters,w.counters,w.messages,w.randomTrace};
}
template<class Value> double Measure(FixtureWorld& w,unsigned iterations) {
    activeWorld=&w; w.trace=false; Value value(&w.ai); uint64 sum=0;
    const auto begin=std::chrono::steady_clock::now();
    for(unsigned i=0;i<iterations;++i) { w.rng=w.scenario.seed; if(auto* target=value.Calculate()) sum+=target->guid.raw; }
    const auto end=std::chrono::steady_clock::now(); sink=sum;
    return std::chrono::duration<double,std::nano>(end-begin).count()/iterations;
}
int main() {
    if(!SetThreadAffinityMask(GetCurrentThread(),DWORD_PTR{4})) return 2;
    std::vector<Case> cases;
    for(unsigned members:{0u,2u,5u,10u,40u}) {
        for(unsigned raw:{0u,1u,2u,3u,4u,16u,64u}) {
            cases.push_back({members,raw,"found"});
            if(raw>=3) for(const char* mode:{"filtered-0","filtered-1","filtered-2"}) cases.push_back({members,raw,mode});
        }
        for(unsigned raw:{0u,2u,16u}) cases.push_back({members,raw,"failed-assist"});
        cases.push_back({members,64u,"early-attacker"});
    }
    const char* names[]={"v16","c_only","ab_only","v17","a_only"};
    using MeasureFn=double(*)(FixtureWorld&,unsigned);
    MeasureFn measures[]={Measure<v16::GrindTargetValue>,Measure<c_only::GrindTargetValue>,Measure<ab_only::GrindTargetValue>,Measure<v17::GrindTargetValue>,Measure<a_only::GrindTargetValue>};
    using CountFn=Outcome(*)(FixtureWorld&);
    CountFn counts[]={Count<v16_count::GrindTargetValue>,Count<c_only_count::GrindTargetValue>,Count<ab_only_count::GrindTargetValue>,Count<v17_count::GrindTargetValue>,Count<a_only_count::GrindTargetValue>};
    std::cout<<"case,mode,members,raw_candidates,variant,pass,iterations,ns_per_calculate,assist_passes,candidate_visits,surviving_visits,player_lookups,current_target_reads,list_reads,target,checksum\n";
    for(std::size_t id=0;id<cases.size();++id) {
        const auto& c=cases[id];
        Scenario s; s.seed=17; s.members=c.members; s.targets=c.raw; s.mixedPlayers=true; s.debug=true;
        s.allDead=std::string(c.mode)=="failed-assist"; s.attackersFirst=std::string(c.mode)=="early-attacker";
        std::vector<std::unique_ptr<FixtureWorld>> worlds;
        std::vector<Outcome> outcomes;
        for(unsigned j=0;j<5;++j) {
            worlds.push_back(std::make_unique<FixtureWorld>(s)); Prepare(*worlds[j],c); outcomes.push_back(counts[j](*worlds[j]));
            const auto& a=outcomes[0]; const auto& b=outcomes[j];
            assert(a.target==b.target && a.messages==b.messages && a.rng==b.rng);
            assert(a.audit.passes==b.audit.passes && a.audit.candidateVisits==b.audit.candidateVisits && a.audit.survivingVisits==b.audit.survivingVisits);
            assert(a.fixture.listReads==b.fixture.listReads && a.fixture.distances==b.fixture.distances);
            // Debug parity was tested above; production debug is off during timing.
            worlds[j]->ai.debug=false;
        }
        double estimate=measures[0](*worlds[0],50);
        unsigned iterations=unsigned(std::clamp(12000000.0/std::max(estimate,1.0),50.0,200000.0));
        for(unsigned pass=0;pass<9;++pass) for(unsigned offset=0;offset<5;++offset) {
            unsigned j=pass%2?(pass+5-offset)%5:(pass+offset)%5;
            double ns=measures[j](*worlds[j],iterations);
            const auto& r=outcomes[j];
            if(pass) std::cout<<id<<','<<c.mode<<','<<c.members<<','<<c.raw<<','<<names[j]<<','<<pass<<','<<iterations<<','<<std::fixed<<std::setprecision(3)<<ns<<','
                <<r.audit.passes<<','<<r.audit.candidateVisits<<','<<r.audit.survivingVisits<<','<<r.fixture.playerLookups<<','<<r.fixture.currentTargetReads<<','<<r.fixture.listReads<<','<<r.target<<','<<sink<<'\n';
        }
        if(id%10==0) std::cerr<<"completed "<<id+1<<'/'<<cases.size()<<" cases\n";
    }
}
