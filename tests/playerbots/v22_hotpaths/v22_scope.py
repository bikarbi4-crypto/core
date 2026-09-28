"""Exact V22 source boundary; legacy suites normalize only this audited patch.
V22 actual-source suites execute every changed body independently. The frozen
hashes protect ALL text in these six files, not just PMO or function names.
"""
from pathlib import Path
import hashlib,json,subprocess,sys
ROOT=Path(__file__).resolve().parents[3]
sys.dont_write_bytecode=True
sys.path.insert(0,str(ROOT/'tests/playerbots/v23_hotpaths'))
from v23_scope import APPROVED as V23_FILES, normalize_v23, verify as verify_v23
BASE='6a32bb9c97d45166719a9b5a9d3e0f0c0c8ac4c7'
APPROVED={'src/game/PlayerBots/playerbot/strategy/values/QuestValues.cpp': '1f0c6dcfa0c7efab6afa55da3e6d7d2595ac0f9cb34a3bfdc5197b561126c58f', 'src/game/PlayerBots/playerbot/strategy/actions/MovementActions.cpp': 'b0cfc090842be506d1db16269c2f5f2d3cabaef0161014cd7b467fd1a237b238', 'src/game/PlayerBots/playerbot/strategy/actions/MovementActions.h': '9d5502bb2f7a18a19f072a4d8b207432828f7738e47b6d063935dbcd3d0f2629', 'src/game/PlayerBots/playerbot/TravelMgr.h': '188087df287832b47cb654e4e8cb42735ba829bcfd98bdb0301e0127165c3cec', 'src/game/PlayerBots/playerbot/strategy/actions/MoveToTravelTargetAction.cpp': 'a824774fac6fb63b323e735f8038dafb271b83a36f8106aa3519be91c8b9b66f', 'src/game/PlayerBots/playerbot/strategy/Event.h': '07571d833761a1c8b5b0a49a6e9681fdc55e5b1ad00c49b7a059fdacb1eeca88'}
def baseline(path):return subprocess.check_output(['git','show',BASE+':'+path],cwd=ROOT).decode('utf-8-sig').replace('\r\n','\n')
def normalize_v22(path,text):
 text=normalize_v23(path,text)
 if path not in APPROVED:return text
 old=baseline(path)
 assert text==old or hashlib.sha256(text.encode()).hexdigest()==APPROVED[path], 'Unaudited V22 source: '+path
 return old
def verify():
 v23=verify_v23()
 changed=subprocess.check_output(['git','diff','--name-only',BASE,'--','src'],cwd=ROOT).decode().splitlines()
 evidence={'baseline':BASE,'changed_source':{},'v23_scope':v23,'presence_and_other_sources':'byte-identical to V21 outside frozen V22/V23 changes'}
 for p in changed:
  text=(ROOT/p).read_text(encoding='utf-8-sig')
  if p=='src/shared/Progression.h':
   template=(ROOT/'cmake/generators/Progression.h.in').read_text(encoding='utf-8-sig')
   assert text in [baseline(p),template.replace('@supported_build@','5875')];continue
  if p in V23_FILES:
   normalize_v23(p,text);continue
  assert p in APPROVED,'Protected source changed: '+p
  normalize_v22(p,text);evidence['changed_source'][p]=hashlib.sha256(text.encode()).hexdigest()
 return evidence
if __name__=='__main__':print(json.dumps(verify(),indent=2))
