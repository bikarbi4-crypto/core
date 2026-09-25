#include "variants.h"
#include <fstream>

static std::atomic<std::uint64_t> fakeNs{100000000000ULL}, clockCalls{0};
namespace PresenceDiagnostics { std::uint64_t TestClockNs() { ++clockCalls; return fakeNs.load(); } }
static thread_local std::uint64_t allocations = 0;
void* operator new(std::size_t n) { ++allocations; if (auto* p=std::malloc(n ? n : 1)) return p; throw std::bad_alloc(); }
void operator delete(void* p) noexcept { std::free(p); }
void operator delete(void* p, std::size_t) noexcept { std::free(p); }
static bool Has(std::vector<std::string> const& lines, std::string const& part)
{ for(auto const& line:lines) if(line.find(part)!=std::string::npos) return true; return false; }

int main()
{
    using namespace PresenceDiagnostics;
    static_assert(unsigned(candidate::ActivePiorityType::MAX_TYPE)==PriorityCount);
    static_assert(candidate::MAX_ACTIVITY_TYPE==ActivityCount && candidate::ALL_ACTIVITY==AllActivity);
    assert(!Enabled());
    assert(Start(5,false,false).find("duration")!=std::string::npos);
    assert(Start(60,false,true).find("refused")!=std::string::npos);
    // Completely OFF collector: zero allocations, clocks, publications or copies.
    auto alloc0=allocations, clocks0=clockCalls.load();
    BotState state;
    for(unsigned i=0;i<100000;++i)
    {
        PriorityProbe p; p.Scan(); p.Entry(); assert(p.Finish(17u)==17);
        DecisionProbe d; assert(!d); assert(d.Finish(true));
        assert(ActivityValue(90,0)==90); Flush();
    }
    assert(allocations==alloc0 && clockCalls==clocks0);
    unsigned cases=0;
    std::vector<std::string> evidence;
    for(unsigned mode=0;mode<3;++mode)
    {
        if(mode) assert(Start(3600,mode==2,false).find("started")!=std::string::npos);
        for(unsigned scene=0;scene<27;++scene)
        for(float a:{0.f,49.f,50.f,69.f,70.f,79.f,80.f,90.f,99.f,100.f})
        for(unsigned activity=1;activity<10;++activity)
        for(unsigned flags=0;flags<4;++flags)
        {
            baseline::Setup(scene); candidate::Setup(scene);
            baseline::env.activity=candidate::env.activity=a;
            baseline::sPlayerbotAIConfig.forceActiveWhenNearPlayer=candidate::sPlayerbotAIConfig.forceActiveWhenNearPlayer=(flags&1)!=0;
            baseline::sPlayerbotAIConfig.limitCombatActivity=candidate::sPlayerbotAIConfig.limitCombatActivity=(flags&2)!=0;
            baseline::sPlayerbotAIConfig.botActiveAlone=candidate::sPlayerbotAIConfig.botActiveAlone=flags%2 ? 10 : 100;
            baseline::PlayerbotAI b; candidate::PlayerbotAI c;
            assert(unsigned(b.GetPriorityType())==unsigned(c.GetPriorityType()));
            assert(baseline::env.calls==candidate::env.calls && baseline::env.rng==candidate::env.rng);
            baseline::env.calls.clear(); candidate::env.calls.clear();
            for(unsigned step=0;step<6;++step)
            {
                std::time_t const times[]={100,104,105,105,106,111};
                baseline::env.now=candidate::env.now=times[step];
                bool const forced=step==3;
                assert(b.AllowActivity(baseline::ActivityType(activity),forced)==c.AllowActivity(candidate::ActivityType(activity),forced));
                assert(baseline::env.calls==candidate::env.calls && baseline::env.rng==candidate::env.rng);
                for(unsigned k=0;k<10;++k)
                { assert(b.allowActive[k]==c.allowActive[k]); assert(b.allowActiveCheckTimer[k]==c.allowActiveCheckTimer[k]); }
                ++cases;
            }
        }
        if(mode) { auto lines=Stop("test"); evidence.insert(evidence.end(),lines.begin(),lines.end()); }
    }
    // Map-player definitions stay distinct, including selfbot/out-of-world/GM-like ordinary players.
    MapSample m;
    m.Player(true,true,true,12); m.Player(true,true,false,12); m.Player(false,false,true,99); m.Player(true,false,false,1);
    assert(m.entries==4 && m.realByPriority==2 && m.realByIsBot==2 && m.zoneCount==1 && m.zonePlayers[0]==2);
    for(unsigned zone=100;zone<140;++zone) m.Player(true,true,true,zone);
    assert(m.zoneCount==32 && m.droppedZones==9);

    assert(Start(60,false,false).find("started")!=std::string::npos);
    std::vector<std::thread> workers;
    for(unsigned w=0;w<8;++w) workers.emplace_back([w] {
        BotState bot;
        for(unsigned i=0;i<1000;++i)
        {
            DecisionProbe d; assert(d); d.Begin(w+100,bot,AllActivity,i%2 ? 16 : 17); d.Finish(i%2==0);
            Cache(w+100,bot,false,false,i%2==0,100+i);
        }
        // Publish explicitly after 5 seconds; all mutable data remains on its owner.
        fakeNs.fetch_add(5000000000ULL); Flush();
    });
    for(auto& w:workers) w.join();
    auto lines=Report();
    assert(Has(lines,"unique_observed_bots=8 "));
    assert(Has(lines,"ordinary_refreshes=8000 "));
    assert(Has(lines,"last_ordinary_refresh_unix_min=1099 last_ordinary_refresh_unix_max=1099"));
    assert(Has(lines,"from=17 to=16 calls=4000"));
    assert(Has(lines,"from=16 to=17 calls=3992"));
    evidence.insert(evidence.end(),lines.begin(),lines.end());
    // Timer expires through the ordinary world Poll; PMO toggles terminate, never mutate config.
    fakeNs.fetch_add(61000000000ULL);
    assert(Has(Poll(false),"reason=deadline") && !Enabled());
    Start(60,false,false); assert(Has(Poll(true),"reason=PMO_enabled") && !Enabled());
    // Bounded storage explicitly counts dropped observations.
    Start(60,false,false);
    for(unsigned i=0;i<MaxBotsPerThread+10;++i)
    { BotState bot; DecisionProbe d; d.Begin(i,bot,AllActivity,17); d.Finish(true); }
    lines=Stop("cap_test");
    assert(Has(lines,"unique_observed_bots=16384 ") && Has(lines,"dropped_bot_observations=10 "));
    Start(60,false,false);
    assert(Has(Stop("reset"),"unique_observed_bots=0 "));
    // Sequential map-owner transfer retains category/timestamp state; deduplicate GUID.
    Start(60,false,false);
    BotState transferred;
    std::thread first([&] {
        DecisionProbe d; d.Begin(777,transferred,AllActivity,17); d.Finish(true);
        Cache(777,transferred,false,false,true,1234); Flush();
    }); first.join();
    std::thread second([&] {
        DecisionProbe d; d.Begin(777,transferred,AllActivity,16); d.Finish(false);
        fakeNs.fetch_add(5000000000ULL); Flush();
    }); second.join();
    lines=Stop("transfer");
    assert(Has(lines,"unique_observed_bots=1 ") && Has(lines,"from=17 to=16 calls=1"));
    assert(Has(lines,"last_ordinary_refresh_unix_min=1234 last_ordinary_refresh_unix_max=1234"));
    Start(60,false,false);
    for(unsigned i=0;i<MaxMapsPerThread+3;++i)
    { MapSample sample; if(BeginMap(sample,0,i)) EndMap(sample); }
    assert(Has(Stop("map_cap"),"dropped_map_observations=3 "));
    Start(60,false,false);
    for(unsigned i=0;i<MaxThreads+2;++i)
    {
        std::thread worker([] { PriorityProbe p; p.Finish(17u); Flush(); }); worker.join();
    }
    lines=Stop("thread_cap");
    // Stop's calling thread also attempts registration after all worker slots filled.
    assert(Has(lines,"registered_threads=64 dropped_threads=3 "));
    std::ofstream output("collector-evidence.log");
    for(auto const& line:evidence) output<<line<<'\n';
    std::cout<<"PASS "<<cases<<" actual-body cache/decision parity observations in OFF/ON/timing; identical RNG, calls, cache state.\n"
             <<"PASS OFF allocation/clock checks, map definitions, 8-thread aggregate, transitions, expiry, PMO stop, bounds/reset.\n";
}
