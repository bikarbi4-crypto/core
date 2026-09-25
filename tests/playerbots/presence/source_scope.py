"""Narrow diagnostic normalization, backed by a whole-source V20 equivalence gate.

The PMO suite still extracts/executes real current PMO fragments. Only its
non-PMO remainder comparison uses this normalization; it does not skip files.
"""
from pathlib import Path
import hashlib, json, re, subprocess, sys

ROOT=Path(__file__).resolve().parents[3]
BASE='169cb7712c8b704a3fe274cbf42a0cbb53fcb90a'
PREFIX='src/game/PlayerBots/playerbot/'

def extract(text,signature):
    start=text.index(signature); opening=text.index('{',start); i=opening+1; depth=1
    while depth:
        depth+=(text[i]=='{')-(text[i]=='}'); i+=1
    return text[start:i]

def compact(text):
    # Preserve internal whitespace, including inside literals.
    return '\n'.join(line.strip() for line in text.splitlines() if line.strip())

def normalize_presence(file,text):
    """Remove only the declared observer hooks, including in a PMO-erased remainder."""
    if file=='PlayerbotAI.h':
        return text.replace('#include "PresenceDiagnostics.h"','',1).replace('    PresenceDiagnostics::BotState presenceState; // Scalar observation state, owned like allowActive.','')
    if file=='PlayerbotAI.cpp':
        for signature in ['ActivePiorityType PlayerbotAI::GetPriorityType()',
                          'bool PlayerbotAI::AllowActive(', 'bool PlayerbotAI::AllowActivity(']:
            body=extract(text,signature); stripped=body
            stripped=re.sub(r'^[ \t]*PresenceDiagnostics::(?:PriorityProbe|DecisionProbe) presence;\n','',stripped,flags=re.M)
            stripped=re.sub(r'^[ \t]*presence\.(?:Snapshot|Friend|Scan|Entry)\([^\n]*\);\n','',stripped,flags=re.M)
            stripped=re.sub(r'    if \(presence && bot\)\n        presence.Begin\([^\n]*\);\n','',stripped)
            stripped=re.sub(r'return presence.Finish\(([^\n]+)\);',r'return \1;',stripped)
            stripped=re.sub(r'^[ \t]*if \(PresenceDiagnostics::Enabled\(\) && activityType == ALL_ACTIVITY && bot\)\n[ \t]*PresenceDiagnostics::Cache\([^\n]*\);\n','',stripped,flags=re.M)
            stripped=stripped.replace('    {\n        return allowActive[activityType];\n    }','        return allowActive[activityType];')
            text=text.replace(body,stripped)
        return text.replace('#include "PresenceDiagnostics.h"','',1)
    if file=='RandomPlayerbotMgr.cpp':
        text=re.sub(r'PresenceDiagnostics::ActivityValue\((getActivityPercentage\(\)|localActivity), [0-6]\)',r'\1',text)
        text=re.sub(r'^[ \t]*if \(PresenceDiagnostics::Enabled\(\)\)\n[ \t]*PresenceDiagnostics::Scale\(\{.*?\}, (?:true|false)\);\n','',text,flags=re.M|re.S)
        for signature in ['    if (cmd == "presence" || cmd.find("presence ") == 0)',
                          'void RandomPlayerbotMgr::UpdatePresenceDiagnostics()',
                          'std::list<std::string> RandomPlayerbotMgr::HandleConsolePresence(']:
            text=text.replace(extract(text,signature),'')
        text=text.replace('// Scalar aggregation only. No game objects are dereferenced by the collector.','')
        return text.replace('#include "PresenceDiagnostics.h"','',1)
    return text

def verify_source_scope():
    def old(path):return subprocess.check_output(['git','show',BASE+':'+path],cwd=ROOT).decode('utf-8-sig').replace('\r\n','\n')
    def current(path):return (ROOT/path).read_text(encoding='utf-8-sig')
    checks={}
    def equal(label,a,b):
        assert compact(a)==compact(b),label
        checks[label]='equal after removing declared observation hooks and outer whitespace only'
    for file in ['PlayerbotAI.cpp','RandomPlayerbotMgr.cpp']:
        equal(file+' entire non-observer source',old(PREFIX+file),normalize_presence(file,current(PREFIX+file)))
    # Negative controls: normalization must not erase gameplay or PMO mutations.
    mutations=[('PlayerbotAI.cpp','!urand(0, 5)','!urand(0, 6)'),
               ('PlayerbotAI.cpp','allowActiveCheckTimer[activityType] + 5','allowActiveCheckTimer[activityType] + 6'),
               ('PlayerbotAI.cpp','if (sPlayerbotAIConfig.perfMonEnabled)','if (true)'),
               ('RandomPlayerbotMgr.cpp','errorMs * 0.5f','errorMs * 1.0f')]
    for file,needle,replacement in mutations:
        text=current(PREFIX+file); assert needle in text
        mutated=text.replace(needle,replacement,1)
        assert compact(old(PREFIX+file))!=compact(normalize_presence(file,mutated)),needle
    checks['negative controls']='RNG range, cache interval, PMO guard, PID gain mutations rejected'
    path='src/game/Maps/Map.cpp';text=current(path)
    signature='void Map::UpdatePlayers(bool updateBots)'
    body=extract(text,signature); stripped=body
    for _ in range(3):stripped=stripped.replace(extract(stripped,'    if (recordPresence)'),'',1)
    stripped=stripped.replace('    PresenceDiagnostics::MapSample presenceMap;','')
    stripped=stripped.replace('    bool const recordPresence = PresenceDiagnostics::Enabled() &&\n        PresenceDiagnostics::BeginMap(presenceMap, GetId(), GetInstanceId());','')
    equal('Map original update statements',extract(old(path),signature),stripped)
    equal('Map unrelated code',old(path).replace(extract(old(path),signature),''),text.replace(body,'').replace('#include "PlayerBots/playerbot/PresenceDiagnostics.h"','',1))
    path=PREFIX+'PlayerbotAIConfig.cpp';text=current(path)
    text=text.replace('#include <filesystem>','',1).replace('#include <iomanip>','',1)
    text=text.replace(extract(text,'std::vector<std::string> PlayerbotAIConfig::GetPresenceConfiguration() const'),'')
    equal('Config loader and defaults',old(path),text)
    path='src/game/World.cpp'
    equal('World update',old(path),current(path).replace('    sRandomPlayerbotMgr.UpdatePresenceDiagnostics();',''))
    path=PREFIX+'PlayerbotAI.h'
    equal('PlayerbotAI declarations',old(path),normalize_presence('PlayerbotAI.h',current(path)))
    path=PREFIX+'RandomPlayerbotMgr.h'
    equal('Manager declarations',old(path),current(path).replace('        void UpdatePresenceDiagnostics();','').replace('        std::list<std::string> HandleConsolePresence(std::string param);',''))
    path=PREFIX+'PlayerbotAIConfig.h'
    equal('Config declarations',old(path),current(path).replace('    std::vector<std::string> GetPresenceConfiguration() const;',''))
    expected={'src/game/Maps/Map.cpp','src/game/World.cpp'}|{PREFIX+p for p in ['PlayerbotAI.cpp','PlayerbotAI.h','PlayerbotAIConfig.cpp','PlayerbotAIConfig.h','RandomPlayerbotMgr.cpp','RandomPlayerbotMgr.h','PresenceDiagnostics.cpp','PresenceDiagnostics.h']}
    changed=subprocess.check_output(['git','diff','--name-only',BASE,'--','src'],cwd=ROOT,text=True).splitlines()
    assert set(changed)==expected,changed
    for path in [PREFIX+'PresenceDiagnostics.cpp',PREFIX+'PresenceDiagnostics.h']:
        assert not re.search(r'\b(?:Map|Player)\s*\*',current(path)),path
    return {'baseline':BASE,'checks':checks,'restricted_src_diff':sorted(expected),
            'sha256':{p:hashlib.sha256(current(p).encode()).hexdigest() for p in sorted(expected)}}

if __name__=='__main__':
    result=verify_source_scope()
    Path(sys.argv[1]).write_text(json.dumps(result,indent=2),encoding='utf-8')
    print('PASS strict V20 source equivalence after declared presence hooks.')
