# Multilayer Plans 1-5 - source-art pass, 2026-10-06

This pass reviewed Plans 1-5 against their current consumers and generated a
bounded first set of missing M1 artwork with the built-in image-generation tool.
It delivers **14 immutable original PNGs, 17 DOS-palette atlases and 247 staged
poses/previews**. It does not complete any feature plan or the M1 art gate.
Plan 6, its gameplay work and all live content packages were left alone.

The [compact native-size preview](../../../../../mlm/reviews/multilayer-art/authoring/import-20261009/images/summary-preview.png) shows the
selected state silhouettes. The [full gallery](../../../../../mlm/reviews/multilayer-art/authoring/import-20261009/images/native-review.png)
shows every staged pose at 4x nearest-neighbour scale. These are source-art
reviews, not screenshots or in-game acceptance evidence.

**Artifact-layout migration (2026-10-09):** this plan consumes the parent [Development artifact layout](../../../../zz-reach2/upgrades/2026-10/r2mlvs-multilayer-vehicles-and-structures.md#development-artifact-layout), implemented by [R2FC](../../../../zz-reach2/upgrades/2026-10/r2fc-file-cleanup.md). It supersedes earlier docs asset/review/helper placement and fixed runtime-config destinations below. New native homes use `mlm/runtime/`, generated feature reviews use `mlm/reviews/`, logs remain under `logs/`, and reusable code/curated inputs keep their code/test owners. Paths and focused native/cold verification are complete; R2FC records the results and retained-input identities. Repository-relative executable commands run from the checkout root through their context-aware wrappers. Historical pass records remain dated evidence and are not new acceptance claims. No gameplay task is completed by this migration and the separate post-demo fixture-pruning gate remains.

## Review of the executed plans

| Plan | Current graphical gap | Sources supplied in this pass | Remaining artwork/integration |
| --- | --- | --- | --- |
| [1 - Content](../../../../zz-reach2/upgrades/2026-10/r2mlvs1-gameplay-contracts-and-content.md) | All 19 added cargos still use the same base Goods icon in `media/multilayer/build_content.py`. Plan 1 freezes definitions; Plan 13 owns original icons. | Original Earth and Rock icons, converted to 20x20. | The other 17 cargo icons; custom cargo-sprite packaging and identity-compatible loading. |
| [2 - Geology](../../../../zz-reach2/upgrades/2026-10/r2mlvs2-geology-and-excavation.md) | Task 16.1 is still pending. `DrawTile_Clear` chooses existing layer floor variants without distinct geology-state art. | Underground and Deep solid/cut/excavating/excavated bases, three texture variants each, converted to 64x31. | Neighbor-aware edge/corner masks, ten survey identities, job footprint/progress displays and actual renderer bindings. |
| [3 - Portals](../../../../zz-reach2/upgrades/2026-10/r2mlvs3-sealed-portals-and-deep-rail.md) | Task 13.1 still lacks Sealed/Commissioning/Connected sprites. Existing housing drawing does not select work-state art. | Standard and Deep portal-state candidates with solid barricade, narrow work aperture and open railway mouth. | Genuine far-facing NE/NW views, state-to-contract mapping, per-end/per-tile back/front cuts, animation and runtime occlusion. |
| [4 - TBMs](../../../../zz-reach2/upgrades/2026-10/r2mlvs4-tunnel-boring-machines.md) | Task 16.2 is pending. `InitialiseTBMEngineDefinitions` selects base train image index 0 for the two cutters. | Distinct yellow Standard and reinforced red/graphite Deep cutters; eight actual nose headings; idle/moving/working/blocked first poses; available purchase previews. | Animation frames 1-3, separate support-part/seam art, reversing/service requirements, sprite/sound binding and real gameplay review. |
| [5 - Spoil](../../../../zz-reach2/upgrades/2026-10/r2mlvs5-spoil-wagons-and-haulage.md) | Hoppers use base image index 13. Exchange-bay art is absent. Minimal Surface receivers explicitly use original coal/iron-mine placeholders. | Earth and Rock hoppers in empty/partial/full/service/blocked first poses; a separate spoil-hauler locomotive; available purchase previews; exchange-bay construction/ready/working/blocked layout candidates. | Remaining animation frames, locked purchase poses, exchange per-cell front/back decomposition, Earth receiver/Rock crusher artwork (Plan 13 Groups 37-38), runtime integration. |

Plan 5's minimal spoil receivers belong to its M1 loop even though the shared
manifest assigns their broader industry art to consumer 6. They were not edited
or reauthored here. No mine/factory sources or Plan 6 files were changed.

## Retained originals and deterministic derivatives

[Generation record](../../../../../media/multilayer/source/m1/generation-record.json) retains the exact final
prompt set, reference paths, original SHA256 hashes, cell grids and measured
source rectangles. Original PNGs live under `media/multilayer/source/m1/raw/`; no original was
painted over, recolored in a script or replaced after generation. Deep material
variants and missing TBM rear views were edited by the built-in image tool.

[Native atlas manifest](../../../../../logs/proofs/mlm/native-art/derived/atlas-manifest.json)
records every converted pose's source cell, state/view, frame, native canvas,
anchor and output hash. The actual indexed PNGs live alongside it under
`media/multilayer/sprites/m1/`. The [public sprite manifest](../../../../../media/multilayer/contracts/multilayer_sprite_manifest.json)
now points to these sources through `source` and `authoring` records. Exact
required sprite counts and planned offsets are unchanged; packed payload hashes
remain null and `runtime_activated` remains false.

The initial TBM sheets repeated front views instead of delivering rear headings.
Separate N/NE/NW sheets corrected that. Native atlas columns were mapped by the
visible cutter direction, rather than trusting the generated column order.
The Earth hopper's symmetric NE/SW poses share one correctly angled source view;
no view was manufactured by mirroring or rotating another drawing.

Vehicle atlases contain **frame 0 only**. Missing animation frames are not filled
with duplicate pixels. Portal and exchange atlases are explicitly named
`*-candidate.png`: they contain whole structures/layouts, not the contract's
per-tile front/back pieces. Portal sources repeat near-facing approaches and
therefore are not claimed as four accepted engine directions. Cargo icons have
complete source coverage; all other source families remain partial.

The converter only measures/crops source cells, resamples them to native-size
canvases and quantizes them with the existing DOS palette and forbidden-index
policy. PNG index 0 is transparent. It does not allocate sprite IDs, activate the
reserved Action5 extension, alter a GRF digest or change simulation behavior.
The existing `map_layers.png` and offsets 0-65 remain byte-for-byte unchanged.

## Rebuild and checks

From the repository root:

```powershell
python media/multilayer/convert_m1_art.py
python media/multilayer/convert_m1_art.py --check
python src/tests/multilayer_sprite_contract.py
git diff --check
```

[Converter](../../../../../media/multilayer/convert_m1_art.py) refuses changed original
hashes, unexpected sprite-cell counts, empty converted poses, wrong dimensions,
incorrect DOS palettes, reserved animation/company indices, incorrect alpha
index, out-of-atlas boxes and mismatched output hashes. Its build also refreshes
the public manifest's source references without changing required coverage.
After intentionally regenerating the M0 contract with its `--generate` option,
rerun this converter to restore those source references.

The conversion and existing contract fixture passed, as did `git diff --check`.
A repeat conversion produced byte-identical hashes for all 22 derived/metadata
files. No executable rebuild, live gameplay art
gallery, multiplayer run or human final-art acceptance was claimed or needed
for this source-only delivery. Feature checkboxes remain unchanged.
