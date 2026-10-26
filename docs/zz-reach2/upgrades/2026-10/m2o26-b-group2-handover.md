# October 26 merge handover: donor reference build and Group 3 entry

@see [historical build architecture](../../architecture/archi-historical-debug.md), [Part 1](m2o26-merge-source-save-compatibility-oct26.md) and [execution observations](m2o26-a-execution-notes.md). This note records a machine-transfer boundary; the plan's Global and nearest Area Mandatory Reading remain the execution contracts.

## Stop boundary

The user requested a stop immediately after Group 2. Groups 1 and 2 are complete, with all ten tasks marked `[X]` in Part 1. All three RelWithDebInfo reference targets built successfully, and final source/profile/tool/image verification passed. Group 3 and every later group remain unstarted. Execution has stopped at this boundary.

The worktree contains the source-staging helper and its focused checks, planning updates and this handover. Donor application/test source is held in an independent reconstruction tree under `test-cache/m26s`; the destination application remains at its original integrated implementation.

## Repository identity

| Root on this machine | Branch | Pinned HEAD |
| --- | --- | --- |
| Destination `D:\Codez\Sandbox\OpenTTD` | `mlm-mlv` | `530daca23b0590d5cec7ea2a2332648a3935f932` |
| Read-only incoming `D:\Codez\Sandbox\OpenTTD4` | `mlm-mlv` | `3d45ad4a4c53186ce03ac2e8bec4616b9db189b7` |

Common ancestor: `cfeff84db52f8a3753a43c2a770a6363c62b2c67`. Neither root has been checked out, staged or committed by this execution. Incoming status was clean at the last check; verify again before resuming. No cleanup or recursive deletion was performed.

The Group 1 inventory has 5,469 path dispositions, including 22 textual-conflict paths / 39 conflict blocks. Fresh donor output uses version445; the destination's separate version441 lineage remains intact. Later groups own the combined446/schema2 implementation. A newly produced445 save cannot stand in for a deleted historical441–444 original.

## Files to transfer with the worktree

Transfer these local edits explicitly; several are untracked:

| File | Purpose |
| --- | --- |
| [package_transfer.py](../../../../src/tests/multilayer/support/package_transfer.py) | Modified existing owner; new `stage_donor_source` copies only declared exact bytes into a fresh project-owned tree. |
| [donor_source_stage_check.py](../../../../src/tests/multilayer/support/donor_source_stage_check.py) | New six-case non-destructive staging checks. |
| [Part 1](m2o26-merge-source-save-compatibility-oct26.md) | Current task status, reading contracts and next Group 3. |
| [Part 2](m2o27-merge-runtime-qualification-oct26.md) | Already split continuation plan; unchanged during Groups 1–2. |
| [Execution notes](m2o26-a-execution-notes.md) and [this handover](m2o26-b-group2-handover.md) | Actual observations, build scope, stop boundary and transfer instructions. |

Both original repositories, retained fixtures and their required Git objects are still needed for later comparison and integration. OpenTTD4 is the incoming source; the historical DonorB/OpenTTD3 references inside existing fixture catalogues designate a different lineage.

## Ignored artifacts to transfer explicitly

Git-visible edits alone do not carry the reconstructed reference. Preserve the following project-relative directories and their hidden files:

| Directory | Required contents and role |
| --- | --- |
| `test-cache/m26s/` | Safest transfer is the whole independent tree: 6,014 pinned source/input members, `.donor-source-manifest.json`, `.ottdrev`, the recovery profile, all three fresh `build/RelWithDebInfo/*.exe` targets, runtime assets, and `mlm/reviews/build/selected-native/20261026-175844-90770062b0/` receipts/source epoch/logs. Keep relative paths together. Generated objects can be large. |
| `test-cache/merge-oct26/` | Path/input inventory, exact donor-source member request, disposable `donor_recovery.py` and `donor_reference_verify.py`, plus retained synthetic staging-check fixtures. These make the observations and verification reproducible. |
| `logs/merge-oct26/` | Comparison preview; source/profile/tool/dependency/runtime reports; native build driver and staging-check output; final build wrapper and identity verification report. |
| Existing destination `savegames/retained/` | Preserve the original worlds referenced by the sole retention journal, especially `mlm/reviews/space/checkpoint-load/merge64-native-relwithdebinfo-01196be93d/saves/checkpoint-after.sav` and `mlm/reviews/space/checkpoint-load/merge64-native-debug-c3820c19e4/saves/checkpoint-after.sav`. These are the actual D441 inputs already inventoried; source code and the journal alone cannot reproduce their original bytes. Carry retained config exports with their journal-owned payloads too. |

The source-stage manifest SHA256 is `4aea5c30b1b1fd03215ebf88d26a77a47ffaa94eb6aea9fa21675d4ba026fa02`. It certifies the selected source copies, independently of executable acceptance. Build-generated profile/revision files are additional explicit inputs recorded by the build receipt.

If rebuilding is necessary on the next machine, probe actual tools and use a fresh contained build directory when the existing CMake tree is incompatible. Stored CMake paths and copied object files do not establish tool availability. The current dependency cache was verified against the same pinned requirements before reuse; native objects/executables were compiled afresh. Do not rerun `stage` or `cache` over their existing destinations.

## Reference build evidence

Native receipt SHA256: `79700d5195a55cde77d06bf62724c3b0b1fe21fc947bfe191d8e69b8ef79ec15`; source-epoch SHA256: `b661bf3063e5d9a7bd99916b8b4359c8373c85afea47f6b3f1fc0fa0e9556bc1`; recovery-profile SHA256: `966de1642f2504579583338e0cc737691825c05f8da00c691ebdb8de02974488`. Full observations are in `logs/merge-oct26/donor-reference-verification.json`, SHA256 `4ab0289db21d54d61086e51db8c6154d8c0a2712a091a19d05fe1da49d25d3a7`. This audit verified all 6,014 source/copy pairs, ten actual tool executables, runtime distribution, all three image identities, successful/reaped native stages and six unchanged destination contracts.

| Built target | SHA256 |
| --- | --- |
| `openttd` | `6063dc066a22c49591c8b0bbbf7e2c8d03b36b52ea1bd768ddbf062404ceea20` |
| `openttd_test` | `9536b556b7983611ec6a87d55e89430ff0bde34c746b18088b268286924e9496` |
| `openttd_space_fixes_test` | `c184e045e003dd5e5c6eb73ac1f1d91a6837194d30705a37003f52e000a2bc36` |

The existing incoming C4018 signed/unsigned warning at `space_return_target.cpp:71` remains. Native test executables were compiled and linked; no native suite or game replay was run in Group 2.

The build uses the destination's existing `native_build.build_selected` owner with explicit staged root, one worker, memory admission/owned-process guards and sequential targets `openttd`, `openttd_test`, `openttd_space_fixes_test`. RelWithDebInfo is the Group 2 reference configuration. The two-configuration combined build remains Part 2 Group 27.

The recovery profile changes only contemporary tool observations. Incoming dependency/content requirements and application/test bytes remain pinned. Actual tool probes found MSVC 19.44.35229, MSBuild 17.14.60.43110, toolset 14.44.35207, SDK 10.0.26100.0, CMake 3.31.6-msvc6, GRFCodec/NFORenum 6.2.0 and GNU patch 2.7.6. The earlier donor compiler/MSBuild/patch pins were unavailable; this is a fresh contemporary donor reference.

External paths that must be rediscovered on the other machine include `build/.native-tools/vcpkg-dcd5788f49cb4490`, `test-cache/graphics-toolchain/grfcodec620/grfcodec-6.2.0-windows-win64/bin`, Visual Studio BuildTools, the Windows SDK and Git. The current disposable recovery driver records their old locations; inspect/update explicit overrides before a new build. Preserve its previous identity alongside the old receipt.

Focused staging verification passed six tests in approximately 0.145 seconds. The new verifier rechecks every pinned donor/staged member, the source epoch, profile, revision, runtime resources, all three images and successful native-stage receipts. It runs no game and no native suite:

```powershell
python -B test-cache/merge-oct26/donor_reference_verify.py --source D:/Codez/Sandbox/OpenTTD4 --report-name donor-reference-verification-next-machine.json
```

Replace the explicit source path if the incoming checkout moved. Run from the destination root after transferring the full required closure. Reports live in `logs/merge-oct26/`; an existing report name refuses overwrite. The original `donor-reference-verification.json` records build-machine tool hashes using `--capture-build-tools`; transfer verification omits that flag and needs no old compiler installation. Build receipts describe the original machine; fresh runtime invocations need new identities and private output roots.

## Resume at Group 3

Read the plan's Global Mandatory Reading and the nearest **Area Mandatory Reading - Independent donor recovery** block, then verify Group 2's source/profile/image receipts and both repository heads. Continue at **3.1**, sequentially. Resolve each existing producer from the staged donor root, with fresh run IDs and private configurations; do not execute a producer in the incoming checkout.

The producer chain remains retained405/418 no-op migration plus retained heavy440 → `edit` → `continue` → paired visible/hidden `lifecycle` → `prepare-high-id`. Group 3's owning source notes describe exact arguments, phase saves and the issuance-prefix limitation. No441–445 Space checkpoint has been regenerated during Groups 1–2. Deleted historical logs/screenshots and441–444 intermediate saves remain unavailable; fresh445 outputs will carry their actual provenance.

If the build/input gate fails, stop dependent replay tasks and preserve the concrete error; pure source inventory/format analysis may continue. Private-pool edits, merged binaries, arbitrary caches and self-derived expected outputs cannot replace the donor reference.

Preserve original saves, configs, the selected foundation and sole retention journal. The selected foundation SHA256 remains `8cc7f8743c2922537d49b1745641049692a3c297a1e1d72208526748c76a1f6e`; canonical `openttd.grf` remains `c04fbf0d5525edc53250cfbd963f5e3bbe0d0bce655a3553c73b76483a8e8744`. Cleanup-policy reconciliation belongs to later groups. The user's no Git writes, no destructive commands, no recursive deletion and sequential/no-subagent instructions continue to apply.
