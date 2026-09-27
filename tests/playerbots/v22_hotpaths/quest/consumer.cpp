#include "state.h"
namespace AUDIT_NAMESPACE {
#include AUDIT_BODIES
bool NeedEntry(void* p,uint32 entry) {
    State& s=*static_cast<State*>(p); Bind(s);
    return s.context->GetValue<bool>("need for quest",std::to_string(entry))->Get();
}
Observation Edge(unsigned mode) {
    Case c;auto p=Create(c);auto& s=*static_cast<State*>(p);Bind(s);
    NeedQuestObjectiveValue objective(nullptr);objective.Qualify("{100,0}");
    observation={};recording=true;bool called=false;
    onTrace=[&](const std::string& text){
        if(called || text!="active:100")return;called=true;
        if(mode==0)objective.Qualify("{100,2}");
        if(mode==1)objective.Qualify("{100,reward}");
        if(mode==2)throw std::runtime_error("world callback");
        if(mode==3)Trace(std::string("nested:")+(objective.Calculate()?"1":"0"));
    };
    try{observation.result=objective.Calculate();}
    catch(const std::runtime_error& e){Trace(std::string("exception:")+e.what());}
    onTrace={};recording=false;auto ret=observation;Destroy(p);return ret;
}
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
Api AUDIT_API() { return {AUDIT_LABEL,AUDIT_NAMESPACE::Create,AUDIT_NAMESPACE::Destroy,AUDIT_NAMESPACE::Run,AUDIT_NAMESPACE::Observe,AUDIT_NAMESPACE::Policy,AUDIT_NAMESPACE::Qualifier,AUDIT_NAMESPACE::GetQuestObjectiveQualifierInt,AUDIT_NAMESPACE::NeedEntry,AUDIT_NAMESPACE::Edge}; }
