# Map layers

A game has four maps stacked on one `TileIndex` space: Surface, Underground, Space, and Deep Underground in storage order. `Map` stores the tiles. `MultiLayerMap` is the only API that knows what a layer means. A viewport's `layer` field chooses which band is drawn. It is not read by commands or ticks.

## What a layer is

`MapLayer` (`src/map_layer_type.h`) is `Surface = 0`, `Underground = 1`, `Space = 2`, `DeepUnderground = 3`, then `End`, with `Invalid = 0xFF`. `MAX_MAP_LAYERS` is 4. Those values are not reordered: saves retain Space at value 2. Display order is Surface, Underground, Deep Underground, Outer Space, defined by `_map_layer_display_order` in `src/map_layer_gui.cpp`.

`MapLayerRules` (`src/map_layer.h`) is one row per layer: which constructions are allowed, the ground kind (`Natural`, `Bedrock`, `Vacuum`, `DeepBedrock`), the base height, and whether the layer has climate, water flow, and ambient sound. Surface allows the historical set. Underground allows rail, road, stations, both depot kinds, signals, signs, and layer portals. Space allows signs. Deep Underground allows roads, road stops, road depots, signs and elevators; rail construction is refused. `CheckLayerAllowsConstruction` refuses the rest with `STR_ERROR_NOT_ALLOWED_IN_MAP_LAYER`.

## Stacked addressing

Every layer uses the same plane size. Layer `L` occupies tiles `[L * PlaneSize, (L + 1) * PlaneSize)`. `Map::Size()` is all layers. `Map::PlaneSize()` is one. `Map::SizeX` and `Map::SizeY` stay the plane axes and stay powers of two.

`TileY` is the stacked row, so a one-tile step along Y stays inside its layer. `TilePlaneY` is that row inside the plane (`TileY & MaxY`). `GetTileLayer` is `tile >> (LogX + LogY)`. `TileXYL(x, y, layer)` builds a tile in one layer. `MultiLayerMap::TileInLayer` keeps the plane position and changes the layer. Gameplay does not add `Map::PlaneSize()` to a tile by hand.

`Map::Allocate(size_x, size_y, layer_count)` stores `size_x * size_y * layer_count` tiles. A live game passes `MAX_MAP_LAYERS`. Four 4096×4096 planes (67,108,864 tiles) fit in a `uint32_t` `TileIndex`; a 16-bit RIFF plane fits in its 28-bit length. Four planes use a third more map storage than three.

## Initialisation and the tile loop

`InitialiseLayer(Surface)` is the historical grass fill and voids only the south edges. North voiding stays in the generators. Other layers are filled with flat rock or vacuum and do not call `Random()`. Each layer has its own void fence: south edges always, north edges when freeform edges are on.

`RunTileLoop` keeps one plane-local cursor and applies it to every allocated layer on each step. Climate, flooding, and growth return immediately where the layer rules disallow them.

## Viewing a layer

`Viewport::layer` selects a band of stacked Y. `SetViewportLayer` shifts `scrollpos_*` by `RemapCoords` of that Y delta, so the same plane position stays on screen. `ScrollWindowTo` and follow-vehicle set the layer from the destination and do not also translate. Drawing treats a tile as inside the map only when its stacked Y lies in the viewport's band. Vehicles, signs, floating text, and link-graph lines are filtered to that layer. The small map owns its own `layer` and marks its link overlay dirty on a change.

The main toolbar, scenario editor, extra viewport and smallmap layer buttons open `BuildMapLayerDropDown`, showing allocated layers in display order. Each choice affects that button's view and icon. Hotkey `N` cycles the main view in the same order. Buttons are disabled in a single-layer game.

## Portals

`TileType::LayerPortal` is value 11. A portal is two tiles at the same plane `(x, y)`, built and removed together by `Commands::BuildLayerPortal`. `LayerPortalKind` has three derived kinds: `RailTunnel` and `RoadTunnel` pair Surface with Underground; `RoadElevator` pairs Deep Underground with Surface or Underground. `MultiLayerMap::IsPortalPairAllowed` validates pairs; `GetPortalTargetLayer(from, kind, elevator_upper)` chooses a placement target. `GetLayerPortalKind(tile)` derives the kind from transport and layers without a saved kind bit. A Surface-to-Deep pair leaves the intermediate Underground tile untouched.

Each end has a shaft direction and a door that faces the opposite way. The other end's shaft is the reverse. `GetLayerPortalNextTile(tile, exitdir)` returns the pair when `exitdir` is the shaft, and the plain neighbour otherwise. That is the only neighbour function that yields a tile in another layer. `TranslateIntoLayerPortalPair` moves a vehicle that has crossed the shaft edge onto the pair's entry edge. There is no wormhole track.

Road tunnels and rail portals are driven through at normal speed. A road elevator caps approach speed, then stops the front vehicle hidden at the shaft edge for `LAYER_ELEVATOR_WAIT_TICKS` (370 game ticks). `RoadVehicle::elevator_wait` is 0 while idle, above 1 while counting down, and 1 while ready to cross. Only a committed crossing or turn resets it to 0; a blocked pair keeps it ready without another ride. Stopping the bus or an approach breakdown pauses the countdown. Articulated parts follow their front. `CmdTurnRoadVeh` refuses during a ride. YAPF adds the portal penalty for both road kinds and `LAYER_ELEVATOR_WAIT_COST` (10 tile costs) for an elevator.

Trams are refused. Signals are not placed on a portal tile. Rail reservations use `m5` bit 4, the same bit as a rail tunnel. Elevator housings on Underground and Deep Underground share art; Deep Underground floor/icon and road tunnel art have marked stand-ins pending r2mlm13 approval.

## Savegame

`SaveLoadVersion::MapLayers` (369) introduces the `MAPS` `layers` field. Files without it load as one plane, and `AfterLoadGame` initialises missing layers, including Deep Underground for version-369 files. The RIFF planes `MAPT` through `MAP8` store every allocated tile in `TileIndex` order. `SaveLoadVersion::DeepUnderground` (370) adds the saved `RoadVehicle::elevator_wait`. `MAPSChunkHandler` rejects counts outside 1..4 and rejects a pre-370 file claiming four layers, in both load and preview. Older Surface/Underground road portals derive as road tunnels and have no wait.

## Code map

| Piece | Where |
| --- | --- |
| Layer enum | `src/map_layer_type.h` |
| Rules, pairing, plane iteration | `src/map_layer.h`, `src/map_layer.cpp` |
| Names, icons, dropdown, display-order cycle | `src/map_layer_gui.cpp`: `BuildMapLayerDropDown`, `GetNextMapLayer`, `SetViewportLayer` |
| Portal accessors | `src/layer_portal_map.h` |
| Build, draw, clear, enter-tile | `src/layer_portal_cmd.cpp` |
| Road tunnel/elevator buttons and upper-end picker | `src/road_gui.cpp` |
| Elevator wait controller and constants | `src/roadveh_cmd.cpp`: `RoadVehController`, `IndividualRoadVehicleController`, `LAYER_ELEVATOR_WAIT_TICKS` |
| Elevator path cost | `src/pathfinder/yapf/yapf_road.cpp`: `LAYER_ELEVATOR_WAIT_COST` |
| Map count / vehicle wait save fields | `src/saveload/map_sl.cpp`, `src/saveload/vehicle_sl.cpp` |
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
