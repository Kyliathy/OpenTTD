# R2TBMF N - Assigned haulage advice and vehicle status

Owner: [fixing Group6](../r2tbmf-tbm-fixes.md#group-6---remove-false-order-warnings-and-show-actual-work-status). Architecture: [vehicles](../../../architecture/vehicles/archi-vehicles.md), [GUI](../../../architecture/gui/archi-gui.md). Group5B is closed in [L](r2tbmf-l-spoil-notes.md#group5b-closure).

**Artifact-layout migration (2026-10-09):** this plan consumes the parent [Development artifact layout](../r2mlvs-multilayer-vehicles-and-structures.md#development-artifact-layout), implemented by [R2FC](../r2fc-file-cleanup.md). It supersedes earlier docs asset/review/helper placement and fixed runtime-config destinations below. New native homes use `mlm/runtime/`, generated feature reviews use `mlm/reviews/`, logs remain under `logs/`, and reusable code/curated inputs keep their code/test owners. Paths and focused native/cold verification are complete; R2FC records the results and retained-input identities. Repository-relative executable commands run from the checkout root through their context-aware wrappers. Historical pass records remain dated evidence and are not new acceptance claims. No gameplay task is completed by this migration and the separate post-demo fixture-pruning gate remains.

## Resume scope

Group6 compaction count0. Read fixing Global/user decisions and the nearest Group6 Local Mandatory Reading after compaction. Preserve the user's simpler-implementation/focused-testing correction: no fractional cargo; Basalt Earth remains1 tonne. Do not reopen completed movement/pacing/cargo checks. Group6 is complete; use the closure below rather than repeating its verification.

Task6.1 is verified using M's existing current native traces: both haul50 and old-work show14 too-few-orders warnings for hauler0 and14 for cutter5 over20,000 ticks. Actual cycles and excavation advance, so the warning concerns the manual station count, not absent managed work. No baseline run was added. Tasks6.2-6.4 are now closed by the evidence below.

## Findings already read

- `CheckOrders` in `src/order_cmd.cpp` reviews the first vehicle of a shared-order chain every20 vehicle days. It checks dummy/unusable/duplicate entries, then issues `STR_NEWS_VEHICLE_HAS_TOO_FEW_ORDERS` for fewer than two station orders. No TBM assignment query is made.
- `TBMExchangeAssignment` already owns the hauler, project, bay, actual wagon set, cycle phase and wait. `ObserveTBMHaulerInputs` checks current project/origin/actual chain; `GetTBMExchangeBay`, `CheckTBMOriginAdmission`, ordinary origin orders and `GetTBMHaulerSteeringTarget` supply existing route facts. ReturnEmpties alone overrides steering; loaded delivery uses ordinary station orders. Do not invent a duplicate order queue.
- `GetVehicleStatusString` in `vehicle_gui.cpp` already prioritizes crash, breakdown, stopped/stopping, depot unbunching and stuck states, then actual station/depot/loading orders. Its default displays No orders when a controller-driven cutter has no manual list. Preserve these useful native states; project status should describe the actual controller where it owns travel/work.
- Existing phase/hold text helpers are `GetTBMPhaseString` and `GetTBMHoldString` (used by `tbm_projects_gui.cpp`); hauler phase/wait text already exists in the portal presentation. Inspect their actual exported owner before adding another mapping.
- `src/tests/tbm_hauler_cycle.cpp` is a small controller test with declared physical placements and stand-in cargo disposal. For real native warning-cycle evidence reuse Group1B's retained log and the newly completed M native20,000-tick runs. Do not mistake that older controller-only test for native driven acceptance.

## Next implementation boundary

Keep normal dummy/invalid/duplicate-order warnings. Replace only the inapplicable station-count diagnosis for an actual managed assignment, and surface useful current missing-origin/bay/owner/order failures. A released train immediately returns to ordinary review. Shared lists must not let one managed train suppress advice for ordinary peers. Use existing assignment/controller owners and native news/status presentation; no new saved state is required.

Batch advice/status implementation and focused failure/release/shared-order cases before building. The final native run should cover several existing review periods and actual visible controller status. Reuse original diagnosis rather than rerunning it merely to obtain another baseline.

## Delivered implementation

- `GetTBMOrderProblem` in `tbm_controls.h/.cpp` returns no value for ordinary trains, INVALID_STRING_ID for a valid managed route, and an existing/new localized actionable reason for a missing project/origin/bay/receiver/order, mismatched ownership/wagons or a lost route away from its admitted bay. It derives everything from current owners; no saved state is added.
- `CheckOrders` retains ordinary invalid/dummy/duplicate checks, uses the managed health result for station-count advice, and chooses the first ordinary shared-list peer independently of managed peers. Managed trains review their individual assignments; release immediately returns to native review.
- `GetTBMVehicleStatus` in the existing project presentation source reuses portal phase/wait strings, preserves native service/loading priority, and adds speed plus current work/hold text. `vehicle_gui.cpp` invokes it after the existing crash/breakdown/stopped/stuck handling. Real outbound station orders retain their native destination caption.
- `src/tests/tbm_advice.cpp` contains three focused cases for normal/paused/released/reassigned behavior, seven genuine/manual-order faults, and mixed shared-order peers. It uses actual commissioned machinery, built haulers, receivers and origin orders. Direct declared faults only exercise review admission; they do not claim native movement.
- Native observer `tbm_feedback_runtime.cpp` has an advice flag on the retained balance loop, skips replacing M's saved evidence, captures actual cutter work and moving empty return, and checks no repeated too-few-order entries over20,000 ordinary ticks. [Runner O](../../../../../src/tests/multilayer/tbm/native_advice.py) isolates output, preserves startup/config/save bytes and retains actual rendered pixels.

## Group6 closure

All four tasks are verified. Final optimized game/test targets built successfully (`logs/r2tbmf-n-advice-caption-build.log`). `[tbm-advice]` passes418 assertions/three cases in0.37 seconds (`logs/r2tbmf-n-advice-focused.log`). [Runner O](../../../../../src/tests/multilayer/tbm/native_advice.py) passes38 native assertions in2.48 seconds, loading the retained393 paid haul scenario through ordinary startup and running20,000 real ticks. `logs/r2tbmf-o-native-advice.log` contains `ADVICE_RESULT ticks=20000 too_few=0 cutter_capture=true hauler_capture=true`, compared with14 false entries per participant in M's original same-scenario log.

Rendered captures were visually inspected: [working](../../../../../mlm/reviews/tbm/work-advice/import-20261009/images/tbm-advice-running.png) shows the cutter at0mph boring and the loaded hauler at34mph heading for its real station; [empty return](../../../../../mlm/reviews/tbm/work-advice/import-20261009/images/tbm-advice-empty-return.png) shows the cutter waiting for empty hoppers and the hauler at6mph bringing empties. This exposed and corrected presentation of the native last-speed cache while physically held; simulation speed/movement was not changed. Short wait captions fit the standard native window. The fixture's known malformed company label is repaired through its existing public RenameCompany command only in the unsaved advice demonstration; retained save/config/startup bytes remain intact.

Focused cases preserve ordinary invalid/duplicate schedule advice, report missing origin order/station ownership, lost receiver acceptance, lost bay ownership and mismatched wagon ownership; valid assignments suppress only the station-count warning. Release immediately restores ordinary advice, reassignment suppresses it again, and an ordinary shared-list peer still gets one appropriate review even with a managed peer. Paused/held captions report zero speed; native service/loading and earlier stopped/breakdown/crash/stuck presentation keep priority. No schema, cargo units, private order queue or saved status was added.

The native loop still reaches20/20 cuts with the same228/456 total spoil,9,493,990 company money and88,439 train upkeep as M. Advice/presentation does not advance work, consume RNG or change cargo/haulage. Actual parking, claim release and alternate destinations remain later groups.

No build/test or Git write is live. Next: Group7 nearest Local Mandatory Reading plus the fixing plan's parking contract; do not rerun completed movement/pacing/advice suites without a new affected behavior or failure.
