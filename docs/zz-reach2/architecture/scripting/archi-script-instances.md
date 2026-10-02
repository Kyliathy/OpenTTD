# AI instance and Game Script instance

Both are a `ScriptInstance` (`src/script/script_instance.hpp`) with its own Squirrel VM. The shared tick, command handshake, and save format are in [archi-scripting.md](archi-scripting.md). This file is how the two subclasses differ.

`AIInstance` (`src/ai/ai_instance.hpp`) is one competitor company. `GameInstance` (`src/game/game_instance.hpp`) is the scenario script: a single deity that can also act as any company.

## Identity

| | AI | Game Script |
| --- | --- | --- |
| C++ type | `AIInstance` | `GameInstance` |
| API name passed to `Squirrel` | `"AI"` | `"GS"` |
| Squirrel class prefix | `AIVehicle`, `AITown`, … | `GSVehicle`, `GSTown`, … |
| How many | One per AI company, stored on `Company::ai_instance` | One, `Game::instance` |
| Company passed to `ScriptController` | `_current_company` at `StartNew` | `OWNER_DEITY` |
| `ScriptObject::GetRootCompany` | That company | `OWNER_DEITY` |
| Randomizer slot | The company's `Owner` | `OWNER_DEITY` |
| Controller registration | `SQAIController_Register` | `SQGSController_Register` |
| Class registration | `SQAI_RegisterAll` | `SQGS_RegisterAll`, then `RegisterGameTranslation` |
| Compatibility scripts | `Subdirectory::Ai`, `AIInfo::ApiVersions` from `"0.7"` | `Subdirectory::Gs`, `GameInfo::ApiVersions` from `"1.2"` |
| DoCommand callback | `CcAI` | `CcGame` |

`ScriptCompanyMode` (`@api game`) is the switch a Game Script uses to act as a real company. While it is alive, `ScriptObject::SetCompany` changes the company used for queries and for `Command::Execute`, and costs are charged to that company. `IsDeity()` is true when the active company is `OWNER_DEITY`. `IsValid()` is true when that company still exists in the pool. `DoCommandPrep` rejects a command when the active company is neither a live company nor the deity.

An AI has no `ScriptCompanyMode` in its API. Its root company is fixed. Bankrupting that company, or an invalid company id, fails the precondition check.

Deity commands still have to pass `InternalExecutePrepTest`: `OWNER_DEITY` is allowed only when the command carries `CommandFlag::Deity` (goals, story pages, and the other GS-only mutations). Acting as a company is how a Game Script builds track, buys vehicles, or spends money.

## Who starts them

`AI::StartNew(company)` (`src/ai/ai_core.cpp`):

- Returns immediately on a network client.
- Reads `Company::ai_config`, or copies the game setting for that slot.
- If the config names no script, `AIScannerInfo::SelectRandomAI` picks one.
- Anchors settings that scripts mark unchangeable, constructs `AIInstance`, calls `Initialize(info)`, then `LoadOnStack` with any blob stashed at load time.

`AI::CanStartNew` is false in multiplayer unless `_settings_game.ai.ai_in_multiplayer` is on. `AI::GameLoop` has the same check, so existing AIs also stop ticking when that setting is off.

`AI::GameLoop` advances only every `2^(4 - competitor_speed)` frames (`difficulty.competitor_speed` is at most 4). Inside that stride it calls `GameLoop` on every company with `is_ai`.

`Game::StartNew` (`src/game/game_core.cpp`):

- Returns if an instance already exists, if the mode is the menu, or if this process is a network client.
- Returns if `GameConfig` has no script. There is no random fallback and no dummy script (`LoadDummyScript` is empty).
- Sets `_current_company` to `OWNER_DEITY`, constructs `GameInstance`, initializes, and pushes save data.

`Game::GameLoop` ticks every frame the authoritative loop calls it, including while the game is paused. `StateGameLoop` calls `Game::GameLoop` in the paused branch and returns before `AI::GameLoop`. AIs do not run during pause.

`AI::Pause` refuses a dedicated server: the dedicated build has no UI that could unpause. `Game::Pause` has no such guard. `ScriptController::Break` is a developer pause of the script; it also no-ops on a dedicated server and when AI developer tools are off.

## API surface

`SquirrelExport.cmake` emits a method only when the header's `@api` line includes that pass. Classes that exist only for Game Scripts (`@api game`):

| Header | Role |
| --- | --- |
| `script_admin.hpp` | JSON to the admin port |
| `script_asyncmode.hpp` | Queue a command and keep running |
| `script_cargomonitor.hpp` | Watch cargo |
| `script_client.hpp`, `script_clientlist.hpp` | Connected clients |
| `script_companymode.hpp` | Act as a company, or sit as deity |
| `script_game.hpp` | Pause the match, landscape, `IsMultiplayer` (`_network_server`) |
| `script_goal.hpp` | Goals |
| `script_league.hpp` | League tables |
| `script_news.hpp` | News |
| `script_story_page.hpp` and the story lists | Story book |
| `script_text.hpp` | Translatable GS strings |
| `script_viewport.hpp` | Viewport control |

`script_event_types.hpp` marks several events `@api game` as well (story, goal, admin). Methods tagged `@api -ai` (founding towns, some industry and subsidy controls, and similar) are compiled into the Game Script VM only.

Both sides get the company-play classes: vehicles, rail, road, stations, towns (queries), map, industries (queries), orders, groups, signs, engine lists, `ScriptTestMode`, `ScriptExecMode`, `ScriptLog`, `ScriptEvent`.

An AI that crashes shows the script debug window for `_current_company` unless the game is in the menu (the dummy AI dies there on purpose) or a save is still loading. A Game Script crash opens the debug window for `OWNER_DEITY`. Both point at `ScriptInfo::GetURL` when the author supplied one.

## Commands and callbacks

`AIInstance::GetDoCommandCallback` returns `CcAI`. `GameInstance::GetDoCommandCallback` returns `CcGame`. `ScriptObject::GetDoCommandCallback` asks the active instance, and the networking path of `Command::Unsafe` passes that pointer into `NetworkSendCommand`.

`CcAI` looks up `Company::GetIfValid(_current_company)`. A missing company or a missing `ai_instance` (the company died before the command frame) drops the result. Otherwise `DoCommandCallback` plus `Continue` wake that company's VM.

`CcGame` always uses `Game::GetInstance()`. The deity does not vanish when a company goes bankrupt.

`ScriptAsyncMode` is GS-only. On that path `Execute` passes a null network callback (the script will not suspend waiting for it) and `DoCommandProcessResult` inserts the test-time result immediately. The command is still queued on the server and executed later; the VM is not resumed a second time when that happens.

## Events

`AI::NewEvent(company, event)` and `AI::BroadcastNewEvent` enqueue on each live AI's `ScriptStorage::event_queue`. `Game::NewEvent` enqueues on the Game Script. Both return at once on a client, so event objects are server-side only. World-visible consequences (a goal, a news item, a story page) are still commands, and those replicate.

## Save chunks

| | AI | Game Script |
| --- | --- | --- |
| Chunk | `AIPL` in `src/saveload/ai_sl.cpp` | `GSDT` in `src/saveload/game_sl.cpp` |
| Key | One array entry per company slot | A single record |
| Config saved | Name, version, settings string, and whether the choice was "random" | Name, version, settings string |
| Blob | `AI::Save` → `AIInstance::Save` for a live AI company | `Game::Save` → `GameInstance::Save` when the instance exists |
| Who keeps the blob | Server, and single player | Server, and single player |
| Client / menu | `AIInstance::LoadEmpty` | `GameInstance::LoadEmpty` |

`AI::Save` writes an empty blob for a company whose `ai_instance` was not finished (emergency save) and for every company when the caller is a client. `Game::Save` does the same when `Game::instance` is null or the caller is a client.

After load, the server attaches the blob to the company `AIConfig` or the `GameConfig` via `SetToLoadData`. `StartNew` moves it onto the new VM with `LoadOnStack`. A client never reaches `StartNew` for scripts, and has already discarded the bytes.

If the saved AI name cannot be resolved, the loader substitutes a random AI and passes version `-1`, so `Load` produces no data. If the saved Game Script cannot be resolved, the game continues with `Game::instance` unset.
