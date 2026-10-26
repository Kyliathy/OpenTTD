# Saved legacy mining departure

Owner: [Space fixes plan](r2sff1-space-fixes-features-1.md), Group25.
Recorded 2026-10-24; prerequisite: [joined transport gate](r2sff1-r-transport-gate.md).

## Certified boundary

The original mission's existing40-byte continuation retains a local departure
for an actual unordered legacy miner. The queue stays empty. Publication pins
the actual live source mineral, releases its lease/partial work and preserves
position, home, cargo, costs and stopped choice. Stopped native Tick retains the
point/retry. Running Tick makes one revalidated swept local step; completion
clears this intent and resumes original Return on a later opportunity. A removed
source remains issued history. Blocked departure retains Detaching,
DepartureBlocked and bounded74-tick retry without a fake queue block or berth.

Recall/cancel supersedes this state and retains cargo. Recall retains history;
cancel keeps its original target-clearing/Holding behavior. Typed LegacyReturn
Mine completes only itself after local departure; subsequent original Visit/work
instructions execute through their original owner, including no handling route.

Save era444 (`SpaceLegacyDeparture`) widens only these semantics. VEHS40-byte
intent, SPOR15-byte orders, field442 and cargo441 layouts are unchanged. Full and
copied source-era gates reject nondefault unqueued intent in genuine443 sources.
Old default state stays inert. Shared validation requires bounded local state,
supported role, immutable mineral/issued history and no hidden route, effective
target, work, lease or automatic unqueued selection.

This is a persistence/dispatcher producer gate. Group25B owns actual unsafe-step
publication and Group29 normal-loop wiring. No body retirement or renewal event
is enabled here.

## Focused evidence

RelWithDebInfo and Debug each pass **22,583 assertions in20 cases** using only
`[space-mining-continuation],[space-mining-wire],[space-mining-policy]`.
Logs: `logs/space-fixes/legacy-departure-{release|debug}-final.log`.
Cases include actual native pursuit/extraction, stopped/running local travel,
declared funded body-removal components, stale blocked waypoint, Recall/cancel,
typed legacy completion and copied transport/source-era faults. Components do
not claim an executed terminal event or paid procurement.

Actual working405 and retained443 inputs save/preview as444 without movement.
Both configurations publish running and stopped test-only departures; separate
native processes full-load, preview and resave444. Native checks compare original
packets, field/renewal, mission/queue digests, costs, pose/home, flags, clock and
four global RNG words. Independent raw comparisons preserve all three hulls'
saved mission/stop/intent/queue on reload. Original405's three packets,40 units,
two natural origins and one ordinary source survive through Debug reload.

Native logs under `logs/space-fixes/`:

- `legacy-{release|debug}-{running|stopped}-443.log`
- `legacy-{release|debug}-{running|stopped}-reload.log`
- `legacy-{release|debug}-old-forged-{preview|full}.log`
- `legacy-release-stopped-405.log` and `legacy-release-isolation.log`

The forgery transplants actual444 departure families into original443 framing,
preserving genuine version/schema and original packets. Both full and
valid-corrupt-valid preview refuse it in each configuration. Independent preview
keeps a different actual440 factory world open, preserving owners/clock/RNG.

Native artifacts are under `test-cache/space-fixes/<run>/cargo-origins-current.sav`.
Retained443 input: `test-cache/space-fixes/mining-wire-legacy-405/cargo-origins-current.sav`.
Immutable original: `src/tests/fixtures/demos/space/space-mining-working-405.sav`.

Rebuild `openttd_space_fixes_test`, then run
`src/tests/multilayer/acceptance/space_fixes_checks.py --cargo-origin`
with `--legacy-departure running|stopped`. Paired refusal uses
`legacy_departure_wire_checks.py --source <443> --departure <444> --run <unique>`.
Optional `--configuration Debug` selects Debug. All mutable output stays inside
test-cache/logs; driver logs must end `-driver.log` to avoid locking native logs.
Build configurations sequentially because their shared version generator can
race. Each native process retains the110-second harness deadline.

Final focused executable hashes:

| Configuration | SHA256 |
| --- | --- |
| RelWithDebInfo | `e80d8eadc9ee38716048238437fadfad60e10779024b82d6930546e21401ec0f` |
| Debug | `919b22c584b2e7b6031d7fd13f3d6f69a57547139722fed3910f769efae4f7e9` |

Build logs: `logs/space-fixes/build-legacy-assertions-{release|debug}.log`.
The plan's final normal game build remains pending.
