# R2MLVS13 - Graphics and Asset Pipeline

Status: proposed; no new images generated during planning. Parent: [R2MLVS master](r2mlvs-multilayer-vehicles-and-structures.md). Group 1 uses draft definitions from r2mlvs1 Groups 1/2 and feeds its Group 3 closure; integration follows each consumer. Final art is required for milestone acceptance.

## Outcome and reading

Produce a coherent OpenTTD sprite set covering terrain, all specialist vehicles, functional buildings and important state changes. Read master I-10 and its content catalogue; [base sets](../../architecture/basesets/archi-basesets.md), [viewport](../../architecture/gui/archi-viewport.md), [prior graphics plan](r2mlm6-layer-graphics-assets.md), [prior art repairs](r2mlm13-layer-art-review-and-new-sprites.md), `src/table/sprites.h`, Action 5 handling and the existing `tools/sprite_quantize.py`/`tools/make_sprite_template.py` contracts.

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

Exact sprite counts depend on footprints, rotations, load stages and animation frames. Freeze them in Group 1 before batch generation. The family table is a coverage obligation, not a claim that one image per row is enough.

## Group 1 - Art and runtime contract

- [ ] 1.1 Inventory reusable existing assets and new requirements. Preserve offsets 0..65 of the current layer block; append core terrain/UI assets or allocate a new documented block. Content-package sprites follow the package's stable IDs.
- [ ] 1.2 Create a manifest with asset ID, consumer, frame/direction/state count, native dimensions, tile footprint, anchor, bounding box, transparency, company colors, palette treatment, foreground cuts and source/provenance.
- [ ] 1.3 Set the art direction: readable isometric TTD scale, consistent light/shadows, native-resolution silhouettes, distinct rock/resource colors and grounded machinery proportions. Use original reference assets only under their actual rights.
- [ ] 1.4 Freeze 8bpp DOS palette/index-0 transparency for the first deliverable, normal-zoom authored sprites and engine scaling. Specify handling of reserved animation/company colors; optional 32bpp/high-zoom art is later work.
- [ ] 1.5 Produce one representative TBM, mine, rocket and module direction/animation test before generating whole families. Validate geometry and direction labels in the engine or exact placement templates.
- [ ] 1.6 Extend the manifest for all five original tube capsules and geothermal technologies. Require visually distinct silhouettes and functions, moving-capsule visibility through tube shells, cutter heading/45-degree bend continuity and separate Surface/orbital launch-area overlays before batch generation.

## Group 2 - Source generation and reproducible conversion

- [ ] 2.1 For AI-generated raster assets, use the available image-generation skill/tool at execution time, preserving prompts, references, originals and provenance in repository asset folders. This planning task authorizes no generated-art completion claim.
- [ ] 2.2 Fit/clean the selected originals into consistent sprite families. Keep direction continuity and load state geometry; generic rotated 2D images are not correct isometric vehicle views.
- [ ] 2.3 Extend repeatable conversion/packing to quantize, validate native coverage/anchors, split fronts/backs and emit NFO/content-package metadata. Retain editable originals and deterministic tool versions.
- [ ] 2.4 Package assets through the selected core/content routes; regenerate required GRFs/hashes and validate IDs/counts. A build without the generator must still use the checked-in generated payload correctly.
- [ ] 2.5 Validate missing frames, wrong dimensions, accidental transparent holes, forbidden palette entries and manifest references automatically. Keep simulation state independent of animation timing.

## Group 3 - Runtime integration and review

- [ ] 3.1 Integrate M1 excavation/TBM/exchange graphics first; then all ten mines, the underground fleet and orbital/solar families with their owning plans.
- [ ] 3.2 Capture native-scale and zoomed scenes across directions, terrain boundaries, load/production states, vehicle overlap and building transparency. Use real runtime frames for animation/occlusion claims.
- [ ] 3.3 Check each launch center/module footprint as a complete site, not isolated tiles. Verify correct vehicle hiding at bays, portals and docks, drawing on the correct layer and clickable moving bodies.
- [ ] 3.4 Produce a compact contact sheet and in-game scenario gallery linked to manifest IDs. Record technical acceptance and user art feedback separately; do not inherit prior plans' skipped art approval.
- [ ] 3.5 Replace all milestone placeholders, document retained limitations and confirm distributed assets match the reviewed hashes.
- [ ] 3.6 Capture all five tube models in motion/load/service states, geothermal power/direct-smelting/battery/forge states at their valid dates, TBM area-clearing bends and off-center cargo drops with train/lorry collection. Include overlapping area overlays, map edges and blocked/locked states in the runtime gallery.

## Completion conditions

Every required building, vehicle, resource and gameplay-critical state has packaged graphics, valid directions/anchors and a runtime capture. Repeatable conversion works from retained sources. Offline sheets, generated originals and a successful compiler run alone do not establish art acceptance.
