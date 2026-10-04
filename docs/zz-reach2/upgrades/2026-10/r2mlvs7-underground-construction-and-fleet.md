# R2MLVS7 - Underground Construction and Fleet

Status: proposed. Parent: [R2MLVS master](r2mlvs-multilayer-vehicles-and-structures.md). Depends on r2mlvs3/5/6.

## Outcome and reading

Replace underground airport controls with a useful construction system and deliver the stations, depots and vehicles needed for mining. Integrate Transport Tubes and geothermal controls from r2mlvs16/17; those plans own the five original tube models and thermal simulation. Read master AD-4/7/8/15/16, its fleet catalogue and I-8/I-10/I-14/I-15; [GUI](../../architecture/gui/archi-gui.md), [vehicles](../../architecture/vehicles/archi-vehicles.md), `src/toolbar_gui.cpp`, `src/rail_gui.cpp`, `src/road_gui.cpp`, `src/station_cmd.cpp`, `src/map_layer_gui.cpp` and relevant vehicle purchase/refit commands.

## Group 1 - Underground construction interface

- [ ] 1.1 Replace the Airport slot/action on both rock layers with Underground Construction. Include surveys, excavation projects, mines, exchange bays, underground freight stations, road terminals and service facilities.
- [ ] 1.2 Keep rail/road construction and new Deep rail controls consistent with excavated state. Explain why an unavailable facility needs excavation, a deposit, rail access or a model capability.
- [ ] 1.3 Update split toolbars, hotkeys, editor controls and tooltips. Bind a construction action to the clicked world's layer, including extra viewports; switching the main view must not redirect a pending command silently.
- [ ] 1.4 Expose estimated full-footprint excavation/material costs before construction. Building a large station cannot auto-clear rock underneath it.
- [ ] 1.5 Add Transport Tubes guideway/terminal/depot/power tools and a distinct capsule fleet filter, consuming r2mlvs16's capability contract. Add Deep vent survey, utilities, generators and future plant controls from r2mlvs17. Show 1980 Deep unlock and later model/factory dates consistently across toolbars, shortcuts and previews.
- [ ] 1.6 Provide general area-clearing selection with previews of legal 45-degree TBM passes, extra turn space and final non-rail footprints, including tube terminals and geothermal plants.

## Group 2 - Facilities and fleet

- [ ] 2.1 Add underground rail freight station/loading-platform and road mine terminal definitions, compatible with existing station orders/loading where possible. Underground passenger catchment is not added here.
- [ ] 2.2 Add compatible train/road service facilities on excavated ground. TBM servicing is allowed but new TBM purchases remain restricted to commissioning portals.
- [ ] 2.3 Add mine trucks, underground freight locomotive/wagons and the deep-capable mineral carrier. Ensure the complete ten-resource/spoil cargo table has at least one transport option; visible refit rules must match command validation.
- [ ] 2.4 Test Deep elevator compatibility for trucks/articulated parts and layer rail compatibility for freight consists. Do not let electric vehicles travel on unpowered temporary work track through a visual rail-type mismatch.
- [ ] 2.5 Integrate running costs, service, replacement, vehicle groups/lists and clear layer/project names. Readable specialist vehicles must remain normal manageable fleet assets.
- [ ] 2.6 Integrate all five original tube models, terminals and powered service facilities from r2mlvs16 with the underground purchase/refit/order interfaces. Show model dates and incompatible rail/portal refusals without treating them as ordinary underground trains.

## Group 3 - Art, workflows and acceptance

- [ ] 3.1 Integrate final toolbar/cursor, station, depot, vehicle direction and cargo-load sprites from r2mlvs13. Verify vehicle occlusion at loading docks, bays and deep portals.
- [ ] 3.2 Complete a player workflow using only visible controls: survey -> commission/dig -> exchange -> build mine/station -> buy/refit carrier -> create orders -> deliver to Surface.
- [ ] 3.3 Repeat on both rock layers and through main/extra viewports, split toolbar sizes, keyboard controls and editor. Confirm ordinary airports cannot be constructed below Surface through commands either.
- [ ] 3.4 Test malformed commands, clone/autoreplace purchase restrictions, same-XY cross-layer stations and missing cargo compatibility. Save/reload the full underground fleet and station order setup.
- [ ] 3.5 Exercise pre/post-1980 Deep controls, tube model introduction dates, geothermal plant timelines and area-clearing tool previews. Verify five distinct tube vehicles appear with their final art and route cargo through layer-local terminals to conventional interchanges.

## Completion conditions

The underground controls make the excavation/mining workflow discoverable, with no unusable Airport menu left behind. All required transport and service facilities have working graphics and enforce the same layer rules through UI, commands and scripts.
