# R2MLVS16 - Underground Transport Tubes

Status: proposed; no implementation or graphics completed. Parent: [R2MLVS master](r2mlvs-multilayer-vehicles-and-structures.md). Depends on excavation/commissioning interfaces from r2mlvs2..5 and content contracts from r2mlvs1; integrates with r2mlvs7, r2mlvs13 and r2mlvs14. Final acceptance is owned by r2mlvs15. No geothermal dependency is required for the first tube network.

## Outcome and reading

Build a distinct underground freight system inside TBM-cleared corridors and chambers. Dedicated powered tubes carry at least five original capsule models between underground terminals; ordinary trains and road vehicles do not use them. Read master AD-5/8/15 and I-2/3/8/13/15; [rail routing](../../architecture/pathfinder/archi-pathfinder-rail.md), [trains](../../architecture/vehicles/trains.md), [stations](../../architecture/landscape/station-tiles.md), railtype/engine compatibility and station cargo commands. Read current code before deciding which train assumptions can be reused.

## First-party vehicle catalogue

These five designs are required deliverables, not alternative names for refits. Dates and relative balance are proposed; each gets its own engine/model identity, stats, purchase sprite and eight-direction/load art. Full capacity values and cargo-unit conversion belong to r2mlvs1's tuning tables.

| Model / proposed introduction | Cargo role | Performance identity | Original visual identity |
| --- | --- | --- | --- |
| Mole-70 Bulk Capsule / 1970 | Coal, Iron/Copper/Bauxite/Salt and compatible deep ores; Rock/Earth outside the TBM's mandatory wagon leg | Cheap, large bulk hold, modest speed and high loaded energy use | Low segmented amber capsule with enclosed hopper ribs and visible load indicators |
| Crate-75 Container Capsule / 1975 | Goods, Batteries, Electronics and Solar Mirror Kits | Faster, medium hold; frequent manufacturing deliveries | Square-section green cargo pods with clamped containers and numbered doors |
| Cistern-80 Fluid Capsule / 1980 | Liquid Chemicals/Propellant and other declared compatible liquids | Medium speed, sealed liquid hold; specialized terminal service | Cyan twin-tank capsule with pressure bands, valves and level display |
| Shield-85 Mineral Capsule / 1985 | Uranium and designated sealed mineral loads | Small hold, expensive shielding/service; reliable valuable loads | Short octagonal capsule with thick pale shielding panels and mineral-specific load markings |
| Forge-05 Heavy Capsule / 2005 | Steel, Refined Metals, Station Components and Robotics Parts | Very high payload, low acceleration, long terminal dwell and high power demand | Wide dark machinery capsule with structural braces and a distinct heavy coupling frame |

The content profile defines one physical form per cargo label: Chemicals are liquid for Cistern-80 in this draft; Crate-75 does not refit to the same label as packaged Chemicals. A future packaged form would require an explicit conversion/label decision. Existing trains/trucks carry early Uranium until the Shield-85 arrives. Tube introduction before 1980 does not grant access to Deep Underground.

## Group 1 - Infrastructure and capability contract

- [ ] 1.1 Prototype a dedicated tube railtype/capability using existing track reservations/pathfinding. Freeze label, allowed layers, compatible capsules, crossing/curve/junction behavior and depot/terminal footprint. This is a separate gameplay transport system even if it reuses Train internals.
- [ ] 1.2 Enforce full-footprint excavation for guideways, terminals, junctions, utilities and service depots. Surface, Space and locked Deep tiles reject tube construction through GUI, commands, scripts, copying and conversion.
- [ ] 1.3 Enforce capability separation: capsules cannot run on ordinary/temporary rail, enter ground portals/elevators or be mixed into ordinary consists; ordinary trains/TBMs cannot enter tubes. First-release networks remain layer-local.
- [ ] 1.4 Define track removal/conversion after TBM withdrawal: replacement of temporary work rail may leave tubes as the final use, but cannot strand a cutter or remove its reserved retreat/haulage path. Tubes never bore rock themselves.
- [ ] 1.5 Define junction reservations, directional travel, turnaround/service and blocked-route recovery for individual self-propelled capsules. Freeze legal turning geometry independently of the TBM's separate 45-degree cutting constraint.
- [ ] 1.6 Bootstrap power via paid external service at an accessible commissioned portal-side interchange. Later explicit utility connections may source geothermal electricity. Account demand/cost once; no free propulsion and no requirement for a not-yet-built geothermal station.

## Group 2 - Terminals, fleet and controls

- [ ] 2.1 Add tube loading/unloading terminals, powered service depots and rail/road cargo interchanges on excavated ground. Transfer uses ordinary accountable cargo packets and layer-local station handling; a tube capsule cannot teleport its cargo to Surface.
- [ ] 2.2 Implement all five catalogue models as first-party definitions/art with distinct cargo roles, prices, capacities, speeds, service and energy behavior. Validate that no model is only a refit/palette swap of another.
- [ ] 2.3 Use proper purchase/clone/refit/autoreplace restrictions, introduction dates and compatible-depot filters. Make relevant original fleet/content available without a third-party vehicle GRF.
- [ ] 2.4 Add Transport Tubes to Underground Construction with dedicated guideway, terminal, depot, utility and vehicle-list controls. Show route, excavation and power failures and useful cargo/refit previews.
- [ ] 2.5 Coordinate underground mine/factory orders and CargoDist transfer to conventional freight for cross-layer legs. Support empty returns and bounded buffers; transfer handling cannot generate duplicate delivery income.
- [ ] 2.6 Preserve mandatory TBM spoil logistics: its spoil leaves in the same wagons behind a second locomotive and visits the originating Surface station. Tubes can carry other mine waste or later cargo legs but are not a substitute for that requirement.

## Group 3 - Art, persistence and acceptance

- [ ] 3.1 Integrate tube straight/bend/junction/end sprites, terminal fronts/backs, depot/service animation and power indicators. Show capsules moving through readable transparent/cutaway tube art, with correct occlusion and selection.
- [ ] 3.2 Save capsule orders/positions/cargo, service/power state and network references. Rebuild only derived route/energy caches; infrastructure changes and bankruptcy cannot orphan cargo.
- [ ] 3.3 Test solid/unlocked-layer refusals, pre-1980 Deep access, ordinary train/tube incompatibility, unsafe work-track replacement, disconnected power and illegal cross-layer routing.
- [ ] 3.4 Run one useful freight route with each of the five models at its introduction date. Include Underground-to-Deep chains with explicit conventional transfer and distinguish all five silhouettes in runtime captures.
- [ ] 3.5 Run a TBM-cleared curved corridor and large terminal cavern -> remove safe temporary rail -> install tubes -> load cargo -> transfer to Surface freight scenario. Save/reload while capsules queue and change reservations.
- [ ] 3.6 Verify multiplayer/state replay and balance capacity, energy demand and throughput against ordinary underground rail/road. Include a paid-grid first network and a later geothermal-powered network.

## Completion conditions

Five original functional vehicles operate on a distinct underground tube network built only after excavation. Their roles/art/stats are distinguishable, power and cargo transfers are accountable, early play needs no Deep energy, and tubes cannot bypass the 1980 unlock or the real TBM wagon haulage loop.
