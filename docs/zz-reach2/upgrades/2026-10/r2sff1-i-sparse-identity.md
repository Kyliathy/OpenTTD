# Sparse live asteroid identity

Owner: [Space fixes plan](r2sff1-space-fixes-features-1.md), Groups 12-14C.
Runtime boundary: [native handoff](../../../zz-rhr/upgrades/2026-10/r2mlsp-z-z-p-probe-runtime-handoff.md#delivered-ownership).

## Group 12: field storage

`AsteroidField` keeps compact `bodies`, a monotonic `next_body_id` and a private
derived ID-to-slot map. The map contains live IDs only; neither its capacity nor
the body vector scales with the highest lifetime ID. It owns no resource,
knowledge, renewal credit or retired-body tombstone.

`FindAsteroidBody` checks the cached slot's actual identity before returning a
borrowed record. Copied/decoded vectors may have no map or an inherited stale
layout; those lookups can scan the original records without mutating anything.
`ValidateAsteroidBodyIdentities` checks raw sorted identities, independent of
the cache: unique IDs below issuance, bounded live count, no invalid sentinel.
`RebuildAsteroidBodyLookup` publishes a fresh derived map after that check.
Decoded/copied records edited directly must rebuild before structural helpers.

`InsertAsteroidBody` accepts only the current fresh cursor, bounded capacity and
a rebuilt layout. It increments issuance only after both vector and map entries
exist. An allocation refusal rolls back any appended record. The exhausted
`UINT32_MAX` cursor never supplies a body ID. `EraseAsteroidBody` swap/pops the
compact vector and updates the moved body's map entry without changing issuance,
ledger, knowledge or cargo. The lifecycle caller must release dependent owners
before erasing. Every insertion/erasure invalidates borrowed body pointers;
retain IDs or copied terminal events across structural changes.

`GetAsteroidBodiesInIDOrder` explicitly sorts borrowed record pointers for
digests, serialization and later simultaneous events. Hash-container iteration
cannot choose event order. Field equality excludes the derived map while still
comparing original record layout and all gameplay members. Digests include
issuance when it differs from the legacy dense count; pristine dense fields keep
the retained diagnostic byte stream until the complete Group 22 wire contract.

Generation preserves its original content, placement and motion draws, sets
issuance to the generated count and builds lookup afterward. Installation
validates raw records and rebuilds lookup, including old-reader cursor absence.
Reset clears the whole owner and its disposable mappings. Until the later
consumer/persistence gates, active installation still requires dense original
IDs and pristine renewal. Sparse unit fixtures do not authorize live high-ID
extraction or claim a sparse save/reload test.

## Group 12 verification

The focused RelWithDebInfo build passed. `[asteroid-identity]` passed 133
assertions across five cases in less than one second. The tested executable
SHA256 was `b62a5f4cf3f1a6284dee95f495dbf536df7484156f3cf9f183b62ee704237984`.
Evidence is in `logs/space-fixes/build-asteroid-identity.log` and
`logs/space-fixes/asteroid-identity.log`. `src/tests/asteroid_identity.cpp` covers
high IDs, all compact erasure positions, stale copied mappings, duplicate and
unissued IDs, explicit sorted diagnostics, live-count limits, exhaustion at
the last valid ID, native reset/install and legacy cursor initialization.
Run `[asteroid-identity]` from `openttd_space_fixes_test`, recording output under
`logs/space-fixes/`. Sparse spatial membership and consumer migration remain the
next sequential groups.

## Group 13: spatial membership

`SpaceSpatialIndex` replaces ID-sized `locations`/`slots` arrays with a private
live-ID membership map and the existing fixed map-cell buckets. Insert and
Update reject points outside physical bounds before calculating any bucket.
Duplicate insertion, a full live cap and invalid identities fail without
changing the previous query answer. Update retains its old upsert behavior for
existing callers, while native installation uses explicit Insert.

Remove uses bucket swap/pop and fixes the moved ID's slot in constant work.
Cross-bucket Update allocates the destination before unlinking the source;
allocation refusal keeps the original membership. Vacated buckets release their
old vector allocation, so traveling clusters do not leave storage in every
visited cell. The map and capacity hints are capped by live population, never
the highest lifetime ID. No index operation changes body reserve, knowledge,
cargo or renewal credit.

Install builds a complete local index before replacing either live owner.
Queries explicitly sort candidate IDs and reject entirely outside rectangles.
Drawing and clicking now resolve each returned ID through canonical lookup and
skip a disappeared body; an old dense slot cannot silently select its successor.
Selected-inspector recovery and remaining numeric consumers belong to Group 14.

## Group 13 verification

The focused RelWithDebInfo build passed. `[space-spatial-membership]` passed
95,897 assertions across four cases in less than one second. The tested
executable SHA256 was
`41a2ee0b706bc1fcfadfca22bd48a1872b32111148868c39fa443c1105ab0eed`.
Evidence is in `logs/space-fixes/build-space-spatial-membership.log` and
`logs/space-fixes/space-spatial-membership.log`.
`src/tests/space_spatial_membership.cpp` covers
physical-edge rejection, the final valid 32-bit ID, a full 14,336-body cluster,
constant unlink counts, stable query order, 10,000 high-ID replacements with a
bounded live set, no ledger side effects, staged install refusal and reset.
Run `[space-spatial-membership]` and store evidence under `logs/space-fixes/`.

## Group 14: continuing validation and disclosures

The consumer audit split original Group 14 into live-field/disclosure work (14),
detached preview joins (14B), and the remaining native transition/custody audit
(14C). Each has scoped reading and focused checks in the plan.

General `ValidateAsteroidField` now checks copied dynamic state without requiring
the initial population forever. All issued IDs must be accounted for by live
bodies plus cumulative exits. Issuance cannot precede the original generated
count. New issuance must be paid for by spent renewal weight: one to five per
spawn, with the parity implied by costs 1/3/5. These are necessary aggregate
checks; the later transaction owner still proves actual original terminal events
and successful candidate admission. Validation cannot reconstruct full lifetime
history from counters.

Raw bodies must have unique issued IDs, canonical classes/sizes/reserves and
velocities/remainders, and positions within physical-plane bounds. Exact initial
class shares and the three bootstrap identities remain constraints while there
has been no retirement. At elapsed tick zero, placement and frozen starter
geometry remain initial-generation evidence. Continuing bodies may cross the
old placement strip, lose a resource class or all disappear. Neither depletion
nor an uncredited vector erasure proves a terminal exit.

Native `InstallAsteroidField` still explicitly refuses evolved ledger/issuance,
shuffled dense layout and outside-strip geometry while the old writer and
reflection tick remain active. Group 22 must replace the writer gate; Group 29
owns motion publication. Copied dynamic validation does not itself enable either.

Knowledge row shape is separate from membership. The field overload joins every
receipt to an actual live ID; only the legacy overload retains a dense count
bound for old transport/tests, pending 14B's version-aware candidate work.
`RemoveAsteroidKnowledge` removes only one terminal body's live receipt without
touching reserve or credit. Quantity disclosure and completed scans resolve the
actual body, so a missing ID cannot borrow a new occupant of its dense slot.
Native company authorization remains a separate required check.

The inspector resolves selected bodies through canonical lookup. Reopening keeps
a surviving ID or deterministically chooses the lowest live ID; an empty field
clears body selection. A retained window cannot display a disappeared body's
reserve. Ownerless preview stepping also resolves IDs and skips absent targets;
Group 26 still owns actual preview retirement/return behavior. Demo starter
commissioning and current legacy cargo validators remain in 14C's audit.

## Group 14 verification

The focused RelWithDebInfo build passed. `[asteroid-live-validation]` passed
94 assertions across four cases in less than one second. The executable SHA256
was `6112d77df54ddff2f860d1a16b715105d2bde11d16a5be1072cae121ff1fb12b`.
Evidence is in `logs/space-fixes/build-asteroid-live-validation.log` and
`logs/space-fixes/asteroid-live-validation.log`.
`src/tests/asteroid_live_validation.cpp` declares
copied terminal histories and checks empty/changed/high-ID fields, missing and
corrupt knowledge, physical geometry, stable selection and legacy publication
refusal. Its native unit fixture checks actual company-gated disclosure; it
claims no paid procurement, native terminal release or new-format reload.
Run `[asteroid-live-validation]`; record build/output under `logs/space-fixes/`.

After a compaction during Group 14, reread the plan's Global Mandatory Reading
and the Renewal accounting/live identity block, then the current named field
validator, install gate, knowledge functions and selection helper. Old line
ranges are stale after Groups 11-14. The former dense-only validator is now an
explicit install compatibility gate; do not recreate its assumptions inside
general copied-state validation. Later 14B/14C have their own narrower blocks.

## Group 14B: detached candidate identity

`AsteroidIdentityRecord` copies only a stable ID and resource class.
`GetAsteroidIdentityRecords` captures sorted rows from one decoded owner;
`ValidateAsteroidIdentityRecords` rejects duplicate/descending, unissued,
sentinel and unknown-class rows without consulting live state.
`FindAsteroidIdentity` binary-searches the validated candidate rows only.

`LoadCheckData` now retains `asteroid_body_rows` and `asteroid_next_body_id`.
`asteroid_body_count` is framing, never an ID bound. ASTR LoadCheck supplies all
three from the same decoded file, and Clear discards them between candidates.
Knowledge receipts and SPOR's approach, active mining and partial-survey target
checks join to those copied IDs. Native staged SPKN validation joins its decoded
field, rather than treating that field's live count as its highest ID.

General copied-field validation now permits future lifecycle states, so the old
ASTR reader explicitly preserves its original dense ID order and middle-strip
position contract. It derives initial issuance from the validated old count.
SPKN's old transport ID bound also remains separate from modern membership.
Group 22 must version-gate these reader restrictions when adding sparse wire
state; they cannot simply disappear for every historical file. These migration
gates are in addition to the native install gate recorded under Group 14.

## Group 14B verification

The focused RelWithDebInfo build passed. `[asteroid-preview-identity]` passed
42 assertions in three cases, and the actual retained load/preview/waiting case
passed 209 assertions in 4.750 seconds. Its executable SHA256 was
`496242c1b387836a0b21ec4199e847767ebfa9832fb34d8cc3f0c0b381084d59`.
Evidence: `logs/space-fixes/build-asteroid-preview-identity.log`,
`logs/space-fixes/asteroid-preview-identity.log`,
`logs/space-fixes/asteroid-preview-legacy.log` and
`test-cache/space-fixes/asteroid-preview-legacy/report.json`.
Run `[asteroid-preview-identity]` for bounded/high IDs,
duplicate/order/class/issuance refusal, candidate-only knowledge, independence
from a different installed field and complete Clear. Logs belong under
`logs/space-fixes/`.

The focused native harness also supports `--asteroid-preview`. Reproduce the
retained-file check with
`python src/tests/multilayer/acceptance/space_fixes_checks.py --asteroid-preview --construction waiting --run asteroid-preview-legacy`.
It first loads the original retained factory, then runs the actual native
read-only preview decoder on that file. Field equality and vehicle, structure,
manifest, power, clock and both global RNG streams must remain unchanged before
the ordinary construction waiting observation. Inputs/reports stay under
`test-cache/space-fixes/<run>/`; the harness preserves its 110-second deadline.

After compaction while 14B is active, read Global Mandatory Reading, then the
new Detached asteroid identity joins block above 14B. Do not preload 14C's
custody/transition implementation. Its remaining direct-index audit is still
pending; old reflection and live high-ID cargo remain intentionally gated.

## Group 14C: remaining native consumers

The native source audit now finds direct `bodies[slot]` access only inside the
compact storage implementation and the explicit legacy install/ASTR reader
gates. Demo starters, preview movement, rendering, picking, inspector selection,
disclosure, survey completion and current packet custody use canonical ID
lookup. There is no native `bodies[source_id]` or `bodies[target_id]` fallback.

`ValidateSpaceTransitionRecords` accepts an explicit same-owner issuance cursor.
Its default checks the full 32-bit transport domain; native installation/save
checks pass the field cursor, and preview passes its candidate cursor. A valid
issued ID may remain as lost/returning flight history after retirement. This is
intent, not admission: the existing `ResolveDestination` and
`ResolveSpaceNavigationTarget` still require a live body before actual departure
or delivery. Their absent-target path remains `TargetRemoved`/`TargetLost`.
Native afterload installs ASTR before Space transitions, so the joined native
cursor is the loaded world's value.

The historical demo and ownerless preview setup preflight all three original
starter IDs/classes and physical home coordinates before publication. Preview
setup also refuses any native mining lease before changing its vector. Current
legacy asteroid packets still require their actual original live body and the
matching resource label. Groups 15-16 replace that retired-source restriction;
this group changes lookup without inventing historical provenance.

Retained restrictions and test classifications:

- `saveload/cargo_source_sl.h` deliberately keeps the old 16-bit asteroid source
  and 14,336 bound until typed origins and schema-qualified CAPA migration in
  Groups 15/21. `surface_supply_custody.cpp`'s corresponding old-era rejection
  remains valid.
- ASTR's old dense order/band and SPKN's legacy ID bound remain reader gates for
  Group 22. Current native installation is still pristine and in-band.
- Literal initial vectors in `asteroid_motion.cpp`, `asteroid_catalogue.cpp`,
  `asteroid_generation_native.cpp` and declared `probe_extraction.cpp` fixtures
  describe initial or explicitly selected original bodies. They do not prove
  lifetime dense indexing.
- `asteroid_depletion.cpp`'s permanent-body assumptions,
  `probe_interception_native.cpp`'s reflection scenario, and long-running loops
  in `space_preview.cpp`/`space_mining.cpp` that join old snapshots by ID-as-slot
  need the Group 29/31 lifecycle expectations. They are not run as acceptance
  for the new straight/retirement behavior while that behavior is disconnected.
- The index's `MAX_ORBITAL_BODIES` uses now bound live membership/capacity and
  bounded row allocation. They do not restrict lifetime identity issuance.

## Group 14C verification

The focused RelWithDebInfo build passed. `[space-identity-consumers]` passed
335 assertions in three cases in less than one second. The actual retained
load/preview/waiting case passed 209 assertions in 4.328 seconds. The executable
SHA256 was `13d3a8a1c5ff5bcd68734113cbbf77747bfaa138a7b6a27d923c1f0e6e9c14e1`.
Evidence: `logs/space-fixes/build-space-identity-consumers.log`,
`logs/space-fixes/space-identity-consumers.log`,
`logs/space-fixes/identity-consumers-legacy.log` and
`test-cache/space-fixes/identity-consumers-legacy/report.json`.
Run `[space-identity-consumers]` for high/exhausted
issued flight intents, absent live navigation, original starter preview setup,
actual native two-unit extraction and recalled cargo custody, wrong-resource
refusal and the retained old source gate. Native unit participants make no paid
procurement or new-format high-ID cargo claim.

Then run one retained-world observation through `space_fixes_checks.py`, using
`--asteroid-preview --construction waiting --run identity-consumers-legacy`.
Its artifacts belong under `test-cache/space-fixes/identity-consumers-legacy/`
and `logs/space-fixes/`; source saves remain immutable.
