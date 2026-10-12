# R2MLVS13 - Completed graphics foundations and assets

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

For any new raster generation/edit or source-layout correction, also read [Framed ImageGen sheets and safe extraction](#framed-imagegen-sheets-and-safe-extraction). This policy applies throughout the source-authoring groups, including groups with a closer family-specific Local Mandatory Reading block. Existing approved originals retain their provenance and do not require regeneration.

Status,2026-10-10: this completed record retains all173 checked tasks from the original graphics plan: the Sprite Contract, selected M1 sources/runtime evidence, later tube/collector sources and collector payload79C.1-79C.2. All169 unchecked tasks are preserved in [Plan14 - Remaining assets and runtime integration](r2mlvs14-remaining-assets-and-runtime-integration.md). Task IDs and dated evidence remain unchanged. This completes the retained portion only; the whole graphics profile remains incomplete. The implementation goal remains paused; resume Plan14 task79C.3 only when requested.

**Artifact-layout migration (2026-10-09):** this plan consumes the parent [Development artifact layout](r2mlvs-multilayer-vehicles-and-structures.md#development-artifact-layout), implemented by [R2FC](r2fc-file-cleanup.md). It supersedes earlier docs asset/review/helper placement and fixed runtime-config destinations below. New native homes use `mlm/runtime/`, generated feature reviews use `mlm/reviews/`, logs remain under `logs/`, and reusable code/curated inputs keep their code/test owners. Paths and focused native/cold verification are complete; R2FC records the results and retained-input identities. Repository-relative executable commands run from the checkout root through their context-aware wrappers. Historical pass records remain dated evidence and are not new acceptance claims. No gameplay task is completed by this migration and the separate post-demo fixture-pruning gate remains.

## Outcome and Boundaries

R2TBMF10 material-art supplement,2026-10-09: two immutable built-in-generated originals add16 hardness/state tiles and16 transparent hardness/side walls. `GetMultiLayerTerrainSprite` reads the same `GetExcavationCellHardness` used by quotes/cutting/survey; `GetMultiLayerTerrainEdgeSprites` reads each adjacent uncut material. The old four terrain families remain in their immutable packed ranges as superseded art. Two new families append at2404/2420, expanding selected packing to42 families/2343 sprites and the native bound to2436 while retaining original offsets0..65. Two identical GRF encodings, source-exact native loading, old/new saved-map rasters and unchanged simulation owners are verified in [Z](tbm-fixes/r2tbmf-z-geology-notes.md). Group10 presentation and bounded actual network acceptance are complete in that record; Group11 player feedback and the paused master's wider obligations remain separate.

Retain completed graphics contracts, authored sources, packed payloads and selected native rendering evidence. Plan14 owns the remaining original implementation/acceptance scope. The asset-family table below is the shared requirements catalogue; listing a family does not claim its artwork is delivered.

## Asset families

| Family | Minimum content and state coverage | Consumers |
| --- | --- | --- |
| Underground terrain | Solid rock and cut face; excavated floor; material-owned edges; progress state; Soft/Mixed/Hard/Basalt art on both rock layers; ten independent survey/resource markers | 2, 6 |
| Portals | Existing 4-direction housings retained; sealed/commissioning/connected faces; Deep rail variant; work entrance and exchange overlays | 3 |
| TBMs and rail haulage | Standard/deep cutter and support parts; drilling animation; Earth/Rock wagons empty/partial/full; hauler; 8 vehicle directions and purchase sprites | 4, 5, 7 |
| Underground facilities | Exchange bay, rail freight station/loading platform, road terminal, train/road service depot; 4 approach directions where applicable; back/front occlusion | 5, 7 |
| Mines and Surface receivers | 5 Underground + 5 Deep mine identities; construction/working/depleted states; metal/chemical/battery/electronics/component/nuclear/disposal/crusher functions as required by content | 6 |
| Underground road fleet | Mine truck and deep mineral carrier; 8 directions, refit/load states and purchase sprites | 7 |
| Transport tubes | Straight/45-degree bend/junction/end guideways; powered/unpowered states; terminal/service/interchange fronts and backs; excavation/route controls | 7, 17 |
| Original tube fleet | Mole-70, Crate-75, Cistern-80, Shield-85 and Forge-05: 5 distinct silhouettes/models, each with 8 directions, purchase icon and cargo/load states; 5 refits or recolors are insufficient | 17 |
| Geothermal sites and factories | Vent field/collector; Vent-Tap and Magma-Loop powerhouses; direct smelter, battery works and Mantle Forge; utility conduits/riser; construction/working/heat-starved states | 18 |
| Space Launch Center | Pad, assembly/service building, cargo/recovery areas; footprint rotations; ready/preparing/launch/occupied states; foreground cuts | 9 |
| Space fleet | Cargo rocket ascent/orbit/recovery; small probe free/attached/mining; return pod; heavy robot and orbital hauler; direction and purchase/state sprites | 9..11 |
| Asteroids | 3 readable classes, size variants, docking/mining overlays and depleted state; animation must preserve gameplay footprint | 10 |
| Orbital modules | Core, dock, storage, utility, assembly and processor; blueprint/partial/complete states, connectors and docking occlusion | 11 |
| Solar | Original solar farm and mirror kit icon; mirror packed/deploying/operating/idle; farm boost indicator | 12 |
| Controls and cargos | Underground/Space Construction menus, general TBM area/45-degree turn previews, tube/geothermal tools, 1980/later technology locks, launch-center collection and separate 20-tile orbital-drop overlays, vehicle commands, cargo/resource icons and blocked-state markers | all |

Exact sprite counts depend on footprints, rotations, load stages and animation frames. Freeze them in the first reading area and Sprite Contract Handoff before batch generation. The family table is a coverage obligation, not a claim that one image per row is enough.

## Public Handoff Contract

Delivered public manifest: `media/multilayer/contracts/multilayer_sprite_manifest.json`, retaining the completed R2FC layout. Groups1-8 publish `asset_id`, `consumer`, `directions`, `frames`, `anchor`, `source` and validated family geometry. This plan retains the foundational schema and completed producer evidence; [Plan14](r2mlvs14-remaining-assets-and-runtime-integration.md) extends remaining asset coverage through that same manifest. Downstream plans consume the public contract rather than private pools or copied tables.

`Sprite Contract Handoff`: the first reading area publishes exact family/direction/frame/footprint/anchor/foreground requirements and the public manifest before content freeze. Generation and full runtime gallery follow consumer implementation; no blanket human-acceptance skip is inherited.

## Implementation Tasks

### Area Mandatory Reading - Art and runtime contract

_Applies to Groups 1-8._

**Entry gate.** The following public contracts must exist before dependent implementation in this area: `src/multilayer_content.h`: `MultiLayerGameplayProfile`, `LayerVehicleCapability`, `GetMultiLayerContentProfile`, `CheckDeepLayerAvailability`, `UtilityPowerAccount`.
Verify each public file/export/schema field with `rg` in the current workspace and inspect its command/field ownership. For a same-plan contract, compile its type/signature fixture or validate its artifact after the preceding producer groups. For cross-plan behavior, also run the producer's focused executable boundary test; declarations alone cannot authorize dependent mutations or runtime claims.
If a required export is absent, stop the dependent implementation, mutation and runtime claims in Groups 1-8. Read-only characterization, source/range verification, test-case design and asset-template work that does not assume the missing behavior may continue.
Do not bypass this gate by copying private pool/tile state, inventing local IDs or cargo registries, using a historical plan as a delivered implementation, opening user-wide content stores, substituting a second utility/accounting authority, or adding an unreviewed fallback path. Finish the named producer or revise the shared contract and its direct consumers first.

| File name | Line Ranges | Description |
| --- | --- | --- |
| [src/newgrf_act5.h](../../../../src/newgrf_act5.h) | 15-31 | `Action5BlockType`, `Action5Type`, `GetAction5Types` define the public block/replacement model; do not reinterpret existing offsets or GRF compatibility. |
| [media/multilayer/tools/sprite_quantize.py](../../../../media/multilayer/tools/sprite_quantize.py) | 29-46 | `CONTRACT_RANGES` ends at 65 and records an unactivated optional 66..73 Deep housing range in a comment; manifest planning must resolve this live extension instead of blindly starting at 66. |
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
- [X] 8.1 Publish `media/multilayer/contracts/multilayer_sprite_manifest.json` with exact `asset_id`, `consumer`, `directions`, `frames`, `anchor`, `source` and the additional validated schema fields. Keep it repository-local and link from consumer plans.
- [X] 8.2 Run schema, duplicate-ID, coverage-count and source/geometry-template checks. Mark planned source assets as undelivered explicitly rather than pointing to nonexistent final files.
- [X] 8.3 Record the M0 Sprite Contract Handoff as dimensions/directions/state requirements plus representative template evidence. Actual family generation, packing and runtime acceptance remain pending in later groups.


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

Owner: this plan's source-authoring/conversion groups; consumers: the feature plans' graphics tasks and Plan16 art review. Adopt this layout for future multi-view raster requests, with one image per view as the fallback when a sheet fails separation or model consistency. It is a source-layout convention; required directions, states, native dimensions, anchors and foreground parts still come from the family's manifest and actual consumer geometry. Eight-direction vehicles need all eight genuine views, for example in two explicitly labelled four-view batches; terrain, icons and other matrices keep their own contract. Do not regenerate approved sources merely to add frames.

**Evidence and limits, 2026-10-10.** The Vent-Tap stage 2 trial allows four fixed rectangular crops with zero clipped opaque model pixels and zero opaque magenta pixels inside the crops. The NW view nevertheless adds a second exchanger, so the whole trial remains unapproved and runtime-inactive. This supports rectangular extraction; every generated output still requires layout and model checks. The [layout receipt](../../../../logs/graphics/geothermal-source/20261010-venttap-layout-01/verification.json) records the measured windows; [the immutable generation record](../../../../media/multilayer/source/ground/geothermal-generation-record.json), candidate `venttap-construction-2-bordered-v1`, retains the exact executed prompt, references, original hash and rejection. Its [layout guide](../../../../media/multilayer/guides/ground/generator-venttap-construction-2-bordered-v1.png) is a layout reference, never gameplay artwork.

**Reusable ImageGen prompt.** This copyable template preserves the successful trial's framing instructions. Fill every placeholder from the current family brief. For an edit, attach the approved views as edit targets and the frame guide as a layout-only reference; inspect local targets first under the imagegen skill. For a new model, use `stylized-concept`, describe genuine reference roles and omit claims of approved edit targets. Match the view labels and camera to the actual engine contract.

```text
Use case: precise-object-edit.
Asset: <asset_id and model>; requested state: <one construction stage or equipment state>.
Input images: Images 1-4 are the approved <NE,SE,SW,NW> model edit targets;
Image 5 is the exact four-cell magenta-frame layout reference, for layout only.

Create a SAFE BORDERED source sheet, 1536x1024, with four complete views of
the same physical model in a 2x2 grid: NE top-left, SE top-right,
SW bottom-left, NW bottom-right. Keep four straight opaque PURE MAGENTA
#FF00FF rectangular frames, approximately 6px thick:
NE(24,24)-(744,488), SE(792,24)-(1512,488),
SW(24,536)-(744,1000), NW(792,536)-(1512,1000).
Magenta is a source separator outside every model.

Each whole subject must fit comfortably inside its corresponding inner
rectangle with at least 32px transparent clearance to its frame:
NE(56,56)-(712,456), SE(824,56)-(1480,456),
SW(56,568)-(712,968), NW(824,568)-(1480,968).
Reduce each model uniformly as necessary: roughly 560px floor width,
maximum 350px whole-subject height. No floor corner, machine, halo,
shadow or other subject pixel may touch a frame, enter a gutter,
cross the panel boundary or overlap another view.
Use real transparency throughout the background, margins and gutters;
keep the magenta frames visible.

Change only <the declared construction/state differences>.
Preserve <the exact equipment inventory, counts, placement and silhouette>
in every physical orientation. Preserve <native footprint and contact points>,
<camera, normally fixed 2:1 isometric>, <light direction, normally upper-left>
and <approved materials and palette accents>.
Do not add, duplicate, remove, mirror or replace equipment beyond the
declared state change. Produce genuine directional views of the same model.
No text, labels, numbers, guide symbols, decorative background or extra props.
Four independent, fully separated cutouts; safe separation takes priority
over making the subjects large.
```

Use the built-in `image_gen` tool with `transparent_background=true`; the prompt's requested canvas is not a promise of the returned dimensions. Retain the original RGBA output immutably, plus the complete substituted prompt, tool, view/state labels, reference roles/hashes and selected/rejected status in the family's generation record. Copy project originals into the repository under the existing asset owner. Keep construction/state changes model-specific; the magenta layout does not authorize procedural substitutes for missing authored views or states.

| View | Frame corners requested | Safe source crop in the 1536x1024 trial |
| --- | --- | --- |
| NE | (24,24)-(744,488) | [32,32,736,480) |
| SE | (792,24)-(1512,488) | [800,32,1504,480) |
| SW | (24,536)-(744,1000) | [32,544,736,992) |
| NW | (792,536)-(1512,1000) | [800,544,1504,992) |

Crop notation is `[left,top,right,bottom)`, with right/bottom exclusive. These source windows exclude the frames; native per-tile slicing happens later. The inner rectangles in the prompt reserve additional subject clearance inside these crop windows. Do not copy source coordinates into native anchors or assume another generated sheet obeys this layout.

Before selecting a sheet or passing it to conversion:

1. Check the actual dimensions, alpha, frame locations and transparent gutters. Record measured non-overlapping rectangular source windows and the alpha/color thresholds used by the check. Verify that every visible model pixel lies inside its own window, all separator pixels lie outside the windows, and no opaque separator magenta enters any extracted view. Inspect faint edges and shadows for clipping too. A requested frame or successful crop of a different sheet is insufficient.
2. Compare all views with the approved model: equipment counts and positions, genuine orientation, requested construction/state differences, footprint, camera/light and native-scale readability. Reject a sheet that adds hardware or changes identity even when its cropping passes.
3. Extract ordinary rectangles that exclude the source frames, preserving the model alpha. Then apply the established palette, registration, native-cell and foreground-part conversion checks. Do not rescue overlapping subjects with guessed pixel masks, erase magenta across the whole image, or ship separator frames. On failure, regenerate a corrected framed sheet or request one isolated view per image with the same geometry references.
4. Keep extraction/model approval, complete source coverage, packed/decoded sprite proof and actual game rendering as separate results. Source checks alone do not close runtime tasks. Follow the master's bounded source-to-game checkpoints before starting another large authoring batch.

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

Source-art pass, 2026-10-06: [Plans 1-5 artwork record](graphics/multilayer-m1-art-pass.md)
retains 14 built-in-generated originals and 17 indexed atlases (247 staged
poses/previews), with exact prompts, hashes and native previews. The public
manifest now links those authored/partial sources. Only Earth/Rock cargo icons
have complete source coverage; vehicle animation and building geometry/slicing
remain unfinished. Runtime packing/integration and all full-plan art acceptance
remain pending. This bounded pass does not complete these global checkboxes.

M1 preparation, 2026-10-06: [selected source/geometry record](graphics/r2mlvs13-b-m1-authoring.md) verifies the available built-in tool/installed skill, all 14 immutable original hashes and existing atlas checks, and publishes 28 bounded M1 briefs. Native consumer geometry passes 47 assertions: both cutters need distinct permanent support art; one-tile depot families, 2x3 Earth and 4x4 Rock replace unsupported draft rectangles. Public manifest/publisher now require 57,163 sprites across the same 141 families while preserving original provenance, protected offsets and payload identity. The user explicitly authorized built-in image outputs in CODEX_HOME on 2026-10-07, with selected originals copied into this repository. These preparation results do not mark missing directions/animation/per-cell art or the whole global authoring tasks complete.

{{MEDIUM}}
- [X] 9.1 For the selected M1 slice, read/use the available image-generation skill/tool for AI raster work and follow its editing/reference requirements. Keep prompts, references, chosen originals and provenance in repository asset directories; this preparation generates no art.
- [X] 9.2 Use the approved geometry/style templates to write one bounded generation brief per selected M1 family/model: view labels, light direction, silhouette, material accents, state differences, transparent background and native target footprint.
- [X] 9.3 Keep selected M1 original outputs immutable and record selection/revision history separately from deterministic conversion. Generative output is not assumed reproducible from a seed; checked-in originals make subsequent packing reproducible.

Selected M1 preparation closure (2026-10-07): 28 briefs, all 14 retained original hashes, the public 141-family schema and actual native geometry are verified in [the scoped authoring record](graphics/r2mlvs13-b-m1-authoring.md). This closes master 8.1; source pixel coverage and final runtime review remain with their later owning groups.


### Local Mandatory Reading - M1 terrain originals

_Applies to Groups 10-11; supplements the Source generation Area._

Read the active terrain brief and manifest, retained terrain original/provenance, geometry template's flat diamond, and actual excavation rendering's state/neighbor/survey mapping. Use the approved tool for new raster pixels; deterministic crop/palette conversion follows selection. Ground masks must follow world neighbors and fixed anchors, with no camera-driven simulation changes.


### Group 10 - Author Underground terrain

{{HARD}}
- [X] 10.1 For Underground terrain, derive a single-model brief from its approved manifest/template: solid face versus cavity with readable edges and survey markers. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [X] 10.2 Produce the required directions/rotations for Underground terrain from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [X] 10.3 Add all cut/progress/edge/corner and five resource-marker states for Underground terrain, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.

Group 10 source handoff (2026-10-07): the retained 12 base-state/texture poses are selected at native 64x31 / anchor (-31,0). Terrain uses one fixed engine camera, so no rotated camera views are required. Two fresh built-in originals add all 16 neighbor-rim combinations and five distinct resource markers in unknown/surveyed/depleted states. [Prompts, references, immutable cache/copy hashes and selection](../../../../media/multilayer/source/m1/terrain-generation-record.json), [native 1x/4x sheet](../../../../mlm/reviews/multilayer-art/authoring/import-20261009/images/terrain-details-review.png), and `convert_terrain_art.py --check` prove deterministic alpha/palette/coverage/bounds for 31 supplemental poses. Source art is handed to later packing; runtime and user acceptance remain pending. Two explicit edge-mask families append planned offsets 168..199 after all original planned offsets, preserving live 0..65 and reserved 66..73; the public contract now has 143 families / 57,195 required source sprites. The selected M1 briefs expand from 28 to 40 to include the two edge families and ten resource markers already required by terrain tasks.


### Group 11 - Author Deep terrain

{{HARD}}
- [X] 11.1 For Deep terrain, derive a single-model brief from its approved manifest/template: distinct basalt palette with the same cavity grammar. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [X] 11.2 Produce the required directions/rotations for Deep terrain from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [X] 11.3 Add all cut/progress/edge/corner and five Deep resource-marker states for Deep terrain, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.

Group 11 source handoff (2026-10-07): retained native basalt base/texture states are selected; terrain has a fixed camera. Two fresh built-in originals supply all 16 Deep neighbor rims and LITH/NORE/COBT/RARE/URAN markers in three disclosure states. The shared [provenance record](../../../../media/multilayer/source/m1/terrain-generation-record.json) and [native 1x/4x review](../../../../mlm/reviews/multilayer-art/authoring/import-20261009/images/terrain-details-review.png) retain all prompts, reference/source hashes and selections. Focused conversion validates 62 supplemental poses across both depths; actual packaging/rendering stays with Groups 79-83.


### Local Mandatory Reading - M1 portal source geometry

_Applies to Groups 12-13; supplements the Source generation Area._

Read the active portal brief/manifest, immutable portal original/provenance, `LayerPortalWorkState` in layer_portal_cmd.h, `DrawTile_LayerPortal` and its separator bounds in layer_portal_cmd.cpp, and the established portal front-mask conversion. Four actual door directions share a fixed world anchor. Surface/Underground or Underground/Deep endpoint variants are independent one-tile housings on distinct layers. Resolve the draft generic state aliases against actual Sealed/Commissioning/Connected before authoring; keep blocked status a presentation of the existing controller, never a new saved work phase. Far-facing roofs/walls must occlude vehicles; native runtime acceptance remains in later integration groups.


### Group 12 - Author sealed and commissioning portal overlays

{{HARD}}
- [X] 12.1 For sealed and commissioning portal overlays, derive a single-model brief from its approved manifest/template: existing four-direction housings with controlled work faces. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [X] 12.2 Produce the required directions/rotations for sealed and commissioning portal overlays from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [X] 12.3 Add Sealed/Commissioning/Connected fronts and backs for sealed and commissioning portal overlays, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.

Group 12 source handoff (2026-10-07): [selected sibling v2 and provenance](../../../../media/multilayer/source/m1/portal-generation-record.json) provide true four-direction housings in actual Sealed/Commissioning/Connected phases, independently for Surface/Underground endpoint tiles. The public publisher and briefs now match the static phase owner (one frame per phase); 123 focused owner assertions pass in [g](../../../../logs/r2mlvs13-g-portal-phase-contract.log). This replaces unsupported generic four-frame/four-state portal drafting, reducing total source requirements to 143 families / 56,779 while preserving all live/reserved/planned native offsets. Deterministic native 64x64 / (-31,-33) conversion stages 48 back/front parts with full far-roof coverage and explicit source-reviewed near apertures. [Native sheet](../../../../mlm/reviews/multilayer-art/authoring/import-20261009/images/portal-details-review.png) verifies door/state readability and aligned cuts; actual train sorting, packed loading and final acceptance remain with later groups. Original v1 pixels and atlases stay immutable.


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

Group 14 base-source handoff (2026-10-07): [selected body v2/support v3](../../../../media/multilayer/source/m1/vehicle-generation-record.json) have eight genuine engine views each, with source-reviewed per-view crop registration and native bounds/anchors. The support is a low generator/hose cart with no cab or second cutter. [Native partial sheet](../../../../mlm/reviews/multilayer-art/authoring/import-20261009/images/vehicle-details-review.png) stages 16 idle frame-zero poses. Two rejected candidates are retained with exact reasons/prompts/hashes and never fill coverage. The publisher/briefs now explicitly include backing and ordinary service for both cutters, and service for the hauler, as required by these source tasks (143 families / 57,051 required source sprites). Art animation creates no new saved work phase. Task 14.3 remains pending until the bounded body/support batches and 14D combined source review pass; 368 Standard poses are still missing. Actual loaded seam/occlusion review remains required during integration.


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

14B.1 source handoff (2026-10-07): four genuinely authored RGB equipment-cue phases now exist in all eight engine views. The tool's partial sheet contributes complete frames 1/2 only; the excluded partial row never fills a frame. A separate original supplies frame 3. Deterministic source-part assembly uses only selected cab-beacon regions onto the immutable idle chassis, as 14.3 requires changing declared parts only. Native checks independently prove identical alpha/contact geometry and exact pixels outside that cue, plus four distinct palette phases in every view. [Prompt/selection/crop/component provenance](../../../../media/multilayer/source/m1/vehicle-generation-record.json), [animation source review](../../../../mlm/reviews/multilayer-art/authoring/import-20261009/images/standard-idle-animation-review.gif), and [focused checks](../../../../logs/r2mlvs13-i-standard-idle-source-checks.log) retain evidence. Current Standard coverage is 40/384 (32 body idle plus 8 support idle); moving/boring/blocked/backing/service and support animations remain pending. This is source review, not a loaded gameplay capture.

Group 14B source handoff (2026-10-07): the retained chassis and eight genuine headings are reused for all **192 body poses**. Five new immutable tool outputs supply selected rolling gear, drilling teeth/work indicator, red wait and cyan service cues; the first movement sheet has 28 real cells and a separate four-cell source completes frame 3. Backing reuses the real rolling phases in inverse order without rotating the body. Explicit per-view source/target equipment registration excludes generated lamp/chassis drift and preserves the original alpha/couplers. [Focused checks](../../../../logs/r2mlvs13-j-standard-body-source-checks.log) prove four distinct native palette phases in every heading/state, exact regeneration and the public manifest. Source-only GIF reviews are linked through [provenance](../../../../media/multilayer/source/m1/vehicle-generation-record.json). Support coverage, combined model handoff, packed loading and runtime review remain later tasks. The user deferred art feedback until integration on 2026-10-07; this does not claim that review passed.


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

Group 14D source closure (2026-10-07): [native/enlarged sheet](../../../../mlm/reviews/multilayer-art/authoring/import-20261009/images/vehicle-details-review.png) and six state GIFs per part retain all **384/384 Standard poses**, with explicit source hash/cell/component ownership in [provenance](../../../../media/multilayer/source/m1/vehicle-generation-record.json). Body/support remain visually distinct; original eight-view geometry is unchanged outside declared equipment, including adjacent 45-degree headings. Missing coverage is zero and the public authoring record is complete-source/runtime-pending. [Technical checks](../../../../logs/r2mlvs13-k-standard-model-source-checks.log) pass. User feedback is deferred until integration by instruction; packed loading and physical articulated seam/occlusion review are not claimed. This closes original 14.3, while master 8.3 still requires Deep, Earth/Rock and hauler source groups.


### Group 15 - Author Deep-rated TBM

{{HARD}}
- [X] 15.1 For Deep-rated TBM, derive a single-model brief from its approved manifest/template: visibly heavier cutter and depth equipment. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [X] 15.2 Produce the required directions/rotations for Deep-rated TBM from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [X] 15.3 Add idle/boring/reversing/service frames with fixed articulated seams for Deep-rated TBM, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.

Group 15 source handoff (2026-10-07): all **384 Deep body/support poses** are baked. The retained red/graphite cutter and its genuine corrected rear views are reused from the immutable original/native source chain; transparent canvas translation aligns the existing poses to the common ground contact without repainting their body. One new tool-authored support original reuses the accepted cart geometry with reinforced Deep material cues and no cab/cutter. Shared authored steel, machinery and indicator phases are registered only inside the Deep model's own physical equipment; its heavier shell, heading, alpha and contacts stay fixed. [Body checks](../../../../logs/r2mlvs13-l-deep-body-source-checks.log), [support checks](../../../../logs/r2mlvs13-l-deep-support-source-checks.log), [provenance](../../../../media/multilayer/source/m1/vehicle-generation-record.json), and per-part/state source GIFs retain exact source/cell/component identities and four distinct native phases in every heading. Public source coverage is complete; packed/native coupling/occlusion review remains later. User art feedback is deferred by instruction.


### Group 16 - Author Earth hopper

{{HARD}}
- [X] 16.1 For Earth hopper, derive a single-model brief from its approved manifest/template: readable soil load and detachable wagon identity. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [X] 16.2 Produce the required directions/rotations for Earth hopper from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [X] 16.3 Add empty/partial/full load and purchase views for Earth hopper, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.

Group 16 source handoff (2026-10-07): **80 Earth poses** reuse the selected brown hopper, original true-axis view mapping and five retained load/status first poses. The second frame selects existing authored steel wheel details at explicit own-state positions; soil/load pixels, alpha and couplers stay exact. Runtime must use rolling phases only during actual movement, keep frame zero while stopped, and give actual load quantity priority over optional empty-service/wait views. [Focused checks](../../../../logs/r2mlvs13-m-earth-source-checks.log) pass five states/two native phases/eight headings and exact conversion/crop refusals. Available purchase art is reused; a single five-model tool original supplies separate locked UI previews with padlocks, never vehicle bodies. [Purchase provenance/review](../../../../media/multilayer/source/m1/purchase-generation-record.json) and [checks](../../../../logs/r2mlvs13-n-purchase-source-checks.log) prove ten native available/locked previews and manifest coverage. Full native/loaded cargo sorting and runtime art review remain later; user feedback is deferred.


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


### Local Mandatory Reading - M1 exchange source

_Applies to Group 23; master checkpoint 8.4._

Read this plan's Global and Source generation Area, the `facility.exchange` public manifest/brief, and the retained `exchange-bay-v1.png` source/provenance. In native source inspect `ResolveTBMExchangeBay` / `CheckTBMExchangeParticipant` in `src/tbm_exchange.cpp` and the depot branch of `DrawTile_Rail` in `src/rail_cmd.cpp`: this is one owned depot tile, with real consists hidden after entering, rather than the old candidate's 3x2 plaza. Keep four actual entry views and aligned back/front parts. Reuse the original steel/hoist/safety palette and preserve its bytes; author compact geometry and genuine presentation phases where missing. Housing/status animation never transfers cargo or grants exchange permission.


### Group 23 - Author TBM exchange bay

{{HARD}}
- [X] 23.1 For TBM exchange bay, derive a single-model brief from its approved manifest/template: stopped-consist exchange space with readable approaches. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [X] 23.2 Produce the required directions/rotations for TBM exchange bay from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [X] 23.3 Add occupied/available and directional back/front parts for TBM exchange bay, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.

23 source closure (2026-10-07): selected compact one-tile source reuses the original steel/hoist/safety design; the first long-plaza revision remains a rejected immutable candidate. Four actual entry views, four state cues and four authored indicator phases yield **128 aligned back/front sprites**. [Geometry boundary](../../../../logs/r2mlvs13-r-facility-geometry.log) passes 47 assertions; [focused source/placement/schema checks](../../../../logs/r2mlvs13-s-exchange-source-checks.log) pass. Originals/prompts are in [facility provenance](../../../../media/multilayer/source/m1/facility-generation-record.json); review is `mlm/reviews/multilayer-art/authoring/import-20261009/images/exchange-bay-details-review.png` plus four per-state GIFs. Native payload/rendering and user art feedback remain later checkpoints.


### Local Mandatory Reading - M1 ordinary service sources

_Applies to Group 26; master checkpoint 8.4._

Read this plan's Global and Source generation Area and only `facility.rail-service` / `facility.road-service` manifest/brief rows. Inspect native depot branches in `DrawTile_Rail` and `DrawTile_Road`, their one-tile build commands, and `src/underground_vehicle.h` for ordinary servicing ownership. Read the completed exchange source record/converter for reusable geometry and phase components. Rail has a single track entry; road has a distinct paved entry. Occupancy comes from real depot vehicles; visual service cues cannot create a second repair or vehicle registry.


### Group 26 - Author underground service depots

{{HARD}}
- [X] 26.1 For underground service depots, derive a single-model brief from its approved manifest/template: distinct rail and road service entry templates. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [X] 26.2 Produce the required directions/rotations for underground service depots from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [X] 26.3 Add open/occupied/service and required approach views for underground service depots, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.

26 source closure (2026-10-07): ordinary rail servicing reuses the exact compact rail original because the exchange owner assigns an existing native depot. The road family reuses its architecture with distinct teal fittings, asphalt/white entry stripe, no rails, and an internal service jack. Four actual entries and fixed construction/open/occupied-service/blocked views plus shared authored lamp phases supply **128 poses per family / 256 total**. [Focused service placement/phase/refusal checks](../../../../logs/r2mlvs13-t-service-source-checks.log) pass. Provenance and per-family PNG/four GIF reviews live beside the exchange originals; no new vehicle or repair authority is introduced. Native payload/rendering and personal art review remain pending.


### Local Mandatory Reading - M1 Surface spoil receiver sources

_Applies to Groups 37-38; master checkpoint 8.4._

Read this plan's Global and Source generation Area, only the executing receiver's manifest/brief, and `InstallSpoilReceiverDefinition`, `GetSpoilReceiverRoom`, `AdvanceSpoilReceiver` in `src/spoil_receiver.cpp` / public recipe/state in `src/spoil_receiver.h`. The measured native layouts are Earth 2x3/six cells and Rock 4x4/sixteen cells (`src/tests/multilayer_art_geometry.cpp`). Inspect the actual layout cells before selecting per-tile art. Industries expose one fixed layout, so unsupported view rotations must be reconciled with that producer rather than synthesized. Construction/idle/working/full cues present existing bounded stock/processing; animation cannot consume cargo. Keep per-cell 64x64 anchors/back/front coverage and assemble a whole-site review before source closure.


### Group 37 - Author Earth reclamation/disposal site

{{HARD}}
- [X] 37.1 For Earth reclamation/disposal site, derive a single-model brief from its approved manifest/template: profile-selected Surface receiver footprint and recipe cues. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [X] 37.2 Produce the required directions/rotations for Earth reclamation/disposal site from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [X] 37.3 Add construction/working/full/idle states for Earth reclamation/disposal site, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.

37 source closure (2026-10-07): the actual fixed native 2x3 layout has six column-major cells, with intake, brown soil bank, retaining drain, soil conveyor, receiving yard and service hut. `ready` presents idle; `blocked` presents full/interval refusal. No industry rotation exists: publisher/validator and both receiver briefs now declare only `none`, reducing the exact whole-profile requirement to **143 families / 54,939 sprites** without altering native offsets/content identity. Four source states, shared authored lamp phases and paired per-cell parts supply **192 poses**; static soil/paving repeats intentionally while the hut fitting animates. [Actual receiver boundaries](../../../../logs/r2mlvs13-u-receiver-boundary.log) pass 2,560 assertions / seven cases; [focused Earth source/contact/refusal/schema checks](../../../../logs/r2mlvs13-v-earth-source-checks.log) pass. Whole-site PNG/GIF review is in `mlm/reviews/multilayer-art/authoring/import-20261009/images/earth-receiver-details-review.png`. Runtime loading/stock-state selection and user art review remain pending.


### Group 38 - Author Rock crusher/disposal site

{{HARD}}
- [X] 38.1 For Rock crusher/disposal site, derive a single-model brief from its approved manifest/template: profile-selected Surface receiver footprint and recipe cues. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [X] 38.2 Produce the required directions/rotations for Rock crusher/disposal site from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [X] 38.3 Add construction/working/full/idle states for Rock crusher/disposal site, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.

38 source closure (2026-10-07): **512 per-cell back/front poses** cover the actual fixed 4x4 layout, construction/idle/working/full states and four shared authored indicator phases. Twelve Rock-specific roles use selected idle/construction/stock originals; hut, paving, retaining wall and drain reuse exact Earth cells. The short stock sheet contributes only its real 20 panels; a separate original fills the missing four full-state roles. Explicit nonoverlapping crop rectangles resolve tight gutters. [Focused Rock source/placement/refusal/schema checks](../../../../logs/r2mlvs13-w-rock-source-checks.log) pass; the slow placement fixture was split into `multilayer_facility_art.py` / `multilayer_facility_art-1.py` and now caches fixed-cell palette reductions. Whole-site PNG/GIF review is `mlm/reviews/multilayer-art/authoring/import-20261009/images/rock-receiver-details-review.png`. User art review and native payload/state selection remain pending.


### Group 47 - Author tube guideways and junctions

Resumed2026-10-10 under master10.4. Execute47B-47G below in order, then close the original aggregate tasks only when all six native families satisfy them. These bounded groups preserve the entire original scope while separating geometry/source reviews; [selected ground art notes](r2mlvs14-a-selected-ground-art.md) hold the single current continuation.

{{HARD}}
- [X] 47.1 For tube guideways and junctions, derive a single-model brief from its approved manifest/template: sealed shells that leave moving capsules readable. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [X] 47.2 Produce the required directions/rotations for tube guideways and junctions from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [X] 47.3 Add straight/45-degree bend/junction/end and powered/unpowered pieces for tube guideways and junctions, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.


### Local Mandatory Reading - Native straight tube originals

_Applies to Group47B; supplements the Source generation and reproducible conversion Area._

Read Global/AD-1/map ownership/protected sprite block, `tube.straight` in the actual manifest, the current brief in [selected ground art notes](r2mlvs14-a-selected-ground-art.md), native `Track::X/Y` and `UTUB` registry geometry, and the shared quantizer's anchor/palette policy. The installed imagegen skill and persistent CODEX_HOME authorization are verified. Use actual engine edge contacts and a clear moving-capsule channel. Reuse existing source/provenance conventions; do not reread M1's completed authoring/native diary or invent another track/power owner.

Context has compacted 1 times while executing this group.


### Group 47B - Freeze native straight geometry and author its originals

{{HARD}}
- [X] 47B.1 Freeze the two undirected native axes,64x31 canvas/anchor and back/front channel clearance. Publish the geometry brief/template, select an immutable built-in-generated straight base and record exact prompt/reference/output provenance before variants.
- [X] 47B.2 Author the independent NE/SW and NW/SE views from that selected form with fixed contacts/light; reversed traversal reuses its original axis rather than a rotated substitute.
- [X] 47B.3 Complete construction/powered/unpowered coverage, native/enlarged contact review, aligned back/front masks and source/palette/anchor validation. Publish the retained original/selected frame mapping for later deterministic packing.

47B closure,2026-10-10: three immutable built-in originals supply six independently authored state/axis frames and twelve aligned back/front poses. Exact source/reference/prompt provenance and reviewed crops live in `media/multilayer/source/ground/tube-generation-record.json`; `convert_tube_art.py --straight` deterministically rebuilds the indexed atlas and native/enlarged/three-tile contact review. Native centreline checks corrected powered X/Y registration to(-1,1)/(-1,0), without changing retained originals. [Nine focused source/refusal cases](../../../../logs/graphics/tube-source/20261010-straight-01/source-checks.log) and [public manifest validation](../../../../logs/graphics/tube-source/20261010-straight-01/contract-2.log) pass. The public source contract is authored; packed offsets/payload and runtime activation remain unchanged. [Source atlas and review mapping](../../../../logs/proofs/mlm/native-art/ground/tube-atlas-manifest.json) identifies all twelve poses. Group47's whole six-family scope remains unchecked;47C is next.


### Local Mandatory Reading - Native curved tube originals

_Applies to Group47C._

Read Global/Source generation Area, completed47B style/clearance, only `tube.bend45`, native Upper/Lower/Left/Right endpoints and the matching curve guides. Preserve fixed light/anchors and actual traversed45-degree geometry; do not rotate flat straight art into a curve.


### Group 47C - Author the four native curved tracks

{{HARD}}
- [X] 47C.1 Freeze actual four-corner endpoints and select a provenance-retained curved-shell base using the approved straight style.
- [X] 47C.2 Author all four engine corner views with exact native contacts and consistent capsule clearance.
- [X] 47C.3 Complete three-state/back-front coverage and native contact/palette/anchor review before conversion handoff.

47C closure,2026-10-10: four selected immutable originals supply twelve actual state/corner frames and24 aligned back/front source poses. Native views are N=Upper, S=Lower, W=Left, E=Right; their actual vehicle chords/adjacent straight mouths are recorded in the public geometry notes. Exact prompts, reference/source hashes, rejected incomplete rims, excluded model-changing cell and explicit uneven-gutter selections live in `tube-generation-record.json`. `convert_tube_curves.py` reproducibly rebuilds the indexed atlas and all-state native/enlarged/two-neighbour contact review. Low end couplings draw beneath capsules; exact facing-rim source subsets leave every actual native chord clear in the foreground. [Nine corner-source checks](../../../../logs/graphics/tube-source/20261010-curves-01/source-checks.log), [nine shared straight checks](../../../../logs/graphics/tube-source/20261010-curves-01/straight-shared-checks.log), [1,051 native geometry assertions/two cases](../../../../logs/graphics/tube-source/20261010-curves-01/native-geometry.log) and [public manifest validation](../../../../logs/graphics/tube-source/20261010-curves-01/contract.log) pass. [Complete source mapping/contact identities](../../../../logs/proofs/mlm/native-art/ground/tube-curves-atlas-manifest.json) retain all24 poses. Source metadata is authored; existing packed identities/payload and runtime activation are untouched. Whole Group47 remains pending for47D-47G;47D is next.


### Local Mandatory Reading - Native tube junction originals

_Applies to Group47D._

Read Global/Source generation Area, completed47B-C, only `tube.junction` and actual native TrackBits combinations. Freeze composition so the shell cannot paint connections absent from the installed native tracks.


### Group 47D - Author native junction shells

{{HARD}}
- [X] 47D.1 Freeze actual junction composition/ports and select an immutable provenance-retained base with no invented arm.
- [X] 47D.2 Author required native views/composable parts from that base with fixed anchors and clear capsule paths.
- [X] 47D.3 Complete three-state/back-front coverage and actual combination contact/palette/anchor review before conversion handoff.

47D closure,2026-10-10: every54 shared-edge native TrackBits layout has three states and aligned back/front source parts:324 poses. The selected X/Upper base and all variants reuse the six previously authored native casing shapes; conversion trims only pixels crossed by another installed track and leaves all actual native channels clear in front. No source pixel is painted/rotated, and no absent contributor/mouth is added. Crossing3 and independent parallel curves12/48 retain their own render cases. Six all-state native/enlarged/neighbour contact pages were inspected. [Twelve focused source/refusal checks](../../../../logs/graphics/tube-source/20261010-junctions-01/source-checks.log) pass in37 seconds. Actual native quote/execute construction of all63 nonempty layouts and unchanged dedicated tube turn policy pass2,233 assertions/three cases in both [optimized](../../../../logs/graphics/tube-source/20261010-junctions-01/native-rel.log) and [Debug](../../../../logs/graphics/tube-source/20261010-junctions-01/native-debug.log). [Public contract validation](../../../../logs/graphics/tube-source/20261010-junctions-01/contract.log) passes145 families/55,295 required sprites. [Source/proof mapping](../../../../logs/proofs/mlm/native-art/ground/tube-junction-atlas-manifest.json) binds every original contributor and contact page; `convert_tube_junctions.py --check` regenerates them exactly. Source metadata is authored; packing/runtime remain pending and existing payload/IDs are unchanged. [Acceptance identities](../../../../logs/graphics/tube-source/20261010-junctions-01/verification.json) bind11 sources, seven unchanged originals, eight source artifacts and both test binaries. Whole Group47 stays pending for47E-47G;47E is next.


### Local Mandatory Reading - Native tube end originals

_Applies to Group47E._

Read Global/Source generation Area, completed guideway style, only `tube.end`, native `GetTubeGuidewayEnds` in `transport_tube.h` / `transport_tube_topology.cpp` and the `[transport-tube-end-geometry]` fixture in `transport_tube_art_geometry.cpp`. A painted cap must not replace a physical end or create a passable guideway.

Context has compacted 1 times while executing this group.


### Group 47E - Author native tube end pieces

{{HARD}}
- [X] 47E.1 Freeze actual one-ended ports and select an immutable provenance-retained end-shell base.
- [X] 47E.2 Author the four actual engine-facing end views with common contact/anchor/light.
- [X] 47E.3 Complete three-state/back-front coverage and native contact/palette/anchor review before conversion handoff.

47E closure,2026-10-10: four independently authored physical NE/SE/SW/NW plates and two original state sheets supply all24 source poses. The known repeated-facing sheet remains rejected. Explicit source selections preserve actual native mouth positions, correct face planes and fixed light; all-state native/enlarged/approach contacts were reviewed. [Twelve source/refusal cases](../../../../logs/graphics/tube-source/20261010-ends-01/source-checks.log) pass in7 seconds, including wrong actual face plane at a correctly positioned port, oversized stub, changed original, substituted lifecycle, painted pixels and false provenance. The bounded native `GetTubeGuidewayEnds` presentation query and both game/test targets build in both configurations; each focused endpoint burst passes408 assertions/two cases. [Acceptance identities](../../../../logs/graphics/tube-source/20261010-ends-01/verification.json) bind eight sources, six selected originals, three source artifacts and four binaries. Public validation passes145 families/55,307 required sprites. Exact regeneration uses `convert_tube_ends.py --check`; source is authored, packing/runtime remain pending. Whole47 remains pending for47F-G.


### Local Mandatory Reading - Native tube crossing originals

_Applies to Group47F._

Read Global/Source generation Area, completed guideway style, only `tube.crossing` and native X/Y crossing geometry. Preserve native reservation/access ownership and show only actual connections.


### Group 47F - Author native X/Y crossings

{{HARD}}
- [X] 47F.1 Freeze actual crossing ports/clearance and select an immutable provenance-retained base.
- [X] 47F.2 Author required crossing views/parts with common contacts, fixed light and readable capsule occupancy.
- [X] 47F.3 Complete three-state/back-front coverage and native contact/palette/anchor review before conversion handoff.

47F closure,2026-10-10: actual native `TRACK_BIT_CROSS` has one fixed X/Y camera layout and two aligned parts, not four door rotations. Three original authored straight sources supply all six crossing state/part poses through exact original pixel selection/trim; both native channels remain clear and no corner/painted arm is added. All-state native/enlarged/four-neighbour contacts were inspected. [Eleven source/refusal cases](../../../../logs/graphics/tube-source/20261010-crossings-01/source-checks.log) pass in10 seconds; the shared selector's [twelve junction checks](../../../../logs/graphics/tube-source/20261010-crossings-01/shared-junction-checks.log) pass in35 seconds. Actual native all63 layout admission/classification remains the already delivered47D evidence; no C++ change/build was needed. Public validation passes145 families/55,301 required sprites after correcting the draft crossing matrix. `convert_tube_crossings.py --check` regenerates exact source/provenance/contact proof; [acceptance identities](../../../../logs/graphics/tube-source/20261010-crossings-01/verification.json) retain six source files, three immutable originals and three artifacts. Source is authored; packed/runtime identities remain unchanged/pending.47G is next; original whole47 remains unchecked.


### Local Mandatory Reading - Native tube service conduit originals

_Applies to Group47G._

Read Global/Source generation Area, completed guideway style, only `tube.conduit` and actual `TubePowerConnection::utility_path`, `CaptureTubePowerConnections`, `CheckTubePowerTopology` in `transport_tube_power.cpp`, and `[transport-tube-power]` fixtures. Placement follows existing active same-layer physical corridors, not guideway TrackBits or catchment. Shared `GetUtilityPowerProducer` owns electrical status; source pixels never issue service capacity, generation, grant or a second utility installation. This is a floor-level cable overlay drawn beneath vehicles/buildings, so it has one ground body rather than an invented foreground wall.


### Group 47G - Author tube service conduit presentation

{{HARD}}
- [X] 47G.1 Freeze actual service-path geometry/status ownership and select an immutable provenance-retained conduit base.
- [X] 47G.2 Author required native conduit views/parts with common contacts/light and readable guideway occupancy.
- [X] 47G.3 Complete three-state ground-overlay coverage and native contact/palette/anchor review; verify the complete47 family source handoff before closing original47.1-47.3. Native floor-level conduit uses one body below vehicles/buildings; all casing families retain their aligned back/front parts.

47G/original47 source closure,2026-10-10: actual `TubePowerConnection::utility_path` supplies the corridor geometry; two original low cable axes and their independent construction/unpowered frames supply12 physical center-to-edge ground branches. Floor bodies draw beneath vehicles/buildings rather than inventing a foreground wall. All-state native/enlarged/straight/turn/neighbour contacts were reviewed. [Thirteen source/refusal cases](../../../../logs/graphics/tube-source/20261010-conduits-01/source-checks.log) pass in5 seconds. Existing actual paid-service topology/invoice/save boundaries pass1,066 assertions/seven cases in each configuration without a C++ rebuild. [Complete source handoff](../../../../logs/graphics/tube-source/20261010-conduits-01/verification.json) verifies all six families: straight12, bends24, junction324, ends24, crossing6 and conduit12, totaling402 source poses. All six conversion commands regenerate exact source/provenance/contact proof read-only; public validation passes145 families/55,301 required sprites. Every packed identity matches the unchanged committed contract.47B-G and original47.1-47.3 areX for source delivery; actual packing/runtime remain the later master10.4/10.7 checkpoints.48.1 is next.


### Local Mandatory Reading - Actual tube facility source ownership

_Applies to Group48 and48B-48D; supplements Source generation Area._

Read `GetTubeTerminal` / `GetTubeNetworkReadiness` in `transport_tube_readiness.cpp`, tube branches in `CmdBuildTrainDepot` / `CmdBuildRailStation`, and only the executing facility's manifest/source. Actual depots occupy one tile/four directed entrances; terminal platforms follow native axis/length cells, and interchanges retain canonical same-layer station/packet owners. Draft2x2/3x2 footprints are not delivered producers. Geometry/source work cannot complete pending Plan17 Group9 operating interchange/cargo acceptance. Preserve existing selected M1 source pixels and packed identities; reuse a retained compatible model before generating duplicate imagery.


### Group 48 - Author tube terminals and service/interchange facilities

Resumed2026-10-10 under master10.4. Execute48B service,48C loading platform and48D interchange source in order, then close the original aggregate tasks only after all three actual families pass. The earlier finalization skip is historical. Actual operating facilities/cargo acceptance remain master10.5/Plan17 Group9.

{{HARD}}
- [X] 48.1 For tube terminals and service/interchange facilities, derive a single-model brief from its approved manifest/template: distinct powered terminal/depot/rail-road transfer templates. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [X] 48.2 Produce the required directions/rotations for tube terminals and service/interchange facilities from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [X] 48.3 Add approach/occupied/unpowered fronts and backs for tube terminals and service/interchange facilities, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.


### Local Mandatory Reading - One-tile tube service enclosure

_Applies to Group48B._

Read this plan's Global/Source generation Area and Actual tube facility source ownership Local. Inspect only `facility.tube-service`, actual `CmdBuildTrainDepot` / `GetRailDepotDirection`, `GetTubeTerminal`, and the retained `facility.rail-service` source plus its `facility-status` original component in `source/m1/facility-generation-record.json` / `convert_facility_art.py`. One compatible native service enclosure may be reused without recolouring or replacing original pixels. Four true entrances, four genuine lifecycle source rows and four actual indicator phases retain native64x64/anchor(-31,-33), with one physical tile and aligned back/front. Blocked includes actual electrical service refusal/unpowered; source art cannot close a physical track or grant service.


### Group 48B - Supply one-tile tube service source

{{HARD}}
- [X] 48B.1 Freeze actual one-tile depot/approach geometry and select a compatible immutable source enclosure with explicit reuse provenance.
- [X] 48B.2 Deliver four actual entrance views and all construction/ready/occupied/blocked-power states through retained original model/status components.
- [X] 48B.3 Verify128 aligned state/view/phase/back-front source poses, native contacts/foreground/palette and exact regeneration; preserve M1 payload/identities and leave operating/runtime acceptance pending.

48B closure,2026-10-10: the actual native tube service depot occupies one cell with four entrances; its incompatible historical3x2 draft is corrected. The complete retained native rail-service enclosure and status component supply128 source poses without new raster generation/recolouring or changed M1 pixels. All actual view/state/back-front/phase contacts were reviewed. [Ten focused source/refusal checks](../../../../logs/graphics/tube-source/20261010-service-01/source-checks.log) pass in30 seconds, including exact unchanged M1 regeneration, missing view/phase/front, false model ancestry, six-cell geometry and painted source pixels. Actual native depot/finance/topology readiness passes245 assertions/two cases per configuration. The initial separate loading-platform fixture crash is retained;48C has since supplied explicit font setup and943 assertions/four native cases pass per configuration. Public validation passes145 families/54,661 required sprites. [Acceptance identities](../../../../logs/graphics/tube-source/20261010-service-01/verification.json) bind nine sources, two unchanged originals, three artifacts and four binaries. `convert_tube_facilities.py --check` regenerates exact proof; only source is authored, packing/operating/runtime remain pending. Whole48 stays unchecked for48C-D.


### Local Mandatory Reading - Native tube loading platform source

_Applies to Group48C._

Read Global/Source generation Area and Actual tube facility source ownership Local. Inspect only `facility.tube-terminal`, actual `CmdBuildRailStation`, native station tile graphics/axes/platform length, `GetTubeTerminal` and corresponding construction/readiness fixtures. Freeze per-cell through approaches and real layouts before generation; generic four-door/multi-tile drafts cannot replace native platform geometry. Canonical station packets/capacity remain their existing owner.


### Group 48C - Supply native tube loading platform source

{{HARD}}
- [X] 48C.1 Freeze actual native platform axes/cell roles/approaches and select an immutable compact loading model.
  Geometry portion verified2026-10-10: twelve real quote/execute layouts retain native station/cargo identities and all eight default graphics roles across two axes;943 assertions/four focused cases pass in both configurations. Explicit mock font setup removes the isolated platform sign-layout crash; the first matrix attempt outside the64x64 fixture boundary is retained and corrected. [Geometry identities](../../../../logs/graphics/tube-source/20261010-platforms-01/geometry-verification.json) bind the producer/test sources and both test binaries. Public geometry is one per-cell canvas/two axes/four role variants. Immutable `tube-terminal-platform-x-base-v1.png` now supplies the selected ready X/bare-role base: all four side contacts, exact low foreground, native/enlarged and three-cell joins were reviewed. Remaining actual axis/roles/states/phases belong to48C.2; complete source/packing/runtime are still pending.
- [X] 48C.2 Deliver required actual platform views/variants and construction/ready/occupied/power-wait states from that model.
- [X] 48C.3 Verify native cell contacts/foreground/palette/phase coverage and exact source handoff without claiming operating cargo acceptance.

48C closure,2026-10-10: twelve real native station layouts expose both axes/four default graphics roles per cell, with original station/cargo identity and berth length;943 assertions/four cases pass in each configuration after explicit font-fixture setup. Six immutable model originals supply16 actual construction/installed cells; the misplaced X construction booth remains excluded and a fixed-position original supplies that cell. Original M1 lamp artwork supplies four genuine state/phase components inside existing opaque rear-ledge fittings. Installed ready/working/blocked models remain exact outside those nine source pixels; no painted cargo/vehicle or private power owner appears. All256 state/axis/role/phase/back-front poses and both full contact pages were reviewed. [Thirteen source/refusal checks](../../../../logs/graphics/tube-source/20261010-platforms-01/source-checks.log) pass in18 seconds, including actual native-route obstruction, model/component ancestry, excluded booth, missing role/phase/part, false state reuse, changed original and transparent cue refusal. Public validation passes145 families/54,405 required sprites. [Acceptance identities](../../../../logs/graphics/tube-source/20261010-platforms-01/verification.json) bind11 sources, seven unchanged model/component originals, four artifacts and both test binaries. `convert_tube_terminals.py --check` regenerates exact source/provenance/contact proof. Every packed identity is preserved; operating/packing/runtime acceptance remains pending.48C isX and48D is next; whole48 stays unchecked.


### Local Mandatory Reading - Native tube interchange source

_Applies to Group48D._

Read Global/Source generation Area and Actual tube facility source ownership Local. Inspect only `facility.tube-interchange`, actual same-layer station joining/loading/road-stop/cargo packet owners and delivered Plan17 Group9 producer scope. A connected visual campus cannot create a second inventory, permit mixed tube/conventional tracks or cross-layer catchment. Missing operating transfer does not authorize fabricated geometry or runtime acceptance; stage its exact producer before dependent authoring if necessary.


### Group 48D - Supply native tube interchange source

{{HARD}}
- [X] 48D.1 Freeze actual existing interchange facility cells and shared station/packet identity; complete a bounded geometry producer first if absent.
- [X] 48D.2 Deliver native tube contact views/parts and lifecycle/status phases using the compatible original platform cells. Conventional rail/road contacts remain with their selected source families in Groups24/25; the native joined station has no additional fixed campus building.
- [X] 48D.3 Verify complete native interchange source/contact/foreground/palette handoff and close original48.1-48.3 only when48B-48D all pass.

48D closure,2026-10-10: eight real combinations of two native platform axes/four freight road-stop entrances share one canonical same-layer StationID/goods owner. Tube/conventional berths retain separate rail types; actual capsules refuse conventional rail/road. Quote/execute cross-layer joins refuse without tile/station/packet/funds mutation, and independent nearby stations retain distinct goods owners. The combined interchange/platform/readiness burst passes1,803 assertions/six cases in each configuration. No new production registry, inventory or station command was needed. Interchange tube cells reuse the exact256-pose original platform atlas and both native contact pages; rail/road partners keep their selected Groups24/25 source owners. Seven focused source/refusal checks pass in14 seconds, including fictitious campus, mixed tracks, cross-layer/private-stock identity, incompatible model, changed original, missing front and false runtime claims. [Acceptance identities](../../../../logs/graphics/tube-source/20261010-interchange-01/verification.json) bind16 sources, seven unchanged originals, four artifacts and both test binaries. `convert_tube_interchanges.py --check` regenerates exact proof. Public validation passes145 families/53,893 required sprites after actual geometry reconciliation; all packed metadata matches HEAD.48B-D and original48 areX for source. Operating cargo transfer, partner-family source, packing and runtime retain their dispatched tasks.49.1 is next.


### Local Mandatory Reading - Native capsule model source

_Applies to Groups49-53; supplements Source generation Area._

Read Global/Source generation Area, only the executing model's public body/purchase manifest and retained content model/cargo/capability row, then its actual native engine/direction/length/sprite consumers. Preserve AD-15's five distinct physical models and fixed camera/light; a service/load state changes only actual authored fittings/load portions. Verify the already delivered model9 prototype for Mole-70. Later models require a bounded geometry contract before dependent authoring if their native definitions are absent; source acceptance never activates their dates, refits, purchase or operating transfer. Those runtime owners remain master10.5/Plan17 Groups9-12.


### Group 49 - Author Mole-70

Resumed2026-10-10 under master10.4 after complete47/48 source. The earlier finalization skip is historical. Actual model9 prototype/cargo/direction/full-length geometry is delivered; no native engine change is needed for source authoring.

{{HARD}}
- [X] 49.1 For Mole-70, derive a single-model brief from its approved manifest/template: low segmented amber hopper ribs. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [X] 49.2 Produce the required directions/rotations for Mole-70 from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [X] 49.3 Add eight-direction empty/loaded/service and purchase frames for Mole-70, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.

49.1/49.2 source geometry,2026-10-10: actual model9 native EIDS/cargo/route producer checks pass331 assertions/seven cases without a new build. Three immutable originals supply eight genuine empty physical views, preserving the selected E base. The initial sheet's repeated south face and steep diagonal cells remain excluded; a corrected four-view original supplies true S/SE/SW/NW faces. Explicit source crops, native sampling/registration and fixed64x48/anchor(-32,-32) preserve the real full-length chassis and stable upper-left materials. All eight native views/adjacent contacts were reviewed through `convert_tube_capsules.py --model-views`; the low capsule uses smaller end/diagonal envelopes within the existing native bounds. [Base geometry identities](../../../../logs/graphics/tube-source/20261010-mole-01/base-view-verification.json) retain originals, sources, eight-view proof and the tested producer binary.49.3 closure below supplies the complete load/status/phase/purchase handoff; this earlier eight-view proof alone never established that coverage.

49.3 closure,2026-10-10: native cargo and service/power conditions are independent, so the corrected source contract has nine load-preserving states rather than five ambiguous rows. Three actual loads remain visible through operating/service/blocked modes. Sixteen existing same-view Rock donor cells regenerate from the immutable M1 original; their cargo material fills only approved recessed compartments, preserving all ribs/chassis alpha. Genuine M1 glass phases0/2 change only the existing nose fitting, and the original blue side plaque alone receives canonical company remap. The own-model empty purchase silhouette retains exact pixels when the original calendar-lock badge is added below it. All144 body/two purchase poses and four contact pages were reviewed. Fifteen focused source/refusal cases pass in25 seconds; the added unchanged-original/shifted-registration refusal passes in6 seconds. Final publication revalidates and exactly regenerates every artifact through `convert_tube_capsule_states.py --check`. [Acceptance identities](../../../../logs/graphics/tube-source/20261010-mole-01/verification.json) bind25 sources, six unchanged originals, seven artifacts and the unchanged tested native producer binary. Public validation passes145 families/53,957 required sprites; all packed metadata matches HEAD.49 isX for source. Operating fleet, packing and runtime remain pending;50.1 must establish actual model10 geometry before Crate-75 authoring. All handles are terminal.

Context has compacted 1 times while executing this group.


### Group 50 - Author Crate-75

Resumed2026-10-10 under master10.4 after complete49 source. Plan17 staged10B now publishes/tests canonical full native8-unit/one-part physical geometry for models9-13 through `GetTubeVehicleGeometry`; only model9 is installed. This resolves source geometry without activating future engines or closing the original operating fleet tasks. Preserve distinct square green clamped-package geometry and actual cargo forms.

{{HARD}}
- [X] 50.1 For Crate-75, derive a single-model brief from its approved manifest/template: square green clamped containers. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [X] 50.2 Produce the required directions/rotations for Crate-75 from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [X] 50.3 Add eight-direction empty/loaded/service and purchase frames for Crate-75, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.

50.1 source geometry,2026-10-10: the verified staged10B producer supplies full native8-unit/one-part spacing before model10 activation. The selected immutable `crate-base-e-v1.png` supplies a flat rectangular green caddy, six actual silver corner/divider clamps and two empty package bays, unlike the rounded amber bulk hull. Explicit32x12 native source sampling/translation(0,-4) retains64x48/anchor(-32,-32) and reviewed E contacts. The monochrome source masks differ from Mole-70's base at125 of341 opaque union pixels, before colour. [Base identities](../../../../logs/graphics/tube-source/20261010-crate-01/base-view-verification.json) retain the original, native proof and current source inputs. The general native-view helper accepts only the public tube vehicle families; complete Mole source still regenerates exactly. All five capsule source rows now preserve three loads through service/blocking,145 families/54,213 required sprites overall.50.2 must supply genuine missing faces; public Crate source remains planned, and no operating engine/purchase/runtime claim is made.

50.2/50.3 closure,2026-10-10: two native-proportion originals supply seven genuine missing headings while the selected E base stays exact. The first narrower four-view draft and the duplicate E remain excluded. All eight native contacts, fixed upper-left materials and true front/rear faces were reviewed. An independent original supplies eight same-view sealed package lids/straps, clipped only to the actual recessed bays: partial fills the rear bay, full both, and all loads survive service/power waits. Genuine M1 lamp phases and the calendar-lock component are reused; the Crate purchase badge sits one pixel lower to preserve every opaque preview pixel. All144 body/two purchase poses and four contact pages pass review. Twenty focused Crate cases pass in23 seconds; sixteen shared-converter Mole compatibility cases, including exact existing proof regeneration, pass in28 seconds. Public validation passes145 families/54,213 sprites; every packed field matches HEAD. [Source acceptance identities](../../../../logs/graphics/tube-source/20261010-crate-01/verification.json) retain23 sources, six immutable originals and seven artifacts.50 isX for source; packing/runtime remain pending. No C++ build was needed.51.1 is next; all handles are terminal.


### Group 51 - Author Cistern-80

Resumed2026-10-10 under master10.4 after complete50 source. The earlier finalization skip is historical. Plan17 staged10B supplies canonical native8-unit/one-part geometry before model11 operating activation. Preserve distinct cyan twin tanks/pressure bands and actual liquid CHEM/PROP cargo meaning; visible level/service fittings never create a fluid stock or power owner.

{{HARD}}
- [X] 51.1 For Cistern-80, derive a single-model brief from its approved manifest/template: cyan twin tanks and pressure bands. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [X] 51.2 Produce the required directions/rotations for Cistern-80 from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [X] 51.3 Add eight-direction fluid-level/service and purchase frames for Cistern-80, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.

51.1 source geometry,2026-10-10: the selected immutable E original has two parallel pale cyan pressure tanks on one compact skid, three dark pressure bands, genuine vertical sight glass, separate blue plaque and front-lamp/rear-coupling fittings. Reviewed32x14 native sampling/translation(0,-4) preserves full8-unit/one-part geometry,64x48/anchor(-32,-32). Before colour, its source mask differs from Mole at196/397 union pixels and Crate at119/421. [Base identities](../../../../logs/graphics/tube-source/20261010-cistern-01/base-view-verification.json) retain the original, native review and actual physical producer inputs.51.2 must retain E unchanged and supply genuine other faces; whole source, liquid-level variants, packing and runtime remain pending.

51.2 source geometry,2026-10-10: two additional immutable originals supply genuine N/NE/S/SW and SE/NW/W; the duplicate E stays excluded and the selected base stays exact. Native end14x24, diagonal24x20 and side32x14 projections/contact registration preserve one full part, both tanks, all three bands and fixed light. The eight-view contact page was reviewed. [Direction identities](../../../../logs/graphics/tube-source/20261010-cistern-01/direction-view-verification.json) retain three originals and exact crop/registration/proof inputs.51.3 shows level changes only through actual visible original sight glass in NE/E/SE/SW/NW; opaque N/S/W views keep their sealed hull. An invisible internal liquid level cannot justify painting a fictional window. Complete state/purchase source remains pending.

51.3 closure,2026-10-10: original half/full cyan liquid material fills only the actual lower/upper sight-glass pixels in the five visible headings. Opaque N/S/W views retain their sealed tank through all three native loads; their cargo metadata and service/power state remain independent. Genuine M1 status phases and calendar lock are reused. Explicit original plaque crops keep cyan tank shadows out of company remap, and a uniform smaller own-model purchase fit preserves its body above the badge. All144 body/two purchase poses and four contact pages were reviewed. Twenty-one focused source/refusal cases pass22.828 seconds; exact complete Mole/Crate regeneration passes in15.156/14.080 seconds. Public145-family/54,213-sprite validation passes; all packed fields match HEAD. [Source acceptance identities](../../../../logs/graphics/tube-source/20261010-cistern-01/verification.json) retain25 sources, six unchanged originals and seven artifacts.51 isX for source; packing/runtime remain pending. No C++ build.52.1 is next; all handles are terminal.


### Group 52 - Author Shield-85

Resumed2026-10-10 under master10.4 after complete51 source. The earlier finalization skip is historical. Staged Plan17 10B supplies canonical native8-unit/one-part geometry before model12 activation. Preserve the distinct short pale shielded octagonal body and sealed URAN/RARE cargo; source fittings never create a cargo inventory or unlock an operating engine.

{{HARD}}
- [X] 52.1 For Shield-85, derive a single-model brief from its approved manifest/template: short pale shielded octagonal body. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [X] 52.2 Produce the required directions/rotations for Shield-85 from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [X] 52.3 Add eight-direction sealed-load/service and purchase frames for Shield-85, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.

52.1 source geometry,2026-10-10: the first oblique draft is retained/excluded; a genuinely rerendered horizontal E original supplies the pale sealed octagonal armour, dark end braces, two distinct original framed roof-meter tiles, front lamps/rear coupling and separate blue plaque. Reviewed32x14 native sampling/translation(0,-4),64x48/anchor(-32,-32), preserves actual full8-unit/one-part contacts. Monochrome masks differ from Mole at188/401, Crate at87/413 and Cistern at42/411 union pixels. [Base identities](../../../../logs/graphics/tube-source/20261010-shield-01/base-view-verification.json) retain the selected original and native contact/source inputs.52.2 must preserve E and supply genuine other directions.52.3 may reuse genuine M1 amber glass in the two original roof-load fittings, one/both lit for partial/full sealed URAN/RARE cargo, with separate actual service/power cues. Source creates no mineral inventory or operating model12. Whole source/packing/runtime remain pending.

52.2/52.3 closure,2026-10-10: two immutable originals supply seven genuine other headings, preserving the selected E and excluding duplicate E. Eight native contacts, original octagonal armour, two separate roof-glass fittings and fixed light pass review. Actual sealed cargo illuminates rear/both roof fittings for partial/full using genuine M1 amber glass, keeping opaque armour, source alpha and every other pixel exact. Independent M1 mode phases and original calendar lock preserve loads through service/power waits. All144 body/two purchase poses and four contacts were reviewed. Nineteen focused source/refusal cases pass17.617 seconds; exact complete Cistern regeneration passes10.636 seconds. Public145/54,213 validation passes and every packed field matches HEAD. [Acceptance identities](../../../../logs/graphics/tube-source/20261010-shield-01/verification.json) retain26 sources, five unchanged originals and seven artifacts.52 isX for source; operating/packing/runtime remain pending. No C++ build.53.1 is next; all handles are terminal.

Context has compacted 1 times while executing this group.


### Group 53 - Author Forge-05

Resumed2026-10-10 under master10.4 after complete52 source. The earlier finalization skip is historical. Plan17 staged10B supplies canonical native8-unit/one-part geometry before model13 activation. Preserve a distinct broad dark braced machinery frame and actual STEL/RFMT/SCMP/RBPT heavy cargo meaning. Source authoring does not unlock the2005 engine, allocate heavy cargo or claim runtime clearance.

{{HARD}}
- [X] 53.1 For Forge-05, derive a single-model brief from its approved manifest/template: broad dark braced machinery frame. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [X] 53.2 Produce the required directions/rotations for Forge-05 from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [X] 53.3 Add eight-direction heavy-load/service and purchase frames for Forge-05, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.

53.1 source geometry,2026-10-10: the selected genuine horizontal E original has a broad dark open frame, reinforced end portals/diagonal struts, thin top brace, two empty grey saddles and restrained orange clamps. Native32x18 sampling/translation(0,-4),64x48/anchor(-32,-32), and full8-unit/one-part contacts pass review. Original and native load apertures are truly transparent. Monochrome mask differences are Mole138/341, Crate173/421, Cistern188/449 and Shield198/462 union pixels. [Base identities](../../../../logs/graphics/tube-source/20261010-forge-01/base-view-verification.json) retain the original, native proof and physical source inputs.53.2 preserves E and supplies genuine other views.53.3 must author actual heavy modules inside the existing rear/both open cargo volumes; only those approved apertures may gain load alpha, with every brace, clamp, beam and foreground fitting preserved. The existing contained-load alpha rule needs a bounded model13 extension rather than flattening heavy modules to floor paint. Whole source/packing/runtime remain pending.

53.2/53.3 closure,2026-10-10: three immutable hull originals supply all eight genuine native views with the selected E preserved and duplicate E excluded. A separate original heavy-module sheet supplies eight actual same-view solid loads; its bottom diagonal labels are registered to their observed axes, with no raster rotation. Approved rear/both volume masks allow original module alpha only inside actual open cargo apertures, skip transparent donor pixels and draw farther modules first. Frame, orange clamps and foreground remain exact. Actual front equipment lamps and own-model purchase/calendar lock retain all quantities independently. All144 body/two purchase poses and four contact pages pass review. Twenty-two focused cases pass46.778 seconds, and exact Shield compatibility passes9.354 seconds. Original-space clamp projection covers downsampled brown end-view hardware; the initial brightness-only test diagnostic remains retained. Public145/54,213 validation and unchanged packed metadata pass. [Acceptance identities](../../../../logs/graphics/tube-source/20261010-forge-01/verification.json) bind24 sources, six unchanged originals and seven artifacts.53 isX for source; engine activation, packing and runtime remain pending. No C++ build.54.1 is next; all handles are terminal.


### Local Mandatory Reading - Native vent and collector source

_Applies to Group54 and54B-54C; supplements Source generation Area._

Read only the public `facility.vent-collector` and `geothermal.vent` manifest rows, AD-16, `GeothermalVentField`/`FindGeothermalVentField`, and actual `GetMultiLayerGeothermalConstruction(Collector)`, `GetGeothermalCollectorIndustryType`, `IsGeothermalCollectorDefinition`, native TIDS layout and `GeothermalCollectorState`. Collector source uses the delivered3x3/nine-cell industry and retained claim/access/endpoint; it cannot become one oversized single-tile building. Natural field geometry belongs to geology and may cover multiple tiles; its per-tile art does not allocate another source. Inspect `GetMultiLayerTerrainSprite`, `GetMultiLayerSurveySprite` and actual viewport anchors before proposing vent visibility. Current survey sprites cover mineral seams, so vent source acceptance cannot claim a delivered vent overlay. Native collector readiness, shared Heat offers and natural capacity keep their existing owners. Reuse genuine M1 component originals where suitable; final packing/runtime remain their dispatched tasks.


### Group 54 - Author vent field and collector

Goal-authorized skip (2026-10-08, master finalization): dispatched expansion work in this group is skipped under [finalization exception GP-TG](portal-tbm/r2mlvs3mp-z-a-finalization-ledger.md#gp-tg-tube-and-geothermal-expansion-requires-major-gap-planning). Existing delivered prefixes stay complete; unimplemented profile work remains pending. No skipped task is markedX.

{{HARD}}
- [X] 54.1 For vent field and collector, derive a single-model brief from its approved manifest/template: natural source versus installed bounded heat capture. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [X] 54.2 Produce the required directions/rotations for vent field and collector from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [X] 54.3 Add survey/excavated/connected/idle states for vent field and collector, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.

Resumed2026-10-10 after complete47-53 source. The earlier finalization skip is historical.54B supplies the actual collector;54C supplies the distinct natural source. Original54.1-54.3 are nowX after both named source scopes pass below. Actual native construction confirms3x3/nine cells and1980 capability; no geometry mutation is needed. The natural field is a saved bounded rectangle, default4x4, with immutable rate and an independent identity. Source artwork creates neither a new vent nor an operating generator.


### Local Mandatory Reading - Collector construction and equipment state

_Applies to Group54B; supplements Source generation Area and the Native vent/collector Local._

Read only the collector public manifest and approved whole-view source records, `GeothermalCollectorState`, `AdvanceGeothermalCollector`, `IsGeothermalCollectorOperational` and `PublishGeothermalCollectorHeat`, actual native `GetIndustryConstructionStage`/`IsIndustryCompleted`, and the shared `UtilityPowerProducer`/account publication fields. The nine native TIDS cells share one IndustryID; one whole original view supplies lossless per-cell sprite sections, not independent machinery or inventories. Construction uses real stages0..3 at74-tick quarters and completes separately at296 paid work ticks; native stage3 can still be unfinished. Construction frames follow these four native stages, while completed modes use authored equipment-glass phases. The collector is a Heat producer; reconcile the generic copied heat-starved art state before dependent authoring. Its native availability/publication owns work/source status, while the common ledger alone owns energy. Compose genuine original glass/state parts on the whole native scene before partitioning; preserve the approved ready view elsewhere. Source coverage creates no materials, work, heat or operating generator. Actual loaded rendering and plant runtime remain the later dispatched checkpoints.


### Group 54B - Author the native nine-cell collector

{{HARD}}
- [X] 54B.1 Freeze actual3x3/TIDS/per-cell geometry and select a distinct low bounded-heat collector base at native scale, retaining original prompt/reference/provenance.
- [X] 54B.2 Supply genuine four access-side views and all nine physical cells, preserving projected joins, native anchors and actual endpoint meaning.
- [X] 54B.3 Complete54BB construction and54BC equipment-state/source groups below. Supply actual native construction stages0..3, ready/working/blocked phases and front/back parts from original machinery/components. The collector is a Heat producer; the copied heat-starved consumer state is removed. Keep this aggregate unchecked until both bounded groups pass.

54B.1 closure,2026-10-10: actual collector IIDS/TIDS construction and natural-field checks pass6,516 assertions/eight cases in the existing optimized binary. The selected original has three capped boreheads, copper/insulated manifold, central pressure drum and instrument cabinet on the complete3x3 floor. Measured native192x98 sampling on256x192 campus reference preserves nine64x64 cells/anchor(-31,-33); it does not supply a single-tile substitute. Actual `RemapCoords` confirms world X runs SW/down-left and Y SE/down-right; referencev2 corrects the initial diagram labels before cell authoring. Selected view metadata is SE despite the retained original filename's initial NE label. Native decorative valve artwork never overrides the claim's actual endpoint. [Base identities](../../../../logs/graphics/geothermal-source/20261010-collector-01/base-view-verification.json) bind18 sources, two original images and three reviewed artifacts. The draft and exact builtin-imagegen prompts/reference/cache hashes remain retained.54B.2 is completed below through lossless native sections of four whole original models, retaining all native endpoint choices. Complete source, states, packing and runtime remain pending. No C++ build; all handles are terminal.

54B.2 closure,2026-10-10: four genuinely rerendered whole original models supply NE/SE/SW/NW, with coherent three-borehead/manifold/drum/cabinet forms and fixed camera/light. Independent-tile drafts exposed joint/role errors; all originals and diagnostics are retained. Selected whole models are losslessly sectioned across the actual nine64x64/anchor(-31,-33) TIDS canvases. All cells share one native IndustryID; sprite sections add no machinery, inventory or Heat owner. Every visible original native pixel occurs exactly once, and decoded native PNG pieces reconstruct the original bitmap exactly. Correct palette-zero transparency preserves assembly. Final ready registrations are SE192x97 at(33,61), and NE/SW/NW192x96 at(33,63); full measured crops and approval hashes are public/retained. Twelve focused source/refusal cases pass6.254 seconds; public145/54,213 and every packed identity match HEAD. [Ready acceptance](../../../../logs/graphics/geothermal-source/20261010-collector-01/ready-view-verification.json) binds27 sources, four selected originals and18 artifacts, with nine unselected originals retained. All four native contacts and the comparison page were reviewed.54B.2 isX for ready views only; construction, equipment states/phases/front parts remain54B.3. No C++ build or live image/test handle.


### Local Mandatory Reading - Four native collector construction frames

_Applies to Group54BB; supplements the Collector construction/state Local._

Read only `AdvanceGeothermalCollector`, `GetIndustryConstructionStage`/`IsIndustryCompleted`, frozen296-tick construction terms, the actual collector-stage boundary fixture, and the public collector frame-owner metadata. Native stages0..3 cover0..73,74..147,148..221,222..295 paid work ticks; completion is separate at296. Frame3 may reuse the final hardware because the native getter also returns3 after completion, but unfinished machinery still cannot publish Heat. Preserve selected whole-view geometry, lossless nine-cell sections and fixed camera/light. Author true incomplete whole models for stages0..2; source frames do not create materials, work or energy.

Context has compacted 1 times while executing this group.


### Group 54BB - Author the four real collector construction frames

{{HARD}}
- [X] 54BB.1 Reconcile the public four-state/four-frame contract with actual native stages/completion and Heat-producer ownership; verify the focused real native boundary and preserve all existing packed identities.
- [X] 54BB.2 Author/select genuine four-view construction-stage0 originals on the actual3x3 footprint, retaining prompts/references/cache/hash and native registration.
- [X] 54BB.3 Author/select genuine four-view stages1/2, then bind stage3 to the approved final hardware under construction status. Review all native stage/view contacts and freeze the actual construction source registrations.

54BB.1 closure,2026-10-10: actual native stages0..3 cover four74-tick quarters, while IsIndustryCompleted becomes true only at296. The earlier three-stage continuation note is corrected. Both incremental builds and the focused collector/definition/geology burst pass6,782 assertions/nine cases per configuration, including stage3 refusal of Heat publication before completion. The public collector now has four real states; construction frames select native stage, completed frames select genuine glass phase. Removing the copied Heat-consumer shortage state leaves145 families/53,925 required sprites; packed identities remain unchanged. [Binding identities](../../../../logs/graphics/geothermal-source/20261010-collector-01/state-binding-verification.json) retain native sources and both game/test binaries. Production C++ is unchanged.No build/test handle is live; the next original-source scope is recorded below.

54BB.2 closure,2026-10-10: four immutable original stage0 views show the marked native3x3 site, three bore pads, central drum outline and genuine framed amber marker at the corresponding cabinet side. All four native contacts and36 lossless cell sections were reviewed. Complete prompts, original/reference/cache identities and measured crops/positions are retained; [stage0 identities](../../../../logs/graphics/geothermal-source/20261010-collector-01/construction-0-verification.json) bind31 source/artifact files. Ready approval remains unchanged. No C++ build/test ran. Intermediate stages are completed below; complete composed source/packing/runtime remain pending.

54BB closure,2026-10-10: twelve genuine incomplete originals supply stages0..2 in all four views; stage3 deliberately aliases four frozen final hardware originals while completion remains separate. All16 native model contacts/144 cell sections were reviewed. Public construction-registration approval is frozen. Seventeen focused original/PNG/alpha/registration/refusal checks pass21.038 seconds; the contract passes145 families/53,925 sprites and all packed metadata matches HEAD. [Construction identities](../../../../logs/graphics/geothermal-source/20261010-collector-01/construction-verification.json) retain96 files plus unchanged native sources/four binaries from54BB.1. No C++ rebuild/retest.54BC.1 composes final equipment glass and back/front source; hardware review alone does not complete54B.3 or activate runtime.


### Local Mandatory Reading - Collector equipment-state source composition

_Applies to Group54BC; supplements the Collector construction/state Local._

Read approved ready and construction source records, the public collector frame-owner map, actual collector completion/availability/publication and `UtilityPowerProducer.meter` interval/generation fields. Read only the genuine M1 facility-status component and the lossless collector partition implementation for composition. Construction frame selects native stage; completed frames select original glass phase. Apply actual glass in the whole native scene before per-cell partition; preserve every other original pixel/alpha. A blocked producer or unused offer does not create thermal demand, starvation, another vent or copied stock. Packing and loaded runtime remain later checkpoints.


### Group 54BC - Complete collector equipment states and publish source

{{HARD}}
- [X] 54BC.1 Compose exact construction/ready/working/blocked coverage, four real frames per state, four views, nine native cells and back/front parts from original models/glass; preserve native mode and completion ownership.
- [X] 54BC.2 Review all native stage/state/phase contacts and run focused exact-source/alpha/cue/provenance/refusal checks, consuming54BB.1 native boundary evidence without unnecessary rebuilds or full regressions.
- [X] 54BC.3 Publish complete deterministic source/approval identities and public metadata, verify the manifest and unchanged packed payload, then close54B.3 while retaining all packing/runtime obligations.

54BC closure,2026-10-10: complete1,152-pose source covers four states/four native frames/four genuine views/nine physical cells/aligned back-front parts. Construction selects actual stages with genuine amber glass phase0; completed states retain all four authored green/cyan/red phases. Only16 reviewed glass interiors change; every other original indexed pixel and all alpha stay exact. Solid native industry cells retain full sortable coverage without a vehicle aperture. All four full state/phase contacts were reviewed. Fifteen focused source/refusal cases pass54.701 seconds; seventeen model/partition compatibility cases pass21.941 seconds. Exact atlas/mapping/all contacts regenerate through `convert_geothermal_art.py --check`; source metadata is authored and complete. [Collector acceptance identities](../../../../logs/graphics/geothermal-source/20261010-collector-01/collector-source-verification.json) retain47 source/artifact files and reused unchanged native sources/four binaries. Public145/53,925 and all packed metadata match HEAD. No C++ rebuild/retest.54B.3 isX;54C.1 natural vent is next. Original54.1-54.3 still require both source families; packing, actual loaded rendering and operating generation remain later checkpoints.


### Local Mandatory Reading - Natural vent presentation

_Applies to Group54C; supplements Source generation Area._

Read `GeothermalVentField`, `FindGeothermalVentField`, the public `geothermal.vent` row, actual `GetMultiLayerTerrainSprite`/`GetMultiLayerSurveySprite` and their viewport anchors. Geology owns the bounded multi-tile field and immutable rate; per-tile art allocates no new source. Current mineral survey sprites do not deliver a vent overlay. Reconcile hidden/surveyed/paid-excavated visibility and the copied shortage label before authoring, preserving solid terrain and the difference between natural source information and installed collector machinery. Collector/native heat internals are unnecessary for these natural-surface pixels except their published availability boundary.


### Group 54C - Author the natural vent source

{{HARD}}
- [X] 54C.1 Freeze geology-owned field/per-tile visibility geometry and select the natural basalt/thermal source base, distinct from installed collector machinery.
- [X] 54C.2 Supply the public native tile presentations with fixed light/anchor and explicit field ownership, preserving the difference between surveyed information and paid excavation.
- [X] 54C.3 Complete genuine phase/state source and focused checks. Hidden presentation must not reveal a source through solid unsurveyed ground; downstream heat shortage cannot change geology's capacity or create another grant.

54C/54 source closure,2026-10-10: the natural source has hidden/surveyed/paid-excavated presentation; connected/idle/blocked equipment belongs to the separate completed collector. One original sheet supplies eight genuinely authored visible phases; four hidden poses select verified alpha-zero padding. All sparse64x31/anchor(-31,0) detail stays inside its native diamond and preserves the existing hardness floor. Actual sampled detail is60x29 at(2,1), with no pixel clipping. All twelve poses were reviewed over four native hardness floors. Sixteen focused source/visibility/alpha/provenance/refusal cases pass1.953 seconds; exact atlas/mapping/contact regeneration and145/53,921 public validation pass. [Natural source identities](../../../../logs/graphics/geothermal-source/20261010-vent-01/verification.json) retain31 files plus reused unchanged native sources/four binaries. All packed metadata and actual GRF match HEAD. No C++ rebuild/retest. Combined54 source is complete: collector1,152 poses plus natural12. Actual native vent visibility alongside mineral marks, packing and loaded runtime remain10.7; no operating generator is inferred.55.1 geometry is next.


### Local Mandatory Reading - Actual dated generator source geometry

_Applies to Groups55-56; supplements Source generation Area._

Read only the two public generator manifest rows, `MultiLayerGeothermalConstructionKind`/`GetMultiLayerGeothermalConstruction`, `PrepareGeothermalSite`, current registered IIDS/TIDS in `src/geothermal_definitions.cpp`, and verified Plan18 Group8B/`GetGeothermalGeneratorGeometry`; operating Group8 remains pending. Actual paid footprint/native cell layout and profile-owned construction terms must precede dependent originals; historical manifest sketches alone do not establish a delivered native type. If a dated geometry producer is missing, supply a bounded passive producer in its owning Plan18 group before authoring, retaining whole operating activation/transported bootstrap for master10.6. Reuse fixed camera/light and genuine original status components after geometry is proven; never substitute the collector body or silently activate an unfunded generator.

For resumed55.3 and later generator originals, use the [framed-sheet prompt and extraction checks](#framed-imagegen-sheets-and-safe-extraction). The bordered stage2 trial passed layout extraction but failed model identity; it remains excluded. Collector packing/display79C remains ahead of further generator authoring in the master queue.


### Group 55 - Author Vent-Tap Powerhouse

Remaining tasks for this mixed group are retained in [r2mlvs14-remaining-assets-and-runtime-integration.md](r2mlvs14-remaining-assets-and-runtime-integration.md#group-55---author-vent-tap-powerhouse). Original task IDs remain unchanged.

Dispatched under resumed master10.4. Plan18 staged8B verifies both dated4x4/sixteen-cell native geometries before55.1. Tasks55.1-55.2 are complete; unfinished55.3 is temporarily deferred behind the explicitly staged collector packing/display proof79C at master10.4B. Whole operating generator/transported bootstrap remains master10.6.

{{HARD}}
- [X] 55.1 For Vent-Tap Powerhouse, derive a single-model brief from its approved manifest/template: 1980 plant footprint and electrical output equipment. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [X] 55.2 Produce the required directions/rotations for Vent-Tap Powerhouse from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.

55.1-55.2 source closure,2026-10-10: four genuine whole originals preserve the selected turbine hall, single turbine/alternator, exchanger and electrical switchgear under actual physical turns. Each is losslessly divided into sixteen native64x64 cells; the unchanged camera/anchors and model continuity are reviewed at native and enlarged scale. A duplicate-view sheet and wrong-axis SW draft remain retained/excluded. Thirteen focused source/provenance/registration/geometry/refusal checks pass8.512 seconds; public145/53,921 validation passes. [Selected-view identities](../../../../logs/graphics/geothermal-source/20261010-venttap-01/verification.json) and [four-view contact](../../../../logs/proofs/mlm/native-art/ground/venttap-ready-views.png) retain exact originals, prompts and review artifacts. Construction and all five equipment-state source55.3 remain pending; packing/runtime/operating bootstrap are unchanged. No C++ rebuild or native retest.


### Local Mandatory Reading - M1 ground control source slice

_Applies to Group 78B; master checkpoint 8.5. Whole Group 78 remains pending for later orbital controls._

Read this plan's Global and Source generation Area, the seven selected `control.survey`, `control.excavation`, `control.tbm-area`, `control.tbm-turn45`, `control.exchange`, `control.deep-lock`, `control.future-lock` rows/briefs and existing `cargo.ERTH` / `cargo.ROCK` originals. Inspect `GetTBMPairControls` in `src/tbm_controls.h/.cpp`, actual portal widgets in `src/layer_portal_gui.cpp`, and project/area/exchange tools in `src/underground_gui.cpp`. `PreviewTBMCommissionArea` / `SetConstructionPreview` supplies canonical outline/turn geometry; sprite icons cannot grant construction, rotate a cutter or change a quote. Read existing layer icons and purchase-menu lock art for reuse. Native 20x20 enabled/disabled/pressed controls have no world heading. Keep locks/date cues in menus and controls, never on world vehicle bodies. Personal art review remains deferred. Testing/build verification is deferred until 2026-10-07 12:25:55 UTC by the user's low-power instruction.


### Group 78B - Author the M1 ground control subset

{{MEDIUM}}
- [X] 78B.1 Derive native ground briefs/bindings from actual survey, portal commissioning, area outline, legal 45-degree turn, exchange and Deep/date control owners; reuse delivered Earth/Rock cargo and existing layer/menu graphics. Retain originals/prompts/provenance.
- [X] 78B.2 Supply the seven ground control families in enabled/disabled/pressed states at 20x20 with fixed anchors, plus an explicit handoff to the existing canonical world outline/turn preview. No orbital collection/drop source is dispatched here.
- [X] 78B.3 Bake model-specific native/enlarged contact review; check source coverage, palette/anchors and distinct blocked/date/selection cues in a focused burst after the low-power window. Record the completed subset before closing master 8.5; leave original Group 78 unchecked.

78B source staging (2026-10-07): seven controls / **21 native 20x20 poses** are baked with retained layer/portal/cutter designs and UI-only Deep/calendar locks. [Original/prompt/bindings](../../../../media/multilayer/source/m1/control-generation-record.json) maps commissioning, floor, 45-degree advancing turn and exchange to their actual command/query/widget owners. Earth/Rock cargo stays exact; existing `SPR_CURSOR_QUERY`, white floor/blue turn-space/red uncovered `SetConstructionPreview` overlays are reused. Review: `mlm/reviews/multilayer-art/authoring/import-20261009/images/ground-controls-details-review.png`. Source status/coverage remains explicitly **verification deferred**, and master 8.5 stays unchecked until the requested focused checks can run after **12:25:55 UTC**. During this user-requested window, continue independent packing/tool development without activating unverified sources or claiming checks passed.

78B source closure (2026-10-07, after 12:25:55 UTC): [focused native controls](../../../../logs/r2mlvs13-x-control-source-checks.log) pass exact regeneration, seven-family/21-state coverage, palette/anchor/public bindings and independent missing-state/bad-bounds/illegal-color refusals. Public source coverage is verified; review/provenance status now matches that evidence. Only this declared ground subset and master 8.5 close; original whole Group 78, orbital collection/drop controls and all native runtime integration remain pending.


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

79B closure (2026-10-07): forty selected families supply 2,311 native sprites in independent Action5 type1C segments. [Two clean pinned conversions/builds](../../../../logs/r2mlvs13-ap-pinned-pack-reproducibility.log) reproduce GRF SHA256 `cc70556571c1b12549538baa5ea91cf176527faacad3f6d1299682defd1e79ac` and GRFID container-v2 checksum `9e6ba7ea0fa173a5d3129c2922875e46`. [Generator-free packaging](../../../../logs/r2mlvs13-aq-generator-free-packaging.log) disables Grfcodec discovery, removes only the disposable build copy, executes the checked-in source-copy rule, and [actually loads every protected/selected sprite](../../../../logs/proofs/mlm/native-art/logs/r2mlvs13-al-generator-free-load.log): 40,431 assertions / one case. Generator discovery is restored. Protected source PNG/NFO and immutable MLVI definitions retain their identity; reserved/unselected slots stay unactivated. [Packed handoff](../../../../logs/r2mlvs13-ar-packed-handoff.log) binds public native ranges/hashes only after passing build/load evidence. Runtime consumers and user acceptance remain pending.


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


### Local Mandatory Reading - Delivered collector packing and actual display

_Applies to staged Group79C; consumes only the source-complete collector before unfinished generator source work._

Read this plan's Global, the Source generation Area's packing exports and only collector-relevant runtime ownership from the Runtime integration Area. Inspect `m1-pack-scope.json`, the public `facility.vent-collector` row and `collector-source-manifest.json`; source completeness and native anchors/cells must be real. Read `pack_m1_art.selected_ranges`, `source_poses`, `format_header`, `derive`, `publish` and `bind_packed_payload`, the base-set PNG/NFO/CMake routes and current generated native ranges. Preserve protected0..65, reserved slots, all existing42-family ranges, both generated and checked-in payload paths and immutable MLVI gameplay definitions.

For display, read `GetMultiLayerSprite`, `GetMultiLayerReceiverSprite`, native `DrawTile_Industry`, `GetIndustryConstructionStage`/`IsIndustryCompleted`, `GetGeothermalCollectorState`, `IsGeothermalCollectorOperational` and the actual collector/claim/shared-Heat state owners needed by the selected state. Reuse actual industry location/access/work/input/Heat records; do not invent a graphics-owned stock or operating generator. Inspect existing focused native sprite-loading and collector save/constructor fixtures only. A normal-game capture must disclose fixture-supplied materials and cannot claim the later Surface-transported bootstrap.


### Group 79C - Pack and display the delivered collector

Remaining tasks for this mixed group are retained in [r2mlvs14-remaining-assets-and-runtime-integration.md](r2mlvs14-remaining-assets-and-runtime-integration.md#group-79c---pack-and-display-the-delivered-collector). Original task IDs remain unchanged.

Explicitly staged by master10.4B after verified source54, before further unfinished55-56 originals. This closes only the collector subset of packing/runtime obligations; original whole Groups79-84 and aggregate master10.4 remain pending for their other selected families.

{{HARD}}
- [X] 79C.1 Append the complete1,152-pose collector family to the selected native packing scope. Verify original/atlas provenance, all four states/frames/views, nine actual cells and aligned back/front parts; preserve every old range and exact original palette/anchor semantics.
- [X] 79C.2 Emit deterministic PNG/NFO/header/public mappings and the actual GRF through the existing generated and checked-in packaging routes. Independently decode the newly packed pixels/anchors and protect existing native sprites; metadata alone does not prove payload inclusion.

79C.1 source/range closure,2026-10-10: collector poses append at2436 through3587, yielding43 families/3495 selected sprites and native bound3588. The frozen42-family range/declaration baseline preserves every prior enum, offset, count, source/cropped box, anchor, palette and all2343 source-pose pixels. The packer now validates the collector producer's explicit complete single-atlas schema; it refuses stale identity/coverage, shifted/overlapping/omitted native families and reserved-slot use. Fourteen new packing checks, seventeen collector source and fifteen construction/state checks pass; all3495 selected source crop/anchor round trips pass. Two complete source derivations are byte-identical. [Source/range receipt](../../../../logs/graphics/collector-integration/20261010-pack-01/verification.json) retains outputs and unchanged original M0 PNG/NFO plus shipped GRF. Actual new GRF decoding, generator-free packaging and loaded rendering remain79C.2-79C.4; no runtime activation or operating powerhouse is claimed. No C++ build has run for this slice.

79C.2 payload closure,2026-10-10: two actual generated GRF builds are byte-identical (`eaf09566e956785b0bd12034777d176ae822c461daebbd6fc18f072169c54a88`, GRFID checksum `3b235499e6d78dc0dccd2198d581b33f`). With generator discovery disabled, the existing cached packaging target copies those exact bytes; the normal CMake discovery setting is restored afterward. Independent GRFCodec decoding compares all3561 actual native sprites against source:66 protected M0 plus3495 selected poses, including every collector state/frame/view/cell/part. Native offsets, dimensions, anchors, DOS palettes and every index match exactly. [Payload receipt](../../../../logs/graphics/collector-integration/20261010-pack-01/payload-verification.json) and [build/copy proof](../../../../logs/proofs/mlm/native-art/packed-build-proof.json) retain current source/output hashes and exact diagnostics. Source/public range mappings are emitted; the stricter public loaded-payload hash/activation binding still waits for the compiled native loader in79C.4. No game/test executable was rebuilt or launched by this payload-only task.

79C runtime continuation,2026-10-10: [Plan14 tasks79C.3-79C.4](r2mlvs14-remaining-assets-and-runtime-integration.md#group-79c---pack-and-display-the-delivered-collector) now pass native selection/loading, both builds and actual ordinary-game construction/completion display, closing master10.4B. Completed79C.1-79C.2 source/payload evidence remains owned here. The native collector is active; transported first-plant bootstrap and operating generators remain later master scopes. Resume Plan14 task55.3 through its applicable Local reading.


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


### Local Mandatory Reading - M1 native terrain and portal binding

_Applies to Group82 and the staged Group83 return in master8.7._

Read this plan's Global and Runtime integration Area, then `multilayer_sprite.h`, generated `multilayer_sprite_assets.h` and the selected packed handoff. Inspect only `GetGeologyWorldState`, `PackedExcavationState::Get`, layer-bounded `TileAddWrap`, `GetLayerPortalWorkState`, reciprocal portal metadata and `DrawTile_Clear` / `DrawTile_LayerPortal` / `DrawLayerPortalPreview`. Preserve Classic/no-geology artwork, real layer filtering, existing rail overlays, building transparency and vanilla portal sorting boxes. Terrain mask bits follow screen upper-left/upper-right/lower-right/lower-left; texture frames are static variants. Verify the focused excavation-storage/portal-work-state producers before binding. Group82.3's actual final-machine crossing capture follows Group83.1-83.2's native vehicle/bay binding; this staging resolves its presentation dependency without claiming placeholder crossings as final art. Group83 then requires only the active train/project/exchange/receiver/service owners and actual sound event path, not unselected orbital/industry providers.


### Group 82 - Integrate M1 terrain and portal states

{{HARD}}
- [X] 82.1 Verify r2mlvs2/3 state queries and matching manifest entries, then bind Solid/Excavating/Excavated terrain and Sealed/Commissioning/Connected portal graphics to authoritative state.
- [X] 82.2 Check adjacent tile edges, both depth palettes, every portal orientation and extra-view layer filtering in runtime scenes. The renderer does not advance excavation.
- [X] 82.3 Capture frame evidence for an assigned TBM entering/reversing through a sealed work pocket; validate front/back occlusion at each crossing phase.

82.1-82.2 closure (2026-10-07): the real clear-tile and rail-portal renderers consume geology/work queries and packed source poses; Classic/no-geology and road art retain their original paths. [Producer checks](../../../../logs/r2mlvs13-ay-terrain-portal-producers.log) pass 122 assertions / three cases, and [native bindings](../../../../logs/proofs/mlm/native-art/logs/r2mlvs13-be-terrain-portal-checks.log) pass 80 / three cases, including a real reserved TBM job and unchanged controller/job state. [Actual native raster checks](../../../../logs/r2mlvs13-bm-native-picker-capture-checks.log) pass 34 assertions / one isolated application case and preserve the immutable stopped Deep save: four independent-layer native/2x viewport frames and four real picker scenes / sixteen depth/endpoint/orientation views. [Gallery metadata](../../../../logs/proofs/mlm/native-art/geology/terrain-hardness/reports/m1-terrain-runtime.json) records PNG hashes. Normal/zoomed terrain and basic/Deep mouth views were inspected; roofs fit the native 64-pixel picker and existing sorting/transparency remains in use. Render-time geology/projects/receivers/RNG remain unchanged. User personal acceptance and82.3's final-machine crossing evidence stay pending; the latter follows83.1-83.2 as staged above.

82.3 closure (2026-10-07): [actual loaded Deep source loop](../../../../logs/r2mlvs13-cx-native-loop-deep-3.log) passes197,581 assertions / one native application case through32,853 real frames and three exchanges. Its [40 native/2x rasters and physical-state records](../../../../mlm/reviews/tbm/spoil-loop-deep/import-20261009/reports/m1-loop-deep-3-runtime.json) cover assigned entry/spanning/pocket, supported backing through both real housings and the correct independent endpoint/layer sorting. Mouths occlude the machine while it is inside; four same-heading drilling phases farther into the actual corridor show the exposed cutter/support and their preserved anchors. Native/2x entry/retreat, drilling and Surface disposal/returned-empty scenes were inspected. Each render preserves actual vehicle packet/position, geology/project/assignment/receiver/cash and both RNG owners; the immutable stopped source and startup script remain unchanged. The first live attempt exposed front-only consist recalculation in the animation refresh; it now calls each physical part's viewport refresh, with [complete-chain/unchanged-physics checks](../../../../logs/r2mlvs13-db-native-animation-refresh-checks.log) passing3,764 / four cases. The first empty-positioning snapshot was replaced by the actual post-disposal empty return. Deep source crossing evidence is complete;83.3 additionally runs and inspects the separate ordinary Underground depot loop. User personal art review remains deferred.


### Local Mandatory Reading - M1 actual train, cargo and facility presentation

_Applies to Group83 under the staged master8.7 order._

Read this plan's Global/Runtime integration Area and the delivered selected packing/terrain closures. Inspect `Train::GetImage` / `Train::Tick` / `Vehicle::UpdateViewport`, actual `GetTBMEngineModel` / `GetUndergroundEngineCapability`, `GetTBMProjectForCutter` / `GetTBMConsistRole` / `GetTBMWorkState`, `IsDrivingBackwards` and saved exchange assignments. Read actual wagon `cargo.StoredCount` / `cargo_cap`, native depot occupancy and Surface receiver stock/interval queries only when binding those consumers. Verify focused TBM-work/exchange-commit producers before integration. Actual part direction supplies physical bend/backing continuity; reversing changes movement, never rotates the cutter body to the motion direction. A stopped loaded wagon keeps its real partial/full load and frame-zero wheels. Derive and invalidate only presentation caches through the existing train tick/rendering path; no new work tick, packet store, role registry or simulation RNG. Use actual saved-world viewport captures for crossing/loop evidence. Audio later reads the real phase/transition owner and distributed sample IDs separately.


### Group 83 - Integrate M1 cutter, wagons and exchange facility

{{HARD}}
- [X] 83.1 Bind r2mlvs4 work phase and heading to cutter/support/boring sprites, preserving directional continuity through legal 45-degree turns and supported retreat.
- [X] 83.2 Bind actual Earth/Rock load quantities to wagon states and r2mlvs5 stopped exchange to bay foregrounds; load graphics follow the same wagon IDs after coupling.
- [X] 83.3 Capture a real dig/fill/exchange/Surface-haul/empty-return loop and inspect clipping at native/zoomed scales. Correct assets or anchors, repack and retain reviewed hashes before claiming M1 graphics complete.
- [X] 83.4 Publish and verify the M1 TBM idle/boring/backing/service sound-event mapping required by Plan 4 task 16.2. Declare concrete distributed sample IDs/source/licensing or reviewed reuse of existing base sounds, verify loading and actual event playback, and update the consumer hook. Events consume authoritative phase/transition state and never advance work, charge money or consume simulation RNG. Track this audio mapping separately from raster frame coverage; an absent sample/hook remains pending. Execution is scheduled by [R2MLVS3MP Group 8](r2mlvs3mp-master-plan.md#group-8---deliver-and-review-the-complete-m1-asset-slice).

83.1-83.2 closure (2026-10-07): `Train::GetImage` consumes actual native engine identity, immutable cutter/support role, physical body facing and saved work/hold/service/backing facts. Backing changes movement rather than turning the cutter body; the existing front-engine tick refreshes only native visual caches at authored phase boundaries. Packet quantity takes priority over empty service/hold cues and stopped hopper wheels remain frame zero. Rail depot rendering reads the existing project/assignment/spatial occupancy owners and uses authored exchange/service back/fronts with existing overlays/transparency. [Producer checks](../../../../logs/r2mlvs13-bn-machine-exchange-producers.log) pass 39,683 assertions / eight cases; [actual GetImage, paid-cut/load/depot and genuine-coupling checks](../../../../logs/proofs/mlm/native-art/logs/r2mlvs13-bw-native-image-exchange-checks.log) pass 3,762 / four cases. The same loaded wagon IDs preserve their packet quantities and source poses after real two-way coupling. Actual final-machine crossing/loop capture and sound remain pending. The remaining selected facilities/UI consumers are scoped83B below, before the combined raster capture.

83.3 closure (2026-10-07): real Deep source and Underground depot loops both load the immutable stopped saves and run the ordinary application/command/timer owners. [Deep3](../../../../logs/r2mlvs13-cx-native-loop-deep-3.log) passes197,581 assertions / one case through32,853 frames and three exchanges; [Underground2](../../../../logs/r2mlvs13-cx-native-loop-underground-2.log) passes94,385 / one through15,677 frames and two exchanges. [Deep review metadata](../../../../mlm/reviews/tbm/spoil-loop-deep/import-20261009/reports/m1-loop-deep-3-runtime.json) and [Underground review metadata](../../../../mlm/reviews/tbm/spoil-loop-underground/import-20261009/reports/m1-loop-underground-2-runtime.json) retain40/28 native/2x PNG hashes, four actual drilling phases and real dig/load/coupling/Surface disposal/returned-empty scenes. Source bays physically retreat through their commissioning pair; ordinary rock-layer depots physically receive their cutter there, as the actual resolved bay kind requires. Rendering snapshots preserve actual packets/positions, terrain/projects/assignments/receiver balances/cash and both RNG streams. Technical inspection found correct exposed body/support alignment, native bay foreground occlusion and stopped load/empty rendering; initial first-cell work remains correctly behind the mouth roof. No raster correction/repacking was needed. The real refresh assertion was corrected and verified as recorded above. Each scenario is a separate bounded focused invocation; no full regression ran. Personal user review and final M1 gallery acceptance remain deferred, and83.4 sound is current.

83.4 closure (2026-10-07): `multilayer_sound.h/.cpp` and the existing front-engine tick bind actual idle/unblocked boring/moving backing/genuine depot state to reviewed base-slot reuse: mine machinery9, cutting motor56, warning bell12 and metallic rail-tool30, with192/96/96/192 tick cadences. `media/multilayer/m1-sound-events.json` records mapping authority, selected-provider rights and exact IDs; no sound samples or proprietary CAT bytes are copied/repackaged. Existing mute, effect-volume, viewport and dedicated behavior owns audibility. [Owner/mute/cadence/depot/assembly-reversal checks](../../../../logs/proofs/mlm/native-art/logs/r2mlvs13-do-native-sound-selection-checks.log) pass95 assertions / three cases. The assembly-magazine reverse command correctly does not create actual backwards motion; genuine backing is verified by the live loop. [Actual native phase/decoder/mixer playback](../../../../logs/proofs/mlm/native-art/logs/r2mlvs13-cx-native-loop-underground-3.log) passes110,129 / one isolated case: all four events are selected by the real moving/stopped/depot controller at ticks0/81/3299/4178 and each emits nonempty signed stereo PCM at11,025Hz. Temporary camera/audio sampling preserves geology/projects/assignments/receivers/cash and both RNG owners. [Publication and silent-dispatch corruption refusal](../../../../logs/r2mlvs13-dp-native-sound-handoff-checks.log) bind that evidence in `logs/proofs/mlm/native-art/sound-runtime.json`; decoded sizes/counts/fingerprints are retained without exporting sample audio. Native raster packing/content identities remain unchanged. Personal sound/art review remains deferred; master8.7 continues with the linked Plan2/3/4 presentation tasks.


### Local Mandatory Reading - Selected M1 service, receiver and UI consumers

_Applies to Group83B; scheduled after83.1-83.2 and before82.3/83.3 combined capture under master8.7._

Read child Global/Runtime integration Area and only delivered selected family ranges/source geometry. Inspect `DrawTile_Industry`, native receiver kind/stock/room/interval queries and fixed world-cell offsets; `DrawTile_Road` depot art and real road-vehicle occupancy; actual underground/portal/TBM widget state/capability queries, `GetRailIcon`, first-party cargo icons and geological survey visibility/depletion. Native receiver and road-service family dimensions/states/parts are authoritative in the public manifest. Preserve foreign/Classic art, date/capability refusals, existing transparency and all stock/cargo/work owners. No presentation clock processes a batch, creates a depot, unlocks a control or surveys a seam. Reuse existing world preview/cursor primitives. Verify focused active producer queries; actual combined runtime capture is later83.3, with user personal review deferred.

Context has compacted 1 times while executing this group.


### Group 83B - Bind the remaining selected M1 facilities and controls

{{HARD}}
- [X] 83B.1 Bind the selected ordinary road-service depot and Surface Earth/Rock receiver parts to actual occupancy, construction, stock and bounded-intake state; retain fixed cells and transparency.
- [X] 83B.2 Bind the selected seven ground controls, ten purchase states, two cargo icons and ten survey marker families to their existing UI/capability/cargo/geological queries. Keep all calendar and command gates with their owners and reuse actual preview/cursor geometry.
- [X] 83B.3 Verify those live family bindings and source ranges in focused cases, publish exact runtime consumer/evidence metadata for the selected set, then return to82.3/83.3 combined capture and83.4 sound. Unselected later families and user personal acceptance stay pending.

83B.1 evidence (2026-10-07): road/industry rendering consumes the actual native service/receiver selectors, fixed cells and occupancy/construction/stock/interval/aggregate bounds. Road-type custom depot graphics and foreign industries retain their resolvers; existing transparency remains active. Native receiver lamps refresh only tile presentation dirtiness through the existing receiver tick, before unchanged processing. [Receiver producer queries](../../../../logs/r2mlvs13-bx-receiver-art-producers.log) pass 691 assertions / six cases; [fixed-cell/stock/construction/road-owner checks](../../../../logs/proofs/mlm/native-art/logs/r2mlvs13-ce-native-facility-art-checks.log) pass 147 / two cases. The road fixture opens its actual entrance and initializes the road registry before invoking the ordinary construction command. Actual combined raster capture remains the staged83.3 work after selected controls are bound.

83B.2 evidence (2026-10-07): actual underground widgets use seven native control families through standard image/text buttons or reserved informational icon regions; unchanged cursor/quote geometry owns placement. Purchase/list rendering resolves the real five engine mappings and existing buildability; the portal button uses its own commissioning query. Cargo icons require the actual first-party definition owner and preserve foreign labels/graphics. Survey overlays require the active lowered survey cursor and its explicit plane, expose the existing public/free geology policy and read actual depletion without setting saved survey flags. A focused cancellation fixture caught and corrected stale overlay ownership. [Controls, calendar, cancellation, ten resources, package ownership and actual EIDS checks](../../../../logs/proofs/mlm/native-art/logs/r2mlvs13-cj-native-control-art-checks.log) pass186 assertions / six cases; [Debug game/test build](../../../../logs/r2mlvs13-ci-native-control-art-build.log) passes. Actual UI raster/range metadata is the following83B.3 task.

83B.3 closure (2026-10-07): [actual loaded control/window/purchase/cargo/survey rendering](../../../../logs/proofs/mlm/native-art/logs/r2mlvs13-cl-native-control-capture.log) passes81 assertions / one isolated application case, with six lossless PNG review images and immutable source/RNG/work/receiver/reserve/cash owners. The [capture runner](../../../../logs/r2mlvs13-cq-native-control-raster-checks.log) retains exact window dimensions/pixels and restores the startup script. Ground menu, portal panel, five real engine purchase previews, two actual first-party cargo icons and Deep seam rendering were inspected for placement/clipping. [Existing area/turn/facility quote checks](../../../../logs/r2mlvs13-cu-native-area-presentation-checks.log) pass302 / three cases. [Exact selected binding publication and three stale-range/payload/raster refusals](../../../../logs/r2mlvs13-ct-native-runtime-binding-checks.log) publish40 families /2,311 sprites in the public manifest and `logs/proofs/mlm/native-art/runtime-manifest.json`; actual call sites, loaded payload and emitted ranges agree. The verifier reads both PowerShell UTF16 and native UTF8 evidence without rewriting logs. Native source/definition bytes remain unchanged; unselected later families remain inactive. Headless window paint runs inside an explicit screenshot canvas, and native buttons reserve their full20px cell before paint changes state. Current work returns to82.3/83.3 final-machine crossing/loop scenes and83.4 sound; gallery/user acceptance remain pending.


### Local Mandatory Reading - Selected M1 final gallery

_Applies to Group 85B under master 8.8; original Groups 85-88 remain the full-profile obligations._

Read child Global and Runtime integration Area only for the delivered M1 geology/portal/TBM/exchange/UI owners. Read the selected forty-family public/packed/runtime manifests and retained terrain/control/Deep/Underground loop reports. Inspect `NativeArtLoopProbe`, `CaptureNativeArtViewport`, actual `Train::GetImage`, `CheckClickOnVehicle` and existing building/industry transparency. Preserve immutable source saves, final payload identity and all simulation owners. Direction sheets query the real renderer without rotating a vehicle; animation and occlusion claims require actual driven world frames. Keep technical acceptance and the user's deferred personal review separate. Do not preload later launch/module/dock families.


### Group 85B - Review the selected M1 runtime slice

{{MEDIUM}}
- [X] 85B.1 Supplement genuine native/2x loop captures with paused/idle/service views, eight-direction renderer queries, building transparency and actual visible-body/layer picking checks.
- [X] 85B.2 Produce a compact contact sheet and linked native scenario gallery for all forty selected manifest IDs, retaining exact frame/source/log provenance and actual animation phases.
- [X] 85B.3 Confirm selected placeholders are retired and distributed GRF/source/range hashes match reviewed evidence; document retained limits and bounded stale-evidence refusals.
- [X] 85B.4 Record technical acceptance separately from deferred user feedback; keep unselected full-profile gallery rows pending.

85B.1 closure (2026-10-07): the optional gallery observer retains genuine paused/idle and real railway-depot service poses, queries all eight directions through actual `Train::GetImage` without rotating a unit, renders the same physical portal/disposal scenes with existing building/industry transparency, and verifies exposed cutter picking on its own layer with rejection on another layer. [Underground5 capture](../../../../logs/r2mlvs3mp-s-gallery-underground-capture-1.log) passes 94,734 assertions / one native case, fourteen physical scenes / forty-five PNGs. [Deep4 capture](../../../../logs/r2mlvs3mp-s-gallery-deep-capture.log) passes 197,868 assertions / one native case, sixteen physical scenes / fifty-four PNGs. Actual logs/source/frame hashes are retained in [Underground5](../../../../logs/proofs/mlm/native-art/tbm/spoil-loop-underground/reports/m1-loop-underground-5-runtime.json) and [Deep4](../../../../logs/proofs/mlm/native-art/tbm/spoil-loop-deep/reports/m1-loop-deep-4-runtime.json). Native direction/load/service/transparency frames were inspected. Rendering preserves projects/jobs/packets/positions/geology/receivers/cash and both random streams; original sources remain unchanged. Direction sheets are explicitly renderer queries; animation and occlusion evidence comes from actual driven world frames. 85B.2-85B.4 remain pending.

85B.2-85B.4 closure (2026-10-07): [gallery and distribution acceptance](portal-tbm/r2mlvs3mp-t-gallery-acceptance.md) retains the [standalone offline gallery](../../../../logs/proofs/mlm/native-art/gallery/m1-runtime-gallery.html), [native contact sheet](../../../../logs/proofs/mlm/native-art/gallery/m1-runtime-contact-sheet.png) and exact forty-family/2,311-sprite review mapping. There are 157 original native frames, including all ten actual resource seams and separate actual road/unassigned rail-service depots built on paid-open ground. [Publication checks](../../../../logs/r2mlvs3mp-t-gallery-publication.log) verify actual source/frame/build/distributed GRF hashes and refuse missing real phases, substituted rasters and incomplete scope. The selected placeholders are retired; original/protected/reserved bytes and source poses retain their packing authority. Gallery frames/contact placement and offline links were inspected. Later binding refresh preserves technical review only when its complete current proof still verifies. User feedback is recorded as deferred, separately from technical acceptance. Original full-profile Groups 85-88 remain unchecked for later families.

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

All173 retained implementation tasks are checked with their original evidence and limited to demonstrated contract/source/payload/runtime scope. This completes the archived portion only. Every remaining authoring, loading, integration and acceptance obligation is preserved in [Plan14](r2mlvs14-remaining-assets-and-runtime-integration.md); no whole-profile or master milestone completion follows from this split.
