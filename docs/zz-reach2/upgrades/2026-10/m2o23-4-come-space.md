# M2O23-4 ? Reconstruction, publication and final qualification (Groups 54-72)

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
| [Destination map-layer architecture](../../architecture/map-layers/archi-map-layers.md) | 20-37; 272-282 | `Storage, coordinates and distance`, `Initialization and simulation`, and `Invariants` define stacked addressing, physical ownership and view-independent simulation. Every imported feature must preserve these boundaries; camera state must never choose command ownership, stop simulation or consume simulation RNG. |
| [Native layer controller](../../../../src/map_layer.h) | 96-104; 204-235 | `MultiLayerMap::DecodeStoredTile`, `IsTileInLayer`, `TileInLayer` and `ProjectToSurface` are the live geometry boundary used across gameplay and save validation. Decode candidate saves using their own dimensions; neither duplicate layer arithmetic nor consult the running world's geometry as a fallback. |
| [Required cleanup/layout contract](r2fc-file-cleanup3.md) | 85-103 | `End-state storage contract` defines original media, independent fixtures, retained saves, generated review/runtime products and the sole recipe/materialization boundary. This user-designated contract is globally necessary: imports must fit these owners, routine generation cannot publish canonical assets, and historical hashes never certify a current executable. |
| [Destination cleanup completion](r2fc2-file-cleanup3.md) | 44-61 | `Checkpoint and resume point` records completed cleanup, the actual Space successor, closed cash/tick boundaries and the provisional Tube checkpoint. This prevents a merge agent from replaying closed Space work, treating cleanup as gameplay authorization or silently promoting the destination's unfinished fleet. |
| [Donor technical closure](../../../zz-rhr/imports/openttd3-75ea75718f/docs/zz-reach2/upgrades/2026-10/r2mlvs16-b-performance.md) | 9-16 | `Current technical finalization,2026-10-20` distinguishes delivered Plan 3/4 behavior and selected performance acceptance from pending human art/demo feedback and the unresolved version 424 save alias. This is the other half of the architecture handoff: preserve delivered underground behavior without copying blanket completion or deletion claims. |

## Four-plan execution scope

The user requested this four-plan partition on2026-10-23 after completion of22D. Stable global group/task IDs and every existing checkbox state are preserved. Suffix batches remain part of their original numbered group.

| Plan | Owned groups | Current state |
| --- | --- | --- |
| [M2O23-1](m2o23-1-come-space.md) | 1-22D, including completed suffix batches | Complete:40 groups,160/160 tasks. |
| [M2O23-2](m2o23-2-come-space.md) | 23-36 | Complete:27 groups,65/65 tasks. |
| [M2O23-3](m2o23-3-come-space.md) | 37-53 | Complete:19 groups,74/74 tasks. |
| [M2O23-4](m2o23-4-come-space.md) | 54-72, including61B/65B | Complete:21 groups,82/82 tasks. |

The former second plan was renamed by the user to M2O23-3 before this partition; its Groups54-72 now continue in M2O23-4. Finish each plan's owned groups and public handoff before its successor. Completion of Plan1 is the completed foundation checkpoint; the full merge requires Plans2-4 and the final Completion Conditions in Plan4. Splitting documentation performs no gameplay task and authorizes no additional saved-world ticks or spending.

## Continuation boundary and prerequisite handoff

This file owns Groups54-72 moved from the user's renamed [Plan3](m2o23-3-come-space.md). It continues [Plan1](m2o23-1-come-space.md) Groups1-22D, [Plan2](m2o23-2-come-space.md)23-36 and Plan3's37-53. It owns typed process/evidence adapters and reconstruction, the consolidated current build, explicit canonical publication, genuine legacy/Space/paid-world/peer qualification, performance/art/recovery and final architecture/preservation handoff.

**ASSUMPTION: this was already built previously. If not, re-asses at runtime**

Entry gate: Plans1-3 must expose the implemented canonical domain/command owners, Group46's complete `MultiLayerCanonicalRecord`/`ComputeMultiLayerStateDigest` contract and Groups52-53's selected pure candidate products with independent expectations and retained-root identities. Verify live declarations, producing-group evidence and the exact candidate/recipe roles before dependent native preparation. Stop native preparation/qualification whose contract is absent; pure typed-reader/source/argument comparisons may continue under their Area gates. Never substitute an old executable, donor cache, mutable prior receipt, alternate worktree, duplicate provider/store/catalogue or candidate-derived expected baseline for a missing prerequisite. A complete native graph and current source/image identities, not prior completion labels, authorize later qualification claims.

The final Completion Conditions here cover all four plans. Preserve the historical branch receipts and exact originals while publishing separate current evidence. Canonical publication remains the explicit62 boundary; no regression recipe or native preparation helper may silently publish source assets.

## Execution boundaries

**A** is `D:/Codez/Sandbox/OpenTTD`, the destination; **B** is `D:/Codez/Sandbox/OpenTTD3`, the read-only donor. Unlabelled paths mean A. Historical A baseline is `cfeff84db52f8a3753a43c2a770a6363c62b2c67`; current A HEAD verified on2026-10-24 is `d0da7e9d76bbb288ee5e8722730836cc9a2229d5`; B HEAD is `75ea75718fa7d14632bf795105721df1e390f753`; common ancestor is `9a47768ae3e4455661264f2c49febde972538e49`. Verify pins before consuming donor links. Existing working changes are the implementation, not disposable state. [The original chronology](m2o23-1-come-space.md#investigated-snapshot-and-chronology) and [execution record](m2o23-1-come-space-a-execution.md) retain historical evidence; consult exact entries only on observed drift.

No Git writes, staging, checkout, reset or recursive filesystem deletion is authorized. Preserve both repositories, `OpenTTD-baseline/` and `!kit`; no individual deletion is needed. Work sequentially without subagents. The document partition was the earlier22D boundary. The user subsequently authorized implementation through all remaining merge plans and explicitly revoked the temporary model-switch pause. Continue sequential implementation and bounded checks; protected saved-world cash/tick policies still apply.

Execution update,2026-10-24: the user requests faster merging with less frequent testing. Keep completed groups closed for their recorded epochs; do not repeat unrelated suites after a harness or metadata change. Run only observations required by the affected downstream interface, once its implementation is ready, and build only when necessary. Batch related implementation fixes before checking them; test the actual next handoff early. Preserve existing identity guards and honest failure/scope records without adding speculative audits or new approval stops.

Available-RAM build update: the user repeatedly directs proceeding with current RAM rather than stopping for more memory. After the former6GiB physical reserve stopped even cached CMake configuration, active selected builds use a4GiB physical reserve, the existing8GiB commit reserve and one worker, with live owned-process monitoring/reaping. The cached incremental admission allowance is512MiB. Historical54/59/60receipts retain their original6GiB policy; this explicit operational adjustment changes no source/image/content identity guard or protected saved-world budget.

New supply purchases use A's Pending-only refund rule; station delivery ends refund rights. Migrated B stock retains only its original finite unshipped entitlement in the same canonical receipt owner. Protect closed Space cash/tick boundaries and the actual chosen continuation; historical art/demo status and component cases are not current runtime approval.

Use R2FC-3's existing durable-input, fixture and materialization owners. Test/tool source belongs beside tests; generated diagnostics belong in `test-cache/`, `logs/` or declared `mlm/reviews/` runs. No new source/test/JSON planning prefixes or Markdown in source directories. Auxiliary Markdown for this plan belongs beside it and extends its own filename prefix with `-a-...`, then `-b-...` as needed. Never duplicate canonical supply, profile, compiler, catalogue or materialization authority.

## Conflict decisions and safeguards

The original comparison remains in [Plan1](m2o23-1-come-space.md#conflict-decisions-and-safeguards). The following existing decisions constrain this file; live code and focused evidence take precedence over historical status.

| Conflict | Observed evidence | Resolution to implement |
| --- | --- | --- |
| Save version collision | A uses 425-440 for later Space/supply work; B uses 425-434 for TBM face transfer, supplies, geothermal and utility work. Both still use metadata schema 1. | Introduce explicit merged-format lineage/capabilities, preserve both legacy interpretations and allocate the next unused merged writer version after the snapshot is refreshed. Numeric `>=425` tests alone cannot identify a feature. Do not renumber old bytes or globally lie about `_sl_version`. |
| Same chunk, different wire format | A `surface_supply_order_sl.cpp` uses `SSOR` RIFF; B `surface_supply_sl.cpp` uses `SSOH` RIFF plus `SSOR` TABLE. | One dispatcher decodes the exact legacy family into the canonical order owner. Validate complete presence, type, dimensions, day and references in full and copied readers. Reject contradictory or genuinely ambiguous files without changing originals. |
| Supply ownership and payment | A has uint64 intent IDs, pending/fulfilled records and command-kernel settlement; B has uint32 strong IDs, immediate stock, packet order/shipped fields and producer self-debit. | Keep A's owner and new-purchase policy. Adapt B UI/commands to it; widen donor IDs losslessly; convert old order/packet history without double debit, refund or quota restoration. Preserve legacy refund rights explicitly, never by pretending delivered stock is Pending. |
| Sprite namespace | A's M1 ends at 3588, preview occupies 3588-3598, Space 3599-5166, orbital 5167-6142, launch 6143-7790, ground through 22044. B expands M1 to 17847 while preserving the preview gap. | Preserve A's shipped ranges and allocate/remap only donor-only families into nonoverlapping space; resolve duplicate logical families once. Regenerate headers/NFO/GRF/hash/manifests together. Never select one binary GRF wholesale or shift Space by changing M1 count. |
| Shared simulation/utility | B extends utility conversion, generator feeding, geothermal bindings and scheduling; A also has Space, orbital and ground consumers. | Combine at original utility/tick owners with one allocation/settlement and deterministic ordering. Heat and electricity retain units, lineage, loss and once-only spending. Hidden-layer execution must remain equivalent. |
| Renderer and UI overlap | Both alter stations, vehicles, sprites, English strings and windows; B adds tube/geothermal native consumers and demo controls. | Integrate by actual render/command owner, preserving A's Space/click bounds and B's layer-aware ground behavior. Add/remove visibility hooks once, with no fallback to preview art for qualified runtime families. |
| Relocation and proof conflict | B still has archived-plan copies and essential inputs under `docs/.../assets`, `logs`, review and build trees. A has migrated owners and stronger source/receipt checks. | Build an explicit path/hash/role map. Preserve historical receipts unchanged and publish relocation adapters/new proof separately. Port new consumers into the existing catalogue; never restore old caches as runtime authority or rewrite old receipts to appear current. |

## Implementation tasks

The live checkboxes below record completed batches; remaining work continues sequentially.

Code sketches are conceptual unless labelled as existing exports. Proposed helpers belong to the named existing owner. Verify prior exports before dependent work; an absent prerequisite stops only its dependent tasks, with safe independent work and forbidden shortcuts stated by the Area entry gate. Mandatory reading is non-transitive; do not preload future Areas. Native checks genuinely deferred until the merged target is buildable must be recorded and closed in [Plan4 Group60](m2o23-4-come-space.md#group-60---run-focused-domain-decoder-and-ordinary-world-checks). Existing focused native passes remain evidence and must not be relabelled as deferred.

### Area Mandatory Reading - Process harness and typed evidence adapters

_Applies to Groups 54-58._

Entry gate: Implemented domain owners, Group 46's complete state contract and Groups 52-53's candidate products must exist. Verify public definitions and role declarations before native preparation. Pure reader adaptation can continue if native prerequisites are absent. Never use an old executable, mutable prior receipt, alternate worktree or a second producer catalogue to make a missing dependency pass.

| File name | Line Ranges | Description |
| --- | --- | --- |
| [Shared run/input APIs](../../../../src/tests/multilayer/support/artifact_paths.py) | 371-396; 418-457 | `create_run_context`/`resolve_input_path` own contained outputs and retained input aliases; donor code must consume these existing boundaries. |
| [Current build identity](../../../../src/tests/multilayer/support/native_build.py) | 273-284; 381-437 | `verify_source_epoch`/`build_selected` reject added/removed/changed source inventory and publish exact selected executables. |
| [Memory guard](../../../../../OpenTTD3/src/tests/multilayer/guarded_native_build.py) | 52-99 | `build` limits one project, one worker default/two maximum, 6 GiB physical/8 GiB commit reserve and owned-process termination/reaping. |
| [Typed reference graph](../../../../src/tests/multilayer/support/artifact_recipes.py) | 19-131; 144-174 | `check_reference_roles` and `validate_selection` separate historical facts from live retained/generated/current-source/executable roles and reject undeclared dependencies. |
| [Current candidate/expectation handoff](m2o23-3-come-space-h-candidate-generation.md) | Group52 completion; Group53 completion; Plan4 entry APIs and limits | Actual71-node source/candidate graph, unchanged original hashes, five typed expectation protocols and ten configuration bindings. `checked_expectation_bundle` is the ready phase boundary; the packing ledger intentionally has no fabricated expectation nodes. Actual GRF input stays separate from expected pixels. |
| [Live complete canonical records](../../../../src/multilayer_state.h) | Live definitions; no fixed range | Group 46 extends `MultiLayerCanonicalRecord` and `ComputeMultiLayerStateDigest` with the declared merged family set. Report adapters must compare that live complete contract rather than a historical121/126-family subset. |

Context has compacted 1 times while executing this group.

### Group 54 - Adapt native runners and build memory guards

**ASSUMPTION: this was already built previously. If not, re-asses at runtime**

Required prior output: Group 3 fixes paths and Group 9 audited incoming executable helpers.

{{HARD}}
- [X] 54.1 Port donor runner/test-main integration through A's RunContext, isolated process homes, current-input observation and current executable identity checks.
- [X] 54.2 Integrate the donor one/two-worker and memory-reserve watchdog into A's existing selected-build owner; do not create a competing build entry point.
- [X] 54.3 Ensure timeout/failure reaps only owned children, keeps required transcripts and reports blocked prerequisites. Do not run native evidence during compilation or source edits during a native case.
- [X] 54.4 Test wrong-root/runtime escape, stale source inventory, reserved-memory crossing and child reaping using bounded focused fixtures; audit helpers for prohibited deletion first.

Group54result:17focused cases pass through the existing Python/native input owners and actual Windows child handles. Current record: `test-cache/merge-harness/group54-verification.json`. The real focused build was refused before creating a compiler because physical availability was5.1GiB, below the mandatory6GiB reserve. The new explicitly hidden arithmetic-fault diagnostic case is implemented but its compiled execution is deferred to60; no old executable supplies that check. See the [execution record](m2o23-4-come-space-a-execution.md#group54-completion-and-native-deferral).

Follow-up before56: memory recovered, the guarded current Debug Tube rebuild passed and the actual hidden arithmetic-fault child retained its original frame/source line, nonzero native exit, transcripts and reaped root. `test-cache/merge-harness/group54-arithmetic-fault.json` closes that deferral; the earlier blocked-attempt record remains unchanged.

### Group 55 - Translate historical and current evidence roles explicitly

**ASSUMPTION: this was already built previously. If not, re-asses at runtime**

Required prior output: Group 54 provides the current process provenance boundary.

```json
{
  "role": "generated_prerequisite",
  "path": "mlm/reviews/<feature>/<scenario>/<run>/data/<candidate>",
  "sha256": "<verified actual bytes>"
}
```

The live `check_reference_roles` also requires this exact path/hash in the supplied `generated_inputs` and the selected run in the path; annotation alone is not a prepared dependency. Use the existing result/catalogue schema for recipe/configuration metadata.

{{HARD}}
- [X] 55.1 Adapt donor normal-start, lifecycle, useful-route and performance readers using retained aliases plus explicit current-run inputs/outputs.
- [X] 55.2 Keep sealed historical receipts unchanged. Publish relocation observations as separate records, then fresh merged proof with actual current source/image identities.
- [X] 55.3 Require explicit generated-prerequisite recipe/run/configuration binding; diagnostic absence is allowed, required evidence absence is not.
- [X] 55.4 Add wrong alias/hash, stale executable, undeclared previous-run read and historical-as-current refusal cases. Do not weaken source/hash checks wholesale.
- [X] 55.5 Register disabled recipes for incomplete providers and enable only when their real APIs/inputs are supplied.

Group55result: the existing `prior_stage_evidence` owner now reads the four imported evidence scopes through explicit historical/current role contracts. Seven bounded transport checks pass; actual retained-root relocation observations are separate from originals. Current supplier request/declaration/source/output, current native command/build/configuration and every structured dependency remain bound. `test-cache/merge-evidence/group55-verification.json` records the scope. Transport verification explicitly grants no gameplay acceptance; the four sole-catalogue providers remain disabled for their56/63/65/66/68interfaces. Complete owner semantics and genuine original-world execution remain with the subsequent groups.

### Group 56 - Join legacy readers and complete canonical ledgers

**ASSUMPTION: this was already built previously. If not, re-asses at runtime**

Required prior output: Groups 40-46 define real legacy normalization and merged state; Group 55 fixes reference roles.

{{HARD}}
- [X] 56.1 Extend existing Python/native full/copied table readers for each admitted dialect and merged format, including nested TABLE semantics and tagged supply receipts.
- [X] 56.2 Use Group 46's explicit complete-family definition and independent cargo/energy/cash/RNG comparisons. Document legitimate legacy normalization separately from current state equality.
- [X] 56.3 Adapt exact normal-start original/source/command bindings; keep prepared worlds, genuine bootstrap and actual-peer evidence distinct.
- [X] 56.4 Add cross-reader mismatch cases where a partial digest agrees but an owner/custody/clock field differs; such a pair must fail.

Group56result:11focused cases pass. The sole Python TABLE reader now preserves nested fields, dense holes, unique sparse IDs, full extents and explicit native script VM tails. Public ground ledgers/equality compare every original owner and require Group46's exact128-family merged set, alongside independent cargo/cash/energy/clock/RNG channels. Original tagged receipt and native full/copied dialect owners remain the producing40–46implementations; original-to-merged normalization stays separate from zero-normalization merged equality. Current record: `test-cache/merge-evidence/group56-verification.json`. Pure/metamorphic checks do not replace60's consolidated native qualification or63's genuine bootstrap.

### Group 57 - Adapt selected art proof and candidate-runtime binding

**ASSUMPTION: this was already built previously. If not, re-asses at runtime**

Required prior output: Groups 52-53 supply independent candidate/expected products; Group 54 owns isolated runtime homes.

{{HARD}}
- [X] 57.1 Port donor selected native binding/gallery checks through A's explicit EvidenceSet/run/source contracts, retaining protected geometry/coverage fields.
- [X] 57.2 Add a contained candidate-baseset preparation adapter within the existing runtime preparation owner: install the selected candidate GRF and any required descriptor/hash companions only in that process home, then verify the exact bytes actually loaded.
- [X] 57.3 Record candidate GRF identity as an actual generated input in addition to the independent expected-payload identity. The expected-sprites environment alone is not proof of which GRF ran.
- [X] 57.4 Test wrong candidate, missing companion, changed frame/source, stale consumer and wrong configuration; require real native loader origin/pixel observations for current art acceptance.
- [X] 57.5 Avoid source→proof→source cycles: retained originals/contracts are roots; current native output never becomes its own source expectation.

Group57result:21focused transport/gallery/build guard checks pass. Actual Debug M1loads3561source records with60559assertions and independently hashes the actual opened16982461-byte SpriteFile to the selected candidate SHA256; a separate deliberate wrong-pixel expectation is rejected by that real decoder. `test-cache/merge-assets/group57-verification.json` binds source/build/process/candidate/expected roles. Canonical files remain unchanged. The earlier110-second preflight-only failure is retained separately; bounded `--stage verify` avoids repeating prepare in that same invocation while retaining all guards. Full five-family/dual-configuration final art acceptance remains61after59.

### Group 58 - Close the selected reconstruction graph without old caches

**ASSUMPTION: this was already built previously. If not, re-asses at runtime**

Required prior output: Groups 55-57 implemented current readers and candidate bindings.

```mermaid
flowchart LR
    O["media originals + independent fixtures"] --> C["Pure conversion / packing"]
    C --> P["Run-local candidates"]
    O --> E["Independent expected payload"]
    P --> N["Current native loader in isolated runtime"]
    E --> N
    N --> V["New verification and required evidence"]
    V --> S["Separate explicit source-publication task"]
    L["Optional logs"] -. "diagnostics only" .-> V
```

{{MEDIUM}}
- [X] 58.1 Update the sole recipe catalogue for the implemented source, expectation, loader, gallery and acceptance leaves with exact inputs/targets/tags and bounded dependencies.
- [X] 58.2 Verify selected Space continuation and donor acceptance inputs resolve with old build/log/review caches unavailable; retain unique inputs rather than disabling missing-input failures.
- [X] 58.3 Check no routine recipe calls canonical publication, mutates independent fixtures, replays closed Space financial work or depends on B's directories.
- [X] 58.4 Publish the architecture role-map deltas from actual implemented adapters for later documentation finalization.

Group58result:12focused cases pass, including actual selected Space verification and all7547donor payloads/20344aliases(1024023909bytes) behind an audit fence that forbids reads/scans from old generated caches and B. Both sole-catalogue input recipes complete through their real public owners. The82-node selected art graph has implemented enabled prerequisites and its two actual native targets; inspection is not a native pass. Current record: `test-cache/merge-reconstruction/group58-verification.json`. Actual roles are documented in `archi-historical-debug.md`. The old440-only native Space qualification API is explicitly disabled/pending64for its441/128-family adaptation; the actual chosen GZVEEJcontinuation and strict missing-input failures remain intact.

### Area Mandatory Reading - Consolidated build, focused suites and asset publication

_Applies to Groups 59-62._

Entry gate: Groups 45-58 must supply complete implementations, candidate assets, independent expectations and exact current input/build roles. Inspect the selected graph before running. Stop native qualification on any missing prerequisite; manifest/static checks may continue. No historical EXE, old canonical GRF substituted for a candidate, source-mutating regression recipe or bypassed source guard is allowed.

| File name | Line Ranges | Description |
| --- | --- | --- |
| [Current selected build](../../../../src/tests/multilayer/support/native_build.py) | 273-284; 381-437 | `verify_source_epoch`/`build_selected` define the exact epoch and executable receipts used by all subsequent runs. |
| [Selected recipe execution](../../../../src/tests/multilayer/support/artifact_rebuild.py) | 137-183; 489-624 | `load_recipe`/`prepare_recipe` gate actual suppliers and outputs. Select bounded leaves, not the whole historical acceptance tree. |
| [Source publication seam](../../../../media/multilayer/pack_m1_art.py) | 502-525 | `publish` writes canonical files and public metadata; call only in the explicit publication batch after candidate verification, with the analogous family owners. |
| [Donor selected publication guarantees](../../../../../OpenTTD3/media/multilayer/activate_m1_art.py) | 563-630 | `require_selected_gallery`/`publish_selected_runtime` preserve protected fields and require actual selected evidence. Adapt these checks rather than copying old completion flags. |

Context has compacted 2 times while executing this group.

### Group 59 - Freeze and build the merged source and candidate epoch

**ASSUMPTION: this was already built previously. If not, re-asses at runtime**

Required prior output: Group 58 exposes only implemented selected recipes and current candidate inputs.

Build scope adjusted from actual admission evidence: two monolithic `openttd_test` attempts crossed the protected physical-memory reserve. The selected scope now uses `openttd`, the focused `openttd_merge_contract_test` (13 existing Classic/geometry/A-Space source files), and the eight existing supply/capital/processor/ground-art/ground-consumer/ground-viewport/Tube/Space-art targets in each configuration. Original general-test registrations remain available; no regression run or memory-policy relaxation is authorized by this adjustment. Exact failed attempts and current build identities belong to `test-cache/merge-build/` and the separate build receipts.

{{MEDIUM}}
- [X] 59.1 Close every donor manifest entry as integrated/equivalent/historical/generated or explicitly unresolved; verify duplicate symbols, stale B paths, CMake registrations and enabled recipe prerequisites.
- [X] 59.2 Freeze the complete source/art candidate epoch and configure through A's portable profile; build selected game/test targets for Debug and RelWithDebInfo.
- [X] 59.3 Use one native build project, one worker default/two maximum, owned node-reuse controls and 6 GiB physical/8 GiB commit reserves. Record compilation separately from tests.
- [X] 59.4 Validate resulting binary/source/tool identities; a build failure invalidates dependent native qualification without rewriting earlier historical receipts.

Group59result: both current configurations complete all10selected targets, with common source-epoch SHA55696da28413b80cc814adbd161983d8313c62aa76a755456cac5625a16d646f, verified current images/profile/11tool executables and unchanged candidate/independent expectations. Minimum physical availability is8090435584bytes optimized and7957291008bytes Debug; all owned roots are reaped and reserves remain unchanged. Sixteen focused build guards and two binary/text preservation cases pass. `test-cache/merge-build/group59-verification.json` binds the current receipts, conservative2453-entry manifest review and separate seven failed build attempts. The1739explicitly unresolved path differences remain visible; this build result grants no native gameplay/art acceptance. See the [execution record](m2o23-4-come-space-a-execution.md#group59-completion).

### Group 60 - Run focused domain, decoder and ordinary-world checks

**ASSUMPTION: this was already built previously. If not, re-asses at runtime**

Required prior output: Group 59 provides exact current executables and the complete selected component inputs.

{{MEDIUM}}
- [X] 60.1 Run bounded supply/purpose/refund/capacity, face/PBS, generator/conversion, fleet/provider, lineage/wire and canonical-state suites for the frozen epoch.
- [X] 60.2 Run Classic/disabled-profile and A Space construction/manufacturing/replacement/cargo controls in that same epoch; passing donor-only cases is insufficient.
- [X] 60.3 Record exact images, inputs, commands, terminal results and deferred cases from earlier component batches. Close those deferrals only with actual passing results.
- [X] 60.4 Fix reported failures at their original owners and rerun affected boundaries; after source changes refresh the epoch and avoid repeating unrelated historical suites.

Group60result:34successful current-image native processes cover962component case executions/5726090assertions and four actual A440/B426full/copied/save-reload observations. Every merged successor pair preserves the exact128-family contract. The focused contract target now includes15existing sources and passes59nonhidden cases per configuration, including all nine actual Space replacement controls. The stale vehicle/face-phase expectations and copied-preview component boundary are corrected at their original owners; native corruption reporting retains every original predicate/message. Both configurations share final source-epoch SHAe280c7e0ab761b828753107516b896ffc0ffe6243e80803828a73ff0fb0ad464. `test-cache/merge-build/group60-verification.json` binds exact current builds/images/inputs/commands/transcripts, closing source/tool checks and separate failures. A110-second combined face burst was reaped and split into four leaves with the same59unique cases; no build accompanied that split. No protected Space tick/spending or genuine paid-world/peer/performance acceptance is claimed. See the [execution record](m2o23-4-come-space-a-execution.md#group60-completion).

Context has compacted 1 times while executing this group.

### Group 61 - Verify candidate assets through the actual native loader

**ASSUMPTION: this was already built previously. If not, re-asses at runtime**

Required prior output: Groups 53, 57 and 59 supply exact independent inputs and candidate-capable current binaries.

{{HARD}}
- [X] 61.1 Materialize candidate GRF/descriptor companions into isolated runtime homes via Group 57 and bind independent expected payloads; verify the actual loaded file identity.
- [X] 61.2 Run M1/Space/orbital/launch/Ground and imported tube/geothermal loader/selector cases against original pixel/geometry expectations.

Group61native result: all40current native processes pass in the frozen`merge61-ground-readers-20261024-01`phase:272case executions/2049654assertions, including ten actual opened-candidate decoders, eight source-selector processes and22imported selector bursts. Exact candidate SHA c04fbf0d5525edc53250cfbd963f5e3bbe0d0bce655a3553c73b76483a8e8744, current images/source/process witnesses and1459unchanged phase media/catalogue identities are sealed in`test-cache/merge-assets/group61-native-verification.json`, SHA d356412bcb0276cba3315db5faafb043c4df3811ebf05323cf0970a2c174adfa. Source/reader failures remain separate. No viewport or later-source/current-image bridge is claimed. The discovered capture handoff repair and original61.3/61.4tasks continue sequentially in61B; all stable task IDs remain unchanged.

### Area Mandatory Reading - Capture prerequisite handoff and native frames

_Applies to Group61B._

Entry gate: Group61's native phase is sealed with exact source/image/process identities. All four launch/Ground capture preflights refuse before C++at the same shared cross-run prerequisite check. Repair the original role/dispatcher owner; a caller-supplied run name or complete flag cannot authorize old generated data. Explicitly validate the completed pure supplier, exact recipe/role/run and current same-build selector before allowing those precise prerequisites in a fresh capture run. Source changes invalidate current native reuse under existing guards; refresh the necessary selected art source/build/native boundaries before real captures. No domain regression, publication, old-image promotion or paid progression is authorized here.

| File name | Exported symbols | Description |
| --- | --- | --- |
| [Live reference roles](../../../../src/tests/multilayer/support/artifact_recipes.py) | `check_reference_roles` | Exact generated prerequisites, current outputs, retained inputs and historical facts must remain distinct. |
| [Actual dispatcher](../../../../src/tests/multilayer/support/artifact_recipe_stage.py) | `execute_request`, `launch_occupied_viewport_stage`, `ground_viewport_stage` | Registered/enabled declarations, exact input/output roles and adapter identity remain the sole execution gate. |
| [Supplying graph](../../../../src/tests/multilayer/support/artifact_rebuild.py) | `snapshot`, `verify_passed_stage`, `verify_current_build` | Recheck current source/declarations and actual passed pure outputs; never fabricate native nodes in the pure ledger. |
| [Current selectors](../../../../src/tests/multilayer/space_source_review.py) | `checked_result`, `checked_request` | Real decoder/candidate/process/configuration/source chain, not a self-declared success flag. |
| [Launch captures](../../../../src/tests/multilayer/launch_viewport_review.py) | `execute_prepared`, `checked_request`, `execute_stage` | Exact approved original pixels/ownership and actual zero-tick native frames/transcripts. |
| [Ground captures](../../../../src/tests/multilayer/ground_viewport_review.py) | `execute_prepared`, `checked_request`, `execute_stage` | Independent complete rasters and source pixels, current native consumer and real process/frame provenance. |
| [Current build](../../../../src/tests/multilayer/support/native_build.py) | `verify_source_epoch`, `build_selected` | Preserve source inventory/image checks and bounded incremental builds; compilation and native execution remain separate. |

Context has compacted 1 times while executing this group.

### Group 61B - Close capture handoff and native viewport evidence

Required prior output: Group61 supplies genuine qualified candidate bytes. The original native phase remains immutable; refreshed observations must identify their actual new source/build epoch. This split isolates the newly discovered shared capture-input integration from the completed loader/selector work.

User-directed reduced scope: capture one actual Debug launch matrix and one Debug Ground collector example. Group61's completed dual-configuration loaders/selectors remain closed at their own epoch; do not repeat optimized copies of these prepared viewport scenes for a transport-only repair. Final ordinary-gallery coverage remains69. Preserve the interrupted broader queue separately and qualify only the actual selected capture chains.

{{HARD}}
- [X] 61.3 Capture selected native viewport/state examples and validate real consumer, source, frame and required transcript identities; keep prepared art evidence distinct from paid-useful-route proof.
- [X] 61.4 Verify routine runs left canonical media/manifests unchanged. Retain explicit failed attempts separately; do not relabel an old gallery as current.

Group61Bresult: actual current Debuglaunch and Groundcollector capture chains pass, including36complete occupied launch views and all original Ground raster/source/provenance checks. All1459protected canonical media/catalogue identities remain unchanged. `test-cache/merge-assets/group61b-capture-verification.json`, SHA22838759923f65f36699e308314449ddd459a51bce6b4dac18551e6579c9896b, binds the exact selected source/build/native/selector/frame outputs. Frozen61proof, failed builds/preflights and intentionally interrupted broader queue remain separate; no optimized duplicate capture, ordinary-game/human acceptance or paid progression is claimed.

### Area Mandatory Reading - Explicit canonical publication

_Applies to Group62._

Entry gate:61Bmust close real current capture provenance, and61must expose exact accepted native candidate bytes. Inspect the original publication owners' current-proof paths before writing; their historical private cached formats cannot be relabelled current. Normal recipes remain candidate-only.

| File name | Exported symbols | Description |
| --- | --- | --- |
| [M1 publication](../../../../media/multilayer/pack_m1_art.py) | `publish`, `bind_packed_payload` | Existing canonical source/public packing owner; preserve all unrelated source and runtime fields. |
| [Append publication](../../../../media/multilayer/pack_space_art.py), [orbital](../../../../media/multilayer/pack_orbital_art.py), [launch](../../../../media/multilayer/pack_launch_fleet_art.py) | `publish` | Publish only exact accepted family PNG/NFO/header ranges and original metadata boundaries. |
| [Ground/container binding](../../../../media/multilayer/tools/ground_payload_binding.py) | `prepare`, `publish` | Existing shared-container/prefix metadata owner; adapt explicit current evidence without rewriting historical proof. |
| [Donor guarantees](../../../../../OpenTTD3/media/multilayer/activate_m1_art.py) | `require_selected_gallery`, `publish_selected_runtime` | Actual selected evidence precedes publication; protected fields and separate human/future-family gates remain intact. |
| [Epoch and input boundaries](../../../../src/tests/multilayer/support/native_build.py), [selected reconstruction](../../../../src/tests/multilayer/support/artifact_rebuild.py) | `verify_source_epoch`, `build_selected`, `load_recipe`, `prepare_recipe` | Guarded source/publication changes require fresh actual receipts and only affected observations; never reuse an old EXE as current. |

### Group 62 - Publish the verified canonical asset bundle explicitly

**ASSUMPTION: this was already built previously. If not, re-asses at runtime**

Required prior output: Group 61 qualified exact candidate bytes and Group 60 passed the relevant component boundary.

{{MEDIUM}}
- [X] 62.1 Review the accepted candidate manifest and frozen preserved ranges; confirm all selected loader/pixel/source-binding checks passed for the exact candidate bytes.
- [X] 62.2 Publish the consistent canonical PNG/NFO/header/GRF/hash and public/runtime metadata set explicitly. Use existing family publication owners for differing bytes; if every accepted output and public binding already matches, publish an idempotent adoption receipt without rewriting identical files or invoking obsolete private-cache paths. Keep authored originals and independent expectations unchanged.
- [X] 62.3 Verify published bytes match the accepted candidates and that future normal regeneration remains candidate-only. No unrelated old or future family's status is promoted.
- [X] 62.4 Publish a new publication receipt; keep historical source/candidate/native receipts intact. If canonical publication changes a guarded source input, refresh and rerun only the affected observation required by its existing contract.
- [X] 62.5 Verify ordinary runtime preparation now resolves the published canonical bundle, then freeze the final acceptance epoch for subsequent gameplay/performance checks.

Group62result: explicit idempotent adoption verifies17identical outputs (five PNG/NFO/header sets plus complete GRF/checksum), preserves all109packed/39deferred public families and confirms both ordinary runtime preparations use the accepted canonical container. No canonical write, source change, legacy private-publisher invocation or additional build occurs. Exact publication`mlm/reviews/multilayer-art/merged-art-candidates/merge62-adopt-f6fef1980ecf/reports/publication.json`, SHA370aa92ca84a9993d229441a830c571c4cba98e9d2d65038bbe199918e0fed14; frozen build/source pointers`test-cache/merge-assets/group62-publication.json`. Normal generation remains candidate-only; human/ordinary-gallery approval stays pending. The first observation's incorrect assumption about original-base descriptor checksum semantics is retained separately and corrected without touching any shipping file.

### Area Mandatory Reading - Legacy migration and protected Space qualification

_Applies to Groups 63-64._

Entry gate: Group 62 must provide a final current epoch and Group 60 must close selected component failures. Verify original hashes and current-image identity. Stop runtime qualification on mismatch; read-only matrix/report preparation may continue. Do not use prepared-only proof, an old binary or a rewritten expected ledger as current acceptance.

| File name | Line Ranges | Description |
| --- | --- | --- |
| [Space foundation API](../../../../src/tests/multilayer/acceptance/space_checkpoint.py) | 114-160 | `load_foundation`, `verify_foundation`, `export_foundation`, `publish_successor` preserve exact lineage and separate new qualification from original seals. |
| [Protected current Space boundary](../../../../docs/zz-reach2/architecture/archi-historical-debug.md) | 394-424 | The original seal, selected GZVEEI successor and next-task/funding invariants forbid replaying closed windows or buying capacity during merge qualification. |
| [Current original-world scope](../../../zz-rhr/imports/openttd3-75ea75718f/docs/zz-reach2/upgrades/2026-10/r2mlvs16-a-normal-start.md) | 67-94 | `Exact closed handoffs` and `Execution limits and remaining scope` identify genuine originals and bounded execution. Later technical closure remains authoritative over stale pending-status prose. |

### Group 63 - Qualify each original lineage and new successors

**ASSUMPTION: this was already built previously. If not, re-asses at runtime**

Required prior output: Groups 39-46 implemented reader/normalization contracts and Group 62 froze the acceptance epoch.

Reduced validation scope follows the user's latest merge/testing direction: run current representative Debugfull/copy/save/reload/first-tick observations for the shared, A and B grammars, plus the four exact donor B425launcher originals and focused current ambiguity/capability/preflight controls. Keep the completed38-46version-boundary and ten original-derived128-family first-tick results at their own frozen epochs instead of repeating their complete matrix. Reports must distinguish current observations, unchanged earlier component proof, explicit source-field normalizations and documented refusals; no historical image is run or relabelled current.

```python
# Conceptual matrix; bind each case to an actual retained original hash.
for dialect in ("pre_fork", "legacy_a", "legacy_b", "merged"):
    for mode in ("preview", "full_load", "save_reload_first_tick"):
        check_case(dialect, mode, expected_normalizations[dialect])
# Cases compare all declared owners, not historical fixed chunk counts.
```

{{HARD}}
- [X] 63.1 Run representative current full/copy/preview and save/reload/first-tick checks covering pre-fork originals, A425-440, B425-434 and merged successors, using recorded original hashes and retaining unchanged completed version-boundary proof rather than rerunning its full matrix.
- [X] 63.2 For ambiguous originals use only the explicit digest-bound origin contract or produce its documented refusal; verify a converted successor then opens normally with its own marker.
- [X] 63.3 Compare exact preserved cash/cargo/geology/RNG/owner history, with named dialect normalizations separated from byte equality. Keep original files immutable.
- [X] 63.4 Run representative malformed/mixed-lineage cases and verify preflight rejection precedes world initialization; copied preview remains nonmutating.
- [X] 63.5 Publish migration receipts through existing retained/current evidence roles, not by changing historical receipt hashes.
- [X] 63.6 After qualifying the exact retained B425 TBM demos, adapt `play.ps1` entries `tbm-work`, `tbm-plan`, `tbm-exit` and `tbm-tunnel` through A's retained-input/runtime owners. Preserve all current Space entries and menu identities. Use verified merged successors or the explicit digest-bound origin path; never point the current launcher blindly at ambiguous425 bytes. Until this gate passes, retain A's working403 entries and record the donor face-exchange descriptions as pending.

Group63result: seven current Debugoriginal-world processes qualify shared403, A438, B434 and all four exact B425TBMdemos. Three first-tick pairs and all paused successors compare every128family; four focused native preflight/capability cases pass12048assertions. Earlier completed version-boundary/first-tick matrices remain separately frozen. Existing preservation owners retain23new save aliases as10payloads; Windows publication uses non-overwriting atomic rename, avoiding per-save temporary deletion on success. Exact migration receipt`mlm/reviews/space/machinery-lineage-original/merge63-migration-be1eb94cd113/reports/migration.json`, SHA4c960134ce0423f29a8a7a6e2a9b245fa52280679b9929b688f5d8e3e171d3f4. All four launcher dry-runs resolve retained, self-describing441successors; original403/425inputs and named Space choices remain intact. `test-cache/merge-assets/group63-launcher-verification.json`records the frontend check. These component-original observations claim no genuine paid normal-start or human acceptance. Refresh the build epoch for64after the retention-tool source change; do not rerun closed art/domain suites.

### Area Mandatory Reading - Selected Space merged qualification

_Applies to Group 64._

| File name | Sections / symbols | Description |
| --- | --- | --- |
| [Protected current Space boundary](../../architecture/archi-historical-debug.md) | `Checkpoints, cleanup and transfer`,430-462 | Exact GZVEEIfoundation, unchanged original seal, closed cash/tick limits and GZVEEJdispatch. Earlier394-424navigation is stale. |
| [Public foundation owner](../../../../src/tests/multilayer/acceptance/space_checkpoint.py) | `load_foundation`, `verify_foundation`, `qualify_selected_foundation` | Qualify the actual successor separately; no replay, promotion of an older seal or next-task completion. |
| [Paused native owner](../../../../src/tests/multilayer/acceptance/space_checkpoint_load.py) | `selected_checkpoint`, `compare_loaded_saves`, `checkpoint_observation` | Typed selected source, exact current process, complete128-family equality and neutral metadata normalization. |
| [Migration comparison](../../../../src/tests/multilayer/acceptance/space_checkpoint_migration.py) | `compare_migration` | Explicit per-field440to441rules; original EIDS/ENGN/vehicle history cannot change. |

Context has compacted 1 times while executing this group.

### Group 64 - Requalify the exact protected Space continuation

**ASSUMPTION: this was already built previously. If not, re-asses at runtime**

Required prior output: Group 63 qualifies the A lineage and Group 62 supplies the exact current epoch.

{{HARD}}
- [X] 64.1 Load A's exact selected GZVEEI foundation in both configurations with current public APIs and verified original/current hashes.
- [X] 64.2 Run the permitted zero-tick native and ordinary observation pair; compare complete state, material/cargo ownership, finance, RNG and current next-task dispatch. Apply only the explicit per-field structural normalizations qualified by Group 63; an EIDS definition append must not hide a change to existing vehicles or gameplay quantities.
- [X] 64.3 Preserve both original seals and the existing GZVEEJ usefulness gate; no closed earning window, gameplay purchase or capital spending is part of this qualification.
- [X] 64.4 Publish a distinct merged-build qualification/foundation result under existing owners without treating it as completion of the next Space development task.

Group64result: both current configurations pass the exact selected original's paused native migration and the ordinary441successor restore. Every128family, full original owner/material/finance observation, clocks and RNG pass explicit structural rules with zero ticks. Separate qualification`mlm/reviews/space/checkpoint-qualification/merge64-qualification-2010a9a281/reports/merged-foundation.json`, SHA76b594cb243d0aa83d9a504bcd3acd2448579e7ecbeeaa584e5400fe37f8deee. Original8cc7foundation and5083seal remain byte-exact; GZVEEJis still next and grants no reopened cash/work scope. Existing retention preserves8new aliases as2payloads/62416bytes. No human acceptance or next-Space-task completion is claimed. The old disabled original-seal recipe remains a distinct pending70recovery adapter; the implemented current public qualification APIs above supply64.

### Area Mandatory Reading - Genuine paid ground operation and real peers

_Applies to Groups 65-67._

Entry gate: Groups 63-64 must preserve original worlds and qualify the current epoch. Verify original-world and component prerequisites before native operation. Stop a dependent scenario on absent evidence; pure scenario comparison may continue. Prepared stock, cash assignment, teleportation and two standalone reloads may not replace paid-world or real-peer proof.

| File name | Line Ranges | Description |
| --- | --- | --- |
| [Genuine normal-start report](../../../../../OpenTTD3/src/tests/multilayer/normal_ground_acceptance_report.py) | 78-162 | `accept` joins actual native commands, original worlds, complete ledgers and art bindings; retain this public acceptance boundary. |
| [Actual peer runner](../../../../../OpenTTD3/src/tests/multilayer/ground_network.py) | 86-194 | `run_one` uses separate server/client homes, actual join acknowledgments, joined snapshot and child reaping. Adapt its hard-coded434/current hashes to explicit merged inputs without dropping these checks. |
| [Technical versus human closure](../../../zz-rhr/imports/openttd3-75ea75718f/docs/zz-reach2/upgrades/2026-10/r2mlvs16-b-performance.md) | 9-16 | `Current technical finalization,2026-10-20` distinguishes delivered technical behavior from pending art/demo feedback; no automated scenario invents that approval. |

### Group 65 - Replay genuine paid work and useful delivery

**ASSUMPTION: this was already built previously. If not, re-asses at runtime**

Required prior output: Groups 19, 23, 36 and 63 provide the qualified owner contracts.

{{HARD}}
- [X] 65.1 Select the minimum actual original-world sequences covering Standard/Deep paid clearing, recurring face and bay logistics, and the settled irregular original for final conversion in65.3.
- [X] 65.2 Use actual canonical purchased supply delivery, useful tube freight and installed geothermal participants; retain original native finance and material accounting. Bind genuine paid-world operations and separately labelled native installed-component checks; do not claim they are one world or replace the paid originals with prepared components.

Group65result: four exact genuinely financed originals pass current Debugmigration/full native operation, complete128-family saved equality and all13expense categories. One new8Steelcanonical order costs800, dispatches through the real station scheduler, acknowledges the same-intent retry and permanently refuses refund. The genuine useful route completes on its existing32768-tick allowance; the earlier16384-tick loaded wait is retained, not passed as complete. Selected public acceptance`mlm/reviews/acceptance/normal-ground-merge/merge65-selected-5b6437ffb8/reports/paid-continuations.json`, SHA9dfd27274249cc968a20b2af985deb8bbac2b9f85cd12e1f284998ca87355e1d. Current installed-geothermal component burst passes3cases/859assertions in one process; separate report`mlm/reviews/acceptance/merged-ground-geothermal/merge65-geothermal-14ce77eee8/reports/installed-geothermal.json`, SHA67570436a8992b03f25f6661a450fd5d8d191e0799a84ecacb992e739f022969. Its declared arrival/setup is not genuine bootstrap or driven arrival.18save aliases/9payloads are retained. The newly discovered callback port is complete; remaining recovery/conversion/publication work is split below without changing task IDs or repeating closed native cases.

### Area Mandatory Reading - Original wait recovery and final conversion

_Applies to Group 65B._

| File name | Sections / symbols | Description |
| --- | --- | --- |
| [Current genuine original owner](../../../../src/tests/multilayer/acceptance/normal_ground_merge.py) | `select_original`, `migrate_original`, `execute`, `accept` | Four completed original continuations and their actual ledgers/processes remain frozen at the65epoch; prepared installed participants stay separate. |
| [Native first-frame replay](../../../../src/tests/save_first_tick_fixture.h) | `CheckFirstNativeTick` | Reuse full native save/reload/first-frame observation for the actual retained useful wait; never assign time, money or cargo. |
| [Original final-access commands](../../../../../OpenTTD3/src/tests/normal_ground_access_fixture.h) | `BuildNormalGroundAccess` | Retire settled work, convert each actual temporary support and open the real portal through public commands; retain the current irregular world's complete history instead of hard-coded original36-tile totals. |
| [Existing native recovery boundaries](../../../../src/tests/transport_tube_recovery.cpp) | Native test cases | Run only named full/depleted/power/path recovery boundaries required here, retaining their component classification beside genuine wait proof. |

### Group 65B - Finish original wait recovery and final conversion

Required prior output:65's exact settled irregular output and genuine loaded useful wait, retained alongside the separately labelled installed-component proof. Add only missing recovery/conversion observers; do not rerun the four completed paid-world sequences or their original migrations for an appended test callback.

{{HARD}}
- [X] 65.3 Exercise full destinations, source depletion, blocked portals, shortage waits and actual recovery; save/reload representative genuine waits and execute the selected irregular world's final public retirement/support/portal conversion. Keep component boundaries separately labelled.
- [X] 65.4 Publish complete original-world command/ledger/graphics bindings; keep component/prepared worlds separately labelled.

Group65Bresult: the genuine irregular successor retires/converts its actual support and portal through public invoiced commands with zero ticks and unchanged cargo/clock/RNG. The retained genuine useful wait replays the same full128-family first native frame after reload. Separate depot/power component invocations pass12cases/459021assertions and retain five actually observed capacity/path/origin waits. Initial depot failure was an unflushed native signal queue in the older component frame driver; only that fixture was repaired, and only components reran. Exact phase comparison confirms no other source changed between the two completed genuine callbacks and repaired component build. Publication`mlm/reviews/acceptance/normal-ground-recovery-publication/merge65b-final-cd1d7fe68a/reports/recovery.json`, SHA534d179dd9ac46c397d1de38be13fc1dfccbef03d4c71339e6a732f66b2ed785. Existing retention preserves13aliases/9payloads, including the failed component's unique save. The genuine/component/current-image roles remain separate. Public original/recovery APIs are implemented; sole-catalogue stage wrappers remain a70reconstruction integration task, without replaying closed65proof now.

Context has compacted 1 times while executing this group.

### Group 66 - Verify cross-feature contention and lifecycle conservation

**ASSUMPTION: this was already built previously. If not, re-asses at runtime**

Required prior output: Groups 65 and 64 provide both feature-side baselines; Group 35 owns the combined allocation.

{{HARD}}
- [X] 66.1 Run a bounded same-world case containing launch/orbital demand, paid ground supply, geothermal generation/town service and active tube freight.
- [X] 66.2 Compare the one-pass request/settlement ledger at matched native ticks with visible versus hidden layers and stable owner ordering.
- [X] 66.3 Inject bounded power/path loss and one relevant company/station/portal/industry removal/reuse case; verify once-only spending/refunds and retained consumed history.
- [X] 66.4 Keep manual stops and waiting orders intact across recovery; report exact violated owner/counter if a case fails rather than broadening the scenario.

Group66result: one focused Debuginvocation passes7cases/12381assertions. Two258-frame worlds compare every tagged supply receipt, actual packet/account ledger and combined synchronized domain at matched native ticks with Deep freight visible versus Space visible. One canonical4Steelorder pays once, same-intent retry costs zero, actual station dispatch fulfills it and refund is refused; loaded freight moves while manual service remains stopped. Existing native contention/path/source/interval/company-reuse cases preserve once-only settlement, waits and consumed history. New component teardown releases actual station packet containers before pool cleanup; it fixes isolated-world packet-ID leakage without changing production behavior. Prepared construction/payload inputs remain explicit, separate from65genuine originals. Report`mlm/reviews/acceptance/combined-ground-orbital/merge65b-2067cb008762/reports/combined-scheduler.json`, SHAb4dec6eabbb0cfb970362c347ad13e4de1f622e3ea63e223797ecfdf0cecc035.

### Group 67 - Prove actual join, resume and synchronized commands

**ASSUMPTION: this was already built previously. If not, re-asses at runtime**

Required prior output: Group 47 fixes the ABI and Group 66 provides the current combined-world comparison.

{{HARD}}
- [X] 67.1 Run merged host/client processes in separate runtime homes with actual network join acknowledgments and a joined snapshot; reap owned processes on every exit.
- [X] 67.2 Cover active face transfer, loaded/queued external/geothermal tube work and thermal conversion at matched ticks. Compare the complete canonical state and clock/RNG inputs; retain the original prepared geothermal-binding component's classification beside the genuine paid originals.
- [X] 67.3 Exercise synchronized supply/provider commands and resume after saved waits; verify command estimates and retries do not double-settle on peers.
- [X] 67.4 Attempt incompatible original A/B protocol join packets against the actual host and verify WrongRevision before authentication, map transfer or mixed command execution. Bind exact current source/binary/content identities; no archived executable is required.

Group67result: five real separate-home host/client pairs pass10native cases/69108assertions and30matched checkpoints, each covering128complete native families plus pure consecutive writes. Genuine retained active-face, loaded external Tube, canonical supply and thermal-riser worlds remain separate from the retained prepared geothermal-binding component. Both actual original-protocol handshakes refuse WrongRevision before authentication. A cold dedicated load exposed a real owner-context defect: `GetTubeTerminal(tile,company)`now runs its pure construction readiness check with the explicit validated company, then restores the global actor. Spectator/OWNER_NONEafterload and all-company ticks no longer misclassify owned utility cells as foreign; original ownership/date/floor admission remains. Only that affected geothermal pair reran after this production fix; earlier completed phases retain their actual epochs. Public report`mlm/reviews/acceptance/merged-ground-peers/merge67-final-32759f4f32/reports/actual-peers.json`, SHAedc0ae6da980a9315033ff1d2e4057efe9a9c15dd9673e346b32a029c177ebf3. Existing retention preserves140aliases/66payloads, including failed attempts. No native processes remain.

### Area Mandatory Reading - Performance, visible behavior and cold reconstruction

_Applies to Groups 68-70._

Entry gate: The final epoch and acceptance records from Groups 62-67 must be current or have explicitly scoped blockers. Verify the selected recipe/input identities. Stop dependent measurements/review on mismatch; offline review selection may continue. Historical timings, old galleries, cache copies or recursive cleaning cannot stand in for a current observation.

| File name | Line Ranges | Description |
| --- | --- | --- |
| [Performance comparisons](../../../../../OpenTTD3/src/tests/multilayer/ground_performance_acceptance.py) | 87-123; 139-214 | `compare_budget` and `accept` bind actual workload/baseline identity and strict artifacts; use comparable maps/configurations without relabelling history. |
| [Reconstruction lifecycle](../../../../docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup3.md) | 85-103; 247-254 | The end-state contract and bounded recipe roles require selected prerequisite reconstruction without earlier workstation history or implicit publication. |
| [Current foundation verification](../../../../src/tests/multilayer/acceptance/space_checkpoint.py) | 114-145 | `load_foundation`/`verify_foundation` are the public recovery boundary for the retained continuation; a package copy alone is not a qualification. |

### Group 68 - Measure bounded combined workloads and recovery cost

**ASSUMPTION: this was already built previously. If not, re-asses at runtime**

Required prior output: Groups 60-67 supply the current accepted build/scenarios.

{{MEDIUM}}
- [X] 68.1 Run existing Classic/disabled, active ground, dense job, utility/tube and visible/hidden workloads against the final production epoch using comparable baseline data; retain each actual producing test image separately.
- [X] 68.2 Measure actual load/reconstruction, first resumed work and real-peer join cost with the existing reporters; keep build time separate and label the measured whole-pair join upper bound conservatively.
- [X] 68.3 Attribute significant regressions to named phases/owners and block only the affected acceptance rows until explained or corrected.
- [X] 68.4 Record exact machine/configuration/workload/source identity and current budget comparisons; do not claim old branch timing as a merged result.

Group68result:7selected native cases/112926assertions pass every unchanged frozen budget, with zero violations. Four map observations retain128warmup/640measured ticks each: Classic128p990.1722ms, idle multilayer1280.3281ms, genuine active64visible0.6697ms/hidden0.7165ms. Current Windows lifetime private peaks are34.7-36.5MiB. Actual dense512/128jobs, moving Tube512/4sources/4capsules/32jobs and sparse utility0/32/256/4096accounts complete. Source comparison proves no production timing owner changed across the burst; only missing test-target registrations and failed-selection reporting changed, so completed observations were not repeated. Load/first-resumed-work and synchronous saves are measured;67's actual geothermal peer whole interval supplies a conservatively labelled join/reconstruction upper bound. Report`mlm/reviews/performance/merged-acceptance/merge68-final-a91d92b3b5/reports/selected-performance.json`, SHAb348dcd341a971977b6445fdbf993e8e44ab4c10bb39379c89ae85b6e06db5c5.16save aliases/6payloads are retained. No FPS, hard-map-maximum or whole-allocator acceptance is claimed.

Context has compacted 1 times while executing this group.

### Group 69 - Review the final native gallery and ordinary controls

**ASSUMPTION: this was already built previously. If not, re-asses at runtime**

Required prior output: Group 62 published verified canonical assets and Groups 65-67 retained runtime proof.

{{MEDIUM}}
- [X] 69.1 Capture final merged runtime representatives for Space/orbital/launch, conventional ground, all five capsules and geothermal construction/working/wait states.
- [X] 69.2 Verify zoom/direction/state/occlusion/click geometry against retained approved sources and current native consumers; compare published bytes with the candidate proof where applicable.
- [X] 69.3 Exercise minimap/layer/hotkeys, supply cancellation, TBM guidance and provider controls in ordinary player flows.
- [X] 69.4 Record human art/demo questions separately from technical passes; preserve outstanding feedback until actually supplied.

Group69result:44selected native cases/21331assertions pass. Exact retained loaded shipments for all five capsules, three original paid geothermal stages and the protected Space/orbital/launch continuation produce repeated two-zoom rasters with complete128-family draw purity and zero gameplay ticks. Conventional rail/road observers additionally compare499native pose frames against unchanged original complete raster and source expectations; their art-only staging remains explicit. Six geometry and27actual player-control cases cover picking, occlusion, refund history, provider selection, TBM advice/change-work, minimap/layers and hotkeys. CanonicalGRFc04remains unchanged. Public gallery`mlm/reviews/multilayer-art/merged-final-gallery/merge69-final-160ddb9804/reports/gallery.html`; report`final-gallery.json`SHAa98dc6d6001f05459abd047759c20a71db2d6a6633c921f661e5a70d7a6aa279.36save aliases/9payloads retained. User-selectedAart for Battery/Electronics is recorded; remaining overall art/demo feedback stays pending.

### Group 70 - Rehearse selected recovery without historical caches

**ASSUMPTION: this was already built previously. If not, re-asses at runtime**

Required prior output: Group 58 defines the selected graph and the final current epoch/retained inputs are available.

{{MEDIUM}}
- [X] 70.1 Prepare an isolated project-owned destination through existing package/input declarations without deleting source trees or invoking a recursive cleaner.
- [X] 70.2 Reconstruct representative Space and underground source/expected/native recipes from declared originals; keep old logs/reviews/build directories unavailable to readers.
- [X] 70.3 Verify original/current continuation and selected assets resolve through public APIs with no archived EXE dependency, no source-mutating regeneration and no entire-history rebuild.
- [X] 70.4 Preserve any new unique saves via existing retention helpers and publish an exact reconstruction receipt; archive creation does not authorize cleanup.

Group70result:five selected sole-catalogue recipes pass in fresh`test-cache/cold70-0cf615`: original Spacevehicle15/Groundcoal source and independent approved-pixel/physical-pose comparisons, actual selected SpaceFoundation native plus ordinary zero-tick restore, genuine Standardoriginal migration and genuine useful-wait first-frame replay. Existing CMake/materialization owners reconstruct basesets, both required English packs and first-party content; no old generated distribution or historical game EXE is required. Only explicitly checked fresh linked images are transferred; no cold compiler rebuild is claimed. The embeddedPythonpath escape, missing standard extension and incomplete runtime setup attempts remain failed diagnostics, separate from the successful five runs. Debugcurrent buildSHAfdb0db93488e463f51d416e0477d44acbc4fd99cf2933165a38dac7ee7c7a3e9; optimizedTube/applicationSHA d36be8088be78d1d52ca7fd63e283ba09565643d5ae382632852ebdee13561c9. Receipt`mlm/reviews/reconstruction/merged-selected/merge70-final-df4dbac471/reports/cold-recovery.json`, SHAe7f0650d7dfbeaf9552558748ac14a5b43831871d56d0c446bc28431e1a66e38.14newsave aliases/5payloads retained. The sole catalogue now has104implemented enabled recipes/3explicitly superseded disabled entries.

### Area Mandatory Reading - Architecture finalization and preservation handoff

_Applies to Groups 71-72._

Entry gate: Current merged receipts must exist, or exact blockers must remain recorded. Verify source/image/input bindings before changing feature status. Documentation may continue with honest planned/incomplete labels; completion stops on missing proof. Historical checkboxes, another repository's result and successful archive copying cannot bypass current acceptance.

| File name | Line Ranges | Description |
| --- | --- | --- |
| [Canonical layout source](../../../../docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup3.md) | 85-103; 230-237; 247-254 | `End-state storage contract`, candidate-versus-publish rules and sole bounded recipe catalogue constrain the final architecture and imported assets. |
| [Existing architecture to finalize](../../../../docs/zz-reach2/architecture/archi-historical-debug.md) | 30-53; 55-128; 226-279 | The storage table, `Parallel-repository originals`, `Graphics candidates, expectations and publication`, and `Historical facts and current proof` must describe actual merged owners and receipts after Group 3's initial contract update. |
| [Durable successor publication](../../../../src/tests/multilayer/acceptance/space_checkpoint.py) | 126-160 | `export_foundation`, `verify_foundation`, `publish_successor` keep originals and current qualification separate; do not create a merge-only checkpoint store. |
| [Outstanding human scope](../../../zz-rhr/imports/openttd3-75ea75718f/docs/zz-reach2/upgrades/2026-10/r2mlvs16-b-performance.md) | 9-16 | The technical-finalization handoff must be revisited so imported closure cannot silently approve remaining human art/demo feedback. |

### Group 71 - Finalize architecture and reconcile feature documentation

**ASSUMPTION: this was already built previously. If not, re-asses at runtime**

Required prior output: Group 3 established the layout contract and Groups 62-70 provide current implementation/proof.

{{SIMPLE}}
- [X] 71.1 Update the architecture edits from Group 3 with actual imported asset paths, producer names, native candidate binding and explicit publication/recovery commands. Remove only obsolete planned labels that now have proof.
- [X] 71.2 Update map-layer/savegame/code maps for lineage, tagged supply receipts, face exchange, fleet/provider ownership, tick order and final art ranges; keep references to the one storage architecture owner.
- [X] 71.3 Reconcile active Plans3/4/6/7/12/14/15/16/17/18 and master dispatch against merged results while retaining A Space/cleanup and B technical closure as dated history.
- [X] 71.4 Update both walkthroughs/current review entry points and keep GZVEEJ and remaining feature scope separate from merge acceptance.
- [X] 71.5 Validate current local links, external-donor historical references and exact public-symbol names. Current recovery must work without B's generated directories.

Group71result:the [integration handoff](m2o23-4-come-space-b-integration-handoff.md) indexes exact source/art/build epochs, current continuation, public recovery/publication boundaries, gameplay owners and outstanding human scope. Architecture/save/code maps, ten active feature plans, masters, both player walkthroughs and current review entry points now reflect the merge while retaining dated original checklists/receipts. Both donor-deleted guides are explicitly preserved as historical/compatibility navigation. Focused documentation check`test-cache/merge-assets/group71-verification.json`passes22current documents/62current links/12public exports, verifies127unchanged imported-document hashes and595explicitly historical donor targets. No native test, build or source/publication change occurred.

### Group 72 - Validate preservation and publish the integration handoff

**ASSUMPTION: this was already built previously. If not, re-asses at runtime**

Required prior output: Group 71 describes the actual merged system and all applicable acceptance/reconstruction rows have evidence.

{{MEDIUM}}
- [X] 72.1 Verify every retained input/asset/save against original hashes/aliases and confirm B's tree and A's original Space seals remain unchanged; state the version 424 alias disposition.
- [X] 72.2 Publish current merged continuations/qualification and migration receipts through existing foundation/retention owners, verifying recovery without old executables or undeclared caches.
- [X] 72.3 Audit every original task disposition and single-owner rule: supply, cargo, fleet, utility, lineage/capabilities, asset identity, recipe catalogue and materialization.
- [X] 72.4 Report exact source/art/build epochs, qualified lineages, current continuations, technical blockers and pending human feedback. No Git write, checkout, commit, push, recursive deletion or hosting cleanup is implied.

Group72result:final read-only preservation/owner audit passes7547retained donor payloads,5340save payloads/16781current aliases,20335physical source/input/current-image files and all2453original path dispositions with completed assigned owner groups. BHEAD/tracked tree/recorded untracked status and A's originalSpace seal/foundation are unchanged. The journal retains every historical payload while each current alias selects its latest committed version; its pre70aliases/payloads remain append-only. Exact A384/B424ground-connected inputs remain separate hash-bound aliases; old contradictory historical receipt hashes are not rewritten or adopted as current inputs. Existing retention resolves both qualified441Space outputs without oldEXEs. Four intentionally absent donor production files are replaced by the one canonical supply/cargo/save owners, with no duplicate stock/command book. Receipt`mlm/reviews/acceptance/merge-preservation/merge72-final-67ec1ab9f0/reports/integration.json`, SHA5299114386c446c4687d23f6382cd913bf14259443e3bbb4b3223c3816c58059. All four plans are complete:107groups/381tasks; [final handoff](m2o23-4-come-space-b-integration-handoff.md). Human art/demo feedback and SpaceGZVEEJretain their separate scope. No Git write, deletion or cleanup occurred.

## Consult on Demand

| Trigger | File name | Start at | Expand only when | Consultation purpose |
| --- | --- | --- | --- | --- |
| A migrated supply case cannot be reconciled with actual quote/refund behavior. | [B paid Surface execution notes](../../../zz-rhr/imports/openttd3-75ea75718f/docs/zz-reach2/upgrades/2026-10/r2mlvs6-b-paid-surface-supplies.md) | `Remaining design obligations`, lines31-37 | Current B code/tests and original save fields do not explain the entitlement in question. | Explain original cap, custody and refund intent. This historical note cannot override the confirmed A rule for new purchases or create missing provenance. |
| A migrated tube model fails cargo/date or useful-route admission despite intact EIDS identity. | [B tube runtime handoff](../../../zz-rhr/imports/openttd3-75ea75718f/docs/zz-reach2/upgrades/2026-10/r2mlvs17-e-tube-runtime.md) | `Completed15FEE actual Forge raster and five-model closure`, lines15-34 | The public model/terminal/readiness APIs do not explain the observed refusal. | Identify the original model/route prerequisite, not a substitute registry or permission to relax admission. |
| Geothermal source-switch or local service loses conserved lineage. | [B local electrical-feed handoff](../../../zz-rhr/imports/openttd3-75ea75718f/docs/zz-reach2/upgrades/2026-10/r2mlvs12-d-local-electrical-feed.md) | `Group9C closure and return to Plan21-10BB.2,2030-14-17`, lines9-16 | The live conversion/link tests leave an unresolved physical-path ownership question. | Explain original same-layer feed versus riser/town ownership; shared utility accounts stay authoritative. |
| A retained donor original or publication dependency is absent after relocation. | [B normal-start handoff](../../../zz-rhr/imports/openttd3-75ea75718f/docs/zz-reach2/upgrades/2026-10/r2mlvs16-a-normal-start.md) | `Exact current files and operation harness`, lines53-60 | The named original/receipt's direct references fail to identify the missing input. | Find the exact original producer/alias. No cache search result may silently replace its hash identity. |
| A Space regression proposes more paid work or a replay of closed earning windows. | [A launch-center source handoff](r2mlsp-z-z-z-z-z-q-launch-center-source.md) | `Current boundary` | The precise next-task/closed-policy contract cannot be resolved from that section and the retained foundation. | Resolve existing Space progression limits; a merge/test failure does not grant further spending or change the chosen task. |
| Reconstruction requests a removed old image, log or source epoch. | [A cleanup execution record](r2fc-file-cleanup3-a-execution.md) | `Group12 - Typed completed qualification and current-build boundaries`, lines585-641 | The live role/capability failure specifically requires an earlier migration disposition. | Recover why a dependency is historical, retained or generated. Do not rebuild the old cache hierarchy or weaken current-source checks. |

## Planning Provenance

This is **not an execution reading queue** and must **not be preloaded after context compaction**. It is **non-transitive**. Ranges record planning use rather than runtime authority. These retained rows document the original decomposition relevant to this file; the four-plan partition preserves that design and moves no feature authority.

| File name | Sections or Line Ranges used while planning | Planning contribution |
| --- | --- | --- |
| [A cleanup continuation](r2fc2-file-cleanup3.md) | `Checkpoint and resume point`,44-61; Group37 result,908-938 | Established the destination's completed preservation/reconstruction investment and exact resumed Space boundary; justified direction and early donor-input preservation. |
| [A cleanup execution record](r2fc-file-cleanup3-a-execution.md) | Heading inventory; `Checkpoint: paused after Group20BB`,2828-2853 | Located the staged historical source/art work and distinguished paused provisional underground material from completed current proof. |
| [A storage architecture](../../architecture/archi-historical-debug.md) | `Delivery boundary`,10-28; `Storage ownership`,30-76; `Historical facts and current proof`,151-164; `Checkpoints, cleanup and transfer`,317-349 | Defined retained/generated ownership, catalogue authority, immutable seals and the current Goods-query continuation used throughout sequencing. |
| [B underground master](../../../zz-rhr/imports/openttd3-75ea75718f/docs/zz-reach2/upgrades/2026-10/r2mlvs3mp-master-plan.md) | Group/area heading inventory,431-720; latest linked technical returns | Located the tube/plant/runtime/lifecycle/ground acceptance progression and decomposed the incoming feature families. Status comes from current code and bounded returns, not the entire master's checkbox count. |
| [B normal-start handoff](../../../zz-rhr/imports/openttd3-75ea75718f/docs/zz-reach2/upgrades/2026-10/r2mlvs16-a-normal-start.md) | `Current return`,5-18; headings and original-file/closed-handoff sections,49-90 | Identified genuine S1/S2 proof, current434 decoding and independent original-world evidence that must not be relabelled as prepared-fixture acceptance. |
| [B performance handoff](../../../zz-rhr/imports/openttd3-75ea75718f/docs/zz-reach2/upgrades/2026-10/r2mlvs16-b-performance.md) | `Current technical finalization`,5-12; `Earlier same-day technical snapshot`,13-20 | Established delivered technical closure, actual receipt identities, pending human feedback and the version 424 alias discrepancy. Prevented copying stale intermediate blocker claims. |
| [B guarded build](../../../../../OpenTTD3/src/tests/multilayer/guarded_native_build.py) | `build`,52-99; read together with normal-start `Execution limits and remaining scope`,84-90 | Established the later one/two-worker and memory-reserve constraints, superseding older four/eight-worker advice without replacing A's portable build owner. |
| [B selected-runtime handoff](../../../zz-rhr/imports/openttd3-75ea75718f/docs/zz-reach2/upgrades/2026-10/r2mlvs14-b-selected-runtime-integration.md) | Current84E and completed84D,5-20; heading inventory | Established current selected art, protected source/range fields and the need to port publication guarantees without reviving historical cache ownership. |
| [Cleanup layout and candidate contract](r2fc-file-cleanup3.md) | `End-state storage contract`,85-103; implementation limits,118-126; Groups4/6,230-271; source candidates,503-519; Ground/expectation/native-input boundaries,744-800 | The user's explicit follow-up made artifact placement and pure generation versus canonical publication a first-class integration contract. Added a dedicated existing-architecture update and split candidate generation, expectations, native proof and publication. |
| [A native loader](../../../../src/saveload/saveload.cpp) and [network reader](../../../../src/network/network_client.cpp) | `SlLoadChunk`,2294-2340; `SlLoadCheckChunk`,2347-2391; `FileReader`,2495-2530; `DoLoad`,3293-3451; `PacketReader`,43-85 | Located pre-InitializeGame classification, raw-reader reset versus codec recreation, and copied/full failure boundaries. Replaced broad compatibility work with separate framing, entry, capabilities and family batches. |
| [Pure/public packing seams](../../../../media/multilayer/pack_m1_art.py), [evidence selector](../../../../media/multilayer/evidence.py), [native payload binder](../../../../src/tests/multilayer/support/sprite_native_inputs.py) | `selected_ranges`,66-157; `derive/publish`,437-525; `selected`,203-217; `output/validate_capture`,256-312; binding/observation,17-75 | Established protected ranges and the distinction between expected-payload identity and actual candidate GRF identity. Added isolated candidate runtime binding and separate explicit canonical publication. |

Read-only Git histories/path/blob inventories supplied the original chronology. The user's planning format, no-Git-write/no-recursive-delete rules, supply-policy answer and four-plan partition govern execution; they add no transitive reading queue.

## Completion Conditions

Technical integration is complete only when all applicable tasks across Plans1-4 have evidence, every incoming change has an explicit disposition, and no feature-preservation or compatibility blocker is hidden as a documentation exception. The merged tree must retain A's Space/runtime/reconstruction work and B's delivered underground/runtime work through one set of canonical owners. The existing storage/graphics architecture must document the actual imported asset roles, and selected reconstruction must obey R2FC-3's candidate-versus-publication boundary.

Both original save lineages must load correctly or receive an explicit, demonstrated ambiguity refusal with their originals preserved and a documented conversion path. Qualified originals require successful full/copy/load/reload and merged-successor checks. New supply purchases use Pending-only refunds; legacy refund rights neither disappear nor grow. Existing cash, cargo, geology, RNG, utility lineage and retired identities remain conserved across migration and runtime.

The selected Debug/RelWithDebInfo build, real-peer, meaningful normal-start/useful-route, performance and art checks must be bound to the merged source/assets. Historical branch receipts alone cannot meet this condition. Cold recovery must resolve through A's declared durable inputs and public producers without B's caches or historical executables.

Keep the current Space dispatch and closed gameplay policies intact. Outstanding human art/demo feedback remains explicitly pending until actually supplied; technical acceptance does not fabricate that feedback. Neither source repository is deleted or reset, no recursive deletion occurs, and no Git write is performed without the human's explicit request.
