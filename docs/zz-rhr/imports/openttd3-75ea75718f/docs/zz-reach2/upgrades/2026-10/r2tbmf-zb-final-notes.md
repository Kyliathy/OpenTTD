> **Historical donor snapshot — not an execution plan.** OpenTTD3 `75ea75718f`, imported2026-10-23.
> Original: `docs/zz-reach2/upgrades/2026-10/r2tbmf-zb-final-notes.md`; exact source SHA256 `04157509635ded5c4aaad7e7311756c28354199d96b8355caceff0cf9bc6a06a`.
> Current integration: [M2O23-1](../../../../../../../zz-reach2/upgrades/2026-10/m2o23-1-come-space.md); storage and evidence roles: [archi-historical-debug.md](../../../../../../../zz-reach2/architecture/archi-historical-debug.md). Original completion claims, instructions and human-review limits below remain dated; they grant no new execution, spending, publication or deletion authority.

# R2TBMF ZB - Final integration and playable checkpoint

Owner: [fixing plan](r2tbmf-tbm-fixes.md), Group11. Status2026-10-09: tasks11.1-11.4 are verified. Optimized/Debug focused checks, connected native journeys, reciprocal exits and service/reuse checks passed. All100 saved chunks match at11 corresponding checkpoints. Only required player feedback11.5 remains pending. No build/test remains running. Group10 is verified in [Z](r2tbmf-z-geology-notes.md#group10-closure). Master remains paused. Latest workspace instructions prohibit Git writes.

## Final verification batch

Reuse the compiled production behavior and existing native observers. `tbm_exit.cpp` adds an isolated current-geology44-cell seed using real commissioning/paid logistics. `tbm_feedback_runtime.cpp` gains a fresh-output Group11 mode: native project selection, straight preview/cancel/requote/confirmation appends six cells to the same machine, records a paused playable ready state, observes actual cutting/haulage/advice, then stops the real completed consist clear of B's terminal footprint locally. `TBMExitReplay` then loads that actual file, uses the existing native portal/hangar input, parks the same machine, requests final release through its real project window, pays for common rail and a public station/junction, and observes ordinary freight reaching both sides while the cutter stays stored at B. Native saves connect the stages; no recreated second world may substitute for the same machine's continuation.

[Runner ZB](../../../../../../../../src/tests/fixtures/cleanup/source-epochs/63fe2c6497a79f50-r2tbmf-zb-final-checks.py) runs serial bounded stages with fresh files/logs and restores supplied saves/config/startup and historical review images. Related focused scopes are planning/UI, vehicles/orders/rates and geology/paid cuts. These are one justified final affected regression, run against completed optimized/Debug builds, not the repository's full unrelated regression suite. Repeat only failures or checks affected by corrections.

## Playable checkpoints and observed behavior

The user requested the current playable checkpoint as the default demo. `.\play.ps1 demo`, `underground-mines` and `tbm-work` now open [underground-demo-tbm-399.sav](../../../../../../../../src/tests/fixtures/haulage/planning/underground-demo-tbm-399.sav). `tbm-plan` opens the earlier44-cell state for actual extension input; `tbm-exit` opens the completed, locally withdrawn machine ready for B construction; `tbm-tunnel` opens the released public tunnel. These are byte-identical named copies of the verified native files below, not regenerated scenarios. The older repaired393 demo remains under `underground-mines-legacy`, and original390 is unchanged. Featured entries prefer the retained repository snapshot over a same-named generated save. All use the normal optimized executable and existing `build/play.cfg`. [Player instructions](d2um-underground-mines.md) identify each starting state and its actual next action.

The fresh regional world uses real paid commissioning/logistics. Native mouse input opens Projects and the actual vehicle, pauses, previews/cancels/requotes and confirms a straight extension from44 to50 cells on **cutter5/job0**. The real journey completes50 cuts, five physical exchanges and zero false too-few-orders warnings across68,715 frames. Its six-cell extension quote is36 Earth/216 Rock,18 supports and2,664 geological work ticks; total completed output is423/1,278 whole tonnes. The same world is saved with the whole TBM stopped locally clear of B, loaded by the actual application, and continued through Underground portal-picker input, paid B construction/connection, selected-hangar travel, complete parking, native project release input, paid support conversion and an ordinary junction. Freight0 reaches destination1 and returns to its original origin while the same cutter remains wholly stored at B, taking11,710 continuation frames.

Demo packaging,2026-10-09: [launcher checks] (historical reference: `../../../../logs/r2tbmf-zd-demo-check.log`) verify all aliases, numbered entry3 and Debug selection through `play.ps1 -DryRun`; player configuration, startup script and both older supplied saves retain their exact hashes. No C++ change, rebuild or repeated simulation suite was needed. The four named copies match their already loaded and physically continued native sources byte-for-byte:

| Demo save | Verified source | SHA256 |
| --- | --- | --- |
| `underground-demo-tbm-399.sav` | `tbm-final-relwithdebinfo-ui-1-ready.sav` | `1669002c38dc808e17bb5d2436fc39eb5cc4dbc3d3576f85dc893ca46bdfd41f` |
| `underground-demo-planning-399.sav` | `tbm-final-relwithdebinfo-seed-1-exit-final-accepted.sav` | `b8b3ccba313225f213ef323cdaf54eb01d1f2cc1584d97704c0bea3a1ad62565` |
| `underground-demo-exit-399.sav` | `tbm-final-relwithdebinfo-ui-1-local-stage.sav` | `e742b9901b25503f8ce2dedc77e233a205d5df5b726c9950adf8b3a71d609168` |
| `underground-demo-tunnel-399.sav` | `tbm-final-relwithdebinfo-exit-1-exit-traffic.sav` | `3626b195b6c9c78ffc5796f197ae40621e03304e04df59d5d6f64140ca563a60` |

This request prepares the user's demo session; it does not itself supply the remaining player feedback or waive task11.5.

| Final optimized slice | Evidence | Result |
| --- | --- | --- |
| Paid regional44-cell seed | [seed] (historical reference: `../../../../logs/r2tbmf-zb-RelWithDebInfo-seed-1.log`) | 61 assertions |
| Real Projects/preview/work/advice/local staging | [UI] (historical reference: `../../../../logs/r2tbmf-zb-RelWithDebInfo-ui-1.log`) | 68,908 assertions |
| Same-file B/parking/release/public freight | [exit] (historical reference: `../../../../logs/r2tbmf-zb-RelWithDebInfo-exit-1.log`) | 58,521 assertions |
| Vehicles/orders/pacing/service/reuse/release | [vehicles] (historical reference: `../../../../logs/r2tbmf-zb-RelWithDebInfo-vehicles-1.log`) | 722,220 assertions /28 cases |
| Repeated50-cell Surface-script and Underground-command journeys | [repeat] (historical reference: `../../../../logs/r2tbmf-zb-RelWithDebInfo-repeat-exit-1.log`) | 114,328 assertions; both directions per world |
| Corrected partial-work, reduced-yield and capacity fixtures | [corrections] (historical reference: `../../../../logs/r2tbmf-zb-RelWithDebInfo-corrections-1.log`) | 5,553 assertions /14 cases |

The vehicle slice includes actual normal depot service retaining partial work and resuming the next real cut, plus remote release/new-job reuse without purchasing another cutter. The connected399 chain retains accepted/ready/local-stage files and B loaded/built/commissioned/withdrawing/moving/parked/released/freight-outward/traffic files. Earlier actual398 remote-reassignment reload, work-rate migration and malformed/lifecycle cases remain referenced by [W](r2tbmf-w-order-control-notes.md#group8-closure), [J](r2tbmf-j-pacing-notes.md#group5-closure) and [Z](r2tbmf-z-geology-notes.md#group10-closure).

## Visual comparison and provenance

Final Debug [game/test build] (historical reference: `../../../../logs/r2tbmf-zb-debug-build-1.log`) passed with one project/two compiler workers; no heavyweight native tests overlapped compilation. Its bounded route-size conversion emits C4267 at `excavation_route.cpp:445`; the preceding `MAX_EXCAVATION_ROUTE` guard bounds the value, so this is not an observed overflow. Final verification used the following completed binary without further C++ edits:

| Debug slice | Evidence | Result / elapsed |
| --- | --- | --- |
| Planning/UI | [planning] (historical reference: `../../../../logs/r2tbmf-zb-Debug-planning-1.log`) | 24,040 assertions /22 cases /17.36s |
| Geology, paid cuts and saved state | [geology] (historical reference: `../../../../logs/r2tbmf-zb-Debug-geology-1.log`) | 1,786 assertions /29 cases /5.30s |
| Work/model/support/movement | [work] (historical reference: `../../../../logs/r2tbmf-zb-Debug-vehicle-work-1.log`) | 195,037 assertions /13 cases /12.56s |
| Advice/parking/settlement/public rail | [parking] (historical reference: `../../../../logs/r2tbmf-zb-Debug-vehicle-parking-1.log`) | 123,552 assertions /7 cases /22.19s |
| Native orders/hangars/service/reuse | [orders] (historical reference: `../../../../logs/r2tbmf-zb-Debug-vehicle-orders-1.log`) | 403,631 assertions /8 cases /59.08s |
| Reciprocal50-cell command/script journeys | [repeat] (historical reference: `../../../../logs/r2tbmf-zb-Debug-repeat-exit-1.log`) | 114,328 assertions /29.38s |
| Actual UI-to-completed-work file | [UI] (historical reference: `../../../../logs/r2tbmf-zb-Debug-ui-1.log`) | 68,908 assertions /108.42s |
| Same-file B/park/release/public freight | [exit] (historical reference: `../../../../logs/r2tbmf-zb-Debug-exit-1.log`) | 58,521 assertions /10.34s |

The first combined Debug vehicle process exceeded the120-second runner limit without reporting an assertion failure. That incomplete log is retained as diagnostic evidence. Splitting only that scope into the three rows above passes all28 cases/722,220 assertions, matching optimized coverage without rebuilding or rerunning unrelated passing scopes. The single final affected regression is now complete; no full-repository regression was run.

[Cross-build save comparison] (historical reference: `../../../../logs/r2tbmf-zb-debug-optimized-chunks-1.log`) finds byte-identical raw payloads for **every one of100 saved chunks** at accepted, ready, local-stage, B built/commissioned/withdrawing/moving/parked/released, freight-outward and final-traffic checkpoints. No metadata or simulation owner was excluded. Both executions reach local staging at68,715 frames and public completion11,710 frames after its real reload. The current production source is unchanged by the final fixture corrections, so the retained current399 legacy/malformed/preview/network results in Z and rate/phase/depot migration proofs in J/W remain applicable; those passing matrices were not repeated.

Native captures use1280x733 at150% UI scale, matching the retained reproduction scale. The [palette](../../../../../../../../../OpenTTD3/docs/zz-reach2/upgrades/2026-10/assets/reviews/tbm-final-relwithdebinfo-ui-1-palette.png) is396x232 versus the original786x569 and has one contextual instruction. The [reviewed quote](../../../../../../../../../OpenTTD3/docs/zz-reach2/upgrades/2026-10/assets/reviews/tbm-final-relwithdebinfo-ui-1-quote.png) names the existing train, numbers the six forward cutter positions and includes extra paid space and time. The [running view](../../../../../../../../../OpenTTD3/docs/zz-reach2/upgrades/2026-10/assets/reviews/tbm-final-relwithdebinfo-ui-1-running.png) shows actual “Boring” and “Bringing empty hoppers” status. The [release-ready project](../../../../../../../../../OpenTTD3/docs/zz-reach2/upgrades/2026-10/assets/reviews/tbm-final-relwithdebinfo-exit-1-tbm-hangar-final-release-ready.png) shows50/50, the selected Surface hangar at(13,60) and retained parked machinery. These images were visually inspected; their automated capture does not replace the user's usability judgement. New material/survey captures and the preserved old-map comparison are in [Z](r2tbmf-z-geology-notes.md#group10-closure).

| Artifact | SHA256 |
| --- | --- |
| Optimized production game | `c74fb78916e14b9fdcffe22854de712588aaafbdfbfd1fa22f4d5f8eaef33411` |
| Optimized corrected test executable | `e1fda8590651061e548e02d39237bb745f64176f39d357e9d761f7b987cbd022` |
| Debug production game | `5acca278f4b7ffa8d585f4ea2a80df8d64aa540f712401fecf9d67fa7087cec4` |
| Debug test executable | `edcd35a3b9fa2fffc4bb3ce930f4d419b3da08d501e639075eb7816e75064da4` |
| Native English | `cf151f6db12a4551668c2be7acf488f03f71440bee28814910cc9c82ac2be993` |
| Native observer config | `396092066483e9eddf9713618ceb44eb391d124677c0be5a4bf14d417a809862` |
| Player config at publication | `18f5df22c65e4c0a79fe36cc5f26551c8a9607f8f9c432f340139dea4a94cf87` |
| Packed native GRF | `4e27c95fc061b3327243bfcd5714ad4ac47248d997fe35f68eb20d43ad2abc9d` |
| Ready work save | `1669002c38dc808e17bb5d2436fc39eb5cc4dbc3d3576f85dc893ca46bdfd41f` |
| Completed public tunnel save | `3626b195b6c9c78ffc5796f197ae40621e03304e04df59d5d6f64140ca563a60` |

## Remaining acceptance

The [optimized correction build] (historical reference: `../../../../logs/r2tbmf-zb-optimized-corrections-build-1.log`) and all14 affected cases pass. The eight earlier failures concerned old whole-work-tick assumptions, old18/36 rather than9/18 Soft receipts, and capacities that now legitimately fit the reduced spoil. Fixtures now earn actual whole work, accept the valid smaller Soft consist and exercise genuine Hard-rock/independent-capacity refusals without weakening atomicity. Passing optimized vehicle/exit/network scopes were not repeated. Player instructions, current owning handoffs, frozen historical-budget corrections and launcher choices are updated. `git diff --check` passes; its logged CRLF conversion warnings are informational. Final Debug verification is complete above.

The user still needs to assess the published checkpoint's usability, route clarity, material appearance and pacing; task11.5 stays open until that feedback arrives and any required corrections are handled. No complete-plan or master-resumption claim is made. Git writes remain prohibited by the user's workspace instructions.

Documentation closure11.4: updated the actual player guide/demo/launcher, parent AD-5, frozen pacing/yield examples, Plans4/5/7/14 public handoffs and GUI/vehicle/save architecture. Existing Plan2/3/13 and map-layer handoffs already record delivered terminal handover and material rendering. The new checkpoint/guide links resolve to retained files. A read-only diff audit currently finds newly completed checkboxes in `r2tbmf-tbm-fixes.md` and `r2mlvs17-geothermal-energy-and-industry.md`; both names belong in the later commit message if Git writing becomes explicitly authorized. No commit was attempted and no geothermal work was resumed.

The user asked whether `assets/reviews` could be deleted before committing. At inspection it held124.5MiB, including66 raw PPMs/67.7MiB and343 tracked files. Running the game does not depend on the folder, but `activate_m1_art.py` / `review_m1_art.py` consume its JSON reports and hash-verified PNGs; plans also link captures. Wholesale deletion would remove those inputs/evidence. No review JSON references PPM files, making raw captures the first pruning candidates after reference review. No deletion was requested or performed. Later Debug capture files add further generated evidence; the quoted size is the inspection-time measurement.

Precommit cleanup,2026-10-09: after the Debug captures, all82 raw PPMs total110.6MiB and remain untracked. A narrow root `.gitignore` rule now excludes only this review directory's `*.ppm` intermediates. All82 files remain locally present; current JSON/PNG evidence stays eligible for Git and is untouched. `git check-ignore` with NUL-delimited paths verifies the exact scope; `git diff --check` passes. No rebuild is needed for this ignore-only change. The published save hashes still match the table, and no changed C++ source is newer than the verified Debug executable.

Prepared commit message, conditional on the remaining player acceptance and an explicit override of the user-supplied Git-write prohibition:

```text
Fix TBM excavation, parking and public tunnel workflows

Record completed work in r2tbmf-tbm-fixes.md and
r2mlvs17-geothermal-energy-and-industry.md.

Deliver compact project planning, deterministic movement, slower cutting with
whole-unit spoil, managed-hauler advice, selected hangars, safe release, far-exit
construction and material-owned geology art. Retain the verified geothermal
completion already present in the worktree.

Validate optimized/Debug workflows and identical native state at eleven saved
checkpoints. Keep raw review PPM intermediates local through a scoped ignore rule.
```

No staging or commit was performed. Player feedback and Git-write authorization remain unresolved; the master has not resumed.
