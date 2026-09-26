"""Allow only declared V21 functions when checking cumulative/non-PMO code."""
from pathlib import Path
import hashlib,json,re,subprocess
ROOT=Path(__file__).resolve().parents[3]
BASE='169cb7712c8b704a3fe274cbf42a0cbb53fcb90a'
P='src/game/PlayerBots/playerbot/'
def old(path):return subprocess.check_output(['git','show',BASE+':'+path],cwd=ROOT).decode('utf-8-sig').replace('\r\n','\n')
def extract(text,sig):
 start=text.index(sig);i=text.index('{',start)+1;depth=1
 masked=re.sub(r'//[^\n]*|/\*.*?\*/|"(?:\\.|[^"\\])*"',lambda m:' '*len(m[0]),text,flags=re.S)
 while depth:depth+=(masked[i]=='{')-(masked[i]=='}');i+=1
 return text[start:i]
def normalize_v21(file,text):
 if file=='PlayerbotAI.cpp':
  original=old(P+file)
  for sig in ['ActivePiorityType PlayerbotAI::GetPriorityType()', 'bool PlayerbotAI::HasPlayerNearby(WorldPosition pos, float range)']:
   text=text.replace(extract(text,sig),extract(original,sig))
 if file=='RandomPlayerbotMgr.cpp':
  # Each replacement is a non-PMO expression or cold counter/command. Actual
  # current PMO scopes are still extracted, compared and executed by pmo_audit.
  text=text.replace('!HasPlayers() ? sPlayerbotAIConfig.diffEmpty','sRandomPlayerbotMgr.GetPlayers().empty() ? sPlayerbotAIConfig.diffEmpty')
  text=text.replace('sPlayerActivityPresence.OnMap(map->GetId(), map->GetInstanceId(), 0).map ?', 'map->HaveRealPlayers() ?')
  text=text.replace('m_registryErases += players.erase(player->GetGUIDLow());','players.erase(player->GetGUIDLow());')
  text=re.sub(r'^[ \t]*\+\+m_registry(?:Client|Bot|Move)Writes;\n','',text,flags=re.M)
  text=text.replace('    handlers["presence"] = &RandomPlayerbotMgr::HandleConsolePresence;\n','')
  text=text.replace('        {"presence", "Show V21 human activity presence and mixed-registry producer counters.\\nUsage: presence"},\n','')
  sig='std::list<std::string> RandomPlayerbotMgr::HandleConsolePresence('
  text=text.replace(extract(text,sig)+'\n\n','')
 return text
def verify():
 evidence={'baseline':BASE,'normalized_exact':[],'changed_source':{}}
 for file in ['PlayerbotAI.cpp','RandomPlayerbotMgr.cpp']:
  text=(ROOT/P/file).read_text(encoding='utf-8-sig')
  assert normalize_v21(file,text)==old(P+file),'Unapproved non-presence change: '+file
  evidence['normalized_exact'].append(P+file)
 allowed={'src/game/'+p for p in ['CMakeLists.txt','Camera.cpp','Camera.h','Maps/Map.cpp','Objects/Object.cpp','Objects/Player.cpp',
  'Objects/Player.h','World.cpp','Server/WorldSession.cpp','PlayerActivityPresence.h','PlayerActivityPresence.cpp']}
 allowed.update(P+p for p in ['PlayerbotAI.cpp','RandomPlayerbotMgr.cpp','RandomPlayerbotMgr.h'])
 changed=subprocess.check_output(['git','diff','--name-only',BASE,'--','src'],cwd=ROOT).decode().splitlines()
 for path in changed:
  content=(ROOT/path).read_text(encoding='utf-8-sig')
  if path=='src/shared/Progression.h':
   template=(ROOT/'cmake/generators/Progression.h.in').read_text(encoding='utf-8-sig')
   assert content in [old(path),template.replace('@supported_build@','5875')];continue
  assert path in allowed,'Changed protected source: '+path
  evidence['changed_source'][path]=hashlib.sha256(content.encode()).hexdigest()
 for path in ['PlayerActivityPresence.h','PlayerActivityPresence.cpp']:
  text=(ROOT/'src/game'/path).read_text()
  assert not re.search(r'\b(?:Player|WorldSession|Map)\s*\*',text),'Gameplay pointer in presence index'
 assert not (ROOT/P/'PresenceDiagnostics.cpp').exists(),'V20 diagnostic collector imported'
 return evidence
if __name__=='__main__':print(json.dumps(verify(),indent=2))
