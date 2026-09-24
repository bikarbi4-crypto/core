#include "state.h"
namespace AUDIT_NAMESPACE {
#include AUDIT_BODIES
__declspec(noinline) std::uint64_t Run(void* opaque,std::uint64_t iterations) {
    State& s=*static_cast<State*>(opaque); Bind(s);
    std::uint64_t total=0;
    for(std::uint64_t i=0;i<iterations;++i) {
        if(s.scenario.refresh) s.context->Reset();
        if(s.scenario.sharedCold) s.dropValue->Reset();
        total+=s.scenario.itemOnly?NeedQuestObjectiveValue::CanGetItemSomewhere(500,1,&s.player):s.need->Calculate();
    }
    return total;
}
Observation Observe(void* p,unsigned step) {
    State& s=*static_cast<State*>(p); Bind(s); SetWorld(s.scenario); sTravelMgr.destinationMap=s.destinationMap;
    fixtureTime=10000+(step==2?3:0);
    if(step==3) s.context->Reset();
    if(step==4 && !s.player.status.empty()) { s.player.status.begin()->second.m_itemcount[0]=1; s.player.status.begin()->second.m_creatureOrGOcount[0]=1; s.context->Reset(); }
    observation={}; recording=true;
    observation.result=Run(p,1)!=0; recording=false;
    return observation;
}
std::string Qualifier(uint32 q,unsigned o) {
#if AUDIT_B
    const std::string objectiveQualifierPrefix="{"+std::to_string(q)+",";
    return objectiveQualifierPrefix+std::to_string(o)+"}";
#else
    return Qualified::MultiQualify({std::to_string(q),std::to_string(o)},",");
#endif
}
}
Api AUDIT_API() { return {AUDIT_LABEL,AUDIT_NAMESPACE::Create,AUDIT_NAMESPACE::Destroy,AUDIT_NAMESPACE::Run,AUDIT_NAMESPACE::Observe,AUDIT_NAMESPACE::Policy,AUDIT_NAMESPACE::Qualifier}; }
