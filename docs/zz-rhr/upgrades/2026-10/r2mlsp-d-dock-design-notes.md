# R2MLSP-D - Dock Consumer Design Notes

Owner: [Space master](../../../zz-reach2/upgrades/2026-10/r2mlsp-space-master-plan.md) and [Plan11 Group4C](../../../zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md#group-4c---activate-dock-services-and-verify-module-dependencies). Updated: 2026-10-14. This execution record separates current source progress from the historical design proposals below. Source changes are not acceptance evidence.

**Final Group4C checkpoint,2026-10-14:** RelWithDebInfo game/test builds,
294 focused assertions in12 cases and all six native phase/refusal reports
passed. Exact frozen evidence: `logs/orbital-dock-verification.json`. Actual
paid deliveries, two admitted service hulls, original Core competition,
pause/zero generation/upkeep,26-family replay/resave, cancellation, shared
consumer retirement and all four loader/preview faults are verified. The final
native inspector capture was inspected after correcting shared module labels
with a language-only build. No process is running. Group4C is checked in both
plans; next is Group5 safe removal. Debug for4/4B/4C remains queued with the next
substantial removal/handoff batch. Any Group5 persistence extension must use
the next explicit schema (422); do not change the verified421 layout in place.
Earlier pending snapshots below are historical and superseded by this banner.

**Execution started,2026-10-14:** Group4B/Plan8 Group11 passed RelWithDebInfo
contracts plus static/body replay/ordinary resave and all four malformed cases.
Its exact proof is `logs/space-approach-verification.json`. Group4C's Global,
Delivered construction Area, module-construction Local and Dock Local reading
have been read. The implementation is in progress and has compacted once.
The first bundled421 RelWithDebInfo build found one missing declaration
include in `space_orders.cpp`; it was fixed. The incremental continuation
`logs/orbital-dock-release-build-2.log` has built the game and is compiling the
test target; no Dock test has run. Finish4C acceptance before beginning Group5's player-facing removal.
The Dock source, two-job persistence/validation, GUI, explicit disconnect,
atomic shared utility retirement and bounded native delivery/service phases
were bundled into this build. Earlier proposals
below are historical design notes; the following actual state is authoritative.

## Current source changes, not yet verified

- `OrbitalModule`/record contain the original first job plus `dock_service` and an explicit `utility_source` native structure reference. `GetServiceJobs` enumerates both physical owners; only Core1/Dock2 grant capacity. `AreOrbitalModuleFootprintsAdjacent` is the shared pure edge query.
- Dock activation is supported without registering a panel. Its two physical slots persist after paid activation even when Core/power is unavailable; actual handling/work still checks operational dependencies. This preserves accountable berth1 during outages.
- Source registration is explicit via appended synchronized `ConnectOrbitalServices`, with one immutable consumer capped at2*750J and the original adjacent Core/Power source. No energy is created. Service lookup/control, job ticks and freight/part exchange select the actual contained berth.
- `IsOrbitalServiceWorkTick` derives each berth's admission from the shared remaining grant plus earlier eligible unsettled jobs. This keeps upkeep accurate before and after berth0 commits when an810J Core cannot fund both jobs. No per-berth energy ledger/cache was added.
- Capture/install/digest, packet exclusivity, aging and teardown now enumerate both jobs. SPST421 appends a10-byte source reference and39-byte empty second-job state plus actual packet references after the older module prefix. Old modules get no connection/second job; older files cannot activate a Dock. Writer/decoder use the existing first-job field order. Structural, utility, full native and independent copied-preview validation are being extended together; review them before a build.
- Native queue edits now refuse while a contained service job retains the craft, preserving its actual Dock reservation until explicit cancellation/completion. Historical Core service has no implicit new reservation requirement.
- The construction UI now has source selection/connection and selected-berth controls; both lost occupants and outage-held parts remain cancellable through their saved job. New English strings are appended. Read only actual native state, and inspect this UI in the real game when acceptance runs.

Still needed: new Dock command/bounds/native acceptance fixtures, producer
catalogue entries, full review of validation/UI behavior, consumer retirement
and removal/evacuation, final handoff, and focused Release/Debug validation.
Old active-group wrappers pin419/420 output; reconcile their current-output and
SPST parsing with421 while preserving genuine old inputs and semantic
old-version corruption gates. Do not count an unrelated framing failure as the
intended activated-module/schema refusal.

**Additional source progress:** `src/tests/orbital_dock.cpp` now has isolated
activation/connection/purity/role-boundary cases, registered in CMake, but has
not compiled or run. Runtime service arbitration and copied validation now
enumerate both jobs; source code still needs final review and native acceptance.
The inspector has selected-berth/source controls and appended translated text.
The asteroid packet-provenance audit was also extended to the second job.

`orbital_dock_native.cpp` and `orbital_dock_checks-1.py` now split genuine418
Steel/service-parts import, Goods import/construction/activation, two-berth
service/replay, and cancellation into bounded phases. The native observer uses
actual rocket commands, queue movement and common750J work; it contains no
packet/pose/frame injection. `orbital_dock_checks-2.py` has independent source,
duplicate-hull, duplicate-part and correctly framed older-schema refusals.
These fixtures and `orbital_utility_retirement.cpp` are registered but unrun.
Existing active Group4/4B fixture outputs now name/assert421. Older-schema
activation checks remove the421 SPST appendices and420 SPOR request fields,
then require the explicit unsupported-activation refusal, preserving the gate
against unrelated framing failures. UI automatic hull refresh preserves the
selected held/lost berth so its real parts remain reachable for cancellation.

**First validation burst:** RelWithDebInfo game/test build succeeded after the
missing include fix. Focused Dock/activation/approach/construction/Core contracts
passed294 assertions in12 cases (`logs/orbital-dock-release-contracts.log`).
The first native Steel phase refused its receiver selection: the genuine418
frame predecessor ends both builders at Storage, not Core. Source inspection
confirms the earlier pose assumption is stale. The fixture now selects an actual
empty paid builder and reaches Core through native Visit/admission before
resupply; a test-only incremental rebuild is pending. No native Dock acceptance
has passed yet. Preserve the original checkpoint unchanged.

Native geometry characterization confirms one legal direct-adjacent2x2
Dock/Power pair around a3x3 Core: for Core(8,4), use Dock(9,7) and Power(11,6).
All existing south approach cells remain free. In the genuine418 fixture, the
existing completed Storage's south approach blocks that Power footprint;
native acceptance may need Group5's real empty-frame removal before placing
Power there. Use the canonical definitions and actual command estimates for
the loaded world's coordinates. Do not widen connection topology or erase a
frame privately just to bypass its paid occupancy.

The real fixture has two paid builders and an existing reusable rocket of
model14. Reuse existing native rocket-to-builder resupply for actual STEL/GOOD
deliveries; direct rocket-to-module handoff is still Plan9 task11.3. A Dock
needs STEL8/GOOD12 and444 construction ticks; Power needs STEL6/GOOD20 and444.
Both then require74 explicit activation ticks. Service parts must be actual
RFMT/ELEC from existing holds/imports, with two jobs reserving one pair each.
Use separate short native phases/checkpoints if resupply/build/service replay
would exceed two minutes, especially Debug. No fixture-produced consumed
receipt or raw packet injection can replace these delivered acceptance steps.

## Removal dependency discovered during implementation

Original4.3's actual-removal acceptance needs the safe producer from Group5.
The source plan now keeps authoritative activation/outage queries in4.3 and
places real dependent-only removal acceptance in5.4, after5.1-5.3. Implement
the Dock producer first, then the related removal/handoff batch in task order,
and mark groups only after their own focused evidence passes. Do not invent
unsafe raw deletion as player-facing removal. Reused numeric module IDs must
still receive a new native generation and cannot inherit old queues/jobs.

Investigate preserving demolished consumed STEL/GOOD receipts in the same
native structure authority, for example bounded aggregate retired-material
counters in SPST421, before deleting native pool entries. An unbounded second
object/tombstone registry would exhaust lifetime IDs or duplicate ownership.
Preflight counter overflow and every physical owner before any deletion, and
never refund spent materials or energy. Actual stock/job evacuation must use
existing native packet transfers and available destination capacity.

Core/Dock service/assembly consumers also need a genuine shared-ledger
retirement API. Do not rewrite a captured whole ledger from this feature.
Review `RemoveUtilityPowerProducer` and share-curtailment internals. Whole
removal must preflight all accounts and sequence capacity before changing any
utility or physical owner; a third operation refusing after the first consumer
retired would violate unchanged refusal. Preserve settled use, unused-share
semantics and unrelated consumers. Source removal must respect still-connected
Docks and active/stocked dependents; outages retain accountable occupancy.

## Relevant findings

The existing `OrbitalModule` contains one `service` job, its dedicated750J
consumer, an original paid panel and one construction consumer. `ServiceSite`,
`ReadyCraft`, `ReadyJob` and `OrbitalServiceStep` in `orbital_service.cpp` all
assume that single Core berth. Extend these actual owners for Dock; do not
introduce an independent cargo/service/utility registry. A queued approach is
contained in the actual hull's `SpaceOrderQueue`; its admitted precise slot is
queried through the shared producer. Completed Visit retains its reservation
until a subsequent action/explicit release, and cancellation alone does not
remove the actual craft occupying the approach.

`RegisterUtilityPowerConsumer` takes explicit original same-owner/local sources
and an immutable bounded capacity. It creates no energy. No public electrical
consumer-rebinding/retirement helper was found; investigate the actual shared
owner rather than mutating its captured copy if source disconnection/removal
requires one. Removal/lifecycle authority remains Group5's responsibility.

## Proposed next batch, subject to source verification

Keep the Core's existing first service job/consumer and paid30kW panel intact.
A Dock should activate its completed paid frame explicitly, with no panel.
Its two precise berths fit inside the previously reserved south approach cell,
using the shared native approach owner. Hold real RFMT/ELEC and freight only
in declared bounded native containers; a connection cannot enlarge another
module's stock. Confirm canonical Dock stock/handling quantities in the shared
budget document before implementing them.

Consider a second contained service job plus one explicit Dock source reference.
A single shared Dock consumer capped at2*750J can avoid partial registration
of two new consumers while allowing each berth's actual750J transaction to
settle once. Requests should sum only eligible jobs, and each job retains its
own saved last-work epoch and manifest service serial. An explicit source
connection should validate an actual physically adjacent owned Core/Power
panel before registering the zero-energy consumer. Actual generation and
common allocation remain necessary; no free Dock panel or private power
balance is permitted. This is a proposed implementation choice, not a
delivered contract or proof of canonical tuning.

Generalize service lookup/control by actual job/craft identity. Preserve
cancellation of lost occupants and return only their unconsumed actual parts.
Use admitted shared berths for new Dock service and packet exchange; preserve
the declared older Core service migration rather than rejecting historical
paid saves merely because they predate shared reservations. If an edit would
drop an occupied Dock service reservation, refuse it or retain that actual
reservation until explicit job cancellation; do not strand parts or secretly
admit another craft into the same physical berth.

Extend SPST through an explicit next save version with bounded contained job,
connection, packet and epoch fields. Old modules receive no second job, source
connection or Dock functionality. Update capture/install/digest, packet
exclusivity, aging, teardown, actual load and copied preview together. Reject
old-version activated Docks, missing/foreign/reused source bindings, duplicate
berths/consumers/parts, copied panels and malformed work receipts. Existing
Group4 wrapper current-output419 assertions must be reconciled when rerunning
them on a newer binary, without weakening old-input/schema corruption checks.

Focused acceptance should exercise two real paid craft, actual delivered
service parts, shared-source competition/zero generation, queued and active
save continuation, cancellation, target/source loss and dependency-local
effects. The genuine paid418 predecessor currently has one complete Storage
frame and real Core Steel plus service parts; constructing a new Dock requires
actual additional Steel/Goods flights/resupply. Use existing public delivery,
reservation and construction commands. A detached unit fixture cannot replace
that delivered native acceptance path.

Bundle Dock, dependency, removal and final handoff edits as their source gates
allow, then build both configurations with two outer workers/`CL_MPCount=2`.
Keep focused native bursts below two minutes; defer broad regression to a
genuine plan boundary. Update checkboxes only after their own evidence passes.
