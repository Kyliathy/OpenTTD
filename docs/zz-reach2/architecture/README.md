# Architecture notes

These pages describe OpenTTD's subsystems for someone who is going to change them. Each directory is one subsystem. The primary page is `archi-<name>.md`. Companion pages sit beside it when a mechanism is large enough to stand alone.

Start with [@archi-ttd-level0.md](@archi-ttd-level0.md) for the game overview, the current four-layer fork and links to every architecture file. The [map-layer page](map-layers/archi-map-layers.md) records cross-subsystem changes made after the original inventory.

The older notes in `docs/` (savegame format, landscape HTML, multiplayer, link graph, and the rest) are the starting point. These pages keep the facts that still match the tree and replace the ones that do not, with the current functions, types, and file paths. `initial-inventory.md` is the short map of the whole program: build, patterns, and where each layer lives.

## Subsystems

| Directory | Start here | Also in that directory | Older note |
| --- | --- | --- | --- |
| `savegame/` | [archi-savegame.md](savegame/archi-savegame.md) | | `docs/savegame_format.md` |
| `landscape/` | [archi-landscape.md](landscape/archi-landscape.md) | Per-tile notes: clear, rail, road, water, house, station, industry, tree, tunnelbridge, object | `docs/landscape.html`, `landscape_grid.html`, `landscape_externals.html` |
| `map-layers/` | [archi-map-layers.md](map-layers/archi-map-layers.md) | | |
| `desync/` | [archi-desync.md](desync/archi-desync.md) | | `docs/desync.md`, `docs/debugging_desyncs.md` |
| `game-coordinator/` | [archi-game-coordinator.md](game-coordinator/archi-game-coordinator.md) | [archi-stun-turn.md](game-coordinator/archi-stun-turn.md) | `docs/game_coordinator.md` |
| `linkgraph/` | [archi-linkgraph.md](linkgraph/archi-linkgraph.md) | [jobs.md](linkgraph/jobs.md), [demands.md](linkgraph/demands.md), [solver.md](linkgraph/solver.md) | `docs/linkgraph.md` |
| `towns/` | [archi-towns.md](towns/archi-towns.md) | [town-growth.md](towns/town-growth.md), [town-import.md](towns/town-import.md) | `docs/importing_town_data.md` |
| `basesets/` | [archi-basesets.md](basesets/archi-basesets.md) | [obg.md](basesets/obg.md), [obs.md](basesets/obs.md), [obm.md](basesets/obm.md) | `docs/obg_format.txt`, `obs_format.txt`, `obm_format.txt` |
| `files/` | [archi-files.md](files/archi-files.md) | | `docs/directory_structure.md` |
| `commands/` | [archi-commands.md](commands/archi-commands.md) | | |
| `game-loop/` | [archi-game-loop.md](game-loop/archi-game-loop.md) | | |
| `economy/` | [archi-economy.md](economy/archi-economy.md) | [archi-cargo-payment.md](economy/archi-cargo-payment.md), [archi-subsidies.md](economy/archi-subsidies.md) | |
| `industries/` | [archi-industries.md](industries/archi-industries.md) | [archi-industry-construction.md](industries/archi-industry-construction.md) | |
| `pathfinder/` | [archi-pathfinder.md](pathfinder/archi-pathfinder.md) | Rail, road, ship, and water-region notes | |
| `newgrf/` | [archi-newgrf.md](newgrf/archi-newgrf.md) | [features.md](newgrf/features.md), [callbacks.md](newgrf/callbacks.md), [sprite-groups.md](newgrf/sprite-groups.md) | |
| `gui/` | [archi-gui.md](gui/archi-gui.md) | [archi-viewport.md](gui/archi-viewport.md), [archi-blitter.md](gui/archi-blitter.md) | |
| `scripting/` | [archi-scripting.md](scripting/archi-scripting.md) | [archi-script-instances.md](scripting/archi-script-instances.md) | |
| `audio/` | [archi-audio.md](audio/archi-audio.md) | [archi-audio-effects-and-music.md](audio/archi-audio-effects-and-music.md) | |
| `release/` | [archi-release.md](release/archi-release.md) | [archi-symbol-server.md](release/archi-symbol-server.md) | `docs/releasing_openttd.md`, `docs/symbol_server.md` |
| `admin-network/` | [archi-admin-network.md](admin-network/archi-admin-network.md) | [packet-catalogue.md](admin-network/packet-catalogue.md) | `docs/admin_network.md` |
| `logging/` | [archi-logging.md](logging/archi-logging.md) | | `docs/logging_and_performance_metrics.md` |
| `vehicles/` | [archi-vehicles.md](vehicles/archi-vehicles.md) | [trains.md](vehicles/trains.md), [road-vehicles.md](vehicles/road-vehicles.md), [ships.md](vehicles/ships.md), [aircraft.md](vehicles/aircraft.md) | |

## Still being written

These subsystems do not have a directory yet: multiplayer, companies, strings, fonts and stations. Their older notes include `docs/multiplayer.md`, `docs/company_colour_indexes.html`, `docs/eints.md`, `docs/compiling_lang_files.md` and `docs/fonts.md`.
