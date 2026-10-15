# R2MLSP-M - Actual Commodity Loop Notes

Owner: [space master](r2mlsp-space-master-plan.md) and
[Plan9 Group18](r2mlvs9-launch-centers-and-rockets.md#group-18---failures-presentation-and-acceptance).
Updated:2026-10-14. Group18 has compacted once.18.1-18.4 remain unchecked pending
both-configuration acceptance. Next auxiliary prefix is n.

## Required reading and retained authority

After compaction read Plan9 Global, Failures/presentation Area and nearest
Actual Surface through Space Local. Relevant boundaries are native rail/depot,
vehicle/refit/orders, exact hub/member generations, MoveLaunchFreight,
CarryLaunchProbe, Craft resupply, purchased pod movement and release/descent.
No private vehicle positions, phases, timers, capacities, availability or cargo
may substitute for actual commands. No agents, Git writes or full regressions.
Two outer MSVC workers plus CL_MPCount=2. Do not run a native test while linking
that same configuration. Previous17 proof is logs/phase-ledger-verification.json;
16.2 authored/packed graphics remains open under its scoped gate.

## Current actual loop

The source is17's original completed paid flight, named explicitly by its
accepted report. Both configurations retain the same after-save SHA256
24695a086ccb9be971645eeb8a13a62b9a1d1c721fa60e5490158ca612b9d7c7.
Release report: mlm/reviews/space/paid-probe-phase-verify-complete/20261014-122529-d67e85300f/reports/paid-probe-phase.json.
Debug report: mlm/reviews/space/paid-probe-phase-verify-complete/20261014-122825-bedbe1aee9/reports/paid-probe-phase.json.

The original rocket15, builder18 and original purchased pods retain their
native identities. The builder's old40 units are GOOD, not Steel: settle them
through the public same-site freight command BEFORE staging the Surface load.
Together with64 existing center Goods,104 declared test-content units exist.
Timed outbound handling stages20 at separate rail member5, leaves84 at lorry
member6, then unlinks only member5. Normal paid rail/depot/station commands build
a short route: existing rail at22,6; gap21,6; new joined center platform20,6;
depot23,6. Native rail models4/5 are genuinely available and support GOOD.
Ordinary pickup/transfer orders move those20 Goods by actual train19.

Deploy performs actual paid Earth service on the original empty builder if its
quota is zero, attaches an existing paid stored pod, and uses a fuelled paid
rocket mission to deploy that same builder at the explicit home zone. Resupply
packs only the physically train-delivered20 Goods and uses a second actual
rocket Craft-target mission. Return releases the builder's same20 Goods with
its purchased shell, completes native descent once and collects through timed
handling into separate linked lorry6, ending with104 Goods there. Canonical
final Goods deliveries remain0; ordinary transfer feeder credit is separate.

src/tests/surface_space_loop_native.cpp and commodity_loop_checks-1.py are
registered native producers. Four bounded stages are prepare/deploy/resupply/
return. Each compares its saved-input replay and ordinary before/after resaves
across26 owner families. All four Release stages passed with fixture build-12:
logs/commodity-loop-release-progress.json names only their accepted reports.
The initial Debug chain also passed all four replay/resave stages. A stronger
cross-configuration comparison then found exactly four VEHS bytes differing on
the new wagon: Release0, DebugCDCDCDCD. The final common saved field is
BaseConsist::round_trip_time, which lacks a default initializer and is cleared
by the primary's order reset but not by ordinary wagon purchase. Vehicle's normal
constructor now initializes that duration to0; native purchase assertions check
both units. No fixture state normalization masks this product defect.

A bundled final Release product/test build is running in
logs/commodity-loop-release-final-build.log. Debug needs the same final batch.
Re-run the explicit four-stage chains after each link, then generate
logs/commodity-loop-verification.json with ordinary/replayed26-owner and
cross-configuration comparisons. The initial accepted chains above are
superseded by this current-binary validation.18.1 remains unchecked until that
proof is complete; do not mask VEHS or settle for a weaker comparison.

## Failed or superseded attempts, excluded from acceptance

Builds1/5/6 failed compilation and were corrected without gameplay workarounds.
Road attempts20261014-131124-504b4e83f6,20261014-131512-ae377fafcb,
20261014-132145-600c2884bb,20261014-132908-8d8363c2a4 failed. Error4538 means
ROAD_VEHICLE_NOT_AVAILABLE;35019 means UNDERGROUND_VEHICLE_CAPABILITY. Native
trucks7/8 do not carry GOOD; model6 is a Rail wagon. No availability bits changed.

Rail attempts20261014-152327-d99dbc892f,20261014-152727-0fdc7bfd7f,
20261014-152955-c04de2fd19 failed route admission: estimates lacking player Auto
flags permitted demolition of the lorry station. Current route quotes include
actual Auto/NoWater flags and avoid that station. No natural water reclamation
remains in the current source.

Prepare20261014-153221-91bb4f4659 passed its narrower stage, but its chain is
superseded: deploy20261014-153249-a0a408c716 exposed the old builder's40 Goods
mixing with newly delivered20. Build-12 settles historical Goods before the
actual train arrival. Never use the superseded prepare or failed deploy as a
current predecessor.18.2-18.4 geometry/current-position/command-order tests
follow full18.1 acceptance;19/20 collection/ledger work and21 handoff remain open.

## Final18.1 acceptance

The final bundled Release/Debug product/test builds both completed. The fresh
four-stage chains in logs/commodity-loop-release-progress.json and
logs/commodity-loop-debug-progress.json both pass11,152 native assertions/8
cases, including saved-input replay. All26 owner families compare exactly
before/after each stage, in ordinary resaves and across configurations.
logs/commodity-loop-verification.json freezes44 reports/66 artifacts/32 accepted
processes and retains the original17 predecessor identities.18.1 is checked;
18.2-18.4 are next. No process remains running. The earlier "build running"
paragraph above is historical; final proof is the authoritative current state.
