# R2MLSP-R - Asteroid catalogue execution notes

Owner: [Space master plan](../../../zz-reach2/upgrades/2026-10/r2mlsp-space-master-plan.md), [Plan10](../../../zz-reach2/upgrades/2026-10/r2mlvs10-asteroids-and-mining-probes.md) Group1 task1.4. Date:2026-10-14.

## Current batch

Plan9's independent runtime/handoff groups are complete; its art task16.2 remains open. Plan10 resumes at1.4. The public class reserve records existed, but size percentages and reserve multipliers lived in the generator. This batch publishes immutable size records beside the class records in `src/multilayer_asteroid_catalogue.cpp`, including Q16 diameter and the existing single mining lease capacity. `MultiLayerGameplayProfile` now exposes the Industry class/size spans; Classic exposes empty spans.

Generation consumes those profile records. Size selection uses one independent percentile draw; reserve sampling uses one subsequent inclusive Small-range draw before applying the size multiplier. The feature domains, draw order, generation version and saved body fields remain unchanged. No saved world is regenerated. Reserve validation consumes the published multiplier.

Before editing, both current423 configurations passed the existing `[space-preview]` producer:4,011 assertions/five cases each. Logs: `logs/asteroid-catalogue-baseline-release.log` and `logs/asteroid-catalogue-baseline-debug.log`. Seed `0x20261012`, Normal256x256 retains95,050t NORE,99,715t WICE and4,018t RARE.

New focused tests independently check the literal class/size records, all100 size percentiles,18 inclusive reserve endpoint combinations, exact one-draw RNG consumption, untouched simulation/interactive RNG, class full-hold ranges and means, the two-return reserve floor and740 work ticks for20t at2t/74ticks. Native `Randomizer` seeds8/0 produce exact first-draw zero/UINT32_MAX, so endpoint checks do not depend on a statistical search. The pre-edit generated per-cargo ledger is frozen in the new fixture.

## Verification and continuation

The consolidated RelWithDebInfo build targets `openttd` and `openttd_test` with two outer workers and `CL_MPCount=2`. Its log is `logs/asteroid-catalogue-release-build.log`. Debug uses the same limits after this build. Run the bounded acceptance wrapper separately for each completed configuration:

```powershell
python -B src/tests/multilayer/acceptance/asteroid_catalogue_checks-1.py --configuration RelWithDebInfo
python -B src/tests/multilayer/acceptance/asteroid_catalogue_checks-1.py --configuration Debug
```

The wrapper runs only `[asteroid-catalogue]`, `[space-preview]` and `[multilayer-content-contract]`, with110-second native timeout and exact native/log identities. Both new binaries have passed and task1.4/Group1 are complete. No paid flight, full saved-phase replay, graphics or network acceptance is supplied by this catalogue fixture. Public motion tuning belongs to Group3 rather than holding the independent resource-content task open.

Both consolidated product/test builds completed, with one build per configuration and no repeated source changes during compilation. RelWithDebInfo passed4,274 assertions/ten focused cases in2.72 seconds. Accepted report: `mlm/reviews/space/asteroid-catalogue-verify/20261014-182037-a24b7c67c1/reports/asteroid-catalogue.json`. Debug passed the same4,274 assertions/ten cases in3.14 seconds: `mlm/reviews/space/asteroid-catalogue-verify/20261014-183633-0e207e5be9/reports/asteroid-catalogue.json`. `logs/asteroid-catalogue-verification.json` freezes both native executable/report/log chains, with all identities checked. Both builds and native processes are finished. `git diff --check` passes (`logs/asteroid-catalogue-diff-check.log`).

After Group1 closes, take Group2.2 next. Its central starter preference exists, but legal launch-site access is not yet delivered by that generator. Important source facts to carry forward: `GenerateWorldAsteroids` runs in `genworld.cpp` before `StartupCompanies`; therefore a proposed accessibility scan must account for the absence of a player company at generation. `GetLaunchCenterFootprint`, `HasLaunchCenterWaterAccess` and the actual `CmdBuildLaunchCenter` predicate are in the Plan9 owner. Inspect the actual terrain/placement checks before choosing a producer change; do not invent a private approximate placement rule. Existing population tests cover all147 supported dimension/density cells and the maximum14,336 bodies.

The actual command in `station_cmd.cpp` first rejects a missing `_current_company` or unavailable technology. Its footprint loop separately requires clear/tree tiles, flat equal-height land, no bridge above, no ground vehicle and successful ordinary clearing quotes. Thus directly calling the command during generation will fail before checking geometry. Publish any necessary company-independent geometric producer from the launch owner and reuse it in the real command; retain company, date, town, spread, joining and clearing/payment policy at that command boundary. This is a design direction, not an implemented or verified new export.

## Reading notes

After compaction, reread Plan10's Global and the nearest `Asteroid generation and motion` Area block. Relocate AD-11 to its heading (currently near339; Global's276 hint is stale). The historical `space.h` mission-type row is superseded by the entry gate: native probe/mission authority is `space_vehicle.h`. Relevant Group14 budget values are in the revised table around258; earlier construction quantities in that document are compatibility evidence. Do not rerun the old `space_mining_checks.py` prepare path: it is a historical405 fixture wrapper, not a current423 acceptance command.

Likewise, `launch_center_checks.py`'s standalone `main` still asserts414. Its `native` helper is already reused by the current retirement wrapper and invokes the actual `.launch-center` producer; reuse that helper from a current acceptance wrapper rather than running the stale full entry point. No new placement acceptance was run during Group1.
