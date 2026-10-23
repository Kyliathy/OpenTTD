> **Historical donor snapshot — not an execution plan.** OpenTTD3 `75ea75718f`, imported2026-10-23.
> Original: `docs/zz-reach2/upgrades/2026-10/r2tbmf-h-motion-notes.md`; exact source SHA256 `5755767b05fd8b3de7f3fe97ae8995528a1f3fb7059a585c24ac4d9a4d7d81b1`.
> Current integration: [M2O23-1](../../../../../../../zz-reach2/upgrades/2026-10/m2o23-1-come-space.md); storage and evidence roles: [archi-historical-debug.md](../../../../../../../zz-reach2/architecture/archi-historical-debug.md). Original completion claims, instructions and human-review limits below remain dated; they grant no new execution, spending, publication or deletion authority.

# R2TBMF H - Native cutter movement holds

Owner: [fixing Group4](r2tbmf-tbm-fixes.md#group-4---make-tbm-travel-match-the-accepted-route). Architecture: [vehicles](../../../../../../../../../OpenTTD3/docs/zz-reach2/architecture/vehicles/archi-vehicles.md), [rail pathfinder](../../../../../../../zz-reach2/architecture/pathfinder/archi-pathfinder-rail.md). Group3C is fully verified in [checkpoint G](r2tbmf-g-fresh-checkpoint.md). Group4 is now fully checked; the historical investigation below precedes the definitive closure section.

## Current source inspection

The original demo did not reproduce wandering outside its accepted path. Checkpoint B's **Group1B movement comparison / closure** reports44,508 attached-part observations on accepted support or actual portal housings. Its confusing cursor9..16 loop was accepted area coverage. Explicit straight intent, exact cutter endpoints and numbered area/tunnel paths now work, with actual Underground/Deep movement and file-reload evidence in F/G. Do not reread whole baseline logs merely to establish those facts again.

Current code paths:

- `GetTBMCutterSteeringTarget` in `tbm_support.cpp` derives exchange/approach/advance targets and the exact entry/exit trackdir on a revisited tile. Missing bay/job/path/piece returns no target. Ordinary trains and `OT_GOTO_DEPOT` service visits intentionally return no project target.
- **`IsTBMHoldingTrain` in `tbm_work.cpp` currently holds only `Bore` and `WaitForEmpties`.** It does not inspect player pause or missing references/target during travel. Conversely, it still holds those work/wait phases even with an actual depot order; reconcile legitimate service travel carefully.
- `ObserveTBMPhaseInputs` and `AdvanceTBMWork` in `tbm_project.cpp` derive the actual controller interruption once per front tick. `StepTBMProjectPhase` preserves player pause, missing references, stopped/breakdown/service state and face-work waits. Approach/advance arrival currently checks the head tile. No missing steering-target wait is derived there yet.
- Native `Train::Tick` invokes the project owner once before both `TrainLocoHandler` substeps. The latter checks the existing work hold before ordinary order processing. Later it replaces the train destination only when a project target exists; absent target leaves ordinary handling reachable.
- `CYapfDestinationRail::SetDestination` in `pathfinder/yapf/yapf_destrail.hpp` uses a present work target, otherwise hauler or ordinary order destinations. A missing project target and a legitimate service override currently share the same absent-optional representation.

The original source findings above are now followed by the physical reproductions and partial verification below. They describe the pre-fix code; use the current implementation section when resuming.

## Next implementation / proof

1. Reproduce actual paused travelling and missing/incompatible target behavior using a naturally driven TBM and owned native infrastructure. Keep source/job/cutter identities and all following parts; declared infrastructure fault injection may characterize recovery, but do not move the train or edit its phase to fabricate arrival.
2. Derive safe travel holds from existing project/target authority and expose a recoverable saved reason through the current controller. Distinguish service/ordinary orders from missing work targets. Do not add a private destination/route cache or unrelated order policy. Check both early native hold and later order/destination processing so a lost target cannot choose an arbitrary no-order/junction branch.
3. Check actual target rail ownership/type/piece compatibility and native failed-path fallback at reused junctions. Existing `IsTBMSupportUsable` is only a tile/owner/plain-track-or-portal check; it does not prove complete physical connectivity or all rail-type compatibility.
4. Reuse existing focused movement/corridor/coverage/turn/recovery fixtures after inspecting their assumptions. A staged `TBMWorkRig::Reload(Bore)` is only unit state setup; it is not driven movement evidence. Group4 still needs head/all-part paths, backing/bends/constrained turns and real approach/bend/wait reload comparisons.

Group4 finished after compaction count1. Its source investigation and intermediate failures below are historical. Continue at Group5 with its own local reading; later pacing, advice, parking/orders, exitB and geological-art groups remain pending.

## Physical reproduction and verified first fix

`logs/r2tbmf-h-reproduction.log` records **13 failed assertions** before production edits. New `[tbm-travel-hold]` cases in `tbm_movement.cpp` drive the actual complete TBM out of storage, pause during real travel, or temporarily change an unoccupied already-paid target's track bits, owner or rail technology. No vehicle position or phase is edited. The old code moved the consist while paused/unsafe, kept nonzero speed, and omitted the visible missing-support reason. Temporary native track faults restore through an RAII scope; completed/partial work and real cargo are retained.

After the first fix, `logs/r2tbmf-h-holds-isolated.log` passes **75,851 assertions/11 cases**: `[tbm-travel-hold]`, `[tbm-phase]`, `[tbm-movement]`, including existing real bends/backing and ordinary freight/portal guards. The first combined production/test build is `logs/r2tbmf-h-holds-build.log`, exit0.

Current core changes:

- `GetTBMTravelHoldReason` is a read-only derived query. Ordinary/finished trains and actual depot orders retain native travel; explicit project pause, lost references and absent work-travel targets produce precise holds. No destination cache or persistent field is added.
- `GetTBMCutterSteeringTarget` revalidates live references plus actual target owner, native rail type and laid piece. Work travel cannot treat incompatible/foreign/missing rail as a usable target.
- `IsTBMHoldingTrain` holds all actual parts during unsafe/paused work travel and permits a real depot order. `ObserveTBMPhaseInputs` supplies the current reason to the existing saved blocked field. The train handler rechecks after ordinary order cleanup, preventing an ended service override from exposing no-order fallback.
- A new **transient** `TBMPhaseInputs::service_finished` observes the previous service interruption and current real order. The phase owner rejoins the face/bay through travel, retaining partial work; even a halt-in-depot arrival must not resume cutting at the depot. No save-field layout changed.

## Service and junction fixes and verification

The new `[tbm-travel-service]` drives a real native depot visit from a partially worked face, in service-only and halt-then-start variants. It first exposed an assertion in `FollowTrainReservation`: the internal reservation walker carried company/rail type but lost the actual train needed for a private Commissioning housing. `pbs.cpp` now supplies that actual train to the follower for its own reservation; anonymous owner queries remain anonymous. The build passed, but the next service test then found the entire actual cutter captured in its source magazine instead of passing through to its real depot (`logs/r2tbmf-h-service-focused.log`). That is a real workflow failure, not a reason to weaken the test.

The verified fix lets a servicing cutter pass the source housing, or leave its existing stored chain through the outer door when a depot order starts there. While backing, it first collects the whole actual consist into the original shaft magazine; a partly deployed train must not mix shaft withdrawal with door departure. `GetLayerPortalMagazineExit` derives that behavior from the actual order, backing state and remaining depot-state parts. Source housing occupancy still excludes another moving train. `MayTBMTrainLeaveMagazine` permits this already-admitted actual depot trip; no fabricated depot ID or public portal release is introduced.

Also added a genuine failed-path guard: project travel must not take YAPF's guessed/first junction piece if the actual target is unreachable. `ChooseTrainTrack` returns Invalid for that work-path failure, and the physical caller holds before stepping onto the branch. Native stuck retries stay active, automatic signal turnaround is disabled only for an actual work target, and the second locomotive substep cannot bypass a failed reservation. The controller presents Obstruction while native retries continue. Work-target searches no longer treat a partial red-signal stop as arrival at the actual target; ordinary train policy is unchanged.

New `[tbm-travel-junction]` uses the **actual hauler exchange** in `SourceLoop(LoopStart::Command)`, then injects a junction and missing intermediate piece on vacated paid track while keeping the actual face target valid. It holds across a normal automatic-turnaround interval and physically recovers after repair, retaining cargo/work: **17,921 assertions** passed in `logs/r2tbmf-h-junction-focused.log`.

No live build remains. The final service correction built successfully in `logs/r2tbmf-h-magazine-build.log`. Real service-only and halt/restart visits pass **2,735 assertions** in `logs/r2tbmf-h-magazine-service.log`. The combined current `[tbm-travel-hold],[tbm-travel-service],[tbm-travel-junction],[tbm-phase],[tbm-movement],[tbm-loop]` burst passes **347,677 assertions/17 cases** in `logs/r2tbmf-h-motion-combined.log`. Group4.3's constrained turns/reused branches and4.4's actual approach/bend/wait file replay remain to finish. Existing `tbm_coverage.cpp` U-turn/turn tests stage geological work; they are not physical driving proof.

## Group4 closure

All four Group4 tasks are verified on2026-10-09. The final service correction records the physical rejoin phase when a depot order takes control, before a subsequent project pause can replace its displayed Servicing reason. Empty and loaded partial faces pass service-and-continue, halt/restart and pause-during-service variants without losing work or packets.

`logs/r2tbmf-h-motion-current.log` passes **599,484 assertions/18 cases** across current holds, real depot visits, failed-path junction recovery, actual constrained U-turn, phase, ordinary freight/portal movement and genuine hauler loops. `logs/r2tbmf-h-turn-coverage.log` additionally passes **259,548 assertions/7 cases**, linking the existing too-small turn refusal to an actual advancing half-circle: every attached part stays on the paid swept floor/support, changes heading by at most45 degrees while moving, backs without cutting, and delivers all spoil through actual hauler/receivers. Every permanent part is individually observed negotiating the turn. No vehicle pose or exchange cargo is edited to achieve it.

The new `src/tests/tbm_motion_reload.cpp` and [runner I](../../../../../../../../src/tests/fixtures/cleanup/source-epochs/25a342fd307adaa4-r2tbmf-i-native-motion.py) use the actual startup loader and normal game loop. A test-only window observes the native post-vehicle/landscape callback; a `TimerGameTick` callback would run too early to save a complete simulation step. The baseline runs20,000 frames on the user-guide's retained renamed demo, audits every actual cutter/wagon against the canonical paid path, and completes its20-cut area route with real haulage, including reused multi-branch support. It passes **125,645 assertions** in `logs/r2tbmf-h-motion-native.log`.

| Actual checkpoint | Baseline frame / cut cursor | Exact replay frames through completion | Retained save |
| --- | --- | --- | --- |
| Approaching its face | 305 /2 | 19,696 | `src/tests/fixtures/haulage/tbm-motion-approach-393.sav` |
| Negotiating the bend | 474 /2 | 19,527 | `src/tests/fixtures/haulage/tbm-motion-bend-393.sav` |
| Waiting for a real wagon exchange | 2,832 /6 | 17,169 | `src/tests/fixtures/haulage/tbm-motion-wait-393.sav` |

Each replay matches **every native frame**, starting with the loader-reconstructed state: route cursor/heading/partial work, all train IDs/chain associations/coordinates/directions/speed/progress, cargo packet IDs/counts/age, full accepted-route reservations, geology/project/exchange/receiver digests, company cash and synchronized RNG. Each continuation makes the next actual cut and finishes. Logs: `r2tbmf-h-motion-approach-reload.log`, `r2tbmf-h-motion-bend-reload.log`, `r2tbmf-h-motion-wait-reload.log` under `logs/`. The baseline already has two completed cuts; the approach is a real later journey, not a manufactured cursor-zero fixture.

Three repeated public preview/projection queries and three repeated native viewport draws at each snapshot leave the exact simulation state unchanged. Actual viewed captures: [approach](../../../../../../../../../OpenTTD3/docs/zz-reach2/upgrades/2026-10/assets/reviews/tbm-motion-approach.png), [bend](../../../../../../../../../OpenTTD3/docs/zz-reach2/upgrades/2026-10/assets/reviews/tbm-motion-bend.png), [wait](../../../../../../../../../OpenTTD3/docs/zz-reach2/upgrades/2026-10/assets/reviews/tbm-motion-wait.png). The two Surface views show the real source magazine; the bend view shows the physically deployed cutter in Underground. These are rendered game pixels, not diagrammatic positions.

Final optimized game/test builds passed; focused tests take under two seconds, each native journey under four seconds. `git diff --check` passes. No full regression was run. Executable/content/config provenance retains the existing Group1 runner setup and protects original390, renamed393, `play.cfg`, `native-art.cfg` and exact startup-script bytes on every run.

SHA256 at closure:

- Game: `e1f05f1bb689c9e7e41b7298d30bce7c1aa4f5b4ca4d7e3a80721e7a79411098`.
- Test executable: `593f668c2640710de41f80cfeb7ee1589f1d0077e86eea995528783ae0d7b9fe`.
- Approach save: `41fb82e56d8d802ad593ee44b4fb400d058ef861ff9c5eeaa5951287814809f9`.
- Bend save: `a5046689cfd1b398c69fd26d12d793a61131a4ecddd7f3fa3d30249f6817babd`.
- Wait save: `6ba9a973215928367f407fbf1a2642bdc60c45a5a203b18414bf72e177e2342c`.

Next: Group5's rate/yield/fraction/save migration. Rates and receipts above remain **pre-halving**. Advice, player-facing return/park/release, alternate hangars, exitB and geological art are still pending. No Git write or commit occurred during Group4.
