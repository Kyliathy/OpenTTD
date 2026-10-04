# R2MLVS16 - Underground Transport Tubes

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

Status: proposed; no implementation or graphics completed. Parent: [R2MLVS master](r2mlvs-multilayer-vehicles-and-structures.md). Depends on excavation/commissioning interfaces from r2mlvs2..5 and content contracts from r2mlvs1; integrates with r2mlvs7, r2mlvs13 and r2mlvs14. Final acceptance is owned by r2mlvs15. No geothermal dependency is required for the first tube network.

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

Proposed public owner: `src/transport_tube.h`. Publish `TubeVehicleCapability`, `TubeTerminal`, `CheckTubeConstruction`, `ComputeTubePowerDemand`. These symbols are producer obligations, not existing code. Keep one authority for the fields/commands they expose; downstream plans consume the public contract rather than private pools or copied tables.

## Implementation Tasks

Deepening pass, 2026-10-04: implementation groups use only SIMPLE, MEDIUM or HARD difficulty, with at most eight pending tasks each. Sketches describe proposed implementation contracts; verify the named earlier producers at each assumption gate. No gameplay or asset implementation is marked complete by this planning pass.

Groups are chronological and contain consecutive tasks of one difficulty. Each difficulty batch has at most eight tasks; area reading changes only when the implementation boundary or source of truth changes.

### Area Mandatory Reading - Tube infrastructure and capability separation

_Applies to Groups 1-4._

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
- [ ] 1.1 Allocate the profile-selected tube rail label through the canonical railtype registry; do not hard-code a runtime slot or silently collide with external content.
- [ ] 1.2 Prototype one self-propelled capsule, one tube tile and a junction using normal reservation/path foundations. Keep compatibility/powered rail masks explicit and separate from dynamic electrical supply.
- [ ] 1.3 Demonstrate ordinary trains/TBMs refuse the tube and a capsule refuses conventional/temporary rail even if a NewGRF compatibility mask is broad. Publish the semantic capability predicate used by both paths.
- [ ] 1.4 Publish the prototype `TubeVehicleCapability` declaration in planned `src/transport_tube.h` and freeze its engine/rail metadata/pathfinding fixture before power integration. Group 8 later completes the public handoff for all consumers.

### Group 2 - Guard tube construction and safe work-track conversion

{{HARD}}
- [ ] 2.1 Freeze label/capability, permitted Underground/Deep layers, terminal/depot footprints and legal rail-realizable junction/curve geometry. Tube motion geometry is separate from TBM cutting turns.
- [ ] 2.2 Call `CheckExcavatedFootprint` for all guideway, terminal, depot and utility tiles; apply the profile Deep date gate. Test GUI, script/direct command, copy and conversion on solid/Surface/Space/locked Deep ground.
- [ ] 2.3 Before replacing temporary work rail, verify TBM/hauler withdrawal and release of its occupied/reserved retreat path. Revalidate the full conversion footprint before mutation; tubes never excavate rock.
- [ ] 2.4 Refuse mixed capsule/conventional consists, ordinary portal/elevator entry and any cross-layer tube route at purchase, rearrangement, route discovery and actual movement boundaries. `CmdConvertRail` can rewrite both portal ends, so reject tube conversion of a portal/tunnel pair before any paired mutation.

### Group 3 - Implement junction leases and blocked-route recovery

{{HARD}}
- [ ] 3.1 Map each legal tube segment/junction to existing rail track bits and reservation ownership; define one capsule movement/length model and exclusive conflict rules.
- [ ] 3.2 Implement deterministic reservation requests, release on cleared segments and invalidation after layout changes. A cached path cannot authorize entry onto a converted/non-tube tile.
- [ ] 3.3 Define turnaround/service approach and a blocked-network stop state without instantaneous direction changes or disappearing cargo. Test two capsules contending at a junction and a closed branch.
- [ ] 3.4 Test removal/conversion while occupied or reserved; refuse unsafe mutation or release only future reservations while retaining the actual capsule footprint.

### Group 4 - Infrastructure and capability contract

{{MEDIUM}}
- [ ] 4.1 Freeze legal turning geometry independently of the TBM's separate 45-degree cutting constraint.

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
| `src/geothermal.h` (planned public contract; link after implementation) | Live definitions; no fixed range | [r2mlvs17](r2mlvs17-geothermal-energy-and-industry.md) `Publish heat allocation and electrical export APIs` supplies `GeothermalHeatAllocation`, `GeothermalUtilityLink`, `AllocateVentHeat`, `ExportGeothermalPower`; its following generator groups supply operational generation. Required only for the later geothermal source-switch integration. |
| [src/train_cmd.cpp](../../../../src/train_cmd.cpp) | 4229-4256 | Complete `Train::Tick` calls two locomotive substeps; tube demand/settlement needs one authoritative simulation-epoch hook. |

**Power sketch (proposed).** Compatibility, reserved track and an actual grant are separate requirements. Never charge from cache-refresh callbacks.

```text
one tube tick -> compute demand -> read/reserve shared grant
             -> guarded movement/service -> commit actual consumption once
grid loss -> separate wait reason -> safe hold with cargo retained
grid returns -> resume only a power-waiting capsule, preserve manual stop
```

**Scoped geothermal gate. ASSUMPTION: this was already built previously. If not, re-asses at runtime**. Only the later source-switch subtask in Group 6 requires r2mlvs17 generation/export. Verify its producer fixture or leave that integration pending; paid-grid work continues without a fake thermal source.

### Group 5 - Connect paid external power without a geothermal prerequisite

{{HARD}}
- [ ] 5.1 Consume r2mlvs12 `Useful Power Handoff` through `src/utility_power.h`: `PublishUtilityGeneration`, `AllocateUtilityPower`, `GetUtilityAllocation`. Verify generation/grant/effect/save fixtures before adding propulsion accounting.
- [ ] 5.2 Build an explicit power connection at an accessible commissioned portal-side interchange. Validate owner, excavated footprint, layer/date and a bounded paid external service contract; coordinate proximity is not a power link.
- [ ] 5.3 Publish external supply once per interval and settle tariff from actual granted/used service through the common utility authority. Refuse insufficient funds/capacity without creating free energy or duplicated charges.
- [ ] 5.4 Test a 1970 paid-service source/grant fixture using the earlier capsule capability prototype; no Deep/1980/geothermal output is required. The real useful route is accepted only in the final area after movement, terminals and production models exist.

### Group 6 - Meter movement and service demand through shared grants

{{HARD}}
- [ ] 6.1 Implement `ComputeTubePowerDemand` from frozen model, load, motion/service phase and interval units. Use checked integer/remainder accounting and one front-vehicle tick hook.
- [ ] 6.2 Reserve/read a utility grant before powered movement/service; settle actual consumption once. Railtype powered compatibility means mechanical compatibility, not available energy.
- [ ] 6.3 Guard actual movement/departure under both acceleration models and force-proceed; zero cached horsepower alone is insufficient. Charge once per simulation epoch, never in `GetPower`, `CargoChanged` or `ConsistChanged` cache refreshes.
- [ ] 6.4 After r2mlvs17 exists, attach its explicit geothermal electrical supply to the same consumer contract and test source switching. Missing geothermal blocks only that integration, not paid-grid movement.

### Group 7 - Recover power waits without changing manual stops

{{HARD}}
- [ ] 7.1 Represent grid waiting separately from a player stop. Existing `PowerChanged` can set Stopped without clearing it on restoration; reconnect only resumes a vehicle that was waiting for power, never one manually stopped.
- [ ] 7.2 Brake/hold safely while preserving cargo/reservations and necessary service/cleanup ticks. Test moving, queued and depot-service capsules under loss and restoration of a grant.
- [ ] 7.3 On power topology split/rejoin, rebuild affected connected components and revoke stale unconsumed grants through the common ledger. Preserve consumed/charged history and stable consumer ordering.
- [ ] 7.4 Keep physical path topology stable across routine allocation intervals; invalidate YAPF only for actual eligible layout/capability changes, not every power query. Test blocked and alternative routes after infrastructure changes.

### Group 8 - Publish tube construction and fleet contracts

{{HARD}}
- [ ] 8.1 Publish `src/transport_tube.h` exports `TubeVehicleCapability`, `TubeTerminal`, `CheckTubeConstruction`, `ComputeTubePowerDemand` with layer/cargo/rail/power ownership and invalid-reference rules.
- [ ] 8.2 Expose read-only network/terminal readiness for r2mlvs7 controls and conventional freight integration. Keep actual grants owned by `utility_power.h` and derived connectivity rebuildable.
- [ ] 8.3 Register source/build entries and compile/run construction, separation and power-boundary fixtures before downstream UI claims operational tube support.

### Area Mandatory Reading - Terminals, fleet and controls

_Applies to Groups 9-12._

**Entry gate.** The following public contracts must exist before dependent implementation in this area: `src/transport_tube.h`: `TubeVehicleCapability`, `TubeTerminal`, `CheckTubeConstruction`, `ComputeTubePowerDemand`.
Verify each public file/export/schema field with `rg` in the current workspace and inspect its command/field ownership. For a same-plan contract, compile its type/signature fixture or validate its artifact after the preceding producer groups. For cross-plan behavior, also run the producer's focused executable boundary test; declarations alone cannot authorize dependent mutations or runtime claims.
If a required export is absent, stop the dependent implementation, mutation and runtime claims in Groups 9-12. Read-only characterization, source/range verification, test-case design and asset-template work that does not assume the missing behavior may continue.
Do not bypass this gate by copying private pool/tile state, inventing local IDs or cargo registries, using a historical plan as a delivered implementation, opening user-wide content stores, substituting a second utility/accounting authority, or adding an unreviewed fallback path. Finish the named producer or revise the shared contract and its direct consumers first.

| File name | Line Ranges | Description |
| --- | --- | --- |
| [src/vehicle_cmd.h](../../../../src/vehicle_cmd.h) | 20-42 | Build/refit/clone/depot-autoreplace commands and traits expose all purchase paths; `CloneVehicle` is `NoTest`, so do not rely on a generic estimate to enforce capability/commissioning. |
| [src/cargopacket.h](../../../../src/cargopacket.h) | 596-603 | `StationCargoList::Reserve`, `Load`, `Truncate`, `Reroute` provide bounded station moves and destination reservations; hub inventory has one physical owner. |
| [src/train.h](../../../../src/train.h) | 120-148 | `Train::Tick`, `GetRunningCost`, `ConsistChanged` and acceleration declarations own simulation progress, costs and consist caches; view state cannot advance cutting. |
| `src/transport_tube.h` (planned public contract; link after implementation) | Live definitions; no fixed range | [r2mlvs16-underground-transport-tubes.md](r2mlvs16-underground-transport-tubes.md) Groups 1-8, ending with Group 8's public-contract task, publish `TubeVehicleCapability`, `TubeTerminal`, `CheckTubeConstruction`, `ComputeTubePowerDemand`. These exports/schema fields are authoritative for this area. Same-plan consumers follow those earlier groups; cross-plan consumers verify the producer's required implementation/tests as well. Proposed names alone do not prove delivery. |
| `src/underground_gui.h` (planned public contract; link after implementation) | Live definitions; no fixed range | [r2mlvs7](r2mlvs7-underground-construction-and-fleet.md) `Publish underground UI entry points` supplies `ShowUndergroundConstructionToolbar` and `BuildUndergroundVehicleFilter`; required only for binding tube controls to that UI, not tube state/commands or power. |
| [Sprite Contract and delivery owner](r2mlvs13-graphics-and-asset-pipeline.md#public-handoff-contract) | Live definitions; no fixed range | `Sprite Contract Handoff` defines manifest fields `asset_id`, `consumer`, `directions`, `frames`, `anchor`, `source`; the matching authoring/conversion/runtime groups deliver actual assets. Verify the requested family and packed loading fixture before graphics integration; template/manifest publication alone is not final art. |

**ASSUMPTION: this was already built previously. If not, re-asses at runtime**. This applies only to the producer contracts named in this area's gate, after their listed producer groups have run. Verify their actual exports and focused tests before using the sketches below; proposed types and helper names in sketches are not evidence of delivered code.

**Five-model integration.** Reuse the previously tested tube railtype/power path and add five separate canonical engine definitions. The same cargo label has one declared physical form; Cistern-80 liquid service cannot silently accept Crate-75 packaged goods. r2mlvs13 supplies each model's original directional/load art; missing art leaves that integration/visual acceptance pending, while definition/refit tests can run.

**Scoped toolbar gate. ASSUMPTION: this was already built previously. If not, re-asses at runtime**. Tube toolbar/fleet-filter binding consumes the r2mlvs7 exports above. Verify them and their focused UI fixture before integration; absent UI stops only that binding, while tube commands, terminals and model tests continue. Do not create a second underground toolbar owner.

**Scoped graphics gate. ASSUMPTION: this was already built previously. If not, re-asses at runtime**. Final graphics tasks in this area require the corresponding r2mlvs13 manifest entries, delivered sprites and packed loading fixture. Verify them before binding/review; if absent, stop only those art tasks and their acceptance claims, continue independent code/state tests, and never count placeholders as finished assets.

### Group 9 - Add terminals and accountable freight interchange

{{HARD}}
- [ ] 9.1 Build fully excavated tube loading terminals and powered service depots with explicit tube approach/berth capacity. Reuse canonical station/cargo identities where appropriate rather than a duplicate invisible stock store.
- [ ] 9.2 Connect tube-to-rail/road interchange through layer-local packet transfer and compatible loading facilities. Joining facilities cannot let capsules traverse conventional tracks or grant same-XY cross-layer catchment.
- [ ] 9.3 Define bounded stock/reservations, empty returns and transfer payment semantics; verify full receiving buffers retain cargo and the final cross-layer leg uses a conventional carrier.

### Group 10 - Register the five original capsule definitions

{{MEDIUM}}
- [ ] 10.1 Register Mole-70 (1970) as an original bulk model with profile price/capacity/speed/loaded energy and compatible ore/Rock/Earth refits outside the mandatory TBM haul.
- [ ] 10.2 Register Crate-75 (1975) as its own container model with Goods/Batteries/Electronics/Mirror Kit roles and distinct stats/silhouette.
- [ ] 10.3 Register Cistern-80 (1980) with the profile liquid Chemicals/Propellant forms, liquid-service compatibility and its own tank stats/art.
- [ ] 10.4 Register Shield-85 (1985) with Uranium/declared sealed-mineral capability, shielding/service price and distinct body; earlier conventional Uranium freight remains available.
- [ ] 10.5 Register Forge-05 (2005) with heavy Steel/Refined Metals/Components/Robotics roles, high payload/demand and distinct braced body.
- [ ] 10.6 Validate each model against the profile and its eight-direction/load/purchase manifest entries. Native content must provide all five without a third-party vehicle GRF.

### Group 11 - Apply tube purchase, refit and replacement admission

{{HARD}}
- [ ] 11.1 Filter purchases by actual tube depot/terminal capability, model calendar date and unlocked layer; enforce the same rule in the command rather than only its list.
- [ ] 11.2 Guard clone/refit/autoreplace and train rearrangement before allocating/changing parts. Refuse mixed consists, wrong cargo form and capacity reductions that would discard existing cargo.
- [ ] 11.3 Test each model at the day before/on introduction and a valid Underground versus locked Deep depot. Shared railtype introduction cannot make a model or Deep access available early.

### Group 12 - Terminals, fleet and controls

{{MEDIUM}}
- [ ] 12.1 Add Transport Tubes to Underground Construction with dedicated guideway, terminal, depot, utility and vehicle-list controls. Show route, excavation and power failures and useful cargo/refit previews.
- [ ] 12.2 Coordinate underground mine/factory orders and CargoDist transfer to conventional freight for cross-layer legs.
- [ ] 12.3 Support empty returns and bounded buffers; transfer handling cannot generate duplicate delivery income.
- [ ] 12.4 Preserve mandatory TBM spoil logistics: its spoil leaves in the same wagons behind a second locomotive and visits the originating Surface station. Tubes can carry other mine waste or later cargo legs but are not a substitute for that requirement.

### Area Mandatory Reading - Art, persistence and acceptance

_Applies to Groups 13-18._

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
| `src/transport_tube.h` (planned public contract; link after implementation) | Live definitions; no fixed range | [r2mlvs16-underground-transport-tubes.md](r2mlvs16-underground-transport-tubes.md) Groups 1-8, ending with Group 8's public-contract task, publish `TubeVehicleCapability`, `TubeTerminal`, `CheckTubeConstruction`, `ComputeTubePowerDemand`. These exports/schema fields are authoritative for this area. Same-plan consumers follow those earlier groups; cross-plan consumers verify the producer's required implementation/tests as well. Proposed names alone do not prove delivery. |
| `src/underground_gui.h` (planned public contract; link after implementation) | Live definitions; no fixed range | [r2mlvs7-underground-construction-and-fleet.md](r2mlvs7-underground-construction-and-fleet.md) Groups 1-5, ending with Group 5's public-contract task, publish `ShowUndergroundConstructionToolbar`, `BuildUndergroundVehicleFilter`. These exports/schema fields are authoritative for this area. Same-plan consumers follow those earlier groups; cross-plan consumers verify the producer's required implementation/tests as well. Proposed names alone do not prove delivery. Read this producer for the tube UI/player-workflow portion; command-only capsule/persistence tests may proceed in Groups 13, 15-17 (only the named integration/subscenario); missing it blocks only that integration, not independent work. |
| `src/geothermal.h` (planned public contract; link after implementation) | Live definitions; no fixed range | [r2mlvs17-geothermal-energy-and-industry.md](r2mlvs17-geothermal-energy-and-industry.md) Groups 1-7, ending with Group 7's public-contract task, publish `GeothermalHeatAllocation`, `GeothermalUtilityLink`, `AllocateVentHeat`, `ExportGeothermalPower`. These exports/schema fields are authoritative for this area. Same-plan consumers follow those earlier groups; cross-plan consumers verify the producer's required implementation/tests as well. Proposed names alone do not prove delivery. Read this producer for the later geothermal-powered network portion; paid-grid bootstrap, tube art/persistence and independent routing tests may proceed in Groups 16-17 (only the named integration/subscenario); missing it blocks only that integration, not independent work. |

**ASSUMPTION: this was already built previously. If not, re-asses at runtime**. This applies only to the producer contracts named in this area's gate, after their listed producer groups have run. Verify their actual exports and focused tests before using the sketches below; proposed types and helper names in sketches are not evidence of delivered code.

### Group 13 - Art, persistence and acceptance

{{MEDIUM}}
- [ ] 13.1 Integrate tube straight/bend/junction/end sprites, terminal fronts/backs, depot/service animation and power indicators. Show capsules moving through readable transparent/cutaway tube art, with correct occlusion and selection.
- [ ] 13.2 Save capsule orders/positions/cargo, service/power state and network references.

### Group 14 - Rebuild tube caches and preserve recovery on lifecycle changes

{{HARD}}
- [ ] 14.1 Load authoritative capsule orders/position/cargo and utility grant state before rebuilding connectivity/path/energy lookup caches. Verify rebuild cannot create power or move cargo.
- [ ] 14.2 Invalidate affected network components on guideway/interchange/depot changes and use stable traversal order; keep global rescans out of the per-tick path.
- [ ] 14.3 Handle bankruptcy/ownership change with canonical company and utility lifecycle hooks. Stop or reassign capsules deterministically, retaining cargo and a declared recovery path.
- [ ] 14.4 Round-trip a queued junction and power-starved route, then compare next-tick grants, reservations and positions.

### Group 15 - Art, persistence and acceptance

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
- [ ] 17.2 Measure capacity, service, paid power and throughput against ordinary Underground rail/road; hand results to r2mlvs15 without declaring balance from static prices.
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
