# Native renderer ownership and qualification

Related architecture: [map layers](../../architecture/map-layers/archi-map-layers.md),
[vehicles](../../architecture/vehicles/archi-vehicles.md) and
[Plan3 Groups50-51](m2o23-3-come-space.md#group-50---reconcile-shared-ground-station-and-vehicle-drawing).
Status: Groups50-51 verified; candidate preparation continues in52. Source selection is A for both
Battery Works and Electronics Works, as explicitly chosen by the user.

## Shared conventional consumers retained by50

| Actual native entry | Selected physical owner / selector | Preserved behavior |
| --- | --- | --- |
| industry_cmd.cpp::DrawTile_Industry | GetMultiLayerIndustrySprite → GetGroundMineSprite / GetGroundProcessorSprite | Real funded cells, finite mine status or recipe/input state; aligned back/fronts; native transparency and20-high front sorting box. No donor placeholder replacement. |
| train_cmd.cpp::Train::GetImage | GetMultiLayerTrainSprite → GetGroundTrainSprite for selected models5/6 | Actual cargo label/load, physical direction/flip and train-head motion; model4 and TBM retain their existing branches. |
| roadveh_cmd.cpp::RoadVehicle::GetImage | GetGroundRoadSprite for selected models7/8 | Real packets/capacity, refits, service/traffic state and source phases; native image cache refresh remains derived. |
| GetRailIcon / GetRoadVehIcon | GetMultiLayerEnginePurchaseSprite → GetMultiLayerPurchaseSprite → existing Ground purchase selector | Existing company/engine/date admission determines the cue; viewing art cannot enable purchase. |
| station_cmd.cpp::DrawTile_Station | GetGroundStationTileLayout → GetGroundStationArtLayout | Actual default native geometry, exact sorting parents and relative roof/glass child; custom/Space/Tube owners retain their own paths. |
| viewport.cpp / asteroid_gui.cpp / space_vehicle.cpp / launch_facility_sprite.cpp | Existing Space entity, hull, module and launch facility consumers | A's current visibility, physical click bounds, native EIDS and protected preview/final ranges remain intact. |

The five source-backed conventional Ground implementations and their headers do
not exist in B. B's older shared multilayer selector would remove these A paths;
it is not a compatible geometry fix. A's production consumers therefore remain
unchanged in50. Incoming Tube/geothermal/control branches are the bounded51work,
which must extend these existing hooks while retaining the table above. No broad
multilayer_sprite, station_cmd, vehicle or viewport replacement is permitted.

## Current source callback proof

The three pure C++token/body helpers from the pinned donor activate_m1_art owner
are now available in A's same existing owner. They ignore comments and string/
character literals and require real bounded function definitions. The new
`src/tests/merge_renderer_binding_checks.py` uses those helpers with the existing
Ground dispatch and independently frozen Group8draw contracts. It verifies all21
conventional family chains through actual native bodies, including intermediary
purchase and station layout bridges. No historical binding_qualification/log
receipt is read as current evidence; no new runtime binding catalogue is created.
Negative controls reject comment/literal/declaration-only fake callbacks.

Rebuild with `python -B src/tests/merge_renderer_binding_checks.py`. Its disposable
receipt is `test-cache/merge-assets/group50-renderer-bindings.json`.

## Focused native proof

Five new`[merged-ground-camera]`cases share the small declaration/one-CPP
ground_render_camera_fixture owner. They build actual funded mine/processor,
station and loaded rail/road component worlds, then change real main and extra
viewports through every physical layer and actual hidden/transparent building
options. Actual selector identities, including foreground/native child images,
stay equal. Real packet fragments, accounts, tagged receipts, utility/provider/
Space diagnostics, both RNGs and clock remain unchanged; helper windows close
before fixture world teardown. A component fixture's deliberately narrow cargo
registry is installed only after loading real window language, preserving the
ordinary sorted-cargo invariant rather than weakening it.

The five camera cases pass624assertions. Existing32conventional selector/geometry
cases pass187650assertions, including all14254source tuples, native rail/glass/
road sorting geometry, physical cargo/refit/direction and inactive/foreign refusal.
Four representative Space/orbital/launch cases pass6622assertions. Total:
41cases/194896assertions. These are selector/native geometry checks; actual raster
emission/occlusion and useful driven routes remain the stated Plan4acceptance.

Build only `openttd_ground_consumer_test` for conventional work, and
`openttd_space_art_test` for the retained Space representatives. The existing
surface-supply/Tube targets do not contain these selectors; an initial wrong
target invocation ran zero tests and was rejected, never counted as evidence.
Successful builds are `group50-ground-consumer-build-4.log` and
`group50-space-art-build.log` in`test-cache/merge-assets`. The conventional build
needed explicit Catch and map_layer_gui declarations; no compiler workaround or
source-world repair was used.

Focused conventional tags are`[ground-sprite-contract]`, `[ground-mine-selection]`,
`[ground-processor-selection]`, `[ground-train-selection]`, `[ground-road-selection]`
and`[ground-station-selection]`. Space representatives are named Final Space ranges,
Orbital per-cell foreground bounds, Surface rocket presentation/native picking,
and Native image dispatch genuine rocket poses. Their exact terminal names remain
in the source test files. Cache transcripts are`group50-camera-tests.txt`,
`group50-conventional-tests.txt`and`group50-space-representatives.txt`; the bound
summary is`group50-renderer-verification.json`. No source/canonical art, shared
sprite range or protected Space continuation save/window changes.

## Group51 integration in progress

Pinned donor guideway, facility, capsule, generator, riser and paid conduit selectors now resolve through the existing A Ground families. M1 identities remain unchanged. The Group46 facility animation and physical terminal owner is extended in place. A borrowed connection span supplies derived conduit ports without copying canonical paths. These source changes still require native callback wiring and focused compilation/selection qualification; no current pixel or runtime acceptance is claimed.

## Group51 completion

All26 previously deferred Ground families now reach actual native consumers: five capsule bodies and their purchase images, terminal/service/interchange, both powerhouses, riser, natural vent, both paid conduit systems, five guideway pieces and two controls. The existing Ground dispatch inventory now describes these actual callback chains. The source verifier follows real bodies and intermediate helpers, requires typed family markers and compares every original offset/complete Cartesian source extent against the unchanged independent Group8 fixture. All47 Ground families have native owners; the inventory's existing model4/collector M1 rows also remain intact. No new runtime catalogue or donor M1 enum append was introduced.

Rail, road, station, industry, portal and generic viewport owners insert paid ground branches at their actual floor stage exactly once. A launch rendering and complete conventional station parent/child layouts remain. Original riser opaque backs use native64-high sortable bounds; other original industry fronts retain their existing bounds. Source/path invalidation drops only derived Tube edge caches; paid installation/retirement/takeover dirties five bounded geothermal cells. Group46 remains the sole facility animation/physical terminal owner. No renderer grants work, cash, packets, supply or energy.

The natural vent has its own native tile-aligned overlay entry; the existing small mineral survey icon alone retains its centering offset. Actual armed window/cursor/layer/date admission controls its lifetime. Typed Ground control states leave protected M1 enums unchanged. Tube/facility construction source rows remain reserved artwork: actual instantly built rail/station/depot geometry admits only real ready/working/blocked states, never an invented unfinished simulation owner. Actual paid collector, generator and riser construction selects its native earned stage.

Current range admission remains A's complete `[0,22045)` prefix. `MAP_LAYER_SPRITE_COUNT=GROUND_SPRITE_END`, preview asteroid/probe offsets3588/3597, Space3599/5167 and `SPR_NEWGRFS_BASE` remain fixed. Action5 block0x1C uses that same complete bound. No generated family header, authored source, canonical PNG/NFO/GRF, public manifest or protected save changed.

Focused proof:31cases/65924assertions. Eight separate selector-file invocations cover63native guideway layouts, all native tube facility roles/service/queues,720capsule body poses, locked/available purchase identities, exact external corridor ports and native-cable exclusion, all paid generator access/construction/operation states, riser construction/flow/lifecycle, both conduit domains/all16neighbour masks, and retained collector states. Two additional cases exercise both actual provider commands for all five models and native control/vent cursor lifetime. Five conventional camera cases and four Space/orbital/launch representatives still pass against current binaries. Each child has a110second guard; the entire fresh observation takes under10seconds. No full regression.

Rebuild focused targets `openttd_tube_test`, `openttd_space_art_test`, `openttd_ground_consumer_test`, then run `python -B src/tests/merge_renderer_selection_checks.py` and `python -B src/tests/merge_renderer_binding_checks.py`. Build transcript: `test-cache/merge-assets/group51-build-4.log`. Current executable/source/terminal receipt: `test-cache/merge-assets/group51-renderer-verification.json`; actual-chain receipt remains the existing `group50-renderer-bindings.json` filename and now includes imported families. Every transcript can be rebuilt by the observer; none is a retained source input. Initial observer-only assumptions about native wait labels were corrected to the real initial-versus-replacement contract; production provider semantics were unchanged. Real raster emission, occlusion, driven routes and peer joining remain Plan4.
