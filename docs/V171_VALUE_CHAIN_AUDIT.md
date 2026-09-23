# V17.1: Value lookup audit and scoped RTTI correction

The accepted gameplay control remains V16, `cea7ba955f5c0d57ff76d63e53f97f34b1d3ab4c`.
V17 is preserved at `e93226fb808e71e2d221bfa9baba950e2bb248f0`.
V17.1 descends from that exact V17. Its only gameplay source change is in
`Value.h`: remove the incidental `ValueBase<T>` inheritance level from
ordinary non-list values and restore the original primary `Value<T>` interface.
The list specialization retains its exact V17 interface and base. The list metadata methods, refresh
policy, Get copy semantics, PMO behavior, V17-A/B and all cumulative V0-V16
code remain intact.

This corrects a measured **failed-type-cast** overhead. It does **not** establish
that the normal successful Value path slowed down in V17, or that this small
cost caused the observed gameplay diff. Do not promote a new gameplay baseline
or claim a server-wide percentage speedup from these tests.

## Gameplay report independently checked

The supplied V17 Server.log SHA-256 is
`8ea89ed7c1ed0af290b7f5cfaf74958673e86323f0216c86faf77730a738dad7`;
Loot.log SHA-256 is
`8a217bad0ad7523f47303c08a8e34255d67b5e34d9cef0a374094bf80decaae3`.
The server records revision `e93226fb808e71e2d221`, 2026-09-23
06:16:21 to 07:05:20, and a 19-second PMO window at 07:04:45-07:05:04.
The final 149/101 ms reading is contaminated by that window.

The last ten clean V16 readings average 2272.3 active, 90.1/90.1 ms; V17
2248.6 active, 94.3/92.9 ms. Their spans differ: 10m08s versus 2m39s.
At minutes 40-47 the means are 2243.4, 86.5/86.2 (V16, n=10) and
2230.73, 95.13/97.27 (V17, n=15). These are unequal, correlated samples
from different evolving world states, not paired performance trials.

V17's 11 overall queue snapshots and 143 partition rows are all zero.
691 AddCooldown and 137 UseItem entries remain; result 60 appears 40 times,
46/80/76 do not. Item queue and pet primary-key errors are absent in this
session. One MoveSpline validation failure is present, with no established
connection to V17. Session-filtered loot: 2225 Creature records and 79
GameObject records, 56 bot GUIDs and 61 source entry/GUID pairs. These counts
are neither kills nor numbers of chests. Logs do not establish loaded DLL
paths/hashes, nor close older unrelated TODOs.

## Full-chain measurement

`tests/playerbots/full_chain` extracts production `PlayerbotAIAware`,
`AiObject`, `AiNamedObject`, `UntypedValue`, relevant Value policies, the
entire named-object AVL cache/factory/context, and `AiObjectContext`.
The measured consumer executes the production lookup, virtual
`GetUntypedValue`, MSVC `dynamic_cast<Value<T>*>`, and Get/GetSize/IsEmpty.
Factories and consumers are separate translation units; `/GL-`, `/GR` and
`/LTCG:OFF` prevent whole-program knowledge from replacing RTTI. Compiler
listings confirm the virtual lookup and `__RTDynamicCast` remain.

Controlled substitutions: bot/world initialization, value registration,
Calculate's returned fixture data, unused world-dependent Format methods,
PMO services (off, external state), and an 8-byte ObjectGuid fixture.
The real CRT clock is used. The cache contains 128 filler keys. These are
operation measurements, not a simulated server workload. Constructors and
first population are outside timed batches; missing keys are cached misses.

VS2022 Community 17.14.27 / MSVC 19.44.35223, x64 RelWithDebInfo
(`/Zi /O2 /Ob1 /DNDEBUG`), Ryzen 7 7800X3D. Single-thread measurements,
12ms target batches, one warm-up round and eight retained rounds, rotating
and reversing variant order. All raw samples and negative cases are retained
in the delivered validation archive. Medians and ranges are under
`docs/validation/v171-*`. Identity controls matter: A+B-only and V16 have
identical Value code; C-only and full V17 also have identical Value code.
Their variation measures executable layout and environmental noise.

Coverage: bool, uint8, uint32, float, Unit pointer and ObjectGuid list;
warm, forced refresh, qualified name, cached missing key, deliberate type
mismatch; calculated/manual/single/memory/log policies where applicable;
list lengths 0/8/64; separate lookup-only and cast-only diagnostics.
Refresh mode uses interval zero, or Reset before each Single Get. Memory/log
policies use a constant calculated value. Exact expected results, including
null cases, are checked independently of timing.

Repeated original V16/V17 runs found successful warm scalar Get around
68 ns for calculated values and 61-62 ns for manual values, with no common
V17 regression. The extra RefreshValue helper is inlined into the measured
scalar Get implementations. Successful RTTI is around 30-35 ns and stops
at Value before the additional base. A deliberate wrong-type lookup scans
the full hierarchy and costs about 6 ns more in V17. The scoped V17.1
change removes that extra RTTI descriptor for non-list types. Lists retain
their V17 hierarchy, including its failed-cast cost. Frequency of wrong-type lookups
in real gameplay is unknown; usual unchecked GetValue calls expect a
matching type.

For a warm calculated list of eight GUIDs, full size/empty access is about
265-270 ns in V16 versus 68 ns in V17. An ordinary list Get still copies;
manual/single policies keep their virtual Get fallback. Refresh measurements
include Calculate, so the size improvement is smaller there. See retained
measurements rather than assuming every scalar or every list read improves.

The initial broader candidate also removed the list interface base. Two
process runs showed a repeatable loss in the manual/single list GetSize
fallback: about 22-24 ns for eight entries and 230-240 ns for 64 entries.
Ordinary Get and IsEmpty did not show that loss. The normalized compiler
instructions of GetSize and its Get callee matched, so a precise machine-level
cause was not established; code/data layout is a possible contributor, not
a proven explanation. This candidate was rejected. Final V17.1 leaves the
entire list hierarchy and policies byte-for-byte equal to V17, with the
non-list RTTI correction retained. Final measurements are `v171-full-chain-final.csv`
and `v171-full-chain-final-repeat.csv`; `v171-discarded-broad-candidate*.csv`
retain the rejected candidate's results. Do not confuse its superseded CI
run with the final HEAD's build.

The final repeat ran with substantially higher background variation: even
identical-code controls diverged strongly, and the whole-PC CPU sample peak
rose from 26.27% to 41.89%. Those rows are retained, but cannot support a
precise timing conclusion or an inferred regression. The clean final run,
source identity checks, and semantic tests support the narrower correction;
they do not establish a live-server performance gain.

## V17-B amortization

`tests/playerbots/grind_audit` extracts complete production grind methods
for V16, C-only (= V16 grind code), A-only, A+B-only (= V17 grind code),
and full V17. Counts are gathered in separately instrumented, untimed copies;
timing uses unmodified extracted bodies. The game services are controlled
fixtures, including the pre-existing fixture Value context. Thus the grind
test isolates A/B and does not remeasure C's production RTTI cost. No four
server executables or four hourly gameplay runs are requested.

115 scenarios cover solo and groups 2/5/10/40, candidate lists 0/1/2/3/4/16/64,
zero/one/two survivors after filtering, attacker early exit, and failed assist
passes. Target, random draws, debug messages, list reads, distance calls and
filter/assist counts match across variants. Existing 2195 deterministic grind
regression scenarios are also retained.

The raw-list `>2` threshold is **not globally optimal**. For group 10,
three candidates and one survivor, A-only is about 0.96 us and V17 1.16 us.
For group 40, four candidates and one survivor, roughly 2.05 vs 2.46 us.
Building/sorting a one-use index costs about 0.1-0.4 us in these cases.
Zero survivors and early attacker returns do not build the targeting index.
Large useful cases still improve: group 5 x 16, V16 about 3.8 us vs V17
2.3 us; group 40 x 64, about 71 us vs 14 us in this fixture.

B is preserved: raising a raw-count threshold merely moves the break-even
point, and delaying the snapshot can change member-target read semantics.
There is no runtime distribution of surviving candidates or changed-branch
invocations in these logs. PMO T/V totals cannot supply that distribution.
If B needs a subsequent change, first agree on a narrow aggregate capture:
bounded counters per group-size bucket for raw candidates, candidates actually
reaching targeting count, assist passes, early exits, index builds and queries.
Dump once after a short separately identified diagnostic window, outside the
PMO-off control measurements. No per-call persistent logging was added.

## Validation and next control

Normal and MSVC AddressSanitizer cumulative tests pass 4/4 locally. The new
full-chain contract mode (`full_chain_audit --check`) is also included in
Windows CI. Timing values are not brittle test thresholds. The Windows
workflow builds mangosd, validates source and ASan tests, checks the full
chain, records exact EXE/PDB/DLL identities, and performs only an isolated
`--version` load test with no server or database startup. See the actual CI
run and build-info manifest for completion; this document alone is not
evidence that a particular CI run finished.

The user requested limited local CPU load: heavy local commands inherit a
four-of-16 logical CPU affinity; compiler parallelism is at most two projects
and one compiler process per project. Benchmarks use one CPU. Whole-PC CPU
samples are included in the validation archive. Unrelated applications remain
outside this task's control. The full mangosd build runs on GitHub.

Keep V16 as control. The next practical game test is one preserved V16 run
for 45-60 minutes with the same configs/DLLs/background applications and
roughly matched diff/cpu observations at 20/30/40/45/50 minutes, PMO off.
Save that log separately. Sequential sessions do not restore an identical
world; a stronger paired test requires separate copies from the same backup,
not rolling back the live database. V17.1 remains a candidate pending gameplay.

No changes to Mema, AddCooldown, UseItem, item saving, `newdistance =+`,
routes/pathfinding, AI timing, configs, population, radii or live DLLs.
