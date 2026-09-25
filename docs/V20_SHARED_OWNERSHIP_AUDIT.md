# V20: ownership audit of all shared value registrations

Reviewed against V19 `33faa8e3c091d73483b42dc07f8f2934ba5f7958` and the V20
`SharedValueContext.h` change. Scope: all 16 registered concrete classes, their
constructors, calculation/read policies, helpers, retained pointers and teardown.
This is a source/lifetime audit; test and ASan results are reported separately.

The owned **botless default PlayerbotAI per SharedObjectContext** is compatible
with these 16 registrations. None of their concrete calculation bodies calls a
method on the retained `ai`, `chat`, `context` or `bot`, and none writes gameplay
state into that AI. They use world/static data, their own qualifier and result
storage, and nested shared-value reads. After construction, the inherited
SingleCalculatedValue PMO path is the relevant use of the retained AI. The
manual string policy does not invoke that PMO path. This conclusion applies to
the audited registrations, not every possible future factory or PlayerbotAI API.

## Why a retained context is required

[AiObject.cpp:6](../src/game/PlayerBots/playerbot/strategy/AiObject.cpp#L6)
initializes four relationships: the PlayerbotAIAware base stores `ai`; `bot` is
`ai->GetBot()`; `context` is `ai->GetAiObjectContext()`; `chat` is the address
returned by `ai->GetChatHelper()`. Thus even a value that never consults a bot
cannot safely be constructed with a null AI. An AI on the lookup stack, or the
previous `new AI -> factory/cache -> delete AI` pattern, also cannot provide the
required retained lifetime. The chat pointer addresses a member of that AI.

The real default constructor at
[PlayerbotAI.cpp:118](../src/game/PlayerBots/playerbot/PlayerbotAI.cpp#L118)
sets bot and AI context to null, constructs ChatHelper/CompositeChatFilter, clears
engine pointers and initializes activity timers. It does not attach a Player,
create a per-bot AiObjectContext, initialize engines or start an AI update loop.
Its accessors return these fields directly
([PlayerbotAI.h:566](../src/game/PlayerBots/playerbot/PlayerbotAI.h#L566),
[615](../src/game/PlayerBots/playerbot/PlayerbotAI.h#L615),
[618](../src/game/PlayerBots/playerbot/PlayerbotAI.h#L618)).

This constructor is not empty. In particular,
[ChatHelper.cpp:38](../src/game/PlayerBots/playerbot/ChatHelper.cpp#L38) assigns
fixed lookup-table entries; the tables are **static**, as declared at
[ChatHelper.h:120](../src/game/PlayerBots/playerbot/ChatHelper.h#L120).
Repeated temporary AI construction previously repeated those assignments.
The change avoids that repeated work; it does not turn previously per-value
tables into new shared tables. The audited value bodies do not call ChatHelper
parsers or modify these tables. Existing concurrent ChatHelper construction or
access elsewhere is outside the new lookup lock's guarantee.

[CompositeChatFilter implementation](../src/game/PlayerBots/playerbot/ChatFilter.cpp#L1183)
allocates the existing filter objects; its destructor deletes them at line 1203.
No registered shared value calls their Filter methods. Activity timers and other
AI fields therefore do not become a communication channel between the audited
values. The owner does not expose its AI as a game-update object.

## Complete registration inventory

All registrations are in
[SharedValueContext.h:21](../src/game/PlayerBots/playerbot/strategy/values/SharedValueContext.h#L21).
There are 15 SingleCalculatedValue policies and one ManualSetValue policy.
“No context use” below concerns the concrete Calculate/Format body; base
construction still captures the four pointers described above, and the single
calculation base retains its PMO behavior.

| Registered key | Concrete class and policy | Calculation and dependencies | Ownership / retained context |
|---|---|---|---|
| `bg masters` | BgMastersValue; SingleCalculatedValue of list of const CreatureDataPair pointers; Qualified | [PvpValues.cpp:14](../src/game/PlayerBots/playerbot/strategy/values/PvpValues.cpp#L14): qualifier selects battleground; copies battlemaster cache; searches WorldPosition creature data for all teams. | No context use. List owns its nodes, not ObjectMgr creature records. No custom destructor. |
| `item drop map` | ItemDropMapValue; SingleCalculatedValue of DropMap pointer | [LootValues.cpp:66](../src/game/PlayerBots/playerbot/strategy/values/LootValues.cpp#L66): allocates a map, enumerates item prototypes and loot templates. | No context use. Returns raw allocation; **no deleting destructor** in this class. Existing lifetime limitation, below. |
| `drop map` | DropMapValue; SingleCalculatedValue of DropMap pointer | [LootValues.cpp:101](../src/game/PlayerBots/playerbot/strategy/values/LootValues.cpp#L101): creature/GO loot templates, then nested shared `item drop map`. | No context use. [LootValues.h:53](../src/game/PlayerBots/playerbot/strategy/values/LootValues.h#L53) deletes its current map in the destructor; elements are integer pairs. |
| `item drop list` | ItemDropListValue; SingleCalculatedValue of list of int32; Qualified | [LootValues.cpp:165](../src/game/PlayerBots/playerbot/strategy/values/LootValues.cpp#L165): parses item qualifier; reads shared `drop map`; copies matching entries. | No context use. Own list. Existing specialized IsEmpty observes warm storage, uses Get on cold/reset. |
| `entry loot list` | EntryLootListValue; SingleCalculatedValue of list of uint32; Qualified | [LootValues.cpp:182](../src/game/PlayerBots/playerbot/strategy/values/LootValues.cpp#L182): parses entry qualifier and scans shared `drop map`. | No context use. Own list; default container destruction. |
| `loot chance` | LootChanceValue; SingleCalculatedValue of float; Qualified | [LootValues.cpp:201](../src/game/PlayerBots/playerbot/strategy/values/LootValues.cpp#L201): parses entry/item qualifier, reads loot template entries/groups. | No context use. Scalar result; no owned external resource. |
| `vendor map` | VendorMapValue; SingleCalculatedValue of VendorMap pointer | [VendorValues.cpp:9](../src/game/PlayerBots/playerbot/strategy/values/VendorValues.cpp#L9): allocates map from creature/vendor templates, ignores limited-stock items. | No context use. Returns raw allocation; **no deleting destructor** in this class. Existing lifetime limitation, below. |
| `item vendor list` | ItemVendorListValue; SingleCalculatedValue of list of int32; Qualified | [VendorValues.cpp:44](../src/game/PlayerBots/playerbot/strategy/values/VendorValues.cpp#L44): parses item qualifier; reads shared `vendor map`. | No context use. Own list. Existing IsEmpty warm/cold/reset policy unchanged. |
| `entry quest relation` | EntryQuestRelationMapValue; SingleCalculatedValue of nested map | [QuestValues.cpp:13](../src/game/PlayerBots/playerbot/strategy/values/QuestValues.cpp#L13): global quest relations/templates and objectives; nested `item drop list`, `loot chance`, `item vendor list`; static ItemCreatedFrom helper. | No context use. Own map of integer keys/flags. Internal PMO name remains `entry quest relation map`, distinct from registration key. |
| `quest guidp map` | QuestGuidpMapValue; SingleCalculatedValue of nested map/list | [QuestValues.cpp:163](../src/game/PlayerBots/playerbot/strategy/values/QuestValues.cpp#L163): local FindQuestObjectData worker; ObjectMgr creature/GO enumeration. Worker constructor reads shared `entry quest relation` ([line 132](../src/game/PlayerBots/playerbot/strategy/values/QuestValues.cpp#L132)). | No context use. Own GuidPosition values and containers; worker/result do not retain the AI. |
| `quest givers` | QuestGiversValue; SingleCalculatedValue of map/list; Qualified | [QuestValues.cpp:172](../src/game/PlayerBots/playerbot/strategy/values/QuestValues.cpp#L172): optional level qualifier; reads shared `quest guidp map`, copies/filter positions using global quest templates. | No context use. Own returned positions/containers. |
| `trainable spell map` | TrainableSpellMapValue; SingleCalculatedValue of trainableSpellMap pointer | [TrainerValues.cpp:10](../src/game/PlayerBots/playerbot/strategy/values/TrainerValues.cpp#L10): global creature/trainer/spell data; constructs map of trainer spell pointers to trainer entries. | No context use. [TrainerValues.h:17](../src/game/PlayerBots/playerbot/strategy/values/TrainerValues.h#L17) deletes the map. Container destruction does not delete borrowed TrainerSpell records or invoke world/AI methods. |
| `entry travel purpose` | EntryTravelPurposeMapValue; SingleCalculatedValue of map | [TravelValues.cpp:38](../src/game/PlayerBots/playerbot/strategy/values/TravelValues.cpp#L38): shared `entry quest relation` and `entry guidps`; global creature/GO metadata and static skill/lock helper. | No context use. Own integer purpose map. No retained AI in helper inputs/results. |
| `entry guidps` | EntryGuidpsValue; SingleCalculatedValue of map/vector | [TravelValues.cpp:11](../src/game/PlayerBots/playerbot/strategy/values/TravelValues.cpp#L11): WorldPosition creature/GO data enumeration; constructs AsyncGuidPosition, tests event spawn and captures area data. | No context use. Own position values; existing global world/event/area dependencies remain. |
| `full mount list` | FullMountListValue; SingleCalculatedValue of vector of MountValue | [MountValues.cpp:209](../src/game/PlayerBots/playerbot/strategy/values/MountValues.cpp#L209): global item/spell enumeration; static mount helpers; intentionally reads `item vendor list` for mount items. | No context use. MountValue holds spell ID and borrowed const ItemPrototype pointer; it does not retain PlayerbotAI. |
| `global string` | StringManualSetValue; ManualSetValue of string; Qualified | [Value.h:427](../src/game/PlayerBots/playerbot/strategy/Value.h#L427): Get/LazyGet return stored string, Set replaces it, Reset restores default, Format returns string. | No context use after construction; no PMO call. Existing mutable shared per-qualified-key string storage is preserved. Internal name remains `manual string`. |

The audit also read the called static/helper paths: DropMapValue::GetLootTemplate
([LootValues.cpp:12](../src/game/PlayerBots/playerbot/strategy/values/LootValues.cpp#L12)),
FindQuestObjectData constructor and workers
([QuestValues.h:49](../src/game/PlayerBots/playerbot/strategy/values/QuestValues.h#L49),
[QuestValues.cpp:131](../src/game/PlayerBots/playerbot/strategy/values/QuestValues.cpp#L131)),
ItemUsageValue::ItemCreatedFrom
([ItemUsageValue.cpp:873](../src/game/PlayerBots/playerbot/strategy/values/ItemUsageValue.cpp#L873)),
MountValue static speed/spell helpers
([MountValues.cpp:9](../src/game/PlayerBots/playerbot/strategy/values/MountValues.cpp#L9)),
the travel skill helper
([TravelValues.cpp:208](../src/game/PlayerBots/playerbot/strategy/values/TravelValues.cpp#L208)),
world data enumeration
([WorldPosition.cpp:1289](../src/game/PlayerBots/playerbot/WorldPosition.cpp#L1289)),
and GuidPosition/AsyncGuidPosition representation
([GuidPosition.h:7](../src/game/PlayerBots/playerbot/GuidPosition.h#L7)).
These paths do not recover or mutate the retained default AI indirectly.

## Policies, PMO and nested reads

[CalculatedValue](../src/game/PlayerBots/playerbot/strategy/Value.h#L72)
value-initializes `value{}`; pointer results start as null. SingleCalculatedValue
([Value.h:126](../src/game/PlayerBots/playerbot/strategy/Value.h#L126)) keeps the
existing timestamp, Reset, Set and first-read behavior. A first read sets the
timestamp, conditionally starts PMO and calls the concrete Calculate. Warm reads
return existing storage. V20 does not add a new result cache, change refresh
intervals, force Calculate, or change handling of nested reads/Calculate failure.

PMO's AI overload
([PerformanceMonitor.cpp:63](../src/game/PlayerBots/playerbot/PerformanceMonitor.cpp#L63))
first tests the enabled flag. For this valid botless AI, GetAiObjectContext is
null, so it uses the existing no-context overload, global map/instance bucket
and no per-bot performanceStack. It never attempts to obtain a map from the null
bot in this path. The V19 dangling AI could fail even on the first context-pointer
read; undefined behavior is not a parity requirement. No new stack shared by
these values is introduced by retaining one AI.

All nested shared reads above use GAI macros
([AiObjectContext.h:146](../src/game/PlayerBots/playerbot/strategy/AiObjectContext.h#L146)),
which explicitly return to `sSharedObjectContext`; they do not use the null
per-bot `context` field. Each value retains its own qualifier, timestamp and
result. The dependency graph's ordinary paths are downward: quest givers ->
quest guidp map -> relation map -> item lists -> drop/vendor maps, and drop map
-> item drop map. Full mount list warms vendor lists. This patch does not invent
a dependency cycle or suppress any of these calls.

The recursive mutex in GetUntypedValue serializes factory/cache access for an
**already created owner**, including recursive factory lookups. It preserves
NamedObjectContext's existing positive/negative entries, qualifier processing
and list priority
([NamedObjectContext.h:159](../src/game/PlayerBots/playerbot/strategy/NamedObjectContext.h#L159),
[203](../src/game/PlayerBots/playerbot/strategy/NamedObjectContext.h#L203),
[294](../src/game/PlayerBots/playerbot/strategy/NamedObjectContext.h#L294)).
The lock is released before the caller invokes Get/Set/Reset. Therefore it does
**not** serialize calculations, result mutation, PMO counter updates, or manual
strings. For example, RpgSubActions already reads/writes qualified global strings
([RpgSubActions.cpp:584](../src/game/PlayerBots/playerbot/strategy/actions/RpgSubActions.cpp#L584)).
Their existing concurrent behavior is not fixed or declared safe here.

## Destruction and exception boundaries

The declaration order in
[SharedValueContext.h:93](../src/game/PlayerBots/playerbot/strategy/values/SharedValueContext.h#L93)
is deliberate. Reverse destruction is:

1. `valueContexts`: the borrowed context list sees `IsShared()==true` and does not
   delete SharedValueContext
   ([NamedObjectContext.h:279](../src/game/PlayerBots/playerbot/strategy/NamedObjectContext.h#L279)).
2. `sharedValues`: its unique_ptr deletes the actual shared context. The virtual
   NamedObjectContext destructor calls Clear, deleting each non-null cached
   UntypedValue through the inherited virtual PlayerbotAIAware destructor
   ([NamedObjectContext.h:214](../src/game/PlayerBots/playerbot/strategy/NamedObjectContext.h#L214),
   [PlayerbotAIAware.h:16](../src/game/PlayerBots/playerbot/PlayerbotAIAware.h#L16)).
3. `valueAI`: only after every cached value is destroyed is the botless AI deleted.
   Its destructor sees null engines/context
   ([PlayerbotAI.cpp:263](../src/game/PlayerBots/playerbot/PlayerbotAI.cpp#L263));
   its own member destructors then release filters/containers.
4. The mutex is destroyed last.

The two concrete deleting value destructors, DropMapValue and
TrainableSpellMapValue, only delete their current result containers. They neither
dereference the retained AI/chat nor invoke a shared lookup/world callback.
Other registered value destructors are implicit/container destruction. This
supports safe local owner teardown even though the shared context was previously
only borrowed by NamedObjectContextList. Null `value{}` also makes destruction
of an uncalculated pointer value safe.

Unique ownership ensures that a failure constructing SharedValueContext or
adding it to the borrowed list unwinds the completed owners in the correct order.
This does not promise that every pre-existing raw allocation inside an arbitrary
PlayerbotAI/filter/value constructor is exception-safe under allocation failure.
Lookup/qualifier/Calculate exception behavior and partially populated value state
retain their pre-existing policy; the patch does not normalize it to a new rule.

Two existing raw-result limitations remain explicit: ItemDropMapValue and
VendorMapValue allocate maps but have no derived destructor deleting them;
Reset followed by recalculation of pointer-valued SingleCalculatedValue can
overwrite a previous result pointer without releasing it. Keeping the AI alive
does not solve those result-ownership issues. The release must not claim a full
shared-value memory-leak cleanup. Any broader result-ownership change needs its
own consumer/Reset/Set audit.

## Startup and concurrency boundary

Normal enabled startup initializes this owner before world updates:

- [Master.cpp:191](../src/mangosd/Master.cpp#L191) completes
  SetInitialWorldSettings before launching WorldRunnable at line 207.
- [World.cpp:1868](../src/game/World.cpp#L1868) calls PlayerbotAIConfig::Initialize.
- With bots enabled, AutoDoQuests defaults true
  ([PlayerbotAIConfig.cpp:623](../src/game/PlayerBots/playerbot/PlayerbotAIConfig.cpp#L623)).
  The call at [line 799](../src/game/PlayerBots/playerbot/PlayerbotAIConfig.cpp#L799)
  enters LoadQuestTravelTable, whose ordinary empty-destination startup path
  reaches unconditional shared reads at
  [TravelMgr.cpp:1307](../src/game/PlayerBots/playerbot/TravelMgr.cpp#L1307).
- The earlier RandomItemMgr initialization
  ([PlayerbotAIConfig.cpp:789](../src/game/PlayerBots/playerbot/PlayerbotAIConfig.cpp#L789),
  [RandomItemMgr.cpp:139](../src/game/PlayerBots/playerbot/RandomItemMgr.cpp#L139))
  can reach a shared vendor-list read at
  [RandomItemMgr.cpp:1200](../src/game/PlayerBots/playerbot/RandomItemMgr.cpp#L1200),
  but that particular path depends on eligible item/source data.

This is a conditional normal-startup argument, not proof for every disabled
configuration, alternate entry point or reload. The macro still uses the existing
MaNGOS Singleton default `SingleThreaded` policy
([SharedValueContext.h:101](../src/game/PlayerBots/playerbot/strategy/values/SharedValueContext.h#L101),
[Singleton.h:38](../src/framework/Policies/Singleton.h#L38),
[SingletonImp.h:37](../src/framework/Policies/SingletonImp.h#L37),
[ThreadingModel.h:59](../src/framework/Policies/ThreadingModel.h#L59)).
Its initial pointer check/creation occurs **before** GetUntypedValue can take the
owner mutex. Concurrent first creation, singleton destruction while in use and
use after shutdown are not protected by this patch. Tests of parallel lookup on
an already constructed owner cannot establish those separate guarantees.

Likewise, ASan supports the lifetime checks but does not prove thread safety of
SingleCalculatedValue's timestamp/result, mutable global strings, the global PMO
flag/counters/report/reset, world data, or static ChatHelper tables. These remain
explicit boundaries of the focused ownership fix.
