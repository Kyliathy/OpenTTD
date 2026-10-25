# R2MLVS7 - Underground Construction and Fleet

**Merged implementation,2026-10-25.** [M2O23qualification](m2o23-4-come-space-b-integration-handoff.md) integrates existing native fleet/provider roles, dated model admission, actual source/path loss and saved/manual-stop recovery. No second fleet registry or constructed substitute cargo owner is introduced. The dated feature checklist keeps its own later scope. [Storage authority](../../architecture/archi-historical-debug.md).

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

Status: in progress; independent construction, conventional native fleet, command isolation and native save/reload are delivered through Group 10B, with native calendar/preview coverage in Group 11. The complete visible Surface-delivery workflow passes on both rock layers through main/extra viewports, full/split toolbar and keyboard/menu entry, with separate editor authoring and normal freight continuation. Tube/geothermal integration and final graphics remain open. Parent: [R2MLVS master](r2mlvs-multilayer-vehicles-and-structures.md). Depends on r2mlvs3/5/6. Execution evidence: [underground interface ledger](../../../zz-rhr/upgrades/2026-10/r2mlvs7-a-underground-interface.md).

**Artifact-layout migration (2026-10-09):** this plan consumes the parent [Development artifact layout](r2mlvs-multilayer-vehicles-and-structures.md#development-artifact-layout), implemented by [R2FC](r2fc-file-cleanup.md). It supersedes earlier docs asset/review/helper placement and fixed runtime-config destinations below. New native homes use `mlm/runtime/`, generated feature reviews use `mlm/reviews/`, logs remain under `logs/`, and reusable code/curated inputs keep their code/test owners. Paths and focused native/cold verification are complete; R2FC records the results and retained-input identities. Repository-relative executable commands run from the checkout root through their context-aware wrappers. Historical pass records remain dated evidence and are not new acceptance claims. No gameplay task is completed by this migration and the separate post-demo fixture-pruning gate remains.

## Outcome and Boundaries

Replace underground airport controls with a useful construction system and deliver the stations, depots and vehicles needed for mining. Integrate Transport Tubes and geothermal controls from r2mlvs17/18; those plans own the five original tube models and thermal simulation.

## Public Handoff Contract

2026-10-09 gameplay feedback: actual native mouse dispatch found that momentary image/text placement buttons cancel themselves before arming their cursor. Prior direct `OnClick`/rendering checks bypassed this interaction. [TBM fixing Groups2-3](r2tbmf-tbm-fixes.md#group-2---make-the-interface-compact-and-discoverable) own the corrected compact palette, real project list and working area input; [native reproduction](../../../zz-rhr/upgrades/2026-10/tbm-fixes/r2tbmf-b-reproduction-checkpoint.md) preserves the finding. Existing pure command/geometry exports below remain authoritative, but their tests alone do not prove a usable mouse workflow.

2026-10-09 fixing handoff: persistent placement controls survive actual mouse dispatch; [interface checkpoint](../../../zz-rhr/upgrades/2026-10/tbm-fixes/r2tbmf-c-interface-checkpoint.md) records the compact palette, native TBM toolbar variants and working project browser. [`src/tbm_gui.h`](../../../../src/tbm_gui.h) exports `ShowTBMProjectsWindow(MapLayer)` and `InspectUndergroundProject(TBMProjectID)` as read-only presentation/navigation owners. Projects opens actual jobs. New area redirects assigned portals to Projects, whose Pause and extend flow quotes and appends work to the same machine through the synchronized command. Straight-tunnel and area intent show canonical charged geometry; both-layer native acceptance is recorded in [F](../../../zz-rhr/upgrades/2026-10/tbm-fixes/r2tbmf-f-intent-checkpoint.md) and [G](../../../zz-rhr/upgrades/2026-10/tbm-fixes/r2tbmf-g-fresh-checkpoint.md). Return/park, compatible hangar selection and final release reuse vehicle/order/job owners. [ZB](../../../zz-rhr/upgrades/2026-10/tbm-fixes/r2tbmf-zb-final-notes.md) connects actual project input to fifty cuts, exit B and public traffic. No command, reservation, cash, terrain or persistent vehicle owner moved into the browser.

Delivered public owner: [`src/underground_gui.h`](../../../../src/underground_gui.h). `ShowUndergroundConstructionToolbar(MapLayer, const MultiLayerProfileSelection &)` opens the requested rock-plane interface. `BuildUndergroundVehicleFilter(MapLayer, const MultiLayerProfileSelection &, TimerGameCalendar::Date, UndergroundFleetKind)` returns immutable profile-owned logical design pointers for conventional freight or separate tube presentation. It allocates no engines and grants no purchase, refit, portal or energy permission. Native feature commands and their existing pool/stock owners remain authoritative; actual tube/geothermal integration is still gated.

Delivered native fleet owner: [`src/underground_vehicle.h`](../../../../src/underground_vehicle.h). `InitialiseUndergroundEngineDefinitions(const GRFConfigList &)` reconstructs the accepted pinned-package definitions in the existing engine/EIDS owner. `GetUndergroundEngine(uint16_t)` resolves models 2-8 to actual persisted identities; `GetUndergroundEngineCapability(EngineID)` returns the corresponding immutable native capability. `CheckUndergroundVehicleCapability(EngineID, MapLayer, CargoType)` supplies the shared profile/date/layer/exact-cargo check to actual buy/refit/clone/renew commands and their UI filters. `MapLayer::Invalid` requests a global presentation check; `INVALID_CARGO` omits a proposed cargo check. Ordinary engines retain their existing policy. Hoppers 2/3 reuse R4's definitions; freight models 4-8 append into its pinned EIDS range. Tube catalogue rows have no native ordinary-engine registration.

Delivered read-only area owner: [`src/tbm_area_preview.h`](../../../../src/tbm_area_preview.h), `PreviewTBMCommissionArea(...)` and `TBMCommissionAreaPreview`, quote canonical floor, swept working space, uncovered cells, whole-consist purchase and later Earth/Rock/support/work charges without allocating a project. Confirmation goes through the existing commissioning command and revalidates the live actor and world. [`src/construction_preview.h`](../../../../src/construction_preview.h) owns cursor-window overlays only; they are cleared with placement cancellation and never saved. [`src/map_layer_gui.h`](../../../../src/map_layer_gui.h) publishes `GetLayerPortalDefaultTBMModel(TileIndex)` / `GetLayerPortalDefaultTBMOrigin(TileIndex, CompanyID)` so portal and area controls use the same real catalogue and owned Surface origin selector; commissioning still checks disposal acceptance.

Delivered diagnostic: `dump_info underground_fleet` in [`src/console_cmds.cpp`](../../../../src/console_cmds.cpp) reads actual EIDS mappings, native vehicle cargo/orders, TBM associations and the two logical fleet filters after normal loading. It grants no construction or vehicle permission. Persistence remains with existing EIDS, vehicle/order and project/exchange chunks; no second fleet registry or saved preview state was introduced. Focused commands, result counts and immutable round-trip artifacts are recorded below and in the execution ledger.

Delivered commissioning defaults: [`src/tbm.h`](../../../../src/tbm.h), `GetDefaultTBMConsist(MapLayer)`, supplies the actual working layer's native separate Earth/Rock groups to both portal and area controls. Underground retains two/two; Deep uses two/four to hold the first complete Hard starter cut. Actual sizing, estimate/purchase and exchange commands retain all capacity, whole-consist length, actor and site checks.

## Implementation Tasks

Deepening pass, 2026-10-04: implementation groups use only SIMPLE, MEDIUM or HARD difficulty, with at most eight pending tasks each. Sketches describe proposed implementation contracts; verify the named earlier producers at each assumption gate. No gameplay or asset implementation is marked complete by this planning pass.

Groups are chronological and contain consecutive tasks of one difficulty. Each difficulty batch has at most eight tasks; area reading changes only when the implementation boundary or source of truth changes.

### Area Mandatory Reading - Underground construction interface

_Applies to Groups 1-5, including 3B/3C/3D._

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
| [`src/multilayer_content.h`](../../../../src/multilayer_content.h) (delivered public owner) | Live definitions; no fixed range | [r2mlvs1-gameplay-contracts-and-content.md](r2mlvs1-gameplay-contracts-and-content.md) Groups 1-6, ending with Group 6's public-contract task, publish `MultiLayerGameplayProfile`, `LayerVehicleCapability`, `GetMultiLayerContentProfile`, `CheckDeepLayerAvailability`, `UtilityPowerAccount`. These exports/schema fields are authoritative for this area. Same-plan consumers follow those earlier groups; cross-plan consumers verify the producer's required implementation/tests as well. Proposed names alone do not prove delivery. |
| [`src/excavation.h`](../../../../src/excavation.h) (delivered public owner) | Live definitions; no fixed range | [r2mlvs2-geology-and-excavation.md](r2mlvs2-geology-and-excavation.md) Groups 1-7, ending with Group 7's public-contract task, publish `ExcavationState`, `GeologyDeposit`, `ExcavationJob`, `CheckExcavatedFootprint`, `CommitExcavationCut`, `GeothermalVentField`. These exports/schema fields are authoritative for this area. Same-plan consumers follow those earlier groups; cross-plan consumers verify the producer's required implementation/tests as well. Proposed names alone do not prove delivery. |
| [src/layer_portal_cmd.h](../../../../src/layer_portal_cmd.h) | Live definitions; no fixed range | [r2mlvs3-sealed-portals-and-deep-rail.md](r2mlvs3-sealed-portals-and-deep-rail.md) Groups 1-7, ending with Group 7's public-contract task, publish `LayerPortalWorkState`, `GetLayerPortalWorkState`, `CheckLayerPortalWorkAccess`, `CommissionLayerPortalTBM`. These exports/schema fields are authoritative for this area. Same-plan consumers follow those earlier groups; cross-plan consumers verify the producer's required implementation/tests as well. Proposed names alone do not prove delivery. |
| [`src/tbm.h`](../../../../src/tbm.h) (delivered public owner) | Live definitions; no fixed range | [r2mlvs4-tunnel-boring-machines.md](r2mlvs4-tunnel-boring-machines.md) Groups 1-5, ending with Group 5's public-contract task, publish `TBMProject`, `TBMWorkState`, `ValidateTBMAreaRoute`, `AdvanceTBMWork`. These exports/schema fields are authoritative for this area. Same-plan consumers follow those earlier groups; cross-plan consumers verify the producer's required implementation/tests as well. Proposed names alone do not prove delivery. |
| [`src/tbm_exchange.h`](../../../../src/tbm_exchange.h) (delivered public owner) | Live definitions; no fixed range | [r2mlvs5-spoil-wagons-and-haulage.md](r2mlvs5-spoil-wagons-and-haulage.md) Groups 1-5, ending with Group 5's public-contract task, publish `TBMExchangeAssignment`, `ValidateTBMWagonExchange`, `ExecuteTBMWagonExchange`. These exports/schema fields are authoritative for this area. Same-plan consumers follow those earlier groups; cross-plan consumers verify the producer's required implementation/tests as well. Proposed names alone do not prove delivery. |
| [`src/multilayer_industry.h`](../../../../src/multilayer_industry.h) (delivered public owner) | Live definitions; no fixed range | [r2mlvs6-mines-and-resource-chains.md](r2mlvs6-mines-and-resource-chains.md) Groups 1-6, ending with Group 6's public-contract task, publish `LayerIndustryCapability`, `ValidateMineDepositClaim`, `ProduceLayerMineCargo`, `PrepareLayerRecipeBatch`, `CommitLayerRecipeBatch`. These exports/schema fields are authoritative for this area. Same-plan consumers follow those earlier groups; cross-plan consumers verify the producer's required implementation/tests as well. Proposed names alone do not prove delivery. |
| `src/transport_tube.h` (planned public contract; link after implementation) | Live definitions; no fixed range | [r2mlvs17-underground-transport-tubes.md](r2mlvs17-underground-transport-tubes.md) Groups 1-8, ending with Group 8's public-contract task, publish `TubeVehicleCapability`, `TubeTerminal`, `CheckTubeConstruction`, `ComputeTubePowerDemand`. These exports/schema fields are authoritative for this area. Same-plan consumers follow those earlier groups; cross-plan consumers verify the producer's required implementation/tests as well. Proposed names alone do not prove delivery. Read this producer for the actual tube command/terminal/fleet-control binding; basic menu registration and conventional tools may continue in Groups 3-4 (only the named integration/subscenario); missing it blocks only that integration, not independent work. |
| `src/geothermal.h` (planned public contract; link after implementation) | Live definitions; no fixed range | [r2mlvs18-geothermal-energy-and-industry.md](r2mlvs18-geothermal-energy-and-industry.md) Groups 1-7, ending with Group 7's public-contract task, publish `GeothermalHeatAllocation`, `GeothermalUtilityLink`, `AllocateVentHeat`, `ExportGeothermalPower`. These exports/schema fields are authoritative for this area. Same-plan consumers follow those earlier groups; cross-plan consumers verify the producer's required implementation/tests as well. Proposed names alone do not prove delivery. Read this producer for the actual vent/utility/geothermal-command binding; basic menu registration and calendar labels may continue in Groups 3-4 (only the named integration/subscenario); missing it blocks only that integration, not independent work. |

**ASSUMPTION: this was already built previously. If not, re-asses at runtime**. This applies only to the producer contracts named in this area's gate, after their listed producer groups have run. Verify their actual exports and focused tests before using the sketches below; proposed types and helper names in sketches are not evidence of delivered code.

**UI boundary sketch.** The tool may use the active view to choose its initial menu, but a placement command must use the clicked world's tile and validated capabilities. Proposed helper names below illustrate the flow only.

```cpp
// Sketch inside a placement handler; tile is the OnPlaceObject world tile.
const MapLayer clicked_layer = GetTileLayer(tile);
if (!ToolSupportsLayer(selected_tool, clicked_layer)) return ShowPlacementError();
// Quote and send the owning feature command with tile/footprint and explicit IDs.
// SetViewportLayer is presentation; it must not rewrite a queued command.
```

The tube/geothermal providers are scoped future integrations. **ASSUMPTION: this was already built previously. If not, re-asses at runtime**. Verify r2mlvs17 `TubeVehicleCapability`/`TubeTerminal` and r2mlvs18 `GeothermalHeatAllocation`/`GeothermalUtilityLink` plus their command/tool exports before their corresponding tasks. Missing providers stop only those integrations; core surveys, mines, conventional fleet and toolbar tests continue. Do not create placeholder command success or copied capability registries.

### Group 1 - Underground construction interface

{{MEDIUM}}
- [X] 1.1 Replace the Airport slot/action on both rock layers with Underground Construction. Include surveys, excavation projects, mines, exchange bays, underground freight stations, road terminals and service facilities.
- [X] 1.2 Keep rail/road construction and new Deep rail controls consistent with excavated state. Explain why an unavailable facility needs excavation, a deposit, rail access or a model capability.

Evidence: `r2mlvs7-e-underground-window-build.log` and `r2mlvs7-f-underground-window-checks.log` record a successful Debug test build and 80 assertions across three focused cases. Surveys read the existing geological authority; portal projects, selected real exchange projects, native mine funding and conventional construction retain their command owners. Both rock planes use the canonical whole-footprint guards and Deep calendar gate. Existing rail artwork is a toolbar substitute, not completed R2MLVS13 asset acceptance.

### Group 2 - Underground construction interface

{{SIMPLE}}
- [X] 2.1 Update split toolbars, hotkeys, editor controls and tooltips.

Evidence: `[underground-toolbar]` passed 23 assertions in one actual toolbar case (`r2mlvs7-m-minimum-toolbar-checks.log`, build `r2mlvs7-l-minimum-toolbar-build.log`). It covers both rock-plane hotkeys, an earlier dropdown after the view changes, a real minimum-width split-row switch, Surface tooltip restoration and the editor industry-slot entry. Editor roads retain their own scenario toolbar; company-only purchase/service controls do not gain an editor bypass.

### Group 3 - Bind tools and previews to the clicked world

{{HARD}}
- [X] 3.1 Capture tool context explicitly and derive final placement layer from the clicked world `TileIndex` in `OnPlaceObject`. If a main/extra view switches layers during a drag, cancel or revalidate the full footprint visibly; never silently redirect a command.

Evidence: `[underground-placement]` passed 22 assertions in one real main/extra viewport case (`r2mlvs7-o-placement-context-checks.log`, build n). Both layer handlers cancel a live sizing drag, release the cursor and raise the selected tool without changing geology. Posted commands keep their captured world tiles. Underground survey/project/exchange clicks retain their explicit menu layer; conventional constructors retain world-tile/full-footprint command validation.

### Local Mandatory Reading - Whole-footprint preview

_Applies to Group 3B; retains the preceding area's entry and scoped producer gates._

Read `CheckExcavatedFootprint`, `CheckExcavationConstructionArea` and the exact station/depot command geometry being quoted. Read command estimate/result presentation in `CommandCost` and `CommandHelperBase::InternalPostResult`. For geological material/work quotes, read `PlanTBMCommission`, `CommissionTBMProject` and the canonical `TBMAreaPreview`; the delivered public entry is `src/tbm_area_preview.h`. UI quotes must share the owning command's complete footprint and return the first blocked world tile, without clearing or reserving it.

### Group 3B - Quote construction footprints

{{HARD}}
- [X] 3.2A Keep facility estimates on the owning command's full footprint and identify the first blocked world cell instead of auto-clearing it. Publish the TBM owner's exact purchase/material/work quote before binding its area-selection UI.

Evidence: `[underground-footprint-quote],[underground-area-quote],[tbm-commission]` passed 704 assertions in six cases (`r2mlvs7-q-footprint-quote-checks.log`, build p). Both rock-layer station estimates name the blocked cell, preserve state, and charge every platform cell after actual excavation. `PreviewTBMCommissionArea` shares the command's canonical geometry, purchase preflight and future paid-cut totals; no second planner, reserve or work controller exists. Existing Shift placement estimates use the same command result presentation. The selected-area display remains task 3.2B in the following UI group.

### Local Mandatory Reading - Area selection and viewport acceptance

_Applies to Group 3C; retains the preceding area's entry and scoped producer gates._

Read `TBMAreaRequest`, `TBMAreaPreview`, `PreviewTBMArea`, `ValidateTBMAreaRoute`, delivered `PreviewTBMCommissionArea` in `src/tbm_area_preview.h`, the rectangle commissioning preparation in `src/tbm_commission.cpp`, and actual placement/drag callbacks in `Window`. Consume canonical floor, route, turn envelope and charges; never execute geological work from a preview. Use actual main/extra viewports and native commands for acceptance. For the derived overlay, read `GetTileHighlightType`, `DrawTileSelection` and `SetObjectToPlace` in `src/viewport.cpp`; preserve their existing cursor ownership and reset boundary.

### Group 3C - Select canonical excavation areas

{{HARD}}
- [X] 3.2B Show the selected canonical area's complete Earth/Rock, support, work and purchase costs in the UI, retaining the separate charging owners and refusing unavailable/partial plans visibly.
- [X] 3.3 Provide area-clearing selection using r2mlvs2/4 canonical passes: render legal 45-degree turn envelopes, extra charged space and final non-rail footprints without running work in the UI.

Evidence: actual widgets and cursor-owned canonical overlays passed 253 assertions in two cases (`r2mlvs7-u-area-turn-checks.log`, build t); the initial area/placement/portal burst passed 112 assertions in three cases (s, build r). The UI selects a real portal, quotes a bounded final-use rectangle, shows purchase separately from future paid work, and uses white/blue/red native highlights for covered floor/extra swept space/uncovered cells. Tests observed canonical advancing 45-degree bends with unchanged geology, cash, RNG, vehicles and project state. These are functional geometry/control checks, not final raster-art acceptance.

### Local Mandatory Reading - Actual viewport command acceptance

_Applies to Group 3D; retains the Underground construction area's producer gates._

Read `Window::OnPlaceObject` / `OnPlaceMouseUp`, `TranslateXYToTileCoord`, `HandleViewportClicked`, `ScrollWindowTo`, vehicle-follow layer handling and `SetViewportLayer`. Read the actual area confirmation command and the delivered quote/overlay reset boundary. Use the shared real commissioning fixture and real main/extra viewport owners; successful purchase is the native command, while an unfinished layer-switched selection must preserve the entire native digest.

### Group 3D - Verify actual viewport command placement

{{HARD}}
- [X] 3.4 Test the same command from main and extra viewports, including a layer switch between selection and placement; both must mutate the intended world layer or refuse with no mutation.

Evidence: actual viewport projection, mouse-event dispatch and native commissioning purchase passed 210 assertions in one three-scenario case (`r2mlvs7-af-clamped-viewport-checks.log`, build ae). Main and extra views purchase against the same clicked Underground portal/floor, with Surface still displayed in the main view for the extra case. A tile-centred layer switch before confirmation preserves the native digest and refuses stale release/confirm events. Direct centring and vehicle following now cancel unfinished selection as well. Initialization now derives virtual dimensions from the clamped viewport zoom, fixing picking before the first resize. The fixture publishes native scroll coordinates without invoking raster repaint; full runtime art remains a separate acceptance scope.

### Group 4 - Connect later tube and geothermal tool providers

Goal-authorized skip (2026-10-08, master finalization): dispatched expansion work in this group is skipped under [finalization exception GP-TG](../../../zz-rhr/upgrades/2026-10/portal-tbm/r2mlvs3mp-z-a-finalization-ledger.md#gp-tg-tube-and-geothermal-expansion-requires-major-gap-planning). Existing delivered prefixes stay complete; unimplemented profile work remains pending. No skipped task is markedX.

{{MEDIUM}}
- [ ] 4.1 After r2mlvs17 delivers its public capability/tools, add guideway, terminal, depot and power actions with a separate capsule fleet filter. Keep this integration pending if the provider is absent while the core underground toolbar remains testable.
- [ ] 4.2 After r2mlvs18 delivers its public tools, add Deep vent survey, utilities, generators and later plant actions. Read dates from the canonical profile rather than copy unlock constants into widgets.
- [X] 4.3A Show canonical Deep access and delivered cutter model dates consistently in toolbar, shortcut and preview paths; invoke the same validated commands regardless of entry path.
- [ ] 4.3B Integrate actual tube/geothermal/factory calendar labels and command paths after their scoped producers publish the required behavior.

Scoped gate: `src/transport_tube.h` and `src/geothermal.h` are absent. Tasks 4.1/4.2/4.3B stay pending; no tube network, factory/utility action or inventory is emulated. Independent native cutter dates are read directly from the profile-owned catalogue.

Independent date evidence: `[underground-window],[underground-toolbar],[underground-area-quote]` passed 132 assertions in four cases (`r2mlvs7-ah-calendar-controls-checks.log`, build ag). The toolbar displays compatible Standard/Deep cutter introduction years from the same canonical catalogue used by its validated commands, and the Deep unlock comes from the profile. Actual later-provider actions remain unchecked.

### Group 5 - Publish underground UI entry points

{{MEDIUM}}
- [X] 5.1 Publish `src/underground_gui.h` exports `ShowUndergroundConstructionToolbar` and `BuildUndergroundVehicleFilter` with explicit layer/profile context; filters display capabilities but never authorize mutation.
- [X] 5.2 Register new source/widgets and verify a direct caller can open the intended-layer toolbar and filter conventional underground freight separately from tubes. Update all handoffs if public names move.

Evidence: the exact public filter signature and direct real-window callers passed 106 assertions in three cases (`r2mlvs7-al-public-filter-checks.log`, build ak). The catalogue's focused producer check passed 181 assertions (ai). Filter results preserve original immutable model identity, commissioning/service/cargo domains and canonical calendar gates. Logical tube rows create no runtime engines or network; actual controls remain pending under Groups 4/8. Native source/widgets and focused fixture registrations are delivered in CMake.

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
| [`src/underground_gui.h`](../../../../src/underground_gui.h) (delivered public owner) | Live definitions; no fixed range | [r2mlvs7-underground-construction-and-fleet.md](r2mlvs7-underground-construction-and-fleet.md) Groups 1-5, ending with Group 5's public-contract task, publish `ShowUndergroundConstructionToolbar`, `BuildUndergroundVehicleFilter`. These exports/schema fields are authoritative for this area. Same-plan consumers follow those earlier groups; cross-plan consumers verify the producer's required implementation/tests as well. Proposed names alone do not prove delivery. |
| `src/transport_tube.h` (planned public contract; link after implementation) | Live definitions; no fixed range | [r2mlvs17-underground-transport-tubes.md](r2mlvs17-underground-transport-tubes.md) Groups 1-8, ending with Group 8's public-contract task, publish `TubeVehicleCapability`, `TubeTerminal`, `CheckTubeConstruction`, `ComputeTubePowerDemand`. These exports/schema fields are authoritative for this area. Same-plan consumers follow those earlier groups; cross-plan consumers verify the producer's required implementation/tests as well. Proposed names alone do not prove delivery. Read this producer for the tube capsule/terminal/service-facility interface integration in Groups 6-8 (only the named integration/subscenario); missing it blocks only that integration, not independent work. |

**ASSUMPTION: this was already built previously. If not, re-asses at runtime**. This applies only to the producer contracts named in this area's gate, after their listed producer groups have run. Verify their actual exports and focused tests before using the sketches below; proposed types and helper names in sketches are not evidence of delivered code.

**Facility versus fleet checks.** Reuse the existing station and vehicle commands named in this reading area. Complete conventional station/depot and carrier fixtures first; the capsule-control group consumes the later r2mlvs17 provider and does not implement guideway physics or another power account.

Context has compacted 1 times while executing this group.

### Group 6 - Add excavation-aware freight and service facilities

{{MEDIUM}}
- [X] 6.1 Add underground rail loading stations and road mine terminals with normal StationIDs/orders and same-layer catchment. Validate full footprint and approaches before construction; no Surface passenger catchment is implied.
- [X] 6.2 Add compatible train/road depots on excavated ground. Separate servicing from commissioning so a TBM can be repaired but cannot be purchased/cloned at an ordinary depot.
- [X] 6.3A Verify joined-station and separate-terminal mine pickup into actual station packets on each rock layer, preserving normal station-spread and company ownership rules.
- [X] 6.3B Verify actual conventional carrier loading at those joined/separate facilities after Group 7 supplies native engines; finish this dependent acceptance during Group 7B.

Evidence: actual station/terminal/depot commands and the delivered TBM service/purchase boundary passed 940 assertions in seven cases (`r2mlvs7-ax-facility-checks.log`, build aw). Foreign-company joins and a three-tile spread limit refuse before station/road-stop allocation or geology/cash changes. Normal StationIDs/orders and same-layer finite mine packets work in all four joined/separate scenarios. Earlier ao/aq/at/av expose fixture pause, teardown, allocation and distant-join setup errors and are not passing evidence. Vehicle loading remains explicitly pending until native freight exists.

### Group 7 - Define and integrate the conventional underground fleet

{{MEDIUM}}
- [X] 7.1 Add mine trucks, underground freight locomotive/mineral wagons and deep-capable sealed carriers. Fill an explicit ten-resource/spoil-to-model/refit table with at least one valid transport option per cargo/date.

Evidence: native EIDS identity/reconstruction and complete mine/spoil coverage passed 230 assertions in two cases (`r2mlvs7-bb-native-engine-checks.log`, build ba). Actual engine definitions resolve original catalogue rows and shared cargo labels; buy/refit/clone/autoreplace parity follows in Group 7B. Initial ay/az build diagnostics were missing refusal definitions and a fixture label formatter, corrected before acceptance.

Conventional transport table (actual immutable catalogue IDs; rail wagons use locomotive model 4):

| Cargo | Rail model/refit | Road model/refit | First carrier date |
| --- | --- | --- | --- |
| Coal (`COAL`) | 5 mineral wagon | 7 bulk truck | 1950-01-01 |
| Iron ore (`IORE`) | 5 mineral wagon | 7 bulk truck | 1950-01-01 |
| Copper ore (`CORE`) | 5 mineral wagon | 7 bulk truck | 1950-01-01 |
| Bauxite (`BAUX`) | 5 mineral wagon | 7 bulk truck | 1950-01-01 |
| Salt (`SALT`) | 5 mineral wagon | 7 bulk truck | 1950-01-01 |
| Lithium (`LITH`) | 5 mineral wagon | 7 bulk truck | 1950-01-01 |
| Nickel ore (`NORE`) | 5 mineral wagon | 7 bulk truck | 1950-01-01 |
| Cobalt (`COBT`) | 5 mineral wagon | 7 bulk truck | 1950-01-01 |
| Uranium (`URAN`) | 6 sealed wagon | 8 sealed carrier | 1980-01-01 |
| Rare minerals (`RARE`) | 6 sealed wagon; 5 also accepts | 8 sealed carrier | 1980-01-01 for paired sealed options |
| Earth (`ERTH`) | 2 dedicated hopper | 7 bulk truck | 1950-01-01 |
| Rock (`ROCK`) | 3 dedicated hopper | 7 bulk truck | 1950-01-01 |

Deep operations still require the profile's 1980 unlock even for an earlier carrier. The shared catalogue supplies all cargo labels, domain and introduction dates. Actual definitions append into the pinned package's existing EIDS range (`0x8004`–`0x8008`); models 2/3 retain their delivered TBM mappings. Tube/orbital logical designs gain no ordinary engines. Conventional locomotive/trucks pay ordinary running costs; wagons are hauled, while project machinery retains its own accounting. Temporary base sprites are not final asset acceptance.

### Local Mandatory Reading - Conventional command parity

_Applies to Group 7B; supplements the Facilities and fleet Area above._

Read `src/underground_vehicle.h` live native EIDS/capability exports after Group 7 delivers them; inspect `CmdBuildVehicle`, `CmdRefitVehicle`, `CmdCloneVehicle` in `src/vehicle_cmd.cpp`, `CmdAutoreplaceVehicle` in `src/autoreplace_cmd.cpp`, and the train/road build lists and refit list in `src/build_vehicle_gui.cpp` / `src/vehicle_gui.cpp`. Commands retain mutation authority and must refuse before temporary clone/refit/replacement allocation. Finish Group 6.3B here using actual native carriers and ordinary packet loading.

### Group 7B - Apply conventional fleet command and UI parity

{{HARD}}
- [X] 7.2 Apply identical capability checks in buy/refit/clone/autoreplace commands and UI. Verify running costs, service, group/list membership and layer/project labels for these ordinary manageable vehicles.

Evidence: actual fleet commands, Deep depot purchase UI, four native mine-loading scenarios and a controller-assigned native hauler's project label passed 568 assertions in six cases (`r2mlvs7-bi-fleet-ui-checks.log`, build bh). Exact normal engine startup dates and affected fleet/purchase cases passed 355 assertions in six cases (bm, build bl). Stock engine startup remains ordinary; native depot designs cannot receive random introduction delays. Refused buy/refit/NoTest clone/whole-chain replacement requests leave allocations, cargo, cash and RNG unchanged. Loading fixtures position a genuinely purchased truck at the real terminal as their arrival boundary, then use actual station orders and economy packet loading; this is not end-to-end route delivery acceptance.

### Local Mandatory Reading - Physical conventional transport

_Applies to Group 7C; supplements the Facilities and fleet Area above._

Read `VehicleEnterTile_LayerPortal` in `src/layer_portal_cmd.cpp`, the elevator wait/crossing paths in `src/roadveh_cmd.cpp`, `src/tests/portal_date.cpp`'s actual vehicle admission fixture, and `HasPowerOnRail` in `src/rail.h` plus `BuildRailVehicleAtSite` in `src/train_cmd.cpp`. Native conventional designs reuse existing vehicle movement, railtype and elevator owners; logical cargo/layer eligibility cannot grant incompatible power or articulated passage. Earlier proposed `layer_portal_vehicle.h/.cpp` names were stale: there is no separate elevator admission controller.

### Group 7C - Verify elevator and rail power admission

{{MEDIUM}}
- [X] 7.3 Test truck/articulated elevator admission and freight railtype power compatibility. Temporary work rail cannot power an incompatible electric vehicle just because its sprite resembles ordinary track.

Evidence: actual command-purchased trucks physically ride Surface/Deep and Underground/Deep elevators, observing the timed wait. Connected endpoints accept pool-owned articulated front/follower reference roles and refuse trams or pre-1980 Deep. A genuine original electric engine refuses a non-electric depot; actual paid TBM work lays support rail whose railtype remains unpowered for electric engines. Focused admission and translated quote checks passed 4,706 assertions in five cases (`r2mlvs7-ca-physical-admission-checks.log`, build bz). Standalone portal housing producer checks passed 147 assertions in three cases (bx, build bw). The follower fixture is physical admission evidence, not an articulated native purchase design. Native roads explicitly connect through `CmdChangeLayerPortalConnection` after both approaches are built; the earlier incomplete fixture's sealed housings correctly refused traffic.

### Group 8 - Integrate the five delivered tube models into fleet controls

Goal-authorized skip (2026-10-08, master finalization): dispatched expansion work in this group is skipped under [finalization exception GP-TG](../../../zz-rhr/upgrades/2026-10/portal-tbm/r2mlvs3mp-z-a-finalization-ledger.md#gp-tg-tube-and-geothermal-expansion-requires-major-gap-planning). Existing delivered prefixes stay complete; unimplemented profile work remains pending. No skipped task is markedX.

{{MEDIUM}}
- [ ] 8.1 Consume r2mlvs17 engine identities for Mole-70, Crate-75, Cistern-80, Shield-85 and Forge-05; display their distinct dates, cargo roles and art rather than define another registry.
- [ ] 8.2 Connect tube-terminal purchase/refit/order/service controls and explain ordinary-rail/portal refusals. Verify each capsule stays in its single-layer network and transfers cargo to conventional freight for layer changes.

Scoped gate: `src/transport_tube.h` remains absent. Group 8 stays unchecked; the delivered logical capsule filter is not a runtime engine/network provider. Conventional fleet delivery does not allocate capsule identities or emulate terminals, service, power or guideway physics.

### Area Mandatory Reading - Art, workflows and acceptance

_Applies to Groups 9-12._

**Raster source workflow.** New or corrected fleet, station/depot, toolbar and cursor originals use Plan13's [framed ImageGen prompt and extraction checks](r2mlvs13-graphics-and-asset-pipeline.md#framed-imagegen-sheets-and-safe-extraction). Separate batched views/states with pure-magenta source frames, transparent gutters and verified rectangular crops, or generate individual files. Vehicle families retain all manifest directions/load states; UI assets retain their own dimensions and matrix. Exclude frames before conversion and review model consistency independently. A clean source sheet does not close the actual loaded fleet/control tasks below.

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
| [`src/underground_gui.h`](../../../../src/underground_gui.h) (delivered public owner) | Live definitions; no fixed range | [r2mlvs7-underground-construction-and-fleet.md](r2mlvs7-underground-construction-and-fleet.md) Groups 1-5, ending with Group 5's public-contract task, publish `ShowUndergroundConstructionToolbar`, `BuildUndergroundVehicleFilter`. These exports/schema fields are authoritative for this area. Same-plan consumers follow those earlier groups; cross-plan consumers verify the producer's required implementation/tests as well. Proposed names alone do not prove delivery. |
| `src/transport_tube.h` (planned public contract; link after implementation) | Live definitions; no fixed range | [r2mlvs17-underground-transport-tubes.md](r2mlvs17-underground-transport-tubes.md) Groups 1-8, ending with Group 8's public-contract task, publish `TubeVehicleCapability`, `TubeTerminal`, `CheckTubeConstruction`, `ComputeTubePowerDemand`. These exports/schema fields are authoritative for this area. Same-plan consumers follow those earlier groups; cross-plan consumers verify the producer's required implementation/tests as well. Proposed names alone do not prove delivery. Read this producer for the tube model/routes/date controls subscenario in Group 11 (only the named integration/subscenario); missing it blocks only that integration, not independent work. |
| `src/geothermal.h` (planned public contract; link after implementation) | Live definitions; no fixed range | [r2mlvs18-geothermal-energy-and-industry.md](r2mlvs18-geothermal-energy-and-industry.md) Groups 1-7, ending with Group 7's public-contract task, publish `GeothermalHeatAllocation`, `GeothermalUtilityLink`, `AllocateVentHeat`, `ExportGeothermalPower`. These exports/schema fields are authoritative for this area. Same-plan consumers follow those earlier groups; cross-plan consumers verify the producer's required implementation/tests as well. Proposed names alone do not prove delivery. Read this producer for the geothermal plant-control/timeline subscenario in Group 11 (only the named integration/subscenario); missing it blocks only that integration, not independent work. |
| [Sprite Contract and delivery owner](r2mlvs13-graphics-and-asset-pipeline.md#public-handoff-contract) | Live definitions; no fixed range | `Sprite Contract Handoff` defines manifest fields `asset_id`, `consumer`, `directions`, `frames`, `anchor`, `source`; the matching authoring/conversion/runtime groups deliver actual assets. Verify the requested family and packed loading fixture before graphics integration; template/manifest publication alone is not final art. |

**ASSUMPTION: this was already built previously. If not, re-asses at runtime**. This applies only to the producer contracts named in this area's gate, after their listed producer groups have run. Verify their actual exports and focused tests before using the sketches below; proposed types and helper names in sketches are not evidence of delivered code.

**Final-art gate. ASSUMPTION: this was already built previously. If not, re-asses at runtime**. Toolbar/cursor, station/depot and directional/load graphics integration consumes r2mlvs13's delivered manifest entries and packed asset fixture. Verify them before those tasks; if absent, continue command/workflow tests but keep final visual acceptance pending. The manifest alone does not prove all assets load.

**Scoped graphics gate. ASSUMPTION: this was already built previously. If not, re-asses at runtime**. Final graphics tasks in this area require the corresponding r2mlvs13 manifest entries, delivered sprites and packed loading fixture. Verify them before binding/review; if absent, stop only those art tasks and their acceptance claims, continue independent code/state tests, and never count placeholders as finished assets.

### Group 9 - Art, workflows and acceptance

{{MEDIUM}}
- [ ] 9.1 Integrate final toolbar/cursor, station, depot, vehicle direction and cargo-load sprites from r2mlvs13.
- [ ] 9.2 Verify vehicle occlusion at loading docks, bays and deep portals.
- [X] 9.3 Complete a player workflow using only visible controls: survey -> commission/dig -> exchange -> build mine/station -> buy/refit carrier -> create orders -> deliver to Surface. Tracked in consecutive Groups 9B/9C below; final art does not gate functional controls.
- [X] 9.4 Repeat on both rock layers and through main/extra viewports, split toolbar sizes, keyboard controls and editor. Tracked in Group 9D below; editor authoring preserves its deliberate company/simulation restrictions.
- [X] 9.5 Confirm ordinary airports cannot be constructed below Surface through commands either.

Scoped final-art gate: the published manifest still identifies itself as `M0_contract_only`; the native model 5-8, rail-station, road-terminal and depot family sources remain `planned` with no delivered source paths. Allocated control offsets are not final-art delivery. The available `multilayer_sprite_contract.py` fixture validates contract/placement, not complete packed runtime art. Tasks 9.1/9.2 and final visual/gallery acceptance remain unchecked. Functional workflows 9.3/9.4 continue with current graphics and must exercise actual controls and physical transport, without substituting independent component passes for a connected route.

Independent airport evidence: actual estimate/execute commands refuse Underground, Deep and Space under both native and preserved Classic policy, leaving station allocation, geology, cash and RNG unchanged (64 assertions, `r2mlvs7-cc-airport-boundary-checks.log`, build cb).

### Local Mandatory Reading - Visible excavation and exchange

_Applies to Group 9B; supplements the Art, workflows and acceptance Area above._

Read `src/underground_gui.cpp`'s actual area/project/exchange event handlers, `src/layer_portal_gui.cpp`'s project start/assign/retire controls, `src/tbm_controls.h`'s public derived controls and `src/tests/tbm_movement.cpp`'s real `Train::Tick` drive boundary. Read only the relevant producer quote/movement/exchange implementation when an actual connected workflow disagrees. Scenario preconditions may prepare an accepted company/content/terrain/receiver world; after the workflow begins, no staged phase, fabricated cargo, teleported vehicle or opened rock cell may substitute for a visible action or native tick.

### Group 9B - Complete visible excavation and exchange

{{HARD}}
- [X] 9.3A Exercise survey, canonical commissioning, actual boring and exchange setup through their visible controls and native simulation owners; fix discovered player-facing gaps.

Evidence: visible survey, area purchase, project assignment, ordinary disposal orders/start and native cutter/hauler ticks complete a normal-rule straight corridor and a sharp-turn-enabled chamber. Actual receiver stock/processing totals account for all committed Earth/Rock and vehicle packets are empty at completion. A forbidden bend is refused by both the visible quote and estimate/execute commands before purchase. The focused burst passed 240 assertions in two cases (ed, build ec); affected real cutter/viewport/area controls passed 34,026 assertions in eight cases (ee). The connected run exposed and fixed missing cutter destinations and plain-support target recognition in YAPF, and an admitted-but-undrivable support bend. No final art or mine-to-Surface mineral delivery is claimed by this excavation subtask.

### Local Mandatory Reading - Visible mine freight delivery

_Applies to Group 9C; supplements the Art, workflows and acceptance Area above._

Read actual `src/industry_gui.cpp` funding/placement, `src/road_gui.cpp` terminal/depot/portal construction, `src/build_vehicle_gui.cpp` purchase, `src/vehicle_gui.cpp` refit/start and `src/order_gui.cpp` station-order placement handlers. Consult actual economy delivery/receipt owners only for the cargo that the route delivers. Consume the actual dug floor from Group 9B; prepared arrival positions or copied inventories do not close delivery acceptance.

Context has compacted 1 times while executing this group.

### Group 9C - Complete visible mine-to-Surface freight

{{HARD}}
- [X] 9.3B Continue the same world through mine/station/depot construction, ordinary carrier purchase/refit, visible orders and actual Surface delivery; verify finite reserve/cargo/receiver conservation.

Evidence: the same actually excavated world continues through visible machine retirement/support conversion, demolition, native mine funding, road terminal/depot/portal construction, carrier purchase, two actual refits, station orders/start and normal construction/industry/vehicle ticks. The carrier physically delivers to the compatible original Surface receiver and becomes empty; finite reserve/output/dispatch/packets reconcile against its real accepted history and company delivery count. Portal placement now completes an existing road stub through the ordinary paid road command. Focused excavation/refusal/freight checks pass 483 assertions in three cases (fu, build ft); the connected freight case takes 1.279 seconds. This validates Coal intake, without claiming useful power from the still-gated utility producer.

### Local Mandatory Reading - Workflow context parity

_Applies to Group 9D; supplements the Art, workflows and acceptance Area above._

Read `src/tests/underground_window.cpp`'s actual main/extra picking and toolbar/hotkey cases, `src/toolbar_gui.cpp`'s captured rock-layer dispatch and the editor's explicit company restrictions. Repeat the connected normal-game workflow on both rock layers and listed UI entry contexts; validate editor authoring separately, then normal-game transport. Scenario editor vehicle purchase or ticking is not a supported player operation.

### Group 9D - Repeat the connected workflow in each UI context

{{HARD}}
- [X] 9.4A Repeat actual functional workflow on both rock layers through main/extra viewport, full/split toolbar and keyboard entry contexts.
- [X] 9.4B Verify the editor's visible authoring entry and construction behavior with its existing company/transport restrictions, and continue freight in the normal game context.

Evidence: all 16 layer/main-extra/full-split/menu-keyboard combinations independently finish actual digging, spoil exchange/disposal and mine-to-Surface mineral delivery (4,520 assertions, gm). Underground uses the bulk truck and compatible Coal receiver; Deep uses the sealed carrier and actual Electronics works Rare input. Both editor cases preserve company claim/fleet restrictions, survey without mutation, author ordinary roads on real dug floor, then complete normal-game freight in that same world (621 assertions, gl). Native cursor picking is used for orders as well as construction, with destination layer selection before arming placement and floating dialogs moved beside the world. Deep's first hard-rock cut exposed insufficient UI defaults: shared `GetDefaultTBMConsist(MapLayer)` now supplies two Earth/four Rock hoppers there, while Underground retains two/two; both area and portal commissioning reuse that owner. No simulation capacity or excavation permission is bypassed.

### Group 10 - Verify command parity and fleet persistence

{{HARD}}
- [X] 10.1 Send malformed commands directly for station footprint/layer and specialist purchase/refit/clone paths; compare refusal with toolbar behavior and confirm no partial allocation.
- [X] 10.2 Test same-XY stations across layers and a cargo with no compatible model; the UI must expose the same restriction the command enforces without creating catchment transfer.

Evidence: malformed command, plane-boundary join, identical-coordinate station, actual unsupported-cargo purchase UI and affected joined/separate facility checks passed 447 assertions in five cases (`r2mlvs7-ch-station-plane-checks.log`, build cg). The boundary fixture exposed a real assertion: a Surface station at Y=61 could join Underground Y=1 because raw stacked rows looked close. `CheckStationSpread` now rejects different physical layers before any joining mutation, preserving normal same-layer spread. Water Ice offers no native freight purchase row while Coal retains one. Earlier cf records the reproduced catchment crash, not a passing result.

### Local Mandatory Reading - Conventional fleet persistence

_Applies to Group 10B; supplements the Art, workflows and acceptance Area above._

Read actual engine setup and EIDS save ownership (`SetupEngines` / `src/saveload/engine_sl.cpp`), actual Vehicle/order serialization (`src/saveload/vehicle_sl.cpp`, `src/saveload/order_sl.cpp`), `src/tests/layer_save_fixture.h`'s file sink/content setup and `src/tests/multilayer_state_fixture.py`'s repository-local loader harness. Preserve actual native mappings when preparing a full save: resetting original mappings after native engine creation cannot authorize a round-trip. Read native project/exchange serialization only for the active association subscenario. Fixtures must build actual vehicles through existing commands and retain immutable inputs.

### Group 10B - Verify native fleet persistence

{{HARD}}
- [X] 10.3A Save/reload a conventional underground fleet with station/service orders and an active project association; compare actual engine mappings, cargo, filters and controller associations through the real game loader.
- [ ] 10.3B After tube delivery, repeat for capsule orders/refits and compare engine IDs, cargo and filters. This subscenario waits for the scoped `transport_tube.h` producer.

Evidence: the real save-writer case passed 378 assertions (ck, build cj). It uses dense original engine setup before native EIDS registration, distributed cargo slots, a physically driven/loaded mine truck, command-funded disposal receivers and an actually assigned native hauler with normal station/service orders. The actual game loader/resaver passed in eight seconds (`r2mlvs7-cm-fleet-roundtrip-checks.log`, detailed cn, Debug GUI build cl). Exact EIDS/VEHS/ORDL and project/exchange descriptor bytes match; read-only `dump_info underground_fleet` confirms every native runtime mapping, nonempty truck cargo, orders, active association and both seven-row filters. The supplied source and startup-script bytes remain unchanged. Capsule persistence remains pending.

### Group 11 - Art, workflows and acceptance

{{MEDIUM}}
- [X] 11.1A Exercise pre/post-1980 native Deep controls and canonical area-clearing tool previews using the delivered event/UI/command cases.
- [ ] 11.1B Exercise actual tube model introduction controls and geothermal plant timelines after their scoped producers arrive. Logical catalogue dates alone are not runtime controls.
- [ ] 11.2 Verify five distinct tube vehicles appear with their final art and route cargo through layer-local terminals to conventional interchanges.

Native evidence is the actual toolbar/viewport/area and depot UI cases in Groups 3C/3D/4/7B, plus the loaded native runtime filter round-trip in Group 10B. Tasks 11.1B/11.2 remain gated by missing runtime providers and their final art; no model/gallery/network closure is claimed.

Context has compacted 2 times while executing this group.

### Group 12 - Art, workflows and acceptance

{{SIMPLE}}
- [X] 12.1 Record the implemented public exports, focused test commands/results and remaining runtime/art/network gaps in this plan; retain original supplied saves and repository-local evidence.
- [X] 12.2 Update affected architecture/handoff documentation to describe delivered behavior and reconcile the parent index only when this plan's Completion Conditions are met. Keep every unfinished task unchecked.

Delivered architecture: [vehicles](../../architecture/vehicles/archi-vehicles.md#native-underground-conventional-fleet), [map layers](../../architecture/map-layers/archi-map-layers.md#underground-construction-and-same-layer-facilities) and [viewport placement](../../architecture/gui/archi-viewport.md#clicks-and-placement) now describe actual native EIDS/capability ownership, ordinary lifecycle/persistence, complete facility/approach quotes, physical-layer joining and cursor-owned previews. Public exports are recorded in this plan's handoff above. The parent completion index remains open because this plan's Completion Conditions are unmet.

Focused executable commands run from `build`, with the Debug test binary rebuilt before each source-changing burst. Use `./Debug/openttd_test.exe '<filter>' --durations yes` with these filters; each passing unit burst took less than two seconds, and no full regression was run.

| Filter / command | Passing result | Recorded evidence suffix |
| --- | --- | --- |
| `[underground-engine]` | 230 assertions / 2 cases | bb, build ba |
| `[underground-fleet],[underground-fleet-ui],[underground-loading],[underground-project-label]` | 568 assertions / 6 cases | bi, build bh |
| `[underground-engine-startup],[underground-fleet],[underground-fleet-ui],[tbm-purchase-gate]` | 355 assertions / 6 cases | bm, build bl |
| `[underground-admission],[underground-footprint-quote]` | 4,706 assertions / 5 cases | ca, build bz |
| `[underground-command-boundary],[underground-facility]` | 447 assertions / 5 cases | ch, build cg |
| `[underground-save-file]` | 378 assertions / 1 case | ck, build cj |
| `[underground-window],[underground-area-window],[underground-area-turns]` | 301 assertions / 4 cases | cp, final Debug build co |
| `python src/tests/underground_save_fixture.py` from repository root | Actual loader/resaver, exact fleet descriptors and loaded native owner checks, 8 seconds | cm and detailed cn, runtime build cl |

All evidence names above expand to `r2mlvs7-<suffix>-*.log` beside this plan. The command-generated [source save](../../../../src/tests/fixtures/underground/native-fleet.sav) and [actual loader resave](../../../../src/tests/fixtures/underground/native-fleet-roundtrip.sav) are retained. The fixture verifies the input hash and restores the original startup bytes; it refuses to overwrite an existing resave, so preserve previous evidence before explicitly preparing another run.

Playable build checkpoint: Debug GUI/tests succeeded (co), then RelWithDebInfo GUI succeeded (cq); both executable directories contain all 66 language packs matching generated ABI `0x748A1284`. The optimized actual-loader smoke check passed in two seconds (cr; detailed cs), resolving all seven native mappings, both filters, loaded mine cargo/station/service orders and the active hauler association. It restored the original startup script and left the source save hash unchanged, writing no new save. The `play.ps1` default optimized configuration is rebuilt.

Remaining acceptance is explicit: actual tube/geothermal producers and their controls; capsule registration/refits/orders/persistence; final station/depot/directional/load sprites and packed loading/occlusion/gallery; the complete visible-controls mine-to-Surface workflow and repetition across both rock layers and all listed UI contexts; and final combined multiplayer/network and large-map runtime checks. Logical capsule filters, development base sprites, command-level loading and physical elevator cases do not close those tasks. No original user save was rewritten.

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
