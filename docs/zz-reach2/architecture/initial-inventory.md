# OpenTTD initial inventory

Findings from the source tree at the parent of `zz-reach2`. Primary sources: `README.md`, `COMPILING.md`, `CODINGSTYLE.md`, `CONTRIBUTING.md`, the root `CMakeLists.txt`, `cmake/Options.cmake`, `vcpkg.json`, and the `src/` layout.

Project identity from the README and CMake: OpenTTD is a transport simulation based on Chris Sawyer's Transport Tycoon Deluxe. The CMake project is version **16.0**, binary name `openttd` unless `BINARY_NAME` is overridden. License is GPL-2.0, with vendored third-party code under other licenses (listed in README section 3.0).

## How it is built

The build system is **CMake**. In-source builds are rejected. Default configuration is **Debug** with assertions on. Language is **C++20** with compiler extensions off (`CMAKE_CXX_STANDARD 20`, `CMAKE_CXX_EXTENSIONS NO`). Any compiler CMake supports that implements C++20 is expected to work. `COMPILING.md` says CMake 3.16 is the minimum; the root `CMakeLists.txt` requires **3.17**.

Typical out-of-source build:

```
mkdir build
cd build
cmake ..
make
```

Release-style build: `-DCMAKE_BUILD_TYPE=RelWithDebInfo`.

### Windows

- Microsoft Visual Studio 2022 or newer (Community is fine), plus the Platform SDK (`Visual C++ MFC for x86 and x64`).
- Dependencies come from **vcpkg**, preferably the static triplets `x64-windows-static` and `x86-windows-static`.
- The folder can be opened as a CMake project, or a Visual Studio solution can be generated:

```
cmake.exe .. -G"Visual Studio 17 2022" -DCMAKE_TOOLCHAIN_FILE="<vcpkg>\scripts\buildsystems\vcpkg.cmake" -DVCPKG_TARGET_TRIPLET="x64-windows-static"
```

Static vcpkg triplets switch the MSVC runtime to `/MT` (`MultiThreaded`). Triplets ending in `-static-md` stay on `/MD`.

### CMake products

| Target | Role |
| --- | --- |
| `openttd_lib` | `OBJECT` library holding almost all game code, plus generated `rev.cpp` (and `ottdres.rc` on Windows) |
| `openttd` | The game executable (`WIN32` subsystem). Output name is `BINARY_NAME`. On Emscripten the suffix is `.html` |
| `openttd_test` | Catch2 test executable, linked privately against `openttd_lib`. Tests are registered with `catch_discover_tests` |
| `tools` | Custom target for host tools (`strgen`, `settingsgen`) |
| `find_version` | Regenerated on every build via `cmake/scripts/FindVersion.cmake`; writes `generated/rev.cpp` |
| `docs` / `docs_source` | Doxygen, only when Doxygen is found (Unix) |
| `regression` | Regression suite added after the game targets |

Sources are attached with `target_sources()` from per-directory `CMakeLists.txt` files, coordinated by `cmake/SourceList.cmake` (`add_files`). Precompiled headers for `openttd_lib` are `src/stdafx.h` and `src/core/format.hpp`. `src/3rdparty/fmt/format.cc` skips the PCH. Link-time optimization (IPO) is enabled for Release, MinSizeRel, and RelWithDebInfo when the compiler supports it. `compile_commands.json` is exported.

`openttd` links `openttd_lib`, `openttd::media`, and `openttd::basesets`. `openttd_lib` links `openttd::languages`, `openttd::settings`, `openttd::script_api`, and `Threads::Threads`.

### CMake options (`cmake/Options.cmake`)

| Option | Default | Effect |
| --- | --- | --- |
| `OPTION_DEDICATED` | OFF | Headless server. Defines `DEDICATED`. Skips GUI libraries |
| `OPTION_TOOLS_ONLY` | OFF | Build tools only (`strgen` and friends), then return. Used for cross-compiling |
| `OPTION_DOCS_ONLY` | OFF | Implies `OPTION_TOOLS_ONLY` |
| `OPTION_USE_ASSERTS` | ON | `WITH_ASSERT`, otherwise `NDEBUG`. Release builds turn asserts off by default |
| `OPTION_INSTALL_FHS` | ON on Linux unless packaging deps | Install under FHS folders (`share/games`, `games`) |
| `OPTION_PACKAGE_DEPENDENCIES` | OFF | Linux only. Sets `RPATH` to `$ORIGIN/lib` and copies linked libraries beside the binary. Incompatible with FHS install. Needs CMake 3.16+ |
| `OPTION_USE_NSIS` | OFF | Windows installer via NSIS; intended for stable releases |
| `OPTION_ALLOW_INVALID_SIGNATURE` | OFF | Load content with a bad signature. Defines `ALLOW_INVALID_SIGNATURE`. CMake warns that this is a security risk |
| `OPTION_SURVEY_KEY` | empty | Opt-in survey key, compiled in as `SURVEY_KEY` |
| `OPTION_FORCE_COLORED_OUTPUT` | OFF | ANSI color from GNU/Clang even when not a TTY |
| `PERSONAL_DIR` / `SHARED_DIR` / `GLOBAL_DIR` | per OS | Compiled into `WITH_PERSONAL_DIR`, `PERSONAL_DIR`, `WITH_SHARED_DIR`, `SHARED_DIR`, `GLOBAL_DATA_DIR` |

Developer `CXXFLAGS` called out in `COMPILING.md`: `-DRANDOM_DEBUG` (desync debugging), `-fno-inline`, `-O0`, `-p`. A clean build directory is required after changing `CXXFLAGS` because CMake caches them.

Base-set graphics can be regenerated with **GRFCodec** / **NFORenum** if those executables are on the path. That rewrites `.grf` files inside the source tree. Cache entries `GRFCODEC_EXECUTABLE` and `NFORENUM_EXECUTABLE` can be cleared to stop that.

### Supported platforms

Actively supported, from the README:

- Linux: SDL2, with or without OpenGL. Allegro is an alternate video/sound/music backend. SDL2 or Allegro is required unless the build is dedicated.
- macOS: universal Cocoa build (Quartz and OpenGL video). Deployment target is macOS 10.15. Objective-C++ is enabled. Links AudioToolbox, AudioUnit, Cocoa, QuartzCore. Defines `WITH_COCOA`.
- Windows: Win32 GDI and Win32 OpenGL. Unicode build (`UNICODE`, `_UNICODE`), Uniscribe (`WITH_UNISCRIBE`), WinHTTP, bcrypt, winsock, winmm.

Also wired in CMake, with less emphasis in the README:

- **Emscripten** produces a browser build (`os/emscripten`). Heap growth, 32 MiB initial memory, exceptions on, IDBFS for persistence, English language and baseset preloaded. HTTP goes through JavaScript rather than libcurl.
- **Dedicated server** (`OPTION_DEDICATED`): no GUI, fewer libraries.
- **Haiku** links `be`, `network`, and `midi`.
- BSD is expected to work but is not actively tested.

Video backends live under `src/video`: Win32 GDI, Win32 OpenGL, SDL2, SDL2 OpenGL, Cocoa Quartz, Cocoa OpenGL, Allegro, null, dedicated.

### External libraries

`COMPILING.md` splits them into encouraged and optional. The game still configures if many are missing, but savegames and content download break without the compression libraries.

| Library | Role | Where it is required |
| --- | --- | --- |
| zlib | Old savegames (0.3.0–1.0.5), content downloads, heightmaps | Encouraged everywhere |
| liblzma | Savegames from 1.1.0 onward | Encouraged everywhere |
| libpng | Screenshots and heightmaps | Encouraged |
| liblzo2 | Savegames older than 0.3.0 | Optional |
| breakpad | Crash minidumps | Encouraged; skipped on Emscripten and on Windows ARM (`vcpkg.json`) |
| libcurl | Content downloads | Linux and other non-Windows, non-Emscripten targets. Windows uses WinHTTP |
| SDL2 | Video, sound, mouse | Linux (or Allegro instead) |
| FreeType, Fontconfig | Fonts | Linux GUI builds |
| HarfBuzz + ICU | Right-to-left scripts and natural sort | Linux GUI builds. HarfBuzz without ICU i18n is disabled |
| OpusFile | Opus sound loading | GUI builds |
| soxr | Audio resampling | GUI builds |
| FluidSynth | MIDI via software synth | Non-Windows GUI builds |
| XAudio2 | Windows audio | Windows GUI builds |
| OpenGL | Hardware video | Linked directly except on Linux+SDL2, where SDL2 loads it and the build only defines `WITH_OPENGL` |
| SSE | Optional sprite sorter (`viewport_sprite_sorter_sse4.cpp`) | When detected |

`vcpkg.json` baseline `f3e10653cc27d62a37a3763cd84b38bca07c6075` declares: breakpad, curl (http2, Linux), dbus (Linux, no default features), fontconfig, freetype, harfbuzz, icu, liblzma, libpng, lzo, opusfile, soxr, sdl2 (Linux), zlib.

### Vendored third party (`src/3rdparty`)

Compiled into the tree rather than fetched at build time:

- **Squirrel** (Zlib) — scripting VM for AIs and Game Scripts. Include path is global.
- **fmt** (MIT) — formatting. Wrapped by `src/core/format.hpp`.
- **nlohmann/json** (MIT) — header-only JSON.
- **Catch2** (BSL-1.0) — unit tests.
- **md5** (Zlib).
- **ICU ScriptRun** (Unicode license) — a small vendored piece, separate from the system ICU package.
- **Monocypher** (BSD-2 / CC0) — cryptography (Ed25519 lives beside it).
- **OpenGL headers** (MIT) — `glext.h`, `wglext.h`, `khrplatform.h`.
- **OpenTTD Social Integration API** (MIT) — plugin ABI for Steam, Discord, and similar. Plugins themselves are dropped into a `social_integration` folder at runtime, not linked at build time.

`cmake/3rdparty/llvm/CheckAtomic.cmake` is Apache-2.0 and only detects whether `libatomic` is needed.

### Code generation and packaging

- `src/strgen` builds the string compiler. Language files are compiled into `openttd::languages`.
- `src/settingsgen` generates the settings tables (`openttd::settings`).
- `cmake/scripts/SquirrelExport.cmake` and `SquirrelIncludes.cmake` generate the script API glue (`*.hpp.in` under `src/script/api`).
- `cmake/scripts/GenerateWidget.cmake` participates in widget generation.
- `cmake/scripts/CreateGRF.cmake` and `Baseset.cmake` build the extra base graphics (`media/baseset`).
- Install and package logic is `cmake/InstallAndPackage.cmake`, with packagers `PackageDeb.cmake`, `PackageRPM.cmake`, `PackageNSIS.cmake`, `PackageBundle.cmake`.
- Version strings come from git via `FindVersion.cmake` on every build.

### Continuous integration

GitHub Actions under `.github/workflows/`:

- `ci-build.yml` orchestrates Emscripten, Linux (Clang+SDL2, GCC+SDL2, GCC dedicated with `-DRANDOM_DEBUG`), macOS, and the Windows/MinGW workflows it includes (`ci-linux.yml`, `ci-windows.yml`, `ci-macos.yml`, `ci-mingw.yml`, `ci-emscripten.yml`).
- Nightly, preview, and release workflows publish Linux, Windows, macOS, source tarballs, docs, Steam, GOG, the Microsoft Store, and the CDN.
- Checkers: commit message, rebase, CodeQL, unused strings, script missing-mode, docs, file descriptions.

### Runtime data the binary expects

The executable needs a graphics, sound, and music base set. Free sets are OpenGFX, OpenSFX, and OpenMSX. Original Transport Tycoon Deluxe GRF/CAT/GM files can be used instead, copied into `baseset/`. Search paths and the `openttd.cfg` INI layout are documented in `docs/directory_structure.md`. Content packs (NewGRF, 32bpp, AI, Game Script) can also be fetched in-game ("Check Online Content") into `newgrf/`, `ai/`, and `game/`.

## Repository layout

| Path | Contents |
| --- | --- |
| `src/` | The game. One large flat module plus subsystem directories |
| `src/3rdparty/` | Vendored libraries above |
| `src/os/` | `windows`, `unix`, `macosx` entry points, crash logs, font backends |
| `src/video`, `sound`, `music` | Driver implementations |
| `src/blitter`, `spriteloader` | Pixel blitters and sprite decoders |
| `src/network` | Multiplayer, admin port, content download, game coordinator, STUN/TURN |
| `src/saveload` | Save/load chunks plus `compat/` tables for old versions |
| `src/script`, `ai`, `game` | Squirrel host, NoAI, Game Script |
| `src/newgrf` | NewGRF interpreter |
| `src/pathfinder/yapf` | Yet Another Path Finder |
| `src/linkgraph` | Cargo distribution (link graph / MCF) |
| `src/timer` | Calendar, economy, tick, and realtime timers |
| `src/core` | Pool, endian, bitsets, strings, k-d tree, math |
| `src/widgets` | Per-window widget ID headers |
| `src/table` | Static data tables, including settings |
| `src/lang` | Translations |
| `src/tests` | Catch2 tests |
| `src/strgen`, `settingsgen` | Build-time tools |
| `bin/ai`, `bin/game` | Scripts shipped with the binary |
| `media/baseset` | OpenTTD extra GRF and base-set build |
| `cmake/` | Options, flags, find-modules, packaging |
| `os/` | Emscripten shell, macOS `Info.plist`, GOG manifests. Distinct from `src/os` |
| `regression/` | Game-level regression (station list, GS compatibility) |
| `docs/` | Savegame format, multiplayer, desync, link graph, admin network, directory structure |
| `.github/` | CI and release workflows |

`zz-reach2/` and `!kit/` sit in the working tree and are not part of the upstream game.

## Architectural patterns

There is no application framework (no Qt, no game engine). The game is a C++20 simulation with a hand-rolled GUI, a driver layer for the OS, and a command log as the way world state changes. Patterns below are the ones the code actually uses.

### 1. Command as the only world mutation

Every player action that changes the game is a **command**: a function returning `CommandCost` (money, success, error `StringID`), registered with `DEF_CMD_TRAIT` against the `Commands` enum. Declarations live in `*_cmd.h`, implementations in `*_cmd.cpp`. `DoCommand` is the execution path. `StateGameLoop` in `src/openttd.cpp` is the only place allowed to change the high-level game state, and that rule is enforced inside `DoCommand`.

Commands are the multiplayer protocol. The client sends the command; the server (or the local game, when offline) executes it. Scripts go through the same path (`ScriptObject::DoCommandPrep` / `DoCommandProcessResult` in `src/script/api/script_object.cpp`). A test-only execution estimates cost without applying the change. Nested commands are counted by `RecursiveCommandCounter`. Flags distinguish things like `NoTest`, `ClientID`, `Deity`, `Auto`, and `Location`.

This is the determinism boundary. GUI, scripts, and network code are supposed to request a command rather than poke tile or vehicle memory.

### 2. Pool-indexed game objects

Vehicles, towns, stations, companies, and similar entities are **pool items**: a slot array plus a typed index. `src/core/pool_type.hpp` defines `Pool`, `PoolID` (a strong index: base type, tag type, end, invalid), and `PoolItem`. Pool kinds are `Normal` (reset with the game), `NetworkClient`, `NetworkAdmin`, and `Data` (NewGRF and other data that survives a game reset).

IDs are not raw integers. `PoolID` only converts back to the underlying integer through `.base()`, so a `VehicleID` and a `StationID` are different types even when both are 16-bit.

### 3. Tile map as a packed array

The world is a 2D array of tiles. `Tile` in `src/map_func.h` is a wrapper the compiler is expected to inline away. Each tile stores a compact `TileBase`: 4-bit `TileType` plus bridge and tropic bits, northern corner height, and general-purpose bytes `m1`–`m5` (ownership, town/industry/station indices, and feature-specific flags). The layout is documented as `docs/landscape.html` (the comment in `map_func.h` points there).

`TileType` (`src/tile_type.h`) is Clear, Railway, Road, House, Trees, Station, Water, Void, Industry, TunnelBridge, Object. A rail tile with a road crossing is stored as Road. Tile geometry uses `TILE_SIZE` 16 world units and `TILE_HEIGHT` 8. Per-feature accessors live in `*_map.h` (`rail_map.h`, `road_map.h`, `water_map.h`, `town_map.h`, and so on), so the packed bits are not read ad hoc from gameplay code.

### 4. File-suffix modules

A feature is split by responsibility rather than by a class hierarchy:

| Suffix | Responsibility |
| --- | --- |
| `_cmd` | Commands that mutate the world |
| `_gui` | Windows for that feature |
| `_map` | Tile bit accessors |
| `_type` | Enums and ID typedefs |
| `_func` | Free functions |
| `_base` | Pool item / core struct |
| `_sl` | Save/load chunk (`src/saveload`) |
| `_widget` | Widget enum for one window (`src/widgets`) |

Example for waypoints: `waypoint_cmd.cpp`, `waypoint_gui.cpp`, `waypoint_base.h`, `waypoint_func.h`, `saveload/waypoint_sl.cpp`, `widgets/waypoint_widget.h`.

### 5. Immediate-mode-ish window and widget GUI

`Window` (`src/window_gui.h`) is the base of every dialog and toolbar. Widgets are described declaratively (type, resize, colour, bounds, strings) and addressed by a per-window enum (`BuildRailDepotWidgets` and similar). Widget headers live in `src/widgets/`. The window class implements virtual hooks for click, paint, place-on-map (`OnPlaceObject` takes a `TileIndex`), drag, and resize. `WidgetDimensions` holds the pixel metrics, with an unscaled table and a zoom-scaled copy.

This is a retained window list (windows exist as objects) drawn by the game's own blitter, not a toolkit.

### 6. Driver factories for video, sound, and music

`Driver` (`src/driver.h`) is the OS-facing plugin: `Start`, `Stop`, `GetName`. `DriverFactoryBase` registers implementations in a process-wide map keyed by name, with a priority used to pick a default. Concrete types:

- `VideoDriver` — Win32, SDL2, Cocoa, Allegro, null, dedicated. The video driver owns the frame loop and calls `GameLoop()`.
- `SoundDriver` — Win32, XAudio2, SDL, Cocoa, Allegro, null.
- `MusicDriver` — Win32, DirectMusic, FluidSynth, ExtMidi, Cocoa, BeMidi, Allegro, null.

`Blitter` (`src/blitter/base.hpp`) is a second rendering strategy, orthogonal to the video driver: 8/16/24/32 bpp sprite drawing, colour remap, transparency, palette animation. Sprite bytes are decoded by `SpriteEncoder` / `src/spriteloader`.

### 7. Game loop and timers

`openttd_main` (`src/openttd.cpp`) boots the process. The frame is `VideoDriver::GameLoop`, which calls into `::GameLoop()` / `StateGameLoop()`.

`StateGameLoop` does, when the game is not paused:

1. Optional desync dump (`dmp_cmds_*.sav`) when desync logging is on.
2. Cache check.
3. Switch company to `OWNER_NONE` so simulation steps stay identical on every peer.
4. Animated tiles, calendar-day vehicle procs, economy timer, game-tick timer, tile loop, vehicle ticks, landscape tick.
5. AI and Game Script `GameLoop`.
6. Window tick and news.

Pause and modal progress windows skip the simulation but still run the Game Script loop. Economy, trains, road vehicles, ships, aircraft, and landscape each have a `PerformanceElement` accumulator.

Time is split across timer types in `src/timer`: `TimerGameCalendar` (the date the player sees), `TimerGameEconomy` (including `date_fract` sub-ticks), `TimerGameTick`, and realtime. `TimerManager<T>` is a singleton registry; timers implement `Elapsed` and are registered automatically.

### 8. Squirrel for AI and Game Scripts

AI (NoAI, one company) and Game Scripts (GS, the scenario/deity layer) are **Squirrel** programs. `ScriptInstance` owns the VM, a suspend/continue handshake (a script blocks until the server finishes its `DoCommand`), and a custom save format (`SQSaveLoadType`) so script state round-trips inside the savegame. The C++ API under `src/script/api` is exported into Squirrel by the CMake squirrel-export scripts. Libraries ship under `bin/ai` and `bin/game` and are loaded from `ai/` and `game/` at runtime.

Scripts are untrusted relative to the simulation: they call the command API, they do not write pools directly.

### 9. NewGRF as the content extension format

NewGRF (the GRF container inherited from TTDPatch / Transport Tycoon) is how vehicles, industries, houses, stations, and graphics are modded. `src/newgrf` plus `src/newgrf.h` hold the feature tables, callbacks, and sprite groups. NewGRF data uses the `PoolType::Data` pool so it is not wiped with a new game the way vehicles are. Save/load has its own `newgrf_sl.cpp` and compatibility tables, because a save must remember which GRFs were active.

### 10. Chunked, big-endian savegames

Documented in `docs/savegame_format.md`. Outer container: 4-byte compression tag (`OTTD` LZO, `OTTN` none, `OTTZ` zlib, `OTTX` LZMA), 2-byte version, then a compressed blob. Inside, data is **big-endian** regardless of host. Chunks are 4-byte tags (`PLYR` for companies, and so on) plus a type byte: RIFF blob, array, sparse array, table, or sparse table. Lengths use a variable-width **gamma** encoding. Each subsystem has a `*_sl.cpp` writer and, when the format moved, a `saveload/compat/*_sl_compat.h` reader. `afterload.cpp` fixes up older saves after the chunks are read. The project guarantees that current OpenTTD loads every older revision's save, corrupt files aside.

### 11. Lockstep multiplayer

`src/network` is a custom stack, not a third-party netcode library. Pieces:

- `network_server` / `network_client` and `core/tcp_game` for the game connection.
- `network_command` — commands are the replicated input.
- `network_admin` — a separate admin port (`docs/admin_network.md`).
- `network_content` — in-game content download (TCP content protocol; HTTP via curl or WinHTTP).
- `network_coordinator`, `network_stun`, `network_turn` — NAT traversal and the game coordinator (`docs/game_coordinator.md`).
- `network_crypto` — session crypto, with Monocypher underneath.
- `core/packet` — the byte packet format.

Clients do not simulate ahead of the server's command stream. Desyncs are a first-class debugging target (`docs/desync.md`, `docs/debugging_desyncs.md`, `-DRANDOM_DEBUG`, periodic desync save dumps).

### 12. Pathfinding and cargo flow

Vehicle routing is **YAPF** (`src/pathfinder/yapf`): a template A* split by rail, road, ship, and river building, with node types, cost functions, and a cost cache. Ships also use a water-region abstraction (`water_regions`).

Cargo distribution is a **link graph** (`src/linkgraph`, `docs/linkgraph.md`). The multi-commodity flow solver is expensive and runs on a worker thread when threads exist. The main loop joins that work on a deadline (`StateGameLoop_LinkGraphPauseControl`); if the job is still running, the game stalls. Accuracy and the day budget are player settings.

### 13. Strings, settings, and endian portability

User-visible text is not hardcoded for each language. `strgen` compiles `.lng` sources into a string table addressed by `StringID`. Translations are maintained out of tree at translator.openttd.org (README section 2.4).

Settings are tables generated by `settingsgen` (`src/table/settings`). The on-disk form is `openttd.cfg`, an INI file, mostly edited through the in-game advanced settings window.

`src/core` also supplies endian helpers, `EnumBitSet`, overflow-safe numbers, strong typedefs, k-d trees (towns and viewport sprite sorting), and a small string consumer/builder. Savegames and network packets go through explicit endian buffers (`misc/endian_buffer.hpp`), so a little-endian Windows host and a big-endian save stay compatible.

### 14. Global state and naming

The simulation still uses process-wide globals, prefixed with `_` (`_networking`, `_pause_mode`, `_current_company`, `_settings_game`). `CODINGSTYLE.md` requires that prefix. Class members are always written `this->member`. Functions are CamelCase; variables are `snake_case`; unscoped enumerators are `SHOUTY_CASE`; scoped enumerators are CamelCase. Hot loops cache repeated expressions rather than relying on the compiler only.

Doxygen comments are mandatory on functions. Enumerators that should appear in the docs use `///<`.

## Framework summary

| Layer | Mechanism |
| --- | --- |
| Build | CMake 3.17+, C++20, vcpkg on Windows, GitHub Actions |
| Language / stdlib | C++20, threads, no compiler extensions |
| Tests | Catch2, plus a `regression/` game suite |
| Docs | Doxygen, `Doxyfile.in` |
| GUI | In-tree `Window` + widget tables |
| Rendering | `VideoDriver` + `Blitter` + sprite loaders |
| Audio | `SoundDriver` + `MusicDriver` |
| Simulation API | Commands (`CommandCost`, `DEF_CMD_TRAIT`) |
| Entities | `Pool` / `PoolID` |
| Map | Packed `Tile` array, `*_map.h` accessors |
| Scripting | Squirrel (AI and Game Script) |
| Mods | NewGRF, base sets, social-integration plugins |
| Persistence | Chunked big-endian saves, per-feature `*_sl.cpp` |
| Netplay | Custom TCP command stream, admin port, coordinator |
| Text / config | `strgen` string tables, `settingsgen`, `openttd.cfg` |

No engine, no GUI toolkit, and no networking framework sit underneath this. The framework is the set of patterns above.
