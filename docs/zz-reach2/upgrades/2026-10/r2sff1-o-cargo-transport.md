# Typed asteroid cargo transport: execution notes

Owner: [Space fixes plan](r2sff1-space-fixes-features-1.md), Group 21.
Prerequisites: [cargo origins/custody](r2sff1-j-cargo-origin.md) and
[native continuation](r2sff1-n-mining-continuation.md).

## Delivered work

Group 21 is complete. The common version-header rebuild and final focused build
passed; final build log is `logs/space-fixes/build-asteroid-cargo-final.log`.

## Original transport and migration

Save version **441 / AsteroidCargoOrigins** extends original CAPA with explicit
U32 `asteroid_body` and U32 `asteroid_mineral`. `source_id` stays U16 for ordinary
sources and old decoding. Modern asteroid sources require canonical invalid
`source_id`, a non-sentinel full body ID and a known immutable mineral label.
Ordinary sources require absent body UINT32_MAX and mineral0. CompanySupply
retains its original non-paying purpose and zero-credit requirement.

`CargoSourceAsteroidWire` in `saveload/cargo_source_sl.h` is per-row transport
adaptation, not a second native packet source. `cargopacket_sl.cpp` binds its two
scalars through native `SLE_GLOBAL_ADDRESS`, resetting before every full/copied
row. The native optional payload itself is never serialized as memory. The
codec shifts four label bytes into portable significance; native SlObject owns
U32 byte writing. `DecodeCargoPacketSource` and
`ValidateCargoPacketSourceSchema` are shared by native and preview admission.
Mandatory original keys are unique; typed keys are required in441 and forbidden
in genuine older tables, including unknown/skipped-key handling.

Full load calls `NormalizeLegacyAsteroidCargoPackets` immediately after original
ASTR acceptance and before manifest/fleet/custody joins. It preflights all native
packet origins against this loaded field before changing any original packet
in place. Missing legacy bodies refuse; no cargo ordinal, open-world body or
regenerated field supplies guessed mineral meaning.

Preview calls `NormalizeLegacyAsteroidCargoPreview` after existing same-file
body/cargo checks. Its all-row preflight and changes touch detached metadata
only. The nonserialized `legacy_asteroid_origin` receipt preserves legitimate
pre-CDIM meaning checks after qualification; modern decoders cannot acquire
that receipt. Candidate identity validation runs once before per-packet binary
lookups. `LoadCheckData` clears receipts by clearing its original row vector.

`CommitSpaceMiningExtraction` now supplies `PrepareAsteroidMiningSource` directly
to the original native packet constructor, with no16-bit truncation. It also
requires `GetMutableAsteroidBody(body.id) == &body`: a valid-looking copy or
unregistered wide ID owns no native reserve. This missing original-owner check
was exposed when the deliberately obsolete width gate was removed.

## Verified results

Actual immutable `space-mining-holds-405.sav` loaded 18 natural packets plus one
ordinary packet, qualified the same original identities and passed native save,
preview and modern441 reload. Source SHA256:
`69cd64ada1b0dd3f5ee84ffee5a7859148ae5459185860ffbbc1f353aed653ad`.
Logs: `cargo-origin-legacy-holds.log` and `cargo-origin-modern-holds.log` under
`logs/space-fixes/`; their runtime/resaves are under matching
`test-cache/space-fixes/` directories. Each run passed585 assertions in roughly
1.8 seconds. Current valid matrix input:
`test-cache/space-fixes/cargo-origin-modern-holds/cargo-origins-441.sav`.

`asteroid_cargo_wire_checks.py` prepares one named invocation-owned malformed
CAPA copy and runs independent native preview/full-refusal processes. It keeps
the original file version, other chunks and source files intact. Cases cover
missing body/mineral, dual scalar/payload identity, sentinel/future ID, empty or
wrong-class mineral, an ordinary paying tag with natural payload, genuine-old
payload forgery and truncation. **All ten cases passed through both native
preview and full-load rejection**, in two focused bursts of five cases. Each
native process completed in roughly 1.8 seconds. Logs and invocation metadata
use `cargo-wire-final-<case>-preview` / `-full` under `logs/space-fixes/` and
`test-cache/space-fixes/`; malformed copies remain under
`test-cache/space-fixes/cargo-wire-cases/`. The wrapper logs are
`logs/space-fixes/cargo-wire-final-<case>.log`.

Final source/custody/continuation/Surface-return tags passed **24,842 assertions
in22 cases**, under one second, in
`logs/space-fixes/asteroid-cargo-transport-final.log`.
The final executable SHA256 is
`349372986712f07e396da5ec2328ee1de8e694abc3e1e7047e68d4a5e797841b`.
The final retained-source run `cargo-origin-final-legacy` also passed585
assertions; log `logs/space-fixes/cargo-origin-final-legacy.log`.

The independent `asteroid_cargo_roundtrip_checks.py` compares complete original
CAPA rows and passed405->441 conservation of **19 native packet identities,
18 natural packets, one ordinary packet and82 total units**. All unchanged
source, age, travel, production, station and virtual-credit fields compare
exactly; only explicit old natural-source qualification differs. Log:
`logs/space-fixes/cargo-origin-byte-roundtrip.log`. Rebuild command:

```powershell
python src/tests/multilayer/acceptance/asteroid_cargo_roundtrip_checks.py --source src/tests/fixtures/demos/space/space-mining-holds-405.sav --target test-cache/space-fixes/cargo-origin-modern-holds/cargo-origins-441.sav
```

The first focused build found a missing framework writer-version declaration,
then test-only include/cleanup API errors; subsequent builds passed. Broader
focused source tests exposed fixture isolation: bulk Vehicle cleanup skips
per-hull packet disposal. `ProbeExtractionRig`, `FactoryMiningRig` and derived
`SurfaceReturnRig` now empty their original carrier containers first. Modern
writer expectations replaced the obsolete Group15/16 pre-transport assertions;
legacy normalization itself has separate explicit codec/container coverage.

The derived Surface fixture cleanup passed its final focused build and tests.
Native observers are in `asteroid_cargo_native.h/.cpp`, invoked by
`space_fixes_checks.py --cargo-origin`. `--cargo-bad` selects only an existing
test-cache copy; `--cargo-full` expects ReInit in its disposable process and
exits without dereferencing torn-down owners. Each process keeps the existing
110second deadline. No separate financial trials were resumed.

## Next gates

High-ID source codecs and native packet copies are covered as components.
Actual sparse high-ID field installation/removal still waits for Groups22–24;
this group publishes no retired native field and claims no whole-save high-ID
runtime acceptance yet. Group22's next available version is442; Group23 must
also persist Group20's `route_cursor` and `travel_ticks` before command exposure.

After compaction, read Global Mandatory Reading and the closest **Versioned
field, packet and mission transport** block above Group21. For current cargo
closeout, focus on CAPA/source adapters, the two migration placements and native
observer/malformed fixture harness. AST/SPOR payload changes belong to the next
groups; do not re-read unrelated UI or financial plans.
