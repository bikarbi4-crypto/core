"""Use actual PlayerbotAI layout, constructors (including ChatHelper/filter work),
AiObject, NamedObjectContext and Value policies. Only uncalled game methods and
world-dependent Calculate are fixtures. Baseline new/factory/delete is unchanged.
"""
from pathlib import Path
import hashlib,json,re,subprocess,sys
root=Path(__file__).resolve().parents[3]
p='src/game/PlayerBots/playerbot/'
out=Path(sys.argv[1]);out.mkdir(parents=True,exist_ok=True)
(out/'revision.h').write_text('#pragma once\n#define REVISION_HASH "v20-constructor-test"\n#define REVISION_DATE "test"\n#define REVISION_TIME "test"\n#define SUPPORTED_CLIENT_BUILD 5875\n')
base='33faa8e3c091d73483b42dc07f8f2934ba5f7958'
manifest={'baseline':base,'sources':{},'fragments':{},'limits':['World/database Calculate fixtures; never initialize a world.','Uncalled virtual game methods abort. Actual AI class and all member constructors retained.','Registry narrowed to actual item drop list and global string registrations, plus test factories.']}
sys.dont_write_bytecode=True
sys.path.insert(0,str(root/'tests/playerbots/v21_presence'))
from source_scope import normalize_v21, verify
manifest['v21_scope']=verify()

def read(file,baseline=False):
 s=subprocess.check_output(['git','show',base+':'+p+file],cwd=root).decode('utf-8-sig') if baseline else (root/p/file).read_text(encoding='utf-8-sig')
 manifest['sources'][('baseline:' if baseline else 'candidate:')+file]=hashlib.sha256(s.encode()).hexdigest();return s
def mask(s):
 return re.sub(r'"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'|//[^\n]*|/\*.*?\*/',lambda m:''.join('\n' if c=='\n' else ' ' for c in m[0]),s,flags=re.S)
def block(s,sig):
 a=s.index(sig);b=s.index('{',a);m=mask(s);e=b+1;depth=1
 while depth:depth+=(m[e]=='{')-(m[e]=='}');e+=1
 return s[a:e]
def function(file,sig):
 s=block(read(file),sig);manifest['fragments'][sig]=hashlib.sha256(s.encode()).hexdigest();return s
for unchanged in ['PlayerbotAI.h','PlayerbotAI.cpp','PlayerbotAIBase.h','PlayerbotAIBase.cpp','ChatHelper.h','ChatHelper.cpp','ChatFilter.h','ChatFilter.cpp','PlayerbotSecurity.h','PlayerbotSecurity.cpp','PlayerbotAIAware.h','WorldPosition.h','WorldPosition.cpp','strategy/AiObject.h','strategy/AiObject.cpp','strategy/NamedObjectContext.h','strategy/NamedObjectCache.h','strategy/Value.h','strategy/values/LootValues.h','strategy/values/VendorValues.h','PerformanceMonitor.h']:
 assert normalize_v21(unchanged,read(unchanged))==read(unchanged,True),unchanged+' changed outside shared ownership scope'
oldregistry=re.findall(r'creators\[[^\n]+',read('strategy/values/SharedValueContext.h',True))
assert len(oldregistry)==16
assert oldregistry==re.findall(r'creators\[[^\n]+',read('strategy/values/SharedValueContext.h'))
manifest['registry_preserved']=oldregistry
# Visibility-only access for assertions of actual metric maps; no layout/body change.
(out/'audit_monitor.h').write_text(read('PerformanceMonitor.h').replace('private:', 'public:'),encoding='utf-8')
constructors=['#include "playerbot/playerbot.h"\n#include "playerbot/strategy/values/LootValues.h"\n#include <cstdlib>\nusing namespace ai;']
for file,sig in [('PlayerbotAI.cpp','PlayerbotAI::PlayerbotAI()'),('PlayerbotAI.cpp','PlayerbotAI::~PlayerbotAI()'),('PlayerbotAIBase.cpp','PlayerbotAIBase::PlayerbotAIBase()'),('ChatHelper.cpp','ChatHelper::ChatHelper('),('PlayerbotSecurity.cpp','PlayerbotSecurity::PlayerbotSecurity('),('strategy/AiObject.cpp','AiObject::AiObject('),('strategy/AiObject.cpp','Player* AiObject::GetMaster('),('WorldPosition.cpp','void WorldPosition::add()'),('WorldPosition.cpp','void WorldPosition::rem()')]:
 fragment=function(file,sig)
 if sig=='PlayerbotAI::PlayerbotAI()':
  fragment=fragment.replace('{','{\n#ifdef SHARED_ALLOCATIONS\nif(SharedCounters::active) ++SharedCounters::aiConstructions;\n#endif\n',1)
 constructors.append(fragment)
helper=read('ChatHelper.cpp')
constructors.insert(1,helper[helper.index('std::map<std::string, uint32> ChatHelper::consumableSubClasses;'):helper.index('template<class T>')])
filters=read('ChatFilter.cpp')
for name in re.findall(r'^class (\w+ChatFilter) : public ChatFilter',filters,re.M):
 c=block(filters,'class '+name+' : public ChatFilter')+';'
 # Preserve full member declarations and constructors, replacing only game-facing Filter bodies.
 for match in list(re.finditer(r'(?:virtual )?std::string Filter\([^\n]*',c))[::-1]:
  f=block(c,match[0]);sig=f[:f.index('{')];c=c.replace(f,sig+'{ std::abort(); }')
 constructors.append(c)
constructors += [function('ChatFilter.cpp','CompositeChatFilter::CompositeChatFilter('),function('ChatFilter.cpp','CompositeChatFilter::~CompositeChatFilter()')]
constructors.append('''
INSTANTIATE_SINGLETON_1(PlayerbotAIConfig);
namespace MaNGOS {
void at_exit(void (*f)()) { std::atexit(f); }
template<> ObjectMgr& Singleton<ObjectMgr>::Instance() { std::abort(); }
template<> TerrainManager& Singleton<TerrainManager, ClassLevelLockable<TerrainManager, std::mutex>>::Instance() { std::abort(); }
namespace Errors { void PrintStacktraceAndThrow(char const*, int, char const*, char const*, char const*) { std::abort(); } }
}
bool Object::PrintIndexError(uint32,bool) const { std::abort(); }
uint32 Player::CalculateTalentsPoints() const { std::abort(); }
uint16 TerrainInfo::GetAreaFlag(float,float,float,bool*) const { std::abort(); }
TerrainInfo* TerrainManager::LoadTerrain(uint32) { std::abort(); }
bool TalentSpec::CheckTalents(uint32, std::ostringstream*) { std::abort(); }
std::string WorldPosition::print() const { std::abort(); }
bool WorldPosition::isVmapLoaded(uint32,int,int) { std::abort(); }
bool WorldPosition::loadVMap(uint32,int,int) { std::abort(); }
bool Engine::DoNextAction(Unit*,int,bool,bool) { std::abort(); }
Action* Engine::InitializeAction(ActionNode*) { std::abort(); }
bool Engine::ListenAndExecute(Action*,Event&) { std::abort(); }
bool PlayerbotAI::HasAura(std::string,Unit*,bool,bool,int,bool,int,int) { std::abort(); }
bool PlayerbotAI::HasAnyAuraOf(Unit*,...) { std::abort(); }
bool PlayerbotAI::HasAuraToDispel(Unit*,uint32) { std::abort(); }
bool PlayerbotAI::CanCastSpell(std::string,Unit*,uint8,Item*,bool,bool,bool,SpellCastResult*) { std::abort(); }
bool PlayerbotAI::CastSpell(std::string,Unit*,Item*,bool,uint32*) { std::abort(); }
bool PlayerbotAI::IsInterruptableSpellCasting(Unit*,std::string,uint8) { std::abort(); }
void PlayerbotAI::UpdateAI(uint32, bool) { std::abort(); }
void PlayerbotAI::UpdateAIInternal(uint32, bool) { std::abort(); }
bool PlayerbotAI::DoSpecificAction(const std::string&, Event, bool) { std::abort(); }
void PlayerbotAIBase::UpdateAI(uint32) { std::abort(); }
void PlayerbotAIBase::UpdateAIInternal(uint32, bool) { std::abort(); }
std::string ChatFilter::Filter(std::string, std::string) { std::abort(); }
std::string CompositeChatFilter::Filter(std::string) { std::abort(); }
Engine::~Engine() { std::abort(); }
uint32 ObjectMgr::GetPlayerAccountIdByGUID(ObjectGuid) const { std::abort(); }
PlayerbotAIConfig::PlayerbotAIConfig() { perfMonEnabled=false; }
extern std::list<int32> fixtureDropCalculate();
std::list<int32> ItemDropListValue::Calculate() { return fixtureDropCalculate(); }
''')
(out/'constructors.cpp').write_text('\n\n'.join(constructors),encoding='utf-8')
monitor=['#include "playerbot/playerbot.h"\nusing namespace ai;']
for sig in ['PerformanceMonitor::PerformanceMonitor()','PerformanceMonitor::~PerformanceMonitor()','std::unique_ptr<PerformanceMonitorOperation> PerformanceMonitor::start(PerformanceMetric metric, std::string name, PerformanceStack*','std::unique_ptr<PerformanceMonitorOperation> PerformanceMonitor::start(PerformanceMetric metric, std::string name, PlayerbotAI','PerformanceMonitorOperation::PerformanceMonitorOperation(','PerformanceMonitorOperation::~PerformanceMonitorOperation()','void PerformanceMonitorOperation::finish()','void PerformanceMonitor::Reset()']:
 assert block(read('PerformanceMonitor.cpp'),sig)==block(read('PerformanceMonitor.cpp',True),sig),sig
 monitor.append(function('PerformanceMonitor.cpp',sig))
(out/'monitor.cpp').write_text('\n\n'.join(monitor),encoding='utf-8')
for variant in ['baseline','candidate']:
 s=read('strategy/values/SharedValueContext.h',variant=='baseline')
 s=re.sub(r'^#include[^\n]*','',s,flags=re.M)
 registry=block(s,'SharedValueContext()')
 selected='''SharedValueContext() : NamedObjectContext(true) {
 creators["item drop list"] = [](PlayerbotAI* ai) { return new ItemDropListValue(ai); };
 creators["global string"] = [](PlayerbotAI* ai) { return new StringManualSetValue(ai); };
 InstallFixtureCreators(creators);
 }'''
 s=s.replace(registry,selected)
 (out/(variant+'.h')).write_text(s,encoding='utf-8')
(out/'provenance.json').write_text(json.dumps(manifest,indent=2),encoding='utf-8')
