# V20 C: local combat-target list nodes

The generator extracts the **actual complete** `RemoveNonThreating` and `Calculate`
bodies from V19 `33faa8e3c091d73483b42dc07f8f2934ba5f7958` and the checkout. It asserts
that Calculate and all other code in the production file are unchanged (allowing
only final newline normalization). Every external predicate remains at its old
source position; the test compares the precise ordered calls, including the
second RTI check and short-circuiting. Production `ObjectGuid.h` must be unchanged.

World APIs are fixtures. GUID storage is uint64 (production ObjectGuid has one
uint64 field and implicit trivial copy/destruction), not a replacement for world
target validation. `IsValid`, CC/RTI predicates, and world refresh internals are
unchanged and not reimplemented here. This measures local list work, not the
total cost of selecting a combat target or overall server performance.

## Safety and behavior

All lists use the same stateless `std::allocator<ObjectGuid>`; allocator equality
required by splice is satisfied. Iteration advances before each node transfer,
so the remaining traversal iterator stays in `targets`. The receiving lists
preserve order and duplicate GUIDs. The input belongs to the caller's local
`Calculate` result; `RemoveNonThreating` is private and has only those two callers.
No iterator/reference escapes. The first ordinary getOne target keeps its node,
and prefix/suffix ranges are erased. The subsequent CC fallback still prefers
unbreakable over breakable CC and can still return **multiple** targets with getOne.

Changing list node addresses is intentional; game-visible GUID order is identical.
The test explicitly verifies the candidate retains selected nodes, including the
first fallback node. Removed allocations naturally remove their allocation-failure
points; identical behavior under injected out-of-memory is not claimed.

The function has no inter-tick cache, expiry, PMO operation, or shared mutable
storage. Its surrounding Value refresh/PMO policy, all target rules and all
predicate bodies remain unchanged. ON/OFF flags appear in parity fixtures to
confirm this local calculation has no dependency on them; this is not an
independent PMO instrumentation test. Full PMO/Value contracts run separately.

## Verification

89,360 paired observations in both normal MSVC and MSVC AddressSanitizer builds:

- Every sequence of length 0..3 drawn from null GUID plus all 32 combinations of
  validity, first/second RTI answers, breakable and unbreakable CC (74,120 cases).
- 12,000 deterministic random lists up to 256 nodes, including duplicate GUIDs.
- 440 injected external-predicate exceptions/reentry cases. Both variants have
  identical partial list state, nested result, exception outcome and call trace.
- 2,800 actual Calculate scenarios: activity/in-world/teleport flags, PMO flag,
  empty/0/1/2/negative/invalid/overflow qualifiers, single attackers list and full
  fallback, ordinary/CC-only/invalid/empty inputs. Exceptions from stoi agree.
- Independent CC preference, multiple getOne fallback and node identity checks.

Build/run on Windows:

```text
cmake -S tests/playerbots/v20_targets -B build-targets -A x64
cmake --build build-targets --config RelWithDebInfo --parallel 1
ctest --test-dir build-targets -C RelWithDebInfo --output-on-failure
build-targets/RelWithDebInfo/targets_audit.exe --allocations allocations.csv
build-targets/RelWithDebInfo/targets_benchmark.exe --benchmark timings.csv
python tests/playerbots/v20_targets/summarize.py timings.csv allocations.csv summary.json
```

Repeat configure/build/test in a separate directory with `-DTARGETS_ASAN=ON`.
CTest adds the compiler directory containing the ASan runtime to the test PATH.

## Microbenchmark

MSVC x64 RelWithDebInfo `/O2`, `/GL-`, `/LTCG:OFF`, no identical-code folding;
extracted functions have explicit noinline boundaries. Real CRT allocation in the
benchmark executable; custom new/delete counters exist only in the separate audit
executable. No allocation-counter branches are charged to timing results.

40 fixtures = lengths 0/1/8/64, ordinary/breakable/unbreakable/invalid/mixed, both
getOne modes. Two scopes: warm prebuilds independent input lists outside the timer;
cold includes their construction and destruction. There is no persistent cache.
Each variant processes the same number of inputs; calibration targets 6 ms on the
faster variant (capped at 65,536 inputs), followed by 9 alternating-order rounds.
Round zero is discarded; summarize medians of 8, retain all 1,440 raw timing rows.
Benchmarks are pinned to one logical CPU; fixtures contain deterministic simple
world predicates, so gains cannot be extrapolated to full-game CPU percentages.

Allocation counts cover `RemoveNonThreating` itself, excluding input construction.
MSVC still allocates two list sentinel nodes (48 bytes total). No target node is
newly allocated by V20. E.g. 8 CC targets: 18 -> 2 allocations; 64 CC targets:
130 -> 2; one ordinary getOne target: 4 -> 2. Every allocation row checks output
size/checksum against V19. Cold list input allocation remains required.

Local sample medians, nanoseconds per call (all raw negatives retained):

| Fixture | Warm V19 -> V20 | Cold V19 -> V20 |
|---|---:|---:|
| 1 ordinary, getOne | 102.77 -> 57.10 | 151.49 -> 105.86 |
| 8 breakable CC, all | 483.53 -> 104.42 | 779.47 -> 418.37 |
| 64 breakable CC, all | 3628.46 -> 555.78 | 5220.37 -> 2086.73 |
| 64 mixed, all | 1883.40 -> 957.27 | 2996.40 -> 2037.12 |
| 8 ordinary, all (negative) | 114.31 -> 117.46 | see raw CSV |
| 64 ordinary, all (negative) | 666.25 -> 679.67 | see raw CSV |

Unchanged ordinary-all/empty/invalid paths can show small regressions or noise;
the worst observed relative negative was +2.76% (3.16 ns), ordinary-all length 8.
No zero-regression claim is made.
