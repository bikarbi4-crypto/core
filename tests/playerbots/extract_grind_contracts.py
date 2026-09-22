"""Compile whole production GrindTarget implementations with deterministic game services."""
from pathlib import Path
from hashlib import sha256
import json
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[2]
BASELINE = 'cea7ba955f5c0d57ff76d63e53f97f34b1d3ab4c'
PREFIX = 'src/game/PlayerBots/playerbot/strategy/values/GrindTargetValue.'


def read(ext, ref):
    path = PREFIX + ext
    if ref:
        return subprocess.check_output(['git', 'show', f'{ref}:{path}'], cwd=ROOT).decode('utf-8-sig')
    return (ROOT/path).read_text(encoding='utf-8-sig')


def class_block(text):
    start = text.index('class GrindTargetValue')
    opening = text.index('{', start)
    end, level = opening+1, 1
    while level:
        if text[end] == '{': level += 1
        elif text[end] == '}': level -= 1
        end += 1
    return text[start:end]+';'


a_ref = subprocess.check_output(['git', 'log', '-1', '--format=%H', '--grep=^V17-A:'], cwd=ROOT, text=True).strip() or None
generated, provenance = [], {}
for ns, ref in [('v16', BASELINE), ('v17a', a_ref), ('v17', None)]:
    header, cpp = read('h', ref), read('cpp', ref)
    body = cpp.split('using namespace ai;', 1)[1]
    generated.extend([f'namespace {ns} {{', class_block(header), body, '}'])
    provenance[ns] = {'ref': ref or 'working-tree', 'header_sha256': sha256(header.encode()).hexdigest(), 'cpp_sha256': sha256(cpp.encode()).hexdigest()}
output = Path(sys.argv[1])
output.write_text('\n\n'.join(generated), encoding='utf-8')
output.with_suffix('.json').write_text(json.dumps(provenance, indent=2), encoding='utf-8')
