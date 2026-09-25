"""Preserve every round and summarize paired equal-work CheckValues timings."""
import csv
import hashlib
import json
from pathlib import Path
import statistics
import sys

path=Path(sys.argv[1])
rows=list(csv.DictReader(path.open(newline='',encoding='utf-8')))
groups={}
for row in rows:
    key=tuple(int(row[k]) for k in ('stage','size','pmo'))
    groups.setdefault(key,{}).setdefault(row['variant'],[]).append(row)
result=[]
for key,variants in sorted(groups.items()):
    a,b=variants['v19'],variants['v20']
    assert len(a)==len(b)==9
    for x,y in zip(a,b):
        assert x['round']==y['round'] and x['iterations']==y['iterations'] and x['checksum']==y['checksum']
    stat={}
    for label,data in variants.items():
        values=[float(r['ns_per_call']) for r in data if int(r['round'])>0]
        stat[label]={'median_ns':statistics.median(values),'min_ns':min(values),'max_ns':max(values)}
    result.append({'stage':['cold','warm','expired','reset'][key[0]],'size':key[1],'pmo':bool(key[2]),
                   **stat,'ratio_v20_over_v19':stat['v20']['median_ns']/stat['v19']['median_ns']})
out={'csv_sha256':hashlib.sha256(path.read_bytes()).hexdigest(),'rows':len(rows),'warmup_rounds_excluded':[0],
     'cases':result,'negative_medians':[r for r in result if r['ratio_v20_over_v19']>1]}
Path(sys.argv[2]).write_text(json.dumps(out,indent=2),encoding='utf-8')
print(json.dumps({'rows':len(rows),'cases':len(result),'negative_medians':len(out['negative_medians'])}))
