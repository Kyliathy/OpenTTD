# Town growth, houses, rating, and cargo

Parent: [archi-towns.md](archi-towns.md). Formulas below are the functions as they stand in the tree. Line numbers refer to that source.

## Who decides, who builds

| Step | When | Function | File |
| --- | --- | --- | --- |
| Permission and rate | Economy month, `TimerGameEconomy::Priority::Town` | `UpdateTownGrowth`, `UpdateTownRating` | `src/town_cmd.cpp` |
| One growth attempt | Game tick, skipped when `_game_mode == GameMode::Editor` | `OnTick_Town` → `TownTickHandler` → `GrowTown` | `src/town_cmd.cpp`, called from `src/landscape.cpp` |
| Editor / import / deity burst | `Commands::ExpandTown` | `CmdExpandTown` | `src/town_cmd.cpp` |
| House construction advance, passenger/mail production, rebuild | House tile loop | `TileLoop_Town` | `src/town_cmd.cpp` |
| House age | Economy year | `_economy_towns_yearly` → `IncrementHouseAge` | `src/town_cmd.cpp` |

`TownFlag::IsGrowing` means the monthly check passed. `grow_counter` counts down toward the next `GrowTown` call. `growth_rate` is how far the counter resets after a successful growth.

## Monthly decision

`UpdateTownGrowth` clears the flag, then sets it only if every gate below lets it through.

```3931:3967:src/town_cmd.cpp
static void UpdateTownGrowth(Town *t)
{
	UpdateTownGrowthRate(t);

	t->flags.Reset(TownFlag::IsGrowing);
	SetWindowDirty(WindowClass::TownView, t->index);

	if (_settings_game.economy.town_growth_rate == 0 && t->fund_buildings_months == 0) return;

	if (t->fund_buildings_months == 0) {
		/* Check if all goals are reached for this town to grow (given we are not funding it) */
		for (TownAcceptanceEffect i : EnumRange(TownAcceptanceEffect::End)) {
			switch (t->goal[i]) {
				case TOWN_GROWTH_WINTER:
					if (TileHeight(t->xy) >= GetSnowLine() && t->received[i].old_act == 0 && t->cache.population > 90) return;
					break;
				case TOWN_GROWTH_DESERT:
					if (GetTropicZone(t->xy) == TropicZone::Desert && t->received[i].old_act == 0 && t->cache.population > 60) return;
					break;
				default:
					if (t->goal[i] > t->received[i].old_act) return;
					break;
			}
		}
	}

	if (t->flags.Test(TownFlag::CustomGrowth)) {
		if (t->growth_rate != TOWN_GROWTH_RATE_NONE) t->flags.Set(TownFlag::IsGrowing);
		SetWindowDirty(WindowClass::TownView, t->index);
		return;
	}

	if (t->fund_buildings_months == 0 && CountActiveStations(t) == 0 && !Chance16(1, 12)) return;

	t->flags.Set(TownFlag::IsGrowing);
	SetWindowDirty(WindowClass::TownView, t->index);
}
```

`Chance16(1, 12)` is the shared helper in `src/core/random_func.hpp` (`Chance16I`: `(((uint16_t)r * b + b / 2) >> 16) < a`). The growth code does not spell a separate probability.

An active station, for both this chance and the rating step, is a station whose sign is inside the town-edge squared radius and whose `time_since_load` or `time_since_unload` is `<= 20` (`CountActiveStations`, `ForAllStationsNearTown`).

Goals are initialised in `DoCreateTown`. The array starts zeroed. Only these assignments run:

```2025:2038:src/town_cmd.cpp
	/* Set the default cargo requirement for town growth */
	switch (_settings_game.game_creation.landscape) {
		case LandscapeType::Arctic:
			if (FindFirstCargoWithTownAcceptanceEffect(TownAcceptanceEffect::Food) != nullptr) t->goal[TownAcceptanceEffect::Food] = TOWN_GROWTH_WINTER;
			break;

		case LandscapeType::Tropic:
			if (FindFirstCargoWithTownAcceptanceEffect(TownAcceptanceEffect::Food) != nullptr) t->goal[TownAcceptanceEffect::Food] = TOWN_GROWTH_DESERT;
			if (FindFirstCargoWithTownAcceptanceEffect(TownAcceptanceEffect::Water) != nullptr) t->goal[TownAcceptanceEffect::Water] = TOWN_GROWTH_DESERT;
			break;

		default:
			break;
	}
```

`TOWN_GROWTH_WINTER` is `0xFFFFFFFE` and `TOWN_GROWTH_DESERT` is `0xFFFFFFFF` (`src/town.h`). The comments on those constants say the town needs the cargo in winter, or on desert, in any amount. The switch above implements “any amount” as `old_act == 0`, and only when population is above 90 (winter) or 60 (desert) and the centre tile is in that band.

`CmdTownCargoGoal` (deity only) overwrites one `goal` entry and calls `UpdateTownGrowth` again.

## Growth rate

`UpdateTownGrowthRate` does nothing when `TownFlag::CustomGrowth` is set. Otherwise it stores `GetNormalGrowthRate` and rescales `grow_counter` through `UpdateTownGrowCounter`.

```3891:3911:src/town_cmd.cpp
static uint GetNormalGrowthRate(Town *t)
{
	/**
	 * Note:
	 * Unserviced+unfunded towns get an additional malus in UpdateTownGrowth(),
	 * so the "320" is actually not better than the "420".
	 */
	static const uint16_t _grow_count_values[2][6] = {
		{ 120, 120, 120, 100,  80,  60 }, // Fund new buildings has been activated
		{ 320, 420, 300, 220, 160, 100 }  // Normal values
	};

	int n = CountActiveStations(t);
	uint16_t m = _grow_count_values[t->fund_buildings_months != 0 ? 0 : 1][std::min(n, 5)];

	uint growth_multiplier = _settings_game.economy.town_growth_rate != 0 ? _settings_game.economy.town_growth_rate - 1 : 1;

	m >>= growth_multiplier;
	if (t->larger_town) m /= 2;

	return TownTicksToGameTicks(m / (t->cache.num_houses / 50 + 1));
}
```

The function comment says a growth speed of none (`0`) uses the same table column as normal (`2`), because the multiplier becomes `1` when the setting is `0`. The early return in `UpdateTownGrowth` is what actually stops growth in that setting.

Conversion into the counter’s tick domain:

```394:400:src/town.h
/*
 * Converts original town ticks counters to plain game ticks. Note that
 * tick 0 is a valid tick so actual amount is one more than the counter value.
 */
inline uint16_t TownTicksToGameTicks(uint16_t ticks)
{
	return (std::min(ticks, MAX_TOWN_GROWTH_TICKS) + 1) * Ticks::TOWN_GROWTH_TICKS - 1;
}
```

`MAX_TOWN_GROWTH_TICKS` is 930 (`src/town.h`). `Ticks::TOWN_GROWTH_TICKS` is 70 (`src/timer/timer_game_tick.h`). `Ticks::DAY_TICKS` is 74. The town window prints `RoundDivSU(growth_rate + 1, Ticks::DAY_TICKS)` as the “grows every” figure (`src/town_gui.cpp`).

`CmdTownGrowthRate` (deity) treats `0` as “clear `TownFlag::CustomGrowth`”. Any other value, including `TOWN_GROWTH_RATE_NONE` (`0xFFFF`), is stored as `growth_rate` and sets `TownFlag::CustomGrowth`. The script wrapper `ScriptTown::SetGrowthRate` (`src/script/api/script_town.cpp`) maps a day count with `std::max<SQInteger>(days_between_town_growth * ::Ticks::DAY_TICKS, 2u) - 1` before calling that command. A value of `TOWN_GROWTH_NORMAL` sends `0` (clear custom growth). `TOWN_GROWTH_NONE` sends `TOWN_GROWTH_RATE_NONE`.

On founding, `DoCreateTown` sets `growth_rate = TownTicksToGameTicks(250)` and `grow_counter = t->index % Ticks::TOWN_GROWTH_TICKS`, then `UpdateTownGrowthRate` replaces the rate.

## Tick placement

```888:913:src/town_cmd.cpp
static void TownTickHandler(Town *t)
{
	if (t->flags.Test(TownFlag::IsGrowing)) {
		TownExpandModes modes{TownExpandMode::Buildings};
		if (_settings_game.economy.allow_town_roads) modes.Set(TownExpandMode::Roads);
		int i = (int)t->grow_counter - 1;
		if (i < 0) {
			if (GrowTown(t, modes)) {
				i = t->growth_rate;
			} else {
				/* If growth failed wait a bit before retrying */
				i = std::min<uint16_t>(t->growth_rate, Ticks::TOWN_GROWTH_TICKS - 1);
			}
		}
		t->grow_counter = i;
	}
}

/** Iterate through all towns and call their tick handler. */
void OnTick_Town()
{
	if (_game_mode == GameMode::Editor) return;

	for (Town *t : Town::Iterate()) {
		TownTickHandler(t);
	}
}
```

`GrowTown` walks a fixed offset list `_town_coord_mod` from the town centre until it finds road bits, then calls `GrowTownAtRoad`. If no road exists and roads are allowed, it clears one flat non-house tile in that same offset list and builds a random road. `GrowTownAtRoad` limits the walk with:

```1778:1790:src/town_cmd.cpp
	switch (t->layout) {
		case TownLayout::BetterRoads:
			iterations = 10 + t->cache.num_houses * 2 / 9;
			break;

		case TownLayout::Grid3x3:
		case TownLayout::Grid2x2:
			iterations = 10 + t->cache.num_houses * 1 / 9;
			break;

		default:
			iterations = 10 + t->cache.num_houses * 4 / 9;
			break;
	}
```

`GrowTownInTile` returns `TownGrowthResult::Succeed`, `SearchStopped`, or `Continue`. Along the way it may call `GrowTownWithExtraHouse` (needs at least three orthogonal neighbours that are houses or void, then `TryBuildTownHouse`), `GrowTownWithRoad`, `GrowTownWithBridge`, or `GrowTownWithTunnel`. Grid layouts also refuse house tiles that sit on the road lattice (`TownLayoutAllowsHouseHere`).

`CmdExpandTown` is the editor and deity burst. `grow_amount == 0` temporarily adds `RandomRange(num_houses / 10) + 3` to `num_houses`, calls `GrowTown` that many times `* 10`, then subtracts the temporary count. A positive `grow_amount` calls `GrowTown` up to 25 times per house it is trying to add. Import uses the positive path with 10 (`HOUSES_TO_GROW` in `LoadTownData`).

Fund-buildings (`TownActionFundBuildings`) sets `fund_buildings_months = 3`, calls `UpdateTownGrowth`, and pulls `grow_counter` forward so funding cannot beat one house per `2 * Ticks::TOWN_GROWTH_TICKS` ticks. The monthly timer decrements `fund_buildings_months`.

## Houses as tiles

`BuildTownHouse` bumps `cache.num_houses`, then either leaves the building under construction or completes it. Completion (and the population add) is immediate when `_generating_world` or the editor is active, except a 1-in-7 `Chance16` during world generation that leaves a non-historical house mid-build:

```2787:2794:src/town_cmd.cpp
	if (_generating_world || _game_mode == GameMode::Editor || house_completed) {
		uint32_t construction_random = Random();

		construction_stage = TOWN_HOUSE_COMPLETED;
		if (_generating_world && !hs->extra_flags.Test(HouseExtraFlag::BuildingIsHistorical) && Chance16(1, 7)) construction_stage = GB(construction_random, 0, 2);

		if (construction_stage == TOWN_HOUSE_COMPLETED) {
			ChangePopulation(t, hs->population);
```

`MakeTownHouse` writes every tile of a multi-tile house via `MakeHouseTile`. The northern tile keeps the requested `HouseID`; Y, X, and the fourth tile receive `++type`. `TileLoop_Town` advances incomplete houses with `IncHouseConstructionTick`. When the stage wraps to `TOWN_HOUSE_COMPLETED` (3), `AdvanceSingleHouseConstruction` adds `HouseSpec::population` and `ResetHouseAge` zeroes `m5`.

`TryBuildTownHouse` builds a candidate list from `HouseSpec::Specs()` whose `building_availability` covers the town zone and the climate bit, skips disabled specs and specs that are NewGRF overrides, and respects per-class counts on `TownCache::building_counts`. Historical houses (`HouseExtraFlag::BuildingIsHistorical`) are rejected when not generating a world and not in the editor.

Player protection is map bit `m3` bit 5 (`IsHousePlayerProtected`). `HouseExtraFlag::BuildingIsProtected` also stops the town from upgrading the building. Clearing a house charges `HouseSpec::GetRemovalCost` (`(_price[Price::ClearHouse] * removal_cost) >> 8`) and calls `ChangeTownRating(t, -remove_rating_decrease, RATING_HOUSE_MINIMUM, flags)`.

## Zones

While `num_houses` is below `23 * 4`, `UpdateTownRadius` copies a row from `_town_squared_town_zone_radius_data[num_houses / 4]`. Above that it computes:

```1954:1966:src/town_cmd.cpp
	if (t->cache.num_houses < std::size(_town_squared_town_zone_radius_data) * 4) {
		t->cache.squared_town_zone_radius = _town_squared_town_zone_radius_data[t->cache.num_houses / 4];
	} else {
		int mass = t->cache.num_houses / 8;
		/* Actually we are proportional to sqrt() but that's right because we are covering an area.
		 * The offsets are to make sure the radii do not decrease in size when going from the table
		 * to the calculated value.*/
		t->cache.squared_town_zone_radius[to_underlying(HouseZone::TownEdge)] = mass * 15 - 40;
		t->cache.squared_town_zone_radius[to_underlying(HouseZone::TownOutskirt)] = mass * 9 - 15;
		t->cache.squared_town_zone_radius[to_underlying(HouseZone::TownOuterSuburb)] = 0;
		t->cache.squared_town_zone_radius[to_underlying(HouseZone::TownInnerSuburb)] = mass * 5 - 5;
		t->cache.squared_town_zone_radius[to_underlying(HouseZone::TownCentre)] = mass * 3 + 5;
	}
```

`GetTownRadiusGroup` starts at `HouseZone::TownEdge` and tightens to any zone whose squared radius is strictly greater than `DistanceSquare(tile, t->xy)`. During a fund-buildings period, a tile with squared distance `<= 25` is forced to `HouseZone::TownCentre`.

## Local authority rating

`DoCreateTown` fills every company slot with `RATING_INITIAL` (500) and clears `have_ratings`. The monthly update is:

```3821:3847:src/town_cmd.cpp
static void UpdateTownRating(Town *t)
{
	/* Increase company ratings if they're low */
	for (const Company *c : Company::Iterate()) {
		if (t->ratings[c->index] < RATING_GROWTH_MAXIMUM) {
			t->ratings[c->index] = std::min((int)RATING_GROWTH_MAXIMUM, t->ratings[c->index] + RATING_GROWTH_UP_STEP);
		}
	}

	ForAllStationsNearTown(t, [&](const Station *st) {
		if (st->time_since_load <= 20 || st->time_since_unload <= 20) {
			if (Company::IsValidID(st->owner)) {
				int new_rating = t->ratings[st->owner] + RATING_STATION_UP_STEP;
				t->ratings[st->owner] = std::min<int>(new_rating, INT16_MAX); // do not let it overflow
			}
		} else {
			if (Company::IsValidID(st->owner)) {
				int new_rating = t->ratings[st->owner] + RATING_STATION_DOWN_STEP;
				t->ratings[st->owner] = std::max(new_rating, INT16_MIN);
			}
		}
	});

	/* clamp all ratings to valid values */
	for (auto it = t->ratings.begin(); it != t->ratings.end(); ++it) {
		*it = Clamp(*it, RATING_MINIMUM, RATING_MAXIMUM);
	}
```

Constants (`src/town_type.h`): `RATING_GROWTH_UP_STEP` is 5, `RATING_GROWTH_MAXIMUM` is `RATING_MEDIOCRE` (200), `RATING_STATION_UP_STEP` is 12, `RATING_STATION_DOWN_STEP` is −15. The comment on `RATING_GROWTH_UP_STEP` says the bump happens “when a town grows”. The call site is this monthly function, which runs for every town after `UpdateTownGrowth`, whether or not `GrowTown` succeeded.

`ChangeTownRating` adds `add` toward `max` (a floor when `add < 0`, a ceiling when `add > 0`) for `_current_company`, and sets `have_ratings`. It returns without writing when the company is invalid, `DoCommandFlag::NoModifyTownRating` is set, or the magic-bulldozer cheat is on and `add` is negative.

`CheckforTownRating` looks up `town_council_tolerance` in a table of road-removal and tunnel/bridge-removal minima (`RATING_ROAD_NEEDED_*`, `RATING_TUNNEL_BRIDGE_NEEDED_*`). Permissive uses `RATING_MINIMUM`, so the check always passes. `CheckIfAuthorityAllowsNewStation` uses a fixed bar of `RATING_VERYPOOR` (−200) inside `economy.dist_local_authority`, and also returns success immediately when the attitude is permissive.

Other company actions (`CmdDoTownAction`): advertise (station rating, not town rating), road rebuild for 6 months, a statue, fund buildings for 3 months, exclusive rights for 12 months, and bribe (`RATING_BRIBE_UP_STEP` 200 up to `RATING_BRIBE_MAXIMUM` 800, or on the 1-in-14 failure `RATING_BRIBE_DOWN_TO` −50 and `unwanted` for 6 months).

`Town::MaxTownNoise` is `population / town_noise_population[tolerance] + 3`, or 0 when population is 0.

## Cargo delivery and production

Delivery path, `DeliverGoods` (`src/economy.cpp` 1107–1112, quoted in the parent page): `accepted_total` is added to `st->town->received[town_acceptance_effect].new_act`. Units that were not taken by an industry are also added to `Town::accepted` for that cargo type. The effect comes from `CargoSpec::town_acceptance_effect`. Default cargos in `src/table/cargo_const.h` map passengers, mail, goods, food, and water onto the matching effect; most other cargos use `TownAcceptanceEffect::None`.

The month timer rotates history and then:

```4211:4214:src/town_cmd.cpp
		for (auto &received : t->received) received.NewMonth();

		UpdateTownGrowth(t);
		UpdateTownRating(t);
```

`NewMonth` sets `old_act = new_act` and clears `new_act` (`src/town_type.h`). Growth therefore sees last month’s deliveries.

Production is per completed house, on `TileLoop_Town`. Original mode (`TownCargoGenMode::Original`):

```542:551:src/town_cmd.cpp
static void TownGenerateCargoOriginal(Town *t, TownProductionEffect tpe, uint8_t rate, StationFinder &stations)
{
	for (const CargoSpec *cs : CargoSpec::town_production_cargoes[tpe]) {
		uint32_t r = Random();
		if (GB(r, 0, 8) < rate) {
			CargoType cargo_type = cs->Index();
			uint amt = (GB(r, 0, 8) * cs->town_production_multiplier / TOWN_PRODUCTION_DIVISOR) / 8 + 1;

			TownGenerateCargo(t, cargo_type, amt, stations, true);
		}
	}
}
```

`rate` is `HouseSpec::population` for passengers and `HouseSpec::mail_generation` for mail. `TOWN_PRODUCTION_DIVISOR` is 256 (`src/cargotype.h`). `TownGenerateCargo` halves `amount` in a recession when `affected_by_recession` is true, then `ScaleByCargoScale`, then adds to `supplied.history[THIS_MONTH]` and `MoveGoodsToStation`. NewGRF callback cargo passes `affected_by_recession = false`.

`UpdateTownMaxPass` overwrites last-month production statistics from population. It does not feed `GrowTown`:

```1973:1988:src/town_cmd.cpp
void UpdateTownMaxPass(Town *t)
{
	for (const CargoSpec *cs : CargoSpec::town_production_cargoes[TownProductionEffect::Passengers]) {
		uint32_t production = ScaleByCargoScale(t->cache.population >> 3, true);
		if (production == 0) continue;

		auto &supplied = t->GetOrCreateCargoSupplied(cs->Index());
		supplied.history[LAST_MONTH].production = production;
	}

	for (const CargoSpec *cs : CargoSpec::town_production_cargoes[TownProductionEffect::Mail]) {
		uint32_t production = ScaleByCargoScale(t->cache.population >> 4, true);
		if (production == 0) continue;

		auto &supplied = t->GetOrCreateCargoSupplied(cs->Index());
		supplied.history[LAST_MONTH].production = production;
	}
}
```

Percent transported for the town view is `SuppliedHistory::PctTransported`: `transported * 256 / production` clamped to `uint8_t`, or 0 when production is 0.
