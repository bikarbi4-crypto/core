// Included in two namespaces. No server, database, network or gameplay is run.
using uint8=std::uint8_t; using uint32=std::uint32_t;
#include "enums.inc"
inline PlayerActivityPresence presence;
inline std::vector<std::pair<unsigned,PlayerActivityPresence::Token>> tokens;
struct Env
{
 bool disabled=false,master=false,self=false,group=false,teleport=false,bg=false,overworld=true;
 bool minimal=true,pathEmpty=false,guild=false,order=false,combat=false,freeAlt=false,travel=false,queue=false;
 bool friendFound=false,realGuild=false,inWorld=true,continent=true;
 unsigned instance=5,samples=20,alone=100,rng=27,fixed=17; float localA=65,activity=90;
 std::time_t now=100; std::vector<unsigned> randomResults;
};
inline Env env;
struct PlayerbotAI; struct Group; struct Map; struct AiObjectContext {};
struct WorldObject
{
 float x=1000,y=0,z=0;
 float GetPositionX() const {return x;} float GetPositionY() const {return y;} float GetPositionZ() const {return z;}
};
struct Camera { WorldObject* body=nullptr; WorldObject* GetBody(){return body;} };
struct Player:WorldObject
{
 unsigned id=0,zone=1,mapId=0,instance=5; bool inWorld=true,gm=false,gmFlag=false,client=false;
 PlayerbotAI* ai=nullptr; Camera camera;
 explicit Player(unsigned guid=0):id(guid) {camera.body=this;}
 Group* GetGroup(); Map* GetMap();
 bool IsBeingTeleported(){return id==1&&env.teleport;}
 bool IsInWorld(){return id==1?env.inWorld:inWorld;}
 unsigned GetGuildId(){return env.guild;}
 bool InBattleGroundQueue(){return env.queue;}
 PlayerbotAI* GetPlayerbotAI(){return ai;}
 unsigned GetGUIDLow()const{return id;} unsigned GetObjectGuid()const{return id;}
 unsigned GetZoneId()const{return zone;} unsigned GetMapId()const{return mapId;}
 unsigned GetInstanceId()const{return instance;}
 bool IsGameMaster()const{return gm;}
 bool HasFlag(int,int)const{return gmFlag;}
 Camera& GetCamera(){return camera;}
 bool isRealPlayer()const{return ai==nullptr;}
};
inline constexpr int PLAYER_FLAGS=0,PLAYER_FLAGS_GM=1;
struct GroupReference
{
 Player* p=nullptr; GroupReference* following=nullptr;
 Player* getSource()const{return p;} GroupReference* next(){return following;}
};
struct Group {GroupReference first;GroupReference* GetFirstMember(){return &first;}};
struct Map
{
 using PlayerList=std::vector<GroupReference>;PlayerList list;
 PlayerList const& GetPlayers(){return list;}
 bool IsContinent(){return env.continent;} unsigned GetInstanceId(){return env.instance;}
 unsigned GetAverageUpdateTimeSamples10s(){return env.samples;} float GetBotActivityPercentage(){return env.localA;}
};
inline Group group;inline Map map;
inline Player bot{1},human{10000},member{2};
inline std::vector<Player> bots;
inline Group* Player::GetGroup(){return env.group?&group:nullptr;}
inline Map* Player::GetMap(){return &map;}
struct LastMovement {std::vector<int> lastPath;};inline LastMovement movement;
struct GuildOrder {bool IsValid(){return env.order;}};
template<class T> T Value(char const*)
{if constexpr(std::is_same_v<T,LastMovement&>)return movement;else return GuildOrder{};}
struct WorldPosition
{
 float x,y,z;
 WorldPosition(WorldObject* p):x(p->x),y(p->y),z(p->z){}
 bool isBg(){return env.bg;} bool isOverworld(){return env.overworld;}
 float getVisibilityDistance(){return 50;}
 float getX(){return x;}float getY(){return y;}float getZ(){return z;}
 float sqDistance(WorldPosition other){return (x-other.x)*(x-other.x)+(y-other.y)*(y-other.y)+(z-other.z)*(z-other.z);}
};
struct Config
{
 bool disableActivityPriorities=false,enableMinimalMove=true,guildOrderAlwaysActive=true,forceActiveWhenNearPlayer=false,limitCombatActivity=false;
 unsigned botActiveAlone=100;float reactDistance=10;
 bool continentInstancedActivityScaling=false;
 bool IsFreeAltBot(Player*){return env.freeAlt;}
};
inline Config sPlayerbotAIConfig;
struct Facade{bool IsInCombat(Player*){return env.combat;}};inline Facade sServerFacade;
using PlayerBotMap=std::map<unsigned,Player*>;
struct RandomPlayerbotMgr
{
 PlayerBotMap registry; std::shared_mutex mutex;
 bool HasPlayers()
 {
#ifdef CANDIDATE
  return presence.HasPlayers();
#else
  std::shared_lock<std::shared_mutex> lock(mutex);return !registry.empty();
#endif
 }
 PlayerBotMap GetPlayersSnapshot(){PROBE(copies);COUNT(copiedEntries,registry.size());return registry;}
 PlayerBotMap& GetPlayers(){return registry;} std::shared_mutex& GetPlayersMutex(){return mutex;}
 float getActivityPercentage(){return env.activity;}
 float getActivityPercentage(Player*);
};
inline RandomPlayerbotMgr sRandomPlayerbotMgr;
struct Social
{
 bool HasFriend(unsigned guid,unsigned){PROBE(friendChecks);return env.friendFound&&guid==human.id;}
};inline Social sSocialMgr;
enum class BotState{BOT_STATE_NON_COMBAT};enum class BotTypeNumber{ACTIVITY_TYPE_NUMBER};
inline uint32 urand(unsigned a,unsigned b)
{
 env.rng=1664525u*env.rng+1013904223u;auto v=a+env.rng%(b-a+1);
#ifdef PRESENCE_COUNT
 env.randomResults.push_back(v);
#endif
 return v;
}
inline std::time_t time(void*){return env.now;}
struct PlayerbotAI
{
 Player* bot=&::NAMESPACE::bot;bool real=false,realMaster=false;
 bool allowActive[MAX_ACTIVITY_TYPE]{};std::time_t allowActiveCheckTimer[MAX_ACTIVITY_TYPE]{};
 bool HasRealPlayerMaster(){return bot->id==1?env.master:realMaster;}
 Player* GetMaster(){return &human;}
 bool IsRealPlayer(){return bot->id==1?env.self:real;}
 bool IsSafe(Player*){return true;}
 bool HasPlayerNearby(float range=0);bool HasPlayerNearby(WorldPosition,float);
 bool HasStrategy(char const*,BotState){return env.travel;}
 bool IsInRealGuild(){return env.realGuild;}
 AiObjectContext* GetAiObjectContext(){return nullptr;}
 uint32 GetFixedBotNumber(BotTypeNumber,uint32,float){return env.fixed;}
 ActivePiorityType GetPriorityType();std::pair<uint32,uint32> GetPriorityBracket(ActivePiorityType);
 bool AllowActive(ActivityType);bool AllowActivity(ActivityType,bool checkNow=false);
};
inline PlayerbotAI botAI,memberAI,humanAI;
inline void Publish(Player& p)
{
 if(!p.inWorld||!p.client)return;
 PlayerActivityPresence::Observation o;
 o.location={p.mapId,p.instance,p.zone,{p.x,p.y,p.z}};o.excludedFromNearby=p.gm&&p.gmFlag;
 if(p.camera.body && p.camera.body!=&p){o.remoteCamera=true;o.camera={p.camera.body->x,p.camera.body->y,p.camera.body->z};}
 tokens.push_back({p.id,presence.Enter(p.id,o)});
}
// Human location: 0 absent, 1 other map, 2 other zone, 3 same zone,
// 4 visible, 5 nearby-only, 6 strict reaction-distance boundary.
inline void Setup(unsigned humans,unsigned mixed=0,unsigned mapBots=0)
{
 for(auto t:tokens)presence.Leave(t.first,t.second);tokens.clear();
 env=Env{};sPlayerbotAIConfig=Config{};map.list.clear();sRandomPlayerbotMgr.registry.clear();bots.clear();
 bot=Player(1);bot.camera.body=&bot;bot.x=0;bot.ai=&botAI;botAI.bot=&bot;
 human=Player(10000);human.camera.body=&human;human.client=humans!=0;human.mapId=humans==1?1:0;
 human.zone=humans==2?2:1;human.x=humans==4?2.f:humans==5?55.f:humans==6?10.f:1000.f;
 member=Player(2);member.camera.body=&member;member.ai=&memberAI;memberAI.bot=&member;memberAI.realMaster=false;
 group.first={&member,nullptr};movement.lastPath={1};
 bots.reserve(std::max(mixed,mapBots));
 for(unsigned i=0;i<std::max(mixed,mapBots);++i)
 {
  bots.emplace_back(10+i);auto& p=bots.back();p.ai=&memberAI;p.camera.body=&p;
  if(i<mixed)sRandomPlayerbotMgr.registry[p.id]=&p;
  if(i<mapBots)map.list.push_back({&p,nullptr});
 }
 if(humans)
 {
  sRandomPlayerbotMgr.registry[human.id]=&human;
  if(human.mapId==bot.mapId)map.list.push_back({&human,nullptr});
  Publish(human);
 }
}
inline void Scenario(unsigned n)
{
 Setup(n>=18?((n==21)?2:(n==22)?3:1):0);
 switch(n)
 {
 case 1:env.disabled=true;sPlayerbotAIConfig.disableActivityPriorities=true;break;
 case 2:Setup(1);env.master=true;break;
 case 3:env.self=true;bot.client=true;Publish(bot);break;
 case 4:Setup(1);env.group=true;group.first={&human,nullptr};break;
 case 5:Setup(1);env.group=true;memberAI.realMaster=true;break;
 case 6:env.teleport=true;break;case 7:env.bg=true;break;
 case 8:env.overworld=false;sPlayerbotAIConfig.enableMinimalMove=false;break;
 case 9:env.overworld=false;movement.lastPath.clear();break;
 case 10:Setup(4);break;case 11:env.guild=true;env.order=true;break;
 case 12:env.combat=true;break;case 13:Setup(5);break;
 case 14:env.freeAlt=true;break;case 15:env.travel=true;break;case 16:env.queue=true;break;
 case 17:movement.lastPath.clear();break;case 18:env.friendFound=true;break;
 case 19:env.realGuild=true;break;case 23:env.inWorld=false;break;
 }
}
