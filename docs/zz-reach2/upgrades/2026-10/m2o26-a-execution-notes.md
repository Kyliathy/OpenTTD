# October 26 merge: sequential execution notes

@see [savegame architecture](../../architecture/savegame/archi-savegame.md) and Part 1 (`d49c3596:docs/zz-reach2/upgrades/2026-10/m2o26-merge-source-save-compatibility-oct26.md`, deleted 2026-10-28; superseded by [m2o28](m2o28-1-merge-oct28.md)). This is an implementation record, not an additional mandatory reading queue.

**User-directed stop boundary:** finish Group 2 and leave the [machine-transfer handover](m2o26-b-group2-handover.md), then stop before Group 3.

## Group 1 completed

Both heads, branches and common ancestor match the plan. Donor status is clean; destination initially had only the two untracked plans. The 5,469-path inventory records 5,247 D-retained, 179 incoming-only imports, 22 textual conflicts, 16 shared clean paths and five cleanup policy deferrals. The textual preview has 39 blocks. No renames/deletions occur in base-to-tip changes.

Machine observations: `test-cache/merge-oct26/execution-inventory.json`; preview: `logs/merge-oct26/merge-tree.txt`. Regenerate path sets with read-only `git diff --name-status <base> <tip>` for each pinned root, and the process-local donor object alternate with `git merge-tree --trivial-merge <base> <D> <S>`. Re-resolve original references through `lineage-originals.json`, `multilayer-inputs.json` and `save_retention.resolve_exported_save`, then recompute SHA256 and unchanged outer headers. These outputs are observations, not an input catalogue.

Verified originals include shared403 (the existing donor-namespaced catalogue alias resolves the unchanged common file), A434/438/440, B434, both D441 outputs and donor405/418/440 seeds. Donor runtime cache remains absent; use the native reconstruction chain. The observation JSON pins asset, catalogue, journal, profile, recipe registry and foundation hashes, and records focused harness commands with their input owners.

Selected foundation SHA remains `8cc7f8743c2922537d49b1745641049692a3c297a1e1d72208526748c76a1f6e`; canonical GRF remains `c04fbf0d5525edc53250cfbd963f5e3bbe0d0bce655a3553c73b76483a8e8744`. Unadapted cleanup fixtures, recursive TemporaryDirectory teardown and Git-mutating fixtures are forbidden. Only launched owned-process watchdogs may supervise children. Staging/runtime outputs belong in `test-cache/merge-oct26`; logs in `logs/merge-oct26`; config changes use private copies.

## Group 2 completed

The new `package_transfer.stage_donor_source` entry passed six focused tests in 0.145 seconds, including case-insensitive metadata refusal, hard-linked input refusal, source-byte races, source-target overlap and absence of preservation/Git callbacks. Test fixtures are retained under `test-cache/merge-oct26/stage-contracts` with no teardown. The actual copy includes 6,014 pinned donor members / 911 MiB, with maximum staged member path 250 characters at the deliberately short `test-cache/m26s` root. It excludes Git/generated/history-source stores and uses independent files.

To rebuild the selected phases: `python -B test-cache/merge-oct26/donor_recovery.py stage`, then `profile`, `inspect`, `cache`, and `build --configuration RelWithDebInfo` (Debug separately). The source-member request is `test-cache/merge-oct26/donor-source-members.json`; driver source stays disposable under test-cache. Do not rerun staging/cache against the already-existing outputs; choose a new owned root. The exported `.ottdrev` binds the actual donor commit without Git metadata. The recovery profile changes tools only; dependency/content pins match exactly. All application/test source bytes remain pinned donor inputs.

The source manifest is `.donor-source-manifest.json` under the stage. Profile/tool/cache phase reports and the build driver log are under `logs/merge-oct26`. Actual native build receipts are produced beneath the staged root by the existing native-build owner. Global contracts are unchanged; the package-transfer Area ranges were updated after adding the new public function.

Exact focused filters were confirmed against delivered test registrations: lineage uses `[save-lineage]`; relevant supply leaves include `[surface-supply-merged-wire]`, `[surface-supply-custody-policy]` and `[surface-supply-departure]`. The observation JSON now includes the MSVC standalone compile command and all available supply leaf tags; these were inventoried without executing unrelated tests.

Reference-build scope: Group 2 uses a fresh RelWithDebInfo donor445 image. The task does not require a duplicate donor Debug build; the mandatory two-configuration combined build remains Part 2 Group 27. No donor Debug acceptance is claimed. The completed reference build run is `test-cache/m26s/mlm/reviews/build/selected-native/20261026-175844-90770062b0`; its `evidence/build-openttd.txt` and reports retain progress/resource gates. The final verifier confirmed the profile-phase `.ottdrev` unchanged and the generated native revision bound to donor commit `3d45ad4a4c53186ce03ac2e8bec4616b9db189b7`.

Final source/profile/tool/image verification passed: 6,014 donor/staged member pairs; current complete source epoch; ten actual tool executable hashes; canonical runtime distribution; all three fresh target images; successful/reaped native stages; and six unchanged destination pins including the original journal/foundation. Native receipt SHA256: `79700d5195a55cde77d06bf62724c3b0b1fe21fc947bfe191d8e69b8ef79ec15`; profile: `966de1642f2504579583338e0cc737691825c05f8da00c691ebdb8de02974488`; source epoch: `b661bf3063e5d9a7bd99916b8b4359c8373c85afea47f6b3f1fc0fa0e9556bc1`. Full observed identities are in `logs/merge-oct26/donor-reference-verification.json` (SHA256 `4ab0289db21d54d61086e51db8c6154d8c0a2712a091a19d05fe1da49d25d3a7`). The disposable verifier is `test-cache/merge-oct26/donor_reference_verify.py`; its original invocation used `--capture-build-tools`.

| RelWithDebInfo target | SHA256 |
| --- | --- |
| `openttd` | `6063dc066a22c49591c8b0bbbf7e2c8d03b36b52ea1bd768ddbf062404ceea20` |
| `openttd_test` | `9536b556b7983611ec6a87d55e89430ff0bde34c746b18088b268286924e9496` |
| `openttd_space_fixes_test` | `c184e045e003dd5e5c6eb73ac1f1d91a6837194d30705a37003f52e000a2bc36` |

All selected build stages passed. The incoming `space_return_target.cpp:71` retains its C4018 signed/unsigned warning. Native test executables were built but were not run; no runtime checkpoint regeneration or full regression suite was executed. Per the user's request, stop after Group 2. Resume at Group 3 only on the next execution, after reading its Global/Area contracts and verifying transferred evidence.
