# Original paid service waits and recovery

Owner: [Space fixes plan](r2sff1-space-fixes-features-1.md), Group31D.
Group31D is complete in both configurations, including actual paid service,
resumed finite extraction and explicit stop/resume/Recall.

## Selected original preconditions

Read-only native inventory mode `receiver-inventory` records all actual hull
roles/placement, allowance/pending count, service history, parts, pods, module
stock and original station parts/construction stock. All13 synchronized owner
observations remain unchanged. Logs/artifacts use
`service-inventory-<case>-release` under the established logs/test-cache owners.

- `pending-light` / `settled-light`: corresponding actual31B
  `role-surface-release/policy-surface-{descent|settled}.sav`. Original15 already
  spent its genuine last shell and has RFMT1/ELEC1. Before actual descent
  settlement it has allowance1/pending1; afterward allowance0/pending0. One real
  onboard service can renew allowance, but creates no separately paid shell.
- `resupply`: immutable `src/tests/fixtures/demos/space/space-resupply-414.sav`.
  Original15 has actual40 lifetime units, two spent20-unit returns, allowance0,
  no parts/pods, and its existing blocked next Mine. The separate actual paid
  rocket is already in ascent with two shells and RFMT1/ELEC1 for that original
  incarnation. The original `space_resupply.cpp` native prepare loop at111-124
  earns the two returns through existing Mine/Visit/release owners; no counter
  assignment establishes this precondition. Its incoming delivery remains a
  genuine native event, not a reason to freeze the rocket or its clock.
- `unloaded-heavy`:31B's actual
  `role-full-status-heavy-release/policy-after-unload.sav`. Original21 spent its
  last quota through the real full40-unit unload. It retains its two paid onboard
  parts per label and two carried shells; the real Core has only RFMT1/ELEC1,
  insufficient for its distinct two-part module-service bill. Its actual Core
  arrival can be re-admitted with an explicit native Visit after queue editing.

The unrelated immutable `space-heavy-service-440.sav` was inspected read-only
but is not needed as a service predecessor; the above own actual full-return
checkpoint already establishes heavy expenditure and available original parts.
Actual station4 stocks remain available for legitimate procurement; these are
not grants to any hull or proof of delivery. Do not duplicate a manifest, issue a
free shell, assign allowance/work, or count service as pod manufacture.

## Planned focused execution

31D.1 observes actual pending descent, a genuinely missing shell after paid
onboard service, the original incoming-resupply world's missing onboard parts,
and the heavy's underfunded Core. The same heavy occupies the sole real Core
berth; an existing deployed constructor has a funded one-part bill but cannot
borrow that berth. Record that constructor admission scope explicitly rather
than calling it another completed mining-role service.

31D.2 must supply/complete the original owners, preserve the held Full Load Mine
and observe renewed real extraction. For15, the incoming paid rocket supplies
the existing parts/shells without renewing allowance. For21, original onboard
parts can move through `CmdMoveOrbitalSupply` into the underfunded Core; only the
real role bill and common power/work path may complete its service. Every native
process retains the110-second budget and actual saved next-opportunity oracles.

31D.3's explicit stop/resume/Recall gates remain pending. No master financial
trial is resumed by these bounded native service cases.

## Native wait acceptance:31D.1

Both configurations pass the four actual source-qualified modes:

| Mode / phase | Original role and actual cause | Assertions | Ordinary opportunities |
| --- | --- | --- | --- |
| `service-pending` |15's actual descent still claims its last allowance; no service may start while pending. |93 |5 |
| `service-shell` |15 earns an actual148-opportunity onboard service with its original RFMT1/ELEC1. Allowance becomes2, but no pod exists; the held Mine waits for paid replacement pods. |1,589 |154, including148 genuine service opportunities |
| `service-parts` | Genuine414 original15 spent two returns and has no onboard parts/shells; the separate existing paid resupply rocket remains a native incoming event. |80 |4 |
| `service-module` |21's actual full return exhausted quota. Its actual Core has one part per label against the real two-part bill; its two onboard parts remain a separate owner. |118 |6 |

The completed prior Mine/return cursor is followed by a real checked new Full
Load Mine where required. The414 world already has its own blocked next Mine;
only its genuine policy is changed. No packet, allowance, countdown, lease or
placement is assigned. Future target selection uses an actual finite body with
enough original remaining travel time for the bounded observation. Heavy's real
Core Visit restores admission through original arbitration after queue editing.
Each saved wait retains its actual instruction/cursor, holding and extraction
history, with no hidden mining or unearned service.

The heavy really owns the sole Core berth. An existing deployed constructor20
has a funded one-part bill there but cannot acquire that occupied berth; both
shared approach and native service-navigation queries refuse without changing
owners. This is actual same-world constructor admission evidence, explicitly
distinct from another completed15/21 service. Heavy's own refusal is the separate
two-part module shortage, even though its onboard compartments hold two parts.

Native captures exposed two display defects. The order heading used the old
fixed one-part service caption for21, while its actual contract row correctly
showed two parts per label. Initial blocked Mines also appeared Idle in orbit in
the fleet because they had no continuation pin yet. The draw-only mapping now
uses the native generic service wait and the actual role bill below it. Concrete
pending/pod/service blocks for an original current Mine supply status before
first admission or while waiting; real moving departure/return phases retain
their own status. No strings, IDs, bill, command or simulation owner changes.

Reviewed current captures show the same precise pending/pod/service reason in
the order, continuation and selected fleet rows. The heavy bill remains2 RFMT,
2 ELEC and296 work opportunities. The shell case shows allowance2/pods0 after
real service, without claiming another service bill could create a pod.

All four actual saved phases independently pass next-opportunity replay and
complete CPP/mission/queue/ASTR no-op transport in both configurations. Current
status replays each have63 assertions and leave all13 synchronized owner
observations unchanged while capturing controls and the final row. A separate
1024x768/200%/font20 heavy service replay passes bounds/hit rectangles/wrapped
last-row reachability; its current bottom capture was reviewed.

Sources:

- Pending/shell: corresponding `role-surface-{release|debug}` actual phase saves.
- Parts: immutable `src/tests/fixtures/demos/space/space-resupply-414.sav` in each
  configuration, retaining its actual paid original40-unit/two-return history.
- Heavy: `role-full-status-heavy-release/policy-after-unload.sav` and
  `role-full-heavy-debug/policy-after-unload.sav`, each its own actual full return.

Artifacts:`service-wait-{pending|shell|parts|module}-{release|debug}` under
`test-cache/space-fixes/`, with exact hashes/configuration/executable identity in
each report. Current accepted captures/replay transport receipts use
`service-status-replay-<kind>-<configuration>`; the phase directory appends
`-service-<kind>`. Original pre-fix `service-wait` captures remain diagnostic for
the misleading heading/fleet text; their real work/wait assertions and saved
decision oracles remain valid. Initial and corrected independent receipts are
both retained. Wrap:`service-status-wrap-release`.

Product and both test targets build in
`build-service-status-{release|debug}.log`; initial focused scenario builds are
`build-service-waits-commands-release.log` /`build-service-waits-debug.log`.
The first native helper compile omitted the existing command declaration header;
including `launch_center_cmd.h` fixed it without changing an API. Final
whitespace:`service-waits-final-whitespace.log`. All logs/transport receipts live
under`logs/space-fixes/`; native captures/saves/private runtimes stay in test-cache.

Reproduce after current builds, with fresh run identities:

```powershell
python -B src/tests/multilayer/acceptance/space_fixes_checks.py --source src/tests/fixtures/demos/space/space-resupply-414.sav --mining-controls service-parts --run service-parts-new
python -B src/tests/multilayer/acceptance/space_policy_replay_checks.py --source test-cache/space-fixes/service-wait-parts-release --role 15 --service-wait parts --run service-parts-replay-new
```

Other modes use the sources above; module replay requires`--role 21`. Debug adds
`--configuration Debug`. No full regression or master financial trial was run.

## Actual funded service and resumed extraction:31D.2

Complete in both configurations. Each input is its own31D.1 actual
`service-wait-parts-<configuration>/policy-service-parts.sav` or
`service-wait-module-<configuration>/policy-service-module.sav`.

| Original role | Actual supplying/working owner | Assertions per configuration | Ordinary opportunities | Real service opportunities |
| --- | --- | --- | --- | --- |
|15 | The already paid original separate rocket delivers its real two pods and RFMT1/ELEC1. Original onboard service then consumes that bill. |7,319 |771 |148 |
|21 | `CmdMoveOrbitalSupply` moves one original onboard RFMT/ELEC into each underfunded Core buffer. The actual Core job reserves/consumes its whole2+2 bill through common electricity. |8,261 |870 |296 |

The incoming rocket's actual generation-qualified target remains the original15.
Only ordinary flight moves its payload. Original pod IDs/generations, positive
paid receipts and actual Craft custody all transfer once; exact combined part
Source totals remain unchanged. Resupply leaves allowance0 and service history
unchanged, so arrival alone cannot masquerade as service completion.

Heavy supply movement requires its actual reached Core berth. Queries preserve
all13 owners; checked transfers preserve whole combined Source quantities across
the original onboard/Core containers despite legitimate CPP merging. The Core
then owns its actual2+2 reserved bill. Its remaining onboard1+1 stays untouched
through service, and no carried pod is created or consumed by that service.

Original remaining-work owners decrease exactly once per ordinary opportunity.
Every heavy work opportunity settles exactly750J in the Core service consumer's
actual saved shares. Total service use is222,000J; the original panel's settled
lifetime use retains at least that amount while other consumers keep their own
accounting. No grant, generator, work counter or synthetic meter is assigned.
Small service uses its original onboard parts/upkeep contract; its recorded
common-grid service use is0J, not a claim of free parts or upkeep.

After148/296 genuine opportunities, each role completes exactly one service,
replaces exhausted allowance with its original bounded2, consumes only its own
actual bill and retains its paid shell identities. The original held Full Load
entries/cursor then resume real admission, pursuit, lease and extraction. Actual
new output is2 units from body4 for15 and4 from body9 for21. Finite reserve debit,
typed `Source::FromAsteroid` and lifetime extraction increment match exactly.
Small's historical instruction remains its original target; native continuation
may choose another actual source without rewriting that instruction. No geometry
or arrival is declared/assigned anywhere in these recovery cases.

Five actual phases are saved: supplied, started, working, completed and resumed.
Each role/configuration passes independent next-opportunity replay (63 assertions)
and complete no-op CPP/mission/queue/ASTR transport. All four native utility
families UPLH/UPLD/UPPH/UPPD are additionally compared byte-for-byte across every
no-op resave, retaining whole source/grant/settlement/lineage authority. Actual
next work is independently replayed rather than inferred from those byte checks.

Reviewed native captures show the heavy's real148/296 remaining work with its
2+2 reserved bill, then the small miner's resumed2/20 holding and actual working
lease. Every saved phase's current controls/bounds/final row is checked during
its separate replay. Test-only copied Source totals remain observations with no
saved/registry authority; body pointers are scoped and reacquired by stable ID
after native field publication.

Current artifacts: `service-resume-owners-{light|heavy}-release` and
`service-resume-{light|heavy}-debug`. Logs have matching identities under
`logs/space-fixes/`, with separate`-driver.log` output. Their exact immutable
source/configuration/executable hashes remain in each private report. Current
builds:`build-service-resume-owners-{release|debug}.log`. Earlier Release
`service-resume-{light|heavy}-release` runs passed before the extra remaining-
onboard/pod assertions; current owners runs above are the final gate.
Replay/transport receipts:`service-resume-replay-{light|heavy}-{release|debug}`.
Whitespace:`service-resume-whitespace.log`.

Reproduce with fresh run identities after the focused native build:

```powershell
python -B src/tests/multilayer/acceptance/space_fixes_checks.py --source test-cache/space-fixes/service-wait-parts-release/policy-service-parts.sav --mining-controls resume-service-light --run service-resume-light-new
python -B src/tests/multilayer/acceptance/space_policy_replay_checks.py --source test-cache/space-fixes/service-resume-owners-light-release --role 15 --service-resume --run service-resume-light-replay-new
```

Heavy uses its module-wait source,`--mining-controls resume-service` and
`--role 21`. Debug uses its own source with`--configuration Debug`. Private
runtime/save/capture output stays under`test-cache/space-fixes/<run>/`; no retained
input, user configuration, financial/master trial or external application changes.

## Stopped safety, actual resume and checked Recall:31D.3

Complete in both configurations:1,245 assertions and84 ordinary opportunities
for each original role. Inputs are actual `role-input-light-module-<configuration>`
and `policy-input-heavy-fixed-release` /`policy-input-heavy-debug` original
`policy-current.sav` checkpoints. Each has one actual emitted2/4-unit batch,
incomplete original work, a current Full Load Mine and a genuine body lease.

The original Start/Stop command is estimated without changing any owner, then
executed before declaring its body's last-safe attachment geometry. Ordinary
field safety releases the unsafe lease/work despite the stopped flag. The actual
hull stays stopped, with a stable safe pose, unchanged order/cursor, original
CPP IDs/Source/quantity and20/40 capacity. Its detached intent stays frozen while
ordinary field motion continues. Actual idle upkeep still decreases its own
native profit; this is not a free-stop contract or a paused-world substitute.

The original command explicitly resumes that incarnation. Actual local safe
departure movement is observed before Recall; changing a running flag alone is
not counted as resumed movement. A stale-generation checked Recall refuses
without changing any of13 owners. The correct command clears the original
queue/continuation and preserves emitted cargo/lifetime work.

After Recall, only the old unleased source's final-contained geometry is declared.
Its real next native exit earns credit and publishes matching resource change.
Across that event and a full74-tick retry interval plus two more opportunities,
the cancelled Mine never reacquires any body or regenerates an instruction.
The original empty queue/default intent persist; native legacy Return/Holding
may continue safe motion with cargo aboard. Both geometric declarations retain
the body's original heading; neither is claimed as elapsed travel.

The first actual stopped capture exposed a misleading Detaching safely caption
while its controller was frozen. The read-only mining status now reports the
original Stopped flag, and the generic Working/travelling order heading follows
that status. Concrete supply waits retain their information. Active onboard
service keeps its original separate work contract while parked; no service,
movement, safety or billing behavior changes. The Area reading list now makes
those existing exceptions explicit, including safe attached-body following.

Current native captures were reviewed: Stopped appears in the order,
continuation and selected fleet rows; after Recall there is no current Mine or
destination, controls are inert as appropriate, and actual4/40 heavy cargo remains
aboard during original safe staging return. A1024x768/200%/font20 stopped-heavy
replay passes window/control hit bounds and final wrapped-row reachability; its
bottom capture was reviewed. Earlier `stop-recall-<role>-release` captures retain
the pre-fix caption for diagnostic comparison; final assertions also require
positive emitted holding and preserved native freight capacity.

Four saved phases per role/configuration (stopped edge, resumed edge, Recall
request and stable Recall) each pass independent actual next-opportunity replay
with63 assertions and complete CPP/mission/queue/ASTR no-op transport. Current
native artifacts/logs:`stop-status-{light|heavy}-{release|debug}`;
replay/transport receipts:`stop-recall-replay-{light|heavy}-{release|debug}`;
wrap:`stopped-wrap-release`. Product and both test targets build in
`build-stopped-status-{release|debug}.log`. Final whitespace:
`stop-recall-final-whitespace.log`.

Reproduce with fresh run identities:

```powershell
python -B src/tests/multilayer/acceptance/space_fixes_checks.py --source test-cache/space-fixes/role-input-light-module-release/policy-current.sav --mining-controls stop-recall-light --run stop-status-light-new
python -B src/tests/multilayer/acceptance/space_policy_replay_checks.py --source test-cache/space-fixes/stop-status-light-release --role 15 --stop-recall --run stop-recall-light-replay-new
```

Heavy uses its original input above,`--mining-controls stop-recall` and`--role 21`.
Debug adds`--configuration Debug` and its own source. All captures/saves/private
runtime artifacts stay in test-cache; logs/transport receipts stay in logs.
No full regression or financial/master simulation trial was resumed.
