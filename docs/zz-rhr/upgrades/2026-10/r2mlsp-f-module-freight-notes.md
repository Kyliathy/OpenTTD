# R2MLSP-F - Direct Rocket Module Freight Notes

Owner: [Space master](../../../zz-reach2/upgrades/2026-10/r2mlsp-space-master-plan.md) and [Plan9 Group11](../../../zz-reach2/upgrades/2026-10/r2mlvs9-launch-centers-and-rockets.md#group-11---deliver-payload-and-recover-the-same-rocket). Started: 2026-10-14.

**Current checkpoint:** Plan11 Groups1-6 are complete. Both configurations pass
3,605 focused assertions/26 cases;22 bounded Debug native phases and repaired
Release freight replay/resave pass. Frozen source/report/artifact identities:
`logs/orbital-handoff-debug-verification.json` (24 reports,235 artifacts).
Release module-flight build and positive verification have passed; no process
is running. `logs/module-flight-release-verification.json` binds7 reports and
the exact artifacts. [Removal/handoff notes](r2mlsp-e-removal-notes.md)
hold details. Direct rocket-to-module is now active Plan9 task11.3;11.4 has
moved immediately after it into Group11C, preserving original task order.
Task11.3 is now checked in Release;11.4 remains unchecked. Master has77 groups;
Group11B was already complete. Debug consumer joins the next substantial batch.

## Release positive handoff - 2026-10-14

Group11/task11.3 is verified:295 assertions/14 cases, actual workshop selection/
launch, physical Space entry,2STEL/1RFMT/1ELEC whole delivery and rocket15/gen1
recovery/turnaround. It packs15 real Surface fuel, reaches105 lifetime burned,
7 started/7 completed/0 cancelled. Five interrupted continuations and paused
resaves match26 owner families exactly, plus one completed ordinary resave.
Each native phase accepts valid full load and independent copied preview,
including a saved InTransit module-flight request. Workshop PNG was inspected.
Proof: `logs/module-flight-release-verification.json`; earlier422 producer
proofs remain immutable. No process is running.

Next: Group11C/task11.4, after reading its closest failure Local plus Global/
Launch Area and this delivered consumer. Add a compaction note to11C only after
meaningful work there. Batch focused all-label/fragment atomic refusal, actual
contention/fullness/lost targets by phase, cancellation with retained cargo,
malformed full-load/preview and deferred Debug consumer checks. Do not rebuild
until that substantive source/fixture batch is ready. Later acceptance remains
unchecked. Historical unbuilt snapshots below are superseded by this handoff.

Group11C implementation has started; [failure notes](r2mlsp-g-failure-notes.md)
now own its unbuilt isolated bounds and upcoming native/malformed acceptance.

## Resumed at the user request - 2026-10-14

Group11 has now compacted once. Global/Area and the closest Operational module
freight Local, parent AD10, packet/payment ownership and the verified Plan11
Public Handoff have been reread. The pause checkpoint below records the source
state at resumption; subsequent entries supersede it as implementation proceeds.
Workshop controls and focused fixtures are next; no423 build has run yet.

## Paused at the user request - 2026-10-14

Stop after the current edit for limited connectivity. No process/build/test is
running. Plan11 Groups1-6 are verified and checked; active Plan9 Group11/task11.3
and new Group11C/task11.4 remain unchecked. No423 executable has been built or
run; both existing game/test configurations are still the verified422 binaries.
The live source sections below are historical snapshots; this checkpoint is
authoritative for resumption. Group11 has not experienced a context compaction
since this implementation started. Add its first compaction note above Group11
if resuming after a compaction, then read only the relevant Global/Area and new
Operational module freight Local. Do not increment completed Plan11 groups.

**Completed source edits, unbuilt/unverified:** orbital_freight.h/.cpp and
CMake registration; transient whole-label/fragment admission and physical packet
moves; SpaceTransition module_delivered receipt, exact module-flight approach
predicate, Structure launch/Tick branch, native Space entry/shared navigation,
return/completion counting and request release. Actual safe removal calls
ForgetSpaceTransitionStructure so paused unresolved flights retain cargo and
TargetLost. Native production permits craft or module freight. Shared approach
request/gather/native validation and occupancy/staging/placement now include
exact module-delivery rockets and actual in-Space hulls, excluding carried
passengers from duplicate physical occupancy.

**Persistence edits now implemented in source:** explicit423 enum;
SPFL remains84bytes, with module receipt bit4 and older-version Structure/flag
refusals; structural/native receipt/domain/issued-generation validation and
conditional MODL digest marker. Independent SPFL preview checks same-file paid
module/owner/history, original manifest and empty successful cargo. SPOR copied
preview allows only an empty-queue InTransit rocket with its exact same-file
OrbitalDelivery/Structure request. NativeSpaceVehiclePreview now contains raw
same-file cargo_packet_refs; SlSpaceOwnerPreview reads canonical common.owner
and common.cargo.packets into copied U32 references without constructing cargo.
This new named-table ReferenceList-to-U32-vector decoding MUST be checked in
the next focused build/native preview burst; it has not compiled or run.

**Resume next:** finish source/comment audit, then workshop module destination
selection/start controls and focused isolated/native positive flight/replay/
resave fixtures. No workshop or English-string edit for module freight has
started. Plan tests before the next consolidated build. Existing423 source is
not producer evidence; keep11.3 unchecked. Group11C owns its subsequent actual
contention/fullness/lost-target and malformed loader/preview acceptance.
Reconcile any current-output wrappers actually run to423 while retaining all
immutable418/421/422 seeds and frozen proofs. Use two MSVC outer workers and
CL_MPCount2. Do not build for individual small edits or run a full regression.

High commenting remains mandatory: new source/public transition helpers have
Doxygen docs, but remaining touched flight/preview functions, compound guards
and nontrivial loops need a final business-comment audit before acceptance.
Two documentation-only role refinements in space_structure_cmd.h and
orbital_service.h were made after the completed422 build; no extra product
rebuild was done. No Git write operations were performed. Preserve the user's
unrelated untracked docs/Demo2026-06-13/.

## Required reading and actual findings

### Resumed source batch - 2026-10-14 (unbuilt)

The first consolidated build completed the game and is finishing test objects;
the new native fixture needed station_base.h for its final pad assertion.
Read-only review also caught the final arrival step's physical tile lag:
UpdateSpacePosition now runs before module packet unload, so a fractional Dock
berth crossing a tile edge records the actual unloading tile/provenance.
These related corrections are batched into one incremental game/test build
after the first pass finishes. No runtime acceptance is claimed yet.

Read-only418 decoding confirms original rocket15/generation1, six started and
completed missions, zero cancelled,90 fuel burned, and **empty onboard supply
lists**. Earlier notes'38 remaining fuel is total original fuel, not fuel still
aboard. The native fixture must pack15 actual Surface propellant through
MoveSpaceSupply before departure; it returns with0 onboard fuel and105 burned.
This fixture-only correction was included before the running consolidated
build reaches the native test source. No fuel is fabricated or privately moved.

Workshop now has explicit Next orbital destination/Launch to module controls,
using current owned operational Core/Storage/Dock and exact issued generation.
Paint retains a stale selection rather than silently retargeting it. Native
command quotes own departure admission. Generic flight status now names the
destination; the separate craft-resupply control remains intact.

New orbital_freight.cpp detached tests bind separate423 receipt/domain/phase
and shared physical priority. orbital_freight_native.cpp imports only the genuine
paid418 seed, packs actual2STEL/1RFMT/1ELEC, launches through actual workshop
controls, saves preburn/ascent/delivery/admission/unloaded/turnaround, and checks
the original hull's15-fuel round trip and no duplicate material consumption.
Python wrappers orbital_freight_checks-1/-2/-3 split preparation, completion and
each interrupted/paused26-family continuation into separate bounded bursts.
New sources and producer metadata are registered. Python syntax/catalogue and
diff whitespace checks pass; no runtime claim yet. Consolidated Release build
is next, with two outer workers/CL_MPCount2. Keep11.3 unchecked until positive
native persistence evidence passes;11C still owns later negative acceptance.

Plan9 Global, Launch/payload Area and closest Connected logistics Local were
read; its Local applies to older2C/8C/10B/7B, so a focused Operational module
freight Local was added immediately above11. Read that after compaction, with
Global/Area and Plan11's verified Public Handoff/proof. Group11C has its own
failure Local. Parent AD10, station command boundary, packet/cache ownership,
no-payment boundary, actual transition/manifest/approach/module and copied
save/preview owners have been inspected. Do not preload later descent/art.

`space.h` is geometry; actual owners are SpaceVehicle, SpaceTransition,
SpaceStructure and contained OrbitalModule. Existing flight supports only
Body/LaunchCenter deployment and generation-qualified Craft resupply.
ResolveDestination, Begin/Tick/record/native/copied validation all exclude a
Structure destination. Rocket placement is InTransit while flying, with
GetSpaceVehicleLayer supplying Surface/Space; every shared approach domain
check currently assumes Orbital placement. Extending admission requires
consistent native, production, SPOR and SPFL preview ownership, including
actual rocket occupancy/staging. No second berth registry or station proxy.

SPFL rows are84bytes. Existing flag byte uses bits0-3; schema423 can add one
module-delivered receipt in bit4 without changing row framing. This is proposed,
not implemented. Older versions must refuse Structure flights/new flags.
SPOR420 optional request framing and SPST422 header/rows can remain intact.
A rocket's empty typed order queue may own a flight-driven request, but only
for its exact saved OrbitalDelivery/Structure target in Space. Release that
request on return/cancellation/target loss. Validators must agree independently.

## Proposed source batch (not implemented or verified)

- Publish a focused orbital_freight.h/.cpp participant using actual module
  queries and HubCargoTransfer, with pure GetOrbitalModuleFreightBlock and
  whole CommitOrbitalModuleFreight. Aggregate freight plus actual RFMT/ELEC
  compartments by physical label; preflight1024 units/64 fragments per label,
  whole source availability/reservations, exact owner/generation and actual
  arrival berth. Retain propellant for the existing recovery burn. No passenger
  or paid pod cargo is silently reinterpreted as module freight. No useful
  input means no delivery. Failed later labels preserve every earlier label.
- Extend native flight with explicit module destination/once-only receipt;
  an arrival must navigate from actual Space entry through shared staging/
  admission, rather than appear at an occupied module approach. Quote destination
  capacity without acquiring a future berth; full/outage/lost targets retain
  actual freight until delivery or explicit cancelled recovery. Preserve
  current probe deployment/resupply, fuel10/5, mission fee and pad custody.
- Extend every actual shared approach domain/occupancy/staging check for only
  exact in-Space module-delivery rockets; update independent same-file preview
  and native production validation. An inert module/foreign or reused numeric
  target grants no operation. No extra power ledger: existing module freight
  handling has no electricity debit; dependency loss blocks handling, zero
  panel generation blocks service work.
- Add synchronized workshop destination selection/start controls using the
  actual command quote and generation-qualified module reference.
- Bundle focused isolated boundaries, genuine paid direct-flight/replay/resave
  phases, schema/receipt/owner refusals and affected parent approach regressions
  before one consolidated build. Two MSVC outer workers/CL_MPCount2 only.
  Current-output wrappers used again will need423 output assertions/names;
  immutable418/421/422 seeds and completed proof files must remain unchanged.

**Useful paid input:** genuine `space-module-frame-418.sav` has an operational
Core and paid Storage frame with two paid builders initially at that Storage.
It predates Dock's two additional flights and retains38 of128 original fuel
(rocket burned90), enough for a new15-fuel direct flight. The frozen421 Dock
world has burned120 and only8 remain; do not use it for extra flights or invent
fuel. Native positive integration can import actual STEL and RFMT/ELEC into
existing Core stock and recover the exact original rocket. Every partial
checkpoint must declare its exact predecessor identity, never latest lookup.

No source change for module flight has been made; no423 build/test has run.
Public role wording in space_structure_cmd.h/orbital_service.h was corrected
as documentation only after the completed422 build; signatures/bodies match
its compiled fixtures. Carry those comment-only refinements into the next
substantial source batch, with no separate product rebuild.

## Live source checkpoint (unbuilt, unverified)

Created orbital_freight.h/.cpp and registered them in src/CMakeLists.txt.
The transient admission combines actual freight and RFMT/ELEC labels, checks
whole availability and1024/64 bounds, and transfers original packets with no
propellant/material/money/utility mutation. Commit still requires the exact
InTransit module-flight arrival and shared physical berth.

SpaceTransition now has proposed module_delivered and pure
IsSpaceModuleFreightApproach. Begin/Tick support a Structure branch, actual
Space entry at departure plus shared navigation, once-only transfer/return,
and module completion counting. StartReturn/Reset release only the original
request. ForgetSpaceTransitionStructure is called by actual safe removal so
a paused saved unresolved flight is already TargetLost with payload aboard.
Production recognizes either freight destination. Shared approach native
domain, gathering, occupancy/staging/site-placement checks now include exact
module flights and actual Space rockets, excluding carried passengers from
second physical occupancy. Existing record/save/preview validators still need
reconciliation before any build or runtime claim. Public transition header
and implementation helpers now have explicit documentation.

**Next required work:** finish full/native transition record receipt/domain
validation and digest;423 SPFL flag bit4/old-schema gate (row remains84bytes);
SPOR copied-preview exact flight ownership; native VEHS copied cargo-reference
preview to refuse a completed module receipt retaining freight. Native preview
currently has only ID/type/owner/position. SlSpaceOwnerPreview in vehicle_sl.cpp
reads common.owner; canonical common.cargo.packets is a saved U32 reference
list, allowing copied raw references without borrowing live cargo. Then add
workshop module selection/start controls and focused genuine418 native
positive/replay/resave plus isolated/refusal fixtures. Reconcile only the
current-output423 wrappers that are run; immutable prior proof inputs stay
unchanged. No module-flight source has compiled; do not check11.3.
