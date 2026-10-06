# R2MLM6 - Layer Graphics Assets (AI-Generated)

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
| [r2mlm-multi-layered-map.md](r2mlm-multi-layered-map.md) | 106-108 | `### AD-10 Graphics`: sprites go into OpenTTD's extra GRF as Action 5 type `0x1C`, mapped to `SPR_MAP_LAYER_BASE`, generated with `GenerateImage` and quantised by a repeatable script; consumers keep stand-ins until the constants exist. |
| [r2mlm-multi-layered-map.md](r2mlm-multi-layered-map.md) | 141-170 | `## Engineering Conventions` and `## Build and Test Commands`: no `r2` prefix on created files, helper scripts in `docs/zz-reach2/upgrades/2026-10/tools/`, no file moves or deletes (copy generated images, never move them). |
| [r2mlm6-layer-graphics-assets.md](r2mlm6-layer-graphics-assets.md) | 56-76 | `## Sprite Contract` of this plan: the 44 sprites, their order, sizes, offsets and constant names. Every group produces or consumes exactly this list; the order is the Action 5 order and must not change once constants are published. |
| [src/table/sprites.h](../../../../src/table/sprites.h) | 370-381 | The bridge-deck block and `SPR_NEWGRFS_BASE`: the newest Action 5 block, which the new block follows, and the base that must move behind it. |
| [docs/zz-reach2/architecture/basesets/archi-basesets.md](../../architecture/basesets/archi-basesets.md) | 31-58 | `### Graphics`: the `extra` file is a NewGRF loaded at `SPR_OPENTTD_BASE`; sprites a base set does not supply are counted from `OPENTTD.GRF`. Explains why every new sprite must exist in `openttd.grf`. |

## Outcome and Boundaries

- `media/baseset/openttd/map_layers.png` and `media/baseset/openttd/map_layers.nfo` holding the 44 sprites of `## Sprite Contract`, included from `openttd.nfo`, regenerated into `media/baseset/openttd.grf` and its `.hash`.
- Action 5 type `0x1C` "Map layer graphics" in `_action5_types`, `SPR_MAP_LAYER_BASE` and named constants in `src/table/sprites.h`, `SPR_NEWGRFS_BASE` moved behind the block.
- Repeatable tooling in `docs/zz-reach2/upgrades/2026-10/tools/`: `make_sprite_template.py` and `sprite_quantize.py`; raw AI outputs and prompts kept in `docs/zz-reach2/upgrades/2026-10/assets/`.
- The stand-ins in r2mlm1, r2mlm5, r2mlm8 and r2mlm9 are switched to the constants by those plans' swap tasks, or by Group 7 when those plans are already done.

This plan does not change gameplay code beyond constant definitions and the Action 5 table. It can run any time after r2mlm1. Groups 3 and 4 need a harness that can generate images; see their Area block.

## Sprite Contract

All sprites are 8bpp, DOS palette, index 0 transparent, zoom `normal`. Directions are the door side of the structure (`DiagDirection` order NE, SE, SW, NW). "Back" draws first (walls, floor, shaft), "Front" draws over vehicles (arch, door frame).

| Offset | Constant | Size (w x h) | rel x, rel y | Content |
| --- | --- | --- | --- | --- |
| 0 | `SPR_IMG_MAP_LAYER_SURFACE` | 20 x 20 | 0, 0 | Toolbar icon: green hill with a small tree under a blue sky |
| 1 | `SPR_IMG_MAP_LAYER_UNDERGROUND` | 20 x 20 | 0, 0 | Toolbar icon: brown cut-away earth with a lit tunnel mouth |
| 2 | `SPR_IMG_MAP_LAYER_SPACE` | 20 x 20 | 0, 0 | Toolbar icon: dark sky with a ringed planet and stars |
| 3 | `SPR_IMG_BUILD_ROAD_ELEVATOR` | 20 x 20 | 0, 0 | Road toolbar icon: elevator cabin with a down arrow |
| 4 | `SPR_IMG_BUILD_RAIL_LAYER_PORTAL` | 20 x 20 | 0, 0 | Rail toolbar icon: track ramp entering a dark portal |
| 5 | `SPR_CURSOR_ROAD_ELEVATOR` | 32 x 32 | 0, 0 | Placement cursor matching offset 3 |
| 6 | `SPR_CURSOR_RAIL_LAYER_PORTAL` | 32 x 32 | 0, 0 | Placement cursor matching offset 4 |
| 7-9 | `SPR_MAP_LAYER_UNDERGROUND_FLOOR` (+0..2) | 64 x 31 | -31, 0 | Flat bedrock tile, three variants |
| 10-11 | `SPR_MAP_LAYER_SPACE_FLOOR` (+0..1) | 64 x 31 | -31, 0 | Flat vacuum tile with sparse stars, two variants |
| 12-19 | `SPR_MAP_LAYER_ROAD_ELEVATOR_SURFACE` (+dir * 2 + part) | 64 x 64 back, 64 x 64 front | -31, -33 | Surface elevator house over a road tile, door on `dir` |
| 20-27 | `SPR_MAP_LAYER_ROAD_ELEVATOR_UNDERGROUND` (+dir * 2 + part) | 64 x 64, 64 x 64 | -31, -33 | Underground elevator shaft housing, door on `dir` |
| 28-35 | `SPR_MAP_LAYER_RAIL_PORTAL_SURFACE` (+dir * 2 + part) | 64 x 48, 64 x 48 | -31, -17 | Surface descent portal: track ramp into a tunnel mouth, mouth on `dir` |
| 36-43 | `SPR_MAP_LAYER_RAIL_PORTAL_UNDERGROUND` (+dir * 2 + part) | 64 x 48, 64 x 48 | -31, -17 | Underground emergence portal: lit tunnel mouth in rock, mouth on `dir` |

`part` is 0 for back and 1 for front. The original contract had `MAP_LAYER_SPRITE_COUNT = 44`; the live count is now 66. Sizes are upper bounds; the quantise script crops transparent margins and adjusts `rel x` / `rel y` so the anchor stays at the tile's north corner. r2mlm13 extends this contract to offset 65 and extends the tools of Groups 2 and 5; see its `## Sprite Contract Extension` and its Group 2.

## Implementation Tasks

### Area Mandatory Reading - Toolchain

_Applies to Group 1._

Entry gate. Groups 6 and 7 need GRFCodec, NFORenum and GRFID; Groups 3 to 5 need Python with Pillow. Verify with `grfcodec -v`, `nforenum -v`, `grfid -v` and `python -c "import PIL"`. If the GRF tools cannot be installed, stop Groups 6 and 7 and report to the user; Groups 2 to 5 may continue and leave their outputs staged. Never hand-edit `media/baseset/openttd.grf`, never commit a GRF built by any other tool, and never load the sprites from a separate user NewGRF as a substitute.

| File name | Line Ranges | Description |
| --- | --- | --- |
| [cmake/scripts/CreateGRF.cmake](../../../../cmake/scripts/CreateGRF.cmake) | 1-60 | How the GRF is built: inline `#include` lines of `openttd.nfo`, run NFORenum, then GRFCodec `-n -s -e -g2 -p1`, then GRFID for the hash. Requires `NFORENUM_EXECUTABLE`, `GRFCODEC_EXECUTABLE`, `GRFID_EXECUTABLE`. |
| [media/baseset/openttd/CMakeLists.txt](../../../../media/baseset/openttd/CMakeLists.txt) | 1-65 | `create_grf_command` with the NFO and PNG source lists; regeneration only happens when GRFCodec is found. |

Entry gate, 2026-10-02: Pillow is not installed and `D:\Codez\Tools\grfcodec` is absent. Groups 6 and 7 stay stopped. Groups 2 and 5 were written with the standard library so they can run before Pillow is installed.

Entry gate, 2026-10-03: Pillow 12.3.0 and GRFCodec / NFORenum / GRFID 6.2.0 are installed (tasks 1.1 and 1.2). The GRF tools are not on `PATH`, so a plain configure keeps using the committed `openttd.grf`; task 1.3 passes them explicitly.

### Group 1 - Toolchain

{{SIMPLE}}
- [x] 1.1 Install Pillow for the user (`python -m pip install --user pillow`) and record the version.
  - Pillow 12.3.0 for Python 3.10.0 (user site-packages).
- [x] 1.2 Download the latest Windows release of GRFCodec (which ships NFORenum and GRFID) from the OpenTTD `grfcodec` GitHub releases into `D:\Codez\Tools\grfcodec` (outside the repository) and record the version.
  - GRFCodec 6.2.0 (`grfcodec-6.2.0-windows-win64.zip`, SHA-256 `A60A402BBFC37CE326CB89B9017AF58ADB8CAB8F99E5B0201F016006873D0553`). The binaries `grfcodec.exe`, `nforenum.exe`, `grfid.exe` and `grfstrip.exe` are in `D:\Codez\Tools\grfcodec\grfcodec-6.2.0-windows-win64\bin`. Each one reports version 6.2.0 with `-v`.
- [x] 1.3 Re-run the configure command with `-DGRFCODEC_EXECUTABLE`, `-DNFORENUM_EXECUTABLE` and `-DGRFID_EXECUTABLE` pointing at those binaries; confirm CMake reports GRFCodec as found. Verified in `build/configure-graphics.log`, 2026-10-03.
- [x] 1.4 Build once without any sprite change; record whether the regenerated `openttd.grf` and `openttd.grf.hash` are byte-identical to the committed ones (a different tool version may change bytes; note it, do not fix it).
  - `build/build-grf-baseline.log`: Debug targets succeeded. `openttd.grf` is byte-identical (SHA-256 `b96cdc70ccabe75479aecb30b46d593c6189b7cbe04cd52fa726963c54c13b96`). The hash file has CRLF instead of the committed LF; its stripped contents match. The Windows NFORenum 6.2.0 binary crashed with an uninitialised data cache; `CreateGRF.cmake` now materialises that cache inside the build folder before linting, retaining the normal NFORenum/GRFCodec/GRFID pipeline.

### Area Mandatory Reading - Art specification and templates

_Applies to Group 2._

| File name | Line Ranges | Description |
| --- | --- | --- |
| [media/baseset/openttd/tunnel_portals.nfo](../../../../media/baseset/openttd/tunnel_portals.nfo) | 7-27 | Real-sprite lines for tunnel portal overlays (sizes 35 to 40 wide, offsets per direction): the style and offset reference for rail portals. |
| [media/baseset/openttd/openttdgui.nfo](../../../../media/baseset/openttd/openttdgui.nfo) | 178-191 | 20 x 20 toolbar icons and 32 x 32 cursor sprites with `0 0` offsets: the size reference for offsets 0 to 6. |
| [media/baseset/openttd/roadstops.nfo](../../../../media/baseset/openttd/roadstops.nfo) | 6-15 | Road stop sprites with back and front parts: the reference for splitting elevator sprites. |

### Group 2 - Art specification and templates

{{MEDIUM}}
- [x] 2.1 Write `docs/zz-reach2/upgrades/2026-10/tools/extract_palette.py` that reads the PLTE chunk of `media/baseset/openttd/tunnel_portals.png` and writes the 256-colour DOS palette to `docs/zz-reach2/upgrades/2026-10/assets/dos_palette.txt`.
- [x] 2.2 Record in that file the indices the quantiser must not use: 0 (transparent, except as transparency), the animated ranges used for water and lights, and the company-colour range; cite where each range was found.
- [x] 2.3 Write `docs/zz-reach2/upgrades/2026-10/tools/make_sprite_template.py` producing one guide PNG per contract row group (icons, ground, elevator, portal) at 4x scale: the isometric tile diamond, the anchor point and the maximum box, on a magenta (`#FF00FF`) background.
- [x] 2.4 Copy the relevant existing sprites (tunnel portals, road stops, a road depot from the base set) at 4x into `docs/zz-reach2/upgrades/2026-10/assets/reference/` as style references for the AI prompts.
- [x] 2.5 Write the prompt template under this task: subject, isometric 2:1 pixel art in the style of Transport Tycoon Deluxe, light from the top left, limited earthy palette, pure magenta background, no text, the guide PNG as a reference image, and the target box.

Prompt template: "Isometric 2:1 pixel art in the style of Transport Tycoon Deluxe. Light from the top left. Limited earthy DOS palette. Pure magenta #FF00FF background, no text, no frame, no anti-aliasing. Match the guide image: stay inside the box and put the tile's north corner on the red anchor. Subject: <subject>. Target box: <width> x <height>." Icons and cursors use the same sentence without the isometric-tile clause, and name the 20 x 20 or 32 x 32 box.

### Area Mandatory Reading - AI generation

_Applies to Groups 3-4._

**Harness.** These groups need a harness that can generate images from a prompt and reference images. Every image so far came from Cursor's `GenerateImage` tool. The Claude Code sessions used so far have no such tool, so run Groups 3 and 4 in a harness that has one, and stop them anywhere else. The other groups need no such tool, though Groups 6 and 7 wait for the finished images.

| File name | Line Ranges | Description |
| --- | --- | --- |
| [r2mlm6-layer-graphics-assets.md](r2mlm6-layer-graphics-assets.md) | 56-76 | `## Sprite Contract`: the sprite being generated, its box, anchor and direction convention. |
| [docs/zz-reach2/upgrades/2026-10/assets/reference/](assets/reference/) | Live definitions; no fixed range | Produced by task 2.4. Passed to `GenerateImage` through `reference_image_paths` so style and scale match the game. |

### Group 3 - AI generation: icons and ground

2026-10-03 replacement candidates and exact prompts: [assets/generation-prompts.md](assets/generation-prompts.md). The Space and elevator icons were regenerated with the built-in `image_gen.imagegen` tool. Both cursors now use their corresponding toolbar artwork at 32 x 32; the mistaken mouse-pointer image is preserved but no longer consumed. Offline palette preview: [assets/icon_preview_v2.png](assets/icon_preview_v2.png). In-game capture checks remain pending.

{{COMPLEX}}
- [x] 3.1 Generate the three layer toolbar icons (contract offsets 0 to 2) with `GenerateImage`, one call per icon at `1:1`, using the guide and reference images; copy each result into `docs/zz-reach2/upgrades/2026-10/assets/raw/` with a contract-based name.
- [x] 3.2 Generate the two build-toolbar icons and two cursors (offsets 3 to 6) in the same way.
- [x] 3.3 Generate the bedrock floor (offsets 7 to 9) as one 3-variant sheet on the tile-diamond guide.
- [x] 3.4 Generate the vacuum floor (offsets 10 to 11) as one 2-variant sheet.
- [x] 3.5 Review every image at game size (downscale preview with the quantise script in dry-run mode once Group 5 exists, or a plain Pillow resize now); regenerate any icon that is unreadable at 20 x 20.

  Offline review completed 2026-10-04 against the actual quantised manifest: the selected Space/elevator `_v2` icons resolve the first-pass readability problems, and both cursors use the tool picture. The four-layer 1x/4x comparison is `assets/reviews/r2mlm13-c-compare-44.png`; the retained-original-set board is `assets/reviews/r2mlm13-d-replaced.png`. Original floors were viewed at contract size and 4x. This closes the downscale review, not the separate in-game art checks.
- [x] 3.6 Record every prompt, the reference images used and the chosen output file under this task, so the art can be regenerated.

Raw files are in `docs/zz-reach2/upgrades/2026-10/assets/raw/`. GenerateImage wrote JPEG; each file was copied to PNG there. The shared prompt is the Group 2 template. Chosen files:

| Offset | File | Reference |
| --- | --- | --- |
| 0 | `layer_icon_surface.png` | none (the guide's box was being copied into the picture) |
| 1 | `layer_icon_underground.png` | `guide_icon_20.png`, `reference/tunnel_portal.png` |
| 2 | `layer_icon_space.png` | `guide_icon_20.png` |
| 3 | `build_icon_road_elevator.png` | none |
| 4 | `build_icon_rail_portal.png` | `reference/tunnel_portal.png` |
| 5 | `cursor_road_elevator.png` | none |
| 6 | `cursor_rail_portal.png` | `reference/tunnel_portal.png` |
| 7-9 | `underground_floor_sheet.png` | `guide_ground.png` |
| 10-11 | `space_floor_sheet.png` | `guide_ground.png` |

A first-pass 4x quantise of the icons is `docs/zz-reach2/upgrades/2026-10/assets/icon_preview.png`. At that stage the rail cursor came back as a mouse pointer, and the Space and elevator icons were unreadable at 20 x 20. The selected replacements in the Group 3 introduction supersede those first-pass files; task 3.5 now records the completed offline review.

### Group 4 - AI generation: elevators and portals

Closure, 2026-10-04: task 4.7's complete native-tile/vehicle-gap visual operator check is skipped under the goal's allowance. All sixteen directional pairs are generated and integrated. R2MLM13 Groups 9B/9C repair their fronts and record actual supplied-save captures, but the save lacks some elevator orientations, so no complete native-composite matrix pass is inferred. Raw attempts and chosen prompts remain retained.

2026-10-03: all sixteen directional pairs are generated as aligned two-panel assets, supplying 32 separate contract sprites. Selected files are `assets/raw/<family>_<direction>_pair_v4.png`, where families are `elevator_surface`, `elevator_underground`, `rail_portal_surface` and `rail_portal_underground`, and directions are `ne`, `se`, `sw`, `nw`. The left/right source panels share their canvas and anchor. Exact prompts and guide paths: [assets/directional-prompts-v4.md](assets/directional-prompts-v4.md); guides are reproduced with `tools/make_portal_guides.py`. Each direction was generated independently to preserve top-left lighting. Earlier raw attempts are retained. Task 4.7 is pending the representative save and visual checks.

{{COMPLEX}}
- [x] 4.1 Generate the surface road elevator for the NE door direction as two images: back (floor, side walls, shaft roof) and front (door frame and front pillars only), both on the same guide so they align.
- [x] 4.2 Derive SE, SW and NW variants: mirror horizontally where the isometric projection allows (NE and NW, SE and SW), and generate the other pair; check lighting stays top-left.
- [x] 4.3 Generate the underground elevator housing (back and front) for all four directions the same way.
- [x] 4.4 Generate the surface rail descent portal (track ramp into a tunnel mouth) for NE, back and front, using `tunnel_portals.png` as the scale reference.
- [x] 4.5 Derive the other three surface portal directions as in task 4.2.
- [x] 4.6 Generate the underground emergence portal (lit mouth in rock) for all four directions.
- [s] 4.7 Composite each back and front pair over a road tile and a rail tile at game size and confirm a vehicle-sized gap exists between them; regenerate pairs that overlap wrongly.
- [x] 4.8 Record prompts and chosen files under this task.

Early draft, before the sixteen `_pair_v4` sets recorded above: only the northeast Surface pair was staged. Both used the Group 2 template plus the named subject.

| File | Reference |
| --- | --- |
| `elevator_surface_ne_back.png` | `guide_elevator.png`, `reference/depot_building.png` |
| `elevator_surface_ne_front.png` | `guide_elevator.png`, `reference/road_stop.png` |
| `rail_portal_surface_ne_back.png` | `guide_portal.png`, `reference/tunnel_portal.png` |
| `rail_portal_surface_ne_front.png` | `guide_portal.png`, `reference/tunnel_portal.png` |

Those early back images contained the front door or the whole portal, so that draft split was unusable and the other directions were not yet generated. The sixteen aligned `_pair_v4` sets in the current Group 4 record supersede this draft. Task 4.7's complete vehicle-gap operator check is explicitly skipped in the closure note above, with the supplied-save repairs/captures recorded separately.

### Area Mandatory Reading - Quantisation and GRF integration

_Applies to Groups 5-6._

| File name | Line Ranges | Description |
| --- | --- | --- |
| [media/baseset/openttd/bridge_decks.nfo](../../../../media/baseset/openttd/bridge_decks.nfo) | 1-29 | The newest sprite block file: licence header, `0C` comment, `05 1B <count>` Action 5 and real-sprite lines. `map_layers.nfo` copies this shape with type `1C`. |
| [media/baseset/openttd/openttd.nfo](../../../../media/baseset/openttd/openttd.nfo) | 80-103 | The `#include` list; `map_layers.nfo` is appended after `bridge_decks.nfo`. |
| [src/newgrf/newgrf_act5.cpp](../../../../src/newgrf/newgrf_act5.cpp) | 51-82 | `_action5_types`: one row per Action 5 type with block type, base sprite, minimum and maximum counts and a name; the `0x1C` row is appended. |
| [src/gfxinit.cpp](../../../../src/gfxinit.cpp) | 202-227 | Loading of the extra GRF at `SPR_OPENTTD_BASE` and the missing-sprite count against `SPR_NEWGRFS_BASE`; the new block must be covered by `openttd.grf`, or the warning appears. |

### Group 5 - Quantisation tool

2026-10-03: the manifest covers all 44 offsets exactly once. Pillow keys magenta before area filtering, preserves source alpha, and keeps shared margins for aligned pairs. `--write-nfo` reproduces the NFO; `--dry-run --preview` leaves the integrated sheet unchanged. The real sheet was generated twice with identical SHA-256 `0e67b6971a1cea1ee46cb8dac0f40fc2c1cd93135dfc52974c398db18a762d4f`.

{{MEDIUM}}
- [x] 5.1 Write `docs/zz-reach2/upgrades/2026-10/tools/sprite_quantize.py` with a Javadoc-style module docstring and per-function docstrings: key out magenta to index 0, downscale with area filtering to the contract size, quantise to the DOS palette excluding the forbidden indices, crop transparent margins and recompute `rel x` / `rel y`.
- [x] 5.2 Make the script take a manifest (a Python list in the script, not JSON) mapping contract offsets to raw files, and write `media/baseset/openttd/map_layers.png` as one sheet with a 2-pixel gap between sprites.
- [x] 5.3 Make the script emit the real-sprite lines (`-1 sprites/map_layers.png 8bpp x y w h relx rely normal`) in contract order to stdout.
- [x] 5.4 Add a `--preview` mode that writes a 4x enlarged sheet to `docs/zz-reach2/upgrades/2026-10/assets/preview.png` for review.
- [x] 5.5 Run the script twice and confirm the output PNG is byte-identical (determinism). `sprite_quantize.py --self-test` wrote two synthetic PNGs and they matched. The contract sheet is not written until `MANIFEST` lists all 44 raw sprites.

### Group 6 - GRF integration

2026-10-03: Debug builds both targets without compiler warnings (`build/build-layer-art-Debug.log`). NFORenum 6.2.0 does not know this fork's new type 0x1C, so the NFO scopes diagnostic 44 suppression to that Action 5; all real sprites are still linted. GRFCodec and GRFID regenerated the extra GRF. Source and build copies match, SHA-256 `c1e88812e85d96bdb411ef8db0fba2848c3d306a230ed1ded1e395c3bd6960b4` before the final header-only regeneration. In-game startup and alignment checks remain pending.

{{MEDIUM}}
- [x] 6.1 Create `media/baseset/openttd/map_layers.nfo` with the licence header, `-1 * 0 0C "Map layer graphics"`, `-1 * 3 05 1C \b 44 // MAP_LAYER_SPRITE_COUNT` and the emitted real-sprite lines.
- [x] 6.2 Append `#include "map_layers.nfo"` to `openttd.nfo` after `bridge_decks.nfo`, and add `map_layers.nfo` and `map_layers.png` to `media/baseset/openttd/CMakeLists.txt`.
- [x] 6.3 Append `/* 0x1C */ { Action5BlockType::AllowOffset, SPR_MAP_LAYER_BASE, 1, MAP_LAYER_SPRITE_COUNT, "Map layer graphics" },` to `_action5_types`.
- [x] 6.4 In `src/table/sprites.h`, add the `SPR_MAP_LAYER_BASE` block after the bridge decks with `MAP_LAYER_SPRITE_COUNT` and every named constant from the contract, each with a `///<` comment, and change `SPR_NEWGRFS_BASE` to follow it.
- [x] 6.5 Build; confirm `openttd.grf` and `openttd.grf.hash` are regenerated and that the build copies the GRF into `build/baseset/`. The source hash remains alongside the source GRF; the existing CMake pipeline does not copy `openttd.grf.hash` into the build folder.
- [x] 6.6 Start the game and confirm no missing-extra-graphics warning and no "unimplemented Action 5 type" message in the `grf` debug output (`-d grf=2`).

  2026-10-04: current Debug build loads `build/save/makinpr2.sav`. `build/makinpr2-review/game.log` reports offsets 0..65 of Action 5 type `0x1C` at SpriteID `0x188B`; no missing-extra-graphics dialog or unimplemented-type message. The unrelated original baseset message about loading shore sprites is expected fallback output.

### Area Mandatory Reading - Wiring and acceptance

_Applies to Group 7._

| File name | Line Ranges | Description |
| --- | --- | --- |
| [src/map_layer_gui.cpp](../../../../src/map_layer_gui.cpp) | Live definitions; no fixed range | Produced by r2mlm1 Group 2: `GetMapLayerIcon` returns stand-ins until task 7.1. |
| [src/clear_cmd.cpp](../../../../src/clear_cmd.cpp) | Live definitions; no fixed range | After r2mlm5 Group 5: `GetLayerGroundSprite` returns stand-ins until task 7.2. If r2mlm5 has not run yet, skip task 7.2; r2mlm5 task 5.3 picks the constant up. |
| [src/table/sprites.h](../../../../src/table/sprites.h) | Live definitions; no fixed range | Produced by task 6.4. The constant names are authoritative for every consumer plan. |

### Group 7 - Wiring and acceptance

Closure, 2026-10-04: operator tasks 7.3 and 7.4 (sprite aligner, every GUI scale and both blitters) are skipped under the goal's allowance. The integrated 66-sprite set loads in Debug with all Action 5 offsets and no missing-extra-sprite warning; R2MLM13 records the actual game captures and remaining gallery gaps. Quantiser anchors, original 44-row replay and GRF hashes are verified, but are not substitutes for the skipped UI/blitter matrix or human art approval.

{{SIMPLE}}
- [x] 7.1 Unless r2mlm5 task 8.1 already did it, switch `GetMapLayerIcon` to `SPR_IMG_MAP_LAYER_SURFACE`, `_UNDERGROUND` and `_SPACE`, and switch the toolbar sprite rows from the stand-in.
- [x] 7.2 Switch `GetLayerGroundSprite` to `SPR_MAP_LAYER_UNDERGROUND_FLOOR` and `SPR_MAP_LAYER_SPACE_FLOOR` with a `TileHash` variant pick (only if r2mlm5 Group 5 is done).
- [s] 7.3 Open the NewGRF sprite aligner (`-d grf` debug tools) on each new sprite and confirm offsets anchor at the tile's north corner; fix offsets in the manifest, not in the NFO by hand.
- [s] 7.4 Take screenshots of the toolbar at every GUI scale and of each ground layer, with the 8bpp and a 32bpp blitter, and link them under this task.
- [x] 7.5 Add a note line under the Group 7 heading of r2mlm8 and of r2mlm9 stating that the elevator and portal constants are available, so those art-swap groups are unblocked.

## Consult on Demand

| Trigger | File name | Start at | Expand only when | Consultation purpose |
| --- | --- | --- | --- | --- |
| NFORenum or GRFCodec rejects `map_layers.nfo` | [docs/obg_format.txt](../../../../docs/obg_format.txt) | The section on the extra GRF | The error is about the GRF identity rather than syntax | Confirm the system-GRF rules; NFO syntax itself follows the existing `bridge_decks.nfo`. |
| Upstream OpenTTD has assigned Action 5 type `0x1C` meanwhile | [src/newgrf/newgrf_act5.cpp](../../../../src/newgrf/newgrf_act5.cpp) | `_action5_types` | A merge from upstream shows a different `0x1C` row | Pick the next free type and update the contract, the NFO and this plan together. |
| Colours look wrong only with the Windows-palette base set | [docs/zz-reach2/architecture/basesets/archi-basesets.md](../../architecture/basesets/archi-basesets.md) | `### Graphics` (lines 31-58) | The DOS-palette flag in `openttd.nfo` seems ignored | Understand palette remapping of the extra GRF. |
| Sprites look blurry when zoomed in | [docs/zz-reach2/architecture/gui/archi-blitter.md](../../architecture/gui/archi-blitter.md) | `## Sprite cache` (lines 58-73) | The user asks for extra zoom levels | Learn how per-zoom sprites are encoded before adding 2x/4x variants. |
| An AI image keeps putting text or a frame in the picture | [r2mlm6-layer-graphics-assets.md](r2mlm6-layer-graphics-assets.md) | Task 2.5 prompt template | Three attempts fail | Tighten the prompt; after three more failures ask the user to approve hand-drawn pixel edits. |

## Planning Provenance

This table records what shaped the plan. It is not an execution reading queue, it must not be preloaded after context compaction, it is non-transitive, and its line ranges record planning use rather than runtime authority.

| File name | Sections or Line Ranges used while planning | Planning contribution |
| --- | --- | --- |
| [media/baseset/openttd/openttd.nfo](../../../../media/baseset/openttd/openttd.nfo) | 1-103 | DOS-palette declaration and include list; decided a new include file rather than editing an existing one. |
| [src/table/sprites.h](../../../../src/table/sprites.h) | 60-192, 325-381 | Sprite id blocks and the bridge-deck precedent for appending a block. |
| [src/newgrf/newgrf_act5.cpp](../../../../src/newgrf/newgrf_act5.cpp) | 51-140 | Action 5 table and offset handling. |
| [media/baseset/openttd/openttdgui.nfo](../../../../media/baseset/openttd/openttdgui.nfo) | 1-199 | Icon and cursor sizes. |
| `GenerateImage` tool schema (Cursor) | `description`, `filename`, `reference_image_paths`, `aspect_ratio` | Images are saved by the tool to its own location, so tasks copy results into the assets folder. |
| Local environment check | Python 3.10 present, Pillow and GRF tools absent | Added the toolchain group and the entry gate. |

## Completion Conditions

Closed 2026-10-04 under the goal's operator allowance, with explicit skips in Groups 4 and 7. Generated art, prompts, repeatable quantisation and GRF integration are delivered; R2MLM13 extends the live set to 66 and records the foreground repairs. The original 44-row replay remains byte-identical. Build/unit evidence is the final zero-warning builds, 111/111 cases in both configurations and 115/115 full release CTest cases. No human art approval or complete GUI-scale/blitter matrix pass is claimed.

- `map_layers.png` and `map_layers.nfo` contain all 44 contract sprites in contract order, and `openttd.grf` was regenerated from them by GRFCodec.
- `_action5_types` has the `0x1C` row and `src/table/sprites.h` defines every contract constant, with `SPR_NEWGRFS_BASE` after the block.
- The game starts without missing-sprite warnings, and the toolbar shows the three new layer icons.
- Every prompt and raw image is recorded, and the quantise script reproduces the sheet byte for byte.
