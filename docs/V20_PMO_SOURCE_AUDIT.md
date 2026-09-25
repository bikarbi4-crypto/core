# V20 PMO source audit and report correction

Baseline: V19 `33faa8e3c091d73483b42dc07f8f2934ba5f7958`.
This is a production-source audit, not a claim that all world clocks or all
work in an OFF tick can disappear. V19's sampling and stack ownership remain.

## Scope and reproducible inventory

`tests/playerbots/v20_pmo/generate.py` scans every Git-tracked file under `src`
(1,981 files at this baseline), retaining all matching lines and SHA-256 hashes
in generated `source-audit.json`. Its search covers `PerformanceMonitor`,
`PerformanceMonitorOperation`, `PerformanceData`, the stack and map aliases,
`sPerformanceMonitor`, `PERF_MON_*`, `perfMonEnabled`, `performanceStack`,
`totalPmo`, `HandlePerfMonCommand`, `PMO_MEMTEST`, `MEMORY_MONITOR`, and
`sMemoryMonitor`. Comments are retained as evidence and masked for active counts.

The inventory was supplemented by manual review of every class/type/alias
reference and a repository search of CMake, project/property files and CI
definitions for PMO/memory-monitor defines. It is broader than the existing
PlayerBots-only literal-start inventory. It is still a lexical inventory plus
source review, not a universal C++ data-flow or macro-expansion proof.

| Production use | Result and interpretation |
| --- | --- |
| External `start` | 47 active sites: PlayerbotAI 3; PlayerbotAIBase 1; PlayerbotFactory 21; RandomPlayerbotMgr 7; Engine 9; Value policies 2; AH 2; BuyAction 1; MovementActions 1. All remain externally guarded by `perfMonEnabled`; PMO-only names/WorldPosition are inside the guard. |
| Commented start | One factory EqSets scope; not executable and not included in 47. |
| Monitor aliases | Sole singleton macro `sPerformanceMonitor = PerformanceMonitor::instance()`. No local/reference/pointer/typedef aliases or external alternate singleton calls found. |
| Operation construction | Sole allocation is `std::make_unique<PerformanceMonitorOperation>` in the primary `start` overload, after its OFF return. No external direct operation constructions or factories found. Other type references are declarations and `unique_ptr` owners. |
| Internal start forwarding | The AI overload checks OFF before touching AI/context, then forwards to the primary overload. Both internal forwarding paths are guarded. |
| `Init` | One caller: RandomPlayerbotMgr map pre-warm. The enclosing loop is ON-only; `Init` also checks the flag. No map enumeration or registration from this loop at stable OFF. |
| `Reset` / `PrintStats` | One caller each, in the explicit chat command handler. These intentionally work with collection OFF and operate on already collected data. The `pmo` command registration is in Chat.cpp; declaration is in Chat.h. |
| `performanceStack` | Owned by AiObjectContext; passed only by monitor's AI overload. Primary `start` pushes a name; operation finish removes the innermost matching live frame. |
| `totalPmo` | Member of PlayerbotAIBase. Previous FullTick is reset before the guarded new start; object destruction is also required cleanup. Its cross-call lifetime is unchanged. |
| `perfMonEnabled` writes | Config-load default false and explicit chat toggle; no new changes. Config/thread-safety remains a separate issue. |
| `PMO_MEMTEST` | Defined in PerformanceMonitor.h but has no active consumer found in production or build configuration. It adds no separate OFF instrumentation in this tree. |
| `MEMORY_MONITOR` | Its define is commented out. Four `sMemoryMonitor` hooks are under three `#ifdef MEMORY_MONITOR` blocks: WorldPosition constructor/destructor Add/Rem; manager Print/LogCount. No active build define found. This separate compile-time diagnostic would intentionally be independent of runtime PMO if enabled. |

The previous V19 fragment suite separately verifies exact external guards,
PMO arguments, non-PMO caller bodies and reset order. The broader inventory
does not substitute a nearby textual `if` match for that stronger scope test.

## Work intentionally retained with PMO OFF

* Reading the runtime flag and maintaining empty RAII owners is still work.
* Destruction of an operation begun ON must pop its stack frame after OFF;
  V19 skips finish-clock/counter updates when it ends OFF. ON -> OFF -> ON
  while an operation remains alive retains V19's finish-time sampling policy.
* FullTick `totalPmo.reset()` closes the previous interval; removing it would
  alter lifetime, timing, and cleanup.
* Manual reset and report commands work after collection is disabled.
* `time(0)` in Value refresh, expiry and change tracking is gameplay freshness
  logic. No refresh clocks were removed.
* `WorldRunnable`'s WorldTimer drives world `diff`, sleep cadence, anticrash
  and independently configured `CONFIG_UINT32_PERFLOG_SLOW_WORLD_UPDATE`.
  Perf.log is not the playerbot PMO collector; its logging and clocks remain.

No new unguarded OFF start/preparation path was found. This does not claim
zero OFF CPU use, arbitrary alias absence in future code, or full concurrency
safety. The shared-context lifetime candidate is audited separately.

## Small correctness change: aggregated maximum

PrintStats previously compared and assigned each partition's `minTime` when
building `pd.maxTime`. Both references now use `performanceData.maxTime`.
Example: partitions `(min=2, max=90, time=300, count=7)` and
`(min=5, max=120, time=700, count=11)` formerly displayed max=5, now max=120.

This changes only the maximum column and the category maximum derived from it.
It does not alter the collected records, totalTime, count, keys, filtering,
sort order, timing boundaries, report formatting or collector's hot path.
Partitions with zero totalTime retain the previous semantics: count is added,
but min/max do not participate. No optimization benchmark is attributed to
this report-only correctness fix.

## Verification

`tests/playerbots/v20_pmo` extracts the actual immutable V19 and current
PrintStats/StackString/Reset implementations and production data structures.
Only includes, surrounding namespaces, private access for snapshots and the
logging service are adapted. Real formatting arguments and formatted strings
are captured; the aggregation algorithm is not reimplemented as the subject.
The generator asserts the entire PrintStats source delta is exactly the two
maxTime substitutions.

The contract runs both implementations over empty registries; zero counters;
multiple maps/instances with deliberately different min/max/count/time;
count-only partitions; zero-time partitions with deliberately inconsistent
extrema; and a separate malformed positive-time/zero-count case. All eight
tick/stack/map option combinations and Reset are exercised. It verifies exact
source snapshots before/after print, unchanged count/time/name/min/order and
rendered output after masking only the maximum column, plus independently
specified expected maxima for merged and split partitions. Reset output must
be byte-identical. Standard MSVC and AddressSanitizer builds are required.

Reproduce on Windows VS2022 x64:

```text
cmake -S tests/playerbots/v20_pmo -B build-v20-pmo -G "Visual Studio 17 2022" -A x64
cmake --build build-v20-pmo --config RelWithDebInfo --parallel 1
ctest --test-dir build-v20-pmo -C RelWithDebInfo --output-on-failure
```

Use a separate directory with `-DV20_PMO_ASAN=ON` for ASan. CTest adds the
compiler runtime directory to PATH for that test. Local runs use the existing
CPU-limited runner, four allowed logical processors and a single build job.

## Preserved report limitations

Zero total/totalCount/count denominators can produce NaN/Inf in existing
report calculations; empty/reset-only output is covered and unchanged. A
positive elapsed time with zero count is not produced by normal sequential
finish, but the test characterizes it rather than silently repairing it.
The report also assumes nonempty stack keys (normal start always supplies
one), uses existing uint32 accumulation, and uses millisecond timings.
Consequently it is not a full all-tick latency distribution.

Registry traversal is locked, but V19's non-atomic runtime flag, metric
counters and report/reset versus live finish are not made generally
thread-safe by this fix. No concurrency claim is inferred from ASan or one
successful in-game toggle. These concerns remain a separate bounded audit;
they are not hidden inside the max-column correction.
