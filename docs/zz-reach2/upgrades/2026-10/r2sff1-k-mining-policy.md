# Mining policy and continuation intent

Owner: [Space fixes plan](r2sff1-space-fixes-features-1.md), Groups 17-20 and
the Group 23-24 persistence gate.
Native boundaries: [runtime handoff](../../../zz-rhr/upgrades/2026-10/r2mlsp-z-z-p-probe-runtime-handoff.md#delivered-ownership).

## Group 17 implementation and exposure gate

Each `SpaceOrder` now has an explicit `mining_policy`, default `LegacyReturn`.
Only Mine may carry a nondefault choice. The existing `SpaceMission` contains
`SpaceMiningContinuationState`: explicit phase, pinned mineral, previous body,
optional effective target and departure point, retry ticks and named wait.
These fields create no packet, target registry, berth or service owner.

The new `CmdSetSpaceMiningPolicy` implementation is in `space_orders.cpp` and
declared in the existing Space command header. It intentionally has **no command
registration or UI path until Group 23** supplies its matching transport.
Existing command IDs and argument layouts are unchanged. Old insert/replace
calls produce LegacyReturn; an explicitly configured instruction uses the new
checked setter. Direct unit calls exercise the pending implementation without
claiming a savable modern gameplay world.

The command checks the exact native generation, ownership, deployed mining
role, current profile, crashed state, valid policy and existing Mine index.
Retained manifest or module service blocks changes even when paused. Ordinary
quotes and mixed Execute/QueryCost flags are pure. Editing an inactive/future
instruction changes only its policy; it cannot reset the current cursor or
physical work. Editing the current policy preserves live movement, work, leases,
approach and real service obligations. Waiting intents become eligible for an
explicit retry. A departure already in progress retains its safe point.

Configured assignments pass policy explicitly to `AssignSpaceMiningMission`.
The first admitted assignment pins the actual body's mineral even with an empty
hold; subsequent configured assignments refuse a different mineral. Opting an
already active legacy instruction into a configured policy pins its actual
mission body then. Old/manual callers retain the default LegacyReturn argument
and inert continuation fields until a configured action exists. No ordinary
writer produces unsaved new state before the command's persistence gate.

| Policy and condition | Next intent after safe release |
| --- | --- |
| LegacyReturn | Existing explicit route/home; no automatic destination substitution |
| Either configured policy, actual service obligation | NeedService; instruction and original service obligation survive |
| ReturnNearest, or a full hold | ReturnCargo to an admitted receiver |
| Partial FullLoadNearest | SeekMineral with the pinned label |
| No compatible destination/departure | Named Waiting state; original freight remains aboard |

`SelectSpaceMiningContinuationPhase` implements the pure policy table. Groups
18-20 supply destination admission and wait transitions; Groups 25/29 supply
actual safe release and lifecycle activation. `Holding` alone does not mean a
configured Mine completed.

Recall/manual mission replacement clears the associated intent only during
real execution. Existing queue clear/replace resets that queue's one current
intent; ordinary action completion clears it before advancing the cursor.
These paths preserve original cargo. Roles belong to the immutable commissioned
hull/Engine definition; there is no additional mutable role-change command to
create. New native hull construction gets default inert state, and unsupported
roles cannot configure mining intent.

The queue and vehicle semantic digests include every nondefault policy/intent
field with explicit markers. Existing legacy/default streams stay unchanged.
Group 23 must serialize and validate all new enums, optionals and retry bounds;
Group 24 joins targets/mineral/roles against candidate/native owners. Group 20
must handle policy changes during an existing departure/return/service phase
using the new preference without losing the current safe movement or mandatory
service. It must also replace the old Holding-completes-Mine and original-target
lookups in both tick and common approach gathering.

## Group 17 verification

`src/tests/space_mining_policy.cpp` covers the policy table, role/identity/owner
and service refusal, quote purity, empty-hold pinning, active legacy opt-in,
future-entry edits and real mined packet conservation through Recall/Clear/
Replace. Containers/service fields explicitly declared for admission tests
claim no paid service work. Extraction conservation earns freight with ordinary
native ticks.

The focused RelWithDebInfo build passed after adding the test's missing
`space_mining_tuning.h` include. The initial test assumed the small legacy mining
fixture supplied a survey Engine; it does not. Unsupported-role command
refusal was therefore checked against **four actual non-mining hulls in the
retained demo**, preserving their native definitions and all owner digests.
No unsupported role was manufactured inside the small mining fixture.

`[space-mining-policy]` passed **1,002 assertions in four cases**, under one
second. Evidence: `logs/space-fixes/space-mining-policy-final.log` and
`logs/space-fixes/build-space-mining-policy-native.log`.
The retained load/preview/waiting spot check passed **274 assertions in 4.219
seconds**, including those four pure role refusals. Evidence:
`logs/space-fixes/mining-policy-legacy.log` and
`test-cache/space-fixes/mining-policy-legacy/report.json`.
Executable SHA256:
`2f3cb8f8645a822ffc688d74243dc18e7dc4bfdf0cb4d0c344954bec3dc48041`.
The source save remains the unchanged `space-factory-440.sav` used in earlier
groups. Initial include/fixture diagnostics remain in
`logs/space-fixes/build-space-mining-policy.log` and
`logs/space-fixes/space-mining-policy.log`.

To reproduce, build `openttd_space_fixes_test`, run `[space-mining-policy]`, then
`space_fixes_checks.py --asteroid-preview --construction waiting --run <unique-name>`.
No full regression or paid financial trial was run.
