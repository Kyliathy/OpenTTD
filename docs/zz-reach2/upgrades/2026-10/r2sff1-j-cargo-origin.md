# Full-width asteroid cargo provenance

Owner: [Space fixes plan](r2sff1-space-fixes-features-1.md), Groups 15-16 and the
matching Group 21 transport gate.
Native ownership: [runtime handoff](../../../zz-rhr/upgrades/2026-10/r2mlsp-z-z-p-probe-runtime-handoff.md#delivered-ownership).

## Group 15: contained source value

`SourceID` remains 16-bit for Industry, Town, Headquarters and CompanySupply.
Their existing type ordinals and non-paying company-supply purpose remain
unchanged. `Source` now defaults to canonical unknown ordinary provenance and
contains a private optional `AsteroidCargoOrigin`: full 32-bit body ID and stable
`CargoLabel`. No heap allocation, live-body lookup or second inventory supplies
that provenance.

`Source::FromAsteroid` constructs the modern tagged value with `id` normalized
to `Source::Invalid`. The invalid body sentinel or an empty label returns invalid
Asteroid provenance. Field-issued identity and recognized-mineral semantics
belong to Group 16's predicate, not this structural factory.
`GetAsteroidOrigin` returns a value copy, so calling it on the temporary returned
by `CargoPacket::GetSource` cannot create a dangling pointer.

`IsCanonical` rejects a modern value with a second scalar identity or a payload
under an ordinary tag. `IsValid` accounts for either canonical modern provenance
or an existing legacy/native scalar. Legacy constructors remain explicit old
16-bit transport; the typed-ID constructor refuses wide asteroid narrowing.
Generic `SetIndex` leaves all Asteroid values unchanged and refuses malformed
representations. Ordinary typed conversions/setters keep their established
domains. Explicit `MakeInvalid` clears identity/payload but retains purpose;
**asteroid retirement must never call it**, because original cargo remains
historically attributable after the live body is erased.

Default Source comparison now includes the whole optional origin. The existing
packet split constructor already copies Source by value, and both actual
VehicleCargoList and StationCargoList merge predicates already compare complete
Source values. Tests exercise those original paths instead of adding another
copy or merge policy.

## Extraction and current wire gate

`PrepareAsteroidMiningSource` constructs exact full-width ID/mineral provenance
from the original body definition. `GetSpaceMiningProductionTile` validates
physical coordinates and actual Space-layer presence before rounding or
`TileXYL`. Invalid coordinates cannot wrap into another layer after packet
allocation or reserve debit.

`CommitSpaceMiningExtraction` uses those preflight results but still emits the
old Source value. It explicitly refuses IDs at/above the old 14,336 transport
bound until Group 21 changes the actual packet constructor and reader together.
This keeps the old explicit narrowing guarded and prevents unsavable output.
Packet allocation, hold append, exact reserve debit and work reset remain the
original native transaction.

`IsValidCargoPacketSource` rejects noncanonical tagged representations before
ordinary-domain handling. Its retained Asteroid bound also refuses every modern
typed source (`id == Invalid`) in the old wire era. Group 21 must add the full
schema-qualified native/CAPA preview representation and then switch live
extraction to the prepared source. Private payload access is intentional: the
decoder must reconstruct a validated Source through the factory rather than
writing arbitrary optional representation bytes.

## Verification

The focused RelWithDebInfo build passed after adding the missing native
`tile_map.h` include. `[asteroid-cargo-origin]` passed 358 assertions in three
cases in less than one second. The retained-world load/preview/waiting check
passed 209 assertions in 4.266 seconds, preserving the original owners.
The executable SHA256 was
`6e83678108f3852e4ce4fef6ad2b1003c4044832042c51cd9efa85467e785443`.
Evidence: `logs/space-fixes/build-asteroid-cargo-origin-fix.log`,
`logs/space-fixes/asteroid-cargo-origin.log`,
`logs/space-fixes/cargo-origin-legacy.log` and
`test-cache/space-fixes/cargo-origin-legacy/report.json`.
The initial missing-include diagnostic remains in
`logs/space-fixes/build-asteroid-cargo-origin.log`.

`src/tests/asteroid_cargo_origin.cpp` covers IDs
65,534/65,535/65,536 and the last valid 32-bit ID, invalid sentinel/empty labels,
same-low-bits and different-mineral distinctions, tag/scalar contradiction,
ordinary domains, non-paying purpose, actual native packet splitting and both
merge predicates. It also tests invalid-coordinate/no-debit refusal and the
legacy live-output gate against an actual attached unit participant. Ready
work in that transaction test is an explicit unit input; it is not a claim of
earned runtime work or paid commissioning.

Run `[asteroid-cargo-origin]` from the focused native executable. Record output
in `logs/space-fixes/asteroid-cargo-origin.log` and build output in
`logs/space-fixes/build-asteroid-cargo-origin.log`. A retained-world native
load/preview check may verify the widely used Source representation still
decodes old packet state without changing original owners.

After compaction during Group 15, reread Global Mandatory Reading and the Cargo
provenance after body retirement block. Relevant current code is `Source` and
its typed payload, the mining adapter/production-tile preflight, original packet
split/merge paths, and the canonical/old-era cargo-source gate. The mandatory
table's former direct `bodies[source.id]` description is stale after Group 14C:
current custody uses canonical lookup but still requires a live legacy source
until Group 16. No modern extraction, retired-source acceptance or new CAPA
serialization is active yet.

## Group 16 implementation in progress

`asteroid_cargo.h/.cpp` supplies live-field and copied-identity overloads of
`ValidateAsteroidCargoOrigin` and `ResolveAsteroidCargoOrigin`. Modern origins
require an issued ID and known immutable mineral; a surviving body must match
its class. An absent issued body needs no live record. Legacy Source values
still require a surviving same-owner body to reconstruct their missing mineral;
missing old provenance is never guessed from another world.

The native all-container validator now accepts an explicit prevalidated field
metadata argument and uses the full origin predicate. This permits controlled
retirement tests with actual packet containers and an erased copied field,
without bypassing the current native installation gate. Active target/lease
validation remains separate. The audit additionally includes manufacturing
reservations alongside module stock and service inputs.

Task 16.3 discovered missing copied evidence. Ordinary station goods and vehicle
readers now capture each original reference's cargo slot in
`NativeCargoReferencePreview`; Clear removes that evidence between candidates.
CAPA preview copies its existing `source_xy` and first-station `source` fields.
No new wire format has been activated. Candidate validation uses the same file's
CDIM labels or immutable fixed recipe/supply labels, never current CargoSpec.
Modern typed origins cannot use missing label evidence. Genuine pre-CDIM legacy
files retain their former native-full-load-only ordinal check.

`asteroid_cargo_custody.cpp` now earns an actual two-unit packet with ordinary
mining ticks, recalls the probe, and removes its source from validated copied
field metadata. The legacy packet first fails without its surviving source.
`CargoPacket::UpgradeLegacyAsteroidSource` then qualifies that exact packet in
place using the mineral captured before removal. This helper is restricted to
explicit migration/unit callers; it does not activate new live output or saves.
The fixture moves the same packet through probe, module stock, descent pod,
Surface handling, station goods and ordinary road freight using native transfer
functions. These declared containers prove custody, not paid procurement or
elapsed descent. Each boundary compares ID, quantity, full source, production
tile, first station, feeder share, money and unchanged installed field. An
incompatible carrier cargo and deliberate duplicate raw-list reference refuse.

`asteroid_cargo_validation.cpp` covers issued high IDs, retired and surviving
origins, wrong/unknown minerals, future/sentinel IDs, missing legacy bodies,
candidate cargo-slot meanings, production plane/first pickup, duplicate/orphan
packets, missing/stale metadata and preview Clear. It allocates no native packet.
The native retained preview observer additionally compares CAPA metadata to the
same already-loaded original packets after preview independently succeeds.

The final whole-file preview join now also runs when natural freight remains
but no Space hull, structure or descent job survives. Ordinary carriers cannot
lose the historical-origin check by outliving all Space assets. Independent
family validators retain ordinary recipe/role admission; the asteroid predicate
returns immediately when no natural packets exist.

## Semantic consumer inventory

`cargo_source_digest.h::AppendCargoSourceDigest` preserves the original type/ID
scalar stream for every ordinary and legacy source. Only modern provenance
appends the explicit `ASTORIG1` marker, full ID and four mineral-label bytes.
Each existing digest retains its own fixed integer byte order. The vehicle,
manifest and structure digests, plus manufacturing clone/replacement observation
fingerprints, use it. The actual cargo-return observer compares complete Source
values. Focused assertions distinguish same-low-16-bit origins and different
minerals while retaining exact old input streams.

Remaining scalar uses are deliberate:

- `asteroid_cargo.cpp` reconstructs only explicitly legacy sources from a
  surviving same-owner body; `CargoPacket::UpgradeLegacyAsteroidSource` copies
  that admitted legacy ID without narrowing.
- CAPA native/copied descriptors and `cargo_source_sl.h` retain old transport
  until Group 21; mining output and its two extraction tests keep the same gate.
- `cargo_source_digest.h` emits the old prefix before the modern extension.
- `station_cmd.cpp` reads Industry-only production; subsidy command transport
  supports Town/Industry and rejects Asteroid in `CmdCreateSubsidy`.
- `hub_circulation_native.cpp` observes ordinary recovered Goods, while Surface
  entry tests identify Industry output. Their scalar source assertions and
  immutable fixture bytes remain unchanged. Geothermal and utility producer IDs
  are separate domains, unrelated to CargoPacket provenance.
- Purpose-only checks in company removal, production and Surface handling need
  only the source tag; they do not resolve or truncate an asteroid ID.

## Group 16 verification

The focused RelWithDebInfo build passed. The first custody run exposed missing
native `CanAllocateItem` preflights in the declared test-container setup; those
were added, and the affected test rebuilt. The final focused burst
`[asteroid-cargo-validation],[asteroid-cargo-custody],[heavy-joined-validation]`
passed **511 assertions in seven cases**, under one second.
Evidence: `logs/space-fixes/asteroid-cargo-custody-final.log` and
`logs/space-fixes/build-asteroid-cargo-custody-pool.log`. The initial diagnostic
remains in `logs/space-fixes/asteroid-cargo-custody.log`.

The real retained `space-factory-440.sav` load/preview/waiting run passed **270
assertions in 4.297 seconds**, including original CAPA metadata and ordinary
reference counts. The executable SHA256 was
`5b8d64d72c0eefa3c39b1f0aedd8894f134917561a6978f812618026172fc373`.
Evidence: `logs/space-fixes/cargo-custody-legacy.log` and
`test-cache/space-fixes/cargo-custody-legacy/report.json`.
Rebuild with the focused target and repeat
`space_fixes_checks.py --asteroid-preview --construction waiting --run <unique-name>`.
No full regression or paid financial trial was run.
