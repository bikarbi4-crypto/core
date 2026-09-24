# V19 PMO OFF Fast Path Batch

Source/development baseline: V18 `fbab350b6cde7a40735d778986382807d6287048`.
V17.1 `2ea64f9d27563ac410bacea79de6e2e2671b01c3` and V16
`cea7ba955f5c0d57ff76d63e53f97f34b1d3ab4c` remain preserved references.
V19 is a separate `vmangos-v19-pmo-off-fast-path` branch. No default branch move
or live server installation is part of this release. Exact HEAD, CI and binary
identity are supplied in the final artifact manifest after the build completes.

## Included changes

- A: guard the three PlayerbotAI total scopes, FullTick, ExecuteAction,
  minimal movement and auction/vendor scopes. Construct DoNextAction's action
  name/event suffix only inside the existing enabled branch. Other uses of the
  event and action name in gameplay/logging remain at their original locations.
- B: guard all factory and RandomPlayerbotMgr start sites, including reassigned
  pointers whose existing resets remain in place. No cold-path redesign.
- C: finish-time OFF still discards the sample, but now always unwinds the
  operation's named AI stack entry. Remove the innermost matching entry only,
  retaining a live parent with the same name. All named production scopes use
  nested RAII lifetimes; distinct-name out-of-order teardown also stays valid.
  This is not a new token-ownership API for arbitrary out-of-order identical
  names or external edits of the stack. Those are not production call patterns.
- D: lazily register map/instance storage at the first enabled start. A shared
  registry lock covers existing-map lookup; exclusive insertion handles a miss.
  Registry nodes are not erased by Reset, so the selected pointer remains valid.
  Per-map string keys and timing remain outside this lock. PrintStats/Reset/Init
  synchronize registry traversal. The old periodic pre-warm loop is retained
  for ON and skipped entirely for OFF.

ProcessTriggers, CalculatedValue and SingleCalculatedValue were already lazy;
their production source stays exact V18. There are 47 active start callsites,
41 changed and 6 already guarded. One commented PlayerbotFactory_EqSets call
is excluded. All final start arguments are within an enabled branch.

## Reproduced defects and toggle policy

The V18 source-only reproducer reports stale stack size 1 after ON -> OFF
destruction; a nested same-name child's destruction incorrectly leaves zero
parent entries; and OFF Init -> runtime toggle ON -> start returns no operation
for an unregistered map. These are recorded failures of the baseline, not test
failures hidden by V19. The same cases pass with the candidate.

Registration was not a map-constructor hook: its only external call was the
loop at the end of RandomPlayerbotMgr::UpdateAIInternal. It could eventually
pre-warm existing maps while randomBotAutologin and enabled were true, but the
early return and update interval made it an incomplete dynamic-toggle guarantee.
Lazy registration covers those modes and new instances immediately, including
the default 0/0 bucket used by FullTick and random-bot operations.

Sampling retains the existing finish-time flag policy. A scope ending OFF has
no finishing clock read or statistics update, but its prior ON stack frame is
removed. A scope spanning ON -> OFF -> ON and ending ON records its full elapsed
wall time, including the OFF interval. No epoch splitting or reset-on-toggle is
introduced. Normal registered, uniquely nested ON keys/counts/timing boundaries
are identical. Duplicate-parent keys and previously missing map samples are
intentional correctness changes. FullTick remains a member pointer that spans
until the next UpdateAI reset/destruction, including early returns.

The registry lock is not a general PMO concurrency redesign. Existing global
configuration-bool races and per-metric counter/report/reset races are not
claimed fixed. Tests change the flag sequentially; parallel registration tests
keep it stable. Named AI stacks retain their existing per-AI ownership model.

## Functional evidence

The generator extracts actual immutable V18/current monitor implementations,
all 47 PMO preparation fragments, and complete Engine::ExecuteAction and
PlayerbotAIBase::UpdateAI bodies. It checks identical start expressions and
identical remaining caller source/reset order after removing only PMO fragments
and the explicit map pre-warm guard. Value/context/quest/target/travel source
invariants are checked separately. No hand-written replacement monitor is used.

5,269 parity/toggle observations pass normally and under MSVC AddressSanitizer.
Coverage includes OFF throughout; ON throughout; OFF -> ON; ON -> OFF; nested
live operations across toggles; repeated names; exception unwind; Reset with
an active operation; null AI context; map IDs 0/1/33/609 and distinct instance IDs;
every metric type; both manager names; long/short names; absent/empty events;
unknown actions and all ExecuteAction useful/possible/result/owner branches;
FullTick early return and cross-call lifetime. Another test creates 512 distinct
map/instance registrations on four threads with PMO continuously ON.

Every OFF caller test records zero start entries, operation constructions, clock
reads, registry lookups, PMO position constructions, action-name reads and event
source reads. Registry/statistics/stack remain empty. Gameplay work checksums and
ordered boundary traces match. Existing cumulative tests pass 4/4 normally and
4/4 with ASan; Value-chain checks pass 810 observations; V18 quest checks pass
34,000 observations normally and under ASan.

Controlled substitutions are explicit: game/world data and work, action service
implementations, coordinates/memory-monitor hooks, and a deterministic clock
only in trace builds. Benchmarks use the real chrono clock and production
monitor. The WorldPosition instance predicates and action/event string-copy
behavior are retained, but full game-object layout/virtual dispatch and real
game work are not modeled. These tests do not replace in-game validation.

MSVC optimized assembly for 11 selected functions/fragments shows the OFF
branches skip all start calls and PMO string/position preparation. FullTick's
prior-scope reset is retained before its flag check. ExecuteAction's actual
gameplay allocation remains. Assembly is from extracted instrumentation and
complete ExecuteAction/FullTick with fixture services, not the linked mangosd
function bodies. Listings and a machine-checked guard inventory are included.

## Same-work microbenchmarks

MSVC 19.44.35223, VS2022 x64 RelWithDebInfo, /O2 /GR /GL- /LTCG:OFF, ICF off;
monitor and consumers in separate translation units. One pinned logical CPU.
Both variants run identical iteration counts/work per case; order alternates;
11 rounds, first discarded; shared-lock sweep calibrates for roughly >=12ms for
the faster variant. Setup is once per large batch. Raw rounds, ranges, median
absolute deviations and all negative results are included. Unchanged OFF
controls drift by sub-nanosecond amounts; percentage changes at that scale are
not meaningful speedups. No server-wide percentage or diff reduction is claimed.

Final shared-registry-lock sweep, map 1 / instance 123, ns per call:

| Operation | V18 OFF | V19 OFF | V18 ON | V19 ON |
|---|---:|---:|---:|---:|
| UpdateAI preparation | 40.82 | 3.64 | 259.02 | 268.45 |
| UpdateAIReaction preparation | 41.85 | 3.69 | 263.62 | 267.04 |
| UpdateAIInternal preparation | 43.41 | 3.90 | 265.86 | 271.12 |
| FullTick full wrapper | 33.74 | 4.03 | 239.05 | 242.79 |
| DoNextAction name preparation | 155.56 | 3.42 | 461.11 | 466.08 |
| ExecuteAction full function | 78.95 | 32.52 | 882.86 | 914.98 |
| minimalMove scope | 6.95 | 2.31 | 129.90 | 132.56 |
| Factory Reset scope (cold) | 30.44 | 2.22 | 242.68 | 249.20 |
| AsyncBotLogin scope (manager interval) | 7.03 | 2.11 | 119.27 | 126.85 |
| SingleCalculatedValue scope (unchanged control) | 2.51 | 2.30 | 299.66 | 315.13 |

ON is not zero-cost: the registry safety fix adds a small measurable overhead.
For the main map-1 hot cases above it is about 3-32 ns/call (roughly 1-4% of this
instrumentation-only fixture); some shorter/control scopes show a larger
relative change. All map-33 and individual-scope samples are also retained.
This is a disclosed correctness/performance tradeoff, not a claim of exact
ON performance parity. In-game ON regression/lock contention is still a runtime
validation item. Existing ON keys, counts and normal nesting pass parity.

Rejected intermediate design/results are retained: the first sweep used an
exclusive lock across the whole start and short fixed batches; the second used
a short exclusive registry lock with calibrated batches. Both showed larger ON
overhead. The final shared-read lock permits existing-map lookups concurrently
and keeps allocations/timing outside the lock. Earlier benchmark source hashes
are retained. The last extra OFF map-warmup guard does not occur in measured
fragments; their hashes and final monitor source match the shared-lock sweep.

## Complete callsite inventory

Categories describe V18: A fully lazy; B guarded start with eager metadata;
C unconditional start; D RandomPlayerbotMgr interval/cold work. Factory and
auction sites are C but their conditional/bursty frequency is stated separately.
Frequency is estimated from control flow, not measured server call rates.
Every row is A in the final source. Paths are relative to
`src/game/PlayerBots/playerbot/`; line numbers are V18 -> V19.

| ID | File:line | Function / key | V18 | Frequency | Change |
|---:|---|---|:---:|---|---|
| 0 | `PlayerbotAI.cpp:279 -> 282` | `PlayerbotAI::UpdateAI` / `PERF_MON_TOTAL, "PlayerbotAI::UpdateAI " + mapString, nullptr, bot->GetMapId(), bot->GetInstanceId());` | C | each reached bot update / reaction / internal update | guard start and all PMO-only preparation |
| 1 | `PlayerbotAI.cpp:604 -> 611` | `PlayerbotAI::UpdateAIReaction` / `PERF_MON_TOTAL, "PlayerbotAI::UpdateAIReaction " + mapString, nullptr, bot->GetMapId(), bot->GetInstanceId());` | C | each reached bot update / reaction / internal update | guard start and all PMO-only preparation |
| 2 | `PlayerbotAI.cpp:1129 -> 1140` | `PlayerbotAI::UpdateAIInternal` / `PERF_MON_TOTAL, "PlayerbotAI::UpdateAIInternal " + mapString, nullptr, bot->GetMapId(), bot->GetInstanceId());` | C | each reached bot update / reaction / internal update | guard start and all PMO-only preparation |
| 3 | `PlayerbotAIBase.cpp:18 -> 19` | `PlayerbotAIBase::UpdateAI` / `PERF_MON_TOTAL, "PlayerbotAIBase::FullTick");` | C | each base UpdateAI (FullTick spans until next reset) | guard start and all PMO-only preparation |
| 4 | `PlayerbotFactory.cpp:186 -> 188` | `PlayerbotFactory::Randomize` / `PERF_MON_RNDBOT, "PlayerbotFactory_Reset");` | C | per randomization/initialization; cold but nested/bursty | guard start and all PMO-only preparation |
| 5 | `PlayerbotFactory.cpp:236 -> 239` | `PlayerbotFactory::Randomize` / `PERF_MON_RNDBOT, "PlayerbotFactory_Talents");` | C | per randomization/initialization; cold but nested/bursty | guard start and all PMO-only preparation |
| 6 | `PlayerbotFactory.cpp:248 -> 252` | `PlayerbotFactory::Randomize` / `PERF_MON_RNDBOT, "PlayerbotFactory_Spells2");` | C | per randomization/initialization; cold but nested/bursty | guard start and all PMO-only preparation |
| 7 | `PlayerbotFactory.cpp:270 -> 275` | `PlayerbotFactory::Randomize` / `PERF_MON_RNDBOT, "PlayerbotFactory_Equip");` | C | per randomization/initialization; cold but nested/bursty | guard start and all PMO-only preparation |
| 8 | `PlayerbotFactory.cpp:316 -> 323` | `PlayerbotFactory::Randomize` / `PERF_MON_RNDBOT, "PlayerbotFactory_Guilds & ArenaTeams");` | C | per randomization/initialization; cold but nested/bursty | guard start and all PMO-only preparation |
| 9 | `PlayerbotFactory.cpp:327 -> 336` | `PlayerbotFactory::Randomize` / `PERF_MON_RNDBOT, "PlayerbotFactory_Pet");` | C | per randomization/initialization; cold but nested/bursty | guard start and all PMO-only preparation |
| 10 | `PlayerbotFactory.cpp:334 -> 345` | `PlayerbotFactory::Randomize` / `PERF_MON_RNDBOT, "PlayerbotFactory_Pet");` | C | per randomization/initialization; cold but nested/bursty | guard start and all PMO-only preparation |
| 11 | `PlayerbotFactory.cpp:359 -> 371` | `PlayerbotFactory::Randomize` / `PERF_MON_RNDBOT, "PlayerbotFactory_Save");` | C | per randomization/initialization; cold but nested/bursty | guard start and all PMO-only preparation |
| 12 | `PlayerbotFactory.cpp:384 -> 398` | `PlayerbotFactory::AddConsumables` / `PERF_MON_RNDBOT, "PlayerbotFactory_Consumables");` | C | per randomization/initialization; cold but nested/bursty | guard start and all PMO-only preparation |
| 13 | `PlayerbotFactory.cpp:1617 -> 1633` | `PlayerbotFactory::InitReputations` / `PERF_MON_RNDBOT, "PlayerbotFactory_Reputations");` | C | per randomization/initialization; cold but nested/bursty | guard start and all PMO-only preparation |
| 14 | `PlayerbotFactory.cpp:2942 -> 2960` | `PlayerbotFactory::InitBags` / `PERF_MON_RNDBOT, "PlayerbotFactory_Bags");` | C | per randomization/initialization; cold but nested/bursty | guard start and all PMO-only preparation |
| 15 | `PlayerbotFactory.cpp:3090 -> 3110` | `PlayerbotFactory::InitAllSkills` / `PERF_MON_RNDBOT, "PlayerbotFactory_Skills1");` | C | per randomization/initialization; cold but nested/bursty | guard start and all PMO-only preparation |
| 16 | `PlayerbotFactory.cpp:3288 -> 3310` | `PlayerbotFactory::UpdateTradeSkills` / `PERF_MON_RNDBOT, "PlayerbotFactory_Skills2");` | C | per randomization/initialization; cold but nested/bursty | guard start and all PMO-only preparation |
| 17 | `PlayerbotFactory.cpp:3581 -> 3605` | `PlayerbotFactory::InitAvailableSpells` / `PERF_MON_RNDBOT, "PlayerbotFactory_Spells1");` | C | per randomization/initialization; cold but nested/bursty | guard start and all PMO-only preparation |
| 18 | `PlayerbotFactory.cpp:3836 -> 3862` | `PlayerbotFactory::InitAmmo` / `PERF_MON_RNDBOT, "PlayerbotFactory_Ammo");` | C | per randomization/initialization; cold but nested/bursty | guard start and all PMO-only preparation |
| 19 | `PlayerbotFactory.cpp:3895 -> 3923` | `PlayerbotFactory::InitMounts` / `PERF_MON_RNDBOT, "PlayerbotFactory_Mounts");` | C | per randomization/initialization; cold but nested/bursty | guard start and all PMO-only preparation |
| 20 | `PlayerbotFactory.cpp:4034 -> 4064` | `PlayerbotFactory::InitPotions` / `PERF_MON_RNDBOT, "PlayerbotFactory_Potions");` | C | per randomization/initialization; cold but nested/bursty | guard start and all PMO-only preparation |
| 21 | `PlayerbotFactory.cpp:4064 -> 4096` | `PlayerbotFactory::InitFood` / `PERF_MON_RNDBOT, "PlayerbotFactory_Food");` | C | per randomization/initialization; cold but nested/bursty | guard start and all PMO-only preparation |
| 22 | `PlayerbotFactory.cpp:4093 -> 4127` | `PlayerbotFactory::InitReagents` / `PERF_MON_RNDBOT, "PlayerbotFactory_Reagents");` | C | per randomization/initialization; cold but nested/bursty | guard start and all PMO-only preparation |
| 23 | `PlayerbotFactory.cpp:4254 -> 4290` | `PlayerbotFactory::InitInventory` / `PERF_MON_RNDBOT, "PlayerbotFactory_Inventory");` | C | per randomization/initialization; cold but nested/bursty | guard start and all PMO-only preparation |
| 24 | `PlayerbotFactory.cpp:4936 -> 4974` | `PlayerbotFactory::InitTaxiNodes` / `PERF_MON_RNDBOT, "PlayerbotFactory_TaxiNodes");` | C | per randomization/initialization; cold but nested/bursty | guard start and all PMO-only preparation |
| 25 | `RandomPlayerbotMgr.cpp:578 -> 580` | `RandomPlayerbotMgr::UpdateAIInternal` / `PERF_MON_RNDBOT, "AsyncBotLogin");` | D | manager interval for first two scopes; per teleport/randomize/refresh otherwise | guard start and all PMO-only preparation |
| 26 | `RandomPlayerbotMgr.cpp:597 -> 601` | `RandomPlayerbotMgr::UpdateAIInternal` / `PERF_MON_RNDBOT,         onlineBotCount < maxAllowedBotCount ? "RandomPlayerbotMgr::Login" : "RandomPlayerbotMgr::UpdateAIInternal");` | D | manager interval for first two scopes; per teleport/randomize/refresh otherwise | guard start and all PMO-only preparation |
| 27 | `RandomPlayerbotMgr.cpp:2562 -> 2571` | `RandomPlayerbotMgr::RandomTeleport` / `PERF_MON_RNDBOT, "RandomTeleportByLocations");` | D | manager interval for first two scopes; per teleport/randomize/refresh otherwise | guard start and all PMO-only preparation |
| 28 | `RandomPlayerbotMgr.cpp:2962 -> 2973` | `RandomPlayerbotMgr::RandomTeleport` / `PERF_MON_RNDBOT, "RandomTeleport");` | D | manager interval for first two scopes; per teleport/randomize/refresh otherwise | guard start and all PMO-only preparation |
| 29 | `RandomPlayerbotMgr.cpp:3047 -> 3060` | `RandomPlayerbotMgr::UpdateGearSpells` / `PERF_MON_RNDBOT, "UpgradeGear");` | D | manager interval for first two scopes; per teleport/randomize/refresh otherwise | guard start and all PMO-only preparation |
| 30 | `RandomPlayerbotMgr.cpp:3076 -> 3091` | `RandomPlayerbotMgr::RandomizeFirst` / `PERF_MON_RNDBOT, "RandomizeFirst");` | D | manager interval for first two scopes; per teleport/randomize/refresh otherwise | guard start and all PMO-only preparation |
| 31 | `RandomPlayerbotMgr.cpp:3166 -> 3183` | `RandomPlayerbotMgr::Refresh` / `PERF_MON_RNDBOT, "Refresh");` | D | manager interval for first two scopes; per teleport/randomize/refresh otherwise | guard start and all PMO-only preparation |
| 32 | `strategy/Engine.cpp:162 -> 162` | `Engine::DoNextAction` / `PERF_MON_ACTION, std::move(actionName), ai);` | B | each action candidate / explicit action / checked trigger | guard start and all PMO-only preparation |
| 33 | `strategy/Engine.cpp:200 -> 201` | `Engine::DoNextAction` / `PERF_MON_ACTION, "isUseful", ai);` | A | each action candidate / explicit action / checked trigger | none: arguments already lazy |
| 34 | `strategy/Engine.cpp:246 -> 247` | `Engine::DoNextAction` / `PERF_MON_ACTION, "isPossible", ai);` | A | each action candidate / explicit action / checked trigger | none: arguments already lazy |
| 35 | `strategy/Engine.cpp:254 -> 255` | `Engine::DoNextAction` / `PERF_MON_ACTION, "Execute", ai);` | A | each action candidate / explicit action / checked trigger | none: arguments already lazy |
| 36 | `strategy/Engine.cpp:454 -> 457` | `Engine::ExecuteAction` / `PERF_MON_ACTION, name, ai);` | C | each action candidate / explicit action / checked trigger | guard start and all PMO-only preparation |
| 37 | `strategy/Engine.cpp:458 -> 463` | `Engine::ExecuteAction` / `PERF_MON_ACTION, "isUseful", ai);` | C | each action candidate / explicit action / checked trigger | guard start and all PMO-only preparation |
| 38 | `strategy/Engine.cpp:464 -> 471` | `Engine::ExecuteAction` / `PERF_MON_ACTION, "isPossible", ai);` | C | each action candidate / explicit action / checked trigger | guard start and all PMO-only preparation |
| 39 | `strategy/Engine.cpp:471 -> 480` | `Engine::ExecuteAction` / `PERF_MON_ACTION, "Execute", ai);` | C | each action candidate / explicit action / checked trigger | guard start and all PMO-only preparation |
| 40 | `strategy/Engine.cpp:632 -> 641` | `Engine::ProcessTriggers` / `PERF_MON_TRIGGER, trigger->getName(), ai);` | A | each action candidate / explicit action / checked trigger | none: arguments already lazy |
| 41 | `strategy/Value.h:99 -> 99` | `CalculatedValue::RefreshValue` / `PERF_MON_VALUE, AiNamedObject::getName(), this->ai);` | A | each actual Value calculation | none: arguments already lazy |
| 42 | `strategy/Value.h:140 -> 140` | `SingleCalculatedValue::Get` / `PERF_MON_VALUE, AiNamedObject::getName(), this->ai);` | A | each actual Value calculation | none: arguments already lazy |
| 43 | `strategy/actions/AhAction.cpp:68 -> 70` | `AhAction::ExecuteCommand` / `PERF_MON_VALUE, "IsMoreProfitableToSellToAHThanToVendor", ai);` | C | per evaluated auction/vendor item; conditional bursts | guard start and all PMO-only preparation |
| 44 | `strategy/actions/AhAction.cpp:242 -> 246` | `AhBidAction::ExecuteCommand` / `PERF_MON_VALUE, "IsWorthBuyingFromAhToResellAtAH", ai);` | C | per evaluated auction/vendor item; conditional bursts | guard start and all PMO-only preparation |
| 45 | `strategy/actions/BuyAction.cpp:78 -> 80` | `BuyAction::Execute` / `PERF_MON_VALUE, "IsWorthBuyingFromVendorToResellAtAH", ai);` | C | per evaluated auction/vendor item; conditional bursts | guard start and all PMO-only preparation |
| 46 | `strategy/actions/MovementActions.cpp:491 -> 493` | `MovementAction::MinimalMove` / `PERF_MON_ACTION, "minimalMove", ai);` | C | each reached minimal movement attempt | guard start and all PMO-only preparation |

## Build, scope and remaining validation

Windows-only CI: VS2022 x64 RelWithDebInfo, BUILD_PLAYERBOTS=1,
SUPPORTED_CLIENT_BUILD=5875, target mangosd. Publish matching EXE/PDB,
build-info.json, application DLLs/licenses, normal/ASan test logs, provenance,
callsite inventory, benchmark data and assembly review. Packaged --version smoke
uses isolated DLL search paths and starts no world/database.

Only ten production files change, all in PlayerBots. Cumulative V0-V18 is
preserved by ancestry plus the production diff and source invariants. V17 A/B/C,
Value/list policies, V18 quest work, newdistance =+, Mema, AddCooldown, UseItem,
Item::AddToUpdateQueueOf / character_inventory, routes/pathfinding, AI cadence,
population/radii and DisableBotOptimizations are unchanged. The existing
non-blocking 40-man raid validation remains in OPTIMIZATION_TODO.md.

Local jobs use four of sixteen logical CPUs and two compiler jobs (/MP1), with
benchmarks pinned to one. Whole-PC CPU samples are included. No active D: server
files, database or configuration are installed/modified. Runtime gameplay and
server-wide performance still require the user's controlled real-server run.
