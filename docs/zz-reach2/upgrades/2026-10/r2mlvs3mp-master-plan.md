# R2MLVS3MP - Portal and TBM execution master plan

Owner: [R2MLVS parent](r2mlvs-multilayer-vehicles-and-structures.md). Targets: complete [r2mlvs3](r2mlvs3-sealed-portals-and-deep-rail.md) and [r2mlvs4](r2mlvs4-tunnel-boring-machines.md), including their late integrations and acceptance. Related architecture: [map layers](../../architecture/map-layers/archi-map-layers.md). Planning snapshot: 2026-10-05; implementation checkpoints below remain unexecuted by this planning change.

Planning continuation note: Context has compacted 1 times while executing this group. This count belongs to creation and validation of the master document, not execution of the implementation groups below.

## What unlocks these plans

There is no single later plan that unlocks both. The parent's [M1 excavation milestone](r2mlvs-multilayer-vehicles-and-structures.md#delivery-milestones) needs Plans 2..5 plus feature persistence and final M1 art. Plan 3 already supplies most portal infrastructure. Plan 4 supplies the controller and actual boring train that finish Plan 3's commissioning/movement cases. Plan 5 supplies the source-side exchange, secondary hauler, minimal Surface receivers and empty return that finish their recurring loop. Plan 13 supplies loaded runtime assets; Plans 14/15 audit save, lifecycle, multiplayer, economics and acceptance.

The immediate wall is a missing **solid-starter behavior contract across Plans 2/3/4**, not an instruction to finish an unrelated feature first. The current geology planner/pass/reservation/cut path expects an open starter footprint, while a native portal preserves solid destination ground. The [focused characterization](r2mlvs4-k-final-results.txt) proves the refusal; it does not prove commissioning. Extend the canonical producer with an explicit portal-issued starter/support contract, then supply actual train/support/wagon participants. A free starter room would violate AD-3/AD-5.

There are also scheduling cycles inside the existing plans: Plan 4 Group 2 needs its own later saved controller, cut participants and recovery; Plan 3's final entry/exchange checks need Plans 4/5; Plan 4's haulage acceptance needs Plan 5; art integration needs live consumers; full Plan 3 Group 12 requires real tube/riser consumers. This master breaks those cycles by completing foundations and individual producer behaviors before activation and returning for integration. It never counts a declaration, placeholder or isolated participant fixture as a live loop.

## Scope and authority

This is an execution queue. Each checkbox links to owning groups; those child checkboxes and public contracts remain the implementation authority. Mark a master checkpoint X only when its exact linked scope and required evidence pass. For a mixed-family child task, split its scope into a named B/C subgroup or record its completed portion before implementation; leave the original whole task/group unchecked until every portion passes.

Follow this queue top to bottom. Within a checkpoint follow the linked groups/tasks in their listed order. The explicitly staged return to Plan 4 Group 2 and Plan 3 Group 8 overrides their former whole-group ordering; it does not override validation or ownership. Already completed child tasks are verified from their retained evidence and relevant focused producer tests, not implemented again. All master boxes begin unchecked because this new scheduling/closure audit has not been executed.

Full completion here includes every outstanding Plan 3/4 task, including Plan 3's tube/riser boundary and Plan 4's real non-rail final-use scenario. Only the supporting portions of other plans are required. This does not require orbital Plans 8..11, mirror deployment, later geothermal smelter/battery/forge chains, or whole-epic closure of Plans 6/7/12..17. Those plans retain their own pending tasks.

## Global Mandatory Reading

Before execution and after every context compaction, read this section, the current queue checkpoint and the nearest applicable Area Mandatory Reading below. Then read the executing child's Global Mandatory Reading and nearest applicable Area block, limited to named symbols/sections relevant to that scope. Reading is non-transitive; do not preload Consult on Demand or Planning Provenance.

| Source | Relevant sections | Purpose |
| --- | --- | --- |
| [R2MLVS parent](r2mlvs-multilayer-vehicles-and-structures.md#proposed-architecture-decisions) | AD-3..AD-6; I-2..I-8 and I-12..I-15 under Cross-plan invariants | Preserve sealed bootstrap, paid cutting, actual wagons, date/capability gates and one owner per quantity. |
| [Plan 3](r2mlvs3-sealed-portals-and-deep-rail.md#public-handoff-contract) and [Plan 4](r2mlvs4-tunnel-boring-machines.md#public-handoff-contract) | Public Handoff Contract; the executing group; Completion Conditions at closure | Distinguish delivered prefixes from complete public runtime behavior. |
| This document | Scope and authority; current Area/checkpoint; completion coverage table | Dispatch work without recreating the circular whole-plan dependency. |

Keep writes inside authorized repositories; no commit/checkout/reset/stash or other Git mutation is authorized. Retain supplied saves and other working changes. New source/test/JSON names do not begin with r2. Assisting Markdown and scratch evidence stays beside this master under `r2mlvs3mp-a-`, `-b-`, etc.; implementation artifacts follow the owning child's established evidence prefix. Do not create assisting scripts in source directories.

Use focused tests for the active owner group. Wait at least 50 seconds when polling a still-running test/build, with waits bounded so progress updates remain possible; the harness must tolerate one minute without agent interaction. Split a test exceeding two minutes into focused files. Avoid full regressions; run them only when justified at ten-group intervals or final closure. After compaction add the required count above the actually executing child group; beyond three compactions split that group into B/C/D groups with scoped reading. Keep continuation notes identifying stale ranges and current producer readiness.

## Current verified baseline

| Owner | Delivered portion | Still needed for this queue |
| --- | --- | --- |
| 1 | Content Freeze, cargo identities, model/date/facility and utility unit contracts | Runtime consumer ownership and measurements remain with their feature owners. |
| 2 | Packed geology, guards, jobs, open-starter cuts, canonical area/pass/retreat planning and saves | New Group 14B solid-starter reconciliation; 16.1 final terrain art. |
| 3 | Most Groups 1..7, ordinary connection/lifecycle prefixes, controls/scripts, Deep gates and genuine legacy portal saves | Actual controller admission/commissioning, occupied runtime/saves, tube/riser integration and final sprites. |
| 4 | Group 1, Group 2 engine/site/guard prefixes; eight focused cases/237 assertions | Complete project commands/saves, work/support/cargo/cost/recovery and commissioning. `src/tbm.h` alone is not the complete handoff. |
| 5 | Logical wagon contract from Plan 1 | Actual exchange, hauler, receivers, queues, persistence and lifecycle; `src/tbm_exchange.h` is absent at this snapshot. |
| 12/13/14 | M0 feasibility/draft, Sprite Contract and Schema Foundation | Runtime power, authored/packed assets and family-specific integration/audits. |
| 6/7/16/17 | Planned public producers | Their required industry/UI/tube/geothermal public implementations are not established by this snapshot. |

### Area Mandatory Reading - Baseline and sealed starter

_Applies to Groups 1-2._

Read the parent's Plan Index handoff-staging paragraph and AD-3/AD-5. Read Plan 4 Group 2's verified refusal and the new Plan 3 Group 8B / Plan 2 Group 14B / Plan 4 Group 2B reading blocks. Inspect only the applicable live owners: `ValidateLayerPortalCommissioning`, `GetLayerPortalWorkPocket`, `PlanExcavationRoute`, `PrepareExcavationPass`, `ReserveExcavationJob`, `PrepareExcavationCut`/`CommitExcavationCut`, and `BuildRailVehicleForTBM`. Quote-only geometry may precede the controller; a paid terrain/cargo mutation must wait for its real participants.

### Group 1 - Verify reusable foundations and reconcile dispatch

{{MEDIUM}}
- [X] 1.1 Verify [Plan 1 Content Freeze](r2mlvs1-gameplay-contracts-and-content.md#public-handoff-contract), [Plan 2 Groups 1-14](r2mlvs2-geology-and-excavation.md#group-1---choose-excavation-storage-with-a-measured-budget), and [Plan 14 Groups 1-6](r2mlvs14-savegame-determinism-and-lifecycle.md#group-1---schema-and-migration-foundation). Reuse delivered storage/profile/schema; verify relevant exports and focused boundary evidence once.
- [X] 1.2 Verify completed scopes of [Plan 3 Groups 1-7](r2mlvs3-sealed-portals-and-deep-rail.md#group-1---portal-state-and-connection-matrix), [Group 10](r2mlvs3-sealed-portals-and-deep-rail.md#group-10---commissioning-and-bootstrap) and [Group 14](r2mlvs3-sealed-portals-and-deep-rail.md#group-14---controls-art-and-acceptance), plus [Plan 4 Group 1](r2mlvs4-tunnel-boring-machines.md#group-1---define-cutter-models-and-immutable-consist-roles) and its delivered [Group 2 prefixes](r2mlvs4-tunnel-boring-machines.md#group-2---commission-only-through-the-validated-rail-facility). Do not reinterpret the remaining role/commissioning refusals as success.
- [X] 1.3 Verify [Plan 13 Groups 1-8 Sprite Contract](r2mlvs13-graphics-and-asset-pipeline.md#group-8---publish-the-sprite-contract-handoff); templates/manifests are inputs to later art, not finished runtime assets.
- [X] 1.4 Reconcile scoped gates and task splits in [Plan 4 Groups 2-5](r2mlvs4-tunnel-boring-machines.md#group-2b---prepare-complete-acquisition-without-premature-activation), [Plan 14 Groups 7-15](r2mlvs14-savegame-determinism-and-lifecycle.md#group-7---feature-persistence-integration) and [Plan 15 Groups 1-5](r2mlvs15-integrated-acceptance-and-balance.md#group-1---fixture-and-build-readiness) before their dependent work. Missing orbital producers must not block the ground runner/audit; missing required ground producers still must. Record exact scoped task IDs and public fields, not a blanket waiver of a child gate.

Group 1 evidence (2026-10-05): focused Debug bursts pass Plan 1/14 (18 cases/715 assertions), Plan 2 (44/5,884) and Plans 3/4 (28/1,274); the sprite manifest validates 141 families. The first Plan 2 burst exposed a stale `excavation_guard.cpp` portal expectation that predates Plan 3's sealed housing; the test now asserts the pure sealed quote. Scoped splits and gates: [r2mlvs3mp-e](r2mlvs3mp-e-scoped-gates.md). Notes: [r2mlvs3mp-d](r2mlvs3mp-d-execution-notes.md).

Context has compacted 1 times while executing this group.

### Group 2 - Reconcile the solid starter before dependent work

{{HARD}}
- [X] 2.1 Execute [Plan 3 Group 8B](r2mlvs3-sealed-portals-and-deep-rail.md#group-8b---publish-the-narrow-solid-starter-geometry-contract): publish/test the narrow portal-issued source-support/destination-pocket proposal. It must be quotable before complete TBM acquisition.
- [X] 2.2 Execute [Plan 2 Group 14B](r2mlvs2-geology-and-excavation.md#group-14b---reconcile-canonical-planning-and-cuts-with-a-solid-commissioning-pocket): extend the one canonical planner/reservation/cut/serialization contract. Charge all newly solid swept starter cells and preserve the housing; never pre-clear a room or duplicate cursor/cargo authority.
- [X] 2.3 Execute [Plan 4 Group 2B](r2mlvs4-tunnel-boring-machines.md#group-2b---prepare-complete-acquisition-without-premature-activation): complete pure whole-consist preparation, bounded unpublished acquisition and rollback. Keep the production commissioning/start control guarded until Group 6 below.

Exit: all three producer/consumer signatures and their focused boundary tests agree. Geological participant fixtures prove the owner's transaction boundary; they do not replace the later actual-wagon proof.

Group 2 status (2026-10-05): all three child groups passed the [milestone burst](r2mlvs3mp-g-burst4-all-results.txt) after master Group 4, so 2.1-2.3 are checked. The burst fixed one code defect (the vehicle pool check in `BuildTBMConsistAtSite`) and three test expectations. Child evidence: Plan 3 Group 8B, Plan 2 Group 14B and Plan 4 Group 2B status paragraphs. Notes: [r2mlvs3mp-d](r2mlvs3mp-d-execution-notes.md#group-2---reconcile-the-solid-starter-verified) and the [milestone notes](r2mlvs3mp-d-execution-notes.md#milestone-build-and-burst-end-of-master-group-4-2026-10-05).

### Area Mandatory Reading - Controller, cutting and haulage

_Applies to Groups 3-5._

Read the executing Plan 4 or Plan 5 area's mandatory sources and Public Handoff Contract. Follow the new starter contract from Group 2; inspect the existing train tick/order/consist/cargo owners only for the active task. Plan 4 owns actual vehicle/material/funds participation, Plan 2 owns route/cursor/partial work/terrain, and Plan 5 owns actual wagon exchange, hauler cycle and bounded receivers. `AdvanceTBMWork` readiness must state which behavior is tested; Plan 5 cannot depend on a header-only claim.

Context has compacted 1 times while executing this group.

### Group 3 - Build the saved controller and actual cut participants

{{HARD}}
- [X] 3.1 Execute [Plan 4 Group 3](r2mlvs4-tunnel-boring-machines.md#group-3---choose-the-project-command-and-order-representation): typed private project assignment, ordinary/shared travel orders and versioned command/save round trips.
- [X] 3.2 Execute [Plan 4 Group 4](r2mlvs4-tunnel-boring-machines.md#group-4---implement-saved-work-orders-and-state-transitions): canonical preview, guarded phases, resumable interruptions and TravelToFace/Bore-entry reload. Real stopped boundary consists may exercise foundations while the production purchase control remains guarded; unavailable exchange behavior stays blocked.
- [X] 3.3 Execute [Plan 4 Group 5](r2mlvs4-tunnel-boring-machines.md#group-5---publish-the-tbm-controller-boundary): publish the real assignment/role/state/route boundary, one tick call site and consumer fixture. Complete the declared work behavior through 3.5-3.7 before using it for live exchange/runtime claims.
- [X] 3.4 Execute [Plan 3 Group 5 task 5.3B](r2mlvs3-sealed-portals-and-deep-rail.md#group-5---expose-sealed-routing-without-losing-physical-pair-geometry): consume actual controller role/part authorization. Test restricted owned-consist entry/retreat and normal/unrelated follower refusal against real boundary consists; final playable entry comes in Group 7.
- [X] 3.5 Execute [Plan 4 Group 6](r2mlvs4-tunnel-boring-machines.md#group-6---advance-bounded-work-against-one-reserved-face): one claimed face, one saved work accumulator/tick and precise capacity/support/funds waits.
- [X] 3.6 Execute [Plan 4 Group 7](r2mlvs4-tunnel-boring-machines.md#group-7---join-geological-cuts-to-real-wagon-inventory-atomically): actual separate Earth/Rock packets, canonical work-site provenance, storage acquisition and one atomic paid cut; test exact fill, one-cargo fullness and pool/participant failure.
- [ ] 3.7 Execute [Plan 4 Group 8](r2mlvs4-tunnel-boring-machines.md#group-8---install-charged-support-and-preserve-retreat): paid temporary support metadata/placement and actual part positions/reservations, including the first solid-pocket segment. Finish the work-behavior producer tests promised by Group 5.

Group 3 status (2026-10-05): all seven child scopes passed the [milestone burst](r2mlvs3mp-g-burst4-all-results.txt), so 3.1-3.6 are checked. 3.7 stays unchecked because one part of it is not delivered: moving the parts onto new support and their reservations. It needs the first pocket entry (7.3) and Plan 4 Group 14 (4.6); until then a working cutter is held still. Child evidence is in the Plan 4 Groups 3-8 status paragraphs and the Plan 3 Group 5 status line. Notes: [r2mlvs3mp-d](r2mlvs3mp-d-execution-notes.md#group-3---saved-controller-and-cut-participants-verified).

### Group 4 - Finish geometry, accounting and protected retreat

{{HARD}}
- [X] 4.1 Execute [Plan 4 Group 9](r2mlvs4-tunnel-boring-machines.md#group-9---boring-and-accounting): declared conversion/removal/traversal rules for actual temporary track.
- [X] 4.2 Execute [Plan 4 Group 10](r2mlvs4-tunnel-boring-machines.md#group-10---bind-canonical-area-passes-to-the-actual-train): bind actual consist dimensions/capacity to accepted canonical passes and charge every authorized extra cut.
- [ ] 4.3 Execute [Plan 4 Group 11](r2mlvs4-tunnel-boring-machines.md#group-11---exercise-area-coverage-and-turning-as-a-separate-behavior): straight/two-bend/irregular coverage, tight-turn/obstruction refusal and saved safe backing.
- [X] 4.4 Execute [Plan 4 Group 12](r2mlvs4-tunnel-boring-machines.md#group-12---accrue-active-and-idle-costs-without-tick-duplication): saved state-duration charging, nonzero idle cost, insufficient funds and bankruptcy protection.
- [X] 4.5 Execute [Plan 4 Group 13](r2mlvs4-tunnel-boring-machines.md#group-13---boring-and-accounting): actual backing preserves cutter-facing authority and cannot bore backwards.
- [ ] 4.6 Execute [Plan 4 Group 14](r2mlvs4-tunnel-boring-machines.md#group-14---separate-work-reservations-from-ordinary-freight): project corridor/retreat claims, movement/force-proceed enforcement and safe release with occupied tails.
- [X] 4.7 Execute the shared removal/replacement and existing-road/rail cases of [Plan 4 Group 15](r2mlvs4-tunnel-boring-machines.md#group-15---boring-and-accounting). Keep real tube/plant construction acceptance pending until Group 10; do not complete the whole task from a hypothetical footprint.

Group 4 status (2026-10-05): all seven child scopes passed the [milestone burst](r2mlvs3mp-g-burst4-all-results.txt), so 4.1, 4.2, 4.4, 4.5 and 4.7 are checked. 4.3 (Plan 4 11.3, reversing along real support) and 4.6 (Plan 4 14.3, release with an occupied tail) stay unchecked. Not delivered:
- real backing in the pocket (Plan 4 Group 13), which needs 7.3;
- the cancellation and portal-tail cases of Plan 4 Group 14, which need 6.3 and 7.3;
- tube and plant reuse (Plan 4 Group 15), which needs Group 10.

The milestone fixed two code defects (validator order in `GetLiveTBMJob`; reversal of a consist on its portal site) and five test setups; see the [milestone notes](r2mlvs3mp-d-execution-notes.md#milestone-build-and-burst-end-of-master-group-4-2026-10-05) and [Group 4 notes](r2mlvs3mp-d-execution-notes.md#group-4---geometry-accounting-and-protected-retreat-verified).

Context has compacted 1 times while executing this group.

### Group 5 - Supply actual exchange and disposal before enabling commissioning

{{HARD}}
- [ ] 5.1 Execute [Plan 5 Group 1](r2mlvs5-spoil-wagons-and-haulage.md#group-1---build-the-controlled-exchange-location): real controlled bay, full stopped-consist admission and restricted rearrangement primitives.
- [ ] 5.2 Execute [Plan 5 Group 2](r2mlvs5-spoil-wagons-and-haulage.md#group-2---describe-both-resulting-consists-before-mutation): complete before/after ID/capacity/order/group proposals with immutable cutter/support prefix.
- [ ] 5.3 Execute [Plan 5 Group 3](r2mlvs5-spoil-wagons-and-haulage.md#group-3---commit-one-wagon-exchange-with-complete-recovery): one two-way transaction preserving real wagon IDs/packets and complete failure recovery.
- [ ] 5.4 Execute [Plan 5 Group 4](r2mlvs5-spoil-wagons-and-haulage.md#group-4---reuse-the-exchange-at-the-commissioning-origin): the same exchange on the source side before any underground bay. Freeze real origin/receiver admission; defer its full loop until receiver delivery below.
- [ ] 5.5 Execute [Plan 5 Group 5](r2mlvs5-spoil-wagons-and-haulage.md#group-5---controlled-exchange-facility): tested `tbm_exchange.h` producer and direct Plan 3/4 consumer fixtures.
- [ ] 5.6 Execute [Plan 5 Group 6](r2mlvs5-spoil-wagons-and-haulage.md#group-6---drive-one-assigned-hauler-through-its-saved-cycle): actual secondary locomotive and saved collect/origin/dispose/empty-return phases; full-cycle test follows receiver implementation.
- [ ] 5.7 Execute [Plan 5 Group 7](r2mlvs5-spoil-wagons-and-haulage.md#group-7---queue-wagon-sets-independently-of-train-heads): stable exclusive sets/queues and multi-hauler refusals.
- [ ] 5.8 Execute [Plan 5 Group 8](r2mlvs5-spoil-wagons-and-haulage.md#group-8---enforce-the-surface-origin-leg-and-real-disposal), then [Group 9](r2mlvs5-spoil-wagons-and-haulage.md#group-9---account-for-spoil-delivery-and-expose-logistics-stalls): the minimal real Earth/Rock Surface sinks, exact intake/fees and controller pause/recovery. These sinks are owned by Plan 5 and must not wait for Plan 6 factories. Close the deferred 4.2/6.3 receiver/cycle cases.

Group 5 status (2026-10-05): every scope of Plan 5 Groups 1-9 is implemented but not yet compiled or run, so 5.1-5.8 stay unchecked until the next milestone burst (end of master Group 7 or 8).
- Designs: [r2mlvs3mp-h](r2mlvs3mp-h-exchange-design.md) (bay, exchange, hauler cycle, wagon sets) and [r2mlvs3mp-i](r2mlvs3mp-i-receiver-design.md) (receivers, origin leg, persistence).
- The deferred cases are closed in code: origin admission is satisfied by real receivers (4.2), and one complete cycle with real disposal is a test (6.3).
- Not delivered here:
  - saving assignments and wagon sets, and queue reload (Plan 5 Group 10, master 6.1);
  - refusing ordinary moves, sales and refits of cutter parts in a depot bay (Plan 5 Group 11, master 6.2);
  - either train driving itself into a bay (master Group 7).
- The burst list and compile/runtime risks are in the [notes](r2mlvs3mp-d-execution-notes.md#group-5---exchange-and-disposal-in-progress-unbuilt).

### Area Mandatory Reading - Activation, occupied runtime and M1 art

_Applies to Groups 6-8._

Read Plan 3's acquisition/cancellation handoff, Plan 4 recovery and Plan 5 lifecycle/persistence scopes. Verify the public owners produced in Groups 2-5 and the relevant Plan 14 lifecycle rows. For art use only the selected Plan 13 authoring/conversion/runtime areas and matching manifest families; read the image-generation skill at actual raster authoring time. Structural/runtime prefixes are not final asset acceptance.

Context has compacted 1 times while executing this group.

### Group 6 - Persist and recover before completing live commissioning

{{HARD}}
- [ ] 6.1 Execute [Plan 5 Group 10](r2mlvs5-spoil-wagons-and-haulage.md#group-10---persist-assignment-and-queue-state): real assignments/queues/packet ownership survive load and every exchange save is before or after the atomic action.
- [ ] 6.2 Execute [Plan 5 Group 11](r2mlvs5-spoil-wagons-and-haulage.md#group-11---handle-fleet-and-company-lifecycle-around-assignments), then [Group 12](r2mlvs5-spoil-wagons-and-haulage.md#group-12---lifecycle-and-acceptance): safe refit/autoreplace/sale, bay loss, takeover and repeated mixed-spoil conservation.
- [ ] 6.3 Execute [Plan 4 Group 17](r2mlvs4-tunnel-boring-machines.md#group-17---implement-recoverable-cancellation-and-reassignment): independent cancellation/service/lost-reference rules and the now-delivered Plan 5 exchange/reassignment integration. Do not strand occupied support or delete spoil to recover.
- [ ] 6.4 Finish [Plan 3 Group 11](r2mlvs3-sealed-portals-and-deep-rail.md#group-11---recover-commissioning-projects-before-lifecycle-mutation), including 11.3/11.4 with actual projects/wagons and two-end failure/refusal.
- [ ] 6.5 Audit ground persistence/lifecycle rows of [Plan 14 Groups 7-15](r2mlvs14-savegame-determinism-and-lifecycle.md#group-7---feature-persistence-integration). Validate real loaded families and missing/reused IDs, packet custody, after-load/Null cleanup, rebuilt caches and next-step state. Orbital rows remain pending in their owner.
- [ ] 6.6 Return to the remaining [Plan 4 Group 2](r2mlvs4-tunnel-boring-machines.md#group-2---commission-only-through-the-validated-rail-facility) and [Plan 3 Group 8](r2mlvs3-sealed-portals-and-deep-rail.md#group-8---build-portal-side-commissioning-validation). Complete legal Surface and accessible Underground-to-Deep commissioning, complete acquisition/payment/job/assignment publication, full rollback, service-only depots and actual purchase-control activation. Missing real route/receiver/recovery admission still refuses before allocation.

Exit: a real funded first cutter can be commissioned and safely stopped/recovered from every implemented phase; its origin/source exchange already has actual empty-wagon and receiver capacity. No underground bay or free chamber is a startup prerequisite.

Group 6 status (2026-10-05): 6.1 is implemented but not yet compiled or run, so it stays unchecked until the next milestone burst.
- Assignments and wagon sets are saved from version 378 and validated after load.
- Details are in the Plan 5 Group 10 status and the [notes](r2mlvs3mp-d-execution-notes.md#group-6---persist-and-recover-in-progress-unbuilt).

### Group 7 - Close the first playable ground loop and occupied saves

{{HARD}}
- [ ] 7.1 Execute [Plan 5 Group 13](r2mlvs5-spoil-wagons-and-haulage.md#group-13---demonstrate-and-reload-the-complete-m1-logistics-loop): commission, paid first cut, fill, source-side exchange, real Surface visit/disposal, empty return and resume; then recurring underground-bay exchange. Retain per-wagon/packet/cost ledgers and wait-state reloads.
- [ ] 7.2 Finish [Plan 3 Group 9](r2mlvs3-sealed-portals-and-deep-rail.md#group-9---connect-only-a-cleared-and-safely-reserved-pair): fully withdrawn TBM then ordinary freight; spanning-consist refusal preserves recovery.
- [ ] 7.3 Finish [Plan 3 Group 15](r2mlvs3-sealed-portals-and-deep-rail.md#group-15---controls-art-and-acceptance): first entry/retreat, sealed normal-train refusal, connected crossing, signals and reversals across both allowed rail pairs.
- [ ] 7.4 Finish [Plan 3 Group 16](r2mlvs3-sealed-portals-and-deep-rail.md#group-16---validate-saved-portal-state-and-occupied-references): actual Sealed/Commissioning/Connected files, work-pocket wagons and spanning consists; retain genuine version-370 inputs/evidence.
- [ ] 7.5 Finish [Plan 3 Group 17](r2mlvs3-sealed-portals-and-deep-rail.md#group-17---controls-art-and-acceptance): complete commissioning and every rail/elevator/script path immediately before/on 1980, including actual fleet paths and explicit legacy/editor/economy-time policies.
- [ ] 7.6 Execute [Plan 4 Group 18](r2mlvs4-tunnel-boring-machines.md#group-18---recovery-presentation-and-acceptance): material conservation across partial/full cuts, cancellation/reversal and saves with actual wagons.
- [ ] 7.7 Execute [Plan 4 Group 19](r2mlvs4-tunnel-boring-machines.md#group-19---recovery-presentation-and-acceptance): visible/hidden equivalence and both first source exchange and recurring underground exchange.
- [ ] 7.8 Execute [Plan 4 Group 20](r2mlvs4-tunnel-boring-machines.md#group-20---recovery-presentation-and-acceptance): measured standard/Deep time and complete cost, including real haulage/disposal; hand actual measurements to Plan 15.

### Group 8 - Deliver and review the complete M1 asset slice

{{HARD}}
- [ ] 8.1 Execute [Plan 13 Group 9](r2mlvs13-graphics-and-asset-pipeline.md#group-9---prepare-controlled-asset-authoring-and-originals): selected family briefs, stable original selection and provenance.
- [ ] 8.2 Execute [Plan 13 Groups 10-13](r2mlvs13-graphics-and-asset-pipeline.md#group-10---author-underground-terrain): Underground/Deep ground, sealed/commissioning overlays and Deep portal.
- [ ] 8.3 Execute [Plan 13 Groups 14-18](r2mlvs13-graphics-and-asset-pipeline.md#group-14---author-standard-tbm): both cutters/support, Earth/Rock load states and the actual spoil hauler.
- [ ] 8.4 Execute [Plan 13 Group 23](r2mlvs13-graphics-and-asset-pipeline.md#group-23---author-tbm-exchange-bay), [Group 26](r2mlvs13-graphics-and-asset-pipeline.md#group-26---author-underground-service-depots), and [Groups 37-38](r2mlvs13-graphics-and-asset-pipeline.md#group-37---author-earth-reclamationdisposal-site): occupied bays/service and both real spoil receiver families.
- [ ] 8.5 Execute the ground commissioning/outline/turn/blocked/date/control subset of [Plan 13 Group 78](r2mlvs13-graphics-and-asset-pipeline.md#group-78---author-construction-and-cargo-controls). Record that subset; the orbital collection/drop controls stay pending.
- [ ] 8.6 Execute [Plan 13 Groups 79-81](r2mlvs13-graphics-and-asset-pipeline.md#group-79---convert-and-pack-from-stable-originals) for this declared family set: deterministic packing, palette/anchor/frame checks, actual packed loading and state-independent animation. Preserve published offsets/content identity rules; unselected later families remain pending.
- [ ] 8.7 Execute [Plan 13 Groups 82-83](r2mlvs13-graphics-and-asset-pipeline.md#group-82---integrate-m1-terrain-and-portal-states), including the new sound task 83.4, then complete [Plan 2 task 16.1](r2mlvs2-geology-and-excavation.md#group-16---presentation-and-validation), [Plan 3 Group 13](r2mlvs3-sealed-portals-and-deep-rail.md#group-13---controls-art-and-acceptance) and [Plan 4 Group 16](r2mlvs4-tunnel-boring-machines.md#group-16---expose-controller-state-and-integrate-matching-assets): real queries, tooltips, work/cost/capacity displays, runtime sprites and verified sound events. Sound delivery and its consumer hook must pass before counting 16.2 complete.
- [ ] 8.8 Execute the M1 rows of [Plan 13 Groups 85-88](r2mlvs13-graphics-and-asset-pipeline.md#group-85---runtime-integration-and-review): actual direction/load/service/turn/portal occlusion frames and reviewed distributed hashes. Record user art feedback separately; remaining launch/module/later-industry rows stay pending.

### Area Mandatory Reading - Real tube and riser consumers

_Applies to Groups 9-10._

Read Plan 3 Group 12 and Plan 4 Groups 15/21, then the current producer areas in Plans 12/6/7/16/17. Read the parent's Useful Power staging paragraph, AD-4/AD-15/AD-16 and I-12/I-14/I-15/I-17 only as relevant. Verify runtime `utility_power.h`, `multilayer_industry.h`, `underground_gui.h`, `transport_tube.h` and `geothermal.h` when their checkpoints produce them; their absence at this snapshot is not permission to copy a ledger or registry. First tube power uses paid external service; first geothermal construction uses actual Surface supplies and conventional delivery.

### Group 9 - Supply the later integrations' shared producers

{{HARD}}
- [ ] 9.1 Execute [Plan 12 Groups 7-9](r2mlvs12-solar-farms-mirrors-and-eti.md#group-7---implement-the-native-useful-power-ledger): tested Useful Power ledger, atomic consume/release/transfer, effects and saved accounting. This handoff does not wait for solar mirrors or rockets.
- [ ] 9.2 Execute [Plan 6 Groups 1-8](r2mlvs6-mines-and-resource-chains.md#group-1---characterize-industry-layer-assumptions-by-lifecycle): layer/capability/funding lifecycle and actually callable shared bounded recipe/extraction/dispatch APIs; Group 6 declarations alone cannot unlock geothermal.
- [ ] 9.3 Execute [Plan 6 Groups 9-13](r2mlvs6-mines-and-resource-chains.md#group-9---instantiate-the-ten-mine-definitions) as needed for the first generator's real Surface construction inputs and published industry/utility participants. Record the selected runtime supply/industry families for art checkpoint 10.4. Preserve Plan 5 ownership of spoil sinks. Later direct-heat plant runtime stays with Plan 17; do not claim full Plan 6 art/acceptance from this producer delivery.
- [ ] 9.4 Execute [Plan 7 Groups 1-3](r2mlvs7-underground-construction-and-fleet.md#group-1---underground-construction-interface), [Group 5](r2mlvs7-underground-construction-and-fleet.md#group-5---publish-underground-ui-entry-points), and the required service/ordinary-fleet rows of [Groups 6-7](r2mlvs7-underground-construction-and-fleet.md#group-6---add-excavation-aware-freight-and-service-facilities). Record the selected fleet/station/terminal families for 10.4. Keep Group 4's absent tube/geothermal actions pending until Group 10, rather than blocking the base toolbar.
- [ ] 9.5 Define/test the relevant tube/power/riser lifecycle rows in [Plan 14 Group 15](r2mlvs14-savegame-determinism-and-lifecycle.md#group-15---define-lifecycle-policy-by-reference-family) alongside their upcoming owners. Preserve consumed energy/cargo and explicit ID-reuse policy.

### Group 10 - Complete tube/riser boundaries and actual non-rail conversion

{{HARD}}
- [ ] 10.1 Execute [Plan 16 Groups 1-4](r2mlvs16-underground-transport-tubes.md#group-1---prove-a-separate-tube-capability-over-rail-foundations): real tube capability, construction/conversion, reservations and rail/portal/elevator separation. Tubes cannot commission cutters or remove occupied retreat support.
- [ ] 10.2 Execute [Plan 16 Groups 5-8](r2mlvs16-underground-transport-tubes.md#group-5---connect-paid-external-power-without-a-geothermal-prerequisite): actual paid-grid movement/service, power waits and public construction/fleet queries. Keep only geothermal source-switch task 6.4 pending until 10.6; do not make it a prerequisite for paid-grid delivery.
- [ ] 10.3 Execute [Plan 17 Groups 1-7](r2mlvs17-geothermal-energy-and-industry.md#group-1---consume-vent-geology-and-bind-excavated-collectors): bounded vent claims/heat, Deep links, commissioned/date-valid riser/export and bootstrap/public APIs. Interface/export tests do not yet claim an operating generator; that proof follows in 10.6.
- [ ] 10.4 Execute the required [Plan 13 Groups 47-53](r2mlvs13-graphics-and-asset-pipeline.md#group-47---author-tube-guideways-and-junctions), [54-56](r2mlvs13-graphics-and-asset-pipeline.md#group-54---author-vent-field-and-collector), [60](r2mlvs13-graphics-and-asset-pipeline.md#group-60---author-heatelectric-utilities-and-risers) and related Group 78 controls. Also deliver the supporting fleet/facility families selected in 9.4 from [Groups 19-22](r2mlvs13-graphics-and-asset-pipeline.md#group-19---author-underground-freight-locomotive) / [24-25](r2mlvs13-graphics-and-asset-pipeline.md#group-24---author-underground-rail-freight-station), and only the actual supply-chain families selected in 9.3 from [Groups 27-36](r2mlvs13-graphics-and-asset-pipeline.md#group-27---author-coal-mine) / [39-46](r2mlvs13-graphics-and-asset-pipeline.md#group-39---author-metal-refinery). Pack/validate the declared set through [Groups 79-81](r2mlvs13-graphics-and-asset-pipeline.md#group-79---convert-and-pack-from-stable-originals). Freeze consumer geometry before authoring; include the 1980/2000 generator/riser families. Unused industries/fleets and unimplemented 2010..2045 plants remain pending; a selected runtime supply producer must not depend on a missing packed asset.
- [ ] 10.5 Execute [Plan 16 Groups 9-12](r2mlvs16-underground-transport-tubes.md#group-9---add-terminals-and-accountable-freight-interchange): real terminals/interchange, five original dated definitions, purchase/refit/clone/replacement guards and controls. The already delivered packed families must back task 10.6's model-art validation.
- [ ] 10.6 Execute [Plan 17 Group 8](r2mlvs17-geothermal-energy-and-industry.md#group-8---implement-the-two-dated-geothermal-generators), generator/riser availability from [Group 13](r2mlvs17-geothermal-energy-and-industry.md#group-13---plants-and-future-production), applicable [Group 14](r2mlvs17-geothermal-energy-and-industry.md#group-14---recover-plants-and-grants-through-lifecycle-changes) recovery, [Group 15](r2mlvs17-geothermal-energy-and-industry.md#group-15---controls-art-and-acceptance) generator/utility controls and 1980/2000 boundaries, and generator/export saves from [Group 17](r2mlvs17-geothermal-energy-and-industry.md#group-17---verify-saved-geothermal-work-and-hidden-layer-replay). Demonstrate actual first Surface-supplied plant/riser and single-ledger electricity export; smelter/battery/forge/orbital supply tasks remain pending.
- [ ] 10.7 Close [Plan 16 task 6.4](r2mlvs16-underground-transport-tubes.md#group-6---meter-movement-and-service-demand-through-shared-grants), then its [Groups 13-15](r2mlvs16-underground-transport-tubes.md#group-13---art-persistence-and-acceptance) relevant art/save/lifecycle/separation/useful-route cases. Integrate these tube/first-generator/riser families through scoped [Plan 13 Group 84.3](r2mlvs13-graphics-and-asset-pipeline.md#group-84---runtime-integration-and-review) and runtime review. Unimplemented later geothermal art remains pending in Plan 13.
- [ ] 10.8 Finish the delivered-provider rows of [Plan 7 Group 4](r2mlvs7-underground-construction-and-fleet.md#group-4---connect-later-tube-and-geothermal-tool-providers) / [Group 8](r2mlvs7-underground-construction-and-fleet.md#group-8---integrate-the-five-delivered-tube-models-into-fleet-controls), fully complete [Plan 3 Group 12](r2mlvs3-sealed-portals-and-deep-rail.md#group-12---commissioning-and-bootstrap) and remaining [Plan 4 Group 15](r2mlvs4-tunnel-boring-machines.md#group-15---boring-and-accounting), then execute [Plan 4 Group 21](r2mlvs4-tunnel-boring-machines.md#group-21---recovery-presentation-and-acceptance). Use actual tube-terminal/geothermal construction after charged irregular clearing and safe withdrawal; retain bend/U-turn/backwards refusal and turn reload proof. No painted/model-only final-use substitute completes 21.4.

### Area Mandatory Reading - Ground acceptance and closure

_Applies to Groups 11-12._

Read the selected Plan 14/15 areas, Plan 13 matching runtime families, and both target Completion Conditions. The runner's ground selection is explicit; each selected scenario must have all its real producers and assets. Separate runtime bootstrap, isolated boundary, genuine legacy, replay and actual host/client join evidence. Scope unrelated orbital rows out before execution; do not silently skip them and complete their whole tasks.

### Group 11 - Audit the complete target behavior

{{HARD}}
- [ ] 11.1 Execute the explicitly ground-scoped [Plan 15 Groups 1-5](r2mlvs15-integrated-acceptance-and-balance.md#group-1---fixture-and-build-readiness): version/config/seed/input hashes, genuine versus injected fixture classification, both build configurations and real acceptance runner/evidence/refusal schema.
- [ ] 11.2 Finish target and delivered tube/riser rows of [Plan 14 Groups 7-15](r2mlvs14-savegame-determinism-and-lifecycle.md#group-7---feature-persistence-integration): save/preview/Null/refusal/next-tick comparison and unique quantity/reference ownership. Keep orbital/descent/direct-heat rows pending.
- [ ] 11.3 Execute ground and required tube/riser scopes of [Plan 14 Groups 16-19](r2mlvs14-savegame-determinism-and-lifecycle.md#group-16---entity-lifecycle-and-multiplayer) and [Group 21](r2mlvs14-savegame-determinism-and-lifecycle.md#group-21---stress-tube-and-geothermal-lifecycle-across-technology-dates). Require real host/client command traces and join with active excavation/wagon waits, differing cameras/rendering, supported pause/time modes and relevant power-link failure. Single-player replay is not join evidence; final combined orbital join stays pending.
- [ ] 11.4 Execute [Plan 15 Groups 6-7](r2mlvs15-integrated-acceptance-and-balance.md#group-6---playable-loops-and-failure-recovery), ground rows of [Groups 9-10](r2mlvs15-integrated-acceptance-and-balance.md#group-9---exercise-lifecycle-scenarios-by-feature-family), [Group 11](r2mlvs15-integrated-acceptance-and-balance.md#group-11---playable-loops-and-failure-recovery) and [Group 12](r2mlvs15-integrated-acceptance-and-balance.md#group-12---playable-loops-and-failure-recovery). Cover S1/S2 plus ground S8/S9, control/script parity, turns/backing, capacity/funds/receiver failure and real final-use conversion from normal declared starting conditions.
- [ ] 11.5 Reconcile [Plan 4 Group 20](r2mlvs4-tunnel-boring-machines.md#group-20---recovery-presentation-and-acceptance) with [Plan 15 Group 16](r2mlvs15-integrated-acceptance-and-balance.md#group-16---measure-complete-ground-project-economics) corridor rows and [18.1](r2mlvs15-integrated-acceptance-and-balance.md#group-18---economy-and-performance): complete setup/work/idle/support/haul/disposal ledger and no perpetual spoil-profit exploit. Update only the canonical owning tuning tables when measured results justify it.
- [ ] 11.6 Execute ground/related utility workloads from [Plan 15 Groups 21-23](r2mlvs15-integrated-acceptance-and-balance.md#group-21---set-reproducible-performance-workloads-and-budgets): bounded per-tick work, memory, route planning, save/load and real join reconstruction. Orbital fleets/handling workloads remain pending.
- [ ] 11.7 Execute target/interop art rows of [Plan 15 Groups 27-29](r2mlvs15-integrated-acceptance-and-balance.md#group-27---build-the-runtime-art-coverage-matrix), with [Plan 13 Group 87](r2mlvs13-graphics-and-asset-pipeline.md#group-87---runtime-integration-and-review): actual loaded direction/state/zoom/occlusion/sound coverage, reviewed hashes and recorded user feedback. A template contact sheet cannot complete runtime art review.
- [ ] 11.8 Produce target player/recovery guidance and requirement/evidence mapping using scoped [Plan 15 Groups 30-31](r2mlvs15-integrated-acceptance-and-balance.md#group-30---art-documentation-and-completion). Show precise blocked reasons and demonstrate supported recovery; record every remaining supporting-plan limitation without treating it as a target pass.

### Group 12 - Close both target plans with evidence

{{SIMPLE}}
- [ ] 12.1 Execute/update [Plan 3 Group 18](r2mlvs3-sealed-portals-and-deep-rail.md#group-18---controls-art-and-acceptance) after every remaining Plan 3 task, including new 8B, is complete with its required evidence. Record current public exports, true commissioning/occupied saves, tube/riser behavior, final graphics and network results.
- [ ] 12.2 Execute [Plan 4 Group 22](r2mlvs4-tunnel-boring-machines.md#group-22---recovery-presentation-and-acceptance) after every remaining Plan 4 task, including new 2B, is complete. Preserve original inputs and document delivered work/cargo/support/cost/recovery semantics.
- [ ] 12.3 Reconcile supporting [Plan 5 Group 14](r2mlvs5-spoil-wagons-and-haulage.md#group-14---lifecycle-and-acceptance) and delivered-family documentation from [Plan 14 Group 22](r2mlvs14-savegame-determinism-and-lifecycle.md#group-22---entity-lifecycle-and-multiplayer), [Plan 15 Groups 32-33](r2mlvs15-integrated-acceptance-and-balance.md#group-32---art-documentation-and-completion) and [Plan 2 Group 19](r2mlvs2-geology-and-excavation.md#group-19---presentation-and-validation). Complete a supporting whole plan only if its own Completion Conditions actually pass.
- [ ] 12.4 Check the coverage table below against all live child checkboxes and the final validation scopes in [Plan 3 Group 18](r2mlvs3-sealed-portals-and-deep-rail.md#group-18---controls-art-and-acceptance) / [Plan 4 Group 22](r2mlvs4-tunnel-boring-machines.md#group-22---recovery-presentation-and-acceptance), including required scenarios and asset families. Perform the justified final targeted build/regression/compatibility burst; document any failure and keep the affected target incomplete.
- [ ] 12.5 Apply the verified closure policy in [Plan 15 Group 32](r2mlvs15-integrated-acceptance-and-balance.md#group-32---art-documentation-and-completion) to the [parent Plan Index](r2mlvs-multilayer-vehicles-and-structures.md#plan-index). Mark Plans 3/4 complete only after their full Completion Conditions; do not complete M2/M3/M4/M5 or any unrelated plan from this queue.

## Completion coverage

| Target groups | Master execution checkpoints | Closure requirement |
| --- | --- | --- |
| 3: 1-4, 6-7, 10, 14 | 1.2; affected-boundary rechecks at 7/11 | Reuse completed core; changes retain its compatibility/date/control evidence. |
| 3: 5 | 3.4, 7.3 | Actual role/part admission and playable safe entry/retreat. |
| 3: 8, new 8B | 2.1, 2.3, 6.6, 7.1 | Geometry handoff plus full funded commissioning and source exchange. |
| 3: 9, 11 | 6.4, 7.2, 11.2-11.4 | Safe connection/lifecycle with real occupied/withdrawn consists. |
| 3: 12 | 9-10, especially 10.8 | Actual tube separation/interchange and commissioned dated riser. |
| 3: 13 | 8.2, 8.6-8.8, 11.7 | Final preview/tooltip/portal states with loaded reviewed assets. |
| 3: 15-17 | 7.3-7.5, 11.2-11.4 | Actual fleet, occupied files, dates/scripts, legacy and network proof. |
| 3: 18 | 12.1, 12.4-12.5 | Every original/additional task and Completion Conditions satisfied. |
| 4: 1-2, new 2B | 1.2, 2.3, 6.6 | Preserve completed roles/guards; complete full acquisition/rollback/service/date checks. |
| 4: 3-5 | 3.1-3.3, 3.7, 11.2 | Saved assignment/controller, canonical route adaptation and actual work handoff. |
| 4: 6-8 | 3.5-3.7, 7.1, 7.6 | Actual once-tick work, atomic spoil/funds/support and paid solid startup. |
| 4: 9-15 | 4.1-4.7, 10.8, 11.4-11.6 | Safe geometry/accounting/reservations and actual final-use conversion. |
| 4: 16 | 8.3, 8.6-8.8, 11.7 | Complete state display, real work/service/reverse sprites and sound events. |
| 4: 17-20 | 6.3, 7.6-7.8, 11.3-11.5 | Recovery, conservation, recurring haulage, visibility and measured full cost. |
| 4: 21 | 10.8, 11.2/11.4 | Legal/illegal turns, saved heading/spoil and actual non-rail build after withdrawal. |
| 4: 22 | 12.2, 12.4-12.5 | Every original/additional task and Completion Conditions satisfied. |

## Completion Conditions

Both target plans have every required task checked with retained evidence and satisfy their complete outcomes. A real rail-only TBM starts from a sealed solid portal without free excavation, pays for all swept cuts/support, carries both spoil types in its actual wagons, turns legally, retreats/recovers safely, exchanges with a secondary locomotive and disposes/returns empties through its Surface origin. Actual Deep dates, tube/riser restrictions, post-withdrawal non-rail use, occupied saves, hidden-layer/headless operation, multiplayer joins, final loaded graphics/sound and full cost are verified. Supporting plans retain unfinished unrelated scopes and the parent index reflects only demonstrated completion.
