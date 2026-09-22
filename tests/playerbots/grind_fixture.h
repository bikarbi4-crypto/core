#pragma once
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

using uint32 = std::uint32_t;
using uint64 = std::uint64_t;
using std::abs;
struct ObjectGuid {
    uint64 raw = 0;
    ObjectGuid() = default;
    explicit ObjectGuid(uint64 id) : raw(id) {}
    bool operator==(ObjectGuid other) const { return raw == other.raw; }
    bool operator<(ObjectGuid other) const { return raw < other.raw; }
    bool IsPlayer() const { return (raw >> 63) != 0; }
    uint64 GetRawValue() const { return raw; }
};

class PlayerbotAI;
class Player;
struct FixtureWorld;
inline FixtureWorld* activeWorld = nullptr;
constexpr int BATTLEGROUND_AV=1, HORDE=1, ALLIANCE=0, QUEST_STATUS_INCOMPLETE=1;
constexpr int BG_AV_QUEST_H_RIDER_HIDE=101, BG_AV_QUEST_A_RIDER_HIDE=102;
constexpr int BG_AV_QUEST_A_SCRAPS1=103, BG_AV_QUEST_H_SCRAPS1=104;
constexpr int BG_AV_QUEST_A_SCRAPS2=105, BG_AV_QUEST_H_SCRAPS2=106;
constexpr int CREATURE_TYPE_HUMANOID=1, CREATURE_TYPE_CRITTER=2, CREATURE_ELITE_NORMAL=0;
enum class BotState { BOT_STATE_NON_COMBAT, BOT_STATE_COMBAT };

class Unit {
public:
    virtual ~Unit() = default;
    ObjectGuid guid;
    uint32 entry=1, level=30, mapId=0;
    float x=0, y=0, z=0;
    bool alive=true, hostile=true, freeTarget=true, valid=true, possible=true, player=false;
    uint32 GetEntry() const { return entry; }
    uint32 GetLevel() const { return level; }
    uint32 GetMapId() const { return mapId; }
    float GetPositionZ() const { return z; }
    ObjectGuid GetObjectGuid() const { return guid; }
    bool IsPlayer() const { return player; }
};
struct CreatureInfo { int rank=0; };
class Creature : public Unit {
public:
    CreatureInfo info;
    bool hasInfo=true, elite=false, experience=true;
    int type=CREATURE_TYPE_HUMANOID;
    const CreatureInfo* GetCreatureInfo() const { return hasInfo ? &info : nullptr; }
    int GetCreatureType() const { return type; }
    bool IsElite() const { return elite; }
};
class Group {
public:
    struct MemberSlot { ObjectGuid guid; };
    using MemberSlotList=std::vector<MemberSlot>;
    using member_citerator=MemberSlotList::const_iterator;
    MemberSlotList slots;
    uint32 GetMembersCount() const { return static_cast<uint32>(slots.size()); }
    const MemberSlotList& GetMemberSlots() const { return slots; }
};
class Player : public Unit {
public:
    Group* group=nullptr;
    PlayerbotAI* ai=nullptr;
    ObjectGuid selection;
    bool teleported=false, battleground=false;
    int battleType=0, team=ALLIANCE;
    std::map<uint32,int> quests;
    std::map<uint32,uint32> items;
    Player() { player=true; }
    Group* GetGroup() { return group; }
    PlayerbotAI* GetPlayerbotAI() { return ai; }
    bool IsBeingTeleported() const { return teleported; }
    bool InBattleGround() const { return battleground; }
    int GetBattleGroundTypeId() const { return battleType; }
    int GetTeam() const { return team; }
    int GetQuestStatus(uint32 id) const { auto i=quests.find(id); return i==quests.end()?0:i->second; }
    uint32 GetItemCount(uint32 id) const { auto i=items.find(id); return i==items.end()?0:i->second; }
    ObjectGuid GetSelectionGuid() const { return selection; }
};
class TravelDestination { public: virtual ~TravelDestination()=default; };
class GrindTravelDestination : public TravelDestination {};
class TravelTarget {
public:
    TravelDestination destination;
    TravelDestination* GetDestination() { return &destination; }
};

struct FixtureCounters {
    uint64 playerLookups=0, contextLookups=0, currentTargetReads=0, listReads=0, travelReads=0, distances=0, rngCalls=0;
};
void RecordValueRead(const std::string& name);
struct FixtureValueBase { virtual ~FixtureValueBase()=default; };
template<class T> struct FixtureValue : FixtureValueBase {
    std::string name;
    T value{};
    T Get() { RecordValueRead(name); return value; }
    operator T() { return Get(); }
};
struct FixtureContext {
    std::map<std::string,std::unique_ptr<FixtureValueBase>> values;
    template<class T> FixtureValue<T>* GetValue(std::string name, std::string qualifier="");
    template<class T> void Set(std::string name, T value) {
        auto entry=std::make_unique<FixtureValue<T>>(); entry->name=name; entry->value=std::move(value); values[name]=std::move(entry);
    }
};
struct FixtureChat { std::string formatWorldobject(Unit* unit) { return std::to_string(unit->GetObjectGuid().raw); } };
class PlayerbotAI {
public:
    Player* bot=nullptr;
    Player* master=nullptr;
    FixtureContext context;
    FixtureChat chat;
    bool debug=false, follow=false, wander=false, avQuester=false;
    Unit* GetUnit(ObjectGuid guid);
    FixtureContext* GetAiObjectContext() { return &context; }
    bool IsAvQuester() const { return avQuester; }
    bool HasStrategy(const std::string& name, BotState) const {
        if(name=="debug grind") return debug;
        if(name=="follow") return follow;
        if(name=="wander") return wander;
        return false;
    }
    void TellPlayer(Player*, const std::string& message);
};
class TargetValue {
public:
    PlayerbotAI* ai;
    Player* bot;
    FixtureContext* context;
    FixtureChat* chat;
    TargetValue(PlayerbotAI* owner,std::string,int) : ai(owner),bot(owner->bot),context(&owner->context),chat(&owner->chat) {}
    virtual ~TargetValue()=default;
    virtual Unit* Calculate()=0;
    Player* GetMaster() { return ai->master; }
};
struct FixtureConfig { float spellDistance=30, sightDistance=60, proximityDistance=20; };
inline FixtureConfig sPlayerbotAIConfig;
struct GuidPosition { Unit* unit; explicit GuidPosition(Unit* target):unit(target){} };
struct CanFreeMoveValue { static bool CanFreeTarget(PlayerbotAI*,GuidPosition position) { return position.unit->freeTarget; } };
struct AttackersValue { static bool IsValid(Unit* unit,Player*,Player*,bool,bool) { return unit->valid; } };
struct PossibleAttackTargetsValue { static bool IsPossibleTarget(Unit* unit,Player*,float,bool) { return unit->possible; } };
namespace MaNGOS { namespace XP { inline uint32 Gain(Player*,Creature* creature) { return creature->experience ? 1u : 0u; } } }
struct FixtureFacade {
    bool IsAlive(Unit* unit) { return unit->alive; }
    bool IsHostileTo(Player*,Unit* unit) { return unit->hostile; }
    float GetDistance2d(Unit* a,Unit* b);
};
inline FixtureFacade sServerFacade;
struct FixtureObjectMgr { Player* GetPlayer(ObjectGuid guid); };
inline FixtureObjectMgr sObjectMgr;

struct Scenario {
    unsigned seed=1, members=0, targets=0;
    bool noTarget=false, allDead=false, attackersFirst=false, randomFilters=false, debug=false;
    bool mixedPlayers=false, duplicateGuidPointers=false, battleground=false, av=false, travelWorking=false;
    bool changingValues=false;
};
struct FixtureWorld {
    Scenario scenario;
    Player bot;
    PlayerbotAI ai;
    Group group;
    TravelTarget travel;
    std::vector<std::unique_ptr<Player>> members;
    std::vector<std::unique_ptr<PlayerbotAI>> memberAis;
    std::vector<std::unique_ptr<Creature>> units;
    std::unique_ptr<Creature> alternatePointer;
    std::unordered_map<uint64,Unit*> unitIndex;
    std::unordered_map<uint64,Player*> playerIndex;
    FixtureCounters counters;
    uint32 rng=1;
    bool trace=true;
    std::vector<std::string> messages;
    std::vector<std::pair<uint32,uint32>> randomTrace;
    unsigned possibleReads=0;
    explicit FixtureWorld(Scenario input);
};

inline void RecordValueRead(const std::string& name) {
    auto& w=*activeWorld;
    if(name=="current target") ++w.counters.currentTargetReads;
    if(name=="possible targets" || name=="possible attack targets") ++w.counters.listReads;
    if(name.find("travel target")==0) ++w.counters.travelReads;
    if(name=="possible targets" && w.scenario.changingValues && ++w.possibleReads==2) {
        // Model an expired/recalculated list between assist passes. A must still Get it.
        auto* value=static_cast<FixtureValue<std::list<ObjectGuid>>*>(w.ai.context.values.at(name).get());
        value->value.clear();
        w.ai.context.Set<bool>("travel target working",!w.scenario.travelWorking);
    }
}
template<class T> FixtureValue<T>* FixtureContext::GetValue(std::string name,std::string qualifier) {
    ++activeWorld->counters.contextLookups;
    if(!qualifier.empty()) name+="::"+qualifier;
    auto it=values.find(name);
    if(it==values.end()) { Set<T>(name,T{}); it=values.find(name); }
    auto* result=dynamic_cast<FixtureValue<T>*>(it->second.get());
    assert(result);
    return result;
}
inline Unit* PlayerbotAI::GetUnit(ObjectGuid guid) {
    auto it=activeWorld->unitIndex.find(guid.raw);
    return it==activeWorld->unitIndex.end()?nullptr:it->second;
}
inline void PlayerbotAI::TellPlayer(Player*,const std::string& message) { if(activeWorld->trace) activeWorld->messages.push_back(message); }
inline float FixtureFacade::GetDistance2d(Unit* a,Unit* b) { ++activeWorld->counters.distances; return std::hypot(a->x-b->x,a->y-b->y); }
inline Player* FixtureObjectMgr::GetPlayer(ObjectGuid guid) {
    ++activeWorld->counters.playerLookups;
    auto it=activeWorld->playerIndex.find(guid.raw); return it==activeWorld->playerIndex.end()?nullptr:it->second;
}
inline uint32 urand(uint32 low,uint32 high) {
    assert(high>=low);
    auto& w=*activeWorld;
    w.rng=w.rng*1664525u+1013904223u;
    const uint32 result=low+w.rng%(high-low+1);
    ++w.counters.rngCalls;
    if(w.trace) w.randomTrace.emplace_back(high,result);
    return result;
}

inline FixtureWorld::FixtureWorld(Scenario input):scenario(input),rng(input.seed) {
    bot.guid=ObjectGuid((uint64(1)<<63)|1); bot.ai=&ai; bot.alive=!input.allDead;
    bot.battleground=input.battleground; bot.battleType=input.av?BATTLEGROUND_AV:2;
    ai.bot=&bot; ai.debug=input.debug; ai.avQuester=input.av;
    if(input.av) bot.quests[BG_AV_QUEST_A_SCRAPS1]=QUEST_STATUS_INCOMPLETE;
    if(input.members) { bot.group=&group; group.slots.push_back({bot.guid}); }
    playerIndex[bot.guid.raw]=&bot;
    std::mt19937 gen(input.seed);
    for(unsigned i=1;i<input.members;++i) {
        auto p=std::make_unique<Player>(); p->guid=ObjectGuid((uint64(1)<<63)|(i+1)); p->group=&group;
        p->x=static_cast<float>(i%7); p->y=static_cast<float>(i%3);
        p->alive=!input.allDead && (!input.randomFilters || i%5!=0);
        if(!input.mixedPlayers || i%3) {
            auto owner=std::make_unique<PlayerbotAI>(); owner->bot=p.get(); p->ai=owner.get(); memberAis.push_back(std::move(owner));
        }
        group.slots.push_back({p->guid}); playerIndex[p->guid.raw]=p.get(); members.push_back(std::move(p));
    }
    std::list<ObjectGuid> targets;
    for(unsigned i=0;i<input.targets;++i) {
        auto u=std::make_unique<Creature>(); u->guid=ObjectGuid((uint64(7)<<48)|(i+1)); u->entry=100+i%5;
        u->x=1.0f+static_cast<float>(gen()%50); u->y=static_cast<float>(gen()%20);
        if(input.noTarget) u->valid=false;
        if(input.randomFilters) {
            u->valid=(gen()%7)!=0; u->possible=(gen()%5)!=0; u->freeTarget=(gen()%7)!=0;
            u->z=static_cast<float>(gen()%60); u->level=25+gen()%12; u->type=(gen()%4)?CREATURE_TYPE_HUMANOID:CREATURE_TYPE_CRITTER;
            u->experience=(gen()%3)!=0; u->info.rank=(gen()%5)==0?1:0;
        }
        targets.push_back(u->guid); unitIndex[u->guid.raw]=u.get(); units.push_back(std::move(u));
    }
    for(unsigned i=0;i<members.size();++i) {
        Unit* target=units.empty()?nullptr:units[i%units.size()].get();
        if(input.duplicateGuidPointers && target && i==0) {
            alternatePointer=std::make_unique<Creature>(*static_cast<Creature*>(target));
            alternatePointer->mapId=2; target=alternatePointer.get();
        }
        if(members[i]->ai) members[i]->ai->context.Set<Unit*>("current target",target);
        else members[i]->selection=target?target->guid:ObjectGuid();
    }
    ai.context.Set<std::list<ObjectGuid>>("possible targets",targets);
    std::list<ObjectGuid> attackers;
    if(input.attackersFirst && !units.empty()) attackers.push_back(units.front()->guid);
    ai.context.Set<std::list<ObjectGuid>>("possible attack targets",attackers);
    ai.context.Set<bool>("travel target working",input.travelWorking);
    ai.context.Set<bool>("travel target traveling",input.travelWorking);
    ai.context.Set<TravelTarget*>("travel target",&travel);
    ai.context.Set<bool>("can fight elite",!input.randomFilters);
    for(unsigned i=0;i<5;++i) ai.context.Set<bool>("need for quest::"+std::to_string(100+i),!input.randomFilters || (gen()%3)!=0);
}

#define AI_VALUE(type,name) context->GetValue<type>(name)->Get()
#define AI_VALUE2(type,name,qualifier) context->GetValue<type>(name,qualifier)->Get()
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4189 4244 4267 4458) // Unchanged V16 variables, conversions and shadowed ai.
#endif
#include "grind_contracts.h"
#ifdef _MSC_VER
#pragma warning(pop)
#endif
#undef AI_VALUE
#undef AI_VALUE2
