# Native missing-target and receiver refusals

Owner: [Space fixes plan](r2sff1-space-fixes-features-1.md), Group31C.

## Missing compatible mineral:31C.1

Both original roles pass381 assertions in each configuration. Inputs are the
unchanged earned partial-holding `policy-current.sav` checkpoints from
`role-input-light-{release|debug}` (original15, genuine missions440 source) and
`policy-input-heavy-fixed-release` / `policy-input-heavy-debug` (original21,
genuine heavy440 source). The preceding role evidence records original hashes;
each native run's report fingerprints its exact input and executable.

Only contained terminal geometry is declared: the leased body's existing
last-safe attachment point, and the five other matching bodies' last-contained
centres. Original headings, reserves, paid hulls, source packets, work clocks,
credit, RNG and countdown are preserved. Actual ordinary field opportunities
retire those five bodies and arm a real replacement interval. The original
source remains live but unsafe to intercept; the observer does not erase it.

After32 ordinary opportunities each role has its native NoCompatibleAsteroid
wait, original Mine/cursor and actual2/4-unit holding. No approach or effective
destination remains. The safe pose is stable; the saved74-tick retry decreases
once per opportunity. The newly armed renewal countdown remains active and is
not suppressed. Read-only target selection, save/preview and actual window
captures leave all13 synchronized owner observations unchanged.

Native captures exposed a misleading generic order heading above the correct
continuation reason. The draw-only order heading now resolves the native named
reason when its queue block is ProducerUnavailable. Concrete pending/pod/service
blocks retain their own disclosures. The reviewed current capture says
"Waiting for a same-mineral asteroid" in the order, continuation and fleet rows,
with the original Wait until full button selected and every control reachable.

Both configurations also pass independent actual next-opportunity replay and
complete no-op CPP/mission/queue/ASTR transport for both roles. Every next-step
observer has63 assertions. Retry/resource-event acceptance continues in31C.4;
these checks do not yet claim that later gate or full/removed receiver coverage.

Artifacts: `test-cache/space-fixes/no-mineral-status-{light|heavy}-release/`,
`no-mineral-status-light-debug/`, `no-mineral-status-heavy-fixed-debug/`.
Logs use the same run names under `logs/space-fixes/`, with separate
`-driver.log` output. Replay/transport receipts use
`no-mineral-replay-{light|heavy}-{release|debug}`. Current product and both test
targets build in `build-no-mineral-precise-status-{release|debug}.log`.

Rebuild after source changes, then reproduce with fresh run names:

```powershell
python -B src/tests/multilayer/acceptance/space_fixes_checks.py --source test-cache/space-fixes/role-input-light-release/policy-current.sav --mining-controls no-mineral-light --run no-mineral-status-light-release
python -B src/tests/multilayer/acceptance/space_policy_replay_checks.py --source test-cache/space-fixes/no-mineral-status-light-release --role 15 --no-mineral --run no-mineral-replay-light-release
```

Heavy uses `--mining-controls no-mineral`, `--role 21` and its original input
above. Debug adds `--configuration Debug`. Native PPM captures can be converted
losslessly beside their originals with Pillow; PNGs remain in test-cache.
The nonexistent heavy Debug input named `policy-input-heavy-fixed-debug` was
rejected before launch; the corrected existing input above passes. The first
compile's incorrect forecast-member spelling was corrected to the actual
`ticks_until_exit` API; no product forecast or clock contract changed.

## Actual receiver refusal and farther fallback:31C.2

Complete in both configurations. Original retained Core0 starts with40 Nickel
Ore; it is the only operational compatible receiver. Kind8 is Storage, not Dock:
site1 is a fully consumed paid Storage frame awaiting activation, site2 is still
a blueprint, and the operational Assembly provides no general ore capacity.
The older declared processor full-output fixtures use packet surgery and are
excluded from this ordinary gameplay proof.

The genuine preparation chain starts at
`role-input-light-module-release/policy-current.sav`, then uses
`receiver-berth-earned-release-01` through`-09`, followed by
`receiver-visit-earned-release-01` and`-02`. Each intermediate input is exactly
the preceding actual `policy-receiver-prepared.sav`; reports fingerprint that
input and its native executable. The final stage owns `policy-no-receiver.sav`.
Each process offers at most4000 opportunities; the final974-opportunity stage
brings the actual chain to40,974 opportunities. Ten genuine20-unit unloads add
200 units to the inherited40. The original15 then earns another full20-unit
holding. Each stage checks stock plus holding against actual lifetime extraction
and preserves its original allowance1. Other actual hulls are explicitly stopped
through the native command, retaining their paid allowances; their original
safety and idle-upkeep owners still run.

The actual refusal is the **64-fragment transport bound**, with240 units in
the receiver and six incoming original packet fragments. Quantity capacity is
still1024; this is not a claim that1024 units were procured. Native whole-batch
admission rejects64+6 references. Real NoReceiver retains every held packet's
ID/Source/quantity, original Mine/cursor, full20-unit capacity and allowance.
Retries decrement normally and the reached Core berth remains physically real.
An explicit native Visit before handling makes the eventual alternative strictly
farther away; no arrival position is assigned.

Release final refusal passes6,855 assertions. Debug reads that actual earned
current checkpoint as its declared retained precondition and independently
passes65 assertions over its next four ordinary opportunities; it does not claim
to repeat all prior preparation. Separate saved next-opportunity replay and
complete no-op CPP/mission/queue/ASTR transport pass in both configurations.

The positive branch explicitly activates existing paid Storage1 with
`CmdActivateOrbitalModule`. Its fully consumed recipe/capital remain original;
the native activation countdown alone commissions it. Its original constructor
still occupies the approach, so a real native Visit order sends that original
hull to an existing body. Ordinary movement frees the berth. The miner retains
its Mine and waits for the existing bounded retry; activation supplies no cargo,
shell, allowance or fabricated wake event.

Native selection then chooses Storage1, whose actual squared distance exceeds
that of the physically visited full Core0. Real travel and original module
handling move20 units once. Every complete Source total matches the held cargo,
Core stock remains240, the Mine/Visit/Unload cursor completes, and original15's
allowance remains1. Release passes2,009 assertions over207 opportunities; Debug
passes1,982 over204. Their slight timing difference follows Debug's declared
four-opportunity refusal checkpoint, with no altered simulation clock.

Actual activated/travel/unloaded phase saves each pass separate next-opportunity
replay (63 assertions) and complete no-op transport in both configurations.
Reviewed native captures show the full20/20 compatible-receiver wait and the
final empty hold/completed order; controls and the final fleet row remain visible.

Artifacts and logs:

- Preparation progress receipts: `logs/space-fixes/receiver-berth-earned-release-progress.json`
  and`receiver-visit-earned-release-progress.json`; each stage has its own native
  log, separate driver log, private runtime, actual save and captures.
- Full refusal: `receiver-visit-earned-release-02` and`receiver-full-refusal-debug`;
  transport receipts `receiver-refusal-replay-{release|debug}-transport.json`.
- Farther positive branch: `receiver-storage-fallback-{release|debug}`;
  transport receipts `receiver-fallback-replay-{release|debug}-transport.json`.
- Final focused builds: `build-receiver-storage-{release|debug}.log`;
  whitespace: `receiver-fallback-whitespace.log`.

Reproduction with fresh run names:

```powershell
python -B src/tests/multilayer/acceptance/space_receiver_preparation_checks.py --source test-cache/space-fixes/role-input-light-module-release/policy-current.sav --run receiver-earned-new
python -B src/tests/multilayer/acceptance/space_fixes_checks.py --source test-cache/space-fixes/receiver-visit-earned-release-02/policy-no-receiver.sav --mining-controls receiver-fallback --run receiver-storage-fallback-new
python -B src/tests/multilayer/acceptance/space_policy_replay_checks.py --source test-cache/space-fixes/receiver-storage-fallback-release --role 15 --receiver-fallback --run receiver-fallback-replay-new
```

The current preparation runner includes the explicit Visit from its first cycle,
so its new phase timing/fragment totals may differ from the retained historical
chain above; actual native assertions, not fixed elapsed timing, govern success.
An earlier orchestration cleared the real source berth before querying its next
target; it earned one20-unit unload and then made no progress. That abandoned
`receiver-earned-release` branch is diagnostic only. The corrected chain queries
while its physically reached berth remains admitted. The first quantity-only
fullness assertion correctly exposed the original64-fragment bound; the observer
now checks both genuine native limits. A mistaken Dock frame precondition was
corrected against the actual named enum and paid Storage frame. Neither issue
changed production stock, actor identity, tuning or ownership.

## Checked real receiver removal:31C.3

Complete in both configurations,164 assertions and five ordinary opportunities
each. Each input is its own preceding `receiver-storage-fallback-{release|debug}`
actual `policy-receiver-travel.sav`. Existing paid Storage1/generation2 is empty
and operational; its original constructor has physically departed. The original
miner remains loaded and remote, with its actual approach request waiting.
`ClassifyOrbitalModuleRemoval` admits real removal; all stock is actually empty.

This uncovered and fixed a production bug: removal cleared the matching shared
request but retained ApproachBusy for a held Mine whose effective receiver
differed from its historical instruction. That block requires a real approach,
so an immediate paused save failed native order validation. The existing checked
removal command now changes that cleared request's ApproachBusy to TargetRemoved.
It preserves the original intent/history, queue/cursor, cargo, pose and allowance;
the ordinary controller alone reselects next opportunity. No validator is relaxed
and no simulation opportunity or free refund is earned by the command.

Immediate save/preview now succeeds. Stale removal, freight transfer, arrival and
candidate admission all refuse the exact removed generation without changing
any original owner. With the remaining Core still transport-full, the next
ordinary opportunity publishes NoReceiver, releases effective navigation and
sets its native74-tick retry. Subsequent ordinary retries retain the actual full
holding and pose. No replacement facility is invented or pool record deleted
by the observer; it calls only the original checked command.

Two actual phases, immediate removal and resulting wait, each pass separate
next-opportunity replay and complete CPP/mission/queue/ASTR no-op transport in
both configurations. All current product/focused/ordinary targets build, and
the20 focused removal/return/continuation/approach cases pass26,651 assertions
in each configuration. No unfiltered regression was run.

Reproduce after current builds, with fresh run identities:

```powershell
python -B src/tests/multilayer/acceptance/space_fixes_checks.py --source test-cache/space-fixes/receiver-storage-fallback-release/policy-receiver-travel.sav --mining-controls receiver-remove --run receiver-removed-new
python -B src/tests/multilayer/acceptance/space_policy_replay_checks.py --source test-cache/space-fixes/receiver-removed-valid-release --role 15 --receiver-removed --run receiver-removal-replay-new
```

Debug uses its corresponding actual travel source and`--configuration Debug`.
Current artifacts: `receiver-removed-valid-{release|debug}`; native logs have the
same identity under`logs/space-fixes/`, with separate`-driver.log`. Replay/transport
receipts: `receiver-removal-replay-{release|debug}`. Product and both test builds:
`build-receiver-removal-valid-{release|debug}.log`; focused unit logs:
`receiver-removal-focused-{release|debug}.log`; whitespace:
`receiver-removal-whitespace.log`. The initial failing `receiver-removed-release`
capture/save attempt remains diagnostic evidence of the pre-fix validation bug.

## Saved refusal decisions, remedies and native resource wake:31C.4

Complete in both configurations. All prior refusal families already have their
own actual next-opportunity and complete no-op CPP/mission/queue/ASTR checks:
the two original roles' NoCompatibleAsteroid waits, the genuinely full receiver,
the immediately removed receiver and the resulting NoReceiver wait. They are
separate actual native application runs, not copied component fixtures.

New `mineral-wake-{light|heavy}-{release|debug}` cases each pass119 assertions
over four ordinary opportunities. Inputs are each corresponding31C.1 actual
`policy-no-mineral.sav`. A surviving unrelated body1 and the original pinned
source0 have their unchanged headings projected to contained terminal geometry
in separate declared stages. Neither declaration earns elapsed travel, credit,
RNG draws or retry ticks. Actual native field opportunities alone retire them.

The unrelated-mineral exit genuinely earns its size credit but preserves the
selected miner's retry cadence: one decrement. The matching-mineral exit invokes
the real publication/wake path before Vehicle ticks. Native failed reselection
rearms74 rather than decrementing the prior unexpired retry; the next ordinary
opportunity decrements to73. No direct Notify call, target grant, lease, reserve
refill or retry assignment appears in the observer. The original Mine/cursor,
exact held packet IDs/Source/quantity, safe pose and released approach remain
unchanged. The actual retired source remains valid immutable cargo history.

Three saved phases per role/configuration cover unrelated exit, immediately
before the matching exit/wake, and after wake. Independent current applications
match each13-owner next decision (63 assertions) and independently preserve all
CPP rows, native mission/order state and the full ASTR ledger/private RNG/timer
across no-op load/preview/save. No unrelated test families are rerun.

The actual native captures distinguish the named same-mineral and compatible-
receiver waits. The same-mineral wait keeps the real Nearest receiver control
available;31B already proves that real policy action recovers partial cargo.
The full-receiver diagnostic records the exact64+6 fragment refusal, and31C.2
proves the actual remedy: commission the existing paid compatible frame, free
its physical berth and allow bounded native reselection. For a removed receiver,
the immediate command publishes TargetRemoved and the next opportunity reports
NoReceiver; no phantom arrival or hidden transfer supplies a remedy. Restoring
a permitted receiver is required. These are operational waits with retained
cargo, not invented work or a controller deadlock.

`receiver-removed-wrap-release` separately replays the actual removed-wait save
at1024x768/200%/font20. Native control hit rectangles, window bounds, final wrapped
fleet row and all13 draw-purity observations pass; its bottom capture was reviewed.
The heavy after-wake capture was also reviewed after the original source's real
retirement: it shows no current destination, the named mineral wait and actual
4/40 holding without resolving the retired history to a new live body.

Reproduce after the focused native build, using fresh run names:

```powershell
python -B src/tests/multilayer/acceptance/space_fixes_checks.py --source test-cache/space-fixes/no-mineral-status-light-release/policy-no-mineral.sav --mining-controls mineral-wake-light --run mineral-wake-light-new
python -B src/tests/multilayer/acceptance/space_policy_replay_checks.py --source test-cache/space-fixes/mineral-wake-light-release --role 15 --mineral-wake --run mineral-wake-replay-light-new
```

Heavy uses`--mining-controls mineral-wake` and`--role 21`. Debug uses its own
actual input,`--configuration Debug`, and`no-mineral-status-heavy-fixed-debug`
for its heavy source. Exact reports, saves, native PPMs and lossless PNGs stay
under`test-cache/space-fixes/<run>/`; native/separate driver logs and
`mineral-wake-replay-{light|heavy}-{release|debug}-transport.json` receipts stay
under`logs/space-fixes/`. Builds:`build-mineral-wake-{release|debug}.log`;
final whitespace:`native-refusals-final-whitespace.log`.

Group31C is complete. Source fixtures, user configuration and original paid
ownership remain unchanged. No master financial simulation was resumed.
