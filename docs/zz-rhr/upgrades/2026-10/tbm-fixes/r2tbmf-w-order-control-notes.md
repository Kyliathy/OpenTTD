# R2TBMF W - Native order-list control

Owner: [fixing Group8](../../../../zz-reach2/upgrades/2026-10/r2tbmf-tbm-fixes.md#group-8---add-coherent-vehicle-orders-and-a-selected-hangar-destination). Prior physical hangar/save contract is in [T](r2tbmf-t-hangar-notes.md); native menus, script admission, removed destinations and the corrected complete B-to-A departure are verified in [V](r2tbmf-v-hangar-input-notes.md). Architecture: [vehicles](../../../../zz-reach2/architecture/vehicles/archi-vehicles.md), [rail pathfinding](../../../../zz-reach2/architecture/pathfinder/archi-pathfinder-rail.md), [savegames](../../../../zz-reach2/architecture/savegame/archi-savegame.md).

Group8 closed after2 compactions. Native-list travel, safe waits, all return intents, selected hangars, final release, public traffic, remote reassignment, native company lifecycle and actual file acceptance are verified below and in T/V. Git writes remain prohibited. Group9 is current; the broader master remains paused.

**Artifact-layout migration (2026-10-09):** this plan consumes the parent [Development artifact layout](../../../../zz-reach2/upgrades/2026-10/r2mlvs-multilayer-vehicles-and-structures.md#development-artifact-layout), implemented by [R2FC](../../../../zz-reach2/upgrades/2026-10/r2fc-file-cleanup.md). It supersedes earlier docs asset/review/helper placement and fixed runtime-config destinations below. New native homes use `mlm/runtime/`, generated feature reviews use `mlm/reviews/`, logs remain under `logs/`, and reusable code/curated inputs keep their code/test owners. Paths and focused native/cold verification are complete; R2FC records the results and retained-input identities. Repository-relative executable commands run from the checkout root through their context-aware wrappers. Historical pass records remain dated evidence and are not new acceptance claims. No gameplay task is completed by this migration and the separate post-demo fixture-pruning gate remains.

## Explicit arbitration implemented in source

Adding the first native travel order, copying/sharing a native list into a working cutter, or selecting **Follow order** suspends excavation. The existing project phase becomes `FollowOrders`; the original job, cursor, partial work, claims and cargo remain with their owners. The same ordinary `OrderList`, current order and native station/depot processing supply all destinations. No private queue, destination copy or work-assignment cloning is added. A temporary explicit depot order retains priority over list editing; deliberate order selection may hand control back to the list.

The ordinary order window reuses its Skip command/control as **Follow order** for an assigned TBM and admits the same single retained order. Commands remain synchronized and shared peers keep separate work identities. Native list processing runs before that phase's missing-order hold so it can select its next leg. A native Halt leg is adopted before physical movement, preserving the existing spoil-exchange and whole-hangar-arrival contract. Exhausted/removed travel orders show `MissingOrders` and hold.

**Resume work** explicitly leaves native station handling if necessary, preserves the list, and requests a native service visit to the existing working bay. Whole physical arrival and the existing exchange/departure then rejoin the retained face with the cutter leading. A direct face rejoin exposed a backwards arrival stall; the bay visit fixes that actual failure without another saved destination or private route. The work controller keeps the native list dormant until another explicit handoff. Service visits preserve their earlier behavior; normal trains and managed haulers keep their native processing.

Save version398 (`TBMNativeOrderControl`) distinguishes the added phase/hold values. It extends the existing phase/reason fields only. Version397 still bounds its earlier values and retains its actual portal Depot/Order meaning. All earlier native files remain immutable.

## Native order acceptance

`[tbm-native-orders]` in `src/tests/tbm_hangar.cpp` drives a real partial cut, a native Surface station visit with explicit NoLoad/NoUnload, a safe missing-list wait, work resume with the list retained, and selection of that same single order again. Optional fresh recorder outputs include actual travelling/station/wait files.

The existing native loader observer and `tbm_hangar_input.h` have input/resume modes for the real **Follow order** and project **Resume** controls. [Runner U](../../../../../src/tests/multilayer/tbm/hangar_files.py) adds `--case orders` using the retained397 B-parked file and `--case orders-reload` for a genuinely moving new file; `--source` selects an explicit retained input. New output names use `-run-N` instead of hard-coding a save version. The actual file header owns the version, preventing later executables from mislabelling output as397. Existing397-named evidence is unchanged.

The initial build needed the native `VehicleOrders` window enum correction; the first station journey then exposed the backwards rejoin failure above. The corrected game/test build [completed successfully](../../../../../logs/r2tbmf-w-order-rejoin-build.log). No build or test remains running at this checkpoint.

- [Native orders](../../../../../logs/r2tbmf-w-native-orders-focused-2.log):35,118 assertions/one case; actual station travel, missing-list wait, retained partial work, bay rejoin and next cut, then selecting the same single order again.
- [Native button input](../../../../../logs/r2tbmf-u-orders-2.log):11,225 assertions; real Follow order and Resume controls, then the next cut after2,792 native frames.
- [Moving-file reload](../../../../../logs/r2tbmf-u-orders-reload-1.log):11,209 assertions; retained398 travel file reaches the same station and resumed cut after2,791 frames. Cutter5, job1 and20 ticks of partial work persist. Earlier397 sources, configuration and original390 demo hashes are retained by runner U.

Order-rejoin checkpoint SHA256: game `DAA26FD7676263E832811FF3B0DD8AB66806B47B104FD2EF44F9119A9286FB63`; test `A3DA39C4354347CF17CAB5BB742038680863380F94ACCD9AF75E3EC80938F03C`. Final Group8 provenance appears below.

The user reported exhausted memory and explicitly requested larger batches and fewer builds. Carry that policy into Group9 and later groups: complete related implementation/test changes together, run multiple acceptance scenarios against the resulting build, and repeat only failed or newly affected checks. Use `-j 1 -- /p:CL_MPCount=2 /nodeReuse:false`, confirmed against the installed MSBuild `ProcessorNumber` property, and keep heavyweight tests after compilation.

## Combined acceptance

### Combined follow-up batch X

The source batch now selects explicit return direction using the actual two-ended native pathfinder, and starts automatic face backing only on its phase transition. Later controller ticks no longer force an interrupted ordinary trip backwards again. Native station handling ends before a selected hangar order replaces it. Portal capacity now distinguishes actual magazine occupants from ordinary public through traffic, and destination admission checks the cutter's existing model/layer/date capability on both portal ends.

Four new acceptance cases in `src/tests/tbm_hangar.cpp` combine all four loaded station-interruption return intents; B release/public traffic/new-job reuse and machinery guards; actual native takeover/bankruptcy; and blocked-route recovery/full/foreign/incompatible admission plus two real cutters sharing ordinary orders. They run with the four directly affected existing order/hangar cases in [runner X](../../../../../src/tests/multilayer/tbm/hangar_acceptance.py). The same runner prepares immutable malformed397/398 phase/reason and real depot-reference files for the existing native preview/full-load observer.

The combined game/test [build succeeded](../../../../../logs/r2tbmf-x-hangar-batch-build.log), using `-j 1 -- /p:CL_MPCount=2 /nodeReuse:false`. Observed free memory was about11GiB during test compilation. [Combined behavior](../../../../../logs/r2tbmf-x-behavior-2.log) passes seven of eight cases, including every station return intent, native takeover/removal, shared cutters and blocked/full/incompatible admission. The B case also passes machinery guards, final release and actual ordinary through traffic beside the retained stored cutter. Its new-work quote exposed a genuine reused-pocket gap: an excavated centre with unopened surrounding pocket cells was incorrectly treated as a fully prepared open start. Initial test-only geometry adjustments did not resolve that gap; their failed logs remain diagnostic evidence rather than completion claims.

All seven actual malformed-file probes refuse safely:397/398 phase values and398 reasons fail both preview and full load without changing the running preview world; a397 depot pointed at the other registered housing fails full load with **Missing portal hangar depot**. The runner originally expected **Invalid portal hangar depot** for that last case; the actual native refusal and no-startup proof passed. Its expected diagnostic was corrected without repeating the seven processes. Logs are `logs/r2tbmf-x-tbm-hangar-bad-*-2-*-2.log`; original397/398 inputs and startup/configuration hashes remain unchanged.

## Group8 closure

`ResolveExcavationStarter` now uses the existing portal issuer when a reused portal's complete initial3x3 pocket still needs excavation. Its first legal cut quotes, reserves, charges and opens every remaining cell through the original geological owners. Ordinary open floor gains no solid support permission. Full-load portal validation applies the same complete-pocket requirement in both directions of the job/pair binding; no field, cargo unit or private route is added.

The [pocket correction build](../../../../../logs/r2tbmf-x-reused-pocket-build.log) passes. [The previously failing B journey plus four affected starter cases](../../../../../logs/r2tbmf-x-reused-pocket-focused.log) pass37,519 assertions/five cases. The actual cutter releases job1 at B, retains native identity5/depot3/permanent machinery, permits ordinary freight through both B housings, explicitly seals B for new work and commissions job2 with purchase0. Native motion completes the charged first cut, matching both quoted cargo quantities and opening the complete starter pocket. A copied missing-issuer record refuses before publication.

The resulting actual `mlm/reviews/tbm/hangar-orders/import-20261009/saves/tbm-hangar-reuse-398-5-remote-reassigned.sav` passes [real full load and nonmutating preview](../../../../../logs/r2tbmf-u-reassigned-1.log), then the same native machine performs the first new cut after967 frames. The file preserves its registered depot, job2, actual hauler assignment and full machinery/fleet.3,909 assertions pass; input/configuration/startup hashes are retained. Runner U's `--case reassigned --source <actual file>` reuses the ordinary native loader and command owner.

Together with T/V and the native-order proofs above, all five Group8 tasks are complete. The command/script/machinery paths, adverse destinations, ordinary orders, company lifecycle, legacy397/new398 files, loaded returns, parking, resume, remote release and reassignment have direct evidence. ExitB construction from an active tunnel, geology/art and final integrated player acceptance remain Groups9-11; this closure does not claim those workflows complete. No build or test remains running.

| Final Group8 artifact | SHA256 |
| --- | --- |
| `build/RelWithDebInfo/openttd.exe` | `150F8A3DF16F1D268455882ED379C2B5EDFE086B8B8865344647A7C5F859FE7F` |
| `build/RelWithDebInfo/openttd_test.exe` | `40B25CB8B96A9996368CCD7CAE1138114431BFA5DA60686175FE6C087A8B8421` |
| `build/lang/english.lng` | `ABBA59D2D13122A128E71AD367C0103B7410135F9DCE836CBD2598D089C040CC` |
| `build/native-art.cfg` | `396092066483E9EDDF9713618CEB44EB391D124677C0BE5A4BF14D417A809862` |
