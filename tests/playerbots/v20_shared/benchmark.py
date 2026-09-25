"""Measure legal V19/V20 shared-value paths with actual production constructors.

Usage: benchmark.py NORMAL_BUILD ALLOCATION_BUILD OUTPUT_PREFIX
Normal executables must exclude SHARED_ALLOCATIONS. Every executable pins CPU 0.
The allocation executable is separate and never supplies timing measurements.
"""
from pathlib import Path
import csv
import hashlib
import json
import math
import statistics
import subprocess
import sys

normal,allocation,prefix=map(Path,sys.argv[1:4])
variants=['baseline','candidate']
kinds=['lookup','long_lookup','unknown_lookup','warm_get','cold_get','reset_get']
assert 'SHARED_ALLOCATIONS:BOOL=OFF' in (normal/'CMakeCache.txt').read_text()
assert 'SHARED_ALLOCATIONS:BOOL=ON' in (allocation/'CMakeCache.txt').read_text()

def exe(build,variant):
    return build/'RelWithDebInfo'/('shared_'+variant+'.exe')

def invoke(build,variant,kind,count,enabled,allocated=False):
    command=[str(exe(build,variant)),'--bench',kind,str(count),str(int(enabled))]
    result=subprocess.run(command,capture_output=True,text=True,check=True,timeout=60)
    values=next(csv.reader([result.stdout.strip()]))
    assert values[:4]==[variant,kind,str(int(enabled)),str(count)],result.stdout
    assert len(values)==(8 if allocated else 6),result.stdout
    if allocated:
        return dict(zip(['allocations','bytes','ai_constructions','checksum'],map(int,values[4:])))
    return {'ns_per_call':float(values[4]),'checksum':int(values[5])}

timings=[]
counts=[]
skipped=[]
calibrations=[]
for kind in kinds:
    for enabled in [False,True]:
        valid=list(variants)
        if enabled and kind in ['cold_get','reset_get']:
            valid.remove('baseline')
            skipped.append({'variant':'baseline','kind':kind,'pmo':True,
                'reason':'ASan-confirmed dangling AI dereference on PMO Calculate; undefined baseline is not a timing comparator.'})
        # Calibrate the faster candidate. Use equal counts for the slower V19.
        first=invoke(normal,'candidate',kind,1000,enabled)
        iterations=min(100000,max(1000,math.ceil(3_000_000/max(first['ns_per_call'],1))))
        calibrations.append({'kind':kind,'pmo':enabled,'probe':first,'iterations':iterations})
        for round_id in range(9):
            order=valid if round_id%2==0 else list(reversed(valid))
            expected=None
            for variant in order:
                row={'kind':kind,'pmo':int(enabled),'round':round_id,'variant':variant,'iterations':iterations,
                     **invoke(normal,variant,kind,iterations,enabled)}
                if expected is not None: assert expected==row['checksum']
                expected=row['checksum'];timings.append(row)
        expected=None
        for variant in valid:
            row={'kind':kind,'pmo':int(enabled),'variant':variant,'iterations':1000,
                 **invoke(allocation,variant,kind,1000,enabled,True)}
            if expected is not None: assert expected==row['checksum']
            expected=row['checksum'];counts.append(row)
        print(f'completed {kind} PMO={int(enabled)} iterations={iterations}',flush=True)

for suffix,rows in [('benchmark',timings),('allocations',counts)]:
    with Path(str(prefix)+'-'+suffix+'.csv').open('w',newline='',encoding='utf-8') as out:
        writer=csv.DictWriter(out,fieldnames=list(rows[0]));writer.writeheader();writer.writerows(rows)
cases=[]
for kind in kinds:
    for enabled in [False,True]:
        case={'kind':kind,'pmo':enabled}
        for variant in variants:
            data=[r['ns_per_call'] for r in timings if r['kind']==kind and r['pmo']==int(enabled) and r['variant']==variant and r['round']>0]
            if data:
                case[variant]={'median_ns':statistics.median(data),'min_ns':min(data),'max_ns':max(data)}
                allocation_row=next(r for r in counts if r['kind']==kind and r['pmo']==int(enabled) and r['variant']==variant)
                case[variant]['allocations_per_call']=allocation_row['allocations']/allocation_row['iterations']
                case[variant]['bytes_per_call']=allocation_row['bytes']/allocation_row['iterations']
                case[variant]['ai_constructions_per_call']=allocation_row['ai_constructions']/allocation_row['iterations']
        if 'baseline' in case:
            case['ratio_candidate_over_baseline']=case['candidate']['median_ns']/case['baseline']['median_ns']
        cases.append(case)

def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()
evidence={'baseline':'33faa8e3c091d73483b42dc07f8f2934ba5f7958','rounds':9,'excluded_warmup_rounds':[0],
          'clock':'actual steady_clock, real CRT allocations in normal build, no allocation hook',
          'affinity_mask':1,'timing_rows':len(timings),'allocation_rows':len(counts),
          'calibrations':calibrations,'cases':cases,'skipped_undefined_baselines':skipped,
          'negative_medians':[c for c in cases if c.get('ratio_candidate_over_baseline',0)>1],
          'boundaries':['Real AI layout and constructors/destructors, named factory/cache, Value policies and real PMO implementation.',
             'World-dependent item-drop Calculate uses a deterministic three-item fixture; registry narrowed to two production registrations plus test factories.',
             'Context and first owner AI construction occur before timer; cold_get recreates one cached value, not a whole context.',
             'Warm shared/statics are primed before timing. Each measurement is a fresh process, paired counts and checksums match.',
             'SingleCalculatedValue does not time-expire. reset_get forces its original Reset semantics.',
             'No full-server throughput or PMO-ON undefined baseline parity claimed.'],
          'executable_sha256':{f'{label}/{v}':sha(exe(build,v)) for label,build in [('normal',normal),('allocations',allocation)] for v in variants},
          'source_provenance_sha256':sha(normal/'generated/provenance.json'),
          'raw_sha256':{suffix:sha(Path(str(prefix)+'-'+suffix+'.csv')) for suffix in ['benchmark','allocations']}}
Path(str(prefix)+'-benchmark-summary.json').write_text(json.dumps(evidence,indent=2),encoding='utf-8')
print(json.dumps({'timing_rows':len(timings),'allocation_rows':len(counts),'negative_medians':len(evidence['negative_medians'])}))
