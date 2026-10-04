# R2MLVS5 - Spoil Wagons and Haulage

Status: proposed. Parent: [R2MLVS master](r2mlvs-multilayer-vehicles-and-structures.md). Depends on r2mlvs4. This is a major train-operation feature, not a cosmetic loading animation.

## Outcome and reading

A second locomotive physically takes the TBM's loaded carriages to its originating Surface station and returns empty capacity. Read master AD-6 and I-3/4/7/12; [trains](../../architecture/vehicles/trains.md), [cargo payment](../../architecture/economy/archi-cargo-payment.md), `CmdMoveRailVehicle`, `ArrangeTrains`, train cache rebuilding, cargo packets, depot commands and station unloading.

## Group 1 - Controlled exchange facility

- [ ] 1.1 Implement the exchange bay as a specialized depot-like rail facility with explicit footprint, approach capacity and reservation rules. Its functional train rearrangement location must satisfy controlled stopped-consist semantics.
- [ ] 1.2 Define the exact exchange transaction: reserve both consists, verify ownership/compatibility/length and stoppage, detach only eligible spoil wagons, attach loaded wagons to the hauler and empties to the cutter, then rebuild caches/reservations and publish state once.
- [ ] 1.3 Reuse reviewed train rearrangement primitives where their invariants hold. Do not relax the global depot-only guard to allow arbitrary in-motion coupling. Preserve vehicle IDs, cargo packets/age, refit types, groups, engine orders and assignment identity.
- [ ] 1.4 Validate complete feasibility before mutation; exercise rollback if a lower-level operation can still fail. No partial swap may leave two heads claiming a wagon chain.
- [ ] 1.5 Apply the same contract to the commissioning-side bootstrap exchange. The original Surface station must be accessible and able to unload both spoil types through a real route/sink.

## Group 2 - Hauler orders and disposal

- [ ] 2.1 Add an assignment/order workflow: Await TBM Exchange -> Collect Loaded Wagons -> Origin Surface Station -> Dispose or Transfer -> Return Empty Wagons. Pair a project with a hauler/empty set without changing unrelated train orders.
- [ ] 2.2 Permit multiple wagon sets and haulers with deterministic queueing; start with one set if necessary. Full/empty set identity and cargo-capacity checks must survive a consist head change.
- [ ] 2.3 Require passage through the project's saved Surface origin station. Underground/Deep projects cannot substitute a nearby below-ground sink for this requirement. Explicit user reassignment validates a replacement Surface origin.
- [ ] 2.4 Implement Earth disposal and Rock crushing/disposal acceptance from r2mlvs1. Returning to the origin is not itself disposal: unloading requires acceptance or a valid onward transfer. No magic deletion at the station.
- [ ] 2.5 Account for disposal fees, bounded sink intake, transfer credits and final delivery income exactly once. Prevent loops through company facilities or repeated transfer orders from farming money.
- [ ] 2.6 Expose waiting reason and routes: no empties, no hauler, bay occupied, blocked line, full destination or lost origin. Pause new boring without making recovery dependent on more excavation.
- [ ] 2.7 Keep the actual-wagon Surface haul mandatory when tube networks are added: do not substitute a tube capsule or underground dump for collection by the assigned secondary locomotive. Area-clearing projects retain a usable bay/retreat route until their TBM and wagon sets withdraw before conversion to tubes or plant foundations.

## Group 3 - Lifecycle and acceptance

- [ ] 3.1 Save all assignments, waiting states and pending exchange state. Prefer an atomic swap with no save-visible half-state; if phased shunting is implemented, persist each phase explicitly.
- [ ] 3.2 Handle autoreplace/refit/sale, changing train length, articulated or dual-headed parts, removal of the bay, bankruptcy and ownership changes. Refuse unsafe actions before any cargo or consist mutation.
- [ ] 3.3 Test mixed Earth/Rock wagons, insufficient empty capacity, competing haulers, blocked exits and cancellation at every wait state. Verify unchanged wagon IDs and packet totals before/after repeated cycles.
- [ ] 3.4 Run M1: commission -> dig -> fill -> exchange -> secondary train crosses to Surface -> real disposal -> empties return -> boring resumes. Save/reload in every waiting state and with the hauler spanning layers.
- [ ] 3.5 Compare the M1 trace in single-player and deterministic command replay; retain wagon/packet identity and cost evidence for r2mlvs15.

## Completion conditions

The observed secondary train carries the same loaded wagons that were behind the TBM. Full spoil stops excavation, empty return restarts it, disposal has a real receiver/cost, and no depot trick or save transition creates cargo, wagons or money. General track shunting remains outside this plan.
