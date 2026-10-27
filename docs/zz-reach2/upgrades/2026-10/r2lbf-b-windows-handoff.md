# Linux build fixes: Windows verification handoff

@see [Linux build fixes plan](r2lbf-linux-build-fixes.md), Group 17 (item 17.2). The run record is [r2lbf-a-execution-notes.md](r2lbf-a-execution-notes.md).

**Status:** waiting for the owner. This plan changed source and test files that the primary Windows/MSVC build also compiles. MSVC cannot run in the Linux cloud environment, so **completion needs the owner's Windows confirmation, or an explicit waiver**.

**Tested commit:** `cdaab156` (branch `mlm-mlv`, not pushed). On Linux, GCC 13.3 and Clang 18.1.3 pass all 1,361 CTest tests, and the CI Dedicated/no-PCH variant builds. Counts and logs are in the execution notes, **Final record**.

## What to run on Windows

1. Build **Debug** and **RelWithDebInfo** through the existing native profile (`src/tests/multilayer/support/native_build.py`, profile `windows-x64`, Visual Studio 17 2022, two workers). The plan changed no part of that profile.
2. Save the block below as `windows-check.txt` next to `openttd_test.exe`, then run `openttd_test.exe -f windows-check.txt` in both configurations. The names are escaped for Catch: a `\,` inside a name is literal. It lists the 45 cases this plan edited, added or fixed:
   - 37 that failed in the Linux baseline: 15 stale expectations (one renamed), 21 isolation fixes and 1 portability fix;
   - the two new cases;
   - the three save producers;
   - the network flavour case;
   - the two former leak victims.
3. Run `openttd_tube_test.exe` and `openttd_surface_supply_test.exe`. Both compile the changed fixtures, and the tube executable compiles the changed listener.
4. If time allows, run the whole suite **through CTest** (`ctest -C <config>`), which gives each case its own process. Do not run all of `openttd_test.exe` as one process. On Linux that fails 485 of 1,357 cases through cross-case leaks this plan did not cause (see **Known limits**). The 45-case list above does pass as one process.

Report each configuration's pass/fail summary. If a case that this plan treated as a stale expectation (S) passed on Windows **before** these edits, say so; it is then a portability case (P), and its edit needs another look.

```text
A TBM holds at a junction until its actual work route is restored
A TBM leaves its work hold for a real depot visit and returns to the face
A TBM releases at the remote hangar and commissions its next job there
A blocked hauler pauses new cutting until its route is found
A face wait without a hauler asks the player to assign one
A project's whole life can be played from the portal window's controls
A reload during a bend continues the same cursor and ledger
A started cutter drives to its face\, works its route and backs into its magazine
Common scheduler settles mixed native contention in original destination order
Excavation outline extra work is explicit charged and read-only
Finite mine state writes its actual savegame descriptor family
Five actual capsule bodies preserve load across native service and source waits
Geothermal interrupted collector holds construction and exclusive source through abandonment
Hauler cycle reports insufficient complete face room without a partial transfer or turnaround
Integrated company transitions cannot revive old native provider rights
Integrated source and riser loss preserve original histories and independent work
Merged native network identity refuses original and stale protocol peers
Native cable loss and repair preserve unrelated town and orbital work
Native conventional fleet writes real mappings orders cargo and project association
Native processor checkpoints write real stock and packet descriptor families
Native provider binding captures and hashes ignore transport row order
Native purchase previews resolve actual engines and both availability states
Native riser abandonment retains material custody and cannot regain readiness
Original interval depletion never republishes energy during consumer retries
Paid Surface packets enter the actual powerhouse construction owner without freight income
Project controls pause and resume without touching orders or phase
Recipe planner checks integer limits before complete commit
Recipe quotes bind their exact owner state and complete deltas
Released support stays while a train stands on it
Sealed TBM startup needs the door heading and capacity for the whole pocket
Shared recipe commit preserves exact all-input deltas and refuses a retry
Support rail turns follow the game's 90-degree rule
The first cut of a standard and a Deep corridor is measured
Tube depot power restoration services the actual capsule without starting it
Tube interchange geometry refuses cross-layer station identity
Tube physical topology change revokes unused original reservation without refund
Tube platform art warns on the exact original full native packet queue
Tube purchase and refit preserve the native movement and cargo domain
Tube restored service preserves manual depot stop and queued unpowered custody
Tube routine allocation leaves native physical path topology stable
Tube terminal and interchange art follows real native platform roles and joins
Tube terminals and utility footprints require every actual floor and approach
Unrelated portal loss preserves an imported native capsule reservation
Visible chamber quote plans only rail turns the current game rule allows
WindowDesc - ini_key validity
```

## Files changed and why

All S-category edits cite the production commit and stated intent behind them (the Groups 11-13 entry gate). No production value was reverted to satisfy an old test.

**Production code (MSVC compiles all of it):**

| File | Change | Why |
| --- | --- | --- |
| `src/saveload/saveload.h` | `required_capability{}` default member initializer | R7: hundreds of GCC/Clang `-Wmissing-field-initializers` warnings; value unchanged (`std::nullopt`) |
| `src/layer_recipe_work.cpp`, `src/surface_supply_order.cpp` | `#include <utility>` | R1: libstdc++ does not supply `std::exchange` or `std::as_const` transitively; MSVC did |
| `src/saveload/layer_processor_sl.cpp`, `transport_tube_power_sl.cpp`, `geothermal_heat_route_sl.cpp` | Bind the destination reference before the braced assignment | R3: GCC rejects a braced list assigned to a conditional lvalue |
| `src/saveload/vehicle_sl.cpp` | Explicit instantiation of `SlOrdinaryCargoPreview` for the four vehicle types | R5: Clang needs the complete `static inline` array before use |
| `src/launch_assembly_gui.cpp`, `src/asteroid_gui.cpp` | INI keys `"launch_assembly"` and `"space_survey"` | Upstream `WindowDesc` rule: a sticky box needs an INI key. The windows' sticky state and size now persist in `windows.cfg` |
| `src/tbm_exchange.cpp` | A face wait (`AwaitFaceTransfer`) with no hauler reports `NoHauler` | Owner decision 13C (a): the portal controls now ask the player to assign a hauler instead of showing "Working" forever. This is the plan's only gameplay-visible change |
| `mlm/review-producers.json` | Three producer entries | Owner decision 13B (a): the native save producers write run-owned outputs |

**Tests and fixtures:**

| Reason | Files |
| --- | --- |
| Portability: missing include, member order, temporary lifetime, odr-used constant (R2, R4, R6, R8) | `layer_save_fixture.h`, `tbm_exit.cpp`, `descent_custody.cpp`, `launch_center.cpp`, `launch_production.cpp`, `launch_retirement_native.cpp`, `surface_collection_boundaries.cpp`, `tbm_face_preflight.cpp`, `tbm_face_cycle.cpp`, `surface_supply_order_owner.cpp` |
| Isolation (I): a case owns its sprite cache, language pack, local company or recipe content | `transport_tube_recovery.cpp`, `transport_tube_construction.cpp`, `transport_tube_vehicle_sprite.cpp`, `transport_tube_facility_sprite.cpp`, `utility_scheduler_lifecycle.cpp`, `utility_scheduler_contention.cpp`, `geothermal_collector.cpp`, `geothermal_riser_lifecycle.cpp`, `layer_recipe_batch.cpp` |
| Stale expectation (S1): halved excavation yields, `bfba42fd` | `excavation_outline.cpp`, `tbm_engine.cpp`, `tbm_coverage.cpp`, `tbm_movement.cpp` (Mixed-rock idiom) |
| Stale expectation (S2): FollowOrders, support in use, halved cutting rate, hangar Resume and bay staging (`bfba42fd`); support-piece quote (`0119308a`); r2df straight exit | `tbm_project.cpp`, `tbm_corridor.cpp`, `tbm_commission.cpp`, `spoil_receiver_stall.cpp`, `tbm_movement.cpp`, `tbm_hangar.cpp`, `underground_workflow.cpp` (case renamed), `tbm_support.cpp` (new case) |
| Stale expectation (S3): ground art (`75cf506b`) | `multilayer_control_selection.cpp` |
| Face-wait prompt (13C) | `tbm_movement.cpp` (new case "A face wait without a hauler asks the player to assign one") |
| Fixture producers write run-owned saves (13B) | `layer_processor_load.cpp`, `layer_mine_load.cpp`, `underground_save.cpp` |
| Cross-case leaks and JUnit crash (15.3-15.4, 17.2) | `tbm_fixture.h` (cleans the cargo packet and depot pools), `underground_workflow_fixture.h` (restores `_price` and `_economy`), `tube_test_environment.cpp` (run-start load without Catch assertions), `layer_processor_load.cpp` (the save carrier deletes its own packet) |
| `RANDOM_DEBUG` build flavour (16.3) | `merged_command_compatibility.cpp` |

## Known limits

- **Not run on Windows.** Nothing here proves the MSVC build or the Windows test run; that is this handoff's purpose.
- **Hidden cases not run.** `openttd_ground_art_test` and `openttd_ground_viewport_test` contain only hidden (`[.…]`) cases, so on Linux their default runs selected nothing. Their tagged runs belong to the Windows native pipeline.
- **Packet leaks left alone.** Several other fixtures still clean only the vehicle pool and leave orphaned packets. No current test counts them, so they were not changed; they are listed as a follow-up.
- **The suite needs one process per case.** Running all of `openttd_test` in one process fails 485 of 1,357 cases on Linux (`logs/linux-baseline/20261027-linux/group17-openttd_test-one-process.log`). The failures are cascading cross-case leaks, led by empty-pool `REQUIRE`s in `surface_supply_space_custody.cpp:114`, `surface_supply_refund.cpp:90`, `surface_supply_cross_owner.cpp:83` and `tbm_fixture.h:74`. They are not caused by this plan: the leaking fixtures are ones it did not change, and its own fixture edits only add cleanup. No single-process run of the whole suite was recorded before this plan. The leaks are listed as a follow-up. CTest, which the plan verified, runs each case in its own process.
