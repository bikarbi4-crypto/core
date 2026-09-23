"""Production A/B isolation, with separate untimed instrumentation copies."""
from pathlib import Path
import hashlib
import json
import subprocess
import sys
ROOT=Path(__file__).resolve().parents[3]
PREFIX='src/game/PlayerBots/playerbot/strategy/values/GrindTargetValue.'
BASE='cea7ba955f5c0d57ff76d63e53f97f34b1d3ab4c'
V17='e93226fb808e71e2d221bfa9baba950e2bb248f0'
A='2cbe4b8a5469794166e4a21cabbdcd0ba3a13ecf'
def read(ext,ref):
    return subprocess.check_output(['git','show',f'{ref}:{PREFIX}{ext}'],cwd=ROOT).decode('utf-8-sig')
def block(text):
    begin=text.index('class GrindTargetValue'); opening=text.index('{',begin)
    end,level=opening+1,1
    while level:
        if text[end]=='{':level+=1
        elif text[end]=='}':level-=1
        end+=1
    return text[begin:end]+';'
generated=[]; manifest={}
for ns,ref in [('v16',BASE),('c_only',BASE),('ab_only',V17),('v17',V17),('a_only',A)]:
    h=read('h',ref); cpp=read('cpp',ref); body=cpp.split('using namespace ai;',1)[1]
    generated.extend([f'namespace {ns} {{',block(h),body,'}'])
    counted=body
    begin=counted.index('Unit* GrindTargetValue::FindTargetForGrinding')
    brace=counted.index('{',begin)
    counted=counted[:brace+1]+'\n++auditCounters.passes;\n'+counted[brace+1:]
    begin=counted.index('for (std::list<ObjectGuid>::iterator tIter = targets.begin();')
    brace=counted.index('{',begin)
    counted=counted[:brace+1]+'\n++auditCounters.candidateVisits;\n'+counted[brace+1:]
    marker='const int targetingPlayerCount = GetTargetingPlayerCount'
    assert counted.count(marker)==1
    counted=counted.replace(marker,'++auditCounters.survivingVisits;\n            '+marker)
    generated.extend([f'namespace {ns}_count {{',block(h),counted,'}'])
    manifest[ns]={'ref':ref,'header_sha256':hashlib.sha256(h.encode()).hexdigest(),'cpp_sha256':hashlib.sha256(cpp.encode()).hexdigest()}
out=Path(sys.argv[1]); out.mkdir(parents=True,exist_ok=True)
(out/'grind_contracts.h').write_text('\n'.join(generated),encoding='utf-8')
(out/'provenance.json').write_text(json.dumps(manifest,indent=2))
