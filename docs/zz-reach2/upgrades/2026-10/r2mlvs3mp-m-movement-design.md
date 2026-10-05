# R2MLVS3MP-M - Movement through the project pair

Owner: [R2MLVS3MP](r2mlvs3mp-master-plan.md) Group 7B. Implements the movement scopes of:
- [Plan 3](r2mlvs3-sealed-portals-and-deep-rail.md) Group 15 (15.2, 15.3) and Group 16 (16.1, 16.2);
- [Plan 4](r2mlvs4-tunnel-boring-machines.md) Group 19 (19.1, 19.2) and the occupied-tail part of 14.3;
- [Plan 5](r2mlvs5-spoil-wagons-and-haulage.md) Group 13.

Parent decisions: AD-5 (the cutter starts from a sealed solid portal) and AD-6 (depot-like exchange bay, real wagons).
Geometry comes from [r2mlvs3mp-f](r2mlvs3mp-f-solid-starter-contract.md): source S (the Surface pair key), destination
D (the rock end, the job's anchor), exit heading H (D's door), and the straddling consist.

## Decisions

1. **The real train controller drives both trains.** Speed, acceleration, signals, reservations and the hidden flag
   stay vanilla. Nothing in the simulation reads the layer view, so a run with the layer shown equals a run with it
   hidden (Plan 4 19.1).
2. **The pair is a depot-like magazine for the project's own trains, at S.**
   - Parts stand in the depot state on S, hidden, as assembly leaves them.
   - Leaving releases one part at a time, like a depot exit: the next part leaves the depot state when the one ahead
     passes the leave point.
   - Entering takes one part at a time into the depot state. When the last part is in, the train's facing is made
     ready to leave forwards, the train stops, and the bay admission sees it whole.
   - The exit side depends on the train: the cutter consist leaves through the shaft, an assigned hauler through the
     door. This is the straddling consist of r2mlvs3mp-f: while the cutter stands at D, the parts behind it are still
     in the magazine.
3. **Admitted track.** A Commissioning pair still shows no track to ordinary routing. Only an admitted train sees its
   axis track, in the train controller, the line-end check and the pathfinder's track follower:
   - at S, the project's cutter consist (both sides) and the project's assigned haulers for that bay (door side only);
   - at D, the cutter consist only. A hauler never passes the cutter (AD-6).

   `VehicleEnterTile_LayerPortal` keeps refusing every other train.
4. **Face travel.** The cutter leaves S head first through the shaft (each released part takes the exit heading),
   crosses to D, and drives through D's door along the support rail. A TBM in a forward phase that reaches a line end
   stops there instead of reversing, and the next controller step finds it at the face (Bore holds it). AdvanceSupport
   no longer holds the train: it drives onto the new support and stops at its end.
5. **Retreat.** Entering ReturnForExchange, the controller reverses the consist once. A TBM consist always backs up
   (the last hopper leads) and is never flipped end for end, so the cutter keeps its facing and the saved heading
   stays. It backs along the support, through D and the shaft, into S's magazine.
6. **Hauler steering.** While an assigned hauler returns empties, the pathfinder's destination is its bay. Its orders
   are not written. One player order, "go to the origin station and unload", is enough for the whole loop: the hauler
   unloads at the origin, is steered back to the bay, exchanges there, and takes the next load out by that order.
7. **Starting.** A commissioned consist stands stopped. Starting it is the player's ordinary start command, after
   which the existing Commissioning to TravelToFace edge applies.

## Changes by owner

| Owner | Change |
| --- | --- |
| Plan 3 (`layer_portal_*`) | `GetLayerPortalAdmittedTrackdirs(tile, front, side)`: the axis trackdirs for an admitted train on a Commissioning pair, with the same side rule as the ordinary track status (door, or no side for a shaft step); empty otherwise. `VehicleEnterTile_LayerPortal`: admit assigned haulers at S's door, and run the magazine entry and release rules at S for admitted trains. A closed job, cancelled or with its last cut done, keeps movement authority while its project lives, so the cutter can back out for the final return. Path reservations accept a portal end's own axis. |
| Trains (`train_cmd.cpp`) | The new-tile and line-end track queries fall back to the admitted trackdirs on a portal. `CheckTrainStayInDepot` gets a portal branch (the vanilla depot reservation helpers assert on a portal tile): it releases the train from the magazine only when its controller lets it go, laying out its parts for the exit side. A TBM in a forward phase stops at a line end. A TBM consist always backs up. |
| Pathfinder (`follow_track.hpp`, `yapf_destrail.hpp`) | The follower uses the admitted trackdirs on a portal for the train it follows. The destination of a steered hauler is its bay, entered through the door; that search bypasses the shared segment cache, whose segments other trains see without the bay's track. |
| Plan 4 (`tbm_*`) | AdvanceSupport stops holding the train. ReturnForExchange reverses the consist once. A query names the magazine exit side for a train. |
| Plan 5 (`tbm_exchange.*`) | Which haulers are admitted at a bay, and the steering query. |

## Changes after the design (master 7.9)

- Decision 3, admission: once the project is finished, its hauler is admitted at the bay only while a part of it
  still stands on the key. It can take the last load out, but its empty return never brings it back in.
- Decision 2, leaving: a hauler of a finished project may leave the magazine whatever its phase.
- Servicing: a cutter with a project never gets an automatic depot order. Its bay services it whenever it backs in,
  so the game's daily service check cannot hold it as Servicing far from any depot.

## Saves

No saved field is added. A train half out of the magazine is saved like a train half out of a depot: some parts in
the depot state on S, the others on track. The steering is derived from the assignment's phase, and the facing in the
magazine from the parts' own directions. The occupied saves (Plan 3 16.2) save and reload a consist straddling the
pair, a cutter in the pocket, and a hauler in the magazine, and compare part positions and the next legal step.

## Not in this group

The recurring underground depot bay needs an underground network that reaches the support corridor without passing
the cutter. It follows once the source loop runs; until then 7.1 and 7.7 keep that half open. Free shunting on
arbitrary track stays deferred (AD-6).

## Test plan

Delivered (the runs are in the [notes](r2mlvs3mp-d-execution-notes.md#group-7b-build-and-test-runs-2026-10-05-on-mains-power)):
1. Cutter movement (`[tbm-movement]`):
   - Start, first entry to D, each cut, the advance onto the new support, and the backing return into S. A straight
     route exchanges once, at its end. On a bend that overflows one load, the consist also leaves S again with its
     empties.
   - A cancellation while the consist straddles the pair, with and without spoil, and with a player's stop on the way.
   - An ordinary train refused at a Commissioning pair's door.
   - The whole life (Plan 3 15.2): bore, retire, refusal at the Sealed door, then freight through the bored tunnel
     after connection.
2. Portal crossing (`[portal-movement]`): a freight train crossing a Connected pair, reversed while it spans the pair,
   and the signals at both ends, on both allowed rail pairs (Plan 3 15.3).
3. Source loop (`[tbm-loop]`), each with the ledger and wagon IDs at every transition:
   - the rig's straight route;
   - the bend, whose mid-route exchange sends the cutter back to bore with the empties;
   - a project commissioned through the player's commands;
   - a replay of the commissioned loop.
4. Layer shown and hidden (Plan 4 19.1): unit tests have no viewport, so an audit replaces the paired run. It finds
   every read of the view layer in GUI code only ([audit](r2mlvs3mp-l-movement-layer-audit.txt)).

Still open: saves at each wait state and with the consist straddling the pair (Plan 3 16.2, Plan 5 13.2), and the
recurring underground bay.
