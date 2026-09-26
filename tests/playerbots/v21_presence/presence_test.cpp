#include "variants.h"
#include "lifecycle.h"
#include "scenarios.h"

int main()
{
 lifecycle::Test();unsigned observations=0;
 for(unsigned scene=0;scene<24;++scene)
 for(float activity:{0.f,49.f,50.f,69.f,70.f,79.f,80.f,90.f,99.f,100.f})
 for(unsigned type=1;type<10;++type)
 for(unsigned flags=0;flags<4;++flags)
 {
  baseline::Scenario(scene);candidate::Scenario(scene);
  baseline::env.activity=candidate::env.activity=activity;
  baseline::sPlayerbotAIConfig.forceActiveWhenNearPlayer=candidate::sPlayerbotAIConfig.forceActiveWhenNearPlayer=(flags&1)!=0;
  baseline::sPlayerbotAIConfig.limitCombatActivity=candidate::sPlayerbotAIConfig.limitCombatActivity=(flags&2)!=0;
  baseline::sPlayerbotAIConfig.botActiveAlone=candidate::sPlayerbotAIConfig.botActiveAlone=flags%2?10:100;
  baseline::PlayerbotAI b;candidate::PlayerbotAI c;
  auto bp=b.GetPriorityType();auto cp=c.GetPriorityType();
  if(unsigned(bp)!=unsigned(cp)){std::cerr<<"Parity mismatch scene "<<scene<<" "<<unsigned(bp)<<" "<<unsigned(cp)<<'\n';return 1;}
  assert(baseline::env.randomResults==candidate::env.randomResults);
  for(unsigned step=0;step<6;++step)
  {
   std::time_t const times[]={100,104,105,105,106,111};baseline::env.now=candidate::env.now=times[step];
   assert(b.AllowActivity(baseline::ActivityType(type),step==3)==c.AllowActivity(candidate::ActivityType(type),step==3));
   assert(baseline::env.randomResults==candidate::env.randomResults);
   for(unsigned k=0;k<10;++k){assert(b.allowActive[k]==c.allowActive[k]);assert(b.allowActiveCheckTimer[k]==c.allowActiveCheckTimer[k]);}
   ++observations;
  }
 }
 std::cout<<"Human parity: "<<observations<<" activity/cache observations, 24 priority scenarios, identical RNG\n";
 // Actual baseline/current nearby and Force bodies: strict float boundary, Z,
 // map/instance separation, visible GM vs GM mode, alternate camera.
 for(float x:{0.f,9.999f,10.f,10.001f,49.999f,50.f,55.f,60.f,1000.f})
 for(float z:{0.f,100.f})for(unsigned mapId:{0u,1u})for(unsigned instance:{5u,6u})for(unsigned gm=0;gm<4;++gm)
 {
  baseline::Setup(3);candidate::Setup(3);
  baseline::human.x=candidate::human.x=x;baseline::human.z=candidate::human.z=z;
  baseline::human.mapId=candidate::human.mapId=mapId;baseline::human.instance=candidate::human.instance=instance;
  baseline::human.gm=candidate::human.gm=(gm&1)!=0;baseline::human.gmFlag=candidate::human.gmFlag=(gm&2)!=0;
  candidate::Publish(candidate::human);
  baseline::sPlayerbotAIConfig.forceActiveWhenNearPlayer=candidate::sPlayerbotAIConfig.forceActiveWhenNearPlayer=true;
  baseline::PlayerbotAI b;candidate::PlayerbotAI c;
  assert(b.HasPlayerNearby(10)==c.HasPlayerNearby(10));assert(b.HasPlayerNearby()==c.HasPlayerNearby());
  assert(baseline::Force(&baseline::bot)==candidate::Force(&candidate::bot));
 }
 baseline::Setup(3);candidate::Setup(3);
 baseline::WorldObject bCamera;candidate::WorldObject cCamera;bCamera.x=cCamera.x=1;
 baseline::human.camera.body=&bCamera;candidate::human.camera.body=&cCamera;candidate::Publish(candidate::human);
 assert(baseline::botAI.HasPlayerNearby()==candidate::botAI.HasPlayerNearby());
 assert(candidate::botAI.HasPlayerNearby());
 // Intentional fixes have independent expected outcomes, not equality to bugs.
 baseline::Setup(0,523,3000);candidate::Setup(0,523,3000);
 assert(baseline::sRandomPlayerbotMgr.HasPlayers()&&!candidate::sRandomPlayerbotMgr.HasPlayers());
 assert(candidate::botAI.GetPriorityType()==candidate::ActivePiorityType::IN_EMPTY_SERVER);
 assert(baseline::botAI.GetPriorityType()==baseline::ActivePiorityType::IN_INACTIVE_MAP);
 baseline::bots[0].x=candidate::bots[0].x=1;
 assert(baseline::botAI.HasPlayerNearby()&&!candidate::botAI.HasPlayerNearby());
 assert(candidate::sRandomPlayerbotMgr.registry.size()==523);
 // Group with a legacy no-AI bot must not impersonate a human; human selfbot must.
 candidate::Setup(0);candidate::env.group=true;candidate::member.ai=nullptr;
 assert(candidate::botAI.GetPriorityType()==candidate::ActivePiorityType::IN_EMPTY_SERVER);
 candidate::member.client=true;candidate::member.ai=&candidate::memberAI;candidate::Publish(candidate::member);
 assert(candidate::botAI.GetPriorityType()==candidate::ActivePiorityType::IN_GROUP_WITH_REAL_PLAYER);
 // Disconnect while master/self body remains in world.
 candidate::Setup(0);candidate::env.master=true;candidate::env.self=true;
 assert(candidate::botAI.GetPriorityType()==candidate::ActivePiorityType::IN_EMPTY_SERVER);
 std::cout<<"Intentional corrections: mixed-only empty/nearby, legacy bot group, human selfbot group, disconnected master/self passed\n";
 std::ofstream out("operations.csv");
 out<<"scenario,variant,operation,copies,copied_entries,registry_entries,map_scans,map_entries,friend_checks,presence_entries\n";
 for(auto scene:scenes)
 {
  baseline::Setup(scene.humans,scene.mixed,scene.mapBots);candidate::Setup(scene.humans,scene.mixed,scene.mapBots);
  work={};baseline::botAI.GetPriorityType();Row(out,scene.name,"V20","priority",work);
  work={};candidate::botAI.GetPriorityType();Row(out,scene.name,"V21","priority",work);
  assert(work.copies==0&&work.mapScans==0&&work.friendChecks<2);
  work={};baseline::botAI.HasPlayerNearby();Row(out,scene.name,"V20","nearby",work);
  work={};candidate::botAI.HasPlayerNearby();Row(out,scene.name,"V21","nearby",work);
  assert(work.registryEntries==0);
  baseline::sPlayerbotAIConfig.forceActiveWhenNearPlayer=candidate::sPlayerbotAIConfig.forceActiveWhenNearPlayer=true;
  work={};baseline::Force(&baseline::bot);Row(out,scene.name,"V20","force",work);
  work={};candidate::Force(&candidate::bot);Row(out,scene.name,"V21","force",work);
 }
 std::cout<<"Operations recorded; no registry copies or full map scans in V21 activity consumers\n";
}
