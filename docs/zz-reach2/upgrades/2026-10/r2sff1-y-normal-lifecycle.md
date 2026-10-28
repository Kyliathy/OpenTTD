# Sole normal-loop asteroid lifecycle

Owner: [Space fixes plan](r2sff1-space-fixes-features-1.md), Group29.

## Runtime ownership and order

`TickAsteroidField` now accepts only normal unpaused opportunities with an active
field and matching actual Map geometry. The editor call is removed; menu/direct
paused calls also refuse progress. `StateGameLoop` still publishes the field
before the existing `CallVehicleTicks` approach/power preparation and ordinary
native decisions. No second arbitration, clock, cargo owner or saved counter is
introduced. New renewals use the existing445 persistence contract.

The owner gathers copied outward IDs in bounded scratch and stable ID order,
releases unsafe native miners/ownerless pursuits, and retires the verified batch
at its original last-contained checkpoint. Retirement precedes survivor movement
because full validation must see consistent original preview attachment joins.
Each survivor then consumes one pure straight step and spatial update. Ownerless
observation advances once against the complete new field, keeping its attachment
joins valid before due creation's validation. A native renewal quote projects
only an exact existing mining lease's pending attachment centre, avoiding an
obsolete hull pose during this in-progress field translation. Final actual
native attachment transforms publish after renewal; released hulls do not follow.

Only credit armed at opportunity entry consumes one countdown tick. First credit
keeps its complete fresh interval. An expiring interval sees this opportunity's
sorted exit/latest metadata and admits at most one replacement. Its new motion
starts next opportunity. No index/tile conversion receives an outside centre.
Invalid geometry/counters, failed mandatory retirement/index storage, or a
nonclosing due interval use the engine's terminal error path, never hidden
freezing, fabricated rollback or a second time owner. Ordinary refused creation
still closes its tested failed-attempt interval and retains all credit.

The copied-field reflection writer is removed. Short-horizon component tests
use `asteroid_snapshot_fixture.h`, which refuses installed owners and outward
horizons before mutation. It grants no native retirement, credit or renewal.
Historical arithmetic reflection utility tests remain distinct from the live
field policy. The old native strip observer now expects straight velocity and
explicitly unpauses its producer opportunity; older component strip assertions
are updated. Ordinary test-target build/remaining motion-runner acceptance is
part of the later affected-demo/final gates, not claimed by the focused target.

## Focused and actual-application verification

RelWithDebInfo and Debug each pass **81,421 assertions in31 cases** with
`[asteroid-lifecycle-tick],[asteroid-creation],[space-mining-departure],
[space-survey-departure] --warn NoTests`.
Logs: `logs/space-fixes/lifecycle-component-{release|debug}.log`.
The final matching-Map preflight also passes all eight lifecycle cases,
**676 assertions** per configuration, in `lifecycle-final-{release|debug}.log`.
Sequential final build logs: `build-lifecycle-final-{release|debug}.log`.

Cases cover old-strip crossing, simultaneous sorted exits, full first-delay
arming, one countdown/one birth, insertion pose/next-tick motion, same-tick exit
and expiry, stopped real mining with earned packets, actually earned ownerless
attachment during creation, all-body retirement/empty delay, and direct
paused/editor/menu/bootstrap refusal. The stopped miner retains its original
idle upkeep on its later ordinary craft tick; the field producer earns no cost.

`space_fixes_checks.py --save space-mining-working-405.sav --asteroid-lifecycle`
passes **76 assertions** in each actual application. It declares only terminal
geometry and a later one-tick deadline, stops actual saved hulls through native
commands, and uses `StateGameLoop` exclusively for completed opportunities.
Four ordinary opportunities retire three originals and admit one replacement.
Real paused/editor callbacks freeze field/countdown; hidden/visible Space and
the actual fast-forward switch retain one increment per completed opportunity.
Fast-forward is an opportunity-semantics check, not a wall-time speed benchmark.
Original packet identities/sources/quantities stay equal. Native roles, partial
work, leases and continuation validate after every completed opportunity.

Source SHA256:
`dc5f93d90a2f21d8f5dca2f76bef10fc6ab4e0578907cf5e8bbb44db761a0fe5`.
Runtime artifacts are reproducible under
`test-cache/space-fixes/lifecycle-native-{release|debug}/lifecycle-current.sav`.
Separate application reloads resave under `lifecycle-reload-{release|debug}/`.
Complete independent wire comparisons preserve all three packets/40 units,
three hulls' missions/stop/continuation, and the eight-body field/ledger/full
random streams. Logs: `lifecycle-{cargo|mining|field}-wire-{release|debug}.log`.
All original/current fixture bytes remain untouched; driver logs are separate
`-driver.log` files. Native reports record their actual executable fingerprints.

The save-only packet oracle correctly rejected comparison of an original405
save against elapsed gameplay because an ordinary Surface packet legitimately
acquired source coordinates/travel distance. That diagnostic remains
`lifecycle-cargo-elapsed-mismatch-release.log`. The native elapsed observer
checks immutable identity/source/quantity; the unchanged complete-row wire
oracle compares the resulting checkpoint to its no-op reload/resave. Its
contract was not weakened to suppress ordinary accounting progress.

Final focused binary SHA256, after the matching-Map preflight:

- Release: `932138c56a9626813f9404f9b31abfa65ed84bf3d14a6e83b78d8c62544b17d2`.
- Debug: `ff6c27f82dd059b6bad7a9f628e2f50ebe9105a8141d02545d72f7a74785fc14`.

CRLF-aware `git diff --check` passes. No full regression or financial trial ran.
Broader order/receiver/service scenarios, long resume/network/performance gates,
ordinary game executable build and the unavailable personal-save reproduction
retain their separate plan status.
