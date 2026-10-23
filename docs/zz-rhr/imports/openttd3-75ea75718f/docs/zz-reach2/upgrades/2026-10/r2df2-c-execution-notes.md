> **Historical donor snapshot — not an execution plan.** OpenTTD3 `75ea75718f`, imported2026-10-23.
> Original: `docs/zz-reach2/upgrades/2026-10/r2df2-c-execution-notes.md`; exact source SHA256 `0a7a1871f77f987a7ae7b1ccb40d92d894a9a7dea9ebad22611c99c108b7ab96`.
> Current integration: [M2O23-1](../../../../../../../zz-reach2/upgrades/2026-10/m2o23-1-come-space.md); storage and evidence roles: [archi-historical-debug.md](../../../../../../../zz-reach2/architecture/archi-historical-debug.md). Original completion claims, instructions and human-review limits below remain dated; they grant no new execution, spending, publication or deletion authority.

# R2DF2-C - Execution notes, 2026-10-16

Companion of [R2DF2](r2df2-face-exchange-and-demo-acceptance.md). These are working notes carried from group to group: what was found stale, what later groups must know, and failures that predate this plan. The design itself is in [R2DF2-A](r2df2-a-face-exchange-design.md).

## Carried from R2DF

- **R2DF handoff.** The R2DF handoff for task 11.3 is in [r2df-a-reproduction-notes.md](r2df-a-reproduction-notes.md#handoff-notes-for-r2df2-task-113).
- **Known pre-existing failures for task 10.3.** These were confirmed during R2DF with its own changes switched off:
  - `[tbm-movement]` `tbm_movement.cpp` 327 (4 assertions), 370 (2), 1029, 1094 and 1163;
  - `[tbm-exchange-acceptance]` 77, 133 and 187;
  - `[spoil-receiver-stall]` 76;
  - `[tbm-coverage]` 236;
  - `[tbm-corridor]` 113;
  - `[tbm-hangar-reuse]` 419.

## Group 2 (2026-10-16)

- **Stale line references.**
  - The plan's `GetTBMCutterSteeringTarget` lines 205-217 are now `tbm_support.cpp` 283-296, after R2DF Group 12.
  - `ValidateTBMProject` also capped phases at `FollowOrders`; it now caps them at `AwaitFaceTransfer`. The save-file bound in `tbm_sl.cpp` deliberately stays at `FollowOrders` until task 9.1.
- **Meeting-point rule as built.**
  - In a working phase, the front may stand anywhere from the approach of the cut before the last one up to the last approach (`front >= before` on the path). This includes the side cell of a diagonal advance, so the meeting point does not flicker while the cutter rolls onto new support.
  - In a Return phase the front must be exactly on the last approach.
- **Corridor rule as built.** Parts with `track == Track::Depot` do not count. The two corridor queries live in `tbm_exchange.cpp`, next to the assignment store, and are declared in `tbm.h`.
- **For Group 3.** Code outside `tbm_exchange.cpp` must use `GetTBMExchangeAssignmentForHauler`; the `Find…` variant is file-static.
- **For Groups 4 and 6.**
  - The next cut's sweep includes the consist's whole planning history, the cells behind the front. A consist packed onto fewer cells than its planning points therefore has no stand until it stretches out, and the meeting point stays Bay.
  - This is most likely right after a diagonal section, because the path then has more cells than cutter positions.
- **Performance.** The work-rail path over 1,024 cuts costs about 19 µs per query in RelWithDebInfo; 1,000 queries take 19 ms.
- **New pre-existing failure for 10.3.** `[tbm-project]` "Project controls pause and resume without touching orders or phase" fails at `tbm_project.cpp` 93, 94, 98 and 110.
  - `AddTravelOrder` now hands the cutter to its list (R2TBMF8 `FollowTBMVehicleOrders` on order insertion), so the phase is FollowOrders and the revision is one higher.
  - Neither `order_cmd.cpp` nor the test changed since HEAD, so this is a stale expectation, not a Group 2 regression.

## Group 3 (2026-10-16)

- **Stale gate tag names.** The Groups 3-4 entry gate names `[demo-fix-signal-boundary]` and `[demo-fix-obstruction]`. R2DF delivered them as `[demo-fix-portal-signals]` and `[demo-fix-tbm-obstruction]`, and both pass.
- **Rules as built.**
  - **Carry rule.** A hauler runs on its own project's support, and also on **released** support (`IsTBMSupportReleased`), because R2DF Group 7's reuse keeps the old project's ID on reused cells. Another live project's support stays closed to it.
  - **Pass-through rule** (`IsTBMHaulerAdmittedThroughPortal`). It applies on the way in for the face turn holder, and on the way out whenever a part is **beyond the key** (on the housing or the corridor).
    - The key itself does not count. A non-turn-holder rolling onto the key to queue must still be stored, which `IsTBMHaulerInCorridor`, counting the key, would have prevented.
    - Magazine exit: shaft side for a stored turn holder, Invalid for one passing through.
  - **Turn holder** (`IsTBMFaceTurnHolder`). An eligible hauler is in ReturnEmpties or AwaitExchange, with no wait or only `OccupiedBay`, and carries its own empty set covering the frozen pass terms. The turn holder is the lowest eligible ID, and holds the turn only while no other hauler of the project is inside the corridor.
  - **Safe waiting** (`IsTBMHaulerFaceStand`). It refuses cheaply first: no assignment, or not plain rail.
- **Transitional failures until Group 6. Do not chase them before then.** *(Corrected in Group 4: only `[tbm-travel-uturn]` was transitional, and it passes again since Group 4. `[spoil-receiver-delivery]` 41, 42 and 90 are pre-existing: R2DF Group 6B recorded them failing identically on the baseline build, because the cut yields less spoil than the test assumes.)*
  - `[tbm-travel-uturn]` (`tbm_movement.cpp` 1119, cutter crashed) and `[spoil-receiver-delivery]` (41, 42, 90: fewer tonnes delivered) fail since Group 3.
  - In these self-driving source loops the hauler is now the face turn holder. It reaches the key, is no longer stored, and follows its bay target on through the shaft into the cutter.
  - Group 4 (steering to and holding at the stand) and Group 6 (the face wait and transfer dispatch) complete the loop. Re-run `[tbm-loop]`, `[tbm-travel-uturn]`, `[spoil-receiver-delivery]` and the other self-driving loops after Group 6. They should then pass with face transfers, or their bay-only expectations must be updated as the plan intends.
- **Neighbouring tags that pass after Group 3:**
  - `[portal-recovery]`, `[spoil-receiver-leg]`, `[tbm-conservation]`;
  - `[tbm-exchange-commit]`, `[tbm-exchange-load]`, `[tbm-exchange-origin]`, `[tbm-exchange-proposal]`;
  - `[tbm-hauler-cycle]`, `[tbm-support]`, `[tbm-travel-hold]`, `[tbm-wagon-sets]`;
  - `[demo-fix-portal-signals]`.
- **Known failures, unchanged:**
  - `[tbm-travel-junction]` 1029;
  - `[tbm-travel-service]` 327 ×4;
  - `[spoil-receiver-stall]` 76;
  - `[tbm-exchange-acceptance]` 77, 133 and 187;
  - `[tbm-corridor]` 113.

## Group 4 (2026-10-16)

- **Shared driving fixture.** `SourceLoop` and its helpers moved from `tbm_movement.cpp` into `src/tests/tbm_source_loop_fixture.h`, with a new `LoopStart::Long` (14 straight cuts). The `[tbm-loop]` line numbers moved: the known 1094 and 1163 are now 774 and 843.
- **Room rule found natively in the test.**
  - A stand too close to the housing let the turn holder stop with its last part still stored in the key's magazine. The F6 reversal then hit `TrackBitsToTrack`'s assertion in `TrainController`.
  - The meeting point now needs `tail >= GetTBMHaulerRoomCells + 2`. As a result, on `LoopStart::Long` the first Face meeting comes around cut 8, and the first breakpoint is at 11.
- **Turnaround as built.**
  - A hauler facing the stand (its moving front's trackdir equal to the stand's) is held in every phase, so it never passes the stand.
  - In a leaving phase it turns there, once, with no stored part. This replaces the plan's one-shot reversal on the phase change.
- **F7 as built.** In a Return phase, `GetTBMTravelHoldReason` returns `Obstruction` while `FindTBMHaulerInCorridor` names a hauler, and `FindTBMObstruction` names that hauler for the status line.
- **Possible pre-existing defect with `reserve_paths` on (not caused by this plan; for task 10.1 or a follow-up).**
  - During cut 1 the cutter's tail stays on the key (the approach for cursor 0 is the housing), so the key counts as a busy magazine. A hauler heading for the Source bay then finds no path, becomes `BlockedLine`, and `GetTBMLogisticsWait` turns that into `LogisticsStall`, which holds the cutter: a deadlock at cursor 0.
  - Seen in `[demo-fix-face-steering]` with reservation on from the start. The test now turns reservation on at the first Face meeting, which is what task 4.6 asks to prove.
- **Evidence.**
  - `[demo-fix-face-steering]` (2 cases, real movement):
    - stands reached and followed, two in a row, with reservation off and on;
    - top hauler speed on support 40 (`TBM_SUPPORT_MAX_SPEED`), held on the stand tile with no part past it, in the next sweep, or on a cutter tile;
    - after a Return and park with nothing aboard, the hauler turns and leaves, while the cutter stays still, blocked by `Obstruction` naming the hauler, then backs to its magazine and parks.
  - `[demo-fix-face-access]` and `[demo-fix-face-meeting]` pass.
  - Neighbours pass:
    - `[tbm-hauler-cycle]`, `[tbm-exchange-bay]`, `[tbm-exchange-commit]`, `[tbm-exchange-origin]`;
    - `[tbm-travel-hold]`, `[tbm-travel-uturn]`;
    - `[demo-fix-tbm-obstruction]`, `[demo-fix-tbm-status]`, `[demo-fix-portal-signals]`;
    - `[spoil-receiver-leg]`, `[tbm-wagon-sets]`, `[tbm-support]`, `[portal-recovery]`, `[tbm-controls]`, `[tbm-advice]`.
  - Known failures remain: `[tbm-loop]` 774 and 843, and `[spoil-receiver-delivery]` 41, 42 and 90.
- **For Group 6.**
  - `ControlTBMProject`'s back-out rule (R2DF task 12.6) still lists only Bore and AdvanceSupport; add AwaitFaceTransfer (task 7.1).
  - `AdvanceTBMHauler` still calls the bay exchange whenever `cutter_waiting`. Task 6.4 must dispatch to the face transfer, and must call `BackUpTBMHauler` only through the turnaround rule, or not at all, since the rule already turns a hauler leaving the stand.

## Group 5 (2026-10-16)

- **Transfer as built.** Describe, validate and execute in `tbm_exchange.cpp`; constant and declarations in `tbm_exchange.h`. See F3 "As built (Group 5)" in [R2DF2-A](r2df2-a-face-exchange-design.md) for the three refinements: the stand check uses the moving front and trackdir, the pool check counts source plus destination parts, and `NoSpoil` is unreachable after the readiness rule.
- **Fixture facts worth carrying.**
  - `TBMWorkRig::Reload` calls `ResetTBMProjectState`, which also clears the exchange store. Assign the hauler after any reload, or it is `NotAssigned`.
  - The TBM engine fixture rebuilds the cargo specs, so spoil weight (16, freight) must be set after the rig exists, as `tbm_work.cpp` does. Before that, spoil has no mass and the weight caches cannot show a stale value.
  - On the default 24-cut rig, the breakpoints are 11 and 24. Once the cutter is clear of the housing (cut 12), the only breakpoint left is the route's end; the breakpoint wait uses a 40-cut route.
- **New shared test pieces** in `tbm_face_fixture.h`: `TBMFaceTransferRig` (a cutter on its face with the last cut's spoil aboard and the exchange due, in ReturnToPark or AwaitFaceTransfer, its hauler assigned and stopped on the stand), `TBMFaceSnapshot`, `TBMPacketTotals` and `CountTBMPackets`. Groups 6 and 7 can reuse them.
- **Warning fixed outside the plan's files.** `underground_gui.cpp` (R2DF change-route summary) used `CeilDiv`, which takes `uint`, on 64-bit work ticks: MSVC C4267 and a real truncation. It now divides in 64 bits; `[demo-fix-change-work]` and `[demo-fix-change-work-ui]` pass.
- **Evidence.**
  - `[demo-fix-face-transfer]` (3 cases) passes:
    - success in ReturnToPark and in AwaitFaceTransfer at a mid-route breakpoint, conserving tonnes per cargo, source tile, first station, age and feeder share, with money unchanged, sole packet custody, both chains unchanged and both weight caches updated;
    - the set is Loaded without the origin visit, the assignment is CollectLoaded with one more exchange, and `MayTBMSpoilLeaveVehicle` allows the spoil to leave at the origin only;
    - estimates are repeatable and change nothing;
    - every refusal, and the earlier of two, leaves the snapshot unchanged.
  - `[demo-fix-face-transfer-pool]` passes in 1.5 s: Limit with a full pool and with one slot short, then success.
  - Neighbours pass: `[demo-fix-face-meeting]`, `[demo-fix-face-access]`, `[demo-fix-face-steering]`, `[tbm-exchange-proposal]`, `[tbm-exchange-commit]`, `[tbm-exchange-bay]`, `[tbm-hauler-cycle]`, `[spoil-receiver-leg]`, `[tbm-work]`.
- **For Group 6.** `AdvanceTBMHauler` must call `ExecuteTBMFaceTransfer` for the turn holder when the meeting is the Face, and the bay exchange otherwise. The transfer itself sets the assignment to CollectLoaded, so the turnaround rule then turns the hauler on the stand.

## Group 6 (2026-10-16)

- **As built.**
  - **Edges.** `StepTBMProjectPhase` has the Bore and AdvanceSupport edges into the face wait, and the edges out of it: to `ReturnForExchange`, `Bore` or `AdvanceSupport`, plus the stand-lost fallback. The wait also joins the cancel and service-rejoin lists.
  - **Hold.** `leaving_face` and `IsTBMHoldingTrain` include the wait.
  - **Hauler dispatch.** `AdvanceTBMHauler` dispatches on `IsTBMReadyForFaceTransfer`, maps `InsufficientEmpties` to `NoEmpties`, and turns the hauler at once after a transfer (the shared `TurnTBMHaulerAtStand`).
  - **Logistics.** `GetTBMLogisticsWait` reports `NoHauler` for an unserved face wait.
  - **Order review.** `GetTBMOrderProblem` no longer reports a lost line for a hauler held at its meeting point (`IsTBMHaulerHeld`).
  - **Wording.** `GetTBMPhaseString` has the wait. A new overload, `GetTBMHaulerPhaseString(const TBMExchangeAssignment &)`, gives the face turn holder "bringing empty hoppers to the TBM" and "taking spoil from the TBM". The pair window and the vehicle status line use it.
- **Defect found and fixed: stand trackdirs.** In the Deep wide-chamber loop, a stand tile carried both an X piece and a diagonal piece. The work path named X_SW, but the hauler came in on LEFT_S, so the hold never fired and the hauler struck the cutter at speed 40. `GetTBMFaceStandTrackdirs` now accepts every laid piece that leaves towards the cutter (design F4).
- **New test pieces.**
  - `src/tests/tbm_face_loop_watch.h`: `TBMFaceLoopWatch`, a per-tick observer usable with `SourceLoop` and with the workflow driver.
  - `src/tests/tbm_face_controller_layers.cpp`: the workflow-driven chamber loops. The workflow fixture and the TBM rig fixtures have never shared a translation unit, so they are kept apart.
- **Route facts.**
  - The narrow workflow route (9 cuts) never meets at the face on Deep: the room rule applies, with breakpoints 5 and 9. Underground's only breakpoint is 9.
  - The wide chamber (20 cuts) has breakpoints 7 and 20 on Underground, and 4, 17 and 20 on Deep.
  - `SourceLoop` `Long` (14 straight cuts) serves both of its breakpoints at the face.
- **Pre-existing failures (not caused by this plan; for task 10.3).**
  - `[tbm-economics]` line 45, `exchanges > 1`. The Underground workflow route freezes a single breakpoint (`exchange_after = {9}`, printed by the Group 6 diagnostic), so one exchange is all any flow can make, at the bay or at the face. The Deep context passes.
  - `[tbm-logistics-queue-save]` line 72, `first != nullptr`. The two haulers collide, and the first one's deletion ends its assignment.
    - **Trace.** The meeting point is the Bay throughout, there is no stand, and no approach tile is support. Every R2DF2 branch on this path is additive and stays inert: the Group 3 admission and exit branches, the Group 4 magazine leave, and the Group 2 support class.
    - **Cause.** The second hauler, already in the magazine, takes the exchange. It leaves the key through the door while the first creeps along the single-track approach (6026) and the two meet head on.
    - `CheckTrainStayInMagazine` has no occupied-line check of the kind `CheckTrainStayInDepot` makes (`UpdateSignalsOnSegment` Full). Candidate follow-up for task 10.1: give magazine departure the depot's segment check.
- **Evidence.**
  - `[demo-fix-face-controller]` (5 cases, 224,030 assertions) passes:
    - every edge, including the bay fallback, cancel, service and holds;
    - hold, logistics and wording;
    - dispatch, `NoEmpties`, and ordinary trains left alone;
    - the Standard 14-cut loop with both breakpoints served at the face, no bay visit before the final return, the cutter still while waiting and while obstructed, backing at the return, and the lost-at-stand order review;
    - the chamber loops of both layers: Deep serves cuts 17 and 20 at the face, and both finish Completed with all spoil delivered.
  - Neighbours pass:
    - the face tags `[demo-fix-face-meeting]`, `[demo-fix-face-access]`, `[demo-fix-face-steering]`, `[demo-fix-face-transfer]` and `[demo-fix-face-transfer-pool]`;
    - the bay tags `[tbm-hauler-cycle]`, `[tbm-exchange-bay]`, `[tbm-exchange-commit]`, `[tbm-exchange-origin]` and `[tbm-exchange-proposal]`;
    - the hold and status tags `[tbm-travel-hold]`, `[tbm-travel-uturn]`, `[demo-fix-tbm-obstruction]`, `[demo-fix-tbm-status]` and `[demo-fix-portal-signals]`;
    - the controls `[tbm-controls]` and `[tbm-advice]`;
    - `[spoil-receiver-leg]`, `[tbm-work]`, `[tbm-wagon-sets]`, `[tbm-support]` and `[portal-recovery]`;
    - every workflow-driven tag except the queue case above: `[underground-workflow-*]`, the `[tbm-logistics-*-save]` cases, `[portal-occupied-save-file]`, `[tbm-exit-journey]`, `[tbm-parking*]`, `[tbm-pacing]`, `[tbm-geology*]`, `[tbm-planning-refusal]`, `[acceptance-ground-fixture]` and `[m1-*]`.
  - Known failures unchanged: `[tbm-project]` 93/94/98/110, `[tbm-loop]` 774/843, `[tbm-movement]` 349 ×2, `[spoil-receiver-delivery]` 41/42/90, `[spoil-receiver-stall]` 76 and `[tbm-exchange-acceptance]` 77/133/187.
- **For Group 7.** `ControlTBMProject`'s back-out rule (line 565, `before == Bore || AdvanceSupport`) still needs the face wait (task 7.1). The face wait is not in `IsTBMWaitingForExchange`, so park and release map it to the Return phases as the plan says.

## Group 7 (2026-10-16)

- **As built.**
  - **Back-out.** `ControlTBMProject`'s back-out list and `BeginTBMHangarReturn` include the face wait, which backs out at once; F7 holds its motion.
  - **Settlement.** `GetTBMProjectSettlementBlocker` returns a hauler still on the corridor or the pair (`IsTBMHaulerInCorridor`).
  - **Far exit.** `CheckTBMExitHandover` refuses with the new `STR_ERROR_TBM_EXIT_HAULER` right after its unfinished-job check.
  - **Meeting refinement (design F1, "Held return").** A Return phase also meets at the face from the approach of the cut before, while a hauler is already in the corridor.
- **Finding behind the refinement.** At a breakpoint the commit leaves the cutter one cut short of the face. Without the refinement, a park, release or hangar request during the face wait made the meeting point the Bay, and the hauler waiting on the stand would leave unserved. The lifecycle test showed this.
- **Plan facts checked, no change needed.**
  - `IsTBMWaitingForExchange` excludes the face wait, so park and cancel map it to `ReturnToPark` and `ReturnToRelease`, and the hangar order to `ReturnForHangar`.
  - `ReplaceExcavationJobRemainder` keeps a breakpoint at the cursor, so a change during the wait keeps the exchange due.
  - A train in the corridor cannot be sold (the sale needs a depot). Deletion after a crash goes through `OnTBMExchangeTrainPartDeleted`, which ends the assignment.
  - After that, the loaded cutter's hold clears and its logistics wait is `NoHauler`.
- **Test facts.**
  - `GetTBMProjectReleaseProblem` reports `RELEASE_RETURN` until the project is withdrawn. The haulage reason, and the hauler's name, apply from `ReleasePending`.
  - `FindTBMObstruction` reads the saved hold, so an in-memory check needs one controller tick first.
  - The change quote needs spoil receivers (`STR_ERROR_TBM_COMMISSION_RECEIVER` on the bare face rig), so that case runs on `SourceLoop`.
  - `SendTBMToHangar` to the loop's own Source key works as a hangar order.
- **Shared watch.** `TBMFaceLoopWatch` now classifies an exchange by the meeting point on the step before it, not by the face-wait phase. Transfers in a Return phase held at the face therefore count as face transfers.
- **Evidence.**
  - `[demo-fix-face-lifecycle]` (6 cases, 210,437 assertions) passes:
    - park, release and hangar, each with the hauler on the stand (one transfer at the face, no bay exchange, the cutter still while obstructed and backed at the request) and without one (a bay exchange as before);
    - outcomes `Parked`, `Cancelled` (through `ReleasePending`) and `TravelToHangar`/`ParkedHangar`;
    - the change of work during the wait;
    - settlement and the far exit;
    - stop, sale and crash in the corridor.
  - Neighbours pass:
    - the face tags `[demo-fix-face-*]`;
    - `[tbm-hangar-orders]`, `[tbm-hangar-service]` and `[tbm-selected-hangar]`;
    - `[tbm-parking]` and `[tbm-parking-depot]`;
    - `[tbm-exit-journey]`, `[demo-fix-change-work]` and `[demo-fix-change-work-ui]`;
    - `[tbm-controls]`, `[tbm-hauler-cycle]` and `[tbm-exchange-lifecycle]`;
    - `[tbm-travel-hold]`, `[tbm-travel-uturn]` and `[demo-fix-tbm-obstruction]`.
  - Known failures unchanged:
    - `[tbm-hangar-acceptance]` `tbm_hangar.cpp` 419, the remote quote. This is the recorded 419; the summary's tag name `[tbm-hangar-reuse]` was stale.
    - `[tbm-exchange-acceptance]` 77/133/187 and `[tbm-loop]` 774/843.

## Group 8 (2026-10-16)

- **Native driver.** A `face-loop` scenario in `src/tests/demo_feedback_runtime.cpp` (`[.demo-feedback]`), run through `src/tests/multilayer/tbm/demo_feedback.py --scenario face-loop`.
  - **Launcher options.**
    - `--view headless|visible|hidden` sets the camera mode.
    - `--face-transfers N` sets how many face transfers to observe (default 3).
    - `--equivalent-to <evidence.json>` compares this run's rows with an earlier run's.
    - `--bend` resumes the loaded project, starts its stopped trains and bends its route.
  - **Rows.** `DEMO_FACE_PLAN`, `_BEND`, `_CUT`, `_PHASE`, `_TRANSFER`, `_STATE`, `_RATE` and `_SUMMARY`. The launcher copies them into `reports/face-loop-evidence.json` of the run.
  - **Checks inside the observer.** For each face transfer: the cargo total is kept, the cutter is left empty, the dwell is at least `TBM_FACE_TRANSFER_TICKS`, and the hauler moves on to `CollectLoaded`. Over the whole run: at least N face transfers, and no bay exchange while a stand existed (`bay_with_stand`).
- **Underground loop (8.1).** The `tbm-work-403` snapshot, with the default hauler (Train #1).
  - Runs: headless `20261016-034943-9791bacb86`, visible `20261016-034946-94ac7459bf`, hidden `20261016-034950-1e4a32b490`.
  - The breakpoints are 11, 25, 35, 42, 44 and 50. Three face transfers took place, at cuts 11, 25 and 35 (ticks 7,767, 15,888 and 22,965).
    - The stand was on the straight corridor, five tiles behind the cutter front, for example stand (13,15) with the cutter at (13,20).
    - The dwell was 74 ticks. The hauler stood on its stand for 456, 455 and 879 ticks before each transfer.
    - 378, 378 and 315 units moved in 24, 30 and 22 packets, which left the cutter empty.
    - Money did not change on the transfer tick, and the cutter's tile did not change during the wait.
  - There was no bay exchange.
    - Up to cut 8 the meeting point was still the bay: there was no room for a stand yet (rule F1). The hauler waited in the bay from tick 3,645.
    - It set off for the stand at tick 6,010, as soon as the meeting point became the face.
  - **Behaviour worth knowing.**
    - While the cutter bores on, the hauler follows the stand: `AwaitExchange`, then `ReturnEmpties`, then `AwaitExchange` again, as each cut moves the stand one tile.
    - After a transfer it visits the origin and unloads at the two Surface receivers, with short `SaturatedReceiver` waits of at most 23 ticks.
- **Deep loop behind a bend (8.2).** The input is `src/tests/fixtures/haulage/tbm-fresh-deep-tunnel-393.sav`: 1980, the Deep layer (3), a straight 9-cut tunnel, the project `Paused` and the hauler `Stopped` in its Surface depot.
  - Runs: headless `20261016-034953-698836afba`, visible `20261016-034958-dc709d7f7d`, hidden `20261016-035005-ca85be5de8`.
  - **Preparation, all through ordinary input.**
    - Resume is clicked in the projects list, and both trains are started with the ordinary start command.
    - After cut 1 the work is changed with `ChangeTBMWork` (the quote's cursor and fingerprint) into a tunnel at 45 degrees ending at (16,38).
    - The diagonal is then extended straight on twice, to (26,48) and (36,58). One change can reach only about six tiles: farther ends are refused with "This area is too far from the boring machine to plan a route" (plan error 18). The refusals are logged as `DEMO_FACE_BEND_CANDIDATE`.
  - **Result.**
    - Three face transfers took place, at cuts 11, 14 and 17. Each stand was a half-tile piece (`RIGHT_S`) of the diagonal, behind the bend; `bends_behind=5` counts the planner's 45-degree steps. 441 units moved each time, in 9 packets.
    - The three earlier exchanges (cuts 1, 5 and 8) were at the bay, each with no stand near the portal (rule F1, `bay_with_stand=0`).
  - **Observation, not retuned.**
    - The Deep cutter waits 3,805 to 5,641 ticks at the face for each breakpoint. Its hoppers fill every three cuts, while the hauler's round trip crosses three layers.
    - The hauler does arrive, and the transfer follows one tick later.
- **View equivalence (8.3).**
  - **Visible view.** The camera follows the cutter on its own layer every 16 ticks and draws a capture at each transfer.
  - **Hidden view.** The camera watches the Surface above the cutter and draws the same captures.
  - **Result.** Both views logged the same rows as headless, in both worlds. The rows cover the plan, bends, cuts, phases and transfers. They also include the state rows: 26 on Underground and 54 on Deep, every 1,000 ticks and at each transfer. Each state row holds the game `Random()` state, money, the geology and portal-work digests, and a digest of every train part's position, motion, status and cargo.
- **Captures (8.4).**
  - Underground: `mlm/reviews/demo-fixes/face-loop/20261016-034946-94ac7459bf/images/06-hauler-behind-cutter.png`. It shows the projects list reading "waiting for the hauler at the face; 11/50 cuts complete", with the cutter full and Train #1 empty behind it in the corridor.
  - Deep: the counterpart, at 11/18 cuts, in `.../20261016-034958-dc709d7f7d/images/`.
  - Each visible and hidden run also keeps `transfer-1..3`.
  - The first attempt fired while the hauler still waited in the bay, as the meeting point moved. The capture now requires the stopped hauler on the stand and the cutter in the face wait, and closes news popups first; these are presentation only.
- **Cut rate (8.5).** Same `tbm-work` route, same snapshot.
  - **Bay loop.** R2DF build, run `logs/demo-fixes/hauler-depot/20261015-220425-298fa44a13`, cut ticks read from its cutter rows (each `Bore` exit is a completed cut).
    - The ticks are identical to the face loop up to cut 11 (7,692). The bay detour then took 2,295 ticks to cut 12 (9,987).
    - Cut 22 came at 15,731 and cut 25 at 17,522. The count at 20,000 ticks is 25, with the cutter still returning from its second breakpoint (exchanged at 19,202).
  - **Face loop.**
    - Cut 12 came at 8,304, 612 ticks after cut 11, of which 75 were the face wait.
    - Cut 22 came at 14,023 and cut 25 at 15,813. The count at 20,000 ticks is 32, about 28% more.
  - No tuning was changed.
- **Carry onward.**
  - Group 10.1 can reuse `face-loop` for the step "the hauler meets the cutter at the face".
  - Shell heredocs again turned a `\n` inside a C++ string into a real line break (the test file has CRLF endings). Keep writing edit scripts with the Write tool.

## Group 9 (2026-10-16)

- **Version (9.1).** `SaveLoadVersion::TBMFaceTransfer` (425) is appended after `AsteroidStarterInputs` (424). The TBMP phase bound admits `AwaitFaceTransfer` from 425 only, and every older bound is unchanged.
  - Plan facts confirmed: no other wire field changes, and the TBXH/TBXA/TBXS bounds stay as they are (hauler phases and waits are unchanged).
  - The load validators name no phase list, and `ComputeTBMProjectDigest` hashes the phase byte.
  - `[demo-fix-face-save]` (in-process) checks the version and that the digest tells the face wait from `Bore` with every other field equal.
- **Real files (9.4, 9.5).** `src/tests/multilayer/tbm/face_save.py` drives all of this. Report: `mlm/reviews/demo-fixes/face-save/20261016-040823-cb0f416e9f/reports/face-save-report.json`.
  - **Writers.** Face-loop runs of the native observer (`demo_feedback.py --scenario face-loop --saves ...` or `--lifecycle ...`) ask for each checkpoint. The checkpoint is saved through the ordinary saver between two ticks, and the run logs its owners (`DEMO_FACE_CHECKPOINT`) and its next transition (`DEMO_FACE_NEXT`).
  - **Reader.** The native reader `[.demo-fix-face-save-file]` in `src/tests/tbm_face_exchange_save.cpp` loads each file with `-g`, logs the owners it holds, unpauses and logs the first transition. It shares `src/tests/tbm_face_native.h` (sample, transition and owner rows) with the observer.
  - **Owners compared.** Tick counter, game random state, money, and the project, exchange, geology, portal-work and train digests.
  - **Result.** All eight files load with the owners they were saved with, and reach the same transition at the same offset with the same owners:
    - `face-travelling`: cutter `AdvanceSupport`, hauler on its way in. Next: `cutter=3->2` after 42 ticks.
    - `face-held`: hauler on its stand while the cutter bores. Next: cut 9 after 247 ticks.
    - `face-dwell`: the face wait, half-way through the dwell. Next: the transfer after 38 ticks.
    - `face-leaving`: the loaded hauler on the pair. Next: cut 12 after 154 ticks.
    - `face-bay-return`: Deep, returning to the bay with no room for a stand. Next: `WaitForEmpties` after 93 ticks.
    - `face-hauler-stopped` (9.5): stopped in the corridor. Next: the cutter bores on after 41 ticks.
    - `face-return-held` (9.5): `ReturnToPark` held by F7. Next: the transfer after 74 ticks, counted on the return phase's timer.
    - `face-release-pending` (9.5): the loaded hauler leaving. Next: its `SaturatedReceiver` wait at the Surface after 516 ticks.
  - **Saving changes nothing.** The writers that saved checkpoints logged the same face-loop evidence as the Group 8 runs, row for row (`--equivalent-to` comparison: plan, cuts, phases, transfers, states, rate and summary).
- **Legacy (9.2).** Files written by the R2DF build (version 424) in `mlm/reviews/demo-fixes/hauler-depot/20261015-220425-298fa44a13/saves/`:
  - `stored-consist.sav` (`WaitForEmpties` at the bay, cut 25): the bay exchange completes two ticks after loading, and the next breakpoint (cut 35) is met at the face.
  - `hauler-departing.sav` (`Bore` at cut 11, after a bay exchange): the next breakpoint (cut 25) is met at the face.
  - A `ReturnForExchange` file: the Deep `face-bay-return` checkpoint, copied with version 424 in its header, reaches the same next transition as the 425 original. No genuine pre-425 file in that phase exists, and 425 changes no other field, so the copy stands for one.
- **Malformed (9.3).** `face-dwell` copied with version 424 (the face wait before it existed) is refused:
  - the preview returns `Error` and leaves the loaded world's owners unchanged;
  - the full load returns `ReInit`, after which the valid file reloads with the same owners.
  - The control copy, `face-held` at 424 (`Bore`), previews and loads.
- **Side effects of the version bump, fixed.**
  - Four Plan10 acceptance scripts asserted that the files they write are exactly version 424 (`asteroid_band_checks-1`, `asteroid_depletion_checks-1`, `asteroid_motion_checks-2`, `launch_site_checks-1`). They now accept 424 or later; the checks of genuine 424 inputs are unchanged.
  - `tbm_feedback_runtime.cpp` sized its per-phase frame counters by `FollowOrders`, so a face wait would have indexed past the end. They now reach `AwaitFaceTransfer`. This was latent since Group 6.
- **Carry onward.**
  - **Group 10.2.** The refreshed checkpoints will be version 425; `demo-checkpoints.json` and `play.ps1` records should say so.
  - **Group 10.3.** Add `[demo-fix-face-save]` to the burst. The native reader runs through `face_save.py` (about one minute).
  - The writer's owner row must be logged between ticks (`SaveCheckpoint`), not inside the tick timer: inside a tick, the TBM controller and the trains have not yet moved, so the row would not match the file.

## Group 10 (2026-10-16)

- **Entry gate.** R2DF Groups 1-12 and R2DF2 Groups 1-9 are all checked, with evidence.
- **Journey (10.1).** Every R2DF reproduction scenario and the face loop were replayed natively on the refreshed `tbm-work-425`. The default input of `demo_feedback.py` is now that file, the one `play.ps1 tbm-work` starts. Summary: `logs/demo-fixes/g10-journey.json`.
  - **R5, `hauler-depot`.** The loaded hauler waits 19 ticks in its Surface depot and leaves under normal signals (`full_decisions=0`).
  - **R6, `change`.** The area is changed while boring (cut 22) and while parked; the next cut is the new route's first.
  - **R8, `approach`.** The beside-exit area is accepted with a bent approach and dug out: floor 8/8, approach 9/9, nothing off support.
  - **R9, `obstruction`.** Not reproduced.
  - **R11, `unload-order`.** Three stops complete.
  - **R10, `face-loop`.** Three face transfers. The hauler unloads all 378 units at the two Surface receivers (378 → 282 → 186 → 90 → 0, ticks 8,643 to 11,505), then returns empty.
  - **`held` (R4).** The scenario no longer finds its state, because the cutter never returns loaded. The wait and cargo lines appear in the face-loop list capture instead.
- **Checkpoints (10.2).** A new `refresh` mode in `src/tests/multilayer/acceptance/underground_demo_checks.py` resaves `tbm-work`, `tbm-plan`, `tbm-exit` and `tbm-tunnel` through the ordinary `openttd.exe`, twice.
  - The resulting `*-425.sav` files sit beside the retained 403 snapshots. All 27 native owner chunks are unchanged against the 403 originals, and against each other after the second load.
  - `demo-checkpoints.json` gains the four records and a `refreshes` entry. `play.ps1` points `tbm-work`, `tbm-plan`, `tbm-exit` and `tbm-tunnel` at the 425 files; the dry run resolves them.
  - `verify` reloads all 12 retained snapshots on the fixed build (`mlm/reviews/acceptance/underground-demo/20261016-041659-d026356b1b/reports/retained-verification.json`).
  - `openttd.exe` had to be rebuilt: it dated from 2026-10-15.
- **Builds and burst (10.3).**
  - **Builds.** RelWithDebInfo and Debug, both `openttd_test` and `openttd`. The Debug tree needed `--clean-first`: its precompiled header came from another compiler version (C1853), from before the Build Tools install.
  - **Warning.** A Debug C4267 at `excavation_route.cpp` 511 is pre-existing; the line is at HEAD line 445.
  - **Scope.** 160 visible tags, matched by TBM, portal, exchange, excavation, spoil, hauler, small-map, signal, pbs, yapf, hotkey, layer-view and `demo-fix`, in chunks of 12 (RelWithDebInfo) or 6 (Debug).
  - **Result.** Both configurations show the same failure set. Every `[demo-fix-*]` tag passes in both. There were no Debug-only assertions.
  - **Reports.** `logs/demo-fixes/g10-burst-rel.json` and `logs/demo-fixes/g10-burst-debug.json`.
  - **Native face loop in Debug.** It logs the same face-loop evidence as the RelWithDebInfo run, row for row (34 s).
- **Every failure, all independent of the face exchange.**
  - Recorded by the R2DF handoff, with R2DF's changes switched off:
    - `tbm_movement` 349 ×2 (formerly 370), 774 and 843 (`[tbm-loop]`, formerly 1094 and 1163), 306 ×4 (`[tbm-travel-service]`, formerly 327) and 709 (`[tbm-travel-junction]`, formerly 1029);
    - `tbm_exchange_acceptance` 77, 133 and 187; `spoil_receiver_stall` 76; `tbm_coverage` 236; `tbm_corridor` 113; `tbm_hangar` 419.
  - Recorded in the R2DF notes for this task: `tbm_acquisition` 55 and 168-181 (yield drift); `excavation_outline` 56-57 (halved rate); `underground_depot_fixture.h` 269 (the depot files tags).
  - Recorded during R2DF2: `spoil_receiver_delivery` 41, 42 and 90; `tbm_project` 93, 94, 98 and 110; `tbm_economics` 45; `tbm_logistics_queue` 72.
  - Not recorded before, and failing identically with face meetings switched off (`GetTBMMeetingKind` returning Bay, each tag run alone, the source then restored by hash):
    - `tbm_engine` 146-157 (`[tbm-sealed-start-gate]`: the first cut yields 72 t of Rock where the test needs more than 128 t, the same drift as `[tbm-acquisition]`);
    - `tbm_commission` 357 (`[tbm-measure]`: the first cut needs more ticks than its estimate);
    - `tbm_depot_stalls` 55 (the depot files tags).
  - **Lesson.** Classify a failure with its tag run alone. In a combined run, test cases leak state, and the switch-off comparison first reported the travel tags as passing.
- **Captures (10.4).**
  - **Images.** `mlm/reviews/demo-fixes/acceptance/20261016-042113-389032f395/images/`, with hashes in `reports/acceptance-captures.json`. The pairing is in [R2DF2-B](r2df2-b-acceptance-notes.md).
  - **Added for screenshot 03.** `hauler-depot` now captures the loaded hauler once it has left its depot and gathers speed, with its vehicle window centred on it and news popups closed. The departure-tick capture still read 0 mph.
  - **Not taken.** `vehicle-spacing` no longer takes its moving capture, which waited for a loaded return. Its paused captures and the face-loop captures show the spacing.
- **Frame rate (10.5).** `framerate` scenario, real video driver, the player's `play.cfg`, normal speed:
  - 1,500 ticks: game loop 36.83 ticks per second, 99.4% of 37.04, with 0.10-0.12 ms game-loop times;
  - 8,000 ticks, through the first face wait and transfer: 36.95 (99.8%), 37.05 achieved over 216 s, game loop 0.10-0.11 ms, train ticks 0.05 ms.
  - Logs: `logs/demo-fixes/framerate/20261016-045025-3c09dc9152` and `.../20261016-045113-a8263d9266`. No profiling was needed.
- **Carry onward to Group 11.**
  - The guides should name the refreshed checkpoints, version 425, the face meeting, and the bay exchange near the portal.
  - The hauler's cargo line is cut short in the list's default width.

## Group 11 (2026-10-16)

- **Source of the wording.** Every quoted label and message in the guides was taken from `src/lang/english.txt` and the drawing code. For example, the wait line comes from `GetTBMWaitLine` (`layer_portal_gui.cpp`) and is drawn by both the portal panel and the Projects list, while the cargo lines are drawn by the Projects list only.
- **How-to (11.1, 11.4).** [the player how-to, now in d2um-underground-mines.md](d2um-underground-mines.md) was changed in these places:
  - the overview, and sections 3 and 6-9;
  - the troubleshooting table, with five new rows and two changed;
  - a new section, **Questions from the 2026-10-15 demo** (Q1 and Q2).
- **Stale wording found and fixed.**
  - The how-to called the area tool "Plan an excavation area"; the button is **New area**.
  - The demo guide said the build writes save version 418, and that the eight underground snapshots are 403. The build now writes 425, and four snapshots are 425.
  - The demo guide said New area on a portal with a project sends you back to Extend work; it now opens Change work area.
  - The face exchange design's example of the first breakpoints at the bay (Standard cut 7, Deep cut 4) is for the test chamber routes. On `tbm-work` every breakpoint (cuts 11, 25 and 35) already meets at the face, so the guides use those numbers.
- **Demo guide (11.2).** [d2um-underground-mines.md](d2um-underground-mines.md) was changed in these places:
  - the dated header, the layer keys, and a new **Keys: F2 and Escape** subsection;
  - menu row 11, and the version paragraph after the menu;
  - tour steps 2-3 and the Change work area paragraph;
  - the area steps: approach, blue cells and the quote line;
  - New area on a portal with a project, five troubleshooting rows (one of them new), two details, and the far-exit hauler refusal.
- **Architecture (11.3).**
  - **[archi-map-layers.md](../../architecture/map-layers/archi-map-layers.md), Paid boring and real spoil haulage.** It gains the face exchange, plus the R2DF handoff notes: task 6.5 (signals and admission), Group 7 (support, quotes, obstruction), Groups 8-10 (approaches) and Groups 11-12 (change of work, from the `r2df-b` contract, task 12.7).
  - **Other sections of that file.** The geology section points to these contracts. Four code-map rows and two invariants were added.
  - **Layer dropdowns and view state.** It already described small-map following, Alt+1..4, the Debug keys and the legend (R2DF Group 2), so it was left unchanged.
  - **[archi-gui.md](../../architecture/gui/archi-gui.md).** It already had the D10 section from R2DF Group 3. It gains the player-facing setting names, the client-only flags and a code-map row.
- **Status notes (11.5).**
  - The [master plan](r2mlvs3mp-master-plan.md#scope-and-authority) has a dated paragraph at the top of Scope and authority. No master checkbox changed.
  - [R2TBMF](r2tbmf-tbm-fixes.md) task 11.5 has a dated note and stays unchecked.
- **Checklist (11.6).** In [R2DF2-B](r2df2-b-acceptance-notes.md#review-checklist-task-116):
  - one row for each of the 12 DemoNotes requests: items 1 and 4 have three and two parts, item 6 two parts, plus Q1 and Q2;
  - each row gives the fix, how to try it on `tbm-work`, the evidence and an unticked review box;
  - an extra note covers R11, which came from the user's saves rather than DemoNotes.
- **Checks.** A link and anchor check over the seven edited documents found nothing broken. No source changed, so nothing was rebuilt or retested.
- **Carry onward.** User acceptance is pending: R2TBMF 11.5, master 12.6 and the master queue all wait for the user's review.
