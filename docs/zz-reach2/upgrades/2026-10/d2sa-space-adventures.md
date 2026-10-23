# Multilayer demo - Space adventures

Player walkthrough for Outer Space and the Surface launch hub. It is owned by the [Space master](r2mlsp-space-master-plan.md) and its plans: [space world and construction](r2mlvs8-space-world-and-construction.md), [launch centers and rockets](r2mlvs9-launch-centers-and-rockets.md), [asteroids and mining probes](r2mlvs10-asteroids-and-mining-probes.md) and [orbital stations and manufacturing](r2mlvs11-orbital-stations-and-manufacturing.md).

- **History.** This guide was split on **2026-10-16** from the combined demo guide, whose Space tours date from **2026-10-13**. It also took in the earlier asteroid and probe guide (`d3sp-space-survey.md`, removed the same day). That guide's "return-path boundary" (full holds stuck in orbit) is superseded by the launch, descent and handling tours below.
- **Where the Underground is explained.** Mines, the boring machine, excavation, the far exit and the Deep collector are covered in [Multilayer demo - underground mines](d2um-underground-mines.md). Both guides list every launcher command, but each explains only its own part of the world.
- **Updated 2026-10-18, through `5d250ceaca`.** The previous text covered `9a47768ae3` (15 October). The file itself was untracked when this update began, with a local modification time of 18 October 10:30; it has no committed file history. The feature baseline in its text, followed by the commits below, determines this update's scope.
- **Coverage.** Orbital manufacture, paid purchases/clones/replacements, heavy mining and service, hauler freight, orbital processing, manufactured products returned to Earth, completed probe missions and final Space art now join the earlier launch/module tours. New paused starting points **31-43** expose these features without rebuilding their prerequisites by hand.

The active plan is [the Space master](r2mlsp-space-master-plan.md). Its [current handover](r2mlsp-z-z-z-z-z-q-launch-center-source.md#current-boundary) selects the sealed GZVEEH world and unfinished **Plan11GZVEEI**; the older Plan6 restart is historical. [Recovery and artifact storage](../../architecture/archi-historical-debug.md) explains rebuilding an absent application and preserving saved games before cleanup. A successful static recovery does not complete the next Goods proposal or authorize gameplay. This demo update is a playable checkpoint of the delivered features; final unassisted Surface factory-to-orbit acceptance remains open. All featured starting saves are paused.

## Choose your tour

Run these from `D:\Codez\Sandbox\OpenTTD`:

```powershell
.\play.ps1 space-factory    # 31: finish a paid orbital construction probe: start here
.\play.ps1 space-heavy      # 32: larger mining robot, final directional/work art
.\play.ps1 space-hauler     # 33: real orbital freight in transit
.\play.ps1 space-steel      # 34: powered Steelworks processing mined ore
.\play.ps1 space-products   # 42: manufactured Steel/Goods returning to Earth
.\play.ps1 space-mining     # 4: the original small-probe tour
.\play.ps1 space-hub        # 6: the Surface launch center and its freight links
.\play.ps1 19               # Paid rocket launch, deployment and return
.\play.ps1 25               # Actual orbital Core service
.\play.ps1 27               # Powered module construction
.\play.ps1 29               # Activate a module, launch to it, build a Dock
```

For a short current demo, use **31 -> 32 -> 33 -> 34 -> 35 -> 42**, then **38 -> 40** for construction and service. The original longer tour is **4 -> 6 -> 19 -> 20 -> 21 -> 22 -> 23 -> 25 -> 26/27/28 -> 29**. These are separate starting points:

- Loading another checkpoint restarts its own world.
- Changing layers does not create missing facilities or merge cargo.
- Save a personal copy before experimenting.

## Start the game

The launcher [play.ps1](../../../../play.ps1) runs the game in the `build` folder with the config `build/play.cfg`. Your saves go to `build/save`.

- **Menu.** `.\play.ps1` shows a numbered menu; type a number and press Enter. `.\play.ps1 21` or `.\play.ps1 space-survey-live` starts an entry directly.
- **Options.**
  - `-DryRun` prints the selected save without launching.
  - `-Config Debug` runs the slower checked build, which stops at any failed internal check. That helps when you report a bug.
  - `-All` also lists the saves the tests generated.
- **Saves.** Featured entries prefer the retained repository snapshot, so a personal or generated save of the same name cannot replace the reviewed starting state.
- **More.** The [underground guide](d2um-underground-mines.md#start-the-game) describes new games, the config and the settings the launcher makes.

**Keys.**

- **Layers.** **Alt+4** shows Outer Space and **Alt+1** the Surface; **N** cycles the layers, and the small map follows every change.
- **Windows.** Pressing a window's hotkey again closes it (F2 twice closes Game Options), and Escape closes the front-most window. Both are settings in **Settings -> Interface -> General**; see [Keys: layers, F2 and Escape](d2um-underground-mines.md#keys-layers-f2-and-escape).
- **Speed.** Why the Frame rate window shows a lower simulation rate than frame rate is answered in [the underground guide](d2um-underground-mines.md#questions-from-the-2026-10-15-demo).

## All launcher commands

Underground entries 9-17 are explained in the [underground guide](d2um-underground-mines.md); Space entries 3-8 and 18-43 are explained here. Entries 31-43 were appended without renumbering earlier demos. **1** is a new Multilayer Industry game and **2** a new Classic game.

If a retained fixture is missing, later menu numbers shift: use the named command in the same row, or run `.\play.ps1` to see the current menu. Ordinary personal saves appear after the featured demos.

| No. | Named command after `.\play.ps1` | Starting point and first experiment | Guide |
| --- | --- | --- | --- |
| 1 | (menu only) | New Multilayer Industry game, 256 x 256 temperate, maximum loan 20,000,000. | [Underground](d2um-underground-mines.md#build-it-yourself-from-a-new-game) |
| 2 | (menu only) | New Classic game: layers, portals and elevators, but no digging. | [Underground](d2um-underground-mines.md#start-the-game) |
| 3 | `space` | Three purchased mining probes approaching moving asteroids. Select Outer Space; follow a probe. | Space |
| 4 | `space-mining` | Actual extraction underway. Watch holds fill and compatible retargeting. | Space |
| 5 | `space-holds` | Three full holds. Inspect cargo and saved upkeep; this older world has no prepared paid return pods. | Space |
| 6 | `space-hub` | Bare paid Surface center at (16,4), joined rail and linked train/lorry stations. Toggle collection/link controls. | Space |
| 7 | `space-survey` | Historical ownerless survey preview approaching bodies; use 21 for actual company knowledge. | Space |
| 8 | `space-attached` | Same historical preview already attached; no cargo or company scan receipt. | Space |
| **9** | **`demo` / `underground-mines` / `mine-haulage`** | Working Coal mine, mineral train and Surface receiver. Follow another delivery. | [Underground](d2um-underground-mines.md) |
| 10 | `mine-build` | Same route with paid mine machinery unfinished. Unpause before expecting Coal. | [Underground](d2um-underground-mines.md) |
| 11 | `tbm-work` | Accepted 50-cell job at 0/50 cuts with enabled hauler. Watch cutting, and the hauler taking the spoil at the face. | [Underground](d2um-underground-mines.md) |
| **12** | **`tbm-plan`** | Earlier 44-cell job, stopped trains. Append up to six straight cells. | [Underground](d2um-underground-mines.md) |
| 13 | `tbm-exit` | All 50 cuts complete; machine stopped clear of the far exit. Build B at (13,60), then park and release safely. | [Underground](d2um-underground-mines.md) |
| 14 | `tbm-tunnel` | Completed public A-to-B route; original TBM parked at B. Follow the ordinary train. | [Underground](d2um-underground-mines.md) |
| 15 | `collector-build` | Deep collector at 74/296 labour ticks with construction materials already committed. Watch actual construction art. | [Underground](d2um-underground-mines.md) |
| 16 | `collector-ready` | Same completed Deep collector around (51,34). Inspect machinery and status glass. | [Underground](d2um-underground-mines.md) |
| 17 | `underground-mines-legacy` | Older bent TBM route and saved geology; compare physical advancing turns. | [Underground](d2um-underground-mines.md) |
| 18 | `space-workshop` | Paid spacecraft assembly interrupted. Open Surface center -> Workshop; pause/resume the working hull. | Space |
| 19 | `space-launch` | Reusable rocket already preparing a launch. Watch deployment, return and turnaround; reload to try pre-burn cancellation. | Space |
| 20 | `space-descent` | Mined cargo in an actual paid pod partway through descent. Follow receipt at the Surface center. | Space |
| 21 | `space-survey-live` | Owned survey at 37/148 work ticks. Finish the scan and compare unknown/known resource quantities. | Space |
| 22 | `space-resupply` | Cargo-only rocket ascending toward the same orbital probe, carrying actual service parts/pods. | Space |
| 23 | `space-core-build` | Steel 16/Goods 24 delivered; two real builders; Core at 37/740 work. Complete and explicitly activate it. | Space |
| 24 | `space-core` | Operational Core and panel, real inventory, two builders and a completed paid service receipt. Inspect exchanges and renewal state. | Space |
| 25 | `space-core-service` | Actual Core service in progress. Try pause/resume, panel loss and cancellation. | Space |
| 26 | `space-module-reserved` | Storage owns Steel 8/Goods 8 exclusively; competitor empty. Queue a builder or try the competing reservation. | Space |
| 27 | `space-module-work` | Storage at 39 whole work ticks plus saved fractional work; competing Core service paused. Resume that service to share real panel output. | Space |
| 28 | `space-module-power` | Same partial work with panel disabled. Unpause to see it wait; toggle the Core panel to resume. | Space |
| 29 | `space-module-frame` | Finished Storage frame: inputs consumed once. Activate it, launch freight straight to a module, build a Dock, or remove the empty frame. | Space |
| 30 | `space-module-cancelled` | Cancelled materials physically returned; replacement blueprint separately paid. Compare old and fresh job progress. | Space |
| **31** | **`space-factory`** | Operational paid Assembly, actual construction probe at 74/148 work ticks. Unpause to finish the same new hull. | Space |
| 32 | `space-heavy` | Actual paid heavy mining robot approaching finite ore. Follow its work, cargo and return. | Space |
| 33 | `space-hauler` | Paid orbital hauler already travelling with 2 Steel units. Watch transfer to its destination. | Space |
| 34 | `space-steel` | Steelworks midway through a powered Nickel Ore batch. Inspect input, output and electricity totals. | Space |
| 35 | `space-goods` | Goodsworks midway through a batch from Steel and Rare Earth. | Space |
| 36 | `space-refinery` | Refinery working on Nickel Ore -> Refined Metal. | Space |
| 37 | `space-ice` | Refinery using its alternative Water Ice recipe. Compare the selected recipe and output history. | Space |
| 38 | `space-power` | Power module at 222/444 construction ticks. Finish its frame, then explicitly activate it. | Space |
| 39 | `space-fuel-store` | Completed Fuel store frame. Inspect final artwork and the explicit unavailable activation. | Space |
| 40 | `space-heavy-service` | Heavy robot service underway at a two-berth powered Dock. | Space |
| 41 | `space-missions` | Saved Visit/Mine queue in progress, using actual company probes. | Space |
| 42 | `space-products` | Manufactured Steel/Goods already released toward the Surface. Follow descent and inspect 2 Steel/4 Goods received. | Space |
| 43 | `space-steel-outage` | Partially paid Steelworks batch held by a disabled panel. Restore its actual source to continue. | Space |

For example, `.\play.ps1 space-module-power` is the stable named equivalent of `.\play.ps1 28`.

- **Versions.** Original Space snapshots span versions 404-418. New entries 31-43 are ordinary-game version **440** resaves of retained paid 430/437/438 producers. Current saves write 440. The underground guide lists its older snapshots.
- **Immutable starting points.** The launcher's featured saves are never overwritten. Personal saves go into `build/save` and should use different names.
- **Manifests.** New snapshot hashes, original producer identities and preparation conditions are in [current checkpoints](../../../../src/tests/fixtures/demos/space/current-checkpoints.json). Earlier identities remain in the [paid Space manifest](../../../../src/tests/fixtures/demos/space/paid-checkpoints.json), [Space demo manifest](../../../../src/tests/fixtures/demos/space/demo-checkpoints.json) and [hub manifest](../../../../src/tests/fixtures/demos/space/hub-checkpoint.json).
- **Prepared conditions.** Acceptance-world Surface freight and service stocks are explicitly prepared. Space work, launches, mining, transfers and consumption use real native owners. These tours do not claim a complete factory-to-launch logistics bootstrap.

## What changed for Space

The checked commit history supplies these milestones:

| Commit | Delivered scope |
| --- | --- |
| `e4fb1d3c89`, 2026-10-12 | Moving finite asteroids, native mining probes, launch centers and linked collection stations. |
| `f09d11be8a`, 2026-10-13 | Material-paid spacecraft/pod assembly, reusable launches, cargo descent, handling, typed orders, company-private surveys and orbital resupply. |
| `5d2d50eed9`, 2026-10-13 | Delivered first-Core construction, explicit Core activation, paid panel/service, exclusive module inputs and exact saved assembly/cancellation; Plan 11 Groups 1-3. Numbered demo tours 18-30. |
| `9a47768ae3`, 2026-10-15 | Shared approach reservations, paid module activation, Dock service, exact cargo custody, evacuation and removal; extended rocket delivery/recovery, descent and Surface hub handling; versioned finite asteroid generation and deterministic motion. See the module, pod, collection and new-world tours. |
| `50ab2f8d87`, 2026-10-17 | Assembly manufacture and real product hulls; paid purchase/clone/replacement settlement; complete probe missions; heavy mining, pods and services; powered processing, hauler freight and product returns; final asteroid/probe art and packing. |
| `dd6491c48e`, 2026-10-17 | Archived execution ledgers under `docs/zz-rhr`; updated documentation links. No gameplay addition. |
| `5d250ceaca`, 2026-10-18 | Final heavy/hauler/module/processor art with ordinary-game review; joined save boundaries; paid Surface supply order execution and persistence. The full game application is rebuilt for this demo because that checkpoint had rebuilt only its focused supply target. |

## Completed Space features you can demonstrate

These are delivered scopes, rather than completion of the entire master or graphics profile.

| Completed feature | Where to see it |
| --- | --- |
| Moving finite asteroids with three resource classes, sizes, headings and reflected edges | `space`, `space-mining`: Nickel Ore, Water Ice and Rare Earth concentrate. |
| Native owned mining probes, pursuit, exclusive attachment, timed extraction and actual cargo holds | `space-mining`; select a probe or body in the inspector. |
| Synchronized assign/recall, compatible loaded retargeting, upkeep, cargo ageing and save/reload | `space`; use **Mine selected asteroid** / **Recall selected probe**. |
| Saved Sparse/Normal/Dense asteroid population setting | New Multilayer Industry worlds; Classic generates no field. |
| Waterside Surface launch-center construction, rotated footprint, native joining/removal and protected water access | `space-hub`, or Surface airport controls from 1990. Launch-center/rocket/pod final art remains a separate pending scope. |
| Explicit train/lorry hub links, whole-footprint 20-tile range, collection overlay and timed cargo handling | `space-hub` for the layout; `space-workshop`/`space-descent` for actual inventory and received cargo. |
| Layer switching and separate Surface/Space world state | Every checkpoint; use the layer selector or Alt+1/Alt+4. |
| Steel/Goods-paid hull and pod assembly, freight and service-part compartments | 18, Workshop; material and capital requirements are visible. |
| Reusable paid launch, deployment, recovery and explicit resupply receiver | 19 and 22; the same rocket and receiving probe survive the mission. |
| Paid cargo pods, timed descent and Surface receipt | 20; a pod carries actual mined cargo and is expended. |
| Saved typed queues and company-private survey knowledge | 21; inspect Survey/Visit/Mine actions and their actual progress. |
| Delivered first-Core construction and explicit activation | 23; completing a frame alone does not activate it. |
| Actual Core inventory, paid 30 kW panel and 148-tick RFMT/ELEC service | 24/25; power loss and manual pause retain the parts. |
| Exclusive module recipe packets, partial-power assembly, cancellation and save receipts | 26-30. |
| Storage and Power activation, and stock moved between modules | 29; **Activate module**, then a builder's **Unload to module**. |
| Shared approach queues: ordered admission, staging waits, target loss | 29; two builders visiting the Core. |
| Rocket freight straight to an operational Core, Storage or Dock | 29; Workshop **Next orbital destination** and **Launch to module**. |
| Docks with two powered service berths | Built from 29; **Connect power**, **Next berth**. |
| Explicit evacuation and safe removal of empty modules | 28 for evacuation, 29 for removal. |
| Pod release anywhere in the 20-tile circle, loaded recall and phase-by-phase failure recovery | 20. |
| Launch-center removal and company deletion that never discard Space cargo | 20, or any center with returned cargo. |
| Automatic Surface collection, the Goods train-rocket-pod loop and preserved cargo history | 20 with linked stations. |
| Generated asteroid field with starter asteroids beside a legal launch site, and depleted bodies kept | A new Multilayer Industry game. |
| Orbital Assembly construction/activation and paid manufacture into a real native vehicle | 31; finish the reserved builder, then inspect the completed product identity. |
| Paid purchase, clone and replacement accounting with material and capacity gates | Available through native vehicle commands; the Assembly inspector supplies **Queue manufacture**. Clone/replacement acceptance is command-level, without a dedicated demo button. |
| Heavy mining, actual cargo, paid pods, return allowance and model-specific service | 32 and 40. The heavy mining robot is distinct from the retained construction robot. |
| Paid hauler transfers between physical orbital inventories | 33. |
| Atomic Steel/Goods/Refined Metal/Water Ice processing and saved work history | 34-37; inspect the selected recipe and original batch totals. |
| Processing outage that retains original work and inputs | 43; restore the connected panel. |
| Manufactured Steel/Goods descended to Earth and collected at linked stations | 42. |
| Final asteroid, small-probe, heavy, hauler and module state/directional artwork | All Space worlds use the current packed art; 31-40 expose the new working states. |

The asteroid and probe foundations behind the first rows, from the earlier asteroid guide:

- **Population.** An actual Space-layer population with stable body IDs and three finite resources: metallic Nickel Ore, carbonaceous Water Ice and rich Rare Earth concentrate.
- **Bodies.** Small, Medium and Large bodies, with independent headings and speeds, edges and corners that reflect them, deterministic motion, and no impacts or resource respawning.
- **Density.** The exact Sparse/Normal/Dense population table, up to 14,336 bodies. New Multilayer Industry worlds use the saved new-game dropdown, defaulting to Normal. Classic disables the selector and generates no bodies.
- **Vehicles.** Native Space vehicle and engine identities, company group statistics, unit numbers and operating accounts. Existing vehicle save meanings 0..5 stay unchanged.
- **Mining.** Moving-target pursuit with a fixed adaptive timeout, one exclusive native lease per asteroid, attached translation, timed extraction and bounded holds.
- **Conserved cargo.** The reserve debit equals the actual hold credit. Full holds and unavailable packet capacity cannot debit more reserve.
- **Commands and art.** Synchronized mission and recall commands are checked against their owner. Native picking and a live inspector are included. The current build uses final asteroid and directional/state probe families, including actual scan, mining, cargo and blocked states.
- **Saves.** Version 405 added fleet, lease and cargo-provenance persistence, while the retained 403 underground and 404 survey worlds keep loading.

## Tour Space: moving asteroids and mining probes

```powershell
.\play.ps1 space-mining
```

1. The mining saves open **paused on Outer Space in 1990**. Unpause and use fast-forward. If the view is lost, select Outer Space and run `space_view` in the game console to recenter the field and open its inspector.
2. Click an asteroid to select it and see its class, current position and remaining/initial reserve. Resource quantities remain unknown to your company until a real survey completes; use 21 for that tour. Click a probe or its inspector row to select the actual company vehicle. The inspector lists each vehicle's mission and cargo hold; its selected line shows the target and work progress.
3. Watch pursuit, attachment and extraction. Each probe starts on a different class. **Mine selected asteroid** assigns the selected body to the selected owned probe. Assigning two probes to one body demonstrates exclusive mining: the second waits nearby until the slot frees or its approach times out. Click another probe before issuing its command.
4. Try **Recall selected probe** during partial mining. It detaches and returns with its cargo intact. A partially loaded probe can retarget only to compatible cargo; a full hold refuses further extraction.
5. Switch to another layer while running, then return. Bodies, motion, work, costs and cargo ageing continue while hidden; the global pause freezes the simulation.
6. Save a personal copy mid-approach, mining or return, and reload it. Leases, partial work, pursuit deadlines, cargo and fractional costs survive. Use `space-holds` to inspect full holds immediately, and `space` to start from the approach.

**Rates and costs.**

- **Extraction.** Each extraction credits **2 tonnes per 74 working ticks**, up to a **20-tonne** hold: ten economy days for a full hold.
- **Running costs.** Mining costs 500 internal pounds a day; travel, waiting and holding cost 100 a day, with saved fractional accrual.
- **Cargo.** Holds contain real native cargo packets with extraction-plane origins and ageing.
- **The older demo saves.** They pre-deploy their purchased probes and keep their original paid receipts: the three purchases debit 120,000 from the existing company. Their setup retires the earlier survey controllers.
- **Income.** Mining and returning to staging pay no delivery income.

In these older worlds the full holds stay in orbit. Use 19 for an actual Earth launch, 20 for cargo returned to the Surface, and 22 for a repeat resupply.

The old [preview sheet](../../../../media/baseset/openttd/space_preview.png) remains as compatibility content; the current game selects the completed Space art families.

**The older survey preview.** `space-survey` and `space-attached` show repeated surveying motions without extraction, costs, company fleet identity or completed company scans:

```powershell
.\play.ps1 space-survey
.\play.ps1 space-attached
```

Ordinary older worlds stay empty in Space until an explicit prepared demo adds its field.

## Tour the Surface launch hub

```powershell
.\play.ps1 space-hub
```

The checkpoint opens paused on **Outer Space**, because it retains the older survey preview.

1. Select **Surface** in the layer selector and find the real paid launch center at **Surface(16,4)**, with freight links already prepared. You can also find it in your company's station list with the airport filter.
2. Open the station window of a separate rail or lorry station nearby. Its **Launch hub** dropdown names the selected center.
3. **Collection area** shows the center's circular 20-tile Surface service area and the linked station tiles.
4. Select **No launch hub** to unlink, then select the center again.

**Membership rules.**

- **Joined rail.** The center's joined rail uses the same StationID and the native station cargo storage. Separate members keep their own IDs.
- **Eligibility.** A member must have the same company, be a Surface rail or lorry facility, and have **every station footprint tile** inside the circle. Extending a member outside the circle retires its link. Station-name position and ordinary catchment do not decide eligibility.
- **Limits.** Up to 32 separate stations can link to one center. Joining does not move its anchor or enlarge its orbital range.

**Build your own center.** In your own copy, open **Surface airport construction -> Space Launch Center**.

1. Use **Rotate**, choose flat clear land beside an orthogonal edge of flat navigable sea, river or canal, and build. The footprint is 4x3 or 3x4.
2. Multilayer Industry makes it available from 1990. Construction charges 500,000 plus clearing; removal costs 10,000.
3. Removing the last qualifying water edge is refused.
4. Demolishing the center keeps its joined rail, and rebuilding creates a new reference.
5. Ordinary aircraft do not use this facility.

**Prepared conditions.** This checkpoint uses explicitly prepared terrain and water, and native paid facility, rail and lorry commands. It has no free orbital dock, Core, rocket or cargo. Its Space field holds the older survey-preview controllers; load `space-mining` separately for native mining. The [hub checkpoint manifest](../../../../src/tests/fixtures/demos/space/hub-checkpoint.json) records the source and exact saved identity.

## Tour paid Space launches, returns and survey

1. **18: workshop.** On Surface, find the launch center through the company station list (airport filter), open its station window, then **Workshop**.
   - Select the working hull row and read the Steel, Goods, capital and remaining work.
   - Try **Pause / resume**, unpause globally, then compare the completed hull.
   - New hulls require actual stock; repeated button clicks do not create free craft. **Build pod** also consumes its declared recipe.
2. **19: reusable launch.** Open the same Workshop and select the rocket.
   - Unpause and watch preparing -> ascending -> deploying -> returning -> turnaround -> stored. The paid probe remains in orbit.
   - To test **Cancel / return** before the fuel burn, reload this starting checkpoint first. Choosing a later phase changes what a cancellation can do.
3. **20: mined return cargo.** Unpause the interrupted descent and inspect the center's actual cargo and the nearby collection stations.
   - Cargo must reach the Surface receiver; an orbital hold or an unrelated station visit is not a final delivery. A dropped pod is spent.
   - For another drop, select an eligible loaded probe and a paid pod in Workshop, then **Release cargo pod**. It needs remaining return allowance and an owned center inside the 20-tile zone.
4. **21: real survey.** Choose Outer Space and click the owned survey probe, or run `space_view` in the console to open or recenter the inspector.
   - Unpause to finish its 37/148 work, then compare the unknown quantities with the completed company scan.
   - Clear or edit its queue to try cancelling a partial scan; incomplete work grants no knowledge.
   - Use the real probe, not the older ownerless preview in 7 and 8.
5. **22: resupply.** Follow the already ascending cargo-only rocket through delivery and return. The receiving orbital probe keeps its identity.
   - For a repeat flight, select that probe in Workshop and **Set resupply receiver**. Then select the stored rocket, load actual parts, pods or compatible freight, **Load 15 Propellant**, and **Launch resupply**.
   - An occupied or full receiver holds the payload; it does not partially unload or create a replacement hull.
6. **Probe orders.** In the probe inspector, select a body and use **Mine**, **Survey** or **Visit** for compatible queue actions.
   - Inspect the current action and block reason; clear or reorder entries before retrying a different job.
   - Mining probes, survey probes and builders keep separate roles.
   - Onboard **Service / pause** requires real RFMT and ELEC parts and an idle, eligible probe. Core service is a separate physical berth in the next tour.

## Tour orbital Core construction, service and module work

Choose **Outer Space**, then the ordinary **Fund new industry** action, to open **Orbital construction**. Select a site row and use **Go to site**.

- **Sites.** Site numbers in this window are one-based. The Core is at (4,4) with its precise approach at (4,7), and the Storage blueprint is at (7,4).
- **Roles.** **Next builder** and **Next service probe** select different roles; read the selected vehicle before issuing work.

1. **23: first Core.** Unpause from 37/740 work ticks.
   - Both delivered materials stay reserved until final completion, and two builders do not double the job's speed.
   - Select the finished Core and press **Activate Core**. Unpause through its 74-tick activation before expecting inventory or service.
   - It bootstraps without an earlier station or utility source.
2. **24: inventory.** Select the operational Core and a builder at its precise approach.
   - Try **Load stored Goods** and **Unload to Core** with actual stock. Loading Steel is disabled while its stock is empty.
   - **Visit** can route a builder to a selected site; clear older builder orders if they prevent the visit.
   - Loading a material does not turn the builder's own hull into cargo.
3. **25: service.** Select the Core and try **Pause service** / **Resume service**, then **Toggle Core panel**.
   - With the panel disabled, global unpause must not advance service. Toggle it back to restore supply.
   - Completion consumes one RFMT and one ELEC, records 148 work ticks and 111,000 J, and replaces the return allowance with 2.
   - Reload and try **Cancel service**: only the reserved unused parts return, and spent work and energy are not refunded. Starting again needs a probe at the Core approach, an available berth and both actual parts.
4. **26: exclusive reservation.** Select Storage at (7,4) and inspect delivered 8/8 and consumed 0/0. Select a builder and **Queue construction**, clearing earlier orders if necessary.
   - The competing Storage at (2,4) cannot count this same stock.
   - **Reserve Core materials** is disabled for the already full job, and when the Core lacks either missing input.
   - A reservation alone never builds an active module.
5. **27: partial power.** Storage starts at 39 ticks plus preserved fractional work; its whole-tick display may at first hide that fraction.
   - Select the Core and **Resume service**, then inspect Storage while unpaused.
   - The panel provides 810 J per native tick: service may use 750 J and construction receives the remaining 60 J.
   - Pausing the service frees capacity. Another builder cannot multiply the Core's single construction slot.
6. **28: power loss.** Unpause before changing anything: partial work and Steel/Goods stay held because the panel is off.
   - Select the Core, **Toggle Core panel**, then watch construction resume.
   - Save and reload while held to compare progress and material ownership.
7. **29: finished frame.** Storage has finished 296 work ticks and consumed Steel 8/Goods 8 once. It stays an inert frame until you press **Activate module**; see [the module tour](#tour-modules-activation-docks-freight-and-removal).
8. **30: cancellation result.** Inspect the old cancelled site and the separately paid replacement at the same location. The Core now holds Steel 10/Goods 8; the old partial work stays recorded, while the replacement starts at zero.

   To perform the cancellation yourself:
   1. Reload 27 and cancel the paused Core service.
   2. Select the Storage job and **Cancel blueprint**.
   3. **Clear builder orders**, so the cancelled Construct action cannot block later visits.
   4. At its approach, use **Collect Steel**, **Visit** the Core and **Unload to Core**.
   5. Return to the cancelled site and collect the Goods.

   A builder carries one freight label at a time, so do not try to mix Steel and Goods in the same hold.

**Fixed.** The earlier station/probe-name compatibility regression is fixed in the current build.

## Tour modules: activation, Docks, freight and removal

These features came with commit `9a47768ae3`. Its native tests ran them from the checkpoints you can load:

- **29 (`space-module-frame`):** activation, the approach queue, Dock construction and **Launch to module**;
- **28 (`space-module-power`):** evacuation.

Open **Orbital construction** as in the Core tour (Outer Space, then **Fund new industry**), and the launch center's **Workshop** on Surface.

**Which modules work.**

- **Activation.** Core, Storage, Power, Dock, Assembly, Steelworks, Goodsworks, Refinery and Ice processor have operational functions. **Fuel store remains frame-only**, with activation explicitly disabled.
- **Placement.** Extensions need a shared edge with an owned operational Core, and every station module must stay outside the asteroid strip.
- **Timing.** Activation takes 74 ticks once the frame is finished, and needs the adjacent Core to be operational.

| Module | Steel / Goods | Work ticks | Price | Once active |
| --- | --- | --- | --- | --- |
| Core | 16 / 24 | 740 | 700,000 | 1,024 units per cargo label; 30 kW panel (810 J per tick); one service berth |
| Storage | 8 / 8 | 296 | 120,000 | Its own 1,024 units per cargo label; no panel and no work slots |
| Power | 6 / 20 | 444 | 140,000 | A 60 kW panel (1,620 J per tick) through the shared power ledger; no cargo |
| Dock | 8 / 12 | 444 | 160,000 | Two service berths and 1,024 units per cargo label; no panel of its own, so it must be connected to an adjacent Core or Power module |

1. **29: activate Storage.** Select the finished Storage at (7,4), which reads "Frame finished; awaiting activation", and press **Activate module**. Unpause through its 74 ticks; it then reads **Operational module**, with its own inventory.
2. **Move stock between modules.** The Core in 29 holds 2 Steel and one each of RFMT and ELEC.
   - Bring a builder to the Core's approach and press **Load stored Steel**.
   - **Visit** the Storage and press **Unload to module**.
   - The same Steel packet moves across. Nothing is paid or created.
3. **Queue at one approach.** Give both builders a **Visit** to the Core.
   - The first takes the precise approach. The second shows **Waiting** at a safe staging point, and moves in only after the first has left.
   - Craft are admitted in the order they asked. Waiting earns no work and freezes their action deadlines.
   - If the target is cancelled or removed, only claims on it are cleared, and the order reads "Target unavailable".
4. **Launch to module.** On Surface, open the Workshop and select the stored rocket.
   - **Load 15 Propellant** (10 for the trip out, 5 for the return). Then load the freight and parts for the module, for example **Load 1 Steel** twice, **Load 1 Refined Metal** and **Load 1 Electronics**. Freight and service parts are delivered together, all or nothing.
   - **Next orbital destination** cycles through your operational Cores, Storages and Docks. The window shows "Orbital destination: ÃƒÂ¢Ã¢â€šÂ¬Ã‚Â¦ at (x, y)".
   - **Launch to module** charges the 5,000 mission fee once. The rocket prepares (148 ticks) and ascends (222), then queues at the module's approach like any craft. It unloads into the module's own inventory and returns for its 74-tick turnaround on the pad.
   - **When the destination isn't ready.** If it is occupied, the rocket waits: "Waiting for the destination to become available". If it cannot take the whole payload, the rocket says so and keeps the cargo. If the module is removed mid-flight, the rocket brings the whole payload home once, using only the funded round-trip fuel.
5. **Build a Dock or Power module.**
   1. Choose the module, then **Place blueprint** on a free site that shares an edge with the Core and keeps its south approach free. The cost line and any refusal show whether a site is legal. In 29, the completed Storage's approach blocks one of the Power sites.
   2. Get the materials into the blueprint. Either **Launch to module** brings Steel and Goods to the Core and **Reserve Core materials** moves them into the selected blueprint, or builders carry them in, one cargo label per hold.
   3. Select a builder and **Queue construction**. When the work ticks are done, press **Activate module** (74 ticks).
6. **Power a Dock.**
   - Select the Dock, cycle **Next power source** to an adjacent owned Core or Power module, and press **Connect power**.
   - **Next berth** chooses berth 1 or 2 for a service probe.
   - **Service costs.** Each service needs one RFMT and one ELEC and uses 750 J per working tick. A Core (810 J per tick) can run one service at a time; a Power module (1,620 J per tick) can run both.
   - **Interruptions.** Pausing a job, or a source with no output, keeps the parts. **Disconnect power** ends the connection without refunding energy already used.
7. **Evacuate and remove.**
   - **28 (or 27): evacuate.** Select the Storage under construction and press **Evacuate cargo and cancel jobs**.
     - **Next cargo destination** picks an adjacent module that can hold everything: "Cargo destination: module n. Craft must depart before removal."
     - Unconsumed Steel, Goods and service parts move there; spent work and energy stay spent.
     - If no adjacent inventory can hold the whole evacuation, it refuses and nothing moves.
   - **29: remove.** Select an empty module, for example the finished Storage frame before activating it, and press **Remove empty module**.
     - The window says what blocks removal: another owner, stored cargo, retained service jobs, a construction job, a craft on the footprint or approach, stocked modules that need this Core, connected Docks, or a full history.
     - A removed module's spent materials stay in the company's history. A module placed there later gets a new identity and is paid for again.

## Tour pod releases, recalls and failures

The native tests ran these from **20** (`space-descent`). It has an original paid miner with 10 mined Nickel Ore aboard and one carried pod shell.

1. **Release anywhere in the circle.** A pod can be released from any point inside your center's 20-tile collection circle, boundary included: 20 tiles along an axis, and diagonally (14,14) is inside while (15,15) is outside.
   - The descent records that exact point.
   - The release is checked again when you press it, so a probe that has moved since the quote may be refused.
2. **Recall with cargo.** **Recall selected probe** stops a loaded probe inside its own center's circle instead of at staging, so **Release cargo pod** works from there.
3. **Refusals keep everything.** In each of these cases the cargo stays aboard:
   - a foreign center overlapping yours, which is never used;
   - full center storage, counting incoming reservations;
   - no return allowance left, no carried pod, or an unsupported cargo.

   A full or lost center makes a descending pod wait ("Awaiting recovery") with its cargo; **Recover pod here** retargets it.
4. **Flights by phase.**
   - **Preparing.** **Cancel / return** keeps the fuel and the payload; the mission fee is not refunded.
   - **Ascending.** Not cancellable.
   - **Delivering.** An unresolved delivery can be cancelled through the already funded return leg.
   - **Lost target.** A lost receiver or module makes the rocket wait, then bring its payload back once.
5. **Launch center removal.** Removing a center is refused while craft, pods, descents or handling still depend on it. It is also refused while the center holds returned cargo and no other freight facility belongs to its station.
   - A joined rail or lorry station keeps that cargo when the launch facility goes.
   - Rockets are not aircraft: ordinary aircraft orders, hangars, cloning and refits refuse them.
6. **Companies.** A company cannot be deleted while its Space craft, pods or descents still hold cargo. A takeover moves craft, pods, structures and survey knowledge to the buyer.

## Tour Surface collection and the Goods loop

Asteroid cargo that reaches a launch center does not stay there.

- **Automatic collection.** Handling moves the cargo automatically to the center's linked rail and lorry stations, but only cargo that a vehicle has already tried to load at that station.
  - It moves at most 8 units per tick, shared across all jobs, within each station's storage.
  - It charges a mass fee, booked as an "Other" expense, and earns no income. The final delivery by train or lorry is paid as usual.
- **Membership changes.** **No launch hub** is refused while a handling job is active. Moving or extending a member out of the circle retires its link, and its unfinished handling returns to the center.
- **The Goods loop.** This was verified natively from 20:
  - Goods brought by an ordinary train to a platform joined to the center become launch stock.
  - A rocket carries them to a builder in orbit, a pod brings them back down, and handling moves them to a linked lorry station.
  - The ordinary rail wagons carry Goods; the mineral-only mine trucks do not.
- **Cargo history.** Age, source and transfer credit survive every handling step and descent. Relinking or handling never makes cargo "fresh".

## New worlds: the generated asteroid field

Start a new Multilayer Industry game (menu 1) and choose **Outer Space**.

- **Density.** The new-game **Sparse/Normal/Dense** setting picks the field size, up to 14,336 bodies on the largest maps. For example, seed `0x20261012` on a Normal 256 x 256 map holds 95,050 t Nickel Ore, 99,715 t Water Ice and 4,018 t Rare Earth.
- **Starter asteroids.**
  - During generation, before any company exists, the game looks for a legal launch-center site: flat, clear land beside navigable water, the same test that building one uses.
  - It then places one asteroid of each class on the three free 4-tile grid points nearest that site's orbital position, preferring a site within 32 tiles of a probe's approach.
  - If the map has no legal site, the world records that and puts the starters at the centre instead.
  - Building a launch center later never rerolls the field.
- **The strip.** Asteroids move in the middle half of the map's height, with eight headings and three speeds, and bounce off its edges and corners. Station modules must stay north or south of that strip: **Place blueprint** refuses inside it.
- **Depletion.** A mined-out asteroid stays visible and keeps moving, with 0 t left.
  - Mining stops when it empties; the probe keeps its cargo and returns.
  - New mining orders to it refuse.
  - An order whose target no longer exists shows "Target unavailable".
- **Saves.** The field and its starting choice are saved from save version 424 onwards. Older worlds keep their field unchanged, with the starting choice marked unknown.

## Tour orbital manufacture and final module art

```powershell
.\play.ps1 space-factory
```

1. Press **Alt+4**, then open **Fund new industry** for **Orbital construction**. Select the operational **Assembly** row and **Go to site**. Its construction and activation were paid before this checkpoint.
2. Read the current construction-probe job: **74/148 work ticks**, with its original **4 Steel + 8 Goods** reserved. This checkpoint has already spent **55,500 J** on that job. Unpause to finish the remaining paid work. Completion creates one actual construction probe and reports its vehicle identity.
3. Pause again and inspect the new probe. Reopening the window or reloading a personal save must retain the same completed product.
4. For another product, clear the completed job with **Cancel / clear manufacture job**, choose **Next vehicle recipe** and **Next input inventory**, then **Queue manufacture**. The Assembly needs its real power connection, input materials, funds and a free product berth. Queuing with missing inputs waits; it creates no hull.
5. Small mining, survey and construction probes can be manufactured locally. Advanced recipes add Robot Parts and Electronics. The **Heavy mining robot** is a separate model from the retained **Heavy robot** construction role; the orbital hauler is another role. Do not expect the retained construction robot or hauler to have the mining robot's service implementation.

| Product | Actual material bill | Capital | Manufacture work |
| --- | --- | --- | --- |
| Mining or construction probe | 4 Steel + 8 Goods | 100,000 | 148 ticks |
| Survey probe | 2 Steel + 4 Goods | 60,000 | 148 ticks |
| Heavy mining robot, retained heavy construction robot, or orbital hauler | 6 Steel + 12 Goods + 4 Robot Parts + 2 Electronics | 200,000 | 592 ticks |

For module construction, load **38 / `space-power`**. Select the unfinished Power module, unpause from **222/444** work ticks, then press **Activate module** after its frame completes. Its 60 kW panel becomes available only after activation. Construction stages, glass activity, couplings and per-tile foregrounds now use the final art.

Load **39 / `space-fuel-store`** to see the completed Fuel store frame. It consumed its **6 Steel + 12 Goods** and **296 work ticks**, but provides no operating fuel service. The caption explicitly says **Fuel store (frame only)** and activation is unavailable.

## Tour heavy mining, typed missions and Dock service

```powershell
.\play.ps1 space-heavy
.\play.ps1 space-missions
.\play.ps1 space-heavy-service
```

- **32: heavy mining.** Use **Alt+4**, click the heavy mining robot or its inspector row, and unpause. Follow pursuit, exclusive asteroid attachment, extraction and the larger native hold. Its model owns its capacity, mining cadence, operating costs, pod fit and return allowance. It shares the finite asteroid reserve with small probes. Turning, loaded travel, working and blocked art follow actual state.
- **41: typed queue.** Inspect the actual probe's **Visit**, then **Mine** actions. Unpause to advance the queue; try another body or edit the orders on a personal save. Changing a survey mission discards unfinished scan work. Company knowledge, target generations, action progress and holds survive reload. Use **21** for a survey close to completion.
- **Return and unloading.** The inspector displays saved typed **Unload** and **Return / service** actions. Its direct controls add visits, surveys and mining, edit the queue, recall and start onboard service. Creating a new typed Unload still uses the command/script interface; there is no dedicated Unload button. For an Earth cargo drop, a probe still needs its own valid center, a paid carried pod, room for the whole descent and return allowance. Module freight uses the physical approach and receiver inventory.
- **40: heavy Dock service.** Open **Orbital construction**, select the Dock and its occupied **Next berth**. The displayed bill and remaining work belong to that actual craft model. Pause/resume service, or disconnect/reconnect the source, to see retained original parts and spent energy. Unpause to finish and inspect the renewed allowance. A Core supplies one service berth; a powered Dock supplies two, subject to actual power.

The Dock frame and foreground surround its original berthed craft. Model-specific service bills are visible; a heavy miner does not silently inherit a small probe's bill. Native command dispatch and replay were accepted for these roles; a complete live multiplayer demo remains a later gate.

| Mining role | Hold | Extraction | Service parts / work | Active / idle daily upkeep |
| --- | --- | --- | --- | --- |
| Small mining probe | 20 t | 2 units per 74 working ticks | 1 RFMT + 1 ELEC / 148 ticks | 500 / 100 |
| Heavy mining robot | 40 t | 4 units per 74 working ticks | 2 RFMT + 2 ELEC / 296 ticks | 1,000 / 200 |

Both mining roles renew two return allowances after completed service; physical paid pod shells are still required for descent. Prices are internal pounds before the display currency conversion.

**Seeing the artwork.** The inspectors can cover most of the viewport. Use **Go to site**, then close the inspectors with Escape to see the centred modules and craft. Reopen **Fund new industry** when you need the controls again.

## Tour processors, haulers and Earth product returns

```powershell
.\play.ps1 space-steel
.\play.ps1 space-goods
.\play.ps1 space-refinery
.\play.ps1 space-ice
.\play.ps1 space-hauler
.\play.ps1 space-products
```

Open **Orbital construction** on **Alt+4** and select the named processor. The inspector shows the selected recipe, held inputs, per-batch requirements, work, lifetime electricity, output capacity and consumed/produced history.

| Starting point | What to watch |
| --- | --- |
| **34: Steelworks** | Transported, actually mined Nickel Ore becomes Steel. The powered batch retains inputs while its paid work advances. |
| **35: Goodsworks** | Steel and Rare Earth must both be present. Unpause the partial batch and compare consumed inputs with produced Goods. |
| **36: Refinery** | Nickel Ore becomes Refined Metal through its bounded conversion recipe. |
| **37: Water Ice alternative** | The Refinery is already configured to the `PICE` alternative. Read the actual recipe and physical output; the shared processor artwork does not identify its recipe. |
| **43: panel outage** | The Steelworks keeps partial work and inputs. Its power wait is visible even while globally paused. Select the actual source Core, enable its panel, then unpause to continue. |

Steelworks/Goodsworks use **74 work ticks and 55,500 J per batch**. Refinery/Ice alternatives earn **74 unpowered work ticks**; the display must not invent an electricity charge. Missing secondary inputs, a full output or an unavailable source retains the batch and shows the limiting reason. These are bounded native recipes, with original cargo units and packet history.

**33: orbital hauler.** The paid hauler starts travelling with 2 actual Steel units already loaded and an existing typed Unload order. Follow it in the inspector and unpause through approach admission and transfer. The same cargo moves between compatible module inventories, subject to destination capacity; an occupied approach waits. Its own hold, recipe and loaded presentation are distinct from a mining probe; this checkpoint carries one freight label. Save during travel and reload to continue the same transfer.

**42: manufactured cargo to Earth.** Steel/Goods produced in orbit have already entered paid descent. Unpause, press **Alt+1**, and inspect the launch center's receipt of **2 Steel and 4 Goods** and its linked rail/lorry stations. Linked collection requires an actual vehicle pickup request; this checkpoint guarantees descent and receipt, rather than a prearranged final delivery route. Handling moves returned packets with their history intact. The original paid cargo pod is expended. This starting point demonstrates downstream orbital products; its inherited Surface stock and terrain were prepared.

## Paid Surface supplies and the remaining bootstrap

The latest commit also implements synchronized paid orders for **Steel, Refined Metal, Electronics and Batteries** at an eligible ordinary Surface rail station. Orders charge once, reserve bounded receiving room and company/day allowance, dispatch original company-supply cargo through station ticks, and support once-only cancellation/refund before fulfillment. Save/reload, takeover and station loss are covered by the focused native producer evidence.

There is **no player-facing order panel in this demo**. These commands are an implemented bootstrap producer, not an automatic supply source for the featured worlds. Actual carrier delivery into collector/riser bills and ordinary application acceptance remain **Plan 6 Group 13CD**. The selected final acceptance route still requires actual Surface factory production before the continuous Earth-launch/orbital chain.

## Still unfinished

- **Unassisted starting chain.** Actual Surface factory production and the continuous launch/orbital expansion acceptance, plus final performance/closure gates ([orbital stations](r2mlvs11-orbital-stations-and-manufacturing.md) Groups 15/15B/16). Inherited demo stocks cannot satisfy this gate.
- **Some UI controls.** Surface supply ordering and creating typed Unload orders still need the command/script interface. Prepared freight/descent queues run normally.
- **Purchased ground construction delivery.** Plan 6 Group 13CD, the current master continuation, including ordinary application acceptance.
- **Fuel store operations and some advanced-role service.** Fuel store remains a built frame; retained construction robot model 16 and hauler model 17 do not gain an unsupported service route from their artwork.
- **Launch art.** The separate launch-center/rocket/pod final-art obligation ([launch centers](r2mlvs9-launch-centers-and-rockets.md) task 16.2) remains open. Asteroids, probes and the reviewed orbital families now have final packed/runtime artwork.
- **Multiplayer.** Full live multiplayer acceptance of Space.

## Verification and feedback

The current ordinary application checks are [optimized demo verification](../../../../logs/space-demo-verify-RelWithDebInfo.json) and [Debug demo verification](../../../../logs/space-demo-verify-Debug.json). They record actual load/resave/reload results and unchanged saved Space owners for every featured checkpoint. The optimized run additionally advances the new working worlds through ordinary ticks. New save provenance is in [current checkpoint preparation](../../../../logs/space-demo-prepare-RelWithDebInfo.json).

**Refresh result:** all **41 featured saves** pass in both builds, preserving **33 checked owner/metadata families** on reload. The optimized build also passes **12 working-world continuations**. The ordinary factory and Steelworks inspector views were checked. [The combined receipt](../../../../logs/space-demo-verification.json) binds current applications, logs, snapshots, launcher aliases and selected captures. These checks demonstrate prepared gameplay; they do not close the remaining bootstrap or full multiplayer acceptance.

Reproduce after building the ordinary application:

```powershell
python src/tests/multilayer/acceptance/space_demo_checks.py verify --advance
python src/tests/multilayer/acceptance/space_demo_checks.py verify --configuration Debug
```

`prepare` is a one-time curation command and refuses to replace existing snapshots. The runner uses isolated repository data homes and writes its logs/reports beneath `logs` and `mlm/reviews/space-demo`; it never changes `build/play.cfg` or the retained starting saves. Build logs are `logs/demo-update-build-RelWithDebInfo.log` and `logs/demo-update-build-Debug.log`.

The records below are **dated producer evidence**, retained for the earlier tours; their counts do not describe this new demo check.

- **Plan 11 Group 3.** It passes 3,173 assertions in 10 focused cases per configuration, plus actual interrupted assembly and cancellation, ten native and preview corrupt-save refusals, and legacy first-Core continuation.
  - All 26 tracked Space owner families match across configurations.
  - Slow Debug wrappers were split into companion files, and the final focused bursts finish within two minutes.
  - Evidence: [orbital assembly verification](../../../../logs/orbital-assembly-verification.json).
- **Featured saves.** All 28 featured saves (menu 3-30) were loaded and resaved with the current RelWithDebInfo game, in 58.7, 29.8 and 28.4-second bursts.
  - All 13 new aliases and their numbers resolve correctly.
  - The version-418 module saves preserve all 26 checked owner families.
  - Evidence: [demo guide verification](../../../../logs/demo-guide-verification.json).
- **Load checks.** The paid Space manifest records each retained source and hash. Demo load checks use the ordinary current game and never overwrite those starting saves.
- **Commit `9a47768ae3` features.** Each was verified in both RelWithDebInfo and Debug, with ordinary and replayed saves compared across 26 owner families. The frozen indexes are:
  - modules: [activation](../../../../logs/orbital-activation-verification.json), [approach](../../../../logs/space-approach-verification.json), [Dock](../../../../logs/orbital-dock-verification.json), [removal](../../../../logs/orbital-removal-verification.json), [evacuation](../../../../logs/orbital-evacuation-verification.json), [lifecycle](../../../../logs/orbital-lifecycle-verification.json) and [handoff](../../../../logs/orbital-handoff-debug-verification.json);
  - flights: [module flights](../../../../logs/module-flight-verification.json), [release admission](../../../../logs/release-admission-verification.json), [descent custody](../../../../logs/descent-custody-verification.json), [cargo return](../../../../logs/cargo-return-verification.json), [phase failures](../../../../logs/phase-failure-verification.json), [retirement](../../../../logs/launch-retirement-verification.json) and [phase ledgers](../../../../logs/phase-ledger-verification.json);
  - the Surface loop: [commodity loop](../../../../logs/commodity-loop-verification.json), [release geometry](../../../../logs/release-geometry-verification.json), [surface collection](../../../../logs/surface-collection-verification.json) and [hub circulation](../../../../logs/hub-circulation-verification.json);
  - asteroids: [catalogue](../../../../logs/asteroid-catalogue-verification.json), [launch site](../../../../logs/launch-site-verification.json), [generation inputs](../../../../logs/asteroid-input-verification.json), [motion](../../../../logs/asteroid-motion-verification.json), [depletion](../../../../logs/asteroid-depletion-verification.json), [strip](../../../../logs/asteroid-band-verification.json) and [handoff](../../../../logs/asteroid-handoff-verification.json).
  - [The runtime handoff](../../../zz-rhr/upgrades/2026-10/r2mlsp-q-runtime-handoff.md) and [the generation handoff](../../../zz-rhr/upgrades/2026-10/r2mlsp-x-generation-handoff.md) summarize the public contracts.

When reporting trouble, include:

- the **menu number or name** and your personal save;
- the selected layer and the exact control you clicked;
- the probe, rocket, site or Core you had selected;
- the visible action, wait or refusal.
