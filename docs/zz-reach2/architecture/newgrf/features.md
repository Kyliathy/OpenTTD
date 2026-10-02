# NewGRF features

Feature IDs are `GrfSpecFeature` (`src/newgrf.h` lines 79–111). The enumerator is a `uint8_t` starting at 0, so the numeric value is the declaration order. Action `0x00` reads that byte and indexes `InvokeGrfChangeInfoHandler::funcs` (`src/newgrf/newgrf_act0.cpp` lines 167–174). The comment on that array says the handlers must stay in feature order. `GrfSpecFeature::Signals` is present in the enum and is a `nullptr` slot in the array, so Action `0x00` reports it as unknown.

| Value | Enumerator | Action `0x00` handler | File |
| --- | --- | --- | --- |
| 0 | `Trains` | `RailVehicleChangeInfo` (Activation only; Reserve returns `Unhandled`) | `src/newgrf/newgrf_act0_trains.cpp` |
| 1 | `RoadVehicles` | `RoadVehicleChangeInfo` | `src/newgrf/newgrf_act0_roadvehs.cpp` |
| 2 | `Ships` | `ShipVehicleChangeInfo` | `src/newgrf/newgrf_act0_ships.cpp` |
| 3 | `Aircraft` | `AircraftVehicleChangeInfo` | `src/newgrf/newgrf_act0_aircraft.cpp` |
| 4 | `Stations` | `StationChangeInfo` | `src/newgrf/newgrf_act0_stations.cpp` |
| 5 | `Canals` | `CanalChangeInfo` | `src/newgrf/newgrf_act0_canals.cpp` |
| 6 | `Bridges` | `BridgeChangeInfo` | `src/newgrf/newgrf_act0_bridges.cpp` |
| 7 | `Houses` | `TownHouseChangeInfo` | `src/newgrf/newgrf_act0_houses.cpp` |
| 8 | `GlobalVar` | `GlobalVarReserveInfo` and `GlobalVarChangeInfo` | `src/newgrf/newgrf_act0_globalvar.cpp` |
| 9 | `IndustryTiles` | `IndustrytilesChangeInfo` | `src/newgrf/newgrf_act0_industries.cpp` |
| 10 | `Industries` | `IndustriesChangeInfo` | `src/newgrf/newgrf_act0_industries.cpp` |
| 11 | `Cargoes` | `CargoReserveInfo` (Activation returns `Unhandled`) | `src/newgrf/newgrf_act0_cargo.cpp` |
| 12 | `SoundEffects` | `SoundEffectChangeInfo` | `src/newgrf/newgrf_act0_sounds.cpp` |
| 13 | `Airports` | `AirportChangeInfo` | `src/newgrf/newgrf_act0_airports.cpp` |
| 14 | `Signals` | no handler (`nullptr` in `funcs`) | — |
| 15 | `Objects` | `ObjectChangeInfo` | `src/newgrf/newgrf_act0_objects.cpp` |
| 16 | `RailTypes` | `RailTypeReserveInfo` and `RailTypeChangeInfo` | `src/newgrf/newgrf_act0_railtypes.cpp` |
| 17 | `AirportTiles` | `AirportTilesChangeInfo` | `src/newgrf/newgrf_act0_airports.cpp` |
| 18 | `RoadTypes` | `RoadTypeReserveInfo` / `RoadTypeChangeInfo` with `RoadTramType::Road` | `src/newgrf/newgrf_act0_roadtypes.cpp` |
| 19 | `TramTypes` | same functions with `RoadTramType::Tram` | `src/newgrf/newgrf_act0_roadtypes.cpp` |
| 20 | `RoadStops` | `RoadStopChangeInfo` | `src/newgrf/newgrf_act0_roadstops.cpp` |
| 21 | `Badges` | `BadgeChangeInfo` | `src/newgrf/newgrf_act0_badges.cpp` |
| 22 | `End` | end marker. Also `Default` and `FakeTowns` | — |

`FakeEnd` follows `FakeTowns`. `OriginalStrings` is `0x48` and `Invalid` is `0xFF` (lines 108–110). `GrfSpecFeatures` is a bitset over `GrfSpecFeature::End` (line 114). A successful Action `0x00` or a non-generic Action `0x03` sets the bit for that feature on `GRFFile::grf_features` (`src/newgrf/newgrf_act0.cpp` line 230, `src/newgrf/newgrf_act3.cpp` line 430).

`GetGrfSpecFeature(VehicleType)` maps train, road, ship, and aircraft onto the first four features (`src/newgrf.cpp` lines 1888–1896).

## What Action `0x00` writes

`FeatureChangeInfo` (`src/newgrf/newgrf_act0.cpp` lines 195–237) reads:

`feature`, `num-props`, `num-info`, an extended id, then `num-props` times a property byte and a variable-size value applied to `num-info` consecutive ids.

`ReserveChangeInfo` (lines 275–296) uses the same header during `GrfLoadingStage::Reserve`. Features whose Reserve specialization returns `ChangeInfoResult::Unhandled` are skipped. Cargo, rail types, road types, tram types, and global variables implement Reserve; the vehicle, house, industry, and station handlers do not.

`SafeChangeInfo` (lines 241–272) is the static-GRF check. Bridge property `0x0D` (sprite layout tables) is allowed. Global-variable property `0x11` (engine ID mappings) is allowed when every source GRFID is static. Every other Action `0x00` in a static file calls `GRFUnsafe`.

Property numbers are per feature and are the `case` labels inside each `*ChangeInfo` function. They are not a second global enum. Examples that the handlers name in comments: train property `0x05` is track type (`src/newgrf/newgrf_act0_trains.cpp`), station property `0x08` is class ID (`src/newgrf/newgrf_act0_stations.cpp`), industry property `0x08` is the substitute type and `0x09` is the override (`src/newgrf/newgrf_act0_industries.cpp`).

## Where the properties land

Vehicles write an `Engine` and its `GRFFileProps`. Action `0x03` for the four vehicle features calls `VehicleMapSpriteGroup` (`src/newgrf/newgrf_act3.cpp` lines 435–438). The engine-to-GRF map is `_engine_mngr` and is saved as chunk `EIDS`.

Houses, industries, industry tiles, objects, airports, and airport tiles keep a spec on the `GRFFile` during the file (`housespec`, `industryspec`, `indtspec`, `objectspec`, `airportspec`, `airtspec` in `src/newgrf.h` lines 133–139). `AfterLoadGRFs` calls `FinaliseHouseArray`, `FinaliseIndustriesArray`, `FinaliseObjectsArray`, and `FinaliseAirportsArray`, and each of those calls `SetEntitySpec` on the matching `OverrideManager`. The managers and their save chunks are listed in [archi-newgrf.md](archi-newgrf.md).

`StandardGRFFileProps` holds two sprite groups: `StandardSpriteGroup::Default` and `StandardSpriteGroup::Purchase` (`src/newgrf_commons.h` lines 365–394). `GetSpriteGroup(bool entity_exists)` returns the purchase group when the entity does not exist yet, and the default group otherwise.

Cargo, rail types, and road types also build translation tables on `GRFFile` (`cargo_list` / `cargo_map`, `railtype_list` / `railtype_map`, and the road and tram equivalents, `src/newgrf.h` lines 146–158). `BuildCargoTranslationMap` fills the inverse map at the end of Activation (`src/newgrf.cpp` lines 542–552 and 1863).

## Action `0x03` binding

`FeatureMapSpriteGroup` (`src/newgrf/newgrf_act3.cpp` lines 392–457) reads a feature, an id count, the ids, then cargo-type / sprite-group pairs and a default group. `idcount == 0` does not bind an entity: it calls `AddGenericCallback` (lines 416–426) and does not set `grf_features`.

The switch dispatches trains, road vehicles, ships, aircraft, canals, stations, houses, industries, industry tiles, cargoes, airports, objects, rail types, road types, tram types, airport tiles, road stops, and badges. Bridges, global variables, sound effects, and signals fall through to the error at line 455.

## Action `0x05` block kinds

`Action5BlockType` (`src/newgrf_act5.h` lines 16–20) describes how a base-sprite replacement is applied: `Fixed` replaces a whole block, `AllowOffset` replaces a subset, `Invalid` is unknown. `GraphicsNew` runs only at Activation.
