# Save-game working notes

## Retained helpers and test saves (2026-10-04)

The supplied-save inspection code and the corrected saves are retained. The platform and path-signal changes were made through the running game's construction tools and saved normally; there is no binary save-editing/replay script for those changes. The helpers below inspect saves, compare map data, control the game window and capture evidence. The format itself is documented in [savegame/archi-savegame.md](savegame/archi-savegame.md), and current layer behavior in [map-layers/archi-map-layers.md](map-layers/archi-map-layers.md).

The Multilayer Vehicles and Structures test saves are retained beside their C++ and Python tests in [`src/tests/fixtures/`](../../../src/tests/fixtures/), under `content`, `state`, `geology`, `portals`, `mines` and `processors`. Processor `.sav.txt` checkpoints accompany the saves they describe. The Python fixtures share the `save_path` resolver in [`multilayer_state_fixture.py`](../../../src/tests/multilayer_state_fixture.py); both C++ save writers use their feature directories too. Planning screenshots and review boards are in [`assets/reviews/`](../upgrades/2026-10/assets/reviews/), while authored sprite assets retain their existing asset directories.

| Helper | Use |
| --- | --- |
| [inspect_layer_save.py](../upgrades/2026-10/tools/inspect_layer_save.py) | Reusable read-only version of the supplied-save diagnostic: record summaries, per-layer tile counts, rail/station fields and portal coordinates. Explicit dimensions replace the prototype's fixed 256 x 256/four-layer assumptions. |
| [savegame_plane_digest.py](../upgrades/2026-10/tools/savegame_plane_digest.py) | Read/decompress saves, hash all ten map chunks, hash only a Surface prefix, or enumerate portal ends. Supports lzma, zlib and uncompressed containers. |
| [ottd_window.ps1](../upgrades/2026-10/tools/ottd_window.ps1) | PrintWindow screenshots, game-window clicks/keys, console commands and timed image bursts. |
| [capture_art.ps1](../upgrades/2026-10/tools/capture_art.ps1) | Read portal positions from a save and orchestrate static, toolbar and traffic captures. |
| [run_game_logged.sh](../upgrades/2026-10/tools/run_game_logged.sh) | Launch a selected game build and retain debug output. |
| [Original diagnostic prototype](../../../build/inspect_review_save.py) | The original save-specific inspection script, left in `build/`; also exports raw record bytes into the review folder. Use the reusable tool for routine inspection. |

From the repository root, these commands only read the selected save and print results:

```powershell
python -B docs/zz-reach2/upgrades/2026-10/tools/inspect_layer_save.py build/save/makinpr2-connected.sav --size-x 256 --size-y 256
python -B docs/zz-reach2/upgrades/2026-10/tools/inspect_layer_save.py build/save/makinpr2-final-verification.sav --size-x 256 --size-y 256 --summary-only
python -B docs/zz-reach2/upgrades/2026-10/tools/savegame_plane_digest.py build/save/makinpr2-connected.sav
python -B docs/zz-reach2/upgrades/2026-10/tools/savegame_plane_digest.py build/save/makinpr2-connected.sav --portals --size-x 256 --size-y 256
```

Tile layer values use storage order: Surface 0, Underground 1, Outer Space 2, Deep Underground 3. The inventory displays station tile IDs/fields, not decoded station names or a semantic dump of vehicle/order tables. Array/table record summaries include format/header records and are not object counts. Neither inspector writes a modified save.

| Save | Role |
| --- | --- |
| [makinpr2.sav](../../../build/save/makinpr2.sav) | Original supplied 256 x 256, four-plane game, preserved unchanged. |
| [makinpr2-connected.sav](../../../build/save/makinpr2-connected.sav) | Reusable connected-platform/path-signal fixture. Surface station 0 has connected platforms at `(92..94,198)`; Underground station 1 at `(80..83,198)`. Path signals are at Surface `(91,198)` and Underground `(85,198)`. |
| [makinpr2-final-verification.sav](../../../build/save/makinpr2-final-verification.sav) | Paused save from the final successful cross-layer train smoke run, using the corrected layout. |
| [makinpr2-connected-graphics-tested.sav](../../../build/save/makinpr2-connected-graphics-tested.sav) | Earlier traffic state from the portal graphics checks. |

The original save's SHA256 is `090f204f01cbbbd87cc5c1ae488fd06597bf079e2a8a7959d97e44dc1db4e30a`. Helpers under the plans' `tools/` directory are repository source artifacts; saves, logs, captures and the old diagnostic prototype remain local artifacts under `build/`. They are not bundled with the game or protected from removal of the build directory. The [art-review plan](../upgrades/2026-10/r2mlm13-layer-art-review-and-new-sprites.md) and [final review](../upgrades/2026-10/r2rvm2-review-map-2.md) record the fixture diagnosis and verification evidence.

## Historical S7 preparation notes

The notes below predate Deep Underground and the final verification. Their old Surface/Underground elevator setup and build-status remarks describe that earlier stage; use the current layer architecture and retained fixtures above for today's implementation.

Yes, I can load save games directly. Building the scenario layouts by posting clicks would be slow and error-prone, so you build the S7 layout once, save it, and I load it to run the checks.

What I can do with a save
Load it in the Debug game, using -g through tools/run_game_logged.sh. Asserts are on there, and I get the debug log, the desync cache checks and any crash files. I'll tell you before I start openttd.exe.
Look and act:
take PrintWindow screenshots of the game window;
post clicks and keys to the game window only, never through your mouse or keyboard;
run console commands right after loading, through scripts/game_start.scr.
Run it with no window for a set number of ticks (-vnull:ticks=N), the way the regression tests do. One in-game year is about 27,000 ticks. That covers the "run for a year, no assert" parts of S1–S6 quickly.
Limit: I click by screen position, so I check every click with a screenshot. One-click actions like S7's are fine. Building a whole layout that way is not.
S7 save: what to build
0. Optional, needed only for the road-conversion step. The game currently has one road type, so there's nothing to convert to. In the main menu, open Check online content, choose NewGRF, and download a set that adds road types; its description will say so. Then add it in NewGRF Settings before you start the game. If you skip this, I'll do the other two checks and the conversion waits.

1. Start the Debug game from Git Bash in the repository folder:


bash docs/zz-reach2/upgrades/2026-10/tools/run_game_logged.sh /d/Codez/Sandbox/OpenTTD/build Debug scenario-logs/s7-build.log -c scenario.cfg
It opens the main menu. Start a new 256×256 temperate game; scenario.cfg already sets those.

2. Setup:

Press Ctrl+Alt+C to open the cheats and add money a few times.
In Settings, type "reserv" in the filter and turn on Show path reservations for tracks.
3. Rail line through a portal. Portals need flat land.

On the Surface, build a 2-tile station, a depot and straight track leading away from the station.
Rail toolbar → Build a rail portal to the other map layer. Pick the mouth direction that faces your track, then click a flat tile at the end of the track.
Put a path signal on the Surface track between the station and the portal.
Press the layer button (right of the map button) to view the Underground. From the portal's underground end, continue the track in the same direction of travel, add a path signal just past the portal, then build a station.
Buy a train with orders for the Surface station, then the Underground station, and start it.
When the train has passed the Surface path signal and the highlight runs through the portal, pause before the train reaches the mouth. Switch to Underground to confirm the underground end is highlighted too.
4. A spare rail portal somewhere else on flat Surface land, with no track. That one is for the "removal charges money" check.

5. Road elevator. Road toolbar → road elevator, then pick the door direction and click a flat tile. Add a short road to the Surface door, and on the Underground a short road from the other door.

6. Before saving:

Switch back to the Surface view, centre the main view on the reserved portal at normal zoom, and leave the game paused. The save keeps where the camera is, which makes my clicks reliable.
Use the ? button → Land area information to read the coordinates of three tiles: the reserved portal's Surface end, the spare portal and the elevator's Surface end.
7. Save with Disk menu → Save game, named s7_portals. It lands in build/save/s7_portals.sav. Then quit.

Send me the three coordinates and say whether you added the road-type set.

What I'll do then
I'll load the save in Debug and run the three S7 actions:

Remove the spare portal: money must go down.
Remove the reserved portal: I'll try its Underground end, then its Surface end. Both must be refused.
Convert the road on one elevator end: both ends must change.
After each, I'll check with screenshots and the logs, and record the results under task 17.8.

















Use this command:


bash /d/Codez/Sandbox/OpenTTD/docs/zz-reach2/upgrades/2026-10/tools/run_game_logged.sh /d/Codez/Sandbox/OpenTTD/build RelWithDebInfo scenario-logs/s7-build.log -c scenario.cfg -G 1234
It differs from yours in three ways:

No -g. With -g the game skips the main menu and starts a new game straight away. You'd have no chance to add the GRF in NewGRF Settings, and you can't add it once the game is running. Without -g, you add it in the main menu, then pick New Game. -G 1234 still sets the seed for that new game.
RelWithDebInfo instead of Debug. The Debug build from 00:09 still hit the same startup assertion in my run at 00:13. I've stopped checking now.
s7-build.log instead of s1.log. It's only where the debug output goes.
When you quit, the game saves your NewGRF choice into scenario.cfg, so later games started from that config will include the Test road type as well.
