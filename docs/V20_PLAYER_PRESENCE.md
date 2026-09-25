# V20 player-presence diagnostic build

Base: `169cb7712c8b704a3fe274cbf42a0cbb53fcb90a` (V20).
Branch: `vmangos-v20-player-presence-diagnostics`.
This is an OFF-by-default observation build, not V21 or a performance claim.
All cumulative V0-V20 behavior, V17 A/B/C and V19/V20 PMO fixes remain.

## Runtime boundary

The assistant must not start mangosd (including `--version`), WoW, MySQL,
the live server, log in, perform the experiment, change live configuration,
or install this build. Windows CI compiles mangosd and executes standalone
tests only. Runtime smoke is explicitly NOT RUN. The user installs/runs the
diagnostic EXE manually and returns logs. Preserve the original V20 EXE/PDB
and existing DLL/configuration files for manual rollback.

## What the source establishes

For background bots, a nonempty policy registry replaces the
`IN_EMPTY_SERVER` bracket {50,100} with friend/guild or map/zone checks.
`IN_ACTIVE_AREA` remains {50,100}; `IN_ACTIVE_MAP` is {70,100};
`IN_INACTIVE_MAP` is {80,100}. This can reduce allowed activity at identical
global A and identical diff targets. It does not predict whole-world active
counts: other priorities, fixed-number discretization, cache and workload
still matter. The reported active counter is not CPU utilization.

HasPlayers() tests the manager registry, which can also contain non-random
bots through OnBotLoginRegistration/MovePlayerBot. Its size is not a strict
human-player count. The map observer uses the actual priority predicate and
reports a separate real-player count. No extra human filter is inserted into
the existing policy; doing that would change behavior.

The nonempty-server path copies GetPlayersSnapshot, checks friends, and may
scan the full map player list, including bots. Its real-world cost is not
established until the user returns diagnostic logs. No policy/threshold/PID,
AI frequency, radius, population, pathfinding or selection behavior changed.

`Map::HaveRealPlayers()` itself loops over the player list and tests `!IsBot()`.
It is not a maintained counter. GetPriorityType instead requires IsInWorld
and accepts no AI or an AI for which IsRealPlayer() is true. These are not
interchangeable definitions (selfbots, pending logout/transfer in particular).
GM/invisibility is not filtered by this particular priority map scan; other
near-player checks have their own rules. No map index replacement is included.

## Manual experiment (user only)

1. Keep existing population/configuration. Use the same local server console
   for every phase. Start with PMO already OFF. Do not use numeric `rndbot diff`,
   pid/reset/reload/update commands to measure: these can mutate behavior.
2. In the local console enter `rndbot presence start 1800 timing` for a bounded
   30-minute window with 1-in-1024 priority timing; omit `timing` for counters
   alone. Valid duration is 30..3600 seconds. If PMO is ON the command refuses
   to start; it does not modify PMO. If PMO becomes ON, collection stops.
3. Observe no player for 5-10 minutes or until stable; then one stationary
   character for 5-10 minutes, with no raid/teleport; then log out and leave
   the client at character selection for 5-10 minutes or until stable.
   Keep the foreground/background arrangement consistent and record changes.
4. During each phase use `rndbot diff` and `rndbot cpu` without arguments from
   the same console. Mark actual login/logout times. `rndbot presence status`
   is read-only; `rndbot presence stop` ends collection early. Expiry is checked
   by the ordinary world tick (a stalled world cannot print an on-time report).
5. Save the whole Server.log for this single session before it is overwritten.
   Return it with actual EXE path/hash, launch working directory, config
   path/hash, per-process mangosd/WoW CPU and affinity/core count, client FPS
   and focus observations. Do not send credentials or the complete config.
   No assistant-side process launch or settings change is needed.

The log prints only the 15 requested configuration keys: current effective
fields, the loaded Config object's parsed value/default, key presence and
whether runtime differs. It resolves the config filename at report time;
normal Windows launch uses cwd, Windows service startup selects EXE directory.
It does not claim an on-disk file hash proves what an older run loaded.
`rndbot diff` numeric arguments can override DiffWithPlayer/DiffEmpty;
reload can reload configuration; PMO commands can toggle PerfMonEnabled.
The generic GetValue/SetValue interface does not expose these 15 fields.

## Reading the telemetry

`[PRESENCE]` aggregate batches appear every 30 seconds. No player names,
account IDs, character GUIDs, chat, addresses or credentials are printed.
GUIDs are used only inside bounded unique-observation sets.

- `world`: policy registry entry count, active Sessions, measured diff10/diff60,
  current global A. Registry size and in-world map real count are different
  populations; do not equate them during login/logout.
  `map_snapshot_real_sum` sums published per-map real counts; sample times
  differ and transfers can temporarily duplicate a player. It is not an
  instantaneous deduplicated world population. Use a stationary stable phase.
- `global_scale` / `local_scale`: last actual regulator update before/after A,
  wanted/current milliseconds, sample count and age. No regulator is called
  for diagnostics. No local_scale record means none was observed this run.
- `map` / `player_zone`: sampled inside the existing map-owner player loop,
  at most once per map per worker per 5 seconds; no extra map traversal.
  Both real-player definitions are reported separately. Map/zone is numeric.
  `wanted_if_local_enabled` is a configured candidate target, not evidence
  that local scaling ran. `source_at_snapshot` describes that sampled map;
  `activity_sources` counts actual getActivityPercentage return paths.
- Source IDs: 0 global (local scaling disabled), 1 fallback (bot not in world),
  2 fallback (no map), 3 fallback (not partitioned continent), 4 fallback (no
  samples), 5 fallback (uninitialized local A), 6 local A. Snapshot source
  can lag the actual selection; use record ages and actual source counters.
- `priority`: cumulative return counts and allow/deny decisions over all
  activity types, ALL_ACTIVITY separately, and unique latest observed ALL
  category. Calls are not online bots. `unique_observed_bots` also includes
  bots seen only for other activity types; these can have unknown ALL category.
- Priority IDs match ActivePiorityType: 0 selfbot, 1 real master/disabled
  priorities, 2 real-player group, 3 battleground/teleport, 4 instance,
  5 visible, 6 always active, 7 combat, 8 BG queue, 9 LFG, 10 nearby,
  11 friend, 12 guild, 13 NO_PATH, 14 active area, 15 active map,
  16 inactive map, 17 empty server.
- `all_transition`: observed category changes at actual ALL decisions;
  movement between unobserved decisions is not inferred. Per-AI scalar state
  follows the same ownership as the existing cached allow state; sequential
  owner transfer retains it. This does not repair unrelated concurrent AI use.
- `all_activity_cache`: cache hits versus ordinary/forced refreshes and
  allow/deny call counts. Last ordinary refresh Unix min/max is taken from
  the latest unique observations; unknown is explicit. Initial cache timer
  initialization and checkNow=true are not called ordinary refreshes.
  These count AllowActivity calls; direct IsActivityAllowedCached getters in
  Map update scheduling are not included.
- `cost`: only the GetPriorityType snapshot/friend/map-scan path, not every
  GetPlayersSnapshot user in the repository. Optional priority nanoseconds
  include in-function instrumentation; empty clock pair cost is measured
  separately. No extra RNG/priority/AllowActivity calls are made.

Counters are cumulative *published* snapshots, not exact whole-world tick
totals. Each owner publishes at most once per 5 seconds, checked on map/world
owner ticks or every 256 observations. Quiet/exited threads can leave an
unpublished tail; stop does not force unsafe access to their mutable buffers.
Publication ages, registered/published thread counts and dropped observations
are explicit. Latest unique states are deduplicated by GUID and scalar
lifetime/revision; maps are deduplicated by latest observation timestamp.
Thread/map transfer may briefly retain an older map snapshot, so use ages.
Do not subtract cumulative counts across different epoch IDs.

Limits per run: 64 registered threads, 16384 bot keys and 128 map keys per
thread, 32 zones per map sample, maximum one hour. Caps discard observations,
not gameplay work, and report drops. These limits bound memory; report merging
can temporarily hold the sum of bounded thread snapshots. No raw Map/Player
pointer is retained by or passed to the cross-thread collector. TLS owns the
mutable data; rare publication/report copies use mutexes. There is no global
mutex or global atomic increment on every hot call. A bot-lifetime ID is
allocated once per observed AI per run; enabling is an atomic read.

OFF performs no diagnostic scans, allocations, snapshot copies or clock reads.
It still adds branches/atomic reads and is not promised to be instruction-free.
ON pays bounded map sampling, thread-local bookkeeping, publication and log
output costs. `publication_ns` excludes the last publication's cost (visible
in the next copy); `report_cost` excludes formatting that final line and log
I/O. Standalone benchmarks are observer-overhead measurements, not a live
performance gain or a quantitative explanation of the user's active drop.

## Validation

`tests/playerbots/presence` compiles exact V20/current GetPriorityType,
GetPriorityBracket, AllowActive, AllowActivity and getActivityPercentage bodies
against identical fixtures. Cases cover priority exits, activity types,
empty/real/bot/selfbot/null/out-of-world map entries, first/last matching zone,
threshold boundaries, botActiveAlone, force-visible/combat flags, timer/cache
and forced refresh. OFF/ON/timing compare results, RNG, ordered external calls
and cache fields. Collector tests cover OFF allocation/clock absence, multiple
threads, unique counts, transitions, ordinary timestamps, expiry, PMO stop,
storage limits and restart. MSVC ASan and the cumulative suites are retained.
No full-world functional test has been performed by the assistant.
