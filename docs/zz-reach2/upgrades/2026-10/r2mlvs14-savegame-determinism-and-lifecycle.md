# R2MLVS14 - Savegame, Determinism and Lifecycle

Status: proposed. Parent: [R2MLVS master](r2mlvs-multilayer-vehicles-and-structures.md). Group 1 uses draft definitions from r2mlvs1 Groups 1/2 and feeds its Group 3 closure, before new world-state implementation. Remaining groups accompany feature plans and audit their integrated result; this is not a final-stage serialization patch.

## Outcome and reading

The complete economy survives saves, multiplayer joins and entity removal without corruption, duplication or dependence on the visible layer. Read master AD-1/14 and all invariants; [savegame](../../architecture/savegame/archi-savegame.md), [desync](../../architecture/desync/archi-desync.md), [commands](../../architecture/commands/archi-commands.md), [game loop](../../architecture/game-loop/archi-game-loop.md), `src/saveload/saveload.h`, `src/saveload/map_sl.cpp`, `src/saveload/vehicle_sl.cpp` and `src/saveload/afterload.cpp`.

## State ownership register

| Authoritative state | Feature owner | Derived data rebuilt after load |
| --- | --- | --- |
| Gameplay profile, content identity, tuning, feature seeds, Deep/model/factory availability and explicit legacy/editor policies | 1 | Resolved capability/content/calendar lookups |
| Excavation, deposit reserve, claim, job progress/support | 2 | Survey/render caches and work queues |
| Sealed/commissioning portal and TBM access assignment | 3 | Normal and work-route connectivity caches |
| TBM state, cutter heading, area outline/pass progress, face, costs/remainders, wagon references | 4 | Vehicle display and route summaries |
| Exchange assignment, origin station, hauler/empty sets | 5 | Queue views; train caches after validated consist restoration |
| Mine/deposit binding, reserve consumption, processor inventory | 6 | Catchment and production UI caches |
| Orbital identity/position/motion, typed targets, transition phase | 8 | Spatial indexes and interpolation state |
| Payload ownership, immutable launch anchor, drop release position, target, pad queue, descent and service timers | 9 | Drop-zone/render previews |
| Launch hub memberships, collection radius, stock reservations and timed Surface handling jobs | 9 | Membership lookup, demand queues and collection overlay |
| Asteroid reserve, probe attachment and extraction remainder | 10 | Target search acceleration |
| Blueprint reservation, inventory, consumed inputs, assembly progress | 11 | Module adjacency and displayed capacity |
| Mirror target, service/coverage and output remainder | 12 | Farm mirror lookup and drawn beam |
| Tube vehicle capability/definition IDs, cargo/orders, service and paid-power demand | 16 | Route/reservation and power-availability caches |
| Vent identity/capacity/claim, utility connectivity, allocated heat, conversion/recipe remainders and export | 17 | Thermal/electrical distribution graphs and heat overlays |

Ordinary cargo packet/vehicle/station persistence should be reused where possible. Do not save a second independent count of the same physical cargo without a strict ownership/consistency rule.

## Group 1 - Schema and migration foundation

- [ ] 1.1 Assign each state field one owner and representation. Define stable typed references, valid ranges, limits, optional/invalid semantics and reference removal hooks before producers write data.
- [ ] 1.2 Reserve symbolic save-version changes and unique chunk names only after checking the current tree; numeric 371 is not assumed free forever. Keep layers 0/1/2/3 and established saved enum meanings unchanged.
- [ ] 1.3 Implement feature/profile metadata and an empty-compatible schema scaffold with load/preview validation. Missing required content produces a specific error before unsafe object restoration.
- [ ] 1.4 Define legacy load policy for Surface-only saves, versions 369/370 and existing content sets: preserve legacy free underground building and working portals under their saved compatibility mode. New profile generation starts solid; loading old infrastructure must not bury it.
- [ ] 1.5 Keep an existing-world conversion tool out of initial scope unless separately designed with cargo/content compatibility and excavation reconstruction. Never auto-enable the new economy by merely changing a setting on load.
- [ ] 1.6 Define command serialization and deterministic state hashing for jobs/space entities, feature RNGs, fixed-point motion, tick-based waits and production remainders. Add these fields to desync diagnostics.
- [ ] 1.7 Include the 1980 Deep policy and later model/factory dates in profile/version compatibility. Calendar availability is derived from the simulation calendar and saved policy, never local wall time. Explicit legacy/editor exceptions must not leak into ordinary new games.

## Group 2 - Feature persistence integration

- [ ] 2.1 Require each feature plan to deliver its state descriptors, after-load reference fixups and meaningful mid-action fixture alongside implementation. Collect a matrix of states covered, not just idle save samples.
- [ ] 2.2 Restore cross-references in a documented order: maps/profile -> entities/vehicles/stations -> references/cargo ownership -> validated consist/mission links -> derived caches. Reject malformed/impossible states without partial gameplay mutation.
- [ ] 2.3 Bound pools, arrays, inventories, map metadata and counters; test invalid IDs, duplicate ownership, integer overflow, bad coordinates and cyclic module/assignment references.
- [ ] 2.4 Verify estimate/execute semantics for portal construction, excavation, wagon swaps, launch/deployment, construction consumption and mirror assignment. Failed estimates or retries cannot consume RNG, inventory or funds.
- [ ] 2.5 Verify saves during TBM cuts, straddling consists, queue waits, probe attachment, ascent/descent, partial modules and partial solar periods resume at the same simulation result.
- [ ] 2.6 Round-trip TBM 45-degree heading/pass transitions, off-center drops at the 20-tile boundary, hub handling packets, all five tube models and geothermal heat allocations/direct-processing inputs. Include these in command/network hashes and validate unique packet/energy ownership after load.

## Group 3 - Entity lifecycle and multiplayer

- [ ] 3.1 Write a lifecycle policy table for sale, removal, closure, bankruptcy, takeover, cancellation, service exhaustion and target ID reuse for every cross-reference. Prefer clear refusal for unsafe removal; implemented evacuation/reassignment must be deterministic and conserve packets.
- [ ] 3.2 Exercise lost origin station, destroyed/removed exchange bay, mine depletion, launch center removal, asteroid deletion, module dependency removal and solar-farm replacement. No stale ID can silently bind to a newly allocated object.
- [ ] 3.3 Run host/client identical-command scenarios with different selected layers/viewports and rendering settings. Compare synchronized state hashes over complete underground and orbital loops.
- [ ] 3.4 Join a multiplayer game while excavation, wagon waiting, probe mining, descent and assembly are active. Verify the joining snapshot and subsequent hashes; single-player command replay is not a substitute for join testing.
- [ ] 3.5 Check dedicated/headless play, pause/speed changes, supported economy time modes and bounded updates. GUI-less worlds must progress identically.
- [ ] 3.6 Stress collection-member extension/unlink/ownership changes during handling, overlapping launch zones, loss of tube power/depot/interchange, vent claim reuse and broken electrical risers. Replay across the 1979/1980 calendar boundary and later introductions; no command retry may bypass a date, radius, turn or heat-capacity rule.

## Completion conditions

Every long-running activity has tested authoritative persistence and safe reference lifecycle. Old supported worlds keep working with their original economy. Multiplayer replay and mid-activity joins agree across views, and no save/reference/action failure creates duplicate cargo, mineral reserves, wagons or power.
