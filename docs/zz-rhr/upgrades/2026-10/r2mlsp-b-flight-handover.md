# R2MLSP-B - Flight Handover

Owner: [Space master](../../../zz-reach2/upgrades/2026-10/r2mlsp-space-master-plan.md) and [Plan 11](../../../zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md). Checkpoint: 2026-10-13. Execution paused at the user's request before takeoff. No implementation should resume until the user asks.

## Completed checkpoint

Plan11 Group4 tasks4.0/4.2 and its master checklist entry are checked. The
native synchronized command activates fully consumed Core/Storage/Power frames
once, with a saved74-tick countdown, exact consumed-material validation and
generation/owner checks. Extensions depend on an adjacent operational owned
Core. Storage has separate bounded stock; Power has its own60kW shared-ledger
panel and no freight inventory. The Core retains30kW. Save419 preserves the418
SPST layout while refusing old-version activated extensions. The Core-only
command and previous command IDs remain intact.

RelWithDebInfo game and test targets built successfully. The first build
completed the game but found a fixture enum typo (`Release` instead of
`ReleaseUnused`); an incremental test-only retry passed. Focused contracts
passed285 assertions in11 cases. Four native wrappers passed in6.53,5.92,8.44
and9.54 seconds. Interrupted activation, real Core-to-Storage freight and
ordinary game resaves compare26 owner families. Four malformed cases fail
both full load and copied preview; live owners, cash and RNG remain unchanged.
The original paid418 fixture is unchanged.

Frozen source/binary/input/report/log hashes:
[verification record](../../../../logs/orbital-activation-verification.json).
Exact native reports:

- [Activation/replay](../../../../mlm/reviews/space/module-activation-verify/20261013-182811-aec2601e84/reports/orbital-module-activation.json).
- [Real Storage freight](../../../../mlm/reviews/space/module-freight-verify/20261013-182829-765c85513c/reports/orbital-module-freight.json).
- [Old schema and Storage panel refusals](../../../../mlm/reviews/space/module-activation-refusals-1/20261013-182844-027c2ce305/reports/orbital-module-activation-refusals.json).
- [Stock cap and forbidden consumer refusals](../../../../mlm/reviews/space/module-activation-refusals-2/20261013-182859-953cbfd8dc/reports/orbital-module-activation-refusals.json).

Debug has deliberately not been rebuilt or checked for this group. Bundle
those checks with the next substantial changes, as requested by the user.
No full regression was run. No build or test remains running, and no Git
commit/reset/checkout or other Git write operation was performed. Changes
remain in the working tree.

## Resume at Plan11 Group4B

Read Plan11 Global Mandatory Reading, Delivered construction Area reading,
Module construction after Core commissioning Local reading and the nearer
Dock functions and completed-module dependencies Local reading. Read the
relevant Plan8 Global/Area reading before implementing its shared approach
producer. Re-read only the sections needed for the active group after any
compaction. Group4 recorded one compaction; Group4B has not started.

1. Complete the missing [Plan8 Group11](../../../zz-reach2/upgrades/2026-10/r2mlvs8-space-world-and-construction.md#group-11---allocate-dock-and-approach-slots-deterministically) shared approach-reservation producer and focused fixtures before its Dock consumer. Existing asteroid attachment leases do not supply this boundary.
2. Implement Plan11 Group4B's Dock activation/berths and dependency acceptance. Required approach behavior includes request-tick/native-ID ordering, one owner per slot, waiting/admission/release/cancellation, queued-save continuation and removed-target refusal. Core already has one paid service berth; Dock declares two additional slots. Preserve incarnation-qualified native references.
3. Continue Groups5-6 for safe removal and the public construction handoff, then return to Plan9 task11.3 for actual rocket-to-module delivery. Plan10 follows the master queue.

Dock must consume explicitly connected shared utility sources; it does not
receive a free panel. Preserve native stock, service, job and utility owners.
Check any expanded approach geometry against retained inert blueprints.
Later Assembly/processors/FuelStore remain excluded from the supported
activation kinds; their owning groups must deliver actual bounded functionality
before extending activation. An inert frame does not satisfy those gates.

Read-only Plan8 characterization is recorded in the
[activation notes](r2mlsp-a-activation-notes.md). Stale navigation: `space.h`
owns shared geometry/preview; native fleet and structures live in
`space_vehicle.h`, `space_transition.h`, `space_structure.h` and
`orbital_station.h`. Historical native wrappers may pin output version418;
update only active consumers' current-output expectations while preserving
their old-input and malformed-version tests.

## Build and focused checks

Builds are expensive. Bundle related implementation and fixture edits before
building. Keep two outer workers and `CL_MPCount=2`; never run concurrent
builds. CMake is absent from PATH:

```powershell
& 'C:/Program Files (x86)/Microsoft Visual Studio/2022/BuildTools/Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin/cmake.exe' --build build --config RelWithDebInfo --target openttd openttd_test --parallel 2 -- /p:CL_MPCount=2
```

For the next substantial Debug batch, use the same command with
`--config Debug`. Focused activation checks:

```powershell
& './build/RelWithDebInfo/openttd_test.exe' '[orbital-activation-contract],[space-construction-contract],[orbital-core-contract],[orbital-service-contract],[utility-power-ledger]'
python -B src/tests/multilayer/acceptance/orbital_activation_checks-1.py
python -B src/tests/multilayer/acceptance/orbital_activation_checks-2.py --activation <exact-report-from-1>
python -B src/tests/multilayer/acceptance/orbital_activation_checks-3.py --freight <exact-report-from-2>
python -B src/tests/multilayer/acceptance/orbital_activation_checks-4.py --freight <same-exact-report-from-2>
```

The Python wrappers accept `--configuration Debug`. Use the new run's exact
report paths when checking new binaries. Keep each burst under two minutes;
split longer scripts into suffixed files. Poll running tests with waits of at
least50 seconds. No test or build is needed merely to resume reading.

Keep source/test file and function/class documentation standards, comments for
compound business conditions and nontrivial loops, and the user's prohibition
on Git writes. Put auxiliary Markdown beside this plan with the next prefix
`r2mlsp-c-`; do not name source/test/JSON files with an `r2` prefix. Do not spawn
subagents unless explicitly requested.
