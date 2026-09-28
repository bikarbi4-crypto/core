inline std::string ObjectGuidListCalculatedValue::Format(){return "fixture";}
struct Context {
 PlayerbotAI* owner;std::map<std::string,std::unique_ptr<UntypedValue>> values;
 explicit Context(PlayerbotAI* owner):owner(owner){}
 UntypedValue* GetUntypedValue(const std::string&);
 template<class T> Value<T>* GetValue(const std::string& name){
  auto* value=dynamic_cast<Value<T>*>(GetUntypedValue(name));assert(value);return value;
 }
 template<class T> Value<T>* GetValue(const std::string& name,const std::string& qualifier){return GetValue<T>(name+"::"+qualifier);}
 bool HasValue(const std::string& name){Count(Counters::lookups);Mark("has",owner->bot->id,&name);return values.count(name)!=0;}
 template<class T> void Put(const std::string& key,T value){values[key]=std::make_unique<ManualSetValue<T>>(owner,value,key);}
};
using AiObjectContext=Context;
#define AI_VALUE(type,name) context->GetValue<type>(name)->Get()
#define AI_VALUE2(type,name,param) context->GetValue<type>(name,param)->Get()
#define PAI_VALUE(type,name) player->GetPlayerbotAI()->GetAiObjectContext()->GetValue<type>(name)->Get()
#define PAI_VALUE2(type,name,param) player->GetPlayerbotAI()->GetAiObjectContext()->GetValue<type>(name,param)->Get()
#define PHAS_AI_VALUE(name) player->GetPlayerbotAI()->GetAiObjectContext()->HasValue(name)

