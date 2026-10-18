# R2MLSP - Core base checkpoint before tile cuts

Owner: [Space master](../../../zz-reach2/upgrades/2026-10/r2mlsp-space-master-plan.md), [Plan11 Group13E](../../../zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md#group-13e---deliver-core-source), [Plan14 Group70](../../../zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md#group-70---author-core-module).

## Stop and resume boundary

The user requested stopping after the current task to switch model. **70.1 is complete; stop before70.2.** Resume only when instructed, at the nine actual world-tile cuts and back/front ownership, then70.3 functional sources. Plan11/master13E remain unchecked. Hauler13D is complete; its [final source handoff](r2mlsp-z-z-z-f-hauler-complete-source-notes.md) remains authoritative. No rebuild, Git write or subagent was used.

All imagegen/conversion/test handles are terminal, including the last coupling imagegen2792 and focused test75624. No compiler or native runner is live. No context compaction occurred during70.1; completed13D's2 and66D's1 notes remain historical. A later meaningful compaction during70 adds/increments the executing group, not completed hauler groups.

## Mandatory reads on resumption

Read Plan11 Global, Presentation Area and Remaining individual orbital sources Local; Plan14 Global, Source generation Area and **Actual fixed Core source Local immediately above Group70**. Then read current public `module.nucleus`, `module_contract`, the Core family and its two selected candidates in `space-generation-record.json`, and `convert_space_module_art.py`. Read only the relevant native geometry exports below, not historical native executable graphs or previous complete source families. Read canonical Plan13 framing/extraction rules and the imagegen skill again before further raster work.

## Accepted70.1 boundary

[Verification](../../../../logs/space-core-base-verification.json) records13focused cases/12.828s wrapper/exit0; conversion4.453s. [Base source review](../../../../logs/proofs/mlm/native-art/space/core-base-v1/base-source-review.json) SHA256 **8579197b3c9be651ed77a369158e748553ac62abe6b439c1690511f335e667f1** binds the exact native image/contact and source geometry. Whole native/4x body and assembled base were inspected. The final native PNG is byte-identical to the independently assembled inspected trial.

- One low octagonal command hub, two low navy solar banks, one shallow supply locker and one separately authored original coupling. One fixed view; no source rotation or extra berth.
- Whole source canvas256x192; actual native3x3 footprint, nine64x64 cells, unchanged anchor[-31,-33]. Parts remain back/front. Public four states and two frames require144eventual native part cells. Tile cuts and state matrix have **not** been delivered.
- Body conversion uses one uniform20/101 reduction, source reference[623,439], whole1254x1254 crop, alpha16bbox[144,432,1110,918]. Measured floor vertices[628,433],[1109,669],[628,917],[144,669] agree with native projection within the original2pixel measurement tolerance. Actual native bounds[33,62,225,159] fit the existing nine cell canvases; no source pixels were warped, painted, alpha-repaired or clipped.
- Coupling uses one uniform1/64 reduction, measured original bbox[308,422,987,885], source mounting reference[614,646] and native contact[208,120]. Its native bounds[203,116,214,123] stay inside approved envelope[200,112,218,127]. It overlaps11original platform pixels and leaves49,109other body-canvas pixels unchanged. The real reserved approach[224,128] stays transparent.
- Both active Python files pass AST, owning-plan headers, class/function and every argument's documentation checks. Focused tests cover live native geometry, pixel/cell fit, approach, signed drift, source ancestry, foreign roles, crop bounds and exact original-part assembly.
- All10previous source families,163previous candidate records,147other public rows and the exact GRF remain unchanged under `logs/space-core-protected-boundary.json`, SHA256 **dfe25b0c8bb37ed968ebf3ac9dd904a229c0f96db974211446f4eecb9cf45602**. Current catalogue has11families/171candidates. No packing offset/hash or runtime activation changed.
- Public Core remains planned/pathnull with authoring status `base_geometry_verified_cut_state_matrix_pending`; full coverage/runtimefalse. The Core source family has `base_selection_review_complete=true`, exact `base_source_review`, body/coupling approvals, and full coverage/runtimefalse.

## Selected immutable originals and exact prompts

Both were generated with the built-in imagegen tool. Preserve the raw originals and paired exact prompt/input receipts:

| Source | Original and SHA256 | Exact prompt/provenance |
| --- | --- | --- |
| Body | [orbital-core-body-v1.png](../../../../media/multilayer/source/space/raw/orbital-core-body-v1.png), **9d324da4b0b160a3e0ebd6aeb47660f6fcd2170d3f56aebf776093f8dca7d4d4** | [Body receipt](../../../../media/multilayer/source/space/orbital-core-body-v1.json) |
| Coupling | [orbital-core-coupling-v1.png](../../../../media/multilayer/source/space/raw/orbital-core-coupling-v1.png), **00bfa7ef74e68e740de81f99f81652f01eee76f8d57585436f0798502d2f2073** | [Coupling receipt](../../../../media/multilayer/source/space/orbital-core-coupling-v1.json) |

Six earlier whole-model candidates `orbital-core-complete-v1` through`v6` remain explicitly rejected in the catalogue, with raw images and exact receipts. They repeatedly misplaced the mouth or steepened the projection. Do not revive them or re-run that failed edit chain. The successful body uses a generator-facing shallow guide, explicitly distinct from the native contract; only its measured actual output was accepted. The final coupling is a genuine separate original placed through ordinary component assembly. The [earlier source notes](r2mlsp-z-z-z-g-core-base-source-notes.md) retain the detailed generation history; their pending/live statements are superseded by this checkpoint.

## Native geometry to preserve at70.2

`GetSpaceConstructionDefinitions` in `multilayer_space_content.cpp` defines Core3x3. `SpaceStructureState` has fixed x/y and no orientation. `GetSpaceStructureApproach` returns(x,y+height); Core has one approach/service slot. `SpaceWorldPoint` in `asteroid_gui.cpp` projects Q16 positions through the existing Space layer to tile centres. `RemapCoords` gives native x-step[-32,+16], y-step[+32,+16]. The existing footprint drawing subtracts half a tile before its per-cell draw.

Whole-source floor corner[128,64], other corners[224,112],[128,160],[32,112]. Native cell image origins are[97+32*(y-x),31+16*(x+y)] for x-fast/y-slow cells0..8. The approach centre is[224,128]; the edge coupling is[208,120]. These are source/rendering coordinates, never saved gameplay positions. `AreOrbitalModuleFootprintsAdjacent` owns orthogonal shared-edge adjacency; cosmetic fittings do not create a connector graph.

`native_body` and `coupling_source` validate exact original ancestry/registration; `native_base` assembles their pixels. `review_base` publishes this bounded whole-model checkpoint. Use those exact pixels for70.2. Partition losslessly into real native cells and decide genuine foreground ownership near the one coupling; do not duplicate the whole base into each tile or invent a contained craft. If a tile has no foreground hardware, represent that absence honestly in the source contract. Functional construction/blocked/operating imagery remains70.3 and must consume original native inputs/work/shared utility, without an art-local clock.

Use explicitUTF8, repository logs and focused child steps below110s. Test waits>=50000ms, individual waits<=60000ms. No subagents or Git writes. New auxiliary Markdown stays in this tracked archive with next prefix `r2mlsp-z-z-z-i-`; source/test/JSON filenames never start r2.
