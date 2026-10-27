# Linux build fixes: sequential execution notes

@see [Linux build fixes plan](r2lbf-linux-build-fixes.md). This is an implementation record, not an additional mandatory reading queue.

**Run identity:** logs for this execution live in `logs/linux-baseline/20261027-linux/` (git-ignored), and disposable tooling lives in `test-cache/linux-baseline/` (git-ignored). Both can be rebuilt with the commands recorded in each group below.

**Test policy for this execution (user instruction, 2026-10-27):** tests run in focused bursts for the group being developed. The full CTest suite runs only as the Group 8 baseline and once more at the end of the plan (Groups 14-17). Each group's "full suite" check in the plan is narrowed to its affected names and files.

## Commands and toolchain

GCC 13.3.0 (`build/`, `/usr/bin/c++`) and Ubuntu Clang 18.1.3 (`build-clang/`), CMake 3.28.3, Ninja 1.11.1, `RelWithDebInfo`, `-j4`, one tree at a time.

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo
CC=clang CXX=clang++ cmake -S . -B build-clang -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo
ninja -C build -j4 -k 0                                  # then the same for build-clang
python3 -I -B test-cache/linux-baseline/materialize_baseset.py build build-clang
ctest --test-dir build -j4 --timeout 600 --output-junit $PWD/logs/linux-baseline/20261027-linux/ctest-gcc.xml
python3 -I test-cache/linux-baseline/ledger.py --junit <report.xml> --src src --out <ledger.tsv>
test-cache/linux-baseline/rerun_failed.sh build <ledger.tsv> <out-dir>
```

## Failure ledger

Categories follow the Group 8 diagram: I isolation, S stale expectation, P Linux portability, C code defect, X compiler divergence, E environment.

| Test | Family | GCC | Clang | Category | Evidence | Group |
| --- | --- | --- | --- | --- | --- | --- |
| Excavation outline extra work is explicit charged and read-only (`src/tests/excavation_outline.cpp:43`) | excavation | fail (alone fail, file order fail); Group 11: pass alone and in file order | pass (Group 14 final suite) | S1 halved yields | `reruns/Excavation_outline_extra_work_is_explicit_charged_and_read-only.log`, `files/excavation_outline.xml` | 11 |
| Geothermal interrupted collector holds construction and exclusive source through abandonment (`src/tests/geothermal_collector.cpp:194`) | geothermal | fail (alone fail, file order pass) | pass (Group 14 final suite) | I (T3 local company) | `reruns/Geothermal_interrupted_collector_holds_construction_and_exclusive_source_through_abandonment.log`, `files/geothermal_collector.xml` | 10 |
| Native riser abandonment retains material custody and cannot regain readiness (`src/tests/geothermal_riser_lifecycle.cpp:79`) | geothermal | fail (alone fail, file order pass) | pass (Group 14 final suite) | I (T3 local company) | `reruns/Native_riser_abandonment_retains_material_custody_and_cannot_regain_readiness.log`, `files/geothermal_riser_lifecycle.xml` | 10 |
| Recipe planner checks integer limits before complete commit (`src/tests/layer_recipe_batch.cpp:274`) | layer | fail (alone fail, file order pass) | pass (Group 14 final suite) | I (T4 recipe world) | `reruns/Recipe_planner_checks_integer_limits_before_complete_commit.log`, `files/layer_recipe_batch.xml` | 10 |
| Recipe quotes bind their exact owner state and complete deltas (`src/tests/layer_recipe_batch.cpp:242`) | layer | fail (alone fail, file order pass) | pass (Group 14 final suite) | I (T4 recipe world) | `reruns/Recipe_quotes_bind_their_exact_owner_state_and_complete_deltas.log`, `files/layer_recipe_batch.xml` | 10 |
| Shared recipe commit preserves exact all-input deltas and refuses a retry (`src/tests/layer_recipe_batch.cpp:220`) | layer | fail (alone fail, file order pass) | pass (Group 14 final suite) | I (T4 recipe world) | `reruns/Shared_recipe_commit_preserves_exact_all-input_deltas_and_refuses_a_retry.log`, `files/layer_recipe_batch.xml` | 10 |
| Native purchase previews resolve actual engines and both availability states (`src/tests/multilayer_control_selection.cpp:140`) | m1 | fail (alone fail, file order fail); Group 13: pass alone and in file order (75cf506b ground art) | pass (Group 14 final suite) | S3 UI/content drift | `reruns/Native_purchase_previews_resolve_actual_engines_and_both_availability_states.log`, `files/multilayer_control_selection.xml` | 13 |
| A blocked hauler pauses new cutting until its route is found (`src/tests/spoil_receiver_stall.cpp:56`) | spoil | fail (alone fail, file order fail); Group 12: pass alone and in file order (bfba42fd halved rate) | pass (Group 14 final suite) | S2 TBM rate/phase/support/planner | `reruns/A_blocked_hauler_pauses_new_cutting_until_its_route_is_found.log`, `files/spoil_receiver_stall.xml` | 12 |
| A TBM holds at a junction until its actual work route is restored (`src/tests/tbm_movement.cpp:694`) | tbm | fail (alone fail, file order fail); Group 11: pass alone and in file order | pass (Group 14 final suite) | S1 halved yields | `reruns/A_TBM_holds_at_a_junction_until_its_actual_work_route_is_restored.log`, `files/tbm_movement.xml` | 11 |
| A TBM leaves its work hold for a real depot visit and returns to the face (`src/tests/tbm_movement.cpp:256`) | tbm | fail (alone fail, file order fail); Group 12: pass alone and in file order (bfba42fd hangar Resume and bay staging) | pass (Group 14 final suite) | S2 TBM rate/phase/support/planner | `reruns/A_TBM_leaves_its_work_hold_for_a_real_depot_visit_and_returns_to_the_face.log`, `files/tbm_movement.xml` | 12 |
| A TBM releases at the remote hangar and commissions its next job there (`src/tests/tbm_hangar.cpp:308`) | tbm | fail (alone fail, file order fail); Group 12: pass alone and in file order (0119308a r2df 7.3 support blocker) | pass (Group 14 final suite) | S2 TBM rate/phase/support/planner | `reruns/A_TBM_releases_at_the_remote_hangar_and_commissions_its_next_job_there.log`, `files/tbm_hangar.xml` | 12 |
| A project's whole life can be played from the portal window's controls (`src/tests/tbm_movement.cpp:875`) | tbm | fail (alone fail, file order fail); Group 11: pass alone and in file order | pass (Group 14 final suite) | S1 halved yields | `reruns/A_project_s_whole_life_can_be_played_from_the_portal_window_s_controls.log`, `files/tbm_movement.xml` | 11 |
| A reload during a bend continues the same cursor and ledger (`src/tests/tbm_coverage.cpp:216`) | tbm | fail (alone fail, file order fail); Group 11: pass alone and in file order | pass (Group 14 final suite) | S1 halved yields | `reruns/A_reload_during_a_bend_continues_the_same_cursor_and_ledger.log`, `files/tbm_coverage.xml` | 11 |
| A started cutter drives to its face, works its route and backs into its magazine (`src/tests/tbm_movement.cpp:325`) | tbm | fail (alone fail, file order fail); Group 11: pass alone and in file order | pass (Group 14 final suite) | S1 halved yields | `reruns/A_started_cutter_drives_to_its_face__works_its_route_and_backs_into_its_magazine.log`, `files/tbm_movement.xml` | 11 |
| Hauler cycle reports insufficient complete face room without a partial transfer or turnaround (`src/tests/tbm_face_cycle.cpp:48`) | tbm | fail (alone pass, file order pass) | pass (Group 14 final suite) | self-inflicted Group 4 comment, fixed in Group 8 | `reruns/Hauler_cycle_reports_insufficient_complete_face_room_without_a_partial_transfer_or_turnaround.log`, `files/tbm_face_cycle.xml` | 4 |
| Project controls pause and resume without touching orders or phase (`src/tests/tbm_project.cpp:81`) | tbm | fail (alone fail, file order fail); Group 12: pass alone and in file order (bfba42fd FollowOrders) | pass (Group 14 final suite) | S2 TBM rate/phase/support/planner | `reruns/Project_controls_pause_and_resume_without_touching_orders_or_phase.log`, `files/tbm_project.xml` | 12 |
| Released support stays while a train stands on it (`src/tests/tbm_corridor.cpp:95`) | tbm | fail (alone fail, file order fail); Group 12: pass alone and in file order (bfba42fd support in use) | pass (Group 14 final suite) | S2 TBM rate/phase/support/planner | `reruns/Released_support_stays_while_a_train_stands_on_it.log`, `files/tbm_corridor.xml` | 12 |
| Sealed TBM startup needs the door heading and capacity for the whole pocket (`src/tests/tbm_engine.cpp:114`) | tbm | fail (alone fail, file order fail); Group 11: pass alone and in file order | pass (Group 14 final suite) | S1 halved yields | `reruns/Sealed_TBM_startup_needs_the_door_heading_and_capacity_for_the_whole_pocket.log`, `files/tbm_engine.xml` | 11 |
| The first cut of a standard and a Deep corridor is measured (`src/tests/tbm_commission.cpp:371`) | tbm | fail (alone fail, file order fail); Group 12: pass alone and in file order (bfba42fd halved rate) | pass (Group 14 final suite) | S2 TBM rate/phase/support/planner | `reruns/The_first_cut_of_a_standard_and_a_Deep_corridor_is_measured.log`, `files/tbm_commission.xml` | 12 |
| Tube depot power restoration services the actual capsule without starting it (`src/tests/transport_tube_recovery.cpp:104`) | transport | fail (alone fail, file order pass) | pass (Group 14 final suite) | I (T1 sprite cache) | `reruns/Tube_depot_power_restoration_services_the_actual_capsule_without_starting_it.log`, `files/transport_tube_recovery.xml` | 9 |
| Tube physical topology change revokes unused original reservation without refund (`src/tests/transport_tube_recovery.cpp:87`) | transport | fail (alone fail, file order pass) | pass (Group 14 final suite) | I (T1 sprite cache) | `reruns/Tube_physical_topology_change_revokes_unused_original_reservation_without_refund.log`, `files/transport_tube_recovery.xml` | 9 |
| Tube purchase and refit preserve the native movement and cargo domain (`src/tests/transport_tube_construction.cpp:226`) | transport | fail (alone fail, file order pass) | pass (Group 14 final suite) | I (T1 sprite cache) | `reruns/Tube_purchase_and_refit_preserve_the_native_movement_and_cargo_domain.log`, `files/transport_tube_construction.xml` | 9 |
| Tube restored service preserves manual depot stop and queued unpowered custody (`src/tests/transport_tube_recovery.cpp:73`) | transport | fail (alone fail, file order pass) | pass (Group 14 final suite) | I (T1 sprite cache) | `reruns/Tube_restored_service_preserves_manual_depot_stop_and_queued_unpowered_custody.log`, `files/transport_tube_recovery.xml` | 9 |
| Tube routine allocation leaves native physical path topology stable (`src/tests/transport_tube_recovery.cpp:116`) | transport | fail (alone fail, file order pass) | pass (Group 14 final suite) | I (T1 sprite cache) | `reruns/Tube_routine_allocation_leaves_native_physical_path_topology_stable.log`, `files/transport_tube_recovery.xml` | 9 |
| Tube terminals and utility footprints require every actual floor and approach (`src/tests/transport_tube_construction.cpp:158`) | transport | fail (alone fail, file order pass) | pass (Group 14 final suite) | I (T1 sprite cache) | `reruns/Tube_terminals_and_utility_footprints_require_every_actual_floor_and_approach.log`, `files/transport_tube_construction.xml` | 9 |
| Five actual capsule bodies preserve load across native service and source waits (`src/tests/transport_tube_vehicle_sprite.cpp:82`) | tube | fail (alone fail, file order fail) | pass (Group 14 final suite) | I (T1 sprite cache) | `reruns/Five_actual_capsule_bodies_preserve_load_across_native_service_and_source_waits.log`, `files/transport_tube_vehicle_sprite.xml` | 9 |
| Tube platform art warns on the exact original full native packet queue (`src/tests/transport_tube_facility_sprite.cpp:158`) | tube | fail (alone fail, file order not-reached) | pass (Group 14 final suite) | I (T1 sprite cache) | `reruns/Tube_platform_art_warns_on_the_exact_original_full_native_packet_queue.log`, `files/transport_tube_facility_sprite.xml` | 9 |
| Tube terminal and interchange art follows real native platform roles and joins (`src/tests/transport_tube_facility_sprite.cpp:59`) | tube | fail (alone fail, file order fail) | pass (Group 14 final suite) | I (T1 sprite cache) | `reruns/Tube_terminal_and_interchange_art_follows_real_native_platform_roles_and_joins.log`, `files/transport_tube_facility_sprite.xml` | 9 |
| Visible chamber quote refuses rail turns forbidden by the current game rule (`src/tests/underground_workflow.cpp:523`) | underground | fail (alone fail, file order fail) | pass (Group 14 final suite) | S2 (12.7: the planner avoids forbidden bends by design, r2df; owner chose a lower-level case). Group 13D: renamed "Visible chamber quote plans only rail turns the current game rule allows", passes alone and in file order | `reruns/Visible_chamber_quote_refuses_rail_turns_forbidden_by_the_current_game_rule.log`, `files/underground_workflow.xml` | 13D |
| WindowDesc - ini_key validity (`src/tests/test_window_desc.cpp:49`) | upstream | fail (alone fail, file order fail); Group 13: pass alone and in file order (upstream WindowDesc rule) | pass (Group 14 final suite) | S3 UI/content drift | `reruns/WindowDesc_-_ini_key_validity.log`, `files/test_window_desc.xml` | 13 |
| Common scheduler settles mixed native contention in original destination order (`src/tests/utility_scheduler_contention.cpp:133`) | utility | fail (alone fail, file order fail) | pass (Group 14 final suite) | I (T2 language pack) | `reruns/Common_scheduler_settles_mixed_native_contention_in_original_destination_order.log`, `files/utility_scheduler_contention.xml` | 10 |
| Integrated company transitions cannot revive old native provider rights (`src/tests/utility_scheduler_lifecycle.cpp:150`) | utility | fail (alone fail, file order not-reached) | pass (Group 14 final suite) | I (T2 language pack) | `reruns/Integrated_company_transitions_cannot_revive_old_native_provider_rights.log`, `files/utility_scheduler_lifecycle.xml` | 10 |
| Integrated source and riser loss preserve original histories and independent work (`src/tests/utility_scheduler_lifecycle.cpp:99`) | utility | fail (alone fail, file order not-reached) | pass (Group 14 final suite) | I (T2 language pack) | `reruns/Integrated_source_and_riser_loss_preserve_original_histories_and_independent_work.log`, `files/utility_scheduler_lifecycle.xml` | 10 |
| Native cable loss and repair preserve unrelated town and orbital work (`src/tests/utility_scheduler_lifecycle.cpp:50`) | utility | fail (alone fail, file order not-reached) | pass (Group 14 final suite) | I (T2 language pack) | `reruns/Native_cable_loss_and_repair_preserve_unrelated_town_and_orbital_work.log`, `files/utility_scheduler_lifecycle.xml` | 10 |
| Native provider binding captures and hashes ignore transport row order (`src/tests/utility_scheduler_lifecycle.cpp:16`) | utility | fail (alone fail, file order fail) | pass (Group 14 final suite) | I (T2 language pack) | `reruns/Native_provider_binding_captures_and_hashes_ignore_transport_row_order.log`, `files/utility_scheduler_lifecycle.xml` | 10 |
| Original interval depletion never republishes energy during consumer retries (`src/tests/utility_scheduler_lifecycle.cpp:129`) | utility | fail (alone fail, file order not-reached) | pass (Group 14 final suite) | I (T2 language pack) | `reruns/Original_interval_depletion_never_republishes_energy_during_consumer_retries.log`, `files/utility_scheduler_lifecycle.xml` | 10 |
| Unrelated portal loss preserves an imported native capsule reservation (`src/tests/utility_scheduler_lifecycle.cpp:81`) | utility | fail (alone fail, file order not-reached) | pass (Group 14 final suite) | I (T2 language pack) | `reruns/Unrelated_portal_loss_preserves_an_imported_native_capsule_reservation.log`, `files/utility_scheduler_lifecycle.xml` | 10 |

## Group 1 completed

Branch `mlm-mlv`, HEAD `169abb88`. The tree was clean. HEAD differs from the planning commit `7a66e52f` only in plan documents (`git diff --stat 7a66e52f HEAD -- . ':(exclude)docs'` is empty), so every line number in the plan is valid.

Toolchain: GCC 13.3.0, Ubuntu Clang 18.1.3, CMake 3.28.3, Ninja 1.11.1, Python 3.13.17, 4 cores. apt installed the CI-parity optional packages: `liballegro4-dev` 4.4.3.1, `libogg-dev` 1.3.5, `libopus-dev` 1.4, `libopusfile-dev` 0.12, `libsoxr-dev` 0.1.3. Both existing trees were reconfigured (`build/` uses `/usr/bin/c++`, `build-clang/` uses `/usr/bin/clang++`). Allegro, OpusFile and Soxr are now found, so `WITH_ALLEGRO`, `WITH_OPUSFILE` and `WITH_SOXR` are enabled. These are sources the research copy never compiled. Doxygen, vcpkg breakpad, FluidSynth and GRFCodec remain absent; all are optional. The ignore rules match: `/build*` covers the build trees, `/logs` covers the logs and `test-cache/.gitignore` covers the tooling folder.

Records: `group1-identity.txt`, `group1-toolchain.txt`, `group1-apt.log`, `configure-{gcc,clang}.log` (filtered) and `configure-{gcc,clang}.full.log`.

**Carry forward:** the new Allegro, Opus and Soxr sources are compiled for the first time in Group 5. Treat any error in them as a 5.3 item, not a research-copy regression.

## Group 2 completed

Items 2.1-2.7 were applied by `test-cache/linux-baseline/apply_group2.py` (anchored, exactly-once replacements; rerun it only on an unmodified checkout). The edits match the research copy, with explanatory comments added: each include names the symbol it serves, and each GCC-portable assignment explains the conditional-lvalue rule. Every touched file's header now also cites this plan. 2.8 compiled the six affected objects (`layer_recipe_work`, `surface_supply_order`, the three saveload handlers and the fixture consumer `space_preview`) in both `build/` and `build-clang/` with 0 failures (`group2-objects-{build,build-clang}.log`).

The 2.8 sweep (`group2-sweep.txt`) found no other conditional-lvalue braced assignment and no `std::exchange`/`std::as_const` user without `<utility>`. The 37 `std::filesystem` users without a direct `<filesystem>` include get it through project headers (`test_artifact_paths.h`, `language.h`, `fileio_func.h`, and now `layer_save_fixture.h`). The research copy compiled all of them with both compilers, so nothing is carried to Group 5.

**Commenting interpretation (user standard, 2026-10-27):** in upstream or fork files that this plan only touches, the standard applies to the header (a plan/architecture reference) and to every changed or added statement. Untouched existing code in those files is not rewritten.

## Group 3 completed

`test-cache/linux-baseline/apply_group3.py` moved `TBMExitJourney::Do` above the constructor (3.1). The doc comment and body are byte-identical, and one placement comment states the deduced-return rule. It then added the four `template class SlOrdinaryCargoPreview<…>` instantiations after the class (3.3), with a comment that ties them to the `SlStationSpecList` precedent. Both file headers now cite this plan. 3.2: `Do` is the class's only deduced-return member (`group3-deduced-members.txt`). 3.4: `vehicle_sl.cpp.o` and `tbm_exit.cpp.o` compile in both trees, with no warnings from the touched lines (`group3-objects-{build,build-clang}.log`).

3.5 diff summary (Groups 2-3, `git diff 169abb88 -- src`): 10 source files. The hunks are 7 header references, 3 includes, 3 reference bindings, 1 default member initializer, 1 member move and 1 explicit-instantiation block. No saved field, version gate, command, test assertion or behaviour changed.

## Group 4 completed

`test-cache/linux-baseline/apply_group4.py` rewrote the six `for (… : *GetLaunchCenterFootprint(…))` loops (4.1-4.6) into a named `footprint` local with `REQUIRE(footprint)`. Each rewrite carries a comment on the C++20 lifetime rule. `launch_center.cpp`'s two loops also gained business-logic comments: stamping the whole footprint as one facility, and a refused build leaving every cell clear. The other four already had one. `launch_center.cpp` had no plan reference in its header, so it now cites `r2mlvs9-launch-centers-and-rockets.md` and this plan. 4.8 compares against `uint32_t{CargoPacket::MAX_COUNT}` in `tbm_face_preflight.cpp` and `tbm_face_cycle.cpp`, with a comment on the odr-use, and leaves upstream `cargopacket.h` unchanged.

4.7 sweep (`group4-sweep.txt`): the only other range-for over a dereferenced call is upstream `src/core/pool_func.cpp:32` (`*PoolBase::GetPools()`), which dereferences a pointer to a static vector, not a temporary, so it is not an R6 case. Compile check: the 10 objects that include these sources (7 `openttd_test`, 2 `openttd_tube_test`, 1 `openttd_space_art_test`) compile in both trees, with 0 `-Wdangling-gsl` (`group4-objects-{build,build-clang}.log`).

## Group 5 completed

5.1 GCC `ninja -C build -j4 -k 0`: **0 failures** in 28 min 14 s (1,152 edges; objects from the validation run were reused). The new Allegro, Opus and Soxr sources compiled cleanly. Warnings (`build-gcc.warnings.txt`): 492 `-Wmissing-field-initializers` (down from 944; the rest come from other structs, for example `TBMFaceSnapshot::facts` in `tbm_face_fixture.h`), 333 `-Wrange-loop-construct`, 71 `-Wunused-function`, 25 `-Wsign-compare`, 13 `-Wmisleading-indentation` (11 in planning; 16.4 classifies them), 11 `-Wredundant-decls`, 1 `-Wunused-variable`, 1 `-Wtype-limits`.

5.2 Clang `ninja -C build-clang -j4 -k 0`: **0 failures** in 23 min 21 s, with `-Wdangling-gsl` 0. Warnings (`build-clang.warnings.txt`): 204 `-Wmissing-field-initializers` (down from 783), 71 `-Wunused-function`, 18 `-Wsign-compare`, 17 `-Wrange-loop-construct`, 1 `-Wdeprecated-enum-compare-conditional`.

5.3: no newly surfaced compile or link error in either tree. R8 (item 4.8) was the only link-time cause, as in the research copy.

5.4: `ctest -N` reports `Total Tests: 1359` in both trees, and the `POST_BUILD` discovery ran without error.

5.5: all nine focused executables built and linked in both trees (`test-cache/linux-baseline/build_focused.sh`, `group5-focused-summary.txt`). Each binary was deleted after linking to keep the disk allowance; the objects remain for Group 15. GCC: surface_supply_test 204 s, surface_capital_test 68 s, processor_contract_test 36 s, ground_art_test 35 s, ground_consumer_test 59 s, ground_viewport_test 37 s, tube_test 325 s, space_art_test 66 s, merge_contract_test 55 s. Clang: surface_supply_test 164 s, surface_capital_test 48 s, processor_contract_test 31 s, ground_art_test 30 s, ground_consumer_test 48 s, ground_viewport_test 28 s, tube_test 269 s, space_art_test 61 s, merge_contract_test 46 s.

5.6 (`group5-status.txt`): `git status --short` is empty, no tracked `media/**/*.grf` changed, and `newgrf/multilayer_industry.grf` (2,489 bytes) and `generated/table/multilayer_content_manifest.h` (2,923 bytes) exist only inside `build/` and `build-clang/`.

## Group 6 completed

6.1: `test-cache/linux-baseline/materialize_baseset.py build build-clang` wraps the repository's own `load_profile` and `prepare_runtime_assets` and materialized the seven retained original-DOS fixtures into both trees (`group6-materialize.log`). 6.2: all 14 destination files match the profile's SHA256s (`group6-hashes.txt`; `trg1.grf` is `38f0494a…`, `sample.cat` is `edb6233c…`). 6.3: `ctest --test-dir build -R '^regression_'` passed **4/4** (regression 4.7 s, stationlist 2.1 s, gs 0.6 s, gs_compat 0.5 s) with no OpenGFX and no network (`group6-regression-gcc.log`).

6.4 coupling, recorded for Groups 14-16: both trees' Catch tests copy `baseset/`, `newgrf/`, `scripts/` and `lang/` from `<repository>/build` (`GetTestRuntimeConfig`). `build/` is built and materialized, so `build-clang/` and `build-dedicated/` test runs are covered; `GetTestRuntimeConfig` is unchanged. 6.5: no wrapper-only case is registered with CTest. Four exist in `openttd_test` behind hidden tags (`[.m1-packed-art-load]` and its siblings); `ground_packed_load.cpp` is compiled only into a focused target (`group6-boundaries.txt`). 6.6: `mlm/reviews/`, `logs/` and `test-cache/` exist and are ignored, `mlm/runtime/` is ignored and will be created by the tests, and `mlm/reviews/README.md` stays tracked.

## Group 7 completed

7.1: the run folders `logs/linux-baseline/20261027-linux/` and `test-cache/linux-baseline/` already existed (Group 1). 7.2: CTest `Total Tests: 1359`, Catch lists 1,355 names, and `--list-tags` reports 581 tags (`group7-inventory.txt`, `tags.txt`). 7.3: `test-cache/linux-baseline/ledger.py` keeps the plan's logic and adds full parameter docs and business-logic comments. 7.4: `test-cache/linux-baseline/rerun_failed.sh`, likewise documented. The smoke run (`group7-smoke*`: 4 `WindowDesc` cases plus `regression_gs`) exposed one bug in 7.4: a relative output directory broke after the `cd` into the build tree. It is fixed with `realpath -m`. The smoke run also showed that `-s` reports **both** `WindowDesc` offenders (`launch_assembly_gui.cpp:95`, `asteroid_gui.cpp:187`) in one run, which confirms 13.1's two sites. 7.5: this file gained the commands header and the failure-ledger table.

## Group 8 completed

8.1 GCC baseline (the first of the two full-suite runs this execution allows): `97% tests passed, 37 tests failed out of 1359` in 35.0 s, with no time-outs (`ctest-gcc.log`, `ctest-gcc.xml`). 8.2 ledger (`ledger-gcc.tsv`): all 36 planned signatures appeared, plus one more.

**Self-inflicted regression, found and fixed here:** "Hauler cycle reports insufficient complete face room…" (`tbm_face_cycle.cpp:48`). Item 4.8's script had appended a `//` comment in the middle of the dense one-line statement at line 51, which turned the following `TBMFaceTransferRig::AddPacket(…)` call into comment text. The comment now sits on its own line. The file's 6 cases pass (85,148 assertions) after relinking `openttd_test`. Lesson for later groups: never append a `//` comment to a line that continues with code.

**Style correction:** CODINGSTYLE.md (line 307) asks for `/* */` after preprocessor directives, so the three Group 2 `#include` comments now use `/* … */`. This is comment-only; it recompiles with the next `openttd_test` build.

8.3 (`reruns/`, `reruns/index.txt`): all 36 planned failures reproduce alone. `rerun_failed.sh` now escapes `,` `[` `]` for Catch. Without the escaping, "A started cutter drives to its face, works its route…" matched no case and exited 0, a false pass; the plan's 7.4 note was corrected. 8.4 (`files/*.xml`, `group8-classification.tsv`): 21 failures pass in file order or are not reached after an in-process crash (category I). 15 fail in file order (category S). Both splits are identical to the research copy and the plan's signature table. 8.5: the classifications are in the ledger above (Clang column pending until Group 14). 8.6: `windows-check.txt` holds the 15 S names, escaped for `openttd_test.exe -f windows-check.txt`, to hand to the owner in Group 17.

**8.7 finding (category C, owner decision):** the full run rewrote 10 tracked fixtures (`group8-fixture-writes.txt`). The writers are `layer_processor_load.cpp:186` (processor `.sav` and `.sav.txt`; the text gets LF on Linux where the tracked bytes are CRLF), `layer_mine_load.cpp:115` (mine `.sav`) and `underground_save.cpp:110` (`native-fleet.sav`). All three build `current_path().parent_path() / "src/tests/fixtures/…"` and save into it, which contradicts `r2fc-file-cleanup.md` **Output and fixture contract** item 1 and CI's `git diff --exit-code`. The files were restored with `git checkout -- src/tests/fixtures/`. The plan gained **Group 13B** (Area block plus 6 tasks), whose first task asks the owner for the target behaviour. **Carry forward:** after every test run that includes these cases, check `git status --short -- src/tests/fixtures` and restore before committing.

## Group 9 completed

`test-cache/linux-baseline/apply_group9.py` inserted `MockEnvironment::Instance();`, with a comment, as the first statement of the nine T1 cases: `transport_tube_recovery.cpp` 73/87/104/116, `transport_tube_construction.cpp` 158/226, `transport_tube_vehicle_sprite.cpp` 82 and `transport_tube_facility_sprite.cpp` 59/158. Each file's header now cites this plan. 9.5: `openttd_test` rebuilt in 6 min 57 s with 0 failures (`group9-build.log`). That build also compiled Group 8's include-comment change. Each of the nine cases then ran alone in a fresh process, as CTest runs it (`group9-reruns/`). **All nine pass**, every log showing "All tests passed (… in 1 test case)" (44 to 4,498 assertions). No tracked fixture was written.

## Group 10 completed

`test-cache/linux-baseline/apply_group10.py` applied 10.1-10.6, each with an explanatory comment and a header reference to this plan:
- T2: the six `utility_scheduler_lifecycle.cpp` cases and the `utility_scheduler_contention.cpp:133` case call `LoadGeneratedWindowTestLanguage()` right after `MockEnvironment::Instance()`, and both files include `layer_window_fixture.h`.
- T3: `geothermal_collector.cpp` and `geothermal_riser_lifecycle.cpp` set `AutoRestoreBackup local(_local_company, CompanyID::Invalid())` before `ChangeOwnershipOfCompanyItems`.
- T4: `layer_recipe_batch.cpp` 220/242/274 construct `ExcavationFixture world;`.
- 10.3: `geothermal_town_service_fixture.h` is unchanged (`git diff 169abb88` is empty).

**Second instance of the trailing-comment trap, caught before any build:** the first script version appended a `//` comment to the shared first line of the six lifecycle cases, which would have commented out `UtilitySchedulerMixedRig rig{…}`. The comment now sits on its own line, and the script is fixed. The guard `test-cache/linux-baseline/check_swallowed_code.py <base>` scans every added `src` line for a `//` comment followed by statement-shaped code. It was checked against both real bug lines and two prose lines, and reports **0** for `169abb88..working tree`. **Carry forward:** run the guard after every scripted edit.

Verification (focused, per the test policy; 10.7's full suite is replaced by this): `openttd_test` rebuilt in 36 s with 0 failures. All 12 Group 10 cases pass alone in fresh processes (`group10-reruns/`, 8 to 2,710 assertions each). In file order, the five edited files and the three files the rejected 10.3 variant once broke (`geothermal_town_service`, `geothermal_town_recovery`, `utility_controls_window`) all pass. No tracked fixture was written.

## Group 11 completed

**Entry gate:** Groups 9-10 cleared all 21 I rows, so the 15 S rows remain (Group 8 split, confirmed again in file order below). `windows-check.txt` is ready; it goes to the owner in Group 17. Any test the owner reports passing on Windows is then reclassified as P.

**11.1 causal proof:** `bfba42fd` (2026-10-09, "complete TBM fixes") changed `_cut_terms` from `{{{6, 12, 1, 1, 30}, {4, 24, 2, 1, 50}, {2, 40, 3, 2, 80}, {1, 48, 0, 4, 120}}}` to `{{{3, 6, 1, 1, 30}, {2, 12, 2, 1, 50}, {1, 20, 3, 2, 80}, {1, 24, 0, 4, 120}}}`. In the same commit it updated sibling tests, for example `earth_t == 18 → 9` and `rock_t == 36 → 18`. The stated intent is `r2tbmf-tbm-fixes.md` line 37 (**Whole-unit decision, 2026-10-09**) and the comment above `_cut_terms` ("R2TBMF Group5B … all other Earth/Rock yields halve"). The scratch-copy differential was already run in the research build (plan **Research build**), so it was not repeated. The junction case first appears in that same commit (r2tbmf Group 4). It was presumably validated before task 5B.1 halved the yields; this is an inference from the plan order, not a recorded run. `0119308a` (2026-10-25) later lengthened the Command area from 7 to 14 tiles, which still yields no early bay exchange.

**How the halving breaks the four route cases:** halved Soft yields move each route's first breakpoint later. By then the consist stands clear of its housing, so `GetTBMMeetingKind` picks a face meeting (`AwaitFaceTransfer`, r2df2) instead of a bay return.
- The bend reload no longer has a breakpoint after the bend.
- The started cutter's bend no longer overflows.
- The junction cutter never drives back to its face.
- The whole-life machine waits at its face.

**Fixes (one test file each, no production change):**
- 11.2: `excavation_outline.cpp` now expects `earth_t == 3` and `rock_t == 6`.
- 11.3: `tbm_engine.cpp` declares the sealed pocket Mixed (`fixture.world.DeclareRockSeam(…, GeologyHardness::Mixed)`) before the proof. The twelve door cells hold 144 t of Rock again, so every `InsufficientCapacity` refusal and the no-mutation checks are exercised.
- 11.4-11.7: the bend reload, the started cutter (all three sections), the junction hold and the whole-life controls each declare `const TBMRigRock mixed_rock{GeologyHardness::Mixed};` before their rig.

These use the repository's own idiom for this situation, not the plan's literal suggestions. `ExcavationFixture::DeclareRockSeam` documents it: "Focused cases that need a capacity refusal or a mid-route exchange therefore declare harder rock … Mixed rock yields 12 t Rock per cell, the former Soft Rock total, so routes keep their geometry and breakpoints". `tbm_exchange_acceptance.cpp` already uses it three times. The plan's suggestions were: harder door cells (Hard), moving the phase assertion, lengthening routes, or adding a player action. The idiom keeps every assertion, including the frozen-breakpoint-after-reload check, without lengthening the shared `SourceLoop` Command route that three other `tbm_movement.cpp` cases use.

**Group 11 finding (category C, owner decision):** a machine in `AwaitFaceTransfer` with no assigned hauler never prompts the player. `GetTBMLogisticsWait` (`src/tbm_exchange.cpp:1339`) returns `NoHauler` only for `IsTBMReturning` or `IsTBMWaitingForExchange` phases, so `ChooseTBMProjectStep` (`src/tbm_controls.cpp:201`) shows `Working` while the face wait can never end. Evidence: the Group 8 Soft-rock run of the whole-life case, `reruns/A_project_s_whole_life_can_be_played_from_the_portal_window_s_controls.log`, shows "after 20000 ticks, step 2: phase 20 blocked 0 cursor 11". The proposed one-line patch adds `|| IsTBMAwaitingFaceTransfer(project.phase)` to that condition. `NoHauler` is not a stall reason (`IsTBMLogisticsStalled`), so only the controls would change. The plan gained **Group 13C**, which asks the owner together with 13B.1. The whole-life case on Mixed rock still covers the bay-side prompt.

**Verification (focused, per the test policy; 11.8's full suite is replaced by this):**
- `openttd_test` rebuilt in 39 s with 0 errors (`group11-build.log`).
- The six cases pass alone in fresh processes (`group11-reruns/`, 22 to 125,550 assertions).
- In file order (`group11-file-<stem>.xml`), these pass: `excavation_outline` (2 cases), `tbm_engine` (4) and `tbm_coverage` (6). `tbm_movement` passes 14 of 15. The one failure is "A TBM leaves its work hold for a real depot visit…", the S2 row owned by 12.5, as before.
- `check_swallowed_code.py eead866e` reports 0. No tracked fixture was written.

## Group 12 completed

Every edit below cites the production commit and the stated intent that changed the behaviour (the Groups 11-13 entry gate).
- **12.1-12.4**: `test-cache/linux-baseline/apply_group12.py` applied the four research-validated edits, each with a comment and a header reference.
  - **12.1** (`tbm_project.cpp`): the travel order hands the project to `FollowOrders`. Cause: `bfba42fd` `FollowTBMVehicleOrders`. Intent: `r2tbmf-tbm-fixes.md` requirement 8 and the line 52 arbitration rule. Pause and Resume now compare against the phase and revision captured after that order (`revision + 1`, `+ 2`).
  - **12.2** (`tbm_corridor.cpp`): a train standing on released support keeps it in use. Cause: `bfba42fd` `IsTBMSupportInUse`, "Neither a hidden tail … becomes convertible". The check is inverted, and `CHECK_FALSE` after withdrawal keeps the release side.
  - **12.3** (`tbm_commission.cpp`) and **12.4** (`spoil_receiver_stall.cpp`): the halved cutting rate. Cause: `bfba42fd` r2tbmf Group 5 (5.1, Standard 1/1 → 1/2, Deep 3/2 → 3/4), with save version `TBMHalfCuttingRate`. The first-cut loop is bounded by the model's own `CeilDiv`, and resumed work is checked over one whole rate period.
- **12.5** (`tbm_movement.cpp`, depot visit): the cause is `bfba42fd` (r2tbmf Group 8, selected hangars), not `0119308a` as planned. A per-tick trace and gdb breakpoints on the phase step (temporary, reverted) showed two things:
  - **Empty halt.** It parks in `ParkedHangar`, and the controller re-stops a stored machine on every step (`tbm_project.cpp:1305`) until the player's explicit Resume. `CmdControlTBMProject` Resume also starts the stored machine itself, so the old `StartStopVehicle` after it toggled the machine back to stopped. The halt sections now require `ParkedHangar`, then Resume, then a running machine.
  - **Loaded halt.** `BeginTBMHangarReturn` stages the spoil through the exchange bay (`ReturnForHangar` → `WaitForHangar`, "Every attached packet must first return through its existing exchange owner"). `TBMMovementRig` has no hauler, so the cutter waits in its bay. The case now requires that wait for 200 more ticks, with the job and every packet intact. The hauler-emptied continuation is covered by `tbm_face_lifecycle.cpp` "A hangar order from the face wait" (r2df2 Group 7).
  - All 3 sections × 2 loads pass (24,614 assertions).
- **12.6** (`tbm_hangar.cpp`): the refusal `STR_ERROR_TBM_COMMISSION_SUPPORT_PIECE` at tile 5200 is deliberate. Cause: `0119308a`, r2df task 7.3: the quote applies the cuts' own support rule (`FindTBMSupportRouteBlocker`), and the company's own permanent track lacking only a piece is "fundable". The plan's suggestion to choose another area does not fit, because every route from B's lower pair crosses Prepare's freight lane at the door. The test now follows the fixture's own `FundFurtherRoute` idiom:
  - It accepts a fundable refusal of the first quote.
  - It builds the missing pieces from the refused route's geometry. The test already did this, but after the quote.
  - It quotes again, and requires cutter reuse at zero cost on that final quote, the one the confirmation uses.
- **12.7** (`underground_workflow.cpp`): **category C, owner decision**. A temporary scan (reverted) quoted 21,316 rectangles with the estimate-only `CmdCommissionTBM`: start at `site ± 3`, end within ±10 of it. **None** was refused with `STR_ERROR_TBM_COMMISSION_RAIL_TURNS`. Counts: 15,947 accepted, 2,560 approach too far, 2,507 support refused, 210 map edge, 92 other (`group12-turn-scan.txt`, full output `group12-turn-scan-full.log`). The planner changes in `bfba42fd` and `0119308a` now avoid forbidden bends, so the UI refusal is unreachable. The test is unchanged and still fails. The plan gained **Group 13D**, which asks the owner together with 13B.1 and 13C.1.

Verification (focused, per the test policy; 12.8's full suite is replaced by this):
- `openttd_test` rebuilt in 35 s with 0 errors (`group12-build.log`).
- Alone (`group12-reruns/`): six of the seven pass with 39 to 34,435 assertions; 12.1-12.4 match the research copy's counts exactly. The 12.7 case fails, as recorded.
- In file order (`group12-file-<stem>.xml`), all pass: `tbm_project` (5 cases), `tbm_corridor` (2), `tbm_commission` (7), `spoil_receiver_stall` (4), `tbm_movement` (15) and `tbm_hangar` (7). `underground_workflow` passes 4 of 5, failing only the 12.7 case.
- `check_swallowed_code.py c86c901c` reports 0. No tracked fixture was written.

**Carry forward:** the remaining failures before Group 13 are the 2 S3 rows and the 12.7 C row. Three owner decisions are queued for one prompt at 13B.1: fixture producers, the face-wait prompt, and rail-turn coverage.

## Group 13 completed

- **13.1** (production, two lines): `launch_assembly_gui.cpp` and `asteroid_gui.cpp` declare `WWT_STICKYBOX` but registered an empty INI key. The sticky boxes and empty keys date from `77f8b8fd` (2026-10-12, `asteroid_gui.cpp`) and `7db9cc09` (2026-10-13, `launch_assembly_gui.cpp`). Upstream's `WindowDesc - ini_key validity` requires a sticky or default-size box exactly when an INI key exists (`test_window_desc.cpp:49-61`). They now register `"launch_assembly"` and `"space_survey"`, so their sticky state and size persist in `windows.cfg`. Both keys are new; the uniqueness case in the same file passes. **Alternative for the owner:** remove the two sticky boxes instead (`launch_assembly_gui.cpp:39`, `asteroid_gui.cpp:167`). That also satisfies the rule, but the windows could then no longer be pinned.
- **13.2** (`multilayer_control_selection.cpp`): since `75cf506b` (ground art), `GetMultiLayerPurchaseSprite` consults `GetGroundRailPurchaseSprite` and then `GetGroundRoadPurchaseSprite` before the five-model M1 slice. Model 5 is now a published ground model. The case now requires that model 5 has art and that the art comes from the ground owner. The "no art outside every published slice" property moved to `UINT16_MAX`. The test includes `ground_train_sprite.h` and `ground_road_sprite.h`.
- **13.3** (focused, per the test policy): `openttd_test` rebuilt in 32 s with 0 errors (`group13-build.log`).
  - Both cases pass alone (`group13-reruns/`, 192 and 35 assertions).
  - In file order, `test_window_desc` passes 4 cases (877 assertions) and `multilayer_control_selection` 4 cases (140 assertions).
  - `git status --short` lists only the three intended files. `check_swallowed_code.py 9b800280` reports 0.
  - The full GCC suite moved into 14.1 as the end-of-plan regression; the plan was updated.

**State after Groups 9-13:** of the 37 Group 8 failures, 36 are fixed and verified by focused runs. One remains: the 12.7 C row, pending the owner's decision in Group 13D.

## Group 13B completed

**13B.1 (owner, 2026-10-27):** a single prompt asked all three queued decisions, and the owner chose the recommended option each time:
- 13B (a): route the producers to run-owned outputs.
- 13C (a): `AwaitFaceTransfer` without a hauler prompts `AssignHauler`.
- 13D (a): cover the rail-turn refusal with a lower-level `CheckTBMSupportRailTurns` case.

**13B.2-13B.4:** `test-cache/linux-baseline/apply_group13b.py` made the edits. Each producer now saves under `GetTestArtifactPaths(<feature>, <scenario>).saves`. Each reuses the feature and scenario its Python checker already declares in `mlm/review-producers.json`:
- `layer_processor_load.cpp` uses `processors/layer-processor-save-fixture`, and writes the `.sav.txt` checkpoint beside its save.
- `layer_mine_load.cpp` uses `mines/layer-mine-save-fixture`.
- `underground_save.cpp` uses `vehicles/underground-save-fixture`.

Every assertion and the checkpoint text are unchanged, and each file header cites this plan. The catalogue gained one entry per native producer, inserted directly before its checker. The file round-trips exactly through `json.dumps(indent=2)`, so only those 42 lines changed.

**13B.5:** unchanged. The Python checkers read the committed fixtures as immutable inputs: `multilayer_state_fixture.save_path` resolves history unless the same process reserved an output. They never read the producers' fresh files.

**13B.6:**
- `openttd_test` rebuilt in 4 min 31 s with 0 errors (`group13b-build.log`).
- In file order, `layer_processor_load` passes 3 cases (2,164 assertions), `layer_mine_load` 3 (495) and `underground_save` 1 (389).
- All 10 outputs landed under the git-ignored `mlm/reviews/<feature>/<scenario>/<run>/saves/`. `git status --short -- src/tests/fixtures` is empty.
- **The Group 8 carry-forward restore step is no longer needed.**

## Group 13C completed

**13C.2 (production, owner option a):** `GetTBMLogisticsWait` (`src/tbm_exchange.cpp`) now also returns `NoHauler` for `IsTBMAwaitingFaceTransfer` when no hauler is assigned. The comment states the rule: none of the three waits can end until the player assigns a hauler. The file header cites this plan. `IsTBMLogisticsStalled` does not count `NoHauler`, so only the controls change.

The new regression case is "A face wait without a hauler asks the player to assign one" (`tbm_movement.cpp`, `[tbm-controls]`). It plays the whole-life opening on halved Soft rock, with no `TBMRigRock`, until the first `AwaitFaceTransfer`. It then requires `NoHauler`, `AssignHauler` and the assign control.
- **Before the fix it failed** (`group13c-before-fix.log`): after 7,224 ticks, phase 20 at cursor 11, wait `None` (0) and step `Working` (2). This reproduces the Group 11 finding.
- **After the fix it passes** (`group13c-after-fix.log`, 7,304 assertions).

**13C.3:** `openttd_test` rebuilt in 29 s with 0 errors (`group13c-build.log`). In file order (`group13c-files.txt`), all six files that read the wait pass:
- `tbm_movement`: 16 cases.
- `spoil_receiver_stall`: 4.
- `tbm_exchange_acceptance`: 4.
- `tbm_face_cycle`: 6.
- `tbm_face_regression`: 3.
- `tbm_face_meeting`: 3.

`check_swallowed_code.py fd7830b5` reports 0.

## Group 13D completed

**Cause, now documented:** the owner's imported r2df notes record this change on the `openttd3-75ea75718f` line (`docs/zz-rhr/imports/openttd3-75ea75718f/docs/zz-reach2/upgrades/2026-10/r2df-a-reproduction-notes.md`, around line 270). The planner was changed deliberately:
- "A start pose whose whole trail stands on its own tile now leaves straight on."
- `GetTBMWorkRailPath` passes the other side cell when its usual one would form a forbidden 90-degree bend.
- "A scratch enumeration shows a legal choice always exists."

The same notes changed the test: "`[underground-workflow-turn-refusal]` became `[underground-workflow-turns]`: the chamber is now accepted with drivable support." So the 12.7 row is S2 (deliberate planner change), and 13D aligns with that precedent.

**13D.2 (owner option a):**
- **New case** "Support rail turns follow the game's 90-degree rule" (`tbm_support.cpp`, `[tbm-support]`). A temporary probe was reverted afterwards; it planned every legal 1-3-cut heading sequence through `AdvanceExcavationPlanningPose`. It found that a single first cut turning 45 degrees at the door is the minimal forbidden bend: the housing's axis track meets a crossing piece. The case requires that bend to be refused with `pf.forbid_90_deg` on and allowed with it off, and the straight exit allowed both ways.
- **UI case renamed** "Visible chamber quote plans only rail turns the current game rule allows" (`[underground-workflow-turns]`). It now requires:
  - the rule is on;
  - the confirm button is enabled and the status has no "90-degree" warning;
  - the quote succeeds, and its route passes `CheckTBMSupportRailTurns`;
  - the estimate succeeds;
  - the four "nothing changed" checks hold: no project, the same vehicle count, cash and geology digest.

  The `Execute` call was dropped: it would now legitimately buy the machine, and the owner asked to keep the UI case for its "nothing changed" checks.

**13D.3:** `openttd_test` rebuilt in 34 s with 0 errors (`group13d-build.log`). Both cases pass alone, with 18 and 49 assertions. In file order, `underground_workflow` passes 5 cases and `tbm_support` 7. `check_swallowed_code.py 0536c15d` reports 0.

**State before Group 14:** all 37 Group 8 failures are fixed and verified by focused runs. No owner decisions remain open. The end-of-plan GCC regression (14.1) is next.

## Group 14 completed

**14.1 (end-of-plan regression, deferred from 13.3):**
- **GCC:** `ninja -C build -j4 -k 0` (22 s, 0 failures, 0 warnings, `group14-build-gcc.log`). Then the full CTest run: **`100% tests passed, 0 tests failed out of 1361`** in 33.1 s (`ctest-gcc-final.log`, `ctest-gcc-final.xml`, `ledger-gcc-final.tsv`). The two cases added in 13C and 13D bring the count from 1,359 to 1,361. `git status --short` was empty before and after.
- **Clang:** `ninja -C build-clang -j4 -k 0` rebuilt 177 steps in 6 min 35 s, with 0 errors and 110 warnings (`group14-build-clang.log`). None of the warnings is on a line this execution changed. The only edited files that warn, `launch_center.cpp` (-Wsign-compare at 495-562) and `launch_production.cpp:120` (-Wmissing-field-initializers), warn outside the edited hunks; Group 16 classifies them. Then the full CTest run: **`100% tests passed, 0 tests failed out of 1361`** in 28.2 s (`ctest-clang.log`, `ctest-clang.xml`, `ledger-clang.tsv`). The tree was unchanged afterwards.

**14.2:** the ledger diff by name and status is empty (`group14-ledger-diff.txt`; 1,361 names in each). **14.3:** therefore no category X test and nothing to fix. **14.4:** `ctest --test-dir build-clang -R '^regression_'` passes 4 of 4 (`group14-clang-regression.log`), so Group 6's content materialization holds for this tree. **14.5:** both compilers read `100% tests passed, 0 tests failed out of 1361`. The ledger's Clang column now reads "pass" for all 37 Group 8 rows.

The full regression ran once here, as the user's test policy allows. Group 8 was the only earlier full run.

## Group 15 completed

**15.1-15.2 (GCC, `group15-gcc/`):** `test-cache/linux-baseline/run_focused.sh` builds, runs and deletes each target in turn. Eight targets passed at once (rc=0):

| Target | Cases | Failures | Note |
| --- | --- | --- | --- |
| `openttd_surface_supply_test` | 176 | 0 | The research copy's 2 failures (12.3, 12.7) are fixed |
| `openttd_surface_capital_test` | 2 | 0 | |
| `openttd_processor_contract_test` | 9 | 0 | |
| `openttd_ground_art_test` | 0 | 0 | Only hidden `[.…]` cases (for example `[.ground-packed-art-load]`); a default run selects none |
| `openttd_ground_consumer_test` | 66 | 0 | |
| `openttd_ground_viewport_test` | 0 | 0 | Only hidden cases, as above |
| `openttd_space_art_test` | 68 | 0 | |
| `openttd_merge_contract_test` | 59 | 0 | |

`openttd_tube_test` died with rc=139 and an empty JUnit file. The console rerun completed: 429 cases, 427 passed, and 2 failed, exactly the two 15.3 leaks. The research copy's other 5 tube failures (11.5-11.7, 12.5, 12.7) are fixed.

**15.3 (`bisect_leak.sh`, `pair_scan.sh`):** the bisection names the first case that completes a leak. Its first version printed the pair result inverted ("passes" meant "fails"); that is fixed, and both `result.txt` files are annotated. Because one leak had several sources, `pair_scan.sh` then ran every earlier case paired with the victim, 0.1 s per run:
- **Packets, 221.** "Paid Surface packets enter the actual powerhouse construction owner…" (`geothermal_generator_delivery.cpp:141`, `CargoPacket::GetNumItems() == 0`) has 32 individual leakers. All are TBM rig cases at positions 162-267, with 1 to 38 packets each, summing to exactly 221 (`scan-geothermal/leakers.tsv`).
  - Cause: OpenTTD's `Vehicle` and `Station` destructors only forget their packet lists while their pool is being cleaned (`CargoList::OnCleanPool`), because a whole-game clean also empties the packet pool.
  - `TBMEngineFixture` cleaned the vehicle and station pools but never `_cargopacket_pool`. Several other fixtures carry comments claiming `_vehicle_pool.CleanPool()` "releases" packets, which it does not. They leak only packets nobody counts, so they were left alone as follow-ups.
- **Money, 3.** "Tube interchange geometry refuses cross-layer station identity" (`transport_tube_interchange_geometry.cpp:138`) has one leaker: "Two driven source haulers save an occupied FIFO turn and its next legal exchange" (`tbm_logistics_queue.cpp:36`). A temporary money trace, reverted, and a gdb breakpoint on `RecomputePrices` showed the mechanism:
  - `UndergroundWorkflowWorld` with `paid_setup` calls `RecomputePrices()` and never restores the test world's zero prices.
  - Under non-zero prices the victim's direct `CmdBuildRailStation` and `CmdBuildRoadStop` calls charge 2 and 1. Called outside the command framework, their inner sub-commands count as top-level.
  - **Not a production defect:** through `Command<>::Post` a refused command charges nothing.

**15.4 (fixed at the source, no assertion changed):**
- `TBMEngineFixture` also calls `_cargopacket_pool.CleanPool()`.
- `UndergroundWorkflowWorld` holds `AutoRestoreBackup<Prices>` and `AutoRestoreBackup<Economy>` members.
- Both pairs now pass, and the whole tube executable passes 429 of 429 cases (4,129,821 assertions, 29 s).

**JUnit crash (rc=139), root cause found:** a gdb backtrace showed `CumulativeReporterBase<JunitReporter>::assertionEnded` dereferencing an empty section stack. The assertion came from `LoadGeneratedWindowTestLanguage()`, which `tube_test_environment.cpp`'s listener calls in `testRunStarting`, before any test case. The listener now uses its own non-asserting load, which throws a `std::runtime_error` with a clear message if the pack is missing. The JUnit run then completes: rc=0, 4,129,817 assertions (the listener's 4 `REQUIRE`s are gone), 429 cases. Commit `577a7cab`.

**15.5 (Clang, `group15-clang/`):** all nine targets pass (rc=0), including `openttd_tube_test` under JUnit (207 s, build included). Every per-target ledger is identical to GCC's by name and status.

**Carry forward:** `tbm_fixture.h` and `underground_workflow_fixture.h` are also compiled into `openttd_test`. The plan's final GCC CTest run in Group 17 re-verifies it, the one remaining full regression the user's test policy allows at the end.

## Group 16 completed

**16.1:** `build-dedicated/` was configured as CI's "GCC - Dedicated" row: `-G Ninja -DOPTION_DEDICATED=ON -DCMAKE_CXX_FLAGS_INIT="-DRANDOM_DEBUG" -DCMAKE_DISABLE_PRECOMPILE_HEADERS=ON`, no build type (`configure-dedicated.log`). `ninja -j4 -k 0` built all 1,530 steps in 22 min 34 s. Every translation unit compiled, so the disabled PCH exposed **no missing include**. The only failure was the `openttd_test` link (`build-dedicated.log`).

**16.2:**
- **The link failure:** `undefined reference to CargoPacket::MAX_COUNT` from `surface_supply_order_owner.cpp:129`. This is root cause R8 again, at an unoptimised site: `CargoPacket::Create()` forwards its arguments by reference, which odr-uses the in-class constant. The fix passes `uint16_t{CargoPacket::MAX_COUNT}` (`58c21e51`). After it, the tree builds completely (`build-dedicated-2.log`).
- **Re-verification:** `build/` and `build-clang/` rebuilt with 0 failures (388 s and 293 s, `group16-reverify-*.log`). Both edited files pass in all three trees: `surface_supply_order_owner` 15 cases (1,237 assertions), and the network case below.

**16.3:** `ctest --test-dir build-dedicated -j4 --timeout 600` reported `99% tests passed, 5 tests failed out of 1360` in 182.5 s (`ctest-dedicated.log`, `ctest-dedicated.xml`, `ledger-dedicated.tsv`). It runs one test fewer than the other trees: upstream's "ICU ScriptRun" is compiled only with ICU text layout, which the dedicated build lacks. New failures:
- **E (environment), 4:** `regression_regression`, `regression_stationlist`, `regression_gs` and `regression_gs_compat` failed with "Failed to find a graphics set". Running Group 6's `materialize_baseset.py build-dedicated` fixed them: 4 of 4 pass (`group16-dedicated-regression.log`). The tracked tree stays clean.
- **RANDOM_DEBUG-only, 1:** "Merged native network identity refuses original and stale protocol peers" (`merged_command_compatibility.cpp:58-59`, added in `0119308a`).
  - With `RANDOM_DEBUG`, `network_internal.h` enables `ENABLE_NETWORK_SYNC_EVERY_FRAME` and `NETWORK_SEND_DOUBLE_SEED`, so the build names itself `dbg_seed-…`. Upstream's `IsNetworkCompatibleVersion` accepts only peers of the same debug flavour.
  - The case hard-coded a standard build's alias pair. It now derives the flavour from its own revision string and checks the same accept/refuse pair (`02ec6b10`). In a standard build the two checks are exactly the old ones.
  - It passes in all three trees. Production is correct.
- **None was Debug-only or dedicated-only** beyond the excluded ICU case.

**16.4:** the 13 GCC `-Wmisleading-indentation` sites come from the Group 5 log. The plan listed 11 and missed `saveload/space_manifest_sl.cpp:192` and `saveload/space_production_sl.cpp:169`. **All 13 are intentional dense style; no code changed:**
- **Guard-then-continue (10 sites):** `asteroid_gui.cpp:317`, `launch_assembly_gui.cpp:239,255` (now 318, 241 and 257 after Group 13's header lines), `space_manifest.cpp:569`, `space_production.cpp:128,131`, `space_transition.cpp:80,348` and `space_manifest_sl.cpp:192`. In each, the guard leaves via `continue` or `return`, or calls `[[noreturn]] SlErrorCorrupt` (`saveload_error.hpp:17`). The next statement is reached only on the good path, as intended.
- **Ensure-then-use (1):** at `space_transition.cpp:139`, `if (!v->transition) v->transition = …; auto &f = *v->transition;` must bind `f` in every case.
- **After-loop statements (2):** at `space_production_sl.cpp:169`, `records.push_back(std::move(r))` runs once after its `for`. At `tests/space_resupply.cpp:203`, the two `CHECK`s run once after the summing loop.
- **`tests/manufacturing_clone_native.cpp:292`:** `ShowSpaceConstructionWindow` must run for all three actions, because the next line `REQUIRE`s the window.

**16.5, warning counts per flag.** Raw counts are compiler output lines, as in 5.1 and 5.2; unique counts are distinct file:line:column sites. GCC and Clang are their Group 5 full builds. Later builds were incremental and added no warning on an edited line (checked in Group 14). Dedicated is this group's full build.

| Flag | GCC raw / unique | Clang raw / unique | Dedicated raw / unique |
| --- | --- | --- | --- |
| `-Wmissing-field-initializers` | 492 / 191 | 204 / 191 | 498 / 192 |
| `-Wrange-loop-construct` | 333 / 221 | 17 / 17 | 340 / 227 |
| `-Wunused-function` | 71 / 5 | 71 / 5 | 76 / 5 |
| `-Wsign-compare` | 25 / 22 | 18 / 15 | 32 / 29 |
| `-Wmisleading-indentation` | 13 / 13 | - | 13 / 13 |
| `-Wredundant-decls` | 11 / 7 | - | 11 / 7 |
| `-Wstringop-overflow=` | 6 / 2 | - | - |
| `-Wunused-variable`, `-Wtype-limits`, `-Wenum-compare` | 1 / 1 each | - | 1 / 1 each |
| `-Wmaybe-uninitialized`, `-Warray-bounds=` | 1 / 1 each | - | - |
| `-Wdeprecated-enum-compare-conditional` | - | 1 / 1 | - |

How these compare with the planning baseline:
- R7 (2.7) removed about 450 GCC and about 580 Clang `-Wmissing-field-initializers` lines (944 → 492 and 783 → 204). The rest come from other aggregates.
- GCC's optimiser-only warnings (`-Wstringop-overflow=`, `-Wmaybe-uninitialized`, `-Warray-bounds=`) do not appear at `-O0`. The no-PCH, Debug and dedicated-specific translation units add a few repeats of the same sites.
- `-Wrange-loop-construct`, `-Wunused-function` and `-Wsign-compare` stay as recorded debt, with no bulk edits, as the plan requires.
