#include AUDIT_HEADER
#include <stdexcept>
namespace AUDIT_NAMESPACE {
struct State {
    Case input;
    std::unique_ptr<AiObjectContext> context;
    std::string name="audit", qualifier="qualified target name::17";
    UntypedValue* volatile untyped=nullptr;
};
template<class T> uint64 Observe(T value) { return static_cast<uint64>(value); }
template<> uint64 Observe<Unit*>(Unit* value) { return value!=nullptr; }
template<> uint64 Observe<std::list<ObjectGuid>>(std::list<ObjectGuid> value) { return value.size(); }
template<class T> __declspec(noinline) uint64 RunTyped(State& state,uint64 iterations) {
    uint64 result=0;
    const auto& c=state.input;
    auto* ctx=state.context.get();
    for(uint64 i=0;i<iterations;++i) {
        if(c.stage==Stage::lookup) {
            result+=ctx->GetUntypedValue(state.name)!=nullptr;
            continue;
        }
        Value<T>* value=nullptr;
        if(c.stage==Stage::cast) value=dynamic_cast<Value<T>*>(state.untyped);
        else if(c.mode==Mode::qualified) value=ctx->GetValue<T>(state.name,state.qualifier);
        else if(c.mode==Mode::wrong_type) {
            result+=ctx->GetValue<std::string>(state.name)!=nullptr;
            continue;
        }
        else value=ctx->GetValue<T>(state.name);
        if(!value) continue;
        if(c.stage==Stage::cast) { ++result; continue; }
        if(c.mode==Mode::refresh && c.policy==Policy::single) value->Reset();
        if constexpr(std::is_same_v<T,std::list<ObjectGuid>>) {
            if(c.operation==Operation::size) {
#if AUDIT_METADATA
                result+=value->GetSize();
#else
                result+=value->Get().size();
#endif
            } else if(c.operation==Operation::empty) {
#if AUDIT_METADATA
                result+=value->IsEmpty();
#else
                result+=value->Get().empty();
#endif
            } else result+=Observe(value->Get());
        } else result+=Observe(value->Get());
    }
    return result;
}
uint64 Run(void* pointer,uint64 n) {
    auto& state=*static_cast<State*>(pointer);
    switch(state.input.kind) {
    case Kind::boolean: return RunTyped<bool>(state,n);
    case Kind::u8: return RunTyped<uint8>(state,n);
    case Kind::u32: return RunTyped<uint32>(state,n);
    case Kind::floating: return RunTyped<float>(state,n);
    case Kind::unit: return RunTyped<Unit*>(state,n);
    case Kind::list: return RunTyped<std::list<ObjectGuid>>(state,n);
    }
    std::abort();
}
void* Create(const Case& c) {
    auto* state=new State;
    state->input=c;
    state->context.reset(MakeContext(c));
    if(c.mode==Mode::missing) state->name="missing value";
    state->untyped=state->context->GetUntypedValue(state->name);
    Run(state,1);
    return state;
}
void Destroy(void* pointer) { delete static_cast<State*>(pointer); }
}
Variant AUDIT_API() { return {AUDIT_LABEL,AUDIT_NAMESPACE::Create,AUDIT_NAMESPACE::Destroy,AUDIT_NAMESPACE::Run}; }
