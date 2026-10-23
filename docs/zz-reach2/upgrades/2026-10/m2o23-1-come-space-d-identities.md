# M2O23-1 — Command, save and capsule identities

Owner: [Part 1, Group 7](m2o23-1-come-space.md#group-7---freeze-append-only-command-and-fleet-identities). Architecture: [map-layer command and storage invariants](../../architecture/map-layers/archi-map-layers.md#invariants). This contract allocates names/values for later implementation; command registration and fleet activation remain Groups9/31. It is not a second runtime registry.

## Pinned allocation and command map

Read-only HEAD refresh on2026-10-23 confirmed A `cfeff84db52f8a3753a43c2a770a6363c62b2c67` and B `75ea75718fa7d14632bf795105721df1e390f753`. Their `Commands : uint8_t` lists share exactly the unchanged prefix0-203 (`BuildRailLong` through `EvacuateOrbitalModule`). Keep that prefix's names, values, traits, argument tuples and owners from A. The table covers every divergent tail command; `—` means absent from that original branch, not numeric zero. Argument tuples omit the local `DoCommandFlags` execution argument. Strong IDs retain their declared underlying native codec; never substitute a platform-sized integer.

| Command / canonical destination | A value | B value | Merged value | Transport argument tuple | Sole implementation owner |
| --- | --- | --- | --- | --- | --- |
| QueueOrbitalManufacture | 204 | — | 204 | `(SpaceStructureID site, uint64 generation, SpaceStructureID source, uint64 source_generation, uint16 model)` | A `orbital_manufacturing.cpp` (declared by `space_structure_cmd.h`). |
| CancelOrbitalManufacture | 205 | — | 205 | `(SpaceStructureID site, uint64 generation, SpaceStructureID destination, uint64 destination_generation)` | A `orbital_manufacturing.cpp` (declared by `space_structure_cmd.h`). |
| CancelEarthSpaceReplacement | 206 | — | 206 | `(VehicleID product, uint64 generation)` | A `space_replacement` owner. |
| ControlOrbitalRecipe | 207 | — | 207 | `(SpaceStructureID site, uint64 generation, uint32 recipe_value, uint8 control_value)` | A `orbital_recipe_runtime.cpp` (declared by `space_structure_cmd.h`). |
| SetSpaceMissionChecked | 208 | — | 208 | `(VehicleID vehicle, uint64 generation, uint32 body)` | A `space_vehicle.cpp` (declared by `space_vehicle_cmd.h`). |
| PlaceSurfaceSupplyOrder; B OrderSurfaceSupply adapts here | 209 | 208 | 209 | A `(StationID origin, uint32 label_value, uint16 quantity, uint64 intent)`; B originally `(StationID station, CargoLabel label, uint16 quantity, uint32 expected_order)` | A `surface_supply_order_runtime.cpp`; new native Pending commitment and command-kernel Property expense. |
| CancelSurfaceSupplyOrder | 210 | — | 210 | `(uint64 intent)` unchanged | A `surface_supply_order_runtime.cpp`; native-only Pending cancellation. |
| CancelLegacySurfaceSupplyOrder; B CancelSurfaceSupply adapts here | — | 209 | 211 | Merged `(uint64 intent, uint16 expected_quantity)`; B originally `(uint32 order, uint16 expected_quantity)` | A `surface_supply_order_runtime.cpp`, checked legacy branch in the one refund owner. |
| ChangeTBMWork | — | 204 | 212 | `(TBMProjectID project, TileIndex area_start, TileIndex area_end, uint32 expected_cursor, uint64 expected_fingerprint, ExcavationPlanIntent intent)` | Imported `tbm_project.cpp` (declared by `tbm_cmd.h`). |
| BuildGeothermalGenerator | — | 210 | 213 | `(TileIndex tile, GeothermalSiteID site, uint32 seed)` | `industry_cmd.cpp`, actual funded cold site. |
| PlanGeothermalSite | — | 211 | 214 | `(TileIndex tile, GeothermalVentID vent, MultiLayerGeothermalConstructionKind kind, TileIndex endpoint, DiagDirection access)` | `geothermal_site_cmd.cpp`, original site owner. |
| CancelGeothermalSite | — | 212 | 215 | `(GeothermalSiteID site)` | Same site owner, no reused IDs. |
| ConnectGeothermalTownService | — | 213 | 216 | `(IndustryID generator, IndustryID first_riser, IndustryID second_riser, StationID terminal, TownID town)` | `geothermal_service_cmd.cpp`, shared physical service owner. |
| ConnectTubePower | — | 205 | 217 | `(TileIndex portal, TileIndex interchange, TubePowerCommandPath path)` | `transport_tube_cmd.cpp`, original external provider transaction. |
| CloseTubePower | — | 206 | 218 | `(UtilityProducerID source)` | Same original external provider owner. |
| SetTubeVehiclePower | — | 207 | 219 | `(VehicleID vehicle, UtilityProducerID source, bool reconnect)` | Same canonical capsule binding owner. |
| ConnectTubeGeothermalPower | — | 214 | 220 | `(IndustryID generator, TileIndex inlet, UtilityProducerID expected_output)` | `transport_tube_cmd.cpp`, original generator/inlet owner. |
| CloseTubeGeothermalPower | — | 215 | 221 | `(TubeGeothermalSourceID source)` | Same inlet owner, distinct from generator producer. |
| SetTubeVehicleGeothermalPower | — | 216 | 222 | `(VehicleID vehicle, TubeGeothermalSourceID source, bool reconnect)` | Same capsule/provider transaction. |
| End (sentinel, never executable) | 211 | 217 | 223 | None | `command_type.h`, always last. |

Revision2 follows Group9B's decision to register commands only with implemented bodies: append in producing-group order, preserving all211 executable A command values. The prior unpublished allocation and its exact original-source records remain in the historical oracle; see the revision record below. No donor numeric tail may be copied over A's enum. `CancelLegacySurfaceSupplyOrder` is the deliberate Group6 refinement: retain B's count check without changing A's existing cancellation payload or importing B's direct money settlement. Only new native purchase semantics are exposed to current UI; old B commands are not an alternate immediate-stock purchase API.

## Payload conversion and replay refusal

The command `EndianBufferWriter` encodes integral values **little-endian**. A's `label_value` is a uint32 whose *semantic construction* is big-endian canonical label bytes; B's `CargoLabel` overload writes the four label bytes directly. Those byte streams differ. Decode by verified source tuple, form A's value from shifts24/16/8/0, then re-encode through the existing tuple codec. Do not memcpy the donor payload or treat the label as a runtime cargo ordinal.

For station0x1234, label RFMT, quantity100 and intent7, the argument bytes excluding command/header/flags are:

```text
B original OrderSurfaceSupply: 34 12 52 46 4d 54 64 00 07 00 00 00
A PlaceSurfaceSupplyOrder:    34 12 54 4d 46 52 64 00 07 00 00 00 00 00 00 00
```

StationID's uint16 transport stays exact. B's original uint32 order widens to uint64; A intents above UINT32_MAX must never narrow through a donor strong typedef. Cancellation keeps the expected uint16 count after the widened intent. Truncated/extra tuple bytes, invalid enums/path counts, unknown command IDs and unrepresentable data refuse before dispatch. `TubePowerCommandPath` remains its explicit bounded count/prefix codec, not raw struct/padding serialization.

**Old numeric command logs and peer packets are not executable merely because this table maps their names.** Refuse unverified source lineage/ABI and all unsupported automatic replay. A future explicitly authorized historical-log converter must verify the exact source epoch and complete tuple and account for B's different immediate-supply behavior before translating anything. Fresh merged commands use the new canonical behavior; do not replay B's old purchase as if its stock timing were unchanged. Group47 must test incompatible-peer refusal before delivery, not repair a received command after it has executed.

## Save and canonical-state allocations

The next unused merged writer is441, reserved by `MERGED_SAVE_LINEAGE_VERSION` in [save_lineage.h](../../../../src/saveload/save_lineage.h). `MERGED_SAVE_LINEAGE_MAGIC = 0x4D4C4E31` and `MERGED_SAVE_LINEAGE_SCHEMA = 1` belong to that same public owner. The `MLLN` RIFF payload is network-order U32 magic, U16 schema, U16 writer version; this save payload order is distinct from the command codec. Current `SaveLoadVersion`/writer remains440 until [Plan3 Group44](m2o23-3-come-space.md#group-44---publish-one-explicit-merged-format-and-round-trip-contract) validates and activates the complete merged format. Recheck availability then; do not redefine these constants elsewhere.

`MULTILAYER_STATE_SCHEMA_VERSION` stays1: it describes the existing metadata row, not save lineage. The following existing donor identities are now reserved once in A's [multilayer_state.h](../../../../src/multilayer_state.h), without publishing bits or activating readers before owners exist. Existing flags and canonical kinds keep their values.

| Public constant / canonical kind | Exact value | Meaning and publication condition |
| --- | --- | --- |
| MULTILAYER_FEATURE_SURFACE_SUPPLIES | 0x00800000 | Complete canonical retained supply history; merged capture checks the actual one-book history. Legacy B retains its original dense-history rule. A440 does not retroactively gain this flag. |
| MULTILAYER_FEATURE_GEOTHERMAL_GENERATORS | 0x01000000 | Actual generator rows/history, matching the original family. |
| MULTILAYER_FEATURE_HEAT_CONVERSIONS | 0x02000000 | Original converted utility sources and their thermal attribution. |
| MULTILAYER_FEATURE_TUBE_GEOTHERMAL | 0x04000000 | Original inlet identity/retirement history. |
| MultiLayerCanonicalKind::SurfaceSupplyRoot | 33 | Digest of the one canonical tagged book and permanent packet purpose; no packet stock duplicate. |
| MultiLayerCanonicalKind::GeothermalGeneratorRoot | 34 | Original machinery/work/receipt root, excluding copied utility balances. |
| MultiLayerCanonicalKind::TubeGeothermalRoot | 35 | Original inlet references/history, excluding derived readiness. |
| MultiLayerCanonicalKind::End | 36 | Non-record sentinel, following the append-only kinds. |

Feature bit presence is not a lineage substitute. Group5's dialect result selects version meaning; Groups36/46 publish complete deterministic records through `ComputeMultiLayerStateDigest`. The uint32 canonical-record key is not permission to truncate uint64 order IDs: use the existing aggregate SupplyRoot record and hash exact64-bit identities in its owner-defined fields/order. Do not invent per-order IDs in the diagnostic schema or a parallel merge hash.

## Delivered capsule coefficients and one installer

Select B's delivered `_tube_definitions` values from [transport_tube_engine.cpp](../../../../../OpenTTD3/src/transport_tube_engine.cpp), while preserving A's complete preflight and persisted-hole allocation safety. Keep the existing geometry/catalogue owner, one native engine installer and `GetTubeVehicleDefinition` as the eventual tuning query. Adapt callers away from A's provisional `GetTubeVehicleTuning` once the replacement is integrated; do not run both installers or leave two active coefficient tables. Preserve provisional A source as dated retained evidence, not an active balance switch.

Tuple order below is `(capacity, speed, power_hp, empty_weight_t, purchase_factor, running_factor, load_amount, refit_cost, tractive_effort)`; each is the existing native unit, not a new financial/energy definition. Air drag8, one full native part and model9's original physical/EIDS identity are unchanged.

| Model / local EIDS | A provisional tuple | Selected B tuple | Exact changed coefficients |
| --- | --- | --- | --- |
| 9 / 0x8009 | `(60,100,1000,20,120,100,8,10,128)` | `(60,100,1000,20,120,100,8,10,128)` | None; preserve delivered prototype identity and actual vehicles. |
| 10 / 0x800a | `(40,140,1200,18,150,110,12,10,128)` | `(40,130,1100,18,150,110,8,10,128)` | speed140→130, power1200→1100, loading12→8. |
| 11 / 0x800b | `(40,100,1000,25,180,130,6,15,128)` | `(48,90,1000,28,180,140,8,10,128)` | capacity40→48, speed100→90, weight25→28, running130→140, loading6→8, refit15→10. |
| 12 / 0x800c | `(24,110,1000,35,220,180,4,20,128)` | `(24,90,950,45,220,160,8,10,128)` | speed110→90, power1000→950, weight35→45, running180→160, loading4→8, refit20→10. |
| 13 / 0x800d | `(96,80,900,60,250,220,4,20,96)` | `(80,70,1500,48,240,220,8,10,128)` | capacity96→80, speed80→70, power900→1500, weight60→48, price250→240, loading4→8, refit20→10, traction96→128. |

The sole identity is `(VehicleType::Train, required package GRFID, local_id = 0x8000 + model)`. Keep existing actual EngineIDs, EIDS reservations, vehicle IDs and simulation RNG. Catalogue dates, labels, layer rules and electrical envelopes stay in their existing owners; physical coefficients grant no energy or availability. `CheckTubeVehicleCargoChange` compares `TotalCount()` (including reservations), retained cargo label and `min(target definition capacity, operation-specific retained_capacity)` before refit/replacement mutation or temporary vehicle creation.

For an original A model13 carrying/reserving more than80 units, migration must preserve all actual packets and its original native retained-capacity field until they can safely unload; no recalculation may silently clip cargo. Groups32/42 must characterize the actual native afterload/cache path and retain that necessary original capacity through the existing train owner, then use the selected80-unit definition for subsequent empty refit/replacement. No new inventory, free refit, balance registry or guessed cargo is permitted. An attempted loaded refit/replacement into80 refuses unchanged. Do not conflate original-load preservation with admission of newly bought96-unit hulls.

The complete223-command expected map, including every unchanged prefix tuple and implementation path, is retained in [merge-command-identity-sequential.json](../../../../src/tests/fixtures/multilayer/merge-command-identity-sequential.json). It is an independent test fixture pinned to the two original source epochs, not an executable registry. All204 common argument declarations were compared and are identical. Original function names/headers and widths are preserved in its source-labelled entries.

## Verification and downstream ownership

The read-only enum audit confirms A executable IDs0-210, B0-216 and the exact shared0-203 prefix. The append map yields merged0-222 plus End223, fitting uint8 without a protocol-width change. Independent standard-library byte packing reproduces the RFMT examples. A/B public feature bits and all35 canonical kind values now agree without publishing new runtime state. Group9 verifies actual trait registration; Groups31-32 implement the selected fleet and loaded-cargo checks; [Plan3 Group47](m2o23-3-come-space.md#group-47---verify-command-abi-and-incompatible-peer-refusal) verifies full native command transport and real peer refusal. No native build/peer/model qualification is claimed here.

## Sequential allocation revision, 2026-10-23

Group14D found that the native `MakeCommandsFromTraits`/network serialization tables instantiate every ID below `Commands::End`. Group9B deliberately deferred unimplemented traits. The original planned order put the first new implemented command, legacy cancellation, behind eleven unavailable bodies. Do not manufacture refusal stubs, sparse protocol holes or premature gameplay owners to satisfy that plan.

Revision2 changes only the unpublished merged tail: legacy cancellation211 (Group15), TBM replanning212 (Group23), generator build/site planning/cancellation213-215 (Groups25-26), town service216 (Group30), and the three external plus three geothermal tube-provider commands217-222 (Group33). A's original0-210, every B original value/tuple, common0-203 and final End223 are unchanged. Each producing group appends only its complete contiguous implemented prefix; the live End advances with that prefix. Neither branch's original numeric tail is accepted as merged replay.

The [original Group7 oracle](../../../../src/tests/fixtures/multilayer/merge-command-identity.json) is retained byte-for-byte as historical planning evidence. The [revision2 oracle](../../../../src/tests/fixtures/multilayer/merge-command-identity-sequential.json) records its SHA256 and copies all original A/B facts unchanged; only the explicit new allocation differs. This is a design revision before implementation, not a baseline generated from changed command output. [Plan3 Group47](m2o23-3-come-space.md#group-47---verify-command-abi-and-incompatible-peer-refusal) must compare live commands with revision2 and independently verify the unchanged original source fields. Existing original-save/media/checkpoint fixtures are untouched.
