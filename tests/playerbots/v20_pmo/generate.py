"""Extract unmodified PrintStats bodies and inventory all tracked production src.

Only includes, access control and surrounding namespace are adapted for the test.
The log service captures the real printf arguments and formatted output. No copied
aggregation algorithm substitutes for either production function.
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
OUT = Path(sys.argv[1])
OUT.mkdir(parents=True, exist_ok=True)


def mask(s):
    # Preserve offsets/newlines while removing comments, respecting string literals.
    token = r'"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'|//[^\n]*|/\*.*?\*/'
    return re.sub(token, lambda m: ''.join('\n' if c == '\n' else ' ' for c in m[0])
                  if m[0].startswith(('//', '/*')) else m[0], s, flags=re.S)


def body(s, signature):
    start = s.index(signature)
    i = s.index('{', start)
    depth = 1
    end = i + 1
    masked = mask(s)
    while depth:
        depth += (masked[end] == '{') - (masked[end] == '}')
        end += 1
    return s[start:end]


sources = {}
fragments = {}
for ns in ('v19', 'v20'):
    def read(file):
        data = ((ROOT / (P + file)).read_bytes() if ns == 'v20' else
                subprocess.check_output(['git', 'show', BASE + ':' + P + file], cwd=ROOT))
        sources[ns + ':' + P + file] = hashlib.sha256(data).hexdigest()
        return data.decode('utf-8-sig').replace('\r\n', '\n')

    header = read('PerformanceMonitor.h')
    header = re.sub(r'^\s*#(?:include|ifndef|define|endif)[^\n]*', '', header, flags=re.M)
    header = header.replace('private:', 'public:')
    cpp = read('PerformanceMonitor.cpp')
    parts = [body(cpp, sig) for sig in (
        'PerformanceMonitor::PerformanceMonitor()',
        'PerformanceMonitor::~PerformanceMonitor()',
        'std::string StackString(',
        'void PerformanceMonitor::PrintStats(',
        'void PerformanceMonitor::Reset()')]
    for n, part in enumerate(parts):
        fragments[f'{ns}:{n}'] = hashlib.sha256(part.encode()).hexdigest()
    (OUT / (ns + '.inc')).write_text('namespace ' + ns + ' {\nclass PlayerbotAI;\n' +
                                   header + '\n' + '\n'.join(parts) + '\n}\n', encoding='utf-8')

old = body(subprocess.check_output(['git', 'show', BASE + ':' + P + 'PerformanceMonitor.cpp'],
                                 cwd=ROOT).decode(), 'void PerformanceMonitor::PrintStats(')
new = body((ROOT / (P + 'PerformanceMonitor.cpp')).read_text(), 'void PerformanceMonitor::PrintStats(')
expected = old.replace('pd.maxTime < performanceData.minTime', 'pd.maxTime < performanceData.maxTime')
expected = expected.replace('pd.maxTime = performanceData.minTime', 'pd.maxTime = performanceData.maxTime')
assert expected == new, 'PrintStats changed beyond the two maxTime corrections'

tracked = subprocess.check_output(['git', 'ls-files', '-z', 'src'], cwd=ROOT).decode().split('\0')
pattern = re.compile(r'PerformanceMonitor|PerformanceData|PerformanceStack|performanceStack|totalPmo|'
                     r'PMO_MEMTEST|MEMORY_MONITOR|perfMonEnabled|PERF_MON_|performanceMetricMap|'
                     r'performanceMapMap|sMemoryMonitor|HandlePerfMonCommand')
records, starts, constructors, aliases, configs = [], [], [], [], []
file_hashes = {}
for rel in filter(None, tracked):
    data = (ROOT / rel).read_bytes()
    text = data.decode('utf-8-sig', errors='replace')
    code = mask(text)
    if pattern.search(text):
        file_hashes[rel] = hashlib.sha256(data).hexdigest()
    for lineno, line in enumerate(text.splitlines(), 1):
        if pattern.search(line):
            records.append({'file': rel, 'line': lineno, 'text': line.strip()})
    for m in re.finditer(r'\bsPerformanceMonitor\b', code):
        tail = code[m.end():]
        lineno = code.count('\n', 0, m.start()) + 1
        line = text.splitlines()[lineno - 1].strip()
        if re.match(r'\s*\.\s*(start|Init|Reset|PrintStats)\s*\(', tail):
            if re.match(r'\s*\.\s*start\s*\(', tail):
                preceding = code[max(0, m.start()-1000):m.start()]
                # The full scope inventory additionally checks exact enclosing fragments.
                assert 'if (sPlayerbotAIConfig.perfMonEnabled)' in preceding, (rel, lineno)
                starts.append({'file': rel, 'line': lineno, 'text': line})
        else:
            aliases.append({'file': rel, 'line': lineno, 'text': line})
    for m in re.finditer(r'\b(?:new\s+PerformanceMonitorOperation|make_unique\s*<\s*PerformanceMonitorOperation)', code):
        constructors.append({'file': rel, 'line': code.count('\n', 0, m.start())+1})
    if re.search(r'^\s*#\s*define\s+MEMORY_MONITOR\b', code, re.M):
        configs.append(rel)
assert len(starts) == 47, starts
assert len(constructors) == 1 and constructors[0]['file'] == P + 'PerformanceMonitor.cpp'
assert len(aliases) == 1 and aliases[0]['text'] == '#define sPerformanceMonitor PerformanceMonitor::instance()'
assert not configs, configs
inventory = {'scope': 'every git-tracked file under production src; comments retained in evidence, masked for active use counts',
             'tracked_files_scanned': len(list(filter(None, tracked))),
             'source_sha256': file_hashes, 'matches': records, 'active_external_start_calls': starts,
             'new_or_make_unique_operation_constructions': constructors, 'monitor_aliases': aliases,
             'active_memory_monitor_defines': configs,
             'limitations': 'Lexical inventory plus manual review, not a universal C++ macro/dataflow proof. Build configurations separately inspected.'}
(OUT / 'source-audit.json').write_text(json.dumps(inventory, indent=2), encoding='utf-8')
(OUT / 'provenance.json').write_text(json.dumps({'baseline': BASE, 'sources': sources,
    'fragment_sha256': fragments, 'max_only_delta': True}, indent=2), encoding='utf-8')
print(f'PMO repo-wide source audit: {inventory["tracked_files_scanned"]} files; 47 starts; '
      'one internal operation allocation; one singleton macro alias; PrintStats max-only delta.')
