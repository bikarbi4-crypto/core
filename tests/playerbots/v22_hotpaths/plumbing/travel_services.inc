enum class BotState {BOT_STATE_NON_COMBAT};
constexpr int TRAVEL_ACTIVITY=1;
struct TravelFixture {bool activity=true,traveling=true,taxi=false,moving=false,canMove=true,follow=false,wander=false,stay=false,guard=false,group=false,leader=true,near=true,loot=false,lootPossible=false,free=true;int delay=11;};
inline TravelFixture config;
struct Group {bool IsLeader(ObjectGuid){Trace("leader");return config.leader;}};
struct Bot {Group group;bool IsTaxiFlying(){Trace("taxi");return config.taxi;}bool IsMoving(){Trace("moving");return config.moving;}Group* GetGroup(){Trace("group");return config.group?&group:nullptr;}ObjectGuid GetObjectGuid(){return {1};}};
struct AI {bool AllowActivity(int){Trace("activity");return config.activity;}bool HasStrategy(const std::string& s,BotState){Trace(s);return s=="follow"?config.follow:s=="wander"?config.wander:s=="stay"?config.stay:config.guard;}};
struct Current {int LastChangeDelay(){Trace("delay");return config.delay;}};
struct LootObject {bool IsLootPossible(Bot*){Trace("loot-possible");return config.lootPossible;}};
struct LootObjectStack {LootObject GetLoot(float){Trace("loot-get");return {};}};
struct CanFreeMoveValue {static bool CanFreeMoveTo(AI*,WorldPosition){Trace("free-move");return config.free;}};
struct TravelMgr {float MapTransDistance(Bot*,WorldPosition,bool){Trace("map-distance");return 1;}};
inline TravelMgr sTravelMgr;
struct Settings {float sightDistance=80,lootDistance=15;};inline Settings sPlayerbotAIConfig;
class MoveToTravelTargetAction {public: AI storageAi;Bot storageBot;AI* ai=&storageAi;Bot* bot=&storageBot;TravelTarget target;Current position;LootObjectStack loot;
 bool isUseful();
 template<class T>T Get(const std::string& n){Trace("Get:"+n);
  if constexpr(std::is_same_v<T,bool>){return n=="travel target traveling"?config.traveling:n=="can move around"?config.canMove:n=="group and"?config.near:config.loot;}
  else if constexpr(std::is_same_v<T,TravelTarget*>)return &target;
  else return &loot;
 }
};
#define AI_VALUE(type,name) Get<type>(name)
#define AI_VALUE2(type,name,param) Get<type>(name)
#define MEM_AI_VALUE(type,name) (&position)
