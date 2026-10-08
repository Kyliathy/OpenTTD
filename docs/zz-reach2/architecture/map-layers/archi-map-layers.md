# Map layers

A live game has four map planes in one `TileIndex` space. `Map` owns tile storage and geometry; `MultiLayerMap` owns layer meaning, initialization, construction rules and portal pairing. A viewport's `layer` selects which plane it draws. Commands and simulation ticks use world tiles, independently of that client selection.

This page describes delivered layer, [geology/excavation](../../upgrades/2026-10/r2mlvs2-geology-and-excavation.md), physical portal/TBM/spoil and electrical-owner contracts through2026-10-08/save385. Selected M1 graphics retain the immutable66 original sprites inside the published2,404 bound and complete technical native review. [Final target audit](../../upgrades/2026-10/r2mlvs3mp-z-b-final-closure-audit.md) distinguishes delivered code/evidence from pending network/bootstrap/full-performance/late-consumer/personal-review acceptance. The [level0 overview](../@archi-ttd-level0.md) and [multilayer epic](../../upgrades/2026-10/r2mlm-multi-layered-map.md) retain the original architectural context.

## Layers and rules

`MapLayer` in `src/map_layer_type.h` has immutable values `Surface = 0`, `Underground = 1`, `Space = 2`, `DeepUnderground = 3`, then `End`, with `Invalid = 0xFF`. `MAX_MAP_LAYERS` is 4. Deep Underground was appended after Space so existing saved tile indices retain their meaning. Menu order is separate: Surface, Underground, Deep Underground, Outer Space, defined by `_map_layer_display_order` in `src/map_layer_gui.cpp`.

| Menu order | Layer / storage value | Terrain and permitted construction |
| --- | --- | --- |
| 1 | Surface / 0 | Ordinary generated terrain and the historical construction set, subject to normal command checks. |
| 2 | Underground / 1 | Flat bedrock at height 0: rail, roads, rail stations/waypoints, road stops/waypoints, train/road depots, signals, signs and permitted portals. |
| 3 | Deep Underground / 3 | Classic retains roads, road stops/waypoints, road depots, signs and elevators. Multilayer Industry additionally permits rail/stations/depots/signals and Underground/Deep rail portals, subject to actual excavation and the 1980 calendar gate. |
| 4 | Outer Space / 2 | Flat vacuum at height 0 with a sparse-star floor; signs only. |

`MapLayerRules` contains the allowed-construction bitset, ground kind (`Natural`, `Bedrock`, `Vacuum`, `DeepBedrock`), initial height and climate, water-flow and ambient-sound flags. `CheckLayerAllowsConstruction` rejects prohibited work with `STR_ERROR_NOT_ALLOWED_IN_MAP_LAYER`. The non-Surface layers have no towns, houses, industries, planted trees, water, terraforming, ordinary bridges/tunnels or trams. Portal permission is also checked against its kind and both layers; allowing `LayerPortal` construction alone does not allow every destination.

## Storage, coordinates and distance

Every plane has the same dimensions. Layer `L` occupies tiles `[L * PlaneSize, (L + 1) * PlaneSize)`. `Map::Size()` bounds all allocated tiles; `Map::PlaneSize()` bounds one plane. `Map::SizeX()` and `Map::SizeY()` remain the plane axes and remain powers of two. `Map::Allocate(size_x, size_y, layer_count)` allocates their product; live new/load games allocate all four layers, while low-level tests can allocate fewer.

`TileY` is the stacked row. `TilePlaneY` is the row within a plane (`TileY & MaxY`). `GetTileLayer` extracts the layer above the plane-index bits; `TileXYL(x, y, layer)` constructs an address. `MultiLayerMap::TileInLayer` preserves plane coordinates while changing layer, `ProjectToSurface` finds the corresponding Surface tile, and `IterateLayer` bounds a walk to one plane. Gameplay does not add `Map::PlaneSize()` to tile addresses by hand.

World `y_pos` uses the same stacking, with one band equal to `Map::SizeY() * TILE_SIZE`. Existing tile references, vehicle positions, hashes, signs and spatial indexes therefore distinguish layers without introducing another layer field into each world object. An ordinary neighbor step remains within the plane's void fence; portal traversal is the explicit exception.

Geometric distance helpers and YAPF's octile heuristic use plane coordinates, avoiding artificial distance from the storage bands. Two tiles at the same horizontal position have zero geometric separation even when their layers differ. Cargo payment likewise avoids charging for stacked Y. Town association and local authority project to Surface. This projection does not make Surface houses produce or accept cargo through an Underground station: shared metro catchment remains future work.

Four 4096 x 4096 planes contain 67,108,864 tiles, fitting the `uint32_t` tile index and the 28-bit RIFF lengths used by map chunks. Four planes use one third more map storage than three, and four times the tile storage of a Surface-only map. These are storage facts; large-map runtime and memory acceptance remain separate checks.

## Initialization and simulation

`InitialiseLayer(Surface)` retains the historical grass fill and south void edges; north-edge handling remains with the generators. Other layers are initialized as flat bedrock or vacuum with their own void fences: south edges always, north edges when freeform edges are enabled. Filling them never calls simulation `Random()`, preserving Surface generation for the same seed.

`RunTileLoop` keeps one plane-local LFSR cursor and applies it to every allocated plane at each step. Climate, growth and flooding are restricted by layer rules. The existing vehicle-pool tick advances vehicles throughout the stacked world. Changing, hiding or closing a layer view cannot suspend its traffic or alter simulation random-number use.

## Layer dropdowns and view state

The main toolbar's Layer button sits immediately to the right of Small map. The scenario editor has the same selector. Extra viewports and the small map have their own Layer buttons. All four button locations call `BuildMapLayerDropDown`, which creates an icon/name entry for each allocated layer in display order. Entry values are stable `MapLayer` IDs, not menu positions. A choice changes that view and its icon; choosing a small-map or extra-viewport layer does not change the main view. Single-layer maps disable the selector, and partially allocated maps omit unavailable layers.

The default `N` hotkey cycles the main game view through the same display order, skipping unavailable layers. The scenario editor's cycle action has no default key binding. View captions report their own layer.

`SetViewportLayer` shifts both current and destination scroll positions by the isometric projection of the stacked world-Y delta. The same horizontal position stays on screen. It cancels following a vehicle on another layer and invalidates cached overlays. `ScrollWindowTo` and vehicle following select the destination layer without translating an already projected destination twice. Map picking, clamping and tile selection use the viewport's own band; construction commands derive the layer from the clicked tile, including clicks in extra viewports.

Landscape, vehicles, signs, floating text and link-graph lines are filtered to the view's layer. The small map owns its layer and invalidates its link overlay when switching. `GetVehicleDisplayLayer` keeps aircraft, ships and disasters on Surface, derives effects from world Y, and normally derives ground vehicles from their tile, with a world-Y fallback for an invalid tile.

`SaveViewportBeforeSaveGame` removes the main camera's projected layer offset before writing the existing `VIEW` coordinates. Loading starts on Surface at the same plane position and zoom; the selected layer is not saved. Headless/no-camera saves retain the invalid-coordinate sentinels. This camera correction required no field or version change and does not retroactively repair offsets written by earlier builds.

## Underground construction and same-layer facilities

[R2MLVS7](../../upgrades/2026-10/r2mlvs7-underground-construction-and-fleet.md) replaces the rock-layer Airport action with the explicit Underground Construction toolbar in main/split toolbar and keyboard contexts; the editor exposes the same construction entry through its industry slot while retaining ordinary scenario road tools. Surface keeps its ordinary tools. Menu choices capture their requested layer, and placement uses the clicked world tile from the actual main or extra viewport. Layer changes cancel unfinished placement, including tile centering and vehicle-follow navigation. Commands remain authoritative: ordinary airport construction below Surface is refused independently of menu visibility.

The toolbar binds delivered survey, native rectangular TBM commission/quote, project exchange, mine, conventional rail/road station and depot controls. Survey and quotes do not claim reserves or excavate. Native facility quotes validate the complete excavated floor and required approaches, report the first blocked world cell and use the same footprint/cost checks as execution. Canonical chamber previews distinguish requested floor, swept approach/turn space and uncovered cells; purchase is separate from later material/work charges. Tube and geothermal controls wait for their named producers.

Stations retain ordinary ownership, spread, orders and cargo handling, but joining requires the same physical layer before adding either area. `CheckStationSpread` in `src/station.cpp` rejects a direct join across the storage-band boundary even when raw stacked Y coordinates appear close. Identical plane coordinates on Surface, Underground and Deep yield separate station identities and same-layer catchment; proximity does not transfer cargo. Actual joined/separate rock terminals load finite mine output through ordinary station orders into purchased native carriers.

Native conventional depot lists and buy/refit/clone/renew commands share the exact profile/date/layer/cargo predicate in `src/underground_vehicle.h`. Deep admission and calendar restrictions therefore remain simulation checks even when a UI snapshot is stale. Connected visible excavation, spoil exchange, mine construction and actual mineral Surface delivery pass on both rock layers through main/extra views, full/split toolbars and keyboard/menu entry. Separate editor cases author ordinary roads on actual open floor, retain company claim/fleet restrictions and resume normal freight in the same world. Portal placement completes an existing road stub with the normal paid road command; connection still validates both approaches. Final artwork remains a separate acceptance task.

## Paid boring and real spoil haulage

[R2MLVS5](../../upgrades/2026-10/r2mlvs5-spoil-wagons-and-haulage.md) delivers both source-magazine and recurring underground-depot logistics. Paid wide clearing opens the genuine depot floor and its whole-consist straight approach. Released support converts through ordinary commands, while a separate funded Connected portal/rail route reaches the existing Surface origin before the next cutter faces. A healthy empty native machine withdrawn at its original Sealed source after retirement can be recommissioned by the existing command. Transient reuse exempts only that exact whole chain from source occupancy, preserves paid part/unit identities and charges no second purchase; rollback deletes only newly bought parts.

`GetTBMWorkRailPath` publishes the pure support path used by cutting. Ordinary commands fund missing pieces only on already excavated floor; support may reuse exact owned compatible permanent plain rail. Depot buildings stay outside cutter sweeps even on open floor. Face steering requires the canonical entry/exit piece at old junctions. Empty assigned haulers return through the actual depot door; their ordinary originating Surface-station order remains the mandatory loaded delivery leg. Atomic exchange moves real carriages and packets; bounded native receivers account for disposal/crushing and costs.

Actual application-load proof covers 12 source and 12 recurring-depot families: stopped/await/disposal/empty-return, spanning parts, infrastructure/capacity holds, queued occupation and irreversible loaded crashes. Native vehicle/packet/order, geology/job/project/support, portal/exchange, receiver and money owners reconstruct before supported recovery; all 35 next-transition owner chunks match after one/two loads. Original crashed wagons retain spoil until native delayed deletion issues explicit destruction receipts. A protected missing-project lifecycle/load refusal is not represented by fabricated orphan data. Actual complete source/depot application-loader pairs prove equal physical completion timing and identical final owner bytes with headless, visible cutter-plane rendering and empty Space rendering. Final art and multiplayer/performance acceptance remain separate master scopes.

## Portal tiles, commands and connectivity

`TileType::LayerPortal` is value 11, distinct from ordinary `TunnelBridge` tiles. A portal consists of two ends at the same plane `(x, y)`, built and removed as a pair. Its kind is derived from transport and the endpoint layers, without a saved kind bit.

| `LayerPortalKind` | Allowed pair | Movement |
| --- | --- | --- |
| `RailTunnel` | Surface/Underground; Industry additionally Underground/Deep | Connected train passage at normal speed. Sealed/work edges remain unavailable to ordinary traffic. |
| `RoadTunnel` | Surface and Underground | Road vehicle crosses at normal speed. |
| `RoadElevator` | Deep Underground and either Surface or Underground | Road vehicle approaches at capped speed, then waits hidden before crossing. |

`MultiLayerMap::IsPortalPairAllowed` validates this matrix. `GetPortalTargetLayer(from, kind, elevator_upper)` selects the build target. The rail toolbar offers rail portals; the road toolbar has separate tunnel and elevator tools and orientation pickers. Building an elevator from Deep Underground also selects its upper destination. Numeric adjacency does not determine connectivity: a Surface-to-Deep elevator leaves the Underground tile between them untouched. Outer Space has no portal connection.

Commands validate both ends, including layer permission, location, flatness, ownership, vehicles and reservations, before committing changes. Clear, ownership/infrastructure accounting and rail/road conversion operate on the pair. Rail conversion validates the whole pair before mutation and refreshes affected train consists from both ends. Ordinary single-tile conversion must not partially modify a portal before those checks.

Each end has a shaft direction and a door facing the opposite way. Its pair has the reverse shaft direction. `GetLayerPortalNextTile(tile, exitdir)` returns the pair for a shaft exit and an ordinary neighbor otherwise. Track followers, signal/reservation traversal, pathfinders and vehicle movement use this shared neighbor contract; the final train-controller correction also obtains its movement destination through it.

`TranslateIntoLayerPortalPair` shifts world Y to the destination band and puts the vehicle onto the pair's entry edge. Controllers maintain the heading and use the destination tile height, allowing different endpoint heights. Layer portals do not use ordinary tunnel wormhole movement. `IsHiddenInLayerPortal` derives visibility from the vehicle's depth along the shaft, using the shared vanilla `_tunnel_visibility_frame`; this applies to each part during entry, exit, turns and reversals.

Signals belong on adjoining normal rail, not portal tiles. Portal rail reservations use `m5` bit 4, as rail tunnels do. Stations and their orders retain ordinary tile/station identities across layers: a train can visit stations on both sides if their platforms actually connect to its route. The supplied save's station/signal report was traced to disconnected platforms; the connected fixture demonstrated stopping and cross-layer travel without a station/pathfinder change for that report.

## Timed road elevators

The front road vehicle stops hidden at the shaft edge for `LAYER_ELEVATOR_WAIT_TICKS`, 370 game ticks, roughly five game days or ten seconds at normal speed. This uses simulation time, not wall-clock time. `RoadVehicle::elevator_wait` is 0 when idle, above 1 while counting down, and 1 when ready to cross. Stopping the vehicle or an approach breakdown pauses the countdown; turning around during a ride is refused.

A blocked destination retains the ready state, so the vehicle does not pay another wait when the exit clears. A committed crossing or permitted turn resets the wait to zero. The vehicle leaves the timed crossing from zero speed; articulated parts follow their front. Saving during the wait stores the counter for later continuation. YAPF includes the portal penalty for road tunnels and elevators and adds `LAYER_ELEVATOR_WAIT_COST`, ten tile costs, for the elevator.

## Generated graphics and drawing

OpenTTD's extra GRF contains an Action 5 type `0x1C` block beginning at `SPR_MAP_LAYER_BASE`. `MAP_LAYER_SPRITE_COUNT` now follows `MULTILAYER_M1_SPRITE_COUNT` (2,404); `SPR_NEWGRFS_BASE` follows the published bound. Original offsets0..65 remain immutable and66..73 stay reserved/unactivated. Selected delivered core segments preserve planned74..199; vehicle/portal/facility/UI/cargo families append after199 in stable manifest order. The original sheet is retained separately from selected M1 PNG/NFO. [sprite_quantize.py](../../upgrades/2026-10/tools/sprite_quantize.py) retains the DOS palette/native anchor contract and original44 recipe; [M1 packing and technical review](../../upgrades/2026-10/r2mlvs3mp-t-gallery-acceptance.md) verifies forty selected families/2,311 sprites,2,377 actually loaded sprites and the checked-in generator-free payload.

| Block offsets | Contents |
| --- | --- |
| 0..43 | Original layer icons, rail-portal/elevator tools and cursors, Underground/Space floors and four-direction rail/elevator back/front pairs. |
| 44 | Deep Underground icon. |
| 45..46 | Road tunnel toolbar icon and cursor from a shared raw image. |
| 47..49 | Three native-coverage Deep Underground floor variants. |
| 50..57 | Surface road tunnel back/front pairs in NE, SE, SW, NW door order. |
| 58..65 | Underground road tunnel back/front pairs in the same door order. |

The installed sheet and NFO are `media/baseset/openttd/map_layers.png` and `map_layers.nfo`, compiled into `media/baseset/openttd.grf`. Floors replace the earlier stand-ins. Portal pickers draw the generated back/front pair for the selected door direction. Underground and Deep Underground elevator housings share the Underground art; the optional separate Deep elevator housing extension has not been generated or approved.

Road tunnel backs draw as ground; cut fronts sort over vehicles using vanilla tunnel bounds and separator boxes. Both parts come from one whole picture with a common anchor. Rail/elevator approaches retain native track or road-type overlays. Their foreground repairs cover those overlays and hide vehicles behind solid walls and roofs: far-facing fronts reuse the whole housing picture with a full tile footprint, while near-facing fronts combine solid roof/wall coverage and an aligned generated doorway frame. These repairs address the apparent housing/orientation mismatch without rotating gameplay directions. Rail/elevator housings follow building transparency controls.

The [original asset plan](../../upgrades/2026-10/r2mlm6-layer-graphics-assets.md) defines the 44-sprite contract; the [art extension and review](../../upgrades/2026-10/r2mlm13-layer-art-review-and-new-sprites.md) records the 66-sprite integration, foreground repairs, screenshots and remaining gallery/approval gaps. The current set is integrated development art with representative runtime checks; full human art acceptance is still unverified.

## Savegame compatibility

`SaveLoadVersion::MapLayers` (369) introduces the `MAPS` `layers` field and storage for Surface, Underground and Space. Files without it contain one Surface plane. `SaveLoadVersion::DeepUnderground` (370) adds the fourth plane and the saved `RoadVehicle::elevator_wait` field. Storage values remain Surface 0, Underground 1, Space 2, Deep Underground 3 regardless of menu order.

The ten map chunks (`MAPT`, `MAPH`, `MAPO`, `MAP2`, `M3LO`, `M3HI`, `MAP5`, `MAPE`, `MAP7`, `MAP8`) serialize tile fields in `TileIndex` order across all stored planes. Loading uses the file's layer count to bound reads while allocating all current planes; after settings load, `AfterLoadGame` initializes missing planes, including Deep Underground in version-369 saves. Old Surface/Underground road portals derive as road tunnels and do not acquire elevator waits.

`MAPSChunkHandler` validates layer counts in both load and load-preview paths, rejects counts outside 1..4, and rejects a pre-370 file claiming four layers. On a real load, `Map::Allocate` validates dimensions; preview records the file's dimensions and layer count without allocating the live map. General chunk/reference conversion is documented in [savegame architecture](../savegame/archi-savegame.md); the view-camera correction above changes only existing camera metadata.

## Gameplay-profile foundation (R2MLVS1)

[R2MLVS1 Content Freeze](../../upgrades/2026-10/r2mlvs1-g-content-freeze.md) adds explicit Classic/Multilayer selection and saved profile metadata in version 371. The shared policy owner is `multilayer_content.cpp`; legacy files without metadata retain Classic and their existing content behavior. Layer storage IDs, addressing and the 66-sprite native block remain unchanged. Metadata acceptance checks exact first-party content and cargo meanings before dependent gameplay fixups, with independent nonmutating preview fields; see [savegame architecture](../savegame/archi-savegame.md).

`CheckDeepLayerAvailability` publishes a game-calendar boundary of 1 January 1980 for the new profile, with explicit saved editor override and legacy policy. R2MLVS2 uses it in excavation construction, job reservation/work and cut checks. R2MLVS3 uses it for actual Deep rail/elevator build, connection, routing and movement. Actual TBMs, tube networks, orbital movement and final excavation/portal art remain in the owning [R2MLVS feature plans](../../upgrades/2026-10/r2mlvs-multilayer-vehicles-and-structures.md).

## Geology and excavation (R2MLVS2)

The explicit Multilayer Industry profile generates independent two-bit Solid/Excavating/Excavated terrain for both rock planes, finite mineral seams and a separate bounded geothermal field. `Map` retains infrastructure/geometry ownership. Clearing a road, rail or building leaves the geological cavity and remaining reserve unchanged. Classic and predecessor saves without geology retain their historical open-floor policy.

`excavation.h` owns typed deposit/job/vent IDs, canonical geometry and public guard/transaction contracts. Ordinary construction checks complete foundations and approaches before writes; viewport selection grants no exception. Job claims are reconstructed from bounded unfinished routes. A pure planner expands actual straight/left-45/right-45 advances with full cutter/consist sweeps and deterministic operation limits. Requested floor and explicitly authorized extra work remain separate.

Reservations freeze supported backing and independent Earth/Rock capacity limits. Complete cuts revalidate job/claims/terrain and the external material/funds participant, then commit once. Destroyed recoverable ore is debited from the sole finite reserve separately from spoil cargo. Actual train controllers, wagon cargo, commissioning pockets and exchange hardware belong to Plans 3/4/5; these geological contracts do not implement those producers. Save versions 372/373/374 add geology, sparse jobs and frozen passes respectively. Rendering acceptance remains unchecked until Plan 13 delivers source sprites and packed loading evidence.

Measured Debug evidence: maximum 4096-square rock state uses 8 MiB; representative 512-square through 1024-square maps retain 128 actual geometry jobs. 8,192 synchronized work calls took about 8.8 ms in the retained run, without planning/save/render scans. Timings describe that build and host, rather than runtime thresholds. [Execution notes and evidence](../../upgrades/2026-10/r2mlvs2-b-execution-notes.md) record exact commands, field digests and remaining boundaries.

## Sealed portal authority (R2MLVS3)

[R2MLVS3](../../upgrades/2026-10/r2mlvs3-sealed-portals-and-deep-rail.md) adds one bounded saved work record per Native physical pair in `layer_portal_work.cpp`, keyed by the lower actual endpoint. Sealed, Commissioning and Connected phases do not change reciprocal tile geometry. `GetLayerPortalNextTile` stays the physical neighbour authority; ordinary track/signal/follower queries and actual tile admission recheck phase and the shared date gate. Classic pairs retain implicit Connected compatibility.

Native build proposals validate both prepared plain-ground ends and doors before mutation. A preallocated actual-owner node precedes both housing writes; infrastructure/accounting follows publication. Restricted far housing changes map metadata over retained Solid rock and grants no ordinary construction, excavation, reserve or spoil credit. A separate synchronized `CmdChangeLayerPortalConnection` requires both real excavated, owned compatible approaches and free endpoints. Sealing drains PBS reservations while the old track is still visible, then publishes phase and invalidates both ends. AI/GS rail/road build/connection APIs submit those same commands.

Clicking either Native housing opens canonical status/date, commissioning and project controls. `CommissionLayerPortalTBM` validates actual company, catalogue model/date/depth, assembly floor, shaft reservations, separate spoil capacities and Surface railway origin, then delegates to the real `CommissionTBMProject` acquisition/purchase owner. `CheckLayerPortalWorkAccess` requires the live project/job and exact cutter/support/wagon roles. Ordinary train ticks perform entry/backing across the controlled pocket; paid cuts publish rock/support/cargo/cash atomically. The area tool quotes requested floor versus extra swept space and legal advancing45-degree bends before purchase. The default Underground consist has2Earth/2Rock wagons; Deep has2Earth/4Rock. GUI selects compatible defaults, while the script command accepts explicit valid counts. Source and recurring-depot exchange move actual wagon IDs/packets and require real Surface disposal/empty return.

Empty lifecycle operations preserve packed rock: remove both housings/one record, convert both rail types, or transfer both owners/counters once. Old-owner queued signals flush before transfer. Ownerless retained Native road housing is explicitly Sealed with an invalid company reference, validated separately from assigned/Connected ownership. Actual assigned/occupied recovery and lifecycle refuse unsafe demolition or retain physical cargo for recovery. Selected Sealed/Commissioning/Connected and Deep art is packed and bound; real crossing/zoom/picking/occlusion frames retain technical review, with personal feedback deferred. Current385 saves preserve partial bends, waits, spanning and cancellation; real network joins, fresh bootstrap and tube/thermal final use remain unverified in [the final audit](../../upgrades/2026-10/r2mlvs3mp-z-b-final-closure-audit.md). The portal GUI has no replacement-origin picker even though the synchronized replacement command is tested.

The shared electrical ledger owns useful-power quantities and exact original-source settlement. Actual company takeover/removal retires routes and unused imports, transfers current live lineage or retains inactive tombstones, and prevents company-ID reuse from renewing grants. Version385 persists consumer retirement; actual ordinary-game and native reader/refusal evidence is linked from [the handoff](../../upgrades/2026-10/r2mlvs3mp-x-useful-power-handoff.md). Tube/thermal registry declarations do not establish capsule movement, paid energy procurement or geothermal construction.

Focused Debug evidence and actual loader artifacts are recorded in [execution notes](../../upgrades/2026-10/r2mlvs3-c-execution-notes.md). These tests are distinct from the historical full-regression results below. They do not establish fleet/art/network acceptance.

## Code map

Paths below are relative to the repository root.

| Piece | Where |
| --- | --- |
| Layer enum and limit | `src/map_layer_type.h` |
| Storage, plane axes and coordinate/distance helpers | `src/map_type.h`, `src/map_func.h`, `src/map.cpp` |
| Rules, pairing, initialization and plane iteration | `src/map_layer.h`, `src/map_layer.cpp` |
| Names, icons, dropdown, cycle, view translation and vehicle display layer | `src/map_layer_gui.h`, `src/map_layer_gui.cpp` |
| Main/editor layer controls and split-toolbar Subsidies fallback | `src/toolbar_gui.cpp` |
| Extra viewport and small-map selectors | `src/viewport_gui.cpp`, `src/smallmap_gui.cpp` |
| Drawing, picking, selection, signs and floating text | `src/viewport.cpp` |
| Tile dispatch, initialization and tile loop | `src/landscape.cpp`, `src/genworld.cpp` |
| Ground variants | `src/clear_cmd.cpp`: `GetLayerGroundSprite`, `DrawTile_Clear` |
| Sprite IDs, Action 5 block and generated payload | `src/table/sprites.h`, `src/newgrf/newgrf_act5.cpp`, `media/baseset/openttd/map_layers.nfo`, `map_layers.png` |
| Portal tile accessors and command interface | `src/layer_portal_map.h`, `src/layer_portal_cmd.h` |
| Canonical saved work, commissioning validation and empty connection/lifecycle | `src/layer_portal_work.cpp`, `src/saveload/layer_portal_work_sl.cpp` |
| Native status/connection controls | `src/layer_portal_gui.cpp` |
| Build, clear, draw, track status, enter-tile, visibility and relocation | `src/layer_portal_cmd.cpp` |
| Tunnel/elevator tools, upper-end picker and generated previews | `src/road_gui.cpp`, `src/rail_gui.cpp`, `src/map_layer_gui.cpp`: `DrawLayerPortalPreview` |
| Train movement and both-end rail conversion refresh | `src/train_cmd.cpp`, `src/rail_cmd.cpp` |
| Elevator wait and road movement | `src/roadveh.h`, `src/roadveh_cmd.cpp` |
| Track follower, signals, reservations and YAPF | `src/pathfinder/follow_track.hpp`, `src/signal.cpp`, `src/rail.cpp`, `src/pathfinder/yapf/` |
| Road elevator route cost | `src/pathfinder/yapf/yapf_road.cpp` |
| Layer counts, map chunks, upgrade and vehicle wait | `src/saveload/map_sl.cpp`, `src/saveload/afterload.cpp`, `src/saveload/vehicle_sl.cpp`, `src/saveload/saveload.h` |
| Surface-coordinate camera save | `src/saveload/misc_sl.cpp`: `SaveViewportBeforeSaveGame` |
| Layer unit coverage | `src/tests/map_layer.cpp` |
| Quantization, placement guides and runtime capture | `docs/zz-reach2/upgrades/2026-10/tools/sprite_quantize.py`, `make_sprite_template.py`, `capture_art.ps1` |

## Invariants

- `t == GetTileLayer(t) * Map::PlaneSize() + TilePlaneY(t) * Map::SizeX() + TileX(t)` for every stored tile.
- `Map::Size()` bounds storage; per-plane operations use `Map::PlaneSize()`, `TilePlaneY()` and layer-bounded iteration.
- Ordinary neighbor movement stays in its plane; a portal shaft exit changes layer through `GetLayerPortalNextTile`.
- Commands derive their layer from world tiles. `Viewport::layer` never influences a command, simulation tick or `Random()`.
- Non-Surface initialization never consumes simulation random numbers, preserving same-seed Surface generation.
- Every portal has a permitted pair at the same plane coordinates with reversed shaft directions; paired commands validate both ends before mutation.
- Numeric layer IDs and saved tile indices keep storage order even when display order changes.
- Elevator waiting uses a saved simulation counter, independently of view state or wall-clock delays.

## Verification and remaining work

The final recorded builds were clean in Debug and RelWithDebInfo. Debug passed 111/111 unit cases; the full release CTest run passed 115/115 cases, including four regression suites. The final [acceptance plan](../../upgrades/2026-10/r2mlm10-multi-layer-acceptance.md) and [review](../../upgrades/2026-10/r2rvm2-review-map-2.md) record exact commands and results.

Four generated worlds covering two seeds and square/rectangular sizes matched the immutable pre-layer Surface digests. Their old-save upgrades and paused reloads preserved all ten complete map chunks; exact values are in [final-layer-save-digests.txt](../../upgrades/2026-10/tools/final-layer-save-digests.txt). Version-369 to 370 upgrade retained Surface data and initialized Deep Underground. Saves made from Underground, Deep Underground and Outer Space reloaded onto Surface at the same plane camera position and zoom without changing the map chunks.

The supplied four-plane save supported 63 static captures of its portal ends and floors at three zooms, plus representative train traffic captures. A separate connected-platform/path-signal fixture demonstrated station stops and Surface/Underground round trips; the original save was preserved. The loaded graphics block had all 66 sprites without a missing-extra-sprite warning. A TTD scenario loaded, advanced and round-tripped; actual TTO scenario name-conversion failures reproduced in the pre-epic baseline and remain a separate compatibility gap.

The [Deep Underground plan](../../upgrades/2026-10/r2mlm12-deep-underground-tunnels-and-elevators.md) and [split-toolbar plan](../../upgrades/2026-10/r2mlm11-split-toolbar-subsidies.md) also record their implementation and checks. The toolbar work preserves Subsidies access through the Towns menu when the dedicated button is absent from the visible arrangement, with `F6` retained. [Save-game working notes](../save-game.md) index the retained inspectors, capture/input helpers and corrected save variants for further work.

Plan closure includes explicitly skipped operator/review gates; those skips are not passing tests. Full multiplayer joins, long-running traffic, elevator queues and wait/save scenarios, the complete construction/conversion/reversal matrix, all-direction/scale/blitter graphics review, large-map performance and human art approval remain unverified. Shared Surface-house catchment for Underground stations, further Outer Space gameplay, and the open portal-cost and pause/fast-forward policy decisions remain future work. See the master plan's acceptance limitations and open decisions before treating this delivery as full release acceptance.
