#include "variants.h"
#include "scenarios.h"
inline volatile unsigned sink=0;
template<class F> double Time(F fn,unsigned iterations)
{
 for(unsigned i=0;i<500;++i)sink=static_cast<unsigned>(fn());
 auto start=std::chrono::steady_clock::now();unsigned result=0;
 for(unsigned i=0;i<iterations;++i)result+=static_cast<unsigned>(fn());
 auto end=std::chrono::steady_clock::now();sink=result;
 return std::chrono::duration<double,std::nano>(end-start).count()/iterations;
}
int main()
{
 std::cout<<"scenario,operation,round,v20_ns,v21_ns\n";
 for(auto scene:scenes)
 {
  baseline::Setup(scene.humans,scene.mixed,scene.mapBots);candidate::Setup(scene.humans,scene.mixed,scene.mapBots);
  baseline::sPlayerbotAIConfig.forceActiveWhenNearPlayer=candidate::sPlayerbotAIConfig.forceActiveWhenNearPlayer=true;
  auto run=[&](char const* name,auto b,auto c){for(unsigned r=0;r<9;++r){double x,y;
   if(r%2){y=Time(c,10000);x=Time(b,10000);}else{x=Time(b,10000);y=Time(c,10000);}
   std::cout<<scene.name<<','<<name<<','<<r<<','<<x<<','<<y<<'\n';}};
  run("priority",[]{return baseline::botAI.GetPriorityType();},[]{return candidate::botAI.GetPriorityType();});
  run("nearby",[]{return baseline::botAI.HasPlayerNearby();},[]{return candidate::botAI.HasPlayerNearby();});
  run("force",[]{return baseline::Force(&baseline::bot);},[]{return candidate::Force(&candidate::bot);});
 }
 // Include the new publisher cost: scalar movement publication (zone lookup is
 // a real terrain service, outside this isolated microbenchmark).
 PlayerActivityPresence index;auto token=index.Enter(1,{});
 for(unsigned r=0;r<9;++r)
 {
  unsigned i=0;double ns=Time([&]{index.Move(1,token,{0,5,++i%3,{}});return i;},100000);
  std::cout<<"human_position_publish,move,"<<r<<",0,"<<ns<<'\n';
 }
}
