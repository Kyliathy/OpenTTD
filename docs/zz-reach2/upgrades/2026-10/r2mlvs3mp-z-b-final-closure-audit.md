# Portal/TBM goal finalization audit

Owner: [master execution plan](r2mlvs3mp-master-plan.md). Targets: [Plan3](r2mlvs3-sealed-portals-and-deep-rail.md), [Plan4](r2mlvs4-tunnel-boring-machines.md). Architecture: [map layers](../../architecture/map-layers/archi-map-layers.md). Authorization and previous evidence: [finalization ledger](r2mlvs3mp-z-a-finalization-ledger.md). This audit preserves engineering requirements and distinguishes implemented checks from explicitly authorized goal exceptions.

Continuation note,2026-10-08: the user requested further implementation after this bounded checkpoint. Master10.1/Plan16 have resumed. Follow the live master cursor and producer checkboxes for newer delivery; this audit retains the prior proof and exceptions without declaring the remaining queue finished.

## GP-NJ: real multiplayer acceptance requires a new multi-group harness

Current code has ordinary OpenTTD networking: `NetworkExecuteLocalCommandQueue`, server `SendSync` and client `ReceiveServerMapDone` -> `SafeLoad`. This is not missing transport implementation. The absent piece is the required complete feature acceptance infrastructure. `rg` finds crypto tests, no ground host/client/join runner, and no network caller of `LogMultiLayerStateDiagnostics`. That diagnostic combines feature roots, whereas ordinary vehicles, packet ownership, clocks and agreed network-frame command ordering require additional evidence. The existing48-owner reader compares local saved files; it does not observe snapshot transfer or live peer agreement.

Meeting master11.3 requires at least three separate implementation/planning groups:

1. A repository-only two-process server/client harness: loopback configuration/ports, real connection/snapshot readiness, synchronized company commands, hidden/visible viewport policies, bounded subprocess deadlines and cleanup that preserves user configs/content.
2. Complete agreed-frame trace capture/comparison: actual network frame and command sequence, canonical base vehicle/packet/clock state plus feature roots, joining baseline and field-level first-divergence diagnostics. RNG-only sync and offline replay cannot provide this contract.
3. Active excavation/wagon-wait join scenarios with physical lifecycle/power-link failures, different cameras/renderers, pause/speed/economy-time modes and repeated snapshot reconstruction. Each needs a real pre-event state and matching subsequent decisions on both peers; operator intent cannot become a result.

Under the user's explicit permission to skip major gaps spanning more than two task groups, **the real host/client/join portions of master11.3 and its dependent join-performance rows are skipped for goal finalization**. Plan14 Groups16-19 remain unchecked where they require those results. Tube/thermal cases retain GP-TG; orbital joins are outside this master's scope. No multiplayer pass is claimed and no production network protocol is changed by this exception.

Independent ground lifecycle work is delivered: [11,736 assertions/18 cases](../../../../logs/r2mlvs3mp-z-b-ground-lifecycle-checks.log) exercise actual origin/bay loss, mine/processor recovery, train/set ownership, company takeover/removal and ID reuse. Current serialized loss/cancellation/retreat/Surface-disposal proof remains in Plan14 Groups13/13B/15B. It cannot complete network assertions. Engineering Plan3/4 and parent milestones still require their original network conditions.

## Audit scope

This goal finishes the developable target queue with expressly documented exceptions. It does not satisfy the master's original full engineering Completion Conditions. Checkboxes retain that distinction; Plans3/4 and parent M1 stay incomplete.

## GP-BT: genuine normal-newgame bootstrap acceptance

The current runner correctly classifies S1/S2/S8/S9 as isolated physical fixtures and rejects `bootstrap_claim`. `UndergroundWorkflowWorld` creates its test company/profile/geology, dries Surface terrain, prepares initial logistics and supplies explicit pre-existing upper access for Deep. Economics pays actual synchronized setup commands, but those facts do not prove a first-build chain from a normal fresh game. Existing real loader/command/GUI/script evidence is retained and never relabelled.

Complete bootstrap acceptance needs at least three additional groups: (1) normal game/profile/package/company generation and legal site selection with no fixture registry/terrain setup; (2) paid readiness of the first actual stations/receivers/depot/hauler and a produced Deep access predecessor rather than a pre-opened assembly floor; (3) an end-to-end normal command/control/script scenario with exact predecessor/cash/material provenance, construction readiness and saved evidence independent of the development fixture. This is a new acceptance/producer reconciliation boundary, not an operator confirmation.

Under the user's multi-group-gap permission, **normal-newgame bootstrap portions of master11.4 and Plan15's whole S1/S2 first-acceptance claim are skipped for this goal**. GP-TG separately covers missing tube/plant final-use conversion. The original scenario/task remains unchecked. Independent code-backed geometry/controls/recovery is verified: [6,021 turn/clearance assertions/nine cases](../../../../logs/r2mlvs3mp-z-b-ground-turn-checks.log), [136,634 actual movement/control/script/recovery assertions/16 cases](../../../../logs/r2mlvs3mp-z-b-ground-control-recovery-checks.log), and current serialized heading/cursor/support/cargo recovery from Plan14. No paid inventory, free cavity or different end state is substituted to obtain a bootstrap pass.

## GP-PF: representative maximum-workload performance acceptance

The delivered toolchain has frame accumulators and focused physical scenarios, but no discovered complete benchmark suite for the specified compact/representative/maximum feasible maps, many simultaneous jobs and actual join reconstruction. Finite caps and a quick unit test cannot establish those latencies or peak memory.

Full master11.6 needs three new groups: (1) seeded repeatable map/entity/job/route workloads and a genuinely feature-disabled baseline with declared hardware/configuration; (2) per-owner peak/RSS/temporary allocation and agreed-tick command/path/save/load/peer-join measurements, warmup/repeats/uncertainty and predeclared incremental budgets; (3) representative stress traces and field/owner attribution for exceeded budgets, followed by targeted fixes and regressions. Real join additionally depends on GP-NJ. Under the user's explicit multi-group-gap permission, **these complete performance rows are skipped for goal finalization**, retaining pending Plan15 Groups21-23 and master11.6.

Independent delivered work is concrete: `utility_power_performance.cpp` measures actual native bounded sparse ledgers at0/32/256/4,096 accounts/edges, one warmup/five intervals, exact quantities/lifetime/retirement and payload estimate. Both Debug/optimized configurations pass78,980 assertions. Optimized max-count allocation median/max3.822/3.996ms and retirement0.437ms fit a27ms native opportunity; Debug37.024/38.115ms exceeds it and is reported as such. This is an account-owner benchmark, not a physical generator/bootstrap, feature-disabled whole-game baseline, dense-map, RSS/peak-memory or network result. No unsupported performance claim or ad hoc simulation shortcut replaces the skipped requirement.

## Target requirement and evidence matrix

All original master checkpoints are accounted for below. Prior closure evidence remains in the owning groups; historical logs removed by the user are not recreated. Current retained evidence is linked where available.

| Master scope | Delivered evidence / exception | Disposition |
| --- | --- | --- |
| 1.1-1.4, 2.1-2.3 | Content/profile/legacy baseline, canonical sealed starter and pure portal/geology/actual acquisition handoffs; current portal/date/profile checks in the final boundary logs below | Complete |
| 3.1-3.7, 4.1-4.7 | Real saved project, job-owned route/work, atomic paid cuts/support/spoil, reservations and supported backing; current turn/control/recovery logs linked under GP-BT | Complete |
| 5.1-5.8, 6.1-6.6 | Actual wagon exchange/Surface receivers/hauler custody and funded commissioning; [Plan5 complete source/depot evidence](r2mlvs5-spoil-wagons-and-haulage.md#group-13b---drive-recurring-underground-bay-exchanges) | Complete; no fresh bootstrap claim |
| 7.1-7.9, 7B/7C/7D scopes | Actual driven source and recurring-depot loops, visibility pairs, occupied portal files, Deep dates/scripts and paid economics; owning master/Plan3/Plan4/Plan5 closures | Complete; fixture initialization declared |
| 8.1-8.8 | Forty selected families,2,311 sprites,2,377 actual loaded sprites and157 native frames; [technical gallery](r2mlvs3mp-t-gallery-acceptance.md), [fresh hash/corruption check](../../../../logs/r2mlvs3mp-z-b-final-art-verification.log); decoded/mixed idle/boring/backing/service base-sound reuse in master8.7 | Technical M1 slice complete; personal feedback deferred |
| 9.1-9.5A | Single electrical owner, exact original-source grants/use/release, linked physical delivery and actual company/town/processor effects; [Useful Power handoff](r2mlvs3mp-x-useful-power-handoff.md) | Complete delivered scope |
| 9.5B, 10.1-10.8 | Native capsule/power-procurement/thermal-construction/final-use/art expansion requires more than two groups; [GP-TG ledger](r2mlvs3mp-z-a-finalization-ledger.md) and owning Plan3/4/7/13/16/17 group comments | Authorized skip; unchecked |
| 11.1 | Schema2 runner, real subprocess evidence/refusals and genuine legacy/current classification; [six runner refusal checks](../../../../logs/r2mlvs3mp-z-a-runner-refusal-checks-2.log), [current S8 record](../../../../logs/multilayer-acceptance-s8-boundary-2.json) | Complete ground scope |
| 11.2 | [Plan14 ground persistence matrix](r2mlvs14-savegame-determinism-and-lifecycle.md#ground-persistence-evidence-matrix):48 real owners,385 partial bend/capacity/spanning/lost-origin/cancelled final cycle; genuine381..384 utility migration and full-load Null refusals | Complete delivered ground/electrical scope; tube/heat GP-TG |
| 11.3 | Actual ground lifecycle above; live host/client trace and active join need GP-NJ | Authorized network skip; unchecked |
| 11.4 | Actual control/script/turn/backing/failure recovery above and Plan4 21.1-21.3; normal-newgame predecessor chain needs GP-BT, tube/plant final use GP-TG | Authorized bootstrap/final-use skip; unchecked |
| 11.5 | [Schema2 paid corridor invoice](../../../../logs/multilayer-acceptance-s1-final.json) and [67,045 no-profit/fee/saturation assertions](../../../../logs/r2mlvs3mp-z-b-spoil-no-profit-saturation-checks.log): Standard10,620ticks/2,546,553 expense, Deep32,926/3,154,099,zero spoil income | Complete specific corridor objective; mineral payback remains separate |
| 11.6 | Actual account-bound benchmark above; full representative/max-map/RSS/save/join workload requires GP-PF/GP-NJ | Authorized performance skip; unchecked |
| 11.7 | Plan15 27/28 G subsets verify technical M1 coverage/hashes. Later interop art needs GP-TG; human feedback is expressly skipped/deferred under operator-gate permission | Partial technical completion; original unchecked |
| 11.8 | [Current player guide](r2mlvs-how-to.md), this evidence matrix and concrete recovery limitations | Complete target documentation |
| 12.1-12.2 | Delivered target exports/docs are reconciled, but complete target closure depends on GP-TG and outstanding acceptance above | Authorized dependent closure skip; original unchecked |
| 12.3-12.5 | Scoped producer documentation, exact remaining-task/coverage audit, final focused builds/checks and truthful parent status | Complete; no M1/full-epic completion |

### Cross-plan invariant mapping

| Invariant | Exact target proof / limit |
| --- | --- |
| I-1 layer identity | Stable Surface0/Underground1/Space2/Deep3; current `[portal-profile]` and existing display-order/legacy native reader evidence |
| I-2 excavation, I-4 atomicity | Canonical paid cut/reservation/support owners, focused turn/clearance/refusal/control/recovery logs above; quotes/camera never open rock |
| I-3 conservation, I-6 transport | Actual exchanged wagon IDs/packets and driven Surface disposal/empty return; Plan5 custody traces and current48-owner/native-cycle comparisons; utility original-source lifetime settlement |
| I-5 determinism | Hidden/visible/headless physical loops and repeated owner/clock/RNG comparisons; real network agreement remains GP-NJ |
| I-7 persistence | Current385 bend/wait/spanning/cancel/origin files; genuine370 portal and381..384 utility readers; actual preview/fixup/Null refusals; old380 acceleration mismatch retained as failure |
| I-8 capability, I-14 Deep | Current final portal/profile/date/script/content tests; actual commissioning/fleet date cases retained in Plan3 Group17;1January1980 calendar authority preserved |
| I-9 bounded work | Real sparse account-cap benchmark and bounded planner operations; whole-game/max-map/peak-RSS/join acceptance remains GP-PF/GP-NJ |
| I-10 graphics | Exact selected source/packed/loaded coverage plus157 actual native frames/sounds; later interop art GP-TG, personal feedback deferred |
| I-11 solar | Electrical producer avoids duplicate source/use accounting; physical solar/mirror feature outside this target queue, no solar-playability claim |
| I-12 bootstrap | First source-side exchange code is delivered; genuine normal-newgame predecessor acceptance GP-BT, first capsule/plant/riser GP-TG |
| I-13 geometry | Real advancing45-degree/two-bend90 routes, swept clearances, supported backing and saved partial heading; actual tube/plant final-use conversion GP-TG |
| I-15 tube separation, I-17 thermal | Declared public boundaries do not prove physical capsule/heat ownership; actual consumers are GP-TG and remain unchecked |
| I-16 launch collection | Orbital collection/descent is outside this master's selected scope; no pass inferred from Surface station proximity |

### Remaining target tasks and documentation limits

Plan3 retains **12.1** (real tube/riser integration) and **18.2** (full completion/index reconciliation). Plan4 retains **15.1**, **21.4** (complete final non-rail use) and **22.2** (full completion/index reconciliation). Group22.1 records delivered exports/evidence/gaps and can close independently. Both target engineering plans remain incomplete. Later Plan13/14/15 full-profile, orbital, network and maximum-workload tasks remain with their owning plans; no scoped G completion closes a mixed-family task.

The guide discloses GUI compatible-default purchase, no arbitrary model/count picker, no replacement-origin picker, deferred normal-newgame proof, genuine Deep upper-access prerequisites, unsupported capsule/thermal/orbital tools and personal art feedback. Actual owner-level origin replacement/recovery is tested; it is not advertised as an existing GUI control. Canonical prices are unchanged because the measured target corridor/no-spoil-profit objectives pass.

### Final focused reproduction

Build both `openttd_test` and `openttd` with the repository CMake build in Debug/RelWithDebInfo; retained logs are [Debug](../../../../logs/r2mlvs3mp-z-b-final-debug-build.log) and [optimized](../../../../logs/r2mlvs3mp-z-b-final-optimized-build.log). From `build/`, run each configuration's test executable with:

```text
[portal-date],[portal-script],[portal-profile],[portal-contract],[portal-lifecycle],[multilayer-content-contract],[utility-power-company],[utility-power-performance],[tbm-controls],[tbm-recovery],[tbm-exchange-lifecycle],[layer-industry-lifecycle]
```

[Debug boundary result](../../../../logs/r2mlvs3mp-z-b-final-debug-boundaries.log) and [optimized boundary result](../../../../logs/r2mlvs3mp-z-b-final-optimized-boundaries.log) each pass162,516 assertions/38 cases. This final burst deliberately selects target compatibility/ownership/recovery boundaries and the changed performance fixture; broader completed physical/native families retain their specific current results above. No whole-epic regression, real join or maximum-map pass is claimed. Original supplied saves, retained artwork, user configs and Git history remain intact; no new commit is authorized.

[Fresh native compatibility](../../../../logs/r2mlvs3mp-z-b-final-native-compatibility.log) loads the actual connected Multilayer fixture and genuine370 Classic input through the final Debug executable:2/2 native rail edges and5/5 legacy rail plus4/4 elevator edges remain valid. Source/config/startup hashes are preserved. [Post-build technical art verification](../../../../logs/r2mlvs3mp-z-b-final-art-verification-2.log) again passes the exact selected hashes/coverage and three corruptions. All child application processes completed; no new raster or source save was substituted.

[Final documentation audit](../../../../logs/r2mlvs3mp-z-b-final-documentation-audit.log) verifies at most eight tasks per affected group, existing current evidence links, no pending producer tasks in Plans2/5 and exact pending Plan3/4 IDs. `git diff --check` passes. The parent index marks only those completed supporting contracts X; Plans3/4 and M1 retain incompletion. The developable queue is finalized with the user's recorded exceptions; original engineering completion remains a future objective.
