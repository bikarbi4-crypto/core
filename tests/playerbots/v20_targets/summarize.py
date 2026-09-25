"""Retain every target filtering timing, including non-improvements."""
import csv
import json
from pathlib import Path
import statistics
import sys

timing, allocation, output = map(Path, sys.argv[1:4])
groups = {}
rows = list(csv.DictReader(timing.open()))
for row in rows:
    if int(row['round']) == 0:
        continue
    key = (row['fixture'], row['get_one'], row['mode'])
    groups.setdefault(key, {}).setdefault(row['variant'], []).append(float(row['ns']))
summary = []
for (fixture, one, mode), values in groups.items():
    assert len(values['v19']) == len(values['v20']) == 8
    old, new = (statistics.median(values[name]) for name in ('v19', 'v20'))
    summary.append({'fixture': fixture, 'get_one': bool(int(one)), 'mode': mode,
                    'v19_ns': old, 'v20_ns': new, 'delta_ns': new - old,
                    'speedup': old / new, 'change_percent': (new / old - 1) * 100})
allocs = {}
for row in csv.DictReader(allocation.open()):
    allocs.setdefault((row['fixture'], row['get_one']), {})[row['variant']] = row
for pair in allocs.values():
    assert pair['v19']['checksum'] == pair['v20']['checksum']
    assert pair['v19']['result_size'] == pair['v20']['result_size']
    assert int(pair['v20']['allocations']) == 2
    assert int(pair['v19']['allocations']) >= 2
result = {'raw_rows': len(rows), 'summaries': summary, 'allocation_pairs': len(allocs),
          'worst_negative': sorted(summary, key=lambda x: x['change_percent'], reverse=True)[:10],
          'notes': ['First round discarded; medians of eight paired same-work rounds; all raw data retained.',
                    'Cold includes creation/destruction of input list; warm prebuilds each independent input outside timer.',
                    'World predicates are fixtures; actual production RemoveNonThreating/Calculate are extracted.',
                    'Benchmark executable has normal CRT allocation. Allocation counts use a separate executable.',
                    'Function has no inter-tick cache, expiration or PMO branch; surrounding Value policy unchanged.']}
output.write_text(json.dumps(result, indent=2))
print(json.dumps({'raw_rows': len(rows), 'allocation_pairs': len(allocs), 'worst_negative': result['worst_negative'][:4]}, indent=2))
