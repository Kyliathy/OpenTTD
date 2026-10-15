# R2MLSP-Q - Launch and Recovery Runtime Handoff

Owner: [space master](r2mlsp-space-master-plan.md),
[Plan9](r2mlvs9-launch-centers-and-rockets.md) Group21,
[vehicle architecture](../../architecture/vehicles/archi-vehicles.md) and
[layer architecture](../../architecture/map-layers/archi-map-layers.md).
Updated:2026-10-14. This records delivered runtime behavior through schema423;
it does not close unfinished graphics, network or later producer work.

## Public owners and command boundaries

| Owner/export | Delivered behavior | Source |
| --- | --- | --- |
| LaunchCenterAnchor, LaunchCenterRef, GetLaunchCenterPlaneReference | Original Surface build anchor and exact station incarnation define the fixed Q16 orbital reference. Joined freight geometry never moves it. | src/launch_center.h, src/launch_center.cpp, src/station_cmd.cpp |
| CmdBuildLaunchCenter, CmdRemoveLaunchCenter | Native owned Surface facility, orthogonal navigable-water placement/protection and normal joining. Removal refuses unsettled custody; last-facility retirement cannot discard stock, while retained ordinary freight facilities keep the Station inventory. | src/launch_center_cmd.h, src/station_cmd.cpp |
| LaunchCollectionHub, LaunchHubMemberRef, CmdSetLaunchHub, ResolveLaunchHubMember | Explicit exclusive same-company Surface freight membership with saved generations and whole physical footprint inside the separate20-tile collection circle. Global station spread/catchment and same-XY rock-plane stations confer no membership. | src/launch_center.h, src/launch_hub.cpp |
| SurfaceHandlingJob, CanStartSurfaceHandling, CmdStartSurfaceHandling, TickSurfaceHandling | A job owns its actual packet list once. Shared8-unit/tick throughput, bounded storage/reservations and mass fees apply. Deliberate unlink refuses active jobs; native range/removal changes settle remaining assignments before retiring membership. | src/launch_center.h, src/surface_handling.cpp |
| HubCargoTransfer | Neutral native packet moves preserve recorded age/source/routing/feeder and loading history, permit required split/merge and create no freight payment. Actual vehicle loading/unloading uses explicit physical tiles. | src/hub_cargo.h, src/hub_cargo.cpp |
| SpaceVehicle, SpaceCraftRef, SpaceManifest, CmdMoveLaunchFreight, CmdCarryLaunchProbe | Ordinary Vehicle pool owns the original paid rocket/miner/survey/builder identities. Real cargo and supply compartments remain sole owners; construction probes accept STEL/GOOD within native mass capacity. Ordinary rail4/5 support GOOD; native mineral-only trucks7/8 do not acquire that capability. | src/space_vehicle.h, src/space_manifest.h, src/launch_center_cmd.h |
| SpaceTransition, CmdBeginLaunchFlight, CmdCancelLaunchFlight | One actual rocket retains preparation/ascent/delivery/return/turnaround, original passenger and target/recovery generations, once-only fuel/material/mission receipts and deterministic pad arbitration. | src/space_transition.h, src/space_transition.cpp |
| GetSpaceResupplyBlock, CommitSpaceResupply | Current owned Craft target admits all real supplied compartments together; a deployed builder can receive actual STEL/GOOD. Invalid/full/lost receivers retain cargo or permit explicit compatible recovery. | src/space_manifest.h, src/space_manifest.cpp |
| GetOrbitalModuleFreightBlock, CommitOrbitalModuleFreight | Direct full-load flight to an actual owned operational Core/Storage/Dock uses the shared berth and sole module inventory; RFMT/ELEC and freight move once with no income. Producer absence cannot become a local proxy receiver. | src/orbital_freight.h, src/orbital_freight.cpp |
| SpacePod, SpaceDescentJob, CanReleaseOrbitalCargo, CommitOrbitalCargoRelease, CmdReleaseOrbitalCargo | Real paid carried shell releases current physical cargo inside the explicitly selected owned inclusive20-tile circle. Job stores its actual off-center point and generation-qualified target; full/lost targets retain packets for explicit recovery. Successful completion consumes the original shell once. | src/space_pod.h, src/launch_center.h, src/space_descent.cpp |
| Typed Space orders and company custody guards | Loaded recall stops within its owned circle; Visit retains precise arrival; Unload invokes the real release boundary. Wrong/current-position/stale/duplicate commands cannot mutate cargo. Manual/bankruptcy deletion refuses unsettled Space custody before generic vehicle teardown; actual ownership transfer preserves original references. | src/space_orders.h, src/space_orders.cpp, src/space_vehicle.cpp, src/company_cmd.cpp |

Pickup demand is the existing per-cargo
GoodsEntry::HasVehicleEverTriedLoading predicate. Automatic member allocation
currently selects Asteroid-origin stock in stable member/cargo order. Compatible
reverse supplies and processed Goods recovery can use explicit timed handling.
There is no additional public filter command, cargo registry or payment owner.

Recorded transit periods never become fresh because of relink/neutral handling.
Ordinary waiting station cargo does not accumulate vehicle transit periods;
vehicle, descent and long retained handling controllers own their actual saved
age opportunities. CargoPacket::GetDistance requires in-vehicle custody. The
circulation verifier reads actual dense CAPA transport for stored travel vectors
instead of violating that precondition or flipping a packet flag.

Handling debits canonical cash and yearly ExpensesType::Other. The economy
summary intentionally includes running/property/interest expense categories,
so it cannot substitute for the actual handling-fee ledger. Canonical income
and delivered cargo remain unchanged by launch, descent or neutral hub routing.

## Persistence and current verification

Current schema is423. VEHS reconstructs the native vehicle class before its
saved members; historical enum meanings remain unchanged. SPFL retains its
84-byte row and uses flag bit4 for once-only module delivery. Original supplied
414/418 inputs remain immutable; ordinary loading does not create a producer,
replace original hulls or reissue material/capital receipts. The normal Vehicle
constructor initializes round_trip_time for every body, including wagons that
never own orders, so uninitialized allocator bytes cannot enter saves.

The current exact-owner comparison covers26 families: SPST, SPKN, SPDJ, SPFL,
SPOD, SPSU, SVST, LCOP, SDEF, SPOR, ASTR, VEHS, CAPA, PLYR, STNN, LCTR, LCHB,
LHND, UPLH, UPLD, UPPH, UPPD, TUPH, TUPD, TUVH, TUVD. Replayed saved inputs and
ordinary before/after resaves preserve their actual bytes. Latest18/19/20 native
pairs also match both configurations; command-order pairs match both sequences.

| Plan9 scope | Frozen acceptance index | Results per configuration |
| --- | --- | --- |
| Direct module freight/failures,11/11C | logs/module-flight-verification.json |494 focused assertions/18 cases; current direct-module positive/failure, interrupted/paused and malformed full/preview evidence |
| Current release,12 | logs/release-admission-verification.json |279 focused assertions/8 cases;1,185 original paid navigation/admission assertions |
| Descent/service/custody,13 | logs/descent-custody-verification.json |223 focused assertions/9 cases;2,257 native assertions/10 cases |
| Cargo return/Visit,14 | logs/cargo-return-verification.json |351 focused assertions/11 cases;1,985 native assertions/3 cases |
| Company/phase failures,15 | logs/phase-failure-verification.json |308 focused assertions/12 cases;440 Release/439 Debug native assertions/4 phases |
| Retirement/placement,16.1/16.3 | logs/launch-retirement-verification.json |433 focused assertions/5 cases;201 native assertions/2 cases |
| Paid probe phases/ledgers,17 | logs/phase-ledger-verification.json |567 focused assertions/13 cases;10,913 new native assertions/15 cases;23 retained phase pairs |
| Physical Surface-Space-Goods loop,18.1 | logs/commodity-loop-verification.json |11,152 native assertions/8 cases;44 reports/66 artifacts/32 accepted processes |
| Exact geometry/current release order,18.2-18.4 | logs/release-geometry-verification.json |201 focused assertions/3 cases;4,716 native assertions/4 cases;20 reports/35 artifacts/16 processes |
| Multi-cargo/boundary collection,19 | logs/surface-collection-verification.json |612 focused assertions/6 cases;5,962 native assertions/12 cases;62 reports/99 artifacts/48 processes |
| Repeated neutral circulation,20 | logs/hub-circulation-verification.json |5,910 native assertions/2 cases;32 cycles/64 legs each;12 reports/18 artifacts/8 processes |

Group21's read-only artifact audit verified1,056 referenced report/artifact/
predecessor identities across these11 indexes. Historical executable hashes
record the actual completed processes; build paths are overwritten by later
batches and cannot be compared as though old runs used the current executable.
No original input or failed run is republished as accepted current evidence.

Focused runtime commands run at repository root. The native hidden tags are
selected by context-aware wrappers; original paid predecessors must be named
explicitly, never discovered as the latest file. Every bounded stage remains a
separate invocation, avoiding a full regression or a long monolithic fixture.

```powershell
build/RelWithDebInfo/openttd_test.exe '[orbital-release-geometry],[launch-center-geometry]'
build/RelWithDebInfo/openttd_test.exe '[surface-collection-contract],[surface-handling-native],[launch-hub-native]'
python -B src/tests/multilayer/acceptance/surface_collection_checks-1.py --configuration RelWithDebInfo --stage release
python -B src/tests/multilayer/acceptance/hub_circulation_checks-1.py --configuration RelWithDebInfo --predecessor mlm/reviews/space/collection-verify-settle/20261014-165709-78157655f0/reports/collection.json
```

The same focused commands support Debug. The six collection stages are release,
handling, settle, unlink, expand and delete. Handling consumes its accepted
release report; all four settlement/lifecycle outcomes independently consume
the SAME accepted partial handling report. Exact accepted per-configuration
sources are in logs/surface-collection-release-progress.json and
logs/surface-collection-debug-progress.json. Earlier phase/flight producer
commands and exact inputs remain in their owning Plan9 group notes/indexes;
stale415/5000-fee monolithic wrappers are historical, not current423 acceptance.

## Remaining gates and continuation

Task16.2 remains unchecked. The actual facility.launch-center manifest entry is
planned: no source path, packed offset or packed SHA256 is delivered. Matching
Plan14 authored/runtime groups remain unfinished. Published geometry/templates
and development/base sprites are not final packed artwork or personal approval.

These local command-order and integer simulation tests do not establish a live
network session's full lockstep/reconnect acceptance. That gap remains explicit;
no snapshot or cached declaration substitutes for a current runtime producer.
Plan10 resource/motion/probe/mining continuation and Plan11 Assembly/manufacture/
processing/late-consumer work remain governed by their own reading/entry gates.

The parent index and Plan9/full-space completion remain open while the source
checklist/gates are unfinished. Architecture documentation records the delivered
runtime without marking missing artwork or later producers complete. The next
independent master queue is Plan10 Group1 task1.4 after the source's required
reading; return for scoped art/dependent acceptance when their real producers
are delivered. No new source/test file begins with the planning r2 prefix, no
Git writes were used, and builds remain batched with two outer MSVC workers and
CL_MPCount=2. Next auxiliary prefix is r.
