"""Inspect dumpbin /HEADERS /DISASM:NOBYTES output for the actual shared method.

Usage: inspect_asm.py V19_DUMP V20_DUMP OUTPUT_DIRECTORY
This is the MSVC object code of the production-method harness, not the release EXE.
"""
from pathlib import Path
import hashlib
import json
import re
import sys

output=Path(sys.argv[3])
symbol='?GetUntypedValue@SharedObjectContext@ai@@UEAAPEAVUntypedValue@2@AEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@@Z'
result={'scope':'MSVC x64 normal production-constructor harness COFF object code; not final server EXE','variants':{}}
for variant,source in zip(['baseline','candidate'],sys.argv[1:3]):
    path=Path(source)
    text=path.read_text(encoding='utf-8-sig')
    sections=re.split(r'(?=^SECTION HEADER #)',text,flags=re.M)
    sections=[s for s in sections if re.search(r'COMDAT; sym=.*\('+re.escape(symbol)+r'\)\s*$',s,flags=re.M)]
    assert len(sections)==1
    body=sections[0]
    calls=re.findall(r'\bcall\s+([^\r\n]+)',body)
    item={'dump_sha256':hashlib.sha256(path.read_bytes()).hexdigest(),
          'method_sha256':hashlib.sha256(body.encode()).hexdigest(),
          'ai_constructor_calls':sum('??0PlayerbotAI@@' in c for c in calls),
          'operator_new_calls':sum('??2@' in c for c in calls),
          'named_cache_calls':sum('NamedObjectContextList' in c for c in calls),
          'mutex_locks':calls.count('_Mtx_lock'),'mutex_unlocks':calls.count('_Mtx_unlock'),
          'calls':calls}
    assert item['named_cache_calls']==1
    if variant=='baseline': assert item['ai_constructor_calls']==item['operator_new_calls']==1
    else:
        assert item['ai_constructor_calls']==item['operator_new_calls']==0
        assert item['mutex_locks']==item['mutex_unlocks']==1
    result['variants'][variant]=item
    (output/('shared-GetUntypedValue-'+variant+'.txt')).write_text(body,encoding='utf-8')
(output/'shared-code-review.json').write_text(json.dumps(result,indent=2),encoding='utf-8')
print('MSVC shared GetUntypedValue: V19 allocates/constructs one AI; V20 uses owned AI, one registry lookup, zero AI construction/allocation calls.')
