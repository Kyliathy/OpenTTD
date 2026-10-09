# Remaining source wait reachability

Owner: [R2MLVS5 Group 13AC](r2mlvs5-spoil-wagons-and-haulage.md#group-13ac---resolve-remaining-source-waits-and-close-their-files), dispatched by [master 7BB](r2mlvs3mp-master-plan.md#group-7bb---complete-logistics-and-wait-state-reloads). This audit does not complete parent source/depot acceptance.

The five basic files and recoverable lost-origin, receiver-intake and broken-line
files are complete in Groups 13A/13AB. Their accepted files, exact reconstruction,
35-owner next-exchange comparisons and immutable hashes are described in
[f](r2mlvs5-f-source-file-continuation.md). The source lifecycle/load/recovery
producer burst passes 56,034 assertions / 21 cases
([ba](r2mlvs5-ba-lifecycle-owner-checks.log)); staged fields in those tests are
schema/refusal evidence, not physical wait reachability.

| Remaining wait | Verified public behavior | Current actual-file evidence |
| --- | --- | --- |
| LostProject | `RetireTBMProject` requires finished/idle work and withdrawal, then calls `ForgetTBMExchangeProject` before erasing the project. Company removal/deletion synchronously clears affected identities. `AcceptLoadedTBMExchange` validates reconstructed project references and refuses missing ones. An orphan assignment is not a supported removal result. | Native `[tbm-recovery]` case `A finished project retires only when nothing depends on it` refuses retirement while unfinished, assigned, spoil-carrying or unwithdrawn; successful retirement leaves no set/assignment. `[tbm-exchange-load]` section `a missing project` refuses the stale reference. The focused owner/portal burst passes 73,786 assertions / 24 cases ([ca](r2mlvs5-ca-crash-boundary-guards.log)). These are command/schema guard tests, not a manufactured physical LostProject file. |
| WrongWagons | Ordinary depot movement can detach an assigned empty unit. The set remains exclusive and records the same unit; the hauler owner observes the mismatch. Loader permits nonworking sets at their actual locations. | `source-special-deep-wrong-wagons.sav`; ordinary recoupling and full physical producer pass 57,413 assertions ([ay](r2mlvs5-ay-wagon-hold-writer.log)). Both actual loads reach exchange 1 after 2,164 frames with exact vehicle reconstruction and 35 next-state chunks ([az-1](r2mlvs5-az-wrong-wagons-1.log), [az-2](r2mlvs5-az-wrong-wagons-2.log)). |
| NoEmpties | A genuinely smaller initial hauler can reach the source with fewer empties than the cutter's accepted next pass. Ordinary cancellation retains paid cuts and loaded spoil; the existing exchange owner permits that final exchange without requiring another full pass. | `source-special-deep-no-empties.sav`; producer passes 4,597 assertions and completes actual final disposal ([bc](r2mlvs5-bc-short-empties-writer.log)). Both actual loads cancel through the owning command and reach the same final exchange after two frames with 35 exact chunks ([bd-1](r2mlvs5-bd-no-empties-1.log), [bd-2](r2mlvs5-bd-no-empties-2.log)). The cancelled area is deliberately partial; no unpaid completion is claimed. |
| OccupiedBay | The actual FIFO turn is the earliest admitted unloaded assignment. A first short-empty request retains its turn; a second physically present sufficient hauler must report the occupied turn. | `source-queue-deep-occupied-bay.sav`; the physical producer passes 26,682 assertions ([bk](r2mlvs5-bk-queue-cycle-checks.log)). Both actual loads cancel through the owner and serve the same first request after two frames, with exact vehicle reconstruction and 35 identical next-state chunks ([bl-1](r2mlvs5-bl-occupied-bay-1.log), [bl-2](r2mlvs5-bl-occupied-bay-2.log)). The peer retains its actual cycle count; it may have served an earlier physical arrival before this occupied turn. |
| Crashed | Two real same-company trains collide after the player starts the opposing locomotive and explicitly forces it past the ordinary reservation. `Vehicle::Crash` marks the whole physical train; packets remain aboard until native delayed deletion. A wreck has no repair/start or exchange path. | `source-wreck-deep-crashed.sav`; producer passes 121,916 assertions, recording actual destruction of 24 Earth / 480 Rock tonnes and balancing surviving cutter packets plus receipts against generated spoil ([by](r2mlvs5-by-native-crash-writer.log)). Both actual loads preserve initial custody and reach the same native deletion boundary after 4,641 frames, with identical destruction receipts and 35 exact next-state chunks ([bz-1](r2mlvs5-bz-crashed-1.log), [bz-2](r2mlvs5-bz-crashed-2.log)). A genuine cleanup assertion exposed a portal-magazine Depot sentinel; `DeleteLastWagon` now updates the housing's signal segment through its door/pair semantics. |

Source 13.2A is complete; parent 13.2 remains unchecked for depot files. Every ordinary source scenario uses
native spoil mass and freight multiplier 1; the junction case additionally
initializes real pathfinder defaults. The old zero-search-budget broken-line
prototype is retained and excluded from accepted proof.

Continuation: current Group 13AC has compacted once. Its Local Mandatory
Reading is the Source wait reachability and lifecycle guards block immediately
above it. Read this audit's current remaining rows, exact public guards and the
specific actual-file observer; do not preload underground depot geometry.

Final source reconstruction review passes all 12 retained actual families
([cc](r2mlvs5-cc-source-owner-reconstruction.log)). Initial packets, physical
vehicles, orders, projects, geology, receivers and invoice fields reconstruct
exactly. The reduced fixture's unset manager face receives the native default
style once, with its normal RNG draws; only those presentation fields and the
corresponding initial RNG pair differ. The real one/two-load next-state
comparison includes complete company/time/RNG owners without exclusions.
Groups 13AC and parent 13.2A can therefore close; next work is the separately
scoped paid underground-depot consumer in Group 13B.

**Artifact-layout migration (2026-10-09):** this plan consumes the parent [Development artifact layout](r2mlvs-multilayer-vehicles-and-structures.md#development-artifact-layout), implemented by [R2FC](r2fc-file-cleanup.md). It supersedes earlier docs asset/review/helper placement and fixed runtime-config destinations below. New native homes use `mlm/runtime/`, generated feature reviews use `mlm/reviews/`, logs remain under `logs/`, and reusable code/curated inputs keep their code/test owners. Paths and focused native/cold verification are complete; R2FC records the results and retained-input identities. Repository-relative executable commands run from the checkout root through their context-aware wrappers. Historical pass records remain dated evidence and are not new acceptance claims. No gameplay task is completed by this migration and the separate post-demo fixture-pruning gate remains.
