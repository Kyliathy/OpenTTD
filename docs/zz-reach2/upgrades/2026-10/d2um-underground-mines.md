# Multilayer demo - underground mines and Outer Space

Bundled player walkthrough for the [underground master](r2mlvs3mp-master-plan.md) and [asteroids/probes plan](r2mlvs10-asteroids-and-mining-probes.md). Updated **2026-10-12**. The underground baseline is tagged **`demo3`**; the later Space work is in the current workspace. Further implementation is paused for this playtest.

The default demo now starts with **a working Underground Coal mine and a mineral train delivering to a Surface power station**. The same world retains the completed50-cell public tunnel and the original TBM parked at its far exit. Separate checkpoints cover excavation planning, active spoil haulage, far-exit construction and the newly delivered Deep collector graphics. All eight current checkpoints start paused.

## Choose your tour

From the repository root, use the default **RelWithDebInfo** build:

```powershell
.\play.ps1 demo          # Underground mine, freight and Surface coal delivery
.\play.ps1 space         # Moving asteroids and three native mining probes
.\play.ps1 space-hub     # New Surface launch center and linked freight stations
```

These are **separate prepared worlds**. Switching the old underground world's
camera to Space does not generate asteroids. Load `space` for mining, or
`space-hub` for the newly delivered facility. All open paused; save a personal
copy before experimenting. `-Config Debug` selects the slower checked build.

A useful tour order is `demo` -> `space-mining` -> `space-holds` -> `space-hub`.
Each command starts the chosen checkpoint; it does not merge worlds or cargo.

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

## Pick a checkpoint

| Command | What is ready | What to do |
| --- | --- | --- |
| `.\play.ps1 space` | Three native company mining probes approaching moving asteroids in1990 | Unpause on Outer Space; click a body/probe and inspect its mission and hold. |
| `.\play.ps1 space-mining` | Probes already attached and extracting their three resource classes | Watch finite reserves fall and20-tonne holds fill. |
| `.\play.ps1 space-holds` | Three full native probe holds at orbital staging | Inspect cargo, saved costs and ageing; cargo delivery to Earth is still pending. |
| `.\play.ps1 space-hub` | Paid Surface launch center, joined rail and explicitly linked separate train/lorry stations | Select **Surface**, find the center at **(16,4)**, then open a member station's **Launch hub** selector and toggle **Collection area**. This world contains the older survey preview. |
| `.\play.ps1 space-survey` / `space-attached` | Older ownerless survey-preview controllers, approaching/already attached | Watch drift, attachment and departure; they create no cargo or company survey knowledge. |
| `.\play.ps1 demo` | Finite Coal mine, mineral train, connected rail portal and Surface receiver; one actual delivery has already succeeded | Select Underground, inspect the mine/station, then unpause and follow the next load. `mine-haulage` is an alias. |
| `.\play.ps1 mine-build` | Earlier state of that same mine route, with paid machinery still under construction | Select Underground at(8,9), unpause and watch completion, extraction and the first full load. |
| `.\play.ps1 tbm-work` | Original50-cell TBM job at0/50 cuts, accepted work and enabled spoil hauler | Open Underground Construction -> Projects, open the machine and unpause. |
| `.\play.ps1 tbm-plan` | Earlier44-cell job with stopped trains | Pause and extend the existing job with a straight endpoint up to six cells farther ahead. |
| `.\play.ps1 tbm-exit` | All50 cells cut; whole cutter stopped clear of the far exit | Build exit B at Underground(13,60), park the same machine there and release the route. |
| `.\play.ps1 tbm-tunnel` | Completed public A-to-B tunnel; original TBM parked at B | Follow Train#1 through both connected portals; Train#2 remains stored at B. |
| `.\play.ps1 collector-build` | Paid Deep collector at construction stage1, with its actual construction materials already received | Choose Deep Underground; inspect machinery around(51,34), then unpause to watch the remaining stages. |
| `.\play.ps1 collector-ready` | Same collector complete at its natural vent | Choose Deep Underground around(51,34); inspect its actual new machinery and status glass. |
| `.\play.ps1 underground-mines-legacy` | Preserved older bent-route checkpoint | Compare the original saved geology and repaired TBM controls. |

The eight refreshed snapshots and their source/build identities are recorded in [the checkpoint manifest](../../../../src/tests/fixtures/demos/underground/demo-checkpoints.json). They use current native format403. Original390/393/399 inputs remain preserved. The mine checkpoints extend the completed-tunnel world; the collector checkpoints are a separate compact review world.

**Prepared conditions:** the mine workshop has explicitly prepared excavated floor. Mine machinery, locomotive, wagon, stations, portal and Surface receiver were bought through native commands; coal comes only from the original natural deposit. The live preparation run reduced its reserve from20,000 to19,856 tonnes and earned delivery income. The collector world has prepared floor and fixture-delivered construction materials; its native capital/work/material receipt and rendered stages are real. These starting conditions shorten the tour. A transported first geothermal plant build remains pending.

## Completed features you can demonstrate

These are delivered scopes, rather than completion of the entire master or graphics profile.

| Completed feature | Where to see it |
| --- | --- |
| Moving finite asteroids with three resource classes, sizes, headings and reflected edges | `space`, `space-mining`; Nickel Ore, Water Ice and Rare Earth concentrate. |
| Native owned mining probes, pursuit, exclusive attachment, timed extraction and actual cargo holds | `space-mining`; select a probe/body in the inspector. |
| Synchronized assign/recall, compatible loaded retargeting, upkeep, cargo ageing and save/reload | `space`; use **Mine selected asteroid** / **Recall selected probe**. |
| Saved Sparse/Normal/Dense asteroid population setting | New Multilayer Industry worlds; Classic generates no field. |
| Waterside Surface launch-center construction, rotated footprint, native joining/removal and protected water access | `space-hub`, or Surface airport controls from1990. The pad art is temporary. |
| Explicit same-company train/lorry hub links, whole-footprint20-tile range and separate collection overlay | `space-hub`; separate stations retain their IDs, while joined facilities share native center storage. Actual handling remains pending. |
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

Further completed foundations include paid external electrical service for prototype tubes, bounded native Heat routing/allocation, paid portal-bound electrical riser construction/links and shared energy lineage. Their focused native tests establish those boundaries. Normal tube/geothermal construction tools, the complete tube fleet/terminals and an operating transported-input geothermal powerhouse still need their later master integrations; this tour does not provide those unfinished player flows.

## Tour Space: moving asteroids and mining probes

```powershell
.\play.ps1 space-mining
```

1. The mining saves open **paused on Outer Space in1990**. Unpause and use
   fast-forward. If the view is lost, select Outer Space and run `space_view`
   in the game console to recenter the field and open its inspector.
2. Click an asteroid to select its class, position and initial/remaining reserve.
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
with saved fractional accrual. The demo explicitly purchased three probes for
120,000 total and predeployed them; it does not demonstrate Earth launches.
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

## Space limits for this playtest

Full probe holds **stay in orbit**. The Surface center and freight memberships
are implemented; physical rocket commissioning, cargo descent/pods, timed
Surface handling and paid repeat/service missions are still pending. Linking a
station does not yet send cargo to it.

The later mining/survey/construction roles, typed vehicle order queues,
survey-gated resource quantities, material-delivered orbital construction and
Steel/Goods processing/reuse are planned requirements. The current inspector
still shows reserves without a completed scan. Temporary art remains in use.

A known cosmetic regression can give an older station a probe-like name. It is
still a station: use its location and facility/vehicle lists to identify it. This
saved-name compatibility fix is recorded for the development resume.

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

For an area preview, use an eligible sealed portal with no active project, choose **New area** and drag the requested floor. A portal with existing work directs you to **Projects** and the extension flow. Blue cells show charged extra working space; requested rectangles still need a physically reachable machine plan. The straight-corridor shortcut accepts2?60 cells; larger area plans have separate bounded planner limits.

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

The current Space/launch-hub cycle builds both configurations and checks native
construction, selection, station extension, ordinary/repeated reload and corrupt
transport rejection. Mining checkpoints preserve exact native replay through two
independent2,200-tick continuations each. All eight underground snapshots preserve
27 checked owner chunks in both executable configurations. The retained mine,
Space mining/survey and new hub saves remain separate, immutable starting points.


The preparation helper loads the old TBM worlds and new native mine/collector scenes through the ordinary rebuilt game, pauses them and independently reloads each saved result. Current-format resource/project/cargo owners remain exact. Old399 TBM files migrate only their empty utility schema to403 while retaining original epoch/IDs/quantities. Graphical checks cover the mine and both collector stages; collector machinery matches exact native pixels. Additional loaded-world continuation passes in both Debug and RelWithDebInfo: the Coal shuttle returns empty and earns another delivery while reserves fall19,856->19,797; the partial collector advances74->296 labour ticks without added materials. [Verification receipt](../../../../logs/underground-demo-verification.json) records current results and identities. These automated checks leave your personal playtest open.

Please note the checkpoint and your personal save name if anything gets stuck. Useful feedback is whether you could find the correct layer, follow actual mine output and freight, understand a wait/refusal, extend the TBM plan, build B without discarding the machine, see the collector's construction changes, and save/reload without losing progress or cargo.
