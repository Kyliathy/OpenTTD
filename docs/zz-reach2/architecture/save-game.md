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