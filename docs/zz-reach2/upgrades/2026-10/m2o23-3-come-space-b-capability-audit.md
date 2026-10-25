# M2O23-3 - Original capability gate audit

Owner: [merge Plan3, Group40](m2o23-3-come-space.md). Architecture:
[native save transport](../../architecture/savegame/archi-savegame.md).
This is an audit/handoff, not another version registry. A/B original enums remain
unchanged. Runtime classifiers consume original bytes and optional exact-digest
hints, never this document or source filenames.

## Current implementation boundary

Updated after Groups41/41B/41C/42/43: native supply, complete typed B packet
custody, actual Space reconstruction and native donor machinery adapters are
implemented and verified. Group43adds one read-only GPGH/GPGD, UCVH/UCVD and
TGSH/TGSD handler per ID, explicit B utility/tube columns and final native
plant/feed/source joins. Its17focused cases/209640assertions include eight
B427-434native boundaries, five binding lifecycle states, genuine legacy external
capsules,23malformed full/copied refusals and explicit townless/prototype limits.
Merged writer publication remains44. Earlier table handoff assignments to41/43
are historical where the current Plan3checkboxes mark those tasks complete.

Group39 binds one accepted original result through parsing/afterload and clears
it on every exit. Group40 maps the original A425-440 enum names explicitly to
existing SaveCapability values in saveload.cpp DestinationVersionCapability.
IsSavegameVersionBefore/BeforeOrAt consult that mapping only during an accepted
attempt. Shared pre425/minor-version gates retain actual numeric semantics.
_sl_version is never renumbered. B's Space prefix remains before A's post-fork
features, even when its original number is higher than their introduction.

The native TABLE candidate predicate is **SlIsObjectValidInSavegame**, originally
static in saveload.cpp. It now checks both interval endpoints through the same
source-feature semantics, plus an optional exact required capability. SaveLoad's
WithCapability creates that explicit requirement without changing a field's wire
type/address or ordinary bounds. Unknown capability values refuse, including an
empty TABLE header. All native header lookup/sizing, object/row loading and writing
already call this one predicate; there is no second schema admission path.

**SlIsObjectCurrentlyValid is different:** it checks constant SAVEGAME_VERSION for
settings/current-writer descriptors. Its code remains unchanged, with clarified
JavaDoc. Replacing it with a candidate predicate would have changed unrelated
settings and still missed the actual native loader. Earlier speculative reading
that treated it as the candidate gate is stale; use the actual predicate above.
SlHasSaveCapability uses the accepted original during a load, otherwise the current
writer contract, never a previous candidate's lingering version. Native Null cleanup
clears original lineage first. Writer440 is still A; merged activation stays44.

## TBMP, CAPA and supply

| Owner | Original meaning and audited gate | Current result / adapter handoff |
| --- | --- | --- |
| A/B tbm_sl.cpp TBMP phase ceiling | Shared control ends at FollowOrders. B425 introduced AwaitFaceTransfer; A425 introduced orbital Assembly, not that phase. | Native max_phase now asks TBMFaceTransfer explicitly, retaining shared older phase/rate bounds. A425-440 cannot use B's phase merely because its number is high; merged will admit both through capability policy. |
| cargopacket_sl.cpp GetCargoPacketDesc | Existing ten common fields retain their pre-fork version/type gates. B426 appends company_supply_order:U32 and company_supply_shipped:Bool even when empty. | Common pre425 descriptors stay numeric. Group41 must attach SurfaceSupplyOrdersB to both B columns using WithCapability, not A's conflicting SurfaceSupplyOrders440 enum. They are not imported as unversioned fields in40. |
| cargo_source_sl.h IsValidCargoPacketSource | Explicit original source-version argument admits A's CompanySupply tag from439. B originals end434 and never wrote that tag; their paid packets used invalid-source Industry plus the separate order link. | The explicit439 comparison is nonoverlapping and remains valid. Group41 normalizes only after the complete typed B receipt/custody join; it must not admit a forged original B CompanySupply tag or infer a buyer from current company/station ownership. |
| surface_supply_order_sl.cpp | A440 fixed SSOR RIFF has its own admission/presence bounds; B426+ has SSOH plus SSOR TABLE. | Existing A gates route through SurfaceSupplyOrdersA via the central enum mapping. Group41 owns one merged dispatcher for the two legacy kinds, using existing detached converters/custody owners, never two live books. |
| afterload.cpp AcceptLoadedSurfaceSupplyOrders | A440's mandatory native book must not be required simply because another branch's number was read. | Its original A enum gate now means SurfaceSupplyOrdersA in a bound load. Canonical merged supply/tagged publication remains44. |
| map_sl.cpp, misc_sl.cpp, station_sl.cpp supply preview joins | A440 requires exact same-file map/company/economy/station facts and native discriminators. | Their unchanged named A gates now use the selected A capability, preserving A requirements while B's earlier contracts remain for41's independent join. |

## Geothermal, utility and Tube

| Original B owner | Exact source threshold / columns | Current result / adapter handoff |
| --- | --- | --- |
| geothermal_generator_sl.cpp GPGH/GPGD | Generators427; schema1 before429, schema2 from429. Operating columns heat_consumer:U32, electrical_producer:U32, service_remainder:U8, has_operating_tick:U8, last_operating_tick:U64, active_service_ticks:U64 from429. | New native owner adapter is Group43. All six operating descriptors must use GeothermalGeneratorOperation, and family presence GeothermalGenerators. Never copy donor enum names as numeric A aliases. |
| geothermal_site_sl.cpp model | MagmaPowerhouse kind belongs to B430, unrelated to A430 ordered Space actions. | Existing native cold-site reader now rejects this kind without GeothermalMagmaLoop. Shared collector/2000/tile/geometry admission stays unchanged. |
| utility_conversion_sl.cpp UCVH/UCVD | Exact conversion family B428, schema1; metadata/origin rows and nonterminal epoch. | Group43 ports the actual wire owner and uses UtilityHeatConversion for presence/scalars. This does not create an energy store or recover lost source work. |
| utility_power_sl.cpp UtilityWireRow | town_terminal:U16, town_supply:U32, town_source_port:U32 from431; generator:U16 from432. | Group43 uses WithCapability(UtilityTownService) and WithCapability(UtilityLocalElectricalFeed), with matching row/scalar semantics. Existing shared <=424 utility descriptors remain numeric. |
| transport_tube_geothermal_sl.cpp TGSH/TGSD | Original geothermal source history B433 with bounded dense rows/sequence. | Group43 uses TubeGeothermalSources for presence/scalars, retaining the one original provider/forwarding owner. |
| transport_tube_vehicle_sl.cpp TUVD | geothermal_source:U32 and local_feed:U32 from434; older source external defaults remain explicit. | Group43 uses WithCapability(TubeGeothermalBindings) for both columns and the same capability for legacy decoding. Existing A nine common columns retain exact original types. |
| multilayer_sl.cpp MLMD | B adds bits23-26 at426/427/428/433; A supports only bits0-22. | Detached inspector/classifier already checks era, complete required families and counts. Group43 must extend native metadata/family owners together; capability tests alone do not claim B426+ native load acceptance. |

These later owners/columns are currently absent from the native registry; they are
explicit43work, not silently valid defaults. The audit specifies their capability
requirements now so imported declarations cannot repeat the numeric collision.

## Later A Space families

Group42retains the full/copy semantic and original-source reader qualification;
this gate audit does not replace those representative native owner checks.

Every following unchanged A gate is handled by the explicit central mapping for
scalar decoders AND candidate descriptor intervals. Names retain their original
A meaning; B does not gain an A layout at the same/higher number.

| Owner / original fields | A feature meanings |
| --- | --- |
| space_structure_sl.cpp SPST |425 Assembly;426 manufacturing reservation/serial;427 work/product receipts;428 clone payload;429 replacement;431 content3/heavy model;434 descent ownership joins;436 immutable contained-service role;437 configured processor buffers. |
| space_production_sl.cpp SVST |427 expanded native manufacturing provenance rows,429 replacement payload,431 heavy/content3 admission. |
| engine_sl.cpp |429 replacement settlement engine reconstruction; B retains the shared pre-fork meaning. |
| space_order_sl.cpp |432 heavy missions. |
| space_manifest_sl.cpp |430 ordered service waits;433 heavy resupply;434 heavy descents;435 heavy onboard service. |
| space_transition_sl.cpp |433 heavy resupply transitions. |
| space_pod_sl.cpp |431 heavy/content3 provenance. |
| space_descent_sl.cpp |434 sender/model/unit-mass/freight-mass fields and same-file joins. |
| multilayer_sl.cpp SDEF/CDIM |431 content3 versus original content2/Classic0;437 configured cargo dimensions. B431-434 must retain content2, never A's content3 threshold. |
| orbital_recipe_sl.cpp SPRC |438 required processing family and its exact bounds/history. |
| saveload.cpp copied joins |437 dimensions and431+same-file cargo owner joins. |
| station_sl.cpp copied Space cargo |430 ordered cargo semantics, separately from440 supply/discriminator rules above. |

No original A439 file exists in the retained catalogue. Capability tests use the
source-defined439 threshold against real A438/440; no generated/downgraded save is
an original compatibility witness.

## Focused verification boundary

Group40is verified:4focused native cases/12048assertions in under3seconds, report
`test-cache/merge-supply/group40-capability-verified.txt`. Rebuild the existing
Debug openttd_tube_test target and run only `[save-lineage-capability-gates],[save-lineage-preflight]`.
[Execution notes](m2o23-3-come-space-a-execution.md#group40---verified-original-scalar-and-descriptor-gates) record the current build and scope.
The native boundary test uses all27 original catalogue inputs, actual preflight
classification and exact-bound hints, then observes scalar/descriptor decisions at
the first native decoder read and deliberately stops copied loading before any
chunk handler. This isolates the gate audit from later41/43owner adapters; it is
not a claim that those not-yet-imported native families load. It tests introduction
AND retirement intervals, explicit B descriptor requirements, every named feature,
unknown capability refusal, unchanged424/shared gates and real A435-440 inputs
(with the disclosed439gap). Independent pure policy cases remain complementary.
Native complete copied A425/B425/A440 replay remains Group39's separate witness.

No source/test/JSON fixture was created with the planning prefix. Generated build
logs and focused reports remain in test-cache/merge-supply; no retained input or
source epoch is modified by this audit.


## Group44 current writer boundary

The tables above retain the Group40original-reader audit. Groups41-43subsequently
implemented all named native families;44now publishes the explicit441writer.
MLLNschema1requires the closed merged capability set and complete actual transports.
Tagged SSM1SSORretains both receipt eras; CAPAis U64/U8only from441, while original
B U32/Bool ends at441. GPG/UCV/TGSave methods write their existing sole canonical
owners. Metadata uses actual history, including unfinished generator construction.
Original A/Bcapability interpretation stays bound and immutable throughout each
load. [Group44proof](m2o23-3-come-space-a-execution.md#group44-completion) includes
19focused cases/9010assertions, exact original migration receipts and20both-reader
marker/version/family/tag/custody refusals. No old-executable compatibility claim.
