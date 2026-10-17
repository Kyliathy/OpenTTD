# R2FC commit-selection handoff

Owner: [cleanup plan](../../../zz-reach2/upgrades/2026-10/r2fc-file-cleanup.md). Shared decision: [R2MLVS development artifact layout](../../../zz-reach2/upgrades/2026-10/r2mlvs-multilayer-vehicles-and-structures.md#development-artifact-layout). Detailed results: [execution record](r2fc-a-execution-notes.md).

The repository now separates source media, immutable test inputs, executable helpers and generated evidence. No staging or other Git write is authorized or performed. Keep the routing helpers, relocated producers/inputs, validator changes, directory retirements and shared plan updates together in a later reviewed change. Existing gameplay edits remain present; files containing both gameplay and cleanup changes require review together with their existing dependencies.

## Candidate accounting

The final read-only accounting is in [commit-candidate-accounting.json](../../../../mlm/reviews/file-cleanup/inventory/20261009-cleanup-01/reports/commit-candidate-accounting.json). The [proposed file list](../../../../mlm/reviews/file-cleanup/inventory/20261009-cleanup-01/reports/proposed-commit-files.json) records each current status entry, its original role and any relocation. It is a review selection, not staged content.

The original inventory contains one temporary inventory script in addition to the planning snapshot's 670 untracked files. Exclude that scratch script from both sides of the comparison. Count existing eligible payloads separately from the one-time deletion/rename entries: a deleted tracked file still appears in `git status`, and an unstaged filesystem move appears as a deletion plus an untracked destination.

478 formerly tracked generated products now live in ignored review/proof storage; three formerly tracked bytecode files are removed. Of the original 670 untracked files, 82 remain eligible at their final owners and 588 leave the candidate set. Only two new raw-art PNGs and 17 meaningful saved inputs remain from that original binary batch. Newly introduced routing/validation infrastructure is also eligible; generated runs add no candidates. Existing eligible payloads fell from **4,334 to 3,282**, a reduction of **1,052 files**. The 547 current untracked paths comprise 446 relocated tracked destinations and **101 genuinely new candidates**, versus the original 670. Raw status contains 261 modified, 929 deleted and 547 untracked entries; these include the one-time migration bookkeeping.

## Retained new binary inputs

These are exact existing input bytes, not newly generated acceptance output. Other untracked binary destinations are relocations of already tracked inputs; the accounting distinguishes them. The [fixture index](../../../../src/tests/fixtures/multilayer-inputs.json) preserves exact identities. The two compatibility diagnostic logs under `src/tests/fixtures/utility/provenance/` retain irreproducible older-format issuer/baseline evidence.

| Retained input | Reason |
| --- | --- |
| [terrain-hardness-v1.png](../../../../media/multilayer/source/m1/raw/terrain-hardness-v1.png) | Original authored floor source consumed by the deterministic hardness converter. |
| [terrain-hardness-edges-v1.png](../../../../media/multilayer/source/m1/raw/terrain-hardness-edges-v1.png) | Original authored wall/edge source; native derived atlases are rebuilt in the cache. |
| [tbm-extended-area-393.sav](../../../../src/tests/fixtures/haulage/planning/tbm-extended-area-393.sav) | Accepted area-planning compatibility input. |
| [tbm-extended-tunnel-393.sav](../../../../src/tests/fixtures/haulage/planning/tbm-extended-tunnel-393.sav) | Distinct straight-tunnel planning compatibility input. |
| [tbm-feedback-haul50-393.sav](../../../../src/tests/fixtures/haulage/planning/tbm-feedback-haul50-393.sav) | Shared native haul/advice/parking/release starting world. |
| [tbm-final-release-396-cancel-return.sav](../../../../src/tests/fixtures/haulage/release/tbm-final-release-396-cancel-return.sav) | Actual cancelled-return release continuation. |
| [tbm-geology-journey-1-regional-midcut.sav](../../../../src/tests/fixtures/haulage/terrain-hardness/tbm-geology-journey-1-regional-midcut.sav) | Regional terrain/render/resave and network source. |
| [tbm-hangar-397-2-hangar-parked.sav](../../../../src/tests/fixtures/haulage/hangar-orders/tbm-hangar-397-2-hangar-parked.sav) | Accepted parked-machine hangar state. |
| [tbm-hangar-reload-orders-run-1-orders-travel.sav](../../../../src/tests/fixtures/haulage/hangar-orders/tbm-hangar-reload-orders-run-1-orders-travel.sav) | Native orders travel continuation used by the hangar runner. |
| [tbm-hangar-reload-orders-run-2-orders-travel.sav](../../../../src/tests/fixtures/haulage/hangar-orders/tbm-hangar-reload-orders-run-2-orders-travel.sav) | Separate accepted travel state used by hangar acceptance. |
| [tbm-motion-wait-393.sav](../../../../src/tests/fixtures/haulage/motion/tbm-motion-wait-393.sav) | Loaded wait/whole-spoil boundary. |
| [tbm-parking-parked-395.sav](../../../../src/tests/fixtures/haulage/parking/tbm-parking-parked-395.sav) | Older parked-state release input. |
| [tbm-release-connected-396.sav](../../../../src/tests/fixtures/haulage/release/tbm-release-connected-396.sav) | Connected portal movement/reload boundary. |
| [tbm-release-released-396.sav](../../../../src/tests/fixtures/haulage/release/tbm-release-released-396.sav) | Distinct released support/movement boundary. |
| [underground-demo-exit-399.sav](../../../../src/tests/fixtures/haulage/far-exit/underground-demo-exit-399.sav) | Explicit player/demo launcher input. |
| [underground-demo-planning-399.sav](../../../../src/tests/fixtures/haulage/planning/underground-demo-planning-399.sav) | Explicit planning demo input. |
| [underground-demo-tbm-399.sav](../../../../src/tests/fixtures/haulage/planning/underground-demo-tbm-399.sav) | Explicit working TBM demo input. |
| [underground-demo-tbm-ui-393.sav](../../../../src/tests/fixtures/haulage/planning/underground-demo-tbm-ui-393.sav) | Shared UI/motion/replay compatibility and demo source. |
| [underground-demo-tunnel-399.sav](../../../../src/tests/fixtures/haulage/far-exit/underground-demo-tunnel-399.sav) | Explicit tunnel demo input. |

No saved input was deleted. Selective pruning remains owned by master task 12.6 after user demo acceptance and resolved feedback.

## Eleven-directory disposition

Every old path is relative to `docs/zz-reach2/upgrades/2026-10/`; all are absent, including ignored/empty descendants, after cold and repeated native launches.

| Retired directory | Delivered owner |
| --- | --- |
| `ai/` | `mlm/runtime/<profile>/<run-id>/<process>/ai/library/` |
| `assets/` | Source/authoring inputs in `media/multilayer/`; generated proof in ignored `logs/proofs/mlm/native-art/`; feature reviews in `mlm/reviews/`; prose in named docs families. |
| `baseset/` | Process-local runtime copy of required repository build content. |
| `content_download/` | Process-local runtime directory and ordinary native descendants. |
| `game/` | Process-local runtime `game/library/`. |
| `newgrf/` | Process-local runtime content projection. |
| `save/` | Process-local transient save/autosave; explicit checkpoints under feature `saves/`; curated inputs under fixtures. |
| `scenario/` | Process-local scenario/heightmap directories; immutable inputs retain their fixture owner. |
| `screenshot/` | Native runtime placeholder; engine PNGs are collected into the declared feature's `images/`. |
| `social_integration/` | Process-local native directory; no integrations enabled by relocation. |
| `tools/` | Reusable tests in `src/tests/multilayer/`, art tools in `media/multilayer/tools/`, launch/inspection tools in `mlm/tools/`; prose retained as documentation. |

The former empty root `reviews/` is also retired. This scope uses `mlm/reviews/`; ordinary OpenTTD user-path policy is unchanged. Root `media/` remains the single source-media authority, avoiding a competing root `assets/` tree.

## Shared plans and actual verification

The [17-child impact map](../../../../mlm/reviews/file-cleanup/inventory/20261009-cleanup-01/reports/plan-reconciliation-final.json) records direct consumers versus inherited-policy changes. The parent, master, TBM fixes, graphics/acceptance handoffs and affected companions now agree with the executable producers. The original co-location rationale and documented config mistake remain recorded; the exact invocation that created the old empty native trees is still unknown.

Focused routing, sprite source/contract, native TBM/hangar/exit, utility reload, geology/resave/network, real capture, reproducible packaging and generator-free loader checks passed. The native loader verifies 2,409 source-exact sprites including protected M0; the final gallery validates 42 selected families, 2,343 sprites and 165 native frames. The network run covered 450 synchronized acknowledged frames with equal state. Repeated captures leave earlier bytes unchanged and refuse accidental cross-run substitution.

The cold checkout admitted 2,860 eligible source/fixture files and 170 explicit native build/content prerequisites, with zero historical proof files. Nine conversion stages regenerated successfully. Offline validation used only 187 explicitly selected fresh proof dependencies; a further native run produced 38 mixed-feature frames there. All source/fixture hashes stayed unchanged. All 1,784 original binary identities are preserved. A representative native run left the working checkout's Git status unchanged.

The original DOS graphics/sound provider is a documented locally supplied prerequisite; it is not redistributed. Historical pacing seed/check workflows retain their original rate/build requirement and now require explicit prior-run selection. Historical pass records are not new passes, and human visual/demo acceptance is not claimed. Missing old diagnostic files are labelled unavailable rather than fabricated; exact existing historical logs retain valid links. Full regressions were not run.

Native acceptance output, source conversion caches, PNG conversion records, isolated data homes and the audit reports linked here are ignored generated artifacts. The persistent recipe is in [Rebuilding the ignored art proof cache](../../../zz-reach2/upgrades/2026-10/r2fc-file-cleanup.md#rebuilding-the-ignored-art-proof-cache); source generators and validators remain committed candidates. Temporary execution scripts beside this plan have been removed. The final accepted-feedback baseline/replay matches exact owner/cash/RNG/native-part state; the updated packing proof passes two real builds without changing the shared C++ header timestamp.
