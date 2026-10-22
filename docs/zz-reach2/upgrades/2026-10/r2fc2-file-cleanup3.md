# R2FC2 - Remaining migrations, cleanup, transfer and Space resumption

Owner: this continuation of [completed preservation and artifact reconstruction](r2fc-file-cleanup3.md). Storage and rebuild contract: [historical debugging architecture](../../architecture/archi-historical-debug.md). Original group/task numbers are retained for continuity.

## Global Mandatory Reading

Read this section before work and after every context compaction. Read only the
nearest preceding **Area Mandatory Reading** block for the active group, plus its
explicitly relevant symbols. Reading is non-transitive. Do not reload the
completed plan or execution ledger wholesale.

- Read only the relevant rows/sections of [Storage ownership](../../architecture/archi-historical-debug.md#storage-ownership) and [Historical facts and current proof](../../architecture/archi-historical-debug.md#historical-facts-and-current-proof). Use that architecture for rebuild commands; do not create another runbook or asset catalogue.
- Execute groups top to bottom, sequentially, without subagents. Mark `[X]` after the stated outcome is verified and record one concise result before the next group.
- Preserve source, authored media, fixtures, every save, unique source epochs, other worktrees and valuable untracked/ignored files. No Git writes, staging, commit, reset, checkout or clean. Write outside repositories only in explicitly authorized temporary/Codex locations.
- New/edited code needs ownership headers, JavaDoc/Doxygen function/class/interface docs and clear argument comments. Explain compound conditions and nontrivial loops as business logic. Preserve immutable fixture bytes and independent expectations.
- Follow the **Fewer debug and verification cycles** policy below. Keep save-preservation and deletion safeguards intact.
- Put optional logs, debug output and scratch in `logs/` or `test-cache/`; required current transcripts belong in review `evidence/`. Long-term test tools stay beside tests. New source/test/JSON names must not begin `r2`.
- After compaction, note `Context has compacted X times while executing this group.` above the active group. After more than three compactions, split that group using B/C/D suffixes and matching local reading blocks; keep groups at most eight tasks. Do not subdivide merely to add proof detail.
- Keep assisting Markdown beside this plan using its own `r2fc2-file-cleanup3-a-...` prefix, then the next lowercase letter if needed. Prefer concise results in this plan over additional documents.

## Fewer debug and verification cycles

The earlier execution spent too long repeating dependency validation, native
qualification and debug runs. Finish the remaining functional work with fewer
cycles and existing tools:

1. Complete a cohesive group before its verification burst. Use one focused burst for the changed behavior; metadata/documentation edits need only focused static checks. Add tests only for meaningful behavior or a data-loss boundary, not to mirror trivial code.
2. Reuse unchanged verified fixtures, prepared source bytes and current build/qualification witnesses through their existing contracts. Do not replay completed groups, rehash entire dependency graphs at every wrapper boundary without a contract need, or rerun native captures merely because a report moved. Keep required input-integrity and final deletion checks.
3. Default to one optimized native configuration. Debug runs are required only for the explicit Space recovery/query obligations below or a concrete configuration-specific change/failure. Do not expand representative checks into every-family/both-configuration matrices.
4. Use one shared cold rehearsal for Groups30-32. Build the selected target union once per required configuration, run representative captures, then perform one staged clean/rebuild. That result satisfies every task referring to that regeneration check.
5. After a failure, inspect its retained artifacts, fix the cause, and rerun only the affected check. Additional debug/verification cycles require new failure evidence or a relevant source/input/image change; avoid blind retries and extra reporting layers.
6. No full regression is required. A justified broader run belongs at final closure, not after each group. Split any test runner exceeding two minutes into focused companions beside tests; keep native caps appropriate, wait at least50 seconds per running-process poll, use at most two build workers and disable owned node reuse.
7. Finish required documentation before the final archive. Record commands, result and material limitations once per group; keep iterative diagnostics out of the plan. Stop adding scope when the stated outcome and its focused checks pass.

| Work | Verification budget |
| --- | --- |
|20BC-22 consumer/history migration|One focused reader/publication burst per changed group; reuse20BB captures and existing Tube history/refusal.|
|23 documentation|One affected-link/command check; no runtime test.|
|24-29 export, runtime, cleaner and packaging|One focused burst per changed mechanism. Keep all save-export/deletion refusal cases; use Group27 for the cleaner suite rather than duplicating it in26.|
|30-32 cold rehearsal|One staged invocation sequence: required Space pair, representative optimized graphics, one clean and one selected rebuild.|
|33-34 real cleanup/package|Required preservation checks, preview/apply and idempotence; one final archive verification.|
|35-37 destination recovery/query|One qualification/query/reload sequence per required configuration; reuse exact still-valid results in the same checkout and perform final durable-reference checks without another native cycle.|

## Checkpoint and resume point

**Paused at the user's request,2026-10-22.** This edit revises planning only; it
does not resume implementation. Start at **Group20BC** when the user resumes.
The completed plan contains146 checked tasks; this plan contains all78 remaining
unchecked tasks. Task descriptions below have been reviewed to reduce repeated
debug and verification cycles while retaining their functional outcomes.

- Ground20BB is complete. Its current pure source run is `cleanup18c-native-rwd-02`, recipe `sprite-native-loaders`; decoder/consumer attempt is `cleanup20bb-viewport-01`. Exact build/capture witnesses are in the [paused checkpoint](r2fc-file-cleanup3-a-execution.md#checkpoint-paused-after-group20bb). Reuse them only while their existing contracts remain current.
- Group22 must durably publish the generated Space foundation before its original report folders are deleted. Before Group24/deletion work, refresh source epochs while original ignored snapshots still exist. Preserve all saves, including16 long-path duplicate aliases.
- Space remains at sealed GZVEEH; GZVEEI is unimplemented. No033:32 windows/64000 ticks are closed, both30000 exceptions spent, normal40000 operating and100000 capital floors restored. Read the live handover before dispatch. Cleanup/art/static qualification authorizes no paid window, purchase or gameplay tick.
- Tube Group9 history stays separate from provisional, unqualified five-model Group10 work. Preserve both source baseline epochs and keep that feature paused.
- Existing Group28 findings: transient WinError5 during atomic report replacement, added/removed source inventory checks, and legacy CLI/cache guards including `runtime_publication.py`. Fix existing owners; do not introduce another framework.
- Broad cleanup, automatic save export, packaging and cold rehearsal are still pending. Protect `OpenTTD-baseline/`, `!kit`, other worktrees and valuable working files. References to `save_retention.py` and `package_transfer.py` below name pending helpers, not delivered implementations.

## Tasks

### Area Mandatory Reading - Selected Ground closure without historical caches

_Applies to Group20BC._

Read qualify_ground_selected_closure.py:native_domain/immutable_views/qualify for
its original50family/domain/current-later boundary and historical-view limitation.
Use20BB's actual current source/decoder/selector/capture outputs and20B's retained
independent facts. Current views must have current native provenance; old executed
image hashes stay historical without retained EXE dependencies. Keep26later-owner
and all useful-route/operating/human gates open.

### Group20BC - Close explicit selected Ground review material

{{MEDIUM}}
- [ ] 20BC.1 Adapt current selected closure readers to explicit source/decoder/selector/capture witnesses, preserving exact domain and later-owner exclusions without historical image/EXE dependencies.
- [ ] 20BC.2 Run one focused closure/refusal burst against the existing current witnesses; verify source/GRF preservation without replaying the completed six-mode Ground capture matrix. Document the command and keep unfinished gameplay/human gates open.

### Area Mandatory Reading - Completed terminal versus provisional tube scope

_Applies to Group20C._

Read src/tests/fixtures/checkpoints/underground/checkpoint.json and history.json's
status/configurations/live_retained_inputs only, tube_terminal_checks.py:main,
and qualify_tube_terminals.py:main for its completed24case/8tag model9scope.
The source checkpoint preserves before.json and before-definitions.json as
distinct epochs. Do not resume the five-model feature or reclassify a current
compile as historical terminal qualification. Rebuild only documented producer
material; record any original/current source-epoch limitation explicitly.

### Group20C - Preserve paused tube review boundaries

{{MEDIUM}}
- [ ] 20C.1 Preserve completed tube terminal scope separately from provisional five-model work, with cleanable producer outputs and explicit current/historical evidence roles.
- [ ] 20C.2 Check the retained bounded tube witnesses or record the precise existing source-epoch refusal once. Preserve both source checkpoints and original art/GRF identities. Do not rebuild provisional five-model work to obtain a green result; useful-route/human gates remain open.

### Area Mandatory Reading - Migrate prior-stage and Space publication consumers

_Applies to Groups 21-22._

Entry gate: Groups9-10 supply verified immutable inputs/shared roles and Group14 supplies explicit selected recipes; Groups8/12 supply the Space checkpoint adapter. Verify those contracts. Stop dependent consumer migration if absent; trace an unclassified reader independently. Do not infer an accepted prior stage from a matching filename or choose the latest run.

| File name | Line Ranges | Description |
| --- | --- | --- |
| [State fixture](../../../../src/tests/multilayer_state_fixture.py) | 28-104 | `prior_artifact/log_path/output_save_path/save_path` are the existing shared selection seam. Keep exact prior run and newly reserved output semantics. |
| [Phase-pair reader](../../../../src/tests/multilayer/acceptance/phase_pair_index_checks-1.py) | 18-26;29-45 | Actual path/hash and owner-family comparisons must survive index relocation; replacing the index alone is insufficient. |
| [Original native driver](../../../../src/tests/multilayer/acceptance/surface_construction_cash_native.py) | 20-82 | Current receipt-based execution records terminal/harness/native reports. Migrate only active publication/adapters; retain historical caller contracts explicitly. |
| [Checkpoint adapter](../../../../src/tests/multilayer/acceptance/space_checkpoint.py) | Live Groups8/12/12B exports; no fixed range | `qualify_checkpoint/publish_successor` supply the new continuation foundation and complete selected successor evidence. |
| [Recipe executor](../../../../src/tests/multilayer/support/artifact_rebuild.py) | Live Group14 `load_recipe/prepare_recipe`; no fixed range | Explicit preparation of a missing prior stage must use the existing recipe dependency/status contract and a retained input, not silently run arbitrary historical stages. |

### Group 21 - Reconstruct prior-stage test evidence explicitly

{{HARD}}
- [ ] 21.1 Replace required log reads in shared state/profile/geology helpers with retained semantic fixtures or explicitly prepared prior-stage evidence. Parse diagnostics from the current process and publish only required semantics/transcripts.
- [ ] 21.2 Migrate TBM feedback/motion/pacing consumers through their actual prior-stage selectors. Preserve accepted/refused cases and configuration/source/save identities.
- [ ] 21.3 Add a bounded prerequisite recipe when a missing earlier stage can be regenerated from a retained fixture. Preserve required older-format inputs instead of generating them with the new save format.
- [ ] 21.4 Run one focused burst covering valid regeneration, missing prior evidence, wrong run/configuration and existing refusal semantics. Remove diagnostic logs in that fixture and confirm the same result; do not replay the full historical chain.
- [ ] 21.5 Close inventory dispositions using the migrated readers. Reuse existing selectors/recipes; add a new protocol or group only for a concrete unresolved dependency, not for extra proof scaffolding.

### Group 22 - Move active Space indexes and successor receipts

{{MEDIUM}}
- [ ] 22.1 Update current progress/phase-pair index producers and readers together to selected report paths. Keep exact path/hash references and complete nested input sets.
- [ ] 22.2 Remove live unique source snapshots, checkpoint indexes and required authorizations from logs/test-cache. Retain spent approval facts only as historical evidence, never reusable execution permission.
- [ ] 22.3 Replace active `--retain` behavior that recursively copies executables with checkpoint/successor publication. Fresh process identities remain in generated proof; durable continuation state uses the compact foundation.
- [ ] 22.4 Exercise one representative successor export, diagnostic removal and next qualification from retained inputs. Reuse this result at later entry gates while its source/input identities remain valid; preserve incomplete/refused status.

### Area Mandatory Reading - Update Space development instructions

_Applies to Group 23._

Entry gate: Groups15-22 have actual passing recipe commands or an explicit unresolved result per affected family. Validate commands against the live catalogue. Stop publishing "ready to rebuild/resume" for an unresolved family; classify historical documentation independently. Do not document hypothetical CLI behavior as delivered.

| File name | Line Ranges | Description |
| --- | --- | --- |
| [Space master](r2mlsp-space-master-plan.md) and [current handover](r2mlsp-z-z-z-z-z-q-launch-center-source.md#current-boundary) | Master7-9/287-291; handover5-17 | Replace current paths/practices and dispatch the actual next task. Keep historical results and closed policies truthful. |
| [Plan11 next task](r2mlvs11-orbital-stations-and-manufacturing.md#group-15csgzveei---price-genuine-additional-goods-service-before-further-capital-earnings) | 2954-2963 | Its Local block still directs future work to old logs/thirteen epochs/archived applications. Replace that bootstrap with the verified checkpoint foundation while preserving the actual Goods-query obligation. |
| [Shared layout](r2mlvs-multilayer-vehicles-and-structures.md#development-artifact-layout) | 36-49, then live Group10 changes | This shared owner must agree with child plans on generated versus durable inputs; no private Space exception keeping logs permanent. |

### Group 23 - Publish concrete cleanup-aware Space instructions

{{MEDIUM}}
- [ ] 23.1 Update master and affected Plans8/9/10/11/13/14 current instructions, mandatory-reading pointers and guide/handovers with actual retained paths and cleanable generated destinations. Label older output paths historical.
- [ ] 23.2 Document each required next-step asset through the existing catalogue: recipe, producer/tag, retained input, target/configuration, command and scoped result. Reference the shared architecture instead of duplicating a second runbook or catalogue.
- [ ] 23.3 Rewrite the current Space resumption instructions to consume the sealed foundation/new qualification and export future checkpoints. Explicitly remove instructions to archive another full EXE graph or replay the closed cash scope.
- [ ] 23.4 Record the post-clean development order and source-plan dispatch used by Groups35-37. Preserve current unfinished checkboxes; cold-load qualification does not complete GZVEEI.
- [ ] 23.5 Check affected live documentation links and command syntax once against the catalogue. Documentation-only edits need no native rebuild or runtime test; keep old receipt quotations historical.

### Area Mandatory Reading - Save export and runtime lifecycle

_Applies to Groups 24-25._

Entry gate: Group9 provides verified save aliases and Group22 publishes compact successors. Check those contracts before adding automatic export/retirement. Stop runtime removal if export is unavailable; measure copying independently. No unsupported save format or failed export may be treated as disposable.

| File name | Line Ranges | Description |
| --- | --- | --- |
| [Save stream reader](../../../../mlm/tools/savegames/savegame_plane_digest.py) | 87-105 | `read_stream` recognizes some compression tags but explicitly lacks LZO. Use header knowledge conservatively; decoder failure cannot justify discarding an old save. |
| [Python runtime materialization](../../../../src/tests/multilayer/support/artifact_paths.py) | 219-242 | `prepare_runtime` copies baseset/newgrf/lang/scripts. Keep mutable scripts/configs isolated while reducing repeated content work. |
| [Console image preparation](../../../../src/tests/multilayer/support/runner.py) | 86-121 | `console_executable` patches a disposable PE copy with atomic replacement and matching DLLs. Preserve source/peer image bytes and recorded identities. |
| [Native runtime materialization](../../../../src/tests/test_artifact_paths.cpp) | 205-226 | `GetTestRuntimeConfig` currently mirrors distribution copying. Python-only changes would leave native callers accumulating old homes. |

### Group 24 - Implement verified save export as a reusable operation

{{MEDIUM}}
- [ ] 24.1 Add `save_retention.py` beside support tests with `plan_save_exports` and `apply_save_exports`. Plan source/destination/hash aliases without writing; use Group9's stable owner and collision policy.
- [ ] 24.2 Copy each new save to a temporary destination, verify original and copied hashes, then atomically publish the retained payload and migration record. Keep source saves untouched during export.
- [ ] 24.3 Preserve all recognized extensions, backup forms and save-like headers even when the test decoder cannot parse them. A same-name/different-hash destination gets an explicit distinct alias or refusal, never overwrite.
- [ ] 24.4 Recheck changed/new save candidates before deletion authorization. A failed/partial export blocks the owning deletion target; rerun export safely after interruption.
- [ ] 24.5 Run one focused exporter burst covering dry planning, successful export, unsupported old format, destination conflict, truncated copy and source modification. Keep all data-loss refusal cases; no real cleanup occurs in these tests.

### Group 25 - Bound repeated runtime and executable growth

{{MEDIUM}}
- [ ] 25.1 Measure one representative repeated invocation by content, language, DLL, console EXE and mutable files. Apply the simplest useful reduction; do not design a new shared cache or perform an exhaustive performance study.
- [ ] 25.2 Stop recursive historical EXE retention in current producers. Keep only current build/run images and their generated provenance; use Group24 exports before retiring any completed home containing saves.
- [ ] 25.3 Reuse immutable prepared distribution content only where existing engine search paths or verified atomic copies support it. Preserve per-process configs/scripts/autosaves; no junction shortcut that contradicts cleaner safeguards.
- [ ] 25.4 Use one focused native/Python parity and host/client isolation burst, including one repeated-run size comparison and source/peer image preservation. Reuse the result in the cold rehearsal unless this mechanism changes.

### Area Mandatory Reading - Cleanup implementation and destruction tests

_Applies to Groups 26-28._

Entry gate: Group24 exports saves safely and Groups16-23 close required-input migrations. Verify export tests and zero unresolved required inputs in the durable inventory. Stop real deletion if absent; isolated target/refusal fixtures may continue. Do not weaken tracked/reparse/nested-repository guards to force a clean result.

| File name | Line Ranges | Description |
| --- | --- | --- |
| [Cleanup target selection](../../../../clean.ps1) | 162-240 | `Assert-CleanupTarget/New-CleanupEntry/Get-DevelopmentCleanupPlan` must share the same exact broad disposable-root policy; deleting more than the validated plan is forbidden. |
| [Cleanup entry point](../../../../clean.ps1) | 306-356 | Current preview exits before process checks/deletion; actual removal rechecks targets and tracked files. Add save export without causing preview writes or shell-crossed deletion. |
| [Existing cleanup tests](../../../../src/tests/multilayer/support/cleanup_check.ps1) | 39-79;94-142 | Current fixtures deliberately discard some generated saves and preserve logs/builds. Reverse those expectations under the new policy, while retaining isolation, hard-link and refusal checks. |
| [Save exporter](../../../../src/tests/multilayer/support/save_retention.py) | Live Group24 exports; no fixed range | `plan_save_exports/apply_save_exports` are the established preservation boundary; PowerShell consumes structured results rather than implementing a divergent save policy. |

### Group 26 - Extend cleanup through one export-and-delete pipeline

{{MEDIUM}}
- [ ] 26.1 Update target generation and the safety allowlist for all logs/test-cache/runtime profiles, generated reviews except their README, audited build roots/products and classified Python/scratch output. Unknown siblings remain outside the deletion set.
- [ ] 26.2 Integrate Group24 export planning into preview and verified export into apply. Check all required retained inputs before deletion; unresolved valuable content produces an explicit refusal.
- [ ] 26.3 Keep preview-by-default and DryRun/WhatIf precedence. Preview must create neither save archives nor directories; apply uses structured arguments and repository-contained native PowerShell deletion only.
- [ ] 26.4 Recheck writer state, absolute target/type, nested Git/reparse points, tracked files and save identities immediately before each removal. If new files appeared, refresh/export/refuse; do not delete a stale preview snapshot blindly.
- [ ] 26.5 Report completed targets and logical/distinct storage after partial failures; preserve source/media/fixtures/savegames/Git/baselines. Use Group27 for the focused cleaner test burst rather than starting a duplicate suite here.

Use the existing cleaner for preview, verified export, final safety rechecks and removal. The focused destruction/refusal checks belong to Group27.

### Group 27 - Exercise cleanup on a disposable fixture

{{MEDIUM}}
- [ ] 27.1 Move cleanup-test scratch under test-cache and keep every nested fake project inside its explicit fixture root. Update existing preserve/discard sets so generated saves are exported, logs/cache/builds are removed and canonical media survives.
- [ ] 27.2 Cover all runtime profiles, unmatched raw PPM, generated reviews, full builds and repeated apply. Verify the retained save bytes and alias map, not just whether a destination filename exists.
- [ ] 27.3 Verify dry-run/WhatIf write nothing and a failed export blocks deletion. Include changed/new save, interrupted copy and destination conflict.
- [ ] 27.4 Preserve tracked-file, sibling/outside-root, nested repository, junction/type-swap and active-writer refusal cases. Test hard links with an outside retained owner and accurate storage estimates.
- [ ] 27.5 Run this focused cleanup suite once after the pipeline edits are complete, including partial-deletion reporting and rerun after a resolved refusal. Retest only failing/affected cases after fixes; no real workspace cleanup occurs here.

### Group 28 - Prevent new undeclared historical dependencies

{{MEDIUM}}
- [ ] 28.1 Add a role-aware check over recipe/manifest references and known reader adapters. Diagnostic writes are allowed; unique retained inputs under deletion roots or unowned generated outputs are rejected.
- [ ] 28.2 Detect a nested receipt pointing into old logs, a stale current EXE requirement and an output that overwrites a fixture. Do not simply ban every string containing `logs`.
- [ ] 28.3 Verify generated prerequisite references are bound to the selected recipe/run and have a preparation path. Missing/corrupt expectations fail instead of being regenerated from current actual output.
- [ ] 28.4 Run the guard once after reader/catalogue migrations are complete and reconcile affected live families. Fix the known atomic-report retry, added/removed source inventory and legacy CLI/cache issues in their existing owners; avoid a new validation framework.

### Area Mandatory Reading - Packaging and cold reconstruction

_Applies to Groups 29-32._

Entry gate: Groups13-28 supply build/recipe/export/cleanup contracts and passing focused checks. Inspect registered recipes, retained identities and actual cleanup target selection. Stop cold acceptance if unresolved required inputs remain; independent package enumeration may continue. No original workstation build/proof folder, external worktree or network-hosted review archive may fill a missing dependency.

| File name | Line Ranges | Description |
| --- | --- | --- |
| [Recipe executor](../../../../src/tests/multilayer/support/artifact_rebuild.py) | Live Group14 `load_recipe/verify_retained_inputs/prepare_recipe`; no fixed range | Cold execution starts through the same public interface users will invoke. No private test-only fallback to old outputs. |
| [Checkpoint adapter](../../../../src/tests/multilayer/acceptance/space_checkpoint.py) | Live Groups8/12/15 seal and qualification exports; no fixed range | Verify original save/state, fresh executable and next-task foundation independently of deleted history. |
| [Save exporter](../../../../src/tests/multilayer/support/save_retention.py) | Live Group24 plan/apply contracts; no fixed range | Every staged/real cleanup must preserve newly produced saves as well as the original inventory. |
| [Cleaner](../../../../clean.ps1) | Live Group26 target/export/application pipeline; no fixed range | The cold fixture uses the same deletion behavior as the final repository, including complete log/cache removal and runtime profile coverage. |

### Group 29 - Implement a complete source transfer package

{{MEDIUM}}
- [ ] 29.1 Add `src/tests/multilayer/support/package_transfer.py` with `enumerate_preserved_files`, `stage_transfer`, `create_transfer_archive` and `verify_transfer_archive`. Enumerate preserved working files explicitly, including untracked/ignored source/assets/saves and required Git metadata; exclude only audited generated roots.
- [ ] 29.2 Detect external Git object stores/alternates, linked-worktree metadata, nested repositories and missing dependencies. Preserve real source repositories; do not silently produce an archive depending on an absent other checkout.
- [ ] 29.3 Stage a source-only tree in an allowed temporary location with all retained identities and no builds/logs/cache/runtime/generated reviews. Keep verification output under that staged project's logs/test-cache.
- [ ] 29.4 Verify archive paths/hashes and self-exclusion; produce an external checksum sidecar. Use one small extraction fixture containing an untracked source, authored PNG and retained save; reserve the actual full package check for Group34.
- [ ] 29.5 Record exact packaging/exclusion commands and the staged root; protect the live workspace throughout rehearsal.

### Group 30 - Rebuild and qualify the Space checkpoint in the staged tree

{{HARD}}
- [ ] 30.1 Run the single shared staged rehearsal for Groups30-32 after migration/helper edits are finished. Start from a missing build through the public recipe interface and reject old-workstation path/cache fallbacks.
- [ ] 30.2 Build the capital/ordinary target union once per required configuration in this staged rehearsal. Record actual tool/source/image identities and reuse unchanged images for later selected checks.
- [ ] 30.3 Run one load-only/static qualification per configuration for the original save pair, checking all121 families, bounded metadata allowances and observer state. Do not run any old cash/replay tag or repeat qualification after documentation-only edits.
- [ ] 30.4 Verify the fresh foundation resolves retained source/saves and current qualification, with one focused altered-time/state refusal check. Carry the qualified result forward while its source/input/image identities stay valid.

### Group 31 - Rebuild graphics witnesses in the staged tree

{{HARD}}
- [ ] 31.1 Let the selected recipe graph prepare only its required source conversions/packing with publication disabled; verify canonical source/media preservation within that run, not in a second standalone conversion sweep.
- [ ] 31.2 Run the native loader needed by the representative graphics selections in one optimized configuration. Do not sweep every family in both configurations; expand only for an actual changed mechanism or configuration-specific failure. Keep independent expectations and actual native provenance.
- [ ] 31.3 Capture one representative ordinary Space view and one Ground view in the optimized build using existing bounded producers. Record fixture/process/frame identities; do not replay complete historical matrices or add new acceptance layers.
- [ ] 31.4 Use the single staged clean/rebuild in32.3 to prove selected graphics regeneration. Compare the specified bytes/semantics there and retain existing corruption checks; do not add a separate graphics wipe/rebuild cycle.

### Group 32 - Rehearse continuation recovery and full staged cleanup

{{MEDIUM}}
- [ ] 32.1 Exercise one representative prior-stage state/TBM recipe with absent logs and freshly prepared inputs in the same staged rehearsal. Preserve existing refusal meanings and explicit prior-run identity.
- [ ] 32.2 Verify underground source/baseline/art material is present and distinguish its provisional feature from completed terminal obligations. Do not implement the five-model task to make cleanup look complete.
- [ ] 32.3 Apply the upgraded cleaner once to the staged project, verify all save exports and generated-root removal, then rebuild one selected graphics recipe. This same result closes31.4; no second staged clean/rebuild is required.
- [ ] 32.4 Record the shared rehearsal commands/results, limitations and footprint once for Groups30-32, and close real-deletion gates. Do not claim another machine was tested or repeat passed staged checks without a relevant change.

### Area Mandatory Reading - Real cleanup and transfer handover

_Applies to Groups 33-34._

Entry gate: Groups30-32 passed and the real live checkpoint has not advanced beyond the seal. Recheck the handover, inventory and current writer state. Stop real deletion if they changed; export/qualify the newer boundary first. A staged success cannot authorize deleting newly created source or saves.

| File name | Line Ranges | Description |
| --- | --- | --- |
| [Space current boundary](r2mlsp-z-z-z-z-z-q-launch-center-source.md#current-boundary) | Live Group23 recovery section; no fixed range | Identifies the currently selected durable checkpoint and next Space task. Reconcile concurrent progress before deletion. |
| [Cleaner](../../../../clean.ps1) | Live Group26 entry point and Group24 export adapter; no fixed range | Execute only the verified preview/apply behavior; no separate ad hoc recursive delete. |
| [Transfer helper](../../../../src/tests/multilayer/support/package_transfer.py) | Live Group29 `enumerate_preserved_files/stage_transfer/create_transfer_archive/verify_transfer_archive`; no fixed range | These public exports package all durable working material with explicit exclusions and an external checksum sidecar; verify actual exports rather than assuming an archive tool exists. |

### Group 33 - Apply verified cleanup to the real repository

{{SIMPLE}}
- [ ] 33.1 Revalidate the actual current checkpoint and source/save inventory against the live handover. Check relevant writers are stopped; never terminate unrelated processes.
- [ ] 33.2 Run the upgraded explicit preview and verify save export/retained inputs, then apply its audited targets with all gates satisfied.
- [ ] 33.3 Check source/media/Git/fixture/save preservation and the absence of generated logs/cache/runtime/build/review payloads. No disguised output archive may have been moved elsewhere.
- [ ] 33.4 Reapply to prove idempotence and record actual deleted totals/remaining footprint/partial failures. Keep any real failure open rather than marking cleanup complete.

### Group 34 - Package the clean project and finish its recovery handover

{{SIMPLE}}
- [ ] 34.1 Finalize the architecture runbook and Space/underground handovers with tested commands, retained hashes, tool versions, last results and next task before creating the final archive.
- [ ] 34.2 Leave the removable Space resume note: it can be deleted after the checkpoint is recovered. Update predecessor cleanup follow-ups and all active Space location/practice links.
- [ ] 34.3 Create the archive in the allowed temporary area, outside its payload. Verify members/hashes and extraction; include all valuable untracked/ignored working files and required Git metadata.
- [ ] 34.4 Record actual tree/archive sizes and checksum. Refresh the archive only if its payload changed; avoid further discretionary edits after packaging and preserve indispensable inputs regardless of size target.
- [ ] 34.5 Mark the transfer-ready milestone only when its checks pass. Keep the final source tree clean for transfer; Groups35-37 are executed in the chosen development checkout when work resumes.

### Area Mandatory Reading - Resume actual Space development

_Applies to Groups 35-37._

Entry gate: Group34's retained payload is available in the chosen development checkout and the Group30 checkpoint qualification can be reproduced there. Check exact source/save identities and tool prerequisites. If the destination has not yet received the project, Groups35-37 stay pending; transfer readiness remains a separate completed milestone. Independent documentation can continue. Do not pull source from an unselected worktree, replay GZVEEH or use old EXE archives as recovery.

| File name | Line Ranges | Description |
| --- | --- | --- |
| [Space master](r2mlsp-space-master-plan.md) and [active handover](r2mlsp-z-z-z-z-z-q-launch-center-source.md#current-boundary) | Live Group23/34 dispatch and recovery sections; no fixed range | Select the actual first unfinished Space task after cleanup; the planning snapshot expects GZVEEI, but a newer recorded dispatch wins. |
| [Plan11 next-task contract](r2mlvs11-orbital-stations-and-manufacturing.md#group-15csgzveei---price-genuine-additional-goods-service-before-further-capital-earnings) | GZVEEI Local/Group, originally2954-2963, plus its explicitly required Global/Presentation/AD-12 sections | This source plan owns the new Goods-demand/native-capacity/full-cost query and its accounting. On dispatch read its relevant mandatory sections; do not treat cleanup qualification as implementation of that query. |
| [Checkpoint adapter](../../../../src/tests/multilayer/acceptance/space_checkpoint.py) | Live `load_checkpoint/qualify_checkpoint/publish_successor`; no fixed range | Actual development uses the retained foundation and fresh proof, then exports a new durable checkpoint before a later clean. No recursive retained-EXE foundation is reintroduced. |

### Group 35 - Recover the chosen development checkout

{{MEDIUM}}
- [ ] 35.1 Verify retained source/assets/saves and tools in the chosen checkout. Run one checkpoint qualification per configuration there; reuse Group30 only if it is this exact checkout with unchanged source/tool/save/image identities. Never reuse a staged proof across a different path or machine.
- [ ] 35.2 Recover the exact pair/current state/closed policies and read the live Space dispatch plus its relevant Global/Local sections. A newer explicit checkpoint wins over this plan's dated GZVEEI snapshot.
- [ ] 35.3 Prepare only missing assets/build targets required by the actual next task through existing recipes. Reuse valid selected prerequisites; do not trigger a broad graphics or historical acceptance sweep.
- [ ] 35.4 Record recovery complete and the source-plan task to execute next. The removable checkpoint-recovery note may now be deleted; feature implementation has its own following completion checks.

### Group 36 - Execute the next bounded Space task through the new foundation

{{HARD}}
- [ ] 36.1 Dispatch the live source-plan task under its own numbering, rather than duplicating gameplay authority here. If it is still Plan11GZVEEI, create its separately selected pure Goods proposal using the qualified checkpoint argument and existing native command/route/cost owners.
- [ ] 36.2 For GZVEEI, read actual station13 waiting Goods/station15 acceptance and packet origins, original49/mixed52 capacity, depot1638 connectivity and platform fit. Quote a currently available callback-free head plus2/4/8/12Goods-wagon lots; include their first full operating interval, every existing owner cost and100,000capital floor. Preserve actual availability/refusals.
- [ ] 36.3 After the cohesive query edits, build the isolated target and qualify/query/reload once per required configuration. Batch the proposal lots in that bounded run. Require zero gameplay ticks/all121 families and unchanged body/material/finance observations; no cash-work033, purchases or old cash-specific cases.
- [ ] 36.4 Bind the actual proposal/refusal through the new successor contract and complete only the corresponding source-plan tasks whose evidence passed. Preserve closed financial scopes; the next paid producer remains subject to its own full native admission and source-plan requirements.

At this snapshot the query can reuse the existing estimate adapter:

```cpp
const auto quote = QuoteSurfaceResupply<Commands::BuildVehicle>(
    depot, depot, candidate, false, goods, ClientID::Server);
```

This fragment obtains one native body quote. The actual task separately proves model/callback eligibility, route/platform fit and the complete proposed-consist/company invoice. `CloneVehicle` carries `CommandFlag::NoTest` in `vehicle_cmd.h`; it cannot replace a reliable whole-purchase estimate.

### Group 37 - Publish the next checkpoint and continue the Space queue

{{MEDIUM}}
- [ ] 37.1 Export the selected task's actual successor save/contract and source epoch through the established checkpoint/save-retention helpers. Preserve original and successor identities; generated process/review output remains cleanable.
- [ ] 37.2 Update the Space master/child handover with real results and the next unfinished task. Mark development resumed only after actual source-plan work, not from archive extraction or a successful static load.
- [ ] 37.3 Perform one read-only check that the exported continuation resolves durable inputs without EXE retention. Reuse the successful Group36 execution evidence; no extra native rebuild/reload is required unless the checkpoint or relevant code changed. Keep underground work paused.

Transfer-ready completion is Group34. Development-resumed completion requires Groups35-37 in the chosen checkout. If transfer has not yet occurred, report those latter tasks pending rather than claiming remote execution.
