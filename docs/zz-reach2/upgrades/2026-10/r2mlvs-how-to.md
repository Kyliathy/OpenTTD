# R2MLVS how-to - Digging around in the current build

What you can play today with the [multilayer epic](r2mlvs-multilayer-vehicles-and-structures.md), and how. Written
2026-10-05 against branch `mlm-mlv`, after master task 7.9 added the portal window's project controls. The code was
checked, and the automated tests drive the whole loop through those same controls. Nobody has played it through the
GUI yet, so expect rough edges.

## In one paragraph

Start a new game with the **Multilayer Industry** profile; below the Surface there is now solid rock.
1. Build a rail portal down to Underground. It starts **Sealed**.
2. Near its Surface door, build a short line: track to the door, a rail station and a depot. Fund an **Earth
   Reclamation Site** and a **Rock Crusher** next to the station.
3. Click the portal, set the corridor length and press **Commission TBM**, then **Start**.
4. Build a hauler train with two Earth and two Rock hoppers and an order to the station, start it, and press
   **Assign hauler** to pick it.

From then on the two trains play the loop by themselves:
- The machine drives down through the shaft, cuts the rock, lays its own temporary track and fills its hoppers.
- It backs into the portal, where the hauler swaps its full hoppers for empties.
- The hauler delivers the spoil to the receivers while the machine goes back to dig.

When the corridor is done, the portal window walks you through finishing: release the hauler, make the track
permanent, stop and retire the machine, sell it, and connect the portal so ordinary trains can drive into your new
tunnel.

## 1. Start the game

From the repository folder, run the launcher [play.ps1](../../../../play.ps1):
```powershell
cd D:\Codez\Sandbox\OpenTTD
.\play.ps1
```
It shows a numbered menu. Type a number and press Enter; Enter alone takes 1.
- **1, New game, Multilayer Industry:** a 256 x 256 temperate map with the settings of section 2 made, and a
  maximum loan of 20,000,000.
- **2, New game, Classic.**
- **Saves we have:** your `makinpr2` games. They predate the profiles, so they load as Classic games: portals,
  elevators and trains across the layers, but no digging.
- **More saves:** the newest saves in `build/save`, the ones you make and the autosaves included.

Options:
- `.\play.ps1 1` starts an entry without the menu.
- `-Seed 1234` fixes the map of a new game.
- `-Config Debug` runs the Debug build. It is slow, but it stops at any internal check that fails, which helps when
  you report a bug.
- `-All` also lists the saves the tests generated.

The launcher runs the game in the `build` folder with the config `build/play.cfg`, which makes `build` the game's own
folder:
- it finds the base graphics (`build/baseset`) and the content package (`build/newgrf/multilayer_industry.grf`) there;
- your saves go to `build/save`, where the menu finds them again.

Started without a config, the game keeps its settings and saves in `Documents\OpenTTD` instead. The game writes the
settings you change into `play.cfg` when you quit, and they stay. Picking a new game only sets the profile there, and
the temperate climate for Multilayer Industry.

## 2. New game settings

Entry 1 of the launcher makes these for you. From the game's main menu, set them in **New Game** (the Generate World
window):
- **Gameplay profile: Multilayer Industry.** Classic gives you the old four-layer map with free building.
- **Climate: Temperate.** The profile refuses the other climates.
- **Map size: at least 64 x 64.** Smaller maps are refused.
- **NewGRF: none of your own.** The profile adds its own package and refuses other NewGRFs.
- **Start year:** the default 1950 is fine; the Standard TBM and both hoppers are available from 1950. Deep
  Underground, its rail portals and the Deep TBM unlock on **1 January 1980**.
- **Money:** a TBM costs 400,000 for the machine alone, plus its hoppers, and each funded receiver costs several
  hundred thousand (I estimated this, not measured). A new company can't afford that. Either raise the maximum loan
  in the difficulty settings (the launcher sets 20,000,000) and **Ctrl+click Borrow** in the Finances window, or
  open the cheat window with **Ctrl+Alt+C** and add money (each click gives 10,000,000).

## 3. Looking around the layers

- The **layer button** on the main toolbar picks the layer you see: Surface, Underground, Deep Underground or Outer
  Space. **N** cycles through them.
- Underground and Deep Underground start as **solid rock**, except where a machine has dug. There is no rock art yet,
  so solid and dug ground look the same. Building on solid rock fails with "solid rock" in the message; that is how
  you can tell.
- None of the space plans (rockets, probes, asteroids, stations, mirrors) is built yet.

## 4. Build a sealed rail portal

- Open the rail toolbar and use the **layer portal** tool (**P**). Its picker sets the direction the mouth faces.
  Placed on the Surface, a portal always leads down to Underground.
- A portal placed on Underground lets you pick Surface or Deep; Deep stays greyed out until 1980. It also needs dug
  ground there, so in today's game an Underground-to-Deep portal is hard to reach.
- The new portal is **Sealed**: its lower end opens onto solid rock, and ordinary trains treat its door as the end of
  the line.
- Leave free ground around the portal's Surface end; the machine is assembled there.

## 5. Build the Surface line, the receivers and the hauler

The machine must have somewhere to send its spoil before you may buy it. All of this is ordinary building:
- **The line:** from the portal's Surface door, lay track out to a **rail station**, then on to a **depot** facing
  back towards the portal. The tests use a straight line out of the door; junctions and signals should work but are
  untested with a hauler.
- **The receivers:** in **Fund new industry**, build an **Earth Reclamation Site** and a **Rock Crusher**, both within
  the station's catchment. They are never generated on the map; you always fund them. Today they look like a coal
  mine and an iron ore mine (stand-in art).
- **The hauler:** in the depot, build any locomotive with **two Earth hoppers and two Rock hoppers**, one empty
  hopper for each of the machine's. Give it **one order: go to that station**.
- **Don't** assign or start the hauler yet; that comes in step 7.

## 6. Commission and start the machine

**Click the portal** (either end) to open its window. It shows:
- the state (Sealed, Commissioning or Connected);
- the buttons **Connect**, **Seal** and **Commission TBM**;
- the **corridor length** with its arrows (Ctrl+click changes it by five);
- the project panel, with a **Next:** line that always says what to do now.

To get going:
- Set the corridor length: the cells the machine digs straight out of the portal's far door, 2 to 60.
- Press **Commission TBM**. It buys a Standard TBM (a Deep TBM at a Deep portal) with 2 Earth and 2 Rock hoppers. The
  spoil goes to the nearest of your Surface rail stations that has both receivers in its catchment. If it refuses,
  the message says why: money, the model or date, the receivers, the portal not being Sealed and free, or no route.
- Press **Start**. The machine leaves the portal and drives down to the rock face. **Machine** opens its vehicle
  window, where you can follow it with the usual "centre view" button.

## 7. Assign the hauler

- Press **Assign hauler**, then **click your hauler train** in the main view, its depot or the train list.
- Start the hauler from its own window, as usual.

You can assign it any time before the hoppers fill; until then the **Next:** line says "nothing, the machine is
working". If the hoppers fill first, the machine waits in the portal and the line asks for a hauler. The window names
what's wrong if the train isn't suitable, for example if it's the machine itself or already serves another project.

## 8. Watch it dig

Switch the view to **Underground** and follow the machine:
- Each cut takes a while. When a cut is done, the cells it covered are dug out, a piece of temporary track is laid,
  and Earth and Rock go into the hoppers. You pay as it digs, and a little more while it stands.
- After each cut it drives onto its new track and starts the next one.
- When the hoppers are full, it **backs up** along its own track into the portal; it never turns around underground.
- The hauler comes in through the door and takes the full hoppers. The machine gets the empties and goes back to dig.
- The hauler takes the spoil to the station. The receivers take it in batches, so the hauler may wait there a little.
  Then it brings the empties back.
- The panel shows:
  - the phase and the cuts done;
  - anything holding the project (stopped, broken down, not enough money);
  - the Earth and Rock dug so far;
  - what the hauler is doing, and why it waits, if it does.
- The machine is serviced in the portal every time it backs in, so it never wanders off to a depot. The hauler
  services at its depot as usual.
- **Pause / Resume** holds the project where it is. **Cancel** (asks first) stops digging for good: the machine comes
  back, hands its spoil to the hauler, and the dug ground stays dug.
- Ordinary trains can't use the portal while the project holds it; they turn back at the door.

## 9. Finish and open the tunnel

When the corridor is done, the **Next:** line takes you through these steps, one at a time:
1. *Wait while the hauler delivers the last load*, then **Release hauler**. After the end, the hauler never drives
   back into the portal.
2. **Make track permanent** turns the machine's temporary track into ordinary rail, for its conversion price.
3. **Stop** the machine, then **Retire** the project. The portal is sealed again, and the machine becomes an ordinary
   train standing inside it.
4. **Sell machine**.
5. **Connect** opens the portal to ordinary trains. Any train can now drive down into the tunnel the machine dug,
   along the converted track, and back.

You can then commission another machine at another portal and dig elsewhere.

## Limits today

- **Straight corridors only.** The window always asks for a straight corridor out of the door. Areas, chambers and
  turns work in the engine and its tests, but the area tool isn't built yet.
- **Fixed consist.** Always 2 Earth and 2 Rock hoppers; you can't choose the model or the wagons.
- **No real art.** The TBM, hoppers, receivers and portal states use stand-ins (an ordinary locomotive and wagon, two
  mine layouts), and solid rock looks like dug ground. The project panel is plain text.
- **Underground only, in practice.** Deep needs an Underground portal with free dug ground around it, which straight
  corridors make hard to reach.
- **Balance is untuned.** Prices, digging speed and receiver intake are placeholders until the balance plan.

## If something goes wrong

- The **Next:** line says what the project waits for.
- The **Held:** line names the reason for a hold, and the hauler line names the reason the hauler waits.
- A button that is greyed out isn't possible right now.
- Please note what the window said, and save the game, when something looks stuck: the save carries the whole
  project, its hauler and its wagons.
