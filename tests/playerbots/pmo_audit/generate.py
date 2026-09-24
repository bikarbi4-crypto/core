"""Extract actual V18/current PMO implementation, callsites and scope bodies.

World/game work is a fixture. The monitor, full ExecuteAction and FullTick
functions, and each PMO preparation fragment come from production source.
Trace-only probes do not exist in optimized benchmark translation units.
"""
from pathlib import Path
import hashlib
import json
import re
import subprocess
import sys

ROOT=Path(__file__).resolve().parents[3]
P='src/game/PlayerBots/playerbot/'
BASE='fbab350b6cde7a40735d778986382807d6287048'
out=Path(sys.argv[1]); out.mkdir(parents=True,exist_ok=True)
manifest={'baseline':BASE,'sources':{},'fragments':{},'controlled_substitutions':['world position coordinates/hooks','game work in scope fragments','action services','trace-only deterministic clock/probes']}

def read(path,current):
    path=P+path
    s=(ROOT/path).read_text(encoding='utf-8-sig') if current else subprocess.check_output(['git','show',BASE+':'+path],cwd=ROOT).decode('utf-8-sig')
    manifest['sources'][('working:' if current else BASE+':')+path]=hashlib.sha256(s.encode()).hexdigest()
    return s

def mask(s,strings=False):
    pattern=r'//[^\n]*|/\*.*?\*/'
    if strings: pattern=r'"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'|'+pattern
    return re.sub(pattern,lambda m: ''.join('\n' if c=='\n' else ' ' for c in m[0]),s,flags=re.S)

def block(s,sig):
    start=s.index(sig); b=s.index('{',start); m=mask(s,True)
    depth=1; end=b+1
    while depth:
        depth+=(m[end]=='{')-(m[end]=='}'); end+=1
    return s[start:end]

def clean(s):
    return re.sub(r'^\s*#(?:include|pragma once|ifndef _PerformanceMonitor_H|define _PerformanceMonitor_H|define sPerformanceMonitor|endif).*$', '',s,flags=re.M)

files=sorted(p.relative_to(ROOT/P).as_posix() for p in (ROOT/P).rglob('*') if p.suffix in ('.cpp','.h') and 'sPerformanceMonitor.start' in p.read_text(encoding='utf-8-sig'))
inventory=[]
all_fragments={}
gameplay_shapes={}
for ns,current in [('v18',False),('v19',True)]:
    frags=[]
    for file in files:
        s=read(file,current); m=mask(s)
        spans=[]
        for call in re.finditer(r'sPerformanceMonitor\.start\([^;]+;',m):
            at=call.start(); line=s.count('\n',0,at)+1
            stmt=s[at:call.end()]
            lineStart=s.rfind('\n',0,at)+1
            assignment=s[lineStart:at]
            var=re.search(r'(\w+)\s*=\s*$',assignment)[1]
            start=lineStart; end=call.end()
            if file=='PlayerbotAI.cpp':
                start=s.rfind('    std::string mapString',0,at) if not current else s.rfind('    std::unique_ptr<PerformanceMonitorOperation>',0,at)
                if current: end=s.index('}',end)+1
            elif file=='strategy/Engine.cpp' and 'std::move(actionName)' in stmt:
                start=s.rfind('            std::string actionName',0,at) if not current else s.rfind('            std::unique_ptr<PerformanceMonitorOperation>',0,at)
                if current: end=s.index('}',end)+1
            else:
                prev=s[:lineStart].rstrip(); prevLine=prev.splitlines()[-1].strip()
                if prevLine=='if (sPlayerbotAIConfig.perfMonEnabled)':
                    start=s.rfind('\n',0,len(prev)-len(prev.splitlines()[-1]))+1
                    # Include the pointer declaration when it immediately precedes the if.
                    prior=s[:start].rstrip()
                    if prior.splitlines()[-1].strip().startswith('std::unique_ptr<PerformanceMonitorOperation>'):
                        start=s.rfind('\n',0,len(prior)-len(prior.splitlines()[-1]))+1
            spans.append((start,end))
            frag=s[start:end].strip()
            if not re.search(r'(?:auto|std::unique_ptr<PerformanceMonitorOperation>)\s+'+var+r'\b',frag):
                frag='std::unique_ptr<PerformanceMonitorOperation> '+var+';\n'+frag
            # Value/trigger wrapper objects expose exactly the same getName service.
            frag=frag.replace('AiNamedObject::getName()', 'action->getName()').replace('this->ai','ai')
            frags.append((file,line,stmt,frag))
            if not current:
                if 'perfMonEnabled' in frag: category='B' if 'std::move(actionName)' in stmt else 'A'
                else: category='D' if file=='RandomPlayerbotMgr.cpp' else 'C'
                signatures=list(re.finditer(r'(?m)^\S[^;\n]*?\b([A-Za-z_]\w*::[A-Za-z_]\w*)\([^;\n]*\)\s*(?:const\s*)?\n?\{',s[:at]))
                function=signatures[-1][1] if signatures else ('CalculatedValue::RefreshValue' if line<110 else 'SingleCalculatedValue::Get')
                if file=='PlayerbotAI.cpp': frequency='each reached bot update / reaction / internal update'
                elif file=='PlayerbotAIBase.cpp': frequency='each base UpdateAI (FullTick spans until next reset)'
                elif file=='strategy/Value.h': frequency='each actual Value calculation'
                elif file=='strategy/Engine.cpp': frequency='each action candidate / explicit action / checked trigger'
                elif file=='RandomPlayerbotMgr.cpp': frequency='manager interval for first two scopes; per teleport/randomize/refresh otherwise'
                elif file=='PlayerbotFactory.cpp': frequency='per randomization/initialization; cold but nested/bursty'
                elif file.endswith('MovementActions.cpp'): frequency='each reached minimal movement attempt'
                else: frequency='per evaluated auction/vendor item; conditional bursts'
                inventory.append({'id':len(inventory),'file':P+file,'line_v18':line,'function':function,'category_v18':category,'frequency':frequency,'statement':stmt,'proposed_change':'none: arguments already lazy' if category=='A' else 'guard start and all PMO-only preparation'})
        remainder=s
        for start,end in reversed(spans): remainder=remainder[:start]+remainder[end:]
        if file=='RandomPlayerbotMgr.cpp':
            old_loop='''    for (auto& [mapId, map] : sMapMgr.Maps())
    {
        sPerformanceMonitor.Init(map->GetId(), map->GetInstanceId());
    }'''
            new_loop='''    if (sPlayerbotAIConfig.perfMonEnabled)
    {
        for (auto& [mapId, map] : sMapMgr.Maps())
        {
            sPerformanceMonitor.Init(map->GetId(), map->GetInstanceId());
        }
    }'''
            expected=new_loop if current else old_loop
            assert remainder.count(expected)==1
            remainder=remainder.replace(expected,'')
        gameplay_shapes[(ns,file)]=re.sub(r'\s+','',remainder)
    all_fragments[ns]=frags
    header=clean(read('PerformanceMonitor.h',current)).replace('private:','public:')
    (out/(ns+'.h')).write_text('#pragma once\n#include "services.h"\nnamespace '+ns+' {\n'+header+'\n}\n',encoding='utf-8')
    cpp=read('PerformanceMonitor.cpp',current)
    cpp=re.sub(r'^\s*#include[^\n]*','',cpp,flags=re.M)
    cpp=cpp.replace('std::chrono::high_resolution_clock::now()', 'PMO_CLOCK::now()')
    # Counter probes are compiled out of benchmarks.
    for sig,probe in [('std::unique_ptr<PerformanceMonitorOperation> PerformanceMonitor::start(', '++counters.starts;'),
                      ('PerformanceMonitorOperation::PerformanceMonitorOperation(', '++counters.operations;')]:
        spans=list(re.finditer(re.escape(sig),cpp))
        for match in reversed(spans):
            b=cpp.index('{',match.start())+1
            cpp=cpp[:b]+'\n#ifdef PMO_TRACE\n'+probe+'\n#endif\n'+cpp[b:]
    # Any registry lookup on the main start path is reached only after the OFF return.
    marker='    auto md = mapsData.find(mapId);'
    if marker not in cpp: marker='    std::vector<std::string> localStack;'
    assert cpp.count(marker)==1
    cpp=cpp.replace(marker, '#ifdef PMO_TRACE\n    ++counters.lookups;\n#endif\n'+marker)
    prefix='#include "'+ns+'.h"\n#define sPerformanceMonitor PerformanceMonitor::instance()\n#ifdef PMO_TRACE\n#define PMO_CLOCK AuditClock\n#else\n#define PMO_CLOCK std::chrono::high_resolution_clock\n#endif\nnamespace '+ns+' {\n'
    (out/(ns+'_monitor.cpp')).write_text(prefix+cpp+'\n}\n',encoding='utf-8')
    functions=[]
    for i,(_,_,_,frag) in enumerate(frags):
        extra='const std::string& name=action->name;\n' if 'PERF_MON_ACTION, name,' in frag else ''
        functions.append('__declspec(noinline) void scope_'+str(i)+'(PlayerbotAI* ai, Action* action, Event& event)\n{\nBot* bot=ai->GetBot(); Action* trigger=action; uint32 onlineBotCount=(action && (action->flags&256)) ? 2 : 1, maxAllowedBotCount=2;\n'+extra+frag+'\nWork("scope body");\n}')
        manifest['fragments'][ns+':'+str(i)]=hashlib.sha256(frag.encode()).hexdigest()
    execute=block(read('strategy/Engine.cpp',current),'ActionResult Engine::ExecuteAction(')
    fulltick=block(read('PlayerbotAIBase.cpp',current),'void PlayerbotAIBase::UpdateAI(')
    functions += [execute,fulltick,'PlayerbotAIBase::PlayerbotAIBase() = default;','PlayerbotAIBase::~PlayerbotAIBase() = default;']
    functions.append('using Scope=void(*)(PlayerbotAI*,Action*,Event&);\nScope scopes[]={'+','.join('scope_'+str(i) for i in range(len(frags)))+'};')
    (out/(ns+'_scopes.inc')).write_text('\n\n'.join(functions),encoding='utf-8')

assert len(all_fragments['v18'])==len(all_fragments['v19'])==47
for file in files:
    assert gameplay_shapes[('v18',file)]==gameplay_shapes[('v19',file)], 'non-PMO source/scope/reset changed: '+file
manifest['non_pmo_source_and_reset_order_unchanged']=files
for row,old,new in zip(inventory,all_fragments['v18'],all_fragments['v19']):
    assert old[0]==new[0] and old[2]==new[2], 'metric/key/arguments changed'
    assert 'if (sPlayerbotAIConfig.perfMonEnabled)' in new[3], 'unguarded '+str(row)
    row['line_v19']=new[1]
# PMO-only edits must leave all of these cumulative consumers/infrastructure exact.
for file in ['strategy/Value.h','strategy/AiObjectContext.h','strategy/values/GrindTargetValue.cpp',
             'strategy/values/GrindTargetValue.h','strategy/values/QuestValues.cpp','strategy/values/LootValues.h',
             'strategy/values/VendorValues.h','TravelMgr.cpp','WorldPosition.h']:
    assert read(file,False)==read(file,True), file+' changed outside PMO batch'
(out/'inventory.json').write_text(json.dumps(inventory,indent=2),encoding='utf-8')
(out/'provenance.json').write_text(json.dumps(manifest,indent=2),encoding='utf-8')
(out/'inventory.h').write_text('inline constexpr int scopeCount='+str(len(inventory))+';\n',encoding='utf-8')
print('Extracted',len(inventory),'active PMO callsites; one commented factory call excluded.')
