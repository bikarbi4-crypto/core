#include AUDIT_HEADER
#include "inventory.h"
#define sPerformanceMonitor PerformanceMonitor::instance()

namespace AUDIT_NAMESPACE
{
Config sPlayerbotAIConfig;
thread_local PmoCounters counters;
thread_local std::vector<std::string> events;
thread_local uint64_t virtualTime=1000;
thread_local bool observe=false;
thread_local Context* activeContext=nullptr;
thread_local volatile uint64_t checksum=0;

void Mark(const std::string& name)
{
#ifdef PMO_TRACE
    if (observe) events.push_back(name);
#endif
}
__declspec(noinline) void Work(const char* name)
{
    checksum=checksum+7;
#ifdef PMO_TRACE
    ++counters.work;
    virtualTime+=3;
    std::string event=name;
    if (activeContext)
        for (const auto& frame: activeContext->performanceStack) event+="|"+frame;
    Mark(event);
#endif
}

#include AUDIT_SCOPES

PmoResult Snapshot(int result, const PlayerbotAI& ai)
{
    PmoResult r;
    r.result=result; r.counters=counters; r.events=events; r.stack=ai.context.performanceStack;
    r.maps=sPerformanceMonitor.mapsData.size();
    for (const auto& m: sPerformanceMonitor.mapsData)
        for (const auto& i: m.second)
            for (const auto& t: i.second)
                for (const auto& entry: t.second)
                {
                    std::string text=std::to_string(m.first)+":"+std::to_string(i.first)+":"+std::to_string(t.first);
                    for (const auto& frame: entry.first) text+="|"+frame;
                    const auto& d=entry.second;
                    text+="="+std::to_string(d.minTime)+","+std::to_string(d.maxTime)+","+std::to_string(d.totalTime)+","+std::to_string(d.count);
                    r.stats.push_back(text);
                }
    std::sort(r.stats.begin(),r.stats.end());
    return r;
}

void Prepare(PlayerbotAI& ai, bool enabled, unsigned map, unsigned instance)
{
    sPlayerbotAIConfig.perfMonEnabled=enabled;
    sPerformanceMonitor.mapsData.clear();
    sPerformanceMonitor.Init(0,0);
    sPerformanceMonitor.Init(map,instance);
    ai.bot.mapId=map; ai.bot.instanceId=instance;
    activeContext=&ai.context;
    counters={}; events.clear(); virtualTime=1000; checksum=0; observe=true;
}

PmoResult Scenario(int test, bool enabled, unsigned map, unsigned instance, unsigned flags)
{
    PlayerbotAI ai;
    Prepare(ai,enabled,map,instance);
    Action action{flags&32 ? "a" : "an action name beyond small string optimization",flags};
    Event event{flags&64 ? "" : "event source beyond small string optimization",bool(flags&128)};
    int result=0;
    if (test<scopeCount)
        scopes[test](&ai,flags&2 ? nullptr : &action,event);
    else if (test==scopeCount)
    {
        Engine engine{&ai,action};
        result=engine.ExecuteAction(action.name,event);
    }
    else if (test==scopeCount+1)
    {
        PlayerbotAIBase base;
        base.aiInternalUpdateDelay=flags&1 ? 1000 : 0;
        base.UpdateAI(20);
        Work("between FullTicks");
        base.UpdateAI(20);
        Work("after FullTick call");
        base.totalPmo.reset();
    }
    else if (test==49) // creation while OFF followed by dynamic ON
    {
        sPlayerbotAIConfig.perfMonEnabled=false;
        sPerformanceMonitor.mapsData.clear();
        sPerformanceMonitor.Init(map,instance);
        char command[]="toggle";
        ChatHandler().HandlePerfMonCommand(command);
        auto p=sPerformanceMonitor.start(PERF_MON_TOTAL,"late map",nullptr,map,instance);
        result=bool(p); Work("late map body"); p.reset();
    }
    else if (test==50) // ON -> OFF at destruction -> ON
    {
        auto p=sPerformanceMonitor.start(PERF_MON_ACTION,"parent",&ai);
        Work("parent body");
        sPlayerbotAIConfig.perfMonEnabled=false;
        p.reset();
        sPlayerbotAIConfig.perfMonEnabled=true;
        auto next=sPerformanceMonitor.start(PERF_MON_VALUE,"next",&ai);
        Work("next body"); next.reset();
    }
    else if (test==51) // repeated nested name must retain parent after child
    {
        auto parent=sPerformanceMonitor.start(PERF_MON_ACTION,"same",&ai);
        auto child=sPerformanceMonitor.start(PERF_MON_ACTION,"same",&ai);
        Work("child body"); child.reset();
        result=int(ai.context.performanceStack.size());
        auto sibling=sPerformanceMonitor.start(PERF_MON_TRIGGER,"sibling",&ai);
        Work("sibling body"); sibling.reset(); parent.reset();
    }
    else if (test==52) // mixed nested teardown with a toggle between destructors
    {
        auto parent=sPerformanceMonitor.start(PERF_MON_ACTION,"same",&ai);
        auto child=sPerformanceMonitor.start(PERF_MON_VALUE,"same",&ai);
        auto inner=sPerformanceMonitor.start(PERF_MON_TRIGGER,"inner",&ai);
        Work("nested body");
        sPlayerbotAIConfig.perfMonEnabled=false;
        inner.reset(); child.reset();
        result=int(ai.context.performanceStack.size());
        sPlayerbotAIConfig.perfMonEnabled=true;
        Work("parent resumed"); parent.reset();
    }
    else if (test==53) // ON -> OFF -> ON while one scope remains alive
    {
        auto p=sPerformanceMonitor.start(PERF_MON_ACTION,"spans toggle",&ai);
        Work("on"); sPlayerbotAIConfig.perfMonEnabled=false;
        Work("off"); sPlayerbotAIConfig.perfMonEnabled=true;
        Work("on again"); p.reset();
    }
    else if (test==54) // existing non-LIFO teardown of distinct names
    {
        auto parent=sPerformanceMonitor.start(PERF_MON_ACTION,"outer",&ai);
        auto child=sPerformanceMonitor.start(PERF_MON_VALUE,"inner",&ai);
        parent.reset(); Work("inner only"); child.reset();
    }
    else if (test==55) // Reset keeps active PerformanceData references valid
    {
        auto p=sPerformanceMonitor.start(PERF_MON_ACTION,"reset alive",&ai);
        Work("before Reset"); sPerformanceMonitor.Reset();
        Work("after Reset"); p.reset();
    }
    else if (test==56)
    {
        ai.withContext=false;
        auto p=sPerformanceMonitor.start(PERF_MON_RNDBOT,"no context",&ai);
        Work("fallback"); p.reset();
    }
    else if (test==57)
    {
        try
        {
            auto a=sPerformanceMonitor.start(PERF_MON_ACTION,"throw",&ai);
            auto b=sPerformanceMonitor.start(PERF_MON_VALUE,"throw",&ai);
            sPlayerbotAIConfig.perfMonEnabled=!(flags&1);
            throw 42;
        }
        catch (int n) { result=n; }
    }
    else if (test==58)
    {
        sPerformanceMonitor.mapsData.clear();
        for (unsigned i=0;i<4;++i)
        {
            auto a=sPerformanceMonitor.start(PERF_MON_TOTAL,"instance",nullptr,map,instance+i);
            result+=bool(a); Work("instance body");
        }
    }
    else if (test==59 || test==60)
    {
        PlayerbotAIBase base;
        sPlayerbotAIConfig.perfMonEnabled=test==60;
        base.UpdateAI(20);
        Work("between FullTicks");
        sPlayerbotAIConfig.perfMonEnabled=!sPlayerbotAIConfig.perfMonEnabled;
        base.UpdateAI(20);
        Work("after toggle");
        base.totalPmo.reset();
    }
    else if (test==61)
    {
        for (auto metric:{PERF_MON_VALUE,PERF_MON_TRIGGER,PERF_MON_ACTION,PERF_MON_RNDBOT,PERF_MON_TOTAL})
        {
            sPlayerbotAIConfig.perfMonEnabled=true;
            auto p=sPerformanceMonitor.start(metric,"toggle type",&ai);
            Work("metric body");
            sPlayerbotAIConfig.perfMonEnabled=false;
            auto before=counters.clocks;
            p.reset();
#ifdef PMO_TRACE
            assert(counters.clocks==before);
#endif
        }
    }
    auto r=Snapshot(result,ai); activeContext=nullptr; observe=false;
    return r;
}

uint64_t Batch(int test, bool enabled, unsigned map, unsigned instance, unsigned flags, uint64_t iterations)
{
    PlayerbotAI ai; Prepare(ai,enabled,map,instance); observe=false;
    Action action{flags&32 ? "a" : "an action name beyond small string optimization",flags};
    Event event{flags&64 ? "" : "event source beyond small string optimization",bool(flags&128)};
    Engine engine{&ai,action}; PlayerbotAIBase base;
    for (uint64_t i=0;i<iterations;++i)
    {
        if (test<scopeCount) scopes[test](&ai,&action,event);
        else if (test==scopeCount) checksum+=engine.ExecuteAction(action.name,event);
        else base.UpdateAI(20);
    }
    base.totalPmo.reset();
    activeContext=nullptr;
    return checksum;
}

void RegistryThreads()
{
    // Flag remains stable. Exercise concurrent lazy insertion, not an unsupported
    // unsynchronised change to the existing global configuration bool.
    sPlayerbotAIConfig.perfMonEnabled=true;
    sPerformanceMonitor.mapsData.clear();
    std::vector<std::thread> threads;
    for (unsigned t=0;t<4;++t)
        threads.emplace_back([t] {
            for (unsigned i=0;i<128;++i)
            {
                auto p=sPerformanceMonitor.start(PERF_MON_TOTAL,"new map",nullptr,100+t,i);
                assert(p); Work("parallel body");
            }
        });
    for (auto& t:threads) t.join();
    assert(sPerformanceMonitor.mapsData.size()==4);
    for (const auto& m:sPerformanceMonitor.mapsData) assert(m.second.size()==128);
}
}

extern const PmoApi AUDIT_API={AUDIT_LABEL,&AUDIT_NAMESPACE::Scenario,&AUDIT_NAMESPACE::Batch,&AUDIT_NAMESPACE::RegistryThreads};
