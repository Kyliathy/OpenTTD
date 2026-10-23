> **Historical donor snapshot — not an execution plan.** OpenTTD3 `75ea75718f`, imported2026-10-23.
> Original: `docs/zz-reach2/upgrades/2026-10/assets/generation-prompts.md`; exact source SHA256 `ad5fb15fb3f64b1450d3bb0747c79f174914fdc49a6bef6327ce02c368e4318b`.
> Current integration: [M2O23-1](../../../../../../../../zz-reach2/upgrades/2026-10/m2o23-1-come-space.md); storage and evidence roles: [archi-historical-debug.md](../../../../../../../../zz-reach2/architecture/archi-historical-debug.md). Original completion claims, instructions and human-review limits below remain dated; they grant no new execution, spending, publication or deletion authority.

# Layer graphics generation record

Generated with the built-in `image_gen.imagegen` tool. Raw outputs are copied into
`raw/`; sprite dimensions and DOS palette conversion are applied by
`../tools/sprite_quantize.py`. Existing raw versions are preserved.

## Space toolbar icon, version 2

Target: contract offset 2, 20 x 20 pixels. No input images.

Prompt:

> Create one game toolbar icon: a large golden ringed planet and three bright
> stars on a small dark navy sky patch, with a pure magenta #FF00FF background
> outside the patch. Transport Tycoon Deluxe DOS pixel art, bold simple shapes,
> crisp hard pixel edges, limited colours, top-left lighting, no antialiasing,
> no text, no border, no grid, no guide marks. The planet and its ring must fill
> most of the icon and remain immediately readable after reduction to 20 x 20
> pixels. Use a square canvas; make the icon fill the central 90 percent.

Selected output: `raw/layer_icon_space_v2.png`.

## Road elevator toolbar icon and matching cursor, version 2

Targets: offsets 3 and 5, 20 x 20 and 32 x 32. No input images. The same
generated artwork supplies both sizes so placement matches the toolbar.

Prompt:

> Create a game toolbar icon of a square industrial road elevator cabin, grey steel with a broad dark doorway and a large bright yellow downward arrow centered above it. Transport Tycoon Deluxe DOS pixel art. Bold simple silhouettes, hard pixel edges, limited earthy colours, light from top left. The cabin and arrow fill 90 percent of a square canvas. Pure magenta #FF00FF outside the icon. No text, no frame, no mouse pointer, no ground shadow, no tiny details. Must be instantly legible at both 20 x 20 and 32 x 32 pixels.

Selected output: `raw/build_icon_road_elevator_v2.png`.
