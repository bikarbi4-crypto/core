#include AUDIT_HEADER
namespace AUDIT_NAMESPACE {
struct TestTrigger : Trigger {
 bool active=false,nested=false,throwActive=false,throwName=false;unsigned serial=0;
 TestTrigger(const std::string& n):Trigger(nullptr,n,4){}
 bool IsActive() override {Trace("IsActive");++serial;if(throwActive)throw std::runtime_error("active");return active;}
 std::string getName() override {Trace("getName");if(throwName)throw std::runtime_error("name");if(nested){nested=false;auto event=Check();assert(!event==name.empty());}return name;}
};
struct State {
 Case c;MovementAction movement;TravelPath path;WorldPosition from,to,dest;
 MoveToTravelTargetAction travel;std::string source,param;std::unique_ptr<TestTrigger> trigger;Player owner;WorldPacket packet;
 State(Case in):c(in),source(in.size,'s'),param(in.size,'p'),trigger(new TestTrigger(source)) {
  std::vector<PathNodePoint> points(in.size);for(unsigned i=0;i<points.size();++i)points[i].point.x=float(i);path.addPath(points);
  for(unsigned i=0;i<in.size;++i)travel.target.travelConditions.push_back(std::string(40,'c'));
  if(in.size && in.mode%4==1)travel.target.travelConditions.front()="should travel named::guild order";
  if(in.size && in.mode%4==2)travel.target.travelConditions.back()="should travel named::guild order";
  if(in.size>2 && in.mode%4==3)travel.target.travelConditions[1]=travel.target.travelConditions[0];
  packet.SetOpcode(17);packet << ObjectGuid{0x123456789abcdef};
 }
};
void Prepare(State& s){config={};config.follow=true;
 if(s.c.mode&4)config.group=true;
 if(s.c.mode&8)config.leader=false;
 if(s.c.mode&16){s.travel.target.pos.mapId=100;config.near=false;}
 if(s.c.mode&32){config.loot=true;config.lootPossible=true;}
 if(s.c.mode&64)config.free=false;
 s.travel.target.forced=s.c.mode&128;
}
__declspec(noinline) std::uint64_t Run(void* p,unsigned n){
 auto& s=*static_cast<State*>(p);std::uint64_t result=0;Prepare(s);
 for(unsigned i=0;i<n;++i){
  if(s.c.kind==0){if(s.c.mode&1)s.to.x=float(i%2);result+=s.movement.FlyDirect(s.from,s.to,s.dest,s.path,false);}
  else if(s.c.kind==1){result+=s.travel.isUseful();}
  else if(s.c.kind==2){
   switch(s.c.mode%4){
    case 0:{Event e(s.source);result+=!e;break;}
    case 1:{Event e(s.source,s.param,&s.owner);result+=!e;break;}
    case 2:{Event e(s.source,s.packet,&s.owner);result+=!e;break;}
    default:{Event e(s.source,ObjectGuid{42},&s.owner);result+=!e;break;}
   }
  }else{
   s.trigger->Reset();s.trigger->active=s.c.mode==1;
   if(s.c.mode>=2)s.trigger->ExternalEventForce(s.param,s.c.mode==3?&s.owner:nullptr);
   Event e=s.trigger->Check();result+=!e;
  }
 }return result;
}
void* Create(Case c){record=false;return new State(c);}
void Destroy(void* p){record=false;delete static_cast<State*>(p);}
Observation Observe(void* p){
 auto& s=*static_cast<State*>(p);auto before=s.path.getPath();observation={};record=true;
 observation.result=Run(p,1);record=false;
 assert(before==s.path.getPath());
 for(const auto& t:observation.trace){observation.gets+=t.rfind("Get:",0)==0;observation.checks+=t=="IsActive";}
 return observation;
}
void Contracts(){
 Player owner;std::string source(128,'a'),param(256,'b');WorldPacket packet;packet.SetOpcode(7);packet<<ObjectGuid{42};
 Event e(source,param,&owner);assert(e.getSource()==source&&e.getParam()==param&&e.getOwner()==&owner);
 assert(source==std::string(128,'a')&&param==std::string(256,'b'));
 Event copy(e);copy=copy;assert(copy.getSource()==source&&copy.getParam()==param);
 copy=Event("guid",ObjectGuid{42},&owner);assert(copy.getObject()==ObjectGuid{42});
 Event fromPacket(source,packet,&owner);assert(fromPacket.getObject()==ObjectGuid{42}&&packet.GetOpcode()==7&&packet.size()==8);
 copy=fromPacket;assert(copy.getObject()==ObjectGuid{42}&&copy.getPacket().GetOpcode()==7);
 Event alias(copy.getSource(),copy.getParam(),copy.getOwner());assert(alias.getSource()==source);
 TestTrigger trigger(source);trigger.active=true;trigger.nested=true;auto first=trigger.Check();assert(first.getSource()==source&&trigger.serial==1);
 trigger.Reset();trigger.throwActive=true;try{trigger.Check();assert(false);}catch(const std::runtime_error& e){assert(std::string(e.what())=="active");}
 assert(!trigger.IsAlreadyTriggered());trigger.throwActive=false;trigger.throwName=true;
 try{trigger.Check();assert(false);}catch(const std::runtime_error& e){assert(std::string(e.what())=="name");}
 assert(trigger.IsAlreadyTriggered());trigger.throwName=false;assert(trigger.Check().getSource()==source);
 for(unsigned step=0;step<30;++step){now=10000+step;bool check=trigger.needCheck();if(step==0)assert(!check);}
}
}
Api AUDIT_API(){return{AUDIT_LABEL,AUDIT_NAMESPACE::Create,AUDIT_NAMESPACE::Destroy,AUDIT_NAMESPACE::Run,AUDIT_NAMESPACE::Observe,AUDIT_NAMESPACE::Contracts};}
