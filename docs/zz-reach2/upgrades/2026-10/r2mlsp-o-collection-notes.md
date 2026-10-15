# R2MLSP-O - Boundary Collection Notes

Owner: [space master](r2mlsp-space-master-plan.md) and
[Plan9 Group19](r2mlvs9-launch-centers-and-rockets.md#group-19---failures-presentation-and-acceptance).
Updated:2026-10-14.19.1-19.7 remain unchecked. No Group19 compaction yet.
Next auxiliary prefix is p.

After compaction read Plan9 Global, Failures/presentation Area and nearest
Surface collection and retained assignments Local. Actual symbols are
IsLaunchHubEligible, CmdSetLaunchHub, exact member generations,
GoodsEntry::HasVehicleEverTriedLoading (last_speed !=0),
CanStartSurfaceHandling, TickSurfaceHandling and
SettleSurfaceHandlingForStation. Automatic allocation requires an Asteroid
source packet and per-cargo pickup demand; reverse handling stays explicit.
Deliberate unlink refuses active jobs; normal station geometry mutations
revalidate membership and return a retiring member's uncompleted packets to
its retained center. There is no invented HasPickupDemand or filter command.

Group18 is complete: logs/commodity-loop-verification.json (11,152 native
assertions/8 cases each) and logs/release-geometry-verification.json (201 focused
assertions/3 cases and4,716 native assertions/4 cases each),26 owners exact
across ordinary/replayed saves, command orderings and configurations. No need
to rerun accepted18.16.2 delivered artwork remains open under its scoped gate.
The overall master,19/20/21 and Plan10/11 continuation remain unfinished.

## Implemented consolidated19 batch

src/tests/surface_collection_native.cpp, surface_collection_boundaries.cpp and
surface_collection_checks-1.py are registered. First Release fixture build is
running: logs/surface-collection-release-build.log. Wait for its link before
running Release native tests. No Debug build or collection native run yet.
No agents/Git writes/full regressions. Two outer MSVC workers plus CL_MPCount=2.

Native initial release uses only original immutable
src/tests/fixtures/demos/space/space-descent-414.sav,
SHA46fa2a166184269ca89e8af53c302bcb771a63827ac5ee1d1ca53878668235d3.
Original paid miner has10 actually mined NORE with one carried purchased shell;
its first10 NORE already descend. The original paid builder holds40 declared
GOOD test-content units. First settle the center's actual historical Goods into its
existing rail member and unlink that member, so they cannot stand in for newly
recovered Goods. Native build commands make separate near19-20-tile-boundary
rail/lorry members and save their actual cosmetic names/identities. No terrain
patches, vehicle/pod/cargo/capacity/quota/phase/timer/position hacks.

Actual paid service if needed, one stored paid shell, actual rocket fuel/carry
and one mission deploy the same loaded40-Goods builder. Release its same40
Goods, navigate the original miner with a real Visit to a two-tile off-center
point and release its remaining10 mined NORE with its original paid shell.
Save the real off-center descent. Handling waits for both types to finish,
reserves10 ore/20 Goods for each real boundary member and observes one partial
native opportunity. Settle completes those four jobs. All stages replay their
saved native input and compare ordinary before/after resaves across26 owners.

Six stage choices: release; handling consumes the exact accepted release
report; settle/unlink/expand/delete EACH consume the same exact accepted
handling report, not another lifecycle branch. Unlink must refuse while active,
then succeed after full settlement. Expansion extends the actual boundary rail
one tile outside: its completed8 ore remain there; unfinished2 ore/20 Goods
return to the center while lorry receives10/20. Deletion removes the empty but
assigned native lorry bay: its10 ore/20 Goods return to the center while rail
finishes10/20. Normal station IDs and cargo owners remain authoritative.

Isolated boundary tests declare only their starting packets/demand/tiles. They
exercise full lower-ID member storage, per-cargo ore/ice filters, demand changes
that preserve existing assignments, foreign/unlinked/same-XY Underground
refusals and separate membership under spread1 and both catchment policies.
They never export a paid native world or claim produced quantities. Focused
commands select [surface-collection-contract],[surface-handling-native],
[launch-hub-native]. Native tag is [.surface-collection].

No task is checked until the current implementation compiles and focused/native
acceptance proves its actual requirement in both configurations. Preserve all
failed runs separately. Do not infer full goal closure from this fixture batch.

## First consolidated verification

Release build passes. Focused collection tests pass612 assertions/6 cases.
Native release20261014-163010-91e51a52b3 failed before recovery because this
original414 descent input has104 center Goods, unlike17's64. Builder cargo40
GOOD remains actual. Staging now queries the real historical center inventory
and moves it through successive bounded handling jobs before return. Those104
units stay distinct from the newly recovered40 in their existing station owner.
No balance, packet quantity or saved input was changed to fit an assumption.
Next compile includes this fixture correction; all19 source tasks remain open.

## Native progress, current commands

Release recovery and partial handling pass, as do baseline settlement, refused
active unlink followed by successful settled unlink, and empty assigned bay
removal with retained pending packets. Exact release report:
mlm/reviews/space/collection-verify-release/20261014-163710-24ee7d6630/reports/collection.json.
The actual partial handling report is named in
logs/surface-collection-release-progress.json steps[1]. Lifecycle variants must
consume that exact report, never each other's output. Debug build and focused
612 assertions/6 cases pass; no Debug native collection run yet.

Expand20261014-163924-1926d12e98 failed its station command before mutation.
FindJoiningBaseStation's automatic neighbor scan rejects more than one nearby
station even with a requested join ID. The fixture now uses the delivered
explicit adjacent/join choice to expand only the original boundary rail beside
its separate lorry neighbor, and checks a native player-flags quote before Post.
No gameplay fallback or terraform/private tile workaround was added. Release
build-3 is running. Revalidate expansion, then consolidate current results and
run all six Debug stages after its updated link.19 tasks remain unchecked.

## Boundary-site correction and current pass

Further native expansion quotes reported4289 (FLAT_LAND_REQUIRED), and the
other outward neighbor was occupied. The earlier neighbor-scan explanation
was a command-contract observation, not sufficient evidence of that run's
root failure. The supplied plateau's first boundary position is unsuitable
for the later expansion test. A attempted public leveling command also
refused; no terrain workaround remains in current source.

Current BuildBoundaryMember preflights one naturally buildable adjoining
out-of-circle rail tile BEFORE selecting the actual19-20-tile member. This
retains the full range-changing active-job test and uses only native placement
queries/public station construction. Release build-6 passes; the fresh six-stage
Release chain is running (session45784), with progress persisted after each
accepted replay/resave stage in logs/surface-collection-release-progress.json.
It must finish before updating Debug to the same fixture and running its six
stages. Both focused612 assertions/6 cases already pass; source19 tasks remain
unchecked until all required current native/boundary evidence is frozen.

Additional excluded failed expansion runs:20261014-164633-8bb437eac5,
20261014-164841-f2a97e9f97,20261014-165213-6417d72071. No agents/Git writes/full
regressions, no source/test r2 prefixes and no created MD outside this directory.

## Final19 acceptance

Both final six-stage chains pass. logs/surface-collection-release-progress.json
and logs/surface-collection-debug-progress.json name only accepted current
site-preflight reports. logs/surface-collection-verification.json freezes62
reports/99 artifacts/48 accepted processes. Each configuration passes612
focused assertions/6 cases and5,962 native assertions/12 cases. All26 owners
compare ordinary/replayed before/after saves and both configurations. Native
source inputs remain immutable.19.1-19.7 and master19 are checked. No process
remains running. Earlier session/build-running paragraphs are historical.

Group20 is next, with its own nearest circulation/canonical cargo-history Local
block. Reuse each configuration's accepted settle stage from the exact progress
index. Old Goods remain at the historical separate station; newly recovered40
are20 each at the boundary rail/lorry. Reverse/outbound Goods commands can route
whole member loads without a new cargo/vehicle producer. Read actual ageing
owners before claiming repeated cycles cannot reset freshness. Do not skip
fees, generations, routing, source, feeder share or company delivery ledgers.
No Group19 compaction occurred. Next auxiliary prefix is p.16.2 graphics and
20/21 plus Plan10/11 continuations remain open; the full master goal is active.
