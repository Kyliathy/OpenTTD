# M2O23-1 save-lineage evidence and classifier contract

Owner: [merge plan, Groups 4-5](m2o23-1-come-space.md). Architecture: [native save transport](../../architecture/savegame/archi-savegame.md). This is an implementation handoff, not an additional runtime schema owner.

## Evidence boundary

Observed 2026-10-23 from the exact retained originals established by Groups 1-2. [lineage-originals.json](../../../../src/tests/fixtures/saveload/lineage-originals.json) records 27 representative source identities, their original chunk framing/table columns and six negative originals with missing required families. The broader disposable sample contains 81 framing observations and eight explicit decompressor refusals. Full semantic/native load acceptance remains Groups 38-47 and 63; a successfully framed negative test save is not a valid gameplay world.

Rebuild the broader audit with `python -B src/tests/multilayer/save_lineage_inventory.py --limit 3`; output is `test-cache/merge-inventory/save-wire-facts.json`. The observer imports the existing `savegame_tables` and `savegame_plane_digest` transport readers. It never changes bytes, allocates game pools or infers a runtime lineage from a path. Origin labels below identify where preserved bytes were observed, not evidence the classifier may consume.

No version439 original was found in A's retained input catalogue. Its source-defined capability is still known, but no native compatibility result may be claimed for an invented or header-downgraded fixture. This gap does not block detached capability tests; later native qualification must disclose it.

## Required families and metadata

For this collision interval, **C** denotes the common lineage-relevant boundary: `MLMD` TABLE with exactly one schema1 row; `SDEF` RIFF with ten bytes; `SPST` RIFF with the original retirement header (42 bytes before any structure rows); and `CAPA` TABLE with its complete original named header. Other ordinary required native families remain owned by their existing readers. The classifier is not a second implementation of every gameplay validator.

A's CAPA columns are `source:U16`, `source_xy:U32`, `loaded_at_xy:U16`, `count:U16`, `periods_in_transit:U16`, `feeder_share:I64`, `source_type:U8`, `source_id:U16`, `travelled.x:I16` and `travelled.y:I16`. B appends `company_supply_order:U32` and `company_supply_shipped:Bool` from426. Both columns must be present with their actual types even when the packet pool is empty; the raw table type codes observed are6 and1. Their values do not grant stock or refunds during classification.

B's added MLMD bits are `SurfaceSupplies = 0x00800000`, `GeothermalGenerators = 0x01000000`, `HeatConversions = 0x02000000` and `TubeGeothermal = 0x04000000`. A's original supported mask is `0x007fffff`; B's is `0x07ffffff`. A did not add a feature bit for its440 order family. A set B-only bit is positive B evidence, but cannot excuse a missing required B header/table. An unset bit never means that the version-required empty family may be omitted.

| Original version | A requirements beyond C | B requirements beyond C |
| --- | --- | --- |
| 424 | Shared pre-fork semantics; SDEF0 for Classic or exact content2 | Same shared semantics |
| 425 | Assembly activation semantics inside existing SPST; no new family | AwaitFaceTransfer semantics inside existing TBM data; no new family |
| 426 | Manufacturing reservations/next-order fields inside actual module records; no new family | Supply family **S**, CAPA supply columns; bit23 when issued history exists |
| 427 | Manufacturing work/product receipts in actual SPST jobs | S plus generator family **G**, schema1; bit24 when generator rows exist |
| 428 | Clone metadata inside actual manufacturing jobs | S + G(schema1) + conversion family **U**; bit25 when converted sources exist |
| 429 | Replacement settlement in existing SPST/SVST owners | S + G(schema2 operating fields) + U |
| 430 | Ordered service/freight semantics in existing SPOR/SPSU owners | S + G(schema2, Magma model admitted) + U |
| 431 | SDEF3 for selected multilayer content (Classic stays0); heavy commissioning | S + G2 + U; native town-service fields in existing utility/link owners; SDEF remains2/0 |
| 432 | Same framing families; heavy mission semantics | S + G2 + U; native local-feed fields; SDEF remains2/0 |
| 433 | Same framing families; heavy resupply/shell semantics | S + G2 + U + geothermal tube family **T**; bit26 when source history exists |
| 434 | Same framing families; physical descent sender/mass receipts | S + G2 + U + T; TUVD appends U32 `geothermal_source` and `local_feed` |
| 435 | Heavy onboard-service semantics; same C, SDEF3/0 | Outside the pinned B format range; never infer an unknown future B schema |
| 436 | Contained-service role appended in existing SPST job records | Outside pinned B range |
| 437 | `CDIM` RIFF: one-byte count plus six bytes per configured cargo row | Outside pinned B range |
| 438 | CDIM plus `SPRC` RIFF, including explicit empty processing history | Outside pinned B range |
| 439 | Same families as438; appended CompanySupply source meaning in existing CAPA, no B columns | Outside pinned B range |
| 440 | CDIM + SPRC + A `SSOR` RIFF: fixed14-byte header plus53 per order | Outside pinned B range |
| Future merged441 | Reserved proposal: explicit MLLN marker, all merged required families and one tagged canonical supply transport | Same unified contract; numeric reuse is forbidden |

**S** is `SSOH` RIFF of12 bytes (three network-order U32 values: schema1, row count, next order) plus `SSOR` TABLE with every original descriptor. The header requires `next_order == row_count + 1`. Family components are mandatory from426 even if empty. A440 `SSOR` instead has little-endian `next_id:U64, saved_day:I32, count:U16`, followed by exactly53 bytes per row; no schema word and no SSOH are present. Mixing these interpretations is corruption, not a fallback.

**G** is `GPGH` RIFF of8 bytes (schema/count) plus `GPGD` TABLE, mandatory from427. Schema is1 for427-428 and2 from429. The operating columns are `heat_consumer`, `electrical_producer`, `service_remainder`, `has_operating_tick`, `last_operating_tick` and `active_service_ticks`. Only430 and later can claim a Magma-Loop model; a header version does not prove its physical references.

**U** is `UCVH` RIFF of32 bytes (four network-order U64 values: schema1, source count, origin count, interval) plus `UCVD` TABLE, mandatory from428. The interval is neither0 nor UINT64_MAX; row cardinality must cover source and origin rows. **T** is `TGSH` RIFF of20 bytes (schema1, next source, sequence high/low, source count) plus `TGSD` TABLE, mandatory from433. Dense source issuance, sequence and row counts agree; empty history has zero sequence. These bounds are checked before any vector allocation.

The source authority for those requirements is `CheckSurfaceSupplyFamily`, `CheckGeothermalGeneratorFamily`, `JoinLoadedUtilityHeatConversions` and `CheckTubeGeothermalFamily` in B's corresponding saveload files, plus A's `ReadSurfaceSupplyOrderFamily`, `ValidateOrbitalRecipeLoadPreview`, `SDEFChunkHandler::Read` and `CDIMChunkHandler::Read`. Named implementation symbols take precedence over this summary; relocate renamed helpers instead of guessing numeric gates.

## Ambiguity and refusal rules

- Version425 does not identify a branch. B's retained mine-construction original below has empty SPST, content2 and no unique branch family; absence of an active Assembly or face-exchange owner cannot choose A or B.
- A426-430 can also carry only shared owner layouts when no new module job is present. The observed A430 probe-mission original has a42-byte empty SPST and content2. Merely noticing that B's added families are absent is insufficient positive A evidence.
- A verified complete branch-specific payload shape can supply positive evidence, but only through a bounded detached decoder. Group 5 may conservatively leave old common-layout files ambiguous; it must not pretend that a chunk length alone proves a variable record layout.
- SDEF content3 is positive A evidence from431 through440; content0 in Classic is neutral. Both branch flags and mandatory family presence must still be consistent. Unknown content values or contradictory A/B marker families refuse.
- B426's real missing-SSOH negative input still has SSOR TABLE and the two CAPA columns; it is corrupt B, never A. Likewise, missing GPGD/UCVD/TGSD at their required versions cannot be repaired with empty defaults.
- Partial CAPA supply columns, wrong types, duplicate selected chunks, a header/table wire-kind mismatch, impossible fixed lengths/counts, premature future families and mixed A/B evidence are corrupt transport.
- Known A435-440 require complete applicable framing and metadata; a large numeric version alone is not enough. Unknown future versions are unsupported until their marker/schema contract is explicitly implemented.

An explicit origin hint is permitted only for otherwise ambiguous legacy transport. It carries the actual original 256-bit SHA256 and the chosen legacy dialect. The classifier compares all 32 digest bytes and refuses an all-zero/unbound digest. It cannot override contradictory evidence, incomplete mandatory families for that dialect, a malformed marker or an unsupported original version. Paths, filenames, current-world state, remembered previous loads and version rewriting are forbidden inputs.

## Detached classifier handoff

The public boundary belongs to the existing saveload owner and is re-exported by `saveload.h`: `SaveDialect`, `SaveWireFacts`, `ClassifySaveDialect`, `HasSaveCapability`. A focused subordinate header/source is acceptable to keep these pure types independent of gameplay pools; it must not become another registry. Reader integration in Groups38-44 supplies facts from the exact file and remains responsible for bounded parsing.

Facts must distinguish **absent**, **valid** and **malformed** evidence. A boolean “has supply” cannot represent a missing half-family or changed table schema. Framing completeness, schema versions, fixed lengths, required table fields, metadata bits, SDEF content revision and exact source digest are detached inputs. A result is shared-pre-fork, legacy A, legacy B, merged, ambiguous, corrupt or unsupported; only successful results supply capabilities.

For the merged writer, reserve version441 without changing `SaveLoadVersion::MaxVersion` or enabling current writes yet. Proposed `MLLN` is RIFF with8 bytes: network-order U32 magic `0x4d4c4e31` (“MLN1”), U16 marker schema1, U16 writer version441. The ordinary envelope must also name441. Group44 must recheck the still-unused allocation before activation and emit the complete marker and canonical tagged supply family together. A marker at a legacy version, a wrong length/magic/schema/version or a partially present merged family is corrupt; never reinterpret it as a legacy file.

The merged marker alone does not validate cargo, finance, physical ownership or every gameplay row. Classification is preflight only. Once ordinary full loading starts, existing OpenTTD initialization/error semantics remain; do not promise whole-world rollback. All legacy numeric versions stay unchanged. Feature lookup uses the selected dialect and the original version, while merged441 includes both delivered capability sets and no unknown future feature.

## Exact observed originals

Each link points to retained A storage; none depends on B's generated directories. Full raw table facts and additional negative examples are in the fixture record linked above.

| Provenance | Version | SHA256 | Retained bytes |
| --- | --- | --- | --- |
| A | 424 | `fcec2e47613ea6ba5a99924f2bfb247d1f3bc5ac218fedf15689573a82cf35ee` | [original](../../../../savegames/retained/mlm/reviews/space/asteroid-starter-native-legacy/20261014-200720-468755aa34/saves/asteroid-starter-before-424.sav) |
| A | 425 | `f9e2c5e1175168109847419c181e251e71901af6c6ab4e517bca1d2e14a9a4a8` | [original](../../../../savegames/retained/mlm/reviews/space/assembly-facility-ordinary-company-after/20261015-141242-b68ef79b9a/saves/ordinary-resave.sav) |
| A | 426 | `1d3cf4149ff52f1b76bc97f5c1115a7be53dd0dc14a1cb0522624159855c3c7d` | [original](../../../../savegames/retained/mlm/reviews/space/manufacturing-reservation-ordinary-company-16/20261015-162351-53b55eaa00/saves/ordinary-resave.sav) |
| A | 427 | `ef713f807e5041ba2ee47c737312944780207d8f7620798f7764ff6e48bbbe40` | [original](../../../../savegames/retained/mlm/reviews/space/manufacturing-materialization-ordinary-company-16/20261015-190240-022c3cdf2e/saves/ordinary-resave.sav) |
| A | 428 | `c3f8659383228144cb6b5329c824217ecfe147da35872ce25cad076f4ef4d502` | [original](../../../../savegames/retained/mlm/reviews/space/manufacturing-clone-verify-main-goods-15/20261015-222221-fb91a095e4/saves/clone-goods-15-428.sav) |
| A | 429 | `74994ff9e0ba73720000b634ff7dc80f78fc553386f680aa462cfc414e4d3854` | [original](../../../../savegames/retained/mlm/reviews/space/manufacturing-replacement-verify-lost-cancel-19/20261016-010239-fc1c635029/saves/replacement-cancel-19-429.sav) |
| A | 430 | `5b668b9dbe01929431f9f9034fa9b0227462f1b41e87d8d1c2fe0ffc49233e72` | [original](../../../../savegames/retained/mlm/reviews/space/probe-mission-local-legacy-unknown/20261016-040241-90e99f82bf/saves/probe-mission-legacy-unknown-430.sav) |
| A | 431 | `cd64061ad52c602c25b4d8114bb86ce1a16e4699ceee8cf1323a2db678d1dfa8` | [original](../../../../savegames/retained/mlm/reviews/space/heavy-commissioning-ordinary-upgrade/20261016-054904-a2c2cc07d9/saves/ordinary-resave.sav) |
| A | 432 | `bce2be8486dd7a140a99a94255420155a8e3d6e4762ef56f008095b330399e3c` | [original](../../../../savegames/retained/mlm/reviews/space/heavy-mining-runtime-approach/20261016-064959-63e47e8ffb/saves/heavy-mining-approach-432.sav) |
| A | 433 | `2945f2de41c2165388e834ced4d9644ac2398beb8023c742bec843ea448a9410` | [original](../../../../savegames/retained/mlm/reviews/space/heavy-supply-custody-delivered/20261016-082806-57d038cbb5/saves/heavy-supply-delivered-433.sav) |
| A | 434 | `9f21be9c3f053d087b17335f5a07701be9267da23717a12f81b36109f8638cd4` | [original](../../../../savegames/retained/mlm/reviews/space/heavy-descent-orphan-recovered/20261016-094415-ab3435595e/saves/heavy-descent-orphan-recovered-434.sav) |
| A | 435 | `80f4fa6fe343a90a650f781096b3bc4063a172242ae8ded4055de137747933af` | [original](../../../../savegames/retained/mlm/reviews/space/heavy-onboard-premature/20261016-110431-ff9afd5f3b/saves/heavy-onboard-premature-435.sav) |
| A | 436 | `b4729d67610d55585a0b8174741e231e540bad40201588a3059ba9a7ee60f542` | [original](../../../../savegames/retained/mlm/reviews/space/heavy-contained-orphan-cancelled/20261016-125139-5aabb10d5e/saves/heavy-contained-orphan-cancelled-436.sav) |
| A | 437 | `68960f0b7b037d80bfa5c1c125ca65299c1ccb91ce9156d675820c2201e75f7c` | [original](../../../../savegames/retained/mlm/reviews/space/heavy-joined-reload-handling/20261016-150517-e77fd25da8/saves/ordinary-resave.sav) |
| A | 438 | `11e0974298b992d63b897bd8b05d443abfe3f4a23647d4396125f267ff63991c` | [original](../../../../savegames/retained/mlm/reviews/space/orbital-recipe-export/20261016-203503-57d3a2a957/saves/orbital-recipe-retired.sav) |
| A | 440 | `738802171de8ca1c88b0427002940be2bdabfcc39788703c1ee9071c6a287efd` | [original](../../../../savegames/retained/mlm/reviews/space/surface-supply-delivery-drive/20261018-094020-c02ab5e0f3/saves/delivery-declared-input-440.sav) |
| B | 424 | `0f10d887bdd07140b614fbc16b83a926481ac5092a22ea4f5daa7c41e8f6a6d7` | [original](../../../../savegames/retained/openttd3-75ea75718f/0f10d887bdd07140-ground-connected.sav) |
| B | 425 | `c86f98f6dae1601d3bcaa70cfe874997133d7e92195ce59617bc33e973e98f5c` | [original](../../../../savegames/retained/openttd3-75ea75718f/c86f98f6dae1601d-mine-construction.sav) |
| B | 426 | `f3f56b9ea05199d3fe20cdc29a66c33ef5ffe5abf8f2dbae2f0b0538d75dca14` | [original](../../../../savegames/retained/openttd3-75ea75718f/f3f56b9ea05199d3-paid-surface-inspect.sav) |
| B | 427 | `34fafcc762285a0ac159ef8799982c0e345d7032f7eb8d201b2a68b91b1be3ff` | [original](../../../../savegames/retained/openttd3-75ea75718f/34fafcc762285a0a-generator-complete.sav) |
| B | 428 | `10da09362a560688ae3652fd35c84e319b066faacb31110c193e143f285840c9` | [original](../../../../savegames/retained/openttd3-75ea75718f/10da09362a560688-heat-conversion-pending.sav) |
| B | 429 | `ace87b2293c5107aa8c42e73107eea0ff4dd59d3012d8d47ef9718b5ab701586` | [original](../../../../savegames/retained/openttd3-75ea75718f/ace87b2293c5107a-powerhouse-0.sav) |
| B | 430 | `5f11bfa48c2775a2708d48d1cb64cbf9034919d9ad0f10374b91edba8751b212` | [original](../../../../savegames/retained/openttd3-75ea75718f/5f11bfa48c2775a2-generator-shortage.sav) |
| B | 431 | `762a54e12a230b6cc3ed168f1e70ef9b9c60978ceaf9c2205a666e499beeff77` | [original](../../../../savegames/retained/openttd3-75ea75718f/762a54e12a230b6c-powerhouse-0.sav) |
| B | 432 | `eda533cd453f4372ba44afbc570e074f6940e5e36fbfc9ecafb01849250474a8` | [original](../../../../savegames/retained/openttd3-75ea75718f/eda533cd453f4372-local-feed-0.sav) |
| B | 433 | `30f51d5bed54b9d497f1ecf6428ba5d4b1fef6babfa394ef9a740c4eaa11a7a7` | [original](../../../../savegames/retained/openttd3-75ea75718f/30f51d5bed54b9d4-geothermal-source-3.sav) |
| B | 434 | `38b235cd30129eddb506dffa97522625867fe316780db90805ccea7b0312b087` | [original](../../../../savegames/retained/openttd3-75ea75718f/38b235cd30129edd-collector-purchased.sav) |

## Source boundaries consumed

| Source | Named contract | Purpose |
| --- | --- | --- |
| [A version enum](../../../../src/saveload/saveload.h) | SaveLoadVersion425-440 | Original Space/supply capability meanings |
| [B version enum](../../../../../OpenTTD3/src/saveload/saveload.h) | SaveLoadVersion425-434 | Original underground capability meanings |
| [A packet descriptors](../../../../src/saveload/cargopacket_sl.cpp) / [B packet descriptors](../../../../../OpenTTD3/src/saveload/cargopacket_sl.cpp) | GetCargoPacketDesc | Exact shared fields and B's appended two-column transport |
| [A supply family](../../../../src/saveload/surface_supply_order_sl.cpp) / [wire](../../../../src/surface_supply_order_wire.h) | SSORChunkHandler, SURFACE_SUPPLY_WIRE_HEADER_SIZE, SURFACE_SUPPLY_WIRE_RECORD_SIZE | A's exact RIFF owner and14/53-byte layout |
| [B supply family](../../../../../OpenTTD3/src/saveload/surface_supply_sl.cpp) | CheckSurfaceSupplyFamily, SSOHChunkHandler, SSORChunkHandler | B's complete empty-or-populated header/table family |
| [B generators](../../../../../OpenTTD3/src/saveload/geothermal_generator_sl.cpp) | GPGHChunkHandler::Read, GPGDChunkHandler |427/429 schema and operating-column boundary |
| [B conversion](../../../../../OpenTTD3/src/saveload/utility_conversion_sl.cpp) | UCVHChunkHandler::Read, UCVDChunkHandler |428 complete original-source/epoch transport |
| [B geothermal tube sources](../../../../../OpenTTD3/src/saveload/transport_tube_geothermal_sl.cpp) | CheckTubeGeothermalFamily, TGSHChunkHandler, TGSDChunkHandler |433 complete dense source history |
| [B capsule bindings](../../../../../OpenTTD3/src/saveload/transport_tube_vehicle_sl.cpp) | TubeVehicleWire, TUVDChunkHandler |434 pair of explicit provider/feed columns |
| [A Space structures](../../../../src/saveload/space_structure_sl.cpp) | SPSTChunkHandler::ReadRecords | Shared empty header versus version-sensitive owner rows |
| [A metadata/content/dimensions](../../../../src/saveload/multilayer_sl.cpp) | MLMDChunkHandler, SDEFChunkHandler, CDIMChunkHandler | Schema1, exact content0/2/3 and437 dimensions |
| [A processing](../../../../src/saveload/orbital_recipe_sl.cpp) | ValidateOrbitalRecipeLoadPreview, SPRCChunkHandler |438 processing family even when empty |

