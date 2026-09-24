#include "state.h"
namespace AUDIT_NAMESPACE {
SharedObjectContext* sharedContext=nullptr;
TravelMgr sTravelMgr;
static AiObjectContext* constructingContext=nullptr;
AiObject::AiObject(PlayerbotAI* ai) : PlayerbotAIAware(ai),bot(fixturePlayer),context(constructingContext),chat(nullptr) {}
AiObjectContext::AiObjectContext(PlayerbotAI* ai) : PlayerbotAIAware(ai) {}
void AiObjectContext::Update() {}
void AiObjectContext::Reset() { valueContexts.Reset(); }
class ObjectiveFactory : public NamedObjectContext<UntypedValue> {
public:
    ObjectiveFactory() { creators["need quest objective"]=[](PlayerbotAI* ai) { return new NeedQuestObjectiveValue(ai); }; }
};
#ifdef QUEST_TRACE
class TraceContext : public AiObjectContext {
public:
    TraceContext() : AiObjectContext(nullptr) {}
    UntypedValue* GetUntypedValue(const std::string& n) override { Trace("lookup:"+n); return AiObjectContext::GetUntypedValue(n); }
};
#endif
void Bind(State& s) { fixturePlayer=&s.player; constructingContext=s.context.get(); sharedContext=s.shared.get(); }
__declspec(noinline) void* Create(const Case& c) {
    recording=false; fixtureTime=10000; SetWorld(c);
    auto s=std::make_unique<State>(); s->scenario=c; s->player.money=c.money;
    for(unsigned n=0;n<c.quests;++n) {
        QuestStatusData q; if(c.state==1) q.m_status=QUEST_STATUS_COMPLETE;
        for(unsigned o=0;o<4;++o) q.m_itemcount[o]=q.m_creatureOrGOcount[o]=(c.incompleteMask&(1<<o))?0:1;
        if(c.state!=4) s->player.status.emplace(100+n,q);
    }
    fixturePlayer=&s->player;
#ifdef QUEST_TRACE
    s->context=std::make_unique<TraceContext>();
#else
    s->context=std::make_unique<AiObjectContext>(nullptr);
#endif
    s->context->AddShared(new ObjectiveFactory());
    s->shared=std::make_unique<SharedObjectContext>(); Bind(*s);
    if(c.sharedCold) s->dropValue=s->shared->GetValue<std::list<int32>>("item drop list",500);
    s->need=std::make_unique<NeedForQuestValue>(nullptr); s->need->Qualify(c.kind==1?"-77":"77");
    for(unsigned n=0;n<c.quests+20;++n) {
        for(unsigned i=0;i<c.destinations;++i) {
            auto d=std::make_unique<TravelDestination>(); d->entry=1000+int(i);
            if(n==c.quests-1 && ((c.match==0 && i==0) || (c.match==1 && i+1==c.destinations))) d->entry=c.kind==1?-77:77;
            d->distance=c.unreachable?FLT_MAX:float(i+1);
            s->destinationMap[TravelDestinationPurpose::QuestObjective1][100+n].push_back(d.get());
            s->destinations.push_back(std::move(d));
        }
    }
    sTravelMgr.destinationMap=s->destinationMap;
    return s.release();
}
void Destroy(void* p) { recording=false; delete static_cast<State*>(p); sharedContext=nullptr; constructingContext=nullptr; fixturePlayer=nullptr; }

template<class Base> struct Probe : Base {
    using SingleCalculatedValue<std::list<int32>>::Reset;
    unsigned count=8, calculates=0;
    std::function<void()> callback;
    Probe() : Base(nullptr) { this->Qualify("500"); }
    std::list<int32> Calculate() override { ++calculates; if(callback) callback(); return std::list<int32>(count,42); }
    time_t Stamp() const { return this->lastCheckTime; }
};
template<class Base> void PolicyFor() {
    Probe<Base> before, after;
    for(bool monitor:{false,true}) {
        sPlayerbotAIConfig.perfMonEnabled=monitor;
        for(unsigned step=0;step<120;++step) {
            fixtureTime=step==0?0:10000+step;
            if(step%7==0) { before.Reset(); after.Reset(); }
            if(step%11==0) { before.Set({}); after.Set({}); }
            before.count=after.count=step%17;
            auto compare=[&] {
                std::vector<std::string> a,b;
                onTrace=[&](const std::string& s) { a.push_back(s); };
                bool oldEmpty=before.Get().empty();
                onTrace=[&](const std::string& s) { b.push_back(s); };
                bool newEmpty=after.IsEmpty(); onTrace={};
                assert(oldEmpty==newEmpty && a==b);
                assert(before.calculates==after.calculates); assert(before.Stamp()==after.Stamp());
            };
            compare(); compare();
            assert(before.Get()==after.Get());
            auto oldCopy=before.Get(); auto copy=after.Get(); oldCopy.clear(); copy.clear(); assert(before.Get()==after.Get());
            assert(before.LazyGet()==after.LazyGet());
        }
    }
    bool nestedBefore=false,nestedAfter=false;
    before.callback=[&] { nestedBefore=before.Get().empty(); };
    after.callback=[&] { nestedAfter=after.IsEmpty(); };
    fixtureTime=12000; before.Reset(); after.Reset();
    assert(before.Get().empty()==after.IsEmpty()); assert(nestedBefore==nestedAfter);
    before.callback=[&] { before.Reset(); }; after.callback=[&] { after.Reset(); };
    before.Reset(); after.Reset();
    for(int n=0;n<3;++n) assert(before.Get().empty()==after.IsEmpty());
    before.callback=[] { throw std::runtime_error("fixture"); }; after.callback=before.callback;
    before.Reset(); after.Reset();
    unsigned exceptions=0;
    try { before.Get(); } catch(const std::runtime_error&) { ++exceptions; }
    try { after.IsEmpty(); } catch(const std::runtime_error&) { ++exceptions; }
    assert(exceptions==2 && before.Stamp()==after.Stamp());
    assert(before.Get().empty()==after.IsEmpty()); // Same timestamp-on-throw behavior.
}
void Policy() { recording=false; PolicyFor<ItemDropListValue>(); PolicyFor<ItemVendorListValue>(); }
}
