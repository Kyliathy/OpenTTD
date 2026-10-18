# R2MLSP - Fixed Core source geometry and base selection

Owner: [Space master](../../../zz-reach2/upgrades/2026-10/r2mlsp-space-master-plan.md), [Plan11 Group13E](../../../zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md#group-13e---deliver-core-source), [Plan14 Group70](../../../zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md#group-70---author-core-module).

**Superseded by the [completed70.1 checkpoint](r2mlsp-z-z-z-h-core-base-checkpoint.md).** The user requested stopping after the current task;70.1 is now complete and all handles are terminal. The pending/live descriptions below are retained generation history. Resume at70.2 only when instructed.

## Earlier current task

Hauler13D is complete; its [final handoff](r2mlsp-z-z-z-f-hauler-complete-source-notes.md) remains authoritative. **Core70.1 is in progress**, with no approved base or source coverage yet.70.2 tile cuts and70.3 functional states follow sequentially. No rebuild or Git writes.

Read Plan11 Global/Presentation Area/Remaining individual orbital sources Local and Plan14 Global/Source generation Area/Actual fixed Core source Local before resuming. Mandatory reading is non-transitive. Read only current public `module.nucleus`, `module_contract` and the native geometry owners below. The framed extraction rules and installed imagegen skill were read before this generation.

## Native geometry established before raster work

- `GetSpaceConstructionDefinitions` in `src/multilayer_space_content.cpp` defines Core3x3. `SpaceStructureState` has fixed x/y and no orientation. Source direction is `none`; nine world cells ordered x-fast within y, each64x64 with anchor[-31,-33], parts back/front. Four public states blueprint/partial/complete/blocked and two logical frames require144eventual part cells.
- `GetSpaceStructureApproach` in `src/space_structure.cpp` returns(x,y+height). `GetSpaceStructureApproachCapacity` supplies Core exactly one slot, and `GetSpaceStructureApproachSlot` offsets only Dock's second slot. `GetOrbitalModuleServiceCapacity` gives Core one service slot. Imagery cannot add another berth.
- `SpaceWorldPoint` in `src/asteroid_gui.cpp` maps Q16 positions to tile centres using the actual Space layer origin. Existing footprint drawing subtracts half a tile before drawing each tile. `RemapCoords` in `src/landscape.h` projects a native tile step as x=(-32,+16), y=(+32,+16), after the base zoom factor.
- Whole source review canvas256x192, footprint upper corner[128,64], other corners[224,112],[128,160],[32,112]. Per-cell image origins are[97+32*(y-x),31+16*(x+y)]. These retain the existing64x64/-31,-33 contract; they are source placement coordinates, not saved gameplay positions.
- Actual approach tile centre is outside the occupied floor at[224,128]. Its coupling contact on the y=3edge at x=0.5projects to[208,120], one-sixth of the way from the diamond's right vertex toward its bottom vertex. Keep this entry clear; do not centre a cosmetic berth elsewhere on the edge.
- `AreOrbitalModuleFootprintsAdjacent` accepts overlapping orthogonal edges, not diagonal corners. Recessed edge fittings are cosmetic anchors; they do not create a connector graph or a new adjacency authority. Core's actual paid panel/shared allocation, job inputs/work and original module stock retain gameplay ownership.

## Base brief and current original

One low central octagonal ivory/teal command hub, one pair of low navy solar-panel banks on the rear deck, one shallow supply locker toward the nearer left, a thin3x3 diamond floor with recessed conduits, and exactly one small low coupling near the right vertex on the front-right edge. No vehicles, duplicate service bay, tall antenna, outside platform, engines or cargo invented by the image. Existing native counts and budget own all capacities and output.

Built-in imagegen output `media/multilayer/source/space/raw/orbital-core-complete-v1.png`, SHA256 **4bbdcb89aa202664dd083009c7732bde28e78e51d7c2c3c2b038130256877021**, is retained with its exact prompt receipt. It is rejected because the mouth lies near the middle of the edge rather than the actual approach contact. The image's transparent background RGB looks magenta in the raw viewer; this is not opaque painted haze. Its actual separator pixels are outside measured crop[56,56,1480,968]; that crop has zero visible separator pixels and alpha16subject bbox[42,157,1383,828]. A targeted built-in correction moves only the original mouth toward[1320,607] in its1536x1024source layout. Preserve whole originals; never move artwork or repair alpha in code.

`convert_space_module_art.py` is a new source-only Core adapter, not yet tested against an approved base. It validates public geometry, exact ancestry, a signed whole-model registration, measured crop/clearance, one uniform scale, measured floor vertices and coverage by actual native cell canvases. It creates only ordinary converted native/contact review images. Source partitioning and front/back assignment remain70.2.

Before Core changes, `logs/space-core-protected-boundary.json` SHA256 **dfe25b0c8bb37ed968ebf3ac9dd904a229c0f96db974211446f4eecb9cf45602** recorded10accepted source families,163candidate records,147other public rows and exact GRF. Preserve all of them. Public Core remains planned, with no packed range or runtime claim.

Use explicitUTF8, repository logs, documented source functions/classes/arguments and business explanations for compound guards/nontrivial loops. Tests stay focused, children below110s, test waits>=50000ms and individual blocking waits<=60000ms. No subagents or Git writes. Next auxiliary prefix `r2mlsp-z-z-z-h-`; source/test/JSON names never start r2.

## Current correction dispatch

**Only imagegen cell2786 is live.** Cells2762/2768/2772/2776/2780/2783 are terminal. No Core base is approved; no Core catalogue/public mutation or native conversion/test run has occurred.70.1 remains unchecked. The active request is a fresh BODY WITHOUT COUPLING, using only the shallow generator-facing guide. Prompt/result session keys: coreBaseBodyPrompt/coreBaseBodyResult. Inspect, copy and measure its actual output next.

The original whole-model requests repeatedly misplaced the coupling and steepened the floor. Revise source assembly within70.1: one genuine whole body plus one separately generated genuine coupling at the actual native contact. This is the established original-part composition route, not another native sprite range or runtime overlay. Native final back/front tile contract remains unchanged. Author the coupling only after the body geometry is sound. The new adapter currently still expects a whole source with a coupling; adapt it after measuring the body rather than claiming its unfinished prototype is complete.

Retained immutable original files and exact prompt/input receipts are `media/multilayer/source/space/orbital-core-complete-v1.json` through `-v6.json`, with raw PNG siblings under `raw/`. Their source decisions remain unapproved:

| Original | SHA256 | Observed reason |
| --- | --- | --- |
| v1 | 4bbdcb89aa202664dd083009c7732bde28e78e51d7c2c3c2b038130256877021 | Mouth in the middle of the edge rather than at actual approach. Measured frame-free crop is valid. |
| v2 | ec8cfb7970a0b5ae926d4f472debb6245f4117b30c5b05f4d09de7984f45a798 | Enlarged/steepened floor, berth still misplaced. |
| v3 | 9df8a5c8a54c10978f096c220af16c52361cb31211b1c23dfa5a30034ed41693 | Berth closer to right end; enlarged/steepened whole model. |
| v4 | 1ec74df1c81dcf873167a205c553aceeff39e8fb7107f39a0e73061e6a53b269 | Camera edit retained about0.54 upper-edge sample slope rather than0.5. |
| v5 | fea21a38f0b1ce1b62e608d47cb9dfdf03bf5d5df969e036083a7df48cc2aca0 |1254x1254 output copied rejected inspiration layout and middle-edge mouth rather than primary template. |
| v6 | eb2963c85c5ca5b3fb396a6c2085c414bc502cad53243dc1873e2df883bb551f | Fresh low hardware, but floor remains too deep and mouth is too close to extreme right vertex.1254x1254, alpha16bbox[156,361,1103,871]. |

Three geometry-only guides have pairedJSON receipts under `media/multilayer/guides/space/`: `orbital-core-berth-geometry-v1.png` draws the old1536native projection/contact; `orbital-core-floor-template-v1.png` draws the1024exact2:1 floor; `orbital-core-shallow-generation-template-v1.png` intentionally requests a shallower floor to compensate the generator's observed steepening. The shallow guide is explicitly NOT the native contract. No guide uses source artwork. Only actual generated output measurements can pass native geometry; no bitmap warping or guard relaxation.

The canonical source constants remain whole256x192, floor origin[128,64], corners[128,64],[224,112],[128,160],[32,112], one reserved approach[224,128], coupling edge contact[208,120]. Native per-cell origins/anchors remain exact. Uniform reduction and ordinary registration may accommodate pixel-centre rounding; source floor vertices must still agree within2native pixels and every opaque pixel must fit an actual64x64 cell. Inspect the actual deck plane separately from raised roofs/corner hardware and underside thickness; whole alpha bounds alone do not measure floor aspect.

`src/tests/space_art_core.py` contains12focused base/geometry/source-refusal cases, not yet run because no base is approved. Source/test AST and owner/class/function/all-argument docs pass. The prototype additionally refuses signed coupling-contact drift. Its base-only coverage assertions read the immutable base review, so later whole-family completion will not invalidate the checkpoint. No compiler, native runner or source conversion is live.
