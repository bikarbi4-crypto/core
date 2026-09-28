"""Compile the complete baseline/current production LogAction method."""
from pathlib import Path
import hashlib,json,re,subprocess,sys
R=Path(__file__).resolve().parents[4]
BASE='24e11eb41b85ab74437dfc1737289c2bcca42de4'
PATH='src/game/PlayerBots/playerbot/strategy/Engine.cpp'
out=Path(sys.argv[1]);out.mkdir(parents=True,exist_ok=True)
def extract(s):
 start=s.index('void Engine::LogAction(');i=s.index('{',start)+1;depth=1
 mask=re.sub(r'//[^\n]*|/\*.*?\*/|"(?:\\.|[^"\\])*"',lambda m:' '*len(m[0]),s,flags=re.S)
 while depth:depth+=(mask[i]=='{')-(mask[i]=='}');i+=1
 return s[start:i]
base=subprocess.check_output(['git','show',BASE+':'+PATH],cwd=R).decode('utf-8-sig')
current=(R/PATH).read_text(encoding='utf-8-sig')
old,new=extract(base),extract(current)
assert base.replace(old,'')==current.replace(new,''),'Changes outside LogAction'
manifest={'baseline':BASE,'path':PATH,'variants':{},'world_fixtures':['bot/group/name getters','logger and test-file sinks'],'production':'complete LogAction including formatting, history maintenance and both logging branches'}
for ns,source,body in [('v22',base,old),('v23',current,new)]:
 (out/(ns+'.inc')).write_text('__declspec(noinline) '+body+'\n',encoding='utf-8')
 manifest['variants'][ns]={'source_sha256':hashlib.sha256(source.encode()).hexdigest(),'function_sha256':hashlib.sha256(body.encode()).hexdigest()}
(out/'provenance.json').write_text(json.dumps(manifest,indent=2),encoding='utf-8')

