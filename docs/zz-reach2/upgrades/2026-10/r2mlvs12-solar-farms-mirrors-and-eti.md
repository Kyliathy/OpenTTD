# R2MLVS12 - Solar Farms, Mirrors and Energy Transition Industries

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
| [docs/zz-reach2/upgrades/2026-10/r2mlvs-multilayer-vehicles-and-structures.md](r2mlvs-multilayer-vehicles-and-structures.md) | 288-294 | `AD-13` supplies the canonical gameplay/ownership decision for every group of this plan. It constrains one source-side solar augmentation and no duplicate utility/grid authority; a consumer must not redefine it locally. |
| [src/map_layer.h](../../../../src/map_layer.h) | 78-85 | `MultiLayerMap` ownership preamble: `Map` owns packed storage/geometry and this controller owns layer semantics. Every group uses world tiles/explicit layer helpers; camera selection and ad hoc plane offsets cannot define gameplay. |
| [src/newgrf_industries.h](../../../../src/newgrf_industries.h) | 70-89 | `IndustryAvailabilityCallType`, `IndustryProductionCallback`, `MapNewGRFIndustryType` and callback declarations define availability/GRFID mapping; no translated-name or cargo-label-only adapter. |

Status: initial M0 native-fallback decision, draft solar contract and tested [Useful Power Handoff](portal-tbm/r2mlvs3mp-x-useful-power-handoff.md) are delivered; whole feature plan remains unfinished. [Execution evidence](r2mlvs12-a-eti-feasibility.md) reproduces the pinned source/hash precheck. ETI runtime tasks remain unchecked; no adapter is approved. Solar/mirror runtime and final art retain their later tasks. Parent: [R2MLVS master](r2mlvs-multilayer-vehicles-and-structures.md).

**Artifact-layout migration (2026-10-09):** this plan consumes the parent [Development artifact layout](r2mlvs-multilayer-vehicles-and-structures.md#development-artifact-layout), implemented by [R2FC](r2fc-file-cleanup.md). It supersedes earlier docs asset/review/helper placement and fixed runtime-config destinations below. New native homes use `mlm/runtime/`, generated feature reviews use `mlm/reviews/`, logs remain under `logs/`, and reusable code/curated inputs keep their code/test owners. Paths and focused native/cold verification are complete; R2FC records the results and retained-input identities. Repository-relative executable commands run from the checkout root through their context-aware wrappers. Historical pass records remain dated evidence and are not new acceptance claims. No gameplay task is completed by this migration and the separate post-demo fixture-pruning gate remains.

## Outcome and Boundaries

Surface solar farms receive useful additional power from launched orbital mirrors. Each supported farm has at most one +300% bonus, for 4x baseline output.

## Investigation record

Upstream is Nathan Pitman's [Open-TTD-Renewable-Energy-Mod](https://github.com/nathanpitman/Open-TTD-Renewable-Energy-Mod). Inspected revision: `09da329c29e78eeb1f26aab6b8e6492e7c06daf7`, dated 2026-09-27; its `VERSION` file and source identify version 14. Use this revision for the feasibility fixture, then deliberately recheck upstream before any final dependency pin.

| Verified observation | Planning consequence |
| --- | --- |
| ETI supplies seven new industries/two cargos and a companion Game Script; its current README warns of likely FIRS conflicts. | It is a whole economy integration, not just a solar sprite pack. Test a declared content combination. [Pinned README](https://github.com/nathanpitman/Open-TTD-Renewable-Energy-Mod/blob/09da329c29e78eeb1f26aab6b8e6492e7c06daf7/README.md) |
| NML identifies GRFID `RE01`, solar local industry ID 5 and `POWR` output at base rate 6; `POWR` and `URAN` use slots/bits 32 and 33. Solar layout is 3x2, flat-location checked, default year 1990. The worker mechanic is removed. | Match supported identity/version, reserve cargo compatibility and test actual production. Avoid an adapter based on translated names or the old worker description. [Pinned NML](https://github.com/nathanpitman/Open-TTD-Renewable-Energy-Mod/blob/09da329c29e78eeb1f26aab6b8e6492e7c06daf7/energy_transition.nml) |
| GS finds `POWR` by label, identifies producers/acceptors, reads last-month production near towns and sets growth; its loop sleeps in ticks. | Extra solar production must appear in normal monthly statistics. Review layer filters, time units and sole authority over growth. [Pinned script](https://github.com/nathanpitman/Open-TTD-Renewable-Energy-Mod/blob/09da329c29e78eeb1f26aab6b8e6492e7c06daf7/energy_transition_gs/main.nut) |
| GS metadata is version 14, dated 2026-09-27, requesting API version 13. | Verify startup and API compatibility in this fork; matching metadata is not a runtime pass. [Pinned metadata](https://github.com/nathanpitman/Open-TTD-Renewable-Energy-Mod/blob/09da329c29e78eeb1f26aab6b8e6492e7c06daf7/energy_transition_gs/info.nut) |
| No license file/declaration was found in the inspected repository tree; GitHub's repository API returned no detected license. | Redistribution/modification permission is unresolved. Do not assume the game's GPL covers those files. [Inspected tree](https://github.com/nathanpitman/Open-TTD-Renewable-Energy-Mod/tree/09da329c29e78eeb1f26aab6b8e6492e7c06daf7) |

The author's [initial announcement](https://nathanpitman.com/posts/energy-economy-in-open-transport-tycoon-deluxe/) and a cached repository page describe workers; the pinned current README/source remove that mechanic. The source revision above is the planning baseline. Public source availability and an advertised feature are not proof of license terms, local runtime compatibility or final-quality graphics.

**Recommendation:** supply an original solar/power implementation with the Multilayer Industry profile, and add an explicit ETI compatibility profile if the fixture passes. Bundling an ETI derivative requires established redistribution terms and a deliberate content decision. This preserves the requested solar gameplay regardless of upstream packaging. Upstream permission enquiries are future user-owned work; no author contact is authorized by this plan.

## Public Handoff Contract

Proposed public owner: `src/solar_power.h`. Publish `SolarGenerationAccount`, `MirrorAssignment`, `ComputeMirrorSolarOutput`. These symbols are producer obligations, not existing code. Keep one authority for the fields/commands they expose; downstream plans consume the public contract rather than private pools or copied tables.

`ETI Feasibility Handoff`: the first reading area records the pinned fixture compatibility, distribution status, native/ETI route and label mapping for r2mlvs1 Content Freeze. This checkpoint does not wait for orbital mirror implementation.

`Useful Power Handoff` (delivered 2026-10-08): [public contract and focused acceptance](portal-tbm/r2mlvs3mp-x-useful-power-handoff.md) publish the implemented `src/utility_power.h` exports `PublishUtilityGeneration`, `AllocateUtilityPower`, `GetUtilityAllocation`, `UtilityGrantID`, `UtilityMutationKind`, `PrepareUtilityMutation`, `CommitUtilityMutation`, work/town effects and physical directed-link APIs. Allocation, consume/release/layer-transfer, actual effects, normal loading and saved retry fixtures pass. This is the single runtime utility owner for r2mlvs6/16/17; r2mlvs1 retains the units/schema contract. Solar/mirror generation and actual geothermal/tube consumers remain with their subsequent owners.

## Implementation Tasks

Deepening pass, 2026-10-04: implementation groups use only SIMPLE, MEDIUM or HARD difficulty, with at most eight pending tasks each. Sketches describe proposed implementation contracts; verify the named earlier producers at each assumption gate. No gameplay or asset implementation is marked complete by this planning pass.

Groups are chronological and contain consecutive tasks of one difficulty. Each difficulty batch has at most eight tasks; area reading changes only when the implementation boundary or source of truth changes.

### Area Mandatory Reading - Reproducible ETI feasibility decision

_Applies to Groups 1-6._

**Entry gate.** The following public contracts must exist before dependent implementation in this area: `src/multilayer_content.h`: `MultiLayerGameplayProfile`, `LayerVehicleCapability`, `GetMultiLayerContentProfile`, `CheckDeepLayerAvailability`, `UtilityPowerAccount`.
Verify each public file/export/schema field with `rg` in the current workspace and inspect its command/field ownership. For a same-plan contract, compile its type/signature fixture or validate its artifact after the preceding producer groups. For cross-plan behavior, also run the producer's focused executable boundary test; declarations alone cannot authorize dependent mutations or runtime claims.
If a required export is absent, stop the dependent implementation, mutation and runtime claims in Groups 1-6. Read-only characterization, source/range verification, test-case design and asset-template work that does not assume the missing behavior may continue.
Do not bypass this gate by copying private pool/tile state, inventing local IDs or cargo registries, using a historical plan as a delivered implementation, opening user-wide content stores, substituting a second utility/accounting authority, or adding an unreviewed fallback path. Finish the named producer or revise the shared contract and its direct consumers first.

| File name | Line Ranges | Description |
| --- | --- | --- |
| [src/cargotype.h](../../../../src/cargotype.h) | 211-223 | `SetupCargoForClimate`, `BuildCargoLabelMap`, `GetCargoTypeByLabel` separate stable labels from runtime slots; consumers must resolve the selected profile rather than hard-code cargo indices. |
| [src/industrytype.h](../../../../src/industrytype.h) | 109-152 | `IndustrySpec` owns layouts, climate/availability, recipes and callbacks; layer capabilities must be explicit and not inferred from fallback graphics. |
| [src/industry_type.h](../../../../src/industry_type.h) | 98-103 | `INDUSTRY_NUM_INPUTS` and `INDUSTRY_NUM_OUTPUTS` provide current 16-slot bounds; do not assume historical 3-input/2-output limits. |
| [docs/zz-reach2/upgrades/2026-10/r2mlvs12-solar-farms-mirrors-and-eti.md](r2mlvs12-solar-farms-mirrors-and-eti.md) | 51-65 | `Investigation record` identifies the pinned ETI NML/GS primary sources, known slot/identity mappings and unresolved redistribution status. Verify them in the isolated fixture; the recorded source findings are not runtime compatibility or permission to bundle. |
| `src/multilayer_content.h` (planned public contract; link after implementation) | Live definitions; no fixed range | [r2mlvs1-gameplay-contracts-and-content.md](r2mlvs1-gameplay-contracts-and-content.md) Groups 1-6, ending with Group 6's public-contract task, publish `MultiLayerGameplayProfile`, `LayerVehicleCapability`, `GetMultiLayerContentProfile`, `CheckDeepLayerAvailability`, `UtilityPowerAccount`. These exports/schema fields are authoritative for this area. Same-plan consumers follow those earlier groups; cross-plan consumers verify the producer's required implementation/tests as well. Proposed names alone do not prove delivery. |

**ASSUMPTION: this was already built previously. If not, re-asses at runtime**. This applies only to the producer contracts named in this area's gate, after their listed producer groups have run. Verify their actual exports and focused tests before using the sketches below; proposed types and helper names in sketches are not evidence of delivered code.

**M0 staging.** This area consumes Draft Content and publishes ETI Feasibility. Its isolated load/production tests answer whether the pinned option is usable; they do not require Content Freeze, operational mirrors or a completed native grid. Preserve the investigation record as dated evidence and record any later runtime differences explicitly.

**Execution disposition, 2026-10-04.** The required redistribution/meaning/authority precheck rejects the pinned ETI option. Task 5.1 explicitly permits a completed native-fallback decision without claiming ETI passed. Publish that decision and independent Group 6 draft contracts for M0; retain unexecuted startup/production/GS/save tasks unchecked for a future adapter effort. This staging follows the master's Draft Content -> producer M0 handoffs -> Content Freeze order and does not authorize later runtime work before its gates.

### Group 1 - Prepare the pinned ETI feasibility fixture

{{MEDIUM}}
- [X] 1.1 Reproduce the recorded revision/content metadata in a repository-local fixture and retain hashes. Verify distribution terms for each proposed reused file; absent terms preserve the original-content route without contacting the author.
- [ ] 1.2 Start pinned ETI with its companion GS using isolated content/config paths and record startup/API errors. The draft r2mlvs1 mapping is sufficient for this feasibility run; final Content Freeze is its consumer.
- [ ] 1.3 Verify actual Power/Uranium labels, solar layout/year/base output and selected profile slots 32/33. Check native profile collisions and load order before interpreting production results.

### Group 2 - Measure ETI production and save compatibility

{{HARD}}
- [ ] 2.1 Deliver Uranium to the declared nuclear receiver and trace exact accepted units, resulting Power and monthly production; record any stock/payment mismatch.
- [ ] 2.2 Trace solar effective generation into normal monthly statistics and the GS consumer path, distinguishing simulation/economy/calendar timing from script sleep ticks.
- [ ] 2.3 Save/reload with the pinned content/profile hashes and compare identities and output. Record runtime incompatibility as a failed feasibility result, not as permission to substitute a different cargo meaning.

### Group 3 - Resolve ETI cargo and power authority

{{HARD}}
- [X] 3.1 Resolve duplicate Uranium definitions and required cargo labels by explicit profile mapping; refuse unsupported replacement sets before activating an existing save.
- [ ] 3.2 Trace `POWR` generation, transport/refit/payment and grid credit. Decide the supported nontransported-utility adapter so the same energy cannot earn freight and grid rewards.
- [ ] 3.3 Test a Surface-only generator filter and script timing across economy/calendar modes. Select exactly one town-growth authority and document unsupported simultaneous GS control.
- [X] 3.4 Publish the feasibility result with a native fallback decision when any required mapping/authority/distribution condition fails. Never patch an unrelated industry merely because it emits a matching label.

### Group 4 - Reproducible ETI feasibility decision

{{SIMPLE}}
- [X] 4.1 Record the delivery decision: original native profile, supported separately installed ETI profile, or licensed bundled adaptation.

### Group 5 - Reproducible ETI feasibility decision

{{MEDIUM}}
- [X] 5.1 Freeze compatible revision/hash, scope and failure message. A fallback is a completed decision, not a claim ETI passed.

### Group 6 - Publish draft solar contracts for M0

{{HARD}}
- [X] 6.1 Publish `src/solar_power.h` declarations `SolarGenerationAccount`, `MirrorAssignment`, `ComputeMirrorSolarOutput`, with solar identity, baseline/effective units, fractional remainder and invalid-target policy.
- [X] 6.2 Record that ETI Feasibility Handoff publishes draft signatures and the tested feasibility/native-fallback decision only. Operational grid/mirror implementation follows Content Freeze; declarations must not gate M0 on later Space behavior.
- [X] 6.3 Register any new source and compile a contract fixture. Keep `UtilityPowerAccount` schema owned by r2mlvs1; the next area implements its single runtime accounting authority.

M0 evidence: [pinned investigation/native refusal decision](r2mlvs12-a-eti-feasibility.md), [draft solar header](../../../../src/solar_power.h). `[solar-power-contract]` verifies types/defaults/signature and actual `RE01` candidate refusal; operational augmentation is deliberately still undeveloped. ETI startup/production/save/GS tests above remain unchecked and no adapter/redistribution is approved.

### Area Mandatory Reading - Native useful-power ledger and consumers

_Applies to Groups 7-9, including 7B/7C/8B._

**Entry gate.** r2mlvs1 Content Freeze supplies `MultiLayerGameplayProfile`/`UtilityPowerAccount` units and authority; r2mlvs14 Schema Foundation supplies the versioned state boundary. No rocket, mirror, geothermal plant or completed mine/tube consumer is required.
**ASSUMPTION: this was already built previously. If not, re-asses at runtime**. Verify the named prior public exports and their type/behavior fixtures with the current source before dependent work. A declaration alone is insufficient for runtime mutation. If missing, stop the dependent implementation and passing claims in Groups 7-9; independent characterization, test design and source verification may continue. Do not copy private stores, invent substitute IDs/inventory/energy or treat historical planning prose as delivered code.

| File name | Line Ranges | Description |
| --- | --- | --- |
| `src/multilayer_content.h` (planned public contract; link after implementation) | Live definitions; no fixed range | [r2mlvs1 Content Freeze](r2mlvs1-gameplay-contracts-and-content.md#public-handoff-contract) freezes `MultiLayerGameplayProfile`, `UtilityPowerAccount`, source/consumer units and numerical policy. Verify the frozen artifact/type fixture, not an unfinished draft. |
| `src/multilayer_state.h` (planned public contract; link after implementation) | Live definitions; no fixed range | [r2mlvs14 Schema Foundation](r2mlvs14-savegame-determinism-and-lifecycle.md#public-handoff-contract) provides `MultiLayerStateSchema` and the reference-validation extension point; this area owns concrete utility descriptors/tests. |
| [src/saveload/saveload.h](../../../../src/saveload/saveload.h) | 447-518 | Complete chunk-handler/save/fixup/preview contract supplies the concrete persistence boundary for utility epochs/grants/mutation sequences. |
| [src/town_cmd.cpp](../../../../src/town_cmd.cpp) | 3270-3305 | Complete `CmdTownGrowthRate` sets custom growth authority; native power effects and ETI GS cannot both control the same town. |
| [src/town_cmd.cpp](../../../../src/town_cmd.cpp) | 3941-3952 | Complete `UpdateTownGrowthRate` skips custom-growth towns; define authority and restoration before adding native effects. |
| [src/timer/timer_game_common.h](../../../../src/timer/timer_game_common.h) | 99-123 | Trigger/priority contract requires explicit ordering for state-dependent production, allocation and history; do not rely on registration order. |

**Same-area consumers. ASSUMPTION: this was already built previously. If not, re-asses at runtime**. Group 7 produces the ledger/query exports in `src/utility_power.h`; Group 8 adds grant/mutation types and prepare/commit operations; Group 9 consumes both. Verify the relevant preceding group's fixtures before continuing after compaction. Stop only the dependent part if missing; do not create a second account owner.

```text
source publication -> one interval ledger -> bounded grants
prepared consume / release / layer-transfer -> one committed mutation sequence
consumer effects -> same metered quantity, one selected town authority
```

### Group 7 - Implement the native useful-power ledger

{{HARD}}
- [X] 7.1 After r2mlvs1 Content Freeze, implement planned `src/utility_power.h` exports `PublishUtilityGeneration`, `AllocateUtilityPower` and `GetUtilityAllocation` using the canonical `UtilityPowerAccount` units and IDs.

### Local Mandatory Reading - Utility source epochs and saved authority

_Applies to Group7B; continues original Group7's source/save tasks under master9.1._

Read Global and Native useful-power Area, the canonical `UtilityPowerAccount` units and current `utility_power.h/.cpp` source/consumer/slice layout and validation. Read only actual chunk registration, utility descriptors, metadata feature bit/version, preview transport and post-company load acceptance; use `tbm_sl.cpp` for the concrete table/header pattern. Epoch/source-loss tests pass601/9 and staging tests514/6; neither is an actual-file round trip. Keep original source lineage/carry and strict scalar/count/owner checks, preserve older empty Classic/native files and validate before publishing. Do not preload consumer effects, solar, mirrors or geothermal code.

Context has compacted 1 times while executing this group.

### Group 7B - Persist utility source epochs and grants

{{HARD}}
- [X] 7.2 Accept generation once per source/accounting interval with checked quantities and saved remainders. Define zero-supply, duplicate publication and source removal behavior; a Surface export is not a second generator.
- [X] 7.3 Allocate energy to explicit linked eligible consumers in stable order, bounded by supply/demand and capacity. Save authoritative interval/grants and rebuild only derived connectivity caches.

7B closure (2026-10-07): version381 `UPLH`/`UPLD` and metadata bit1024 preserve exact source carry/publication, stable company/layer IDs, explicit links, original reservations and released source-loss slices. Full loading validates after real company/profile/map fixups; independent preview rejects malformed counts/owners/fractions/balances/missing tables without changing the accepted ledger. UtilityRoot adds a field-wise diagnostic digest. [Owner checks](../../../../logs/r2mlvs3mp-u-power-save-owner-checks-1.log) pass555 assertions/eight cases. [Ten actual native file cases](../../../../logs/r2mlvs3mp-u-power-native-file-checks-1.log) prove published/granted/source-loss phases through two byte-exact reloads, five corruption refusals and valid preview retry, plus saved carry27mJ to54mJ and next-epoch surviving-source continuation. The fixture exposed and corrected omitted consumers retaining stale epochs; allocation now publishes current zero baselines before explicit requests. Supplied paid-world input and startup bytes remain unchanged. Actual settlement/transfer/effects remain with Groups8/9.

### Local Mandatory Reading - Utility reservation policy and owner acceptance

_Applies to Group7C after7B's actual source/save checks._

Read Global/Native useful-power Area and only the delivered utility API, owner/epoch/slice fields, exact file transport and source/consumer proof. Define explicit publication/allocation/settlement/rotation order; actual mutation operations belong to the following Group8 and consumer effects to Group9. Do not count fields or a staging-copy fixture as saved or consumed energy. Publish full Useful Power Handoff only after the stated mutation/effect boundaries pass.

### Group 7C - Define reservation policy and complete ledger fixtures

{{HARD}}
- [X] 7.4 Define reservation, actual-use settlement and unused-grant release so failed recipes cannot consume energy twice. Make `GetUtilityAllocation` read-only; repeated queries cannot renew a grant. Order generation, allocation, consumption and month rotation explicitly instead of relying on timer registration order.
- [X] 7.5 Test zero, oversubscribed and multiple-source/consumer ledger fixtures plus save/retry. Record ledger readiness here; publish the full `Useful Power Handoff` only after the following consumer-effect fixtures pass. Neither stage requires rockets, mirrors or completed geothermal plants.

7C closure (2026-10-07): [reservation/settlement contract](portal-tbm/r2mlvs3mp-v-utility-settlement-contract.md) specifies publication, one global allocation, revalidated settlement/effect, then explicit monthly rotation; current/free/source-loss conservation and persisted retry identity are separated from actual use. Delivered read-only/zero/fraction/shortage/multiple-owner/source/save boundaries are the focused7B evidence above. This closes ledger readiness and policy, with actual typed atomic mutation/link/sequence implementation in Group8 and observable effects in Group9. Full Useful Power Handoff and master9.1 stay pending.

7.1 API checkpoint (2026-10-07, master9.1): `src/utility_power.h/.cpp` now owns native company/layer accounts and monotonically allocated producer/consumer IDs, once-only checked rate publication with the frozen millijoule carry, explicit bounded same-owner/same-layer consumer links, and the frozen deterministic allocation proposal. One synchronized `OWNER_DEITY` allocation serves all companies together; a company cannot close the world's interval ahead of peers. Promised source energy is reserved, never called settled consumption. `GetUtilityAllocation` only reads current owned grants, and metadata/world reset clears the prior ledger. The [upstream contract boundary](../../../../logs/r2mlvs3mp-u-power-producer-contracts.log) passes 180 assertions / eight cases. [New API and affected utility/schema checks](../../../../logs/r2mlvs3mp-u-power-api-checks.log) pass 263 assertions / eight cases, including zero/fractional publication, estimate/refusal purity, duplicate/stale input, stable shortage allocation, multiple companies and repeated queries. This checkpoint does not deliver epoch rotation/source removal, saved grants, atomic settlement or consumer effects; 7.2-7.5 and master9.1 remain pending.

### Local Mandatory Reading - Atomic utility transactions

_Applies to Group8 after7B/7C._

Read Global/Native useful-power Area, the delivered `utility_power.h/.cpp` grant slices, version381 descriptors/digest and [settlement contract](portal-tbm/r2mlvs3mp-v-utility-settlement-contract.md). Read `LayerRecipeCommitParticipant` and `CommitLayerRecipeBatch` only to identify the existing actual complete-transaction boundary; do not copy inventory state. For explicit link eligibility, inspect actual Connected/date-valid portal queries and canonical UtilityLink ownership in Content Freeze. Physical riser construction remains Plan17; absent later buildings do not authorize a new generator or implicit cross-layer credit. Extend saved records/version/preview and run mutation-owner plus actual-file fixtures before claiming retry or transfer delivered.

### Group 8 - Expose atomic utility consumption, release and transfer

{{HARD}}
- [X] 8.1 Add public `UtilityGrantID`, `UtilityMutationKind`, `PrepareUtilityMutation` and `CommitUtilityMutation` in `src/utility_power.h`. Typed mutation kinds cover Consume, ReleaseUnused and TransferLayer; proposals carry grant/epoch, exact quantity, owner and explicit destination when needed.
- [X] 8.2 Prepare against saved grant balance/source lineage without mutation; commit through a revalidated transaction participant shared with recipe/movement jobs. Use a saved operation sequence/identity so retry cannot settle or export the same energy twice.

8.1 API/consume-release checkpoint (2026-10-07): exact consumer/epoch `UtilityGrantID`, typed kinds, pure sealed preparation, whole actual participant revalidation and saved operation-sequence receipts are implemented. Every accepted ledger change invalidates old tickets; estimates/queries do not. Generation/settlement lifetime totals survive rotation and retirement. Version382 extends the bounded transport while explicitly reading version381. [Focused mutation/ledger owners](../../../../logs/r2mlvs3mp-v-utility-mutation-checks.log) pass589 assertions/eight cases; [publication file checks](../../../../logs/r2mlvs3mp-v-utility-publication-file-checks.log) pass ten native cases; [settlement/migration file checks](../../../../logs/r2mlvs3mp-v-utility-settlement-file-checks.log) pass six native cases, including byte-exact consumption/release and original issued-ticket refusal after load. TransferLayer is typed but refuses until8.3 supplies actual commissioned link authority. The shared recipe adapter and its focused fixture have been added but are not yet compiled/verified;8.2-8.4 and the full handoff remain pending.

8.2 shared transaction closure (2026-10-07): `LayerRecipeCommitParticipant::GetElectricalGrant` defaults to refusal; an actual token routes the complete existing material/output/work participant through `CommitUtilityMutation`. Inventory and ledger layers must agree, actual preparation/commit revalidates all owners, and thermal recipes still require their separate vent producer. [Shared recipe/mutation checks](../../../../logs/r2mlvs3mp-v-utility-recipe-checks.log) pass366 assertions/nine cases, including validation/commit failure, cross-layer refusal, exactly20J settled with two complete batches, stale recipe retry and forged historical-use refusal. Native scalar-only/bootstrap boundaries remain intact. The saved sequence and issued-ticket evidence above close settlement retry; actual link transfer and its file/loss/re-export cases remain8.3/8.4.

### Local Mandatory Reading - Directed utility flow and topology loss

_Applies to Group8B; continues original8.3/8.4 after the verified8.1/8.2 transaction producer._

Read Global/Native useful-power Area and only current `UtilityPowerShare`/`UtilityPowerLink`, free-slice/transfer/retirement/validation paths in `utility_power.cpp`, utility383 wire descriptors and actual `ChangeLayerPortalConnection`/`RetireLayerPortalWorkPair` hooks. Existing recipe/consume/release proof is already delivered; do not preload it again unless those callbacks change. Run `utility_power_transfer_load.cpp` / `utility_power_save_fixture-2.py` against the unchanged paid Deep4 native input and its actual owned Connected Surface/Underground pair; no new room, copied grid or second generator proves a physical route. Preserve retained381/382 saves and verify older loading alongside new import/source/flow/loss/rebuild files. Current transfer code is drafted but not compiled/verified yet; keep8.3/8.4 pending.

### Group 8B - Commission and persist directed utility flows

{{HARD}}
- [X] 8.3 Define unused-grant expiry and topology-loss release without refunding already consumed units. A layer transfer debits its source and credits destination availability while retaining source lineage, never invokes a second source-generation publication.
- [X] 8.4 Test insufficient/stale grants, duplicate commit, failed recipe participant and transfer/release around save/reload. Publish these mutation fixtures with the full Useful Power Handoff after consumer effects pass; downstream owners must not edit account fields privately.

8B closure (2026-10-08): directed ledger-issued links require real same-owner/date-valid Connected portal endpoints and exact commissioning revision. Transfers debit only original local slices, credit existing requested destination room within actual route capacity, retain original generator IDs and cannot re-export imports this interval. Sealing/demolition retires routes and releases only unused imports; consumed units survive loss/rotation and reconnection requires new link IDs. Version383 stores routes, flow and imported/transferred slices with381/382 readers. [Owner/recipe checks](../../../../logs/r2mlvs3mp-v-utility-transfer-owner-checks.log) pass645 assertions/nine cases; [five native physical transfer cases](../../../../logs/r2mlvs3mp-v-utility-transfer-file-checks.log) prove40J conserved from81J,10J actual original-source use,30J loss expiry without donor refund, new linkID2/rebuilt5J and byte-exact four-phase reloads. [Publication/current transport](../../../../logs/r2mlvs3mp-v-utility-transfer-publication-checks.log), [settlement/381 migration](../../../../logs/r2mlvs3mp-v-utility-transfer-settlement-checks.log) and [five retained382 loads](../../../../logs/r2mlvs3mp-v-utility-transfer-legacy382-checks.log) pass in separate bounded scripts. The [rebuilt normal executable](../../../../logs/r2mlvs3mp-v-utility-normal-build.log) and [ordinary loader/writer](../../../../logs/r2mlvs3mp-v-utility-normal-loader-checks.log) preserve exact current utility chunks with no fixture hook. Original paid Deep4/Underground5 inputs, retained old utility inputs and startup bytes remain unchanged. Physical riser construction/payment and complete tube/mine consumers remain their owning plans; Group9 effects and full Useful Power Handoff/master9.1 remain pending.

### Local Mandatory Reading - Bounded consumer effects and town authority

_Applies to Group9 after verified7/8/8B._

Read Global/Native useful-power Area, `GetUtilityConsumerEffects`/`ComputeNativeTownPowerBonus`, actual `GetMultiLayerContentProfile().town_power_authority`, delivered current grant/whole-participant consumption boundary and saved consumer/source accounting. Read actual Town growth/custom-GS fields, `CmdTownGrowthRate` and `UpdateTownGrowthRate` before changing town behavior. Full native mine/tube implementations and solar/mirror authoring remain later; small real-owner consumer/effect fixtures must exhaust this exact ledger quantity and produce an observable bounded effect. Do not reread resolved link conversion/packing or ETI source investigations. A supported ETI adapter is absent; retain native authority and explicit custom-growth refusal/restoration policy.

Context has compacted 1 times while executing this group.

### Group 9 - Implement consumer effects and exclusive town authority

{{HARD}}
- [X] 9.1 Apply the r2mlvs1 bounded native processor/tube service grants and Surface town-growth benefit through the canonical consumer boundary. Validate grants with small inventory/consumer fixtures; full mine/tube consumers are later integrations.
- [X] 9.2 Select native versus supported ETI town authority explicitly. Respect custom GS growth ownership and prevent native recalculation and GS commands from setting the same effect.
- [X] 9.3 Test accepted energy changes one observable consumer effect and exhausts the same ledger quantity, while unsupported/absent consumers receive no phantom benefit.
- [X] 9.4 Publish effect limits, update interval and public allocation-query fixture for r2mlvs6/16/17; keep missing final sprites pending under r2mlvs13.

9 closure (2026-10-08): [Useful Power Handoff](portal-tbm/r2mlvs3mp-x-useful-power-handoff.md) records the single ledger, exact transaction/effect APIs, frozen numerical caps, explicit synchronized interval ordering, exclusive native town authority and version384 transport. [829 assertions / 14 focused cases](../../../../logs/r2mlvs3mp-w-utility-effects-final-checks.log) prove whole-owner refusal, partial supply, zero-benefit preservation, actual work/real-town changes and matching original-source consumption, GS takeover before/after use and town ID reuse. [Five real town file cases](../../../../logs/r2mlvs3mp-w-utility-town-file-checks.log) and [normal executable loading/resaving](../../../../logs/r2mlvs3mp-w-utility-town-normal-checks.log) preserve exact utility/CITY state. [Current-format transport](../../../../logs/r2mlvs3mp-w-utility-current-transport-checks.log) passes ten native cases, including five corruption refusals, valid preview retry and saved carry/reservation continuation; [retained381/382/383 inputs](../../../../logs/r2mlvs3mp-w-utility-legacy-format-checks.log) pass current-reader grant/settlement-retry/physical-flow acceptance unchanged. Master9.1 closes. Actual solar/mirror generation, geothermal heat/construction, complete mine/tube consumers and their final art remain later tasks.

### Area Mandatory Reading - Solar generation and source accounting

_Applies to Groups 10-14._

**Entry gate.** Group 6 supplies draft `SolarGenerationAccount`, `MirrorAssignment`, `ComputeMirrorSolarOutput` signatures; Groups 7-9 supply tested `src/utility_power.h` runtime ledger and mutation APIs. This area implements the solar source hook; actual orbital mirror deployment follows later.
**ASSUMPTION: this was already built previously. If not, re-asses at runtime**. Verify the named prior public exports and their type/behavior fixtures with the current source before dependent work. A declaration alone is insufficient for runtime mutation. If missing, stop the dependent implementation and passing claims in Groups 10-14; independent characterization, test design and source verification may continue. Do not copy private stores, invent substitute IDs/inventory/energy or treat historical planning prose as delivered code.

| File name | Line Ranges | Description |
| --- | --- | --- |
| [src/industry.h](../../../../src/industry.h) | 75-106 | `ProducedCargo`, `AcceptedCargo`, `ProducedCargoes`, `AcceptedCargoes` expose bounded inventory and economy-time history; conserve materials and avoid narrow-counter overflow. |
| [src/industry_cmd.cpp](../../../../src/industry_cmd.cpp) | 1152-1240 | `ProduceIndustryGoodsHelper` and `ProduceIndustryGoods` expose complete rate/callback production paths; augment output once before authoritative monthly accounting. |
| [src/industry_cmd.cpp](../../../../src/industry_cmd.cpp) | 1242-1264 | `OnTick_Industry` owns production scheduling and editor exclusion; visible animations never produce power or minerals. |
| `src/solar_power.h` (planned public contract; link after implementation) | Live definitions; no fixed range | [r2mlvs12-solar-farms-mirrors-and-eti.md](r2mlvs12-solar-farms-mirrors-and-eti.md) Groups 1-6, ending with Group 6's public-contract task, publish `SolarGenerationAccount`, `MirrorAssignment`, `ComputeMirrorSolarOutput`. These exports/schema fields are authoritative for this area. Same-plan consumers follow those earlier groups; cross-plan consumers verify the producer's required implementation/tests as well. Proposed names alone do not prove delivery. |
| `src/geothermal.h` (planned public contract; link after implementation) | Live definitions; no fixed range | [r2mlvs17-geothermal-energy-and-industry.md](r2mlvs17-geothermal-energy-and-industry.md) Groups 1-7, ending with Group 7's public-contract task, publish `GeothermalHeatAllocation`, `GeothermalUtilityLink`, `AllocateVentHeat`, `ExportGeothermalPower`. These exports/schema fields are authoritative for this area. Same-plan consumers follow those earlier groups; cross-plan consumers verify the producer's required implementation/tests as well. Proposed names alone do not prove delivery. Read this producer for the geothermal export adapter integration in Group 14 (only the named integration/subscenario); missing it blocks only that integration, not independent work. |
| [src/industry_cmd.cpp](../../../../src/industry_cmd.cpp) | 525-554 | Complete `TransportIndustryGoods` records production during freight dispatch; exclude selected utility output from this second history/payment path. |
| [src/newgrf_industries.cpp](../../../../src/newgrf_industries.cpp) | 600-678 | Complete `IndustryProductionCallback` mutates/clamps waiting output; capture eligible baseline before clamping rather than infer it from stock differences. |
| [src/saveload/industry_sl.cpp](../../../../src/saveload/industry_sl.cpp) | 87-119 | Complete `SlIndustryProducedHistory` saves narrow history values; widening arithmetic requires a compatible storage/descriptor decision. |
| [src/script/api/script_industry.cpp](../../../../src/script/api/script_industry.cpp) | 94-105 | Complete `GetLastMonthProduction` reads industry history; supported ETI effects must see the same effective output as the grid. |
| `src/utility_power.h` (planned public contract; link after implementation) | Live definitions; no fixed range | Earlier Groups 7-9 produce `PublishUtilityGeneration`, `AllocateUtilityPower`, `GetUtilityAllocation`, `UtilityGrantID`, `UtilityMutationKind`, `PrepareUtilityMutation`, `CommitUtilityMutation` with effect/save/retry fixtures. Solar publishes source energy; exports use the transfer mutation, never duplicate generation. |

**Scoped geothermal gate. ASSUMPTION: this was already built previously. If not, re-asses at runtime**. Group 14 alone requires operational r2mlvs17 generation/export and its boundary tests. If absent, keep that adapter pending and continue native solar arithmetic; do not invent thermal supply. Final solar art requires the corresponding r2mlvs13 manifest and delivered assets; verify them before art acceptance and leave only that integration pending if unavailable.

Use a fixed accounting denominator `D` for saved fractional power. Within each constant-rate sample, accumulate `3 * baseline_rate_numerator * covered_ticks`, emit whole units by division by `D`, and retain modulo `D`. Split samples at eligibility/baseline changes without changing `D`; save/version the scale and prove intermediate bounds. Never multiply a historical monthly total.

### Group 10 - Supply solar content and eligible generation identity

{{MEDIUM}}
- [ ] 10.1 Register the original Surface solar farm/native fallback with profile recipe/cost/availability and a useful-power connection. For a supported ETI profile, bind its tested generation adapter to the same runtime ledger.
- [ ] 10.2 Mark mirror eligibility by native capability or pinned GRFID/local industry type/version mapping. Names, fallback graphics and the `POWR` label alone cannot identify solar generation.
- [ ] 10.3 Verify one unboosted solar interval and selected nuclear/Coal source integration produce their declared grid effect. Material-only mine/processor work may precede this handoff; power acceptance cannot.
- [ ] 10.4 Integrate final original solar sprites when the matching r2mlvs13 assets are delivered; standalone ledger tests do not claim visual acceptance.

### Group 11 - Install one source-side solar augmentation hook

{{HARD}}
- [ ] 11.1 Trace rate and callback production to a common authoritative baseline sample after their availability modifiers. Apply the mirror modifier once before produced/history/grid publication, with an explicit marker/flow preventing a callback and helper from both boosting it.
- [ ] 11.2 Capture selected callback generation before its 16-bit waiting clamp, not from a post-callback stock delta. Commit effective utility generation and monthly history once, and suppress legacy `TransportIndustryGoods` freight/history handling for that owned utility output.
- [ ] 11.3 Implement pure `ComputeMirrorSolarOutput` for zero or one operational assignment, capped at baseline plus three times baseline. Preserve consumed inputs and every non-power cargo.
- [ ] 11.4 Audit rate, output and monthly-history widths plus grid units. Use checked wide intermediates and a supported storage bound/representation; a 4x result must not wrap or be silently clamped away.
- [ ] 11.5 Match any wider production history to its save descriptors and script-visible values; `GetLastMonthProduction` reads the industry history, so a private utility ledger alone is insufficient for the supported ETI path.
- [ ] 11.6 Test native and selected ETI source paths independently with constant baseline 100 -> 400 and zero -> zero before binding real orbital mirrors.

### Group 12 - Preserve partial-period production and history

{{HARD}}
- [ ] 12.1 Define the coverage/accounting interval and saved remainder so variable baseline production is sampled when it actually occurs. Do not multiply last-month totals after a mirror turns on.
- [ ] 12.2 Accumulate bonus from eligible baseline and operational coverage within each interval, then publish the same effective amount to production statistics and grid exactly once.
- [ ] 12.3 Test half-period coverage, service failure halfway through, repeated small fractions and save/reload with a nonzero remainder. Two operational mirrors assigned to one farm still cap at one bonus.

### Group 13 - Solar industry and useful power

{{SIMPLE}}
- [ ] 13.1 Record original/native and ETI compatibility statistics consistently, including partial operational periods. A stopped/depleted/zero-baseline facility gets no bonus.

### Group 14 - Adapt later geothermal export to the same grid

{{HARD}}
- [ ] 14.1 When r2mlvs17 is delivered, consume its explicit metered electrical export at a commissioned portal and publish it once to the existing useful-power ledger.
- [ ] 14.2 Identify original geothermal source/export lineage so neither the Deep generator nor its Surface riser can be counted twice by native or ETI scans. Exclude all non-solar sources from mirror eligibility.
- [ ] 14.3 Compare a mixed solar/geothermal interval against separate ledgers; heat allocation remains with r2mlvs17. Missing geothermal behavior blocks only this adapter and combined scenario.

### Area Mandatory Reading - Orbital mirror lifecycle

_Applies to Groups 15-17._

**Entry gate.** The following public contracts must exist before dependent implementation in this area: `src/solar_power.h`: `SolarGenerationAccount`, `MirrorAssignment`, `ComputeMirrorSolarOutput`; `src/space.h`: `SpaceVehicle`, `OrbitalBodyID`, `SpaceMission`, `SpaceStructure`, `SpaceTransition`; `src/launch_center.h`: `LaunchCenterAnchor`, `LaunchCollectionHub`, `SurfaceHandlingJob`, `SpaceDescentJob`, `CanReleaseOrbitalCargo`, `CommitOrbitalCargoRelease`.
Verify each public file/export/schema field with `rg` in the current workspace and inspect its command/field ownership. For a same-plan contract, compile its type/signature fixture or validate its artifact after the preceding producer groups. For cross-plan behavior, also run the producer's focused executable boundary test; declarations alone cannot authorize dependent mutations or runtime claims.
If a required export is absent, stop the dependent implementation, mutation and runtime claims in Groups 15-17. Read-only characterization, source/range verification, test-case design and asset-template work that does not assume the missing behavior may continue.
Do not bypass this gate by copying private pool/tile state, inventing local IDs or cargo registries, using a historical plan as a delivered implementation, opening user-wide content stores, substituting a second utility/accounting authority, or adding an unreviewed fallback path. Finish the named producer or revise the shared contract and its direct consumers first.

| File name | Line Ranges | Description |
| --- | --- | --- |
| [src/command.cpp](../../../../src/command.cpp) | 301-328 | `CommandHelperBase::InternalExecuteValidateTestAndPrepExec` defines refusal/estimate before execution; no validation phase may consume inventory, RNG or funds. |
| [src/tests/map_layer.cpp](../../../../src/tests/map_layer.cpp) | 162-183 | Complete `Plane distances ignore the layer` test fixes projected geometry but grants no cross-layer transfer authority. |
| [src/cargopacket.h](../../../../src/cargopacket.h) | 458-489 | `VehicleCargoList::Append`, `AgeCargo`, `Stage`, `Return`, `Unload`, `Shift` move accountable packets; exchange/descent/processing must not clone packet age or feeder shares. |
| `src/solar_power.h` (planned public contract; link after implementation) | Live definitions; no fixed range | [r2mlvs12-solar-farms-mirrors-and-eti.md](r2mlvs12-solar-farms-mirrors-and-eti.md) Groups 1-6, ending with Group 6's public-contract task, publish `SolarGenerationAccount`, `MirrorAssignment`, `ComputeMirrorSolarOutput`. These exports/schema fields are authoritative for this area. Same-plan consumers follow those earlier groups; cross-plan consumers verify the producer's required implementation/tests as well. Proposed names alone do not prove delivery. |
| `src/space.h` (planned public contract; link after implementation) | Live definitions; no fixed range | [r2mlvs8-space-world-and-construction.md](r2mlvs8-space-world-and-construction.md) Groups 1-7, ending with Group 7's public-contract task, publish `SpaceVehicle`, `OrbitalBodyID`, `SpaceMission`, `SpaceStructure`, `SpaceTransition`. These exports/schema fields are authoritative for this area. Same-plan consumers follow those earlier groups; cross-plan consumers verify the producer's required implementation/tests as well. Proposed names alone do not prove delivery. |
| `src/launch_center.h` (planned public contract; link after implementation) | Live definitions; no fixed range | [r2mlvs9-launch-centers-and-rockets.md](r2mlvs9-launch-centers-and-rockets.md) Groups 1-7, ending with Group 7's public-contract task, publish `LaunchCenterAnchor`, `LaunchCollectionHub`, `SurfaceHandlingJob`, `SpaceDescentJob`, `CanReleaseOrbitalCargo`, `CommitOrbitalCargoRelease`. These exports/schema fields are authoritative for this area. Same-plan consumers follow those earlier groups; cross-plan consumers verify the producer's required implementation/tests as well. Proposed names alone do not prove delivery. |
| [Sprite Contract and delivery owner](r2mlvs13-graphics-and-asset-pipeline.md#public-handoff-contract) | Live definitions; no fixed range | `Sprite Contract Handoff` defines manifest fields `asset_id`, `consumer`, `directions`, `frames`, `anchor`, `source`; the matching authoring/conversion/runtime groups deliver actual assets. Verify the requested family and packed loading fixture before graphics integration; template/manifest publication alone is not final art. |

**ASSUMPTION: this was already built previously. If not, re-asses at runtime**. This applies only to the producer contracts named in this area's gate, after their listed producer groups have run. Verify their actual exports and focused tests before using the sketches below; proposed types and helper names in sketches are not evidence of delivered code.

**Mirror dependency boundary.** Only this orbital area needs tested Space/rocket deployment behavior. Earlier native power allocation and pure solar arithmetic remain usable before those producers. **ASSUMPTION: this was already built previously. If not, re-asses at runtime**. The earlier source hook and useful-power handoff must pass their fixture before actual mirror assignment changes output; the r2mlvs9 payload producer must pass its kit-deployment boundary before kits are consumed.

**Scoped graphics gate. ASSUMPTION: this was already built previously. If not, re-asses at runtime**. Final graphics tasks in this area require the corresponding r2mlvs13 manifest entries, delivered sprites and packed loading fixture. Verify them before binding/review; if absent, stop only those art tasks and their acceptance claims, continue independent code/state tests, and never count placeholders as finished assets.

### Group 15 - Deploy a mirror from delivered kits

{{HARD}}
- [ ] 15.1 Consume a rocket-delivered Solar Mirror Kit through r2mlvs9 payload ownership and r2mlvs8 Space Construction/transition APIs; no first station is required.
- [ ] 15.2 Persist deployment progress, owner, kit consumption and service inventory/cost. A completed deployment activates one `MirrorAssignment` candidate, never a second cargo kit.
- [ ] 15.3 Test cancelled/failed deployment and save around completion before implementing target assignment; preserve unconsumed kits and record consumed parts once.

### Group 16 - Assign an operational mirror to one eligible farm

{{HARD}}
- [ ] 16.1 Define saved plane coverage range and assignment command over stable farm/mirror references. Validate owner policy, compatible solar identity, range and one-assignment capacity before charging.
- [ ] 16.2 Maintain forward/back references and invalidate them before farm/mirror deletion or ID reuse. Public farms remain public, but an occupied mirror assignment cannot be stolen by another company.
- [ ] 16.3 On service loss, invalid target, sale or bankruptcy, stop eligibility at the next declared simulation boundary and release only owned reservations. Preserve fractional accounting under the shared hook.
- [ ] 16.4 Test two mirrors/companies contending for a farm, a replaced farm at the same pool index and range/maintenance failures; no geometry-only beam grants power.

### Group 17 - Expose mirror and power state

{{MEDIUM}}
- [ ] 17.1 Display baseline, bonus and effective generation plus assignment range/service reason from authoritative records; show the same effective quantity the grid receives.
- [ ] 17.2 Integrate deployed/aiming/idle mirror and boosted solar graphics from r2mlvs13. Optical beams read state and never trigger generation.

### Area Mandatory Reading - Acceptance

_Applies to Groups 18-21._

**Entry gate.** The following public contracts must exist before dependent implementation in this area: `src/solar_power.h`: `SolarGenerationAccount`, `MirrorAssignment`, `ComputeMirrorSolarOutput`.
Verify each public file/export/schema field with `rg` in the current workspace and inspect its command/field ownership. For a same-plan contract, compile its type/signature fixture or validate its artifact after the preceding producer groups. For cross-plan behavior, also run the producer's focused executable boundary test; declarations alone cannot authorize dependent mutations or runtime claims.
If a required export is absent, stop the dependent implementation, mutation and runtime claims in Groups 18-21. Read-only characterization, source/range verification, test-case design and asset-template work that does not assume the missing behavior may continue.
Do not bypass this gate by copying private pool/tile state, inventing local IDs or cargo registries, using a historical plan as a delivered implementation, opening user-wide content stores, substituting a second utility/accounting authority, or adding an unreviewed fallback path. Finish the named producer or revise the shared contract and its direct consumers first.

**Scoped integration gates.** The following producer dependencies apply only to the named implementation/test families. Verify each required public export and its focused producer test before those integrations; absent late producers do not block already independent features.

- `src/geothermal.h` is required for the mixed solar/geothermal accounting subscenario; standalone solar/mirror acceptance may proceed in Group 20 (only the named integration/subscenario). If absent, stop only that integration/subscenario and its passing claim. Other independent code, art, persistence and tests in this area may continue.

Do not emulate an absent producer with copied private state, fake inventories/energy, local IDs or a second registry/grid. A final combined closure/gallery/round-trip still waits for every required family; an independent passing case is not a pass for the missing integration.

| File name | Line Ranges | Description |
| --- | --- | --- |
| [src/network/network_command.cpp](../../../../src/network/network_command.cpp) | 256-288 | `NetworkExecuteLocalCommandQueue` executes agreed frame order/company authority; all jobs use deterministic simulation time. |
| [src/network/network_server.cpp](../../../../src/network/network_server.cpp) | 670-687 | `ServerNetworkGameSocketHandler::SendSync` compares RNG seeds, not a full gameplay checksum; validate orbital/geology/cargo ledgers in addition to a no-desync result. |
| `src/solar_power.h` (planned public contract; link after implementation) | Live definitions; no fixed range | [r2mlvs12-solar-farms-mirrors-and-eti.md](r2mlvs12-solar-farms-mirrors-and-eti.md) Groups 1-6, ending with Group 6's public-contract task, publish `SolarGenerationAccount`, `MirrorAssignment`, `ComputeMirrorSolarOutput`. These exports/schema fields are authoritative for this area. Same-plan consumers follow those earlier groups; cross-plan consumers verify the producer's required implementation/tests as well. Proposed names alone do not prove delivery. |
| `src/geothermal.h` (planned public contract; link after implementation) | Live definitions; no fixed range | [r2mlvs17-geothermal-energy-and-industry.md](r2mlvs17-geothermal-energy-and-industry.md) Groups 1-7, ending with Group 7's public-contract task, publish `GeothermalHeatAllocation`, `GeothermalUtilityLink`, `AllocateVentHeat`, `ExportGeothermalPower`. These exports/schema fields are authoritative for this area. Same-plan consumers follow those earlier groups; cross-plan consumers verify the producer's required implementation/tests as well. Proposed names alone do not prove delivery. Read this producer for the mixed solar/geothermal accounting subscenario; standalone solar/mirror acceptance may proceed in Group 20 (only the named integration/subscenario); missing it blocks only that integration, not independent work. |

**ASSUMPTION: this was already built previously. If not, re-asses at runtime**. This applies only to the producer contracts named in this area's gate, after their listed producer groups have run. Verify their actual exports and focused tests before using the sketches below; proposed types and helper names in sketches are not evidence of delivered code.

### Group 18 - Acceptance

{{MEDIUM}}
- [ ] 18.1 Compare identical no-mirror/mirror fixtures at the same base production and duration: 100 normal units become 400 total; two mirrors still yield at most 400; zero remains zero.
- [ ] 18.2 Test partial coverage without rounding drift.
- [ ] 18.3 Verify effective power reaches actual consumer/grid behavior and the ETI monthly statistic path when that profile is enabled. Town-growth changes occur only under the selected authority.

### Group 19 - Validate mirror lifecycle and saved accounting

{{HARD}}
- [ ] 19.1 Test kit use, out-of-range/unsupported targets, deletion/reused IDs, sale/bankruptcy and blocked service with one expected ownership transition per case.
- [ ] 19.2 Round-trip deployment and fractional production remainder; compare next interval output, assignment backlinks and native/ETI authority selection.
- [ ] 19.3 Run the actual Surface kit -> rocket -> mirror -> 4x solar consumer chain after its feature/art producers exist, then deterministic replay. Pure output fixtures from earlier groups are not this end-to-end acceptance.

### Group 20 - Acceptance

{{MEDIUM}}
- [ ] 20.1 Test solar and exported geothermal production together: solar alone receives the +300% mirror bonus, thermal allocations remain unchanged, and native/ETI modes cannot credit both a Deep generator and its Surface export as separate supplies.

### Group 21 - Acceptance

{{SIMPLE}}
- [ ] 21.1 Record the implemented public exports, focused test commands/results and remaining runtime/art/network gaps in this plan; retain original supplied saves and repository-local evidence.
- [ ] 21.2 Update affected architecture/handoff documentation to describe delivered behavior and reconcile the parent index only when this plan's Completion Conditions are met. Keep every unfinished task unchecked.

## Consult on Demand

| Trigger | File name | Start at | Expand only when | Consultation purpose |
| --- | --- | --- | --- | --- |
| ETI GS reports a different effective power total from the industry ledger | [../../../../src/script/api/script_industry.cpp](../../../../src/script/api/script_industry.cpp) | `ScriptIndustry::GetLastMonthProduction` and `GetDistanceManhattanToTile` | The single source-side production path and pinned GS filter are already verified | Inspect API history and distance units; do not multiply power a second time in script or include Deep generation without explicit export. |
| A required public export is absent and its owner/location is disputed | [r2mlvs-multilayer-vehicles-and-structures.md](r2mlvs-multilayer-vehicles-and-structures.md) | `Requirements and ownership` and `Plan Index` | The named producer confirms an intentional contract/ownership change | Find the responsible producer and reconcile its public handoff with direct consumers; the hub cannot substitute for missing implementation. |
| Current behavior contradicts a historical subsystem description | [../../architecture/map-layers/archi-map-layers.md](../../architecture/map-layers/archi-map-layers.md) | `Invariants` and the relevant named source in `Code map` | The live symbol and executable test do not explain the difference | Locate layer addressing or lifecycle rationale. Live code/tests govern delivered behavior; this architecture page does not prove future interfaces exist. |

## Planning Provenance

This is not an execution reading queue. Do not preload it after context compaction. It is non-transitive; ranges record planning use, not runtime authority.

| File name | Sections or Line Ranges used while planning | Planning contribution |
| --- | --- | --- |
| [docs/zz-reach2/architecture/initial-inventory.md](../../architecture/initial-inventory.md) | 185-220 | `Command as the only world mutation`, `Pool-indexed game objects`, `Tile map as a packed array` and `File-suffix modules` oriented command/pool/header ownership and source registration. This historical inventory is a location survey, not authority for current layer behavior. |
| [docs/zz-reach2/architecture/@archi-ttd-level0.md](../../architecture/@archi-ttd-level0.md) | 24-63 | `How the program fits together` and `Source and runtime boundaries` located shared simulation, data/presentation, CMake targets and subsystem handoffs used to choose the live reading sources. |
| [docs/zz-reach2/upgrades/2026-10/r2mlvs-multilayer-vehicles-and-structures.md](r2mlvs-multilayer-vehicles-and-structures.md) | 119-140 | `Requirements and ownership` assigned this plan's feature responsibilities; it shaped the decomposition without making every sibling plan a mandatory reading dependency. |
| [src/newgrf_industries.h](../../../../src/newgrf_industries.h) | 70-89 | The inspected live boundary helped separate characterization, public-contract production, implementation and acceptance batches. It materially constrained one source-side solar augmentation and no duplicate utility/grid authority; this entry records planning influence rather than an additional reading obligation. |
| [User planning-format instructions](<C:/Users/Axonn/.codex/attachments/cd59b20a-3a96-4d54-b4bd-1e8f37639e7d/Pasted text.txt>) | `Global Mandatory Reading`, `Area Mandatory Reading`, `Consult on Demand`, `Planning Provenance`, `Final validation` | Supplies the exact protocol, chronological difficulty batches of at most eight tasks, reading gates and validation format. |
| [src/script/api/script_industry.cpp](../../../../src/script/api/script_industry.cpp) | 94-105 | Deeper planning pass, 2026-10-04: Script-visible history must agree with one effective-generation commit, motivating separate source, grid and history tasks. |

## Completion Conditions

The selected content route is documented and reproducible. A launched, maintained mirror measurably increases a real solar farm's useful output by exactly 300%, without stacking, duplicate production/payments or stale target references. ETI source findings, compatibility results and redistribution status are reported separately.
