# R2MLVS16 - Integrated Acceptance and Balance

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
| [docs/zz-reach2/upgrades/2026-10/r2mlvs-multilayer-vehicles-and-structures.md](r2mlvs-multilayer-vehicles-and-structures.md) | 296-300 | `AD-14` supplies the canonical gameplay/ownership decision for every group of this plan. It constrains measured combined evidence without treating no-desync or art sheets as full acceptance; a consumer must not redefine it locally. |
| [src/map_layer.h](../../../../src/map_layer.h) | 78-85 | `MultiLayerMap` ownership preamble: `Map` owns packed storage/geometry and this controller owns layer semantics. Every group uses world tiles/explicit layer helpers; camera selection and ad hoc plane offsets cannot define gameplay. |
| [src/network/network_server.cpp](../../../../src/network/network_server.cpp) | 670-687 | `ServerNetworkGameSocketHandler::SendSync` compares RNG seeds, not a full gameplay checksum; validate orbital/geology/cargo ledgers in addition to a no-desync result. |

Status, 2026-10-08: ground-scoped runner/schema2, physical fixture loops/recovery, paid corridor ledger, selected native art and bounded electrical-owner measurements are delivered. [Final audit](portal-tbm/r2mlvs3mp-z-b-final-closure-audit.md) records GP-TG/GP-NJ/GP-BT/GP-PF and deferred personal review. Combined acceptance and the full plan remain incomplete. Parent: [R2MLVS master](r2mlvs-multilayer-vehicles-and-structures.md). This plan owns evidence, not duplicated feature implementations.

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

Demonstrate a playable connected economy with finished graphics, conserved materials, working save/multiplayer behavior and measured costs/performance. Mandatory sources below govern each area; consult broad architecture and predecessor build notes only through their declared triggers.

## Scenario register

**Revised Space scenarios,2026-10-12:** use the current
[Steel/Goods and probe-role contract](r2mlvs1-d-recipes-and-bootstrap.md#space-economy-revision---2026-10-12)
and numerical budget rather than the historical preassembled-Core ledger.
Cover Earth-only initial mining/survey/construction launches; hidden quantities
until a company completes a scan; Visit(A) then Mine(B); two-material first
Core with no previous station; imported versus orbital-produced construction
inputs; a built Steelworks/Goodsworks ore-to-material loop; exclusive reuse
versus Earth drop; and later local probe assembly. Refuse missing inputs,
blocked output, invalid roles and insufficient cargo mass/stock capacity before
mutation. Include partial-job saves, two-company knowledge, company/target-ID
reuse, native expenses, one payment/packet owner, hidden views and actual
network replay. No scenario is passed by these planning changes.

| ID | Scenario | Evidence required |
| --- | --- | --- |
| S1 | Sealed entrance and general area clearing | Ordinary traffic refused; portal-commissioned TBM clears corridors/irregular chambers with maximum 45-degree advancing turns, returns real spoil and recovers before a bay exists; cleared ground can later become tubes or plant foundations |
| S2 | Recurring spoil logistics | Actual wagon IDs move from TBM to secondary train; it reaches the original Surface station, disposes/transfers to a real sink, returns empties and restarts work |
| S3 | Underground economy | All five Underground resources are delivered to working Surface consumers; rejected solid-rock construction across command families |
| S4 | Deep economy and calendar | All Deep entry/construction/operation paths refused on 31 December 1979 and available from 1 January 1980; five additional resources; train uses two layer transitions and truck a timed elevator; legacy saves preserve their explicit policy |
| S5 | Space mining and collection hub | Waterside center built from Airport controls; probe mines moving asteroid; cargo release at a point within the inclusive 20-tile circle; descent and bounded Surface handling supply separate train and lorry stations |
| S6 | Solar support | Surface kit manufacture/delivery; launched mirror; valid assignment; 100 baseline -> 400 effective useful power, capped despite extra mirrors |
| S7 | Orbital manufacturing | Surface-built components form Core/Dock/Assembly; heavy robot mined input feeds a processor; real product reaches a useful destination |
| S8 | Lifecycle stress | Queues, full wagons/buffers, funds exhausted, cancellation, unsafe removal, target ID reuse, takeover/bankruptcy and recovery |
| S9 | Persistence and network | Legacy/new saves, every mid-action state, headless progression and live multiplayer join with different visible layers |
| S10 | Art and scale | Complete direction/state/zoom gallery, toolbar/extra viewport behavior, representative map/entity performance and distribution hashes |
| S11 | Five original tube vehicles | Mole-70, Crate-75, Cistern-80, Shield-85 and Forge-05 have distinct designs/stats and useful cargo routes on excavated tube networks, with power, dates, mode separation and conventional cross-layer transfers |
| S12 | Geothermal technology progression | 1980 vent power, 2000 improved generation, 2010 direct smelting, 2030 batteries and 2045 Mantle Forge; shared heat budget, transported inputs, useful output and explicit electricity export |

## Public Handoff Contract

Delivered ground owner: [src/tests/multilayer/acceptance/native_ground.py](../../../../src/tests/multilayer/acceptance/native_ground.py), migrating under R2FC to `src/tests/multilayer/acceptance/native_ground.py`, publishing `run_scenario`, `capture_ledger`, `compare_state_digest` and `validate_art_manifest`. Schema2 records actual subprocess/artifact/configuration hashes, complete selected owner state and explicit isolated-fixture classification. Six refusal tests prevent false bootstrap/invalid-output claims. Later-family and real peer-join scenarios require their actual producers; no ground prefix completes the whole scenario register.

## Implementation Tasks

Ground execution scope: [R2MLVS3MP](r2mlvs3mp-master-plan.md) selects S1/S2, ground S8/S9, the related portal/tube/riser checks and ground art/economics/performance rows. Groups 1-5 must build the real acceptance runner for this explicitly selected family set without requiring all orbital producers listed in the shared entry gate. Each scenario requires its actual public producers, matching loaded assets and meaningful executable boundary tests; absent required producers block that scenario. Preserve the stricter complete producer set for combined S1..S12/whole-epic closure. A scoped ground pass does not complete a mixed-family task or this entire plan, and skipped/injected evidence never becomes bootstrap acceptance. The named ground portions of Groups 1-5 (1.1-G, 2.2-G, 3.3-G, 4.1-G) and the family-neutral tasks are listed in [r2mlvs3mp-e](portal-tbm/r2mlvs3mp-e-scoped-gates.md#plan-15-groups-1-5).

Deepening pass, 2026-10-04: implementation groups use only SIMPLE, MEDIUM or HARD difficulty, with at most eight pending tasks each. Sketches describe proposed implementation contracts; verify the named earlier producers at each assumption gate. No gameplay or asset implementation is marked complete by this planning pass.

Groups are chronological and contain consecutive tasks of one difficulty. Each difficulty batch has at most eight tasks; area reading changes only when the implementation boundary or source of truth changes.

### Area Mandatory Reading - Fixture and build readiness

_Applies to Groups 1-5._

**Selected-family gate.** For the R2MLVS3MP ground runner, the complete producer list below is filtered by its explicitly selected scenarios. Always require the actual content/geology/portal/TBM/exchange/schema owners and matching loaded ground asset families; additionally require industry/UI/tube/utility/geothermal owners for selected scenarios that use them. Space/launch/asteroid/orbital/mirror exports are required only for selected orbital/mirror scenarios or combined whole-epic closure. The runner must record its family selection and reject a missing required producer/artifact; this is not an option to skip unavailable dependencies and report success. Verify the following named exports and producer tests for every required selected family.

**Entry gate.** The following public contracts must exist before dependent implementation in this area: `src/multilayer_content.h`: `MultiLayerGameplayProfile`, `LayerVehicleCapability`, `GetMultiLayerContentProfile`, `CheckDeepLayerAvailability`, `UtilityPowerAccount`; `src/excavation.h`: `ExcavationState`, `GeologyDeposit`, `ExcavationJob`, `CheckExcavatedFootprint`, `CommitExcavationCut`, `GeothermalVentField`; `src/layer_portal_cmd.h`: `LayerPortalWorkState`, `GetLayerPortalWorkState`, `CheckLayerPortalWorkAccess`, `CommissionLayerPortalTBM`; `src/tbm.h`: `TBMProject`, `TBMWorkState`, `ValidateTBMAreaRoute`, `AdvanceTBMWork`; `src/tbm_exchange.h`: `TBMExchangeAssignment`, `ValidateTBMWagonExchange`, `ExecuteTBMWagonExchange`; `src/multilayer_industry.h`: `LayerIndustryCapability`, `ValidateMineDepositClaim`, `ProduceLayerMineCargo`, `PrepareLayerRecipeBatch`, `CommitLayerRecipeBatch`; `src/underground_gui.h`: `ShowUndergroundConstructionToolbar`, `BuildUndergroundVehicleFilter`; `src/space.h`: `SpaceVehicle`, `OrbitalBodyID`, `SpaceMission`, `SpaceStructure`, `SpaceTransition`; `src/launch_center.h`: `LaunchCenterAnchor`, `LaunchCollectionHub`, `SurfaceHandlingJob`, `SpaceDescentJob`, `CanReleaseOrbitalCargo`, `CommitOrbitalCargoRelease`; `src/asteroid.h`: `AsteroidBody`, `AdvanceAsteroidField`, plus `SpaceVehicle`, `SpaceMission`, `AssignSpaceMiningMission` and `SpaceVehicle::Tick` in `src/space_vehicle.h`; `src/orbital_station.h`: `OrbitalModule`, `OrbitalConstructionJob`, `ReserveOrbitalConstructionInputs`, `CommitOrbitalConstructionStep`; `src/solar_power.h`: `SolarGenerationAccount`, `MirrorAssignment`, `ComputeMirrorSolarOutput`; `media/multilayer/contracts/multilayer_sprite_manifest.json`: `asset_id`, `consumer`, `directions`, `frames`, `anchor`, `source`; `src/multilayer_state.h`: `MultiLayerStateSchema`, `ValidateMultiLayerStateReferences`; `src/transport_tube.h`: `TubeVehicleCapability`, `TubeTerminal`, `CheckTubeConstruction`, `ComputeTubePowerDemand`; `src/geothermal.h`: `GeothermalHeatAllocation`, `GeothermalUtilityLink`, `AllocateVentHeat`, `ExportGeothermalPower`.
Verify each public file/export/schema field with `rg` in the current workspace and inspect its command/field ownership. For a same-plan contract, compile its type/signature fixture or validate its artifact after the preceding producer groups. For cross-plan behavior, also run the producer's focused executable boundary test; declarations alone cannot authorize dependent mutations or runtime claims.
If a required export is absent, stop the dependent implementation, mutation and runtime claims in Groups 1-5. Read-only characterization, source/range verification, test-case design and asset-template work that does not assume the missing behavior may continue.
Do not bypass this gate by copying private pool/tile state, inventing local IDs or cargo registries, using a historical plan as a delivered implementation, opening user-wide content stores, substituting a second utility/accounting authority, or adding an unreviewed fallback path. Finish the named producer or revise the shared contract and its direct consumers first.

| File name | Line Ranges | Description |
| --- | --- | --- |
| [src/tests/map_layer.cpp](../../../../src/tests/map_layer.cpp) | 293-322 | Complete Deep initialization test checks fence/terrain and unchanged RNG; new geology must preserve feature-disabled behavior. |
| [src/tests/map_layer.cpp](../../../../src/tests/map_layer.cpp) | 430-442 | Complete display-order test retains immutable Space=2 and Deep=3 storage meanings; future UI/profile work cannot reorder saved IDs. |
| [media/baseset/openttd/CMakeLists.txt](../../../../media/baseset/openttd/CMakeLists.txt) | 6-67 | Complete `if(GRFCODEC_FOUND)` and `create_grf_command` build contract covers PNG/NFO inputs and checked-in fallback GRFs; final reviewed assets must reach both paths. |
| `src/multilayer_content.h` (planned public contract; link after implementation) | Live definitions; no fixed range | [r2mlvs1-gameplay-contracts-and-content.md](r2mlvs1-gameplay-contracts-and-content.md) Groups 1-6, ending with Group 6's public-contract task, publish `MultiLayerGameplayProfile`, `LayerVehicleCapability`, `GetMultiLayerContentProfile`, `CheckDeepLayerAvailability`, `UtilityPowerAccount`. These exports/schema fields are authoritative for this area. Same-plan consumers follow those earlier groups; cross-plan consumers verify the producer's required implementation/tests as well. Proposed names alone do not prove delivery. |
| `src/excavation.h` (planned public contract; link after implementation) | Live definitions; no fixed range | [r2mlvs2-geology-and-excavation.md](r2mlvs2-geology-and-excavation.md) Groups 1-7, ending with Group 7's public-contract task, publish `ExcavationState`, `GeologyDeposit`, `ExcavationJob`, `CheckExcavatedFootprint`, `CommitExcavationCut`, `GeothermalVentField`. These exports/schema fields are authoritative for this area. Same-plan consumers follow those earlier groups; cross-plan consumers verify the producer's required implementation/tests as well. Proposed names alone do not prove delivery. |
| [src/layer_portal_cmd.h](../../../../src/layer_portal_cmd.h) | Live definitions; no fixed range | [r2mlvs3-sealed-portals-and-deep-rail.md](r2mlvs3-sealed-portals-and-deep-rail.md) Groups 1-7, ending with Group 7's public-contract task, publish `LayerPortalWorkState`, `GetLayerPortalWorkState`, `CheckLayerPortalWorkAccess`, `CommissionLayerPortalTBM`. These exports/schema fields are authoritative for this area. Same-plan consumers follow those earlier groups; cross-plan consumers verify the producer's required implementation/tests as well. Proposed names alone do not prove delivery. |
| `src/tbm.h` (planned public contract; link after implementation) | Live definitions; no fixed range | [r2mlvs4-tunnel-boring-machines.md](r2mlvs4-tunnel-boring-machines.md) Groups 1-5, ending with Group 5's public-contract task, publish `TBMProject`, `TBMWorkState`, `ValidateTBMAreaRoute`, `AdvanceTBMWork`. These exports/schema fields are authoritative for this area. Same-plan consumers follow those earlier groups; cross-plan consumers verify the producer's required implementation/tests as well. Proposed names alone do not prove delivery. |
| `src/tbm_exchange.h` (planned public contract; link after implementation) | Live definitions; no fixed range | [r2mlvs5-spoil-wagons-and-haulage.md](r2mlvs5-spoil-wagons-and-haulage.md) Groups 1-5, ending with Group 5's public-contract task, publish `TBMExchangeAssignment`, `ValidateTBMWagonExchange`, `ExecuteTBMWagonExchange`. These exports/schema fields are authoritative for this area. Same-plan consumers follow those earlier groups; cross-plan consumers verify the producer's required implementation/tests as well. Proposed names alone do not prove delivery. |
| `src/multilayer_industry.h` (planned public contract; link after implementation) | Live definitions; no fixed range | [r2mlvs6-mines-and-resource-chains.md](r2mlvs6-mines-and-resource-chains.md) Groups 1-6, ending with Group 6's public-contract task, publish `LayerIndustryCapability`, `ValidateMineDepositClaim`, `ProduceLayerMineCargo`, `PrepareLayerRecipeBatch`, `CommitLayerRecipeBatch`. These exports/schema fields are authoritative for this area. Same-plan consumers follow those earlier groups; cross-plan consumers verify the producer's required implementation/tests as well. Proposed names alone do not prove delivery. |
| `src/underground_gui.h` (planned public contract; link after implementation) | Live definitions; no fixed range | [r2mlvs7-underground-construction-and-fleet.md](r2mlvs7-underground-construction-and-fleet.md) Groups 1-5, ending with Group 5's public-contract task, publish `ShowUndergroundConstructionToolbar`, `BuildUndergroundVehicleFilter`. These exports/schema fields are authoritative for this area. Same-plan consumers follow those earlier groups; cross-plan consumers verify the producer's required implementation/tests as well. Proposed names alone do not prove delivery. |
| `src/space.h` (planned public contract; link after implementation) | Live definitions; no fixed range | [r2mlvs8-space-world-and-construction.md](r2mlvs8-space-world-and-construction.md) Groups 1-7, ending with Group 7's public-contract task, publish `SpaceVehicle`, `OrbitalBodyID`, `SpaceMission`, `SpaceStructure`, `SpaceTransition`. These exports/schema fields are authoritative for this area. Same-plan consumers follow those earlier groups; cross-plan consumers verify the producer's required implementation/tests as well. Proposed names alone do not prove delivery. |
| `src/launch_center.h` (planned public contract; link after implementation) | Live definitions; no fixed range | [r2mlvs9-launch-centers-and-rockets.md](r2mlvs9-launch-centers-and-rockets.md) Groups 1-7, ending with Group 7's public-contract task, publish `LaunchCenterAnchor`, `LaunchCollectionHub`, `SurfaceHandlingJob`, `SpaceDescentJob`, `CanReleaseOrbitalCargo`, `CommitOrbitalCargoRelease`. These exports/schema fields are authoritative for this area. Same-plan consumers follow those earlier groups; cross-plan consumers verify the producer's required implementation/tests as well. Proposed names alone do not prove delivery. |
| `src/asteroid.h` (planned public contract; link after implementation) | Live definitions; no fixed range | [r2mlvs10-asteroids-and-mining-probes.md](r2mlvs10-asteroids-and-mining-probes.md) Groups 1-7, ending with Group 7's public-contract task, publish `AsteroidBody`, `AdvanceAsteroidField`, plus `SpaceVehicle`, `SpaceMission`, `AssignSpaceMiningMission` and `SpaceVehicle::Tick` in `src/space_vehicle.h`. These exports/schema fields are authoritative for this area. Same-plan consumers follow those earlier groups; cross-plan consumers verify the producer's required implementation/tests as well. Proposed names alone do not prove delivery. |
| `src/orbital_station.h` (planned public contract; link after implementation) | Live definitions; no fixed range | [r2mlvs11-orbital-stations-and-manufacturing.md](r2mlvs11-orbital-stations-and-manufacturing.md) Groups 1-6, ending with Group 6's public-contract task, publish `OrbitalModule`, `OrbitalConstructionJob`, `ReserveOrbitalConstructionInputs`, `CommitOrbitalConstructionStep`. These exports/schema fields are authoritative for this area. Same-plan consumers follow those earlier groups; cross-plan consumers verify the producer's required implementation/tests as well. Proposed names alone do not prove delivery. |
| `src/solar_power.h` (planned public contract; link after implementation) | Live definitions; no fixed range | [r2mlvs12-solar-farms-mirrors-and-eti.md](r2mlvs12-solar-farms-mirrors-and-eti.md) Groups 1-6, ending with Group 6's public-contract task, publish `SolarGenerationAccount`, `MirrorAssignment`, `ComputeMirrorSolarOutput`. These exports/schema fields are authoritative for this area. Same-plan consumers follow those earlier groups; cross-plan consumers verify the producer's required implementation/tests as well. Proposed names alone do not prove delivery. |
| `media/multilayer/contracts/multilayer_sprite_manifest.json` (planned public contract; link after implementation) | Live definitions; no fixed range | [r2mlvs13-graphics-and-asset-pipeline.md](r2mlvs13-graphics-and-asset-pipeline.md) Groups 1-8, ending with Group 8's public-contract task, publish `asset_id`, `consumer`, `directions`, `frames`, `anchor`, `source`. These exports/schema fields are authoritative for this area. Same-plan consumers follow those earlier groups; cross-plan consumers verify the producer's required implementation/tests as well. Proposed names alone do not prove delivery. |
| `src/multilayer_state.h` (planned public contract; link after implementation) | Live definitions; no fixed range | [r2mlvs15-savegame-determinism-and-lifecycle.md](r2mlvs15-savegame-determinism-and-lifecycle.md) Groups 1-6, ending with Group 6's public-contract task, publish `MultiLayerStateSchema`, `ValidateMultiLayerStateReferences`. These exports/schema fields are authoritative for this area. Same-plan consumers follow those earlier groups; cross-plan consumers verify the producer's required implementation/tests as well. Proposed names alone do not prove delivery. |
| `src/transport_tube.h` (planned public contract; link after implementation) | Live definitions; no fixed range | [r2mlvs17-underground-transport-tubes.md](r2mlvs17-underground-transport-tubes.md) Groups 1-8, ending with Group 8's public-contract task, publish `TubeVehicleCapability`, `TubeTerminal`, `CheckTubeConstruction`, `ComputeTubePowerDemand`. These exports/schema fields are authoritative for this area. Same-plan consumers follow those earlier groups; cross-plan consumers verify the producer's required implementation/tests as well. Proposed names alone do not prove delivery. |
| `src/geothermal.h` (planned public contract; link after implementation) | Live definitions; no fixed range | [r2mlvs18-geothermal-energy-and-industry.md](r2mlvs18-geothermal-energy-and-industry.md) Groups 1-7, ending with Group 7's public-contract task, publish `GeothermalHeatAllocation`, `GeothermalUtilityLink`, `AllocateVentHeat`, `ExportGeothermalPower`. These exports/schema fields are authoritative for this area. Same-plan consumers follow those earlier groups; cross-plan consumers verify the producer's required implementation/tests as well. Proposed names alone do not prove delivery. |
| [CMakeLists.txt](../../../../CMakeLists.txt) | 236-248 | Game/library/test target declarations identify the actual targets for Debug and RelWithDebInfo readiness checks. |
| [CMakeLists.txt](../../../../CMakeLists.txt) | 284-296 | Base-set/media linkage and Catch discovery bind assets and executable tests; record current test counts rather than historical totals. |
| [CMakeLists.txt](../../../../CMakeLists.txt) | 465-467 | Complete CTest/regression registration adds the integrated regression entry point. |
| `src/utility_power.h` (planned public contract; link after implementation) | Live definitions; no fixed range | [r2mlvs12](r2mlvs12-solar-farms-mirrors-and-eti.md) `Useful Power Handoff`, produced by its native-ledger, atomic-mutation and consumer-effect groups, supplies `PublishUtilityGeneration`, `AllocateUtilityPower`, `GetUtilityAllocation`, `UtilityGrantID`, `UtilityMutationKind`, `PrepareUtilityMutation`, `CommitUtilityMutation`. Verify its allocation, consume/release/layer-transfer, effect and save/retry fixtures before dependent power mutations; no private account writes or duplicate grid. |

**ASSUMPTION: this was already built previously. If not, re-asses at runtime**. This applies only to the producer contracts named in this area's gate, after their listed producer groups have run. Verify their actual exports and focused tests before using the sketches below; proposed types and helper names in sketches are not evidence of delivered code.

**Evidence boundary.** The earlier feature producers supply real state and test hooks. This plan orchestrates them; it does not implement a second simulation or create passing ledgers from expected values.

```python
# Conceptual harness flow; exported functions are implemented in this plan.
result = run_scenario(scenario_id, fixture, config)
ledger = capture_ledger(result.runtime_artifacts)
assert result.exit_status == 0
assert ledger.inputs == ledger.remaining + ledger.in_transit + ledger.consumed
# Use one equation per cargo/energy unit and retain explicit conversions.
compare_state_digest(result.reference, result.replay, same_simulation_tick=True)
```

Test-only setup remains valid for isolated failure fixtures, with a visible label. Final bootstrap scenarios use actual supplied/produced predecessors.

### Group 1 - Fixture and build readiness

{{SIMPLE}}
- [ ] 1.1 Record source revision, content/profile versions, configs, map seeds, toolchain and fixture hashes.
- [X] 1.1G Publish the ground runner's actual revision/executable/source/config/fixture/toolchain/package hashes, selected profile versions and declared setup metadata. Unselected whole-profile scenarios remain with1.1.

### Group 2 - Fixture and build readiness

{{MEDIUM}}
- [X] 2.1 Keep fixture saves and logs in the repository; preserve original user saves.
- [ ] 2.2 Establish feature-disabled regression fixtures and a compact fully connected Multilayer fixture.
- [X] 2.2G Load the actual connected ground fixture through the application loader and preserve genuine version370 Classic portal bytes/ordinary edges. No orbital connection is implied.

### Group 3 - Classify fixtures and enforce bootstrap evidence

{{MEDIUM}}
- [ ] 3.1 Mark each fixture as characterization, isolated boundary or real bootstrap acceptance, and record every seeded/test-only supply shortcut in its metadata.
- [X] 3.1G Classify every currently delivered ground runner fixture as an isolated boundary, retaining its actual seeded predecessor shortcuts and protected source identity.
- [X] 3.2 Specify the scenario-runner refusal contract for a bootstrap-pass claim whose predecessor inventory/facility was injected rather than produced/delivered. The later runner group implements it; keep isolated fixtures usable for fault diagnosis.
- [ ] 3.3 For S1..S7, record starting cash/date/resources and the normal commands that create the first required predecessor; ensure final evidence can distinguish development setup from playable supply chains.
- [X] 3.3G Record S1/S2 native invoice setup and S8/S9 individual/checkpoint provenance. Explicitly refuse a first-build bootstrap claim for these injected/retained development predecessors; actual playable bootstrap remains pending.

### Group 4 - Fixture and build readiness

{{MEDIUM}}
- [ ] 4.1 Build Debug and RelWithDebInfo game/test targets and required asset packages with the existing repository toolchain.
- [ ] 4.2 Run focused feature tests and the full appropriate CTest/regression suites; record exact current counts and new warnings.
- [ ] 4.3 Distinguish existing known baseline failures (including previously recorded legacy TTO conversion issues) from regressions. A historical skip or failure is never silently renamed a passing check.
- [X] 4.1G Build Debug and RelWithDebInfo game/test targets and current required ground packages.
- [X] 4.2G Run focused ground economics, native connected/Classic loading, failure-boundary, real continuation and runner refusal checks. Full whole-profile regression remains with4.2, outside this focused checkpoint.
- [X] 4.3G Retain failed development captures and distinguish an existing-output refusal/shutdown fault from the later successful fresh native loader runs. No legacy TTO or unrelated suite pass is inferred.

### Local Mandatory Reading - Ground acceptance runner publication

_Applies to Group5 when dispatched by master11.1._

Read this plan's Global/Fixture and build readiness Area, only `src/tests/multilayer/acceptance/native_ground.py` public exports and fixed fixture definitions, `src/tests/multilayer_acceptance.py` refusal checks, `src/tests/multilayer_acceptance-1.py` actual loaders, and the Ground runner evidence section in [the finalization ledger](portal-tbm/r2mlvs3mp-z-a-finalization-ledger.md). Actual serialized ground owners come from `tbm_logistics_save_fixture.owned_chunks`/`saved_table`; exact selected art verification comes from `media/multilayer/review_m1_art.py`. These sources do not establish real bootstrap or host/client join.

Context has compacted 1 times while executing this group.

### Group 5 - Implement the acceptance runner and evidence schema

{{HARD}}
- [X] 5.1 Publish repository-local `src/tests/multilayer/acceptance/native_ground.py` with `run_scenario`, `capture_ledger`, `compare_state_digest`, `validate_art_manifest`. Define inputs, scenario IDs, fixture/content hashes, exit status and artifact schema.
- [X] 5.2 Wrap the verified build/game/test commands without silently changing a user config or source save. Bound runs and preserve stdout/stderr plus exact commands for diagnosis.
- [X] 5.3 Capture canonical per-resource/per-owner ledgers and r2mlvs15 matching-frame digests; validate actual runtime artifacts rather than synthesizing expected pass output.
- [X] 5.4 Test one successful evidence record, a failed command, a missing required artifact and a shortcut-marked fixture rejected for bootstrap acceptance. Missing live producers stay blocked, not passed.

Ground publication,2026-10-08: Group5 delivers the runner infrastructure for S1/S2 measured native economics, S8 actual owner failure boundaries and S9 actual saved full-loop continuation. Versioned records retain declared fixture shortcuts and actual command/artifact identities; unavailable scenarios refuse. [Evidence/schema/commands](portal-tbm/r2mlvs3mp-z-a-finalization-ledger.md#ground-runner-evidence) include real successful outputs, six independent refusal tests and equal headless/hidden owner+clock digests. These complete master11.1 preparation, not master11.3 real join or11.4 normal-bootstrap acceptance.

### Area Mandatory Reading - Playable loops and failure recovery

_Applies to Groups 6-15._

**Entry gate.** The following public contracts must exist before dependent implementation in this area: `src/tests/multilayer/acceptance/native_ground.py`: `run_scenario`, `capture_ledger`, `compare_state_digest`, `validate_art_manifest`.
Verify each public file/export/schema field with `rg` in the current workspace and inspect its command/field ownership. For a same-plan contract, compile its type/signature fixture or validate its artifact after the preceding producer groups. For cross-plan behavior, also run the producer's focused executable boundary test; declarations alone cannot authorize dependent mutations or runtime claims.
If a required export is absent, stop the dependent implementation, mutation and runtime claims in Groups 6-15. Read-only characterization, source/range verification, test-case design and asset-template work that does not assume the missing behavior may continue.
Do not bypass this gate by copying private pool/tile state, inventing local IDs or cargo registries, using a historical plan as a delivered implementation, opening user-wide content stores, substituting a second utility/accounting authority, or adding an unreviewed fallback path. Finish the named producer or revise the shared contract and its direct consumers first.

| File name | Line Ranges | Description |
| --- | --- | --- |
| [src/network/network_command.cpp](../../../../src/network/network_command.cpp) | 256-288 | `NetworkExecuteLocalCommandQueue` executes agreed frame order/company authority; all jobs use deterministic simulation time. |
| [src/cargopacket.h](../../../../src/cargopacket.h) | 458-489 | `VehicleCargoList::Append`, `AgeCargo`, `Stage`, `Return`, `Unload`, `Shift` move accountable packets; exchange/descent/processing must not clone packet age or feeder shares. |
| [src/cargopacket.h](../../../../src/cargopacket.h) | 596-603 | `StationCargoList::Reserve`, `Load`, `Truncate`, `Reroute` provide bounded station moves and destination reservations; hub inventory has one physical owner. |
| [src/economy_base.h](../../../../src/economy_base.h) | 20-38 | `CargoPayment`, `PayTransfer`, `PayFinalDelivery` own transport income. Visiting an origin station, a descent, or handling transfer is not automatic final payment. |
| `src/tests/multilayer/acceptance/native_ground.py` (planned public contract; link after implementation) | Live definitions; no fixed range | [r2mlvs16-integrated-acceptance-and-balance.md](r2mlvs16-integrated-acceptance-and-balance.md) Groups 1-5, ending with Group 5's public-contract task, publish `run_scenario`, `capture_ledger`, `compare_state_digest`, `validate_art_manifest`. These exports/schema fields are authoritative for this area. Same-plan consumers follow those earlier groups; cross-plan consumers verify the producer's required implementation/tests as well. Proposed names alone do not prove delivery. |

**ASSUMPTION: this was already built previously. If not, re-asses at runtime**. This applies only to the producer contracts named in this area's gate, after their listed producer groups have run. Verify their actual exports and focused tests before using the sketches below; proposed types and helper names in sketches are not evidence of delivered code.

### Group 6 - Playable loops and failure recovery

Goal finalization exception GP-BT: whole normal-bootstrap S1/S2 first acceptance is skipped under the user's multi-group-gap permission, as [the final audit](portal-tbm/r2mlvs3mp-z-b-final-closure-audit.md#gp-bt-genuine-normal-newgame-bootstrap-acceptance) explains. Delivered physical fixtures retain their declared setup and refuse bootstrap claims. Tube/plant final-use uses GP-TG.

{{MEDIUM}}
- [ ] 6.1 Execute S1/S2 as the first acceptance milestone.
- [X] 6.1G Execute delivered ground physical S1/S2 invoice/movement/exchange/disposal fixtures and retain their isolated-boundary classification, actual controls and current source hashes. Full bootstrap remains6.1.

### Group 7 - Playable loops and failure recovery

{{SIMPLE}}
- [ ] 7.1 Record excavation progress, generated material, wagon IDs, packets, station acceptance and total costs over several cycles.
- [X] 7.1G Record actual ground controller/cargo/stock/invoice owners over multiple source-side exchanges and final disposal, including current48-owner serialized comparisons and balanced full project costs.

### Group 8 - Playable loops and failure recovery

{{MEDIUM}}
- [ ] 8.1 Execute S3/S4 with all ten distinct cargos and the complete freight fleet.
- [ ] 8.2 Confirm no receiver, terrain or capacity dead end makes any required resource decorative.
- [ ] 8.3 Execute S5/S6 through normal controls, including invalid launch placement and mirror conflicts.
- [ ] 8.4 Verify real Surface collection and useful power accounting, not only animations/UI counters.
- [ ] 8.5 Execute S7 from Surface supplies without cheat-built orbital predecessors.
- [ ] 8.6 Check exact module/robot/processor inventories and outputs.

### Group 9 - Exercise lifecycle scenarios by feature family

{{HARD}}
- [ ] 9.1 Run S8 against the r2mlvs15 ground lifecycle matrix: lost origin/bay, full wagons/receivers, insufficient funds, cancellation and takeover. Capture the first failing command and retained cargo owner.
- [ ] 9.2 Run S8 for launch/probe/module/mirror target loss, full storage and ID reuse; then delivered tube/thermal-link loss. Use one failure trigger at a time before combining simultaneous projects.
- [ ] 9.3 Keep each failure reproducible from its pre-event save and require a demonstrated recovery or explicit safe refusal, not only an absence of crash.
- [X] 9.1G Exercise actual ground origin/bay loss, capacity/funds/receiver waits, cancellation and takeover through public owners; retain native current pre-event/recovery files and actual cargo ownership.
- [X] 9.3G Reproduce current serialized ground origin loss/cancellation and exact recovery/retreat/Surface-disposal outcomes; retain scoped owner refusals for other isolated conditions.

Group9 scope: missing tube/thermal loss in9.2 is skipped under GP-TG; normal-newgame scenario provenance uses GP-BT. Orbital target loss remains outside this master. [Ground lifecycle](../../../../logs/r2mlvs3mp-z-b-ground-lifecycle-checks.log) and [control/recovery](../../../../logs/r2mlvs3mp-z-b-ground-control-recovery-checks.log) are real owner tests, not complete whole-profile S8.

### Group 10 - Exercise save, hidden-layer and real join scenarios

Goal finalization exception GP-NJ:10.3 real join is skipped with the new harness/trace/scenario groups documented in the final audit. Tube/thermal scopes use GP-TG; orbital scopes remain pending. Replay and existing headless/hidden native evidence never count as join.

{{HARD}}
- [ ] 10.1 Run S9 mid-action save/reload for each queue/transaction boundary from the feature matrix and compare next-tick ledgers.
- [ ] 10.2 Run long hidden-layer operation and simultaneous projects with matching visible-layer reference runs; compare canonical state at the same simulation ticks.
- [ ] 10.3 Perform a real host/client join during active ground/orbital work and compare subsequent matching-frame digests. Include later tube/geothermal state; replay alone is not join evidence.
- [X] 10.1G Compare actual current mid-bend, capacity-wait, spanning, failure/cancellation and electrical saved-owner transitions after real application reload/reload-again.
- [X] 10.2G Retain actual M1 source/depot visible/hidden/headless loops and their matching saved state/clock evidence. Unselected simultaneous whole-profile work remains pending.

### Group 11 - Playable loops and failure recovery

{{SIMPLE}}
- [ ] 11.1 Capture player-facing confusing/blocked states and fix their controls/messages.

### Group 12 - Playable loops and failure recovery

{{MEDIUM}}
- [ ] 12.1 Confirm commands, scripts and shortcuts cannot bypass hidden toolbar restrictions.
- [ ] 12.2 Run straight and advancing 45-degree cutting cases plus a 90-degree change formed by two bends.
- [ ] 12.3 Run illegal single-step, stationary-pivot and corner-clipping refusal cases.
- [ ] 12.4 Run safe TBM backing and recovery after interrupted turns.
- [ ] 12.5 Clear a large irregular tube/plant footprint, then withdraw and install its final use.
- [ ] 12.6 Reconcile complete swept-space excavation, support cost and Earth/Rock totals.
### Local Mandatory Reading - Completed ground geometry/control subset

_Applies to Group12B; supplement Playable loops Area._

Read only the current turn/control/recovery logs and Plan4 Group21/Plan15 Group13B actual serialized bend evidence. Original whole-profile Group12 stays pending where final-use/bootstrap obligations do not pass. This completed subset is separate to retain the eight-task group limit.

### Group 12B - Completed ground geometry and controls

{{MEDIUM}}
- [X] 12.1G Verify ground synchronized command/control/script parity and hidden construction/date/role restrictions through actual producer tests.
- [X] 12.2G Verify actual straight and advancing45-degree bends, including90 degrees formed by two bends.
- [X] 12.3G Verify sharp/stationary/corner-clearance refusals before cut/accounting mutation.
- [X] 12.4G Verify real supported backing/cancellation recovery, including current native serialized partial-bend retreat.
- [X] 12.6G Retain exact ground swept/support/spoil/cost accounting and real packet ownership across these physical scenarios.

Group12 goal exception GP-TG:12.5 actual tube/plant final use is skipped with its absent consumers. Remaining ground evidence passes [6,021 geometry assertions/nine cases](../../../../logs/r2mlvs3mp-z-b-ground-turn-checks.log) and [136,634 movement/control/script/recovery assertions/16 cases](../../../../logs/r2mlvs3mp-z-b-ground-control-recovery-checks.log), with actual current Plan15 serialized proof. Whole mixed scenario claims retain pending boxes.

### Group 13 - Playable loops and failure recovery

{{HARD}}
- [ ] 13.1 Run valid orbital release offsets (20,0) and (14,14) and invalid offsets (21,0) and (15,15).
- [ ] 13.2 Run fractional boundary crossings, map edges and overlapping authorized target centers.
- [ ] 13.3 Run full recovery capacity and verify rejected release leaves cargo on the craft.
- [ ] 13.4 Collect cargo through separate train and lorry members and compare packet age/origin/amount ledgers.
- [ ] 13.5 Expand, unlink and delete a member during handling and exclude same-XY Underground members.

### Group 14 - Playable loops and failure recovery

{{MEDIUM}}
- [ ] 14.1 Run a useful Mole-70 bulk route with its final direction/load graphics.
- [ ] 14.2 Run a useful Crate-75 container route with its final direction/load graphics.
- [ ] 14.3 Run a useful Cistern-80 liquid route with its final direction/load graphics.
- [ ] 14.4 Run a useful Shield-85 sealed-mineral route with its final direction/load graphics.
- [ ] 14.5 Run a useful Forge-05 heavy-component route with its final direction/load graphics.
- [ ] 14.6 Verify paid-grid bootstrap, later geothermal supply and each introduction boundary.
- [ ] 14.7 Verify solid/Surface/Space/locked-Deep construction refusals and ordinary rail/portal incompatibility.
- [ ] 14.8 Save/reload capsule queues and compare cargo/reservation/power ledgers.

### Group 15 - Playable loops and failure recovery

{{MEDIUM}}
- [ ] 15.1 Execute S12 across the proposed calendar dates using real supply chains.
- [ ] 15.2 Demonstrate one vent shared between generators/direct smelting without excess heat use, later battery/forge production, useful Surface export and no solar-mirror or native/ETI double credit.

### Area Mandatory Reading - Economy and performance

_Applies to Groups 16-26._

**Entry gate.** The following public contracts must exist before dependent implementation in this area: `src/tests/multilayer/acceptance/native_ground.py`: `run_scenario`, `capture_ledger`, `compare_state_digest`, `validate_art_manifest`.
Verify each public file/export/schema field with `rg` in the current workspace and inspect its command/field ownership. For a same-plan contract, compile its type/signature fixture or validate its artifact after the preceding producer groups. For cross-plan behavior, also run the producer's focused executable boundary test; declarations alone cannot authorize dependent mutations or runtime claims.
If a required export is absent, stop the dependent implementation, mutation and runtime claims in Groups 16-26. Read-only characterization, source/range verification, test-case design and asset-template work that does not assume the missing behavior may continue.
Do not bypass this gate by copying private pool/tile state, inventing local IDs or cargo registries, using a historical plan as a delivered implementation, opening user-wide content stores, substituting a second utility/accounting authority, or adding an unreviewed fallback path. Finish the named producer or revise the shared contract and its direct consumers first.

| File name | Line Ranges | Description |
| --- | --- | --- |
| [src/industry_cmd.cpp](../../../../src/industry_cmd.cpp) | 1152-1240 | `ProduceIndustryGoodsHelper` and `ProduceIndustryGoods` expose complete rate/callback production paths; augment output once before authoritative monthly accounting. |
| [src/timer/timer_game_calendar.h](../../../../src/timer/timer_game_calendar.h) | 15-39 | `TimerGameCalendar` technology-clock contract and date conversions govern 1980/model/factory introductions; economic/realtime scheduling cannot unlock a technology. |
| [src/rail.h](../../../../src/rail.h) | 178-182 | `powered_railtypes` and `compatible_railtypes` have distinct invariants; ordinary/TBM/tube locomotion needs explicit power and compatibility checks. |
| `src/tests/multilayer/acceptance/native_ground.py` (planned public contract; link after implementation) | Live definitions; no fixed range | [r2mlvs16-integrated-acceptance-and-balance.md](r2mlvs16-integrated-acceptance-and-balance.md) Groups 1-5, ending with Group 5's public-contract task, publish `run_scenario`, `capture_ledger`, `compare_state_digest`, `validate_art_manifest`. These exports/schema fields are authoritative for this area. Same-plan consumers follow those earlier groups; cross-plan consumers verify the producer's required implementation/tests as well. Proposed names alone do not prove delivery. |
| [src/framerate_type.h](../../../../src/framerate_type.h) | 100-119 | Complete `PerformanceAccumulator` contract defines summed multi-step measurements and explicit reset boundaries; distinguish simulation cost from rendering and reset once per measured frame/step. |

**ASSUMPTION: this was already built previously. If not, re-asses at runtime**. This applies only to the producer contracts named in this area's gate, after their listed producer groups have run. Verify their actual exports and focused tests before using the sketches below; proposed types and helper names in sketches are not evidence of delivered code.

**Measured tuning workflow.** Define the comparison workload and objective, record the baseline ledger/profile, change one canonical parameter or bounded-work implementation, and rerun only the affected scenario plus relevant regression. Prices, capacity and throughput may be tuned; the required circle radius, turn rule, five distinct models and solar multiplier remain invariant.

### Group 16 - Measure complete ground project economics

{{HARD}}
- [ ] 16.1 Create a ledger schema with time basis, initial investment, active/idle operating cost, support, haulage, disposal, accepted output and net balance. Record all quantities and bottlenecks from fixed seeded runs.
- [ ] 16.2 Measure a standard corridor/chamber and a harder Deep project through several real wagon cycles. Separate one-time build cost from repeated operating expense and include receiver saturation delays.

**R2MLVS3MP note, 2026-10-05.** The solid starter charges the whole 3x3 pocket in cut 0 (12 cells, not 3). This supersedes the cleared-area assumption in [r2mlvs1-e](r2mlvs1-e-ground-and-space-budgets.md) line 32. Measure the larger first-cut cost and the minimum consist it implies (two Earth and two Rock hoppers on the soft-ground fixture site) ([r2mlvs3mp-f](portal-tbm/r2mlvs3mp-f-solid-starter-contract.md#capacity-consequence)).
**R2MLVS3MP handoff, 2026-10-05 (r2mlvs4 Group 20).** First-cut measurements from `[tbm-measure]` (Debug, fixture site, 1980, base currency units): standard cutter with 2 Earth/2 Rock hoppers, purchase 400,000, first cut 888 ticks, charge 360, 216 t of spoil, running cost 3,945 over those ticks; Deep cutter with 2 Earth/4 Rock hoppers, purchase 800,000, first cut 1,184 ticks (1,776 work ticks at 3/2 per tick), charge 960, 504 t, running cost 11,397. Whole-corridor time with real haulage and disposal follows the movement work (R2MLVS3MP Group 7B). These are inputs, not a balance verdict.
**R2MLVS3MP handoff, 2026-10-06 (r2mlvs4 20.1 complete).** [Complete project ledger](r2mlvs4-t-complete-project-ledger.md): medium native prices, unit inflation, seed 420, calendar fixed at 1980-01-01, actual 74-tick stopped startup. Standard/Deep both cut 36 swept cells in nine cuts and physically deliver all spoil through two/three source exchanges. Standard takes 10,620 ticks with setup 2,101,454, TBM purchase 403,124, cut/support 1,080, active 38,176, idle 39, haulage 952 and receiver fees 1,728: total 2,546,553. Deep takes 32,926 ticks with setup 2,106,196, purchase 804,686, cut/support 2,880, active 232,758, idle 151, haulage 2,964 and fees 4,464: total 3,154,099. Every component balances against actual native cash/expense receipts. All packets reach real Surface receivers. This is producer evidence for 16.1/16.2; the required acceptance runner, saturation cases, payback, complete route and performance/art proofs still govern those pending tasks. No tuning change or whole-acceptance verdict is claimed.
- [ ] 16.3 Measure each mineral-family route from finite reserve through Surface processing and final useful output. Compare like-for-like distances/capacities and document missing consumers as failures.
- [ ] 16.4 Calculate payback/sensitivity from observed complete loops, not nominal vehicle price. Send proposed numerical changes to r2mlvs1 with the specific failed balance objective.
- [X] 16.1G Publish schema2 actual ground invoices with time/seed/date, full setup/purchase/work/support/active/idle/haul/disposal/material/remaining-cash ownership, sourced from actual paid loops.
- [X] 16.2G Verify standard/Deep corridor rows through two/three actual physical wagon exchanges and real Surface disposal; retain independent actual bounded saturation/resume evidence. Mineral/future whole-chain and detailed sensitivity rows remain pending.

Ground economics closure,2026-10-08: [current Debug invoice record](../../../../logs/multilayer-acceptance-s1-final.json) verifies the canonical standard/Deep economics fixture. Standard10,620 ticks/2,546,553 and Deep32,926/3,154,099 retain their exact seven native invoice categories and216/432 versus72/1,440 tonnes Earth/Rock. All packets finish at actual receivers, train spoil revenue is zero, and company cash balances. [Receiver fee/no-income/saturation/recovery](../../../../logs/r2mlvs3mp-z-b-spoil-no-profit-saturation-checks.log) passes67,045 assertions/nine cases. The full16.2 saturation-cost sensitivity and16.3/16.4 whole mineral payback remain beyond this corridor subset; no fictional output revenue or fixture-bootstrap verdict is credited.

### Group 17 - Measure orbital launch and manufacture economics

{{HARD}}
- [ ] 17.1 Measure first rocket/probe deployment and several mined return/service cycles, including pad, drop recovery, Surface handling and receiver demand limits.
- [ ] 17.2 Measure first Core/Dock/Assembly/heavy robot/processor from Surface supplies; separate setup investment, repeated extraction throughput and returned product value.
- [ ] 17.3 Compare small probe versus heavy robot bottlenecks and sustainable payload/service budgets. Record blocked intervals and why a larger station improves a usable earlier loop.

### Group 18 - Economy and performance

{{MEDIUM}}
- [ ] 18.1 Confirm active TBM costs are consequential, Deep access is meaningfully harder, spoil alone cannot finance perpetual boring and disposal saturation creates manageable logistics. Tune defaults and publish changed tables in r2mlvs1.
- [ ] 18.2 Check mining throughput against launch/pod/dock capacities and receiver demand. Ensure small probes are useful before heavy robots, and large stations improve throughput without making the first launch unprofitable by design.
- [X] 18.1G Confirm ground active/idle costs are positive and consequential, Deep is slower/costlier for equal swept scope, actual spoil delivery charges fees with zero transport revenue, and bounded receiver saturation resumes physically without destroying cargo. No canonical tuning changes are justified by these passing objectives.

### Group 19 - Tune mirror costs while preserving the required multiplier

{{HARD}}
- [ ] 19.1 Run equal-seed no-mirror/one-mirror fixtures with identical baseline production, demand and duration; verify 100 -> 400, nonstacking and zero-baseline behavior before any balance change.
- [ ] 19.2 Measure kit manufacture, launch, service and useful consumed-energy benefit. Distinguish generated excess from actually consumed power so payback is not credited for discarded demand.
- [ ] 19.3 Tune only declared costs, upkeep and proposed coverage values; keep +300% unchanged. Update the canonical tuning table and rerun the affected solar/bootstrap scenario with before/after ledgers.

### Group 20 - Economy and performance

{{SIMPLE}}
- [ ] 20.1 Document any native grid/town-growth effect and ETI profile limitation.

### Group 21 - Set reproducible performance workloads and budgets

Goal finalization exception GP-PF: complete compact/representative/max-map workload authoring, feature-disabled process baselines/peak-memory attribution, agreed-tick latency/save/load instrumentation and stressed hot-path diagnosis require more than two new groups. These full21.1-21.3 rows remain unchecked. The delivered account-bound subset is Group21B; real join budgets independently use GP-NJ.

{{MEDIUM}}
- [ ] 21.1 Record hardware/build/configuration, fixed simulation duration and workload counts for compact, representative and maximum feasible maps. Define warmup, repeated-sample policy and tick/command latency percentiles.
- [ ] 21.2 Measure feature-disabled baseline overhead and memory before selecting acceptance budgets. State the allowed incremental memory, tick time, pathfinding and command latency for each workload.
- [ ] 21.3 Add save size/save-load duration and real join reconstruction budgets with the same content/feature counts. Record measurement uncertainty rather than comparing unrelated machines/runs.

### Local Mandatory Reading - Delivered utility bound measurement

_Applies to Group21B; independent bounded-owner subset of master11.6._

Read Global/Economy and performance Area, `src/tests/utility_power_performance.cpp`, utility source/consumer/connection limits and actual allocation/retirement APIs. Benchmark real sparse0/32/256/4,096-account ledgers, one warmup and five equal intervals, separately from setup/expiry/rendering. Nanoseconds are performance observations, never simulation time. Report retained payload bytes separately from unmeasured RSS/temporary peaks. This cannot establish representative/max-map, dense simultaneous-job, route-planning or real join budgets.

### Group 21B - Measure the delivered utility owner bound

{{MEDIUM}}
- [X] 21B.1 Define and run fixed bounded real source/consumer workloads in Debug and optimized configurations, preserving quantity/lifetime/retirement correctness.
- [X] 21B.2 Record hardware/configuration, warmup/sample policy, allocation median/maximum, setup/retirement time and retained payload estimate with explicit uncertainty and unmeasured scopes.

21B closure,2026-10-08: Ryzen7 PRO5850U, eight cores/16 threads,29,878,575,104 bytes physical memory; current repository Debug/RelWithDebInfo executables. Real0/32/256/4,096 sparse account/edge workloads use one full-interval warmup and five equal allocation samples, with source publication/expiry outside timing. Both configurations pass78,980 assertions/one generated case. [Debug](../../../../logs/r2mlvs3mp-z-b-utility-performance-debug.log) max-account median/max are37.024/38.115ms; [optimized](../../../../logs/r2mlvs3mp-z-b-utility-performance-optimized.log) are3.822/3.996ms, setup10.035ms and retirement0.437ms. Optimized retained payload estimate is1,327,216 bytes; allocator/RSS/temporary peaks are unmeasured. Empty-account optimized allocation median is300ns, a subsystem baseline only. Against the native27ms timer opportunity the optimized sparse bound fits; Debug instrumentation exceeds it and is reported, not silently called a budget pass. Five samples cannot establish robust tail percentiles. No rendering, real dense-job/map/path workload or join profile is claimed; no production tuning/optimization follows these passing sparse-bound results.

### Group 22 - Attribute performance cost before optimizing

Goal finalization exception GP-PF: full22.1-22.3 peak ownership attribution, representative-map active-job/path/recipe profiling and budget-exceedance optimization are skipped for this goal under the documented multi-group gap.21B actual sparse utility measurements remain valid limited evidence; they do not complete these rows.

{{HARD}}
- [ ] 22.1 Measure packed terrain, sparse geology/jobs, entity indices, packet inventories and utility graph memory separately, including peak generation/load allocations.
- [ ] 22.2 Measure active-job/entity updates, pathfinding/approach search, Surface handling and recipe allocation separately from rendering. A hidden layer must not skip simulation work to make a benchmark pass.
- [ ] 22.3 Capture a profile for each exceeded budget and identify the owning hot path; fix one bounded-work issue in its feature owner, then retest the affected workload and regression fixture.

### Group 23 - Economy and performance

Goal finalization exception GP-PF: full dense excavation/orbital workload23.1 and related23.2 hot-path acceptance need the absent representative stress/instrumentation groups. Existing finite account/connection/share limits and measured sparse utility retirement are retained; no unmeasured dense-map performance pass is claimed.

{{MEDIUM}}
- [ ] 23.1 Profile dense excavation projects and orbital fleets.
- [ ] 23.2 Resolve unbounded tile scans, all-pairs rendezvous work, inventory growth and command stalls; retest only affected budgets after fixes.

### Group 24 - Compare five tube models with conventional freight

{{HARD}}
- [ ] 24.1 Use the same useful cargo, distance and delivery demand to measure each capsule role against compatible underground rail/road at its introduction date.
- [ ] 24.2 Include paid-grid cost, loaded/empty demand, terminal dwell, service and interchange time; verify five models retain distinct useful roles and do not bypass mandatory TBM wagon haulage.
- [ ] 24.3 Tune model capacities/costs within the canonical profile, then rerun the affected cargo/refit/power scenarios. Keep underground-only cleared construction and all five designs intact.

### Group 25 - Balance geothermal power and direct processing

{{HARD}}
- [ ] 25.1 Measure 1980/2000 generator investment and operating cost against explicit delivered construction inputs and actual useful electrical output.
- [ ] 25.2 Compare 2010/2030/2045 direct-heat recipes with Surface alternatives using the same transported materials, output demand and bounded vent heat budget.
- [ ] 25.3 Adjust proposed prices/efficiencies/throughput while retaining one shared heat allocation and a viable earlier Surface route. Re-run no-double-credit and first-plant/riser bootstrap cases after changes.

### Group 26 - Profile combined hubs, tubes and thermal graphs

{{HARD}}
- [ ] 26.1 Scale linked stations/handling jobs at fixed returned cargo demand; record dispatch and cancellation latency plus packet/reservation growth.
- [ ] 26.2 Scale tube vehicles/reservations and vent utility links/consumers independently, then combine with ground excavation/orbital workloads.
- [ ] 26.3 Fix bounded-work/caching issues in their owners and compare against the declared budgets. Tune handling costs/capacity or proposed dates only through the profile; retain the required 20-tile drop radius and 45-degree TBM limit.

### Area Mandatory Reading - Art, documentation and completion

_Applies to Groups 27-33._

**Raster source review.** Corrections requested by this area's art review go through Plan13's [framed ImageGen prompt and extraction checks](r2mlvs13-graphics-and-asset-pipeline.md#framed-imagegen-sheets-and-safe-extraction). For framed multi-view originals, require the retained prompt/references, measured non-overlapping crops, complete subject clearance and exclusion of separator frames from native sprites. Check hardware identity and required direction/state coverage separately. The framed trial passed extraction but failed model identity; neither its source preview nor another clean sheet can replace packed/decoded hashes and actual loaded runtime captures in Groups27-29.

**Entry gate.** The following public contracts must exist before dependent implementation in this area: `src/tests/multilayer/acceptance/native_ground.py`: `run_scenario`, `capture_ledger`, `compare_state_digest`, `validate_art_manifest`.
Verify each public file/export/schema field with `rg` in the current workspace and inspect its command/field ownership. For a same-plan contract, compile its type/signature fixture or validate its artifact after the preceding producer groups. For cross-plan behavior, also run the producer's focused executable boundary test; declarations alone cannot authorize dependent mutations or runtime claims.
If a required export is absent, stop the dependent implementation, mutation and runtime claims in Groups 27-33. Read-only characterization, source/range verification, test-case design and asset-template work that does not assume the missing behavior may continue.
Do not bypass this gate by copying private pool/tile state, inventing local IDs or cargo registries, using a historical plan as a delivered implementation, opening user-wide content stores, substituting a second utility/accounting authority, or adding an unreviewed fallback path. Finish the named producer or revise the shared contract and its direct consumers first.

| File name | Line Ranges | Description |
| --- | --- | --- |
| [src/table/sprites.h](../../../../src/table/sprites.h) | 380-409 | Complete Map Layer sprite constants and `SPR_NEWGRFS_BASE` bound the published block; preserve 0..65 and resolve any accepted live extension before allocating new IDs. |
| [src/map_layer_gui.h](../../../../src/map_layer_gui.h) | 67-73 | `GetVehicleDisplayLayer` is the public visible/pickable-layer contract; keep ordinary aircraft/ships/disasters on Surface and place orbital units explicitly. |
| `src/tests/multilayer/acceptance/native_ground.py` (planned public contract; link after implementation) | Live definitions; no fixed range | [r2mlvs16-integrated-acceptance-and-balance.md](r2mlvs16-integrated-acceptance-and-balance.md) Groups 1-5, ending with Group 5's public-contract task, publish `run_scenario`, `capture_ledger`, `compare_state_digest`, `validate_art_manifest`. These exports/schema fields are authoritative for this area. Same-plan consumers follow those earlier groups; cross-plan consumers verify the producer's required implementation/tests as well. Proposed names alone do not prove delivery. |

**ASSUMPTION: this was already built previously. If not, re-asses at runtime**. This applies only to the producer contracts named in this area's gate, after their listed producer groups have run. Verify their actual exports and focused tests before using the sketches below; proposed types and helper names in sketches are not evidence of delivered code.

**Art evidence gate. ASSUMPTION: this was already built previously. If not, re-asses at runtime**. The complete r2mlvs13 manifest, packed assets and required feature state transitions exist before S10 capture. Verify their hashes/tests; missing families remain explicitly pending and prevent final closure, while already delivered family captures can be recorded independently.

Context has compacted 1 times while executing this group.

### Group 27 - Build the runtime art coverage matrix

{{MEDIUM}}
- [ ] 27.1 Expand r2mlvs13 manifest entries into required direction/state/zoom/blitter captures and map each to a real S10 runtime scene plus asset hash.
- [ ] 27.2 Capture excavation/portals/TBM/exchange and ten mines/conventional fleet first, then five tube models and each geothermal era. Use actual animation/load/service states.
- [ ] 27.3 Capture launch/probe/asteroids/descent, module construction/processing and mirrors, including extra viewports and separate Surface/orbital overlays.
- [X] 27.1G Map the selected forty-family M1 manifest to retained native runtime scenes, source/distribution hashes and explicit renderer-query versus driven-frame coverage.
- [X] 27.2G Verify the delivered geology/portal/TBM/wagon/exchange/receiver/service/control slice through the published 157-frame native gallery.

Ground closure, 2026-10-08: [technical gallery](portal-tbm/r2mlvs3mp-t-gallery-acceptance.md) and [fresh hash/corruption verification](../../../../logs/r2mlvs3mp-z-b-final-art-verification.log) pass forty families/2,311 sprites/157 actual native frames. Original later mine/fleet/tube/geothermal rows remain pending under master GP-TG; orbital rows are outside this queue. No template or renderer direction query is presented as physical motion.

### Group 28 - Review occlusion and distributed art coverage

{{HARD}}
- [ ] 28.1 Inspect full-site anchors, tile seams, vehicle foreground sorting, tube-shell visibility and moving-object picking across required orientations and supported blitters.
- [ ] 28.2 Compare native and scaled zoom captures to the manifest, verify every gameplay-critical state has a real loaded asset, and fix incomplete/misleading art through r2mlvs13.
- [ ] 28.3 Recheck packed/distributed hashes after corrections and retain before/after runtime evidence. Record user art feedback separately; automated coverage cannot stand in for visual approval.
- [X] 28.1G Retain selected native whole-site/portal crossing/coupling/body-picking/transparency evidence and fixed source anchors.
- [X] 28.2G Verify selected native/scaled scenes and independent complete packed/source coverage against actual loaded assets.
- [X] 28.3G Recheck unchanged selected source/frame/distribution hashes; retain personal feedback as deferred.

Goal exception: the user authorized skipping human operator gates. Personal art feedback is deferred, not supplied or approved. Selected technical review is complete through Group85B of Plan13; unselected interop art remains GP-TG and prevents full-profile completion of 28.1-28.3.

### Group 29 - Art, documentation and completion

{{SIMPLE}}
- [ ] 29.1 Record user art review separately from automated checks.
- [ ] 29.2 Update delivered architecture: layers/rules, vehicles, industries/cargos, command boundaries, new save chunks/migrations, orbital simulation, content compatibility and asset packaging.
- [X] 29.1G Record the deferred M1 personal-review status separately from completed technical acceptance.

Goal exception: human personal review is skipped for finalization under the user's explicit operator-gate permission. Original29.1 remains unchecked; no visual approval is inferred from technical checks. Later-family architecture remains with its actual producers.

### Group 30 - Art, documentation and completion

{{MEDIUM}}
- [ ] 30.1 Keep proposal history in plans.
- [ ] 30.2 Add player instructions for commissioning, 45-degree area clearing, actual wagon exchange, all ten resource chains, 1980 Deep access, five-model tube transport, geothermal timelines, launch collection membership, 20-tile cargo drops, mirrors and station delivery. Explain recovery without exposing irrelevant implementation details in the UI.
- [X] 30.2G Update the delivered portal/TBM/area/exchange/Deep player path and concrete supported recovery in [the player guide](r2mlvs-how-to.md); label fixture-only acceptance and absent later consumers.

Ground scope excludes original tube/geothermal guidance under GP-TG and unrelated orbital guidance; no instruction implies those consumers exist. Proposal history remains retained in its owning plans.

### Group 31 - Art, documentation and completion

{{SIMPLE}}
- [ ] 31.1 Publish an evidence matrix mapping every master requirement/invariant to exact scenario/test/log/artifact. List all unresolved limitations and unsupported content profiles honestly.
- [X] 31.1G Publish this master's target requirement/invariant/evidence/exception matrix in [the final audit](portal-tbm/r2mlvs3mp-z-b-final-closure-audit.md), including exact remaining target tasks and unsupported runtime claims.

### Group 32 - Art, documentation and completion

{{MEDIUM}}
- [ ] 32.1 Close each child only when its completion conditions and required checks are satisfied, then reconcile the master index. An explicitly authorized skip remains recorded as unverified and does not imply runtime, network or art acceptance.
- [X] 32.1G Apply the target closure policy: completed supporting Plan2/5 contracts can close; Plans3/4, M1 and unrelated milestones remain incomplete because authorized skips do not satisfy engineering conditions.

### Group 33 - Art, documentation and completion

{{SIMPLE}}
- [ ] 33.1 Record the implemented public exports, focused test commands/results and remaining runtime/art/network gaps in this plan; retain original supplied saves and repository-local evidence.
- [ ] 33.2 Update affected architecture/handoff documentation to describe delivered behavior and reconcile the parent index only when this plan's Completion Conditions are met. Keep every unfinished task unchecked.
- [X] 33.1G Record delivered ground exports, focused results, protected source inputs and explicit limitations in the final audit and player guide.
- [X] 33.2G Reconcile delivered ground/electrical architecture and truthful parent progress; retain original full-profile closure tasks unchecked.

## Consult on Demand

| Trigger | File name | Start at | Expand only when | Consultation purpose |
| --- | --- | --- | --- | --- |
| A combined run reports no desync but gameplay ledgers differ | [../../../../src/network/network_server.cpp](../../../../src/network/network_server.cpp) | `ServerNetworkGameSocketHandler::SendSync` | Command order and RNG state agree while cargo/geology/orbital digests disagree | Confirm the RNG-only sync scope and compare explicit feature ledgers; no-desync output cannot replace full state equality. |
| A required public export is absent and its owner/location is disputed | [r2mlvs-multilayer-vehicles-and-structures.md](r2mlvs-multilayer-vehicles-and-structures.md) | `Requirements and ownership` and `Plan Index` | The named producer confirms an intentional contract/ownership change | Find the responsible producer and reconcile its public handoff with direct consumers; the hub cannot substitute for missing implementation. |
| Current behavior contradicts a historical subsystem description | [../../architecture/map-layers/archi-map-layers.md](../../architecture/map-layers/archi-map-layers.md) | `Invariants` and the relevant named source in `Code map` | The live symbol and executable test do not explain the difference | Locate layer addressing or lifecycle rationale. Live code/tests govern delivered behavior; this architecture page does not prove future interfaces exist. |
| A configure/build invocation no longer resolves the recorded Windows toolchain or test target | [R2MLM build commands](r2mlm-multi-layered-map.md#build-and-test-commands) | `Build and Test Commands` | The current CMake target declarations and local configuration do not explain the failure | Recover the repository build invocation, then verify it against the live toolchain; historical success/counts are not current test evidence. |

## Planning Provenance

This is not an execution reading queue. Do not preload it after context compaction. It is non-transitive; ranges record planning use, not runtime authority.

| File name | Sections or Line Ranges used while planning | Planning contribution |
| --- | --- | --- |
| [docs/zz-reach2/architecture/initial-inventory.md](../../architecture/initial-inventory.md) | 185-220 | `Command as the only world mutation`, `Pool-indexed game objects`, `Tile map as a packed array` and `File-suffix modules` oriented command/pool/header ownership and source registration. This historical inventory is a location survey, not authority for current layer behavior. |
| [docs/zz-reach2/architecture/@archi-ttd-level0.md](../../architecture/@archi-ttd-level0.md) | 24-63 | `How the program fits together` and `Source and runtime boundaries` located shared simulation, data/presentation, CMake targets and subsystem handoffs used to choose the live reading sources. |
| [docs/zz-reach2/upgrades/2026-10/r2mlvs-multilayer-vehicles-and-structures.md](r2mlvs-multilayer-vehicles-and-structures.md) | 119-140 | `Requirements and ownership` assigned this plan's feature responsibilities; it shaped the decomposition without making every sibling plan a mandatory reading dependency. |
| [src/network/network_server.cpp](../../../../src/network/network_server.cpp) | 670-687 | The inspected live boundary helped separate characterization, public-contract production, implementation and acceptance batches. It materially constrained measured combined evidence without treating no-desync or art sheets as full acceptance; this entry records planning influence rather than an additional reading obligation. |
| [User planning-format instructions](<C:/Users/Axonn/.codex/attachments/cd59b20a-3a96-4d54-b4bd-1e8f37639e7d/Pasted text.txt>) | `Global Mandatory Reading`, `Area Mandatory Reading`, `Consult on Demand`, `Planning Provenance`, `Final validation` | Supplies the exact protocol, chronological difficulty batches of at most eight tasks, reading gates and validation format. |
| [src/framerate_type.h](../../../../src/framerate_type.h) | 100-119 | Deeper planning pass, 2026-10-04: Measurement reset/accumulation semantics shaped repeatable workload attribution and bounded optimization tasks. |

## Completion Conditions

All requested gameplay, five original tube vehicles, geothermal progression and the planned M4 expansion are demonstrably connected, supplied with final graphics and supported by save/network evidence. Calendar gates, TBM turn geometry and the two launch-center areas have boundary tests; no visual counter substitutes for real material transfer or heat allocation. Performance and balance claims have measurements; remaining limitations are explicit rather than hidden behind checked boxes.

Retained recurring-depot producer evidence, 2026-10-06: [Plan 5 Group 13B](r2mlvs5-spoil-wagons-and-haulage.md#group-13b---drive-recurring-underground-bay-exchanges) now supplies two actual depot exchanges and mandatory Surface disposal/empty return, with both paid spoil ledgers and native expense accounting. Two full native replays match all 63 physical/cargo/owner/cost transitions ([ei](../../../../logs/r2mlvs5-ei-complete-depot-trace-checks.log), 378,756 assertions / one case). This component evidence does not complete Plan 16's broader network, balancing or performance gates.
