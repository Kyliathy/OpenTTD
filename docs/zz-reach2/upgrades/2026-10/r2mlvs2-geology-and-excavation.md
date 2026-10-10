# R2MLVS2 - Geology and Excavation

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
| [docs/zz-reach2/upgrades/2026-10/r2mlvs-multilayer-vehicles-and-structures.md](r2mlvs-multilayer-vehicles-and-structures.md) | 194-202 | `AD-2` supplies the canonical gameplay/ownership decision for every group of this plan. It constrains persistent excavation/deposit ownership and no ordinary construction bypass; a consumer must not redefine it locally. |
| [src/map_layer.h](../../../../src/map_layer.h) | 78-85 | `MultiLayerMap` ownership preamble: `Map` owns packed storage/geometry and this controller owns layer semantics. Every group uses world tiles/explicit layer helpers; camera selection and ad hoc plane offsets cannot define gameplay. |
| [src/map_layer.h](../../../../src/map_layer.h) | 281-315 | `IsPortalPairAllowed`, `GetPortalTargetLayer`, `CheckLayerAllowsConstruction`, `CheckTilesShareLayer` define live connection/construction authority; new profile checks must preserve legacy behavior. |

Status, 2026-10-08: all tasks and this plan's producer Completion Conditions are satisfied. Geology/job/cut/planning/guard/save validation and16.1's actual loaded terrain/survey/job presentation are delivered; retained closure evidence appears below and in [execution notes](r2mlvs2-b-execution-notes.md). Combined normal-newgame/network/performance acceptance remains with Plans15/16 and is not inferred from producer completion. Parent: [R2MLVS master](r2mlvs-multilayer-vehicles-and-structures.md).

**Artifact-layout migration (2026-10-09):** this plan consumes the parent [Development artifact layout](r2mlvs-multilayer-vehicles-and-structures.md#development-artifact-layout), implemented by [R2FC](r2fc-file-cleanup.md). It supersedes earlier docs asset/review/helper placement and fixed runtime-config destinations below. New native homes use `mlm/runtime/`, generated feature reviews use `mlm/reviews/`, logs remain under `logs/`, and reusable code/curated inputs keep their code/test owners. Paths and focused native/cold verification are complete; R2FC records the results and retained-input identities. Repository-relative executable commands run from the checkout root through their context-aware wrappers. Historical pass records remain dated evidence and are not new acceptance claims. No gameplay task is completed by this migration and the separate post-demo fixture-pruning gate remains.

## Outcome and Boundaries

Replace visual bedrock with persistent solid/excavated terrain and finite resource deposits.

## Public Handoff Contract

R2TBMF10 regional-rock amendment,2026-10-09: new worlds retain frozen ore/vent generation and add an independent version1/seed rock field. `GetExcavationCellHardness` first respects saved deposit hardness, otherwise queries nine deterministic24-tile regional centres. Expected Underground Soft/Mixed/Hard/Basalt proportions are60/25/14/1; Deep5/15/60/20. Cutting, quotes, survey and material art use that one query. Save399 GEOS wire2 persists the regional algorithm/seed; earlier worlds load version0/seed0 and retain their measured layer/deposit difficulty through resaves. Actual generation, native390/399 reloads, malformed preview/full-load refusals and physical regional cutting/retreat are recorded in [Z](tbm-fixes/r2tbmf-z-geology-notes.md). Whole cargo remains3/6,2/12,1/20,1/24 Earth/Rock per material; no new cargo fractions exist.

R2TBMF9 terminal-portal consumer,2026-10-09: the existing portal command may replace the completed route's vacant terminal support after the whole cutter stages clear locally. `CheckExcavationConstruction` still requires the full cutter pocket already open and unclaimed; the handover does not change geological terrain, accepted geometry, receipts, cursor or the immutable commissioning anchor. The support/portal owners quote and publish the replacement together. Actual50-cell reciprocal journeys, whole-machine parking, public traffic and native files are verified in [Y](tbm-fixes/r2tbmf-y-exit-notes.md#group9-closure); neither an unfinished route nor a missing footprint gets free clearance.

R2TBMF8 reused-pocket amendment,2026-10-09: an existing portal with an excavated centre but unopened cells in its initial3x3 cutter pocket still requires its real portal issuer and the existing paid first cut. `ResolveExcavationStarter`, canonical quotes/cuts and bidirectional loaded job/pair validation preserve that requirement. Already open cells produce no spoil; remaining cells are charged exactly once. Ordinary open-floor starts gain no bootstrap authority. Actual remote reuse, quoted receipts and full-file/preview continuation are verified in [W](tbm-fixes/r2tbmf-w-order-control-notes.md#group8-closure), superseding the narrower centre-cell-only wording below.

Delivered public type owner: [src/excavation.h](../../../../src/excavation.h). Exports include `ExcavationState`, `GeologyDeposit`, `ExcavationJob`, `CheckExcavatedFootprint`, `CommitExcavationCut`, `GeothermalVentField`. Storage/geology/vent, complete-footprint guards, job reserve/control/persistence, coordinated cuts, outline normalization and deterministic advancing geometry have tested behavior. `PrepareExcavationPass`, `ValidateExcavationPassTerms`, `GetExcavationRetreatPath` and `CheckExcavationSupportReplacement` freeze/check supported backing and independent whole-cut spoil limits.

Save versions 372/373/374 own geology/jobs/frozen passes, with metadata feature bits 1/2/4/8 for geology/vents/job history/pass input. Exact IDs, finite reserves, canonical geometry, progress and static capacity/return input are authoritative; spatial/claim caches rebuild. Predecessor absence remains explicit legacy policy, and historical jobs without frozen input cannot commit cuts. [Architecture](../../architecture/map-layers/archi-map-layers.md#geology-and-excavation-r2mlvs2) and [save transport](../../architecture/savegame/archi-savegame.md#geology-sparse-jobs-and-frozen-passes-372-374) document delivered behavior.

Solid-starter extension (R2MLVS3MP Group 2, implemented 2026-10-05, focused tests pending the milestone build): `GetExcavationCutterEnvelope`, `ExcavationStarterResult`, `ResolveExcavationStarter` and `CheckExcavationStarterGeometry`. While the destination housing's packed cell is not Excavated, the canonical planner, pass, reservation and cut accept a portal-issued first segment that leaves straight through the door and sweeps the whole 3x3 pocket. They charge every swept cell, housing included, and leave portal metadata untouched. Rules: [r2mlvs3mp-f](portal-tbm/r2mlvs3mp-f-solid-starter-contract.md).

Actual TBM/wagon/controller, physical support, portal issuance, exchange and mine production use their delivered feature owners. Material/cash/actor participants cannot substitute statistics for inventory. Multi-process network and combined normal-newgame acceptance remain Plans15/16. Task16.1 is complete through the selected Plan13 source/packed/native gallery and actual public job/survey presentation. Focused producer/save/refusal evidence remains in [execution notes](r2mlvs2-b-execution-notes.md); downstream consumers use public contracts rather than copied private pools/tables.

## Implementation Tasks

Deepening pass, 2026-10-04: implementation groups use only SIMPLE, MEDIUM or HARD difficulty, with at most eight pending tasks each. Sketches describe proposed implementation contracts; verify the named earlier producers at each assumption gate. No gameplay or asset implementation is marked complete by this planning pass.

Groups are chronological and contain consecutive tasks of one difficulty. Each difficulty batch has at most eight tasks; area reading changes only when the implementation boundary or source of truth changes.

### Area Mandatory Reading - Storage and deterministic geology

_Applies to Groups 1-7._

**Entry gate.** The following public contracts must exist before dependent implementation in this area: `src/multilayer_content.h`: `MultiLayerGameplayProfile`, `LayerVehicleCapability`, `GetMultiLayerContentProfile`, `CheckDeepLayerAvailability`, `UtilityPowerAccount`; `src/multilayer_state.h`: `MultiLayerStateSchema`, `ValidateMultiLayerStateReferences`.
Verify each public file/export/schema field with `rg` in the current workspace and inspect its command/field ownership. For a same-plan contract, compile its type/signature fixture or validate its artifact after the preceding producer groups. For cross-plan behavior, also run the producer's focused executable boundary test; declarations alone cannot authorize dependent mutations or runtime claims.
If a required export is absent, stop the dependent implementation, mutation and runtime claims in Groups 1-7. Read-only characterization, source/range verification, test-case design and asset-template work that does not assume the missing behavior may continue.
Do not bypass this gate by copying private pool/tile state, inventing local IDs or cargo registries, using a historical plan as a delivered implementation, opening user-wide content stores, substituting a second utility/accounting authority, or adding an unreviewed fallback path. Finish the named producer or revise the shared contract and its direct consumers first.

| File name | Line Ranges | Description |
| --- | --- | --- |
| [src/map_func.h](../../../../src/map_func.h) | 29-53 | `TileBase` and `TileExtended` are the complete packed storage layouts. New excavation bits require an explicit ownership budget; do not allocate heavyweight objects per tile. |
| [src/core/random_func.hpp](../../../../src/core/random_func.hpp) | 24-43 | `Randomizer` and simulation versus interactive state constrain feature-seed generation: preserve Surface simulation RNG and exclude presentation randomness. |
| [src/map_layer.cpp](../../../../src/map_layer.cpp) | 201-239 | `MultiLayerMap::InitialiseLayer` is the complete delivered per-layer initializer; new-profile rock/geology must retain legacy initialization and Surface seed behavior. |
| [src/multilayer_content.h](../../../../src/multilayer_content.h) (delivered) | Live definitions; no fixed range | [r2mlvs1-gameplay-contracts-and-content.md](r2mlvs1-gameplay-contracts-and-content.md) Groups 1-6, ending with Group 6's public-contract task, publish `MultiLayerGameplayProfile`, `LayerVehicleCapability`, `GetMultiLayerContentProfile`, `CheckDeepLayerAvailability`, `UtilityPowerAccount`. These exports/schema fields are authoritative for this area. Same-plan consumers follow those earlier groups; cross-plan consumers verify the producer's required implementation/tests as well. Proposed names alone do not prove delivery. |
| [src/multilayer_state.h](../../../../src/multilayer_state.h) (delivered foundation) | Live definitions; no fixed range | [r2mlvs15-savegame-determinism-and-lifecycle.md](r2mlvs15-savegame-determinism-and-lifecycle.md) Groups 1-6, ending with Group 6's public-contract task, publish `MultiLayerStateSchema`, `ValidateMultiLayerStateReferences`. These exports/schema fields are authoritative for this area. Same-plan consumers follow those earlier groups; cross-plan consumers verify the producer's required implementation/tests as well. Proposed names alone do not prove delivery. |
| [R2MLVS master](r2mlvs-multilayer-vehicles-and-structures.md) | 310-316 | `AD-16` fixes vent identity and bounded thermal output; geology publishes source capacity while r2mlvs18 later owns allocation and plants. |
| [src/clear_map.h](../../../../src/clear_map.h) | 247-265 | Complete `MakeClear` overwrites tile metadata; the persistent cavity store must survive all tile-type conversions. |
| [src/landscape.cpp](../../../../src/landscape.cpp) | 558-575 | Complete `DoClearSquare` reconstructs clear ground after demolition; constrains storage independence from infrastructure. |

**ASSUMPTION: this was already built previously. If not, re-asses at runtime**. This applies only to the producer contracts named in this area's gate, after their listed producer groups have run. Verify their actual exports and focused tests before using the sketches below; proposed types and helper names in sketches are not evidence of delivered code.

**Storage sketch (proposed).** Terrain occupancy, geology and jobs have different lifetimes. A tile becoming `Industry` must not erase its cavity or the deposit reserve beneath it. A sparse job owns progress; the packed array answers buildability.

```text
rock-state[rock-layer, plane-tile] -> Solid / Excavating / Excavated
deposit-ID -> shape + cargo meaning + remaining reserve + hardness
job-ID -> claimed route + completed cursor + unfinished work
vent-ID -> Deep footprint + bounded thermal rate
```

The Schema Foundation is assumed delivered by the entry gate; concrete geology/job chunks are implemented here, not presumed to exist in that foundation. Vent generation consumes the master's geological requirements and does not depend on a working geothermal plant.

### Group 1 - Choose excavation storage with a measured budget

{{HARD}}
- [X] 1.1 List which state must survive Clear-to-Rail/Industry/Station tile conversion and demolition. Choose a separate packed rock-plane state array unless a proven tile-bit ownership scheme survives every conversion; do not borrow generic m-fields without that proof.
- [X] 1.2 Prototype indexing through `MultiLayerMap` layer/local-coordinate helpers; encode Solid, Excavating and Excavated with a rejected/reserved fourth bit pattern. Check Surface, Space and void tiles cannot address rock-plane storage.
- [X] 1.3 Measure packed state, sparse records and lookup index separately on representative maximum maps. Two bits over two 4096x4096 planes cost 8 MiB; a byte costs 32 MiB before records. Record allocation/reset costs and select the measured layout.

Evidence: [storage decision](r2mlvs2-b-execution-notes.md#group-1---measured-storage-decision), 30 assertions/two cases (`r2mlvs2-a-storage-results.txt`; historical file unavailable). Lookup payload baseline is separate from future bucket/node overhead; lifecycle/persistence follow Groups 3/4.

### Group 2 - Represent deposits and deterministic claims

{{HARD}}
- [X] 2.1 Define a typed deposit ID and compact record containing layer, cargo label reference, bounded shape/span list, initial/remaining reserve and hardness. Keep deposit identity separate from its future mine and visible survey information.
- [X] 2.2 Choose a deterministic tile-to-deposit lookup with bounded per-query cost, such as chunk buckets over compact spans; specify stable ordering and reject/resolve intersecting deposits without iteration-order dependence.
- [X] 2.3 Define claim ownership and overlap rules for mines and excavation jobs. Specify how a cut through ore debits recoverable reserve or leaves retained seam according to the content contract; never leave ore claimable twice.
- [X] 2.4 Add a small deposit fixture covering intersecting shapes, invalid IDs, depleted reserve and deletion/reclaim. Check serialized authority versus rebuilt lookup index explicitly.

Evidence: [finite seam/claim rules](r2mlvs2-b-execution-notes.md#group-2---finite-seam-and-claim-authority), 206 assertions/three cases (`r2mlvs2-c-deposit-results.txt`; historical file unavailable). Canonical authority replay proves rebuilt lookup independence; actual file round trips follow Group 4.

### Group 3 - Generate guaranteed deposits without Surface RNG changes

{{HARD}}
- [X] 3.1 Derive and save versioned geology seeds independently of `_random`; use a local deterministic generator and fixed layer/resource traversal order. Record generator version with the generated state.
- [X] 3.2 Place one mandatory deposit of each of the five resource types on each rock layer first. Reserve enough in-bounds space for an excavatable mine/loading approach; reject unsupported tiny maps before partial world activation.
- [X] 3.3 Add optional deposits only after guaranteed placements, with bounded retries and deterministic fallback locations. Validate rectangular maps and both void-fence policies.
- [X] 3.4 Compare `_random` state and feature-disabled Surface digest before/after geology generation; separately record the enabled-profile fixture where new Surface industries are intentional.

Evidence: [guaranteed generation and seed ownership](r2mlvs2-b-execution-notes.md#group-3---deterministic-guaranteed-generation). 602 focused assertions and three actual dedicated candidates passed; all ten saved Surface fields and RNG streams were preserved. Root metadata is saved now; full geological version/state descriptors follow Group 4.

### Group 4 - Persist geology authority before consumers

{{HARD}}
- [X] 4.1 Using the earlier Schema Foundation Handoff, serialize packed state, deposits, reserve balances, seed/version and company survey visibility. Rebuild spatial indices after load rather than saving addresses.
- [X] 4.2 Define absent-chunk legacy defaults without making existing infrastructure solid. Add round trips for partially depleted deposits, invalid references and truncated/out-of-bounds state.
- [X] 4.3 Publish the descriptor-extension obligation for `ExcavationJob` fields finalized in Groups 7/10; require their implementation and active-progress round trip in 10.3 before TBM consumers start. Do not treat this storage foundation as finished job persistence.

Evidence: [bounded persistence and job extension handoff](r2mlvs2-b-execution-notes.md#group-4---bounded-feature-persistence). 40 focused assertions, six actual accepted loads and five corruption/reference refusals passed. Job descriptors and active-progress acceptance remain in 10.3.

### Group 5 - Storage and deterministic geology

{{MEDIUM}}
- [X] 5.1 Specify survey visibility and pricing. The initial survey overlay may expose deposit locations for planning; revealed information does not excavate tiles or spawn productive mines. Save any information that affects company gameplay.

Evidence: [public planning survey and zero price](r2mlvs2-b-execution-notes.md#group-5---survey-information-and-price). Information creates no terrain, reserve, industry or heat mutation; exact optional information-mask persistence was verified in Group 4. Overlay runtime remains Group 16.

### Group 6 - Add vent geology independently of mineral reserves

{{HARD}}
- [X] 6.1 Define `GeothermalVentField` with stable typed ID, Deep location/shape, thermal-rate capacity and generation version. It has no freight inventory and is not one of the five mineral types.
- [X] 6.2 Place at least one vent after mandatory minerals using a separate deterministic feature stream or explicitly versioned substream. Check enough surrounding solid-but-excavatable ground remains for the proposed 1980 plant and utility footprint.
- [X] 6.3 Define overlap of vent influence and mineral deposits without deleting either identity. Clearing vent tiles preserves the vent; survey visibility reveals it but neither allocates heat nor spawns a plant.
- [X] 6.4 Save and round-trip vent identity/capacity; add a tiny-map failure fixture and a repeated-generation comparison. Publish this geological contract to r2mlvs18 before its heat consumers exist.

Evidence: [independent natural thermal source](r2mlvs2-b-execution-notes.md#group-6---independent-natural-thermal-geology). 73 focused assertions and five actual save/migration/refusal boundaries passed; no power/plant allocation is claimed.

### Group 7 - Publish geology and cut interfaces

{{HARD}}
- [X] 7.1 Publish `src/excavation.h` exports `ExcavationState`, `GeologyDeposit`, `ExcavationJob`, `CheckExcavatedFootprint`, `CommitExcavationCut`, `GeothermalVentField`; declare invalid IDs, read-only lookup and command/simulation ownership.
- [X] 7.2 Specify the cut request/result and job fields now; implement their behavior in the next area. Keep wagon mutation with r2mlvs4/5 through an explicit validated material/capacity contract, not a private vehicle lookup in geology.
- [X] 7.3 Register new source files in CMake and compile a public-header fixture. Identify which exports have behavior tests already and which still require the following groups before downstream mutation is allowed; update every consumer if the public location changes.

Evidence: [public declarations and behavioral readiness](r2mlvs2-b-execution-notes.md#group-7---public-type-and-transaction-handoff), 20 assertions/type-signature fixture (`r2mlvs2-j-contract-results.txt`; historical file unavailable). Operational footprint/job/cut tests remain the next areas.

### Area Mandatory Reading - Construction guards and excavation job ownership

_Applies to Groups 8-10._

**Entry gate.** r2mlvs2 Groups 1-7 supply excavation/deposit/job types and storage; r2mlvs1 supplies profile/Deep availability. This area implements the declared footprint/job behaviors before later cutting consumers use them.
**ASSUMPTION: this was already built previously. If not, re-asses at runtime**. Verify the named prior public exports and their type/behavior fixtures with the current source before dependent work. A declaration alone is insufficient for runtime mutation. If missing, stop the dependent implementation and passing claims in Groups 8-10; independent characterization, test design and source verification may continue. Do not copy private stores, invent substitute IDs/inventory/energy or treat historical planning prose as delivered code.

| File name | Line Ranges | Description |
| --- | --- | --- |
| [src/landscape_cmd.h](../../../../src/landscape_cmd.h) | 15-19 | `CmdLandscapeClear`, `CmdClearArea` and their traits include `NoTest` area clear; enforce excavation even through auto-clear without assuming every command has a harmless estimate. |
| [src/clear_cmd.cpp](../../../../src/clear_cmd.cpp) | 31-64 | `ClearTile_Clear` is the complete delivered ordinary clear path, which currently removes rock. Guard solid terrain here and at full-footprint constructors rather than only hiding GUI tools. |
| [src/command.cpp](../../../../src/command.cpp) | 301-328 | `CommandHelperBase::InternalExecuteValidateTestAndPrepExec` defines refusal/estimate before execution; no validation phase may consume inventory, RNG or funds. |
| [src/excavation.h](../../../../src/excavation.h) (delivered types; later behavior gates apply) | Live definitions; no fixed range | [r2mlvs2-geology-and-excavation.md](r2mlvs2-geology-and-excavation.md) Groups 1-7, ending with Group 7's public-contract task, publish `ExcavationState`, `GeologyDeposit`, `ExcavationJob`, `CheckExcavatedFootprint`, `CommitExcavationCut`, `GeothermalVentField`. These exports/schema fields are authoritative for this area. Same-plan consumers follow those earlier groups; cross-plan consumers verify the producer's required implementation/tests as well. Proposed names alone do not prove delivery. |
| [src/landscape.cpp](../../../../src/landscape.cpp) | 759-817 | `CmdClearArea` performs per-tile work and may partially succeed; guard refused solid tiles without promising whole-area rollback. |

Keep existing partial-area command semantics: refuse each solid mutation without promising unrelated legal tiles roll back. A portal work pocket is a restricted capability, not generally cleared terrain.

Context has compacted 1 times while executing this group.

### Group 8 - Implement one full-footprint construction predicate

{{HARD}}
- [X] 8.1 Implement `CheckExcavatedFootprint` over all footprint/approach tiles with layer validity, full excavation and the canonical Deep availability predicate. Return a stable failure tile/reason without changing tiles or RNG.
- [X] 8.2 Define the portal work pocket as a capability-scoped exception usable only by the assigned commissioning TBM; ordinary footprint checks cannot treat that pocket as cleared ground.
- [X] 8.3 Wire rail/road track, signals, depots and station constructors to the predicate before their first tile mutation. Add one mixed solid/cleared footprint refusal per family.
- [X] 8.4 Wire existing industries, objects and portal approaches; publish the same guard for later tube/geothermal/utility constructors rather than requiring those future implementations here.

Evidence: [complete construction authority](r2mlvs2-b-execution-notes.md#group-8---complete-construction-authority), 71 assertions/three cases (`r2mlvs2-k-guard-results.txt`; historical file unavailable). Commissioning pocket issuance remains with the actual portal/TBM producers; its occupancy contract never changes ordinary buildability.

### Group 9 - Close clearing and area-tool bypasses

{{HARD}}
- [X] 9.1 Guard `ClearTile_Clear` and auto-clear paths so payment cannot excavate solid terrain. Ensure removing existing infrastructure leaves cavity state intact.
- [X] 9.2 Audit area construction, conversion and terraforming entry points, including commands marked `NoTest`. Preflight each atomic footprint; preserve documented partial-area command behavior while forbidding any solid tile mutation.
- [X] 9.3 Exercise command/API paths independently of toolbar visibility, including script calls and extra-view coordinates. Keep editor terrain painting behind the saved explicit override and test normal gameplay cannot inherit it.

Evidence: [clearing and editor boundaries](r2mlvs2-b-execution-notes.md#group-9---clearing-and-editor-boundaries), 57 assertions/three cases (`r2mlvs2-l-clearing-results.txt`; historical file unavailable). Area commands retain their prior per-tile success semantics.

### Group 10 - Implement excavation job reservations

{{HARD}}
- [X] 10.1 Define reserve, advance, pause and cancel states with one owner per active cut. Validate a connected route and claim the work footprint deterministically; conflicting jobs refuse before modifying either claim.
- [X] 10.2 Cancel only unfinished reservations; keep completed cavities, paid work and generated materials. Make invalid owner/removed support produce a recoverable pause reason.

Evidence: [sparse job owner and control](r2mlvs2-b-execution-notes.md#group-10---sparse-job-owner-and-control), 80 assertions/three cases (`r2mlvs2-m-job-results.txt`; historical file unavailable). Saved active progress is still gated by Group 10B.

### Area Mandatory Reading - Excavation job save ownership

_Applies to Group 10B._

The reservation owner in Group 10 must pass its focused competing-job, pause/cancel and unchanged-refusal fixtures first. This subgroup finishes persistence before any TBM consumer can treat reservations as delivered behavior. It is separated to keep the save descriptor/reference work independently reviewable; original task order is unchanged.

| File name | Section / symbols | Description |
| --- | --- | --- |
| [src/excavation.h](../../../../src/excavation.h) | `ExcavationJob`, reserve/advance/pause/cancel exports | Sole sparse job, route/progress and claim authority delivered by Group 10. |
| [src/excavation_state.h](../../../../src/excavation_state.h), [src/geology_state.cpp](../../../../src/geology_state.cpp) | `GeologyStateSnapshot`, `PrepareGeologyRestoration` | Atomic restore and rebuilt caches; active cells must resolve to actual saved jobs. |
| [src/saveload/geology_sl.cpp](../../../../src/saveload/geology_sl.cpp) | `GEOSChunkHandler::FixPointers`, bounded table helpers | Combined validation after all chunks; preview remains independent from live state. |
| [src/multilayer_state.h](../../../../src/multilayer_state.h), [src/saveload/saveload.h](../../../../src/saveload/saveload.h) | Feature flags and append-only save versions | Require present complete job families without reinterpreting older geology-only saves. |

### Group 10B - Persist excavation jobs before the TBM handoff

{{HARD}}
- [X] 10.3 Add descriptors for job route, cursor, partial work and claims, then rebuild lookup caches on load. Test two competing jobs and reload during partial progress before publishing behavioral readiness.

Evidence: [bounded saved job authority](r2mlvs2-b-execution-notes.md#group-10b---bounded-saved-job-authority), 27 snapshot assertions (`r2mlvs2-n-job-state-results.txt`; historical file unavailable), eight real save/migration/refusal boundaries (`r2mlvs2-o-job-save-results.txt`; historical file unavailable). Actual file round trips cover explicit paused removed-owner progress; owned Working progress is covered by the snapshot fixture.

### Area Mandatory Reading - Atomic cuts and bounded area route planning

_Applies to Groups 11-15._

**Entry gate.** The preceding Groups 8-10B must provide tested `CheckExcavatedFootprint` and job reserve/pause/cancel behavior in `src/excavation.h`; Group 7 declared the cut contract that this area now implements.
**ASSUMPTION: this was already built previously. If not, re-asses at runtime**. Verify the named prior public exports and their type/behavior fixtures with the current source before dependent work. A declaration alone is insufficient for runtime mutation. If missing, stop the dependent implementation and passing claims in Groups 11-15; independent characterization, test design and source verification may continue. Do not copy private stores, invent substitute IDs/inventory/energy or treat historical planning prose as delivered code.

| File name | Line Ranges | Description |
| --- | --- | --- |
| [src/excavation.h](../../../../src/excavation.h) (delivered types; later behavior gates apply) | Live definitions; no fixed range | [r2mlvs2-geology-and-excavation.md](r2mlvs2-geology-and-excavation.md) Groups 1-7, ending with Group 7's public-contract task, publish `ExcavationState`, `GeologyDeposit`, `ExcavationJob`, `CheckExcavatedFootprint`, `CommitExcavationCut`, `GeothermalVentField`. For this area, Groups 8-10B additionally implement footprint checking and job ownership; cut behavior is produced here. Same-plan consumers follow those earlier groups; cross-plan consumers verify the producer's required implementation/tests as well. Proposed names alone do not prove delivery. |
| [src/command.cpp](../../../../src/command.cpp) | 301-328 | Complete command validation/estimate boundary prevents cut proposals from mutating terrain, inventory, money or RNG. |
| [R2MLVS master](r2mlvs-multilayer-vehicles-and-structures.md) | 222-232 | Complete `AD-5` supplies advancing 45-degree geometry, charged support, arbitrary final use and safe backed retreat. |

**Cut/route sketch (proposed).** Profile cutter dimensions and yields already exist; r2mlvs4's actual train controller is a later consumer and is not a prerequisite for pure geometry tests.

```text
prepare(job, next_cut, support, material_sink, funds) -> validated cut or pause
estimate -> price/reason without claims, inventory, RNG or money mutation
execute -> revalidate all participants -> commit the complete cut once
```

```mermaid
flowchart LR
    Outline[Canonical outline] --> Pass[Legal advancing pass]
    Pass --> Sweep[Full swept footprint]
    Sweep --> Retreat[Supported retreat]
    Retreat --> Capacity[Earth and Rock capacity]
    Capacity --> Reserve[Reserve accepted route]
    Reserve --> Cut[Commit one cut]
```

### Group 11 - Prepare and commit one cut transaction

{{HARD}}
- [X] 11.1 Specify a pure cut preflight over job identity, target rock, claim ownership, legal support, spoil yields/capacity and cost. Snapshot every field needed by the commit and compute with checked integers.
- [X] 11.2 Implement `CommitExcavationCut` as the geometry/reserve part of a coordinated transaction: revalidate the cut and its material sink before irreversible writes, then commit excavation/support, reserve debit, Earth/Rock additions and charge once. The owning TBM controller supplies the material/funds participant through the public contract.
- [X] 11.3 Define resource-bearing cuts in the result ledger so later mines see the reduced reserve. Include no-progress/paused outcomes that leave all quantities unchanged.
- [X] 11.4 Test estimate versus execute, last-unit reserve, full wagon sink and insufficient cash with state digests before/after; failed preparation must leave no temporary rail or partial cargo.

Evidence: [complete coordinated transaction](r2mlvs2-b-execution-notes.md#group-11---complete-coordinated-transaction), 219 focused assertions/three cases (`r2mlvs2-p-cut-results.txt`; historical file unavailable), real full-authority digest stability/sensitivity (`r2mlvs2-q-digest-results.txt`; historical file unavailable).

### Group 12 - Normalize arbitrary clearing outlines

{{MEDIUM}}
- [X] 12.1 Convert user corridors/branches/chambers to a canonical layer-local target tile set. Reject empty, disconnected, out-of-bounds and wrong-layer outlines with the first failing location.
- [X] 12.2 Separate requested final floor area from temporary approach/turn/support space. Preview any extra required space and its cost; require it in the accepted job so the planner never silently excavates outside authorization.
- [X] 12.3 Keep final land use out of geometry acceptance: road, rail, tube, mine and empty chamber use the same reachable-clearing domain.

Evidence: [canonical floor and explicit extra space](r2mlvs2-b-execution-notes.md#group-12---canonical-floor-and-explicit-extra-space), 38 assertions/two cases (`r2mlvs2-r-outline-results.txt`; historical file unavailable).

### Group 13 - Plan legal advancing passes with bounded search

{{HARD}}
- [X] 13.1 Represent a planning node as cutter position, one of eight headings and the minimum consist/support history needed for collision checks. Expand only forward straight/left-45/right-45 segments; no stationary rotation edges.
- [X] 13.2 Implement a bounded deterministic search for a single target pass with stable neighbor/tie order. Check diagonal swept tiles and the full cutter/consist envelope, not only endpoint centers.
- [X] 13.3 Build area coverage by selecting remaining targets in stable order and joining legal passes. Cap work per planning command and total search; return a visible partial/proposed route or refusal requiring a revised outline rather than an unbounded solver.
- [X] 13.4 Supply geometric tests for straight, two-bend 90-degree, irregular chamber, too-tight corner and impossible turn space. r2mlvs4 owns movement execution against this route contract.

Evidence: [bounded full-consist advancing search](r2mlvs2-b-execution-notes.md#group-13---bounded-full-consist-advancing-search), 156 assertions/three cases (`r2mlvs2-s-route-results.txt`; historical file unavailable).

Context has compacted 1 times while executing this group.

### Group 14 - Validate retreat and spoil capacity before reserving work

{{HARD}}
- [X] 14.1 For each route prefix, verify a supported reverse path to the current exchange point or commissioning pocket. Backing along the path does not create a backward cut or change the cutter heading.
- [X] 14.2 Accumulate Earth and Rock independently against the proposed wagon capacities until the next exchange. Split passes at safe exchange returns; refuse a first cut or pocket exit whose load cannot be recovered.
- [X] 14.3 Check planned support removal/replacement leaves a retreat route after cutter and hauler withdrawal, so a cleared tube corridor can later replace temporary rail.
- [X] 14.4 Freeze the validated pass/return/capacity result as the job input and revalidate at execution when ownership/obstructions change. Test refusal leaves excavation, claims and RNG unchanged.

### Area Mandatory Reading - Sealed-start producer reconciliation

_Applies to Group 14B._

Read master AD-3/AD-5, [R2MLVS3MP Group 2](r2mlvs3mp-master-plan.md#group-2---reconcile-the-solid-starter-before-dependent-work), and Plan 3 Group 8B's actual published bootstrap contract and focused issuance test. Inspect `PlanExcavationRoute`, `ReserveExcavationJob`, `PrepareExcavationPass`, `PrepareExcavationCut`/`CommitExcavationCut`, `ExcavationMaterialParticipant`, and job restoration/pass fingerprints in their current public owners. Existing open-starter tests remain valid for that behavior; they do not prove a solid-starter transaction. If the portal-issued producer is absent, freeze its shared contract with its owner first; do not invent a geological permission flag or paint a room.

### Group 14B - Reconcile canonical planning and cuts with a solid commissioning pocket

{{HARD}}
- [X] 14B.1 Freeze the first advancing-segment contract with Plan 3 Group 8B and Plan 4 Group 2B: source support, actual straddling-consist geometry, permitted destination pocket, safe retreat and every newly solid old/new swept cell that must be paid for.
- [X] 14B.2 Extend the canonical route/pass/reservation APIs to consume and revalidate that explicit portal-issued contract. An occupancy pocket is never initial cleared floor; accepted geometry must include and quote all starter/turn/support excavation without a second planner.
- [X] 14B.3 Extend prepared cut validation/commit for the first solid-pocket segment while preserving physical portal metadata. Keep one geology/material participant transaction, separate Earth/Rock capacity, real support preparation and one cash/reserve debit; ordinary construction remains refused.
- [X] 14B.4 Extend owned job descriptors, reference checks, pass fingerprints and canonical digests for any new authoritative fields. Preserve old open-starter/legacy behavior and test initial-reservation/partial-work reload and stale issuer references.
- [X] 14B.5 Run focused producer success/refusal/rollback tests for solid startup, full one-cargo capacity, unavailable support/funds, issuer loss and replay. Distinguish interface-participant boundary tests from the later actual-wagon commissioning proof; no fake cargo/vehicle owner may substitute for that integration.
- [X] 14B.6 Publish the tested handoff to Plan 4 Groups 2B/4/6-8 and Plan 3 Group 8; update all direct signatures/reading gates. Retain the original MissingSupport characterization as historical evidence and replace its expected outcome only when the corresponding behavior is delivered.

**R2MLVS3MP status, 2026-10-05.** 14B.1 is frozen in [r2mlvs3mp-f](portal-tbm/r2mlvs3mp-f-solid-starter-contract.md). 14B.2-14B.6 are implemented and passed the R2MLVS3MP milestone burst (`r2mlvs3mp-g-burst4-all-results.txt`; historical file unavailable) on 2026-10-05, so they are ticked. Code: `src/excavation.h`, `src/excavation_starter.cpp` (new), and starter mode in `excavation_route.cpp`, `excavation_pass.cpp`, `excavation.cpp` (`ReserveExcavationJob`) and `excavation_cut.cpp`. 14B.4: starter mode is derived from packed geology plus the pair record, so no saved field, descriptor, pass fingerprint or digest changes. The new reference check is the load-time binding in `ValidateLayerPortalWorkRecords`: an unopened cursor-0 job needs its own Commissioning record. Open-starter and legacy behaviour is unchanged. Tests: `src/tests/excavation_starter.cpp` (`[excavation-starter]`, interface participants only) and the renamed `[tbm-sealed-start-gate]`. The historical MissingSupport refusal stays recorded in Plan 4 evidence `r2mlvs4-k`. The test now expects IllegalTurn for a sideways start and InsufficientCapacity for one Earth plus one Rock hopper. Capacity consequence: the 12-cell first cut at the fixture site needs 72 t Earth and 144 t Rock, more than the single-hopper assumption of a cleared 3x3 commissioning area.

### Area Mandatory Reading - Existing cavity and reserve invariants

_Applies to Group 15._

Read master AD-2/AD-5 and the live `CheckExcavatedFootprint`/`CommitExcavationCut` contracts. Inspect infrastructure clearing and the canonical geological reserve-debit ledger only as relevant to demolition/retained seams. Group 14B's bootstrap capability grants no ordinary demolition or reserve reset.

### Group 15 - Excavation and construction contracts

{{MEDIUM}}
- [X] 15.1 Demolition leaves excavated ground. Resource-bearing cut material is accounted as reserve consumption or retained seam according to the content contract; it must not be extracted again by a later mine.

### Area Mandatory Reading - Presentation and validation

_Applies to Groups 16-19._

**Raster source workflow.** For new or corrected terrain, edge, survey and excavation-overlay originals, use Plan13's [framed ImageGen prompt and extraction checks](r2mlvs13-graphics-and-asset-pipeline.md#framed-imagegen-sheets-and-safe-extraction). Multi-cell sheets need pure-magenta source frames, transparent gutters and verified rectangular crops. Keep the terrain connectivity/state matrix and native diamonds; four building views do not replace those requirements. Exclude frames before conversion, and retain separate source/model, packed and runtime proof.

**Entry gate.** The following public contracts must exist before dependent implementation in this area: `src/excavation.h`: `ExcavationState`, `GeologyDeposit`, `ExcavationJob`, `CheckExcavatedFootprint`, `CommitExcavationCut`, `GeothermalVentField`.
Verify each public file/export/schema field with `rg` in the current workspace and inspect its command/field ownership. For a same-plan contract, compile its type/signature fixture or validate its artifact after the preceding producer groups. For cross-plan behavior, also run the producer's focused executable boundary test; declarations alone cannot authorize dependent mutations or runtime claims.
If a required export is absent, stop the dependent implementation, mutation and runtime claims in Groups 16-19. Read-only characterization, source/range verification, test-case design and asset-template work that does not assume the missing behavior may continue.
Do not bypass this gate by copying private pool/tile state, inventing local IDs or cargo registries, using a historical plan as a delivered implementation, opening user-wide content stores, substituting a second utility/accounting authority, or adding an unreviewed fallback path. Finish the named producer or revise the shared contract and its direct consumers first.

| File name | Line Ranges | Description |
| --- | --- | --- |
| [src/tests/map_layer.cpp](../../../../src/tests/map_layer.cpp) | 293-322 | Complete Deep initialization test checks fence/terrain and unchanged RNG; new geology must preserve feature-disabled behavior. |
| [src/tests/map_layer.cpp](../../../../src/tests/map_layer.cpp) | 162-183 | Complete `Plane distances ignore the layer` test fixes projected geometry but grants no cross-layer transfer authority. |
| [src/cargopacket.h](../../../../src/cargopacket.h) | 458-489 | `VehicleCargoList::Append`, `AgeCargo`, `Stage`, `Return`, `Unload`, `Shift` move accountable packets; exchange/descent/processing must not clone packet age or feeder shares. |
| [src/excavation.h](../../../../src/excavation.h) (delivered types; later behavior gates apply) | Live definitions; no fixed range | [r2mlvs2-geology-and-excavation.md](r2mlvs2-geology-and-excavation.md) Groups 1-7, ending with Group 7's public-contract task, publish `ExcavationState`, `GeologyDeposit`, `ExcavationJob`, `CheckExcavatedFootprint`, `CommitExcavationCut`, `GeothermalVentField`. These exports/schema fields are authoritative for this area. Same-plan consumers follow those earlier groups; cross-plan consumers verify the producer's required implementation/tests as well. Proposed names alone do not prove delivery. |
| [Sprite Contract and delivery owner](r2mlvs13-graphics-and-asset-pipeline.md#public-handoff-contract) | Live definitions; no fixed range | `Sprite Contract Handoff` defines manifest fields `asset_id`, `consumer`, `directions`, `frames`, `anchor`, `source`; the matching authoring/conversion/runtime groups deliver actual assets. Verify the requested family and packed loading fixture before graphics integration; template/manifest publication alone is not final art. |

**ASSUMPTION: this was already built previously. If not, re-asses at runtime**. This applies only to the producer contracts named in this area's gate, after their listed producer groups have run. Verify their actual exports and focused tests before using the sketches below; proposed types and helper names in sketches are not evidence of delivered code.

**Art integration gate. ASSUMPTION: this was already built previously. If not, re-asses at runtime**. The solid-face/cavity/overlay rendering task consumes r2mlvs13's delivered excavation assets and `media/multilayer/contracts/multilayer_sprite_manifest.json` entries. Verify their asset IDs, directions/frames and packed loading fixture before integrating them; if absent, stop that art task and keep geology/state tests independent. A temporary visualization is not final graphics acceptance.

**Scoped graphics gate. ASSUMPTION: this was already built previously. If not, re-asses at runtime**. Final graphics tasks in this area require the corresponding r2mlvs13 manifest entries, delivered sprites and packed loading fixture. Verify them before binding/review; if absent, stop only those art tasks and their acceptance claims, continue independent code/state tests, and never count placeholders as finished assets.

### Group 16 - Presentation and validation

{{MEDIUM}}
- [X] 16.1 Add solid faces, cavity floor, edges/corners, survey overlay, job footprint and progress display through r2mlvs13 assets. Solid rock and a dark open chamber must be visually distinguishable.
- [X] 16.2 Test all construction families on solid, active and excavated footprints, including one blocked tile in a larger footprint and a placement through an extra viewport.

16.1 closure (2026-10-07, master8.7): Plan13's40 selected families /2,311 source-exact loaded sprites supply distinct solid/work/cavity/edge depth art and ten live survey markers. The actual project cursor now shows the geological job's accepted final floor, additional charged working space and uncovered requested cells through existing preview roles; sorted temporary set operations avoid quadratic scans. Its localized progress row reads the sole saved job cursor/route count and partial-work accumulator via the public controller query. A presentation timer dirties only the UI, and cancellation/stale company references remove the transient overlay. [Canonical job/area/turn UI checks](../../../../logs/r2mlvs3mp-s-job-presentation-checks.log) pass318 assertions / three focused cases with unchanged geological/project ownership. [Actual saved-world UI/footprint capture](../../../../logs/r2mlvs3mp-s-job-runtime-capture.log) runs the real graphics/application loader and passes97 / one case in [native evidence](../../../../logs/proofs/mlm/native-art/logs/r2mlvs13-cl-native-control-capture.log); [job menu](../../../../logs/proofs/mlm/native-art/construction/layer-controls/images/m1-job-inspection-controls.png) and [canonical footprint](../../../../logs/proofs/mlm/native-art/construction/layer-controls/images/m1-job-inspection-footprint.png) were inspected for localized progress and white/blue roles over distinct rock. The saved source, reserves/work/funds/receiver state and both RNG streams remain unchanged during presentation. Source/GRF bytes remain immutable; user personal art review remains deferred under the master.

### Group 17 - Presentation and validation

{{HARD}}
- [X] 17.1 Test two competing jobs, partial progress, cancellation, insufficient capacity/funds and save/reload without reserve duplication.
- [X] 17.2 Verify deterministic generation on square/rectangular maps, all ten guaranteed deposits, void edges and minimum-size failure.

### Group 18 - Presentation and validation

{{MEDIUM}}
- [X] 18.1 Compare feature-disabled Surface digests with the prior baseline.
- [X] 18.2 Measure memory and active-job tick work on representative large maps; retain results for r2mlvs16.
- [X] 18.3 Verify vent generation does not replace any required mineral type or pre-excavate a plant.
- [X] 18.4 Test area clearing with diagonal turns, irregular outlines and occupied boundaries; reject illegal corner cuts and pre-1980 Deep jobs without terrain/RNG mutation.

### Group 19 - Presentation and validation

{{SIMPLE}}
- [X] 19.1 Record the implemented public exports, focused test commands/results and remaining runtime/art/network gaps in this plan; retain original supplied saves and repository-local evidence.
- [X] 19.2 Update affected architecture/handoff documentation to describe delivered behavior and reconcile the parent index only when this plan's Completion Conditions are met. Keep every unfinished task unchecked.

Evidence: final focused checks (`r2mlvs2-z-final-results.txt`; historical file unavailable), current Surface preservation (`r2mlvs2-z-surface-results.txt`; historical file unavailable), large-map measurements (`r2mlvs2-z-runtime-results.txt`; historical file unavailable), actual frozen-pass saves/refusals (`r2mlvs2-u-pass-save-results.txt`; historical file unavailable), art gate (`r2mlvs2-w-art-gate.txt`; historical file unavailable), and [public handoff/remaining producer boundaries](r2mlvs2-b-execution-notes.md#group-19---delivered-handoffs-and-final-focused-verification). The final build passed; 5,835 Plan 2 assertions and 44 metadata assertions passed, followed by 750 affected assertions after the final ownership review. Every case completed below two minutes. **At that original closure only task 16.1 remained unchecked. The later R2MLVS3MP scheduling amendment also adds pending Group 14B for solid-starter behavior; both that extension and actual Plan 13 art must pass before current plan closure.**

The reconciled [leftovers tracker](r2mlvs99-leftovers.md) records16.1's closure and the actual remaining Plan3 tasks. Full selected art/hash proof is in [the final audit](portal-tbm/r2mlvs3mp-z-b-final-closure-audit.md); personal feedback and later-profile art remain separate.

## Consult on Demand

| Trigger | File name | Start at | Expand only when | Consultation purpose |
| --- | --- | --- | --- | --- |
| A constructor or area-clear command bypasses the solid-rock guard | [../../../../src/landscape.cpp](../../../../src/landscape.cpp) | `CmdLandscapeClear` and `CmdClearArea` | The single-tile guard passes but nested/area execution still mutates solid terrain | Trace auto-clear, NoTest and command execution ordering; patch the canonical excavation guard rather than a menu-only refusal. |
| A required public export is absent and its owner/location is disputed | [r2mlvs-multilayer-vehicles-and-structures.md](r2mlvs-multilayer-vehicles-and-structures.md) | `Requirements and ownership` and `Plan Index` | The named producer confirms an intentional contract/ownership change | Find the responsible producer and reconcile its public handoff with direct consumers; the hub cannot substitute for missing implementation. |
| Current behavior contradicts a historical subsystem description | [../../architecture/map-layers/archi-map-layers.md](../../architecture/map-layers/archi-map-layers.md) | `Invariants` and the relevant named source in `Code map` | The live symbol and executable test do not explain the difference | Locate layer addressing or lifecycle rationale. Live code/tests govern delivered behavior; this architecture page does not prove future interfaces exist. |

## Planning Provenance

This is not an execution reading queue. Do not preload it after context compaction. It is non-transitive; ranges record planning use, not runtime authority.

| File name | Sections or Line Ranges used while planning | Planning contribution |
| --- | --- | --- |
| [docs/zz-reach2/architecture/initial-inventory.md](../../architecture/initial-inventory.md) | 185-220 | `Command as the only world mutation`, `Pool-indexed game objects`, `Tile map as a packed array` and `File-suffix modules` oriented command/pool/header ownership and source registration. This historical inventory is a location survey, not authority for current layer behavior. |
| [docs/zz-reach2/architecture/@archi-ttd-level0.md](../../architecture/@archi-ttd-level0.md) | 24-63 | `How the program fits together` and `Source and runtime boundaries` located shared simulation, data/presentation, CMake targets and subsystem handoffs used to choose the live reading sources. |
| [docs/zz-reach2/upgrades/2026-10/r2mlvs-multilayer-vehicles-and-structures.md](r2mlvs-multilayer-vehicles-and-structures.md) | 119-140 | `Requirements and ownership` assigned this plan's feature responsibilities; it shaped the decomposition without making every sibling plan a mandatory reading dependency. |
| [src/map_layer.h](../../../../src/map_layer.h) | 281-315 | The inspected live boundary helped separate characterization, public-contract production, implementation and acceptance batches. It materially constrained persistent excavation/deposit ownership and no ordinary construction bypass; this entry records planning influence rather than an additional reading obligation. |
| [User planning-format instructions](<C:/Users/Axonn/.codex/attachments/cd59b20a-3a96-4d54-b4bd-1e8f37639e7d/Pasted text.txt>) | `Global Mandatory Reading`, `Area Mandatory Reading`, `Consult on Demand`, `Planning Provenance`, `Final validation` | Supplies the exact protocol, chronological difficulty batches of at most eight tasks, reading gates and validation format. |
| [src/clear_map.h](../../../../src/clear_map.h) | 247-265 | Deeper planning pass, 2026-10-04: Tile reconstruction overwrites metadata, motivating independent persistent cavity storage and guarded type transitions. |

## Completion Conditions

Every ordinary building/traversal path respects solid rock. Excavation changes only through its authoritative transaction, conserves yields and resumes from saves. All required deposits can support an excavated mine and transport access; simply revealing or clearing a tile cannot create free production.
