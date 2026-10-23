> **Historical donor snapshot — not an execution plan.** OpenTTD3 `75ea75718f`, imported2026-10-23.
> Original: `docs/zz-reach2/upgrades/2026-10/d2um-underground-mines.md`; exact source SHA256 `13ae6d4e68303e9bc1c174adcb39fc02bb6dfd0174df334ed9a0730c5057d06e`.
> Current integration: [M2O23-1](../../../../../../../zz-reach2/upgrades/2026-10/m2o23-1-come-space.md); storage and evidence roles: [archi-historical-debug.md](../../../../../../../zz-reach2/architecture/archi-historical-debug.md). Original completion claims, instructions and human-review limits below remain dated; they grant no new execution, spending, publication or deletion authority.

# Multilayer demo - underground mines

Player walkthrough for the Underground and Deep Underground: portals, boring machines, excavation, mines, conventional freight, transport tubes and geothermal construction and service. It is owned by the [underground master](r2mlvs3mp-master-plan.md) within the [multilayer epic](r2mlvs-multilayer-vehicles-and-structures.md). Architecture: [map layers](../../architecture/map-layers/archi-map-layers.md).

- **Dates.** Updated **2026-10-20** for completed underground/TBM runtime delivery, conventional/tube vehicles, geothermal flows, loaded art and the review stage. The retained TBM checkpoints were refreshed on **2026-10-16**, after the [2026-10-15 demo fixes](r2df-demo-fixes-interface-movement-and-planning.md) and the [face exchange](r2df2-face-exchange-and-demo-acceptance.md).
- **What else changed on 2026-10-16.** This guide took in the former player how-to (`r2mlvs-how-to.md`, now removed): starting the game, building a portal and boring machine from a new game, limits, troubleshooting and the demo questions. Its Space tours moved to the Space guide.
- **Where Space is explained.** Asteroids, probes, the launch hub, rockets and orbital construction are covered in [Multilayer demo - Space adventures](d2sa-space-adventures.md). Both guides list every launcher command, but each explains only its own part of the world.

**Current status.** Plans 3 and 4 are complete against their runtime requirements: sealed commissioning, paid excavation and spoil haulage, recovery, useful final land use, saves and real multiplayer have retained evidence. Selected conventional/tube vehicle gameplay and geothermal operation are delivered too. **Proceed with demos and reviews; prepare cleanup, keeping review/test inputs until their review is resolved.** The remaining master gates are your art/demo reviews and the fixture cleanup that follows them; the master and M1 milestone are still open. Art has a separate limit: 21 packed mine, conventional-freight and factory families still await native integration, as explained [below](#current-art-and-runtime-review). The `demo3` tag identifies an older playable baseline, not all subsequent work. Both Debug and RelWithDebInfo native test builds have passed their recorded checks.

The default demo starts with **a working Underground Coal mine and a mineral train delivering to a Surface power station**. The same world keeps the completed 50-cell public tunnel and the original TBM parked at its far exit. Separate checkpoints cover excavation planning, active spoil haulage, far-exit construction and Deep collector graphics. All prepared checkpoints start paused.

## Choose your tour

Run these from `D:\Codez\Sandbox\OpenTTD`:

```powershell
.\play.ps1 demo         # Underground Coal mine and freight: start here
.\play.ps1 tbm-plan     # TBM planning: try selecting additional work
.\play.ps1 tbm-work     # Active excavation; the hauler takes spoil at the face
.\play.ps1 tbm-exit     # Build the far exit and park the TBM there
.\play.ps1 1            # A new Multilayer Industry game, to build it yourself
```

Recommended order: **demo -> tbm-plan -> tbm-work -> tbm-exit -> tbm-tunnel -> collector-build / collector-ready**. These are separate starting points:

- Loading another checkpoint restarts its own world.
- Changing layers does not create missing facilities or merge cargo.
- Save a personal copy before experimenting.

For Space, start with the [Space guide](d2sa-space-adventures.md#choose-your-tour).

## Review the underground work

Use the named commands above so later menu additions do not change your starting point. Save each experiment under a personal name, then work through these checks:

| Review | Starting point | What to check |
| --- | --- | --- |
| Planning and controls | `tbm-plan` | Area selection, paid approach and turning space, clear quotes and refusal messages, Change work area, layer keys and window controls. |
| Cutting and haulage | `tbm-work` | Actual Earth/Rock loads, repeated face transfers, Surface disposal and empty return; pause/resume and return/park without lost cargo or misleading status. |
| Completion and ordinary traffic | `tbm-exit`, then `tbm-tunnel` | Whole-machine withdrawal, final haulage settlement, paid support conversion, an open public route and the retained parked TBM. |
| Mines and conventional vehicles | `mine-build`, then `demo` | Construction, finite reserves, loading, wagon art, portal crossings, useful delivery and empty return. |
| Collector construction | `collector-build`, then `collector-ready` | Construction stages, completed machinery, picking and status text. These saves contain a collector, not a full operating powerhouse. |
| Tubes and geothermal operation | [Transport tube steps](#build-and-run-transport-tubes) and [geothermal steps](#build-and-run-geothermal-service) | Real paid power, useful cargo/service, visible waits and recovery. These later systems have no named featured tour in the current launcher. |

Review the [current 69-family native gallery] (historical reference: `../../../../logs/proofs/mlm/native-art/selected-gallery-bootstrap-20261019/gallery/m1-runtime-gallery.html`) alongside the game. Check directions, loaded/empty and working/waiting states, zoom, portal/vehicle occlusion, picking, anchors and sound. The gallery records technical captures; it does not record your approval.

Record findings against the [refreshed-demo review checklist](r2df2-b-acceptance-notes.md#review-checklist-task-116). Include the named save, selected layer, exact action and what you expected. If a route looks correct, also save and reload during work or travel and check that it continues consistently.

**Cleanup order.** Cleanup can be prepared now. Keep featured demo inputs, supplied and compatibility saves, malformed/adverse test cases, the gallery and its required review inputs available while reviewing. Master task 12.6 schedules selective fixture pruning after the refreshed-demo feedback is resolved, using named consumers and distinct coverage. The [prepared inventory](../../../../../../../../src/tests/fixtures/checkpoints/underground/openttd3-75ea75718f/historical/76fe933adf33d518-fixture-retention-inventory.json) lists candidate references and identical-byte groups; it does not authorize deleting them. Use personal copies for experiments.

## Start the game

From the repository folder, run the launcher [play.ps1](../../../../../../../../src/tests/fixtures/cleanup/source-epochs/openttd3-75ea75718f/cb5a151ec99c6efc-play.ps1):

```powershell
cd D:\Codez\Sandbox\OpenTTD
.\play.ps1
```

It shows a numbered menu. Type a number and press Enter; Enter alone takes 1.

- **1, New game, Multilayer Industry:** a 256 x 256 temperate map with the [new game settings](#new-game-settings) made, and a maximum loan of 20,000,000.
- **2, New game, Classic.**
- **Demos (Multilayer Industry):** the permanent featured checkpoints, listed in [All launcher commands](#all-launcher-commands).
- **Saves we have:** your `makinpr2` games. They predate the profiles, so they load as Classic games: portals, elevators and trains across the layers, but no digging.
- **More saves:** the newest saves in `build/save`, including the ones you make and the autosaves.

Options:

- `.\play.ps1 1` starts an entry without the menu; `.\play.ps1 tbm-work` starts a demo by name.
- `-Seed 1234` fixes the map of a new game.
- `-Config Debug` runs the Debug build. It is slow, but it stops at any internal check that fails, which helps when you report a bug.
- `-All` also lists the saves the tests generated.
- `-DryRun` prints the selected save and command line without launching.

The launcher runs the game in the `build` folder with the config `build/play.cfg`, which makes `build` the game's own folder:

- it finds the base graphics (`build/baseset`) and the content package (`build/newgrf/multilayer_industry.grf`) there;
- your saves go to `build/save`, where the menu finds them again.

**Settings and saves.**

- **Without a config.** Started without a config, the game keeps its settings and saves in `Documents\OpenTTD` instead.
- **Settings you change.** The game writes them into `play.cfg` when you quit, and they stay. Picking a new game only sets the profile there, and the temperate climate for Multilayer Industry.
- **Builds.** The default executable is the **RelWithDebInfo** game; `-Config Debug` selects the Debug executable. The launcher reports when a source timestamp is newer than the chosen executable. Check that hint before reviewing a recent change; the recorded native test passes and older featured-save checks keep their own build identities.
- **Featured saves.** Featured entries prefer the retained repository snapshot, so a personal or generated save of the same name cannot replace the reviewed starting state.

## Keys: layers, F2 and Escape

**Layers.**

- The **layer button** on the main toolbar picks the layer you see: Surface, Underground, Deep Underground or Outer Space.
- **Alt+1**, **Alt+2**, **Alt+3** and **Alt+4** show them directly, in that order; a layer the map does not have is ignored. **N** cycles through them.
- The Debug build's own debug keys moved to Ctrl+Alt+0, 1 and 2.

**The small map.** It follows the main view: every layer change by key, **N** or the layer button switches it too. Its own layer button can still show another layer until the next change. It can be made short, because its legend is sized for the map type shown, and its contour legend lists the map's real heights.

**Windows.** Two settings in **Settings -> Interface -> General** control how windows close from the keyboard:

- **Pressing a window's hotkey again closes it** (default on). Press **F2** to open Game Options and **F2** again to close it.
  - This works for any toolbar key that opens a window, once that window is in front.
  - Keys that open no window (pause, fast-forward, zoom, screenshots, layers) are unchanged.
  - A build toolbar showing another rail or road type switches type instead of closing.
- **Close window with Escape**: No, Yes, or **Yes, except sticky** (the default).
  - Escape first cancels a tool being placed, such as an area draft.
  - With no tool active, it closes the front-most window that has a close button, except the main toolbar, status bar and news. Under the default, pinned (sticky) windows stay open.
  - A text box being typed into keeps Escape for itself.

**Close window on right-click** and the **Delete** key, which closes every unpinned window, work as before.

## All launcher commands

This table is the same in both guides. Underground entries 9-17 are explained here; Space entries 3-8 and 18-30 are explained in the [Space guide](d2sa-space-adventures.md).

If a retained fixture is missing, later menu numbers shift: use the named command in the same row, or run `.\play.ps1` to see the current menu. Ordinary personal saves appear after the featured demos.

| No. | Named command after `.\play.ps1` | Starting point and first experiment | Guide |
| --- | --- | --- | --- |
| 1 | (menu only) | New Multilayer Industry game, 256 x 256 temperate, maximum loan 20,000,000. | [Underground](#build-it-yourself-from-a-new-game) |
| 2 | (menu only) | New Classic game: layers, portals and elevators, but no digging. | [Underground](#start-the-game) |
| 3 | `space` | Three purchased mining probes approaching moving asteroids. Select Outer Space; follow a probe. | [Space](d2sa-space-adventures.md) |
| 4 | `space-mining` | Actual extraction underway. Watch holds fill and compatible retargeting. | [Space](d2sa-space-adventures.md) |
| 5 | `space-holds` | Three full holds. Inspect cargo and saved upkeep; this older world has no prepared paid return pods. | [Space](d2sa-space-adventures.md) |
| 6 | `space-hub` | Bare paid Surface center at (16,4), joined rail and linked train/lorry stations. Toggle collection/link controls. | [Space](d2sa-space-adventures.md) |
| 7 | `space-survey` | Historical ownerless survey preview approaching bodies; use 21 for actual company knowledge. | [Space](d2sa-space-adventures.md) |
| 8 | `space-attached` | Same historical preview already attached; no cargo or company scan receipt. | [Space](d2sa-space-adventures.md) |
| **9** | **`demo` / `underground-mines` / `mine-haulage`** | Working Coal mine, mineral train and Surface receiver. Follow another delivery. | Underground |
| 10 | `mine-build` | Same route with paid mine machinery unfinished. Unpause before expecting Coal. | Underground |
| 11 | `tbm-work` | Accepted 50-cell job at 0/50 cuts with enabled hauler. Watch cutting, and the hauler taking the spoil at the face. | Underground |
| **12** | **`tbm-plan`** | Earlier 44-cell job, stopped trains. Follow the selection tutorial below; append up to six straight cells. | Underground |
| 13 | `tbm-exit` | All 50 cuts complete; machine stopped clear of the far exit. Build B at (13,60), then park and release safely. | Underground |
| 14 | `tbm-tunnel` | Completed public A-to-B route; original TBM parked at B. Follow the ordinary train. | Underground |
| 15 | `collector-build` | Deep collector at 74/296 labour ticks with construction materials already committed. Watch actual construction art. | Underground |
| 16 | `collector-ready` | Same completed Deep collector around (51,34). Inspect machinery and status glass. | Underground |
| 17 | `underground-mines-legacy` | Older bent TBM route and saved geology; compare physical advancing turns. | Underground |
| 18 | `space-workshop` | Paid spacecraft assembly interrupted. Open Surface center -> Workshop; pause/resume the working hull. | [Space](d2sa-space-adventures.md) |
| 19 | `space-launch` | Reusable rocket already preparing a launch. Watch deployment, return and turnaround; reload to try pre-burn cancellation. | [Space](d2sa-space-adventures.md) |
| 20 | `space-descent` | Mined cargo in an actual paid pod partway through descent. Follow receipt at the Surface center. | [Space](d2sa-space-adventures.md) |
| 21 | `space-survey-live` | Owned survey at 37/148 work ticks. Finish the scan and compare unknown/known resource quantities. | [Space](d2sa-space-adventures.md) |
| 22 | `space-resupply` | Cargo-only rocket ascending toward the same orbital probe, carrying actual service parts/pods. | [Space](d2sa-space-adventures.md) |
| 23 | `space-core-build` | Steel 16/Goods 24 delivered; two real builders; Core at 37/740 work. Complete and explicitly activate it. | [Space](d2sa-space-adventures.md) |
| 24 | `space-core` | Operational Core and panel, real inventory, two builders and a completed paid service receipt. Inspect exchanges and renewal state. | [Space](d2sa-space-adventures.md) |
| 25 | `space-core-service` | Actual Core service in progress. Try pause/resume, panel loss and cancellation. | [Space](d2sa-space-adventures.md) |
| 26 | `space-module-reserved` | Storage owns Steel 8/Goods 8 exclusively; competitor empty. Queue a builder or try the competing reservation. | [Space](d2sa-space-adventures.md) |
| 27 | `space-module-work` | Storage at 39 whole work ticks plus saved fractional work; competing Core service paused. Resume that service to share real panel output. | [Space](d2sa-space-adventures.md) |
| 28 | `space-module-power` | Same partial work with panel disabled. Unpause to see it wait; toggle the Core panel to resume. | [Space](d2sa-space-adventures.md) |
| 29 | `space-module-frame` | Finished Storage frame: inputs consumed once. Activate it, launch freight straight to a module, build a Dock, or remove the empty frame. | [Space](d2sa-space-adventures.md) |
| 30 | `space-module-cancelled` | Cancelled materials physically returned; replacement blueprint separately paid. Compare old and fresh job progress. | [Space](d2sa-space-adventures.md) |

For example, `.\play.ps1 tbm-work` is the stable named equivalent of `.\play.ps1 11`.

- **Versions.** Entries 11-14 (`tbm-work`, `tbm-plan`, `tbm-exit` and `tbm-tunnel`) start checkpoints refreshed on 2026-10-16 at save version **425**, which adds the face wait.
  - The ordinary game loaded and saved them, and every native owner chunk is unchanged. Their 403 originals stay beside them.
  - The other four underground snapshots keep format 403, and legacy 17 is 393. The Space guide lists the Space versions.
  - The current save format is **434**. Current builds read these older checkpoints and write personal saves in the current format; the featured originals keep their recorded versions.
- **Immutable starting points.** The launcher's featured saves are never overwritten. Personal saves go into `build/save` and should use different names.
- **Manifests.** Save identities and preparation conditions are in the [underground manifest](../../../../../../../../src/tests/fixtures/legacy/openttd3-75ea75718f/670aef5a07d8995e-demo-checkpoints.json).
- **Prepared conditions.**
  - The mine world has prepared workshop floor. Its mine, train, wagons, stations, portal and receiver were bought through native commands, and Coal comes from the finite natural deposit.
  - Collector inputs and floor are explicitly prepared.
  - These tours do not claim a complete normal-newgame bootstrap.

## What changed since the old guide

The checked commit history supplies these milestones. The Space milestones are in the [Space guide](d2sa-space-adventures.md#what-changed-for-space).

| Commit | Delivered scope |
| --- | --- |
| `0840083c40`, 2026-10-12 | Collector graphics in the game and further ground asset source work. |
| `c99b6f0d55`, 2026-10-12 (`demo3`) | Eight paused underground/mine/collector checkpoints; actual Coal delivery; chemical works source art. |
| Demo refresh, 2026-10-16 | The [2026-10-15 demo fixes](r2df-demo-fixes-interface-movement-and-planning.md) and the [face exchange](r2df2-face-exchange-and-demo-acceptance.md). They bring a small map that follows layers and can be made short, Alt+1..4, F2/Escape window closing, "Waiting:" and "Carrying:" lines, loaded haulers leaving under normal signals, a correct packet split for refused spoil, and a cutter freed from false obstructions. They also add paid angled approaches, Change work area at any time, the hauler taking the spoil at the face, and the `tbm-*` checkpoints at save version 425. User review is pending: see the [review checklist](r2df2-b-acceptance-notes.md#review-checklist-task-116). |
| Current workspace, 2026-10-20 | Completed portal/TBM runtime plans, actual paid tube and geothermal final use, genuine normal-newgame Standard/Deep loops, real ground/tube/thermal peers, selected performance acceptance and 69-family native art. The older featured saves remain distinct prepared tours. Art/demo feedback and review-gated fixture pruning remain open. |

## Completed features you can demonstrate

These are delivered scopes, rather than completion of the entire master or graphics profile.

| Completed feature | Where to see it |
| --- | --- |
| Layer switching and separate Surface/Underground/Deep world state | Every checkpoint. Deep access and operation are gated until 1 January 1980; use the selector or Alt+1..4 to choose the intended layer. |
| Surveyed material deposits, saved regional hardness and finite reserves | Mine tour and Underground Construction -> Survey. Soft/Mixed/Hard/Basalt affect excavation; surveying creates no cargo. |
| Paid TBM commissioning, actual machinery/support wagons and a saved excavation project | `tbm-work`, Projects and its portal controls. |
| Straight/area excavation preview and extending the same machine's accepted work | `tbm-plan`; numbered cutter positions, extra working floor, spoil, time and cost appear before confirmation. |
| Paid angled approaches to a floor beside or away from the exit | **New area** at a portal; blue cells and the quote's **Approach and turning space** line. |
| Change work area while the machine works, is paused, returns or is parked | `tbm-work` after its first cut: Projects -> **Change work area**. |
| Paid cuts, protected temporary support, actual Earth/Rock cargo, and the hauler taking the spoil at the face for Surface disposal | Unpause `tbm-work` and follow both trains. |
| Pause/resume, return/park, selected hangar, safe release and paid permanent-track conversion | Projects/portal controls; `tbm-exit` provides the far-exit handover. |
| Connected cross-layer public rail and ordinary freight after safe TBM withdrawal | `tbm-tunnel` and the coal shuttle in `demo`. |
| Ten resource-specific native mine definitions, matching-seam funding and finite extraction | The prepared Coal mine; try another properly excavated site using the instructions below. New selected mine artwork is packed, with native display still pending. |
| Conventional locomotive, mineral/sealed wagons and open/sealed road freight capability with cargo/date/layer checks | The coal shuttle demonstrates locomotive/mineral-wagon movement, loading and unloading. Other carriers can be purchased or refitted in compatible owned depots. |
| Native Surface metal refinery, chemical, battery and electronics recipes with real input/output stock | Fund compatible Surface sites and deliver their required cargo. The default Coal route uses the original power-station coal receiver, rather than one of these processing chains. |
| Actual paid/material-built Deep collector, native construction stages and completed game graphics | `collector-build` and `collector-ready`; construction advances through its actual labour owner. |
| All five dated tube capsule models, native guideways/terminals/depots, paid power and actual intermodal cargo | Underground Construction -> **Transport Tubes**; [build and run a route](#build-and-run-transport-tubes) on already excavated floor. |
| Geothermal planning, transported construction inputs, both plant models, Heat/electrical utilities, risers and useful service | Underground Construction -> **Geothermal**; [build the complete service chain](#build-and-run-geothermal-service), following the displayed date, floor and material requirements. |
| Current-format save/reload for the demo's natural resources, jobs, vehicles, cargo, industry and utility owners | Save your run mid-action and reopen it; the prepared checkpoints also passed ordinary load/resave checks. |

**Tube and geothermal systems delivered after these demo checkpoints.**

- **Delivered.** All five dated tube models, terminals/depots, ordinary construction and controls, paid power, actual useful cargo and interchanges are implemented. The two geothermal plant models include actual construction supplies, operation, Heat/electricity ownership and useful Surface export through commissioned, date-valid risers. Cleared floor is still a prerequisite; these systems cannot bore rock or commission a TBM.
- **Tour limits.** The older mine/TBM/collector checkpoints below demonstrate their named scenarios. They do not automatically contain every later facility. [Plan17](r2mlvs17-underground-transport-tubes.md) and [Plan18](r2mlvs18-geothermal-energy-and-industry.md) retain the current workflows and exact paid acceptance evidence. Orbital work and personal art/demo feedback remain separate.

## Tour the mine and conventional freight

```powershell
cd D:\Codez\Sandbox\OpenTTD
.\play.ps1 demo
```

`underground-mines` and `mine-haulage` select the same starting point.

1. Before experimenting, **Save game** under your own name, for example `underground-mines-my-run`.
2. Choose **Underground** with the main toolbar's layer selector or **Alt+2**. The ordinary loader starts on Surface; the saved camera is aligned with the mine's Underground position.
3. Find the Coal mine at **Underground(8,9)** and its loading station at **(8,12)**. Click the mine to inspect its actual output and finite deposit; click the station and open its train list to find the coal shuttle.
4. Unpause. Watch coal collect, the mineral wagon fill, and the train use the connected portal at **(12,12)**. Switch to Surface to follow delivery to the station at **(15,12)**, beside the power station at **(15,14)**. Then watch the empty return.
5. Reopen your personal copy with `.\play.ps1 underground-mines-my-run`.

**Following the train.** The `demo` checkpoint starts immediately after a successful delivered load, so the train may at first be on Surface. Use the **loading station's train list** or the company's train list to open its vehicle window. Its centre-view control follows the actual train across layers. Its orders name the mine loading stop and the Surface unloading stop.

**What to compare.** Compare the mine's output, waiting station cargo, wagon load and remaining reserve over several journeys. Cargo should move through those owners, and reloading should preserve it.

**Keep the route working.**

- The Surface station must keep accepting Coal for delivery income.
- Keep the powered locomotive and compatible mineral wagon together, keep both station orders, and leave the portal connected.

**See the beginning.** Use `mine-build` instead. The route and train are already enabled; release the global pause and let the actual machinery construction finish before expecting ore. The new selected mine, conventional wagon and station artwork is packed but still awaits native display; this tour uses the existing presentation. A retained save also keeps its original vehicle and industry choices; loading it does not replace them with every newly available model.

**The old tunnel.** The completed A-to-B tunnel also remains in this mine world: A is at (13,10), B at (13,60), and its original TBM is stored at B. The dedicated Coal portal at (12,12) is separate from that route. Nothing in the Coal tour requires selling or replacing the retained TBM.

## Tour excavation and the spoil loop

Open `.\play.ps1 tbm-work`, select Underground, then:

1. Open **Underground Construction -> Projects** and select the project for **Train#2**. **Open machine** opens the actual cutter; its centre-view control follows it even while it is in the Surface magazine. **Train#1** is the spoil hauler.
2. Choose **Portal controls**. Read **Next:** and the **Waiting:** line (or **Working normally**) before changing the layout. The Projects list also shows what the cutter and the hauler carry.
3. Unpause and watch cutting.
   - When the hoppers are full, after cuts 11, 25 and 35, the machine stops at the face (**waiting for the hauler at the face**).
   - The hauler drives down its track, stops behind it, takes the spoil across in about two seconds and reverses out, and the machine carries on cutting.
   - Follow the hauler to Surface disposal and its empty return. The accepted project and both trains are already enabled. Fast-forward shortens the wait.
4. Pause the project independently of the whole game, resume it, then save and reload your own copy during a wait. Compare the physical wagons, floor and project progress.

**The palette.** On Underground, the main toolbar's airport-construction position becomes the **TBM icon** for **Underground Construction**.

- The palette contains **Survey**, **Projects**, **New area** and **Fund a mine**.
- **Requirements and cutter dates** expands optional guidance.
- Escape clears the active draft or footprint, and a second Escape closes the front-most window.

**Change work area.** In **Projects**, it replaces the cuts the machine has not made yet with a new floor, while the machine keeps working: no pause, return or parking.

- It is available once the machine has finished its first cut, whether it is working, paused, returning or parked.
- The quote states the completed cuts kept, the planned cuts replaced, the new cuts, any share of the cut in progress abandoned, and the approach.
- If another cut finishes before you confirm, draw the area again.

**Extending `tbm-plan`.**

1. Choose **Extend work** or **Pause and extend**, then **Straight tunnel**, and click an endpoint directly ahead. Six more cells reach Underground(13,60).
2. Review the numbered route, charged working space, cost, cutting time and spoil before **Add work to this machine**.
3. Start both trains if stopped, resume the project and unpause.

Cancelling the draft leaves earlier accepted work intact and buys no second TBM.

### Select work successfully: start with save 12

1. Load **12 / `tbm-plan`** and select **Underground**. Open the TBM-icon
   **Underground Construction** palette, then **Projects**.
2. Select the existing Train#2 project. Press **Extend work** or **Pause and
   extend**. This selects its actual portal/machine and leaves that project
   paused. **Global pause alone is not the project's pause control.**
3. In the planning palette's work-shape dropdown, choose **Straight tunnel**.
   Click one endpoint **directly ahead of the accepted route**, up to
   Underground(13,60). This mode uses a single click, not a dragged rectangle.
4. Review the numbered cutter positions and quote. Press **Add work to this
   machine** only when the whole plan is accepted. No second machine is bought.
5. Open **Portal controls**, **Resume** the project, start any stopped machine
   and hauler, and release global pause. Confirming the draft alone does not
   start excavation. Escape cancels only the draft and leaves the project paused.

### Select a digging area: the exact mouse sequence

A rectangle describes **floor to excavate**. It does not instantly remove rock,
lay normal rail or buy a machine. Use **Area clearing**, not Straight tunnel.

**For an existing project.**

1. Start from **Projects -> Pause and extend** as above, or from **Projects ->
   Change work area** to replace the remaining work without a pause. The portal
   is already selected.
2. Hold the **left mouse button** on one floor corner, move to the opposite
   corner while holding it, and **release**. The quote is created on mouse
   release.

For a first small experiment, request a narrow three-cell strip immediately
beyond the last accepted cutter position, in the same forward heading. The
planner adds the cutter's required width as working space. Reload 12 before
switching between alternative experiments.

For a **new machine at an unused portal**:

1. Build a **Sealed** portal, its Surface line, the two receivers and a hauler,
   as in [Build it yourself](#build-it-yourself-from-a-new-game). The current
   controls automatically choose a compatible cutter and the nearest eligible
   owned Surface station; there is no manual origin picker in this area tool.
   Deep work requires 1980 or later.
2. Switch to the destination rock layer. Open **Underground Construction** and
   press **New area**. **First click the actual rock-side portal housing.** A
   click on an ordinary rock tile at this stage only repeats the portal prompt.
3. Wait for **Drag the intended final floor on this layer**. Keep **Area
   clearing** selected. Left-drag the intended rectangle and release. Start
   with the three-cell strip directly forward of the portal, leaving room on
   both sides for the cutter. A floor beside the exit or off the portal's axis
   is also accepted: the planner adds a paid approach, straight out of the
   door and then 45-degree bends.
4. Read the quote and overlay: **white = requested floor**, **blue = additional
   charged working space, approach included**, **red = uncovered/refused
   floor**. Numbers are the cutter's successive positions. The quote includes
   Earth/Rock, work and cost, and an **Approach and turning space** line.
5. Only a complete valid plan enables **Commission the complete area plan**.
   Confirm, open the portal controls and **Start** the project. Press **Assign
   hauler**, then click the actual hauler in the map, depot or train list.
   Start that train and unpause globally. Previewing and surveying never
   excavate the floor. Read **Next:** whenever the machine waits.

**Other ways in.**

- **A straight corridor.** For a simple new straight corridor, the portal window
  also offers a length of 2-60 cells and **Commission TBM**; it uses the same
  physical logistics.
- **The endpoint tool.** For the graphical endpoint tool, select **Straight
  tunnel** after selecting the portal, then click a forward endpoint. That mode
  does not accept a floor drag.
- **New area at a portal that has a project.** On an unfinished project it opens
  **Change work area** for that project. On a finished project whose machine is
  parked, it offers **Release and plan new work** with the same machine.

Switching layers, selecting another tool, closing the palette or pressing Escape
can cancel the placement tool; re-arm it before drawing again.

| What you see | What to try |
| --- | --- |
| Dragging produces no quote | Check the correct layer, armed Area clearing tool and selected portal. Hold left-drag and release in the map, outside the palette. |
| "Click an owned rail portal..." | Click the rock-side housing first. For an existing project, enter through Projects -> Extend work or Change work area. |
| "This portal's TBM project cannot take a new area now" | The project is finished, parking or missing its machine. Manage it from Projects; New area commissions a separate machine at an unused eligible portal. |
| "Has not finished its first cut yet" (Change work area) | Wait for the first cut, or use Pause and extend to add work after the plan. |
| "No Surface origin" / no available cutter | Provide an owned Surface rail station accepting Earth and Rock, a valid sealed portal, and an available cutter for this layer/date. Defaults are selected automatically. |
| Red cells or disabled confirmation | The planner already adds a paid approach where one is legal. Read the reason: the quote marks the tile where planning stopped with **!** and counts the reachable floor cells; the error names the map edge, a building, other work, or an area too far away. Move, shrink or bring the rectangle closer, allowing the full cutter/consist width and advancing 45-degree turns. |
| Selected floor already covered | Request new floor beyond the accepted route; selecting existing planned floor adds no work. |
| Quote becomes stale | Redraw after project, terrain, owner or money changes; old quotes cannot authorize new work. |
| Accepted but nothing moves | Resume the project, start both trains, unpause globally, then read **Next:** and the **Waiting:** line. "Waiting: Train 1 is in the way" names the train or tile; the machine carries on once it is clear. Check matching empty Earth/Rock capacity and Surface disposal orders. |
| Ordinary rail refuses solid rock | Use the TBM to earn floor first, withdraw safely, then release/convert support before ordinary construction. |

Look for these details:

- **Earned floor.** Newly open floor must be earned before ordinary rail can use solid rock. Occupied temporary support stays protected.
- **Whole consist.** The cutter advances and backs with its whole consist; it cannot spin on one tile. Use the legacy bent route to inspect legal advancing bends.
- **Yields and time.** Soft, Mixed, Hard and Basalt cuts yield Earth/Rock 3/6, 2/12, 1/20 and 1/24 tonnes respectively. Cutting takes twice the former active time; travel and haulage waits are separate.
- **Costs.** Surface disposal charges money. Pausing or reloading should not remove loaded spoil or buy free cuts.
- **Face transfer.** At the face, whole cargo packets move from the cutter's hoppers to the hauler's; no wagon changes train. Only one hauler enters the tunnel at a time, and the cutter never backs past it.
- **Signals.** The loaded hauler leaves its Surface depot under ordinary signals: a portal that is not Connected ends the signal block, so the machine below never holds it.

## Build the far exit and retain the TBM

`tbm-exit` skips the cutting wait. All 50 cuts are complete and the whole machine is stopped clear of **Underground(13,60)**. A remains at (13,10).

1. Leave the cutter stopped. From Underground rail portal construction, choose Surface and preview B at (13,60). Select the mouth orientation that joins the retained approach; inspect both endpoint footprints and the price.
2. Build B and **Connect** it when its actual approaches permit. If the hauler is still in the tunnel, the handover refuses with "Wait for the hauler to leave the tunnel first".
3. Choose B as the machine's hangar, then **Return and park**. Let the original hauler settle remaining spoil through its assigned origin.
4. Once the whole machine is stored and release guidance allows it, **Finish/cancel and release** the project.
5. Pay to **Make track permanent** where needed, then **Connect** A. The stored TBM remains available for later use.

`.\play.ps1 tbm-tunnel` starts after this sequence, with an ordinary train already running between both ends.

- **Finding the machine.** The released project is no longer in Projects; find the parked machine through the train list.
- **The controls.** **Pause** holds current work, **Return and park** preserves it for later, and **Finish/cancel and release** ends it safely.
- **Recovery.** [If something goes wrong](#if-something-goes-wrong) explains the recovery reasons.

## Tour the Deep collector

```powershell
.\play.ps1 collector-build
# Or skip to the completed machinery:
.\play.ps1 collector-ready
```

Choose **Deep Underground** in the layer selector, or press **Alt+3**. The collector occupies the area around **(51,34)**, and its machinery anchor is (50,33). If necessary, open the industry list and centre on the collector.

- **`collector-build`.** Inspect the incomplete hardware before unpausing. All actual recipe inputs have already been received and committed, and 74 of 296 native labour ticks are complete. Unpause to watch the remaining paid construction stages and the completed apparatus. Pause again to compare the construction silhouette and status glass.
- **`collector-ready`.** It provides the complete state immediately.

This is the newly packed collector artwork in the real game, rather than a source-art contact sheet. Its natural vent and claim and its bounded Heat accounting are implemented.

This retained checkpoint is a collector-only world; it does not include a finished powerhouse or complete electrical export route. Those later plant/construction/control and useful-export flows are now delivered in [Plan18](r2mlvs18-geothermal-energy-and-industry.md), with actual transported first-plant supplies and native paid risers. To obtain useful service you still need the complete commissioned, date-valid plant/riser/consumer chain; the collector alone does not provide it.

## Try another mine or material chain

Underground resources are **Coal, Iron Ore, Copper Ore, Bauxite and Salt**. Deep resources are **Lithium Ore, Nickel Ore, Cobalt Ore, Rare Earth concentrate and Uranium Ore**, after the 1980 gate. A geothermal vent is a separate renewable heat source.

1. **Survey a matching seam** on the intended layer.
2. **Excavate a real site** with the TBM.
   - A mine needs 3x3 machinery cells and an adjoining 2x2 loading approach; the narrow 50-cell corridor alone is not enough.
   - Withdraw safely, and release or remove temporary support from the machinery footprint.
3. **Fund the matching mine** from Underground Construction -> Fund a mine. Funding rechecks floor, seam, exclusive claim, date, owner and money; it cannot excavate its own foundation.
4. **Build compatible freight infrastructure** on open floor. Connect an ordinary station, depot and route to a genuine receiver, preserving portal approaches and machinery loading access.
5. **Buy or refit compatible vehicles**, and assign collection and unloading orders.
   - Ordinary mineral wagons cover compatible ores; Uranium requires a sealed carrier.
   - Follow the date, layer and cargo refusals rather than changing a vehicle's meaning through its appearance.
6. **Watch reserve, stock and cargo**, then save and reload during loading or travel. On Surface, try supplying the implemented refinery, chemical, battery and electronics recipes with actual transported inputs.

## Build and run transport tubes

Use a Multilayer Industry game at the displayed technology date: Underground tubes start in **1970**, Deep in **1980**. Individual capsule designs have their own introduction dates. A new game starting in 1980 is convenient for trying Deep and the early systems; check later designs in **Designs and refits**.

1. **Earn the floor with a TBM.** Excavate the whole route, terminal/depot footprints and approaches. Withdraw the whole machine, settle its spoil and release the construction support before replacing it with tube infrastructure.
2. Open **Underground Construction -> Transport Tubes** on the intended layer. Use **Guideway**, **Terminal** and **Service depot** to build a connected route. Capsules stay on one layer; they cannot use ordinary rail, rail portals, road tunnels or elevators.
3. **Install paid power.** For an external inlet, choose **Draw power route**, click your commissioned portal, then its door cell and adjacent excavated cells in order, ending at your tube depot or terminal. Review the quote and click **Pay and connect**. The route is limited to 128 cells, without repeated cells.
4. **Buy and prepare a capsule** at the tube service depot. Use an available design and a permitted cargo/refit. The five models have different dates, capacities and cargo roles; the preview shows those limits.
5. Choose the installed inlet and your stopped capsule in the tube controls, then **Bind power inlet**. Open **Vehicle and orders**, set real loading/unloading orders and start it. Check both the cargo and power status if it waits.
6. **Demonstrate useful delivery and empty return.** Load actual mine or station cargo, deliver it to a compatible destination and watch the return. At an interchange, transfer cargo to conventional freight for a different layer or the Surface. TBM Earth/Rock disposal still belongs to the mandatory hauler/receiver loop.
7. **Try recovery on a personal copy.** Closing an inlet or breaking the physical route stops usable power; cargo remains aboard. Repair the route, buy a valid replacement connection if needed and use **Reconnect power** with the capsule stopped or waiting as permitted by its quote. Check that delivery continues, then save and reload.

For a Deep plant-powered route, first complete the [geothermal chain](#build-and-run-geothermal-service). Build its paid electrical cable path to the tube facility, choose **Geothermal inlet**, select the operating plant and connected inlet, then **Connect geothermal inlet**. Bind or reconnect the capsule to that selected source. A built guideway or a reserved plant plan alone supplies no electricity.

The retained `tbm-tunnel` is an ordinary public railway. It demonstrates withdrawal and rail use; it is not already a tube route. There is currently no named tube checkpoint in `play.ps1`.

## Build and run geothermal service

Open **Underground Construction -> Geothermal** on Deep Underground, available from **1980**, and select a facility allowed at the current date. The pages **Sites and buildings**, **Utilities and risers**, **Town service** and **Information** follow the actual construction and service chain.

1. **Inspect the vent and clear a real site.** Use **Inspect geothermal** or choose an existing vent. Excavate the full displayed footprint, access side and utility connection with the TBM, then withdraw and release its work safely.
2. On **Sites and buildings**, select the vent and facility family, choose its access direction, and click **Choose site**. Click the footprint corner and boundary utility connection cell. Review the preview and **Reserve plan**. Start with a **Vent collector**, then repeat the planning, funding and material delivery for a date-valid **Vent-Tap Powerhouse** or **Magma-Loop Power Station** on a compatible site.
3. Select the actual reserved plan from **Choose an existing plan**, check its capital quote and **Fund building**. Reserving or funding does not provide its construction materials or finish the work.
4. **Deliver the displayed material bill.** Buy required supplies through a [Surface station](#buy-construction-supplies-at-a-surface-station), then transport them to the facility using compatible freight and real access. Watch **Construction stock and work**; allow construction to finish before expecting operating output.
5. **Complete the utilities.** On **Utilities and risers**, use **Heat pipe** to connect the original collector and generator ports, and **Electrical cable** for the electrical route on excavated floor. Provide the plant's Heat supply and a real electrical destination. Each installed cell has its own quote and cost.
6. **Build the riser route for Surface export.** Use **Build electrical riser** and follow its selection of the original commissioned portal, machinery corner, boundary cell and access. Fund and supply each required riser; the portal alone is not a finished electrical connection.
7. On **Town service**, choose the completed plant, its compatible paid riser route, your Surface freight terminal and a town. Review and submit **Connect town service**. Alternatively, supply an eligible Deep tube inlet through the cable route described above.
8. **Check useful operation.** Unpause and inspect **Information** for construction, Heat availability, generated/used electricity, town use and the current waiting reason. Compare these with actual cargo/service activity. Save and reload while operating or waiting, then confirm that stock and work continue consistently.

The `collector-build` and `collector-ready` tours cover only the collector and its prepared construction inputs. They do not include a complete plant, cable, riser and useful destination. No named operating-powerhouse tour is currently registered in `play.ps1`; use these steps and the current native gallery to review the later delivered systems.

## Buy construction supplies at a Surface station

Current builds provide paid imports through [Plan6's Surface station service](r2mlvs6-mines-and-resource-chains.md#group-13ccb---expose-ordinary-paid-surface-station-supplies). Open one of your Surface railway or lorry stations and expand **Construction supplies**.

1. Choose **Steel**, **Refined Metals**, **Electronics** or **Batteries**.
2. Click **Quantity** and enter the complete number of whole cargo units.
3. Check the introduction date, current unit price, company allowance left today and complete order price. Prices follow the game's inflation.
4. Click **Buy supplies**. The purchased cargo waits in that station's ordinary queue. Refit and send a suitable freight vehicle to load it and deliver it to the receiver.
5. Use **Choose paid order** to inspect its paid, sent and refunded units. **Close order** refunds only stock that has never left the original pickup, at the price actually paid. Loading or reserving cargo permanently removes its refund eligibility; returning it to the pickup keeps that history.

Steel and Refined Metals imports start in1950; Electronics and Batteries in1965. The respective company-wide economy-day limits are256,128,64 and64 units, shared across all supply stations. Closing an order keeps its original allowance usage for that day. Station cargo weights follow your unit settings; the quantity editor uses whole cargo units.

Bought supplies earn no freight income or subsidy. Station removal follows normal cargo-loss rules and retires the old refund claim. Saved orders preserve their paid history and remaining real cargo, so loading the world again does not repeat a purchase or credit.

## Build it yourself from a new game

**The loop in one paragraph.** Start a new game with the **Multilayer Industry** profile; below the Surface there is now solid rock.

1. Build a rail portal down to Underground. It starts **Sealed**.
2. Near its Surface door, build a short line: track to the door, a rail station and a depot. Fund an **Earth Reclamation Site** and a **Rock Crusher** next to the station.
3. Click the portal, set the corridor length and press **Commission TBM**, then **Start**.
4. Build a compatible hauler train with matching empty Earth/Rock capacity and an order to the station, start it, and press **Assign hauler** to pick it.

From then on the two trains play the loop by themselves:

- The machine drives down through the shaft, cuts the rock, lays its own temporary track and fills its hoppers.
- When its hoppers are full, it waits at the rock face. The hauler drives down behind it with empty hoppers, takes the spoil over and reverses out. During the first cuts there is no room for the whole hauler behind the machine, so the machine backs into the portal instead and the hauler swaps hoppers there.
- The hauler delivers the spoil to the receivers while the machine carries on digging.

When the corridor is done, the portal window walks you through whole-machine withdrawal, final spoil settlement, release, paid track conversion and connection for ordinary traffic. You can keep the parked machine for another eligible job; selling it is optional.

Underground and Deep Underground start as **solid rock**, except where a machine has dug. The delivered rock, cut-face, cavity and resource-marker art distinguishes the two depths. **Survey geology** in Underground Construction reports solid, reserved or excavated ground, the mineral and the remaining reserve. Surveying opens no rock.

### New game settings

Entry 1 of the launcher makes these for you. From the game's main menu, set them in **New Game** (the Generate World window):

- **Gameplay profile: Multilayer Industry.** Classic gives you the old four-layer map with free building.
- **Climate: Temperate.** The profile refuses the other climates.
- **Map size: at least 64 x 64.** Smaller maps are refused.
- **NewGRF: none of your own.** The profile adds its own package and refuses other NewGRFs.
- **Start year.** The default 1950 is fine: the Standard TBM and both hoppers are available from 1950. Deep Underground, its rail portals and the Deep TBM unlock on **1 January 1980**.
- **Money.**
  - The Standard cutter's base purchase is 400,000 and the Deep cutter's 800,000, with wagons and setup charged separately.
  - The current measured fixture corridors cost about 2.54 million Standard and 3.07 million Deep, including setup, cutting, support, running costs, hauling and disposal. These are scenario totals; use the current construction quotes for your map.
  - Ordinary freight can finance the work from normal 100,000 cash/loan and the available 300,000 credit limit. The complete 1980 acceptance route earns its own construction money and upper access before commissioning Standard and Deep machinery; the [current audit](portal-tbm/r2mlvs3mp-z-b-final-closure-audit.md#current-selected-technical-reconciliation-2026-10-20) records its actual paid chain.
  - For a funded sandbox, you can raise the maximum loan (the launcher sets 20,000,000) and **Ctrl+click Borrow**, or use **Ctrl+Alt+C** to add money. Those shortcuts describe a sandbox, not the normal-finance acceptance route.

### Build a sealed rail portal

- Open the rail toolbar and use the **layer portal** tool (**P**). Its picker sets the direction the mouth faces. Placed on the Surface, a portal always leads down to Underground.
- A portal placed on Underground lets you pick Surface or Deep; Deep stays greyed out until 1980. It also needs dug ground there, so in today's game an Underground-to-Deep portal is hard to reach.
- The new portal is **Sealed**: its lower end opens onto solid rock, and ordinary trains treat its door as the end of the line.
- Leave free ground around the portal's Surface end; the machine is assembled there. Direct Surface-to-Deep rail is unavailable.

### Build the Surface line, the receivers and the hauler

The machine must have somewhere to send its spoil before you may buy it. All of this is ordinary building:

- **The line.** From the portal's Surface door, lay track out to a **rail station**, then on to a **depot** facing back towards the portal. The tests use a straight line out of the door; junctions and signals should work but are untested with a hauler.
- **The receivers.** In **Fund new industry**, build an **Earth Reclamation Site** and a **Rock Crusher**, both within the station's catchment, so it accepts both spoil cargos.
  - They are funded facilities with their own art and fixed six-cell and sixteen-cell layouts.
  - Both have bounded intake and processing. Spoil disposal costs money and pays no freight profit.
- **The hauler.** In the depot, buy the spoil-hauler locomotive and matching empty hoppers.
  - The current default Underground cutter has **two Earth and two Rock** wagons; Deep has **two Earth and four Rock**.
  - Give the hauler **one order: go to that originating Surface station**. Assignment rechecks its actual cargo, capacity and suitability.
- **Not yet.** Don't assign or start the hauler yet; that comes after the machine is commissioned.

### Commission and start the machine

**Click the portal** (either end) to open its window. It shows:

- the state (Sealed, Commissioning or Connected);
- the buttons **Connect**, **Seal** and **Commission TBM**;
- the **corridor length** with its arrows (Ctrl+click changes it by five);
- the project panel, with a **Next:** line that always says what to do now.

To get going:

1. Set the corridor length: the cells the machine digs straight out of the portal's far door, 2 to 60.
2. Press **Commission TBM**. It buys a Standard TBM (a Deep TBM at a Deep portal) with the layer's default wagon set.
   - The spoil goes to the nearest of your Surface rail stations that has both receivers in its catchment.
   - If it refuses, the message says why: money, the model or date, the receivers, the portal not being Sealed and free, or no route.
3. Press **Start**. The machine leaves the portal and drives down to the rock face. **Machine** opens its vehicle window, where you can follow it with the usual "centre view" button.

**An area instead of the corridor.** Open **Underground Construction** on the intended rock layer and choose **New area**, then follow [the exact mouse sequence](#select-a-digging-area-the-exact-mouse-sequence).

- **The overlay.** White marks the requested floor, blue the extra swept approach and turning space, and red uncovered cells.
- **The quote.** Check the separate machinery/wagon price, the later geological charge, the Earth/Rock tonnage, supports and work estimate. **Commission the complete area plan** buys only a currently valid complete plan; previewing buys nothing. Start the project through its portal window.
- **The approach.** The floor need not touch the portal. The planner works out a paid approach from the portal to the floor: straight out of the door, then advancing 45-degree bends.
  - The blue cells and the quote's **Approach and turning space** line show it: cells, price, Earth and Rock, and how many cutter positions use it.
  - Numbers on the map show the cutter's positions in order.
  - A **Straight tunnel** end off the portal's axis becomes an angled tunnel the same way.
- **When it cannot be reached.** The quote says how many floor cells can be reached and marks the tile where planning stopped with **!**. The error names the reason: the map edge, a building, other work in the way, or an area too far away (draw a closer one).
- **Turns.** The planner permits at most 45 degrees per advancing segment, so a 90-degree change needs two real bends and enough clearance. The machine backs along its support instead of rotating or excavating backwards.

### Assign the hauler

- Press **Assign hauler**, then **click your hauler train** in the main view, its depot or the train list.
- Start the hauler from its own window, as usual.

You can assign it any time before the hoppers fill. Until then the **Next:** line says "nothing, the machine is working". If the hoppers fill first, the machine waits for one, at the rock face or, during the first cuts, in the portal, and the line asks for a hauler. The window names what's wrong if the train isn't suitable, for example if it's the machine itself or already serves another project.

### Watch it dig

Switch the view to **Underground** and follow the machine.

**The cutting cycle.**

- **A cut.** Each cut takes a while. When a cut is done, the cells it covered are dug out, a piece of temporary track is laid, and Earth and Rock go into the hoppers. You pay as it digs, and a little more while it stands.
- **The next cut.** After each cut it drives onto its new track and starts the next one.
- **Full hoppers.** When the next cut will not fit in the free Earth/Rock capacity, the machine stops at the face, and its phase reads **waiting for the hauler at the face**. It need not fill every hopper to the brim, and it never turns around underground.
- **The face meeting.** The hauler drives in through the door and down the machine's own track (**bringing empty hoppers to the TBM**), then stops right behind it. After about two seconds the spoil moves from the machine's hoppers into the hauler's empty ones (**taking spoil from the TBM**); no wagon changes train. The hauler reverses out, and the machine goes on cutting at once.
- **Near the portal.** During the first cuts the machine is still too close to the portal for the whole hauler to fit behind it. It then **backs up** along its own track into the portal, and the hauler swaps its full hoppers for empties there. On `tbm-work` every exchange, after cuts 11, 25 and 35, already meets at the face. A route whose hoppers fill within its first few cuts makes those first exchanges at the portal.
- **One hauler inside.** Only one hauler is in the tunnel at a time; any other waits at the portal. The machine never backs past a hauler: a return, park or cancel waits until the hauler has taken the spoil and left.
- **Delivery.** The hauler takes the spoil to the station. The receivers take it in batches, so the hauler may wait there a little. Then it brings the empties back.

**What the windows show.** The portal's project panel and the **Projects** list of Underground Construction show:

- the phase and the cuts done;
- a **Waiting:** line saying what holds the machine, for example "Waiting: the project is paused" or "Waiting: Train 3 is in the way". **Working normally** means nothing holds it, and **Not waiting for anything** means it is parked, finished or following orders;
- the Earth and Rock dug so far;
- what the hauler is doing, and why it waits, if it does.

The Projects list also shows what is aboard: **Carrying: Earth 24/64 t, Rock 48/128 t** for the machine, then a "carrying" line for each hauler, or **No hauler assigned**. Widen the window if a line is cut short.

When something is in the way, the machine's own window names it too ("0 mph - Waiting: Train 3 is in the way", or the tile of a track or building). The machine goes on by itself once it is clear.

**Orders and controls.**

- **Service and travel orders.** The machine can take ordinary travel and depot service orders. They take precedence over new cutting, keep partial work and follow legal open track. A service visit resumes work afterwards; after explicit **Follow order** travel, use **Resume work** to return to the kept excavation.
- **Pause and park.** **Pause / Resume** holds work in place. **Return and park** withdraws to the chosen compatible hangar while preserving the job. **Choose hangar** offers eligible owned portal magazines and native depots.
- **Change work area.** It works once the machine has finished its first cut, while it is working, paused, returning or parked; no pause, return or parking is needed. Details are in the [excavation tour](#tour-excavation-and-the-spoil-loop). **New area** on the portal of such a project opens the same tool. Before the first cut, use **Pause and extend**.
- **Release and plan new work.** **New area** on the portal of a finished project whose machine is parked offers it: it releases the project and plans the new area with the same machine.
- **Finish/cancel and release.** It requests final settlement. Future claims stay protected until the complete machine has withdrawn and actual spoil has been delivered. Completed excavation and its costs stay recorded.
- **Ordinary trains.** They can't use the portal while the project holds it; they turn back at the door. Until the portal is **Connected** it also ends the signal block, so the machine below never holds a Surface signal or keeps the loaded hauler in its Surface depot.

### Finish and open the tunnel

When the corridor is done, use the project guidance to complete these steps:

1. **Return and park** at the original portal, a compatible depot or a selected alternative portal.
   - The entire train must arrive; a hidden engine with its tail outside is still occupying the route.
   - A machine at the face backs out along its track. With a hauler in the tunnel, it first waits for the hauler to take the spoil and leave.
2. Let the hauler deliver the last load and clear the work route. **Finish/cancel and release** checks both the actual cutter and logistics before releasing their claims. A refusal identifies what still needs attention.
3. **Make track permanent** converts released temporary support for its normal price. Build any required conventional approaches, then **Connect** the portal for ordinary trains.

The parked TBM keeps its identity and can take another eligible job. Its storage leaves the portal's public through-route usable; selling it is unnecessary.

**A far exit B.**

1. Complete its terminal excavation, back the whole cutter locally clear of the footprint and stop.
2. Use rail portal construction from Underground with Surface selected, or the reciprocal Surface tool. Review and pay for both endpoints and approaches.
   - Only safely vacated terminal support can be handed over, and only with no hauler in the tunnel ("Wait for the hauler to leave the tunnel first").
3. Connect B when its approaches are ready, choose it as the hangar and park the original machine there.
4. Settle spoil at its existing origin, release the project, convert the remaining support and connect A.

The `tbm-work`, `tbm-exit` and `tbm-tunnel` saves come from this same actual journey, refreshed at save version 425.

**Rates and yields.**

- **Cutting rates.** Standard 1/2 and Deep 3/4 geological work ticks per simulation tick: twice the former active cutting time.
- **Yields.** Whole-tonne Earth/Rock yields per new cell are Soft 3/6, Mixed 2/12, Hard 1/20 and Basalt 1/24.
- **Other values.** Travel speed, support costs and disposal prices keep their own values.
- **Hardness.** Surveyed material hardness matches the rock and face texture. New maps vary by saved geological region, while older maps keep their original difficulty.

## Limits today

**Overall.** The tours use prepared, funded worlds with existing logistics. Separate ordinary-newgame acceptance now completes both Standard and Deep paid loops, including earned finance/access, real spoil disposal, empty return and 64 tonnes of useful cargo. The first personal playtest was the 2026-10-15 demo, and its [review checklist](r2df2-b-acceptance-notes.md#review-checklist-task-116) is pending. Art feedback remains deferred. The [current final audit](portal-tbm/r2mlvs3mp-z-b-final-closure-audit.md#current-selected-technical-reconciliation-2026-10-20) records exact evidence and remaining limits.

- **Machine choice.** Both corridor and rectangular area controls exist. GUI purchase selects the compatible default cutter and wagon set; it has no arbitrary model/count picker. The synchronized script API accepts explicit compatible counts.
- **Deep.** Deep unlocks on **1 January 1980**. Its upper Underground assembly and approach floor must first be excavated. Older isolated tests prepare that predecessor; the separate genuine normal-world acceptance now earns it through paid Standard work. A prepared demo remains a prepared demo.
- **Art.** The current selected gallery covers 69 families/10,557 poses, including target terrain, portals, cutter/support/spoil units, tubes and geothermal structures/controls. Another 21 packed mine, conventional-freight and factory families await native integration.
  - The [current native gallery] (historical reference: `../../../../logs/proofs/mlm/native-art/selected-gallery-bootstrap-20261019/gallery/m1-runtime-gallery.html`) keeps the technical review.
  - Personal direction/state/zoom/occlusion/sound feedback remains pending. Later unrelated orbital/whole-profile art retains its own acceptance.
  - Padlocks belong to locked purchase/control previews, not moving vehicle bodies.
- **Loops tested.** Actual source-magazine, underground-depot, public freight, paid curved tube and irregular geothermal final-use routes are tested. Real ground/tube/thermal peers now cover joins, commands, pause/time modes and paid power failure/recovery; normal-newgame Standard/Deep acceptance is complete.
  - Arbitrary junction/signalling layouts and unrelated orbital workloads retain separate scope. Selected performance passes compact 128/representative 512/largest admitted 1024 Debug and 2048 optimized maps under its bounded workflow; 4096 engine support is not a measured passing claim. These are measured scenarios, not a guarantee for every layout or machine.
- **Economy.** Cutting and spoil disposal have measured costs and no spoil-profit income. Mineral-chain payback and full economic and performance tuning remain separate acceptance work.
- **Other systems.** Delivered tube capsules and useful plant/riser service retain separate paid construction, controls, file and real-peer evidence in the master and owning plans. Safely withdrawn open ground supports their ordinary construction; required floor, facilities, supplies, date gates and power still apply. Orbital gameplay remains outside this ground acceptance.

## If something goes wrong

Read **Next**, **Waiting**, the hauler status and the failed command's message before changing the layout. A greyed button needs its stated predecessor; buying another train does not repair a stale assignment automatically. Area-planning refusals are in the [mouse sequence table](#select-a-digging-area-the-exact-mouse-sequence).

| Reason shown or observed | Supported action |
| --- | --- |
| Project paused or actual machine manually stopped | Resume the project and start the actual train when appropriate. The project controller cannot move a manually stopped train. |
| Insufficient cash | Restore company funds, then resume. Completed ground/spoil remain owned; a refused cut adds no partial debit or cargo. |
| Hoppers full: **waiting for the hauler at the face**, or waiting for empty hoppers | Assign and start a compatible empty hauler. It meets the machine at the face, or at the portal during the first cuts. A hauler with too few empties is not let into the tunnel and says so at the portal. Larger Rock capacity is particularly important for Deep. |
| Loaded hauler stays in its Surface depot | A portal that is not Connected ends the signal block, so the machine below cannot hold the hauler. Look for a real train or red signal on the Surface line, and check the hauler's order. |
| Receiver full or intake interval exhausted | Leave loaded wagons at the Surface delivery leg while bounded processing/intake makes room; keep both receivers accepting the required cargos. Cargo remains aboard until accepted. |
| Origin no longer accepts both cargos | Restore its receiver catchment and route. If the station itself was deleted, the tested synchronized origin-replacement command can bind a replacement, but the portal window has no replacement-origin picker yet; automatic recovery is not promised. A reused numeric ID is not the old owner. |
| Exchange bay or assigned train lost | Restore a valid owned bay/route or compatible train, then reassign. Source-side exchange is the delivered first-cycle path before an underground bay exists. |
| Invalid turn, occupied swept space or uncovered area | Change the requested outline or clear the obstructing owned construction safely. Requote a complete plan; a 90-degree pivot on one tile cannot become a legal bend. |
| Area refused: planning stopped at a tile (**!** on the map) because of the map edge, a building, other work, or "too far" | Clear the named tile or draw a closer area. The planner adds the approach by itself, so the floor need not touch the portal. |
| **Change work area** refused | "Has not finished its first cut": wait for it, or use **Pause and extend**. "Finished another cut … since this quote": draw and review the area again. "Stands … facing another way": choose work that starts the way the machine faces, or return and park it first. |
| Retreat blocked / occupied support | Clear the real obstruction and let the train back out. Do not demolish support under any part of the consist or bypass the refusal with ordinary track in solid rock. |
| **Waiting: Train 3 is in the way**, or the track or building at a tile is in the way | Move the named train or clear the named tile; the machine carries on by itself. If the named train is the project's own hauler in the tunnel, wait: the machine never backs past it and moves once the hauler has left. |
| **Wait for the hauler to leave the tunnel first** (far exit) | Let the hauler take the spoil and leave the tunnel, then hand over again. |
| Deep locked | Wait until the calendar reaches 1980 and provide excavated upper access; changing the camera cannot unlock commands. |
| Finish/cancel pending | Let the whole cutter withdraw and the hauler deliver spoil and clear the route; a hauler still in the tunnel is named. Release is permitted only after those owners settle; completed ground and cargo receipts stay intact. |
| Unsupported Basalt ahead | Replan within supported material or use a compatible cutter. Return/parking and service remain available; the machine cannot cut backwards through unsupported rock. |

**More evidence.** Actual lost-origin recovery, partial-bend reload and cancelled final disposal/empty return are kept in [the ground persistence evidence matrix](r2mlvs15-savegame-determinism-and-lifecycle.md#ground-persistence-evidence-matrix).

**If progress still appears stuck.**

1. Save the game.
2. Record the exact status, company funds and train orders.
3. Preserve the original save when reporting the problem.

Current saves keep the project, headings, support, assignments, wagon and packet identities, and accounting.

## Questions from the 2026-10-15 demo

**Why is the Simulation rate lower than the Frame rate?** The **Frame rate** window measures two separate clocks, and they are not meant to match. Its tooltips say the same.

- **Simulation rate.** The game advances one tick every 27 ms: about **37.04 ticks per second** at normal speed. Fast-forward shortens only this interval, so the simulation rate then climbs above the frame rate.
- **Frame rate.** Drawing follows the **Refresh rate** setting, 60 frames per second by default, or the display when vsync is on. About 37 against about 60 is therefore normal.
- **A low simulation rate.** If it stays clearly below about 35 at normal speed, the computer cannot keep up. The whole game, the calendar and the machines with it, then runs slower than real time. Nothing is computed differently or lost.
  - The **Game loop** row and its sub-rows show where the time goes; Space and the per-layer tile loop are part of it.
  - The refreshed `tbm-work` demo measured 36.83 and 36.95 ticks per second, at least 99% of full speed, with a 0.1 ms game loop.

**Can F2 close Settings again, and Escape close windows?** Yes. Both are settings in **Settings -> Interface -> General**; [Keys: layers, F2 and Escape](#keys-layers-f2-and-escape) describes them.

## Current art and runtime review

The selected underground/vehicle gallery now contains **69 families and 10,557 poses**, with 65 active consumers and four retained terrain ranges. It includes terrain and cut faces, portals, cutter/support/spoil units, the conventional locomotive, receivers/service facilities, tube models and facilities, geothermal machinery, utility/riser structures and controls. Its rows identify current and retained dated observations.

**Packed art still awaiting native display.** The selected package contains 90 families, including completed Battery and Electronics sources. The 21 families outside the selected runtime review are seven mines; ordinary/sealed rail wagons and open/sealed road carriers with their purchase art; the conventional rail station and road terminal; and the metal refinery, chemical, battery and electronics works. Packing these sprites does not make them appear in the game. Their native integration and captures remain open in [Plan14 Group84](r2mlvs14-remaining-assets-and-runtime-integration.md#group-84---runtime-integration-and-review). Existing gameplay can be reviewed now; assess those new visuals after their integration.

Use the [current native gallery] (historical reference: `../../../../logs/proofs/mlm/native-art/selected-gallery-bootstrap-20261019/gallery/m1-runtime-gallery.html`) and its [whole-site contact sheet] (historical reference: `../../../../logs/proofs/mlm/native-art/selected-gallery-bootstrap-20261019/gallery/m1-runtime-contact-sheet.png`) for the review. The [publication receipt] (historical reference: `../../../../logs/graphics/selected-bootstrap/20261019-native-01/publication-verification.json`) records actual loaded captures, distributed identities and the distinction between current and retained observations. Older source sheets remain authoring evidence; the native gallery is the relevant starting point for judging the game presentation.

Personal feedback on directions, states, zoom, occlusion, picking and sound is still pending. Unrelated orbital and whole-profile graphics keep their own scope; this underground review does not complete them.

## Verification and feedback

- **Retained featured-save verification.** The [demo guide verification] (historical reference: `../../../../logs/demo-guide-verification.json`) records the earlier load/resave pass for all 28 featured saves (menu 3-30) on its recorded RelWithDebInfo build. Legacy 17 upgrades its geology and utility schemas, and its original 393 file is unchanged.
  - The selection and area-planning checks pass 19,344 assertions in 10 cases.
  - Evidence: [demo guide verification] (historical reference: `../../../../logs/demo-guide-verification.json`).
  - The `tbm-*` entries were refreshed on 2026-10-16; see the [face exchange plan](r2df2-face-exchange-and-demo-acceptance.md), Group 10.
- **Earlier native verification.** The underground tour's earlier native verification includes another successful Coal delivery, and partial collector completion without added material. Its [receipt] (historical reference: `../../../../logs/underground-demo-verification.json`) records the prepared conditions and 27 checked owner chunks.
- **Current runtime completion.** The [target-condition audit](../../../../../../../../src/tests/fixtures/checkpoints/underground/openttd3-75ea75718f/historical/bedac682c11c0c01-target-completion-verification.json) passes 116 cases and 1,220,340 assertions across both current readers, including unchanged version-370 portal endpoints loaded into version 434 and real resumed ticks. Exact normal-world, useful-route, recovery, peer and performance evidence is mapped in the [final audit](portal-tbm/r2mlvs3mp-z-b-final-closure-audit.md#current-selected-technical-reconciliation-2026-10-20). These runtime results are separate from your pending review of the featured tours and art.
- **Load checks.** Featured inputs remain unchanged. Use a personal Save game name for your experiments; current loading does not turn a prepared collector/TBM tour into a complete later-system demonstration.
- **Where files go (2026-10-09).** Under the parent [Development artifact layout](r2mlvs-multilayer-vehicles-and-structures.md#development-artifact-layout), implemented by [R2FC](r2fc-file-cleanup.md):
  - native runs use `mlm/runtime/`, generated feature reviews use `mlm/reviews/`, and logs stay under `logs/`;
  - reusable code and curated inputs keep their code and test owners;
  - repository-relative commands run from the checkout root through their context-aware wrappers;
  - historical pass records stay dated evidence, not new acceptance claims.

When reporting trouble, include:

- the **menu number or name** and your personal save;
- the selected layer and the exact control you clicked;
- the project selection and the visible wait or refusal.

For area selection, also say whether you entered through **New area**, **Extend work** or **Change work area**, selected the rock-side portal, chose **Area clearing**, and released the left drag. Those details tell a selection problem from a valid planner refusal.
