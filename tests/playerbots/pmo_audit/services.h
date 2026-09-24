#pragma once
#include "api.h"
#include <algorithm>
#include <cassert>
#include <chrono>
#include <cstring>
#include <ctime>
#include <iostream>
#include <iterator>
#include <list>
#include <map>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <sstream>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

namespace AUDIT_NAMESPACE
{
using uint32 = uint32_t;
struct Config { bool perfMonEnabled=false; uint32 globalCoolDown=1500, reactDelay=100, maxWaitForMove=5000; };
extern Config sPlayerbotAIConfig;
extern thread_local PmoCounters counters;
extern thread_local std::vector<std::string> events;
extern thread_local uint64_t virtualTime;
extern thread_local bool observe;
void Work(const char* name);
void Mark(const std::string& name);
struct AuditClock
{
    static std::chrono::high_resolution_clock::time_point now()
    {
        ++counters.clocks;
        return std::chrono::high_resolution_clock::time_point(std::chrono::milliseconds(virtualTime));
    }
};
struct Log { template<class... T> void Out(T&&...) {} };
inline Log sLog;
inline constexpr int LOG_BASIC=0, LOG_LVL_MINIMAL=0, LOG_LVL_DEBUG=0;
class PlayerbotAI;
class PerformanceMonitorOperation;
class ChatHandler { public: bool HandlePerfMonCommand(char*); };
struct Bot
{
    uint32 mapId=0, instanceId=0;
    uint32 GetMapId() const { return mapId; }
    uint32 GetInstanceId() const { return instanceId; }
};
struct Context { std::vector<std::string> performanceStack; };
class PlayerbotAI
{
public:
    Bot bot;
    Context context;
    bool withContext=true;
    Bot* GetBot() { return &bot; }
    Context* GetAiObjectContext() { return withContext ? &context : nullptr; }
};
// The PMO scope uses the exact production isInstance predicates. Position
// coordinates/memory-monitor hooks are fixtures, not a game-world benchmark.
struct WorldPosition
{
    uint32 mapId;
    explicit WorldPosition(Bot* bot) : mapId(bot->GetMapId())
    {
#ifdef PMO_TRACE
        ++counters.positions;
#endif
    }
    bool isOverworld() const { return mapId == 0 || mapId == 1 || mapId == 530 || mapId == 571 || mapId == 609; }
    bool isInstance() const { return !isOverworld() || mapId == 609; }
};
struct Event
{
    std::string source;
    bool owner=false;
    std::string getSource() const
    {
#ifdef PMO_TRACE
        ++counters.eventSources;
#endif
        return source;
    }
    void* getOwner() const { return owner ? (void*)this : nullptr; }
};
struct Action
{
    std::string name;
    unsigned flags=0;
    std::string getName() const
    {
#ifdef PMO_TRACE
        ++counters.actionNames;
#endif
        return name;
    }
    bool isUseful() { Work("isUseful"); return !(flags & 4); }
    bool isPossible() { Work("isPossible"); return !(flags & 8); }
    void MakeVerbose(bool) { Work("MakeVerbose"); }
    int getContinuers() { Work("getContinuers"); return 0; }
};
struct ActionNode {};
enum ActionResult { ACTION_RESULT_UNKNOWN, ACTION_RESULT_OK, ACTION_RESULT_FAILED, ACTION_RESULT_IMPOSSIBLE, ACTION_RESULT_USELESS };
class Engine
{
public:
    PlayerbotAI* ai;
    Action action;
    ActionResult ExecuteAction(const std::string&, Event&);
    ActionNode* CreateActionNode(const std::string&) { Work("CreateActionNode"); return action.flags&1 ? nullptr : new ActionNode; }
    Action* InitializeAction(ActionNode*) { Work("InitializeAction"); return action.flags&2 ? nullptr : &action; }
    bool ListenAndExecute(Action*, Event&) { Work("ListenAndExecute"); return !(action.flags&16); }
    void MultiplyAndPush(int,float,bool,Event&,const char*) { Work("MultiplyAndPush"); }
};
class PlayerbotAIBase
{
public:
    unsigned aiInternalUpdateDelay=0;
    std::unique_ptr<PerformanceMonitorOperation> totalPmo;
    PlayerbotAIBase();
    ~PlayerbotAIBase();
    void UpdateAI(uint32 elapsed);
    bool CanUpdateAIInternal() const { return aiInternalUpdateDelay<100; }
    void UpdateAIInternal(uint32) { Work("UpdateAIInternal"); }
    void YieldAIInternalThread() { Work("YieldAIInternalThread"); }
};
}
