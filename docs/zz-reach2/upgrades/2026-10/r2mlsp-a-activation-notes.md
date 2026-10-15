# R2MLSP-A - Module Activation Execution Notes

Owner: [Space master](r2mlsp-space-master-plan.md), [Plan 11](r2mlvs11-orbital-stations-and-manufacturing.md) Group 4. Date: 2026-10-13. These are working notes, not acceptance evidence.

**Final checkpoint,2026-10-13:** paused at the user's request after Group4's
RelWithDebInfo verification. Tasks4.0/4.2 and the master Group4 are checked.
Both native targets built;285 focused assertions and all four native wrappers
passed. No build or test remains running. Debug is deferred to the next
substantial batch. Evidence is frozen in `logs/orbital-activation-verification.json`.
The group experienced one context compaction. Resume at Group4B using the
[flight handover](r2mlsp-b-flight-handover.md). The sections below are historical
implementation snapshots; their pending-build/unverified statements are
superseded by this checkpoint.

## Current group and reading

Execute Group 4 before Group 4B. Group 4 now owns tasks 4.0 and 4.2 for Core/Storage/Power. Original tasks 4.1 and 4.3 remain unchecked in new Group 4B, which explicitly requires completing the missing Plan 8 Group 11 approach producer. The master includes this extra group once. No new group is verified or checked yet.

After compaction, re-read Plan 11's Global Mandatory Reading, the Delivered construction Area gate (Groups 1-6), and the Module construction after Core commissioning Local block (Groups 2-6). Relevant source reading: parent AD-12; `MultiLayerMap` ownership preamble in `src/map_layer.h`; packet Split/Merge in `src/cargopacket.cpp`; command estimate/execute boundary in `src/command.cpp`; `SpaceConstructionDefinition`; actual overlap/activation/packet ownership in `src/space_structure.cpp`; reservation/work and inventory ticks in `src/orbital_station.cpp`; shared sources/service validation in `src/orbital_service.cpp`. Do not preload later processing or art reading.

Stale navigation: `src/space.h` is shared geometry/preview, not the native fleet/structure owner. Actual owners are `space_vehicle.h`, `space_transition.h`, `space_structure.h`, `orbital_station.h`. All requested producer symbols were located through their actual public headers; same-plan contract cases and the shared utility producer cases still need execution with this batch. Plan 8 Group 11 docking reservations are genuinely unchecked, unlike the existing asteroid attachment lease. Do not substitute a private berth registry.

## Implementation awaiting verification

Save 419 (`OrbitalModuleActivation`) retains the 418 SPST byte layout and admits explicit paid Storage/Power functions. Older files cannot contain activated non-Core modules. The new synchronized `ActivateOrbitalModule` command is appended; existing Core command IDs and its Core-only entry remain intact. Activation prepares the contained native module before registering a panel, rechecks both consumed recipe slots, and requires an operational adjacent owned Core for extensions.

Core/Storage have separate 1,024-unit per-cargo stock and 64 packet-reference bounds. Storage has no panel, service consumer or assembly consumer. Power owns its original shared-ledger panel, publishes 60 kW only after activation and with a live dependency, and has no freight stock or work slots. The Core retains its 30 kW panel and existing service/construction behavior. Both extension activation and operational queries resolve actual Core adjacency. Full dependency/removal acceptance remains Group 4B/5.

New `orbital_module_activation.cpp` has two focused contract cases and one hidden native observer. Native actions prepare/continue use the immutable `src/tests/fixtures/demos/space/space-module-frame-418.sav`; freight/stored use actual typed movement and a genuine Core Steel packet. Malformed preview actions must leave structures, power, company cash and RNG unchanged. No native acceptance manufactures input stock or injects positions.

## Build and focused validation

MSVC: two outer workers and `CL_MPCount=2`. CMake is absent from PATH; use the cached executable:

```powershell
& 'C:/Program Files (x86)/Microsoft Visual Studio/2022/BuildTools/Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin/cmake.exe' --build build --config RelWithDebInfo --target openttd openttd_test --parallel 2 -- /p:CL_MPCount=2
```

The first consolidated release build is running in exec session 4011, writing `logs/orbital-activation-release-build.log`. The shared header/version changes require a broad rebuild, not a broad test run. Test-only changes were batched while the game target was still compiling; the later test target should include them. Check errors and completion before running the new binary. User correction while building: bundle more changes because builds take long. Let this build finish, validate its focused Release boundaries, then bundle the next related Dock/approach, removal, handoff and fixture changes before rebuilding. Defer Debug to that next substantial batch; use the same worker limits and never run concurrent two-worker builds.

Run only active group contracts and directly consumed producer contracts, for example `[orbital-activation-contract],[space-construction-contract],[orbital-core-contract],[orbital-service-contract],[utility-power-ledger]`. Native acceptance is split into `orbital_activation_checks-1.py` (activation/replay/ordinary resave), `-2.py --activation <exact preceding report>` (real Storage freight), `-3.py --freight <exact preceding report>` (schema/panel refusals), and `-4.py --freight <same report>` (stock/work-slot refusals). Pass `--configuration Debug` for Debug. Use exact report paths printed by the preceding invocation; never choose a latest unrelated run. These producers are registered in `mlm/review-producers.json`.

Each native subprocess has a 110-second timeout; if an entire wrapper exceeds two minutes, split that wrapper further. Poll running tests with waits of at least 50 seconds and keep user updates within a minute. Avoid full regression. New wrapper imports and syntax have been checked; actual builds/tests remain pending.

Historical native wrappers pin output version 418. Their retained input snapshots remain valid and unchanged. When a historical wrapper is needed for an active group, reconcile its current-output assertion with 419 while preserving old input-version/corruption gates; do not rewrite old evidence or run the full regression automatically.

After successful validation, freeze source/binary/report hashes in `logs/orbital-activation-verification.json`, update Plan 11's status and task evidence, then check Group 4 in the source and master. Continue at Group 4B. The current implementation has had no context compactions yet; increment the active group's compaction note after an actual compaction.

## First build correction and next-batch characterization

The game target built successfully. Initial test compilation found one fixture-only API-name mistake: the enum is `UtilityMutationKind::ReleaseUnused`, not `Release`. The source is corrected. Let session4011 finish compiling the remaining test files, inspect all errors, then retry only `openttd_test` incrementally with the same limits. Do not start another broad game/Debug build for this correction. All four new Python runners import successfully. The retained418 frame contains Core stock of2 Steel and one unit each of RFMT/ELEC, plus the completed Storage site1 and another inert Storage site2; the actual freight observer has a genuine Steel source.

Read-only characterization for the next batch has covered Plan8 Global, its Orbital world and construction Area (Groups8-17), and Group11. Relevant sources read: parent AD-9, persisted `VehicleType` ordering, `Map::TileBase`/`TileExtended`, simulation versus interactive `Randomizer`, and the plane-distance fixture; the command boundary and map-layer preamble are already read for Group4. Re-read only the applicable sections after compaction when actually executing that producer.

Plan8 Group11 requires request-tick/native-ID ordering, one owner per slot, waiting/admission/release/cancellation, and queued save/target-loss fixtures. A completed Dock declares two additional reservations; the existing Core has one actual paid service berth. Existing body leases do not prove a generic static approach producer. Preserve native incarnation-qualified references and existing paid service/cargo/utility owners. Dock services must consume explicit physically connected shared sources rather than registering a free Dock panel. Review any expanded Dock approach geometry against retained inert blueprints before changing overlap acceptance. These are design constraints, not implemented or verified behavior.

Later facility activation must remain explicit: Group4's supported-kind whitelist currently excludes Assembly, Steelworks/Goodsworks, FuelStore and advanced processors. When their owning manufacturing/processing groups become active, extend the activation/role validation together with the actual bounded facility producer and its tests; add explicit producer tasks if the source plan assumes these functions without delivering them. An inert frame or a catalogue definition cannot satisfy a completed-Assembly/processor gate.
