# Attachment safety and bounded departure

Owner: [Space fixes plan](r2sff1-space-fixes-features-1.md), Groups 10, 10B and 10C.
Prerequisite: [straight-motion geometry](r2sff1-f-straight-motion.md).
The split separates pure collision timing, bounded departure proposals and
native slot publication. Live lease release still belongs to Group 25, with
terminal body motion connected only after the later persistence gates.

## Group 10: first unsafe attached centre

`ForecastSpaceAttachmentSafety` extends the existing `space_departure.h`
boundary and is implemented in the shared approach owner. Inputs are the original
body motion, its exact pure exit forecast, a fixed craft offset, physical plane
bounds and copied original static rectangles/approach points. It performs no
live pool lookup, lease mutation, cargo operation or movement.

The result distinguishes invalid inputs from an actual hazard. An already unsafe
offset returns first tick zero and no invented last-safe point. A safe stationary
attachment has no future hazard and retains its current point. Moving attachments
select the earliest of body exit, offset-centre map exit and static intersection.
The complete last-safe point comes from `ProjectSpaceMotionStraight`, which uses
the original carry arithmetic and refuses a horizon beyond the first outside
opportunity. The projection is also pure; even its first-outside result must not
be published into a live hull.

Static inputs are inclusive Q16 rectangles. Native footprints will be converted
from their half-open bounds by subtracting one coordinate unit from each upper
edge. Actual approach points have equal minimum and maximum. A malformed rectangle
or non-point approach refuses the complete forecast. Geometry can extend beyond
the last-centre rectangle, as a valid footprint on the final tile can do; the
craft's own map edge still limits the first unsafe opportunity.

For each positive-moving axis with initial `p`, magnitude `m`, carry `r` and day
length `D`, occupancy of `[lo,hi]` is an inclusive tick interval. Its lower bound
is zero if already at/above `lo`, otherwise `ceil(((lo-p)*D-r)/m)`. Its upper
bound is `floor(((hi-p+1)*D-1-r)/m)`. Negative axes mirror the inequalities in
64-bit coordinates without altering saved velocity. A stationary axis either
overlaps throughout the horizon or never enters. Intersect X/Y time intervals;
entering their coordinate ranges at different times is not a collision.

Work is linear in the supplied obstacle count and independent of the number of
future simulation ticks. Equal static hazards use a stable reason priority, so
reordering copied geometry does not change the result. `IsSpaceAttachmentPointClear`
exposes the same bounds/static predicate for the later admission/departure work.

One literal example starts a northbound body at `(20,18)` tiles with the original
`(1,-0.5)` craft offset. An outer-strip module covering `[21,23) x [13,15)` is not
damaged or displaced. The first unsafe attached centre is opportunity 741; the
last safe centre is `(21,15)`. The asteroid itself may continue straight through
the projected module region after the hull releases.

## Verification status

Group 10 passed its focused C++ cases together with the changed shared geometry:
244,908 assertions across seven cases, completing in under one second. Evidence:
`logs/space-fixes/attachment-safety.log`; test executable SHA256
`cc548cf3aa2ad28b03a749246907ac70e92a62213fbb182639ff9648ba1d21b2`.
They compare analytic
hazards against repeated pure steps for signed edges/corners, the outer-strip
footprint, exact approach points, unequal fractional carry, stationary and
already-invalid offsets, malformed/stale inputs and obstacle-order ties.

Run `[space-attachment-safety]` together with `[space-straight-motion]` after the
shared projection/validation change. Output belongs in
`logs/space-fixes/attachment-safety.log`. This focused combination checks the
currently changed arithmetic; it is not a full regression.

## Group 10B: bounded departure proposal

`PlanSpaceDeparture` reuses `FindSafeStaging`'s original current-point plus eight
rings/eight directions. A new optional route filter leaves existing waiting and
return callers unchanged. Equal onward distances keep the first original
candidate. The query inspects at most 65 candidates, moves no blocker, grants no
berth and never changes a hull, lease, packet, work receipt, tick or RNG.

`GetSpaceAttachmentObstacles` copies actual native footprints and current
approach slots. `GetSpaceDepartureTravelTicks` walks the original
`MoveSpaceSurveyTowards` arithmetic on copies, checking every swept segment
against those static rectangles/points. Exact rational clipping prevents an
intermediate reserved berth from being skipped between ticks. Routes are limited
to eight tiles on each axis; a conservative Manhattan-distance/truncation bound
caps evaluation at 298 steps with the current native speed. The actual longest
diagonal test takes 210 opportunities. Endpoint hull/body/staging admission
continues to use the original shared owner.

A matching actual mining lease supplies the attachment forecast. Stopped flags
do not waive physical safety. Clearance lead is the admitted travel duration
plus one pre-translation opportunity. Without a clear route, the query keeps the
original valid point and reports release due just before the next unsafe step.
`release_after_ticks == 0` is a proposal, not a committed release or saved timer;
the later native owner must revalidate current state before acting.

Focused `[space-departure-plan]` results: 716 assertions in four cases, completing
in under one second. Log: `logs/space-fixes/departure-plan.log`; executable SHA256
`63d4337172990f433216e3a64bd35765738c554f704201aa2b4175356d0352bf`.
Cases cover exact travel timing, swept footprint/berth refusal, equal-distance
corner ties, a completely crowded ring, and identical stopped/running proposals
for an actual isolated native attachment with two earned cargo units. Unit
blockers use the existing declared `ProbeExtractionRig`; these results claim no
paid procurement, save migration or live edge release.

## Group 10C: current-point body-slot admission

`GetSafeSpaceBodyApproachPoint` validates the original body centre and its fixed
`(+1,-0.5)` offset through the same plane/static predicate. `GetTargetSlots`
withholds an unsafe body slot while leaving the body, velocity, mining lease and
all original static slot definitions untouched. Both ordinary shared arbitration
and `GetSpaceApproachDestination` already use this one derived-slot producer, so
an old request cannot bypass the guard through a separate destination path.

Focused `[space-body-approach-safety],[space-approach-contract],[space-departure-plan]`
results: 785 assertions across nine cases, completing in under one second. Log:
`logs/space-fixes/body-approach-safety.log`; executable SHA256
`acb5daf56c0aa4b4e4690dca60c6d2a867adf6f5a8643473d42edbabb48aa4a8`.
Coverage includes edge/corner offsets, copied footprint/approach conflicts,
retaining a static occupant while a moving slot disappears, reordered requests,
original arbitration/transport contracts and the actual isolated native
stopped-miner/departure cases.

Extreme future body positions remain copied test geometry because the current
field still validates its old placement band. No test installs a malformed live
field to claim current-save acceptance. The connected current-point guard is
compatible with valid old in-band motion. Live lease release and body retirement
remain behind Groups 25/29; neither the new forecast nor departure proposal is
yet called by that live lifecycle.
