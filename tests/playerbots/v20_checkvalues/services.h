#pragma once
#include "api.h"
#include <algorithm>
#include <array>
#include <cassert>
#include <ctime>
#include <functional>
#include <list>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

using uint32=uint32_t;
using ObjectGuid=uint64_t;
inline bool trace=true, benchmark=false;
inline time_t logicalTime=10000;
inline std::vector<std::string> events;
template<class F> inline void Record(F&& message) {
    if (trace) {
        struct Pause { bool prior=countAllocations; Pause(){countAllocations=false;} ~Pause(){countAllocations=prior;} } pause;
        events.push_back(message());
    }
}
#define AUDIT_MARK(message) Record([&]{return (message);})
inline time_t AuditTime(time_t*) { ++counters.clock; if (benchmark) (void)std::time(nullptr); return logicalTime; }
struct Config { bool perfMonEnabled=false; };
inline Config sPlayerbotAIConfig;
constexpr int PERF_MON_VALUE=1;
struct PerformanceMonitorOperation {
    std::string name;
    explicit PerformanceMonitorOperation(std::string name):name(std::move(name)) { ++live; }
    ~PerformanceMonitorOperation() { --live; AUDIT_MARK("finish:"+name); }
    inline static unsigned live=0;
};
class PlayerbotAI;
struct Monitor {
    std::unique_ptr<PerformanceMonitorOperation> start(int,const std::string& name,PlayerbotAI*) {
        ++counters.pmo; AUDIT_MARK("pmo:"+name); return std::make_unique<PerformanceMonitorOperation>(name);
    }
};
inline Monitor sPerformanceMonitor;
enum class BotState { BOT_STATE_NON_COMBAT };
struct Position { bool valid=false; explicit operator bool() const { return valid; } float getX() { return 1; } float getY() { return 2; } };
struct LastMovement { Position lastMoveShort; };
struct Bot { float GetPositionX(){return 10;} float GetPositionY(){return 20;} };
class PlayerbotAI {
public:
    unsigned flags=0;
    bool HasStrategy(const std::string& name,BotState) {
        AUDIT_MARK("strategy:"+name);
        return name=="debug move" ? flags&1 : (name=="map" ? flags&2 : flags&4);
    }
    void Ping(float x,float y) { AUDIT_MARK("ping:"+std::to_string(x)+":"+std::to_string(y)); }
};
struct Travel { void manageNodes(Bot*,bool full) { AUDIT_MARK(full ? "manage:full" : "manage:normal"); } };
inline Travel sTravelNodeMap;
struct Event {};
struct AiNamedObject {
    PlayerbotAI* ai;
    std::string name;
    AiNamedObject(PlayerbotAI* ai,std::string name):ai(ai),name(std::move(name)) {}
    // Production PlayerbotAIAware has a virtual destructor. Keep that ownership
    // contract in this world-service fixture as well as the production policies.
    virtual ~AiNamedObject()=default;
    virtual std::string getName() { return name; }
};
#define time AuditTime
#include "policies.inc"
#undef time
inline std::string ObjectGuidListCalculatedValue::Format() { return "fixture"; }

class ListProbe : public ObjectGuidListCalculatedValue {
public:
    ListProbe(PlayerbotAI* ai,std::string name,int interval,unsigned size,unsigned id):ObjectGuidListCalculatedValue(ai,name,interval),size(size),id(id) { ++live; }
    ~ListProbe() override { --live; }
    inline static unsigned live=0;
    unsigned size,id;
    bool fail=false;
    std::function<void()> nested;
    std::list<ObjectGuid> Calculate() override {
        ++counters.calculations; AUDIT_MARK("calculate:"+name);
        struct Scope { Scope(){++calculateDepth;} ~Scope(){--calculateDepth;} } scope;
        if (nested) { auto call=std::move(nested); call(); }
        if(fail) throw std::runtime_error("world search failure");
        std::list<ObjectGuid> result;
        for(unsigned i=0;i<size;++i) result.push_back(id*100000+i/2); // retain duplicate GUIDs
        return result;
    }
    void Snapshot(std::vector<uint64_t>& out) const {
        out.push_back(lastCheckTime); out.push_back(value.size());
        for(auto guid:value) out.push_back(guid);
    }
};
// An unrecognized policy exercises Value<list>::GetSize's virtual Get fallback.
class UnknownList : public CalculatedValue<std::list<ObjectGuid>> {
public:
    ListProbe delegate;
    UnknownList(PlayerbotAI* ai,std::string name,int interval,unsigned size,unsigned id):CalculatedValue(ai,name,interval),delegate(ai,name,interval,size,id) {}
    std::list<ObjectGuid> Get() override { AUDIT_MARK("fallback:"+name); return delegate.Get(); }
    std::list<ObjectGuid> Calculate() override { return {}; }
    void Reset() override { delegate.Reset(); }
    void Set(std::list<ObjectGuid> value) override { delegate.Set(std::move(value)); }
};
class MovementValue : public UntypedValue, public Value<LastMovement&> {
public:
    LastMovement movement;
    MovementValue(PlayerbotAI* ai):UntypedValue(ai,"last movement") {}
    LastMovement& Get() override { return movement; }
    LastMovement& LazyGet() override { return movement; }
    void Set(LastMovement& value) override { movement=value; }
};
class Context {
public:
    std::map<std::string,UntypedValue*> values;
    UntypedValue* GetUntypedValue(const std::string& name) { ++counters.lookups; AUDIT_MARK("lookup:"+name); return values.at(name); }
#include "getvalue.inc"
};
class CheckValuesAction {
public:
    PlayerbotAI* ai;
    Bot* bot;
    Context* context;
    bool Execute_v19(Event&);
    bool Execute_v20(Event&);
};
