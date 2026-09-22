# V16 cumulative source recovery

The working gameplay reference is `mangos_V15.exe`, SHA-256
`a4922a55e1e5bfbe8b88cc78af949b86450c0a17a933713bf5a8812e55cd7a5b`.
The source starting point is **candidate** commit
`b81cfd7e2bef1c5e693db64e52727ecbf7c09511`; an exact source-to-original-EXE
match has not been established. Compilation does not establish gameplay parity.

## State actually present when work resumed on 2026-09-22

GitHub refs, commit history, full source diffs, Actions jobs/logs and the local
V0-V15 patch packages were inspected. The progress captions on PDF pages 124-125
were not used as proof of completion.

* `vmangos-v16-target-selection`: `5e38a6cd31c78ce22fd5f1cb8726d9c4cd999d34`.
  Its gameplay diff changes only `GrindTargetValue.cpp/.h`; the other changed
  file is `V16_TARGET_SELECTION.md` documentation.
* `vmangos-v16-ci-trigger`: `7137a3a659cd8159a118e85b44f8a3ea1691d212`.
  Windows Actions runs `35651265638` and `35651271774` succeeded. They validate
  the V16-only sources plus CI configuration, not a V0-V15 cumulative build.
* `vmangos-v16-cumulative`: `7cc8539b6714a461051045bd6e6b4a50b4727eec`,
  **19 commits**, 17 changed files, 248 insertions and 62 deletions above the
  candidate. V9 was absent.
* `vmangos-v16-cumulative-ci`: `0688a6fb34b1c5beb2d8c802d310e13c3e240a1c`.
  Run `35657939977`, job `106526078862`, failed with C2027 in `FollowActions.cpp`
  and `UseItemAction.cpp`: missing complete `Movement::MoveSpline` definitions.
  This CI ref also predates the last three pre-V0 source commits.

Original cumulative commits (oldest first):

| Area | Commit | Audit result |
| --- | --- | --- |
| V0 GameObject loot dispatch | b37e62ca | Present; opening movement needed completion |
| V6 direct Event return | f2baf38a | Present |
| V7 one successful context lookup | 639c7497 | Present |
| V8 copy key only on miss | e9f82ff6 | Present |
| V10 compare queue names by reference | 4f6f9a80 | Present |
| V11 trigger handler list | e261eb33 | Present |
| V12 ActionNode lists | d7942f11 | Present |
| V13 dynamic list ownership | 7c7b316b | Present |
| V14 queue lifecycle | 4481b9bc | Present |
| V15 temporary name copies | 5ee66830 | Present |
| V16 A/B/C | 551d8de1 | Identical to the V16-only branch |
| V1 Travel Form / StopFollow | 2efc7d82 | Present; missing MoveSpline include |
| V2 rest / consumable preflight | 4163c4cf | Present; missing MoveSpline include |
| V3 location preflight | a81a97da | Present |
| V4 pet-learning preflight | beaffa2f | Present |
| V5 item readiness preflight | 2d781333 | Present |
| pre-V0 loot open | d49c4257 | Stopped only active objects; incomplete |
| pre-V0 PMO / action-name move | cc9a2b50 | Present |
| pre-V0 route minimum / pathguard | 7cc8539b | Route minimum present; pathguard was in unreachable code |

## Completed source ports and corrections

* V1/V2 now include `MoveSpline.h` explicitly.
* V9 uses `NamedObjectCache`, a small ordered AVL store with three-way
  `std::string::compare` lookup that terminates on equality. This is a source
  implementation of the binary optimization, not a dependency on MSVC private
  `std::map` internals. Cached nulls, exact byte keys, the V8 key snapshot,
  context order, object ownership, stable node addresses and lexical
  Update/Reset traversal remain. There is no extra time-based cache.
* V11-V13 call the dynamic callback before cloning the template explicitly,
  preserving the binary baseline's sequence independently of compiler argument
  evaluation order. Allocated empty arrays remain distinct from null pointers.
* Loot opening stops movement before reading any nearby GameObject's state,
  preserving the pre-V0/V0 helper sequence for inactive as well as active objects.
* The cross-map guard now lives in the active `ResolveMovePath`, after a failed
  graph route and movement-event clearing. The earlier copy sat after `MoveTo`
  unconditionally returned through `MoveTo2` and had no effect.
* Missing pre-V0 chatguard, near-teleport ACK counter, failed SpellId 0 lookup
  fast rejection and unique `.rndbot diff` display are restored. The count uses
  the holder's synchronized map size and the original `active - total + unique`
  formula; it does not change population/activity bookkeeping.
* V16 files are retained unchanged: resolved group members per assist pass,
  one targeting count per candidate, and a quest cache shared only during one
  `Calculate`. The existing `newdistance =+ ...` expression remains unchanged.

## Validation and reproducible Windows build

The standalone tests include the actual production context/cache headers.
They compare 400,000 mixed operations against `std::map`, 20,000 ordered
inserts, erase/transplant and node stability, embedded NUL/high-byte keys,
cached null fallback, qualification, reentrant factories, callback insertion
during ordered traversal, and object destruction. AddressSanitizer is optional.

```powershell
cmake -S tests/playerbots -B build-tests -G "Visual Studio 17 2022" -A x64
cmake --build build-tests --config RelWithDebInfo
ctest --test-dir build-tests -C RelWithDebInfo --output-on-failure
```

Prepare the bundled Windows dependencies and cURL 8.10.1 at commit
`7eb8c048470ed2cc14dca75be9c1cdae7ac8498b`, as in the checked-in workflow, then:

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -DBUILD_EXTRACTORS=0 -DENABLE_MAILSENDER=1 -DBUILD_WARNINGS_AS_ERROR=0 -DBUILD_PLAYERBOTS=1 -DSUPPORTED_CLIENT_BUILD=5875
cmake --build build --config RelWithDebInfo --target mangosd --parallel 3
```

The cumulative branch's workflow now runs Windows MSVC only, runs the source
tests, and publishes the EXE/PDB, DLLs and build identity/hash manifest.
The independent V16-only branch is preserved.

## Remaining acceptance boundary

The source baseline is still a candidate. Neither synthetic tests nor an MSVC
build prove that every unrelated repack function matches the original executable.
The AVL implementation also needs real workload validation; no server-diff or
bot-population performance gain is claimed from these tests.

The built cumulative executable is a test candidate. An isolated copy of the
world/database and real gameplay must validate follow/stay, combat, corpse/GO
loot, druid rest/form use, item preflights, near teleports and cross-map movement,
followed by a comparable V15/V16 profile with unchanged settings. Preparing and
building this branch does not authorize replacing or launching the live server.
