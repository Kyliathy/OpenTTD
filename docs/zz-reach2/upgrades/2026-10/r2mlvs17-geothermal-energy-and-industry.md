# R2MLVS17 - Geothermal Energy and Industry

Status: proposed; futuristic facilities below are game designs, not assertions of real industrial capabilities. Parent: [R2MLVS master](r2mlvs-multilayer-vehicles-and-structures.md). Depends on geology/1980 access from r2mlvs2/3, industry lifecycle from r2mlvs6 and the useful-power contract from r2mlvs1. r2mlvs16 consumes available electricity but does not require geothermal for its bootstrap.

## Outcome and reading

Deep geothermal vents become useful energy sites and eventually host direct-heat factories. Read master AD-4/16 and I-3/5/12/14/17; [industry construction](../../architecture/industries/archi-industry-construction.md), [industries](../../architecture/industries/archi-industries.md), [savegame](../../architecture/savegame/archi-savegame.md), `src/timer/timer_game_calendar.h` and the actual power/production interfaces established by r2mlvs1/12. Technology uses calendar dates; production and thermal allocation use simulation/economy scheduling.

## Proposed technology timeline

| Available from | Facility | Gameplay |
| --- | --- | --- |
| 1 January 1980 | Deep access, Vent Collector and Vent-Tap Powerhouse | Excavate/claim a vent, tap a bounded heat rate and turn it into electricity for Deep users or explicit Surface export |
| 2000 | Magma-Loop Power Station | A fictional advanced closed-loop installation with better heat-to-electric conversion, higher capital/service costs and larger footprint |
| 2010 | Geothermal Smelter | Use direct vent heat with delivered Iron/Copper/Nickel ore to produce Steel/Refined Metals; saved alternative recipes, no requirement to generate electricity first |
| 2030 | Lithothermal Battery Works | Use heat, Lithium/Cobalt and delivered Chemicals to manufacture Batteries near their mineral sources |
| 2045 | Mantle Forge | Use heat, Refined Metals and Electronics to manufacture Station Components/Robotics Parts for orbital construction |

These recipes reuse existing planned cargos, so vents/heat introduce no extra freight cargo slot. Exact dates after the 1980 access baseline, input quantities, conversion efficiencies and throughput are tuning proposals. Every plant requires transported construction/service inputs, sufficient heat, output space and paid operation. Conventional Surface processing remains available so the timeline never leaves a mineral without a receiver.

## Group 1 - Vent sites, heat and utilities

- [ ] 1.1 Define stable vent-field records, claims, sustainable thermal capacity and surveyed state in r2mlvs2's geology system. Generate at least one usable vent field on supported Deep maps, in addition to all five mineral types; validate plant footprints/access without making pre-cleared caverns.
- [ ] 1.2 Bind collectors/plants to a vent only after full excavation and the calendar unlock. Excavation may clear any required plant/utility area; raw vent identity survives ordinary clearing, and removing/rebuilding a plant never resets or multiplies its heat capacity.
- [ ] 1.3 Implement a shared deterministic heat budget, allocating capacity between electrical generators and direct-heat consumers. Specify fair/priority allocation, ownership, conversion losses and partial-tick remainders; competing claims cannot each use the full vent output.
- [ ] 1.4 Add heat/electric utility conduits on excavated Deep tiles, with declared coexistence rules for rail/road/tube facilities. Heat remains layer-local. A disconnected consumer pauses or uses explicitly purchased backup, never gets heat from catchment proximity.
- [ ] 1.5 Implement a paid electrical utility riser through a commissioned inter-layer portal and explicit surface grid injection. Gates on excavation, ownership, capacity and the 1980 unlock apply to both ends. Electricity is a metered flow, not duplicated freight.
- [ ] 1.6 Bootstrap the first collector/powerhouse/riser using cash and Surface-available Steel/Chemicals/machinery requirements that do not depend on geothermal, Deep batteries or tube-only delivery. Early tube external service remains independently usable.

## Group 2 - Plants and future production

- [ ] 2.1 Implement Vent-Tap and Magma-Loop generator definitions with distinct costs/efficiencies, capacity-limited output, heat use and service behavior. Reuse useful-power interfaces while preserving vent identity and actual layer.
- [ ] 2.2 Implement the direct Geothermal Smelter, Lithothermal Battery Works and Mantle Forge recipes/timelines. Direct-heat production checks physical input cargo, allocated heat and output capacity before consuming anything.
- [ ] 2.3 Keep input/output cargo accounting compatible with r2mlvs6 mines and Surface chains. One plant must not consume both a full heat allocation and an unrecorded electricity subsidy for the same production benefit.
- [ ] 2.4 Enforce availability at purchase/funding, commissioning, script commands, clone/conversion and production entry points. Before 1980, Deep operations remain locked; factories introduced later cannot be made early by reusing an older plant ID.
- [ ] 2.5 Integrate Deep electricity with facilities/tubes and explicit Surface export. Native/ETI modes choose one accounting authority; report exported electricity at a Surface adapter/riser without also crediting its Deep generator as an independent Surface supply. Solar mirrors never boost geothermal output.
- [ ] 2.6 Handle shutdown, lost heat/utility connection, full storage, vent claim transfer, bankruptcy and demolition with recoverable inventory and stable target references.

## Group 3 - Controls, art and acceptance

- [ ] 3.1 Add vent surveys and heat-capacity overlays, utility links and calendar-aware plant tools to Underground Construction. Show allocated/available heat, electric conversion, direct-heat consumption and blocked reasons.
- [ ] 3.2 Deliver distinct vent/collector, two powerhouses, smelter, battery works, forge and utility sprites with construction/working/starved states. Smelter/furnace animation must reflect production state but never drive it.
- [ ] 3.3 Test 31 December 1979 refusal and 1 January 1980 unlock for every Deep portal/elevator entry path and plant/vehicle operation. Test each later factory's introduction boundary in calendar time with varied economy speed.
- [ ] 3.4 Run mineral mine -> tube/conventional freight -> geothermal smelter -> Surface or orbital supply chain. Demonstrate a battery and Station Components/Robotics Parts chain at their later dates without cheat-supplied inputs.
- [ ] 3.5 Test two generators plus a direct smelter sharing one vent: combined heat use never exceeds capacity, useful output follows losses, and disconnection reallocates predictably. Verify zero solar boost and no duplicate native/ETI credit.
- [ ] 3.6 Save/reload vent claims, partly processed inputs, heat/electric remainders and utility failures; run multiplayer replay with Deep hidden. Record capacity/price/throughput comparisons for r2mlvs15.

## Completion conditions

Deep opens in the proposed 1980 era; excavated geothermal sites then provide bounded useful energy. Later, visibly distinct fictional factories use direct heat and transported materials for useful products. Heat allocation, electrical export and recipes conserve resources across saves and competing consumers; no first facility requires its own future output.
