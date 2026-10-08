# R2MLVS13 - Graphics and Asset Pipeline

Owner: [R2MLVS parent](r2mlvs-multilayer-vehicles-and-structures.md). M1 dispatch: [R2MLVS3MP](r2mlvs3mp-master-plan.md). Architecture: [map layers](../../architecture/map-layers/archi-map-layers.md).

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

| File name | Line Ranges | Description |
| --- | --- | --- |
| [docs/zz-reach2/upgrades/2026-10/r2mlvs-multilayer-vehicles-and-structures.md](r2mlvs-multilayer-vehicles-and-structures.md) | 188-192 | `AD-1` owns the complete supplied content/art profile, the split between engine behavior and content definitions, and missing-required-content failure. Every asset group must package the selected profile consistently; live sprite constants and the manifest, not this AD, own geometry/offsets. |
| [src/map_layer.h](../../../../src/map_layer.h) | 78-85 | `MultiLayerMap` ownership preamble: `Map` owns packed storage/geometry and this controller owns layer semantics. Every group uses world tiles/explicit layer helpers; camera selection and ad hoc plane offsets cannot define gameplay. |
| [src/table/sprites.h](../../../../src/table/sprites.h) | 380-409 | Complete Map Layer sprite constants and `SPR_NEWGRFS_BASE` bound the published block; preserve 0..65 and resolve any accepted live extension before allocating new IDs. |

Status: Groups 1-8 M0 contract and the master's selected forty-family M1 source/packing/native-consumer/crossing/loop/sound/gallery slice are complete. Group 85B records selected technical gallery/distribution acceptance; unselected later families and full-profile gallery obligations remain pending. User personal art review is deferred as requested. Evidence: [sprite contract and geometry](r2mlvs13-a-sprite-contract.md) and [selected gallery acceptance](r2mlvs3mp-t-gallery-acceptance.md). Parent completion is unchanged. Final art is required for milestone acceptance.

## Outcome and Boundaries

Produce a coherent OpenTTD sprite set covering terrain, all specialist vehicles, functional buildings and important state changes.

## Asset families

| Family | Minimum content and state coverage | Consumers |
| --- | --- | --- |
| Underground terrain | Solid rock and cut face; excavated floor; edges/corners; progress overlay; two depth palettes; ten survey/resource markers | 2, 6 |
| Portals | Existing 4-direction housings retained; sealed/commissioning/connected faces; Deep rail variant; work entrance and exchange overlays | 3 |
| TBMs and rail haulage | Standard/deep cutter and support parts; drilling animation; Earth/Rock wagons empty/partial/full; hauler; 8 vehicle directions and purchase sprites | 4, 5, 7 |
| Underground facilities | Exchange bay, rail freight station/loading platform, road terminal, train/road service depot; 4 approach directions where applicable; back/front occlusion | 5, 7 |
| Mines and Surface receivers | 5 Underground + 5 Deep mine identities; construction/working/depleted states; metal/chemical/battery/electronics/component/nuclear/disposal/crusher functions as required by content | 6 |
| Underground road fleet | Mine truck and deep mineral carrier; 8 directions, refit/load states and purchase sprites | 7 |
| Transport tubes | Straight/45-degree bend/junction/end guideways; powered/unpowered states; terminal/service/interchange fronts and backs; excavation/route controls | 7, 16 |
| Original tube fleet | Mole-70, Crate-75, Cistern-80, Shield-85 and Forge-05: 5 distinct silhouettes/models, each with 8 directions, purchase icon and cargo/load states; 5 refits or recolors are insufficient | 16 |
| Geothermal sites and factories | Vent field/collector; Vent-Tap and Magma-Loop powerhouses; direct smelter, battery works and Mantle Forge; utility conduits/riser; construction/working/heat-starved states | 17 |
| Space Launch Center | Pad, assembly/service building, cargo/recovery areas; footprint rotations; ready/preparing/launch/occupied states; foreground cuts | 9 |
| Space fleet | Cargo rocket ascent/orbit/recovery; small probe free/attached/mining; return pod; heavy robot and orbital hauler; direction and purchase/state sprites | 9..11 |
| Asteroids | 3 readable classes, size variants, docking/mining overlays and depleted state; animation must preserve gameplay footprint | 10 |
| Orbital modules | Core, dock, storage, utility, assembly and processor; blueprint/partial/complete states, connectors and docking occlusion | 11 |
| Solar | Original solar farm and mirror kit icon; mirror packed/deploying/operating/idle; farm boost indicator | 12 |
| Controls and cargos | Underground/Space Construction menus, general TBM area/45-degree turn previews, tube/geothermal tools, 1980/later technology locks, launch-center collection and separate 20-tile orbital-drop overlays, vehicle commands, cargo/resource icons and blocked-state markers | all |

Exact sprite counts depend on footprints, rotations, load stages and animation frames. Freeze them in the first reading area and Sprite Contract Handoff before batch generation. The family table is a coverage obligation, not a claim that one image per row is enough.

## Public Handoff Contract

Proposed public owner: `docs/zz-reach2/upgrades/2026-10/assets/multilayer_sprite_manifest.json`. Publish `asset_id`, `consumer`, `directions`, `frames`, `anchor`, `source`. These symbols are producer obligations, not existing code. Keep one authority for the fields/commands they expose; downstream plans consume the public contract rather than private pools or copied tables.

`Sprite Contract Handoff`: the first reading area publishes exact family/direction/frame/footprint/anchor/foreground requirements and the public manifest before content freeze. Generation and full runtime gallery follow consumer implementation; no blanket human-acceptance skip is inherited.

## Implementation Tasks

Deepening pass, 2026-10-04: implementation groups use only SIMPLE, MEDIUM or HARD difficulty, with at most eight pending tasks each. Sketches describe proposed implementation contracts; verify the named earlier producers at each assumption gate. No gameplay or asset implementation is marked complete by this planning pass.

Groups are chronological and contain consecutive tasks of one difficulty. Each difficulty batch has at most eight tasks; area reading changes only when the implementation boundary or source of truth changes.

### Area Mandatory Reading - Art and runtime contract

_Applies to Groups 1-8._

**Entry gate.** The following public contracts must exist before dependent implementation in this area: `src/multilayer_content.h`: `MultiLayerGameplayProfile`, `LayerVehicleCapability`, `GetMultiLayerContentProfile`, `CheckDeepLayerAvailability`, `UtilityPowerAccount`.
Verify each public file/export/schema field with `rg` in the current workspace and inspect its command/field ownership. For a same-plan contract, compile its type/signature fixture or validate its artifact after the preceding producer groups. For cross-plan behavior, also run the producer's focused executable boundary test; declarations alone cannot authorize dependent mutations or runtime claims.
If a required export is absent, stop the dependent implementation, mutation and runtime claims in Groups 1-8. Read-only characterization, source/range verification, test-case design and asset-template work that does not assume the missing behavior may continue.
Do not bypass this gate by copying private pool/tile state, inventing local IDs or cargo registries, using a historical plan as a delivered implementation, opening user-wide content stores, substituting a second utility/accounting authority, or adding an unreviewed fallback path. Finish the named producer or revise the shared contract and its direct consumers first.

| File name | Line Ranges | Description |
| --- | --- | --- |
| [src/newgrf_act5.h](../../../../src/newgrf_act5.h) | 15-31 | `Action5BlockType`, `Action5Type`, `GetAction5Types` define the public block/replacement model; do not reinterpret existing offsets or GRF compatibility. |
| [docs/zz-reach2/upgrades/2026-10/tools/sprite_quantize.py](tools/sprite_quantize.py) | 29-46 | `CONTRACT_RANGES` ends at 65 and records an unactivated optional 66..73 Deep housing range in a comment; manifest planning must resolve this live extension instead of blindly starting at 66. |
| [src/window_gui.h](../../../../src/window_gui.h) | 799-805 | `Window::OnPlaceObject` receives the clicked world tile; construction uses that layer even from extra viewports. |
| `src/multilayer_content.h` (planned public contract; link after implementation) | Live definitions; no fixed range | [r2mlvs1-gameplay-contracts-and-content.md](r2mlvs1-gameplay-contracts-and-content.md) Groups 1-6, ending with Group 6's public-contract task, publish `MultiLayerGameplayProfile`, `LayerVehicleCapability`, `GetMultiLayerContentProfile`, `CheckDeepLayerAvailability`, `UtilityPowerAccount`. These exports/schema fields are authoritative for this area. Same-plan consumers follow those earlier groups; cross-plan consumers verify the producer's required implementation/tests as well. Proposed names alone do not prove delivery. |

**ASSUMPTION: this was already built previously. If not, re-asses at runtime**. This applies only to the producer contracts named in this area's gate, after their listed producer groups have run. Verify their actual exports and focused tests before using the sketches below; proposed types and helper names in sketches are not evidence of delivered code.

**Manifest decomposition.** First freeze schema and placement geometry, then fill exact family coverage, then publish the M0 contract. The family table is a checklist of gameplay states, not a sprite count or an invitation to generate one generic sheet for every vehicle.

```text
logical asset ID + direction + state + frame
    -> native dimensions / world anchor / foreground part
    -> immutable source + palette policy
    -> stable packed sprite reference + reviewed output hash
```

### Group 1 - Art and runtime contract

{{SIMPLE}}
- [X] 1.1 Inventory reusable existing assets and new requirements.

### Group 2 - Art and runtime contract

{{MEDIUM}}
- [X] 2.1 Preserve offsets 0..65 of the current layer block; append core terrain/UI assets or allocate a new documented block. Content-package sprites follow the package's stable IDs.

### Group 3 - Define a verifiable sprite manifest schema

{{HARD}}
- [X] 3.1 Define one manifest record per asset/state family with stable asset ID, consumer, direction/frame/state counts, native dimensions, footprint, anchor and bounds. Distinguish logical asset ID from packed sprite offset.
- [X] 3.2 Add transparency, company-color/animation palette policy, back/front parts, source/provenance and packed-output hash fields. Declare which fields are mandatory versus not applicable for icons/terrain/vehicles.
- [X] 3.3 Write schema/coverage examples for one vehicle and one multi-tile building; define uniqueness, complete direction/state coverage and missing-source errors before filling the catalogue.
- [X] 3.4 Resolve live sprite block size and any already-approved extension against code and packing inputs. Preserve offsets 0..65; never silently occupy a reserved/claimed later offset.

### Group 4 - Set measurable art rules and reference ownership

{{MEDIUM}}
- [X] 4.1 Publish a compact style sheet: isometric TTD scale, one light direction, readable native-resolution silhouettes, consistent ground contact/shadows and distinct depth/resource colors.
- [X] 4.2 Specify native-scale readability checks for empty/loaded/blocked/working states. Identify silhouette and accent features that distinguish each of the five tube models without relying only on color.
- [X] 4.3 List the reference assets, their actual reuse rights and intended use. Keep first-party/generated originals and pinned provenance separate from third-party ETI imagery with unresolved terms.

### Group 5 - Art and runtime contract

{{MEDIUM}}
- [X] 5.1 Freeze 8bpp DOS palette/index-0 transparency for the first deliverable, normal-zoom authored sprites and engine scaling.
- [X] 5.2 Specify handling of reserved animation/company colors; optional 32bpp/high-zoom art is later work.

### Group 6 - Validate representative geometry before family generation

{{HARD}}
- [X] 6.1 Prepare exact tile/vehicle placement templates from live sprite anchors and direction conventions. Label each view using the engine orientation, not a generic 2D rotation.
- [X] 6.2 Produce one representative TBM directional/boring test with fixed cutter/support alignment; compare straight and two legal 45-degree bend poses against its swept gameplay footprint.
- [X] 6.3 Produce one mine layout/state test and one rocket deployment/recovery test, checking anchor, bounding box, tile joins and native-size readability.
- [X] 6.4 Produce one module docking foreground/back test with a vehicle behind/in front. Validate with exact placement templates for M0; full runtime occlusion awaits the owning feature.
- [X] 6.5 Record accepted geometry and corrections for each representative before generating its family. A contact sheet validates schema/layout only, not runtime animation or occlusion.

### Group 7 - Complete all family coverage contracts

{{MEDIUM}}
- [X] 7.1 Expand the manifest to all ten mines, conventional underground fleet, portals/exchange/service facilities and Surface receiver functions from the frozen draft catalogue.
- [X] 7.2 Add distinct Mole-70, Crate-75, Cistern-80, Shield-85 and Forge-05 direction/load/service matrices plus tube guideway/junction/terminal visibility rules.
- [X] 7.3 Add geothermal collector, both powerhouses, smelter, battery works, Mantle Forge, utility/riser and heat-starved states at their dates.
- [X] 7.4 Add every Space fleet/module/asteroid/mirror state and separate Surface collection versus orbital-drop overlays; fill exact counts/anchors/foreground parts before batch authoring.

### Group 8 - Publish the Sprite Contract Handoff

{{MEDIUM}}
- [X] 8.1 Publish `assets/multilayer_sprite_manifest.json` with exact `asset_id`, `consumer`, `directions`, `frames`, `anchor`, `source` and the additional validated schema fields. Keep it repository-local and link from consumer plans.
- [X] 8.2 Run schema, duplicate-ID, coverage-count and source/geometry-template checks. Mark planned source assets as undelivered explicitly rather than pointing to nonexistent final files.
- [X] 8.3 Record the M0 Sprite Contract Handoff as dimensions/directions/state requirements plus representative template evidence. Actual family generation, packing and runtime acceptance remain pending in later groups.

### Area Mandatory Reading - Source generation and reproducible conversion

_Applies to Groups 9-81._

**Entry gate.** The following public contracts must exist before dependent implementation in this area: `docs/zz-reach2/upgrades/2026-10/assets/multilayer_sprite_manifest.json`: `asset_id`, `consumer`, `directions`, `frames`, `anchor`, `source`.
Verify each public file/export/schema field with `rg` in the current workspace and inspect its command/field ownership. For a same-plan contract, compile its type/signature fixture or validate its artifact after the preceding producer groups. For cross-plan behavior, also run the producer's focused executable boundary test; declarations alone cannot authorize dependent mutations or runtime claims.
If a required export is absent, stop the dependent implementation, mutation and runtime claims in Groups 9-81. Read-only characterization, source/range verification, test-case design and asset-template work that does not assume the missing behavior may continue.
Do not bypass this gate by copying private pool/tile state, inventing local IDs or cargo registries, using a historical plan as a delivered implementation, opening user-wide content stores, substituting a second utility/accounting authority, or adding an unreviewed fallback path. Finish the named producer or revise the shared contract and its direct consumers first.

**Image-generation gate.** Verify that the execution harness exposes the required image-generation tool and read the installed imagegen skill before generating/editing raster art. If unavailable, stop only raster-generation/edit tasks in this area; manifest checks, placement templates, palette/anchor validation and packing-tool work may continue. Do not substitute unauthorized providers, private caches or a different image-editing route to bypass the skill/tool prerequisite.

| File name | Line Ranges | Description |
| --- | --- | --- |
| [docs/zz-reach2/upgrades/2026-10/tools/sprite_quantize.py](tools/sprite_quantize.py) | 379-411 | `build_sheet` and `sprite_lines` implement deterministic native packing/anchors and NFO emission; preserve the repeatable source-to-payload contract. |
| [media/baseset/openttd/CMakeLists.txt](../../../../media/baseset/openttd/CMakeLists.txt) | 6-67 | Complete `if(GRFCODEC_FOUND)` and `create_grf_command` block defines PNG/NFO generation inputs; validate the separate checked-in fallback packaging path too. |
| [media/baseset/CMakeLists.txt](../../../../media/baseset/CMakeLists.txt) | 16-34 | Complete payload list and conditional generated-payload marking retain the checked-in `openttd.grf` path when the generator is unavailable. |
| `docs/zz-reach2/upgrades/2026-10/assets/multilayer_sprite_manifest.json` (planned public contract; link after implementation) | Live definitions; no fixed range | [r2mlvs13-graphics-and-asset-pipeline.md](r2mlvs13-graphics-and-asset-pipeline.md) Groups 1-8, ending with Group 8's public-contract task, publish `asset_id`, `consumer`, `directions`, `frames`, `anchor`, `source`. These exports/schema fields are authoritative for this area. Same-plan consumers follow those earlier groups; cross-plan consumers verify the producer's required implementation/tests as well. Proposed names alone do not prove delivery. |

**ASSUMPTION: this was already built previously. If not, re-asses at runtime**. This applies only to the producer contracts named in this area's gate, after their listed producer groups have run. Verify their actual exports and focused tests before using the sketches below; proposed types and helper names in sketches are not evidence of delivered code.

**Conversion sketch.** Asset generation selects original pixels; deterministic processing begins from those retained originals. A later runtime correction updates source/manifest and reruns conversion, never patches only the distributed binary.

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

### Group 9 - Prepare controlled asset authoring and originals

Source-art pass, 2026-10-06: [Plans 1-5 artwork record](multilayer-m1-art-pass.md)
retains 14 built-in-generated originals and 17 indexed atlases (247 staged
poses/previews), with exact prompts, hashes and native previews. The public
manifest now links those authored/partial sources. Only Earth/Rock cargo icons
have complete source coverage; vehicle animation and building geometry/slicing
remain unfinished. Runtime packing/integration and all full-plan art acceptance
remain pending. This bounded pass does not complete these global checkboxes.

M1 preparation, 2026-10-06: [selected source/geometry record](r2mlvs13-b-m1-authoring.md) verifies the available built-in tool/installed skill, all 14 immutable original hashes and existing atlas checks, and publishes 28 bounded M1 briefs. Native consumer geometry passes 47 assertions: both cutters need distinct permanent support art; one-tile depot families, 2x3 Earth and 4x4 Rock replace unsupported draft rectangles. Public manifest/publisher now require 57,163 sprites across the same 141 families while preserving original provenance, protected offsets and payload identity. The user explicitly authorized built-in image outputs in CODEX_HOME on 2026-10-07, with selected originals copied into this repository. These preparation results do not mark missing directions/animation/per-cell art or the whole global authoring tasks complete.

{{MEDIUM}}- [X] 9.1 For the selected M1 slice, read/use the available image-generation skill/tool for AI raster work and follow its editing/reference requirements. Keep prompts, references, chosen originals and provenance in repository asset directories; this preparation generates no art.
- [X] 9.2 Use the approved geometry/style templates to write one bounded generation brief per selected M1 family/model: view labels, light direction, silhouette, material accents, state differences, transparent background and native target footprint.
- [X] 9.3 Keep selected M1 original outputs immutable and record selection/revision history separately from deterministic conversion. Generative output is not assumed reproducible from a seed; checked-in originals make subsequent packing reproducible.

Selected M1 preparation closure (2026-10-07): 28 briefs, all 14 retained original hashes, the public 141-family schema and actual native geometry are verified in [the scoped authoring record](r2mlvs13-b-m1-authoring.md). This closes master 8.1; source pixel coverage and final runtime review remain with their later owning groups.

### Local Mandatory Reading - Later-family authoring preparation

_Applies to Group 9B when dispatched by a later milestone; master M1 does not dispatch it._

Read this plan's Global and Source generation Area, then only the nonselected manifest families and their actual consumer geometry. Use the installed imagegen skill, repository provenance and original immutability rules. Do not re-author already selected M1 sources.

### Group 9B - Prepare remaining profile asset families

{{MEDIUM}}
- [ ] 9B.1 Apply the image-generation skill/tool and repository provenance workflow to the remaining profile families when their actual consumer contracts are ready.
- [ ] 9B.2 Publish bounded approved geometry/style briefs for each remaining family/model, preserving the original whole-profile authoring requirement.
- [ ] 9B.3 Retain immutable selected originals and independent revision history for those families before deterministic conversion.

### Local Mandatory Reading - M1 terrain originals

_Applies to Groups 10-11; supplements the Source generation Area._

Read the active terrain brief and manifest, retained terrain original/provenance, geometry template's flat diamond, and actual excavation rendering's state/neighbor/survey mapping. Use the approved tool for new raster pixels; deterministic crop/palette conversion follows selection. Ground masks must follow world neighbors and fixed anchors, with no camera-driven simulation changes.

### Group 10 - Author Underground terrain

{{HARD}}
- [X] 10.1 For Underground terrain, derive a single-model brief from its approved manifest/template: solid face versus cavity with readable edges and survey markers. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [X] 10.2 Produce the required directions/rotations for Underground terrain from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [X] 10.3 Add all cut/progress/edge/corner and five resource-marker states for Underground terrain, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.

Group 10 source handoff (2026-10-07): the retained 12 base-state/texture poses are selected at native 64x31 / anchor (-31,0). Terrain uses one fixed engine camera, so no rotated camera views are required. Two fresh built-in originals add all 16 neighbor-rim combinations and five distinct resource markers in unknown/surveyed/depleted states. [Prompts, references, immutable cache/copy hashes and selection](assets/mlvs/terrain-generation-record.json), [native 1x/4x sheet](assets/mlvs/terrain-details-review.png), and `convert_terrain_art.py --check` prove deterministic alpha/palette/coverage/bounds for 31 supplemental poses. Source art is handed to later packing; runtime and user acceptance remain pending. Two explicit edge-mask families append planned offsets 168..199 after all original planned offsets, preserving live 0..65 and reserved 66..73; the public contract now has 143 families / 57,195 required source sprites. The selected M1 briefs expand from 28 to 40 to include the two edge families and ten resource markers already required by terrain tasks.

### Group 11 - Author Deep terrain

{{HARD}}
- [X] 11.1 For Deep terrain, derive a single-model brief from its approved manifest/template: distinct basalt palette with the same cavity grammar. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [X] 11.2 Produce the required directions/rotations for Deep terrain from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [X] 11.3 Add all cut/progress/edge/corner and five Deep resource-marker states for Deep terrain, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.

Group 11 source handoff (2026-10-07): retained native basalt base/texture states are selected; terrain has a fixed camera. Two fresh built-in originals supply all 16 Deep neighbor rims and LITH/NORE/COBT/RARE/URAN markers in three disclosure states. The shared [provenance record](assets/mlvs/terrain-generation-record.json) and [native 1x/4x review](assets/mlvs/terrain-details-review.png) retain all prompts, reference/source hashes and selections. Focused conversion validates 62 supplemental poses across both depths; actual packaging/rendering stays with Groups 79-83.

### Local Mandatory Reading - M1 portal source geometry

_Applies to Groups 12-13; supplements the Source generation Area._

Read the active portal brief/manifest, immutable portal original/provenance, `LayerPortalWorkState` in layer_portal_cmd.h, `DrawTile_LayerPortal` and its separator bounds in layer_portal_cmd.cpp, and the established portal front-mask conversion. Four actual door directions share a fixed world anchor. Surface/Underground or Underground/Deep endpoint variants are independent one-tile housings on distinct layers. Resolve the draft generic state aliases against actual Sealed/Commissioning/Connected before authoring; keep blocked status a presentation of the existing controller, never a new saved work phase. Far-facing roofs/walls must occlude vehicles; native runtime acceptance remains in later integration groups.

### Group 12 - Author sealed and commissioning portal overlays

{{HARD}}
- [X] 12.1 For sealed and commissioning portal overlays, derive a single-model brief from its approved manifest/template: existing four-direction housings with controlled work faces. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [X] 12.2 Produce the required directions/rotations for sealed and commissioning portal overlays from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [X] 12.3 Add Sealed/Commissioning/Connected fronts and backs for sealed and commissioning portal overlays, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.

Group 12 source handoff (2026-10-07): [selected sibling v2 and provenance](assets/mlvs/portal-generation-record.json) provide true four-direction housings in actual Sealed/Commissioning/Connected phases, independently for Surface/Underground endpoint tiles. The public publisher and briefs now match the static phase owner (one frame per phase); 123 focused owner assertions pass in [g](r2mlvs13-g-portal-phase-contract.log). This replaces unsupported generic four-frame/four-state portal drafting, reducing total source requirements to 143 families / 56,779 while preserving all live/reserved/planned native offsets. Deterministic native 64x64 / (-31,-33) conversion stages 48 back/front parts with full far-roof coverage and explicit source-reviewed near apertures. [Native sheet](assets/mlvs/portal-details-review.png) verifies door/state readability and aligned cuts; actual train sorting, packed loading and final acceptance remain with later groups. Original v1 pixels and atlases stay immutable.

### Group 13 - Author Deep rail portal variant

{{HARD}}
- [X] 13.1 For Deep rail portal variant, derive a single-model brief from its approved manifest/template: rail-compatible deep housing aligned to the shared shaft geometry. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [X] 13.2 Produce the required directions/rotations for Deep rail portal variant from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [X] 13.3 Add all four approach directions and work states for Deep rail portal variant, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.

### Local Mandatory Reading - M1 moving machinery originals

_Applies to Groups 14, 14D and 15-18; supplements the Source generation Area._

Read the executing model's public brief/manifest and immutable sources, engine vectors in the approved SVG and its raster guide, `TBMConsistRole`/`TBMPhase`/`TBMBlockedReason` in tbm.h, and actual native rail/chassis/parts in tbm_engine.cpp and articulated_vehicles.cpp as relevant to the current model. The cutter and permanent support are separate physical train parts; the support never receives a second cutter head. Reversal follows ordinary train orientation/backing; never turn an idle cutter into an advancing backward bore. Preserve fixed articulated contact points through all eight genuine views. Animation must use authored moving pixels and remain display-only, with no cargo/terrain/controller mutations. Service and reversing views required by the source tasks must be declared explicitly before final coverage.

### Group 14 - Author Standard TBM

{{HARD}}
- [X] 14.1 For Standard TBM, derive a single-model brief from its approved manifest/template: cutter/support proportions and legal 45-degree heading continuity. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [X] 14.2 Produce the required directions/rotations for Standard TBM from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [X] 14.3 Add idle/boring/reversing/service frames with fixed articulated seams for Standard TBM, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.

Group 14 base-source handoff (2026-10-07): [selected body v2/support v3](assets/mlvs/vehicle-generation-record.json) have eight genuine engine views each, with source-reviewed per-view crop registration and native bounds/anchors. The support is a low generator/hose cart with no cab or second cutter. [Native partial sheet](assets/mlvs/vehicle-details-review.png) stages 16 idle frame-zero poses. Two rejected candidates are retained with exact reasons/prompts/hashes and never fill coverage. The publisher/briefs now explicitly include backing and ordinary service for both cutters, and service for the hauler, as required by these source tasks (143 families / 57,051 required source sprites). Art animation creates no new saved work phase. Task 14.3 remains pending until the bounded body/support batches and 14D combined source review pass; 368 Standard poses are still missing. Actual loaded seam/occlusion review remains required during integration.

### Local Mandatory Reading - Standard cutter body animation

_Applies to Group 14B; supplements Source generation Area._

Read vehicle.model-00 body states/frames/anchor in the public manifest and selected brief, selected idle body source/crop registration in vehicle-generation-record.json, and the sparse converter's real coverage/missing list. Use the viewed body original as the built-in tool's edit/reference target; preserve chassis/heading/contact points. Each state batch retains all eight true views and four genuinely authored frames. Idle/blocked/service never imply boring; reversing keeps the actual physical heading. No duplicate frame padding. Stop only the affected asset if a generated sheet misses a view/state/frame; preserve rejected provenance and iterate.

### Group 14B - Author Standard cutter body states and frames

{{HARD}}
- [X] 14B.1 Author/select idle body frames 1-3 against the retained eight-view frame 0, preserving static chassis and declaring the animated equipment cue.
- [X] 14B.2 Author/select moving body frames 0-3 with actual wheel/rod cues, preserving cutter-stop and articulated contacts.
- [X] 14B.3 Author/select boring body frames 0-3 with genuine cutter phase/debris changes inside fixed bounds.
- [X] 14B.4 Author/select blocked body frames 0-3 with clear resumable wait cues and stopped cutter.
- [X] 14B.5 Author/select backing body frames 0-3 without turning the machine or presenting backward excavation.
- [X] 14B.6 Author/select ordinary service body frames 0-3 with fixed housing, distinct service cue and no depot/cargo invention.

14B.1 source handoff (2026-10-07): four genuinely authored RGB equipment-cue phases now exist in all eight engine views. The tool's partial sheet contributes complete frames 1/2 only; the excluded partial row never fills a frame. A separate original supplies frame 3. Deterministic source-part assembly uses only selected cab-beacon regions onto the immutable idle chassis, as 14.3 requires changing declared parts only. Native checks independently prove identical alpha/contact geometry and exact pixels outside that cue, plus four distinct palette phases in every view. [Prompt/selection/crop/component provenance](assets/mlvs/vehicle-generation-record.json), [animation source review](assets/mlvs/standard-idle-animation-review.gif), and [focused checks](r2mlvs13-i-standard-idle-source-checks.log) retain evidence. Current Standard coverage is 40/384 (32 body idle plus 8 support idle); moving/boring/blocked/backing/service and support animations remain pending. This is source review, not a loaded gameplay capture.

Group 14B source handoff (2026-10-07): the retained chassis and eight genuine headings are reused for all **192 body poses**. Five new immutable tool outputs supply selected rolling gear, drilling teeth/work indicator, red wait and cyan service cues; the first movement sheet has 28 real cells and a separate four-cell source completes frame 3. Backing reuses the real rolling phases in inverse order without rotating the body. Explicit per-view source/target equipment registration excludes generated lamp/chassis drift and preserves the original alpha/couplers. [Focused checks](../../../../logs/r2mlvs13-j-standard-body-source-checks.log) prove four distinct native palette phases in every heading/state, exact regeneration and the public manifest. Source-only GIF reviews are linked through [provenance](assets/mlvs/vehicle-generation-record.json). Support coverage, combined model handoff, packed loading and runtime review remain later tasks. The user deferred art feedback until integration on 2026-10-07; this does not claim that review passed.

### Local Mandatory Reading - Standard permanent support animation

_Applies to Group 14C; supplements Source generation Area._

Read vehicle.model-00 support states/frames/anchor, selected support v3 and its crop registration, TBMConsistRole::PermanentSupport and actual native articulated prefix, then sparse source coverage. Animate existing machinery/wheels/status cues only. The same real cart stays permanently behind the cutter; no second head, cab, detachable cargo or new quantity owner. Retain all eight views and exact state/frame identities separately from body sources.

### Group 14C - Author Standard permanent support states and frames

{{HARD}}
- [X] 14C.1 Author/select idle support frames 1-3 against the retained eight-view frame 0.
- [X] 14C.2 Author/select moving support frames 0-3 with wheel/rod cues at fixed coupler contacts.
- [X] 14C.3 Author/select boring support frames 0-3 with genuine machinery/hose-reel changes and no cutter head.
- [X] 14C.4 Author/select blocked support frames 0-3 with distinct wait cue and unchanged physical machinery.
- [X] 14C.5 Author/select backing support frames 0-3 with the same real forward/rear couplers.
- [X] 14C.6 Author/select service support frames 0-3 with declared equipment panels/cues, no cargo or detached cart.

Group 14C source handoff (2026-10-07): **192 support poses** reuse the selected generator/hose cart and shared tool-authored indicator components. Two new immutable sheets supply selected wheel and hose-reel machinery phases; explicit crop rectangles handle uneven/touching generated cells while rejecting missing/overlapping/out-of-bounds registrations. Every source frame preserves the original own-part chassis, alpha and contacts, and backing reuses rolling phases in inverse order. [Combined focused checks](../../../../logs/r2mlvs13-k-standard-model-source-checks.log) pass all six states/four native phases/eight headings for both physical parts. Source-only native/enlarged GIF reviews and exact crop/component provenance are retained; actual loaded coupling remains with runtime integration.

### Local Mandatory Reading - Standard combined source review

_Applies to Group 14D; supplements M1 moving machinery Local above Group 14._

Read only the selected Standard body/support records and full 384-pose coverage, public native anchor, real Train::GetImage orientation handling and actual articulated parts. Compare neighboring headings/state/frame contacts and retained immutable hashes. Source review does not replace later packed/native runtime rendering or user acceptance.

### Group 14D - Review the complete Standard source model

{{MEDIUM}}
- [X] 14D.1 Verify all 384 declared body/support poses, genuine animation pixels and deterministic palette/alpha/crop/anchor conversion; refuse missing or duplicated coverage.
- [X] 14D.2 Review the combined native sheet for eight-view continuity, 45-degree neighbors, static chassis/contacts and readable body/support/work/backing/service differences.
- [X] 14D.3 Publish the completed source handoff and check original 14.3 only after 14B/14C/14D checks pass; keep actual packed train seam/occlusion/runtime acceptance with the owning later groups.

Group 14D source closure (2026-10-07): [native/enlarged sheet](assets/mlvs/vehicle-details-review.png) and six state GIFs per part retain all **384/384 Standard poses**, with explicit source hash/cell/component ownership in [provenance](assets/mlvs/vehicle-generation-record.json). Body/support remain visually distinct; original eight-view geometry is unchanged outside declared equipment, including adjacent 45-degree headings. Missing coverage is zero and the public authoring record is complete-source/runtime-pending. [Technical checks](../../../../logs/r2mlvs13-k-standard-model-source-checks.log) pass. User feedback is deferred until integration by instruction; packed loading and physical articulated seam/occlusion review are not claimed. This closes original 14.3, while master 8.3 still requires Deep, Earth/Rock and hauler source groups.

### Group 15 - Author Deep-rated TBM

{{HARD}}
- [X] 15.1 For Deep-rated TBM, derive a single-model brief from its approved manifest/template: visibly heavier cutter and depth equipment. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [X] 15.2 Produce the required directions/rotations for Deep-rated TBM from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [X] 15.3 Add idle/boring/reversing/service frames with fixed articulated seams for Deep-rated TBM, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.

Group 15 source handoff (2026-10-07): all **384 Deep body/support poses** are baked. The retained red/graphite cutter and its genuine corrected rear views are reused from the immutable original/native source chain; transparent canvas translation aligns the existing poses to the common ground contact without repainting their body. One new tool-authored support original reuses the accepted cart geometry with reinforced Deep material cues and no cab/cutter. Shared authored steel, machinery and indicator phases are registered only inside the Deep model's own physical equipment; its heavier shell, heading, alpha and contacts stay fixed. [Body checks](../../../../logs/r2mlvs13-l-deep-body-source-checks.log), [support checks](../../../../logs/r2mlvs13-l-deep-support-source-checks.log), [provenance](assets/mlvs/vehicle-generation-record.json), and per-part/state source GIFs retain exact source/cell/component identities and four distinct native phases in every heading. Public source coverage is complete; packed/native coupling/occlusion review remains later. User art feedback is deferred by instruction.

### Group 16 - Author Earth hopper

{{HARD}}
- [X] 16.1 For Earth hopper, derive a single-model brief from its approved manifest/template: readable soil load and detachable wagon identity. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [X] 16.2 Produce the required directions/rotations for Earth hopper from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [X] 16.3 Add empty/partial/full load and purchase views for Earth hopper, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.

Group 16 source handoff (2026-10-07): **80 Earth poses** reuse the selected brown hopper, original true-axis view mapping and five retained load/status first poses. The second frame selects existing authored steel wheel details at explicit own-state positions; soil/load pixels, alpha and couplers stay exact. Runtime must use rolling phases only during actual movement, keep frame zero while stopped, and give actual load quantity priority over optional empty-service/wait views. [Focused checks](../../../../logs/r2mlvs13-m-earth-source-checks.log) pass five states/two native phases/eight headings and exact conversion/crop refusals. Available purchase art is reused; a single five-model tool original supplies separate locked UI previews with padlocks, never vehicle bodies. [Purchase provenance/review](assets/mlvs/purchase-generation-record.json) and [checks](../../../../logs/r2mlvs13-n-purchase-source-checks.log) prove ten native available/locked previews and manifest coverage. Full native/loaded cargo sorting and runtime art review remain later; user feedback is deferred.

### Group 17 - Author Rock hopper

{{HARD}}
- [X] 17.1 For Rock hopper, derive a single-model brief from its approved manifest/template: coarse rock load distinct from Earth. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [X] 17.2 Produce the required directions/rotations for Rock hopper from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [X] 17.3 Add empty/partial/full load and purchase views for Rock hopper, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.

Group 17 source handoff (2026-10-07): all **80 Rock poses** reuse the selected blue/steel hopper and real coarse rubble quantities, with common authored steel wheel phases registered on each own-state frame-zero chassis. [Focused checks](../../../../logs/r2mlvs13-o-rock-source-checks.log) pass all five states/two native phases/eight headings, unchanged load/chassis/alpha outside declared gear and exact regeneration. Available/locked purchase previews pass the shared [UI source checks](../../../../logs/r2mlvs13-n-purchase-source-checks.log). The same runtime policy as Earth applies: actual quantities choose load views, rolling phases require real movement, and empty-service/wait art cannot hide carried cargo. Native sheets/GIFs retain technical source review; final packed/load/service/coupling review remains later.

### Group 18 - Author spoil hauler locomotive

{{HARD}}
- [X] 18.1 For spoil hauler locomotive, derive a single-model brief from its approved manifest/template: secondary hauling engine distinct from the cutter. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [X] 18.2 Produce the required directions/rotations for spoil hauler locomotive from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [X] 18.3 Add running/idle/service and purchase views for spoil hauler locomotive, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.

Group 18 source handoff (2026-10-07): all **80 hauler poses** reuse the selected plain orange/gray locomotive and eight genuine headings. The actual native model 4 is the ordinary Singlehead diesel in `src/underground_vehicle.cpp`, eligible for Plan 5's separate non-cutter haulage assignment. Existing travel/exchange/wait first poses are retained; selected common steel/indicator phases provide missing animation and both cyan service poses on the own-body reference. No cutter disk, debris, articulated support or invisible cargo buffer enters this family. [Focused checks](../../../../logs/r2mlvs13-p-hauler-source-checks.log) pass five states/two native phases/eight headings, exact source/public-manifest regeneration, fixed chassis/alpha and malformed crop refusals. Shared available/locked previews pass [UI checks](../../../../logs/r2mlvs13-n-purchase-source-checks.log). All vehicle source groups in master 8.3 now close: **1,008 vehicle/support poses plus ten purchase UI poses**. Packing, actual model callbacks/sounds and native rendered acceptance retain their later gates.

### Group 19 - Author underground freight locomotive

Goal-authorized skip (2026-10-08, master finalization): dispatched expansion work in this group is skipped under [finalization exception GP-TG](r2mlvs3mp-z-a-finalization-ledger.md#gp-tg-tube-and-geothermal-expansion-requires-major-gap-planning). Existing delivered prefixes stay complete; unimplemented profile work remains pending. No skipped task is markedX.

{{HARD}}
- [ ] 19.1 For underground freight locomotive, derive a single-model brief from its approved manifest/template: conventional mineral-freight traction silhouette. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [ ] 19.2 Produce the required directions/rotations for underground freight locomotive from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [ ] 19.3 Add running/idle/service and purchase views for underground freight locomotive, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.

### Group 20 - Author mineral wagon

Goal-authorized skip (2026-10-08, master finalization): dispatched expansion work in this group is skipped under [finalization exception GP-TG](r2mlvs3mp-z-a-finalization-ledger.md#gp-tg-tube-and-geothermal-expansion-requires-major-gap-planning). Existing delivered prefixes stay complete; unimplemented profile work remains pending. No skipped task is markedX.

{{HARD}}
- [ ] 20.1 For mineral wagon, derive a single-model brief from its approved manifest/template: profile-compatible ore load presentation. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [ ] 20.2 Produce the required directions/rotations for mineral wagon from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [ ] 20.3 Add each required refit/load and purchase view for mineral wagon, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.

### Group 21 - Author mine truck

Goal-authorized skip (2026-10-08, master finalization): dispatched expansion work in this group is skipped under [finalization exception GP-TG](r2mlvs3mp-z-a-finalization-ledger.md#gp-tg-tube-and-geothermal-expansion-requires-major-gap-planning). Existing delivered prefixes stay complete; unimplemented profile work remains pending. No skipped task is markedX.

{{HARD}}
- [ ] 21.1 For mine truck, derive a single-model brief from its approved manifest/template: compact underground road freight body. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [ ] 21.2 Produce the required directions/rotations for mine truck from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [ ] 21.3 Add required load/refit/service and purchase views for mine truck, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.

### Group 22 - Author sealed mineral carrier

Goal-authorized skip (2026-10-08, master finalization): dispatched expansion work in this group is skipped under [finalization exception GP-TG](r2mlvs3mp-z-a-finalization-ledger.md#gp-tg-tube-and-geothermal-expansion-requires-major-gap-planning). Existing delivered prefixes stay complete; unimplemented profile work remains pending. No skipped task is markedX.

{{HARD}}
- [ ] 22.1 For sealed mineral carrier, derive a single-model brief from its approved manifest/template: deep-capable closed cargo body. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [ ] 22.2 Produce the required directions/rotations for sealed mineral carrier from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [ ] 22.3 Add required sealed-load/service and purchase views for sealed mineral carrier, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.

### Local Mandatory Reading - M1 exchange source

_Applies to Group 23; master checkpoint 8.4._

Read this plan's Global and Source generation Area, the `facility.exchange` public manifest/brief, and the retained `exchange-bay-v1.png` source/provenance. In native source inspect `ResolveTBMExchangeBay` / `CheckTBMExchangeParticipant` in `src/tbm_exchange.cpp` and the depot branch of `DrawTile_Rail` in `src/rail_cmd.cpp`: this is one owned depot tile, with real consists hidden after entering, rather than the old candidate's 3x2 plaza. Keep four actual entry views and aligned back/front parts. Reuse the original steel/hoist/safety palette and preserve its bytes; author compact geometry and genuine presentation phases where missing. Housing/status animation never transfers cargo or grants exchange permission.

### Group 23 - Author TBM exchange bay

{{HARD}}
- [X] 23.1 For TBM exchange bay, derive a single-model brief from its approved manifest/template: stopped-consist exchange space with readable approaches. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [X] 23.2 Produce the required directions/rotations for TBM exchange bay from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [X] 23.3 Add occupied/available and directional back/front parts for TBM exchange bay, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.

23 source closure (2026-10-07): selected compact one-tile source reuses the original steel/hoist/safety design; the first long-plaza revision remains a rejected immutable candidate. Four actual entry views, four state cues and four authored indicator phases yield **128 aligned back/front sprites**. [Geometry boundary](../../../../logs/r2mlvs13-r-facility-geometry.log) passes 47 assertions; [focused source/placement/schema checks](../../../../logs/r2mlvs13-s-exchange-source-checks.log) pass. Originals/prompts are in [facility provenance](assets/mlvs/facility-generation-record.json); review is `assets/mlvs/exchange-bay-details-review.png` plus four per-state GIFs. Native payload/rendering and user art feedback remain later checkpoints.

### Group 24 - Author underground rail freight station

Goal-authorized skip (2026-10-08, master finalization): dispatched expansion work in this group is skipped under [finalization exception GP-TG](r2mlvs3mp-z-a-finalization-ledger.md#gp-tg-tube-and-geothermal-expansion-requires-major-gap-planning). Existing delivered prefixes stay complete; unimplemented profile work remains pending. No skipped task is markedX.

{{HARD}}
- [ ] 24.1 For underground rail freight station, derive a single-model brief from its approved manifest/template: loading platform and mine loader aligned to rail. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [ ] 24.2 Produce the required directions/rotations for underground rail freight station from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [ ] 24.3 Add all approaches/loading and back/front parts for underground rail freight station, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.

### Group 25 - Author road mine terminal

Goal-authorized skip (2026-10-08, master finalization): dispatched expansion work in this group is skipped under [finalization exception GP-TG](r2mlvs3mp-z-a-finalization-ledger.md#gp-tg-tube-and-geothermal-expansion-requires-major-gap-planning). Existing delivered prefixes stay complete; unimplemented profile work remains pending. No skipped task is markedX.

{{HARD}}
- [ ] 25.1 For road mine terminal, derive a single-model brief from its approved manifest/template: truck loading and turning access. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [ ] 25.2 Produce the required directions/rotations for road mine terminal from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [ ] 25.3 Add all approaches/loading and back/front parts for road mine terminal, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.

### Local Mandatory Reading - M1 ordinary service sources

_Applies to Group 26; master checkpoint 8.4._

Read this plan's Global and Source generation Area and only `facility.rail-service` / `facility.road-service` manifest/brief rows. Inspect native depot branches in `DrawTile_Rail` and `DrawTile_Road`, their one-tile build commands, and `src/underground_vehicle.h` for ordinary servicing ownership. Read the completed exchange source record/converter for reusable geometry and phase components. Rail has a single track entry; road has a distinct paved entry. Occupancy comes from real depot vehicles; visual service cues cannot create a second repair or vehicle registry.

### Group 26 - Author underground service depots

{{HARD}}
- [X] 26.1 For underground service depots, derive a single-model brief from its approved manifest/template: distinct rail and road service entry templates. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [X] 26.2 Produce the required directions/rotations for underground service depots from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [X] 26.3 Add open/occupied/service and required approach views for underground service depots, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.

26 source closure (2026-10-07): ordinary rail servicing reuses the exact compact rail original because the exchange owner assigns an existing native depot. The road family reuses its architecture with distinct teal fittings, asphalt/white entry stripe, no rails, and an internal service jack. Four actual entries and fixed construction/open/occupied-service/blocked views plus shared authored lamp phases supply **128 poses per family / 256 total**. [Focused service placement/phase/refusal checks](../../../../logs/r2mlvs13-t-service-source-checks.log) pass. Provenance and per-family PNG/four GIF reviews live beside the exchange originals; no new vehicle or repair authority is introduced. Native payload/rendering and personal art review remain pending.

### Group 27 - Author Coal mine

Goal-authorized skip (2026-10-08, master finalization): dispatched expansion work in this group is skipped under [finalization exception GP-TG](r2mlvs3mp-z-a-finalization-ledger.md#gp-tg-tube-and-geothermal-expansion-requires-major-gap-planning). Existing delivered prefixes stay complete; unimplemented profile work remains pending. No skipped task is markedX.

{{HARD}}
- [ ] 27.1 For Coal mine, derive a single-model brief from its approved manifest/template: Underground machinery/loading silhouette with Coal identity. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [ ] 27.2 Produce the required directions/rotations for Coal mine from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [ ] 27.3 Add construction/working/paused/depleted layout states for Coal mine, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.

### Group 28 - Author Iron mine

Goal-authorized skip (2026-10-08, master finalization): dispatched expansion work in this group is skipped under [finalization exception GP-TG](r2mlvs3mp-z-a-finalization-ledger.md#gp-tg-tube-and-geothermal-expansion-requires-major-gap-planning). Existing delivered prefixes stay complete; unimplemented profile work remains pending. No skipped task is markedX.

{{HARD}}
- [ ] 28.1 For Iron mine, derive a single-model brief from its approved manifest/template: Underground machinery/loading silhouette with Iron identity. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [ ] 28.2 Produce the required directions/rotations for Iron mine from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [ ] 28.3 Add construction/working/paused/depleted layout states for Iron mine, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.

### Group 29 - Author Copper mine

Goal-authorized skip (2026-10-08, master finalization): dispatched expansion work in this group is skipped under [finalization exception GP-TG](r2mlvs3mp-z-a-finalization-ledger.md#gp-tg-tube-and-geothermal-expansion-requires-major-gap-planning). Existing delivered prefixes stay complete; unimplemented profile work remains pending. No skipped task is markedX.

{{HARD}}
- [ ] 29.1 For Copper mine, derive a single-model brief from its approved manifest/template: Underground machinery/loading silhouette with Copper identity. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [ ] 29.2 Produce the required directions/rotations for Copper mine from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [ ] 29.3 Add construction/working/paused/depleted layout states for Copper mine, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.

### Group 30 - Author Bauxite mine

Goal-authorized skip (2026-10-08, master finalization): dispatched expansion work in this group is skipped under [finalization exception GP-TG](r2mlvs3mp-z-a-finalization-ledger.md#gp-tg-tube-and-geothermal-expansion-requires-major-gap-planning). Existing delivered prefixes stay complete; unimplemented profile work remains pending. No skipped task is markedX.

{{HARD}}
- [ ] 30.1 For Bauxite mine, derive a single-model brief from its approved manifest/template: Underground machinery/loading silhouette with Bauxite identity. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [ ] 30.2 Produce the required directions/rotations for Bauxite mine from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [ ] 30.3 Add construction/working/paused/depleted layout states for Bauxite mine, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.

### Group 31 - Author Salt mine

Goal-authorized skip (2026-10-08, master finalization): dispatched expansion work in this group is skipped under [finalization exception GP-TG](r2mlvs3mp-z-a-finalization-ledger.md#gp-tg-tube-and-geothermal-expansion-requires-major-gap-planning). Existing delivered prefixes stay complete; unimplemented profile work remains pending. No skipped task is markedX.

{{HARD}}
- [ ] 31.1 For Salt mine, derive a single-model brief from its approved manifest/template: Underground machinery/loading silhouette with Salt identity. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [ ] 31.2 Produce the required directions/rotations for Salt mine from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [ ] 31.3 Add construction/working/paused/depleted layout states for Salt mine, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.

### Group 32 - Author Lithium mine

Goal-authorized skip (2026-10-08, master finalization): dispatched expansion work in this group is skipped under [finalization exception GP-TG](r2mlvs3mp-z-a-finalization-ledger.md#gp-tg-tube-and-geothermal-expansion-requires-major-gap-planning). Existing delivered prefixes stay complete; unimplemented profile work remains pending. No skipped task is markedX.

{{HARD}}
- [ ] 32.1 For Lithium mine, derive a single-model brief from its approved manifest/template: Deep machinery/loading silhouette with Lithium identity. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [ ] 32.2 Produce the required directions/rotations for Lithium mine from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [ ] 32.3 Add construction/working/paused/depleted layout states for Lithium mine, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.

### Group 33 - Author Nickel mine

Goal-authorized skip (2026-10-08, master finalization): dispatched expansion work in this group is skipped under [finalization exception GP-TG](r2mlvs3mp-z-a-finalization-ledger.md#gp-tg-tube-and-geothermal-expansion-requires-major-gap-planning). Existing delivered prefixes stay complete; unimplemented profile work remains pending. No skipped task is markedX.

{{HARD}}
- [ ] 33.1 For Nickel mine, derive a single-model brief from its approved manifest/template: Deep machinery/loading silhouette with Nickel identity. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [ ] 33.2 Produce the required directions/rotations for Nickel mine from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [ ] 33.3 Add construction/working/paused/depleted layout states for Nickel mine, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.

### Group 34 - Author Cobalt mine

Goal-authorized skip (2026-10-08, master finalization): dispatched expansion work in this group is skipped under [finalization exception GP-TG](r2mlvs3mp-z-a-finalization-ledger.md#gp-tg-tube-and-geothermal-expansion-requires-major-gap-planning). Existing delivered prefixes stay complete; unimplemented profile work remains pending. No skipped task is markedX.

{{HARD}}
- [ ] 34.1 For Cobalt mine, derive a single-model brief from its approved manifest/template: Deep machinery/loading silhouette with Cobalt identity. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [ ] 34.2 Produce the required directions/rotations for Cobalt mine from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [ ] 34.3 Add construction/working/paused/depleted layout states for Cobalt mine, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.

### Group 35 - Author Rare-earth mine

Goal-authorized skip (2026-10-08, master finalization): dispatched expansion work in this group is skipped under [finalization exception GP-TG](r2mlvs3mp-z-a-finalization-ledger.md#gp-tg-tube-and-geothermal-expansion-requires-major-gap-planning). Existing delivered prefixes stay complete; unimplemented profile work remains pending. No skipped task is markedX.

{{HARD}}
- [ ] 35.1 For Rare-earth mine, derive a single-model brief from its approved manifest/template: Deep machinery/loading silhouette with Rare-earth identity. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [ ] 35.2 Produce the required directions/rotations for Rare-earth mine from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [ ] 35.3 Add construction/working/paused/depleted layout states for Rare-earth mine, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.

### Group 36 - Author Uranium mine

Goal-authorized skip (2026-10-08, master finalization): dispatched expansion work in this group is skipped under [finalization exception GP-TG](r2mlvs3mp-z-a-finalization-ledger.md#gp-tg-tube-and-geothermal-expansion-requires-major-gap-planning). Existing delivered prefixes stay complete; unimplemented profile work remains pending. No skipped task is markedX.

{{HARD}}
- [ ] 36.1 For Uranium mine, derive a single-model brief from its approved manifest/template: Deep machinery/loading silhouette with Uranium identity. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [ ] 36.2 Produce the required directions/rotations for Uranium mine from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [ ] 36.3 Add construction/working/paused/depleted layout states for Uranium mine, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.

### Local Mandatory Reading - M1 Surface spoil receiver sources

_Applies to Groups 37-38; master checkpoint 8.4._

Read this plan's Global and Source generation Area, only the executing receiver's manifest/brief, and `InstallSpoilReceiverDefinition`, `GetSpoilReceiverRoom`, `AdvanceSpoilReceiver` in `src/spoil_receiver.cpp` / public recipe/state in `src/spoil_receiver.h`. The measured native layouts are Earth 2x3/six cells and Rock 4x4/sixteen cells (`src/tests/multilayer_art_geometry.cpp`). Inspect the actual layout cells before selecting per-tile art. Industries expose one fixed layout, so unsupported view rotations must be reconciled with that producer rather than synthesized. Construction/idle/working/full cues present existing bounded stock/processing; animation cannot consume cargo. Keep per-cell 64x64 anchors/back/front coverage and assemble a whole-site review before source closure.

### Group 37 - Author Earth reclamation/disposal site

{{HARD}}
- [X] 37.1 For Earth reclamation/disposal site, derive a single-model brief from its approved manifest/template: profile-selected Surface receiver footprint and recipe cues. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [X] 37.2 Produce the required directions/rotations for Earth reclamation/disposal site from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [X] 37.3 Add construction/working/full/idle states for Earth reclamation/disposal site, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.

37 source closure (2026-10-07): the actual fixed native 2x3 layout has six column-major cells, with intake, brown soil bank, retaining drain, soil conveyor, receiving yard and service hut. `ready` presents idle; `blocked` presents full/interval refusal. No industry rotation exists: publisher/validator and both receiver briefs now declare only `none`, reducing the exact whole-profile requirement to **143 families / 54,939 sprites** without altering native offsets/content identity. Four source states, shared authored lamp phases and paired per-cell parts supply **192 poses**; static soil/paving repeats intentionally while the hut fitting animates. [Actual receiver boundaries](../../../../logs/r2mlvs13-u-receiver-boundary.log) pass 2,560 assertions / seven cases; [focused Earth source/contact/refusal/schema checks](../../../../logs/r2mlvs13-v-earth-source-checks.log) pass. Whole-site PNG/GIF review is in `assets/mlvs/earth-receiver-details-review.png`. Runtime loading/stock-state selection and user art review remain pending.

### Group 38 - Author Rock crusher/disposal site

{{HARD}}
- [X] 38.1 For Rock crusher/disposal site, derive a single-model brief from its approved manifest/template: profile-selected Surface receiver footprint and recipe cues. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [X] 38.2 Produce the required directions/rotations for Rock crusher/disposal site from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [X] 38.3 Add construction/working/full/idle states for Rock crusher/disposal site, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.

38 source closure (2026-10-07): **512 per-cell back/front poses** cover the actual fixed 4x4 layout, construction/idle/working/full states and four shared authored indicator phases. Twelve Rock-specific roles use selected idle/construction/stock originals; hut, paving, retaining wall and drain reuse exact Earth cells. The short stock sheet contributes only its real 20 panels; a separate original fills the missing four full-state roles. Explicit nonoverlapping crop rectangles resolve tight gutters. [Focused Rock source/placement/refusal/schema checks](../../../../logs/r2mlvs13-w-rock-source-checks.log) pass; the slow placement fixture was split into `multilayer_facility_art.py` / `multilayer_facility_art-1.py` and now caches fixed-cell palette reductions. Whole-site PNG/GIF review is `assets/mlvs/rock-receiver-details-review.png`. User art review and native payload/state selection remain pending.

### Group 39 - Author metal refinery

Goal-authorized skip (2026-10-08, master finalization): dispatched expansion work in this group is skipped under [finalization exception GP-TG](r2mlvs3mp-z-a-finalization-ledger.md#gp-tg-tube-and-geothermal-expansion-requires-major-gap-planning). Existing delivered prefixes stay complete; unimplemented profile work remains pending. No skipped task is markedX.

{{HARD}}
- [ ] 39.1 For metal refinery, derive a single-model brief from its approved manifest/template: profile-selected Surface receiver footprint and recipe cues. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [ ] 39.2 Produce the required directions/rotations for metal refinery from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [ ] 39.3 Add construction/working/full/idle states for metal refinery, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.

### Group 40 - Author chemical works

Goal-authorized skip (2026-10-08, master finalization): dispatched expansion work in this group is skipped under [finalization exception GP-TG](r2mlvs3mp-z-a-finalization-ledger.md#gp-tg-tube-and-geothermal-expansion-requires-major-gap-planning). Existing delivered prefixes stay complete; unimplemented profile work remains pending. No skipped task is markedX.

{{HARD}}
- [ ] 40.1 For chemical works, derive a single-model brief from its approved manifest/template: profile-selected Surface receiver footprint and recipe cues. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [ ] 40.2 Produce the required directions/rotations for chemical works from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [ ] 40.3 Add construction/working/full/idle states for chemical works, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.

### Group 41 - Author Surface battery works

Goal-authorized skip (2026-10-08, master finalization): dispatched expansion work in this group is skipped under [finalization exception GP-TG](r2mlvs3mp-z-a-finalization-ledger.md#gp-tg-tube-and-geothermal-expansion-requires-major-gap-planning). Existing delivered prefixes stay complete; unimplemented profile work remains pending. No skipped task is markedX.

{{HARD}}
- [ ] 41.1 For Surface battery works, derive a single-model brief from its approved manifest/template: profile-selected Surface receiver footprint and recipe cues. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [ ] 41.2 Produce the required directions/rotations for Surface battery works from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [ ] 41.3 Add construction/working/full/idle states for Surface battery works, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.

### Group 42 - Author electronics works

Goal-authorized skip (2026-10-08, master finalization): dispatched expansion work in this group is skipped under [finalization exception GP-TG](r2mlvs3mp-z-a-finalization-ledger.md#gp-tg-tube-and-geothermal-expansion-requires-major-gap-planning). Existing delivered prefixes stay complete; unimplemented profile work remains pending. No skipped task is markedX.

{{HARD}}
- [ ] 42.1 For electronics works, derive a single-model brief from its approved manifest/template: profile-selected Surface receiver footprint and recipe cues. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [ ] 42.2 Produce the required directions/rotations for electronics works from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [ ] 42.3 Add construction/working/full/idle states for electronics works, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.

### Group 43 - Author component manufacture

Goal-authorized skip (2026-10-08, master finalization): dispatched expansion work in this group is skipped under [finalization exception GP-TG](r2mlvs3mp-z-a-finalization-ledger.md#gp-tg-tube-and-geothermal-expansion-requires-major-gap-planning). Existing delivered prefixes stay complete; unimplemented profile work remains pending. No skipped task is markedX.

{{HARD}}
- [ ] 43.1 For component manufacture, derive a single-model brief from its approved manifest/template: profile-selected Surface receiver footprint and recipe cues. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [ ] 43.2 Produce the required directions/rotations for component manufacture from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [ ] 43.3 Add construction/working/full/idle states for component manufacture, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.

### Group 44 - Author robotics manufacture

Goal-authorized skip (2026-10-08, master finalization): dispatched expansion work in this group is skipped under [finalization exception GP-TG](r2mlvs3mp-z-a-finalization-ledger.md#gp-tg-tube-and-geothermal-expansion-requires-major-gap-planning). Existing delivered prefixes stay complete; unimplemented profile work remains pending. No skipped task is markedX.

{{HARD}}
- [ ] 44.1 For robotics manufacture, derive a single-model brief from its approved manifest/template: profile-selected Surface receiver footprint and recipe cues. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [ ] 44.2 Produce the required directions/rotations for robotics manufacture from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [ ] 44.3 Add construction/working/full/idle states for robotics manufacture, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.

### Group 45 - Author mirror-kit manufacture

Goal-authorized skip (2026-10-08, master finalization): dispatched expansion work in this group is skipped under [finalization exception GP-TG](r2mlvs3mp-z-a-finalization-ledger.md#gp-tg-tube-and-geothermal-expansion-requires-major-gap-planning). Existing delivered prefixes stay complete; unimplemented profile work remains pending. No skipped task is markedX.

{{HARD}}
- [ ] 45.1 For mirror-kit manufacture, derive a single-model brief from its approved manifest/template: profile-selected Surface receiver footprint and recipe cues. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [ ] 45.2 Produce the required directions/rotations for mirror-kit manufacture from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [ ] 45.3 Add construction/working/full/idle states for mirror-kit manufacture, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.

### Group 46 - Author nuclear plant

Goal-authorized skip (2026-10-08, master finalization): dispatched expansion work in this group is skipped under [finalization exception GP-TG](r2mlvs3mp-z-a-finalization-ledger.md#gp-tg-tube-and-geothermal-expansion-requires-major-gap-planning). Existing delivered prefixes stay complete; unimplemented profile work remains pending. No skipped task is markedX.

{{HARD}}
- [ ] 46.1 For nuclear plant, derive a single-model brief from its approved manifest/template: profile-selected Surface receiver footprint and recipe cues. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [ ] 46.2 Produce the required directions/rotations for nuclear plant from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [ ] 46.3 Add construction/working/full/idle states for nuclear plant, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.

### Group 47 - Author tube guideways and junctions

Goal-authorized skip (2026-10-08, master finalization): dispatched expansion work in this group is skipped under [finalization exception GP-TG](r2mlvs3mp-z-a-finalization-ledger.md#gp-tg-tube-and-geothermal-expansion-requires-major-gap-planning). Existing delivered prefixes stay complete; unimplemented profile work remains pending. No skipped task is markedX.

{{HARD}}
- [ ] 47.1 For tube guideways and junctions, derive a single-model brief from its approved manifest/template: sealed shells that leave moving capsules readable. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [ ] 47.2 Produce the required directions/rotations for tube guideways and junctions from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [ ] 47.3 Add straight/45-degree bend/junction/end and powered/unpowered pieces for tube guideways and junctions, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.

### Group 48 - Author tube terminals and service/interchange facilities

Goal-authorized skip (2026-10-08, master finalization): dispatched expansion work in this group is skipped under [finalization exception GP-TG](r2mlvs3mp-z-a-finalization-ledger.md#gp-tg-tube-and-geothermal-expansion-requires-major-gap-planning). Existing delivered prefixes stay complete; unimplemented profile work remains pending. No skipped task is markedX.

{{HARD}}
- [ ] 48.1 For tube terminals and service/interchange facilities, derive a single-model brief from its approved manifest/template: distinct powered terminal/depot/rail-road transfer templates. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [ ] 48.2 Produce the required directions/rotations for tube terminals and service/interchange facilities from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [ ] 48.3 Add approach/occupied/unpowered fronts and backs for tube terminals and service/interchange facilities, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.

### Group 49 - Author Mole-70

Goal-authorized skip (2026-10-08, master finalization): dispatched expansion work in this group is skipped under [finalization exception GP-TG](r2mlvs3mp-z-a-finalization-ledger.md#gp-tg-tube-and-geothermal-expansion-requires-major-gap-planning). Existing delivered prefixes stay complete; unimplemented profile work remains pending. No skipped task is markedX.

{{HARD}}
- [ ] 49.1 For Mole-70, derive a single-model brief from its approved manifest/template: low segmented amber hopper ribs. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [ ] 49.2 Produce the required directions/rotations for Mole-70 from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [ ] 49.3 Add eight-direction empty/loaded/service and purchase frames for Mole-70, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.

### Group 50 - Author Crate-75

Goal-authorized skip (2026-10-08, master finalization): dispatched expansion work in this group is skipped under [finalization exception GP-TG](r2mlvs3mp-z-a-finalization-ledger.md#gp-tg-tube-and-geothermal-expansion-requires-major-gap-planning). Existing delivered prefixes stay complete; unimplemented profile work remains pending. No skipped task is markedX.

{{HARD}}
- [ ] 50.1 For Crate-75, derive a single-model brief from its approved manifest/template: square green clamped containers. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [ ] 50.2 Produce the required directions/rotations for Crate-75 from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [ ] 50.3 Add eight-direction empty/loaded/service and purchase frames for Crate-75, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.

### Group 51 - Author Cistern-80

Goal-authorized skip (2026-10-08, master finalization): dispatched expansion work in this group is skipped under [finalization exception GP-TG](r2mlvs3mp-z-a-finalization-ledger.md#gp-tg-tube-and-geothermal-expansion-requires-major-gap-planning). Existing delivered prefixes stay complete; unimplemented profile work remains pending. No skipped task is markedX.

{{HARD}}
- [ ] 51.1 For Cistern-80, derive a single-model brief from its approved manifest/template: cyan twin tanks and pressure bands. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [ ] 51.2 Produce the required directions/rotations for Cistern-80 from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [ ] 51.3 Add eight-direction fluid-level/service and purchase frames for Cistern-80, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.

### Group 52 - Author Shield-85

Goal-authorized skip (2026-10-08, master finalization): dispatched expansion work in this group is skipped under [finalization exception GP-TG](r2mlvs3mp-z-a-finalization-ledger.md#gp-tg-tube-and-geothermal-expansion-requires-major-gap-planning). Existing delivered prefixes stay complete; unimplemented profile work remains pending. No skipped task is markedX.

{{HARD}}
- [ ] 52.1 For Shield-85, derive a single-model brief from its approved manifest/template: short pale shielded octagonal body. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [ ] 52.2 Produce the required directions/rotations for Shield-85 from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [ ] 52.3 Add eight-direction sealed-load/service and purchase frames for Shield-85, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.

### Group 53 - Author Forge-05

Goal-authorized skip (2026-10-08, master finalization): dispatched expansion work in this group is skipped under [finalization exception GP-TG](r2mlvs3mp-z-a-finalization-ledger.md#gp-tg-tube-and-geothermal-expansion-requires-major-gap-planning). Existing delivered prefixes stay complete; unimplemented profile work remains pending. No skipped task is markedX.

{{HARD}}
- [ ] 53.1 For Forge-05, derive a single-model brief from its approved manifest/template: broad dark braced machinery frame. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [ ] 53.2 Produce the required directions/rotations for Forge-05 from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [ ] 53.3 Add eight-direction heavy-load/service and purchase frames for Forge-05, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.

### Group 54 - Author vent field and collector

Goal-authorized skip (2026-10-08, master finalization): dispatched expansion work in this group is skipped under [finalization exception GP-TG](r2mlvs3mp-z-a-finalization-ledger.md#gp-tg-tube-and-geothermal-expansion-requires-major-gap-planning). Existing delivered prefixes stay complete; unimplemented profile work remains pending. No skipped task is markedX.

{{HARD}}
- [ ] 54.1 For vent field and collector, derive a single-model brief from its approved manifest/template: natural source versus installed bounded heat capture. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [ ] 54.2 Produce the required directions/rotations for vent field and collector from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [ ] 54.3 Add survey/excavated/connected/idle states for vent field and collector, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.

### Group 55 - Author Vent-Tap Powerhouse

Goal-authorized skip (2026-10-08, master finalization): dispatched expansion work in this group is skipped under [finalization exception GP-TG](r2mlvs3mp-z-a-finalization-ledger.md#gp-tg-tube-and-geothermal-expansion-requires-major-gap-planning). Existing delivered prefixes stay complete; unimplemented profile work remains pending. No skipped task is markedX.

{{HARD}}
- [ ] 55.1 For Vent-Tap Powerhouse, derive a single-model brief from its approved manifest/template: 1980 plant footprint and electrical output equipment. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [ ] 55.2 Produce the required directions/rotations for Vent-Tap Powerhouse from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [ ] 55.3 Add construction/working/heat-starved states for Vent-Tap Powerhouse, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.

### Group 56 - Author Magma-Loop Power Station

Goal-authorized skip (2026-10-08, master finalization): dispatched expansion work in this group is skipped under [finalization exception GP-TG](r2mlvs3mp-z-a-finalization-ledger.md#gp-tg-tube-and-geothermal-expansion-requires-major-gap-planning). Existing delivered prefixes stay complete; unimplemented profile work remains pending. No skipped task is markedX.

{{HARD}}
- [ ] 56.1 For Magma-Loop Power Station, derive a single-model brief from its approved manifest/template: 2000 closed-loop plant distinct from the early powerhouse. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [ ] 56.2 Produce the required directions/rotations for Magma-Loop Power Station from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [ ] 56.3 Add construction/working/heat-starved states for Magma-Loop Power Station, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.

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

Goal-authorized skip (2026-10-08, master finalization): dispatched expansion work in this group is skipped under [finalization exception GP-TG](r2mlvs3mp-z-a-finalization-ledger.md#gp-tg-tube-and-geothermal-expansion-requires-major-gap-planning). Existing delivered prefixes stay complete; unimplemented profile work remains pending. No skipped task is markedX.

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

### Local Mandatory Reading - M1 ground control source slice

_Applies to Group 78B; master checkpoint 8.5. Whole Group 78 remains pending for later orbital controls._

Read this plan's Global and Source generation Area, the seven selected `control.survey`, `control.excavation`, `control.tbm-area`, `control.tbm-turn45`, `control.exchange`, `control.deep-lock`, `control.future-lock` rows/briefs and existing `cargo.ERTH` / `cargo.ROCK` originals. Inspect `GetTBMPairControls` in `src/tbm_controls.h/.cpp`, actual portal widgets in `src/layer_portal_gui.cpp`, and project/area/exchange tools in `src/underground_gui.cpp`. `PreviewTBMCommissionArea` / `SetConstructionPreview` supplies canonical outline/turn geometry; sprite icons cannot grant construction, rotate a cutter or change a quote. Read existing layer icons and purchase-menu lock art for reuse. Native 20x20 enabled/disabled/pressed controls have no world heading. Keep locks/date cues in menus and controls, never on world vehicle bodies. Personal art review remains deferred. Testing/build verification is deferred until 2026-10-07 12:25:55 UTC by the user's low-power instruction.

### Group 78B - Author the M1 ground control subset

{{MEDIUM}}
- [X] 78B.1 Derive native ground briefs/bindings from actual survey, portal commissioning, area outline, legal 45-degree turn, exchange and Deep/date control owners; reuse delivered Earth/Rock cargo and existing layer/menu graphics. Retain originals/prompts/provenance.
- [X] 78B.2 Supply the seven ground control families in enabled/disabled/pressed states at 20x20 with fixed anchors, plus an explicit handoff to the existing canonical world outline/turn preview. No orbital collection/drop source is dispatched here.
- [X] 78B.3 Bake model-specific native/enlarged contact review; check source coverage, palette/anchors and distinct blocked/date/selection cues in a focused burst after the low-power window. Record the completed subset before closing master 8.5; leave original Group 78 unchecked.

78B source staging (2026-10-07): seven controls / **21 native 20x20 poses** are baked with retained layer/portal/cutter designs and UI-only Deep/calendar locks. [Original/prompt/bindings](assets/mlvs/control-generation-record.json) maps commissioning, floor, 45-degree advancing turn and exchange to their actual command/query/widget owners. Earth/Rock cargo stays exact; existing `SPR_CURSOR_QUERY`, white floor/blue turn-space/red uncovered `SetConstructionPreview` overlays are reused. Review: `assets/mlvs/ground-controls-details-review.png`. Source status/coverage remains explicitly **verification deferred**, and master 8.5 stays unchecked until the requested focused checks can run after **12:25:55 UTC**. During this user-requested window, continue independent packing/tool development without activating unverified sources or claiming checks passed.

78B source closure (2026-10-07, after 12:25:55 UTC): [focused native controls](../../../../logs/r2mlvs13-x-control-source-checks.log) pass exact regeneration, seven-family/21-state coverage, palette/anchor/public bindings and independent missing-state/bad-bounds/illegal-color refusals. Public source coverage is verified; review/provenance status now matches that evidence. Only this declared ground subset and master 8.5 close; original whole Group 78, orbital collection/drop controls and all native runtime integration remain pending.

### Group 78 - Author construction and cargo controls

Goal-authorized skip (2026-10-08, master finalization): dispatched expansion work in this group is skipped under [finalization exception GP-TG](r2mlvs3mp-z-a-finalization-ledger.md#gp-tg-tube-and-geothermal-expansion-requires-major-gap-planning). Existing delivered prefixes stay complete; unimplemented profile work remains pending. No skipped task is markedX.

{{HARD}}
- [ ] 78.1 For construction and cargo controls, derive a single-model brief from its approved manifest/template: native-size layer/cargo identities with separate collection/drop circles. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [ ] 78.2 Produce the required directions/rotations for construction and cargo controls from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [ ] 78.3 Add menu/cursor/locked/blocked/turn-preview and resource icons for construction and cargo controls, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.

### Local Mandatory Reading - M1 deterministic packing preparation

_Applies to Groups 79B/80B/81B; master checkpoint 8.6. Independent tool preparation may proceed while 78B.3 testing is deferred; do not pack/activate unverified control art._

Read this plan's Global and Source generation Area, the exact 40-family M1 brief set and current public/source atlas manifests. Read `sprite_quantize.crop_and_shift` / `png_indexed` and native `src/newgrf/newgrf_act5.cpp` type 0x1C offset/count handling. Read base-set NFO includes, `media/baseset/openttd/CMakeLists.txt`, `media/baseset/CMakeLists.txt`, `cmake/scripts/CreateGRF.cmake` and published constants in `src/table/sprites.h`. Keep original 0..65 pixels and files immutable, leave 66..73 unactivated, preserve planned 74..199 offsets and emit only delivered core segments. Append selected vehicle/portal/facility/purchase/cargo art after 199 in stable public family/state/frame/direction/tile/part order. Use a separate base-set PNG/NFO so the immutable MLVI cargo package and legacy source sheet retain their hashes. Validate each actual source cell and anchor; static paving and terrain texture variants are explicitly distinguished from animated machinery/status. Later unselected families remain pending.

Context has compacted 1 times while executing this group.

### Group 79B - Pack the selected M1 source set

{{HARD}}
- [X] 79B.1 Read the selected per-family authoritative atlases and quantization policy, validate exact cell identities, and crop transparent margins with anchor shifts; preserve all original source bytes.
- [X] 79B.2 Emit separate PNG/NFO, native range metadata and generated source declarations in stable manifest order. Preserve old 0..65, reserved 66..73 and planned 74..199; append selected art with an explicit native packing route while retaining definition-package identity.
- [X] 79B.3 Pack the actual native base-set payload with pinned tools and regenerate its checksum metadata; compare two clean selected-source conversions for identical output.
- [X] 79B.4 Prove the generator-free build loads checked-in payload/IDs/counts; keep unselected later-family scope and whole 79.1-79.4 pending.

79B closure (2026-10-07): forty selected families supply 2,311 native sprites in independent Action5 type1C segments. [Two clean pinned conversions/builds](../../../../logs/r2mlvs13-ap-pinned-pack-reproducibility.log) reproduce GRF SHA256 `cc70556571c1b12549538baa5ea91cf176527faacad3f6d1299682defd1e79ac` and GRFID container-v2 checksum `9e6ba7ea0fa173a5d3129c2922875e46`. [Generator-free packaging](../../../../logs/r2mlvs13-aq-generator-free-packaging.log) disables Grfcodec discovery, removes only the disposable build copy, executes the checked-in source-copy rule, and [actually loads every protected/selected sprite](../../../../logs/r2mlvs13-al-generator-free-load.log): 40,431 assertions / one case. Generator discovery is restored. Protected source PNG/NFO and immutable MLVI definitions retain their identity; reserved/unselected slots stay unactivated. [Packed handoff](../../../../logs/r2mlvs13-ar-packed-handoff.log) binds public native ranges/hashes only after passing build/load evidence. Runtime consumers and user acceptance remain pending.

### Group 80B - Validate the packed M1 source set

{{HARD}}
- [X] 80B.1 Validate complete selected Cartesian source coverage, exact native dimensions/anchors and unique packed references, with failing family/source diagnostics.
- [X] 80B.2 Check DOS palette/index-zero alpha, forbidden colors and nonempty physical parts, preserving authored near-mouth masks and immutable protected source bytes.
- [X] 80B.3 Match public native-route ranges/hashes to emitted and actually loaded sprite counts; reject stale/unreferenced selected payloads.
- [X] 80B.4 Add bounded missing-frame, bad-bounds and illegal-palette corruption fixtures; execute them in focused files after the low-power window.

80B closure (2026-10-07): [source/refusal/crop checks](../../../../logs/r2mlvs13-z-pack-source-checks.log) cover all 2,311 exact Cartesian poses, DOS palette/index-zero rules and independent missing-frame/bounds/reserved-color guards. The actual native loader compares every normal-resolution pixel, dimension and anchor for the 66 protected M0 and 2,311 selected M1 sprites, actual file ownership and exact loaded count at bound 2,404. Its observation encoder reads the normal decoder without replacing the renderer/cache. Initial test assumptions about Windows config-folder lookup and scaled zoom roots were corrected; the [native loader](../../../../logs/r2mlvs13-am-packed-art-loading.log) and generator-free check pass. Whole-profile Groups79/80 remain pending.

### Group 81B - Preserve M1 simulation independence

{{MEDIUM}}
- [X] 81B.1 Expose presentation-only frame/texture selection to native consumers. Actual stopped wagon load/phase, reversed motion, depot occupancy and receiver stock use their existing owners; clocks/cameras never advance cutting, exchange or processing.

81B closure (2026-10-07): `multilayer_sprite.h` supplies pure validated pose, animation-phase and stable floor-texture selection over the one generated range authority. Invalid family/state/frame/view/cell/part indices refuse; stopped hopper wheels remain frame zero while actual load level stays a separate caller-owned state. [Native boundary/owner checks](../../../../logs/r2mlvs13-aw-visual-selector-checks.log) pass 9,541 assertions / two cases, covering all 2,311 unique poses, explicit reversed/portal/receiver references and unchanged native receiver/project/funds/RNG owners across widely separated visual clocks. The native world fixture explicitly loads language before sorted cargo definitions. [Packed public schema](../../../../logs/r2mlvs13-ax-packed-public-contract.log) validates all 143 families / 54,939 profile requirements. Actual rendering consumers and sound remain the next checkpoint; this closes only the selected simulation-independence scope.

### Group 79 - Convert and pack from stable originals

Goal-authorized skip (2026-10-08, master finalization): dispatched expansion work in this group is skipped under [finalization exception GP-TG](r2mlvs3mp-z-a-finalization-ledger.md#gp-tg-tube-and-geothermal-expansion-requires-major-gap-planning). Existing delivered prefixes stay complete; unimplemented profile work remains pending. No skipped task is markedX.

{{HARD}}
- [ ] 79.1 Extend deterministic quantization/cropping to consume manifest records, preserve index-0 transparency and handle reserved company/animation colors explicitly. Preserve world anchor when transparent margins are cropped.
- [ ] 79.2 Generate back/front parts and NFO/content-package metadata in stable manifest order with pinned tool versions. Keep existing published sprite offsets unchanged.
- [ ] 79.3 Pack the selected core/content routes and regenerate required GRF/checksum metadata. Compare two clean conversions from the same originals/manifest for identical outputs.
- [ ] 79.4 Verify a build without the asset generator loads the checked-in generated payload and resolves the same IDs/counts; source-only success is not distributed-asset acceptance.

### Group 80 - Validate asset structure and palette deterministically

Goal-authorized skip (2026-10-08, master finalization): dispatched expansion work in this group is skipped under [finalization exception GP-TG](r2mlvs3mp-z-a-finalization-ledger.md#gp-tg-tube-and-geothermal-expansion-requires-major-gap-planning). Existing delivered prefixes stay complete; unimplemented profile work remains pending. No skipped task is markedX.

{{HARD}}
- [ ] 80.1 Validate required record fields, unique IDs, expected direction/frame/state Cartesian coverage and exact native dimensions/anchors before packing. Report the failing asset ID and source.
- [ ] 80.2 Check transparent index, forbidden palette ranges, intended company-color pixels and empty/missing frames against the declared per-asset policy.
- [ ] 80.3 Compare manifest packed references and output hashes against emitted sprite count/offset metadata; reject stale or unreferenced gameplay assets.
- [ ] 80.4 Add small corruption fixtures for one missing frame, bad bounds and illegal palette entry. Validate authored transparent holes against explicit masks/templates; arbitrary legitimate holes still need human visual review.

### Group 81 - Source generation and reproducible conversion

Goal-authorized skip (2026-10-08, master finalization): dispatched expansion work in this group is skipped under [finalization exception GP-TG](r2mlvs3mp-z-a-finalization-ledger.md#gp-tg-tube-and-geothermal-expansion-requires-major-gap-planning). Existing delivered prefixes stay complete; unimplemented profile work remains pending. No skipped task is markedX.

{{MEDIUM}}
- [ ] 81.1 Keep simulation state independent of animation timing.

### Area Mandatory Reading - Runtime integration and review

_Applies to Groups 82-89._

**Entry gate.** The following public contracts must exist before dependent implementation in this area: `docs/zz-reach2/upgrades/2026-10/assets/multilayer_sprite_manifest.json`: `asset_id`, `consumer`, `directions`, `frames`, `anchor`, `source`.
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
| `docs/zz-reach2/upgrades/2026-10/assets/multilayer_sprite_manifest.json` (planned public contract; link after implementation) | Live definitions; no fixed range | [r2mlvs13-graphics-and-asset-pipeline.md](r2mlvs13-graphics-and-asset-pipeline.md) Groups 1-8, ending with Group 8's public-contract task, publish `asset_id`, `consumer`, `directions`, `frames`, `anchor`, `source`. These exports/schema fields are authoritative for this area. Same-plan consumers follow those earlier groups; cross-plan consumers verify the producer's required implementation/tests as well. Proposed names alone do not prove delivery. |
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
| `src/transport_tube.h` (planned public contract; link after implementation) | Live definitions; no fixed range | [r2mlvs16-underground-transport-tubes.md](r2mlvs16-underground-transport-tubes.md) Groups 1-8, ending with Group 8's public-contract task, publish `TubeVehicleCapability`, `TubeTerminal`, `CheckTubeConstruction`, `ComputeTubePowerDemand`. These exports/schema fields are authoritative for this area. Same-plan consumers follow those earlier groups; cross-plan consumers verify the producer's required implementation/tests as well. Proposed names alone do not prove delivery. Read this producer for the tube/geothermal integration and corresponding runtime captures; full-gallery/placeholder retirement awaits every required family in Groups 84-89 (only the named integration/subscenario); missing it blocks only that integration, not independent work. |
| `src/geothermal.h` (planned public contract; link after implementation) | Live definitions; no fixed range | [r2mlvs17-geothermal-energy-and-industry.md](r2mlvs17-geothermal-energy-and-industry.md) Groups 1-7, ending with Group 7's public-contract task, publish `GeothermalHeatAllocation`, `GeothermalUtilityLink`, `AllocateVentHeat`, `ExportGeothermalPower`. These exports/schema fields are authoritative for this area. Same-plan consumers follow those earlier groups; cross-plan consumers verify the producer's required implementation/tests as well. Proposed names alone do not prove delivery. Read this producer for the tube/geothermal integration and corresponding runtime captures; full-gallery/placeholder retirement awaits every required family in Groups 84-89 (only the named integration/subscenario); missing it blocks only that integration, not independent work. |

**ASSUMPTION: this was already built previously. If not, re-asses at runtime**. This applies only to the producer contracts named in this area's gate, after their listed producer groups have run. Verify their actual exports and focused tests before using the sketches below; proposed types and helper names in sketches are not evidence of delivered code.

### Local Mandatory Reading - M1 native terrain and portal binding

_Applies to Group82 and the staged Group83 return in master8.7._

Read this plan's Global and Runtime integration Area, then `multilayer_sprite.h`, generated `multilayer_sprite_assets.h` and the selected packed handoff. Inspect only `GetGeologyWorldState`, `PackedExcavationState::Get`, layer-bounded `TileAddWrap`, `GetLayerPortalWorkState`, reciprocal portal metadata and `DrawTile_Clear` / `DrawTile_LayerPortal` / `DrawLayerPortalPreview`. Preserve Classic/no-geology artwork, real layer filtering, existing rail overlays, building transparency and vanilla portal sorting boxes. Terrain mask bits follow screen upper-left/upper-right/lower-right/lower-left; texture frames are static variants. Verify the focused excavation-storage/portal-work-state producers before binding. Group82.3's actual final-machine crossing capture follows Group83.1-83.2's native vehicle/bay binding; this staging resolves its presentation dependency without claiming placeholder crossings as final art. Group83 then requires only the active train/project/exchange/receiver/service owners and actual sound event path, not unselected orbital/industry providers.

### Group 82 - Integrate M1 terrain and portal states

{{HARD}}
- [X] 82.1 Verify r2mlvs2/3 state queries and matching manifest entries, then bind Solid/Excavating/Excavated terrain and Sealed/Commissioning/Connected portal graphics to authoritative state.
- [X] 82.2 Check adjacent tile edges, both depth palettes, every portal orientation and extra-view layer filtering in runtime scenes. The renderer does not advance excavation.
- [X] 82.3 Capture frame evidence for an assigned TBM entering/reversing through a sealed work pocket; validate front/back occlusion at each crossing phase.

82.1-82.2 closure (2026-10-07): the real clear-tile and rail-portal renderers consume geology/work queries and packed source poses; Classic/no-geology and road art retain their original paths. [Producer checks](../../../../logs/r2mlvs13-ay-terrain-portal-producers.log) pass 122 assertions / three cases, and [native bindings](../../../../logs/r2mlvs13-be-terrain-portal-checks.log) pass 80 / three cases, including a real reserved TBM job and unchanged controller/job state. [Actual native raster checks](../../../../logs/r2mlvs13-bm-native-picker-capture-checks.log) pass 34 assertions / one isolated application case and preserve the immutable stopped Deep save: four independent-layer native/2x viewport frames and four real picker scenes / sixteen depth/endpoint/orientation views. [Gallery metadata](assets/reviews/m1-terrain-runtime.json) records PNG hashes. Normal/zoomed terrain and basic/Deep mouth views were inspected; roofs fit the native 64-pixel picker and existing sorting/transparency remains in use. Render-time geology/projects/receivers/RNG remain unchanged. User personal acceptance and82.3's final-machine crossing evidence stay pending; the latter follows83.1-83.2 as staged above.

82.3 closure (2026-10-07): [actual loaded Deep source loop](../../../../logs/r2mlvs13-cx-native-loop-deep-3.log) passes197,581 assertions / one native application case through32,853 real frames and three exchanges. Its [40 native/2x rasters and physical-state records](assets/reviews/m1-loop-deep-3-runtime.json) cover assigned entry/spanning/pocket, supported backing through both real housings and the correct independent endpoint/layer sorting. Mouths occlude the machine while it is inside; four same-heading drilling phases farther into the actual corridor show the exposed cutter/support and their preserved anchors. Native/2x entry/retreat, drilling and Surface disposal/returned-empty scenes were inspected. Each render preserves actual vehicle packet/position, geology/project/assignment/receiver/cash and both RNG owners; the immutable stopped source and startup script remain unchanged. The first live attempt exposed front-only consist recalculation in the animation refresh; it now calls each physical part's viewport refresh, with [complete-chain/unchanged-physics checks](../../../../logs/r2mlvs13-db-native-animation-refresh-checks.log) passing3,764 / four cases. The first empty-positioning snapshot was replaced by the actual post-disposal empty return. Deep source crossing evidence is complete;83.3 additionally runs and inspects the separate ordinary Underground depot loop. User personal art review remains deferred.

### Local Mandatory Reading - M1 actual train, cargo and facility presentation

_Applies to Group83 under the staged master8.7 order._

Read this plan's Global/Runtime integration Area and the delivered selected packing/terrain closures. Inspect `Train::GetImage` / `Train::Tick` / `Vehicle::UpdateViewport`, actual `GetTBMEngineModel` / `GetUndergroundEngineCapability`, `GetTBMProjectForCutter` / `GetTBMConsistRole` / `GetTBMWorkState`, `IsDrivingBackwards` and saved exchange assignments. Read actual wagon `cargo.StoredCount` / `cargo_cap`, native depot occupancy and Surface receiver stock/interval queries only when binding those consumers. Verify focused TBM-work/exchange-commit producers before integration. Actual part direction supplies physical bend/backing continuity; reversing changes movement, never rotates the cutter body to the motion direction. A stopped loaded wagon keeps its real partial/full load and frame-zero wheels. Derive and invalidate only presentation caches through the existing train tick/rendering path; no new work tick, packet store, role registry or simulation RNG. Use actual saved-world viewport captures for crossing/loop evidence. Audio later reads the real phase/transition owner and distributed sample IDs separately.

### Group 83 - Integrate M1 cutter, wagons and exchange facility

{{HARD}}
- [X] 83.1 Bind r2mlvs4 work phase and heading to cutter/support/boring sprites, preserving directional continuity through legal 45-degree turns and supported retreat.
- [X] 83.2 Bind actual Earth/Rock load quantities to wagon states and r2mlvs5 stopped exchange to bay foregrounds; load graphics follow the same wagon IDs after coupling.
- [X] 83.3 Capture a real dig/fill/exchange/Surface-haul/empty-return loop and inspect clipping at native/zoomed scales. Correct assets or anchors, repack and retain reviewed hashes before claiming M1 graphics complete.
- [X] 83.4 Publish and verify the M1 TBM idle/boring/backing/service sound-event mapping required by Plan 4 task 16.2. Declare concrete distributed sample IDs/source/licensing or reviewed reuse of existing base sounds, verify loading and actual event playback, and update the consumer hook. Events consume authoritative phase/transition state and never advance work, charge money or consume simulation RNG. Track this audio mapping separately from raster frame coverage; an absent sample/hook remains pending. Execution is scheduled by [R2MLVS3MP Group 8](r2mlvs3mp-master-plan.md#group-8---deliver-and-review-the-complete-m1-asset-slice).

83.1-83.2 closure (2026-10-07): `Train::GetImage` consumes actual native engine identity, immutable cutter/support role, physical body facing and saved work/hold/service/backing facts. Backing changes movement rather than turning the cutter body; the existing front-engine tick refreshes only native visual caches at authored phase boundaries. Packet quantity takes priority over empty service/hold cues and stopped hopper wheels remain frame zero. Rail depot rendering reads the existing project/assignment/spatial occupancy owners and uses authored exchange/service back/fronts with existing overlays/transparency. [Producer checks](../../../../logs/r2mlvs13-bn-machine-exchange-producers.log) pass 39,683 assertions / eight cases; [actual GetImage, paid-cut/load/depot and genuine-coupling checks](../../../../logs/r2mlvs13-bw-native-image-exchange-checks.log) pass 3,762 / four cases. The same loaded wagon IDs preserve their packet quantities and source poses after real two-way coupling. Actual final-machine crossing/loop capture and sound remain pending. The remaining selected facilities/UI consumers are scoped83B below, before the combined raster capture.

83.3 closure (2026-10-07): real Deep source and Underground depot loops both load the immutable stopped saves and run the ordinary application/command/timer owners. [Deep3](../../../../logs/r2mlvs13-cx-native-loop-deep-3.log) passes197,581 assertions / one case through32,853 frames and three exchanges; [Underground2](../../../../logs/r2mlvs13-cx-native-loop-underground-2.log) passes94,385 / one through15,677 frames and two exchanges. [Deep review metadata](assets/reviews/m1-loop-deep-3-runtime.json) and [Underground review metadata](assets/reviews/m1-loop-underground-2-runtime.json) retain40/28 native/2x PNG hashes, four actual drilling phases and real dig/load/coupling/Surface disposal/returned-empty scenes. Source bays physically retreat through their commissioning pair; ordinary rock-layer depots physically receive their cutter there, as the actual resolved bay kind requires. Rendering snapshots preserve actual packets/positions, terrain/projects/assignments/receiver balances/cash and both RNG streams. Technical inspection found correct exposed body/support alignment, native bay foreground occlusion and stopped load/empty rendering; initial first-cell work remains correctly behind the mouth roof. No raster correction/repacking was needed. The real refresh assertion was corrected and verified as recorded above. Each scenario is a separate bounded focused invocation; no full regression ran. Personal user review and final M1 gallery acceptance remain deferred, and83.4 sound is current.

83.4 closure (2026-10-07): `multilayer_sound.h/.cpp` and the existing front-engine tick bind actual idle/unblocked boring/moving backing/genuine depot state to reviewed base-slot reuse: mine machinery9, cutting motor56, warning bell12 and metallic rail-tool30, with192/96/96/192 tick cadences. `media/multilayer/m1-sound-events.json` records mapping authority, selected-provider rights and exact IDs; no sound samples or proprietary CAT bytes are copied/repackaged. Existing mute, effect-volume, viewport and dedicated behavior owns audibility. [Owner/mute/cadence/depot/assembly-reversal checks](../../../../logs/r2mlvs13-do-native-sound-selection-checks.log) pass95 assertions / three cases. The assembly-magazine reverse command correctly does not create actual backwards motion; genuine backing is verified by the live loop. [Actual native phase/decoder/mixer playback](../../../../logs/r2mlvs13-cx-native-loop-underground-3.log) passes110,129 / one isolated case: all four events are selected by the real moving/stopped/depot controller at ticks0/81/3299/4178 and each emits nonempty signed stereo PCM at11,025Hz. Temporary camera/audio sampling preserves geology/projects/assignments/receivers/cash and both RNG owners. [Publication and silent-dispatch corruption refusal](../../../../logs/r2mlvs13-dp-native-sound-handoff-checks.log) bind that evidence in `media/multilayer/sprites/m1/sound-runtime.json`; decoded sizes/counts/fingerprints are retained without exporting sample audio. Native raster packing/content identities remain unchanged. Personal sound/art review remains deferred; master8.7 continues with the linked Plan2/3/4 presentation tasks.

### Local Mandatory Reading - Selected M1 service, receiver and UI consumers

_Applies to Group83B; scheduled after83.1-83.2 and before82.3/83.3 combined capture under master8.7._

Read child Global/Runtime integration Area and only delivered selected family ranges/source geometry. Inspect `DrawTile_Industry`, native receiver kind/stock/room/interval queries and fixed world-cell offsets; `DrawTile_Road` depot art and real road-vehicle occupancy; actual underground/portal/TBM widget state/capability queries, `GetRailIcon`, first-party cargo icons and geological survey visibility/depletion. Native receiver and road-service family dimensions/states/parts are authoritative in the public manifest. Preserve foreign/Classic art, date/capability refusals, existing transparency and all stock/cargo/work owners. No presentation clock processes a batch, creates a depot, unlocks a control or surveys a seam. Reuse existing world preview/cursor primitives. Verify focused active producer queries; actual combined runtime capture is later83.3, with user personal review deferred.

Context has compacted 1 times while executing this group.

### Group 83B - Bind the remaining selected M1 facilities and controls

{{HARD}}
- [X] 83B.1 Bind the selected ordinary road-service depot and Surface Earth/Rock receiver parts to actual occupancy, construction, stock and bounded-intake state; retain fixed cells and transparency.
- [X] 83B.2 Bind the selected seven ground controls, ten purchase states, two cargo icons and ten survey marker families to their existing UI/capability/cargo/geological queries. Keep all calendar and command gates with their owners and reuse actual preview/cursor geometry.
- [X] 83B.3 Verify those live family bindings and source ranges in focused cases, publish exact runtime consumer/evidence metadata for the selected set, then return to82.3/83.3 combined capture and83.4 sound. Unselected later families and user personal acceptance stay pending.

83B.1 evidence (2026-10-07): road/industry rendering consumes the actual native service/receiver selectors, fixed cells and occupancy/construction/stock/interval/aggregate bounds. Road-type custom depot graphics and foreign industries retain their resolvers; existing transparency remains active. Native receiver lamps refresh only tile presentation dirtiness through the existing receiver tick, before unchanged processing. [Receiver producer queries](../../../../logs/r2mlvs13-bx-receiver-art-producers.log) pass 691 assertions / six cases; [fixed-cell/stock/construction/road-owner checks](../../../../logs/r2mlvs13-ce-native-facility-art-checks.log) pass 147 / two cases. The road fixture opens its actual entrance and initializes the road registry before invoking the ordinary construction command. Actual combined raster capture remains the staged83.3 work after selected controls are bound.

83B.2 evidence (2026-10-07): actual underground widgets use seven native control families through standard image/text buttons or reserved informational icon regions; unchanged cursor/quote geometry owns placement. Purchase/list rendering resolves the real five engine mappings and existing buildability; the portal button uses its own commissioning query. Cargo icons require the actual first-party definition owner and preserve foreign labels/graphics. Survey overlays require the active lowered survey cursor and its explicit plane, expose the existing public/free geology policy and read actual depletion without setting saved survey flags. A focused cancellation fixture caught and corrected stale overlay ownership. [Controls, calendar, cancellation, ten resources, package ownership and actual EIDS checks](../../../../logs/r2mlvs13-cj-native-control-art-checks.log) pass186 assertions / six cases; [Debug game/test build](../../../../logs/r2mlvs13-ci-native-control-art-build.log) passes. Actual UI raster/range metadata is the following83B.3 task.

83B.3 closure (2026-10-07): [actual loaded control/window/purchase/cargo/survey rendering](../../../../logs/r2mlvs13-cl-native-control-capture.log) passes81 assertions / one isolated application case, with six lossless PNG review images and immutable source/RNG/work/receiver/reserve/cash owners. The [capture runner](../../../../logs/r2mlvs13-cq-native-control-raster-checks.log) retains exact window dimensions/pixels and restores the startup script. Ground menu, portal panel, five real engine purchase previews, two actual first-party cargo icons and Deep seam rendering were inspected for placement/clipping. [Existing area/turn/facility quote checks](../../../../logs/r2mlvs13-cu-native-area-presentation-checks.log) pass302 / three cases. [Exact selected binding publication and three stale-range/payload/raster refusals](../../../../logs/r2mlvs13-ct-native-runtime-binding-checks.log) publish40 families /2,311 sprites in the public manifest and `media/multilayer/sprites/m1/runtime-manifest.json`; actual call sites, loaded payload and emitted ranges agree. The verifier reads both PowerShell UTF16 and native UTF8 evidence without rewriting logs. Native source/definition bytes remain unchanged; unselected later families remain inactive. Headless window paint runs inside an explicit screenshot canvas, and native buttons reserve their full20px cell before paint changes state. Current work returns to82.3/83.3 final-machine crossing/loop scenes and83.4 sound; gallery/user acceptance remain pending.

### Group 84 - Runtime integration and review

Goal-authorized skip (2026-10-08, master finalization): dispatched expansion work in this group is skipped under [finalization exception GP-TG](r2mlvs3mp-z-a-finalization-ledger.md#gp-tg-tube-and-geothermal-expansion-requires-major-gap-planning). Existing delivered prefixes stay complete; unimplemented profile work remains pending. No skipped task is markedX.

{{MEDIUM}}
- [ ] 84.1 Integrate all ten mines and the underground conventional fleet after their r2mlvs6/7 code is delivered.
- [ ] 84.2 Integrate orbital launch, probe, module and solar families after their corresponding r2mlvs8..12 producers are delivered.
- [ ] 84.3 Integrate tube/geothermal art after r2mlvs16/17 producers are delivered; this later work does not block M1 art.

### Local Mandatory Reading - Selected M1 final gallery

_Applies to Group 85B under master 8.8; original Groups 85-88 remain the full-profile obligations._

Read child Global and Runtime integration Area only for the delivered M1 geology/portal/TBM/exchange/UI owners. Read the selected forty-family public/packed/runtime manifests and retained terrain/control/Deep/Underground loop reports. Inspect `NativeArtLoopProbe`, `CaptureNativeArtViewport`, actual `Train::GetImage`, `CheckClickOnVehicle` and existing building/industry transparency. Preserve immutable source saves, final payload identity and all simulation owners. Direction sheets query the real renderer without rotating a vehicle; animation and occlusion claims require actual driven world frames. Keep technical acceptance and the user's deferred personal review separate. Do not preload later launch/module/dock families.

### Group 85B - Review the selected M1 runtime slice

{{MEDIUM}}
- [X] 85B.1 Supplement genuine native/2x loop captures with paused/idle/service views, eight-direction renderer queries, building transparency and actual visible-body/layer picking checks.
- [X] 85B.2 Produce a compact contact sheet and linked native scenario gallery for all forty selected manifest IDs, retaining exact frame/source/log provenance and actual animation phases.
- [X] 85B.3 Confirm selected placeholders are retired and distributed GRF/source/range hashes match reviewed evidence; document retained limits and bounded stale-evidence refusals.
- [X] 85B.4 Record technical acceptance separately from deferred user feedback; keep unselected full-profile gallery rows pending.

85B.1 closure (2026-10-07): the optional gallery observer retains genuine paused/idle and real railway-depot service poses, queries all eight directions through actual `Train::GetImage` without rotating a unit, renders the same physical portal/disposal scenes with existing building/industry transparency, and verifies exposed cutter picking on its own layer with rejection on another layer. [Underground5 capture](../../../../logs/r2mlvs3mp-s-gallery-underground-capture-1.log) passes 94,734 assertions / one native case, fourteen physical scenes / forty-five PNGs. [Deep4 capture](../../../../logs/r2mlvs3mp-s-gallery-deep-capture.log) passes 197,868 assertions / one native case, sixteen physical scenes / fifty-four PNGs. Actual logs/source/frame hashes are retained in [Underground5](assets/reviews/m1-loop-underground-5-runtime.json) and [Deep4](assets/reviews/m1-loop-deep-4-runtime.json). Native direction/load/service/transparency frames were inspected. Rendering preserves projects/jobs/packets/positions/geology/receivers/cash and both random streams; original sources remain unchanged. Direction sheets are explicitly renderer queries; animation and occlusion evidence comes from actual driven world frames. 85B.2-85B.4 remain pending.

85B.2-85B.4 closure (2026-10-07): [gallery and distribution acceptance](r2mlvs3mp-t-gallery-acceptance.md) retains the [standalone offline gallery](assets/reviews/m1-runtime-gallery.html), [native contact sheet](assets/reviews/m1-runtime-contact-sheet.png) and exact forty-family/2,311-sprite review mapping. There are 157 original native frames, including all ten actual resource seams and separate actual road/unassigned rail-service depots built on paid-open ground. [Publication checks](../../../../logs/r2mlvs3mp-t-gallery-publication.log) verify actual source/frame/build/distributed GRF hashes and refuse missing real phases, substituted rasters and incomplete scope. The selected placeholders are retired; original/protected/reserved bytes and source poses retain their packing authority. Gallery frames/contact placement and offline links were inspected. Later binding refresh preserves technical review only when its complete current proof still verifies. User feedback is recorded as deferred, separately from technical acceptance. Original full-profile Groups 85-88 remain unchecked for later families.

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
| [docs/zz-reach2/upgrades/2026-10/tools/sprite_quantize.py](tools/sprite_quantize.py) | 379-411 | Deeper planning pass, 2026-10-04: Stable packing and metadata emission shaped per-model originals, coverage validation and reproducible conversion. |

## Completion Conditions

Every required building, vehicle, resource and gameplay-critical state has packaged graphics, valid directions/anchors and a runtime capture. Repeatable conversion works from retained sources. Offline sheets, generated originals and a successful compiler run alone do not establish art acceptance.
