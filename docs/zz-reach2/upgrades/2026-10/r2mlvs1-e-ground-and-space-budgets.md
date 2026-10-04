# R2MLVS1 execution record - ground and Space budgets

Owner: [r2mlvs1-gameplay-contracts-and-content.md](r2mlvs1-gameplay-contracts-and-content.md), Groups 13-14. Architecture: [archi-map-layers.md](../../architecture/map-layers/archi-map-layers.md) and [archi-cargo-payment.md](../../architecture/economy/archi-cargo-payment.md). Inputs: [recipes, bootstrap and capability contract](r2mlvs1-d-recipes-and-bootstrap.md). These are initial tuning numbers and worked accounting examples; feature owners and Plan 15 still owe runtime geometry, transport, art and balance acceptance.

## Group 13 - excavation units, bounded spoil and legal heading budgets

Money below means the game's internal pre-inflation unit. One economy day is 74 synchronized ticks; calendar years only gate technology. Charge rates accrue as `rate * elapsed_ticks / 74`, retaining the numerator remainder 0..73 in the project so splitting a day cannot erase upkeep. Check products/sums in wide integers before commit. Work duration is per newly solid **cell**, with any swept-envelope cells included once; already excavated cells produce no spoil and charge only actual transit/idle time. Active and paused-project charges are separate from per-cell support and disposal. Idle charge applies to a commissioned active project waiting for haulage/materials, not an unassigned purchased cutter; conventional vehicle upkeep remains separately visible.

| Model / item | Purchase | Capacity / work or recurring charge |
| --- | --- | --- |
| Standard TBM | 200,000 | 480/day actively cutting; 120/day paused/returning; cannot work Deep/basalt |
| Deep TBM | 320,000 | 960/day actively cutting; 240/day paused/returning; Deep from 1980 |
| Earth hopper | 20,000 | 64 t ERTH only |
| Rock hopper | 20,000 | 128 t ROCK only |
| Conventional spoil hauler | 60,000 | Actual coupled wagons; 60/day active or assigned idle; construction-track effective 5 tiles/day |
| Mineral/heavy wagon | 24,000 | 64 t of the selected solid refit; GOODS uses its established crate/weight conversion |
| Sealed mineral wagon | 30,000 | 32 t sealed URAN/RARE |
| Mine bulk truck | 25,000 | 24 t selected bulk cargo |
| Sealed mineral carrier | 35,000 | 16 t sealed URAN/RARE |

TBM uses the same 64/128 t spoil wagons on both rock layers; it can couple additional explicit wagons rather than receive an invisible hold upgrade. A cut/advancing segment refuses unless **all** its resulting Earth/Rock fit the attached permitted wagons. Earth/Rock capacities are independent; free Earth space cannot absorb Rock. One shared set of two wagons is assumed in the worked scenarios, so the TBM waits for their actual empty return. Spare sets can improve utilization only through paid, real wagons and atomic exchange.

| Hardness | Earth / Rock per newly cleared cell | Standard work days | Deep work days | Support/cell |
| --- | --- | --- | --- | --- |
| Soft | 6 t / 12 t | 1 | 1 | 30 |
| Mixed | 4 / 24 | 2 | 1 | 50 |
| Hard | 2 / 40 | 3 | 2 | 80 |
| Deep basalt | 1 / 48 | refuse | 4 | 120 |

Support covers temporary construction rail/lining and is debited once with the cut, independently of upkeep. Receiver caps remain Earth 1,024 t, Rock 1,024 t, internal aggregate 1,024 t. Earth intake is 32 t/day, disposal 2/t; Rock intake 64 t/day shared between crushing and surplus disposal, charged 3/t crushing or 4/t surplus. Make the Group 7 exception explicit: Earth processes at most **four** 8-t batches/day; Rock at most **eight**, sharing one 64-t allowance across alternatives. Full stock/aggregate or an absent origin station pauses further work and preserves loaded wagons. Fees are charged on admitted intake; incomplete 8-t batches remain in receiver stock. No Earth/Rock transport subsidy or freight income exists.

For the paper geometry use a conservative 3x3 cell cutter safety envelope around each advanced center, starting with a cleared 3x3 commissioning area. This is a budgeting envelope awaiting Plan 4 swept-consist and Plan 13 sprite geometry acceptance, not permission to ignore actual corners/occupied cells. Every newly solid envelope cell is charged. A segment which would exceed wagon space waits for the real wagon cycle before advancing; unused hopper capacity does not permit half an advancing segment. Loaded/empty travel is rounded **up** to whole economy days for these small example budgets; runtime timers retain tick remainders.

Straight corridor: advance centers `(1,0)..(10,0)`, always positive map-X, which is engine viewport direction **SW**. Each step exposes three new cells: 30 soft cells, Earth 180 t and Rock 360 t, 30 active days. Atomic three-cell advances fit at most three per wagon load: new-cell loads `9,9,9,3`, giving Earth `54,54,54,18` and Rock `108,108,108,36`. Cutter ends at distance `3,6,9,10` from exchange bay; backing to the bay and returning at 5 tiles/day plus one exchange day costs `3,5,5,5` idle days. The Surface origin is 10 rail tiles from the bay: two days loaded, two empty, receiver intake two days for each of the first three loads and one for the last. Thus Surface cycles cost `6,6,6,5` days, total TBM/assigned-hauler idle 41 days. Empty delivery and the cutter's supported retreat are both included.

| Straight ledger | Money |
| --- | --- |
| Active cutter, 30 x 480 | 14,400 |
| Paused/return cutter, 41 x 120 | 4,920 |
| Per-cell support, 30 x 30 | 900 |
| Assigned hauler, 41 x 60 | 2,460 |
| Earth 180 x 2 + Rock 360 x 3 intake | 1,440 |
| **Cash operating debit** | **24,120** |
| Maximum aggregate replacement value, 270 x 1 | at most 270 in kind |
| **Loss even after full aggregate value** | **at least 23,850** |

Start with 1,000,000. Origin/portal/route 220,000 + cutter 200,000 + two hoppers 40,000 + hauler 60,000 + TBM construction supplies 1,520 leaves 478,480 before work; after the corridor debit, **454,360** cash remains. Earth 176 t is disposed and 4 t retained in stock. Rock 360 becomes 270 aggregate + 90 recorded residue; aggregate is useful only in bounded local construction inventory, not saleable unrestricted Goods. Receiver/aggregate stock stays within every cap.

Turning chamber: centers `(0,0),(1,0),(2,0),(3,-1),(3,-2),(3,-3),(2,-4),(1,-4),(0,-4),(-1,-3),(-1,-2),(0,-1),(1,-1)`. Engine viewport headings are `SW,SW,W,NW,NW,N,NE,NE,E,SE,S,SW`; each successive change is 0 or 45 degrees, every bend advances, and the last segment moves through an already cleared envelope. There is no on-tile pivot or single 90-degree cut. The swept union has 44 cells, nine initially cleared, so **35 new cells**; per-advance new counts are `3,3,5,3,3,5,3,3,5,1,1,0`. Whole-segment loads must be `6,8,8,6,7` new cells rather than pretending four perfectly full loads: Earth `36,48,48,36,42`, Rock `72,96,96,72,84`.

Load endpoints are at Manhattan bay distance `2,5,6,4,1`; conservative supported retreat/return and exchange cost `3,3,5,3,3` days. Surface cycles each need two loaded + two empty + two receiver days, adding 30 days. TBM waits/returns 47 days, plus one idle transit day for the final already-cleared advance: 48; hauler assignment is 47 days. Work 35 x 480 = 16,800, support 1,050, cutter idle 5,760, hauler 2,820, spoil intake 1,680: **28,110** cash debit. Even crediting the maximum possible 315 aggregate value leaves **27,795 loss**. Actual processing leaves Earth 208 disposed + 2 stock; Rock 312 aggregate + 104 residue + 4 stock. No overlapping cell is excavated/paid twice. Live swept-consist track feasibility is an explicit Plan 4/22 check still outstanding.

The loss is structural, not just this travel schedule: useful aggregate value is at most `0.75 * admitted Rock`, while Rock intake alone costs at least `3 * Rock` and Earth costs `2 * Earth`, before positive support/work/haulage. Spoil-only digging therefore cannot profit even with faster return travel. All ten ore routes use the conventional cargo carriers/dates from Group 12; hazardous Uranium never relies on a Bulk hopper or a 1985-only capsule. These hand budgets were independently recalculated; no new runtime implementation or regression test is claimed.

## Group 14 - launch, returns and Surface-supplied orbital assembly

| Vehicle/facility quantity | Initial value / bound |
| --- | --- |
| Launch rocket payload / fuel hold | 30 t solid payload / 40 kL PROP; fuel mass/hold is separate from payload |
| Surface-to-Space / return fuel | 10 kL / 5 kL; reserve the whole round trip before departure |
| Pad preparation / turnaround | 148 / 74 economy ticks; one mission occupies one pad |
| Outbound / return flight | 222 / 222 ticks, saved phase/timer; no camera-dependent work |
| Rocket recurring mission fee | 5,000 per round trip, independent of fuel purchase |
| Active pad upkeep | 500/economy day during preparation/turnaround |
| Probe hull / hold / starting pods | 8 t / 20 t NORE/RARE/WICE / two expendable pods |
| Probe collection interval / cost | at most 2 t per 74 work ticks, limited by finite body reserve and hold; 500 active/day, 100 assigned idle/day |
| Probe service | After two complete return batches, 148 ticks and RFMT 1 + ELEC 1; visible pause before a third batch if absent |
| Return pod payload / shell | 20 t selected asteroid cargo / 1 t shell, SCMP 1 + 2,000 cash + 74-tick Surface assembly |
| Pod descent | 148 saved economy ticks; shell is expended only when the preserved load reaches a valid Surface owner |
| Surface hub handling | 1 Money/tonne actually moved; 8 cargo units/tick shared across jobs |
| Launch service stock caps | PROP 128 kL, RFMT 64 t, ELEC 32 t, SCMP 64 t, eight assembled pod shells |
| Probe service stock caps | RFMT 8 t, ELEC 4 t, two pod shells; separate from its 20-t freight hold |

First fuel is Surface `PCHM`: eight batches consume CHEM 16 kL to make PROP 16 kL, using 592 work ticks across two max-four-batch evaluations. Chemicals can be made from SALT 16 t + OIL_ 16 kL, or bought and delivered through the bounded Surface seed. Purchasing PROP 16 costs 4,000; launch/empty rocket return consume 15 and retain 1 kL. No asteroid ice, orbital refinery or Deep heat is a prerequisite.

First probe body consumes SCMP 2 + RFMT 4 + ELEC 2 = 8 t. Two pod shells use SCMP 2 = 2 t; service preload RFMT 1 + ELEC 1 adds 2 t. Thus first payload is **12/30 t**, with no fuel hidden in a container refit. These purchases cost 4,800; body cash is 100,000 and shell assembly 4,000, so Group 10 was reconciled to a 104,000 capital row and SCMP 4/RFMT 5/ELEC 3 input row. Empty rocket returns to its Surface pad while the probe remains in Space.

The probe collects 20 t in ten complete 2-t batches, reserves/deploys one pod, then collects/returns another 20 t. Hold bounds are respected and the body loses exactly 40 t finite reserve; packet source/age/owner are preserved through descent. Two 20-t Surface jobs cost 40 total handling units, without final payment until an actual useful receiver admits cargo. Twenty active collection days cost 10,000. Preparation/turnaround costs 1,500; rocket mission fee 5,000. The initial operating purchase/debit ledger, excluding center/rocket/probe body capital and their body construction materials, is fuel 4,000 + pod material 1,200 + pod assembly 4,000 + service preload 680 + probe work 10,000 + mission 5,000 + pad 1,500 + handling 40 = **26,420**. Fuel 1 kL remains; service preload is consumed after the second batch. Pods are gone; before a third return, new Surface-made shells must be launched, and depleted service/pod/hold state visibly pauses work. This is a cost ledger, not a promised freight profit.

The first mirror uses SMIK 4 + RFMT 2 = **6/30 t** payload and a 148-tick deployment. Its independent Surface solar target uses STEL 8 + ELEC 4. Orbital assembly 40,000 and Surface farm 80,000 match the 120,000 bootstrap capital row. The mirror links to one eligible Surface solar generator through the shared assignment contract; this ledger applies no bonus itself.

| Station module / model, from 1995 | Construction inputs | Work ticks / cash | Inventory/function bound |
| --- | --- | --- | --- |
| Nucleus/bootstrap kit | SCMP 16 + RFMT 8 + ELEC 4 = 28 t | 740 Surface preassembly + 74 deployment / 700,000 | Basic anchor, docking, one assembly job, 30 kW independent panel; no earlier heavy robot required |
| Dock extension | SCMP 8 + RFMT 4 + ELEC 2 | 444 / 160,000 | Two additional docking reservations |
| Storage module | SCMP 8 + RFMT 2 + ELEC 2 | 296 / 120,000 | 1,024 per declared cargo label; every label still has one physical form |
| Workshop | SCMP 8 + RFMT 4 + ELEC 4 | 592 / 180,000 | Four bounded service/assembly jobs; no mandatory first-nucleus dependency |
| Orbital processor | SCMP 10 + RFMT 4 + ELEC 4 | 740 / 220,000 | Group 9 PICE/OREF choice, input/output caps 1,024 per label |
| Power module | SCMP 6 + RFMT 4 + ELEC 4 + SMIK 2 | 444 / 140,000 | Additional 60 kW Space-local panel; never a mirrored Surface farm |
| Fuel store | SCMP 6 + RFMT 4 + ELEC 2 | 296 / 100,000 | PROP 128 kL; may start empty, not a bootstrap prerequisite |
| Heavy robot | SCMP 6 + RBPT 4 + ELEC 2 = 12 t | 592 / 200,000 | One station-assembled model; no freight hold |

Nucleus is a preassembled **saved payload object**, not a new 28-t cargo label; its inputs are consumed once on Surface assembly and its 28-t resulting payload fits one launch. Deployment cannot re-consume the same input packets. Heavy robot parts fit a second 12-t launch and are assembled after the nucleus exists; Group 9 expands their Surface manufacturing. With 750 J/work tick and the nucleus's 30 kW panel (810 J/economy tick at fixed simulation duration), full-rate assembly takes 592 ticks and 444,000 J; no geothermal link, mirror or asteroid material is needed. Other consumers can lengthen this work through the single allocation owner.

Station service inventory is bounded RFMT 64, ELEC 32, BATY 32, PROP 128, SCMP 64; no room means unload refusal with cargo retained. A construction job reserves every required material, checks output/module space and energy, and consumes the complete material batch only at its atomic completion. Partial work is saved in Q16 work ticks. For 750 J per full work tick, `progress = (granted_j * 65536 + remainder) / 750`, remainder 0..749 is retained; grant/progress are capped at remaining work before consuming energy. For 300 J then 450 J the increments are 26,214 and 39,322 with the carried remainder, summing exactly one full work tick. No render interval enters this calculation. Changes to per-job energy cost require an explicit schema conversion of retained remainder.

Zero fuel blocks departure before any launch debit; absent round-trip fuel cannot strand an automatically launched rocket. Full hold/product/module storage, missing pods/service parts, invalid target or lost delivery owner pauses/resolves the saved job without duplicating packets or deleting reserved inputs. The first station/robot capital is 900,000; nucleus supplies 13,040 and robot supplies 7,600, totaling **920,640**, plus two round-trip fuel/mission/pad windows of 10,500 each and their material-delivery route. Starting from the reconciled 5,000,000 bootstrap seed leaves 1,219,620 after all 11 capital/supply rows; these small work windows fit that allowance. Runtime launch state, payload serialization, ownership recovery, utility commit, art and multiplayer remain feature obligations.
