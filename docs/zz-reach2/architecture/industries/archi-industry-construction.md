# Industry construction

How an industry is placed, how its tiles grow to completion, and how the game decides to place more of them. Cargo, rates, and the monthly production change are in [archi-industries.md](archi-industries.md).

## Who calls creation

The native layer-aware extension below is owned by [R2MLVS6](../../upgrades/2026-10/r2mlvs6-mines-and-resource-chains.md). The following conventional construction paths retain their legacy role.

| Path | Entry | Founder | Notes |
| --- | --- | --- | --- |
| Map generation | `GenerateIndustries` → `PlaceInitialIndustry` → `PlaceIndustry` | `OWNER_NONE` | Runs only while the world is generating. Density `FundedOnly` skips it outside the editor |
| Gameplay random build | `_economy_industries_daily` → `IndustryBuildData::TryBuildNewIndustry` | `OWNER_NONE` | Skipped entirely while the economy is in recession |
| Player or deity | `CmdBuildIndustry` | current company, or `OWNER_TOWN` while prospecting | Deity prospecting (`fund == false`) searches random tiles. Deity funding and the editor place on the chosen tile |
| Raw prospecting | `CmdBuildIndustry` when `raw_industry_construction == 2` and the spec is raw | original company, tiles cleared as `OWNER_TOWN` | Up to 5000 random tiles. `prospecting_chance` can fail the attempt before the search |

`CreateNewIndustry` picks one random layout and calls `CreateNewIndustryHelper` with `DoCommandFlag::Execute`. `PlaceIndustry` tries that 2000 times, or 10000 times when the generator is forcing one of a type. Failure returns nullptr. There is no reason code on that path.

`CmdBuildIndustry` rejects a type that is disabled or has no layouts, a raw type when `raw_industry_construction == 0` (editor and deity exempt), and a type whose `CBID_INDUSTRY_PROBABILITY` result is 0 (`GetIndustryProbabilityCallback`, mask `IndustryCallbackMask::Probability`). The placed-on-tile path walks every layout, starting at `first_layout`, and returns the helper’s error when every layout fails. The prospect path runs only when the command is executed: deity prospecting always searches, raw prospecting searches when `Random() <= prospecting_chance`. A failed search shows an error to the local company and still falls through. The success return is `IndustrySpec::GetConstructionCost` (price base `BuildIndustry`, or `BuildIndustryRaw` when raw construction is priced that way, times `cost_multiplier`, shifted 8), including the prospect path after a failed search and the test-only call that does not search.

## Checks before tiles exist

`CreateNewIndustryHelper` runs the checks in this order. Any failure aborts before `Industry::Create`.

1. `CheckIfFarEnoughFromConflictingIndustry`. Each of `conflicting[3]` that is not `IT_INVALID` refuses a site within `DistanceMax` 14 of an industry of that type (`STR_ERROR_INDUSTRY_TOO_CLOSE`).
2. `FindTownForIndustry`. The town is `ClosestTownFromTile`. When `multiple_industry_per_town` is off, a second industry of the same type in that town fails.
3. `CheckIfIndustryIsAllowed`. `Town1200More` requires town population at least 1200. `OnlyNearTown` requires `DistanceMax` from the town centre of at most 9.
4. `CheckIfIndustryTilesAreFree`. Every layout cell must be on the map. A `GFX_WATERTILE_SPECIALCHECK` cell must be flat water. A real cell must have no vehicle and no bridge. Unless slope-refusal bit 5 is set, water tiles are allowed only when `BuiltOnWater` is set, and dry tiles are refused for a water industry. `OnlyInTown` and `Town1200More` require a house tile, cleared as `OWNER_TOWN`. `OnlyNearTown` may clear a house the same way. Other tiles are cleared with `DoCommandFlag::Auto` and without town-rating changes. The helper snapshots `_cleared_object_areas` and restores it after this test.
5. Location rule. Mask `IndustryCallbackMask::Location` calls `CheckIfCallBackAllowsCreation` (`CBID_INDUSTRY_LOCATION`). A failed callback **allows** the site. Otherwise `_check_new_industry_procs[check_proc]` runs:

| `IndustryCheck` | Rule |
| --- | --- |
| `None` | Succeeds |
| `Forest` | Arctic: tile Z at least `HighestSnowLine() + 2` |
| `Refinery` | Outside the editor, within the scaled oil-refinery distance of a map edge |
| `Farm` | Arctic: tile Z + 2 stays below `HighestSnowLine()` |
| `Plantation` | Refuses `TropicZone::Desert` |
| `Water` | Requires `TropicZone::Desert` |
| `Lumbermill` | Requires `TropicZone::Rainforest` |
| `BubbleGen` | Tile Z at most 4 |
| `OilRig` | Sea level, and within the scaled edge distance, unless the editor is ignoring industry restrictions |

`CheckScaledDistanceFromEdge` scales the limit with map size independently on X and Y, and only when that dimension is greater than 256.

6. `CheckIfIndustryTileSlopes`. For each real tile, mask `IndustryTileCallbackMask::ShapeCheck` calls `CBID_INDTILE_SHAPE_CHECK` (`PerformIndustryTileSlopeCheck`). Otherwise `IsSlopeRefused` compares the tile slope to `IndustryTileSpec::slopes_refused`. A refused slope still passes during TerraGenesis world generation when no tile used the shape callback and `_ignore_industry_restrictions` is clear, because the platform may be levelled next.
7. That same TerraGenesis case then requires `CheckIfCanLevelIndustryPlatform`. Custom shape checks skip this flatten test.
8. `Industry::CanAllocateItem` must be true (`STR_ERROR_TOO_MANY_INDUSTRIES`).

On execute, the helper creates the pool item, levels the platform when the built-in slope check was used, and calls `DoCreateNewIndustry`.

## Planting the industry

`DoCreateNewIndustry` (`src/industry_cmd.cpp`):

- Inserts the id into `Industry::industries[type]`.
- Copies produced and accepted cargos from the spec, then optionally randomises rates. See the industries note for slot rules and `CBID_INDUSTRY_INPUT_CARGO_TYPES` / `CBID_INDUSTRY_OUTPUT_CARGO_TYPES`.
- Sets `prod_level` to `PRODLEVEL_DEFAULT`. Mask `ProdChangeBuild` may replace it from `CBID_INDUSTRY_PROD_CHANGE_BUILD` and recompute rates. The result must lie between `PRODLEVEL_MINIMUM` and `PRODLEVEL_MAXIMUM`.
- Sets `construction_date` from the calendar, and `construction_type` from the mode: `ScenarioEditor`, `MapGeneration`, or `Gameplay`. `selected_layout` is the layout index plus one (zero means “built before newindustries”).
- `random_colour` is four random bits, unless mask `DecideColour` and `CBID_INDUSTRY_DECIDE_COLOUR` return a colour.
- During world generation, mask `Production256Ticks` runs `IndustryProductionCallback(ind, 1)` once, copies scaled waiting into last month’s production, and clears waiting. Every valid output then also adds `ScaleByCargoScale(rate * 8)` to last month’s production so a new map shows history immediately.
- For each layout tile that is not the water sentinel: expand `location`, remember the water class, clear the tile, `MakeIndustry(tile, id, gfx, random, water class)`. World generation also sets the construction counter to 3 and the stage to 2. Animated tile specs are registered immediately.
- After the tiles exist, each tile gets `TriggerIndustryTileAnimation_ConstructionStageChanged`.
- `PlantOnBuild` calls `PlantRandomFarmField` fifty times.
- Outside world generation, `PopulateStationsNearby` fills `stations_near`.

`MakeIndustry` (`src/industry_map.h`) writes `TileType::Industry` and:

| Field | Contents |
| --- | --- |
| `m1` | 0 at creation. Bits 0–1 stage, bits 2–3 counter, bit 7 completed |
| `m2` | `IndustryID` |
| `m3` | per-tile random bits |
| `m4` | animation loop, starts at 0 |
| `m5` and `m6` bit 2 | graphics id, 9 bits (`GetCleanIndustryGfx`) |
| `m6` bits 3–5 | random triggers, cleared |
| water class | via `SetWaterClass`; `m6` bits 6–7 cleared |

`GetIndustryConstructionStage` returns `INDUSTRY_COMPLETED` (3) when bit 7 is set, otherwise bits 0–1. `ResetIndustryConstructionStage` clears the low nibble and bit 7, which restarts construction. The production animation uses that reset and then sets the completed bit again so the new sprite is shown as finished.

## Growing to completion

`TileLoop_Industry` runs for every industry tile every 256 ticks (`TILE_UPDATE_FREQUENCY` in `src/landscape.cpp`). While `IsIndustryCompleted` is false, the only work is `MakeIndustryTileBigger`:

- Add one to the 2-bit counter.
- When the counter would become 4, set it to 0, add one to the stage, and fire the construction-stage animation trigger.
- When the stage reaches 3, set the completed bit.

From `MakeIndustry` (stage 0, counter 0) that is four tile-loop visits per stage and three stage steps, 12 visits, 3072 ticks. World generation starts at stage 2, counter 3, so the next visit completes the tile.

A finished tile with `gfx >= NEW_INDUSTRYTILEOFFSET` (175) does not run the original completion switch. The comment there is that new industry tiles are already animated on construction. Original graphics do a few one-shot setups: power-plant chimney smoke, `BuildOilRig` when a completed `GFX_OILRIG_1` has another `GFX_OILRIG_1` of the same industry one tile south, and animation registration for the toy factory, bubble catcher, toffee quarry, and plastic fountain.

Until every tile of the industry is completed, `ChopLumberMillTrees` refuses to cut. Cargo transport also waits for the completed bit on that tile. The editor still advances construction; `OnTick_Industry` and the post-completion half of the tile loop do not produce or move cargo in the editor.

Farms with `PlantFields` keep planting after completion, from the 256-tick production edge (see the industries note). `PlantRandomFarmField` aims at a random tile within about 16 of the industry centre and builds a fenced field rectangle when enough of those tiles are clear or trees.

## Map generation counts

`GetNumberOfIndustries` scales a density table (very low 10, low 25, normal 55, high 80, on a 256×256 map) with `Map::ScaleBySize`, or uses `custom_industry_number` for `IndustryDensity::Custom`, capped by the pool max. The editor uses the very-low row.

`GenerateIndustries` splits land and water. `GetScaledIndustryGenerationProbability` returns 0 when the type’s `BuiltOnWater` bit disagrees with the pass. Otherwise the weight is `appear_creation[landscape]`, times 16, then `Map::ScaleBySize`, except refinery and oil-rig checks which use `Map::ScaleBySize1D`. `CBID_INDUSTRY_PROBABILITY` with creation type `MapGeneration` can replace that chance (GRF version 8 and later: values below `0x100` replace it, `0x100` keeps the default). A type is forced once when the chance stays positive, `NoBuildMapCreation` is clear, and this is not the editor.

Each pass places the forced types first (`try_hard`), then spends the remaining count by weighted random draw. A pass’s count starts as that pass’s probability sum times `GetNumberOfIndustries`, divided by the land-plus-water probability sum. The land pass then applies `Map::ScaleByLandProportion` unless density is custom. If the scaled count is below the number of forced types, the count is raised to that number. After generation, `_industry_builder.Reset()` sets `wanted_inds` from the count actually placed (integer in the top 16 bits).

`CheckIndustries` warns, up to three messages, when a type that should have been forced has a count of zero.

## Gameplay counts

`IndustryTypeBuildData::GetIndustryTypeData` stores `appear_ingame[landscape]` as `probability`, after the same probability callback with creation type `RandomCreation`. The chance is 0 when the type is disabled, has no layout, is `Before1950` after 1950, or is `After1960` before 1960. `min_number` is 1 when `CanCloseLastInstance` is set, otherwise 0.

`SetupTargetCount` gives every buildable type its `min_number`, then deals the rest of `wanted_inds` by weighted random using `probability`. `TryBuildNewIndustry` compares `target_count` with `GetIndustryTypeCount`. Types with `wait_count > 0` are skipped. A missing type whose `min_number` is positive is forced. Otherwise a type is chosen with weight equal to how many are missing. `PlaceIndustry` failure raises `max_wait` (capped at 1000) and sets `wait_count` so the type backs off. Success halves `max_wait` (minimum 1) and calls `AdvertiseIndustryOpening`. Every attempt decrements positive wait counters.

`wanted_inds` itself grows in `IndustryBuildData::EconomyMonthlyLoop`, documented with the monthly timer in the industries note. Density `FundedOnly` leaves both generation and that monthly growth idle, so industries appear only when a player or deity builds one.

## Removal

`ClearTile_Industry` deletes the whole industry, not one tile. Water, the scenario editor, and the magic bulldozer may do it. A company auto-command, and water against `BuiltOnWater` or a tile whose slope-refusal bit 5 is set, cannot. The cost is `GetRemovalCost` (`Price::ClearIndustry` times `removal_cost_multiplier`); the comment on that function says non-GRF industries have a zero multiplier.

`Industry::~Industry` turns each of its industry tiles into water of the same class (`MakeWaterKeepingClass`), deletes an oil-rig station tile, clears farmland industry indexes out to 21 tiles when `PlantFields` is set, releases disaster targets, deletes persistent storage, erases the type set entry, closes the view and production graph, drops subsidies and invalidates the industry's identity in existing sourced cargo packets without deleting their physical units, and removes itself from nearby stations. `PostDestructor` rebuilds the industry directory.

The economy does not call this destructor on the month it announces closure. It sets `prod_level` to `PRODLEVEL_CLOSURE`. The next economy month deletes the object. See the industries note for which types are protected from that announcement.

## Native layered mines and Surface processors

`multilayer_industry.h` resolves only the accepted first-party package's mapped
mine IDs `0x8100..0x8109` and processor IDs `0x8200..0x8206`. Cargo labels resolve
through CargoSpec; translated names, substitute graphics and camera selection
grant no placement or production permission. All ten mine definitions and seven
Surface processor families install after GRF polishing with native scheduling;
their automatic appearance/rates/callbacks are disabled. Final mine/processor
art remains with R2MLVS13.

`CmdBuildLayerMine` takes an explicit actual seam and installed layout. Complete
company/calendar admission, excavated machinery and separate loading floor,
seam contact, existing physical/deposit exclusion, clear/equipment cost, funds,
town and allocation checks finish before binding/claim publication and tile
creation. It preserves original/remaining geological reserve. Generic industry
funding/prospecting cannot open these mines on Surface or bypass Deep's 1980
calendar rule. `CmdBuildIndustry` remains the Surface processor funding boundary.

Native machinery completes on its own physical rock layer. `OnTick_Industry`
earns bounded rational/rate-scaled work and consumes geology into bounded real
`Industry::produced` stock; native tile loops dispatch through exact same-layer
station packet receipts and never run inherited Surface smoke/water/farm effects.
Depletion, full storage, stopped production and unavailable/pause/editor states
retain material. A viewport or same-XY station on another layer changes neither
production nor catchment. Supplied units alone reduce mine stock; rating limits
throughput and retains every unsupplied unit.

`layer_industry_processor.h` owns recipe choice, controller, work and historical
complete-batch counters. The existing Industry accepted/produced vectors own
all physical cargo, including inputs for an alternative that is not selected.
`CmdSelectLayerIndustryRecipe` changes choice through shared company/calendar
admission without moving stocks. The common inventory/commit interfaces require
every input and every product's room before a whole batch commits. Scalar
heat/power cannot commit until the utility producer supplies real atomic grants.
`layer_industry_baseline.h` reuses only compatible original one-to-one Iron
Ore/Steel behavior; it allocates no duplicate receiver or recipe identity.

Stored mine output, processor inputs/products and bounded baseline Steel protect
against demolition/automatic disposal. Abandoned mines stop extraction but can
dispatch existing material to an eligible same-layer station, then empty
machinery can be demolished/rebuilt against the original remaining reserve.
The first valid explicit recipe-selection actor can recover abandoned processor
control. Deity actions do not invent a company controller, and repeated processor
registration cannot reset saved control/work/history. Company changes invalidate
quotes without wrapping an exhausted revision.

Save versions 379/380 append owner-specific binding/choice/work/history tables.
Geology, Industry and existing station/vehicle/packet chunks retain their own
physical state. Final activation validates real definitions, seam/claim/geometry,
exact role sets, frozen batch arithmetic and output/dispatch conservation before
publishing the complete family. Missing rows are refused rather than repaired
with renewed reserves or zeroed histories. See [savegame architecture](../savegame/archi-savegame.md).

The complete useful Coal/Uranium power, native freight/portal workflow, tubes,
typed direct-heat grants, final graphics and all-resource financial acceptance
remain explicitly gated in R2MLVS6; substitute drawings and packet fixtures are
not completion of those producer obligations.

## Invariants

- No industry pool item exists until `CreateNewIndustryHelper` has passed conflict distance, town rules, tile clearance, the location callback or `IndustryCheck` proc, slopes, the optional TerraGenesis platform check, and the pool cap.
- `location` is a bounding box. A tile inside the box belongs to the industry only when its type is industry and `m2` equals this id. Water-check cells never become industry tiles.
- New tiles start at stage 0, counter 0, completed bit clear, except world generation, which plants stage 2, counter 3 so the next tile loop finishes them.
- Completion is bit 7 of `m1`. The stage field reaching 3 and that bit being set happen in the same `MakeIndustryTileBigger` call. Tiles of one industry complete independently.
- Cargo transport and the lumber-mill cut require the completed bit. The editor advances the counter and does not run production.
- `selected_layout` is the chosen layout index plus one.
- Conflicting types are refused at distance 14 or less, measured from the north tile of the existing industry to the new site tile.
- Random gameplay construction uses `appear_ingame`, a per-type wait, and `wanted_inds`. Map generation uses `appear_creation` and does not consult `wanted_inds` until `Reset` copies the resulting count.
- A failed random placement increases that type’s wait. It does not change `prod_level` of any existing industry.
- Clearing one industry tile removes the industry and every tile that still carries its id.
