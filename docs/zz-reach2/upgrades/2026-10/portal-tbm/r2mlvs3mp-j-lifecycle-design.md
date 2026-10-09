# R2MLVS3MP-J - Exchange lifecycle design (master 6.2, Plan 5 Group 11)

This note covers how the exchange store survives everything that can happen to trains, bays and companies:
- sale, refit and autoreplace of a TBM consist or a hauler;
- a train being deleted in a crash or a bankruptcy;
- removal of an exchange bay;
- a company merger or removal.

It builds on [r2mlvs3mp-h](r2mlvs3mp-h-exchange-design.md) (exchange, sets and persistence) and [r2mlvs3mp-i](r2mlvs3mp-i-receiver-design.md) (receivers).

## Rule

Every state the game can reach must also load. The load check `AcceptLoadedTBMExchange` repairs nothing. So each event below either leaves the stores consistent or is refused before anything changes.

## The cutter consist is locked (11.1, 11.2)

While a project record names a train's front engine as its cutter, ordinary commands may not change any part of that train:
- ordinary moves, whether a part leaves the train or is attached to it;
- sale of any part, including the whole train;
- refit;
- autoreplace and autorenew, which were already refused for cutter engines.

The refusal message is `STR_ERROR_TBM_CONSIST_LOCKED`. Only the project's restricted owners still change the consist: portal assembly, the wagon exchange and support work.

Why sale is refused too: a sold cutter leaves dangling references.
- The portal work record of a Commissioning pair still names the cutter.
- The Assigned set would no longer ride on the cutter.

A saved game in that state cannot load. Releasing the consist of a finished or cancelled project is project retirement, which belongs to Plan 4 Group 17 (master 6.3).

The TBM hoppers have an empty refit mask (Plan 1 content), so refitting them already did nothing. The new refit check also covers other ERTH/ROCK wagons inside a cutter consist.

## Haulers and wagon sets (11.1, 11.2)

| Event | Rule |
| --- | --- |
| Sale | A unit of a wagon set that carries cargo refuses (`STR_ERROR_TBM_SPOIL_ABOARD`). The spoil must be delivered first. Anything else may be sold: the locomotive, empty set wagons, units outside any set. |
| Refit | A unit of a wagon set refuses (`STR_ERROR_TBM_SET_REFIT`), manual or automatic, because its Earth/Rock capacity is part of the set. The locomotive and units outside any set may be refitted. |
| Autoreplace and autorenew | A train with an assigned hauler or any set unit is skipped, like a cutter train. A replacement would give the wagons new vehicle IDs and break the set. To replace them, release the hauler first. |
| Ordinary moves | Allowed. A set wagon taken out of its hauler is the recoverable WrongWagons hold. |
| Length or articulation changes | Every exchange checks lengths, coupling and capacity again (Group 2). A smaller set leads to the NoEmpties hold. |

Recovery when only the locomotive of a loaded hauler is sold:
1. The loaded wagons stay in the depot with their spoil.
2. The player couples them to any locomotive.
3. `AssignTBMHauler` adopts the whole set and resumes VisitOrigin, or DisposeOrTransfer if the set already visited the origin.

## Deleted trains (all paths)

`Vehicle::PreDestructor` calls `OnTBMExchangeTrainPartDeleted(part)` for every train part. This covers sales, crashes, bankruptcy and flooding: every path except cleaning the whole pool.

- The part is an assigned hauler: its assignment is released, as `ReleaseTBMHauler` does. An empty set becomes Available; a loaded set keeps its spoil and its visit flag.
- The part is a set wagon: it leaves the set and the set's revision advances. When the set is empty:
  - the set is erased;
  - a hauler carrying it is released, because it has nothing left to haul.
- The part is a project's cutter: the project's Assigned set no longer rides on a cutter, so it becomes:
  - Loaded (not visited), if any of its units carries cargo, so a hauler can adopt it and take it to the origin;
  - Available, otherwise.

  With the lock above, only a crash or a bankruptcy reaches this case, and both delete the wagons too. It is a safety net.

So no saved set or assignment can name a deleted vehicle. A reused vehicle ID can never join a set without being added on purpose.

The common hook is `OnTBMTrainPartDeleted` (master 6.5). After the exchange store, it clears a deleted cutter from its project (no parts, `MissingCutter`) and from its Commissioning pair, which keeps its binding with no cutter. A train that later takes the ID inherits nothing. Such a project is ended with Cancel, which makes it Cancelled at once whatever its job's state, and then erased with Retire, which withdraws the pair.

## Deleted stations (master 6.5)

`Station::~Station` calls `OnTBMStationDeleted(station)` on every deletion but pool cleaning:
- A project whose origin it was loses the origin at once. An unfinished project shows `MissingOrigin` until a new origin is set.
- A pair naming it forgets it (`ForgetLayerPortalWorkOrigin`). An explicit origin is optional in the record, so the pair still loads.

Before this hook, a pair naming a deleted station failed its load check, and a finished project, which no longer steps, could take a reused station ID as its origin.

## Removing a bay (11.3)

The Source bay is the project's portal. Removing it belongs to Plan 3 Group 11 (master 6.4). This note covers depot bays.

Before removing a depot that is a project's bay, `RemoveTrainDepot` checks two things in order:
1. It refuses an occupied depot (`EnsureNoVehicleOnGround`). No train is touched, so a bay with the cutter or a hauler in it cannot go.
2. `CheckTBMExchangeBayRemoval(tile, flags)` refuses with `STR_ERROR_TBM_EXCHANGE_BAY_IN_USE` while an unfinished project uses this bay and its Source bay does not resolve. Such a project would be left with no supported exchange.

When the depot is removed, `ReleaseTBMExchangeBay(tile)` runs:
- Every project using the bay falls back to its Source bay, or no bay when its portal is gone.
- Each assignment queued for the removed bay moves to the project's new bay. The haulers were not in the depot, so only reservations without a train are changed.

Bankruptcy and flooding (`DoCommandFlag::Bankrupt`, `OWNER_WATER`) never refuse, but they still release.

Damaged approach rail only makes the bay stop resolving. The project then waits on `MissingExchange` until the rail is rebuilt.

## Merger and company removal (11.4)

`ChangeOwnershipOfCompanyItems` calls `ChangeTBMOwnership(old_owner, new_owner)` after the vehicle loops and before the tile loop.

- Merger: every project of the old company moves to the buyer (`company`, revision), and so does its excavation job (`ChangeExcavationJobOwnership`). Assignments and sets take their company from the project, and the vehicles and stations move in the same call, so the exchange store needs no change. No cargo moves.
- Removal:
  - The vehicles were already deleted, and the deletion hook emptied their sets and assignments.
  - The company's project records are erased. Identities are never reused, because the cursor stays.
  - No controller can ever resume an erased project's work, so its open job is cancelled (unfinished cells return to rock, cavities stay) and its Commissioning pair is released (`ReleaseLayerPortalWorkBinding`). The empty rail pair is then removed with the company's tiles.
  - Any other job of the company becomes an explicit orphan: Invalid company, unfinished work paused with `InvalidOwner`, as Plan 2's validator expects.
  - Any exchange record still naming an erased project is purged.

Plan 3 Group 11 (master 6.4) closed the merger gap: `ChangeTileOwner_LayerPortal` now moves an assigned or occupied pair with its project. Only company removal still needs an empty pair, and the step above provides one.

## Cancellation, retirement and withdrawal (master 6.3/6.4)

- Cancel (Plan 4 17.1):
  - The job is cancelled at once: unfinished cells return to rock, cavities and committed totals stay, no money moves.
  - Forward phases turn to ReturnForExchange. The cancelled job still anchors the return.
  - The exchange gives away any spoil aboard; short empties are enough, because no next pass is due.
  - Resume ends the project Cancelled.
- Retire (Plan 4 17.2): a finished project is erased once all of these hold:
  - no hauler serves it and none of its sets carries spoil;
  - its consist is withdrawn: stopped, on one tile, at its Source bay or in a rail depot, with nothing aboard;
  - its Commissioning pair can be withdrawn.

  The pair is withdrawn in the same action, its remaining empty sets are dropped, and the consist becomes an ordinary train.
- Withdrawal (Plan 3 11.3): exactly the bound job and cutter, a closed job, and no part or claim on the rock side. The pair returns to Sealed with no references.
- A lost bay (Plan 4 17.3) can be replaced with `CmdSetTBMExchangeBay` even while haulers are queued; they follow it to the new bay.

## Tests

`src/tests/tbm_exchange_lifecycle.cpp` (`[tbm-exchange-lifecycle]`):
- refused sale, move and refit of cutter parts;
- refused sale of loaded set wagons, and allowed sale of empty ones and of the locomotive;
- refused set refit and skipped autoreplace;
- the deletion hook for a hauler, its wagons and a cutter;
- bay removal: refused, released and occupied;
- merger and removal;
- a store that loads after each event.
