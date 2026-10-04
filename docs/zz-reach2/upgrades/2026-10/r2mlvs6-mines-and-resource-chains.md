# R2MLVS6 - Mines and Resource Chains

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
| [docs/zz-reach2/upgrades/2026-10/r2mlvs-multilayer-vehicles-and-structures.md](r2mlvs-multilayer-vehicles-and-structures.md) | 242-246 | `AD-7` supplies the canonical gameplay/ownership decision for every group of this plan. It constrains mine/deposit claims, finite output and useful layer-local acceptance; a consumer must not redefine it locally. |
| [src/map_layer.h](../../../../src/map_layer.h) | 78-85 | `MultiLayerMap` ownership preamble: `Map` owns packed storage/geometry and this controller owns layer semantics. Every group uses world tiles/explicit layer helpers; camera selection and ad hoc plane offsets cannot define gameplay. |
| [src/industry_cmd.h](../../../../src/industry_cmd.h) | 18-28 | Industry construction/funding command declarations and `DEF_CMD_TRAIT` entries are the shared UI/script mutation boundary; availability must agree on every entry path. |

Status: proposed. Parent: [R2MLVS master](r2mlvs-multilayer-vehicles-and-structures.md). Depends on r2mlvs2/5 and the r2mlvs1 content contract.

## Outcome and Boundaries

Make all ten proposed mine resources productive and transportable to useful Surface consumers. Deep mines obey the 1980 calendar unlock; r2mlvs17 adds geothermal power and later direct-heat receivers without replacing the five Deep mineral types or removing their Surface receivers.

## Public Handoff Contract

Proposed public owner: `src/multilayer_industry.h`. Publish `LayerIndustryCapability`, `ValidateMineDepositClaim`, `ProduceLayerMineCargo`, `PrepareLayerRecipeBatch`, `CommitLayerRecipeBatch`. These symbols are producer obligations, not existing code. Keep one authority for the fields/commands they expose; downstream plans consume the public contract rather than private pools or copied tables.

## Implementation Tasks

Deepening pass, 2026-10-04: implementation groups use only SIMPLE, MEDIUM or HARD difficulty, with at most eight pending tasks each. Sketches describe proposed implementation contracts; verify the named earlier producers at each assumption gate. No gameplay or asset implementation is marked complete by this planning pass.

Groups are chronological and contain consecutive tasks of one difficulty. Each difficulty batch has at most eight tasks; area reading changes only when the implementation boundary or source of truth changes.

### Area Mandatory Reading - Layer-aware industry lifecycle

_Applies to Groups 1-6._

**Entry gate.** The following public contracts must exist before dependent implementation in this area: `src/multilayer_content.h`: `MultiLayerGameplayProfile`, `LayerVehicleCapability`, `GetMultiLayerContentProfile`, `CheckDeepLayerAvailability`, `UtilityPowerAccount`; `src/excavation.h`: `ExcavationState`, `GeologyDeposit`, `ExcavationJob`, `CheckExcavatedFootprint`, `CommitExcavationCut`, `GeothermalVentField`; `src/tbm_exchange.h`: `TBMExchangeAssignment`, `ValidateTBMWagonExchange`, `ExecuteTBMWagonExchange`.
Verify each public file/export/schema field with `rg` in the current workspace and inspect its command/field ownership. For a same-plan contract, compile its type/signature fixture or validate its artifact after the preceding producer groups. For cross-plan behavior, also run the producer's focused executable boundary test; declarations alone cannot authorize dependent mutations or runtime claims.
If a required export is absent, stop the dependent implementation, mutation and runtime claims in Groups 1-6. Read-only characterization, source/range verification, test-case design and asset-template work that does not assume the missing behavior may continue.
Do not bypass this gate by copying private pool/tile state, inventing local IDs or cargo registries, using a historical plan as a delivered implementation, opening user-wide content stores, substituting a second utility/accounting authority, or adding an unreviewed fallback path. Finish the named producer or revise the shared contract and its direct consumers first.

| File name | Line Ranges | Description |
| --- | --- | --- |
| [src/industrytype.h](../../../../src/industrytype.h) | 109-152 | `IndustrySpec` owns layouts, climate/availability, recipes and callbacks; layer capabilities must be explicit and not inferred from fallback graphics. |
| [src/newgrf_industries.h](../../../../src/newgrf_industries.h) | 70-89 | `IndustryAvailabilityCallType`, `IndustryProductionCallback`, `MapNewGRFIndustryType` and callback declarations define availability/GRFID mapping; no translated-name or cargo-label-only adapter. |
| [src/timer/timer_game_calendar.h](../../../../src/timer/timer_game_calendar.h) | 15-39 | `TimerGameCalendar` technology-clock contract and date conversions govern 1980/model/factory introductions; economic/realtime scheduling cannot unlock a technology. |
| `src/multilayer_content.h` (planned public contract; link after implementation) | Live definitions; no fixed range | [r2mlvs1-gameplay-contracts-and-content.md](r2mlvs1-gameplay-contracts-and-content.md) Groups 1-6, ending with Group 6's public-contract task, publish `MultiLayerGameplayProfile`, `LayerVehicleCapability`, `GetMultiLayerContentProfile`, `CheckDeepLayerAvailability`, `UtilityPowerAccount`. These exports/schema fields are authoritative for this area. Same-plan consumers follow those earlier groups; cross-plan consumers verify the producer's required implementation/tests as well. Proposed names alone do not prove delivery. |
| `src/excavation.h` (planned public contract; link after implementation) | Live definitions; no fixed range | [r2mlvs2-geology-and-excavation.md](r2mlvs2-geology-and-excavation.md) Groups 1-7, ending with Group 7's public-contract task, publish `ExcavationState`, `GeologyDeposit`, `ExcavationJob`, `CheckExcavatedFootprint`, `CommitExcavationCut`, `GeothermalVentField`. These exports/schema fields are authoritative for this area. Same-plan consumers follow those earlier groups; cross-plan consumers verify the producer's required implementation/tests as well. Proposed names alone do not prove delivery. |
| `src/tbm_exchange.h` (planned public contract; link after implementation) | Live definitions; no fixed range | [r2mlvs5-spoil-wagons-and-haulage.md](r2mlvs5-spoil-wagons-and-haulage.md) Groups 1-5, ending with Group 5's public-contract task, publish `TBMExchangeAssignment`, `ValidateTBMWagonExchange`, `ExecuteTBMWagonExchange`. These exports/schema fields are authoritative for this area. Same-plan consumers follow those earlier groups; cross-plan consumers verify the producer's required implementation/tests as well. Proposed names alone do not prove delivery. |
| [src/industry_cmd.cpp](../../../../src/industry_cmd.cpp) | 1995-2063 | Complete `CreateNewIndustryHelper` is shared by direct generation and commands; admission belongs here before construction writes. |
| [src/industry_cmd.cpp](../../../../src/industry_cmd.cpp) | 1762-1993 | Complete `DoCreateNewIndustry` mutates footprint and currently ignores clear results; mine claim/allocation preflight must finish before this boundary. |
| [src/industry_cmd.cpp](../../../../src/industry_cmd.cpp) | 839-960 | Complete `TileLoop_Industry` contains Surface-only water/effects/transfer assumptions; dispatch supported behavior by capability, not fallback graphics. |
| [src/industry_cmd.cpp](../../../../src/industry_cmd.cpp) | 150-215 | Complete `Industry` destructor removes tiles/references; mine closure must preserve cavity, geological reserve and accountable outputs. |

**ASSUMPTION: this was already built previously. If not, re-asses at runtime**. This applies only to the producer contracts named in this area's gate, after their listed producer groups have run. Verify their actual exports and focused tests before using the sketches below; proposed types and helper names in sketches are not evidence of delivered code.

**Lifecycle decomposition.** Placement policy, geology claims and production authority are independent checks. Introduce a capability whitelist for supported first-party definitions; ordinary NewGRF industries stay Surface unless the selected compatibility profile explicitly supports them. Preserve legacy policies while auditing each live lifecycle entry point.

### Group 1 - Characterize industry layer assumptions by lifecycle

{{MEDIUM}}
- [ ] 1.1 Trace one funded industry from command validation through tile placement and stable IndustryID creation; list Surface-only checks, footprint iteration and callback inputs needing layer-aware behavior.
- [ ] 1.2 Trace ticking/TileLoop, effects, climate/water and production separately. Classify each behavior as supported underground, disabled by capability or still Surface-only; removing one assertion is not the change.
- [ ] 1.3 Trace station association, town proximity, closure, news/lists and script access. Record the authoritative layer coordinate at each boundary and a failing fixture for any Surface-only assumption.

### Group 2 - Layer-aware industry lifecycle

{{SIMPLE}}
- [ ] 2.1 Record each change; removing the Surface assertion alone is insufficient.

### Group 3 - Layer-aware industry lifecycle

{{MEDIUM}}
- [ ] 3.1 Add a layer/capability whitelist for new mine and processor definitions. Ordinary NewGRF industries remain Surface unless explicitly supported; fallback/substitute graphics do not grant placement permission.
- [ ] 3.2 Implement mine funding only on fully excavated footprints touching a suitable unclaimed deposit.

### Group 4 - Build and bind one mine atomically

{{HARD}}
- [ ] 4.1 Prepare a mine placement request with complete machinery/loading approach footprint, selected deposit ID and allowed layer/type/date. Verify r2mlvs2 excavation and reserve/claim queries before any tile clearing.
- [ ] 4.2 Put admission in the shared `CreateNewIndustryHelper` path as well as command UI: `CreateNewIndustry` bypasses `CmdBuildIndustry`. Replace default Surface-random prospecting for geological mines with explicit deposit selection; do not let generation bypass date/claim checks.
- [ ] 4.3 Validate same-company claim rules, seam contact, unclaimed available reserve, access and complete construction cost. Reserve industry/claim capacity before tile mutation.
- [ ] 4.4 Commit industry identity, map footprint and deposit claim together; on failure retain both geology and tiles. `DoCreateNewIndustry` currently ignores execution-time clear failures, so complete preflight before its first write and handle failure explicitly. Test one unexcavated corner, competing claim, depleted deposit and insufficient funds.
- [ ] 4.5 Define rebuilding as a new industry attached to the original remaining reserve; demolition releases the claim without recreating geological material.

### Group 5 - Layer-aware industry lifecycle

{{MEDIUM}}
- [ ] 5.1 Bind mine and deposit with stable IDs.
- [ ] 5.2 Define depletion, closure, abandonment and rebuilding so rebuilding cannot reset the original reserve or claim the same seam twice.
- [ ] 5.3 Keep station acceptance/catchment and cargo generation within the industry's own layer. Provide explicit layer names and navigation in lists, news and vehicle orders.
- [ ] 5.4 Apply the shared Deep calendar availability rule to funding, automatic construction, activation, production and script entry points. Scenario/editor authoring must not silently enable operating Deep industries before 1980 in normal new-profile play.

### Group 6 - Publish layer-industry and recipe boundaries

{{HARD}}
- [ ] 6.1 Publish `src/multilayer_industry.h` with `LayerIndustryCapability`, `ValidateMineDepositClaim` and `ProduceLayerMineCargo`, including ownership, invalid IDs and command/estimate rules.
- [ ] 6.2 Declare proposed `PrepareLayerRecipeBatch` and `CommitLayerRecipeBatch` for shared bounded all-input processing. Accept an explicit inventory owner/view and commit participant rather than requiring `Industry*`, so a later orbital `SpaceStructure` can reuse them. Use r2mlvs1 recipe IDs/units; separate reserve debit, delivered stock and future utility grants.
- [ ] 6.3 Compile the public header fixture and register sources. The following production groups implement the callable recipe/extraction behavior; declarations alone do not authorize downstream mutation.

### Area Mandatory Reading - Resource production and consumers

_Applies to Groups 7-14._

**Entry gate.** The following public contracts must exist before dependent implementation in this area: `src/multilayer_industry.h`: `LayerIndustryCapability`, `ValidateMineDepositClaim`, `ProduceLayerMineCargo`, `PrepareLayerRecipeBatch`, `CommitLayerRecipeBatch`.
Verify each public file/export/schema field with `rg` in the current workspace and inspect its command/field ownership. For a same-plan contract, compile its type/signature fixture or validate its artifact after the preceding producer groups. For cross-plan behavior, also run the producer's focused executable boundary test; declarations alone cannot authorize dependent mutations or runtime claims.
If a required export is absent, stop the dependent implementation, mutation and runtime claims in Groups 7-14. Read-only characterization, source/range verification, test-case design and asset-template work that does not assume the missing behavior may continue.
Do not bypass this gate by copying private pool/tile state, inventing local IDs or cargo registries, using a historical plan as a delivered implementation, opening user-wide content stores, substituting a second utility/accounting authority, or adding an unreviewed fallback path. Finish the named producer or revise the shared contract and its direct consumers first.

**Scoped integration gates.** The following producer dependencies apply only to the named implementation/test families. Verify each required public export and its focused producer test before those integrations; absent late producers do not block already independent features.

- `src/transport_tube.h` is required for the tube-terminal cargo/refit integration in Group 14 (only the named integration/subscenario). If absent, stop only that integration/subscenario and its passing claim. Other independent code, art, persistence and tests in this area may continue.

Do not emulate an absent producer with copied private state, fake inventories/energy, local IDs or a second registry/grid. A final combined closure/gallery/round-trip still waits for every required family; an independent passing case is not a pass for the missing integration.

| File name | Line Ranges | Description |
| --- | --- | --- |
| [src/industry.h](../../../../src/industry.h) | 75-106 | `ProducedCargo`, `AcceptedCargo`, `ProducedCargoes`, `AcceptedCargoes` expose bounded inventory and economy-time history; conserve materials and avoid narrow-counter overflow. |
| [src/industry_type.h](../../../../src/industry_type.h) | 98-103 | `INDUSTRY_NUM_INPUTS` and `INDUSTRY_NUM_OUTPUTS` provide current 16-slot bounds; do not assume historical 3-input/2-output limits. |
| [src/cargotype.h](../../../../src/cargotype.h) | 211-223 | `SetupCargoForClimate`, `BuildCargoLabelMap`, `GetCargoTypeByLabel` separate stable labels from runtime slots; consumers must resolve the selected profile rather than hard-code cargo indices. |
| [src/industry_cmd.cpp](../../../../src/industry_cmd.cpp) | 1152-1240 | `ProduceIndustryGoodsHelper` and `ProduceIndustryGoods` expose complete rate/callback production paths; augment output once before authoritative monthly accounting. |
| `src/multilayer_industry.h` (planned public contract; link after implementation) | Live definitions; no fixed range | [r2mlvs6-mines-and-resource-chains.md](r2mlvs6-mines-and-resource-chains.md) Groups 1-6, ending with Group 6's public-contract task, publish `LayerIndustryCapability`, `ValidateMineDepositClaim`, `ProduceLayerMineCargo`, `PrepareLayerRecipeBatch`, `CommitLayerRecipeBatch`. These exports/schema fields are authoritative for this area. Same-plan consumers follow those earlier groups; cross-plan consumers verify the producer's required implementation/tests as well. Proposed names alone do not prove delivery. |
| `src/transport_tube.h` (planned public contract; link after implementation) | Live definitions; no fixed range | [r2mlvs16-underground-transport-tubes.md](r2mlvs16-underground-transport-tubes.md) Groups 1-8, ending with Group 8's public-contract task, publish `TubeVehicleCapability`, `TubeTerminal`, `CheckTubeConstruction`, `ComputeTubePowerDemand`. These exports/schema fields are authoritative for this area. Same-plan consumers follow those earlier groups; cross-plan consumers verify the producer's required implementation/tests as well. Proposed names alone do not prove delivery. Read this producer for the tube-terminal cargo/refit integration in Group 14 (only the named integration/subscenario); missing it blocks only that integration, not independent work. |
| [src/economy.cpp](../../../../src/economy.cpp) | 1134-1167 | Complete `TriggerIndustryProduction` distinguishes callbacks from independent-input conversion; the shared recipe executor must preserve all-input atomicity. |
| [src/economy.cpp](../../../../src/economy.cpp) | 1022-1073 | Complete `DeliverGoodsToIndustry` exposes bounded intake; accept exact quantities before packet removal for finite processors. |
| [src/cargoaction.cpp](../../../../src/cargoaction.cpp) | 107-119 | Complete `CargoDelivery::operator()` removes requested cargo; integrate accepted-count admission rather than discarding receiver overflow. |
| [src/industry_cmd.cpp](../../../../src/industry_cmd.cpp) | 525-554 | Complete `TransportIndustryGoods` removes stock before delivery and can lose finite output; new-profile dispatch must conserve units. |
| [src/station_cmd.cpp](../../../../src/station_cmd.cpp) | 4412-4458 | Complete `UpdateStationWaiting` retains fractional credits; retries need a single saved remainder owner. |
| [src/linkgraph/refresh.cpp](../../../../src/linkgraph/refresh.cpp) | 176-236 | Complete `LinkRefresher::RefreshStats` derives service edges from vehicles/orders; preserve valid physical cross-layer routes without catchment shortcuts. |
| `src/utility_power.h` (planned public contract; link after implementation) | Live definitions; no fixed range | [r2mlvs12](r2mlvs12-solar-farms-mirrors-and-eti.md) `Useful Power Handoff`, produced by its native-ledger, atomic-mutation and consumer-effect groups, supplies `PublishUtilityGeneration`, `AllocateUtilityPower`, `GetUtilityAllocation`, `UtilityGrantID`, `UtilityMutationKind`, `PrepareUtilityMutation`, `CommitUtilityMutation`. Verify its allocation, consume/release/layer-transfer, effect and save/retry fixtures before dependent power mutations; no private account writes or duplicate grid. |

**ASSUMPTION: this was already built previously. If not, re-asses at runtime**. This applies only to the producer contracts named in this area's gate, after their listed producer groups have run. Verify their actual exports and focused tests before using the sketches below; proposed types and helper names in sketches are not evidence of delivered code.

**Proposed bounded production sketch.** The profile producer has already defined recipes; the previous area has declared the public batch API. Implement it here before any Surface processor uses it. Existing `ProduceIndustryGoodsHelper` clamps a 16-bit output buffer and the default cargo-arrival path consumes each input independently, so those defaults cannot enforce finite extraction or an all-input recipe.

```cpp
// Design sketch; quantities use checked wide intermediates.
const auto amount = min(work_allowance, remaining_reserve, output_room);
// After revalidation, one commit debits reserve and credits output by amount.
// For processing, replace reserve with the minimum batch count over ALL inputs
// and ALL output slots; no input is consumed on a zero-batch result.
```

The batch utility participant is an interface for r2mlvs17, not a second heat allocator. **ASSUMPTION: this was already built previously. If not, re-asses at runtime**. Actual tube-terminal integration additionally requires r2mlvs16 `TubeTerminal` and its cargo/refit behavior; verify those exports/tests only for that integration task, leave it pending if absent, and continue conventional mine/processor work. Do not create substitute tube inventory.

**Useful-power integration gate. ASSUMPTION: this was already built previously. If not, re-asses at runtime**. Nuclear/Coal useful-power acceptance needs r2mlvs12's early `Useful Power Handoff`, including tested native grid publication/allocation. r2mlvs1's `UtilityPowerAccount` declaration alone is insufficient. Verify the public grid exports and focused consumer test; if absent, stop only power mutations/acceptance, continue material processors, and do not create a second local grid. This early handoff must not depend on rockets or mirrors.

### Group 7 - Implement finite mine extraction and bounded recipe batches

{{HARD}}
- [ ] 7.1 Implement mine extraction amount as the minimum of allowed work, remaining deposit reserve and free output space. Debit reserve only when the same quantity reaches accountable output; no clamped-away production.
- [ ] 7.2 Implement `PrepareLayerRecipeBatch` from the profile all-input recipe record: compute the minimum feasible batch count over every input, every output slot and work budget with checked integers.
- [ ] 7.3 Implement `CommitLayerRecipeBatch` with revalidation and one input/output/work commit. Use the chosen content callback/native path; default independent-input multipliers cannot substitute for the shared batch executor.
- [ ] 7.4 Test missing secondary input, full one-of-many output, partial reserve and near-counter-limit values. Keep supplied service modifiers optional at bootstrap and preserve buffers while paused.

### Group 8 - Dispatch finite output without loss or fractional double credit

{{HARD}}
- [ ] 8.1 Separate extraction from station dispatch. Update production history once when reserve becomes output; dispatch only updates transported quantities. Apply any rate/recession modifier before extraction or record explicit loss, never silently discard finite stock.
- [ ] 8.2 Implement a bounded integer dispatch ticket over eligible same-layer stations, output units and packet/storage capacity. Commit the station supply and subtract exactly those units from mine output together; no station or packet room means stock remains.
- [ ] 8.3 Account for `UpdateStationWaiting` fractional credits explicitly: use a producer-owned saved dispatch remainder or a dedicated exact-integer path, not repeated calls on unchanged output that each add station fractions.
- [ ] 8.4 Test no station, zero rating, packet-pool exhaustion, fractional rate, partial station capacity and save/retry. Compare reserve + mine stock + station/vehicle stock + recorded consumption; no unit disappears or receives credit twice.

### Group 9 - Instantiate the ten mine definitions

{{MEDIUM}}
- [ ] 9.1 Register Underground Coal, Iron Ore, Copper Ore, Bauxite and Salt definitions with distinct cargo labels, geology capability and layouts; validate each uses the common extraction path.
- [ ] 9.2 Register Deep Lithium Ore, Nickel Ore, Cobalt Ore, Rare Earth concentrate and Uranium with the shared 1980 gate, deeper equipment capability and harder access costs.
- [ ] 9.3 Map all ten mines to art IDs/overlays and test their label-to-runtime-slot resolution. Geology determines reserve; a new mine instance never grants fresh reserves.

### Group 10 - Build the Surface processing chain from shared recipes

{{MEDIUM}}
- [ ] 10.1 Add metal and chemical works using the published all-input executor and content quantities; keep accepted packet inventory distinct from recipe consumption.
- [ ] 10.2 Add battery and electronics plants, then component/robotics/mirror-kit/propellant production with their frozen recipes. Check actual incoming labels and available output capacity for each.
- [ ] 10.3 Add nuclear/useful-power acceptance through the single utility authority; Uranium must have a functioning Surface consumer when Deep opens, before later geothermal manufacturing.
- [ ] 10.4 Keep minimal Earth/Rock receivers owned by r2mlvs5 and reuse their exact-count intake contract. Test a partially full receiver retains unaccepted freight instead of ordinary delivery silently deleting surplus.

### Group 11 - Reuse compatible baseline receivers explicitly

{{MEDIUM}}
- [ ] 11.1 Resolve Coal and Iron Ore by selected-profile label and inspect the active power-plant/steelworks definitions. Require matching accepted labels, output meaning and supported production path; names/graphics alone are insufficient.
- [ ] 11.2 Where those checks pass, reference the existing receiver identity rather than registering a duplicate. Otherwise register the first-party receiver specified by the profile and report the incompatibility.
- [ ] 11.3 Deliver one Coal and one Iron load through the selected path, check exact accepted quantities and resulting power/Steel, and verify all recipe/industry/cargo slots stay within the frozen budget.

### Group 12 - Integrate demand and freight accounting on explicit routes

{{HARD}}
- [ ] 12.1 Connect industry output to same-layer stations and allow ordinary CargoDist links only from actual vehicle service. Test same-XY stations on different layers receive no free cargo or demand edge.
- [ ] 12.2 Verify delivery distances use plane coordinates while route feasibility uses actual portals/elevators. Compare an equivalent horizontal route on each layer and reject an artificial stacked-Y income bonus.
- [ ] 12.3 Integrate supported train/truck refits and profile payment categories; cap processor intake before removing packets, preserving refused remainder and feeder provenance.
- [ ] 12.4 Run a Surface-only manufacture bootstrap for launch fuel, components, Robotics Parts and Solar Mirror Kits; no first orbital facility may require prior orbital production.

### Group 13 - Expose a utility participant for later direct-heat recipes

{{HARD}}
- [ ] 13.1 Extend the shared recipe proposal with an optional authoritative utility grant token/quantity and recipe selection. The future geothermal owner supplies heat; the mine reserve API is not reused as a heat source.
- [ ] 13.2 Specify prepare/commit/cancel ownership for the grant: no inputs or heat are consumed if output capacity or one transported input is absent. A refused/retried batch cannot consume the grant twice.
- [ ] 13.3 Publish contract fixtures for the future 2010 smelter, 2030 battery works and 2045 Mantle Forge using recipe records and supplied grant values. These fixtures validate the boundary, not a claim that r2mlvs17 plants already exist.
- [ ] 13.4 Document exact public batch fields/signatures for r2mlvs11 and r2mlvs17; keep their actual scheduling, heat allocation and buildings in those owners.

### Group 14 - Resource production and consumers

{{MEDIUM}}
- [ ] 14.1 Keep mineral reserve extraction, direct heat use, transported inputs and produced output separately accountable; all five Deep minerals retain an earlier useful Surface receiver.
- [ ] 14.2 Integrate tube terminal cargo/refits with r2mlvs16. Mine/factory freight may transfer to tubes on its own layer and back to conventional transport for inter-layer movement; tubes neither replace mandatory TBM spoil-wagon hauling nor open a pre-1980 Deep route.

### Area Mandatory Reading - Presentation and acceptance

_Applies to Groups 15-17._

**Entry gate.** The following public contracts must exist before dependent implementation in this area: `src/multilayer_industry.h`: `LayerIndustryCapability`, `ValidateMineDepositClaim`, `ProduceLayerMineCargo`, `PrepareLayerRecipeBatch`, `CommitLayerRecipeBatch`.
Verify each public file/export/schema field with `rg` in the current workspace and inspect its command/field ownership. For a same-plan contract, compile its type/signature fixture or validate its artifact after the preceding producer groups. For cross-plan behavior, also run the producer's focused executable boundary test; declarations alone cannot authorize dependent mutations or runtime claims.
If a required export is absent, stop the dependent implementation, mutation and runtime claims in Groups 15-17. Read-only characterization, source/range verification, test-case design and asset-template work that does not assume the missing behavior may continue.
Do not bypass this gate by copying private pool/tile state, inventing local IDs or cargo registries, using a historical plan as a delivered implementation, opening user-wide content stores, substituting a second utility/accounting authority, or adding an unreviewed fallback path. Finish the named producer or revise the shared contract and its direct consumers first.

**Scoped integration gates.** The following producer dependencies apply only to the named implementation/test families. Verify each required public export and its focused producer test before those integrations; absent late producers do not block already independent features.

- `src/underground_gui.h` is required for the complete conventional freight/player-fleet route scenario in Group 15 (only the named integration/subscenario). If absent, stop only that integration/subscenario and its passing claim. Other independent code, art, persistence and tests in this area may continue.

Do not emulate an absent producer with copied private state, fake inventories/energy, local IDs or a second registry/grid. A final combined closure/gallery/round-trip still waits for every required family; an independent passing case is not a pass for the missing integration.

| File name | Line Ranges | Description |
| --- | --- | --- |
| [src/cargopacket.h](../../../../src/cargopacket.h) | 596-603 | `StationCargoList::Reserve`, `Load`, `Truncate`, `Reroute` provide bounded station moves and destination reservations; hub inventory has one physical owner. |
| [src/economy_base.h](../../../../src/economy_base.h) | 20-38 | `CargoPayment`, `PayTransfer`, `PayFinalDelivery` own transport income. Visiting an origin station, a descent, or handling transfer is not automatic final payment. |
| [src/industry_cmd.cpp](../../../../src/industry_cmd.cpp) | 1242-1264 | `OnTick_Industry` owns production scheduling and editor exclusion; visible animations never produce power or minerals. |
| `src/multilayer_industry.h` (planned public contract; link after implementation) | Live definitions; no fixed range | [r2mlvs6-mines-and-resource-chains.md](r2mlvs6-mines-and-resource-chains.md) Groups 1-6, ending with Group 6's public-contract task, publish `LayerIndustryCapability`, `ValidateMineDepositClaim`, `ProduceLayerMineCargo`, `PrepareLayerRecipeBatch`, `CommitLayerRecipeBatch`. These exports/schema fields are authoritative for this area. Same-plan consumers follow those earlier groups; cross-plan consumers verify the producer's required implementation/tests as well. Proposed names alone do not prove delivery. |
| `src/underground_gui.h` (planned public contract; link after implementation) | Live definitions; no fixed range | [r2mlvs7-underground-construction-and-fleet.md](r2mlvs7-underground-construction-and-fleet.md) Groups 1-5, ending with Group 5's public-contract task, publish `ShowUndergroundConstructionToolbar`, `BuildUndergroundVehicleFilter`. These exports/schema fields are authoritative for this area. Same-plan consumers follow those earlier groups; cross-plan consumers verify the producer's required implementation/tests as well. Proposed names alone do not prove delivery. Read this producer for the complete conventional freight/player-fleet route scenario in Group 15 (only the named integration/subscenario); missing it blocks only that integration, not independent work. |
| [Sprite Contract and delivery owner](r2mlvs13-graphics-and-asset-pipeline.md#public-handoff-contract) | Live definitions; no fixed range | `Sprite Contract Handoff` defines manifest fields `asset_id`, `consumer`, `directions`, `frames`, `anchor`, `source`; the matching authoring/conversion/runtime groups deliver actual assets. Verify the requested family and packed loading fixture before graphics integration; template/manifest publication alone is not final art. |

**ASSUMPTION: this was already built previously. If not, re-asses at runtime**. This applies only to the producer contracts named in this area's gate, after their listed producer groups have run. Verify their actual exports and focused tests before using the sketches below; proposed types and helper names in sketches are not evidence of delivered code.

**Final-art gate. ASSUMPTION: this was already built previously. If not, re-asses at runtime**. Mine/receiver graphics integration requires r2mlvs13's corresponding manifest entries, packed assets and loading fixture. Verify those exact delivered assets before visual acceptance; absent art leaves those tasks pending while production/conservation tests continue. Placeholder renderings cannot close the plan.

**Scoped graphics gate. ASSUMPTION: this was already built previously. If not, re-asses at runtime**. Final graphics tasks in this area require the corresponding r2mlvs13 manifest entries, delivered sprites and packed loading fixture. Verify them before binding/review; if absent, stop only those art tasks and their acceptance claims, continue independent code/state tests, and never count placeholders as finished assets.

### Group 15 - Presentation and acceptance

{{MEDIUM}}
- [ ] 15.1 Show resource, remaining reserve, production, output storage, supply requirements and depletion status.
- [ ] 15.2 Integrate ten identifiable mine variants and Surface receiver graphics from r2mlvs13.
- [ ] 15.3 Test each mine's allowed/refused layer and footprint, duplicate claims, depletion, demolition/rebuild, full output, no nearby station and same-XY stations on other layers.
- [ ] 15.4 Operate all ten resources through trains/trucks and portals/elevators to Surface receivers, verifying actual acceptance and resulting products. At least one Deep chain must use two rail transitions and another the road elevator.

### Group 16 - Verify reserve and recipe persistence separately

{{HARD}}
- [ ] 16.1 Round-trip a mine under construction, partially depleted and exhausted; compare original reserve, remaining reserve, output buffers and consumed totals without resetting claims.
- [ ] 16.2 Round-trip a processor with one missing input, a full output and cargo in transit; compare packet/provenance plus committed recipe batches.
- [ ] 16.3 Exercise closure/rebuild and an industry on a hidden underground layer; verify no Surface-only effects/assertions or station catchment leak and no view-dependent production.

### Group 17 - Presentation and acceptance

{{SIMPLE}}
- [ ] 17.1 Record complete chain quantities, income, transit costs and bottlenecks for balancing. Missing consumers or invisible placeholder industries prevent completion.
- [ ] 17.2 Record the implemented public exports, focused test commands/results and remaining runtime/art/network gaps in this plan; retain original supplied saves and repository-local evidence.
- [ ] 17.3 Update affected architecture/handoff documentation to describe delivered behavior and reconcile the parent index only when this plan's Completion Conditions are met. Keep every unfinished task unchecked.

## Consult on Demand

| Trigger | File name | Start at | Expand only when | Consultation purpose |
| --- | --- | --- | --- | --- |
| A correct mine buffer is not accepted by a connected receiver | [../../../../src/economy.cpp](../../../../src/economy.cpp) | `DeliverGoodsToIndustry` | Layer catchment and produced/accepted cargo identity are already verified | Inspect actual acceptance, 65535 waiting saturation and history updates; do not create a second acceptance/payment store. |
| A required public export is absent and its owner/location is disputed | [r2mlvs-multilayer-vehicles-and-structures.md](r2mlvs-multilayer-vehicles-and-structures.md) | `Requirements and ownership` and `Plan Index` | The named producer confirms an intentional contract/ownership change | Find the responsible producer and reconcile its public handoff with direct consumers; the hub cannot substitute for missing implementation. |
| Current behavior contradicts a historical subsystem description | [../../architecture/map-layers/archi-map-layers.md](../../architecture/map-layers/archi-map-layers.md) | `Invariants` and the relevant named source in `Code map` | The live symbol and executable test do not explain the difference | Locate layer addressing or lifecycle rationale. Live code/tests govern delivered behavior; this architecture page does not prove future interfaces exist. |

## Planning Provenance

This is not an execution reading queue. Do not preload it after context compaction. It is non-transitive; ranges record planning use, not runtime authority.

| File name | Sections or Line Ranges used while planning | Planning contribution |
| --- | --- | --- |
| [docs/zz-reach2/architecture/initial-inventory.md](../../architecture/initial-inventory.md) | 185-220 | `Command as the only world mutation`, `Pool-indexed game objects`, `Tile map as a packed array` and `File-suffix modules` oriented command/pool/header ownership and source registration. This historical inventory is a location survey, not authority for current layer behavior. |
| [docs/zz-reach2/architecture/@archi-ttd-level0.md](../../architecture/@archi-ttd-level0.md) | 24-63 | `How the program fits together` and `Source and runtime boundaries` located shared simulation, data/presentation, CMake targets and subsystem handoffs used to choose the live reading sources. |
| [docs/zz-reach2/upgrades/2026-10/r2mlvs-multilayer-vehicles-and-structures.md](r2mlvs-multilayer-vehicles-and-structures.md) | 119-140 | `Requirements and ownership` assigned this plan's feature responsibilities; it shaped the decomposition without making every sibling plan a mandatory reading dependency. |
| [src/industry_cmd.h](../../../../src/industry_cmd.h) | 18-28 | The inspected live boundary helped separate characterization, public-contract production, implementation and acceptance batches. It materially constrained mine/deposit claims, finite output and useful layer-local acceptance; this entry records planning influence rather than an additional reading obligation. |
| [User planning-format instructions](<C:/Users/Axonn/.codex/attachments/cd59b20a-3a96-4d54-b4bd-1e8f37639e7d/Pasted text.txt>) | `Global Mandatory Reading`, `Area Mandatory Reading`, `Consult on Demand`, `Planning Provenance`, `Final validation` | Supplies the exact protocol, chronological difficulty batches of at most eight tasks, reading gates and validation format. |
| [src/industry_cmd.cpp](../../../../src/industry_cmd.cpp) | 525-554; 1995-2063 | Deeper planning pass, 2026-10-04: Shared construction and lossy legacy dispatch drove separate claim, extraction, finite handoff and recipe groups. |

## Completion Conditions

There are five distinct usable resources on each rock layer. Each has a built mine, a compatible vehicle, a genuine cross-layer route and a Surface consumer with visible economic effect. Generation, production and rebuilding cannot duplicate deposits or leak cargo through vertical catchment.
