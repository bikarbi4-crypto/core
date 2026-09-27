# V22 actual-source validation

Baseline `6a32bb9c97d45166719a9b5a9d3e0f0c0c8ac4c7`. Standalone programs only;
never execute mangosd, a game client, or a database.

`quest` extracts complete NeedForQuest/NeedQuestObjective, GetDestinations,
Value/cache/RTTI and shared ownership code. The deterministic world services
are inherited from the V18 audit. Its extra `chain_test` binds complete,
unchanged GrindTarget source to those actual quest functions through a fixture
Value service. It compares GUID, RNG trace, duplicate candidates, list reads,
quest/refresh traces for 0/1/8/64/512 candidates, no target, attacker-first,
quest misses, group/duplicates and filtered candidates, PMO OFF and ON.
AttackAnything's untouched wrapper is source-audited, not simulated as a live
game session. World/quest services, not C++ standard containers, are fixtures.

`plumbing` extracts the actual FlyDirect function/signature, TravelPath and
PathNodePoint, complete MoveToTravelTarget::isUseful, Event, WorldPacket and
Trigger::Check/needCheck. Fixtures replace world services and ByteBuffer IO.
WorldPosition retains the real x64 data layout/copy constructor with a test-only
copy counter; no production instrumentation is added. MEMORY_MONITOR has an
additional ASan build requiring original path-copy counts. MoveTo/Execute and
pathfinding source is protected by the full-file scope gate; this suite is not
a navigation mesh simulator. Short/long/empty paths and changed target coordinates
exercise the disabled flight probe without changing the stored path.

Only the `*_test` binaries contain semantic trace probes. Only `*_allocations`
replace global new for allocation/byte counts. `*_benchmark` contains neither.
Timings rotate variant order, use the same work, and retain all rounds, including
negative/no-benefit samples. Parser exceptions, trigger callback exceptions,
reentry/qualifier mutation, Event copy/self-assignment, packet/GUID/owner and
SSO/long strings are tested. Removing allocations necessarily removes their
possible bad_alloc sites; identical failure-at-allocation-N is not a contract.

The source boundary freezes six approved changed files against V21 and rejects
any other source modification. Older audit generators normalize exactly these
hash-checked files back to V21 for their historical scope gates; new suites
compile the actual current bodies. Prior tests and assertions are retained.
Presence, scheduling, PMO implementation and every other source file must remain
identical to V21. No new shared state or concurrent gameplay path is introduced;
existing V20 shared-registry and V21 lifecycle concurrency tests remain required.

Build each subdirectory with VS2022 x64 and `-DV22_ASAN=ON` for ASan.
Run `ctest -C RelWithDebInfo --output-on-failure`. Normal builds expose
`--operations` and `--bench`; quest also provides `--parser-bench`.
Allocation counts and timings are separate outputs: timings from allocation
probe binaries are not performance evidence.
