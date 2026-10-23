> **Historical donor snapshot — not an execution plan.** OpenTTD3 `75ea75718f`, imported2026-10-23.
> Original: `docs/zz-reach2/upgrades/2026-10/r2tbmf-v-hangar-input-notes.md`; exact source SHA256 `b3de0efaaf6a08cf3173758050e833a482498c15df32a981f4f8bc9409c87017`.
> Current integration: [M2O23-1](../../../../../../../zz-reach2/upgrades/2026-10/m2o23-1-come-space.md); storage and evidence roles: [archi-historical-debug.md](../../../../../../../zz-reach2/architecture/archi-historical-debug.md). Original completion claims, instructions and human-review limits below remain dated; they grant no new execution, spending, publication or deletion authority.

# R2TBMF V - Native hangar input and order admission

Owner: [fixing Group8](r2tbmf-tbm-fixes.md#group-8---add-coherent-vehicle-orders-and-a-selected-hangar-destination). Read [T](r2tbmf-t-hangar-notes.md) for the saved397 native Depot/Order contract and first physical return/service/file proofs. Architecture: [vehicles](../../../../../../../../../OpenTTD3/docs/zz-reach2/architecture/vehicles/archi-vehicles.md), [GUI](../../architecture/gui/archi-gui.md), [rail pathfinding](../../../../../../../zz-reach2/architecture/pathfinder/archi-pathfinder-rail.md).

Group8 remains current, compaction count1. This checkpoint adds actual project/vehicle mouse input, script admission, shared/copy refusal, native destination removal and the reverse B-to-A hangar journey. No whole Group8 checkbox is complete yet. No build or test is running; Git writes remain prohibited.

## Delivered behavior and evidence

- The project palette's **Choose hangar** opens the actual native destination list. Normal hit testing and window timers select portal B; the same loaded cutter unloads, parks there and resumes its retained partial cut. [Project input] (historical reference: `../../../../logs/r2tbmf-u-project-1.log`) passes20,448 assertions and resumes after5,096 native frames.
- The vehicle's depot button opens the same destination list, including the already parked machine. Native input selects original portal A from a real B-parked397 file; every physical part travels back, parks and resumes the same accepted job. [Vehicle input] (historical reference: `../../../../logs/r2tbmf-u-vehicle-3.log`) passes9,852 assertions and resumes after2,447 frames. Cutter5, job1 and the20-tick partial cut persist; A has genuine depot2, B genuine depot3.
- Native captures were inspected: [project controls](../../../../../../../../../OpenTTD3/docs/zz-reach2/upgrades/2026-10/assets/reviews/tbm-hangar-project-controls.png), [project choices](../../../../../../../../../OpenTTD3/docs/zz-reach2/upgrades/2026-10/assets/reviews/tbm-hangar-project-choices.png), [vehicle controls](../../../../../../../../../OpenTTD3/docs/zz-reach2/upgrades/2026-10/assets/reviews/tbm-hangar-vehicle-controls.png), [vehicle choices](../../../../../../../../../OpenTTD3/docs/zz-reach2/upgrades/2026-10/assets/reviews/tbm-hangar-vehicle-choices.png). Both controls and full menu choices are reachable. The stopped vehicle displays its parked work status. Generic depot travel captions now yield to a real selected-hangar wait; portal next-action advice recognizes `ParkedHangar` as resumable.
- [Script/order admission] (historical reference: `../../../../logs/r2tbmf-v-hangar-orders.log`) passes17,154 assertions/one case in approximately0.55 seconds. A real GameInstance/ScriptCompanyMode uses `ScriptVehicle::SendTBMToHangar` in native Test/Exec modes, including ordinary suspension/callback behavior. Estimates preserve cash, RNG, depot/fleet cardinality, geology and controller digests; wrong-machine/invalid-tile script requests refuse. Native order insertion accepts the actual cutter; insertion and copy/share into its ordinary hauler refuse in both estimate and execute modes without copying work or changing the hauler's list.
- That same case demolishes the actual unoccupied remote portal through `LandscapeClear`. Native Depot destruction invalidates current/list destinations and stops the cutter; the missing-hangar hold preserves its complete accepted job and fleet. Selecting the valid original magazine recovers to whole parked storage. This is real destination destruction, not removal of a scratch ID.

The first reverse trip exposed a real physical-tail defect: `GetLayerPortalMagazineExit` derived the departure side from the head after it had turned farther along the route. Five parts reached A while one remained stored at B. The corrected owner reads the actual stored parts' existing native direction until each leaves, then the actual last part on the housing. It adds no persistent exit field or synthetic movement. The failed trace/file remains diagnostic evidence; it is not a valid playable checkpoint. Only the failing vehicle journey was rerun after this fix.

## Current files and executable provenance

`src/tests/tbm_hangar_input.h` supplies native cursor/press/release/timer observation and real rendering. It never invokes a window callback directly or writes a train position/order. The existing native loader observer consumes this helper only when the explicit input mode is set. [Runner U](../../../../../../../../src/tests/fixtures/cleanup/source-epochs/7791909fc6bc3433-r2tbmf-u-hangar-files.py) now supports `--case project` and `--case vehicle`, reusing its retained397 sources and restoring the startup script/configuration. New successful files are under `src/tests/fixtures/haulage/tbm-hangar-reload-project-397-1-*` and `tbm-hangar-reload-vehicle-397-3-*`; all earlier395/396/397 inputs remain unchanged.

Builds: [input/status batch] (historical reference: `../../../../logs/r2tbmf-v-hangar-input-build.log`), [physical tail correction] (historical reference: `../../../../logs/r2tbmf-v-hangar-tail-build.log`). The diagnostic build first needed `TrackBits::base()` rather than enum conversion; that test-only compile correction is retained in its log. No broad regression was run.

| Current artifact | SHA256 |
| --- | --- |
| `build/RelWithDebInfo/openttd.exe` | `766F2B7F7B446E117A18F5DA03464B54FF5F3E2628580E955445AA655BE3FCEB` |
| `build/RelWithDebInfo/openttd_test.exe` | `CEDBFA87A401E7CB0CC8BEE2F26826BE0ADFE67A08EF3E3DC64B7AD7913C18E3` |
| `build/native-art.cfg` | `396092066483E9EDDF9713618CEB44EB391D124677C0BE5A4BF14D417A809862` |
| `build/lang/english.lng` | `3E4AC9E8D2FB3CB0143F85C16289FC727599EB5814C34C03DFC1A34B9246BD41` |

## Remaining narrow acceptance

1. Audit ordinary station/waypoint and recurring native depot-order arbitration. The selected hangar command, script command and explicit service replacement work; a native non-depot list currently still competes with geological steering. Preserve a single native order owner and explicit work resume. Do not introduce a private queue or duplicate assignments.
2. Complete full/foreign/incompatible/blocked admission, permitted sharing between distinct cutters, takeover/bankruptcy and machinery sale/clone/autoreplace coverage for the newly registered storage. Existing purchase/immutable-machinery guards remain, but old tests alone cannot prove every new destination path.
3. Cover malformed397 and legacy preview/full-load policy. New valid files and actual return/parked reloads already pass; do not regenerate those proven inputs merely to collect more counts.
4. Prove final release and new-job reassignment at B, and ordinary public traffic beside the registered parked machine. Current proofs resume A's same accepted job; new-site reassignment and the later paid far-exit construction sequence remain incomplete.

After these, update the precise parent/owning-plan public contracts and close only tasks with direct evidence. Exit construction, geological variation/art and integrated player acceptance remain later fixing groups. Whole cargo units, batch verification and the paused broader master remain unchanged.
