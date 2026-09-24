# Optimization baseline and pending validation

Accepted source/development baseline (2026-09-24): V17.1,
`2ea64f9d27563ac410bacea79de6e2e2671b01c3`, branch
`vmangos-v17-1-value-rtti`. V17 A/B/C remain enabled.

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
