# Underground mines demo

Player walkthrough for the [portal/TBM master](r2mlvs3mp-master-plan.md). Updated **2026-10-12**, after commit `0840083c4054` and completion of the interrupted chemical works source task40C. Repository checkpoint tag: **`demo3`**.

The default demo now starts with **a working Underground Coal mine and a mineral train delivering to a Surface power station**. The same world retains the completed50-cell public tunnel and the original TBM parked at its far exit. Separate checkpoints cover excavation planning, active spoil haulage, far-exit construction and the newly delivered Deep collector graphics. All eight current checkpoints start paused.

## Start here

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

The preparation helper loads the old TBM worlds and new native mine/collector scenes through the ordinary rebuilt game, pauses them and independently reloads each saved result. Current-format resource/project/cargo owners remain exact. Old399 TBM files migrate only their empty utility schema to403 while retaining original epoch/IDs/quantities. Graphical checks cover the mine and both collector stages; collector machinery matches exact native pixels. Additional loaded-world continuation passes in both Debug and RelWithDebInfo: the Coal shuttle returns empty and earns another delivery while reserves fall19,856->19,797; the partial collector advances74->296 labour ticks without added materials. [Verification receipt](../../../../logs/underground-demo-verification.json) records current results and identities. These automated checks leave your personal playtest open.

Please note the checkpoint and your personal save name if anything gets stuck. Useful feedback is whether you could find the correct layer, follow actual mine output and freight, understand a wait/refusal, extend the TBM plan, build B without discarding the machine, see the collector's construction changes, and save/reload without losing progress or cargo.
