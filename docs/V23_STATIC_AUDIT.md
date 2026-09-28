# V23 pre-edit audit (2026-09-28)

Source baseline: V22 `24e11eb41b85ab74437dfc1737289c2bcca42de4`.
V21 and V22 checkpoints, the default branch, and the active installation remain
preserved. No server, game, proxy or database execution/connection is part of
this work. Local compilation uses two jobs, /MP1 and inherited CPU affinity.

The supplied V22 review was independently recalculated from the original
SHA-256 `4bb51538713545197932d5792e1bcb78b5972d3798f77ecec4e385a8d7231237`:
6528 lines, 70 diff samples, 14 CPU snapshots, 182 partition records, 1096 zone
records, five presence samples and all 20 listed item-failure bursts agree.
Late minus middle active is 262.4429429 at similar recorded mean diff. These
are overlapping snapshots of different workload within one run, not a paired
V21/V22 capacity benchmark. PMO and orderly shutdown are absent. Raw gameplay
logs and player identities are not part of the public validation package.

## A: AttackersValue

ValueContext registers `attackers` as AttackersValue and `possible targets` as
PossibleTargetsValue. Calculate calls AddTargetsOf for the owner, eligible group
members and sometimes the master. AddTargetsOf(Player*) formats two integer
strings, builds a temporary vector<string>, then MultiQualify builds another
stream/string before the same typed possible-targets lookup. The exact key is
`{range:1}`. Both existing to_string conversions and the float-to-int32 cast
must remain, at their original evaluation sites. The global Qualified parser
and formatter, PossibleTargetsValue and lookup policies are out of scope.

Candidate: concatenate those same already-converted strings with the same
literal punctuation. There are no world callbacks between the two conversions
and formatting. No key/result is cached across AddTargetsOf calls or ticks:
GetRange reads mutable configuration, and Value/world callbacks may reenter,
change configuration or qualifiers between members. Tests must exercise this.

The successful shareTargets path also allocates a vector for four fixed target
names. A local string array can preserve string element types, lifetime, names
and iteration order while removing only vector storage. Do not replace the
strings with pointers whose equality/overload resolution would differ.

Keep complete Calculate/AddTargetsOf/InCombat/IsValid/IgnoreTarget bodies in
parity tests, with declared world-service fixtures. Preserve pointer-set order,
GUIDs, duplicates on the shared-list path, validation/invalid-target sets,
getOne, group/master/pet/guardian/duel handling, typed lookup and refresh traces,
RNG observations, exceptions, reentry and PMO ON/OFF. Actual CalculatedValue
refresh policies should be compiled; fixture registry/world queries remain
explicit boundaries rather than claimed live gameplay coverage.

## B: Engine::LogAction

DoNextAction, ProcessTriggers, strategy and queue operations call LogAction.
The complete method formats a message, appends history, truncates, then either
writes test.log or obtains the bot/group/name and invokes sLog.Out. There is no
PMO guard: history maintenance is useful independently of the profiler.

For an appended string S with size >512, the old history is the suffix beginning
at the first `|` at or after offset 512, or empty if there is none. Searching S
directly from 512 can eliminate its first substr(512) allocation/copy while
retaining the final existing conditional substring assignment. This also keeps
the final string capacity/allocation policy on MSVC, unlike an in-place erase
which can retain a large history buffer after clearing. Measure retained
capacity and preserve negative/no-benefit cases before choosing a candidate.

Formatting, append order, exact bytes, embedded pipes, group/test logging,
callback order, exceptions and reentry must remain identical. Use complete
production LogAction with explicit logger/file/world service fixtures, not a
replacement trimming algorithm presented as the whole AI engine. Long formatted
test messages stay within the existing 1024-byte buffer's defined domain; this
release does not repair its pre-existing unchecked vsprintf or fopen failure
handling. Removal of an allocation necessarily removes its bad_alloc site;
identical failure on the Nth allocation is not an achievable parity contract.

## Validation and excluded work

Record an independent unchanged V22 baseline before production edits. Timing
executables contain neither trace logging nor global-new counters. Count
allocations/bytes separately, alternate variant order and keep all raw rounds.
Run normal/ASan, then cumulative V17-V22 suites and a Windows-only full build.
Actual source hashes and the exact changed-body boundary accompany the tests.

Do not change activity/PID, frequency/delay/relevance/iterations, population,
radii, target scoring, pathfinding, presence, PMO scopes or disabled behavior,
configuration/DB, Mema, cooldown/use-item/item-queue code, raid or partitions.
Item retry bursts, PMO aggregation and Teldrassil/Darnassus scheduling remain
separate investigations. Existing activity_pid.csv collects additional bot
level/gold/gearscore data and will not be enabled for this source audit.
