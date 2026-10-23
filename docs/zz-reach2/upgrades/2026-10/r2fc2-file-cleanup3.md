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

**Completed,2026-10-23.** All continuation groups20BC-37passed their stated
checks. Group34is the verified source-transfer snapshot;35-37then recovered the
user-selected original checkout and completed actual GZVEEIquery development.
The exported successor dispatches Plan11GZVEEJ's read-only usefulness gate.
Original Space/useful-route/human gates and provisional Tube work remain open.
The checked tasks and group results below record the implementation boundary.
The completed plan contains146 checked tasks; this continuation began with78
unchecked tasks. The live checkboxes below record completed groups. Task descriptions were reviewed to reduce repeated
debug and verification cycles while retaining their functional outcomes.

- Ground20BB is complete. Its current pure source run is `cleanup18c-native-rwd-02`, recipe `sprite-native-loaders`; decoder/consumer attempt is `cleanup20bb-viewport-01`. Exact build/capture witnesses are in the [paused checkpoint](r2fc-file-cleanup3-a-execution.md#checkpoint-paused-after-group20bb). Reuse them only while their existing contracts remain current.
- Group22 must durably publish the generated Space foundation before its original report folders are deleted. Before Group24/deletion work, refresh source epochs while original ignored snapshots still exist. Preserve all saves, including16 long-path duplicate aliases.
- The original GZVEEH seal remains unchanged; GZVEEIpassed at zero ticks and the current successor dispatches GZVEEJread-only usefulness. No033:32 windows/64000 ticks are closed, both30000 exceptions spent, normal40000 operating and100000 capital floors restored. Read the live handover before dispatch. Cleanup/art/static qualification authorizes no paid window, purchase or gameplay tick.
- Tube Group9 history stays separate from provisional, unqualified five-model Group10 work. Preserve both source baseline epochs and keep that feature paused.
- Existing Group28 findings: transient WinError5 during atomic report replacement, added/removed source inventory checks, and legacy CLI/cache guards including `runtime_publication.py`. Fix existing owners; do not introduce another framework.
- Cold rehearsal, real cleanup and source packaging passed Groups30–34. Protect `OpenTTD-baseline/`, `!kit`, other worktrees and valuable working files. Groups35–37 recover and continue Space in the selected checkout; the transfer archive records the completed Group34 boundary.

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
- [X] 20BC.1 Adapt current selected closure readers to explicit source/decoder/selector/capture witnesses, preserving exact domain and later-owner exclusions without historical image/EXE dependencies.
- [X] 20BC.2 Run one focused closure/refusal burst against the existing current witnesses; verify source/GRF preservation without replaying the completed six-mode Ground capture matrix. Document the command and keep unfinished gameplay/human gates open.

Result,2026-10-22: `python -B src/tests/multilayer/selected_ground_closure_check.py`
passed seven focused domain/current-history refusals (0.078s). The explicit
`qualify_ground_selected_closure.py --configuration RelWithDebInfo --run-id cleanup20bc-closure-rwd-01`
selection passed50families/15488poses,24current/26deferred, using
`--selector-result mlm/reviews/selected-ground/current-source-consumers/cleanup18c-native-rwd-02/reports/prepared-cleanup20bb-viewport-01/current-RelWithDebInfo.json`.
Its six `--<mode>-result` arguments are the exact20BB optimized report paths
under `mlm/reviews/selected-ground/`: collector=`retained-collector/cleanup20bb-collector-rwd-03`,
industry-states=`industry-states/cleanup20bb-industry-rwd-01`,
purchases/rail/road=`transport-<mode>/cleanup20bb-<mode>-rwd-01`,
stations=`stations/cleanup20bb-stations-rwd-01`, each suffixed
`/reports/current-RelWithDebInfo.json`. Output:
`mlm/reviews/selected-ground/selected-closure/cleanup20bc-closure-rwd-01/reports/current-RelWithDebInfo.json`;
diagnostics: `test-cache/cleanup/ground-selected-closure-01.log`.
Current source/decoder/process/frame contracts and independent original source
facts passed; no native build/capture was repeated. Historical image/EXE paths
are not read. Canonical GRF remains SHA256
`c04fbf0d5525edc53250cfbd963f5e3bbe0d0bce655a3553c73b76483a8e8744`;
all useful-route, operating, paid-progress and human gates remain open.

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
- [X] 20C.1 Preserve completed tube terminal scope separately from provisional five-model work, with cleanable producer outputs and explicit current/historical evidence roles.
- [X] 20C.2 Check the retained bounded tube witnesses or record the precise existing source-epoch refusal once. Preserve both source checkpoints and original art/GRF identities. Do not rebuild provisional five-model work to obtain a green result; useful-route/human gates remain open.

Result,2026-10-22: `python -B src/tests/multilayer/qualify_tube_terminals.py --run-id cleanup20c-history-01`
verified42durable retained inputs, both distinct baseline epochs, all four
provisional source copies,772original media payloads and both historical
24-case/8687-assertion/eight-tag model9 terminal observations. Cleanable result:
`mlm/reviews/underground-tubes/terminal-history/cleanup20c-history-01/reports/history.json`;
diagnostics: `test-cache/cleanup/tube-terminal-history-01.log`.
The legacy `tube_terminal_checks.py --mode run --configuration RelWithDebInfo --run logs/underground-tubes/cleanup20c-must-refuse`
refused before creating its output directory: original native sources differ
at `src/CMakeLists.txt` and `src/transport_tube_engine.cpp`; the migrated legacy
driver also has its own new source identity. Refusal transcript:
`test-cache/cleanup/tube-terminal-refusal-01.log`. No provisional build/native
execution occurred; current native qualification, five-model work, useful-route
and human acceptance remain open. No historical EXE/log alias was dereferenced.

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
- [X] 21.1 Replace required log reads in shared state/profile/geology helpers with retained semantic fixtures or explicitly prepared prior-stage evidence. Parse diagnostics from the current process and publish only required semantics/transcripts.
- [X] 21.2 Migrate TBM feedback/motion/pacing consumers through their actual prior-stage selectors. Preserve accepted/refused cases and configuration/source/save identities.
- [X] 21.3 Add a bounded prerequisite recipe when a missing earlier stage can be regenerated from a retained fixture. Preserve required older-format inputs instead of generating them with the new save format.
- [X] 21.4 Run one focused burst covering valid regeneration, missing prior evidence, wrong run/configuration and existing refusal semantics. Remove diagnostic logs in that fixture and confirm the same result; do not replay the full historical chain.
- [X] 21.5 Close inventory dispositions using the migrated readers. Reuse existing selectors/recipes; add a new protocol or group only for a concrete unresolved dependency, not for extra proof scaffolding.

Result,2026-10-22: shared state/profile/geology and actual TBM selectors now use
`prior_stage_evidence.py`: exact prior feature/scenario/run/configuration,
compact actual semantic lines, historical source/image identities and verified
retained-first save roles, including explicit refused-output absence. Logs and
old EXEs are not read. Twelve focused tests passed across
`prior_stage_evidence_check.py` (6), `prior_stage_compatibility_check.py` (3)
and existing `legacy_fixture_paths_check.py` (3); corresponding diagnostics are
`test-cache/cleanup/prior-stage-evidence-01.log`,
`prior-stage-compatibility-01.log` and `legacy-prior-paths-01.log`.
The fixture removes optional diagnostics/old application and retains identical
semantic/save selection. Missing/wrong prior, configuration/source absence,
changed saves, newly appeared refused outputs and old-format geology authority
changes refuse. Original371/372-378/393/394 inputs remain retained; current
writers cannot substitute for an old-rate393 pacing seed, and pacing continuation
requires its actual393/394 format. Missing original pacing semantics remain an
explicit prerequisite refusal, never an inferred success from matching filenames.

Public native regeneration passed:
`python -B src/tests/multilayer/support/artifact_rebuild.py state-prior-native --configuration RelWithDebInfo --run-id cleanup21-prior-rwd-02 --vcpkg D:/Codez/Tools/vcpkg --grfcodec D:/Codez/Tools/grfcodec/grfcodec-6.2.0-windows-win64/bin`.
This builds only ordinary `openttd`, reloads the retained original371 save once,
and publishes `mlm/reviews/state/metadata/cleanup21-prior-rwd-02/reports/prior-native-RelWithDebInfo.json`
plus `prior-r2mlvs14-b-native-reload-semantics.json`; graph result is
`mlm/reviews/rebuild/state-prior-native/cleanup21-prior-rwd-02/reports/rebuild.json`.
Attempt01 failed in the nested shell driver; direct native console launch now
records actual process/transcripts before raising. Only this affected native
check was retried; no historical chain or full regression was replayed.
The catalogue also exposes historical-only `tube-checkpoint` preparation;
all six state/TBM inventory dispositions are reconciled. Phase-pair consumers
continue in Group22.

### Group 22 - Move active Space indexes and successor receipts

{{MEDIUM}}
- [X] 22.1 Update current progress/phase-pair index producers and readers together to selected report paths. Keep exact path/hash references and complete nested input sets.
- [X] 22.2 Remove live unique source snapshots, checkpoint indexes and required authorizations from logs/test-cache. Retain spent approval facts only as historical evidence, never reusable execution permission.
- [X] 22.3 Replace active `--retain` behavior that recursively copies executables with checkpoint/successor publication. Fresh process identities remain in generated proof; durable continuation state uses the compact foundation.
- [X] 22.4 Exercise one representative successor export, diagnostic removal and next qualification from retained inputs. Reuse this result at later entry gates while its source/input identities remain valid; preserve incomplete/refused status.

Result,2026-10-22: `space_checkpoint.export_foundation` exports only the declared
compact foundation/operation lineage into the durable checkpoint owner, with
verified save/receipt/transcript/source-epoch/profile roles and no EXEs. It keeps
original process path/hash facts inside immutable witnesses while selecting
the exact relocated parent/save roles. All consumed financial authorizations
remain spent historical facts. Three consumed adapter versions were retained
in the existing source-epoch inventory before editing; broad inventory refresh
still precedes Group24/deletion.

Actual command:
`python -B src/tests/multilayer/acceptance/space_checkpoint.py --export-foundation mlm/reviews/space/checkpoint-qualification/cleanup15-qualified-01/reports/foundation.json`.
Verified durable selection:
`src/tests/fixtures/checkpoints/space/continuations/7b9db672fe0057dba73942b65085749ace8ce5ec6df819eebfdd85fb1c4daa4a/reports/foundation.json`,
SHA256 `56b978186f95d615be4c386fc833afb0ce632be1042ca358340ea4e2c98fd146`.
The original seal/save pair is unchanged; this completed qualification grants
no current paid or gameplay permission. Actual export/verification diagnostics:
`test-cache/cleanup/space-foundation-durable-{export,verify}-01.log`.

Focused synthetic transport checks `space_checkpoint_export_check-1.py` and
`-2.py` passed (67.391s/119.714s): idempotent export, diagnostic removal, a new
qualification from retained originals and unchanged synthetic current witnesses,
two selected successors, complete ancestry after removal of fixture generated
roots/images, and copied-witness corruption refusal. These tests do not claim
new real native Space execution. Short role/hash filenames fixed the observed
Windows path-length failure; only affected fixtures were retried. Diagnostics:
`test-cache/cleanup/space-checkpoint-export-{1-03,2-02}.log`.

The phase index now accepts explicit current progress roots or exact retained
historical aliases, preserves original nested identities and publishes actual
retained report/save paths. Its focused read-only command with
`--handling-release src/tests/fixtures/space/phase-history/reports/2222f51beff29b73-handling-phase.json`
and `--handling-debug src/tests/fixtures/space/phase-history/reports/ebea4239d0bbed91-handling-phase.json`
verified23pairs per configuration; output is
`mlm/reviews/space/phase-pair-index/20261022-074906-8bfb6074e2/reports/phase-pair-index.json`.
All46pairs and selected report references avoid diagnostic roots.
Legacy recursive `bind(... retained_directory=...)` now refuses; the current
`--retain --foundation <typed-selection>` route exports the compact foundation.
No native/gameplay run or real cleanup occurred in this group.

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
- [X] 23.1 Update master and affected Plans8/9/10/11/13/14 current instructions, mandatory-reading pointers and guide/handovers with actual retained paths and cleanable generated destinations. Label older output paths historical.
- [X] 23.2 Document each required next-step asset through the existing catalogue: recipe, producer/tag, retained input, target/configuration, command and scoped result. Reference the shared architecture instead of duplicating a second runbook or catalogue.
- [X] 23.3 Rewrite the current Space resumption instructions to consume the sealed foundation/new qualification and export future checkpoints. Explicitly remove instructions to archive another full EXE graph or replay the closed cash scope.
- [X] 23.4 Record the post-clean development order and source-plan dispatch used by Groups35-37. Preserve current unfinished checkboxes; cold-load qualification does not complete GZVEEI.
- [X] 23.5 Check affected live documentation links and command syntax once against the catalogue. Documentation-only edits need no native rebuild or runtime test; keep old receipt quotations historical.

Result,2026-10-22: updated the Space master, Plans8/9/10/11/13/14 mandatory
recovery pointers, parent layout, current launch-center handover and Space
player-guide dispatch. The shared architecture alone owns actual recipe
scopes, retained inputs, fresh paired qualification/API commands and durable
export. GZVEEI's nearest Local block now uses the qualified typed foundation,
preserving its real Goods-demand/route/capacity/full-cost obligation and all
unfinished feature checkboxes. It no longer bootstraps from thirteen old EXE
epochs or closed cash work. Missing original pacing semantics and the absent
new Goods producer/tag remain explicit unresolved cases. Recorded execution
order is helper completion24-29, shared staged rehearsal30-32, real cleanup and
archive33-34, chosen-checkout recovery35, source-plan query36 and successor37.
One focused static burst checked49affected local links/anchors, the actual
Python qualification/export snippet syntax and seven named enabled catalogue
recipes. Rebuildable diagnostic report: `test-cache/cleanup/documentation-group23.json`.
No native build, runtime test or regression was required for this group.

### Area Mandatory Reading - Save export and runtime lifecycle

_Applies to Groups 24-25._

Entry gate: Group9 provides verified save aliases and Group22 publishes compact successors. Check those contracts before adding automatic export/retirement. Stop runtime removal if export is unavailable; measure copying independently. No unsupported save format or failed export may be treated as disposable.

| File name | Line Ranges | Description |
| --- | --- | --- |
| [Save stream reader](../../../../mlm/tools/savegames/savegame_plane_digest.py) | 87-105 | `read_stream` recognizes some compression tags but explicitly lacks LZO. Use header knowledge conservatively; decoder failure cannot justify discarding an old save. |
| [Python runtime materialization](../../../../src/tests/multilayer/support/artifact_paths.py) | 219-242 | `prepare_runtime` copies baseset/newgrf/lang/scripts. Keep mutable scripts/configs isolated while reducing repeated content work. |
| [Console image preparation](../../../../src/tests/multilayer/support/runner.py) | 86-121 | `console_executable` patches a disposable PE copy with atomic replacement and matching DLLs. Preserve source/peer image bytes and recorded identities. |
| [Native runtime materialization](../../../../src/tests/test_artifact_paths.cpp) | 205-226 | `GetTestRuntimeConfig` currently mirrors distribution copying. Python-only changes would leave native callers accumulating old homes. |

Context has compacted 1 times while executing this group.

### Group 24 - Implement verified save export as a reusable operation

{{MEDIUM}}
- [X] 24.1 Add `save_retention.py` beside support tests with `plan_save_exports` and `apply_save_exports`. Plan source/destination/hash aliases without writing; use Group9's stable owner and collision policy.
- [X] 24.2 Copy each new save to a temporary destination, verify original and copied hashes, then atomically publish the retained payload and migration record. Keep source saves untouched during export.
- [X] 24.3 Preserve all recognized extensions, backup forms and save-like headers even when the test decoder cannot parse them. A same-name/different-hash destination gets an explicit distinct alias or refusal, never overwrite.
- [X] 24.4 Recheck changed/new save candidates before deletion authorization. A failed/partial export blocks the owning deletion target; rerun export safely after interruption.
- [X] 24.5 Run one focused exporter burst covering dry planning, successful export, unsupported old format, destination conflict, truncated copy and source modification. Keep all data-loss refusal cases; no real cleanup occurs in these tests.

Result,2026-10-22: added `src/tests/multilayer/support/save_retention.py`:
`plan_save_exports`, `apply_save_exports` and `verify_save_exports`. Default CLI
prints a read-only JSON proposal; `--apply` exports without removing sources.
It reuses verified Group9 payloads and `savegames/retained/`, assigns changed
same-name saves distinct bounded hash owners, atomically admits verified copies
without overwrite, then commits `src/tests/fixtures/cleanup/save-exports.json`.
That incremental journal retains all versions/aliases without rebinding frozen
Group9 inputs. Changed/new/missing candidates, damaged copies, concurrent record
edits and retention inside a deletion scope refuse authorization. Interrupted
publication leaves reusable exact copies and no committed deletion authority.

`python -B src/tests/multilayer/support/save_retention_check.py` exercised12
focused cases; five passed initially and the seven affected copy cases passed
on their bounded rerun (0.997s) after fixing Windows' writable-descriptor flush
requirement. Logs: `test-cache/cleanup/save-retention-{01,02}.log`.
Entry refresh `artifact_source_epochs.py --export` passed2827source payloads
(983unique retained/1844identical live references),970generated candidates;
its four focused checks passed0.203s. Rebuilt mutable `rev.cpp` now resolves
only to its previously declared exact old copy, without duplicate retention.
Logs: `test-cache/cleanup/source-epochs-{entry-check-01,refresh-before-export-02}.log`.
No real cleanup, save mutation or native rebuild occurred in this group.

### Group 25 - Bound repeated runtime and executable growth

{{MEDIUM}}
- [X] 25.1 Measure one representative repeated invocation by content, language, DLL, console EXE and mutable files. Apply the simplest useful reduction; do not design a new shared cache or perform an exhaustive performance study.
- [X] 25.2 Stop recursive historical EXE retention in current producers. Keep only current build/run images and their generated provenance; use Group24 exports before retiring any completed home containing saves.
- [X] 25.3 Reuse immutable prepared distribution content only where existing engine search paths or verified atomic copies support it. Preserve per-process configs/scripts/autosaves; no junction shortcut that contradicts cleaner safeguards.
- [X] 25.4 Use one focused native/Python parity and host/client isolation burst, including one repeated-run size comparison and source/peer image preservation. Reuse the result in the cold rehearsal unless this mechanism changes.

Result,2026-10-22: Python `prepare_runtime` and native `GetTestRuntimeConfig`
copy English/US-English fallback packs plus `[misc].language`, instead of all
translations. BOM, empty native defaults and invalid language paths have matching
handling. Private configs/scripts/autosaves and ordinary distribution copies
remain isolated. Console/DLL preparation reuses exact matching bytes and replaces
changed entries atomically, preserving source and hard-linked historical peers.
Enabled current recipes keep only their current build/run images and generated
identity facts;21/22 already removed historical image retention. No completed
home was retired here; Group26 must use24's verified export before removal.
Legacy non-catalogue publication/CLI guards remain the explicit Group28 work.

One representative new home (`cleanup25-runtime-01`) used38374065bytes:
23927692content,369476language,14076416console,223scripts, zeroDLLbytes.
Language copying fell by14220333bytes; repeated preparation added zero bytes
and changed no timestamps. Rebuildable observation:
`test-cache/cleanup/runtime-{entry-size,size-comparison}.json`.
Build the small actual native owner with
`src/tests/multilayer/support/artifact_paths_probe.cmd`, then run
`python -B src/tests/multilayer/support/runtime_growth_check.py --application test-cache/cleanup/artifact_paths_probe.exe`.
The first three focused checks passed0.677s; the added BOM/empty-default parity
case passed0.528s. Logs: `test-cache/cleanup/runtime-growth-{01,02}.log`.
The probe links no world implementation; no native gameplay, paid window, game
build or real cleanup was performed. Carry these results into the cold rehearsal.

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
- [X] 26.1 Update target generation and the safety allowlist for all logs/test-cache/runtime profiles, generated reviews except their README, audited build roots/products and classified Python/scratch output. Unknown siblings remain outside the deletion set.
- [X] 26.2 Integrate Group24 export planning into preview and verified export into apply. Check all required retained inputs before deletion; unresolved valuable content produces an explicit refusal.
- [X] 26.3 Keep preview-by-default and DryRun/WhatIf precedence. Preview must create neither save archives nor directories; apply uses structured arguments and repository-contained native PowerShell deletion only.
- [X] 26.4 Recheck writer state, absolute target/type, nested Git/reparse points, tracked files and save identities immediately before each removal. If new files appeared, refresh/export/refuse; do not delete a stale preview snapshot blindly.
- [X] 26.5 Report completed targets and logical/distinct storage after partial failures; preserve source/media/fixtures/savegames/Git/baselines. Use Group27 for the focused cleaner test burst rather than starting a duplicate suite here.

Implementation result,2026-10-22: `clean.ps1` now uses one
`Invoke-DevelopmentCleanup` pipeline and the same exact target allowlist for
planning/removal. It covers all runtime profiles, logs/cache, generated review
children except README, audited full build roots and source-backed Python caches.
`-KeepBuildIntermediates`/`-KeepBuild` conservatively retains whole build roots.
Structured stdin JSON goes to the existing `artifact_preservation.py --cleanup`
adapter; Group24's exporter also supports native configs in their established
durable owner. Required migration/catalogue/source epochs, durable Space
foundations and paused Tube physical inputs are checked; unclassified source
refuses deletion. Historical nested image paths are not imported as dependencies.

Apply commits exports before native PowerShell removal, rechecks exact file
sets/metadata/allocation identities, save hashes, stopped writers and fresh Git
tracking, and reports actual removed lengths/distinct allocations after partial
failure. No unrelated process is terminated. PowerShell's parser accepted all18
functions and Python AST checks passed both changed support modules. As planned,
behavior/destruction verification follows once in27; no real preview/apply or
workspace cleanup was performed here. Existing stale/unclassified real inputs
remain refusals until Group28's audit and the later deletion gates pass.

Use the existing cleaner for preview, verified export, final safety rechecks and removal. The focused destruction/refusal checks belong to Group27.

### Group 27 - Exercise cleanup on a disposable fixture

{{MEDIUM}}
- [X] 27.1 Move cleanup-test scratch under test-cache and keep every nested fake project inside its explicit fixture root. Update existing preserve/discard sets so generated saves are exported, logs/cache/builds are removed and canonical media survives.
- [X] 27.2 Cover all runtime profiles, unmatched raw PPM, generated reviews, full builds and repeated apply. Verify the retained save bytes and alias map, not just whether a destination filename exists.
- [X] 27.3 Verify dry-run/WhatIf write nothing and a failed export blocks deletion. Include changed/new save, interrupted copy and destination conflict.
- [X] 27.4 Preserve tracked-file, sibling/outside-root, nested repository, junction/type-swap and active-writer refusal cases. Test hard links with an outside retained owner and accurate storage estimates.
- [X] 27.5 Run this focused cleanup suite once after the pipeline edits are complete, including partial-deletion reporting and rerun after a resolved refusal. Retest only failing/affected cases after fixes; no real workspace cleanup occurs here.

Result,2026-10-22: `powershell -NoProfile -NonInteractive -File
src/tests/multilayer/support/cleanup_check.ps1` passed86scope/export checks;
companion `cleanup_check-1.ps1` passed35refusal/recovery checks, each below the
two-minute budget. `cleanup_fixture.ps1` keeps all synthetic projects under
`test-cache/cleanup/cleaner-checks/`; it doubles Git/writer observations only.
Actual discovery, preservation/export, hash/identity checks and native PowerShell
removal are exercised. Whole builds, every runtime profile, unmatched PPM and
generated reviews disappeared while authored media, source, fixture/save/Git
owners and unknown siblings survived. Nine save/config aliases were checked
against independent original bytes and their committed hashes. Preview and
WhatIf changed no fixture path/bytes; repeated apply was empty.

The companion passed nested-Git, junction, type-swap, tracking, active-writer,
unclassified-source and conflicting-destination refusals; actual exclusive
journal locking interrupted export publication without deleting originals.
After-export changed/new saves refused final removal and recovered through
replan/export, retaining both changed-save versions. Group24's unchanged copy
truncation/mid-copy-source refusals supply the lower-level copy boundary.
Injected partial removal accurately reported3logical/3distinct bytes and
stopped before remaining data; a resolved rerun completed. Complete child stderr
is now captured so preservation failures expose their actual reason.
Logs: `test-cache/cleanup/cleaner-{scope,refusals}-01.log`.
No real workspace cleanup or Git mutation occurred.

Context has compacted 1 times while executing this group.

### Group 28 - Prevent new undeclared historical dependencies

{{MEDIUM}}
- [X] 28.1 Add a role-aware check over recipe/manifest references and known reader adapters. Diagnostic writes are allowed; unique retained inputs under deletion roots or unowned generated outputs are rejected.
- [X] 28.2 Detect a nested receipt pointing into old logs, a stale current EXE requirement and an output that overwrites a fixture. Do not simply ban every string containing `logs`.
- [X] 28.3 Verify generated prerequisite references are bound to the selected recipe/run and have a preparation path. Missing/corrupt expectations fail instead of being regenerated from current actual output.
- [X] 28.4 Run the guard once after reader/catalogue migrations are complete and reconcile affected live families. Fix the known atomic-report retry, added/removed source inventory and legacy CLI/cache issues in their existing owners; avoid a new validation framework.

Result,2026-10-22: the existing recipe/stage owners now validate typed live
roles, retained-first aliases, exact selected prerequisites and run-owned
outputs. Historical quotations and diagnostic writes remain allowed. Current
build reuse rejects archived images and added/removed source files; atomic
report replacement retries only bounded Windows sharing/access failures.
Three legacy cache-publication CLIs refuse before importing old readers.
`dependency_roles_check.py` passed9checks, `artifact_shared_build_check.py`
passed8, and the affected `artifact_rebuild_check.py` integration cases passed
after correcting relative declaration handling. The actual catalogue audit
passed100recipes/97enabled/6314retained roles/160generated prerequisites.
Logs: `test-cache/cleanup/dependency-role-audit-01.log`,
`recipe-guard-integration-03.log` and the matching focused check transcripts.

The real preservation gate passed6238required paths and3567source payloads,
with zero unresolved required inputs (199.188s; inventory, not a unit runner).
Newly discovered unique source aliases were retained through the existing
source-epoch owner; exact pinned provider members remain reconstructible.
Sixteen differing consumed native source versions and their original inventory
were recovered by exact SHA256, without changing working source or Git state.
Five source-history checks, six traversal checks and the affected provider
classification check passed. Save preservation now includes startup `.scr`
files; the affected cleaner fixture passed88checks. Required source/byte gates
remain intact; cached directory observations reduced the measured traversal
overhead. Diagnostics: `required-input-gate-02.log`,
`source-ownership-reconciliation-01.log`, `source-epoch-history-01.log` and
`cleaner-startup-script-02.log`, all beneath `test-cache/cleanup/`.
No real cleanup, native/gameplay execution or feature completion occurred.

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
- [X] 29.1 Add `src/tests/multilayer/support/package_transfer.py` with `enumerate_preserved_files`, `stage_transfer`, `create_transfer_archive` and `verify_transfer_archive`. Enumerate preserved working files explicitly, including untracked/ignored source/assets/saves and required Git metadata; exclude only audited generated roots.
- [X] 29.2 Detect external Git object stores/alternates, linked-worktree metadata, nested repositories and missing dependencies. Preserve real source repositories; do not silently produce an archive depending on an absent other checkout.
- [X] 29.3 Stage a source-only tree in an allowed temporary location with all retained identities and no generated build/log/cache/runtime/review payloads; preserve authored routing metadata. Keep verification output under that staged project's logs/test-cache.
- [X] 29.4 Verify archive paths/hashes and self-exclusion; produce an external checksum sidecar. Use one small extraction fixture containing an untracked source, authored PNG and retained save; reserve the actual full package check for Group34.
- [X] 29.5 Record exact packaging/exclusion commands and the staged root; protect the live workspace throughout rehearsal.

Result,2026-10-22: the four public `package_transfer.py` exports use the actual
cleaner's exclusions, enumerate ignored/untracked working bytes and local Git
metadata, refuse external/partial object stores and worktree pointers, preserve
local nested source repositories, and verify complete ZIP64 member hashes plus
an external checksum before extraction. Eight focused package checks passed
across affected bursts, including valid PNG/source/save extraction, corrupt or
unsafe members, incomplete Git stores, unchanged export reuse versus a changed
save, and Windows path-length refusal before output creation.

Actual staging used `stage_transfer(Path('C:/Users/Axonn/AppData/Local/cxc-a'))`
(the architecture documents its equivalent CLI). Final result:
13609files/3189027718bytes,6238required paths/3567source payloads and zero
unresolved inputs. The committed export journal verifies14790save aliases,
1653configuration aliases and5221distinct payloads/414836370bytes; journal SHA256
`07c01156a9736a95c3b1d7f1b532add2d6526423a43867eea30cf49275f97080`.
Both original/source-copy hashes and the staged Git stores passed. Stage report:
`C:/Users/Axonn/AppData/Local/cxc-a/test-cache/cleanup/transfer-stage.json`;
source diagnostics: `test-cache/cleanup/package-stage-04.log`.

Real enumeration exposed the tracked `test-cache/.gitignore`: the cleaner now
keeps routing metadata and selects its payload siblings, retaining arbitrary
tracked-file/forced-parent refusals. The affected cleaner fixture passed91checks.
Scope ancestry is validated once per export boundary rather than once per
payload/scope combination; all12save-retention checks passed. Existing exports
are reused only through fresh physical verification. Short Windows roots keep
the original longest retained names within259characters; the incomplete
`cxc-ottd29` attempt is not the selected rehearsal tree. No registry settings,
retained aliases, live source payloads or Git refs were changed. Exact commands
and exclusions remain in the architecture; actual full packaging is still34.
No real cleanup or native/gameplay execution occurred in this group.

Context has compacted 1 times while executing this group.

### Group 30 - Rebuild and qualify the Space checkpoint in the staged tree

{{HARD}}
- [X] 30.1 Run the single shared staged rehearsal for Groups30-32 after migration/helper edits are finished. Start from a missing build through the public recipe interface and reject old-workstation path/cache fallbacks.
- [X] 30.2 Build the capital/ordinary target union once per required configuration in this staged rehearsal. Record actual tool/source/image identities and reuse unchanged images for later selected checks.
- [X] 30.3 Run one load-only/static qualification per configuration for the original save pair, checking all121 families, bounded metadata allowances and observer state. Do not run any old cash/replay tag or repeat qualification after documentation-only edits.
- [X] 30.4 Verify the fresh foundation resolves retained source/saves and current qualification, with one focused altered-time/state refusal check. Carry the qualified result forward while its source/input/image identities stay valid.

Result,2026-10-22: the shared cold rehearsal in
`C:/Users/Axonn/AppData/Local/cxc-a` passed the original sealed Space pair in both
configurations:121families/242comparisons per native load, ordinary static
restore, zero gameplay ticks and unchanged closed scope/floors. The optimized
public command used `space-checkpoint --configuration RelWithDebInfo --run-id
cleanup30-checkpoint-rwd-04 --prebuild-recipe space-paused-viewports
--prebuild-recipe ground-viewport-stations`; Debug used
`space-checkpoint --configuration Debug --run-id cleanup30-checkpoint-debug-01`.
Both used the architecture's explicit vcpkg/grfcodec paths. The optimized
six-target build is
`mlm/reviews/build/selected-native/20261022-182103-8faa06edca/reports/native-build.json`,
SHA256 `dc19225bd03d532fcd101c75e0d25cfc640f34a5f03404e94ec19187bb4aca13`;
Debug's capital/ordinary build is `20261022-182858-8d891c4edf` under the same owner.
Reuse the optimized build through `--build-result` in31?32; later recipes still
run their own native observers. No original build/proof folder supplied runtime
inputs to these completed runs.

Cold failures exposed two provider omissions (root license and CRLF patch
handling), then original DOS graphics/audio held only in the old build folder.
The exact pinned license/source bundle and seven user-provided runtime assets
are now durable fixtures. The portable profile verifies their hashes and rebuilds
the distribution; runtime data leaves the compiler-provider key unchanged.
The three affected runtime/provider/source checks, ten shared-build checks,
existing recipe integration and one altered-clock refusal passed in focused
bursts; no full regression ran. Consumed earlier native/helper epochs and the
initial/licensed profiles remain retained, including
`src/tests/fixtures/cleanup/source-epochs/native-e1f8bd877392911d.json`.

`qualify_checkpoint` and `export_foundation` published `cleanup30-qualified-01`.
The durable foundation, also physically copied and verified in the original
checkout, is
`src/tests/fixtures/checkpoints/space/continuations/3da902a50f240e58982d6f30a66aef7ade78f06fde6618d5aa318a197efa2ceb/reports/foundation.json`,
SHA256 `6d1d5e4a9345f5ba418b64ce5f8b67f6bf4d529674ccd52ccf3103084fae01b7`.
It requires no historical EXE and grants no authorization; next dispatch remains
`Plan11/15CSGZVEEI`. Staged diagnostics are
`test-cache/cleanup/cold-checkpoint-{rwd-04,debug-01}.log` and
`cold-foundation-pair-01.log`; original verification is
`test-cache/cleanup/cold-foundation-original-01.json`, reproducible through
`verify_foundation(load_foundation(<durable-path>))`. The earlier token-limit
interruption is superseded. No paid window, purchase or gameplay tick occurred.

Continue with31, then the one staged clean/rebuild in32, real cleanup33 and
archive34. Refresh source epochs/new save exports before deletion. The incomplete
`cxc-ottd29` stage is not selected. **After packaging, Groups35?37 resume in
`D:/Codez/Sandbox/OpenTTD`, as the user selected, with fresh qualification there.**

Context has compacted 1 times while executing this group.

### Group 31 - Rebuild graphics witnesses in the staged tree

{{HARD}}
- [X] 31.1 Let the selected recipe graph prepare only its required source conversions/packing with publication disabled; verify canonical source/media preservation within that run, not in a second standalone conversion sweep.
- [X] 31.2 Run the native loader needed by the representative graphics selections in one optimized configuration. Do not sweep every family in both configurations; expand only for an actual changed mechanism or configuration-specific failure. Keep independent expectations and actual native provenance.
- [X] 31.3 Capture one representative ordinary Space view and one Ground view in the optimized build using existing bounded producers. Record fixture/process/frame identities; do not replay complete historical matrices or add new acceptance layers.
- [X] 31.4 Use the single staged clean/rebuild in32.3 to prove selected graphics regeneration. Compare the specified bytes/semantics there and retain existing corruption checks; do not add a separate graphics wipe/rebuild cycle.

Result,2026-10-22: `space-paused-viewports`, run `cleanup31-space-rwd-01`,
completed all12nodes using the exact30optimized `--build-result`. Its ordinary
field matched original model18 descending/NW indexed pixels and preserved the
independent owners/clocks with zero ticks. Output:
`mlm/reviews/space/space-paused-viewports/cleanup31-space-rwd-01/reports/current-RelWithDebInfo.json`
(SHA256 `e9eaccd1e04366ed478090d8655a45e3f737b8c329e302a0c5b998250885199a`).
The first capture failed before launch because a cold runtime lacked `scripts/`;
creating its empty private directory and publicly resuming reused all11verified
predecessors and retried only capture. The wrapper now creates that parent itself;
its exact earlier version is retained as
`src/tests/fixtures/cleanup/source-epochs/8319d38bbd721d86-ordinary_art_capture.py`.
This helper is outside the native build's source scope; existing images remain
valid. Use `space-paused-viewports` for the single32.3clean/rebuild so that cold
capture also exercises this correction. No extra capture/test cycle is needed.
Space diagnostics: staged `test-cache/cleanup/cold-space-view-{rwd-01,resume-01}.log`.
Ground selection `ground-viewport-stations`, run `cleanup31-stations-rwd-01`,
prepared all77predecessors and completed the actual stations native observer:
210frames/2580assertions, unchanged owners, exact independent source pixels and
no paid progress. Its typed result is
`mlm/reviews/selected-ground/stations/cleanup31-stations-rwd-01/reports/current-RelWithDebInfo.json`,
SHA256 `f0760bc1fc3f36df5529258994f48a8428974be4b653417c90335f2920f53e3f`.
The outer recipe timed out during final read-only provenance verification;
its graph remains failed (77/78), while the existing `checked_result` verifier
independently passed the completed native result. No stage status was rewritten
and no native recapture was needed. Diagnostics: staged
`test-cache/cleanup/cold-ground-stations-{rwd-01,resume-01,verify-01}.log`.

Both capture wrappers now create their private script directory. Native verify
stages have a bounded300-second supervisor budget for execution plus provenance;
pure/preparation stages retain120seconds and native subprocess caps are unchanged.
Consumed wrapper/executor bytes and the successful native inventory
`source-epochs/native-33ba6c075f273963.json` were retained before edits. The executor
change requires a new actual incremental build report before32.1; it does not
retroactively certify old results against changed source. All10existing focused
executor checks passed (3.478s), diagnostics:
`test-cache/cleanup/recipe-executor-31-01.log`. The prescribed single32.3clean/rebuild
passed and closes31.4. No gameplay tick or purchase occurred.

### Group 32 - Rehearse continuation recovery and full staged cleanup

{{MEDIUM}}
- [X] 32.1 Exercise one representative prior-stage state/TBM recipe with absent logs and freshly prepared inputs in the same staged rehearsal. Preserve existing refusal meanings and explicit prior-run identity.
- [X] 32.2 Verify underground source/baseline/art material is present and distinguish its provisional feature from completed terminal obligations. Do not implement the five-model task to make cleanup look complete.
- [X] 32.3 Apply the upgraded cleaner once to the staged project, verify all save exports and generated-payload removal (keeping source routing metadata), then rebuild one selected graphics recipe. This same result closes31.4; no second staged clean/rebuild is required.
- [X] 32.4 Record the shared rehearsal commands/results, limitations and footprint once for Groups30-32, and close real-deletion gates. Do not claim another machine was tested or repeat passed staged checks without a relevant change.

Result,2026-10-22: the shared rehearsal ran only in
`C:/Users/Axonn/AppData/Local/cxc-a` on this workstation. Public
`state-prior-native --configuration RelWithDebInfo --run-id cleanup32-prior-rwd-02`
passed a fresh reload of the retained original371-format save using explicit
`--build-result mlm/reviews/build/selected-native/20261022-204653-bba6d3122f/reports/native-build.json`.
No old chain or absent393/394 pacing semantics were inferred. Public
`tube-checkpoint --run-id cleanup32-tube-history-01` verified both distinct source
epochs, provisional sources and completed terminal history; five-model/current
native/useful-route/human gates remain open.

Cold preparation now resolves migrated `grftest.cfg` through its retained
catalogue identity and creates private scripts directories. The first cleaner
preflight refused without deleting anything:274older stable keys still selected
generated review paths despite already retained saves. Migration now redirects
those aliases without changing hashes, and the required-input gate refuses
physical dependencies inside selected disposable scopes even when present.
Seven focused preservation checks passed (0.337s); the two affected cases also
passed after observation deduplication. Existing peer isolation and retained-byte
corruption checks passed. Previous helper/test/catalogue/native inventories were
retained before edits. Original diagnostics:
`test-cache/cleanup/portable-inputs-32-{01,02}.log`,
`cold-runtime-paths-32-{02,03}.log` and `preclean-source-32-01.log`.

Before deletion,3015new source aliases were classified and6323distinct source
payloads verified. Their new immutable files and every exported save/configuration
also survive in the original checkout; the journal contains5230payloads/16489aliases.
The actual staged `clean.ps1 -Apply -ShowPaths` removed all10targets with zero
failures:8.67GiB file lengths/8.47GiB estimated distinct freed. All51current
save/configuration candidates passed preservation. The next public
`clean.ps1 -DryRun` reported `Nothing to clean`; logs/build/runtime and generated
reviews were absent, with routing `.gitignore`/`mlm/reviews/README.md` preserved.

The single post-clean public `space-paused-viewports --configuration RelWithDebInfo
--run-id cleanup32-space-regenerated-01` with the documented explicit tool paths
then passed all12nodes. It rebuilt both selected targets from missing build and
pinned dependencies. Build:
`mlm/reviews/build/selected-native/20261022-210937-87966a6d7e/reports/native-build.json`,
SHA256 `171005fe866e48f8be52a440f58b7bf3ba70a0223bbfb53b85088899448d8b12`.
Rebuild root SHA256:
`a4f9b8cc24478dfe67c23bbdac9f3742cd976088019d5083157af3478253f510`.
Actual field result:
`mlm/reviews/space/space-paused-viewports/cleanup32-space-regenerated-01/reports/current-RelWithDebInfo.json`,
SHA256 `724debf251665c0331e147065350c9fef3953aa17dce90325be3a5d71c56ed7d`.

Pre/post PNG/NFO/header bytes matched exactly:
`9cc86a28374e0cdfcb1d32cb758956bd53e05dc44cec4d7b407bc6b208f7ef96`,
`2b54ca81a26eaccfb2ad4f4460f1624d58a45dd78a5d72499e5d59b006885284`,
`aaf332cf387c23bb74ac9f19a6373b2fde7d6ae95391ac4468d93e4e005a08a9`.
Field indexed pixels also matched
`e88ea1f7715d8df55c5c3da292cf96c760307f5386e4182c89fb893fac68e56d`,
with the independent model18 descending/NW source match
`b4e1f7cc4b3b9d1de654a7beebd73a8ee1b5266136e7b27f75e67bb92698c179`.
All36checked owners were unchanged, simulation ticks stayed zero and canonical
GRF remained `c04fbf0d5525edc53250cfbd963f5e3bbe0d0bce655a3553c73b76483a8e8744`.
These are regeneration observations; original independent expectations were
unchanged. This closes31.4/32.3 without another cleanup/capture cycle.
Staged diagnostics: `test-cache/cleanup/cold-space-regenerated-32-01.log`.
No other machine, original-workstation build fallback, source publication, paid
window or gameplay progression was tested/authorized. The rehearsal closes the
tooling gate;33still requires fresh preservation/preview/apply checks in the real
checkout. Groups35?37 must resume in `D:/Codez/Sandbox/OpenTTD` after packaging,
with fresh qualification there, as the user selected.

### Area Mandatory Reading - Real cleanup and transfer handover

_Applies to Groups 33-34._

Entry gate: Groups30-32 passed and the real live checkpoint has not advanced beyond the seal. Recheck the handover, inventory and current writer state. Stop real deletion if they changed; export/qualify the newer boundary first. A staged success cannot authorize deleting newly created source or saves.

| File name | Line Ranges | Description |
| --- | --- | --- |
| [Space current boundary](r2mlsp-z-z-z-z-z-q-launch-center-source.md#current-boundary) | Live Group23 recovery section; no fixed range | Identifies the currently selected durable checkpoint and next Space task. Reconcile concurrent progress before deletion. |
| [Cleaner](../../../../clean.ps1) | Live Group26 entry point and Group24 export adapter; no fixed range | Execute only the verified preview/apply behavior; no separate ad hoc recursive delete. |
| [Preservation owner](../../../../src/tests/multilayer/support/artifact_preservation.py) | `cleanup_operation` verify narrowing and `save_retention.verify_save_exports` | For33.5, select canonical relative aliases without resolving retired owners; fresh physical/hash checks remain with the selected observer. |
| [Transfer helper](../../../../src/tests/multilayer/support/package_transfer.py) | Live Group29 `enumerate_preserved_files/stage_transfer/create_transfer_archive/verify_transfer_archive`; no fixed range | These public exports package all durable working material with explicit exclusions and an external checksum sidecar; verify actual exports rather than assuming an archive tool exists. |

### Group 33 - Apply verified cleanup to the real repository

{{SIMPLE}}
- [X] 33.1 Revalidate the actual current checkpoint and source/save inventory against the live handover. Check relevant writers are stopped; never terminate unrelated processes.
- [X] 33.2 Run the upgraded explicit preview and verify save export/retained inputs, then apply its audited targets with all gates satisfied.
- [X] 33.3 Check source/media/Git/fixture/save preservation and the absence of generated logs/cache/runtime/build/review payloads. No disguised output archive may have been moved elsewhere.
- [X] 33.4 Reapply to prove idempotence and record actual deleted totals/remaining footprint/partial failures. Keep any real failure open rather than marking cleanup complete.

- [X] 33.5 Remove repeated filesystem resolution of foreign/retired aliases from per-target verification in the existing preservation owner. Keep fresh selected-scope observation/hash/refusal checks and add one focused removed-foreign/changed-selected/new-selected boundary check before packaging.

Result for33.1?33.4,2026-10-23: the real checkout's original seal and both durable
foundations verified unchanged; no relevant writer was running. Fresh source
classification retained one additional alias,6324distinct payloads/9443candidates.
The public `Invoke-DevelopmentCleanup -Repository D:/Codez/Sandbox/OpenTTD`
pipeline passed explicit `-DryRun`, then `-Apply -ShowPaths` with16443protected
candidates and zero required-input refusals. All64targets were removed with zero
failures:59128115119bytes (55.07GiB) logical/estimated distinct freed. No output
archive was moved elsewhere.

An in-memory `enumerate_preserved_files` baseline compared every pre-existing
preserved file across deletion. All17320non-journal files were byte-identical,
including source/media/Git/fixtures/saves; the committed export journal verified
all5230payloads/16489aliases. The remaining tree was17321files/3251876195bytes,
with only routing metadata in generated owners. Explicit `-Apply` again reported
`Nothing to clean`. No Git write, unrelated process termination, purchase or
simulation progression occurred. The source tree stayed frozen through this
comparison.33.5addresses observed repeated whole-alias path resolution in
`cleanup_operation`'s per-target narrowing; this is a bounded performance fix,
not another real cleanup or native qualification cycle.

33.5also passed,2026-10-23: per-target selection now compares canonical
relative paths lexically and indexes observations once; it never resolves retired
foreign aliases. The unchanged selected observer still checks fresh paths,
redirects, changed/new candidates, retained hashes and committed ownership.
Eight focused `artifact_preservation_check.py` cases passed (0.577s), including
removed/similarly prefixed foreign owners, changed/new selected saves and duplicate
measurements. Function/class ownership and argument documentation passed static
inspection. Both consumed helper/test versions were retained before editing.
The public cleaner removed the sole new test-log target (1.47KiB), zero refusals
or failures; a final `-Apply` again reported `Nothing to clean`. No real cleanup
or native qualification matrix was replayed. Group33is complete.

Context has compacted 1 times while executing this group.

### Group 34 - Package the clean project and finish its recovery handover

{{SIMPLE}}
- [X] 34.1 Finalize the architecture runbook and Space/underground handovers with tested commands, retained hashes, tool versions, last results and next task before creating the final archive.
- [X] 34.2 Leave the removable Space resume note: it can be deleted after the checkpoint is recovered. Update predecessor cleanup follow-ups and all active Space location/practice links.
- [X] 34.3 Create the archive in the allowed temporary area, outside its payload. Verify members/hashes and extraction; include all valuable untracked/ignored working files and required Git metadata.
- [X] 34.4 Record actual tree/archive sizes and checksum. Refresh the archive only if its payload changed; avoid further discretionary edits after packaging and preserve indispensable inputs regardless of size target.
- [X] 34.5 Mark the transfer-ready milestone only when its checks pass. Keep the final source tree clean for transfer; Groups35-37 are executed in the chosen development checkout when work resumes.

Result,2026-10-23: architecture, active Space master/handover and Goods-capacity
Local, paused Tube handover and all three cleanup predecessor follow-ups now
record delivered recovery/cleanup and the user-selected original development
checkout. The removable `r2fc2-file-cleanup3-a-space-resume.md` was delivered and
subsequently deleted after35recovered the checkpoint. Eighty-four current file/heading links passed;
read-only checkpoint `--inspect` verified the enabled registered graph. Dated
historical log/EXE paths remain facts, not current recovery dependencies.

Actual source packaging through public `create_transfer_archive` and
`verify_transfer_archive` passed complete member/path/hash/self-exclusion and
external-checksum verification, then extraction into
`C:/Users/Axonn/AppData/Local/cxc-b` with local Git stores verified. Initial package:
`C:/Users/Axonn/AppData/Local/cxc-a/test-cache/cleanup/openttd-transfer-20261023-01.zip`,
17324files/3251921289payload bytes,2684488422archive bytes, SHA256
`45441a97523638f8746b0d6875334e3b771cf8e979ac6bef4a78fd1432c89ca9`.
It verified5964distinct required paths/6324source payloads with zero unresolved
inputs. Source, assets, all saved versions, ignored/untracked working bytes and
required Git metadata were included. No old executable/proof graph was restored.
Only this Windows workstation/source-transfer extraction was tested.

Closing checkboxes/results change metadata after that validated snapshot. The
final named refresh `openttd-transfer-20261023-02.zip` uses the same public
interface and fresh short extraction root `C:/Users/Axonn/AppData/Local/cxc-c`.
Its exact archive/sidecar/SHA256, source/archive byte totals and extraction result
are recorded outside the payload in
`C:/Users/Axonn/AppData/Local/cxc-a/test-cache/cleanup/transfer-20261023-current.json`.
That optional observation record selects the latest verified ZIP; verification
requires the ZIP plus its external `.sha256`, not the observation report. This
avoids rewriting a source file to contain its own archive hash. No unchanged
payload needs another archive. Staged diagnostics are
`test-cache/cleanup/package-34-{01,02}.log`.

Final refresh passed,2026-10-23:02contains17324files/3251922674payload bytes,
2684488963archive bytes (2.50GiB), SHA256
`adbaf0b78f160f92aa8f7df48bee08dc30c002501c495747ba863ad4ed45f030`.
Fresh extraction into `cxc-c` and its local Git checks passed. Compressed contents
are dominated by Git history (1621MiB), authored media (723MiB) and retained
saves (119MiB); the55.07GiBof deleted generated outputs are excluded. The
compaction note was deferred until the frozen archive source had finished
verification. Later Group35–37 development changes follow this transfer boundary
and do not require repackaging that completed milestone.

Transfer readiness is separate from development acceptance. The clean original
checkout `D:/Codez/Sandbox/OpenTTD` is the user's chosen35?37destination. Fresh
qualification there precedes actual GZVEEI implementation; the staged proof and
package do not implement Goods capacity, authorize033 or grant paid progress.

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
- [X] 35.1 Verify retained source/assets/saves and tools in the chosen checkout. Run one checkpoint qualification per configuration there; reuse Group30 only if it is this exact checkout with unchanged source/tool/save/image identities. Never reuse a staged proof across a different path or machine.
- [X] 35.2 Recover the exact pair/current state/closed policies and read the live Space dispatch plus its relevant Global/Local sections. A newer explicit checkpoint wins over this plan's dated GZVEEI snapshot.
- [X] 35.3 Prepare only missing assets/build targets required by the actual next task through existing recipes. Reuse valid selected prerequisites; do not trigger a broad graphics or historical acceptance sweep.
- [X] 35.4 Record recovery complete and the source-plan task to execute next. The removable checkpoint-recovery note may now be deleted; feature implementation has its own following completion checks.

Result,2026-10-23: recovered `D:/Codez/Sandbox/OpenTTD` through the public
`space-checkpoint` recipe, sequential runs `cleanup35-checkpoint-rwd-01` and
`cleanup35-checkpoint-debug-01`. Both freshly built only `openttd` and
`openttd_surface_capital_test` using pinned tools/dependencies, two workers and
no owned node reuse. Native load plus ordinary restore passed in each
configuration: all121families preserved, zero gameplay ticks, original seal,
cash182302/debt5000000and closed policies unchanged. No staged proof was reused.

Actual build reports are `mlm/reviews/build/selected-native/20261023-001408-f3203180a5/reports/native-build.json`
and `20261023-003912-742f6b930b/reports/native-build.json` under the same owner.
Paired publication `cleanup35-qualified-01` exported the selected durable
foundation to
`src/tests/fixtures/checkpoints/space/continuations/8919e87faaec30bfaacee97e55dece2d54c3181eeff173bde90c9f0da4427c1d/reports/foundation.json`,
SHA256 `ed75dbf2582e5b9b4b55b9d07105918daa282f9ab9f14b61f3e3988c004c764e`.
Group30's foundation remains historical. Diagnostics can be rebuilt through the
architecture recipe and live in `test-cache/cleanup/cleanup35-checkpoint-*.log`;
required transcripts remain in each review's `evidence/`. The removable transfer
note is deleted and active links point to the shared recovery owner. Relevant
Plan11Global/Presentation/AD-12/Goods Local and actual command/route/cost owners
were read. Next is still Plan11GZVEEI, unimplemented at this boundary; no paid
window, purchase, stock change or historical acceptance sweep occurred.
Sixty active recovery file links and the focused documentation whitespace check
passed; no reference still links to the deleted removable note.

### Group 36 - Execute the next bounded Space task through the new foundation

{{HARD}}
- [X] 36.1 Dispatch the live source-plan task under its own numbering, rather than duplicating gameplay authority here. If it is still Plan11GZVEEI, create its separately selected pure Goods proposal using the qualified checkpoint argument and existing native command/route/cost owners.
- [X] 36.2 For GZVEEI, read actual station13 waiting Goods/station15 acceptance and packet origins, original49/mixed52 capacity, depot1638 connectivity and platform fit. Quote a currently available callback-free head plus2/4/8/12Goods-wagon lots; include their first full operating interval, every existing owner cost and100,000capital floor. Preserve actual availability/refusals.
- [X] 36.3 After the cohesive query edits, compile the isolated target and check the unchanged ordinary target up to date in the same current build receipt, then qualify/query/reload once per required configuration. The existing receipt contract binds the full current source inventory to both actual images; no ordinary game source edit or broad rebuild is required. Batch the proposal lots in that bounded run. Require zero gameplay ticks/all121 families and unchanged body/material/finance observations; no cash-work033, purchases or old cash-specific cases.
- [X] 36.4 Bind the actual proposal/refusal through the new successor contract and complete only the corresponding source-plan tasks whose evidence passed. Preserve closed financial scopes; the next paid producer remains subject to its own full native admission and source-plan requirements.

At this snapshot the query can reuse the existing estimate adapter:

```cpp
const auto quote = QuoteSurfaceResupply<Commands::BuildVehicle>(
    depot, depot, candidate, false, goods, ClientID::Server);
```

This fragment obtains one native body quote. The actual task separately proves model/callback eligibility, route/platform fit and the complete proposed-consist/company invoice. `CloneVehicle` carries `CommandFlag::NoTest` in `vehicle_cmd.h`; it cannot replace a reliable whole-purchase estimate.

Result,2026-10-23: Plan11GZVEEIis complete. Registered producer
`space_goods_capacity.py` and native `[.space-goods-capacity]` use the explicit
Group35foundation and native command/follower/full-company cost owners. Actual
source bytes and Group35's consumed inventory were retained before edits.
Version-two operations add exact ordinary result/process/terminal/save witnesses
to the existing successor owner; version-one completed operations remain valid.
Native wagon calendar age is distinguished from the powered head's useful-life
limit, matching `CmdBuildRailWagon`/`AgeVehicle`.

Thirteen focused business/reload/backward-compatibility checks passed. Reload
checks are split into two files (90.0s/60.9s); every runner stayed below two
minutes. Syntax/ownership/argument/business-comment checks passed. Sequential
incremental builds used two workers/no node reuse and only capital/ordinary
targets; their actual query plus ordinary reload passed in both configurations:
4processes, zero gameplay ticks, all121families and original owner/material/
finance observations unchanged. The shared generated content object refreshed;
no ordinary game source edit or broad native matrix occurred.

Runs: `cleanup36-goods-rwd-01` and `cleanup36-goods-debug-01`. All four lots fit
7tileplatforms and are financially funded. The2wagon invoice is37382and full
target147602against182302cash, including100000capital floor/10000existing first-
interval costs/220new upkeep. Actual13stock is only2Goods from packet35/Industry5;
52has50empty Goods spaces on15/13. Original49's300capacity/93held serves12/11.
Affordability alone does not establish useful additional service. The exact next
prerequisite is Plan11GZVEEJ's read-only incremental-usefulness gate; no purchase
or further cash window is approved.

Public paired successor `cleanup36-goods-paired-01` passed publication at
`mlm/reviews/space/goods-capacity-publication/cleanup36-goods-paired-01/reports/foundation.json`,
SHA256 `5d8ca01b552778970bde977259f25037e935d3c87de779803e221280d398252c`.
Optional rebuildable diagnostics are `test-cache/cleanup/cleanup36-goods-*.log`
and `goods36-*.log`; required native/reload transcripts stay in review `evidence/`.
Actual proposals and model availability/refusals are bound in the copied native
terminal. Group37exports this selected closure before later cleaning. Original
15.1/.2, useful-route and human acceptance remain open.

### Group 37 - Publish the next checkpoint and continue the Space queue

{{MEDIUM}}
- [X] 37.1 Export the selected task's actual successor save/contract and source epoch through the established checkpoint/save-retention helpers. Preserve original and successor identities; generated process/review output remains cleanable.
- [X] 37.2 Update the Space master/child handover with real results and the next unfinished task. Mark development resumed only after actual source-plan work, not from archive extraction or a successful static load.
- [X] 37.3 Perform one read-only check that the exported continuation resolves durable inputs without EXE retention. Reuse the successful Group36 execution evidence; no extra native rebuild/reload is required unless the checkpoint or relevant code changed. Keep underground work paused.

Result,2026-10-23: exported the actual paired query successor through
`space_checkpoint.export_foundation` to
`src/tests/fixtures/checkpoints/space/continuations/5d8ca01b552778970bde977259f25037e935d3c87de779803e221280d398252c/reports/foundation.json`,
SHA256 `8cc7f8743c2922537d49b1745641049692a3c297a1e1d72208526748c76a1f6e`.
Original seal/qualified predecessor and all source/save identities remain
preserved; generated reports/runtime/images stay cleanable. Both consumed native
inventories share retained `source-epochs/native-0888c1850056a966.json`, SHA256
`0888c1850056a966111c52afe7bef39d9c9ab2bd83f9a3882670a06268d454ec`.
The public save-retention helper verified16save/8configuration aliases across
7payloads/200816bytes in the nine exact selected review/runtime owners, without
deletion. The supplemental journal now has5233payloads/16513aliases; original
migration/seal contracts are unchanged.

Architecture, current Space master/Plan11/handover, predecessor follow-ups and
paused Tube handover now record actual resumed development and the selected
successor. GZVEEI's three tasks are checked; new GZVEEJ's three tasks remain open
for the exact usefulness prerequisite: only2Goods wait at13while mixed52has50
empty spaces. The capital quote is affordable, not permission to buy. Original
15.1/.2, human/useful-route and provisional five-model Tube gates remain open.

One actual read-only verification traversed131distinct durable/current-source
inputs, with build/review/runtime/log/cache and all EXE/DLL reads blocked,
all writes/process launches blocked. Both configurations verified, zero forbidden
reads/writes/launches, `historical_images_required=false` and next dispatch
`Plan11/15CSGZVEEJ`. No extra native execution, rebuild or regression was needed.
The public `load_foundation/verify_foundation` APIs above reproduce continuation
verification; optional diagnostics/retention observations are in
`test-cache/cleanup/continuation-37-{current,save-source,readonly}.json` and
`export-37.log`. They are not recovery dependencies. Group34's archive remains
the verified transfer snapshot before this explicitly resumed development;
no unchanged milestone was repackaged. No Git write or paid gameplay occurred.

Final focused closure passed:80active file/heading links, authored-source
whitespace and tracked-change whitespace checks. All79continuation tasks are
checked. No full regression or additional native cycle was run for documentation.

Transfer-ready completion is Group34. Development-resumed completion requires Groups35-37 in the chosen checkout. If transfer has not yet occurred, report those latter tasks pending rather than claiming remote execution.
