"""Freeze the two V23 patches; legacy tests still verify their original scopes.

Historical suites normalize only these whole-file hashes to V22. The new V23
suites compile the actual changed bodies. All other production source is frozen.
"""
from pathlib import Path
import hashlib,json,subprocess
ROOT=Path(__file__).resolve().parents[3]
BASE='24e11eb41b85ab74437dfc1737289c2bcca42de4'
APPROVED={
 'src/game/PlayerBots/playerbot/strategy/values/AttackersValue.cpp':'c2d5e25cccfa67d0934b05727c380bea5dcde90a84fb2a599f87effe65d2297a',
 'src/game/PlayerBots/playerbot/strategy/Engine.cpp':'b3a474914e2d41a206cd7ef4302c91e618176f8547e99ac1a5940cb1fc8c7398',
}

def baseline(path):return subprocess.check_output(['git','show',BASE+':'+path],cwd=ROOT).decode('utf-8-sig').replace('\r\n','\n')

def normalize_v23(path,text):
 if path not in APPROVED:return text
 old=baseline(path)
 assert text==old or hashlib.sha256(text.encode()).hexdigest()==APPROVED[path],'Unaudited V23 source: '+path
 return old

def verify():
 changed=subprocess.check_output(['git','diff','--name-only',BASE,'--','src'],cwd=ROOT).decode().splitlines()
 untracked=subprocess.check_output(['git','ls-files','--others','--exclude-standard','--','src'],cwd=ROOT).decode().splitlines()
 assert not untracked,'Untracked production source: '+str(untracked)
 evidence={'baseline':BASE,'changed_source':{},'other_production_sources':'byte-identical to V22 (normalized line endings); generated client header checked separately','historical_normalization':'only the two frozen whole files; actual V23 bodies execute in separate normal/ASan suites'}
 for p in changed:
  text=(ROOT/p).read_text(encoding='utf-8-sig')
  if p=='src/shared/Progression.h':
   template=(ROOT/'cmake/generators/Progression.h.in').read_text(encoding='utf-8-sig')
   assert text in [baseline(p),template.replace('@supported_build@','5875')];continue
  assert p in APPROVED,'Protected source changed: '+p
  normalize_v23(p,text);evidence['changed_source'][p]=hashlib.sha256(text.encode()).hexdigest()
 return evidence

if __name__=='__main__':print(json.dumps(verify(),indent=2))
