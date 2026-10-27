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
| Excavation outline extra work is explicit charged and read-only (`src/tests/excavation_outline.cpp:43`) | excavation | fail (alone fail, file order fail); Group 11: pass alone and in file order | pending (Group 14) | S1 halved yields | `reruns/Excavation_outline_extra_work_is_explicit_charged_and_read-only.log`, `files/excavation_outline.xml` | 11 |
| Geothermal interrupted collector holds construction and exclusive source through abandonment (`src/tests/geothermal_collector.cpp:194`) | geothermal | fail (alone fail, file order pass) | pending (Group 14) | I (T3 local company) | `reruns/Geothermal_interrupted_collector_holds_construction_and_exclusive_source_through_abandonment.log`, `files/geothermal_collector.xml` | 10 |
| Native riser abandonment retains material custody and cannot regain readiness (`src/tests/geothermal_riser_lifecycle.cpp:79`) | geothermal | fail (alone fail, file order pass) | pending (Group 14) | I (T3 local company) | `reruns/Native_riser_abandonment_retains_material_custody_and_cannot_regain_readiness.log`, `files/geothermal_riser_lifecycle.xml` | 10 |
| Recipe planner checks integer limits before complete commit (`src/tests/layer_recipe_batch.cpp:274`) | layer | fail (alone fail, file order pass) | pending (Group 14) | I (T4 recipe world) | `reruns/Recipe_planner_checks_integer_limits_before_complete_commit.log`, `files/layer_recipe_batch.xml` | 10 |
| Recipe quotes bind their exact owner state and complete deltas (`src/tests/layer_recipe_batch.cpp:242`) | layer | fail (alone fail, file order pass) | pending (Group 14) | I (T4 recipe world) | `reruns/Recipe_quotes_bind_their_exact_owner_state_and_complete_deltas.log`, `files/layer_recipe_batch.xml` | 10 |
| Shared recipe commit preserves exact all-input deltas and refuses a retry (`src/tests/layer_recipe_batch.cpp:220`) | layer | fail (alone fail, file order pass) | pending (Group 14) | I (T4 recipe world) | `reruns/Shared_recipe_commit_preserves_exact_all-input_deltas_and_refuses_a_retry.log`, `files/layer_recipe_batch.xml` | 10 |
| Native purchase previews resolve actual engines and both availability states (`src/tests/multilayer_control_selection.cpp:140`) | m1 | fail (alone fail, file order fail) | pending (Group 14) | S3 UI/content drift | `reruns/Native_purchase_previews_resolve_actual_engines_and_both_availability_states.log`, `files/multilayer_control_selection.xml` | 13 |
| A blocked hauler pauses new cutting until its route is found (`src/tests/spoil_receiver_stall.cpp:56`) | spoil | fail (alone fail, file order fail) | pending (Group 14) | S2 TBM rate/phase/support/planner | `reruns/A_blocked_hauler_pauses_new_cutting_until_its_route_is_found.log`, `files/spoil_receiver_stall.xml` | 12 |
| A TBM holds at a junction until its actual work route is restored (`src/tests/tbm_movement.cpp:694`) | tbm | fail (alone fail, file order fail); Group 11: pass alone and in file order | pending (Group 14) | S1 halved yields | `reruns/A_TBM_holds_at_a_junction_until_its_actual_work_route_is_restored.log`, `files/tbm_movement.xml` | 11 |
| A TBM leaves its work hold for a real depot visit and returns to the face (`src/tests/tbm_movement.cpp:256`) | tbm | fail (alone fail, file order fail) | pending (Group 14) | S2 TBM rate/phase/support/planner | `reruns/A_TBM_leaves_its_work_hold_for_a_real_depot_visit_and_returns_to_the_face.log`, `files/tbm_movement.xml` | 12 |
| A TBM releases at the remote hangar and commissions its next job there (`src/tests/tbm_hangar.cpp:308`) | tbm | fail (alone fail, file order fail) | pending (Group 14) | S2 TBM rate/phase/support/planner | `reruns/A_TBM_releases_at_the_remote_hangar_and_commissions_its_next_job_there.log`, `files/tbm_hangar.xml` | 12 |
| A project's whole life can be played from the portal window's controls (`src/tests/tbm_movement.cpp:875`) | tbm | fail (alone fail, file order fail); Group 11: pass alone and in file order | pending (Group 14) | S1 halved yields | `reruns/A_project_s_whole_life_can_be_played_from_the_portal_window_s_controls.log`, `files/tbm_movement.xml` | 11 |
| A reload during a bend continues the same cursor and ledger (`src/tests/tbm_coverage.cpp:216`) | tbm | fail (alone fail, file order fail); Group 11: pass alone and in file order | pending (Group 14) | S1 halved yields | `reruns/A_reload_during_a_bend_continues_the_same_cursor_and_ledger.log`, `files/tbm_coverage.xml` | 11 |
| A started cutter drives to its face, works its route and backs into its magazine (`src/tests/tbm_movement.cpp:325`) | tbm | fail (alone fail, file order fail); Group 11: pass alone and in file order | pending (Group 14) | S1 halved yields | `reruns/A_started_cutter_drives_to_its_face__works_its_route_and_backs_into_its_magazine.log`, `files/tbm_movement.xml` | 11 |
| Hauler cycle reports insufficient complete face room without a partial transfer or turnaround (`src/tests/tbm_face_cycle.cpp:48`) | tbm | fail (alone pass, file order pass) | pending (Group 14) | self-inflicted Group 4 comment, fixed in Group 8 | `reruns/Hauler_cycle_reports_insufficient_complete_face_room_without_a_partial_transfer_or_turnaround.log`, `files/tbm_face_cycle.xml` | 4 |
| Project controls pause and resume without touching orders or phase (`src/tests/tbm_project.cpp:81`) | tbm | fail (alone fail, file order fail) | pending (Group 14) | S2 TBM rate/phase/support/planner | `reruns/Project_controls_pause_and_resume_without_touching_orders_or_phase.log`, `files/tbm_project.xml` | 12 |
| Released support stays while a train stands on it (`src/tests/tbm_corridor.cpp:95`) | tbm | fail (alone fail, file order fail) | pending (Group 14) | S2 TBM rate/phase/support/planner | `reruns/Released_support_stays_while_a_train_stands_on_it.log`, `files/tbm_corridor.xml` | 12 |
| Sealed TBM startup needs the door heading and capacity for the whole pocket (`src/tests/tbm_engine.cpp:114`) | tbm | fail (alone fail, file order fail); Group 11: pass alone and in file order | pending (Group 14) | S1 halved yields | `reruns/Sealed_TBM_startup_needs_the_door_heading_and_capacity_for_the_whole_pocket.log`, `files/tbm_engine.xml` | 11 |
| The first cut of a standard and a Deep corridor is measured (`src/tests/tbm_commission.cpp:371`) | tbm | fail (alone fail, file order fail) | pending (Group 14) | S2 TBM rate/phase/support/planner | `reruns/The_first_cut_of_a_standard_and_a_Deep_corridor_is_measured.log`, `files/tbm_commission.xml` | 12 |
| Tube depot power restoration services the actual capsule without starting it (`src/tests/transport_tube_recovery.cpp:104`) | transport | fail (alone fail, file order pass) | pending (Group 14) | I (T1 sprite cache) | `reruns/Tube_depot_power_restoration_services_the_actual_capsule_without_starting_it.log`, `files/transport_tube_recovery.xml` | 9 |
| Tube physical topology change revokes unused original reservation without refund (`src/tests/transport_tube_recovery.cpp:87`) | transport | fail (alone fail, file order pass) | pending (Group 14) | I (T1 sprite cache) | `reruns/Tube_physical_topology_change_revokes_unused_original_reservation_without_refund.log`, `files/transport_tube_recovery.xml` | 9 |
| Tube purchase and refit preserve the native movement and cargo domain (`src/tests/transport_tube_construction.cpp:226`) | transport | fail (alone fail, file order pass) | pending (Group 14) | I (T1 sprite cache) | `reruns/Tube_purchase_and_refit_preserve_the_native_movement_and_cargo_domain.log`, `files/transport_tube_construction.xml` | 9 |
| Tube restored service preserves manual depot stop and queued unpowered custody (`src/tests/transport_tube_recovery.cpp:73`) | transport | fail (alone fail, file order pass) | pending (Group 14) | I (T1 sprite cache) | `reruns/Tube_restored_service_preserves_manual_depot_stop_and_queued_unpowered_custody.log`, `files/transport_tube_recovery.xml` | 9 |
| Tube routine allocation leaves native physical path topology stable (`src/tests/transport_tube_recovery.cpp:116`) | transport | fail (alone fail, file order pass) | pending (Group 14) | I (T1 sprite cache) | `reruns/Tube_routine_allocation_leaves_native_physical_path_topology_stable.log`, `files/transport_tube_recovery.xml` | 9 |
| Tube terminals and utility footprints require every actual floor and approach (`src/tests/transport_tube_construction.cpp:158`) | transport | fail (alone fail, file order pass) | pending (Group 14) | I (T1 sprite cache) | `reruns/Tube_terminals_and_utility_footprints_require_every_actual_floor_and_approach.log`, `files/transport_tube_construction.xml` | 9 |
| Five actual capsule bodies preserve load across native service and source waits (`src/tests/transport_tube_vehicle_sprite.cpp:82`) | tube | fail (alone fail, file order fail) | pending (Group 14) | I (T1 sprite cache) | `reruns/Five_actual_capsule_bodies_preserve_load_across_native_service_and_source_waits.log`, `files/transport_tube_vehicle_sprite.xml` | 9 |
| Tube platform art warns on the exact original full native packet queue (`src/tests/transport_tube_facility_sprite.cpp:158`) | tube | fail (alone fail, file order not-reached) | pending (Group 14) | I (T1 sprite cache) | `reruns/Tube_platform_art_warns_on_the_exact_original_full_native_packet_queue.log`, `files/transport_tube_facility_sprite.xml` | 9 |
| Tube terminal and interchange art follows real native platform roles and joins (`src/tests/transport_tube_facility_sprite.cpp:59`) | tube | fail (alone fail, file order fail) | pending (Group 14) | I (T1 sprite cache) | `reruns/Tube_terminal_and_interchange_art_follows_real_native_platform_roles_and_joins.log`, `files/transport_tube_facility_sprite.xml` | 9 |
| Visible chamber quote refuses rail turns forbidden by the current game rule (`src/tests/underground_workflow.cpp:523`) | underground | fail (alone fail, file order fail) | pending (Group 14) | S2 TBM rate/phase/support/planner | `reruns/Visible_chamber_quote_refuses_rail_turns_forbidden_by_the_current_game_rule.log`, `files/underground_workflow.xml` | 12 |
| WindowDesc - ini_key validity (`src/tests/test_window_desc.cpp:49`) | upstream | fail (alone fail, file order fail) | pending (Group 14) | S3 UI/content drift | `reruns/WindowDesc_-_ini_key_validity.log`, `files/test_window_desc.xml` | 13 |
| Common scheduler settles mixed native contention in original destination order (`src/tests/utility_scheduler_contention.cpp:133`) | utility | fail (alone fail, file order fail) | pending (Group 14) | I (T2 language pack) | `reruns/Common_scheduler_settles_mixed_native_contention_in_original_destination_order.log`, `files/utility_scheduler_contention.xml` | 10 |
| Integrated company transitions cannot revive old native provider rights (`src/tests/utility_scheduler_lifecycle.cpp:150`) | utility | fail (alone fail, file order not-reached) | pending (Group 14) | I (T2 language pack) | `reruns/Integrated_company_transitions_cannot_revive_old_native_provider_rights.log`, `files/utility_scheduler_lifecycle.xml` | 10 |
| Integrated source and riser loss preserve original histories and independent work (`src/tests/utility_scheduler_lifecycle.cpp:99`) | utility | fail (alone fail, file order not-reached) | pending (Group 14) | I (T2 language pack) | `reruns/Integrated_source_and_riser_loss_preserve_original_histories_and_independent_work.log`, `files/utility_scheduler_lifecycle.xml` | 10 |
| Native cable loss and repair preserve unrelated town and orbital work (`src/tests/utility_scheduler_lifecycle.cpp:50`) | utility | fail (alone fail, file order not-reached) | pending (Group 14) | I (T2 language pack) | `reruns/Native_cable_loss_and_repair_preserve_unrelated_town_and_orbital_work.log`, `files/utility_scheduler_lifecycle.xml` | 10 |
| Native provider binding captures and hashes ignore transport row order (`src/tests/utility_scheduler_lifecycle.cpp:16`) | utility | fail (alone fail, file order fail) | pending (Group 14) | I (T2 language pack) | `reruns/Native_provider_binding_captures_and_hashes_ignore_transport_row_order.log`, `files/utility_scheduler_lifecycle.xml` | 10 |
| Original interval depletion never republishes energy during consumer retries (`src/tests/utility_scheduler_lifecycle.cpp:129`) | utility | fail (alone fail, file order not-reached) | pending (Group 14) | I (T2 language pack) | `reruns/Original_interval_depletion_never_republishes_energy_during_consumer_retries.log`, `files/utility_scheduler_lifecycle.xml` | 10 |
| Unrelated portal loss preserves an imported native capsule reservation (`src/tests/utility_scheduler_lifecycle.cpp:81`) | utility | fail (alone fail, file order not-reached) | pending (Group 14) | I (T2 language pack) | `reruns/Unrelated_portal_loss_preserves_an_imported_native_capsule_reservation.log`, `files/utility_scheduler_lifecycle.xml` | 10 |

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
