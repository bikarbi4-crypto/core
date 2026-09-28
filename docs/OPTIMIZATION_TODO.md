# Optimization baseline and pending validation

Accepted development baseline: V22 `24e11eb41b85ab74437dfc1737289c2bcca42de4`,
branch `vmangos-v22-pmo-hotpaths`. Cumulative V0–V22 remains enabled.
V21 `6a32bb9c97d45166719a9b5a9d3e0f0c0c8ac4c7` remains preserved.
V20 `169cb7712c8b704a3fe274cbf42a0cbb53fcb90a` remains a fallback.
V23 `vmangos-v23-attackers-logaction` is the next validation release from V22.
Diagnostic `4b9d62de1c5dbe1398f5067596fa9111f00e2fdd` is reference only.
The default branch and live installation are not changed by this release.

Preserved V18 fallback: `fbab350b6cde7a40735d778986382807d6287048`, branch
`vmangos-v18-quest-hot-values`.

Preserved V17.1 reference: `2ea64f9d27563ac410bacea79de6e2e2671b01c3`,
branch `vmangos-v17-1-value-rtti`.

Preserved fallback: V16 `cea7ba955f5c0d57ff76d63e53f97f34b1d3ab4c`,
branch `vmangos-v16-cumulative`. Historical reports describe the baseline
accepted when they were written; this decision supersedes them for development.

- [ ] **40-man raid performance validation** (does not block further optimization).
  When the user actually assembles the raid, compare boss single-target and
  trash with roughly 5-10 available targets. Record raid-instance time,
  reaction, target switching, healing, spikes and overall diff. Keep population,
  AI settings and observation windows comparable; separate PMO windows.
- [x] V18 source parity and same-work microbenchmarks, including negative
  samples and MSVC AddressSanitizer checks.
- [x] Prior V18 gameplay session retained as historical context in the V19
  handoff. The V18 and V19 samples are not paired equivalent-world experiments;
  their difference is not an overall optimization percentage.

Existing unrelated Mema, AddCooldown, UseItem residuals and other open runtime
investigations remain pending; V23 does not close or modify them.

- [x] V19 PMO OFF guards, nested/toggle cleanup and late map registration,
  with standalone source contracts, MSVC ASan and same-work microbenchmarks.
- [x] V19 initial user gameplay session (2026-09-25): five exact input hashes
  and 17 groups of handoff claims independently reproduced. Normal shutdown,
  one 26-second PMO window. This does not establish every gameplay regression,
  memory-leak absence, paired performance or the 40-man test.
- [ ] V20 gameplay comparison against this V19, separating PMO OFF, brief ON,
  report printing and shutdown. Check normal quest/perception/target behavior,
  cold shared values while ON, toggles, spikes and corrected report maximum.
- [ ] Separate PMO concurrency audit: the existing configuration bool and
  per-metric counters/report/reset accesses are not made generally thread-safe
  by V19. The new mutex protects map-registry registration/traversal only.
- [x] V20 shared-value AI lifetime: actual-constructor V19 ASan reproducer,
  owned botless AI/registry, ordered destruction, locked lookup/factory cache.
- [x] V20 unused CheckValuesAction list copies and local combat-target node
  copies: actual-body parity, allocation checks, normal and ASan tests.
- [x] V20 PMO report maximum aggregation correctness, isolated from hot paths.
- [ ] Shared-value concurrency follow-up: the unchanged SingleThreaded singleton
  first creation/destruction, simultaneous cold Get, and concurrent Get/Set/Reset
  are not made generally safe by the owner lookup mutex. Normal auto-quest startup
  initializes the singleton before world updates; not every configuration does.
- [ ] Shared raw-map ownership follow-up: ItemDropMapValue/VendorMapValue lack
  deleting destructors; pointer-valued Reset/recalculate may replace old storage.
  Do not broaden V20's context fix into a generic pointer-storage redesign.
- [ ] PMO report zero-denominator presentation (existing NaN/Inf) separately
  from the corrected maxTime aggregation; retain counts and timing scope.
- [ ] Numeric qualified-key formatter reserve: preserve C++ locale, exact bytes,
  signed int32 limits, overload resolution and manual qualifiers before considering
  an alternative formatter. No new inter-tick cache is authorized by this item.

- [x] V21 user A/B/C/D presence validation: empty, human in-world, logout
  to character screen, full disconnect. Accepted by the user for V22.
- [ ] Extended presence scenarios: reconnect/transfer/GM/selfbot.
  Record `rndbot presence`, `rndbot cpu`, `rndbot diff`, phase times and logs.
- [ ] Audit unused legacy `GetRandomPlayer` numeric map indexing under read lock.
- [ ] Unused `HasManyPlayersNearby`: review its legacy squared/rounded radius
  contract separately before adding consumers or changing semantics.
- [ ] Existing async login manager's mixed raw-Player snapshot lifetime and social
  consumers' naming/definitions: separate population/social work, not V21 activity.

- [ ] V22 user-only runtime comparison, same population/config/route/quests,
  PMO OFF steady samples and separate short ON windows. Confirm combat, quest,
  travel, target switching and V21 presence; no whole-server percentage from
  standalone microbenchmarks.
- [x] V22 follow-up uptime review independently recalculated on 2026-09-28:
  5881 seconds, 70 diff samples, 14 CPU snapshots, 182 partition rows and 1096
  zone rows. Late versus middle has +262.44 active at similar mean diff within
  one changing workload. It is not a paired V21/V22 gain. No PMO tables or
  orderly shutdown in this file; no conclusion about the reason logging ended.
- [ ] Separate partition experiment after matched runtime evidence. Latest V22
  snapshot: instance 11 has 665 bots / 510 active / 93.41 ms; instance 12 has
  78 / 63 / 17.27 ms; Teldrassil + Darnassus 308 / 230. V23 changes no partition.
- [ ] Separate item-failure retry audit: repeated AddCooldown/UseItem/item-queue
  bursts verified in the V22 log. No retry/error suppression is part of V23.
- [ ] V23 user-only smoke and paired V22/V23 comparison: combat, target changes,
  quest/travel, group/master/pet/guardians/duel, shareTargets, real-player presence,
  normal/group-only/test logging. Same configuration/population and matched uptime;
  PMO OFF steady window, then separate short ON/report window. Save full logs and
  phase times. Automated fixture tests do not replace this gameplay check.
