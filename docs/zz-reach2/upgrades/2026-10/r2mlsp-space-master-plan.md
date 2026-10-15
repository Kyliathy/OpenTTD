# R2MLSP - Space Master Plan

Created: 2026-10-13. Baseline: `5d2d50eed9` (delivered orbital construction and numbered demo tours). Parent: [R2MLVS master](r2mlvs-multilayer-vehicles-and-structures.md).

This checklist coordinates every group in Plans 9, 10 and 11. The main queue is **Plan 9, then Plan 10**, with the specific Plan 11 prerequisites completed when needed. Each source group appears once, including completed foundations and lettered continuations.

Resumed at the user's request,2026-10-14. **Plan9 Groups11/11C/12/13/14/15/17/18/19/20/21 and tasks16.1/16.3 are complete in both configurations; all independent Plan9 runtime/handoff groups are complete; Plan10 Groups1-7 are complete (`logs/asteroid-handoff-verification.json`:37,122 focused assertions/12 cases each and30 current native processes; prior band, depletion, motion and input proofs remain accepted); next is Plan10 Group8 task8.1 (`logs/surface-collection-verification.json`:612 focused assertions/6 cases and5,962 native assertions/12 cases each,26 exact owner families). Group18 proofs are `logs/commodity-loop-verification.json` (11,152 native assertions/8 cases each) and `logs/release-geometry-verification.json` (201 focused assertions/3 cases plus4,716 native assertions/4 cases each);26 owners match ordinary/replayed saves, configurations and command orderings.** Task16.2 remains open under the source's scoped graphics gate: delivered launch-center/fleet artwork and packed loading proof are absent. Schema423 phase/ledger acceptance passes567 focused assertions/13 cases and10,913 new native assertions/15 cases each, with23 phase pairs per configuration compared across26 owners. `logs/phase-ledger-verification.json` freezes77 reports/136 artifacts/60 accepted processes; [phase/ledger notes](r2mlsp-l-phase-ledger-notes.md) preserve the handoff. Retirement/placement evidence remains `logs/launch-retirement-verification.json`. Earlier phase/cargo-return, release/descent and direct module/failure proofs remain immutable. Plan11 prerequisite Groups1-6 remain complete; `logs/orbital-handoff-debug-verification.json` binds their422 producer handoff. Remaining presentation/art/network, Assembly/manufacturing and source-plan completion conditions remain open.

## Execution order and status rules

1. Complete **Plan 11 Groups 4-6** to supply operational modules, safe removal and the public construction contract. Groups 1-3 and 1B are already checked below.
2. **Plan9 independent runtime/handoff groups are complete.** The current delivered423 public exports, architecture and retained proof audit are in [runtime handoff](r2mlsp-q-runtime-handoff.md). Task16.2 alone remains open under the source's scoped graphics gate. Retain the completed retirement, phase/custody, direct module/failure and later Group11B probe-resupply handoffs.
3. Continue **Plan 10 from Group 8 task 8.1**, preserving completed P1/P2 and the later delivered batches. Verify existing behavior against each group's full requirements before implementing missing work.
4. When Plan 10 reaches orbital manufacture or processor freight in Groups 8B/8C, use the **Plan 11 continuation** below to supply the required producers, then return to those consumers. Groups 4-6 alone do not supply manufacturing or processing. Independent mining/recovery work can continue while those consumer tasks remain open.
5. Complete the remaining Plan 11 presentation, persistence and end-to-end acceptance groups after their runtime producers. Close each source plan only when its own Completion Conditions are satisfied.

A checked master item means every task under that source group is checked. Partially completed groups remain unchecked; the notes below identify their current pending tasks. Checkboxes reflect the source plans at the baseline above, rather than a new implementation audit. Update the source plan and its evidence first, then reconcile this checklist.

Before executing a group, read its source plan's Global Mandatory Reading and applicable Area/Local Mandatory Reading. Source tasks, public contracts, producer gates and Completion Conditions remain authoritative. Verify and reuse delivered behavior; an unchecked group may include substantial existing implementation. Keep required art, save/network and performance gates open until their evidence passes. Dependencies on Plans 1, 6, 8, 12, 13-16 remain owned by those plans and must be resolved before dependent completion claims.

## First prerequisite - Plan 11 Groups 1-6

Source: [Orbital stations and manufacturing](r2mlvs11-orbital-stations-and-manufacturing.md). Groups1-6 are complete. Continue at Plan9 Group11; the rest of Plan 11 is listed separately below.

- [X] [Plan 11 Group 1 - Build the first Core from construction-probe deliveries](r2mlvs11-orbital-stations-and-manufacturing.md#group-1---build-the-first-core-from-construction-probe-deliveries)
- [X] [Plan 11 Group 1B - Complete the first Core's paid service functions](r2mlvs11-orbital-stations-and-manufacturing.md#group-1b---complete-the-first-cores-paid-service-functions)
- [X] [Plan 11 Group 2 - Reserve module footprint and delivered inputs](r2mlvs11-orbital-stations-and-manufacturing.md#group-2---reserve-module-footprint-and-delivered-inputs)
- [X] [Plan 11 Group 3 - Advance saved assembly milestones and cancellation](r2mlvs11-orbital-stations-and-manufacturing.md#group-3---advance-saved-assembly-milestones-and-cancellation)
- [X] [Plan 11 Group 4 - Activate completed module functions](r2mlvs11-orbital-stations-and-manufacturing.md#group-4---activate-completed-module-functions)
- [X] [Plan 11 Group 4B - Deliver shared approach reservations](r2mlvs11-orbital-stations-and-manufacturing.md#group-4b---deliver-shared-approach-reservations). Plan8 Group11 producer verified in RelWithDebInfo/Debug.
- [X] [Plan 11 Group 4C - Activate Dock services and verify module dependencies](r2mlvs11-orbital-stations-and-manufacturing.md#group-4c---activate-dock-services-and-verify-module-dependencies). RelWithDebInfo/Debug native gates verified.
- [X] [Plan 11 Group 5 - Classify and safely remove empty modules](r2mlvs11-orbital-stations-and-manufacturing.md#group-5---classify-and-safely-remove-empty-modules)
- [X] [Plan 11 Group 5B - Evacuate unconsumed module inventory explicitly](r2mlvs11-orbital-stations-and-manufacturing.md#group-5b---evacuate-unconsumed-module-inventory-explicitly)
- [X] [Plan 11 Group 5C - Verify removal dependencies and company lifecycle](r2mlvs11-orbital-stations-and-manufacturing.md#group-5c---verify-removal-dependencies-and-company-lifecycle)
- [X] [Plan 11 Group 6 - Delivered construction](r2mlvs11-orbital-stations-and-manufacturing.md#group-6---delivered-construction)

## Main queue first - Plan 9

Source: [Launch centers and rockets](r2mlvs9-launch-centers-and-rockets.md). Preserve the source's execution order, including inserted lettered groups. Resume the first unchecked item after the prerequisite above; the checked later Group 11B retains its delivered resupply evidence.

- [X] [Plan 9 Group 1 - Place a waterside launch station through airport controls](r2mlvs9-launch-centers-and-rockets.md#group-1---place-a-waterside-launch-station-through-airport-controls)
- [X] [Plan 9 Group 2 - Preserve water access and independent service geometry](r2mlvs9-launch-centers-and-rockets.md#group-2---preserve-water-access-and-independent-service-geometry)
- [X] [Plan 9 Group 2B - Physical facility zones and native slot authority](r2mlvs9-launch-centers-and-rockets.md#group-2b---physical-facility-zones-and-native-slot-authority)
- [X] [Plan 9 Group 3 - Surface facility and placement](r2mlvs9-launch-centers-and-rockets.md#group-3---surface-facility-and-placement)
- [X] [Plan 9 Group 4 - Validate hub membership independently of catchment](r2mlvs9-launch-centers-and-rockets.md#group-4---validate-hub-membership-independently-of-catchment)
- [X] [Plan 9 Group 5 - Reserve and move bounded Surface handling cargo](r2mlvs9-launch-centers-and-rockets.md#group-5---reserve-and-move-bounded-surface-handling-cargo)
- [X] [Plan 9 Group 6 - Settle handling when station membership changes](r2mlvs9-launch-centers-and-rockets.md#group-6---settle-handling-when-station-membership-changes)
- [X] [Plan 9 Group 7 - Publish launch, hub and release contracts](r2mlvs9-launch-centers-and-rockets.md#group-7---publish-launch-hub-and-release-contracts)
- [X] [Plan 9 Group 8 - Define rocket model and saved launch phases](r2mlvs9-launch-centers-and-rockets.md#group-8---define-rocket-model-and-saved-launch-phases)
- [X] [Plan 9 Group 8B - Material-paid native hulls before flight](r2mlvs9-launch-centers-and-rockets.md#group-8b---material-paid-native-hulls-before-flight)
- [X] [Plan 9 Group 2C - Carry real fuel and service packets in native compartments](r2mlvs9-launch-centers-and-rockets.md#group-2c---carry-real-fuel-and-service-packets-in-native-compartments)
- [X] [Plan 9 Group 8C - Produce and pack paid passive payloads](r2mlvs9-launch-centers-and-rockets.md#group-8c---produce-and-pack-paid-passive-payloads)
- [X] [Plan 9 Group 10B - Run the paid rocket round trip through SpaceTransition](r2mlvs9-launch-centers-and-rockets.md#group-10b---run-the-paid-rocket-round-trip-through-spacetransition)
- [X] [Plan 9 Group 7B - Deliver the actual pod/descent public owner](r2mlvs9-launch-centers-and-rockets.md#group-7b---deliver-the-actual-poddescent-public-owner)
- [X] [Plan 9 Group 9 - Work the reusable-rocket and persistent-probe ledger](r2mlvs9-launch-centers-and-rockets.md#group-9---work-the-reusable-rocket-and-persistent-probe-ledger)
- [X] [Plan 9 Group 10 - Load and prepare a launch atomically](r2mlvs9-launch-centers-and-rockets.md#group-10---load-and-prepare-a-launch-atomically)
- [X] [Plan 9 Group 11 - Deliver payload and recover the same rocket](r2mlvs9-launch-centers-and-rockets.md#group-11---deliver-payload-and-recover-the-same-rocket). Direct module freight/recovery verified in RelWithDebInfo/Debug.
- [X] [Plan 9 Group 11C - Verify direct module freight contention and target loss](r2mlvs9-launch-centers-and-rockets.md#group-11c---verify-direct-module-freight-contention-and-target-loss)
- [X] [Plan 9 Group 11B - Resupply the existing orbital probe with the same rocket](r2mlvs9-launch-centers-and-rockets.md#group-11b---resupply-the-existing-orbital-probe-with-the-same-rocket)
- [X] [Plan 9 Group 12 - Validate release geometry and reserve recovery](r2mlvs9-launch-centers-and-rockets.md#group-12---validate-release-geometry-and-reserve-recovery)
- [X] [Plan 9 Group 13 - Commit descent with one saved packet owner](r2mlvs9-launch-centers-and-rockets.md#group-13---commit-descent-with-one-saved-packet-owner)
- [X] [Plan 9 Group 14 - Launch and payload lifecycle](r2mlvs9-launch-centers-and-rockets.md#group-14---launch-and-payload-lifecycle)
- [X] [Plan 9 Group 15 - Recover launch and descent failures by phase](r2mlvs9-launch-centers-and-rockets.md#group-15---recover-launch-and-descent-failures-by-phase)
- [ ] [Plan 9 Group 16 - Failures, presentation and acceptance](r2mlvs9-launch-centers-and-rockets.md#group-16---failures-presentation-and-acceptance). Pending:16.2 delivered art/packed loading;16.1/16.3 verified in both423 configurations.
- [X] [Plan 9 Group 17 - Compare phase persistence and payment ledgers](r2mlvs9-launch-centers-and-rockets.md#group-17---compare-phase-persistence-and-payment-ledgers)
- [X] [Plan 9 Group 18 - Failures, presentation and acceptance](r2mlvs9-launch-centers-and-rockets.md#group-18---failures-presentation-and-acceptance)
- [X] [Plan 9 Group 19 - Failures, presentation and acceptance](r2mlvs9-launch-centers-and-rockets.md#group-19---failures-presentation-and-acceptance)
- [X] [Plan 9 Group 20 - Failures, presentation and acceptance](r2mlvs9-launch-centers-and-rockets.md#group-20---failures-presentation-and-acceptance)
- [X] [Plan 9 Group 21 - Failures, presentation and acceptance](r2mlvs9-launch-centers-and-rockets.md#group-21---failures-presentation-and-acceptance)

## Main queue second - Plan 10

Source: [Asteroids and mining probes](r2mlvs10-asteroids-and-mining-probes.md). The first pending task is **8.1**. Groups1-7 are complete; [generation handoff](r2mlsp-x-generation-handoff.md) retains the current public contracts and exact new/legacy continuation evidence. Groups 8B/8C include later station-dependent consumers: invoke the [Plan 11 continuation](#plan-11-continuation---manufacturing-processing-and-acceptance) when those producers are required, then return here. Do not interpret this section's position as requiring full Plan 10 closure before that continuation.

- [X] [Plan 10 Group P1 - Populate and show the first moving Space field](r2mlvs10-asteroids-and-mining-probes.md#group-p1---populate-and-show-the-first-moving-space-field)
- [X] [Plan 10 Group P2 - Native owned probes and finite orbital extraction](r2mlvs10-asteroids-and-mining-probes.md#group-p2---native-owned-probes-and-finite-orbital-extraction)
- [X] [Plan 10 Group 1 - Define resource content and finite asteroid records](r2mlvs10-asteroids-and-mining-probes.md#group-1---define-resource-content-and-finite-asteroid-records). Immutable selected-profile catalogue and inclusive reserve sampling verified in both configurations.
- [X] [Plan 10 Group 2 - Generate the field from the population defaults](r2mlvs10-asteroids-and-mining-probes.md#group-2---generate-the-field-from-the-population-defaults). Exact seeded generation, actual terrain selection and424 input persistence verified in both configurations; [generation input notes](r2mlsp-t-generation-inputs-notes.md) retain the accepted proof and explicit no-site limitation.
- [X] [Plan 10 Group 3 - Implement trajectory and speed on the shared schedule](r2mlvs10-asteroids-and-mining-probes.md#group-3---implement-trajectory-and-speed-on-the-shared-schedule). Shared motion, pre-decision lease translation and exact boundary continuation verified in both configurations; [motion notes](r2mlsp-u-motion-notes.md) retain the26-process proof and geometry-only pursuit scope.
- [X] [Plan 10 Group 4 - Publish the population, resource and motion tuning](r2mlvs10-asteroids-and-mining-probes.md#group-4---publish-the-population-resource-and-motion-tuning). Tables/settings, shared geometry and worked budget records reconciled; defaults and generation version remain unchanged.
- [X] [Plan 10 Group 5 - Keep depleted body identity and attachment safety](r2mlvs10-asteroids-and-mining-probes.md#group-5---keep-depleted-body-identity-and-attachment-safety). Zero-reserve record, actual matching-lease exit and exact persistence verified in both configurations; runtime body removal remains unsupported.
- [X] [Plan 10 Group 6 - Asteroid generation and motion](r2mlvs10-asteroids-and-mining-probes.md#group-6---asteroid-generation-and-motion). Shared strip/envelope and actual static placement refusals verified in both configurations; broader Plan8 obligations remain open.
- [X] [Plan 10 Group 7 - Asteroid generation and motion](r2mlvs10-asteroids-and-mining-probes.md#group-7---asteroid-generation-and-motion). Public generation/body-state handoff verified in both current configurations; [generation handoff](r2mlsp-x-generation-handoff.md) records the consumer gates and version policy.
- [ ] [Plan 10 Group 8 - Configure the delivered small-probe shell](r2mlvs10-asteroids-and-mining-probes.md#group-8---configure-the-delivered-small-probe-shell)
- [ ] [Plan 10 Group 8B - Implement survey probes and typed ordered missions](r2mlvs10-asteroids-and-mining-probes.md#group-8b---implement-survey-probes-and-typed-ordered-missions). Pending: 8B.1, 8B.2, 8B.4; orbital manufacture in 8B.1 requires Plan 11's completed Assembly and manufacturing producer.
- [ ] [Plan 10 Group 8C - Deliver construction materials and reuse orbital products](r2mlvs10-asteroids-and-mining-probes.md#group-8c---deliver-construction-materials-and-reuse-orbital-products). Pending: 8C.3-8C.4; requires Plan 11 manufacturing, processing and freight producers in Groups 7-12.
- [ ] [Plan 10 Group 9 - Deploy and pursue a moving target](r2mlvs10-asteroids-and-mining-probes.md#group-9---deploy-and-pursue-a-moving-target)
- [ ] [Plan 10 Group 10 - Lease an attachment slot and mine finite reserve](r2mlvs10-asteroids-and-mining-probes.md#group-10---lease-an-attachment-slot-and-mine-finite-reserve)
- [ ] [Plan 10 Group 11 - Detach and return to an authorized release region](r2mlvs10-asteroids-and-mining-probes.md#group-11---detach-and-return-to-an-authorized-release-region)
- [ ] [Plan 10 Group 12 - Hand off descent and repeat after paid service](r2mlvs10-asteroids-and-mining-probes.md#group-12---hand-off-descent-and-repeat-after-paid-service)
- [X] [Plan 10 Group 12B - Pause exhausted mining and complete a paid third collection](r2mlvs10-asteroids-and-mining-probes.md#group-12b---pause-exhausted-mining-and-complete-a-paid-third-collection)
- [ ] [Plan 10 Group 13 - Reconcile mission exits and recovery](r2mlvs10-asteroids-and-mining-probes.md#group-13---reconcile-mission-exits-and-recovery)
- [ ] [Plan 10 Group 14 - UI, art and acceptance](r2mlvs10-asteroids-and-mining-probes.md#group-14---ui-art-and-acceptance)
- [ ] [Plan 10 Group 15 - Test interception and exclusive attachment](r2mlvs10-asteroids-and-mining-probes.md#group-15---test-interception-and-exclusive-attachment)
- [ ] [Plan 10 Group 16 - Test mining exits and lost mission references](r2mlvs10-asteroids-and-mining-probes.md#group-16---test-mining-exits-and-lost-mission-references)
- [ ] [Plan 10 Group 17 - Verify mission conservation through saved phases](r2mlvs10-asteroids-and-mining-probes.md#group-17---verify-mission-conservation-through-saved-phases)
- [ ] [Plan 10 Group 18 - UI, art and acceptance](r2mlvs10-asteroids-and-mining-probes.md#group-18---ui-art-and-acceptance)
- [ ] [Plan 10 Group 19 - UI, art and acceptance](r2mlvs10-asteroids-and-mining-probes.md#group-19---ui-art-and-acceptance)
- [ ] [Plan 10 Group 20 - UI, art and acceptance](r2mlvs10-asteroids-and-mining-probes.md#group-20---ui-art-and-acceptance)
- [ ] [Plan 10 Group 21 - UI, art and acceptance](r2mlvs10-asteroids-and-mining-probes.md#group-21---ui-art-and-acceptance)
- [ ] [Plan 10 Group 22 - UI, art and acceptance](r2mlvs10-asteroids-and-mining-probes.md#group-22---ui-art-and-acceptance)

## Plan 11 continuation - Manufacturing, processing and acceptance

Source: [Orbital stations and manufacturing](r2mlvs11-orbital-stations-and-manufacturing.md). Groups 7-12 supply later Plan 10 consumers and use the delivered Plan 9 transport path and Plan 10 mining controller. Verify those producers at the source gates and coordinate unfinished shared scenarios before claiming completion. Groups 13-16 finish Plan 11's own presentation and acceptance requirements.

- [ ] [Plan 11 Group 7 - Prepare heavy-robot and hauler manufacturing orders](r2mlvs11-orbital-stations-and-manufacturing.md#group-7---prepare-heavy-robot-and-hauler-manufacturing-orders)
- [ ] [Plan 11 Group 8 - Materialize one built vehicle without duplicate inputs](r2mlvs11-orbital-stations-and-manufacturing.md#group-8---materialize-one-built-vehicle-without-duplicate-inputs)
- [ ] [Plan 11 Group 9 - Orbital manufacture and processing](r2mlvs11-orbital-stations-and-manufacturing.md#group-9---orbital-manufacture-and-processing)
- [ ] [Plan 11 Group 10 - Bind orbital inventory to the shared recipe executor](r2mlvs11-orbital-stations-and-manufacturing.md#group-10---bind-orbital-inventory-to-the-shared-recipe-executor)
- [ ] [Plan 11 Group 11 - Verify recipe congestion and recovery before scaling](r2mlvs11-orbital-stations-and-manufacturing.md#group-11---verify-recipe-congestion-and-recovery-before-scaling)
- [ ] [Plan 11 Group 12 - Connect processor outputs to explicit orbital freight](r2mlvs11-orbital-stations-and-manufacturing.md#group-12---connect-processor-outputs-to-explicit-orbital-freight)
- [ ] [Plan 11 Group 13 - Presentation and acceptance](r2mlvs11-orbital-stations-and-manufacturing.md#group-13---presentation-and-acceptance)
- [ ] [Plan 11 Group 14 - Verify construction and processing save boundaries](r2mlvs11-orbital-stations-and-manufacturing.md#group-14---verify-construction-and-processing-save-boundaries)
- [ ] [Plan 11 Group 15 - Presentation and acceptance](r2mlvs11-orbital-stations-and-manufacturing.md#group-15---presentation-and-acceptance)
- [ ] [Plan 11 Group 16 - Presentation and acceptance](r2mlvs11-orbital-stations-and-manufacturing.md#group-16---presentation-and-acceptance)
