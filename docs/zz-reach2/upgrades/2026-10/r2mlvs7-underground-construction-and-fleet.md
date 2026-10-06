# R2MLVS7 - Underground Construction and Fleet

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
| [docs/zz-reach2/upgrades/2026-10/r2mlvs-multilayer-vehicles-and-structures.md](r2mlvs-multilayer-vehicles-and-structures.md) | 248-254 | `AD-8` supplies the canonical gameplay/ownership decision for every group of this plan. It constrains clicked-layer tools, usable fleet filters and command-enforced construction capabilities; a consumer must not redefine it locally. |
| [src/map_layer.h](../../../../src/map_layer.h) | 78-85 | `MultiLayerMap` ownership preamble: `Map` owns packed storage/geometry and this controller owns layer semantics. Every group uses world tiles/explicit layer helpers; camera selection and ad hoc plane offsets cannot define gameplay. |
| [src/window_gui.h](../../../../src/window_gui.h) | 799-805 | `Window::OnPlaceObject` receives the clicked world tile; construction uses that layer even from extra viewports. |

Status: in progress; Group 1 completed. Parent: [R2MLVS master](r2mlvs-multilayer-vehicles-and-structures.md). Depends on r2mlvs3/5/6. Execution evidence: [underground interface ledger](r2mlvs7-a-underground-interface.md).

## Outcome and Boundaries

Replace underground airport controls with a useful construction system and deliver the stations, depots and vehicles needed for mining. Integrate Transport Tubes and geothermal controls from r2mlvs16/17; those plans own the five original tube models and thermal simulation.

## Public Handoff Contract

Proposed public owner: `src/underground_gui.h`. Publish `ShowUndergroundConstructionToolbar`, `BuildUndergroundVehicleFilter`. These symbols are producer obligations, not existing code. Keep one authority for the fields/commands they expose; downstream plans consume the public contract rather than private pools or copied tables.

## Implementation Tasks

Deepening pass, 2026-10-04: implementation groups use only SIMPLE, MEDIUM or HARD difficulty, with at most eight pending tasks each. Sketches describe proposed implementation contracts; verify the named earlier producers at each assumption gate. No gameplay or asset implementation is marked complete by this planning pass.

Groups are chronological and contain consecutive tasks of one difficulty. Each difficulty batch has at most eight tasks; area reading changes only when the implementation boundary or source of truth changes.

### Area Mandatory Reading - Underground construction interface

_Applies to Groups 1-5._

**Entry gate.** The following public contracts must exist before dependent implementation in this area: `src/multilayer_content.h`: `MultiLayerGameplayProfile`, `LayerVehicleCapability`, `GetMultiLayerContentProfile`, `CheckDeepLayerAvailability`, `UtilityPowerAccount`; `src/excavation.h`: `ExcavationState`, `GeologyDeposit`, `ExcavationJob`, `CheckExcavatedFootprint`, `CommitExcavationCut`, `GeothermalVentField`; `src/layer_portal_cmd.h`: `LayerPortalWorkState`, `GetLayerPortalWorkState`, `CheckLayerPortalWorkAccess`, `CommissionLayerPortalTBM`; `src/tbm.h`: `TBMProject`, `TBMWorkState`, `ValidateTBMAreaRoute`, `AdvanceTBMWork`; `src/tbm_exchange.h`: `TBMExchangeAssignment`, `ValidateTBMWagonExchange`, `ExecuteTBMWagonExchange`; `src/multilayer_industry.h`: `LayerIndustryCapability`, `ValidateMineDepositClaim`, `ProduceLayerMineCargo`, `PrepareLayerRecipeBatch`, `CommitLayerRecipeBatch`.
Verify each public file/export/schema field with `rg` in the current workspace and inspect its command/field ownership. For a same-plan contract, compile its type/signature fixture or validate its artifact after the preceding producer groups. For cross-plan behavior, also run the producer's focused executable boundary test; declarations alone cannot authorize dependent mutations or runtime claims.
If a required export is absent, stop the dependent implementation, mutation and runtime claims in Groups 1-5. Read-only characterization, source/range verification, test-case design and asset-template work that does not assume the missing behavior may continue.
Do not bypass this gate by copying private pool/tile state, inventing local IDs or cargo registries, using a historical plan as a delivered implementation, opening user-wide content stores, substituting a second utility/accounting authority, or adding an unreviewed fallback path. Finish the named producer or revise the shared contract and its direct consumers first.

**Scoped integration gates.** The following producer dependencies apply only to the named implementation/test families. Verify each required public export and its focused producer test before those integrations; absent late producers do not block already independent features.

- `src/transport_tube.h` is required for the actual tube command/terminal/fleet-control binding; basic menu registration and conventional tools may continue in Groups 3-4 (only the named integration/subscenario). If absent, stop only that integration/subscenario and its passing claim. Other independent code, art, persistence and tests in this area may continue.
- `src/geothermal.h` is required for the actual vent/utility/geothermal-command binding; basic menu registration and calendar labels may continue in Groups 3-4 (only the named integration/subscenario). If absent, stop only that integration/subscenario and its passing claim. Other independent code, art, persistence and tests in this area may continue.

Do not emulate an absent producer with copied private state, fake inventories/energy, local IDs or a second registry/grid. A final combined closure/gallery/round-trip still waits for every required family; an independent passing case is not a pass for the missing integration.

| File name | Line Ranges | Description |
| --- | --- | --- |
| [src/toolbar_gui.cpp](../../../../src/toolbar_gui.cpp) | 1048-1067 | Complete Airport button/menu handlers currently call ordinary airport construction; specialize these actions by clicked layer/profile without changing simulation through camera state. |
| [src/rail_gui.h](../../../../src/rail_gui.h) | 16-21 | Public rail toolbar/dropdown exports are the UI boundary for rail and specialized underground construction; commands remain the mutation authority. |
| [src/road_gui.h](../../../../src/road_gui.h) | 18-23 | Public road/scenario toolbar exports preserve truck/elevator controls while underground systems are introduced. |
| [src/map_layer_gui.h](../../../../src/map_layer_gui.h) | 75-81 | `SetViewportLayer` changes presentation only; a view switch cannot change the layer of an already issued command or simulation job. |
| `src/multilayer_content.h` (planned public contract; link after implementation) | Live definitions; no fixed range | [r2mlvs1-gameplay-contracts-and-content.md](r2mlvs1-gameplay-contracts-and-content.md) Groups 1-6, ending with Group 6's public-contract task, publish `MultiLayerGameplayProfile`, `LayerVehicleCapability`, `GetMultiLayerContentProfile`, `CheckDeepLayerAvailability`, `UtilityPowerAccount`. These exports/schema fields are authoritative for this area. Same-plan consumers follow those earlier groups; cross-plan consumers verify the producer's required implementation/tests as well. Proposed names alone do not prove delivery. |
| `src/excavation.h` (planned public contract; link after implementation) | Live definitions; no fixed range | [r2mlvs2-geology-and-excavation.md](r2mlvs2-geology-and-excavation.md) Groups 1-7, ending with Group 7's public-contract task, publish `ExcavationState`, `GeologyDeposit`, `ExcavationJob`, `CheckExcavatedFootprint`, `CommitExcavationCut`, `GeothermalVentField`. These exports/schema fields are authoritative for this area. Same-plan consumers follow those earlier groups; cross-plan consumers verify the producer's required implementation/tests as well. Proposed names alone do not prove delivery. |
| [src/layer_portal_cmd.h](../../../../src/layer_portal_cmd.h) | Live definitions; no fixed range | [r2mlvs3-sealed-portals-and-deep-rail.md](r2mlvs3-sealed-portals-and-deep-rail.md) Groups 1-7, ending with Group 7's public-contract task, publish `LayerPortalWorkState`, `GetLayerPortalWorkState`, `CheckLayerPortalWorkAccess`, `CommissionLayerPortalTBM`. These exports/schema fields are authoritative for this area. Same-plan consumers follow those earlier groups; cross-plan consumers verify the producer's required implementation/tests as well. Proposed names alone do not prove delivery. |
| `src/tbm.h` (planned public contract; link after implementation) | Live definitions; no fixed range | [r2mlvs4-tunnel-boring-machines.md](r2mlvs4-tunnel-boring-machines.md) Groups 1-5, ending with Group 5's public-contract task, publish `TBMProject`, `TBMWorkState`, `ValidateTBMAreaRoute`, `AdvanceTBMWork`. These exports/schema fields are authoritative for this area. Same-plan consumers follow those earlier groups; cross-plan consumers verify the producer's required implementation/tests as well. Proposed names alone do not prove delivery. |
| `src/tbm_exchange.h` (planned public contract; link after implementation) | Live definitions; no fixed range | [r2mlvs5-spoil-wagons-and-haulage.md](r2mlvs5-spoil-wagons-and-haulage.md) Groups 1-5, ending with Group 5's public-contract task, publish `TBMExchangeAssignment`, `ValidateTBMWagonExchange`, `ExecuteTBMWagonExchange`. These exports/schema fields are authoritative for this area. Same-plan consumers follow those earlier groups; cross-plan consumers verify the producer's required implementation/tests as well. Proposed names alone do not prove delivery. |
| `src/multilayer_industry.h` (planned public contract; link after implementation) | Live definitions; no fixed range | [r2mlvs6-mines-and-resource-chains.md](r2mlvs6-mines-and-resource-chains.md) Groups 1-6, ending with Group 6's public-contract task, publish `LayerIndustryCapability`, `ValidateMineDepositClaim`, `ProduceLayerMineCargo`, `PrepareLayerRecipeBatch`, `CommitLayerRecipeBatch`. These exports/schema fields are authoritative for this area. Same-plan consumers follow those earlier groups; cross-plan consumers verify the producer's required implementation/tests as well. Proposed names alone do not prove delivery. |
| `src/transport_tube.h` (planned public contract; link after implementation) | Live definitions; no fixed range | [r2mlvs16-underground-transport-tubes.md](r2mlvs16-underground-transport-tubes.md) Groups 1-8, ending with Group 8's public-contract task, publish `TubeVehicleCapability`, `TubeTerminal`, `CheckTubeConstruction`, `ComputeTubePowerDemand`. These exports/schema fields are authoritative for this area. Same-plan consumers follow those earlier groups; cross-plan consumers verify the producer's required implementation/tests as well. Proposed names alone do not prove delivery. Read this producer for the actual tube command/terminal/fleet-control binding; basic menu registration and conventional tools may continue in Groups 3-4 (only the named integration/subscenario); missing it blocks only that integration, not independent work. |
| `src/geothermal.h` (planned public contract; link after implementation) | Live definitions; no fixed range | [r2mlvs17-geothermal-energy-and-industry.md](r2mlvs17-geothermal-energy-and-industry.md) Groups 1-7, ending with Group 7's public-contract task, publish `GeothermalHeatAllocation`, `GeothermalUtilityLink`, `AllocateVentHeat`, `ExportGeothermalPower`. These exports/schema fields are authoritative for this area. Same-plan consumers follow those earlier groups; cross-plan consumers verify the producer's required implementation/tests as well. Proposed names alone do not prove delivery. Read this producer for the actual vent/utility/geothermal-command binding; basic menu registration and calendar labels may continue in Groups 3-4 (only the named integration/subscenario); missing it blocks only that integration, not independent work. |

**ASSUMPTION: this was already built previously. If not, re-asses at runtime**. This applies only to the producer contracts named in this area's gate, after their listed producer groups have run. Verify their actual exports and focused tests before using the sketches below; proposed types and helper names in sketches are not evidence of delivered code.

**UI boundary sketch.** The tool may use the active view to choose its initial menu, but a placement command must use the clicked world's tile and validated capabilities. Proposed helper names below illustrate the flow only.

```cpp
// Sketch inside a placement handler; tile is the OnPlaceObject world tile.
const MapLayer clicked_layer = GetTileLayer(tile);
if (!ToolSupportsLayer(selected_tool, clicked_layer)) return ShowPlacementError();
// Quote and send the owning feature command with tile/footprint and explicit IDs.
// SetViewportLayer is presentation; it must not rewrite a queued command.
```

The tube/geothermal providers are scoped future integrations. **ASSUMPTION: this was already built previously. If not, re-asses at runtime**. Verify r2mlvs16 `TubeVehicleCapability`/`TubeTerminal` and r2mlvs17 `GeothermalHeatAllocation`/`GeothermalUtilityLink` plus their command/tool exports before their corresponding tasks. Missing providers stop only those integrations; core surveys, mines, conventional fleet and toolbar tests continue. Do not create placeholder command success or copied capability registries.

### Group 1 - Underground construction interface

{{MEDIUM}}
- [X] 1.1 Replace the Airport slot/action on both rock layers with Underground Construction. Include surveys, excavation projects, mines, exchange bays, underground freight stations, road terminals and service facilities.
- [X] 1.2 Keep rail/road construction and new Deep rail controls consistent with excavated state. Explain why an unavailable facility needs excavation, a deposit, rail access or a model capability.

Evidence: `r2mlvs7-e-underground-window-build.log` and `r2mlvs7-f-underground-window-checks.log` record a successful Debug test build and 80 assertions across three focused cases. Surveys read the existing geological authority; portal projects, selected real exchange projects, native mine funding and conventional construction retain their command owners. Both rock planes use the canonical whole-footprint guards and Deep calendar gate. Existing rail artwork is a toolbar substitute, not completed R2MLVS13 asset acceptance.

### Group 2 - Underground construction interface

{{SIMPLE}}
- [ ] 2.1 Update split toolbars, hotkeys, editor controls and tooltips.

### Group 3 - Bind tools and previews to the clicked world

{{HARD}}
- [ ] 3.1 Capture tool context explicitly and derive final placement layer from the clicked world `TileIndex` in `OnPlaceObject`. If a main/extra view switches layers during a drag, cancel or revalidate the full footprint visibly; never silently redirect a command.
- [ ] 3.2 Show full-footprint excavation/material cost using command estimates and canonical footprint checks. A station preview must identify the blocked tile rather than auto-clear it.
- [ ] 3.3 Provide area-clearing selection using r2mlvs2/4 canonical passes: render legal 45-degree turn envelopes, extra charged space and final non-rail footprints without running work in the UI.
- [ ] 3.4 Test the same command from main and extra viewports, including a layer switch between selection and placement; both must mutate the intended world layer or refuse with no mutation.

### Group 4 - Connect later tube and geothermal tool providers

{{MEDIUM}}
- [ ] 4.1 After r2mlvs16 delivers its public capability/tools, add guideway, terminal, depot and power actions with a separate capsule fleet filter. Keep this integration pending if the provider is absent while the core underground toolbar remains testable.
- [ ] 4.2 After r2mlvs17 delivers its public tools, add Deep vent survey, utilities, generators and later plant actions. Read dates from the canonical profile rather than copy unlock constants into widgets.
- [ ] 4.3 Show 1980 Deep access and later model/factory dates consistently in toolbar, shortcut and preview paths; invoke the same validated commands regardless of entry path.

### Group 5 - Publish underground UI entry points

{{MEDIUM}}
- [ ] 5.1 Publish `src/underground_gui.h` exports `ShowUndergroundConstructionToolbar` and `BuildUndergroundVehicleFilter` with explicit layer/profile context; filters display capabilities but never authorize mutation.
- [ ] 5.2 Register new source/widgets and verify a direct caller can open the intended-layer toolbar and filter conventional underground freight separately from tubes. Update all handoffs if public names move.

### Area Mandatory Reading - Facilities and fleet

_Applies to Groups 6-8._

**Entry gate.** The following public contracts must exist before dependent implementation in this area: `src/underground_gui.h`: `ShowUndergroundConstructionToolbar`, `BuildUndergroundVehicleFilter`.
Verify each public file/export/schema field with `rg` in the current workspace and inspect its command/field ownership. For a same-plan contract, compile its type/signature fixture or validate its artifact after the preceding producer groups. For cross-plan behavior, also run the producer's focused executable boundary test; declarations alone cannot authorize dependent mutations or runtime claims.
If a required export is absent, stop the dependent implementation, mutation and runtime claims in Groups 6-8. Read-only characterization, source/range verification, test-case design and asset-template work that does not assume the missing behavior may continue.
Do not bypass this gate by copying private pool/tile state, inventing local IDs or cargo registries, using a historical plan as a delivered implementation, opening user-wide content stores, substituting a second utility/accounting authority, or adding an unreviewed fallback path. Finish the named producer or revise the shared contract and its direct consumers first.

**Scoped integration gates.** The following producer dependencies apply only to the named implementation/test families. Verify each required public export and its focused producer test before those integrations; absent late producers do not block already independent features.

- `src/transport_tube.h` is required for the tube capsule/terminal/service-facility interface integration in Groups 6-8 (only the named integration/subscenario). If absent, stop only that integration/subscenario and its passing claim. Other independent code, art, persistence and tests in this area may continue.

Do not emulate an absent producer with copied private state, fake inventories/energy, local IDs or a second registry/grid. A final combined closure/gallery/round-trip still waits for every required family; an independent passing case is not a pass for the missing integration.

| File name | Line Ranges | Description |
| --- | --- | --- |
| [src/station_cmd.h](../../../../src/station_cmd.h) | 29-47 | Station construction declarations/traits own IDs, joining and facility mutation; specialized launch and underground handling reuse this public command boundary. |
| [src/vehicle_cmd.h](../../../../src/vehicle_cmd.h) | 20-42 | Build/refit/clone/depot-autoreplace commands and traits expose all purchase paths; `CloneVehicle` is `NoTest`, so do not rely on a generic estimate to enforce capability/commissioning. |
| [src/timer/timer_game_calendar.h](../../../../src/timer/timer_game_calendar.h) | 15-39 | `TimerGameCalendar` technology-clock contract and date conversions govern 1980/model/factory introductions; economic/realtime scheduling cannot unlock a technology. |
| `src/underground_gui.h` (planned public contract; link after implementation) | Live definitions; no fixed range | [r2mlvs7-underground-construction-and-fleet.md](r2mlvs7-underground-construction-and-fleet.md) Groups 1-5, ending with Group 5's public-contract task, publish `ShowUndergroundConstructionToolbar`, `BuildUndergroundVehicleFilter`. These exports/schema fields are authoritative for this area. Same-plan consumers follow those earlier groups; cross-plan consumers verify the producer's required implementation/tests as well. Proposed names alone do not prove delivery. |
| `src/transport_tube.h` (planned public contract; link after implementation) | Live definitions; no fixed range | [r2mlvs16-underground-transport-tubes.md](r2mlvs16-underground-transport-tubes.md) Groups 1-8, ending with Group 8's public-contract task, publish `TubeVehicleCapability`, `TubeTerminal`, `CheckTubeConstruction`, `ComputeTubePowerDemand`. These exports/schema fields are authoritative for this area. Same-plan consumers follow those earlier groups; cross-plan consumers verify the producer's required implementation/tests as well. Proposed names alone do not prove delivery. Read this producer for the tube capsule/terminal/service-facility interface integration in Groups 6-8 (only the named integration/subscenario); missing it blocks only that integration, not independent work. |

**ASSUMPTION: this was already built previously. If not, re-asses at runtime**. This applies only to the producer contracts named in this area's gate, after their listed producer groups have run. Verify their actual exports and focused tests before using the sketches below; proposed types and helper names in sketches are not evidence of delivered code.

**Facility versus fleet checks.** Reuse the existing station and vehicle commands named in this reading area. Complete conventional station/depot and carrier fixtures first; the capsule-control group consumes the later r2mlvs16 provider and does not implement guideway physics or another power account.

### Group 6 - Add excavation-aware freight and service facilities

{{MEDIUM}}
- [ ] 6.1 Add underground rail loading stations and road mine terminals with normal StationIDs/orders and same-layer catchment. Validate full footprint and approaches before construction; no Surface passenger catchment is implied.
- [ ] 6.2 Add compatible train/road depots on excavated ground. Separate servicing from commissioning so a TBM can be repaired but cannot be purchased/cloned at an ordinary depot.
- [ ] 6.3 Verify joined-station and separate-terminal loading for one mine on each rock layer, preserving normal station-spread and company ownership rules.

### Group 7 - Define and integrate the conventional underground fleet

{{MEDIUM}}
- [ ] 7.1 Add mine trucks, underground freight locomotive/mineral wagons and deep-capable sealed carriers. Fill an explicit ten-resource/spoil-to-model/refit table with at least one valid transport option per cargo/date.
- [ ] 7.2 Apply identical capability checks in buy/refit/clone/autoreplace commands and UI. Verify running costs, service, group/list membership and layer/project labels for these ordinary manageable vehicles.
- [ ] 7.3 Test truck/articulated elevator admission and freight railtype power compatibility. Temporary work rail cannot power an incompatible electric vehicle just because its sprite resembles ordinary track.

### Group 8 - Integrate the five delivered tube models into fleet controls

{{MEDIUM}}
- [ ] 8.1 Consume r2mlvs16 engine identities for Mole-70, Crate-75, Cistern-80, Shield-85 and Forge-05; display their distinct dates, cargo roles and art rather than define another registry.
- [ ] 8.2 Connect tube-terminal purchase/refit/order/service controls and explain ordinary-rail/portal refusals. Verify each capsule stays in its single-layer network and transfers cargo to conventional freight for layer changes.

### Area Mandatory Reading - Art, workflows and acceptance

_Applies to Groups 9-12._

**Entry gate.** The following public contracts must exist before dependent implementation in this area: `src/underground_gui.h`: `ShowUndergroundConstructionToolbar`, `BuildUndergroundVehicleFilter`.
Verify each public file/export/schema field with `rg` in the current workspace and inspect its command/field ownership. For a same-plan contract, compile its type/signature fixture or validate its artifact after the preceding producer groups. For cross-plan behavior, also run the producer's focused executable boundary test; declarations alone cannot authorize dependent mutations or runtime claims.
If a required export is absent, stop the dependent implementation, mutation and runtime claims in Groups 9-12. Read-only characterization, source/range verification, test-case design and asset-template work that does not assume the missing behavior may continue.
Do not bypass this gate by copying private pool/tile state, inventing local IDs or cargo registries, using a historical plan as a delivered implementation, opening user-wide content stores, substituting a second utility/accounting authority, or adding an unreviewed fallback path. Finish the named producer or revise the shared contract and its direct consumers first.

**Scoped integration gates.** The following producer dependencies apply only to the named implementation/test families. Verify each required public export and its focused producer test before those integrations; absent late producers do not block already independent features.

- `src/transport_tube.h` is required for the tube model/routes/date controls subscenario in Group 11 (only the named integration/subscenario). If absent, stop only that integration/subscenario and its passing claim. Other independent code, art, persistence and tests in this area may continue.
- `src/geothermal.h` is required for the geothermal plant-control/timeline subscenario in Group 11 (only the named integration/subscenario). If absent, stop only that integration/subscenario and its passing claim. Other independent code, art, persistence and tests in this area may continue.

Do not emulate an absent producer with copied private state, fake inventories/energy, local IDs or a second registry/grid. A final combined closure/gallery/round-trip still waits for every required family; an independent passing case is not a pass for the missing integration.

| File name | Line Ranges | Description |
| --- | --- | --- |
| [src/tests/map_layer.cpp](../../../../src/tests/map_layer.cpp) | 269-291 | Complete Deep road-only rule test is a legacy baseline; add profile-selective expectations rather than erasing Classic behavior. |
| [src/tests/map_layer.cpp](../../../../src/tests/map_layer.cpp) | 374-427 | Complete permitted/refused portal matrix tests currently restrict rail to Surface/Underground; add explicit new-profile and calendar cases. |
| [src/tests/map_layer.cpp](../../../../src/tests/map_layer.cpp) | 430-442 | Complete display-order test retains immutable Space=2 and Deep=3 storage meanings; future UI/profile work cannot reorder saved IDs. |
| `src/underground_gui.h` (planned public contract; link after implementation) | Live definitions; no fixed range | [r2mlvs7-underground-construction-and-fleet.md](r2mlvs7-underground-construction-and-fleet.md) Groups 1-5, ending with Group 5's public-contract task, publish `ShowUndergroundConstructionToolbar`, `BuildUndergroundVehicleFilter`. These exports/schema fields are authoritative for this area. Same-plan consumers follow those earlier groups; cross-plan consumers verify the producer's required implementation/tests as well. Proposed names alone do not prove delivery. |
| `src/transport_tube.h` (planned public contract; link after implementation) | Live definitions; no fixed range | [r2mlvs16-underground-transport-tubes.md](r2mlvs16-underground-transport-tubes.md) Groups 1-8, ending with Group 8's public-contract task, publish `TubeVehicleCapability`, `TubeTerminal`, `CheckTubeConstruction`, `ComputeTubePowerDemand`. These exports/schema fields are authoritative for this area. Same-plan consumers follow those earlier groups; cross-plan consumers verify the producer's required implementation/tests as well. Proposed names alone do not prove delivery. Read this producer for the tube model/routes/date controls subscenario in Group 11 (only the named integration/subscenario); missing it blocks only that integration, not independent work. |
| `src/geothermal.h` (planned public contract; link after implementation) | Live definitions; no fixed range | [r2mlvs17-geothermal-energy-and-industry.md](r2mlvs17-geothermal-energy-and-industry.md) Groups 1-7, ending with Group 7's public-contract task, publish `GeothermalHeatAllocation`, `GeothermalUtilityLink`, `AllocateVentHeat`, `ExportGeothermalPower`. These exports/schema fields are authoritative for this area. Same-plan consumers follow those earlier groups; cross-plan consumers verify the producer's required implementation/tests as well. Proposed names alone do not prove delivery. Read this producer for the geothermal plant-control/timeline subscenario in Group 11 (only the named integration/subscenario); missing it blocks only that integration, not independent work. |
| [Sprite Contract and delivery owner](r2mlvs13-graphics-and-asset-pipeline.md#public-handoff-contract) | Live definitions; no fixed range | `Sprite Contract Handoff` defines manifest fields `asset_id`, `consumer`, `directions`, `frames`, `anchor`, `source`; the matching authoring/conversion/runtime groups deliver actual assets. Verify the requested family and packed loading fixture before graphics integration; template/manifest publication alone is not final art. |

**ASSUMPTION: this was already built previously. If not, re-asses at runtime**. This applies only to the producer contracts named in this area's gate, after their listed producer groups have run. Verify their actual exports and focused tests before using the sketches below; proposed types and helper names in sketches are not evidence of delivered code.

**Final-art gate. ASSUMPTION: this was already built previously. If not, re-asses at runtime**. Toolbar/cursor, station/depot and directional/load graphics integration consumes r2mlvs13's delivered manifest entries and packed asset fixture. Verify them before those tasks; if absent, continue command/workflow tests but keep final visual acceptance pending. The manifest alone does not prove all assets load.

**Scoped graphics gate. ASSUMPTION: this was already built previously. If not, re-asses at runtime**. Final graphics tasks in this area require the corresponding r2mlvs13 manifest entries, delivered sprites and packed loading fixture. Verify them before binding/review; if absent, stop only those art tasks and their acceptance claims, continue independent code/state tests, and never count placeholders as finished assets.

### Group 9 - Art, workflows and acceptance

{{MEDIUM}}
- [ ] 9.1 Integrate final toolbar/cursor, station, depot, vehicle direction and cargo-load sprites from r2mlvs13.
- [ ] 9.2 Verify vehicle occlusion at loading docks, bays and deep portals.
- [ ] 9.3 Complete a player workflow using only visible controls: survey -> commission/dig -> exchange -> build mine/station -> buy/refit carrier -> create orders -> deliver to Surface.
- [ ] 9.4 Repeat on both rock layers and through main/extra viewports, split toolbar sizes, keyboard controls and editor.
- [ ] 9.5 Confirm ordinary airports cannot be constructed below Surface through commands either.

### Group 10 - Verify command parity and fleet persistence

{{HARD}}
- [ ] 10.1 Send malformed commands directly for station footprint/layer and specialist purchase/refit/clone paths; compare refusal with toolbar behavior and confirm no partial allocation.
- [ ] 10.2 Test same-XY stations across layers and a cargo with no compatible model; the UI must expose the same restriction the command enforces without creating catchment transfer.
- [ ] 10.3 Save/reload a conventional underground fleet with station/service orders and an active project association. After tube delivery, repeat for capsule orders/refits and compare engine IDs, cargo and filters.

### Group 11 - Art, workflows and acceptance

{{MEDIUM}}
- [ ] 11.1 Exercise pre/post-1980 Deep controls, tube model introduction dates, geothermal plant timelines and area-clearing tool previews.
- [ ] 11.2 Verify five distinct tube vehicles appear with their final art and route cargo through layer-local terminals to conventional interchanges.

### Group 12 - Art, workflows and acceptance

{{SIMPLE}}
- [ ] 12.1 Record the implemented public exports, focused test commands/results and remaining runtime/art/network gaps in this plan; retain original supplied saves and repository-local evidence.
- [ ] 12.2 Update affected architecture/handoff documentation to describe delivered behavior and reconcile the parent index only when this plan's Completion Conditions are met. Keep every unfinished task unchecked.

## Consult on Demand

| Trigger | File name | Start at | Expand only when | Consultation purpose |
| --- | --- | --- | --- | --- |
| A placement from an extra viewport targets the wrong layer | [../../architecture/gui/archi-viewport.md](../../architecture/gui/archi-viewport.md) | `Clicks and placement` | The current OnPlaceObject world tile differs from the issued command tile | Trace picking/placement ownership and its live source call sites; selected main-view state cannot override the clicked world. |
| A required public export is absent and its owner/location is disputed | [r2mlvs-multilayer-vehicles-and-structures.md](r2mlvs-multilayer-vehicles-and-structures.md) | `Requirements and ownership` and `Plan Index` | The named producer confirms an intentional contract/ownership change | Find the responsible producer and reconcile its public handoff with direct consumers; the hub cannot substitute for missing implementation. |
| Current behavior contradicts a historical subsystem description | [../../architecture/map-layers/archi-map-layers.md](../../architecture/map-layers/archi-map-layers.md) | `Invariants` and the relevant named source in `Code map` | The live symbol and executable test do not explain the difference | Locate layer addressing or lifecycle rationale. Live code/tests govern delivered behavior; this architecture page does not prove future interfaces exist. |

## Planning Provenance

This is not an execution reading queue. Do not preload it after context compaction. It is non-transitive; ranges record planning use, not runtime authority.

| File name | Sections or Line Ranges used while planning | Planning contribution |
| --- | --- | --- |
| [docs/zz-reach2/architecture/initial-inventory.md](../../architecture/initial-inventory.md) | 185-220 | `Command as the only world mutation`, `Pool-indexed game objects`, `Tile map as a packed array` and `File-suffix modules` oriented command/pool/header ownership and source registration. This historical inventory is a location survey, not authority for current layer behavior. |
| [docs/zz-reach2/architecture/@archi-ttd-level0.md](../../architecture/@archi-ttd-level0.md) | 24-63 | `How the program fits together` and `Source and runtime boundaries` located shared simulation, data/presentation, CMake targets and subsystem handoffs used to choose the live reading sources. |
| [docs/zz-reach2/upgrades/2026-10/r2mlvs-multilayer-vehicles-and-structures.md](r2mlvs-multilayer-vehicles-and-structures.md) | 119-140 | `Requirements and ownership` assigned this plan's feature responsibilities; it shaped the decomposition without making every sibling plan a mandatory reading dependency. |
| [src/window_gui.h](../../../../src/window_gui.h) | 799-805 | The inspected live boundary helped separate characterization, public-contract production, implementation and acceptance batches. It materially constrained clicked-layer tools, usable fleet filters and command-enforced construction capabilities; this entry records planning influence rather than an additional reading obligation. |
| [User planning-format instructions](<C:/Users/Axonn/.codex/attachments/cd59b20a-3a96-4d54-b4bd-1e8f37639e7d/Pasted text.txt>) | `Global Mandatory Reading`, `Area Mandatory Reading`, `Consult on Demand`, `Planning Provenance`, `Final validation` | Supplies the exact protocol, chronological difficulty batches of at most eight tasks, reading gates and validation format. |
| [src/window_gui.h](../../../../src/window_gui.h) | 799-805 | Deeper planning pass, 2026-10-04: Clicked world tiles shaped tool-context validation independently of viewport selection. |

## Completion Conditions

The underground controls make the excavation/mining workflow discoverable, with no unusable Airport menu left behind. All required transport and service facilities have working graphics and enforce the same layer rules through UI, commands and scripts.
