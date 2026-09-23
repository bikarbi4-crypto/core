"""Extract the production lookup path and complete relevant inheritance chains.

Only world services, constructors requiring a live bot, and Format are stubbed.
No RTTI, cache, GetValue, Get, refresh, or metadata implementation is rewritten.
Each variant is compiled in separate factory and consumer translation units.
"""
from pathlib import Path
import hashlib
import json
import re
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[3]
P = 'src/game/PlayerBots/playerbot/'
V16 = 'cea7ba955f5c0d57ff76d63e53f97f34b1d3ab4c'
V17 = 'e93226fb808e71e2d221bfa9baba950e2bb248f0'
C = '580772ee0c6cb7a38ce73b090d7493d5c52f3bed'

def read(path, ref):
    if ref == 'working-tree':
        return (ROOT/path).read_text(encoding='utf-8-sig')
    return subprocess.check_output(['git', 'show', f'{ref}:{path}'], cwd=ROOT).decode('utf-8-sig')

def block(text, signature):
    begin = text.index(signature)
    opening = text.index('{', begin)
    level, end = 1, opening+1
    while level:
        if text[end] == '{': level += 1
        elif text[end] == '}': level -= 1
        end += 1
    return text[begin:end]+';'

def clean(text):
    return re.sub(r'^\s*#(?:include|pragma once).*$', '', text, flags=re.M)

out = Path(sys.argv[1]); out.mkdir(parents=True, exist_ok=True)
manifest = {}
variants = [('v16', V16), ('c_only', C), ('ab_only', V16), ('v17', V17)]
if '--candidate' in sys.argv:
    variants.append(('candidate', 'working-tree'))
for ns, ref in variants:
    sources = {}
    def get(path):
        path = P+path
        text = read(path, ref)
        sources[path] = hashlib.sha256(text.encode()).hexdigest()
        return text
    parts = ['#pragma once', '#include "services.h"']
    parts += [clean(get('PlayerbotAIAware.h')),
              clean(get('strategy/NamedObjectCache.h')),
              clean(get('strategy/NamedObjectContext.h')),
              clean(get('strategy/AiObject.h').split('// MACROS GO HERE')[0])]
    h = get('strategy/Value.h')
    first = h.index('    class UntypedValue')
    last = h.index('    class CDPairCalculatedValue')
    parts += ['namespace ai {', h[first:last], block(h, 'class ObjectGuidListCalculatedValue'),
              block(h, 'template<class T>\n    class ManualSetValue'),
              block(h, 'class UnitManualSetValue'), block(h, 'class BoolManualSetValue'), '}']
    ctx = get('strategy/AiObjectContext.h').split('#define AI_VALUE')[0]
    parts += ['namespace ai { class Action : public UntypedValue {}; class Strategy : public UntypedValue {}; class Trigger : public UntypedValue {}; }', clean(ctx)]
    parts += ['namespace ai {\n'
              'AiObjectContext* MakeContext(const Case& c);\n'
              '}']
    text = '\n'.join(parts)
    text = re.sub(r'\bnamespace ai\b', 'namespace '+ns, text)
    (out/(ns+'.h')).write_text(text, encoding='utf-8')
    manifest[ns] = {'value_ref': ref, 'grind_ref': V17 if ns in ('ab_only','v17','candidate') else V16,
                    'generated_sha256': hashlib.sha256(text.encode()).hexdigest(), 'source_sha256': sources}
(out/'provenance.json').write_text(json.dumps(manifest, indent=2), encoding='utf-8')
