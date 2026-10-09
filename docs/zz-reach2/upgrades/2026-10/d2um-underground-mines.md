# Underground mines demo

Player walkthrough for the delivered [portal/TBM master](r2mlvs3mp-master-plan.md), its [demo checkpoint](portal-tbm/r2mlvs3mp-z-l-demo-checkpoint.md) and the active [TBM fixes](r2tbmf-tbm-fixes.md). Architecture: [map layers](../../architecture/map-layers/archi-map-layers.md). Updated 2026-10-09.

The default demo now contains the current TBM fixes: a reviewed **50-cell straight tunnel**, regional geology, the same actual machine and an assigned spoil hauler. Both trains and the project are ready; the game starts paused. Unpause to watch cutting and haulage. Additional checkpoints let you try planning, build the far exit, or inspect the completed public tunnel immediately.

**Artifact-layout migration (2026-10-09):** this plan consumes the parent [Development artifact layout](r2mlvs-multilayer-vehicles-and-structures.md#development-artifact-layout), implemented by [R2FC](r2fc-file-cleanup.md). It supersedes earlier docs asset/review/helper placement and fixed runtime-config destinations below. New native homes use `mlm/runtime/`, generated feature reviews use `mlm/reviews/`, logs remain under `logs/`, and reusable code/curated inputs keep their code/test owners. Paths and focused native/cold verification are complete; R2FC records the results and retained-input identities. Repository-relative executable commands run from the checkout root through their context-aware wrappers. Historical pass records remain dated evidence and are not new acceptance claims. No gameplay task is completed by this migration and the separate post-demo fixture-pruning gate remains.

## Launch it

From PowerShell:

```powershell
cd D:\Codez\Sandbox\OpenTTD
.\play.ps1 demo
```

The longer name works too:

```powershell
.\play.ps1 underground-mines
```

Or run `.\play.ps1` without arguments and choose **underground-mines** under **Demos (Multilayer Industry)**. The demo has its own permanent entry above the recent saves, so you do not have to hunt for it among autosaves. The launcher prints this guide's path before starting the game.

The default is the optimized **RelWithDebInfo** build. To use the Debug build when reporting a problem:

```powershell
.\play.ps1 demo -Config Debug
```

Both choices use `build/play.cfg` and load [underground-demo-tbm-399.sav](../../../../src/tests/fixtures/haulage/planning/underground-demo-tbm-399.sav). Featured demos prefer their retained repository snapshot, so a same-named save in `build/save` cannot silently replace the reviewed starting state. A generated copy is used only if the retained snapshot is missing. Your own saves go into `build/save` and can be selected by name or path. The [original390 save](../../../../src/tests/fixtures/haulage/planning/underground-demo-390.sav) and [repaired393 checkpoint](../../../../src/tests/fixtures/haulage/planning/underground-demo-tbm-ui-393.sav) are preserved.

The game starts **paused**. Before experimenting, use **Save game** to make your own copy, for example `underground-mines-my-run`. This preserves the original demo for another tour. Existing saves can be reopened by name: `.\play.ps1 underground-mines-my-run`.

## Get your bearings

Choose how much of the journey to play. All current checkpoints follow the **same machine and world** and start paused:

| Command | Starting point | First action |
| --- | --- | --- |
| `.\play.ps1 demo` | 0/50 cuts, accepted work and ready hauler | Open Projects, then unpause. `underground-mines` and `tbm-work` are aliases for this same checkpoint. |
| `.\play.ps1 tbm-plan` | Earlier44-cell job with stopped trains | Choose Extend work (Pause and extend if running), select Straight tunnel, and preview up to six more cells. |
| `.\play.ps1 tbm-exit` | All50 cells cut; entire cutter stopped clear of the exit site | Build B from Underground, then choose its hangar and park the machine. |
| `.\play.ps1 tbm-tunnel` | Completed public A-to-B route; original cutter parked at B | Unpause and follow Train#1 through both portals. Train#2 is retained in B's magazine. |
| `.\play.ps1 underground-mines-legacy` | Preserved older bent-route demo | Compare the repaired interface and original saved geology on your older world. |

The completed tunnel has released its excavation project; find its parked machine through the train list. The planning/work/exit checkpoints still have a project in **Projects**. Exact native evidence is in [the final checkpoint](tbm-fixes/r2tbmf-zb-final-notes.md).

1. Use the main toolbar's **layer selector**, or **N**, to switch between Surface and Underground. Surface has the hauling infrastructure; Underground has the excavation. Deep Underground is a separate layer, with access gated until **1 January 1980** in the game calendar.
2. Open **Underground Construction -> Projects**, select the project for **Train#2**, and choose **Open machine**. Its vehicle window's centre-view control finds the actual cutter, including while it is at the Surface magazine. **Train#1** is its automatic hauler.
3. Choose **Portal controls** from Projects. The portal window shows **Next:** guidance, waits and controls. **Machine** opens the cutter's vehicle window; use that to follow its progress.
4. Unpause the game and watch a complete cycle. You can pause the whole game to inspect it, or use the project's **Pause / Resume** controls to hold the excavation while the rest of the world runs.

While viewing Underground, the main toolbar button that opens airport construction on Surface becomes a **TBM icon** for **Underground Construction**. Its compact palette contains **Survey**, **Projects**, **New area** and **Fund a mine**. **Projects** opens the actual owned jobs on this layer, with phase, progress and hold reason; select a footprint, centre the map, open its machine or open the portal controls. **Requirements and cutter dates** expands optional guidance. Escape clears the active tool and footprint.

The default checkpoint starts before the first of50 accepted cuts. Its project and both trains are already enabled, so only the global game pause needs releasing. Use the toolbar's fast-forward control if you want to shorten the wait. The separate `tbm-exit` checkpoint skips directly to the far-exit work. If anything waits, read the portal's reason before changing the layout; the recovery table in [the full how-to](r2mlvs-how-to.md#if-something-goes-wrong) explains the supported remedies.

For the planning demo, select **Extend work** (or **Pause and extend** if running), choose **Straight tunnel**, then click an endpoint ahead of the accepted route. Six extra cells reach Underground(13,60); review the numbered path, added floor, working space, cost and spoil before confirming. Try cancelling and quoting again first. After **Add work to this machine**, resume the project, start both trains if stopped, and unpause the game. The default50-cell demo already includes this extension and ends near the map boundary.

## The things I am particularly keen for you to see

- **The full spoil loop.** Watch the cutter fill its actual Earth/Rock wagons, retreat, exchange with the hauler, and resume. Then follow the hauler to the Surface receivers and back with empties. The moving cargo and wagons are the actual game objects, and disposal costs money.
- **Excavation that earns its floor.** Compare solid rock, cut faces and the open cavity. Ordinary rail should refuse solid terrain and become possible where the TBM has paid to excavate. Temporary support stays protected while the machine still needs it.
- **A machine that has to fit.** The default demo follows its accepted straight route and backs along support for exchange. Use `underground-mines-legacy` to inspect the retained bends. A right-angle change needs advancing bends and enough space; the machine cannot simply spin on one tile to reach any requested area.
- **A useful excavation preview.** Once you have an eligible sealed portal for another project, open **Underground Construction -> New area**, click that layer's portal and drag a floor area. Inspect the requested floor, extra turning clearance, uncovered cells, spoil, support and cost estimates before confirming. Previewing alone should spend nothing. A portal that already has a project directs you to **Projects**; select **Pause and extend**, drag additional floor beyond its accepted route, review the machine and quote, then **Add work to this machine**. The work-intent dropdown also offers **Straight tunnel**: click an endpoint directly ahead of the accepted route. Numbered map labels show successive cutter positions; blue cells show charged extra working space. Review the quoted cutting time and spoil before confirming. Resume in the portal controls. Escape cancels the draft and leaves the machine paused; it does not discard previous work or buy another TBM.
- **Readable geology.** Open **Underground Construction -> Survey** and compare the displayed Soft, Mixed, Hard or Basalt hardness with the floor and rock-face texture. New maps have saved material regions; your old demo retains its original geological difficulty. Mineral markers and reserves remain separate. Surveying spends nothing and produces no cargo.
- **Recovery you can understand.** Try pausing/resuming the project and saving/reloading your personal copy during a wait. The cutter, loaded wagons, open floor and progress should agree with what you saved. I would especially like to know whether the **Next:** text helps you understand what to do.

The straight-corridor shortcut in the portal window accepts **2 to 60 cells**. The area tool handles larger reachable plans, subject to map space and the planner's limits. A large rectangle is a request to plan, not a promise that the machinery can safely clear every tile.

Cutting takes twice the former active time. Each newly cleared cell produces whole tonnes of Earth/Rock: Soft **3/6**, Mixed **2/12**, Hard **1/20**, Basalt **1/24**. Basalt keeps its minimum one tonne of Earth. Travel speed and support/disposal prices are unchanged; haulage and other waits still affect the time you observe.

## Return, keep the machine and build the far exit

**Pause** holds work in place. **Return and park** preserves it for later. **Choose hangar** selects compatible owned storage, including another eligible portal; the same native train travels there after any necessary spoil delivery. Ordinary service orders safely interrupt cutting and retain progress. **Finish/cancel and release** ends the project after the complete machine and logistics are safely clear. Read its blocking reason if the action is unavailable.

To make an A-to-B connection, finish the accepted terminal excavation, then back the whole cutter a few cells clear of B's intended footprint and stop it. On Underground, open rail portal construction, select Surface and review both endpoints, orientation, approach and price. The reciprocal Surface-to-Underground placement works too. The command permits only the safe, vacated terminal handover; it cannot remove occupied support or excavate free rock.

In **tbm-exit**, the excavation and local withdrawal are already done. The exit site is Underground(13,60), at the completed tunnel end; A is at(13,10). Leave the cutter stopped while placing B, and choose the mouth orientation that joins the retained approach. The picker previews both endpoint footprints and explains a wrong orientation. Then follow the remaining steps below and unpause for actual train travel.

Build B, use **Connect** when its real approaches permit it, then choose B as the TBM's hangar and **Return and park**. Let its actual hauler settle remaining spoil through the assigned origin. Once the whole machine is stored and the release guidance allows it, **Finish/cancel and release**, pay to **Make track permanent** where needed, and **Connect** A. Ordinary trains can then use the tunnel while the TBM remains stored at B. Neither selling it nor purchasing a replacement is part of this sequence.

## Try an underground mine

For a first attempt, Coal or Iron Ore gives you a familiar Surface receiver. Copper Ore, Bauxite and Salt are also Underground resources. Deep Underground adds Lithium Ore, Nickel Ore, Cobalt Ore, Rare Earth concentrate and Uranium Ore after its date gate.

1. **Survey a matching deposit.** Decide which resource you want to mine, and find its seam on the intended layer.
2. **Excavate a real site.** A mine needs its **3 x 3 machinery footprint** plus an adjoining **2 x 2 loading approach** already excavated. A narrow railway corridor is not automatically wide enough. Finish or use an appropriate TBM project, withdraw safely, and release/remove temporary support from the machinery cells so they are clear, flat floor.
3. **Fund the matching mine.** In Underground Construction, choose **Fund a mine**, select the resource's mine and click its matching deposit. Funding rechecks the whole site, seam contact, claim, date and funds. A refusal should tell you what prevents placement; the mine cannot dig its own foundation for free.
4. **Give it a freight route.** Build an ordinary station in the mine's catchment and compatible track/depot on excavated ground. Buy conventional freight equipment compatible with that cargo, with orders to collect at the mine and unload at its Surface receiver. Mineral wagons carry compatible ores; hazardous Uranium needs a compatible sealed carrier.
5. **Connect the layers safely.** An active TBM work pocket is reserved for its operation. Use **Return and park**, settle spoil, then **Finish/cancel and release**. Pay for necessary support conversion and **Connect** the portal. Keep the same machine for reuse. The [full how-to](r2mlvs-how-to.md#9-finish-and-open-the-tunnel) has the sequence.
6. **Watch extraction and delivery.** Look for actual mine output, station cargo and vehicle loading. The geological reserve should decrease as ore is extracted. Save and reload your copy to check that reserve, cargo and the route remain consistent.

On Surface, the material chain includes the original Steel receiver plus metal refinery, chemical, battery and electronics works. Mines require matching resources and unlocked dates; Surface processors require transported inputs. There is still selected industry/freight art to finish, so some new facilities or vehicles can reuse existing graphics at this checkpoint.

## What to tell me afterwards

The most useful feedback is concrete: what you clicked, what happened and what you expected. Please include your saved copy's name if something gets stuck.

- Could you find the demo, cutter, portal and correct layer without hunting?
- Did the excavation and exchange loop run, and were waiting reasons understandable?
- Did the terrain, wagons, cutter motion and sounds communicate what was happening?
- Was the area preview useful enough to judge space and cost before commissioning?
- Could you survey and fund a mine on a properly cleared site, then move its ore with conventional freight?
- Did pause/resume and reload preserve the state you expected?
- Are the cutting pace and rock textures readable, and can you build B, park the same machine and release the tunnel without selling it?

## Tubes and geothermal at this checkpoint

Tube and geothermal work have separate delivery and acceptance records in [the master plan](r2mlvs3mp-master-plan.md), which remains paused during TBM finalization. Use those records for their current status. This walkthrough and its retained checkpoints cover excavation, spoil haulage and conventional public rail.
