# Merged canonical state and complete-family contract

Related architecture: [native savegame](../../architecture/savegame/archi-savegame.md),
[map layers](../../architecture/map-layers/archi-map-layers.md) and [Plan3Group46](m2o23-3-come-space.md).
Status: all Group46 tasks verified; this is the scoped canonical contract for later qualification.

## Existing canonical owners

ComputeMultiLayerStateDigest remains the only explicit record/field diagnostic
encoder. ComputeLiveMultiLayerStateDigest extracts the real installed roots that
LogMultiLayerStateDiagnostics already assembled, adding the reserved SurfaceSupply,
GeothermalGenerator and TubeGeothermal roots. CargoPacketRoot is appended after
all earlier kind IDs. It reads every actual fragment's saved scalar fields,
including source/purpose, U64receipt and literal departure, without another stock
owner. ComputeSurfaceSupplyOrderDigest includes issuer, exact common receipt facts
and structurally distinct Native/LegacyBfields; no legacy payer or inflation is
invented. Existing UtilityRoot already covers conversion ratio/numerator/original
thermal contributions and physical feed references; TubeVehicleRoot covers both
provider IDs, carry and once-only ticks. Existing SpaceVehicle/SpaceStructure roots
retain orders/manufacturing/replacement/contained work/recipe retirement coverage.

These are diagnostics over live owners, not another runtime pool, persistence
schema or subset-based success threshold. Detached candidates cannot supply a
schema to borrow a running world's roots. Copied admission keeps its own facts.

## Explicit merged transport set

`src/tests/fixtures/saveload/merged-family-set.json` freezes exact names/kinds from
the unchanged original A440and B434catalogue inputs plus Group7allocated MLLNand
new RIFFSSORdecisions. It is independent of the new writer/output. Original counts
121and126remain source provenance. The merged contract happens to contain128named
families; success requires the exact set and framing kinds, never a count alone.
The existing savegame_tables.py reader's merged_world_chunks enforces this fixture
and rejects duplicate, missing, unknown or differently framed owners, including
empty ones. It retains every raw owner, including GLOG/VIEW/SRND, native packets,
company accounts and every Space/underground extension. No whole owner is excluded.

## Permitted original-to-merged structural transformations

Every rule is selected by original IDs, original fields and the frozen native
catalogue/allocated format contract. No new actual save supplies expected values,
and no whole owner is excluded from comparison.

| Original evidence | Permitted new representation | Facts that cannot change |
| --- | --- | --- |
| Original A/Bheader and exact accepted families | New441envelope, sole MLN1/schema1marker and complete original-derived current transport set; absent historical extensions get only their explicitly specified empty/default representation. | Original file bytes/header/hash; native world quantities, receipts and references. |
| Original A440fixed SSORor BSSOH/SSORTABLE | Exact canonical Native1or LegacyB2tagged SSM1receipt fields and original monotonic issuer; remove old Bheader only from the new successor. | Every real common/era-specific scalar, dates, tariff, dispatch/refund counters, payer/buyer availability, cost/inflation and issuance history. No fabricated BNative facts. |
| Original B U32CAPAlink and raw0/1departure; invalid-Industry/zero-credit linked supply purpose | Lossless U64/U8transport and the existing checked non-paying CompanySupply purpose with unavailable original buyer; source-era custody validation precedes normalization. A439/440unlinked permanent purpose stays unlinked. | Actual packet IDs, quantities, age, transfer credit, queue/vehicle/Space custody, finite entitlement and irreversible dispatch. |
| Original B427/428GPGschema1without operating columns, older external-only TUVDand absent newer families | Current complete schema with only documented original absence sentinels/empty histories. UCVjoins the same UtilityPowerState. | Actual industry/site/company/output/input identities, material/work receipts, service fractions, conversion numerator/provenance and source/provider history. No free machinery or grants. |
| Original content2paid Space receipts | Installed current SDEF3and explicit current CDIM/required catalogue; receipts keep their exact original content revision and provenance through the existing native adapter. | Original hull/module identities, reservations, paid work, products/replacement/clone payloads, contained jobs, quantities and retirement. |
| Original native EIDSand exact first-party capsule catalogue | Preserve all original mappings; append only missing canonical TUBE_ENGINE_LOCAL_BASE+model slots through the existing definition owner. Exact introduction years come from the frozen model contract. Saved history is restored first, then the existing canonical publication owner also visits appended definitions. | Initialized original reliability, user-hidden preference, unrelated engine history, actual vehicles, cargo, money and RNG. No new runtime catalogue. |
| Original saved rail-label slots and canonical capsule front/physical consist | Existing ConvertRailTypes remaps actual native labels; CCF_SAVELOADrebuilds those capsule caches afterward, deriving power/weight/acceleration from the same native consist/definitions. | Original body/engine/owner, saved work/carry/phase, packet stock and utility/cash. No train movement or powered opportunity occurs during this derived-cache rebuild. |

The all-family audit exposed two first-successor initialization gaps: appended
capsule definitions missed the publication pass because CopyTempEngineData stopped
at the original history frontier; capsule acceleration cached before rail-label
conversion could remain1until the next reload recalculated200. The existing owners
now finish both operations on the first load. These are runtime fixes derived from
native catalogue/physical rules, not allowances to discard changed ENGNor VEHS.

**Every first441successor?full reload?second441save now compares all128families
byte-for-byte with zero field normalizations and zero owner omissions.** Original
migration retains the precise structural/semantic transformations above. The
historical121/126counts never define merged success.

## Focused proof for46.1-46.2

Fourteen focused cases/2473assertions pass on2026-10-24: four pure tagged-wire/
digest cases(518assertions, including29one-field mutations) and ten native
roundtrips(1955assertions). Existing utility conversion/provider and Space
manufacturing/recipe diagnostic coverage remains present. Machinery observers
also compare the complete live canonical aggregate across actual reconstruction.

Rebuild both existing Debug Tube/Surface targets using the same bounded CMake
parameters as44. The successful log is
`test-cache/merge-supply/group46-canonical-roots-build-2.log`; the first build's
missing explicit native-observer include is fixed. Run Tube filter
`[surface-supply-merged-digest],[surface-supply-merged-wire]`; transcript
`group46-tagged-digests.txt`. Run each native producer with`--canonical`:

- machinery_lineage_native_checks.py`--epoch N --roundtrip --canonical` for
  N427,428,432,433; then`--binding-phase 0 --roundtrip --canonical`.
- space_lineage_native_checks.py`--epoch donor-space426 --roundtrip --canonical`
  and`--epoch 438 --roundtrip --canonical`.
- merge_supply_native_checks.py`--origin B --canonical`, then
  `--origin A --a-fixture paid --canonical` and`--origin A --a-fixture states --canonical`.

Each child remains under the existing110-second guard. Fresh Group46prefixes
retain exact original hashes/current executable identity/new successors/second
saves and all128compared names. The summary is
`test-cache/merge-supply/group46-canonical-verification.json`, explicitly scoped
to46.1-46.2; it does not claim first-tick/loss completion. No retained original,
media, source epoch or protected Space seal changed. No full regression was run.


## First-tick qualification (46.3)

New permanent`src/tests/save_first_tick_fixture.h/.cpp` explicitly reloads the unchanged
original, saves a new441before-state, advances one actual native tick, reloads the
before-state and runs that real native tick again. Both branches must advance the
native clock by exactly1. It compares simulation RNG, real financial arrays,
actual packet fragments, tagged book, native domain diagnostics and the complete
canonical aggregate. The Python producer then compares all128saved owners exactly,
including VIEW/GLOG/SRND. No original-to-new normalization is applied to tick output.
Only isolated component worlds are used; protected Space continuation seals/windows
are not opened. Default native observation still performs zero work.

Existing machinery/Space/supply producers now accept`--first-tick` with their
roundtrip/canonical options, use new Group46`-first-tick`output prefixes and bind
before/direct/reloaded save identities in a dedicated receipt. The existing
savegame_tables.py owner supplies first_tick_worlds rather than another parser.
Machinery`--input-case` selects these exact immutable manifest aliases:

- fractional: donor/openttd3-75ea75718f/mlm/reviews/geothermal/atomic-conversion/20261016-222730-cc33c0ee32/saves/heat-conversion-fractional.sav.
  This genuine B428original has one town and numerator15. Keep the default catalogue
  townless428case's ordinary NoTownrefusal unchanged; this separate original permits
  actual Normal-mode simulation without fabricating a town.
- face-held: donor/openttd3-75ea75718f/mlm/reviews/demo-fixes/face-loop/20261016-040520-e7e13ab341/saves/face-held.sav (B425).
- face-bay: donor/openttd3-75ea75718f/mlm/reviews/demo-fixes/face-loop/20261016-040523-9ad0b5a791/saves/face-bay-return.sav (B425).
- queued-provider: donor/openttd3-75ea75718f/mlm/reviews/tubes/queue-files-read/20261018-014451-b3cda2ed9e/saves/tube-queue-7-0-inspect-inspect.sav.
  The two actual active original fronts both wait on Grant(3), preserving queued
  request/provider state through the first resumed opportunity.

The existing genuine B426supply original retains partial dispatch history4of10,
with6already refunded and the exhausted receipt closed; no first tick may recreate
its stock/refund entitlement. A427manufacturing and A438processing/retirement cases
provide the Space side beside those underground states. Planned focused invocations
use each component separately, under the same110-second child guard.

Three native first-tick cases pass on the first qualified executable: fractional
(239assertions), face-held(158) and face-bay(158). Every pair includes exact128-family
comparison. Standalone header includes were fixed; MSVCthen reported LNK1163COMDAT
in the duplicated inline observer. Its body now lives once per binary in
save_first_tick_fixture.cpp, declared by the small header and listed in existing
focused/ordinary CMake owners. Successful build is`group46-first-tick-build-4.log`.
No file deletion or compiler workaround was needed.

The queued original reached an AnimateTile assertion before vehicle scheduling.
It is NOT an invalid original: inspection of the pinned donor shows its Railway
TileTypeProcs has AnimateTile_Rail, delegating to AnimateTubeFacilityArt; A lacked
that imported callback. All three original ANITentries name actual tube facilities.
An intermediate commentary suspected invalid references before checking the donor;
that diagnosis was corrected. Do not clear ANIT, weaken native checks or demote the
original to a malformed prototype.

The minimal donor animation lifecycle is now imported: new
transport_tube_facility_sprite.h/.cpp exposes the existing display registration/
eight-tick refresh; GetBuiltTubeTerminal factors retained physical geometry from
today's construction admission in the existing readiness owner; the Railway tile
handler delegates display animation and bounded tile visits recover registration.
No sprite range, pixels, power quantity, invoice, RNG or movement owner is imported
or replaced. Full packed drawing/remapping stays51and must reuse this same owner.
The supporting build`group46-tube-animation-build.log` passed; four focused native
readiness/platform cases pass947assertions. The later observer-only bounded wait
build is`group46-first-tick-wait-build.log`. A paid original legitimately pauses
callbacks for an asynchronous link-graph job. The observer allows up to10seconds,
yielding1ms between paused callbacks, and counts only one actual clock increment;
it never bypasses the native pause owner.

Final first-tick proof passes ten cases/2211assertions: fractional241, face-held160,
face-bay160, queued-provider424, Space427436, Space438215, donor Space42680,
B426partial dispatch133, A440paid199and A440states163. All ten paused roundtrips
and direct/reloaded first-tick pairs compare exactly128families with zero field
normalizations. `test-cache/merge-supply/group46-first-tick-verification.json`
binds current executable, original, observer, contract and individual reports.
Rebuild with the three existing native producers'`--roundtrip --canonical
--first-tick`options and the exact machinery`--input-case`aliases above. Files
under`test-cache/merge-supply`are disposable outputs, not expected baselines.

## Original identity loss and reuse (46.4)

Existing native boundary tests already exercise the required destructive world
events without filesystem deletion. The focused supply selection comprises the
actual berth-removal/station-deletion/reused-slot case and actual company takeover
and loss cases: three cases/133assertions. Retired legacy source/company references
stay invalid, no refund moves to a new occupant, and ownerless packets retain their
non-paying purpose.

The focused machinery selection passes twelve cases/2908assertions using tags
`[geothermal-generator-reuse]`, `[geothermal-generator-retired-stage]`,
`[geothermal-generator-reference]`, `[geothermal-generator-operating-removal]`,
`[geothermal-generator-operating-company]`, `[tube-geothermal-source-lifecycle]`,
`[tube-geothermal-binding-retirement]`, plus named Tube retired-source recovery
and actual portal-loss cases. Actual industry slot reuse starts with a fresh site,
empty bill and zero work; stale staged machinery refuses. Rebuilt inlets and sold/
replaced capsules receive fresh provider/consumer/local-feed identities. Original
generation, fractional debt and consumed service remain historical; unrelated
siblings retain their own rights. `group46-supply-loss.txt`,
`group46-machinery-loss.txt`and`group46-loss-verification.json`under the same
test-cache directory bind the two executables and focused results. No new duplicate
test or full regression was required.
