# Linux build fixes: GCC and Clang compile, link and test baseline

## Global Mandatory Reading

> **Execution protocol**
>
> - Read this Global Mandatory Reading section before the first task and again
>   after every context compaction.
> - Before starting or resuming a task group, read the nearest preceding Area
>   Mandatory Reading block whose `Applies to` label includes that group. Area
>   blocks appear immediately before the first group they govern.
> - A referenced heading or exported symbol is authoritative. Global navigation
>   hints and Area line ranges are for navigation only; if they drift, relocate
>   the named section or symbol rather than expanding to the whole file.
> - Mandatory reading is non-transitive. Do not preload a linked file's own
>   Mandatory Reading, Consult on Demand, or Planning Provenance entries unless
>   this plan names them directly for the current area.
> - Do not read Consult on Demand or Planning Provenance merely because those
>   sources are listed. Use a Consult on Demand source only when its trigger
>   occurs.
> - No created source file, test file or JSON file should start with same prefix as a planning MD file (such as r2). Only MD files can have that prefix. No MD file should be created anywhere in source directories unless specifically asked.
> - After EVERY CONTEXT COMPACTION, you must add a note above the task group
>   which is currently in execution. The note will say: "Context has compacted
>   X times while executing this group.". If the compaction occurs before any
>   meaningful work was done (this can happen when there is a lot of baggage
>   from the previous compaction), then and ONLY then you can skip this step.
> - When exceeding 3 context compactions, YOU MUST ask yourself if you're not 
>   chewing more than we can handle :). Heck, plans change! :). In such a case, 
>   the way out is to split the group on the spot,
>   in the plan, into more groups, 
>   by appending B, C, D as a suffixes to those groups (splitting sub-groups 
>   themselves is also allowed). When you do this, create Local Mandatory
>   Reading areas so you can optimize your context only for the
>   ACTUAL task at hand.
>   Before starting or resuming a task group, read the nearest preceding Area
>   Mandatory Reading block whose `Applies to` label includes that group. Area
>   blocks appear immediately before the first group they govern.

| File name | Line Ranges | Description |
| --- | --- | --- |
| [.github/workflows/ci-linux.yml](../../../../.github/workflows/ci-linux.yml) | 93-118 | **Build** and **Test** steps: `cmake ..`, `cmake --build . -j $(nproc)`, `ctest -j $(nproc) --timeout 120`, then `git diff --exit-code`. This is the upstream reference recipe for Linux and the invariant that building and testing leave every tracked file unmodified. Every verification step in this plan uses the same recipe with this plan's settings: Ninja, `RelWithDebInfo`, at most `-j4`, and a `git status --short` that shows only intended source edits. |
| [media/multilayer/build_content.py](../../../../media/multilayer/build_content.py) | 160-185 | `main()` raises `ValueError("content build output must remain inside the repository")` when `--package`/`--manifest` resolve outside the checkout. This is why every build tree must live inside the repository (`build/`, `build-clang/`, `build-dedicated/`). An out-of-tree build directory is an environment error, not a code defect. Never relax this guard. |
| [.gitignore](../../../../.gitignore) | 3-4 | `/logs` and `/build*` are the ignore boundary for build trees and run logs. They keep generated GRF/manifest outputs, objects, baseset copies and logs out of `git status`. Do not add new ignore rules to hide an unexpected tracked-file change. |
| [src/stdafx.h](../../../../src/stdafx.h) | 43-74 | The standard headers every translation unit receives. `<filesystem>`, `<fstream>` and `<utility>` are absent, so code using `std::filesystem`, `std::exchange` or `std::as_const` must include its header explicitly. The Windows MSVC STL supplies these transitively; libstdc++ (used by both GCC and Clang here) does not. This is the root cause of the largest failure class, and it constrains every fix: include what you use, never rely on transitive includes. |
| [CMakeLists.txt](../../../../CMakeLists.txt) | 54-55; 261-262; 655-659; 707-708 | `CMAKE_CXX_STANDARD 20` (required): fixes must be standard C++20, with no reliance on C++23 lifetime rules or compiler extensions. `openttd` and `openttd_test` are the default executables. `target_precompile_headers(openttd_lib …)` force-includes `src/stdafx.h` and `src/core/format.hpp` (which hides missing includes in PCH builds). `catch_discover_tests(openttd_test)` is the only CTest-registered Catch2 binary. |
| [src/tests/multilayer/support/native_build.py](../../../../src/tests/multilayer/support/native_build.py) | 43-76 | `load_profile` refuses any profile other than `platform == "windows-x64"`, `"Visual Studio 17 2022"` and `maximum_workers == 2`. This is the ownership boundary: the fork's native build and proof pipeline is Windows/MSVC-only and remains the primary build. Linux work uses plain CMake/Ninja and must not edit this profile to admit Linux. Every source fix must stay valid under MSVC. |

## Scope and observed baseline

Planning date: **2026-10-27**. Branch `mlm-mlv`, commit `7a66e52f64bdde4c260328c87f3449bd53356be1` ("Brought back clean.ps1 - the bad one."), clean worktree. The environment is a Linux cloud container (Ubuntu 24.04, 4 cores) with GCC 13.3.0, Ubuntu Clang 18.1.3, CMake 3.28.3, Ninja 1.11.1 and Python 3.13. The required libraries were already installed: SDL2, zlib, liblzma, liblzo2, libpng, libcurl, FreeType, Fontconfig, HarfBuzz and ICU. Allegro, FluidSynth, Opus/OpusFile/Ogg, Soxr, GRFCodec and vcpkg breakpad were absent. All of these are optional, and none caused a failure.

The validation builds (`-G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo`, `ninja -j4 -k 0`) failed on both compilers, so no `openttd` or `openttd_test` binary was linked and no test ran. The build took 21 min 57 s with GCC and 23 min 34 s with Clang. CTest reported 5/5 failing because `openttd_test_NOT_BUILT` and the four regression tests require binaries that do not exist. These failures are build failures, not test results.

| ID | Root cause | GCC | Clang | Sites | Fix |
| --- | --- | --- | --- | --- | --- |
| R1 | `std::filesystem::path` used without `<filesystem>` | fails | fails | `src/tests/layer_save_fixture.h:22`; 92 test objects include it | 2.1 |
| R2 | `std::exchange` / `std::as_const` used without `<utility>` | fails | fails | `src/layer_recipe_work.cpp:42,47`; `src/surface_supply_order.cpp:181` | 2.2-2.3 |
| R3 | Braced-list assignment to a conditional lvalue, `(c ? a : b) = {x, y}`, rejected by GCC 13 | fails | accepts | `layer_processor_sl.cpp:126`, `transport_tube_power_sl.cpp:129`, `geothermal_heat_route_sl.cpp:136` | 2.4-2.6 |
| R4 | Member template `TBMExitJourney::Do` with deduced `auto` return used by the constructor before its definition | fails | fails | `src/tests/tbm_exit.cpp:64-86` | 3.1 |
| R5 | `static inline const SaveLoad description[]` of class template `SlOrdinaryCargoPreview<Type>` is still an incomplete `const SaveLoad[]` when `DefaultSaveLoadHandler::GetDescription` is instantiated | accepts | fails | `src/saveload/vehicle_sl.cpp:1204-1215` via `saveload.h:683` | 3.3 |
| R6 | Range-for over `*GetLaunchCenterFootprint(...)` dereferences a destroyed `std::optional<TileArea>` temporary (C++20 undefined behaviour; Clang `-Wdangling-gsl`) | silent | warns | 6 test sites, Group 4 | 4.1-4.7 |
| R7 | `SaveLoad::required_capability` has no default member initializer, so every designated-initializer factory warns | 944 warnings | 783 warnings | `src/saveload/saveload.h:880` | 2.7 |

Failed build edges: GCC 98 (5 `openttd_lib`, 93 `openttd_test`); Clang 96 (3 `openttd_lib`, 93 `openttd_test`). The planning session checked the R3, R5 and R7 fixes against scratch copies with both compilers. These were not edits to the repository: binding a reference first (`auto &e = c ? a : b; e = {…};`) compiles on both compilers; explicit instantiation of the four `SlOrdinaryCargoPreview` specializations compiles `vehicle_sl.cpp` on both; a default member initializer `{}` removes the warning on both. Clang stops after 20 errors per translation unit, and nothing ever reached the link step. Expect further, currently hidden diagnostics in Group 5.

Network: the proxy refused `https://cdn.openttd.org/opengfx-releases/0.6.0/opengfx-0.6.0-all.zip` (HTTP 403, `CONNECT tunnel failed`), and GitHub release pages also returned 403. The Ubuntu archive package `openttd-opengfx` 7.1-1 (universe) was available through apt.

**In scope:** source fixes that make the branch compile, link and test on Linux with GCC and Clang. These are standard C++20 fixes only, and they keep MSVC builds valid. The scope also covers test-runtime prerequisites and an explicit classification of every remaining failure.

**Out of scope:** changing the Windows native-build profile, CI workflow files, upstream-only code, bulk warning cleanup, vendoring OpenGFX into the repository, and any commit, push or pull request unless the user explicitly asks. Environment-only failures are recorded, not hidden: never use fake environment variables, `[.]` tags, `ctest -E` or skips to make a test look green.

Reference commands. This is an implementation sketch; record the exact commands you actually run:

```bash
# Build trees live inside the checkout (build_content.py guard) and are git-ignored by /build*.
cmake -S . -B build       -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo
CC=clang CXX=clang++ \
cmake -S . -B build-clang -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo
ninja -C build -j4 -k 0 2>&1 | tee logs/linux-baseline/<run>/build-gcc.log   # trees built one after another, never in parallel
ctest --test-dir build -j4 --timeout 600 --output-junit ../logs/linux-baseline/<run>/ctest-gcc.xml
```

Run logs go under `logs/linux-baseline/<run>/`; disposable staging goes under `test-cache/linux-baseline/`. Both are ignored. Implementation progress, the failure ledger and final results go in `r2lbf-a-execution-notes.md` (in this directory), which Group 7 creates.

## Implementation tasks

Every code snippet is an **implementation sketch** written against the live symbols named in the Area blocks. Keep each change minimal and local to the owning file. Match the surrounding style (tabs, dense one-line Doxygen comments) and add no compiler-specific `#if` guards.

### Area Mandatory Reading - Environment preflight

_Applies to Group 1._

| File name | Line Ranges | Description |
| --- | --- | --- |
| [.github/workflows/ci-linux.yml](../../../../.github/workflows/ci-linux.yml) | 39-73 | **Install dependencies**: the upstream apt set (`liballegro4-dev`, `libcurl4-openssl-dev`, `libfontconfig-dev`, `libharfbuzz-dev`, `libicu-dev`, `liblzma-dev`, `liblzo2-dev`, `libogg-dev`, `libopus-dev`, `libopusfile-dev`, `libsoxr-dev`, `zlib1g-dev`, plus `libsdl2-dev` per matrix row) and breakpad only through vcpkg. This defines the CI-parity dependency target and shows that breakpad is optional and vcpkg-only on Linux. |
| [COMPILING.md](../../../../COMPILING.md) | 3-28 | **Required/optional libraries** classifies every library as encouraged or optional. A missing optional library must be recorded, never treated as a build blocker, and never "fixed" by editing CMake. |
| [CMakeLists.txt](../../../../CMakeLists.txt) | 132-194 | The `find_package` block. Only `Threads` is `REQUIRED`. The other packages enable `WITH_*` features conditionally, so a newly installed library changes which sources compile (for example `CONDITION OpusFile_FOUND` / `Soxr_FOUND` in `src/CMakeLists.txt`). Trees must be reconfigured after installing anything. |

### Group 1 - Record the baseline and prepare CI-parity build trees

The only dependency is the clean checkout. This group writes no source.

{{SIMPLE}}

- [ ] **1.1** Confirm branch `mlm-mlv` and record `git rev-parse HEAD` and `git status --short`. The plan was written against `7a66e52f64bdde4c260328c87f3449bd53356be1` with a clean worktree. If HEAD has moved, re-check every site in the **Scope and observed baseline** table with `grep -n` before editing, and update the line numbers in this plan when they drifted.

- [ ] **1.2** Record the exact toolchain: `g++ --version`, `clang++ --version`, `cmake --version`, `ninja --version`, `python3 --version`, `nproc`, and the installed versions of the CI apt packages (`dpkg -l`). The minimums are CMake 3.16, Python 3.9 and a C++20 compiler.

- [ ] **1.3** If package installation is permitted, install the CI-parity optional packages that are missing: `liballegro4-dev libogg-dev libopus-dev libopusfile-dev libsoxr-dev`. Apt candidates were present on 2026-10-27. Skip vcpkg breakpad and record that it is absent. If installation is refused, record that and continue, because these libraries are optional.

- [ ] **1.4** Configure two trees inside the checkout. `build/` uses the default GCC `c++`; `build-clang/` uses `CC=clang CXX=clang++`. Both use `-G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo`. Existing trees from the 2026-10-27 validation may be reused incrementally but must be reconfigured after 1.3. Save each CMake summary (`found` / `Could NOT find` lines, `Option …` lines) to `logs/linux-baseline/<run>/configure-{gcc,clang}.log`.

- [ ] **1.5** Confirm the boundaries: `git check-ignore -v build build-clang build-dedicated logs/linux-baseline test-cache/linux-baseline` matches the expected rules, and `git status --short` is still clean.

### Area Mandatory Reading - Portable source fixes

_Applies to Groups 2-4._

**Entry gate:** Group 1 left two configured trees whose CMake summaries name the same compiler families as above. Verify that `build/build.ninja` and `build-clang/build.ninja` exist. If either is missing, stop object-level verification for that compiler. Source edits may proceed, but no fix is accepted until it compiles in both trees. Neither a scratch copy outside the repository, a single-compiler result, `-fpermissive`, nor removing a source from CMake may stand in for that verification.

| File name | Line Ranges | Description |
| --- | --- | --- |
| [src/tests/layer_save_fixture.h](../../../../src/tests/layer_save_fixture.h) | 12-30 | Include block and `LayerSaveFileSink(const std::filesystem::path &)`. R1's single edit point. The fixture header must include its own `<filesystem>`; no consumer `.cpp` should be patched instead. |
| [src/ground_processor_sprite.cpp](../../../../src/ground_processor_sprite.cpp) | 6-17 | In-repository placement precedent: `"stdafx.h"`, the own header and project headers, then system headers (`<array>`, `<string_view>`, `<utility>`), a blank line, then `"safeguards.h"` last. Apply the same order for R2. |
| [src/saveload/layer_processor_sl.cpp](../../../../src/saveload/layer_processor_sl.cpp) | 116-127 | `LPCHChunkHandler::Read(bool preview)`: the existing `auto &presence = preview ? … : …;` idiom and the failing `(preview ? … : _processor_expected) = {header[1], header[2]};`. R3 uses that same reference-binding idiom. `TUPHChunkHandler::Read` and `GHTHChunkHandler::Read` have the identical shape. |
| [src/saveload/saveload.h](../../../../src/saveload/saveload.h) | 669-685; 858-891 | `DefaultSaveLoadHandler<TImpl, TObject>::GetDescription`, which returns `TImpl::description` as `SaveLoadTable`. This is the instantiation point that needs a complete array type for R5. The `struct SaveLoad` data members through `WithCapability` show `required_capability` as the only member without a default initializer (R7). |
| [src/tests/tbm_exit.cpp](../../../../src/tests/tbm_exit.cpp) | 46-97 | `TBMExitJourney`: the constructor (59-87) calls `this->Do<…>` six times, and `template <Commands Tcmd, typename... Args> auto Do(Args...)` (89-97) is defined after it. A deduced return type must be defined before its first use (R4). |
| [src/saveload/vehicle_sl.cpp](../../../../src/saveload/vehicle_sl.cpp) | 1200-1215; 1301-1312 | `template <VehicleType Type> class SlOrdinaryCargoPreview` with `static inline const SaveLoad description[]`. `VEHSChunkHandler::LoadCheck` instantiates it for `Train`, `Road`, `Ship` and `Aircraft` through `SaveLoad::Struct<…>` (R5). |
| [src/saveload/station_sl.cpp](../../../../src/saveload/station_sl.cpp) | 215-237 | `SlStationSpecList<T>` has the same templated-handler shape and is followed by `/* Instantiate SlStationSpecList classes. */ template class SlStationSpecList<…>;`. This is the repository's established cure for R5; follow it rather than inventing an array bound or an out-of-line definition. |
| [src/launch_center.h](../../../../src/launch_center.h) | 328-334 | `std::optional<TileArea> GetLaunchCenterFootprint(TileIndex anchor, uint8_t rotation)` returns by value. This is why `for (… : *GetLaunchCenterFootprint(…))` iterates a destroyed temporary in C++20 (R6). |
| [src/tests/launch_production.cpp](../../../../src/tests/launch_production.cpp) | 78-79 | `const auto footprint = GetLaunchCenterFootprint(anchor, rotation); REQUIRE(footprint);` is the existing in-repository idiom for R6. Reuse it exactly; do not introduce a helper. |

### Group 2 - Fix missing headers and GCC-rejected assignments before the first full rebuild

These are mechanical, local edits. Item 2.7 touches `saveload.h`, so it goes here: that way Group 5 needs one full rebuild per compiler, not two.

{{SIMPLE}}

- [ ] **2.1** In `src/tests/layer_save_fixture.h`, add `#include <filesystem>` directly after `#include <fstream>` (line 12). This resolves R1 for all 92 dependent test objects on both compilers.

- [ ] **2.2** In `src/layer_recipe_work.cpp`, add `#include <utility>` after `#include "layer_recipe_work.h"`, then a blank line before `#include "safeguards.h"`, following the `ground_processor_sprite.cpp` order. This fixes `std::exchange` at lines 42 and 47. Do not move the include into `layer_recipe_work.h`, because only the `.cpp` uses it.

- [ ] **2.3** In `src/surface_supply_order.cpp`, add `#include <utility>` the same way, after `#include "timer/timer_game_calendar.h"`. This fixes `std::as_const` at line 181.

- [ ] **2.4** In `src/saveload/layer_processor_sl.cpp:126`, replace the conditional-lvalue assignment with the reference idiom already used two lines above. The behaviour is identical:

  ```cpp
  auto &expected = preview ? _load_check_data.layer_processor_expected : _processor_expected;
  expected = {header[1], header[2]};
  ```

- [ ] **2.5** Apply the same rewrite to `src/saveload/transport_tube_power_sl.cpp:129` (`tube_power_expected` / `_tube_power_expected`).

- [ ] **2.6** Apply the same rewrite to `src/saveload/geothermal_heat_route_sl.cpp:136` (`geothermal_heat_route_expected` / `_heat_route_expected`, three elements). Leave line 137 unchanged: GCC accepts assigning a scalar `interval[0]` to a conditional lvalue.

- [ ] **2.7** In `src/saveload/saveload.h:880`, change `std::optional<SaveCapability> required_capability;` to `std::optional<SaveCapability> required_capability{};`. The value is still `std::nullopt` and `SaveLoad` stays an aggregate. This removes about 944 GCC and 783 Clang `-Wmissing-field-initializers` warnings so that real warnings can be seen. Change no factory.

- [ ] **2.8** Verify the six edited objects in both trees: `ninja -C build <obj>` and `ninja -C build-clang <obj>` for `CMakeFiles/openttd_lib.dir/src/{layer_recipe_work,surface_supply_order}.cpp.o`, `CMakeFiles/openttd_lib.dir/src/saveload/{layer_processor_sl,transport_tube_power_sl,geothermal_heat_route_sl}.cpp.o` and the fixture consumer `CMakeFiles/openttd_test.dir/src/tests/space_preview.cpp.o`. Then sweep for remaining R1-R3 shapes: `grep -rnE "\) = \{" src --include=*.cpp --include=*.h` for conditional-lvalue braced assignments, and `grep -rlE "std::(exchange|as_const|filesystem)" src` cross-checked against each file's own or fixture includes. Record the results. Leave any new hit for Group 5, which owns diagnostics, unless it is the identical shape.

### Group 3 - Correct template instantiation and member ordering

These changes need C++ template-semantics judgement and checks under both compilers. MSVC accepts both original forms, so each change must stay a pure reordering or addition.

{{MEDIUM}}

- [ ] **3.1** In `src/tests/tbm_exit.cpp`, move the `Do` member template (doc comment plus lines 89-97) above the `explicit TBMExitJourney(...)` constructor, before line 59. Leave its body unchanged. Do not use a trailing return type, which would duplicate the `Command<Tcmd>::Do(...)` call expression. The diff must be a pure move.

- [ ] **3.2** Confirm `TBMExitJourney` has no other member that uses a deduced-return member defined later in the class. Treat the Clang diagnostic "function '…' with deduced return type cannot be used before it is defined" in Group 5 as the authoritative sweep over all tests. Do not refactor other fixtures speculatively.

- [ ] **3.3** In `src/saveload/vehicle_sl.cpp`, directly after the closing `};` of `SlOrdinaryCargoPreview` (line 1215), add the explicit instantiations, mirroring `station_sl.cpp`:

  ```cpp
  /* Instantiate SlOrdinaryCargoPreview classes. */
  template class SlOrdinaryCargoPreview<VehicleType::Train>;
  template class SlOrdinaryCargoPreview<VehicleType::Road>;
  template class SlOrdinaryCargoPreview<VehicleType::Ship>;
  template class SlOrdinaryCargoPreview<VehicleType::Aircraft>;
  ```

  Use exactly the four types that `VEHSChunkHandler::LoadCheck` names. Clang's diagnostic prints `VehicleType::Begin`, which is the same value as `Train`.

- [ ] **3.4** Verify `CMakeFiles/openttd_lib.dir/src/saveload/vehicle_sl.cpp.o` and `CMakeFiles/openttd_test.dir/src/tests/tbm_exit.cpp.o` compile in both trees with no new warnings from those files.

- [ ] **3.5** Review the combined diff of Groups 2-3 adversarially. Each hunk must be an include, a reference binding, a member move, an explicit instantiation or a default member initializer. No hunk may change a saved field, a version gate, a command or an assertion. Record the diff summary in the execution notes when Group 7 creates them.

### Group 4 - Remove dangling footprint temporaries in tests

Use the idiom from `launch_production.cpp:78-79` at every site: name the `std::optional` so that it outlives the loop, and require it before dereferencing. The original code dereferenced unconditionally, which was also undefined behaviour if empty. A named local with `REQUIRE(footprint)` keeps the intent and turns a silent fault into a test failure. If `footprint` is already taken in that scope, pick a nearby unused name.

```cpp
const auto footprint = GetLaunchCenterFootprint(tile, 0);
REQUIRE(footprint);
for (const auto cell : *footprint) { /* unchanged body */ }
```

{{SIMPLE}}

- [ ] **4.1** Fix `src/tests/descent_custody.cpp:36`.

- [ ] **4.2** Fix `src/tests/launch_center.cpp:105`.

- [ ] **4.3** Fix `src/tests/launch_center.cpp:600` (a one-line `for … CHECK(IsTileType(…))` inside a `SECTION`; keep the `CHECK` unchanged).

- [ ] **4.4** Fix `src/tests/launch_production.cpp:292` (iteration variable `tile`; pass `anchor, rotation` unchanged).

- [ ] **4.5** Fix `src/tests/launch_retirement_native.cpp:94` (rotation `receiver->launch_center->rotation`).

- [ ] **4.6** Fix `src/tests/surface_collection_boundaries.cpp:47`.

- [ ] **4.7** Sweep `grep -rnE "for \(.*: \*[A-Za-z_:]+\(" src` for the same shape over any function that returns `std::optional`/by value, including production code. Fix every hit with the same idiom. Production code takes `assert(footprint.has_value())` (or the owner's existing check) instead of `REQUIRE`. Compile the five edited test objects in both trees.

### Area Mandatory Reading - Full builds and test runtime prerequisites

_Applies to Groups 5-6._

**Entry gate:** items 2.8, 3.4 and 4.7 record successful object compiles in both trees. If any is missing, stop 5.1-5.6. Items 6.1, 6.2, 6.4 and 6.5 may continue because they do not need new binaries. Do not use any of these to pass the gate or to produce the Group 5 result: stale binaries or objects from a different configuration, `-k 0` partial builds reported as success, compiler-flag suppression, or removing tests/sources from `CMakeLists.txt`.

| File name | Line Ranges | Description |
| --- | --- | --- |
| [CMakeLists.txt](../../../../CMakeLists.txt) | Target heads at 264, 360, 369, 379, 387, 420, 435, 576, 609 (263 is the ownership comment) | The nine focused `EXCLUDE_FROM_ALL` executables: `openttd_surface_supply_test`, `openttd_surface_capital_test`, `openttd_processor_contract_test`, `openttd_ground_art_test`, `openttd_ground_consumer_test`, `openttd_ground_viewport_test`, `openttd_tube_test`, `openttd_space_art_test` and `openttd_merge_contract_test`. They are neither built by `ninja` nor registered with CTest, so they must be built and run explicitly. |
| [cmake/Catch.cmake](../../../../cmake/Catch.cmake) | 149-170 | `catch_discover_tests` adds a `POST_BUILD` command that executes the freshly linked `openttd_test` (`--list-test-names-only`). A startup crash on Linux therefore surfaces as a *build* failure of `openttd_test`, and discovery output is the test inventory. |
| [cmake/CreateRegression.cmake](../../../../cmake/CreateRegression.cmake) | 49-57 | `add_test(regression_<name> …)` runs `cmake/scripts/Regression.cmake` with `WORKING_DIRECTORY ${CMAKE_BINARY_DIR}`, so regression runs execute in the build tree root. |
| [cmake/scripts/Regression.cmake](../../../../cmake/scripts/Regression.cmake) | 38-65 | `execute_process(${OPENTTD_EXECUTABLE} -x -c regression/regression.cfg -g … -snull -mnull -vnull:ticks=30000 -d script=2 -Q)` and the failure rules: a crash log, unexpected stdout, or empty stderr. A real `openttd` binary and a discoverable graphics baseset are mandatory. |
| [src/fileio.cpp](../../../../src/fileio.cpp) | 958-1018 | In `DeterminePaths`, with `-c` the config directory is `Searchpath::WorkingDir`, and outside the XDG branch `_personal_dir = config_dir`. For regression runs the personal directory is therefore the build tree, and `<build>/baseset/` is a repository-local, git-ignored baseset location. |
| [.github/workflows/ci-linux.yml](../../../../.github/workflows/ci-linux.yml) | 75-88 | **Get OpenGFX**: CI downloads OpenGFX 0.6.0 into `~/.local/share/openttd/baseset`. This environment's proxy refuses that host (HTTP 403), so the reference step must be adapted, not copied. |

### Group 5 - Produce complete GCC and Clang builds and resolve newly surfaced diagnostics

This is the first time this branch is linked on Linux. Build the trees one after another so that the 4-job limit holds.

{{MEDIUM}}

- [ ] **5.1** GCC: run `ninja -C build -j4 -k 0` with full output in `logs/linux-baseline/<run>/build-gcc.log`. Record the wall time (about 22 minutes on 4 cores on 2026-10-27), the `FAILED:` count and the warning counts per `-W` flag.

- [ ] **5.2** Clang: the same in `build-clang/`, logged to `build-clang.log`. Also record `-Wdangling-gsl`, which must be 0 after Group 4.

- [ ] **5.3** Triage every newly surfaced compile or link error. These were previously hidden by Clang's 20-error-per-file limit, or they come from the first link ever done on Linux: undefined references from Windows-only or unregistered sources, duplicate symbols, or `inline` variables. Fix each one with a standard-conforming edit in the owning file, using the Global contracts. If more than five new distinct root causes appear, split this group per the protocol (5B, 5C…), one root-cause family per sub-group.

- [ ] **5.4** Confirm that the `POST_BUILD` discovery succeeded in both trees, and record `ctest -N | tail -1`. Planning estimate: about 3,376 `TEST_CASE`s, of which about 500 are hidden `[.`, so roughly 2,870 registered. If discovery crashes, record the signal and output and carry it to Group 7 as a runtime defect. Do not disable discovery.

- [ ] **5.5** Explicitly build the nine focused executables in both trees, for example `ninja -C build -j4 openttd_surface_supply_test … openttd_merge_contract_test`, and fix their diagnostics under 5.3's rules.

- [ ] **5.6** Run `git status --short`. It must show only the intended source edits from Groups 2-5. Confirm that no `media/**/*.grf` was regenerated (GRFCodec is absent) and that `newgrf/multilayer_industry.grf` and `generated/table/multilayer_content_manifest.h` exist only inside the build trees.

### Group 6 - Provide test runtime prerequisites without touching tracked content

{{SIMPLE}}

- [ ] **6.1** Obtain OpenGFX. First retry the CI URL once. If it is still refused (expected: HTTP 403 through the proxy), install the Ubuntu package `openttd-opengfx` (7.1-1). Record the installed version and the files `dpkg -L openttd-opengfx` lists. Note the difference from CI's 0.6.0 as a known risk to regression output.

- [ ] **6.2** Copy or symlink the OpenGFX `.obg`/`.grf` files into `build/baseset/` and `build-clang/baseset/`, which are the personal directories for `-c` runs under `DeterminePaths`. Fall back to CI's `~/.local/share/openttd/baseset` only if 6.3 proves the repository-local location is not searched, and record why.

- [ ] **6.3** Smoke-test the regression path with `ctest --test-dir build -R regression_regression --output-on-failure`. It must not report a missing graphics set. If it fails for any other reason, stop here and pass the output to Group 7 for classification, without fixing anything.

- [ ] **6.4** Inventory the wrapper-only test inputs. Consumers of `GetTestExpectedSprites` (6 files) and other `OTTD_TEST_*` variables require the Windows harness's environment, and will throw when run directly. List their test names and tags as the expected **E** (environment) category for Group 7.

- [ ] **6.5** Confirm that the test output locations are present and ignored: `mlm/reviews/`, `mlm/runtime/`, `logs/` and `test-cache/`. Check them with `git check-ignore -v`, keeping `mlm/reviews/README.md` tracked. Run tests only from inside the checkout so that `FindTestRepositoryRoot` resolves this repository.

### Area Mandatory Reading - Test execution and failure triage

_Applies to Groups 7-8._

**Entry gate:** for each tree, `openttd` and `openttd_test` are linked by the Group 5 build of the current source and `ctest -N` lists more than zero tests. Verify the binary timestamps against the last edited source and the `ctest -N` count. If one tree is missing, stop its 7.x tasks and run the other tree's. If both are missing, stop Groups 7-8 and return to Group 5. Do not use binaries from another commit, configuration or machine; hand-written test lists; tag edits; `ctest -E` exclusions that are not recorded as E-category in the ledger; or fabricated `OTTD_TEST_*` values.

| File name | Line Ranges | Description |
| --- | --- | --- |
| [src/tests/test_artifact_paths.h](../../../../src/tests/test_artifact_paths.h) | 17-66 | `TestArtifactPaths`, `FindTestRepositoryRoot`, `ValidateTestArtifactIdentifier`, `RequireTestArtifactWithin`, `GetTestArtifactPaths`. Defines where every test may write (feature/scenario/run routing inside the checkout) and which identifiers may be used. A Linux failure here is shared infrastructure and is ranked first in Group 8. |
| [src/tests/test_artifact_paths.cpp](../../../../src/tests/test_artifact_paths.cpp) | 284-336 | `GetTestExpectedSprites` throws unless `OTTD_TEST_EXPECTED_SPRITES_FAMILY`, `…_CONFIGURATION`, `OTTD_TEST_RUN_ID`, payload/manifest paths and SHA256s are supplied by the Windows harness. Such throws are category E, not code defects. Do not synthesize these inputs. |
| [docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup.md](r2fc-file-cleanup.md) | 271-282 | **Output and fixture contract**: fixtures are read-only; outputs are routed by feature, then scenario, then run under `mlm/reviews/`; logs go under `logs/`; native data homes go under `mlm/runtime/<profile>/<run-id>/<process>/`. A Linux run that writes anywhere else, or modifies a fixture, is a defect to fix, not a path to whitelist. |

### Group 7 - Run every registered and focused test and classify each failure

{{HARD}}

- [ ] **7.1** GCC: run `ctest --test-dir build -j4 --timeout 600 --output-junit <abs>/logs/linux-baseline/<run>/ctest-gcc.xml`. The timeout is raised from CI's 120 s because native cases drive up to 250,000 frames. Record passed, failed, skipped (Catch `SKIP`/not run) and timed-out counts, and list the 20 slowest tests.

- [ ] **7.2** Clang: the same in `build-clang/` into `ctest-clang.xml`.

- [ ] **7.3** Run each of the nine focused executables directly from its tree, with the working directory inside the checkout and `-r junit -o <abs>/logs/linux-baseline/<run>/<target>-<compiler>.xml`. Record the same counts.

- [ ] **7.4** Re-run every failure singly (`ctest -R '^<name>$' --output-on-failure`, or the executable with the quoted test name and `-s`) to capture its diagnostics. Use `gdb -batch -ex run -ex bt --args …` for crashes, because the stack printer in `test_main.cpp` is MSVC-only.

- [ ] **7.5** Classify each failure as:
  - **E**: environment (wrapper-only `OTTD_TEST_*` inputs, baseset version, network, display or fonts).
  - **P**: platform portability (path separators or case-sensitive names, CRLF fixtures, Windows-only APIs, LP64 `long`, `char` signedness, unordered-container iteration order, unstable `std::sort` ties, locale or formatting).
  - **C**: a genuine defect that reproduces on both compilers.
  - **X**: GCC and Clang disagree.

  Base each label on evidence from the singly re-run output.

- [ ] **7.6** Create `r2lbf-a-execution-notes.md` (in this directory) in this directory, following the house execution-notes style. It holds the toolchain and commands, the results per tree, and the failure ledger: test name, GCC result, Clang result, category, one-line evidence, owning file, and the fix group or "environment, not fixed".

- [ ] **7.7** Confirm that the tracked tree is unchanged after all test runs (`git status --short`), mirroring CI's `git diff --exit-code`. Record any test that writes tracked files as category P or C.

### Group 8 - Fix diagnosed Linux test failures

The ledger determines this group's actual content. Work through root-cause families, not individual tests, and keep Windows behaviour identical.

{{COMPLEX}}

- [ ] **8.1** Order the ledger by blast radius. Shared test infrastructure comes first: `test_artifact_paths.cpp`, `layer_save_fixture.h`, `mock_*` and the workflow fixtures. Individual scenarios follow. Group failures that share one root cause.

- [ ] **8.2** Fix category P items in shared infrastructure with portable standard facilities, for example `std::filesystem::path` composition instead of `'\\'` literals, exact-case file names, binary-mode I/O for byte fixtures, and explicit ordering instead of hash-order assumptions. MSVC semantics must be unchanged. Re-run the affected tests in both trees.

- [ ] **8.3** Fix category C items in the owning production or test code, with focused re-runs. Change a test expectation only if it encoded a Windows-only artifact, and record the reason in the ledger.

- [ ] **8.4** Fix category X items with standard-conforming code that both compilers accept. A compiler-specific guard is allowed only for a documented compiler bug that has no portable alternative, and it must cite the bug.

- [ ] **8.5** Do not change code for category E items. Record the exact missing prerequisite and the harness that normally provides it, so the item stays visibly not runnable on Linux.

- [ ] **8.6** After the last fix, re-run the full CTest suite and the nine focused executables in both trees. Update the ledger with final per-tree pass/fail/skip counts.

- [ ] **8.7** If the ledger has more than eight distinct C/P/X root-cause families, or this group passes three compactions, split it per the protocol into 8B, 8C… by family, each with its own local Area block.

### Area Mandatory Reading - CI-parity variant and residual warnings

_Applies to Group 9._

**Entry gate:** Group 7's ledger exists and every non-E failure in it is fixed or explicitly carried. Verify this in the execution notes. If it is absent, stop 9.3, because a test run in a third tree cannot be classified without a baseline. Items 9.1, 9.2, 9.4 and 9.5 may continue.

| File name | Line Ranges | Description |
| --- | --- | --- |
| [.github/workflows/ci-build.yml](../../../../.github/workflows/ci-build.yml) | 23-52 | The `linux` matrix: Clang, GCC with SDL2, and **GCC - Dedicated** with `-DOPTION_DEDICATED=ON -DCMAKE_CXX_FLAGS_INIT="-DRANDOM_DEBUG" -DCMAKE_DISABLE_PRECOMPILE_HEADERS=ON` and no build type (Debug, asserts on). The Dedicated row is the only CI variant not yet covered, and its disabled PCH exposes includes hidden by the forced `stdafx.h`/`format.hpp`. |
| [cmake/CompileFlags.cmake](../../../../cmake/CompileFlags.cmake) | 59-79 | The GNU/Clang warning set (`-W -Wall -Wextra -Wsign-compare -Wredundant-decls …`) without `-Werror`. Warnings never fail the build, so residual warnings are recorded debt. Do not add `-Werror` or `-Wno-*` flags in this plan. |

### Group 9 - Cover the CI Dedicated/no-PCH variant and classify residual warnings

{{MEDIUM}}

- [ ] **9.1** Configure `build-dedicated/` with GCC exactly as the CI Dedicated row does, with no build type: `-G Ninja -DOPTION_DEDICATED=ON -DCMAKE_CXX_FLAGS_INIT="-DRANDOM_DEBUG" -DCMAKE_DISABLE_PRECOMPILE_HEADERS=ON`. Build it with `ninja -j4 -k 0` into `build-dedicated.log`.

- [ ] **9.2** Fix every missing include the disabled PCH exposes by including what is used in the owning file. Do the same for any `OPTION_DEDICATED`-only compile error in sources this branch added. Re-verify each touched object in `build/` and `build-clang/`.

- [ ] **9.3** Run `ctest --test-dir build-dedicated -j4 --timeout 600`, then classify and fix only new failures using Group 7/8 rules. Debug, `RANDOM_DEBUG` and dedicated-only failures are recorded separately in the ledger.

- [ ] **9.4** Classify the 11 GCC `-Wmisleading-indentation` sites: `asteroid_gui.cpp:317`, `launch_assembly_gui.cpp:239,255`, `space_manifest.cpp:569`, `space_production.cpp:128,131`, `space_transition.cpp:80,139,348`, `tests/manufacturing_clone_native.cpp:292` and `tests/space_resupply.cpp:203`. Confirm that each dense `if (x) a; b;` line means `b` to run unconditionally. Change only a site where it does not, as a C-category fix. Record the rest as intentional style.

- [ ] **9.5** Record the final per-flag warning counts for all three trees in the execution notes, compared with the planning baseline (R7 and the counts in 5.1/5.2). `-Wrange-loop-construct`, `-Wunused-function` and `-Wsign-compare` stay as recorded debt, with no bulk edits.

### Area Mandatory Reading - Baseline record and Windows handoff

_Applies to Group 10._

**Entry gate:** Groups 1-9 are complete or have recorded, explicit carry-overs in the execution notes. Verify this against the checkboxes and the ledger. If a group is open without a carry-over, stop 10.1 and 10.3. Item 10.2 may be prepared. A summary must not describe an unexecuted run as passing.

| File name | Line Ranges | Description |
| --- | --- | --- |
| [src/tests/multilayer/support/native_build.py](../../../../src/tests/multilayer/support/native_build.py) | 109-175 | `discover_tools` binds the pinned MSVC compiler, MSBuild, CMake, Windows SDK and vcpkg executables. This is the toolchain the owner will use to confirm that the Linux fixes did not regress the primary Windows Debug/RelWithDebInfo build. |
| [docs/zz-reach2/upgrades/2026-10/m2o26-a-execution-notes.md](m2o26-a-execution-notes.md) | 7-15 | **Group 1 completed**: the house record style. It gives the observed facts, where the logs and observations live, how to regenerate them, and the explicit limits of what was and was not run. Group 10's final record follows this shape. |

### Group 10 - Publish the Linux baseline and hand over Windows verification

{{SIMPLE}}

- [ ] **10.1** Finalize `r2lbf-a-execution-notes.md` (in this directory) with: the commit tested, the toolchains, the exact configure/build/test commands for all three trees, durations, final pass/fail/skip/time-out counts per tree and per focused executable, and the remaining E-category list with its prerequisites.

- [ ] **10.2** Write the Windows handoff: list every touched source file and the reason for each change. Ask the owner to build Debug and RelWithDebInfo through the existing Windows native-build profile and run the affected focused tests. Completion needs that confirmation or an explicit waiver from the owner, because MSVC cannot be run in the Linux environment.

- [ ] **10.3** Report `git status --short` and `git diff --stat`. Do not commit, push or open a pull request unless the user explicitly asks for it.

- [ ] **10.4** List follow-up options for the user to decide on, without implementing them: a cloud SessionStart hook that installs the Group 1/6 prerequisites, a Linux job on the fork's CI, and pinning OpenGFX for regression parity.

## Consult on Demand

These are conditional sources, not more prerequisites. Read a row only after its observable trigger occurs.

| Trigger | File name | Start at | Expand only when | Consultation purpose |
| --- | --- | --- | --- | --- |
| Linking `openttd` or `openttd_test` reports undefined references to Unix platform symbols, or Windows-only symbols are still referenced | [src/os/unix/CMakeLists.txt](../../../../src/os/unix/CMakeLists.txt) | The `add_files(… CONDITION UNIX …)` blocks and the `unix_main.cpp` registration (lines 1-21) | The missing symbol is defined in a source registered under another `CONDITION` in `src/CMakeLists.txt` | Find which source owns the symbol on Linux and whether it is conditionally excluded. This shows registration; it does not authorize stubbing out platform code. |
| `openttd_test` builds but `ctest -N` lists zero tests, or the `POST_BUILD` step fails | [cmake/CatchAddTests.cmake](../../../../cmake/CatchAddTests.cmake) | `execute_process(… --list-test-names-only)` and its result check, lines 38-55 | The listing command succeeds by hand but the generated `openttd_test_tests-*.cmake` is empty or malformed | Separate a test-binary startup failure from a discovery-script problem. It does not justify bypassing discovery. |
| A test fails because `ValidateTestArtifactIdentifier` or a producer lookup refuses a feature or scenario | [mlm/review-producers.json](../../../../mlm/review-producers.json) | `_documentation` and the exact feature key named in the error | The key exists but the refusal persists | Confirm the declared feature/scenario catalogue. It is the routing authority; do not add entries to make a Linux run pass. |
| `RequireTestArtifactWithin` refuses a path because the checkout or `/tmp` resolves through a symlink on Linux | [src/tests/test_artifact_paths.cpp](../../../../src/tests/test_artifact_paths.cpp) | `RequireTestArtifactWithin` at lines 148-161 | Canonicalization differs between the owner and the candidate only by symlink resolution | Decide whether this is a P-category portability defect in containment. The containment invariant itself must stay. |
| `regression_*` reports `Unexpected output` or a result mismatch after the baseset is found | [cmake/scripts/Regression.cmake](../../../../cmake/scripts/Regression.cmake) | Normalization rules after line 66, then `regression/regression/result.txt` | The difference survives the script's normalizations and is not baseset-version text | Distinguish OpenGFX 7.1-vs-0.6.0 or platform formatting noise from a real script or engine regression. Expected results are upstream-owned and are not edited to fit. |
| `openttd` reports no graphics set even though `<build>/baseset/` holds OpenGFX | [src/fileio.cpp](../../../../src/fileio.cpp) | `DeterminePaths` search-path setup, lines 932-957 | `-d misc=3` output does not list the build tree as a search path | Confirm the actual search-path order on this build (`WITH_PERSONAL_DIR`, XDG) before choosing CI's user-wide fallback. |
| GCC and Clang disagree on whether a construct is valid, or a fix relies on temporary-lifetime rules | [C++ working draft, \[expr.ass\]](https://eel.is/c++draft/expr.ass) and [\[stmt.ranged\]](https://eel.is/c++draft/stmt.ranged); [P2718R0](https://wg21.link/p2718r0) | Assignment from a *braced-init-list*; the range-for rewrite and its C++23 lifetime extension | The compiler behaviour contradicts the C++20 rule as read | Decide which compiler is conforming for X-category items. The draft tracks C++26, so check that a rule is not C++23-only before relying on it. |
| A test, build step or fix needs to write outside `mlm/reviews/`, `mlm/runtime/`, `logs/` or `test-cache/` | [docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup.md](r2fc-file-cleanup.md) | **Proposed directory ownership**, lines 173-228 | The owning directory for the artifact kind is still unclear | Resolve artifact ownership. It never permits writing to tracked fixtures or user-wide directories. |
| A test crashes or aborts on Linux without a useful stack | [src/tests/test_main.cpp](../../../../src/tests/test_main.cpp) | The `#ifdef _MSC_VER` block at line 24, closed by `#endif /* _MSC_VER */` at line 157 | `gdb` cannot reproduce the abort | Confirm that the abort-stack printer is MSVC-only, so Linux diagnosis uses `gdb`/core dumps. Do not port the Windows handler in this plan. |
| The build tries to regenerate `.grf` files, or `git status` shows modified `media/**/*.grf` | [COMPILING.md](../../../../COMPILING.md) | **Compilation of base sets**, lines 123-136 | GRFCodec/NFORenum have been installed by someone | Disable `GRFCODEC_EXECUTABLE`/`NFORENUM_EXECUTABLE` in the CMake cache and restore nothing by hand; only tracked bytes from Git are authoritative. |

## Planning Provenance

This is **not an execution reading queue** and must not be preloaded after context compaction. It is **non-transitive**: linked reading lists do not become requirements. Ranges record what was used during planning, not runtime authority. Live code and tests take precedence for delivered behaviour. The user's 2026-10-27 instructions supplied the format and the repository-safety rules: no commits, pushes or pull requests, and no source changes during the validation that preceded this plan.

| File name | Sections or Line Ranges used while planning | Planning contribution |
| --- | --- | --- |
| Validation build logs from the 2026-10-27 cloud session (ephemeral scratchpad; not retained in the repository) | GCC and Clang `ninja -j4 -k 0` logs; `ctest` output | Supplied R1-R7, the failed-edge counts, warning-flag counts, the 20-error Clang cap, and the fact that no link or test ever ran. These shaped the order Groups 2-4, then a single full rebuild in Group 5. |
| Scratch compile experiments from the same session (ephemeral) | Conditional-lvalue braced assignment variants; default-member-initializer warning test; patched `vehicle_sl.cpp` compiled with the real Clang and GCC commands | Confirmed the exact R3, R5 and R7 remedies on both compilers before they were prescribed, and showed that GCC rejects only the conditional-lvalue form. |
| [.github/workflows/ci-linux.yml](../../../../.github/workflows/ci-linux.yml) | 39-118 | Defined the dependency set, the OpenGFX step, the build/test recipe and the tracked-file invariant that Groups 1, 5, 6 and 7 mirror. |
| [.github/workflows/ci-build.yml](../../../../.github/workflows/ci-build.yml) | 23-52 | Identified the three Linux CI variants and the uncovered Dedicated/no-PCH/`RANDOM_DEBUG` row, which became Group 9. |
| [COMPILING.md](../../../../COMPILING.md) | 3-28; 85-136 | Classified libraries as optional, gave the `RelWithDebInfo` build option, and warned about GRFCodec regenerating `.grf` files. |
| [media/multilayer/build_content.py](../../../../media/multilayer/build_content.py) and [media/multilayer/CMakeLists.txt](../../../../media/multilayer/CMakeLists.txt) | `main()`, 160-185; custom command, 4-21 | Explained the first validation failure (an out-of-tree build directory) and fixed the rule that all build trees live in the checkout. |
| [src/tests/multilayer/support/native_build.py](../../../../src/tests/multilayer/support/native_build.py) | `load_profile`, 43-76; `discover_tools`, 109-175 | Showed that the fork has only ever been built through a pinned Windows/MSVC profile. This explains why transitive-include and MSVC-lenient constructs went unnoticed and requires the Windows handoff in 10.2. |
| [src/saveload/station_sl.cpp](../../../../src/saveload/station_sl.cpp) | 215-237 | Supplied the in-repository precedent for curing R5 by explicit instantiation. |
| [src/ground_processor_sprite.cpp](../../../../src/ground_processor_sprite.cpp) | 6-17 | Supplied the include-order precedent for R2. |
| [src/tests/test_artifact_paths.cpp](../../../../src/tests/test_artifact_paths.cpp) | `ArtifactEnvironment` consumers, 32-188; `GetTestExpectedSprites`, 284-336 | Identified the wrapper-only environment contract, so E-category failures are expected and must not be faked (6.4, 7.5, 8.5). |
| [src/fileio.cpp](../../../../src/fileio.cpp) | `DeterminePaths`, 932-1018 | Showed that `-c` makes the build tree the personal directory, which allows a repository-local baseset (6.2) instead of CI's user-wide location. |
| [cmake/Catch.cmake](../../../../cmake/Catch.cmake), [cmake/CreateRegression.cmake](../../../../cmake/CreateRegression.cmake), [cmake/scripts/Regression.cmake](../../../../cmake/scripts/Regression.cmake) | 149-170; 49-57; 38-65 | Established post-build discovery, the working directory of regression runs, and the regression pass/fail rules used by Groups 5-7. |
| [docs/zz-reach2/upgrades/2026-10/m2o27-merge-runtime-qualification-oct26.md](m2o27-merge-runtime-qualification-oct26.md) | 1-130; 337-365; 569-632 | Format precedent for Area blocks with entry gates, task phrasing, Consult on Demand/Provenance tables and a final Completion Conditions audit. |
| [docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup.md](r2fc-file-cleanup.md) | **Output and fixture contract**, 271-282 | Fixed the artifact-ownership invariants that Linux test runs must respect (6.5, 7.7, Area D). |
| Proxy probes during planning | `curl -I` to `cdn.openttd.org` and GitHub releases; `apt-cache policy openttd-opengfx liballegro4-dev libopusfile-dev libsoxr-dev` | Established the HTTP 403 environment blocker and the apt fallbacks used in 1.3 and 6.1. |

## Completion Conditions

The Linux baseline is complete only when these all hold:

- Groups 1-10 are resolved in order. Any open item is visibly carried in the execution notes with its reason; none is silently skipped.
- `openttd`, `openttd_test` and the nine focused executables build from the same commit with GCC (`build/`) and Clang (`build-clang/`) in `RelWithDebInfo`, and the CI-parity Dedicated/no-PCH GCC tree (`build-dedicated/`) builds. Every fix is standard C++20, local to its owning file, contains no compiler-specific guard unless a cited compiler bug requires one, and was not achieved by removing sources, tests or warnings flags.
- The full CTest suite and all focused executables have been run in every tree, with recorded pass/fail/skip/time-out counts. Every remaining failure is in the ledger with an evidence-backed category. Only E-category items may remain unfixed, and each names its missing prerequisite. No test was hidden, retagged, excluded without a record, or fed fabricated `OTTD_TEST_*` inputs.
- Building and testing leave every tracked file unchanged apart from the intended source edits, matching CI's `git diff --exit-code` rule. Generated packages, baseset copies, logs and test outputs stay in ignored, repository-local locations.
- The Windows owner has confirmed Debug and RelWithDebInfo builds through the existing native-build profile after these edits, or has explicitly waived that check.
- No commit, push or pull request was made unless the user explicitly asked.

Planning-format audit (2026-10-27): **59 empty task checkboxes in 10 groups (1-10), one difficulty tag per group, at most 8 tasks per batch (Group 2), and 6 Area blocks with unambiguous coverage**: Group 1; Groups 2-4; Groups 5-6; Groups 7-8; Group 9; Group 10. Every line range above was checked against commit `7a66e52f` during planning. Item 1.1 re-checks them if HEAD has moved. No implementation group has started, and only this plan file was created.
