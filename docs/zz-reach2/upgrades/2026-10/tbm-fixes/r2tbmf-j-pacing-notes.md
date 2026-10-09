# R2TBMF J - Half-speed cutting and work-fraction migration

Current invocation contract: continuation commands below describe dated evidence. Use the relocated runner with `--prior-run <exact-invocation-id>` for accepted feedback replay or pacing continuation; saves and comparison logs come from that invocation. Pacing seed/check migration additionally requires its original rate/build prerequisite, which this cleanup does not recreate or substitute. See [R2FC](../r2fc-file-cleanup.md).

Owner: [fixing Group5](../r2tbmf-tbm-fixes.md#group-5---halve-cutting-speed-and-preserve-partial-work). Architecture: [vehicles](../../../architecture/vehicles/archi-vehicles.md), [savegames](../../../architecture/savegame/archi-savegame.md). Group4 is closed in [H](r2tbmf-h-motion-notes.md#group4-closure). Do not repeat its old-rate acceptance or reread geothermal work while implementing pacing.

**Artifact-layout migration (2026-10-09):** this plan consumes the parent [Development artifact layout](../r2mlvs-multilayer-vehicles-and-structures.md#development-artifact-layout), implemented by [R2FC](../r2fc-file-cleanup.md). It supersedes earlier docs asset/review/helper placement and fixed runtime-config destinations below. New native homes use `mlm/runtime/`, generated feature reviews use `mlm/reviews/`, logs remain under `logs/`, and reusable code/curated inputs keep their code/test owners. Paths and focused native/cold verification are complete; R2FC records the results and retained-input identities. Repository-relative executable commands run from the checkout root through their context-aware wrappers. Historical pass records remain dated evidence and are not new acceptance claims. No gameplay task is completed by this migration and the separate post-demo fixture-pruning gate remains.

## Current scope and reading

Group5 was split prospectively into cutting speed/migration (Group5) and exact fractional Earth/Rock accounting (Group5B). All original requirements survive in those groups. Current Group5 compaction count0. After compaction re-read fixing Global/user requirements, nearest Group5 local reading/tasks and this note; increment its count. Split again after more than three compactions. Group5B and advice/parking/orders/exitB/geology remain pending.

Required current symbols: `_tbm_models` in `tbm_model.cpp`; fractional accumulation in `AdvanceTBMFaceWork` (`tbm_work.cpp`); once-per-front call in `Train::Tick`; `TBMPChunkHandler::Read` in `saveload/tbm_sl.cpp`; version394 `TBMHalfCuttingRate` in `saveload/saveload.h`; the pace branches in `tbm_motion_reload.cpp` and runner I. Historical budget tables in `r2mlvs1-e-ground-and-space-budgets.md` have stale purchase/running-cost examples: use measured native values, do not retune unrelated money.

## Implementation and retained inputs

Standard changes1/1→1/2 geological work per simulation tick; Deep3/2→3/4. Travel speed, power, purchase/annual charges and per-cell work/support/Earth/Rock terms are unchanged. **Spoil is not halved yet.** No extra work call or work owner was introduced.

Version394 keeps the existing U16 project work remainder. `TBMPChunkHandler::Read` validates old files against their original divisor (Standard1, Deep2), then doubles only that numerator. New files validate against current2/4 divisors and are never scaled again. This also runs during preview. Unknown models/oversized fractions refuse before rows enter staging. Geological partial work, completed receipts, parts, packets and cash are untouched.

Before tuning changed, the native game drove the actual Group3C fresh Underground/Deep accepted area saves into their first partial cut. No pose, phase or remainder was edited. Genuine version393 seeds:

- `mlm/reviews/tbm/pacing/import-20261009/saves/tbm-pace-underground-393.sav`: partial1, fraction0/1; SHA256 `76b7da97ccec8412d5df85fdc8a6e352db2a7c2729170fe9aa41f3b9ac32728d`.
- `mlm/reviews/tbm/pacing/import-20261009/saves/tbm-pace-deep-393.sav`: partial1, fraction1/2; SHA256 `a49f16c7fe0deea010e6bb1a3ed8fd9e7959ef0ec2e7427d3f2afa932af4657f`.

Old-seed logs: `logs/r2tbmf-j-pace-seed-{underground|deep}.log`; producer test-executable SHA256 `4417d852abd7df70540879e631e234b903a6e884a150d87665068d535d5b8c3b`. Runner I's `--pace-seed-layer` is historical and deliberately asserts old model rates; **do not regenerate these seeds with the new tuning**.

## Verified native pacing

`src/tests/multilayer/tbm/native_motion.py --pace-check-layer underground|deep` loads the actual393 seed, checks the migrated fraction and saves a new394 copy before any movement/work. It then uses ordinary complete game frames through the next atomic cut, checking partial work/remainder every tick. No manually advanced work or simulated arrival is used.

- Standard Soft: remaining887 old ticks→**1,774 new ticks**, fraction0/2,7,114 assertions.
- Deep Hard: remaining1,183 old ticks→**2,366 new ticks**, fraction2/4,9,482 assertions.
- `--pace-resaved` reloads the actual394 copy. Both initial and next-cut records match the migrated run exactly; the fraction is not converted twice.

Logs `r2tbmf-j-pace-new-{underground|deep}[-reload].log` retain actual full part/cargo/reservation state and owner digests. Initial states match historical seed observations except the expected project digest change from Deep's equivalent1/2→2/4 representation. The loader and first real cut preserve work identity/cargo/terrain/cash/RNG. The runner protects original390, published393, configurations, source seed and exact startup-script bytes. Each native run takes less than four seconds.

An independent read-only inspection using `saved_table`/`owned_chunks` confirms exact393→394 preservation of every decoded TBMP field except the doubled remainder. All inspected vehicle, order, cargo, station, industry, company, geology, route/pass, portal, support, exchange, receiver, date/RNG and map chunks match byte for byte. Standard changes no owned chunk; Deep changes only TBMP. This custody/migration inspection is complete; actual malformed-file acceptance is still pending.

Current save files: `tbm-pace-underground-394.sav`, `tbm-pace-deep-394.sav` in the same haulage fixture directory. Game/test394 build succeeded in `logs/r2tbmf-j-half-rate-build.log`. Native checks used test SHA256 `67337bd924ef8a5f8740bb6c8aaf09add33943c1faa043c922245e7a709814fb`.

## Remaining acceptance and immediate handoff

The first focused `[tbm-model],[tbm-work],[tbm-cost],[tbm-route],[tbm-backing],[tbm-coverage]` run passed18/19 cases; one old late-obstruction fixture still waited the old number of ticks. It is corrected and verified: `logs/r2tbmf-j-boundary-build.log` finished successfully; **142,906 assertions/19 cases** pass in `logs/r2tbmf-j-half-rate-final-focused.log`. A justified `[tbm-travel-service],[tbm-travel-uturn]` spot check passes **360,807 assertions/2 cases** in `logs/r2tbmf-j-movement-spot.log`. No live build/test remains. Task5.1 isX;5.2/5.3 remain pending for their full acceptance. No full regression was run.

Shared `TBMWorkRig::BoreOneCut` now computes remaining simulation ticks from the actual model/divisor/partial work/remainder, including the one-tick zero-material commit. `WorkToLastTick` checks Standard whole work and half-work remainder on every tick. Affected route/cost/coverage assertions now use two Standard ticks per work unit. Further tests may contain old1tick=1work assumptions; update only ones actually selected for relevant verification rather than sweeping unrelated suites.

Before closing all of Group5: add actual malformed old/current fraction preview and full-load evidence; cover Mixed/Basalt duration and fractional retry/interval subdivision with focused meaningful cases. Exact decoded migration/custody comparison is already complete above. Existing helper modules `src/tests/tbm_logistics_save_fixture.py` (`saved_table`, `table_header`, `table_record`) and `geology_save_fixture.py` (`chunks`, `encode_chunk`, `gamma`) can inspect/mutate only a separately named malformed fixture. Do not edit historical source bytes.

Important deferred maintenance: Group4 motion-generation filenames currently say393; a future new-rate rerun must write separate394/current files and logs, preserving H's historic393 saves and baseline. Existing Group1/3 runners similarly name historical393 checkpoints: avoid overwriting those while testing new rates/yields. Group5 uses its separate pace files/logs. No Git writes or commit have occurred.

## Group5 closure

All three Group5 tasks areX on2026-10-09. Earlier pending paragraphs above are historical. No build/test remains live. Group5B fractional spoil is next; read its nearest local list rather than reloading these historical work-rate investigations.

[Runner K](../../../../../src/tests/multilayer/tbm/pacing_files.py) independently verifies every saved owner and decoded TBMP field, then produces eight separate malformed files: each model in393/394, at its invalid divisor and at65535. All eight fail both actual preview and ordinary startup loading (**16 native refusals**) with `Invalid TBM work fraction`. Previews retain the entire observed running world; failed full loads never dispatch the game-start callback. The original four valid pacing files, demo/config/startup bytes are protected. Logs `logs/r2tbmf-k-{underground|deep}-{393|394}-{value}-{preview|load}.log` retain each boundary.

`src/tests/tbm_pacing.cpp` declares initial homogeneous solid-rock patches before any work exists, then buys actual sufficiently sized wagon groups, drives the cutter physically to its face and runs ordinary vehicle frames. It checks every supported hardness with and without a pause after the first fractional work tick. No pose/progress is fabricated. `logs/r2tbmf-k-hardness-pacing.log` passes **162,105 assertions** over the16 model/hardness/pause combinations, including Standard's two unchanged Basalt refusals. Complete first-cut active ticks:

| Model | Soft | Mixed | Hard | Basalt |
| --- | --- | --- | --- | --- |
| Standard | 1,776 (was888) | 3,552 (1,776) | 5,328 (2,664) | refuses |
| Deep | 1,184 (592) | 1,184 (592) | 2,368 (1,184) | 4,736 (2,368) |

Each37-tick pause preserves the exact partial geological job and fractional numerator. Only active cutting enters this table; unchanged travel/idle costs keep their original owners. Existing focused boundary-full/refusal/retry tests continue to pass.

The native pacing observer additionally saves after the first actual resumed frame. Standard now has partial1 plus1/2; Deep has partial2 plus1/4. Files `tbm-pace-{underground|deep}-fraction-394.sav` are actual native current saves, not edited legacy headers. `--pace-check-layer L --pace-fraction` reloads them without migration change and reaches the same next atomic cut, clock, owner digests, all physical parts/cargo/reservations, cash and RNG as uninterrupted execution. Logs `r2tbmf-j-pace-new-L-fraction-reload.log` retain this exact interval subdivision proof. Final native tests used SHA256 `61df1c45c0e9d8171fc3a91abee60b72d9797eec549d07c6931ec989f6b90fe3`. The ordinary game394 rate implementation remains the successfully built preceding game executable.

Earth/Rock yield and all prior receipts remain unchanged at this checkpoint. Group5B must implement half-tonne Earth conservation for odd Basalt cell counts before enabling half output everywhere.
