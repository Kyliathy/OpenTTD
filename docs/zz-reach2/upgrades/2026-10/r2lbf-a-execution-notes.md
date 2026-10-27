# Linux build fixes: sequential execution notes

@see [Linux build fixes plan](r2lbf-linux-build-fixes.md). This is an implementation record, not an additional mandatory reading queue.

**Run identity:** logs for this execution live in `logs/linux-baseline/20261027-linux/` (git-ignored), and disposable tooling lives in `test-cache/linux-baseline/` (git-ignored). Both can be rebuilt with the commands recorded in each group below.

**Test policy for this execution (user instruction, 2026-10-27):** tests run in focused bursts for the group being developed. The full CTest suite runs only as the Group 8 baseline and once more at the end of the plan (Groups 14-17). Each group's "full suite" check in the plan is narrowed to its affected names and files.

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
