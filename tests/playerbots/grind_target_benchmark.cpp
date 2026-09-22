#include "grind_fixture.h"
static volatile uint64 benchmarkSink=0;
template<class Value> static double Measure(FixtureWorld& world,unsigned iterations) {
    activeWorld=&world; world.trace=false; Value value(&world.ai);
    uint64 sum=0;
    auto start=std::chrono::steady_clock::now();
    for(unsigned i=0;i<iterations;++i) { world.rng=world.scenario.seed; if(Unit* target=value.Calculate()) sum+=target->guid.raw; }
    auto end=std::chrono::steady_clock::now(); benchmarkSink=sum;
    return std::chrono::duration<double,std::nano>(end-start).count()/iterations;
}
int main() {
    std::cout << "case,members,candidates,v16_ns,a_ns,v17_ns,a_ratio,v17_ratio,player_lookups_v16,player_lookups_v17,current_target_reads_v16,current_target_reads_v17\n";
    for(const char* mode : {"found","none","all-dead","attacker"})
        for(unsigned members : {0u,5u,40u})
            for(unsigned targets : {0u,1u,2u,4u,16u,64u}) {
                Scenario s; s.seed=17; s.members=members; s.targets=targets; s.mixedPlayers=true;
                s.noTarget=std::string(mode)=="none"; s.allDead=std::string(mode)=="all-dead"; s.attackersFirst=std::string(mode)=="attacker";
                FixtureWorld old(s),a(s),all(s);
                std::vector<double> ot,at,nt;
                // Each timed batch lasts roughly 40ms or more; tiny fixed batches
                // are distorted by timer granularity and CPU frequency changes.
                double estimate=Measure<v16::GrindTargetValue>(old,50);
                const unsigned iterations=static_cast<unsigned>(std::clamp(40000000.0/(std::max)(estimate,1.0),100.0,300000.0));
                old.counters={}; a.counters={}; all.counters={};
                for(unsigned pass=0;pass<7;++pass) {
                    double x,y,z;
                    if(pass%2) {z=Measure<v17::GrindTargetValue>(all,iterations); y=Measure<v17a::GrindTargetValue>(a,iterations); x=Measure<v16::GrindTargetValue>(old,iterations);}
                    else {x=Measure<v16::GrindTargetValue>(old,iterations); y=Measure<v17a::GrindTargetValue>(a,iterations); z=Measure<v17::GrindTargetValue>(all,iterations);}
                    if(pass) {ot.push_back(x);at.push_back(y);nt.push_back(z);}
                }
                std::sort(ot.begin(),ot.end()); std::sort(at.begin(),at.end()); std::sort(nt.begin(),nt.end());
                double x=(ot[2]+ot[3])/2,y=(at[2]+at[3])/2,z=(nt[2]+nt[3])/2;
                const uint64 calls=static_cast<uint64>(iterations)*7;
                std::cout << mode << ',' << members << ',' << targets << ',' << std::fixed << std::setprecision(2) << x << ',' << y << ',' << z << ',' << std::setprecision(4) << y/x << ',' << z/x << ',' << old.counters.playerLookups/calls << ',' << all.counters.playerLookups/calls << ',' << old.counters.currentTargetReads/calls << ',' << all.counters.currentTargetReads/calls << '\n';
            }
}
