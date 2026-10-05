# R2MLVS3MP-D - Execution notes

Owner: [R2MLVS3MP](r2mlvs3mp-master-plan.md). Targets: [Plan 3](r2mlvs3-sealed-portals-and-deep-rail.md), [Plan 4](r2mlvs4-tunnel-boring-machines.md). Shared decisions: parent AD-3..AD-6. These notes hold evidence and continuation context for the master queue; child evidence keeps the child's own prefix.

## Reading already done (do not reread unless the task needs it)

- Parent AD-3..AD-6, I-1..I-17, Plan Index staging paragraph and Execution conventions.
- Plan 3 and Plan 4 Global Mandatory Reading tables and Public Handoff Contracts; Plan 3 Groups 1-12 plus its execution notes; Plan 4 Groups 1-5 plus its execution notes (verified sealed-start refusal).
- Plan 14 Groups 7-15 area and Plan 15 Groups 1-5 area (selected-family gate).
- Stale/irrelevant for later groups: the Plan 3/4 "Continuation" paragraphs that say work is uncommitted. HEAD `5f40e4d755` now holds every Plan 2/3/4 change made so far.

## Build and test commands in use

- CMake: `/c/Program Files (x86)/Microsoft Visual Studio/2022/BuildTools/Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin/cmake.exe`.
- Debug test build: `cmake --build build --config Debug --target openttd_test --parallel -- /nodeReuse:false` (about one minute incremental).
- Focused tests: from `build/`, `./Debug/openttd_test.exe "<tag>,<tag>"`. Catch2 does not expand `[prefix-*]`; build the tag list from `--list-tags`.

## Build cadence (user instruction, 2026-10-05)

Do not build or run tests after each change. Compiling slows the user's machine. Build and run one focused burst only at a milestone, at most once every three or four master groups. The end-of-Group-4 milestone ran on 2026-10-05 (see [the milestone section](#milestone-build-and-burst-end-of-master-group-4-2026-10-05)). Code from master Group 5 on is unbuilt until the next milestone, due at the end of master Group 7 or 8 (or earlier only if the user asks). Read real signatures before using them, because compile errors surface late.

Every test run must be wrapped in `timeout` (a Debug assert opens a modal dialog and hangs the run). Run tags one at a time with the runner described in the milestone section.

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

- Builds: Debug `openttd_test` ([g build](r2mlvs3mp-g-milestone-build.log), then [rebuild 2](r2mlvs3mp-g-milestone-build2.log) and [rebuild 3](r2mlvs3mp-g-milestone-build3.log) after the fixes below); no warnings. Debug `openttd` for the save fixtures ([game build](r2mlvs3mp-g-milestone-game-build.log)).
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
- Results: first per-tag pass [burst 1](r2mlvs3mp-g-burst1-tbm-results.txt), then [burst 2](r2mlvs3mp-g-burst2-tbm-results.txt) and [burst 3](r2mlvs3mp-g-burst3-fixed-results.txt) after fixes. Final: [all 86 project tags](r2mlvs3mp-g-burst4-all-results.txt) (every `excavation-*`, `geology-*`, `geothermal-*`, `multilayer-*`, `portal-*`, `solar-*` and `tbm-*` tag) pass: 145 cases, 91,436 assertions.
- Save fixtures: all seven `src/tests/*_save_fixture.py` pass at version 376 ([results](r2mlvs3mp-g-save-fixture-results.txt)). They rewrite retained evidence under the owning child's prefix. `geology_save_fixture.py` in full mode also overwrites `r2mlvs2-g-native.sav`, which is the geothermal fixture's geology-only predecessor. Run it with `--refusals-only`, then restore every rewritten tracked file with `git cat-file --filters HEAD:<path>` (to a temporary name, then move it into place; a direct redirect truncates first). Record outcomes only in the master's `r2mlvs3mp-g-` files.
- Coverage gaps: no unit test drives `CmdMoveRailVehicle` or autoreplace, and no existing unit test covers ordinary rail, road, waypoint, station or path-finding commands. Those hooks are covered only through the TBM support, corridor and reuse tags. The untagged upstream tests were not run (full regression is rare by instruction).

## Group 1 - Verify reusable foundations

- 1.1: Plan 1 Groups 1-23, Plan 2 Groups 1-14 and Plan 14 Groups 1-6 are all checked in their plans. Exports verified by `grep` (table in [r2mlvs3mp-e](r2mlvs3mp-e-scoped-gates.md#producer-exports-at-this-snapshot)). Plan 1/14 burst: 18 cases/715 assertions ([b](r2mlvs3mp-b-foundation-plan1-14-results.txt)).
- 1.1 finding: the first Plan 2 burst failed 1 case/3 assertions. `src/tests/excavation_guard.cpp` still expected `CmdBuildLayerPortal` into a solid door to refuse with `STR_ERROR_EXCAVATION_SOLID_ROCK`. Plan 3 Group 4/5 deliberately changed that: `PrepareNativePortalBuild` accepts a solid Clear door as a Sealed face. The test was not part of the Plan 3/4 bursts, so the stale expectation went unnoticed. The assertion now checks a pure quote that succeeds, allocates no pair and leaves packed geology and tiles unchanged. After the fix, Plan 2 burst: 44 cases/5,884 assertions ([b](r2mlvs3mp-b-foundation-plan2-results.txt)).
- 1.2: Plan 3 Groups 1-7 (except the correctly pending 5.3B), 10 and 14 are checked; Plan 4 Group 1 and prefixes 2.1A/2.1B/2.2 are checked. Portal/TBM burst: 28 cases/1,274 assertions ([b](r2mlvs3mp-b-foundation-plan3-4-results.txt)). `[tbm-sealed-start-gate]` still proves the refusal; it is not a success.
- 1.3: Plan 13 Groups 1-8 checked; `python src/tests/multilayer_sprite_contract.py` validates 141 families/58,059 required sprites read-only ([c](r2mlvs3mp-c-sprite-contract-results.txt)). Templates are inputs, not runtime art.
- 1.4: [r2mlvs3mp-e](r2mlvs3mp-e-scoped-gates.md) records every scoped portion and gate. Child plans now point to it: Plan 4 adds 4.2A/4.2B; Plan 14 and Plan 15 name `-G`/`-T`/`-O` portions.
- Build: Debug `openttd_test` incremental build passes ([a](r2mlvs3mp-a-baseline-build.log)). No full regression was run.

## Group 2 - Reconcile the solid starter (verified)

- Contract: [r2mlvs3mp-f](r2mlvs3mp-f-solid-starter-contract.md). Plan 3 8B.1 and Plan 2 14B.1 are ticked (contract freeze). Every other task in Plan 3 Group 8B, Plan 2 Group 14B and Plan 4 Group 2B stays unticked until the milestone burst passes. Their plans now carry an "R2MLVS3MP status" paragraph and new Public Handoff Contract entries.
- Plan 4 Group 2B design decisions:
  - Preparation is pure.
  - Acquisition builds stopped, hidden parts on the source site and holds them in a `TBMAcquisition` owner. It charges nothing and reserves no job.
  - Publication (pay, reserve while Sealed, publish Commissioning) is master 6.6.
  - The site is validated once, while empty. `ValidateLayerPortalCommissioning` refuses any vehicle on the source, so per-part revalidation cannot work after the cutter exists.
- Planner history size for a consist: ceil(length / 16) + 1, clamped to at least 3; above 16 refuses. Cutter plus two Earth and two Rock hoppers = 48 units = 4 points. The first segment is identical for any history of 3 or more points (all points start at the housing). Differences appear from cut index 3 on a straight route.
- Unit tests start with `max_trains` and `max_train_length` at zero; the acquisition rig sets 500 and 7.
- Supersession notes were added under Plan 4 Group 20 and Plan 15 Group 16: cut 0 charges 12 cells, not the 3 of a cleared area.

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

Context: compacted once while executing 6.1. Read Plan 5's Global Mandatory Reading, the Lifecycle and acceptance area (`VEHSChunkHandler`) and this master's Area block for Groups 6-8 again.

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
