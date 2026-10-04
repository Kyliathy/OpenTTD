# R2MLVS8 - Space World and Construction

Status: proposed. Parent: [R2MLVS master](r2mlvs-multilayer-vehicles-and-structures.md). Depends on r2mlvs1 and r2mlvs14 Group 1.

## Outcome and reading

Provide deterministic Space entities, vehicle control and construction semantics without treating vacuum as ordinary land or asteroids as moving map industries. Read master AD-8/9 and I-1/4/5/8/9; [map layers](../../architecture/map-layers/archi-map-layers.md), [vehicles](../../architecture/vehicles/archi-vehicles.md), [viewport](../../architecture/gui/archi-viewport.md), `src/vehicle_type.h`, engine/company/vehicle tables, `src/map_layer_gui.cpp` and vehicle saveload.

## Group 1 - Vehicle model and compatibility spike

- [ ] 1.1 Inventory assumptions of exactly four company transport modes: saved enum values, engine types, expense arrays, vehicle lists/groups, orders, depot handling, ownership, CargoDist, scripts, networking and NewGRF feature mapping.
- [ ] 1.2 Select an explicit Space category/controller representation. Preferred: a dedicated company-owned type with stable saved IDs. Document conversion if internal enums change; Effect/Disaster and existing train/road/ship/aircraft saves retain their meaning.
- [ ] 1.3 Prove one stationary and one moving orbital test vehicle can tick, own cargo, be selected, appear in company accounting, save/reload and coexist with ordinary aircraft. Do this before rockets/probes depend on the model.
- [ ] 1.4 Define mission/order references for moving body IDs, static facilities and Surface recovery centers. Unknown/removed targets produce a recoverable blocked/return state.

## Group 2 - Orbital world and construction

- [ ] 2.1 Add pooled orbital bodies/structures and fixed-point motion with stable IDs, ownership where applicable, a spatial index and bounded active update costs. Define coordinates relative to the Space plane, rendered through existing stacked world positions.
- [ ] 2.2 Specify asteroid drift band versus structure/docking occupancy, docking approaches, collision avoidance, deterministic tie-breaks and map-edge behavior. No orbit integrator or impact-damage simulation is required.
- [ ] 2.3 Add Space Construction controls for docking/sites, modules, processing, assembly and mirrors. Replace ordinary land/rail/road/water/airport/terraform controls on Space; keep layer/view/navigation controls.
- [ ] 2.4 Implement blueprint footprint reservations, ownership and explicit delivered-inventory prerequisites. This plan owns placement/occupancy; r2mlvs11 owns module assembly and function.
- [ ] 2.5 Define a synchronized cross-layer transition service for rockets/descent jobs using source/destination references and timers. Ground portal movement continues through its established helper. No controller performs casual plane-offset teleportation.
- [ ] 2.6 Support the launch-center's 20-tile circular release target as a region distinct from its exact docking/servicing point. Use inclusive squared fixed-point distance in plane coordinates, explicit center ownership/selection and saved release positions. Route/picking overlays must not confuse the orbital drop zone with the independent Surface collection area.

## Group 3 - Rendering, persistence and acceptance

- [ ] 3.1 Update display-layer, vehicle hashing, viewport filtering, shadows/effects, click detection, following and small-map overlays for Space entities. Ordinary aircraft and ships stay Surface-only.
- [ ] 3.2 Persist entities/missions and rebuild spatial indexes/derived motion caches on load. Presentation interpolation cannot influence gameplay positions or docking success.
- [ ] 3.3 Test ordinary construction refusals by commands and toolbar, overlapping blueprints, map edges, hidden layers, view-follow crossing and invalid targets.
- [ ] 3.4 Compare fixed-tick entity states across visible/hidden views and command replay. Measure entity scaling and network-join reconstruction before downstream fleets grow.
- [ ] 3.5 Integrate final controls, blueprint/occupancy overlays and Space object picking with r2mlvs13 assets. Keep placeholders explicitly temporary.

## Completion conditions

Space has a coherent build and vehicle model, persistent identity and deterministic motion. Rockets/probes can be implemented without abusing aircraft terrain flight or renumbering old save values. Ordinary land construction is unavailable on Space at both UI and command levels.
