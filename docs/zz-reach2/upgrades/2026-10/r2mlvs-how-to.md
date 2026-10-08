# R2MLVS how-to - Digging around in the current build

Player guidance for the delivered [portal/TBM master](r2mlvs3mp-master-plan.md), owned by the
[multilayer epic](r2mlvs-multilayer-vehicles-and-structures.md). Architecture:
[map layers](../../architecture/map-layers/archi-map-layers.md). Updated 2026-10-08.
Automated native worlds exercise the actual controls, paid cutting, carriage exchange, Surface disposal and recovery.
Those worlds prepare their starting logistics: a complete normal-newgame bootstrap has not been demonstrated.
Personal playtesting and art feedback remain deferred. [The final audit](r2mlvs3mp-z-b-final-closure-audit.md)
records the evidence and remaining limits.

## In one paragraph

Start a new game with the **Multilayer Industry** profile; below the Surface there is now solid rock.
1. Build a rail portal down to Underground. It starts **Sealed**.
2. Near its Surface door, build a short line: track to the door, a rail station and a depot. Fund an **Earth
   Reclamation Site** and a **Rock Crusher** next to the station.
3. Click the portal, set the corridor length and press **Commission TBM**, then **Start**.
4. Build a compatible hauler train with matching empty Earth/Rock capacity and an order to the station, start it, and press
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
- **Money:** the Standard cutter's base purchase is 400,000, the Deep cutter's 800,000, with wagons and setup charged
  separately. Current paid test corridors cost about 2.55 million Standard / 3.15 million Deep in total, including
  setup, cutting, support, running costs, hauling and disposal. These are measured fixture projects, not a quote
  for your map; use the current construction quotes. Either raise the maximum loan
  in the difficulty settings (the launcher sets 20,000,000) and **Ctrl+click Borrow** in the Finances window, or
  open the cheat window with **Ctrl+Alt+C** and add money (each click gives 10,000,000).

## 3. Looking around the layers

- The **layer button** on the main toolbar picks the layer you see: Surface, Underground, Deep Underground or Outer
  Space. **N** cycles through them.
- Underground and Deep Underground start as **solid rock**, except where a machine has dug. The delivered rock,
  cut-face, cavity and resource-marker art distinguishes the two depths. **Survey geology** in Underground
  Construction reports solid/reserved/excavated ground, the mineral and remaining reserve. Surveying opens no rock.
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
  the station's catchment. They are funded facilities with their own art and fixed six-cell / sixteen-cell layouts.
  Both have bounded intake and processing; spoil disposal costs money and pays no freight profit.
- **The hauler:** in the depot, buy the spoil-hauler locomotive and matching empty hoppers. The current default
  Underground cutter has **two Earth and two Rock** wagons; Deep has **two Earth and four Rock**. Give the hauler
  **one order: go to that originating Surface station**. Assignment rechecks its actual cargo, capacity and suitability.
- **Don't** assign or start the hauler yet; that comes in step 7.

## 6. Commission and start the machine

**Click the portal** (either end) to open its window. It shows:
- the state (Sealed, Commissioning or Connected);
- the buttons **Connect**, **Seal** and **Commission TBM**;
- the **corridor length** with its arrows (Ctrl+click changes it by five);
- the project panel, with a **Next:** line that always says what to do now.

To get going:
- Set the corridor length: the cells the machine digs straight out of the portal's far door, 2 to 60.
- Press **Commission TBM**. It buys a Standard TBM (a Deep TBM at a Deep portal) with the layer's default wagon set. The
  spoil goes to the nearest of your Surface rail stations that has both receivers in its catchment. If it refuses,
  the message says why: money, the model or date, the receivers, the portal not being Sealed and free, or no route.
- Press **Start**. The machine leaves the portal and drives down to the rock face. **Machine** opens its vehicle
  window, where you can follow it with the usual "centre view" button.

For an area rather than the corridor shortcut, open **Underground Construction** on the intended rock layer and
choose **Plan an excavation area**. Click that layer's rail portal, then drag the final floor. White marks requested
floor, blue the extra swept approach/turn space, and red uncovered cells. Check the separate machinery/wagon price,
later geological charge, Earth/Rock tonnage, supports and work estimate. **Commission the complete area plan**
buys only a currently valid complete plan; previewing buys nothing. Start the project through its portal window.
The planner permits at most 45 degrees per advancing segment: a 90-degree change needs two real bends and enough
clearance. The machine backs along its support instead of rotating or excavating backwards.

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
- When the next cut will not fit in the available Earth/Rock capacity, it **backs up** along its own track into the
  portal; it need not fill every carriage to the brim and never turns around underground.
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
- **Pause / Resume** holds the project where it is. **Cancel** (asks first) stops digging for good: paid-open ground
  remains open, unfinished cut progress is abandoned, the machine retreats and the actual carried spoil still goes
  through exchange and Surface disposal. Cancellation does not erase wagons or refund completed excavation.
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

- Both corridor and rectangular area controls exist. GUI purchase selects the compatible default cutter and wagon
  set; it has no arbitrary model/count picker. The synchronized script API accepts explicit compatible counts.
- Deep unlocks on **1 January 1980**. Its upper Underground assembly and approach floor must first be excavated;
  the automated Deep loop supplies that predecessor explicitly, so it is not normal-newgame bootstrap proof.
- The selected forty M1 art families are packed and loaded, including cutter/support, wagons, portals, receivers,
  service depots and controls. [Native gallery](assets/reviews/m1-runtime-gallery.html) preserves the technical review;
  personal approval and later conventional/tube/geothermal/orbital art remain pending. Padlocks belong to locked
  purchase/control previews, not moving vehicle bodies.
- Actual source-magazine and underground-depot exchange loops are tested. Arbitrary junction/signalling layouts,
  a real multiplayer join, maximum-map performance and normal-newgame end-to-end acceptance remain unverified.
- Cutting and spoil disposal have measured costs and no spoil-profit income. Mineral-chain payback and full
  economic/performance tuning remain separate acceptance work.
- Tube capsules, geothermal plant/riser final use and orbital gameplay are not delivered by this queue. Safely
  withdrawn, open ground can serve delivered conventional construction; do not remove occupied support or expect
  a tube/plant tool to finish an unsupported final-use scenario.

## If something goes wrong

Read **Next**, **Held**, the hauler status and the failed command's message before changing the layout. A greyed
button needs its stated predecessor; buying another train does not repair a stale assignment automatically.

| Reason shown or observed | Supported action |
| --- | --- |
| Project paused or actual machine manually stopped | Resume the project and start the actual train when appropriate. The project controller cannot move a manually stopped train. |
| Insufficient cash | Restore company funds, then resume. Completed ground/spoil remain owned; a refused cut adds no partial debit or cargo. |
| Next cut exceeds free wagon capacity / waiting for empties | Assign/start a compatible empty hauler and allow the stopped whole-consist exchange. Larger Rock capacity is particularly important for Deep. |
| Receiver full or intake interval exhausted | Leave loaded wagons at the Surface delivery leg while bounded processing/intake makes room; keep both receivers accepting the required cargos. Cargo remains aboard until accepted. |
| Origin no longer accepts both cargos | Restore its receiver catchment and route. If the station itself was deleted, the tested synchronized origin-replacement command can bind a replacement, but the portal window has no replacement-origin picker yet; automatic recovery is not promised. A reused numeric ID is not the old owner. |
| Exchange bay or assigned train lost | Restore a valid owned bay/route or compatible train, then reassign. Source-side exchange is the delivered first-cycle path before an underground bay exists. |
| Invalid turn, occupied swept space or uncovered area | Change the requested outline or clear the obstructing owned construction safely. Requote a complete plan; a 90-degree pivot on one tile cannot become a legal bend. |
| Retreat blocked / occupied support | Clear the real obstruction and let the train back out. Do not demolish support under any part of the consist or bypass the refusal with ordinary track in solid rock. |
| Deep locked | Wait until the calendar reaches 1980 and provide excavated upper access; changing the camera cannot unlock commands. |
| Cancelled project | Finish retreat, actual spoil exchange/disposal and empty return before release/retirement. The remaining open floor stays available; cancellation does not resume drilling. |

Actual lost-origin recovery, partial-bend reload and cancelled final disposal/empty return are retained in
[the ground persistence evidence matrix](r2mlvs14-savegame-determinism-and-lifecycle.md#ground-persistence-evidence-matrix).
If progress still appears stuck, save the game and record the exact status, company funds and train orders. Current
saves retain the project, headings, support, assignments, wagon/packet identities and accounting. Preserve the
original save when reporting the problem.
