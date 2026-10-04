# R2MLVS9 - Launch Centers and Rockets

Status: proposed. Parent: [R2MLVS master](r2mlvs-multilayer-vehicles-and-structures.md). Depends on r2mlvs8 and r2mlvs1 cargo definitions. Production supply chains come from r2mlvs6; development fixtures may supply inventory explicitly.

## Outcome and reading

Build waterside Space Launch Centers through Surface airport controls; release orbital cargo anywhere within their required 20-tile drop radius and distribute recovered goods to train/lorry stations in a large Surface collection area. Read master AD-8/10 and I-3/4/6/7/16; [stations](../../architecture/landscape/station-tiles.md), [commands](../../architecture/commands/archi-commands.md), [cargo payment](../../architecture/economy/archi-cargo-payment.md), `src/airport_gui.cpp`, `src/station_cmd.cpp`, `src/station_base.h`, water/terraform commands, station inventory and r2mlvs8's transition contract.

## Two-area contract

| Area | Proposed/required extent | Purpose |
| --- | --- | --- |
| Surface collection area | Proposed 20-tile circular radius around the immutable center build anchor | Link same-company train and lorry stations, with their full station footprints inside the circle, to the center's cargo-handling service |
| Orbital cargo-drop zone | Required 20-tile circular radius around the anchor's projection into Space | Permit an authorized spacecraft to release a recoverable cargo pod without flying directly over the center tile |

The two areas have separate semantics/settings and overlays even when both radii are 20. Squared plane distance gives an inclusive circle: `(20,0)` and `(14,14)` are valid offsets; `(21,0)` and `(15,15)` are outside. Actual spacecraft checks use the equivalent fixed-point position calculation, not a rounded tile that admits an out-of-range craft. Clip display to map bounds without wrapping. Expanded/joined stations do not move the center anchor. Neither area grants underground catchment or a free cross-layer transfer.

Ordinary catchment and station joining do not automatically move cargo between separate Station IDs. Implement a launch-center logistics hub with explicit membership and bounded timed Surface handling jobs. Returned cargo exists in exactly one place: center inventory, a reserved handling job, a member station, or a vehicle. Separate member stations retain their own IDs and normal station-spread rules; no global catchment-limit increase is needed to simulate this relationship. All returned materials have compatible train/lorry refits and can be routed through these members.

## Group 1 - Surface facility and placement

- [ ] 1.1 Add Space Launch Center to Surface Airport construction with footprint/rotation preview, service/cargo access and cost. It is a special station facility; ordinary aircraft do not inherit launch permissions.
- [ ] 1.2 Validate the entire land footprint, station joining, ownership/authority and at least one shared orthogonal edge with navigable sea/river/lake/canal water. Reject diagonal-only adjacency and decorative water; record the exact qualifying tile predicate.
- [ ] 1.3 Protect the last required water adjacency against relevant clear/terraform/water commands. Explain the restriction; allow removal of the center after safe evacuation according to lifecycle rules.
- [ ] 1.4 Create an immutable center anchor and its orbital navigation reference with no free storage/dock/station module. Define the independent Surface collection and orbital-drop circles, edge clipping and display overlays. Docking/servicing still use their own approach points.
- [ ] 1.5 Add bounded pad queues, assembly/service capacity, Surface station cargo access and recoverable inventory. Normal trains/trucks/ships bring supplies and remove recovered resources.
- [ ] 1.6 Add same-company train/lorry collection membership within the proposed 20-tile Surface area, including joined facilities. Keep separate Station IDs and station-spread limits. An overlapping area offers a deterministic/explicit center choice; one station cannot allocate the same cargo from multiple hubs at once.
- [ ] 1.7 Implement bounded Surface handling with saved packet reservations, cargo filters and member demand. Move packets once to a member station for pickup, or reverse the path for explicitly transferred launch supplies. Preserve age/origin and charge only the specified handling cost; this is not final cargo acceptance/payment.
- [ ] 1.8 Revalidate membership on station extension/removal/ownership change and full-footprint range changes. Exclude below-ground/same-XY stations. Settle or return in-flight handling inventory on unlink; changing membership cannot copy cargo or strand it at a reused Station ID.

## Group 2 - Launch and payload lifecycle

- [ ] 2.1 Define rocket purchase/service, propellant consumption, payload mass/capacity, preparation and turnaround. Permit probe transport, components, Robotics Parts and mirror kits; do not silently convert ordinary aircraft into rockets.
- [ ] 2.2 Implement Ground Loading -> Preparing -> Ascent -> Orbital Delivery -> Return/Recovery -> Service. Specify reusable rocket economics for the first release; payload probes remain orbital. Save vehicle/payload identities throughout.
- [ ] 2.3 Reserve pad/approach and cargo before launch. Move cargo into the rocket hold, then to its designated orbital inventory or deployed unit exactly once. Space deployment is a synchronized action, not an animation callback.
- [ ] 2.4 Define probe-as-payload identity before and after deployment. Avoid counting the same probe as an independently ticking unit while carried. Its target/home reference survives launch.
- [ ] 2.5 Add the return-pod flow used by any compatible cargo spacecraft, including r2mlvs10 probes: accept release at a current Space position within 20 tiles of the explicit authorized target center, reserve recovery capacity, move packets into a saved descent job, then unload once into that center's Surface hub storage. Save source release position, target and timer; recover/replace pods through explicit cost/service rules. An out-of-range release fails without mutation.
- [ ] 2.6 Distinguish unloading to station storage from final industry acceptance/payment. Descent itself earns no freight income and does not reset cargo age or origin. Preserve all delivery history used by CargoDist/accounting.
- [ ] 2.7 Route cargo-only return missions to a valid point inside the target drop zone rather than requiring the anchor tile. Select the target explicitly where orbital circles overlap; nearby foreign centers receive no automatic cargo. Service/docking orders may still need the precise facility approach.

## Group 3 - Failures, presentation and acceptance

- [ ] 3.1 Handle blocked pads, full recovery storage, cancelled launch orders, lost home centers, ownership changes, failed service budgets and unavailable orbital destinations. Hold/return/divert via explicit valid alternatives without cargo deletion.
- [ ] 3.2 Refuse center demolition while cargo/vehicles/jobs would be orphaned, or complete an explicit evacuation transaction. A station ID later reused by the pool must not become an old mission's new home accidentally.
- [ ] 3.3 Integrate launch-center, rocket, probe deployment and return-pod graphics with correct layer filtering, occlusion and vehicle selection. Show payload, phase, fuel/service and queue reasons.
- [ ] 3.4 Test water predicates/orientations, all placement entry points, two launches contending for a pad and rocket ordinary-airport refusal.
- [ ] 3.5 Save/reload and replay during every launch/descent phase. Verify packet conservation and payment history, including full station storage and invalid targets.
- [ ] 3.6 Run an actual Surface freight delivery -> rocket payload -> Space inventory -> descent -> Surface collection loop with repository-local test content/configs.
- [ ] 3.7 Test release at the center, exactly 20 tiles away, just beyond the fixed-point boundary, valid/invalid diagonals, map edges and overlapping centers. Include moving craft and reordered commands; release/target validation occurs before any hold mutation.
- [ ] 3.8 Recover several cargo types from orbit and collect them through separate train and lorry stations near the Surface area boundary. Test full stations, two consumers contending for one packet, cargo filters, station expansion out of range, deletion, unlinked/foreign/underground stations and no dependence on global station-spread settings.
- [ ] 3.9 Save/reload during off-center descent and Surface handling. Verify total packets/age/provenance and no income generated by repeatedly relinking or routing cargo around the hub.

## Completion conditions

The airport controls build a water-adjacent launch facility whose large Surface collection area supplies separate train and lorry stations. Compatible spacecraft can drop cargo anywhere within the inclusive 20-tile orbital circle, without an exact-center rendezvous. Descent and subsequent Surface handling deliver each packet once to the intended inventory; range changes, animations and repeated saves cannot create or lose goods.
