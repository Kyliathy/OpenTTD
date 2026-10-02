# Scripting architecture (NoAI and Game Scripts)

OpenTTD runs untrusted Squirrel scripts against the game through one runtime: `ScriptInstance` plus a `Squirrel` wrapper around an `HSQUIRRELVM`. Two concrete instances share that runtime.

| Kind | C++ type | Lives on | Company identity |
| --- | --- | --- | --- |
| NoAI (an AI company) | `AIInstance` in `src/ai/` | `Company::ai_instance` | That company's `CompanyID` |
| Game Script (scenario / deity) | `GameInstance` in `src/game/` | `Game::instance` (one per game) | `OWNER_DEITY` |

The instance split (lifecycle, API surface, callbacks, save chunks) is in [archi-script-instances.md](archi-script-instances.md). This file is the shared machine: the VM, the command handshake, save/load, and the generated API glue.

## Where the bytecode runs

Script bytecode runs on the process that owns the world: a single-player game, or the network server (`_network_server`).

`StateGameLoop` in `src/openttd.cpp` calls `AI::GameLoop()` and `Game::GameLoop()` from the authoritative tick. Both functions return immediately when `_networking` is set and this process is not the server. `AI::StartNew` and `Game::StartNew` have the same guard, so a client never constructs an `AIInstance` or `GameInstance`. `AI::NewEvent` and `Game::NewEvent` drop events on clients.

AIs have a second gate: `AI::GameLoop` and `AI::CanStartNew` also require `_settings_game.ai.ai_in_multiplayer`. Game Scripts have no equivalent switch; the server runs the one configured script whenever `Game::instance` exists.

The editor tick does not call either loop. `Game::StartNew` also refuses `GameMode::Menu`.

While the game is paused (`_pause_mode`), `StateGameLoop` still calls `Game::GameLoop()` and then returns, so a Game Script keeps ticking. `AI::GameLoop` sits in the unpaused branch, so AIs wait out a pause.

Clients still apply world edits. Those edits arrive as ordinary network commands. The Squirrel VM that asked for them stays on the server. See [Invariants](#invariants).

## One VM per instance

`ScriptInstance` owns:

- `engine`, a `Squirrel` (`src/script/squirrel.hpp`). `Squirrel::Initialize` calls `sq_open(1024)`, installs compile and runtime error handlers, and stores `this` with `sq_setforeignptr`. Native glue recovers the engine from that pointer.
- `controller`, a `ScriptController` constructed with the company id (the AI's company, or `OWNER_DEITY`).
- `instance`, an `HSQOBJECT` for the script's main class (`ScriptInfo::CreateInstance`).
- `storage`, a `ScriptStorage`: current company, DoCommand mode, last command and result, event queue, log, rail/road type, per-script cost counter.

`ScriptObject::ActiveInstance` is a scope object. Constructing it sets the global active instance and a `ScriptAllocatorScope` so Squirrel allocations hit this VM's allocator. Destroying it restores the previous instance. The game thread ticks scripts one after another; the active instance is a stack, not a thread-local.

The script file is a `.nut` (or Squirrel bytecode) loaded by `Squirrel::LoadScript`. Search paths follow the API name: `AI` reads `Subdirectory::Ai` then `AiLibrary`, `GS` reads `Gs` then `GsLibrary`.

Startup inside `ScriptInstance::Initialize` registers the API, then loads the file and creates the class instance. `DoCommand` is forbidden for that whole stretch (`DisableDoCommandScope`). The constructor, `Load`, and `Start` run later, on the first `GameLoop` tick:

1. `constructor`, if present, with `allow_do_command` cleared and a hard opcode cap (`MAX_CONSTRUCTOR_OPS`, 100000).
2. `Load(version, data)` when a save blob was pushed by `LoadOnStack`. Same restriction, cap `MAX_SL_OPS`.
3. `Start`, with the normal opcode budget. `Start` must suspend. A `Start` that returns cleanly kills the script (`Died`).

A crash, a fatal error, or an opcode overrun in those steps calls `Died`: the VM is released and `is_dead` sticks. The next ticks no-op.

### Opcode budget

Every Squirrel instruction calls `SQVM::DecreaseOps`. The budget for a tick is `_settings_game.script.script_max_opcode_till_suspend`, passed into `sq_call` / `sq_resumecatch`. When the counter hits zero, `ShouldSuspend` sets `SQVM::_suspended` and `Execute` returns. `Squirrel::Resume` continues the same VM next tick.

Native work can spend extra ops (`Squirrel::DecreaseOps`, also `ScriptController::DecreaseOps`). List and tile-list valuators do this. If a tick goes negative, `Squirrel::overdrawn_ops` shrinks the next tick's budget. Async commands charge a flat 100 ops so a Game Script cannot flood the queue for free.

`constructor`, `Save`, and `Load` use the fixed caps in `src/script/script_info.hpp` and run with suspend disabled (`sq_can_suspend` is false when the suspend argument is negative). Overrun there is a fatal "took too long" death, not a yield.

### Memory budget

Each VM has a `ScriptAllocator` (`src/script/squirrel.cpp`). `sq_vm_malloc` / `realloc` / `free` go through the allocator installed by `ScriptAllocatorScope`. The cap is `_settings_game.script.script_max_memory_megabytes` (a zero setting falls back to 128 MiB). Crossing it throws `Script_FatalError`. `ScriptStdAllocator` charges C++ containers that scripts cause (event queues and similar) to the same counter.

`CollectGarbage` runs `sq_collectgarbage` about every 255 ticks: one AI company per such tick, and the Game Script on its own counter.

### Random numbers

`ScriptBase::Rand` / `RandRange` / `Chance` read `ScriptObject::random_states`, one `Randomizer` per `Owner`. The Game Script's slot is `OWNER_DEITY`. `InitializeRandomizers` (world generation and afterload) copies `_random` and seeds each slot from the copy. The shared game RNG is left unchanged, so later script rolls do not move the simulation RNG that clients also run.

### What the script is allowed to call

Public API methods live in `src/script/api/script_*.hpp`. A method is published only when its `@api` tag names the target (`ai`, `game`, or both). `@api none` and `@api -all` stay internal. `@api -ai` / `@api -game` strip that side. The script never sees pool types (`Vehicle`, `Town`, `Company`, …) as Squirrel classes. It sees `AIVehicle` / `GSVehicle` and the other generated names, which are thin C++ facades.

Reads (`ScriptVehicle::GetLength` and the rest) call into the pools and return integers, strings, or lists. Writes call `ScriptObject::Command<Commands::…>::Do`. Strings are passed through `StrMakeValidInPlace` unless the command has `CommandFlag::StrCtrl`. A tile argument outside the map is rejected before the command runs. `DoCommand` from `constructor`, `Save`, `Load`, or a valuator (`DisableDoCommandScope` in list and tile-list code) throws `Script_FatalError`.

`ScriptTestMode` sets estimate mode: the command proc runs with test flags only, cost is accumulated, and the VM does not suspend. `ScriptExecMode` is the real-execution mode. `ScriptAsyncMode` exists only on the Game Script API: the command is still validated and queued, and the VM keeps running. The header on `ScriptAsyncMode` says the eventual cost and success can differ from the value returned to the script.

## Suspend until the command finishes

A script that builds, buys, or otherwise edits the world does not poke a pool. The facade calls `ScriptObject::ScriptDoCommandHelper::Execute` (`src/script/api/script_object.hpp`).

`DoCommandPrep` (`src/script/api/script_object.cpp`) returns four flags: hard error, estimate only, asynchronous, networking. Networking is `_networking && !_generating_world`. A hard error is "DoCommand is forbidden here" (`Script_FatalError`) or "company is invalid and this is not the deity" (`ERR_PRECONDITION_INVALID_COMPANY`).

`Execute` then calls `Command<Tcmd>::Unsafe`. `Unsafe` is `CommandHelper::Execute` in `src/command_func.h`:

1. Test the command proc (no `DoCommandFlag::Execute`).
2. On failure, estimate-only, or insufficient money, return that result. Nothing is queued.
3. When `_networking && !_generating_world` and this call did not already come from the network, `NetworkSendCommand` queues the packet and `Execute` returns without applying it.
4. Otherwise the proc runs again with `DoCommandFlag::Execute` and the pools change in this process.

`DoCommandProcessResult` turns that outcome into script control flow:

| Situation | What the script does |
| --- | --- |
| Command test failed | `SetLastError`, return `false` to the facade. The VM keeps running. |
| Estimate only (`ScriptTestMode`) | Add the estimated cost, return success. No suspend. |
| World generation, or `ScriptAsyncMode` | Apply-or-queue already happened inside `Unsafe`. The result callback runs immediately and throws `SQInteger(1)`. The glue in `src/script/squirrel_helper.hpp` catches that and returns it as the native function's Squirrel result. The VM does not suspend. |
| Network (server, game in progress) | `throw Script_Suspend(-delay, callback)`. |
| Single player | The command has already been executed. `throw Script_Suspend(+delay, callback)` so the script still waits `delay` ticks (default 1, `ScriptStorage::delay`). That wait matches multiplayer timing. |

`Script_Suspend` is a C++ exception (`src/script/script_suspend.hpp`). The vendored VM (`src/3rdparty/squirrel/squirrel/sqvm.cpp`) lets it escape a native call: `CallNative` and the `OP_CALL` handler mark `SQVM::_suspended`, record `_suspended_target` (the stack slot that should receive the return value), and rethrow. `ScriptInstance::GameLoop` catches it and stores `suspend` plus the callback (`DoCommandReturn`, `DoCommandReturnVehicleID`, …).

Negative `suspend` means "blocked on the server command". Each later tick walks the counter toward `-1` and then waits. `Continue` is the only way out:

```
suspend = -suspend - 1;
```

Whatever delay was still outstanding becomes a normal positive sleep. At `-1`, `Continue` sets `suspend` to 0 and the next tick runs the callback.

The network callback is `CcAI` or `CcGame` (`src/ai/ai_instance.cpp`, `src/game/game_instance.cpp`), both registered in the callback table in `src/network/network_command.cpp`. `NetworkSendCommand` on the server parks the packet on `_local_wait_queue` with `frame = _frame_counter_max + 1` so the server's own commands wait one frame, the same delay other clients see. `DistributeCommandPacket` keeps the callback on the server's `_local_execution_queue` (`owner == nullptr`) and sends clients a copy with a null callback. When the frame arrives, `NetworkExecuteLocalCommandQueue` runs the command for real (`PostFromNet` / `network_command == true`, so `send_net` is false and the pools update here and, via the replicated packet, on every client).

`DoCommandCallback` checks that the finished command matches `ScriptStorage::last_cmd` / `last_data` (stored only on the networking path). On a match it records success or `ScriptError`, then `Continue`. On a mismatch it logs and returns false, and `Continue` is not called, so the VM stays blocked.

The callback's job on resume is `Squirrel::InsertResult`. If the VM is still suspended, the value is written straight into `_stackbase + _suspended_target`. `GameLoop` then calls `Squirrel::Resume` → `sq_resumecatch` (`ET_RESUME_OPENTTD`), and the Squirrel call that issued the command returns that value.

`ScriptController::Sleep` is the same exception with a null callback: a positive tick count, then resume with no inserted result.

```mermaid
sequenceDiagram
    participant S as Squirrel script
    participant D as ScriptObject::Command::Do
    participant E as Command::Execute
    participant Q as Server command queue
    participant C as Clients
    participant L as ScriptInstance::GameLoop

    S->>D: facade method (build, order, goal, ...)
    D->>D: DoCommandPrep
    D->>E: Unsafe (test, then queue)
    E->>E: command proc, test flags only
    E->>Q: NetworkSendCommand (pools unchanged)
    D-->>L: throw Script_Suspend(negative delay)
    Note over S,L: VM is suspended; opcode loop is not running
    Q->>Q: frame elapses, server executes command
    Q->>C: same command, callback stripped
    Q->>L: CcAI or CcGame
    L->>L: DoCommandCallback stores result
    L->>L: Continue()
    L->>S: callback InsertResult, then Resume
```

Single player skips the queue. `Execute` writes the pools before the throw, and the positive `Script_Suspend` is only the fairness delay. The same callback then runs from `GameLoop` with no `CcAI` / `CcGame` hop.

## Why scripts cannot write pools

Pool objects are the multiplayer state. Clients reconstruct them by executing the same command stream the server executed. A script that assigned `Vehicle::cargo` or spawned a pool item in place would change the server's pools only. Clients never run the script, so they would diverge on the next checksum.

The boundary is structural:

- Squirrel holds `ScriptObject` subclasses. `sq_setinstanceup` stores that facade, not a `Vehicle *` or `Town *`.
- Mutating facades go through `Command<Tcmd>::Unsafe`, which is the same test-then-execute path the GUI uses.
- On a network server the test may succeed and the execute is deferred to `NetworkExecuteLocalCommandQueue`, which every peer runs.
- Money, company validity, pause rules, and `CommandFlag::Deity` are enforced inside that path (`InternalExecutePrepTest` rejects `OWNER_DEITY` unless the command is marked `CommandFlag::Deity`).

Read-only facades do touch pools (`::Vehicle::Get` and similar). Those calls return values. They do not publish a setter.

## Save and load of Squirrel state

The VM image is not what goes into the savegame. A fresh VM is built on load. The only persisted script state is the table returned by `Save()`.

`ScriptInstance::Save` (called from `AI::Save` / `Game::Save`, and only on the server or in single player):

1. Refuses to save if the VM is gone or already crashed (`SaveEmpty` writes a single zero byte).
2. If a previous load left the table on the stack (`is_save_data_on_stack`), that table is written again. This covers a save that happens before `Start` has consumed it.
3. Otherwise calls `Save()` with `DoCommand` disabled and `MAX_SL_OPS`. The return value must be a Squirrel table. Anything else, or an opcode overrun, writes an empty blob and marks the script crashed. `Died` runs on the next tick, because destroying the VM during the save is unsafe.
4. `SaveObject` walks the table. A dry run (`test = true`) checks the shape, then a second walk writes it. The table is left on the stack so the suspended VM can keep running.

Wire format (`ScriptInstance::SQSaveLoadType`), after a leading byte that is 0 (no blob) or 1 (blob follows):

| Tag | Payload |
| --- | --- |
| `SQSL_INT` (0x00) | int64 (int32 in saves from before `SaveLoadVersion::ScriptInt64`) |
| `SQSL_STRING` (0x01) | length byte, then at most 255 bytes |
| `SQSL_ARRAY` (0x02) | elements, then `SQSL_ARRAY_TABLE_END` (0xFF) |
| `SQSL_TABLE` (0x03) | key, value, key, value, …, then 0xFF |
| `SQSL_BOOL` (0x04) | one byte |
| `SQSL_NULL` (0x05) | nothing |
| `SQSL_INSTANCE` (0x06) | class name (the `Script` prefix stripped) plus a nested value, via `ScriptObject::SaveObject` |

Nesting stops at `SQUIRREL_MAX_DEPTH` (25). Closures, classes, floats, and instances that do not implement `SaveObject` fail the save and crash the script.

Chunks:

- `AIPL` (`src/saveload/ai_sl.cpp`): per company slot, the configured AI name, version, settings, and, for a live AI company, the running script's blob.
- `GSDT` (`src/saveload/game_sl.cpp`): the Game Script name, version, settings, and blob.

On load, a client or the intro menu calls `LoadEmpty` and discards the blob. The server (and single player) calls `ScriptInstance::Load`, which builds a `ScriptData` list, and `ScriptConfig::SetToLoadData` holds it until `StartNew`. `LoadOnStack` pushes the version and the rebuilt table onto the new VM. The first `GameLoop` calls `Load` inside the script, then `Start`.

If the exact script version is gone, the loader tries the newest version of the same name. When `MinVersionToLoad` says the blob is incompatible, the version passed to `Load` is `-1`, `Load(int)` discards the blob, and the script starts with no data. A missing AI is replaced by a random AI, then by the dummy script (`%_dummy`). A missing Game Script leaves the game with no `GameInstance`.

## Generated glue

The C++ headers are the API. CMake turns them into Squirrel registrations. Nothing in `src/script/api/*.hpp` is hand-registered method by method, except the controller files and `ScriptObject`'s constructor / `_cloned`.

`src/script/api/CMakeLists.txt` globs `script_*.hpp` (plus generated `script_window.hpp`) and, for each of `ai`/`AI`, `game`/`GS`, and `template`/`Template`, runs `cmake/scripts/SquirrelExport.cmake`. Outputs land in the build tree:

- `generated/script/api/<apilc>/<apilc>_*.sq.hpp` from `squirrel_export.sq.hpp.in`
- `generated/script/api/<apilc>/<apilc>_includes.hpp` from `script_includes.hpp.in` via `SquirrelIncludes.cmake`

`SquirrelExport.cmake` is a line-oriented reader, not a C++ parser. For each header it:

- Honors `#ifndef DOXYGEN_API` / `#ifdef DOXYGEN_API` so the Squirrel-facing signature and the real signature can differ (`ScriptAsyncMode`, `ScriptAdmin::Send`).
- Skips `#ifdef DEFINE_SQUIRREL_CLASS` blocks.
- Tracks class and brace depth, and only exports `public` members.
- Selects a class or method when the latest `* @api` line matches `APILC` (`ai` or `game`). `none` and `-all` exclude. `-ai` excludes the AI pass. The `Template` pass exports conversion specializations only.
- Records `* @suspendable` and passes that flag into `DefSQMethod` so the native call may suspend the VM.
- Maps parameter types onto Squirrel type-check characters (`i` integer, `b` bool, `x` instance, `.` string-or-instance, `a` array, `v` raw `HSQUIRRELVM`).

For a class `ScriptVehicle` on the AI pass the generated function is `SQAIVehicle_Register`. It builds a `DefSQClass<ScriptVehicle, ScriptType::AI>` named `AIVehicle`, chains to the parent (`AIObject` for `ScriptObject` subclasses), and emits `DefSQStaticMethod` / `DefSQMethod` / `DefSQConst` / scoped enums. `PushClassName` makes the glue reject an `AIVehicle` passed into a `GSVehicle` method and the reverse.

`script_includes.hpp.in` emits `SQAI_RegisterAll` / `SQGS_RegisterAll`, which register `AIObject` / `GSObject` and then every generated class. `AIInstance::RegisterAPI` calls `SQAI_RegisterAll`. `GameInstance::RegisterAPI` calls `SQGS_RegisterAll` and then `RegisterGameTranslation`.

After registration, `LoadCompatibilityScripts` walks `AIInfo::ApiVersions` or `GameInfo::ApiVersions` from the current version down to the script's `GetAPIVersion()`, loading `compat_<version>.nut` from the AI or GS directory. The current version is the last entry (`"16"` at the time of this tree). A script on the current version skips that loop.

`openttd_lib` links the interface library `openttd::script_api`, which depends on those generated headers.

## Code map

| Concern | File | Symbol |
| --- | --- | --- |
| Authoritative tick | `src/openttd.cpp` | `StateGameLoop` |
| Per-script VM and tick | `src/script/script_instance.cpp` | `ScriptInstance::GameLoop`, `Initialize`, `Save`, `Load` |
| VM wrapper | `src/script/squirrel.cpp` | `Squirrel::Initialize`, `Resume`, `InsertResult` |
| Suspend exception | `src/script/script_suspend.hpp` | `Script_Suspend` |
| Opcode suspend inside the VM | `src/3rdparty/squirrel/squirrel/sqvm.cpp` | `SQVM::Execute`, `CallNative` |
| Resume entry | `src/3rdparty/squirrel/squirrel/sqapi.cpp` | `sq_resumecatch` |
| DoCommand gate | `src/script/api/script_object.cpp` | `ScriptObject::DoCommandPrep`, `DoCommandProcessResult` |
| Facade → command | `src/script/api/script_object.hpp` | `ScriptDoCommandHelper::Execute` |
| Test, queue, or execute | `src/command_func.h`, `src/command.cpp` | `CommandHelper::Execute`, `InternalExecuteValidateTestAndPrepExec` |
| Queue and who receives the callback | `src/network/network_command.cpp` | `NetworkSendCommand`, `DistributeCommandPacket` |
| AI resume hook | `src/ai/ai_instance.cpp` | `CcAI` |
| Game Script resume hook | `src/game/game_instance.cpp` | `CcGame` |
| AI scheduling | `src/ai/ai_core.cpp` | `AI::GameLoop`, `AI::StartNew` |
| Game Script scheduling | `src/game/game_core.cpp` | `Game::GameLoop`, `Game::StartNew` |
| Per-script RNG | `src/script/api/script_object.cpp` | `ScriptObject::InitializeRandomizers` |
| AI save chunk | `src/saveload/ai_sl.cpp` | `AIPLChunkHandler` |
| Game Script save chunk | `src/saveload/game_sl.cpp` | `GSDTChunkHandler` |
| Export driver | `src/script/api/CMakeLists.txt` | `script_api`, `script_ai`, `script_game` |
| Header → registration | `cmake/scripts/SquirrelExport.cmake` | `SQ${APIUC}…_Register` |
| Register-all header | `src/script/api/script_includes.hpp.in` | `SQAI_RegisterAll`, `SQGS_RegisterAll` |

## Invariants

- **Scripts are untrusted.** Each instance has an opcode budget per tick, a memory cap, and a hard cap on `constructor` / `Save` / `Load`. Exceeding them, or throwing `Script_FatalError`, destroys that VM. `DoCommand` is refused in the constructor, in `Save` / `Load`, and in valuators. Command strings are sanitized. Tile arguments are range-checked before the command proc runs.
- **Every world edit is a command.** Script-visible mutation goes through `ScriptObject::Command<…>::Do` → `Command::Unsafe` → the shared command proc. The proc's test pass and its execute pass are the only pool writes on that path. Squirrel has no handle it can use to write a pool itself.
- **Script execution must not diverge peers, and it does not, because peers do not run it.** `AI::GameLoop`, `Game::GameLoop`, `StartNew`, and script events run only when `!_networking || _network_server`. Clients load AI and Game Script chunks with `LoadEmpty` and never construct an `AIInstance` or `GameInstance`. (`AI::Initialize` / `Game::Initialize` still build a scanner VM on every process, including clients, to read `info.nut` for the script list. That VM is not ticked and cannot issue game commands.) A script command on the server is queued by `NetworkSendCommand`, executed later on the server, and replicated by `DistributeCommandPacket`. Clients receive the command with a null callback, so they apply the same pool update and they do not enter `CcAI` / `CcGame`. Script RNG is a per-owner `Randomizer` seeded from a copy of `_random`, so rolls do not move the simulation RNG that both sides run.
