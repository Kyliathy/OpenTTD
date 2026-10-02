# Importing external town data

Parent: [archi-towns.md](archi-towns.md). This is the path that implements `docs/importing_town_data.md`. The player document’s stale sentences are listed on the parent page.

## From the Found Town window to `LoadTownData`

The editor Found Town window (`src/town_gui.cpp`) builds `WID_TF_LOAD_FROM_FILE` inside the stacked widget `WID_TF_TOWN_ACTION_SEL`. `FoundTownWindow::OnInit` hides that stack when `_game_mode` is not `GameMode::Editor`. Choosing a road-layout button in the editor copies it onto `_settings_game.economy.town_layout`, and the import uses that setting.

The click opens `ShowSaveLoadDialog(AbstractFileType::TownData, SaveLoadOperation::Load)`. `FiosGetTownDataList` (`src/fios.cpp`) lists files whose extension compares equal to `.json` ignoring case, starting at `FioFindDirectory(Subdirectory::Heightmap)`. That subdirectory’s relative name is `scenario/heightmap/` (`src/fileio.cpp`). Selecting a file stores it in `_file_to_saveload` and calls `LoadTownData` (`src/fios_gui.cpp`).

`LoadTownData` (`src/genworld.cpp`) reads the whole file as binary text. A missing file or a short read shows `STR_TOWN_DATA_ERROR_JSON_FORMATTED_INCORRECTLY` and returns. That is the same string used for a JSON parse failure.

## JSON the parser accepts

`ParseTownData` parses with `nlohmann::json`. The root must be an array. Each element must be an object with:

| Field | Check | Stored as |
| --- | --- | --- |
| `name` | `is_string()` | `ParsedTown::name` (`std::string`) |
| `population` | `is_number()` | `ParsedTown::population` (`uint`) via `get_to` |
| `city` | `is_boolean()` | `ParsedTown::is_city` |
| `x`, `y` | `is_number()` | Passed as `float` into `TranslateCoordinates` |

A bad `name`, a non-object element, or a non-array root uses the generic JSON error and returns `{}`. A bad population, city, or coordinate names the town in `STR_TOWN_DATA_ERROR_TOWN_FORMATTED_INCORRECTLY` or `STR_TOWN_DATA_ERROR_BAD_COORDINATE` and also returns `{}`. Anything already parsed is discarded. No town is founded.

`ParsedTown`’s comments say a blank population defaults to 0 and a blank city defaults to false. The parser does not implement those defaults; the keys are required.

`get_to` on the `uint` population uses nlohmann’s arithmetic `from_json`, which `static_cast`s a JSON float to the destination type (`src/3rdparty/nlohmann/json.hpp`, the `number_float` arm of `from_json` for arithmetic types). The sample value `217.16` in `docs/importing_town_data.md` is therefore a legal number and becomes an integer target. Extra JSON keys are not read.

An empty array parses, then `LoadTownData` returns because `town_data.empty()` is true. No error dialog.

## Coordinates

```366:383:src/genworld.cpp
static TileIndex TranslateCoordinates(float x, float y)
{
	if (x <= 0.0f || y <= 0.0f || x >= 1.0f || y >= 1.0f) return INVALID_TILE;

	/* Determine the target tile.. */
	switch (_settings_game.game_creation.heightmap_rotation) {
		case HM_CLOCKWISE:
			/* Tile coordinates align with what we expect. */
			return TileXY(x * Map::MaxX(), y * Map::MaxY());

		case HM_COUNTER_CLOCKWISE:
			/* Tile coordinates are rotated and must be adjusted. */
			return TileXY((1 - y) * Map::MaxX(), x * Map::MaxY());

		default:
			NOT_REACHED();
	}
}
```

`Map::MaxX()` is `Map::SizeX() - 1` and `Map::MaxY()` is `Map::SizeY() - 1` (`src/map_func.h`). `TileXY` takes `uint` arguments (`src/map_func.h`), so the `float` product is truncated toward zero. Clockwise mode does not swap axes; the file is expected to already be in tile-X / tile-Y proportions. Counter-clockwise mode is the only branch that rewrites the pair. `INVALID_TILE` fails the whole file.

## Founding

`LoadTownData` sets `_generating_world` for the duration and pauses nearest-road updates with `UpdateNearestTownForRoadTiles(true)`.

Each record calls:

```486:496:src/genworld.cpp
		const Town *t = TryGenerateNamedTownAroundTile(town.target_tile, TownSize::Small, town.is_city, _settings_game.economy.town_layout, town.name);

		/* If we still fail to found the town, we'll create a sign at the intended location and tell the player how many towns we failed to create in an error message.
		 * This allows the player to diagnose a heightmap misalignment, if towns end up in the sea, or place towns manually, if in rough terrain. */
		if (t == nullptr) {
			Command<Commands::PlaceSign>::Post(town.target_tile, town.name);
			failed_towns++;
			continue;
		}

		towns.emplace_back(t, town.population);
```

`TryGenerateNamedTownAroundTile` (`src/town_cmd.cpp`) returns null when the pool cannot allocate, the name is empty, `Utf8StringLength(name) >= MAX_LENGTH_TOWN_NAME_CHARS` (32), or `IsUniqueTownName` fails. `IsUniqueTownName` compares `Town::name` only, so a generated town with an empty custom name does not block the same string.

Otherwise it walks:

```2492:2504:src/town_cmd.cpp
	/* Try founding on the target tile, and if that doesn't work, find the nearest suitable tile up to 16 tiles away.
	 * The target might be on water, blocked somehow, or on a steep slope that can't be terraformed by the founding command. */
	for (TileIndex tile : SpiralTileSequence(target_tile, 16, 0, 0)) {
		CommandCost ret = TownCanBePlacedHere(tile, false);
		if (ret.Failed()) continue;

		Town *t = Town::Create(tile);
		t->name = name;
		DoCreateTown(t, tile, 0, size, city, layout, true);

		return t;
	}
```

`SpiralTileSequence(TileIndex, uint radius, uint w, uint h)` is the radius constructor (`src/tilearea_spiral.h`). With `w == 0` and `h == 0` the iterator’s first coordinate is `TileX(start_north) + 1`, `TileY(start_north)` (`src/tilearea_spiral.cpp`). `IsEnd` is `cur_radius == max_radius && dir != DiagDirection::Invalid`, and `max_radius` is 16, so the walk stops when the radius counter reaches 16.

`TownCanBePlacedHere(tile, false)` (`src/town_cmd.cpp`) rejects:

- `DistanceFromEdge(tile) < 12`
- `IsCloseToTown(tile, economy.town_min_distance)`, which is Manhattan distance to the nearest k-d-tree town strictly less than that setting
- a tile that is not clear or trees, or is not flat

The surrounding-tile check (`check_surrounding`) is off for this call. The first tile that passes is the town centre. `DoCreateTown` runs with `manual = true` and `TownSize::Small`. `to_underlying(TownSize::Small)` is 0, so the founding seed is `x = 3` before the city multiplier. In the editor a city multiplies that seed by `economy.initial_city_size`:

```2060:2063:src/town_cmd.cpp
	int x = to_underlying(size) * 16 + 3;
	if (size == TownSize::Random) x = (Random() & 0xF) + 8;
	/* Don't create huge cities when founding town in-game */
	if (city && (!manual || _game_mode == GameMode::Editor)) x *= _settings_game.economy.initial_city_size;
```

`larger_town` is set from `city`. The custom name is assigned before `DoCreateTown`. `townnameparts` stays 0. Houses built while `_generating_world` is set follow the world-generation completion rule in `BuildTownHouse` (see [town-growth.md](town-growth.md)).

After the loop, a non-zero `failed_towns` shows `STR_TOWN_DATA_ERROR_FAILED_TO_FOUND_TOWN`. Towns that did found are kept.

## Growing to the target population

```504:522:src/genworld.cpp
	/* Now that we've created the towns, let's grow them to their target populations. */
	for (const auto &[t, population] : towns) {
		/* Grid towns can grow almost forever, but the town growth algorithm gets less and less efficient as it wanders roads randomly,
		 * so we set an arbitrary limit. With a flat map and a 3x3 grid layout this results in about 4900 houses, or 2800 houses with "Better roads." */
		int try_limit = 1000;

		/* If a town repeatedly fails to grow, continuing to try only wastes time. */
		int fail_limit = 10;

		/* Grow by a constant number of houses each time, instead of growth based on current town size.
		 * We want our try limit to apply in a predictable way, no matter the road layout and other geography. */
		const int HOUSES_TO_GROW = 10;

		do {
			uint before = t->cache.num_houses;
			Command<Commands::ExpandTown>::Post(t->index, HOUSES_TO_GROW, {TownExpandMode::Buildings, TownExpandMode::Roads});
			if (t->cache.num_houses <= before) fail_limit--;
		} while (fail_limit > 0 && try_limit-- > 0 && t->cache.population < population);
	}
```

The 4900 / 2800 figures are that comment, not a second formula in the function.

`Command::Post` runs `Execute` before it returns (`src/command_func.h`), so the house-count check sees the command’s result in this call. `CmdExpandTown` allows the editor, or a deity company outside the editor. Each post asks for 10 houses; each of those tries `GrowTown` up to 25 times (`CmdExpandTown`, positive `grow_amount` branch). The `do`/`while` always posts once. It stops when population is not strictly below the target, when 1000 attempts have been counted by `try_limit--`, or when 10 attempts in a row fail to increase `cache.num_houses`.

`cache.population` is the sum of completed houses’ `HouseSpec::population`, so the JSON number is not written onto the town. A batch of 10 houses can step past the target. The comparison uses population, not `num_houses`.

`CmdExpandTown` still updates radius and `UpdateTownMaxPass` after the burst. It does not consult cargo goals. Monthly `IsGrowing` behaviour starts only after the scenario is played, because `OnTick_Town` returns in the editor.
