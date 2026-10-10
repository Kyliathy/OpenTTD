# R2MLVS14 - Remaining assets and runtime integration

Owner: [R2MLVS parent](r2mlvs-multilayer-vehicles-and-structures.md). Dispatch: [R2MLVS3MP](r2mlvs3mp-master-plan.md). Completed producer: [Plan13 - Completed graphics foundations and assets](r2mlvs13-graphics-and-asset-pipeline.md). Architecture: [map layers](../../architecture/map-layers/archi-map-layers.md).

Split,2026-10-10: this continuation owns all169 previously unchecked tasks. Original task/group IDs remain stable, including mixed55.3 and79C.3-79C.4. Completed tasks and evidence remain in Plan13. Existing persistence, acceptance, tube and geothermal Plans14-17 are now15-18. This reorganization grants no implementation or art acceptance.

**Execution remains paused at the user's request after Plan13 task79C.2. Resume at this plan's79C.3 under master10.4B, then79C.4.** Collector selector/industry-draw source changes are unbuilt/unverified. Finish the actual collector display proof before unfinished55.3; the master dispatch still overrides simple numerical ordering. Original later-family and whole-profile scope remains intact.

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
> - Per the user's 2026-10-07 instruction, store all new build/test/application
>   capture logs in the repository `logs/` directory. Preserve owning-plan
>   basenames and link them here with `../../../../logs/<basename>.log`.
>   Earlier documentation logs were removed by the user; retain historical
>   result summaries without recreating those files. Markdown notes and scratch
>   scripts still belong beside their owning plan.
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

| Source | Relevant sections | Purpose |
| --- | --- | --- |
| [Plan13 completed contract](r2mlvs13-graphics-and-asset-pipeline.md#public-handoff-contract) | Public Handoff Contract and only the executing family's completed producer | Consume established schema/geometry/source boundaries without redoing completed tasks. |
| [R2MLVS parent](r2mlvs-multilayer-vehicles-and-structures.md#development-artifact-layout) | Development artifact layout; AD-1; applicable ownership invariants | Preserve repository-local source/runtime/review homes and feature-owned state. |
| This plan and [master queue](r2mlvs3mp-master-plan.md) | Current dispatch and nearest applicable Area/Local | Resume the correct remaining scope without treating partial art as runtime delivery. |

Before raster generation/editing, read the [canonical framed ImageGen prompt and extraction checks](r2mlvs13-graphics-and-asset-pipeline.md#framed-imagegen-sheets-and-safe-extraction) plus the installed imagegen skill. After compaction, read only relevant completed producer sections. Preserve all existing native ranges, immutable originals and MLVI gameplay identities. Use focused combined checks and the master's build cadence.

## Outcome and Boundaries

Complete the remaining source families, deterministic conversion, native consumers and runtime review for the original complete graphics profile. The [shared asset-family requirements](r2mlvs13-graphics-and-asset-pipeline.md#asset-families) remain authoritative. Collector source/payload evidence is complete through Plan13 task79C.2; actual native loading and ordinary-game rendering remain here. User feedback and later-profile tasks retain their unchecked scope and historical dispositions.

## Public Handoff Contract

Consume the existing `media/multilayer/contracts/multilayer_sprite_manifest.json` schema from Plan13: `asset_id`, `consumer`, `directions`, `frames`, `anchor`, `source` and validated per-family fields. This continuation extends source/payload/runtime coverage through the same owners and public paths; it creates no second manifest, inventory, utility ledger or rendering authority. Completed foundational exports stay in Plan13; new asset delivery and full-profile acceptance belong here.

## Implementation Tasks

### Area Mandatory Reading - Source generation and reproducible conversion

_Applies to Groups 9-81._

**Entry gate.** The following public contracts must exist before dependent implementation in this area: `media/multilayer/contracts/multilayer_sprite_manifest.json`: `asset_id`, `consumer`, `directions`, `frames`, `anchor`, `source`.
Verify each public file/export/schema field with `rg` in the current workspace and inspect its command/field ownership. For a same-plan contract, compile its type/signature fixture or validate its artifact after the preceding producer groups. For cross-plan behavior, also run the producer's focused executable boundary test; declarations alone cannot authorize dependent mutations or runtime claims.
If a required export is absent, stop the dependent implementation, mutation and runtime claims in Groups 9-81. Read-only characterization, source/range verification, test-case design and asset-template work that does not assume the missing behavior may continue.
Do not bypass this gate by copying private pool/tile state, inventing local IDs or cargo registries, using a historical plan as a delivered implementation, opening user-wide content stores, substituting a second utility/accounting authority, or adding an unreviewed fallback path. Finish the named producer or revise the shared contract and its direct consumers first.

**Image-generation gate.** Verify that the execution harness exposes the required image-generation tool and read the installed imagegen skill before generating/editing raster art. If unavailable, stop only raster-generation/edit tasks in this area; manifest checks, placement templates, palette/anchor validation and packing-tool work may continue. Do not substitute unauthorized providers, private caches or a different image-editing route to bypass the skill/tool prerequisite.

| File name | Line Ranges | Description |
| --- | --- | --- |
| [media/multilayer/tools/sprite_quantize.py](../../../../media/multilayer/tools/sprite_quantize.py) | 379-411 | `build_sheet` and `sprite_lines` implement deterministic native packing/anchors and NFO emission; preserve the repeatable source-to-payload contract. |
| [media/baseset/openttd/CMakeLists.txt](../../../../media/baseset/openttd/CMakeLists.txt) | 6-67 | Complete `if(GRFCODEC_FOUND)` and `create_grf_command` block defines PNG/NFO generation inputs; validate the separate checked-in fallback packaging path too. |
| [media/baseset/CMakeLists.txt](../../../../media/baseset/CMakeLists.txt) | 16-34 | Complete payload list and conditional generated-payload marking retain the checked-in `openttd.grf` path when the generator is unavailable. |
| `media/multilayer/contracts/multilayer_sprite_manifest.json` (planned public contract; link after implementation) | Live definitions; no fixed range | [r2mlvs13-graphics-and-asset-pipeline.md](r2mlvs13-graphics-and-asset-pipeline.md) Groups 1-8, ending with Group 8's public-contract task, publish `asset_id`, `consumer`, `directions`, `frames`, `anchor`, `source`. These exports/schema fields are authoritative for this area. Same-plan consumers follow those earlier groups; cross-plan consumers verify the producer's required implementation/tests as well. Proposed names alone do not prove delivery. |

**ASSUMPTION: this was already built previously. If not, re-asses at runtime**. This applies only to the producer contracts named in this area's gate, after their listed producer groups have run. Verify their actual exports and focused tests before using the sketches below; proposed types and helper names in sketches are not evidence of delivered code.

**Conversion sketch.** Asset generation selects original pixels; deterministic processing begins from those retained originals. A later runtime correction updates source/manifest and reruns conversion, never patches only the distributed binary.

#### Framed ImageGen sheets and safe extraction

Read the canonical [prompt, fixed magenta-frame geometry and extraction/model checks](r2mlvs13-graphics-and-asset-pipeline.md#framed-imagegen-sheets-and-safe-extraction) before new raster work. The completed plan retains the measured trial and immutable prompt provenance. Apply its per-output checks and separate-file fallback without regenerating approved sources.

```mermaid
flowchart LR
    Template[Approved geometry and style] --> Original[Selected original raster]
    Original --> Quantize[Palette and anchor-preserving crop]
    Quantize --> Validate[Coverage and mask validation]
    Validate --> Pack[Stable IDs and GRF payload]
    Pack --> Runtime[Real gameplay review]
    Runtime --> Original
```

**ASSUMPTION: this was already built previously. If not, re-asses at runtime**. The preceding Sprite Contract Handoff supplies exact schema/counts/anchors. Verify the manifest fixture before writing the converter; stop dependent packing on an absent/invalid manifest and continue independent source/template work without inventing offsets.

Context has compacted 1 times while executing this group.


### Local Mandatory Reading - Later-family authoring preparation

_Applies to Group 9B when dispatched by a later milestone; master M1 does not dispatch it._

Read this plan's Global and Source generation Area, then only the nonselected manifest families and their actual consumer geometry. Use the installed imagegen skill, repository provenance and original immutability rules. Do not re-author already selected M1 sources.


### Group 9B - Prepare remaining profile asset families

{{MEDIUM}}
- [ ] 9B.1 Apply the image-generation skill/tool and repository provenance workflow to the remaining profile families when their actual consumer contracts are ready.
- [ ] 9B.2 Publish bounded approved geometry/style briefs for each remaining family/model, preserving the original whole-profile authoring requirement.
- [ ] 9B.3 Retain immutable selected originals and independent revision history for those families before deterministic conversion.


### Local Mandatory Reading - Remaining ground family originals

_Applies to remaining Groups19-46; completed families remain in Plan13._

Read this plan's Global and Source generation Area, the executing family's manifest row, and only its actual vehicle/industry/station geometry owner. Use the completed Plan13 style/registration contract and canonical framed prompt. Preserve the selected model across every required view/state; do not redo completed exchange, service or receiver families. Missing geometry/runtime producers block only their dependent source claims.

### Group 19 - Author underground freight locomotive

Goal-authorized skip (2026-10-08, master finalization): dispatched expansion work in this group is skipped under [finalization exception GP-TG](portal-tbm/r2mlvs3mp-z-a-finalization-ledger.md#gp-tg-tube-and-geothermal-expansion-requires-major-gap-planning). Existing delivered prefixes stay complete; unimplemented profile work remains pending. No skipped task is markedX.

{{HARD}}
- [ ] 19.1 For underground freight locomotive, derive a single-model brief from its approved manifest/template: conventional mineral-freight traction silhouette. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [ ] 19.2 Produce the required directions/rotations for underground freight locomotive from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [ ] 19.3 Add running/idle/service and purchase views for underground freight locomotive, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.


### Group 20 - Author mineral wagon

Goal-authorized skip (2026-10-08, master finalization): dispatched expansion work in this group is skipped under [finalization exception GP-TG](portal-tbm/r2mlvs3mp-z-a-finalization-ledger.md#gp-tg-tube-and-geothermal-expansion-requires-major-gap-planning). Existing delivered prefixes stay complete; unimplemented profile work remains pending. No skipped task is markedX.

{{HARD}}
- [ ] 20.1 For mineral wagon, derive a single-model brief from its approved manifest/template: profile-compatible ore load presentation. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [ ] 20.2 Produce the required directions/rotations for mineral wagon from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [ ] 20.3 Add each required refit/load and purchase view for mineral wagon, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.


### Group 21 - Author mine truck

Goal-authorized skip (2026-10-08, master finalization): dispatched expansion work in this group is skipped under [finalization exception GP-TG](portal-tbm/r2mlvs3mp-z-a-finalization-ledger.md#gp-tg-tube-and-geothermal-expansion-requires-major-gap-planning). Existing delivered prefixes stay complete; unimplemented profile work remains pending. No skipped task is markedX.

{{HARD}}
- [ ] 21.1 For mine truck, derive a single-model brief from its approved manifest/template: compact underground road freight body. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [ ] 21.2 Produce the required directions/rotations for mine truck from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [ ] 21.3 Add required load/refit/service and purchase views for mine truck, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.


### Group 22 - Author sealed mineral carrier

Goal-authorized skip (2026-10-08, master finalization): dispatched expansion work in this group is skipped under [finalization exception GP-TG](portal-tbm/r2mlvs3mp-z-a-finalization-ledger.md#gp-tg-tube-and-geothermal-expansion-requires-major-gap-planning). Existing delivered prefixes stay complete; unimplemented profile work remains pending. No skipped task is markedX.

{{HARD}}
- [ ] 22.1 For sealed mineral carrier, derive a single-model brief from its approved manifest/template: deep-capable closed cargo body. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [ ] 22.2 Produce the required directions/rotations for sealed mineral carrier from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [ ] 22.3 Add required sealed-load/service and purchase views for sealed mineral carrier, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.


### Group 24 - Author underground rail freight station

Goal-authorized skip (2026-10-08, master finalization): dispatched expansion work in this group is skipped under [finalization exception GP-TG](portal-tbm/r2mlvs3mp-z-a-finalization-ledger.md#gp-tg-tube-and-geothermal-expansion-requires-major-gap-planning). Existing delivered prefixes stay complete; unimplemented profile work remains pending. No skipped task is markedX.

{{HARD}}
- [ ] 24.1 For underground rail freight station, derive a single-model brief from its approved manifest/template: loading platform and mine loader aligned to rail. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [ ] 24.2 Produce the required directions/rotations for underground rail freight station from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [ ] 24.3 Add all approaches/loading and back/front parts for underground rail freight station, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.


### Group 25 - Author road mine terminal

Goal-authorized skip (2026-10-08, master finalization): dispatched expansion work in this group is skipped under [finalization exception GP-TG](portal-tbm/r2mlvs3mp-z-a-finalization-ledger.md#gp-tg-tube-and-geothermal-expansion-requires-major-gap-planning). Existing delivered prefixes stay complete; unimplemented profile work remains pending. No skipped task is markedX.

{{HARD}}
- [ ] 25.1 For road mine terminal, derive a single-model brief from its approved manifest/template: truck loading and turning access. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [ ] 25.2 Produce the required directions/rotations for road mine terminal from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [ ] 25.3 Add all approaches/loading and back/front parts for road mine terminal, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.


### Group 27 - Author Coal mine

Goal-authorized skip (2026-10-08, master finalization): dispatched expansion work in this group is skipped under [finalization exception GP-TG](portal-tbm/r2mlvs3mp-z-a-finalization-ledger.md#gp-tg-tube-and-geothermal-expansion-requires-major-gap-planning). Existing delivered prefixes stay complete; unimplemented profile work remains pending. No skipped task is markedX.

{{HARD}}
- [ ] 27.1 For Coal mine, derive a single-model brief from its approved manifest/template: Underground machinery/loading silhouette with Coal identity. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [ ] 27.2 Produce the required directions/rotations for Coal mine from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [ ] 27.3 Add construction/working/paused/depleted layout states for Coal mine, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.


### Group 28 - Author Iron mine

Goal-authorized skip (2026-10-08, master finalization): dispatched expansion work in this group is skipped under [finalization exception GP-TG](portal-tbm/r2mlvs3mp-z-a-finalization-ledger.md#gp-tg-tube-and-geothermal-expansion-requires-major-gap-planning). Existing delivered prefixes stay complete; unimplemented profile work remains pending. No skipped task is markedX.

{{HARD}}
- [ ] 28.1 For Iron mine, derive a single-model brief from its approved manifest/template: Underground machinery/loading silhouette with Iron identity. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [ ] 28.2 Produce the required directions/rotations for Iron mine from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [ ] 28.3 Add construction/working/paused/depleted layout states for Iron mine, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.


### Group 29 - Author Copper mine

Goal-authorized skip (2026-10-08, master finalization): dispatched expansion work in this group is skipped under [finalization exception GP-TG](portal-tbm/r2mlvs3mp-z-a-finalization-ledger.md#gp-tg-tube-and-geothermal-expansion-requires-major-gap-planning). Existing delivered prefixes stay complete; unimplemented profile work remains pending. No skipped task is markedX.

{{HARD}}
- [ ] 29.1 For Copper mine, derive a single-model brief from its approved manifest/template: Underground machinery/loading silhouette with Copper identity. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [ ] 29.2 Produce the required directions/rotations for Copper mine from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [ ] 29.3 Add construction/working/paused/depleted layout states for Copper mine, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.


### Group 30 - Author Bauxite mine

Goal-authorized skip (2026-10-08, master finalization): dispatched expansion work in this group is skipped under [finalization exception GP-TG](portal-tbm/r2mlvs3mp-z-a-finalization-ledger.md#gp-tg-tube-and-geothermal-expansion-requires-major-gap-planning). Existing delivered prefixes stay complete; unimplemented profile work remains pending. No skipped task is markedX.

{{HARD}}
- [ ] 30.1 For Bauxite mine, derive a single-model brief from its approved manifest/template: Underground machinery/loading silhouette with Bauxite identity. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [ ] 30.2 Produce the required directions/rotations for Bauxite mine from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [ ] 30.3 Add construction/working/paused/depleted layout states for Bauxite mine, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.


### Group 31 - Author Salt mine

Goal-authorized skip (2026-10-08, master finalization): dispatched expansion work in this group is skipped under [finalization exception GP-TG](portal-tbm/r2mlvs3mp-z-a-finalization-ledger.md#gp-tg-tube-and-geothermal-expansion-requires-major-gap-planning). Existing delivered prefixes stay complete; unimplemented profile work remains pending. No skipped task is markedX.

{{HARD}}
- [ ] 31.1 For Salt mine, derive a single-model brief from its approved manifest/template: Underground machinery/loading silhouette with Salt identity. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [ ] 31.2 Produce the required directions/rotations for Salt mine from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [ ] 31.3 Add construction/working/paused/depleted layout states for Salt mine, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.


### Group 32 - Author Lithium mine

Goal-authorized skip (2026-10-08, master finalization): dispatched expansion work in this group is skipped under [finalization exception GP-TG](portal-tbm/r2mlvs3mp-z-a-finalization-ledger.md#gp-tg-tube-and-geothermal-expansion-requires-major-gap-planning). Existing delivered prefixes stay complete; unimplemented profile work remains pending. No skipped task is markedX.

{{HARD}}
- [ ] 32.1 For Lithium mine, derive a single-model brief from its approved manifest/template: Deep machinery/loading silhouette with Lithium identity. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [ ] 32.2 Produce the required directions/rotations for Lithium mine from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [ ] 32.3 Add construction/working/paused/depleted layout states for Lithium mine, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.


### Group 33 - Author Nickel mine

Goal-authorized skip (2026-10-08, master finalization): dispatched expansion work in this group is skipped under [finalization exception GP-TG](portal-tbm/r2mlvs3mp-z-a-finalization-ledger.md#gp-tg-tube-and-geothermal-expansion-requires-major-gap-planning). Existing delivered prefixes stay complete; unimplemented profile work remains pending. No skipped task is markedX.

{{HARD}}
- [ ] 33.1 For Nickel mine, derive a single-model brief from its approved manifest/template: Deep machinery/loading silhouette with Nickel identity. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [ ] 33.2 Produce the required directions/rotations for Nickel mine from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [ ] 33.3 Add construction/working/paused/depleted layout states for Nickel mine, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.


### Group 34 - Author Cobalt mine

Goal-authorized skip (2026-10-08, master finalization): dispatched expansion work in this group is skipped under [finalization exception GP-TG](portal-tbm/r2mlvs3mp-z-a-finalization-ledger.md#gp-tg-tube-and-geothermal-expansion-requires-major-gap-planning). Existing delivered prefixes stay complete; unimplemented profile work remains pending. No skipped task is markedX.

{{HARD}}
- [ ] 34.1 For Cobalt mine, derive a single-model brief from its approved manifest/template: Deep machinery/loading silhouette with Cobalt identity. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [ ] 34.2 Produce the required directions/rotations for Cobalt mine from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [ ] 34.3 Add construction/working/paused/depleted layout states for Cobalt mine, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.


### Group 35 - Author Rare-earth mine

Goal-authorized skip (2026-10-08, master finalization): dispatched expansion work in this group is skipped under [finalization exception GP-TG](portal-tbm/r2mlvs3mp-z-a-finalization-ledger.md#gp-tg-tube-and-geothermal-expansion-requires-major-gap-planning). Existing delivered prefixes stay complete; unimplemented profile work remains pending. No skipped task is markedX.

{{HARD}}
- [ ] 35.1 For Rare-earth mine, derive a single-model brief from its approved manifest/template: Deep machinery/loading silhouette with Rare-earth identity. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [ ] 35.2 Produce the required directions/rotations for Rare-earth mine from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [ ] 35.3 Add construction/working/paused/depleted layout states for Rare-earth mine, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.


### Group 36 - Author Uranium mine

Goal-authorized skip (2026-10-08, master finalization): dispatched expansion work in this group is skipped under [finalization exception GP-TG](portal-tbm/r2mlvs3mp-z-a-finalization-ledger.md#gp-tg-tube-and-geothermal-expansion-requires-major-gap-planning). Existing delivered prefixes stay complete; unimplemented profile work remains pending. No skipped task is markedX.

{{HARD}}
- [ ] 36.1 For Uranium mine, derive a single-model brief from its approved manifest/template: Deep machinery/loading silhouette with Uranium identity. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [ ] 36.2 Produce the required directions/rotations for Uranium mine from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [ ] 36.3 Add construction/working/paused/depleted layout states for Uranium mine, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.


### Group 39 - Author metal refinery

Goal-authorized skip (2026-10-08, master finalization): dispatched expansion work in this group is skipped under [finalization exception GP-TG](portal-tbm/r2mlvs3mp-z-a-finalization-ledger.md#gp-tg-tube-and-geothermal-expansion-requires-major-gap-planning). Existing delivered prefixes stay complete; unimplemented profile work remains pending. No skipped task is markedX.

{{HARD}}
- [ ] 39.1 For metal refinery, derive a single-model brief from its approved manifest/template: profile-selected Surface receiver footprint and recipe cues. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [ ] 39.2 Produce the required directions/rotations for metal refinery from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [ ] 39.3 Add construction/working/full/idle states for metal refinery, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.


### Group 40 - Author chemical works

Goal-authorized skip (2026-10-08, master finalization): dispatched expansion work in this group is skipped under [finalization exception GP-TG](portal-tbm/r2mlvs3mp-z-a-finalization-ledger.md#gp-tg-tube-and-geothermal-expansion-requires-major-gap-planning). Existing delivered prefixes stay complete; unimplemented profile work remains pending. No skipped task is markedX.

{{HARD}}
- [ ] 40.1 For chemical works, derive a single-model brief from its approved manifest/template: profile-selected Surface receiver footprint and recipe cues. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [ ] 40.2 Produce the required directions/rotations for chemical works from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [ ] 40.3 Add construction/working/full/idle states for chemical works, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.


### Group 41 - Author Surface battery works

Goal-authorized skip (2026-10-08, master finalization): dispatched expansion work in this group is skipped under [finalization exception GP-TG](portal-tbm/r2mlvs3mp-z-a-finalization-ledger.md#gp-tg-tube-and-geothermal-expansion-requires-major-gap-planning). Existing delivered prefixes stay complete; unimplemented profile work remains pending. No skipped task is markedX.

{{HARD}}
- [ ] 41.1 For Surface battery works, derive a single-model brief from its approved manifest/template: profile-selected Surface receiver footprint and recipe cues. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [ ] 41.2 Produce the required directions/rotations for Surface battery works from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [ ] 41.3 Add construction/working/full/idle states for Surface battery works, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.


### Group 42 - Author electronics works

Goal-authorized skip (2026-10-08, master finalization): dispatched expansion work in this group is skipped under [finalization exception GP-TG](portal-tbm/r2mlvs3mp-z-a-finalization-ledger.md#gp-tg-tube-and-geothermal-expansion-requires-major-gap-planning). Existing delivered prefixes stay complete; unimplemented profile work remains pending. No skipped task is markedX.

{{HARD}}
- [ ] 42.1 For electronics works, derive a single-model brief from its approved manifest/template: profile-selected Surface receiver footprint and recipe cues. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [ ] 42.2 Produce the required directions/rotations for electronics works from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [ ] 42.3 Add construction/working/full/idle states for electronics works, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.


### Group 43 - Author component manufacture

Goal-authorized skip (2026-10-08, master finalization): dispatched expansion work in this group is skipped under [finalization exception GP-TG](portal-tbm/r2mlvs3mp-z-a-finalization-ledger.md#gp-tg-tube-and-geothermal-expansion-requires-major-gap-planning). Existing delivered prefixes stay complete; unimplemented profile work remains pending. No skipped task is markedX.

{{HARD}}
- [ ] 43.1 For component manufacture, derive a single-model brief from its approved manifest/template: profile-selected Surface receiver footprint and recipe cues. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [ ] 43.2 Produce the required directions/rotations for component manufacture from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [ ] 43.3 Add construction/working/full/idle states for component manufacture, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.


### Group 44 - Author robotics manufacture

Goal-authorized skip (2026-10-08, master finalization): dispatched expansion work in this group is skipped under [finalization exception GP-TG](portal-tbm/r2mlvs3mp-z-a-finalization-ledger.md#gp-tg-tube-and-geothermal-expansion-requires-major-gap-planning). Existing delivered prefixes stay complete; unimplemented profile work remains pending. No skipped task is markedX.

{{HARD}}
- [ ] 44.1 For robotics manufacture, derive a single-model brief from its approved manifest/template: profile-selected Surface receiver footprint and recipe cues. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [ ] 44.2 Produce the required directions/rotations for robotics manufacture from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [ ] 44.3 Add construction/working/full/idle states for robotics manufacture, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.


### Group 45 - Author mirror-kit manufacture

Goal-authorized skip (2026-10-08, master finalization): dispatched expansion work in this group is skipped under [finalization exception GP-TG](portal-tbm/r2mlvs3mp-z-a-finalization-ledger.md#gp-tg-tube-and-geothermal-expansion-requires-major-gap-planning). Existing delivered prefixes stay complete; unimplemented profile work remains pending. No skipped task is markedX.

{{HARD}}
- [ ] 45.1 For mirror-kit manufacture, derive a single-model brief from its approved manifest/template: profile-selected Surface receiver footprint and recipe cues. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [ ] 45.2 Produce the required directions/rotations for mirror-kit manufacture from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [ ] 45.3 Add construction/working/full/idle states for mirror-kit manufacture, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.


### Group 46 - Author nuclear plant

Goal-authorized skip (2026-10-08, master finalization): dispatched expansion work in this group is skipped under [finalization exception GP-TG](portal-tbm/r2mlvs3mp-z-a-finalization-ledger.md#gp-tg-tube-and-geothermal-expansion-requires-major-gap-planning). Existing delivered prefixes stay complete; unimplemented profile work remains pending. No skipped task is markedX.

{{HARD}}
- [ ] 46.1 For nuclear plant, derive a single-model brief from its approved manifest/template: profile-selected Surface receiver footprint and recipe cues. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [ ] 46.2 Produce the required directions/rotations for nuclear plant from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [ ] 46.3 Add construction/working/full/idle states for nuclear plant, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.


### Local Mandatory Reading - Actual dated generator source geometry

_Applies to Groups55-56; supplements Source generation Area._

Read only the two public generator manifest rows, `MultiLayerGeothermalConstructionKind`/`GetMultiLayerGeothermalConstruction`, `PrepareGeothermalSite`, current registered IIDS/TIDS in `src/geothermal_definitions.cpp`, and verified Plan18 Group8B/`GetGeothermalGeneratorGeometry`; operating Group8 remains pending. Actual paid footprint/native cell layout and profile-owned construction terms must precede dependent originals; historical manifest sketches alone do not establish a delivered native type. If a dated geometry producer is missing, supply a bounded passive producer in its owning Plan18 group before authoring, retaining whole operating activation/transported bootstrap for master10.6. Reuse fixed camera/light and genuine original status components after geometry is proven; never substitute the collector body or silently activate an unfunded generator.

For resumed55.3 and later generator originals, use the [framed-sheet prompt and extraction checks](#framed-imagegen-sheets-and-safe-extraction). The bordered stage2 trial passed layout extraction but failed model identity; it remains excluded. Collector packing/display79C remains ahead of further generator authoring in the master queue.


### Group 55 - Author Vent-Tap Powerhouse

Completed prerequisites for this mixed group are retained in [r2mlvs13-graphics-and-asset-pipeline.md](r2mlvs13-graphics-and-asset-pipeline.md#group-55---author-vent-tap-powerhouse). Original task IDs remain unchanged.

Dispatched under resumed master10.4. Plan18 staged8B verifies both dated4x4/sixteen-cell native geometries before55.1. Tasks55.1-55.2 are complete; unfinished55.3 is temporarily deferred behind the explicitly staged collector packing/display proof79C at master10.4B. Whole operating generator/transported bootstrap remains master10.6.

{{HARD}}
- [ ] 55.3 Add construction/working/heat-starved states for Vent-Tap Powerhouse, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.


### Group 56 - Author Magma-Loop Power Station

Goal-authorized skip (2026-10-08, master finalization): dispatched expansion work in this group is skipped under [finalization exception GP-TG](portal-tbm/r2mlvs3mp-z-a-finalization-ledger.md#gp-tg-tube-and-geothermal-expansion-requires-major-gap-planning). Existing delivered prefixes stay complete; unimplemented profile work remains pending. No skipped task is markedX.

{{HARD}}
- [ ] 56.1 For Magma-Loop Power Station, derive a single-model brief from its approved manifest/template: 2000 closed-loop plant distinct from the early powerhouse. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [ ] 56.2 Produce the required directions/rotations for Magma-Loop Power Station from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [ ] 56.3 Add construction/working/heat-starved states for Magma-Loop Power Station, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.


### Local Mandatory Reading - Remaining plant and orbital originals

_Applies to Groups57-78._

Read this plan's Global and Source generation Area, the executing family's manifest row, and its feature owner's delivered geometry/state contract. Geothermal geometry is owned by Plan18; orbital and solar providers retain Plans8-12. Consult only that family's completed Plan13 source/style examples and the canonical framed prompt. Keep unimplemented providers and missing source/packing/runtime coverage explicitly pending.

### Group 57 - Author Geothermal Smelter

{{HARD}}
- [ ] 57.1 For Geothermal Smelter, derive a single-model brief from its approved manifest/template: 2010 ore/direct-heat furnace footprint. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [ ] 57.2 Produce the required directions/rotations for Geothermal Smelter from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [ ] 57.3 Add construction/working/heat-starved/full states for Geothermal Smelter, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.


### Group 58 - Author Lithothermal Battery Works

{{HARD}}
- [ ] 58.1 For Lithothermal Battery Works, derive a single-model brief from its approved manifest/template: 2030 chemical/battery processing footprint. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [ ] 58.2 Produce the required directions/rotations for Lithothermal Battery Works from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [ ] 58.3 Add construction/working/heat-starved/full states for Lithothermal Battery Works, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.


### Group 59 - Author Mantle Forge

{{HARD}}
- [ ] 59.1 For Mantle Forge, derive a single-model brief from its approved manifest/template: 2045 heavy component machinery footprint. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [ ] 59.2 Produce the required directions/rotations for Mantle Forge from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [ ] 59.3 Add construction/working/heat-starved/full states for Mantle Forge, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.


### Group 60 - Author heat/electric utilities and risers

Goal-authorized skip (2026-10-08, master finalization): dispatched expansion work in this group is skipped under [finalization exception GP-TG](portal-tbm/r2mlvs3mp-z-a-finalization-ledger.md#gp-tg-tube-and-geothermal-expansion-requires-major-gap-planning). Existing delivered prefixes stay complete; unimplemented profile work remains pending. No skipped task is markedX.

{{HARD}}
- [ ] 60.1 For heat/electric utilities and risers, derive a single-model brief from its approved manifest/template: visually different thermal and electrical links. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [ ] 60.2 Produce the required directions/rotations for heat/electric utilities and risers from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [ ] 60.3 Add connected/broken/idle conduit and portal-riser states for heat/electric utilities and risers, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.


### Group 61 - Author Space Launch Center

{{HARD}}
- [ ] 61.1 For Space Launch Center, derive a single-model brief from its approved manifest/template: complete pad/assembly/recovery footprint with Surface freight access. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [ ] 61.2 Produce the required directions/rotations for Space Launch Center from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [ ] 61.3 Add all rotations/preparation/launch/occupied foreground parts for Space Launch Center, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.


### Group 62 - Author cargo rocket

{{HARD}}
- [ ] 62.1 For cargo rocket, derive a single-model brief from its approved manifest/template: reusable hull and payload/service identity. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [ ] 62.2 Produce the required directions/rotations for cargo rocket from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [ ] 62.3 Add loading/preparing/ascent/orbit/recovery/service views for cargo rocket, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.


### Group 63 - Author small mining probe

{{HARD}}
- [ ] 63.1 For small mining probe, derive a single-model brief from its approved manifest/template: compact hook/mining robot silhouette. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [ ] 63.2 Produce the required directions/rotations for small mining probe from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [ ] 63.3 Add free/attached/mining/loaded/service directional views for small mining probe, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.


### Group 64 - Author cargo return pod

{{HARD}}
- [ ] 64.1 For cargo return pod, derive a single-model brief from its approved manifest/template: separate saved descent unit. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [ ] 64.2 Produce the required directions/rotations for cargo return pod from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [ ] 64.3 Add release/descent/recovery frames for cargo return pod, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.


### Group 65 - Author heavy mining robot

{{HARD}}
- [ ] 65.1 For heavy mining robot, derive a single-model brief from its approved manifest/template: larger station-built extraction machinery. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [ ] 65.2 Produce the required directions/rotations for heavy mining robot from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [ ] 65.3 Add free/attached/mining/loaded/service directional views for heavy mining robot, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.


### Group 66 - Author orbital hauler

{{HARD}}
- [ ] 66.1 For orbital hauler, derive a single-model brief from its approved manifest/template: cargo carrier distinct from miners. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [ ] 66.2 Produce the required directions/rotations for orbital hauler from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [ ] 66.3 Add direction/load/docking/service and purchase views for orbital hauler, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.


### Group 67 - Author metallic asteroid

{{HARD}}
- [ ] 67.1 For metallic asteroid, derive a single-model brief from its approved manifest/template: readable Nickel composition with stable body footprint. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [ ] 67.2 Produce the required directions/rotations for metallic asteroid from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [ ] 67.3 Add required size/attachment/mining/depleted variants for metallic asteroid, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.


### Group 68 - Author carbonaceous asteroid

{{HARD}}
- [ ] 68.1 For carbonaceous asteroid, derive a single-model brief from its approved manifest/template: readable Water Ice composition with stable body footprint. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [ ] 68.2 Produce the required directions/rotations for carbonaceous asteroid from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [ ] 68.3 Add required size/attachment/mining/depleted variants for carbonaceous asteroid, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.


### Group 69 - Author rich asteroid

{{HARD}}
- [ ] 69.1 For rich asteroid, derive a single-model brief from its approved manifest/template: readable Rare Earth composition with stable body footprint. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [ ] 69.2 Produce the required directions/rotations for rich asteroid from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [ ] 69.3 Add required size/attachment/mining/depleted variants for rich asteroid, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.


### Group 70 - Author Core module

{{HARD}}
- [ ] 70.1 For Core module, derive a single-model brief from its approved manifest/template: distinct station anchor footprint and connector/dock anchors. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [ ] 70.2 Produce the required directions/rotations for Core module from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [ ] 70.3 Add blueprint/partial/complete/blocked functional states for Core module, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.


### Group 71 - Author Dock module

{{HARD}}
- [ ] 71.1 For Dock module, derive a single-model brief from its approved manifest/template: distinct berth footprint and connector/dock anchors. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [ ] 71.2 Produce the required directions/rotations for Dock module from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [ ] 71.3 Add blueprint/partial/complete/blocked functional states for Dock module, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.


### Group 72 - Author Storage module

{{HARD}}
- [ ] 72.1 For Storage module, derive a single-model brief from its approved manifest/template: distinct bounded inventory footprint and connector/dock anchors. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [ ] 72.2 Produce the required directions/rotations for Storage module from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [ ] 72.3 Add blueprint/partial/complete/blocked functional states for Storage module, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.


### Group 73 - Author Utility module

{{HARD}}
- [ ] 73.1 For Utility module, derive a single-model brief from its approved manifest/template: distinct power/service footprint and connector/dock anchors. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [ ] 73.2 Produce the required directions/rotations for Utility module from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [ ] 73.3 Add blueprint/partial/complete/blocked functional states for Utility module, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.


### Group 74 - Author Assembly module

{{HARD}}
- [ ] 74.1 For Assembly module, derive a single-model brief from its approved manifest/template: distinct robot manufacture footprint and connector/dock anchors. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [ ] 74.2 Produce the required directions/rotations for Assembly module from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [ ] 74.3 Add blueprint/partial/complete/blocked functional states for Assembly module, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.


### Group 75 - Author Processor module

{{HARD}}
- [ ] 75.1 For Processor module, derive a single-model brief from its approved manifest/template: distinct material conversion footprint and connector/dock anchors. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [ ] 75.2 Produce the required directions/rotations for Processor module from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [ ] 75.3 Add blueprint/partial/complete/blocked functional states for Processor module, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.


### Group 76 - Author native solar farm

{{HARD}}
- [ ] 76.1 For native solar farm, derive a single-model brief from its approved manifest/template: Surface panel layout and utility connection. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [ ] 76.2 Produce the required directions/rotations for native solar farm from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [ ] 76.3 Add construction/normal/boosted/idle states for native solar farm, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.


### Group 77 - Author solar mirror and kit

{{HARD}}
- [ ] 77.1 For solar mirror and kit, derive a single-model brief from its approved manifest/template: deployed reflective structure distinct from cargo kit. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [ ] 77.2 Produce the required directions/rotations for solar mirror and kit from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [ ] 77.3 Add packed/deploying/aiming/operating/idle states for solar mirror and kit, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.


### Group 78 - Author construction and cargo controls

Goal-authorized skip (2026-10-08, master finalization): dispatched expansion work in this group is skipped under [finalization exception GP-TG](portal-tbm/r2mlvs3mp-z-a-finalization-ledger.md#gp-tg-tube-and-geothermal-expansion-requires-major-gap-planning). Existing delivered prefixes stay complete; unimplemented profile work remains pending. No skipped task is markedX.

{{HARD}}
- [ ] 78.1 For construction and cargo controls, derive a single-model brief from its approved manifest/template: native-size layer/cargo identities with separate collection/drop circles. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [ ] 78.2 Produce the required directions/rotations for construction and cargo controls from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [ ] 78.3 Add menu/cursor/locked/blocked/turn-preview and resource icons for construction and cargo controls, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.


### Local Mandatory Reading - Delivered collector packing and actual display

_Applies to staged Group79C; consumes only the source-complete collector before unfinished generator source work._

Read this plan's Global, the Source generation Area's packing exports and only collector-relevant runtime ownership from the Runtime integration Area. Inspect `m1-pack-scope.json`, the public `facility.vent-collector` row and `collector-source-manifest.json`; source completeness and native anchors/cells must be real. Read `pack_m1_art.selected_ranges`, `source_poses`, `format_header`, `derive`, `publish` and `bind_packed_payload`, the base-set PNG/NFO/CMake routes and current generated native ranges. Preserve protected0..65, reserved slots, all existing42-family ranges, both generated and checked-in payload paths and immutable MLVI gameplay definitions.

For display, read `GetMultiLayerSprite`, `GetMultiLayerReceiverSprite`, native `DrawTile_Industry`, `GetIndustryConstructionStage`/`IsIndustryCompleted`, `GetGeothermalCollectorState`, `IsGeothermalCollectorOperational` and the actual collector/claim/shared-Heat state owners needed by the selected state. Reuse actual industry location/access/work/input/Heat records; do not invent a graphics-owned stock or operating generator. Inspect existing focused native sprite-loading and collector save/constructor fixtures only. A normal-game capture must disclose fixture-supplied materials and cannot claim the later Surface-transported bootstrap.

Completed79C.1-79C.2 are in [Plan13](r2mlvs13-graphics-and-asset-pipeline.md#group-79c---pack-and-display-the-delivered-collector):43 packed families/3495 selected poses, including1152 collector poses at2436 and native bound3588. Preserve the collector append and all42 earlier ranges. Inspect the unbuilt `GetMultiLayerCollectorSprite`/`GetMultiLayerIndustrySprite` changes as pending implementation; existing payload decoding is not evidence that these native consumers work.


### Group 79C - Pack and display the delivered collector

Completed prerequisites for this mixed group are retained in [r2mlvs13-graphics-and-asset-pipeline.md](r2mlvs13-graphics-and-asset-pipeline.md#group-79c---pack-and-display-the-delivered-collector). Original task IDs remain unchanged.

Explicitly staged by master10.4B after verified source54, before further unfinished55-56 originals. This closes only the collector subset of packing/runtime obligations; original whole Groups79-84 and aggregate master10.4 remain pending for their other selected families.

{{HARD}}
- [ ] 79C.3 Bind the packed family to real native collector industry cells and construction/completion/current Heat states in the actual industry draw path. Visual clocks change presentation only; camera/animation selection must preserve work, cargo, funds, claims, Heat and RNG. Do not activate an operating powerhouse or replace its missing producer.
- [ ] 79C.4 After the packing and consumer changes, build both configurations once and run a focused combined source/packing/loading/selection/collector burst. Load a current native collector world in the ordinary game and capture the actual construction/completed machinery plus native source-pixel identities, with fixture staging disclosed. Close master10.4B only after this loaded evidence passes, then resume55.3. No full regression or transported first-plant claim is required by this art slice.

**Paused at the user's request after79C.2.** Resume at79C.3 under master10.4B. Collector selector/industry-draw source changes have begun but are unbuilt/unverified; they do not complete79C.3. Do not resume generator originals or claim actual game display until79C.3-79C.4 pass.


### Local Mandatory Reading - Remaining conversion and packing

_Applies to Groups79-81._

Read this plan's Global/Source generation Area and the executing selected families' manifest/source reports. Consume completed Plan13 packing exports, stable native ranges and actual generated/cached payload proofs; extend the same owners. Read only the relevant quantizer/packer/build paths, preserving existing sprite indices, anchors, palette rules and immutable source provenance. Native loading and gameplay presentation require their own current evidence.

### Group 79 - Convert and pack from stable originals

Goal-authorized skip (2026-10-08, master finalization): dispatched expansion work in this group is skipped under [finalization exception GP-TG](portal-tbm/r2mlvs3mp-z-a-finalization-ledger.md#gp-tg-tube-and-geothermal-expansion-requires-major-gap-planning). Existing delivered prefixes stay complete; unimplemented profile work remains pending. No skipped task is markedX.

{{HARD}}
- [ ] 79.1 Extend deterministic quantization/cropping to consume manifest records, preserve index-0 transparency and handle reserved company/animation colors explicitly. Preserve world anchor when transparent margins are cropped.
- [ ] 79.2 Generate back/front parts and NFO/content-package metadata in stable manifest order with pinned tool versions. Keep existing published sprite offsets unchanged.
- [ ] 79.3 Pack the selected core/content routes and regenerate required GRF/checksum metadata. Compare two clean conversions from the same originals/manifest for identical outputs.
- [ ] 79.4 Verify a build without the asset generator loads the checked-in generated payload and resolves the same IDs/counts; source-only success is not distributed-asset acceptance.


### Group 80 - Validate asset structure and palette deterministically

Goal-authorized skip (2026-10-08, master finalization): dispatched expansion work in this group is skipped under [finalization exception GP-TG](portal-tbm/r2mlvs3mp-z-a-finalization-ledger.md#gp-tg-tube-and-geothermal-expansion-requires-major-gap-planning). Existing delivered prefixes stay complete; unimplemented profile work remains pending. No skipped task is markedX.

{{HARD}}
- [ ] 80.1 Validate required record fields, unique IDs, expected direction/frame/state Cartesian coverage and exact native dimensions/anchors before packing. Report the failing asset ID and source.
- [ ] 80.2 Check transparent index, forbidden palette ranges, intended company-color pixels and empty/missing frames against the declared per-asset policy.
- [ ] 80.3 Compare manifest packed references and output hashes against emitted sprite count/offset metadata; reject stale or unreferenced gameplay assets.
- [ ] 80.4 Add small corruption fixtures for one missing frame, bad bounds and illegal palette entry. Validate authored transparent holes against explicit masks/templates; arbitrary legitimate holes still need human visual review.


### Group 81 - Source generation and reproducible conversion

Goal-authorized skip (2026-10-08, master finalization): dispatched expansion work in this group is skipped under [finalization exception GP-TG](portal-tbm/r2mlvs3mp-z-a-finalization-ledger.md#gp-tg-tube-and-geothermal-expansion-requires-major-gap-planning). Existing delivered prefixes stay complete; unimplemented profile work remains pending. No skipped task is markedX.

{{MEDIUM}}
- [ ] 81.1 Keep simulation state independent of animation timing.


### Area Mandatory Reading - Runtime integration and review

_Applies to Groups 82-89._

**Entry gate.** The following public contracts must exist before dependent implementation in this area: `media/multilayer/contracts/multilayer_sprite_manifest.json`: `asset_id`, `consumer`, `directions`, `frames`, `anchor`, `source`.
Verify each public file/export/schema field with `rg` in the current workspace and inspect its command/field ownership. For a same-plan contract, compile its type/signature fixture or validate its artifact after the preceding producer groups. For cross-plan behavior, also run the producer's focused executable boundary test; declarations alone cannot authorize dependent mutations or runtime claims.
If a required export is absent, stop the dependent implementation, mutation and runtime claims in Groups 82-89. Read-only characterization, source/range verification, test-case design and asset-template work that does not assume the missing behavior may continue.
Do not bypass this gate by copying private pool/tile state, inventing local IDs or cargo registries, using a historical plan as a delivered implementation, opening user-wide content stores, substituting a second utility/accounting authority, or adding an unreviewed fallback path. Finish the named producer or revise the shared contract and its direct consumers first.

**Scoped integration gates.** The following producer dependencies apply only to the named implementation/test families. Verify each required public export and its focused producer test before those integrations; absent late producers do not block already independent features.

- `src/excavation.h` is required for the M1 excavation/TBM/exchange integration and corresponding runtime captures; full-gallery/placeholder retirement awaits every required family in Groups 82-83, 85-89 (only the named integration/subscenario). If absent, stop only that integration/subscenario and its passing claim. Other independent code, art, persistence and tests in this area may continue.
- `src/layer_portal_cmd.h` is required for the M1 excavation/TBM/exchange integration and corresponding runtime captures; full-gallery/placeholder retirement awaits every required family in Groups 82-83, 85-89 (only the named integration/subscenario). If absent, stop only that integration/subscenario and its passing claim. Other independent code, art, persistence and tests in this area may continue.
- `src/tbm.h` is required for the M1 excavation/TBM/exchange integration and corresponding runtime captures; full-gallery/placeholder retirement awaits every required family in Groups 82-83, 85-89 (only the named integration/subscenario). If absent, stop only that integration/subscenario and its passing claim. Other independent code, art, persistence and tests in this area may continue.
- `src/tbm_exchange.h` is required for the M1 excavation/TBM/exchange integration and corresponding runtime captures; full-gallery/placeholder retirement awaits every required family in Groups 82-83, 85-89 (only the named integration/subscenario). If absent, stop only that integration/subscenario and its passing claim. Other independent code, art, persistence and tests in this area may continue.
- `src/multilayer_industry.h` is required for the mine/conventional-fleet integration and corresponding runtime captures; full-gallery/placeholder retirement awaits every required family in Groups 84-89 (only the named integration/subscenario). If absent, stop only that integration/subscenario and its passing claim. Other independent code, art, persistence and tests in this area may continue.
- `src/underground_gui.h` is required for the mine/conventional-fleet integration and corresponding runtime captures; full-gallery/placeholder retirement awaits every required family in Groups 84-89 (only the named integration/subscenario). If absent, stop only that integration/subscenario and its passing claim. Other independent code, art, persistence and tests in this area may continue.
- `src/space.h` is required for the orbital/solar integration and corresponding runtime captures; full-gallery/placeholder retirement awaits every required family in Groups 84-89 (only the named integration/subscenario). If absent, stop only that integration/subscenario and its passing claim. Other independent code, art, persistence and tests in this area may continue.
- `src/launch_center.h` is required for the orbital/solar integration and corresponding runtime captures; full-gallery/placeholder retirement awaits every required family in Groups 84-89 (only the named integration/subscenario). If absent, stop only that integration/subscenario and its passing claim. Other independent code, art, persistence and tests in this area may continue.
- `src/asteroid.h` is required for the orbital/solar integration and corresponding runtime captures; full-gallery/placeholder retirement awaits every required family in Groups 84-89 (only the named integration/subscenario). If absent, stop only that integration/subscenario and its passing claim. Other independent code, art, persistence and tests in this area may continue.
- `src/orbital_station.h` is required for the orbital/solar integration and corresponding runtime captures; full-gallery/placeholder retirement awaits every required family in Groups 84-89 (only the named integration/subscenario). If absent, stop only that integration/subscenario and its passing claim. Other independent code, art, persistence and tests in this area may continue.
- `src/solar_power.h` is required for the orbital/solar integration and corresponding runtime captures; full-gallery/placeholder retirement awaits every required family in Groups 84-89 (only the named integration/subscenario). If absent, stop only that integration/subscenario and its passing claim. Other independent code, art, persistence and tests in this area may continue.
- `src/transport_tube.h` is required for the tube/geothermal integration and corresponding runtime captures; full-gallery/placeholder retirement awaits every required family in Groups 84-89 (only the named integration/subscenario). If absent, stop only that integration/subscenario and its passing claim. Other independent code, art, persistence and tests in this area may continue.
- `src/geothermal.h` is required for the tube/geothermal integration and corresponding runtime captures; full-gallery/placeholder retirement awaits every required family in Groups 84-89 (only the named integration/subscenario). If absent, stop only that integration/subscenario and its passing claim. Other independent code, art, persistence and tests in this area may continue.

Do not emulate an absent producer with copied private state, fake inventories/energy, local IDs or a second registry/grid. A final combined closure/gallery/round-trip still waits for every required family; an independent passing case is not a pass for the missing integration.

| File name | Line Ranges | Description |
| --- | --- | --- |
| [src/map_layer_gui.h](../../../../src/map_layer_gui.h) | 67-73 | `GetVehicleDisplayLayer` is the public visible/pickable-layer contract; keep ordinary aircraft/ships/disasters on Surface and place orbital units explicitly. |
| [src/tests/map_layer.cpp](../../../../src/tests/map_layer.cpp) | 430-442 | Complete display-order test retains immutable Space=2 and Deep=3 storage meanings; future UI/profile work cannot reorder saved IDs. |
| [src/network/network_command.cpp](../../../../src/network/network_command.cpp) | 256-288 | `NetworkExecuteLocalCommandQueue` executes agreed frame order/company authority; all jobs use deterministic simulation time. |
| `media/multilayer/contracts/multilayer_sprite_manifest.json` (planned public contract; link after implementation) | Live definitions; no fixed range | [r2mlvs13-graphics-and-asset-pipeline.md](r2mlvs13-graphics-and-asset-pipeline.md) Groups 1-8, ending with Group 8's public-contract task, publish `asset_id`, `consumer`, `directions`, `frames`, `anchor`, `source`. These exports/schema fields are authoritative for this area. Same-plan consumers follow those earlier groups; cross-plan consumers verify the producer's required implementation/tests as well. Proposed names alone do not prove delivery. |
| `src/excavation.h` (planned public contract; link after implementation) | Live definitions; no fixed range | [r2mlvs2-geology-and-excavation.md](r2mlvs2-geology-and-excavation.md) Groups 1-7, ending with Group 7's public-contract task, publish `ExcavationState`, `GeologyDeposit`, `ExcavationJob`, `CheckExcavatedFootprint`, `CommitExcavationCut`, `GeothermalVentField`. These exports/schema fields are authoritative for this area. Same-plan consumers follow those earlier groups; cross-plan consumers verify the producer's required implementation/tests as well. Proposed names alone do not prove delivery. Read this producer for the M1 excavation/TBM/exchange integration and corresponding runtime captures; full-gallery/placeholder retirement awaits every required family in Groups 82-83, 85-89 (only the named integration/subscenario); missing it blocks only that integration, not independent work. |
| [src/layer_portal_cmd.h](../../../../src/layer_portal_cmd.h) | Live definitions; no fixed range | [r2mlvs3-sealed-portals-and-deep-rail.md](r2mlvs3-sealed-portals-and-deep-rail.md) Groups 1-7, ending with Group 7's public-contract task, publish `LayerPortalWorkState`, `GetLayerPortalWorkState`, `CheckLayerPortalWorkAccess`, `CommissionLayerPortalTBM`. These exports/schema fields are authoritative for this area. Same-plan consumers follow those earlier groups; cross-plan consumers verify the producer's required implementation/tests as well. Proposed names alone do not prove delivery. Read this producer for the M1 excavation/TBM/exchange integration and corresponding runtime captures; full-gallery/placeholder retirement awaits every required family in Groups 82-83, 85-89 (only the named integration/subscenario); missing it blocks only that integration, not independent work. |
| `src/tbm.h` (planned public contract; link after implementation) | Live definitions; no fixed range | [r2mlvs4-tunnel-boring-machines.md](r2mlvs4-tunnel-boring-machines.md) Groups 1-5, ending with Group 5's public-contract task, publish `TBMProject`, `TBMWorkState`, `ValidateTBMAreaRoute`, `AdvanceTBMWork`. These exports/schema fields are authoritative for this area. Same-plan consumers follow those earlier groups; cross-plan consumers verify the producer's required implementation/tests as well. Proposed names alone do not prove delivery. Read this producer for the M1 excavation/TBM/exchange integration and corresponding runtime captures; full-gallery/placeholder retirement awaits every required family in Groups 82-83, 85-89 (only the named integration/subscenario); missing it blocks only that integration, not independent work. |
| `src/tbm_exchange.h` (planned public contract; link after implementation) | Live definitions; no fixed range | [r2mlvs5-spoil-wagons-and-haulage.md](r2mlvs5-spoil-wagons-and-haulage.md) Groups 1-5, ending with Group 5's public-contract task, publish `TBMExchangeAssignment`, `ValidateTBMWagonExchange`, `ExecuteTBMWagonExchange`. These exports/schema fields are authoritative for this area. Same-plan consumers follow those earlier groups; cross-plan consumers verify the producer's required implementation/tests as well. Proposed names alone do not prove delivery. Read this producer for the M1 excavation/TBM/exchange integration and corresponding runtime captures; full-gallery/placeholder retirement awaits every required family in Groups 82-83, 85-89 (only the named integration/subscenario); missing it blocks only that integration, not independent work. |
| `src/multilayer_industry.h` (planned public contract; link after implementation) | Live definitions; no fixed range | [r2mlvs6-mines-and-resource-chains.md](r2mlvs6-mines-and-resource-chains.md) Groups 1-6, ending with Group 6's public-contract task, publish `LayerIndustryCapability`, `ValidateMineDepositClaim`, `ProduceLayerMineCargo`, `PrepareLayerRecipeBatch`, `CommitLayerRecipeBatch`. These exports/schema fields are authoritative for this area. Same-plan consumers follow those earlier groups; cross-plan consumers verify the producer's required implementation/tests as well. Proposed names alone do not prove delivery. Read this producer for the mine/conventional-fleet integration and corresponding runtime captures; full-gallery/placeholder retirement awaits every required family in Groups 84-89 (only the named integration/subscenario); missing it blocks only that integration, not independent work. |
| `src/underground_gui.h` (planned public contract; link after implementation) | Live definitions; no fixed range | [r2mlvs7-underground-construction-and-fleet.md](r2mlvs7-underground-construction-and-fleet.md) Groups 1-5, ending with Group 5's public-contract task, publish `ShowUndergroundConstructionToolbar`, `BuildUndergroundVehicleFilter`. These exports/schema fields are authoritative for this area. Same-plan consumers follow those earlier groups; cross-plan consumers verify the producer's required implementation/tests as well. Proposed names alone do not prove delivery. Read this producer for the mine/conventional-fleet integration and corresponding runtime captures; full-gallery/placeholder retirement awaits every required family in Groups 84-89 (only the named integration/subscenario); missing it blocks only that integration, not independent work. |
| `src/space.h` (planned public contract; link after implementation) | Live definitions; no fixed range | [r2mlvs8-space-world-and-construction.md](r2mlvs8-space-world-and-construction.md) Groups 1-7, ending with Group 7's public-contract task, publish `SpaceVehicle`, `OrbitalBodyID`, `SpaceMission`, `SpaceStructure`, `SpaceTransition`. These exports/schema fields are authoritative for this area. Same-plan consumers follow those earlier groups; cross-plan consumers verify the producer's required implementation/tests as well. Proposed names alone do not prove delivery. Read this producer for the orbital/solar integration and corresponding runtime captures; full-gallery/placeholder retirement awaits every required family in Groups 84-89 (only the named integration/subscenario); missing it blocks only that integration, not independent work. |
| `src/launch_center.h` (planned public contract; link after implementation) | Live definitions; no fixed range | [r2mlvs9-launch-centers-and-rockets.md](r2mlvs9-launch-centers-and-rockets.md) Groups 1-7, ending with Group 7's public-contract task, publish `LaunchCenterAnchor`, `LaunchCollectionHub`, `SurfaceHandlingJob`, `SpaceDescentJob`, `CanReleaseOrbitalCargo`, `CommitOrbitalCargoRelease`. These exports/schema fields are authoritative for this area. Same-plan consumers follow those earlier groups; cross-plan consumers verify the producer's required implementation/tests as well. Proposed names alone do not prove delivery. Read this producer for the orbital/solar integration and corresponding runtime captures; full-gallery/placeholder retirement awaits every required family in Groups 84-89 (only the named integration/subscenario); missing it blocks only that integration, not independent work. |
| `src/asteroid.h` (planned public contract; link after implementation) | Live definitions; no fixed range | [r2mlvs10-asteroids-and-mining-probes.md](r2mlvs10-asteroids-and-mining-probes.md) Groups 1-7, ending with Group 7's public-contract task, publish `AsteroidBody`, `MiningProbeMission`, `AdvanceAsteroidMotion`, `AdvanceProbeMining`. These exports/schema fields are authoritative for this area. Same-plan consumers follow those earlier groups; cross-plan consumers verify the producer's required implementation/tests as well. Proposed names alone do not prove delivery. Read this producer for the orbital/solar integration and corresponding runtime captures; full-gallery/placeholder retirement awaits every required family in Groups 84-89 (only the named integration/subscenario); missing it blocks only that integration, not independent work. |
| `src/orbital_station.h` (planned public contract; link after implementation) | Live definitions; no fixed range | [r2mlvs11-orbital-stations-and-manufacturing.md](r2mlvs11-orbital-stations-and-manufacturing.md) Groups 1-6, ending with Group 6's public-contract task, publish `OrbitalModule`, `OrbitalConstructionJob`, `ReserveOrbitalConstructionInputs`, `CommitOrbitalConstructionStep`. These exports/schema fields are authoritative for this area. Same-plan consumers follow those earlier groups; cross-plan consumers verify the producer's required implementation/tests as well. Proposed names alone do not prove delivery. Read this producer for the orbital/solar integration and corresponding runtime captures; full-gallery/placeholder retirement awaits every required family in Groups 84-89 (only the named integration/subscenario); missing it blocks only that integration, not independent work. |
| `src/solar_power.h` (planned public contract; link after implementation) | Live definitions; no fixed range | [r2mlvs12-solar-farms-mirrors-and-eti.md](r2mlvs12-solar-farms-mirrors-and-eti.md) Groups 1-6, ending with Group 6's public-contract task, publish `SolarGenerationAccount`, `MirrorAssignment`, `ComputeMirrorSolarOutput`. These exports/schema fields are authoritative for this area. Same-plan consumers follow those earlier groups; cross-plan consumers verify the producer's required implementation/tests as well. Proposed names alone do not prove delivery. Read this producer for the orbital/solar integration and corresponding runtime captures; full-gallery/placeholder retirement awaits every required family in Groups 84-89 (only the named integration/subscenario); missing it blocks only that integration, not independent work. |
| `src/transport_tube.h` (planned public contract; link after implementation) | Live definitions; no fixed range | [r2mlvs17-underground-transport-tubes.md](r2mlvs17-underground-transport-tubes.md) Groups 1-8, ending with Group 8's public-contract task, publish `TubeVehicleCapability`, `TubeTerminal`, `CheckTubeConstruction`, `ComputeTubePowerDemand`. These exports/schema fields are authoritative for this area. Same-plan consumers follow those earlier groups; cross-plan consumers verify the producer's required implementation/tests as well. Proposed names alone do not prove delivery. Read this producer for the tube/geothermal integration and corresponding runtime captures; full-gallery/placeholder retirement awaits every required family in Groups 84-89 (only the named integration/subscenario); missing it blocks only that integration, not independent work. |
| `src/geothermal.h` (planned public contract; link after implementation) | Live definitions; no fixed range | [r2mlvs18-geothermal-energy-and-industry.md](r2mlvs18-geothermal-energy-and-industry.md) Groups 1-7, ending with Group 7's public-contract task, publish `GeothermalHeatAllocation`, `GeothermalUtilityLink`, `AllocateVentHeat`, `ExportGeothermalPower`. These exports/schema fields are authoritative for this area. Same-plan consumers follow those earlier groups; cross-plan consumers verify the producer's required implementation/tests as well. Proposed names alone do not prove delivery. Read this producer for the tube/geothermal integration and corresponding runtime captures; full-gallery/placeholder retirement awaits every required family in Groups 84-89 (only the named integration/subscenario); missing it blocks only that integration, not independent work. |

**ASSUMPTION: this was already built previously. If not, re-asses at runtime**. This applies only to the producer contracts named in this area's gate, after their listed producer groups have run. Verify their actual exports and focused tests before using the sketches below; proposed types and helper names in sketches are not evidence of delivered code.


### Local Mandatory Reading - Remaining runtime families and review

_Applies to Groups84-89._

Read this plan's Global/Runtime integration Area, the executing family's completed source/payload handoff in Plan13 or this continuation, and only its real feature state/rendering owners. Reuse selected native M1 evidence without reopening completed tasks. Require current loaded pixels, actual state transitions and family-specific captures for new runtime claims; defer missing producers and operator feedback explicitly.

### Group 84 - Runtime integration and review

Goal-authorized skip (2026-10-08, master finalization): dispatched expansion work in this group is skipped under [finalization exception GP-TG](portal-tbm/r2mlvs3mp-z-a-finalization-ledger.md#gp-tg-tube-and-geothermal-expansion-requires-major-gap-planning). Existing delivered prefixes stay complete; unimplemented profile work remains pending. No skipped task is markedX.

{{MEDIUM}}
- [ ] 84.1 Integrate all ten mines and the underground conventional fleet after their r2mlvs6/7 code is delivered.
- [ ] 84.2 Integrate orbital launch, probe, module and solar families after their corresponding r2mlvs8..12 producers are delivered.
- [ ] 84.3 Integrate tube/geothermal art after r2mlvs17/18 producers are delivered; this later work does not block M1 art.


### Group 85 - Runtime integration and review

{{SIMPLE}}
- [ ] 85.1 Capture native-scale and zoomed scenes across directions, terrain boundaries, load/production states, vehicle overlap and building transparency.


### Group 86 - Runtime integration and review

{{MEDIUM}}
- [ ] 86.1 Use real runtime frames for animation/occlusion claims.
- [ ] 86.2 Check each launch center/module footprint as a complete site, not isolated tiles.
- [ ] 86.3 Verify correct vehicle hiding at bays, portals and docks, drawing on the correct layer and clickable moving bodies.


### Group 87 - Runtime integration and review

Goal finalization disposition, 2026-10-08: selected M1 technical acceptance is complete in85B and freshly verified in [the final art check](../../../../logs/r2mlvs3mp-z-b-final-art-verification.log). The user explicitly permits skipping human operator gates; personal feedback remains **deferred**, never recorded as approval. Later-family whole-profile gallery/feedback tasks stay unchecked, with target tube/geothermal interop art under GP-TG. This exception applies only to goal finalization, not full-profile art acceptance.

{{SIMPLE}}
- [ ] 87.1 Produce a compact contact sheet and in-game scenario gallery linked to manifest IDs.
- [ ] 87.2 Record technical acceptance and user art feedback separately; do not inherit prior plans' skipped art approval.


### Group 88 - Runtime integration and review

{{MEDIUM}}
- [ ] 88.1 Replace all milestone placeholders, document retained limitations and confirm distributed assets match the reviewed hashes.


### Group 89 - Runtime integration and review

{{SIMPLE}}
- [ ] 89.1 Capture all five tube models in motion/load/service states, geothermal power/direct-smelting/battery/forge states at their valid dates, TBM area-clearing bends and off-center cargo drops with train/lorry collection. Include overlapping area overlays, map edges and blocked/locked states in the runtime gallery.
- [ ] 89.2 Record the implemented public exports, focused test commands/results and remaining runtime/art/network gaps in this plan; retain original supplied saves and repository-local evidence.
- [ ] 89.3 Update affected architecture/handoff documentation to describe delivered behavior and reconcile the parent index only when this plan's Completion Conditions are met. Keep every unfinished task unchecked.

## Consult on Demand

| Trigger | File name | Start at | Expand only when | Consultation purpose |
| --- | --- | --- | --- | --- |
| Reviewed sprites are absent or a fallback GRF ships old artwork | [../../architecture/basesets/archi-basesets.md](../../architecture/basesets/archi-basesets.md) | `Graphics` | The manifest, generated package/hash and live sprite IDs match but runtime resolution differs | Locate extra-GRF/base-set resolution and missing-sprite fallback; a contact sheet is not proof the payload was distributed. |
| A required public export is absent and its owner/location is disputed | [r2mlvs-multilayer-vehicles-and-structures.md](r2mlvs-multilayer-vehicles-and-structures.md) | `Requirements and ownership` and `Plan Index` | The named producer confirms an intentional contract/ownership change | Find the responsible producer and reconcile its public handoff with direct consumers; the hub cannot substitute for missing implementation. |
| Current behavior contradicts a historical subsystem description | [../../architecture/map-layers/archi-map-layers.md](../../architecture/map-layers/archi-map-layers.md) | `Invariants` and the relevant named source in `Code map` | The live symbol and executable test do not explain the difference | Locate layer addressing or lifecycle rationale. Live code/tests govern delivered behavior; this architecture page does not prove future interfaces exist. |

## Planning Provenance

This is not an execution reading queue. Do not preload it after context compaction. It is non-transitive; ranges record planning use, not runtime authority.

| File name | Sections or Line Ranges used while planning | Planning contribution |
| --- | --- | --- |
| [docs/zz-reach2/architecture/initial-inventory.md](../../architecture/initial-inventory.md) | 185-220 | `Command as the only world mutation`, `Pool-indexed game objects`, `Tile map as a packed array` and `File-suffix modules` oriented command/pool/header ownership and source registration. This historical inventory is a location survey, not authority for current layer behavior. |
| [docs/zz-reach2/architecture/@archi-ttd-level0.md](../../architecture/@archi-ttd-level0.md) | 24-63 | `How the program fits together` and `Source and runtime boundaries` located shared simulation, data/presentation, CMake targets and subsystem handoffs used to choose the live reading sources. |
| [docs/zz-reach2/upgrades/2026-10/r2mlvs-multilayer-vehicles-and-structures.md](r2mlvs-multilayer-vehicles-and-structures.md) | 119-140 | `Requirements and ownership` assigned this plan's feature responsibilities; it shaped the decomposition without making every sibling plan a mandatory reading dependency. |
| [src/table/sprites.h](../../../../src/table/sprites.h) | 380-409 | The inspected live boundary helped separate characterization, public-contract production, implementation and acceptance batches. It materially constrained manifest-owned sprite identity/geometry and final runtime art evidence; this entry records planning influence rather than an additional reading obligation. |
| [User planning-format instructions](<C:/Users/Axonn/.codex/attachments/cd59b20a-3a96-4d54-b4bd-1e8f37639e7d/Pasted text.txt>) | `Global Mandatory Reading`, `Area Mandatory Reading`, `Consult on Demand`, `Planning Provenance`, `Final validation` | Supplies the exact protocol, chronological difficulty batches of at most eight tasks, reading gates and validation format. |
| [media/multilayer/tools/sprite_quantize.py](../../../../media/multilayer/tools/sprite_quantize.py) | 379-411 | Deeper planning pass, 2026-10-04: Stable packing and metadata emission shaped per-model originals, coverage validation and reproducible conversion. |

## Completion Conditions

Every required building, vehicle, resource and gameplay-critical state has packaged graphics, valid directions/anchors and a runtime capture. Repeatable conversion works from retained sources. Offline sheets, generated originals and a successful compiler run alone do not establish art acceptance.
