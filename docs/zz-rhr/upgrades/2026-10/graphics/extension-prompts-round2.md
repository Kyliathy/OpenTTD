# R2MLM13 resumed generation prompts

Generated with the built-in image tool on 2026-10-04. Raws are retained in `media/multilayer/source/legacy/raw/`; earlier attempts are kept. Human art review remains pending.

## deep_floor_sheet_v2.png

Repository output: `media/multilayer/source/legacy/raw/deep_floor_sheet_v2.png`.
References: `media/multilayer/source/legacy/raw/deep_floor_sheet.png`, `media/multilayer/guides/legacy/guide_ground.png`.

```text
Edit the first image into a correctly projected OpenTTD ground sprite sheet. Preserve its dark slate-blue basalt colour, fine pixel texture, three different subtle vein patterns, magenta backdrop, and three equally spaced tiles. CHANGE the three diamond silhouettes: compress each vertically so its WIDTH is slightly MORE THAN TWICE its HEIGHT, exact width:height 64:31. Image 2 is GEOMETRY ONLY: its extremely flat rhombus is the exact desired outline for EACH tile. Do not copy its black lines, frame or guide marks. All three diamonds must have this same flattened guide shape, with the left and right corners only half the diamond-height below its top. No thickness, no slab sides, no bevel or rim. Low contrast vein detail with uniform texture and tone right to the four edges, no bright vein ending on an edge that will produce a seam. Single wide landscape sheet, each tile within its own equal horizontal third, with generous pure #FF00FF magenta gaps and margins. No text or outlines. Every diamond is a flat surface viewed at the game angle, not a square rotated 45 degrees. The whole diamond including top point must fit in a box only 31/64 as high as it is wide.
```

## road_tunnel_surface_door_sw.png

Repository output: `media/multilayer/source/legacy/raw/road_tunnel_surface_door_sw.png`.
References: `media/multilayer/guides/legacy/reference/tunnel_portal.png`, `media/multilayer/source/legacy/raw/rail_portal_surface_ne_back.png`.

```text
Use case: stylized-concept. Asset: one whole OpenTTD road layer tunnel sprite, drawn in Transport Tycoon Deluxe coarse pixel art. Reference 1 sets the game's low grassy-mound scale and 2:1 projection; reference 2 supplies stone/road detail style only, DO NOT copy its tall mound or red cross. One structure on a single flat diamond ground tile, seen from the south. Top-left light, limited earthy DOS-like colours, no anti-aliasing. Plain solid #FF00FF magenta outside the subject. No labels, frame, grid, outline box, red cross or guides. A LOW grassy mound over a grey stone-framed tunnel with a grey paved road descending inside. Clearly a TUNNEL, not a depot or hangar. The whole tile is twice as wide as its projected ground height; total subject height is NO MORE THAN THREE QUARTERS of its width, so a 64x48 sprite box fits without cutting. No slab sides, no projecting walls at the front. The ground's south point is the lowest opaque point. Keep at least 15% blank margin around all sides. ONE picture, not a back/front split. The door faces the LOWER LEFT of the picture (SW). The road begins at the tile's lower-left edge, runs straight diagonally toward the upper right and disappears into the stone mouth in the BACK HALF of the tile. No walls beside the road in front of the opening; a clear wide approach remains outside. Low grassy mound behind and above the mouth. The interior is black. Keep the mound very low and broad.
```

## road_tunnel_surface_door_ne.png

Repository output: `media/multilayer/source/legacy/raw/road_tunnel_surface_door_ne.png`.
References: `media/multilayer/guides/legacy/reference/tunnel_portal.png`, `media/multilayer/source/legacy/raw/rail_portal_surface_ne_back.png`.

```text
Use case: stylized-concept. Asset: one whole OpenTTD road layer tunnel sprite, drawn in Transport Tycoon Deluxe coarse pixel art. Reference 1 sets the game's low grassy-mound scale and 2:1 projection; reference 2 supplies stone/road detail style only, DO NOT copy its tall mound or red cross. One structure on a single flat diamond ground tile, seen from the south. Top-left light, limited earthy DOS-like colours, no anti-aliasing. Plain solid #FF00FF magenta outside the subject. No labels, frame, grid, outline box, red cross or guides. A LOW grassy mound over a grey stone-framed tunnel with a grey paved road descending inside. Clearly a TUNNEL, not a depot or hangar. The whole tile is twice as wide as its projected ground height; total subject height is NO MORE THAN THREE QUARTERS of its width, so a 64x48 sprite box fits without cutting. No slab sides, no projecting walls at the front. The ground's south point is the lowest opaque point. Keep at least 15% blank margin around all sides. ONE picture, not a back/front split. The door faces the UPPER RIGHT, AWAY from the viewer (NE), on the back-facing side of the low mound. The opening and inside are HIDDEN behind the structure: only a solid grassy back and stone side walls show. No doorway facing the bottom, left or right foreground. The road enters from the tile's upper-right edge and is hidden by the mound immediately; show NO paved floor or long road spur behind the structure that could cover a vehicle before it reaches the mouth. Keep the mound very low and broad.
```

## road_tunnel_surface_door_sw_v2.png

Repository output: `media/multilayer/source/legacy/raw/road_tunnel_surface_door_sw_v2.png`.
References: `media/multilayer/source/legacy/raw/road_tunnel_surface_door_sw.png`.

```text
Edit this pixel-art road tunnel sprite ONLY to give it generous source-canvas padding. Preserve the low grassy mound, grey stone arch, black mouth, paved road, coarse Transport Tycoon Deluxe pixel style, top-left lighting, road direction entering from LOWER LEFT, and flat single-tile footprint. Keep the exact same view and proportions. Make the entire subject SMALLER within the canvas: centred in an invisible inner rectangle only 60% of the canvas width, with at least 25% of the canvas height pure magenta ABOVE the subject. Keep generous magenta below and on both sides. Extend the plain solid #FF00FF backdrop; do not add a visible frame or border. No guide marks, red cross, labels, new walls or extra tiles. This is a complete ONE-picture game sprite, not two panels. All opaque artwork must be fully surrounded by blank magenta, so fitting a 64x48 sprite can include extra magenta above its north corner.
```

## road_tunnel_underground_door_sw.png

Repository output: `media/multilayer/source/legacy/raw/road_tunnel_underground_door_sw.png`.
References: `media/multilayer/guides/legacy/reference/tunnel_portal.png`, `media/multilayer/source/legacy/raw/rail_portal_underground_sw_pair_v4.png`.

```text
Use case: stylized-concept. Asset: ONE whole OpenTTD UNDERGROUND road tunnel sprite, Transport Tycoon Deluxe coarse pixel art. Reference 1 gives low tunnel scale and 2:1 tile projection only; reference 2 gives grey rock/masonry and yellow lamp colours only, NOT its box roof or split panels. A LOW round stone-framed TUNNEL mouth in a charcoal-grey rock mound, with one small warm yellow lamp on either side, on one flat dark grey 2:1 diamond floor. Grey paved road begins at the LOWER-LEFT tile edge and runs straight toward UPPER RIGHT into the opening, which stands in the BACK HALF of the tile. The black interior disappears under the rounded arch. No walls beside the road in front of the opening. Definitely a rock TUNNEL, not a rectangular garage, depot, hangar or building with a flat roof. View from the south, light from top left, limited DOS-like greys and yellow, no anti-aliasing. Subject total height no more than three quarters of its width; fit the whole structure within one tile, without an overhang or slab sides. Ground south corner is lowest opaque point. Centre the entire subject in an invisible inner rectangle only 60% of the canvas width, with at least 25% canvas height blank ABOVE it and generous margin below. Pure solid #FF00FF magenta outside it. No text, frame, red cross, grid or outlines. One picture, not back/front panels.
```

## road_tunnel_underground_door_ne.png

Repository output: `media/multilayer/source/legacy/raw/road_tunnel_underground_door_ne.png`.
References: `media/multilayer/source/legacy/raw/road_tunnel_underground_door_sw.png`, `media/multilayer/source/legacy/raw/road_tunnel_surface_door_ne.png`.

```text
Create ONE whole far-facing UNDERGROUND road tunnel sprite in Transport Tycoon Deluxe coarse pixel art. Reference 1 supplies ONLY the charcoal stone, rounded arch masonry and warm yellow lamp colour/style. Reference 2 supplies ONLY the far-facing orientation and compact low tile geometry, NOT grass or green colour. The door is on the UPPER-RIGHT side of the tile, AWAY from the viewer (NE): the tunnel opening, road and interior are hidden behind a LOW charcoal-grey rock mound. Show only its solid rounded rock back and stone side faces, with the far stone mouth rim barely visible at the upper-right edge. NO opening or road facing any foreground edge. NO floor or road behind the mound that can hide an approaching bus before the mouth. This must read as a low stone TUNNEL, not a box-shaped garage or depot; no flat building roof. Exactly one flat 2:1 diamond tile. Left faces are brighter, right faces darker, light comes from TOP LEFT. Limited grey DOS-like colours, a tiny warm lamp glow may be hidden on the far side, no anti-aliasing. Total subject height is at most 3/4 of its width. Lowest opaque pixel is the ground's south corner. Make the full subject only 60% of canvas width, centred with at least 25% canvas height pure magenta above it and generous magenta margins elsewhere. Plain solid #FF00FF backdrop. No text, frame, guide lines, cross, extra tile, slab sides or split panels.
```

