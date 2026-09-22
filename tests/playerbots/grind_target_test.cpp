#include "grind_fixture.h"

struct Outcome {
    uint64 target=0;
    std::vector<std::string> messages;
    std::vector<std::pair<uint32,uint32>> random;
    FixtureCounters counters;
};
template<class Value> static Outcome Run(Scenario scenario) {
    FixtureWorld world(scenario); activeWorld=&world;
    Value value(&world.ai);
    Unit* target=value.Calculate();
    return {target?target->guid.raw:0,world.messages,world.randomTrace,world.counters};
}
static void Compare(Scenario scenario) {
    const auto old=Run<v16::GrindTargetValue>(scenario);
    const auto a=Run<v17a::GrindTargetValue>(scenario);
    const auto all=Run<v17::GrindTargetValue>(scenario);
    for(const auto* actual : {&a,&all}) {
        assert(old.target==actual->target);
        assert(old.messages==actual->messages);
        assert(old.random==actual->random);
        assert(old.counters.listReads==actual->counters.listReads);
        assert(old.counters.travelReads==actual->counters.travelReads);
        assert(old.counters.distances==actual->counters.distances);
    }
}

int main() {
    unsigned scenarios=0;
    for(unsigned members : {0u,1u,2u,5u,10u,40u})
        for(unsigned targets : {0u,1u,8u,64u})
            for(unsigned mode=0;mode<8;++mode) {
                Scenario s; s.seed=mode+1; s.members=members; s.targets=targets; s.debug=true;
                s.noTarget=mode==1; s.allDead=mode==2; s.attackersFirst=mode==3;
                s.mixedPlayers=mode==4; s.duplicateGuidPointers=mode==4;
                s.battleground=mode==5 || mode==6; s.av=mode==6;
                s.randomFilters=mode==7; s.travelWorking=mode==7;
                Compare(s); ++scenarios;
            }
    for(unsigned seed=1;seed<=2000;++seed) {
        Scenario s; s.seed=seed; s.members=seed%13; s.targets=seed%25;
        s.randomFilters=true; s.mixedPlayers=true; s.debug=seed%7==0;
        s.duplicateGuidPointers=seed%5==0; s.travelWorking=seed%3==0; s.battleground=seed%11==0;
        Compare(s); ++scenarios;
    }
    Scenario changing; changing.members=5; changing.targets=16; changing.allDead=true; changing.changingValues=true; changing.debug=true;
    Compare(changing); ++scenarios;
    // A fresh Calculate must observe changed selections; no snapshot survives it.
    FixtureWorld world({1,5,16}); activeWorld=&world;
    v17::GrindTargetValue value(&world.ai);
    Unit* first=value.Calculate();
    world.ai.context.Set<std::list<ObjectGuid>>("possible targets",{});
    assert(first && value.Calculate()==nullptr);
    std::cout << "PASS: " << scenarios << " deterministic production V16/A/V17 scenarios; target, random draws, debug messages, refresh reads and distance ordering match.\n";
}
