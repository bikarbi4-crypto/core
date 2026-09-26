"""Extract V20/current production bodies. Fixtures replace world services only."""
from pathlib import Path
import hashlib, json, re, subprocess, sys
ROOT=Path(__file__).resolve().parents[3]
BASE='169cb7712c8b704a3fe274cbf42a0cbb53fcb90a'
P='src/game/PlayerBots/playerbot/'
out=Path(sys.argv[1]);out.mkdir(parents=True,exist_ok=True)
def old(path):return subprocess.check_output(['git','show',BASE+':'+path],cwd=ROOT).decode('utf-8-sig').replace('\r\n','\n')
def current(path):return (ROOT/path).read_text(encoding='utf-8-sig')
def extract(text,sig):
 start=text.index(sig);i=text.index('{',start)+1;depth=1
 masked=re.sub(r'//[^\n]*|/\*.*?\*/|"(?:\\.|[^"\\])*"',lambda m:' '*len(m[0]),text,flags=re.S)
 while depth:depth+=(masked[i]=='{')-(masked[i]=='}');i+=1
 return text[start:i]
sys.dont_write_bytecode=True
from source_scope import verify
(out/'source-scope.json').write_text(json.dumps(verify(),indent=2),encoding='utf-8')
evidence={'baseline':BASE,'variants':{},'protected':{}}
sigs=['ActivePiorityType PlayerbotAI::GetPriorityType()', 'std::pair<uint32, uint32> PlayerbotAI::GetPriorityBracket(',
      'bool PlayerbotAI::AllowActive(', 'bool PlayerbotAI::AllowActivity(',
      'bool PlayerbotAI::HasPlayerNearby(WorldPosition pos, float range)', 'bool PlayerbotAI::HasPlayerNearby(float range)']
for variant,read in [('baseline',old),('candidate',current)]:
 text=read(P+'PlayerbotAI.cpp'); bodies='\n\n'.join('__declspec(noinline) '+extract(text,sig) for sig in sigs)
 bodies+='\n\n'+extract(read(P+'RandomPlayerbotMgr.cpp'),'float RandomPlayerbotMgr::getActivityPercentage(Player* bot)')
 force=extract(read('src/game/Maps/Map.cpp'),'        if (sPlayerbotAIConfig.forceActiveWhenNearPlayer && !plr->isRealPlayer())')
 bodies+='\n__declspec(noinline) bool Force(Player* plr) { bool playerNearby=false;\n'+force+'\nreturn playerNearby; }\n'
 evidence['variants'][variant]=hashlib.sha256(bodies.encode()).hexdigest()
 # Instrument only this standalone copy. The timed build compiles probes out.
 bodies=bodies.replace('Player* player = i.second;', 'PROBE(registryEntries); Player* player = i.second;')
 bodies=bodies.replace('Player* plr = itr->getSource();','PROBE(mapEntries); Player* plr = itr->getSource();')
 bodies=bodies.replace('Map::PlayerList const& mapPlayers = botMap->GetPlayers();','PROBE(mapScans); Map::PlayerList const& mapPlayers = botMap->GetPlayers();')
 (out/(variant+'.inc')).write_text(bodies,encoding='utf-8')
for sig in sigs[1:4]:assert extract(old(P+'PlayerbotAI.cpp'),sig)==extract(current(P+'PlayerbotAI.cpp'),sig),sig
for sig in ['enum class ActivePiorityType','enum ActivityType']:
 assert extract(old(P+'PlayerbotAI.h'),sig)==extract(current(P+'PlayerbotAI.h'),sig)
(out/'enums.inc').write_text('\n'.join(extract(current(P+'PlayerbotAI.h'),sig)+';' for sig in ['enum class ActivePiorityType','enum ActivityType']),encoding='utf-8')
hooks='\n\n'.join(extract(current('src/game/Objects/Player.cpp'),'void Player::'+name+'(') for name in
 ['StartActivityPresence','StopActivityPresence','UpdateActivityPosition','UpdateActivityCamera','UpdateActivityGmState'])
(out/'hooks.inc').write_text(hooks,encoding='utf-8')
evidence['lifecycle_methods_sha256']=hashlib.sha256(hooks.encode()).hexdigest()
# Test operations are instrumented outside production; timing uses the real cpp.
cpp=current('src/game/PlayerActivityPresence.cpp')
cpp=cpp.replace('#include "PlayerActivityPresence.h"','#include "PlayerActivityPresence.h"\n#include "measurements.h"')
cpp=cpp.replace('auto const& location = entry.second.observation.location;', 'PROBE(presenceEntries); auto const& location = entry.second.observation.location;')
cpp=cpp.replace('auto const& observation = entry.second.observation;', 'PROBE(presenceEntries); auto const& observation = entry.second.observation;')
(out/'presence_ops.cpp').write_text(cpp,encoding='utf-8')
for path in ['PerformanceMonitor.cpp','PerformanceMonitor.h','strategy/values/QuestValues.cpp',
 'strategy/values/GrindTargetValue.cpp','strategy/values/PossibleTargetsValue.cpp','strategy/AiObjectContext.cpp',
 'strategy/AiObjectContext.h','strategy/Value.h','TravelMgr.cpp','PlayerbotAIConfig.cpp','PlayerbotAIConfig.h']:
 if (ROOT/P/path).exists():
  assert old(P+path)==current(P+path),path
  evidence['protected'][P+path]=hashlib.sha256(current(P+path).encode()).hexdigest()
# Source wiring checks: the standalone executable never creates a real session.
wiring=[('src/game/Objects/Player.cpp','void Player::AddToWorld()','StartActivityPresence();'),
 ('src/game/Objects/Player.cpp','void Player::RemoveFromWorld()','StopActivityPresence();'),
 ('src/game/Objects/Player.cpp','Player::~Player()','StopActivityPresence();'),
 ('src/game/Objects/Player.cpp','void Player::SetSession(','StartActivityPresence();'),
 ('src/game/Objects/Player.cpp','void Player::RelocateToLastClientPosition()','UpdateActivityPosition();'),
 ('src/game/Objects/Player.cpp','void Player::SetGameMaster(','UpdateActivityGmState();'),
 ('src/game/Server/WorldSession.cpp','bool WorldSession::ForcePlayerLogoutDelay()','StopActivityPresence();'),
 ('src/game/Server/WorldSession.cpp','void WorldSession::LogoutPlayer(','StopActivityPresence();'),
 ('src/game/Objects/Object.cpp','void WorldObject::Relocate(float x, float y, float z, float orientation)','UpdateActivityPosition();'),
 ('src/game/Objects/Object.cpp','void WorldObject::SetMap(','UpdateActivityPosition();'),
 ('src/game/Camera.cpp','void Camera::SetView(','UpdateActivityCamera(m_source);'),
 ('src/game/Camera.cpp','void Camera::Event_ActivityRelocated()','UpdateActivityCamera(m_source);')]
for file,sig,hook in wiring:assert hook in extract(current(file),sig),(file,sig)
evidence['wiring']=wiring
evidence['limits']='World services are deterministic fixtures; live map scheduling and gameplay require user runtime validation.'
(out/'provenance.json').write_text(json.dumps(evidence,indent=2),encoding='utf-8')
print('V21: production priority, nearby, force, lifecycle methods extracted; brackets/cache/config/PMO protected.')
