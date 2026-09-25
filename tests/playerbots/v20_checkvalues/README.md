# V20-B CheckValues allocation audit

The production change touches only the six unused list copies at the end of
`CheckValuesAction::Execute`. Each becomes a discarded `GetSize()` call. It keeps
all six lookups, their ordering, the debug/map branches, refresh checks and PMO
names. No cache reference escapes and no generic Value policy changes.

The generator reads immutable V19 `33faa8e3c091d73483b42dc07f8f2934ba5f7958` and
the working tree. It extracts both **complete Execute bodies**, the unchanged
Value/CalculatedValue/ObjectGuidListCalculatedValue classes and the actual RTTI
`AiObjectContext::GetValue(name)` method. It rejects any difference outside the
six replaced statements and their two explanatory comments. Source and extracted
fragment hashes are written to `generated/provenance.json`.

## Registration and policy audit

The generator scans all production `.h`/`.cpp` files for registrations of the six
exact keys, including class-specific contexts. Each has one registration, in
ValueContext.h. All six ultimately inherit ObjectGuidListCalculatedValue, whose
Get/GetSize/IsEmpty overrides are final; ordinary Get and scalar observation both
call the same RefreshValue. They customize world searches through Calculate and
FindUnits/AcceptUnit, without a different cache policy.

| Lookup key | Registered concrete type | Parent chain | Interval | PMO name |
|---|---|---|---:|---|
| possible targets | PossibleTargetsValue | NearestUnitsValue | 4 | possible targets |
| all targets | AllTargetsValue | PossibleTargetsValue → NearestUnitsValue | 4 | all targets |
| nearest npcs | NearestNpcsValue | NearestUnitsValue | 4 | nearest npcs |
| nearest corpses | NearestCorpsesValue | NearestUnitsValue | 4 | nearest corpses |
| nearest game objects no los | NearestGameObjects, LOS_IGNORE | ObjectGuidListCalculatedValue | 1 | nearest game objects |
| nearest friendly players | NearestFriendlyPlayersValue | NearestUnitsValue | 4 | nearest friendly players |

The GO value retains its existing interval 1. The existing integer `interval/2`
refresh test is zero, so GO Calculate still runs on **every** read, including the
warm test. Changing this behavior or renaming its PMO key is outside this patch.
Unknown policies keep the existing `Value<list>::GetSize()` virtual Get fallback.

## Boundaries and checks

World searches, bot debug/map services, AiNamedObject construction, context
storage, and the PMO sink are fixtures. The world produces ordered GUIDs including
duplicates; list storage is real MSVC `std::list<uint64_t>` with the production
refresh and read bodies. World/grid work and the full PMO implementation are **not
benchmarked**. Context storage uses a fixture map and the actual RTTI GetValue
method. PMO fixtures retain operation construction/destruction and record all
production start names/order; the cumulative PMO suite covers the real monitor.
AiNamedObject retains PlayerbotAIAware's virtual destruction contract and the
harness checks every owned probe is destroyed, including exceptional paths.

2,560 paired observations cover ON/OFF, 0/1/8/64/512 GUIDs, cold/warm/expired/reset,
Set, mixed expiration, nested reads, Calculate exceptions, all 16 debug/map/short
movement flag combinations, and the unknown-policy virtual Get fallback. Exact
results, external call order, refresh clocks, PMO start/finish sequence, cached
GUIDs and timestamps must match. Calculate allocation counts match separately;
fallback allocation counts must be identical. An allocation hook counts actual
MSVC allocator calls; trace formatting/storage is excluded. Assertions remain on
in RelWithDebInfo. ASan is enabled in a separate build.

## Reproduce on Windows

```text
cmake -S tests/playerbots/v20_checkvalues -B checkvalues-build -G "Visual Studio 17 2022" -A x64
cmake --build checkvalues-build --config RelWithDebInfo --parallel 1
ctest --test-dir checkvalues-build -C RelWithDebInfo --output-on-failure
checkvalues-build/RelWithDebInfo/checkvalues_audit.exe --allocations allocations.csv
checkvalues-build/RelWithDebInfo/checkvalues_benchmark.exe --benchmark benchmark.csv
python tests/playerbots/v20_checkvalues/summarize.py benchmark.csv summary.json
```

Configure a separate directory with `-DCHECKVALUES_ASAN=ON` for ASan; CTest supplies
the compiler runtime directory on PATH. Select an explicit Python3 executable if
multiple Python installations exist. Use the workspace CPU limiter for local runs.

The benchmark pins one logical CPU (mask 4), calibrates both variants to at least
approximately 12 ms, uses equal iterations and alternates variant order for nine
rounds. Round zero is discarded only from summaries; all raw rows and negative
medians remain. Clock reads execute the real CRT time call but return a controlled
logical time to give both variants identical cache work. Cold constructs/destroys
the fixture context and production policy objects per iteration; warm reuses
them; expired advances two seconds per iteration; reset calls Reset on all six.
This measures the copy-removal mechanism under explicit cache workloads, not
whole-server throughput or the complete production cache construction cost.
Timing uses a separate executable with the allocator override excluded entirely;
the allocation-counting executable is used only for contracts/counts. An earlier
allocator-instrumented timing CSV is retained with `instrumented` in its filename
and excluded from the final timing comparison.

An initial local benchmark was invalidated during fixture review because its
AiNamedObject stub omitted virtual destruction. Its raw CSV is retained as
`checkvalues-benchmark-invalid-fixture-first.csv` and is excluded from results.
The corrected suite explicitly verifies destruction and reruns normal/ASan tests
and benchmarks. No production patch was needed for that harness issue.
