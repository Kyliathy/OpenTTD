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

Status: resumed at the user's request, 2026-10-06. Groups 1-9 and independent material/accounting tasks through Group 16C are complete; handoff/evidence tasks 17.2 and 17.3A are complete. Useful-power, native freight, tube and final-art integrations remain gated on their absent producers; full financial acceptance and parent completion remain unchecked. The separate portal-click crash is fixed and playable language-pack snapshots protect both configurations during test-only rebuilds. Parent: [R2MLVS master](r2mlvs-multilayer-vehicles-and-structures.md). Depends on r2mlvs2/5 and the r2mlvs1 content contract.

## Outcome and Boundaries

Make all ten proposed mine resources productive and transportable to useful Surface consumers. Deep mines obey the 1980 calendar unlock; r2mlvs17 adds geothermal power and later direct-heat receivers without replacing the five Deep mineral types or removing their Surface receivers.

## Public Handoff Contract

Delivered public owner: [`src/multilayer_industry.h`](../../../../src/multilayer_industry.h). `LayerIndustryCapability`, `ValidateMineDepositClaim`, `ProduceLayerMineCargo`, `PrepareLayerMineDispatch`/`CommitLayerMineDispatch`, `PrepareLayerRecipeBatch` and `CommitLayerRecipeBatch` are implemented and compiled against their focused producer fixtures. `CmdBuildLayerMine` and `CmdSelectLayerIndustryRecipe` are appended command traits in [`src/industry_cmd.h`](../../../../src/industry_cmd.h); UI/script consumers use the same admission/calendar rules.

| Delivered boundary | Sole authority and consumer rule |
| --- | --- |
| Mine definitions, geometry and finite production | Exact first-party IIDS/TIDS identities and label-resolved recipes; geological claims/reserve stay with `excavation.h`. Industry output owns physical mineral units. No name, artwork, viewport layer or generic cargo slot grants mine capability. |
| Processor choice/work/batches | [`layer_industry_processor.h`](../../../../src/layer_industry_processor.h) publishes selected recipe, controller, work and historical counters. Every incoming/outgoing unit remains in existing `Industry::accepted`/`produced` slots. Exact final-delivery receipts retain refused cargo aboard the real vehicle. |
| Baseline Steel reuse | [`layer_industry_baseline.h`](../../../../src/layer_industry_baseline.h) identifies the compatible original one-to-one Iron Ore/Steel receiver and bounds conversion/supply without duplicate cargo/industry/recipe IDs. Coal identity characterization grants no useful-power implementation. |
| Shared complete batch commit | `LayerRecipeInventoryView` and `LayerRecipeCommitParticipant` expose one real stock owner. Proposals bind immutable definition, owner and saved revision; foreign/stale/repeated or partial commits refuse unchanged. Scalar heat/power commitments still refuse until r2mlvs12 supplies actual grant/atomic participants. |
| Save/preview/activation | Version 379 `LMNH`/`LMNR` and version 380 `LPCH`/`LPCR`/`LPCC` transport bindings/choices/work/history only. Metadata bits 256/512 advertise actual owners. After final industry/geology rebuild, complete reference/role/stock validation precedes activation; claims/reserves are never recreated. Existing industry/station/vehicle/packet formats own cargo and provenance. Transient supply/batch proposals are never saved. |

Keep one authority for these fields and commands. Later r2mlvs11/17 inventory owners consume the exported batch interface, and r2mlvs7/8/12/13 supply their own still-pending fleet/tube/grid/art implementations; this producer does not emulate them.

## Implementation Tasks

Deepening pass, 2026-10-04: implementation groups use only SIMPLE, MEDIUM or HARD difficulty, with at most eight pending tasks each. That original planning pass marked no implementation complete. Current checked tasks below describe delivered code and focused evidence; unchecked producer-dependent integrations retain their explicit gates.

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
- [X] 1.1 Trace one funded industry from command validation through tile placement and stable IndustryID creation; list Surface-only checks, footprint iteration and callback inputs needing layer-aware behavior.
- [X] 1.2 Trace ticking/TileLoop, effects, climate/water and production separately. Classify each behavior as supported underground, disabled by capability or still Surface-only; removing one assertion is not the change.
- [X] 1.3 Trace station association, town proximity, closure, news/lists and script access. Record the authoritative layer coordinate at each boundary and a failing fixture for any Surface-only assumption.

Evidence: [live lifecycle trace and required failure fixtures](r2mlvs6-a-industry-lifecycle.md#group-1-lifecycle-characterization). All named producer exports exist; focused producer checks passed 7,625 assertions in 12 cases. Existing town projection, plane distance and layer-bounded catchment work are preserved.

### Group 2 - Layer-aware industry lifecycle

{{SIMPLE}}
- [X] 2.1 Record each change; removing the Surface assertion alone is insufficient.

Evidence: [change ledger](r2mlvs6-a-industry-lifecycle.md#group-2-change-ledger-21) records the separate admission, claim, ticking, conservation and presentation obligations; subsequent completed implementation groups append their changes and checks there.

### Group 3 - Layer-aware industry lifecycle

{{MEDIUM}}
- [X] 3.1 Add a layer/capability whitelist for new mine and processor definitions. Ordinary NewGRF industries remain Surface unless explicitly supported; fallback/substitute graphics do not grant placement permission.
- [X] 3.2 Implement admission for mine funding only on fully excavated footprints touching a suitable unclaimed deposit. Group 4 performs the complete command/claim/tile commit; admission alone does not authorize live funding.

Implementation boundary clarified during execution: Group 3 owns pure eligibility and Group 4 owns actual atomic construction. [Native admission evidence](r2mlvs6-a-industry-lifecycle.md#group-3-native-admission-implementation): Debug compilation and `[layer-industry-admission]` passed 99 assertions in four cases. Surface processor policy remains explicit Surface permission; their recipe identities are installed in Group 10.

### Group 4 - Build and bind one mine atomically

{{HARD}}
- [X] 4.1 Prepare a mine placement request with complete machinery/loading approach footprint, selected deposit ID and allowed layer/type/date. Verify r2mlvs2 excavation and reserve/claim queries before any tile clearing.
- [X] 4.2 Put admission in the shared `CreateNewIndustryHelper` path as well as command UI: `CreateNewIndustry` bypasses `CmdBuildIndustry`. Replace default Surface-random prospecting for geological mines with explicit deposit selection; do not let generation bypass date/claim checks.
- [X] 4.3 Validate same-company claim rules, seam contact, unclaimed available reserve, access and complete construction cost. Reserve industry/claim capacity before tile mutation.
- [X] 4.4 Commit industry identity, map footprint and deposit claim together; on failure retain both geology and tiles. `DoCreateNewIndustry` currently ignores execution-time clear failures, so complete preflight before its first write and handle failure explicitly. Test one unexcavated corner, competing claim, depleted deposit and insufficient funds.
- [X] 4.5 Define rebuilding as a new industry attached to the original remaining reserve; demolition releases the claim without recreating geological material.

Evidence: [atomic funding and rebuilding](r2mlvs6-a-industry-lifecycle.md#group-4-atomic-funding-and-rebuilding). The geological owner now publishes stamped claim/release/debit operations; mine placement never mutates a copied snapshot. Debug build passed; focused admission/geology/funding checks passed 239 assertions in eight cases. Actual ten-definition registration and persistence remain later groups, so this is not yet a playable-chain claim.

### Group 5 - Layer-aware industry lifecycle

{{MEDIUM}}
- [X] 5.1 Bind mine and deposit with stable IDs.
- [X] 5.2 Define depletion, closure, abandonment and rebuilding so rebuilding cannot reset the original reserve or claim the same seam twice.
- [X] 5.3 Keep station acceptance/catchment and cargo generation within the industry's own layer. Provide explicit layer names and navigation in lists, news and vehicle orders.
- [X] 5.4 Apply the shared Deep calendar availability rule to funding, automatic construction, activation, production and script entry points. Scenario/editor authoring must not silently enable operating Deep industries before 1980 in normal new-profile play.

Evidence: [lifecycle, layer names and shared availability](r2mlvs6-a-industry-lifecycle.md#group-5-lifecycle-layer-names-and-shared-availability). Debug build and focused mine checks passed 346 assertions in 12 cases; a separate small Surface receiver compatibility burst passed. Active/depleted/abandoned states and output-preserving closure are defined here; Group 7 installs finite extraction and its depletion transition, Group 8 installs exact dispatch, and later groups publish/validate save transport. Native definitions are not yet installed in real games.

### Group 6 - Publish layer-industry and recipe boundaries

Resumed after the separate portal repair, 2026-10-06. The public header and signature fixture compile, and `[layer-industry-contract]` passed 27 assertions in one case (`r2mlvs6-aa-contract-checks.log`). Extraction and recipe execution remain Group 7's callable producer obligation; publishing these declarations alone grants no downstream runtime mutation. [Crash diagnosis and startup evidence](r2mlvs6-a-industry-lifecycle.md#paused-portal-crash-investigation) record the separate repair.

{{HARD}}
- [X] 6.1 Publish `src/multilayer_industry.h` with `LayerIndustryCapability`, `ValidateMineDepositClaim` and `ProduceLayerMineCargo`, including ownership, invalid IDs and command/estimate rules.
- [X] 6.2 Declare proposed `PrepareLayerRecipeBatch` and `CommitLayerRecipeBatch` for shared bounded all-input processing. Accept an explicit inventory owner/view and commit participant rather than requiring `Industry*`, so a later orbital `SpaceStructure` can reuse them. Use r2mlvs1 recipe IDs/units; separate reserve debit, delivered stock and future utility grants.
- [X] 6.3 Compile the public header fixture and register sources. The following production groups implement the callable recipe/extraction behavior; declarations alone do not authorize downstream mutation.

### Area Mandatory Reading - Resource production and consumers

_Applies to Groups 7-14._

**Entry gate.** The following public contracts must exist before dependent implementation in this area: `src/multilayer_industry.h`: `LayerIndustryCapability`, `ValidateMineDepositClaim`, `ProduceLayerMineCargo`, `PrepareLayerRecipeBatch`, `CommitLayerRecipeBatch`.
Verify each public file/export/schema field with `rg` in the current workspace and inspect its command/field ownership. For a same-plan contract, compile its type/signature fixture or validate its artifact after the preceding producer groups. For cross-plan behavior, also run the producer's focused executable boundary test; declarations alone cannot authorize dependent mutations or runtime claims.
If a required export is absent, stop the dependent implementation, mutation and runtime claims in Groups 7-14. Read-only characterization, source/range verification, test-case design and asset-template work that does not assume the missing behavior may continue.
Do not bypass this gate by copying private pool/tile state, inventing local IDs or cargo registries, using a historical plan as a delivered implementation, opening user-wide content stores, substituting a second utility/accounting authority, or adding an unreviewed fallback path. Finish the named producer or revise the shared contract and its direct consumers first.

**Scoped integration gates.** The following producer dependencies apply only to the named implementation/test families. Verify each required public export and its focused producer test before those integrations; absent late producers do not block already independent features.

- `src/transport_tube.h` is required for the tube-terminal cargo/refit integration in Group 14 (only the named integration/subscenario). If absent, stop only that integration/subscenario and its passing claim. Other independent code, art, persistence and tests in this area may continue.
- r2mlvs7 Group 7's operational mine trucks/mineral wagons/sealed carriers and purchase/refit fixtures are required for Group 12's native-fleet service, route and refit cases (12.1B/12.2B/12.3B). The r2mlvs1 logical model catalogue is not installed EIDS behavior. Stop only those native-fleet integrations when absent; packet accounting, baseline compatibility, Surface manufacturing and persistence remain independent. The complete visible player workflow additionally keeps Group 15's `src/underground_gui.h` gate.

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
- [X] 7.1 Implement mine extraction amount as the minimum of allowed work, remaining deposit reserve and free output space. Debit reserve only when the same quantity reaches accountable output; no clamped-away production.
- [X] 7.2 Implement `PrepareLayerRecipeBatch` from the profile all-input recipe record: compute the minimum feasible batch count over every input, every output slot and work budget with checked integers.
- [X] 7.3 Implement `CommitLayerRecipeBatch` with revalidation and one input/output/work commit. Use the chosen content callback/native path; default independent-input multipliers cannot substitute for the shared batch executor.
- [X] 7.4 Test missing secondary input, full one-of-many output, partial reserve and near-counter-limit values. Keep supplied service modifiers optional at bootstrap and preserve buffers while paused.

Evidence: [finite production and atomic batch execution](r2mlvs6-a-industry-lifecycle.md#group-7-finite-production-and-batch-execution). Debug compilation passed; `[layer-mine-production],[layer-recipe-batch]` passed 647 assertions in nine cases and the content recipe producer passed 112 assertions in four cases. Native mine ticks use the sole geological/output owners and preserve exact saved rational work. Scalar heat/power planning remains read-only; utility-backed commits stay refused until the scoped producer gate is met. Finite station dispatch remains Group 8.

### Group 8 - Dispatch finite output without loss or fractional double credit

{{HARD}}
- [X] 8.1 Separate extraction from station dispatch. Update production history once when reserve becomes output; dispatch only updates transported quantities. Apply any rate/recession modifier before extraction or record explicit loss, never silently discard finite stock.
- [X] 8.2 Implement a bounded integer dispatch ticket over eligible same-layer stations, output units and packet/storage capacity. Commit the station supply and subtract exactly those units from mine output together; no station or packet room means stock remains.
- [X] 8.3 Account for `UpdateStationWaiting` fractional credits explicitly: use a producer-owned saved dispatch remainder or a dedicated exact-integer path, not repeated calls on unchanged output that each add station fractions.
- [X] 8.4 Test no station, zero rating, packet-pool exhaustion, fractional rate, partial station capacity and stale/repeated/aborted commit retries. Compare reserve + mine stock + station/vehicle stock + recorded consumption; no unit disappears or receives credit twice. Group 16 adds actual save/load retry once mine/recipe transport has been installed; Group 8's transient tickets are never saved.

Execution clarification: legacy station queues have no small explicit storage bound. The exact native supply owner now declares a per-cargo queue cap of `UINT16_MAX` whole units, checks actual packet-backed totals, and leaves all excess material at its mine. A pre-existing larger queue is preserved and receives no new finite supply. Actual save/load retry belongs to Group 16 because side-state persistence is not yet delivered here; this adjustment completes no persistence task.

Evidence: [exact finite dispatch and real packet-capacity refusal](r2mlvs6-a-industry-lifecycle.md#group-8-exact-dispatch-and-packet-capacity). Debug dispatch/production checks passed 912 assertions in nine cases. The isolated optimized case filled the actual packet pool, verified unchanged refusal, freed one real slot and committed exactly four units: 37 assertions in one case passed in 1.5 seconds (`r2mlvs6-ah-packet-pool-checks.log`). Optimized test and playable game builds passed; no whole-suite regression was run.

### Group 9 - Instantiate the ten mine definitions

{{MEDIUM}}
- [X] 9.1 Register Underground Coal, Iron Ore, Copper Ore, Bauxite and Salt definitions with distinct cargo labels, geology capability and layouts; validate each uses the common extraction path.
- [X] 9.2 Register Deep Lithium Ore, Nickel Ore, Cobalt Ore, Rare Earth concentrate and Uranium with the shared 1980 gate, deeper equipment capability and harder access costs.
- [X] 9.3 Map all ten mines to logical art IDs/overlays and test their label-to-runtime-slot resolution. Geology determines reserve; a new mine instance never grants fresh reserves. Actual authored/packed artwork remains r2mlvs13's delivery and the later acceptance gate; logical binding does not complete it.

Evidence: [actual native catalogue, footprint and identity checks](r2mlvs6-a-industry-lifecycle.md#group-9-native-catalogue-and-footprints). Debug `[layer-mine-definition]` passed 1,108 assertions in two cases (`r2mlvs6-am-definition-checks.log`), including every resource's actual funding and finite output. All definitions reuse IIDS/TIDS and exact labels, match the manifest's 3x3 machinery shape, have no legacy rate/callback/random-generation path, and create no geological reserve during installation. Ten logical art families are published; packed mine artwork is still planned in its producer manifest.

Context has compacted 1 times while executing this group.

### Group 10 - Build the Surface processing chain from shared recipes

{{MEDIUM}}
- [X] 10.1 Add metal and chemical works using the published all-input executor and content quantities; keep accepted packet inventory distinct from recipe consumption.
- [X] 10.2 Add battery and electronics plants, then component/robotics/mirror-kit/propellant production with their frozen recipes. Check actual incoming labels and available output capacity for each.
- [ ] 10.3 Add nuclear/useful-power acceptance through the single utility authority; Uranium must have a functioning Surface consumer when Deep opens, before later geothermal manufacturing.
- [X] 10.4 Keep minimal Earth/Rock receivers owned by r2mlvs5 and reuse their exact-count intake contract. Test a partially full receiver retains unaccepted freight instead of ordinary delivery silently deleting surplus.

Evidence: [native processing and exact accepted-count delivery](r2mlvs6-a-industry-lifecycle.md#group-10-native-processors-and-final-delivery). All seven command-funded Surface families and ten material alternatives use the shared executor, actual Industry stocks and exact station supply. Debug compilation/game build passed; focused processing, packet delivery and spoil compatibility passed 9,923 assertions in ten cases (`r2mlvs6-be-processor-delivery-checks.log`). Actual packed factory artwork and processor save transport remain later obligations. `src/utility_power.h` is absent: task 10.3 remains unchecked, no Uranium/useful-power acceptance is installed, and only independent material work proceeds under the area's scoped gate.

### Group 11 - Reuse compatible baseline receivers explicitly

{{MEDIUM}}
- [X] 11.1 Resolve Coal and Iron Ore by selected-profile label and inspect the active power-plant/steelworks definitions. Require matching accepted labels, output meaning and supported production path; names/graphics alone are insufficient. The original one-to-one Steel path is supported; the Coal identity matches intake but has no delivered useful-power owner.
- [X] 11.2A Reuse the compatible original Steel receiver identity without registering another definition or recipe ID. Reject mismatched cargo, callbacks, independent rate output and unsupported conversion ratios; report an unsupported active definition instead of guessing a fallback.
- [ ] 11.2B Reuse or register the profile's Coal useful-power receiver after the single utility producer publishes/tests its handoff. No local grid or new power acceptance may substitute for it.
- [X] 11.3A Deliver an actual Iron packet load to the selected original receiver, verify exact accepted quantities and Steel production on the real tick dispatcher, and preserve the existing industry/cargo/recipe slot budgets. Test full output and later exact station supply without losing input.
- [ ] 11.3B Deliver Coal through the useful-power path and check resulting utility generation/allocation after the producer gate is satisfied.

Execution refinement: split receiver reuse/acceptance by material and useful-power family so the area's explicit scoped gate remains visible. [Baseline compatibility and bounded Steel evidence](r2mlvs6-a-industry-lifecycle.md#group-11-existing-baseline-receiver-compatibility) records the original 1:1 semantics and unchanged runtime identities. Focused baseline/packet/spoil checks passed 2,029 assertions in six cases (`r2mlvs6-bm-baseline-delivery-checks.log`). Incompatible third-party definitions have no frozen fallback recipe/identity in this profile and remain a reported refusal; useful-power tasks stay unchecked.

### Group 12 - Integrate demand and freight accounting on explicit routes

{{HARD}}
- [X] 12.1A Connect finite industry output to same-layer stations. Test same-XY stations on different layers receive no free cargo or demand edge; source supply creates no service edge by itself.
- [ ] 12.1B Exercise ordinary CargoDist service links with the operational native mine-freight models after r2mlvs7 Group 7 supplies their installed definitions and focused purchase/refit fixtures.
- [X] 12.2A Verify paid cargo-vector distance uses plane coordinates. Compare equivalent horizontal movement on each layer and reject a stacked-Y reward after free movement within a transfer station, while retaining the existing saved vector representation.
- [ ] 12.2B Compare actual feasible native freight routes through portals/elevators after the native carrier producer is delivered; coordinate/accounting fixtures do not prove driven route feasibility.
- [X] 12.3A Cap processor intake before packet removal, retain refused quantities and exact source-station/packet identity, and settle feeder shares only for admitted units. Profile payment categories retain the content/economy owners.
- [ ] 12.3B Integrate the native train/truck refit and payment cases after r2mlvs7 Group 7's model/capability bindings and focused producer fixtures exist.
- [X] 12.4 Run the connected Surface manufacture bootstrap for launch fuel, components, Robotics Parts and Solar Mirror Kits from explicit earlier raw shipments. Use real factory buffers and station/carrier packets; no orbital facility supplies an input. Procurement costs and the driven player-fleet scenario remain their separate acceptance obligations.

Execution refinement: separate independent custody/geometry/manufacturing from native-fleet integration. [Freight accounting and Surface chain evidence](r2mlvs6-a-industry-lifecycle.md#group-12-freight-accounting-and-surface-manufacturing) records 199 assertions in six accounting/delivery cases (`r2mlvs6-br-freight-checks.log`) and 173 assertions in the connected manufacture case (`r2mlvs6-bt-surface-chain-checks.log`). r2mlvs7's native freight model/refit group is unchecked and its public GUI/filter header is absent; no invented engine IDs, fake vehicle-service edges or claimed driven routes substitute for it.

### Group 13 - Expose a utility participant for later direct-heat recipes

{{HARD}}
- [ ] 13.1 Extend the shared recipe proposal with an optional authoritative utility grant token/quantity and recipe selection. The future geothermal owner supplies heat; the mine reserve API is not reused as a heat source.
- [ ] 13.2 Specify prepare/commit/cancel ownership for the grant: no inputs or heat are consumed if output capacity or one transported input is absent. A refused/retried batch cannot consume the grant twice.
- [X] 13.3A Publish read-only quantity/refusal fixtures for all six future 2010 smelter, 2030 battery and 2045 Forge choices. Supplied scalar heat/power may bound planning; commits refuse without native utility custody, and missing input/full output consume nothing. This is no claim that r2mlvs17 plants or actual grants exist.
- [ ] 13.3B Validate the real typed utility grant/epoch/consume/release/retry participant once r2mlvs12's tested native handoff exists; scalar fixtures cannot complete that integration.
- [X] 13.4 Document the exact delivered public material batch fields/signatures for r2mlvs11/r2mlvs17 and the blocked utility-token extension. Scheduling, allocation and buildings remain with those owners; no copied account grants runtime authority.

Evidence: [delivered batch API and blocked utility participant](r2mlvs6-a-industry-lifecycle.md#group-13-delivered-batch-api-and-blocked-utility-participant). Focused `[layer-recipe-heat-boundary]` passed 126 assertions in one case (`r2mlvs6-by-heat-contract-checks.log`). r2mlvs12's `Useful Power Handoff` confirms the missing owner of `UtilityGrantID`/atomic mutations. Tasks 13.1/13.2/13.3B stay unchecked; no substitute token, registry or grid was declared.

### Group 14 - Resource production and consumers

{{MEDIUM}}
- [X] 14.1A Keep finite reserve extraction, delivered inputs, recipe consumption and output/dispatch separately accountable through their sole owners. Direct heat is neither mineral reserve nor freight and has no runtime mutation while its utility producer is absent.
- [ ] 14.1B Complete all five Deep minerals' earlier useful Surface receivers after the Uranium useful-power gate is satisfied; current Lithium/Nickel/Cobalt/Rare Earth processors cannot count as Uranium acceptance.
- [ ] 14.2 Integrate tube terminal cargo/refits with r2mlvs16. Mine/factory freight may transfer to tubes on its own layer and back to conventional transport for inter-layer movement; tubes neither replace mandatory TBM spoil-wagon hauling nor open a pre-1980 Deep route.

Scoped gate evidence: `src/transport_tube.h` and `src/utility_power.h` are absent. Only their dependent acceptance/integration tasks remain blocked; no tube inventory, heat source or grid is emulated. Existing geological debit, physical stock and exact packet receipts are characterized in the preceding focused checks.

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

Context has compacted 1 times while executing this group.

### Group 15 - Presentation and acceptance

{{MEDIUM}}
- [X] 15.1 Show resource, remaining reserve, production, output storage, supply requirements and depletion status.
- [ ] 15.2 Integrate ten identifiable mine variants and Surface receiver graphics from r2mlvs13.
- [X] 15.3 Test each mine's allowed/refused layer and footprint, duplicate claims, depletion, demolition/rebuild, full output, no nearby station and same-XY stations on other layers.
- [ ] 15.4 Operate all ten resources through trains/trucks and portals/elevators to Surface receivers, verifying actual acceptance and resulting products. At least one Deep chain must use two rail transitions and another the road elevator.

Independent acceptance evidence: `r2mlvs6-cb-presentation-lifecycle-checks.log` records 2,477 assertions across 19 focused mine cases; `r2mlvs6-co-window-paint-checks.log` records 81 assertions across three actual mine/refinery/portal window cases. The shared fixture supplies valid sprite metadata and an empty drawing clip; this exercises real UI construction and refresh, not final raster-art acceptance. All 17 requested art families remain planned, and the native-fleet producer gate still blocks 15.4.

### Local Mandatory Reading - Mine persistence

_Applies to Group 16; narrows the preceding Presentation area to actual reserve/binding serialization._

Read `LayerMineState`, `ResetLayerIndustryState`, `ChangeLayerIndustryOwnership` and production/dispatch in `src/layer_industry_mine.cpp`; geology claim/reserve ownership in `src/geology.cpp`; staged header/table and preview handling in `src/saveload/spoil_receiver_sl.cpp`; metadata feature/version checks in `src/multilayer_state.cpp` and `src/saveload/multilayer_sl.cpp`; final industry validation in `AfterLoadGame`. Physical output and packets remain in their existing save owners. No recreation of missing claims or reserve is permitted.

### Group 16 - Verify mine reserve persistence

{{HARD}}
- [X] 16.1 Round-trip a mine under construction, partially depleted and exhausted; compare original reserve, remaining reserve, output buffers and consumed totals without resetting claims.

Delivered save version 379: mandatory `LMNH`/`LMNR` family, metadata feature 256, independent preview reset/checks and final claim/stock validation after real industry/geology rebuild. `r2mlvs6-df-mine-save-checks.log` records 495 focused assertions. Actual game reload/save preserved all mine, original/remaining reserve and seam rows for construction/partial/exhausted saves (`r2mlvs6-dk-mine-game-roundtrips.log`). Missing header, wrong count and wrong saved company refused without saving (`r2mlvs6-do-mine-refusals.log`). Source saves are preserved; transient station tickets are never serialized.

### Local Mandatory Reading - Processor persistence

_Applies to Group 16B; use the Presentation area's existing capability gate._

Read `LayerProcessorState`, inventory capture/validation, `AdvanceLayerProcessor` and recipe selection in `src/layer_industry_processor.cpp`; installed input/output unions in `src/layer_industry_definitions.cpp`; native ingredient receipt in `DeliverGoodsToIndustry`; the delivered Group 16 save family and existing `Industry`, station and vehicle packet descriptors. Restore counters/choice only after exact runtime definition/stock validation; never copy physical cargo into a side store.

### Group 16B - Verify processor persistence

{{HARD}}
- [X] 16.2 Round-trip a processor with one missing input, a full output and cargo in transit; compare packet/provenance plus committed recipe batches.

Delivered save version 380: mandatory `LPCH`/`LPCR`/`LPCC`, metadata feature 512, preview reset/checks and exact frozen role/batch/stock activation. Focused restoration/malformed/carrier cases passed 2,131 assertions (`r2mlvs6-eh-processor-carrier-checks.log`); final native-layout full-save cases passed (`r2mlvs6-en-processor-native-carrier-checks.log`, with the later actual named-station carrier checkpoint in `r2mlvs6-er-carrier-save-checks.log`). Actual missing-input/full-output reloads retained stock/choice/history digests (`r2mlvs6-el-processor-game-roundtrips.log`, its transit attempt was subsequently corrected); the real wagon transit round trip and all three corrupted-family refusals passed in `r2mlvs6-es-processor-carrier-roundtrip.log`. Packet bytes/provenance stayed unchanged. The fixture uses an existing original Surface wagon and normal engine-pool initialization; native-fleet purchase/refit/driven-route acceptance remains under 12.1B/12.2B/12.3B/15.4.

### Local Mandatory Reading - Industry lifecycle acceptance

_Applies to Group 16C; use the Presentation area's existing capability gate._

Read delivered Group 16/16B load validators and focused fixtures; mine/processor removal guards and company takeover/removal in `src/industry_cmd.cpp` and `src/economy.cpp`; `MultiLayerMap` and `OnTick_Industry` ownership. Characterize recovery using actual stocks and geological claims; selected camera layers cannot alter production or catchment.

### Group 16C - Verify industry lifecycle acceptance

{{HARD}}
- [X] 16.3 Exercise closure/rebuild and an industry on a hidden underground layer; verify no Surface-only effects/assertions or station catchment leak and no view-dependent production.

Actual viewport Surface/Deep/Space selection leaves Deep extraction at four units per 74 ticks and creates no Surface effects or same-XY station supply. Stopped mines retain work/reserve/output; abandoned output can be recovered through an eligible same-layer station before whole-site rebuild against the original remaining reserve. Recipe-controller recovery, deity initialization and idempotent registration preserve stocks/history. Focused lifecycle plus relevant processor/dispatch/window checks passed 1,263 assertions in 16 cases (`r2mlvs6-ey-industry-recovery-checks.log`).

### Local Mandatory Reading - Handoff and remaining acceptance

_Applies to Group 17; retains the Presentation area's capability/fleet/art gates._

Read this plan's delivered public handoff, completed task evidence and unchecked scoped producer gates; the Group 16/16B/16C sections in `r2mlvs6-a-industry-lifecycle.md`; actual exported ownership in `src/multilayer_industry.h`, `src/layer_industry_processor.h` and `src/layer_industry_baseline.h`; named industry/save architecture sections being updated. Verify only the exact mine/receiver art families and missing utility/fleet/tube publishers before final closure claims. Preserve supplied saves and do not reconcile the parent as complete while these gates remain open.

### Group 17 - Presentation and acceptance

{{SIMPLE}}
- [ ] 17.1 Record complete chain quantities, income, transit costs and bottlenecks for balancing. Missing consumers or invisible placeholder industries prevent completion.
- [X] 17.2 Record the implemented public exports, focused test commands/results and remaining runtime/art/network gaps in this plan; retain original supplied saves and repository-local evidence.
- [X] 17.3A Update affected architecture/handoff documentation to describe delivered behavior and explicit remaining producer gates.
- [ ] 17.3B Reconcile the parent index only when this plan's Completion Conditions are met. Keep every unfinished task unchecked.

### Focused execution evidence and remaining gates

From `build`, focused C++ checks use `Debug/openttd_test.exe '<tags>' --durations yes`; build with `cmake --build build --config Debug --target openttd_test --parallel 4`. Actual save fixtures run from the repository with `python -B src/tests/layer_mine_save_fixture.py` and `python -B src/tests/layer_processor_save_fixture.py`. Both preserve supplied source hashes and the original startup script; their saves and processor checkpoints live in `src/tests/fixtures/mines/` and `src/tests/fixtures/processors/`, respectively, beside the tests; logs live beside this plan. Individual evidence files are named on the completed groups above and in [`r2mlvs6-a-industry-lifecycle.md`](r2mlvs6-a-industry-lifecycle.md).

| Focused family | Delivered evidence |
| --- | --- |
| Mine status, definitions, admission, finite production and exact dispatch | 2,477 assertions / 19 cases; real mine/refinery/portal control construction and paint: 81 assertions / 3 cases. |
| Real final-delivery receipts, partial spoil receipts and processors | 9,923 assertions / 10 cases; baseline Steel and related packet boundaries: 2,029 assertions / 6 cases. |
| Plane-distance/provenance and Surface manufacturing | 199 assertions / 6 packet/accounting cases; connected Surface materials: 173 assertions / 1 case. Raw shipments are explicit fixture inputs, not demonstrated mine-route procurement. |
| Scalar-energy refusal | 126 assertions / 1 case over all six direct-heat rows; actual utility grants remain absent. |
| Mine persistence | 495 assertions / 3 save/restore cases plus actual construction/partial/exhausted game reloads; missing family/count/company corruptions refuse without output saves. |
| Processor persistence | 2,131 assertions / 3 focused restore/malformed/carrier cases, followed by actual native-layout missing-input/full-output/wagon-transit reloads with exact stock/history digest and packet comparison. Final carrier checkpoint: 54 assertions. |
| Lifecycle/recovery and relevant boundary rechecks | 1,263 assertions / 16 cases including actual viewport layer changes, closure, abandoned-stock recovery/rebuild, recipe control/deity/idempotence and real portal/window paint. |

Material accounting witness: given CORE 20, COAL 5, SALT 6, OIL_ 6, STEL 4, RARE 2, LITH 2 and COBT 1, actual Surface batches and packet receipts leave PROP 2, SCMP 2, RBPT 2, SMIK 1 and RFMT/CHEM/BATY/ELEC one each, with raw buffers empty. This reduced fixture is zero-priced and supplies no actual income, driven transit cost or all-ten-resource balancing claim; 17.1 remains unchecked.

Remaining completion gates, reverified 2026-10-06: `src/utility_power.h`, `src/underground_gui.h` and `src/transport_tube.h` are absent; native mine truck/mineral wagon/sealed-carrier purchase/refit/service from r2mlvs7 Group 7 is not delivered. All ten `mine.*` and seven material-processor `facility.*` manifest families still have `source.status = planned` and no source path/packed delivery. Useful Coal/Uranium power, real typed heat grants, native fleet service/portal routes, tubes, final art, full financial balancing and multiplayer runtime remain unverified and unfinished. Parent completion/index reconciliation waits for those exact obligations; no full regression was run.

Delivered architecture updates: [industry construction](../../architecture/industries/archi-industry-construction.md), [cargo payment](../../architecture/economy/archi-cargo-payment.md) and [savegame state](../../architecture/savegame/archi-savegame.md). They describe actual native admission, finite/whole-batch ownership, exact accepted-count payment, preserved packet encoding, bounded save families and stock recovery. The parent index remains open; 17.3B is unchecked.

Final playable Debug and RelWithDebInfo builds passed, including per-configuration language pairing (`r2mlvs6-ez-debug-game-build.log`, `r2mlvs6-fa-release-game-build.log`, followed by the final recovery/exclusivity rebuilds `r2mlvs6-fc-recovery-exclusivity-build.log` and `r2mlvs6-fe-release-recovery-build.log`). Fresh-game startup using the real `build/play.cfg` and hidden null media exited zero for both configurations. The final recovery/exclusivity lifecycle burst passed 180 assertions in five cases (`r2mlvs6-fd-recovery-exclusivity-checks.log`).

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
