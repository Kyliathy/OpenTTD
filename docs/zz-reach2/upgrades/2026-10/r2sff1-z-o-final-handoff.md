# Space fixes final handoff

Owner: [Space fixes plan](r2sff1-space-fixes-features-1.md), Group34.
Architecture: [GUI](../../architecture/gui/archi-gui.md),
[map layers](../../architecture/map-layers/archi-map-layers.md),
[vehicles](../../architecture/vehicles/archi-vehicles.md),
[game loop](../../architecture/game-loop/archi-game-loop.md),
[savegame](../../architecture/savegame/archi-savegame.md) and
[desync](../../architecture/desync/archi-desync.md).

This handover note can be deleted. The plan and its scoped evidence notes remain
the completion record; the generated evidence index can be rebuilt below.

The delivered fixes provide reachable construction pages, Alt+1..4 and repeat-F2
behavior, actionable native construction/mining waits, straight asteroid exits,
safe attachment release, order-owned nearest/full-load continuation and delayed
different-size replacement. Original cargo, paid work, shell, service, vehicle
and command owners remain authoritative. Current saves use version445; ASTR
lifecycle schema is1. Typed cargo origins441, lifecycle442, policies443,
unordered legacy departure444 and ownerless no-target Return445 have matching
native/preview readers. Genuine374 Classic and405/418/440 Space sources were tested without
regenerating bodies or paid assets. Earlier service/transport gates remain
recorded in the plan's respective evidence notes.

## Playable entries

From the repository root:

```powershell
.\play.ps1 space-mining
.\play.ps1 space-module-work -Config Debug
```

The first opens retained405 mining; the second opens the partly built418 module.
Both start paused. Alt+4 selects Space. Open Fund new industry to inspect the
five construction action pages. The [demo guide](d2sa-space-adventures.md)
explains paid prerequisites, Mine continuation controls and legitimate waits.
Old orders default to Follow route / home; select Wait until full explicitly
when testing automatic same-mineral continuation. Save experiments separately.
Dry-run selection was verified in
`logs/space-fixes/final-launch-mining-release.log` and
`final-launch-construction-debug.log`; those commands do not edit user settings.

## Verified evidence

The machine-readable [implementation index](../../../../logs/space-fixes/implementation-evidence.json)
binds the final six binaries, uncommitted source snapshot, immutable demo input
hashes, scoped documentation, build logs and acceptance receipts. Executables
are `build/<RelWithDebInfo|Debug>/openttd.exe`, `openttd_test.exe` and
`openttd_space_fixes_test.exe`. Historical receipts retain the executable hash
that actually ran; they are not relabeled as runs of a later binary.

| Area | Evidence and practical scope |
| --- | --- |
| Reported controls/construction | [Report replay](r2sff1-z-d-report-replay.md): actual window bounds/clicks, normal/editor dispatch, configuration copies and ordinary paid module completion. [Initial layout matrix](r2sff1-b-layout-notes.md) includes smaller/enlarged configurations. |
| Actual mining/custody | [Roles](r2sff1-z-e-mining-roles.md), [full holds](r2sff1-z-f-full-holds.md), [Surface delivery](r2sff1-z-g-surface-return.md), [refusals](r2sff1-z-h-native-refusals.md) and [service/control](r2sff1-z-i-native-service.md) retain original paid actors and packets through ordinary work. |
| Natural expiry and speed | [Lifecycle](r2sff1-z-j-native-lifecycle.md):1596 completed opportunities in each visible/hidden run, all five saved phases and actual next decisions; real driver speed/pause dispatch. |
| Migration | [Compatibility](r2sff1-z-k-native-compatibility.md): ordinary current application load/advance of374/405/418/440 sources; original CPP/mission/order/field ownership and Classic absence retained. |
| Real peers | [Native peers](r2sff1-z-l-native-peers.md) and [high IDs](r2sff1-z-m-high-id-peers.md): real host, owning client command, late observer, local twin and reversed-storage twin. Each shared opportunity compares fifteen owners; every saved phase includes its next real decision. |
| Bounded work/storage | [Work measurements](r2sff1-z-n-lifecycle-work.md): seven isolated native cases per configuration, actual capacity refusal,64 high-ID turnovers, earned intervals and genuine NoCompatibleAsteroid retry work. |

Final `final-high-id-peers-{release,debug}-peers.json` receipts bind the last
ordinary test executables and pass real socket/local/reversed comparisons.
`final-expiry-replay-{release,debug}-driver.log` binds the last focused
executables and passes the actual pre-expiry save's next-opportunity oracle.
`final-report-ui-{release,debug}-driver.log` also binds those focused executables:
actual window/click and normal/editor shortcut checks pass in both configurations.
All final native child deadlines are110 seconds; focused work cases remain
separate invocations. No full regression was run for this closing group.

## Reproduction

Run commands from the repository root, sequentially, with unused run identities.
Use `-B` to avoid Python cache artifacts. Every harness writes native logs under
`logs/space-fixes/` and private configuration/saves/captures/samples under
`test-cache/space-fixes/<run>/`; original fixtures are immutable. Keep parent
redirection separate from the harness's native `<run>.log`.

Rebuild one configuration at a time (change RelWithDebInfo to Debug for the
checked build):

```powershell
& 'C:/Program Files (x86)/Microsoft Visual Studio/2022/BuildTools/Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin/cmake.exe' --build build --config RelWithDebInfo --target openttd_space_fixes_test openttd openttd_test --parallel 2 -- /nodeReuse:false /p:CL_MPCount=4 *> logs/space-fixes/repro-build-release.log
```

Representative focused commands, using existing immutable actual-run sources:

```powershell
python -B src/tests/multilayer/acceptance/space_fixes_checks.py --save space-factory-440.sav --layout --shortcuts --hotkeys retained --run repro-final-ui-release *> logs/space-fixes/repro-final-ui-release-driver.log
python -B src/tests/multilayer/acceptance/space_fixes_checks.py --mining-controls lifecycle --source test-cache/space-fixes/role-heavy-preserved-release/policy-before-departure.sav --run repro-lifecycle-visible-release *> logs/space-fixes/repro-lifecycle-visible-release-driver.log
python -B src/tests/multilayer/acceptance/space_fixes_checks.py --mining-controls lifecycle-hidden --source test-cache/space-fixes/role-heavy-preserved-release/policy-before-departure.sav --run repro-lifecycle-hidden-release *> logs/space-fixes/repro-lifecycle-hidden-release-driver.log
python -B src/tests/multilayer/acceptance/space_policy_replay_checks.py --lifecycle --source test-cache/space-fixes/repro-lifecycle-visible-release --paired-view test-cache/space-fixes/repro-lifecycle-hidden-release --run repro-lifecycle-replay-release *> logs/space-fixes/repro-lifecycle-replay-release-driver.log
python -B src/tests/multilayer/acceptance/space_legacy_compatibility_checks.py --case 405 --run repro-legacy-405-release *> logs/space-fixes/repro-legacy-405-release-driver.log
python -B src/tests/multilayer/acceptance/space_lifecycle_peer_checks.py --source test-cache/space-fixes/high-id-native-funded-release/policy-high-id-current.sav --body 65536 --holding 8 --run repro-high-id-peers-release *> logs/space-fixes/repro-high-id-peers-release-driver.log
python -B src/tests/multilayer/acceptance/asteroid_lifecycle_work_checks.py --case small --run repro-work-small-release *> logs/space-fixes/repro-work-small-release-driver.log
python -B src/tests/multilayer/acceptance/space_fixes_checks.py --mining-controls work-retry --source test-cache/space-fixes/no-mineral-status-heavy-release/policy-no-mineral.sav --run repro-retry-work-release *> logs/space-fixes/repro-retry-work-release-driver.log
```

Each Debug invocation adds `--configuration Debug` and uses a distinct `-debug`
run. Its lifecycle predecessor is
`test-cache/space-fixes/policy-module-route-debug/policy-before-departure.sav`;
its retry predecessor is
`test-cache/space-fixes/no-mineral-status-heavy-fixed-debug/policy-no-mineral.sav`.
The high-ID source uses `high-id-native-funded-debug`. Compatibility cases are
`405`, `418`, `440`, `classic`; work cases are `small`, `normal`,
`largest-normal`, `largest-dense`, `clustered`, `narrow-tall`, `narrow-wide`.
Run only the relevant case. Source preparation and other phase-family commands
are documented in the linked owning notes. Their hashes and actual application
identities are retained by the native receipt/process records.

Regenerate the final index and scoped audit without running tests:

```powershell
python -B src/tests/multilayer/acceptance/space_fixes_evidence_index.py --require-complete *> logs/space-fixes/final-plan-audit.log
git -c core.whitespace=cr-at-eol diff --check *> logs/space-fixes/final-whitespace.log
```

## Limits and remaining ownership

The user's exact personal save is unavailable. Retained-demo/native proof does
not establish an exact screenshot-save reproduction or a construction deadlock.
Legitimate prerequisite waits now disclose their reason and remedy. High-ID
sources declare a historically fundable issuance prefix, then earn real native
birth/extraction/retirement; they do not claim 65,536 elapsed births. Component
geometry/due inputs are declared in their notes. Operation/container counts
exclude allocator overhead and do not promise whole-game FPS.

The Space master retains its financial, useful-service, continuous-chain,
graphics and human acceptance gates. Earlier demo work retains small-map
following/legend, Escape, other popup mappings and frame-rate tooltips; only
the fully delivered Alt+1..4 task is checked there. Original user configuration
and source saves remain preserved. No Git writes were performed. Future work
must re-read its own nearest mandatory reading and actual code; old line ranges
are navigation hints, and historical reflection receipts are not current rules.
