// This file is intentionally included separately in v22 and v23 namespaces.
using uint8=uint8_t;using int8=int8_t;using uint32=uint32_t;using int32=int32_t;using uint64=uint64_t;
enum class BotState {BOT_STATE_NON_COMBAT,BOT_STATE_COMBAT};
enum class BotTypeNumber {DUMMY_ATTACK_NUMBER};
constexpr int ALL_ACTIVITY=1,UNIT_FIELD_FLAGS=2,UNIT_FLAG_CONFUSED=4,BATTLEGROUND_AV=1;
constexpr int CONTROLLED_PET=1,CONTROLLED_GUARDIANS=2,CONTROLLED_CHARM=4,CONTROLLED_MINIPET=8,CONTROLLED_TOTEMS=16;
constexpr int PERF_MON_VALUE=1;
struct ObjectGuid {
 uint64 value=0;ObjectGuid()=default;ObjectGuid(uint64 value):value(value){}
 bool IsPet() const{return (value>>63)!=0;}
 bool operator<(ObjectGuid x)const{return value<x.value;}bool operator==(ObjectGuid x)const{return value==x.value;}bool operator!=(ObjectGuid x)const{return value!=x.value;}
};
inline std::ostream& operator<<(std::ostream& s,ObjectGuid id){return s<<id.value;}
struct Unit;struct Player;struct Creature;struct Pet;struct Group;struct PlayerbotAI;struct Context;struct State;
inline State* state=nullptr;
void Mark(const char*,unsigned=0,const std::string* =nullptr);
void Count(unsigned);
struct Counters {enum{lookups,gets,calculations,iterations,validations,rng,distances,objects,clock,pmo,count};};
struct Config {
 bool perfMonEnabled=false,shareTargets=false;float sightDistance=60,reactDistance=50;bool prohibited=false;
 bool IsInPvpProhibitedZone(uint32){return prohibited;}
};
inline Config sPlayerbotAIConfig;
time_t AuditTime(time_t*);
struct PerformanceMonitorOperation {std::string name;explicit PerformanceMonitorOperation(std::string n):name(std::move(n)){}~PerformanceMonitorOperation(){Mark("pmo.end",0,&name);}};
struct Monitor {std::unique_ptr<PerformanceMonitorOperation> start(int,const std::string& name,PlayerbotAI*) {Count(Counters::pmo);Mark("pmo.start",0,&name);return std::make_unique<PerformanceMonitorOperation>(name);}};
inline Monitor sPerformanceMonitor;
struct Threat {
 Unit* owner=nullptr;std::map<Unit*,float> levels;
 float getThreat(Unit* player);
};
struct Unit {
 unsigned id=0;bool inWorld=true,safe=true,possibleValid=true,alive=true;uint32 map=0,level=10,entry=100,area=0;
 float distance=2;Unit* victim=nullptr;ObjectGuid guid,targetGuid;std::set<Unit*> attackers;std::vector<Unit*> controlled;Threat threat;
 explicit Unit(unsigned id=0):id(id),guid(id){threat.owner=this;}virtual ~Unit()=default;
 ObjectGuid GetObjectGuid()const{return guid;}bool IsInWorld(){Mark("world",id);return inWorld;}
 uint32 GetMapId(){Mark("map",id);return map;}uint32 GetLevel(){Mark("level",id);return level;}uint32 GetEntry(){Mark("entry",id);return entry;}
 Unit* GetVictim(){Mark("victim",id);return victim;}ObjectGuid GetTargetGuid(){return targetGuid;}
 Threat& GetThreatManager(){return threat;}const std::set<Unit*>& GetAttackers(){Mark("attackers",id);return attackers;}
 const char* GetName(){return "fixture-unit";}
 bool IsWithinDist(Unit*,float range,bool){Mark("within",id);return distance<=range;}
 template<class F> void CallForAllControlledUnits(F callback,int mask){assert(mask==31);Mark("controlled",id);for(auto* unit:controlled)callback(unit);}
};
using WorldObject=Unit;
struct Creature:Unit {using Unit::Unit;};
struct Pet:Creature {explicit Pet(unsigned id=0):Creature(id){guid.value|=uint64(1)<<63;}};
struct Duel {Player* opponent=nullptr;};
struct Player:Unit {
 using Unit::Unit;
 bool teleport=false,flying=false,confused=false,bg=false;int bgType=0;float height=0;
 Group* group=nullptr;Pet* pet=nullptr;Duel* m_duel=nullptr;PlayerbotAI* ai=nullptr;std::list<Unit*> possible;
 bool IsBeingTeleported(){Mark("teleport",id);return teleport;}
 bool IsFlying(){Mark("flying",id);return flying;}
 bool HasFlag(int field,int flag){assert(field==UNIT_FIELD_FLAGS && flag==UNIT_FLAG_CONFUSED);Mark("confused",id);return confused;}
 Group* GetGroup(){Mark("group",id);return group;}
 bool InBattleGround(){Mark("bg",id);return bg;}int GetBattleGroundTypeId(){return bgType;}
 PlayerbotAI* GetPlayerbotAI(){Mark("ai",id);return ai;}
 Pet* GetPet(){Mark("pet",id);return pet;}
};
struct Group {
 struct MemberSlot {ObjectGuid guid;};using MemberSlotList=std::list<MemberSlot>;using member_citerator=MemberSlotList::const_iterator;
 MemberSlotList members;const MemberSlotList& GetMemberSlots(){Mark("members");return members;}
};
struct WorldPosition {
 float coordinate=0,height=0;bool overworld=true;
 WorldPosition()=default;WorldPosition(Player* p):coordinate(p?p->distance:0),height(p?p->height:0){}
 WorldPosition& operator=(Player* p){coordinate=p->distance;height=p->height;return *this;}
 float currentHeight(){return height;}float sqDistance2d(Player* p){return (coordinate-p->distance)*(coordinate-p->distance);}
 bool isOverworld(){return overworld;}
};
struct PlayerbotAI {
 Player* bot;Player* master=nullptr;std::unique_ptr<Context> context;bool activity=true,focus=false,pvp=true,quester=false,activeMaster=false,follow=false,wander=false;BotState combat=BotState::BOT_STATE_NON_COMBAT;
 explicit PlayerbotAI(Player* bot);~PlayerbotAI();
 Player* GetBot(){return bot;}Context* GetAiObjectContext(){return context.get();}
 bool AllowActivity(int=ALL_ACTIVITY){Mark("activity",bot->id);return activity;}
 bool IsSafe(Unit* u){Mark("safe",u?u->id:0);return u && u->safe;}
 bool HasStrategy(const std::string& name,BotState){Mark("strategy",bot->id,&name);return name=="focus rti targets"?focus:name=="pvp"?pvp:name=="follow"?follow:name=="wander"?wander:false;}
 bool IsAvQuester(){Mark("av",bot->id);return quester;}bool HasActivePlayerMaster(){Mark("activeMaster",bot->id);return activeMaster;}
 Player* GetMaster(){Mark("master",bot->id);return master;}BotState GetState(){return combat;}
 uint32 GetFixedBotNumber(BotTypeNumber,int,float);Unit* GetUnit(ObjectGuid);WorldObject* GetWorldObject(ObjectGuid id){return GetUnit(id);}
};
struct Facade {
 float GetDistance2d(Unit*,Unit* other){Count(Counters::distances);Mark("distance",other->id);return other->distance;}
 bool IsAlive(Unit* unit){Mark("alive",unit->id);return unit->alive;}
 uint32 GetAreaId(Unit* unit){Mark("area",unit->id);return unit->area;}
 bool IsWithinLOSInMap(Unit*,Unit*){return true;}
};
inline Facade sServerFacade;
struct ObjectMgr {Player* GetPlayer(ObjectGuid);};
inline ObjectMgr sObjectMgr;
struct EnemyPlayerValue {static float GetMaxAttackDistance(Player*){return 60;}};
struct AiNamedObject {
 PlayerbotAI* ai;Player* bot;Context* context;std::string name;
 AiNamedObject(PlayerbotAI* ai,std::string name):ai(ai),bot(ai->bot),context(ai->GetAiObjectContext()),name(std::move(name)){}
 virtual ~AiNamedObject()=default;virtual std::string getName(){return name;}Player* GetMaster(){return ai->GetMaster();}
};
inline void ObserveGet(const std::string& name){Count(Counters::gets);Mark("get",0,&name);}

