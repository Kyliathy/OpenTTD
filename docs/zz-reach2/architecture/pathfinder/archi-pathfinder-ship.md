# Ship YAPF

Ship movement runs two searches. A coarse A* over water-region patches (`YapfShipRegions`, [archi-pathfinder-water-regions.md](archi-pathfinder-water-regions.md)) produces a corridor. A tile A* (`CYapfShip` in `src/pathfinder/yapf/yapf_ship.cpp`) finds the next track inside that corridor. Both use `CYapfBaseT::FindPath`. `CYapfShip` is `CYapfT<CYapfShip_TypesT<CYapfShip>>`. The region search inherits `CYapfBaseT` directly and does not go through `CYapfT`.

## Tile pathfinder types

`CYapfShip_TypesT`:

| Role | Class |
| --- | --- |
| Follower | `CFollowTrackWater` |
| Node list | `CShipNodeList` = `NodeList<CYapfShipNode, 10, 12>` |
| Vehicle | `Ship` |
| Follow | `CYapfFollowShipT` |
| Origin | `CYapfOriginTileT` |
| Destination | `CYapfDestinationTileWaterT` |
| Cache | `CYapfSegmentCostCacheNoneT` |
| Cost | `CYapfCostShipT` |

`CYapfShipNode` (`yapf_node_ship.hpp`) adds no fields. The key is `CYapfNodeKeyExitDir` (tile + exit direction). A node is one tile, not a folded segment. `PfFollowNode` steps `Follow` once from `node.key` and calls `AddMultipleNodes`.

`CYapfShip`'s constructor replaces `max_search_nodes` with `MAX_SHIP_PF_NODES`:

```
(NUMBER_OR_WATER_REGIONS_LOOKAHEAD + 1) * WATER_REGION_NUMBER_OF_TILES * 4
```

`NUMBER_OR_WATER_REGIONS_LOOKAHEAD` is 4 (`yapf_ship.cpp` line 21). `WATER_REGION_NUMBER_OF_TILES` is `16 * 16` (`water_regions.h` lines 19–20). The `4` is the comment's "4 possible exit dirs per tile" (`yapf_ship.cpp` line 22).

## Choosing a track

`CYapfFollowShipT::ChooseShipTrack`:

1. `YapfShipFindWaterRegionPath(v, tile, NUMBER_OR_WATER_REGIONS_LOOKAHEAD + 1)` — at most 5 patches, ship's patch first.
2. An empty corridor means the ship is lost. `CreateRandomPath` writes `SHIP_LOST_PATH_LENGTH` steps into the path cache. That constant is 8 (`yapf_ship.cpp` line 24). Each step is `GetRandomTrackdir` (`RandomRange` over the follower's trackdirs, 90° turns removed unless they are the only option).
3. Otherwise two attempts, each a fresh `CYapfShip(MAX_SHIP_PF_NODES)`:
   - Origin: `v->tile` and `forward_dirs | reverse_dirs`.
   - `SetDestination(v)` from the order.
   - If the region path came back at full length (`size >= NUMBER_OR_WATER_REGIONS_LOOKAHEAD + 1`), `SetIntermediateDestination` aims at the centre tile of the last patch (`GetWaterRegionCenterTile`) and treats arrival as "any tile of that patch".
   - Attempt 0 does not restrict tiles. Attempt 1 calls `RestrictSearch`, and `PfFollowNode` drops any step whose new tile lies outside the corridor's regions.
   - If attempt 0 does not reach the destination, attempt 1 runs. The comment names long aqueducts and maze-shaped water as the cases that hit the node limit.

`PfDetectDestinationTile`:

- intermediate patch: the tile's region must equal the patch's region (a fast reject), then `GetWaterRegionPatchInfo` must equal the patch
- go-to station: `IsDockingTile` and `IsShipDestinationTile` for that station
- otherwise: tile equals `dest_tile` and the trackdir is in `GetTileTrackStatus` for water

`PfCalcEstimate` is `cost` on the destination, otherwise `cost + OctileDistanceCost` to the intermediate centre or the real `dest_tile`, with `assert(estimate >= parent->estimate)`.

### Path cache write-back

`Ship::path` is a `vector` of trackdirs only (`ShipPathElement` in `ship.h`), back element consumed first. While walking parents from the best node toward the origin, a node is pushed, with these resets (`ChooseShipTrack`):

- The cached steps must stay on the high-level patch list. A node whose patch is off that list clears the cache. The comment points at ships looping when this is violated.
- For a final (not intermediate) destination, nodes outside the end patch are kept (`add_full_path`) so the ship does not re-plan onto a different dock in a neighbouring region and loop.
- Hitting the start patch pushes that node and subsequent parents (the origin side).

If the origin node's trackdir is not in `forward_dirs`, the cache is cleared and the function returns `Trackdir::Invalid`. `YapfShipChooseTrack` turns that into `Track::Invalid`, which the movement code treats as reverse.

An empty cache at an already-reached destination returns `CreateRandomPath` of length 1. The comment says this nudges a ship that should not have called the pathfinder.

When the start patch and the end patch are the same, the cache is cleared after the next step is taken, so ships can spread across docking tiles in the final region.

`YapfShipChooseTrack` itself only passes forward dirs (`TrackdirToTrackdirBits` of the ship's current trackdir) and an empty reverse set.

## Reverse check

`YapfShipCheckReverse` → `CheckShipReverse`:

- `trackdir == nullptr` (normal leave-dock): forward bits are the current trackdir, reverse bits are `ReverseTrackdir`. Returns true when a path exists and the winning origin trackdir is the reverse one.
- `trackdir != nullptr` (the ship can no longer move forward, for example after terraforming): origin dirs are empty and reverse dirs are the water trackdirs reachable from the reversed exit. The out-parameter receives the best origin trackdir, or a random reverse trackdir when the search fails. The function returns true.

`CheckShipStayInDepot` (`ship_cmd.cpp`) calls this with a null trackdir and flips `direction` when it returns true.

## Tile cost

`CYapfCostShipT::PfCalcCost` prices the child node only. It reads `n.parent`, so it runs on expanded children, not on startup nodes.

Base:

- diagonal trackdir: `YAPF_TILE_LENGTH` (100, `pathfinder_type.h` line 16)
- other trackdir: `YAPF_TILE_CORNER_LENGTH` (71, `pathfinder_type.h` line 19)

`CurveCost`:

- 90° (`TrackdirCrossesTrackdirs`): `ship_curve90_penalty`
- any other turn that is not `NextTrackdir`: `ship_curve45_penalty`
- straight: 0

Defaults in `src/table/settings/pathfinding_settings.ini`: `ship_curve45_penalty` is `1 * YAPF_TILE_LENGTH` (line 381), `ship_curve90_penalty` is `6 * YAPF_TILE_LENGTH` (line 390). The running values are `_settings_game.pf.yapf.ship_curve45_penalty` and `ship_curve90_penalty`.

Further literals in `PfCalcCost` (`yapf_ship.cpp`):

- docking tile: `count * 3 * YAPF_TILE_LENGTH`, where `count` is the number of ships on the tile that are not hidden (aircraft and ships inside a depot are ignored). The `3` is the literal on that line (line 385).
- a trackdir that fails `IsPreferredShipDirection`: `YAPF_TILE_LENGTH` (line 389). The predicate keeps ships of opposite directions on different tiles: diagonal X/Y depend on the parity of Y or X; the other trackdirs use `(odd_x ^ odd_y) ^` membership in `{Right_N, Left_S, Upper_W, Lower_E}`.
- aqueduct tiles skipped by the follower: `YAPF_TILE_LENGTH * tiles_skipped` (line 392)
- ocean or canal speed fraction: `YAPF_TILE_LENGTH * (1 + tiles_skipped) * speed_frac / (256 - speed_frac)` (line 397). `speed_frac` is `ocean_speed_frac` on sea (`GetEffectiveWaterClass`) and `canal_speed_frac` otherwise, from `ShipVehicleInfo`. The `256` is the literal divisor on that line. A zero fraction adds nothing.
- middle tile of a lock: `(TILE_HEIGHT * YAPF_TILE_LENGTH * canal_speed) / 128` (line 403). `canal_speed` is `ApplyWaterClassSpeedFrac(max_speed, false)`. The comment says the cost scales with speed because the ship stops in the lock. `128` is the literal divisor on that line.

Node cost is `parent->cost + c`.

## Call sites in `ship_cmd.cpp`

| Caller | What it runs |
| --- | --- |
| `ChooseShipTrack` | path-cache pop, else `YapfShipChooseTrack` |
| `CheckReverseShip` | `YapfShipCheckReverse` |
| `CheckIfShipNeedsService` | `FindClosestShipDepot`, which is a BFS over patches, not tile A* |

`ChooseShipTrack` skips YAPF when `dest_tile` is invalid: it keeps the current axis, or the first available track, and reports `path_found = false`.

`FindClosestShipDepot` limits the BFS to `maximum_go_to_depot_penalty / YAPF_TILE_LENGTH` tiles, converted to a per-axis region distance of `(max_distance / WATER_REGION_EDGE_LENGTH) + 1`. `WATER_REGION_EDGE_LENGTH` is 16 (`water_regions.h` line 19). The default of `maximum_go_to_depot_penalty` is `20 * YAPF_TILE_LENGTH` (`pathfinding_settings.ini` line 372). Among owned ship depots inside the visited patches it picks the smallest `DistanceSquare`, still within `max_distance` squared.

## Lost ships and determinism

Tile costs above do not call `RandomRange`. The lost-ship walk and the one-step nudge do. They exist so a failed search is not repeated every tick (`CreateRandomPath`'s comment). Those trackdirs are written into `Ship::path` and consumed like a normal cache.
