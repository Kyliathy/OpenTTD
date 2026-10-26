# Native lifecycle continuation evidence

Owner: [Space fixes plan](r2sff1-space-fixes-features-1.md), Group31E.
Architecture: [GUI invariants](../../architecture/gui/archi-gui.md#invariants).

The observer starts from an actual earlier `policy-before-departure.sav` with
the original paid heavy miner, four real ore units, incomplete work and its
held Full Load queue. That predecessor explicitly declared last-safe attachment
geometry. This pass assigns no geometry, velocity, elapsed time, renewal timer,
credit, reserve, packet, allowance or service progress. Ordinary opportunities
must earn detachment, centre exit, the complete newly armed interval and birth.

Five phase saves record before detachment, after departure, after actual exit
credit, one opportunity before expiry, and after successful native birth.
`policy-next.tsv` records each phase's actual next thirteen synchronized owner
observations. `lifecycle-opportunities.tsv` records every completed opportunity
for independent visible/hidden comparison. Each process remains bounded by the
existing110-second harness; the observer also limits itself to5000 opportunities.

Independent replay and no-op transport use separate current applications.
The former compares actual next decisions; the latter preserves complete CPP,
mission/queue and ASTR records, including both complete private RNG streams.

The production null driver's `MainLoop` deliberately bypasses speed scheduling.
The cadence observer therefore inherits its platform stubs while directly
exercising the original shared `VideoDriver::Tick`, `GetGameInterval` and
`GameLoop`. It defers drawing and retains the existing selected application
driver. Actual `ChangeGameSpeed` selects100% or1000%; measured wall-time windows
must show more completed opportunities at fast-forward. Each driver callback
admits at most one field opportunity and decrements an armed timer once. Actual
paused callbacks preserve the thirteen synchronized owners. No replacement
timing algorithm is implemented in the observer.

## Verified results

Both configurations completed1596 ordinary opportunities, naturally armed a
1446-tick interval after the original Large body's exit, and published exactly
one new monotonic body ID. Each visible and hidden trace passed107904 assertions.
The original four real units survived safety departure unchanged; subsequent
extraction, navigation, receiving work and paid counters remained under their
original native owners. No world/time/permit edits were made in this observer.

| Configuration | Immutable predecessor | Visible / hidden run identities |
| --- | --- | --- |
| RelWithDebInfo | `role-heavy-preserved-release/policy-before-departure.sav` | `lifecycle-earned-visible-release`, `lifecycle-earned-hidden-release` |
| Debug | `policy-module-route-debug/policy-before-departure.sav` | `lifecycle-earned-visible-debug`, `lifecycle-earned-hidden-debug` |

All run directories are under `test-cache/space-fixes/`. In each configuration,
both TSV files match byte for byte between views: every thirteen-owner outcome
on all1596 opportunities, and all five independent next-decision oracles. Each
corresponding phase also matches complete CPP/mission/typed queue/ASTR transport.

Independent current application next-opportunity replay and a separate current
no-op application passed for every phase in both configurations. Each next-step
application passed63 assertions. Receipts are
`logs/space-fixes/lifecycle-phase-replay-final-release-transport.json` and
`lifecycle-phase-replay-debug-transport.json`; their `view_comparison` contains
the complete paired-view evidence. Reproduce with
`space_policy_replay_checks.py --lifecycle --source <visible-run> --paired-view <hidden-run>`
and an unused `--run` identity, adding `--configuration Debug` when appropriate.

The real scheduler checks passed in `lifecycle-speed-dispatched-release`
(350 assertions) and `lifecycle-speed-dispatched-debug` (346 assertions).
Normal600ms windows completed22 opportunities in each configuration; actual
1000% fast-forward windows completed41 and39 respectively. These measured ratios
are workload dependent, not a claim of guaranteed tenfold throughput. Paused
callbacks completed zero opportunities and preserved all thirteen owners.
Resumed120ms windows completed7 and9 opportunities, with every callback checked
for at most one opportunity and exactly one corresponding timer decrement.

Final focused builds are `build-lifecycle-dispatched-{release,debug}.log` in
`logs/space-fixes/`. Product code was unchanged in31E. An initial build corrected
the test setting's explicit uint16_t type. The first scheduler run reentered
`SwitchToMode` because this console callback executes before the outer load
dispatcher clears its request. The corrected observer temporarily marks that
already dispatched request `None`; no source reload or duplicate callback is
allowed during the measured native `GameLoop` calls. The failed
`lifecycle-speed-release` log is diagnostic only. The earlier phase/replay runs
were unaffected. Whitespace validation passed in `lifecycle-final-whitespace.log`.

Reading navigation correction: `AdvanceAsteroidRenewalTimer` and
`FinishAsteroidRenewalAttempt` are in `asteroid.cpp`; `asteroid_renewal.cpp`
contains candidate entry planning. Exported symbols remain authoritative.
