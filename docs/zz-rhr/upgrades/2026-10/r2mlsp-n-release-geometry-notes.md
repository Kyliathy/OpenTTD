# R2MLSP-N - Release Geometry and Command Ordering Notes

Owner: [space master](../../../zz-reach2/upgrades/2026-10/r2mlsp-space-master-plan.md) and
[Plan9 Group18](../../../zz-reach2/upgrades/2026-10/r2mlvs9-launch-centers-and-rockets.md#group-18---failures-presentation-and-acceptance).
Updated:2026-10-14.18.1 is checked;18.2-18.4 acceptance is in progress.
Group18 has compacted once. Next auxiliary prefix is o.

Read Plan9 Global, Failures/presentation Area and nearest Actual Surface through
Space Local after compaction.18.1 final proof is
logs/commodity-loop-verification.json:11,152 native assertions/8 cases each,
four original paid train/deploy/resupply/descent-collection stages,26 exact
owners across ordinary/replayed saves and both configurations,44 reports/66
artifacts/32 accepted processes. The normal Vehicle constructor fixes the
previously uninitialized wagon round_trip_time; no fixture masks saved bytes.
See r2mlsp-m-commodity-loop-notes.md for exact source identities and failed runs.

## Current18.2-18.4 batch

src/tests/orbital_release_geometry.cpp adds two isolated copied geometry/schema
cases plus one genuinely paid loaded-world observer. Copied records never
allocate cargo, pods, vehicles or gameplay saves. Exact-circle checks cover the
anchor, four signed20-tile axes,12/16 boundary diagonal,14/14 inside,15/15
outside, and one Q16 unit beyond the axes/diagonal. Transport rejects one unit
outside all four map corners without wrapping; overlap binds an explicit center
and exact generation, with no fallback to another geometric candidate.

The native observer consumes only the original immutable
src/tests/fixtures/demos/space/space-descent-414.sav,
SHA46fa2a166184269ca89e8af53c302bcb771a63827ac5ee1d1ca53878668235d3.
It retains the original materially paid miner, ten actually mined aboard units
and one purchased carried shell. Actual Visit travel invalidates an initially
valid release quote. Public SetSpaceMission(UINT32_MAX) recalls it with cargo
intact, then actual current-position release selects its owned center. Foreign
and valid commands execute in both orders; invalid and duplicate commands leave
all packet/pod/descent/manifest/cash/RNG owners unchanged. The sole release
records the actual current point and original shell once. No private position,
cargo, phase, timer, quota or target mutation is used.

release_command_order_checks-1.py runs each ordering separately, replays its
saved native input and compares ordinary before/after resaves across26 families.
Source/wrapper producers and bounded contexts are registered. One initial
compile failed for a missing command-trait include and an assumed nonexistent
Recall action; the corrected code includes space_vehicle_cmd.h and uses the
actual delivered public recall command. No gameplay API was invented.

Current Release build-2 passes. Focused geometry passes201 assertions/3 cases;
valid-first and invalid-first native/replay/resave wrappers both pass. Exact
report paths are in logs/release-order-release-valid-first.log and
logs/release-order-release-invalid-first.log. Debug build passes and its same
focused201 assertions/3 cases pass. Debug invalid-first wrapper is currently
running (session70281); wait at least50 seconds when polling. Then run Debug
valid-first. Freeze both orderings across configurations and compare their26
owners before checking18.2-18.4 and master18. Missing16.2 artwork and all19/20/21
collection/ledger/handoff tasks remain open; do not claim full master closure.

No agents or Git writes. Two outer MSVC workers plus CL_MPCount=2. Do not run a
native test during its configuration's link. No full regressions, and retain all
original supplied saves. New source/test names never begin with r2.

## Final Group18 handoff

Both Debug orderings finished. logs/release-geometry-verification.json freezes
20 reports/35 artifacts/16 accepted processes. Each configuration passes201
focused geometry assertions/3 cases and4,716 native assertions/4 cases (two
command orderings plus saved-input replays). All26 owner families match both
ordinary/replayed before/after saves, command orderings and configurations.
18.2-18.4 and master18 are checked. No build or test remains running. The
"session70281 running" paragraph above is historical. Group19 is next, with a
new nearest Local Mandatory Reading block in Plan9.16.2 art remains open under
its scoped gate;19/20/21 and the full master goal remain unfinished.

## Group19 entry observations

Global/Area authority is unchanged; the new nearest Surface collection Local
block names the actual native exports. There is no HasPickupDemand function:
GoodsEntry::HasVehicleEverTriedLoading is last_speed !=0. Current automatic
allocation in TickSurfaceHandling uses that per-cargo predicate and requires
the first stock packet to have SourceType::Asteroid; reverse supplies remain
explicit. Do not infer a separate public cargo-filter command or registry.
CmdSetLaunchHub refuses deliberate unlink/reselection while handling is active.
Native station geometry changes call RevalidateLaunchHubMemberships; settlement
returns a retiring member's remaining assigned packets to the retained center.

19.1 is still unchecked and no Group19 source has been created. The genuine414
descent input contains two original purchased miner pods'20 mined NORE units:
ten already descending and ten held with the remaining carried shell. It also
contains the original paid Surface-stored builder's40 GOOD test-content units.
A multi-type native recovery fixture can use real service/pod/carry/fuelled
deployment to release that same builder Goods, plus real miner navigation and
its second NORE release. Before return, settle the center's old64 Goods through
public handling so they cannot be mistaken for the newly recovered40. Build
real separate rail/lorry members near the20-tile boundary through placement
queries; preserve IDs/generations and normal station-spread settings. Use bounded
off-center descent, partial handling and settlement save pairs; actual supplied
inputs remain immutable. This is an unexecuted design, not acceptance evidence.

Next Group19 source/test names must not start with r2. Next auxiliary prefix is
o. Run focused bursts for active19 only, bundle source/fixture edits before
building, and do not repeat the accepted18 chains without a new relevant change.
The full master goal stays active, with16.2 graphics,19/20/21 and Plans10/11
continuations remaining open.
