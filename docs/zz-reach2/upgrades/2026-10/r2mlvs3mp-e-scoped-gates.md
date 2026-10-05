# R2MLVS3MP-E - Scoped gate and task-split reconciliation

Owner: [R2MLVS3MP](r2mlvs3mp-master-plan.md) task 1.4. Children: [Plan 4](r2mlvs4-tunnel-boring-machines.md) Groups 2-5, [Plan 14](r2mlvs14-savegame-determinism-and-lifecycle.md) Groups 7-15 and [Plan 15](r2mlvs15-integrated-acceptance-and-balance.md) Groups 1-5. Architecture: [map layers](../../architecture/map-layers/archi-map-layers.md). Snapshot: 2026-10-05, source HEAD `5f40e4d755` plus the `excavation_guard.cpp` test correction recorded in [r2mlvs3mp-d](r2mlvs3mp-d-execution-notes.md).

This record names which portion of each child task the master queue executes, at which checkpoint, and which public exports gate it. It is not a waiver. A child task whose scope spans families stays unchecked until every portion passes. A portion label (`-G` ground, `-T` tube/riser/first generator, `-O` orbital or later direct-heat) is a named split; executors record evidence against that label in the child plan.

## Producer exports at this snapshot

Verified with `grep` in `src/` and focused executable bursts ([b results](r2mlvs3mp-b-foundation-plan1-14-results.txt), [Plan 2](r2mlvs3mp-b-foundation-plan2-results.txt), [Plans 3/4](r2mlvs3mp-b-foundation-plan3-4-results.txt)).

| Owner header | Present exports | Absent exports (producer obligation) |
| --- | --- | --- |
| `src/multilayer_content.h` | `MultiLayerGameplayProfile`, `LayerVehicleCapability`, `GetMultiLayerContentProfile`, `CheckDeepLayerAvailability`, `UtilityPowerAccount` | none |
| `src/excavation.h` | `ExcavationState`, `GeologyDeposit`, `ExcavationJob`, `CheckExcavatedFootprint`, `CommitExcavationCut`, `GeothermalVentField`, `PlanExcavationRoute`, `PrepareExcavationPass`, `ReserveExcavationJob`, `PrepareExcavationCut`, `ValidateExcavationPassTerms`, `GetExcavationRetreatPath`, `CheckExcavationSupportReplacement`, `ExcavationCommissioningPocket` | solid-starter contract (Plan 2 Group 14B) |
| `src/multilayer_state.h` | `MultiLayerStateSchema`, `ValidateMultiLayerStateReferences` | feature descriptors for Plans 4/5/16/17 |
| `src/layer_portal_cmd.h` | `LayerPortalWorkState`, `LayerPortalWorkRecord`, `GetLayerPortalWorkState`, `CheckLayerPortalWorkAccess`, `CommissionLayerPortalTBM`, `ValidateLayerPortalCommissioning`, `GetLayerPortalWorkPocket` | portal-issued bootstrap proof (Plan 3 Group 8B) |
| `src/tbm.h`, `src/train_cmd.h` | `TBMProject`, `TBMWorkState`, `InspectTBMConsist`, `BuildRailVehicleForTBM` (unpublished part primitive) | `ValidateTBMAreaRoute`, `AdvanceTBMWork`, project commands/serialization |
| `src/tbm_exchange.h` | file absent | `TBMExchangeAssignment`, `ValidateTBMWagonExchange`, `ExecuteTBMWagonExchange` |
| `src/utility_power.h`, `src/transport_tube.h`, `src/geothermal.h`, `src/multilayer_industry.h`, `src/underground_gui.h` | files absent | every listed handoff symbol; master Groups 9-10 produce them |
| `src/space.h`, `src/launch_center.h`, `src/asteroid.h`, `src/orbital_station.h` | files absent | orbital; outside this queue |
| `src/solar_power.h` | M0 ETI/solar contract only | runtime mirror behavior; outside this queue |
| `assets/multilayer_sprite_manifest.json` | `asset_id`, `consumer`, `directions`, `frames`, `anchor`, `source` (141 families, [c result](r2mlvs3mp-c-sprite-contract-results.txt)) | authored/packed runtime art (master Groups 8/10) |

## Plan 4 Groups 2-5

| Task | Scope | Master checkpoint | Gate (must exist and pass its focused test first) |
| --- | --- | --- | --- |
| 2.1 | Ground, whole | 6.6 | Plan 3 8B proof and Plan 2 14B starter APIs (2.1/2.2); Plan 4 2B acquisition (2.3); Plan 4 Groups 3-15 and 17; Plan 5 `TBMExchangeAssignment`/`ExecuteTBMWagonExchange` plus the Group 8/9 Surface receivers |
| 2.3 | Ground, whole | 6.6 | `CheckDeepLayerAvailability` (present); actual accessible Underground rail portal; Plan 4 project travel orders (Group 3) |
| 2.4 | Ground, whole | 6.6 | 2.1 and 2.3 behavior |
| 2B.1-2B.4 | Ground, whole | 2.3 | Plan 3 8B issuance/revalidation and Plan 2 14B route/pass/reservation/cut consumers; existing engine/train owners |
| 3.1-3.3 | Ground, whole | 3.1 | `OrderType`/`Order` storage (unchanged meanings); `MultiLayerStateSchema` command/schema rules |
| 4.1, 4.3, 4.4 | Ground, whole | 3.2 | `PlanExcavationRoute`, `PrepareExcavationPass`, `ValidateExcavationPassTerms` (present) |
| 4.2 | Mixed producer | 4.2A at 3.2; 4.2B at 5.5 | 4.2A: saved project/phase timer only. 4.2B: Plan 5 `ValidateTBMWagonExchange`/`ExecuteTBMWagonExchange` for ReturnForExchange/WaitForEmpties/Resume; never a header-only claim |
| 5.1-5.3 | Ground, whole | 3.3 | `AdvanceTBMWork` work behavior is finished by Plan 4 Groups 6-8 (master 3.5-3.7) before live exchange/runtime claims |

Plan 4's Area gate for Groups 1-5 is satisfied for its named exports. The sealed-start refusal recorded under Plan 4 Group 2 still applies until master Group 2 delivers the starter contract.

## Plan 14 Groups 7-15

The family-neutral loader tasks (9.1, 9.3) complete once for the live loader; the rest are per family. Orbital `-O` portions stay pending in Plan 14 and are not executed by this queue.

| Task | `-G` ground portion (master 6.5, closed again at 11.2) | `-T` tube/riser portion (master 9.5/11.2) | `-O` remainder |
| --- | --- | --- | --- |
| 7.1 | Plans 2/3/4/5 descriptors, after-load fixups and mid-action fixtures | Plan 12 ledger, Plan 16 tubes, Plan 17 first generator/riser | Plans 8-11, mirrors, later direct heat |
| 8.1 | Matrix rows for geology/jobs/portal work/TBM project/exchange/queues | Tube network, utility grants, generator/riser | Orbital rows |
| 9.1, 9.3 | Neutral: complete once for the live loader | Neutral | Neutral |
| 9.2 | Bounds for job/project/wagon-set/queue/receiver lengths and enums | Tube/utility/generator lengths | Orbital pools |
| 10.1 | Deposit/job, `TBMProject`, wagon set/head, portal work, exchange assignment | Tube terminal, utility link, vent claim | Center/mission/module |
| 10.2 | Null-action handlers for the ground families | Tube/utility handlers | Orbital handlers |
| 10.3 | Wagon packets, spoil receiver stock, geological material ledger | Interchange cargo, utility grants | Orbital inventories |
| 10.4 | Consist links, job references, exchange set membership | Tube consist/terminal links | Attachment leases, module dependencies |
| 10.5 | Two heads claiming one wagon; two jobs claiming one face | Duplicate utility grant | Two jobs claiming one orbital input packet |
| 11.1 | Geology/spatial buckets, path/approach reservations | Utility connectivity | Orbital caches |
| 11.2, 11.3 | Ground cache rebuild and next-tick comparisons | Tube/utility next-tick | Orbital |
| 12.1 | Portal construction, excavation, wagon swaps | Tube construction, utility mutation | Launch/deployment, construction consumption, mirror assignment |
| 12.2 | TBM cuts, straddling consists, queue waits | Tube power waits | Probe attachment, ascent/descent, partial modules, partial solar periods |
| 13.1, 13.2 | Ground, whole | - | - |
| 13.3 | - | Tube, whole (after Plan 16 delivery) | - |
| 14.1 | - | - | Orbital, whole |
| 14.2 | - | First generator heat allocation and single-ledger export | Direct-processing inputs/remainders (later Plan 17 plants) |
| 14.3 | Ground families | Tube/first-generator families | Combined closure |
| 15.1 | Ground, whole (master 6.5) | - | - |
| 15.2 | - | - | Orbital, whole |
| 15.3 | - | Tube/utility, whole (master 9.5) | - |
| 15.4 | Ground owner hooks | Tube/utility owner hooks | Orbital owner hooks |

Gate per portion: `-G` needs `src/excavation.h`, `src/layer_portal_cmd.h`, `src/tbm.h` (with project serialization from Plan 4 Group 3.3) and `src/tbm_exchange.h`; a missing Plan 5 header blocks only its exchange rows. `-T` needs `src/utility_power.h`, `src/transport_tube.h` and `src/geothermal.h`. Combined closure waits for every family.

## Plan 15 Groups 1-5

Ground selection for the runner (master 11.1): S1, S2, ground S8, ground S9, the S4 date checks the portal path uses, the selected S11/S12 tube and first-generator/riser rows, and ground art/economics/performance rows. The runner must record this selection and reject a missing required producer or artifact.

| Task | Classification | Ground portion |
| --- | --- | --- |
| 1.1 | Mixed | 1.1-G: revision, profile, config, seed, toolchain and fixture hashes for the selected ground fixtures |
| 2.1 | Neutral | Whole task |
| 2.2 | Mixed | 2.2-G: feature-disabled regression fixtures plus a compact Surface/Underground/Deep connected fixture |
| 3.1, 3.2 | Neutral | Whole task, applied to every fixture the runner sees |
| 3.3 | Mixed (S1..S7) | 3.3-G: starting cash/date/resources and first-predecessor commands for S1/S2 and the S4 date path; S3/S5-S7 stay pending |
| 4.1 | Mixed | 4.1-G: both build configurations plus the ground asset packages; orbital packages pending |
| 4.2, 4.3 | Neutral | Whole task |
| 5.1-5.4 | Neutral runner, mixed scenarios | Runner code and schema are family-neutral; a scenario without its producers reports blocked. Ground runs prove only the selected IDs |

Required ground producers for 5.x passing scenarios: Plan 1/2/3/4/5/14 exports above, matching loaded ground asset families from Plan 13 Groups 79-81, and, for S11/S12 rows, Plans 12/16/17. Orbital exports are needed only if an orbital scenario is selected.
