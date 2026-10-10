# R2MLVS3MP-D - Execution notes

Owner: [R2MLVS3MP](../r2mlvs3mp-master-plan.md). Targets: [Plan 3](../r2mlvs3-sealed-portals-and-deep-rail.md), [Plan 4](../r2mlvs4-tunnel-boring-machines.md). Shared decisions: parent AD-3..AD-6. These notes hold evidence and continuation context for the master queue; child evidence keeps the child's own prefix.

**Artifact-layout migration (2026-10-09):** this plan consumes the parent [Development artifact layout](../r2mlvs-multilayer-vehicles-and-structures.md#development-artifact-layout), implemented by [R2FC](../r2fc-file-cleanup.md). It supersedes earlier docs asset/review/helper placement and fixed runtime-config destinations below. New native homes use `mlm/runtime/`, generated feature reviews use `mlm/reviews/`, logs remain under `logs/`, and reusable code/curated inputs keep their code/test owners. Paths and focused native/cold verification are complete; R2FC records the results and retained-input identities. Repository-relative executable commands run from the checkout root through their context-aware wrappers. Historical pass records remain dated evidence and are not new acceptance claims. No gameplay task is completed by this migration and the separate post-demo fixture-pruning gate remains.

## Reading already done (do not reread unless the task needs it)

- Parent AD-3..AD-6, I-1..I-17, Plan Index staging paragraph and Execution conventions.
- Plan 3 and Plan 4 Global Mandatory Reading tables and Public Handoff Contracts; Plan 3 Groups 1-12 plus its execution notes; Plan 4 Groups 1-5 plus its execution notes (verified sealed-start refusal).
- Plan 15 Groups 7-15 area and Plan 16 Groups 1-5 area (selected-family gate).
- Stale/irrelevant for later groups: the Plan 3/4 "Continuation" paragraphs that say work is uncommitted. HEAD `5f40e4d755` now holds every Plan 2/3/4 change made so far.

## Build and test commands in use

- CMake: `/c/Program Files (x86)/Microsoft Visual Studio/2022/BuildTools/Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin/cmake.exe`.
- Debug test build: `cmake --build build --config Debug --target openttd_test --parallel -- /nodeReuse:false` (about one minute incremental).
- Focused tests: from `build/`, `./Debug/openttd_test.exe "<tag>,<tag>"`. Catch2 does not expand `[prefix-*]`; build the tag list from `--list-tags`.

## Build cadence (user instruction, 2026-10-05)

Do not build or run tests after each change. Compiling slows the user's machine. Build and run one focused burst only at a milestone, at most once every three or four master groups. The end-of-Group-4 milestone ran on 2026-10-05 (see [the milestone section](#milestone-build-and-burst-end-of-master-group-4-2026-10-05)). The end-of-Group-7 milestone ran the same day (see [its section](#milestone-build-and-burst-end-of-master-group-7-2026-10-05)). Code from master Group 7B on is unbuilt until the next milestone. For Group 7B the user allowed 2-3 build-and-test runs (2026-10-05), because movement needs runtime feedback; spend them at checkpoints, not after each change. The user also set the laptop to maximum performance. Read real signatures before using them, because compile errors surface late.

Every test run must be wrapped in `timeout`. Since the end-of-Group-7 milestone, `src/tests/test_main.cpp` (MSVC) prints a failed assert with its call stack and exits with code 3, instead of the Debug runtime's modal Abort/Retry/Ignore dialog. Run tags one at a time with the runner described in the milestone section.

Changes built at the Group 4 milestone (kept as a map of what Groups 2-4 touched):

- Geology: `excavation.h` (starter declarations; defaulted `operator==` on `ExcavationCutGeometry`, `ExcavationPassTerms`, `ExcavationJob`), `excavation_starter.cpp` (new), `excavation_pass.cpp`, `excavation.cpp` (`ReserveExcavationJob`), `excavation_cut.cpp`, `excavation_route.cpp`.
- Portal: `layer_portal_cmd.h` proof/issuer declarations, `layer_portal_bootstrap.cpp` (new), `layer_portal_work.cpp` (shared pocket helper, load binding).
- Trains/TBM: `train_cmd.cpp` (`MoveRailVehicleParts` factored out of `CmdMoveRailVehicle`; `BuildTBMConsistAtSite`), `train_cmd.h`, `tbm.h` (acquisition API, role contract; now includes `layer_portal_cmd.h` and `core/random_func.hpp`), `tbm_acquisition.cpp` (new), `vehicle_gui.cpp` (`StopGlobalFollowVehicle` tolerates no main window).
- Tests: `excavation_starter.cpp`, `portal_bootstrap.cpp`, `solid_starter_fixture.h`, `tbm_fixture.h`, `tbm_acquisition.cpp`, `tbm_project.cpp` (all new); `tbm_engine.cpp` (fixture moved out, sealed-start test rewritten).
- Group 3 (project store): `tbm.h` (store API, `operator==` on `TBMProject`), `tbm_project.cpp` and `tbm_cmd.h` (new), `command_type.h` (appended `ControlTBMProject`), `command.cpp` and `network/network_command.cpp` (include), `saveload/saveload.h` (version 376), `saveload/tbm_sl.cpp` (new), `saveload/saveload.cpp` (chunk table, preview), `saveload/afterload.cpp` (accept after vehicles), `saveload/multilayer_sl.cpp` (old-version flag), `multilayer_state.h/.cpp` (feature bit 32, dependency, capture, digest record), `fios.h`/`fios_gui.cpp` (preview fields), `misc.cpp` (reset).
- Group 4 (phases, preview): `excavation.h`/`excavation_cut.cpp` (`GetExcavationCellHardness`, shared by quote and cut), `tbm_area.cpp` (new), `tbm_acquisition.cpp` (`GetTBMConsistDimensions`), `tbm_project.cpp` (references, phase step), `tbm.h`, `saveload/tbm_sl.cpp` (blocked bound uses `End`). Tests: `tbm_area.cpp`, `tbm_phase.cpp` (new); `tbm_fixture.h` now also holds `TBMAcquisitionRig` and `TBMProjectRig`.
- Group 5 (boundary): `tbm.h` (boundary block, `GetTBMWorkState`, `MeasureTBMConsist`, `ValidateTBMAreaRoute`, `AdvanceTBMWork`), `tbm_project.cpp`, `tbm_acquisition.cpp`, `tbm_area.cpp`, `train_cmd.cpp` (`Train::Tick` call site). Test: `tbm_consumer.cpp` (new).
- Plan 3 5.3B: `layer_portal_work.cpp` (`CheckLayerPortalWorkAccess` consumes TBM roles), `layer_portal_cmd.h` (doc). Test: `portal_tbm_access.cpp` (new).
- Plan 4 Groups 6-8 (face work, cut participant, support):
  - New sources: `tbm_work.cpp`, `tbm_support.cpp`.
  - Changed: `tbm.h` (declarations; includes `track_type.h`/`rail_type.h`), `tbm_project.cpp` (work wait hold, face facts, face step in `AdvanceTBMWork`, support in reset/load/digest; a completed job stays live for the final return), `excavation.h`/`excavation_cut.cpp` (`EstimateExcavationCut`), `train_cmd.cpp` (`IsTBMHoldingTrain` hold in `TrainLocoHandler`), `saveload/tbm_sl.cpp` (16-byte `TBMH`, new `TBMS` table), `fios.h`/`fios_gui.cpp` (support preview counts).
  - Tests: `tbm_work.cpp`, `tbm_support.cpp` (new); `tbm_fixture.h` (`TBMProjectRig(segments)`, `TBMWorkRig`); comments in `tbm_phase.cpp`, `tbm_consumer.cpp`.
- Plan 4 Group 9 (support rules): `tbm.h`, `tbm_support.cpp` (rules and `CmdConvertTBMSupport`), `tbm_cmd.h` (trait), `command_type.h` (appended `ConvertTBMSupport`), `rail_cmd.cpp` (hooks in build/remove/signal/convert/owner change; includes `tbm.h`), `train_cmd.cpp` (support speed limit in `GetCurrentMaxSpeed`), `lang/english.txt` (two strings). Tests: two new cases in `tbm_support.cpp`.
- Build lists: `src/CMakeLists.txt`, `src/saveload/CMakeLists.txt`, `src/tests/CMakeLists.txt`.

Milestone burst (end of master Group 4): `[portal-bootstrap]`, `[excavation-starter]`, `[tbm-sealed-start-gate]`, `[tbm-acquisition]`, `[tbm-project]`, `[tbm-area]`, `[tbm-phase]`, `[tbm-consumer]`, `[portal-tbm-access]`, `[tbm-work]`, `[tbm-support]`, `[tbm-route]`, `[tbm-backing]`, `[tbm-coverage]`, `[tbm-cost]`, `[tbm-corridor]`, `[tbm-reuse]`, `[tbm-site]`, `[tbm-engine]`, `[tbm-purchase-gate]`, plus every existing `excavation-*`, `portal-*` and `tbm-*` tag taken from `--list-tags` (the starter changed shared planner/pass/reserve/cut code). Also run any test that drives `CmdMoveRailVehicle` or autoreplace, because the move body was factored.

Stale since the milestone: the compile and runtime risk lists below were all resolved by the milestone build and burst. Keep them only as history.

Check these first when the build runs; they are the least certain compile points:

- Defaulted `operator==` on `ExcavationJob` needs `==` on every member, including `std::optional<ExcavationPassTerms>` and the strong ID types.
- `TBMAcquisitionPlan` equality includes `Money` and `LayerPortalBootstrapProof`.
- `std::span<const Engine *const>` is built from `std::vector`/`std::array` arguments.
- The `AcquisitionDigest` aggregate initialisation in `tests/tbm_acquisition.cpp`.
- Group statistics bookkeeping balance in `BuildTBMConsistAtSite`: `CountEngine`/`CountVehicle` on build must match `PreDestructor` on delete.
- Groups 6-8:
  - `TBMSupportClasses` uses `EnumBitSet` with an explicit end value, and the defaulted `operator==` on `TBMSupportTile` needs `TrackBits` equality.
  - The `TrackBits{uint8_t}` and `TBMSupportClasses{uint8_t}` storage constructors in `tbm_sl.cpp` must not pick the initializer-list constructor.
  - `CargoPacket::Create(StationID, uint16_t, Source)` forwarding.
  - The multiple inheritance of `TBMCutParticipant`, passed as both participant interfaces.
  - In `tbm_work.cpp`: `GetWindowClassForVehicleType` (from `vehicle_gui.h`) and `Train::MarkDirty` after the cargo append.
- Groups 6-8 runtime risks to check first:
  - `estimate.new_cells == 12` in `[tbm-work]` assumes no deposit in the pocket.
  - The side cell of a corner advance must lie in that cut's sweep (`[tbm-support]` turning case).
  - `AcceptLoadedTBMProjects` now also clears and accepts support, so any test that reloads after a cut must restage the support (`TBMWorkRig::Reload` does).

## Milestone build and burst (end of master Group 4, 2026-10-05)

- Builds: Debug `openttd_test` (g build (`r2mlvs3mp-g-milestone-build.log`; historical file unavailable), then rebuild 2 (`r2mlvs3mp-g-milestone-build2.log`; historical file unavailable) and rebuild 3 (`r2mlvs3mp-g-milestone-build3.log`; historical file unavailable) after the fixes below); no warnings. Debug `openttd` for the save fixtures (game build (`r2mlvs3mp-g-milestone-game-build.log`; historical file unavailable)).
- Compile fixes: `command_func.h` in `tbm_project.cpp`/`tbm_support.cpp` (`CMD_ERROR`); `table/strings.h` in the support/reuse tests; `Money(...)` around `int64_t` comparisons in `tbm_cost.cpp`; `TrackBits{track}` (there is no `TrackToTrackBits` in this fork).
- Runner: each tag runs as `timeout 100 ./Debug/openttd_test.exe "[tag]"`, one line per tag in the results file. The script lives in the user's Temp folder, not in the repository. Catch2 test names that contain commas need `\,` when run by name.
- Code defects found and fixed:
  - `BuildTBMConsistAtSite` (`train_cmd.cpp`): `AddArticulatedParts` re-arms `Vehicle::CanAllocateItem()` one part at a time, which spent the whole-consist arming. The cutter's native articulated part left the count at 0, so the first hopper's `Train::Create()` asserted (`pool_func.hpp:119`). Each hopper now re-arms its own demand.
  - `GetLiveTBMJob` (`tbm_project.cpp`): a portal that is not a canonical pair key no longer makes the job look foreign; the validator now reports `MissingPortal` instead of `MissingJob`.
  - `ReverseTrainDirection` (`train_cmd.cpp`): a whole train on a portal assembly site is treated like a train wholly inside a depot. The housing has no track, so the reservation follower asserted (`follow_track.hpp:144`).
- Test defects found and fixed (the tests had never been built):
  - `tbm_area.cpp`: the first solid-start cut opens corridor cells 1 and 2 together, so a three-cell corridor is 2 cuts, not 3.
  - `portal_bootstrap.cpp`: the door cell touches the housing, so it is inside the pocket; the check now proves the row beyond it is outside. `SolidStarterFixture` now calls `ResetRoadTypes()`. The [contract](r2mlvs3mp-f-solid-starter-contract.md) wording that suggested D' lies outside P was corrected.
  - `tbm_acquisition.cpp`: `Release()` empties the holder's project, so the test compares with the cutter's vehicle ID.
  - `tbm_work.cpp`: the obstruction case laid the project's own support, which is reused by design; it now lays foreign unregistered track.
  - `tbm_coverage.cpp`: outlines are 8-connected, so the bend floor now runs to along 5. The tight-turn case now needs a cell behind the housing, which only a 180-degree change could reach in a three-wide corridor (the old floor was covered by the cutter body without turning).
- Results: first per-tag pass burst 1 (`r2mlvs3mp-g-burst1-tbm-results.txt`; historical file unavailable), then burst 2 (`r2mlvs3mp-g-burst2-tbm-results.txt`; historical file unavailable) and burst 3 (`r2mlvs3mp-g-burst3-fixed-results.txt`; historical file unavailable) after fixes. Final: all 86 project tags (`r2mlvs3mp-g-burst4-all-results.txt`; historical file unavailable) (every `excavation-*`, `geology-*`, `geothermal-*`, `multilayer-*`, `portal-*`, `solar-*` and `tbm-*` tag) pass: 145 cases, 91,436 assertions.
- Save fixtures: all seven `src/tests/*_save_fixture.py` pass at version 376 (results (`r2mlvs3mp-g-save-fixture-results.txt`; historical file unavailable)). They rewrite retained evidence under the owning child's prefix. `geology_save_fixture.py` in full mode also overwrites `../../../../src/tests/fixtures/geology/r2mlvs2-g-native.sav`, which is the geothermal fixture's geology-only predecessor. Run it with `--refusals-only`, then restore every rewritten tracked file with `git cat-file --filters HEAD:<path>` (to a temporary name, then move it into place; a direct redirect truncates first). Record outcomes only in the master's `r2mlvs3mp-g-` files.
- Coverage gaps: no unit test drives `CmdMoveRailVehicle` or autoreplace, and no existing unit test covers ordinary rail, road, waypoint, station or path-finding commands. Those hooks are covered only through the TBM support, corridor and reuse tags. The untagged upstream tests were not run (full regression is rare by instruction).

## Group 1 - Verify reusable foundations

- 1.1: Plan 1 Groups 1-23, Plan 2 Groups 1-14 and Plan 15 Groups 1-6 are all checked in their plans. Exports verified by `grep` (table in [r2mlvs3mp-e](r2mlvs3mp-e-scoped-gates.md#producer-exports-at-this-snapshot)). Plan 1/15 burst: 18 cases/715 assertions (b (`r2mlvs3mp-b-foundation-plan1-14-results.txt`; historical file unavailable)).
- 1.1 finding: the first Plan 2 burst failed 1 case/3 assertions. `src/tests/excavation_guard.cpp` still expected `CmdBuildLayerPortal` into a solid door to refuse with `STR_ERROR_EXCAVATION_SOLID_ROCK`. Plan 3 Group 4/5 deliberately changed that: `PrepareNativePortalBuild` accepts a solid Clear door as a Sealed face. The test was not part of the Plan 3/4 bursts, so the stale expectation went unnoticed. The assertion now checks a pure quote that succeeds, allocates no pair and leaves packed geology and tiles unchanged. After the fix, Plan 2 burst: 44 cases/5,884 assertions (b (`r2mlvs3mp-b-foundation-plan2-results.txt`; historical file unavailable)).
- 1.2: Plan 3 Groups 1-7 (except the correctly pending 5.3B), 10 and 14 are checked; Plan 4 Group 1 and prefixes 2.1A/2.1B/2.2 are checked. Portal/TBM burst: 28 cases/1,274 assertions (b (`r2mlvs3mp-b-foundation-plan3-4-results.txt`; historical file unavailable)). `[tbm-sealed-start-gate]` still proves the refusal; it is not a success.
- 1.3: Plan 13 Groups 1-8 checked; `python src/tests/multilayer_sprite_contract.py` validates 141 families/58,059 required sprites read-only (c (`r2mlvs3mp-c-sprite-contract-results.txt`; historical file unavailable)). Templates are inputs, not runtime art.
- 1.4: [r2mlvs3mp-e](r2mlvs3mp-e-scoped-gates.md) records every scoped portion and gate. Child plans now point to it: Plan 4 adds 4.2A/4.2B; Plan 15 and Plan 16 name `-G`/`-T`/`-O` portions.
- Build: Debug `openttd_test` incremental build passes (a (`r2mlvs3mp-a-baseline-build.log`; historical file unavailable)). No full regression was run.

## Group 2 - Reconcile the solid starter (verified)

- Contract: [r2mlvs3mp-f](r2mlvs3mp-f-solid-starter-contract.md). Plan 3 8B.1 and Plan 2 14B.1 are ticked (contract freeze). Every other task in Plan 3 Group 8B, Plan 2 Group 14B and Plan 4 Group 2B stays unticked until the milestone burst passes. Their plans now carry an "R2MLVS3MP status" paragraph and new Public Handoff Contract entries.
- Plan 4 Group 2B design decisions:
  - Preparation is pure.
  - Acquisition builds stopped, hidden parts on the source site and holds them in a `TBMAcquisition` owner. It charges nothing and reserves no job.
  - Publication (pay, reserve while Sealed, publish Commissioning) is master 6.6.
  - The site is validated once, while empty. `ValidateLayerPortalCommissioning` refuses any vehicle on the source, so per-part revalidation cannot work after the cutter exists.
- Planner history size for a consist: ceil(length / 16) + 1, clamped to at least 3; above 16 refuses. Cutter plus two Earth and two Rock hoppers = 48 units = 4 points. The first segment is identical for any history of 3 or more points (all points start at the housing). Differences appear from cut index 3 on a straight route.
- Unit tests start with `max_trains` and `max_train_length` at zero; the acquisition rig sets 500 and 7.
- Supersession notes were added under Plan 4 Group 20 and Plan 16 Group 16: cut 0 charges 12 cells, not the 3 of a cleared area.

## Group 4 - Geometry, accounting and protected retreat (verified)

- 4.1 (Plan 4 Group 9) implemented; see the Plan 4 Group 9 status paragraph. The milestone must also run existing rail tests that build, remove, signal or convert track, because those commands now consult the support registry.
- 4.2 (Plan 4 Group 10) implemented: `TBMBlockedReason::IllegalTurn` (after `Paused`), the heading set on commit, the segment check and the vehicle envelope check in `tbm_work.cpp`. Test: `tbm_route.cpp` (`[tbm-route]`).
- 4.3 (Plan 4 Group 11) written as tests in `tbm_coverage.cpp` (`[tbm-coverage]`). Support now reuses or extends the project's own track (junction pieces). Fixture: `TBMProjectRig`/`TBMWorkRig` take relative turns or a proposal callback, and `TBMWorkRig::BoreOneCut`/`ExchangeAndReturn` are shared.
- 4.4 (Plan 4 Group 12): `ComputeTBMRecurringCharge` and `ChargeTBMProjectCosts` (`tbm_project.cpp`), the charge in `Train::OnNewEconomyDay`, the cutter skipped in `Train::GetRunningCost`, and the bankruptcy wait in `tbm_work.cpp`. Test: `tbm_cost.cpp` (`[tbm-cost]`).
- 4.5 (Plan 4 Group 13): covered by the heading authority. Test: `[tbm-backing]` in `tbm_route.cpp`.
- 4.6 (Plan 4 Group 14): `GetTBMWorkCorridor` and `IsTBMCorridorOccupied` (`tbm_support.cpp`), the entry refusal in `VehicleEnterTile_Rail` (`rail_cmd.cpp`), and the follower exclusion in `pathfinder/follow_track.hpp` (which now includes `tbm.h`). Test: `tbm_corridor.cpp` (`[tbm-corridor]`).
- 4.7 (Plan 4 Group 15 subset): the in-place replacement guards in `road_cmd.cpp` (level crossing) and `waypoint_cmd.cpp`, plus a third string. Test: `tbm_reuse.cpp` (`[tbm-reuse]`).
- The milestone must also run existing road, waypoint, station and path-finding tests, because those owners now consult the support registry.
- Runtime risks (resolved at the milestone; the coverage floors and the reversal needed fixes):
  - `[tbm-coverage]` assumes the planner fully covers the three floors with the four-point consist and the accepted rectangle (along -1..12, right -6..6).
  - The reversal case calls the real `CmdReverseTrainDirection` on the consist at the source bay.

## Group 3 - Saved controller and cut participants (verified)

- 3.1 (Plan 4 Group 3) implemented; see the Plan 4 Group 3 status paragraph. The milestone also needs the existing multilayer schema, profile and save tests (`multilayer-*`, `portal-*`), because the feature mask and preview changed. The Python save fixtures (`src/tests/*_save_fixture.py`) drive the real game binary, so run them only at the milestone, and check that they still accept version 376 headers.
- Static review of the Group 2B code by a read-only agent (no build): no compile errors found against the real declarations. One certain test crash was fixed: coupling calls `Train::MarkDirty`, which reads sprite bounds from the empty unit-test sprite cache. `TBMAcquisitionRig` now creates `MockEnvironment` (mock sprites cover `SPR_IMG_QUERY`). Also applied: planner errors during route replay report as Geology; the `UnavailableModel` doc now says the cutter is gated by profile policy, the hoppers by `IsEngineBuildable`. Warnings are not errors (`/W3`, no `/WX`).
- 3.2 (Plan 4 Group 4) implemented; see the Plan 4 Group 4 status paragraph.
- 3.3 (Plan 4 Group 5) implemented; see the Plan 4 Group 5 status paragraph.
- 3.4 (Plan 3 5.3B) implemented; see the Plan 3 Group 5 status line.
- 3.5-3.7 (Plan 4 Groups 6-8) implemented; see the Plan 4 Group 6, 7 and 8 status paragraphs. Design choices:
  - The face work runs inside `AdvanceTBMWork`.
  - Waits use the cheap `EstimateExcavationCut` each tick. The full preparation runs only at the boundary, so a waiting face does not validate the whole route every tick.
  - Spoil gets unknown-source provenance rather than a new `SourceType`, to avoid touching subsidy, monitor and save code.
  - Support is a registry beside the projects in the same save family.
- Not delivered in 3.7: driving the consist into the pocket and onto new support, and project reservations. These belong to master Group 7 and Plan 4 Group 14. Until then, working cutters are held still and AdvanceSupport holds.
- Stale after this group: the Plan 4 Group 5 status sentence "Until those exist, a started project holds in TravelToFace". A started project still holds there, but now only because the cutter is not at the housing.

## Group 5 - Exchange and disposal (in progress, unbuilt)

Stale since the end-of-Group-7 milestone: this group is built and verified. Its risk lists are kept as history.

- Design: [r2mlvs3mp-h](r2mlvs3mp-h-exchange-design.md). Plan 5 Groups 1-7 (master 5.1-5.7) and Plan 4 4.2B are implemented but unbuilt; the burst waits for the next milestone.
- New sources: `tbm_exchange.h`, `tbm_exchange.cpp` (in `src/CMakeLists.txt`).
- Public API now (Groups 6-7 changed Groups 1-5's first shape):
  - Assignment per hauler, not per project: `TBMExchangeAssignment` {id, project, hauler, bay, set, phase, wait, phase_ticks, exchanges, revision}. `ReleaseTBMHauler(hauler)`, `CmdReleaseTBMHauler(hauler)` and `CmdExchangeTBMWagons(project, hauler)` take the hauler. `GetTBMExchangeAssignment(project)` is the first by ID; `GetTBMExchangeAssignmentForHauler`, `GetTBMExchangeTurn(bay)`.
  - `TBMWagonSet` {id, project, wagons, state, visited_origin, revision}; `GetTBMWagonSet`, `GetTBMWagonSetForWagon`, `CaptureTBMWagonSets`.
  - Cycle: `ObserveTBMHaulerInputs` -> pure `StepTBMHaulerPhase` -> `AdvanceTBMHauler`, which runs the exchange as the project's company. `project_live` is true for a finished project, so the final return is still hauled. `AssignTBMHauler` also refuses `MissingOrigin`.
- Changed:
  - `train_cmd.h`/`train_cmd.cpp`: `ExchangeRailWagonUnits` and `CheckArrangedCargoCapacity`, placed after `MoveRailVehicleParts`. `MoveRailVehicleParts` now needs `IsRailDepotTile` when `require_depot`. The hold also takes `IsTBMHaulerHeld`; `Train::Tick` calls `AdvanceTBMHauler` for an assigned front engine, beside the `AdvanceTBMWork` call.
  - `tbm.h`: `IsTBMProjectFinished` is public; `SetTBMProjectExchangeBay`; `TBMPhaseInputs::at_exchange`; step documentation.
  - `tbm_project.cpp`: 4.2B edges, `at_exchange` observation, `IsTBMExchangeLive` through `GetTBMExchangeBay`, and `ResetTBMExchangeState` inside `ResetTBMProjectState`.
  - `tbm_work.cpp`: `IsTBMHoldingTrain` also holds in WaitForEmpties.
  - `tbm_cmd.h`: four commands. `command_type.h`: `AssignTBMHauler`, `ReleaseTBMHauler`, `SetTBMExchangeBay`, `ExchangeTBMWagons` appended. `lang/english.txt`: `STR_ERROR_TBM_EXCHANGE_CAPACITY_LOSS`.
- Tests (all new; burst tags `[tbm-exchange-bay]`, `[tbm-exchange-proposal]`, `[tbm-exchange-commit]`, `[tbm-exchange-origin]`, `[tbm-exchange-consumer]`, `[tbm-hauler-cycle]`, `[tbm-wagon-sets]`, plus `[tbm-phase]` and every `tbm-*` tag, because `MoveRailVehicleParts`, the hold and the phase step changed): `tbm_exchange_fixture.h`, `tbm_exchange_bay.cpp`, `tbm_exchange_proposal.cpp`, `tbm_exchange_commit.cpp`, `tbm_exchange_origin.cpp`, `tbm_exchange_consumer.cpp`, `tbm_hauler_cycle.cpp`, `tbm_wagon_sets.cpp`; `tbm_phase.cpp` adjusted.
- Fixture helpers: `PlaceAt` (depot-like state at a bay), `PlaceOnTrack` (started, visible, off any bay), `PlaceInDepot` (stopped in a genuine depot), `MakeOriginStationTile` (a real rail station tile at the origin's sign, (18,20)), `StepHauler()`/`StepHauler(head)`, `BuildHauler(depot, 0, 0)` for a bare locomotive.
- Compile risks to check first:
  - `std::span<Train *const>` built from `std::vector<Train *>`.
  - `Train::track` (a `TrackBits`) compared and assigned with `Track::Depot`/`Track::X`, as existing code does.
  - The aggregate `TBMExchangeSnapshot`/`TBMExchangeLedger` initialisers in the tests.
  - The `static_assert` signatures in `tbm_exchange_consumer.cpp`.
- Runtime risks to check first:
  - The fixture sets the original engine's `info.cargo_type` to PASS before `CmdBuildRailVehicle`, which asserts a valid default cargo.
  - `ExcavationFixture::Open` while a job is reserved (`BuildDepotBay`).
  - The phase after the commit tick of a one-cut rig must be ReturnForExchange.
  - `CheckTrainAttachment` lengths (8 units per part; two tiles hold 32).
  - `MakeRailStation` on the origin's sign tile, and `GetStationIndex` there for `at_origin`.
  - `CmdMoveRailVehicle` of loaded TBM hoppers between two stopped heads in the Surface depot (`[tbm-wagon-sets]`).
- Plan 5 Group 8 (receivers), design [r2mlvs3mp-i](r2mlvs3mp-i-receiver-design.md):
  - New: `spoil_receiver.h`/`.cpp`, `saveload/spoil_receiver_sl.cpp` (both CMake lists); tests `spoil_receiver_fixture.h` and `spoil_receiver_definition/intake/leg/load/delivery.cpp` (tags `[spoil-receiver-definition]`, `-intake`, `-leg`, `-load`, `-delivery`).
  - Hooks: `newgrf.cpp` (install + `SortIndustryTypes` after the TBM engines); `economy.cpp` (`PrepareUnload` origin-leg rule, `LoadUnloadVehicle` room cap, `DeliverGoodsToIndustry` admission, `TriggerIndustryProduction` early return); `industry_cmd.cpp` (register in `DoCreateNewIndustry`, unregister in `~Industry`, `AdvanceSpoilReceiver` in `OnTick_Industry`, Surface-only rule in `CmdBuildIndustry`); `misc.cpp` reset.
  - Commands appended: `SetTBMProjectOrigin`, `SetSpoilReceiverMode` (traits in `tbm_cmd.h`). Strings `STR_INDUSTRY_NAME_EARTH_RECLAMATION_SITE`/`_ROCK_CRUSHER`, outside the fixed 0x4800 industry-name block.
  - Persistence: `SaveLoadVersion::SpoilReceivers` (377), `SPRH`/`SPRR`, `fios.h`/`fios_gui.cpp` preview fields, `MULTILAYER_FEATURE_SPOIL_RECEIVERS` (64) in validation/capture/`IndustryInventory` digest, `multilayer_sl.cpp` old-version check, `afterload.cpp` `AcceptLoadedSpoilReceivers`.
  - Fixture changes: `MakeOriginStationTile` now also adds the tile to `spread` (catchment/acceptance); `AssignTBMHauler` refuses `MissingOrigin`.
  - Compile risks: `IndustryTileSpec` copy and `SubstituteGRFFileProps` assignment; `std::array<std::variant<...>>::fill(CT_INVALID)`; `Money{uint}`; the `Industry::AcceptedCargo` pointer helpers; `std::ranges::count_if(..., IsValidCargoType)`.
  - Runtime risks: `SortIndustryTypes` needs language strings (called only in `newgrf.cpp`, not in tests); `RecomputeCatchment`/`UpdateStationAcceptance` on the fixture station; `LoadUnloadStation` in a unit test (no GUI: loading indicators off, payment deleted by hand); the IIDS slot `AddEntityID` picks; save-version-377 effects on the 7 save fixtures.
  - Open question for Plan 3: the portal work record keeps its commissioning-time origin; replacing the project's origin does not touch it.
- Plan 5 Group 9 (accounting and stalls):
  - `tbm.h`: `TBMBlockedReason::LogisticsStall` appended before `End`. `tbm_exchange.h`: `TBMHaulerWait::BlockedLine`, the `TBMHaulerInputs::lost`/`receiver_room` inputs, `TBMLogisticsWait`, `GetTBMLogisticsWait`, `IsTBMLogisticsStalled`. `tbm_work.cpp`: the face-work pause after the bankruptcy check.
  - Tests: `spoil_receiver_stall.cpp` (`[spoil-receiver-stall]`) and `spoil_receiver_accounting.cpp` (`[spoil-receiver-accounting]`). `spoil_receiver_delivery.cpp` also has the full-cycle-with-disposal case (6.3). `LoadedAtOrigin` and `RunStationStop` now live in `spoil_receiver_fixture.h`.
  - Existing tests: a hauler standing at an origin without receivers now holds SaturatedReceiver. The cycle tests check only the phase there.
  - Milestone burst additions: `[spoil-receiver-definition]`, `-intake`, `-leg`, `-load`, `-delivery`, `-stall`, `-accounting`, `[tbm-hauler-cycle]`, `[tbm-wagon-sets]`, and every earlier `tbm-*`/`excavation-*` tag, because face work and unloading changed.
- Shell quirk: in a quoted bash heredoc a Python `"\\n"` reached Python as a real newline. Write edit scripts with the Write tool when they need a literal backslash.
- Not delivered yet: saving assignments and sets, and queue reload (Plan 5 Group 10, master 6.1); driving either train (master 7); refusing ordinary sale/move/refit of cutter-consist parts in a genuine depot bay (Plan 5 Group 11, master 6.2); receivers and the full cycle with real disposal (Plan 5 Group 8); fees and stall reasons (Group 9).

## Group 6 - Persist and recover (in progress, unbuilt)

Stale since the end-of-Group-7 milestone: this group is built and verified. Its risk lists are kept as history.

Context: compacted once while executing 6.1 and again during 6.5. Read Plan 5's Global Mandatory Reading, the Lifecycle and acceptance area (`VEHSChunkHandler`) and this master's Area block for Groups 6-8 again.

- 6.1 (Plan 5 Group 10), design under Persistence in [r2mlvs3mp-h](r2mlvs3mp-h-exchange-design.md#persistence-plan-5-group-10):
  - New files: `saveload/tbm_exchange_sl.cpp` (listed in `saveload/CMakeLists.txt`) and `tests/tbm_exchange_load.cpp` (tag `[tbm-exchange-load]`).
  - `tbm_exchange.h`/`.cpp`: `GetNextTBMExchangeAssignmentID`, `GetNextTBMWagonSetID`, `StageLoadedTBMExchange`, `AcceptLoadedTBMExchange`, `ComputeTBMExchangeDigest`. `ResetTBMExchangeState` also clears the staging. `ResetTBMExchangeLoadStaging` and `ValidateTBMExchangeLoadPreview` are defined in the save file.
  - Save wiring: `SaveLoadVersion::TBMExchange` (378); `saveload.cpp` (table, preview call, include); `fios.h`/`fios_gui.cpp` (`tbm_exchange_present`/`_header`/`_rows`); `multilayer_state.h`/`.cpp` (`MULTILAYER_FEATURE_TBM_EXCHANGE` = 128, its dependency on TBM projects, capture, the `Exchange` digest record); the `multilayer_sl.cpp` old-version check; `afterload.cpp` (accept after the projects).
  - The five Python save fixtures now accept versions 377 and 378. The receiver bump to 377 had already broken them.
  - Compile risks:
    - the ternary binding `const auto &rows` to a temporary `std::array`;
    - building `std::span<const SaveLoad>` from the descriptor arrays;
    - the `TBMExchangeSave` default-member capture in the test (a const aggregate without a user-provided constructor);
    - `std::set<TBMWagonSetID>` ordering.
  - Runtime risks:
    - `AcceptLoadedTBMExchange` runs before `AcceptLoadedSpoilReceivers`. That is safe, because neither reads the other's store.
    - The fixture flag test assumes the rig's reset leaves both cursors at zero.
  - Burst additions: `[tbm-exchange-load]`, `[spoil-receiver-load]`, plus every `tbm-*` tag, because `ResetTBMExchangeState` changed.
- 6.2 (Plan 5 Groups 11-12), design [r2mlvs3mp-j](r2mlvs3mp-j-lifecycle-design.md):
  - New tests:
    - `tests/tbm_exchange_lifecycle.cpp` (`[tbm-exchange-lifecycle]`);
    - `tests/tbm_exchange_acceptance.cpp` (`[tbm-exchange-acceptance]`).
  - `TBMExchangeSave` and `TBMExchangeLoadRig` moved from `tbm_exchange_load.cpp` into `tbm_exchange_fixture.h`, and gained `Reloads()`.
  - `tbm_exchange.h`/`.cpp`:
    - new `OnTBMExchangeTrainPartDeleted`, `ForgetTBMExchangeProject`, `CheckTBMTrainRearrangement`, `CheckTBMRailVehicleSale`, `CheckTBMRefit`, `IsTBMReplacementLocked`, `CheckTBMExchangeBayRemoval`, `ReleaseTBMExchangeBay`;
    - `ReleaseTBMHauler` now calls `EndTBMExchangeAssignment`.
  - `tbm.h`/`tbm_project.cpp`: `ReleaseTBMProjectExchangeBay`, `ChangeTBMOwnership`.
  - `excavation.h`/`.cpp`: `ChangeExcavationJobOwnership`. On removal it also clears a cutter binding that no longer resolves.
  - Hooks:
    - `vehicle.cpp`: `PreDestructor`.
    - `train_cmd.cpp`:
      - `MoveRailVehicleParts`, after the genuine-depot checks, so the existing bay test still gets the depot error first;
      - `CmdSellRailWagon`.
    - `vehicle_cmd.cpp`: `CmdRefitVehicle`, for every candidate unit.
    - `autoreplace_cmd.cpp`: the parts loop.
    - `rail_cmd.cpp`: `RemoveTrainDepot`, checking after `EnsureNoVehicleOnGround` and releasing after `DoClearSquare`.
    - `economy.cpp`: `ChangeOwnershipOfCompanyItems`, after the vehicle loops and before the tiles.
  - Strings: `STR_ERROR_TBM_CONSIST_LOCKED`, `_SPOIL_ABOARD`, `_SET_REFIT`, `_EXCHANGE_BAY_IN_USE`.
  - Risks to check first at the burst:
    - Direct `delete` of rig trains in the tests; the hook runs inside `PreDestructor`.
    - `GetVehicleSet` for a front with `num_vehicles == 0`.
    - `LandscapeClear` of an Underground depot (`DoClearSquare` on a rock layer).
    - `CmdSellVehicle` of a lone front engine leaves a free wagon chain, which `MoveRailVehicle` must accept with `move_chain`.
    - Whether the second cut of a two-cut `SpoilReceiverRig` reaches WaitForEmpties straight after its commit.
    - Exchange 2's unit order. The test compares sorted IDs only.
  - Burst additions: `[tbm-exchange-lifecycle]` and `[tbm-exchange-acceptance]`, plus `[tbm-exchange-bay]`, `[tbm-wagon-sets]` and `[portal-lifecycle]`, because moves, sales, depot removal and ownership changed. Run every `tbm-*`, `excavation-*` and `portal-*` tag.
- 6.3 (Plan 4 Group 17), unbuilt:
  - `tbm.h`: `TBMProjectAction::Retire` before `End`; `TBMPhaseInputs::cancelled`; step documentation.
  - `tbm_project.cpp`:
    - `GetLiveTBMJob` accepts cancelled jobs;
    - Cancel and Retire in `ControlTBMProject` (`RetireTBMProject`, `IsTBMConsistWithdrawn`);
    - the cancelled forward-phase edge and Resume to Cancelled in `StepTBMProjectPhase`;
    - `cancelled` and `route_complete` in `ObserveTBMPhaseInputs`;
    - `works` in `AdvanceTBMWork` excludes cancelled jobs;
    - adds `rail_map.h`.
  - `tbm_work.cpp`: `IsTBMExchangeDue` with a cancelled job.
  - `tbm_exchange.h`/`.cpp`: `IsTBMProjectExchangeIdle`; `DescribeTBMWagonExchange` treats a cancelled route as complete; `CmdSetTBMExchangeBay` replaces a lost bay and moves its queue.
  - `layer_portal_work.cpp`: the closed-job relaxation for Commissioning records.
  - Tests: `tests/tbm_recovery.cpp` (`[tbm-recovery]`); the `tbm_project.cpp` pause/resume check.
  - Risks:
    - the existing `[tbm-phase]` and `[tbm-project]` expectations around cancelled jobs;
    - `AcceptLoadedLayerPortalWork` with a hand-released Sealed record;
    - the partial-cut test needs `work_ticks >= 2`;
    - the breakdown test assumes the first tick does not commit;
    - selling the retired consist at the portal through `CmdSellVehicle` (the depot-like state);
    - `std::ranges::count_if` over the claims map.
  - Burst additions: `[tbm-recovery]`, `[tbm-phase]`, `[tbm-project]`, `[portal-tbm-access]`, `[portal-bootstrap]` and every `portal-*` tag, because of the portal validator.
- 6.4 (Plan 3 Group 11.3/11.4), unbuilt:
  - `layer_portal_cmd.h` and `layer_portal_work.cpp`:
    - new `WithdrawLayerPortalWorkPair` and `ReleaseLayerPortalWorkBinding`, sharing the helper `UnbindLayerPortalWork`;
    - `TransferLayerPortalWorkPair` checks the lifecycle only for `OWNER_NONE`.
  - `layer_portal_cmd.cpp`: `ChangeTileOwner_LayerPortal` refuses an unsafe pair only for `INVALID_OWNER`.
  - `tbm_project.cpp`:
    - Retire withdraws a bound pair;
    - in `ChangeTBMOwnership`, removal cancels each erased project's open job (as `OWNER_DEITY`), releases its pair and forgets its exchange records, then orphans the remaining jobs;
    - adds `core/backup_type.hpp`.
  - Tests:
    - new `tests/portal_recovery.cpp` (`[portal-recovery]`);
    - updated `tbm_recovery.cpp` (Retire withdraws the pair itself) and the removal section of `tbm_exchange_lifecycle.cpp` (job cancelled, pair released).
  - Risks:
    - the vehicle-on-ground check on the rock-side tile (`StandOn`, `z_pos`);
    - selling at the portal after Retire;
    - the bankrupt `LandscapeClear` of a released rail pair (`assert` in `ChangeTileOwner_LayerPortal`);
    - `AutoRestoreBackup` with `OWNER_DEITY`;
    - `[portal-lifecycle]`'s assigned-pair owner test, which must still refuse unchanged.
- 6.5 (Plan 15 Groups 7-15, `-G`), unbuilt. Audit: [r2mlvs3mp-k](r2mlvs3mp-k-ground-persistence-audit.md).
  - `tbm.h`/`tbm_project.cpp`:
    - `OnTBMTrainPartDeleted`, now the only call in `Vehicle::PreDestructor`; it calls the exchange hook first;
    - `OnTBMStationDeleted`, called from `Station::~Station` (`station.cpp` includes `tbm.h`);
    - Cancel accepts a cutter-less project whatever its job's state, and cancels the job only when it is open;
    - a stray duplicate doc block above `IsTBMConsistWithdrawn` was removed.
  - `layer_portal_cmd.h`/`layer_portal_work.cpp`: `ForgetLayerPortalWorkCutter`, `ForgetLayerPortalWorkOrigin`; the validator accepts a Commissioning record with no cutter.
  - `afterload.cpp`: `LogMultiLayerStateDiagnostics` moved after `AcceptLoadedSpoilReceivers`.
  - Tests:
    - `tbm_exchange_fixture.h`: `TBMPacketCustody`, used in `tbm_exchange_acceptance.cpp`;
    - `tbm_exchange_load.cpp`: section "one set carried by two haulers";
    - `spoil_receiver_load.cpp`: section "a closed receiver leaves no row behind";
    - `tbm_recovery.cpp`: the deleted-cutter, cancelled-reload and deleted-origin cases.
  - Risks:
    - `delete rig.front` in a `TBMExchangeLoadRig`: the hook order (exchange, then project, then pair) while the wagons are deleted after the front;
    - `delete rig.fixture.origin`: `Station::~Station` in a test world (kdtree removal of a station never inserted is a no-op; `RemoveOrderFromAllVehicles`);
    - `delete &rock` in the receiver rig: `Industry::~Industry` clears its Surface tiles;
    - `TBMPacketCustody` iterates `CargoPacket::Iterate()`; packets left by earlier fixtures are excluded by the baseline;
    - the reused station must get the freed index (`REQUIRE(reuse->index == origin)`).
  - Burst additions: `[tbm-recovery]`, `[tbm-exchange-acceptance]`, `[tbm-exchange-load]`, `[spoil-receiver-load]`, `[portal-recovery]`, `[portal-work-state]`, plus every `portal-*` tag (validator and record hooks) and the station-using tags `[spoil-receiver-delivery]` and `[tbm-exchange-origin]`.
- 6.6 (Plan 4 Group 2, Plan 3 Group 8), unbuilt:
  - New `src/tbm_commission.cpp` (listed in `src/CMakeLists.txt`): `CommissionTBMProject`, `PlanTBMCommission`, `GetTBMCommissionErrorMessage`, `CmdCommissionTBM`.
  - `tbm.h`: `TBMAcquisitionError::NoReceiver` and `ProjectLimit`; `TBMCommissionResult`; `CommissionTBMProject`.
  - `tbm_cmd.h` and `command_type.h`: `Commands::CommissionTBM`, appended last.
  - `layer_portal_cmd.h`/`layer_portal_work.cpp`: request wagon counts; `BindLayerPortalWorkCommissioning`; `CommissionLayerPortalTBM` delegates.
  - `layer_portal_gui.cpp`: the active control, `GetDefaultTBMCommissionModel` and `GetDefaultTBMCommissionOrigin`.
  - `lang/english.txt`: `STR_ERROR_CAN_T_COMMISSION_TBM`, eleven `STR_ERROR_TBM_COMMISSION_*` strings and a new tooltip.
  - Tests: new `tests/tbm_commission.cpp` (`[tbm-commission]`); comments in `tbm_acquisition.cpp` and `tbm_engine.cpp`.
  - Risks:
    - building the second pair at (32, 20) in the receiver rig, and its door corridor;
    - the one-cell working ring accepted by `AuthorizeExcavationOutline` (the `[tbm-area]` rig used the same ring for a straight corridor);
    - `CheckTBMOriginAdmission` with a probe project;
    - the CTAD of `AutoRestoreBackup` for the date and the money;
    - `CmdCloneVehicle` refusing a cutter in a depot it is not in;
    - the `[portal-contract]`, `[tbm-acquisition]` and `[tbm-sealed-start-gate]` checks that still expect `ControllerUnavailable` (no wagons);
    - `std::ranges::binary_search` over the row-ordered floor;
    - compiling the GUI's `Command<Commands::CommissionTBM>::Post`.
  - Burst additions: `[tbm-commission]`, `[tbm-acquisition]`, `[tbm-area]`, `[tbm-sealed-start-gate]`, `[portal-contract]`, `[portal-bootstrap]`, every `portal-*` tag, and the game build for the GUI.

## Group 7 - Ground acceptance without new movement (unbuilt)

Stale since the end-of-Group-7 milestone: 7.2 and 7.6 are verified; 7.5 and 7.8 keep the open parts named in the master.

- Split: the movement tasks (7.1, 7.3, 7.4, 7.7) moved to Group 7B, to be written after the milestone with runtime feedback.
- 7.2: a test in `tests/portal_recovery.cpp`: complete, retire, sell, convert support, lay the Surface approach, refuse a spanning hauler, then connect.
- 7.5: `TBMDeepCommissionRig` and the Deep date-gate case in `tests/tbm_commission.cpp`.
- 7.6: new `tests/tbm_conservation.cpp` (`[tbm-conservation]`).
- 7.8: `MeasureFirstCut` and the `[tbm-measure]` case in `tests/tbm_commission.cpp`; it reports `MLVS_TBM_MEASURE` lines through `WARN`.
- Risks:
  - `CmdBuildLayerPortal` on opened Underground ground (the tile types after `ExcavationFixture::Open`);
  - the Deep pocket fitting six Rock hoppers within the train-length setting;
  - `ComputeTBMRecurringCharge` after the in-test ticks (no daily charge runs);
  - the hauler spanning both ends registered through `UpdatePosition` for `EnsureNoVehicleOnGround`;
  - `CmdConvertTBMSupport` after Retire (the project gone, so the support is no longer in use).

## Milestone burst list (end of master Group 7)

Build `openttd_test` and `openttd` (Debug). Then run these tags in short bursts with timeouts, so that no run exceeds two minutes:
- Plan 5: `[tbm-exchange-bay]`, `[tbm-exchange-proposal]`, `[tbm-exchange-commit]`, `[tbm-exchange-origin]`, `[tbm-exchange-consumer]`, `[tbm-hauler-cycle]`, `[tbm-wagon-sets]`, `[spoil-receiver-definition]`, `[spoil-receiver-intake]`, `[spoil-receiver-delivery]`, `[spoil-receiver-leg]`, `[spoil-receiver-accounting]`, `[spoil-receiver-stall]`, `[spoil-receiver-load]`, `[tbm-exchange-load]`, `[tbm-exchange-lifecycle]`, `[tbm-exchange-acceptance]`.
- Plan 4: every `tbm-*` tag, including `[tbm-recovery]`, `[tbm-commission]`, `[tbm-conservation]`, `[tbm-measure]`, `[tbm-sealed-start-gate]`.
- Plan 3: every `portal-*` tag, including `[portal-recovery]`, `[portal-contract]`, `[portal-tbm-access]`.
- Plans 2/15: every `excavation-*` and `geology-*` tag, and `[multilayer-state-foundation]` and `[multilayer-state-digest]`.
- Python save fixtures: the five that accept versions 377/378.

## Milestone build and burst (end of master Group 7, 2026-10-05)

- Builds: `openttd_test` and `openttd` (Debug) build with no warnings (build 6 (`r2mlvs3mp-l-milestone-build6.log`; historical file unavailable)).
  Logs 1-5 record the compile fixes and the fix iterations; logs 7 and 8 add the last two test cases.
- Compile fixes: the `Money` multiplication in `spoil_receiver.cpp` was ambiguous; `tests/tbm_commission.cpp` needed
  `table/strings.h`.
- Product fixes found by the burst:
  - `AdvanceTBMWork` checks the exchange again after a commit. A commit at a frozen exchange breakpoint, or of the last
    cut, now goes straight to ReturnForExchange, not first onto support the consist would not use before the exchange.
  - `AcceptLoadedSpoilReceivers` with nothing staged keeps the live store, and checks that every receiver industry has
    its row.
  - A whole train stopped in the depot state on a portal assembly site can be sold there (`IsTrainStoppedAtPortalSite`
    in `CmdSellVehicle`), so a retired cutter is sold where it stands.
- Test fixes (wrong expectations, no product change):
  - Single-cut rigs expect ReturnForExchange at the commit. Two-cut rigs expect AdvanceSupport, because both cuts fit
    one load and exchange breakpoints are frozen in the pass terms.
  - The Deep measurement expects the model's work rate of 3/2.
  - A portal tile never passes `IsStoppedInDepot`, which the exchange-bay test assumed. A detached wagon keeps its own
    load in the hauler-hold test. The occupied-bay test needs the cutter waiting before the second hauler asks.
  - The conservation test now takes its exchange from a cancellation. The two-cycle test uses a bending route whose
    first breakpoint comes before its last cut; new turn constructors on `TBMExchangeRig` and `SpoilReceiverRig` build it.
  - `BuildDepotBay` flushes queued signals after building, as the command kernel does after each command. Its stale
    "depot inside" entry asserted in the fixture teardown once a test had removed the depot.
- New cases at the milestone: NoEmpties at a mid-route breakpoint (`[tbm-exchange-acceptance]`), and the ledger across a
  reversal (`[tbm-conservation]`).
- Test runner: `src/tests/test_main.cpp` (MSVC) no longer opens the abort dialog. An assert prints with its DbgHelp stack
  and exits with code 3; this located the depot assert.
- Results: all 108 project tags (`r2mlvs3mp-l-burst4-all-results.txt`; historical file unavailable) pass (per-tag sums 221 cases and 252,050
  assertions; a case with two tags counts twice). After the last two cases, the touched tags pass again
  (burst 5 (`r2mlvs3mp-l-burst5-results.txt`; historical file unavailable)). Earlier rounds: burst 1 (`r2mlvs3mp-l-burst1-results.txt`; historical file unavailable),
  burst 2 (`r2mlvs3mp-l-burst2-results.txt`; historical file unavailable), burst 3 (`r2mlvs3mp-l-burst3-all-results.txt`; historical file unavailable) and
  lifecycle by case (`r2mlvs3mp-l-lifecycle-results.txt`; historical file unavailable).
- Save fixtures (results (`r2mlvs3mp-l-save-fixture-results.txt`; historical file unavailable)): `excavation_save_fixture`,
  `excavation_pass_save_fixture`, `geology_save_fixture --refusals-only` and `portal_acceptance_save_fixture` pass.
  `portal_save_fixture` timed out twice in its `native-company` case (fixed 70 s limit), and no assertion failed.
  - The laptop ran on battery (discharging, Balanced plan, 1.9 GHz). Every fixture ran about twice as slow as at the
    Group 4 milestone.
  - A stack sample of the stalled game showed 81% of the time in vanilla `CheckCaches` (the fixtures' `-d desync=5`)
    while the title game used up the null driver's ticks.
  - Rerun after the user set the laptop to maximum performance: `portal_save_fixture` passes in 75 s, so all five
    save fixtures pass. Its 26 rewritten evidence files were restored from HEAD the same way.
- Restore: the fixtures rewrote 50 tracked evidence files, none of them modified before the run. Each was restored from
  HEAD with `git cat-file --filters`, with no index or ref change.
- Measurements for r2mlvs16: see the Plan 4 Group 20 status (standard 888 ticks, 360 charge, 216 t; Deep 1,184 ticks,
  960 charge, 504 t).
- Checked: master 5.1-5.8, 6.1-6.6, 7.2 and 7.6.
  - Plan 5 Groups 1-12.
  - Plan 4: 2.1, 2.3, 2.4, 4.2, 4.2B, 17.1-17.4 and 18.1.
  - Plan 3: 8.1-8.3, 9.3, 11.3 and 11.4.
- Still open:
  - 7.5: script commissioning, and Deep fleet movement.
  - 7.8 (Plan 4 20.1): whole-corridor time and cost with haulage.
  - Plan 3 17.1.
  - Gap G1: real-file round trips, with 7.4.

## Group 7B - Movement (in progress)

Design: [r2mlvs3mp-m](r2mlvs3mp-m-movement-design.md). Steps 1 and 2 are built; the runs are listed below.

- Portal side (`layer_portal_work.cpp`, `layer_portal_cmd.cpp`):
  - `GetLayerPortalMagazineExit`: the key of a Commissioning pair is a magazine. The project's cutter consist leaves
    it through the shaft, an assigned hauler of its Source bay through the door.
  - `IsLayerPortalMagazineBusy`: another train has a part on the key outside the depot state.
  - `GetLayerPortalAdmittedTrackdirs`: the axis for an admitted train, with the ordinary side rule; none for anyone
    else, and none from outside while the magazine is busy.
  - `VehicleEnterTile_LayerPortal` admits an assigned hauler at its bay and runs the magazine rules
    (`RunLayerPortalMagazine`): one part released at a time at the leave point, parts taken into the depot state at
    the inner point, and the last part normalizes facing and stops the train.
  - A cancelled job keeps movement authority while its project lives, so the cutter can back out to its bay. The
    project check after it still ends authority for a finished project.
- Trains (`train_cmd.cpp`):
  - The new-tile and line-end track queries fall back to the admitted track on a portal.
  - A wagon whose part ahead already stands inside follows the tile's own track (it used to get no track at all).
  - `CheckTrainStayInDepot` hands a train standing wholly on a portal to `CheckTrainStayInMagazine`. That function
    lays the parts out at the exit side's inner point when the controller lets the train go and the magazine is free.
  - `TrainCanLeaveTile` treats heading for the magazine's inner point as entering a depot.
  - A boring machine, or a hauler in front of its busy bay, waits at a line end instead of reversing.
  - A boring machine always backs up and is never flipped. `BackUpTBMConsist` is the controller's reversal.
- Controller (`tbm_work.cpp`, `tbm_project.cpp`, `tbm_exchange.cpp`):
  - AdvanceSupport no longer holds the train.
  - Entering ReturnForExchange unblocked backs the consist up once.
  - `IsTBMHaulerAdmittedAtBay`, `MayTBMTrainLeaveMagazine`, `IsTBMWaitingAtLineEnd` and
    `GetTBMHaulerSteeringTarget` answer the movement questions.
- Pathfinder: `follow_track.hpp` uses the admitted track on a portal, both for the next tile and in its old-tile
  assert. `yapf_destrail.hpp` steers a hauler returning empties to its bay, entered through the door, and keeps that
  search out of the shared segment cache.
- Tests (`src/tests/tbm_movement.cpp`, `src/tests/portal_movement.cpp`, shared helpers in
  `src/tests/movement_fixture.h`):
  - `[tbm-movement]`:
    - the whole three-cut route by real movement, straight and on a bend whose mid-route exchange sends the consist
      out again;
    - a cancellation while the consist straddles the pair;
    - the admitted-track queries at a Commissioning and a Sealed pair;
    - an ordinary train turned back at a Commissioning pair's door;
    - the whole life of a pair: bore, retire, Sealed refusal, then freight through the bored tunnel.
  - `[tbm-loop]`: both trains drive the whole route on the rig's straight route, on its bend, and on a 6-cut route
    commissioned by command, plus a replay of the commissioned loop. The hauler has one order to the origin and
    delivers to real receivers. Every exchange comes at its frozen breakpoint, and every committed tonne is delivered.
  - `[portal-movement]`: freight crossing, reversal while spanning, and signals, on both allowed rail pairs.
- Visible/hidden equivalence (decision 1): the tests run with no viewport at all. No simulation path reads the
  viewport layer; a vehicle's hidden flag comes only from the portal point rule (FD-3) and the depot state.

### Group 7B build-and-test runs (2026-10-05, on mains power)

- Run 1 (build 1 (`r2mlvs3mp-l-movement-build1.log`; historical file unavailable), build 2 (`r2mlvs3mp-l-movement-build2.log`; historical file unavailable)): the first build
  refused `std::to_string` in the new test (an OpenTTD safeguard); `fmt::format` replaced it. All
  55 TBM, portal and spoil tags (`r2mlvs3mp-l-movement-run1-tags.txt`; historical file unavailable) pass with the step 1 and 2 changes. Both new
  tags stopped early (output (`r2mlvs3mp-l-movement-run1-new.txt`; historical file unavailable)):
  - `[tbm-movement]`: a divide by zero in vanilla `TrainController` (`tick_counter % path_backoff_interval`). Unit
    tests start with every game setting at zero, and no earlier test drove a train. The movement tests now reset the
    `pf.*` settings to their defaults (`PathfinderDefaults`).
  - `[tbm-loop]`: building a depot names it after the nearest town, and the fixture town was never in the town index.
    The loop test indexes it for its own duration (`TownIndexEntry`).
  - The divide by zero gave no stack, because Catch catches it. A small fault catcher outside the repository
    (`%TEMP%\stack_sampler\faultstack.exe`) runs the test under the Windows debug API and prints the stack of the
    first hardware fault.
- Run 2 (build 3 (`r2mlvs3mp-l-movement-build3.log`; historical file unavailable), output (`r2mlvs3mp-l-movement-run2-new.txt`; historical file unavailable)): both tags
  asserted in `TryReserveRailTrack`. The reservation asserts checked the ordinary track status, which a Commissioning
  pair hides. `HasReservableTrack` in `pbs.cpp` also accepts a rail portal end's own axis.
- Run 3 (build 4 (`r2mlvs3mp-l-movement-build4.log`; historical file unavailable), output (`r2mlvs3mp-l-movement-run3-new.txt`; historical file unavailable)): both tags asserted
  in the follower, from `FreeTrainTrackReservation`, after the cutter consist reversed at "invalid rail" on its final
  return. The last cut completes the job, and `CheckLayerPortalWorkAccess` refused a completed job, so the consist
  straddling the pair lost its authority. A closed job, cancelled or completed, now keeps authority while its own
  project lives. In the loop test this came after the first exchange cycle: the hauler was steered back, exchanged
  and the cutter left again for its third cut.
- After run 3 the user confirmed mains power and lifted the run limit.
- Run 4 (build 5 (`r2mlvs3mp-l-movement-build5.log`; historical file unavailable), output (`r2mlvs3mp-l-movement-run4-new.txt`; historical file unavailable)): `[tbm-movement]`
  passes (3 cases, 13,215 assertions). `[tbm-loop]` ran its 200,000 ticks without finishing: the cutter was
  Completed, but the hauler stood stopped in its depot with the load half delivered.
- Run 5 (build 6 (`r2mlvs3mp-l-movement-build6.log`; historical file unavailable), journal (`r2mlvs3mp-l-movement-run5-loop.txt`; historical file unavailable)): a state journal
  in the loop test showed the whole cycle. The hauler waited in its depot while the cutter was on the pair (with no
  signals the line is one block), was steered to the bay, exchanged at tick 2,491, delivered part of the load (the
  receivers admit 32 t per interval), and turned round in its depot. Vanilla `VehicleEnteredDepotThisTick` stops
  every train entering a depot, and `CallVehicleTicks` starts it again after its autoreplace step. The test had
  ticked the trains directly, so nothing restarted the hauler. The loop test now runs the game's vehicle step.
- Run 6 (build 7 (`r2mlvs3mp-l-movement-build7.log`; historical file unavailable), output (`r2mlvs3mp-l-movement-run6-loop.txt`; historical file unavailable)): `[tbm-loop]`
  passes (36,015 assertions): every committed tonne reaches a receiver, one exchange per frozen breakpoint, and the
  hauler's single order is untouched.
- Burst after run 6 (results (`r2mlvs3mp-l-movement-burst-all.txt`; historical file unavailable)): all 110 project tags pass, the two movement tags
  included.
- Run 7 (build 8 (`r2mlvs3mp-l-movement-build8.log`; historical file unavailable), output (`r2mlvs3mp-l-movement-run7-portal.txt`; historical file unavailable)): the new
  `[portal-movement]` tag (`src/tests/portal_movement.cpp`) asserted in the vehicle pool. The test built its wagons
  without `Vehicle::CanAllocateItem`, which a pool allocation must follow. Every build now checks it first.
- Run 8 (build 9 (`r2mlvs3mp-l-movement-build9.log`; historical file unavailable), output (`r2mlvs3mp-l-movement-run8-portal.txt`; historical file unavailable)):
  `[portal-movement]` passes (3 cases, 11,944 assertions). On both allowed rail pairs, Surface to Underground and
  Underground to Deep, an ordinary train crosses a Connected pair and comes back. Reversed while it spans the pair, it
  stays whole on its tiles. The block signals at both ends show red while it spans the pair and green once it is home.
- Run 9 (build 10 (`r2mlvs3mp-l-movement-build10.log`; historical file unavailable), output (`r2mlvs3mp-l-movement-run9-cancel.txt`; historical file unavailable)):
  `[tbm-movement]` passes with the cancellation case (4 cases, 20,715 assertions). A consist cancelled while it
  straddles the pair backs whole into its magazine: in the first cut, in the second cut with spoil aboard, and when
  the player stops it on the way back.
- Run 10 (build 11 (`r2mlvs3mp-l-movement-build11.log`; historical file unavailable), summary and trace (`r2mlvs3mp-l-movement-run10-loop.txt`; historical file unavailable)):
  `[tbm-loop]` passes (2 cases, 108,330 assertions).
  - The loop now checks the Earth/Rock ledger at every transition. Every committed tonne is in exactly one place:
    aboard a train, on a platform, or received.
  - The trace records each train's wagon IDs. The exchange swaps the loaded wagons for empties: the cutter goes from
    [0,1,2,3,4,5] to [0,1,12,13,14,15], the hauler from [11,12,13,14,15] to [11,2,3,4,5].
  - A second run from the same seeded fixture gives the same trace (Plan 5 13.3).
  - The assertion count is per evaluation: the loop checks crashes, the rock end and the ledger on every tick.
- Visible/hidden equivalence (audit (`r2mlvs3mp-l-movement-layer-audit.txt`; historical file unavailable)): every read of `Viewport::layer` and
  `GetVehicleDisplayLayer` outside the tests is in GUI code, so no simulation path depends on the layer shown.
- After run 10 the train builder moved into `movement_fixture.h` as `BuildTrainInDepot`, and a whole-life test
  (Plan 3 15.2) joined `[tbm-movement]`.
- Run 11 (build 12 (`r2mlvs3mp-l-movement-build12.log`; historical file unavailable), output (`r2mlvs3mp-l-movement-run11-new.txt`; historical file unavailable)): the whole-life
  test got as far as opening the pair. The bore, the retirement and the refusal at the Sealed door all passed. The
  freight train's departure then asserted in `UpdateSignalsOnSegment`, because the signal buffer was not empty. The
  test had opened the pair with `ChangeLayerPortalConnection` directly, which queues signal updates that only the
  command kernel finishes. The test now uses the player's command and finishes the queued signals, as the kernel
  does. `[portal-movement]` and `[tbm-loop]` passed.
- Run 12 (build 13 (`r2mlvs3mp-l-movement-build13.log`; historical file unavailable), output (`r2mlvs3mp-l-movement-run12-life.txt`; historical file unavailable)):
  `[tbm-movement]` passes with the whole-life test (5 cases, 28,242 assertions). After it, all 111 tags passed
  (burst (`r2mlvs3mp-l-movement-burst2-all.txt`; historical file unavailable)).
- Run 13 (build 14 (`r2mlvs3mp-l-movement-build14.log`; historical file unavailable), output (`r2mlvs3mp-l-movement-run13-loop.txt`; historical file unavailable)): a loop
  commissioned through `CmdCommissionTBM` at a second pair passes. The traces showed that every loop so far exchanged
  only once, at the end of its route: three straight cuts and the command's two cuts both fit one load. So no run had
  shown boring resume after an exchange, and the first `[tbm-movement]` case never reached its "leave again" branch.
- Run 14 (build 15 (`r2mlvs3mp-l-movement-build15.log`; historical file unavailable), output (`r2mlvs3mp-l-movement-run14-bend.txt`; historical file unavailable)): the bend of
  `[tbm-exchange-acceptance]` overflows one load.
  - The loop on that bend passes. The cutter bores on with the empties, and the loop now checks that each exchange
    comes at its frozen breakpoint.
  - The bending section of the first `[tbm-movement]` case failed one check: it expected the saved heading to stay at
    the first cut's. The saved heading follows each committed cut, so on a bend it turns. Backing never changes it, as
    the check meant to show, and it now compares with the last committed cut.
- Run 15 (build 16 (`r2mlvs3mp-l-movement-build16.log`; historical file unavailable), output (`r2mlvs3mp-l-movement-run15-bend.txt`; historical file unavailable)):
  `[tbm-movement]` passes (5 cases, 33,557 assertions), and all 111 tags pass
  (burst (`r2mlvs3mp-l-movement-burst3-all.txt`; historical file unavailable)).
- Run 16 captured build 16's loop traces; run 17 replaced them.
- Run 17 (build 17 (`r2mlvs3mp-l-movement-build17.log`; historical file unavailable), output (`r2mlvs3mp-l-movement-run17-loop.txt`; historical file unavailable),
  traces (`r2mlvs3mp-l-movement-run17-loops.txt`; historical file unavailable)): the commissioning area now covers seven cells. The command plans six
  cuts, which overflow one load, so a single run goes commission, cut, fill, exchange, delivery, empty return and
  resumed boring. `[tbm-loop]` passes (4 cases, 248,664 assertions), and all 111 tags pass on build 17
  (burst (`r2mlvs3mp-l-movement-burst4-all.txt`; historical file unavailable)).

### Master 7.9 - Playable from the portal window (2026-10-05)

The user asked for a game they can play. Only the commissioning button reached the TBM from the GUI, so a project
stalled at its first exchange. Plan 5 13.4, Plan 4 16.3 and master 7.9 were added for the missing controls.
- `src/tbm_controls.h/.cpp`: `GetTBMPairControls` lists what a company can do at a pair, using the commands' own
  estimates run as that company, and names the next step of the project's life.
- `src/layer_portal_gui.cpp`: the portal window gets a corridor length, a project panel and nine buttons. A train is
  picked as hauler by clicking it, as the depot's clone tool picks one. Cancel asks first.
- Named refusals in `CmdAssignTBMHauler`, `CmdReleaseTBMHauler` and `CmdControlTBMProject`, with new English strings.
- Product fixes:
  - A finished project's hauler is admitted at its bay only while part of it stands there, and may then always
    leave. Before this, the hauler's empty return after the last delivery steered it back into the bay, where it
    could not leave.
  - A cutter with a project is exempt from automatic servicing, and its bay services it on every entry. Before this,
    the daily service check would give an overdue cutter a depot order, which the controller holds as Servicing
    forever. This was found by reading the code; the tests never ran the daily steps.
- Runs:
  - Build 1 (`r2mlvs3mp-l-controls-build1.log`; historical file unavailable) failed. Commands whose first argument is no tile take no location in
    `Post`, and `UpdateWidgetSize` takes the size before the padding.
  - Run 1 (output (`r2mlvs3mp-l-controls-run1.txt`; historical file unavailable)): everything passed except my own expectation that Connect is
    refused on a Connected pair. It is accepted as a no-op, so the test now checks the state.
  - Run 2 (output (`r2mlvs3mp-l-controls-run2.txt`; historical file unavailable)) and the first burst (`r2mlvs3mp-l-controls-burst1-all.txt`; historical file unavailable): all
    112 tags pass.
  - Runs 3-4 (output (`r2mlvs3mp-l-controls-run3.txt`; historical file unavailable), daily (`r2mlvs3mp-l-controls-run4-daily.txt`; historical file unavailable)) cover the
    servicing fix, then the controls test with the game's daily train steps, a 150-day service interval and every
    train overdue.
  - Final burst (`r2mlvs3mp-l-controls-burst2-all.txt`; historical file unavailable): all 112 tags pass.
- Games rebuilt with all of it: Debug at 22:54 and RelWithDebInfo at 22:51. A headless run of the RelWithDebInfo
  build with the Multilayer Industry profile loaded the package and generated the rock layers.
