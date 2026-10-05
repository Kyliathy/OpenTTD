# R2MLVS3MP-K - Ground persistence and lifecycle audit (master 6.5)

This audit covers the `-G` ground portions of [Plan 14 Groups 7-15](r2mlvs14-savegame-determinism-and-lifecycle.md#group-7---feature-persistence-integration), as split in [r2mlvs3mp-e](r2mlvs3mp-e-scoped-gates.md#plan-14-groups-7-15). It maps each row to the code and tests that cover it and lists what is still missing. The `-T` and `-O` portions stay pending in Plan 14, so no Plan 14 task can be checked from this audit alone.

Status words:
- **verified**: compiled and passed at the master Group 4 milestone burst.
- **unbuilt**: written in master Groups 5-6 and not yet compiled. Burst at the end of master Group 7.
- **gap**: not covered. The owner is named.

## Ground families

| Family | Owner | Chunks | Staged in | Accepted in | Digest kind |
| --- | --- | --- | --- | --- | --- |
| Geology (terrain, deposits, vents) | Plan 2 | `geology_sl.cpp` | `FixPointers` (guarded) | `AcceptLoadedGeologyState`, before vehicles | `GeologyRoot` |
| Excavation jobs, passes, claims | Plan 2 | `excavation_sl.cpp` | `Load`, into the geology candidate | with geology; claims are rebuilt from the jobs | inside `GeologyRoot` |
| Portal work | Plan 3 | `layer_portal_work_sl.cpp` | `FixPointers` (guarded) | `AcceptLoadedLayerPortalWork`, before vehicles | `PortalWorkRoot` |
| TBM projects and support | Plan 4 | `tbm_sl.cpp` (version 377) | `FixPointers` (guarded) | `AcceptLoadedTBMProjects`, after vehicles and stations | `TBMProject` |
| Exchange assignments and wagon sets | Plan 5 | `tbm_exchange_sl.cpp` (version 378) | `FixPointers` (guarded) | `AcceptLoadedTBMExchange`, after projects | `Exchange` |
| Spoil receivers | Plan 5 | `spoil_receiver_sl.cpp` | `Load` | `AcceptLoadedSpoilReceivers`, after industries | `IndustryInventory` |

Physical cargo is not a TBM family. Spoil packets live in ordinary vehicle and station cargo lists; receiver stock is the industry's own `accepted[].waiting`. No TBM store saves a second count of the same tonnes.

## Row map

| Row | Ground portion | Code | Tests | Status |
| --- | --- | --- | --- | --- |
| 7.1 | Descriptors, after-load fixups and a mid-action fixture per family | The six handlers above. Each family has a feature flag, an old-version check (`multilayer_sl.cpp`) and a digest record. | Geology: `[geology-state]`, `[excavation-job-state]`. Portal: `[portal-work-state]`. Projects: `[tbm-project]`, `[tbm-phase]`, `[tbm-work]` "Saves on both sides of a cut". Exchange: `[tbm-exchange-load]`. Receivers: `[spoil-receiver-load]`. | Geology, portal and projects verified; exchange and receivers unbuilt. |
| 8.1 | State matrix, not idle samples | The matrix below. | As listed there. | Unbuilt rows marked. |
| 9.1, 9.3 | Loader order and failure path, done once | Metadata and profile are accepted first (`AcceptLoadedMultiLayerStateSchema`). Geology and portal are then accepted before vehicles; projects, exchange and receivers after the vehicles, stations and industries they name. Any refusal calls `SlErrorCorrupt`, the loader's own failure path. Nothing is published before its validator passes. | `[multilayer-state-foundation]` "Load staging and failed activation retain initialized Classic authority"; Python fixtures `portal_save_fixture.py`, `portal_acceptance_save_fixture.py`, `excavation_save_fixture.py`, `geology_save_fixture.py` refuse corrupt files through the real loader. | Verified for metadata, geology and portal. For 377/378 the real-file refusal is a gap (see G1). |
| 9.2 | Bounds before allocation | Headers bound each count before rows are read: projects `MAX_TBM_PROJECTS`, support `MAX_TBM_SUPPORT_TILES`, sets `MAX_TBM_WAGON_SETS`, set wagons 1..512, wagon rows sets×512, receivers 64,000. Enums, tiles (map size × layers), ordinals and revisions are checked on each row. Header and row counts must agree (`Incomplete ... family`). | Field refusals in each family's load test; the preview path in `ValidateTBMExchangeLoadPreview`. | Projects verified; exchange and receivers unbuilt. |
| 10.1 | Typed references after every object loads | Projects: `ValidateTBMProjects` against live trains, jobs, the portal and the origin. Load normalization turns a lost reference into a recoverable block; it never drops the record. Exchange: `IsLoadedTBMWagonSetStoreValid` and `IsLoadedTBMAssignmentStoreValid`. Portal: `ValidateLayerPortalWorkRecords(check_references)`. Receivers: one row per receiver industry of its own kind. ID reuse: every deletion path now clears the ID at once (see 15.1), so a saved ID always names its original object. | `[tbm-exchange-load]` "refuse missing, reused or shared references" (14 sections). `[tbm-project]` "two projects for one cutter". `[spoil-receiver-load]` missing or wrong industry. `[tbm-recovery]` deleted cutter and deleted origin. | Unbuilt (projects verified). |
| 10.2 | Null action | Every ground `FixPointers` returns unless `SlIsFixingPointers()` (true only for `Ptrs`), so the loader's `SlNullPointers` cleanup stages nothing. Staging is cleared by each family's `Reset...State` at `InitializeGame`, and staged rows are never live before acceptance. | `TBMExchangeSave::Refuse` (store and cursors stay empty after a refusal); `[multilayer-state-foundation]` failed activation. | Code reviewed. A loader-driven failing load for 377/378 is part of G1. |
| 10.3 | One owner per packet and tonne | Spoil stays in vehicle and station packets; sets and assignments name wagons, never packets. Receiver stock is the industry counter. The geological material ledger (committed totals per job) records what was cut; it does not own the cargo. | New fixture audit `TBMPacketCustody`: no packet is in two lists, and no packet made since the audit began is allocated outside every list. It runs in `[tbm-exchange-acceptance]` at every release hold and through both full cycles, including the reload. `GetTBMPacketOwners` checks each packet stays with its wagon. | Unbuilt. |
| 10.4 | Consist links, job references, set membership | A wagon may be in one set only; an Assigned set must ride on its cutter; a set has at most one carrier; a hauler has one assignment and is never a cutter. Job claims are rebuilt from the jobs, and a competing job refuses (`ClaimConflict`). | `[tbm-exchange-load]` sections; `[excavation-job-state]`. | Unbuilt for sets. Chain links: see G2. |
| 10.5 | Two heads claiming one wagon; two jobs claiming one face | Two sets with one wagon, two assignments of one hauler, one set carried by two haulers (new section), two projects for one cutter, and a duplicated job (`ClaimConflict`). | As in 10.4, plus the new "one set carried by two haulers" section. | Unbuilt. Raw chain links: G2. |
| 11.1 | Ground caches rebuilt from authority | Job claims come from the saved jobs (`PrepareGeologyRestoration`). Portal connectivity is logged from accepted records. Rail path reservations are ordinary tile bits. No TBM or exchange cache exists: lookups scan the stores. | `[excavation-job-state]` restored claims equal live claims. | Verified. Corridor claims: G3. |
| 11.2, 11.3 | Rebuild changes no cargo; same next step after reload | Rebuilt claims hold no cargo. Next-step comparisons use staged copies: projects at TravelToFace/Bore entry, both sides of a cut, every hauler phase and wait, both sides of the exchange step, a cancelled project mid-return (new), and the second cycle after a reload. | `[tbm-phase]`, `[tbm-work]`, `[tbm-exchange-load]`, `[tbm-recovery]`, `[tbm-exchange-acceptance]`. | Mixed: projects verified, the rest unbuilt. Receivers: G4. |
| 12.1 | Estimates change nothing | Portal construction, excavation reservation, exchange proposal and commit, acquisition and controls are all validated before execution and compared against digests. | `[portal-build]` atomic refusal; `[excavation-job-reservation]`; `[tbm-exchange-proposal]` "The estimate tries the whole rearrangement and restores everything"; `[tbm-exchange-commit]` "An estimate and a refusal after the first leg change nothing"; `[tbm-acquisition]` rollback; `[tbm-recovery]` Cancel and Retire estimates. | Mixed. |
| 12.2 | Saves during cuts, straddling consists, queue waits | Cuts and queue waits are covered by staged copies. | `[tbm-work]`, `[tbm-exchange-load]`, `[tbm-wagon-sets]`. | Straddling consists: G5. |
| 13.1 | Bend or partial pass, full-wagon pause, spanning hauler | Partial pass: both sides of a cut. Full-wagon pause: the WaitForEmpties hold with every hauler wait. | `[tbm-work]`, `[tbm-exchange-load]`. | A 45-degree bend save and a spanning-layer hauler: G5. |
| 13.2 | Failure and cancellation reload; retreat and origin cycle kept | Breakdown is a saved pause. A cancelled project reloads mid-return and ends Cancelled; its last load still goes to the origin. Support rows are saved with the project. Phase, blocked reason, assignments and sets are in the digests. | `[tbm-recovery]` "Breakdown holds the face as a saved pause", "A cancelled project reloads mid-return and ends the same way" (new), "gives its spoil away through the exchange first". | Unbuilt. |
| 14.3 | Hashes and next-step ledgers per ground family | Each family has its own digest. The after-load `MLVS_STATE` diagnostic used to run before projects, exchange and receivers were accepted, so it never covered them. It now runs after the last acceptance (`afterload.cpp`). | `[multilayer-state-digest]`; each family's load test compares its digest. | Unbuilt. |
| 15.1 | Ground lifecycle table | Below. | Below. | Unbuilt. |
| 15.4 | Owner hooks, tested | Below. | Below. | Unbuilt. |

## State matrix (8.1)

| State | Families saved | Reload test |
| --- | --- | --- |
| Open job, partial work | Geology, jobs | `[excavation-job-state]` |
| Sealed, Commissioning or Connected pair | Portal work | `[portal-work-state]`, Python portal fixtures |
| Project at TravelToFace or Bore entry | Projects, support | `[tbm-phase]` |
| Mid-cut and just after a cut | Projects, jobs | `[tbm-work]` |
| Breakdown pause | Projects | `[tbm-recovery]` |
| Every hauler phase × wait | Exchange | `[tbm-exchange-load]` |
| Just before and just after the exchange step | Exchange, packets | `[tbm-exchange-load]` |
| Loaded set released in each hold | Exchange, packets | `[tbm-exchange-acceptance]` |
| Second cycle after a reload | All TBM families, receivers | `[tbm-exchange-acceptance]` |
| Cancelled project waiting for its exchange | Projects, exchange | `[tbm-recovery]` (new) |
| Cutter deleted, pair still bound | Projects, portal, exchange | `[tbm-recovery]` (new) |
| Receiver mid-interval, surplus mode | Receivers | `[spoil-receiver-load]` |

## Ground lifecycle table (15.1, 15.4)

"Refuse" means the command fails before any change. Every row leaves stores that load.

| Event | Jobs | TBM project | Wagon sets and haulers | Bays | Portal pair | Cargo owner after | Hook | Test |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Sale, move or refit of a cutter part | - | Refuse | - | - | - | Unchanged | `CheckTBMTrainRearrangement`, `CheckTBMRailVehicleSale`, `CheckTBMRefit`, `IsTBMReplacementLocked` | `[tbm-exchange-lifecycle]` |
| Sale of a loaded set unit | - | - | Refuse | - | - | Unchanged | `CheckTBMRailVehicleSale` | `[tbm-exchange-lifecycle]` |
| Sale of a hauler locomotive | - | - | Assignment ends; loaded wagons keep spoil and set | - | - | The wagons | Deletion hook | `[tbm-exchange-lifecycle]` |
| Train deleted (crash, flood, any path) | Unchanged | Cutter cleared at once, `MissingCutter`; Cancel then ends it at once and Retire erases it | Parts leave sets; empty sets erased; their carriers released | - | Cutter cleared, binding kept | Deleted with the train (ordinary vehicle rule) | `OnTBMTrainPartDeleted` → `OnTBMExchangeTrainPartDeleted`, `ForgetLayerPortalWorkCutter` | `[tbm-exchange-lifecycle]`, `[tbm-recovery]` (new) |
| Origin station deleted | - | Origin cleared at once, `MissingOrigin`; a new choice is needed | Haulers wait `LostOrigin` | - | Origin cleared, binding kept | Unchanged | `OnTBMStationDeleted` → `ForgetLayerPortalWorkOrigin` (new) | `[tbm-recovery]` (new) |
| Origin loses its rail facility or owner | - | `MissingOrigin` block | `LostOrigin` | - | - | Unchanged | Normalization | `[tbm-recovery]` |
| Depot bay removed | - | Falls back to its Source bay; refused if that cannot resolve | Queued assignments follow the project's bay | Released | - | Unchanged | `CheckTBMExchangeBayRemoval`, `ReleaseTBMExchangeBay` | `[tbm-exchange-lifecycle]` |
| Bay approach damaged | - | `MissingExchange` block; a new bay may be set under queued haulers | Follow the new bay | - | - | Unchanged | Normalization, `CmdSetTBMExchangeBay` | `[tbm-recovery]` |
| Cancel | Cancelled: unfinished cells free, cavities kept, no money | ReturnForExchange, exchange, Cancelled; Cancelled at once without a cutter | The exchange takes the spoil; short empties suffice | - | Bound until Retire | Hauler, then receivers | `ControlTBMProject` | `[tbm-recovery]` |
| Retire | Kept (closed) | Erased once idle and withdrawn | Empty sets dropped | - | Withdrawn to Sealed | Unchanged | `RetireTBMProject`, `WithdrawLayerPortalWorkPair` | `[tbm-recovery]`, `[portal-recovery]` |
| Merger | Company moves | Company moves | Follow the project | - | Moves with the project | Unchanged | `ChangeTBMOwnership`, `ChangeExcavationJobOwnership`, `ChangeTileOwner_LayerPortal` | `[tbm-exchange-lifecycle]`, `[portal-recovery]` |
| Company removal | The project's job is cancelled; other jobs are orphaned and paused (`InvalidOwner`) | Erased | Emptied by the deletion hook, then purged | Removed with the company's tiles | Released, then removed with the tiles | Deleted with the vehicles | `ChangeTBMOwnership`, `ReleaseLayerPortalWorkBinding` | `[tbm-exchange-lifecycle]`, `[portal-recovery]` |
| Receiver industry closes | - | - | A hauler at the origin waits `SaturatedReceiver` | - | - | Unchanged aboard | `UnregisterSpoilReceiver` in `Industry::~Industry` | `[spoil-receiver-load]` (new section) |
| Deposit claimant leaves | - | - | - | - | - | - | None | G6 |

## Gaps

| ID | Gap | Owner and next step |
| --- | --- | --- |
| G1 | No real-file round trip or failing load for the 377/378 families: the project, support, exchange and receiver chunks are proved only through staged copies. A saved file with a live project needs real commissioning. | Python save fixtures after master 6.6, then the occupied saves of master 7.4 (Plan 3 Group 16). |
| G2 | The ordinary vehicle loader rebuilds `previous` and `first` from `next` without refusing two heads that link the same part. The TBM stores refuse every duplicate among themselves, but a corrupt raw chain is the vehicle loader's authority, and no TBM-local repair is added. | Plan 14 10.4 `-G`, kept open; a vehicle-loader check would also change Classic behaviour, so it needs its own decision. |
| G3 | Project corridor and retreat claims are not delivered, so there is no corridor cache to rebuild. | Plan 4 Group 14 (master 4.6) with master 7.3. |
| G4 | Receiver reload compares the published rows and digest, not the next interval. The interval reads only saved fields and the industry stock, which are both compared. | A next-interval check with G1. |
| G5 | No save of a 45-degree bend, a straddling consist or a spanning-layer hauler, because neither train can drive itself yet. | Master Group 7 (7.3, 7.4). |
| G6 | Deposit claims have no live caller, so no merger or removal rule exists for a deposit claimant. | Plan 6 mines, when they claim deposits. |

## Fixes made by this audit

- Cancel of a project whose cutter was deleted ends it Cancelled at once, even when its job is already closed; before, a project that lost its cutter on its final return could never finish or retire.
- The deletion hook clears a project's cutter and its pair's cutter at once. A Commissioning record may now have no cutter.
- A new station deletion hook clears a deleted origin from projects and pairs at once. Before, a pair naming a deleted origin failed its load check, and a finished project could silently take a reused station ID as its origin.
- The after-load `MLVS_STATE` diagnostic now includes the project, exchange and receiver families.
- Tests: `TBMPacketCustody` packet audit, "one set carried by two haulers", a closed receiver, and the deleted-cutter, deleted-origin and cancelled-reload cases.
