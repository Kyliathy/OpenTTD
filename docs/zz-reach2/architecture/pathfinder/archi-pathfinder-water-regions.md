# Water regions

Ships do not run tile A* across the whole map. The map is cut into squares of water, each square is labelled into connected patches, and a small A* searches that patch graph first. The tile search then stays near the patch path. Tile behaviour is in [archi-pathfinder-ship.md](archi-pathfinder-ship.md). The shared `FindPath` loop is in [archi-pathfinder.md](archi-pathfinder.md).

## The square

`WATER_REGION_EDGE_LENGTH` is 16 (`src/pathfinder/water_regions.h` line 19). `WATER_REGION_NUMBER_OF_TILES` is that length squared (line 20). Region coordinates are `TileX / 16` and `TileY / 16`. `AllocateWaterRegions` (`water_regions.cpp`, called from `Map::Allocate` in `src/map.cpp`) sizes two arrays to `(map_x / 16) * (map_y / 16)`:

- `_water_region_data` — edge bits, patch labels, aqueduct flag, patch count
- `_is_water_region_valid` — filled with `false`, so the first lookup builds the square

`WaterRegionDesc` is `(x, y)`. `WaterRegionPatchDesc` adds a `WaterRegionPatchLabel` (`uint8_t`). `INVALID_WATER_REGION_PATCH` is 0 (line 21). The first real label is `FIRST_REGION_LABEL` (1) in `water_regions.cpp`.

`CalculateWaterRegionPatchHash` is `label | (region_index << 8)` (`water_regions.cpp`). The file static-asserts that a label is one byte, so the low 8 bits are the label and the rest is the region index. `WaterRegionPatchKey::operator==` compares those hashes (`yapf_ship_regions.cpp`).

## How a square is labelled

`WaterRegion::ForceUpdate` flood-fills every tile inside the 16×16 area. A tile is water when `GetTileTrackStatus` for `TransportType::Water` has track bits. Expansion uses `CFollowTrackWater`, so aqueducts, locks, and the ship follower agree on what "connected" means.

- A step that stays inside the square is pushed on the flood stack and gets the current label.
- A step that leaves the square and is not a bridge sets one bit in `edge_traversability_bits` for that side. The bit index is the local coordinate along the edge. The bitfield type is `uint16_t`, and a static assert ties its width to `WATER_REGION_EDGE_LENGTH`.
- A bridge step that leaves the square sets `has_cross_region_aqueducts`. The far end is not labelled here.

When the square has no water, or exactly one label on every tile, the label array is released. `GetLabel` then returns `INVALID_WATER_REGION_PATCH` for an empty square and `FIRST_REGION_LABEL` for a single patch. Anything more keeps a label per local tile index.

`GetUpdatedWaterRegion` calls `ForceUpdate` when `_is_water_region_valid` is false, then sets the bit true. `GetWaterRegionPatchInfo` and the neighbour walk go through that function, so a stale square is rebuilt on read.

## Neighbours

`VisitWaterRegionPatchNeighbours`:

- For each of the four sides, `VisitAdjacentWaterRegionPatchNeighbours` ANDs this square's edge bits with the neighbour square's bits on the opposite side. Each shared bit is an edge tile pair. The neighbour labels of those tiles that belong to the source patch are reported once.
- If both squares have a single patch, the neighbour is reported as `FIRST_REGION_LABEL` without a per-tile scan.
- If this square has cross-region aqueducts, every aqueduct tile of this patch reports `GetWaterRegionPatchInfo` of the other bridge end, when that end is in a different region.

`GetWaterRegionCenterTile` is the tile at local `(8, 8)` of the square (`EDGE_LENGTH / 2`). The ship tile search uses it as the heuristic target of an intermediate patch. The centre is not required to be water.

## Invalidation

`InvalidateWaterRegion` (`water_regions.cpp`) returns immediately on an invalid tile index. Otherwise it sets `_is_water_region_valid` false for the tile's region. It also checks the four diagonal neighbours: when the neighbour tile falls in a different region, that region is invalidated too. The comment says edge traversability is computed by looking at the first tile of the adjacent region, so an edit on the boundary changes both squares.

Call sites:

| Site | When |
| --- | --- |
| `DoClearSquare` in `src/landscape.cpp` | A tile is cleared to grass. `CmdBuildCanal` clears each tile through `Commands::LandscapeClear` before it places canal, river, or sea, so the square is marked invalid before the new water exists. The next lookup rebuilds from the finished tile. |
| `CmdBuildLock` in `src/water_cmd.cpp` | After the lock is placed, both mouth tiles (`tile - delta`, `tile + delta`) |
| Flooding in `src/water_cmd.cpp` | The flooded tile, after `MakeSea` or shore conversion |
| Object placement in `src/object_cmd.cpp` | Each tile of the object footprint that `HasTileWaterGround` |

`AllocateWaterRegions` drops the arrays and marks every region invalid when the map is allocated. There is no rail-style generation counter. A region stays invalid until something calls `GetUpdatedWaterRegion` on it.

## Region-level A*

`YapfShipFindWaterRegionPath` (`src/pathfinder/yapf/yapf_ship_regions.h`) is implemented by `YapfShipRegions` in `yapf_ship_regions.cpp`. The class inherits `CYapfBaseT<WaterRegionTypes>` and `CYapfSegmentCostCacheNoneT` directly. It does not use `CYapfT`, `CYapfOriginTileT`, or a real track follower. `WaterRegionFollower` exists because `AddNewNode` takes a `TrackFollower` argument; the follow callback ignores it.

`WaterRegionNode` keys on `WaterRegionPatchDesc`. The node list is `NodeList<WaterRegionNode, 12, 12>`.

The search runs **from the destination toward the ship**:

- `SetDestination` is the ship's current patch (`GetWaterRegionPatchInfo(start_tile)`).
- `AddOrigin` is each docking tile of the ordered station that `IsDockingTile` and `IsShipDestinationTile`, or the single `v->dest_tile` for any other order. An origin whose label is `INVALID_WATER_REGION_PATCH` is skipped.
- If the ship's patch is already an origin, the function returns that one patch and does not search.
- Otherwise `FindPath`. `PfDetectDestination` is `n.key == dest` (the ship's patch).

The node budget is `min(Map::Size() * NODES_PER_REGION / WATER_REGION_NUMBER_OF_TILES, MAX_NUMBER_OF_NODES)`. `NODES_PER_REGION` is 4 and `MAX_NUMBER_OF_NODES` is 65536 (`yapf_ship_regions.cpp` lines 20–21). The comment says most regions have one or two patches, and that the cap matches one node per region on a 4096×4096 map at 16×16 regions.

`PfFollowNode` calls `VisitWaterRegionPatchNeighbours` and `AddNewNode`s each neighbour.

### Cost

`DIRECT_NEIGHBOUR_COST` is 100 (`yapf_ship_regions.cpp` line 19).

`ManhattanDistance` between two patch keys is `(|dx| + |dy|) * DIRECT_NEIGHBOUR_COST`, with `dx` and `dy` in region coordinates (lines 39–41). An aqueduct that jumps several regions therefore costs the Manhattan gap, not one hop.

`PfCalcCost` sets `n.cost = parent.cost + ManhattanDistance(n, parent)`. If a grandparent exists, and the diagonal direction from grandparent to parent differs from parent to node by something other than a left or right 90° (`DiagDirDifference`), it adds 1 (line 160). The comment says this nudges the path into zigzags. The `1` is that literal, on top of the Manhattan term.

`PfCalcEstimate` is `cost` on the ship's patch, otherwise `cost + ManhattanDistance` to the destination key. There is no `estimate >= parent->estimate` assert in this class.

`TransportTypeChar` is `'^'`.

### Path returned to the tile search

The result vector starts with the ship's patch. Parents are then appended, up to `max_returned_path_length - 1` extra entries, so the vector is the ship's patch followed by patches toward the destination. `CYapfShip` asks for length `NUMBER_OR_WATER_REGIONS_LOOKAHEAD + 1` (5). A vector that long is treated as an intermediate aim; a shorter vector means the walk reached the destination side within the lookahead. An empty vector means `FindPath` failed (or no origin was usable and the ship was not already there). `ChooseShipTrack` treats empty as lost.

`FindClosestShipDepot` (`ship_cmd.cpp`) uses the same neighbour function with its own BFS and hash set. It does not build a `YapfShipRegions` instance. Visited keys are `CalculateWaterRegionPatchHash`.
