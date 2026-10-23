> **Historical donor snapshot — not an execution plan.** OpenTTD3 `75ea75718f`, imported2026-10-23.
> Original: `docs/zz-reach2/upgrades/2026-10/r2mlvs16-integrated-acceptance-and-balance.md`; exact source SHA256 `c99365f1be7d15db9725fae0a41cab953f2413c524b9a3a85c68985db410dbff`.
> Current integration: [M2O23-1](../../../../../../../zz-reach2/upgrades/2026-10/m2o23-1-come-space.md); storage and evidence roles: [archi-historical-debug.md](../../../../../../../zz-reach2/architecture/archi-historical-debug.md). Original completion claims, instructions and human-review limits below remain dated; they grant no new execution, spending, publication or deletion authority.

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
| [src/map_layer.h](../../../../../../../../src/tests/fixtures/cleanup/source-epochs/29c86bf77e2c7b62-map_layer.h) | 78-85 | `MultiLayerMap` ownership preamble: `Map` owns packed storage/geometry and this controller owns layer semantics. Every group uses world tiles/explicit layer helpers; camera selection and ad hoc plane offsets cannot define gameplay. |
| [src/network/network_server.cpp](../../../../../../../../src/tests/fixtures/cleanup/source-epochs/openttd3-75ea75718f/6a45d374c71c770f-network_server.cpp) | 670-687 | `ServerNetworkGameSocketHandler::SendSync` compares RNG seeds, not a full gameplay checksum; validate orbital/geology/cargo ledgers in addition to a no-desync result. |

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
[Steel/Goods and probe-role contract](../../../../../../../../../OpenTTD3/docs/zz-reach2/upgrades/2026-10/r2mlvs1-d-recipes-and-bootstrap.md#space-economy-revision---2026-10-12)
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

Delivered ground owner: [src/tests/multilayer/acceptance/native_ground.py](../../../../../../../../src/tests/fixtures/cleanup/source-epochs/openttd3-75ea75718f/d60f9a238f270b04-native_ground.py), migrating under R2FC to `src/tests/multilayer/acceptance/native_ground.py`, publishing `run_scenario`, `capture_ledger`, `compare_state_digest` and `validate_art_manifest`. Schema2 records actual subprocess/artifact/configuration hashes, complete selected owner state and explicit isolated-fixture classification. Six refusal tests prevent false bootstrap/invalid-output claims. Later-family and real peer-join scenarios require their actual producers; no ground prefix completes the whole scenario register.

## Implementation Tasks

Ground execution scope: [R2MLVS3MP](r2mlvs3mp-master-plan.md) selects S1/S2, ground S8/S9, the related portal/tube/riser checks and ground art/economics/performance rows. Groups 1-5 must build the real acceptance runner for this explicitly selected family set without requiring all orbital producers listed in the shared entry gate. Each scenario requires its actual public producers, matching loaded assets and meaningful executable boundary tests; absent required producers block that scenario. Preserve the stricter complete producer set for combined S1..S12/whole-epic closure. A scoped ground pass does not complete a mixed-family task or this entire plan, and skipped/injected evidence never becomes bootstrap acceptance. The named ground portions of Groups 1-5 (1.1-G, 2.2-G, 3.3-G, 4.1-G) and the family-neutral tasks are listed in [r2mlvs3mp-e](../../../../../../../../../OpenTTD3/docs/zz-reach2/upgrades/2026-10/portal-tbm/r2mlvs3mp-e-scoped-gates.md#plan-16-groups-1-5).

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
| [src/tests/map_layer.cpp](../../../../../../../../src/tests/fixtures/cleanup/source-epochs/openttd3-75ea75718f/f2e0e6c6fa33cc70-map_layer.cpp) | 293-322 | Complete Deep initialization test checks fence/terrain and unchanged RNG; new geology must preserve feature-disabled behavior. |
| [src/tests/map_layer.cpp](../../../../../../../../src/tests/fixtures/cleanup/source-epochs/openttd3-75ea75718f/f2e0e6c6fa33cc70-map_layer.cpp) | 430-442 | Complete display-order test retains immutable Space=2 and Deep=3 storage meanings; future UI/profile work cannot reorder saved IDs. |
| [media/baseset/openttd/CMakeLists.txt](../../../../../../../../src/tests/fixtures/cleanup/source-epochs/openttd3-75ea75718f/26ddfd2fcd3975fb-CMakeLists.txt) | 6-67 | Complete `if(GRFCODEC_FOUND)` and `create_grf_command` build contract covers PNG/NFO inputs and checked-in fallback GRFs; final reviewed assets must reach both paths. |
| `src/multilayer_content.h` (planned public contract; link after implementation) | Live definitions; no fixed range | [r2mlvs1-gameplay-contracts-and-content.md](../../../../../../../../../OpenTTD3/docs/zz-reach2/upgrades/2026-10/r2mlvs1-gameplay-contracts-and-content.md) Groups 1-6, ending with Group 6's public-contract task, publish `MultiLayerGameplayProfile`, `LayerVehicleCapability`, `GetMultiLayerContentProfile`, `CheckDeepLayerAvailability`, `UtilityPowerAccount`. These exports/schema fields are authoritative for this area. Same-plan consumers follow those earlier groups; cross-plan consumers verify the producer's required implementation/tests as well. Proposed names alone do not prove delivery. |
| `src/excavation.h` (planned public contract; link after implementation) | Live definitions; no fixed range | [r2mlvs2-geology-and-excavation.md](r2mlvs2-geology-and-excavation.md) Groups 1-7, ending with Group 7's public-contract task, publish `ExcavationState`, `GeologyDeposit`, `ExcavationJob`, `CheckExcavatedFootprint`, `CommitExcavationCut`, `GeothermalVentField`. These exports/schema fields are authoritative for this area. Same-plan consumers follow those earlier groups; cross-plan consumers verify the producer's required implementation/tests as well. Proposed names alone do not prove delivery. |
| [src/layer_portal_cmd.h](../../../../../../../../src/tests/fixtures/cleanup/source-epochs/openttd3-75ea75718f/c9b88cad24d07722-layer_portal_cmd.h) | Live definitions; no fixed range | [r2mlvs3-sealed-portals-and-deep-rail.md](r2mlvs3-sealed-portals-and-deep-rail.md) Groups 1-7, ending with Group 7's public-contract task, publish `LayerPortalWorkState`, `GetLayerPortalWorkState`, `CheckLayerPortalWorkAccess`, `CommissionLayerPortalTBM`. These exports/schema fields are authoritative for this area. Same-plan consumers follow those earlier groups; cross-plan consumers verify the producer's required implementation/tests as well. Proposed names alone do not prove delivery. |
| `src/tbm.h` (planned public contract; link after implementation) | Live definitions; no fixed range | [r2mlvs4-tunnel-boring-machines.md](r2mlvs4-tunnel-boring-machines.md) Groups 1-5, ending with Group 5's public-contract task, publish `TBMProject`, `TBMWorkState`, `ValidateTBMAreaRoute`, `AdvanceTBMWork`. These exports/schema fields are authoritative for this area. Same-plan consumers follow those earlier groups; cross-plan consumers verify the producer's required implementation/tests as well. Proposed names alone do not prove delivery. |
| `src/tbm_exchange.h` (planned public contract; link after implementation) | Live definitions; no fixed range | [r2mlvs5-spoil-wagons-and-haulage.md](r2mlvs5-spoil-wagons-and-haulage.md) Groups 1-5, ending with Group 5's public-contract task, publish `TBMExchangeAssignment`, `ValidateTBMWagonExchange`, `ExecuteTBMWagonExchange`. These exports/schema fields are authoritative for this area. Same-plan consumers follow those earlier groups; cross-plan consumers verify the producer's required implementation/tests as well. Proposed names alone do not prove delivery. |
| `src/multilayer_industry.h` (planned public contract; link after implementation) | Live definitions; no fixed range | [r2mlvs6-mines-and-resource-chains.md](r2mlvs6-mines-and-resource-chains.md) Groups 1-6, ending with Group 6's public-contract task, publish `LayerIndustryCapability`, `ValidateMineDepositClaim`, `ProduceLayerMineCargo`, `PrepareLayerRecipeBatch`, `CommitLayerRecipeBatch`. These exports/schema fields are authoritative for this area. Same-plan consumers follow those earlier groups; cross-plan consumers verify the producer's required implementation/tests as well. Proposed names alone do not prove delivery. |
| `src/underground_gui.h` (planned public contract; link after implementation) | Live definitions; no fixed range | [r2mlvs7-underground-construction-and-fleet.md](r2mlvs7-underground-construction-and-fleet.md) Groups 1-5, ending with Group 5's public-contract task, publish `ShowUndergroundConstructionToolbar`, `BuildUndergroundVehicleFilter`. These exports/schema fields are authoritative for this area. Same-plan consumers follow those earlier groups; cross-plan consumers verify the producer's required implementation/tests as well. Proposed names alone do not prove delivery. |
| `src/space.h` (planned public contract; link after implementation) | Live definitions; no fixed range | [r2mlvs8-space-world-and-construction.md](r2mlvs8-space-world-and-construction.md) Groups 1-7, ending with Group 7's public-contract task, publish `SpaceVehicle`, `OrbitalBodyID`, `SpaceMission`, `SpaceStructure`, `SpaceTransition`. These exports/schema fields are authoritative for this area. Same-plan consumers follow those earlier groups; cross-plan consumers verify the producer's required implementation/tests as well. Proposed names alone do not prove delivery. |
| `src/launch_center.h` (planned public contract; link after implementation) | Live definitions; no fixed range | [r2mlvs9-launch-centers-and-rockets.md](../../../../../../../../../OpenTTD3/docs/zz-reach2/upgrades/2026-10/r2mlvs9-launch-centers-and-rockets.md) Groups 1-7, ending with Group 7's public-contract task, publish `LaunchCenterAnchor`, `LaunchCollectionHub`, `SurfaceHandlingJob`, `SpaceDescentJob`, `CanReleaseOrbitalCargo`, `CommitOrbitalCargoRelease`. These exports/schema fields are authoritative for this area. Same-plan consumers follow those earlier groups; cross-plan consumers verify the producer's required implementation/tests as well. Proposed names alone do not prove delivery. |
| `src/asteroid.h` (planned public contract; link after implementation) | Live definitions; no fixed range | [r2mlvs10-asteroids-and-mining-probes.md](r2mlvs10-asteroids-and-mining-probes.md) Groups 1-7, ending with Group 7's public-contract task, publish `AsteroidBody`, `AdvanceAsteroidField`, plus `SpaceVehicle`, `SpaceMission`, `AssignSpaceMiningMission` and `SpaceVehicle::Tick` in `src/space_vehicle.h`. These exports/schema fields are authoritative for this area. Same-plan consumers follow those earlier groups; cross-plan consumers verify the producer's required implementation/tests as well. Proposed names alone do not prove delivery. |
| `src/orbital_station.h` (planned public contract; link after implementation) | Live definitions; no fixed range | [r2mlvs11-orbital-stations-and-manufacturing.md](../../../../../../../../../OpenTTD3/docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md) Groups 1-6, ending with Group 6's public-contract task, publish `OrbitalModule`, `OrbitalConstructionJob`, `ReserveOrbitalConstructionInputs`, `CommitOrbitalConstructionStep`. These exports/schema fields are authoritative for this area. Same-plan consumers follow those earlier groups; cross-plan consumers verify the producer's required implementation/tests as well. Proposed names alone do not prove delivery. |
| `src/solar_power.h` (planned public contract; link after implementation) | Live definitions; no fixed range | [r2mlvs12-solar-farms-mirrors-and-eti.md](r2mlvs12-solar-farms-mirrors-and-eti.md) Groups 1-6, ending with Group 6's public-contract task, publish `SolarGenerationAccount`, `MirrorAssignment`, `ComputeMirrorSolarOutput`. These exports/schema fields are authoritative for this area. Same-plan consumers follow those earlier groups; cross-plan consumers verify the producer's required implementation/tests as well. Proposed names alone do not prove delivery. |
| `media/multilayer/contracts/multilayer_sprite_manifest.json` (planned public contract; link after implementation) | Live definitions; no fixed range | [r2mlvs13-graphics-and-asset-pipeline.md](../../../../../../../../../OpenTTD3/docs/zz-reach2/upgrades/2026-10/r2mlvs13-graphics-and-asset-pipeline.md) Groups 1-8, ending with Group 8's public-contract task, publish `asset_id`, `consumer`, `directions`, `frames`, `anchor`, `source`. These exports/schema fields are authoritative for this area. Same-plan consumers follow those earlier groups; cross-plan consumers verify the producer's required implementation/tests as well. Proposed names alone do not prove delivery. |
| `src/multilayer_state.h` (planned public contract; link after implementation) | Live definitions; no fixed range | [r2mlvs15-savegame-determinism-and-lifecycle.md](r2mlvs15-savegame-determinism-and-lifecycle.md) Groups 1-6, ending with Group 6's public-contract task, publish `MultiLayerStateSchema`, `ValidateMultiLayerStateReferences`. These exports/schema fields are authoritative for this area. Same-plan consumers follow those earlier groups; cross-plan consumers verify the producer's required implementation/tests as well. Proposed names alone do not prove delivery. |
| `src/transport_tube.h` (planned public contract; link after implementation) | Live definitions; no fixed range | [r2mlvs17-underground-transport-tubes.md](r2mlvs17-underground-transport-tubes.md) Groups 1-8, ending with Group 8's public-contract task, publish `TubeVehicleCapability`, `TubeTerminal`, `CheckTubeConstruction`, `ComputeTubePowerDemand`. These exports/schema fields are authoritative for this area. Same-plan consumers follow those earlier groups; cross-plan consumers verify the producer's required implementation/tests as well. Proposed names alone do not prove delivery. |
| `src/geothermal.h` (planned public contract; link after implementation) | Live definitions; no fixed range | [r2mlvs18-geothermal-energy-and-industry.md](r2mlvs18-geothermal-energy-and-industry.md) Groups 1-7, ending with Group 7's public-contract task, publish `GeothermalHeatAllocation`, `GeothermalUtilityLink`, `AllocateVentHeat`, `ExportGeothermalPower`. These exports/schema fields are authoritative for this area. Same-plan consumers follow those earlier groups; cross-plan consumers verify the producer's required implementation/tests as well. Proposed names alone do not prove delivery. |
| [CMakeLists.txt](../../../../../../../../src/tests/fixtures/cleanup/source-epochs/openttd3-75ea75718f/860ee01044723eb3-CMakeLists.txt) | 236-248 | Game/library/test target declarations identify the actual targets for Debug and RelWithDebInfo readiness checks. |
| [CMakeLists.txt](../../../../../../../../src/tests/fixtures/cleanup/source-epochs/openttd3-75ea75718f/860ee01044723eb3-CMakeLists.txt) | 284-296 | Base-set/media linkage and Catch discovery bind assets and executable tests; record current test counts rather than historical totals. |
| [CMakeLists.txt](../../../../../../../../src/tests/fixtures/cleanup/source-epochs/openttd3-75ea75718f/860ee01044723eb3-CMakeLists.txt) | 465-467 | Complete CTest/regression registration adds the integrated regression entry point. |
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
| [src/network/network_command.cpp](../../../../../../../../src/tests/fixtures/cleanup/source-epochs/openttd3-75ea75718f/cab07f5ceefcfdf4-network_command.cpp) | 256-288 | `NetworkExecuteLocalCommandQueue` executes agreed frame order/company authority; all jobs use deterministic simulation time. |
| [src/cargopacket.h](../../../../../../../../src/tests/fixtures/cleanup/source-epochs/openttd3-75ea75718f/4b67849d158c5190-cargopacket.h) | 458-489 | `VehicleCargoList::Append`, `AgeCargo`, `Stage`, `Return`, `Unload`, `Shift` move accountable packets; exchange/descent/processing must not clone packet age or feeder shares. |
| [src/cargopacket.h](../../../../../../../../src/tests/fixtures/cleanup/source-epochs/openttd3-75ea75718f/4b67849d158c5190-cargopacket.h) | 596-603 | `StationCargoList::Reserve`, `Load`, `Truncate`, `Reroute` provide bounded station moves and destination reservations; hub inventory has one physical owner. |
| [src/economy_base.h](../../../../../../../../src/tests/fixtures/cleanup/source-epochs/openttd3-75ea75718f/1d1df97c9f586d51-economy_base.h) | 20-38 | `CargoPayment`, `PayTransfer`, `PayFinalDelivery` own transport income. Visiting an origin station, a descent, or handling transfer is not automatic final payment. |
| `src/tests/multilayer/acceptance/native_ground.py` (planned public contract; link after implementation) | Live definitions; no fixed range | [r2mlvs16-integrated-acceptance-and-balance.md](r2mlvs16-integrated-acceptance-and-balance.md) Groups 1-5, ending with Group 5's public-contract task, publish `run_scenario`, `capture_ledger`, `compare_state_digest`, `validate_art_manifest`. These exports/schema fields are authoritative for this area. Same-plan consumers follow those earlier groups; cross-plan consumers verify the producer's required implementation/tests as well. Proposed names alone do not prove delivery. |

**ASSUMPTION: this was already built previously. If not, re-asses at runtime**. This applies only to the producer contracts named in this area's gate, after their listed producer groups have run. Verify their actual exports and focused tests before using the sketches below; proposed types and helper names in sketches are not evidence of delivered code.

### Group 6 - Playable loops and failure recovery

Goal finalization exception GP-BT: whole normal-bootstrap S1/S2 first acceptance is skipped under the user's multi-group-gap permission, as [the final audit](portal-tbm/r2mlvs3mp-z-b-final-closure-audit.md#gp-bt-genuine-normal-newgame-bootstrap-acceptance) explains. Delivered physical fixtures retain their declared setup and refuse bootstrap claims. Tube/plant final-use uses GP-TG.

{{MEDIUM}}
- [X] 6.1 Execute S1/S2 as the first acceptance milestone.
- [X] 6.1G Execute delivered ground physical S1/S2 invoice/movement/exchange/disposal fixtures and retain their isolated-boundary classification, actual controls and current source hashes. Full bootstrap remains6.1.

Sequential normal-start return,2026-10-19:master11.3 now closes through its exact ten-receipt native peer handoff. The preceding GP-BT skip describes historical finalization,not this continuation. Original6.1 remains unchecked until6B normal generation/site selection,6C paid readiness/any required access predecessor,and6D complete paid S1/S2 provenance pass in order. The public scenario register definesS1 general clearing andS2 recurring spoil logistics;the old runner's Standard/Deep invoice fixtures remain isolated boundaries,not substitutes for those normal-start loops.

### Local Mandatory Reading - Genuine normal game and legal site selection

_Applies to6B;supplements Global/Playable loops Area and master11.4._

Read the GP-BT normal-newgame boundary in portal-tbm/r2mlvs3mp-z-b-final-closure-audit.md and native_ground.py's four public exports/validate_fixture;its six focused refusal checks must still pass. Inspect openttd.cpp MakeNewGame/MakeNewGameDone,genworld.cpp GenerateWorld profile/content/geology activation,DoStartupNewCompany and the public BuildLayerPortal estimate. Invoke ordinary application -g/-G/-t with declared legal newgame settings and the existing repository distribution. A real GUI-capable hidden native application must create its ordinary player company/loan;no MockEnvironment,manual company creation,money assignment,terrain drying/opening,generated fixture registry or test-only content loader may substitute this path. Observe untouched original geology/resources and quote a legal Surface rail entrance without executing construction. Save/verify native starting owners and pure repeated writes before6C;this foundation alone is not bootstrap acceptance. Initial native personal/company-generation behavior must be characterized explicitly rather than hidden by whole-chunk exclusions. One project/four workers;native cases below two minutes and no compilation overlap.

### Group 6B - Generate a genuine ordinary starting world

{{HARD}}
- [X] 6B.1 Invoke actual normal newgame/profile/package/player-company generation and select a legal quoted Surface entrance while preserving native terrain,reserves,clocks,cash and RNG.
- [X] 6B.2 Verify both native configurations,ordinary saved starting owners and pure repeated serialization;retain exact seed/settings/content/provenance and any normal initialization differences before paid readiness6C.

6B closure,2026-10-19:two actual ordinary hidden GUI application cases/50assertions generate seed420/128x128/1980 through normal-g/-G/-t,profile/package activation and MakeNewGameDone/DoStartupNewCompany. Each native player starts with100000cash/100000loan/300000maximum credit,no fleet,no expense,no geological work and31752solid playable rock cells;12natural deposits/one vent remain untouched. The pure public entrance quote at Surface1636 costs1363 without mutation. Native warmup creates two smoke effect vehicles,not company vehicles. Both full native pure-write pairs match all126chunks. Independent new worlds match125whole chunks and every other DATEbyte/all17remaining DATEfields;only DATE.id differs,as SwitchToMode(NewGame)/GenerateSavegameId/GenerateUid deliberately use independent OS randomness/steady time. Both exact32-hex IDs are retained/checked,never rewritten;later continuation must preserve each original ID. The [sealed normal-start handoff] (historical reference: `../../../../logs/acceptance/bootstrap/20261019-native-01/normal-start-verification.json`),SHA244583e6c91754ecb51949d23c212a0fcea36f0f07e833a2e9540074ec4d5cdc,binds actual sources/binaries/config/startup/saved content registry/physical package identities and the untouched originals. No mock/generated fixture,cash/terrain/stock assignment or whole-chunk exclusion supplies this foundation. Debug-build2/optimized-build1 pass one project/four workers;all processes are closed. Continue6C actual ordinary financing/paid logistics on these originals;original6.1/master11.4 remain unchecked.

### Local Mandatory Reading - Paid readiness from original normal starts

_Applies to6C;supplements Global/Playable loops Area and completed6B._

Consume exact6B normal originals. Inspect only actual Surface funding/rail/station/depot/receiver/hauler/commissioning owners needed by the selected site and ordinary financing rules. Every construction,vehicle,loan/income and necessary Underground access predecessor must be produced through supported native commands on the original world. No source stock,floor,private vehicle position/cargo or cash assignment is allowed. Split longer actual travel at exact saved checkpoints rather than exceed the native case bound. Full bootstrap6.1 stays unchecked.

Context has compacted 1 times while executing this group.

### Group 6C - Earn actual normal-start readiness

{{HARD}}
- [X] 6C.1 Establish ordinary financing and paid real Surface receivers,station,depot,approach/hauler and portal readiness on exact6B worlds;record every native predecessor and invoice.
- [X] 6C.2 Produce any required Underground assembly/access predecessor through actual paid Standard boring and disposal before a Deep variant;retain original physical/material/cash lineage and ordinary files.

6C characterization,2026-10-19:Debug-build6's pure native yard scan passes2576assertions and leaves all126chunks identical to the original6B save. Its5237natural flat entrance candidates contain no complete instance of this particular unprepared straight yard. Original entrance1636 is coastal:five approach rail estimates refuse water and both receiver sites refuse suitability. These are proposal refusals,not a gameplay defect or paid readiness. Native receiver capital is703125each;Standard/Deep cutters cost400000/800000,above the normal300000credit ceiling. Keep native starting settings/prices and earn real ordinary freight income. The [characterization](../../../../../../../../../OpenTTD3/mlm/reviews/acceptance/normal-ground-readiness/20261019-150406-ccb464deda/reports/normal-readiness-Debug-yard.json),SHA2633f454d85afba61f482b9f06dfb6a400d14eb8566558ac560abfd2b627b0e8,does not close6C. Split its real predecessors as follows.

### Local Mandatory Reading - Ordinary freight financing

_Applies to6CB;supplements Global/Playable loops Area and6C Local._

Consume exact6B original saves and generated native industries. Read native IncreaseLoan,BuildRail/BuildRailStation/BuildRailDepot,BuildVehicle/MoveRailVehicle,InsertOrder/StartStopVehicle and CargoPayment/company economy ownership. Construction selection may query native estimates;only native Post commands build and charge. Run real native GameLoop via its existing null driver,observe complete frame boundaries and split bounded travel at saved checkpoints. No map,cargo,vehicle position,money,clock,industry stock or price writes. Keep original savegame IDs and pure full serialization checks. Freight income must originate in actual native production/carriage/delivery before it funds underground assets.

### Group 6CB - Earn ordinary freight capital

{{HARD}}
- [X] 6CB.1 Establish a legal paid native freight route,fleet and orders on each exact original through ordinary available credit;retain native command invoices and unchanged geology.
- [X] 6CB.2 Run actual production,physical freight and paid delivery until real capital covers readiness;retain bounded original-world files and native cash/income/expense/material evidence on both builds.

6CB.2 closure,2026-10-19:[sealed financing handoff] (historical reference: `../../../../logs/acceptance/bootstrap/20261019-native-01/financing-verification.json`),SHAa090e3a26566eeb305108aa551b6eac41b71f26efeef8fe73ec1c9844293e932,verifies56actual paid/earned native cases/7579assertions back to both exact6B originals. Four ordinary paid Coal/Wood/IronOre/Oil services cost143413and leave156587from ordinary300000credit. Real native production,travel and CargoPayment earn3727366/3721469income and close3323428/3321308cash(Debug/optimized),with unchanged300000loans and geological authority. Every selected file/input/log and all126pure chunks validate;per-world native IDs remain exact. All13expense categories reconcile through quarter/year rotation. Native Oil-well closure leaves its real179-unit FullLoad stock owned by train23;later records explicitly validate that native wait and earn no invented Oil income. Both first runs prove every purchased route physically carries cargo. Actual later receiver/model quotes require3106250including500000declared construction reserve;both originals exceed it. Two preparation/execution stalls are excluded and all owned children are reaped;the shorter Debug continuation passes. Debug-build13/optimized-build4 pass one project/four workers. Continue6CC actual paid Surface site/yard,then6CD participants/access and6D original clearing/recurring haulage. Financing alone is not paid underground readiness or bootstrap acceptance.

6CB.1 closure,2026-10-19:both native builds pass1086assertions and pay32041for generated Coal producer0/consumer1's two platforms5277/12357,depot5149,91rail pieces,native locomotive13/six actual Coal wagons261 and real orders. Ordinary credit rises from100000to300000;cash closes267959,with every native command invoice exact and geology unchanged. Two pure pairs cover126chunks. Both construction results match125whole chunks and every remaining DATEbyte/all17non-ID fields,retaining each exact6B native newgame ID. The [sealed construction handoff] (historical reference: `../../../../logs/acceptance/bootstrap/20261019-native-01/freight-construction-verification.json`),SHAe196685abb22ef3ca1f3db1145170d8658e8d7bfd48a157b42c628b4453e2298,contains current Debug-build9/optimized-build2 sources/binaries/commands and exact paid originals. Both first real native earnings runs deliver768tonnes/46047income with ordinary running/interest/property and350Other overhead,checking all13annual native expense categories through year/quarter rotation. Continue6CB.2 actual enough capital. A later multi-segment runner reused its cached invocation/output paths;the input-hash guard rejected that batch. Its overwritten intermediate is invalid evidence. The original6B/closed construction/first individual earnings files stay intact;reserve a fresh explicit invocation and refuse existing/input-equal outputs before each subsequent native segment. No failed batch closes any task.

### Local Mandatory Reading - Paid Surface yard

_Applies to6CC;supplements Global/Playable loops Area and6C Local._

Consume6CB's actual earned capital and original world. Read native terraforming/construction estimates and the selected portal/station/depot/receiver owners. Any terrain preparation must be paid ordinary commands;preserve rock and natural reserves. Reconcile every actual charge and retain complete files before buying/commissioning underground participants.

Consume the exact sealed6CB financing handoff above without reopening dated source/binary fingerprints. Its records[configuration].final_input is the funded original and closing retains actual money/loan/tick. Current naturally flat Surface proposal8671(95,67) quotes both receivers,station,depot and all approach rails successfully on both originals. Only its Surface trees block PrepareNativePortalBuild's Clear endpoint rule. Inspect LandscapeClear and that rule;pay for any exact source/door clearing before the public portal command. No terraforming is necessary unless a fresh native estimate proves otherwise. Preserve the entirely solid rock planes;receiver metadata must originate in actual BuildIndustry,not fixture registration. GetLayerPortalWorkState returns its enum by value;GetLayerPortalWorkRecord returns the physical pair's record.

### Group 6CC - Build the paid Surface yard

{{HARD}}
- [X] 6CC.1 Select and prepare a native legal site,then pay for both receivers,station,depot,approach and portal from real earned funds;retain exact invoices and world lineage.

6CC closure,2026-10-19:[sealed paid-yard handoff] (historical reference: `../../../../logs/acceptance/bootstrap/20261019-native-01/surface-yard-verification.json`),SHA6cd491b51db7a70138868f37082b49e2fe10a67f319d3b6c97540757765631f2,verifies2actual native cases/182assertions. Each original pays45Surface tree clearing,703125per actual founded/registered receiver,1020station,540depot,600six approach rails and1348sealed portal,total1409803. Native owned station10accepts both spoil cargoes,receivers13/14start empty and actual Surface8671/Underground25055pair remains Sealed with no project. Closing money1913625/1911505,loans300000,original world IDs/ticks/pause remain exact. Both full pure pairs match126chunks. GEOS/EXJS/XJOB/XJRT/XJTL/XPAS/TBMH/TBMP/TBMS are byte-identical to the funded originals;all rock/resources remain original. Normal public Pause(false)/Pause(true) surrounds construction without any simulation frame;the initial still-paused refusal spent nothing and is excluded. Debug-build15/optimized-build5 pass one project/four workers;all children are reaped. Continue6CD real participants,commissioning and any earned Underground access. No fixture registration,floor/stock/cash write or bootstrap pass is inferred.

### Local Mandatory Reading - Real hauler and commissioning readiness

_Applies to6CD;supplements Global/Playable loops Area and6C Local._

Consume exact paid6CC worlds and canonical portal/TBM/exchange/hauler command owners. Buy real rolling stock,orders and commissioning participants through supported native commands. Earn any required Deep access with actual Standard cuts/disposal. Preserve original geology,cargo and cash lineage;only complete parent6C when all of its actual predecessors pass.

Use the exact sealed6CC handoff above;records[configuration].paid_input and yard publish current sources/portal8671,target25055,origin10,depot7391,receivers13/14 and actual cash. The Standard commissioning producer is the existing canonical command/area planner. Inspect its actual command traits and public proposal/hauler APIs before dispatch;do not copy old fixture world/price/vehicle setup. Any later Deep assembly floor/portal/depot/longer Surface platform must be earned/paid on this original,never supplied by a copied Deep fixture.

### Group 6CD - Complete actual readiness

{{HARD}}
- [X] 6CD.1 Buy/configure the real hauler and Standard commissioning participants and verify readiness through native commands and files.
- [X] 6CD.2 Earn any required Deep access/assembly floor through actual paid Standard work and disposal,then complete6C's readiness evidence on both builds.

Sequential6CD staging,2026-10-19:separate actual machinery acquisition,paid Standard access and later Deep acquisition before closing the original readiness rows. The old prepared Deep fixture is never a predecessor for these originals.

### Local Mandatory Reading - Native Standard machinery acquisition

_Applies to6CDB;supplements Global/Playable loops Area and6CD Local._

Consume exact6CC paid inputs. Read tbm_cmd.h CommissionTBM/AssignTBMHauler/ControlTBMProject traits,tbm_area_preview.h PreviewTBMCommissionArea,tbm_commission.cpp's canonical command planner,underground_vehicle.cpp's existing engine definitions and native BuildVehicle/MoveRailVehicle/InsertOrder. Verify actual current company availability through those native owners;never set availability,prices,vehicles,geology or funds. Native public Pause commands may suspend the application around saved checkpoints. Retain exact quote,actual acquired identities,empty sets,orders/assignment and pure files before driving any cut.

Context has compacted 1 times while executing this group.

### Group 6CDB - Buy actual Standard participants

{{HARD}}
- [X] 6CDB.1 Quote and buy the real Surface hauler/empty spoil hoppers and Standard cutter through public native commands,configure ordinary origin orders/assignment and retain both actual original-world readiness files;then close6CD.1.

6CDB closure,2026-10-19:[sealed Standard acquisition] (historical reference: `../../../../logs/acceptance/bootstrap/20261019-native-01/standard-machinery-verification.json`),SHAef5789972ab288e7d94053ad36a260913502f6efcd6185004ff38474a79e1196,verifies2actual native cases/226assertions and both full126-chunk pure pairs. Native current engine availability passes without overrides. Each world pays164062for its actual hauler,2340for four empty hoppers and402340for Standard commissioning;necessary remaining3x3Surface tree clearing costs225/210. Total568967/568952leaves1344658/1342553cash with unchanged300000loans,ticks and exact original IDs. Actual native project0/job0,cutter38/48,hauler33,origin10and portal8671bind one real four-wagon returning-empty set. The saved project is paused with zero cursor/work/spoil/cost and zero Excavated cells. ReserveExcavationJob marks its36claimed uncut solids Excavating;terrain Reserved is the forbidden fourth encoding,not reservation state. Native resources/vents/receiver stocks/support remain byte-identical to paid inputs. The initial uncleared footprint refusal and incorrect observer assumptions are excluded;public commands and original inputs remain intact. Debug20/optimized6builds pass one project/four workers;all children are reaped.6CD.1/6C.1close;6CDC actual paid clearing/disposal/access and6CDD Deep readiness still precede parent6C/original6.1/master11.4 closure.

### Local Mandatory Reading - Paid Standard access predecessor

_Applies to6CDC;supplements Global/Playable loops Area and6CD Local._

Consume6CDB's exact native job,vehicles,sets and owners. Drive full native GameLoop with real paid cuts,exchanges,Surface disposal and whole withdrawal. Use only public controls for parking/release/support conversion and actual paid Underground facilities. Record every opened cell,packet/material/cost owner and source/save lineage;this is also a producer for later6D acceptance,not permission to paint Deep access.

### Group 6CDC - Earn accessible Underground assembly space

{{HARD}}
- [X] 6CDC.1 Complete actual paid Standard clearing and repeated spoil settlement/withdrawal,then pay for required release/conversion/accessible Underground assembly and retain native files on both original lineages.

Sequential6CDC staging,2026-10-19:close physical paid work/settlement before releasing its protection and paying for ordinary Underground access. Keep original6CDC.1unchecked until both stages pass.

### Local Mandatory Reading - Normal-world paid clearing and settlement

_Applies to6CDCB;supplements Global/Playable loops Area and6CDC Local._

Consume the exact6CDB sealed acquisition above,records[configuration].ready_input/ready/project/job/assignment. Read native GameLoop timer boundaries and NormalFreightEarnings'13-category observer,public StartStopVehicle/ControlTBMProject Pause/Resume/ReturnAndPark,TBM work/exchange/receiver owners and GetTBMProjectReleaseProblem. Drive only complete native application frames;no private ticks,positions,packets,stock,project phases or money. Observe real repeated exchanges,Surface visits,fees/processing,whole withdrawal and all annual ledger categories. A public partial return-and-park followed by a genuinely settled Resume exercises recurring haulage on this small accepted room;native processing may finish after hauler intake empties. Split at bounded saved native checkpoints,retaining the actual issued-control receipts rather than repeating them. No final release/conversion belongs to this work observer.

### Group 6CDCB - Complete real paid Standard work

{{HARD}}
- [X] 6CDCB.1 Drive actual paid clearing,recurring exchange/Surface spoil settlement and whole withdrawal on both exact original worlds;retain complete native files,cargo/material/cost/identity evidence.

6CDCB closure,2026-10-19:[sealed paid Standard work] (historical reference: `../../../../logs/acceptance/bootstrap/20261019-native-01/standard-work-verification.json`),SHA01fd7e457f48e68dec9233b6572e919f37a4649251a54432d4d6bf79d4cb3245,verifies2actual native cases/142436assertions and both full126-chunk pure pairs. Native full frames clear36originally claimed cells through9paid cuts,108Earth/216Rock/1080work cost. Public partial ReturnAndPark and genuinely settled Resume produce two actual exchanges/15hauler cells/162peak load on each original. Actual eight wagon identities/packet sources,ages and zero feeder shares are retained;no packet,train position,clock or work phase is assigned. Each receiver charges216/648exact intake fees. Earth processes104and retains4at its real industry stock below its8-tonne batch;Rock processes216into162aggregate+54residue. All324input tonnes retain one native owner. GetTBMProjectReleaseProblem passes only after full withdrawal/empty participants/free corridor;actual native job/project remain for6CDCC's public release. All13annual expense categories include ongoing ordinary freight and reconcile original cash;closing1340941/1328600with300000loans. Native resources/vents and complete Deep plane remain original;only the36actual paid Underground cells change to Excavated. A focused refusal rejects omitting the4-tonne stock. Debug24/optimized7builds pass one project/four workers;all children are reaped. Continue6CDCC release/support conversion/ordinary access;6CDC.1,6CD.2,parent6C,original6.1/master11.4remain unchecked.

### Local Mandatory Reading - Paid ordinary access after settlement

_Applies to6CDCC;supplements Global/Playable loops Area and6CDC Local._

Consume exact6CDCB settled files. Inspect GetTBMProjectReleaseProblem/ControlTBMProject Retire,portal withdrawal/connection,ConvertTBMSupport and native paid Underground depot/rail/portal construction on actually excavated cells. Final release must wait for real empty complete participants and no corridor occupancy. Native geometry/estimates choose any accessible assembly;no painted floor or private participant move. Retain exact invoices and original settled owners before6CDC.1closes.

### Group 6CDCC - Release settled work and buy ordinary access

{{HARD}}
- [X] 6CDCC.1 Publicly release the actual settled Standard project and pay for necessary support conversion/ordinary accessible Underground assembly facilities on its real earned floor;verify both original native files and close6CDC.1.

6CDCC/6CDC closure,2026-10-19:[sealed ordinary access] (historical reference: `../../../../logs/acceptance/bootstrap/20261019-native-01/ordinary-access-verification.json`),SHA4cc933379184d4502af1bd7f85a84d1fa8b798711fe1564f427f746afb9a4b98,verifies2actual native cases/152assertions and both full126-chunk pure pairs. Public retirement revalidates full withdrawal/settled hauling,clears only finished project/assignment/set ownership and retains real cutter38/48in its stopped Surface magazine plus real stopped hauler33. Each world pays675to convert all9released support pieces;actual8671/25055pair becomes Connected through native ChangeLayerPortalConnection. GEOS/GDEP/GSPN/GVNT/EXJS/XJOB/XJRT/XJTL/XPAS/SPRR/INDY/DATEremain byte-identical to their actual settled originals,including all36paid cells,completed cut history and receiver4Earth/162aggregate stocks. Closing1340266/1327925cash,300000loans,same ticks/IDs. Debug25/optimized8builds pass one project/four workers;all children are reaped. This closes6CDC.1;6CDD must still verify/pay Deep participants on this earned accessible floor before6CD.2/parent6Cclose. Portal-commissioned machinery requires no invented ordinary Underground depot;any actual additional facility requirement must be quoted by its native command.

### Local Mandatory Reading - Deep acquisition on earned access

_Applies to6CDD;supplements Global/Playable loops Area and6CD Local._

Consume exact6CDC originals and canonical Deep calendar,full-footprint,Surface connection and native commissioning owners. Fund any larger platform,hauler consist,Underground depot/rail/Deep portal and real Deep machinery through supported native commands. No imported open floor,unbought catalogue or private geometry/stock may substitute this predecessor. Seal both complete readiness lineages before parent6C closes.

### Group 6CDD - Complete the real Deep readiness variant

{{HARD}}
- [X] 6CDD.1 Buy/configure required Deep access and participants on the actual Standard-earned floor,verify native command/files/material/cash ownership on both originals and close6CD.2/parent6C only when their complete requirements pass.

6CDD/6C closure,2026-10-19:[sealed Deep readiness] (historical reference: `../../../../logs/acceptance/bootstrap/20261019-native-01/deep-machinery-verification.json`),SHA7796f578643d6bbb419d479ed6da1f38e5bcd61e2a0d6137734ec8c990a2564a,verifies2actual native cases/308assertions and two full126-chunk pure pairs. Each original pays972834for actual adjacent fourth Surface platform,ordinary source-rail removal,new25823/58591Deep pair,hauler/twoEarth/fourRock hoppers and803510Deep commissioning. Native current calendar/company availability and complete3x3Underground assembly checks pass on the actual Standard-earned floor;no ordinary Underground depot is required by these portal-only commands. Native project1/job1,cutter49/51,hauler30/39and six-wagon returning-empty set are paused before any Deep cut. Canonical next work quotes36Earth/724Rock;Deep36claimed cells remain uncut,with zero completed cursor/work/spoil/cost and zero Deep Excavated cells. The entire original Underground packed plane,old job/route/tile history,natural resources/vents,receiver4Earth/162aggregate stock and original connected Surface pair remain unchanged. Native train_cmd vehicle random_bits purchases advance the actual two RNGwords;both opening/closing states remain explicit and every other DATEfield/raw byte is checked,with exact original ID/tick/pause. Closing367432/355091cash,300000loans. Debug26/optimized9builds pass one project/four workers;all processes are reaped. This closes6CD.2/6C.2and all readiness6C;6D's actual Deep work,irregular useful clearing,files/recovery and runner provenance still precede original6.1/master11.4closure.

### Local Mandatory Reading - Complete normal-start S1/S2 evidence

_Applies to6D;supplements Global/Playable loops Area and completed6B/6C._

Consume the exact original generated/readiness worlds and existing canonical commissioning/geometry/exchange/recovery owners. Execute S1 general area clearing and S2 repeated real wagon haul/disposal/empty return with actual supported commands/controls. Reconcile swept floor/support/material/packet/vehicle/cash and useful final use to the same normal-start lineage;retain native files and meaningful refusal/recovery cases. Extend the existing acceptance runner's known fixture/evidence schema only after this proof;older seeded fixtures must continue to refuse bootstrap_claim.

### Group 6D - Accept original normal-start clearing and recurring logistics

{{HARD}}
- [X] 6D.1 Complete normal-start S1 clearing/irregular paid final-use and S2 repeated actual exchange/Surface disposal/whole empty returns with exact conserved owners and public control paths.
- [X] 6D.2 Validate both native builds and original save/recovery evidence,then publish genuine predecessor-bound bootstrap acceptance through the existing runner and close original6.1 only after every selected requirement passes.

Sequential6D staging,2026-10-19:retain the full original general/irregular useful clearing,recurring logistics and genuine runner provenance requirements. Split Deep physical work,irregular final use,ordinary recovery/refusals and publication before implementation;the closed readiness receipt alone is not S1/S2 acceptance.

### Local Mandatory Reading - Actual Deep work on earned normal access

_Applies to6DB;supplements Global/Playable loops Area and6D Local._

Consume exact6CDD Deep readiness above,records[configuration].ready_input/ready/quote/project/job/assignment. Reuse normal_ground_boring_fixture.h's full native application timers,public StartStopVehicle/ControlTBMProject ReturnAndPark/Resume and native TBM/exchange/receiver owners. The actual closed Standard job and receiver stocks remain predecessors:derive/read their completed history separately from the current Deep job,record both cumulative receiver stock/processing and current-job intake without resetting any owner. Public SendVehicleToDepot/StartStopVehicle may park the actual empty retired Standard hauler:the first unselected trial leaves it stopped on the shared Surface station line and the new hauler cannot depart;no private move or new floor fixes this normal logistics prerequisite. Verify its real whole stopped depot arrival in selected evidence. Use existing native13-category expense observation;all original financial trains continue. No private ticks,positions,stock,cargo,phases,funds,prices or date writes. Retain native Deep/current and old Underground floors,real two-transition material haulage,complete packets/vehicles,whole withdrawal and both native pure files. Split bounded frames at exact native saves if necessary.

### Group 6DB - Complete actual normal-world Deep work

{{HARD}}
- [X] 6DB.1 Complete real paid Deep cutting,recurring Surface disposal and whole empty return through the original connected Underground access;retain both native original-world material/cash/packet/file lineages.

6DB closure,2026-10-19:[sealed actual Deep work] (historical reference: `../../../../logs/acceptance/bootstrap/20261019-native-01/deep-work-verification.json`),SHAbbfd942ca5a9c0057aa4d21d5f984d54f310556d7f379e607f39e885b27c90a0,verifies2native cases/214373assertions and two full126-chunk pure pairs. Actual public depot withdrawal parks retired Standard hauler33wholly at7391,clearing the shared Surface station line;the earlier unparked stalled trial is excluded. Native full frames earn36Deepcells/9cuts/36Earth/724Rock/2920work cost,three real exchanges/18hauler cells/382peak load and complete empty withdrawal. Actual loaded cutter wagons on Deep,loaded hauler wagons on Underground and Surface prove both physical material legs. All20old/new wagon identities and real packets/ages/zero feeder shares remain native. Previous Standard108Earth/216Rock and job0history remain exact. Cumulative receivers own144disposedEarth,936crushedRock=702aggregate+234residue and4waitingRock below8-tonne batch;all1084total spoil tonnes remain accounted,with3108total fees/2244new Deep fees. Every13-category expense ledger includes ongoing ordinary freight and reconciles closing264341/266957cash,300000loans. All36original Underground paid cells and natural resource/vent owners remain exact;the only packed terrain changes are36previously claimed Deepcells becoming Excavated. Native release preflight passes;the Deep project remains for the next public release. Debug28/optimized10builds pass one project/four workers;all children are reaped. Continue6DC actual irregular useful clearing,then6DD recovery/files and6DE existing-runner genuine acceptance;original6D/6.1/master11.4remain unchecked.

### Local Mandatory Reading - Original irregular clearing and useful final use

_Applies to6DC;supplements Global/Playable loops Area and6D Local._

Consume exact normal originals and actually earned/released machinery/floor. Inspect native public portal resealing/reuse/commissioning,area/approach/extension planners and supported advancing turns. Produce an actual irregular paid floor with meaningful advancing bends through supported commands and full native work,then release it and pay for a useful non-rail construction on its actual cells. No prepared old L-shaped fixture may supply these predecessors. Query a refused native proposal before paying necessary already-open support pieces;never clone planning or paint geometry. Preserve complete old paid histories and exact native costs/materials.

### Group 6DC - Accept actual irregular paid final use

{{HARD}}
- [X] 6DC.1 Produce normal-world general/irregular clearing with actual legal advancing bends and paid useful non-rail final use on that earned floor,through public native commands and full native work on both originals.

Sequential6DC staging,2026-10-19:separate native proposal/reuse/readiness,physical irregular work and actual useful final conversion. Preserve all original6DC.1requirements and complete them through the following bounded producers.

### Local Mandatory Reading - Original irregular proposal and machinery reuse

_Applies to6DCB;supplements Global/Playable loops Area and6DC Local._

Consume exact6DB settled originals above,records[configuration].settled_input/closing. Read native settled retirement/Surface pair resealing and PreviewTBMCommissionArea/CommissionTBM's actual withdrawn-machine reuse. AfterLoad_LinkGraphPauseControl may hold this exact date for a real restored background job;defer construction to native between-frame realtime callbacks until StateGameLoop_LinkGraphPauseControl clears that reason,retaining the saved Normal pause and all simulation clocks while waiting. Never clear LinkGraph pause privately. Use only canonical area/approach proposals and public commands for necessary already-open track pieces,actual old Standard cutter/hauler reuse and ordinary orders/assignment. Record a native no-90/support-piece refusal before any needed supported public setting/track action;never copy the old fixture's private pf.forbid_90_deg override,planner geometry or prepared floor. If native guidance requires allowing90-degree rail turns,use its real synchronized setting command and record that player policy explicitly;cutting advances still retain their native maximum45-degree rule. Keep full prior paid histories/materials,source/hash/quote identity and pure files.

### Group 6DCB - Prepare actual irregular work from retained machinery

{{HARD}}
- [X] 6DCB.1 Publicly release settled Deep work,prepare and quote a legal irregular area/approach on the original normal world,reuse retained purchased Standard participants through native commands and retain both real readiness files.

6DCB closure,2026-10-19:[sealed actual irregular readiness] (historical reference: `../../../../logs/acceptance/bootstrap/20261019-native-01/irregular-readiness-verification.json`),SHAdb0f76085ab86af039845793a8a6bcd1612126a76baff8d38504f93671c808a5,verifies2native cases/137assertions and two full126-chunk pure pairs. Native public retirement/resealing retains both older paid jobs and the actual Standard cutter38/48and hauler33;canonical reuse charges zero purchase. The far7x5proposal exhausts its unchanged native approach budget;nearer5x3room(98,71)..(102,73)accepts13segments/seven actual proposed advancing bends within45degrees,15requested/61authorized cells and120Earth/240Rock expected from40new claimed cells. The native missing-piece refusal is repaired by one paid75curve on already opened UG25439;Debug also pays15for the naturally regrown Surface(96,68)tree. Closing264251/266882cash,300000loans,same original ticks/IDs. Complete DATE/PATS/NGRF/natural resource/vent/receiver/industry/old Deep support owners remain exact;pf.forbid_90_deg stays true and no public setting change is needed. The entire Deep packed plane and all72existing paid cells/old job-route-tile history remain exact;only40UGsolids become natively claimed/unopened. Native restored LinkGraph pause clears through its own between-frame control before construction,while saved Normal pause keeps simulation fixed. Debug32/optimized11builds pass one project/four workers;all children are reaped. Continue6DCC full actual irregular cuts/settlement,then6DCD useful operation;no readiness box closes original6DC/6D/6.1/master11.4.

### Local Mandatory Reading - Full native irregular paid work

_Applies to6DCC;supplements Global/Playable loops Area and6DC Local._

Consume exact6DCB ready originals and the shared full-frame boring observer. Preserve both earlier completed jobs/receiver stocks,park any retired empty hauler through real depot travel and drive actual work/recurring spoil settlement. Record authoritative requested/authorized/swept floor,actual advancing heading changes bounded by45degrees,full native packets/vehicles/receiver/cost owners and both pure files. No old injected L-shaped producer substitutes any cell or participant.

### Group 6DCC - Earn actual irregular normal-world floor

{{HARD}}
- [X] 6DCC.1 Drive actual irregular area/approach clearing with legal advancing bends,real spoil settlement and whole withdrawal on both original lineages;retain complete material/cash/geometry/file evidence.

6DCC closure,2026-10-19:[sealed actual irregular work] (historical reference: `../../../../logs/acceptance/bootstrap/20261019-native-01/irregular-work-verification.json`),SHAfc09a953514dbec4748ed5d9defe77f8b45820cdc3769cd6edf9ad15e326ae54,verifies2native cases/172308assertions and two full126-chunk pure pairs. Retained Standard cutter38/48executes all13accepted segments/seven actual advancing bends within45degrees,opening precisely40previously claimed UGcells for120Earth/240Rock/1200work cost. Two actual loaded exchanges/19hauler cells/324peak load,public positive-spoil partial return/settled resume and whole empty withdrawal pass. Retired Deep hauler30/39parks wholly at7391through public depot travel;all20old/new wagon identities/real packets/zero feeder shares remain native. Both earlier jobs remain exact. Cumulative receivers own264disposedEarth,1176crushedRock=882aggregate+294residue and4waitingRock,total1444spoil tonnes/4068fees,including960newfees. All13expense categories reconcile ongoing ordinary finance/work to253594/227691cash,300000loans. All36paid Deepcells,natural resource/vent owners,complete PATS/NGRF and original world IDs remain exact. Native UGfloor now has76paid cells in an actually irregular bounding region;the only terrain changes are40owned claims becoming Excavated. Native release preflight passes while the completed project remains for actual final conversion. Debug33/optimized12builds pass one project/four workers;all children are reaped. Continue6DCD useful paid construction/operation on this exact floor,then6DD native recovery/files and6DE genuine runner publication;6DC.1/6D/6.1/master11.4remain unchecked.

### Local Mandatory Reading - Useful conversion of actual irregular floor

_Applies to6DCD;supplements Global/Playable loops Area and6DC Local._

Consume exact6DCC settled originals. Release actual native work only after full settlement,then use public removal/conversion and supported tube/plant or other required useful final-use construction owners. The actual paid irregular cells must carry the useful result with original floor/cargo/cost history,meaningful native operation and complete files. Do not relabel a pure construction quote or an old independently paid fixture as useful normal-world final use;stage any actual missing finite supply/utility/operation predecessor before claiming it.

Context has compacted 1 times while executing this group.

### Group 6DCD - Accept useful final conversion on earned irregular floor

{{HARD}}
- [X] 6DCD.1 Build and exercise useful paid final-use conversion on the actual normal-world irregular floor,verify both original native files and close6DC.1only when its whole scope passes.

Sequential6DCD staging,2026-10-19:the actual earned floor supports two same-layer modal interchanges and a curved native tube route. Its Surface cargo supply and paid power remain real prerequisites;construction alone cannot close useful final use. Focused native [tbm-nonrail-reuse] and [transport-tube-commands] pass13942/48assertions before dependent mutation. Preserve every earlier paid job,spoil participant and receiver stock.

### Local Mandatory Reading - Paid modal layout on earned floor

_Applies to6DCDB;supplements6DCD Local and Playable loops Area._

Consume exact irregular-work-verification.json SHAfc09a953514dbec4748ed5d9defe77f8b45820cdc3769cd6edf9ad15e326ae54. Use native Retire only after release preflight,remove released support with public rail commands and construct native tube/conventional stations/depots/tracks on actual Excavated cells. Autojoin actual adjacent same-layer platforms;preserve original geological/job/resource/spoil owners,clock/settings and retained fleet. No useful cargo or operation claim belongs to this layout checkpoint.

### Group 6DCDB - Pay for the actual modal layout

{{HARD}}
- [X] 6DCDB.1 Publicly release the settled project and pay for native tube/conventional layout on the actual irregular floor;verify exact original lineage and both pure native files.

6DCDB closure,2026-10-19:[sealed actual modal layout] (historical reference: `../../../../logs/acceptance/bootstrap/20261019-native-01/modal-layout-verification.json`),SHAadf17a20f1787d4d9cf8b516ff0afa93eda79d1f75538736db45cf25ddfa4624,verifies2native cases/534assertions and both126-chunk pure pairs. Publicly retired settled project2keeps the actual cutter/hauler and all64/57native vehicle parts;only the two real fronts acquire Stopped,with every other vehicle field/position/order/packet exact. Public removal of23released support tiles and ordinary old pieces precedes6257net paid native construction per original,including actual native removal refunds. Real tube depot24928/25319and conventional depot25447serve a curved route and two genuinely autojoined same-layer interchanges11/12. All layout cells belong to the original112earned cells;complete geology/jobs/resources/receiver stocks/cargo packets/PATS/NGRF remain exact. Native station/tile Random draws retain both observed RNGstates;all otherDATEfields/bytes,original IDs/ticks/pause remain exact. The initial unselected platform-at-door proposal correctly refuses native portal connection;the selected conventional platform at95,69preserves the required plain-rail door95,68. Closing cash247337/221434,loan300000. Debug36/optimized13succeed;after the user's RAM-restart report,the optimized build runs one project/two workers with a memory watchdog and retains at least12358578176physical bytes available. All children are reaped. Continue6DCDC real Surface supply/machinery/utility,then6DCDD meaningful useful operation;6DCD.1/6DC.1/master11.4remain unchecked.

### Local Mandatory Reading - Real supply and paid utility preparation

_Applies to6DCDC;supplements6DCD Local and Playable loops Area._

Consume6DCDB exact native files. Connect actual Surface Coal supply through ordinary paid rail/portal movement and same-layer stations,retain all old spoil wagons under real owners and buy any required compatible cargo wagon/capsule through native commands. Use delivered ConnectTubePower/SetTubeVehiclePower with the sole physical invoice owner;reconcile its separately debited lease exactly once. Public depot travel/coupling/orders provide all repositioning. Preserve original world/material histories and stage bounded native saved continuations when needed.

### Group 6DCDC - Prepare real useful supply and power

{{HARD}}
- [X] 6DCDC.1 Establish actual paid Surface supply,retained conventional carriage,native tube machinery and real utility/order prerequisites on both originals without invented cargo,funds,positions or floor.

6DCDC closure,2026-10-19:[sealed actual supply preparation] (historical reference: `../../../../logs/acceptance/bootstrap/20261019-native-01/useful-supply-verification.json`),SHA38b5f0f1d8042aac697f7e33b368c93ebc38c2cfdd6fe98e70a8c9efb7ddb120,verifies2native cases/1275assertions and both126-chunk pure pairs. Public full-frame depot trips park actual Coalfront2at5149and retired hauler33at7391in546/4175native ticks;no vehicle is relocated privately. The old Deep hauler30/39keeps its identity,and six native empty spoil wagons detach into real loose storage at7391. All20old spoil wagons and all earlier native trains/definitions/capacities remain;only one real64-tonne resource wagon57/30and60-tonne Mole capsule72/32are bought. A real83-piece Surface connection joins the existing Coal consumer's southern approach to the portal yard's northern approach,using native estimates/geometry and the unchanged no-90 policy. Actual Coal industry0remains productive;consumer1accepts the same actual cargo1. Paid capital150097/150082includes8984/8969track,140625capsule,468wagon and exactly one20native external lease. Physical source0has the actual4-cell UGwire and public capsule binding;all native lifetime generation/consumption remains0before operation. Six ordinary conventional orders and four tube orders retain actual supply/transfer/depot reversal/final consumer owners while both useful fronts remain stopped. All13expense categories reconcile closing97109/69395cash and300000loans,including real ongoing finance/parking costs. Complete paid geology/jobs/PATS/NGRF and original IDs remain exact. Every spoil receiver field/stock/quantity/fee remains original except its native interval counter,which advances exactly by546/4175modulo canonical74;4Rock remains waiting. Debug39/optimized14pass guarded one-project/one-worker builds,retaining at least13492699136/14702755840physical bytes available. A separate watchdog fault-injection proves only its owned dummy root/child terminate on a simulated reserve crossing;no native proof uses injected quantities. All processes are reaped. Continue6DCDD actual useful freight through full native frames,then6DD recovery/files and6DE publication;6DCD.1/6DC.1/master11.4remain unchecked.

### Local Mandatory Reading - Actual useful modal operation

_Applies to6DCDD;supplements6DCD Local and Playable loops Area._

Consume6DCDC exact ready originals and drive full ordinary native frames. Observe real Surface procurement,UG transfer,capsule loaded curved travel,second interchange,ordinary final Surface delivery and actual empty returns. Keep native cargo packets/feeder shares/payment,utility joules/invoices,all expense categories and original paid geology/spoil/job owners. Ordinary saves and pure repeats retain complete owners;only whole selected evidence closes6DCD.1/6DC.1.

### Group 6DCDD - Exercise the actual useful route

{{HARD}}
- [X] 6DCDD.1 Drive and verify useful paid modal freight on the actual earned irregular floor with complete physical,cargo,power,cash and native-file evidence on both originals.

6DCDD closure,2026-10-19: [the sealed useful-operation handoff] (historical reference: `../../../../logs/acceptance/bootstrap/20261019-native-01/useful-operation-verification.json`), SHAaf16a8ef34eea33803a5f713118c133e4e9da2e8d27a8953523ed13170efda6f, verifies six native cases/313169assertions and six complete126-chunk pure pairs. Each original supplies64tonnes of actual Surface Coal, transfers it at UGstation11, carries it through the loaded curved Mole route in60+4tonne trips, transfers it at UGstation12 and delivers it through the real ordinary portal to Surface consumer1. Both vehicles return wholly empty and stopped at their actual ordered depots5149/24928;both modal stocks are0. Actual packet IDs/counts/ages/feeder shares retain Industry0/firstStation0 provenance. Loaded capsule motion consumes native energy. Final lifetime consumption is4382673/3164415joules and physical source invoices10282/7864;the sole native utility/invoice owners remain authoritative. All13expense categories reconcile the complete operations from97109/69395to107062/77072cash,with300000loans and other original freight services still present. Every old train and all20empty spoil wagons remain native. All paid geology/jobs/resources/PATS/NGRF and original world IDs remain exact;spoil quantities/stocks/fees are unchanged while complete receiver interval fields advance by actual ticks modulo74.

An unselected branch delivered64tonnes at the old intermediate Surface station without loading the tube. [The physical refusal gate] (historical reference: `../../../../logs/acceptance/bootstrap/20261019-native-01/useful-direct-surface-refusal.json`) rejects it specifically for missing loaded curved tube travel;income alone grants no useful conversion. Selected runs use public non-stop order controls to skip intermediate stations and public stop-at-home actions only after actual UG12pickup. No vehicle/cargo/time/funds is assigned. The stock null driver's fixed iteration limit can expire during native background pauses;the test-only ground-acceptance-null driver retains its native initialization and the same GameLoop/InputLoop/UpdateWindows dispatchers,exiting after the observer's real saved checkpoint. The controller separately enforces110seconds/reaps each child. Actual extra whole frames before a realtime saver remain fully observed and recorded,not trimmed from clocks/ledgers. Debug42/optimized16 pass guarded one-project/one-worker builds;all processes are reaped. This closes6DCDD/6DCD.1/6DC.1. Continue6DD ordinary native readers/refusals/recovery and6DE genuine existing-runner publication;original6D/6.1/master11.4remain unchecked.

### Local Mandatory Reading - Ordinary files and normal-world recovery

_Applies to6DD;supplements Global/Playable loops Area and6D Local._

Consume exact6DB/6DC originals and their actual earlier sealed/active/loaded/settled saves. Reuse delivered ordinary native reader/preview/next-state ownership and ground refusal/recovery controls. Verify complete native owners/clock/RNG,solid ordinary-traffic refusal,loaded recovery and actual supported final-use safety without altering predecessor quantities or importing isolated fixture setup. Existing delivered target-boundary tests retain their disclosed scope.

### Group 6DD - Verify original native files and recovery

Context has compacted 1 times while executing this group.

{{HARD}}
- [X] 6DD.1 Verify both ordinary native readers/pure next-state checks and meaningful refusal/recovery cases on the actual normal-start sealed,working,loaded and settled/final-use originals.

Sequential6DD staging,2026-10-19:the retained work receipts contain settled endpoints and useful loaded intermediates,but no saved active boring/loaded spoil checkpoints. Runtime traces alone cannot supply those files. Complete the following bounded scopes in order;keep the original6DD.1 until all pass.

### Local Mandatory Reading - Original working checkpoint bank

_Applies to6DDB;supplements Global/Playable loops Area and6DD Local._

Consume exact sealed Standard readiness and its public full-frame boring/control producer. Save actual working cutter and loaded Surface-hauler states only between complete ordinary frames,with public pause/resume. Retain complete native pure pairs and untouched next opportunities. Bind these branches to their original readiness;do not replace the selected completed mainline.

### Group 6DDB - Retain real active and loaded normal-world originals

{{MEDIUM}}
- [X] 6DDB.1 Retain predecessor-bound actual active cutting and loaded spoil-haul native files,pure repeats and untouched full-next baselines in both builds.

6DDB closure,2026-10-19:[actual working-file receipt] (historical reference: `../../../../logs/acceptance/bootstrap/20261019-native-01/working-files-verification.json`),SHAdeabd77e157d3b4e9656550137972cbe47bd4d109dbd4f0c653edf550a872ee8,binds2actual original Standard branches/178524assertions. Each retains a108-tonne loaded cutter after actual first cutting and a162-tonne Surface hauler after public partial return,with four126-chunk pure pairs and exact immediate full-next baselines. Complete original packets,clock/RNG/ID and native material owners remain saved;both branches then physically finish9cuts/108Earth/216Rock/1080work cost,two exchanges,864fees and full empty withdrawal. The existing whole paid-work sealer verifies all13expense categories and actual4Earthwaiting stock. These are disclosed readiness-bound recovery branches;the selected completed mainline remains unchanged. Debug44/optimized17guarded one-worker builds pass,minimum physical availability10836582400/13115166720bytes;all native/compiler children are reaped. Continue6DDC complete ordinary readers,then6DDD physical refusals/recovery before parent6DD.1closes.

### Local Mandatory Reading - Complete ordinary original-file readers

_Applies to6DDC;supplements Global/Playable loops Area and6DD Local._

Consume exact6DDB files and sealed/settled/useful originals from immutable receipts. Reuse SaveOrLoad(Check),ordinary application loading and complete GameLoop opportunities. Compare all126native chunks,including complete DATE clock/RNG/ID,packets,reservations and every saved owner,across both readers,pure writes,reloads and independent next baselines. No private fixture constructor or field exclusion.

Native entry characterization,2026-10-19:StateGameLoop intentionally refills saved landscaping allowances even under Normal/LinkGraph pause. Pure read/preview must execute in the actual startup callback before any such frame. When afterload restores an unfinished graph worker,wait that worker and invoke its existing StateGameLoop_LinkGraphPauseControl owner to clear only its transient pause before inspection;never assign pause/allowances or run substitute clocks. Subsequent next evidence uses the full ordinary GameLoop/InputLoop/UpdateWindows opportunity. The two too-late initial probes are excluded,not accepted with a PLYR exception.

### Group 6DDC - Verify complete ordinary normal-world readers

{{HARD}}
- [X] 6DDC.1 Accept exact sealed,working,loaded spoil,settled,loaded useful and final-use originals through both ordinary readers/preview/pure repeat/reload and complete next-state comparisons.

6DDC closure,2026-10-19:[ordinary-file acceptance] (historical reference: `../../../../logs/acceptance/bootstrap/20261019-native-01/ordinary-files-verification.json`),SHA4fc54a2f9e6312cc434df0a691bc23cc12580abdd2c67bba40dd660e8cec3d6e,verifies192isolated native cases/4834assertions and744complete126-chunk comparisons. Both readers accept all24exact original files in12physical phases per lineage:sealed Standard/Deep/irregular,settled Standard/Deep/irregular,paid layout,ready useful,loaded useful,final useful,active cutting and loaded spoil. Each performs untouched baseline,preview/read,pure repeat,next and reload-next;72additional complete cross-reader next pairs pass. Actual working/loaded originals also match their previously retained uninterrupted6DDBnext frames. No chunk/field is excluded;complete saved clocks/RNG/IDs,packets,reservations,physical work,receiver stock,cash,power and all other native owners match. Initial unpaused generation remains the exact ancestor receipt rather than a relabelled paused physical stage. Debug47/optimized19guarded one-worker builds pass;all192children and compiler roots are reaped. Existing six acceptance-runner refusal tests also pass. Continue6DDD actual original refusal/loaded recovery before parent6DD.1or genuine6DEpublication closes.

### Local Mandatory Reading - Actual original-world refusal and recovery

_Applies to6DDD;supplements Global/Playable loops Area and6DD Local._

Use exact loaded originals and delivered public ground controls. Prove native refusal leaves original owners unchanged for solid ordinary traffic and unsafe occupied final conversion,then recover loaded spoil through real disposal/empty withdrawal and preserve useful final-use service. Bind actual saved outputs and complete material/cash owners;no cargo/phase/position assignment.

### Group 6DDD - Verify meaningful original refusal and recovery

{{HARD}}
- [X] 6DDD.1 Prove actual normal-world solid/unsafe conversion refusal and loaded recovery,then close6DD.1 only after6DDB/6DDC and these physical cases pass.

6DDD/6DD closure,2026-10-19:[original refusal/recovery handoff] (historical reference: `../../../../logs/acceptance/bootstrap/20261019-native-01/original-recovery-verification.json`),SHA520a5d841c6e57f1d81f3cea417924dbdda3dc792c818f2e8db48b4bbce90d9a,binds4actual native refusal cases/122assertions and12complete unchanged126-chunk comparisons. Both actual active worlds admit their cutter but refuse ordinaryTrain2,rail on original solid30574,permanent conversion of genuinely vacant active support25183,and premature retirement. Both actual final-use worlds refuse occupied depot demolition and a140625capsule purchase against107062/77072cash,with native insufficient-cash error and no changed cash/packet/vehicle/terrain/RNG/clock owner. QueryCost distinguishes pure price from ordinary estimates,which themselves check funds;the two earlier incorrectly assumed positive-quote probes are excluded. The [actual loaded recovery] (historical reference: `../../../../logs/acceptance/bootstrap/20261019-native-01/loaded-recovery-verification.json`),SHA58cb946c70cbbab888d4ca19f1e704a3c22a903bccfc8b49884f167657d38532,adds2native cases/82839assertions. Reloading actual162-tonne hauler checkpoints at cursor3/54Earth/108Rock then earns remaining cuts,public settled Resume,real disposal and whole empty withdrawal:108Earth/216Rock/1080work cost,864fees,104disposedEarth+4waiting,216crushedRock=162aggregate+54residue,all8spoil wagons empty and all13expenses reconciled. Exact original jobs/resources/Deep plane/IDs and all36earned cells remain. Combined with6DDB's real checkpoint bank and6DDC's192cases/744whole comparisons,this closes original6DD.1. Debug51/optimized20guarded one-worker builds pass;all children are reaped. Continue6DE genuine existing-runner publication before6D.1/6D.2/original6.1/master11.4close.

### Local Mandatory Reading - Genuine normal-start runner publication

_Applies to6DE;supplements Global/Playable loops Area and6D Local._

Consume exact completed6B/6C/6DB/6DC/6DD receipts and inspect native_ground.py's existing scenario/evidence/classification exports. Extend the one runner/schema owner with a genuine normal-start predecessor-bound S1/S2 path only after every required physical/final-use/recovery proof passes. Require all exact input/source/binary/log/save identities and complete material/cost owners. Older injected/incomplete records must still refuse bootstrap claims;no parallel runner authority or relabelling of isolated invoices.

### Group 6DE - Publish genuine first-milestone acceptance

{{HARD}}
- [X] 6DE.1 Publish complete genuine predecessor-bound S1/S2 acceptance through the existing runner,verify focused provenance/refusal gates and close6D.1/6D.2/original6.1only when every selected requirement passes.

Sequential6DE staging,2026-10-19:all physical/read/recovery predecessors now pass. Separate exact immutable lineage/material/cost proof from the public runner/schema consumer before implementation;neither alone closes the original6DE.1.

### Local Mandatory Reading - Exact normal-start proof graph

_Applies to6DEB;supplements Global/Playable loops Area and6DE Local._

Consume all exact generated,financing,paid readiness,Standard/Deep/irregular work,layout,supply,useful operation and6DD receipts. Verify every physical predecessor edge,original native ID,retained save/log/config/report/package identity and actual conserved material/cash endpoint. Dated binary/test-source bindings were verified when each immutable receipt sealed;do not pretend overwritten historical binaries are still the same bytes. Current production owners must remain the named delivered implementations. Publish a bounded proof artifact for the existing acceptance runner to consume,not another scenario authority or an acceptance claim by itself.

### Group 6DEB - Bind the complete genuine original-world proof

{{HARD}}
- [X] 6DEB.1 Validate and retain the complete exact normal-start predecessor graph,physical quantities and actual cost owners needed by S1/S2,including the original loaded-file/refusal/recovery evidence.

6DEB closure,2026-10-19:[complete prerequisite graph] (historical reference: `../../../../logs/acceptance/bootstrap/20261019-native-01/normal-start-complete-prerequisites.json`),SHAf4a20cd530257ae7fe56d7496409d2971736f3140644692b754bdb28e4ef11f4,binds all17exact physical/file/recovery receipts and re-verifies1658retained artifact identities,44current production-source identities and84actual mainline native cases/852729assertions. Every native save/pure repeat,report/log/config/startup/package and generation-to-useful predecessor edge remains exact,including each original native ID. The110dated overwritten test/binary bindings retain their sealed historical meaning;they are not asserted to be today's bytes. Actual native cash/loan steps telescope from100000cash/100000loan through200000additional ordinary borrowing to107062/77072cash and300000loan,net192938/222928expense;existing native invoices/13-category ledgers remain their sole cost owners. Final112paid cells,264Earth+1180Rock/4068fees,264disposedEarth,1176crushedRock=882aggregate+294residue plus4waitingRock,20empty spoil wagons and64useful deliveredCoal per original pass. A second independent payload verification reproduces the exact proof. This prerequisite is explicitly bootstrap_claim false;6DEC's existing public runner must still execute current native verification and focused negative gates before genuine acceptance.

### Local Mandatory Reading - Existing-runner genuine acceptance

_Applies to6DEC;supplements Global/Playable loops Area and6DE Local._

Consume6DEB's exact complete proof through acceptance/native_ground.py's existing public run_scenario/capture_ledger/compare_state_digest/validate_art_manifest owner and versioned evidence record. Add an explicit genuine normal-start path for S1/S2;execute current bounded native verification and retain original physical/material/cost facts. Preserve every older isolated fixture classification and its bootstrap refusal. Focused negative gates must reject altered/incomplete/mislabelled/wrong-lineage proof,not merely check exported names. Close original6DE.1/6D.1/6D.2/6.1only after actual public runner records pass in both builds.

Context has compacted 1 times while executing this group.

### Group 6DEC - Publish genuine S1/S2 through the public runner

{{HARD}}
- [X] 6DEC.1 Extend the existing evidence owner with explicit genuine normal-start S1/S2 selection,verify current native acceptance and focused provenance/refusal gates in both builds,and close the original milestone only after the complete records pass.

6DEC dependency return,2026-10-19:the first public genuine S1 record correctly refuses at the existing art gate:all four selected native gallery receipts still bind the old src/tests/CMakeLists.txt. Required separate normal modal/file/refusal translation units changed that file to keep compilation memory bounded. The read-only source audit finds exactly that one changed source in every role;no artwork,renderer,English,GRF or gameplay source changed. Unlike the earlier unnecessary header listing,this source registration is required. Dispatch Plan14-84F's exact affected native observer refresh/new named publication before resuming6DEC. Keep every producer-source/PNG/SAV/log guard strict;no relaxed hash,restored fake build list,substitute gallery or passing public record is authorized by this failed attempt. All native/compiler processes are closed;Debug51/optimized20remain current.

6DEC/6DE/6D/original6.1 closure,2026-10-19:[genuine public acceptance](../../../../../../../../src/tests/fixtures/checkpoints/underground/openttd3-75ea75718f/historical/acf7837bcf2d0f5d-genuine-bootstrap-verification.json),SHAacf7837bcf2d0f5dd9506cde1f393abcee8a12d37631a05d2a127f6316711846,binds four actual schema2 public S1/S2 records through the existing run_scenario owner,both Debug51/optimized20. Each explicitly consumes exact complete f4a20cd... prerequisites,retains real_bootstrap/shortcuts[] and uses four fresh isolated ordinary native readers.16current native cases/388assertions pass56complete126-chunk original/pure/untouched-next pairs plus four independent S1/S2 inspection/next pairs;zero ignored owners. Canonical capture_ledger now handles current nested native434 tables and records all126owners plus actual vehicles/packets/company/project/exchange/support/portal/job/tube/utility tables. The modern decoder is selected explicitly for434;the actual older380scoped46-owner ledger remains readable and refuses the complete434contract. Seven genuine proof/lineage/cost/decoder refusals and six existing public evidence-boundary tests pass. The17exact physical/file/recovery receipts retain84historical mainline cases/852729assertions without counting the same predecessors again for each public scenario. Both original112paid cells/1444spoil tonnes/4068fees/20empty wagons/64useful deliveredCoal and complete cash/loan ledgers remain accountable. Required Plan14-84F strict gallery refresh/publication is X;all native/source/pixel guards remain strict,69selected families/10557poses are validated by each public call. Every native child is bounded/reaped;no production/schema434/GRF change. Earlier absent-proof/art-return statements describe superseded checkpoints. Continue master11.6 representative performance;human art/demo and unrelated whole orbital scopes remain pending.

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

Group9 scope: missing tube/thermal loss in9.2 is skipped under GP-TG; normal-newgame scenario provenance uses GP-BT. Orbital target loss remains outside this master. [Ground lifecycle] (historical reference: `../../../../logs/r2mlvs3mp-z-b-ground-lifecycle-checks.log`) and [control/recovery] (historical reference: `../../../../logs/r2mlvs3mp-z-b-ground-control-recovery-checks.log`) are real owner tests, not complete whole-profile S8.

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

Group12 goal exception GP-TG:12.5 actual tube/plant final use is skipped with its absent consumers. Remaining ground evidence passes [6,021 geometry assertions/nine cases] (historical reference: `../../../../logs/r2mlvs3mp-z-b-ground-turn-checks.log`) and [136,634 movement/control/script/recovery assertions/16 cases] (historical reference: `../../../../logs/r2mlvs3mp-z-b-ground-control-recovery-checks.log`), with actual current Plan15 serialized proof. Whole mixed scenario claims retain pending boxes.

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
| [src/industry_cmd.cpp](../../../../../../../../src/tests/fixtures/cleanup/source-epochs/openttd3-75ea75718f/5316863e999d8f04-industry_cmd.cpp) | 1152-1240 | `ProduceIndustryGoodsHelper` and `ProduceIndustryGoods` expose complete rate/callback production paths; augment output once before authoritative monthly accounting. |
| [src/timer/timer_game_calendar.h](../../../../../../../../src/tests/fixtures/cleanup/source-epochs/openttd3-75ea75718f/1c2e911d6fe35ac2-timer_game_calendar.h) | 15-39 | `TimerGameCalendar` technology-clock contract and date conversions govern 1980/model/factory introductions; economic/realtime scheduling cannot unlock a technology. |
| [src/rail.h](../../../../../../../../src/tests/fixtures/cleanup/source-epochs/openttd3-75ea75718f/6e6501df4baaa8f2-rail.h) | 178-182 | `powered_railtypes` and `compatible_railtypes` have distinct invariants; ordinary/TBM/tube locomotion needs explicit power and compatibility checks. |
| `src/tests/multilayer/acceptance/native_ground.py` (planned public contract; link after implementation) | Live definitions; no fixed range | [r2mlvs16-integrated-acceptance-and-balance.md](r2mlvs16-integrated-acceptance-and-balance.md) Groups 1-5, ending with Group 5's public-contract task, publish `run_scenario`, `capture_ledger`, `compare_state_digest`, `validate_art_manifest`. These exports/schema fields are authoritative for this area. Same-plan consumers follow those earlier groups; cross-plan consumers verify the producer's required implementation/tests as well. Proposed names alone do not prove delivery. |
| [src/framerate_type.h](../../../../../../../../src/tests/fixtures/cleanup/source-epochs/openttd3-75ea75718f/d94650a37e44178f-framerate_type.h) | 100-119 | Complete `PerformanceAccumulator` contract defines summed multi-step measurements and explicit reset boundaries; distinguish simulation cost from rendering and reset once per measured frame/step. |

**ASSUMPTION: this was already built previously. If not, re-asses at runtime**. This applies only to the producer contracts named in this area's gate, after their listed producer groups have run. Verify their actual exports and focused tests before using the sketches below; proposed types and helper names in sketches are not evidence of delivered code.

**Measured tuning workflow.** Define the comparison workload and objective, record the baseline ledger/profile, change one canonical parameter or bounded-work implementation, and rerun only the affected scenario plus relevant regression. Prices, capacity and throughput may be tuned; the required circle radius, turn rule, five distinct models and solar multiplier remain invariant.

### Group 16 - Measure complete ground project economics

{{HARD}}
- [ ] 16.1 Create a ledger schema with time basis, initial investment, active/idle operating cost, support, haulage, disposal, accepted output and net balance. Record all quantities and bottlenecks from fixed seeded runs.
- [ ] 16.2 Measure a standard corridor/chamber and a harder Deep project through several real wagon cycles. Separate one-time build cost from repeated operating expense and include receiver saturation delays.

**R2MLVS3MP note, 2026-10-05.** The solid starter charges the whole 3x3 pocket in cut 0 (12 cells, not 3). This supersedes the cleared-area assumption in [r2mlvs1-e](../../../../../../../../../OpenTTD3/docs/zz-reach2/upgrades/2026-10/r2mlvs1-e-ground-and-space-budgets.md) line 32. Measure the larger first-cut cost and the minimum consist it implies (two Earth and two Rock hoppers on the soft-ground fixture site) ([r2mlvs3mp-f](../../../../../../../../../OpenTTD3/docs/zz-reach2/upgrades/2026-10/portal-tbm/r2mlvs3mp-f-solid-starter-contract.md#capacity-consequence)).
**R2MLVS3MP handoff, 2026-10-05 (r2mlvs4 Group 20).** First-cut measurements from `[tbm-measure]` (Debug, fixture site, 1980, base currency units): standard cutter with 2 Earth/2 Rock hoppers, purchase 400,000, first cut 888 ticks, charge 360, 216 t of spoil, running cost 3,945 over those ticks; Deep cutter with 2 Earth/4 Rock hoppers, purchase 800,000, first cut 1,184 ticks (1,776 work ticks at 3/2 per tick), charge 960, 504 t, running cost 11,397. Whole-corridor time with real haulage and disposal follows the movement work (R2MLVS3MP Group 7B). These are inputs, not a balance verdict.
**R2MLVS3MP handoff, 2026-10-06 (r2mlvs4 20.1 complete).** [Complete project ledger](r2mlvs4-t-complete-project-ledger.md): medium native prices, unit inflation, seed 420, calendar fixed at 1980-01-01, actual 74-tick stopped startup. Standard/Deep both cut 36 swept cells in nine cuts and physically deliver all spoil through two/three source exchanges. Standard takes 10,620 ticks with setup 2,101,454, TBM purchase 403,124, cut/support 1,080, active 38,176, idle 39, haulage 952 and receiver fees 1,728: total 2,546,553. Deep takes 32,926 ticks with setup 2,106,196, purchase 804,686, cut/support 2,880, active 232,758, idle 151, haulage 2,964 and fees 4,464: total 3,154,099. Every component balances against actual native cash/expense receipts. All packets reach real Surface receivers. This is producer evidence for 16.1/16.2; the required acceptance runner, saturation cases, payback, complete route and performance/art proofs still govern those pending tasks. No tuning change or whole-acceptance verdict is claimed.
- [ ] 16.3 Measure each mineral-family route from finite reserve through Surface processing and final useful output. Compare like-for-like distances/capacities and document missing consumers as failures.
- [ ] 16.4 Calculate payback/sensitivity from observed complete loops, not nominal vehicle price. Send proposed numerical changes to r2mlvs1 with the specific failed balance objective.
- [X] 16.1G Publish schema2 actual ground invoices with time/seed/date, full setup/purchase/work/support/active/idle/haul/disposal/material/remaining-cash ownership, sourced from actual paid loops.
- [X] 16.2G Verify standard/Deep corridor rows through two/three actual physical wagon exchanges and real Surface disposal; retain independent actual bounded saturation/resume evidence. Mineral/future whole-chain and detailed sensitivity rows remain pending.

Ground economics closure,2026-10-08: [current Debug invoice record] (historical reference: `../../../../logs/multilayer-acceptance-s1-final.json`) verifies the canonical standard/Deep economics fixture. Standard10,620 ticks/2,546,553 and Deep32,926/3,154,099 retain their exact seven native invoice categories and216/432 versus72/1,440 tonnes Earth/Rock. All packets finish at actual receivers, train spoil revenue is zero, and company cash balances. [Receiver fee/no-income/saturation/recovery] (historical reference: `../../../../logs/r2mlvs3mp-z-b-spoil-no-profit-saturation-checks.log`) passes67,045 assertions/nine cases. The full16.2 saturation-cost sensitivity and16.3/16.4 whole mineral payback remain beyond this corridor subset; no fictional output revenue or fixture-bootstrap verdict is credited.

Re-measured 2026-10-16 ([master task 10R.3](r2mlvs3mp-master-plan.md#group-10r---re-baseline-the-underground-after-the-demo-fixes)): the closure above predates R2TBMF's pacing and yields (2026-10-09). The current [S1](../../../../../../../../../OpenTTD3/mlm/reviews/acceptance/native-ground/20261016-093620-04c966649c/reports/ground-s1-rebaseline.json) and [S2](../../../../../../../../../OpenTTD3/mlm/reviews/acceptance/native-ground/20261016-093631-fdc10fd9d1/reports/ground-s2-rebaseline.json) schema2 invoices (RelWithDebInfo) record Standard 9,620 ticks / 2,539,452 with 108/216 t Earth/Rock, and Deep 19,335 / 3,066,086 with 36/720 t. All seven invoice categories still balance, every packet finishes at a receiver, and spoil revenue is zero. The fee, no-profit and saturation tags pass alone, 53,866 assertions / nine cases ([report] (historical reference: `../../../../logs/underground-rebaseline/no-profit/20261016-113715-b1f6a5d69b/focused-tags.json`)). The halved yields freeze fewer breakpoints, so the rows now pass through one (Standard) and two (Deep) physical exchanges instead of the two/three that 16.2G names. 16.2G stays checked as the corridor-row verification. The Standard corridor alone no longer shows the several wagon cycles that the open 16.2 asks for. Full table: [complete ledger](r2mlvs4-t-complete-project-ledger.md#current-ledger-re-measured-2026-10-16).

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
- [X] 21.1G Record selected ground hardware/build/counts,compact128/representative512/largest admitted1024Debug2048optimized,warmup and individual tick/command percentiles through21C-21F.
- [X] 21.2G Freeze selected ground budgets from actual feature-disabled baselines before active work;verify incremental memory,ticks,route and command limits without later increases.
- [X] 21.3G Verify actual original/large-map file sizes,synchronous saves,first resumed native work and16real peer reconstructions under the same fixed selected budgets,with explicit uncertainty.

Selected21.1G-21.3G closure,2026-10-20:the former GP-PF/GP-NJ absence is historical for this ground scope. The exact21F aggregate below closes the selected rows;whole mixed-profile tasks remain unchecked.

### Local Mandatory Reading - Delivered utility bound measurement

_Applies to Group21B; independent bounded-owner subset of master11.6._

Read Global/Economy and performance Area, `src/tests/utility_power_performance.cpp`, utility source/consumer/connection limits and actual allocation/retirement APIs. Benchmark real sparse0/32/256/4,096-account ledgers, one warmup and five equal intervals, separately from setup/expiry/rendering. Nanoseconds are performance observations, never simulation time. Report retained payload bytes separately from unmeasured RSS/temporary peaks. This cannot establish representative/max-map, dense simultaneous-job, route-planning or real join budgets.

### Group 21B - Measure the delivered utility owner bound

{{MEDIUM}}
- [X] 21B.1 Define and run fixed bounded real source/consumer workloads in Debug and optimized configurations, preserving quantity/lifetime/retirement correctness.
- [X] 21B.2 Record hardware/configuration, warmup/sample policy, allocation median/maximum, setup/retirement time and retained payload estimate with explicit uncertainty and unmeasured scopes.

21B closure,2026-10-08: Ryzen7 PRO5850U, eight cores/16 threads,29,878,575,104 bytes physical memory; current repository Debug/RelWithDebInfo executables. Real0/32/256/4,096 sparse account/edge workloads use one full-interval warmup and five equal allocation samples, with source publication/expiry outside timing. Both configurations pass78,980 assertions/one generated case. [Debug] (historical reference: `../../../../logs/r2mlvs3mp-z-b-utility-performance-debug.log`) max-account median/max are37.024/38.115ms; [optimized] (historical reference: `../../../../logs/r2mlvs3mp-z-b-utility-performance-optimized.log`) are3.822/3.996ms, setup10.035ms and retirement0.437ms. Optimized retained payload estimate is1,327,216 bytes; allocator/RSS/temporary peaks are unmeasured. Empty-account optimized allocation median is300ns, a subsystem baseline only. Against the native27ms timer opportunity the optimized sparse bound fits; Debug instrumentation exceeds it and is reported, not silently called a budget pass. Five samples cannot establish robust tail percentiles. No rendering, real dense-job/map/path workload or join profile is claimed; no production tuning/optimization follows these passing sparse-bound results.

Sequential ground performance return,2026-10-19:master11.4 now closes genuine public normal-start acceptance. Earlier GP-PF/GP-NJ absent-harness dispositions describe historical finalization. Stage the selected ground workload/baseline,active-owner measurement,file/join and attribution scopes below before closing master11.6. Keep original mixed orbital requirements pending. Actual process peaks and a memory reserve are mandatory after the user's RAM restart;retained payload alone cannot substitute peak measurement.

### Local Mandatory Reading - Ground workload and disabled baseline

_Applies to21C;supplements Global/Economy and performance Area._

Inspect ordinary generation/profile selection,Map packed allocation,GeologyWorldState/StorageBytes/IndexPayloadBytes and the normal application's startup/full GameLoop dispatch. Use explicit compact128,representative512 and increasing1024/2048/4096candidate squares with seed420/calendar1980,both Classic-disabled and current Multilayer profiles. One warmup128ticks then five128tick intervals;retain every sample/nearest-rank percentiles. Measure startup-to-ready separately from actual simulation,input/window dispatch and synchronous save. Whole-application startup includes content/generation overhead and must not be relabelled generation-only. Measure actual Windows process working/private peaks and6GiBphysical/8GiBcommit reserves;stop/reap only owned children. Maximum feasible means the largest candidate actually admitted/completed under the110s/process-memory policy;an unattempted/unadmitted size is not a pass. Record actual native entity/job/packet/utility counts and packed/geology payload separately from process peaks. Freeze budgets only after measured disabled baselines;empty maps do not satisfy dense active stress.

### Group 21C - Measure ordinary maps and feature-disabled baselines

{{MEDIUM}}
- [X] 21C.1 Define exact repeatable ground map/sample/resource protocol and observe real Classic/Multilayer ordinary maps in both native builds,retaining actual peak memory,workload counts and uncertainty.
- [X] 21C.2 Use those measured disabled baselines to declare incremental memory,tick,command/path and file/join budgets for selected ground workloads;leave active/file/join acceptance to their measured groups.

21C closure,2026-10-20:[ordinary baseline verification] (historical reference: `../../../../logs/performance/ground-maps/20261019-native-01/ordinary-baseline-verification.json`),SHA27c6131b6952f0174b9155d6fdc7d484fb92c46dbc90c69ac63a9200d68db264,binds all20actual profile/build/size attempts.14complete native cases retain8960measuredticks/five128tick intervals each,all tick/input/window samples,real counts,28byte-identical whole native save pairs,actual Windows lifetime working/commit peaks/10msprivate/system samples and matching physical content packages. Both profiles/builds pass128/512/1024;optimized also passes2048. Six larger workflows time out safely at110s and are retained/reaped as failures. Largest complete workflow under the declared110s/3GiBprivate/6GiBphysical+8GiBcommit policy is1024Debug/2048optimized,distinct from engine hard maximum4096. No RAM crash,full maximum-map pass or isolated generation-only timing is claimed. Five actual corruption/incomplete-data refusal tests pass1.533s. Frozen future budgets use measured Classic baseline plus27msoptimized p99/54msmaximum simulation increments(Debug10x diagnostic),256MiBincrementalcommit under the3GiBceiling,command2/route4opportunities,15soptimized/30sDebug synchronous save,80sstartup/load and45sreal join. Debug53/optimized21guarded one-worker builds pass;all children reaped. Continue21D active/dense work,21E actual files/joins and21F attribution before master11.6 closes. Desktop/cache/timer overhead remains explicit uncertainty;empty maps cannot substitute dense activity.

### Local Mandatory Reading - Active ground owner stress

_Applies to21D;supplements Global/Economy and performance Area and21C protocol._

Consume declared baseline/budgets and actual excavation/project/route,packet,utility and tube owners. Separate explicit prepared stress fixtures from genuine normal-start originals. Exercise bounded dense job/route planning/commands,actual active tick work and simultaneous selected utility/vehicle counts,including disabled/hidden-view comparisons. Attribute operation timings to existing owners;no camera-based skipping,private duplicate quantity registry or unmeasured fast-path claim. Retain samples,physical owner correctness and memory before changing an implementation.

### Group 21D - Measure dense ground work and command paths

{{HARD}}
- [X] 21D.1 Measure real active original-world full-tick workloads and reproducible bounded dense excavation/route/command stress with actual native counts and retained physical correctness.
- [X] 21D.2 Measure selected utility/tube scaling independently and together with ground work,attributing payload/temporary peaks and simulation cost separately from rendering.

Sequential21D staging,2026-10-20:separate original-world full application sampling,prepared dense job/route/command owners and utility/tube/view scaling before implementation. These consume21C's fixed budgets;none alone closes original21D or master11.6.

### Local Mandatory Reading - Original active-world performance

_Applies to21DB;supplements Global/Economy and performance Area/21D Local._

Consume exact21C baseline receipt and6DD original-file bank;select actual working-bore,loaded-spoil,loaded-useful and final-useful originals in both builds. Use existing ground_performance.py/full native driver with explicit original input,ordinary load/public unpause and128warmup+640measured ticks. Preserve all input bytes and complete native pure writes;verify actual original job/material/packet/cash owners and native progress against frozen budgets. Performance observations remain separate from the existing genuine-bootstrap claim. Do not privately reset work,cargo,company,clock or utility state to make a sample fast.

### Group 21DB - Measure real active original worlds

{{HARD}}
- [X] 21DB.1 Measure actual working,loaded and useful original-world full native dispatch in both builds,verify retained physical/cost continuity and fixed budgets,and retain its bounded technical scope.

21DB closure,2026-10-20:[original-world performance verification] (historical reference: `../../../../logs/performance/ground-maps/20261019-native-01/original-performance-verification.json`),SHA5cd7cfe3987218597147bfdbcadc4a596cde33d28fc279ac96e8f309825063bc,binds eight exact working-bore/loaded-spoil/loaded-useful/final-useful originals,both native builds,12464assertions/5120measuredticks and16complete126-chunk pure pairs. Every declared tick/max/memory/save/startup budget passes;optimized worst observed simulation p99 is88200ns. Working jobs actually advance cursor1to2 and retained Earth/Rock36/72to45/90. Actual162-tonne loaded haulers unload through native receivers:54Earth=48disposed+6waiting;108Rock=96crushed+8waiting+4held. All actual packet references/quantities/zero spoil feeder shares and real stock remain conserved;both loaded useful capsules physically move and update tube/utility owners. Final-useful is explicitly a completed idle control. All13native expense deltas exactly explain cash change with300000loans and unchanged prior-year ledgers. Seven initial reads preserve all126original chunks;optimized loaded-spoil has only the real restored LinkGraph pause1to65,verified as exactly one DATEfield/one rawbyte/bit64 with every other native byte unchanged. No saved-field assignment or ignored owner is used;full cache-ready/load latency remains21E's distinct task. An early sealer assumed every empty vehicle's default cargo_type denoted a spoil hopper;it correctly refused and was corrected to require actual loaded packet owners,not change any native data. All eight native children reaped. Continue21DC dense job/route/command stress,then21DD utility/tube/view scaling;original21D/master11.6remain unchecked.

### Local Mandatory Reading - Dense job and route bounds

_Applies to21DC;supplements Global/Economy and performance Area/21D Local._

Inspect canonical excavation job/claim/route limits,PlanExcavationRoute and actual command/participant owners. Prepare explicitly classified stress worlds through delivered APIs;retain actual IDs,counts,payload and pure deterministic planning/command refusals. Measure dense retained/live job lookup,legal and limited route searches and command latency with warmup/equal samples against21C budgets. A prepared stress world is not normal-bootstrap proof. Attribute any excess before changing an owner.

### Group 21DC - Measure dense ground jobs and planning

{{HARD}}
- [X] 21DC.1 Measure bounded dense ground job/claim/route/command workloads with real owner counts,deterministic results and fixed-budget comparison;close original21D.1 only after21DB also passes.

21DC/original21D.1 closure,2026-10-20:[dense owner/planning acceptance] (historical reference: `../../../../logs/performance/ground-maps/20261019-native-01/dense-planning-verification.json`),SHAc53a31e13ee4a97b074b8bff789ad5a421547586e7ea0ffeae84800dc4436a87,binds the exact earlier68dense stress cases/163320assertions,ten focused producer cases/8384assertions and20new full-search/current-hardness cases/48372assertions. Explicit512/1024/2048prepared maps expose0/32/128actual reservations/up to1152claims,64warmup+5x64owner-work rounds,320pure reservation estimates/five10000-query bucket batches and actual acquired-project native command estimates. These controlled support/work records do not pretend to be real train/material commits;actual physical full-loop work remains exact21DB. Short-query max75expansions did not complete the planner bound. The new pure long-target queries exercise default4096-state/8192-expansion and accepted8192-state/32768-expansion inputs at512/2048and0/128jobs,retaining visible bounded partial/refusal results,whole deterministic returned geometry and unchanged geology/RNG. Current regional-hardness1queries select genuinely admitted native paths without a privately invented route. All original fixed budgets pass in both builds;no owner optimization/tuning is justified by these passing rows. Six original corruption gates and six full-search count/hardness/sample/budget/peak gates pass;latest1.867s. Five full heavy queries report median/observed maximum,p99equalsmaximum and is not a robust tail estimate. Debug55/optimized23guarded one-worker builds pass;all children reaped and no production/CMake/GRF change. Dated overwritten test/binary bindings retain their sealed earlier meaning;physical artifacts/current production guards remain strict. Combined with21DB,this closes original21D.1. Continue21DD utility/tube/view scaling,21Efiles/actualjoins and21F attribution before master11.6closes.

### Local Mandatory Reading - Related utility and tube scaling

_Applies to21DD;supplements Global/Economy and performance Area/21D Local._

Consume actual utility producer/consumer/share/connection limits and tube vehicle/path/reservation owners,including delivered utility_power_performance.cpp. Scale actual selected accounts/links/vehicles independently and alongside ground activity,retaining conserved energy/cargo and real memory/timing samples. Compare hidden/selected-view samples at equal simulation ticks without suppressing simulation work. Preserve unrelated orbital/direct-processing scope and fix measured excess only in its owner.

### Group 21DD - Measure utility,tube and view-independent work

{{HARD}}
- [X] 21DD.1 Measure selected independent/combined utility/tube scaling and view-independent ground work with actual counts/quantities,then close original21D.2 only after complete fixed-budget evidence passes.

Sequential21DD staging,2026-10-20:separate the shared utility allocator/settlement/retirement bound,actual physical tube account/vehicle work and combined/view-independent sampling before implementation. Original21DD.1/21D.2/master11.6remain pending until every component passes the existing fixed budgets.

### Local Mandatory Reading - Utility allocation and settlement scale

_Applies to21DDB;supplements Global/Economy and performance Area/21DD Local._

Read utility_power_performance.cpp,utility_power_fixture.h,the shared4096account/16384share/connection limits and Register/Publish/Allocate/PrepareUtilityMutation/CommitUtilityMutation/BeginUtilityPowerInterval/company-retirement APIs. Reuse actual issued producers/consumers and real company expense participants;never synthetic grant IDs or another energy balance. Measure0/32/256/4096accounts with sparse and bounded multi-source eligibility,one full warmup plus five equal intervals. Separate setup/publication/allocation/whole settlement/expiry/retirement timings,retain exact joules and real cash/expenses,measure OS peaks independently of retained payload and compare the earlier fixed budgets. Prepared owner stress remains explicitly distinct from normal bootstrap/actual tube movement. A passing allocator alone cannot complete settlement or physical tubes.

Context has compacted 1 times while executing this group.

### Group 21DDB - Measure the shared utility account bound

{{HARD}}
- [X] 21DDB.1 Measure actual shared utility allocation,settlement,expiry and company-retirement scaling with bounded eligibility graphs,real energy/cash conservation and fixed memory/timing budgets in both builds.

21DDB closure,2026-10-20:[exact native utility receipt] (historical reference: `../../../../logs/performance/ground-maps/20261019-native-01/utility-performance-verification.json`),SHA2bc8deb7260e7ba2963826db28e1f4836e99626dd83ead42a31d04dbe9586d9b,160232bytes.16actual stresscases plus2existingproducer cases/1298872assertions and8focused refusal tests pass. Actual0/32/256/4096sources+consumers and1/4eligibility fanout reach16384eligibilityedges;zero physical links are claimed. Five equal measured intervals after fullwarmup separately retainpublication/allocation/settlement/rotation,then actualunusedexpiry andcompanyretirement. Peraccount216Jgenerated=162Jsettled+27Jexpired+27Jretired;exact30currencyexpense survivesallother38expenseentries unchanged. Allpriorfixedbudgets pass;optimized4096/fanout4allocationmax9534000ns,settlementmax1128200ns. Debug59/optimized25singleworker guardedbuilds pass. Interruptedoptimized24wasstoppedbyguardduringextension-causedkernelpoolpressure;userresolvedpressurebefore25. Allnativechildrenreaped,no production/CMake/GRFchange. Consumeexactouterreceiptafterlaterbinaries/testsourceschange;physicaltube/view/master11.6remainpending. Continue21DDCwithoutcheckpointpause.

### Local Mandatory Reading - Physical tube workload scaling

_Applies to21DDC;supplements Global/Economy and performance Area/21DD Local._

Read actual ConnectTubeExternalPower/BindTubePowerVehicle/PrepareTubePowerTick/AdvanceTubePowerTick and native guideway/capsule/request/reservation limits. Existing TubePowerRig declares fixture-open Connected portal geometry and actual model9/Mole-70 in its historical prototype fixture;it is not a useful route or five-model/bootstrap authority. Actual selected model IDs are9..13,as native transport_tube_engine.cpp proves;the initial0..4assumption was wrong. Use actual selected capsule models,owned physical depots/inlets/paths and native commands/participants for the scaled scope,with explicit declared preparation. Preserve invoices,packet custody,one global epoch and real movement/service semantics;stopped capsules cannot be relabelled moving demand. Measure independent and combined selected counts under the same earlier budgets,not only ComputeDemand arithmetic.

### Group 21DDC - Measure actual tube service and capsule work

{{HARD}}
- [X] 21DDC.1 Measure selected physical inlet/path/account and actual capsule work scaling,including combined ground/utility counts and conserved native money/energy/cargo;retain every unmeasured role explicitly.

21DDC closure,2026-10-20:[exact physical tube receipt] (historical reference: `../../../../logs/performance/ground-maps/20261019-native-01/tube-performance-verification.json`),SHA956f67a0bdbc849f7ef61b495487edf8dc261e08406247f3527fff4319008e9e,952502bytes.26actual workload cases/2562858assertions plus26focused producer cases/12750assertions=52cases/2575608assertions and8focused negative gates1.763spass. Actual256connections,128cells/path,16384aggregatecells,4096nativeboundcapsules/all5models9..13 and0/128partialexcavationjobs pass bothbuilds. Oneglobalpowerpreparation,real540Jdepotserviceswithfunded/waitingcounts,320measurednativeTrainTickmotionframes/case,originalpacketIDs/exactcash/all39expenseentries/nativevalidationretainoneowner. Optimizedworstcombinedp99=20098200ns;maxprocesscommit39559168bytes. Nativebatchtimingexcludesseparatelyretainedobserverchecks_ns;old284761100nsobserver-inclusive Debugexcess remainsdated/excluded,andno productiontuning/budgetincreaseoccurred. Explicitfixturefloor/Connectedportals/funding/nativepaidcapsules/loadedpacketsarepreparedstress,notnormalbootstrap/usefulproduction. Debug65/optimized26guardedoneworkerbuildspass;allchildrenreaped,noCMake/production/GRFchange. Consumeexactouterreceiptafterlaterbinary/testchanges. Continue21DDDactualviewcomparisonwithoutcheckpointpause.

### Local Mandatory Reading - View-independent performance acceptance

_Applies to21DDD;supplements Global/Economy and performance Area/21DD Local._

Use exact original active files and completed utility/tube stress receipts. Compare full native simulation at equal ticks for Surface/Underground/hidden/headless views through existing canonical owners,retaining complete physical ledgers separately from raster timing. No camera may suppress simulation work. Seal the component/combined bounds and close21DD.1/original21D.2 only after all selected rows pass;file/join reconstruction remains21E.

### Group 21DDD - Accept view-independent combined work

{{HARD}}
- [X] 21DDD.1 Verify view-independent active/combined ground-utility-tube work and reconcile the complete21DD/original21D.2scope against exact measured component budgets.

21DDD/21DD/original21D.2 closure,2026-10-20:[exact view/component receipt] (historical reference: `../../../../logs/performance/ground-maps/20261019-native-01/view-performance-verification.json`),SHAbec97d35e4f2822d2a9e08af62d0307a3b37e4a949f4ddc4498edad43d253a78,3884169bytes.32actualfullapplicationcases/50016assertions/20480measuredticks/96complete126-owner comparisons and8focused negative gates1.804spass. Four exactoriginalworking/loaded-spoil/loaded-useful/final-idle worlds run bothbuilds withSurface,Underground,hiddenDeepandreallynon-GUIdriverroles. Logicalviewportretained;allrolesnullraster/noFPSclaim. Everyfinalworldequalsall126chunks oftheearlierphysicallyreconciled original21DBrun at768ticks;noVIEWorotherownerignored. InitialLinkGraphpause1→65 allowedonlyasexactnativeDATEfield/singlebytebit64,allotherinitialbytesoriginal. NativecameraAPI/actualdrivercapability/selectedlayer/raw640ticks/windowcost/OSpeaks/allfixedbudgetsverified. Maxcommit39538688bytes,worstoptimizedsimulationp9993300ns. Exactdated utility2bc8...andphysicaltube956f...receiptsretainallnativequantities/currentunchangedproduction/physicaloutputsafterobserverbuilds. Debug66/optimized27singleworkerguardedbuildspass,allchildrenreaped,no production/CMake/GRFchange. Selected21Eactualfiles/caches/realjoinsand21Fattribution/master11.6remainpending;continue21EBwithoutcheckpointpause.

### Local Mandatory Reading - Performance files and actual joins

_Applies to21E;supplements Global/Economy and performance Area and21C protocol._

Use exact measured workload files and delivered ordinary native loader/real two-process peer transport. Measure save sizes,synchronous writing,whole startup/load/rebuilt caches and actual join reconstruction with identical content/counts;retain complete agreed-frame owners/untouched-next correctness. Sampling and filesystem-cache uncertainty remain explicit. Fresh actual join timing cannot be inferred from a replay or historical ACK interval.

### Group 21E - Measure ground file and real-join reconstruction

{{MEDIUM}}
- [X] 21E.1 Measure actual save/load and reconstructed native caches for selected compact/representative/maximum admitted workloads against their declared budgets.
- [X] 21E.2 Measure real host/client reconstruction and later matching-frame ground/utility owners with process peaks and retained transport/count identity.

Sequential21E staging,2026-10-20:separate actual synchronous files/first-resumed-native-work reconstruction from paired live network reconstruction. Keep original21E.1/21E.2/master11.6 pending until their exact selected children pass;no historical ACK interval or header-only load becomes a fresh reconstruction measurement.

### Local Mandatory Reading - Actual performance files and first resumed work

_Applies to21EB;supplements Global/Economy-performanceArea/21E Local._

Read ground_performance_fixture.h GroundPerformanceSave/startup/sample and ground_performance.py,exact21C baseline largest-completed selections and21DB original filebank. Retain actual compact/representative/largest-admitted Multilayer files perbuild and actual active ground/tube originals. Measure synchronous full native writing and whole fresh-process startup/read/fixup/first-resumed-GameLoop with OS peaks. Explicitly subtract only retained observer initial inspection/save overhead from launch-to-first-work;asynchronous LinkGraph waits remain included. Preserve every126nativeowner on repeated saves and independent untouched-next/full-loop outputs. Report file size and actual owner counts,cache/desktop uncertainty and unqueried/later cache scopes honestly. No filesystem cache flush or synthetic reconstructed cache may manufacture a load result.

### Group 21EB - Measure actual files and resumed native work

{{MEDIUM}}
- [X] 21EB.1 Measure selected actual compact/representative/largest-admitted files and native first-work reconstruction with exact repeated/independent full-owner correctness and fixed file/load/memory budgets,then close original21E.1.

21EB/original21E.1 closure,2026-10-20:[exact file reconstruction receipt] (historical reference: `../../../../logs/performance/ground-maps/20261019-native-01/file-performance-verification.json`),SHA10d63ef739d31bbb98230dde0505ae73d30a7ae5d079dafabd003d78dbdf0adf,3445684bytes.28actualfreshnativecases/43652assertions/17920measuredticks/8focused negative gates pass.14exactinputs:ordinaryMultilayer128/512/largestcompleted1024Debug2048optimizedandfouractualcompactoriginalstages/build,eachreadbytwofreshprocesses.56compressedwhole-filepurepairs,14complete126-ownerindependentfinalpairsand16complete126-owneroriginalactivefinalpairsretainallnativeauthority. Input/initialall126nativeownersareexactexcepttheexplicitoneDATEfield/singlebytebit64LinkGraph1→65restorewait;actualfileDATEclocksalsomatchnativeobservations. Wholelaunch/read/fixup/firstcompletedGameLoopincludesrealasynchronouswaits,subtractingonlyretainedinitialobserverinspection/pure-save/unpauseoverhead. Allpriorfile/load/tick/command/OSpeakbudgets pass. Explicitordinarylargegeometryversuscompactactiveowners,uncontrolledfilesystemcacheandonlyactuallyexercisednativecachesremainhonestlimits. Debug67/optimized28guardedsingleworkerbuildspass/allchildrenreaped;no production/CMake/GRFchange. Continue21ECfreshrealpeerperformancewithoutcheckpointpause.

### Local Mandatory Reading - Real peer reconstruction performance

_Applies to21EC;supplements Global/Economy-performanceArea/21E Local._

Read delivered RunGroundNetworkPeer/SaveGroundNetworkOpportunity,ground_network_trace.py transport/config/authentication and compare_frames,plus exact closed original peer receipts. Use actual loopback host/client processes with identical content/counts on selected admitted maps/active original ground and utility worlds. Retain actual client-launch-to-native-reconstructed-first-work timing and later agreed-frame complete126nativeowners,with only seven documented NoNetworkSync unit fields separated. The unavoidable two live peers require shared6GiBphysical/8GiBcommit reserves,per-child ceilings/bounded lifetime/finallyreap and no concurrent build. Never infer fresh join performance from replay,staged state,old ACK timing or no-desync RNG alone.

Context has compacted 1 times while executing this group.

### Group 21EC - Measure live peer reconstruction

{{HARD}}
- [X] 21EC.1 Measure actual selected host/client reconstruction and later equal native opportunities under fixed join/memory budgets,then close original21E.2.

21EC/original21E.2 closure,2026-10-20:[exact live reconstruction receipt] (historical reference: `../../../../logs/performance/ground-maps/20261019-native-01/network-performance-verification.json`),SHAe741123dc468cb20b0d101cd5855b0de2f918a55df29712093c1e4f99318e617,2810049bytes. Sixteen actual peer pairs/32native cases/19056assertions retain96complete native peer checkpoints/192all126-owner pure pairs;all fixed join/memory budgets and8corruption gates pass(51.225s). Both builds cover ordinary128/512/largest admitted1024Debug2048optimized,four actual original ground/tube stages,and legacy431 operating thermal reconstructed normally by434peers. Conservative client-launch-to-flushed-first-work bound includes four observer saves and polling. All gameplay owners/settings match except seven source-bound native NoNetworkSync unit preferences. Six earlier long-horizon pairs are exact dated evidence;ten focused pairs retain8/16/24/32later game ticks and16tail ticks. The earlier Debug1024 long-horizon110s workflow timeout remains excluded/reaped,with actual32.743s join below45s;focused replay completes88.084s with32.954s join without raised budgets or physics changes. Debug68/optimized29 guarded single-worker builds pass;all children reaped. Continue21F aggregation/attribution and selected master11.6 reconciliation without a checkpoint pause.

### Local Mandatory Reading - Ground budget diagnosis and closure

_Applies to21F;supplements Global/Economy and performance Area and21C-21E receipts._

Compare the exact declared budgets with actual repeated samples/peaks and retain exceeded rows. Attribute any excess to its feature owner before one bounded optimization and affected regression/retest. Do not tune away correctness,raise a failed budget after the result or complete original orbital rows. Close master11.6 only when every selected ground memory/tick/route/command/file/join requirement has measured passing evidence or an explicitly human-approved disposition.

### Group 21F - Diagnose measured ground budgets and close selected performance

{{HARD}}
- [X] 21F.1 Attribute all selected measured excesses,fix bounded work in its actual owner where required,and rerun affected correctness/workload budgets.
- [X] 21F.2 Seal selected ground performance results and reconcile Groups21-23/master11.6 only after their complete selected scope passes;retain unrelated orbital tasks.

21F selected performance closure,2026-10-20:[immutable aggregate] (historical reference: `../../../../logs/performance/ground-maps/20261019-native-01/selected-performance-verification.json`),SHA3023a8aced290e07439f2e36150782a3cb8f3f3988888b151c1e5ad554186a97,1500984bytes. Nine exact closed receipts retain252unique successful invocations/282native cases/4241556assertions,57component corruption checks and4aggregate checks. All1222 independently recomputed budget comparisons pass;2265immutable physical/current-production artifacts are stream-hashed,while75historical test/binary identities retain their dated meaning. Actual peak process commit471212032bytes,largest peer472944640bytes;worst conservative join32.9543073s/45s. Default/maximum heavy planner peaks867.5969msDebug/48.4131msoptimized remain below1080/108ms. Maximum combined tube work262.4678msDebug/20.0982msoptimized passes its actual measured-baseline budget. No production optimization is required by any admitted row.

All failures remain explicit: six110s larger-map refusals;the first startup observer's native re-pause;the tube284.7611ms observer-inclusive timing corrected by separately retaining checks_ns;the Debug1024 long later-frame network110s timeout with32.743s completed join,followed by unchanged-budget focused success;and the extension-caused kernel-pool build guard stop resolved by the user. None becomes a completed native case or a raised budget. Map4096 engine support is distinct from largest completed workflow. OS lifetime generation/load peaks and native packed/geology/index/job/utility payload/count observations remain separate;all126serialized-owner byte sizes on eight compact original final worlds provide transport attribution,not entity/packet allocator heap accounting. Full22.1 and mixed orbital rows stay open. All native/build children are reaped;continue master closure without a checkpoint pause.

### Group 22 - Attribute performance cost before optimizing

Goal finalization exception GP-PF: full22.1-22.3 peak ownership attribution, representative-map active-job/path/recipe profiling and budget-exceedance optimization are skipped for this goal under the documented multi-group gap.21B actual sparse utility measurements remain valid limited evidence; they do not complete these rows.

{{HARD}}
- [ ] 22.1 Measure packed terrain, sparse geology/jobs, entity indices, packet inventories and utility graph memory separately, including peak generation/load allocations.
- [ ] 22.2 Measure active-job/entity updates, pathfinding/approach search, Surface handling and recipe allocation separately from rendering. A hidden layer must not skip simulation work to make a benchmark pass.
- [ ] 22.3 Capture a profile for each exceeded budget and identify the owning hot path; fix one bounded-work issue in its feature owner, then retest the affected workload and regression fixture.
- [X] 22.1G Verify selected OS lifetime generation/load/process peaks separately from native packed-map/geology/index/job/utility payloads and entity/packet counts;retain compact complete serialized-owner attribution and explicitly unassigned allocator overhead.
- [X] 22.2G Measure active ground jobs,short/default/maximum routes,actual controls,Surface handling,utility allocation and physical capsule work separately from raster;complete native final worlds match across all four view roles.
- [X] 22.3G Reconcile every selected exceeded observation with its actual owner or observer/deadline cause and retained affected retest;no admitted selected production budget excess remains and no budget is increased.

Selected22G closure,2026-10-20:exact21F receipt above supplies the measured evidence. The original full allocator-memory attribution22.1 is not implied by process peaks or serialized bytes;the historical GP-PF absence of all representative ground profiling is superseded only for this selected scope.

### Group 23 - Economy and performance

Goal finalization exception GP-PF: full dense excavation/orbital workload23.1 and related23.2 hot-path acceptance need the absent representative stress/instrumentation groups. Existing finite account/connection/share limits and measured sparse utility retirement are retained; no unmeasured dense-map performance pass is claimed.

{{MEDIUM}}
- [ ] 23.1 Profile dense excavation projects and orbital fleets.
- [ ] 23.2 Resolve unbounded tile scans, all-pairs rendezvous work, inventory growth and command stalls; retest only affected budgets after fixes.
- [X] 23.1G Profile0/32/128actual partial excavation owners through2048prepared maps and selected utility/tube bounds through4096accounts/capsules,retaining actual original active worlds independently.
- [X] 23.2G Verify selected repeated job/lookup/route/command/allocation/physical tube work against frozen budgets with conserved packet/cash/energy ownership;retain bounded-limit failures and unmeasured orbital workloads explicitly.

Selected23G closure,2026-10-20:exact21F receipt closes measured ground/related utility performance. Neither finite measurements nor this scoped closure prove every possible asymptotic workload;original orbital/fleet/whole-profile tasks remain unchecked.

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

**Raster source review.** Corrections requested by this area's art review go through Plan13's [framed ImageGen prompt and extraction checks](../../../../../../../../../OpenTTD3/docs/zz-reach2/upgrades/2026-10/r2mlvs13-graphics-and-asset-pipeline.md#framed-imagegen-sheets-and-safe-extraction). For framed multi-view originals, require the retained prompt/references, measured non-overlapping crops, complete subject clearance and exclusion of separator frames from native sprites. Check hardware identity and required direction/state coverage separately. The framed trial passed extraction but failed model identity; neither its source preview nor another clean sheet can replace packed/decoded hashes and actual loaded runtime captures in Groups27-29.

**Entry gate.** The following public contracts must exist before dependent implementation in this area: `src/tests/multilayer/acceptance/native_ground.py`: `run_scenario`, `capture_ledger`, `compare_state_digest`, `validate_art_manifest`.
Verify each public file/export/schema field with `rg` in the current workspace and inspect its command/field ownership. For a same-plan contract, compile its type/signature fixture or validate its artifact after the preceding producer groups. For cross-plan behavior, also run the producer's focused executable boundary test; declarations alone cannot authorize dependent mutations or runtime claims.
If a required export is absent, stop the dependent implementation, mutation and runtime claims in Groups 27-33. Read-only characterization, source/range verification, test-case design and asset-template work that does not assume the missing behavior may continue.
Do not bypass this gate by copying private pool/tile state, inventing local IDs or cargo registries, using a historical plan as a delivered implementation, opening user-wide content stores, substituting a second utility/accounting authority, or adding an unreviewed fallback path. Finish the named producer or revise the shared contract and its direct consumers first.

| File name | Line Ranges | Description |
| --- | --- | --- |
| [src/table/sprites.h](../../../../../../../../src/tests/fixtures/cleanup/source-epochs/openttd3-75ea75718f/fea054eeef617b1a-sprites.h) | 380-409 | Complete Map Layer sprite constants and `SPR_NEWGRFS_BASE` bound the published block; preserve 0..65 and resolve any accepted live extension before allocating new IDs. |
| [src/map_layer_gui.h](../../../../../../../../src/tests/fixtures/cleanup/source-epochs/openttd3-75ea75718f/2c249444d059d449-map_layer_gui.h) | 67-73 | `GetVehicleDisplayLayer` is the public visible/pickable-layer contract; keep ordinary aircraft/ships/disasters on Surface and place orbital units explicitly. |
| `src/tests/multilayer/acceptance/native_ground.py` (planned public contract; link after implementation) | Live definitions; no fixed range | [r2mlvs16-integrated-acceptance-and-balance.md](r2mlvs16-integrated-acceptance-and-balance.md) Groups 1-5, ending with Group 5's public-contract task, publish `run_scenario`, `capture_ledger`, `compare_state_digest`, `validate_art_manifest`. These exports/schema fields are authoritative for this area. Same-plan consumers follow those earlier groups; cross-plan consumers verify the producer's required implementation/tests as well. Proposed names alone do not prove delivery. |

**ASSUMPTION: this was already built previously. If not, re-asses at runtime**. This applies only to the producer contracts named in this area's gate, after their listed producer groups have run. Verify their actual exports and focused tests before using the sketches below; proposed types and helper names in sketches are not evidence of delivered code.

**Art evidence gate. ASSUMPTION: this was already built previously. If not, re-asses at runtime**. The complete r2mlvs13 manifest, packed assets and required feature state transitions exist before S10 capture. Verify their hashes/tests; missing families remain explicitly pending and prevent final closure, while already delivered family captures can be recorded independently.

Context has compacted 1 times while executing this group.

### Group 27 - Build the runtime art coverage matrix

{{MEDIUM}}
- [ ] 27.1 Expand r2mlvs13 manifest entries into required direction/state/zoom/blitter captures and map each to a real S10 runtime scene plus asset hash.
- [ ] 27.2 Capture excavation/portals/TBM/exchange and ten mines/conventional fleet first, then five tube models and each geothermal era. Use actual animation/load/service states.
- [ ] 27.3 Capture launch/probe/asteroids/descent, module construction/processing and mirrors, including extra viewports and separate Surface/orbital overlays.
- [X] 27.1G Map the selected target manifest to retained native runtime scenes,source/distribution hashes and explicit renderer-query versus driven-frame coverage;current selected publication69families/10557poses.
- [X] 27.2G Verify delivered geology/portal/TBM/wagon/exchange/receiver/service/control and selected tube/thermal consumers through the published native gallery,retaining current versus dated physical observations explicitly.

Ground closure, 2026-10-08: [technical gallery](../../../../../../../../../OpenTTD3/docs/zz-reach2/upgrades/2026-10/portal-tbm/r2mlvs3mp-t-gallery-acceptance.md) and [fresh hash/corruption verification] (historical reference: `../../../../logs/r2mlvs3mp-z-b-final-art-verification.log`) pass forty families/2,311 sprites/157 actual native frames. Original later mine/fleet/tube/geothermal rows remain pending under master GP-TG; orbital rows are outside this queue. No template or renderer direction query is presented as physical motion.

### Group 28 - Review occlusion and distributed art coverage

Current selected technical return,2026-10-20:[published native gallery verification] (historical reference: `../../../../logs/graphics/selected-bootstrap/20261019-native-01/publication-verification.json`),SHAea157f9f4cfcbd1f1efb692c63e849a4a8694adeb2695a012f996d4e6e0db7dd,retains69families/10557poses/65active consumers,27current plus42dated producer families,66unique native cases/139718assertions and2209PNG/2212offline links. Twenty whole1920x2968canvases match original distributed source/ranges. The preceding40-family/157-frame snapshot is historical,as is its absent tube/thermal scope. Full direction/state/zoom/occlusion/sound user feedback remains unrecorded;original28.1-28.3/29whole acceptance and master11.7 stay unchecked.

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
- [X] 30.2G Update the delivered portal/TBM/area/exchange/Deep player path and concrete supported recovery in [the player guide](d2um-underground-mines.md); label fixture-only acceptance and absent later consumers.

Ground scope excludes original tube/geothermal guidance under GP-TG and unrelated orbital guidance; no instruction implies those consumers exist. Proposal history remains retained in its owning plans.

### Group 31 - Art, documentation and completion

{{SIMPLE}}
- [ ] 31.1 Publish an evidence matrix mapping every master requirement/invariant to exact scenario/test/log/artifact. List all unresolved limitations and unsupported content profiles honestly.
- [X] 31.1G Publish this master's target requirement/invariant/evidence/exception matrix in [the final audit](portal-tbm/r2mlvs3mp-z-b-final-closure-audit.md), including exact remaining target tasks and unsupported runtime claims.

### Group 32 - Art, documentation and completion

{{MEDIUM}}
- [ ] 32.1 Close each child only when its completion conditions and required checks are satisfied, then reconcile the master index. An explicitly authorized skip remains recorded as unverified and does not imply runtime, network or art acceptance.
- [X] 32.1G Apply each target's own Completion Conditions: supporting Plan2/5 contracts and now fully verified Plan3/4 runtime tasks can close;master11.7/12.6,M1 and unrelated milestones remain incomplete until their own requirements pass.

### Group 33 - Art, documentation and completion

{{SIMPLE}}
- [ ] 33.1 Record the implemented public exports, focused test commands/results and remaining runtime/art/network gaps in this plan; retain original supplied saves and repository-local evidence.
- [ ] 33.2 Update affected architecture/handoff documentation to describe delivered behavior and reconcile the parent index only when this plan's Completion Conditions are met. Keep every unfinished task unchecked.
- [X] 33.1G Record delivered ground exports, focused results, protected source inputs and explicit limitations in the final audit and player guide.
- [X] 33.2G Reconcile delivered ground/electrical architecture and truthful parent progress; retain original full-profile closure tasks unchecked.

## Consult on Demand

| Trigger | File name | Start at | Expand only when | Consultation purpose |
| --- | --- | --- | --- | --- |
| A combined run reports no desync but gameplay ledgers differ | [../../../../src/network/network_server.cpp](../../../../../../../../src/tests/fixtures/cleanup/source-epochs/openttd3-75ea75718f/6a45d374c71c770f-network_server.cpp) | `ServerNetworkGameSocketHandler::SendSync` | Command order and RNG state agree while cargo/geology/orbital digests disagree | Confirm the RNG-only sync scope and compare explicit feature ledgers; no-desync output cannot replace full state equality. |
| A required public export is absent and its owner/location is disputed | [r2mlvs-multilayer-vehicles-and-structures.md](r2mlvs-multilayer-vehicles-and-structures.md) | `Requirements and ownership` and `Plan Index` | The named producer confirms an intentional contract/ownership change | Find the responsible producer and reconcile its public handoff with direct consumers; the hub cannot substitute for missing implementation. |
| Current behavior contradicts a historical subsystem description | [../../architecture/map-layers/archi-map-layers.md](../../architecture/map-layers/archi-map-layers.md) | `Invariants` and the relevant named source in `Code map` | The live symbol and executable test do not explain the difference | Locate layer addressing or lifecycle rationale. Live code/tests govern delivered behavior; this architecture page does not prove future interfaces exist. |
| A configure/build invocation no longer resolves the recorded Windows toolchain or test target | [R2MLM build commands](r2mlm-multi-layered-map.md#build-and-test-commands) | `Build and Test Commands` | The current CMake target declarations and local configuration do not explain the failure | Recover the repository build invocation, then verify it against the live toolchain; historical success/counts are not current test evidence. |

## Planning Provenance

This is not an execution reading queue. Do not preload it after context compaction. It is non-transitive; ranges record planning use, not runtime authority.

| File name | Sections or Line Ranges used while planning | Planning contribution |
| --- | --- | --- |
| [docs/zz-reach2/architecture/initial-inventory.md](../../../../../../../zz-reach2/architecture/initial-inventory.md) | 185-220 | `Command as the only world mutation`, `Pool-indexed game objects`, `Tile map as a packed array` and `File-suffix modules` oriented command/pool/header ownership and source registration. This historical inventory is a location survey, not authority for current layer behavior. |
| [docs/zz-reach2/architecture/@archi-ttd-level0.md](../../../../../../../../../OpenTTD3/docs/zz-reach2/architecture/@archi-ttd-level0.md) | 24-63 | `How the program fits together` and `Source and runtime boundaries` located shared simulation, data/presentation, CMake targets and subsystem handoffs used to choose the live reading sources. |
| [docs/zz-reach2/upgrades/2026-10/r2mlvs-multilayer-vehicles-and-structures.md](r2mlvs-multilayer-vehicles-and-structures.md) | 119-140 | `Requirements and ownership` assigned this plan's feature responsibilities; it shaped the decomposition without making every sibling plan a mandatory reading dependency. |
| [src/network/network_server.cpp](../../../../../../../../src/tests/fixtures/cleanup/source-epochs/openttd3-75ea75718f/6a45d374c71c770f-network_server.cpp) | 670-687 | The inspected live boundary helped separate characterization, public-contract production, implementation and acceptance batches. It materially constrained measured combined evidence without treating no-desync or art sheets as full acceptance; this entry records planning influence rather than an additional reading obligation. |
| [User planning-format instructions] (historical reference: `<C:/Users/Axonn/.codex/attachments/cd59b20a-3a96-4d54-b4bd-1e8f37639e7d/Pasted text.txt>`) | `Global Mandatory Reading`, `Area Mandatory Reading`, `Consult on Demand`, `Planning Provenance`, `Final validation` | Supplies the exact protocol, chronological difficulty batches of at most eight tasks, reading gates and validation format. |
| [src/framerate_type.h](../../../../../../../../src/tests/fixtures/cleanup/source-epochs/openttd3-75ea75718f/d94650a37e44178f-framerate_type.h) | 100-119 | Deeper planning pass, 2026-10-04: Measurement reset/accumulation semantics shaped repeatable workload attribution and bounded optimization tasks. |

## Completion Conditions

All requested gameplay, five original tube vehicles, geothermal progression and the planned M4 expansion are demonstrably connected, supplied with final graphics and supported by save/network evidence. Calendar gates, TBM turn geometry and the two launch-center areas have boundary tests; no visual counter substitutes for real material transfer or heat allocation. Performance and balance claims have measurements; remaining limitations are explicit rather than hidden behind checked boxes.

Retained recurring-depot producer evidence, 2026-10-06: [Plan 5 Group 13B](r2mlvs5-spoil-wagons-and-haulage.md#group-13b---drive-recurring-underground-bay-exchanges) now supplies two actual depot exchanges and mandatory Surface disposal/empty return, with both paid spoil ledgers and native expense accounting. Two full native replays match all 63 physical/cargo/owner/cost transitions ([ei] (historical reference: `../../../../logs/r2mlvs5-ei-complete-depot-trace-checks.log`), 378,756 assertions / one case). This component evidence does not complete Plan 16's broader network, balancing or performance gates.
