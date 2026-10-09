# Commissioned electrical riser continuation

Owner: [Plan17 Groups5B-5](../r2mlvs17-geothermal-energy-and-industry.md#group-5b---establish-actual-riser-construction-before-electrical-export), under [master10.3](../r2mlvs3mp-master-plan.md). Architecture: [map layers](../../../architecture/map-layers/archi-map-layers.md), [savegames](../../../architecture/savegame/archi-savegame.md). Recorded2026-10-09.

**Artifact-layout migration (2026-10-09):** this plan consumes the parent [Development artifact layout](../r2mlvs-multilayer-vehicles-and-structures.md#development-artifact-layout), implemented by [R2FC](../r2fc-file-cleanup.md). It supersedes earlier docs asset/review/helper placement and fixed runtime-config destinations below. New native homes use `mlm/runtime/`, generated feature reviews use `mlm/reviews/`, logs remain under `logs/`, and reusable code/curated inputs keep their code/test owners. Paths and focused native/cold verification are complete; R2FC records the results and retained-input identities. Repository-relative executable commands run from the checkout root through their context-aware wrappers. Historical pass records remain dated evidence and are not new acceptance claims. No gameplay task is completed by this migration and the separate post-demo fixture-pruning gate remains.

## Current cursor and verified scope

The user explained that a harness error staged queued messages and explicitly corrected the queue to **resume TBM fixes, then commit when complete**. This master is paused again. Preserve the riser work described here; TBM implementation is active and incomplete. No new commit/tag/push has been made. The eventual authorized completion commit must name both plans if their new checked tasks remain in it.

**Plan17 task5B.1 is complete;5B.2 is the later resume point.** Original Group5 lacked a real riser construction producer, so prerequisite Group5B now precedes it. Pure admission lives in `src/geothermal_riser.h/.cpp`, with actual-source tests in `src/tests/geothermal_riser.cpp`. Riser machinery occupies its own2x2 excavated floor on the sending rock plane, with one boundary cell cardinally adjoining the actual source housing. It preserves the housing/through-route. Both actual endpoint owners and the canonical Connected incarnation must agree. The1980 gate applies even to an Underground/Surface relay. A direct Deep/Surface road elevator reads only those two endpoints and leaves intermediate Underground solid.

The frozen content-owned construction record now exposes `transfer_capacity_w`:200,000 for a riser, zero for collector/powerhouse construction. Existing capital/material/work terms stay120,000/STEL4 RFMT4 ELEC2 BATY2/296ticks. A successful pure quote allocates no industry, link, stock or energy and charges no money. Native funding must still revalidate geometry, actual pool/cash capacity and portal incarnation.

[Optimized build2](../../../../../logs/r2mlvs3mp-z-r-riser-admission-build-2.log) completes game/test targets. [Focused final checks](../../../../../logs/r2mlvs3mp-z-r-riser-admission-final-checks.log) pass1,592 assertions/nine cases, comprising riser admission, shared mutation and frozen utility contracts. No full regression ran. The first build's PowerShell exit1 came from CMake CMP0177 developer warnings; both targets linked, and the subsequent build exited0. Current admission tests use explicitly supplied open floor and accepted native portal records, not a paid excavation/bootstrap claim.

## Construction implementation next

Reuse actual native Industry/IIDS/TIDS, command-kernel charging, delivered accepted stock/history and once-per-game-tick machinery construction. Existing collector owners provide the pattern, but a riser does not acquire a vent claim or a heat producer. Do not bind its stock to `GeothermalSiteID` or misuse the collector's saved vent association.

Relevant existing symbols/files:

- `geothermal_definitions.cpp`: exact native package mapping/capability/complete layout validation. Collector localID8300 uses tile IDs8300..8308; allocate a distinct family for riser tiles. The existing entry point registers only collectors.
- `geothermal_collector.cpp/.h`: actual Industry binding, exact remaining-bill receipt and296 prepaid labour ticks. Native stock is the sole delivered inventory; a saved receipt prevents a second material debit. Paused/date/owner/retry checks are already explicit.
- `industry_cmd.cpp`: destructor/removal stock guard, production/tile-loop guards, `CreateNewIndustry`'s collector branch near2154, funding wrapper near2371, native tick near1267 and ordinary production refusal near3125. Do not edit source during a live verification build.
- `economy.cpp`: actual construction delivery near1089 and ordinary input-consumption exclusion near1194; company takeover near500. New riser hooks must preserve actual surplus packets and native accepted history.
- `saveload/industry_sl.cpp` and `afterload.cpp`: actual collector transport/acceptance; Plan17 task5B.4 requires real riser save transport before activation. Current latest version393 remains unchanged by pure admission.

Read Global and the nearest **Electrical risers and metered transfer** Local Mandatory Reading after compaction, with the above construction symbols scoped to5B.2. No meaningful compaction of this new prerequisite has yet occurred.

## Existing transfer restriction to reconcile in Group5

`RegisterUtilityPowerLink` already allocates one shared metered electrical edge for actual same-owner Connected reciprocal endpoints and a saved portal revision. Its public comment explicitly leaves physical construction/payment to the feature builder. It cannot establish completed riser machinery by itself.

The existing `TransferLayer` mutation preserves the original producer and interval, debits a source reservation and credits the already requested destination. **It deliberately refuses re-export of imported shares.** `PrepareUtilityMutation` near925, `CommitUtilityMutation` near1024 and `ValidateUtilityPowerState` near1505 all encode one-hop provenance. Intermediate Deep→Underground→Surface therefore needs an explicit shared-owner extension with saved lineage and bounded cycle/capacity checks; do not introduce a second generator or simply remove the current guards. Existing single-hop/old-file policy must remain explicit and tested.

The focused shared mutation producer passes61 assertions/two cases in [prerequisite checks](../../../../../logs/r2mlvs3mp-z-r-riser-prerequisite-checks.log). Actual existing transfer-file tests are hidden under `[.utility-power-transfer-file]` and require `OTTD_UTILITY_FILE_INPUT`, `OTTD_UTILITY_FILE_CASE` and their registered startup callback. They write fixed utility fixture names; preserve originals when extending/rerunning. Do not claim this investigation reran those native files.

## Native definition prefix retained at queue correction

The shared definition implementation now validates/registers collector and riser families through the same canonical native managers. Collector IDs8300..8308 remain unchanged; riser industry/local tile family8310..8313 accepts the four exact construction cargo meanings and exposes no output. `geothermal_riser.h` publishes its pure definition/capability queries. This prefix is callable in tests only: normal package activation, funding, delivered-stock/work hooks and save transport remain unimplemented. Do not mark5B.2 complete from it.

Both optimized targets linked in [definition build](../../../../../logs/r2mlvs3mp-z-r-riser-definition-build.log); PowerShell again reports the CMake developer-warning exit1 without a compiler/linker error. [Focused collector/riser definition tests](../../../../../logs/r2mlvs3mp-z-r-riser-definition-checks.log) pass7,639 assertions/six cases. Preserve `geothermal_definitions.cpp`, the new riser definition test and English facility string. No production riser can be funded yet.
