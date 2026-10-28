# Original role selection and small-miner continuation

Owner: [Space fixes plan](r2sff1-space-fixes-features-1.md),31B.1-31B.2.
Full holds and implicit Surface descent are recorded separately in
[full-hold evidence](r2sff1-z-f-full-holds.md) and
[Surface/descent evidence](r2sff1-z-g-surface-return.md). This extends
[heavy continuation](r2sff1-z-b-policy-continuation.md).

The native observer now selects original15 explicitly with `edit-light`,
`continue-light` and `replay-light`. Existing heavy modes retain their default21.
Selection resolves an owned deployed original incarnation; no engine, model,
manifest, cargo or generation is assigned. Actual input retains all work/cargo/
pose/cost/clock/RNG checks, including disabled/stale-generation cases. Both
configurations compile (`build-role-slow-source-{release|debug}.log`).

Both current applications pass the following actual inputs:

| Immutable source / role | Native outcome per configuration | Assertions |
| --- | --- | --- |
| `space-missions-440.sav`,15 |38 ordinary opportunities establish the actual partial hold/work; three native policy choices preserve original owners |415|
| `space-heavy-440.sav`,15 |445 ordinary opportunities establish the original15's actual partial holding; source already includes a real Core receiver |2,860|

Run names are `role-input-light-{release|debug}` and
`role-input-light-module-{release|debug}`. The mission source has no orbital
module; it remains a useful original paid Surface-route input. The module case
uses the existing actual heavy-demo world, selecting its original15 rather than
fabricating a receiver or changing the heavy's model. Current heavy regression
`role-heavy-preserved-release` also passes7,485 assertions and1,224 opportunities
through original21 handling after role parameterization.

Small-miner partial continuation now passes **10,213 assertions and1,679 actual
ordinary opportunities** in each configuration. Its original first body7 has a
slow diagonal heading; body6 is its actual same-mineral second source. The real
Core0/berth0 receives both complete immutable Source quantities through native
handling. Original15 retains its original module allowance semantics: no heavy
return charge and no invented manifest. Its real20-unit cargo capacity is
preserved after transfer. Both original centres later retire through actual
ordinary motion; stored cargo retains their historical provenance.

Five separately saved phases replay their actual next opportunity in current
applications, matching all13 owner observations. Another independent native
load/preview/no-op save per phase preserves complete CPP rows, original mission/
stop/continuation/queue and the entire ASTR lifecycle/RNG trailer. All five
next-step and all five transport applications pass in both configurations;
each next-step observer passes63 assertions. Reviewed actual return capture
shows the original15 returning4/20 Nickel Ore to orbital facility#1, with its
existing allowance unchanged and all controls visible.

Reproduce with fresh run names:

```powershell
python -B src/tests/multilayer/acceptance/space_fixes_checks.py --save space-heavy-440.sav --mining-controls edit-light --run role-input-light-module-release
python -B src/tests/multilayer/acceptance/space_fixes_checks.py --source test-cache/space-fixes/role-input-light-module-release/policy-current.sav --mining-controls continue-light --run role-light-partial-release
python -B src/tests/multilayer/acceptance/space_policy_replay_checks.py --source test-cache/space-fixes/role-light-partial-release --role 15 --run role-light-replay-release
```

Debug adds `--configuration Debug` and uses the corresponding `-debug` source/run
names. Reports bind actual source/config/executable hashes. Native/driver logs
live under `logs/space-fixes/<run>{|-driver}.log`; every runtime/phase save/original
PPM capture lives under `test-cache/space-fixes/<run>/`. Transport receipts are
`role-light-replay-{release|debug}-transport.json` under the log directory.
Lossless PNG viewing conversion follows the earlier policy evidence pattern.

The first small-miner trace failed an observer precondition because it waited
for the slow first centre to retire before selecting partial return. That
legitimately filled the smaller hold. The corrected observer earns a second
source batch, tests partial ReturnNearest, then observes both actual later
retirements. It changes no velocity, reserve, holding, paid owner or time to
force retirement. Keep `role-light-continuation-release.log` and its source
captures as diagnostic history. The passing trace is `role-light-partial-*`.
Scoped whitespace check passes in `role-partial-whitespace.log`.
