# R2MLVS9 - Launch Centers and Rockets

## Global Mandatory Reading

> **Execution protocol**
>
> - Read this Global Mandatory Reading section before the first task and again
>   after every context compaction.
> - Before starting or resuming a task group, read the nearest preceding Area
>   Mandatory Reading block whose `Applies to` label includes that group. Area
>   blocks appear immediately before the first group they govern.
> - A referenced heading or exported symbol is authoritative. Global navigation
>   hints and Area line ranges are for navigation only; if they drift, relocate
>   the named section or symbol rather than expanding to the whole file.
> - Mandatory reading is non-transitive. Do not preload a linked file's own
>   Mandatory Reading, Consult on Demand, or Planning Provenance entries unless
>   this plan names them directly for the current area.
> - Do not read Consult on Demand or Planning Provenance merely because those
>   sources are listed. Use a Consult on Demand source only when its trigger
>   occurs.
> - No created source file, test file or JSON file should start with same prefix as a planning MD file (such as r2). Only MD files can have that prefix. No MD file should be created anywhere in source directories unless specifically asked.
> - After EVERY CONTEXT COMPACTION, you must add a note above the task group
>   which is currently in execution. The note will say: "Context has compacted
>   X times while executing this group.". If the compaction occurs before any
>   meaningful work was done (this can happen when there is a lot of baggage
>   from the previous compaction), then and ONLY then you can skip this step.
> - When exceeding 3 context compactions, YOU MUST ask yourself if you're not 
>   chewing more than we can handle :). Heck, plans change! :). In such a case, 
>   the way out is to split the group on the spot,
>   in the plan, into more groups, 
>   by appending B, C, D as a suffixes to those groups (splitting sub-groups 
>   themselves is also allowed). When you do this, create Local Mandatory
>   Reading areas so you can optimize your context only for the
>   ACTUAL task at hand.
>   Before starting or resuming a task group, read the nearest preceding Area
>   Mandatory Reading block whose `Applies to` label includes that group. Area
>   blocks appear immediately before the first group they govern.

| File name | Line Ranges | Description |
| --- | --- | --- |
| [docs/zz-reach2/upgrades/2026-10/r2mlvs-multilayer-vehicles-and-structures.md](r2mlvs-multilayer-vehicles-and-structures.md) | 264-274 | `AD-10` supplies the canonical gameplay/ownership decision for every group of this plan. It constrains 20-tile orbital release, separate Surface hub membership and one packet owner; a consumer must not redefine it locally. |
| [src/map_layer.h](../../../../src/map_layer.h) | 78-85 | `MultiLayerMap` ownership preamble: `Map` owns packed storage/geometry and this controller owns layer semantics. Every group uses world tiles/explicit layer helpers; camera selection and ad hoc plane offsets cannot define gameplay. |
| [src/station_cmd.h](../../../../src/station_cmd.h) | 29-47 | Station construction declarations/traits own IDs, joining and facility mutation; specialized launch and underground handling reuse this public command boundary. |

Status: paused for demo at the user request,2026-10-12; native empty waterside facility, freight membership and collection UI are delivered. Handling, commissioning and launches remain pending. Parent: [R2MLVS master](r2mlvs-multilayer-vehicles-and-structures.md). Depends on r2mlvs8 and r2mlvs1 cargo definitions. Production supply chains come from r2mlvs6; development fixtures may supply inventory explicitly.

**Artifact-layout migration (2026-10-09):** this plan consumes the parent [Development artifact layout](r2mlvs-multilayer-vehicles-and-structures.md#development-artifact-layout), implemented by [R2FC](r2fc-file-cleanup.md). It supersedes earlier docs asset/review/helper placement and fixed runtime-config destinations below. New native homes use `mlm/runtime/`, generated feature reviews use `mlm/reviews/`, logs remain under `logs/`, and reusable code/curated inputs keep their code/test owners. Paths and focused native/cold verification are complete; R2FC records the results and retained-input identities. Repository-relative executable commands run from the checkout root through their context-aware wrappers. Historical pass records remain dated evidence and are not new acceptance claims. No gameplay task is completed by this migration and the separate post-demo fixture-pruning gate remains.

## Outcome and Boundaries

Build waterside Space Launch Centers through Surface airport controls; release orbital cargo anywhere within their required 20-tile drop radius and distribute recovered goods to train/lorry stations in a large Surface collection area.

## Two-area contract

| Area | Proposed/required extent | Purpose |
| --- | --- | --- |
| Surface collection area | Proposed 20-tile circular radius around the immutable center build anchor | Link same-company train and lorry stations, with their full station footprints inside the circle, to the center's cargo-handling service |
| Orbital cargo-drop zone | Required 20-tile circular radius around the anchor's projection into Space | Permit an authorized spacecraft to release a recoverable cargo pod without flying directly over the center tile |

The two areas have separate semantics/settings and overlays even when both radii are 20. Squared plane distance gives an inclusive circle: `(20,0)` and `(14,14)` are valid offsets; `(21,0)` and `(15,15)` are outside. Actual spacecraft checks use the equivalent fixed-point position calculation, not a rounded tile that admits an out-of-range craft. Clip display to map bounds without wrapping. Expanded/joined stations do not move the center anchor. Neither area grants underground catchment or a free cross-layer transfer.

Ordinary catchment and station joining do not automatically move cargo between separate Station IDs. Implement a launch-center logistics hub with explicit membership and bounded timed Surface handling jobs. Returned cargo exists in exactly one place: center inventory, a reserved handling job, a member station, or a vehicle. Separate member stations retain their own IDs and normal station-spread rules; no global catchment-limit increase is needed to simulate this relationship. All returned materials have compatible train/lorry refits and can be routed through these members.

**User revision,2026-10-12:** the
[shared Space economy contract](r2mlvs1-d-recipes-and-bootstrap.md#space-economy-revision---2026-10-12)
requires Earth launch-center commissioning for mining, survey and construction
probes initially. Carry a paid, material-built probe as a rocket payload and
activate that same native unit once. Station-bound Steel/Goods remains actual
freight, including cargo held by a launched construction probe; the first
blueprint requires no existing station. Later orbital Assembly is a separate
completed-facility gate, not an ordinary free-build path. Rocket, pod, Surface
handling and hub filters must also carry processed Steel/Goods for Earth
delivery. Goods uses Express-class crates and mass-based capacity. The revised
pod/body/material numbers supersede the old Space rows; fuel/service scheduling
and the two independent20-tile circles retain their own owners.

## Public Handoff Contract

Proposed public owner: `src/launch_center.h`. Publish `LaunchCenterAnchor`, `LaunchCollectionHub`, `SurfaceHandlingJob`, `SpaceDescentJob`, `CanReleaseOrbitalCargo`, `CommitOrbitalCargoRelease`. These symbols are producer obligations, not existing code. Keep one authority for the fields/commands they expose; downstream plans consume the public contract rather than private pools or copied tables.

## Implementation Tasks

Deepening pass, 2026-10-04: implementation groups use only SIMPLE, MEDIUM or HARD difficulty, with at most eight pending tasks each. Sketches describe proposed implementation contracts; verify the named earlier producers at each assumption gate. No gameplay or asset implementation is marked complete by this planning pass.

Groups are chronological and contain consecutive tasks of one difficulty. Each difficulty batch has at most eight tasks; area reading changes only when the implementation boundary or source of truth changes.

### Area Mandatory Reading - Surface facility and placement

_Applies to Groups 1-7._

**Entry gate (producer correction, 2026-10-12).** Surface placement consumes the
selected-profile/content owner, native Station/command/map ownership and
`src/space.h` plane-coordinate geometry. Water-edge and circle queries can be
implemented and tested before mutating a facility. Construction, joining,
removal and cargo handling require their actual native owners and focused tests.
Native fleet references consume `SpaceVehicle`/`SpaceMission` from
`src/space_vehicle.h`; rocket transition/deployment gates remain in the next
area. An unbuilt SpaceStructure or rocket transition is not a prerequisite for
checking Surface water or constructing an empty Surface facility. No preview
record or declared interface proves commissioning, descent or handling.
Verify each public file/export/schema field with `rg` in the current workspace and inspect its command/field ownership. For a same-plan contract, compile its type/signature fixture or validate its artifact after the preceding producer groups. For cross-plan behavior, also run the producer's focused executable boundary test; declarations alone cannot authorize dependent mutations or runtime claims.
If a consumed required export is absent, stop the dependent implementation, mutation and runtime claims; independent placement geometry may proceed. Read-only characterization, source/range verification, test-case design and asset-template work that does not assume the missing behavior may continue.
Do not bypass this gate by copying private pool/tile state, inventing local IDs or cargo registries, using a historical plan as a delivered implementation, opening user-wide content stores, substituting a second utility/accounting authority, or adding an unreviewed fallback path. Finish the named producer or revise the shared contract and its direct consumers first.

| File name | Line Ranges | Description |
| --- | --- | --- |
| [src/newgrf_airport.h](../../../../src/newgrf_airport.h) | 61-104 | `AirportSpec` couples layouts, year/catchment data and aircraft FSMs; menu placement must not imply rocket-flight semantics. |
| [src/water_map.h](../../../../src/water_map.h) | 145-197 | `IsWater`, `IsSea`, `IsCanal`, `IsRiver`, `IsWaterTile` distinguish navigable classes from special/coast/decorative tiles; enforce orthogonal waterside placement. |
| [src/station.cpp](../../../../src/station.cpp) | 307-363 | `GetTileCatchmentRadius` and `Station::GetCatchmentRadius` are complete ordinary mode-specific catchment calculations; launch-hub membership is a separate service. |
| [src/station_base.h](../../../../src/station_base.h) | 615-654 | `ForAllStationsAroundTiles` bounds candidates by `MAX_CATCHMENT`; do not assume a 20-tile launch hub works by enlarging every airport catchment. |
| `src/multilayer_content.h` (planned public contract; link after implementation) | Live definitions; no fixed range | [r2mlvs1-gameplay-contracts-and-content.md](r2mlvs1-gameplay-contracts-and-content.md) Groups 1-6, ending with Group 6's public-contract task, publish `MultiLayerGameplayProfile`, `LayerVehicleCapability`, `GetMultiLayerContentProfile`, `CheckDeepLayerAvailability`, `UtilityPowerAccount`. These exports/schema fields are authoritative for this area. Same-plan consumers follow those earlier groups; cross-plan consumers verify the producer's required implementation/tests as well. Proposed names alone do not prove delivery. |
| `src/space.h` (planned public contract; link after implementation) | Live definitions; no fixed range | [r2mlvs8-space-world-and-construction.md](r2mlvs8-space-world-and-construction.md) Groups 1-7, ending with Group 7's public-contract task, publish `SpaceVehicle`, `OrbitalBodyID`, `SpaceMission`, `SpaceStructure`, `SpaceTransition`. These exports/schema fields are authoritative for this area. Same-plan consumers follow those earlier groups; cross-plan consumers verify the producer's required implementation/tests as well. Proposed names alone do not prove delivery. |
| [src/water_cmd.cpp](../../../../src/water_cmd.cpp) | 1447-1472 | Complete `GetTileTrackStatus_Water` distinguishes navigability from plain-water identity; nonflat clear water is not a valid waterside connection. |
| [src/station_cmd.cpp](../../../../src/station_cmd.cpp) | 759-801 | Complete `Station::AfterStationTileSetChange` is the footprint-change boundary for hub membership revalidation. |
| [src/station.cpp](../../../../src/station.cpp) | 77-163 | Complete `Station` destructor truncates waiting cargo; settle jobs and invalidate center/member references before generic deletion. |
| [src/cargoaction.cpp](../../../../src/cargoaction.cpp) | 135-150 | Complete `CargoReservation::operator()` is physical vehicle loading with travel/reserved-counter effects, not a generic hub capacity reservation. |

**ASSUMPTION: this was already built previously. If not, re-asses at runtime**. This applies only to the producer contracts named in this area's gate, after their listed producer groups have run. Verify their actual exports and focused tests before using the sketches below; proposed types and helper names in sketches are not evidence of delivered code.

**Facility/handling sketch.** The immutable center anchor owns both range references. Station geometry may change, but normal airport catchment and station-spread values do not implement the hub's special collection service. Existing `ForAllStationsAroundTiles` searches ordinary catchment; use explicit saved membership plus full-footprint checks here.

```mermaid
flowchart LR
    Craft[Spacecraft hold] --> Descent[Saved descent job]
    Descent --> Hub[Center storage]
    Hub --> Job[Saved Surface handling job]
    Job --> Member[Linked Surface station]
    Member --> Freight[Train or lorry]
```

Each edge is one packet-ownership transfer with capacity validation. Reservation records identify quantities; they must not contain a second copy of the packets.

Context has compacted 2 times while executing this group.

### Group 1 - Place a waterside launch station through airport controls

{{HARD}}
- [X] 1.1 Add the Surface Airport-menu entry and rotated land-footprint preview, then route it to a specialized launch-station command. Menu placement does not grant ordinary aircraft rocket capability or reuse the aircraft FSM.
- [X] 1.2 Preflight every footprint tile, owner/local authority, joining StationID and construction cost before any mutation; store an immutable launch anchor separate from the mutable station bounding box.
- [X] 1.3 Define a guarded water predicate over orthogonal outer footprint edges: require real navigable clear sea/river/lake/canal water, reject coast/decorative/diagonal-only adjacency and avoid invalid map-neighbor access.
- [X] 1.4 Create the center and its orbital navigation reference atomically. The reference contains no free orbital dock, inventory or Station Core.

**1.3 producer evidence, 2026-10-12:** `src/launch_center.h/.cpp` owns the
rotated4x3 Surface footprint extent, guarded navigable-water predicate and
orthogonal perimeter query. Four rotations and all four edges pass actual-map
fixtures for flat sea, river and canal; sloped clear water, coast, diagonal/internal contact, other planes,
invalid coordinates and off-map wrapped neighbours refuse. Excluding one water
tile models the last-edge preflight and preserves an alternative valid edge.
Independent Surface-collection and orbital-drop circle queries use widened Q16
arithmetic and inclusive20-tile boundaries, including12/16 and14/14 diagonals,
a one-Q16-unit outside point and extreme signed inputs. Command:
`openttd_test.exe "[launch-center-geometry]"` (RelWithDebInfo;105 assertions).
The flat-water correction and focused results are retained in
`logs/launch-center-flat-water-release-build.log` and
`logs/launch-center-flat-water-release-test.log`.
This dated fixture publishes placement geometry. The subsequent native facility
and water-hook evidence is recorded below; launch, descent and handling remain pending.
The new footprint dimensions are an explicit initial facility default; rotation
changes extent but does not change a future immutable build anchor.

**Group1 native delivery,2026-10-12:** the airport toolbar exposes the specialized
Surface launch center and four rotated4x3/3x4 placement selections. Native
commands preflight the complete footprint, water, company/authority, joining,
spread and allocation before mutation, charge500,000 plus clearing, and create
one native Station facility with an immutable anchor and monotonic generation.
Removal costs10,000 and preserves joined rail; rebuilding cannot resolve the
retired reference. The center grants no airport FSM, free Space facility or cargo.
The initial pad drawing is temporary. World capacity is64 centers.

Save407 adds bounded staged `LCTR` transport without changing `STNN` fields.
The native lifecycle fixture passes95 assertions in RelWithDebInfo; two independent
reloads preserve `LCTR/STNN/ASTR/VEHS/CAPA/PLYR`, and ordinary `openttd` load/resave
preserves the center and station exactly. Five corrupt actual loads and four
read-only valid/corrupt/valid preview sequences reject while preserving authority.
The toolbar capture uses actual native selection state. Source/binary hashes,
logs, artifacts and exact scope are in `logs/launch-center-native-verification.json`
and `mlm/reviews/space/launch-center-verify/20261012-142808-2eb96968ba/reports/launch-center-verification.json`.
Focused geometry/identity/state/content/motion gates pass4,617 assertions/12 cases.
Debug407 and broad old-save compatibility are queued for the next batch.

**Verification batching,2026-10-12 (user instruction):** run tests and builds only
after at least two completed feature groups since the preceding cycle. The native
facility cycle above has finished. Implement the next feature batch before
starting another cycle; compiler/test fixes belong to that same cycle. Keep
unverified requirements unchecked.

### Group 2 - Preserve water access and independent service geometry

{{HARD}}
- [ ] 2.1 Track the center-to-qualifying-water relationship through explicit validation or a derived nearby-center index. Guard clear, terraform and water conversion/removal paths before they remove the last qualifying edge.
- [ ] 2.2 Allow changes that retain another valid edge; test simultaneous/area operations sequentially against updated state so two removals cannot each assume the other edge survives.
- [ ] 2.3 Define the independent Surface collection circle and Space release circle at the immutable anchor, clipped at map edges without wrapping. Keep precise rocket service/docking approaches separate.
- [ ] 2.4 Add bounded pad queue, assembly/service slots, recovery storage and ordinary Surface freight supply access. Save queue/capacity authority; a station join cannot enlarge orbital range.

**Implementation batch after the native facility cycle,2026-10-12:** Group3's
exclusive train/lorry membership and Group4's full-footprint validation, joined
storage, native selection UI and separate Surface overlay are implemented together.
`Station::launch_hub` owns up to32 separate member relationships; typed monotonic
link generations prevent reuse. Save408's bounded `LCHB` family stages these
relationships without changing `LCTR`/`STNN`. Footprint edits, deletion and settled
company ownership changes revalidate links without moving native cargo.
The generic native `IsInUse` predicate now includes the launch facility.
This is the next two-feature-group implementation batch; one combined build/test
cycle follows. Checkboxes await its evidence. Packet reservation/allocation remains
Group5.2 and the matching part of4.2. Once handling jobs exist, unlink/reselection
must refuse outstanding jobs until settlement; this producer creates no jobs.

### Group 3 - Surface facility and placement

{{MEDIUM}}
- [X] 3.1 Add same-company train/lorry collection membership within the proposed 20-tile Surface area, including joined facilities.

### Group 4 - Validate hub membership independently of catchment

{{HARD}}
- [X] 4.1 Keep separate member StationIDs and ordinary station-spread limits; require same company, Surface rail/lorry facilities and every station footprint tile within the collection circle.
- [ ] 4.2 Use explicit center selection for overlapping areas and exclusive per-packet allocation. Define whether a station may change hub while jobs exist; do not infer membership from overlapping normal catchment.
- [X] 4.3 Handle joined center/member facilities sharing one StationID as a no-self-transfer case: expose the same authoritative storage for normal pickup without a duplicate handling job or second cargo list.
- [X] 4.4 Publish a membership query used by commands and overlays, including joined center facilities and footprint changes. Test a station anchor inside with one remote tile outside.

**Native membership evidence,2026-10-12:** both configurations pass4,857 focused
assertions/14 cases and147 actual loaded-world build/UI assertions. Native rail
and lorry stations link through the synchronized command/actual station selector;
expansion from an inside station tile to an outside tile retires the link.
Overlapping-center choice, no automatic enrollment, whole-footprint checks,
foreign/other-plane refusal, bounded32-member capacity, joined one-store behavior,
unlink/reselection and native pool-ID reuse pass. No packet is copied/moved by a
membership change. `LCHB` retains exact saved relationships through two independent
reloads and ordinary resave;12 corrupt loads and9 read-only previews reject in each
configuration. All three mining snapshots pass exact two-by2,200-tick continuation
per configuration; all eight underground snapshots preserve27 owner chunks each.
`logs/launch-hub-verification.json` binds scope, binaries, source, receipts and limits.

Task4.2 remains unchecked for the per-packet allocation/active-job boundary supplied
by Group5. The next implementation batch is handling and its lifecycle, after the
user resumes development. First resolve the observed saved station-name ID
regression: `STR_SV_SPACE_NAME` was inserted ahead of `STR_SV_STNAME` in the reserved
0x6000 save-string range. Restore historical station IDs and check retained names;
do not regenerate or overwrite original retained saves to hide that defect.
The user requested a demo break after this verified batch. See the
[bundled tour](d2um-underground-mines.md); do not continue feature work during it.

### Group 5 - Reserve and move bounded Surface handling cargo

{{HARD}}
- [ ] 5.1 Define a saved `SurfaceHandlingJob` with source/target IDs, exact cargo quantity, remaining time and one packet owner. Select members by filter/demand/capacity with stable ordering.
- [ ] 5.2 Preflight destination headroom and any split-packet pool capacity; reserve one source quantity to one job only. If preparation fails, leave source packets and reservations unchanged.
- [ ] 5.3 Move actual packets from center storage into the job and then into the member station exactly once. Preserve age, source and feeder share; age job-held cargo on the simulation clock.
- [ ] 5.4 Provide a dedicated packet-owner transfer adapter. `StationCargoList::Reserve` means vehicle loading and mutates travel/reserved counters; `UpdateStationWaiting` creates new cargo. Neither can substitute for moving returned packets through a handling job.
- [ ] 5.5 Add reverse handling only for explicitly transferred rocket supplies. Charge handling once with no final-delivery income; do not use a fake vehicle or an implicit catchment transfer to make the cargo list APIs fit.

### Group 6 - Settle handling when station membership changes

{{HARD}}
- [ ] 6.1 Revalidate full-footprint range on station extension, facility removal and owner changes. Exclude same-XY Underground stations and invalidate cached membership before dispatch.
- [ ] 6.2 On unlink/removal, release not-yet-started reservations and settle in-flight packets to a valid retained owner or refuse removal until settlement. Never delete cargo just to release a link.
- [ ] 6.3 Clear/validate all references before a StationID can be reused; old jobs must not deliver to a replacement station occupying the same pool slot. Guard evacuation/removal before `Station::~Station`, which otherwise truncates waiting cargo and lacks these new job references.
- [ ] 6.4 Test two member demands for one packet, mid-job unlink, full target, station expansion across the boundary and company transfer with conservation snapshots.

### Group 7 - Publish launch, hub and release contracts

{{HARD}}
- [ ] 7.1 Publish `src/launch_center.h` exports `LaunchCenterAnchor`, `LaunchCollectionHub`, `SurfaceHandlingJob`, `SpaceDescentJob`, `CanReleaseOrbitalCargo`, `CommitOrbitalCargoRelease` with exact owned fields and invalid-reference rules.
- [ ] 7.2 Declare release validation/commit inputs now; implement orbital release in the following area. Publish tested Surface handling separately from still-pending rocket/descent behavior.
- [ ] 7.3 Register source/build entries, compile the public contract fixture and update all consumers if names change. r2mlvs10 may consume release only after its behavioral tests pass.

### Area Mandatory Reading - Launch and payload lifecycle

_Applies to Groups 8-14._

**Entry gate.** The following public contracts must exist before dependent implementation in this area: `src/launch_center.h`: `LaunchCenterAnchor`, `LaunchCollectionHub`, `SurfaceHandlingJob`, `SpaceDescentJob`, `CanReleaseOrbitalCargo`, `CommitOrbitalCargoRelease`.
Verify each public file/export/schema field with `rg` in the current workspace and inspect its command/field ownership. For a same-plan contract, compile its type/signature fixture or validate its artifact after the preceding producer groups. For cross-plan behavior, also run the producer's focused executable boundary test; declarations alone cannot authorize dependent mutations or runtime claims.
If a required export is absent, stop the dependent implementation, mutation and runtime claims in Groups 8-14. Read-only characterization, source/range verification, test-case design and asset-template work that does not assume the missing behavior may continue.
Do not bypass this gate by copying private pool/tile state, inventing local IDs or cargo registries, using a historical plan as a delivered implementation, opening user-wide content stores, substituting a second utility/accounting authority, or adding an unreviewed fallback path. Finish the named producer or revise the shared contract and its direct consumers first.

| File name | Line Ranges | Description |
| --- | --- | --- |
| [src/cargopacket.h](../../../../src/cargopacket.h) | 596-603 | `StationCargoList::Reserve`, `Load`, `Truncate`, `Reroute` provide bounded station moves and destination reservations; hub inventory has one physical owner. |
| [src/cargopacket.h](../../../../src/cargopacket.h) | 458-489 | `VehicleCargoList::Append`, `AgeCargo`, `Stage`, `Return`, `Unload`, `Shift` move accountable packets; exchange/descent/processing must not clone packet age or feeder shares. |
| [src/cargopacket.cpp](../../../../src/cargopacket.cpp) | 96-121 | `CargoPacket::Split` and `Merge` are complete packet split/merge implementations; failed allocation and feeder-share accounting must remain conservative. |
| [src/economy_base.h](../../../../src/economy_base.h) | 20-38 | `CargoPayment`, `PayTransfer`, `PayFinalDelivery` own transport income. Visiting an origin station, a descent, or handling transfer is not automatic final payment. |
| `src/launch_center.h` (planned public contract; link after implementation) | Live definitions; no fixed range | [r2mlvs9-launch-centers-and-rockets.md](r2mlvs9-launch-centers-and-rockets.md) Groups 1-7, ending with Group 7's public-contract task, publish `LaunchCenterAnchor`, `LaunchCollectionHub`, `SurfaceHandlingJob`, `SpaceDescentJob`, `CanReleaseOrbitalCargo`, `CommitOrbitalCargoRelease`. These exports/schema fields are authoritative for this area. Same-plan consumers follow those earlier groups; cross-plan consumers verify the producer's required implementation/tests as well. Proposed names alone do not prove delivery. |

**ASSUMPTION: this was already built previously. If not, re-asses at runtime**. This applies only to the producer contracts named in this area's gate, after their listed producer groups have run. Verify their actual exports and focused tests before using the sketches below; proposed types and helper names in sketches are not evidence of delivered code.

**Release geometry sketch (proposed; use r2mlvs8's selected units).** Widen before subtraction and check axis bounds before squaring. The chosen radius/coordinate bounds must prove the remaining products fit; do not use stacked world Y for this predicate.

```cpp
const int64_t dx = int64_t(craft_plane_x) - int64_t(anchor_plane_x);
const int64_t dy = int64_t(craft_plane_y) - int64_t(anchor_plane_y);
const int64_t r = int64_t{20} * int64_t(fixed_units_per_tile);
if (dx < -r || dx > r || dy < -r || dy > r) return false;
return dx * dx + dy * dy <= r * r; // inclusive circular boundary
```

**ASSUMPTION: this was already built previously. If not, re-asses at runtime**. The earlier Space producer supplies vehicle identity, transitions and approach reservations. A completed mining mission requires r2mlvs10, and delivery to operational orbital modules requires r2mlvs11; verify those contracts/tests only for their combined scenarios. Their absence does not block center handling, reusable-rocket transitions or basic carried-probe deployment. Do not fake module inventory or mark mining acceptance from a payload fixture.

### Group 8 - Define rocket model and saved launch phases

{{MEDIUM}}
- [ ] 8.1 Register the reusable cargo rocket through the native Space vehicle/engine owner. Earth hull assembly consumes its Steel/Goods once; bind purchase/service, propellant, separate dry-hull/payload/fuel mass, preparation and turnaround to the revised profile budget.
- [ ] 8.2 Define mining/survey/construction probe payloads, Steel/Goods construction stock and later advanced supplies. Retain separate hull, carried probe and cargo identities; check canonical cargo weight/classes, not a tonne per Goods crate. Ordinary aircraft are not converted.
- [ ] 8.3 Register the three minimal probe hull/engine roles and passive carry/deploy identity through the native Space owner. Initially all require material-paid Earth launch-center commissioning; an idle deployed shell needs no asteroid target. Plan10 adds role actions/orders to those same units, with later completed-Assembly commissioning owned by Plan11.
- [ ] 8.4 Save GroundLoading, Preparing, Ascent, OrbitalDelivery, ReturnRecovery and Service phases with source center, explicit destination and simulation timer. Define resumable blocked reasons for each phase.

### Group 9 - Work the reusable-rocket and persistent-probe ledger

{{MEDIUM}}
- [ ] 9.1 Calculate first purchase/launch and a second flight using the same rocket hull: fuel, preparation, service, pad occupancy and replacement cost are distinct charges from payload value.
- [ ] 9.2 Specify probe deployment as releasing the carried robot into orbital operation once. Subsequent mined cargo descents pay pod/service costs without purchasing or launching that probe again.
- [ ] 9.3 Publish revised first-mining/two-return and first-Core two-material launch fixtures from Plan1 tuning, including mass-limited Goods, exhausted holds and unavailable service. No free hull, preassembled Core bypass or duplicated cargo is allowed.

### Group 10 - Load and prepare a launch atomically

{{HARD}}
- [ ] 10.1 Reserve the pad/approach, exact propellant and payload capacity through saved launch ownership. Validate current center/company and destination before loading/preparation consumes anything.
- [ ] 10.2 Move actual cargo into the rocket hold once, with explicit packet split failure handling. Keep carried probe identity dormant: it must not tick independently while inside the payload.
- [ ] 10.3 Advance preparation from simulation ticks and debit fuel/cost at the declared single event. Cancellation before/after that event follows an explicit retained/refunded-input rule, never an accidental second charge.

### Group 11 - Deliver payload and recover the same rocket

{{HARD}}
- [ ] 11.1 Drive ascent through r2mlvs8 `SpaceTransition`, then perform one synchronized payload delivery/deployment after destination capacity or deployable-unit capacity is reserved.
- [ ] 11.2 Activate a carried probe exactly once with saved target/home reference; test save immediately before and after activation without two ticking copies.
- [ ] 11.3 Return/recover the original rocket and service it before reuse. If an orbital module destination is not yet implemented by r2mlvs11, keep that delivery integration pending while probe deployment proceeds independently.
- [ ] 11.4 Test pad contention, full hold/destination and lost target at each transition; preserve payload in its current authoritative owner until valid completion or explicit diversion.

### Group 12 - Validate release geometry and reserve recovery

{{HARD}}
- [ ] 12.1 Implement `CanReleaseOrbitalCargo` for compatible spacecraft and an explicit authorized center. Consume r2mlvs8 plane-distance predicate at the craft current fixed-point position, including the 20-tile boundary.
- [ ] 12.2 Reserve exact recovery storage/throughput and pod/service budget before cargo leaves the craft. Allocate descent record/split capacity before changing any hold.
- [ ] 12.3 Test center/foreign target, exact boundary, off-center valid point, just outside, valid/invalid diagonal and overlapping circles; every refusal keeps cargo and financial state unchanged.

### Group 13 - Commit descent with one saved packet owner

{{HARD}}
- [ ] 13.1 Implement `CommitOrbitalCargoRelease` as a revalidated atomic move of packets into `SpaceDescentJob`, recording release position, center identity, reservation and remaining simulation time.
- [ ] 13.2 Advance descent independently of the craft and render frame rate; age held cargo through the canonical schedule. Completion moves packets once into the target hub and releases the recovery reservation.
- [ ] 13.3 Persist phase/owner before and after transfer; define cancellation/target-loss settlement that retains cargo and cannot bind to a reused center StationID.
- [ ] 13.4 Charge pod recovery/replacement and servicing once at the specified event. Test save immediately around completion, full hub, deleted/retargeted center and repeated release command.

### Group 14 - Launch and payload lifecycle

{{MEDIUM}}
- [ ] 14.1 Distinguish unloading to station storage from final industry acceptance/payment. Descent itself earns no freight income and does not reset cargo age or origin.
- [ ] 14.2 Preserve all delivery history used by CargoDist/accounting.
- [ ] 14.3 Route cargo-only return missions to a valid point inside the target drop zone rather than requiring the anchor tile.
- [ ] 14.4 Select the target explicitly where orbital circles overlap; nearby foreign centers receive no automatic cargo. Service/docking orders may still need the precise facility approach.

### Area Mandatory Reading - Failures, presentation and acceptance

_Applies to Groups 15-21._

**Entry gate.** The following public contracts must exist before dependent implementation in this area: `src/launch_center.h`: `LaunchCenterAnchor`, `LaunchCollectionHub`, `SurfaceHandlingJob`, `SpaceDescentJob`, `CanReleaseOrbitalCargo`, `CommitOrbitalCargoRelease`.
Verify each public file/export/schema field with `rg` in the current workspace and inspect its command/field ownership. For a same-plan contract, compile its type/signature fixture or validate its artifact after the preceding producer groups. For cross-plan behavior, also run the producer's focused executable boundary test; declarations alone cannot authorize dependent mutations or runtime claims.
If a required export is absent, stop the dependent implementation, mutation and runtime claims in Groups 15-21. Read-only characterization, source/range verification, test-case design and asset-template work that does not assume the missing behavior may continue.
Do not bypass this gate by copying private pool/tile state, inventing local IDs or cargo registries, using a historical plan as a delivered implementation, opening user-wide content stores, substituting a second utility/accounting authority, or adding an unreviewed fallback path. Finish the named producer or revise the shared contract and its direct consumers first.

| File name | Line Ranges | Description |
| --- | --- | --- |
| [src/tests/map_layer.cpp](../../../../src/tests/map_layer.cpp) | 162-183 | Complete `Plane distances ignore the layer` test fixes projected geometry but grants no cross-layer transfer authority. |
| [src/saveload/vehicle_sl.cpp](../../../../src/saveload/vehicle_sl.cpp) | 1119-1196 | `VEHSChunkHandler` and registration construct the vehicle class from saved type before member loading; protect old enum meanings and validate new references. |
| [src/network/network_command.cpp](../../../../src/network/network_command.cpp) | 256-288 | `NetworkExecuteLocalCommandQueue` executes agreed frame order/company authority; all jobs use deterministic simulation time. |
| `src/launch_center.h` (planned public contract; link after implementation) | Live definitions; no fixed range | [r2mlvs9-launch-centers-and-rockets.md](r2mlvs9-launch-centers-and-rockets.md) Groups 1-7, ending with Group 7's public-contract task, publish `LaunchCenterAnchor`, `LaunchCollectionHub`, `SurfaceHandlingJob`, `SpaceDescentJob`, `CanReleaseOrbitalCargo`, `CommitOrbitalCargoRelease`. These exports/schema fields are authoritative for this area. Same-plan consumers follow those earlier groups; cross-plan consumers verify the producer's required implementation/tests as well. Proposed names alone do not prove delivery. |
| [Sprite Contract and delivery owner](r2mlvs13-graphics-and-asset-pipeline.md#public-handoff-contract) | Live definitions; no fixed range | `Sprite Contract Handoff` defines manifest fields `asset_id`, `consumer`, `directions`, `frames`, `anchor`, `source`; the matching authoring/conversion/runtime groups deliver actual assets. Verify the requested family and packed loading fixture before graphics integration; template/manifest publication alone is not final art. |

**ASSUMPTION: this was already built previously. If not, re-asses at runtime**. This applies only to the producer contracts named in this area's gate, after their listed producer groups have run. Verify their actual exports and focused tests before using the sketches below; proposed types and helper names in sketches are not evidence of delivered code.

**Scoped graphics gate. ASSUMPTION: this was already built previously. If not, re-asses at runtime**. Final graphics tasks in this area require the corresponding r2mlvs13 manifest entries, delivered sprites and packed loading fixture. Verify them before binding/review; if absent, stop only those art tasks and their acceptance claims, continue independent code/state tests, and never count placeholders as finished assets.

### Group 15 - Recover launch and descent failures by phase

{{HARD}}
- [ ] 15.1 Implement blocked pad, unavailable fuel/service and cancelled preparation behavior from the phase table; preserve packet reservations or return them exactly once.
- [ ] 15.2 Implement full recovery storage and unavailable orbital target as wait or explicitly authorized diversion. A rejected release keeps craft cargo; an accepted descent retains its saved owner.
- [ ] 15.3 Handle lost home center and company changes by typed-reference cleanup plus declared evacuation/reassignment. Refuse demolition when no cargo-preserving destination exists.
- [ ] 15.4 Test each phase with one failure at a time, then two competing commands in deterministic order; verify no repeated fuel fee, phantom deployment or cargo deletion.

### Group 16 - Failures, presentation and acceptance

{{MEDIUM}}
- [ ] 16.1 Refuse center demolition while cargo/vehicles/jobs would be orphaned, or complete an explicit evacuation transaction. A station ID later reused by the pool must not become an old mission's new home accidentally.
- [ ] 16.2 Integrate launch-center, rocket, probe deployment and return-pod graphics with correct layer filtering, occlusion and vehicle selection. Show payload, phase, fuel/service and queue reasons.
- [ ] 16.3 Test water predicates/orientations, all placement entry points, two launches contending for a pad and rocket ordinary-airport refusal.

### Group 17 - Compare phase persistence and payment ledgers

{{HARD}}
- [ ] 17.1 Capture a before/after save pair for every launch and descent phase, including carried probe and just-completed delivery. Compare vehicle/payload identity and next legal transition.
- [ ] 17.2 Compare source/craft/descent/hub/member inventories, packet age/provenance and reservation totals under full storage and invalid targets. Only one container owns each quantity.
- [ ] 17.3 Replay the same commands and compare normalized ledgers, with no final-delivery income for launch, descent or handling. Preserve ordinary final acceptance/payment as a separate event.

### Group 18 - Failures, presentation and acceptance

{{MEDIUM}}
- [ ] 18.1 Run an actual Surface freight delivery -> rocket payload -> Space inventory -> descent -> Surface collection loop with repository-local test content/configs.
- [ ] 18.2 Test release at the anchor, exactly 20 tiles away and just outside the fixed-point circle.
- [ ] 18.3 Test valid/invalid diagonals, map-edge clipping without wrapping and overlapping target centers.
- [ ] 18.4 Test a moving spacecraft and reordered release commands to ensure current position/target checks precede cargo mutation.

### Group 19 - Failures, presentation and acceptance

{{HARD}}
- [ ] 19.1 Recover multiple orbital cargos and collect them through separate train and lorry stations near the Surface collection boundary.
- [ ] 19.2 Test full member storage and two member demands contending for the same returned packet.
- [ ] 19.3 Test member cargo filters and deterministic allocation after demand changes.
- [ ] 19.4 Test station expansion beyond the whole-footprint range, unlink and deletion while handling is active.
- [ ] 19.5 Reject foreign, unlinked and same-XY Underground station members.
- [ ] 19.6 Verify collection membership works independently of ordinary global station-spread/catchment settings.
- [ ] 19.7 Save/reload during off-center descent and Surface handling.

### Group 20 - Failures, presentation and acceptance

{{MEDIUM}}
- [ ] 20.1 Verify total packets/age/provenance and no income generated by repeatedly relinking or routing cargo around the hub.

### Group 21 - Failures, presentation and acceptance

{{SIMPLE}}
- [ ] 21.1 Record the implemented public exports, focused test commands/results and remaining runtime/art/network gaps in this plan; retain original supplied saves and repository-local evidence.
- [ ] 21.2 Update affected architecture/handoff documentation to describe delivered behavior and reconcile the parent index only when this plan's Completion Conditions are met. Keep every unfinished task unchecked.

## Consult on Demand

| Trigger | File name | Start at | Expand only when | Consultation purpose |
| --- | --- | --- | --- | --- |
| Large Surface hub membership misses stations despite correct radius checks | [../../../../src/station.cpp](../../../../src/station.cpp) | `Station::RecomputeCatchment` | The specialized hub/member query is correct but ordinary station association is being used as a substitute | Separate launch handling membership from normal catchment/spread and layer-local industry/house association. |
| A required public export is absent and its owner/location is disputed | [r2mlvs-multilayer-vehicles-and-structures.md](r2mlvs-multilayer-vehicles-and-structures.md) | `Requirements and ownership` and `Plan Index` | The named producer confirms an intentional contract/ownership change | Find the responsible producer and reconcile its public handoff with direct consumers; the hub cannot substitute for missing implementation. |
| Current behavior contradicts a historical subsystem description | [../../architecture/map-layers/archi-map-layers.md](../../architecture/map-layers/archi-map-layers.md) | `Invariants` and the relevant named source in `Code map` | The live symbol and executable test do not explain the difference | Locate layer addressing or lifecycle rationale. Live code/tests govern delivered behavior; this architecture page does not prove future interfaces exist. |

## Planning Provenance

This is not an execution reading queue. Do not preload it after context compaction. It is non-transitive; ranges record planning use, not runtime authority.

| File name | Sections or Line Ranges used while planning | Planning contribution |
| --- | --- | --- |
| [docs/zz-reach2/architecture/initial-inventory.md](../../architecture/initial-inventory.md) | 185-220 | `Command as the only world mutation`, `Pool-indexed game objects`, `Tile map as a packed array` and `File-suffix modules` oriented command/pool/header ownership and source registration. This historical inventory is a location survey, not authority for current layer behavior. |
| [docs/zz-reach2/architecture/@archi-ttd-level0.md](../../architecture/@archi-ttd-level0.md) | 24-63 | `How the program fits together` and `Source and runtime boundaries` located shared simulation, data/presentation, CMake targets and subsystem handoffs used to choose the live reading sources. |
| [docs/zz-reach2/upgrades/2026-10/r2mlvs-multilayer-vehicles-and-structures.md](r2mlvs-multilayer-vehicles-and-structures.md) | 119-140 | `Requirements and ownership` assigned this plan's feature responsibilities; it shaped the decomposition without making every sibling plan a mandatory reading dependency. |
| [src/station_cmd.h](../../../../src/station_cmd.h) | 29-47 | The inspected live boundary helped separate characterization, public-contract production, implementation and acceptance batches. It materially constrained 20-tile orbital release, separate Surface hub membership and one packet owner; this entry records planning influence rather than an additional reading obligation. |
| [User planning-format instructions](<C:/Users/Axonn/.codex/attachments/cd59b20a-3a96-4d54-b4bd-1e8f37639e7d/Pasted text.txt>) | `Global Mandatory Reading`, `Area Mandatory Reading`, `Consult on Demand`, `Planning Provenance`, `Final validation` | Supplies the exact protocol, chronological difficulty batches of at most eight tasks, reading gates and validation format. |
| [src/cargoaction.cpp](../../../../src/cargoaction.cpp) | 135-150 | Deeper planning pass, 2026-10-04: Vehicle-loading reservations could not act as neutral hub inventory tickets; explicit packet-owner transfers were planned. |

## Completion Conditions

The airport controls build a water-adjacent launch facility whose large Surface collection area supplies separate train and lorry stations. Compatible spacecraft can drop cargo anywhere within the inclusive 20-tile orbital circle, without an exact-center rendezvous. Descent and subsequent Surface handling deliver each packet once to the intended inventory; range changes, animations and repeated saves cannot create or lose goods.
