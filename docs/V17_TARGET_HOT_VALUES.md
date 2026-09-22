# V17: target selection and hot list observations

Development starts at accepted cumulative V16
`cea7ba955f5c0d57ff76d63e53f97f34b1d3ab4c`, on the separate
`vmangos-v17-target-hot-values` branch. The V16 branch is retained.
The exact build revision and binary hashes are in the artifact's `build-info.json`.
V0-V16 history remains ancestral to this branch; V17 changes five gameplay source
files and does not discard the cumulative ports.

## V16 acceptance evidence

The original local Server/Loot logs were independently re-counted for the session
2026-09-22 18:46:16 through 19:43:26 (57 minutes 10 seconds):

* Core revision `cea7ba955f5c0d57ff76`, orderly `Halting process`.
* Last ten observations before PMO: mean 2,272.3 active bots and 90.1/90.1 ms
  (10-second/60-second diff windows).
* 23 overall packet snapshots and 299 partition rows, all pending counts zero.
* 2,456 Creature loot records and 93 GameObject records; the latter involve
  66 bots and 70 distinct object entry/GUID pairs.
* AddCooldown 857; UseItemAction 361; Item::AddToUpdateQueueOf 39 for 15 item GUIDs.
  UseItem code 60 remains 40 times. Codes 46/80/76 and spells 22563/22564 are absent;
  pet_spell.PRIMARY and GameObject lost-owner warnings are absent in this log.

PMO ran from 19:38:48 to 19:42:59 (251 seconds). The final 140/137 ms sample is
not an uninstrumented control. These logs do not independently quantify the
profiler's overhead. Different V15/V16 world states cannot establish an exact
server-wide speedup. Normal observed gameplay is the user's acceptance report;
log coverage is not proof that every possible gameplay path was exercised.

Printed PMO Value rows reproduce the supplied hotspot aggregates:

| Value | Inclusive printed seconds | Printed measurement count |
| --- | ---: | ---: |
| grind target | 39.218 | 48,990 |
| need for quest | 31.226 | 117,909 |
| attackers | 21.839 | 184,292 |
| attackers count | 19.189 | 187,941 |
| need quest objective | 11.429 | 2,013,387 |
| has enemy players | 3.272 | 24,864 |

These are nested, threshold-filtered printed rows, not disjoint CPU percentages
or a complete enumeration of all Value calls. Raw game logs are not committed.

The V15 binary fallback remains SHA-256
`a4922a55e1e5bfbe8b88cc78af949b86450c0a17a933713bf5a8812e55cd7a5b`.
No live server, database, configuration or population settings are changed by
preparing this release.

## Included candidates and semantic limits

**A — lazy group scratch.** The resolved member vector is owned by one
`GrindTargetValue::Calculate` invocation and prepared only at the original
group-snapshot point. It preserves member order and keeps live per-candidate
alive/distance checks. A different group pointer rebuilds it. The audited
selection path reads group membership; it does not run group-changing actions.

The proposed blanket hoisting of target lists and travel Values is deliberately
not included: their Get/refresh policies and read order remain intact across
assist passes, including a refresh during a long calculation. The master read
also stays on its original path. No borrowed list reference is held across
nested selection helpers.

**B — targeting counts.** For more than two possible targets, group selections
are captured lazily once and represented by two sorted key arrays. Equal-range
lookups count duplicates. Bot targets retain exact `Unit*` equality; human
selections retain full `ObjectGuid` equality. Merging them into a GUID-only map
would incorrectly equate distinct Unit objects with the same GUID. Self, dead,
missing members, null bot targets and ordinary-player selections follow V16.

One or two possible targets use the original comparisons over the already
resolved member vector. This avoids the measured index setup cost on tiny
inputs. No target index survives Calculate. Existing random draws, candidate
order, `newdistance =+ ...`, and V16's per-Calculate needForQuest cache remain.

**C — list metadata without copies.** List-valued interfaces expose scalar
`GetSize()`/`IsEmpty()` operations. By default they call the existing virtual
Get, so manual/specialized policies retain their behavior. Only the standard
ObjectGuid calculated-list family opts into observing its refreshed storage
directly. Its Get is final to prevent a future Get override from silently
disagreeing with the metadata path; Calculate remains virtual.

Get still returns by value. The refresh condition, lastCheckTime updates,
Calculate, PMO scope, LazyGet, Set and Reset semantics are preserved. Metadata
returns only a size or boolean, so no reference can escape, dangle after context
destruction or be retained across a tick. There is no new persistent cache.

The four requested attackers/possible-target consumers are changed. The fifth,
`has enemy players`, is the same immediate empty check and independently appears
in the measured PMO (3.272 printed inclusive seconds). Other list traversals and
pathfinding consumers are left unchanged.

Excluded: Mema, AddCooldown, UseItem residuals, Item::AddToUpdateQueueOf,
AI frequency, population/radii, DisableBotOptimizations, route/pathfinding.

## Differential tests and local measurements

Tests extract production implementation bodies from immutable V16 and the
checked-out source. The A-only source is located by the ancestral `V17-A:`
commit. A full Git history is therefore checked out by the Windows workflow.
Game services are controlled test doubles; these tests do not load a world/DB.

* 2,195 deterministic V16/A/V17 selection scenarios compare target identity,
  random sequence, debug output, list/travel refresh reads and distance ordering.
  They include solo, groups up to 40, early hostile targets, empty/large lists,
  success/failure, varied assist counts, mixed/dead/missing members, duplicate
  slots, same-GUID/different-pointer targets and repeated Calculate calls.
* The Value test uses the actual policies and all five consumer bodies. It checks
  refresh intervals 0/1/2/3/4/5/6/30, Calculate, PMO, LazyGet, Set, Reset,
  reentrant reads, reset during Calculate, manual/single/memory behavior, empty
  and nonempty lists, existing uint8 truncation and zero element copies on warm
  metadata reads.
* The V16 production-context/AVL and list ownership tests are retained.
* Local normal and MSVC AddressSanitizer suites both pass 4/4. CTest explicitly
  supplies the matching compiler ASan runtime path, even outside a Developer Prompt.

Local timing uses MSVC 19.44.35223, x64 RelWithDebInfo (/O2). Grind timings use
the whole extracted selection algorithm with deterministic service doubles,
alternating measurement order and median batches of roughly 40 ms or more.
Value timing uses plain 64-bit list elements and the real CRT clock on warm reads.
No benchmark was run concurrently with the full server compilation.

Representative combined A+B results, microseconds per Calculate:

| Group members | Candidates | V16 | V17 | Current-target reads V16 -> V17 |
| ---: | ---: | ---: | ---: | ---: |
| 5 | 1 | 0.812 | 0.806 | 3 -> 3 |
| 5 | 2 | 1.116 | 1.095 | 6 -> 6 |
| 5 | 16 | 4.186 | 2.358 | 48 -> 3 |
| 5 | 64 | 12.902 | 5.436 | 192 -> 3 |
| 40 | 1 | 1.992 | 1.858 | 26 -> 26 |
| 40 | 16 | 20.469 | 5.503 | 416 -> 26 |
| 40 | 64 | 76.574 | 13.986 | 1,664 -> 26 |

A alone reduces a repeated failed 40-member snapshot from 1,600 player lookups
to 40. Its benefit is concentrated in retries, not a successful first pass.
Small control paths do not universally improve: the largest observed relative
increase in this microtest is 11.6%, about 29 ns on an empty solo case. Full
72-scenario results, including regressions, are in the companion CSV.

C warm size read: an 8-element list measured 212.8 ns -> 8.0 ns, a 64-element
list 1,629.4 ns -> 7.9 ns. An empty list measured 29.8 ns -> 8.1 ns (MSVC's list
copy still constructs a sentinel). Refresh/calculation work is not removed.
These figures measure the stated operations; they are not a server-diff or
bot-capacity forecast. Real V17/V16 workload comparison remains necessary.

## Windows build and artifact

Visual Studio 2022, x64, RelWithDebInfo, target mangosd, BUILD_PLAYERBOTS=1,
SUPPORTED_CLIENT_BUILD=5875, BUILD_EXTRACTORS=0, ENABLE_MAILSENDER=1,
BUILD_WARNINGS_AS_ERROR=0. cURL remains pinned to 8.10.1 commit
`7eb8c048470ed2cc14dca75be9c1cdae7ac8498b`.

The Windows-only workflow builds mangosd, runs all four normal tests and all
four ASan tests, then publishes
`vmangos-v17-target-hot-values-windows-x64-relwithdebinfo` with:

* mangosd.exe and its matching mangosd.pdb;
* build-info.json with source identity and EXE/PDB/DLL hashes;
* runtime/libmySQL.dll, runtime/libeay32.dll, runtime/libcurl.dll,
  runtime/libssl-3-x64.dll and runtime/libcrypto-3-x64.dll;
* source/ASan test logs and extracted-source provenance.

The runtime folder contains the repository's application DLLs, pinned cURL and
OpenSSL 3.0.14 rebuilt from commit `9cff14fd97814baf8a9a07d8447960a64d616ada`.
The OpenSSL 3 version matches the inspected working installation; its license
is included. The core already imports this SSL DLL through its checked-in
import library. Both local and CI builds have that dependency. No installed
server DLLs are uploaded. Before publishing, CI runs only `--version` with the
packaged DLLs in an isolated directory and a Windows-system-only PATH; the
resulting loader/revision check is in `validation/runtime-smoke.json`.
It does not replace an installation automatically. Its bundled libmySQL.dll
requires the Microsoft VC++ 2008 x64 CRT; the other application components need
the VC++ 2015-2022 x64 runtime. Existing working server DLLs should not be blindly
replaced. No database/configuration upgrade or server start is performed.

Use an isolated copy for gameplay acceptance and compare against cumulative
V16 with identical settings. Keep PMO windows short and compare uninstrumented
samples separately. V15 remains available as the older binary fallback.
