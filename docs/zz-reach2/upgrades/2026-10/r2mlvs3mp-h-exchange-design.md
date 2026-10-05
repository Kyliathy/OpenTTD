# R2MLVS3MP-H - Wagon exchange design (master Group 5, Plan 5 Groups 1-7)

Owner: [R2MLVS3MP](r2mlvs3mp-master-plan.md) Group 5. Implements [Plan 5](r2mlvs5-spoil-wagons-and-haulage.md) Groups 1-5 and the exchange edges of [Plan 4](r2mlvs4-tunnel-boring-machines.md) task 4.2B. Parent decision: AD-6 (atomic real-wagon movement, mandatory Surface origin leg). Architecture: [map layers](../../architecture/map-layers/archi-map-layers.md).

## Bay

A bay is the place where both consists stand still, wholly on one rearrangement tile, as trains do inside a depot.

| Kind | Rearrangement tile | Valid when |
| --- | --- | --- |
| Source | The project's Surface source portal (`project.portal`), the cutter's assembly site | The pair is the project's canonical key. The cutter is built there (Track::Depot, hidden, stopped). |
| Depot | A company rail depot on a rock layer | The depot tile and an approach of `ceil(cutter consist length / TILE_SIZE)` straight tiles in front of its door are Excavated, plain rail on the door axis, same owner, railtype compatible with the cutter consist. |

- `project.exchange_bay` (already saved) names the bay: the portal key for the source bay, or the depot tile.
- `CmdSetTBMExchangeBay(project, tile)` designates a bay after `ValidateTBMExchangeBaySite`.
- A participant is admitted only when it is the company's front engine, not crashed, stopped, `cur_speed == 0`, and every part is on the rearrangement tile with `Track::Depot`. A depot gives this natively (`IsStoppedInDepot`).
- Moving into the bay is not part of Groups 1-5. Tests place real consists with a fixture. Movement comes with the hauler cycle (Plan 5 Group 6) and master Group 7.

## Exchange

- Request: project, hauler head. Loaded units are every TBM exchangeable unit that carries spoil. Empties are every hauler wagon unit with no cargo whose cargo is ERTH or ROCK.
- Final chains: TBM = permanent prefix, then empties in Earth-then-Rock order. Hauler = head unit, then the loaded units in TBM order, then its remaining parts (a rear dual head stays last).
- The cutter can never be the hauler (2.3). The hauler is a non-TBM front engine of the same company.
- Checks without trial (2.1-2.3): unit boundaries, ownership, every loaded part holds only Keep cargo (no Transfer, Deliver or Load actions), empties hold no cargo, and empty Earth/Rock capacity is at least the frozen pass terms.
- Trial (2.4): back up both chains, relink to the final chains, then run the shared `CheckTrainAttachment` (length and coupling callbacks). For every part, recompute the capacity that `ConsistChanged(CCF_ARRANGE)` would use, with the new head as first engine. Refuse if any part would hold less than its cargo, because that path truncates cargo. Restore both chains. The trial consumes no RNG and changes no cache.
- Commit (Group 3): the same relinking, now kept. Then the fixed bookkeeping of `MoveRailVehicleParts`: head statistics out, `NormaliseSubtypes`, `NormaliseTrainHead` (consist caches, group follow), head statistics in, window invalidation. `CheckCargoCapacity` is never called. Nothing after the trial can fail, so no later restore is needed.
- Primitive: `ExchangeRailWagonUnits` in `train_cmd.cpp` (declared in `train_cmd.h`). It has no command of its own. Ordinary `CmdMoveRailVehicle` keeps its depot guard unchanged (1.3).

## Assignment and reservation

`TBMExchangeAssignment` {id, project, hauler, bay, set, phase, wait, phase_ticks, exchanges, revision} is the reservation. Each hauler has one assignment; a project and its bay may have several, queued as described under Wagon sets and queue. `AssignTBMHauler` needs the project's Surface origin station (`MissingOrigin` otherwise) and a valid bay. Saved from version 378 (see Persistence below). Production commissioning stays guarded until master 6.6, so before then only tests and scripted fixtures create assignments.

## TBM phase edges (Plan 4 4.2B)

- ReturnForExchange -> WaitForEmpties when the cutter consist is admitted at its bay.
- WaitForEmpties -> Resume when no exchange is due any more (`IsTBMExchangeDue` is false, so the actual wagons hold no spoil).
- Resume -> TravelToFace, or Completed when the route is complete.

## Origin admission (Plan 5 4.2)

`CheckTBMOriginAdmission(project)`: the origin is a company-owned Surface railway station, and real Earth and Rock receivers exist (Plan 5 Group 8). It refuses `NoReceiver` until Group 8 delivers them.

## Hauler cycle (Plan 5 Group 6)

The assignment carries a saved phase, a wait and a phase timer. Travel uses the hauler's ordinary orders, which the controller never rewrites. Each step observes where the hauler actually is.

| Phase | Ends when | Verified against |
| --- | --- | --- |
| AwaitExchange | An exchange executed for this hauler: the controller executes it itself once both trains are admitted and the cutter is in WaitForEmpties | Both admissions; `ExecuteTBMWagonExchange` |
| CollectLoaded | The hauler is no longer wholly at the bay | Actual part positions |
| VisitOrigin | The hauler's head stands on a tile of the project's origin station; the set records the visit | Actual tile and station |
| DisposeOrTransfer | Every wagon of the carried set is empty (disposal or onward transfer emptied it) | Actual cargo of each set wagon |
| ReturnEmpties | The hauler is wholly at the bay again with the empty set | Actual admission and cargo |

- Waits (recoverable pauses, cargo untouched): Stopped (the player's stop flag outside the bay), Crashed, WrongWagons (a set wagon is not in the hauler), LostOrigin, LostProject, NoEmpties (the exchange refused for capacity), OccupiedBay (another hauler holds the bay's turn).
- Cancellation is `ReleaseTBMHauler`. It ends the assignment and leaves every wagon and packet where it is; a later assignment can recover the set.
- The step runs once per hauler tick from `Train::Tick`, beside the cutter's `AdvanceTBMWork`.

## Wagon sets and queue (Plan 5 Group 7)

- `TBMWagonSet` {id, project, wagons (ordered unit heads), state, visited_origin, revision}. States: Assigned (serving the cutter), Loaded (on a hauler after an exchange), ReturningEmpty (on a hauler, emptied), Available (released). Each wagon is in at most one set; each set is carried by at most one hauler.
- Assigning a hauler registers its empty spoil units as a new ReturningEmpty set, or adopts the one unowned set of this project that its wagons form. The adopted set must be wholly aboard (`WrongWagons` otherwise); a carried set, the cutter's Assigned set or another project's set is `Reserved`. The cutter's units become a set (Assigned) at the first exchange. An exchange swaps the two: the cutter's set becomes Loaded on the hauler, and the hauler's set becomes Assigned on the cutter.
- Several haulers may serve one project. Each hauler has one assignment. The bay serves one exchange at a time, in accepted request order (assignment id), and later haulers wait OccupiedBay.

## Persistence (Plan 5 Group 10)

- Version `TBMExchange` (378), `src/saveload/tbm_exchange_sl.cpp`:
  - `TBXH` header: format 1, assignment count, assignment cursor, set count, set cursor, wagon row count.
  - `TBXA`: one row per assignment, in identity order.
  - `TBXS`: one row per set, with its wagon count.
  - `TBXW`: (set, wagon) rows in set order, then chain order.
- Each row is checked for bounds and known values, then staged. `FixPointers` attaches the wagons to their sets and calls `StageLoadedTBMExchange`.
- `AcceptLoadedTBMExchange` runs in `AfterLoadGame` right after `AcceptLoadedTBMProjects`. It publishes nothing unless every check passes:
  - Sets: identities increase and stay below the cursor. Each set has a project, a known state, a stamp and at least one wagon. Every wagon is a live wagon unit head of the project's company, in one set only. An Assigned set rides on the project's cutter.
  - Assignments: identities increase and stay below the cursor. Each assignment has a project, a known phase and wait, and a stamp. Its hauler is a live front engine of that company, is not a cutter, and has only this assignment. A carried set exists, serves the same project, is not Assigned, and has only this carrier.
- Not checked on purpose: where a non-Assigned set is, or whether the phase fits the set's state. The hauler's observed holds (WrongWagons and the rest) report these after load, as they would during play.
- Packets are never in the store; they stay in each wagon's cargo list. The head and wagon lookups are searches, so nothing is rebuilt.
- The exchange happens inside one hauler step, so a save is always before or after it.
- Metadata: `MULTILAYER_FEATURE_TBM_EXCHANGE` (128) is set while either cursor is non-zero. It requires `MULTILAYER_FEATURE_TBM_PROJECTS`. Its digest record kind is `Exchange`.
