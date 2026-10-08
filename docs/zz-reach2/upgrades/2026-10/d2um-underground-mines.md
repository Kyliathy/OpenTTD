# Underground mines demo

Player walkthrough for the delivered [portal/TBM master](r2mlvs3mp-master-plan.md) and its [demo checkpoint](r2mlvs3mp-z-l-demo-checkpoint.md). Architecture: [map layers](../../architecture/map-layers/archi-map-layers.md). Updated 2026-10-08.

This demo starts with an established TBM project and spoil-haulage logistics. It is a good place to watch the machinery work, explore the geology and try building a mine. The starting save does not supply a completed mineral-freight business; that is something you can build and test yourself.

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

Both choices use `build/play.cfg` and load the prepared [underground-demo-390.sav](../../../../build/save/underground-demo-390.sav). If that generated copy is missing, the launcher uses the identical [repository snapshot](../../../../src/tests/fixtures/haulage/underground-demo-390.sav), so the demo remains available with the checkpoint commit/tag. Your own saves go into `build/save` and appear in the launcher. If neither demo copy is available, the launcher reports it explicitly.

The game starts **paused**. Before experimenting, use **Save game** to make your own copy, for example `underground-mines-my-run`. This preserves the original demo for another tour. Existing saves can be reopened by name: `.\play.ps1 underground-mines-my-run`.

## Get your bearings

1. Use the main toolbar's **layer selector**, or **N**, to switch between Surface and Underground. Surface has the hauling infrastructure; Underground has the excavation. Deep Underground is a separate layer, with access gated until **1 January 1980** in the game calendar.
2. Open the **train list**, find the TBM and use its vehicle window's **centre view** control. If it is at the Surface portal exchanging wagons, that is part of its normal work cycle.
3. Click its **rail layer portal**. The portal window shows the project, **Next:** guidance, waits and controls. **Machine** opens the cutter's vehicle window; use that to follow its progress.
4. Unpause the game and watch a complete cycle. You can pause the whole game to inspect it, or use the project's **Pause / Resume** controls to hold the excavation while the rest of the world runs.

While viewing Underground, the main toolbar button that opens airport construction on Surface becomes **Underground Construction**. Hover over it to check the tooltip, then open its menu. This is where you find **Survey geology**, **Plan an excavation area** and **Fund a mine**.

The prepared checkpoint is at a bent-route spoil-exchange stage. You are joining an existing operation rather than commissioning a fresh machine. If it waits, read the portal's reason before changing the layout; the recovery table in [the full how-to](r2mlvs-how-to.md#if-something-goes-wrong) explains the supported remedies.

## The things I am particularly keen for you to see

- **The full spoil loop.** Watch the cutter fill its actual Earth/Rock wagons, retreat, exchange with the hauler, and resume. Then follow the hauler to the Surface receivers and back with empties. The moving cargo and wagons are the actual game objects, and disposal costs money.
- **Excavation that earns its floor.** Compare solid rock, cut faces and the open cavity. Ordinary rail should refuse solid terrain and become possible where the TBM has paid to excavate. Temporary support stays protected while the machine still needs it.
- **A machine that has to fit.** Watch how the cutter and trailing equipment take a bend and back along their support. A right-angle change needs advancing bends and enough space; the machine cannot simply spin on one tile to reach any requested area.
- **A useful excavation preview.** Once you have an eligible sealed portal for another project, open **Underground Construction -> Plan an excavation area**, click that layer's portal and drag a floor area. Inspect the requested floor, extra turning clearance, uncovered cells, spoil, support and cost estimates before confirming. Previewing alone should spend nothing.
- **Readable geology.** Open **Underground Construction -> Survey geology** and click around the rock. Check whether the resource markers, mineral names and remaining reserves make it clear where you might want to dig next. Looking at a seam should not excavate it or produce cargo.
- **Recovery you can understand.** Try pausing/resuming the project and saving/reloading your personal copy during a wait. The cutter, loaded wagons, open floor and progress should agree with what you saved. I would especially like to know whether the **Next:** text helps you understand what to do.

The straight-corridor shortcut in the portal window accepts **2 to 60 cells**. The area tool handles larger reachable plans, subject to map space and the planner's limits. A large rectangle is a request to plan, not a promise that the machinery can safely clear every tile.

## Try an underground mine

For a first attempt, Coal or Iron Ore gives you a familiar Surface receiver. Copper Ore, Bauxite and Salt are also Underground resources. Deep Underground adds Lithium Ore, Nickel Ore, Cobalt Ore, Rare Earth concentrate and Uranium Ore after its date gate.

1. **Survey a matching deposit.** Decide which resource you want to mine, and find its seam on the intended layer.
2. **Excavate a real site.** A mine needs its **3 x 3 machinery footprint** plus an adjoining **2 x 2 loading approach** already excavated. A narrow railway corridor is not automatically wide enough. Finish or use an appropriate TBM project, withdraw safely, and release/remove temporary support from the machinery cells so they are clear, flat floor.
3. **Fund the matching mine.** In Underground Construction, choose **Fund a mine**, select the resource's mine and click its matching deposit. Funding rechecks the whole site, seam contact, claim, date and funds. A refusal should tell you what prevents placement; the mine cannot dig its own foundation for free.
4. **Give it a freight route.** Build an ordinary station in the mine's catchment and compatible track/depot on excavated ground. Buy conventional freight equipment compatible with that cargo, with orders to collect at the mine and unload at its Surface receiver. Mineral wagons carry compatible ores; hazardous Uranium needs a compatible sealed carrier.
5. **Connect the layers safely.** An active TBM work pocket is reserved for its operation. Once that project is finished, follow the portal's finishing controls: release the hauler, make the support track permanent, stop/retire and sell the machine, then **Connect** the portal for ordinary trains. The [full how-to](r2mlvs-how-to.md#9-finish-and-open-the-tunnel) has the sequence.
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

## Tubes and geothermal at this checkpoint

The tested tube foundations include guideways, a prototype capsule and paid metered electricity. Their normal construction toolbar, complete five-model fleet, terminals, final art and useful-route acceptance remain ahead. This demo's launcher does not unlock those future tools.

Geothermal has natural vents, actual collector construction and shared Heat accounting. The connected heat network, operating powerhouse, electricity export and normal geothermal controls remain pending. Those parts will follow as we continue the master plan.
