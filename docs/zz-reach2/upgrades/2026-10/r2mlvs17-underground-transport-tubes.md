# R2MLVS17 - Underground Transport Tubes

Storage/recovery: [historical debugging architecture](../../architecture/archi-historical-debug.md) and [R2FC2](r2fc2-file-cleanup3.md); these do not resume paused Tube feature work.

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
| [docs/zz-reach2/upgrades/2026-10/r2mlvs-multilayer-vehicles-and-structures.md](r2mlvs-multilayer-vehicles-and-structures.md) | 302-308 | `AD-15` supplies the canonical gameplay/ownership decision for every group of this plan. It constrains five original layer-local capsules, excavated guideways and explicit paid power; a consumer must not redefine it locally. |
| [src/map_layer.h](../../../../src/map_layer.h) | 78-85 | `MultiLayerMap` ownership preamble: `Map` owns packed storage/geometry and this controller owns layer semantics. Every group uses world tiles/explicit layer helpers; camera selection and ad hoc plane offsets cannot define gameplay. |
| [src/rail_type.h](../../../../src/rail_type.h) | 16-39 | Rail labels, 64 rail slots and `RailTypes` define dedicated tube compatibility; a sprite or purchase filter is insufficient to segregate networks. |

Status: registry/prototype/construction/separation/junction/turning Groups1-4, paid service5, native metering6.1-6.3 and recovery7 are delivered and tested. Read-only public readiness8 is delivered; geothermal6.4 returns after master10.6 and native terminal/interchange Group9 is delivered; dated fleet/control Groups10-12 are current under master10.5. Final art and acceptance remain pending. Parent: [R2MLVS master](r2mlvs-multilayer-vehicles-and-structures.md). Prototype metadata is not the full operational handoff.

**Artifact-layout migration (2026-10-09):** this plan consumes the parent [Development artifact layout](r2mlvs-multilayer-vehicles-and-structures.md#development-artifact-layout), implemented by [R2FC](r2fc-file-cleanup.md). It supersedes earlier docs asset/review/helper placement and fixed runtime-config destinations below. New native homes use `mlm/runtime/`, generated feature reviews use `mlm/reviews/`, logs remain under `logs/`, and reusable code/curated inputs keep their code/test owners. Paths and focused native/cold verification are complete; R2FC records the results and retained-input identities. Repository-relative executable commands run from the checkout root through their context-aware wrappers. Historical pass records remain dated evidence and are not new acceptance claims. No gameplay task is completed by this migration and the separate post-demo fixture-pruning gate remains.

## Outcome and Boundaries

Build a distinct underground freight system inside TBM-cleared corridors and chambers. Dedicated powered tubes carry at least five original capsule models between underground terminals; ordinary trains and road vehicles do not use them.

## First-party vehicle catalogue

These five designs are required deliverables, not alternative names for refits. Dates and relative balance are proposed; each gets its own engine/model identity, stats, purchase sprite and eight-direction/load art. Full capacity values and cargo-unit conversion belong to r2mlvs1's tuning tables.

| Model / proposed introduction | Cargo role | Performance identity | Original visual identity |
| --- | --- | --- | --- |
| Mole-70 Bulk Capsule / 1970 | Coal, Iron/Copper/Bauxite/Salt and compatible deep ores; Rock/Earth outside the TBM's mandatory wagon leg | Cheap, large bulk hold, modest speed and high loaded energy use | Low segmented amber capsule with enclosed hopper ribs and visible load indicators |
| Crate-75 Container Capsule / 1975 | Goods, Batteries, Electronics and Solar Mirror Kits | Faster, medium hold; frequent manufacturing deliveries | Square-section green cargo pods with clamped containers and numbered doors |
| Cistern-80 Fluid Capsule / 1980 | Liquid Chemicals/Propellant and other declared compatible liquids | Medium speed, sealed liquid hold; specialized terminal service | Cyan twin-tank capsule with pressure bands, valves and level display |
| Shield-85 Mineral Capsule / 1985 | Uranium and designated sealed mineral loads | Small hold, expensive shielding/service; reliable valuable loads | Short octagonal capsule with thick pale shielding panels and mineral-specific load markings |
| Forge-05 Heavy Capsule / 2005 | Steel, Refined Metals, Station Components and Robotics Parts | Very high payload, low acceleration, long terminal dwell and high power demand | Wide dark machinery capsule with structural braces and a distinct heavy coupling frame |

The content profile defines one physical form per cargo label: Chemicals are liquid for Cistern-80 in this draft; Crate-75 does not refit to the same label as packaged Chemicals. A future packaged form would require an explicit conversion/label decision. Existing trains/trucks carry early Uranium until the Shield-85 arrives. Tube introduction before 1980 does not grant access to Deep Underground.

## Public Handoff Contract

Partial public owner: [`src/transport_tube.h`](../../../../src/transport_tube.h) publishes tested registry/prototype queries and `TubeVehicleCapability`, `CanTubeTrainEnterTile`, `CheckTubeConstruction`, `CheckTubeConstructionArea`, `CheckTubeVehicleAdmission` and `CheckTubeRailConversion`. Group5 adds `ConnectTubeExternalPower`, `PublishTubeExternalPower`, `ConsumeTubeExternalPower`, `CloseTubeExternalPower` and saved physical/invoice queries, consuming the existing utility authority. Native metering adds `ComputeTubePowerDemand`, `BindTubePowerVehicle`, `ReconnectTubePowerVehicle`, separate power waits and rebuildable owned connectivity; actual387 bindings preserve admitted demand fractions and once-tick history. Group8 publishes `TubeTerminal`, `GetTubeTerminal`, `CanTubeTerminalServeVehicle` and `GetTubeNetworkReadiness` over actual native station/depot identities, cargo/date admission and stable physically reachable paid inlets. Queries allocate no grants, move no packets and create no invoices. Only model9 is prototyped; full operational terminals/fleet remain later obligations. Saved386/387 inlets restore hidden canonical guideway/prototype definitions; new-world controls and full operational fleet/terminal activation remain pending.

Physical source geometry is separately published by [`transport_tube_model.h`](../../../../src/transport_tube_model.h): `TubeVehicleGeometry` borrows the canonical capability and exposes native rail length/part count through `GetTubeVehicleGeometry`. Staged10B verifies all five descriptors and actual model9 construction. Descriptor availability does not establish an installed engine, purchase/refit/date admission, model balance or operating fleet acceptance; those remain Groups10-12.

## Implementation Tasks

Deepening pass, 2026-10-04: implementation groups use only SIMPLE, MEDIUM or HARD difficulty, with at most eight pending tasks each. Sketches describe proposed implementation contracts; verify the named earlier producers at each assumption gate. No gameplay or asset implementation is marked complete by this planning pass.

Groups are chronological and contain consecutive tasks of one difficulty. Each difficulty batch has at most eight tasks; area reading changes only when the implementation boundary or source of truth changes.

### Area Mandatory Reading - Tube infrastructure and capability separation

_Applies to Groups 1-4, including1B._

**Entry gate.** r2mlvs1 profile/cargo/date capabilities, r2mlvs2 excavation and r2mlvs3/4/5 safe portal/work-track contracts must be delivered for dependent construction. Dynamic utility implementation is needed only in the following area.
**ASSUMPTION: this was already built previously. If not, re-asses at runtime**. Verify the named prior public exports and their type/behavior fixtures with the current source before dependent work. A declaration alone is insufficient for runtime mutation. If missing, stop the dependent implementation and passing claims in Groups 1-4; independent characterization, test design and source verification may continue. Do not copy private stores, invent substitute IDs/inventory/energy or treat historical planning prose as delivered code.

| File name | Line Ranges | Description |
| --- | --- | --- |
| [src/rail.h](../../../../src/rail.h) | 178-182 | `powered_railtypes` and `compatible_railtypes` have distinct invariants; ordinary/TBM/tube locomotion needs explicit power and compatibility checks. |
| [src/rail.h](../../../../src/rail.h) | 239-257 | Railtype introduction/requirement fields constrain tube availability; the profile calendar unlock remains a shared additional gate. |
| [src/timer/timer_game_calendar.h](../../../../src/timer/timer_game_calendar.h) | 15-39 | `TimerGameCalendar` technology-clock contract and date conversions govern 1980/model/factory introductions; economic/realtime scheduling cannot unlock a technology. |
| [src/train.h](../../../../src/train.h) | 45-72 | `ConsistChangeFlag`, `FreeTrainTrackReservation`, `TryPathReserve`, `NormalizeTrainVehInDepot` define cache/reservation rebuilding after actual wagon changes. |
| `src/multilayer_content.h` (planned public contract; link after implementation) | Live definitions; no fixed range | [r2mlvs1-gameplay-contracts-and-content.md](r2mlvs1-gameplay-contracts-and-content.md) Groups 1-6, ending with Group 6's public-contract task, publish `MultiLayerGameplayProfile`, `LayerVehicleCapability`, `GetMultiLayerContentProfile`, `CheckDeepLayerAvailability`, `UtilityPowerAccount`. These exports/schema fields are authoritative for this area. Same-plan consumers follow those earlier groups; cross-plan consumers verify the producer's required implementation/tests as well. Proposed names alone do not prove delivery. |
| `src/excavation.h` (planned public contract; link after implementation) | Live definitions; no fixed range | [r2mlvs2-geology-and-excavation.md](r2mlvs2-geology-and-excavation.md) Groups 1-7, ending with Group 7's public-contract task, publish `ExcavationState`, `GeologyDeposit`, `ExcavationJob`, `CheckExcavatedFootprint`, `CommitExcavationCut`, `GeothermalVentField`. These exports/schema fields are authoritative for this area. Same-plan consumers follow those earlier groups; cross-plan consumers verify the producer's required implementation/tests as well. Proposed names alone do not prove delivery. |
| [src/layer_portal_cmd.h](../../../../src/layer_portal_cmd.h) | Live definitions; no fixed range | [r2mlvs3-sealed-portals-and-deep-rail.md](r2mlvs3-sealed-portals-and-deep-rail.md) Groups 1-7, ending with Group 7's public-contract task, publish `LayerPortalWorkState`, `GetLayerPortalWorkState`, `CheckLayerPortalWorkAccess`, `CommissionLayerPortalTBM`. These exports/schema fields are authoritative for this area. Same-plan consumers follow those earlier groups; cross-plan consumers verify the producer's required implementation/tests as well. Proposed names alone do not prove delivery. |
| `src/tbm.h` (planned public contract; link after implementation) | Live definitions; no fixed range | [r2mlvs4-tunnel-boring-machines.md](r2mlvs4-tunnel-boring-machines.md) Groups 1-5, ending with Group 5's public-contract task, publish `TBMProject`, `TBMWorkState`, `ValidateTBMAreaRoute`, `AdvanceTBMWork`. These exports/schema fields are authoritative for this area. Same-plan consumers follow those earlier groups; cross-plan consumers verify the producer's required implementation/tests as well. Proposed names alone do not prove delivery. |
| `src/tbm_exchange.h` (planned public contract; link after implementation) | Live definitions; no fixed range | [r2mlvs5-spoil-wagons-and-haulage.md](r2mlvs5-spoil-wagons-and-haulage.md) Groups 1-5, ending with Group 5's public-contract task, publish `TBMExchangeAssignment`, `ValidateTBMWagonExchange`, `ExecuteTBMWagonExchange`. These exports/schema fields are authoritative for this area. Same-plan consumers follow those earlier groups; cross-plan consumers verify the producer's required implementation/tests as well. Proposed names alone do not prove delivery. |
| [src/rail.h](../../../../src/rail.h) | 346-394 | Complete `IsCompatibleRail` and `HasPowerOnRail` overloads describe type compatibility, not runtime electrical grants. |
| [src/pathfinder/follow_track.hpp](../../../../src/pathfinder/follow_track.hpp) | 314-435 | Complete `CanEnterNewTile` controls route entry; enforce semantic tube/layer restrictions alongside actual movement checks. |
| [src/rail_cmd.cpp](../../../../src/rail_cmd.cpp) | 1789-1838 | Complete `TileType::LayerPortal` branch in `CmdConvertRail` changes both ends; tube conversion must refuse before this paired mutation. |

Prove semantic tube/ordinary separation independently of broad railtype masks. Validate route discovery and actual entry, including paired rail conversion. A tube never performs excavation or replaces mandatory spoil hauling.

### Group 1 - Prove a separate tube capability over rail foundations

{{HARD}}
- [X] 1.1 Allocate the profile-selected tube rail label through the canonical railtype registry; do not hard-code a runtime slot or silently collide with external content.

1.1 registry closure (2026-10-08): exact admitted native package identity allocates `UTUB` through `AllocateRailType`, with canonical-row semantic authority and no cached/hard-coded slot. Missing content, foreign main/alternate claims and a full64-slot registry refuse unchanged; registry reset removes the derived native authority. Power/compatibility masks are explicit self-only metadata, separate from energy. Existing base guideway art is reused; the type stays hidden and loader/construction/engine activation awaits1B/2. [Both executables build](../../../../logs/r2mlvs3mp-z-tube-registry-build-2.log); [registry/native-engine/upstream-support checks](../../../../logs/r2mlvs3mp-z-tube-registry-checks-2.log) pass6,052 assertions/11 cases. A stale support assertion was corrected to the already implemented recurring-project rule: reuse exact owned track on excavated floor, while refusing foreign ownership, railtype or geometry. No support runtime behavior changed. Entry producers independently pass [380 assertions/12 cases](../../../../logs/r2mlvs3mp-z-tube-prerequisite-checks.log).1B is current; master10.1 remains unchecked until all dispatched foundations pass.

### Local Mandatory Reading - Native capsule prototype and semantic separation

_Applies to Group1B after the verified canonical registry producer1.1._

Read Global/Tube infrastructure Area and only `InitialiseTubeRailType`/`IsTubeRailType`, actual shared catalogue model9, canonical EIDS allocation in `InitialiseUndergroundEngineDefinitions`, rail compatibility/powered helpers, `CFollowTrackT::CanEnterNewTile`, actual train build/entry checks and existing reservation APIs. Verify1.1's focused registry fixture before allocating a native prototype. Keep the railtype hidden and ordinary runtime activation pending until construction/movement guards exist. Compatibility masks never supply energy; broad masks cannot erase the semantic capsule/ordinary domain boundary.

### Group 1B - Prototype capsules and enforce semantic separation

Resumed2026-10-08 at the user's request: the earlier GP-TG finalization skip is historical. This group is active again after delivered1.1; prototype validation keeps construction, paid electricity and full fleet activation with their subsequent owners.

{{HARD}}
- [X] 1.2 Prototype one self-propelled capsule, one tube tile and a junction using normal reservation/path foundations. Keep compatibility/powered rail masks explicit and separate from dynamic electrical supply.
- [X] 1.3 Demonstrate ordinary trains/TBMs refuse the tube and a capsule refuses conventional/temporary rail even if a NewGRF compatibility mask is broad. Publish the semantic capability predicate used by both paths.
- [X] 1.4 Publish the prototype `TubeVehicleCapability` declaration in planned `src/transport_tube.h` and freeze its engine/rail metadata/pathfinding fixture before power integration. Group 8 later completes the public handoff for all consumers.

1B closure,2026-10-08: model9 reuses native EIDS local0x8009, canonical catalogue names/dates/layers/exact bulk refits and existing base train art. Its bounded prototype physics are Singlehead/Maglev, speed100, power1000, weight20t, capacity60 native units, running factor100/electric class and purchase factor120; these are provisional engine parameters, never electrical grants or final five-model balance. The producer is explicitly called by the test fixture; normal loader/purchase-menu activation remains pending. Actual command-created depot/capsule, native junction follower and conflicting track reservation/release pass. Initial positions/open floor are declared fixture setup; no physical powered journey is claimed.

`CanTubeTrainEnterTile` checks every actual part independently of broad masks, model/profile/layer/Deep dates and portal exclusion. Native follower, actual train movement and tile-entry dispatcher consume the same predicate; acquired cutter/support/spoil and ordinary locomotive entry into tube track refuse. Definition preflight rejects missing cargo, foreign persisted mapping and retired registry without allocating or overwriting owners. [Debug prototype/registry/refusals](../../../../logs/r2mlvs3mp-z-c-tube-prototype-checks-3.log) and [optimized checks](../../../../logs/r2mlvs3mp-z-c-tube-prototype-optimized-checks.log) pass432 assertions/eight cases. [Affected physical movement/fleet/support boundaries](../../../../logs/r2mlvs3mp-z-c-tube-movement-boundaries.log) pass115,844/23 cases. Both game/test configurations compile; earlier header/direction/include-order/fixture-precondition failures are retained in diagnostic logs and are not passing evidence. Construction/purchase/rearrangement/paired conversion and runtime electricity remain with the following groups; master10.1 stays unchecked.

### Local Mandatory Reading - Tube construction and conversion ownership

_Applies to Group2; supplements Global/Tube infrastructure Area._

Read the delivered prototype capability/admission and only current `CmdBuildSingleRail`, `CmdBuildTrainDepot`, `CmdConvertRail`, `CheckExcavatedFootprint`, `CheckExcavationConstructionEntrance`, `CheckTBMSupportChange` and normal occupancy/reservation guards. Construction must validate full world footprints before mutation; Surface/Space/locked Deep and any paired portal conversion refuse. Loader/fleet activation remains staged until purchase/mixing and metered movement owners exist. Existing artwork is reused during capability/construction development.

### Group 2 - Guard tube construction and safe work-track conversion

Resumed2026-10-08 after completed1B: the previous GP-TG skip is historical; this group is now the active construction/purchase/conversion boundary.

{{HARD}}
- [X] 2.1 Freeze label/capability, permitted Underground/Deep layers, terminal/depot footprints and legal rail-realizable junction/curve geometry. Tube motion geometry is separate from TBM cutting turns.
- [X] 2.2 Call `CheckExcavatedFootprint` for all guideway, terminal, depot and utility tiles; apply the profile Deep date gate. Test GUI, script/direct command, copy and conversion on solid/Surface/Space/locked Deep ground.
- [X] 2.3 Before replacing temporary work rail, verify TBM/hauler withdrawal and release of its occupied/reserved retreat path. Revalidate the full conversion footprint before mutation; tubes never excavate rock.
- [X] 2.4 Refuse mixed capsule/conventional consists, ordinary portal/elevator entry and any cross-layer tube route at purchase, rearrangement, route discovery and actual movement boundaries. `CmdConvertRail` can rewrite both portal ends, so reject tube conversion of a portal/tunnel pair before any paired mutation.

Group2 closure,2026-10-08: the canonical `UTUB` prototype remains rock-layer/local only, introduced1970 with the separate Deep1980 gate. Guideways use the existing six plain-rail track bits and exclusive junction conflicts, without level crossings, paired shafts or ordinary tunnel/bridge transport. Each guideway cell is one tile; service depots are one native depot tile plus their actual direction's approach. Terminals reuse the submitted ordinary rail-platform rectangle, bounded by station spread and both axis approaches. Utility producers submit their complete explicit node/link footprint to the same public floor guard. The prototype uses the normal train length/heading owner; TBM's45-degree cutting limit is not a capsule motion rule.

Canonical excavation helpers check every foundation/approach, with no new terrain/camera authority. Ordinary rail/depot/station commands and shared GUI/direct/copied command arguments retain that guard. A complete native tube conversion is preflighted before its first write; later reserved/occupied cells, paired portal/tunnel/bridge/crossing and any unreleased work-support record refuse unchanged. A finished project alone does not erase its temporary traffic metadata: the existing paid support-conversion/removal owner must release it first, including under bankruptcy flags. Native purchase/refit and physical unit/chain rearrangement enforce exact model/cargo/domain before allocation or relinking; existing source/depot haulage retains its own ordinary domain.

[Debug native/prototype/GUI/refusal checks](../../../../logs/r2mlvs3mp-z-c-tube-construction-final-checks.log) and [optimized checks](../../../../logs/r2mlvs3mp-z-c-tube-construction-optimized-checks.log) pass2,852 assertions/18 cases. Actual shared toolbar callbacks reject solid/Surface/Space/locked Deep and construct only after explicit fixture-open floor/approaches; direct command paths cover copied parameters, real station/utility footprints, whole conversion, actual committed support and real bought/refitted/coupled vehicle identities. [Affected existing movement/portal/fleet/support/command checks](../../../../logs/r2mlvs3mp-z-c-tube-construction-boundaries.log) pass116,212/31 cases. Both game/test builds and whitespace checks pass. Earlier GUI failures were missing font/main-toolbar test setup and remain diagnostic evidence, not production regressions. No raw review raster was generated or deleted. Dedicated tube menus, actual paid power, useful transport, full Squirrel scenarios and final art remain later consumers; no full runtime bootstrap/energy claim is made.

### Local Mandatory Reading - Junction ownership and native recovery

_Applies to Group3; supplements Global/Tube infrastructure Area._

Read the delivered whole-consist tube admission and construction/conversion guards, existing `TryPathReserve`/`FreeTrainTrackReservation`, `TryReserveRailTrack`/`UnreserveRailTrack`, track-layout invalidation and actual train reversal/entry controllers. Use real existing train/PBS/path owners; do not add another lease or cargo registry. Tests may seed initial positions explicitly but must distinguish route/reservation boundaries from actual driven recovery. Electricity and normal-loader activation remain staged to the following power area.

Context has compacted 1 times while executing this group.

### Group 3 - Implement junction leases and blocked-route recovery

Resumed2026-10-08 after completed Group2: the previous GP-TG skip is historical. This group is now active for existing reservation ownership, layout invalidation and blocked-route recovery.

{{HARD}}
- [X] 3.1 Map each legal tube segment/junction to existing rail track bits and reservation ownership; define one capsule movement/length model and exclusive conflict rules.
- [X] 3.2 Implement deterministic reservation requests, release on cleared segments and invalidation after layout changes. A cached path cannot authorize entry onto a converted/non-tube tile.
- [X] 3.3 Define turnaround/service approach and a blocked-network stop state without instantaneous direction changes or disappearing cargo. Test two capsules contending at a junction and a closed branch.
- [X] 3.4 Test removal/conversion while occupied or reserved; refuse unsafe mutation or release only future reservations while retaining the actual capsule footprint.

Group3 closure,2026-10-08: the prototype is one physical native train unit of `VEHICLE_LENGTH` (eight pixels), using the six existing plain-rail bits and their ordinary overlapping-track conflicts. Two command-bought capsules leave different actual depots, reserve the shared Y/Right junction in stable native order, retain their distinct packet IDs and release cleared segments before the next capsule crosses. Normal removal retires only the future path and revalidates the remaining safe prefix; occupied removal/conversion and reserved conversion refuse. A warmed route stops before an actual converted conventional cell despite broadened mechanical masks, then resumes through real ticks after repair. No duplicate lease, movement or cargo registry exists.

Moving capsule reverse commands now queue normal braking under both acceleration settings. Physical backing preserves body facing, packet ID and native position, followed by real halt/service-depot entry. Missing/incompatible branches hold the capsule in its existing PBS wait instead of automatic line-end or timed-signal reversal; repair resumes through native retries, while an explicit manual stop survives repair. [Debug selected tube checks](../../../../logs/r2mlvs3mp-z-c-tube-reservations-final-checks.log) and [optimized checks](../../../../logs/r2mlvs3mp-z-c-tube-reservations-optimized-checks.log) pass13,709 assertions/22 cases; the new four-case physical subset passes10,857. [Affected actual TBM/portal/fleet/support boundaries](../../../../logs/r2mlvs3mp-z-c-tube-reservations-boundaries.log) pass115,844/23. Both game/test configurations build, and whitespace passes. Earlier fixtures requesting a hidden-depot path outside its departure handler, selecting an alternate safe depot, or leaving direct-command signal updates buffered were corrected; their diagnostic logs are retained. These are mechanical/reservation proofs with initial real cargo packets, not paid electricity, normal-loader activation, new screenshots or useful production-route acceptance.

### Local Mandatory Reading - Tube turning contract

_Applies to Group4; supplements Global/Tube infrastructure Area._

Read the delivered native capsule length and junction/recovery fixture, canonical tube rail flags, `Rail90DegTurnDisallowed` and the real follower's track-direction filtering. Freeze tube segment geometry using existing track bits and actual traversed cells. The tube policy must not depend on camera, TBM cutting state or the player's ordinary-rail sharp-turn setting. Existing capsule reversal keeps its physical body facing; final art and all-model activation remain later obligations.

### Group 4 - Infrastructure and capability contract

Resumed2026-10-08 after completed Group3: the earlier GP-TG finalization skip is historical. This group now freezes capsule turning geometry independently of excavation turns.

{{MEDIUM}}
- [X] 4.1 Freeze legal turning geometry independently of the TBM's separate 45-degree cutting constraint.

Group4 closure,2026-10-08: canonical `UTUB` now explicitly sets `Disallow90Deg`. Existing plain-rail segment bits admit straight and traversed45-degree heading steps and reject a direct sharp90-degree step even if the ordinary-rail setting permits it. This policy belongs to guideway movement, without reading TBM project/cut state. [Focused geometry](../../../../logs/r2mlvs3mp-z-c-tube-geometry-checks.log) passes1,045 assertions/two cases: the actual follower retains straight/45-degree choices and refuses the remaining sharp choice, while a command-bought capsule physically traverses SW/S/SE through distinct approach/junction/output cells. Backing keeps the body's facing through the existing train owner. [Complete selected foundations](../../../../logs/r2mlvs3mp-z-c-tube-foundations-final-checks.log) and [optimized foundations](../../../../logs/r2mlvs3mp-z-c-tube-foundations-optimized-checks.log) pass14,754 assertions/24 cases; both game/test builds and whitespace pass. This closes master10.1 only. Paid service, normal-loader/full fleet activation, art and actual useful production routes remain in later groups.

### Area Mandatory Reading - Paid tube power and movement recovery

_Applies to Groups 5-8._

**Entry gate.** Earlier Groups 1-3 supply the prototype `TubeVehicleCapability`, construction guard and reservation behavior; r2mlvs12 Useful Power Handoff supplies tested ledger/query and consume/release/transfer mutation operations.
**ASSUMPTION: this was already built previously. If not, re-asses at runtime**. Verify the named prior public exports and their type/behavior fixtures with the current source before dependent work. A declaration alone is insufficient for runtime mutation. If missing, stop the dependent implementation and passing claims in Groups 5-8; independent characterization, test design and source verification may continue. Do not copy private stores, invent substitute IDs/inventory/energy or treat historical planning prose as delivered code.

| File name | Line Ranges | Description |
| --- | --- | --- |
| `src/transport_tube.h` (planned public contract; link after implementation) | Live definitions; no fixed range | Earlier Group 1 publishes `TubeVehicleCapability`; Groups 2-3 supply its construction/reservation behavior. Verify those fixtures before power integration; Group 8 completes remaining public exports afterward. |
| `src/utility_power.h` (planned public contract; link after implementation) | Live definitions; no fixed range | [r2mlvs12](r2mlvs12-solar-farms-mirrors-and-eti.md) `Useful Power Handoff`, produced by its native-ledger, atomic-mutation and consumer-effect groups, supplies `PublishUtilityGeneration`, `AllocateUtilityPower`, `GetUtilityAllocation`, `UtilityGrantID`, `UtilityMutationKind`, `PrepareUtilityMutation`, `CommitUtilityMutation`. Verify its allocation, consume/release/layer-transfer, effect and save/retry fixtures before dependent power mutations; no private account writes or duplicate grid. |
| [src/ground_vehicle.cpp](../../../../src/ground_vehicle.cpp) | 17-72 | Complete `PowerChanged` updates caches and may set a stopped flag; dynamic grid waits require explicit manual-stop-preserving recovery. |
| [src/train_cmd.cpp](../../../../src/train_cmd.cpp) | 455-463 | Complete `Train::UpdateAcceleration` retains minimum acceleration; zero power alone cannot enforce an allocation-based movement refusal. |
| `src/geothermal.h` (planned public contract; link after implementation) | Live definitions; no fixed range | [r2mlvs18](r2mlvs18-geothermal-energy-and-industry.md) `Publish heat allocation and electrical export APIs` supplies `GeothermalHeatAllocation`, `GeothermalUtilityLink`, `AllocateVentHeat`, `ExportGeothermalPower`; its following generator groups supply operational generation. Required only for the later geothermal source-switch integration. |
| [src/train_cmd.cpp](../../../../src/train_cmd.cpp) | 4229-4256 | Complete `Train::Tick` calls two locomotive substeps; tube demand/settlement needs one authoritative simulation-epoch hook. |

**Power sketch (proposed).** Compatibility, reserved track and an actual grant are separate requirements. Never charge from cache-refresh callbacks.

```text
one tube tick -> compute demand -> read/reserve shared grant
             -> guarded movement/service -> commit actual consumption once
grid loss -> separate wait reason -> safe hold with cargo retained
grid returns -> resume only a power-waiting capsule, preserve manual stop
```

**Scoped geothermal gate. ASSUMPTION: this was already built previously. If not, re-asses at runtime**. Only the later source-switch subtask in Group 6 requires r2mlvs18 generation/export. Verify its producer fixture or leave that integration pending; paid-grid work continues without a fake thermal source.

### Group 5 - Connect paid external power without a geothermal prerequisite

Resumed2026-10-08 after completed Groups1-4: the earlier GP-TG finalization skip is historical. This group now consumes the actual shared electrical authority and owns bounded paid source procurement; compatibility metadata and a mechanical driving fixture cannot stand in for that service.

{{HARD}}
- [X] 5.1 Consume r2mlvs12 `Useful Power Handoff` through `src/utility_power.h`: `PublishUtilityGeneration`, `AllocateUtilityPower`, `GetUtilityAllocation`. Verify generation/grant/effect/save fixtures before adding propulsion accounting.
- [X] 5.2 Build an explicit power connection at an accessible commissioned portal-side interchange. Validate owner, excavated footprint, layer/date and a bounded paid external service contract; coordinate proximity is not a power link.
- [X] 5.3 Publish external supply once per interval and settle tariff from actual granted/used service through the common utility authority. Refuse insufficient funds/capacity without creating free energy or duplicated charges.
- [X] 5.4 Test a 1970 paid-service source/grant fixture using the earlier capsule capability prototype; no Deep/1980/geothermal output is required. The real useful route is accepted only in the final area after movement, terminals and production models exist.

Group5 closure,2026-10-08: canonical100-kW/20-per-economy-day/1-per1,000-used-J service now validates an explicit contiguous excavated corridor from an owned Connected/date-valid rail portal to an owned tube depot/station. Estimate/refusal paths publish no IDs, energy or invoices; the native ledger integrates supply and owns every grant/used joule. Whole physical depot service and usage payment settle together, retaining a0..999 numerator and charging its final fraction once on closure. [Focused source/financial/owner checks](../../../../logs/r2mlvs3mp-z-d-tube-power-checks-2.log) pass1,268 assertions/12 cases; [earlier electrical/prototype prerequisites](../../../../logs/r2mlvs3mp-z-d-power-prerequisites.log) pass1,223/22. [Actual native files](../../../../logs/r2mlvs3mp-z-d-tube-power-native-checks-3.log) prove retained385 grant/effect loading, new386 grant/use/closure, exact four-chunk reloads, four malformed-preview refusals with valid retry, and [ordinary game loading/resaving](../../../../logs/r2mlvs3mp-z-d-tube-power-normal-loader.log). The original paid excavation and385 utility inputs/outputs are preserved. [Both Debug targets build](../../../../logs/r2mlvs3mp-z-d-tube-power-build-7.log); optimized checkpoint verification follows with dispatched power foundations. This is actual source/grant/service delivery, without claiming native metered propulsion, terminal integration or normal newgame useful-route acceptance. **Group6 is current; master10.2 remains unchecked.**

### Local Mandatory Reading - Native metered capsule ticks

_Applies to Group6._

Read this plan's Global and the Paid tube power Area's entry gate and tick sketch. Consume the newly delivered Group5 public source/invoice API, `utility_power.h` grant/mutation boundary and the frozen `TubeWork` unit contract in `multilayer_utility.cpp`. Inspect `Train::Tick`, both `TrainLocoHandler` substeps, actual `VehicleServiceInDepot`/depot departure, `CallVehicleTicks` and the simulation tick owner. A per-front cache callback cannot close the global allocation early or charge twice. Same-company source/capsule bindings and fractional demand need saved actual IDs; physical movement and service must refuse missing grants under both acceleration models and force-proceed. Only6.4 waits for the later geothermal producer. Preserve Group5's actual386 source/invoice files and completed385 ground/electrical/art evidence; their replay is a prefix, never later fleet/route acceptance.

### Group 6 - Meter movement and service demand through shared grants

Resumed2026-10-08 after paid external service Group5: the historical finalization exception is not a stop gate. Implement native metering6.1-6.3 now; retain6.4 until actual geothermal source/export delivery at master10.6.

{{HARD}}
- [X] 6.1 Implement `ComputeTubePowerDemand` from frozen model, load, motion/service phase and interval units. Use checked integer/remainder accounting and one front-vehicle tick hook.
- [X] 6.2 Reserve/read a utility grant before powered movement/service; settle actual consumption once. Railtype powered compatibility means mechanical compatibility, not available energy.
- [X] 6.3 Guard actual movement/departure under both acceleration models and force-proceed; zero cached horsepower alone is insufficient. Charge once per simulation epoch, never in `GetPower`, `CargoChanged` or `ConsistChanged` cache refreshes.
- [ ] 6.4 After r2mlvs18 exists, attach its explicit geothermal electrical supply to the same consumer contract and test source switching. Missing geothermal blocks only that integration, not paid-grid movement.

6.1-6.3 closure,2026-10-08: complete actual-consist model/load/phase demand integrates checked27-ms units with saved millijoule carry. Native `CallVehicleTicks` prepares all capsule companies together; one front `TubeNativeTick` surrounds both locomotive substeps and complete paid settlement. Native service/departure/movement need admission under both acceleration models and force-proceed; cache callbacks never invoice. The common ledger rejects raw paid-source consumption outside its invoice participant. [Debug](../../../../logs/r2mlvs3mp-z-e-tube-meter-final-checks-1.log) and [optimized](../../../../logs/r2mlvs3mp-z-e-tube-meter-optimized-checks.log) selected meter/source/junction/geometry/utility/metadata checks pass18,470 assertions/28 cases, including actual partial-load travel/carry, native service, cash refusal and binding estimate/refusal purity. [Real387/retained386 files](../../../../logs/r2mlvs3mp-z-e-tube-meter-native-checks-4.log) prove exact binding/source/invoice/grant reload, next service, saved loss and five corrupt preview refusals with valid retry; [ordinary loading/resaving](../../../../logs/r2mlvs3mp-z-e-tube-meter-normal-loader.log) preserves all six families. A waiting binding correctly retains a retired source whose eligibility is empty. Earlier junction fixtures now use actual paid sources; native depot departure setup remains inside its funded work action, avoiding an ownerless hidden-depot reservation. Both game/test configurations build; [read-only version discovery preserves Git's index](../../../../logs/r2mlvs3mp-z-e-read-only-build.log).6.4 alone returns after master10.6. **Group7 is current; master10.2 remains unchecked.**

### Local Mandatory Reading - Power loss, topology and recovery

_Applies to Group7._

Read Global and the Paid tube power Area's entry gate. Consume tested Groups5/6 and `utility_power.h` original-source consume/release/retirement. Inspect `GetTubePowerPhase`, `PrepareTubePowerTick`, native movement/depot gates, `VehicleServiceInDepot`, actual rail/depot/station layout hooks and portal/company lifecycle callbacks. Keep power waits separate from player stops and cached horsepower. Derived components follow actual owned guideway geometry, revoke only unused offers after eligible changes and rebuild without per-interval YAPF invalidation. Preserve consumed energy, invoices, demand fractions and real packet identity; a retired source is historical identity, never a free replacement contract.387 waiting files and386/385 inputs are authoritative prefixes, without terminal/full fleet/bootstrap claims.

Context has compacted 1 times while executing this group.

### Group 7 - Recover power waits without changing manual stops

Resumed2026-10-08 after native metering6.1-6.3: retain the separate power wait and player stop, derive connectivity from actual owned guideways, and test physical split/repair, paid source replacement and exact historical accounting. The prior finalization skip is historical; this group is active.

{{HARD}}
- [X] 7.1 Represent grid waiting separately from a player stop. Existing `PowerChanged` can set Stopped without clearing it on restoration; reconnect only resumes a vehicle that was waiting for power, never one manually stopped.
- [X] 7.2 Brake/hold safely while preserving cargo/reservations and necessary service/cleanup ticks. Test moving, queued and depot-service capsules under loss and restoration of a grant.
- [X] 7.3 On power topology split/rejoin, rebuild affected connected components and revoke stale unconsumed grants through the common ledger. Preserve consumed/charged history and stable consumer ordering.
- [X] 7.4 Keep physical path topology stable across routine allocation intervals; invalidate YAPF only for actual eligible layout/capability changes, not every power query. Test blocked and alternative routes after infrastructure changes.

Group7 closure,2026-10-08: actual owned layer-local trackdir components derive electricity independently of native reservations/signals. Eligible physical layout changes revoke only unused original slices and lazily rebuild affected components; routine allocation/readiness does not invalidate YAPF. Native motion, queued departure and damaged depot service hold safely with packet/manual-stop custody retained, then resume admitted work after cash/guideway restoration. Paid source replacement creates fresh identities and moves demand carry once; portal sealing/company takeover retire old sources, retaining used/invoiced history and insolvent fractional debt. [Debug focused checks](../../../../logs/r2mlvs3mp-z-f-tube-recovery-checks-6.log) pass10,202 assertions/27 cases; [optimized recovery plus affected junction/geometry](../../../../logs/r2mlvs3mp-z-f-tube-recovery-optimized-checks.log) passes20,802/32. [Real387 recovery/retained386/387 files](../../../../logs/r2mlvs3mp-z-f-tube-recovery-native-checks-2.log) prove paid replacement, exact six-family reload, no repeated service, five malformed previews and [ordinary game loading](../../../../logs/r2mlvs3mp-z-f-tube-meter-normal-loader.log); earlier meter387 inputs remain hash-identical. [Debug](../../../../logs/r2mlvs3mp-z-f-tube-recovery-build-8.log) and [optimized](../../../../logs/r2mlvs3mp-z-f-tube-recovery-optimized-build.log) game/test targets build. Recovery tests now have a separate translation unit and shared actual-route fixture. **Group8 is current; master10.2 stays unchecked.**

### Local Mandatory Reading - Public tube readiness

_Applies to Group8._

Read Global and the Paid tube power Area's tested entry gate. Inspect `transport_tube.h`, the canonical native station/depot IDs and platform geometry, `ComputeTubePowerDemand`, actual saved source/binding queries and rebuildable guideway connectivity. Expose immutable borrowed/copied views with explicit company/layer/date and invalid-reference behavior; no query grants energy, registers a vehicle, moves cargo or issues an invoice. Native station packets remain the freight owner. Group8 publishes views over actual eligible facilities, while terminal commands/full five-model/normal-game activation remain later Groups9-12. Preserve historical386/387 state and Group7's exact recovery evidence.

### Group 8 - Publish tube construction and fleet contracts

Resumed2026-10-08 after recovery7: publish actual read-only facility/network views and verify earlier public boundaries before downstream controls. The prior finalization skip is historical.

{{HARD}}
- [X] 8.1 Publish `src/transport_tube.h` exports `TubeVehicleCapability`, `TubeTerminal`, `CheckTubeConstruction`, `ComputeTubePowerDemand` with layer/cargo/rail/power ownership and invalid-reference rules.
- [X] 8.2 Expose read-only network/terminal readiness for r2mlvs7 controls and conventional freight integration. Keep actual grants owned by `utility_power.h` and derived connectivity rebuildable.
- [X] 8.3 Register source/build entries and compile/run construction, separation and power-boundary fixtures before downstream UI claims operational tube support.

Group8 closure,2026-10-08: public native facility views retain actual station/depot IDs, explicit owner/layer, canonical UTUB and native platform length. Exact model/cargo/calendar admission reuses the construction/capability owner. Network views expose only physically reachable active paid inlets and current minimum lease/usage affordability, without acquiring consumers, grants or stock. Invalid/foreign/ordinary/wrong-layer facilities refuse; real guideway split/repair and retired source change readiness. [Debug](../../../../logs/r2mlvs3mp-z-f-tube-readiness-checks-1.log) and [optimized](../../../../logs/r2mlvs3mp-z-g-tube-readiness-optimized-checks.log) construction/separation/registry/native movement/power/recovery/readiness bursts pass25,086 assertions/50 cases. [Actual386/387 loaded-world queries](../../../../logs/r2mlvs3mp-z-g-tube-readiness-native-checks.log) preserve all four/six source/invoice/binding/ledger authority chunks and original inputs. [Debug targets](../../../../logs/r2mlvs3mp-z-g-tube-readiness-build-3.log) and [optimized targets/index preservation](../../../../logs/r2mlvs3mp-z-g-tube-readiness-optimized-build.log) pass. This completes dispatched paid-grid Groups5-8/master10.2 except the explicitly staged6.4; terminal commands, production five-model activation and final art remain later groups. **Master10.3 / Plan18 Group1 is current.**

### Area Mandatory Reading - Terminals, fleet and controls

_Applies to Groups 9-12, including staged10B._

**Entry gate.** The following public contracts must exist before dependent implementation in this area: `src/transport_tube.h`: `TubeVehicleCapability`, `TubeTerminal`, `CheckTubeConstruction`, `ComputeTubePowerDemand`.
Verify each public file/export/schema field with `rg` in the current workspace and inspect its command/field ownership. For a same-plan contract, compile its type/signature fixture or validate its artifact after the preceding producer groups. For cross-plan behavior, also run the producer's focused executable boundary test; declarations alone cannot authorize dependent mutations or runtime claims.
If a required export is absent, stop the dependent implementation, mutation and runtime claims in Groups 9-12. Read-only characterization, source/range verification, test-case design and asset-template work that does not assume the missing behavior may continue.
Do not bypass this gate by copying private pool/tile state, inventing local IDs or cargo registries, using a historical plan as a delivered implementation, opening user-wide content stores, substituting a second utility/accounting authority, or adding an unreviewed fallback path. Finish the named producer or revise the shared contract and its direct consumers first.

| File name | Line Ranges | Description |
| --- | --- | --- |
| [src/vehicle_cmd.h](../../../../src/vehicle_cmd.h) | 20-42 | Build/refit/clone/depot-autoreplace commands and traits expose all purchase paths; `CloneVehicle` is `NoTest`, so do not rely on a generic estimate to enforce capability/commissioning. |
| [src/cargopacket.h](../../../../src/cargopacket.h) | 596-603 | `StationCargoList::Reserve`, `Load`, `Truncate`, `Reroute` provide bounded station moves and destination reservations; hub inventory has one physical owner. |
| [src/train.h](../../../../src/train.h) | 120-148 | `Train::Tick`, `GetRunningCost`, `ConsistChanged` and acceleration declarations own simulation progress, costs and consist caches; view state cannot advance cutting. |
| `src/transport_tube.h` (planned public contract; link after implementation) | Live definitions; no fixed range | [r2mlvs17-underground-transport-tubes.md](r2mlvs17-underground-transport-tubes.md) Groups 1-8, ending with Group 8's public-contract task, publish `TubeVehicleCapability`, `TubeTerminal`, `CheckTubeConstruction`, `ComputeTubePowerDemand`. These exports/schema fields are authoritative for this area. Same-plan consumers follow those earlier groups; cross-plan consumers verify the producer's required implementation/tests as well. Proposed names alone do not prove delivery. |
| `src/underground_gui.h` (planned public contract; link after implementation) | Live definitions; no fixed range | [r2mlvs7](r2mlvs7-underground-construction-and-fleet.md) `Publish underground UI entry points` supplies `ShowUndergroundConstructionToolbar` and `BuildUndergroundVehicleFilter`; required only for binding tube controls to that UI, not tube state/commands or power. |
| [Sprite Contract and delivery owner](r2mlvs13-graphics-and-asset-pipeline.md#public-handoff-contract) | Live definitions; no fixed range | `Sprite Contract Handoff` defines manifest fields `asset_id`, `consumer`, `directions`, `frames`, `anchor`, `source`; the matching authoring/conversion/runtime groups deliver actual assets. Verify the requested family and packed loading fixture before graphics integration; template/manifest publication alone is not final art. |

**ASSUMPTION: this was already built previously. If not, re-asses at runtime**. This applies only to the producer contracts named in this area's gate, after their listed producer groups have run. Verify their actual exports and focused tests before using the sketches below; proposed types and helper names in sketches are not evidence of delivered code.

**Five-model integration.** Reuse the previously tested tube railtype/power path and add five separate canonical engine definitions. The same cargo label has one declared physical form; Cistern-80 liquid service cannot silently accept Crate-75 packaged goods. r2mlvs13 supplies each model's original directional/load art; missing art leaves that integration/visual acceptance pending, while definition/refit tests can run.

**Scoped toolbar gate. ASSUMPTION: this was already built previously. If not, re-asses at runtime**. Tube toolbar/fleet-filter binding consumes the r2mlvs7 exports above. Verify them and their focused UI fixture before integration; absent UI stops only that binding, while tube commands, terminals and model tests continue. Do not create a second underground toolbar owner.

**Scoped graphics gate. ASSUMPTION: this was already built previously. If not, re-asses at runtime**. Final graphics tasks in this area require the corresponding r2mlvs13 manifest entries, delivered sprites and packed loading fixture. Verify them before binding/review; if absent, stop only those art tasks and their acceptance claims, continue independent code/state tests, and never count placeholders as finished assets.

### Local Mandatory Reading - Current native interchange cargo

_Applies to Group9; supplements the Terminals, fleet and controls Area._

The current producer gate is `logs/underground-tubes/20261021-terminal-01/producer-debug-04/run.json`:6,878 assertions/seventeen cases,1.719s. Do not repeat completed art authoring/packing. Inspect `transport_tube_cargo.h/.cpp`, native `VehicleCargoList::Stage`/`Unload` and the current `transport_tube_freight_fixture.h`/`transport_tube_interchange*.cpp` only for this cargo boundary. `tube_terminal_checks.py` builds/runs isolated `openttd_tube_test` with a60sec native timeout; logs stay under that terminal run directory. Baseline before.json protects2,318 prior files/retained applications, permitting only named CMake owners and cargopacket.cpp. No existing application is relinked. Original provider tests remain unchanged; `tube_test_environment.cpp` explicitly initializes their font/language prerequisites. Group10 model activation, controls, final art, save/lifecycle and driven TBM-cleared/cross-layer acceptance remain their later tasks.

### Group 9 - Add terminals and accountable freight interchange

Resumed2026-10-21 under master10.5; the earlier2026-10-08 GP-TG skip is historical. The current native provider gate passes6,878 assertions/seventeen cases in Debug, including actual station/depot floor/approach admission, paid native movement/service and joined tube/ordinary-rail/truck geometry. The isolated target explicitly initializes font/language prerequisites previously supplied by full-suite ordering; its first uninitialized station-sign failure remains diagnostic evidence. Current work is operating cargo/interchange admission, not repeating completed source art.

**Transfer policy.** One actual native StationID owns joined tube/rail/road stock. Its first-release interchange buffer admits at most1,024 whole units **per cargo**, counting available stock, native loading reservations and transfers already staged on actual queued vehicles. No separate stock, packet/reservation identity, cross-layer catchment or income ledger is introduced. Partial packets split through the existing native pool; a full buffer or allocation refusal retains cargo aboard without feeder credit for the refused portion. Recheck physical room when unloading; existing overfull stock is preserved rather than truncated. Ordinary stations retain their existing behavior. Native feeder shares remain virtual until the final conventional delivery. This conservative bound is a gameplay admission limit awaiting measured balance, not a claim of useful-route throughput. Boundary fixtures disclose supplied packets/arrival positions; driven TBM clearing/cross-layer routes and five-model silhouettes remain later acceptance.

{{HARD}}
- [X] 9.1 Build fully excavated tube loading terminals and powered service depots with explicit tube approach/berth capacity. Reuse canonical station/cargo identities where appropriate rather than a duplicate invisible stock store.
- [X] 9.2 Connect tube-to-rail/road interchange through layer-local packet transfer and compatible loading facilities. Joining facilities cannot let capsules traverse conventional tracks or grant same-XY cross-layer catchment.
- [X] 9.3 Define bounded stock/reservations, empty returns and transfer payment semantics; verify full receiving buffers retain cargo and the final cross-layer leg uses a conventional carrier.

Group9 closure,2026-10-21: [native qualification](../../../../logs/underground-tubes/20261021-terminal-01/qualification.json) joins8,687 assertions/24cases in **each** configuration (Debug4.594s, RelWithDebInfo1.141s), with identical native terminal logs and retained exact source/executable copies. Actual commands build fully excavated tube/ordinary-rail/truck berths with one StationID; capsule movement remains exclusive. Native paid movement/service and platform-role/length tests retain existing owners. The new finite transfer guard counts native stock, queued transfer packets and loading reservations in place, splits only admitted packet portions, keeps full/overfull or late-refused cargo aboard, and preserves ordinary station behavior. A genuinely driven paid capsule unloads its supplied17-unit packet and physically returns empty under station/depot orders. Both ordinary rail and road load the transferred packet, retain its virtual feeder share and settle exactly one final income at an actual Surface power plant; a repeat empty visit pays nothing. The latter test explicitly places the conventional carrier at Surface to isolate final settlement; driven cross-layer/TBM-cleared useful routes, five operating models, final graphics and acceptance remain pending. All2,318 predecessor files/retained applications are protected except four named build/packet owners; no Space implementation or GRF change. Earlier zero-price/unregistered-source and omitted original tile-label-finalization failures were fixture defects and remain diagnostic evidence; current fixtures use actual source-station commands and native label resolution.

### Local Mandatory Reading - Capsule physical geometry before source art

_Applies to staged Group10B; supplements Terminals, fleet and controls Area._

Master10.4/Plan13 Groups50-53 need native geometry before their source art, while operating fleet activation remains master10.5. Read this plan's Global/Terminals fleet Area, canonical `LayerVehicleCapability` / `GetMultiLayerVehicleCapabilities`, actual `InitialiseTubeEngineDefinitions`, `RailVehicleInfo::shorten_factor` and `Train::ConsistChanged` length ownership. Publish only a passive borrowed-catalogue physical descriptor for one full native rail part per capsule; the existing model9 producer must consume it. Querying future model geometry cannot allocate/activate an engine, bypass dates/rail domains or copy a cargo/energy registry. Original Group10 definitions/stats/model validation remain unchecked. Use a narrow public header to avoid unnecessary broad root-header rebuilds.

### Group 10B - Freeze canonical capsule physical geometry for source authoring

Staged2026-10-10 before Plan13 task50.1 under master10.4. This bounded producer supplies source geometry only; it does not execute or close the operating fleet work in original Group10.

{{MEDIUM}}
- [X] 10B.1 Publish canonical catalogue-based immutable capsule geometry with borrowed capability, full native length and one physical part; reject non-tube/unknown logical models without mutation.
- [X] 10B.2 Bind the existing model9 native definition to that geometry without changing its price/cargo/physics/identity or activating models10-13.
- [X] 10B.3 Verify pure all-model queries and actual command-bought prototype length/part count in one focused producer burst, then hand geometry to Plan13 Groups50-53. Full definitions/purchase/refit/date/art/runtime validation remain original Group10/master10.5.

10B closure,2026-10-10: [`transport_tube_model.h`](../../../../src/transport_tube_model.h) publishes callable `GetTubeVehicleGeometry` over the sole canonical catalogue, with one full native8-unit part for each of the five designs. Model9 initialization explicitly consumes that spacing; actual command purchase proves one part and matching native cached length. Queries before/after installation preserve engine/vehicle/packet counts, funds and utility state, reject other movement domains/unknown IDs, and leave models10-13 uninstalled. Game/test targets build in both configurations; the combined model/prototype/readiness/platform/interchange burst passes2,111 assertions/twelve cases in each. [Acceptance identities](../../../../logs/graphics/tube-source/20261010-model-geometry-01/verification.json) bind14 sources and four binaries. No new save state or operating fleet activation was added. Original Group10 stays unchecked and must consume this geometry when its definitions arrive. Plan13 task50.1 source may now proceed.

### Local Mandatory Reading - Five native capsule definitions

_Applies to Group10; supplements the Terminals, fleet and controls Area._

Read `transport_tube_engine.cpp` native EIDS/definition producer and `transport_tube_model.h` geometry, canonical `multilayer_vehicle_catalogue.cpp` rows9-13 and `multilayer_bootstrap.h/.cpp::GetMultiLayerTubeEnergyModels`. Cargo labels/dates/layers and propulsion rates already have those sole owners. Existing `InitialiseTubeEngineDefinitions` is the retained Mole-only prototype entry point; extend its shared owner with explicit full-fleet installation rather than rewriting historical prototype proofs or duplicating a registry. Whole-fleet preflight must validate every cargo, EIDS collision and capacity before allocating the first new definition. Preserve model9's current capacity60/speed100/price factor120 and energy10/20kW. New model stats need a frozen profile table; native price/running factors use the existing economy, not another cash owner. New-world tool/startup activation and all purchase/refit/clone/replacement guards remain Groups11-12. Group10.6 consumes the actual accepted packed capsule/purchase families, never placeholders.

Group10 baseline,2026-10-21: `logs/underground-tubes/20261021-models-01/before.json` protects2,389 prior files, including completed Group9 sources/images/qualification and unchanged packet guard. Its named existing mutation paths are root/src/tests CMake owners, transport_tube_engine.cpp and newgrf.cpp. A subsequent `before-definitions.json` retains2,389 files at the current partial implementation epoch and additionally authorizes transport_tube.h documentation changes; that header has not been changed yet. Preserve both snapshots and their different epochs. No Group10 compaction has occurred.

#### Checkpoint handover — paused at the user's request,2026-10-21

Cleanup handover,2026-10-23: the [paused source checkpoint](../../../../src/tests/fixtures/checkpoints/underground/checkpoint.json) retains both `before.json` and `before-definitions.json`, four provisional source owners and completed Group9terminal history. Public `python -B src/tests/multilayer/support/artifact_rebuild.py tube-checkpoint --run-id recovery-tube-history-001` verifies42retained witnesses/772original art files and the historical24cases/8687assertions/eight-tag model9 scope; this mechanism passed staged R2FC2Group32. Group10remains provisional and unqualified. Its incidental shared-engine compilation does not implement or test the five-model feature. Real cleanup passed Group33; old EXE paths are facts, not required images. Space recovered and completed its pure Goods query in the user-selected original checkout; its exported successor dispatches a read-only usefulness gate. Use the live Space master/handover. Tube work remains paused. The earlier other-worktree/not-built statements below describe the original pause snapshot.


**This handover note can be deleted once we pick up again and have recovered the checkpoint.** Stop here: Group9 is complete; current work is **master10.5 / this Group10**, with every10.1-10.6 checkbox still unchecked. Do not interpret the older2026-10-08 skip below as current authority. Space is complete in another worktree. No Git writes, subagents, commits or checkout operations are authorized. No build, test or game process remains running at this checkpoint.

**Read on resume.** Read the master's Global Mandatory Reading/current10.5 checkpoint and Selected asset, plant and lifecycle Local, then this plan's Global and closest **Five native capsule definitions** Local above. Restrict further reading to the current native definition symbols and canonical tube rows; completed Plan14 source/packing and Group9 do not need to be redone. If resuming after a real context compaction, increment the count for current Group10, not completed Group9 or staged10B. This group has not compacted yet. The older selected-art continuation document's statement that model implementation has not started is now stale; this in-plan handover supersedes it.

**Completed and retained.** Group9's [qualification](../../../../logs/underground-tubes/20261021-terminal-01/qualification.json) passes8,687 assertions/24cases in both Debug4.594s and RelWithDebInfo1.141s, with identical native terminal logs. Exact executed images and their source copies are under `logs/underground-tubes/20261021-terminal-01/retained/`. Native1,024-unit per-cargo transfer admission, real packet custody, queued/reserved space, partial/full/late refusals, conventional rail/road final payment once, and genuinely driven paid capsule/empty return are delivered. Explicit fixture floor/inlet/packets and Surface placement for the isolated final-payment check remain disclosed. These are not the later TBM-cleared/driven cross-layer useful-route acceptance. The new guard is `transport_tube_cargo.h/.cpp`, bound in native `VehicleCargoList::Stage`/`Unload`; do not change its completed source or rewrite historical receipts without a newly scoped producer task.

**Current incomplete edits — NOT BUILT OR TESTED.** New `src/transport_tube_fleet.h/.cpp` publish immutable `TubeVehicleTuning` and the full-fleet installer declaration; src/CMakeLists.txt registers them. `src/transport_tube_engine.cpp` now broadens native EIDS queries to canonical tube models9-13 and refactors definition application through `ApplyTubeDefinition`/`InitialiseTubeModels`. The old `InitialiseTubeEngineDefinitions` remains a Mole-only wrapper, so existing prototype fixtures can retain their original contract. New `InitialiseTubeFleetDefinitions` enumerates the sole canonical tube catalogue and uses whole-set cargo/identity/capacity preflight. Fixed persisted slots are reconstructed before fresh first-free allocation, preventing a fresh model from claiming a later model's saved slot. New native model tuning is documented in the existing `r2mlvs1-e-ground-and-space-budgets.md` capsule table. This code is provisional until focused compilation/execution succeeds; no model checkbox is completed by it.

The tuning rows are `(model,capacity,speed,power_hp,empty_t,price_factor,running_factor,load_amount,refit_factor,tractive_effort)`:9=`60,100,1000,20,120,100,8,10,128`;10=`40,140,1200,18,150,110,12,10,128`;11=`40,100,1000,25,180,130,6,15,128`;12=`24,110,1000,35,220,180,4,20,128`;13=`96,80,900,60,250,220,4,20,96`. Factors use the existing economy; no new cash authority. Cargo/date/layer policy remains `multilayer_vehicle_catalogue.cpp` rows9-13. Existing `GetMultiLayerTubeEnergyModels` owns empty/full kW10/20,12/18,12/24,10/20,20/50 respectively; no copied propulsion ledger. Every capsule remains one native8-unit part, with original accepted source/purchase art already packed. Model9 coefficients are intentionally unchanged.

**Next concrete work.** First review/compile the partial producer; `transport_tube.h` still describes `GetTubeEngine` as prototype-model9-only and needs its already scoped documentation update. Add focused full-fleet cases and a new runner using `before-definitions.json`; the completed `tube_terminal_checks.py` runner belongs to its earlier epoch. Reuse the isolated openttd_tube_test target/environment without relinking old applications. Test actual distinct EIDS installation/idempotence, command-bought single-part models and exact refits/load-derived energy at their dates; test a late missing SCMP label, late foreign/duplicate persisted mapping, persisted-hole reconstruction before fresh allocation and insufficient whole-fleet pool capacity with no partial publication. Preserve the old Mole-only producer fixtures instead of weakening their historical expectations. Native pool allocation checks must precede creation; RailVehicleInfo capacity and price/running fields are byte-sized, so preflight/casts must remain bounded.

No full-fleet tests/runner, model-art validation/binding, new-world startup/toolbar activation or Group11 clone/refit/replacement guards have been added yet. `src/newgrf.cpp` is still unchanged and restores only the existing prototype for saved tube-power metadata. Complete Group10 in task order, including actual packed model/purchase validation for10.6; only then continue11/12. Startup/tools are dispatched by12 and actual infrastructure art/save/lifecycle/useful-route work by master10.7. Keep master10.5 unchecked until its entire Groups9-12 scope is complete. Focused native processes remain capped60sec; poll running work with50sec waits, no full regressions. New logs go under `logs/underground-tubes/20261021-models-01/`; disposable intermediates go under repository test-cache.

**Preserved graphics boundary.** Selected master10.4A-D/aggregate10.4 and Plan14 selected closure are complete. Final selected receipt is `logs/proofs/mlm/native-art/ground/ground-consumers-v1/selected-current-closure.json`:50 families/15,488 poses,24 delivered current consumers,26 later owners. Current GRF remains SHA`c04fbf0d5525edc53250cfbd963f5e3bbe0d0bce655a3553c73b76483a8e8744`; no Group9/10 art or GRF edits were made. Earlier payload/source reports retain their own historical hashes. Preserve Space and earlier executed images; do not rerun old publication guards against a newer source epoch or imply full underground/human acceptance.

### Group 10 - Register the five original capsule definitions

Goal-authorized skip (2026-10-08, master finalization): dispatched expansion work in this group is skipped under [finalization exception GP-TG](../../../zz-rhr/upgrades/2026-10/portal-tbm/r2mlvs3mp-z-a-finalization-ledger.md#gp-tg-tube-and-geothermal-expansion-requires-major-gap-planning). Existing delivered prefixes stay complete; unimplemented profile work remains pending. No skipped task is markedX.

{{MEDIUM}}
- [ ] 10.1 Register Mole-70 (1970) as an original bulk model with profile price/capacity/speed/loaded energy and compatible ore/Rock/Earth refits outside the mandatory TBM haul.
- [ ] 10.2 Register Crate-75 (1975) as its own container model with Goods/Batteries/Electronics/Mirror Kit roles and distinct stats/silhouette.
- [ ] 10.3 Register Cistern-80 (1980) with the profile liquid Chemicals/Propellant forms, liquid-service compatibility and its own tank stats/art.
- [ ] 10.4 Register Shield-85 (1985) with Uranium/declared sealed-mineral capability, shielding/service price and distinct body; earlier conventional Uranium freight remains available.
- [ ] 10.5 Register Forge-05 (2005) with heavy Steel/Refined Metals/Components/Robotics roles, high payload/demand and distinct braced body.
- [ ] 10.6 Validate each model against the profile and its eight-direction/load/purchase manifest entries. Native content must provide all five without a third-party vehicle GRF.

### Group 11 - Apply tube purchase, refit and replacement admission

Goal-authorized skip (2026-10-08, master finalization): dispatched expansion work in this group is skipped under [finalization exception GP-TG](../../../zz-rhr/upgrades/2026-10/portal-tbm/r2mlvs3mp-z-a-finalization-ledger.md#gp-tg-tube-and-geothermal-expansion-requires-major-gap-planning). Existing delivered prefixes stay complete; unimplemented profile work remains pending. No skipped task is markedX.

{{HARD}}
- [ ] 11.1 Filter purchases by actual tube depot/terminal capability, model calendar date and unlocked layer; enforce the same rule in the command rather than only its list.
- [ ] 11.2 Guard clone/refit/autoreplace and train rearrangement before allocating/changing parts. Refuse mixed consists, wrong cargo form and capacity reductions that would discard existing cargo.
- [ ] 11.3 Test each model at the day before/on introduction and a valid Underground versus locked Deep depot. Shared railtype introduction cannot make a model or Deep access available early.

### Group 12 - Terminals, fleet and controls

Goal-authorized skip (2026-10-08, master finalization): dispatched expansion work in this group is skipped under [finalization exception GP-TG](../../../zz-rhr/upgrades/2026-10/portal-tbm/r2mlvs3mp-z-a-finalization-ledger.md#gp-tg-tube-and-geothermal-expansion-requires-major-gap-planning). Existing delivered prefixes stay complete; unimplemented profile work remains pending. No skipped task is markedX.

{{MEDIUM}}
- [ ] 12.1 Add Transport Tubes to Underground Construction with dedicated guideway, terminal, depot, utility and vehicle-list controls. Show route, excavation and power failures and useful cargo/refit previews.
- [ ] 12.2 Coordinate underground mine/factory orders and CargoDist transfer to conventional freight for cross-layer legs.
- [ ] 12.3 Support empty returns and bounded buffers; transfer handling cannot generate duplicate delivery income.
- [ ] 12.4 Preserve mandatory TBM spoil logistics: its spoil leaves in the same wagons behind a second locomotive and visits the originating Surface station. Tubes can carry other mine waste or later cargo legs but are not a substitute for that requirement.

### Area Mandatory Reading - Art, persistence and acceptance

_Applies to Groups 13-18._

**Raster source workflow.** New or corrected guideway, junction, terminal, service/depot and capsule originals use Plan13's [framed ImageGen prompt and extraction checks](r2mlvs13-graphics-and-asset-pipeline.md#framed-imagegen-sheets-and-safe-extraction). Use pure-magenta source frames, transparent clearance and verified rectangular crops, or separate view files. Preserve canonical guideway connectivity and every capsule direction/load/service pose; a four-view sheet does not redefine those matrices. Review model identity, cell joins and foreground apertures separately from crop safety. Exclude frames before native conversion; actual loaded movement and occlusion remain required.

**Entry gate.** The following public contracts must exist before dependent implementation in this area: `src/transport_tube.h`: `TubeVehicleCapability`, `TubeTerminal`, `CheckTubeConstruction`, `ComputeTubePowerDemand`.
Verify each public file/export/schema field with `rg` in the current workspace and inspect its command/field ownership. For a same-plan contract, compile its type/signature fixture or validate its artifact after the preceding producer groups. For cross-plan behavior, also run the producer's focused executable boundary test; declarations alone cannot authorize dependent mutations or runtime claims.
If a required export is absent, stop the dependent implementation, mutation and runtime claims in Groups 13-18. Read-only characterization, source/range verification, test-case design and asset-template work that does not assume the missing behavior may continue.
Do not bypass this gate by copying private pool/tile state, inventing local IDs or cargo registries, using a historical plan as a delivered implementation, opening user-wide content stores, substituting a second utility/accounting authority, or adding an unreviewed fallback path. Finish the named producer or revise the shared contract and its direct consumers first.

**Scoped integration gates.** The following producer dependencies apply only to the named implementation/test families. Verify each required public export and its focused producer test before those integrations; absent late producers do not block already independent features.

- `src/underground_gui.h` is required for the tube UI/player-workflow portion; command-only capsule/persistence tests may proceed in Groups 13, 15-17 (only the named integration/subscenario). If absent, stop only that integration/subscenario and its passing claim. Other independent code, art, persistence and tests in this area may continue.
- `src/geothermal.h` is required for the later geothermal-powered network portion; paid-grid bootstrap, tube art/persistence and independent routing tests may proceed in Groups 16-17 (only the named integration/subscenario). If absent, stop only that integration/subscenario and its passing claim. Other independent code, art, persistence and tests in this area may continue.

Do not emulate an absent producer with copied private state, fake inventories/energy, local IDs or a second registry/grid. A final combined closure/gallery/round-trip still waits for every required family; an independent passing case is not a pass for the missing integration.

| File name | Line Ranges | Description |
| --- | --- | --- |
| [src/saveload/vehicle_sl.cpp](../../../../src/saveload/vehicle_sl.cpp) | 1119-1196 | `VEHSChunkHandler` and registration construct the vehicle class from saved type before member loading; protect old enum meanings and validate new references. |
| [src/network/network_command.cpp](../../../../src/network/network_command.cpp) | 256-288 | `NetworkExecuteLocalCommandQueue` executes agreed frame order/company authority; all jobs use deterministic simulation time. |
| [src/network/network_server.cpp](../../../../src/network/network_server.cpp) | 670-687 | `ServerNetworkGameSocketHandler::SendSync` compares RNG seeds, not a full gameplay checksum; validate orbital/geology/cargo ledgers in addition to a no-desync result. |
| `src/transport_tube.h` (planned public contract; link after implementation) | Live definitions; no fixed range | [r2mlvs17-underground-transport-tubes.md](r2mlvs17-underground-transport-tubes.md) Groups 1-8, ending with Group 8's public-contract task, publish `TubeVehicleCapability`, `TubeTerminal`, `CheckTubeConstruction`, `ComputeTubePowerDemand`. These exports/schema fields are authoritative for this area. Same-plan consumers follow those earlier groups; cross-plan consumers verify the producer's required implementation/tests as well. Proposed names alone do not prove delivery. |
| `src/underground_gui.h` (planned public contract; link after implementation) | Live definitions; no fixed range | [r2mlvs7-underground-construction-and-fleet.md](r2mlvs7-underground-construction-and-fleet.md) Groups 1-5, ending with Group 5's public-contract task, publish `ShowUndergroundConstructionToolbar`, `BuildUndergroundVehicleFilter`. These exports/schema fields are authoritative for this area. Same-plan consumers follow those earlier groups; cross-plan consumers verify the producer's required implementation/tests as well. Proposed names alone do not prove delivery. Read this producer for the tube UI/player-workflow portion; command-only capsule/persistence tests may proceed in Groups 13, 15-17 (only the named integration/subscenario); missing it blocks only that integration, not independent work. |
| `src/geothermal.h` (planned public contract; link after implementation) | Live definitions; no fixed range | [r2mlvs18-geothermal-energy-and-industry.md](r2mlvs18-geothermal-energy-and-industry.md) Groups 1-7, ending with Group 7's public-contract task, publish `GeothermalHeatAllocation`, `GeothermalUtilityLink`, `AllocateVentHeat`, `ExportGeothermalPower`. These exports/schema fields are authoritative for this area. Same-plan consumers follow those earlier groups; cross-plan consumers verify the producer's required implementation/tests as well. Proposed names alone do not prove delivery. Read this producer for the later geothermal-powered network portion; paid-grid bootstrap, tube art/persistence and independent routing tests may proceed in Groups 16-17 (only the named integration/subscenario); missing it blocks only that integration, not independent work. |

**ASSUMPTION: this was already built previously. If not, re-asses at runtime**. This applies only to the producer contracts named in this area's gate, after their listed producer groups have run. Verify their actual exports and focused tests before using the sketches below; proposed types and helper names in sketches are not evidence of delivered code.

### Group 13 - Art, persistence and acceptance

Goal-authorized skip (2026-10-08, master finalization): dispatched expansion work in this group is skipped under [finalization exception GP-TG](../../../zz-rhr/upgrades/2026-10/portal-tbm/r2mlvs3mp-z-a-finalization-ledger.md#gp-tg-tube-and-geothermal-expansion-requires-major-gap-planning). Existing delivered prefixes stay complete; unimplemented profile work remains pending. No skipped task is markedX.

{{MEDIUM}}
- [ ] 13.1 Integrate tube straight/bend/junction/end sprites, terminal fronts/backs, depot/service animation and power indicators. Show capsules moving through readable transparent/cutaway tube art, with correct occlusion and selection.
- [ ] 13.2 Save capsule orders/positions/cargo, service/power state and network references.

### Group 14 - Rebuild tube caches and preserve recovery on lifecycle changes

Goal-authorized skip (2026-10-08, master finalization): dispatched expansion work in this group is skipped under [finalization exception GP-TG](../../../zz-rhr/upgrades/2026-10/portal-tbm/r2mlvs3mp-z-a-finalization-ledger.md#gp-tg-tube-and-geothermal-expansion-requires-major-gap-planning). Existing delivered prefixes stay complete; unimplemented profile work remains pending. No skipped task is markedX.

{{HARD}}
- [ ] 14.1 Load authoritative capsule orders/position/cargo and utility grant state before rebuilding connectivity/path/energy lookup caches. Verify rebuild cannot create power or move cargo.
- [ ] 14.2 Invalidate affected network components on guideway/interchange/depot changes and use stable traversal order; keep global rescans out of the per-tick path.
- [ ] 14.3 Handle bankruptcy/ownership change with canonical company and utility lifecycle hooks. Stop or reassign capsules deterministically, retaining cargo and a declared recovery path.
- [ ] 14.4 Round-trip a queued junction and power-starved route, then compare next-tick grants, reservations and positions.

### Group 15 - Art, persistence and acceptance

Goal-authorized skip (2026-10-08, master finalization): dispatched expansion work in this group is skipped under [finalization exception GP-TG](../../../zz-rhr/upgrades/2026-10/portal-tbm/r2mlvs3mp-z-a-finalization-ledger.md#gp-tg-tube-and-geothermal-expansion-requires-major-gap-planning). Existing delivered prefixes stay complete; unimplemented profile work remains pending. No skipped task is markedX.

{{MEDIUM}}
- [ ] 15.1 Test solid/unlocked-layer refusals, pre-1980 Deep access, ordinary train/tube incompatibility, unsafe work-track replacement, disconnected power and illegal cross-layer routing.
- [ ] 15.2 Run one useful freight route with each of the five models at its introduction date. Include Underground-to-Deep chains with explicit conventional transfer and distinguish all five silhouettes in runtime captures.

### Group 16 - Demonstrate cleared ground becoming a useful tube route

{{HARD}}
- [ ] 16.1 Use a real TBM to clear a curved corridor and large terminal cavern, withdraw cutter/hauler and remove only safe temporary support before constructing tubes.
- [ ] 16.2 Load a useful cargo, run a capsule to a layer-local interchange and deliver via conventional freight to Surface. Verify actual wagon spoil hauling from the excavation remains a separate completed leg.
- [ ] 16.3 Save/reload while capsules queue and reservations change; compare packet/position/grant ledgers and demonstrate recoverable loss/restoration of paid-grid supply.

### Group 17 - Compare tube replay and later geothermal supply

{{HARD}}
- [ ] 17.1 Run matching host/client or deterministic replay fixtures for all five models and compare state at fixed ticks, including hidden views and model-date boundaries.
- [ ] 17.2 Measure capacity, service, paid power and throughput against ordinary Underground rail/road; hand results to r2mlvs16 without declaring balance from static prices.
- [ ] 17.3 After geothermal export is delivered, repeat a network powered by that source and compare single grid credit/consumption. Keep this late case pending without blocking the independent paid-grid evidence.

### Group 18 - Art, persistence and acceptance

{{SIMPLE}}
- [ ] 18.1 Record the implemented public exports, focused test commands/results and remaining runtime/art/network gaps in this plan; retain original supplied saves and repository-local evidence.
- [ ] 18.2 Update affected architecture/handoff documentation to describe delivered behavior and reconcile the parent index only when this plan's Completion Conditions are met. Keep every unfinished task unchecked.

## Consult on Demand

| Trigger | File name | Start at | Expand only when | Consultation purpose |
| --- | --- | --- | --- | --- |
| A powered capsule cannot reserve a route on compatible tubes | [../../architecture/pathfinder/archi-pathfinder-rail.md](../../architecture/pathfinder/archi-pathfinder-rail.md) | `Segment cache` and `Reservation` | Tube capability, topology and power checks pass but cached search/reservation differs | Locate rail cache/reservation invalidation ownership while keeping capsules segregated from ordinary rails and layer portals. |
| A required public export is absent and its owner/location is disputed | [r2mlvs-multilayer-vehicles-and-structures.md](r2mlvs-multilayer-vehicles-and-structures.md) | `Requirements and ownership` and `Plan Index` | The named producer confirms an intentional contract/ownership change | Find the responsible producer and reconcile its public handoff with direct consumers; the hub cannot substitute for missing implementation. |
| Current behavior contradicts a historical subsystem description | [../../architecture/map-layers/archi-map-layers.md](../../architecture/map-layers/archi-map-layers.md) | `Invariants` and the relevant named source in `Code map` | The live symbol and executable test do not explain the difference | Locate layer addressing or lifecycle rationale. Live code/tests govern delivered behavior; this architecture page does not prove future interfaces exist. |

## Planning Provenance

This is not an execution reading queue. Do not preload it after context compaction. It is non-transitive; ranges record planning use, not runtime authority.

| File name | Sections or Line Ranges used while planning | Planning contribution |
| --- | --- | --- |
| [docs/zz-reach2/architecture/initial-inventory.md](../../architecture/initial-inventory.md) | 185-220 | `Command as the only world mutation`, `Pool-indexed game objects`, `Tile map as a packed array` and `File-suffix modules` oriented command/pool/header ownership and source registration. This historical inventory is a location survey, not authority for current layer behavior. |
| [docs/zz-reach2/architecture/@archi-ttd-level0.md](../../architecture/@archi-ttd-level0.md) | 24-63 | `How the program fits together` and `Source and runtime boundaries` located shared simulation, data/presentation, CMake targets and subsystem handoffs used to choose the live reading sources. |
| [docs/zz-reach2/upgrades/2026-10/r2mlvs-multilayer-vehicles-and-structures.md](r2mlvs-multilayer-vehicles-and-structures.md) | 119-140 | `Requirements and ownership` assigned this plan's feature responsibilities; it shaped the decomposition without making every sibling plan a mandatory reading dependency. |
| [src/rail_type.h](../../../../src/rail_type.h) | 16-39 | The inspected live boundary helped separate characterization, public-contract production, implementation and acceptance batches. It materially constrained five original layer-local capsules, excavated guideways and explicit paid power; this entry records planning influence rather than an additional reading obligation. |
| [User planning-format instructions](<C:/Users/Axonn/.codex/attachments/cd59b20a-3a96-4d54-b4bd-1e8f37639e7d/Pasted text.txt>) | `Global Mandatory Reading`, `Area Mandatory Reading`, `Consult on Demand`, `Planning Provenance`, `Final validation` | Supplies the exact protocol, chronological difficulty batches of at most eight tasks, reading gates and validation format. |
| [src/ground_vehicle.cpp](../../../../src/ground_vehicle.cpp) | 17-72 | Deeper planning pass, 2026-10-04: Power cache updates and stopped-state behavior required metered movement admission and manual-stop-preserving recovery. |

## Completion Conditions

Five original functional vehicles operate on a distinct underground tube network built only after excavation. Their roles/art/stats are distinguishable, power and cargo transfers are accountable, early play needs no Deep energy, and tubes cannot bypass the 1980 unlock or the real TBM wagon haulage loop.
