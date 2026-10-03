# Map layers

A game has three maps stacked on one `TileIndex` space: Surface, Underground, and Space. `Map` stores the tiles. `MultiLayerMap` is the only API that knows what a layer means. A viewport's `layer` field chooses which band is drawn. It is not read by commands or ticks.

## What a layer is

`MapLayer` (`src/map_layer_type.h`) is `Surface = 0`, `Underground = 1`, `Space = 2`, then `End`, with `Invalid = 0xFF`. `MAX_MAP_LAYERS` is 3. Those values are not reordered: a save stores them as numbers.

`MapLayerRules` (`src/map_layer.h`) is one row per layer: which constructions are allowed, the ground kind (`Natural`, `Bedrock`, `Vacuum`), the base height, and whether the layer has climate, water flow, and ambient sound. Surface allows the historical set. Underground allows rail, road, stations, depots, signals, signs, and layer portals. Space allows signs. `CheckLayerAllowsConstruction` refuses the rest with `STR_ERROR_NOT_ALLOWED_IN_MAP_LAYER`.

## Stacked addressing

Every layer uses the same plane size. Layer `L` occupies tiles `[L * PlaneSize, (L + 1) * PlaneSize)`. `Map::Size()` is all layers. `Map::PlaneSize()` is one. `Map::SizeX` and `Map::SizeY` stay the plane axes and stay powers of two.

`TileY` is the stacked row, so a one-tile step along Y stays inside its layer. `TilePlaneY` is that row inside the plane (`TileY & MaxY`). `GetTileLayer` is `tile >> (LogX + LogY)`. `TileXYL(x, y, layer)` builds a tile in one layer. `MultiLayerMap::TileInLayer` keeps the plane position and changes the layer. Gameplay does not add `Map::PlaneSize()` to a tile by hand.

`Map::Allocate(size_x, size_y, layer_count)` stores `size_x * size_y * layer_count` tiles. A live game passes `MAX_MAP_LAYERS`. A 4096×4096 plane times three layers still fits in a `uint32_t` `TileIndex`.

## Initialisation and the tile loop

`InitialiseLayer(Surface)` is the historical grass fill and voids only the south edges. North voiding stays in the generators. Other layers are filled with flat rock or vacuum and do not call `Random()`. Each layer has its own void fence: south edges always, north edges when freeform edges are on.

`RunTileLoop` keeps one plane-local cursor and applies it to every allocated layer on each step. Climate, flooding, and growth return immediately where the layer rules disallow them.

## Viewing a layer

`Viewport::layer` selects a band of stacked Y. `SetViewportLayer` shifts `scrollpos_*` by `RemapCoords` of that Y delta, so the same plane position stays on screen. `ScrollWindowTo` and follow-vehicle set the layer from the destination and do not also translate. Drawing treats a tile as inside the map only when its stacked Y lies in the viewport's band. Vehicles, signs, floating text, and link-graph lines are filtered to that layer. The small map has its own layer and a button that cycles it.

The main toolbar button, hotkey `N`, cycles Surface, Underground, and Space. It is disabled when the game has one layer.

## Portals

`TileType::LayerPortal` is value 11. A portal is two tiles at the same plane `(x, y)` in adjacent layers, built and removed together by `Commands::BuildLayerPortal`. Version 1 pairs Surface with Underground only (`MultiLayerMap::GetPortalTargetLayer`).

Each end has a shaft direction and a door that faces the opposite way. The other end's shaft is the reverse. `GetLayerPortalNextTile(tile, exitdir)` returns the pair when `exitdir` is the shaft, and the plain neighbour otherwise. That is the only neighbour function that yields a tile in another layer. `TranslateIntoLayerPortalPair` moves a vehicle that has crossed the shaft edge onto the pair's entry edge. There is no wormhole track.

A road portal is a bus elevator: the road toolbar builds it, and road vehicles ride it hidden at a capped speed. A rail portal is the special train tunnel: the rail toolbar builds it, and trains keep their axis track through the mouth. Trams are refused. Signals are not placed on the portal tile. Reservations use `m5` bit 4, the same bit as a rail tunnel.

## Savegame

`SaveLoadVersion::MapLayers` is the version immediately before `MaxVersion`. The `MAPS` chunk has a `layers` field from that version. Files without it load as one plane, and `AfterLoadGame` initialises the missing layers. The RIFF planes `MAPT` through `MAP8` store every allocated tile in `TileIndex` order.

## Code map

| Piece | Where |
| --- | --- |
| Layer enum | `src/map_layer_type.h` |
| Rules, pairing, plane iteration | `src/map_layer.h`, `src/map_layer.cpp` |
| Toolbar cycle | `src/map_layer_gui.cpp` |
| Portal accessors | `src/layer_portal_map.h` |
| Build, draw, clear, enter-tile | `src/layer_portal_cmd.cpp` |
| Road elevator button | `src/road_gui.cpp` |
| Rail portal button | `src/rail_gui.cpp` |
| Follower | `src/pathfinder/follow_track.hpp` |
| Unit tests | `src/tests/map_layer.cpp` |

## Invariants

- `t == GetTileLayer(t) * Map::PlaneSize() + TilePlaneY(t) * Map::SizeX() + TileX(t)` for every tile.
- A one-tile step from a non-void tile stays in its layer, except `GetLayerPortalNextTile` on a shaft exit.
- `Viewport::layer` never influences a command, a tick, or `Random()`.
- `Map::Size()` bounds storage. Per-plane logic uses `Map::PlaneSize()` and `TilePlaneY()`.
- Surface generation for a given seed does not call `Random()` while filling the other layers.
- Every portal end has a pair whose shaft direction is reversed.
