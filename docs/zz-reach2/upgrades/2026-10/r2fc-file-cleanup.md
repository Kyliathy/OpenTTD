# R2FC - Repository files, test fixtures and review output cleanup

## Global Mandatory Reading

This cleanup changes artifact locations and test output routing, not gameplay, save formats, native sprite identities or the meaning of retained acceptance evidence. Preserve the existing working-tree changes and immutable inputs. Git operations remain read-only. Keep review evidence in `mlm/reviews/`, disposable native test/demo data homes in `mlm/runtime/`, and captured logs in `logs/`; keep Markdown outside source directories. `/mlm/` is a repository-local multi-layer development workspace, not a replacement for OpenTTD's general data-directory policy.

The architecture spine below establishes one asset authority, content-bound verification, and repository-local artifact ownership. Old path literals are migration inputs, not a requirement to preserve the docs layout. Read the task's edited symbols after these contracts; do not preload whole implementation files.

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
| [Graphics pipeline](r2mlvs13-graphics-and-asset-pipeline.md#public-handoff-contract) | 83-87 | `Public Handoff Contract` and `Sprite Contract Handoff`: one owner for asset identity, source, geometry and consumer fields. Globally constrains relocation to move the existing public manifest without creating a second catalogue; live validators supersede the historical proposed-status wording. |
| [activate_m1_art.py](../../../../media/multilayer/activate_m1_art.py) | 105-153 | `validate(public, packed, rasters)` binds packed bytes, source saves, frames and native evidence to hashes and actual consumers. Every group must preserve this chain while changing paths; the old directory name is not a validation invariant. |
| [multilayer_state_fixture.py](../../../../src/tests/multilayer_state_fixture.py) | 20-61 | `log_path` and `save_path` establish repository-local logs and feature-owned save routing. Every group must preserve containment and retained inputs while replacing mixed input/output destinations; generated artifacts must not fall back into docs or immutable fixtures. |
| [R2MLVS parent](r2mlvs-multilayer-vehicles-and-structures.md) | 7-9 | The parent owns shared decisions and its artifact-placement paragraph directs children to fixtures and docs assets/reviews. Every group must follow the replacement `Development artifact layout` contract published in Group 2, rather than leaving conflicting parent instructions or creating a private layout policy. |

## Planning status

Status: **execution complete, 2026-10-09**. Groups 1-11 are complete. Final scope and file reduction: [commit-selection handoff](r2fc-j-commit-handoff.md). Git operations remain read-only. Evidence and continuation notes: [execution record](r2fc-a-execution-notes.md).

Execution-standard revision: 2026-10-09. Tests must group screenshots by feature and document their producers. The original audit snapshot below is retained; it is not a new status measurement.

## Requirements and scope

- Keep `docs/zz-reach2/` for documentation. Move executable scripts, configuration fixtures, machine-readable asset contracts and binary assets to their appropriate owners.
- Relocate the contents and runtime responsibility of **all eleven directories named by the user**, including empty descendants. Verify the original directories are absent after migration and remain absent after native runs; Git status cannot prove that empty directories have gone.
- Give supporting documents, tests and generated evidence purpose-named parent directories. Modify native tests and wrappers to select screenshot paths by feature at creation time. Moving existing files or adding filename prefixes alone does not satisfy this requirement.
- Use repository-root `mlm/reviews/` for multi-layer screenshots, galleries, reports, intermediate saves and review artifacts, grouped by feature, scenario and run. This proposed destination supersedes the earlier root `reviews/` target following the user's request to group multi-layer infrastructure under `/mlm/`. Maintain a producer-to-feature catalogue and run provenance so each screenshot can be traced to its test, native writer and input.
- Keep reusable test programs and the immutable inputs they need under `src/tests/`. A Python file or PNG can belong in a commit; its role and dependencies decide, rather than its extension.
- Put source artwork and asset tooling with the existing root-level `media/` tree. The proposed owner is `media/multilayer/`, rather than introducing a second generic root `assets/` directory alongside it.
- Preserve current gameplay work, supplied saves, historical compatibility fixtures and evidence needed by validators. This is a layout and output-routing change, with focused verification of the affected code.
- **Commit-noise reduction (user instruction, 2026-10-09):** reproducible validation proof files, capture/report products and automatically rebuilt intermediates belong under ignored `logs/proofs/` (visual run output remains feature-grouped under `mlm/reviews/`). Keep only irreproducible sources, meaningful immutable regression inputs and genuinely required distribution artifacts tracked. Replace dependence on committed generated proof with a verified regeneration path. Report the reduction in existing, eligible file payloads and new generated additions; account for one-time renames/deletions separately from recurring output.
- **Post-demo selective deletion (user decision,2026-10-09):** [master task12.6](r2mlvs3mp-master-plan.md#group-12---close-both-target-plans-with-evidence) owns pruning selected fixtures after the user verifies the demo and its required feedback is resolved. Inventory and relocation preserve inputs until then. Keep fixtures with distinct coverage and favor reusable inputs covering several meaningful behaviors; preserve specific compatibility, malformed/failure, supplied-save, demo and validator inputs where needed. Only duplicate or superseded outputs adding no coverage are removal candidates, with consumer references repaired before deletion. No fixture deletion is authorized by this planning update.
- The subsequent execution request authorizes this implementation. Git operations remain read-only: no staging, commit, reset, checkout, stash, tag or other Git writes. The commit-preparation tasks below produce a reviewable file selection for a later commit action.

This plan replaces the old artifact-placement conventions when implemented. In particular, the TBM plan and master currently direct assisting scripts beside Markdown, saves into fixture directories, and captures into `assets/reviews/`; leaving those instructions in place would recreate the clutter. Their gameplay requirements and completion records remain authoritative.

## Audit snapshot

Measured before editing this document using `git status --porcelain=v1 --untracked-files=all`, `git ls-files`, filesystem counts and source inspection. Counts describe this working tree on 2026-10-09, not a permanent baseline.

| Working-tree category | Count | Implication |
| --- | ---: | --- |
| Modified tracked files | 139 | Includes genuine source/test changes, documentation and distributed art. |
| Untracked files | 670 | Needs classification before any broad commit selection. |
| Staged files | 0 | Nothing is currently staged. |
| Untracked documentation assets | 314 | 312 PNGs and 2 JSON files; includes new source artwork and required proof inputs as well as run output. |
| Untracked haulage saves | 291 | Generated checkpoints are mixed with potential permanent regression/demo inputs. |
| Untracked Python scripts beside October plans | 13 | TBM/geology runners, currently named after plan checkpoints. |
| Other untracked files | 52 | 20 Markdown documents, including this initially empty plan; 32 source/test/media files elsewhere. |

The existing clutter also includes already committed files:

| Location | Files on disk | Tracked files | Observation |
| --- | ---: | ---: | --- |
| `docs/zz-reach2/upgrades/2026-10/assets/` | 955 | 559 | About 290.9 MiB; contains source art, contracts, guides, reviews and scratch output. |
| Its `mlm/reviews/` child | 736 | 343 | 642 PNGs, 11 JSON reports, 1 HTML gallery and 82 ignored PPM intermediates; about 169.5 MiB. This row is a subset of the preceding row. |
| `src/tests/fixtures/haulage/` | 535 | 244 | All `.sav`; about 6.1 MiB. Being in a fixture directory does not establish that every file is a required input. |
| Repository-root `reviews/` | 0 | 0 | Exists and is empty. Current capture producers do not use it; the updated target is `mlm/reviews/`. |

Across `docs/zz-reach2/` there are 36 Python scripts: 25 directly beside the October plans and 11 under `tools/`. There are also two PowerShell scripts, two shell scripts, one test configuration, three text reports, and **three tracked Python bytecode files** under `tools/__pycache__/`. The current ignore rules already cover bytecode, but do not remove previously tracked files.

### Directory follow-up and origin of the old layout

The follow-up filesystem audit found nine empty trees: `ai/`, `baseset/`, `content_download/`, `game/`, `newgrf/`, `save/`, `scenario/`, `screenshot/` and `social_integration/`. Descendants include `ai/library/`, `game/library/`, `save/autosave/`, `scenario/heightmap/` and downloaded-content mirrors. `assets/` has 955 files; `tools/` has 23, including five local bytecode files, three tracked. Root `mlm/` does not yet exist. Refresh this inventory before implementation; currently empty directories may acquire content.

The reference scan followed 88 in-scope parent, numbered-child and companion documents reachable from R2MLVS/R2MLVS3MP, then searched earlier R2MLM/review plans for the same conventions. It distinguishes intentional organization from an accidental runtime side effect:

| Source | Decision or observation | Consequence |
| --- | --- | --- |
| [R2MLVS parent](r2mlvs-multilayer-vehicles-and-structures.md), artifact-placement paragraph, line 9 | Saves belong beside tests, but screenshots/review boards belong in `assets/reviews/`, and source art/previews in docs assets. The stated aim is to keep binaries out of the planning directory itself. | Change this shared rule so future children do not recreate the old layout. |
| [R2MLVS3MP](r2mlvs3mp-master-plan.md), placement/log paragraphs, 39-43; [Plan 13](r2mlvs13-graphics-and-asset-pipeline.md), execution protocol and public handoff | Assisting scripts stay beside plans; the public graphics manifest lives in docs assets. Plan 13 also retains the scratch-script convention. | Reconcile normative instructions and public handoffs, not only file links. |
| [Industry lifecycle notes](r2mlvs6-a-industry-lifecycle.md), startup correction, 666-675 | A diagnostic config beside the plan **incorrectly** redirected working/config lookup there and caused language popups. It was removed; probes then used `build/play.cfg`. | A docs runtime home is a documented failure, not a gameplay requirement. This record does not identify the exact launch that created every surviving directory. |
| [Native art notes](portal-tbm/r2mlvs3mp-r-runtime-art-notes.md), Windows capture paragraph, line 15; [packed-art fixture producer](../../../../src/tests/multilayer_packed_art_fixture.py), `main` | Native use moved to `build/native-art.cfg` because Windows baseset lookup follows the config directory. Current code copies the docs template into `build/` first. | Preserve content/language discovery when moving the runtime home; moving only the template or shell working directory is insufficient. |
| [Windows path setup](../../../../src/os/windows/win32.cpp), `DetermineBasePaths`; [native path setup](../../../../src/fileio.cpp), `DeterminePaths` | `-c` selects the configuration directory as a search root. Native startup creates standard save/scenario/content/script/screenshot trees in its selected data home. `-x` suppresses config saving, not directory creation. | Fix the launch/configuration root. Do not hard-code MLM paths into general OpenTTD behavior. |

These records explain why assets/helpers were co-located with plans and how an ordinary OpenTTD data tree could appear there. The exact creating invocation for the remaining empty trees is unproven; preserve that uncertainty. The replacement contract distinguishes immutable templates, writable runtime homes, curated fixtures, source media and generated review evidence.

### Complete directory relocation map

Source paths below are relative to `docs/zz-reach2/upgrades/2026-10/`. `runtime` means repository `mlm/runtime/<profile>/<run-id>/<process>/`; host/client processes receive separate homes. The standard directory names are generic OpenTTD names, but these instances belong to multi-layer development sessions. Keep engine directory semantics intact.

| Directory to retire from docs | State | New owner and handling |
| --- | --- | --- |
| `ai/` | Empty, including `library/` | `runtime/ai/` and `runtime/ai/library/`. Preserve any content discovered during refreshed inventory; reusable script sources/fixtures remain with their test owner. |
| `assets/` | 955 files | Source art/contracts/palettes/guides to `media/multilayer/`; reproducible proof inputs to ignored `logs/proofs/mlm/native-art/`; generated evidence to `mlm/reviews/<feature>/<scenario>/<run-id>/`; explanatory Markdown to named docs families. Classify every file before retiring the entire old tree. |
| `baseset/` | Empty | `runtime/baseset/` as a local projection of required distribution content. `media/baseset/` and existing build outputs retain their source/distribution roles. |
| `content_download/` | Empty, nine descendant directories | `runtime/content_download/` with its standard content/library/heightmap mirrors. Preserve cache files if found later; relocation requires no new downloads. |
| `game/` | Empty, including `library/` | `runtime/game/` and `runtime/game/library/` for GameScript discovery; reusable sources/fixtures remain under their code/test owners. |
| `newgrf/` | Empty | `runtime/newgrf/` for the selected run's required content; existing media definitions/build outputs remain authoritative. |
| `save/` | Empty, including `autosave/` | `runtime/save/` and `runtime/save/autosave/` for native transients. Retained checkpoints go to feature-run `mlm/reviews/.../saves/` or explicitly curated fixtures. Relocation does not prune supplied/demo inputs. |
| `scenario/` | Empty, including `heightmap/` | `runtime/scenario/` and `runtime/scenario/heightmap/`; immutable scenario/heightmap inputs stay in purpose-named fixture directories. |
| `screenshot/` | Empty | Engine placeholder at `runtime/screenshot/`; review captures at `mlm/reviews/<feature>/<scenario>/<run-id>/images/`. If an engine screenshot command requires its native folder, collect and catalogue its output in the feature directory within the same run. |
| `social_integration/` | Empty | `runtime/social_integration/`; moving this directory does not enable/install integrations. |
| `tools/` | 23 files | Test runners to `src/tests/multilayer/`; art helpers to `media/multilayer/tools/`; multi-layer inspection/launch/ledger helpers to `mlm/tools/`; retained text to docs/fixtures; generated output to `mlm/reviews/`; disposable bytecode leaves the tracked layout. |

Root `reviews/` is also empty. Adopt `mlm/reviews/` for this scope, classify any content subsequently added to root `reviews/`, and retire that old empty directory when no consumer needs it. Do not move unrelated review workflows into MLM automatically. Empty source directories require no content copy: redirect their creators, verify the new data home, then remove the old empty trees without deleting retained fixtures.

### Shared decision and affected-plan reconciliation

Group 2 publishes one **Development artifact layout** contract in the R2MLVS parent, explaining the old rationale, documented runtime mistake, replacement locations and migration status. R2MLVS3MP consumes it and links this cleanup as its implementation owner. This is multi-layer test/demo infrastructure; ordinary OpenTTD/Classic user paths and existing media/build/test authorities remain unchanged.

Audit **all 17 numbered R2MLVS children**, linked execution/companion records, TBM fixing plans and earlier R2MLM/R2RVM documents supplying active commands or placement rules. Record each as affected or checked/no change, with related tests/scripts and the reason. At minimum reconcile:

- Parent placement; master's assisting-file/log rules, execution queue and separate post-demo fixture-pruning gate.
- Plan 13's execution protocol, `Public Handoff Contract`, source/packing/capture/review tasks and sprite-contract companion. Repoint asset/tool ownership without changing schema authority.
- Plan 15's `Public Handoff Contract`, `run_scenario`/`validate_art_manifest` links and verification tasks. Repoint the acceptance runner and its tests together.
- Plans 1-7, 14, 16 and 17 wherever config, fixtures, outputs, captures or commands are consumed; audit Plans 8-12 too rather than assuming future work has no inherited placement rules.
- Player/demo guides and capture/checkpoint records, including the native art and industry startup corrections. Preserve historical outcomes/provenance; label superseded commands and provide current equivalents without inventing new passes.

Update policy text, public handoffs, reading tables, runnable examples, test/launcher assumptions, links and completion checks. Link repair alone does not complete reconciliation. Record which parent decision each changed producer consumes so later work cannot reintroduce a docs data home.

### Screenshot validators can be repointed

**No screenshot validator needs screenshots to remain in `docs/zz-reach2/`.** Repoint validators to an explicit manifest under ignored `logs/proofs/mlm/native-art/` or a complete named feature run under `mlm/reviews/`. Update their JSON/HTML references and hashes together, and update the tests producing the screenshots. Evidence identity and completeness matter; the former directory does not. No compatibility copy, symlink or silent fallback into docs should remain.

### Dependencies to update together

1. `media/multilayer/convert_m1_art.py` reads raw artwork and the palette from the documentation assets tree and imports quantization code from its `tools/`. Other converters and guide generators share this dependency. `pack_m1_art.py` reads the public sprite manifest through that layout.
2. `src/tests/multilayer_sprite_contract.py` reads `multilayer_sprite_manifest.json` and `multilayer_geometry_templates.svg` from docs. Its explicit generation mode also writes there. These are machine-consumed asset contracts, not incidental illustrations.
3. `media/multilayer/activate_m1_art.py` and `review_m1_art.py` validate reports, source saves, PNG hashes, logs, galleries and the packed GRF. Some files called reviews have become required proof inputs. A directory-wide ignore or deletion would break these consumers.
4. Specifically, the current untracked `tbm-geology-runtime.json` references **eight untracked PNGs** and `src/tests/fixtures/haulage/tbm-geology-journey-1-regional-midcut.sav`. The activation code now selects that report. All referenced files exist locally; this audit has not rerun their hash/semantic validation. They need explicit promotion or replacement by equivalent freshly validated evidence.
5. The M1 control, terrain, selected deep/underground loop and facility reports also reference retained frames and saves. Their logs exist locally under ignored `logs/`. A checkout cannot rely on those local logs: classify and retain the evidence required by offline validation, or regenerate it through a documented native verification step.
6. Native C++ capture writers hard-code the docs review directory, including `tbm_feedback_runtime.cpp`, `tbm_hangar_input.h`, `tbm_exit_replay.h`, `multilayer_terrain_runtime.cpp`, `multilayer_control_runtime.cpp`, `multilayer_facility_runtime.cpp` and `native_art_loop_fixture.h`. Moving only Python wrappers will not redirect their output.
7. Save paths are both inputs and outputs. `tbm_logistics_runtime.cpp` validates their parent against specific fixture directories; Python runners also assert `source.parent == FIXTURES`. Those checks must be changed deliberately when adding nested fixtures and run directories.
8. The relocated Python runners use `parents[4]`/`parents[5]`, `sys.path` manipulation and, in two master-plan runners, sibling imports by the old filename. Renaming them requires import and root-resolution changes.
9. `src/tests/multilayer_state_fixture.py` still writes result text into the plan directory. `sprite_quantize.py --self-test` creates three `quantize_selftest_*.png` files in docs. Both are output producers to fix, not just files to move.
10. `play.ps1` names the untracked version-399 demo saves and the version-393 UI demo explicitly. These saves cannot be discarded as unused run output without preserving the launcher/demo contract.

## Proposed directory ownership

Paths below are proposed destinations. Existing numbered gameplay plans remain entry points in the month directory; supporting material moves into named subdirectories. This cleanup plan remains at its requested path.

```text
docs/zz-reach2/upgrades/2026-10/
  r2fc-file-cleanup.md
  r2tbmf-tbm-fixes.md                 # plan entry point
  r2mlvs3mp-master-plan.md             # plan entry point
  ...numbered owner plans...
  tbm-fixes/                         # TBM notes and checkpoints
  portal-tbm/                        # master-plan audits and checkpoints
  graphics/                          # art briefs, prompts and explanatory notes
  map-layers/                        # earlier map migration/review notes

src/tests/
  multilayer/
    tbm/                             # native TBM runners
    geology/                         # geology and network runners
    utilities/                       # tube/geothermal/heat file checks
    acceptance/                      # demo preparation and combined acceptance
    support/                         # shared run paths and helpers
  fixtures/
    utility/provenance/               # true older-format input diagnostics
    haulage/<scenario>/              # selected immutable saves, by purpose
    geology/<scenario>/              # geology inputs where that is the owner
    config/multilayer/               # native test configuration templates

media/multilayer/
  source/legacy/                     # original layer source art, with provenance
  source/m1/                         # M1 raw art and generation records
  contracts/                        # public sprite manifest and geometry template
  palettes/                         # DOS palette
  guides/legacy/                     # authoring/reference guides
  guides/m1/
  tools/                            # shared art authoring/conversion helpers
  sprites/m1/                       # existing derived atlases/manifests
media/baseset/                       # existing distributed GRF/source ownership

mlm/
  runtime/<profile>/<run-id>/<process>/ # ignored: isolated OpenTTD data home
    ai/library/ baseset/ content_download/ game/library/ newgrf/
    save/autosave/ scenario/heightmap/ screenshot/ social_integration/
  tools/
    savegames/                      # inspection and plane digest tools
    diagnostics/                    # call-site ledger tooling
    run/                            # multi-layer game/window helpers
  reviews/
    README.md                       # tracked: producer catalogue and retention policy
    <feature>/<scenario>/<run-id>/   # ignored: one feature/scenario attempt
      images/ saves/ reports/ scratch/
logs/<feature>/<scenario>/<run-id>/  # ignored: stdout/stderr/build/native logs
```

Examples: `mlm/reviews/tbm/hangar-return/<run-id>/images/`, `mlm/reviews/geology/region-boundary/<run-id>/saves/`, and `mlm/reviews/geology/terrain-hardness/<run-id>/reports/`. Test entry points live under `src/tests/`; they write artifacts into the corresponding purpose-named review directory.

### Screenshot producer and feature catalogue

Complete this initial mapping from live producers in Groups 1-2. The feature is the player-facing system being reviewed, rather than a plan prefix or test language. Shared capture utilities receive feature/scenario context from their caller. A combined test may emit to several feature directories with the same run ID; its aggregate report links to those artifacts without flattening or duplicating them.

| Current producer or test family | Required feature/scenario placement | What to record |
| --- | --- | --- |
| `tbm_feedback_runtime.cpp`, native feedback/advice runners | `mlm/reviews/tbm/<planning-or-work-scenario>/<run-id>/images/` | Exact UI action, test case/runner and source save; distinguish planning, replanning and work advice. |
| `CaptureHangarWindow`, `TBMExitReplay::BuildThroughInput` | `mlm/reviews/tbm/hangar-orders/<run-id>/images/` or `mlm/reviews/tbm/far-exit/<run-id>/images/` | Caller's scenario, machine/stage and native writer; a shared helper must not force exit captures into the hangar category. |
| `NativeArtLoopProbe`, `multilayer_loop_capture.py` | `mlm/reviews/tbm/spoil-loop-<layer>/<run-id>/images/` | Layer, physical loop stage, frame/view/zoom and source/report identities. |
| `multilayer_terrain_runtime.cpp`, geology runners | `mlm/reviews/geology/terrain-hardness/<run-id>/images/`; portal picker frames under `mlm/reviews/portals/picker/<run-id>/images/` | Partition the mixed frame set by feature; retain material/portal and zoom variants. |
| `multilayer_control_runtime.cpp` and its capture wrapper | `mlm/reviews/construction/layer-controls/<run-id>/images/`, `mlm/reviews/geology/survey/<run-id>/images/`, `mlm/reviews/portals/controls/<run-id>/images/`, `mlm/reviews/vehicles/purchase-cargo/<run-id>/images/` | Assign each emitted window/viewport to its feature instead of putting every control-test frame in one generic directory. |
| `multilayer_facility_runtime.cpp` and its capture wrapper | `mlm/reviews/facilities/<service-depot-or-receiver-scenario>/<run-id>/images/` | Facility, direction/state and transparency variant. |
| Quantizer self-tests, guide previews and `review_m1_art.py` | `mlm/reviews/multilayer-art/<quantizer-or-authoring-scenario>/<run-id>/`; aggregate gallery in `mlm/reviews/multilayer-art/gallery/<run-id>/` | These are art-tool outputs. Galleries reference the original feature-specific game captures without moving them into the art-tool directory. |

The implemented catalogue belongs in `mlm/reviews/README.md`, with producer source, runnable entry point/test tag, feature/scenario, artifact types and input/output roles. Run reports carry `producer`, `native_writer`, `feature`, `scenario`, `run_id`, build configuration, input identities and artifact paths/hashes. Direct native invocations use a documented feature/scenario mapping too; an unmapped writer reports an error instead of dumping into a generic screenshots directory. Historical imports retain known provenance and explicitly label unknown provenance; they are not new test passes.

### File disposition

| Current material | Proposed destination and commit policy |
| --- | --- |
| `r2tbmf-*.py` | Commit reusable runners under `src/tests/multilayer/`: ten TBM runners in `tbm/`, geology checks/network in `geology/`, final integration in `acceptance/`. Use purpose names such as `native_feedback.py`, `pacing_files.py`, `native_checks.py`, `network_checks.py`, `final_checks.py`; preserve owner references in headers. |
| Twelve `r2mlvs3mp-*.py` runners | Eleven utility/tube/geothermal checks in `utilities/`; demo checkpoint preparation in `acceptance/`. Remove chronological plan prefixes from code filenames. |
| `tools/multilayer_acceptance.py` | Move to `src/tests/multilayer/acceptance/native_ground.py`; distinguish it from the existing `src/tests/multilayer_acceptance.py` before consolidating shared functionality. |
| `tools/make_test_road_type_grf.py` | Put the fixture generator under an appropriate `src/tests/multilayer/` feature/support directory; its generated candidates use a run directory. |
| Six art helpers: `contact_sheet.py`, `extract_palette.py`, `make_portal_guides.py`, `make_sprite_template.py`, `png_indexed.py`, `sprite_quantize.py` | `media/multilayer/tools/`; commit these reusable programs and update all imports/callers. Captures, previews and self-test output go to `mlm/reviews/`; publishing art remains an explicit operation. |
| `tools/savegame_plane_digest.py`, `tools/inspect_layer_save.py` | `mlm/tools/savegames/`; keep them reusable and independent of documentation depth. |
| `tools/build_call_site_ledger.py` | `mlm/tools/diagnostics/`; its intentional Markdown update remains an explicit documentation operation. |
| `tools/capture_art.ps1` | `media/multilayer/tools/`, with capture output in a named review run. |
| `tools/ottd_window.ps1`, `run_dedicated.sh`, `run_game_logged.sh` | `mlm/tools/run/`; update command examples and preserve logs under `logs/`. |
| `r2mlvs3mp-q-native-art.cfg` | `src/tests/fixtures/config/multilayer/native-art.cfg`; use it as an immutable template for build/run-local copies. |
| `assets/raw/`, `assets/mlvs/raw/`, generation records, authoring JSON | Split into `media/multilayer/source/legacy/` and `source/m1/`, retaining source/provenance relationships and every required build input. Classify older candidates individually. |
| `assets/multilayer_sprite_manifest.json`, `multilayer_geometry_templates.svg`, `dos_palette.txt` | `media/multilayer/contracts/` and `palettes/`; update generators and consumers together. |
| `assets/reference/`, guide PNGs, reference/registration images under `assets/mlvs/` | Named `media/multilayer/guides/` families when reusable authoring inputs. Reproducible output-only previews belong under `mlm/reviews/`. |
| PNGs/JSON/HTML currently required by activation or gallery validation | Curate the complete dependency set under `src/tests/fixtures/graphics/m1/<purpose>/`, with a manifest explaining producer, consumer and hashes. Commit required proof inputs, including any necessary saved/log evidence. |
| Other screenshots, contact sheets, animated previews, repeated galleries, PPMs, self-test images and machine-generated reports | `mlm/reviews/<feature>/<scenario>/<run-id>/`; ignored by default. Existing historical material is classified and preserved before retiring its old path. |
| Saves in `src/tests/fixtures/haulage/` | Keep selected immutable regression, compatibility and demo inputs in purpose-named fixture subdirectories. Move intermediate/repeated run output into `mlm/reviews/.../saves/`. A filename containing a version or run number is insufficient grounds for deletion. |
| Three text files in docs `tools/` | Preserve baseline/final digest records as fixture data if machine-consumed, otherwise as Markdown evidence tables in `map-layers/`. Convert the closure-status report into documented historical results. Preserve exact hashes and recorded outcomes. |
| Tracked `tools/__pycache__/*.pyc` | Remove disposable bytecode from the tracked layout during implementation; existing bytecode ignore rules remain. |
| `r2tbmf-*.md` companions, `r2mlvs3mp-*.md` companions, art prompts/notes and earlier map review notes | Group under `tbm-fixes/`, `portal-tbm/`, `graphics/`, and `map-layers/`. Keep plan entry points and preserve historical filenames/anchors where useful. Update inbound and outbound links. |

The existing packed GRF, its hash, NFO/PNG sources, generated C++ sprite table and `src/tests/fixtures/graphics/m1-packed-sprites.bin` are an established delivery/test chain. They are not blanket-ignore candidates merely because tools generate them. Preserve their required checked-in contract and verify it after the path migration.

## Output and fixture contract

1. **Separate input from output.** Fixtures are read-only during normal tests. A source save may be a curated fixture or an explicitly selected earlier stage in the same run. Resaves, malformed variants and later checkpoints always go into the run's `saves/` directory. Promotion to a permanent fixture is deliberate and documented.
2. **Feature before scenario before run.** Python and native tests share the routing contract. Allocate one unique invocation ID and pass it to native children using `--run-id` / `OTTD_TEST_RUN_ID`. Each producer requests paths for its declared feature/scenario. A single-feature test uses one directory; mixed tests use several directories sharing the invocation ID. Every screenshot goes into its feature run's `images/` directory. Direct native invocations use documented test identities and never fall back to docs or fixtures.
3. **Keep the boundaries.** Resolve the repository root independently of the current directory and old plan depth. Resolve and check output containment under repository `mlm/reviews/`, with logs under `logs/` and process-local native data under `mlm/runtime/`. Update existing parent-equality checks to permit the intended nested fixture/run paths without allowing arbitrary filesystem writes or traversal through junctions/symlinks.
4. **No collisions or shared global rasters.** Reserve the invocation ID once and give each child explicit context. Repeated invocations cannot reuse earlier output; feature/scenario paths distinguish children within one invocation. Remove global backup/overwrite/restore capture behavior. Keep serial execution where startup/configuration files are shared; separate artifact directories alone do not make native runners safe to parallelize.
5. **Trace every capture.** Record the producer catalogue and per-run metadata above, including exact input/frame hashes, command/configuration and build identity. Reports and galleries link across feature directories using validated repository-relative paths. Do not guess a feature from a chronological filename prefix or scan historical captures to select the latest result.
6. **Select proof inputs explicitly.** A committed fixture manifest lists each retained proof/input and its consumer. Follow JSON references through frames, saves, logs, galleries and payloads before deciding that an artifact is disposable. Live validation selects complete new runs; offline validation selects a complete curated fixture set. An explicitly curated gallery may combine different historical runs, with each input's provenance retained; undeclared cross-run substitution must fail. Neither mode silently depends on ignored prior runs or fabricated provenance.
7. **Preserve evidence meaning.** Moving a PNG or save preserves its bytes and hash. Rewriting path-bearing JSON or HTML changes that document's hash; update dependent metadata in dependency order and revalidate. Historical source records should retain provenance and the relocation mapping. Moving evidence does not constitute a new successful run or user acceptance.
8. **Ignore only generated locations.** Add anchored rules such as `/mlm/reviews/*` with `!/mlm/reviews/README.md` and `/mlm/runtime/`, retaining log/bytecode exclusions. Keep `mlm/tools/` and the producer catalogue trackable; do not ignore all of `/mlm/` or blanket-ignore extensions. Tracked artifacts require explicit reviewed relocation/removal from their old layout. Remove the docs PPM rule after its writers/files migrate.
9. **Separate native data homes from review evidence.** Give each multi-layer native process an explicit config beneath `mlm/runtime/<profile>/<run-id>/<process>/`, including separate host/client homes. Preserve baseset/NewGRF/language/startup-script discovery from existing repository distributions; stage only needed runtime copies or use supported explicit paths and verify identities. Never launch using a docs/fixture template as the writable data home. Capture or collect logs under `logs/` and evidence under `mlm/reviews/`; runtime scratch is not automatically a retained fixture or review. Preserve ordinary game path behavior and shared build inputs.

## Implementation sequence

All tasks below are unstarted. The sequence follows dependencies; each batch groups work requiring comparable difficulty. Complete contract/design batches before the repetitive consumer migrations they enable. Difficulty tags guide later executor selection and do not request parallel agent work.

Retain architecture/owning-plan headers in touched source/tests and document changed functions' purpose, parameters and returns. Keep builds and heavyweight native tests serial, with the established resource limit (`-j 1 -- /p:CL_MPCount=2 /nodeReuse:false`). Batch source changes before building; rebuild only after a completed source batch or a failure requiring correction. Do not add Markdown beneath `src/` or other source directories; directory policy stays in this plan and root `mlm/reviews/README.md`.

### Area Mandatory Reading - Inventory and dependency ownership

_Applies to Groups 1-2._

| File name | Line Ranges | Description |
| --- | --- | --- |
| [activate_m1_art.py](../../../../media/multilayer/activate_m1_art.py) | 18-23 | `PUBLIC`, `PACKED`, `RUNTIME`, `CONTROL`, `TERRAIN` identify the selected contracts/reports. Start dependency enumeration from these entry points instead of inferring usage from filenames. |
| [review_m1_art.py](../../../../media/multilayer/review_m1_art.py) | 22-26; 78-135 | `REPORTS`, `GALLERY` and `validate` define cross-report frame/save/log/sound dependencies and acceptance. Inventory the complete selected evidence set before deciding what to retain. |
| [convert_m1_art.py](../../../../media/multilayer/convert_m1_art.py) | 18-28; 36-41 | `ASSETS`, `OUTPUT`, palette import and `repository_path` distinguish original media, converted output and containment. Classify artifacts by role rather than extension. |
| [Industry lifecycle notes](r2mlvs6-a-industry-lifecycle.md) | 666-675 | The startup diagnostic correction records docs config placement as an error, its content/language consequences and the move back to build-local startup. Use this evidence when documenting why the old layout arose; it does not prove which invocation created each empty directory. |
| [Native art notes](portal-tbm/r2mlvs3mp-r-runtime-art-notes.md) | 15-15 | The Windows capture paragraph explains the `build/native-art.cfg` workaround and baseset discovery requirement. The replacement runtime contract must preserve that behavior while superseding the fixed build path. |
| [Plan 15](r2mlvs15-integrated-acceptance-and-balance.md#public-handoff-contract) | 68-70 | `Public Handoff Contract` names the docs acceptance runner and its exported validation functions. Include this handoff and consuming tests in the plan-impact map; a script move alone leaves a stale public contract. |

### Group 1 - Enumerate files and screenshot producers

{{SIMPLE}}

- [X] 1.1 Record a fresh read-only status/file inventory under `mlm/reviews/file-cleanup/inventory/<run-id>/reports/migration.json`, including old path, tracked state, type, size and hashes for retained binary candidates. Keep the original planning snapshot unchanged.
- [X] 1.2 Enumerate every screenshot writer and capture wrapper, including native C++, Python, PowerShell and art tools. Record source symbol/test tag, invocation, output types and current path expressions; include mixed-feature producers.
- [X] 1.3 Enumerate readers of selected PNG/report/save files and path-bearing JSON/HTML, including activation, galleries, demo launcher entries and ignored log dependencies. Record missing references without inventing replacements.
- [X] 1.4 Enumerate tools, configurations, bytecode, generated text and Markdown companion families under docs. List dynamic imports, depth-based root discovery and command examples affected by each move.
- [X] 1.5 Inventory all eleven named directory trees, including empty descendants and ignored files; capture current native `-c`, working-directory, startup-script and data-search assumptions. Record root `reviews/` separately and recheck before removing any empty directory.
- [X] 1.6 Build a plan-to-decision-to-producer impact map from the R2MLVS parent, R2MLVS3MP and all 17 numbered children, linked companions/TBM plans and active predecessor commands. Record relevant rules, rationale evidence, tests/scripts and checked/no-change dispositions; do not infer that an unimplemented child is unaffected.

### Group 2 - Resolve roles and feature assignments

{{HARD}}

- [X] 2.1 Complete the required evidence dependency graph from current validators. Explicitly classify the eight geology PNGs, their report/source save, selected M1 evidence and required logs as retained input or reproducible prerequisite.
- [X] 2.2 Assign every screenshot producer and mixed producer's frame categories to canonical feature/scenario names. Record the mapping, test invocation and provenance fields in `mlm/reviews/README.md`; shared helpers consume the caller's classification.
- [X] 2.3 Decide the minimal immutable regression/demo save set and destinations; protect compatibility versions and all launcher targets. Keep uncertain unique evidence pending classification rather than deleting it.
- [X] 2.4 Finalize the per-file destination/disposition map, distinguishing source media, immutable input, historical documentation evidence and generated output. Include JSON/HTML rewrite order, hash dependencies and existing tracked artifacts.
- [X] 2.5 Specify the shared Python/native path API, invocation-ID transport, multi-feature routing and fixture manifest. Keep one routing policy and the existing asset/validation authorities; do not add a second gameplay or sprite registry.
- [X] 2.6 Publish `Development artifact layout` in the R2MLVS parent: explain intentional docs asset/helper placement versus the documented accidental runtime root; select `mlm/runtime/`, `mlm/reviews/` and `mlm/tools/`, retaining source media and curated-test ownership. Define config/content discovery, all eleven directory destinations, root-review migration and explicit migration status; do not claim implementation complete.
- [X] 2.7 Reconcile the master and affected child/companion plans against that shared decision before migrating producers. Supersede conflicting placement/config rules, update affected public handoffs and test obligations, and link this cleanup as the migration owner. Preserve gameplay completion states, historical provenance and master task12.6's separate user-acceptance gate for fixture pruning.

### Area Mandatory Reading - Path contract and executable producers

_Applies to Groups 3-5._

**Entry gate:** Group 3 requires Group 2's published parent `Development artifact layout`, reconciled plan-impact map and producer catalogue. Verify `mlm/reviews/README.md` maps mixed-feature producers and the inventory assigns all eleven directories. Missing decisions/assignments stop dependent routing work; inventory completion may continue. Groups 4-5 then require Group 3's `RunContext`, `create_run_context`, `resolve_input_path` in `src/tests/multilayer/support/artifact_paths.py`, plus `TestArtifactPaths`, `GetTestArtifactPaths`, `ResolveTestInputPath` in `src/tests/test_artifact_paths.h`, including process-local `runtime_dir` and `config_path`. Verify declarations/signatures and focused Python checks; native integration is built in Group 9. Missing exports stop Groups 4-5; independent documentation analysis may continue. Do not invent feature buckets, duplicate helpers, reach through private fixtures or use docs/flat fixture directories as a fallback. These exports are proposed until Group 3 produces them.

| File name | Line Ranges | Description |
| --- | --- | --- |
| [multilayer_state_fixture.py](../../../../src/tests/multilayer_state_fixture.py) | 64-105; 108-152 | `run_case` and `main` show startup/config restoration and result emission into docs. Preserve restoration while routing generated saves/reports separately from source inputs. |
| [tbm_logistics_runtime.cpp](../../../../src/tests/tbm_logistics_runtime.cpp) | 634-680 | `Loaded source logistics reaches its next physical exchange` validates source/output together against fixture parents. Separate read/write permissions while preserving the real loader/timer observer. |
| [native_art_loop_fixture.h](../../../../src/tests/native_art_loop_fixture.h) | 179-202 | `NativeArtLoopProbe` constructor binds scene output to the old review directory. Replace this path owner with feature/scenario context without changing assignment or initial-held observation. |
| [tbm_hangar_input.h](../../../../src/tests/tbm_hangar_input.h) | 58-69 | `CaptureHangarWindow` is a shared native writer. Use caller-supplied feature/scenario paths while preserving blitter/font restoration; hangar and far-exit callers need different scenarios. |
| [Final checks runner](../../../../src/tests/multilayer/acceptance/final_checks.py) | 38-104 | `run` chains saves, restores startup/configuration and backs up shared rasters. Replace global raster handling with per-feature paths while preserving serial child execution and source hashes. |
| [Windows path setup](../../../../src/os/windows/win32.cpp) | 250-345 | `DetermineBasePaths` derives the working search root from `_config_file` and the binary root from the executable. A docs template passed directly to `-c` changes content lookup regardless of the shell's current directory. |
| [Native path setup](../../../../src/fileio.cpp) | 925-1050 | `DeterminePaths` selects the personal data home, creates native/default download subdirectories and assigns the native log location. Launch isolation must direct these outputs into MLM runtime and collect evidence/logs appropriately; do not change global directory semantics to repair test setup. |
| [Packed-art fixture producer](../../../../src/tests/multilayer_packed_art_fixture.py) | 35-65 | `main` currently materializes the docs config template as `build/native-art.cfg`. Preserve exact fixture production while replacing the hard-coded runtime destination and updating all native consumers. |
| `src/tests/multilayer/support/artifact_paths.py`; `src/tests/test_artifact_paths.h` | Live definitions; no fixed range | Produced by Group 3: the six exports named in the gate become the public routing boundary for Groups 4-5. Consume these exports without duplicating private path rules. |

### Group 3 - Implement shared artifact routing

{{HARD}}

- [X] 3.1 Implement repository discovery and validated feature/scenario/run identifiers in the shared Python helper, independent of current directory and documentation depth.
- [X] 3.2 Implement `RunContext` and `create_run_context` for atomic invocation-ID reservation, feature-specific `images/saves/reports/scratch` paths and matching logs. Support multiple features in one invocation without flattening output or permitting reuse by another invocation.
- [X] 3.3 Implement `resolve_input_path` for immutable fixtures and explicit earlier-stage run inputs; reject fixture writes and output paths escaping authorized roots after canonicalization.
- [X] 3.4 Implement C++ `TestArtifactPaths`, `GetTestArtifactPaths(feature, scenario)` and `ResolveTestInputPath`, consuming `OTTD_TEST_RUN_ID` consistently. Define direct-native invocation behavior using an explicit mapped test identity.
- [X] 3.5 Add focused behavioral checks for feature partitioning, same-invocation cross-feature output, distinct repeated invocations, nested fixture inputs, malformed identifiers and attempted fixture/output-root escape. Register native checks in the existing test build; do not trigger a standalone build yet.
- [X] 3.6 Extend shared context with validated `runtime_dir` and `config_path` under `mlm/runtime/<profile>/<run-id>/<process>/`. Materialize writable config/startup copies from immutable templates, isolate network peers, preserve required repository content discovery and validate containment independently of feature-specific screenshot paths.

### Group 4 - Integrate native test writers

{{HARD}}

- [X] 4.1 Route TBM feedback, advice, hangar and far-exit captures through the shared helper and producer catalogue. Shared screenshot functions take caller context; filename prefixes do not choose the destination.
- [X] 4.2 Route terrain/control/facility tests by each frame's feature. Split survey, portal, construction and purchase frames into their feature directories even when one native test emits them together.
- [X] 4.3 Route `NativeArtLoopProbe` scenes into the selected TBM spoil-loop scenario, preserving layer/frame/zoom distinctions and aggregate report references.
- [X] 4.4 Split native save readers from checkpoint/resave writers, including recorder environment paths and logistics/reload guards. Accept nested fixture inputs and earlier-stage run inputs; generated saves use the run directory.
- [X] 4.5 Remove native docs/flat-output fallbacks and make each producer report actual artifact paths. Preserve simulation observations, immutable-input checks and native shutdown/restoration behavior.
- [X] 4.6 Update native test startup/config consumers to use the supplied runtime home instead of docs templates or hard-coded build configs. Preserve local-search/config-write controls, required base-set/NewGRF/language identities and startup behavior; never modify global OpenTTD path rules or silently use a private installed data set.

Context has compacted 1 times while executing this group.

### Group 5 - Relocate and adapt runners and helpers

{{MEDIUM}}

- [X] 5.1 Move thirteen TBM-plan runners into their mapped TBM/geology/acceptance directories; rename by purpose and repair imports/root discovery.
- [X] 5.2 Move twelve master-plan runners into utilities/acceptance; replace readiness/recovery and topology/allocation imports by old sibling filenames with relocated module references.
- [X] 5.3 Move the docs acceptance runner and road-type fixture generator into their test owners. Distinguish `native_ground.py` from existing `src/tests/multilayer_acceptance.py` and preserve validation behavior.
- [X] 5.4 Move six Python art helpers and the capture PowerShell helper into `media/multilayer/tools/`; move save inspection, ledger, window and shell helpers into the mapped root tooling directories. Repair script imports/callers; manual launch behavior is completed in Group 5C.
- [X] 5.5 Update existing `src/tests/` capture wrappers and relocated runners to pass the invocation ID, collect feature-specific artifacts and emit producer/input/path/hash metadata. Remove shared raster backup/overwrite/restore and broad historical globs.
### Area Mandatory Reading - Legacy fixture consumers

_Applies to Group 5B. Execution split discovered during Group 5: native output routing also changes the older Python consumers of those checkpoints._

Read the Global Mandatory Reading contracts; `multilayer_state_fixture.py` exports `save_path`, `output_save_path`, `log_path`, `run_case`; and the current caller's `main`/writer function. Read `artifact_paths.py::resolve_input_path` and `runner.py::current_run_context` before changing input/output ownership. Immutable input lookup must not hide a missing newly generated result. Apply only this group's focused checks; native integration remains Group 9.

### Group 5B - Separate older fixture inputs and outputs

{{MEDIUM}}

- [X] 5B.1 Separate metadata fixture input lookup from declared output reservation; route startup/config/log/report writes through the shared context.
- [X] 5B.2 Adapt geology, portals, mines, processors and content fixture callers to their explicit feature contexts; preserve malformed input/refusal checks and source hashes.
- [X] 5B.3 Adapt older TBM, underground fleet and utility fixture wrappers to collect new native outputs from the declared run and read retained inputs separately.
- [X] 5B.4 Redirect remaining documentation result writers and malformed/resave products into their named run; check that no edited producer writes into immutable fixture ownership.

### Area Mandatory Reading - Launch and art helper entry points

_Applies to Group 5C._

Read `runner.py::native_command`, `console_executable`; the current launcher/capture function; `artifact_paths.py::prepare_runtime`; and Windows `DetermineBasePaths` only if changing content discovery. Read `multilayer_packed_art_fixture.py::main` and `sprite_quantize.py::self_test` for their exact output responsibilities. Ordinary player/Classic launcher behavior remains unchanged.

### Group 5C - Complete manual/runtime entry points

{{MEDIUM}}

- [X] 5C.1 Redirect quantizer self-test PNGs into a named run, and use the relocated media/save inspection helper paths.
- [X] 5C.2 Update shell/PowerShell capture launchers to construct shared runtime context, pass explicit native config and collect feature-owned screenshots and logs.
- [X] 5C.3 Finish packed-art setup and acceptance configuration guards using immutable templates and materialized runtime copies; preserve repository content discovery.
- [X] 5C.4 Run focused helper/import/routing checks and complete the producer catalogue for these entry points before moving media inputs.

### Area Mandatory Reading - Media and retained proof migration

_Applies to Groups 6-7._

**Entry gate:** Groups 2-5C must provide the resolved migration/producer catalogue and public routing exports above. Verify each candidate's role/destination and each changed producer's context propagation to its native writer. Unresolved evidence dependencies stop retirement of those files; missing helpers stop Groups 6-7. Independent reference enumeration and draft mappings may continue. Do not satisfy missing dependencies from private user directories, unrelated repositories, guessed screenshots, dummy logs, docs compatibility copies or an implicit latest-run fallback.

| File name | Line Ranges | Description |
| --- | --- | --- |
| [pack_m1_art.py](../../../../media/multilayer/pack_m1_art.py) | 316-361; 423-474 | `derive` and `bind_packed_payload` bind deterministic source outputs, packed payload and proof records. Repoint sources/metadata without changing sprite content or bypassing reproducible/loader evidence. |
| [activate_m1_art.py](../../../../media/multilayer/activate_m1_art.py) | 156-181 | `verified_gallery` preserves acceptance only when the referenced gallery still validates. Recheck path-bearing document hashes in dependency order; stale evidence remains a failure. |
| [review_m1_art.py](../../../../media/multilayer/review_m1_art.py) | 138-144; 213-265 | `relative_url` and `main` produce a gallery from selected reports. Replace flat-directory assumptions with explicit feature report paths and correct cross-feature links. |
| [multilayer_sprite_contract.py](../../../../src/tests/multilayer_sprite_contract.py) | 311-344 | `main` reads or explicitly generates public manifest/templates and checks refusal cases. Migrate read and generation destinations together, retaining the schema authority. |
| [play.ps1](../../../../play.ps1) | 52-75 | `$demoSaves` contains the five featured checkpoint mappings. Repoint `Snapshot` values with save relocation while preserving aliases and scenario identities. |

### Group 6 - Repoint validators and manifest consumers

{{HARD}}

- [X] 6.1 Adapt screenshot activation/gallery entry points to select an explicit evidence manifest, either curated fixtures or a complete named run. Remove docs defaults/fallbacks while retaining semantic/hash refusal checks.
- [X] 6.2 Support feature-partitioned report/frame paths and combined galleries. Validate feature/scenario/producer metadata against the catalogue; reject missing, mismatched or undeclared cross-run substitutions. Preserve explicit historical run selection and label unknown historical provenance rather than inventing it.
- [X] 6.3 Repoint public asset contracts, palette, raw-source and helper imports in converters, packers and contract tests to their `media/multilayer/` owners. Preserve one manifest authority and the native sprite layout.
- [X] 6.4 Specify a reproducible proof manifest under `logs/proofs/mlm/native-art/`, including generation commands and required source identities. Immutable regression inputs remain under `src/tests/fixtures/`; generated proof images/reports/logs are not promoted into the commit merely because a validator consumes them.
- [X] 6.5 Implement dependency-ordered metadata rewriting for Group 7: repair HTML targets, rewrite JSON paths, then propagate changed metadata hashes while original PNG/save bytes and historical provenance remain unchanged. Relocation must not create a new acceptance claim.
- [X] 6.6 Audit automatically rebuilt files, including native pixel expectations and build-proof records. Route regenerable products to ignored `logs/proofs/` and provide explicit regeneration; Group 10 must verify the cold cache before completion. Preserve non-regenerable sources and explain any distributed generated file that must remain checked in.

### Rebuilding the ignored art proof cache

Source conversion is split into bounded stages: run `python -B media/multilayer/regenerate_art_sources.py <stage>` in this order: `base`, `terrain`, `portal`, `vehicle`, `purchase`, `facility`, `control`, `hardness`, `packing`. These regenerate the derived atlases/reports and verify the distributed PNG/NFO/header without changing public source completion flags. Then run `python -B src/tests/multilayer_packed_art_fixture.py` to regenerate native pixel expectations.

After the affected native build, run the existing `src/tests/multilayer_packed_build.py` modes `reproducible` and `generator-free` separately, with the actual `--cmake` executable and `--config`. The latter exercises cached distribution and real pixel/anchor loading. These are focused packaging checks, not whole-game regressions.

For fresh visual proof, select a new explicit manifest with `python -B media/multilayer/regenerate_art_proofs.py initialize --manifest <path-under-logs/proofs/mlm/native-art>`. Using the same `--manifest`, run each phase separately: `focused`, `terrain`, `control`, `loop-deep`, `loop-underground`, `facility`, `geology`, `finalize`. Each capture retains native feature paths and exact source/log/frame hashes. `finalize` validates sound, activation and the current 42-family gallery. A missing phase fails; an existing selected capture cannot silently become a different run.

Prerequisites are the configured native build, pinned Python/Pillow/GRF tools, repository-local baseset/content files, and immutable saved inputs. No sound samples or installed user data are copied into the commit. Retain the distributed GRF and its normal source PNG/NFO/header because generator-free packaging explicitly supports building without the art toolchain. Authoring guides that were inputs to image generation retain their exact bytes and hashes; regenerating a revised guide is an explicit source-authoring operation. Historical relocated proofs are labelled as such and do not count as fresh verification.

### Group 7 - Execute classified file relocations

{{MEDIUM}}

- [X] 7.1 Move selected originals/generation records into `media/multilayer/source/legacy/` and `source/m1/`; preserve hashes and source/provenance associations.
- [X] 7.2 Move palette, contracts, geometry templates and reusable guides into mapped media directories; update remaining path-bearing authoring/atlas records.
- [X] 7.3 Move reproducible proof frames, reports, gallery dependencies and evidence logs into the feature-named `logs/proofs/mlm/native-art/` cache. Materialize the proof manifest and validator references; keep immutable regression sources separate and preserve historical evidence without staging it.
- [X] 7.4 Move selected save fixtures into purpose-named subdirectories and repoint Python/native readers, report sources and launcher snapshots together. Verify exact bytes against the inventory.
- [X] 7.5 Move classified generated/historical output into feature/scenario run directories with a relocation record. Preserve uncertain historical files until classified; do not invent their producer or feature.
- [X] 7.6 Recheck the nine currently empty native data trees and root `reviews/` for newly added files. Relocate any discovered contents under the directory map and preserve identities; empty trees require no invented content or downloads. Account for every descendant before retiring the old paths.

### Area Mandatory Reading - Documentation and retention policy

_Applies to Group 8._

**Entry gate:** Groups 6-7 must provide the finalized relocation map and migrated evidence/source destinations. Verify named destination files exist and retained binary hashes match the inventory before rewriting their links or retiring the old paths. Missing destinations stop dependent link rewrites, ignore-policy retirement and directory cleanup; read-only link enumeration may continue. Do not point links at guessed destinations, hide unresolved source inputs with ignore rules, or leave docs compatibility copies as a substitute for completing migration.

| File name | Line Ranges | Description |
| --- | --- | --- |
| [TBM fixes](r2tbmf-tbm-fixes.md) | 18-25 | `Execution rules` contains old assisting-script/save/review destinations and source documentation/build constraints. Replace placement instructions while preserving gameplay task states and applicable execution limits. |
| [Portal/TBM master](r2mlvs3mp-master-plan.md) | 39-43 | Artifact-placement paragraphs and `Log location` direct future producers. Reconcile them with feature-grouped reviews and immutable fixtures; historical Git permissions do not override the current read-only restriction. |
| [.gitignore](../../../../.gitignore) | 3-4; 11-16 | Root log/build and bytecode rules plus the docs PPM exception. Ignore generated review locations specifically while keeping required sources/fixtures eligible for a future commit. |

### Group 8 - Apply mechanical documentation and policy updates

{{SIMPLE}}

- [X] 8.1 Group companion notes/checkpoints under named documentation families; move prompt/brief Markdown into `graphics/` and preserve entry-point plans, anchors and recorded results.
- [X] 8.2 Repair links, commands, architecture/owner references and demo instructions using the migration map. Keep directory-policy Markdown outside source directories.
- [X] 8.3 Complete the Group 2 plan-impact map against implemented paths: reconcile parent/master, every affected child/companion, public handoffs, reading tables, commands and completion checks. Mark audited unaffected plans with reasons; remove conflicting active rules rather than merely adding a new paragraph. Complete `mlm/reviews/README.md` with producer mappings and fixture-promotion instructions.
- [X] 8.4 Convert retained text-only reports into documentation when appropriate, preserving exact digest values; put machine-consumed records with fixtures.
- [X] 8.5 Ignore generated `/mlm/runtime/` and `/mlm/reviews/` runs while keeping `mlm/tools/` and `mlm/reviews/README.md` trackable. Remove the docs PPM exception after migration and disposable tracked bytecode from the working-tree layout without Git writes. Preserve source art and curated fixtures.
- [X] 8.6 Search for old executable paths, docs output writers, global screenshot folders, absolute workspace paths and old helper basenames. Resolve active references and label historical ones. Retire old directories only when every file has a classified destination.
- [X] 8.7 Remove all eleven old directory trees after their contents and creators have migrated; verify absolute targets remain inside this repository and remove only confirmed-empty obsolete runtime trees. Retire root `reviews/` only if empty and unused. This is not the separately gated fixture-pruning operation.

### Area Mandatory Reading - Verification and isolated acceptance

_Applies to Groups 9-10._

**Entry gate:** Groups 3-8 must deliver routing exports, producer catalogue, selected proof manifest and coherent path migrations. Verify these contracts and resolve required input hashes/dependencies before native execution. Missing exports/data stop dependent tests and closure claims; static link/syntax audits may continue. Do not bypass the gate by weakening hashes, restoring docs fallbacks, substituting private artifacts or silently using ignored historical runs/logs. Native/base-set prerequisites must be documented and present; build or report a missing prerequisite instead of substituting mock evidence.

| File name | Line Ranges | Description |
| --- | --- | --- |
| `src/tests/multilayer/support/artifact_paths.py`; `src/tests/test_artifact_paths.h` | Live definitions; no fixed range | Group 3 exports `RunContext`, `create_run_context`, `resolve_input_path`, `TestArtifactPaths`, `GetTestArtifactPaths`, `ResolveTestInputPath`. Verify feature-specific output, protected inputs and no collision/escape through these public contracts. |
| [multilayer_sprite_pack.py](../../../../src/tests/multilayer_sprite_pack.py) | 22-53; 56-85 | `verify_source_refusals` and `main` enforce selected source coverage, native ranges, palette and pixel/anchor round trips. Path migration must preserve these outcomes. |
| [multilayer_packed_build.py](../../../../src/tests/multilayer_packed_build.py) | 70-115; 118-158 | `reproducible` and `generator_free` are focused native packaging modes. Use their actual tool/build prerequisites and retained proof instead of a source-only approximation. |
| [multilayer_packed_load.cpp](../../../../src/tests/multilayer_packed_load.cpp) | 86-162 | `Actual native loader preserves M0 and loads every selected M1 sprite` verifies distributed GRF pixels/anchors through the real cache. Existing delivery artifacts remain required inputs. |
| [Final checks runner](../../../../src/tests/multilayer/acceptance/final_checks.py) | 18-30; 107-114 | `FILTERS` and `main` identify focused cases/configurations. After Group 5, use mapped `acceptance/final_checks.py` and live symbols, not the obsolete docs command. |
| `logs/proofs/mlm/native-art/evidence.json` | Live definitions; no fixed range | Produced by Groups 6-7: selected artifacts, producers, consumers and hashes define the offline input allowlist. Unlisted local evidence cannot make the check pass. |

Context has compacted 1 times while executing this group.

### Group 9 - Run focused checks over the completed source batch

{{MEDIUM}}

- [X] 9.1 Run side-effect-free syntax/import/CLI checks for relocated tools and routing behavior checks, including same-invocation multi-feature partitioning and repeated-run isolation.
- [X] 9.2 Run sprite-contract, quantizer self-test, source-pack, activation and gallery checks against relocated sources/fixtures. Confirm self-tests write only to their declared feature run.
- [X] 9.3 Build the complete affected native source batch with established resource limits, then run native routing checks. Rebuild only for a diagnosed source correction.
- [X] 9.4 Execute representative TBM feedback/hangar/exit, geology capture/resave/network, utility/reload and mixed control/terrain captures covering each changed writer/helper. Verify feature paths against the catalogue and fixture/config hashes against the baseline.
- [X] 9.5 Repeat a capture and verify distinct directories with earlier bytes untouched. Check that aggregate galleries link across feature directories and cannot accept frames from another invocation accidentally.
- [X] 9.6 Run affected reproducible/generator-free packaging checks and actual native pixel/anchor loading. Retain results with command/build/input identities; do not claim unexecuted modes passed.
- [X] 9.7 Cold-start representative multi-layer native test/demo profiles and a host/client case from a different shell directory using their new runtime configs. Verify data/search paths, required content/language loading and process separation; exercise default save/autosave/scenario/screenshot handling and confirm retained outputs reach the declared review/log owners.

### Group 10 - Verify independence from workstation history

{{HARD}}

- [X] 10.1 Validate from a checkout-like input allowlist excluding ignored historical runs/logs. Use an isolated area inside the repository or another user-authorized temporary location without Git writes; include documented build/base-set prerequisites.
- [X] 10.2 Prove curated offline validation resolves its complete selected evidence set and fresh-run validation resolves explicitly selected feature runs. Missing required proof must fail; no docs directory or implicit prior run may satisfy it.
- [X] 10.3 Diagnose dependency, provenance, feature-routing or binary-identity mismatches against the owning producer/validator; fix that authority and rerun affected checks. Do not regenerate immutable source art or modify gameplay to make relocation pass.
- [X] 10.4 Compare status, input hashes and filesystem destinations before/after a representative run. Confirm no docs artifacts, fixture mutations, feature-root screenshot dumps or unexpected tracked outputs; producer metadata accounts for every screenshot.
- [X] 10.5 Check the filesystem, including empty/ignored directories, before and after cold/repeated native launches. Require all eleven original docs directory paths to stay absent; verify runtime-generated trees are confined to `mlm/runtime/`, review evidence to `mlm/reviews/`, and logs to their declared destination. Git cleanliness alone cannot pass this check.

### Area Mandatory Reading - Commit-selection handoff

_Applies to Group 11._

**Entry gate:** Groups 9-10 must have actual passing evidence for required checks and no unexplained artifacts/dependency failures. Verify reports and outcomes, not just checkboxes. If missing, stop completion and final file-selection claims; read-only diff inspection and listing outstanding failures may continue. Do not substitute the old audit snapshot, historical pass records or ignored source-workstation files.

| File name | Line Ranges | Description |
| --- | --- | --- |
| [.gitignore](../../../../.gitignore) | Live definitions; no fixed range | Group 8's anchored review policy and existing log/bytecode rules govern inclusion. Inspect resulting patterns and check examples with read-only `git check-ignore`; ignores alone cannot retire tracked artifacts. |
| `mlm/reviews/README.md`; `mlm/reviews/file-cleanup/inventory/<run-id>/reports/migration.json` | Live definitions; no fixed range | Produced by Groups 1-2 and finalized by Group 8: producer catalogue and classified relocation records explain every retained/moved artifact. Reconcile proposed selection against them; new local files are not automatically commit-worthy. |

### Group 11 - Audit links, status and proposed commit scope

{{SIMPLE}}

Group 11 corrected two additional legacy continuation modes with shared-log assumptions. Accepted feedback and pacing continuation now require explicit prior-run selection. A fresh accepted-feedback baseline and replay match exactly; pacing seed/check retains its original rate/build prerequisites. Reproducible packing also passes without touching the byte-identical C++ header timestamp.

- [X] 11.1 Check Markdown links, gallery references and canonical commands against final paths; confirm no Markdown was added in source directories and no newly named source/test/JSON uses a planning prefix.
- [X] 11.2 Check ignored generated examples and non-ignored source/fixture examples with read-only Git, accounting explicitly for formerly tracked outputs and bytecode.
- [X] 11.3 Produce the proposed commit file list and explain each retained new binary/input. Exclude reproducible proof/output caches, preserve unrelated gameplay changes, and compare eligible existing file counts and untracked generated additions against the initial inventory; explain rename/deletion bookkeeping separately.
- [X] 11.4 If proposing multiple later commits, keep each dependency chain complete and reviewable. Record limitations and actual validation results; do not stage or commit under the current restriction.
- [X] 11.5 Include the eleven-directory disposition and completed plan-impact map in the handoff. Verify affected parents/children, public contracts and executable tests agree on the shared layout; list any remaining historical-only references and unresolved rationale instead of presenting a file move as a completed decision migration.

## Consult on Demand

Read a source only after its trigger occurs; expand only under the stated condition. A linked source's own reading lists do not become mandatory.

| Trigger | File name | Start at | Expand only when | Consultation purpose |
| --- | --- | --- | --- | --- |
| Sprite ranges or pixel/anchor identity change during path-only migration | [multilayer_sprite.h](../../../../src/multilayer_sprite.h) | `MultiLayerSpritePose` and `GetMultiLayerSprite`, 32-59 | Selected pose/range still differs after input paths and hashes are corrected | Confirm the native pose-to-sprite boundary; this does not authorize changing rendering or simulation semantics. |
| An old save is classified as disposable although a migration test references it | [Savegame lifecycle](r2mlvs14-savegame-determinism-and-lifecycle.md#public-handoff-contract) | `Public Handoff Contract`, 74-82 | Version/phase ownership remains unresolved in the live loader test | Identify the compatibility obligation, then locate its live test; historical prose cannot establish a current pass. |
| Demo alias resolves to a missing/wrong checkpoint after relocation | [Demo guide](d2um-underground-mines.md#launch-it) | `Launch it` | Matching `$demoSaves` alias and input hash do not establish intended scenario identity | Clarify the user-facing checkpoint mapping; launcher code and exact bytes remain delivered behavior. |
| Readiness/topology runner cannot import its relocated sibling | [Readiness checks](../../../../src/tests/multilayer/utilities/readiness_file_checks.py) and [Topology checks](../../../../src/tests/multilayer/utilities/heat_topology_file_checks.py) | Respective `main`, 17-46 and 20-86; mapped successors after Group 5 | Dynamic reference is corrected but required callable exports remain absent | Identify the recovery/allocation helper boundary; do not duplicate implementation to hide an import failure. |
| Combined ground-acceptance report loses an art dependency or accepts wrong evidence | [Ground acceptance runner](../../../../src/tests/multilayer/acceptance/native_ground.py) | `validate_art_manifest`, 184-196; mapped `native_ground.py` after Group 5 | Artifact references resolve but semantic validation still differs | Reconcile aggregate acceptance with canonical validators; its report must not become a second sprite/proof authority. |
| An engine screenshot command still writes to a previous native data home | [fios.cpp](../../../../src/fios.cpp) | `FiosGetScreenshotDir`, 598-609 | The explicit config/root is correct but the same process previously resolved its screenshot directory | Check the cached native-directory boundary; use the run's fixed process home and feature-aware capture/collection, not a global gameplay path change. |

## Planning Provenance

This is not an execution reading queue and must not be preloaded after context compaction. It is non-transitive. Ranges record planning use, not runtime authority; live code and named execution contracts govern implementation.

| File name | Sections or Line Ranges used while planning | Planning contribution |
| --- | --- | --- |
| [User-supplied execution standards](</C:/Users/Axonn/.codex/attachments/54ab4a9d-4a29-404d-8d8f-c1b110b6ec1a/Pasted text.txt>) | Task/difficulty rules; Global/Area Mandatory Reading; Consult on Demand; Planning Provenance; Final validation | Supplies the verbatim protocol, batch limits, scoped reading/gates and final checklist. This attachment is planning provenance, not a runtime dependency. |
| [TBM fixes](r2tbmf-tbm-fixes.md) | `Execution rules`, 18-25 | Identified old artifact-placement conventions and retained documentation/build-resource/Git constraints. |
| [Portal/TBM master](r2mlvs3mp-master-plan.md) | Artifact-placement and log-location paragraphs, 39-43 | Exposed instructions that would recreate docs scripts and flat capture output. |
| [convert_m1_art.py](../../../../media/multilayer/convert_m1_art.py) | Path/import/palette bindings, 18-28; `repository_path`, 36-41 | Established root-level media ownership and why moving docs assets is a code change. |
| [activate_m1_art.py](../../../../media/multilayer/activate_m1_art.py) | Selected path constants, 18-23; `validate`, 105-153; `verified_gallery`, 156-181 | Demonstrated that screenshot locations can change while hashes, source saves and complete proof remain essential. |
| [Geology checks](../../../../src/tests/multilayer/geology/native_checks.py) | `fresh`, 30-35; `run`, 38-98; `malformed`, 101-131 | Established mixed save input/output handling and native-to-PNG/report production. |
| [native_art_loop_fixture.h](../../../../src/tests/native_art_loop_fixture.h) | `NativeArtLoopProbe` constructor, 179-202 | Established that native writers, as well as wrappers, must change to enforce feature grouping. |
| [R2MLVS parent](r2mlvs-multilayer-vehicles-and-structures.md) | Ownership and artifact-placement paragraphs, 7-9; complete numbered child-plan table, 148-166 | Identified the root placement decision, its stated rationale and the 17-child scope for policy/test reconciliation. |
| [Industry lifecycle notes](r2mlvs6-a-industry-lifecycle.md) | Startup diagnostic correction, 666-675 | Provides explicit evidence that a config beside plans incorrectly redirected runtime lookup; prevents inventing a deliberate docs-data-home requirement. |
| [Native art notes](portal-tbm/r2mlvs3mp-r-runtime-art-notes.md) | Windows capture paragraph, 15-15 | Records why startup moved to `build/native-art.cfg`; the new data-home design must preserve base-set/language discovery. |
| [Packed-art fixture producer](../../../../src/tests/multilayer_packed_art_fixture.py) | `main`, 35-65 | Confirms the current template-to-build config copy and distinguishes source location from the runtime home. |
| [Windows path setup](../../../../src/os/windows/win32.cpp); [native path setup](../../../../src/fileio.cpp) | `DetermineBasePaths`, 250-345; `DeterminePaths`, 925-1050 | Explains configuration-root selection and automatic creation of the nine native data trees; informed process-local `mlm/runtime/` and tests for directory recreation. |
| [Plan 15](r2mlvs15-integrated-acceptance-and-balance.md) | `Public Handoff Contract`, 68-70 | Demonstrates that a docs-hosted helper is a public producer consumed by other plans/tests, requiring a handoff migration rather than only link repair. |

## Completion Conditions

- `docs/zz-reach2/` contains documentation; reusable executables and binary/machine-consumed assets have explicit owners elsewhere.
- All eleven named directory trees are absent from the October docs directory and remain absent after cold/repeated native runs, including empty descendants. Their content and runtime responsibilities have documented destinations; root `reviews/` has no remaining MLM dependency.
- Multi-layer native sessions use isolated `mlm/runtime/` homes with the required content/language/startup inputs; standard OpenTTD/Classic path semantics and immutable fixtures remain intact.
- The R2MLVS parent owns the replacement artifact decision. R2MLVS3MP, every affected child/companion, public handoff and executable producer agrees with it; the impact map records all 17 children and audited predecessor/guide dependencies, including checked/no-change dispositions.
- Supporting notes are grouped by purpose, and plans point to working commands and valid evidence locations.
- Test inputs are deliberately retained fixtures; native tests and wrappers create screenshots under `mlm/reviews/<feature>/<scenario>/<run-id>/images/`, with saves/reports alongside and associated logs under `logs/`. Mixed tests partition frames by feature; no flat screenshots directory remains.
- Every screenshot producer is mapped in `mlm/reviews/README.md`, and each run identifies the actual producer, feature/scenario, inputs, build and artifacts. New screenshots are traceable without guessing from filenames.
- Screenshot validators read explicitly selected fixture/run evidence outside docs, including cross-feature references and hashes, without compatibility copies or docs fallbacks.
- Art sources live under the existing root `media/` tree and the build/validation chain resolves every dependency.
- A repeat test run creates no commit noise under docs or fixture directories, changes no source inputs, and preserves prior run artifacts.
- Required proof can be regenerated and validated without undeclared local history, with original source identities, provenance and acceptance state preserved. Automatically rebuilt proof files reside under ignored `logs/proofs/`; the eligible committed payload contains fewer files and recurring runs add no generated commit candidates.
- The final proposed file selection explains every retained new binary. No gameplay work has been lost, and no Git write has been performed as part of this planning/cleanup workflow without a later explicit change to the restriction.
