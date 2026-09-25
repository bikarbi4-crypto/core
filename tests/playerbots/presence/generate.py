"""Compile the exact baseline/current priority, decision and cache bodies against identical fixtures."""
import hashlib, json, re, subprocess, sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[3]
BASE='169cb7712c8b704a3fe274cbf42a0cbb53fcb90a'
AI='src/game/PlayerBots/playerbot/PlayerbotAI.cpp'
HEADER='src/game/PlayerBots/playerbot/PlayerbotAI.h'
MGR='src/game/PlayerBots/playerbot/RandomPlayerbotMgr.cpp'
SIGS=['ActivePiorityType PlayerbotAI::GetPriorityType()', 'std::pair<uint32, uint32> PlayerbotAI::GetPriorityBracket(',
      'bool PlayerbotAI::AllowActive(', 'bool PlayerbotAI::AllowActivity(']
def extract(text, sig):
    start=text.index(sig); opening=text.index('{', start); depth=1; i=opening+1
    while depth:
        depth+=(text[i]=='{')-(text[i]=='}'); i+=1
    return text[start:i]
def old(path):return subprocess.check_output(['git','show',f'{BASE}:{path}'],cwd=ROOT).decode('utf-8-sig').replace('\r\n','\n')
out=Path(sys.argv[1]); out.mkdir(parents=True, exist_ok=True)
sys.dont_write_bytecode=True
from source_scope import verify_source_scope
(out/'source-scope-audit.json').write_text(json.dumps(verify_source_scope(),indent=2),encoding='utf-8')
baseline=old(AI); current=(ROOT/AI).read_text(encoding='utf-8-sig')
assert extract(baseline,SIGS[1])==extract(current,SIGS[1]), 'Priority brackets changed'
evidence={'baseline':BASE,'variants':{},'checks':{}}
for name, text in [('baseline',baseline),('candidate',current)]:
    bodies='\n\n'.join('__declspec(noinline) '+extract(text,s) for s in SIGS)
    mgr=old(MGR) if name=='baseline' else (ROOT/MGR).read_text(encoding='utf-8-sig')
    selector=extract(mgr,'float RandomPlayerbotMgr::getActivityPercentage(Player* bot)')
    bodies+='\n\n__declspec(noinline) '+selector
    (out/(name+'.inc')).write_text(bodies,encoding='utf-8')
    evidence['variants'][name]={'source_sha256':hashlib.sha256(text.encode()).hexdigest(),'bodies_sha256':hashlib.sha256(bodies.encode()).hexdigest()}
old_selector=extract(old(MGR),'float RandomPlayerbotMgr::getActivityPercentage(Player* bot)')
new_selector=extract((ROOT/MGR).read_text(encoding='utf-8-sig'),'float RandomPlayerbotMgr::getActivityPercentage(Player* bot)')
assert re.sub(r'PresenceDiagnostics::ActivityValue\((getActivityPercentage\(\)|localActivity), [0-6]\)',r'\1',new_selector)==old_selector
header=(ROOT/HEADER).read_text(encoding='utf-8-sig')
enums='\n'.join(extract(header,s)+';' for s in ['enum class ActivePiorityType', 'enum ActivityType'])
assert enums=='\n'.join(extract(old(HEADER),s)+';' for s in ['enum class ActivePiorityType', 'enum ActivityType'])
(out/'enums.inc').write_text(enums,encoding='utf-8')
# Assert protected code is unchanged; the only new source files are diagnostics.
for path in ['src/game/PlayerBots/playerbot/PerformanceMonitor.cpp','src/game/PlayerBots/playerbot/PerformanceMonitor.h',
             'src/game/PlayerBots/playerbot/strategy/values/Value.h','src/game/PlayerBots/playerbot/strategy/values/QuestValues.cpp']:
    if not (ROOT/path).exists(): continue
    assert old(path)==(ROOT/path).read_text(encoding='utf-8-sig'), path
    evidence['checks'][path]='unchanged'
(out/'provenance.json').write_text(json.dumps(evidence,indent=2),encoding='utf-8')
print('Extracted actual V20 and candidate bodies; brackets/enums/protected files unchanged.')
