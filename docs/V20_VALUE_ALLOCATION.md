# V20: shared-value lifetime and local allocation reduction

Source baseline: V19 `33faa8e3c091d73483b42dc07f8f2934ba5f7958`.
Branch: `vmangos-v20-value-allocation`. The release's exact commit, CI run,
EXE/PDB hashes and runtime DLL hashes belong to its `build-info.json` and delivery
manifest. This document does not promote or install V20 as a gameplay baseline.
The accepted V19, V18 `fbab350b6cde7a40735d778986382807d6287048`, V17.1
`2ea64f9d27563ac410bacea79de6e2e2671b01c3` and V16
`cea7ba955f5c0d57ff76d63e53f97f34b1d3ab4c` references remain preserved.

## Included changes

**A — correct shared-value ownership.** SharedObjectContext formerly created a
temporary default PlayerbotAI for every lookup and deleted it before returning
the value. AiObject retains that AI and its ChatHelper address. A real-constructor
MSVC ASan reproducer confirmed a heap-use-after-free in PMO start from a cold
SingleCalculatedValue Get with PMO ON. It reads the already-freed 1344-byte AI.
This is a demonstrated code defect, not an attribution of past gameplay spikes.

The context now owns one botless default AI and its shared registry. Cached values
are destroyed before AI/chat; the borrowed context list remains unchanged. A
recursive mutex serializes lookup/factory mutations, including negative entries.
The AI is not attached to a Player or used to run game updates. All 16 registered
values, concrete Calculate bodies, nested GAI dependencies and destructors were
audited: their calculations do not mutate this AI or depend on a changing bot.
See [ownership audit](V20_SHARED_OWNERSHIP_AUDIT.md) and
[actual-constructor tests](../tests/playerbots/v20_shared/README.md).

Factory priority, name/qualifier bytes, null negative caching, exception policy
and per-value results/refresh policies remain unchanged. Warm cache hits cease
constructing/destructing AI, its filters and container storage. The small registry
lock is not a claim of general shared-value thread safety: Value Get/Set/Reset,
cold Calculate, existing SingleThreaded singleton first creation/shutdown and PMO
counter/report concurrency remain separately bounded.

**B — refresh six perception values without unused list copies.** CheckValuesAction
still reads possible targets, all targets, nearest NPCs, corpses, game objects
without LOS and friendly players in the same order. It discards GetSize results
instead of copying six lists. All concrete registered policies use the existing
final ObjectGuidListCalculatedValue scalar refresh implementation. Unknown
policies retain the original virtual Get fallback. The GO interval remains 1,
including its effective refresh on every read; its original PMO name is preserved.
See [B tests and allocation accounting](../tests/playerbots/v20_checkvalues/README.md).

**C — move existing local combat-target list nodes.** RemoveNonThreating uses
splice for CC lists and their fallback, and keeps the selected ordinary node for
getOne. Order, duplicate GUIDs, every validity/CC/RTI short-circuit, and the priority
of unbreakable CC remain exact. The old CC-only fallback can still return multiple
targets even with getOne. All lists have equal standard allocators; no iterator
or node reference escapes. See [C tests](../tests/playerbots/v20_targets/README.md).

**Separate PMO correctness fix.** PrintStats aggregation now derives maxTime
from each partition's maxTime instead of minTime. No stored sample/count/key,
total time, measurement scope, sorting or formatting is changed. Synthetic
partitions `(min=2,max=90)` and `(min=5,max=120)` now report 120 instead of 5.
The existing zero-denominator NaN/Inf presentation is documented, not silently
fixed in this change. See [repository-wide PMO audit](V20_PMO_SOURCE_AUDIT.md).

The numeric qualified-key formatter is **not included**: locale and exact-key
equivalence need their own audit. No extra optimization was added merely to
increase the candidate count. Broader shared concurrency, raw-map ownership and
gameplay/item-save defects remain separate TODOs.

## Verification and preserved behavior

- A: 16,188 candidate checks in normal and ASan builds; legal V19 OFF contracts
  and a separately validated expected V19 ASan UAF. Real production layout,
  AI/member constructors, factory/cache and Value policies are used. Names/counts
  are inspected in actual PMO storage: ON contract matrix yields 48 item-drop-list
  and 64 probe samples in map/instance 0/0. OFF yields none. Reset/Set/LazyGet,
  expiry observations, reentry, exceptions, owner unwind and destruction are checked.
  Eight threads test an already-created owner's insertion/negative-cache/warm-read
  paths; this is not a general race-freedom proof.
- B: 2,560 paired scenarios each in normal and ASan, covering cold/warm/expired/
  Reset/Set, empty through 512 GUIDs, PMO flags, actual call order, world-debug flags,
  unknown policies, reentry and exceptions. Calculate allocations remain identical.
- C: 89,360 paired observations each in normal and ASan: exhaustive small lists,
  12,000 seeded larger random lists, duplicates, predicate traces, exception/reentry
  states and the actual surrounding Calculate gates/qualifiers/fallback.
- PMO report: 128 actual PrintStats calls, 88 compared rows, 80 corrected maximum
  fields. Other columns, source records and all flag combinations are checked.
- Previous cumulative context/action/Value/grind tests, full-chain Value/RTTI
  contracts, 34,000 quest observations and 5,269 PMO observations remain required,
  including their existing ASan suites. The old quest generator's whole-file
  SharedValueContext guard is narrowed to unchanged registrations and typed-key
  overloads; its previous quest fixture/cases remain intact. Actual V20 ownership
  is tested separately with the real constructor, not its older dummy fixture.

The broad PMO source audit scans 1,981 tracked production files plus build
configuration, not just literal starts in one directory. It finds 47 external
guarded starts, one guarded internal operation allocation and no additional
active instrumentation alias/path. ON-to-OFF cleanup, FullTick reset, manual
report/reset, Value freshness clocks and WorldRunnable/Perf.log are intentionally
retained. Unchanged cumulative behavior includes V17 A/B/C and V18 quest logic.

Only four production files change relative to V19. No changes to AI frequencies,
radii/population, DisableBotOptimizations, target scoring, quest decisions,
pathfinding/routes, `newdistance =+`, Mema, AddCooldown, UseItem, item save/queue
handling, logging, config, database or dependency versions are included.

## Same-work local measurements

MSVC 19.44 x64 RelWithDebInfo `/O2`, no LTO or identical-code folding. Variant
order is alternated, iterations match, warmup is discarded and raw samples are
retained. Timing executables use the real CRT allocator; allocation accounting
uses separate executables. These are controlled function/constructor fixtures,
not paired live-world server diff measurements.

| Work, PMO OFF | V19 median ns | V20 median ns | Allocations per call V19 -> V20 |
|---|---:|---:|---:|
| A, warm untyped shared lookup | 4741.47 | 16.17 | 67 -> 0 |
| A, warm typed Get including list return | 5401.15 | 483.32 | 73 -> 6 |
| A, cold value inside existing owner | 6346.45 | 1081.73 | 85 -> 18 |
| A, Reset plus typed Get | 5538.89 | 609.37 | 77 -> 10 |
| B, six lists of 64 GUIDs, warm | 11448.85 | 1927.94 | 458 -> 68 |
| B, six lists of 64 GUIDs, cold | 22395.65 | 12575.40 | 783 -> 393 |
| C, eight CC targets, warm local input | 483.53 | 104.42 | 18 -> 2 |
| C, 64 CC targets, warm local input | 3628.46 | 555.78 | 130 -> 2 |

A warm lookups remove one actual AI construction per call. Its reduced fixture
registry/world Calculate limits are explicit in the test README. Cold means a
new value in an existing owner, excluding owner/static-table/world startup.
Warm typed Get with PMO ON: 5380.23 -> 477.61 ns. V19 cold/Reset ON invokes the
confirmed UAF and has **no valid comparison timing**; it is not repaired silently
inside the baseline fixture.

B eliminates exactly `6 * (list length + 1)` MSVC allocations, including list
sentinels; the warm GO calculation still allocates its original storage. C's two
remaining allocations are MSVC local-list sentinels. Neither local test is a
measurement of total quest/perception/world-scan or full combat-target cost.

Negative and rejected results are retained: C's ordinary eight-target/all-results
case regressed by 3.16 ns (2.76%) in the final sweep. A and B had no negative final
medians. Earlier B timing runs with allocator hooks are explicitly excluded from
the release benchmark because the hooks biased allocation cost; raw runs remain
as diagnostic evidence. COFF inspection confirms A's production-method fixture
has no AI constructor/new call on lookup and C reduces direct new sites 8 -> 2.
This source/object evidence is not described as final-server EXE tracing.

## V19 gameplay handoff recheck and delivery

All five original log hashes, sizes and line counts match the handoff. Seventeen
groups of claims reproduce, including the last ten clean samples (2363.8 active,
99.4/99.0 ms), maximum recorded clean active 2445, one 26-second PMO window,
normal shutdown, loot/warning counts and printed metric aggregates. History
outside 2026-09-25 06:05:02-06:58:29 is excluded. Thresholded Perf.log is not a
complete tick distribution; the previous V18 session is not a paired experiment.
Raw personal/server logs are not published with the source evidence.

Windows-only CI builds VS2022 x64 RelWithDebInfo mangosd, BUILD_PLAYERBOTS=1,
SUPPORTED_CLIENT_BUILD=5875, with existing pinned dependencies. It reruns old/new
contracts and packages EXE, PDB, build-info, runtime DLLs/licenses and evidence.
Final delivery verifies CI at exact HEAD and EXE/PDB identity and hashes. No
automatic deployment, world startup or database connection is part of this task.
V20's next gameplay comparison is against accepted V19. The 40-man raid test stays
nonblocking and must use an actual user-assembled raid.
