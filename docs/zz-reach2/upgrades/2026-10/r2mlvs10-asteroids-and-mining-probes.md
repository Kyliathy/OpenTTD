# R2MLVS10 - Asteroids and Mining Probes

Status: proposed. Parent: [R2MLVS master](r2mlvs-multilayer-vehicles-and-structures.md). Depends on r2mlvs9.

## Outcome and reading

Slowly moving asteroids are finite deposits. A launched robot intercepts, attaches, mines and returns within the 20-tile orbital drop radius of its home launch center to send cargo down for train/lorry collection. Read master AD-9/10/11 and I-3/5/6/9/16; r2mlvs8's entity/motion contract, r2mlvs9's payload/descent/hub contract, [vehicle ticking](../../architecture/vehicles/archi-vehicles.md), [game loop](../../architecture/game-loop/archi-game-loop.md) and [savegame](../../architecture/savegame/archi-savegame.md).

## Group 1 - Asteroid generation and motion

- [ ] 1.1 Define metallic/Nickel, carbonaceous/Water Ice and rich/Rare Earth bodies with stable ID, finite reserve, docking capacity, visual size and deterministic composition. Start with one main output per class to keep probe holds/refits clear.
- [ ] 1.2 Generate bodies from the saved Space feature seed without Surface RNG draws. Specify counts and an initial reachable target; do not rely on building a station to discover the first asteroid.
- [ ] 1.3 Implement slow fixed-point drift on a saved schedule. Publish the initial numerical speed and deterministic edge policy; attached probes follow the same authoritative motion.
- [ ] 1.4 Define depleted-body persistence/removal and any replenishment policy explicitly. Default finite bodies do not spontaneously refill on save/load or after detaching a probe.
- [ ] 1.5 Enforce the space-band/approach policy from r2mlvs8 so bodies cannot invalidate static module footprints or teleport latched probes through edges.

## Group 2 - Probe mission controller

- [ ] 2.1 Create and carry a small probe through the launch lifecycle. Add home-center and target-body orders, hold/refit choice, costs, speed, extraction rate and service budget.
- [ ] 2.2 Implement Deploy -> Rendezvous -> Attach -> Mine -> Detach -> Return -> Descent Handoff -> Service/Repeat. Interception follows a body ID/current predicted position rather than an obsolete tile order.
- [ ] 2.3 Reserve an attachment slot with deterministic competition handling. Define a finite approach timeout and retarget/return behavior if the target becomes unavailable.
- [ ] 2.4 While attached, bind position to the body and consume reserve into the probe hold once per work step. Full hold, empty reserve, low service budget and cancellation trigger safe detachment.
- [ ] 2.5 Return to any reachable release point inside the originating center's inclusive 20-tile circle and create a capacity-checked descent job from r2mlvs9. Do not require flight to the anchor tile; explicit service orders may have a separate precise approach. The probe keeps cargo until the handoff commits; it cannot start another cycle with duplicated pending cargo or silently use a foreign/overlapping center.
- [ ] 2.6 Service/rearm/refuel through explicit supplies or priced servicing defined in r2mlvs1/9. The basic loop needs no orbital station and no new rocket per batch.

## Group 3 - UI, art and acceptance

- [ ] 3.1 Add asteroid selection, reserve/composition/drift information, assignment controls and mission status. Show clear attachment/mining and return-pod states using r2mlvs13 art.
- [ ] 3.2 Test moving-target interception, two probes racing for one slot, partial holds, depletion, service limits, cancelled orders, removed home and edge motion while attached.
- [ ] 3.3 Verify reserve + probe hold + descending cargo + delivered/consumed totals over repeated cycles. Save/reload during rendezvous, attachment, mining, detachment and descent handoff.
- [ ] 3.4 Run one full mission for each asteroid class, including drops away from the center and at the 20-tile boundary. Collect returned cargo through both linked Surface train and lorry stations and deliver it to useful receivers. Compare visible/hidden layer results and multiplayer replay.
- [ ] 3.5 Measure target search and per-body/per-probe costs at the proposed population limits; avoid all-pairs scanning every tick.

## Completion conditions

Asteroid movement is observable, slow and deterministic. Probes physically attach and mine moving bodies, then return into the correct center's 20-tile drop zone and deliver accountable Surface cargo through its collection hub. The first useful mission completes without an orbital station, and all interrupted states can recover.
