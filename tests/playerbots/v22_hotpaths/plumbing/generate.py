from pathlib import Path
import re,json,hashlib,subprocess,sys
ROOT=Path(__file__).resolve().parents[4];P='src/game/PlayerBots/playerbot/'
BASE='6a32bb9c97d45166719a9b5a9d3e0f0c0c8ac4c7'
out=Path(sys.argv[1]);out.mkdir(parents=True,exist_ok=True)
manifest={'baseline':BASE,'sources':{},'substitutions':['ByteBuffer storage fixture','AiObject constructor fixture','unused Trigger target services','world services in isUseful','trace-only copy counter'], 'compiled_client':'MANGOSBOT_ZERO'}
def read(p,current=False):
 s=(ROOT/p).read_text(encoding='utf-8-sig') if current else subprocess.check_output(['git','show',BASE+':'+p],cwd=ROOT).decode('utf-8-sig')
 manifest['sources'][('working:' if current else BASE+':')+p]=hashlib.sha256(s.encode()).hexdigest();return s
def block(s,sig,semi=False):
 start=s.index(sig);i=s.index('{',start)+1;n=1
 m=re.sub(r'//[^\n]*|/\*.*?\*/|"(?:\\.|[^"\\])*"',lambda x:' '*len(x[0]),s,flags=re.S)
 while n:n+=(m[i]=='{')-(m[i]=='}');i+=1
 return s[start:i]+(';' if semi else '')
for ns,current in [('v21',False),('v22',True)]:
 parts=['#pragma once','#include "services.h"','namespace '+ns+' {']
 shared=read('src/game/SharedDefines.h',current)
 parts+=['struct WorldLocation;',block(shared,'struct Position\n',True),block(shared,'struct WorldLocation :',True)]
 wp=read(P+'WorldPosition.h',current)
 parts+=['class WorldPosition : public WorldLocation { public:',
  'void add() {} void rem() {}',
  'WorldPosition() : WorldLocation(0,0,0,0,0) { add(); }',
  'WorldPosition(const WorldPosition& pos) : WorldLocation(pos) { add(); PositionCopy(); }',
  block(wp,'virtual ~WorldPosition()'),block(wp,'bool operator==(const WorldPosition&'),
  'bool isDungeon() const { return mapId==100; }','};']
 # Assert copied constructors match production; only add a test-only counter.
 assert 'WorldPosition(const WorldPosition& pos) : WorldLocation(pos) { add(); }' in wp
 tn=read(P+'TravelNode.h',current)
 parts += [block(tn,'enum class TravelNodePathType',True),block(tn,'enum class PathNodeType',True),
  block(tn,'struct PathNodePoint',True),block(tn,'class TravelPath\n',True)]
 h=read(P+'strategy/actions/MovementActions.h',current)
 alias=''
 if 'using FlyDirectPathArgument' in h:
  start=h.rfind('#if',0,h.index('using FlyDirectPathArgument'));end=h.index('#endif',start)+len('#endif');alias=h[start:end]
 decl=re.search(r'bool FlyDirect\([^;]+;',h)[0]
 parts += ['class MovementAction {public:',alias,decl,'};']
 func=block(read(P+'strategy/actions/MovementActions.cpp',current),'bool MovementAction::FlyDirect(')
 parts.append(func)
 th=read(P+'TravelMgr.h',current)
 parts+=['class TravelTarget {public: std::vector<std::string> travelConditions; WorldPosition pos; bool forced=false;',
  block(th,'std::vector<std::string> GetConditions()')]
 if 'bool HasCondition(' in th:parts.append(block(th,'bool HasCondition('))
 parts+=['WorldPosition* GetPosition(){Trace("position");return &pos;} bool IsForced(){Trace("forced");return forced;}','};',
  '#include "travel_services.inc"',block(read(P+'strategy/actions/MoveToTravelTargetAction.cpp',current),'bool MoveToTravelTargetAction::isUseful()'),
  '#undef AI_VALUE','#undef AI_VALUE2','#undef MEM_AI_VALUE']
 packet=read('src/game/Server/WorldPacket.h',current)
 parts+=[block(packet,'class WorldPacket :',True),block(read(P+'strategy/Event.h',current),'class Event\n',True),
  block(read(P+'strategy/Event.cpp',current),'ObjectGuid Event::getObject()'),
  'class AiObject {public: AiObject(PlayerbotAI*){} };',block(read(P+'strategy/AiObject.h',current),'class AiNamedObject :',True),
  'class NextAction; template<class T>class Value;','#define time clockTime','#define rand clockRand',
  block(read(P+'strategy/Trigger.h',current),'class Trigger :',True),'#undef time','#undef rand',
  block(read(P+'strategy/Trigger.cpp',current),'Event Trigger::Check()'),
  'Unit* Trigger::GetTarget(){return nullptr;} Value<Unit*>* Trigger::GetTargetValue(){return nullptr;}','}']
 (out/(ns+'.h')).write_text('\n'.join(parts),encoding='utf-8')
(out/'provenance.json').write_text(json.dumps(manifest,indent=2))
