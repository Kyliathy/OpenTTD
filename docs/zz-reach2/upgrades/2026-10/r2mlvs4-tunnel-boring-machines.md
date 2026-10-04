# R2MLVS4 - Tunnel Boring Machines

Status: proposed. Parent: [R2MLVS master](r2mlvs-multilayer-vehicles-and-structures.md). Depends on r2mlvs3 and r2mlvs1's wagon contract. r2mlvs5 completes the recurring haulage loop.

## Outcome and reading

Deliver a real train that bores connected rock, advances on paid support track and accumulates Earth/Rock in attached wagons. Read master AD-3/5/6 and I-2..5/I-8/I-12; [trains](../../architecture/vehicles/trains.md), [vehicles](../../architecture/vehicles/archi-vehicles.md), [economy](../../architecture/economy/archi-economy.md), `src/train_cmd.cpp`, `src/train.h`, orders, cargo containers and vehicle saveload.

## Group 1 - Vehicle and work orders

- [ ] 1.1 Add standard and deep-rated cutter definitions with visible capability, running cost, power/speed and compatible Earth/Rock wagons. State whether articulated cutter/support parts are permanently attached; only designated spoil wagons are exchangeable.
- [ ] 1.2 Enforce rail-portal commissioning in purchase, clone, autoreplace, scripts and depot controls. Servicing must not become a purchase bypass. A deep model must physically reach its commissioning source.
- [ ] 1.3 Add Boring Project orders with saved project, current face, Surface origin station and exchange bay references. Preview route, hardness, projected spoil and support costs before starting.
- [ ] 1.4 Implement states: Commissioning -> Travel to Face -> Bore -> Advance/Support -> Return for Exchange -> Wait for Empties -> Resume, plus Stopped/Blocked/Servicing. Timers are simulation ticks.
- [ ] 1.5 Add a saved cutter heading and area-clearing work orders. Accept outlines for any reachable underground use, not only rail/mining corridors; show generated passes, turn envelopes and extra turning-space excavation before starting.

## Group 2 - Boring and accounting

- [ ] 2.1 Reserve the face and safe retreat route. Implement bounded incremental progress with model/hardness speed. Only the authorized cutter may work the reserved solid face.
- [ ] 2.2 Before committing a cut, check Earth and Rock capacities independently, support availability, funding and resulting safe train positions. Commit terrain/yield/cost once, even at a save or tick boundary.
- [ ] 2.3 Add generated spoil as ordinary cargo packets in the attached carriages, with a defined work-site origin and age. No temporary infinite hold or automatic Surface delivery.
- [ ] 2.4 Lay temporary support/rail as the cutter advances, charging the declared costs and recording ownership. Define track conversion/removal and traversal by the cutter/support consist.
- [ ] 2.5 Support branches/chambers and arbitrary connected area outlines through reachable passes. The eight-heading cutter may advance straight or turn at most 45 degrees per segment. Require actual forward movement between bends; reject 90/135/180-degree single-step turns, stationary repeated pivots and diagonal corner clipping. Account for the cutter/consist swept volume and additional turning space.
- [ ] 2.6 Charge active/idle running costs and work costs separately. Bankruptcy or insufficient funds halts new work without deleting spoil, completed cavities or retreat capability.
- [ ] 2.7 Distinguish backing along completed support rail from changing the cutter's facing: retreat cannot enable backwards boring or bypass the 45-degree rule. Keep ordinary freight outside the work reservation and retain a safe retreat/haulage path.
- [ ] 2.8 Allow cleared ground to become tubes, roads, terminals, mines, geothermal plants or open chambers after safe support-track removal/conversion. Final use need not contain rails. Never remove support under the TBM or strand it by completing a tube segment behind it. Deep cutting obeys the 1980 calendar gate.

## Group 3 - Recovery, presentation and acceptance

- [ ] 3.1 Expose current face, progress, hardness, both spoil capacities, cost and precise blocked reason. Integrate idle/boring/reversing/servicing graphics and sound events from r2mlvs13.
- [ ] 3.2 Define order cancellation, breakdown/recovery, cutter sale, project reassignment and a lost exchange bay. Preserve cargo and permit safe retreat; do not strand a stopped TBM inside a permanently locked portal.
- [ ] 3.3 Test exact material conservation across partial cuts, capacity boundaries, mixed spoil ratios, cancellation, reversal and saves during work. Compare progress with the layer hidden and visible.
- [ ] 3.4 Demonstrate the first cut and return to the source-side commissioning exchange, then integrate a recurring underground-bay loop with r2mlvs5.
- [ ] 3.5 Record time and full cost for a standard corridor and a harder Deep corridor. Hand the measurements to r2mlvs15 instead of declaring balance from nominal prices.
- [ ] 3.6 Test legal 45-degree bends, 90-degree direction changes assembled from two advancing bends, U-turn space, backing, illegal pivots and interrupted turn saves. Clear a large irregular tube-terminal/plant footprint with complete spoil accounting, then withdraw and replace safe temporary track with its final non-rail use.

## Completion conditions

A commissioned TBM clears any reachable excavatable underground area for its intended use, turns at most 45 degrees per advancing segment and carries the resulting spoil in actual wagons. It cannot bore with full carriages or insufficient support/cash, cannot be spawned through other transport modes, and can recover safely from every stopped state. Heading, swept-footprint progress and cargo resume exactly after save/load.
