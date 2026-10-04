# R2MLVS11 - Orbital Stations and Manufacturing

Status: proposed. Parent: [R2MLVS master](r2mlvs-multilayer-vehicles-and-structures.md). Depends on r2mlvs9/10 and Surface recipes from r2mlvs6. This is milestone M4; the basic probe and mirror loops remain independently playable.

## Outcome and reading

Rockets deliver components that become functional station modules. Stations assemble larger mining robots and process asteroid cargo. Read master AD-8/12 and I-3/4/7/12; [industry production](../../architecture/industries/archi-industries.md), [station/cargo architecture](../../architecture/vehicles/archi-vehicles.md), [commands](../../architecture/commands/archi-commands.md), and the orbital entity, occupancy and payload contracts from r2mlvs8/9.

## Module contract

| Module | Role | Bootstrap input |
| --- | --- | --- |
| Core | Anchor, initial assembly/service capacity and ownership | Rocket-delivered Station Components; deployable core recipe |
| Dock | Rocket/hauler service and explicit cargo exchange | Components and Electronics |
| Storage | Bounded cargo inventory | Components |
| Utility | Station power/service capacity | Components, Batteries and service supplies |
| Assembly | Builds heavy robots and orbital haulers | Components, Electronics; vehicle recipes consume Robotics Parts |
| Processor | Bounded conversion of asteroid materials | Components/Robotics Parts; then mineral or Water Ice feedstock |

Exact quantities, footprints and service consumption belong to r2mlvs1's numerical tables. Inputs to the first Core/Assembly/robot must all be producible on Surface.

## Group 1 - Delivered construction

- [ ] 1.1 Implement a reserved blueprint anchor and a deployable-core rocket mission. Before the core exists, the delivery job holds its cargo; a fictional orbital station cannot accept it in advance.
- [ ] 1.2 Add module adjacency/footprint, compatible docking approaches, ownership, inventory reservation and assembly time. A blueprint provides no service, storage or production capacity.
- [ ] 1.3 Consume construction inputs at explicitly saved milestones. Partial assembly/cancellation/refund rules conserve materials and cannot pay delivery twice or refund consumed inventory as new cargo.
- [ ] 1.4 Add completed Core/Dock/Storage/Utility functions, power/service limits and module dependency checks. Removing the final dock or core requires safe inventory/vehicle evacuation.

## Group 2 - Orbital manufacture and processing

- [ ] 2.1 Add Assembly purchase/production orders for heavy mining robots and orbital haulers, requiring physical parts, funds and a free compatible berth. Ordinary purchase/clone commands cannot bypass assembly inputs.
- [ ] 2.2 Extend the mining mission controller with heavy-robot capacity, service and extraction differences. Return orders may unload at a compatible station processor or launch-center descent point.
- [ ] 2.3 Implement processor recipes with bounded input/output buffers, useful service consumption and explicit material balance. Output congestion pauses production instead of discarding cargo.
- [ ] 2.4 Add Water Ice -> Propellant and appropriate mineral -> Refined Metals processing according to the content contract. Use existing transport cargos where meanings match.
- [ ] 2.5 Carry output by orbital hauler/rocket/descent to a real next consumer. Define docking CargoDist/order support and freight accounting rather than relying on geometric proximity between modules owned by different stations.
- [ ] 2.6 Let compatible heavy robots/haulers use the same 20-tile launch-center cargo-drop rule as small probes, while preserving their docking/service rules. Route returned processed cargo through linked Surface train/lorry collection stations; later Mantle Forge output can supply module/robot construction without becoming a prerequisite for the first station.

## Group 3 - Presentation and acceptance

- [ ] 3.1 Integrate module construction stages, adjacency connectors, docking foregrounds, heavy robots, haulers and process animations from r2mlvs13. Show required/delivered/consumed inventory and blocked reasons.
- [ ] 3.2 Test insufficient deliveries, overlapping blueprints, competing jobs reserving the same stock, missing utilities, full buffers, invalid dock references and unsafe removal.
- [ ] 3.3 Save/reload halfway through core deployment, each module build, robot assembly and processing; assert exact inventory and output conservation.
- [ ] 3.4 Demonstrate a complete chain: Surface-made components -> rocket -> Core/Dock/Assembly -> heavy robot -> asteroid -> processor -> useful Surface or orbital output. Use no cheat-built predecessor in the acceptance run.

## Completion conditions

Station construction and robot manufacturing consume delivered goods, and orbital processing produces transportable useful output. Every first component can be supplied from Earth. Free blueprint placement, instant vehicle purchase or save/load cannot substitute for the supply chain.
