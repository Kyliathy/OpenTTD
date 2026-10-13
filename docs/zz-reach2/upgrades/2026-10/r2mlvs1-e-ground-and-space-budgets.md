# R2MLVS1 execution record - ground and Space budgets

Owner: [r2mlvs1-gameplay-contracts-and-content.md](r2mlvs1-gameplay-contracts-and-content.md), Groups 13-14. Architecture: [archi-map-layers.md](../../architecture/map-layers/archi-map-layers.md) and [archi-cargo-payment.md](../../architecture/economy/archi-cargo-payment.md). Inputs: [recipes, bootstrap and capability contract](r2mlvs1-d-recipes-and-bootstrap.md). These are initial tuning numbers and worked accounting examples; feature owners and Plan 16 still owe runtime geometry, transport, art and balance acceptance.

**Artifact-layout migration (2026-10-09):** this plan consumes the parent [Development artifact layout](r2mlvs-multilayer-vehicles-and-structures.md#development-artifact-layout), implemented by [R2FC](r2fc-file-cleanup.md). It supersedes earlier docs asset/review/helper placement and fixed runtime-config destinations below. New native homes use `mlm/runtime/`, generated feature reviews use `mlm/reviews/`, logs remain under `logs/`, and reusable code/curated inputs keep their code/test owners. Paths and focused native/cold verification are complete; R2FC records the results and retained-input identities. Repository-relative executable commands run from the checkout root through their context-aware wrappers. Historical pass records remain dated evidence and are not new acceptance claims. No gameplay task is completed by this migration and the separate post-demo fixture-pruning gate remains.

## Group 13 - excavation units, bounded spoil and legal heading budgets

**Current runtime correction,2026-10-09 (R2TBMF5/5B):** the paper budgets below are retained historical assumptions, not current cutter tuning. Standard earns1/2 and Deep3/4 geological work ticks per simulation tick; purchase/upkeep and travel retain their runtime owners. Current whole-tonne Earth/Rock yields and uninterrupted per-cell simulation ticks are:

| Hardness | Earth / Rock | Standard ticks | Deep ticks | Support/cell |
| --- | --- | --- | --- | --- |
| Soft | 3 / 6 | 148 | 99 | 30 |
| Mixed | 2 / 12 | 296 | 99 | 50 |
| Hard | 1 / 20 | 444 | 198 | 80 |
| Basalt | 1 / 24 | refuse | 395 | 120 |

Tick counts assume no pre-existing work remainder and round up only after accumulating the complete work amount; do not sum rounded per-cell times for a multi-cell cut. New whole-unit Basalt Earth is deliberately1, with no fractional cargo or carry. A 30-new-Soft-cell corridor therefore yields90 Earth/180 Rock and4,440 Standard active ticks; a35-cell chamber yields105/210 and5,180 active ticks. Overlap yields zero. Travel, exchange waits, actual full-consist fit and disposal determine complete project time/cost separately. [J](tbm-fixes/r2tbmf-j-pacing-notes.md) proves the rate migration, and [L](tbm-fixes/r2tbmf-l-spoil-notes.md) records actual combined fifty-cell haulage. The older money/yield ledgers below must not be used to retune runtime prices.

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

## Space economy revision - 2026-10-12

The user's [revised common contract](r2mlvs1-d-recipes-and-bootstrap.md#space-economy-revision---2026-10-12)
supersedes Group14's old Space body/module/pod material rows and preassembled
nucleus. These are initial revised tuning defaults, pending compiled content
freeze and real-route acceptance. Keep Group14's rocket fuel/phase/fee,
20-tonne mining hold,2-tonne/74-tick extraction, service and descent schedules.
The old26,420 operating ledger and11-milestone bootstrap total describe the
previous recipes; do not cite them as proof of this revised economy.

`STEL` is tonnes; `GOOD` is crates, not tonnes. With the current cargo weights,
one Steel unit weighs1 tonne and one Goods crate0.5 tonne. The authoritative
weight calculation uses `CargoSpec::weight` in sixteenths of a tonne, summed
before division. Refits and mixed rocket manifests must obey both packet-count
and mass bounds. Revised bounded Surface seed: GOOD from1950,120 money/crate,
256 crates/company/economy day, with the existing non-paying supply purpose.
Ordinary Earth-produced Goods remains the production alternative.

| New Space asset | Materials consumed once | Embodied mass / work / capital |
| --- | --- | --- |
| Cargo rocket hull | STEL12 + GOOD16 | 20t dry hull, separate from30t payload / assembly schedule /400,000 |
| Mining probe15 | STEL4 + GOOD8 | 8t hull / existing launch assembly schedule /100,000 |
| Survey probe19 | STEL2 + GOOD4 | 4t hull / same assembly owner /60,000 |
| Construction probe20 | STEL4 + GOOD8 | 8t hull / same assembly owner /100,000 |
| Return pod shell | STEL1 + GOOD2 | 2t shell /74 ticks /2,000 |
| Orbital mirror | STEL4 + GOOD4 | 6t /148 deployment ticks /40,000; Surface target unchanged |
| First Core,1995 | STEL16 + GOOD24 | 28t delivered /740 probe construction +74 activation ticks /700,000 |
| Dock extension | STEL8 + GOOD12 | 14t /444 ticks /160,000 |
| Storage module | STEL8 + GOOD8 | 12t /296 ticks /120,000 |
| Assembly/workshop | STEL8 + GOOD16 | 16t /592 ticks /180,000 |
| Steelworks or Goodsworks module | STEL10 + GOOD16 | 18t /740 ticks /220,000 each |
| Power module | STEL6 + GOOD20 | 16t /444 ticks /140,000; existing60kW local output |
| Fuel store | STEL6 + GOOD12 | 12t /296 ticks /100,000; existing128kL capacity |
| Later heavy robot16 | STEL6 + GOOD12 + RBPT4 + ELEC2 | 18t /592 ticks /200,000 |

Unlisted later orbital assets, including haulers and advanced processors, must
receive both base materials plus any declared advanced inputs before purchase
is enabled. Existing old SCMP-only rows are not an alternate free route.

The rocket's base materials are embodied at Earth assembly too. Its STEL12
share comes from the existing combined center/rocket STEL24 row; add GOOD16
to that row, retaining center-side RFMT16/SCMP8/ELEC8 and the separate PROP16
fuel purchase. The remainder of the Steel belongs to the Surface center.
Do not charge rocket hull mass against its freight allowance or charge its
capital twice. Initial capital split:500,000 for the native Surface launch
facility and400,000 for the reusable rocket; the combined total remains900,000.
The native facility command charges its500,000 plus clearing costs. That empty
facility supplies no free stock or commissioned rocket; material/launch gates
remain pending their own producers and Group12B freeze.

Construction probe hold:20t, one selected STEL or GOOD cargo at a time (20 Steel
units or40 current Goods crates). The body is separate from this freight hold.
Two Earth launches can supply the first Core: one construction probe with16t
Steel, and a second with24 Goods crates(12t). Including a2t RFMT/ELEC service
preload, their rocket manifests are26/30t and22/30t. Both vehicles persist for
later work. Neither delivery requires an existing orbital dock or storage;
the blueprint construction job receives the materials. Later a delivered
resupply can let a single probe make multiple trips through the public transfer
owner. A blueprint cannot process cargo or build probes before completion.

The first mining payload becomes8t body + two2t shells +2t service =14/30t.
Inputs are STEL6 + GOOD12 + RFMT1 + ELEC1, costing2,720 at the published seed
prices; capital remains104,000 including two pod assemblies. Replacing the old
1,200 pod-material term with680 gives a revised two-return operating debit of
**25,900**, before travel/wait upkeep, body material and center/rocket capital.
Probe service remains RFMT1 + ELEC1 after two returns, not free Steel/Goods.

Survey scan:148 synchronized working ticks inside the0.25-tile approach
tolerance, following the moving body without an exclusive mining lease;500
money per working day and100 while travelling/waiting, with saved remainders.
The company receives knowledge only on completion. Mining may precede survey.

| Specialized processor /1995 | All inputs per batch | Output and disposition | Work / power / interval maximum |
| --- | --- | --- | --- |
| Orbital Steelworks | NORE4t | STEL2t;2t recorded process residue | 74 ticks /55,500J /4 batches |
| Orbital Goodsworks | STEL2t + RARE1t | GOOD4 crates(2t);1t recorded process residue | 74 ticks /55,500J /4 batches |

These are game conversion rules. Per-label input/output caps are1,024 native
units; no free output, reserve replenishment or stock overflow. An ore-to-Goods
worked path consumes NORE4 + RARE1, passes its2t Steel through exactly one
inventory, produces4 crates and records3t total residue. Reserve debit occurs
only at mining; processor input consumption must not debit the asteroid again.
Each batch requires the shared utility grant, all inputs and complete output
headroom before committing. Missing either Goodsworks input or one free crate
short of its four-crate output retains every input. Products can go to an
orbital build or Earth drop, with one exclusive quantity reservation.

Freeze must add the stable recipe identities, expand all launch/seed/bootstrap
budgets for the extra probe roles and real material routes, and verify the
unchanged original ground capital separately. Preserve saved old builds and
paid packets with an explicit version migration; no retroactive material bill,
free survey, manufactured stock or completed blueprint is permitted.

### Group12B numerical reconciliation

Compiled revision2 lives in `multilayer_space_content.h`. Hull preparation is
148 ticks. The first mining manifest is14t, with2,720 material cost,104,000
body/pod capital and the unchanged25,900 two-return operating debit above.
Construction manifests remain26t and22t, including8t builder bodies and2t
service preloads. The Core's28t of loose inputs is delivered once through the
blueprint job; its capital700,000 and740 work plus74 activation ticks are
separate from both reusable builders'200,000 and296 assembly ticks.

For the two Core flights, preparation/flight/return/turnaround sum to
`2 * (148 + 222 + 222 + 74) = 1,332` rocket ticks. A sequential import-to-Core
budget adds296 builder-assembly and814 construction/activation ticks, giving
2,442 ticks excluding freight travel/waiting and the rocket's initial hull
assembly. The two rocket round trips use30kL,10,000 mission fee and3,000 active
pad cost. Independently starting with16kL needs14 more; the cumulative graph
has already spent15 on mining and therefore buys29 more for these flights.

An ore-to-Goods batch path consumes NORE4t and RARE1t. `OSTL` uses74 work ticks
and55,500J, passes exactly2t Steel to `OGDS`, which uses another74/55,500J,
produces4 crates(2t) and records1t residue. Steelworks records the other2t
residue. Total input5t equals product2t plus residue3t, with111,000J granted
through the shared utility owner. Reuse reserves those four crates once for a
construction recipe; a drop instead transfers their existing packets through
descent/hub owners. Neither choice debits a reserve or pays a delivery again.
The runtime reservation, production and provenance gates remain pending.

A complete reuse/drop branch starts with NORE4t, RARE1t and separately imported
STEL4t:9t total. The two batches above leave STEL4t + GOOD4 crates(2t), with3t
recorded residue. Reserving those four crates for a mirror consumes both stocks
once into6t of hardware,148 deployment ticks and40,000 capital;6+3=9t. Its
Surface farm remains a separate build. Alternatively, retain the4t Steel in
orbit and drop only the four Goods crates as a2t payload. A separately paid
2t shell costs STEL1/GOOD2(340 material),2,000 assembly and74 ticks; its148-tick
descent expends that shell only on valid receipt. Including the shell, initial
11t equals4t orbital Steel +2t Surface Goods +3t residue +2t spent shell.
Handling those four crates costs2, with no per-crate rounding. A single exclusive
reservation prevents both consuming the same Goods in a mirror and dropping it;
processing, reuse, launch and handling award no extra delivery income. These
worked alternatives constrain the still-pending physical producers.

Original ground capital remains1,680,000. The full revised14-row graph totals
4,144,000 capital plus91,030 supplies, including105kL for seven flights and the
separate Surface mirror target. The5,000,000 seed leaves764,970 before recurring
operations, or719,470 after seven mission/pad windows. Shared service/stock/panel
and750J-per-station-work-tick defaults are exported beside the recipes; no
onboard construction probe needs a station's electricity to build the first Core.

## Group 14 - launch, returns and Surface-supplied orbital assembly

**Historical version1 rows below:** the Space construction materials, pod mass,
preassembled Core and aggregate cash totals are superseded by the revision
above. Unchanged flight, service, handling and utility ownership rules still
apply. Existing tests of these old definitions are retained compatibility
evidence, not acceptance of the revised construction economy.

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
