# R2MLSP-C - Shared Approach Execution Notes

Owner: [Space master](../../../zz-reach2/upgrades/2026-10/r2mlsp-space-master-plan.md), [Plan11 Group4B](../../../zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md#group-4b---deliver-shared-approach-reservations) and its [Plan8 Group11 prerequisite](../../../zz-reach2/upgrades/2026-10/r2mlvs8-space-world-and-construction.md#group-11---allocate-dock-and-approach-slots-deterministically). Resumed at the user's request,2026-10-13. Implementation notes are not acceptance evidence.

**Verified checkpoint,2026-10-14:** source Plan8 tasks11.1-11.3, Plan11 task4B.1
and master Group4B are checked. Both RelWithDebInfo targets build; focused
contracts pass3,369 assertions in16 cases. Static/body queued interruption and
ordinary resave each compare26 owner families. Four malformed cases fail both
full load and independent preview with live authority unchanged. Evidence:
`logs/space-approach-verification.json`. No build or test remains running.
Debug is deferred to the next substantial batch. Continue at Group4C using the
[Dock design findings](r2mlsp-d-dock-design-notes.md). Pending/unverified/live
statements below are historical snapshots superseded by this checkpoint.

## Reading and scope

Plan11 Global, Delivered construction Area, Module construction after Core
commissioning Local and Shared approach prerequisite Local reading have been
read. Plan8 Global and Orbital world and construction Area reading also apply
while implementing the shared producer. Relevant source sections: parent AD9
and AD12, map-layer ownership, persisted VehicleType ordering, TileBase/Extended,
Randomizer simulation versus interactive state, plane-distance test, command
estimate/execute, canonical construction definitions, actual site geometry,
native job/stock/service owners and shared utility publication. After compaction,
re-read only these applicable sections and current source boundaries. Stale
navigation remains: `space.h` is geometry/preview, not the native fleet owner.

The producer is being implemented and tested before Dock mutation, as the
source gate requires. No new group is checked. Group4B has not compacted yet.
The newly noticed untracked `docs/Demo2026-06-13/` belongs to the user and is
unrelated; do not edit it.

## Current implementation

`space_approach.h/.cpp` publishes pure deterministic arbitration and native
request/admission/release queries. Each actual hull contains at most one
optional `SpaceApproachRequest` in its existing `SpaceOrderQueue`; there is no
parallel hull or berth pool. Requests save normalized Body/Structure target,
generation, request tick, bounded slot or waiting sentinel, and a safe staging
destination. Admission preserves surviving grants, then sorts waiting requests
by original request tick/native ID and chooses the lowest free physical slot.
Existing body mining leases and actual occupants block admission without being
rewritten. Waiting uses actual native movement, freezes action deadlines and
earns no work. Target loss clears only affected claims. Blueprint placement
cannot displace native hulls or retained staging claims.

Version420 extends the existing SPOR queue transport with one presence byte
and30 request bytes when present. Old queues migrate without a request or
implicit slot. Same-file clock/geometry, target ownership and duplicate grants
are checked by full load and copied preview. The existing26-family observer
includes this authority through SPOR. Core/inert-site approaches remain one;
a future operational Dock supplies two precise berths within its already paid
south approach cell, preserving old inert blueprint geometry.

Dock activation/services, utility connections, removal and final handoff have
not been implemented by this producer. These remain Group4C/5/6. Do not claim
Dock capacity based on its conditional query alone.

## Focused validation queued

`src/tests/space_approach.cpp` contains pure arbitration/transport contracts and
an ordinary loaded-world observer. The native observer uses actual paid craft
from the unchanged418 module frame, queues two simultaneous visits to Core0,
saves while queued, requires actual departure before the second craft reaches
the berth, then places/cancels another paid native blueprint and checks target
loss. No test hull, packet or position is injected.

Run `space_approach_checks-1.py` for exact interrupted/ordinary resave comparison;
pass its exact report to `-2.py` and `-3.py` for two malformed cases each:
duplicate slot, stale target generation, future priority and staging bounds.
Each case requires full-load and independent preview refusal with live owners,
cash and RNG unchanged. Both wrappers accept `--configuration Debug`.

**Live build,2026-10-14:** consolidated RelWithDebInfo build session65903 writes
`logs/space-approach-release-build.log`. The game target has built and the test
target is still compiling; no producer tests have run yet. All three Python
wrappers import and their four explicit producer entries are registered.
Review found and fixed the inspector's missing approach-wait label, dynamic
staging safety, native owner admission, common pre-movement request gathering,
mining-command priority preservation, and construction/survey work admission.
Several fixes were made after their objects compiled; once this live build
finishes, perform one incremental game/test rebuild to include that complete
review batch. No header changed in that correction batch. Do not restart the
broad build or run a second concurrent build. Keep two outer workers and
`CL_MPCount=2`. CMake's full path and standing
focused-build rules are in the flight handover. The user requested bundling
costly builds. Keep tests under two minutes, polling live tests at least50s.
After the producer passes, record exact evidence and complete its source tasks,
then continue the Dock consumer. Group4 Debug activation checks remain queued
for the next substantial batch; its historical wrappers still pin current
output419 and require reconciliation when actively rerun on420+ binaries.

The paid418 fixture contains two genuine builders, including one retained at
the Core after service. The native contention observer now vacates that berth
through real orders before testing simultaneous free-slot priority. Do not
teleport either craft or count existing physical occupancy as a priority bug.
The refusal parser selects only the two Core0 requests; unrelated retained
body requests keep their own owners. Native source/script tests remain pending.

**First runtime result,2026-10-14:** both Release targets and the focused
contracts pass3,369 assertions in16 cases. The native preparation passes457
assertions for actual contention/departure/cancellation. Queued reload exposed
an exact-length error: target fields occupy13 bytes, so a request is30 bytes,
not31. The decoder/writer use one explicit field-width sum; the independent
corruption parser is corrected too. Run an incremental native/test rebuild,
then rerun the exact acceptance chain. The initial failed generated checkpoint
is not accepted evidence; retained paid418 input remains unchanged. No group
has been checked and Debug remains deferred.
