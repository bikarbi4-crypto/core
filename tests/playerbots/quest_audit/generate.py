"""Extract V17.1 and V18 production bodies; never hand-copy the algorithms.

Factor variants select source fragments from the immutable baseline and current
tree. Only A/B's two independent edits in NeedForQuest are removed to isolate
them. The combined variant is the exact current function. Game/world services
are fixtures; lookup, RTTI, qualifiers, Value policies and consumers are real.
"""
from pathlib import Path
import hashlib
import json
import re
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[3]
P = 'src/game/PlayerBots/playerbot/'
BASE = '2ea64f9d27563ac410bacea79de6e2e2671b01c3'
out = Path(sys.argv[1]); out.mkdir(parents=True, exist_ok=True)
manifest = {'baseline': BASE, 'variants': {}, 'sources': {}}

def read(path, current=False):
    path = P + path
    s = (ROOT/path).read_text(encoding='utf-8-sig') if current else subprocess.check_output(
        ['git', 'show', f'{BASE}:{path}'], cwd=ROOT).decode('utf-8-sig')
    manifest['sources'][('working:' if current else BASE+':')+path] = hashlib.sha256(s.encode()).hexdigest()
    return s

def block(s, signature, semi=False):
    start = s.index(signature); begin = s.index('{', start)
    # Source blocks used here contain balanced braces even in comments/strings.
    depth, end = 1, begin+1
    while depth:
        depth += (s[end] == '{') - (s[end] == '}'); end += 1
    return s[start:end] + (';' if semi else '')

def clean(s):
    return re.sub(r'^\s*#(?:include|pragma once).*$', '', s, flags=re.M)

old = read('strategy/values/QuestValues.cpp')
new = read('strategy/values/QuestValues.cpp', True)
assert block(old,'bool NeedQuestObjectiveValue::Calculate()') == block(new,'bool NeedQuestObjectiveValue::Calculate()')
for invariant in ['PlayerbotAIAware.h','strategy/AiObject.h','strategy/AiObjectContext.h',
                  'strategy/NamedObjectCache.h','strategy/NamedObjectContext.h',
                  'strategy/values/SharedValueContext.h','strategy/values/QuestValues.h',
                  'strategy/values/LootValues.cpp','strategy/values/VendorValues.cpp','TravelMgr.cpp']:
    assert read(invariant)==read(invariant,True), invariant+' changed outside the audited candidates'
sig = 'bool NeedForQuestValue::Calculate()'
baseline_need, final_need = block(old, sig), block(new, sig)
a_block = '\t\t\t// This query uses the quest id and the same travel snapshot, not the\n' + final_need.split('\t\t\t// This query uses the quest id and the same travel snapshot, not the\n',1)[1].split('\t\t\tDestinationList destinations',1)[0]
def no_a(s):
    assert s.count(a_block) == 1
    return s.replace('\t\tbool destinationsChecked = false;\n','').replace(a_block,'')
def no_b(s):
    prefix = '\t\tconst std::string objectiveQualifierPrefix = "{" + std::to_string(questId) + ",";\n'
    line = '\t\t\tconst std::string qualifier = objectiveQualifierPrefix + std::to_string(objective) + "}";'
    assert s.count(prefix) == s.count(line) == 1
    return s.replace(prefix,'').replace(line, '\t\t\tstd::vector<std::string> qualifier = { std::to_string(questId), std::to_string(objective) };').replace(
        'AI_VALUE2(bool, "need quest objective", qualifier)', 'AI_VALUE2(bool, "need quest objective", Qualified::MultiQualify(qualifier, ","))')
assert no_a(no_b(final_need)) == baseline_need
for ns, a, b, c in [('v171',False,False,False),('a_only',True,False,False),('b_only',False,True,False),('c_only',False,False,True),('v18',True,True,True)]:
    need = final_need if a or b else baseline_need
    if a and not b: need = no_b(need)
    if b and not a: need = no_a(need)
    parts = ['#pragma once', '#include "services.h"', clean(read('PlayerbotAIAware.h')),
             clean(read('strategy/NamedObjectCache.h')), clean(read('strategy/NamedObjectContext.h')),
             clean(read('strategy/AiObject.h').split('// MACROS GO HERE')[0])]
    value = read('strategy/Value.h')
    assert value == read('strategy/Value.h', True), 'V17.1 Value infrastructure must stay intact'
    parts += ['namespace ai {',value[value.index('    class UntypedValue'):value.index('    template<class T> class MemoryCalculatedValue')],
              block(value,'class BoolCalculatedValue',True),
              'class Action : public UntypedValue {}; class Strategy : public UntypedValue {}; class Trigger : public UntypedValue {};','}',
              clean(read('strategy/AiObjectContext.h').split('#define AI_VALUE')[0]), 'namespace ai {']
    for cls, file in [('NeedForQuestValue','QuestValues.h'),('NeedQuestObjectiveValue','QuestValues.h'),('ItemDropListValue','LootValues.h'),('ItemVendorListValue','VendorValues.h')]:
        parts.append(block(read('strategy/values/'+file, c and cls.startswith('Item')), 'class '+cls,True))
    shared = block(read('strategy/values/SharedValueContext.h'), 'class SharedObjectContext',True)
    parts += ['#include "variant_services.h"', shared, 'extern SharedObjectContext* sharedContext;',
              '#define sSharedObjectContext (*sharedContext)',
              '#define AI_VALUE2(type, name, param) context->GetValue<type>(name, param)->Get()',
              '#define GAI_VALUE(type, name) sSharedObjectContext.GetValue<type>(name)->Get()',
              '#define GAI_VALUE2(type, name, param) sSharedObjectContext.GetValue<type>(name, param)->Get()',
              '#ifdef QUEST_TRACE', '#define time quest_time', '#endif']
    # Value templates use the injected clock only in the trace build.
    header = '\n'.join(parts)
    header = header.replace('time(0)', 'audit_time(0)')
    header += '\n}\n'
    header = re.sub(r'\bnamespace ai\b','namespace '+ns,header)
    (out/(ns+'.h')).write_text(header,encoding='utf-8')
    funcs = [need, block(old,'bool NeedQuestObjectiveValue::Calculate()'),
             block(new if c else old,'bool NeedQuestObjectiveValue::CanGetItemSomewhere('),
             block(read('TravelMgr.cpp'),'DestinationList TravelMgr::GetDestinations('),
             block(read('strategy/values/LootValues.cpp'),'std::list<int32> ItemDropListValue::Calculate()'),
             block(read('strategy/values/VendorValues.cpp'),'std::list<int32> ItemVendorListValue::Calculate()')]
    body = '\n\n'.join(funcs)
    # Trace events are removed entirely from optimized benchmark compilation.
    for signature, event in [(sig,'Trace("need:"+getQualifier());'),
                            ('bool NeedQuestObjectiveValue::Calculate()','Trace("calculate:"+getQualifier());'),
                            ('DestinationList TravelMgr::GetDestinations(', 'TraceDestination(entries, onlyPossible, maxDistance);'),
                            ('std::list<int32> ItemDropListValue::Calculate()','Trace("drop-calculate:"+getQualifier());'),
                            ('std::list<int32> ItemVendorListValue::Calculate()','Trace("vendor-calculate:"+getQualifier());')]:
        start=body.index(signature); at=body.index('{',start)+1
        body=body[:at]+'\n#ifdef QUEST_TRACE\n'+event+'\n#endif\n'+body[at:]
    (out/(ns+'_bodies.inc')).write_text(body,encoding='utf-8')
    manifest['variants'][ns]={'A':a,'B':b,'C':c,'need_sha256':hashlib.sha256(need.encode()).hexdigest(),
                              'header_sha256':hashlib.sha256(header.encode()).hexdigest(),
                              'bodies_sha256':hashlib.sha256(body.encode()).hexdigest()}
for path in ['TravelMgr.h','WorldSquare.h','WorldSquare.cpp','WorldPosition.h','strategy/values/TravelValues.h']:
    assert read(path)==read(path,True), path+' changed'
(out/'provenance.json').write_text(json.dumps(manifest,indent=2),encoding='utf-8')
