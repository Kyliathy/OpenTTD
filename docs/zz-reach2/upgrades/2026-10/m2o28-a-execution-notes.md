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

## Group 3

All producers ran headless on Linux with the null video driver, from the export root, with `GIT_CEILING_DIRECTORIES` set as in Group 2. Driver logs are in `logs/merge-oct28/donor445/<run>-driver.log`; each native log and run directory stays in the export (`logs/space-fixes/<run>.log`, `test-cache/space-fixes/<run>/`).

**3.1** Export-only patch to `space_fixes_checks.py`: when `build/<Configuration>/openttd_space_fixes_test.exe` is absent, use `build/openttd_space_fixes_test` (Ninja is single-configuration). Group 8.1 applies the merged-tree form. Results, all exit 0: `--asteroid-preview` on 405 (137 assertions) and 418; `--cargo-origin` on 405 (3 packets, 2 natural; preview, unchanged and resave all true; 112 assertions) and 418 (16 packets; 406 assertions). Each `--cargo-origin` run wrote a fresh `cargo-origins-current.sav`.

**3.2** From retained `space-heavy-440.sav`: `edit` (2,901 assertions) → `continue` (7,486 assertions; before-departure, after-departure, retarget, return-travel and after-unload saves). Light role: `edit-light` (2,860) → `continue-light` (10,214). S's own record had 7,485 assertions for the heavy continuation; the one-assertion difference is in a fresh run, not a failure.

**3.3** `lifecycle` and `lifecycle-hidden` from `m28-continue-heavy/policy-before-departure.sav`: each 107,904 assertions, 1,596 ordinary opportunities, armed delay 1,446, one birth. This matches S's lifecycle record of 1,596 opportunities. All five phase saves are **byte-identical** between the visible and hidden runs. `prepare-high-id` from the after-credit save: 22,071 assertions; 65,526 declared historical turnovers (the declared-prefix limitation stays), then native birth 65536, 4 extracted units, 3,664 opportunities, funded countdown 2,221.

**3.4** `python3 -I test-cache/merge-oct28/collect_donor445.py test-cache/merge-oct28/s-reference test-cache/merge-oct28/donor445 66713bdf 079181de` copied **28 saves** (all `OTTX`, outer version **445**) with `test-cache/merge-oct28/donor445/manifest.json`: per save its run, producer source and arguments, output SHA256 and the executable SHA256 (`openttd_space_fixes_test` `d49e28c2…`). Nothing was written to the tracked retention journal.

**3.5** No producer failed and none needed a real video driver, so no Windows-handoff row comes from this group. These are contemporary reconstructions from retained 405/418/440 originals; historical S441-444 originals stay unavailable and unverified.

## Group 4

**4.1** `git merge --no-ff --no-commit origin/mlm-mlv-2sp` on `merge-2sp-oct28` (`logs/merge-oct28/merge-start.log`): 23 unmerged paths, 221 status entries (S's 222 paths minus the identical `clean.ps1`). The resolution helper `test-cache/merge-oct28/resolve.py <file> <block> <text>` replaces one numbered block and refuses a stale index; resolutions were applied from the last block upward.

**4.2** `saveload.h`: D's tail stays (`MergedLineage = 441`, then `MaxVersion`); the five incoming names follow `MaxVersion` with value `MaxVersion` and a comment naming Groups 9-22 and the "no real games" rule. `required_capability{}` kept. `saveload.cpp` gained `static_assert(SAVEGAME_VERSION == SaveLoadVersion::MergedLineage)`.

**4.3** `cargopacket_sl.cpp`, all five blocks as unions: D's four supply columns and S's two asteroid columns (still on the pending enumerator); `CheckCargoPacketSourceFields` runs D's checks and then S's `ValidateCargoPacketSourceSchema`; native and copied rows decode through `DecodeCargoPacketSource` (which already applies `IsValidCargoPacketSource`), then D's departure/link check; the copied descriptor carries both D's supply columns and S's `source_xy`/`source` columns.

**4.4** `cargopacket.h`: both friends (`AcceptLoadedSurfaceSupplyOrders`, `CAPAChunkHandler`) and `UpgradeLegacyAsteroidSource`.

**4.5** `NativeCargoReferencePreview` and `LoadCheckData::native_cargo_reference_rows` are gone; `NativeCargoOwnerPreview`/`native_cargo_owner_rows` serve both joins. S's bound check "Excessive native cargo meaning references" now guards the owner rows in `station_sl.cpp` and `vehicle_sl.cpp`. `vehicle_sl.cpp` keeps r2lbf's four instantiations and S's `CheckSpaceMiningContinuationHeader`. Discovery: two S tests (`asteroid_cargo_validation.cpp`, `asteroid_cargo_wire.cpp`) build `NativeCargoPacketPreview` positionally with the origin fields right after `feeder_share`, so the merged struct orders S's three fields before D's two supply fields (D only initializes the first two members positionally). S's `asteroid_cargo_validation.cpp` and `space_fixes_native.cpp` now use owner rows.

**4.6** `space_cargo_preview.cpp`: D's descent line, then S's orphan-asteroid loop and `return ValidateSpaceAsteroidCargoPreview(data);`, and S's two functions, which read `native_cargo_owner_rows`. A local `GetOptionalSpaceSupplyLabel` adapts D's `CT_INVALID` label API to S's optional-label semantics at its two call sites. The Group 20 note sits at the label preparation.

**4.7** Every Group 4 file: no markers, `git diff --check` clean, staged.

## Group 5

**5.1** D's `CargoLabel GetSpaceSupplyLabel(SpaceSupply)` kept, S's optional overload removed; the only S caller was the copied preview (4.6).

**5.2** `ValidateAsteroidPacketCustody(const AsteroidField &field)` (S's signature, declared in `asteroid_cargo.h`) now runs on `VisitNativeCargoPackets` with S's checks inside the visitor (canonical source, `ResolveAsteroidCargoOrigin`, valid cargo, Space-plane pickup, no first station, label match). The final pool loop also refuses any non-canonical source. S's hand-written owner walk is gone. The `ValidateSpaceVehicles` call already passed `field`.

**5.3** `SetSpaceMiningPolicy` appended after `SetTubeVehicleGeothermalPower` (223). `merged_command_compatibility.cpp`: `static_assert`s for 222/223 and the suffix expectation `+m441c224p`. S code uses the command only by name.

**5.4** `asteroid_gui.cpp`: `"space_inspector", 560, 380`. The r2lbf key `"space_survey"` was never released, so no `windows.cfg` migration; Group 40 updates `r2lbf-b-windows-handoff.md`.

**5.5** `d2sa-space-adventures.md`: one paragraph with both sides' facts; Group 40 rewords it for writer 446.

**5.6** Checked per file as each was staged.

## Group 6

**6.1-6.2** `toolbar_gui.h/.cpp`: D's `MTHK_SHOW_MAP_LAYER_*`, `MainToolbarEditorHotkeys::ShowMapLayer*` and `show_map_layer_*` INI names. The S `case` lines that merged into both `OnHotkey` bodies without markers (`MTHK_LAYER_*`, `MainToolbarEditorHotkeys::Layer*`, `HandleWindowOpeningHotkey(GameOptions)`) were removed by hand, as was a duplicated `@param` doc line.

**6.3** `ToggleHotkeyWindow` gained S's safeguards for every window-opening key: modal progress → handled, nothing happens; a modal window → it is raised; pinned or `NoClose` targets → raised, never closed. Discovery: S's handler also avoided **recreating** Game Options on a repeated F2 (D's path called `ShowGameOptions()`, which closes and rebuilds the window). `ShowOrRaiseGameOptions()` now raises an existing window or opens a new one, in both toolbars. `HandleWindowOpeningHotkey` was removed from `window.cpp` (taken from the index's merge result so no unrelated whitespace changed) and `window_func.h`.

**6.4** `SelectMainViewportLayer` removed from `map_layer_gui.h/.cpp`; `space_fixes_native.cpp:202` now checks `CHECK_FALSE(ShowMainViewportLayerInDisplayOrder(MAX_MAP_LAYERS))`.

**6.5** One `hotkey_toggles_window` member (D's comment plus the pinned/non-closable rule) and `escape_closes_window`; the unmarked second `[SDTC_BOOL] gui.hotkey_toggles_window` block in `gui_settings.ini` and S's unmarked string pair in `english.txt` were deleted; D's help text now names the raise-intact and pinned/non-closable behavior. The layer tooltips keep D's wording. No `STR_` ID is defined twice.

**6.6** D's Ctrl+Alt comment kept; S's `HotkeyList::Load` migration names (`crash_game`, `money`, `update_coordinates`) match D's global hotkeys. `space_fixes_checks.py` custom hotkey now writes `show_map_layer_space`.

**6.7** `git grep` finds the removed names only in planning documents. The seven header-only blocks were resolved by `test-cache/merge-oct28/resolve_headers.py` (D's description, every `@see` from both sides, plus an m2o28 line), with two duplicated `@see` lines removed afterwards.

## Group 7

**7.1** The 17 clean paths: the four CMake lists add only the incoming units and `openttd_space_fixes_test` (no repeats, checked in 1.5); `cargopacket.cpp` adds `UpgradeLegacyAsteroidSource`; `afterload.cpp` calls `NormalizeLegacyAsteroidCargoPackets` right after `AcceptLoadedAsteroidField` and `saveload.cpp` calls `NormalizeLegacyAsteroidCargoPreview`, both under the pending gate (so for every load, which is right for D441: it has no typed origins); `space_cargo_preview.h` declares the two incoming preview functions; `window.cpp`/`map_layer_gui.cpp` were edited in 6.3/6.4; `clean.ps1` is identical; the six docs wait for Group 40.

**7.2** Pending-gate sites: 32 in production (9 files) and 23 in tests (3 files), as forecast. S-only callers of changed APIs were adapted during Groups 4-6: the copied preview, two S tests on owner rows, the layer test, the harness hotkey name.

**7.3** `ninja -C build -j4 -k 0 openttd openttd_test openttd_space_fixes_test`: 1,187 edges in 31.6 min with **one** failure: `src/tests/transport_tube_provider_switch.cpp:265`, a D test that freezes `Commands::End == 223`. It now asserts `SetSpaceMiningPolicy == 223` and `End == 224`. The rebuild (`build-gcc-2.log`, 45 s) linked all three targets with 0 failures, including `openttd_test`'s POST_BUILD test discovery.

**7.4** No conflict markers anywhere; `git diff --cached --check` clean; `check_swallowed_code.py HEAD` found 0 suspicious lines across the whole merge diff.

**7.5** Merge commit `aa91b303`: first parent `a500f8ea` (descends from `d49c3596`), second parent `66713bdf`. Not pushed.

## Group 8

**8.1** Commit `14a17b91`: the R6 footprint copy in `src/tests/space_return_surface.cpp`, and `native_executable(configuration, name)` in `space_fixes_checks.py` (multi-configuration `build/<config>/<name>.exe` first, else single-configuration `build/<name>`), now used by `space_fixes_checks.py`, `asteroid_lifecycle_work_checks.py`, `space_lifecycle_peer_checks.py` and `space_ordinary_checks.py`. Those harnesses import from `space_fixes_checks`, so run them with `python3 -B` (isolated `-I` mode drops the script directory from `sys.path`).

**8.2** GCC: `ninja -C build -j4 -k 0` (default target) 0 failed; `ctest -N` **1,496** tests (D's 1,361 plus 135 incoming `openttd_test` cases); `ctest -j4 --timeout 600` in 35 s: 1,493 passed, 3 failed (`logs/merge-oct28/baseline/ctest-gcc.{log,xml}`).

**8.3** Clang: `build-clang/` rebuilt with 0 failed in 25.4 min; no `-Wdangling-gsl`. CTest 1,496 tests: 1,495 passed, 1 failed in 32 s; it already included the 8.6 fixes. There are no GCC/Clang differences.

**8.4** `test-cache/linux-baseline/run_focused.sh build logs/merge-oct28/baseline/focused` (its list now includes `openttd_space_fixes_test`): all ten built; nine returned 0. `openttd_space_fixes_test` had 245 cases and 510,652 assertions; one case (2 assertions) failed, the class (b) case below. `openttd_tube_test` now passes as a whole executable. The runner deletes each executable after its run, so rebuild `openttd_space_fixes_test` before using the Space harnesses.

**8.5** Classification of every failure:

| Case | Class | Cause and owner |
| --- | --- | --- |
| `Mining continuation schema distinguishes genuine legacy absence` (`space_mining_wire.cpp:158-159`) | (b) pending-gate | Uses `SaveLoadVersion::AsteroidFieldLifecycle` as an "older than 443" era; under the gate both names equal `MaxVersion`. Group 18. |
| `Finite extraction preserves small tails full holds and original packet provenance` (`probe_extraction.cpp:171`) | (a) S-reference | S's typed origins (r2sff1 Group15) made the test's 16-bit `source.id` check stale; it failed in the S export too (Group 2 had wrongly put it among the D-fixed failures; the ledger is corrected). Fixed in 8.6. |
| `WindowDesc - ini_key validity` (`space_structure_gui.cpp:150`) | (a) S-reference | S gave the construction window the INI key `space_construction` with no sticky or default-size box. Fixed in 8.6. |
| Build failure `transport_tube_provider_switch.cpp:265` | (c) merge | D froze `Commands::End == 223`; fixed in Group 7 (now 224, with `SetSpaceMiningPolicy == 223`). |

**8.6** Commit `8635bd02`: `probe_extraction.cpp` checks `GetAsteroidOrigin()->body`; the construction window gains `WWT_DEFSIZEBOX`. `ctest -R "Finite extraction preserves|WindowDesc - ini_key validity|probe"`: 11/11 passed.

**8.7** **Pending ledger** (class (b); Part 1 must clear all of it by Group 22):

| Pending item | Evidence | Owner |
| --- | --- | --- |
| ~~`Mining continuation schema distinguishes genuine legacy absence`~~ **Cleared in Group 18** (passes in GCC and Clang) | CTest (GCC, Clang) and `openttd_space_fixes_test` | Group 18 |
| Re-saving any world whose packets carry a typed asteroid origin refuses with "Invalid native cargo source for saving". Loading normalizes legacy asteroid cargo to typed origins, so every Space world with asteroid cargo is affected. | 8.8 `space-heavy-440.sav` load and advance | Groups 15 and 22 (writer 446 carries the typed-origin columns) |

The pure codec cases that pass a pending enumerator straight to a codec pass under the gate, as the plan expected.

**8.8** `python3 -B src/tests/multilayer/acceptance/space_ordinary_checks.py --source <save> --mode <load|advance> --run <id>` (real `build/openttd` with the null video driver, paused save, `quit`). Results: `underground/mine-haulage-403.sav` load: exit 0, saved. `space/space-factory-440.sav` load: exit 0, saved. `space/space-heavy-440.sav` load and advance: both load, then saving refuses (the pending item above), with no crash and no corrupt file. The first attempt exposed an S harness defect: `space_ordinary_checks.py` wrote its log without creating `logs/space-fixes/`. It now creates the directory, as its sibling does (commit `31cc118d`).

## Group 9

**9.1** Pending-gate burn-down. Each row names a site that still compares a pending `SaveLoadVersion` enumerator, which equals `MaxVersion` under the Group 4 gate. Its owning group replaces the comparison with a capability query and ticks the row here. Rebuild the list with `grep -rn "SaveLoadVersion::\(AsteroidCargoOrigins\|AsteroidFieldLifecycle\|SpaceMiningContinuation\|SpaceLegacyDeparture\|SpaceSurveyDeparture\)" src`. Line numbers are as of commit `ec5f4c18`.

| Done | File | Lines | Gate | Owner |
| --- | --- | --- | --- | --- |
| [X] | `src/saveload/cargopacket_sl.cpp` | 163-164, 256-257 | CAPA `asteroid_body`/`asteroid_mineral` columns (native and copied descriptors) | 15 |
| [X] | `src/saveload/cargo_source_sl.cpp` | 29, 59 | `DecodeCargoPacketSource` legacy-or-typed branch and the schema check | 15 |
| [X] | `src/saveload/cargo_source_sl.h` | 48 | `IsValidCargoPacketSource` typed-origin acceptance | 15 |
| [X] | `src/tests/asteroid_cargo_wire.cpp` | 25-107 (15 sites) | Pure codec cases that pass `AsteroidCargoOrigins` straight to the codec | 15 |
| [X] | `src/tests/asteroid_cargo_native.cpp` | 215 | Copied-row source check at `AsteroidCargoOrigins` | 15 |
| [X] | `src/saveload/afterload.cpp` | 3528 | Native legacy asteroid cargo normalization | 16 |
| [X] | `src/saveload/asteroid_sl.cpp` | 105-106, 179 | ASTR wire era and SPKN lifecycle state | 17 |
| [X] | `src/saveload/space_order_sl.cpp` | 48, 60-61, 177 | SPOR legacy departure, copied continuation and mining policies | 18 |
| [X] | `src/saveload/space_mining_sl.cpp` | 62 | Continuation schema decoder | 18 |
| [X] | `src/saveload/vehicle_sl.cpp` | 1136-1149, 1295 | VEHS `mission.continuation.*` columns and the missing-schema refusal | 18 |
| [X] | `src/tests/asteroid_cargo_native.cpp` | 87 | Copied mission decision check at `SpaceMiningContinuation` | 18 |
| [X] | `src/tests/space_mining_wire.cpp` | 157-162 | Continuation-schema absence case (the pending CTest failure) | 18 |
| [X] | `src/saveload/saveload.cpp` | 3631 | Copied legacy asteroid cargo normalization during load-check | 21 (custody checks in 20) |
| [X] | `src/saveload/saveload.h` | 525-535 | The five pending enumerators and their interim comment | 22 |
| [X] | `src/saveload/saveload.cpp` | 105-107 | `static_assert(SAVEGAME_VERSION == MergedLineage)` becomes the 446 writer check | 22 |
| [X] | `src/saveload/saveload.cpp` | `DoLoad` preflight (added in Group 13; retired in Group 17B) | Pending refusal of admitted `SaveDialect::SpaceFixes` files (`TOO_NEW_SAVEGAME`, "admitted but not yet readable") | 21 (lift once 15-20 read every era; 442-445 also need 22's MaxVersion) |

These sites read the accepted dialect directly instead of a pending enumerator. Once the classifier admits S files, `SaveDialect::SpaceFixes` must reach them as the A-fixed supply wire:

| Done | File | Line | Current rule | Owner |
| --- | --- | --- | --- | --- |
| [X] | `src/saveload/surface_supply_order_sl.cpp` | 120 | Fixed or tagged SSOR accepted only for `DestinationA` and `Merged` | 13 (classifier), 21 (load order) |
| [X] | `src/saveload/surface_supply_order_sl.cpp` | 260 | `legacy_a_supply_wire = dialect == DestinationA` (copied preview) | 13, 20 |

Both rows closed in Group 20 without a code change: Group 17B removed `SaveDialect::SpaceFixes`, so development-build files refuse in `DoLoad` before any chunk reader runs, and these two sites only ever see A, B or merged sources.

**9.2** `src/saveload/save_lineage.h` appends `SaveDialect::SpaceFixes` (the S writer, 441-445 without MLLN) and five capabilities before `Count`: `AsteroidCargoOrigins`, `AsteroidFieldLifecycle`, `SpaceMiningContinuation`, `SpaceLegacyDeparture` and `SpaceSurveyDeparture`. It adds `CURRENT_MERGED_SAVE_VERSION = 446` and `CURRENT_MERGED_SAVE_LINEAGE_SCHEMA = 2` beside the frozen 441/schema 1 constants. They stay reserved: nothing reads them until Groups 13 and 22. No code switches over `SaveDialect`, so the append needs no other edits.

**9.3** `src/tests/save_lineage_policy.cpp`:

- `CAPABILITY_EXPECTATIONS` gains five `SpaceFixes` rows at 441-445, so its `static_assert` on `Count` holds.
- `SPACE_FIXES_MATRIX` writes the plan's seven-row matrix out literally.
- The new case `Save lineage freezes the Space fixes source-format matrix` checks the five columns per row. It also checks every older capability: merged sources carry all of A and B, S sources all of A and none of B.
- The merged-marker case now expects only the A/B capabilities, because merged 441 must lack the five.
- The new case uses `AcceptedSource(dialect, version)` because the classifier cannot produce `SpaceFixes` or 446 before Group 13.
- It is tagged `[!mayfail]` until Group 10.

Quick check without the native tree: `g++ -std=c++20 -Wall -Wextra -DSAVE_LINEAGE_STANDALONE -Isrc src/tests/save_lineage_policy.cpp src/saveload/save_lineage.cpp -o test-cache/merge-oct28/lineage/policy`, then run it (11 s to compile, no warnings). Result: 6 cases, 5 passed and 1 failed as expected (131 matrix assertions, as forecast: the positional rule gives merged 441 the five, and gives S sources nothing).

**9.4** The header comment on the reserved constants says that S's eras are capability rules of `SpaceFixes`, that `MergedLineage = 441` keeps its meaning, and that Group 22 deletes the pending enumerators. The comment on the frozen 441 constants, which still pointed at the old Group44 allocation plan, now says what 441 means.


## Group 10

**10.1** Commit `074285ff`. `HasSaveCapability` now uses one `CapabilityRule` row per capability: first A, first B and first Space fixes version, plus merged 441 and merged 446 membership. It looks the row up by capability, never by enum position.

- Two `static_assert`s, one on the table size and one on each value occurring exactly once, force a row for every future append. That row must choose its merged 441 value explicitly.
- `WriterProducedVersion` first checks each writer's closed range: A 425-440, B 425-434, Space fixes 441-445, merged exactly 441 or 446, Shared none. A version outside that range grants nothing.
- Space fixes rows give every A capability from 441, because that writer began from A440. They give no B capability.

**10.2** The Group 9 matrix case is no longer `[!mayfail]`. A new case covers:

- each Space fixes introduction at n-1/n;
- both 441 dialects;
- foreign versions (S446, merged 442/445/447, A441, B435, Shared, an undeclared dialect value);
- refused statuses;
- `Count` and 255 as unknown capabilities.

Standalone build: 7 cases, 1,459 assertions, all passed.

**10.3** `SlHasSaveCapability`'s writer context is now always `{Accepted, Merged, SAVEGAME_VERSION}`. The `DestinationA` fallback was dead code, because the Group 4 `static_assert` pins the writer to 441. So the five Space fixes capabilities stay off when saving until Group 22. The load context still answers from the bound lineage. `DestinationVersionCapability` is unchanged and has no pending enumerators.

**10.4** `ScopedLoadLineage` clears `_sl.lineage` on construction and destruction, and `SlNullPointers` clears it on failure cleanup. `Bind` stores the classifier result unchanged, so `original_version` stays the envelope version. `GetLoadCapabilities` returns that stored value.

GCC: `ninja -C build openttd_test openttd`, 299 steps in 11 min 49 s, exit 0. Its 308 warnings are all in files this group did not touch.

- `ctest -R "save-lineage|lineage"`: 20/20 passed. These are the 7 policy cases, the 4 native lineage cases and 9 utility-lineage cases (`logs/merge-oct28/group10/ctest-gcc-lineage.{log,xml}`).
- Wider save-load subset `-R "Original|Native scalar|supply wire|merged|Merged|Save envelope|copied|Copied"`: 53/53 passed (`ctest-gcc-saveload.log`).

Clang: `ninja -C build-clang openttd_test`, 298 steps in 9 min 45 s, exit 0. It printed 113 warnings, none in a file this group touched. Lineage subset 20/20 and save-load subset 53/53 passed (`logs/merge-oct28/group10/ctest-clang-{lineage,saveload}.log`). There are no GCC/Clang differences.

## Group 11

**11.1** Commit `463c6626`. `save_lineage.h` adds `SupplyColumnEncoding {Absent, LegacyU32Bool, CanonicalU64U8, Malformed}`. `SaveWireFacts` gains two fields with inert defaults: `cargo_supply_encoding` and `asteroid_origin_columns` (a `SaveEvidenceState`). `supply_columns` and `merged_cargo_valid` stay, now derived from the new observation:

- legacy pair → `supply_columns` Complete;
- malformed pair → `supply_columns` Malformed;
- canonical pair → `merged_cargo_valid`, and the merged packet columns become required.

The classifier's malformed-first rule now also refuses either new observation when it is Malformed.

**11.2** `CheckSelectedColumns` reads both pairs from the CAPA descriptor through `ObserveSupplyColumns` and `ObserveAsteroidOriginColumns`, never from `original_version == 441`.

- Native type codes: U32/I8 for legacy B, U64/U8 for merged, U32/U32 for the asteroid key.
- A half pair or any other widths is recorded as Malformed. It no longer throws, so the classifier refuses it as Corrupt.
- Duplicate names still throw in `ReadColumns`.
- Behaviour change in existing tests: three cases that expected a thrown `SaveFramingError` (half pair, mixed pair, legacy envelope with a merged marker) now check the Malformed fact and a Corrupt classification. Both paths refuse the load with `STR_GAME_SAVELOAD_ERROR_INVALID_LINEAGE`.

**11.3** `ReadRiff`'s MLLN case keeps `exact(8)`: a wrong length is still a framing error. After that it records the tuple. Only `(441, schema 1)` and `(446, schema 2)` with magic MLN1 and `writer == original_version` stay Valid; any other tuple marks the family Malformed. The envelope version is known before any chunk, so the result does not depend on chunk order.

The SSOR branch still chooses its decoder from `original_version == 441`. Group 12.1 removes that. So a synthetic S441 *with* SSOR is not yet scannable, and the Group 11 fixtures leave SSOR out.

**11.4** `src/tests/save_envelope_cargo_columns.cpp` (registered in `openttd_test` and `openttd_tube_test`) builds every stream byte by byte. It has four cases:

- each writer's columns and marker: D441, S441, S445, 446, legacy B434, empty versus populated CAPA, marker first versus last;
- malformed pairs and six bad marker tuples, plus a short marker as a framing error;
- exact work ceilings passing and one less failing for fields, rows and chunks, with every truncation of two streams refusing and rewinding;
- an optional real-save scan.

`Observe` checks that the reader is rewound, the input is unchanged and the digest equals SHA-256 of the input.

Results (GCC, `logs/merge-oct28/group11/`):

- `ninja -C build openttd_test` took 11 min 58 s, exit 0, with no warnings in the touched files.
- `ctest -R "Save envelope|Save lineage|Native lineage|Original|Native scalar|supply wire|merged|Merged|copied|Copied"`: 65/65 passed (`ctest-gcc.{log,xml}`).
- `OTTD_DONOR445_DIR=$PWD/test-cache/merge-oct28/donor445 build/openttd_test "Save envelope observes typed CAPA columns in real donor 445 saves"` scanned all 28 real S445 saves (169 assertions, `donor445-scan.log`). Each is version 445, with typed asteroid columns, no supply pair and no MLLN.

Clang is deferred to the Group 13 close, because each header change rebuilds about 300 objects.

## Group 12

**12.1-12.2** Commit `e185eb8c`. `ReadRiff`'s SSOR case calls `ObserveSupplyBook`, which never looks at the version or at MLLN.

- It reads a header-sized prefix (20 bytes, `MERGED_SURFACE_SUPPLY_HEADER_SIZE`).
- The fixed A440 shape fits when the little-endian count at bytes 12-13 is at most 16384 and the extent is exactly 14 + 53 * n.
- The SSM1 signature is the first word in network order. Only when it is present does the streaming `InspectMergedSurfaceSupplyOrders` run, with a reader that replays the prefix and then continues from the payload. No payload-sized buffer and no receipt vector exist.
- The result goes into `SaveWireFacts::supply_book_shape`:
  - both readings validate → `Ambiguous`;
  - only the tagged book validates → `Tagged`;
  - signature present but the book is invalid → `Malformed` (it never falls back to the fixed reading);
  - no signature and no fixed fit → `Malformed`;
  - otherwise → `Fixed`.
- `merged_supply_valid` and `merged_supply_next_id` are set whenever the tagged book validates.
- The classifier's malformed-first rule refuses `Malformed` and `Ambiguous`. The positive shape requirements (merged needs Tagged, A/S need Fixed) belong to Group 13.

**Risk:** a real merged book whose bytes also satisfy the fixed equation would be refused as Ambiguous. That needs `20 + 54a + 39b == 14 + 53n`, with n taken from the frontier's low 16 bits byte-swapped. It is rare, but possible for some frontiers. The fixture `OverlappingBook()` is one such case: 8 LegacyB rows with next ID 0x600. Group 13 decides whether a valid marker settles this overlap or the plan's refusal stands.

**12.3** Inventory of S-only schema changes, by family:

| Family | Era | Change |
| --- | --- | --- |
| CAPA | 441 | Adds `asteroid_body` and `asteroid_mineral` (U32). Observed in Group 11. |
| ASTR | 442 | Adds the 62-byte lifecycle trailer (`ASTEROID_LIFECYCLE_WIRE_BYTES`) and sparse body IDs. The era maximum comes from `GetAsteroidFieldWireMaximum`. |
| ASTR | 445 | Survey records may carry no target (ownerless return/waiting). |
| SPKN | 442 | Sorted, full-width sparse IDs, bounded by `MAX_ORBITAL_BODIES` instead of the dense population. |
| SPOR | 443 | Order rows grow from 14 to 15 bytes (the policy byte). |
| SPOR | 444 | Admits unqueued legacy departure. |
| VEHS | 443 | Adds the 14 `mission.continuation.*` columns. |
| SDEF | all | S writes content version 3, as A440 does. |

None of these is needed to decide the writer. Typed CAPA columns (Group 11) and the fixed SSOR (Group 12) identify S441-445 positively, and each family's era is validated by its native decoder once the source is known. So no ASTR/SPKN/SPOR/VEHS fact was added.

**12.4** VEHS is not inspected, so its conditional nested-schema path is not needed, and there are no non-Space vehicle cases to add.

**12.5** `src/tests/save_envelope_supply_book.cpp` holds five cases:

- fixed SSOR read by shape in A440, S441 (marker-free, CAPA first or last) and S445, with every truncation rewinding;
- tagged SSOR in D441 and 446 with the marker first or last, plus an empty tagged book;
- historical B `SSOH` + `SSOR` TABLE kept outside the RIFF shapes;
- signature collisions: fixed-shaped books starting with SSM1 become Malformed, and the true overlap becomes Ambiguous;
- refusals: too short, missing records, count 16385, a truncated or over-long tagged book (all Malformed), plus duplicate SSOR, an over-claimed RIFF length and SSOR as a sparse array (framing errors with rewind).

The byte-level builders now live in `src/tests/save_envelope_chunks.h`, shared with Group 11's file. `save_lineage_policy.cpp` refuses `Malformed` and `Ambiguous` on complete merged and A440 facts, and accepts `Tagged` and `Fixed` there.

Results (GCC, `logs/merge-oct28/group12/`):

- Build: 12 min 4 s, exit 0, no warnings in the touched files.
- The focused subset (the Group 11 pattern plus `Merged supply`): 70/70 passed (`ctest-gcc.{log,xml}`).
- With the donor scan extended to the SSOR shape, all 28 real S445 saves read as `Fixed` (197 assertions, `donor445-scan.log`).


## Group 13

**13.1-13.3** Commit `6cd8a5ba`. `ClassifySaveDialect` works in this order:

1. Malformed evidence refuses first: framing, common transport, Group 11's column observations, the `Malformed` book shape and every Malformed family.
2. Versions above 446 are `Unsupported`.
3. A present marker goes to `CompatibleMerged`. It needs exactly 441/schema 1 or 446/schema 2, with the writer equal to the envelope version, plus:
   - the canonical CAPA pair;
   - the tagged book (Tagged or Ambiguous);
   - no typed origins at 441 and typed origins at 446;
   - every retained family check from the old marker branch.
4. An unmarked 446 is Corrupt.
5. A markerless 441-445 goes to `CompatibleSpaceFixes` and is never given a hint. It needs:
   - typed CAPA origins;
   - the Fixed book as a RIFF family;
   - no B metadata bits, CAPA supply pair, B layout or merged book;
   - none of the seven B families;
   - content 0 or 3, with CDIM and SPRC present and `ValidSpaceFamilies`.
6. Everything else goes to `ClassifyLegacy`: the old A/B/shared body and its hint semantics. It now also refuses typed origins, the canonical pair or an SSM1 book at 440 and below. `CompatibleA`'s A440 rule also requires `SupplyBookShape::Fixed`.

The version constants moved to the top of `save_lineage.cpp`.

Plan change, recorded in 13.3: an SSOR overlap (`Ambiguous`) no longer always refuses. A marked file takes the tagged book; a markerless file refuses. Group 12's notes give the collision arithmetic. Always refusing would have made some genuine D441 saves unloadable.

**13.4** `DoLoad` binds the result as before. A new pending refusal follows the accepted-status check: `SaveDialect::SpaceFixes` refuses with `STR_GAME_SAVELOAD_ERROR_TOO_NEW_SAVEGAME` and "admitted but not yet readable". It is a burn-down row owned by Group 21. World initialization did not move.

Three real S445 saves from Group 3 are committed under `src/tests/fixtures/saveload/space-fixes-445/`: typed-origin packets, mining continuation and field lifecycle. `catalogue.json` records the writer, the producer runs and the SHA-256 of each file.

Tests:

- `save_lineage_policy.cpp` builds facts with `Merged446()` and `SpaceFixes(version)`. It covers:
  - both 441 writers, and each one's proof planted in the other's file;
  - S441-445 accepted, including Classic content 0 and hints ignored, with ten foreign-evidence refusals;
  - marked 446 accepted, with refusals for missing origins, the wrong schema, an unmarked file, markers on 442-445, and 447 as Unsupported;
  - the overlap settled by the marker;
  - the A442/A447 expectations updated to Corrupt/Unsupported.
  Standalone result: 10 cases, 1,537 assertions.
- `save_lineage_preflight.cpp` adds `Native preflight admits Space fixes originals and keeps them pending`. Each catalogued original:
  - has the catalogued digest;
  - classifies as SpaceFixes 445 (all five Space capabilities and all of A's, none of B's), and a DonorB hint is ignored;
  - is refused by the network reader, `Check` (`TOO_NEW_SAVEGAME`) and `Load` without one native read or a retained lineage.
  The open world (vehicle, money, map, tick, RNG) is unchanged.
- The optional donor scan also classifies each of the 28 real saves.

GCC results (`logs/merge-oct28/group13/`):

- Build: 12 min 3 s, exit 0.
- Focused subset: 74/74 (`ctest-gcc.{log,xml}`).
- A wider save/load subset `-R "[Ss]ave|[Ll]oad|[Pp]review|[Ll]ineage|[Oo]riginal|[Rr]etained|[Cc]opied|[Rr]eload|[Ss]pace fixes|[Aa]steroid"`: 369/369 (`ctest-gcc-wide.{log,xml}`).
- Donor scan: all 28 saves classify as SpaceFixes 445 (253 assertions, `donor445-scan.log`).

Clang (Groups 11-13 together; deferred from Group 11):

- `ninja -C build-clang openttd_test` took 10 min 2 s, exit 0, with no warnings in the touched files.
- Focused subset: 74/74. Wide save/load subset: 369/369 (`logs/merge-oct28/group13/ctest-clang{,-wide}.log`).
- There are no GCC/Clang differences.

## Group 14

The merge commit already carried the value union, so this group adds proof. Commit `e1c630dd`, plus the pool-check fix below, adds three cases to `src/tests/asteroid_cargo_origin.cpp`:

- **14.1** `Merged source values keep asteroid identity beside ordinary and company-supply sources` checks:
  - 65536 against body 0, and body 0 as a real origin;
  - the mineral as part of the identity;
  - the invalid sentinel equal to `Source{Invalid, Asteroid}`;
  - ordinary equality across domains;
  - a CompanySupply source next to asteroid cargo, with no payload and not mergeable with it;
  - `SetIndex` on legacy and typed asteroid sources changing nothing;
  - `CargoPacket::InvalidateAllFrom(company supply)` never touching asteroid packets.
- **14.3** `Legacy asteroid upgrade refuses invalid, ordinary and already-typed mismatches`:
  - invalid, Industry, CompanySupply, an empty mineral, and a typed source with another mineral all refuse unchanged;
  - a real legacy ID upgrades once, a repeat agrees, and quantity and credit stay the same.
  - The two friend hooks keep their existing owners and tests:
    - `AcceptLoadedSurfaceSupplyOrders` normalizes linked B purpose at `surface_supply_order_runtime.cpp:404`, covered by `surface_supply_adoption.cpp`;
    - `CAPAChunkHandler` installs decoded sources, covered by the native CAPA round trips. Its capability gating is Group 15.
- **14.4** `Native packet split, merge and reduce keep every merged packet fact`:
  - the historical-restore constructor gives age 7 and credit 100, and both survive Split (4/6, credit summing to 100), Merge (back to 10/100) and Reduce (7/70);
  - a CompanySupply packet with receipt 2^32+5 and departure 1, and a typed 65536 origin, keep source, receipt and raw departure on every fragment after Split, Merge and Reduce.
  - The first run aborted on the debug pool check (`pool_func.hpp:119 checked != 0`). `Split` calls `CanAllocateItem()` itself and resets the pre-check, so the case now re-checks before its later creations.

**14.2** Label API audit. `GetSpaceSupplyLabel` has three callers:

- `GetSpaceSupplyCargo` guards with `supply < End`.
- The preview owner join in `space_cargo_preview.cpp:115,129` passes the label to `admit`, which refuses `CT_INVALID` and empty labels.
- The asteroid-origin join adapts it through `GetOptionalSpaceSupplyLabel` (`CT_INVALID` → no label).

The runtime callers of `GetSpaceSupplyCargo` all check `IsValidCargoType` before indexing, and their execute paths follow the same preflight:

- `space_manifest.cpp:110,260`
- `space_structure.cpp:443`
- `orbital_service.cpp:155,423-455,473`
- `orbital_freight.cpp:63-101`
- `cargo_custody.cpp:86,112`

None reads the current `CargoSpec` for a compartment label; the labels are the fixed PROP/RFMT/ELEC table.

One exception: the GUI draw at `space_structure_gui.cpp:719` indexes `module.stock[GetSpaceSupplyCargo(...)]` with no check. The line is identical in D (`d49c3596`, line 489) and S (line 716), so the merge did not introduce it. It is left for Part 2's runtime work.

Results (`logs/merge-oct28/group14/`):

- GCC incremental build, exit 0.
- `ctest -R "[Aa]steroid|[Ss]ource|[Ss]upply|[Cc]argo|[Pp]acket|[Mm]anifest|[Ss]ervice"`: 312/312 (`ctest-gcc.{log,xml}`).
- Clang incremental build and the same subset: 312/312 (`ctest-clang.log`).

## Group 15

**15.1** Commit `3882a7ce`. `cargo_source_sl.h` adds `CargoSourceFormat {version, typed_asteroid_origins}`:

- `FromLineage(result)` takes the envelope version and `HasSaveCapability(result, AsteroidCargoOrigins)`.
- `Current()` takes `_sl_version` and `SlHasSaveCapability(AsteroidCargoOrigins)`. That is the bound lineage while loading, and the writer while saving, because a save sets `_sl_version = SAVEGAME_VERSION` and binds no lineage.
- A one-argument implicit constructor from a shared-era `SaveLoadVersion` keeps D's and S's shared-gate tests readable (TableChunks, SpaceMiningVehicles, CompanySupplyPackets, SurfaceSupplyOrders), with `typed_asteroid_origins = false`. It asserts the version is not `MaxVersion`, which every pending Space fixes name equals.
- `IsValidCargoPacketSource`, `DecodeCargoPacketSource` and `ValidateCargoPacketSourceSchema` take the format. Their typed-origin branch reads `format.typed_asteroid_origins`; no version comparison decides it.

**15.2** Both CAPA descriptors (native and LoadCheck) declare `asteroid_body`/`asteroid_mineral` with the default version range plus `.WithCapability(SaveCapability::AsteroidCargoOrigins)`.

- The B U32/Bool and merged U64/U8 supply rows are unchanged.
- On save the writer is merged 441, which lacks the capability, so the columns are not written. The typed-origin save refusal stays pending until Group 22 (pending ledger).
- `CheckCargoPacketSourceFields` still sees every header key, including unknown skipped ones, and calls the schema check with `Current()`.
- Wrong widths and half pairs on a merged 441 or legacy file are already refused at preflight by Group 11's descriptor observations, before any native handler runs.
- The dead `extern _sl_version` and `extern SAVEGAME_VERSION` lines in `cargopacket_sl.cpp` were removed; `cargo_source_sl.cpp` now declares `_sl_version` the same way `vehicle_sl.cpp` does.

**15.3** Verified, not changed:

- `_cargo_source_asteroid_wire = {}` already runs before every native and copied `SlObject`, and Save fills it from the packet.
- `SlIterateArray` yields no row for a pool hole, so a hole never reaches `SlObject` or the decoder, and the next real row starts from a reset scratch.
- Copied rows keep `source_xy`, `source` (first station) and D's receipt and departure fields.
- Decoding runs after the identity and quantity checks in both paths.

**15.4** Tests:

- The pending-era arguments in `asteroid_cargo_wire.cpp` now name `AcceptedFormat(SaveDialect::SpaceFixes, 441)`.
- `asteroid_cargo_native.cpp:215` judges loaded packets against the combined writer's format, `FromLineage({Accepted, Merged, 446})`.
- New case `CAPA source format follows the accepted writer, never the number`:
  - typed schemas pass only for S441-445 and merged 446; merged 441 and B434 forbid them; half pairs refuse everywhere;
  - a high ID decodes only where typed origins exist;
  - merged 441 keeps D's legacy scalar for the later upgrade;
  - an ordinary source with a payload refuses in every format;
  - bought supplies are refused for historical B.
- New case `CAPA rows of real Space fixes 445 saves decode through the accepted format` (23,418 assertions). For each committed S445 original it:
  - decompresses the file with a fresh codec and parses CAPA from its own descriptor;
  - checks the schema passes as S445 and fails as merged 441;
  - checks every real row decodes as S445, and every typed row refuses as merged 441.
  This is the "reading Group 3's S445 files" check of 15.5 at codec level. Full native reading of 442-445 needs MaxVersion > 445, which arrives with Group 22, and `DoLoad` refuses SpaceFixes as pending until Group 21.
- Raw departure 2 and a U64 receipt above `UINT32_MAX` stay covered by D's merged refusal fixture (`save_merged_refusal_fixture.h` faults 18 and 19) inside the subset below.

**15.5** Burn-down rows ticked: the CAPA descriptors, the codec `.cpp`/`.h`, the wire tests and the native line 215. Only Group 16's two normalization gates still name `AsteroidCargoOrigins` outside `saveload.h`.

GCC (`logs/merge-oct28/group15/`): `ctest -R "[Aa]steroid|[Ss]ource|[Ss]upply|[Cc]argo|[Pp]acket|Save envelope|Save lineage|Native lineage|Native preflight|Original|[Cc]opied|[Mm]erged"` passed 320/320 (`ctest-gcc.{log,xml}`).

The first build caught a real `-Wdangling-reference` in the new real-file case: `SupplyWireFind` returns a reference into the chunk vector, so that vector is now kept in a local.

Clang: incremental `ninja -C build-clang openttd_test` (10 steps) exit 0 with no warnings; the same subset passed 320/320 (`ctest-clang.log`).

## Group 16

**16.1** The Group 5 function `ValidateAsteroidPacketCustody(const AsteroidField &)` (`space_vehicle.cpp:569`) was already complete in the merge. It runs on `VisitNativeCargoPackets` and checks, in order:

1. it refuses a non-canonical source;
2. for an asteroid packet, it resolves the origin, collects the identity once, and requires the container cargo label to equal the mineral, the source tile to be on the Space plane, and the first station to be Invalid;
3. after the traversal, it refuses any allocated packet that is non-canonical, and any asteroid packet that has no owner.

It is still called at the end of `ValidateSpaceVehicles` (`space_vehicle.cpp:659`), as in both S and D. No code change was needed.

**16.2** `cargo_custody.cpp` visits each of these once:

- station goods and launch-hub handling;
- every vehicle's cargo, including Space freight;
- the three manifest compartments;
- pod descent jobs;
- construction inputs;
- module stock;
- every service-job berth;
- manufacture inputs, all four slots, with unused slots required to be empty.

New case `Asteroid custody visits the fourth manufacture input slot`:

- It puts mined ore into slot 4 of a heavy-miner job. That recipe (STEL, GOOD, RBPT, ELEC) is the only one with four inputs.
- Recording the shared visitor's own callbacks shows the packet visited exactly once, with that slot's ELEC cargo and an Invalid station. Because slot 4 holds ELEC, refusal alone could not tell "visited and relabelled" from "not visited".
- Custody refuses the packet there, and accepts it again once it moves to module stock of its own cargo.

**16.3** Commit (this group) changes two gates:

- `afterload.cpp` calls `NormalizeLegacyAsteroidCargoPackets` when `!SlHasSaveCapability(SaveCapability::AsteroidCargoOrigins)`.
- `saveload.cpp`'s LoadCheck calls `NormalizeLegacyAsteroidCargoPreview` under the same condition.

Both run inside the `ScopedLoadLineage` scope, so the source's own capability answers. Shared, A, B and merged 441 sources normalize. Space fixes and merged 446 sources already carry typed origins, so they skip it, and their codec refuses unqualified scalars. Both functions already preflight every upgrade before applying any.

The burn-down rows for both are ticked. `git grep "SaveLoadVersion::AsteroidCargoOrigins" src` now finds only the `saveload.h` definition.

**16.4** New case `Asteroid custody refuses every unsupported origin and keeps ordinary and bought cargo alongside`:

- An Industry packet and a CompanySupply packet next to mined ore keep custody valid.
- Each of these refuses on its own:
  - a never-issued origin (`next_body_id`);
  - a wrong mineral for a live body;
  - a legacy ID with no surviving body;
  - a correct origin with a Surface pickup tile;
  - an allocated asteroid packet in no container.
- Added packets are raw list references, as in the existing duplicate-alias check, so `Append` cannot merge and delete them.

The retired-origin and duplicate cases already exist in the six-owner case, and the atomic missing-body normalization in `asteroid_cargo_wire.cpp`.

Results (`logs/merge-oct28/group16/`):

- The `[asteroid-cargo-custody]` tag: 3 cases, 999 assertions.
- Wide subset `-R "[Ss]ave|[Ll]oad|[Pp]review|[Ll]ineage|[Oo]riginal|[Rr]etained|[Cc]opied|[Rr]eload|[Ss]pace fixes|[Aa]steroid|[Ss]ource|[Cc]argo|[Pp]acket|[Cc]ustody"`: GCC 519/519 (`ctest-gcc.{log,xml}`), Clang 519/519 (`ctest-clang.log`), no new warnings.
- The first builds had three compile errors in the new test: a missing `space_mining_tuning.h`, a brace-initialized comparison, and the station-to-station `Move` taking no tile. All are fixed.

## Group 17

**Defect found first.** The interim merged-441 writer wrote S's newest ASTR layout, with the 62-byte lifecycle trailer, into a 441 file, and a merged-441 reader expects StarterInputs. Reproduction: the Group 8 resave of `space-factory-440.sav` (`test-cache/space-fixes/m28-smoke2-factory-load/ordinary-current.sav`) fails to reload with "Invalid asteroid field lifecycle or record" (`logs/space-fixes/m28-astr-reload.log`). This has been the case since the merge commit.

**17.1** `asteroid_sl.cpp` gains one owner-local `SelectAsteroidFieldWireEra()`:

- the shared numeric gates `SpaceMiningVehicles` (405) and `AsteroidStarterInputs` (424) stay;
- without `AsteroidFieldLifecycle` the era is StarterInputs (merged 441);
- with it, the era is SurveyDeparture if `SpaceSurveyDeparture` is also present, otherwise Lifecycle.

While saving, `IsSavegameVersionBefore` sees the writer version and `SlHasSaveCapability` the writer context, so the same selector names the writer's own layout.

**17.2** The selector is used three times:

- ASTR native `Read` and LoadCheck share it, with the bounded length check kept before allocation.
- `Save` uses it too.
- SPKN's sparse/dense choice uses `SlHasSaveCapability(AsteroidFieldLifecycle)`; the next-ID and knowledge semantics are unchanged.

`EncodeAsteroidField(field, era = SurveyDeparture)` takes the era:

- It refuses eras before StarterInputs and unknown eras.
- It omits the lifecycle trailer below Lifecycle.
- Any layout older than the current one goes through `ProveOlderAsteroidLayout`. That decodes the bytes with the same layout, re-encodes both fields in the current layout and compares bytes. Survey knowledge (an SPKN concern) and the derived lookup do not take part.
- `AsteroidField::operator==` could not be used for this: it also compares knowledge and the raw body-vector order, which retirement permutes.

Behaviour that follows:

- A Space world whose field is still dense with seed-derived renewal saves at 441 and reloads exactly.
- After the first asteroid exit, the 441 writer refuses ("field state this writer's layout cannot hold") instead of writing an unreadable file. That stays pending until Group 22's writer 446 (pending ledger).
- Elapsed time alone does not change the state: an unarmed renewal timer stays at zero.

**17.3** `asteroid_field_wire.cpp` adds `ASTR writer layouts are lossless or refuse`:

- the default and Lifecycle eras give the current bytes;
- StarterInputs gives the current bytes minus the trailer, and decodes to the same field;
- survey knowledge does not block StarterInputs;
- an advanced unarmed timer is unchanged;
- a retired body refuses StarterInputs but encodes as Lifecycle and keeps its renewal;
- Preview, NativeMining and End write nothing;
- the inactive field encodes in every writer era.

S's existing cases already cover every era's exact trailer and row extents, unearned credit, sparse high IDs, absent-history defaults and the RNG words. The ASTR/SPKN burn-down row is ticked.

**Evidence** (`logs/merge-oct28/group17/`):

- `[asteroid-field-wire]`: 5 cases, 285 assertions.
- Subset `-R "[Aa]steroid|ASTR|SPKN|[Ss]urvey|[Kk]nowledge|[Ss]ave|[Ll]oad|[Pp]review|[Ll]ineage|[Oo]riginal|[Cc]opied|[Rr]eload|[Ss]pace fixes"`: GCC 376/376, Clang 376/376.
- Application: a fresh resave of `space-factory-440.sav` now reloads past ASTR. It stops at SPOR ("Invalid Space order count"), the same writer-era mismatch for order rows (14 versus 15 bytes). That is Group 18's to fix.

**User decisions recorded during this group:**

- Saves written by the incoming dev build (S 441-445) need not load.
- The lineage vocabulary is renamed to format names after Group 22.

The plan is amended below, as Group 17B and in the Group 18-22 text.

## Group 17B

Commit `19f4df73`, after the plan change `b4e13ae0`.

**17B.1** Removed, with no other use left:

- `SaveDialect::SpaceFixes`;
- `CompatibleSpaceFixes`;
- the `first_space` rule column;
- the Space fixes range in `WriterProducedVersion`, and its `HasSaveCapability` case.

The five capabilities are merged-446 rows (`{0, 0, false, true}`). The classifier now refuses any markerless 441-445 as Unsupported. The range constants are `FIRST/LAST_UNMARKED_DEVELOPMENT_VERSION` (441/445). The check runs after the malformed-first checks and the marker branch, and before the legacy path, so no hint is consulted.

**17B.2** `DoLoad` maps Unsupported to `STR_GAME_SAVELOAD_ERROR_TOO_NEW_SAVEGAME` above 446. At or below 446 it maps to the new `STR_GAME_SAVELOAD_ERROR_DEVELOPMENT_SAVEGAME`: "Savegame was written by a development build whose format this version does not read". The Group 13 pending refusal is gone; its burn-down row is ticked as retired.

Comments that still said "Space fixes" were updated in `save_lineage.*`, `saveload.cpp`, `cargo_source_sl.h`, `cargopacket_sl.cpp` and `asteroid_sl.cpp`. The ASTR selector never picks the Lifecycle-only layout now.

**17B.3** Tests:

- `save_lineage_policy.cpp`:
  - `DevelopmentBuild(version)` replaces `SpaceFixes(version)`;
  - the five expectation rows are `{Merged, 446}`;
  - the matrix is reduced to merged 441 against 446;
  - new cases: `grants the Space eras only to merged 446`, `reads merged 441 and refuses a markerless 441`, and `refuses every markerless 441-445 as a development build` (hints, Classic and changed content stay Unsupported; malformed evidence is still Corrupt).
- `save_lineage_preflight.cpp`: `Native preflight refuses development-build originals without touching the world`, checking for `DEVELOPMENT_SAVEGAME`.
- `asteroid_cargo_wire.cpp`: typed-origin contexts use merged 446. The real-row case becomes `CAPA rows of real typed-origin saves decode through the merged 446 format`, because these saves have the same column shape as 446.
- The donor scan expects Unsupported.

**17B.4** Results (`logs/merge-oct28/group17b/`):

- The standalone policy build: 10 cases, 1,333 assertions.
- GCC full rebuild (the new string regenerates the tables): 19 min 7 s.
- Wide subset `-R "[Ss]ave|[Ll]oad|[Pp]review|[Ll]ineage|[Oo]riginal|[Rr]etained|[Cc]opied|[Rr]eload|[Ss]pace fixes|[Aa]steroid|[Ss]ource|[Cc]argo|[Pp]acket|[Cc]ustody|ASTR|[Mm]erged"`: GCC 524/524, Clang 524/524 (616 steps).
- `OTTD_DONOR445_DIR` scan: all 28 donor saves refuse as Unsupported (225 assertions).
- Warnings seen in the build (`asteroid_gui.cpp`, `space_resupply.cpp`, `cargo_return_native.cpp`) are in files this group did not touch.

## Group 18

**18.1** `ValidateSpaceMiningContinuationSchema(fields, bool supported)` replaces the version argument. `supported` is `SlHasSaveCapability(SpaceMiningContinuation)`, which only merged 446 has. All fourteen keys are required once where supported and forbidden otherwise, including a skipped unknown `mission.continuation.*` key.

The fourteen VEHS `mission.continuation.*` descriptors use the default version range plus `.WithCapability(SaveCapability::SpaceMiningContinuation)`. The per-row scratch is still reset before every native and copied row.

Two VEHS checks needed more than a renamed gate:

- The copied preview's field-presence check compared `_sl_version` with each field's raw version range. With capability-gated fields that test would have demanded the keys in every file, so it now calls `SlIsObjectValidInSavegame`, which covers both the range and the capability.
- `CheckSpaceMiningContinuationHeader`'s "missing Space schema" refusal uses the capability.
- VEHS Save refuses non-default continuation intent when the writer lacks the capability (merged 441), rather than dropping it.

**18.2** SPOR's Read and Save both take `policies` from `SlHasSaveCapability(SpaceMiningContinuation)`:

- the writer emits 14-byte rows at 441 and 15-byte rows at 446;
- a 441 writer refuses any order whose policy is not `LegacyReturn`;
- the reader keeps `LegacyReturn` as the default for 14-byte rows.

This fixed the interim defect Group 17 exposed: a resaved Space world failed to reload with "Invalid Space order count".

**18.3** The two departure gates are separate capabilities:

- `AcceptLoadedSpaceOrders` refuses unqueued continuation unless the source has `SpaceLegacyDeparture`.
- The copied preview runs `ValidateSpaceMiningContinuationPreview` only for sources with `SpaceMiningContinuation`, passing `SpaceLegacyDeparture` as its flag.
- Ownerless released survey Return/Waiting follows the ASTR era (Group 17): only SurveyDeparture admits it, and the 441 writer's lossless proof refuses it.

**18.4** Tests:

- `space_mining_wire.cpp`'s schema case uses `true`/`false`, plus a skipped foreign key in an older source. It is the pending-ledger case and now passes.
- S's existing wire cases already cover unknown phases, contradictory presence flags, stale generations and leases, and wrong roles; they run in the subset below.
- S's native harness (`asteroid_cargo_native.cpp`) previously read the last file's version. `SaveFileCarries` now classifies the copied file through the public boundary (`InspectSaveWireFacts`, `ClassifySaveDialect`, `HasSaveCapability`).
- All five burn-down rows are ticked. `git grep` finds no pending enumerator outside `saveload.h`.

Results (`logs/merge-oct28/group18/`):

- Subset `-R "[Ss]ave|[Ll]oad|[Pp]review|[Ll]ineage|[Oo]riginal|[Cc]opied|[Rr]eload|[Ss]pace|[Aa]steroid|[Mm]ining|[Oo]rder|[Mm]ission|[Dd]eparture|[Ss]urvey|VEHS|SPOR|[Cc]ontinuation"`: GCC 540/540, Clang 540/540.
- `space-factory-440.sav` resaves and reloads (exit 0 both).
- `space-heavy-440.sav` still refuses on save with "Invalid native cargo source for saving". That is the pending typed-origin item for Group 22, and the engine leaves a 0-byte file, as it already did before this group.

## Group 19

**19.1** Checked by reading:

- `NativeCargoPacketPreview` (`fios.h`) carries the packet, quantity, source and feeder share; S's `source_tile`, `first_station` and `legacy_asteroid_origin`; and D's U64 `company_supply_order` and literal `company_supply_shipped` byte.
- `NativeCargoOwnerPreview` is the only owner row. Group 4 removed the reference-only `NativeCargoReferencePreview`, and no consumer expects it.

**19.2** Readers:

- The station reader (`station_sl.cpp:776-780`) writes one owner row for every bucket reference.
- The vehicle reader (`vehicle_sl.cpp:1418-1425`) writes one owner row for every ordinary-carrier reference.
- So aggregate references and owner rows describe the same packets one-to-one in every real file. Group 20 relies on that.

These `LoadCheckData` fields already exist with S's value types: `asteroid_body_rows`, `asteroid_next_body_id`, `asteroid_mining_leases`, `next_launch_center_generation`, and the per-row `mining_continuation`. No additions were needed.

**19.3** `Clear()` audit: a scan of `LoadCheckData`'s 167 members against `LoadCheckData::Clear()` found every merged field reset.

- `companies` is cleared without the `this->` prefix.
- `grf_compatibility` is not reset in D or S either; it is recomputed on every load, as upstream does.

Tests:

- `CheckDiscardedLoadCandidate` (`save_candidate_lifecycle_fixture.h`) now also asserts that owner rows, Space vehicle rows, the asteroid field, body rows, the frontier, leases, the launch-center issuer and order rows are cleared after a refusal.
- `Candidate previews discard failed transport across original lineages` appends a refused development-build original (with its `DEVELOPMENT_SAVEGAME` error kept and the world unchanged), then a valid A440 preview.
- New case `Candidate preview reset clears every merged copied field` sets every merged field and checks `Clear()` restores each default.

Results (`logs/merge-oct28/group19/`):

- `[save-candidate-lifecycle]`: 2 cases, 10,764 assertions.
- `[save-lineage-preflight]`: 4 cases, 3,048 assertions.
- Subset `-R "[Ss]ave|[Ll]oad|[Pp]review|[Ll]ineage|[Oo]riginal|[Cc]opied|[Rr]eload|[Cc]andidate|[Dd]iscard|[Rr]efus"`: GCC 481/481, Clang 481/481.

## Group 20

**20.1-20.2** `ValidateSpaceCargoPreviewOwners` (`space_cargo_preview.cpp`) is now the one copied owner join. S's separate walk `ValidateSpaceAsteroidCargoPreview` is gone from the header and the source, together with its private owner-meaning map and the `GetOptionalSpaceSupplyLabel` adapter.

- `natural` (any `SourceType::Asteroid` row) or a paid book (`supply != nullptr`) switches on `detailed` mode. Natural packets alone now request the per-container meanings, so a candidate without a paid book is judged in every container.
- In natural mode, the CDIM rows and the sparse identity records (`ValidateAsteroidIdentityRecords`) are validated once before any lookup.
- The paid-supply labels (CDIM, or the explicitly admitted legacy label table) are prepared only when there is a book; the origin policy reads its own optional CDIM meaning, so genuine old no-CDIM files keep S's full-load-only ordinal rule.
- One `admit(packet, supply_meaning, origin_meaning, station)` applies the supply policy, then the origin policy. Ownership is counted separately by `add`/`count`, once per packet. Ordinary station and carrier references are admitted only through their owner rows, which must match the aggregate references one to one (Group 19 made the readers guarantee that). Space hulls, handling, manifests, structure inputs, module stock, service berths, manufacture inputs and descents admit each reference once.
- New refusals in detailed mode: an owner row whose slot is outside the cargo table, a structure input slot beyond its recipe that is not empty (S indexed past the recipe here), and a structure kind with no real recipe. Manufacture keeps D's spare-slot rule.
- The orphan loop for natural packets is unchanged.

**20.3** Order, checked in `saveload.cpp`'s load-check block: `ValidateSpaceOrderLoadPreview` (mission, queue, lease, frontier, mineral, Space bounds and generation cursors through `ValidateSpaceMiningContinuationPreview`, under `SpaceMiningContinuation`) → knowledge → dimensions → `ValidateSpaceCargoPreviewOwners` → `NormalizeLegacyAsteroidCargoPreview`. The join runs before normalization, and normalization still resolves every row before changing any (its `upgrades` vector). It reads the unified `NativeCargoPacketPreview` rows (`legacy_asteroid_origin`), so no adaptation was needed.

**20.4** Tests:

- `asteroid_cargo_validation.cpp`, `One copied owner join judges natural origins in every container without a paid book` (26 assertions): out-of-table owner slot, one owner row describing a packet twice, owner rows that do not match the references, processor stock under a slot with a different meaning, natural material in a construction input or a service berth, a manufacture spare slot and a Steel recipe slot, and an unknown structure kind. Every case is refused and the native packet pool is untouched.
- Same file, `Paid-supply custody and natural origin share one admission per packet` (6 assertions): with a real `SurfaceSupplyCustodyCheck`, a natural hull packet, ordinary freight and natural processor stock pass, and the final allocated pass leaves the check `Valid()`. The check refuses a repeated owner, so this proves each packet was admitted once. With a contradicting CDIM meaning, the supply policy alone would accept the unpaid natural packet; the origin policy at the same point refuses.
- `save_lineage_preflight.cpp`, `Owner-join previews alternate lineages and refuse malformed custody without touching the world` (149,381 assertions). The running world is a `TubePrototypeRig` with a capsule. The test saves that world with the current writer (merged 441, MLLN) into `test-cache/merge-oct28/group20/running-world-merged.sav`, then previews: A433 (natural freight, paid supply, eight stations) → the development-build typed-origin 445 file (`DEVELOPMENT_SAVEGAME` kept, data cleared) → the merged file, and then one of two malformed A433 copies:
  - Source: one natural packet's `source_xy` moved to a Surface tile.
  - Duplicate owner: each station row copied as the next station, and each train row copied under sparse index 127. Copies of owners that hold packets give those packets a second owner. A433's trains hold none, and its loaded stations are refused.
  Every refusal is checked to come from the owner join ("ownership" or "custody" in the kept reason) and to clear the candidate (`CheckDiscardedLoadCandidate`). The A433 and merged previews then pass again with A433's original packet and owner-row counts. After every step the world's accounts, packets, RNG, domains, tick, camera, profile and geometry are unchanged (`PreviewWorldFacts`).
- The headless test world has no cursor sprite, and the writer toggles the busy cursor, so the test gives `_cursor` one neutral entry around the save and restores it. `LayerSaveContent` names the real package in the saved file, so the merged preview finds it.
- Plan change: a merged file with natural packets needs writer 446, and copied leases are joined only under `SpaceMiningContinuation` (446). These cases move to 22.4.

Results (`logs/merge-oct28/group20/`): GCC and Clang builds without warnings. The focused set is 59 cases, one process each through `test-cache/merge-oct28/run_cases.sh`, which runs from the build directory as CTest does. It covers `[asteroid-cargo-*]`, `[heavy-joined-validation]`, the supply custody tags, `[save-candidate-lifecycle]`, `[save-lineage-preflight]`, `[save-space-lineage-*]`, `[space-mining-wire]` and `[save-supply-wire-*]`. GCC passes 59/59 with 380,638 assertions, and Clang 59/59. CTest subset `-R "[Ss]ave|[Ll]oad|[Pp]review|[Ll]ineage|[Oo]riginal|[Cc]opied|[Rr]eload|[Cc]andidate|[Dd]iscard|[Rr]efus|[Oo]wner|[Cc]ustody|[Aa]steroid"`: GCC 591/591 and Clang 591/591.


## Group 21

**21.1** The acceptance block in `AfterLoadGame` (`afterload.cpp`), from `AfterLoadCompanyStats` to `AfterLoadStoryBook`, was compared call by call with D (`origin/mlm-mlv`) and S (`origin/mlm-mlv-2sp`); the call lists are in `test-cache/merge-oct28/afterload-*.txt`. HEAD is D's list with exactly one insertion: `NormalizeLegacyAsteroidCargoPackets`, under `!SlHasSaveCapability(AsteroidCargoOrigins)`, right after `AcceptLoadedAsteroidField` and before launch centers, hubs, handling, orders, knowledge, pods, manifests, transitions, production, dimensions, descents and structures. D's TBM, exchange, spoil, mine, processor, utility, recipe, supply, service, approach, fleet, Tube and geothermal order is unchanged. S's own list differs only by the B-side machinery D brought.

**21.2** Audit by the named calls:

- `AcceptLoadedSpaceCargoDimensions` runs after NewGRF reconstruction, so real cargo definitions exist.
- `ValidateSpaceVehicles` (queues, leases, physical custody through `VisitNativeCargoPackets`, production links) is the last Space check.
- Nothing in the block issues a command, a payment, a target selection or a tick. The 21.4 recovery checks confirm this: after each reload the tick, accounts, packets, RNG and domain digests equal the first load's.

**21.3** Gap found and fixed. A failed full load runs every chunk handler's `FixPointers` in the null pass (`SlError` → `SlNullPointers`). D's B-side owners (supply, utility, Tube power and vehicles, geothermal generators, risers and Tube sources) reset there. The Space handlers ASTR, SPKN and SPOR returned early. So a late refusal left the installed asteroid field, the staged knowledge, the Space order candidate and the generation issuer in place until the next `InitializeGame`. They now reset in the same pass: ASTR calls `ResetAsteroidField()` (field, index and both stagings), SPKN `ResetAsteroidLoadStaging()`, and SPOR `ResetSpaceOrders()` (issuer and candidate). Initialization (`ResetMultiLayerStateSchema` in `InitializeGame`) and success (each `AcceptLoaded*` clears its staging) already cleared them. `ScopedLoadLineage` is unchanged, and `SlNullPointers` already drops the lineage. Commit `3175609b`.

**21.4** `space_lineage_native.cpp` gained `CheckSpaceLateFailures`, run by `python3 -B src/tests/space_lineage_native_checks.py --configuration RelWithDebInfo --epoch heavy-440 --late-failures`. The harness now has the `heavy-440` epoch (`src/tests/fixtures/demos/space/space-heavy-440.sav`, SHA256 checked against `current-checkpoints.json`; A440 needs no digest hint, which the malformed copies could not carry) and falls back to `build/openttd_tube_test` on single-configuration trees. With the A440 world loaded, three malformed copies are each previewed, fully loaded, and then followed by a full reload of the source:

| Failure | Malformation | Copied preview | Full load |
| --- | --- | --- | --- |
| After field install | One natural packet's CAPA `source_id` set to the field's `next_body_id` (never issued) | Error | ReInit, "Invalid legacy asteroid cargo origin" |
| After packet upgrade | The VEHS row of an idle, unleased, empty hull removed | Error | ReInit, "Invalid native Space order ownership" |
| After queue install | ASTR re-encoded through its own codec era with a free body leased to a hull that is not mining | accepted (see below) | ReInit, "Invalid native Space fleet" |

After each ReInit, the test checks that the asteroid field is empty, the order issuer is 1 and there is no load lineage. The source then reloads with the same field, fleet, orders and structure digests, packets, accounts, domains, tick and RNG. Result: 1 case, 43,379 assertions, passed (`logs/merge-oct28/group21/late-failures-gcc-5.log`).

The fix was then reverted locally and the run repeated (`late-failures-gcc-without-reset.log`). All three cases failed `GetAsteroidField() == AsteroidField{}`, and the third also left the issuer at 9. The fix was restored. The existing A437 mode still passes (84 assertions).

The A440 demo holds no lease, so the lease case adds a foreign one. A genuine A440 copied preview has no lease join (only 446 joins leases, under `SpaceMiningContinuation`), so that preview accepts the copy. The full loader's final join refuses it. 22.4 covers 446 lease previews.

The incoming branch's malformed-file harnesses (`asteroid_cargo_wire_checks.py`, `asteroid_field_wire_checks.py`, `legacy_departure_wire_checks.py`) still assert source version 442 or later. That is recorded as a plan change in Part 2 Group 33.1.

Results (`logs/merge-oct28/group21/`):

- `openttd_test` builds without warnings in GCC and Clang. `openttd_tube_test` builds in both; its warnings are all in existing tube and TBM fixture headers, none in the files changed here.
- Focused GCC cases: 32/32.
- CTest save/preview subset (the Group 20 regex): GCC 591/591, Clang 591/591.
- Late failures: GCC and Clang (`--build build-clang`) both pass 1 case with 43,379 assertions.

## Group 22

**Entry gate.** `git grep` finds no use of the five pending `SaveLoadVersion` names outside their own definitions (the same names in `SaveCapability` are the capability enum and stay). The burn-down holds only the `saveload.h` and `saveload.cpp` rows, which this group closes.

**22.1** Writer activated:

- `saveload.h` appends `SpaceMiningLifecycle = CURRENT_MERGED_SAVE_VERSION` (446) after `MergedLineage`, so `SAVEGAME_VERSION` is 446. The five pending enumerators and their interim comment are deleted, and 442-445 are never allocated. The plan's working name `CombinedSpaceFixes` was not used, because Group 22B removes branch-style names; the version is named by what it adds.
- `saveload.cpp` asserts `SAVEGAME_VERSION == SpaceMiningLifecycle` and that its value is `CURRENT_MERGED_SAVE_VERSION`.
- `SlHasSaveCapability` keeps its writer context `{Accepted, Merged, SAVEGAME_VERSION}`, which now grants every A, B and Space capability; only its comment changed.
- `GetMergedNetworkRevisionSuffix` uses `CURRENT_MERGED_SAVE_VERSION`. `merged_command_compatibility.cpp` expects `+m446c224p`.

**22.2** `save_lineage_sl.cpp`: MLLN `Save` writes `MLN1`, schema 2, writer 446. `Read` (Load and LoadCheck) accepts exactly (441, 1) or (446, 2), and the marker's writer must equal the admitted envelope version (`MergedLineageSchema`). There is no `>= 441` shortcut and no other schema. The classifier and the detached inspector already held the same two tuples (Groups 10-11).

**22.3** Family contract:

- The incoming branch's diff from the merge base adds no chunk handler, so 446 has exactly 441's 128 family names and framing kinds.
- New `src/tests/fixtures/saveload/merged-446-family-set.json` takes `merged-family-set.json` (441, unchanged) as its `base`. It declares only the 446 differences: the MLLN tuple, the two CAPA columns `asteroid_body` and `asteroid_mineral` (U32), and the fourteen VEHS `mission.continuation.*` columns with their file types. It names the ASTR and SPOR layout changes, which their own codec tests own.
- `savegame_tables.py`: `merged_contract(version)` selects the contract, `merged_world_chunks` checks the marker payload and every declared column (exactly once, any nesting depth, exact kind), and `complete_world` accepts both versions.
- Checked on a real 446 save: accepted, 128 families. Copies with a crossed marker (schema 1 at 446) and a renamed CAPA column are refused (`test-cache/merge-oct28/group22/reader-*.sav`).

**22.4** Writer tests (`save_lineage_preflight.cpp`):

- `Writer 446 emits its exact marker, Space columns and an identical second save` (6,785 assertions) covers current-to-current identity. Two uncompressed saves of the same paused rig world are byte-identical, and saving changes no live state. The envelope is 446, and the file's own facts are the exact tuple, typed-origin columns, canonical U64/U8 supply links and the tagged book. It classifies as merged 446 with the five Space capabilities and both supply capabilities. MLLN, ASTR, SPKN, SPOR, CAPA, VEHS, SSOR and CDIM are each present once, even though the world is empty. The MLLN payload is exactly `MLN1 00 02 01 BE`, the two CAPA columns are adjacent, and the file previews with the world unchanged. `SaveRunningWorld` is shared with the Group 20 sequence.
- `Merged 446 natural previews refuse malformed leases without touching the world` (28,815 assertions) is the 20.4 extension. The fixture `src/tests/fixtures/saveload/merged-446/mining-working-446.sav` (with `catalogue.json`: producer command, input and executable identities) is a real native load-and-save of the retained shared-era working-miner demo by `space_fixes_checks.py --cargo-origin`: 3 packets, 2 natural, preview, unchanged world and resave all true. In the test it previews between A433 and the development-build file, every natural packet carries its typed origin with no legacy receipt, and its lease rows are stable. ASTR is decoded through the survey-departure era, changed and re-encoded in two copies: a mining hull without its lease, and two bodies leased to one hull. Both refuse with the copied lease join's reason, the candidate is cleared, and the world is unchanged.
- Original-to-current conversion is qualified separately by the native harnesses (22.5).

**22.5** Milestone suites (`logs/merge-oct28/group22/`):

- Full CTest, GCC: 1525/1525.
- The ten focused executables (`test-cache/linux-baseline/run_focused.sh build`): all build and return 0. The JUnit `tests` attributes count assertions: surface supply 247,222; surface capital 550; processor contract 2,457; ground art 0; ground consumer 208,547; ground viewport 0; tube 4,319,276; Space art 29,073; merge contract 951,965; Space fixes 534,864. All have zero failures, and Space fixes no longer has its baseline class (b) failure.
- Pending ledger: the last class (b) item is cleared. With writer 446, `space_ordinary_checks.py --source src/tests/fixtures/demos/space/space-heavy-440.sav --mode load` and `--mode advance` both save. The saves are 446 files of 16,796 and 16,852 bytes, all 128 families meet the 446 contract, and the advanced save reloads in the game (exit 0). Before, saving refused with "Invalid native cargo source for saving".
- Original-to-current conversion (`space_lineage_native_checks.py --roundtrip --canonical`): A437 → 446 and the shared-donor B426 → 446 both complete. Native Space digests and copied receipts are equal, and the second save matches the successor byte for byte over all 128 families. The report's `migration_receipt` labels the conversion (original version to 446) separately from the 446 → 446 identity. `merged_supply_receipts.merged_book` now accepts each merged writer's own marker tuple; it had asserted 441.
- Four more Python oracles still assume the current writer is 441 (listed in Part 2 Group 24.1); Part 2 owns them.
- Full CTest, Clang: 1525/1525. A first Clang run failed the four `regression_*` entries with "No available language packs": `build-clang/openttd` predated Group 17B's new language string. After rebuilding `openttd`, all pass. GCC was rebuilt the same way and rerun on the final tree: 1525/1525, no warnings in the rebuilt units.
- Two stale code comments that still said "until Group 22" (ASTR and VEHS save) now describe writer 446.

**22.6** Part 1 hand-off state:

- One writer (merged 446, MLLN schema 2, all A, B and Space capabilities).
- One loader that reads exactly: shared, A and B originals; merged 441 (schema 1, no typed origins); and merged 446 (schema 2, typed origins). Development-build 441-445 files without a marker are refused with their own message (17B).
- Every Space field is read and written by capability, never by version number. The burn-down table and the pending ledger are both empty.
- Copied previews use one owner join (20), late native failures revoke all Space staging (21), and the 446 family contract is checked by its own reader (22.3).
- Still open for Part 2: the Python oracles listed in Part 2 Group 24.1, the incoming branch's malformed-file harnesses (Part 2 Group 33.1), and Group 22B's vocabulary rename, which comes first.
- Nothing is pushed.
