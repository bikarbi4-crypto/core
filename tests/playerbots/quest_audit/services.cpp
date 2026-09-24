#include "services.h"
ObjectMgr sObjectMgr; AuditConfig sPlayerbotAIConfig; AuditMonitor sPerformanceMonitor;
time_t fixtureTime=10000; Observation observation; bool recording=false;
std::function<void(const std::string&)> onTrace;
DropMap fixtureDrops; VendorMap fixtureVendors; Player* fixturePlayer=nullptr;
#ifdef QUEST_TRACE
void Trace(const std::string& s) {
    if(recording) observation.trace.push_back(s);
    if(onTrace) onTrace(s);
}
#endif
time_t audit_time(time_t* t) {
#ifdef QUEST_TRACE
    if(t) *t=fixtureTime;
    return fixtureTime;
#else
    return std::time(t);
#endif
}
void TraceDestination(const std::vector<int32>& entries,bool possible,float distance) {
    assert(entries.size()==1 && !possible && distance==0);
    if(recording) observation.destinations.push_back(entries.front());
    Trace("dest:"+std::to_string(entries.front()));
}
QuestStatusMap& Player::GetQuestStatusMap() { Trace("status-map"); return status; }
bool Player::IsActiveQuest(uint32 q) const { Trace("active:"+std::to_string(q)); auto i=status.find(q); return i!=status.end() && !i->second.m_rewarded; }
uint32 Player::GetMoney() const { Trace("money"); return money; }
PlayerTravelInfo::PlayerTravelInfo(Player*) { Trace("travel-snapshot"); }
const Quest* ObjectMgr::GetQuestTemplate(uint32 q) const { Trace("template:"+std::to_string(q)); auto i=quests.find(q); return i==quests.end()?nullptr:&i->second; }
const ItemPrototype* ObjectMgr::GetItemPrototype(uint32) const { Trace("prototype"); return prototype?&item:nullptr; }
PerformanceMonitorOperation::~PerformanceMonitorOperation() { Trace("pmo-end"); }
std::unique_ptr<PerformanceMonitorOperation> AuditMonitor::start(int,const std::string& s,PlayerbotAI*) { Trace("pmo:"+s); return std::make_unique<PerformanceMonitorOperation>(); }
float TravelDestination::DistanceTo(const WorldPosition&) const {
#ifdef QUEST_TRACE
    if(recording) ++observation.distanceCalls;
#endif
    return distance;
}
void SetWorld(const Case& c) {
    sObjectMgr.quests.clear(); fixtureDrops.clear(); fixtureVendors.clear();
    sObjectMgr.prototype=c.prototype; sPlayerbotAIConfig.perfMonEnabled=c.monitor;
    for(unsigned i=0;i<c.dropSize;++i) fixtureDrops.emplace(500,1000+int(i));
    for(unsigned i=0;i<c.vendorSize;++i) fixtureVendors.emplace(500,2000+int(i));
    for(unsigned n=0;n<c.quests;++n) {
        Quest q; q.active=c.state!=2;
        for(unsigned o=0;o<c.objectives;++o) {
            if(c.kind==2) { q.ReqItemCount[o]=1; q.ReqItemId[o]=500; }
            else { q.ReqCreatureOrGOCount[o]=1; q.ReqCreatureOrGOId[o]=c.kind==1?-77:77; }
        }
        if(c.state!=3) sObjectMgr.quests.emplace(100+n,q);
    }
}
