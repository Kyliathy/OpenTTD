# R2TBMF L - Whole-unit spoil tuning

Owner: [fixing Group5B](../r2tbmf-tbm-fixes.md#group-5b---reduce-spoil-using-native-whole-cargo-units). Architecture: [vehicles](../../../architecture/vehicles/archi-vehicles.md), [savegames](../../../architecture/savegame/archi-savegame.md). Group5 cutting speed/migration is closed in [J](r2tbmf-j-pacing-notes.md#group5-closure).

**Artifact-layout migration (2026-10-09):** this plan consumes the parent [Development artifact layout](../r2mlvs-multilayer-vehicles-and-structures.md#development-artifact-layout), implemented by [R2FC](../r2fc-file-cleanup.md). It supersedes earlier docs asset/review/helper placement and fixed runtime-config destinations below. New native homes use `mlm/runtime/`, generated feature reviews use `mlm/reviews/`, logs remain under `logs/`, and reusable code/curated inputs keep their code/test owners. Paths and focused native/cold verification are complete; R2FC records the results and retained-input identities. Repository-relative executable commands run from the checkout root through their context-aware wrappers. Historical pass records remain dated evidence and are not new acceptance claims. No gameplay task is completed by this migration and the separate post-demo fixture-pruning gate remains.

## Latest user decision

On2026-10-09 the user rejected half-unit cargo as unnecessary expansion and explicitly chose **keep1 tonne Earth per Basalt cell**. Preserve native whole cargo units. The approved Earth/Rock table is Soft3/6, Mixed2/12, Hard1/20, Basalt1/24. Standard1/2 and Deep3/4 cutting rates remain as verified in Group5. Travel, support charges, running costs, disposal prices and mineral reserve debits retain their existing meanings.

The user also requested faster progress and fewer checks. Batch edits, then one focused verification burst for the active group; rerun only failures or checks affected by a correction. Do not repeat verified movement suites or design a replacement fractional system.

## Current implementation and next checks

Group5B compaction count1. Read fixing Global/user decisions and the closest Group5B Local Mandatory Reading after compaction, then only the active symbols below. Earlier versions of this note proposing physical half-tonne custody are **superseded**, not mandatory design.

The fractional-only source/tests and integrations have been removed through ordinary workspace file edits, preserving earlier UI, continuation, movement and half-speed changes. No Git write commands were used. Version395/TBSF, wagon/receiver/job half fields, generic loading/capacity changes and fractional presentation are removed. Current save version remains394 for the verified work-rate migration. No395 evidence files were published. Historical exploratory L logs are not current acceptance evidence.

The shared `_cut_terms` table in `excavation_cut.cpp` now supplies the agreed integer yields. Quotes, complete cuts, work estimates, passes and extensions already consume it. Native cargo transport and save descriptors need no changes. Verify no rejected fractional symbols remain, compile once after the batch, and use existing hardness/native-cut tests plus one-cargo-full/refusal checks. Then use a representative old save and the retained50-square haulage setup. All5B tasks are now verified; closure evidence follows below.

Preserve Group5's393/394 seeds and baseline logs. Do not overwrite them using current tuning: new output files/logs need distinct names. Group1B's `src/tests/fixtures/haulage/planning/tbm-feedback-haul50-393.sav` is the paid50-square baseline; the original user demo390 remains intact. The current playable renamed demo is393.

## Active owners

- `excavation_cut.cpp`: `_cut_terms`, `QuoteExcavationCells`, `PrepareExcavationCut`, `EstimateExcavationCut`.
- `tbm_work.cpp`: existing `PlanTBMSpoil`, `CaptureTBMMaterial`, `PrepareTBMCutParticipation`, atomic packet allocation/commit.
- `excavation_pass.cpp` and `AppendExcavationJob`: existing independent whole cargo capacity batching; old conservative return points remain valid with lower yields.
- `src/tests/tbm_pacing.cpp`: real first-face cuts for both models/all supported hardnesses; add direct expected whole-yield checks to this existing test.
- `src/tests/tbm_work.cpp`: boundary full-cargo, allocation-refusal and paid-cut checks. Initial overload setup must distribute cargo across actual wagons if reduced yields require a larger preload.
- `src/tests/tbm_feedback_runtime.cpp` and retained native runner: actual50-square haul cycle measurement.

## Group5B closure

All four revised whole-unit tasks are complete. The optimized game/test executables build, and `[tbm-pacing],[tbm-work]` passes242,416 assertions/six cases in1.6 seconds (`logs/r2tbmf-l-whole-spoil-focused.log`). Both models/all supported hardnesses reach actual first faces, produce the exact approved native packet counts, preserve interrupted work, and retain independent full-cargo and allocation-refusal behavior. Source audit finds no rejected fractional-spoil symbol or schema. The shared production table is the only runtime change for the yield reduction.

[Runner M](../../../../../src/tests/multilayer/tbm/whole_spoil.py) executes two ordinary native startups/20,000-tick continuations in about2?2.5 seconds each. It immediately saves separately named394 copies and compares all retained vehicle, packet, industry, company, geological receipt, map and project owners with the393 inputs. Only the already verified work-remainder representation may differ. Old-work input preserves210Earth/420Rock at cursor6; continuation ends at cursor20 with288/576. The long-haul input preserves90/180 at cursor2 and ends with228/456. No old material or completed floor is rescaled. Original inputs/configs/startup bytes remain unchanged.

Long-haul balance against the retained original20,000-tick sample:

| Measurement | Earlier tuning | Current whole-unit tuning |
| --- | ---: | ---: |
| Final completed cuts | 18/20 | 20/20 |
| WaitForEmpties frames | 11,931 | 7,622 |
| Unblocked Bore frames | 2,819 | 6,815 |
| LogisticsStall frames | 40 | 42 |
| First retained loaded set, Earth/Rock | 90/180 | 90/180 |
| Second loaded set, Earth/Rock | 120/240 | 60/120 |
| Third loaded set, Earth/Rock | 108/216 | 54/108 |
| First departure?next bay admission | 8,620 ticks | 10,027 ticks |
| Second departure?next bay admission | 10,553 ticks | 7,234 ticks |

For the same first four new cuts, active work increases1,480?2,960 ticks while output falls120/240?60/120. The shorter empty-wagon wait is36.1% for this scenario; actual round trips still depend on receiver interval timing and existing loaded material. This does not promise arbitrary-distance haulage throughput. The current sample ends during final return/haulage, with the cutter carrying24Earth/48Rock and hauler22/44; it does not prove parking or release.

Current native train upkeep is88,439 internal Money across20,000 ticks; total company money falls9,585,265?9,493,990. The measurement sums the retained yearly running-cost ledgers across New Year. Original baseline logs lack the equivalent upkeep total, so no numeric cost delta is invented. Running/disposal/support prices were not retuned. The old-work continuation's train upkeep is60,484. Logs: `logs/r2tbmf-m-whole-spoil-summary.log`, `...-haul50.log`, `...-old-work.log`. New saves: `src/tests/fixtures/haulage/tbm-spoil-{haul50,old-work}-loaded-394.sav`.

No build/test or Git write is live. Group6 assigned-hauler advice is next; do not reopen cargo fractions or repeat the completed movement/pacing suites.
