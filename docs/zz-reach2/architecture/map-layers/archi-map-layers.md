# Map layers

A live game has four map planes in one `TileIndex` space. `Map` owns tile storage and geometry; `MultiLayerMap` owns layer meaning, initialization, construction rules and portal pairing. A viewport's `layer` selects which plane it draws. Commands and simulation ticks use world tiles, independently of that client selection.

This page retains the delivered layer, [geology/excavation](../../upgrades/2026-10/r2mlvs2-geology-and-excavation.md), physical portal/TBM/spoil and electrical-owner contracts through2026-10-08/save385 and adds the native launch/recovery ownership handoff verified2026-10-14 through423 below. Selected M1 graphics retain the immutable66 original sprites inside the published2,404 bound and complete technical native review. [Final target audit](../../../zz-rhr/upgrades/2026-10/portal-tbm/r2mlvs3mp-z-b-final-closure-audit.md) distinguishes delivered code/evidence from pending network/bootstrap/full-performance/late-consumer/personal-review acceptance. The [level0 overview](../@archi-ttd-level0.md) and [multilayer epic](../../upgrades/2026-10/r2mlm-multi-layered-map.md) retain the original architectural context.

## Layers and rules

Current merged boundary,2026-10-25: the [M2O23handoff](../../upgrades/2026-10/m2o23-4-come-space-b-integration-handoff.md) records delivered face/bay operation, canonical paid supply, all five capsule consumers, geothermal/town/Spacecontention, writer441, real peers and selected native performance/art/recovery. Earlier dated component/pending statements retain their historical scope. [Storage and reconstruction](../archi-historical-debug.md) remain one authority; player camera selection still has no simulation ownership.

`MapLayer` in `src/map_layer_type.h` has immutable values `Surface = 0`, `Underground = 1`, `Space = 2`, `DeepUnderground = 3`, then `End`, with `Invalid = 0xFF`. `MAX_MAP_LAYERS` is 4. Deep Underground was appended after Space so existing saved tile indices retain their meaning. Menu order is separate: Surface, Underground, Deep Underground, Outer Space, defined by `_map_layer_display_order` in `src/map_layer_gui.cpp`.

| Menu order | Layer / storage value | Terrain and permitted construction |
| --- | --- | --- |
| 1 | Surface / 0 | Ordinary generated terrain and the historical construction set, subject to normal command checks. |
| 2 | Underground / 1 | Flat bedrock at height 0: rail, roads, rail stations/waypoints, road stops/waypoints, train/road depots, signals, signs and permitted portals. |
| 3 | Deep Underground / 3 | Classic retains roads, road stops/waypoints, road depots, signs and elevators. Multilayer Industry additionally permits rail/stations/depots/signals and Underground/Deep rail portals, subject to actual excavation and the 1980 calendar gate. |
| 4 | Outer Space / 2 | Flat vacuum with a sparse-star floor; ordinary tile construction remains signs-only. Native paid spacecraft, asteroid and module owners use their separate typed/profile-authorized commands. |

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

[R2SFF1](../../upgrades/2026-10/r2sff1-space-fixes-features-1.md) adds direct
Alt+1/2/3/4 selection of Surface/Underground/Deep Underground/Outer Space in both
toolbars. The shared viewport setter ignores unavailable layers and preserves
the same plane position and unrelated extra views. Display order is mapped
explicitly rather than cast from key digits. Conflicting old Debug defaults
migrate only their exact obsolete chords to Ctrl+Alt+0/1/2; intentional custom
bindings are retained. Small-map following/legend changes are separate pending
work and are not implied by these shortcuts.

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

R2TBMF9 adds a bounded paid terminal-support handover to that same paired constructor. `CheckTBMExitHandover` requires a completed accepted route, compatible single terminal track, its already paid/open pocket, and the entire stopped real TBM clear on the retained approach. After both housings and the existing pair owner publish, `CompleteTBMExitHandover` removes only the replaced support row and adjusts native rail infrastructure. Remaining support and the original geological/haulage identities stay intact. Both Surface-to-Underground and Underground-to-Surface input quote the endpoints, approaches, owner and full portal/support charge. An already excavated B can connect without buying another TBM; actual selected-hangar arrival and final release then permit ordinary traffic. [Native50-cell/input/file evidence](../../../zz-rhr/upgrades/2026-10/tbm-fixes/r2tbmf-y-exit-notes.md#group9-closure) covers both construction directions, public freight both ways, a paid junction and preserved398 files. This introduces no saved fields or new portal pair type.

Each end has a shaft direction and a door facing the opposite way. Its pair has the reverse shaft direction. `GetLayerPortalNextTile(tile, exitdir)` returns the pair for a shaft exit and an ordinary neighbor otherwise. Track followers, signal/reservation traversal, pathfinders and vehicle movement use this shared neighbor contract; the final train-controller correction also obtains its movement destination through it.

`TranslateIntoLayerPortalPair` shifts world Y to the destination band and puts the vehicle onto the pair's entry edge. Controllers maintain the heading and use the destination tile height, allowing different endpoint heights. Layer portals do not use ordinary tunnel wormhole movement. `IsHiddenInLayerPortal` derives visibility from the vehicle's depth along the shaft, using the shared vanilla `_tunnel_visibility_frame`; this applies to each part during entry, exit, turns and reversals.

Signals belong on adjoining normal rail, not portal tiles. Portal rail reservations use `m5` bit 4, as rail tunnels do. Stations and their orders retain ordinary tile/station identities across layers: a train can visit stations on both sides if their platforms actually connect to its route. The supplied save's station/signal report was traced to disconnected platforms; the connected fixture demonstrated stopping and cross-layer travel without a station/pathfinder change for that report.

## Timed road elevators

The front road vehicle stops hidden at the shaft edge for `LAYER_ELEVATOR_WAIT_TICKS`, 370 game ticks, roughly five game days or ten seconds at normal speed. This uses simulation time, not wall-clock time. `RoadVehicle::elevator_wait` is 0 when idle, above 1 while counting down, and 1 when ready to cross. Stopping the vehicle or an approach breakdown pauses the countdown; turning around during a ride is refused.

A blocked destination retains the ready state, so the vehicle does not pay another wait when the exit clears. A committed crossing or permitted turn resets the wait to zero. The vehicle leaves the timed crossing from zero speed; articulated parts follow their front. Saving during the wait stores the counter for later continuation. YAPF includes the portal penalty for road tunnels and elevators and adds `LAYER_ELEVATOR_WAIT_COST`, ten tile costs, for the elevator.

## Surface launch references and orbital recovery

[Plan9](../../upgrades/2026-10/r2mlvs9-launch-centers-and-rockets.md) and its
[current423 runtime handoff](../../../zz-rhr/upgrades/2026-10/r2mlsp-q-runtime-handoff.md)
add explicit paid orbital transport without a Space portal or an ordinary
cross-layer station join. A launch center is a native owned Surface station
facility with protected orthogonal navigable-water access. Its original build
anchor supplies GetLaunchCenterPlaneReference in Q16 plane coordinates. Camera
selection, stacked Y and later joined station geometry cannot change gameplay.

The orbital drop circle and Surface collection circle are independent20-tile
regions. Current spacecraft release checks the explicitly selected owned
center, exact generation, real purchased shell, physical inventory/quota and
inclusive squared-distance circle BEFORE moving packets. A saved descent job
retains the actual off-center release point/target and transfers its sole
packets once. Full/lost targets retain recovery custody for explicit control.
Valid diagonals, one-Q16-unit outside points, map-edge clipping and overlapping
references are checked without wrapping or selecting another center implicitly.

Surface collection links separate same-company rail/lorry Stations with their
whole physical footprint inside the collection area. Native spread/catchment
settings retain normal station geometry; identical XY on Underground supplies
no membership. Timed neutral handling owns packets once, settles incomplete
assignments when native footprint/removal changes invalidate a member, and
refuses deliberate active unlink. Original source, recorded transit periods,
routing, feeder share and horizontal travel remain preserved through real
replay/circulation, with mass fees and no final delivery payment.

Current423 VEHS/native Space/pod/flight/handling records retain exact owners and
once-only receipts; SPFL's existing84-byte row carries its module-delivery flag
in bit4. Derived spatial/viewport state cannot produce progress or cargo. The
handoff names26 exact owner families, actual paid input identities and focused
full/preview/ordinary/replay evidence. This remains a dated423 boundary; the
later438 mining handoff below records its own joined network evidence without
closing unrelated parent/source tasks.

## Generated Space bodies and current mining simulation

[Plan10](../../upgrades/2026-10/r2mlvs10-asteroids-and-mining-probes.md)
and its [current438 runtime handoff](../../../zz-rhr/upgrades/2026-10/r2mlsp-z-z-p-probe-runtime-handoff.md)
keep exact generated bodies, finite reserves, frozen initial access history
and Q16 motion under `AsteroidField`. ASTR and native vehicle/order/packet
records own continuation; SPKN owns company scan receipts. Reloading cannot
reroll or replenish from current density, definitions or camera selection.
Depleted body IDs remain visible, moving and indexed until terminal map exit. Bodies use plane
coordinates; `MultiLayerMap` owns native layer meaning and tile conversion.

Actual body/index steps and attached transforms precede all native probe
decisions. Visible Space and hidden Surface views produce identical paid
mining/return/descent/Surface results. Actual joined loopback peers retain six
matching frame/tick/elapsed checkpoints and55 owner families through recovery;
company access setup grants no physical or financial resource. Plan11's
broader module/manufacturing acceptance remains independently pending.

The current [Space fixes](../../upgrades/2026-10/r2sff1-space-fixes-features-1.md)
replace reflection with signed straight Q16 motion to the actual plane edge.
The historical middle-half generation/static-module placement strip is not the
terminal boundary. Native safety releases a threatened attachment before the
unsafe step, including on a stopped hull; safe existing attachments still follow
their body. Actual field/index publication precedes native craft decisions.
Retirement removes live lookup/index/knowledge/lease rows; cargo may retain its
immutable retired32-bit Source without a tombstone or body-ID reuse.

The original field owns cumulative exits, dedicated Small/Medium/Large weights
1/3/5, pending credit, latest exit size/class, monotonic issuance and independent
timer/candidate RNG streams. First credit arms741..2222 ordinary opportunities
(20..60 normal-speed seconds); pause freezes and fast-forward changes opportunity
frequency. A due attempt creates at most one affordable body of a different
size from the latest exit, debiting only success. Failed attempts retain credit
and rearm; there is no minimum-population refill or catch-up burst.

The original native Mine order chooses LegacyReturn, ReturnNearest or
FullLoadNearest. Automatic selection ranks actual squared distance, then stable
typed identity, among safely interceptable compatible targets. Held Full Load
intent retains cargo and seeks the same mineral; no target/receiver produces a
bounded visible wait. Approach/arrival, whole-batch transfer, paid shells and
per-role service remain original owners. Checked Recall cancels reacquisition
while retaining earned packets. Removing an effective receiver releases its
approach and records TargetRemoved immediately, so a paused save cannot retain
ApproachBusy without a request.

Current [lifecycle peers](../../upgrades/2026-10/r2sff1-z-m-high-id-peers.md)
exercise owning-client policy commands, armed-timer late join and local/reversed
replay with original low-ID and native-produced high-ID cargo. The declared
historical high-ID prefix is separate from earned native birth/extraction.
New [work/storage measurements](../../upgrades/2026-10/r2sff1-z-n-lifecycle-work.md)
count actual events and live container capacities; fixed geometric bucket
headers and allocator-specific heap overhead are distinguished. Diagnostic
counters are disposable and confer no simulation authority. Earlier438 mining
and art receipts above retain their original scope.

Exact population performance fixtures cover10,160,10,240 and14,336 bodies,
clustered drift and64x1024/1024x64 maps. Each body steps and updates its derived
index once; bucket departure uses constant-work swap/pop. Local queries cost
their visited buckets and actual sorted candidate output, including all
14,336 clustered records. Native hull fixtures allocate matching Map geometry,
but initialize only the measured Space layer and claim no whole-save,
maximum-map Surface-generation or whole-game FPS acceptance. Plan10's seven
final Space sprite families passed the [dated packed/native/ordinary review](../../upgrades/2026-10/r2mlvs10-asteroids-and-mining-probes.md):
stable native3599..5166, two identical actual GRF builds, generator-free exact
old/new decoding,95,743 assertions per configuration and34 ordinary captures.
Body/pod rendering reads the original Space spatial index and passive custody;
craft render caches refresh after all late native load owners are accepted.
Neither drawing nor paused reload grants gameplay work or changes an attachment
point. Plan9/11's launch, rocket/hauler/heavy and station source/packed/ordinary presentation now pass their scoped receipts below; broader continuous-chain/user acceptance remains separate.

## Delivered orbital and launch presentation boundaries

[Plan11's current handoff](../../upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md#current-runtime-handoff---2026-10-20) records native contained module construction, manufacturing and shared recipe/utility ownership through438. Actual generation-qualified site/Vehicle/packet state owns work and transfers. Ordinary reloads and copied validation use their own saved owners; camera layer, drawings and GUI queries cannot issue resources, shift a gameplay position or commission a facility.

The [dated scoped launch-art handoff](../../upgrades/2026-10/r2mlvs9-launch-centers-and-rockets.md) records qualification of all7,764distributed decoded sprites, preserving6,116old sprites and adding1,648facility/rocket parts. Offsets6143..7678 own the four-view/twelve-cell launch facility;7679..7790 own112rocket poses; exclusive bound7791 preserves earlier reserved/unactivated gaps. Ten orbital families occupy5167..6142; earlier Space and M0/M1 content retain their fixed bounds and exact pixels/anchors. Its retired log path is historical evidence, not a current physical dependency.

Surface facility drawing uses actual per-zone ground/front pieces and native sort boxes. The near-side foreground cut stays with its own occupied cell, and original role/queue/pad state alone decides which exact Surface hull may draw and be picked. Space free bodies/pods draw only on Space; deployed Orbital selection and non-orbital active-flight selection use their actual typed GUI owner. Ordinary occupied/phase/FIFO/window/deployment/pod captures pass in both configurations. Passive shells stay at their original release point, with no camera-defined descent path; successful native packet recovery consumes the exact shell. Stored shells have no duplicate visible object.

These presentation/save boundaries do not complete Plan11's continuous genuine Surface-funded chain, its network/payment convergence or final personal review. Its latest whole-interval funding refusal and all original inputs remain binding; source-plan/parent completion stays open.

## Generated graphics and drawing

Original-media placement, retained donor variants and the
candidate → independent expectation → native observation → explicit publication
boundary are owned by [Storage ownership](../archi-historical-debug.md#storage-ownership).
`mlm/review-producers.json` is the sole reconstruction recipe catalogue; imported
art does not create another supplier or change gameplay ownership. Numeric
delivery snapshots below are historical; live generated headers and packing
contracts govern the current ranges during M2O23-1 integration.

OpenTTD's extra GRF contains an Action 5 type `0x1C` block beginning at `SPR_MAP_LAYER_BASE`. `MAP_LAYER_SPRITE_COUNT` now follows `LAUNCH_FLEET_SPRITE_END` (7,791); the historical M1 boundary remains2,404; `SPR_NEWGRFS_BASE` follows the published bound. Original offsets0..65 remain immutable and66..73 stay reserved/unactivated. Selected delivered core segments preserve planned74..199; vehicle/portal/facility/UI/cargo families append after199 in stable manifest order. The original sheet is retained separately from selected M1 PNG/NFO. [media/multilayer/tools/sprite_quantize.py](../../../../media/multilayer/tools/sprite_quantize.py) retains the DOS palette/native anchor contract and original44 recipe; [M1 packing and technical review](../../../zz-rhr/upgrades/2026-10/portal-tbm/r2mlvs3mp-t-gallery-acceptance.md) verifies forty selected families/2,311 sprites,2,377 actually loaded sprites and the checked-in generator-free payload.

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

[R2MLVS1 Content Freeze](../../../zz-rhr/upgrades/2026-10/r2mlvs1-g-content-freeze.md) adds explicit Classic/Multilayer selection and saved profile metadata in version 371. The shared policy owner is `multilayer_content.cpp`; legacy files without metadata retain Classic and their existing content behavior. Layer storage IDs, addressing and the 66-sprite native block remain unchanged. Metadata acceptance checks exact first-party content and cargo meanings before dependent gameplay fixups, with independent nonmutating preview fields; see [savegame architecture](../savegame/archi-savegame.md).

`CheckDeepLayerAvailability` publishes a game-calendar boundary of 1 January 1980 for the new profile, with explicit saved editor override and legacy policy. R2MLVS2 uses it in excavation construction, job reservation/work and cut checks. R2MLVS3 uses it for actual Deep rail/elevator build, connection, routing and movement. Actual TBMs, tube networks, orbital movement and final excavation/portal art remain in the owning [R2MLVS feature plans](../../upgrades/2026-10/r2mlvs-multilayer-vehicles-and-structures.md).

## Geology and excavation (R2MLVS2)

R2TBMF10 adds an immutable regional hardness rule for newly generated worlds, independently of the frozen ore/vent stream. The saved U16 algorithm/U64 seed drive bounded queries of nine jittered centres on24-tile spacing; there is no per-cell hardness allocation or rendering RNG. Saved deposit hardness overrides host rock. Region version0 preserves the former Soft Underground/Hard Deep defaults for all existing maps. `GetExcavationCellHardness` owns the answer consumed by cuts, estimates, survey and native material art; exposed walls read the adjacent uncut material, while mineral overlays retain resource identity. Save399 persists the rule without changing old terrain, work or cargo. [Focused and native evidence](../../../zz-rhr/upgrades/2026-10/tbm-fixes/r2tbmf-z-geology-notes.md) includes actual old/new file replays and a Standard cutter physically retreating from generated Basalt.

The explicit Multilayer Industry profile generates independent two-bit Solid/Excavating/Excavated terrain for both rock planes, finite mineral seams and a separate bounded geothermal field. `Map` retains infrastructure/geometry ownership. Clearing a road, rail or building leaves the geological cavity and remaining reserve unchanged. Classic and predecessor saves without geology retain their historical open-floor policy.

`excavation.h` owns typed deposit/job/vent IDs, canonical geometry and public guard/transaction contracts. Ordinary construction checks complete foundations and approaches before writes; viewport selection grants no exception. Job claims are reconstructed from bounded unfinished routes. A pure planner expands actual straight/left-45/right-45 advances with full cutter/consist sweeps and deterministic operation limits. Requested floor and explicitly authorized extra work remain separate.

Reservations freeze supported backing and independent Earth/Rock capacity limits. Complete cuts revalidate job/claims/terrain and the external material/funds participant, then commit once. Destroyed recoverable ore is debited from the sole finite reserve separately from spoil cargo. Actual train controllers, wagon cargo, commissioning pockets and exchange hardware belong to Plans 3/4/5; these geological contracts do not implement those producers. Save versions 372/373/374 add geology, sparse jobs and frozen passes respectively. Rendering acceptance remains unchecked until Plan 13 delivers source sprites and packed loading evidence.

Measured Debug evidence: maximum 4096-square rock state uses 8 MiB; representative 512-square through 1024-square maps retain 128 actual geometry jobs. 8,192 synchronized work calls took about 8.8 ms in the retained run, without planning/save/render scans. Timings describe that build and host, rather than runtime thresholds. [Execution notes and evidence](../../../zz-rhr/upgrades/2026-10/r2mlvs2-b-execution-notes.md) record exact commands, field digests and remaining boundaries.

## Sealed portal authority (R2MLVS3)

[R2MLVS3](../../upgrades/2026-10/r2mlvs3-sealed-portals-and-deep-rail.md) adds one bounded saved work record per Native physical pair in `layer_portal_work.cpp`, keyed by the lower actual endpoint. Sealed, Commissioning and Connected phases do not change reciprocal tile geometry. `GetLayerPortalNextTile` stays the physical neighbour authority; ordinary track/signal/follower queries and actual tile admission recheck phase and the shared date gate. Classic pairs retain implicit Connected compatibility.

Native build proposals validate both prepared plain-ground ends and doors before mutation. A preallocated actual-owner node precedes both housing writes; infrastructure/accounting follows publication. Restricted far housing changes map metadata over retained Solid rock and grants no ordinary construction, excavation, reserve or spoil credit. A separate synchronized `CmdChangeLayerPortalConnection` requires both real excavated, owned compatible approaches and free endpoints. Sealing drains PBS reservations while the old track is still visible, then publishes phase and invalidates both ends. AI/GS rail/road build/connection APIs submit those same commands.

Clicking either Native housing opens canonical status/date, commissioning and project controls. `CommissionLayerPortalTBM` validates actual company, catalogue model/date/depth, assembly floor, shaft reservations, separate spoil capacities and Surface railway origin, then delegates to the real `CommissionTBMProject` acquisition/purchase owner. `CheckLayerPortalWorkAccess` requires the live project/job and exact cutter/support/wagon roles. Ordinary train ticks perform entry/backing across the controlled pocket; paid cuts publish rock/support/cargo/cash atomically. The area tool quotes requested floor versus extra swept space and legal advancing45-degree bends before purchase. The default Underground consist has2Earth/2Rock wagons; Deep has2Earth/4Rock. GUI selects compatible defaults, while the script command accepts explicit valid counts. Source and recurring-depot exchange move actual wagon IDs/packets and require real Surface disposal/empty return.

Empty lifecycle operations preserve packed rock: remove both housings/one record, convert both rail types, or transfer both owners/counters once. Old-owner queued signals flush before transfer. Ownerless retained Native road housing is explicitly Sealed with an invalid company reference, validated separately from assigned/Connected ownership. Actual assigned/occupied recovery and lifecycle refuse unsafe demolition or retain physical cargo for recovery. Selected Sealed/Commissioning/Connected and Deep art is packed and bound; real crossing/zoom/picking/occlusion frames retain technical review, with personal feedback deferred. Current385 saves preserve partial bends, waits, spanning and cancellation; real network joins, fresh bootstrap and tube/thermal final use remain unverified in [the final audit](../../../zz-rhr/upgrades/2026-10/portal-tbm/r2mlvs3mp-z-b-final-closure-audit.md). The portal GUI has no replacement-origin picker even though the synchronized replacement command is tested.

The shared electrical ledger owns useful-power quantities and exact original-source settlement. Actual company takeover/removal retires routes and unused imports, transfers current live lineage or retains inactive tombstones, and prevents company-ID reuse from renewing grants. Version385 persists consumer retirement; actual ordinary-game and native reader/refusal evidence is linked from [the handoff](../../../zz-rhr/upgrades/2026-10/portal-tbm/r2mlvs3mp-x-useful-power-handoff.md). Tube/thermal registry declarations do not establish capsule movement, paid energy procurement or geothermal construction.

Focused Debug evidence and actual loader artifacts are recorded in [execution notes](../../../zz-rhr/upgrades/2026-10/r2mlvs3-c-execution-notes.md). These tests are distinct from the historical full-regression results below. They do not establish fleet/art/network acceptance.

## Code map

Final merged joins retain their existing owners:

| Owner | Responsibility |
| --- | --- |
| [`surface_supply_order.cpp`](../../../../src/surface_supply_order.cpp), [`surface_supply_merged_wire.cpp`](../../../../src/surface_supply_merged_wire.cpp) | One tagged Native/LegacyBreceipt book, full native custody, Pending-only new refunds and finite inherited rights. |
| [`tbm_exchange.cpp`](../../../../src/tbm_exchange.cpp) | Physical face/bay admission and whole native transfer; job/train/cargo ownership stays with the original pools. |
| [`transport_tube_fleet.cpp`](../../../../src/transport_tube_fleet.cpp), [`transport_tube_readiness.cpp`](../../../../src/transport_tube_readiness.cpp) | Existing capsule/fleet/provider roles and company-scoped terminal readiness, including dedicated cold loading. |
| [`utility_power.cpp`](../../../../src/utility_power.cpp) | Existing common interval allocation/settlement with geothermal source publication and all Space/ground consumers; no parallel scheduler. |
| [`multilayer_state.cpp`](../../../../src/multilayer_state.cpp) | Complete128-family canonical comparison used by save/reload and matched real peers. |
| [`ground_sprite_assets.h`](../../../../src/table/ground_sprite_assets.h) | Exclusive native Groundbound22045; preserved M1/preview/Space/orbital/launch prefix and one public packing manifest. |


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
| Quantization, placement guides and runtime capture | `media/multilayer/tools/sprite_quantize.py`, `media/multilayer/tools/make_sprite_template.py`, `media/multilayer/tools/capture_art.ps1` |

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

The final recorded builds were clean in Debug and RelWithDebInfo. Debug passed 111/111 unit cases; the full release CTest run passed 115/115 cases, including four regression suites. The final [acceptance plan](../../upgrades/2026-10/r2mlm10-multi-layer-acceptance.md) and [review](../../upgrades/2026-10/map-layers/r2rvm2-review-map-2.md) record exact commands and results.

Four generated worlds covering two seeds and square/rectangular sizes matched the immutable pre-layer Surface digests. Their old-save upgrades and paused reloads preserved all ten complete map chunks; exact values are in [final-layer-save-digests.txt](../../../zz-rhr/upgrades/2026-10/r2fc-f-retained-records.md#final-layer-save-digests). Version-369 to 370 upgrade retained Surface data and initialized Deep Underground. Saves made from Underground, Deep Underground and Outer Space reloaded onto Surface at the same plane camera position and zoom without changing the map chunks.

The supplied four-plane save supported 63 static captures of its portal ends and floors at three zooms, plus representative train traffic captures. A separate connected-platform/path-signal fixture demonstrated station stops and Surface/Underground round trips; the original save was preserved. The loaded graphics block had all 66 sprites without a missing-extra-sprite warning. A TTD scenario loaded, advanced and round-tripped; actual TTO scenario name-conversion failures reproduced in the pre-epic baseline and remain a separate compatibility gap.

The [Deep Underground plan](../../upgrades/2026-10/r2mlm12-deep-underground-tunnels-and-elevators.md) and [split-toolbar plan](../../upgrades/2026-10/r2mlm11-split-toolbar-subsidies.md) also record their implementation and checks. The toolbar work preserves Subsidies access through the Towns menu when the dedicated button is absent from the visible arrangement, with `F6` retained. [Save-game working notes](../save-game.md) index the retained inspectors, capture/input helpers and corrected save variants for further work.

Plan closure includes explicitly skipped operator/review gates; those skips are not passing tests. Full multiplayer joins, long-running traffic, elevator queues and wait/save scenarios, the complete construction/conversion/reversal matrix, all-direction/scale/blitter graphics review, large-map performance and human art approval remain unverified. Shared Surface-house catchment for Underground stations, further Outer Space gameplay, and the open portal-cost and pause/fast-forward policy decisions remain future work. See the master plan's acceptance limitations and open decisions before treating this delivery as full release acceptance.

## Shared physical cargo admission (M2O23 integration)

[`VisitNativeCargoPackets`](../../../../src/cargo_custody.h) is the full read-only native owner traversal. Station goods, every vehicle (including reserved cargo), launch handling, spacecraft supply compartments, pod descent, orbital construction, module stock, both service berths and all active manufacturing inputs retain their actual packet lists. A station ID is supplied only for an actual goods queue; job location and packet-pool allocation cannot manufacture source custody. A's asteroid-specific admission consumes this traversal while retaining its natural-source rules. `InsertOrbitalPacketOwners` remains a bounded subset helper, never a second full traversal.

[`SurfaceSupplyCustodyCheck`](../../../../src/surface_supply_order.h) consumes that native ownership evidence and original packet scalars against the sole canonical receipt book. A complete final allocated-row pass detects paid orphans and missing observations; it does not assign ownership. Exact links, actual container cargo labels, unique references and original refunded/dispatched limits constrain legacy stock. `StageLoadedSurfaceSupplyOrders`/`AcceptLoadedSurfaceSupplyOrders` join independent DATE/MAPS evidence before whole-book publication. Ordinary orphan/source rules remain with existing native owners. Future copied adapters must use the same checker with their own candidate rows and references, never live-world substitutions.

[`The implementation handoff`](../../upgrades/2026-10/m2o23-1-come-space-h-cargo-custody.md) names the physical fields and component proof boundary. Receipt restoration is not a purchase or dispatch; actual Load/Reserve/Hub transfers record first departure once, and returned legacy stock cannot regain source eligibility. New purchases keep A's Pending-only refund rule. Group15 refunds only the recaptured exact unshipped legacy source count at its saved tariff through the native command kernel, with wallet overflow and stale requests refused before removal. Group16 retains the entire legacy prefix for the world lifetime and counts its original quantity against the original purchase-day cap, including refunded rows. Actual takeover carries that history to the successor even above its new-admission limit. Berth removal or movement of the same owned Surface sign retains original stock rights; source deletion or company loss closes them without credit before identity reuse. Native Pending settlement and launch/hub lifecycle hooks remain unchanged. Purchased native and migrated packets share permanent non-paying purpose through rating, reservations, hub moves and accepted delivery statistics. Partial Deliver-to-Transfer reassignment preserves the exact existing prefix and splits only the requested boundary; unavailable packet capacity refuses before route/action mutation. Tube transfer capacity has one owner in `transport_tube_cargo`:1024 units per actual shared station/cargo, including native reservations and (at staging) pending arrivals. The exact-destination adapter applies the same calculation only to that station's real receiving list; zero room and non-applicability are distinct. Original B file decoding, merged writing and genuine saved-world acceptance remain later merge gates; these component contracts do not reopen protected Space continuation work. Test tooling remains beside tests and rebuildable diagnostics under `test-cache/merge-supply/`, following [storage ownership](../archi-historical-debug.md#storage-ownership).

## Unfinished geological work replacement

Plan2 Group23C adds `ReplayExcavationRoute`, `PlanExcavationJobReplacement` and
`ReplaceExcavationJobRemainder` to the original excavation owners. Completed cuts,
paid cost, native cargo and finite deposits remain unchanged. Only unfinished
claims are replaced; released unopened cells become Solid, while open floor never
reverts. The first new cut retains partial work only if its complete geometry is
identical. Current actual load seeds new exchange breakpoints; no second stock or
geology owner is created. Own unfinished claims remain chargeable rock through
`QuoteExcavationCells`; foreign jobs' entire swept clearance remains protected.
Six focused native cases pass46224 assertions, including exact original cargo,
cash/RNG and deposit conservation plus rebuilt-claim equality. Group23CB now connects bounded approaches and exact tunnel endpoints to native commission/extension quotes, with shared route-support and occupied-face admission. Missing permanent pieces, incompatible infrastructure and actual train blockers refuse before acceptance. Eleven focused cases pass220603 assertions. Group23CC now appends complete native command212, `ChangeTBMWork`, with a pure live quote and shared attached-spoil measurement. Revalidation binds completed cursor, geometry and participants; partial ticks remain valid, while another cut or changed work refuses. Existing phases and paid cargo remain intact; changed occupied face pieces refuse. Fifteen focused command/remainder/face-support cases pass121352 assertions. Group23D makes explicit face returns back along paid support and keeps final settlement blocked by a hauler anywhere in the corridor/pair. Native face/bay park, cancel, hangar and selected service/rejoin checks pass4 cases/142198 assertions. Claims survive a cancellation request and retire only after physical withdrawal and final disposal. Group23E proves the driven face wait survives work replacement and the original wreck/sale/occupied-exit hooks preserve native custody; its shared lifecycle/regression burst passes6 cases/213783 assertions. Group23EB adapts explicit finite harder-rock fixtures for genuinely recurring exchanges and verifies exact frozen counts, whole-project cost, completed depot storage and a fully reserved native face loop:10 focused cases/316134 assertions. Group23EC writes a fresh genuinely occupied FIFO file and runs two full native loads to the same next legal exchange, preserving engine mappings, physical vehicles, packets, work and cost owners while the waiting peer retains its count. The generated disk fixture preserves actual configured cargo dimensions and the normal default Space catalogue entry. Three isolated cases pass20272 assertions; both continuations exchange after2 actual frames and have identical resulting owners. No phase20 save dialect is activated. UI integration remains24.

## Merge face observations and activation boundary

The four-plan integration route keeps completed Groups1-22D in [Plan1](../../upgrades/2026-10/m2o23-1-come-space.md), remaining domain/phase/scheduler Groups23-36 in [Plan2](../../upgrades/2026-10/m2o23-2-come-space.md), save/assets Groups37-53 in [Plan3](../../upgrades/2026-10/m2o23-3-come-space.md), and reconstruction/qualification Groups54-72 in [Plan4](../../upgrades/2026-10/m2o23-4-come-space.md). These file boundaries do not reassign native owners or expand any acceptance claim.

[The merge foundation](../../upgrades/2026-10/m2o23-1-come-space.md) Groups20/20B/20C/20D now provide pure face observations through `tbm.h` and `tbm_exchange.h`. `GetLiveTBMCutter`/`GetLiveTBMJob` retain their original independent reference rules; `FindTBMProjectReferenceLoss` remains the complete owner/refusal boundary. `IsTBMHaulerInCorridor`/`FindTBMHaulerInCorridor` read actual assigned running parts, including either portal end, excluding stored magazine/depot parts; the lowest actual vehicle ID wins. `GetTBMHaulerRoomCells` reads the longest native assigned consist, with no saved meeting/length cache.

`FindTBMFaceStand` derives the stand two path cells behind the actual cutter tail through the existing paid work path. Every actual part must lie in its bounded tail window; physical same-layer body/safety clearance and next-cut sweep exclusion override a merely convenient path index. A folded prefix needs a real directed approach through owned permitted corridor support, using the native railway follower and current sharp-turn rule. `GetTBMFaceStandTrackdirs` returns actual laid pieces approaching the cutter. No viewport controls these decisions.

`GetTBMMeetingKind` permits Face only for a live Source-magazine project; a real underground-depot bay retains Bay. Working phases allow current/previous approach. A returning cutter requires actual due spoil and its exact face, except an already-inside assigned hauler can hold the previous approach. Empty returns never invite another hauler. The tail must leave at least `GetTBMHaulerRoomCells + 2` cells. `IsTBMReadyForFaceTransfer` adds wait/return phase and actual exchange-due evidence; it does not replace later stand orientation, dwell, whole-load capacity or execution checks.

Current focused proof covers original/appended phase identities, native reference/corridor observations, straight/corner/folded stands, invalid references, Source versus depot, held/empty returns and exact Standard/Deep clearance. The Deep case acquires actual model1 machinery,10 parts/native length80, with a5-cell hauler and16 earned cuts; declared pose/emptying/initial-floor components are not driven-route acceptance. Rebuild `openttd_tube_test` and run only the active tags `[tbm-merge-observation]`, `[tbm-face-stand]`, `[tbm-face-meeting]` or `[tbm-face-deep]`, with compact output beneath `test-cache/merge-supply/`.

`AwaitFaceTransfer` is appended as identity20; original0-19 are unchanged. Plan2 Group23 now activates its native validation and phase transitions: the actual due cut chooses Face/Bay, transfer releases the cutter through any required support advance, and a lost stand returns through existing bay/withdrawal holds. The focused phase/whole-transfer burst passes6 cases/161570 assertions, including actual paid-cut entry and earned dwell. Whole transfer preflight/commit remains the21/21B owner; movement/portal/PBS remains22-22D. Group23B now proves the full Standard driven face/disposal/empty-return loop and frame-by-frame equality under actual native visible/hidden viewport selection. Its5-case focused burst, including affected existing loop cases, passes494099 assertions. Native camera positioning uses clipped refresh with mock sprite headers; this is simulation/camera proof, not raster or human visual acceptance. Original-file/merged-writer compatibility remains Plan3. No protected Space continuation or provisional Tube fleet is promoted by this component handoff.

The existing `tbm_exchange` owner now exposes `DescribeTBMFaceTransfer`, `ValidateTBMFaceTransfer` and `ExecuteTBMFaceTransfer`, with the retained74-tick dwell. Pure admission requires a live loaded encounter, actual stand direction, intact stationary trains, whole Keep-only Earth/Rock stock, the hauler's actual matching empty set and conservative native split room. A refusal has no partial movement plan. Commit prepares destination vectors before shifting original packets, refreshes both native weight/acceleration/view owners, then updates the existing set/assignment once. Original wagon chains remain exact. Focused preflight/commit cases pass including a real boundary split with exact feeder shares, pool-limit/dwell refusal and empty retry. Until23 activates the new phase, these native components use an admitted loaded return and grant no installed face-wait or driven-cycle acceptance.

Groups22-22D connect the same native observations to movement. `IsTBMFaceTurnHolder` selects the lowest eligible assignment with whole capacity and no other running hauler in the corridor. `IsTBMHaulerAdmittedThroughPortal` and `IsTBMHaulerFaceStand` own exact pair/stand admission. `GetTBMHaulerSteeringTarget` returns a derived tile/trackdir pair for empty return; loaded/stored/finished legs keep ordinary orders. Native YAPF recognizes both cutter and hauler work endpoints but retains the cutter-only signal exception. Assigned routes observe current body clearance without a shared stale segment cache.

`GetLayerPortalMagazineExit` preserves whole-chain ingress/departure orientation. `MayTBMSupportCarry` requires native owner/class/permission, own or released support and actual cutter clearance. `IsLayerPortalOpenToNetwork` is shared by ordinary track status and signal blocks; commissioning ends keep separate local blocks while actual shaft movement updates each end. PBS uses only the exact admitted storage/stand direction. Return and post-transfer cutter holds protect departing haulers; a far exit cannot replace support while one remains inside.

The existing `ObserveTBMHaulerInputs`/`StepTBMHaulerPhase`/`AdvanceTBMHauler` cycle now chooses complete face transfer or bay exchange, records the original origin visit and returns the actual emptied set. `BackUpTBMHauler` owns native braking/reversal and refuses any still-stored follower, independently of its caller. Lost-route recovery uses native two-end YAPF comparison only for an assigned running corridor leg; stored stale flags and a temporarily busy Source are not broken-line evidence. Focused headless components cover one transfer/turn, unchanged original orders/chains/provenance, capacity refusal, empty cutter clearance, partial storage and pending braking. Native access and signal cases also drive actual face path discovery and closed-pair local block changes. These component results do not certify installed face-wait operation, a complete driven disposal loop or visible/hidden continuation equality;23 must supply those proofs before the feature is declared integrated. Rebuild the selected `openttd_tube_test` target and run `[tbm-face-cycle]`, `[tbm-face-access]`, `[tbm-portal-signals]`, `[tbm-face-steering]` or `[tbm-face-turn]` only as relevant; reports remain reproducible disposable `test-cache/merge-supply/` output.


## Cold generator machinery and retained history

Merge25 installs the canonical dated Vent-Tap/Magma native definitions and paid
whole-building construction. `geothermal_generator.h` owns original site,
Industry, controller and paid-work history; `Industry::accepted` alone owns
arrived materials. Funding213 consumes an active cold reservation and settles
capital once. Planning214 and cancellation215 reserve/retire geometry without
machinery, materials or energy. Cancelled/consumed site IDs remain historical;
demolition never restores a consumed bill, capital or natural geology.

The26 implementation extends native demolition/takeover and adds detached
construction validation; its focused9-case/948-assertion burst passes. Removal refuses
stored cargo, takeover preserves work, and abandonment suspends further work.
`CaptureGeothermalGenerators`, `ValidateGeothermalGenerators`,
`StageLoadedGeothermalGenerators` and `AcceptLoadedGeothermalGenerators` transfer
the complete family without repair or a second inventory. Existing live bindings
also protect site/controller lineage when an Industry pool ID is reused.
`GetGeothermalGeneratorPort` and `HasGeothermalGeneratorInstalledPorts` inspect
the original declared boundary and separately paid Heat/electrical cells; neither
query installs a link or allocates an account.

This cold handoff issues no generator utility identities. Groups27-30 retain
atomic conversion, first account acquisition, coupled retirement, physical
service and scheduling. Group29 must extend the construction lifecycle/staging
checks to those actual issued utility roles. Plan3 retains original-writer
dialects, same-file decoding, chunk registration, copied previews and real save
round-trips; detached component staging grants none of that acceptance. Current
focused build/test recipes and disposable `test-cache/merge-supply/` reports are
recorded in [the merge execution record](../../upgrades/2026-10/m2o23-2-come-space-a-execution.md).


Merge27 appends the shared conversion-only source role and original Heat-input,
ratio, ceiling, percent carry and bounded lifetime-source attribution to the
existing utility producer. `RegisterUtilityHeatConversionProducer` creates no
energy. `PrepareUtilityHeatConversion` acquires a complete detached validated
after-state and returns a pure field-wise sealed request/result. Ordinary rate
publication and standalone consumption of a bound input cannot bypass the
coupled settlement. The13-case/585-assertion draft/typed-Heat burst passes.
Group28 adds `CommitUtilityHeatConversion`: exact sequence/seal/results are
rederived before one complete participant invoice, followed only by a validated
no-throw state move. Its14-case/603-assertion transaction burst passes, including
fractional carry, mixed original sources, stale/repeated/refused proposals and
recursive mutation. Merge29 adds actual native generator registration and service through the routed
physical Heat facade, using ready funded machinery, separately purchased ports
and cardinal paths. Once-issued partial input/output identities survive retries;
whole native service/carry commits with the shared conversion. Demolition retires
both original utility roles while retaining consumed history, and detached native
validation checks the original model/carry/roles. The operating13-case/6667-assertion
burst and separate Magma329-assertion case pass. These use explicit native component
opportunities; the production scheduler remains with34-35, local-feed/Tube retirement
notifications with their30/33 owners, and current-writer persistence with Plan3.


Merge30 implements paid same-layer native electrical feeds and explicit geothermal
town connections through the shared utility owner. A local feed names the original
completed generator, its original output reservation, purchased cardinal cable,
and an actual owned Tube depot/platform. The attachment has no independent energy
or watt budget. Town command216 selects actual generator, ordered one/two paid
risers, occupied Surface terminal and native town; one whole candidate either
publishes all reference identities or refuses unchanged. Recovery preserves the
original town account and unchanged route edges, expires only unused superseded
rights, and retains spent flow. Missing/foreign terminals and demolished original
machinery revoke eligibility before native pool reuse. Custom GS growth suppresses
native requests and effects. These APIs do not own another balance or tick driver;
production scheduling remains34-35 and Tube provider lifecycle remains33. Focused
component results and rebuild commands are recorded in the merge execution record;
Plan3 retains current-file transport and copied-preview acceptance.

The30focused component burst passes18cases/22878assertions, including original
source replacement with live unused grants, terminal ownership loss, custom GS
suspension and pure recovery isolation. The native sign-index rebuild is fixture
cleanup; no production viewport authority is introduced.


Merge31 consolidates all five native capsule models under `TubeVehicleDefinition`,
`GetTubeVehicleDefinition` and the single `InitialiseTubeEngineDefinitions`
installer. Geometry/energy/cargo/package/mapping/pool checks precede both saved-slot
reconstruction and fresh allocation; model9/local0x8009is unchanged. NewGRF polishing
calls `InitialiseTubeContentDefinitions` independently of saved Tube object presence.
A prototype-era load may append four definition-only EIDS rows while retaining
original IDs, packets and RNG. The old provisional tuning files remain dated,
uncompiled evidence. The focused11-case/1140-assertion burst passes. Group32and42
retain loaded-capacity and actual legacy-file checks: selected model13capacity80
cannot discard an original96-unit hull's packets or retained native capacity.


Merge32 routes capsule dates/layers/labels through the existing logical capability
policy and preflights retained packets/reservations against the operation's target
capacity before refit/renewal preview or temporary allocation. Native cache passes
preserve an existing greater capsule `cargo_cap` while loaded; non-capacity passes
preserve its saved field even empty. Empty refit uses the delivered definition.
Canonical startup/monthly/saved Engine publication has no ordinary preview-year
offset and preserves unrelated history. Real capsule chains cannot substitute for
the TBM's mandatory conventional wagon/Surface-origin hauler, including staged
assignments. The focused10-case/1865-assertion burst passes; declared96-unit native
field/packet inputs are component proof, with actual historical-file acceptance42.


Merge33 adds `transport_tube_geothermal` as a bounded reference owner for original
native generator/output, one shared forwarding root, paid cable and actual inlet
incarnation. Connection/closure create no electrical publication or second balance.
Original generator/inlet loss revokes permission before native reuse; company
ownership traverses only retained resource components anchored by current live
rights, preserving connected closed history and excluding unrelated old CompanyID
tombstones. The source/ownership11-case/4588-assertion burst passes. Provider bindings
and six complete command bodies remain33B; actual file/candidate preview remains
Plan3 and the sole production scheduler remains34-35.


Merge33B adds original geothermal source/feed fields to native capsule bindings,
whole bind/switch preflight, original-inlet guideway components and scoped retirement.
One saved demand fraction moves to the new binding; historical bindings relinquish
it. Geothermal work uses original imported energy and native service already paid
at conversion; external supply retains its lease/tariff owner. Six complete typed
commands append217-222with End223. External legacy binding digests remain unchanged
when new fields are absent. The focused17-case/6389-assertion burst passes, including
loaded packet preservation, real original service/history, unused rights, both
switch directions, same-output distinct inlets and native stop flags. Explicit
component source/allocation opportunities and declared saved fraction input leave
the sole production scheduler/driven proof with34-35and actual file dialects with
Plan3. No second energy, inventory or tick owner is introduced.


Merge34 integrates source publication in the existing `PrepareTubePowerTick`,
borrowing optional complete physical Heat demands and running the original
`RunGeothermalGenerators` before electrical allocation. The placeholder empty Heat
closure is removed; the generator owner alone closes Heat. The sole guard includes
retained plants and town subscriptions for source-loss expiry. Portable Core work,
original tick/pause/mode guards and all four Space appenders remain. The focused
6-case/1227-assertion publication burst passes, including both real plants without
capsules, duplicate/refused rotation and native orbital-only publication without
Heat.35retains forwarding demand/import/town settlement and combined driven proof.


Merge35 completes the existing common demand/settlement pass: actual capsule
phase/load demand, one capped root request per geothermal inlet association,
town plus all four Space appenders, one electrical allocation, whole original
path imports sorted by destination ConsumerID, then native town effects.
AdvanceTubePowerTick authorizes native movement/repair exactly once; the original
VehicleServiceInDepot now accepts that native authorization as well as the paid
external participant. Automatic unpaid repair remains refused. Actual mixed-world
contention verifies Vent source shortage, Magma shared cable capacity, native
repair/manual stops, town25% benefit, Assembly work and fractional processor work,
with exact accounting/cash/work equality after reversing valid binding rows.
Construction floor, receiver packets, consumed orbital frames and nonzero unit
price baselines are disclosed inputs; this does not certify native save dialects,
paid bootstrap, protected Space progression or human raster acceptance.


Merge36 normalizes fully validated external-service and capsule-binding transports
by original producer/consumer identity before acceptance; captures and explicit
field digests never treat table insertion order as synchronized authority.
Generator maps, conversion origins, original provider references and utility
lineage retain their existing canonical owners. Portal/company events no longer
expire every unrelated Tube reservation: exact source retirement and actual tile
notifications close only affected descendants. Integrated native cable repair,
town pair/generator loss, takeover/slot reuse and current-interval budget exhaustion
preserve consumed history and native custody. Natural vents remain renewable.
A128-interval actual Deep/Space camera replay matches every original accounting,
work, native pose, cargo, cash and RNG frame; clipped unit drawing provides no
raster qualification. Plan2handoff/public interfaces and focused rebuild commands
are recorded in m2o23-2-come-space-a-execution.md. File dialects/writers and original
saved-world activation remain Plan3; publication/final qualification remain their
later gates. Protected original media, source epochs and Space seals stay intact.
