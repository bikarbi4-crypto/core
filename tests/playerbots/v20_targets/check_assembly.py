"""Check MSVC extracted-body assembly (not the complete mangosd binary)."""
import hashlib
import json
from pathlib import Path
import re
import sys

assembly, output = map(Path, sys.argv[1:3])
text = assembly.read_text(errors='replace')
result = {'assembly_sha256': hashlib.sha256(assembly.read_bytes()).hexdigest(), 'variants': {}}
for name in ('v19', 'v20'):
    start = re.search(r'^\?RemoveNonThreating@PossibleAttackTargetsValue@' + name + r'@@.*? PROC.*$', text, re.M)
    assert start
    end = re.search(r'^\?RemoveNonThreating@PossibleAttackTargetsValue@' + name + r'@@.*? ENDP.*$', text[start.start():], re.M)
    assert end
    body = text[start.start():start.start() + end.end()]
    (output.parent / ('targets-' + name + '-function.asm')).write_text(body)
    calls = [line.strip() for line in body.splitlines() if re.search(r'\bcall\b', line)]
    direct_new = [line for line in calls if '; operator new' in line]
    assignments = [line for line in calls if '>::operator=' in line]
    result['variants'][name] = {'direct_operator_new_calls': len(direct_new), 'list_assignment_calls': len(assignments), 'all_calls': calls}
assert result['variants']['v20']['direct_operator_new_calls'] == 2, 'Candidate should allocate only local sentinel nodes'
assert result['variants']['v20']['list_assignment_calls'] == 0
assert result['variants']['v19']['direct_operator_new_calls'] > 2
output.write_text(json.dumps(result, indent=2))
print(json.dumps({k: {p: v[p] for p in ('direct_operator_new_calls', 'list_assignment_calls')} for k, v in result['variants'].items()}))
