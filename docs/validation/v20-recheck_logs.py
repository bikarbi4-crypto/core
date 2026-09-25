"""Read-only independent V19 handoff recount; output contains no raw player logs."""
import argparse
import ctypes
from collections import Counter
from datetime import datetime, timedelta
from decimal import Decimal
import hashlib
import json
from pathlib import Path
import re

p=argparse.ArgumentParser()
p.add_argument('--logs',type=Path,required=True)
p.add_argument('--output',type=Path,required=True)
args=p.parse_args()
kernel=ctypes.WinDLL('kernel32',use_last_error=True)
kernel.GetCurrentProcess.restype=ctypes.c_void_p
kernel.SetProcessAffinityMask.argtypes=[ctypes.c_void_p,ctypes.c_size_t]
if not kernel.SetProcessAffinityMask(kernel.GetCurrentProcess(),0x40):
    raise ctypes.WinError(ctypes.get_last_error())
START='2026-09-25 06:05:02'; END='2026-09-25 06:58:29'
WORLD='2026-09-25 06:06:22'; ON='2026-09-25 06:56:44'; OFF='2026-09-25 06:57:10'
EXPECTED={
'Server.log':(307744,4048,'99e414656aa4e8caca4819a3371a1cb2dcd662a8d6bef18f7817992ccba38264'),
'Perf.log':(35305638,656628,'50dbad0f6f714aa5acbb2ede5b0568c0e7c4adf8e8ee49fcea46f5383a9160ac'),
'Loot.log':(17699647,90843,'f7f105a543618d43ff78fcad2678dc51f3dff4b376b3209237d449b10123cfff'),
'Anticheat.log':(71290,587,'2ab937e89b61cee70b8684830b763e34318642a3459f551fc506297caafd3b99'),
'DBErrors.log':(71638,629,'601afa8b314254871a9bf06118796d28a9dedf3bfa071d90bb129f687d9f501c')}
timestamp=re.compile(r'^\d{4}-\d\d-\d\d \d\d:\d\d:\d\d')
sources={}; rows={}
for name,(size,linecount,sha) in EXPECTED.items():
    data=(args.logs/name).read_bytes()
    lines=data.decode('utf-8',errors='replace').splitlines()
    digest=hashlib.sha256(data).hexdigest()
    sources[name]={'bytes':len(data),'raw_lines':len(lines),'sha256':digest,
        'matches_handoff':len(data)==size and len(lines)==linecount and digest==sha}
    assert sources[name]['matches_handoff'], (name,sources[name])
    current=''; rows[name]=[]
    for n,line in enumerate(lines,1):
        if timestamp.match(line): current=line[:19]
        if START<=current<=END: rows[name].append((n,current,line))

def count(name,pattern): return sum(bool(re.search(pattern,r[2],re.I)) for r in rows[name])
def moments(name,pattern): return [{'line':n,'time':t} for n,t,s in rows[name] if re.search(pattern,s)]
def diffs(records):
    result=[]
    for i,(n,t,s) in enumerate(records):
        m=re.search(r'Avg diff \(10 sec\): (\d+)',s)
        if not m: continue
        block='\n'.join(x[2] for x in records[i:i+5])
        m60=re.search(r'Avg diff \(60 sec\): (\d+)',block)
        bots=re.search(r'Bots online: (\d+) \(active: (\d+)\)',block)
        assert m60 and bots,(n,t)
        result.append({'line':n,'time':t,'diff10':int(m[1]),'diff60':int(m60[1]),
                       'online':int(bots[1]),'active':int(bots[2])})
    return result
def summary(seq):
    return {'n':len(seq),'first':seq[0]['time'],'last':seq[-1]['time'],
        **{key:sum(v[key] for v in seq)/len(seq) for key in ['active','diff10','diff60']}}
def seconds(a,b): return int((datetime.fromisoformat(b)-datetime.fromisoformat(a)).total_seconds())
server=rows['Server.log']
all_diff=diffs(server)
clean=[x for x in all_diff if WORLD<=x['time']<ON]
world_dt=datetime.fromisoformat(WORLD)
window=lambda a,b:[x for x in clean if (world_dt+timedelta(minutes=a)).isoformat(' ')<=x['time']<(world_dt+timedelta(minutes=b)).isoformat(' ')]
slow=[]
for n,t,s in rows['Perf.log']:
    m=re.search(r'Slow world update: (\d+)ms',s)
    if m: slow.append({'line':n,'time':t,'ms':int(m[1])})
mature=[x for x in slow if (world_dt+timedelta(minutes=30)).isoformat(' ')<=x['time']<ON]
pending=[]
for n,t,s in server:
    m=re.search(r'Pending packets: total=(\d+) \| avg/bot=([\d.]+) \| max/bot=(\d+)',s)
    if m: pending.append({'line':n,'time':t,'total':int(m[1]),'average':float(m[2]),'maximum':int(m[3])})
item_guids=set()
for n,t,s in server:
    m=re.search(r'Item::AddToUpdateQueueOf - Item \(Guid: (\d+)\)',s)
    if m:item_guids.add(m[1])
use=Counter()
for n,t,s in server:
    m=re.search(r'UseItemAction:.*result (\d+)',s)
    if m:use[m[1]]+=1
go=[]; go_bots=set(); go_objects=set()
for n,t,s in rows['Loot.log']:
    m=re.search(r'loot from Gameobject \(Entry: (\d+) Guid: (\d+)\)',s)
    if m:
        go.append(n); go_objects.add((m[1],m[2]))
        bot=re.search(r', guid (\d+), name ',s)
        assert bot
        go_bots.add(bot[1])
profile={}
for i,(n,t,s) in enumerate(server):
    if t=='2026-09-25 06:56:42':
        m=re.search(r'Instance 11: (\d+) bots \((\d+) active\).*Work avg: ([\d.]+) ms',s)
        if m:
            players=re.search(r'players=([\d.]+) ms',server[i+1][2])
            profile={'line':n,'time':t,'bots':int(m[1]),'active':int(m[2]),'work_ms':float(m[3]),'players_ms':float(players[1]),'world_region':'Kalimdor, not a raid instance'}
printed={}
for name,metric in [('grind target','V'),('need for quest','V'),('attackers','V'),('attackers count','V'),('need quest objective','V'),('check values','A'),('add gathering loot','A')]:
    total=Decimal(0); calls=0; source_lines=[]
    pattern=re.compile(r'\s([\d.]+)s \|.*of\s+(\d+)\) - '+metric+r'\s+: '+re.escape(name)+r'(?=\s*(?:\[|<|\d|$))')
    for n,t,s in server:
        m=pattern.search(s)
        if m: total+=Decimal(m[1]); calls+=int(m[2]); source_lines.append(n)
    printed[name]={'metric':metric,'seconds':str(total),'count':calls,'source_lines':source_lines}

report={'scope':{'start':START,'world_initialized':WORLD,'pmo_on':ON,'pmo_off':OFF,'end':END,
    'timezone':'timestamps used as logged; no conversion','raw_logs_in_output':False},'inputs':sources,
    'session':{'world_to_halting_seconds':seconds(WORLD,END),'process_seconds':seconds(START,END),
        'world_to_shutdown_seconds':seconds(WORLD,'2026-09-25 06:57:53'),'shutdown_to_halting_seconds':36,
        'pmo_seconds':seconds(ON,OFF),'pmo_commands':moments('Server.log',r'Performance monitor (reset|enabled|disabled)'),
        'core_revision_matches_v19':any('Core revision: 33faa8e3c091d73483b4' in r[2] for r in server)},
    'diff_snapshots':{'all_count':len(all_diff),'clean_count':len(clean),'clean_definition':'from world initialization until first PMO enable; all later snapshots excluded',
        'last10':summary(clean[-10:]),'minutes30_40':summary(window(30,40)),'minutes40_50':summary(window(40,50)),
        'maximum_active':max(clean,key=lambda r:r['active']),'last_clean':clean[-1],
        'after_pmo': [r for r in all_diff if r['time']>=ON], 'clean_samples':clean},
    'perf':{'slow_world_lines':len(slow),'update_map_lines':count('Perf.log',r'Update map system:'),
        'total_session_lines':len(rows['Perf.log']),'all_max':max(slow,key=lambda r:r['ms']),
        'mature_gt200':[r for r in mature if r['ms']>200], 'mature_gt500':sum(r['ms']>500 for r in mature),
        'mature_max':max(mature,key=lambda r:r['ms']),
        'pmo_print_second_max':max([r for r in slow if r['time']=='2026-09-25 06:57:13'],key=lambda r:r['ms'])},
    'queues':{'general_profiles':count('Server.log',r'CPU / map partition profile'),
        'partition_samples':len(pending),'all_sampled_zero':all(r['total']==r['maximum']==r['average']==0 for r in pending)},
    'last_clean_profile_instance11':profile,
    'loot':{'creature_records':count('Loot.log',r'loot from Creature \('),'gameobject_records':len(go),
        'gameobject_distinct_bot_guids':len(go_bots),'gameobject_distinct_entry_guid_pairs':len(go_objects)},
    'server_warnings':{'add_cooldown':count('Server.log','AddCooldown'),'use_item':count('Server.log','UseItemAction'),
        'use_item_results':dict(sorted(use.items())),'item_update_queue':count('Server.log','Item::AddToUpdateQueueOf'),
        'item_update_queue_distinct_guids':len(item_guids),'sql_error':count('Server.log','SQL ERROR'),
        'duplicate_entry':count('Server.log','Duplicate entry'),'movespline_path_size':count('Server.log',r"path\.size\(\) > 1"),
        'cancelled_instance_creation':count('Server.log','Scheduled instance creation'),
        'guild_names_exhausted':count('Server.log','No more names left for random guilds'),
        'find_script_targets':count('Server.log','FindScriptTargets'),'broken_zone_data':count('Server.log','broken zone-data'),
        'escort_warning':count('Server.log','ScriptedEscortAI.GetCombatStartPosition')},
    'db_errors':{'total':len(rows['DBErrors.log']),'first':rows['DBErrors.log'][0][1],'last':rows['DBErrors.log'][-1][1],
        'unused_gossip_menu':count('DBErrors.log',r'gossip_menu.*unused'),'missing_item_loot_template':count('DBErrors.log',r'item_loot_template.*not exist'),
        'same_proc_flags':count('DBErrors.log','same proc flags'),'empty_reputation_reward':count('DBErrors.log',r'reputation_reward_rate.*empty')},
    'anticheat':{'session_lines':len(rows['Anticheat.log']),'items_check_lines':count('Anticheat.log','ItemsCheck'),
        'non_unit_loot_lines':count('Anticheat.log','CMSG_LOOT on non-unit guid')},'printed_pmo':printed,
    'limitations':['Arithmetic means of irregular logged snapshots; not all-tick distributions.',
        'Perf.log threshold samples cannot establish p95/p99 without denominator.',
        'Slow-world and map-update lines are distinct phases, not distinct lag events.',
        'Loot rows are item receipts, not unique kills or opened objects.',
        'PMO rows overlap and omit below-threshold entries; do not sum to CPU percentage.',
        'V18/V19 sessions are unpaired and do not establish a whole-server speedup.',
        'No crash shown before normal halting; this does not prove no leaks or all gameplay parity.',
        'One final Server.log network shutdown line at 06:58:30 lies outside handoff session cutoff.']}
previous_file=args.logs/'Server_V18.log'
if previous_file.exists():
    previous=previous_file.read_bytes()
    previous_lines=previous.decode('utf-8',errors='replace').splitlines()
    previous_rows=[]; current=''
    for n,line in enumerate(previous_lines,1):
        if timestamp.match(line): current=line[:19]
        previous_rows.append((n,current,line))
    previous_on=next(t for n,t,s in previous_rows if 'Performance monitor enabled' in s)
    previous_world=next(t for n,t,s in previous_rows if 'World initialized' in s)
    previous_clean=[r for r in diffs(previous_rows) if previous_world<=r['time']<previous_on]
    report['v18_context_recheck']={'sha256':hashlib.sha256(previous).hexdigest(),'bytes':len(previous),
        'last10':summary(previous_clean[-10:]),'comparison':'unpaired; informational context only'}
checks={
    'last10':report['diff_snapshots']['last10']['n']==10 and report['diff_snapshots']['last10']['active']==2363.8 and report['diff_snapshots']['last10']['diff10']==99.4 and report['diff_snapshots']['last10']['diff60']==99.0,
    'window30_40':tuple(round(report['diff_snapshots']['minutes30_40'][k],2) for k in ('n','active','diff10','diff60'))==(7,2423.86,97.71,99.86),
    'window40_50':tuple(round(report['diff_snapshots']['minutes40_50'][k],2) for k in ('n','active','diff10','diff60'))==(28,2366.96,102.25,101.86),
    'max_active':report['diff_snapshots']['maximum_active']['active']==2445,
    'perf_counts':len(slow)==15298 and report['perf']['update_map_lines']==14854 and len(report['perf']['mature_gt200'])==12 and report['perf']['mature_gt500']==0,
    'loot_counts':report['loot']==dict(creature_records=2660,gameobject_records=96,gameobject_distinct_bot_guids=58,gameobject_distinct_entry_guid_pairs=67),
    'queues':report['queues']==dict(general_profiles=9,partition_samples=117,all_sampled_zero=True),
    'warnings':report['server_warnings']==dict(add_cooldown=647,use_item=269,use_item_results={'100':161,'19':2,'30':19,'60':86,'96':1},item_update_queue=39,item_update_queue_distinct_guids=16,sql_error=0,duplicate_entry=0,movespline_path_size=1,cancelled_instance_creation=4,guild_names_exhausted=4,find_script_targets=2,broken_zone_data=6,escort_warning=1),
    'db':report['db_errors']['total']==629 and report['db_errors']['unused_gossip_menu']==448 and report['db_errors']['missing_item_loot_template']==178,
    'anticheat':report['anticheat']==dict(session_lines=7,items_check_lines=1,non_unit_loot_lines=1),
    'pmo':all((printed[n]['seconds'],printed[n]['count'])==(s,c) for n,s,c in [
        ('grind target','4.737',6218),('need for quest','3.776',14826),('attackers','2.351',19651),
        ('attackers count','1.935',19324),('need quest objective','1.394',274739),
        ('check values','0.981',2654),('add gathering loot','1.658',10780)])}
checks['duration_and_pmo']=(report['session']['world_to_halting_seconds'],report['session']['process_seconds'],
    report['session']['world_to_shutdown_seconds'],report['session']['pmo_seconds'])==(3127,3207,3091,26)
checks['spikes']=(report['perf']['all_max']['ms'],report['perf']['mature_max']['ms'],report['perf']['pmo_print_second_max']['ms'])==(636,281,464)
checks['profile']=(profile['bots'],profile['active'],profile['work_ms'],profile['players_ms'])==(689,534,95.26,90.58)
checks['last_clean']=(clean[-1]['active'],clean[-1]['diff10'],clean[-1]['diff60'],clean[-1]['time'])==(2368,96,102,'2026-09-25 06:56:40')
checks['after_pmo']=(report['diff_snapshots']['after_pmo'][-1]['diff10'],report['diff_snapshots']['after_pmo'][-1]['diff60'],report['diff_snapshots']['after_pmo'][-1]['active'])==(153,102,2169)
if 'v18_context_recheck' in report:
    checks['v18_context']=tuple(report['v18_context_recheck']['last10'][k] for k in ('n','active','diff10','diff60'))==(10,2145.8,94.3,93.7)
report['handoff_claim_checks']=checks
report['all_checked_claims_match']=all(checks.values())
args.output.write_text(json.dumps(report,indent=2),encoding='utf-8')
print(json.dumps({'input_hashes_match':all(x['matches_handoff'] for x in sources.values()),
                 'handoff_checks':checks,'output':str(args.output)},indent=2))
assert report['all_checked_claims_match'],checks
