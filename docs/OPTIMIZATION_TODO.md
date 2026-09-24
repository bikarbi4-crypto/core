# Optimization baseline and pending validation

Accepted source/development baseline (2026-09-24): V18,
`fbab350b6cde7a40735d778986382807d6287048`, branch
`vmangos-v18-quest-hot-values`. Cumulative V0-V18, including V17 A/B/C,
remain enabled. This is a development-baseline decision; the repository's
GitHub default branch is not changed by this release.

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
- [ ] V18 user gameplay validation with the published Windows VS2022 x64
  RelWithDebInfo / PlayerBots / client 5875 package. Final CI status belongs
  to the exact-HEAD delivery manifest. Standalone microbenchmarks do not
  establish an overall server speedup.

Existing unrelated Mema, AddCooldown, UseItem residuals and other open runtime
investigations remain pending; V18 does not close or modify them.

- [x] V19 PMO OFF guards, nested/toggle cleanup and late map registration,
  with standalone source contracts, MSVC ASan and same-work microbenchmarks.
- [ ] V19 real-server validation: comparable OFF windows and ON windows;
  check metric names, nested duplicate scopes and runtime toggles on existing
  instances. Microbenchmark times measure instrumentation, not server diff.
- [ ] Separate PMO concurrency audit: the existing configuration bool and
  per-metric counters/report/reset accesses are not made generally thread-safe
  by V19. The new mutex protects map-registry registration/traversal only.
