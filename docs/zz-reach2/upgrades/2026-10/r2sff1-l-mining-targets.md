# Nearest mining target: execution notes

Owner: [Space fixes plan](r2sff1-space-fixes-features-1.md), Group 18.
Prerequisite intent and exposure gate: [mining policy](r2sff1-k-mining-policy.md).

## Delivered query and native admission

Groups 17-18 are complete and verified. `CanAcquireSpaceApproach` belongs to the existing
approach owner. It copies only requests
for the considered target, retains original priorities and reservations, derives
the actual target slots, and calls `PlanSpaceApproaches`. A proposed request does
not become a saved request or mining lease. This distinguishes immediate
automatic eligibility from the old `RequestSpaceApproach` quote, which merely
proves a request can be queued.

Its first focused build and native case passed **336 assertions**; evidence is
`logs/space-fixes/build-space-mining-target-slots.log` and
`logs/space-fixes/space-mining-target-slots.log`.

`space_mining_target.h/.cpp` now provide `RankMiningTargetCandidates` over
explicit original/copied metadata and `FindNearestEligibleMiningTarget` over
actual native owners. The ranker includes every finite matching body and sorts
exact squared distance/full-ID keys. The native selector then queries original
assignment admission in that order. Configured assignment rechecks immediate
shared-slot eligibility, so a later claimant cannot be preempted after selection;
old/manual requests retain their original ability to queue.

The ranking/native admission stage passed **634 assertions in three cases** in
`logs/space-fixes/space-mining-target-ranking.log`. It is now followed by
`ForecastSpaceMiningIntercept` before a candidate can be returned. No caller
activates the selector or new exit behavior before the later continuation and
persistence gates.

## Interception, departure and retry semantics

The forecast uses exact copied straight body motion and the original native
radial mover, travel remainder and attachment tolerance. Its first opportunity
sees the supplied already-moved field; subsequent opportunities move the body
first. The existing pursuit deadline bounds the loop but does not become a
minimum travel time. A cheap per-axis best-possible closing bound rejects
obviously late bodies before walking their pursuit.

Both pursuit and its final tolerance snap sweep the original static footprints
and berths through `IsSpaceMovementSegmentClear`. An independently proven
currently admitted source berth can exempt only its own starting point on the
first segment. It cannot exempt a footprint, another berth or a later crossing.
The native selector obtains that proof from `GetSpaceApproachDestination` at
the craft's actual position, never a repositioned temporary Vehicle.

At predicted arrival, the forecast searches the original bounded staging rings
with shared direction/radius constants, clears the target's two-tile envelope,
and counts a real swept local departure with `GetSpaceDepartureTravelTicks`.
The first unsafe attachment must leave an additional release margin after that
travel. The endpoint is a proposal; native occupancy and departure admission
remain authoritative when the craft actually arrives. Original far-field
motion is still reflective until Group29, so the native timing checks stay in
the old band and compare their actual final body motion to the straight forecast.

`GetSpaceMiningTargetRetryDelay` returns 74 simulation opportunities, separate
from the replacement timer. Group20 consumes the saved retry field while normal
simulation runs. `NotifySpaceMiningTargetChange` wakes only an existing matching
FullLoadNearest Mine in `Waiting/NoCompatibleAsteroid`; unrelated mineral,
receiver and service waits remain intact. The actual lease-release owner calls
it after freeing the original claim. Group28 must notify after successful
body-entry/exit commits, using class metadata captured before erasure. Notifications
select nothing, move nothing, allocate nothing and draw no RNG.

`HasHeavyMiningModuleReturn` now uses a configured mission's pinned mineral after
source retirement. Legacy/unpinned instructions still resolve their actual
surviving source. The first subsequent real module Unload, original ownership
and operational capacity remain required; the pinned label grants no service,
pod, allowance or cargo rights.

## Source navigation and integration notes

- `space_approach.cpp::GetTargetSlots` owns body-slot geometry, original mining
  and preview-survey occupancy, native physical occupants and static slots.
  `PlanSpaceApproaches` retains admitted reservations before ordering waiters by
  native request tick and hull ID. Do not add a parallel reservation book.
- `MoveSpaceSurveyTowards` is **radial integer movement**, using integer square
  root, the common four-tile/day speed and retained unsigned remainder. It is
  not independent axis movement. Both mining roles use this same native pursuit
  path in `SpaceVehicle::Tick`.
- `GetSpaceMiningApproachTicks` includes a minimum deadline. Treating that entire
  minimum as necessary travel would falsely reject reachable bodies near an
  edge. An exact copied pursuit bounded by that existing deadline can use
  actual arrival time; reuse `StepSpaceMotionStraight` and the native mover.
- Keep opportunity order explicit: selection during a craft tick sees the
  field already moved by the common tick. Its first immediate pursuit step
  sees that current body position; later opportunities move the body first.
- `ForecastSpaceAttachmentSafety` detects physical edge and static-footprint/
  berth hazards independently from visual size. A candidate must leave time
  for departure after interception. Any conservative departure bound must come
  from the existing local movement math, not the pursuit minimum or wall time.
- `GetSpaceDepartureTravelTicks` bounds each axis to eight tiles, computes a
  conservative limit from Manhattan distance and `minimum_step - 2`, then uses
  the real mover and swept-segment obstacle checks. `FindSafeStaging` inspects
  the current point then eight directions at radii 1..8, at most 65 points.
  The body waiting/departure envelope is two tiles. `PlanSpaceDeparture` checks
  an actual lease and actual pose; never mutate/copy a native Vehicle to force
  it through hypothetical arrival admission.
- Rank current squared distances with body-ID ties before expensive forecast
  work, or use an exact equivalent bounded live scan. A fixed shortlist is not
  proof of global nearest. Derived temporary candidate metadata is allowed;
  another persistent body/target owner is not.
- High-ID and reversed-vector tests can use copied metadata while native sparse
  installation remains gated until Group 22. Real native lease/competition tests
  must still exercise actual approach/assignment owners separately.
- `HasHeavyMiningModuleReturn` now supports the pinned same-mineral intent while
  preserving its first real subsequent module Unload rule.
- `HeavyMiningRuntimeRig` in `src/tests/heavy_mining_runtime.cpp` demonstrates
  actual model21 materialization from `ManufacturingMaterializationRig`, native
  Mine/module-Unload orders and common motion/approach/tick opportunities. The
  smaller `ProbeExtractionRig` supplies only its legacy miner; do not assume
  it has survey/heavy Engine definitions.

## Group 18 verification

The final focused build passed. The burst
`[space-mining-target],[space-mining-intercept-native],[space-departure-plan],[space-mining-policy]`
passed **3,862 assertions in 17 cases in about one second**. It covers actual
shared arbitration/leases, unchanged quotes, refused stale automatic assignment,
partial incompatible freight, depletion, full-width sparse IDs, reversed storage,
distance ties, a doomed nearest body with a safe second candidate, edge timing,
static swept paths, source-berth exception bounds and mineral-scoped retry events.

`space_mining_intercept_native.cpp` compares forecast opportunity count, final
body motion, attachment pose and native travel remainder against actual model15
and factory-materialized model21 pursuit. The heavy fixture uses original
manufacturing material/work owners with explicitly declared unit frames/content;
it makes no gameplay procurement or whole-save claim. Both actual arrivals also
have a ready native departure plan. Policy and local-departure checks were
included because their shared admission/movement functions changed; no full
regression was run.

Evidence: `logs/space-fixes/build-space-mining-target-verification.log` and
`logs/space-fixes/space-mining-target-verification.log`.
Executable SHA256:
`73e16cf4f2d402df9ddc2120e7f85ddd967115f104c9023abf4df22227a5ce8f`.
Earlier compiler diagnostics (incorrect proposal member name and a narrowing
test cast) remain in the `build-space-mining-intercept*.log` files. All were
corrected before the final burst.

For further work, read the main Global Mandatory Reading and the nearest
**Native mining policy and target selection** block scoped to the active group.
Named symbols are authoritative because the original line ranges have drifted.
