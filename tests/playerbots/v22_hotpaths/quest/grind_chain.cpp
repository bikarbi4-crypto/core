#include "api.h"
#include "../allocations.h"
#include <algorithm>
#include <cassert>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <functional>
#include <iomanip>
#include <iostream>
#include <list>
#include <map>
#include <memory>
#include <random>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>
#define NOMINMAX
#include <windows.h>
extern Observation observation;
extern bool recording;
namespace grind {
inline std::function<bool(uint32_t)> needHook;
inline uint64_t candidateVisits=0;
#include "grind_fixture.inc"
}
struct Result {uint64_t guid=0,candidates=0,reads=0;std::vector<std::pair<uint32_t,uint32_t>> rng;std::vector<std::string> trace;};
struct Setup {
 Api api;void* quest;grind::FixtureWorld world;
 Setup(Api a,unsigned candidates,unsigned mode,bool pmo):api(a),world([&]{grind::Scenario s;s.targets=candidates;s.seed=73;s.members=mode==4?5:0;s.attackersFirst=mode==1;s.noTarget=mode==2;s.randomFilters=mode==5;return s;}()){
  Case c;c.monitor=pmo;c.match=mode==3?-1:0;c.quests=3;c.objectives=4;quest=api.create(c);
  grind::activeWorld=&world;
  if(mode==4 && candidates){auto& list=world.ai.context.GetValue<std::list<grind::ObjectGuid>>("possible targets")->value;list.push_front(list.front());list.push_back(list.back());}
 }
 ~Setup(){api.destroy(quest);grind::needHook={};}
 uint64_t Run(unsigned n){grind::activeWorld=&world;grind::needHook=[&](uint32_t entry){return api.need(quest,entry);};
  uint64_t result=0;grind::current::GrindTargetValue value(&world.ai);
  for(unsigned i=0;i<n;++i){world.rng=73;auto target=value.Calculate();result+=target?target->guid.raw:0;}return result;
 }
 Result Observe(){world.trace=true;world.counters={};world.randomTrace.clear();grind::candidateVisits=0;observation={};recording=true;
  auto result=Run(1);recording=false;return {result,grind::candidateVisits,world.counters.listReads,world.randomTrace,observation.trace};}
};
static volatile uint64_t sink=0;
static double Time(Setup& s,unsigned n){auto start=std::chrono::steady_clock::now();sink+=s.Run(n);return std::chrono::duration<double,std::nano>(std::chrono::steady_clock::now()-start).count()/n;}
int main(int argc,char** argv){std::string mode=argc>1?argv[1]:"test";auto apis=std::vector<Api>{v21_api(),v22_api()};unsigned checks=0;
 if(mode=="--bench"){assert(SetThreadAffinityMask(GetCurrentThread(),4));std::cout<<"candidates,mode,pmo,variant,round,iterations,ns_per_call\n";}
 if(mode=="--operations")std::cout<<"candidates,mode,pmo,variant,allocations,bytes,candidate_iterations,list_gets,quest_trace_events,target\n";
 for(unsigned candidates:{0u,1u,8u,64u,512u})for(unsigned scenario=0;scenario<6;++scenario)for(bool pmo:{false,true}){
  if(mode=="test"){Result base;for(auto api:apis){Setup s(api,candidates,scenario,pmo);auto o=s.Observe();if(api.name==apis[0].name)base=o;else assert(base.guid==o.guid&&base.rng==o.rng&&base.trace==o.trace&&base.reads==o.reads&&base.candidates==o.candidates);++checks;}}
  else if(mode=="--operations"){for(auto api:apis){Setup s(api,candidates,scenario,pmo);s.world.trace=false;s.Run(1);Allocations::reset();Allocations::active=true;auto result=s.Run(1);Allocations::active=false;auto a=Allocations::allocations,b=Allocations::bytes;auto o=s.Observe();
   std::cout<<candidates<<','<<scenario<<','<<pmo<<','<<api.name<<','<<a<<','<<b<<','<<o.candidates<<','<<o.reads<<','<<o.trace.size()<<','<<result<<'\n';}}
  else {
   // Rebind each disposable world's services between variants. Same seed and
   // iteration count, alternating order; warm qualified Values within a world.
   unsigned n=200;
   for(unsigned round=0;round<9;++round)for(unsigned j=0;j<2;++j){auto api=apis[(j+round)%2];Setup s(api,candidates,scenario,pmo);s.world.trace=false;s.Run(1);double ns=Time(s,n);
    std::cout<<candidates<<','<<scenario<<','<<pmo<<','<<api.name<<','<<round<<','<<n<<','<<std::fixed<<std::setprecision(3)<<ns<<'\n';}
  }
 }
 if(mode=="test")std::cout<<"PASS "<<checks<<" complete GrindTarget -> NeedForQuest -> NeedQuestObjective observations; target, duplicate candidate order, RNG draws and Value traces match\n";
}
