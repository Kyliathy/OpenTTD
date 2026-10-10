# First geothermal plant and Surface export bootstrap specification

Owner: [Plan18 Group6](r2mlvs18-geothermal-energy-and-industry.md#group-6---prove-the-first-plant-and-riser-bootstrap-requirements), dispatched by [master10.3](r2mlvs3mp-master-plan.md). Runtime acceptance follows at master10.6. Architecture: [map layers](../../architecture/map-layers/archi-map-layers.md). Frozen authorities: [Group10 seed/milestone ledger](r2mlvs1-d-recipes-and-bootstrap.md#group-10---bootstrap-graph-and-bounded-external-seed), [Group19 construction terms](r2mlvs1-f-utility-and-transfer-contracts.md#group-19---direct-heat-and-early-power-bootstrap) and [current TBM pacing/spoil correction](r2mlvs1-e-ground-and-space-budgets.md#group-13---excavation-units-bounded-spoil-and-legal-heading-budgets). This document specifies the later fixture; it records no operating generator or driven first-build pass.

## First material and capital ledger

`GetMultiLayerGeothermalConstruction` remains the construction authority. Quantities below are actual transported cargo units; capital is internal Money, separate from purchased materials. Work uses synchronized simulation ticks and requires no electricity from the unfinished plant.

| First facility | STEL | RFMT | ELEC | BATY | Capital | Construction ticks | Excavated machinery floor | Paid seed bill |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | --- | ---: |
| Collector | 8 | 4 | 2 | 0 | 180,000 | 296 | 3x3 | 2,520 |
| Vent-Tap Powerhouse | 12 | 8 | 4 | 2 | 180,000 | 592 | 4x4 | 5,340 |
| First Deep-to-Underground riser | 4 | 4 | 2 | 2 | 120,000 | 296 | 2x2 at the original Connected portal | 2,820 |
| Frozen first-plant milestone | 24 | 16 | 8 | 4 | 480,000 | 1,184 | 29 disjoint facility cells, plus access and utility paths | 10,680 |
| Additional Underground-to-Surface riser for the rail route | 4 | 4 | 2 | 2 | 120,000 | 296 | Another actual 2x2 sending machine | 2,820 |
| Complete selected Surface-export route | 28 | 20 | 10 | 6 | 600,000 | 1,480 | 33 disjoint facility cells, plus access and utility paths | 13,500 |

The frozen milestone owns one riser. A rail route through Underground pays the additional actual riser separately before claiming Surface export. It also pays every intermediate electrical cable cell. The direct road-elevator electrical path is delivered, but that alone proves no construction-material carrier route through a road elevator; the selected bootstrap uses the delivered conventional rail fleet.

Surface seed terms are STEL100/unit from1950, RFMT180 from1950, ELEC500 from1965 and BATY350 from1965. Per-company economy-day caps are respectively256/128/64/64; even the selected two-riser bill fits each cap in one ordering day. Actual delivery can take several trips/days. Copying a station cannot multiply a cap. Bought company-supply packets earn no freight income or subsidies and never appear directly in Deep inventory.

The [ground producer handoff](portal-tbm/r2mlvs3mp-y-ground-producer-handoff.md) verifies these reusable Surface factories:

| Required material | Actual Surface producer | Earlier manufacturing inputs | Conventional final delivery |
| --- | --- | --- | --- |
| Steel | Compatible original Steel receiver | Iron Ore, one-to-one | Native model5 wagon with model4 locomotive |
| Refined Metals | `CFMT` metal refinery | Copper Ore and Coal | Same wagon, refitted for RFMT |
| Electronics | `ELEC` electronics works | Refined Metals and Rare Earth concentrate | Same wagon, refitted for ELEC |
| Batteries | `BATT` battery works | Lithium Ore, Cobalt Ore and Chemicals | Same wagon, refitted for BATY |

`CHEM` Surface chemical works supplies the battery chain from Salt/Oil. The actual first-milestone manufacturing fixture supplies24 Steel,16 Refined Metals,8 Electronics and4 Batteries, with48 Copper Ore/12 Coal producing24 Refined Metals before8 goes into Electronics;24 Iron Ore;4 Rare Earth concentrate;4 Lithium;2 Cobalt;2 Salt/2 Oil. Extending to the second riser requires the extra four-material bill through the same owners. A first build may instead buy the bounded seed; both branches preserve native stock and carrier custody.

Model5's catalogue introduction is1950; its Deep use unlocks with actual1980 layer/portal access. The old shorthand that the wagon itself unlocks in1980 is stale. Its64-unit selected-cargo hold supports all four bill labels. Different labels occupy separate actual loads/refits; the locomotive or an empty output account provides no extra cargo capacity. Spoil remains in its separate64-t Earth and128-t Rock wagons.

## Commissioning work and money

Start from declared cash and an actual owned Surface origin. Retain the earlier funded Surface route/Standard TBM/Deep-access participants and their actual command receipts. The frozen capital allocations are220,000 +300,000 +440,000; their TBM/deep construction seed bills are1,520 +4,640. These are planning allocations: actual native command/work receipts determine the accepted save's cash, including inflation and existing structures. Buy an additional model5 freight wagon if the retained consist has none; its frozen purchase allocation is24,000. Keep origin/exchange/receiver and empty-return work active until all produced spoil is settled.

The canonical excavation planner must quote the complete collector9-cell, powerhouse16-cell, first riser4-cell and upper riser4-cell floors, loading/approach cells, source heat/electrical cells and actual swept approach/retreat envelopes. Count the union of newly solid cells once. An already open supplied test floor cannot satisfy this fixture. Actual vents can require Basalt/Hard work, so use the accepted map's hardness/yield/cost quotation and actual Deep TBM. The post-R2TBMF values supersede older paper Earth/Rock quantities and per-cell pacing.

| Current hardness | Earth/Rock per new cell | Deep uninterrupted ticks for one cell | Support per cell |
| --- | --- | ---: | ---: |
| Soft | 3/6 | 99 | 30 |
| Mixed | 2/12 | 99 | 50 |
| Hard | 1/20 | 198 | 80 |
| Basalt | 1/24 | 395 | 120 |

Retain the actual geological-work and Q16 remainders; summing rounded single-cell times overstates combined work. Overlap creates no new spoil/work. Deep active/paused rates are960/240 per74 ticks; assigned haulage is60/day. Full-consist fit and both independent wagon capacities control each cut. Surface origin disposal costs2/Earth and3/Rock, with actual bounded receiver stock/intake and empty returns. There is no spoil freight profit funding the plant.

Worked normalization: for a planner union of60 new Soft cells, aggregate Deep work is5,920 ticks/80 economy days, yielding180 Earth/360 Rock. Active charge76,800 +support1,800 +disposal1,440 =80,040. With one64/128 wagon pair, even pure-Soft loads admit at most21 cells before Rock fills; complete swept advances may force earlier exchanges. Add actual pause/retreat/hauler costs instead of assuming three free exchanges. This normalization checks arithmetic; the eventual vent geometry replaces its cell count/hardness with the canonical quote.

For that normalization and the selected two-riser route, the first-build allocation is600,000 capital +13,500 seed +24,000 freight wagon +80,040 cutting/support/disposal =717,540, **plus1,000 for each distinct installed Heat/electrical cell**, actual pause/hauler/route expenses and recorded service work. Heat cells carry1MW; electrical cells and each paid riser carry200kW. Installation has no extra transported material bill. If two heat and two electrical cells sufficed for a hypothetical placement, the cell charge would be4,000; the real Underground relay must pay its full installed housing-to-inlet path, which typically needs more. Use the exact path, never this illustration's cell count, for acceptance.

The full ledger is therefore earlier actual receipts +600,000 +13,500 +optional24,000 +actual canonical cutting/support/disposal +1,000 per installed cell +actual transit/idle/service costs. Debit construction capital once and all material stock once; construction ticks are retained work, not a second material charge. Report cargo at factories/stations/vehicles/construction stocks, consumed materials, disposed spoil and money at every checkpoint. A shortage pauses before the next whole action; it never unlocks free floor, a hidden receiver, negative inventory or an unbilled second riser.

## Runtime acceptance to execute at master10.6

The dependency graph is `cash -> owned Surface station/paid supplies -> conventional rail fleet -> Standard access -> Deep TBM/commissioned Deep access -> paid excavation/spoil loop -> delivered collector/plant/riser construction -> allocated native Heat -> electricity -> paid riser export`. The upper Surface-export riser adds its own paid construction after conventional Underground access. The graph contains no edge from geothermal output or tube freight back to its own first construction.

Two implementation obligations remain before the actual fixture can pass: the operating generator/heat-to-electric participant under Plan18 Group8, and the missing paid Surface procurement/packet-custody command under [Plan6 Group13C](r2mlvs6-mines-and-resource-chains.md#group-13c---supply-paid-surface-bootstrap-cargo). The existing manufacturing fixture's explicit raw receipts and the export fixture's explicit electrical input are useful boundary tests; the runtime first-build proof must use actual purchased/produced stock and real electrical conversion. Master10.6 executes13C before claiming that proof.

1. Load a declared native1980 baseline with cash, owned conventional stations/fleet and original solid target geology. Retain exact source hashes. Record all prior purchases, routes and initial inventories; require zero geothermal output and no prebuilt plant floor/material bill.
2. Execute actual Surface paid orders or the complete manufacturing branch, retaining per-company caps, money, source/provenance and non-paying company-supply purpose. Drive actual model4/model5 loads through valid Surface/Underground/Deep rail portals to the actual receiver. Material station catchment cannot cross a layer by coordinates alone.
3. Quote and execute actual TBM area/access cuts with complete native participants, support and original job identity. Exchange both loaded spoil wagons, settle the Surface receivers and return empties. Verify the exact footprint union and actual hardness/time/spoil/cash against the planner and receipts, including hidden-layer operation.
4. Fund actual collector,1980 powerhouse and both required riser machines through ordinary commands. Deliver only remaining required stock through native packet owners; advance exactly the retained construction work, and preserve an unfinished save. Before each complete building is ready, reject generation/export without partial energy or inventory changes.
5. Install and charge the complete actual Deep Heat path and Deep/Underground electrical inlets. Both original portal pairs must be Connected/date-valid and all machinery/stock/work complete. Confirm unrelated Underground terrain remains solid for any separately tested direct-elevator branch.
6. Allocate one actual vent thermal budget. The1980 generator converts its granted heat at15%, retains conversion carry, settles actual heat use and publishes resulting electricity once in the common owner. At1MW thermal/74 ticks the upper bound is1,998,000 thermal J ->299,700 electric J; a200-kW riser window admits at most399,600 J, further bounded by source/demand/path. Invoke the actual feature prepare/export; preserve source identity/epoch across both paid transitions and credit only the requested final consumer.
7. Save/reload unfinished construction and operating/export states, then continue actual delivery/work/use. Verify ordinary game resaves, changed dates, hidden views, a sealed/removed edge, full destination, insufficient money/stock, cancellation/retry and source/company loss. Reconnection/repair obtains fresh references and never revives released rights or duplicates generation/expense/cargo.
8. Publish exact source/binary/input identities, selected native chunks, total money/material/heat/electric ledgers and the actual run reports. Reject closure if the operating proof still relies on fixture-only floor, delivered stock, source publication or unmatched carrier claims. Keep user-feedback and post-demo pruning gates pending.

## Independent tube bootstrap

The delivered `ConnectTubeExternalPower` / `PublishTubeExternalPower` service remains independently buildable from1970:100kW,20/day lease,1 Money per1,000 supplied J with saved0..999 invoice carry. A20-kW/74-tick capsule uses39,960 J; its reference first-day allowance is60. Geothermal is absent from this dependency chain. Later substitution selects a genuinely connected original electricity producer and retains the same tube meter/physical demand/carry; it never creates a second grant, waives a lease that still exists or makes the initial tube depend on Deep material delivery.

## Specification verification

Current `multilayer_bootstrap.cpp/.h`, vehicle catalogue, actual Surface industry definitions/manufacturing fixture, native riser delivery and tube power exports were checked. Focused `[multilayer-bootstrap],[layer-surface-geothermal-supply]` passes356 assertions/two cases in the optimized acceptance binary; [evidence](../../../../logs/geothermal/power-export/20261010-export-01/bootstrap-producers-optimized-1.log). Existing Group5 both-configuration/native results verify the independent physical/export owner. This closes the Group6 specification; actual paid procurement, operating generation and driven first-build acceptance remain pending under their named owners.
