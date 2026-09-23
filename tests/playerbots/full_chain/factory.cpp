#include AUDIT_HEADER
namespace AUDIT_NAMESPACE {
// Keep the exact production bases, members and virtual tables. Only live world
// initialization/formatting is replaced; no mock RTTI or mock lookup is used.
AiObject::AiObject(PlayerbotAI* ai) : PlayerbotAIAware(ai), bot(nullptr), context(nullptr), chat(nullptr) {}
std::string ObjectGuidListCalculatedValue::Format() { return "fixture"; }
AiObjectContext::AiObjectContext(PlayerbotAI* ai) : PlayerbotAIAware(ai) {}
void AiObjectContext::Update() {}
void AiObjectContext::Reset() { valueContexts.Reset(); }

template<class T> struct CalcBase;
template<> struct CalcBase<bool> { using type=BoolCalculatedValue; };
template<> struct CalcBase<uint8> { using type=Uint8CalculatedValue; };
template<> struct CalcBase<uint32> { using type=Uint32CalculatedValue; };
template<> struct CalcBase<float> { using type=FloatCalculatedValue; };
template<> struct CalcBase<Unit*> { using type=UnitCalculatedValue; };
template<> struct CalcBase<std::list<ObjectGuid>> { using type=ObjectGuidListCalculatedValue; };
template<class T> class Probe : public CalcBase<T>::type, public Qualified {
    using Base=typename CalcBase<T>::type;
    unsigned count;
public:
    Probe(const Case& c) : Base(nullptr,"audit",c.mode==Mode::refresh?0:3600), count(c.listSize) {}
    T Calculate() override { return Initial<T>(count); }
};
template<class T> class SingleProbe : public SingleCalculatedValue<T>, public Qualified {
    unsigned count;
public:
    SingleProbe(const Case& c) : SingleCalculatedValue<T>(nullptr,"audit"), count(c.listSize) {}
    T Calculate() override { return Initial<T>(count); }
};
template<class T> class MemoryProbe : public MemoryCalculatedValue<T>, public Qualified {
public:
    MemoryProbe(const Case& c) : MemoryCalculatedValue<T>(nullptr,"audit",c.mode==Mode::refresh?0:3600) { this->lastValue={}; }
    T Calculate() override { return Initial<T>(0); }
    bool EqualToLast(T value) override { return value==this->lastValue; }
};
template<class T> class ManualProbe : public ManualSetValue<T>, public Qualified {
public:
    ManualProbe(const Case& c) : ManualSetValue<T>(nullptr,Initial<T>(c.listSize),"audit") {}
};
template<class T> class LogProbe : public LogCalculatedValue<T>, public Qualified {
public:
    LogProbe(const Case& c) : LogCalculatedValue<T>(nullptr,"audit",c.mode==Mode::refresh?0:3600) { this->lastValue={}; }
    T Calculate() override { return Initial<T>(0); }
    bool EqualToLast(T value) override { return value==this->lastValue; }
};
template<class T> UntypedValue* MakeValue(const Case& c) {
    if(c.policy==Policy::manual) {
        if constexpr(std::is_same_v<T,bool>) return new BoolManualSetValue(nullptr,true,"audit");
        if constexpr(std::is_same_v<T,Unit*>) return new UnitManualSetValue(nullptr,&auditUnit,"audit");
        return new ManualProbe<T>(c);
    }
    if(c.policy==Policy::single) return new SingleProbe<T>(c);
    if constexpr(std::is_same_v<T,uint32> || std::is_same_v<T,float>) {
        if(c.policy==Policy::memory) return new MemoryProbe<T>(c);
        if(c.policy==Policy::logged) return new LogProbe<T>(c);
    }
    return new Probe<T>(c);
}
UntypedValue* MakeValueByKind(const Case& c) {
    switch(c.kind) {
    case Kind::boolean: return MakeValue<bool>(c);
    case Kind::u8: return MakeValue<uint8>(c);
    case Kind::u32: return MakeValue<uint32>(c);
    case Kind::floating: return MakeValue<float>(c);
    case Kind::unit: return MakeValue<Unit*>(c);
    case Kind::list: return MakeValue<std::list<ObjectGuid>>(c);
    }
    std::abort();
}
class Factory : public NamedObjectContext<UntypedValue> {
public:
    explicit Factory(const Case& c) {
        creators["audit"]=[c](PlayerbotAI*) { return MakeValueByKind(c); };
        for(unsigned i=0;i<128;++i) {
            auto key="filler-"+std::to_string(i);
            creators[key]=[c](PlayerbotAI*) { return MakeValueByKind(c); };
            Create(key,nullptr);
        }
    }
};
__declspec(noinline) AiObjectContext* MakeContext(const Case& c) {
    auto* context=new AiObjectContext(nullptr);
    context->AddShared(new Factory(c));
    return context;
}
}
