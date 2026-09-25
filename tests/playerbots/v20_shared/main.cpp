#include "playerbot/playerbot.h"
#include "playerbot/strategy/values/LootValues.h"
#include <atomic>
#include <chrono>
#include <iostream>
#include <cstdlib>
#include <thread>
#include <stdexcept>
#include <functional>
#include <algorithm>
#include <iomanip>
#include <Windows.h>
using Creators=std::map<std::string,std::function<ai::UntypedValue*(PlayerbotAI*)>>;
void InstallFixtureCreators(Creators&);
#include SHARED_HEADER

namespace {
std::atomic<unsigned> checks{0}, factories{0}, destroyed{0}, calculates{0};
thread_local unsigned calculateMode=0;
thread_local ai::SharedObjectContext* current=nullptr;
bool ownerChecks=false;
void require(bool ok) { if(!ok) throw std::runtime_error("shared contract failure"); ++checks; }
class Probe final : public ai::SingleCalculatedValue<int>, public ai::Qualified {
public:
    Probe(PlayerbotAI* ai):SingleCalculatedValue(ai,"probe") { ++factories; }
    ~Probe() override {
        if(ownerChecks) {
            require(ai->GetBot()==nullptr);
            require(ai->GetAiObjectContext()==nullptr);
            require(ai->GetChatHelper()==chat);
        }
        ++destroyed;
    }
    int Calculate() override {
        ++calculates;
        if(calculateMode==1) { calculateMode=0; require(Get()==0); }
        if(calculateMode==2) throw std::runtime_error("calculate fixture");
        return 17;
    }
    bool Owned() const { return ai->GetBot()==nullptr && ai->GetAiObjectContext()==nullptr && ai->GetChatHelper()==chat; }
};
class TestContext : public ai::SharedObjectContext {
public:
    void ResetValues() { valueContexts.Reset(); }
    void Erase(const std::string& name) { valueContexts.Erase(name); }
    std::set<std::string> Keys() { return valueContexts.GetCreated(); }
    // Test-only baseline cleanup. V19's shared registry itself is borrowed and
    // leaked; erase cached fixture values after observations, never prolong AI.
    ~TestContext() {
#ifndef SHARED_CANDIDATE
        for(const auto& key:Keys()) Erase(key);
#endif
    }
};
}

void InstallFixtureCreators(Creators& creators) {
    creators["probe"]=[](PlayerbotAI* ai){return new Probe(ai);};
    creators["reentry"]=[](PlayerbotAI* ai)->ai::UntypedValue* {
        require(current!=nullptr);
        auto* other=current->GetValue<std::string>("global string","nested");
        other->Set("nested-ok");
        return new Probe(ai);
    };
    creators["throws"]=[](PlayerbotAI*)->ai::UntypedValue* { ++factories;throw std::runtime_error("factory fixture"); };
    creators["null"]=[](PlayerbotAI*)->ai::UntypedValue* {++factories;return nullptr;};
}

std::list<int32> fixtureDropCalculate() { ++calculates; return {11,22,33}; }

void repro() {
    sPlayerbotAIConfig.perfMonEnabled=true;
    ai::SharedObjectContext shared;
    auto* v=shared.GetValue<std::list<int32>>("item drop list",42);
    require(v->Get()==std::list<int32>{11,22,33});
}

void contracts(bool on) {
    sPerformanceMonitor.Reset();
    const auto initialFactories=factories.load(), initialDestroyed=destroyed.load(), initialCalculates=calculates.load();
    sPlayerbotAIConfig.perfMonEnabled=on;
    for(unsigned round=0;round<16;++round) {
        unsigned before=destroyed;
        {
            TestContext shared;current=&shared;
            auto* value=shared.GetValue<std::list<int32>>("item drop list",42);
            require(value!=nullptr);
            require(value==shared.GetValue<std::list<int32>>("item drop list","42"));
            require(shared.GetValue<int>("item drop list",42)==nullptr);
            unsigned n=calculates;
            require(value->Get()==std::list<int32>{11,22,33});require(calculates==n+1);
            require(value->Get()==std::list<int32>{11,22,33});require(calculates==n+1);
            require(!value->IsEmpty());require(calculates==n+1);
            require(dynamic_cast<ai::UntypedValue*>(value)->Expired(0));
            require(value->Get()==std::list<int32>{11,22,33});require(calculates==n+1);
            value->Set({});require(value->IsEmpty());require(calculates==n+1);
            value->Reset();require(!value->IsEmpty());require(calculates==n+2);
            require(value->LazyGet()==std::list<int32>{11,22,33});
            value->Set({7});require(value->Get()==std::list<int32>{7});
            shared.ResetValues();require(value->Get()==std::list<int32>{11,22,33});
            // Qualified byte handling remains in the original generic factory.
            for(auto key: {std::string(""),std::string("-2147483648"),std::string("2147483647"),std::string(90,'q'),std::string("a\0b",3)}) {
                auto* string=shared.GetValue<std::string>("global string",key);
                string->Set(key);require(string->Get()==key);
                require(shared.GetValue<std::string>("global string",key)==string);
            }
            auto* probe=dynamic_cast<Probe*>(shared.GetUntypedValue("probe::5"));require(probe!=nullptr);
#ifdef SHARED_CANDIDATE
            require(probe->Owned());
#endif
            require(probe->getQualifier()=="5");
            calculateMode=1;require(probe->Get()==17); // Reentry observes initialized storage.
            static_cast<ai::Value<int>*>(probe)->Reset();probe->Set(0);calculateMode=2;
            bool threw=false;try{probe->Get();}catch(const std::runtime_error&){threw=true;}
            require(threw);require(probe->Get()==0); // Preserve lastCheckTime after throw.
            calculateMode=0;static_cast<ai::Value<int>*>(probe)->Reset();require(probe->Get()==17);
            unsigned attempts=factories;
            for(unsigned i=0;i<2;++i){bool caught=false;try{shared.GetUntypedValue("throws");}catch(const std::runtime_error&){caught=true;}require(caught);}
            require(factories==attempts+2); // Exceptions are not negative-cache entries.
            attempts=factories;require(shared.GetUntypedValue("null")==nullptr);require(shared.GetUntypedValue("null")==nullptr);require(factories==attempts+1);
            require(shared.GetUntypedValue("missing")==nullptr);require(shared.GetUntypedValue("missing")==nullptr);
            require(shared.Keys().count("missing")==1);
            auto* nested=shared.GetValue<int>("reentry");require(nested->Get()==17);
            require(shared.GetValue<std::string>("global string","nested")->Get()=="nested-ok");
            // Toggle with a warm value, then reset and cold-read ON only in fixed variant.
            sPlayerbotAIConfig.perfMonEnabled=!on;require(value->Get()==std::list<int32>{11,22,33});
            sPlayerbotAIConfig.perfMonEnabled=on;
        }
        require(destroyed==before+2);current=nullptr;
    }
    std::map<std::string,uint32> recorded;
    for(const auto& [mapId,instances]:sPerformanceMonitor.mapsData)
        for(const auto& [instanceId,metrics]:instances)
            for(const auto& [metric,names]:metrics)
                for(const auto& [stack,data]:names) {
                    require(mapId==0 && instanceId==0 && metric==PERF_MON_VALUE && stack.size()==1);
                    recorded[stack.front()]+=data.count;
                }
    require(recorded["item drop list"]==(on?48:0));
    require(recorded["probe"]==(on?64:0));
    for(const auto& [name,count]:recorded)require(name=="item drop list" || name=="probe" || count==0);
    std::cout<<"contract_trace,"<<on<<","<<factories-initialFactories<<","<<destroyed-initialDestroyed<<","<<calculates-initialCalculates<<","<<recorded["item drop list"]<<","<<recorded["probe"]<<"\n";
}

void unwind() {
#ifdef SHARED_CANDIDATE
    const auto n=destroyed.load();
    try {TestContext shared;shared.GetUntypedValue("probe::unwind");throw std::runtime_error("owner scope");}
    catch(const std::runtime_error&){}
    require(destroyed==n+1);
    TestContext shared;
    auto* original=dynamic_cast<Probe*>(shared.GetUntypedValue("probe::erase"));require(original->Owned());
    shared.Erase("probe::erase");require(destroyed==n+2);
    auto* next=dynamic_cast<Probe*>(shared.GetUntypedValue("probe::erase"));require(next->Owned());
#endif
}

void threads() {
#ifdef SHARED_CANDIDATE
    sPlayerbotAIConfig.perfMonEnabled=false;
    TestContext shared;
    std::atomic<bool> start{false};
    std::vector<std::thread> workers;
    std::array<ai::UntypedValue*,8> same{};
    const auto before=factories.load();
    for(unsigned t=0;t<8;++t) workers.emplace_back([&,t]{
        while(!start.load(std::memory_order_acquire))std::this_thread::yield();
        for(unsigned i=0;i<256;++i){
            auto* v=shared.GetUntypedValue("probe::shared");
            if(i==0)same[t]=v;else require(same[t]==v);
            require(shared.GetUntypedValue("missing"+std::to_string(t)+"::"+std::to_string(i))==nullptr);
            auto* unique=shared.GetValue<std::string>("global string",std::to_string(t));
            unique->Set("worker"+std::to_string(t)); // Different stored object per thread.
            require(unique->Get()=="worker"+std::to_string(t));
        }
    });
    start.store(true,std::memory_order_release);for(auto& t:workers)t.join();
    require(factories==before+1);for(auto* p:same)require(p==same[0]);
    auto* list=shared.GetValue<std::list<int32>>("item drop list",77);require(!list->IsEmpty());
    // Concurrent already-published immutable reads. No racing Reset/Set/Calculate
    // is declared safe by this registry-only change.
    workers.clear();sPlayerbotAIConfig.perfMonEnabled=true;
    for(unsigned t=0;t<8;++t)workers.emplace_back([&]{for(unsigned i=0;i<512;++i){auto* v=shared.GetValue<std::list<int32>>("item drop list",77);require(v==list);require(v->Get()==std::list<int32>{11,22,33});}});
    for(auto& t:workers)t.join();sPlayerbotAIConfig.perfMonEnabled=false;
#endif
}

void benchmark(std::string kind,unsigned iterations,bool on) {
    SetProcessAffinityMask(GetCurrentProcess(),1);
    sPlayerbotAIConfig.perfMonEnabled=false;
    TestContext shared;current=&shared;
    const std::string key=kind=="long_lookup" ? "item drop list::"+std::string(96,'x') : kind=="unknown_lookup" ? "absent" : "item drop list::42";
    auto* list=shared.GetValue<std::list<int32>>("item drop list",42);list->Get();
    shared.GetUntypedValue(key); // Warm registry/static helper maps before timing.
    sPlayerbotAIConfig.perfMonEnabled=on;
    if(on && (kind=="cold_get" || kind=="reset_get")) {
#ifndef SHARED_CANDIDATE
        throw std::runtime_error("V19 cold PMO ON is undefined: ASan-confirmed UAF; no timing");
#endif
    }
    uint64 checksum=0;
#ifdef SHARED_ALLOCATIONS
    SharedCounters::active=true;
#endif
    auto begin=std::chrono::steady_clock::now();
    for(unsigned i=0;i<iterations;++i) {
        if(kind=="warm_get") checksum+=shared.GetValue<std::list<int32>>("item drop list",42)->Get().size();
        else if(kind=="reset_get") {list->Reset();checksum+=shared.GetValue<std::list<int32>>("item drop list",42)->Get().size();}
        else if(kind=="cold_get") {shared.Erase("item drop list::42");checksum+=shared.GetValue<std::list<int32>>("item drop list",42)->Get().size();}
        else checksum+=shared.GetUntypedValue(key)!=nullptr;
    }
    double ns=std::chrono::duration<double,std::nano>(std::chrono::steady_clock::now()-begin).count()/iterations;
#ifdef SHARED_ALLOCATIONS
    SharedCounters::active=false;
    std::cout<<SHARED_VARIANT<<","<<kind<<","<<on<<","<<iterations<<","<<SharedCounters::allocations<<","<<SharedCounters::bytes<<","<<SharedCounters::aiConstructions<<","<<checksum<<"\n";
#else
    std::cout<<SHARED_VARIANT<<","<<kind<<","<<on<<","<<iterations<<","<<std::setprecision(12)<<ns<<","<<checksum<<"\n";
#endif
}

int main(int argc,char** argv)
{
    try {
        if(argc>1 && std::string(argv[1])=="--repro"){repro();return 0;}
        if(argc>1 && std::string(argv[1])=="--bench"){benchmark(argv[2],std::stoul(argv[3]),std::stoi(argv[4])!=0);return 0;}
#ifdef SHARED_CANDIDATE
        ownerChecks=true;
#endif
        contracts(false);
#ifdef SHARED_CANDIDATE
        contracts(true);unwind();threads();
#endif
        std::cout<<SHARED_VARIANT<<" contracts passed checks="<<checks<<", factories="<<factories<<", destroyed="<<destroyed<<", calculates="<<calculates<<", sizeof(PlayerbotAI)="<<sizeof(PlayerbotAI)<<"\n";
        return 0;
    }catch(const std::exception& e){std::cerr<<e.what()<<"\n";return 1;}
}
