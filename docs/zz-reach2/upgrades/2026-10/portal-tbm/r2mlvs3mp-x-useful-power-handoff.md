# Useful Power Handoff

Owner: [Plan 12 Groups 7-9](../r2mlvs12-solar-farms-mirrors-and-eti.md#area-mandatory-reading---native-useful-power-ledger-and-consumers). Dispatch: [master 9.1](../r2mlvs3mp-master-plan.md#group-9---supply-the-later-integrations-shared-producers). Architecture: [map layers](../../../architecture/map-layers/archi-map-layers.md). Earlier transaction policy: [settlement contract](r2mlvs3mp-v-utility-settlement-contract.md).

The native utility producer is implemented in [utility_power.h](../../../../../src/utility_power.h) and its `.cpp` owner. The content contract remains in `multilayer_content.h` / `multilayer_utility.h`. Feature owners register actual company/layer consumers and commit their complete physical actions through this boundary. They cannot edit captured account fields or create a second grid.

**Artifact-layout migration (2026-10-09):** this plan consumes the parent [Development artifact layout](../r2mlvs-multilayer-vehicles-and-structures.md#development-artifact-layout), implemented by [R2FC](../r2fc-file-cleanup.md). It supersedes earlier docs asset/review/helper placement and fixed runtime-config destinations below. New native homes use `mlm/runtime/`, generated feature reviews use `mlm/reviews/`, logs remain under `logs/`, and reusable code/curated inputs keep their code/test owners. Paths and focused native/cold verification are complete; R2FC records the results and retained-input identities. Repository-relative executable commands run from the checkout root through their context-aware wrappers. Historical pass records remain dated evidence and are not new acceptance claims. No gameplay task is completed by this migration and the separate post-demo fixture-pruning gate remains.

## Public use and ordering

1. Register producer/consumer IDs through `RegisterUtilityPowerProducer` / `RegisterUtilityPowerConsumer`. IDs are ledger-issued, monotonic and retained after retirement. Consumers list explicit eligible same-owner, same-layer sources; proximity grants no connection.
2. `PublishUtilityGeneration` integrates the actual source rate in watts over synchronized 27-ms economy ticks once per source/interval. Joules and the saved 0..999-millijoule remainder use checked integer arithmetic. Zero publication is valid; duplicate publication refuses.
3. The synchronized system actor `OWNER_DEITY` calls `AllocateUtilityPower` once for the complete interval request set. Stable allocation reserves original source slices without calling them consumed. Company callers cannot close the global interval independently.
4. The asking company reads `GetUtilityAllocation(consumer)`. It returns only the current owned remaining grant and unmet demand; repeated queries and estimates change no state.
5. `PrepareUtilityMutation(kind, grant, quantity, destination, link)` produces a transient proposal. `CommitUtilityMutation` rechecks ownership, epoch, saved operation sequence, exact proposal and available source slices. Consumption requires a real whole `UtilityMutationParticipant`; validation or commit refusal preserves energy and the physical owner. `LayerRecipeCommitParticipant::GetElectricalGrant` joins the existing complete recipe transaction to this owner.
6. Release unused offers, then rotate explicitly with `BeginUtilityPowerInterval` under system authority. Rotation expires reservations and current town effects, retains source carry/lifetime settlement and invalidates old tickets. Source publication, allocation, physical settlement and rotation are ordered by the calling synchronized feature scheduler, never rendering or timer registration order.

`Consume`, `ReleaseUnused` and `TransferLayer` are distinct mutation kinds. Every accepted mutation advances the saved sequence; replay after save/load refuses. The maximum rate is 1,000,000,000 W, opportunity 1,000,000 ticks, account energy 27,000,000,000,000 J, producers/consumers 4,096 each, and whole-ledger source connections/shares 16,384 each.

## Directed flow and effects

`RegisterUtilityPowerLink` requires actual owned, date-valid Connected portal endpoints and their commissioning revision. Transfer debits only original local slices, credits an already requested destination within route capacity, and retains the original generator. Imports cannot be exported again in that interval. Physical sealing/demolition retires the route and expires only unused imports; settled use survives and the donor receives no refund. A rebuilt route gets a new ID. Physical riser construction, cost and supplies remain Plan 17.

| Consumer | Frozen demand | Accepted observable response |
| --- | --- | --- |
| ProcessorBoost | 50,000 W; 1,350 J/tick | Optional bonus work, proportional to actual accepted energy, capped at 25%. |
| TubeWork | 20,000 W; 540 J/tick | Mandatory service advances only paid whole work ticks. |
| AssemblyWork | 30,000 W; 810 J/tick | Mandatory assembly advances only paid whole work ticks. |
| TownGrowth | 5,000 W; 135 J/tick | One native Surface town offer per interval, capped at 25% against its normal growth baseline. |

`ApplyUtilityWorkEffect` validates and commits the existing physical work owner through `UtilityWorkParticipant`. A missing consumer, exhausted offer, zero opportunity or rounded zero benefit advances nothing and consumes nothing. This fixture proves the public effect boundary; full native mine, tube and assembly scheduling remains with their feature owners.

`BindUtilityTownConsumer` binds one actual Surface town and prevents supplier stacking. `ApplyUtilityTownEffect` consumes its exact offer while changing the real town rate without RNG or CustomGrowth ownership. The immutable native profile selects Native authority; no ETI adapter is supported. `CmdTownGrowthRate` takeover expires unused offers and suppresses the current native bonus. Relinquishment restores the normal baseline without renewing the old offer. Town deletion unbinds before pool-ID reuse. Ordinary growth recalculation applies one current bonus to the normal baseline, never to a previously boosted rate.

## Persistence and acceptance

Save version **385**, `UtilityCompanyLifecycle`, transports `UPLH` revision5 (96 bytes) and `UPLD` (25 fields,104-byte rows). The existing active scalar now explicitly persists consumer retirement; genuine381..384 consumers migrate as active. Original meters/carry, reservations, settlement/lifetime totals, directed links/imports, operation sequence and actual town association/posted/suspended effect remain authoritative. Utility feature bit1024 and the field-wise UtilityRoot digest include consumer liveness. Full loading validates after physical company/profile/map/portal/town reconstruction; independent preview refuses malformed authority without changing the live ledger. Explicit readers preserve actual retained381..384 files, with no invented effect or renewed retired grant.

Actual company takeover/removal calls `ChangeUtilityPowerCompanyOwnership` before tile/pool identity changes. Current live account/source lineage transfers on takeover; physical routes retire and unused imports expire without a refund. Deletion retains inactive IDs and exact settlement, clears town eligibility and expires unused original slices. A reused company ID cannot access an old consumer/source, and its later takeover cannot rewrite the former incarnation's tombstones. [Current lifecycle/native/normal-game evidence](../r2mlvs14-savegame-determinism-and-lifecycle.md#group-15b---complete-delivered-electrical-company-ownership) closes this delivered row while capsule/thermal producers remain pending.

Focused verification:

- [Current owner/effect checks](../../../../../logs/r2mlvs3mp-w-utility-effects-final-checks.log): **829 assertions / 14 cases**, including partial mandatory supply, proportional processor caps, rounded zero benefit, whole-owner refusals, exact source exhaustion and GS takeover before and after use.
- [Actual native town files](../../../../../logs/r2mlvs3mp-w-utility-town-file-checks.log): five create/applied/custom/relinquished/expired cases; exact utility and ordinary CITY chunks survive load, with no replay benefit or RNG change.
- [Normal game loader/writer](../../../../../logs/r2mlvs3mp-w-utility-town-normal-checks.log): current applied town state survives ordinary loading/resaving with exact utility/CITY bytes and no hidden test observer.
- [Current publication/transport](../../../../../logs/r2mlvs3mp-w-utility-current-transport-checks.log): ten actual native cases, exact repeated loading, five corruption refusals, valid preview retry and saved carry/reservation continuation.
- [Retained old-format loading](../../../../../logs/r2mlvs3mp-w-utility-legacy-format-checks.log): actual 381 grant, 382 released/issued-ticket retry and 383 rebuilt physical transfer are accepted through the current reader without changing the inputs.
- [Directed physical flow proof](../../../../../logs/r2mlvs3mp-v-utility-transfer-file-checks.log): original-source conservation, imported use, loss without refund, new rebuilt ID and exact native reloads. Prior publication, settlement and older transport proof remains linked from Plan 12.

The focused C++ command uses `[utility-power-effects],[utility-power-recipe],[utility-power-mutation],[utility-power-ledger],[utility-power-epoch],[utility-power-stage]`; `[utility-power-company]` selects actual company lifecycle separately. Native scripts are `utility_power_save_fixture.py` and its bounded `-1` through `-7` files; `-4 --retired` runs ordinary-game retirement loading. Every supplied paid-world input and historical utility fixture remains unchanged; build/test/application logs live in `logs/`.

Plans 6/16/17 may now consume this tested producer. Solar/mirror runtime, geothermal heat and construction, paid external source procurement, full fleet/job integration and their final sprites retain their owning pending tasks. This handoff does not complete those features or the whole Plan 12.
