> **Historical donor snapshot — not an execution plan.** OpenTTD3 `75ea75718f`, imported2026-10-23.
> Original: `docs/zz-reach2/upgrades/2026-10/r2mlvs3mp-z-s-rebaseline-notes.md`; exact source SHA256 `0acd392eae8dbef95a9cce784ecac8963c6b30d80d0b856e68062abfe9feabb4`.
> Current integration: [M2O23-1](../../../../../../../zz-reach2/upgrades/2026-10/m2o23-1-come-space.md); storage and evidence roles: [archi-historical-debug.md](../../../../../../../zz-reach2/architecture/archi-historical-debug.md). Original completion claims, instructions and human-review limits below remain dated; they grant no new execution, spending, publication or deletion authority.

# R2MLVS3MP-Z-S - Re-baseline notes, 2026-10-16

Companion of the [R2MLVS3MP master](r2mlvs3mp-master-plan.md), [Group 10R](r2mlvs3mp-master-plan.md#group-10r---re-baseline-the-underground-after-the-demo-fixes). These are working notes: what was found, what is stale, and what later groups must know.

## Carried into Group 10R

- **Inputs.** The failure list is the R2DF2 Group 10 burst (`logs/demo-fixes/g10-burst-rel.json`, RelWithDebInfo, 160 tags in 14 chunks).
- **Runner.** `src/tests/multilayer/tbm/focused_tags.py` runs each tag in its own process and writes `logs/underground-rebaseline/<scenario>/<run-id>/focused-tags.json`.
- **Binaries.** At the start, `openttd_test.exe` (RelWithDebInfo) dated from 2026-10-16 06:38, and no source file was newer.
- **Tag corrections.** `tbm_commission` 357 is the shared static `MeasureFirstCut`, so both `[tbm-measure]` and `[portal-script-commission]` reach it; R2DF2-C's `[tbm-measure]` is right. `underground_depot_fixture.h` 269 (now 287) is the shared `DriveUntil` helper used by the depot tags.
- **Runner use.** `focused_tags.py` accepts tags with or without brackets, and fails a tag that matches no case (Catch exits 0 for that).

## 10R.1 classification

Run alone on the current build (`logs/underground-rebaseline/focused-failures/`), every listed case falls in one of four causes.

| Cause | Cases | Disposition |
| --- | --- | --- |
| Leaks only | `[portal-script-commission]` | Passes alone; no change. |
| Halved yields (R2TBMF 5B.1, user decision 2026-10-09: Soft 6/12 to 3/6 t per cell) | `[excavation-outline-authorization]`, `[tbm-acquisition]`, `[tbm-sealed-start-gate]`, `[tbm-movement]`, `[tbm-loop]` 774/843, `[tbm-travel-junction]`, `[tbm-exchange-acceptance]` 77/133/187, `[tbm-coverage]`, `[spoil-receiver-delivery]`, `[tbm-economics]` | Stale. Capacity and overflow cases declare Mixed rock (12 t Rock per cell, the former Soft Rock total) through `TBMRigRock` / `ExcavationFixture::DeclareRockSeam`, so routes keep their geometry. Receiver cases derive intake from the actual cut and 8 t batches. `[tbm-economics]` moves to 10R.3. |
| Half-speed cutting (R2TBMF Group 5) | `[spoil-receiver-stall]` 76 | Stale: one tick may bank no work; the case now runs two. |
| Order and hangar contract (R2TBMF Group 8) | `[tbm-project]`, `[tbm-travel-service]` | Stale. The first travel order hands the cutter to FollowOrders; Resume hands it back through the bay; a halt parks the empty machine until an explicit Resume. Loaded halts need a hauler and are covered by `[tbm-hangar-acceptance]`. |
| Support in use under a train (R2TBMF) | `[tbm-corridor]` 113 | Stale: a train on released support keeps it in use. |
| Quotes refuse unfunded pieces (R2DF Group 7, task 7.3) | `[tbm-hangar-reuse]` 419 | Stale: fund the route's missing pieces before quoting, as the depot fixture already does. |
| Pacing in a measurement | `[tbm-measure]` / `tbm_commission` 357 | Stale: the first cut takes `estimate.work_ticks` at the layer's pacing (Standard 1/2, Deep 3/4), so `MeasureFirstCut` bounds and checks it with that rate. |
| Fewer breakpoints | `[tbm-economics]` 45 | Stale: the Standard route freezes one breakpoint, Deep two; the case derives its count from `pass_terms->exchange_after` (10R.3). |
| Product defects | `[tbm-logistics-queue-save]` 72; `[tbm-reserved-loop]` (new); the depot face meeting | Fixed in 10R.2; see below. |
| Routed | the seven depot-loop tags | [Plan 5 Group 13D](r2mlvs5-spoil-wagons-and-haulage.md#group-13d---re-baseline-the-recurring-depot-loop-after-the-halved-yields); see below. |

## Finding: face meetings with an underground depot bay

R2DF recorded `[tbm-depot-cycle]` failing late (the finished cutter lost on the Surface). Since R2DF2 it fails at the first exchange: the cutter waits in `AwaitFaceTransfer` (phase 20) at cursor 19 while the hauler stays hidden in the underground depot bay for 50,000 ticks. R2DF2-C filed it as pre-existing because the same line fails; the cause is new. The face-exchange design (R2DF2-A) covers the Source magazine only.

Resolution: `GetTBMMeetingKind` (`src/tbm_support.cpp`) now returns Bay for a project whose exchange bay is not a Source bay. The hauler leaves an underground depot by the native depot rule, which keeps it inside while another train occupies the depot's signal block. The waiting cutter stands in that same unsignalled chamber block, so each waited for the other. Depot-bay projects meet at that nearby depot again, as before R2DF2. This narrows R2DF2 decision D9 (the face serves every project whose stand fits) to Source-bay projects.

## 10R.2 fixes

Each was confirmed by a focused failing test before the fix.

- **Magazine departure** (`CheckTrainStayInMagazine`, `src/train_cmd.cpp`). A train leaving through the door now waits while the block in front of the door holds another train, as a train leaving a depot does (`UpdateSignalsOnSegment` Full). The test applies whatever the reservation setting, because a magazine has no depot path reservation. Trains stored in the magazine do not count, and an exit through the shaft towards the work never enters that block. `[tbm-logistics-queue-save]` passes: its second hauler is started once the first is admitted at the bay, and the declared FIFO turn is the first hauler's.
- **Cursor-0 deadlock with `reserve_paths`** (`ObserveTBMHaulerInputs`, `src/tbm_exchange.cpp`). An empty hauler bound for its Source bay finds no path while the bay's housing is busy, because `GetLayerPortalAdmittedTrackdirs` hides a busy magazine. During cut 0 the busy train is the project's own cutter. Reporting BlockedLine raised LogisticsStall and held the cut that frees the housing. `PathfinderLost` is now not read as lost in that case, and the pathfinder clears it once the housing is free. New test: `[tbm-reserved-loop]` (`src/tests/tbm_reserved_loop.cpp`), with reservation on from the first tick: no stall in cut 0, and the first breakpoint is served at the face.
- **Stale lost flag in a magazine or depot** (same owner). A hauler standing wholly inside a magazine or depot has no route to be lost on, so its old `PathfinderLost` flag is ignored. A magazine exit also resets the flag (`CheckTrainStayInMagazine`). Read stale, the flag made a face turn holder in its bay alternate between BlockedLine and waiting every tick.
- **Test fixture.** `UndergroundDepotWorkflow::Do` flushes the signal queue after each direct command, as the command kernel does; otherwise the next depot departure met a non-empty queue (`_globset.IsEmpty()` assertion).

## 10R.3 ledger

Re-measured on the RelWithDebInfo build: Standard 9,620 ticks / 2,539,452 with one exchange; Deep 19,335 / 3,066,086 with two. All categories balance and spoil income is zero. The current table is in the [complete ledger](r2mlvs4-t-complete-project-ledger.md#current-ledger-re-measured-2026-10-16). The S1 and S2 schema2 invoices are under `mlm/reviews/acceptance/native-ground/` (runs `20261016-093620-04c966649c` and `20261016-093631-fdc10fd9d1`). The four fee, no-profit and saturation tags pass alone (53,866 assertions / nine cases). 11.5 stays checked.

## 10R.4 art finalize

The vent collector (Plan 14 79C) joined the packed payload on 2026-10-10, but only `activate_m1_art.py --collector-native` bound it. The generic path asked the selection for `collector-checks` and `collector-capture`, and the gallery still pinned 42 families / 2,343 sprites. Changes:

- `regenerate_art_proofs.py`: `focused` also runs `[collector-art-selection]`, `[geothermal-collector]` and `[geothermal-heat-source]` and records `collector-checks`. A new `collector` capture runs `collector_runtime_checks.py native`.
- `review_m1_art.py`: the scope is 43 families / 3,495 sprites, with the collector's native frames as its scenes.
- `native_ground.py`: its art scope label reads `selected_M1_43_families`.

The cold run `20261016-093503-c44780389e` completed: initialize, focused (25,108 assertions / 31 cases), seven captures, then finalize (sound, activation 43 / 3,495 with three corruption refusals, gallery of 179 frames). Phase log: `logs/underground-rebaseline/art-finalize/20261016-cold-01/phases.log`. The previous selection and outputs are kept in `logs/proofs/mlm/native-art/backup-before-10r4/`. The public sprite contract is unchanged (`--cache-only`).

## Routed: the recurring depot family

Run alone ([report] (historical reference: `../../../../logs/underground-rebaseline/depot-family/20261016-113926-4ab1ec1479/focused-tags.json`)), `[tbm-depot-foundation]` and `[tbm-depot-recommission]` pass. Seven tags fail: cycle, capacity-files, crash-file, infrastructure-files, queue-file, replay and save-file. Two causes overlap:

- **Stale.** The further job freezes one breakpoint and completes at its first depot exchange (cursor 19, phase 7, one exchange). Checkpoints after that exchange never come. Examples: `[tbm-depot-cycle]`'s second exchange, and the Pause after the crash in `[tbm-depot-crash-file]`, refused with `STR_ERROR_TBM_PROJECT_CANT_PAUSE` because the project has finished.
- **Defect.** After completion, the hauler stays hidden in its Surface depot in DisposeOrTransfer (wait None) for 50,000 ticks, while the finished cutter runs lost on the line. This is R2DF's earlier end state.

This spans the test fixture, the hauler's Surface exit and the finished cutter's route, so it is not bounded to one owner: [Plan 5 Group 13D](r2mlvs5-spoil-wagons-and-haulage.md#group-13d---re-baseline-the-recurring-depot-loop-after-the-halved-yields). The parent index row for Plan 5 is reopened until 13D closes.

## Resolved finding: Surface depot exit with reservation on

Resolved 2026-10-16 in Plan 5 task 13D.2: the bay admitted a loaded hauler, so path reservation found no safe waiting position between the origin platform and the cutter. `IsTBMHaulerAdmittedAtBay` now admits only a hauler bound for or waiting in its bay, or one with a part on the key. `[tbm-reserved-loop]` runs to delivery with reservation on. The original finding follows.

In `[tbm-reserved-loop]`, after the first face transfer, the loaded hauler ends its origin visit in the Surface depot at the end of the line (tile 1044, DisposeOrTransfer, spoil aboard) and stays there. The case therefore stops at the first face transfer. It looks like the depot end state above; Plan 5 task 13D.2 owns both.
