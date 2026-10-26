# Native policy continuation and separate application replay

Owner: [Space fixes plan](r2sff1-space-fixes-features-1.md),30B.2-30B.3.
This extends [actual input evidence](r2sff1-z-a-policy-input.md).

Both configurations pass the actual21 scenario with7,483 assertions and1,224
ordinary `StateGameLoop` opportunities. Inputs are the preceding actual
`policy-input-heavy-fixed-release/policy-current.sav` and
`policy-input-heavy-debug/policy-current.sav` under `test-cache/space-fixes/`.
Their original paid hull, manifest, earned partial holding and incarnation are
retained. No source file is edited.

The retained heavy also carries actual Surface shells. Nearest policy respects
the original route's domain. The observer adds an explicit Unload instruction
for an existing operational owned module using `CmdEditSpaceOrder`; this
legitimate queue edit restarts incomplete work and preserves actual cargo and
pose. Subsequent ordinary opportunities earn attachment/work before the edge
preservation assertions begin. The selected actual receiver is Core0, berth0;
the inspector consistently presents it as orbital facility#1.

Only last-safe map-edge geometry is declared, preserving the original body's
signed heading/carry. Geometry projection is not elapsed travel. Native safety
then releases the real lease/work, preserves the held packets and original Mine,
travels to same-mineral body6 after body0, earns a genuine finite batch there,
and later handles the actual exits. Actual mouse input selects Nearest receiver.
The original continuation selects its nearest admitted owned Structure, travels
to its real berth and invokes original `CmdMoveOrbitalFreight`. Its sole native
stock receives both full immutable source quantities; the actual manifest
spends precisely one return allowance. No arrival, reserve, stock, cargo,
allowance, simulation clock or paid asset is assigned by the observer.

Native captures exposed a presentation defect: saved SeekMineral intent remains
after attachment, so the fleet still said Seeking during actual Mining. The
status now reads the actual Mining phase after acquisition. This is a draw-only
status correction, with genuine work and original continuation untouched.

Five actual phases are saved: before departure, after departure, second-source
attachment, receiver travel and after unload. Each records the uninterrupted
run's **next ordinary opportunity** in `policy-next.tsv`. Separate current
applications load each phase and match13 scalar observations: actual clock,
field, fleet, queue, structures, manifests, pods, descents, launch centers,
transitions, both global simulation RNG words and company balance. Field digest
includes both complete private renewal streams, credit, latest exit, cursor,
countdown, bodies and knowledge. Fleet/order digests include original typed
packet sources, mission work, physical pose, leases/effective destinations and
saved policy. Cosmetic interactive RNG is not a saved simulation prerequisite.

Another independent application per phase performs native load/preview/no-op
save. Existing raw wire readers verify **complete CPP rows**, every original
mission/stop/continuation/queue and the entire ASTR lifecycle. Natural historical
origins remain valid after bodies0/6 retire. Native elapsed packet bookkeeping
is compared only against uninterrupted gameplay, never against a no-op save.
Both configurations pass all five next-step and all five transport applications;
each next-step case now has63 assertions. Current format is445.

Rebuild `openttd_space_fixes_test`, sequentially per configuration. Final logs:
`logs/space-fixes/build-policy-final-{release|debug}.log`.
Actual continuation:

```powershell
python src/tests/multilayer/acceptance/space_fixes_checks.py --source test-cache/space-fixes/policy-input-heavy-fixed-release/policy-current.sav --mining-controls continue --run policy-status-continuation-release
python src/tests/multilayer/acceptance/space_fixes_checks.py --configuration Debug --source test-cache/space-fixes/policy-input-heavy-debug/policy-current.sav --mining-controls continue --run policy-final-continuation-debug
```

Replay and complete transport audit:

```powershell
python src/tests/multilayer/acceptance/space_policy_replay_checks.py --source test-cache/space-fixes/policy-status-continuation-release --run policy-final-release
python src/tests/multilayer/acceptance/space_policy_replay_checks.py --configuration Debug --source test-cache/space-fixes/policy-final-continuation-debug --run policy-final-debug
```

Harness run names must be fresh. Redirect driver stdout to a separate
`logs/space-fixes/<run>-driver.log`; the native harness owns `<run>.log`.
Native traces/captures/saves/reports are in
`test-cache/space-fixes/<run>/`; reports bind exact source, config and executable
SHA256. Machine-readable transport receipts are
`logs/space-fixes/policy-final-{release|debug}-transport.json`.

Every replay asserts actual window bounds, all three policy hit rectangles and
scroll reachability through the final row, then captures actual status/top/bottom
without changing the13 observations. Reviewed captures at1920x1027,200%, default
font show all controls, actual second-source Mining, receiver travel and
completed unload. Additional `policy-wrap-release` at1024x768,200%, font20 passes
and visually demonstrates wrapped rows and the reachable final fleet entry;
long labels ellipsize while original controls stay present. Rebuild that capture:

```powershell
python src/tests/multilayer/acceptance/space_fixes_checks.py --source test-cache/space-fixes/policy-status-continuation-release/policy-retarget.sav --mining-controls replay --resolution 1024x768 --font-size 20 --run policy-wrap-release
```

Native captures are original P6PPM pixels. Lossless viewing conversion only:

```powershell
python -c "from PIL import Image; Image.open('test-cache/space-fixes/policy-wrap-release/policy-replay-bottom.ppm').save('test-cache/space-fixes/policy-wrap-release/policy-replay-bottom.png')"
```

The initial continuation run's receiver expectation failed because the real
implicit route correctly used its carried Surface shells; that log remains
`policy-continuation-release.log`. The explicit-route passing runs establish
module custody without weakening native admission. Earlier compile-error logs
retain the observer corrections for original station next-hop grouping and a
Catch parenthesized assertion. CRLF-aware scoped whitespace check passes in
`policy-final-whitespace.log`. No financial trial or full regression ran.

Group31 still owns the broader two-role refusal/service/Recall matrix and
longer save/expiry/spawn acceptance. These scoped passes do not close it.
