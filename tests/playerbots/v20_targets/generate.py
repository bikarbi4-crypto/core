"""Extract exact V19/current production functions; only unrelated world predicates are fixtures."""
import hashlib
import json
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[3]
BASE = '33faa8e3c091d73483b42dc07f8f2934ba5f7958'
FILE = 'src/game/PlayerBots/playerbot/strategy/values/PossibleAttackTargetsValue.cpp'
SIGNATURES = [
    'std::list<ObjectGuid> PossibleAttackTargetsValue::Calculate()',
    'void PossibleAttackTargetsValue::RemoveNonThreating(std::list<ObjectGuid>& targets, bool getOne)',
]


def extract(text, signature):
    start = text.index(signature)
    opening = text.index('{', start)
    depth = 1
    i = opening + 1
    while depth:
        if text[i] == '{':
            depth += 1
        elif text[i] == '}':
            depth -= 1
        i += 1
    return text[start:i]


out = Path(sys.argv[1])
out.mkdir(parents=True, exist_ok=True)
baseline = subprocess.check_output(['git', 'show', f'{BASE}:{FILE}'], cwd=ROOT).decode().replace('\r\n', '\n')
current = (ROOT / FILE).read_text()
old = [extract(baseline, signature) for signature in SIGNATURES]
new = [extract(current, signature) for signature in SIGNATURES]
assert old[0] == new[0], 'Calculate wrapper changed'
assert baseline.replace(old[1], '').rstrip() == current.replace(new[1], '').rstrip(), 'Unrelated production changes'
evidence = {'baseline': BASE, 'path': FILE, 'variants': {}}
guid_path = 'src/game/ObjectGuid.h'
guid_baseline = subprocess.check_output(['git', 'show', f'{BASE}:{guid_path}'], cwd=ROOT).decode().replace('\r\n', '\n')
guid_current = (ROOT / guid_path).read_text()
assert guid_baseline == guid_current, 'ObjectGuid representation changed'
assert 'uint64 m_guid;' in guid_current and '~ObjectGuid' not in guid_current
evidence['object_guid'] = {
    'path': guid_path, 'sha256': hashlib.sha256(guid_current.encode()).hexdigest(),
    'fixture': 'uint64: production ObjectGuid has one uint64 field and implicit trivial copy/destruction; world APIs use raw fixture IDs',
}
for name, text, bodies in [('v19', baseline, old), ('v20', current, new)]:
    (out / f'{name}.inc').write_text('\n\n'.join('__declspec(noinline) ' + body for body in bodies) + '\n')
    evidence['variants'][name] = {
        'source_sha256': hashlib.sha256(text.encode()).hexdigest(),
        'function_sha256': [hashlib.sha256(body.encode()).hexdigest() for body in bodies],
    }
(out / 'provenance.json').write_text(json.dumps(evidence, indent=2))
print(json.dumps(evidence))
