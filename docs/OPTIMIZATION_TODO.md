# Optimization baseline and pending validation

Player-presence investigation (2026-09-25): separate diagnostic branch from
V20 `169cb7712c8b704a3fe274cbf42a0cbb53fcb90a`. No policy/index replacement
and no assistant-run gameplay. See `V20_PLAYER_PRESENCE.md` for the explicit
runtime boundary and the user-run experiment. Historical release status below
is retained; no V0-V20 change is reverted by this diagnostic build.

- [ ] User-run no-player -> stationary player -> no-player experiment, PMO OFF;
  compare priorities, targets/A, scan cost, client/server CPU and focus separately.
- [ ] `pet_spell.PRIMARY`: five conflicts in the handoff's earlier V20 session;
  that log's SHA-256 differs from the currently accessible Server.log. No DB edit,
  suppression or causal connection to the presence drop is established.
- [ ] `Item::AddToUpdateQueueOf`: separate lifetime/update investigation; untouched.
- [ ] AddCooldown / UseItem residuals: separate investigation; untouched.

Accepted source/development baseline (2026-09-25 handoff): V19,
`33faa8e3c091d73483b42dc07f8f2934ba5f7958`, branch
`vmangos-v19-pmo-off-fast-path`. Cumulative V0-V19, including V17 A/B/C,
remain enabled. This is a development-baseline decision; the repository's
GitHub default branch is not changed by this release.

V20 is the next validation build on `vmangos-v20-value-allocation`, not an
automatic installation or a claim of completed V20 gameplay validation.
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
investigations remain pending; V20 does not close or modify them.

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
