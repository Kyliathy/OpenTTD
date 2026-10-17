# R2MLSP-P - Native asteroid and probe runtime handoff

Owner: [space master](../../../zz-reach2/upgrades/2026-10/r2mlsp-space-master-plan.md), [Plan10 Group22](../../../zz-reach2/upgrades/2026-10/r2mlvs10-asteroids-and-mining-probes.md#group-22---ui-art-and-acceptance), [vehicle architecture](../../../zz-reach2/architecture/vehicles/archi-vehicles.md) and [layer architecture](../../../zz-reach2/architecture/map-layers/archi-map-layers.md). Updated2026-10-17, schema438. This extends the dated [generation handoff](r2mlsp-x-generation-handoff.md) and [launch handoff](r2mlsp-q-runtime-handoff.md); their original evidence remains intact.

## Delivered ownership

`asteroid.h` owns generated stable body IDs, finite per-class reserves, saved Q16 position/velocity/remainders, frozen starter history and sparse company scan knowledge. Zero-reserve bodies remain live and moving. Generation uses its own feature streams and exact population table; loading never regenerates from current settings. `TickAsteroidField` publishes body motion, derived buckets and actual matching attachment transforms before native craft decisions. The Map/layer controller owns geometry; neither viewport choice nor a second target registry supplies simulation authority.

`space_vehicle.h` owns the actual ordinary Vehicle-pool hull, incarnation, mission, work/cost/travel remainder and exclusive body lease. `GetSpaceMiningTuning` supplies immutable model15/21 contracts. `AssignSpaceMiningMission` estimates without mutation and commits only original native admission. New synchronized `CmdSetSpaceMissionChecked(flags,VehicleID,uint64_t generation,uint32_t body)` rejects a stale incarnation at execution; the GUI uses this command for mining Recall. The original generation-free command layout and IDs remain available offline and refuse multiplayer. `UINT32_MAX` recalls while retaining the last body as nonactionable safe-departure history and preserving emitted cargo; it releases the lease and discards uncommitted work.

`CommitSpaceMiningExtraction` admits a complete ready batch through the original native packet pool and finite body/hold limits. Refusal preserves reserve, cargo and the last partial work interval. Successful output keeps Asteroid/body/Space production provenance, debits only that body's resource and resets work once. Model15 earns two units per74 working opportunities into its20-unit hold. `GetSpaceMiningDepartureDestination` delegates safe homeward departure to the existing shared occupancy/staging owner. Return reaches the explicitly selected owned inclusive20-tile circle; Visit and service berths retain precise arrival rules.

Original `SpaceManifest`, `SpacePod`, `SpaceDescentJob`, native command and paid service owners retain physical shells, allowance, pending custody, actual148-tick descent and actual148-tick service with RFMT/ELEC consumption. Release moves the original packets once into the sole descent job; capacity/identity loss retains recovery intent. Two real batches repeat before paid rearming. Original Surface handling, linked stations and actual trains/lorries carry returned ore into native refinery input and ordinary settled delivery payment. No orbital inventory, payment account or work clock is duplicated.

`GetAsteroidQuantityDisclosure` and `GetAsteroidInspectionDisclosure` reveal quantities only after the observing company's actual scan. `GetSpaceTrajectoryDisclosure` formats signed integer motion without stepping it. Current inspector disclosures include actual construction, work, service and selected-sender descent state. Functional captures are accepted; temporary sprites remain outside final-art acceptance.

## Retained executable evidence

All paths below are repository-local acceptance indexes. Their executable/source hashes describe the original dated invocation. Later builds may replace mutable build paths; consumers preserve direct index/report/save identities rather than reopening an old executable graph.

| Boundary | Acceptance index | Essential result in both configurations |
| --- | --- | --- |
| Generated field and old/new continuation | `logs/asteroid-handoff-verification.json` | 37,122 focused assertions/12 cases;30 native processes; exact saved population/motion/history and26 owners. |
| Typed roles, scan and original command dispatch | `logs/probe-mission-verification.json` | 11,726 native assertions;13 checkpoints, six continuations, company isolation and actual next-frame dispatch. |
| Construction/processing/local assembly/product replay | `logs/probe-product-replay-verification.json` | 2,949 focused/23 cases and1,862 native assertions/eight scenarios; dated paid producer inputs and actual local model15 receipt. |
| Original paid pursuit | `logs/probe-pursuit-verification.json` | Original miner16/gen2, home4/gen3, content2 receipt100000 and two actual shells; native interception92 opportunities, no orbital station. |
| Finite native extraction | `logs/probe-extraction-verification.json` | Atomic packet/finite-reserve transaction; actual74-tick batch,37 partial work and original full20-unit load. |
| Safe return and paid repeat | `logs/probe-return-verification.json`, `logs/probe-repeat-verification.json` | Actual homeward departure, two20-unit recoveries, exhausted quota, paid resupply and next Mine. |
| Phase exits and cargo-preserving Recall | `logs/probe-exit-verification.json` | All native phases/trigger policies, original paid cancellations and safe retained-body departure; later20 closes the generation-bound mission wire gap. |
| Inspector | `logs/asteroid-inspection-verification.json` | 641 focused/eight cases,435 native assertions,11 actual captures and company-gated quantities; final art pending. |
| Interception and lost references | `logs/probe-interception-verification.json`, `logs/probe-boundary-verification.json` | Maximum drift, multiple offsets, deterministic native tie, saved reflection/attachment and actual phase cancellation/recovery; depleted IDs retained. |
| Conservation | `logs/probe-conservation-verification.json` | Five original saved phases, ordinary/next/repeated continuation and all28 paid repeat checkpoints; separate per-body/per-resource equations. |
| Three resources | `logs/probe-class-verification.json` | Actual20-unit Nickel/Ice/Rare-Earth missions and148-tick recovery;24 checkpoints/720 physical cross-build comparisons. |
| Useful Surface delivery | `logs/probe-surface-verification.json` | 251 focused/three cases and14,667 native assertions; linked actual rail/lorry collection, native refinery input and settled payment. |
| Views and real sockets | `logs/probe-replay-verification.json` | 1,178 focused/three cases; four paid local/queued view twins, two partial Recall twins and two real joined TCP peers;55 owner families. |
| Exact work bounds | `logs/asteroid-work-verification.json` | 7,068 measured/14 cases plus38,589 focused/13 cases;15 processes/build; all14 semantic cases agree.39 sources/150 direct identities. |

The actual joined socket client issues generation-checked assignment and original shell release, and both peers retain six matching frame/tick/elapsed checkpoints through recovery. A cloned loopback host first enables native company access; this grants no financial or physical resource. Separate sessions can admit commands on different frames, so socket byte equality is required within each session. Deterministic local/view/queued results also match between builds. This closes Plan10's mining replay task; it does not claim every Plan11 module/manufacturing command has passed that plan's remaining multiplayer acceptance.

All supplied originals and derived accepted paid saves remain at their original paths. The original deployed small miner is `flight-complete-414.sav` by historical filename but actually schema424; its identity is `b7b3568a9f464bed6863f36dfad1781dd05d93d2719c18686b166f4171431b5d`. Paid histories retain their inherited starting-stock procurement limits. Declared isolated final-unit, edge geometry and performance fixtures do not replace those originals or prove new paid procurement.

`logs/probe-runtime-handoff-verification.json` retains the direct accepted-index identities, both unchanged original paid deployments, current Group21 source/evidence identity checks and authored documentation links. This documentation audit does not rerun historical executable graphs or claim another runtime regression.

## Focused commands and work bounds

Run commands from the repository root. Each invocation owns logs/runtime/reports under the established harness. New selector filenames must be unused. No full regression is required for this documentation handoff.

```powershell
python src/tests/multilayer/acceptance/probe_replay_checks-1.py --configuration RelWithDebInfo --mode focused --output logs/probe-replay-handoff-focused.json
python src/tests/multilayer/acceptance/probe_replay_checks-1.py --configuration Debug --mode queue --view hidden --output logs/probe-replay-handoff-hidden.json
python src/tests/multilayer/acceptance/probe_replay_checks-2.py --configuration Debug --output logs/probe-replay-handoff-sockets.json
python src/tests/multilayer/acceptance/asteroid_work_checks-1.py --configuration RelWithDebInfo --domain field --case largest-dense --output logs/asteroid-work-release-field-largest-dense-2.json
python src/tests/multilayer/acceptance/asteroid_work_checks-1.py --configuration Debug --domain probes --case clustered --output logs/asteroid-work-debug-probes-clustered-2.json
python src/tests/multilayer/acceptance/asteroid_work_checks-1.py --configuration Debug --domain focused --output logs/asteroid-work-debug-focused-2.json
```

Accepted selectors are suffix1 for all seven field cases, seven probe cases and one focused burst per configuration. `asteroid_work_checks-2.py` requires those fifteen explicit selectors for each build plus their build logs; its complete accepted index is SHA256 `bc223f9c6a7251cf2981275cc5722bdc64406e1483d25a800b1aee39dd1944c0`. The process records retain the exact executable, argv and original native log/result identities. Final game/test builds are Release2/Debug1; Release1's test-only money comparison diagnostic is excluded.

Measured populations are10/160/10,240/14,336 plus Dense clustered and64x1024/1024x64 Normal160. Pure generation costs O(G+N): exactly G grid insertions,3G-3 leading visits,2(N-3) swaps,N constructions and24N velocity checks. The largest G is522,753. Each motion step performs N body updates and N native index updates. Cluster swap/pop unlink/link costs one operation per departing body. Direct target lookup is O(1); a local query visits K buckets and copies/sorts C candidates, O(K+C log C), including all14,336 in the cluster. Existing shared arbitration is bounded by the64-hull cap and probe/slot pairs, O(P squared + P log P); it has no unconditional N-by-P body-target pass. Cold staging may encounter a whole cluster and is separately recorded.

Actual approaching fleets retain32 native opportunities and charge43 per hull. Separate fresh worlds earn attachment through real commands/ticks, then74 mining opportunities emit two units and charge500 per hull. Isolated legacy hulls, matching actual Map geometry and one declared clustered attachment claim no paid commissioning. Opt-in `space_work_cost.h` counters and instrumented nanoseconds have no saved/network/admission authority. CPU time is diagnostic; no whole-game FPS or maximum-map Surface generation timing is accepted.

## Remaining queue and reading

Plan10's independent runtime, save, view/network and performance groups are complete. **14B.1/14B.2 remain unchecked:** final original asteroid, mining/survey/construction craft and return-pod sources, complete manifest directions/frames/anchors, packed loading fixtures and actual ordinary-game review remain required. Planned/null manifest rows and code-authored preview sprites cannot satisfy them. Plan10 and its parent remain open until those tasks and all source Completion Conditions pass.

Continue sequentially with14B's closest Local Mandatory Reading and UI/art/acceptance Area, then matching Plan13 Sprite Contract and Plan14 Space family producer groups only. Do not reopen unrelated Underground queues. The space master subsequently owns Plan11 Groups13-16 and Plan9's retained16.2 art gate. Their broader presentation/save/network/end-to-end acceptance is not automatically closed by Plan10 evidence. Global reading is non-transitive; use live exported symbols instead of stale line hints. Next auxiliary prefix is `r2mlsp-z-z-q-`. No subagents or Git writes. Original supplied saves stay immutable.
