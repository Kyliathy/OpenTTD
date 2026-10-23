# R2MLVS10 - Asteroids and Mining Probes

**Current recovery contract,2026-10-22.** Use the [current Space handover](r2mlsp-z-z-z-z-z-q-launch-center-source.md#current-boundary) and [shared artifact architecture](../../architecture/archi-historical-debug.md#checkpoints-cleanup-and-transfer) before continuing. The exact original save pair and compact foundation are durable; current native qualification, frames, reports and runtime homes are generated. Dated log/EXE paths below are historical facts. Select actual prerequisites through the [producer/recipe catalogue](../../../../mlm/review-producers.json); an unresolved recipe or missing original input remains a refusal. Cleanup qualification completes no gameplay or human-acceptance checkbox.

## Global Mandatory Reading

> Current recovery: read the [live dispatch](r2mlsp-z-z-z-z-z-q-launch-center-source.md#current-boundary) and only its selected [durable-input/rebuild contract](../../architecture/archi-historical-debug.md#checkpoints-cleanup-and-transfer); older execution paths in completed groups remain historical.

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
> - User execution correction,2026-10-12: batch related feature changes and
>   fixture fixes before building. Builds take time and block the workspace.
>   Do not rebuild after each small edit; use a consolidated build/validation
>   pass when the next substantial batch is ready. Carry remaining fixture-only
>   corrections into that batch instead of repeatedly rebuilding this one.
> - User compiler limit,2026-10-13: keep tools launching MSVC `CL.exe` at two
>   outer build workers, including on battery. Keep nested compiler parallelism
>   bounded (`--parallel 2` and `CL_MPCount=2`); do not increase it to exploit
>   all16 cores. An allowance of eight workers applies only to a different compiler.
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
| [docs/zz-reach2/upgrades/2026-10/r2mlvs-multilayer-vehicles-and-structures.md](r2mlvs-multilayer-vehicles-and-structures.md) | 276-280 | `AD-11` supplies the canonical gameplay/ownership decision for every group of this plan. It constrains fixed-point moving targets, finite reserves and conserved probe cargo; a consumer must not redefine it locally. |
| [src/map_layer.h](../../../../src/map_layer.h) | 78-85 | `MultiLayerMap` ownership preamble: `Map` owns packed storage/geometry and this controller owns layer semantics. Every group uses world tiles/explicit layer helpers; camera selection and ad hoc plane offsets cannot define gameplay. |
| [src/vehicle_func.h](../../../../src/vehicle_func.h) | 192-204 | `VehicleServiceInDepot`, `CallVehicleTicks`, `ResetVehicleHash`, `ViewportAddVehicles` are public tick/service/spatial-render boundaries for orbital vehicles. |

Status: complete,2026-10-17. All source groups and Completion Conditions are accepted: dated438 gameplay/runtime evidence plus final authored/packed/native/ordinary-game Space art in `logs/space-art-final-verification.json`. Plan11's broader presentation/save/network/end-to-end and Plan9's deferred launch/fleet art remain separate. Parent: [R2MLVS master](r2mlvs-multilayer-vehicles-and-structures.md). Depends on r2mlvs9.

## Current dispatch - Full-plan completion, 2026-10-12

**Historical checkpoint,2026-10-13; subsequently resumed.** The user requested stopping after the
current task. Plan8 Group13B and Plan11's physical construction-job owner now
support native Core blueprint placement, actual Earth-builder Steel/Goods
delivery, saved work, cancellation/collection and a finished frame. Verification
is in `logs/space-construction-verification.json`: RelWithDebInfo804 contract
assertions/23 cases, native interrupted/lifecycle/conservation checks, twelve
full/preview corruption cases and genuine414-to415 migration. Debug remains
for the next substantial batch. First-Core activation, operational modules,
processing/reuse, final art and full network acceptance remain open. Resume
with the Core activation producer only after a new continuation request.

**Resumed objective, 2026-10-12:** finalize both Plan9 and this plan. The recorded
demo pause is superseded. The saved-name correction and Plan9 Groups5/6
packet handling/lifecycle are delivered in both builds; the remaining launch,
mining, survey, construction, service, graphics and acceptance scope stays active.

The active objective is to finalize this entire plan. P1/P2 are delivered
increments, not completion of the original Groups1-22. Group2.1's saved
population control and Plan9's native empty Surface launch center are delivered.
Native Surface membership and timed handling are delivered. Next producers are
paid rocket commissioning and remaining service/pad/recovery producers. Plan1
Group12B's Space definitions and Plan8 Group6B's native typed queue foundation
are verified in both builds; generation/motion evidence and deployment/drop/descent
continue toward the full loop. The user's latest revision also requires
Groups8B/8C: survey and construction roles, ordered missions and survey-gated
resource disclosure, followed by Steel/Goods orbital construction and reuse.
Paid service/repeat, final art, recovery, network replay and useful Surface
delivery remain required. Underground work stays paused. Every unfinished
original task remains unchecked; the added requirements are pending too.

**Plan9 producer progress,2026-10-13; partial.** The connected launch logistics
batch now has RelWithDebInfo native evidence for real supplies, paid passive
pods, interrupted service and reusable rocket deployment/recovery, alongside
passing assembly/order/handling/center regressions. Descent stopped at the
foreign-center test-site setup before release. Its paid-canal fixture correction
and an active-service capture await the next substantial feature batch; Debug
remains deferred. `logs/launch-logistics-verification.json` binds the tested
binaries/reports and identifies those uncompiled fixture changes. This does not
complete the role actions, orbital resupply or either full plan.

**Historical demo break,2026-10-12 (superseded):** the Plan9 native
facility/membership/UI batch was verified in both builds
(`logs/launch-hub-verification.json`) before the user paused implementation. The [bundled underground/Space walkthrough](d2um-underground-mines.md)
provides mining, full-hold and `space-hub` checkpoints. Handling and its lifecycle
are now delivered; descent, Earth commissioning, survey/construction roles, paid
service/repeat and the remaining original requirements stay pending. Retain the user instruction
to batch at least two feature groups between builds/tests.

### Area Mandatory Reading - Moving field preview

_Applies to GroupP1 only._ Read this Global block, parent AD-9/AD-11 and
I-3/I-5/I-7/I-9, Plan8 GroupP1 and its shared Group10 geometry defaults,
`map_layer.h` ownership preamble, `core/random_func.hpp` randomizers,
`timer_game_calendar.h` technology clock, and this plan's Groups1-3 tables.
Verify the existing profile/content and state-foundation boundary tests. The
field consumes actual `OrbitalBodyID`/`SpaceMotion` exports and focused motion
tests from Plan8P1; do not invent a second coordinate or resource authority.

### Group P1 - Populate and show the first moving Space field

{{HARD}}

- [X] P1.1 Implement the three canonical classes, sizes, finite reserve sampling and exact population table from Groups1-2 with independent content/placement/motion RNG streams. Normal is the preview default; a player population dropdown remains Group2.1.
- [X] P1.2 Generate selected-profile new worlds only, using the shared band and four-tile grid, stable class IDs0/1/2 for starter targets, bounded capacity checks and no reload/depletion respawning. Persist generation inputs and all generated records.
- [X] P1.3 Bind the Group3 speed/heading/fraction tables to the shared synchronized motion producer. Verify exact counts/classes/reserves, seed repeatability, maximum field size, boundaries and hidden/save-reload continuation.
- [X] P1.4 Prepare predeployed model15 survey probes through Plan8's preview controller and a player walkthrough. Demonstrate moving-target pursuit, exclusive attachment and repeated departure without claiming extraction, paid launches, return pods, service or Surface delivery.

**Delivered preview,2026-10-12:** the content catalogue publishes the exact13
population rows and three cargo/reserve definitions. `asteroid.cpp` generates
one field in new Multilayer Industry worlds, with independent RNG domains and
central starter classes0/1/2; actual waterside launch access remains Group2.2.
`space.cpp` supplies the shared integer band/reflection producer; survey work
cannot debit reserves or create packets. The real new-world hook produces10
bodies/no probes for Normal64x64 and an empty field for Classic. Both featured
1990 checkpoints preserve exact orbital state through ordinary load/resave and
2,000 native ticks in both RelWithDebInfo and Debug, including hidden Space and
saved attachment. Maximum14,336-body generation and clustered index movement
pass the focused fixtures. Native and ordinary graphical evidence is under
`mlm/reviews/space/`; retained inputs are hash-bound in
`src/tests/fixtures/demos/space/demo-checkpoints.json` and results in
`logs/space-preview-verification.json`. Start with `play.ps1 space-survey`; see the
[completed-feature list and controls](d3sp-space-survey.md).

The preview does not satisfy Groups8-22's mining/economic acceptance. Reserves
remain unchanged by survey work. Final authored/packed Plan13 space families and
native company fleet integration retain their separate delivery gates.

### Area Mandatory Reading - Native orbital mining increment

_Applies to Group P2 only._ This staged producer revision follows the user's
request to continue functionality after P1. Read the Global section, parent
AD-9/AD-11, the delivered P1 exports, and
[native compatibility matrix](../../../zz-rhr/upgrades/2026-10/space-vehicle-compatibility.md). Entry producers:
state/content focused gates, canonical Vehicle/Engine/EIDS/CAPA ownership,
`space.h` Q16 geometry and the one `AsteroidField`. Consume actual native owners;
no second fleet, engine or cargo registry is allowed. This increment exports
`SpaceVehicle`, `SpaceMission`, `CmdSetSpaceMission` and finite body leases.
Explicit test/demo setup retires survey records and debits native purchases;
it is not rocket commissioning. Cargo remains in the vehicle at orbital home.
Missing launch-center/structure/descent producers stay outside P2; the original
full-fleet/full-loop entry gates and checkboxes remain in force.

Context has compacted 1 times while executing this group.

### Group P2 - Native owned probes and finite orbital extraction

{{HARD}}

- [X] P2.1 Preserve historical vehicle bytes and fleet window identities; add explicit dense company indexing, canonical Space engine/class and versioned VEHS/ASTR/CAPA persistence. Reject unsupported ordinary purchase/order/clone/script/NewGRF paths.
- [X] P2.2 Pursue moving bodies with a frozen adaptive timeout, lease one slot in native VehicleID order, mine 2 tonnes per 74 working ticks into a 20-tonne canonical packet hold, debit finite reserves and native company operating costs, and return with cargo intact.
- [X] P2.3 Provide synchronized owner-validated assign/recall commands and a native Space inspector. Prepare purchased fleet snapshots; identify retained cargo and pending launch/descent/service work in the walkthrough.
- [X] P2.4 Verify actual native contention, all resource classes, conservation/full-hold refusal/fleet bounds, cancellation/retargeting, costs/cargo age, paused/hidden ticks, old-save compatibility and exact reload continuation in both build configurations.

**Delivered native mining handoff, 2026-10-12:** `SpaceVehicle` is saved native
vehicle type6, with dense company slot4 and canonical Engine/EIDS, group/unit,
operating-account and VehicleCargoList/CAPA owners. Historical bytes0..5 and
old fleet-window numbers retain their meanings. `SpaceMission` and
`CmdSetSpaceMission` provide owner-validated moving-target pursuit, bounded
approach, exclusive native-ID leases, partial extraction and recall/retarget.
The field ticks before native vehicles; each extraction credits packets before
debiting finite reserves. Version405 saves body leases, native mission/hold
state and Asteroid provenance. Load validation rejects orphan leases, duplicate
packet ownership, legacy-byte aliases and fleets above the64-probe bound.

Focused state/content/motion/index tests pass 3,818 assertions in nine cases in
RelWithDebInfo and Debug. All three checkpoints pass ordinary exact orbital
resave and two independent 2,200-tick native continuations per configuration;
ASTR/VEHS/CAPA/PLYR outputs match exactly. Native setup tests in both builds
exercise paid purchases, simultaneous arrival, loaded recall, incompatible
retarget refusal, resumed mining, costs, ageing and reserve/hold conservation.
Paused and hidden-view checks pass. All three corrupt-save cases reject in both
builds; all eight underground demos preserve27 native owner chunks each.
Ordinary graphical inspector captures pass for approach, partial work and full
holds in both builds. Older404 survey and new-world population checks remain
supported. See `src/tests/fixtures/demos/space/mining-checkpoints.json`,
`logs/space-mining-verification.json` and the [walkthrough](d3sp-space-survey.md).

This handoff exports native orbital mining only. The explicit demo purchase
retires preview controllers and charges120,000 for three probes; it does not
commission rockets. Full20-tonne holds wait at orbital staging. The next producer
is Plan9's Surface launch center and20-tile circular drop zone, then saved
cargo descent and Surface handling. No delivery income, automatic repeat,
service producer, free Surface facility or final space-art completion is claimed.
Original full-fleet/full-loop groups retain their own pending acceptance gates.

**Artifact-layout migration (2026-10-09):** this plan consumes the parent [Development artifact layout](r2mlvs-multilayer-vehicles-and-structures.md#development-artifact-layout), implemented by [R2FC](r2fc-file-cleanup.md). It supersedes earlier docs asset/review/helper placement and fixed runtime-config destinations below. New native homes use `mlm/runtime/`, generated feature reviews use `mlm/reviews/`, logs remain under `logs/`, and reusable code/curated inputs keep their code/test owners. Paths and focused native/cold verification are complete; R2FC records the results and retained-input identities. Repository-relative executable commands run from the checkout root through their context-aware wrappers. Historical pass records remain dated evidence and are not new acceptance claims. No gameplay task is completed by this migration and the separate post-demo fixture-pruning gate remains.

## Outcome and Boundaries

**Delivered definition/queue handoff,2026-10-12:** Plan1 Group12B publishes
`multilayer_space_content.h` revision2 recipes, mass, budgets and paid-facility
admission; Plan8 Group6B publishes native `space_orders.h` queue/generation
ownership. Save410 carries exact `SDEF` content identity and separate bounded
`SPOR` transport, preserving ground/profile/package1 and paid old objects.
Both builds and focused native save/replay/refusal gates pass; see
`logs/space-content-orders-verification.json`. These handoffs grant no missing
commissioning, survey, blueprint, processing, descent, service, UI/art or network
producer. Consume their public definitions/commands; retain unfinished tasks.

Slowly moving asteroids are finite deposits. Earth-launched mining, survey and
construction probes are native vehicles with orders. Survey completion reveals
resource quantities to the owning company; mining converts finite reserves into
held cargo. Returns use the home center's20-tile drop region or a completed
orbital station. Steel/Goods may arrive from Earth or be produced by constructed
orbital processors, then be reused for infrastructure/probes or dropped to Earth.

**User revision,2026-10-12:** the canonical
[Space economy contract](../../../zz-rhr/upgrades/2026-10/r2mlvs1-d-recipes-and-bootstrap.md#space-economy-revision---2026-10-12)
and [revised numerical defaults](../../../zz-rhr/upgrades/2026-10/r2mlvs1-e-ground-and-space-budgets.md#space-economy-revision---2026-10-12)
supersede the old material/bootstrap rows. Plan9 owns initial Earth commissioning;
Plan8 owns native typed order/vehicle/blueprint identity; this plan owns probe
actions and survey disclosure; Plan11 owns delivered construction, orbital
processing and later local assembly. Every new build needs both Steel and Goods.
Placing a Fund Industry-style blueprint only reserves a site. The initial Core
is built by construction probes with delivered materials, not a free orbital
reference or preassembled Core bypass. Existing P1/P2 evidence proves neither
these additions nor completed scans: preview controllers have no company owner.

## Public Handoff Contract

Public owners: [src/asteroid.h](../../../../src/asteroid.h) supplies `AsteroidBody`, `AsteroidField`, `GenerateAsteroidField`, `AdvanceAsteroidField` and the sole live-field accessors. [src/space_vehicle.h](../../../../src/space_vehicle.h) supplies the existing native `SpaceVehicle`, `SpaceMission`, `AssignSpaceMiningMission` and `SpaceVehicle::Tick` controller. There is no alternate body/mission registry or renamed local substitute. Groups1-7 publish generation/body geometry; later consumers retain their separate role, paid launch/service/descent, manufacturing and full mission gates.

| Public owner / fields | Authority and boundary |
| --- | --- |
| `AsteroidBody::{id, resource_class, size, initial_reserve, remaining_reserve}` | This world's stable vector-ID record. One class maps to one canonical cargo through the selected profile's resource definitions; reserves are whole tonnes, one tonne per cargo unit. Zero remains a live depleted identity. No in-play removal/replacement API exists. |
| `AsteroidBody::motion` / [SpaceMotion](../../../../src/space.h) | Shared authoritative Q16 `position`, signed per-day `velocity_x/y` and unsigned `remainder_x/y`0..73. Fractional travel supplies the saved motion phase; there is no duplicate body mission-phase field. |
| `AsteroidBody::{mining_slot,survey_slot}` | One actual native mining lease or explicitly ownerless observation lease. Full fleet acceptance validates reverse native ownership; a stored target or structural slot ID alone grants no lease. |
| `AsteroidField::{generation_version,width,height,seed,density,band,elapsed_ticks,starter_input,bodies}` | Sole persistent generated inputs and actual list. Version1, exact selected population, shared band and frozen initial access input persist in ASTR424. Legacy input is unknown, never reconstructed. Loads keep actual positions/velocities/reserves; current new-game settings cannot reroll them. |
| `GetAsteroidField`, `GetMutableAsteroidBody(OrbitalBodyID)` | Const full-field view and internal owner lookup. Sentinel/out-of-range IDs return null; no tile number or reused static structure ID aliases a natural target. Quantity presentation still consumes the survey disclosure gate. |
| `GenerateAsteroidField(width,height,seed,density,optional starter)` | Pure feature-local generator; no Surface/interactive RNG draw. Unsupported geometry/input refuses. Explicit world generation obtains actual Surface site geometry first. `InstallAsteroidField` validates complete staged records and rebuilds disposable buckets; it is a world/load boundary, not a runtime respawn command. |
| `AdvanceAsteroidField`, `TickAsteroidField`, `UpdateAttachedSpaceCraftTransforms` | One synchronized body step in stable ID order, then current buckets and actual matching native attachment transforms before craft decisions. Pure fixtures may advance copied records; viewport reads cannot call this as presentation work. |
| [Asteroid catalogue](../../../../src/multilayer_asteroid_catalogue.h), selected profile resource/size spans and `space_tuning` | Immutable population/share/placement/heading/speed definitions and shared geometry/clock. Classic has no orbital definitions. Version changed simulation definitions through the profile/content contract; algorithm changes also version the generator. Saved records remain authoritative. |
| `AsteroidField::knowledge` / `GetAsteroidQuantityDisclosure` | Sparse native scan receipts persist separately in SPKN. Ownerless previews grant no quantity disclosure or resource ownership. Later survey/controller tasks retain their independent acceptance gates. |
| `AssignSpaceMiningMission(SpaceVehicle &, uint32_t body, bool execute)` | Native hull/role/owner/resource/lease admission. `execute=false` estimates without changing target, work, packets or money; commands validate the actual asking owner. The explicit invalid-body mining-command sentinel means recall, while a typed invalid natural navigation target reports `TargetRemoved`. |
| [CmdSetSpaceMissionChecked](../../../../src/space_vehicle_cmd.h) `(flags, VehicleID, uint64_t generation, uint32_t body)` | Appended synchronized assignment/Recall command checks the original hull incarnation again at agreed execution. The GUI uses it. Historical generation-free arguments/IDs remain offline-compatible and refuse multiplayer; no save-schema change. |
| [GetSpaceMiningTuning](../../../../src/space_mining_tuning.h), [CommitSpaceMiningExtraction](../../../../src/space_mining_inventory.h) | Immutable15/21 operating quantities and the atomic original native body/hold/CargoPacket participant. Complete ready work, exact lease, resource, whole capacity and actual packet allocation precede finite debit/output; refusal grants no capacity or material. |
| [GetSpaceMiningDepartureDestination](../../../../src/space_departure.h) | Pure clear homeward exit through original shared staging/occupancy. Recall keeps the last body as nonactionable departure/history, preserving emitted packets while releasing the lease and uncommitted work. |
| [GetAsteroidInspectionDisclosure, GetSpaceTrajectoryDisclosure](../../../../src/asteroid_inspection.h) | Read-only company survey-gated quantities, public composition and signed integer motion. Native construction/service/descent owners supply actual inspector progress; opening/rendering cannot step gameplay. |
| [SpaceWorkSample, ScopedSpaceWork](../../../../src/space_work_cost.h) | Opt-in thread-local diagnostic operations only. No saved/network/command/stock authority; native timings are diagnostic. Group21 accepts exact population, linear body/index work, bounded probe arbitration and output-sensitive local queries. |

Actual C++ sources are registered in the existing root/test CMake lists. Direct
consumers include the native mission/order/approach owners, ASTR/SPKN codec,
read-only Space renderer and ordinary new-world boundary. They use these public
owners; no compatibility shim fabricates a second field or vehicle identity.

## Implementation Tasks

Deepening pass, 2026-10-04: implementation groups use only SIMPLE, MEDIUM or HARD difficulty, with at most eight pending tasks each. Sketches describe proposed implementation contracts; verify the named earlier producers at each assumption gate. No gameplay or asset implementation is marked complete by this planning pass.

Groups are chronological and contain consecutive tasks of one difficulty. Each difficulty batch has at most eight tasks; area reading changes only when the implementation boundary or source of truth changes.

### Area Mandatory Reading - Asteroid generation and motion

_Applies to Groups 1-7._

**Entry gate (dependency correction, 2026-10-12).** Field generation and motion consume
`src/multilayer_content.h` selected-profile/asteroid definition exports and
`src/space.h` `OrbitalBodyID`, `SpaceMotion`, `SpaceTravelBand`, reflection and
spatial-query producers. P1's focused generation/motion/state/content tests prove
these producers. They do not consume a spacecraft, orbital structure, launch,
cargo owner or descent job. Group2.2's legal launch-site accessibility separately
requires the actual Plan9 placement predicate; that task cannot close before it
exists. Groups8-22 retain their deployment/release/service/acceptance obligations.
This corrects the former circular prerequisite without changing any gameplay or
completion requirement. The canonical native probe/mission types are in
`src/space_vehicle.h`, rather than `src/space.h`; no duplicate types are allowed.

**Published geometry producer,2026-10-14.** [src/launch_site.h](../../../../src/launch_site.h)
now exports `GetLaunchCenterTerrainTileBlock` and `GetLaunchCenterSiteGeometryBlock`.
The real Plan9 placement command reuses the terrain predicate. Both423
configurations pass its focused/native/ordinary gate in
`logs/launch-site-verification.json`; use
`src/tests/multilayer/acceptance/launch_site_checks-1.py` for current producer
verification. Read the two helper implementations and the actual command's
remaining policy checks for Group2.2; these geometry queries grant no company,
date, town, joining or clearing permission.

Verify each public file/export/schema field with `rg` in the current workspace and inspect its command/field ownership. For a same-plan contract, compile its type/signature fixture or validate its artifact after the preceding producer groups. For cross-plan behavior, also run the producer's focused executable boundary test; declarations alone cannot authorize dependent mutations or runtime claims.
If a consumed required export is absent, stop the implementation, mutation and runtime claims that depend on it. Independent field inputs may proceed; launch-site accessibility still waits for its named producer. Read-only characterization, source/range verification, test-case design and asset-template work that does not assume the missing behavior may continue.
Do not bypass this gate by copying private pool/tile state, inventing local IDs or cargo registries, using a historical plan as a delivered implementation, opening user-wide content stores, substituting a second utility/accounting authority, or adding an unreviewed fallback path. Finish the named producer or revise the shared contract and its direct consumers first.

| File name | Line Ranges | Description |
| --- | --- | --- |
| [src/core/random_func.hpp](../../../../src/core/random_func.hpp) | 24-43 | `Randomizer` and simulation versus interactive state constrain feature-seed generation: preserve Surface simulation RNG and exclude presentation randomness. |
| [src/tests/map_layer.cpp](../../../../src/tests/map_layer.cpp) | 162-183 | Complete `Plane distances ignore the layer` test fixes projected geometry but grants no cross-layer transfer authority. |
| [src/timer/timer_game_calendar.h](../../../../src/timer/timer_game_calendar.h) | 15-39 | `TimerGameCalendar` technology-clock contract and date conversions govern 1980/model/factory introductions; economic/realtime scheduling cannot unlock a technology. |
| [r2mlvs1-a-content-contract.md](../../../zz-rhr/upgrades/2026-10/r2mlvs1-a-content-contract.md#group-2---draft-cargo-meanings) | `Group 2 - draft cargo meanings` | Canonical `NORE`, `WICE` and `RARE` meanings, units and carrier forms constrain composition and finite reserve; do not allocate alternate asteroid cargo meanings. |
| [r2mlvs1-e-ground-and-space-budgets.md](../../../zz-rhr/upgrades/2026-10/r2mlvs1-e-ground-and-space-budgets.md#group-14---launch-returns-and-surface-supplied-orbital-assembly) | `Group 14 - launch, returns and Surface-supplied orbital assembly` | The 20-tonne hold, 2-tonne/74-work-tick extraction bound, two-return/service budget and paid pod supplies constrain starter reserves and movement/interception tuning. |
| `src/multilayer_content.h` (planned public contract; link after implementation) | Live definitions; no fixed range | [r2mlvs1-gameplay-contracts-and-content.md](r2mlvs1-gameplay-contracts-and-content.md) Groups 1-6, ending with Group 6's public-contract task, publish `MultiLayerGameplayProfile`, `LayerVehicleCapability`, `GetMultiLayerContentProfile`, `CheckDeepLayerAvailability`, `UtilityPowerAccount`. These exports/schema fields are authoritative for this area. Same-plan consumers follow those earlier groups; cross-plan consumers verify the producer's required implementation/tests as well. Proposed names alone do not prove delivery. |
| [src/space.h](../../../../src/space.h) (published geometry; native mission in `space_vehicle.h`) | Live definitions; no fixed range | [r2mlvs8-space-world-and-construction.md](r2mlvs8-space-world-and-construction.md) Groups 1-7, ending with Group 7's public-contract task, publish `SpaceVehicle`, `OrbitalBodyID`, `SpaceMission`, `SpaceStructure`, `SpaceTransition`. These exports/schema fields are authoritative for this area. Same-plan consumers follow those earlier groups; cross-plan consumers verify the producer's required implementation/tests as well. Proposed names alone do not prove delivery. |
| `src/launch_center.h` (planned public contract; link after implementation) | Live definitions; no fixed range | [r2mlvs9-launch-centers-and-rockets.md](r2mlvs9-launch-centers-and-rockets.md) Groups 1-7, ending with Group 7's public-contract task, publish `LaunchCenterAnchor`, `LaunchCollectionHub`, `SurfaceHandlingJob`, `SpaceDescentJob`, `CanReleaseOrbitalCargo`, `CommitOrbitalCargoRelease`. These exports/schema fields are authoritative for this area. Same-plan consumers follow those earlier groups; cross-plan consumers verify the producer's required implementation/tests as well. Proposed names alone do not prove delivery. |

**ASSUMPTION: this was already built previously. If not, re-asses at runtime**. This applies only to the producer contracts named in this area's gate, after their listed producer groups have run. Verify their actual exports and focused tests before using the sketches below; proposed types and helper names in sketches are not evidence of delivered code.

**Motion sketch (proposed).** Consume the earlier Space coordinate, edge and slot contracts. One ordered body-motion step publishes positions before probe decisions; attached craft use the same body transform.

```text
for body in stable_body_ID_order:
    body.position, body.velocity = shared_edge_step(body.saved_motion)
    update_derived_bucket(body)
for attached_probe in stable_probe_ID_order:
    probe.position = body.position + saved_attachment_offset
then advance mission decisions and bounded mining
```

Body motion and reserve extraction are independent: a depleted body still follows the chosen persistence/motion policy.

Context has compacted 1 times while executing this group.

### Group 1 - Define resource content and finite asteroid records

{{MEDIUM}}

#### Resource-content defaults

These are editable design defaults. Each asteroid supplies one cargo, with one tonne removed from its finite reserve for each tonne loaded into a probe. Inert material produces no cargo. Choose a size, sample an inclusive whole-tonne reserve from the Small range, then apply the Medium x2 or Large x4 multiplier.

| Asteroid class / cargo | Small reserve | Medium reserve | Large reserve |
| --- | --- | --- | --- |
| Metallic / Nickel Ore (`NORE`) | 400-800 t | 800-1,600 t | 1,600-3,200 t |
| Carbonaceous / Water Ice (`WICE`) | 800-1,600 t | 1,600-3,200 t | 3,200-6,400 t |
| Rich / Rare Earth concentrate (`RARE`) | 100-200 t | 200-400 t | 400-800 t |

| Size | Chance | Body diameter | Mining-probe slots |
| --- | --- | --- | --- |
| Small | 50% | 0.5 tile | 1 |
| Medium | 35% | 1 tile | 1 |
| Large | 15% | 2 tiles | 1 |

| Class | Complete 20-tonne loads per body | Average reserve per body |
| --- | --- | --- |
| Metallic | 20-160 | 1,080 t |
| Carbonaceous | 40-320 | 2,160 t |
| Rich | 5-40 | 270 t |

The average size multiplier is 1.8. Even the smallest rich body supports five full loads, exceeding the two-return starter requirement of 40 tonnes. Class proportions and total population are set in Group 2; size and reserve are sampled independently and saved once. Art selection never changes resource content.

**Configuration:** author these class/size/reserve records in planned `src/multilayer_asteroid_catalogue.cpp`, exposed as immutable typed definitions through [src/multilayer_content.h](../../../../src/multilayer_content.h), following the [logical vehicle catalogue](../../../../src/multilayer_vehicle_catalogue.cpp) style. Cargo meanings remain in [media/multilayer/cargo_catalogue.json](../../../../media/multilayer/cargo_catalogue.json). During planning, edit these tables; implementation transfers their values into the definition records. No configuration code is delivered by this document.

- [X] 1.1 Define metallic/Nickel, carbonaceous/Water Ice and rich/Rare Earth body records using r2mlvs8 `OrbitalBodyID`, fixed-point position, shape/visual size and attachment-slot capacity.
- [X] 1.2 Resolve the single class output from the Resource-content defaults through the selected cargo profile, reusing terrestrial Nickel Ore and Rare Earth meanings. Implement the 100% mineable-content and 1:1 reserve-debit policy; save initial/remaining reserve per cargo and generated composition. A visual sprite variant cannot change the cargo.
- [X] 1.3 Give each class a fixture with one slot, finite reserve and a compatible probe hold. Separate authoritative mining fields from derived spatial/render data.
- [X] 1.4 Add the class/size/reserve definition records at the planned content-catalogue location and expose them through the selected profile. Implement the Resource-content defaults' inclusive sampling and multipliers; verify per-class full-hold counts, expected reserves and extraction durations against the 20-tonne/2-tonne-per-74-tick probe budget. Group 2 combines these reserves with its exact population table into a whole-field resource budget.

**1.1-1.3 producer evidence, 2026-10-12:** `AsteroidBody` uses the shared
`OrbitalBodyID`/`SpaceMotion`, immutable class and size, saved initial/remaining
whole-tonne reserve and one native `VehicleID` lease. The three catalogue outputs
resolve to selected-profile NORE/WICE/RARE; size/art cannot change composition.
ASTR preserves each generated class/reserve and CAPA preserves compatible cargo
with actual extraction-plane provenance. Native mining fixtures operate all
three class holds, enforce one attachment lease, debit one reserve unit per hold
unit and retain cargo through recall and reload. The version406 regression run
passes all three retained phases and independent exact native replays in
RelWithDebInfo (`logs/asteroid-population-mining-regression.log`). Spatial buckets
and drawing remain derived. Group1.4 still requires the complete public size and
resource-tuning records; this closes no launch, service or delivery requirement.

**Historical partial resource-budget evidence for1.4:** the independent
canonical-definition fixture checks all three literal cargo labels and reserve
ranges, full-hold counts and weighted means. Seed `0x20261012` on Normal256x256
produces160 bodies (96/48/16), with95,050 t NORE,99,715 t WICE and4,018 t RARE:
198,783 t actual, inside78,400..627,200 t; the distribution mean is211,680 t.
Native continuation checks count actual Mining steps against74 per2 tonnes,
subtract saved partial progress and charge recalled uncommitted work before
resuming. The focused suite including launch geometry passes4,581 assertions in
11 cases in RelWithDebInfo. At that checkpoint the public size records and
focused producer acceptance were missing. The completed1.4 evidence below
supersedes that gap; motion tuning and later handoff requirements remain with
their respective groups.

**1.4 complete,2026-10-14; both configurations.** The catalogue publishes typed
class and size records, including50/35/15-percent weights,0.5/1/2-tile Q16
diameters, one mining lease and1/2/4 reserve multipliers. The selected Industry
profile exposes immutable class/size spans; Classic exposes none. Generation
uses one independent size draw and one inclusive Small-range reserve draw,
then applies the size multiplier. Feature domains, draw order, generation
version and saved fields remain unchanged; loading never regenerates bodies.
Independent fixtures exhaust all100 percentiles and both reserve endpoints
for every class/size combination, check exact one-draw RNG consumption and
untouched simulation/interactive RNG, and freeze the pre-edit per-cargo ledger.
Shared runtime budget checks prove20-160/40-320/5-40 full loads,1,080/2,160/270t
means, the two-return reserve floor and740 work ticks per20t full hold at2t/74ticks.
`logs/asteroid-catalogue-verification.json` binds the two accepted native
executable/report/log chains:4,274 assertions/ten focused cases each. Command:
`python -B src/tests/multilayer/acceptance/asteroid_catalogue_checks-1.py --configuration RelWithDebInfo`
(also Debug). [Execution notes](../../../zz-rhr/upgrades/2026-10/r2mlsp-r-asteroid-catalogue-notes.md) preserve
the producer evidence and next placement task. This supplies no new paid
mission, saved-phase replay, graphics or network acceptance.

Context has compacted 1 times while executing this group.

### Group 2 - Generate the field from the population defaults

{{MEDIUM}}

#### Population defaults

**Each number below is the total number of asteroids generated on the Space layer. Normal is the default.** Edit the numbers directly to change the desired population.

| Map size | Sparse | Normal (default) | Dense |
| --- | --- | --- | --- |
| 64 x 64 | 6 | 10 | 14 |
| 64 x 128 | 12 | 20 | 28 |
| 128 x 128 | 24 | 40 | 56 |
| 128 x 256 | 48 | 80 | 112 |
| 256 x 256 | 96 | 160 | 224 |
| 256 x 512 | 192 | 320 | 448 |
| 512 x 512 | 384 | 640 | 896 |
| 512 x 1,024 | 768 | 1,280 | 1,792 |
| 1,024 x 1,024 | 1,536 | 2,560 | 3,584 |
| 1,024 x 2,048 | 2,304 | 3,840 | 5,376 |
| 2,048 x 2,048 | 3,072 | 5,120 | 7,168 |
| 2,048 x 4,096 | 4,608 | 7,680 | 10,752 |
| 4,096 x 4,096 | 6,144 | 10,240 | 14,336 |

Rectangular maps use the row with the same total number of tiles: **64 x 1,024 uses the 256 x 256 row**, and 128 x 512 uses that same row. Rotating a map does not change its count. All supported power-of-two dimensions from 64 to 4,096 are covered by these rows. The largest-map counts intentionally grow more slowly to bound simulation work; the hard maximum is the table's largest entry, **14,336 bodies**.

| Class | Population share | Bodies on a Normal 64 x 64 map |
| --- | --- | --- |
| Metallic / Nickel Ore | 60% | 6 |
| Carbonaceous / Water Ice | 30% | 3 |
| Rich / Rare Earth concentrate | 10% | 1 |
| **Total** | **100%** | **10** |

| Placement / lifetime choice | Default |
| --- | --- |
| Distribution | Spread across the permitted travel band using seeded selection of 4-tile grid positions. |
| Minimum initial center separation | 4 tiles; future drift may bring bodies closer. |
| Starter targets | Place one of each class first, preferably within 32 tiles of a valid launch-site candidate; use the nearest admissible grid positions when the band is farther away. |
| Depleted bodies | Remain visible and keep drifting. |
| New bodies during play | None; no replacement on depletion, new-center construction or reload. |
| Generation version | 1; save with feature seed, population selection and generated records. |

The smallest map has 105 admissible 4-tile grid positions, enough for its 14-body Dense setting. Exact class counts use the percentages above: round down with a minimum of one body per class, then award remaining bodies by largest fractional remainder, breaking ties in class-ID order. The smallest Sparse field therefore has 3 metallic, 2 carbonaceous and 1 rich body. The first three body IDs hold the three starter classes; assign the remaining class labels with a seeded shuffle. Keep content, placement and motion draws separate.

**Resource-budget example:** a Normal 256 x 256 map contains **160 asteroids**: 96 metallic, 48 carbonaceous and 16 rich. Group 1's average reserves give 103,680 t Nickel Ore, 103,680 t Water Ice and 4,320 t Rare Earth concentrate: **211,680 t total**, with an absolute 78,400-627,200 t bound. Save/report the actual generated quantities per cargo.

**Configuration:** [src/multilayer_asteroid_catalogue.h](../../../../src/multilayer_asteroid_catalogue.h) publishes selected-profile population rows, class shares and placement records implemented in [src/multilayer_asteroid_catalogue.cpp](../../../../src/multilayer_asteroid_catalogue.cpp). The saved new-game **Asteroid population** dropdown (`game_creation.asteroid_density`: Sparse=0, Normal=1, Dense=2) is declared in [src/table/settings/world_settings.ini](../../../../src/table/settings/world_settings.ini), with storage in [src/settings_type.h](../../../../src/settings_type.h). It chooses a table column. Definition edits require Group7's compatibility/version handling; the player chooses the column when starting a world.

The travel band only constrains placement and capacity; it does not recalculate the selected count. Generate exactly the table count or report that the chosen geometry cannot fit it. These defaults apply to Multilayer Industry; Classic has no asteroid field. Targets continue drifting before the player launches, so missions use their current positions and Group 8's adaptive approach timeout. The 32-tile starter preference is not a guarantee for every later center.

**No-site policy,2026-10-14.** If current Surface terrain has no legal geometric
candidate, retain the exact field with an explicit saved `NoSite` input and
central preference. This supplies no launch permission or accessibility claim;
preparing a later waterside site still uses ordinary Surface commands. A missing
terrain candidate cannot silently reduce the population or fabricate a valid
anchor. Older fields retain `Legacy` unknown initial history, never a reconstructed
site based on their moved bodies or altered terrain.

- [X] 2.1 Implement the exact map-size/count table and class shares in Population defaults. Add the synchronized saved new-game population dropdown at its named location; verify every Sparse/Normal/Dense cell and same-area rectangular maps. There is no tiles-per-body density formula or silent count reduction.
**2.1 implementation evidence, 2026-10-12:** `game_creation.asteroid_density`
is a synchronized, NewgameOnly, version406 saved setting with Sparse=0,
Normal=1, Dense=2. Both random-map and heightmap generation windows expose the
selector; Classic disables it while retaining the user's selection. Existing
fields continue from ASTR without regeneration. The focused population fixture
checks all147 dimension/density combinations against the13 explicit rows,
including same-area rectangles. Production generation/reload checks cover all
three choices for selected-profile and Classic worlds; PATS retains the chosen
column despite a different local config, ASTR stays exact, and Classic has no
bodies. The native widget fixture selects every value and verifies disabled and
restored states. Command: `python -B src/tests/multilayer/acceptance/asteroid_population_checks.py`.
RelWithDebInfo evidence is under `mlm/reviews/space/population-verify/`; Debug and
full network acceptance remain part of the overall completion queue.

- [X] 2.2 Implement the Population defaults' seeded 4-tile grid placement, spacing and three starter targets within the shared travel band. Validate legal launch-site access and publish the actual initial approach-distance bound, using the 32-tile preference when feasible and Group 3's adaptive timeout for more distant targets. Validate tiny-map capacity and refuse unsupported geometry rather than silently losing a class. Discovery and the first mission require no orbital station; building a launch center must not create extra bodies.

**2.2 placement producer verified,2026-10-14.**
`src/launch_site.h` exposes the actual launch command's company-independent
per-tile terrain predicate and complete guarded site geometry query. The real
paid command calls that same predicate; company/date/town/joining/clearing
policy and costs remain at its boundary. This resolves generation's need to
inspect geometry before `StartupCompanies` without fabricating a company or
duplicating placement rules. `logs/launch-site-verification.json` binds143
focused assertions/three cases plus147 actual native placement assertions/one
case each in RelWithDebInfo/Debug, and ordinary exact LCTR/LCHB/STNN/CAPA resaves
(six native processes).
The current424 acceptance also proves that paid placement/removal/rebuild
preserves generated body identities and initial reserves. Isolated native
terrain proves no-site, distant-site and preferred32-tile selection through
this same geometry producer. Genuine generated worlds in the accepted seed
fixture have explicit `NoSite` inputs; they supply no claim of immediately
available launch terrain. [Starter access notes](../../../zz-rhr/upgrades/2026-10/r2mlsp-s-starter-access-notes.md)
preserve the producer handoff.

- [X] 2.3 Generate the complete body list, class/content and initial positions from the saved Space feature seed, generation version and selected tuning without Surface RNG draws. Save the generated records and generation inputs; load, view changes and target selection never reroll them. Group 3 extends this generator with seeded initial motion before producer readiness.
- [X] 2.4 Test repeat generation for the same seed/version/settings and changed-seed fixtures: exact total/per-class counts, reserve bounds and aggregate budget, spacing/band validity, geometric starter accessibility/approach-distance bounds and bounded generation work. Include tiny/narrow-map capacity refusal and maximum-population cases; actual motion/interception checks follow in Group 3.

**Group2 accepted,2026-10-14.** `logs/asteroid-input-verification.json` binds20
reports and64 native processes across RelWithDebInfo/Debug. Each configuration
passes37,122 assertions/12 focused generation/content cases, plus143 focused
and147 native placement assertions. ASTR schema424 appends26 bytes of frozen
initial access inputs; pre424 fields retain unknown `Legacy` history. Genuine
new/legacy worlds continue37 working ticks, independently replay and ordinarily
resave with all26 owner families exact across configurations. Seven malformed
input cases each refuse full load and read-only preview without mutating the
live field. The nearest-grid estimator matches an independent full-grid oracle
and examines at most25 points per candidate; real site search is bounded by
two geometry queries per Surface anchor. No full regression was run.
[Generation input notes](../../../zz-rhr/upgrades/2026-10/r2mlsp-t-generation-inputs-notes.md) retain exact run
identities, schema and the no-site limitation. Group3's complete motion and
attachment requirements remain open.

### Group 3 - Implement trajectory and speed on the shared schedule

{{HARD}}

#### Movement-trajectory defaults

| Trajectory choice | Default |
| --- | --- |
| Path between boundaries | Straight-line drift. |
| Initial direction | One of eight compass directions, equally likely. |
| Direction changes | Reflect at travel-band boundaries; reverse both components at a corner. |
| Random turns / orbit simulation | None. |
| Movement with an attached probe | Carry the probe at its saved attachment offset; reflection does not rotate the body. |
| Collisions / impact damage | None; keep bodies inside the dedicated travel band. |
| Depleted body movement | Continues at its saved speed and direction. |

The shared [Space geometry and reflection defaults](r2mlvs8-space-world-and-construction.md#shared-space-geometry-and-motion-defaults) are implemented by r2mlvs8 Group 10. For example, a body two tiles from a boundary moving toward it at 0.25 tile/day reaches it after eight economy days, then reflects at the same speed.

#### Movement-speed defaults

| Body speed | Chance | Tiles per economy day | Time to move one tile |
| --- | --- | --- | --- |
| Slow | 1/3 | 0.0625 | 16 days |
| Medium | 1/3 | 0.125 | 8 days |
| Fast | 1/3 | 0.25 | 4 days |

Choose the speed once at generation, independently of class and size. Group 8's basic probe travels at 4 tiles/day, 16 times the fastest body. Filling its 20-tonne hold takes 10 mining days; a fastest body drifts 2.5 tiles during that work.

**Configuration:** selected-profile `GetMultiLayerAsteroidHeadingDefinitions` and `GetMultiLayerAsteroidSpeedDefinitions` expose the eight equal-weight directions and three equal-weight speeds from the asteroid catalogue. Coordinate precision, step timing and travel margins use the shared `MultiLayerSpaceTuning` record in [src/multilayer_content.h](../../../../src/multilayer_content.h) / [src/multilayer_content.cpp](../../../../src/multilayer_content.cpp); [src/space.cpp](../../../../src/space.cpp) applies r2mlvs8's existing translation/reflection contract. Probe-model binding remains in Group8.

**Implementation notes:** one economy day is 74 synchronized simulation ticks. Update once per tick, in body-ID order, then update attached transforms and probe decisions. Use the shared Q16 coordinates (65,536 units/tile); nominal speed numerators are 4,096 / 8,192 / 16,384 Q16 units per 74 ticks. Heading components use 65,536 for axial directions and 46,340 for diagonals, rounding diagonal speed down. Start phase/remainders at zero; divide each unsigned component magnitude plus its saved remainder by 74, retain remainder 0..73 and apply the velocity sign to whole displacement. Reflection changes the sign while preserving fractional travel. Rendering projects the published result without advancing gameplay.

**Group3 accepted,2026-10-14.** `logs/asteroid-motion-verification.json`
binds10 reports,26 native processes and96 identities audited at acceptance.
Both configurations pass44,368 assertions/16 focused cases plus3,684 boundary
assertions/eight native cases. All26 owner families match independent resume,
ordinary resave and520 corresponding cross-configuration comparisons. Catalogue
heading/speed records retain the original random draws, content and positions;
`MultiLayerSpaceTuning` exposes the unchanged shared geometry/step defaults.
Generation version1 and schema424 remain unchanged. All attached preview and
actual matching mining-lease transforms publish before their work decisions.
[Motion notes](../../../zz-rhr/upgrades/2026-10/r2mlsp-u-motion-notes.md) retain the worked edge/corner records,
632-tick32-tile pursuit, distant generated starter geometry, travel-cost ledger
and explicit isolated-fixture scope. Rendering projects published Q16 positions
at native sprite-pixel precision; it does not advance gameplay. Later paid
pursuit/attachment and full mission acceptance remain open.

- [X] 3.1 Implement the Movement-trajectory defaults' equal eight-heading distribution and shared `linear_reflect` behavior. Extend Group 2's seeded generator to assign heading and speed without changing already generated content/positions; no fresh random turns per tick or full orbit solver. Publish worked initial-position/vector, edge and corner trajectories using the r2mlvs8 shared defaults unchanged.
- [X] 3.2 Implement the three body-speed choices, one-tick schedule and per-axis fractional accumulation from Movement-speed defaults. Compare with Group 8's declared probe speed, tolerance and adaptive timeout; actual probe-model binding follows in Group 8. Save velocity/phase/remainders through the shared motion record and validate the numerical interception examples and travel upkeep without platform-dependent floating point.
- [X] 3.3 Advance bodies in stable ID order once per declared simulation step, then update their spatial buckets and attached craft positions before probe work decisions. Rendering projects the published Q16 results at native sprite-pixel precision; any presentation interpolation must not advance or alter authoritative motion.
- [X] 3.4 Validate a candidate edge/corner step against the permitted drift band. Apply the shared reflection rule to velocity/overshoot and translate attached craft with the body's saved offset unchanged; never detach or teleport a probe independently.
- [X] 3.5 Test exact displacement at minimum/maximum speeds and cardinal/diagonal headings, fractional-step accumulation, and declared attachment-offset boundary records across an edge/corner and save/reload, with the layer hidden. These early geometry fixtures do not claim real latched-probe mission acceptance; that waits for the later controller groups.
- [X] 3.6 Validate the 32-tile numerical interception fixture and the actual generated starter-distance bounds, including more distant targets using the adaptive deadline. Use the fastest supported body and basic-probe speed, and account for pursuit/return upkeep in the two-return budget. Later Group 15 runs these fixtures through the actual pursuit/attachment controller.

### Group 4 - Publish the population, resource and motion tuning

{{SIMPLE}}
- [X] 4.1 Reconcile these default tables with the implemented definition records/settings and record worked map-population, finite-resource and interception validation plus the consumed r2mlvs8 band/edge contract. Document any deliberate tuning changes and their version handling before Group 7 producer readiness; attached probes follow the same authoritative motion.

**Group4 reconciled,2026-10-14.** The tables above match the accepted immutable
records and saved density selector; this documentation-only group needs no
additional build. Group3's current focused gate includes the generation and
catalogue fixtures, so all147 population cells,14,336 maximum bodies, inclusive
resource bounds and unchanged seed ledger remain verified in both configurations.
Normal256x256 is160 bodies (96/48/16), with actual95050/99715/4018 tonnes and
198783 total for feature seed `0x20261012`; expected total is211680 tonnes and
the absolute bound is78400..627200. [Motion notes](../../../zz-rhr/upgrades/2026-10/r2mlsp-u-motion-notes.md)
publish the shared64x64 centre rectangle, saved edge/corner fractions and
632-tick fastest-body32-tile interception. The basic probe's declared4-tile/day
speed is16 times the fastest body's nominal speed; its0.25-tile tolerance and
frozen adaptive deadline remain the Group8 model-binding obligation.

No population, class, reserve, size, heading, speed, strip, margin or clock
default changed. Definition extraction and earlier publication of matching
attachment transforms preserve generator version1. ASTR424 adds frozen initial
access inputs while pre424 fields keep unknown history and their actual saved
motion. Future simulation-definition changes must version the existing selected
profile/content contract, and algorithm changes must version the generator;
neither loads nor renderers may replace saved body records with current defaults.
The rendering wording now describes the actual native read-only pixel projection,
as recorded in Group3; broader presentation and Group7 readiness remain open.

### Group 5 - Keep depleted body identity and attachment safety

{{HARD}}
- [X] 5.1 Make zero reserve stop mining without refilling on detach/reload. Default depleted bodies persist visibly until a declared safe removal policy applies; generation does not respawn them silently.
- [X] 5.2 If removal is supported, first resolve all leases/mission references and release the spatial entry in deterministic order. Prevent a reused body ID from becoming an old probe target.
- [X] 5.3 Test depletion with slot/reference boundary records and publish the invalid-target notification contract. Run the real attached/approaching probe detach/retarget/return cases after the later mission-controller groups.

**Group5 accepted,2026-10-14.** `logs/asteroid-depletion-verification.json`
binds two reports/eight native processes,1051 focused assertions/two cases and
61 native boundary assertions/two cases each, plus130 cross-configuration owner
comparisons. Depleted bodies keep moving and remain indexed/rendered. There is
no in-play removal/replacement API; exact counts and stable IDs reject erasure
or renumbering, so task5.2 closes under its explicit conditional policy.
The native fixture stages a declared zero-reserve record in an existing405 copy,
then proves the actual existing lease exits to Return on one working tick,
retaining cargo and refusing another Mine assignment. Independent resume and
ordinary resave retain all26 owners exactly. This fixture does not claim
production of its historical missing reserve or a newly launched mission.
[Depletion notes](../../../zz-rhr/upgrades/2026-10/r2mlsp-v-depletion-notes.md) publish invalid-ID/recall distinctions
and `TargetRemoved` / `Target unavailable` notification semantics. Later controller
groups retain their actual final-debit, retarget, cancellation and full mission
conservation obligations.

### Group 6 - Asteroid generation and motion

{{MEDIUM}}
- [X] 6.1 Enforce the space-band/approach policy from r2mlvs8 so bodies cannot invalidate static module footprints or teleport latched probes through edges.

**Group6 accepted,2026-10-15.** `logs/asteroid-band-verification.json` binds
43,699 focused assertions/ten cases and20 actual native assertions/one case each,
six native processes and52 cross-configuration owner comparisons. Static site
validation now consumes the same immutable strip quarters as body motion;
all49 supported map shapes, static recipe footprints/south approaches and every
heading/speed/corner envelope pass. Actual native quotes accept a free north
site and refuse the moving strip/crossing approach without changing structures,
orders, bodies, money or RNG. Ordinary resave and corresponding configurations
retain all26 owners exactly. [Band policy notes](../../../zz-rhr/upgrades/2026-10/r2mlsp-w-band-policy-notes.md)
preserve the current gate and19 identities audited at acceptance. This narrow
consumer proof does not close Plan8's broader first-station/launch-access groups
or later paid mining-controller acceptance.

### Group 7 - Asteroid generation and motion

{{HARD}}
**Saved-definition policy:** population, resource and motion tuning use the selected profile's existing public owner. Version simulation-affecting definition changes through the profile/content compatibility contract before load/network use; generation-algorithm changes also increment the generation version. Retain actual generated counts, reserves and saved motion in existing worlds. Reject incompatible definitions unless an explicit migration is implemented.

- [X] 7.1 Publish `src/asteroid.h` with the exact exports/schema fields in `Public Handoff Contract`; document authoritative fields, invalid-ID semantics and command/estimate boundaries. Register any created C++ source in the existing build list. If the chosen implementation uses a different public location, update this contract and every direct consumer before declaring the producer ready.
- [X] 7.2 Validate the public generation/tuning and body-state contract against Groups 1-4 fixtures: population inputs/version, class/composition, per-cargo reserves and shared saved motion are accessible through their named owners. Loading continues existing bodies without regeneration or replacing saved velocities with new profile defaults.

**Group7 accepted,2026-10-15.** The public handoff table above documents exact
current owners, fields, invalid-ID and estimate semantics. Existing current
binaries verify the published signatures through their field/tuning consumers;
no new build was needed. `logs/asteroid-handoff-verification.json` binds four
current reports/30 native processes and85 identities audited at acceptance.
Both configurations pass37,122 assertions/12 focused cases and actual new-world
population/reload plus new/legacy37-work-tick continuation. All26 owner families
match independent replay, ordinary reload and260 comparisons across configurations.
Saved records retain their original content/motion instead of current defaults.
[Generation handoff](../../../zz-rhr/upgrades/2026-10/r2mlsp-x-generation-handoff.md) preserves the exact gate,
version policy and transition to the next mandatory-reading Area. This publishes
generation/body readiness, not completion of the later mission/manufacturing,
presentation or network consumers.

### Area Mandatory Reading - Probe mission controller

_Applies to Groups 8-13, including8B/8C._

**Entry gate.** The following public contracts must exist before dependent implementation in this area: `src/asteroid.h`: `AsteroidBody`, `AdvanceAsteroidField`, plus `SpaceVehicle`, `SpaceMission`, `AssignSpaceMiningMission` and `SpaceVehicle::Tick` in `src/space_vehicle.h`.
Verify each public file/export/schema field with `rg` in the current workspace and inspect its command/field ownership. For a same-plan contract, compile its type/signature fixture or validate its artifact after the preceding producer groups. For cross-plan behavior, also run the producer's focused executable boundary test; declarations alone cannot authorize dependent mutations or runtime claims.
If a required export is absent, stop the dependent implementation, mutation and runtime claims in Groups 8-13. Read-only characterization, source/range verification, test-case design and asset-template work that does not assume the missing behavior may continue.
Do not bypass this gate by copying private pool/tile state, inventing local IDs or cargo registries, using a historical plan as a delivered implementation, opening user-wide content stores, substituting a second utility/accounting authority, or adding an unreviewed fallback path. Finish the named producer or revise the shared contract and its direct consumers first.

| File name | Line Ranges | Description |
| --- | --- | --- |
| [src/cargopacket.h](../../../../src/cargopacket.h) | 458-489 | `VehicleCargoList::Append`, `AgeCargo`, `Stage`, `Return`, `Unload`, `Shift` move accountable packets; exchange/descent/processing must not clone packet age or feeder shares. |
| [src/cargopacket.h](../../../../src/cargopacket.h) | 596-603 | `StationCargoList::Reserve`, `Load`, `Truncate`, `Reroute` provide bounded station moves and destination reservations; hub inventory has one physical owner. |
| [src/command.cpp](../../../../src/command.cpp) | 301-328 | `CommandHelperBase::InternalExecuteValidateTestAndPrepExec` defines refusal/estimate before execution; no validation phase may consume inventory, RNG or funds. |
| [src/asteroid.h](../../../../src/asteroid.h) (published generation/body contract) | Live definitions; no fixed range | [r2mlvs10-asteroids-and-mining-probes.md](r2mlvs10-asteroids-and-mining-probes.md) Groups 1-7, ending with Group 7's public-contract task, publish `AsteroidBody`, `AdvanceAsteroidField`, plus `SpaceVehicle`, `SpaceMission`, `AssignSpaceMiningMission` and `SpaceVehicle::Tick` in `src/space_vehicle.h`. These exports/schema fields are authoritative for this area. Same-plan consumers follow those earlier groups; cross-plan consumers verify the producer's required implementation/tests as well. Proposed names alone do not prove delivery. |
| [src/source_type.h](../../../../src/source_type.h) | 19-56 | Current append-only `SourceType::Asteroid=3` identifies a stable bounded body; reuse it rather than aliasing an industry/town ID. The16-bit SourceID fits the14,336-body maximum. |
| [src/cargopacket.cpp](../../../../src/cargopacket.cpp) | 246-285 | Complete `VehicleCargoList::Append` does not enforce hold capacity and may merge/delete pointers; mining preflights capacity and initializes provenance first. |

**ASSUMPTION: this was already built previously. If not, re-asses at runtime**. This applies only to the producer contracts named in this area's gate, after their listed producer groups have run. Verify their actual exports and focused tests before using the sketches below; proposed types and helper names in sketches are not evidence of delivered code.

**Mission sketch. ASSUMPTION: this was already built previously. If not, re-asses at runtime**. The preceding asteroid groups provide saved motion/leases and r2mlvs9 provides tested deployment/release/descent. Verify those producer behaviors at the entry gate; neither a sprite nor a declared API proves delivery.

```mermaid
stateDiagram-v2
    Deploy --> Rendezvous
    Rendezvous --> Attach: slot granted and approach valid
    Attach --> Mine
    Mine --> Detach: full or depleted or service limit
    Detach --> ReturnRegion
    ReturnRegion --> DescentHandoff: range and capacity accepted
    DescentHandoff --> Service
    Service --> Rendezvous: paid and target valid
```

Implement one transition family per group. Failure paths retain the last authoritative cargo owner and release only reservations they own; the diagrams do not replace those guards.

### Group 8 - Configure the delivered small-probe shell

{{MEDIUM}}

#### Mining-probe defaults

| Probe quantity | Default |
| --- | --- |
| Travel speed | 4 tiles per economy day. |
| Approach tolerance | 0.25 tile from the authorized approach point. |
| Approach timeout | At least 20 economy days; extend for distant targets using the rule below. |
| Cargo hold | 20 t of one selected compatible asteroid cargo. |
| Mining rate | At most 2 t per economy day. |
| Time to fill a hold | 10 mining days. |
| Upkeep while mining | 500 money/day. |
| Upkeep while pursuing, returning or waiting | 100 money/day. |
| Initial return pods | 2; one expended per completed cargo return. |
| Service interval | After two cargo-return batches. |
| Service time / materials | 2 economy days; RFMT 1 + ELEC 1. |
| Replacement pod | 2t shell; STEL1 + GOOD2 +2,000 money +74 ticks of Surface assembly. |
| Cargo descent time | 2 economy days. |
| Separate elapsed-travel-time endurance cutoff | None; pods, service state and the saved approach deadline constrain missions. |

The [revised Space budget](../../../zz-rhr/upgrades/2026-10/r2mlvs1-e-ground-and-space-budgets.md#space-economy-revision---2026-10-12)
is authoritative for body/pod materials and masses; a pod now uses STEL1 +
GOOD2 and weighs2t. The unchanged launch/probe schedules govern shared hold,
work, cost and service. r2mlvs9 owns paid replacement and descent. No orbital
station is required for the basic loop. The old SCMP-only pod recipe must be
replaced in the catalogue, not exposed as a second construction route.

**Configuration:** logical model15 (Mining probe) owns `MultiLayerMiningProbeTuning` through [src/multilayer_vehicle_catalogue.cpp](../../../../src/multilayer_vehicle_catalogue.cpp) / [src/multilayer_content.h](../../../../src/multilayer_content.h). `GetMultiLayerMiningProbeTuning` publishes that model's speed, tolerance and frozen deadline defaults. The r2mlvs9 hull and this controller consume the same native engine/model; hold mass, extraction, upkeep and service remain in `MultiLayerSpaceBudget`. These expose the existing revision2 values without a content or save-schema change.

**Timeout rule:** measure upward-rounded Q16 distance `D_fp` to the current approach point when rendezvous begins, then save `max(1480, ceil(D_fp * 74 / (3 * 65536)) + 148)` ticks as that attempt's deadline. This allows at least 20 days, or travel at a conservative 3-tile/day closing speed plus two days. Target-position updates cannot keep extending the deadline. Group 15 must verify the controller meets this allowance on unblocked paths. A 32-tile approach needs 632 ticks under ideal straight pursuit or 790 ticks under the conservative bound, within the 1,480-tick minimum.

Charge travel/wait upkeep in addition to the revised25,900-money two-return
collection ledger: add `100 * assigned_idle_ticks / 74`, retaining the accrual
remainder. A saved descent job owns and charges its cargo once.

- [X] 8.1 Consume the minimal probe hull/carry/deployment identity already delivered by r2mlvs9; add mining capability to that same engine/unit rather than create another probe registry or launch mechanism.
- [X] 8.2 Bind the Mining-probe defaults to the existing model's tuning and add home-center/body-target commands, hold/refit choice, speed, extraction rate, upkeep, timeout and service budget. Use Group 3's body-speed bounds and the canonical r2mlvs1 cost/service budget. Validate missing target as an idle/unassigned state before a mission begins.

**Delivered shell binding,2026-10-15:** both configurations execute the actual paid commissioning/carry/deployment producer, then mine with that same model15 EngineID/VehicleID/generation. Generation-checked paid commissioning/recovery supplies its home; `CmdSetSpaceMission` selects the body and compatible empty-hold cargo, refusing an incompatible loaded refit. A newly deployed shell with no target stays Idle. The saved allowance now uses upward-rounded distance to the current attachment point, including its offset; attachment uses the declared0.25-tile tolerance. Native work charges the common budget with saved remainders and extracts20t in740 working ticks without spending a pod before descent. Focused checks pass13,518 assertions/nine cases per configuration; actual collection and its2t/37-work-tick reload pass8,941 and7,317 assertions. Ordinary partial/complete resaves and interrupted continuation preserve26 owner families;156 comparisons also match across configurations. `logs/probe-shell-verification.json` binds14 accepted processes and63 audited referenced identities. [Shell notes](../../../zz-rhr/upgrades/2026-10/r2mlsp-y-probe-shell-notes.md) preserve the public exports and next producer gate. Content2/schema424 remain unchanged. Later roles/manufacturing and full mission acceptance stay open.

### Local Mandatory Reading - Additional probe roles and orders

_Applies to Groups8B/8C._ Read the Global and Probe mission controller Area
blocks, the shared Space economy revision and revised numerical tables,
parent AD-9/AD-11/AD-12, actual `SpaceVehicle`/`SpaceMission` ownership in
`src/space_vehicle.h`, `AsteroidBody` in `src/asteroid.h`, and the current native
command/save validation. Initial role deployment requires Plan9's tested
commissioning handoff. Construction mutation/unload additionally requires
Plan8's `space_structure.h` and Plan11's `orbital_station.h` blueprint/job
owners. Their coordinated Group13B construction batch implements and validates
the missing producer together with these order consumers; batch the build as
requested by the user. Until that combined executable validation passes,
retain all runtime handoff claims as pending. No free deployment or parallel
cargo registry is authorized by declarations.

Context has compacted 2 times while executing this group.

### Group 8B - Implement survey probes and typed ordered missions

{{HARD}}

**Working-tree survey/order batch,2026-10-13; native verification passed in both configurations.** Native model19 now
executes148 working ticks against the current moving-body approach without
claiming the mining lease. `AsteroidField` owns sparse per-body company knowledge;
`GetAsteroidQuantityDisclosure` is the sole player quantity query. Takeover
transfers knowledge and company removal clears old bits before ID reuse. The
inspector now shows Unknown until completion and exposes generation-checked
append/delete/reorder/clear controls with scrollable fleet/order rows. Editing
restarts the mission and discards partial scan work; unsurveyed mining stays legal.
Proposed save413 adds mandatory `SPKN` receipts and permits bounded Survey-only
work in `SPOR`, with independent copied company/body/order preview validation.
The native fixture uses two actual paid launches, concurrent mining, cancelled
and interrupted scans, company-specific screens and real company takeover/reuse.
Generated fixture names/version expectations moved together to413; retained
inputs and prior reports remain unchanged. This batch carries Plan9's paid-canal
descent fixture correction and active-service capture. Both builds pass685
contract assertions in15 cases plus the complete native survey and paid
descent/service suites. Survey verification includes fourteen full and fourteen
copied-preview corruptions and seventeen exact saved owner families. Actual
inspector captures show owner quantities, foreign Unknown and the distinct
Survey-probe default name; saved0x6000 string identities remain unchanged.
The complete Debug regression pass is green: all eight native feature suites,
both fresh profiles, eight retained underground saves and both legacy Space
continuation suites. Current RelWithDebInfo also passes the affected native
logistics, descent/service, survey and refreshed rocket suites. Exact build,
source, binary and report identities are indexed by `logs/space-survey-verification.json`.
Construction, orbital resupply, final art and full integrated/network acceptance
remain open; passing these suites does not finalize either plan.

- [X] 8B.1 Add survey19 and construction20 without renumbering models0..18 or native vehicle kinds. Expose role/recipe-specific Earth launch-center commissioning; enable orbital manufacture only through a completed Assembly's same material gate. Version the content/save contract explicitly.
- [X] 8B.2 Add synchronized owner-validated typed order queues for Visit, Survey, Mine, Deliver/Construct, orbital Unload and Return/Service. Save order index, target generation, work and blocked reason; role-invalid actions refuse without cargo/reserve/money mutation. Ground StationID orders cannot truncate32-bit body IDs.
- [X] 8B.3 Implement148-working-tick moving-body scans and saved per-company knowledge. Hide initial/remaining quantities through one disclosure query in every player-facing body view until completion. Cancellation, hidden views and old preview surveys grant nothing; unsurveyed mining remains legal. Scans do not occupy the exclusive mining lease.
- [X] 8B.4 Verify Visit(bodyA) then Mine(bodyB), role refusals, two companies with different knowledge, partial/cancelled/completed scans, company-ID reuse, old unsurveyed migration and exact save/network continuation. No UI/tooltips/scripts may expose undiscovered amounts.

**Commissioning consumer verified,2026-10-16.** `logs/probe-role-commissioning-verification.json` binds both current429 configurations' focused content, Earth production-receipt and ordinary physical purchase contracts, plus eight exact paid19/20 queued/reserved/before/complete milestones each from the immutable Plan11 replacement handoff. Its83 current producer source identities still match before reuse. Logical19/20 retain the original native Space type and append to0..18; content2/SDEF, actual Earth workshop/body and operational Assembly/full-input/shared-work owners are delivered. No new commissioning implementation or save revision is needed. Continue sequentially at8B.2: typed queues exist, but orbital Unload and Return/Service still stop at producer placeholders despite delivered freight/service commands. Actual action completion and8B.4 acceptance remain open; this commissioning proof does not close Group8B.

**Typed actions verified,2026-10-16.** New `space_order_actions.h/.cpp` delegates reached whole-hold Unload and due Return/Service to the delivered native freight/Core-Dock/onboard owners. Pending descents, full stock and absent parts/power retain the actual instruction and assets. Save430 explicitly admits the existing queue alongside its actual onboard service work without adding bytes or another timer. Independent preview reads normal Station authority from the same-file native STNN table and exact launch-anchor incarnation/position, without live ownership. `logs/probe-order-actions-verification.json` binds both430 builds,19 sources and324 directly audited identities:587 focused assertions/four cases and12,688 native assertions/15 stages each;8,123/8,122 resumed assertions,13 ordinary checkpoints and ten full/copied refusal pairs each. Thirty owners match across1,080 cross-configuration comparisons. Original paid parts/pods arrive through actual rocket resupply; two real returns exhaust allowance before148 service ticks consume parts once. Pause/reload and removal of a separately paid empty service center preserve work and lost-target intent; the original receiving center's asset demolition guard remains intact. No physical input, pose, money or allowance is granted. Continue at8B.4; this proof does not claim its remaining survey/migration/network acceptance. [Current notes](../../../zz-rhr/upgrades/2026-10/r2mlsp-z-f-probe-order-notes.md) retain exact boundaries.

**Mission acceptance verified,2026-10-16.** `logs/probe-mission-verification.json` binds both430 builds,16 sources and269 directly audited identities:11,726 native assertions/13 stages and792 resumed assertions/six continuations each. Thirteen ordinary checkpoints preserve30 native owners, including real Visit(body1) then Mine(body0), unsurveyed extraction, role/owner/stale-generation refusals,37/148-tick scans, cancellation, actual knowledge takeover and company-ID reuse. Genuine408 input withoutSPKN remains unknown after148 old-preview ticks. Actual SendNet/distribution/next-frame/PostFromNet dispatch matches local commands, refuses early execution and does not repeat on duplicate dispatch. Thirty owners match across960 cross-configuration and360 local/network comparisons per build. Fourteen actual inspector captures per build retain native/pixel provenance; the completed owner sees quantities while the other company sees Unknown. The sole orbital GUI quantity path uses the company disclosure query, and172 script units expose no raw reserve fields. Full multiplayer socket integration remains in its later owning gate. Group8B is complete; continue at8C's remaining processor/freight producer gate. [Mission handoff](../../../zz-rhr/upgrades/2026-10/r2mlsp-z-g-probe-mission-notes.md) preserves exact originals and evidence.

Context has compacted 1 times while executing this group.

### Group 8C - Deliver construction materials and reuse orbital products

{{HARD}}

- [X] 8C.1 Implement the Earth-launched construction probe's20t mass-limited STEL/GOOD hold, paid material-built body and generation-checked blueprint orders. Count Goods crates with the canonical weight; preserve payload/cargo ownership through launch and role-specific refits.
- [X] 8C.2 Deliver actual packets to an empty Core blueprint through Plan11's saved construction job; advance bounded construction work and consume each required input once. Support missing-material waits, multiple trips/probes and cancellation without needing a pre-existing dock/storage/Assembly.
- [X] 8C.3 Allow explicit unloading of mined ore at completed orbital processors and loading of their Steel/Goods for construction or Plan9 authorized drop/descent. Distinguish construction reservation from transport ownership; competing builds/export cannot claim the same quantity.
- [X] 8C.4 Exercise Earth-only probe commissioning, two-material first-station delivery, constructed Steelworks/Goodsworks, imported versus locally produced construction stock, later local probe assembly and product Earth collection. Save interrupted orders/jobs and compare next-step conservation and command replay; no free station, input, launch or duplicate payment.

**Delivered boundary:**8C.1/8C.2 finish the physical frame only. The native UI
explicitly identifies it as awaiting activation; it grants no free storage,
docking, assembly or electricity. Plan11's1.3/1.4 and the remaining8C tasks
still gate a usable station and orbital manufacturing. Cancelled jobs retain
their site until unconsumed packets are collected; consumed materials cannot
be recovered through the cancellation command.

**Processor transport consumer verified,2026-10-17.** `logs/probe-product-contract-verification.json` SHA256 `c637da7e4728965c5240af99bf2e026dec789c87616dddddb2f85a76be54d2e2` reconciles the exact accepted Plan11 11/12/12B indexes and original438 saves in both configurations. Actual finite mined inputs enter constructed specialized processors through precise native berths. Original20Steel separately finances18 construction and two Goods inputs; fresh later10Steel supplies two exports and eight retained units. Four actual Goods and those twoSteel use original paid-shell descent and linked Surface rail/lorry collection. Historical Surface36Steel/64Goods remain separate. Paid haulers retain canonical mass, quantity and original provenance through typed explicit exchanges, with three independent full/copied refusal pairs. The consumer independently decodes original packet/material/custody owners without requalifying old binaries or adding stock/work.8C.3 is complete;8C.4 still requires coordinated acceptance and product command replay. Starting-content procurement remains unverified.

**Coordinated scenario and command-replay acceptance,2026-10-17.** `logs/probe-product-replay-verification.json` SHA256 `f1acde0c26243341ae493df720ddb3419d05e84e7f5f6998d96c68ef96901c72` binds both current438 configurations, eight dated producer indexes and exact original report/save memberships. Both pass2949 focused assertions/23cases and1862 native assertions/eight command scenarios. Original Earth commissioning, no-station first-Core construction/activation, imported versus processed construction inputs, local Assembly and interrupted local model15 production remain their accepted dated scenarios. Independent427 receipt decoding identifies actual local miner20/gen6 from factory3/gen4 serial1, capital100000; an Earth-carried model15 cannot substitute that producer. The original inherited starting-stock procurement limit remains explicit.

Current native tests load exact12B paid checkpoints and dispatch processing Resume, full original product freight, typed LaunchCenter Unload and four actual Surface reservations through local Post and original SendNet/distribute/next-frame/PostFromNet. Early execution and repeating a drained queue preserve work, money and all original physical owners. Four pairs compare120physical/eightmetadata families per build, plus90physical/sixmetadata comparisons against original saved next actions. Both builds agree across240physical/16metadata comparisons. No new stock, station, hull, source account, pose or work is installed; no new delivery income appears. The focused burst exposed a stale Earth test treating now-supported content3 as future; its refusal now uses the declared current version plus one, with documented fixture ownership. Final Releasebuild2/Debugbuild2, Release native-a2/native-b2/focused3, Debug native-a2/native-b2/focused1 and audit1 pass. Earlier executables/failed stale-version burst are excluded. Full socket multiplayer remains its later owning gate. Group8C is complete; continue9.

### Local Mandatory Reading - Moving-target pursuit acceptance

_Applies to Group9 only._ Read Global and Probe mission controller Area. Inspect canonical `GetSpaceMiningApproachTicks`, `AssignSpaceMiningMission`, `GetSpaceMiningBlock` and actual `SpaceVehicle::Tick`/typed-order dispatch in current sources; there is no separate `AdvanceSpaceMiningMissions` export. Read the exact current generation/motion/shell/paid mission producer indexes as dated boundaries and their directly relevant focused tests. Verify deployed original home/body admission, current moving approaches, deterministic speed/timeout bounds and lost/busy/low-budget exits with retained packet ownership. No fixture pose or stock may stand in for a native paid route; reuse actual saved mission phases when available. Do not reopen historical executable/source graphs or preload later art/UI gates.

### Group 9 - Deploy and pursue a moving target

{{HARD}}
- [X] 9.1 Activate the carried small probe only through the r2mlvs9 deployment handoff and validate its home center/body target. Start Rendezvous from the saved deployment position; no orbital station is required.
- [X] 9.2 Use bounded deterministic pursuit of the target current approach position, updating by target ID each simulation step. Consume Group 3's body-speed bounds and Group 8's probe speed/timeout, with fixed-point step rounding; validate them against the actual approach geometry and service budget. An optimal orbit solver is unnecessary.
- [X] 9.3 Revalidate target/slot availability during pursuit; on disappearance, timeout or exhausted service budget choose the explicit blocked/retarget/return state without losing cargo.
- [X] 9.4 Test a body crossing its initial tile and edge while approached; the probe follows current coordinates rather than an old tile order.

**Moving-target pursuit verified,2026-10-17.** `logs/probe-pursuit-verification.json` SHA256 `58389fe799980625900bbe16ff068a718b7efbceb7000ed45bcd5555c1007b33` binds both current438 builds to the exact original paid Earth deployment. Original miner16/gen2 retains its100000 paid content2 receipt, actual home4/gen3, two paid pods and return allowance2; zero orbital structures exist. The original saved pose begins a real body0 mission with a frozen1480-opportunity ceiling-root allowance. Actual native simulation intercepts after92 opportunities while the body crosses a tile boundary. Independent SVST/VEHS/ASTR decoding joins the same hull, home, current body/offset and exclusive lease; all1760 resource units remain and no extraction work or cargo is granted.

Both builds pass13048 focused assertions/ten cases and856 native/save assertions across five processes. Native assignment/interception, unchanged own-assignment and actual own-resave interception preserve30 physical families plus CDIM/SPRC:60physical/fourmetadata local comparisons per build and120physical/eightmetadata between builds. Separate geometry components follow both reflected travel edges across initial tiles using the original fixed-point movement functions. Explicit isolated legacy loss/depletion/deadline inputs and an exhausted paid-manifest input retain the exact two-unit packet; these declared unit starting conditions are not native paid procurement evidence. Shared approach components separately preserve stable request priority/exclusive ownership. Final Releasebuild3/Debugbuild1 and audit1 pass; Release native2/own1/resume1/focused2 and Debug all1 are accepted. Initial missing-include/content-descriptor diagnostics are excluded. No production/schema changes; inherited starting-content procurement remains unverified. Continue10.

### Local Mandatory Reading - Finite extraction and packet provenance

_Applies to Group10 only._ Read Global and Probe mission controller Area. Inspect actual `SpaceVehicle::Tick` Approach/Mining transitions, original shared `RequestSpaceApproach`/`PrepareSpaceApproachTick` priority, `UpdateAttachedSpaceCraftTransforms`, finite `AsteroidBody` reserve, `CargoPacket::Append` ownership and source initialization. Consume exact accepted9 assigned/intercepted original paid saves and their immutable index; retain old motion/shell/heavy finite-mining proofs as dated boundaries where relevant. Verify exclusive leases, body-relative attached transforms, complete hold/reserve/work/allocation preflight and once-only native debit/provenance through focused actual participants. Separate declared failure fixtures from paid native extraction; preserve interrupted extraction next actions without installing new cargo, pose, reserve or work. Do not preload later return/service or artwork gates.

### Group 10 - Lease an attachment slot and mine finite reserve

{{HARD}}
- [X] 10.1 Acquire a body attachment lease through r2mlvs8 stable request/ID ordering. Attach only within approach tolerance with valid relative position and free slot; two competing probes cannot both own it.
- [X] 10.2 Save body ID, slot and attachment offset; derive latched position from the body after each motion update rather than independently integrating probe velocity.
- [X] 10.3 At each mining step, preflight the minimum of work allowance, remaining body reserve and compatible hold room plus packet capacity. Commit reserve debit, hold credit and service/work cost once.
- [X] 10.4 Initialize new cargo through the canonical provenance contract: record extraction-plane coordinates and age at production, never cast `OrbitalBodyID` to `IndustryID`. Account for reserved hold capacity and `Append` merging/deleting packet pointers; moving the body later cannot rewrite existing cargo origin.
- [X] 10.5 Handle full hold, depleted body, low service and cancellation by requesting detachment after the last committed extraction. A failed packet allocation must not debit reserve.

**Finite extraction and native packet participant verified,2026-10-17.** `space_mining_inventory.h/.cpp` publishes `CommitSpaceMiningExtraction` over the original native hull, finite body and CargoPacket owners. Ready interval, same exclusive lease/target, resource meaning and whole hold room precede the minimum model/reserve/capacity batch. Actual native packet capacity remains mandatory; an additional admission veto can refuse but cannot manufacture headroom. Original `SpaceVehicle::Tick` retains upkeep/work accrual and the last partial interval on refusal. Successful commit initializes Asteroid/body/Space production provenance before Append may merge/delete the new packet, then debits reserve, credits extraction, resets work and releases full/depleted leases once. No new registry, stock, account or wire/schema field appears.

`logs/probe-extraction-verification.json` SHA256 `6785d60976c3572b3606b54b91bb66c14b523e4bb3586c121e5d39c645f233af` binds77 current sources,83 direct identities and the exact paid9 interception producer. Both438 builds pass9744 focused assertions/11cases and8998 native/save assertions across11 processes: five original stages, three unchanged own-resaves, two actual own-resave next actions and one focused burst. Original miner16/gen2 earns stopped37, active37, first74, second partial111 and complete740-work boundaries. The full actual20NORE hold leaves1740 of1760 body units, no lease and Return state; original two shells and allowance2 remain. Each actual opportunity verifies500active/100idle per74-day remainder accounting. Stopped attachment follows current body motion without earning work.

Independent SVST/VEHS/ASTR/CDIM/CAPA decoding verifies exact quantities, work, current body-relative poses, original paid content2 receipt and positive asteroid0 packet references with unchanged production coordinates, nondecreasing age, invalid pickup and zero feeder credit. Thirty physical families plus CDIM/SPRC match150physical/tenmetadata own/resume comparisons per build and300physical/20metadata between builds. Isolated actual legacy hulls separately contend for one real shared lease, recall/reacquire, preserve stopped transforms and exercise one-unit reserve/hold tails. The deterministic allocation case is an additional component admission refusal, not native physical heap exhaustion or paid procurement. Adjacent current heavy/pursuit/shared-approach callers pass. Final Releasebuild2/Debugbuild1, all selectors1 and audit1 pass; initial test include/overload compile diagnostics are excluded. Inherited starting-content procurement remains unverified. Continue11.

### Local Mandatory Reading - Retained cargo detachment and return geometry

_Applies to Group11 only._ Read Global and Probe mission controller Area. Inspect `ReleaseSpaceMiningSlot`, native Return movement, shared approach occupancy/staging and original inclusive circular `IsInsideLaunchCenterDropZone`/release authority; retain precise service berths as separate producers. Consume the exact accepted10 loaded paid save and its original body/hold/provenance receipt. Inspect existing geometry, recall and lost/full/foreign-center tests before changing gameplay. Verify safe shared-policy departure, deterministic circle entry, current-position admission and unchanged retained cargo while invalid/full/foreign targets wait or require explicit retargeting. Separate controlled boundary fixtures from actual paid travel; save and resume original release-region entry without changing pose, reserve, allowance or stock. Later descent/service owns actual cargo handoff and payment.

### Group 11 - Detach and return to an authorized release region

{{HARD}}
- [X] 11.1 Release the attachment lease and choose a safe approach-exit position via the shared occupancy policy; preserve held cargo and saved work history.
- [X] 11.2 Route toward a deterministic reachable point inside the home center circular region rather than the exact anchor. Recompute when the craft is still outside, and check current position at command execution.
- [X] 11.3 Wait or request explicit retargeting if the home center is invalid/full; overlap with another circle does not authorize foreign delivery. Keep exact service-docking orders separate from cargo-only returns.

**Retained-cargo departure and return verified,2026-10-17.** `space_departure.h` publishes the native `GetSpaceMiningDepartureDestination` query over the original shared staging/occupancy owner. Inside the original body's two-tile approach envelope, existing clear candidates are preferred toward the actual onward home; absent admission retains the full hold and Return state. Actual movement clears the envelope before ordinary circle entry. Existing waiting-request ring order is unchanged. A homeward preference prevents departure from oscillating back into the body envelope. No saved waypoint, phase, timer, lease, cargo owner or schema field is added;438 remains valid.

`logs/probe-return-verification.json` SHA256 `b1c57dee86998772c8dad1b13a091f432656ee6ee39840ef6d6a25e4e2e47953` binds86 sources,57 direct identities and the exact10 paid full-hold producer. Both438 builds pass11073 focused assertions/19cases and4532 native/save/component assertions across eight processes. Three actual paid travel stages preserve all20NORE,1740 reserve, original packet IDs/quantities/production coordinates, nondecreasing age and unchanged allowance/pods. Exit advances by an actual bounded movement step; outbound clears the original body; Holding stops inside home4/gen3's inclusive20-tile circle at a nonanchor point distinct from precise service. Valid/stale-generation release quotes preserve funds, cargo and pending jobs; no descent is committed here. Two unchanged own-resaves and one actual own-resave arrival match90physical/sixmetadata comparisons per build and180physical/12metadata between builds.

Separate unsaved component runs use a valid temporary foreign identity without funds/assets and explicit capacity fixture stock to qualify foreign/full refusal on the actual release predicate. They retain the same original20-unit hold and zero pending descents and emit no candidate save; supplied fixture stock is not a native paid procurement claim. Independent SVST/VEHS/ASTR/CDIM/CAPA/LCTR decoding verifies actual retained packets and circle geometry. Shared arbitration, exact circle/edge/overlap, original logistics and direct extraction/heavy callers pass. Final Releasebuild3/Debugbuild1, Release native1/own1/resume1/focused2/boundaries1 and Debug all1, plus audit1 pass. Initial missing Station include and helper-entry path diagnostics are excluded. Inherited starting-content procurement remains unverified. Continue12 actual descent/repeat/service.

### Local Mandatory Reading - Actual descent, paid service and next rendezvous

_Applies to Group12 only._ Read Global and Probe mission controller Area, the completed Plan9 orbital resupply/repeat public handoff and the relevant revised Space service/launch numerical rows. Inspect original `CanReleaseOrbitalCargo`/`CommitOrbitalCargoRelease`, SPOD/SPDJ custody/recovery, actual `SpaceManifest` pending/remaining/service fields, `CmdServiceLaunchProbe` and derived `IsSpaceServiceWorkTick`, typed Return/Service navigation, original paid shell packing and same-rocket Craft resupply. Consume the exact accepted11 region paid save; keep its full/foreign component fixture outside all successors. Require actual source packets, expired quotas, paid parts/work/fees and transported original shells; no allowance, material, pose, source account or service time is installed. Save actual pending descent and service next actions and compare once-only recovery/rearming/retry before next Mine. Retain completed12B's paid third-collection proof as a dated producer boundary without requalifying old binaries.

### Group 12 - Hand off descent and repeat after paid service

{{HARD}}
- [X] 12.1 Call r2mlvs9 release preflight/commit, retaining the probe hold until descent ownership actually commits. Record the returned descent/job identity so a retried mission tick cannot release twice.
- [X] 12.2 Implement paid servicing, rearming/pod replacement and budget exhaustion from the profile. The probe stays in orbit and can repeat without another rocket launch or station.
- [X] 12.3 Enter the next rendezvous only after the cargo handoff/service conditions succeed. Save/reload at that boundary and verify no duplicated pending hold or free service.

**Actual descent and paid repeat verified,2026-10-17.** Existing original producers supply this group without new runtime state or schema. Native monotonic SPDJ identity, exact paid shell custody, reserved pending allowance and the emptied sender hold prevent a second release. Typed Unload uses the same preflight/commit and advances its saved cursor only after transfer; ReturnService retains pending claims and does not top up a partly used allowance. Actual onboard completion alone consumes RFMT1/ELEC1 and replaces quota after148 paid work opportunities. Original two-return batches repeat without another probe, station or per-batch rocket launch.

`logs/probe-repeat-verification.json` SHA256 `4538c0a58ef7d1440788df02b1ff6355bdf8d02defbab0e67d4e4f3e7b6dc952` binds the exact11 paid region producer and current original descent/recovery/resupply/service/rendezvous owners. Both438 builds pass886 focused assertions/14cases and8587 native/save assertions across15 processes. Eight native stages retain actual40 mined NORE/reserve1720, commit two20-unit descents, expend original paid shells0/gen1 and1/gen2 once and exhaust quota2->1->0. The native pad queue selects original paid rocket19/gen5, preserving rocket15/gen1; actual unload retains carried constructor18/gen4 in Surface storage. Two original paid stored shells2/gen3 and3/gen4, RFMT1/ELEC1 and real fuel move through original packing/flight/recovery commands. Native fuel burn15 and Surface fuel debit15 are checked. Resupply leaves quota0. Initial service history1 becomes2 only after actual148 work costs1000, consumes both real parts once and restores quota2; only then the same probe starts its next rendezvous.

Three unchanged own-resaves and three actual own-resave next actions compare180physical/12metadata per build, and14 checkpoints compare420physical/28metadata between builds. Independent SVST/VEHS/ASTR/CDIM/CAPA/SPSU/SPOD/SPDJ decoding verifies job identities/countdowns, actual positive conserved asteroid packets, exact paid shell generations/capital, parts and allowance history. Final Releasebuild3/Debugbuild2, Release native-a2/native-b3/own1/resume1/focused1, Debug native-a2/native-b1/own1/resume1/focused1 and audit1 pass. Initial fixture assumptions about pad queue, retained passenger and service history are excluded/corrected; no producer bypass or supplied stock enters successors. Dated12B retains the completed paid third collection and interrupted-service proof. Inherited starting-content procurement remains unverified. Continue13.

### Local Mandatory Reading - Paid repeat and orbital resupply batch

_Applies to Group12B._ Read Global, the Probe mission controller Area, Plan9's
Orbital resupply and repeat Local block, and the revised Space numerical budget.
Use the actual saved `SpaceManifest` return/pending/service fields and native
pod/descent owners. `IsSpaceServiceWorkTick` derives work/upkeep from current
state; an unsaved previous-tick flag cannot govern continuation. Require Plan9
Group11B's tested same-rocket transfer before claiming repeat acceptance. Preserve
legacy paid identities and do not grant old saves free pods or service quotas.
Implement the complete repeat/resupply behavior and fixtures before one build.

Context has compacted 1 times while executing this group.

### Group 12B - Pause exhausted mining and complete a paid third collection

{{HARD}}
**Verified414 batch,2026-10-13; both configurations passed.** Paid mining now derives a
shared wait for reserved return allowances, service or actual pods. Direct
assignment refuses unavailable work; queued Mine retains its cursor and cargo,
releases its lease and waits in Holding. Current eligibility also governs upkeep.
Manual onboard service may preserve that waiting order through148 paid work
ticks, pause/reload and atomic RFMT1/ELEC1 consumption; completion permits a new
approach without a new probe purchase. Old P2 mining behavior is grandfathered
without a free return quota. Actual paid resupply may create its previously
absent native compartment owner with zero allowance; earned service remains
required. The inspector exposes parts, pods, pending returns and service work.
Plan9's connected fixture/checker is registered for55 review contexts, including
eight genuine413-to414 ordinary migrations. Both configurations compile, pass
713 contract assertions in17 cases and complete the native three-return loop:
60 actual mined tonnes, three expended paid pods, replacement pods/parts from
the same rocket, one148-tick paid service and exact interrupted/paused replay.
Service work costs1,000 and the74-tick pause costs100, with no reload charge
difference. All17 checked owner families remain exact across continuations and
the older-save migrations. RelWithDebInfo's connected checks and Debug's full
15-check pass are indexed by `logs/space-resupply-verification.json`, including
the shared helper filename correction resolved without a rebuild. Full Core,
construction/processing, disposition, art and integrated/network requirements
remain open. Further work stops after this batch at the user's request.

- [X] 12B.1 Gate new/continued extraction on available paid return allowance and
  pod/service state through the same eligibility rule in direct missions, typed
  orders, upkeep and UI. Pending descents reserve their allowances; a waiting
  probe owns its existing cargo but cannot extract a third batch for free.
- [X] 12B.2 Keep exhausted orders recoverable with visible waits for pending
  returns, parts, pods or service. Permit explicit onboard service of the waiting
  mission and resume it only after its148 paid work ticks consume RFMT1/ELEC1
  once. Preserve partial service, cancellation and legacy migration semantics.
- [X] 12B.3 Run three real mined/descent batches with one paid probe and the same
  reusable rocket supplying replacement pods/parts. Exercise missing parts and
  pods, service interruption, lost receiver and failed transfer without reserve,
  packet, stock or money duplication. Compare exact saved/replayed owners in both
  configurations; no orbital station is required.

### Local Mandatory Reading - Mission exit and recovery reconciliation

_Applies to Group13 only._ Read Global and the Probe mission controller Area. Inspect the actual `SpaceMissionPhase` fields, assignment/recall/refit guards, native phase switch and shared mining eligibility, plus typed navigation/ReturnService ownership, GUI command controls and same-file mission validation. Use accepted9/10/11/12 and dated12B only as exact producer boundaries. Build a phase/trigger transition table from current source; cover timeouts, absent target/home, full hold, finite depletion, paid service exhaustion and cancellation without inventing target removal or automatic retargeting. Current focused boundary tests must exercise recoverable ownership/refit refusals and retain cargo. Reuse accepted12 for the paid station-free repeat; precise requested docking remains its separate shared berth/service producer. No replay of old complete producer suites or historical executable/source requalification is required.

### Group 13 - Reconcile mission exits and recovery

{{MEDIUM}}
- [X] 13.1 Build a transition table covering timeout, removed target/home, full hold, reserve exhaustion, low service and cancellation for every phase; connect each to the previously implemented recoverable exit.
- [X] 13.2 Verify the same home/target/hold/refit rules in UI, commands and saved mission validation. Keep failed handoffs in their current phase with cargo intact.
- [X] 13.3 Check the basic repeat loop uses the published paid service path without requiring a later station; precise docking/service remains a separate mission when requested.

**Mission exit/recovery reconciliation verified,2026-10-17.** [The phase/trigger transition table](../../../zz-rhr/upgrades/2026-10/r2mlsp-z-z-g-probe-exit-notes.md#phase-and-trigger-table) covers all five native phases, deployment, typed waits, descent and service owners. Actual Recall now retains the last natural body as nonactionable departure/history so the existing shared safe-exit query still applies after an attached cancellation. Return and released lease end extraction; original cancellation still discards the uncommitted attachment interval, preserving emitted packets/extracted history. The inspector now quotes/posts the same native Recall for mining roles instead of installing exact home Visit; other roles retain explicit home navigation. No new flag, phase, stock owner or schema appears.

`logs/probe-exit-verification.json` SHA256 `3c5c5ea4473693346b895aebe5c62dfc5a65b127728bdf0f23d00ab1c254010c` binds98 current sources,57 direct identities and three exact dated paid producers. Both438 builds pass10995 focused assertions/15cases and1444 native/save assertions across seven processes. Original full-hold11 region, actual2-unit/37-partial-work10 attachment and12 active74-work service exercise incompatible target/refit/mixed-query refusals, safe cargo-preserving cancellation and conflicting-service refusal. Actual partial Recall releases its lease, clears only unfinished work, leaves reserve/extracted packets unchanged and physically reaches the original inclusive circle. Running/stopped isolated legacy components additionally exercise the same shared departure; these create no paid procurement claim.

Three unchanged own-resaves and unchanged service-source comparisons match150physical/tenmetadata per build; six checkpoints match180physical/12metadata between builds. Independent current packet, body, circle, manifest, paid shell and descent decoding agrees. Accepted12 supplies the actual paid station-free repeat without requalifying historical binaries; precise requested service remains its separate shared berth owner. Final Releasebuild2/Debugbuild1, both native1/own1/focused1 and audit1 pass. Initial missing service declaration compile diagnostic is excluded. Inherited starting-content procurement remains unverified; visual capture/final art retain14's gate. Continue14.

### Area Mandatory Reading - UI, art and acceptance

_Applies to Groups 14-22, including14B._

**Entry gate.** The following public contracts must exist before dependent implementation in this area: `src/asteroid.h`: `AsteroidBody`, `AdvanceAsteroidField`, plus `SpaceVehicle`, `SpaceMission`, `AssignSpaceMiningMission` and `SpaceVehicle::Tick` in `src/space_vehicle.h`.
Verify each public file/export/schema field with `rg` in the current workspace and inspect its command/field ownership. For a same-plan contract, compile its type/signature fixture or validate its artifact after the preceding producer groups. For cross-plan behavior, also run the producer's focused executable boundary test; declarations alone cannot authorize dependent mutations or runtime claims.
If a required export is absent, stop the dependent implementation, mutation and runtime claims in Groups 14-22. Read-only characterization, source/range verification, test-case design and asset-template work that does not assume the missing behavior may continue.
Do not bypass this gate by copying private pool/tile state, inventing local IDs or cargo registries, using a historical plan as a delivered implementation, opening user-wide content stores, substituting a second utility/accounting authority, or adding an unreviewed fallback path. Finish the named producer or revise the shared contract and its direct consumers first.

| File name | Line Ranges | Description |
| --- | --- | --- |
| [src/saveload/vehicle_sl.cpp](../../../../src/saveload/vehicle_sl.cpp) | 1119-1196 | `VEHSChunkHandler` and registration construct the vehicle class from saved type before member loading; protect old enum meanings and validate new references. |
| [src/network/network_command.cpp](../../../../src/network/network_command.cpp) | 256-288 | `NetworkExecuteLocalCommandQueue` executes agreed frame order/company authority; all jobs use deterministic simulation time. |
| [src/network/network_server.cpp](../../../../src/network/network_server.cpp) | 670-687 | `ServerNetworkGameSocketHandler::SendSync` compares RNG seeds, not a full gameplay checksum; validate orbital/geology/cargo ledgers in addition to a no-desync result. |
| [src/asteroid.h](../../../../src/asteroid.h) (published generation/body contract) | Live definitions; no fixed range | [r2mlvs10-asteroids-and-mining-probes.md](r2mlvs10-asteroids-and-mining-probes.md) Groups 1-7, ending with Group 7's public-contract task, publish `AsteroidBody`, `AdvanceAsteroidField`, plus `SpaceVehicle`, `SpaceMission`, `AssignSpaceMiningMission` and `SpaceVehicle::Tick` in `src/space_vehicle.h`. These exports/schema fields are authoritative for this area. Same-plan consumers follow those earlier groups; cross-plan consumers verify the producer's required implementation/tests as well. Proposed names alone do not prove delivery. |
| [Sprite Contract and delivery owner](r2mlvs13-graphics-and-asset-pipeline.md#public-handoff-contract) | Live definitions; no fixed range | `Sprite Contract Handoff` defines manifest fields `asset_id`, `consumer`, `directions`, `frames`, `anchor`, `source`; the matching authoring/conversion/runtime groups deliver actual assets. Verify the requested family and packed loading fixture before graphics integration; template/manifest publication alone is not final art. |

**ASSUMPTION: this was already built previously. If not, re-asses at runtime**. This applies only to the producer contracts named in this area's gate, after their listed producer groups have run. Verify their actual exports and focused tests before using the sketches below; proposed types and helper names in sketches are not evidence of delivered code.

**Scoped graphics gate. ASSUMPTION: this was already built previously. If not, re-asses at runtime**. Final graphics tasks in this area require the corresponding r2mlvs13 manifest entries, delivered sprites and packed loading fixture. Verify them before binding/review; if absent, stop only those art tasks and their acceptance claims, continue independent code/state tests, and never count placeholders as finished assets.

Context has compacted 2 times while executing this group.

### Local Mandatory Reading - Native inspector disclosures

_Applies to Group14 only._ Read Global and UI/art/acceptance Area. Inspect `GetAsteroidQuantityDisclosure`, actual company scan knowledge, selected native role/order/work/service/packet owners and `SpaceSurveyWindow` drawing/picking/command handlers. Public packed Sprite Contract rows for the three asteroid classes and Space craft currently retain planned/null sources; Plan14 Groups63/63B/63C/64/67-69 own their source art. Under the Area's scoped graphics gate, complete independent native inspector fields/captures here and retain final source/packed/runtime art binding in Group14B after22. No existing temporary sprites count as final. Derive displayed composition, signed trajectory/speed and state progress from original native fields without revealing unsurveyed quantities, modifying RNG, or creating another inventory/work clock. Reuse existing capture/runtime tooling with exact paid/surveyed/unknown saves, and test read-only company isolation before current UI captures.

### Group 14 - UI, art and acceptance

{{MEDIUM}}
**Scoped split,2026-10-17.** The Area explicitly permits independent code/state work while final graphics producers are absent. Original14.1's final r2mlvs13-art obligation remains unchecked as14B below; that group follows independent14-22 work and cannot use temporary preview sprites as completion evidence.

- [X] 14.1 Add asteroid selection with survey-gated initial/remaining per-cargo quantities, class/composition, trajectory direction and speed, role-aware order editing and mission status. Show Unknown before a company scan completes, plus scan progress, construction deliveries, attachment/mining and return-pod states. Verify current native inspector captures; final r2mlvs13-art binding/review remains14B.

**Functional inspector verified,2026-10-17; final art remains14B.** New `asteroid_inspection.h/.cpp` publishes pure company-authorized composition/quantity and signed native motion presentation. It delegates quantity authorization to the original survey owner; unknown observers receive no copied raw reserves. A fixed32-candidate-bit integer display magnitude and explicit direction signs preserve reflected/sub-resolution motion without advancing simulation. The native inspector now shows actual primary resource, signed trajectory/speed, exact original Construct target deliveries/consumption/work and selected-sender-generation descent payload/countdown/status. Non-mining typed work/waits no longer appear Idle; eligibility alone no longer appears Working. Existing role-aware commands, scan, mining, parts and paid service remain their original owners.

`logs/asteroid-inspection-verification.json` SHA256 `dd738d03ac7c633c3749fa6307b25ec3e06ac6dcb19a428f7eaf10e20f7c2294` binds current code and exact paid/scan producers. Both438 builds pass641 focused assertions/eight cases and435 native assertions across12 processes and11 captures each. Eight native views cover unknown full hold, actual partial mining, pending descent, active service, partial scan, completed survey, another company's Unknown view and actual37/740-work construction. Original430 scan and the one original Release415 construction input cold-load into current438 before/after snapshots; no historical Debug construction producer is claimed. All originals and their dated native writer/procurement limits remain intact.

Actual native widget captures are encoded to PNG with identical PPM pixels and retained writer provenance. Opening/viewing preserves every resource owner, money, RNG, work, clock and sparse company knowledge. Three ordinary reload captures remain identical;420physical/28metadata comparisons pass per build, and22 checkpoints plus11 images match660physical/44metadata and11pixel comparisons between builds. Actual known/foreign/pending/construction images were visually reviewed; no missing fields or clipping. Final Releasebuild3/Debugbuild1, Release native-a2/native-b2/own1/focused2, Debug native-a1/native-b1/own1/focused1 and audit1 pass. Initial string-parameter compile diagnostic and pre-refinement status captures are excluded. The public Space family manifest still has absent/planned sources and no required packed fixtures; temporary preview sprites are not final art. Continue15, retaining14B unchecked.

### Local Mandatory Reading - Interception, tie order and saved edge continuation

_Applies to Group15 only._ Read Global and UI/art/acceptance Area. Inspect the existing Group3 maximum-drift/basic-probe/starter-budget tests and frozen starter-distance contract, actual `GetSpaceMiningApproachTicks`/`SpaceVehicle::Tick`, shared body approach request priority/admission and exclusive mining lease. Inspect current motion/reflection and direct competing-probe fixtures before adding coverage. Require multiple actual native probe initial offsets, same-tick commands in both orders with the documented deterministic tie, and approaching/latched edge save/reload next-step comparisons. Declared isolated geometry/legacy hull fixtures are not paid procurement; retain exact accepted9/10 paid pursuit/extraction indexes as dated producer boundaries without replaying old complete suites or historical binary/source graphs. Use public field installation/validation and original saved ASTR/VEHS/SPOR fields; no copied movement, local lease registry or artificial progress can substitute the native transition under test. Keep final art14B's gate separate.

Context has compacted 1 times while executing this group.

### Group 15 - Test interception and exclusive attachment

{{HARD}}
- [X] 15.1 Run deterministic moving-target pursuit from multiple initial offsets and across the body edge policy, including Group 3's maximum-drift/basic-probe interception fixture and guaranteed starter-distance bound. Assert starter interception within its budget, and finite completion or the declared timeout for other cases, with bounded per-tick work.
- [X] 15.2 Issue two attachment requests for one slot in the same simulation tick and in reversed command order; verify the documented tie rule, one lease and recoverable waiting for the loser.
- [X] 15.3 Save/reload while approaching and latched near an edge; compare body position, attachment offset and next-step decision.

**Native interception verified,2026-10-17.** `logs/probe-interception-verification.json` SHA256 `e6c11d97bb05fa201509aeac83a1072824e98092f8e7c5be5e8958b3a8513687` binds85 sources,68 direct identities and exact dated9/10 paid producers. Both438 builds pass20,290 focused assertions/11cases and258 native/save assertions across nine processes. Actual isolated native hulls intercept from five offsets at maximum canonical drift; the32-tile basic case meets the632-opportunity bound. All three genuine generated starters retain their historical input and intercept within the frozen assignment. Two same-tick command orders select lower native hull ID despite its greater initial distance; one shared lease exists and the waiting loser recovers after recall.

Original paid approach16/gen2 and partial attachment16/gen2 supply unchanged cargo, service and finite reserve owners for explicitly declared edge geometry. Staging changes only existing pose/canonical motion, earns no travel or work, and preserves elapsed time. One real native opportunity reflects X by222Q16 units, reverses velocity and retains remainder29; latched work37 becomes38 without extracting another unit. Two ordinary stage resaves and two resumed next steps match120physical/eightmetadata families per build; eight checkpoints match240physical/16metadata across builds, including SPOR admission, VEHS decisions, attachment offsets and packets. No production/schema change or paid geometry claim. Releasebuild1/Debugbuild1, all four selectors1 and audit1 pass. Continue16; final art14B remains open.

### Local Mandatory Reading - Exit boundaries and lost identities

_Applies to Group16 only._ Read Global and UI/art/acceptance Area. Inspect actual native phase exits, finite extraction commit, paid service eligibility, typed cancellation and original pod/descent custody. Reuse accepted9/10/11/12/13/15 indexes as dated boundaries; inspect focused tests before adding only missing coverage. Verify partial/full/one-unit/service exits separately and cancellation during approach/mining/detachment/pending descent. For home retirement/reassignment, use original generation-checked launch-center/retarget commands and existing guards; never invent automatic alternatives or reused-ID ownership. Inspect the supported body command surface before testing depletion/removal: natural finite depletion must use actual extraction, and unsupported removal is a refusal/characterization rather than a new destructive gameplay feature. Clearly label isolated component inputs; do not present copied service state, geometry or reserve as paid progress. Preserve original packet IDs/provenance, lease and recovery owners across all cases. Group20 retains the deferred legacy Recall wire-generation audit.

### Group 16 - Test mining exits and lost mission references

{{HARD}}
- [X] 16.1 Test partial/full holds, last-unit reserve and service exhaustion separately; compare exact extracted quantities and retained cargo for each exit.
- [X] 16.2 Cancel at approach, mining, detachment and pending descent; validate lease release and packet ownership without repeat extraction.
- [X] 16.3 Remove/reassign home and deplete/remove the body through supported commands; check the mission never follows a reused ID and selects only explicit valid recovery alternatives.

**Native exit boundaries verified,2026-10-17.** `logs/probe-boundary-verification.json` SHA256 `9ef31a97fb07e69f30439bd91baacb6b158c2ac89f2b533b291375fe2fb6bfe2` binds exact9/10/12 paid inputs and original paid alternate-recovery reports. Both438 builds pass3513 focused assertions/ten cases and1002 native/save assertions across13 processes. Existing original partial/full production remains accepted. A declared one-unit reserve component now earns73+1 native work opportunities and emits exactly one packet; another75 steps retain the exhausted body ID/record, original initial reserve and emitted packet. It claims no paid depletion or installed ready work. Actual spent two-return allowance refuses quote/execute/mixed-query pursuit without mutation.

Actual paid Recall at approach,2-unit/37-work mining,full20-unit detachment and pending20-unit descent releases any mining lease and runs75 actual opportunities without new extraction. Only unfinished work resets; held quantities, extracted history, original packet IDs/coordinates, paid shells, quota and service history survive. The already released descent remains its own sender-generation/pod/job/packet owner and advances111->36. Occupied paid home demolition refuses both quote and execution; wrong recovery/shell generations cannot redirect its original job. Existing focused retired-center tests exercise same numeric StationID/new generation and explicit authorized retarget; exact accepted paid retarget reports retain the purchased alternative rather than inventing another recovery owner.

**Supported lifecycle boundary:** there is no gameplay body-delete command and no arbitrary probe-home setter. Exhausted bodies remain moving records with unreused IDs; invalid/out-of-range assignments refuse. Home demolition with paid craft/pods/recovery is guarded; actual explicit descent recovery uses `CmdRetargetSpaceDescent`, not an automatic reassignment. Testing those supported refusals/retained identities satisfies16.3 without introducing destructive gameplay. Six ordinary resaves and two unchanged refusal-source pairs match240physical/16metadata per build; twelve checkpoints match360physical/24metadata between builds. Releasebuild2/Debugbuild1, all selectors1 and audit1 pass; initial missing command include diagnostic is excluded. No production/schema change. Continue17; legacy Recall wire-generation audit remains20 and final art remains14B.

### Local Mandatory Reading - Conserved resources and exact phase continuation

_Applies to Group17 only._ Read Global and UI/art/acceptance Area. Inspect original ASTR finite per-body/class fields, same-file CDIM meanings, actual VEHS/CAPA packets and original SPDJ/SPOD/SPSU ownership. Reuse accepted9/10/11/12/15/16 direct indexes and exact paid saves without reopening old source/executable graphs. Reconcile every asteroid resource separately, including unchanged classes; positive emitted packets must belong to exactly one original physical holding. Preserve paid return-cycle evidence and distinguish RFMT/ELEC service consumption from raw ore or processed products. Round-trip actual approach, first attachment, partial mining, detached full hold and pending descent, then earn one actual native opportunity from the original and its own resave and compare every original physical owner, saved remainder and next decision. No installed phase, ready work, pose, reserve, cargo, replacement shell or elapsed tick may substitute these paid sources. Keep isolated16 final-tail fixture outside paid conservation and keep final art14B separate.

### Group 17 - Verify mission conservation through saved phases

{{HARD}}
- [X] 17.1 Track each resource separately: initial reserve equals remaining reserve plus probe/descent/Surface inventory plus recorded final consumption/disposal. Do not equate product units from later processing with unconverted ore.
- [X] 17.2 Round-trip Rendezvous, Attach, Mine, Detach and DescentHandoff with their phase/lease/service remainder; compare the next exact simulation step.
- [X] 17.3 Repeat multiple return cycles and reconcile split/merged packet quantities, age/provenance and one container owner at a time; no new reserve appears after docking or reload.

**Conserved native phases verified,2026-10-17.** `logs/probe-conservation-verification.json` SHA256 `901ca9ba65d9fb45a21d7ac890fd7edb7c3c09ebd09171f5461753f5abd2bc2a` binds90 sources,131 direct identities and exact9/10/12 paid inputs. Both438 builds pass330 native assertions across ten processes. Five actual saved phases cover Rendezvous/Approach, first Attach,37-work Mine, full-hold Detach/Return and111-tick DescentHandoff. Each process writes an unchanged own save, earns exactly one original synchronized opportunity, then writes next; a second process consumes that own save and repeats. Saved admission/lease, attachment offset, travel/cost/work remainders, packet age/provenance and descent110 decision match450physical/30metadata comparisons per build and600physical/40metadata comparisons across twenty checkpoints between builds.

The independent wire audit joins every ASTR body debit to original CAPA packets, actual VEHS Space/Surface holds, STNN goods/next-hop buckets and SPDJ descent references. It resolves cargo meanings through same-file CDIM, checks every asteroid-source packet has exactly one physical owner, and reconciles Nickel Ore, Water Ice and Rare Earth separately, including untouched classes. These raw-ore worlds have no raw-ore consumption/disposal; RFMT/ELEC service inputs and processed products cannot substitute ore. All28 exact accepted12 paid return checkpoints also reconcile: first20 units descend and recover; both20 returns leave40 real Surface ore units, original body0 reserve1720 and aggregate Nickel reserve5061 of5101. Original shell replacement, completed paid service1->2, parts consumption and quota2 before next rendezvous remain their dated native producer. No copied ready work, pose, reserve, clock or inventory is installed. No production/schema change. Final Releasebuild2/Debugbuild1, both native1/resume1 and audit1 pass; initial missing company include diagnostic is excluded. Continue18; final art remains14B.

### Local Mandatory Reading - Three-resource missions and inclusive drop boundary

_Applies to Group18 only._ Read Global and UI/art/acceptance Area. Inspect actual class cargo definitions/dimensions, original mission assignment/refit/pursuit/extraction/return, inclusive `IsInsideLaunchCenterDropZone` and generation-checked release/recovery commands. Reuse exact accepted9 original empty paid hull and17 per-resource/unique-owner decoder. Run a complete actual mission for each original resource class with no installed work, cargo, reserve or paid travel. Record real off-anchor circle entry and original shell/job/packet recovery. Inspect accepted11 boundary tests before adding only missing exact20-tile release coverage. If exact boundary geometry needs an explicit isolated control, label it separately from earned full-mission travel, preserve the actual original paid hold/shell and use original release commands; never count artificial pose as earned route evidence. Keep final graphics14B's scoped gate separate and retain all original paid inputs.

### Group 18 - UI, art and acceptance

{{MEDIUM}}
- [X] 18.1 Run one full mission for each asteroid class, including drops away from the center and at the 20-tile boundary.

**Three resource missions verified,2026-10-17.** `logs/probe-class-verification.json` SHA256 `058483cef0fa7fcaa6e51186ea1a3c8f70ced6854a5aef419345af88a94b960e` binds exact original9 empty paid miner16/gen2, its original100000 content2 receipt/home4/gen3 and two actual shells. Release/Debug438 pass13,001/12,999 native assertions across twelve processes. Independent fresh original worlds for Nickel Ore, Water Ice and Rare Earth each earn pursuit/attachment,740 extraction work for a20-unit hold, safe off-anchor circle entry, actual generation-checked shell release and148 native descent opportunities. Surface receives20 units, the exact shell is expended once, quota2->1 and paid service history remains1. No pose, stock, reserve, work, clock or replacement allowance is installed on these full-mission paths.

Three separate boundary forks consume those actual earned holds. Explicitly declared geometry places only the original hull one Q16 unit outside20 tiles (quote and execution refuse), then exactly20 tiles (actual release/recovery succeeds). These forks claim no earned travel; paid resources/history remain their original owners. Same-file SVST/VEHS/CDIM/ASTR/SPSU/SPDJ/LCTR decoding checks all three resource meanings, original sender/model/mass, exclusive packet holdings, real off-anchor release points, exact boundary, finite debit and recovery custody. Three ordinary recovered resaves match90physical/sixmetadata per build;24 checkpoints match720physical/48metadata between builds. No production/schema change. Final Releasebuild3/Debugbuild1 and all twelve selectors1/audit1 pass. Initial safeguarded-parser/Catch assertion compile diagnostics and the preflight's mistaken111 literal are excluded:111 was an earlier partially elapsed checkpoint; fresh descents correctly start148. The native fixture always used actual saved countdown; its comment was corrected without runtime change. Continue19; final art remains14B.

### Local Mandatory Reading - Linked terrestrial collection and useful receivers

_Applies to Group19 only._ Read Global and UI/art/acceptance Area. Inspect original native launch-hub membership, Surface loading and actual train/lorry cargo transfer, plus supported receiver/recipe consumption and payment. Reuse accepted Plan9 Surface collection/commodity-loop producer indexes and exact original paid returned packets as dated boundaries; inspect their native scenario membership before adding missing work. Run the relevant current focused producer boundary tests before consumer claims. Verify both linked terrestrial vehicle families collect actual returned cargo and deliver to useful receivers with source/age/packet conservation and existing payment rules. Keep imported starting-stock/procurement limits explicit and never grant replacement cargo, invent a second stock/recipe account, count processed product units as raw ore, or requalify old executable/source graphs. Group18's actual three-resource returns can supply new consumers only where additional evidence is required.

Context has compacted 1 times while executing this group.

### Group 19 - UI, art and acceptance

{{SIMPLE}}
- [X] 19.1 Collect returned cargo through both linked Surface train and lorry stations and deliver it to useful receivers.

**Native Surface delivery verified,2026-10-17.** `logs/probe-surface-verification.json` SHA256 `9d69de693f8ede26289b207cae2dec431159019de1cafff85a9e9d9f5f0ce57b` binds the original18 paid Nickel return and two independent ordinary carrier continuations. Each438 configuration passes251 focused assertions/three cases and14,667 native assertions/nine processes. Normal launch-hub commands link actual rail5/lorry6 stations; actual timed handling splits the original20 units10+10. Normal paid construction, native refinery construction time, vehicle purchases/orders and physical trips deliver10 units to the native NFMT input. The other linked station retains10. Actual payment completes before the final save; no pending carrier payment remains.

Independent same-file CDIM/CAPA/VEHS/STNN/INDY/LPCR/LPCC decoding reconciles original body0 reserve1760 =1740 remaining +10 raw packets +10 native unconverted input. NFMT awaits Coal; no Coal is borrowed, consumption/product remains0, and processed units never replace raw ore in this equation. Original paid miner/home/shell/quota/service history and all other resource classes are retained. Thirty-five owner families include native Industry, processor history and CAPY payment custody: unchanged own pairs match70 owner/four metadata comparisons per build; eight checkpoints match280 owner/16 metadata between builds. Final Releasebuild8/Debugbuild2, native selectors3/2, focused3 and audit1 pass. Provisional route/edge/payment snapshots are excluded. No production/schema change; inherited starting-stock procurement remains outside the original producer boundary. Continue20; final art remains14B.

### Local Mandatory Reading - Visibility and synchronized mission replay

_Applies to Group20 only._ Read Global and UI/art/acceptance Area. Inspect current mission command wire types, generation references, all mission frontend posts, native command serialization/server-frame dispatch and existing heavy/product network fixtures before choosing missing coverage. Preserve legacy argument layouts and native authority; a delayed command must not act on a reused vehicle incarnation. Compare actual visible/hidden simulation, original paid mining/return/recovery owners and financial/native Industry/payment custody, beyond RNG-only synchronization. Distinguish native queued dispatch from real socket multiplayer; retain unsupported gates explicitly. Reuse original paid checkpoint/index identities as dated boundaries, never install mining work, cargo, money or elapsed time as mission evidence. A cloned loopback socket host may configure company access through the native company command before its first join; record that endpoint setup and prove no money, cargo, work, hull or clock grant. Require actual joined company membership before client commands. Batch related protocol/frontend/test changes before the two-worker builds. Do not preload performance21 or final-art14B sources.

### Group 20 - UI, art and acceptance

{{HARD}}
- [X] 20.1 Compare visible/hidden layer results and multiplayer replay.

**View and native multiplayer replay verified,2026-10-17.** `logs/probe-replay-verification.json` SHA256 `30478028651def2e426437fa1440df66a348dc146cd07b0be3c66a221d6a2f2c` binds116 current source files and228 direct evidence identities. Each438 build passes1178 focused assertions/three cases and nine actual processes; Release/Debug native assertions are34,465/34,495. Four complete original paid20-unit missions cover local/queued commands with Space visible or hidden beneath Surface. Two original earned two-unit partial loads exercise queued Recall, native return, shell release and recovery under both views. Actual native game opportunities earn all pursuit, extraction, movement and148-tick descent; no cargo, reserve, hull, work, money or interval is installed.

Two independent actual loopback peers also complete the original paid small-miner mission. The real client submits assignment and shell release over its socket, after native company membership is established; both peers retain six identical frame/tick/elapsed boundaries and physical saves through recovery. The cloned host's closed original allow-list requires explicitly recorded native company access setup before joining; this grants no physical/financial resource and the listener binds only loopback. Fifty-five native owner families include original orbital/Surface custody, Industry/history/payment, finite geology/mine records, packed map storage, economy and clock/RNG state. Fifty checkpoints per build reconcile independently. Thirty-eight local/view/own/peer comparisons match2090 owner/76metadata per build;38 deterministic local checkpoints match2090 owner/76metadata between builds. Separate socket sessions may schedule commands on different frames, so exact byte equality is required between peers within each session. Reviewed original visible/hidden native captures retain pixel-identical conversion provenance; final art remains14B.

Production adds `CmdSetSpaceMissionChecked(flags,VehicleID,uint64_t generation,uint32_t body)` as an appended synchronized command. GUI Recall quotes/posts the original incarnation; actual native ID reuse, foreign actors, pure estimates and stale queued Recall refuse unchanged. The original mission argument layout and command IDs remain intact for offline callers/replays; generation-free legacy multiplayer admission now refuses. Final normal game and test binaries pass Releasebuild3/Debugbuild2, all final selectors3/2, both real peer sessions and audit1. Initial closed-company/spectator and superseded-binary attempts remain provisional. No save-schema change. Continue21.

### Local Mandatory Reading - Exact population work and search costs

_Applies to Group21 only._ Read Global and UI/art/acceptance Area, the exact Population defaults table and its10/160/10,240/14,336 examples. Inspect current public generation, canonical motion, body lookup/spatial index and pursuit/arbitration owners, existing operation instrumentation and focused maximum/clustered/narrow fixtures before adding missing work. Measure actual native owners at those exact counts, including clustered drift and narrow maps; do not substitute proxy lists or a second target/accounting authority. Reuse delivered generation/index focused boundaries and add only missing generation, target-search, per-body/per-probe cost records. Exact work counts and bounded queries govern acceptance; elapsed CPU timings are diagnostic rather than fragile wall-time thresholds. Keep declared isolated performance worlds separate from paid mission procurement. Avoid all-pairs per-tick scans, run only focused current-group bursts, and split any scope exceeding two minutes. Do not preload22 or final-art14B.

Context has compacted 1 times while executing this group.

### Group 21 - UI, art and acceptance

{{MEDIUM}}
- [X] 21.1 Measure generation, target search and per-body/per-probe costs at Group 2's 10/160-body small/normal examples, 10,240-body largest Normal map and 14,336-body Dense maximum, including clustered drift and narrow-map cases. Record those exact counts and work bounds; avoid all-pairs scanning every tick.

**Exact native work verified,2026-10-17.** `logs/asteroid-work-verification.json` SHA256 `bc223f9c6a7251cf2981275cc5722bdc64406e1483d25a800b1aee39dd1944c0` binds39 current source files,150 direct evidence identities and30 actual processes. Each438 build passes7,068 measured assertions/14 cases and38,589 focused assertions/13 producer cases. All14 native semantic results, exact operations, finite resource facts and final field digests agree between builds. Final normal game/test binaries pass Releasebuild2/Debugbuild1. The first Release test-only money comparison diagnostic is excluded. Focused commands and original invocation selectors are retained by `asteroid_work_checks-1.py/-2.py` and [performance notes](../../../zz-rhr/upgrades/2026-10/r2mlsp-z-z-o-probe-performance-notes.md).

| Actual geometry / population | Bodies N | Generated grid cells G | Leading candidate visits 3G-3 | Release generation ms, diagnostic |
| --- | --- | --- | --- | --- |
| 64x64 Normal | 10 | 105 | 312 | 0.052 |
| 256x256 Normal | 160 | 1,953 | 5,856 | 0.094 |
| 4096x4096 Normal | 10,240 | 522,753 | 1,568,256 | 14.900 |
| 4096x4096 Dense | 14,336 | 522,753 | 1,568,256 | 22.940 |
| 4096x4096 Dense, declared clustered drift | 14,336 | 522,753 | 1,568,256 | 16.905 |
| 64x1024 Normal | 160 | 1,905 | 5,712 | 0.095 |
| 1024x64 Normal | 160 | 1,785 | 5,352 | 0.102 |

Generation also performs exactly2(N-3) shuffle swaps, N record constructions and24N canonical velocity checks: one-off O(G+N), excluding native Map allocation and Surface launch-site search. Every measured motion opportunity performs exactly N body steps and N native index updates. Clustered departure performs exactly N swap/pop unlinks and links, independent of bucket occupancy; no body-pair pass or unconditional body-by-probe target scan is introduced. Native target lookup remains direct by stable ID. Local +/-2-tile queries visit at most four buckets; copied/sorted candidates C cost O(K+C log C), and the cluster actually returns all14,336. This is an output-sensitive bound, not constant query work.

Actual approaching/mining legacy15 fleets use6 native hulls on the smallest map and64 elsewhere, with one attached hull in the overlapping-body cluster and64 distant approaching hulls there. Shared arbitration retains its existing bounded probe-pair/slot work, O(P squared + P log P), P<=64; it does not scan the complete asteroid population per request. Cold assignment/staging queries are measured separately and can encounter the whole cluster. Warm native craft ticks make at most32 direct target lookups per hull/opportunity and copy no body-query candidates in these scenarios. Thirty-two approaching opportunities charge43 per hull; separate fresh native assignments earn attachment, then74 actual mining opportunities charge500 and emit exactly two accountable units per hull from the original finite reserve. Native component timings are diagnostic, including observation overhead; no paid procurement, whole-game FPS or maximum-map Surface-startup performance is claimed. Existing paid histories remain dated immutable boundaries. Continue22; final art14B remains unchecked.

### Local Mandatory Reading - Public runtime and evidence handoff

_Applies to Group22 only._ Read Global and UI/art/acceptance Area, this plan's Completion Conditions and the accepted public-contract/evidence summaries for7/8B/8C/9-21. Verify live exported signatures and the current39-source performance index, with earlier accepted indexes treated as dated boundaries rather than historical executable graphs. Read only relevant Space paragraphs in vehicle/map-layer architecture and current master queue. Record actual native ownership, focused commands/results, original supplied-save retention and exact remaining art/packed-loading gates; distinguish delivered20 socket replay from broader Plan11 acceptance. Update affected handoffs without marking this source plan or parent complete while14B is unchecked. No new runtime regression is needed for accurate documentation alone.

### Group 22 - UI, art and acceptance

{{SIMPLE}}
- [X] 22.1 Record the implemented public exports, focused test commands/results and remaining runtime/art/network gaps in this plan; retain original supplied saves and repository-local evidence.
- [X] 22.2 Update affected architecture/handoff documentation to describe delivered behavior and reconcile the parent index only when this plan's Completion Conditions are met. Keep every unfinished task unchecked.

**Runtime handoff recorded,2026-10-17.** [Current438 handoff](../../../zz-rhr/upgrades/2026-10/r2mlsp-z-z-p-probe-runtime-handoff.md) records live generation/body/mission/checked-command/extraction/departure/disclosure/measurement exports, original paid owners, exact focused commands and accepted7/8B/8C/9-21 indexes. `logs/probe-runtime-handoff-verification.json` retains direct dated producer identities, both unchanged original paid deployments, current performance/source evidence and documentation-link checks. Vehicle and map-layer architecture now distinguish delivered finite mining, actual paid recovery/Surface delivery, company disclosure, real joined socket replay and exact population work from the dated423 launch and424 generation boundaries. Original supplied/derived saves and their native provenance remain unchanged; no historical binary graph or full regression is rerun for documentation. The master reconciles completed groups only. This source plan and parent remain open while14B requires actual authored/packed family delivery and ordinary-game review; Plan11's broader acceptance remains its own queue.

### Local Mandatory Reading - Final Space family art acceptance

_Applies to Group14B only._ Read Global and UI/art/acceptance Area, the public Sprite Contract Handoff and matching Plan14 Space source/conversion/packed/runtime tasks. Require actual asteroid/mining/survey/construction/return-pod family sources, complete directions/states/frames, their original provenance and packed loading fixtures before binding/review. Keep gameplay quantities, movement, work and custody under original native owners. Temporary code-authored preview sprites and a planned manifest row cannot satisfy this group. Preserve all existing supplied saves and repository-local review captures; final native ordinary-game images must show the requested original14.1 states.

### Group 14B - Bind and review final inspector and mission art

{{HARD}}
- [X] 14B.1 Complete original14.1's final r2mlvs13-art integration using delivered asteroid, mining/survey/construction probe and return-pod family sprites through their packed loading owners, with the existing native state/quantity queries.
- [X] 14B.2 Review actual ordinary-game selection/scan/construction/attachment/mining/return-pod captures with those final sprites; retain source/packed/runtime evidence and keep this group pending until all required families pass.

**Execution split,2026-10-17, retained history.** Final producers were absent when this split began; all seven sources, packed/native binding and ordinary-game review are now complete in the children below. Execute the bounded children below sequentially, preserving original14B.1/.2 as aggregate obligations. Each child consumes the existing public manifest and its corresponding Plan14 source tasks; no unrelated Underground queue is reopened. Source delivery cannot close packing/runtime acceptance.

### Local Mandatory Reading - Small mining probe original and Space source gate

_Applies to Group14BB only._ Read Global, UI/art/acceptance Area and final14B Local, Plan14 Global/Source generation Area and closest Remaining plant and orbital originals Local for63, Plan13 Public Handoff Contract and canonical framed prompt, installed imagegen skill, current `vehicle.model-15` manifest row and actual native model15 mission/work/hold/service/direction exports. Reuse the scoped existing-manifest validator; its unrelated broad mine-hash failure is retained without refreshing that producer. Freeze a single hook/mining silhouette, eight real engine-viewport directions, states, two phases,64x48 canvas/(-32,-32) anchor and original provenance before variants. Author and inspect actual bitmap originals through built-in imagegen; no procedural replacement or rotated flat views. All quantities/work remain native owners. Do not preload survey/builder/pod/asteroid source details while authoring this model.

Context has compacted 1 times while executing this group.

### Group 14BB - Deliver the small mining probe source

{{HARD}}
- [X] 14BB.1 Freeze the native model15 brief, manifest state/direction/anchor policy and scoped source/refusal fixture.
- [X] 14BB.2 Author and review the compact hook/mining model and eight genuine directional views with immutable prompt/reference/source provenance, satisfying Plan14 63.1/63.2.
- [X] 14BB.3 Author required attached/mining/load/service differences, publish native-scale contacts/source coverage and satisfy Plan14 63.3; retain packing/runtime false.

**Small-miner source complete,2026-10-17.** `logs/space-miner-source-verification.json` binds18 immutable built-in originals,128 logical native poses,72 actual directional component poses and32 explicit static aliases. Eight genuine views retain protected original hardware; actual front-tool, two travel pulses, full/blocked access panels and two service phases occupy declared parts. Both native/4x frame contacts are reviewed. Status preserves97,532 outside-panel pixels and all original opacity; travel preserves every original opaque craft pixel.43 focused unit tests, the five-family/576-pose public contract and three refusal cases pass. Five actual alpha failures and the gap-producing whole-nozzle pilot remain excluded with exact receipts. Public model15 source coverage is true; company colors are none, packed/runtime false. Original quantities, cargo, work, leases and service remain native. Continue14BC/Plan14 63B; original14B aggregates remain open.

### Local Mandatory Reading - Distinct survey probe source

_Applies to Group14BC only._ Read Global/UI Area, Plan14 Global/Source generation Area and63B/63C Local, native19 scan/knowledge/service owners and the existing source/manifest/conversion contract delivered14BB. Freeze and author a distinct scanner model and genuine directions; scan effects must follow real native work and confer no quantities. Preserve all approved mining originals.

### Group 14BC - Deliver the survey probe source

{{HARD}}
- [X] 14BC.1 Complete Plan14 63B.1-63B.3 with native model19 manifest, genuine scanner originals/directions/states and reviewed source contacts/provenance; packing/runtime remain pending.

**Survey source complete,2026-10-17.** `logs/space-survey-source-verification.json` binds12 new survey originals and four explicitly registered existing gas-source instances,96 logical poses/64 actual components/16 static aliases, reviewed native/4x contacts and complete-view separation. Actual dish sweeps, model-specific service connector phases and blocked hatch preserve the distinct original bus/paddles/nozzle. Service protects73,620 non-hatch pixels; gas protects every opaque original pixel and touches the original rear edge in both phases. Whole-frame panel shading and first detached-pulse assemblies remain excluded explicitly.34 focused unit/compatibility tests, six-family/672-pose contract with three refusal cases, and41 native role/survey assertions/three cases in each438 build pass. Original native scan work/company disclosure and all saved owners remain unchanged. Public source coverage is complete; packed/runtime false. Continue14BD/Plan14 63C.

### Local Mandatory Reading - Distinct construction probe source

_Applies to Group14BD only._ Read Global/UI Area, Plan14 Global/Source generation Area and closest Native construction carrier Local, native20 cargo/Construct/job/approach/service owners and delivered source/conversion contracts. Steel/Goods loads and active construction derive only from original native cargo/work. Preserve approved mining/survey originals.

Current bounded handover: [construction source notes](../../../zz-rhr/upgrades/2026-10/r2mlsp-z-z-r-construction-art-notes.md); after compaction consume its active source section rather than the full earlier Space-art history.

Context has compacted 1 times while executing this group.

### Group 14BD - Deliver the construction probe source

{{HARD}}
- [X] 14BD.1 Complete Plan14 63C.1-63C.3 with a distinct manipulator/material carrier, genuine directions, actual empty/Steel/Goods/constructing/service states and reviewed provenance/contacts; packing/runtime remain pending.

**Construction source complete,2026-10-17.** `logs/space-builder-source-verification.json` SHA256`e8d444442a77b971282314f3c989b77c120a8ce40b5b65181ca3720540b24fd8` binds16 own actual originals, four explicitly registered original gas sources,128 logical poses/80 actual component poses/32 static aliases and reviewed native/4x contacts. All eight genuine headings retain exact original hardware outside actual load/arm/hatch parts; two actual manipulator phases differ27-96pixels, service3-5 and gas3-6. All24 hatch cues retain real red/cyan source pixels. Loads protect48,230 outside-volume pixels, work44,722 outside-swept pixels, status73,608 non-hatch pixels and gas every original opaque pixel. Whole-source layout has zero nonseparator model pixels outside selected windows; the Goods frame-crop misclassification is corrected without alpha repair. Two unsafe extended-arm source attempts and two weak hatch assemblies remain excluded explicitly.44 focused unit/compatibility cases, seven-family/800-pose contract with three refusal cases and both438 native entry boundaries255assertions/five cases pass. Original cargo/mass, Construct/site inputs/work, paid service and movement remain sole native owners. Packing/runtime false. Continue14BE/Plan14 64; original14B aggregates remain open.

### Local Mandatory Reading - Actual return-pod source

_Applies to Group14BE only._ Read Global/UI Area, Plan14 Global/Source generation Area and closest Native passive return-pod Local for64, native18 SpacePod/SpaceDescentJob custody/progress and existing manifest/source/conversion owners. Preserve actual shell/job identity and author genuine packed/release/descent/recovery views; no art timer or packet owner.

### Group 14BE - Deliver the return-pod source

{{HARD}}
- [X] 14BE.1 Complete Plan14 64.1-64.3 with native18 silhouette/directions/states, reviewed originals/contacts and immutable provenance; packing/runtime remain pending.

**Passive-pod source complete,2026-10-17.** `logs/space-pod-source-verification.json` SHA256`17c1bb74dc56f414b26066189d5a77c4ee6d83ee6d76e58f14a26a5fbdc4a49e` binds eight own actual originals,64 logical poses/24 actual components/24 explicit static aliases and reviewed native/4x contacts. Eight genuine shell views include corrected direct N/S hatch occlusion. Two original tab plates register independently at each own original hinge; all32 phase contacts touch,48,270 outside-volume pixels and every fixed joint stay exact, and all original opacity is conserved. Actual phases differ10-24pixels per heading. Packed/release and blocked explicitly share folded original; legacy `recovered` holds actual open-tab phase1 only for a final recovery-ready live job. Successful native atomic transfer still expends/deletes that exact shell; no persistent recovered asset, extra packets or art clock. One hazy original and known equipment/control views remain excluded.42 focused unit/compatibility cases, seven-family/800-pose contract/three refusals and299 native logistics/custody assertions/ten cases in each438 build pass. Public source complete; packing/runtime false. Passive-pool rendering/placement remains14BG, ordinary-game art14BH. Continue14BF/Plan14 67-69.

### Local Mandatory Reading - Metallic asteroid source

_Applies to Group14BF only._ Read Global/UI Area, Plan14 Global/Source generation Area and closest Metallic asteroid Local for67, current [metallic source notes](../../../zz-rhr/upgrades/2026-10/r2mlsp-z-z-t-metallic-asteroid-art-notes.md), actual native class/size/motion/lease and company-disclosure owners, and only the metallic public source row. The shared saved size order is Small/Medium/Large with visual diameters0.5/1/2tiles. Freeze declared variant/canvas/anchor and actual static/animated aliases before genuine views. Native motion reflection does not rotate attachment offsets or grant a saved visual spin. Depleted appearance requires the original company's quantity disclosure; unknown observers retain normal body appearance. Preserve natural body identity and exact outside-part source pixels. No resource reroll, extra lease/hardware, body replacement or art clock.

Context has compacted 1 times while executing this group.

### Group 14BF - Deliver the metallic asteroid source

{{HARD}}
**Scoped split,2026-10-17.** The original three-family group is split before authoring so each physical model has bounded mandatory reading and acceptance. Original14BF.2/.3 obligations continue unchanged as14BFB/14BFC below; complete all three sequentially before14BG.
- [X] 14BF.1 Complete Plan14 67.1-67.3 for metallic/Nickel with reviewed original views/sizes/states and provenance.

**Completed metallic source,2026-10-17.** Plan14 67.1-67.3 and `logs/space-metallic-source-verification.json` bind45 focused cases, both native entry boundaries,8 originals and384 reviewed logical poses at all3sizes. Native source canvas96x80 and original static/two-phase aliases retain finite body/disclosure ownership; packed/runtime gates stay open. Continue14BFB.

### Local Mandatory Reading - Carbonaceous asteroid source

_Applies to Group14BFB only._ Read Global/UI Area, Plan14 Global/Source generation Area and closest Carbonaceous Local for68, completed metallic public size/conversion contract and the executing carbonaceous public row/native class definition. Reuse only validated helpers/size geometry; author a distinct carbonaceous/Water-Ice body and own state parts. Preserve original finite body/lease/company knowledge owners; do not reload the full metallic authoring diary.

### Group 14BFB - Deliver the carbonaceous asteroid source

{{HARD}}
- [X] 14BFB.1 Complete the original14BF.2 obligation, Plan14 68.1-68.3, for carbonaceous/Water Ice with distinct reviewed original views/sizes/states and provenance.

**Completed Water-Ice source,2026-10-17.** Plan14 68.1-68.3 and `logs/space-carbonaceous-source-verification.json` bind33 focused cases,8 own originals and384 reviewed logical poses across all3sizes. Exact outside-fissure pixels/opacity and native finite company-disclosure owners are preserved; packed/runtime stay open. Continue14BFC.

### Local Mandatory Reading - Rich asteroid source

_Applies to Group14BFC only._ Read Global/UI Area, Plan14 Global/Source generation Area and closest Rich Local for69, completed shared size/conversion contract and the executing rich public row/native class definition. Author a distinct Rare-Earth body and own state parts; preserve native finite reserve/lease/disclosure boundaries. Read only relevant completed helper contracts, not preceding whole-family diaries.

Context has compacted 1 times while executing this group.

### Group 14BFC - Deliver the rich asteroid source

{{HARD}}
- [X] 14BFC.1 Complete the original14BF.3 obligation, Plan14 69.1-69.3, for rich/Rare Earth with distinct reviewed original views/sizes/states and provenance; retain packing/runtime false until14BG.

**Rich source accepted,2026-10-17.** Plan14 69.1-69.3 and `logs/space-rich-source-verification.json` bind31 focused cases,9 own selected originals and384 reviewed logical poses across all3sizes. Actual violet-specific depletion and distinct gold contact phases pass; original outside-fissure stone/opacity and company-gated native resource ownership remain unchanged. Packed/runtime stay open. Continue14BG.

### Local Mandatory Reading - Final Space packing and native selection

_Applies to Group14BG only._ Read Global/UI Area, completed14BB-14BFC source receipts, Plan14 Remaining conversion and packing Local, existing quantize/GRF/fallback/runtime range owners and actual Space draw/selection boundaries. Append stable native ranges without moving old sprites, compile actual state/role/size selectors and preserve original gameplay fields/RNG/quantities. Require decoded packed loading and checked-in generator-free payload proof before closing original14B.1.

Context has compacted 2 times while executing this group.

### Group 14BG - Pack and bind final Space sprites

{{HARD}}
- [X] 14BG.1 Convert and validate all required source packing inputs through existing palette/anchor/provenance owners, append protected native ranges and prepare exact actual-loader expectations.
- [X] 14BG.2 Bind actual native read-only state/role/size selectors; consolidate both configuration builds, two reproducible actual GRF builds, generator-free packaging and decoded loading proofs with focused selection/owner-isolation checks; then complete original14B.1.

**Packing/native selection accepted,2026-10-17.** [Current verification index](../../../../logs/space-art-packed-verification.json) binds both ordinary application/focused target builds and95,743 assertions per configuration:6,736 across nine selectors,28,448 for1,579 final/preview sprites and60,559 for3,561 original M0/M1 sprites. Eight source-packing cases pass after binding. Two actual GRF builds and generator-disabled cached packaging use identical SHA256`021baffd5857e892cc7d91af939bfe36b50a96bf2cc901ab282458ab09e0e51f`; final native3599..5166 and all prior IDs/pixels remain fixed. Seven final public families are bound and43 protected families retain their original source/offset ownership with the updated whole-container hash. Read-only native selectors preserve original work/cargo/custody/RNG; idle craft use authored idle poses.14BG and original14B.1 are complete. Runtime activation, original14B.2, aggregate14B and source-plan closure remain pending ordinary-game14BH. Earlier checkpoint paragraphs below are historical.

**Shared-build alignment,2026-10-17.** Original14BG.1 actual decoded/fallback proof obligation moves intact to14BG.2 beside the required compiled selectors, avoiding a full native rebuild solely for the loader followed by another rebuild for selectors.14BG.1 now certifies packing INPUTS only:8 focused cases/7.2s,1568 exact crop/anchor round trips, native3599..5166, protected seven old source/header hashes unchanged. Exact fixture expects1568 final +11 unchanged previews. No actual GRF generation, decoding, fallback or runtime acceptance claimed yet. Master14BG/original14B remain unchecked.

**Checkpoint disposition,2026-10-17.** Read-only native selectors, eight focused C++ cases and the separate bounded actual-build/load runner are implemented. The first consolidated RelWithDebInfo build generated the appended GRF, then exposed a missing direct `direction_type.h` include in `space_sprite.h`. The include is corrected. The user explicitly instructed stopping compilation and deferring the rebuild; the active build tree was stopped and no replacement build or native tests were run.14BG.2, original14B.1 and master14BG remain unchecked: both configuration builds, selector checks, two byte-identical actual GRF builds, generator-free packaging and both actual decoded prefix/final loading proofs remain required. See [bounded packing handover](../../../zz-rhr/upgrades/2026-10/r2mlsp-z-z-w-space-packing-notes.md). This checkpoint creates no native or runtime acceptance claim.

**Focused build scope on resumption,2026-10-17.** `openttd_space_art_test` links the actual unchanged `openttd_lib` engine objects, the same standard Catch/diagnostic/runtime helpers, all nine current selector cases and both existing actual decoder cases. It is explicitly targeted and excluded from the default build. This avoids recompiling hundreds of unrelated test cases after the shared sprite-range header changes. Build the ordinary `openttd` application and this focused target in both configurations; exact source/owner/decoder obligations remain unchanged. The full `openttd_test` target remains available for its own later scenarios and regression gates. Native acceptance remains pending until the focused modes actually pass.

### Local Mandatory Reading - Ordinary-game final Space art review

_Applies to Group14BH only._ Read Global/UI Area, original14B requirements and delivered14BG packed/selector evidence, actual paid/scan/construction/descent source checkpoints and existing native capture provenance. Review required states in ordinary native game with no fixture work/cargo/pose grants. Preserve supplied originals and distinguish source/packed/runtime acceptance; source-plan/master closure requires all Completion Conditions.

### Group 14BH - Review final native Space art

{{HARD}}
- [X] 14BH.1 Capture and review actual selection/scan/construction/attachment/mining/return-pod states using final packed sprites in both configurations, retaining original native/pixel/owner evidence.
- [X] 14BH.2 Complete original14B.2 and reconcile Plan10/master/parent only after all required art/runtime and Completion Conditions pass; then resume the master's next source group.

**Final Space art/source completion accepted,2026-10-17.** [Final verification](../../../../logs/space-art-final-verification.json) binds all seven source families, stable1568 final sprites, both current application/focused builds and95,743 native assertions each. Thirty-two paused ordinary field/inspector captures preserve30 physical plus2 metadata families and match all16 pixel pairs between configurations. Two further ordinary live construction captures earn actual native progress, then pause/save/screenshot through view-only console input; no work/cargo/pose is assigned. Exact final source pixels qualify scanning, constructing, attached/mining, full hold, all three asteroid classes and passive descent. Native contacts, original relative attachment geometry and current quantities/construction/descent widgets are reviewed; all supplied originals remain unchanged. Late-load presentation refresh fixes stale idle hulls after native owners restore. Eight packing cases and the scoped7-family/1568-pose public contract pass after activation. The final index maps every Completion Condition to its dated accepted runtime producer; no full regression or broader Plan11/personal review is claimed.14BH, original14B and Plan10 are complete; continue Plan11 Group13 via the Space master.

## Consult on Demand

| Trigger | File name | Start at | Expand only when | Consultation purpose |
| --- | --- | --- | --- | --- |
| A probe follows a stale target tile after asteroid movement | [../../architecture/vehicles/archi-vehicles.md](../../architecture/vehicles/archi-vehicles.md) | `Code map` | The stable body ID and fixed-point motion ledger agree but order/controller routing differs | Locate the live order/vehicle controller responsible for typed moving targets; this broad index is a triggered navigation aid, not a new mandatory preload. |
| A required public export is absent and its owner/location is disputed | [r2mlvs-multilayer-vehicles-and-structures.md](r2mlvs-multilayer-vehicles-and-structures.md) | `Requirements and ownership` and `Plan Index` | The named producer confirms an intentional contract/ownership change | Find the responsible producer and reconcile its public handoff with direct consumers; the hub cannot substitute for missing implementation. |
| Current behavior contradicts a historical subsystem description | [../../architecture/map-layers/archi-map-layers.md](../../architecture/map-layers/archi-map-layers.md) | `Invariants` and the relevant named source in `Code map` | The live symbol and executable test do not explain the difference | Locate layer addressing or lifecycle rationale. Live code/tests govern delivered behavior; this architecture page does not prove future interfaces exist. |

## Planning Provenance

This is not an execution reading queue. Do not preload it after context compaction. It is non-transitive; ranges record planning use, not runtime authority.

| File name | Sections or Line Ranges used while planning | Planning contribution |
| --- | --- | --- |
| [docs/zz-reach2/architecture/initial-inventory.md](../../architecture/initial-inventory.md) | 185-220 | `Command as the only world mutation`, `Pool-indexed game objects`, `Tile map as a packed array` and `File-suffix modules` oriented command/pool/header ownership and source registration. This historical inventory is a location survey, not authority for current layer behavior. |
| [docs/zz-reach2/architecture/@archi-ttd-level0.md](../../architecture/@archi-ttd-level0.md) | 24-63 | `How the program fits together` and `Source and runtime boundaries` located shared simulation, data/presentation, CMake targets and subsystem handoffs used to choose the live reading sources. |
| [docs/zz-reach2/upgrades/2026-10/r2mlvs-multilayer-vehicles-and-structures.md](r2mlvs-multilayer-vehicles-and-structures.md) | 119-140 | `Requirements and ownership` assigned this plan's feature responsibilities; it shaped the decomposition without making every sibling plan a mandatory reading dependency. |
| [src/vehicle_func.h](../../../../src/vehicle_func.h) | 192-204 | The inspected live boundary helped separate characterization, public-contract production, implementation and acceptance batches. It materially constrained fixed-point moving targets, finite reserves and conserved probe cargo; this entry records planning influence rather than an additional reading obligation. |
| [User planning-format instructions](<C:/Users/Axonn/.codex/attachments/cd59b20a-3a96-4d54-b4bd-1e8f37639e7d/Pasted text.txt>) | `Global Mandatory Reading`, `Area Mandatory Reading`, `Consult on Demand`, `Planning Provenance`, `Final validation` | Supplies the exact protocol, chronological difficulty batches of at most eight tasks, reading gates and validation format. |
| [src/source_type.h](../../../../src/source_type.h) | 19-56 | Deeper planning pass, 2026-10-04: Existing source identity limits require canonical asteroid provenance and a fixed extraction-position snapshot. |

## Completion Conditions

Population, resource content, trajectory and speed match the editable tables in their owning groups and have deterministic fixtures, including supported map-size/class coverage, aggregate initial resource budgets and basic-probe interception bounds. Generated state and subsequent saved motion survive reload without rerolling or replenishment.

Asteroid movement is observable, slow and deterministic. Probes physically attach and mine moving bodies, then return into the correct center's 20-tile drop zone and deliver accountable Surface cargo through its collection hub. The first useful mission completes without an orbital station, and all interrupted states can recover.

The additional8B/8C requirements are part of completion: three Earth-launchable
probe roles with saved typed orders, completed-scan-only company knowledge,
Steel/Goods-delivered station construction, mined-ore processing into both
materials, exclusive orbital reuse or Earth export, and later material-paid
local probe assembly. Current P1/P2 snapshots remain immutable regression inputs;
they do not bypass the mature commissioning or disclosure rules.
