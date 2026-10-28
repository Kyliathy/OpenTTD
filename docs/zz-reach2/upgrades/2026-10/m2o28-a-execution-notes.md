# October 28 merge: execution notes

@see [Part 1](m2o28-1-merge-oct28.md), [Part 2](m2o28-2-merge-oct28.md) and the [savegame architecture](../../architecture/savegame/archi-savegame.md). This is an execution record, not a mandatory reading queue: read a section only when its group needs it.

Disposable evidence lives in `test-cache/merge-oct28/` and `logs/merge-oct28/` (both git-ignored). Every file named here can be rebuilt with the command given next to it.

## Carry-forward notes

Short facts that later groups need, so a resumed session does not have to rediscover them:

- `SaveCapability` ends with `Count`; Group 9 appends before it, and `src/tests/save_lineage_policy.cpp` has a `static_assert` on `Count` that forces an expectation row per capability.
- The forecast tree changes whenever `mlm-mlv` gains a commit (conflict-marker labels and the plan files). Compare conflicted paths and block counts, not the tree hash.
- Disk is tight: about 8.4 GiB free at the start, with `build/` 8.2 GiB, `build-clang/` 2.7 GiB and `build-dedicated/` 6.1 GiB. Part 2 Group 31's "move the old tree aside" needs revisiting before it runs.

## Group 1

**1.1** `git fetch origin mlm-mlv-2sp mlm-mlv` (2026-10-28). `mlm-mlv` = `799d284d`, a descendant of `d49c3596` whose only changes are planning Markdown plus a comment-only `@see` update in `src/tests/multilayer/support/donor_source_stage_check.py` and `package_transfer.py` (repointed from the deleted m2o26 plan). `origin/mlm-mlv` = `d49c3596`; `origin/mlm-mlv-2sp` = `66713bdf`; merge base `9f36af95`. `git status --short` was empty. No drift.

**1.2** `git switch -c merge-2sp-oct28 mlm-mlv`. The plan files were already committed on `mlm-mlv` (`ce3f75a0`, `799d284d`), so no extra planning commit was needed. The branch has no upstream (`git rev-parse @{u}` fails), and none will be set.

**1.3** `python3 -I test-cache/merge-oct28/dispositions.py 9f36af95 mlm-mlv origin/mlm-mlv-2sp test-cache/merge-oct28/dispositions.tsv`. Result: 5,286 `D-only`, 182 `S-only`, 23 `shared-conflict`, 17 `shared-clean-review`; 6 S-only paths flagged `policy`. S has 128 additions, 94 modifications, 0 renames and 0 deletions. The raw name/status lists are `test-cache/merge-oct28/{d,s}-status.txt`.

**1.4** `git merge-tree --write-tree --name-only mlm-mlv origin/mlm-mlv-2sp > logs/merge-oct28/merge-tree.txt`, with block counts in `logs/merge-oct28/merge-tree-blocks.txt`: 23 paths, 42 blocks, matching the plan. The tree is now `4884d28f…` instead of the planning `d0486387…`. The 32 differing files are the plan files plus the 23 conflicted files, whose markers now say `mlm-mlv` instead of `HEAD`; no conflict content changed.

**1.5** `test-cache/merge-oct28/duplicate-scan.txt`:

| Scan | Result | Resolved in |
| --- | --- | --- |
| Symbols on one side only | `NativeCargoReferencePreview`, `HandleWindowOpeningHotkey`, `SelectMainViewportLayer` exist only in S; `NativeCargoOwnerPreview`, `ToggleHotkeyWindow`, `ShowMainViewportLayerInDisplayOrder` only in D. `GetSpaceSupplyLabel` and `hotkey_toggles_window` exist on both. | 4.5, 6.2-6.4, 5.1, 6.5 |
| Command 211 | D: `CancelLegacySurfaceSupplyOrder`; S: `SetSpaceMiningPolicy`. | 5.3 (S moves to 223) |
| S uses of names Group 6 removes | `toolbar_gui.cpp` (18), `toolbar_gui.h` (4), `map_layer_gui.cpp` (2), `window.cpp` (2), `map_layer_gui.h`, `window_func.h`, `tests/space_fixes_native.cpp`, `tests/multilayer/acceptance/space_fixes_checks.py` (1 each). | 6.2-6.7 |
| Hidden duplicates | `STR_CONFIG_SETTING_HOTKEY_TOGGLES_WINDOW` (+`_HELPTEXT`) and `gui.hotkey_toggles_window` merge twice without markers. The two layer tooltips appear twice only inside conflict blocks. `gui.scroll_mode`, `gui.show_date_in_logs`, `_video_hw_accel` and `_old_diff_custom` repeat in D already (version-ranged). | 6.5 |
| CMake | No source repeated inside one target; every S-added test `.cpp` is registered. | — |

**1.6** This file. Environment: CMake 3.28.3, Ninja 1.11.1, GCC 13.3.0, Clang 18.1.3, Python 3.13.17; no MSVC, no `pwsh`/`powershell`. m2o26's Windows outputs (`test-cache/merge-oct26/`, the staged donor build under `test-cache/m26s/`) do not exist on this machine; Group 2 rebuilds the reference here instead.

## Group 2

**2.1** `git archive origin/mlm-mlv-2sp | tar -x -C test-cache/merge-oct28/s-reference` (commit `66713bdf`, tree `079181de`). The export sits inside this checkout, so every build, test and harness command for it runs with `GIT_CEILING_DIRECTORIES=<repo>/test-cache/merge-oct28`; otherwise Git and CMake's version detection would read this repository's history and stamp the reference with the wrong revision.

**2.2** The r2lbf source hunks were applied in commit order (patches kept in `logs/merge-oct28/s-reference/port-<commit>.patch`). Four hunks were rejected, each for a reason that makes it unnecessary in S: `saveload.h` (S has no `SaveLoad::required_capability`), `surface_supply_order.cpp` twice (S's file has no `std::as_const`, so it needs no `<utility>`), and `tests/surface_supply_order_owner.cpp` (S's file has no `CargoPacket::MAX_COUNT` use). Two `68d0ef40` files do not exist in S. One harmless side effect: the `58c21e51` header hunk added an r2lbf `@see` to S's `surface_supply_order_owner.cpp` in the export only. The S-only R6 site `src/tests/space_return_surface.cpp:39` now copies the footprint before the range-for; Group 8 applies the same fix to the merged tree.

**2.3** GCC 13.3 RelWithDebInfo, `ninja -j4 -k 0 all openttd_space_fixes_test`: 1,542 edges, **0 failed**, 27 min 24 s (`logs/merge-oct28/s-reference/build-gcc.log`). Warnings: 344 `-Wmissing-field-initializers`, 306 `-Wrange-loop-construct`, 76 `-Wunused-function`, 29 `-Wsign-compare`, 13 `-Wmisleading-indentation`, 5 `-Wstringop-overflow`, 5 `-Wredundant-decls`, 1 `-Wmaybe-uninitialized`, 1 `-Wenum-compare`. No `-Wdangling-pointer` remains after the R6 fix.

**2.4** The export's own `native_build.prepare_runtime_assets` filled `build/baseset/` (all five `trg*.grf` present). `ctest -N`: 1,137 tests. `ctest -j4 --timeout 600`: **1,091 passed, 46 failed** in 15 s (`ctest-gcc.log`, `ctest-gcc.xml`). The four `regression_*` tests pass now that the default target is built.

**2.5** `openttd_space_fixes_test`: 245 cases, 510,652 assertions, **0 failures**, about 1 s (`space-fixes.xml`).

**2.6** Ledger: `test-cache/merge-oct28/s-reference-ledger.tsv`. All 46 failures are class **D-fixed**: each one's test file or fixture (`underground_depot_fixture.h`, `tbm_fixture.h`, the Tube environment) was changed on D after the base, by M2O23 (`99d8aa5a`, `0119308a`) or r2lbf, and S never touched any of them. No failure is in an S-only case; there are no S-Linux and no S-behavior failures. A fourth class, D-fixed, was needed beside the plan's three: the failures are base-era defects D already repaired, so for Group 8 they mean "expected to pass once merged".

**Disk:** after 2.3 only 2.3 GiB were free (the reference `build/` is 6.5 GiB). `build-dedicated/` (6.1 GiB of build outputs, only a generated `regression/regression.cfg` copy inside) was deleted; Part 2 Group 31 rebuilds it from scratch anyway. 8.3 GiB free afterwards.
