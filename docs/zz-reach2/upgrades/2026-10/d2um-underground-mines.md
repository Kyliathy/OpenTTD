# Multilayer demo - underground mines and Outer Space

Bundled player walkthrough for the [underground master](r2mlvs3mp-master-plan.md) and [asteroids/probes plan](r2mlvs10-asteroids-and-mining-probes.md). Updated **2026-10-13**. The underground baseline is tagged **`demo3`**. Recent committed Space features and the current verified construction/service work are included below. Gameplay implementation stopped after Plan11 Group3 for this playtest; both Debug and RelWithDebInfo compile.

The default demo now starts with **a working Underground Coal mine and a mineral train delivering to a Surface power station**. The same world retains the completed50-cell public tunnel and the original TBM parked at its far exit. Separate checkpoints cover excavation planning, active spoil haulage, far-exit construction and Deep collector graphics. Thirteen additional retained Space checkpoints cover paid assembly, launches, descent, survey, resupply, Core service and module work. All prepared checkpoints start paused.

## Choose your tour

Run these from `D:\Codez\Sandbox\OpenTTD`:

```powershell
.\play.ps1 9            # Underground Coal mine and freight: start here
.\play.ps1 12           # TBM planning: try selecting additional work
.\play.ps1 11           # Active excavation and spoil haulage
.\play.ps1 19           # Paid rocket launch, deployment and return
.\play.ps1 25           # Actual orbital Core service
.\play.ps1 27           # Powered module construction
```

Numbers below match the launcher's featured entries in this checkout. **1** is
new Multilayer Industry; **2** is new Classic, which does not require digging.
If a retained fixture is missing, subsequent menu numbers shift: use the named
command in the same row, or run `.\play.ps1` to inspect the current menu.
`-DryRun` prints the selected save without launching; `-Config Debug` chooses
the slower checked build. Ordinary personal saves appear after featured demos.

Start with **9 -> 12 -> 11 -> 13 -> 14 -> 15/16** for Underground, then
**19 -> 20 -> 21 -> 22 -> 23 -> 25 -> 26/27/28** for Space. These are separate
starting points. Loading another checkpoint restarts its own world; changing
layers does not create missing facilities or merge cargo. Save a personal copy
before experimenting.

## What changed since the old guide

The checked commit history supplies these milestones:

| Commit | Delivered scope |
| --- | --- |
| `0840083c40`, 2026-10-12 | Collector graphics in the game and further ground asset source work. |
| `c99b6f0d55`, 2026-10-12 (`demo3`) | Eight paused underground/mine/collector checkpoints; actual Coal delivery; chemical works source art. |
| `e4fb1d3c89`, 2026-10-12 | Moving finite asteroids, native mining probes, launch centers and linked collection stations. |
| `f09d11be8a`, 2026-10-13 | Material-paid spacecraft/pod assembly, reusable launches, cargo descent, handling, typed orders, company-private surveys and orbital resupply. |
| Current verified workspace, 2026-10-13 | Delivered first-Core construction, explicit Core activation, paid panel/service, exclusive module inputs and exact saved assembly/cancellation. Plan11 Groups1-3 are complete; later module activation/processing remain open. |

The last row is uncommitted work. A completed module **frame** currently grants
no Storage/Dock/Assembly/Power function. The already operational **Core** has its
own bounded inventory, panel and service berth.

## Start the underground tour

From PowerShell:

```powershell
cd D:\Codez\Sandbox\OpenTTD
.\play.ps1 demo
```

`underground-mines` and `mine-haulage` select the same starting point. Run `.\play.ps1` without arguments to see the permanent **Demos (Multilayer Industry)** entries above recent saves.

1. Before experimenting, **Save game** under your own name, for example `underground-mines-my-run`.
2. Choose **Underground** with the main toolbar's layer selector. The ordinary loader starts on Surface; the saved camera is aligned with the mine's Underground position. **N** also cycles layers.
3. Find the Coal mine at **Underground(8,9)** and its loading station at **(8,12)**. Click the mine to inspect its actual output and finite deposit; click the station and open its train list to find the coal shuttle.
4. Unpause. Watch coal collect, the mineral wagon fill, and the train use the connected portal at **(12,12)**. Switch to Surface to follow delivery to the station at **(15,12)**, beside the power station at **(15,14)**. Then watch the empty return.
5. Reopen your personal copy with `.\play.ps1 underground-mines-my-run`.

The default executable is the rebuilt **RelWithDebInfo** game. `-Config Debug` selects the Debug executable when reporting a problem. Both use repository-local `build/play.cfg`; your saves go into `build/save`. Featured entries prefer the retained repository snapshot, so a same-named personal/generated save cannot silently replace the reviewed starting state.

## Numbered save menu

| No. | Named command after `.\play.ps1` | Starting point and first experiment |
| --- | --- | --- |
| 3 | `space` | Three purchased mining probes approaching moving asteroids. Select Outer Space; follow a probe. |
| 4 | `space-mining` | Actual extraction underway. Watch holds fill and compatible retargeting. |
| 5 | `space-holds` | Three full holds. Inspect cargo and saved upkeep; this older world has no prepared paid return pods. |
| 6 | `space-hub` | Bare paid Surface center at(16,4), joined rail and linked train/lorry stations. Toggle collection/link controls. |
| 7 | `space-survey` | Historical ownerless survey preview approaching bodies; use21 for actual company knowledge. |
| 8 | `space-attached` | Same historical preview already attached; no cargo or company scan receipt. |
| **9** | **`demo` / `underground-mines` / `mine-haulage`** | Working Coal mine, mineral train and Surface receiver. Follow another delivery. |
| 10 | `mine-build` | Same route with paid mine machinery unfinished. Unpause before expecting Coal. |
| 11 | `tbm-work` | Accepted50-cell job at0/50 cuts with enabled hauler. Watch cutting and spoil exchange. |
| **12** | **`tbm-plan`** | Earlier44-cell job, stopped trains. Follow the selection tutorial below; append up to six straight cells. |
| 13 | `tbm-exit` | All50 cuts complete; machine stopped clear of far exit. Build B at(13,60), then park and release safely. |
| 14 | `tbm-tunnel` | Completed public A-to-B route; original TBM parked at B. Follow the ordinary train. |
| 15 | `collector-build` | Deep collector at74/296 labour ticks with construction materials already committed. Watch actual construction art. |
| 16 | `collector-ready` | Same completed Deep collector around(51,34). Inspect machinery and status glass. |
| 17 | `underground-mines-legacy` | Older bent TBM route and saved geology; compare physical advancing turns. |
| 18 | `space-workshop` | Paid spacecraft assembly interrupted. Open Surface center -> Workshop; pause/resume the working hull. |
| 19 | `space-launch` | Reusable rocket already preparing a launch. Watch deployment, return and turnaround; reload to try pre-burn cancellation. |
| 20 | `space-descent` | Mined cargo in an actual paid pod partway through descent. Follow receipt at the Surface center. |
| 21 | `space-survey-live` | Owned survey at37/148 work ticks. Finish the scan and compare unknown/known resource quantities. |
| 22 | `space-resupply` | Cargo-only rocket ascending toward the same orbital probe, carrying actual service parts/pods. |
| 23 | `space-core-build` | Steel16/Goods24 delivered; two real builders; Core at37/740 work. Complete and explicitly activate it. |
| 24 | `space-core` | Operational Core and panel, real inventory, two builders and a completed paid service receipt. Inspect exchanges and renewal state. |
| 25 | `space-core-service` | Actual Core service in progress. Try pause/resume, panel loss and cancellation. |
| 26 | `space-module-reserved` | Storage owns Steel8/Goods8 exclusively; competitor empty. Queue a builder or try the competing reservation. |
| 27 | `space-module-work` | Storage at39 whole work ticks plus saved fractional work; competing Core service paused. Resume that service to share real panel output. |
| 28 | `space-module-power` | Same partial work with panel disabled. Unpause to see it wait; toggle the Core panel to resume. |
| 29 | `space-module-frame` | Finished Storage frame: inputs consumed once; functional activation still pending. Inspect the inert result. |
| 30 | `space-module-cancelled` | Cancelled materials physically returned; replacement blueprint separately paid. Compare old and fresh job progress. |

For example, `.\play.ps1 space-module-power` is the stable named equivalent
of `.\play.ps1 28`. The eight refreshed underground snapshots retain format403
(legacy17 is393); Space
snapshots span404-418. The current build reads these older saves and writes418.
The launcher's featured saves are immutable starting points; personal saves go
into `build/save` and should use different names.

Save identities and preparation conditions are in the
[underground manifest](../../../../src/tests/fixtures/demos/underground/demo-checkpoints.json)
and [paid Space manifest](../../../../src/tests/fixtures/demos/space/paid-checkpoints.json).
The mine world has prepared workshop floor; its mine, train, wagons, stations,
portal and receiver were bought through native commands and Coal comes from
the finite natural deposit. Collector inputs/floor and acceptance-world Surface
freight/service stocks are explicitly prepared. Space work, launches, mining,
transfers and consumption use real native owners; these tours do not claim a
complete factory-to-launch logistics bootstrap.

## Completed features you can demonstrate

These are delivered scopes, rather than completion of the entire master or graphics profile.

| Completed feature | Where to see it |
| --- | --- |
| Moving finite asteroids with three resource classes, sizes, headings and reflected edges | `space`, `space-mining`; Nickel Ore, Water Ice and Rare Earth concentrate. |
| Native owned mining probes, pursuit, exclusive attachment, timed extraction and actual cargo holds | `space-mining`; select a probe/body in the inspector. |
| Synchronized assign/recall, compatible loaded retargeting, upkeep, cargo ageing and save/reload | `space`; use **Mine selected asteroid** / **Recall selected probe**. |
| Saved Sparse/Normal/Dense asteroid population setting | New Multilayer Industry worlds; Classic generates no field. |
| Waterside Surface launch-center construction, rotated footprint, native joining/removal and protected water access | `space-hub`, or Surface airport controls from1990. The pad art is temporary. |
| Explicit train/lorry hub links, whole-footprint20-tile range, collection overlay and timed cargo handling | `space-hub` for layout; `space-workshop`/`space-descent` for actual inventory and received cargo. |
| Layer switching and separate Surface/Underground/Deep world state | Every checkpoint. Deep access/operation is gated until1 January1980; use the selector to choose the intended layer. |
| Surveyed material deposits, saved regional hardness and finite reserves | Mine tour and Underground Construction -> Survey. Soft/Mixed/Hard/Basalt affect excavation; surveying creates no cargo. |
| Paid TBM commissioning, actual machinery/support wagons and a saved excavation project | `tbm-work`, Projects and its portal controls. |
| Straight/area excavation preview and extending the same machine's accepted work | `tbm-plan`; numbered cutter positions, extra working floor, spoil, time and cost appear before confirmation. |
| Paid cuts, protected temporary support, actual Earth/Rock cargo and the repeated Surface disposal/empty-return loop | Unpause `tbm-work` and follow both trains. |
| Pause/resume, return/park, selected hangar, safe release and paid permanent-track conversion | Projects/portal controls; `tbm-exit` provides the far-exit handover. |
| Connected cross-layer public rail and ordinary freight after safe TBM withdrawal | `tbm-tunnel` and the new coal shuttle in `demo`. |
| Ten resource-specific native mine definitions, matching-seam funding and finite extraction | The prepared Coal mine; try another properly excavated site using the instructions below. Seven selected mine artwork families have completed source art, with final packing still pending. |
| Conventional locomotive, mineral/sealed wagons and open/sealed road freight capability with cargo/date/layer checks | The coal shuttle demonstrates locomotive/mineral-wagon movement, loading and unloading. Other carriers can be purchased/refitted in compatible owned depots. |
| Native Surface metal refinery, chemical, battery and electronics recipes with real input/output stock | Fund compatible Surface sites and deliver their required cargo. The default Coal route uses the original power-station coal receiver, rather than one of these processing chains. |
| Actual paid/material-built Deep collector, native construction stages and completed game graphics | `collector-build` and `collector-ready`; construction advances through its actual labour owner. |
| Current-format save/reload for the demo's natural resources, jobs, vehicles, cargo, industry and utility owners | Save your run mid-action and reopen it; the prepared checkpoints also passed ordinary load/resave checks. |
| Steel/Goods-paid hull and pod assembly, freight and service-part compartments |18, Workshop; material and capital requirements are visible. |
| Reusable paid launch, deployment, recovery and explicit resupply receiver |19 and22; the same rocket and receiving probe survive the mission. |
| Paid cargo pods, timed descent and Surface receipt |20; a pod carries actual mined cargo and is expended. |
| Saved typed queues and company-private survey knowledge |21; inspect Survey/Visit/Mine actions and their actual progress. |
| Delivered first-Core construction and explicit activation |23; completing a frame alone does not activate it. |
| Actual Core inventory, paid30kW panel and148-tick RFMT/ELEC service |24/25; power loss and manual pause retain the parts. |
| Exclusive module recipe packets, partial-power assembly, cancellation and save receipts |26-30; later module functions are still unfinished. |

Further completed foundations include paid external electrical service for prototype tubes, bounded native Heat routing/allocation, paid portal-bound electrical riser construction/links and shared energy lineage. Their focused native tests establish those boundaries. Normal tube/geothermal construction tools, the complete tube fleet/terminals and an operating transported-input geothermal powerhouse still need their later master integrations; this tour does not provide those unfinished player flows.

## Tour Space: moving asteroids and mining probes

```powershell
.\play.ps1 space-mining
```

1. The mining saves open **paused on Outer Space in1990**. Unpause and use
   fast-forward. If the view is lost, select Outer Space and run `space_view`
   in the game console to recenter the field and open its inspector.
2. Click an asteroid to select its class and position. Resource quantities remain unknown to your company until a real survey completes; use21 for that tour.
   Click a probe or its inspector row to select the actual company vehicle.
3. Watch pursuit, attachment and extraction. Each probe starts on a different
   class. **Mine selected asteroid** assigns the selected body to the selected
   owned probe; assigning two probes to one body demonstrates exclusive mining.
4. Try **Recall selected probe** during partial mining. It detaches and returns
   with cargo intact. A partially loaded probe can retarget only to compatible
   cargo; a full hold refuses further extraction.
5. Switch to another layer while running, then return. Motion, work, costs and
   cargo ageing continue while hidden; the global pause freezes simulation.
6. Save a personal copy mid-approach/mining/return and reload it. Use
   `space-holds` to inspect full holds immediately.

Each extraction credits **2 tonnes per74 working ticks**, up to a **20-tonne**
hold. Mining costs500 internal pounds/day; travel/waiting/holding costs100/day,
with saved fractional accrual. These older demo saves predeploy their purchased probes and preserve their
original paid receipts. Use19 for an actual Earth launch and20/22 for return
cargo and repeat resupply.
Mining and return to staging pay no delivery income.

The sprites are temporary: [asteroid/probe sheet](../../../../media/baseset/openttd/space_preview.png).
The older `space-survey` / `space-attached` saves show repeated surveying motions
without extraction, ownership or completed company scans.

## Tour the new Surface launch hub

```powershell
.\play.ps1 space-hub
```

The checkpoint opens paused on **Outer Space** because it retains the older
survey preview. Select **Surface** in the layer selector and find the real paid
launch center at **Surface(16,4)**, with freight links already prepared. You can
also locate it through your company's station list using the airport filter.
Open the station window of a separate rail or lorry station nearby. Its
**Launch hub** dropdown identifies the selected center;
**Collection area** shows its circular20-tile Surface service area and linked
station tiles. Select **No launch hub** to unlink, then select the center again.

The center's joined rail uses the same StationID and native station cargo storage.
Separate members keep their own IDs. Eligibility requires the same company,
Surface rail/lorry facilities, and **every station footprint tile** inside the
circle. Extending a member outside retires its link; station-name position and
ordinary catchment do not determine eligibility. Up to32 separate stations can
link to one center. Joining does not move its anchor or enlarge orbital range.

To try construction in your own copy, open **Surface airport construction ->
Space Launch Center**. Use **Rotate**, choose flat clear land beside an orthogonal
edge of flat navigable sea/river/canal, and build. The footprint is4x3 or3x4;
availability starts in1990 for Multilayer Industry. Construction charges500,000
plus clearing; removal costs10,000. Removing the last qualifying water edge is
refused. Demolishing the center retains joined rail, and rebuilding creates a
new reference. Ordinary aircraft do not use this facility.

**Prepared conditions:** this checkpoint uses explicitly prepared terrain/water
and native paid facility/rail/lorry commands. It has no free orbital dock, Core,
rocket or cargo. Its Space field contains the older survey-preview controllers;
load `space-mining` separately for native mining. The
[hub checkpoint manifest](../../../../src/tests/fixtures/demos/space/hub-checkpoint.json)
records the source and exact saved identity.

## Tour paid Space launches, returns and survey

1. **18: workshop.** On Surface, locate the launch center through the company
   station list (airport filter), open its station window, then **Workshop**.
   Select the working hull row. Read Steel/Goods/capital and remaining work;
   try **Pause / resume**, unpause globally, then compare the completed hull.
   New hulls require actual stock; repeated button clicks do not create free
   craft. **Build pod** also consumes its declared recipe.
2. **19: reusable launch.** Open the same Workshop and select the rocket.
   Unpause and watch preparing -> ascending -> deploying -> returning ->
   turnaround -> stored. The paid probe remains in orbit. To test **Cancel /
   return** before fuel burn, reload this starting checkpoint first. Selecting
   a different phase later changes what a cancellation can do.
3. **20: mined return cargo.** Unpause the interrupted descent and inspect the
   center's actual cargo and nearby collection stations. Cargo must reach the
   Surface receiver; an orbital hold or an unrelated station visit is not a
   final delivery. A dropped pod is spent. For another drop, select an eligible
   loaded probe and paid pod in Workshop, then **Release cargo pod**; it needs
   remaining return allowance and an owned center inside the20-tile zone.
4. **21: real survey.** Choose Outer Space and click the owned survey probe, or
   run `space_view` in the console to open/recenter the inspector. Unpause to
   finish its37/148 work. Compare unknown quantities with the completed company
   scan. Clear/edit its queue to try cancelling a partial scan; incomplete work
   grants no knowledge. Use the real probe, not the older ownerless preview in7/8.
5. **22: resupply.** Follow the already ascending cargo-only rocket through
   delivery and return. The receiving orbital probe keeps its identity. For a
   repeat flight, select that probe in Workshop, **Set resupply receiver**,
   select the stored rocket, load actual parts/pods/compatible freight and
   **Load 15 Propellant**, then **Launch resupply**. An occupied or full receiver
   holds the payload; it does not partially unload or create a replacement hull.
6. In the probe inspector, select a body and use **Mine**, **Survey** or **Visit**
   for compatible queue actions. Inspect the current action/block reason; clear
   or reorder entries before retrying a different job. Mining probes, survey
   probes and builders keep separate roles. Onboard **Service / pause** requires
   real RFMT/ELEC parts and an idle, eligible probe; Core service is a separate
   physical berth in the next tour.

## Tour orbital Core construction, service and module work

Choose **Outer Space**, then the ordinary **Fund new industry** action to open
**Orbital construction**. Select a site row and use **Go to site**. Site numbers
shown in this window are one-based; the Core is at(4,4), its precise approach
at(4,7), and the Storage blueprint is at(7,4). **Next builder** and **Next service
probe** select different roles; read the selected vehicle before issuing work.

1. **23: first Core.** Unpause from37/740 work ticks. Both delivered materials
   remain reserved until final completion; two builders do not double the job's
   speed. Select the finished Core and press **Activate Core**. Unpause through
   its74-tick activation before expecting inventory/service. It bootstraps
   without an earlier station or utility source.
2. **24: inventory.** Select the operational Core and a builder at its precise
   approach. Try **Load stored Goods** and **Unload to Core** with actual stock;
   loading Steel is disabled if its stock is empty. **Visit** can route a builder
   to a selected site. Clear older builder orders if they prevent the visit.
   Loading a material does not transform the builder's embodied hull into cargo.
3. **25: service.** Select the Core. Try **Pause service** / **Resume service**,
   then **Toggle Core panel**. With the panel disabled, global unpause must not
   advance service. Toggle it back to restore supply. Completion consumes one
   RFMT and one ELEC, records148 work ticks/111,000J, and replaces return
   allowance with2. Reload and try **Cancel service**: only the reserved unused
   parts return; spent work/energy is not refunded. Starting again needs a probe
   at the Core approach, an available berth and both actual parts.
4. **26: exclusive reservation.** Select Storage at(7,4), inspect delivered8/8
   and consumed0/0, select a builder and **Queue construction**. Clear earlier
   orders if necessary. The competing Storage at(2,4) cannot count this same
   stock. **Reserve Core materials** is disabled for the already full job and
   when the Core lacks either missing input. A reservation alone never builds
   an active module.
5. **27: partial power.** Storage starts at39 ticks plus preserved fractional
   work; its whole-tick display may initially hide that fraction. Select the
   Core and **Resume service**, then inspect Storage while unpaused. The panel
   provides810J per native tick: service may use750J and construction receives
   the remaining60J. Pausing the service frees capacity; another builder cannot
   multiply the Core's single construction slot.
6. **28: power loss.** Unpause before changing anything: partial work and
   Steel/Goods remain held because the panel is off. Select the Core, **Toggle
   Core panel**, then watch construction resume. Save/reload while held to
   compare progress and material ownership.
7. **29: honest completion boundary.** Storage has finished296 work ticks and
   consumed Steel8/Goods8 once. It remains an inert frame: there is currently no
   Storage activation command/function. Dock, Assembly, processing and Power
   module activation/manufacture remain later tasks.
8. **30: cancellation result.** Inspect the old cancelled site and separately
   paid replacement at the same location. The Core now holds Steel10/Goods8;
   old partial work remains recorded, while the replacement starts at zero.
   To perform the cancellation yourself, reload27, cancel the paused Core
   service, select the Storage job and **Cancel blueprint**, then **Clear builder
   orders** so the cancelled Construct action cannot block later visits. At its approach,
   use **Collect Steel**, **Visit** the Core, **Unload to Core**, return to the
   cancelled site and collect Goods. A builder carries one freight label at a
   time, so do not try to mix Steel and Goods in the same hold.

**Still unfinished:** direct rocket-to-module delivery, activation/removal of
later module functions, locally manufactured probes/heavy robots/haulers,
orbital processors and the complete factory-to-Earth chain. Existing rocket
resupply delivers to an actual probe; construction probes then move real
materials. Core service is available, while a completed Storage frame grants
no free station function. Most Space artwork is temporary. The earlier
station/probe-name compatibility regression is fixed in the current build.

## Tour the mine and conventional freight

The `demo` checkpoint starts immediately after a successful delivered load, so the train may initially be on Surface. Use the **loading station's train list** or the company's train list to open its vehicle window. Its centre-view control follows the actual train across layers; its orders identify the mine loading stop and Surface unloading stop.

Compare the mine's output, waiting station cargo, wagon load and remaining reserve over several journeys. Cargo should move through those owners; reloading should preserve it. The Surface station must continue accepting Coal for delivery income. Keep the powered locomotive and compatible mineral wagon together, retain both station orders and leave the portal connected.

Use `mine-build` to see the beginning instead. The route and train are enabled already; release the global pause and allow actual machinery construction to finish before expecting ore. Mine and freight presentation currently includes existing substitute graphics. The recently authored selected mine/wagon/station models still await their packing/runtime milestone.

The completed old A-to-B tunnel also remains in this mine world: A is at(13,10), B at(13,60), and its original TBM is stored at B. The dedicated Coal portal at(12,12) is separate from that route. Nothing in the Coal tour requires selling or replacing the retained TBM.

## Tour excavation and the spoil loop

Open `.\play.ps1 tbm-work`, select Underground, then:

1. Open **Underground Construction -> Projects** and select the project for **Train#2**. **Open machine** opens the actual cutter; its centre-view control follows it even while it is in the Surface magazine. **Train#1** is the spoil hauler.
2. Choose **Portal controls**. Read **Next:** and the current wait reason before changing the layout.
3. Unpause and watch cutting, retreat, wagon exchange, Surface disposal and empty return. The accepted project and both trains are already enabled. Fast-forward shortens the wait.
4. Pause the project independently of the whole game, resume it, then save/reload your own copy during a wait. Compare the physical wagons, floor and project progress.

On Underground, the main toolbar's airport-construction position becomes the **TBM icon** for **Underground Construction**. Its palette contains **Survey**, **Projects**, **New area** and **Fund a mine**. **Requirements and cutter dates** expands optional guidance; Escape clears the active draft/footprint.

For `tbm-plan`, choose **Extend work** or **Pause and extend**, then **Straight tunnel** and an endpoint directly ahead. Six more cells reach Underground(13,60). Review the numbered route, charged working space, cost, cutting time and spoil before **Add work to this machine**. Start both trains if stopped, resume the project and unpause. Cancelling the draft leaves earlier accepted work intact and buys no second TBM.

### Select work successfully: start with save12

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

For an existing project, start from **Projects -> Pause and extend** as above.
The portal is already selected. Hold the **left mouse button** on one floor
corner, move to the opposite corner while holding it, and **release**. The quote
is created on mouse release. For a first small experiment, request a narrow
three-cell strip immediately beyond the last accepted cutter position, in the
same forward heading. The planner adds the cutter's required width as working
space. Reload12 before switching between alternative experiments.

For a **new machine at an unused portal**:

1. On Surface, open the **rail toolbar -> layer portal** tool (**P**). Choose
   the mouth direction, build on clear ground and leave the portal **Sealed**.
   Its other end is in Underground. Leave room around the Surface housing for
   the machine and exchange approach; direct Surface-to-Deep rail is unavailable.
2. Lay a connected Surface line to an owned rail station and depot. Fund an
   **Earth Reclamation Site** and **Rock Crusher** in that station's catchment,
   so it accepts both spoil labels. The current controls automatically choose
   a compatible cutter and the nearest eligible owned Surface station; there
   is no manual origin picker in this area tool. Buy a compatible spoil hauler
   with matching empty capacity: the default Standard set has two Earth and
   two Rock wagons; Deep has two Earth and four Rock. Give it a visit order to
   the Surface origin station. Deep work requires1980 or later.
3. Switch to the destination rock layer. Open **Underground Construction** and
   press **New area**. **First click the actual rock-side portal housing.** A
   click on an ordinary rock tile at this stage only repeats the portal prompt.
4. Wait for **Drag the intended final floor on this layer**. Keep **Area
   clearing** selected. Left-drag the intended rectangle and release. Start
   with the three-cell strip directly forward of the portal, leaving room on
   both sides for the cutter; large rooms require legal approaches and turns.
5. Read the quote and overlay: **white = requested floor**, **blue = additional
   charged working space**, **red = uncovered/refused floor**. Numbers are the
   cutter's successive positions. The quote includes Earth/Rock, work and cost.
6. Only a complete valid plan enables **Commission the complete area plan**.
   Confirm, open the portal controls and **Start** the project. Press **Assign
   hauler**, then click the actual hauler in the map, depot or train list.
   Start that train and unpause globally. Previewing and surveying never
   excavate the floor. Read **Next:** whenever the machine waits.

For a simple new straight corridor, the portal window also offers a length
of2-60 cells and **Commission TBM**; it uses the same physical logistics. For
the graphical endpoint tool, select **Straight tunnel** after selecting the
portal, then click a forward endpoint. That mode does not accept a floor drag.

Do not use **New area** on save12's existing portal: it already owns a project
and deliberately directs you back to **Projects -> Extend work**. Switching
layers, selecting another tool, closing the palette or pressing Escape can
cancel the placement tool; re-arm it before drawing again.

| What you see | What to try |
| --- | --- |
| Dragging produces no quote | Check the correct layer, armed Area clearing tool and selected portal. Hold left-drag and release in the map, outside the palette. |
| "Click an owned rail portal..." | Click the rock-side housing first. For an existing project, enter through Projects -> Extend work. |
| "This portal already has a TBM project" | Extend that project; New area commissions a separate machine at an unused eligible portal. |
| "No Surface origin" / no available cutter | Provide an owned Surface rail station accepting Earth and Rock, a valid sealed portal, and an available cutter for this layer/date. Defaults are selected automatically. |
| Red cells or disabled confirmation | Move/shrink the rectangle into a physically reachable forward area; allow the full cutter/consist width and advancing45-degree turns. An arbitrary remote room cannot be dug through intervening solid rock. |
| Selected floor already covered | Request new floor beyond the accepted route; selecting existing planned floor adds no work. |
| Quote becomes stale | Redraw after project, terrain, owner or money changes; old quotes cannot authorize new work. |
| Accepted but nothing moves | Resume the project, start both trains, unpause globally, then read **Next:** and the exact wait reason. Check matching empty Earth/Rock capacity and Surface disposal orders. |
| Ordinary rail refuses solid rock | Use the TBM to earn floor first, withdraw safely, then release/convert support before ordinary construction. |

Look for these details:

- Newly open floor must be earned before ordinary rail can use solid rock. Occupied temporary support remains protected.
- The cutter advances/backtracks with its whole consist; it cannot spin on one tile. Use the legacy bent route to inspect legal advancing bends.
- Soft/Mixed/Hard/Basalt cut yields are respectively Earth/Rock3/6,2/12,1/20 and1/24 tonnes. Cutting takes twice the former active time; travel and haulage waits remain separate.
- Surface disposal charges money. Pausing or reloading should not remove loaded spoil or buy free cuts.

## Build the far exit and retain the TBM

`tbm-exit` skips the cutting wait. All50 cuts are complete and the whole machine is stopped clear of **Underground(13,60)**. A remains at(13,10).

1. Leave the cutter stopped. From Underground rail portal construction, choose Surface and preview B at(13,60). Select the mouth orientation that joins the retained approach; inspect both endpoint footprints and the price.
2. Build B and **Connect** it when its actual approaches permit.
3. Choose B as the machine's hangar, then **Return and park**. Let the original hauler settle remaining spoil through its assigned origin.
4. Once the whole machine is stored and release guidance allows it, **Finish/cancel and release** the project.
5. Pay to **Make track permanent** where needed, then **Connect** A. The stored TBM remains available for later use.

`.\play.ps1 tbm-tunnel` starts after this sequence, with an ordinary train already running between both ends. The released project is no longer in Projects; find the parked machine through the train list. **Pause** holds current work, **Return and park** preserves it for later, and **Finish/cancel and release** ends it safely. The [full how-to](r2mlvs-how-to.md#if-something-goes-wrong) explains recovery reasons.

## Tour the newly delivered Deep collector

```powershell
.\play.ps1 collector-build
# Or skip to the completed machinery:
.\play.ps1 collector-ready
```

Choose **Deep Underground** in the layer selector. The collector occupies the area around **(51,34)**; its machinery anchor is(50,33). If necessary, open the industry list and centre on the collector.

In `collector-build`, inspect the incomplete hardware before unpausing. All actual recipe inputs have already been received and committed, and74 of296 native labour ticks are complete. Unpause to watch the remaining paid construction stages and completed apparatus. Pause again to compare the construction silhouette and status glass. `collector-ready` provides the complete state immediately.

This is the newly packed collector artwork in the real game, rather than a source-art contact sheet. Its natural vent/claim and bounded Heat accounting are implemented. An operating powerhouse, useful geothermal electrical export, normal geothermal toolbar and transported first-plant supply tour are still pending. Do not expect this collector-only world to power a finished factory/tube system.

## Try another mine or material chain

Underground resources are **Coal, Iron Ore, Copper Ore, Bauxite and Salt**. Deep resources are **Lithium Ore, Nickel Ore, Cobalt Ore, Rare Earth concentrate and Uranium Ore**, after the1980 gate. A geothermal vent is a separate renewable heat source.

1. **Survey a matching seam** on the intended layer.
2. **Excavate a real site** with the TBM: a mine needs3x3 machinery cells and an adjoining2x2 loading approach. The narrow50-cell corridor alone is insufficient. Withdraw safely and release/remove temporary support from the machinery footprint.
3. **Fund the matching mine** from Underground Construction -> Fund a mine. Funding rechecks floor, seam, exclusive claim, date, owner and money; it cannot excavate its own foundation.
4. **Build compatible freight infrastructure** on open floor. Connect an ordinary station/depot/route to a genuine receiver, preserving portal approaches and machinery loading access.
5. **Buy/refit compatible vehicles** and assign collection/unloading orders. Ordinary mineral wagons cover compatible ores; Uranium requires a sealed carrier. Follow the date/layer/cargo refusals rather than changing a vehicle's meaning through appearance.
6. **Watch reserve, stock and cargo**, then save/reload during loading or travel. On Surface, try supplying the implemented refinery/chemical/battery/electronics recipes with actual transported inputs.

## Latest work that is complete as source art

The interrupted **chemical works40C** task is now complete: genuine unfinished/finishing apparatus plus original status components supply all512 required parts; the completed model stays unchanged. The [source review](../../../../logs/proofs/mlm/native-art/ground/chemical-works-source-review.png) and [construction comparison](../../../../logs/proofs/mlm/native-art/ground/chemical-works-source-construction-cells.png) show that finished source work.

Other completed selected source families include the ordinary/sealed mineral wagons, open/sealed mine trucks, conventional station/terminal, seven selected mines, metal refinery, Vent-Tap/Magma-Loop generators, utility conduits/riser and tube/geothermal control icons. **Those sources still need the later packing/native-display checkpoint.** Battery and electronics source families remain next in the selected queue. No completion box was added for unfinished game integration.

## Verification and feedback

The current Debug and RelWithDebInfo game/test targets compile. Plan11 Group3
passes3,173 assertions in10 focused cases per configuration, actual interrupted
assembly/cancellation, ten native/preview corrupt-save refusals, and legacy
first-Core continuation. All26 tracked Space owner families match across
configurations. Slow Debug wrappers were split into companion files; the final
focused bursts finish within two minutes. Evidence:
[orbital assembly verification](../../../../logs/orbital-assembly-verification.json).

All28 featured saves (menu3-30) were loaded and resaved using the current
RelWithDebInfo game, in58.7/29.8/28.4-second bursts.
All13 new aliases and their numeric choices resolve correctly. The version418
module saves preserve all26 checked owner families. Legacy17 upgrades its
geology/utility schemas; its original393 file remains unchanged. The selection
and area-planning checks pass19,344 assertions in10 cases. Current demo evidence:
[demo guide verification](../../../../logs/demo-guide-verification.json).

The underground tour's earlier native verification includes another successful
Coal delivery and partial collector completion without added material; its
[receipt](../../../../logs/underground-demo-verification.json) records prepared
conditions and27 checked owner chunks. The paid Space manifest records each
retained source/hash. Demo load checks use the ordinary current game and never
overwrite those starting saves. Source-art work still awaiting packing is
identified above; it is not presented as finished in-game content.

When reporting trouble, include the **menu number/name**, your personal save,
selected layer, exact clicked control, project/probe selection and visible
wait/refusal. For area selection, include whether you entered through **New
area** or **Extend work**, selected the rock-side portal, chose **Area clearing**,
and released the left drag. Those details distinguish a selection problem from
a valid planner refusal.
