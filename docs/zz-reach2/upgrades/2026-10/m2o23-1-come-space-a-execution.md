# M2O23-1 execution record

**Current routing,2026-10-23:** [Plan1](m2o23-1-come-space.md) is complete through22D; [Plan2](m2o23-2-come-space.md) owns23-36, [Plan3](m2o23-3-come-space.md)37-53 and [Plan4](m2o23-4-come-space.md)54-72. Earlier entries' Part1/Part2 labels describe the earlier two-file scope; resolve their unchanged group IDs through this current map. The four-plan partition entry at the end records conservation checks. Implementation remains stopped before23.


Owner: [merge plan](m2o23-1-come-space.md). Architecture: [historical inputs and reconstruction](../../architecture/archi-historical-debug.md).

This record describes observed execution, not a new runtime catalogue. Temporary audit data and tools live under `test-cache/merge-inventory/`; durable originals live in the existing fixture/media owners. Donor B remains read-only. No Git writes, deletions, full game builds or gameplay have occurred. Focused standalone compilation/test results are recorded in their groups below.

## Group 1 — Pinned inventory, 2026-10-23

All five tasks are complete. Both HEADs and the common ancestor match the plan:
A `cfeff84db52f8a3753a43c2a770a6363c62b2c67`,
B `75ea75718fa7d14632bf795105721df1e390f753`,
base `9a47768ae3e4455661264f2c49febde972538e49`.
Initial status in each repository was only the untracked plan. B's plan is still empty. A's active plan is the intentional working-file difference. History was read with optional locks disabled and a process-only alternate object path; no Git configuration or object store was changed.

The reproducible inventory is `test-cache/merge-inventory/inventory.json`; its Group 1 SHA256 is `5ea829d29e26e2280e78a4937a25e7687877972a44cfbf4e68bfe897779f2855`. Rebuild using `python -B test-cache/merge-inventory/freeze_inventory.py`. The tool is disposable observation tooling and must remain with its report under test-cache. It records full base/A/B tree blobs, no-rename and rename-aware deltas, every changed donor path's disposition, exact equivalent destination paths, per-repo plan mtime order and commit/path chronology, ignored/untracked donor paths, original hashes and explicit aliases. The two pinned source repositories supply the original inputs; this audit does not certify a merged executable.

| Observation | Verified result |
| --- | --- |
| Baseline changes | A 12,470; B 2,453; 115 shared paths; 114 different final blobs |
| Shared production paths | 47, individually routed below |
| Initial donor dispositions | 1 equivalent; 290 semantic adaptations; 1,119 historical relocations; 1,041 candidate imports; 2 unresolved deletions |
| Exact bytes already at another A path | 688 donor delta rows have destination aliases; use those identities before making duplicate copies |
| Hidden donor paths | 73,515 enumerated; 15,865 save/config candidates; 17,744 receipt/contract candidates |
| Hidden non-originals | 124 bytecode products; 278 protected-neighbor paths excluded from import scope; other products need dependency-driven inspection |
| Hidden source candidate | Only B's empty plan; no new ignored authored source was found under src/media/docs |
| Protected A input hashes | 6,951 media/fixture/catalogue files; 949,865,692 bytes |
| Original/current Space seals | Both match the plan's exact SHA256 values |
| Four selected B receipts | Target completion, genuine bootstrap, selected guidance and fixture retention all match their handoff hashes |

The single comment/whitespace-only production difference is `src/multilayer_sprite.h`. Its diff was inspected: A's mine/processor/wagon documentation must survive while B's delivered tube/geothermal control documentation is joined in Group 50. Do not replace it with B's older descriptions.

B deletes `docs/zz-reach2/upgrades/2026-10/d3sp-space-survey.md` and `r2mlvs-how-to.md`. Both still exist in A. They are explicitly unresolved documentation dispositions for Group 71, not deletion instructions. Preserve them.

### Same-purpose boundaries not detected by a same-path merge

| A authority | B contribution | Integration boundary |
| --- | --- | --- |
| `surface_supply_order.{h,cpp}`, runtime/command and wire owners | `surface_supply.{h,cpp}`, self-debit command and TABLE save family | Groups 6, 10-16 and 41: tagged legacy receipt in A's single owner; keep Pending-only new refunds |
| `surface_supply_order_sl.cpp` SSOR RIFF | `surface_supply_sl.cpp` SSOH + SSOR TABLE | Groups 4-5 and 38-44: explicit dialect, never duplicate SSOR handlers |
| `tube_cargo_capacity.*` | B tube station transfer-room helper | Groups 17-18: one capacity policy, adapt callers |
| A's complete station/hub/pod/module/Space cargo traversal | `cargo_custody.*` | Groups 12-13: shared visitor must include A's newer containers |
| A's Space/orbital/launch/ground pack owners and published ranges | B's expanded M1 pack and selected runtime gallery | Groups 8, 48-53 and 57-62: logical families, remapping, candidates, native observation, explicit publication |
| `RunContext`, retained input aliases and `mlm/review-producers.json` | B logs-based consumers/receipts and guarded native runners | Groups 2, 54-58: retain original identity, adapt roles, never restore a second cache/catalogue |
| A's orbital manufacturing/recipe demand appenders | B's generators, conversion, town/local feed and providers | Groups 27-36: one allocation and deterministic settlement pass |

### Shared production routing

Every row remains a semantic reconciliation unless the comment-only exception above applies. The listed groups identify owners, not permission to execute them out of order.

| File | Ownership boundary | Groups |
| --- | --- | --- |
| `src/CMakeLists.txt` | build registration | 9 |
| `src/autoreplace_cmd.cpp` | fleet replacement admission | 32 |
| `src/cargopacket.cpp` | cargo purpose and custody | 12, 13, 17, 18, 19 |
| `src/cargopacket.h` | cargo purpose and custody | 12, 13, 17, 18, 19 |
| `src/command.cpp` | command ABI and kernel | 7, 14, 25, 47 |
| `src/command_type.h` | command ABI and kernel | 7, 14, 25, 47 |
| `src/economy.cpp` | non-paying purchased delivery | 17, 35 |
| `src/engine.cpp` | native fleet definitions | 7, 31, 32 |
| `src/excavation.h` | excavation and TBM | 20, 21, 22, 23 |
| `src/excavation_cut.cpp` | excavation and TBM | 20, 21, 22, 23 |
| `src/fios.h` | detached same-file previews | 40, 41, 42, 43, 45 |
| `src/fios_gui.cpp` | file controls and lineage preview | 24, 39 |
| `src/geothermal_heat.h` | geothermal machinery and provider | 25, 26, 29, 30, 33, 36 |
| `src/lang/english.txt` | ordinary player guidance | 24, 37, 69 |
| `src/layer_portal_cmd.h` | portal command contract | 22 |
| `src/layer_portal_work.cpp` | portal work settlement | 22, 23 |
| `src/multilayer_bootstrap.h` | bounded demo bootstrap | 24 |
| `src/multilayer_sprite.cpp` | renderer and logical assets | 48, 49, 50, 51, 52, 53 |
| `src/multilayer_sprite.h` | renderer and logical assets | 48, 49, 50, 51, 52, 53 |
| `src/multilayer_state.cpp` | complete canonical state | 46, 56 |
| `src/network/network_command.cpp` | network command ABI | 47 |
| `src/saveload/CMakeLists.txt` | save compatibility | 38, 39, 40, 41, 42, 43, 44, 45 |
| `src/saveload/afterload.cpp` | save compatibility | 38, 39, 40, 41, 42, 43, 44, 45 |
| `src/saveload/cargopacket_sl.cpp` | save compatibility | 38, 39, 40, 41, 42, 43, 44, 45 |
| `src/saveload/engine_sl.cpp` | save compatibility | 38, 39, 40, 41, 42, 43, 44, 45 |
| `src/saveload/multilayer_sl.cpp` | save compatibility | 38, 39, 40, 41, 42, 43, 44, 45 |
| `src/saveload/saveload.cpp` | save compatibility | 38, 39, 40, 41, 42, 43, 44, 45 |
| `src/saveload/saveload.h` | save compatibility | 38, 39, 40, 41, 42, 43, 44, 45 |
| `src/saveload/vehicle_sl.cpp` | save compatibility | 38, 39, 40, 41, 42, 43, 44, 45 |
| `src/space_vehicle.cpp` | shared custody visitor; retain newer Space behavior | 13, 17 |
| `src/station.cpp` | station, launch and cargo lifecycle | 13, 16, 36, 50 |
| `src/station_cmd.cpp` | station removal and renderer hooks | 16, 36, 50 |
| `src/surface_supply.cpp` | canonical supply order book | 6, 10, 11, 14, 15, 16 |
| `src/surface_supply.h` | canonical supply order book | 6, 10, 11, 14, 15, 16 |
| `src/surface_supply_cmd.h` | canonical supply order book | 6, 10, 11, 14, 15, 16 |
| `src/table/sprites.h` | renderer and logical assets | 48, 49, 50, 51, 52, 53 |
| `src/tbm.h` | excavation and TBM | 20, 21, 22, 23 |
| `src/tbm_exchange.cpp` | excavation and TBM | 20, 21, 22, 23 |
| `src/tbm_exchange.h` | excavation and TBM | 20, 21, 22, 23 |
| `src/transport_tube_engine.cpp` | tube fleet and network | 18, 31, 32, 33, 34, 36, 37 |
| `src/transport_tube_power.cpp` | tube fleet and network | 18, 31, 32, 33, 34, 36, 37 |
| `src/transport_tube_runtime.cpp` | tube fleet and network | 18, 31, 32, 33, 34, 36, 37 |
| `src/utility_power.cpp` | utility allocation and settlement | 27, 28, 30, 34, 35, 36 |
| `src/utility_power.h` | utility allocation and settlement | 27, 28, 30, 34, 35, 36 |
| `src/vehicle.cpp` | whole-consist cargo and caches | 21, 22, 32 |
| `src/vehicle_cmd.cpp` | fleet refit and retirement | 21, 32, 36 |
| `src/window.cpp` | layer-aware window controls | 24, 37 |

### Reading and preservation notes

Global and Area reading was completed at entry. The architecture link for `archi-map-layers.md` is under `architecture/map-layers/`, not directly under `architecture/`. Named headings and APIs are authoritative if line ranges drift. The existing source prefix, comment standards, no-deletion rule and non-transitive reading protocol remain active.

The following exact seals were rechecked:
- `src/tests/fixtures/checkpoints/space/checkpoint.json`: 5818 bytes, SHA256 `5083a1e8c64359f06eb3f9913032ad8b25b7ba5fea73d2c5ad16519985928df9`.
- `src/tests/fixtures/checkpoints/space/continuations/5d8ca01b552778970bde977259f25037e935d3c87de779803e221280d398252c/reports/foundation.json`: 182252 bytes, SHA256 `8cc7f8743c2922537d49b1745641049692a3c297a1e1d72208526748c76a1f6e`.

No game test was needed for this read-only inventory. Focused verification checked pinned histories, all disposition coverage, the 47 routing rows, both deletion entries, literal-aware comment comparison and original receipt/seal hashes.

## Group 2 — Exact donor inputs and separate aliases, 2026-10-23

All five tasks are complete. The preservation contract is [donor-import.json](../../../../src/tests/fixtures/cleanup/donor-import.json), with an explicit original path list for every byte identity. It is an audit/retention record, never a second runtime or asset catalogue. Existing [multilayer-inputs.json](../../../../src/tests/fixtures/multilayer-inputs.json) remains the sole native fixture alias resolver. Donor aliases begin `donor/openttd3-75ea75718f/` and leave all 20,899 original A entries unchanged.

The import retained 7,547 payloads and 20,344 original aliases, totaling 1,024,023,909 bytes including reused inputs. It created 4,531 exact copies totaling 310,759,113 bytes and added 19,931 native fixture aliases. Authored media aliases remain in the preservation record and their existing media owners, outside the native save resolver. No canonical shipping asset was replaced.

| Retained role | Payloads | Durable owner |
| --- | --- | --- |
| Original saves/scenarios | 4,221 | Existing exact A fixtures where available; otherwise `savegames/retained/openttd3-75ea75718f/` |
| Runtime configuration originals | 53 | `src/tests/fixtures/config/multilayer/retained/openttd3-75ea75718f/`, or exact existing configuration owner |
| Unique source epochs | 2,833 | Existing `src/tests/fixtures/cleanup/source-epochs/` with a donor epoch subdirectory for new bytes |
| Authored/selected media, palette and geometry originals | 412 | Existing exact media where available; competing originals under `media/multilayer/source/legacy/openttd3-75ea75718f/` |
| Independent existing fixture inputs | 14 | Existing fixture owners or `src/tests/fixtures/legacy/openttd3-75ea75718f/` |
| Selected historical receipts and named art ancestry | 14 | `src/tests/fixtures/checkpoints/underground/openttd3-75ea75718f/historical/` |

Immutable copied source/JSON bytes retain their original comments and identities. The containing preservation contract supplies architecture/plan provenance; changing historical headers would destroy the exact originals being preserved. New live verifier/test functions have documented arguments, owner headers and business-logic comments.

### Connected-world alias correction

Direct `OTTX` header inspection corrects the original planning assumption:

| Original | Version | Bytes | SHA256 | Merged resolution |
| --- | --- | --- | --- | --- |
| A `src/tests/fixtures/acceptance/ground-connected.sav` | 384 | 9,132 | `fce011e9093ad91b052b03517132319c14c48086ecb3c7d1c5d501e416d385c7` | Existing A key and bytes unchanged |
| B at the same relative path | 424 | 9,632 | `0f10d887bdd07140b614fbc16b83a926481ac5092a22ea4f5daa7c41e8f6a6d7` | Namespaced donor key resolves to `savegames/retained/openttd3-75ea75718f/0f10d887bdd07140-ground-connected.sav` |
| B clean-checkout alias of that path | 384 | 9,132 | `fce011e9093ad91b052b03517132319c14c48086ecb3c7d1c5d501e416d385c7` | Separate donor alias reuses A's exact original |

Both `src/tests/multilayer_acceptance-1.py` readers use a direct repository-relative Path, so A selects its version384 bytes and B selects version424. B's historical fixture catalogue still declares the version384 hash for the changed direct path; that stale assertion is not repaired by relabelling either payload. Future imported consumers must select an explicit namespaced key or exact hash. The ambiguous storage identity is resolved by preserving both; native compatibility remains Groups 38-47/63.

A's `acceptance_ground_fixture.cpp` still writes the authoritative fixture path. B changed this to `GetTestCaseArtifactPaths(...).Output("saves", ...)`. Group 9.5 now explicitly requires that safe routing before any execution of `[acceptance-ground-fixture]`. This generator has not been run.

### Selected dependency trace and proof limits

The disposable trace followed 465,676 concrete edges from the selected target-completion, genuine-bootstrap and selected-guidance receipts. It distinguished exact originals from generated native reports, images, atlases/payloads, executable images and optional diagnostics. The durable record keeps selected direct-reference facts and exact named metadata ancestry. It does not archive 3,498 generated historical reports. Full trace/review proposals remain under `test-cache/merge-inventory/{preservation-plan,retention-plan}.json`.

Mutable donor defaults have advanced beyond the selected-guidance receipt: default evidence, packed metadata and the public sprite manifest no longer match that older receipt's hashes. Their exact named predecessor metadata is retained alongside the current source originals. Historical runtime-config paths are also often gone. These observations are dated facts, not failing current qualifications: Groups 48/55/57 must choose named exact inputs and produce fresh merged proof, never relabel default paths or restore a historical executable. Required current native terminal/capture transcripts belong in the selected run's `evidence/`; optional diagnostics alone belong in `logs/`.

### Focused verification and next dispatch

`python -B src/tests/multilayer/donor_input_checks.py` passed six tests in 0.034 seconds: no donor-directory dependency, byte tampering, alias rebinding, wrong owner, wrong namespace and scope separation. `python -B src/tests/multilayer/verify_donor_inputs.py --output test-cache/merge-inventory/retained-input-verification.json` verified all 7,547 retained payloads and 20,344 aliases in under one minute. It reads only A. For bounded repeat checks use `--scope saves`, `sources`, `assets` or `history`; no game or broad regression runs are involved.

The existing catalogue's original 100 recipes are unchanged. The new `donor-underground-inputs` declaration passes selected schema validation and remains explicitly disabled pending Group 58's selected-graph adapter. The direct preservation verifier is implemented now; disabled recipe status does not claim its stage adapter already exists. A's original Space seal still has its exact original hash, and B still reports only its untouched untracked empty plan.

## Group 3 — Existing architecture updated, 2026-10-23

All five tasks are complete. `archi-historical-debug.md` now records the actual donor retention roles and namespaces under `Storage ownership`, plus `Parallel-repository originals` and `Graphics candidates, expectations and publication`. The new flow names existing `derive`, explicit `publish`, expected-payload binding, native loader and `EvidenceSet` boundaries. It distinguishes the candidate GRF actually loaded from an independent expected-payload hash. Donor adapters and Group 58's catalogue stage remain explicitly planned.

`archi-map-layers.md` links this storage contract from `Generated graphics and drawing` without taking over asset identities or gameplay rules. Its historical sprite counts are identified as dated snapshots; current headers and packing contracts remain authoritative. Two broken historical log links were changed to the corresponding dated source-plan handoffs, without claiming fresh proof. The active merge plan's affected Mandatory Reading navigation ranges were refreshed; Planning Provenance continues to record the ranges actually used during original planning.

All local links in the four edited documents resolve, and one retained payload from each of the six roles was independently rehashed after the documentation update. No native test was appropriate for this documentation-only group. Next dispatch is Group 4, whose save-format Area reading replaces the retention area; it must not preload unrelated historical reading queues.

## Group 4 — Original save framing characterized, 2026-10-23

All four tasks are complete. [The save-lineage handoff](m2o23-1-come-space-b-save-lineage.md) is the narrow contract for Group 5. It records versions424-440, the reserved future441 marker proposal, required RIFF/TABLE pairs, exact CAPA extension columns and MLMD feature bits. No current save version or writer changed.

The read-only `save_lineage_inventory.py` uses the existing transport parser and retained A storage. It sampled81 original framings and recorded8 decompressor refusals rather than hiding damaged negative fixtures. The durable `src/tests/fixtures/saveload/lineage-originals.json` selects27 representative observations plus6 missing-family negative originals. All27 exact SHA256 identities, sampled mandatory family kinds/lengths and44 handoff links passed focused verification. This is transport characterization, not native gameplay acceptance.

Evidence corrects two assumptions that matter to the classifier: empty/common-layout ambiguity extends beyond425 into A426-430, and B's empty supply/generator/conversion/geothermal families are still mandatory at their introduction versions. Their absence cannot prove A. SDEF3 is positive A evidence from431; Classic0 is neutral. The version439 capability is source-defined, but no retained A439 original was found; do not manufacture a downgraded save to conceal this gap.

The next Group 5 implements detached pure types/results and capability lookup. It must keep invalid marker/family evidence distinct from absence, bind any ambiguous-origin hint to all32 exact digest bytes, and leave current serialization untouched until Group44. The proposed merged RIFF `MLLN` payload is8 bytes (`MLN1`, schema1, envelope version441); activation and complete tagged-supply writing remain later. No live world, filename or cached previous load may choose lineage.

## Plan split and Group 5 — Pure classification, 2026-10-23

At the user's request the plan was split into [Part 1, Groups 1-36](m2o23-1-come-space.md) and the then Part2, Groups37-72 (now [Plan3,37-53](m2o23-3-come-space.md) and [Plan4,54-72](m2o23-4-come-space.md)). Stable global numbering preserves references in existing handoffs and code. The split preserved all314 tasks exactly once, including19 completed tasks at that point:159 tasks in Part1 and155 in Part2. Each file has its own verbatim execution protocol, nearest-Area coverage, optional consultation/provenance and scoped completion conditions. Group37 now has an explicit command-control handoff gate. Local links, all72 numbered groups, difficulty placement and task limits passed a structural check. No gameplay task was completed by splitting the file.

After the first context compaction during Group5, the Global and save-classification Area were reread. The current map-layer invariants, R2FC storage/protected-Space boundary and audited lineage handoff remain authoritative. A's enum navigation shifted to489-506 after the public include; the plan's Area range was corrected. Actual load-entry integration is still deferred, so no broad reread or loader execution was needed.

All four Group5 tasks are complete. `save_lineage.h/.cpp` provides detached evidence with absent/valid/malformed states, conservative A/B ambiguity, all32 digest-byte origin binding, required-family/type/count/flag joins and explicit refusal results. `saveload.h` re-exports the boundary. Version441 remains reserved only; the current enum/writer is unchanged. The merged contract requires later canonical supply/CAPA validators to provide affirmative evidence; an old decoder or marker alone cannot assert it. Ordinary pre425 numeric gates retain their existing meaning.

Focused verification: the standalone `src/tests/save_lineage_policy.cpp` Catch leaf compiled with MSVC C++20 `/W4 /WX` and passed5 cases/831 assertions in under a second. Expectations explicitly name all26 historical capability thresholds rather than reproducing implementation ordinal arithmetic. Cases cover both lineages, all affected versions, wrong/all-zero digest, malformed/missing pairs, contradictory content, forged/incomplete merged marker and unknown features. This is pure policy evidence; it does not claim native save loading. CMake registration belongs to Group9, bounded inspection/loading to Part2 Groups38-44.

Rebuild with `vcvars64.bat` initialized, then `cl /nologo /std:c++20 /EHsc /W4 /WX /DSAVE_LINEAGE_STANDALONE /Fe:test-cache\merge-lineage\save_lineage_test.exe /Fo:test-cache\merge-lineage\ src\tests\save_lineage_policy.cpp src\saveload\save_lineage.cpp`; run that executable with `[save-lineage] --reporter compact --out test-cache/merge-lineage/test-results.txt`. Compilation diagnostics belong in `test-cache/merge-lineage/compile.log`; no permanent result depends on those disposable files. Next dispatch is Group6's receipt-design contract under the nearest Supply/command/asset Area.

## Group 6 — Lossless receipt design, 2026-10-23

All five design tasks are complete. [The receipt contract](m2o23-1-come-space-c-supply-receipts.md) specifies the shared fields and explicit native/legacy variants, original B tariff/date/refund/dispatch/closure facts, source-specific day rules,16384 new-order admission versus65535 complete-history bound, conservative legacy retention and thirteen independent conversion/refusal examples. Groups10-16 implement these contracts later; no runtime order or refund changed here.

Direct code inspection found an additional ABI requirement: B's cancellation transports `expected_quantity`, while A's existing command has only its uint64 intent. Routing both through the unchanged A tuple would lose B's stale-count check. Group7 now allocates an append-only checked legacy command into A's existing book/transaction owner; Group15 implements it with command-kernel Other settlement. The native A command keeps its original tuple and refuses legacy receipts. This preserves both behaviors without a second order store or double credit. The second plan's Group37 control gate was updated accordingly.

Validation was source/schema review against both actual order validators, original wire fields, A retirement/dispatch/command settlement and B cancellation/lifecycle implementations. All new documentation links resolve. No unit/native test is claimed for a design-only group. The nearest Area remains Groups6-8; next dispatch is Group7's numeric command/model/feature identity map.

## Group 7 — Append-only identity contract, 2026-10-23

All four tasks are complete. [The identity handoff](m2o23-1-come-space-d-identities.md) fixes the divergent command tail, exact source-labelled tuples and sole implementation owners. The independent `src/tests/fixtures/multilayer/merge-command-identity.json` records all223 expected merged commands and their original A/B declarations. A's211 executable IDs stay unchanged; all204 common-prefix tuples were verified identical. Donor-only entries append211-221; checked legacy cancellation is222; End223 still fits uint8. This is a test oracle, never a runtime dispatch catalogue. Registration remains Group9 and each supplying implementation group.

HEADs still match the pinned original commits. Merged441/MLLN constants remain in `save_lineage.h` without writer activation. Four original donor feature bits and canonical digest kinds33-35 are now reserved in the existing `multilayer_state.h`; no capture/acceptance path publishes them yet. Its affected Part2 Mandatory Reading ranges were refreshed. The schema row remains version1 and cannot choose a branch.

The RFMT codec example demonstrates a subtle required conversion: the command buffer writes integers little-endian, whereas A's label integer has big-endian semantic construction and B wrote raw label bytes. Donor packet bytes therefore cannot be copied into A's tuple. Old command logs/peers require verified source ABI and explicit supported conversion; the table is not replay permission. B's expected-count cancellation is retained through the appended canonical-owner entry, not by changing A's tuple.

All model9-13 coefficient differences, EIDS0x8000+model and one-installer decision are recorded. Model9 is unchanged. Loaded original A model13 cargo must survive its96-to80 definition change through the native train/custody owner; loaded refit/replacement to insufficient capacity refuses, and migration cannot silently clip packets. Groups31/32/42 own actual native implementation and acceptance.

Focused static verification confirmed223 unique sequential expected values, every original A value preserved,217 B command mappings represented, all204 common argument tuples identical, exact feature-bit equality and all plan/handoff local links resolving. Byte examples were independently checked using Python's standard struct codec. No native peer/fleet or full build result is claimed. Next dispatch is Group8, still under the same nearest Area; reload only the asset-layout/public-range sources needed for that comparison.

## Group 8 — Logical assets and protected ranges, 2026-10-23

All four inventory tasks are complete. [The family handoff](m2o23-1-come-space-e-asset-families.md) and frozen `src/tests/fixtures/graphics/merge-asset-families.json` describe148 IDs, every pose domain, exact authored originals, actual native draw boundaries and existing A packing owners. B adds no logical family: its90 packed families are precisely A's43 M1 plus47 Ground families. A's19 additional selected Space/orbital/launch families and three additional declared model IDs remain protected. No media/GRF/header was replaced or published.

The disposition is82 identical declarations,45 A-owned families needing donor integration,16 A-delivered families over B placeholders,3 A-only families and2 concrete source conflicts. Battery Works and Electronics Works each have512 matching pose keys/anchors but different real machinery originals; both alternatives remain retained and publication is held for Part2 Group48's bounded review. This does not stop independently classified Group9 source work. Sixteen ground families have A pixels but incoming B native draw behavior, which Part2 must actually bind and qualify.

The protected native interval remains0-22044 inclusive, partitioned into M0/reserved core, M1, preview, Space, orbital, launch and Ground. B's expanded M1 offsets collide with A's current Space/Ground ranges; same logical assets therefore keep A's existing offsets. No new append family is required by the observed inventory. Four M1 historical `authoring.originals` entries point to generated old-log atlases; the observer correctly records generated ancestry rather than treating those cache paths as authored originals.

`python -B src/tests/multilayer/asset_merge_inventory.py` regenerated the observation in about1.7seconds, entirely from A's live media plus Group2's hash-verified retained donor originals. Output is only `test-cache/merge-assets/family-inventory.json`; the tool cannot publish sources, shipping assets or the frozen fixture. All declared authored originals are present. All current draw symbols and12 distinct retained donor draw boundaries were verified in actual source. These are static inventory results, not pixel or native-runtime acceptance. Next dispatch is Group9's new mechanical-import Area, with the reviewed command and asset handoffs added to its reading gate.

## Groups 9 and 9B — Reviewed import boundary and registration, 2026-10-23

Group9's original five tasks were split before copying into9/9B/9C/9D, retaining later group identities. The size was hidden by the original summary:1,041 candidate paths include539 saves and276 Python files. The current [import handoff](m2o23-1-come-space-f-import-routing.md) and `merge-path-dispositions.json` now assign all2453 donor delta paths explicitly. Shared hooks originally assigned generic Group9 work now name their actual producing groups.

All four Group9 tasks are complete. Only `newgame_settings.h` was copied unchanged into active source: its declaration matches A's already existing `MakeNewgameSettingsLive`. All500 newly named source candidates have exact retained originals; the other499 stay with their semantic/harness groups until their APIs, path roles and comments are adapted. All544 fixture routes retain their exact namespaced hashes without duplicating saved worlds. No code candidate in the inspected set contained recursive-deletion primitives. Output/path/publication patterns and74 Python docstring gaps in the full source comparison remain explicit review triggers, not claims that every write is unsafe. Original blobs were verified allowing only Git's CRLF/LF representation difference; exact retained payload bytes were unchanged.

All three Group9B tasks are complete. The existing CMake owners now register `newgame_settings.h`, `save_lineage.cpp/.h` and `save_lineage_policy.cpp` exactly once. The production leaf follows the normal `stdafx.h` convention while retaining its standalone flag. MSVC C++20 `/W4 /WX` compilation passed, then5 focused cases/831 assertions passed again. Native full-target integration is deferred to Group60; no game or broad regression ran. Unimplemented command traits are not registered, and the sole donor reconstruction recipe remains disabled pending Group58. A's384 connected fixture and original Space seal were rehashed unchanged.

Next dispatch is9C's documentation-routing Area, followed by9D's separate helper/output gate. Do not skip those suffix batches before Group10.

## Groups 9C and 9D — Historical prose and safe entry points, 2026-10-23

All four Group9C tasks are complete. [The documentation handoff](m2o23-1-come-space-g-document-routing.md) and `merge-document-routing.json` cover129 incoming Markdown paths:127 dated historical copies under `docs/zz-rhr/imports/openttd3-75ea75718f/`, one exact existing A lifecycle-design reuse and the review README kept A-owned for9D. Each copy discloses source identity and non-executable historical status. All4351 prose reference dispositions and127 output hashes were verified. Twenty-nine current merge citations now use dated A copies; affected mandatory navigation ranges account for the four-line banner. No active A feature plan/architecture was overlaid. The586 remaining historical donor-source links and9 source-directory links are explicit final Group71 documentation dependencies, not current native prerequisites.

All four Group9D tasks are complete. `.gitignore` is byte-identical between the pinned repositories. B's review README predates A's completed R2FC source/native reconstruction boundaries; A's current routing remains unchanged. The meaningful donor launcher change selects four B425 TBM demos, while an overlay would drop A's thirteen later Space tours. Those TBM selections now belong to explicit task63.6, after real lineage qualification and an exact-origin/merged-successor route exists. Current A403 menu entries remain usable; no unsupported425 world is activated. Configuration originals already retained by Group2 do not replace A's portable profile or runtime owners.

The only unchanged active donor copy is the documented startup declaration header; it contains no executable helper, deletion, publication or machine path. Unadapted code/report helpers remain assigned and unavailable under their semantic/harness groups, with the static review flags/docstring gaps recorded by Group9. Neither cleaner nor launcher was executed. `acceptance_ground_fixture.cpp` now calls the live `GetTestCaseArtifactPaths("acceptance", "ground-connected").Output("saves", "ground-connected.sav")` boundary, so a generated world cannot overwrite the original source fixture. Its isolated/seeded disclosure stays intact. Static routing checks passed and the original384 hash remains `fce011e9093ad91b052b03517132319c14c48086ecb3c7d1c5d501e416d385c7`.

Native fixture execution is deliberately deferred to the focused Group60 matrix, after the merged save/owner contracts are buildable. It will emit a new run-owned save under `mlm/reviews/acceptance/ground-connected/<run-id>/saves/` (or its explicitly inherited case context); no source fixture is its output. Pure lineage results remain the five-case/831-assertion standalone evidence from9B. Donor Git status still contains only the original untouched empty plan. Next sequential task is Group10.1, under `Detached supply records and lossless conversion`; read that Area and the receipt/identity contracts before modifying the canonical record types.

## Group 10 ? Tagged receipts and unchanged native behavior, 2026-10-23

All four tasks are complete. The sole snapshot/vector now contains explicit native or legacy evidence; unsupported transport tags have an invalid monostate. Native payer/Q16 price/anchor/state checks remain unchanged. Legacy validation preserves actual dates, frozen unit tariff, finite counters and source/company-loss sentinels, including owned Surface signs without a remaining freight berth. The complete original legacy prefix is retained for life; sparse subsequent native IDs use the shared exact uint64 lookup, never ordinal indexing. Native admission remains16384; detached complete history allows65535.

A440 still emits exactly14 header bytes and53 bytes per native record and refuses variants its old wire cannot represent. Its independent67-byte literal passed unchanged. Runtime callers were migrated to explicit native access; dispatch, cancellation and retirement cannot mistake a legacy row for Pending. The native installer deliberately rejects legacy candidates until Groups13/41 provide their complete custody join. This temporary gate is not completed merged-load support, and later groups must remove it only with the actual join.

The focused Debug target built successfully. A pre-existing MSVC PCH/toolset mismatch required refreshing only the generated PCH source timestamp, followed by the normal two-worker target build; no cleaner, deletion, checkout or Git write was used. One obsolete include was corrected to the actual calendar owner. Build diagnostics are reproducible under `test-cache/merge-supply/` with `cmake --build build --config Debug --target openttd_surface_supply_test --parallel 2 -- /nodeReuse:false /p:CL_MPCount=2` after selecting the installed CMake/VS toolchain.

Focused checks passed: `[surface-supply-state],[surface-supply-receipts],[surface-supply-order-wire]` ran14 cases/403 assertions; `[surface-supply-dispatch],[surface-supply-lifecycle],[surface-supply-commands],[surface-supply-owner]` ran14 cases/1134 assertions. Both bursts took under four seconds and write `group10-detached.txt`/`group10-native-owner.txt` under `test-cache/merge-supply/` using `--reporter compact --out`. New detached cases cover twenty independent corruptions, full65535-row retention, high uint64 lookup, unchanged input/packet pool/RNG and source-specific cap use. The detached validator has no finance or live-owner mutation path. Existing native command tests confirm actual charge/refund behavior. No full regression, original-world load or protected Space tick ran.

After one context compaction, Global and the nearest Groups10?11 Area were reread and its navigation ranges corrected. Group11's entry gate now passes: the live tagged records, shared origin validators and exact lookup exist with focused evidence. Next work is pure same-file conversion, with no publication, packet creation or settlement.

## Group 11 ? Pure original-family conversion, 2026-10-23

All five tasks are complete. `surface_supply_order_wire.h/.cpp` now exposes `LegacyBSupplyOrderWire`, `LegacyBSupplyTransport`, `ConvertLegacyBSupply` and `ConvertLegacyASupply` beside the existing A codec. The B adapter checks schema1, exact count/frontier, dense ordered original IDs and the raw0/1 closure byte before allocating a bounded canonical candidate. It preserves every actual scalar, widens IDs exactly, then calls the one origin-specific validator. A projection refuses nonnative evidence and retains sparse uint64 IDs, exact whole charge, Pending rights and terminal historical packet indices.

The already existing `SurfaceSupplyRecordContext` supplies same-file references; the plan's conceptual extra `CandidateSupplyReferences` registry was replaced with this concrete boundary. Neither adapter creates stock, updates a wallet/counter, restores an allowance or publishes a live book. Full/copy loaders still must establish complete original chunk presence/kind/columns, retain A's saved-day stamp for independent DATE agreement and perform the complete custody join. Part2's adapter Area now explicitly consumes these functions instead of duplicating conversion.

The focused Debug target compiled successfully. `[surface-supply-conversion],[surface-supply-order-wire]` passed10 cases/465 assertions in under one second, with evidence at `test-cache/merge-supply/group11-conversion.txt`; build diagnostics are `group11-build.log` in that directory. Cases compare every converted scalar for original-price refunds, partial shipment, source loss, takeover and company loss; enforce missing/contradictory-reference refusals and raw header/identity checks; preserve all65535 original rows; and observe unchanged installed history/caps, packet count, actual wallets, RNG and current day. The independent A440 literal remains unchanged. This is component conversion evidence, not native merged-save acceptance.

Next sequential work is Group12 under the new Packet purpose and complete custody Area. The native live installer still refuses legacy candidates until complete custody integration exists; no old world or protected Space continuation was run.

## Group 12 ? Packet evidence without premature legacy admission, 2026-10-23

All four refined Group12 tasks are complete. The original combined packet batch was split into12 (metadata),12B (first-departure accounting) and12C (exact source selection), with distinct Area context for metadata, accounting/selection and complete custody. No original requirement was dropped. Successful linked departure through normal live adoption explicitly belongs to13.5, once its complete native custody join exists; the current installer gate was not weakened for a fixture.

`CargoPacket` now retains optional uint64 order identity and original shipped state; zero/false remain defaults for ordinary cargo and valid unlinked A CompanySupply. `RestoreCompanySupplyOrder` accepts original evidence once only on already non-paying cargo, refuses zero/relinking/ordinary sources, and grants no order admission or counter update. Copy/split and both native list merge guards preserve exact link/state beside all existing provenance and MAX_COUNT checks. A's purpose-based rating protection remains unchanged. Old CAPA serialization remains unchanged pending Part2's explicit format work.

The Debug target built successfully; the only compiler warning observed was the existing signed/unsigned comparison in `tbm_projects_gui.cpp:255`, outside this change. `[surface-supply-packet-evidence],[surface-supply-custody]` passed10 cases/572 assertions in under one second. Evidence is reproducible at `test-cache/merge-supply/group12-packet-evidence.txt`; build diagnostics are `group12-build.log`. Cases exercise raw high-width identity, relinking refusal, original shipped/unshipped splits, actual buyer invalidation, both list merge owners, saturation, retained A rating protection and positive ordinary payment. Raw high-ID/saturated components are explicitly not valid funded-history or merged-save acceptance claims.

During the direct packet diff, B's delivered partial Deliver-to-Transfer sign correction was identified separately from metadata. Task17.5 now requires that exact correction plus an independent8-to3 partial case and pool-exhaustion handling; A's neighboring tube-room hooks remain protected. Its original31-line implementation is now in the relevant Area reading. No unrelated cargo behavior was imported here.

Both plans' local links, task numbering, difficulty placement/batch bounds and nearest-Area coverage were checked during this work. Donor status still contains only its original untouched empty plan. No Git write, deletion, full regression or original-world load occurred. Next dispatch is12B.1 under First departure and exact source selection;12B/12C and all subsequent groups remain incomplete.

## Group 12B ? Checked first departure and truthful movement, 2026-10-23

All four tasks are complete at the declared component boundary. `CanRecordSurfaceSupplyDispatch`/`RecordSurfaceSupplyDispatch` operate on the existing canonical snapshot, with thin overloads for the sole installed book. Exact lookup, explicit legacy provenance, nonzero quantities and checked refunded/dispatched subtraction precede mutation. Final departure after closure/company loss remains possible within the original finite remainder. Only the dispatched counter changes; no payment, stock, cap or closure is manufactured.

`CargoPacket::CanMarkCompanySupplyShipped` validates the selected amount before splitting; `MarkCompanySupplyShipped` executes the actual book update outside assertions before setting the permanent flag. Unknown/native linked references refuse even when the incoming packet claims to have shipped already. Ordinary and unlinked A cargo remain unchanged. Native load/reservation hooks preflight before `CargoMovement::Preprocess`; an unexpected post-split refusal reunites the untouched fragment. Whole hub moves preflight all selected fragments and aggregate per-order first-departure quantities before any mutation. Native vehicle transfers also reject a forged unshipped linked hold.

Inspection found that generic `ShiftCargo` reports movement by consumed budget. Stopping traversal with budget zero would falsely report refused units as moved. The concrete station Load/Reserve wrappers now return their actual waiting-cache decrease; generic rerouting semantics remain unchanged. Tests prove both zero movement and a valid two-unit prefix followed by an unchanged invalid ten-unit fragment. A hub request containing that later invalid fragment refuses before moving the prefix. Hub function/class/argument documentation and business-logic comments now cover the changed boundary.

The focused Debug target built successfully. `[surface-supply-departure],[surface-supply-custody],[surface-supply-dispatch]` passed13 cases/935 assertions in under three seconds. Logs are `test-cache/merge-supply/group12b-build.log` and `group12b-departure.txt`. The same existing unrelated TBM signedness warning remains. The implementation contains no accounting side effects inside assertions; optimized native execution still belongs to the later qualification matrix, and successful linked live adoption/return/first-departure cases remain explicitly assigned to13.5 after complete custody exists. No test-only live installer or private order store was introduced.

A further original-wire distinction was verified directly: B's producer creates paid packets as unknown Industry sources and supplies purpose through its order link. Part2's41.3 now explicitly validates that original evidence and normalizes proven legacy paid purpose to CompanySupply with an unavailable original buyer, never today's owner. The corresponding producer range is mandatory only for that loader area; B's self-debit producer is not an import candidate. Current canonical packet restoration deliberately refuses relabelling ordinary sources without that later verified dialect conversion.

Next dispatch is12C.1 under the same First departure and exact source selection Area. The live legacy installer remains closed until Group13's complete ownership join; all later groups remain incomplete.

## Group 12C ? Exact native refund-stock selection, 2026-10-23

All three tasks are complete. `StationCargoList::CountUnshippedCompanySupply` and `RemoveUnshippedCompanySupply` use exact uint64 links and permanent CompanySupply purpose across actual routing buckets. Returned/shipped, unrelated-order, unlinked A and ordinary freight are excluded; carrier-held/reserved material is outside the source waiting list. Removal recaptures the complete expected count before changing any fragment, then maintains the native waiting and weighted-age caches. A zero-eligible selection changes nothing and grants no closure/payment by itself. Group15 remains the sole cancellation/settlement boundary.

The focused Debug target built successfully. Available host capacity was checked before raising only the compiler worker bound to four; no second build or regression ran concurrently. `[surface-supply-refund-stock],[surface-supply-departure]` passed8 cases/424 assertions in under two seconds. Logs are `test-cache/merge-supply/group12c-build.log` and `group12c-refund-stock.txt`; reproduce the selected target with `/p:CL_MPCount=4` or a smaller host-appropriate bound. Independent cases cover multi-bucket fragments, unknown/retired buyer purpose, distinct ages, exact remaining quantities/routing/identity, stale counts, zero identity, returned and actual vehicle-owned stock, zero eligible selection and unchanged real wallet, receipt history and RNG. No legacy command refund or merged-save load is claimed.

Next sequential work is Group13 under Complete physical custody and admission. Read that nearest Area before inventorying A's real station/vehicle/launch/Space owners. The legacy installation guard remains closed until the complete actual ownership join exists. Its successful linked-runtime departure cases from12B are still required by13.5. Both split plans and all later tasks remain active; Part2 has no completed implementation groups yet.

## Group 13 ? Complete actual-owner traversal, 2026-10-23

All four refined Group13 tasks are complete. The former large custody group is now13/13B/13C/13D, preserving all original inventory, validation, adoption and integration requirements. Earlier execution notes referring to13.5 now dispatch13D.2?13D.3. Nearest Area blocks separate actual-owner enumeration, detached receipt policy and native adoption/acceptance; no legacy installation guard was removed in this batch.

[The physical-owner handoff](m2o23-1-come-space-h-cargo-custody.md) lists nine owner families and their exact native fields, including reserved vehicle freight, both service berths and all four heavy manufacturing inputs. Pod/carrier references, copied IDs and processing metadata are explicitly not extra inventories. `cargo_custody.h/.cpp` now supplies the complete read-only native visitor, refusing null references, unresolved nonempty cargo meanings, missing/incompatible recipes and nonempty unused manufacturing slots without creating goods data or changing stock.

A's existing full asteroid walk was factored through this visitor, closing its former manufacturing omission. Direct comparison preserves its original natural-body bounds, Space source tile, invalid first station, cargo label, uniqueness and final orphan predicates. The bounded `InsertOrbitalPacketOwners` helper stays in its existing orbital-only admission role; replacing it with the full walk would double-count its caller's earlier owners. The available asteroid provenance scenarios require saved-world startup and remain with later native acceptance; no protected Space window, original-world operation or hidden scenario was run here.

The selected Debug target compiled successfully. `[native-cargo-custody]` passed3 cases/429 assertions in under one second, with actual independent expectations for19 native references across every owner family. Five structural corruptions, absent/refusing callbacks, empty owners, repeated read-only observation, unchanged RNG/cash and unchanged native object/packet counts passed. The fixture declares actual pool/container objects and stock, not paid gameplay or whole-world operational validity. Logs are `test-cache/merge-supply/group13-build.log` and `group13-custody.txt`; CMake registers both native visitor files and the focused test/fixture in their existing owners.

Next dispatch is13B.1. Its checker must consume the same explicit original packet lookup for owned and allocated observations, with independent candidate packet count and ordered final enumeration so an omitted or duplicate final scan cannot pass. Native and later copied adapters must reuse this policy; no second complete inventory, active-world fallback or publication belongs in the pure checker.

## Group 13B ? Shared same-candidate custody policy, 2026-10-23

All four tasks are complete. The existing canonical model now exports `SurfaceSupplyPacketFacts`, `SurfaceSupplyPacketLookup` and `SurfaceSupplyCustodyCheck`. It reuses `SurfaceSupplyRecordContext`/`ValidateSurfaceSupplyRecords` rather than creating a second receipt validator or station registry. One immutable candidate lookup supplies both passes; independent original packet cardinality, strict final ID order and transient uniqueness/completeness evidence prevent missing, duplicate or late observations from passing. Per-receipt totals use exact canonical lookup, never `id - 1`, and check finite bounds before addition/subtraction.

Legacy links require the actual container label and CompanySupply purpose with unknown original buyer, matching B's real producer and planned wire normalization. Native issuance indices remain historical only. Unshipped legacy material requires an actual same-candidate Surface queue; the closed owner-loss exception cannot admit Underground, missing-station, carrier or job stock. Every visited reference is unique. The final pass rejects all paid orphans, including unlinked A CompanySupply, while ordinary orphan/source policy stays with its existing owners and gains no synthesized custody here.

The first build found a Catch2 compound-assertion syntax error, which was fixed. The final focused Debug target built successfully; `[surface-supply-custody-policy],[surface-supply-state]` passed10 cases/440 assertions in under one second. Evidence is `test-cache/merge-supply/group13b-policy.txt`; build diagnostics are `group13b-build.log` and `group13b-build-retry.log`. Cases cover20 independent contradictions, both source/shipped visitation orders, mixed native/legacy history, source/reference refusal, owner loss, duplicate/partial enumeration, paid orphan versus ordinary policy, sticky errors and unchanged input history/facts. No native packet or live order was installed by this pure batch.

Pinned HEADs still match A `cfeff84db52f8a3753a43c2a770a6363c62b2c67` and B `75ea75718fa7d14632bf795105721df1e390f753`; donor status remains only its original untouched empty plan. Main-plan links, nearest-Area coverage, numbering and maximum-eight difficulty batches passed after the custody split. The two execution protocols still match exactly. The legacy installation gate remains closed pending13C; next dispatch is13C.1 under Native adoption and cross-owner acceptance.


## Group 13C ? Native custody before staged adoption, 2026-10-23

All four tasks are complete. The native adapter now supplies original packet scalars and actual container labels/queue IDs to the shared checker, then enumerates the allocated pool only for completeness/orphans. Only a complete successful join removes the temporary legacy rejection. Staging preserves up to65535 total history rows and at most16384 native rows; new purchases retain their existing16384 retained-history limit and the A440 decoder is unchanged. Same-file DATE/MAPS agreement still precedes publication. Missing old family evidence cannot erase linked history while its actual paid packets remain.

The selected Debug build passed after fixing one fixture-only narrowing conversion. `[surface-supply-adoption],[surface-supply-owner],[surface-supply-dispatch]` passed12 cases/1172 assertions in3.17 seconds. Evidence: `test-cache/merge-supply/group13c-adoption.txt`; diagnostics: `group13c-build.log` and `group13c-build-retry.log`. Tests cover valid actual source custody, all65535 original rows, unchanged history after new-purchase refusal, both transport bounds,10 independent candidate contradictions, absent original references/clocks, duplicate actual station/handling ownership, paid orphans, invented original buyers and unlinked A controls. Successful admission creates no packet or charge; refusal preserves actual packets, queue caches, money, cap and installed book.

These are declared native component DATE/MAPS/stock inputs through the production loader boundary, never a test-only live installer. B426-434 wire decoding, merged serialization, real original-save acceptance and legacy lifecycle remain later tasks. No saved Space continuation, Git write or filesystem deletion occurred. Next dispatch is13D.1 under the same Native adoption and cross-owner acceptance Area.

## Group 13D - Actual Space custody and once-only departure, 2026-10-23

All four tasks are complete. Thirteen applicable actual Space owners admit declared shipped legacy material beside ordinary/unlinked A controls. Impossible PROP/GOOD/RBPT compartment claims and unshipped nonstation stock refuse unchanged. Actual station Load/Reserve and Hub movements prove partial splitting, reservation-to-load, reservation return, repeated handling/carrier movement and source return. Only30 initial units and the70 untouched remainder increment original dispatched history; stale source selection refuses, returned stock remains non-paying/non-refundable, and no command credit, cap release or automatic closure occurs. This closes12B's successful installed-native-path deferral without a test-only book publisher.

The first focused run correctly rejected four100-unit ELEC declarations above the existing64-unit cap. Only the fixture quantity changed to30; production admission stayed intact. After rebuilding, `[surface-supply-space-custody],[native-cargo-custody]` passed6 cases/1328 assertions in2.14 seconds. Logs: `test-cache/merge-supply/group13d-build.log`, `group13d-build-retry.log`, `group13d-space-custody.txt` and `group13d-space-custody-retry.txt`. The shared fixture's teardown tracking was exercised again by the actual-owner observer cases.

The map-layer architecture now names the sole full visitor, original subset helper, canonical custody checker, publication boundary and still-open downstream gates. Handoff H records exact owner coverage and reproduction filters. Components are not paid operational fleets; original B decoding, copied joins, merged writing and original-world acceptance remain Part2. No original save or protected Space continuation was touched. Next dispatch is14.1 under Supply commands, refunds and lifecycle.

## Group 14 - Native commands and whole dispatch preserved, 2026-10-23

The four refined command/dispatch tasks are complete. Existing canonical uint64 intent and cargo-label-value tuples, exact retries and changed-payload refusal remain intact. New receipts remain native Pending; only actual whole station ingress makes them Fulfilled. Placement/cancellation still return Property expenses to the original command kernel, with no donor self-debit. Actual place/cancel/dispatch transitions now dirty station views and the owner's station list; all source views share the company-day cap, so observations never publish a new cache or mutate gameplay. Native station-loss reconciliation retains its pre-existing explicit lifecycle settlement.

A new actual-storage capacity case fills the real queue to UINT32_MAX through65537 native unlinked packets, using bounded opaque routing buckets to avoid quadratic merge scans. These routes are explicit capacity components, not an operational route or original-save proof. The real station tick refuses the complete two-unit paid order unchanged, exact retry charges nothing, and after native Truncate frees room the same order enters stock exactly once and cannot refund. No fake queue count or replacement receiver was used.

The selected Debug build passed; `[surface-supply-commands],[surface-supply-dispatch],[surface-supply-lifecycle]` passed11 cases/683 assertions in3.53 seconds. Evidence is `test-cache/merge-supply/group14-commands.txt`, build diagnostics `group14-build.log`. Existing estimate, retry, source loss, dispatch and post-delivery cancellation cases remain passing.

Inspection confirmed A has no ordinary station supply controls, while B's controller relies on incompatible order types, self-settling commands and int-cast order IDs. The former14.4 is refined into14B controller adaptation and14C real StationView integration/acceptance, with their own nearest Area and explicit future legacy refund gate. Group15.6 will enable that controller's legacy cancellation only after the real canonical quote/command exists. This preserves scope while keeping the incoming context bounded. Next dispatch is14B.1; those UI tasks remain unchecked.

## Group 14B - Canonical transient supply controller, 2026-10-23

All five controller tasks are complete at the compilation boundary; native StationView wiring and runtime acceptance remain14C. The donor-named SurfaceSupplyControls now consumes canonical tagged records, pure command estimates, existing company-day usage and A's synchronized label-value/uint64 tuples. Temporary dropdown indices map to exact original IDs and resolve current source/owner again on delayed selection; no receipt is narrowed to int or stored as a retained row pointer. Pending native cancellation uses only A's command. Legacy history is truthful, but its refund control stays unavailable until15.6 supplies the actual canonical boundary.

Quantity input requires a complete positive uint16; an unfinished/invalid editor also blocks a delayed Buy click before another paint, so it cannot purchase the previous valid quantity. Native Pending/Fulfilled/Cancelled/Lost displays describe station delivery as the refund cutoff. Existing original legacy counters remain separate. Controller file/function/class headers and compound business checks are documented. Existing GUI/widget/string/CMake owners register the component without a second catalogue or finance path.

The selected Debug target and generated language consumers rebuilt successfully; diagnostics are `test-cache/merge-supply/group14b-build.log`. No runtime UI claim is made from compilation. Current plan validation found46 batches/196 tasks in Part1 and36 batches/156 tasks in Part2, correct nearest-Area coverage and matching execution protocols, with no missing local links. Part2 task37.3 now verifies the existing controller after further integrations instead of duplicating its implementation. Next dispatch is14C.1 under Native station supply controls; no window callbacks have been changed yet.

## Group 14C - Actual StationView integration and acceptance, 2026-10-23

All four tasks are complete. A's existing StationView now composes the supply foldout into its layout, construction, paint/text, click, dropdown and query callbacks. The original launch-hub/assembly controls and other station actions remain their original handlers. Runtime ownership/layer/profile changes hide the local panel without changing gameplay. Real commands/ingress dirty dependent views; no observation dispatches supplies. GUI architecture records the sole controller and canonical settlement boundary.

The first window attempt exposed fixture prerequisites: paused management commands were disabled, and native quantity-editor focus callbacks require a video driver. The fixture now explicitly permits management while retaining pause, uses the actual null driver only when none exists, never enters its loop, and closes its modal/error windows before parent/world cleanup. No production command policy or UI safety check was weakened. The intermediate corrected3-case window run passed234 assertions; the final scoped pass below also covers a genuinely pruned high-ID order while its original menu remains open.

The final selected Debug build passed. `[surface-supply-window],[surface-supply-commands],[surface-supply-dispatch]` passed10 cases/770 assertions in3.55 seconds. Evidence: `test-cache/merge-supply/group14c-window-final.txt` and its stderr log; build diagnostics: `group14c-build-final.log`. Earlier attempt/retry outputs remain beside them. Native callbacks prove exact IDs above32 bits, actual posted purchase/cancellation, refused stale post-delivery refund, unchanged book/packets/money/tick/RNG under repeated sizing/paint/text, malformed and unfinished quantity input, ordinary rename delegation, stale menu identity after real retirement, legacy history display, foreign/hidden-layer visibility and preserved disabled ordinary launch controls. This is native component acceptance, not pixel review or a live paid-world continuation.

Part1 now has100 checked tasks out of196 across46 numbered/suffix batches. Part2 retains36 groups/156 unchecked tasks. Next sequential dispatch is15.1 under Legacy refund and lifecycle settlement. The Group15 public legacy quote/command and controller enablement remain absent and unchecked; never bypass that gate through the donor's self-credit path. No Git write, recursive/individual filesystem deletion, original-save replay or protected Space continuation occurred.

## Group 14D - Dependency-ordered unpublished command allocation, 2026-10-23

All three tasks are complete. Native MakeCommandsFromTraits and network MakeDispatchTable instantiate every value below End; Group9B deliberately registered no unavailable bodies. The original Group7 planned222 legacy command therefore could not be activated at15 without eleven premature implementations/stubs. The user-authorized execution-plan refinement revises only the unpublished new tail into producing-group order:211 legacy cancellation;212 TBM replanning;213-215 generator/site commands;216 town service;217-222 external/geothermal tube commands. Original A0-210 and every original B value/tuple remain exact. Current code was confirmed to contain only the original211 A commands before any append.

The original Group7 fixture remains byte-for-byte unchanged, SHA25694e17438558c2695ba1a856c0f5de5b96c163031386862b1880bd3e116fc5fcd. The separately named merge-command-identity-sequential.json carries revision2 and the original digest, preserves all source-labelled facts, and provides independent future expectations specified before the implementation. This is not a regenerated test baseline or runtime registry. Static comparison verifies223 unique final IDs, unchanged source fields/prefix and no new live trait activation. Evidence: test-cache/merge-supply/group14d-identities.json. The handoff records the revision; Groups25/33/47 now explicitly follow it.

This resolves the plan sequencing conflict without sparse command dispatch, fake command bodies or changes to original epochs/saves. No native build/test was needed for this contract-only batch. Next dispatch resumes15.1 under Legacy refund and lifecycle settlement, with the legacy command now allocated211.

## Group 15 - Exact legacy cancellation and native UI settlement, 2026-10-23

All six tasks are complete at the command/component boundary. The native enum appends only implemented CancelLegacySurfaceSupplyOrder211 and its exact uint64 intent/uint16 expected-count trait, preserving original209/210. PrepareLegacySurfaceSupplyCancellation resolves the current owner/source, runs shared full scalar/custody validation, counts actual never-shipped source units, uses the saved tariff and bounds wallet credit. Berth removal alone does not erase B's source entitlement. A zero-eligible closure allocates no stock/list and pays nothing. Native Pending/Fulfilled receipts cannot enter this legacy path.

Both command phases recapture and compare the complete expected count. Before physical removal the owner prepares the expense and any allocating native window invalidation. Exact native removal then precedes only scalar refunded/closed mutation and return of the frozen Other expense; the original finance kernel credits once. Returned/reserved/unlinked/ordinary stock is excluded. No donor self-credit or second refund balance exists. The UI displays original legacy counters and saved-tariff refund, stores only the last rendered transient request, rejects changed facts before posting, and uses the separately checked command.

The first build found one fixture-only Money/int64 comparison ambiguity; it was corrected to explicit Money equality. The final selected Debug build passed. `[surface-supply-refund],[surface-supply-window]` passed12 cases/802 assertions in1.97 seconds. Evidence is `test-cache/merge-supply/group15-refund-window.txt` and its stderr log; build diagnostics are `group15-build.log`/`group15-build-retry.log`. New cases cover exact literal ABI bytes, frozen Other accounting with unchanged Property/running expenses, missing freight berth, partial leftovers, stale counts, reserved/returned units, zero-unit closure, positive-wallet overflow/exact maximum/negative balances, corrupt source evidence, unchanged cap/RNG and reconstructed transferred ownership. Current native/window cases remain passing.

Actual company-event takeover-followed-by-refund requires Group16's lifecycle implementation and is explicitly retained in16.5; this batch's transferred-owner case is a declared restored component, not a substitute for that event. Group16 remains wholly unchecked. No original saved world, source art, protected Space continuation or Git state was altered. Next dispatch is16.1 under the same Legacy refund and lifecycle settlement Area.

## Group 16 - Native lifecycle and retained legacy history, 2026-10-23

All five tasks are complete. The sole lifecycle owner now distinguishes native Pending settlement from legacy source loss. Legacy rights survive actual berth removal and movement of the same owned Surface station sign; station destruction closes the receipt and clears its source without credit. Takeover follows settled native station ownership and transfers original history/caps while preserving unavailable original legacy buyer evidence. Company loss seals legacy rights before demolition and keeps native Pending loss policy, physical non-paying purpose and existing launch/hub hooks. Repeated callbacks and reused company/station slots cannot recreate rights.

Existing source-specific day usage, lifetime legacy retention and separate65535 import/16384 new-admission bounds were verified rather than replaced. Safe old native terminal retirement leaves sparse IDs permanently issued; physical unlinked native stock survives retirement. Actual takeover followed by legacy refund closes15.5's deferred event proof. The combined248-unit history is valid but blocks new admission; neither legacy refund nor bankruptcy resets its original spending history.

The selected Debug build passed. The first focused run exposed a fixture assertion that ignored native ChangeOwnershipOfCompanyItems installing a temporary demolition budget. Corrected checks use the unchanged expense ledger, zero refunded units and the actual native budget; no production bankruptcy policy changed. Final `[surface-supply-legacy-lifecycle],[surface-supply-lifecycle]` passed11 cases/414 assertions in1.12 seconds. Evidence: `test-cache/merge-supply/group16-lifecycle-retry.txt`; diagnostics: `group16-build.log` and `group16-build-retry.log`. Tests cover real removal/movement/deletion, slot reuse, takeover/refund, above-cap histories, ownerless107-unit mixed stock, rollover and16384-row retention. Prior65535-row import/admission proof remains Group13C's independent coverage.

No original saved-world replay, protected Space continuation, donor write, Git write or filesystem deletion occurred. Current runtime-header navigation is99-127 (stale97-125 ranges corrected). Next sequential dispatch is17.1 under Cargo delivery and tube transfer admission; original save compatibility remains a later plan gate.

## Group 17 - Permanent non-paying purpose and exact partial transfer, 2026-10-23

All five tasks are complete. Inspection confirmed the current native DeliverGoods/CargoPayment/subsidy and rating owners already use permanent CompanySupply purpose independently of an optional legacy link. Actual receiver admission, accepted quantities, station/company/town statistics and cargo monitoring precede revenue suppression; ordinary freight retains its original positive tariff and feeder settlement. Hub, reservation and Space service/manufacturing moves use the existing native packet transfer/consume owner, preserving source/link/departure facts rather than recreating cargo. No second payment or delivery owner was imported.

VehicleCargoList's partial Deliver-to-Transfer arithmetic now splits only the excess over the previous Transfer prefix plus requested movement. It prepares the sole boundary split before changing any earlier next-hop/action state, returning zero unchanged on actual Split allocation-admission refusal. Full-packet boundaries still work without allocation. Adjacent native tube room hooks remain intact for18. New independent cases cover8->3Transfer/5Deliver, an existing2-unit Transfer prefix, paying feeder30/50 shares, native and high-uint64 legacy purpose, spanning-packet refusal, whole-boundary and full/clamped requests. The exhaustion case is explicitly scoped allocation-admission fault injection into the real pool predicate, not a claim that16million physical packets were allocated; the count is restored before any pool iteration/destruction.

A new migrated-stock case uses the normal component adoption boundary, native protected rating cull, Reserve/Load, intermediate transfer and bounded actual processor intake. It proves one accepted unit updates stock and statistics at1024 capacity, seven remain aboard with the exact shipped link, and neither transfer nor final delivery pays. Existing ordinary-positive/subsidy/purpose cases and actual native Space custody/departure cases pass alongside it. Initial compile-only issues were test include order and using the donor rather than native rating helper; corrected to the existing TruncateAbandonedOffers contract without widening production APIs.

The final selected Debug build passed. `[surface-supply-custody],[surface-supply-legacy-delivery],[surface-supply-space-custody],[cargo-partial-transfer]` passed12 cases/1567 assertions in2.94 seconds. Evidence: `test-cache/merge-supply/group17-cargo.txt`; build diagnostics: `group17-build.log`/`group17-build-retry.log`. Source header/comments describe the new two-pass refusal boundary. No original-save proof, protected-world continuation or broad regression is claimed. Next sequential dispatch is18.1 under the same Cargo delivery and tube transfer admission Area.

## Group 18 - One actual tube-capacity calculation, 2026-10-23

All four tasks are complete. CalculateTubeTransferRoom is the sole read-only calculation over exact installed native station membership, queued/reserved stock and optionally pending Transfer designations in the actual loading queue. It accumulates in uint64 and clamps full/overfull stock to zero. A's1024-unit policy remains unchanged. GetTubeStationTransferRoom retains original staging/unlimited-ordinary semantics; the public GetTubeInterchangeTransferRoom adapter verifies exact station/cargo/list identity and distinguishes no applicability from zero available space. Native physical Unload now consumes that adapter without double-counting pending packets or applying another station's tube bound. No donor transport_tube_stock owner or65535-unit duplicate calculation was introduced.

The selected Debug build passed. Isolating the old tube cases exposed a pre-existing generated-language setup dependency; each selected test now explicitly initializes mock presentation and loads current native language before its fixture world. The final focused run passed7 cases/1052 assertions in1.49 seconds: existing interchange transfer, empty/partial/full/overfull buffers, pending arrivals, reserved loading, late capacity loss, conventional rail and road final payment, exact destination/invalid/unallocated lists, foreign footprint membership and mismatched actual receiving queue. No original-save or driven route acceptance was invoked. Evidence: `test-cache/merge-supply/group18-room-retry.txt`; diagnostics: `group18-build.log`/`group18-build-retry.log`, with the first language-failure report retained beside them.

The native public header is now the narrow mandatory navigation for the unified boundary. Next sequential dispatch is19.1; it will register/close the component handoff and exercise a continuous cross-owner legacy-stock scenario. Full original save integration remains in Part2.

## Group 19 - Cross-owner supply/cargo component closure, 2026-10-23

All four tasks are complete. Existing test/CMake owners register both receipt origins, exact refunds, native station/company lifecycle, permanent purpose, partial transfer and unified capacity. Static caller/registration inspection found no production donor _surface_supply_orders vector, self-credit cancellation producer or transport_tube_stock owner; native runtime/controller and transport_tube_cargo remain the sole boundaries. The expanded H handoff publishes exact live exports, rebuild/filter/output locations, component limitations and Part2-only save gates.

A new continuous native-component case refunds70 original source units after30 depart, splits tube admission24/6 at1024 capacity, moves the same30 through conventional carrier/launch handling/Space manifest/orbital stock/service inputs, consumes10 and returns20 without another credit or restored refund eligibility. Shared full custody is checked at each boundary. The real canonical command changes cash by exactly1750; original cap stays100. Initial stock, open floor and empty hull/module bodies are disclosed inputs; this is not a driven-world/constructed-service claim.

The selected Debug build passed on its first attempt. `[surface-supply-cross-owner]` passed1 case/92 assertions in0.30 seconds. Evidence: `test-cache/merge-supply/group19-cross-owner.txt`; diagnostics: `group19-build.log`. No broad regression or original saved-world continuation was run. Structural validation before this closure found47 batches/199 tasks in Part1,36/156 in Part2, matching execution protocols, correct nearest-Area coverage, no batches over8 and no missing local links. Completion now reaches127 of199 Part1 tasks; all Part2 tasks remain open. Next sequential dispatch is20.1 under Derived TBM geometry and packet exchange.

## Group 20 - Shared live references and corridor observations, 2026-10-23

The formerly broad20.1-20.4 required unimplemented reference predicates, corridor/length queries, whole-body stand geometry and phase policy. It is now20/20B/20C, preserving all original scope, with an explicit later Group21 mutation gate. Portal access/turn-holder/steering/PBS activation and corresponding access cases remain22. Source ranges were checked at complete named symbols. No COMPLEX batch was introduced.

All four refined Group20 tasks are complete. Existing GetLiveTBMCutter/GetLiveTBMJob are now public without behavior changes. AwaitFaceTransfer appends identity20, with a pure predicate; original0-19 remain exact. No controller transition or save admission was activated. Original assignment/train owners now supply GetTBMHaulerRoomCells and Is/FindTBMHaulerInCorridor. Only running parts in the paid corridor/either portal end count; stored magazine/depot parts do not. The lowest actual vehicle ID wins deterministically and the longest actual assigned cached consist length supplies whole-cell room.

The selected openttd_tube_test Debug build passed. The first test run exposed an incorrect assumption that a missing cutter invalidates an otherwise valid unbound geological job. The original predicate deliberately treats those facts independently; the corrected test preserves it and downstream geometry must require both live predicates. Final `[tbm-merge-observation]` passed3 cases/179 assertions in0.30 seconds, covering all original/new phase identities, missing/foreign/wrong-model references, assignment membership, multiple train lengths, running versus stored portal parts, deterministic selection and unchanged digests/assignments. Evidence: `test-cache/merge-supply/group20-observation-retry.txt`; diagnostics: `group20-build.log`/`group20-build-retry.log`. Only the pre-existing tbm_projects_gui.cpp signedness warning appeared.

Next sequential dispatch is20B.1 under Derived TBM geometry. The native work path and existing controllers remain authoritative; stand helpers and meeting policy are not yet implemented.

## Group 20B - Pure native stand geometry and folded approach, 2026-10-23

All three tasks are complete. FindTBMFaceStand/GetTBMFaceStandTrackdirs consume the shared original cutter/job predicates and existing canonical work path. The imported bounded placement searches only the last part-count+2 cells; the stand is two path cells behind the actual tail, outside all same-layer body/safety cells and the next cut sweep. A blocked folded prefix uses the native directed track follower over only owned allowed corridor support, respecting the sharp-turn preference. No saved geometry, steering policy or controller transition changed.

Adapted native fixture helpers explicitly disclose train placements and source emptying. Straight/compressed, housing, short, off-path, next-sweep and actual corner-piece cases passed first. A bounded temporary U/fold survey established an independent physical case; its temporary test was then removed. The final folded case earns31 native cuts, fixes the literal40-cell folded prefix and six actual cutter parts, and verifies an otherwise clear stand is unreachable through the body. Explicit retained native rail/support branch components (not a construction/payment claim) then supply a real alternate approach. Bare rails outside the corridor, removed hauler permission and foreign ownership each refuse; admitted original support rows reach the same exact stand. Query digests/support remain unchanged.

The final selected Debug build passed. `[tbm-face-stand]` passed4 cases/114586 assertions in5.25 seconds. Evidence: `test-cache/merge-supply/group20b-final-stand.txt`; diagnostics: `group20b-build.log` and `group20b-final-build.log`. The earlier bounded survey outputs remain disposable under the same directory, with no temporary survey case left in source. All test durations remain below the two-minute split threshold. Original save and driven route proof remains deferred as designed.

Next dispatch is20C.1. Inspection found the existing shared geometry fixture acquires only Standard machinery; actual Deep commissioning/complete-body size requires its own explicit context. The former Deep clause of20C.3 is now20D's three-task native qualification batch with a distinct Area entry gate. No scope is dropped or satisfied by relabelling a Standard train.

## Group 20C - Source-only Face/Bay observation policy, 2026-10-23

All three tasks are complete. TBMMeetingKind/GetTBMMeetingKind/IsTBMReadyForFaceTransfer derive from the live project, original work path, actual complete cutter and assigned-hauler length. Working phases permit the current or previous approach; loaded returns require the exact face unless a genuine assigned hauler is already inside. Cleared returning machinery always resolves Bay. The longest assigned hauler needs the complete tail>=cells+2 margin, and real underground-depot bays retain Bay.

The integration also consumes A's existing FindTBMProjectReferenceLoss before geometry: missing/foreign cutter, consist, job, origin, portal or exchange bay cannot invite a hauler through an otherwise safe-looking pose. This avoids duplicating reference admission and closes the donor query's invalid-bay gap. Pure phase copies can query AwaitFaceTransfer, while ValidateTBMProject still rejects publishing that new phase before23's controller/save activation. No current phase, geometry, movement admission, camera or RNG mutation occurs.

The selected Debug build passed. An initial clearance test exposed the old component Bore/ExchangeAndReturn helper's deliberate ResetTBMProjectState, which clears assignments. The test now reassigns the actual original haulers through native admission after that fixture reload and explicitly checks the longest room value; production policy was not weakened. The empty-return case similarly proves an actually assigned inside hauler rather than a stale assignment. Final `[tbm-face-meeting]` passed3 cases/42221 assertions in2.09 seconds. Evidence: `test-cache/merge-supply/group20c-meeting-retry.txt`; diagnostics: `group20c-build.log`/`group20c-build-retry.log`.

Next sequential dispatch is20D.1 under TBM layer-size characterization. Its entry gate now has tested public meeting exports. The actual Deep acquisition/geometry case remains unchecked; no shorter Standard pose or protected saved world substitutes for it. Operational face activation remains22-23, original-file and driven acceptance remain Part2.

## Group 20D - Actual Deep whole-consist qualification, 2026-10-23

All three tasks are complete. A real native CommissionTBM estimate and synchronized finance-kernel execution acquire model1 with2 Earth/6 Rock hoppers at an actual Underground-to-Deep source, debit exactly the quoted price and preserve original engine/model/part/length identities. Only the upper3x3 assembly floor and starting cash are declared setup. The lower route remains solid until the normal work controller earns its support/cargo/cash changes at the unchanged Deep work rate. Source emptying, supported phase restoration and later poses remain explicit geometry components, not driven gameplay proof.

The initial early-prefix assertion incorrectly required a safe stand even though Deep's wider next-cut sweep legitimately excludes it. The corrected case requires Bay for that short prefix and independently proves the exact later clearance boundary. The actual10-part cutter retains native length80; its real9-unit hauler requires5 whole cells. After16 earned native cuts, tail6 refuses and tail7 admits Face, with unchanged query digests, project/assignment records and native length. Reassignment follows the component's intentional project-store reload; no private live-state installer or length/model edits are used.

The selected Debug build passed. `[tbm-face-deep]` passed1 case/22734 assertions in1.51 seconds. Evidence: `test-cache/merge-supply/group20d-deep-retry.txt`, with actual size diagnostics in its stderr/stdout log; builds: `group20d-build.log`/`group20d-build-retry.log`. The architecture now records the public pure-query boundary and explicitly unactivated controller/save behavior.

Next is21.1 under Whole native face cargo transfer. Preflight and commit are now21/21B so each incoming context has one transaction stage. The original native phase validator still rejects AwaitFaceTransfer until23. Therefore21/21B's components use a supported loaded-return phase;23.1 explicitly retains the installed face-wait replay. No early phase bypass or premature merged-save claim is permitted.

## Group 21 - Whole face-transfer preflight, 2026-10-23

All three tasks are complete. The existing exchange owner exports DescribeTBMFaceTransfer/ValidateTBMFaceTransfer and the original74-tick dwell constant. Ordered native admission covers actor/project/assignment, actual live Face meeting and due spoil, elapsed dwell, intact stationary participants and the moving front's exact stand direction, Keep-only source parts, actual empty hauler-set membership, separate complete Earth/Rock capacity and the conservative source-plus-destination part split allowance. Refusal preserves display identities but emits no partial movement chains/quantities. No execution body or controller call was activated in this batch. Explicit crashed-participant refusal strengthens the stationary guard without changing native recovery ownership.

The adapted fixture uses only already-admitted loaded returns; it rejects attempts to instantiate the future saved face wait. Original native Restore remains the sole installer. Snapshot checks now include every public packet provenance/quantity/route/feeder/link/departure fact, chain IDs, per-part pose/status/actions, weight/power/length and assignment/set/project records. The scoped pool fault overrides only allocation admission and restores the real count before allocation/deletion/iteration; zero/seven slots refuse and the complete eight-part allowance admits the pure proposal. It does not allocate16million meaningless fixture packets.

The selected Debug build passed; `[tbm-face-preflight]` passed5 cases/215181 assertions in9.78 seconds. Cases cover repeated successful descriptions, complete original chains/actual capacity, dwell, short/reversed stand, moving/crashed trains, wrong phase, empty retry, reserved source cargo, loaded destination, insufficient whole capacity, invalid/foreign identities and ordered competing failures with unchanged snapshots. Evidence: `test-cache/merge-supply/group21-preflight.txt`; diagnostics: `group21-build.log`. Next sequential dispatch is21B.1 under the same Whole native face cargo transfer Area; actual packet mutation and cache refresh remain unchecked.

## Group 21B - Native face packet commit and physical cache refresh, 2026-10-23

All four tasks are complete. ExecuteTBMFaceTransfer reruns the whole original preflight, builds both destination vectors before movement, then shifts native Keep packets into the existing matching hoppers in stable set/part order. Each shift empties a source or fills a destination; debug checks assert the exact actual debit. An impossible exhausted destination is an internal invariant failure, never a partial success. No wagon changes train and no source stock is recreated. Both actual fronts receive CargoChanged/UpdateAcceleration and normal dirty notifications before the original set/assignment advances once to Loaded/CollectLoaded with visited_origin=false.

The adapted successful loaded-return case proves full original provenance/tonne/feeder conservation, exact chains, capacity/action counts, both native physical weights, original origin-delivery restriction and single bookkeeping increments. A deliberate two-fragment capacity boundary adds exactly one native split, keeps the original remainder identity, fills the first hopper then the second, and retains exact10-per-unit feeder shares. Refusing full/seven-slot allocation faults execute no prefix; normal capacity then succeeds. Empty retries and unelapsed dwell retain the complete native snapshot. No successful physical allocation runs under a fault-injected count.

The first build identified one omitted direct include for GetWindowClassForVehicleType; adding its existing vehicle_gui.h owner fixed compilation. The selected Debug build passed. `[tbm-face-transfer],[tbm-face-preflight]` passed9 cases/272569 assertions in12.38 seconds, including the directly affected shared fixture's native loaded-packet metadata. Evidence: `test-cache/merge-supply/group21b-transfer.txt`; diagnostics: `group21b-build.log`/`group21b-build-retry.log`. The test helper now gives declared held packets real native loading evidence rather than appending an unloaded packet to a carrier.

The live transfer header replaces the donor-header reading prerequisite for the next area. Structural validation before this closure found51 batches/211 tasks in Part1,36/156 in Part2, matching protocols, correct nearest-Area coverage and no missing local links. Completion reaches147 Part1 tasks. Next sequential dispatch is22.1 under TBM movement, replanning and player controls. Actual AwaitFaceTransfer activation/replay is still23; source/copy/merged-writer integration remains Part2. No original saved-world continuation or Git write occurred.

## Group 22 - Pure turn, portal and stand eligibility, 2026-10-23

All three tasks are complete. The canonical exchange owner now exports IsTBMFaceTurnHolder, IsTBMHaulerAdmittedThroughPortal and IsTBMHaulerFaceStand. Turn selection uses lowest eligible assignment ID, full empty capacity and actual other-hauler corridor occupancy. Own-company membership, intact trains, exact portal-pair identity and exact allowed stand directions are required. A loaded train already beyond its Source key retains physical outbound clearance without acquiring an inbound turn. These are derived observations; native callbacks are integrated in22B-22D.

The selected Debug build and `[tbm-face-turn]` passed3 cases/40427 assertions in1.88 seconds. Tests distinguish assignment priority from vehicle identity, stored from running parts, short capacity, stopped/foreign/crashed candidates, own versus unrelated portal pairs, exact versus reverse stand pieces and the final tail clearing the housing. Repeated queries preserve actual snapshots and original stores. Reports: `test-cache/merge-supply/group22-turn.txt`; build diagnostics: `group22-build.log`. Completion reaches150 Part1 tasks. Next sequential work is22B directed steering; actual installed face-wait activation remains23.

## Group 22B - Directed steering and native YAPF consumers, 2026-10-23

All three tasks are complete. GetTBMHaulerSteeringTarget now returns an optional exact tile/trackdir view: the face turn uses the real stand, other empty returns use their validated bay door, and loaded/stored/finished legs retain ordinary orders. Actual Source traversal permission cannot revive an ended steering destination or line-end hold. The complete live caller inventory includes yapf_destrail.hpp (not just cpp/h), the line-end predicate and focused tests; archived source-epoch fixtures remain immutable.

Native destination/cost handling now recognizes both cutter and hauler work endpoints, including a closed Source dead end, while the separate cutter flag retains its original two-way-signal policy. Assigned loaded legs disable shared segment caching because actual cutter-body clearance can change. No ordinary order, destination, phase, cargo or simulation RNG is written by observation/search.

The selected Debug build passed with only the previously recorded unrelated signedness warning. `[tbm-face-steering]` passed4 cases/27987 assertions in1.45 seconds. Actual destination-template tests prove exact/reverse pieces, Source queue versus stand, flag reset after an executed native face transfer, finished/stored work with a real part still on the key, and unchanged snapshots/orders. A real non-reserving YAPF search reaches the genuine Underground depot; an unrelated train retains ordinary cached destination handling. Actual through-pair and face routing callbacks remain22C and installed wait-loop acceptance remains23. Evidence: `test-cache/merge-supply/group22b-steering.txt`; build: `group22b-build.log`. Completion reaches153 tasks.

## Group 22C - Native portal, support, PBS and signal admission, 2026-10-23

All four tasks are complete. The actual portal work and tile-entry owners now share own-pair face turn/outbound admission, Source occupancy and full-chain ingress/departure orientation. A loaded origin leg cannot newly enter the old bay; a part already on its key may finish. Native magazine departure checks the door-side block, clears stale pathfinder-lost state only on actual departure, and retains the genuine depot-bay controller gate. Support admission recognizes every actual assigned hauler part, requires its own or released support permissions and refuses the cutter body/safety gap. Cutter return and post-transfer work hold until the assigned corridor occupant clears; ordinary service orders retain their existing precedence. Far exit replacement now reports the real remaining-hauler refusal.

IsLayerPortalOpenToNetwork is the single ordinary track-status/signal-block boundary. Closed commissioning ends retain separate local blocks; actual running parts occupy their own end while off-line magazine parts do not. Native storage arrival releases only an unowned-by-other-running-train key reservation, and arrival/departure/shaft movement update their actual signal blocks. PBS accepts only the admitted magazine's inward piece or the winning hauler's exact stand direction. Public header descriptions now state these boundaries. No geothermal drawing or later Tube-admission changes were imported with the overlapping donor portal diff.

The initial broad compile (triggered by the new refusal string's generated header) found a missing direct PBS include. The exchange and signal portal direct dependencies were fixed, and the selected Debug rebuild passed. `[tbm-face-access]` passed10 cases/165580 assertions in8.95 seconds, covering actual tile callbacks, native non-reserving face search, queued/contending/foreign traffic, Source occupancy, remote depot storage, partial magazine orientation, body/gap clearance, exact corner PBS directions, return/post-transfer holds, service precedence and exit refusal. `[tbm-portal-signals]` passed1 generated case/89322 assertions in7.36 seconds across no/block/path signals and reservation off/on, driving the native cutter and ordinary depot train through local occupancy changes. Its explicitly declared cutter emptying is not a driven hauler loop. Reports: `test-cache/merge-supply/group22c-access.txt`, `group22c-signals.txt`; build logs `group22c-build.log`/`group22c-build-retry.log`. Completion reaches157 tasks.22D now owns actual hauler step/turn integration; installed AwaitFaceTransfer acceptance remains23.

## Group 22D - Existing hauler cycle, native turnaround and handoff, 2026-10-23

All three tasks are complete. ObserveTBMHaulerInputs now observes the actual eligible face/bay, due load, inside clearance, original origin and physical set. The original pure step and AdvanceTBMHauler perform the complete native face transfer, normal bay exchange or no exchange; actual origin visit/disposal still returns the same set. Stored stale PathfinderLost and a temporarily occupied Source no longer manufacture BlockedLine, while actual running loss remains observable. Native lost-corridor recovery compares both real ends against the existing destination.

TurnTBMHaulerAtStand uses the original native reversal owner only once the whole chain is out of storage. BackUpTBMHauler additionally enforces this at its public boundary, releases the original reservation and either turns a stopped body or requests one native braking reversal. Ordinary player stop/crash/storage and pending-turn controls retain authority. No new controller, saved stand cache, wagon relinking or rendering side effect was introduced.

The selected Debug build passed. `[tbm-face-cycle]` passed6 cases/85152 assertions in4.14 seconds; directly affected `[tbm-face-steering],[tbm-face-turn]` passed7 cases/68410 assertions in3.19 seconds. Cases exercise original loaded-return phase, whole native transfer/turn, exact packet/chain/order retention, original origin visit and actual emptied-set return, whole-load refusal, an already-inside empty hauler clearing an empty returning cutter, partial storage deferring physical turnaround, stale versus current lost flags, and one scheduled braking reversal. These headless components have no open layer camera; declared arrival/disposal poses are not a fully driven Surface loop. Positive directed-chamber recovery and full visible/hidden installed face-wait continuation remain23's integrated proof, alongside replan/lifecycle coverage.

Reports: `test-cache/merge-supply/group22d-cycle.txt`, `group22d-callers.txt`; build: `group22d-build.log`. The map-layer architecture now names the live rail/turn/step/backing exports and exact activation limits. Completion reaches160 Part1 tasks. The new phase remains inadmissible until23; original-file/merged-writer compatibility remains Part2. No original-save ticks, protected Space continuation, Git writes or filesystem deletion occurred.

## Human-requested thread stop after22D,2026-10-23

The human explicitly stopped execution after22D and requested a handover for a new thread.22D is complete; Group23 has no implementation changes and remains unchecked. The next action is to split the oversized plan again without overwriting existing Part2. [The thread handover](m2o23-1-come-space-i-handover.md) records live contracts, activation limits, exact scoped results, native build commands, fixture traps and cross-feature identity/custody boundaries. Part1:54groups/220tasks/160done; Part2:36groups/156tasks/0done. No running tools, Git writes or filesystem deletion remain. The full merge is unfinished.

## Four-plan partition after22D,2026-10-23

At the user's request, all still-open23-36 tasks moved from Plan1 into the new Plan2. The user-renamed Plan3 was split at its natural Area transition:37-53 remain there;54-72 moved to the new Plan4. The existing Global execution protocol is copied verbatim, Area blocks stay with their governed groups and each file has scoped completion conditions. Final merge completion remains in Plan4. Completed Plan1 has40 groups/160 tasks, all done; Plan2 has14/60, Plan3 has17/74, Plan4 has19/82, all unchecked. All90 groups/376 task IDs survive exactly once;160 completed and216 open states are preserved. Only two completed task descriptions received later-plan navigation corrections, with no change to their obligations or evidence.

Current command/receipt/asset/custody handoffs and architecture now route future work to the correct plans. Original identity JSON fixtures retain their historical metadata and exact bytes. Optional consultation/provenance rows remain scoped and non-transitive. The22D handover now starts at the actual Plan2/23 entry, with no pending file split; its recommended finer Group23 decomposition remains future planning work. This change performed document edits and structural/link/task-conservation checks only. No gameplay code, task execution, unit test, saved-world continuation, Git write or deletion occurred. The user-requested execution pause remains in force.
