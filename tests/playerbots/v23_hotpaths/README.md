# V23 same-work validation

Baseline: V22 `24e11eb41b85ab74437dfc1737289c2bcca42de4`. Fetch complete Git
history first. Requires Windows, Visual Studio 2022 x64, CMake and Python 3.
These programs never start or connect a server, game client or database.

For each `SUITE` = `attackers` or `log_action`:

```powershell
cmake -S tests/playerbots/v23_hotpaths/SUITE -B build-v23-SUITE -G "Visual Studio 17 2022" -A x64
cmake --build build-v23-SUITE --config RelWithDebInfo --parallel 2
ctest --test-dir build-v23-SUITE -C RelWithDebInfo --output-on-failure -V
& build-v23-SUITE/RelWithDebInfo/SUITE_benchmark.exe --measure
& build-v23-SUITE/RelWithDebInfo/SUITE_allocations.exe --measure
```

Use a separate build directory with `-DV23_ASAN=ON` for AddressSanitizer; CTest
adds the matching runtime directory to PATH. Timing/allocator probes are separate
executables; ASan only builds the parity executable. Assertions remain enabled.
The timing executable has neither trace recording nor global-new interception.
Use `--baseline` before source edits to measure only the unchanged baseline;
the local evidence includes matching baseline/current source hashes at that time.

Nine alternating rounds, warmup, fixed CPU affinity, no LTO/ICF, observable result
sinks. All raw cases are retained, including negative/control results. The local
runner limits compilation to two jobs and four of eight logical CPUs. Counts
are allocations/requested bytes per call, not peak live memory. Durations include
the same fixture services and result consumption in both variants; do not treat
them as whole-server or on-disk logging measurements. Shared-target duplicates,
set iteration ordering and registry/refresh/RNG observations are parity checks.

Attackers: complete production AttackersValue.cpp, actual Qualified and Value
refresh policies, and PossibleTargets Calculate/FindUnits/AcceptUnit. World
objects, grid/predicate services, registry factory, clock and PMO sink are explicit
fixtures. The allocator arena imposes the same pointer order in both variants;
it is not a model of live cross-thread ownership. No fixture object escapes.
Matrix: 0/1/8/64/512 candidate units; 0/1/5/40 group members; humans and bots,
master, pet/guardians, duel/BG/PvP, activity/focus/invalid targets; shared value
fresh/expired/missing/wrong-type/distant; duplicate GUIDs; getOne; valid/invalid
qualifiers; int32 range boundaries; refresh/reset; reentry, callback changes and
exceptions; PMO on/off; errno 0/EDOM/ERANGE. 6755 observations.

LogAction: complete production method, actual formatting/history logic. Bot,
logger and test-file I/O are fixtures that check emitted bytes and call order.
Matrix includes 0..4096 history bytes, delimiter positions around 512 or absent,
messages up to 1000 bytes, four modes (normal/group-filtered/group-enabled/test),
embedded NULs, mixed formatting, reentry/exceptions, errno, 2048 repeated actions
per mode, and PMO on/off. 26876 observations, including retained string capacity.
The existing unchecked 1024-byte vsprintf buffer is tested only in its defined
domain. Allocation-failure-on-Nth-allocation equivalence is not claimed.

`v23_scope.py` freezes the two whole changed files, rejecting any other production
edit or untracked production source. V21/V22 scope adapters normalize only these
exact frozen patches for historical cumulative assertions; new V23 suites execute
the actual modified functions. This does not suppress PMO or presence checks.

Raw CSVs and local logs: `docs/validation/v23-*`. CI results are packaged separately
without overwriting local evidence. Baseline runtime/gameplay remains user-owned.
