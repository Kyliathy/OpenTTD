> **Historical donor snapshot — not an execution plan.** OpenTTD3 `75ea75718f`, imported2026-10-23.
> Original: `docs/zz-reach2/upgrades/2026-10/r2mlvs17-d-handover.md`; exact source SHA256 `43f25b0b5a80a224e4bf931a9f491f7aa68f0606b4d9aaa1b8fbc8115b3a345c`.
> Current integration: [M2O23-1](../../../../../../../zz-reach2/upgrades/2026-10/m2o23-1-come-space.md); storage and evidence roles: [archi-historical-debug.md](../../../../../../../zz-reach2/architecture/archi-historical-debug.md). Original completion claims, instructions and human-review limits below remain dated; they grant no new execution, spending, publication or deletion authority.

# Plan17 handover after geothermal source commands

Owner: [Plan17](r2mlvs17-underground-transport-tubes.md), dispatched by [master10.7](r2mlvs3mp-master-plan.md). Architecture: [map layers](../../architecture/map-layers/archi-map-layers.md). Supporting producer contracts: [geothermal source notes](r2mlvs17-b-geothermal-source.md).

## User stop, 2026-10-17

The user requested: "Stop after the current task. Need to switch model." Execution stopped at completed Group6BDB. Resume implementation only when requested. Group6BDC was read and considered, but no control, readiness, widget or string changes were made. Its two tasks remain unchecked. The context compacted during stop documentation; no meaningful6BDC implementation had begun, so its compaction count was not incremented.

All changes remain uncommitted. No build, test or game process remains active. No Git write, checkout, subagent or full regression was performed. The previous temporary `r2mlvs17-c-handover.md` was read and removed as it instructed; that was the sole individual deletion. Retain all supplied saves, original art and human acceptance gates.

## Completed checkpoint

- Group6BCD and original6BC.1: actual geothermal capsule motion and depot repairs consume the original shared allocation through real local feeds/cable capacity. Depot service now admits native geothermal work authorization. Both configurations passed33 isolated cases/17,230 assertions each at the [work checkpoint] (historical reference: `../../../../logs/tubes/geothermal-work/20261017-work-01/verification.json`).
- Original6BC.2 and6B.2: explicit switches in both provider directions preserve original energy history, invoices, cargo and demand fractions. Different inlets may share one original generator. Complete old-grant retirement is checked before acquiring replacement permissions. Both configurations passed27 cases/15,989 assertions each at the [switch checkpoint] (historical reference: `../../../../logs/tubes/geothermal-switch/20261017-switch-01/verification.json`).
- Group6BDB.1/.2: append-only synchronized geothermal connect/close and capsule bind/reconnect commands delegate to those owners. Both game/test targets build in Debug and RelWithDebInfo; each configuration passes13 isolated cases/7,878 assertions in the [current command receipt] (historical reference: `../../../../logs/tubes/geothermal-commands/20261017-command-01/verification.json`). This receipt records exact sources, controller, binaries and packed art. The two earlier receipts describe their dated source checkpoints. No live host/client acceptance is claimed.

Native save format434 and packed `media/baseset/openttd.grf` are unchanged. The GRF SHA256 is `6d3d2dc0e9e8ede6c07931a6fc446573af48f90fca5d65122e252ea827efc92a`. Stop-only Markdown changes do not require another native test run.

## Resume scope and useful findings

Read the master Global/current10.7/Selected asset, plant and lifecycle Local, then Plan17 Global/Paid tube power Area and the nearest **Native geothermal source choices and guidance** Local above6BDC. Read only relevant named sections/symbols. Older notes claiming commands, bindings or native work are absent are historical. Continue6BDC, then6BDD ordinary switched-file acceptance; original6BD.1/.2,6B.3,6.4 and master10.7 stay unchecked until their full scope passes.

`UndergroundTubeControls` in `transport_tube_gui.h/.cpp` still implements external supply only. Its source dropdown uses external producer IDs; its bound-source display assumes `GetTubePowerConnection`. Native geothermal inlet IDs are a distinct type and must not alias those producer IDs. Player selection must identify the actual original plant and tube inlet, consume existing purchased-cable admission, and show selected/bound provider and actual waits. Queries must remain pure and retain manual stops.

`GetTubeNetworkReadiness` in `transport_tube_readiness.cpp` currently describes external inlets and lease/usage affordability. Native supply needs truthful provider-specific guidance. Existing native controls tests in `src/tests/transport_tube_controls.cpp` exercise real window callbacks, generated language, command dispatch and painting. Tube widgets live in `src/widgets/underground_widget.h`; layout and compact status/preview live in `src/underground_gui.cpp`. No GUI/readiness design has been committed to by implementation.

Current synchronized command signatures in `transport_tube_cmd.h/.cpp` are:

```cpp
CommandCost CmdConnectTubeGeothermalPower(DoCommandFlags flags, IndustryID generator, TileIndex inlet);
CommandCost CmdCloseTubeGeothermalPower(DoCommandFlags flags, TubeGeothermalSourceID source);
CommandCost CmdSetTubeVehicleGeothermalPower(DoCommandFlags flags, VehicleID vehicle, TubeGeothermalSourceID source, bool reconnect);
```

The connect command has no expected-output argument. Revalidate stale plant/source selections when implementing controls. Do not infer that any proposed follow-up design was already delivered.

Shared actual work/switch fixtures are `geothermal_tube_work_fixture.h` and `geothermal_tube_switch_fixture.h`. Focused command tests are `transport_tube_geothermal_commands.cpp`; the reusable controller is `src/tests/multilayer/utilities/geothermal_tube_source_checks.py`. These fixtures disclose prepared physical inputs; they do not prove normal-world bootstrap.

Build one MSBuild project at a time with eight compiler workers (`-j 1 -- /p:CL_MPCount=8 /nodeReuse:false`). Keep native file tests separate from compilation, poll running work at least50 seconds apart, and keep individual focused invocations below two minutes. Preserve function/argument/class/interface documentation, plan/architecture file headers and business explanations for compound conditions and nontrivial loops. New assisting Markdown stays beside this plan with the next chronological letter; new source/test filenames must not begin with `r2`.
