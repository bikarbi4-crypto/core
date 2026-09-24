# V18 Quest Hot Path Batch

Development baseline: V17.1 `2ea64f9d27563ac410bacea79de6e2e2671b01c3`.
V16 `cea7ba955f5c0d57ff76d63e53f97f34b1d3ab4c` remains the saved fallback.
V18 descends from V17.1 and preserves cumulative V0-V17.1, including V17 A/B/C.
The separate 40-man raid validation in `OPTIMIZATION_TODO.md` is not a gate.

## Included candidates

**A: one destination scan per quest per Calculate.** `GetDestinations` receives
the same `PlayerTravelInfo`, `QuestAllObjective`, questId, false and zero for
every needed objective. There is no objective argument. `onlyPossible=false`
skips IsPossible (and therefore objective-specific/gameplay predicates);
`maxDistance=0` skips the distance limit. The existing unconditional DistanceTo
filter remains exactly as written. DistanceTo -> WorldWpSquare/MapWpSquare ->
sqMapTransDistances -> MapTransfer::sqDist reads the captured position, square
bounds/points and preloaded transfer data. It performs no RNG, bot Value reads,
callbacks or writes. GetEntry reads a stored entry. Destination/transfer data
are not changed by any need-objective calculation on this path.

The implementation queries at the first needed objective. On a miss it still
reads every later objective Value in the original order, so refreshes, shared
item checks and PMO remain in order. Only redundant pure destination queries
are removed. On a hit it returns at the original point. A local boolean dies
at function exit; nothing is retained between Calculate calls or ticks.
For one quest and 0/1/2/4 needed objectives on a miss, query counts are
0/1/2/4 -> 0/1/1/1. With 16 destinations, distance reads are 0/16/32/64 ->
0/16/16/16. TravelMgr, distance/pathfinding code and filtering are unchanged.

**B: identical qualifier formatting without vector/stringstream.** The known
two unsigned decimal fields produce exactly `{questId,objective}`, including
the original braces. The per-quest decimal prefix is formed once locally.
The ordinary named lookup, qualified cache key, RTTI and virtual Get remain.
This removes a vector allocation and general MultiQualify stream formatting.

The broader parsing proposal is **not included**. `NeedQuestObjectiveValue`
also accepts whole-quest/reward/braced forms; its sequence parses questId,
checks active status, tests reward/status/comma, and only then parses objective.
Changing it to an eager numeric pair risks changing short circuits and exception
order. Its `getMultiQualifierInt`, find calls and entire Calculate body remain
byte-for-byte V17.1. The measured formatting improvement is sufficient without
altering this interface. No unmeasured parsing speedup is claimed.

**C: specialized warm IsEmpty in two concrete shared values.** SharedValueContext
registers ItemDropListValue and ItemVendorListValue, both
`SingleCalculatedValue<std::list<int32>>`, not ObjectGuid calculated lists.
Their Calculate functions copy matching entries from shared drop/vendor maps
into an owned list. No production subclass overrides their Get. Once populated,
SingleCalculatedValue does not refresh on elapsed time; Reset clears its stamp,
Set changes its owned value without changing that stamp, and LazyGet keeps its
original policy. The new overrides read only `value.empty()` when the stamp is
nonzero. At zero (including Reset, or the clock-zero edge case) they call the
original virtual Get, retaining first-calculation copies, PMO, exceptions and
reentry behavior. They expose a boolean, never a storage reference.

Only the two `.empty()` consumers in CanGetItemSomewhere switch to IsEmpty.
List iteration/copy consumers, GetSize, generic list policies and Value.h are
unchanged. The existing empty-vendor-list -> true and strict money > price *
required-count rules are deliberately preserved. This does not attempt to
optimize first population or solve pre-existing shared initialization races.

## Functional and memory validation

`tests/playerbots/quest_audit` extracts the immutable V17.1 and current production
functions, full relevant Value inheritance, qualifiers, named AVL cache/factory,
AiObjectContext and SharedObjectContext retrieval/RTTI. A-only, B-only and C-only
variants isolate the source edits; the V18 function is exact working-tree source.
Generation asserts that subtracting A/B reconstructs the exact V17.1 function,
and that Value.h and travel/distance sources are unchanged. Source hashes are
recorded in generated/provenance.json.

34,000 observations pass for V17.1, A-only, B-only, C-only and V18, normally and
under MSVC AddressSanitizer. The matrix covers 0/1/2/4 objectives, all 16
incomplete masks, creature/GO/item needs, first/last/absent entry, empty and
unreachable destinations, completed/inactive/missing-template/not-in-log
quests, 1/3/20 quests, first reads, subsequent reads, elapsed clock, Reset,
changed progress, PMO on/off, list sizes 0/1/8/64, missing item prototype, and
money below/equal/above price. Result oracles are independent of the source
implementations. Significant traces match exactly after dropping only repeated
pure destination events; A/V18 assert at most one query per quest per call.
There is no RNG in the audited reachable production path.

Separate list-policy tests cover Set, Reset, LazyGet, clock zero, returned-copy
independence, Calculate reentry, Reset during Calculate, and timestamp/state
after exceptions. Formatter equality includes unsigned boundary quest IDs.
Malformed-input parser behavior is not broadened or repaired in V18.

Controlled substitutions: live-world construction, quest/item/position data,
destination DistanceTo return values and shared drop/vendor map population.
These are source contract tests, not a live world or a database integration
test. Actual square/portal geometry is reviewed unchanged, not simulated by
the fixtures. The fixture owns its shared-context lifetime for leak-free teardown;
the production context remains process-global. Baseline and candidate have
identical substitutions. PMO is real policy with fixture callbacks; parity uses
a deterministic clock and benchmark uses the real CRT clock.

## Same-work microbenchmarks and negative results

VS2022 / MSVC 19.44.35223, x64 RelWithDebInfo, C++17, `/GR /GL- /LTCG:OFF`,
separate factory and consumer translation units, ICF off. Calls use real named
lookup, RTTI, qualifiers and Value policy; no hand-written replacement lookup.
One pinned logical CPU, identical iteration count for all five variants,
rotating/reversed order, 9 rounds (round zero is warm-up), roughly 12ms or more
per timed batch. Constructors/initial population are outside warm batches.
The artificial world services and real-world PlayerTravelInfo construction cost
are not representative of a whole server. Timings must not be read as server
diff reductions. Raw rounds, all variant medians/ranges and negative samples
are retained under docs/validation/v18-* and the delivered evidence package.

First sweep, microseconds per operation (median of 8 retained rounds):

| Operation | V17.1 | A only | B only | C only | V18 |
|---|---:|---:|---:|---:|---:|
| 0 needed objectives | 3.873 | 3.846 | 1.605 | 3.845 | 1.632 |
| 1 objective, first match | 1.096 | 1.100 | 0.752 | 1.095 | 0.776 |
| 2 objectives, absent | 4.715 | 4.327 | 2.242 | 4.729 | 1.947 |
| 4 objectives, absent | 5.572 | 4.329 | 2.896 | 5.576 | 1.954 |
| 20 quests, absent | 90.271 | 69.306 | 62.631 | 91.432 | 42.604 |
| Item objectives, existing cache | 7.765 | 6.768 | 6.630 | 5.950 | 3.807 |
| Drop list, 8 entries | 0.548 | 0.550 | 0.546 | 0.341 | 0.340 |
| Drop list, 64 entries | 2.028 | 2.019 | 2.012 | 0.341 | 0.342 |
| Vendor list, 64 entries | 2.452 | 2.441 | 2.432 | 0.702 | 0.704 |

The qualifier optimization helps even 0-objective quests because the original
code still reads all four objective slots. The objective BoolCalculatedValue
default interval is 1: its unchanged integer `checkInterval/2` condition means
Calculate can run on each Get. "Warm" denotes existing named/shared objects,
not a forced change to objective refresh behavior.

A has no destination-work benefit with one needed objective or an early hit;
first-sweep early-hit samples include approximately +4 to +15 ns regressions.
V18's combined early-hit result still improves because B removes formatting
work. C changes no creature-only quest work; its corresponding factor is an
identity control, not evidence of a general Value optimization.

A second sweep adds explicit drop-list Reset cases. Background variance is
larger: some unchanged controls differ by about 10-14%. All those observations
remain published. Examples include V18 cold/reset size 1 and 8 at +17.8% and
+6.4%, while identical controls also drift. The cold path is deliberately not
optimized; no cold-path speedup or precise cold regression is established.
The large warm improvements persist in this sweep (4-objective miss
6.376 -> 2.060 us; drop-64 2.904 -> 0.449 us), but the first sweep is the more
stable numerical reference. Do not average unrelated sweeps into a server gain.

## Build and handoff

Windows-only V18 workflow: Visual Studio 2022 x64 RelWithDebInfo,
BUILD_PLAYERBOTS=1, SUPPORTED_CLIENT_BUILD=5875. It runs cumulative tests and
ASan, the existing full Value-chain contracts, and the new quest contracts
normally and with ASan. Artifacts include mangosd.exe, its matching mangosd.pdb,
build-info.json with source HEAD and hashes, runtime DLLs, smoke evidence,
source provenance, benchmarks and this document. CI validates `--version` in
an isolated directory with packaged DLLs; it does not start a world or connect
to a database. Final CI run, binary hashes and PDB identity are reported in the
delivery manifest, after the workflow actually completes.

Only QuestValues.cpp and the two concrete list headers change in production.
V17 A/B/C, `newdistance =+`, Mema, AddCooldown, UseItem residuals,
Item::AddToUpdateQueueOf, paths/routes, AI frequencies, population/radii and
DisableBotOptimizations remain unchanged. No active server files are installed.
