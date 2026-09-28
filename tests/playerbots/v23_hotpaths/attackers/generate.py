"""Complete Attackers bodies + real Value policies, with explicit world fixtures."""
from pathlib import Path
import hashlib,json,re,subprocess,sys
R=Path(__file__).resolve().parents[4];BASE='24e11eb41b85ab74437dfc1737289c2bcca42de4'
P='src/game/PlayerBots/playerbot/'
out=Path(sys.argv[1]);out.mkdir(parents=True,exist_ok=True)
manifest={'baseline':BASE,'variants':{},'sources':{},'production':['complete AttackersValue.cpp','AttackersValue declarations','Qualified','Value/CalculatedValue/ManualSetValue/ObjectGuidList policies','NearestUnitsValue and PossibleTargetsValue declarations','PossibleTargets Calculate/FindUnits/AcceptUnit'],'world_fixtures':['unit/player/group/position services','deterministic grid query and possible-target validity','typed registry factory adapter','clock and PMO sink'],'instrumentation':'trace/counters compile out of timing binary'}
def read(path,current=False):
 full=P+path;s=(R/full).read_text(encoding='utf-8-sig') if current else subprocess.check_output(['git','show',BASE+':'+full],cwd=R).decode('utf-8-sig')
 manifest['sources'][('working:' if current else BASE+':')+full]=hashlib.sha256(s.encode()).hexdigest();return s
def block(s,sig,semi=False):
 start=s.index(sig);i=s.index('{',start)+1;depth=1
 mask=re.sub(r'//[^\n]*|/\*.*?\*/|"(?:\\.|[^"\\])*"',lambda m:' '*len(m[0]),s,flags=re.S)
 while depth:depth+=(mask[i]=='{')-(mask[i]=='}');i+=1
 return s[start:i]+(';' if semi else '')
for ns,current in [('v22',False),('v23',True)]:
 cpp=read('strategy/values/AttackersValue.cpp',current)
 manifest['variants'][ns]={'source_sha256':hashlib.sha256(cpp.encode()).hexdigest()}
 v=read('strategy/Value.h',current);q=read('strategy/NamedObjectContext.h',current)
 policies=v[v.index('    class UntypedValue'):v.index('    template<class T> class MemoryCalculatedValue')]
 policies+=block(v,'class ObjectGuidListCalculatedValue',True)
 policies+=block(v,'template<class T>\n    class ManualSetValue',True)
 policies=policies.replace('time(0)','AuditTime(0)')
 policies=policies.replace('virtual T Get() override\n        {\n            RefreshValue();','virtual T Get() override\n        {\n            ObserveGet(this->name);\n            RefreshValue();')
 policies=policies.replace('virtual T Get() override { return value; }','virtual T Get() override { ObserveGet(this->name); return value; }')
 (out/(ns+'-policies.inc')).write_text(block(q,'class Qualified',True)+'\n'+policies+'\n',encoding='utf-8')
 declarations='\n'.join([block(read('strategy/values/NearestUnitsValue.h',current),'class NearestUnitsValue',True),
  block(read('strategy/values/PossibleTargetsValue.h',current),'class PossibleTargetsValue',True),
  block(read('strategy/values/AttackersValue.h',current),'class AttackersValue',True),
  block(read('strategy/values/AttackersValue.h',current),'class AttackersTargetingMeValue',True)])
 (out/(ns+'-declarations.inc')).write_text(declarations,encoding='utf-8')
 body=re.sub(r'^#include[^\n]*\n|^using namespace[^\n]*\n','',cpp,flags=re.M)
 for sig,label in [('std::list<ObjectGuid> AttackersValue::Calculate()','attackers.calculate'),('void AttackersValue::AddTargetsOf(Player*','member.calculate')]:
  at=body.index('{',body.index(sig))+1;body=body[:at]+'\n Mark("'+label+'",bot->id);\n'+body[at:]
 for loop in ['for (const ObjectGuid& guid : possibleTargets)','for (Unit* unit : units)','for (auto& guid : result)']:
  at=body.index('{',body.index(loop))+1;body=body[:at]+'\n Count(Counters::iterations);\n'+body[at:]
 p=read('strategy/values/PossibleTargetsValue.cpp',current)
 body+='\n'+ '\n'.join(block(p,sig) for sig in ['std::list<ObjectGuid> PossibleTargetsValue::Calculate()','void PossibleTargetsValue::FindUnits(','bool PossibleTargetsValue::AcceptUnit('])
 at=body.index('{',body.index('std::list<ObjectGuid> PossibleTargetsValue::Calculate()'))+1
 body=body[:at]+'\n Mark("possible.calculate",bot->id);\n'+body[at:]
 (out/(ns+'-bodies.inc')).write_text(body,encoding='utf-8')
(out/'provenance.json').write_text(json.dumps(manifest,indent=2),encoding='utf-8')

