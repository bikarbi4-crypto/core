"""V21/current complete quest functions, Value/cache/RTTI and TravelMgr query.
Only game/world services and shared factory registration are fixtures.
No candidate algorithm is copied into the fixture. Pure parser fallback before
the candidate exists allows a separately recorded pre-edit baseline run.
"""
from pathlib import Path
import hashlib,json,re,subprocess,sys
ROOT=Path(__file__).resolve().parents[4]
P='src/game/PlayerBots/playerbot/'
BASE='6a32bb9c97d45166719a9b5a9d3e0f0c0c8ac4c7'
out=Path(sys.argv[1]);out.mkdir(parents=True,exist_ok=True)
manifest={'baseline':BASE,'sources':{},'substitutions':['game services','clock in trace build','shared factory registrations']}
def read(path,current=False):
 path=P+path
 text=(ROOT/path).read_text(encoding='utf-8-sig') if current else subprocess.check_output(['git','show',BASE+':'+path],cwd=ROOT).decode('utf-8-sig')
 manifest['sources'][('working:' if current else BASE+':')+path]=hashlib.sha256(text.encode()).hexdigest()
 return text
def block(text,sig,semi=False):
 start=text.index(sig);i=text.index('{',start)+1;depth=1
 mask=re.sub(r'//[^\n]*|/\*.*?\*/|"(?:\\.|[^"\\])*"',lambda m:' '*len(m[0]),text,flags=re.S)
 while depth:depth+=(mask[i]=='{')-(mask[i]=='}');i+=1
 return text[start:i]+(';' if semi else '')
def clean(s):return re.sub(r'^\s*#(?:include|pragma once).*$', '',s,flags=re.M)
for ns,current in [('v21',False),('v22',True)]:
 cpp=read('strategy/values/QuestValues.cpp',current)
 parts=['#pragma once','#include "services.h"',clean(read('PlayerbotAIAware.h',current)),
  clean(read('strategy/NamedObjectCache.h',current)),clean(read('strategy/NamedObjectContext.h',current)),
  clean(read('strategy/AiObject.h',current).split('// MACROS GO HERE')[0])]
 v=read('strategy/Value.h',current)
 parts+=['namespace ai {',v[v.index('    class UntypedValue'):v.index('    template<class T> class MemoryCalculatedValue')],
  block(v,'class BoolCalculatedValue',True),
  'class Action : public UntypedValue {}; class Strategy : public UntypedValue {}; class Trigger : public UntypedValue {};','}',
  clean(read('strategy/AiObjectContext.h',current).split('#define AI_VALUE')[0]),'namespace ai {']
 for cls,file in [('NeedForQuestValue','QuestValues.h'),('NeedQuestObjectiveValue','QuestValues.h'),('ItemDropListValue','LootValues.h'),('ItemVendorListValue','VendorValues.h')]:
  parts.append(block(read('strategy/values/'+file,current),'class '+cls,True))
 parts+=['#include "variant_services.h"',block(read('strategy/values/SharedValueContext.h',current),'class SharedObjectContext',True),
  'extern SharedObjectContext* sharedContext;','#define sSharedObjectContext (*sharedContext)',
  '#define AI_VALUE2(type,name,param) context->GetValue<type>(name,param)->Get()',
  '#define GAI_VALUE(type,name) sSharedObjectContext.GetValue<type>(name)->Get()',
  '#define GAI_VALUE2(type,name,param) sSharedObjectContext.GetValue<type>(name,param)->Get()',
  'int32 GetQuestObjectiveQualifierInt(const std::string&, uint32);','}']
 header=re.sub(r'\bnamespace ai\b','namespace '+ns,'\n'.join(parts)).replace('time(0)','audit_time(0)')
 (out/(ns+'.h')).write_text(header,encoding='utf-8')
 parser='int32 GetQuestObjectiveQualifierInt(const std::string& value, uint32 index) { return Qualified::getMultiQualifierInt(value,index,","); }'
 if 'int32 GetQuestObjectiveQualifierInt(' in cpp:parser=block(cpp,'int32 GetQuestObjectiveQualifierInt(')
 bodies=[parser]+[block(cpp,s) for s in ['bool NeedForQuestValue::Calculate()',
  'bool NeedQuestObjectiveValue::Calculate()','bool NeedQuestObjectiveValue::CanGetItemSomewhere(']]
 bodies += [block(read('TravelMgr.cpp',current),'DestinationList TravelMgr::GetDestinations('),
  block(read('strategy/values/LootValues.cpp',current),'std::list<int32> ItemDropListValue::Calculate()'),
  block(read('strategy/values/VendorValues.cpp',current),'std::list<int32> ItemVendorListValue::Calculate()')]
 body='\n\n'.join(bodies)
 for sig,event in [('bool NeedForQuestValue::Calculate()','Trace("need:"+getQualifier());'),
  ('bool NeedQuestObjectiveValue::Calculate()','Trace("calculate:"+getQualifier());'),
  ('DestinationList TravelMgr::GetDestinations(','TraceDestination(entries,onlyPossible,maxDistance);'),
  ('std::list<int32> ItemDropListValue::Calculate()','Trace("drop-calculate:"+getQualifier());'),
  ('std::list<int32> ItemVendorListValue::Calculate()','Trace("vendor-calculate:"+getQualifier());')]:
  at=body.index('{',body.index(sig))+1
  body=body[:at]+'\n#ifdef QUEST_TRACE\n'+event+'\n#endif\n'+body[at:]
 (out/(ns+'_bodies.inc')).write_text(body,encoding='utf-8')
(out/'provenance.json').write_text(json.dumps(manifest,indent=2))
# Bind the established deterministic GrindTarget fixture to the actual quest
# context through its Value service boundary, in its own C++ namespace.
fixture=(ROOT/'tests/playerbots/grind_fixture.h').read_text(encoding='utf-8-sig')
fixture=re.sub(r'^#include .*\n','',fixture,flags=re.M)
fixture=fixture.replace('T Get() { RecordValueRead(name); return value; }', '''T Get() { RecordValueRead(name);
    if constexpr(std::is_same_v<T,bool>) if(name.rfind("need for quest::",0)==0 && needHook) return needHook(uint32(std::stoul(name.substr(16))));
    return value; }''')
fixture=fixture.replace('u->entry=100+i%5','u->entry=77+i%5').replace('std::to_string(100+i)','std::to_string(77+i)')
h=read('strategy/values/GrindTargetValue.h',True);cpp=read('strategy/values/GrindTargetValue.cpp',True)
assert h==read('strategy/values/GrindTargetValue.h') and cpp==read('strategy/values/GrindTargetValue.cpp'),'Grind scoring changed'
body=cpp.split('using namespace ai;',1)[1]
marker='for (std::list<ObjectGuid>::iterator tIter = targets.begin();'
at=body.index('{',body.index(marker))+1
body=body[:at]+'\n#ifdef QUEST_TRACE\n++candidateVisits;\n#endif\n'+body[at:]
code='namespace current {\n'+block(h,'class GrindTargetValue',True)+body+'\n}'
fixture=fixture.replace('#undef AI_VALUE\n',code+'\n#undef AI_VALUE\n')
(out/'grind_fixture.inc').write_text(fixture,encoding='utf-8')
(out/'provenance.json').write_text(json.dumps(manifest,indent=2))
