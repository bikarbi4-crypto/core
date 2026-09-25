// Included independently in baseline/candidate namespaces. No world/DB is linked.
using uint8=std::uint8_t; using uint32=std::uint32_t;
#include "enums.inc"
struct Env
{
    bool disabled=false, master=false, self=false, group=false, groupReal=false, groupMaster=false;
    bool teleport=false, bg=false, overworld=true, minimal=true, pathEmpty=false;
    bool visible=false, guild=false, order=false, combat=false, nearby=false, freeAlt=false, travel=false, queue=false;
    bool players=false, friendFound=false, realGuild=false, inWorld=true, mapExists=true;
    bool forceVisible=false, limitCombat=false, guildOrderAlways=true;
    bool continent=true; unsigned instance=5, samples=20; float localA=65;
    float activity=90; unsigned alone=100, rng=27, fixed=17;
    unsigned mapCase=0; std::time_t now=100;
    bool tracing=true; std::vector<std::string> calls;
};
inline Env env;
inline void Call(char const* name) { if(env.tracing) env.calls.push_back(name); }
struct PlayerbotAI;
struct Map;
struct Group;
struct AiObjectContext {};
struct Player
{
    unsigned id=0, zone=1; bool inWorld=true; PlayerbotAI* ai=nullptr;
    Group* GetGroup();
    bool IsBeingTeleported() { Call("teleport"); return id==1 && env.teleport; }
    bool IsInWorld() { Call("in-world"); return id==1 ? env.inWorld : inWorld; }
    unsigned GetGuildId() { Call("guild-id"); return env.guild; }
    bool InBattleGroundQueue() { Call("queue"); return env.queue; }
    PlayerbotAI* GetPlayerbotAI() { Call("get-ai"); return ai; }
    unsigned GetGUIDLow() { return id; }
    unsigned GetObjectGuid() { return id; }
    unsigned GetZoneId() { Call("zone"); return zone; }
    Map* GetMap();
};
struct GroupReference
{
    Player* p=nullptr; GroupReference* following=nullptr;
    Player* getSource() const { Call("group-source"); return p; }
    GroupReference* next() { return following; }
};
struct Group { GroupReference first; GroupReference* GetFirstMember() { Call("group-first"); return &first; } };
struct Map
{
    using PlayerList=std::vector<GroupReference>;
    PlayerList list;
    PlayerList const& GetPlayers() { Call("map-list"); return list; }
    bool IsContinent() { Call("continent"); return env.continent; }
    unsigned GetInstanceId() { Call("instance"); return env.instance; }
    unsigned GetAverageUpdateTimeSamples10s() { Call("samples"); return env.samples; }
    float GetBotActivityPercentage() { Call("local-A"); return env.localA; }
};
inline Group group;
inline Map map;
inline Player bot{1}, other{2}, human{3}, selfbot{4}, absent{5};
inline Group* Player::GetGroup() { Call("group"); return env.group ? &group : nullptr; }
inline Map* Player::GetMap() { Call("map"); return env.mapExists ? &map : nullptr; }
struct LastMovement { std::vector<int> lastPath; };
struct GuildOrder { bool IsValid() { Call("guild-order"); return env.order; } };
inline LastMovement movement;
template<class T> T Value(char const* name)
{
    Call(name);
    if constexpr(std::is_same_v<T,LastMovement&>) return movement;
    else return GuildOrder{};
}
#define AI_VALUE(T, name) Value<T>(name)
struct WorldPosition
{
    explicit WorldPosition(Player*) {}
    bool isBg() { Call("bg"); return env.bg; }
    bool isOverworld() { Call("overworld"); return env.overworld; }
    float getVisibilityDistance() { Call("visibility"); return 50; }
};
struct Config
{
    bool disableActivityPriorities=false, enableMinimalMove=true, guildOrderAlwaysActive=true, forceActiveWhenNearPlayer=false, limitCombatActivity=false;
    unsigned botActiveAlone=100; float reactDistance=5;
    bool continentInstancedActivityScaling=false;
    bool IsFreeAltBot(Player*) { Call("free-alt"); return env.freeAlt; }
};
inline Config sPlayerbotAIConfig;
struct Facade { bool IsInCombat(Player*) { Call("combat"); return env.combat; } };
inline Facade sServerFacade;
using PlayerBotMap=std::map<unsigned,Player*>;
struct RandomPlayerbotMgr
{
    bool HasPlayers() { Call("has-players"); return env.players; }
    PlayerBotMap GetPlayersSnapshot() { Call("snapshot"); return {{3,&human}}; }
    float getActivityPercentage() { Call("activity"); return env.activity; }
    float getActivityPercentage(Player*);
};
inline RandomPlayerbotMgr sRandomPlayerbotMgr;
struct Social { bool HasFriend(unsigned, unsigned) { Call("friend"); return env.friendFound; } };
inline Social sSocialMgr;
enum class BotState { BOT_STATE_NON_COMBAT };
enum class BotTypeNumber { ACTIVITY_TYPE_NUMBER };
inline uint32 urand(unsigned a, unsigned b)
{ Call("rng"); env.rng=1664525u*env.rng+1013904223u; return a+env.rng%(b-a+1); }
inline std::time_t time(void*) { Call("time"); return env.now; }
struct PlayerbotAI
{
    Player* bot=&::NAMESPACE::bot;
    bool real=false, realMaster=false;
    bool allowActive[MAX_ACTIVITY_TYPE]{};
    std::time_t allowActiveCheckTimer[MAX_ACTIVITY_TYPE]{};
    PresenceDiagnostics::BotState presenceState;
    bool HasRealPlayerMaster() { Call("master"); return bot->id==1 ? env.master : realMaster; }
    bool IsRealPlayer() { Call("selfbot"); return bot->id==1 ? env.self : real; }
    bool IsSafe(Player*) { Call("safe"); return true; }
    bool HasPlayerNearby(float range=0) { Call(range==0 ? "visible" : "nearby"); return range==0 ? env.visible : env.nearby; }
    bool HasStrategy(char const*, BotState) { Call("travel"); return env.travel; }
    bool IsInRealGuild() { Call("real-guild"); return env.realGuild; }
    AiObjectContext* GetAiObjectContext() { Call("context"); return nullptr; }
    uint32 GetFixedBotNumber(BotTypeNumber, uint32, float p)
    { Call("fixed-number"); if(env.tracing) env.calls.push_back(std::to_string(p)); return env.fixed; }
    ActivePiorityType GetPriorityType();
    std::pair<uint32,uint32> GetPriorityBracket(ActivePiorityType);
    bool AllowActive(ActivityType);
    bool AllowActivity(ActivityType,bool checkNow=false);
};
inline PlayerbotAI otherAI, selfAI;
inline void Setup(unsigned scenario)
{
    env=Env{};
    // Each high-priority exit, then friend/guild/map/zone/selfbot/null/out-of-world cases.
    switch(scenario)
    {
    case 0: break;
    case 1:env.disabled=true;break; case 2:env.master=true;break; case 3:env.self=true;break;
    case 4:env.group=true;env.groupReal=true;break; case 5:env.group=true;env.groupMaster=true;break;
    case 6:env.teleport=true;break; case 7:env.bg=true;break;
    case 8:env.overworld=false;env.minimal=false;break;
    case 9:env.overworld=false;env.pathEmpty=true;break;
    case 10:env.visible=true;break;case 11:env.guild=true;env.order=true;break;
    case 12:env.combat=true;break;case 13:env.nearby=true;break;
    case 14:env.freeAlt=true;break;case 15:env.travel=true;break;
    case 16:env.queue=true;break;case 17:env.pathEmpty=true;break;
    case 18:env.players=true;env.friendFound=true;break;
    case 19:env.players=true;env.realGuild=true;break;
    case 20:env.players=true;env.inWorld=false;break;
    case 21:env.players=true;env.mapExists=false;break;
    default:env.players=true;env.mapCase=scenario-22;break;
    }
    movement.lastPath=env.pathEmpty ? std::vector<int>{} : std::vector<int>{1};
    bot.inWorld=true; bot.zone=1; bot.ai=nullptr;
    otherAI.bot=&other; otherAI.real=false; otherAI.realMaster=env.groupMaster;
    selfAI.bot=&selfbot; selfAI.real=true;
    other.ai=env.groupReal ? nullptr : &otherAI;
    selfbot.ai=&selfAI; selfbot.zone=1; human.ai=nullptr; human.zone=1; human.inWorld=true;
    absent.inWorld=false; absent.ai=nullptr;
    group.first={&other,nullptr};
    map.list={{nullptr,nullptr},{&absent,nullptr},{&other,nullptr}};
    if(env.mapCase==1) { human.zone=2; map.list.push_back({&human,nullptr}); }
    if(env.mapCase==2) map.list.push_back({&human,nullptr});
    if(env.mapCase==3) map.list.insert(map.list.begin(),{&selfbot,nullptr});
    if(env.mapCase==4) { human.zone=2; map.list.insert(map.list.begin(),{&human,nullptr}); map.list.push_back({&selfbot,nullptr}); }
    sPlayerbotAIConfig={env.disabled,env.minimal,env.guildOrderAlways,env.forceVisible,env.limitCombat,env.alone,5};
}
#undef AI_VALUE
