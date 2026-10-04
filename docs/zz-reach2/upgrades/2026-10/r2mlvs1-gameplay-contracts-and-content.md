# R2MLVS1 - Gameplay Contracts and Content

Status: proposed; no implementation tasks completed. Parent: [R2MLVS master](r2mlvs-multilayer-vehicles-and-structures.md). Owners of requirements and proposed decisions are listed there.

## Outcome and boundaries

Freeze an implementable first content profile before adding vehicles or industries. The profile includes ten distinct mine resources, their receivers, excavation spoil, five original tube vehicles, geothermal/future industries, launch collection hubs, orbital construction and useful power. It remains within the current cargo/industry limits and has a complete bootstrap path. This plan owns definitions and interface contracts; feature plans own behavior. Also consume master AD-15/16 and the catalogues in r2mlvs16/17 while drafting Groups 1/2; their implementation is not a prerequisite to the contract.

## Reading

Read master AD-1, AD-5..7, AD-10..13 and I-3/I-8/I-12. Consult [industries](../../architecture/industries/archi-industries.md), [NewGRF](../../architecture/newgrf/archi-newgrf.md), [cargo payment](../../architecture/economy/archi-cargo-payment.md), `src/cargo_type.h`, `src/industry_type.h`, `src/industry.h`, `src/engine_type.h` and the actual content loaders. Resolve symbols in current code rather than relying on historical line numbers.

## Group 1 - Content budget and profile

- [ ] 1.1 Inventory live cargo labels/slots, industry types/tiles, accepted/produced cargo-array limits and engine IDs for the proposed Temperate profile. Preserve existing numeric meanings.
- [ ] 1.2 Publish the ten-resource table with stable four-character labels, units, cargo classes, refit compatibility, payment category and consumer. Reuse a label only when its meaning matches; Uranium must have one identity if ETI is selected.
- [ ] 1.3 Budget the entire catalogue. Starting allowance: at most 12 baseline cargos + 8 additional mine resources (Coal/Iron Ore reused) + 11 additional cargos from the master + 1 Power representation = 32 active cargos before any further additions. Verify actual labels and climate counts; this arithmetic is a design budget, not a slot allocation.
- [ ] 1.4 Choose built-in definitions or a reproducibly bundled first-party NewGRF through a small load/save/package spike. Record asset loading, dedicated-server availability, required-content failure and stable mapping behavior.
- [ ] 1.5 Define Classic and Multilayer Industry creation settings, supported climates, dates/unlocks and saved profile version. Disable unsupported industry-set combinations before world creation with a specific reason. Existing worlds retain their profile.
- [ ] 1.6 Freeze the proposed 1 January 1980 calendar unlock for all Deep entry/construction/operation paths, explicit legacy/editor exceptions, five tube introduction dates and the 1980/2000/2010/2030/2045 geothermal timeline. Keep technology dates separate from economic production timers.
- [ ] 1.7 Budget tube models, terminals and geothermal buildings as well as cargo slots. Heat/electric flows reuse metered utility state, not extra freight cargos; geothermal recipes reuse the existing planned materials. Define liquid Chemicals/Propellant consistently across vehicle refits.

## Group 2 - Recipes, capability and economics

- [ ] 2.1 Specify inputs, outputs, reserve consumption, storage and processing time for each mine/processor. Include usable Surface sinks for all ten resources, Earth disposal, Rock crushing and surplus disposal.
- [ ] 2.2 Define a bootstrap chain using Surface inputs: Steel/Refined Metals + Chemicals -> Station Components; Refined Metals + rare-earth supply -> Electronics; Lithium/Cobalt -> Batteries; Electronics/Batteries/metals -> Robotics Parts; Chemicals or Water Ice processing -> Propellant; components/electronics -> Solar Mirror Kits. Split recipes to respect actual input/output limits and remove circular dependencies.
- [ ] 2.3 Specify how construction/service inputs enter facility inventory, when packets are consumed, and when a carrier earns delivery income. A delivery, a transfer and construction consumption cannot each pay for the same final transport leg.
- [ ] 2.4 Publish capability tables for standard/deep TBMs, hoppers, haulers, trucks, all five tube capsules, rockets, probes and robots: layers, transport/rail compatibility, cargo classes, commissioning facility, service facility, dates and replacement rules. Tube capability excludes ordinary rail/portals and cannot bypass spoil-wagon hauling.
- [ ] 2.5 Publish initial prices, time/capacity units and ratios: active/idle TBM costs, spoil per rock type, support costs, hauler capacity, disposal limits, launch payload/turnaround, probe hold/service budget and module recipes. All time is simulation time. Define deterministic exhaustion behavior.
- [ ] 2.6 Define the useful-power contract: eligible generation, baseline/effective units, layer-local consumer allocation, explicit inter-layer electrical export, monthly history and a single grid authority. Original-content mode supplies processors/tubes and a bounded Surface town-growth benefit; ETI mode may use its GS for town effects. Geothermal heat is allocated once between power and direct processing; solar mirrors boost only solar. Do not allow two systems to set the same town growth or reward virtual electricity as unrestricted freight.
- [ ] 2.7 Define the two independent launch-center areas: required 20-tile circular orbital release radius and proposed 20-tile Surface collection radius. Specify membership, cargo filters, bounded handling rates/costs, stock reservations and no extra final-delivery payments.
- [ ] 2.8 Specify direct-heat smelting, battery and Mantle Forge recipes and their transported inputs. Bootstrap the first geothermal plant/riser from Surface materials and early tubes from paid external electricity, without a dependency on Deep products before 1980.

## Group 3 - Shared interfaces and feasibility records

- [ ] 3.1 Define typed references and ownership for Deposit, ExcavationJob, TBMProject, ExchangeAssignment, OrbitalBody, SpaceMission, ConstructionInventory, MirrorAssignment, LaunchCollectionHub, SurfaceHandlingJob, TubeNetwork and GeothermalVent/UtilityLink. These are proposed concepts, not claims of existing C++ classes.
- [ ] 3.2 Freeze shared state transitions and atomic boundaries with r2mlvs14 Group 1. Publish a dependency table identifying authoritative fields versus derived caches.
- [ ] 3.3 Finish the ETI feasibility decision with r2mlvs12 Group 1 before selecting the production content profile. Do not spend the whole cargo budget assuming arbitrary GRFs can coexist.
- [ ] 3.4 Finish the initial sprite/footprint contract with r2mlvs13 Group 1. Every functional type needs visible identity, status and purchase/build previews.
- [ ] 3.5 Review the master defaults against concrete worked budgets: TBM area clearing with legal 45-degree turns, all ten mines, first tube route, 1980 vent plant and later smelter, first probe/drop/collection route, first mirror and first station/heavy robot. Record any design changes centrally.

## Completion conditions

Every resource has a transport vehicle and useful receiver; every advanced facility can be bootstrapped from earlier supplies. Cargo and industry budgets fit actual limits. Content can be loaded deterministically, identified in saves and distributed with the chosen delivery route. ETI uncertainty has a working original-content fallback. Definitions are specific enough for feature work without inventing incompatible IDs or payment semantics.
