# R2FC-2 - Disk cleanup and independence from disposable logs

Recorded: 2026-10-15. Predecessor: [R2FC file cleanup](r2fc-file-cleanup.md). Shared layout owner: [R2MLVS development artifact layout](r2mlvs-multilayer-vehicles-and-structures.md#development-artifact-layout).

Successor execution: [R2FC-3](r2fc-file-cleanup3.md) supersedes this plan's
unimplemented permanent-output retention/migration approach. Groups1-9 there
have captured/classified inputs and registered disabled recipe declarations;
sealing, relocation, fresh verification and cleanup remain pending. The unchecked
tasks below remain unchecked; this link claims no implementation of them.

**Separate authorized archive cleanup,2026-10-17.** The user requested and authorized a separate Git commit moving the October execution ledgers, handovers, audits and generation records to `docs/zz-rhr/upgrades/2026-10/`, preserving their relative subfolders.168 auxiliary Markdown files moved;45 main plans, review plans and player guides remain at the original location. Inbound links, archived outbound links and repository-qualified source/tool references follow the new paths. Repository VS Code settings hide the archive from Explorer, search and file watching; Git continues to include it. This relocation supersedes the earlier same-directory companion placement rule. It does not execute the pending artifact/log migrations below or resume deferred native compilation.

**Status:** the disk audit, cleanup script, baseline retention and post-cleanup checks below are completed work. The migration tasks in Groups 2-9 are **planned, not implemented**. In particular, `logs/` is still required by some checks and must not yet be emptied wholesale. This documentation request does not apply another cleanup, move test inputs or authorize Git writes.

## Scope and execution rules

This follow-up records the 2026-10-15 cleanup conversation and defines the next work needed to make diagnostic logs disposable. It does not reopen completed gameplay plans, alter save formats, weaken provenance checks or mark unfinished artwork/Space work complete.

- Preserve unrelated working-tree changes. Git inspection is read-only; no staging, commits, resets, checkouts or other Git writes under the current instruction.
- Keep writes and deletions inside the workspace's Git repository, except locations explicitly authorized by the user. The external baseline was inspected read-only; only the requested copies were written inside this repository.
- Before implementing a group, read its named producers/consumers below and the live shared layout contract. Follow references only as needed for that group; historical line numbers and run IDs are navigation aids.
- Mark a task `[X]` only when its stated behavior has been implemented and checked. Historical passes and a path rename cannot substitute for current validation.
- Record relocated byte identities and distinguish historical evidence from a new execution. Do not change recorded hashes just to silence a mismatch.
- Keep executable helpers in their existing source/tool owners. New source, test and JSON filenames must not inherit planning prefixes such as `r2`; this file is Markdown.

## Completed work and findings

### Earlier commit request

At the user's earlier explicit request, the workspace inventory found one Git repository and committed its 173 pending files as `9a47768ae3e4455661264f2c49febde972538e49` (`Implement orbital freight lifecycle and asteroid generation`). The commit message named all five plans with newly completed `[X]` tasks:

- `r2mlsp-space-master-plan.md`
- `r2mlvs10-asteroids-and-mining-probes.md`
- `r2mlvs11-orbital-stations-and-manufacturing.md`
- `r2mlvs8-space-world-and-construction.md`
- `r2mlvs9-launch-centers-and-rockets.md`

That authorization was specific to the earlier commit. The later cleanup changes remain working-tree edits; this follow-up performs no Git writes. The commit-only request did not rerun builds or tests.

### Native runtime accumulation and the original 21.55 GiB estimate

The initial `mlm/runtime/native/` audit found 598 runs, 602 process homes and 60,660 files, totaling about 23.5402 GiB of file lengths. Approximately 9.7778 GiB was executable paths, 7.9978 GiB language packs and 5.7627 GiB base assets; the rest was small.

The cause is per-process materialization. [RunContext.prepare_runtime](../../../../src/tests/multilayer/support/artifact_paths.py) copies `baseset`, `newgrf`, `lang` and `scripts` from the build into new process homes. The language directory contained all 66 packs. [console_executable](../../../../src/tests/multilayer/support/runner.py) also produces a Windows console executable beside each run's dependencies. Isolated writable configs/startup files are useful; retaining hundreds of complete distributions after execution is unnecessary. Neither helper currently prevents that accumulation on future runs.

The first script retained historical executable paths, removed copied assets and hard-linked identical executable copies. Its 21.55 GiB figure combined approximately 13.762 GiB of removable asset payload with 7.790 GiB of estimated executable deduplication. The user observed only about 14 GiB of visible reduction. The original reporting mixed file lengths and deduplication estimates and should not have presented that total as measured physical space freed.

A subsequent inspection found 368 executable paths sharing 82 Windows file identities: about 9.778 GiB of summed lengths, but only 1.987 GiB of distinct executable data. Tools that add every path's length count shared hard links repeatedly. This proves the sharing at inspection time; without before/after allocation measurements it does not establish the exact physical recovery from the first run. The screenshot alone does not resolve which size metric was displayed. The current script reports distinct data separately and observes drive free-space change on apply.

The revised decision is to remove the old native runtime tree entirely. Historical report paths/hashes describe completed executions; they do not require keeping old executable copies installed. Fresh tests create new homes from the retained build and inputs.

### Minimal local baseline archive

The external `D:/Codez/Sandbox/OpenTTD-baseline/` was a source export of commit `886ef09a091993e80ce8db1b46948b01f8d8d59d`, not a Git repository/worktree. Its 1,972 source files matched that commit, allowing Windows line-ending conversion. It occupied approximately 4.0765 GiB, of which 4.0232 GiB was build output.

At the user's request, the necessary project material was copied into the user-created local [OpenTTD-baseline](../../../../OpenTTD-baseline/) directory:

- Eight original/reloaded version-368 baseline saves and `build/baseline.cfg`.
- Seven original configure/build/unit/regression proof logs and eight small dedicated generation/reload logs.
- Twenty-four retained payload files totaling 2,866,471 bytes, plus [retained-files.json](../../../../OpenTTD-baseline/retained-files.json) with source commit, paths, sizes, SHA256 identities and regeneration references.

All copied hashes were checked, as were all 80 historical map-plane digest records. Four stock regression saves and their config were already identical to repository regression inputs. Exported source, compiled binaries, package installations and caches were unnecessary to retain. The archive is protected by the root-anchored `/OpenTTD-baseline/` ignore rule and is deliberately independent of `clean.ps1`.

The retained location is documented in [R2FC-F](../../../zz-rhr/upgrades/2026-10/r2fc-f-retained-records.md) and [R2MLM](r2mlm-multi-layered-map.md). At the post-cleanup check the external sibling directory was absent; the agent did not delete it. Preserve the local archive. Because it is ignored, it is local retained data rather than something automatically supplied by a fresh checkout.

### Expanded cleanup and reasons the large directories were disposable

[clean.ps1](../../../../clean.ps1) now targets a precise allowlist. The second audit/dry run estimated **14.51 GiB of additional distinct file data**, before the user's subsequent cleanup:

| Category | Summed file lengths, GiB | Estimated distinct data reclaimable, GiB | Reason |
| --- | ---: | ---: | --- |
| Old `mlm/runtime/native/` homes | 9.778 | 1.988 | Historical test executables, configs and generated process data; hard links account for the difference. |
| `build-probe/` and `build-probe-tree/` | 3.752 | 3.752 | Retired experimental build trees. |
| Main-build intermediate directories | 5.605 | 5.605 | Compiler objects, intermediate libraries and build state. |
| Main-build generated symbol/linker files | 2.467 | 2.467 | PDB/ILK/OBJ/PCH/LIB/EXP files directly in build configurations. |
| Raw PPM review captures with PNG counterparts | 0.698 | 0.698 | Redundant raw containers; 1,255 paired PPM paths selected. |
| Acceptance outputs, invocation markers and routing scratch | Less than 0.001 | Less than 0.001 | Small reproducible run output. |

These are historical estimates, not a measurement of today's remaining clutter or an exact drive-free-space claim. Do not add them to the earlier 21.55 GiB as verified physical recovery. The full second preview is currently in `logs/cleanup-dry-run.txt`; that diagnostic report is itself disposable.

- `build-probe-tree/` was a scratch source copy and build used on 2026-10-03 to discover the nine Group-3 compilation fixes and obtain 103 passing Debug unit tests. The fixes were subsequently delivered to the real source. [R2RVM](map-layers/r2rvm-review-map.md) and [R2RVM2](map-layers/r2rvm2-review-map-2.md) already describe the copy as scratch that may be gone. It is not a required source authority or separate repository. Both probe trees were confirmed absent after the user reported cleanup.
- The main `build/` audit totaled approximately 9.440 GiB. Multiple build configurations, object files, intermediate libraries and debugging symbols explained most of the size. About 8.072 GiB could be removed while retaining the playable build. Deleting the entire directory would also lose its runnable game, dependencies and any local saves/configs; the script therefore removes selected generated bulk.
- `mlm/reviews/acceptance/data-home/` and every `demo-guide-load-N/`, including 10 through 17, contain reproducible test homes/reports/captures and demo resaves. The original reusable demos remain under `src/tests/fixtures/demos/`. The preview selected 29 acceptance directories in total.
- Other review runs remain: some contain explicitly selected saves/evidence for unfinished Space work and native-art validators. Do not erase all reviews before their needed inputs are curated or reproducibly regenerated.

### Current script behavior and safeguards

The script is well commented, requires PowerShell 5.1 and previews by default. Review/run commands from the repository root:

```powershell
.\clean.ps1
.\clean.ps1 -DryRun
.\clean.ps1 -Apply
.\clean.ps1 -Apply -KeepBuildIntermediates
.\clean.ps1 -Apply -ShowPaths
```

`-DryRun` and `-WhatIf` override `-Apply`. Dry runs list every selected directory/file and its estimated reclaimed data, as well as summed lengths. `-KeepBuildIntermediates` retains the main build's intermediate directories and generated symbols. Stop games, runners and builds before applying; deletion is permanent.

The exact targets are the old native runtimes, runtime `.runs` markers and routing-check scratch, both probe trees, main `openttd.dir`/`openttd_lib.dir`/`openttd_test.dir`/`x64` directories, selected symbol/linker extensions directly in Debug/RelWithDebInfo/Release/MinSizeRel, the named acceptance directories and PPM files with same-basename PNG counterparts. It does not blanket-delete `build*`, all reviews, saves or logs.

Containment checks verify repository ownership and exact allowed paths. The script refuses tracked candidates, nested Git repositories and reparse points/junctions, rechecks before removal and blocks apply while relevant game/build processes are active. Windows file identities/link counts prevent counting a shared allocation twice or claiming it freed while an unselected hard link remains. Apply reports completed deletion lengths, estimated distinct data freed and observed drive-free-space change; unrelated drive activity and allocation/compression can affect the latter.

Preserved data includes source, Git metadata, fixtures, authored media, `!kit/`, the local baseline archive, main game/test executables and DLLs, language/base assets, dependencies, CMake configuration/project files, local saves/configs, demo runtimes, logs, PNGs and other needed review runs. The next compile recreates removed intermediates/debug symbols and takes longer. Historical output paths may disappear; rerunning their producer supplies fresh evidence rather than reviving the original historical pass.

Supporting code changes are the atomic temporary-file/replace write in `runner.console_executable`, its hard-link safety regression in [artifact_paths_check.py](../../../../src/tests/multilayer/support/artifact_paths_check.py), [cleanup_check.ps1](../../../../src/tests/multilayer/support/cleanup_check.ps1), the baseline ignore/docs changes and [review routing notes](../../../../mlm/reviews/README.md). Atomic replacement prevents regeneration of one console copy from overwriting another historical hard link. No live `-Apply` was run by the agent; removal tests used disposable repository-local fixtures.

### Verification and remaining failures

| Check | Observed result |
| --- | --- |
| Cleanup behavior fixture suite | 86 checks passed: selection, preview, preservation, hard-link accounting, repeatability, containment, tracked-file/nested-repository/junction refusal. |
| Artifact-path Python suite | Eight tests passed, including after the user's cleanup. |
| Native artifact ownership cases | Two cases/12 assertions passed in Debug and again in RelWithDebInfo after cleanup. |
| Native Space type/reflection and asteroid population cases | Two cases/312 assertions passed in Debug after cleanup. |
| Local baseline | All 24 copied payload identities verified; 80 historical plane-digest records matched. |
| Formatting | `git diff --check` passed; ordinary LF/CRLF notices were observed. |
| Full project suite | Not run for this cleanup; no claim that every test currently passes. |

Two focused graphics checks failed before reaching their assertions: `native_mine_contract_checks.py` reports a changed owner `src/industry_cmd.cpp`; `native_processor_contract_checks.py` reports `src/multilayer_recipe.cpp`. Their recorded source hashes already disagree with the source in committed `HEAD`, so deletion did not cause these failures. They need an owner-led proof refresh, not arbitrary hash edits. The source and retained test executables remain available; ordinary fresh runs recreate runtime homes.

## Why logs cannot be emptied yet

The first cleanup's separation of runtime/review/log paths did not make all logs disposable. The earlier policy explicitly sent reproducible proof products to `logs/proofs/`; some later consumers also put permanent comparison inputs and progress indexes in `logs/`. The following concrete dependencies were found:

| Current dependency | Consumers / importance | Target owner |
| --- | --- | --- |
| `logs/graphics/.../before.json`, rail-station `schema-reconciliation.json` | Station/terminal, mine, processor and coal-mine source checks read historical comparison inputs. | Curated immutable fixtures under `src/tests/fixtures/graphics/<purpose>/`. |
| `logs/proofs/mlm/native-art/` manifests, atlases, reports and expected `m1-packed-sprites.bin` | `evidence.py`, converters, packers, packed-art fixture/load/build checks and collector payload checks read generated prerequisites. | Explicit generated run under `mlm/reviews/`, with a complete preparation command. |
| Required native transcripts and executable identity | Published graphics descriptors and mine/processor checks verify exact proof. One processor proof executable is currently inside `logs/graphics/processor-source/.../bin/Debug/`. | Retained evidence beside its run/fixture; current binaries built through declared build targets, not kept in logs. |
| `logs/*-progress.json`, verification receipts and indexes | Space phase-pair checks, continuation commands and chemical-state conversion consume these records. | Run-owned reports/indexes under `mlm/reviews/`; promote only independently necessary immutable inputs to fixtures. |
| Prior-run `.log` files | State/geology/profile partial reruns and TBM feedback/motion/pacing compare earlier accepted results. | Explicit prior-run semantic evidence or required transcript under `mlm/reviews/.../evidence/`. |

Diagnostic text can itself be essential evidence when a validator reads or hashes it. Merely changing its extension, ignoring it in Git or moving the top-level manifest does not remove that dependency. Relocate or regenerate its complete dependency set, including nested references, saved states, relevant transcripts and indexes.

## Target storage contract for implementation

Use the established owners rather than introduce another generic top-level artifact tree:

| Role | Destination | Lifecycle |
| --- | --- | --- |
| Permanent regression inputs, expected baselines and meaningful compatibility/provenance inputs | `src/tests/fixtures/<feature-or-format>/<purpose>/`; reuse existing `graphics/`, `state/`, etc. | Curated, immutable and reviewable; ordinary tests never overwrite them. Retain only inputs adding coverage. |
| Authored artwork, palettes and public asset contracts | Existing `media/`, including `media/multilayer/source/` and `contracts/` | Existing source authority; test migration creates no second asset catalogue. |
| Generated machine-consumed test data, conversions and expected binary payloads | `mlm/reviews/<feature>/<scenario>/<run-id>/data/` | Ignored, reproducible from declared source/build inputs; explicitly prepared/selected for a test invocation. |
| Run reports, progress indexes, captures and saved checkpoints | Existing run `reports/`, `images/`, `saves/` | Explicit run/configuration and hashes; never select the newest directory implicitly. |
| Retained semantic evidence / required raw transcripts | Run `evidence/`, or curated fixture provenance when truly immutable | Stored outside diagnostic logs and identified by the consuming manifest. Preserve only what validates a real obligation. |
| Writable process homes, startup config and autosaves | Existing `mlm/runtime/<profile>/<run-id>/<process>/` | Disposable after the process exits; reconstruct from declared inputs. |
| Build products and tools | Existing build/dependency owners | Rebuildable through documented prerequisites; a test must not rely on an undeclared executable under logs. |
| Optional stdout/stderr, build diagnostics and cleanup previews | `logs/<feature>/<scenario>/<run-id>/` | Safe to empty between runs, or between stages after processes have closed the files. No required test input/evidence/index lives here. |

`data/` and `evidence/` are planned additions to the shared run API, not directories already supported by every helper. For example, a permanent processor comparison belongs under `src/tests/fixtures/graphics/processors/`; a freshly generated packed-sprite input belongs under a selected `mlm/reviews/multilayer-art/packed-load/<run-id>/data/`.

Do not move the entire current `logs/` tree into fixtures/reviews. Classify and retain the minimum necessary set, regenerate reproducible bulk, and leave redundant diagnostics disposable. If a generated prerequisite is absent, a documented preparation phase must create it or fail with an exact command. Missing/changed evidence must still fail honestly. No silent historical fallback, automatic approval of old proof or duplication of every run into a permanent fixture archive.

This target supersedes the earlier `logs/proofs/` placement **when the migration is implemented and verified**. Until then, current consumers still need the old locations. A new run must not read previous diagnostics from `logs/`; live stdout can be parsed directly, with any transcript required for later validation published under `evidence/` and an optional diagnostic copy under `logs/`.

## Tasks

### Group 1 - Record completed cleanup

- [X] 1.1 Record the earlier authorized commit and the later read-only Git restriction.
- [X] 1.2 Record runtime duplication, both cleanup estimates, hard-link accounting limits and the user's observed discrepancy.
- [X] 1.3 Record minimal local baseline retention, its verified identities and independence from cleanup.
- [X] 1.4 Record current script targets, exclusions, safety checks, commands and rebuild consequences.
- [X] 1.5 Record actual verification and distinguish existing stale-proof failures from deletion effects.
- [X] 1.6 Identify current log dependencies, recommend owners and create unchecked migration tasks. This completes planning only.

### Group 2 - Inventory every important input under logs

Read: the dependency table above; [evidence.py](../../../../media/multilayer/evidence.py); [multilayer_state_fixture.py](../../../../src/tests/multilayer_state_fixture.py); [phase_pair_index_checks-1.py](../../../../src/tests/multilayer/acceptance/phase_pair_index_checks-1.py); consumers named in Groups 4-6.

- [ ] 2.1 Search Python, C++, PowerShell, build tooling, JSON manifests and active plan/guide commands for direct and dynamically assembled reads/hashes of `logs/`. Trace imported helpers and subprocess producers; separate current-run diagnostic writes from future-stage prerequisites.
- [ ] 2.2 Produce a dependency inventory outside `logs/`, recording producer, consumer, role, configuration, source identity, nested dependencies, destination, retention reason and regeneration command. Include binaries, expected payloads, verification receipts and prior-run logs, not just JSON.
- [ ] 2.3 Classify each required item as curated immutable input, reproducible generated data, retained evidence/checkpoint or redundant diagnostic. Identify exact continuation inputs for unfinished work before any pruning. Unknown roles remain protected until resolved.
- [ ] 2.4 Record the existing mine/processor source-hash failures separately. Establish which migrated checks pass before edits so relocation is not credited with fixing unrelated stale proofs.

### Group 3 - Publish ownership and extend shared routing

Read: [shared layout](r2mlvs-multilayer-vehicles-and-structures.md#development-artifact-layout); [RunContext](../../../../src/tests/multilayer/support/artifact_paths.py); [native artifact API](../../../../src/tests/test_artifact_paths.h) and [implementation](../../../../src/tests/test_artifact_paths.cpp); [producer catalogue](../../../../mlm/review-producers.json).

- [ ] 3.1 Update the parent layout, review README, ignore-policy comments and affected active handoffs to reserve `logs/` for disposable diagnostics. Identify older `logs/proofs/` instructions as superseded without rewriting historical run outcomes.
- [ ] 3.2 Add shared run-owned `data/` and `evidence/` locations to Python/native routing and relevant manifests/catalogues. Preserve containment, host/client isolation, exact run/configuration selection and existing source/media/fixture ownership.
- [ ] 3.3 Give consumers explicit artifact/manifest inputs and producers explicit output owners. Replace fixed proof-cache defaults and implicit history selection; a single invocation must pass the same selected context through imported helpers.
- [ ] 3.4 Add meaningful boundary checks for traversal, foreign runs, fixture overwrite and attempts to select required evidence from `logs/`. Diagnostic output remains writable there. Do not merely ban every occurrence of the word `logs`.

### Group 4 - Curate permanent comparison fixtures

Read: [station checks](../../../../src/tests/multilayer/conventional_station_contract_checks.py), [terminal checks](../../../../src/tests/multilayer/conventional_terminal_contract_checks.py), [mine checks](../../../../src/tests/multilayer/native_mine_contract_checks.py), [processor checks](../../../../src/tests/multilayer/native_processor_contract_checks.py), and coal-mine `mine_base_source_checks.py`, `mine_construction_source_checks.py`, `mine_state_source_checks.py` under `src/tests/multilayer/`.

- [ ] 4.1 Retain required `before.json`/schema-reconciliation comparison data as purpose-named immutable graphics fixtures with provenance. Check complete original identities before relocating; preserve the minimum information needed for the intended regression.
- [ ] 4.2 Repoint all comparison readers and nested references, including coal-mine base/construction/state checks. Ensure repeat runs do not rewrite these fixtures or create replacement baselines under logs.
- [ ] 4.3 Separate stable comparison expectations from proofs of the current native build. Refresh the two stale mine/processor descriptors only through their owning native producer and actual successful checks; never replace a hash without establishing the corresponding evidence.
- [ ] 4.4 Preserve existing curated compatibility/demo inputs and the independent local baseline archive. Inventory promotion here does not bypass [master task 12.6](r2mlvs3mp-master-plan.md#group-12---close-both-target-plans-with-evidence) or authorize indiscriminate fixture pruning.

### Group 5 - Move generated art prerequisites and proof chains

Read: [evidence.py](../../../../media/multilayer/evidence.py), [source regeneration](../../../../media/multilayer/regenerate_art_sources.py), [proof regeneration](../../../../media/multilayer/regenerate_art_proofs.py), [packed build checks](../../../../src/tests/multilayer_packed_build.py), [expected sprite producer](../../../../src/tests/multilayer_packed_art_fixture.py), [native packed loader](../../../../src/tests/multilayer_packed_load.cpp), [collector payload checks](../../../../src/tests/multilayer/collector_payload_checks.py).

- [ ] 5.1 Redirect `EvidenceSet` selection/publication and every converter, packer, guide/activation helper and payload check from `logs/proofs/mlm/native-art/` to an explicit review run. Include `convert_chemical_states.py` and guide builders that currently read receipts/atlases from logs.
- [ ] 5.2 Move packed manifests, expected sprite bytes, converted atlases, galleries and related machine-consumed products to the selected run's `data/`/`reports/`/`images/`. Update Python and C++ inputs together; the packed-loader fixture must no longer be hard-coded below logs.
- [ ] 5.3 Publish required native transcripts under `evidence/`, update complete path/hash reference chains and preserve actual pixel/source/save/native-writer identities. A relocation changes metadata, not the claim that a historical execution ran again.
- [ ] 5.4 Remove the dependency on the processor proof executable stored under `logs/.../bin/Debug/`. Use declared reproducible build targets for fresh proof, and deliberately retained semantic provenance for immutable historical input where justified. Do not preserve another large executable archive merely to keep an old path alive.
- [ ] 5.5 Provide a bounded preparation/regeneration entry point and documented prerequisite order. Starting without generated caches must either build the selected prerequisites and test them, or give a precise actionable error; no source-workstation logs may satisfy the gate.

### Group 6 - Move progress, continuation and acceptance evidence

Read: [state fixture prior_artifact](../../../../src/tests/multilayer_state_fixture.py), [profile fixture](../../../../src/tests/multilayer_profile_save_fixture.py), [geology fixture](../../../../src/tests/geology_save_fixture.py), [phase-pair index](../../../../src/tests/multilayer/acceptance/phase_pair_index_checks-1.py), TBM `native_feedback.py`/`native_motion.py`, and [Space runtime handoff](../../../zz-rhr/upgrades/2026-10/r2mlsp-q-runtime-handoff.md).

- [ ] 6.1 Move Space paid-probe/descent/module/collection progress indexes and verification receipts into purpose-named run reports. Repoint their producers, phase-pair comparisons, chemical-state inputs and active continuation commands together.
- [ ] 6.2 Replace required previous `.logs` reads in state/profile/geology and TBM feedback/motion/pacing with explicit prior-run reports/evidence. Preserve `--prior-run`, configuration identity, exact saved input and accepted-stage semantics; no latest-run search or fallback into old logs.
- [ ] 6.3 Retain any transcript genuinely needed for financial/replay/provenance assertions in `evidence/`; parse live output directly when persistence is unnecessary. Optional stdout/stderr copies may still go to logs.
- [ ] 6.4 Promote only independently necessary saved inputs to curated fixtures. Keep unfinished-work checkpoints with complete manifests until replacements are verified, and repair all relocated references before releasing old copies.

### Group 7 - Prevent runtime growth from recurring

Read: [Python runtime preparation](../../../../src/tests/multilayer/support/artifact_paths.py), [console executable preparation](../../../../src/tests/multilayer/support/runner.py), [native runtime preparation](../../../../src/tests/test_artifact_paths.cpp), [cleanup allowlist](../../../../clean.ps1).

- [ ] 7.1 Measure representative native and Python invocations after cleanup; identify which content/languages/DLLs/binaries each needs instead of copying complete distributions by default.
- [ ] 7.2 Implement a contained shared immutable distribution/cache or supported explicit search paths for reusable assets. Keep configs, startup scripts, saves and autosaves process-local. Do not introduce runtime junctions that conflict with cleanup's reparse-point refusal.
- [ ] 7.3 Avoid repeated console executable payloads through a verified shared preparation/cache strategy. Preserve atomic replacement and copy-on-write behavior if hard links are used; regenerating one build/run must not mutate another run or source executable.
- [ ] 7.4 Verify native/Python parity, host/client isolation, language/base-content discovery and repeated-run disk growth. Document cache cleanup and regeneration without making historical binaries permanent test prerequisites.

### Group 8 - Make clearing logs safe and prove it

Read: [cleanup script](../../../../clean.ps1), [cleanup behavior checks](../../../../src/tests/multilayer/support/cleanup_check.ps1), and the completed dependency inventory from Group 2.

- [ ] 8.1 In an isolated repository-shaped fixture or checkout-like area inside the repository, run affected checks with logs initially absent, with unrelated logs present, and after clearing only those fixture logs between producer/consumer stages. Preserve real workspace evidence during this experiment.
- [ ] 8.2 Exercise graphics comparisons, packed art/expected sprites, relevant continuation modes and Space indexes with selected inputs outside logs. Missing or modified required data must fail, while absent diagnostic copies must not.
- [ ] 8.3 Add a focused regression check that detects newly introduced required-input dependencies on logs, including dynamically referenced manifest/transcript paths. Keep it role-aware so normal diagnostic writers remain permitted.
- [ ] 8.4 Only after dependency migration passes, add an explicit reviewed logs-cleanup mode to `clean.ps1`, with per-directory size preview and the same tracked-file/containment/process protections. Keep important fixtures/evidence outside its deletion set; document that clearing logs is safe between stopped runs, not during active writers.

### Group 9 - Validate and close the migration

- [ ] 9.1 Run affected Python/native checks and relevant build/asset preparation in the required configurations. Verify stale-proof failures against their owning producer; report remaining unrelated failures instead of claiming the whole suite passed.
- [ ] 9.2 Verify checkout-like execution with only tracked sources/fixtures plus declared build/content prerequisites, no ignored source-workstation logs, and explicit generated data preparation. Check source/fixture hashes and output destinations before and after repeat runs.
- [ ] 9.3 Reconcile active parent/child plans, review routing docs, producer catalogue and runnable examples. Preserve historical `logs/...` quotations as labeled history; remove them as live dependencies. List every audited producer as migrated or unaffected.
- [ ] 9.4 Record measured runtime/cache growth and cleanup estimates separately from observed drive free-space changes. Record commands, outcomes and limitations in this plan or a durable review/documentation owner, never only under disposable logs.
- [ ] 9.5 Mark logs safely disposable only when the tests prove it. Keep all uncompleted tasks unchecked and report final Git status without staging or committing.

## Completion conditions

- Permanent test inputs are immutable fixtures; authored assets retain their existing media authority.
- Generated test prerequisites have an explicit run owner, selected manifest and complete preparation path outside logs.
- Required proof transcripts, checkpoint indexes and previous-stage evidence live outside logs, including all nested dependencies.
- Clearing diagnostic logs between stopped runs/stages changes no pass/fail outcome and loses no required test input or accepted continuation state.
- Fresh checks do not rely on ignored historical executables, implicit latest runs or unrecorded workstation state.
- Runtime distribution/executable duplication is bounded, or a measured remaining limitation is explicitly left open; shared content never leaks writable state between processes.
- Cleanup keeps its preview, containment, tracked-input protection and correct hard-link accounting. Tests remain runnable after cleanup through retained or explicitly regenerated prerequisites.
- The actual migration and its verification are complete before any claim that all of `logs/` can be deleted safely.
