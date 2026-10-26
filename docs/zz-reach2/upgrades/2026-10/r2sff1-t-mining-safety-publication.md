# Native mining safety publication

Owner: [Space fixes plan](r2sff1-space-fixes-features-1.md), Group25B.
Prerequisite: [saved legacy departure](r2sff1-s-legacy-departure.md).

`space_mining_departure.h/.cpp` gathers exact original native participants in
VehicleID order. `PlanSpaceBodyDeparture` checks an actual leased attachment's
release lead or an actual approach's next moved body and remaining interception
budget. Stopped craft participates. `BeginSpaceBodyDeparture` revalidates craft
generation, body, reason, current pose, due timing and complete local proposal
before the existing original mission owner releases its lease/partial work.
It moves nothing, starts nothing, charges nothing and preserves original cargo.
Detached/completed/unrelated hulls and actual retained static service are excluded.

`ReleaseUnsafeSpaceMiningAttachments` replans each participant after the prior
commit. The common staging owner now reads existing contained departure points;
there is no new reservation registry. Each normal local movement opportunity
rechecks the endpoint's current original occupancy as well as its swept static
segment. A stale/occupied point retains the same mission's bounded named wait.
Group25's removed-source and explicit Recall/cancel checks remain applicable.

**The helper is prepared but has no normal-loop caller.** Group29 must call it
before body translation and `UpdateAttachedSpaceCraftTransforms`, using the
sole ordinary simulation opportunity. Group26 still supplies native survey and
ownerless preview release; Group28 still owns actual atomic body erasure/credit.
No new body event or movement loop is enabled by this producer gate.

RelWithDebInfo and Debug each pass **29,775 assertions in30 cases** with
`[space-mining-departure],[space-mining-continuation],[space-mining-wire],`
`[space-departure-plan],[space-attachment-safety] --warn NoTests`.
Logs: `logs/space-fixes/mining-safety-gate-final-{release|debug}.log`.
Eight new native component cases cover actual pursuit/extraction, partial work,
both configured roles, stopped/running publication, final safe transform,
crowded endpoints, stale incarnation/body/reason, sequential competitors,
actual full-hold completion, dynamic occupancy and a real retained service job.
Cost assertions prove only subsequent original Tick earns one idle opportunity;
clock, complete field renewal/RNG and four global RNG words stay unchanged by
publication. Repeated calls release nothing and earn no final free extraction.

Test geometry explicitly projects only an original body's straight trajectory;
the actual matching lease transform supplies the hull position. These are
declared geometry components, not executed elapsed travel, terminal events or
paid procurement. A final-edge input must declare evolved field geometry, not
claim pristine birth placement. Initial fixtures, blockers and declared exhausted
service allowance retain their existing component provenance.

Current444 stopped-departure full-load/preview/resave also passes in the actual
application: `logs/space-fixes/safety-gate-current-release.log` (127 assertions).
Rebuildable output: `test-cache/space-fixes/safety-gate-current-release/cargo-origins-current.sav`.
Use the ordinary `space_fixes_checks.py --cargo-origin` harness with the Group25
saved stopped departure to repeat this transport spot check; it claims no live
unsafe-step activation.

Build configurations sequentially with target `openttd_space_fixes_test`.
Both the ordinary and focused test source lists now register the new test file.
Build logs: `logs/space-fixes/build-mining-safety-gate-final-{release|debug}.log`.

| Configuration | Focused executable SHA256 |
| --- | --- |
| RelWithDebInfo | `98ce019b452339a2d8341c1221a9020dbb6a65146f6a37c1542fa6ba04f02af9` |
| Debug | `c510f7a8c432b0d3eb4dae2a469e86a28ade535a413ed2b5e56a75cb72659771` |
