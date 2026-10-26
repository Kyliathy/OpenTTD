# Sparse asteroid field transport: execution notes

Owner: [Space fixes plan](r2sff1-space-fixes-features-1.md), Group22.
Prerequisites: [renewal](r2sff1-h-renewal-ledger.md),
[identity](r2sff1-i-sparse-identity.md), [cargo](r2sff1-o-cargo-transport.md).

## Delivered contract

Current gate refinement: [Group24](r2sff1-r-transport-gate.md) tightens known
replacement cost and initial-credit admission. Earlier sparse artifacts below
are historical; use Group24's funded recipe for current acceptance checks.

Save version **442 / AsteroidFieldLifecycle** adds lifecycle revision1 while
retaining original generation provenance. Current active fields explicitly own
issuance and initialized renewal. Inactive ownership stays canonical empty.
Installation accepts validated sparse/empty current fields and physical-plane
positions; genuine legacy input retains its dense/pristine/band upgrade gate.

`saveload/asteroid_field_sl.h/.cpp` owns the shared full/preview byte codec.
The original51-byte header,40-byte native bodies,34-byte ownerless surveys and
26-byte starter history remain intact. A62-byte big-endian trailer saves:
lifecycle U16, next ID U32, initialized U8, three exit counters U64, credit U64,
last-exit presence/size/class U8 each, timer U32 and both U32 words of each RNG.
No host optional, RNG struct, lookup or spatial index memory is serialized.

The handler bounds complete byte allocation; the codec checks exact source-era
length/count arithmetic before allocating body/survey records. It rejects
trailing bytes, unknown revisions, noncanonical option absence, invalid flags,
duplicate/descending/future/sentinel IDs and inconsistent accounting. Saving
sorts by ID. Decoding validates a staged field and rebuilds its disposable
lookup; it publishes no gameplay, tick, RNG draw, retirement or generation.

Modern SPKN framing validates sorted unique full-width IDs and company masks,
then joins against the same file's actual live identity set. Genuine older
transport retains dense ID bounds. Empty current fields accept empty knowledge.

Legacy migration preserves bodies, signed motion, remainders, reserves, leases,
surveys and known starter bytes. It derives issuance from the dense count and
seeds absent streams once from the saved feature seed, granting no historical
exits/credit. Pre424 starter history remains explicitly unknown. The digest
includes every evolved ledger decision and all four RNG words, preserving the
pristine golden stream. Lifecycle1 is the constant current schema contract.

## Verified results

RelWithDebInfo build: `logs/space-fixes/build-asteroid-field-native.log`.
Executable SHA256:
`d72be8c43320b2a21fed2a9465be7386dc9f17e66f9338aca63e4942b2d7e2d0`.
Focused tags `[asteroid-field-wire],[asteroid-renewal],`
`[asteroid-live-validation],[asteroid-identity],[asteroid-preview-identity]`
passed **25,841 assertions in21 cases** under one second; log
`logs/space-fixes/asteroid-field-wire-first.log`. Component checkpoints cover
pristine, armed, due, failed attempt, successful debit, disarmed, changed latest
exit, sparse and empty ownership, including next timer/attempt and full RNG.

Actual immutable404 preview,405 mining,414 launch,440 factory and441 native
files passed full load, independent preview and native442 resave. Current442
reload passed. Logs under `logs/space-fixes/`: `field-preview-era-404.log`,
`field-legacy-405.log`, `field-launch-414.log`, `field-starter-440.log`,
`field-legacy-441.log`, `field-current-442.log`. Independent byte comparisons
verified original records, old zero accounting and current full trailer
preservation. The404 conversion adds only the absent invalid native mining
lease to each original36-byte body. The405 case preserves19 packets/82 units:
18 natural packets and one ordinary packet.

Fifteen malformed actual-version copies passed **both native preview and full
refusal**: truncation, oversized extent, impossible count, duplicate/sentinel/
future ID, missing cursor, unknown revision, unearned credit, inconsistent exit
counter, excessive timer, invalid optional flag, missing knowledge body,
duplicate knowledge and invalid mask. All30 runs passed, each under2 seconds.
Each preview performs valid-corrupt-valid and checks original field, packets,
fleet, structures, clock and all global RNG words. Full refusals use separate
disposable processes. Logs: `logs/space-fixes/field-wire-<case>-preview.log`
and `field-wire-<case>-full.log`.

Declared component-history fixtures passed native load, preview, resave and
another reload, retaining every original paid asset and packet:

- Body70000 and its SPKN receipt: `field-wire-sparse-fixed-accepted.log` and
  `field-sparse-reload.log`.
- Empty active field/knowledge based on414 launch without actionable body
  approaches: `field-empty-launch-accepted.log` and `field-empty-reload.log`.

Their complete lifecycle trailers survive byte-for-byte. These are transport
fixtures with declared aggregate history, not executed70,000 departures, new
procurement, earned survey or gameplay spawn evidence. An initial sparse
fixture correctly refused the wrong VehicleID sentinel (UINT32_MAX instead of
0xFFFFF). Empty440 factory correctly refused its retained real body approach.
The408 source retains ownerless surveys and was ineligible for this fixture;
Group26 owns their safe departure. Production validators were not weakened.

## Rebuilding evidence and next gate

Run the focused C++ tags from `build`, logging into `../logs/space-fixes/`.
Long-term tooling stays beside tests. Runtime files/reports stay under
`test-cache/space-fixes/`.

`src/tests/multilayer/acceptance/space_fixes_checks.py --source <repo save>
--cargo-origin --run <fresh-name>` performs native load/preview/resave. Its new
output basename is `cargo-origins-current.sav`, avoiding a stale version number.
`asteroid_field_wire_checks.py --source <current save> --case <case>
--run <fresh-name>` runs one refusal pair or declared sparse/empty case.
Its `--source <original> --target <native resave>` form independently compares
original records/trailers. Empty fixtures require no actionable body owners,
natural cargo or ownerless attachments.

Group23 must persist policy/continuation before command exposure. Group24 still
owns the joined gate in both configurations; actual departure, event commits
and normal-loop activation remain Groups25-29. No live exit writer is enabled.
