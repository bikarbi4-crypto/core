"""Extract both complete Execute bodies and unchanged production list policies.

World searches and context storage are fixtures; refresh, scalar fallback, Get,
RTTI GetValue, PMO call sites and all Execute control flow are production code.
"""
from pathlib import Path
import hashlib
import json
import re
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[3]
BASE = '33faa8e3c091d73483b42dc07f8f2934ba5f7958'
P = 'src/game/PlayerBots/playerbot/'
OUT = Path(sys.argv[1]); OUT.mkdir(parents=True, exist_ok=True)
manifest = {'baseline': BASE, 'sources': {}, 'registrations': []}

def read(path, old=False):
    path = P + path
    text = (subprocess.check_output(['git', 'show', f'{BASE}:{path}'], cwd=ROOT).decode('utf-8-sig')
            if old else (ROOT / path).read_text(encoding='utf-8-sig'))
    manifest['sources'][('v19:' if old else 'working:') + path] = hashlib.sha256(text.encode()).hexdigest()
    return text

def block(text, signature):
    start = text.index(signature); opening = text.index('{', start)
    level, end = 1, opening + 1
    while level:
        level += (text[end] == '{') - (text[end] == '}'); end += 1
    return text[start:end]

value = read('strategy/Value.h')
assert value == read('strategy/Value.h', True), 'B must not modify generic Value policies'
signatures = ['class UntypedValue', 'template<class T>\n    class Value\n',
              'template<class T>\n    class ValueBase',
              'template<class T, class Allocator>\n    class Value<std::list<T, Allocator>>',
              'template<class T>\n    class CalculatedValue', 'class ObjectGuidListCalculatedValue']
(OUT/'policies.inc').write_text('\n'.join(block(value,s)+';' for s in signatures), encoding='utf-8')
ctx = read('strategy/AiObjectContext.h')
get = block(ctx, 'template<class T>\n        Value<T>* GetValue(const std::string& name)')
(OUT/'getvalue.inc').write_text(get, encoding='utf-8')

names = ['possible targets', 'all targets', 'nearest npcs', 'nearest corpses', 'nearest game objects no los', 'nearest friendly players']
types = ['PossibleTargetsValue', 'AllTargetsValue', 'NearestNpcsValue', 'NearestCorpsesValue', 'NearestGameObjects', 'NearestFriendlyPlayersValue']
headers = ['PossibleTargetsValue.h', 'PossibleTargetsValue.h', 'NearestNpcsValue.h', 'NearestCorpsesValue.h', 'NearestGameObjects.h', 'NearestFriendlyPlayersValue.h']
vc = read('strategy/values/ValueContext.h')
for name, typ, header in zip(names, types, headers):
    source = read('strategy/values/' + header)
    assert source == read('strategy/values/' + header, True)
    reg = re.findall(r'^.*creators\["'+re.escape(name)+r'"\].*$', vc, flags=re.M)
    assert len(reg)==1 and ('new '+typ+'(') in reg[0]
    # Search every production source, including class-specific contexts, for any
    # additional registration of these exact keys (whitespace tolerated).
    matches=[]
    pattern=re.compile(r'creators\s*\[\s*"'+re.escape(name)+r'"\s*\]')
    for path in (ROOT/'src').rglob('*'):
        if path.suffix in ('.h','.cpp') and pattern.search(path.read_text(encoding='utf-8-sig', errors='replace')):
            matches.append(str(path.relative_to(ROOT)))
    assert len(matches)==1, (name,matches)
    parent = ('PossibleTargetsValue' if typ=='AllTargetsValue' else
              'ObjectGuidListCalculatedValue' if typ=='NearestGameObjects' else 'NearestUnitsValue')
    assert f'class {typ} : public {parent}' in source
    manifest['registrations'].append({'lookup':name,'type':typ,'registration':reg[0].strip(),
        'interval':1 if typ=='NearestGameObjects' else 4,
        'pmo_name':'nearest game objects' if typ=='NearestGameObjects' else name})
nu = read('strategy/values/NearestUnitsValue.h')
assert nu == read('strategy/values/NearestUnitsValue.h', True)
assert 'class NearestUnitsValue : public ObjectGuidListCalculatedValue' in nu
assert 'ObjectGuidListCalculatedValue(ai, name, 4)' in nu
assert 'class AllTargetsValue : public PossibleTargetsValue' in read('strategy/values/PossibleTargetsValue.h')
assert 'Get() final override' in value and 'GetSize() final override' in value
aware=read('PlayerbotAIAware.h')
assert 'virtual ~PlayerbotAIAware() = default;' in aware
guid_path='src/game/ObjectGuid.h'
guid=(ROOT/guid_path).read_text(encoding='utf-8-sig')
assert guid==subprocess.check_output(['git','show',f'{BASE}:{guid_path}'],cwd=ROOT).decode('utf-8-sig').replace('\r\n','\n')
assert 'uint64 m_guid;' in guid and '~ObjectGuid' not in guid
manifest['guid_fixture']={'path':guid_path,'sha256':hashlib.sha256(guid.encode()).hexdigest(),
    'representation':'one uint64; trivial copy/destruction; fixture uses uint64 GUIDs'}

action = 'strategy/actions/CheckValuesAction.cpp'
old, new = read(action,True), read(action)
oldbody = block(old,'bool CheckValuesAction::Execute(Event& event)')
newbody = block(new,'bool CheckValuesAction::Execute(Event& event)')
old_calls = re.findall(r'    std::list<ObjectGuid> \w+ = AI_VALUE\(std::list<ObjectGuid>, "([^"]+)"\);',oldbody)
new_calls = re.findall(r'    context->GetValue<std::list<ObjectGuid>>\("([^"]+)"\)->GetSize\(\);',newbody)
assert old_calls == new_calls == names
withoutold = re.sub(r'^    std::list<ObjectGuid> \w+ = AI_VALUE.*;\n','',oldbody,flags=re.M)
withoutnew = re.sub(r'^    context->GetValue<std::list<ObjectGuid>>.*;\n','',newbody,flags=re.M)
withoutnew = re.sub(r'^    //.*\n','',withoutnew,flags=re.M)
assert withoutold == withoutnew, 'Only unused copy statements may change'
for label, body in [('v19',oldbody),('v20',newbody)]:
    out=body.replace('CheckValuesAction::Execute','CheckValuesAction::Execute_'+label)
    (OUT/(label+'.inc')).write_text(out,encoding='utf-8')
manifest['generated_sha256']={p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in OUT.glob('*.inc')}
manifest['harness_sha256']={p.name:hashlib.sha256(p.read_bytes()).hexdigest()
    for p in Path(__file__).parent.iterdir() if p.is_file()}
(OUT/'provenance.json').write_text(json.dumps(manifest,indent=2),encoding='utf-8')
print('CheckValues: six unique registrations; production Value unchanged; exact call order and remaining Execute body preserved.')
