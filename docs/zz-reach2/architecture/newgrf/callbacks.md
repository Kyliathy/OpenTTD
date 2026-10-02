# Callbacks

Callback identifiers are `enum CallbackID : uint16_t` in `src/newgrf_callbacks.h` lines 22–296. The comments in that enum are the implementation's own description, including which results are "not implemented". Values that the enum skips are not IDs.

`CBID_NO_CALLBACK` (`0x00`) is the resolver default when the caller wants a sprite, not a callback (`ResolverObject` constructor, `src/newgrf_spritegroup.h` lines 334–336). `CBID_RANDOM_TRIGGER` (`0x01`) is the value `RandomizedSpriteGroup::Resolve` tests before it records reseeds (`src/newgrf_spritegroup.cpp` line 256).

Most callbacks run only when the matching bit is set on the feature's callback mask. The enum's file comment says so (lines 16–19). Masks are separate enums later in the same header.

`CALLBACK_FAILED` is `0xFFFF` (line 459). `ResolveCallback` returns that when the resolved variant is not a `CallbackResult` (`src/newgrf_spritegroup.h` lines 423–427).

## `CallbackID`

| Name | Value | Line | Note from the enum |
| --- | --- | --- | --- |
| `CBID_NO_CALLBACK` | `0x00` | 24 | Resolve system, not a callback |
| `CBID_RANDOM_TRIGGER` | `0x01` | 27 | Randomizing trigger |
| `CBID_VEHICLE_VISUAL_EFFECT` | `0x10` | 32 | 8 bit. Visual effects and wagon power |
| `CBID_VEHICLE_LENGTH` | `0x11` | 35 | Length in 1/8 for trains and road vehicles |
| `CBID_VEHICLE_LOAD_AMOUNT` | `0x12` | 38 | 8 bit. Gradual loading amount |
| `CBID_STATION_AVAILABILITY` | `0x13` | 41 | 8 bit |
| `CBID_STATION_DRAW_TILE_LAYOUT` | `0x14` | 44 | Choose a tile layout |
| `CBID_VEHICLE_REFIT_CAPACITY` | `0x15` | 50 | 15 bit |
| `CBID_VEHICLE_ARTIC_ENGINE` | `0x16` | 53 | 8 bit when GRF version < 8 |
| `CBID_HOUSE_ALLOW_CONSTRUCTION` | `0x17` | 56 | 8 bit |
| `CBID_GENERIC_AI_PURCHASE_SELECTION` | `0x18` | 59 | 8 bit. Stations only |
| `CBID_VEHICLE_CARGO_SUFFIX` | `0x19` | 62 | Cargo suffix per refit |
| `CBID_HOUSE_ANIMATION_NEXT_FRAME` | `0x1A` | 65 | 15 bit |
| `CBID_HOUSE_ANIMATION_TRIGGER_TILE_LOOP` | `0x1B` | 68 | 15 bit |
| `CBID_HOUSE_ANIMATION_TRIGGER_CONSTRUCTION_STAGE_CHANGED` | `0x1C` | 71 | 15 bit |
| `CBID_TRAIN_ALLOW_WAGON_ATTACH` | `0x1D` | 74 | Wagon attach |
| `CBID_HOUSE_COLOUR` | `0x1E` | 77 | 15 bit |
| `CBID_HOUSE_CARGO_ACCEPTANCE` | `0x1F` | 80 | 15 bit |
| `CBID_HOUSE_ANIMATION_SPEED` | `0x20` | 83 | 8 bit |
| `CBID_HOUSE_DESTRUCTION` | `0x21` | 86 | 8 bit |
| `CBID_INDUSTRY_PROBABILITY` | `0x22` | 89 | 15 bit. GRF version ≥ 8 may return a probability |
| `CBID_VEHICLE_ADDITIONAL_TEXT` | `0x23` | 95 | Purchase-list string in the `0xD000` range |
| `CBID_STATION_BUILD_TILE_LAYOUT` | `0x24` | 98 | 15 bit |
| `CBID_INDTILE_ANIMATION_TRIGGER` | `0x25` | 101 | 15 bit |
| `CBID_INDTILE_ANIMATION_NEXT_FRAME` | `0x26` | 104 | 15 bit |
| `CBID_INDTILE_ANIMATION_SPEED` | `0x27` | 107 | 8 bit |
| `CBID_INDUSTRY_LOCATION` | `0x28` | 110 | 15 bit |
| `CBID_INDUSTRY_PRODUCTION_CHANGE` | `0x29` | 113 | 15 bit |
| `CBID_HOUSE_ACCEPT_CARGO` | `0x2A` | 116 | 15 bit |
| `CBID_INDTILE_CARGO_ACCEPTANCE` | `0x2B` | 119 | 15 bit |
| `CBID_INDTILE_ACCEPT_CARGO` | `0x2C` | 122 | 15 bit |
| `CBID_VEHICLE_COLOUR_MAPPING` | `0x2D` | 128 | 15 bit |
| `CBID_HOUSE_PRODUCE_CARGO` | `0x2E` | 131 | 15 bit |
| `CBID_INDTILE_SHAPE_CHECK` | `0x2F` | 134 | 15 bit |
| `CBID_INDTILE_DRAW_FOUNDATIONS` | `0x30` | 137 | 15 bit |
| `CBID_VEHICLE_START_STOP_CHECK` | `0x31` | 143 | 15 bit; the `0xFF` test uses 8 bits |
| `CBID_VEHICLE_32DAY_CALLBACK` | `0x32` | 146 | 2 bit. Every vehicle, every 32 days |
| `CBID_VEHICLE_SOUND_EFFECT` | `0x33` | 149 | 15 bit |
| `CBID_VEHICLE_AUTOREPLACE_SELECTION` | `0x34` | 152 | 15 bit. Enum comment: not implemented |
| `CBID_INDUSTRY_MONTHLYPROD_CHANGE` | `0x35` | 155 | 15 bit |
| `CBID_VEHICLE_MODIFY_PROPERTY` | `0x36` | 161 | 8 or 15 bit, depending on the Action `0x00` property index in parameter 1 |
| `CBID_INDUSTRY_CARGO_SUFFIX` | `0x37` | 164 | 15 bit; the `0xFF` test uses 8 bits |
| `CBID_INDUSTRY_FUND_MORE_TEXT` | `0x38` | 167 | 15 bit |
| `CBID_CARGO_PROFIT_CALC` | `0x39` | 170 | 15 bit |
| `CBID_INDUSTRY_WINDOW_MORE_TEXT` | `0x3A` | 173 | 15 bit |
| `CBID_INDUSTRY_SPECIAL_EFFECT` | `0x3B` | 176 | 15 bit |
| `CBID_INDTILE_AUTOSLOPE` | `0x3C` | 179 | 15 bit |
| `CBID_INDUSTRY_REFUSE_CARGO` | `0x3D` | 182 | 15 bit |
| `CBID_STATION_ANIMATION_TRIGGER` | `0x140` | 187 | 15 bit. The enum states there are no callbacks `0x3E`–`0x13F` (line 184) |
| `CBID_STATION_ANIMATION_NEXT_FRAME` | `0x141` | 190 | 15 bit |
| `CBID_STATION_ANIMATION_SPEED` | `0x142` | 193 | 8 bit |
| `CBID_HOUSE_DENY_DESTRUCTION` | `0x143` | 196 | 15 bit |
| `CBID_SOUNDS_AMBIENT_EFFECT` | `0x144` | 199 | 15 bit |
| `CBID_CARGO_STATION_RATING_CALC` | `0x145` | 202 | 15 bit |
| `CBID_NEW_SIGNALS_SPRITE_DRAW` | `0x146` | 205 | 15 bit. Enum comment: not implemented |
| `CBID_CANALS_SPRITE_OFFSET` | `0x147` | 208 | 15 bit |
| `CBID_HOUSE_ANIMATION_TRIGGER_WATCHED_CARGO_ACCEPTED` | `0x148` | 211 | 15 bit |
| `CBID_STATION_LAND_SLOPE_CHECK` | `0x149` | 214 | 15 bit |
| `CBID_INDUSTRY_DECIDE_COLOUR` | `0x14A` | 217 | 4 bit |
| `CBID_INDUSTRY_INPUT_CARGO_TYPES` | `0x14B` | 220 | 8 bit |
| `CBID_INDUSTRY_OUTPUT_CARGO_TYPES` | `0x14C` | 223 | 8 bit |
| `CBID_HOUSE_CUSTOM_NAME` | `0x14D` | 226 | 15 bit |
| `CBID_HOUSE_DRAW_FOUNDATIONS` | `0x14E` | 229 | 15 bit |
| `CBID_HOUSE_AUTOSLOPE` | `0x14F` | 232 | 15 bit |
| `CBID_AIRPTILE_DRAW_FOUNDATIONS` | `0x150` | 235 | 15 bit. The next enumerator is `0x152`; `0x151` is not in the enum |
| `CBID_AIRPTILE_ANIMATION_TRIGGER` | `0x152` | 238 | 15 bit |
| `CBID_AIRPTILE_ANIMATION_NEXT_FRAME` | `0x153` | 241 | 15 bit |
| `CBID_AIRPTILE_ANIMATION_SPEED` | `0x154` | 244 | 8 bit |
| `CBID_AIRPORT_ADDITIONAL_TEXT` | `0x155` | 250 | 15 bit. String in the `0xD000` range |
| `CBID_AIRPORT_LAYOUT_NAME` | `0x156` | 253 | 15 bit |
| `CBID_OBJECT_LAND_SLOPE_CHECK` | `0x157` | 256 | 15 bit |
| `CBID_OBJECT_ANIMATION_NEXT_FRAME` | `0x158` | 259 | 15 bit |
| `CBID_OBJECT_ANIMATION_TRIGGER` | `0x159` | 262 | 15 bit |
| `CBID_OBJECT_ANIMATION_SPEED` | `0x15A` | 265 | 8 bit |
| `CBID_OBJECT_COLOUR` | `0x15B` | 268 | 15 bit |
| `CBID_OBJECT_FUND_MORE_TEXT` | `0x15C` | 271 | 15 bit |
| `CBID_OBJECT_AUTOSLOPE` | `0x15D` | 274 | 15 bit |
| `CBID_VEHICLE_REFIT_COST` | `0x15E` | 277 | 15 bit |
| `CBID_INDUSTRY_PROD_CHANGE_BUILD` | `0x15F` | 280 | 15 bit |
| `CBID_VEHICLE_SPAWN_VISUAL_EFFECT` | `0x160` | 283 | 15 bit |
| `CBID_VEHICLE_NAME` | `0x161` | 286 | 15 bit |
| `CBID_VEHICLE_BUILD_PROBABILITY` | `0x162` | 289 | 15 bit |
| `CBID_VEHICLE_CUSTOM_REFIT` | `0x0163` | 295 | 15 bit. Once per defined cargo, after all NewGRFs load |

`CALLBACK_HOUSEPRODCARGO_END` is `0x20FF` (line 460), the sentinel for the `CBID_HOUSE_PRODUCE_CARGO` loop.

## Masks

A mask bit is not the callback id. It is the bit the GRF sets so the game will call the callback. The bit positions below are the enumerator values in `src/newgrf_callbacks.h`.

| Mask | Lines | Bits |
| --- | --- | --- |
| `VehicleCallbackMask` | 302–313 | `VisualEffect` 0, `Length` 1, `LoadAmount` 2, `RefitCapacity` 3, `ArticEngine` 4, `CargoSuffix` 5, `ColourRemap` 6, `SoundEffect` 7, `Name` 8, `CustomRefit` 9. The comment says some vehicle callbacks are always used and have no mask. |
| `StationCallbackMask` | 321–327 | `Avail` 0, `DrawTileLayout` 1, `AnimationNextFrame` 2, `AnimationSpeed` 3, `SlopeCheck` 4 |
| `RoadStopCallbackMask` | 335–339 | `Avail` 0, `AnimationNextFrame` 1, `AnimationSpeed` 2 |
| `HouseCallbackMask` | 347–361 | `AllowConstruction` 0 through `Autoslope` 12, in declaration order |
| `CanalCallbackMask` | 369–371 | `SpriteOffset` 0 |
| `CargoCallbackMask` | 379–382 | `ProfitCalc` 0, `StationRatingCalc` 1 |
| `IndustryCallbackMask` | 390–406 | `Probability` 0 through `ProdChangeBuild` 14, in declaration order. `ProductionChange` is 4, `MonthlyProdChange` is 5 |
| `IndustryTileCallbackMask` | 414–422 | `AnimationNextFrame` 0, `AnimationSpeed` 1, `CargoAcceptance` 2, `AcceptCargo` 3, `ShapeCheck` 4, `DrawFoundations` 5, `Autoslope` 6 |
| `ObjectCallbackMask` | 430–437 | `SlopeCheck` 0 through `Autoslope` 5 |
| `AirportTileCallbackMask` | 445–451 | `AnimationNextFrame` 0, `AnimationSpeed` 1, `ShapeCheck` 4, `DrawFoundations` 5, `Autoslope` 6. Bits 2 and 3 are unused in this enum |

`GrfMiscBit::AmbientSoundCallback` (bit 4 of global variable `0x1E` / `0x9E`, `src/newgrf.h` lines 65–71) is the switch for callback `0x144`, separate from these masks.

## How a callback is resolved

The caller constructs a `ResolverObject` subclass with the `CallbackID` and two parameters. Those become variables `0x0C`, `0x10`, and `0x18` inside every deterministic group (`src/newgrf_spritegroup.cpp` lines 57–59). The root group is the one Action `0x03` stored on the entity.

`ResolveCallback` walks `SpriteGroup::Resolve` until a group returns a `CallbackResult` (a `CallbackResultSpriteGroup`, or a deterministic group with `calculated_result`). Registers `0x100` and up are copied into the caller's `regs100` span (lines 428–430).

### Industry production, traced

1. `_economy_industries_daily` or `_economy_industries_monthly` (`src/industry_cmd.cpp` lines 3084 and 3125) calls `ChangeIndustryProduction(i, monthly)`.
2. The function tests `IndustryCallbackMask::ProductionChange` or `MonthlyProdChange` on `IndustrySpec::callback_mask` (lines 2888–2889).
3. `GetIndustryCallback` (`src/newgrf_industries.cpp` line 528) is called with `CBID_INDUSTRY_PRODUCTION_CHANGE` (`0x29`) or `CBID_INDUSTRY_MONTHLYPROD_CHANGE` (`0x35`), parameter 1 `0`, parameter 2 `Random()`.
4. `IndustriesResolverObject` (lines 478–483) sets `root_spritegroup` from `IndustrySpec::grf_prop.GetSpriteGroup`. An industry with a valid index uses `StandardSpriteGroup::Default`.
5. `ResolveCallback` → `DoResolve` → `SpriteGroup::Resolve`. A randomized group in self scope indexes `Industry::random`. Parent scope is the town, whose resolver does not override `GetRandomBits`. Deterministic nodes see the callback id in variable `0x0C` and `Random()` in variable `0x18`.
6. `CALLBACK_FAILED` means the caller does nothing (line 2892). A result uses bits 0–3 as the operation, bit 7 to suppress the news message, and bit 8 to take a custom string from register `0x100`.

The same `GetIndustryCallback` entry point is used for location checks (`CheckIfCallBackAllowsCreation`, callback `0x28`, lines 545–561), probability (`0x22`, line 582), and refuse-cargo (`0x3D`, line 693). Fund-window and industry-window text (`0x38`, `0x3A`) are called from `src/industry_gui.cpp` (lines 579 and 965), which is the UI, not the economy timer.

### Vehicles

`GetVehicleCallback` (`src/newgrf_engine.cpp` lines 1211–1214) builds a `VehicleResolverObject` with `WagonOverride::Uncached` and returns `ResolveCallback`. The constructor (lines 1086–1115) chooses the root group from a wagon override, the cached train override, or `Engine::grf_prop.GetFirstSpriteGroupOf` for the vehicle's cargo and then `SG_DEFAULT`. `GetVehicleCallbackParent` (lines 1228–1232) is the same walk with an explicit parent vehicle.

`GetEngineProperty` (lines 1243–1245) is callback `0x36`: parameter 1 is the Action `0x00` property id. `DoTriggerVehicleRandomisation` (line 1278) resolves `CBID_RANDOM_TRIGGER` and then stores new `random_bits`. `TestVehicleBuildProbability` (line 1266) resolves callback `0x162` and combines a non-failed result with `RandomRange`.

Drawing uses the same resolver with `CBID_NO_CALLBACK` and `Resolve<ResultSpriteGroup>` (`GetCustomEngineSprite`, lines 1118–1127). That path selects a sprite; it does not return a `CallbackResult`.
