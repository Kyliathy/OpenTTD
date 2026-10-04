# R2MLVS15 - Integrated Acceptance and Balance

Status: proposed. Parent: [R2MLVS master](r2mlvs-multilayer-vehicles-and-structures.md). Depends on required groups in r2mlvs1..14, r2mlvs16 and r2mlvs17. This plan owns final combined evidence, not duplicated feature implementation tasks. Acceptance remains last even though later-added feature plans have larger numbers.

## Outcome and reading

Demonstrate a playable connected economy with finished graphics, conserved materials, working save/multiplayer behavior and measured costs/performance. Read the master's requirements, milestones and invariants; feature acceptance records; [level 0 architecture](../../architecture/@archi-ttd-level0.md), [map layers](../../architecture/map-layers/archi-map-layers.md), [savegame](../../architecture/savegame/archi-savegame.md) and the [R2MLM build commands](r2mlm-multi-layered-map.md#build-and-test-commands).

## Scenario register

| ID | Scenario | Evidence required |
| --- | --- | --- |
| S1 | Sealed entrance and general area clearing | Ordinary traffic refused; portal-commissioned TBM clears corridors/irregular chambers with maximum 45-degree advancing turns, returns real spoil and recovers before a bay exists; cleared ground can later become tubes or plant foundations |
| S2 | Recurring spoil logistics | Actual wagon IDs move from TBM to secondary train; it reaches the original Surface station, disposes/transfers to a real sink, returns empties and restarts work |
| S3 | Underground economy | All five Underground resources are delivered to working Surface consumers; rejected solid-rock construction across command families |
| S4 | Deep economy and calendar | All Deep entry/construction/operation paths refused on 31 December 1979 and available from 1 January 1980; five additional resources; train uses two layer transitions and truck a timed elevator; legacy saves preserve their explicit policy |
| S5 | Space mining and collection hub | Waterside center built from Airport controls; probe mines moving asteroid; cargo release at a point within the inclusive 20-tile circle; descent and bounded Surface handling supply separate train and lorry stations |
| S6 | Solar support | Surface kit manufacture/delivery; launched mirror; valid assignment; 100 baseline -> 400 effective useful power, capped despite extra mirrors |
| S7 | Orbital manufacturing | Surface-built components form Core/Dock/Assembly; heavy robot mined input feeds a processor; real product reaches a useful destination |
| S8 | Lifecycle stress | Queues, full wagons/buffers, funds exhausted, cancellation, unsafe removal, target ID reuse, takeover/bankruptcy and recovery |
| S9 | Persistence and network | Legacy/new saves, every mid-action state, headless progression and live multiplayer join with different visible layers |
| S10 | Art and scale | Complete direction/state/zoom gallery, toolbar/extra viewport behavior, representative map/entity performance and distribution hashes |
| S11 | Five original tube vehicles | Mole-70, Crate-75, Cistern-80, Shield-85 and Forge-05 have distinct designs/stats and useful cargo routes on excavated tube networks, with power, dates, mode separation and conventional cross-layer transfers |
| S12 | Geothermal technology progression | 1980 vent power, 2000 improved generation, 2010 direct smelting, 2030 batteries and 2045 Mantle Forge; shared heat budget, transported inputs, useful output and explicit electricity export |

## Group 1 - Fixture and build readiness

- [ ] 1.1 Record source revision, content/profile versions, configs, map seeds, toolchain and fixture hashes. Keep fixture saves and logs in the repository; preserve original user saves.
- [ ] 1.2 Establish feature-disabled regression fixtures and a compact fully connected Multilayer fixture. Test-only supply shortcuts are labeled and cannot be used as proof that the bootstrap economy works.
- [ ] 1.3 Build Debug and RelWithDebInfo game/test targets and required asset packages with the existing repository toolchain. Run focused feature tests and the full appropriate CTest/regression suites; record exact current counts and new warnings.
- [ ] 1.4 Distinguish existing known baseline failures (including previously recorded legacy TTO conversion issues) from regressions. A historical skip or failure is never silently renamed a passing check.

## Group 2 - Playable loops and failure recovery

- [ ] 2.1 Execute S1/S2 as the first acceptance milestone. Record excavation progress, generated material, wagon IDs, packets, station acceptance and total costs over several cycles.
- [ ] 2.2 Execute S3/S4 with all ten distinct cargos and the complete freight fleet. Confirm no receiver, terrain or capacity dead end makes any required resource decorative.
- [ ] 2.3 Execute S5/S6 through normal controls, including invalid launch placement and mirror conflicts. Verify real Surface collection and useful power accounting, not only animations/UI counters.
- [ ] 2.4 Execute S7 from Surface supplies without cheat-built orbital predecessors. Check exact module/robot/processor inventories and outputs.
- [ ] 2.5 Execute S8/S9 using the r2mlvs14 lifecycle/save matrix. Include long hidden-layer operation, multiple simultaneous projects, live join and repeated save/load at queue boundaries.
- [ ] 2.6 Capture player-facing confusing/blocked states and fix their controls/messages. Confirm commands, scripts and shortcuts cannot bypass hidden toolbar restrictions.
- [ ] 2.7 Execute S1's geometry cases: straight/45-degree cutting, a 90-degree change using two advancing bends, illegal single-step/pivot/corner-cut refusals, safe backing and a large irregular footprint subsequently used for tubes/geothermal facilities. Account for all swept-space excavation and spoil.
- [ ] 2.8 Execute S5 boundary/collection cases: valid `(20,0)`/`(14,14)` offsets, invalid `(21,0)`/`(15,15)`, fractional boundary crossings, map edges, overlapping target centers, full recovery capacity and separate train/lorry members. Test station expansion/unlink/deletion during handling and exclude same-XY Underground members.
- [ ] 2.9 Execute S11 with all five original tube vehicles and final directional/load art, including a paid-grid first route, geothermal supply later, introduction dates, command refusals on solid/Surface/Space/locked Deep, incompatible rail/portal access and save/load while queuing.
- [ ] 2.10 Execute S12 across the proposed calendar dates using real supply chains. Demonstrate one vent shared between generators/direct smelting without excess heat use, later battery/forge production, useful Surface export and no solar-mirror or native/ETI double credit.

## Group 3 - Economy and performance

- [ ] 3.1 Measure complete project economics: boring/support/haulage/disposal, mine extraction/transport/processing, launch/service/return and mirror/station manufacture. Balance net costs and payback over usable operations rather than isolated vehicle prices.
- [ ] 3.2 Confirm active TBM costs are consequential, Deep access is meaningfully harder, spoil alone cannot finance perpetual boring and disposal saturation creates manageable logistics. Tune defaults and publish changed tables in r2mlvs1.
- [ ] 3.3 Check mining throughput against launch/pod/dock capacities and receiver demand. Ensure small probes are useful before heavy robots, and large stations improve throughput without making the first launch unprofitable by design.
- [ ] 3.4 Confirm mirrors retain the specified +300% effect while their kit/launch/service economics remain meaningful; tune cost/coverage, not the requested multiplier. Document any native grid/town-growth effect and ETI profile limitation.
- [ ] 3.5 Set and record performance budgets against the measured baseline before optimizing: feature-disabled overhead, compact/representative/maximum feasible map memory, active-job/entity tick time, pathfinding, save size/time and join cost. Report hardware and workload counts.
- [ ] 3.6 Profile dense excavation projects and orbital fleets. Resolve unbounded tile scans, all-pairs rendezvous work, inventory growth and command stalls; retest only affected budgets after fixes.
- [ ] 3.7 Balance the five tube models against underground rail/road, including paid-grid demand, terminal throughput and introduction dates. Price geothermal plants and direct-heat recipes against transported inputs, heat scarcity and conventional Surface alternatives.
- [ ] 3.8 Profile large Surface collection hubs, many handling jobs, tube reservations and thermal utility graphs alongside existing workloads. Tune handling capacity/costs and technology progression without silently changing the required 20-tile orbital radius or 45-degree TBM turn limit.

## Group 4 - Art, documentation and completion

- [ ] 4.1 Execute S10 against the r2mlvs13 manifest: real runtime captures, all required directions/states, native and scaled zoom, foreground/vehicle sorting, layer filtering and supported blitters. Record user art review separately from automated checks.
- [ ] 4.2 Update delivered architecture: layers/rules, vehicles, industries/cargos, command boundaries, new save chunks/migrations, orbital simulation, content compatibility and asset packaging. Keep proposal history in plans.
- [ ] 4.3 Add player instructions for commissioning, 45-degree area clearing, actual wagon exchange, all ten resource chains, 1980 Deep access, five-model tube transport, geothermal timelines, launch collection membership, 20-tile cargo drops, mirrors and station delivery. Explain recovery without exposing irrelevant implementation details in the UI.
- [ ] 4.4 Publish an evidence matrix mapping every master requirement/invariant to exact scenario/test/log/artifact. List all unresolved limitations and unsupported content profiles honestly.
- [ ] 4.5 Close each child only when its completion conditions and required checks are satisfied, then reconcile the master index. An explicitly authorized skip remains recorded as unverified and does not imply runtime, network or art acceptance.

## Completion conditions

All requested gameplay, five original tube vehicles, geothermal progression and the planned M4 expansion are demonstrably connected, supplied with final graphics and supported by save/network evidence. Calendar gates, TBM turn geometry and the two launch-center areas have boundary tests; no visual counter substitutes for real material transfer or heat allocation. Performance and balance claims have measurements; remaining limitations are explicit rather than hidden behind checked boxes.
