#pragma once
#include "api.h"
#include <algorithm>
#include <array>
#include <cassert>
#include <cctype>
#include <cfloat>
#include <cmath>
#include <ctime>
#include <functional>
#include <list>
#include <map>
#include <memory>
#include <set>
#include <sstream>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <vector>
using uint8=std::uint8_t; using int8=std::int8_t;
using uint32=std::uint32_t; using int32=std::int32_t;
using uint64=std::uint64_t;
using std::stoi;
constexpr unsigned QUEST_OBJECTIVES_COUNT=4;
constexpr int QUEST_STATUS_INCOMPLETE=1, QUEST_STATUS_COMPLETE=2, PERF_MON_VALUE=1;
struct QuestStatusData { int m_status=QUEST_STATUS_INCOMPLETE; bool m_rewarded=false; uint32 m_itemcount[4]{},m_creatureOrGOcount[4]{}; };
using QuestStatusMap=std::map<uint32,QuestStatusData>;
struct Quest { bool active=true; uint32 ReqItemCount[4]{},ReqItemId[4]{},ReqCreatureOrGOCount[4]{}; int32 ReqCreatureOrGOId[4]{}; bool IsActive() const { return active; } };
struct ItemPrototype { uint32 BuyPrice=100; };
class Player {
public:
    QuestStatusMap status;
    uint32 money=101;
    QuestStatusMap& GetQuestStatusMap();
    bool IsActiveQuest(uint32) const;
    uint32 GetMoney() const;
};
class PlayerbotAI {}; // live-world constructor deliberately excluded
struct GuidPosition {};
struct WorldPosition { float x=1; };
struct PlayerTravelInfo { WorldPosition position; PlayerTravelInfo(Player*); const WorldPosition& GetPosition() const { return position; } };
struct ObjectMgr {
    std::map<uint32,Quest> quests;
    bool prototype=true;
    ItemPrototype item;
    const Quest* GetQuestTemplate(uint32) const;
    const ItemPrototype* GetItemPrototype(uint32) const;
};
extern ObjectMgr sObjectMgr;
struct AuditConfig { bool perfMonEnabled=false; };
extern AuditConfig sPlayerbotAIConfig;
struct PerformanceMonitorOperation { ~PerformanceMonitorOperation(); };
struct AuditMonitor { std::unique_ptr<PerformanceMonitorOperation> start(int,const std::string&,PlayerbotAI*); };
extern AuditMonitor sPerformanceMonitor;
extern time_t fixtureTime;
extern Observation observation;
extern bool recording;
extern std::function<void(const std::string&)> onTrace;
time_t audit_time(time_t*);
#ifdef QUEST_TRACE
void Trace(const std::string&);
#else
#define Trace(...) ((void)0)
#endif
void TraceDestination(const std::vector<int32>&,bool,float);
enum class TravelDestinationPurpose : uint32 { None=0, QuestGiver=1, QuestObjective1=2, QuestObjective2=4, QuestObjective3=8, QuestObjective4=16, QuestAllObjective=30 };
struct TravelDestination {
    int32 entry=0; float distance=1;
    virtual ~TravelDestination()=default;
    virtual int32 GetEntry() const { return entry; }
    bool IsPossible(const PlayerTravelInfo&) const { assert(false); return false; }
    float DistanceTo(const WorldPosition&) const;
};
using DestinationList=std::vector<TravelDestination*>;
using EntryDestinationMap=std::unordered_map<int32,DestinationList>;
using PurposeDestinationMap=std::unordered_map<TravelDestinationPurpose,EntryDestinationMap>;
using DropMap=std::unordered_multimap<uint32,int32>;
using VendorMap=std::unordered_multimap<uint32,int32>;
extern DropMap fixtureDrops;
extern VendorMap fixtureVendors;
extern Player* fixturePlayer;
void SetWorld(const Case&);
