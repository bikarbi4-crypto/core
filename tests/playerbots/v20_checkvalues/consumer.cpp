#include "services.h"
#define AI_VALUE(type,name) context->GetValue<type>(name)->Get()
#include "v19.inc"
#include "v20.inc"
#undef AI_VALUE

const char* lookupNames[]={"possible targets","all targets","nearest npcs","nearest corpses","nearest game objects no los","nearest friendly players"};
const char* pmoNames[]={"possible targets","all targets","nearest npcs","nearest corpses","nearest game objects","nearest friendly players"};
struct Fixture {
    PlayerbotAI ai;
    Bot bot;
    Context context;
    MovementValue movement{&ai};
    std::vector<std::unique_ptr<UntypedValue>> owned;
    std::array<ListProbe*,6> probes;
    CheckValuesAction action{&ai,&bot,&context};
    Event event;
    Fixture(unsigned size,bool fallback,unsigned flags) {
        assert(ListProbe::live==0);
        ai.flags=flags; movement.movement.lastMoveShort.valid=flags&8;
        context.values["last movement"]=&movement;
        for(unsigned i=0;i<6;++i) {
            int interval=i==4 ? 1 : 4;
            if(fallback) {
                auto p=std::make_unique<UnknownList>(&ai,pmoNames[i],interval,size,i+1);
                probes[i]=&p->delegate; context.values[lookupNames[i]]=p.get(); owned.push_back(std::move(p));
            } else {
                auto p=std::make_unique<ListProbe>(&ai,pmoNames[i],interval,size,i+1);
                probes[i]=p.get(); context.values[lookupNames[i]]=p.get(); owned.push_back(std::move(p));
            }
        }
    }
    ~Fixture() { owned.clear(); assert(ListProbe::live==0); }
    void Prepare(unsigned stage) {
        if(stage) for(auto p:probes) p->GetSize();
        if(stage==2) logicalTime+=2;
        if(stage==3) for(auto p:probes) p->Reset();
        if(stage==4) for(auto p:probes) p->Set({99,99,100});
        if(stage==5) { probes[1]->Reset(); probes[5]->Reset(); logicalTime+=1; }
        if(stage==6) { probes[0]->Reset(); probes[0]->nested=[this]{probes[1]->GetSize();}; }
        if(stage==7) { probes[2]->Reset(); probes[2]->fail=true; }
    }
    bool Run(bool candidate) { return candidate ? action.Execute_v20(event) : action.Execute_v19(event); }
};
Result Scenario(bool candidate,bool enabled,unsigned size,unsigned stage,bool fallback,unsigned flags) {
    countAllocations=false; trace=true; benchmark=false; events.clear(); logicalTime=10000;
    sPlayerbotAIConfig.perfMonEnabled=enabled;
    Fixture f(size,fallback,flags); f.Prepare(stage); events.clear(); counters={};
    Result result;
    countAllocations=true;
    try { result.result=f.Run(candidate); } catch(const std::runtime_error&) { result.threw=true; }
    countAllocations=false;
    assert(PerformanceMonitorOperation::live==0 && calculateDepth==0);
    result.counts=counters; result.events=events;
    for(auto p:f.probes) p->Snapshot(result.state);
    return result;
}
uint64_t Batch(bool candidate,bool enabled,unsigned size,unsigned stage,uint64_t iterations) {
    countAllocations=false; trace=false; benchmark=true; logicalTime=10000;
    sPlayerbotAIConfig.perfMonEnabled=enabled;
    uint64_t checksum=0;
    if(stage==0) {
        counters={};
        for(uint64_t i=0;i<iterations;++i) {
            Fixture f(size,false,0);
            checksum+=f.Run(candidate);
        }
        return checksum+counters.calculations+counters.clock+counters.pmo+counters.lookups;
    }
    Fixture f(size,false,0); f.Prepare(1); counters={};
    for(uint64_t i=0;i<iterations;++i) {
        if(stage==3) for(auto p:f.probes) p->Reset();
        if(stage==2) logicalTime+=2;
        checksum+=f.Run(candidate);
    }
    for(auto p:f.probes) checksum+=p->size;
    return checksum+counters.calculations+counters.clock+counters.pmo+counters.lookups;
}
