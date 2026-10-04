# R2MLVS13 - Graphics and Asset Pipeline

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
| [docs/zz-reach2/upgrades/2026-10/r2mlvs-multilayer-vehicles-and-structures.md](r2mlvs-multilayer-vehicles-and-structures.md) | 188-192 | `AD-1` owns the complete supplied content/art profile, the split between engine behavior and content definitions, and missing-required-content failure. Every asset group must package the selected profile consistently; live sprite constants and the manifest, not this AD, own geometry/offsets. |
| [src/map_layer.h](../../../../src/map_layer.h) | 78-85 | `MultiLayerMap` ownership preamble: `Map` owns packed storage/geometry and this controller owns layer semantics. Every group uses world tiles/explicit layer helpers; camera selection and ad hoc plane offsets cannot define gameplay. |
| [src/table/sprites.h](../../../../src/table/sprites.h) | 380-409 | Complete Map Layer sprite constants and `SPR_NEWGRFS_BASE` bound the published block; preserve 0..65 and resolve any accepted live extension before allocating new IDs. |

Status: Groups 1-8 M0 contract completed; final source generation/packing/runtime art remain pending. Evidence: [sprite contract and geometry](r2mlvs13-a-sprite-contract.md). Parent: [R2MLVS master](r2mlvs-multilayer-vehicles-and-structures.md). First reading area uses r2mlvs1 `Draft Content Handoff` and feeds its `Content Freeze Handoff`; integration follows each consumer. Final art is required for milestone acceptance.

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

### Group 9 - Prepare controlled asset authoring and originals

{{MEDIUM}}
- [ ] 9.1 At asset execution time, read/use the available image-generation skill/tool for AI raster work and follow its editing/reference requirements. Keep prompts, references, chosen originals and provenance in repository asset directories; this planning pass generates no art.
- [ ] 9.2 Use the approved geometry/style templates to write one bounded generation brief per family/model: view labels, light direction, silhouette, material accents, state differences, transparent background and native target footprint.
- [ ] 9.3 Keep original outputs immutable and record selection/revision history separately from deterministic conversion. Generative output is not assumed reproducible from a seed; checked-in originals make subsequent packing reproducible.

### Group 10 - Author Underground terrain

{{HARD}}
- [ ] 10.1 For Underground terrain, derive a single-model brief from its approved manifest/template: solid face versus cavity with readable edges and survey markers. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [ ] 10.2 Produce the required directions/rotations for Underground terrain from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [ ] 10.3 Add all cut/progress/edge/corner and five resource-marker states for Underground terrain, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.

### Group 11 - Author Deep terrain

{{HARD}}
- [ ] 11.1 For Deep terrain, derive a single-model brief from its approved manifest/template: distinct basalt palette with the same cavity grammar. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [ ] 11.2 Produce the required directions/rotations for Deep terrain from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [ ] 11.3 Add all cut/progress/edge/corner and five Deep resource-marker states for Deep terrain, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.

### Group 12 - Author sealed and commissioning portal overlays

{{HARD}}
- [ ] 12.1 For sealed and commissioning portal overlays, derive a single-model brief from its approved manifest/template: existing four-direction housings with controlled work faces. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [ ] 12.2 Produce the required directions/rotations for sealed and commissioning portal overlays from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [ ] 12.3 Add Sealed/Commissioning/Connected fronts and backs for sealed and commissioning portal overlays, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.

### Group 13 - Author Deep rail portal variant

{{HARD}}
- [ ] 13.1 For Deep rail portal variant, derive a single-model brief from its approved manifest/template: rail-compatible deep housing aligned to the shared shaft geometry. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [ ] 13.2 Produce the required directions/rotations for Deep rail portal variant from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [ ] 13.3 Add all four approach directions and work states for Deep rail portal variant, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.

### Group 14 - Author Standard TBM

{{HARD}}
- [ ] 14.1 For Standard TBM, derive a single-model brief from its approved manifest/template: cutter/support proportions and legal 45-degree heading continuity. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [ ] 14.2 Produce the required directions/rotations for Standard TBM from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [ ] 14.3 Add idle/boring/reversing/service frames with fixed articulated seams for Standard TBM, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.

### Group 15 - Author Deep-rated TBM

{{HARD}}
- [ ] 15.1 For Deep-rated TBM, derive a single-model brief from its approved manifest/template: visibly heavier cutter and depth equipment. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [ ] 15.2 Produce the required directions/rotations for Deep-rated TBM from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [ ] 15.3 Add idle/boring/reversing/service frames with fixed articulated seams for Deep-rated TBM, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.

### Group 16 - Author Earth hopper

{{HARD}}
- [ ] 16.1 For Earth hopper, derive a single-model brief from its approved manifest/template: readable soil load and detachable wagon identity. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [ ] 16.2 Produce the required directions/rotations for Earth hopper from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [ ] 16.3 Add empty/partial/full load and purchase views for Earth hopper, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.

### Group 17 - Author Rock hopper

{{HARD}}
- [ ] 17.1 For Rock hopper, derive a single-model brief from its approved manifest/template: coarse rock load distinct from Earth. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [ ] 17.2 Produce the required directions/rotations for Rock hopper from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [ ] 17.3 Add empty/partial/full load and purchase views for Rock hopper, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.

### Group 18 - Author spoil hauler locomotive

{{HARD}}
- [ ] 18.1 For spoil hauler locomotive, derive a single-model brief from its approved manifest/template: secondary hauling engine distinct from the cutter. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [ ] 18.2 Produce the required directions/rotations for spoil hauler locomotive from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [ ] 18.3 Add running/idle/service and purchase views for spoil hauler locomotive, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.

### Group 19 - Author underground freight locomotive

{{HARD}}
- [ ] 19.1 For underground freight locomotive, derive a single-model brief from its approved manifest/template: conventional mineral-freight traction silhouette. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [ ] 19.2 Produce the required directions/rotations for underground freight locomotive from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [ ] 19.3 Add running/idle/service and purchase views for underground freight locomotive, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.

### Group 20 - Author mineral wagon

{{HARD}}
- [ ] 20.1 For mineral wagon, derive a single-model brief from its approved manifest/template: profile-compatible ore load presentation. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [ ] 20.2 Produce the required directions/rotations for mineral wagon from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [ ] 20.3 Add each required refit/load and purchase view for mineral wagon, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.

### Group 21 - Author mine truck

{{HARD}}
- [ ] 21.1 For mine truck, derive a single-model brief from its approved manifest/template: compact underground road freight body. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [ ] 21.2 Produce the required directions/rotations for mine truck from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [ ] 21.3 Add required load/refit/service and purchase views for mine truck, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.

### Group 22 - Author sealed mineral carrier

{{HARD}}
- [ ] 22.1 For sealed mineral carrier, derive a single-model brief from its approved manifest/template: deep-capable closed cargo body. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [ ] 22.2 Produce the required directions/rotations for sealed mineral carrier from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [ ] 22.3 Add required sealed-load/service and purchase views for sealed mineral carrier, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.

### Group 23 - Author TBM exchange bay

{{HARD}}
- [ ] 23.1 For TBM exchange bay, derive a single-model brief from its approved manifest/template: stopped-consist exchange space with readable approaches. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [ ] 23.2 Produce the required directions/rotations for TBM exchange bay from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [ ] 23.3 Add occupied/available and directional back/front parts for TBM exchange bay, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.

### Group 24 - Author underground rail freight station

{{HARD}}
- [ ] 24.1 For underground rail freight station, derive a single-model brief from its approved manifest/template: loading platform and mine loader aligned to rail. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [ ] 24.2 Produce the required directions/rotations for underground rail freight station from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [ ] 24.3 Add all approaches/loading and back/front parts for underground rail freight station, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.

### Group 25 - Author road mine terminal

{{HARD}}
- [ ] 25.1 For road mine terminal, derive a single-model brief from its approved manifest/template: truck loading and turning access. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [ ] 25.2 Produce the required directions/rotations for road mine terminal from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [ ] 25.3 Add all approaches/loading and back/front parts for road mine terminal, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.

### Group 26 - Author underground service depots

{{HARD}}
- [ ] 26.1 For underground service depots, derive a single-model brief from its approved manifest/template: distinct rail and road service entry templates. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [ ] 26.2 Produce the required directions/rotations for underground service depots from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [ ] 26.3 Add open/occupied/service and required approach views for underground service depots, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.

### Group 27 - Author Coal mine

{{HARD}}
- [ ] 27.1 For Coal mine, derive a single-model brief from its approved manifest/template: Underground machinery/loading silhouette with Coal identity. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [ ] 27.2 Produce the required directions/rotations for Coal mine from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [ ] 27.3 Add construction/working/paused/depleted layout states for Coal mine, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.

### Group 28 - Author Iron mine

{{HARD}}
- [ ] 28.1 For Iron mine, derive a single-model brief from its approved manifest/template: Underground machinery/loading silhouette with Iron identity. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [ ] 28.2 Produce the required directions/rotations for Iron mine from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [ ] 28.3 Add construction/working/paused/depleted layout states for Iron mine, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.

### Group 29 - Author Copper mine

{{HARD}}
- [ ] 29.1 For Copper mine, derive a single-model brief from its approved manifest/template: Underground machinery/loading silhouette with Copper identity. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [ ] 29.2 Produce the required directions/rotations for Copper mine from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [ ] 29.3 Add construction/working/paused/depleted layout states for Copper mine, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.

### Group 30 - Author Bauxite mine

{{HARD}}
- [ ] 30.1 For Bauxite mine, derive a single-model brief from its approved manifest/template: Underground machinery/loading silhouette with Bauxite identity. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [ ] 30.2 Produce the required directions/rotations for Bauxite mine from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [ ] 30.3 Add construction/working/paused/depleted layout states for Bauxite mine, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.

### Group 31 - Author Salt mine

{{HARD}}
- [ ] 31.1 For Salt mine, derive a single-model brief from its approved manifest/template: Underground machinery/loading silhouette with Salt identity. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [ ] 31.2 Produce the required directions/rotations for Salt mine from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [ ] 31.3 Add construction/working/paused/depleted layout states for Salt mine, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.

### Group 32 - Author Lithium mine

{{HARD}}
- [ ] 32.1 For Lithium mine, derive a single-model brief from its approved manifest/template: Deep machinery/loading silhouette with Lithium identity. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [ ] 32.2 Produce the required directions/rotations for Lithium mine from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [ ] 32.3 Add construction/working/paused/depleted layout states for Lithium mine, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.

### Group 33 - Author Nickel mine

{{HARD}}
- [ ] 33.1 For Nickel mine, derive a single-model brief from its approved manifest/template: Deep machinery/loading silhouette with Nickel identity. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [ ] 33.2 Produce the required directions/rotations for Nickel mine from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [ ] 33.3 Add construction/working/paused/depleted layout states for Nickel mine, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.

### Group 34 - Author Cobalt mine

{{HARD}}
- [ ] 34.1 For Cobalt mine, derive a single-model brief from its approved manifest/template: Deep machinery/loading silhouette with Cobalt identity. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [ ] 34.2 Produce the required directions/rotations for Cobalt mine from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [ ] 34.3 Add construction/working/paused/depleted layout states for Cobalt mine, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.

### Group 35 - Author Rare-earth mine

{{HARD}}
- [ ] 35.1 For Rare-earth mine, derive a single-model brief from its approved manifest/template: Deep machinery/loading silhouette with Rare-earth identity. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [ ] 35.2 Produce the required directions/rotations for Rare-earth mine from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [ ] 35.3 Add construction/working/paused/depleted layout states for Rare-earth mine, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.

### Group 36 - Author Uranium mine

{{HARD}}
- [ ] 36.1 For Uranium mine, derive a single-model brief from its approved manifest/template: Deep machinery/loading silhouette with Uranium identity. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [ ] 36.2 Produce the required directions/rotations for Uranium mine from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [ ] 36.3 Add construction/working/paused/depleted layout states for Uranium mine, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.

### Group 37 - Author Earth reclamation/disposal site

{{HARD}}
- [ ] 37.1 For Earth reclamation/disposal site, derive a single-model brief from its approved manifest/template: profile-selected Surface receiver footprint and recipe cues. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [ ] 37.2 Produce the required directions/rotations for Earth reclamation/disposal site from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [ ] 37.3 Add construction/working/full/idle states for Earth reclamation/disposal site, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.

### Group 38 - Author Rock crusher/disposal site

{{HARD}}
- [ ] 38.1 For Rock crusher/disposal site, derive a single-model brief from its approved manifest/template: profile-selected Surface receiver footprint and recipe cues. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [ ] 38.2 Produce the required directions/rotations for Rock crusher/disposal site from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [ ] 38.3 Add construction/working/full/idle states for Rock crusher/disposal site, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.

### Group 39 - Author metal refinery

{{HARD}}
- [ ] 39.1 For metal refinery, derive a single-model brief from its approved manifest/template: profile-selected Surface receiver footprint and recipe cues. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [ ] 39.2 Produce the required directions/rotations for metal refinery from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [ ] 39.3 Add construction/working/full/idle states for metal refinery, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.

### Group 40 - Author chemical works

{{HARD}}
- [ ] 40.1 For chemical works, derive a single-model brief from its approved manifest/template: profile-selected Surface receiver footprint and recipe cues. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [ ] 40.2 Produce the required directions/rotations for chemical works from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [ ] 40.3 Add construction/working/full/idle states for chemical works, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.

### Group 41 - Author Surface battery works

{{HARD}}
- [ ] 41.1 For Surface battery works, derive a single-model brief from its approved manifest/template: profile-selected Surface receiver footprint and recipe cues. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [ ] 41.2 Produce the required directions/rotations for Surface battery works from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [ ] 41.3 Add construction/working/full/idle states for Surface battery works, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.

### Group 42 - Author electronics works

{{HARD}}
- [ ] 42.1 For electronics works, derive a single-model brief from its approved manifest/template: profile-selected Surface receiver footprint and recipe cues. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [ ] 42.2 Produce the required directions/rotations for electronics works from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [ ] 42.3 Add construction/working/full/idle states for electronics works, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.

### Group 43 - Author component manufacture

{{HARD}}
- [ ] 43.1 For component manufacture, derive a single-model brief from its approved manifest/template: profile-selected Surface receiver footprint and recipe cues. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [ ] 43.2 Produce the required directions/rotations for component manufacture from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [ ] 43.3 Add construction/working/full/idle states for component manufacture, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.

### Group 44 - Author robotics manufacture

{{HARD}}
- [ ] 44.1 For robotics manufacture, derive a single-model brief from its approved manifest/template: profile-selected Surface receiver footprint and recipe cues. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [ ] 44.2 Produce the required directions/rotations for robotics manufacture from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [ ] 44.3 Add construction/working/full/idle states for robotics manufacture, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.

### Group 45 - Author mirror-kit manufacture

{{HARD}}
- [ ] 45.1 For mirror-kit manufacture, derive a single-model brief from its approved manifest/template: profile-selected Surface receiver footprint and recipe cues. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [ ] 45.2 Produce the required directions/rotations for mirror-kit manufacture from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [ ] 45.3 Add construction/working/full/idle states for mirror-kit manufacture, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.

### Group 46 - Author nuclear plant

{{HARD}}
- [ ] 46.1 For nuclear plant, derive a single-model brief from its approved manifest/template: profile-selected Surface receiver footprint and recipe cues. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [ ] 46.2 Produce the required directions/rotations for nuclear plant from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [ ] 46.3 Add construction/working/full/idle states for nuclear plant, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.

### Group 47 - Author tube guideways and junctions

{{HARD}}
- [ ] 47.1 For tube guideways and junctions, derive a single-model brief from its approved manifest/template: sealed shells that leave moving capsules readable. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [ ] 47.2 Produce the required directions/rotations for tube guideways and junctions from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [ ] 47.3 Add straight/45-degree bend/junction/end and powered/unpowered pieces for tube guideways and junctions, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.

### Group 48 - Author tube terminals and service/interchange facilities

{{HARD}}
- [ ] 48.1 For tube terminals and service/interchange facilities, derive a single-model brief from its approved manifest/template: distinct powered terminal/depot/rail-road transfer templates. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [ ] 48.2 Produce the required directions/rotations for tube terminals and service/interchange facilities from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [ ] 48.3 Add approach/occupied/unpowered fronts and backs for tube terminals and service/interchange facilities, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.

### Group 49 - Author Mole-70

{{HARD}}
- [ ] 49.1 For Mole-70, derive a single-model brief from its approved manifest/template: low segmented amber hopper ribs. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [ ] 49.2 Produce the required directions/rotations for Mole-70 from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [ ] 49.3 Add eight-direction empty/loaded/service and purchase frames for Mole-70, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.

### Group 50 - Author Crate-75

{{HARD}}
- [ ] 50.1 For Crate-75, derive a single-model brief from its approved manifest/template: square green clamped containers. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [ ] 50.2 Produce the required directions/rotations for Crate-75 from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [ ] 50.3 Add eight-direction empty/loaded/service and purchase frames for Crate-75, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.

### Group 51 - Author Cistern-80

{{HARD}}
- [ ] 51.1 For Cistern-80, derive a single-model brief from its approved manifest/template: cyan twin tanks and pressure bands. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [ ] 51.2 Produce the required directions/rotations for Cistern-80 from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [ ] 51.3 Add eight-direction fluid-level/service and purchase frames for Cistern-80, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.

### Group 52 - Author Shield-85

{{HARD}}
- [ ] 52.1 For Shield-85, derive a single-model brief from its approved manifest/template: short pale shielded octagonal body. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [ ] 52.2 Produce the required directions/rotations for Shield-85 from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [ ] 52.3 Add eight-direction sealed-load/service and purchase frames for Shield-85, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.

### Group 53 - Author Forge-05

{{HARD}}
- [ ] 53.1 For Forge-05, derive a single-model brief from its approved manifest/template: broad dark braced machinery frame. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [ ] 53.2 Produce the required directions/rotations for Forge-05 from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [ ] 53.3 Add eight-direction heavy-load/service and purchase frames for Forge-05, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.

### Group 54 - Author vent field and collector

{{HARD}}
- [ ] 54.1 For vent field and collector, derive a single-model brief from its approved manifest/template: natural source versus installed bounded heat capture. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [ ] 54.2 Produce the required directions/rotations for vent field and collector from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [ ] 54.3 Add survey/excavated/connected/idle states for vent field and collector, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.

### Group 55 - Author Vent-Tap Powerhouse

{{HARD}}
- [ ] 55.1 For Vent-Tap Powerhouse, derive a single-model brief from its approved manifest/template: 1980 plant footprint and electrical output equipment. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [ ] 55.2 Produce the required directions/rotations for Vent-Tap Powerhouse from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [ ] 55.3 Add construction/working/heat-starved states for Vent-Tap Powerhouse, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.

### Group 56 - Author Magma-Loop Power Station

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

{{HARD}}
- [ ] 78.1 For construction and cargo controls, derive a single-model brief from its approved manifest/template: native-size layer/cargo identities with separate collection/drop circles. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [ ] 78.2 Produce the required directions/rotations for construction and cargo controls from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [ ] 78.3 Add menu/cursor/locked/blocked/turn-preview and resource icons for construction and cargo controls, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.

### Group 79 - Convert and pack from stable originals

{{HARD}}
- [ ] 79.1 Extend deterministic quantization/cropping to consume manifest records, preserve index-0 transparency and handle reserved company/animation colors explicitly. Preserve world anchor when transparent margins are cropped.
- [ ] 79.2 Generate back/front parts and NFO/content-package metadata in stable manifest order with pinned tool versions. Keep existing published sprite offsets unchanged.
- [ ] 79.3 Pack the selected core/content routes and regenerate required GRF/checksum metadata. Compare two clean conversions from the same originals/manifest for identical outputs.
- [ ] 79.4 Verify a build without the asset generator loads the checked-in generated payload and resolves the same IDs/counts; source-only success is not distributed-asset acceptance.

### Group 80 - Validate asset structure and palette deterministically

{{HARD}}
- [ ] 80.1 Validate required record fields, unique IDs, expected direction/frame/state Cartesian coverage and exact native dimensions/anchors before packing. Report the failing asset ID and source.
- [ ] 80.2 Check transparent index, forbidden palette ranges, intended company-color pixels and empty/missing frames against the declared per-asset policy.
- [ ] 80.3 Compare manifest packed references and output hashes against emitted sprite count/offset metadata; reject stale or unreferenced gameplay assets.
- [ ] 80.4 Add small corruption fixtures for one missing frame, bad bounds and illegal palette entry. Validate authored transparent holes against explicit masks/templates; arbitrary legitimate holes still need human visual review.

### Group 81 - Source generation and reproducible conversion

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

### Group 82 - Integrate M1 terrain and portal states

{{HARD}}
- [ ] 82.1 Verify r2mlvs2/3 state queries and matching manifest entries, then bind Solid/Excavating/Excavated terrain and Sealed/Commissioning/Connected portal graphics to authoritative state.
- [ ] 82.2 Check adjacent tile edges, both depth palettes, every portal orientation and extra-view layer filtering in runtime scenes. The renderer does not advance excavation.
- [ ] 82.3 Capture frame evidence for an assigned TBM entering/reversing through a sealed work pocket; validate front/back occlusion at each crossing phase.

### Group 83 - Integrate M1 cutter, wagons and exchange facility

{{HARD}}
- [ ] 83.1 Bind r2mlvs4 work phase and heading to cutter/support/boring sprites, preserving directional continuity through legal 45-degree turns and supported retreat.
- [ ] 83.2 Bind actual Earth/Rock load quantities to wagon states and r2mlvs5 stopped exchange to bay foregrounds; load graphics follow the same wagon IDs after coupling.
- [ ] 83.3 Capture a real dig/fill/exchange/Surface-haul/empty-return loop and inspect clipping at native/zoomed scales. Correct assets or anchors, repack and retain reviewed hashes before claiming M1 graphics complete.

### Group 84 - Runtime integration and review

{{MEDIUM}}
- [ ] 84.1 Integrate all ten mines and the underground conventional fleet after their r2mlvs6/7 code is delivered.
- [ ] 84.2 Integrate orbital launch, probe, module and solar families after their corresponding r2mlvs8..12 producers are delivered.
- [ ] 84.3 Integrate tube/geothermal art after r2mlvs16/17 producers are delivered; this later work does not block M1 art.

### Group 85 - Runtime integration and review

{{SIMPLE}}
- [ ] 85.1 Capture native-scale and zoomed scenes across directions, terrain boundaries, load/production states, vehicle overlap and building transparency.

### Group 86 - Runtime integration and review

{{MEDIUM}}
- [ ] 86.1 Use real runtime frames for animation/occlusion claims.
- [ ] 86.2 Check each launch center/module footprint as a complete site, not isolated tiles.
- [ ] 86.3 Verify correct vehicle hiding at bays, portals and docks, drawing on the correct layer and clickable moving bodies.

### Group 87 - Runtime integration and review

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
