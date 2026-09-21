# V16 Target Selection Batch

Baseline: `b81cfd7e2bef1c5e693db64e52727ecbf7c09511`

This branch contains performance-only changes for `GrindTargetValue`:

- reuse resolved group-member pointers during a grind-target scan;
- compute `GetTargetingPlayerCount(unit)` once per candidate;
- reuse the local `needForQuest` cache across assist passes within one `Calculate()` call.

The cache does not persist between AI updates. Existing target-selection rules, priorities,
activity cadence, population settings, travel logic, Mema, cooldown behavior, and database
behavior are intentionally unchanged.

The existing `newdistance =+ ...` expression is intentionally preserved because changing
it would alter behavior rather than only remove redundant work.

CI validation trigger branch for GitHub Actions.

CI matrix retry with fail-fast disabled.
