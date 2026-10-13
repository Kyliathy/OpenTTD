# R2MLVS1 execution record - recipes and bootstrap

Owner: [r2mlvs1-gameplay-contracts-and-content.md](r2mlvs1-gameplay-contracts-and-content.md), Groups 7-19. Architecture: [archi-cargo-payment.md](../../architecture/economy/archi-cargo-payment.md). Depends on the compiled [content contract](../../../../src/multilayer_content.h) and [initial content evidence](r2mlvs1-a-content-contract.md). Values are first tuning contracts, not measured final balance or delivered downstream facility behavior.

**Artifact-layout migration (2026-10-09):** this plan consumes the parent [Development artifact layout](r2mlvs-multilayer-vehicles-and-structures.md#development-artifact-layout), implemented by [R2FC](r2fc-file-cleanup.md). It supersedes earlier docs asset/review/helper placement and fixed runtime-config destinations below. New native homes use `mlm/runtime/`, generated feature reviews use `mlm/reviews/`, logs remain under `logs/`, and reusable code/curated inputs keep their code/test owners. Paths and focused native/cold verification are complete; R2FC records the results and retained-input identities. Repository-relative executable commands run from the checkout root through their context-aware wrappers. Historical pass records remain dated evidence and are not new acceptance claims. No gameplay task is completed by this migration and the separate post-demo fixture-pruning gate remains.

## Space economy revision - 2026-10-12

**Current user decision; implementation pending.** This section and the revised
[Space numerical budget](r2mlvs1-e-ground-and-space-budgets.md#space-economy-revision---2026-10-12)
supersede the earlier orbital construction/bootstrap rows below. Existing
Surface/underground recipes and their completed evidence remain valid. The
compiled version1 Space catalogue is the historical baseline; it must receive
an explicit content/save compatibility revision before these rules become live.
Rechecking a document does not finish that migration or any facility.

Every newly constructed orbital structure, module, mirror and Space vehicle
(including cross-layer rocket hulls) requires
**Steel (`STEL`) and Goods (`GOOD`)**. They may be shipped from Earth or made in
orbit. Initial station construction uses these two materials; later advanced
equipment may require additional declared inputs, always retaining this common
base. Natural asteroids are resources, not construction projects. Fuel and
recurring service keep their separately declared consumables.

An Earth-assembled probe or mirror carries a saved paid payload identity whose
Steel/Goods were already consumed during assembly. Deployment cannot debit
those materials again. Loose materials carried for a station blueprint remain
actual cargo until consumed by its orbital construction job. Money pays for
work/equipment; it never substitutes for missing delivered materials.

### Probe roles and commissioning

| Role | Initial commissioning | Orders/function | Later commissioning |
| --- | --- | --- | --- |
| Mining probe, existing model15 | Surface Space Launch Center, carried by an actual rocket | Visit body; mine a selected body/cargo; unload at an authorized station or return-drop center; service | Completed orbital Assembly with materials, berth, time and funds |
| Survey probe, new model19 | Same Earth launch path | Visit body; complete a saved scan revealing reserves to its company; return/service | Same completed Assembly gate |
| Construction probe, new model20 | Same Earth launch path | Carry Steel or Goods to a selected blueprint; construct/extend the selected station; unload/return/service | Same completed Assembly gate |

Model IDs0..18 retain their meanings;19/20 are additions, never replacements for
the later heavy construction robot16. All three initial probes are available
with the1990 launch technology. Buying, cloning or replacing a Space vehicle
must use the same material/commissioning gate. The first probe needs no station;
the first station needs only Earth-launched construction probes and supplies.

Typed vehicle orders identify a moving body or a generation-checked station,
blueprint or launch center, never its current tile or an `IndustryID` cast.
Visit, Survey, Mine, Deliver/Construct, Unload and Return/Service are distinct
operations with role checks. Queues, current index, partial work and blocked
reasons are synchronized/saved. Cancelling or editing an order preserves cargo
and releases only that vehicle's reservations. It must not grant a survey or
replay a completed delivery. Existing native Vehicle/Engine/cargo owners remain
authoritative; adding roles creates no second fleet or inventory registry.

### Survey knowledge

Asteroid position, silhouette, size and broad class may be visible immediately.
**Initial and remaining per-cargo quantities are hidden until a survey probe
finishes scanning that body for its company.** Selection, tooltips, assignment
lists, overlays and scripts use the same disclosure query. Display Unknown,
not a guessed or zero reserve. An unfinished/cancelled scan reveals nothing.
Completed knowledge is saved per body/company and then permits current reserve
display; viewing a layer does not discover deposits. Mining is permitted before
a survey, without revealing the body's quantities. Survey work extracts no ore
and does not monopolize the exclusive mining attachment lease.

Old preview survey controllers have no company owner and cannot grant knowledge.
Migration defaults to unsurveyed; save/load and company deletion/ID reuse must
not reveal another company's survey. Developer fixture ledgers may inspect
reserves for conservation, but ordinary player UI may not expose them.

### Build, process and reuse

Use a **Fund Industry-style orbital construction window**: choose station/module,
see footprint, money, Steel/Goods requirements and delivered/remaining stock,
then place a blueprint. Placement reserves occupancy and a construction job; it
provides no operational dock, factory, storage or assembly capacity. A first
construction probe may deliver to this empty site through the saved job itself,
so no pre-existing station is required. Multiple trips/probes can complete it.

Completed orbital Steelworks convert metallic asteroid ore to Steel; completed
Goodsworks convert Steel plus rare-earth concentrate to Goods. These are
explicit game recipes with bounded stock, work, power and disposition ledgers,
using the shared all-input planner/commit contract. Their own buildings must
first be constructed from shipped Steel/Goods. Existing Water Ice/Propellant
and mineral/Refined Metals choices remain later alternatives.

Products may be reserved for nearby construction/vehicle assembly, carried to
another orbital station, or sent through the authorized drop/descent/hub path
to a useful Earth receiver. Steel and Goods therefore belong in rocket,
construction-probe, orbital-hauler, return-pod and Surface collection admission.
`GOOD` already exists; do not allocate a duplicate cargo. Its native class is
Express, and its unit is a crate, currently8 sixteenths of a tonne. Capacity
checks widen and sum `count * CargoSpec::weight` before comparison, without
rounding every packet down; retain the authoritative unit/form for each label.

Every reserved/held load has one owner. Local production, construction,
reloading and dropping create no extra freight income. Industrial delivery is
settled once through CargoPayment at a valid useful intake; later consumption
is not another delivery. Company-supply purpose survives transfers and follows
the declared recipe policy on conversion. Cancellation returns only unconsumed
materials; consumed materials stay in the saved build/recipe ledger.

**Required freeze/acceptance:** revise the profile definitions and compatibility
signature, seed/price/cap table, all initial payload mass and bootstrap budgets,
three role capabilities/orders, survey knowledge and processed-product
provenance. Test Earth-only commissioning; two-stage first-station delivery;
missing either material; output/storage congestion; imported and locally made
inputs competing for one reservation; product drop versus orbital reuse;
save/network continuation; and company/typed-ID reuse. These are pending
implementation requirements, not new completed tasks.

### Group12B compiled definition reconciliation

`multilayer_space_content.h` publishes Space construction revision2 independently
of the unchanged ground profile1 and required cargo package1. `SDEF` in save410
pins the exact revision/signature. Older absence explicitly upgrades the
definitions while retaining every paid object, packet, reserve, mission and
work remainder. It creates no craft, stock, blueprint or survey knowledge.
Unknown revisions/signatures refuse full load and independent preview.

Logical models0..18 retain their meanings; survey19 and construction20 are
appended. `CanCommissionSpaceVehicle` admits these three probe roles at Earth
launch payload assembly or a completed orbital Assembly. Actual physical
commissioning, receipts and native engine registration remain their feature
owners' work; the definition query cannot allocate a vehicle. The old staged
model15 demo retains its historical purchase convention.

The public construction list includes every proposed build family, both common
materials, capital, work, embodied mass, year and static footprint. Newly settled
defaults are148 ticks for rocket/probe hull assembly; the later hauler uses the
heavy model's STEL6/GOOD12/RBPT4/ELEC2,18t,592 ticks and200,000; advanced refinery
and ice-processor buildings use STEL10/GOOD16,18t,740 ticks,220,000 and3x3.
These definitions enable no unfinished runtime producer. Pods are model18
construction payloads, never an independently purchased routed vehicle.

Manufacturing adds stable choices `OSTL` and `OGDS`; their physical residue
fields are32 and16 sixteenths of a tonne per committed batch. Processor code
must record that disposition alongside its single input consumption. Goods
procurement adds one existing-label SKU from1950 at120/crate and256/company/day.
Rocket, construction-probe, hauler and return-pod admissions include Goods'
Express class. Ground models0..13 and their admission rows remain unchanged.

The revised graph has14 milestones. The Core depends on two Earth builders;
heavy assembly depends on the completed Core and separately delivered Assembly.
It preserves the original six ground rows and the mirror's separate80,000
Surface farm with STEL8/ELEC4. Its total capital is4,144,000 and paid supplies
91,030, leaving764,970 from5,000,000 before recurring operations. Seven flights
consume105kL: initial16, then15 survey,15 mirror,29 builders,15 Assembly and15
heavy-material delivery. Each pad window adds1,500 and each mission5,000,
leaving719,470 before other work/travel/service/handling. These are definition
ledgers; real routes and physical receipts remain acceptance gates.

## Group 7 - mines, processors and bounded receivers

All cargo quantities use the existing packet unit for their label: tonnes for ores/metals/industrial packaged products; kilolitres for liquid CHEM/PROP and baseline OIL_; baseline GOOD keeps its existing crate unit. Stock caps below are per distinct label and never exceed the existing `uint16_t` industry stock bound. A mine debits finite deposit reserve only when a complete batch fits output stock. Discovery, construction, blocked output and paused work consume no reserve. Every mine requires excavated machinery/loading footprint and conventional access; none creates transport by sharing coordinates with Surface.

One production interval is **74 synchronized economy ticks**. Work accumulated between intervals is saved; calendar dates gate technology separately. Facilities evaluate no more than four batches per interval unless a row explicitly specifies one. The all-input publication in Group 8 supplies the execution/validation boundary before feature owners implement these records.

| Mine / layer / first date | Input per batch | Output / reserve debit per batch | Product stock cap | Time / maximum per interval | Useful Surface receiver |
| --- | --- | --- | --- | --- | --- |
| Coal / Underground / 1950 | none | COAL 4 t / COAL reserve 4 t | 1,024 t | 74 ticks / 1 | Existing Coal power station, capacity 64 t/interval |
| Iron / Underground / 1950 | none | IORE 4 t / IORE reserve 4 t | 1,024 t | 74 / 1 | Existing Steel mill, compatible original single-input conversion |
| Copper / Underground / 1950 | none | CORE 4 t / CORE reserve 4 t | 1,024 t | 74 / 1 | Metal refinery |
| Bauxite / Underground / 1950 | none | BAUX 4 t / BAUX reserve 4 t | 1,024 t | 74 / 1 | Metal refinery |
| Salt / Underground / 1950 | none | SALT 4 t / SALT reserve 4 t | 1,024 t | 74 / 1 | Chemical works |
| Lithium / Deep / 1980 | none | LITH 4 t / LITH reserve 4 t | 1,024 t | 74 / 1 | Surface Battery works from 1980 |
| Nickel / Deep / 1980 | none | NORE 4 t / NORE reserve 4 t | 1,024 t | 74 / 1 | Metal refinery already available |
| Cobalt / Deep / 1980 | none | COBT 4 t / COBT reserve 4 t | 1,024 t | 74 / 1 | Surface Battery works from 1980 |
| Rare earth / Deep / 1980 | none | RARE 2 t / RARE reserve 2 t | 512 t | 74 / 1 | Surface Electronics works from 1980 |
| Uranium / Deep / 1980 | none | URAN 1 t / URAN reserve 1 t | 256 t | 74 / 1 | Surface Nuclear plant from 1980; sealed conventional carrier available |

Ores may be produced into mine stock only after time, reserve and output-space checks succeed. Mine service cost is an independent recurring charge, not a second deduction of reserve. The normal model works without geothermal/tubes; those can improve later transport and processing.

| Processor / first date / alternative | Inputs per batch | Outputs / recorded disposition | Input / output stock cap per label | Time / maximum |
| --- | --- | --- | --- | --- |
| Metal refinery / 1950 / Copper | CORE 4 t + COAL 1 t | RFMT 2 t; ore/fuel consumed recorded | 1,024 / 1,024 | 74 ticks / 4 |
| Metal refinery / 1950 / Bauxite | BAUX 4 t + COAL 1 t | RFMT 2 t | 1,024 / 1,024 | 74 / 4 |
| Metal refinery / 1980 / Nickel | NORE 4 t + COAL 1 t | RFMT 2 t | 1,024 / 1,024 | 74 / 4 |
| Chemical works / 1950 | SALT 2 t + OIL_ 2 kL | CHEM 2 kL | 1,024 / 1,024 | 74 / 4 |
| Surface Battery works / 1980 | LITH 2 t + COBT 1 t + CHEM 1 kL | BATY 2 t | 1,024 / 1,024 | 74 / 4 |
| Surface Electronics works / 1980 | RFMT 2 t + RARE 1 t | ELEC 2 t | 1,024 / 1,024 | 74 / 4 |
| Component works / 1980 | STEL 2 t + RFMT 2 t + CHEM 1 kL | SCMP 2 t | 1,024 / 1,024 | 74 / 4 |
| Robotics works / 1980 | ELEC 2 t + BATY 1 t + RFMT 1 t | RBPT 2 t | 1,024 / 1,024 | 74 / 4 |
| Propellant line at Chemical works / 1980 / Chemicals | CHEM 2 kL | PROP 2 kL | 1,024 / 1,024 | 74 / 4 |
| Orbital processor / 1995 / Water Ice | WICE 2 t | PROP 1 kL; conversion consumes Water Ice | 1,024 / 1,024 | 74 / 4 |
| Orbital processor / 1995 / Nickel | NORE 4 t | RFMT 2 t | 1,024 / 1,024 | 74 / 4 |
| Mirror-kit works / 1980 | SCMP 2 t + ELEC 1 t | SMIK 1 t | 1,024 / 512 | 148 ticks / 2 |
| Nuclear plant / 1980 | URAN 1 t | metered electricity; no paid Power cargo | 256 / no freight output | 74 / 1 |

Alternative recipes are a **saved explicit selection**. The same refinery/processor does not consume every alternative at once. Single physical forms and reserve/consumption audit totals stay associated with their label. Surface product plants exist before the later geothermal factories; the latter do not supply their own first construction materials. Native production owns consumption even when the definition arrives through NewGRF; callbacks cannot consume that stock again.

| Spoil receiver / 1950 | Admission and processing | Bounded stock / intake | Money and useful outcome |
| --- | --- | --- | --- |
| Earth reclamation/disposal | ERTH 8 t per sink batch; record 8 t disposed | 1,024 t / 32 t per interval | Disposal charges 2 internal Money units/t; no freight payment |
| Rock crusher | ROCK 8 t per batch; 6 t construction aggregate + 2 t recorded residue | 1,024 t Rock; 1,024 t aggregate / 64 t Rock per interval | Charges 3 Money units/t intake; aggregate is bounded internal construction material, with at most 1 unit/t replacement value; no new freight cargo/payment |
| Rock surplus disposal at same receiver | ROCK 8 t per batch, all disposed | 1,024 t / same shared 64 t intake allowance | Charges 4 Money units/t; no freight payment; shared capacity prevents doubling crusher/disposal intake |

Aggregate is recorded by the receiver's ConstructionInventory and consumed in local ground foundations; it is not converted into unrestricted paid GOOD packets. Full aggregate stock pauses crushing; the player can select paid surplus disposal explicitly. This avoids a spoil-to-Goods transport subsidy while retaining useful construction output and the unchanged cargo budget. Group 13 includes its maximum replacement value in the spoil-loss ledger.

Receiver introduction dates above are content-contract requirements for Plan 6/12; they are not a claim that those industries already operate. Existing Coal/Iron consumers retain their current definitions and numeric meanings. All ten resource identities have a conventional vehicle/receiver path in the contract, with explicit 1980 gating for Deep extraction.

## Group 8 - one atomic all-input batch owner

The public [recipe record and pure planner](../../../../src/multilayer_recipe.h) is shared by mine, Surface, orbital and geothermal feature owners. Bundled definitions select a stable four-character `MultiLayerRecipeID`; the facility saves that explicit choice. Alternatives share one work budget and do not all run on the same incoming stock. Switching a choice retains cargo; its feature command must refuse while a batch is reserved. Callback presentation/availability may consult the record, but **native production alone consumes stock**. The ordinary `input_cargo_multiplier` path independently converts each incoming cargo, so a selected all-input facility must never fall through to that path. Classic facilities keep their existing production behavior.

Planning is read-only: compute the minimum of completed work, interval maximum, every input quotient, every output free-space quotient, reserve debit, granted heat/electricity and internal aggregate space. Zero batches change no stock, reserve, work, money or RNG. Positive batches authorize a proposed set of deltas, not mutation by this helper. The feature owner must revalidate the same stock/selected recipe and utility reservation at commit, debit all inputs/reserve/energy/time, credit every output and record material disposition atomically. It must never clamp an overflowing output after consuming inputs. Do not reacquire utility allocations during the same interval. A paused facility retains existing work and inputs but does not accrue unlimited work debt.

Validation rejects empty identity, empty activity, duplicate cargo roles within or across input/output lists, unknown physical forms, zero quantities, more than 16 inputs or outputs, stock caps above 65,535 and work/max-batch zero. Auxiliary `uint64_t` energy products are checked before multiplication. Existing stock width is `uint16_t`; computation uses `uint64_t`, bounded complete-batch count is `uint16_t`. Mine reserve identity/debit must be supplied together; exactly one output matches that identity and quantity. Cargo units/forms come from the same generated cargo catalogue as the content package; baseline recipe cargos keep their established meaning. No new runtime cargo registry is allocated.

Worked Component recipe (`COMP`): Steel 2 t **and** Refined Metals 2 t **and** Chemicals 1 kL make Components 2 t in 74 ticks; maximum four batches. With 296 ticks and stocks `(4,1,3)`, Refined Metals permit zero batches; `(4,1,3)` and output remain unchanged. With stocks `(4,6,3)`, two batches fit, proposing input deltas `(-4,-4,-2)`, output `+4`, work debit 148. The pure planner still leaves actual stocks unchanged. With the same inputs but Components stock 1,023/1,024, its single free tonne cannot fit a two-tonne product: zero batches and all materials/work are retained. At 1,022/1,024 exactly one batch fits. Consumers must display these pauses rather than discard partial products.

The focused fixture covers these examples, depleted finite reserve, separate heat/power shortages, aggregate saturation, maximum batch bounds, invalid/ambiguous stock and malformed/overflowing definitions. Feature mutation/save/network integration remains with Plans 6/11/18 and Schema Foundation; declaring a saved choice here does not implement its serialization.

## Group 9 - manufacturing choices and first robot inputs

[GetMultiLayerManufacturingRecipes](../../../../src/multilayer_recipe.cpp) publishes the numerical rows from Group 7. Stable choices are `CFMT`/`BFMT`/`NFMT` (refinery alternatives), `CHEM`, `BATT` (recipe identity, distinct from cargo `BATY`), `ELEC`, `COMP`, `RBOT`, `PCHM`/`PICE` (Propellant alternatives), `OREF` and `MKIT`. The Surface Chemical works exposes Chemicals versus Propellant as a selected line; orbital Water Ice requires its later processor and is never the first rocket's fuel source. Metal alternatives likewise share a refinery's saved work and stock.

`COMP` requires **both** STEL 2 t and RFMT 2 t, plus CHEM 1 kL: there is no interchangeable metal discount. Components require transported liquid Chemicals, with a tanker-compatible inventory. Electronics consume rare-earth concentrate at a Surface receiver available in 1980; Batteries consume Lithium, Cobalt and Chemicals at the Surface Battery works from the same date. These plants process conventional deliveries from Deep mines without requiring a tube, vent or geothermal factory. Their first outputs may instead be obtained through the bounded paid Surface supply described in Group 10.

The first heavy robot's provisional construction batch is **SCMP 6 t + RBPT 4 t + ELEC 2 t**, all delivered from Surface; assembly date 1995 and construction time/cash are assigned in Group 14. Expanding that batch through the Surface plants gives this complete input ledger:

| Production step | Batches | Inputs | Output committed to robot path |
| --- | --- | --- | --- |
| Components | 3 | STEL 6 + RFMT 6 + CHEM 3 | SCMP 6 |
| Robotics Parts | 2 | ELEC 4 + BATY 2 + RFMT 2 | RBPT 4 |
| Electronics, including direct assembly input | 3 | RFMT 6 + RARE 3 | ELEC 6: 4 to Robotics, 2 to assembly |
| Batteries | 1 | LITH 2 + COBT 1 + CHEM 1 | BATY 2 |
| Copper refinery | 7 | CORE 28 + COAL 7 | RFMT 14: 6 Components, 2 Robotics, 6 Electronics |
| Chemicals | 2 | SALT 4 + OIL_ 4 | CHEM 4: 3 Components, 1 Batteries |
| Existing Steel source | existing rule | Existing Iron/Steel chain or paid Surface supplies | STEL 6 |

Units are tonnes except CHEM/OIL_ kilolitres. Every packet moves to one next inventory; intermediates are never counted twice. Three Components/Electronics batches require 222 work ticks, two Robotics/Chemicals 148, one Batteries 74, and seven refinery batches 518 across at least two evaluations because the interval maximum is four. This is a dependency ledger, not seven parallel batches hidden in one interval. All stock is below the 1,024-per-label cap. The largest published manufacturing recipe has three inputs and one output; none approaches 16/16, so no split plant is needed for slot capacity. Mirror-kit output has a separate 512 t cap and 148-tick batch. No recipe consumes a product needed to create its own first raw input.

The manufacturing fixture validates every choice/identity, plans two complete batches, removes each required input in turn and fills output storage. This checks useful refusal behavior across the entire published table without installing downstream facilities.

## Group 10 - bootstrap graph and bounded external seed

[Bootstrap definitions](../../../../src/multilayer_bootstrap.h) publish the dependency graph, exact material ledger, seed prices and daily caps. Surface procurement is a native first-party station inventory service, supplied by the profile: it requires an owned Surface rail/lorry station, a paid order and actual carrier delivery to the construction/service inventory. It does not require an external NewGRF, add an industry type, seed invisible free stock or teleport material to Deep/Space. Plan 6 owns procurement/ConstructionInventory integration; Plan 9 owns launch-center transfers. Purchased packets retain a non-paying company-supply purpose until consumed; shipping them cannot earn ordinary final income or subsidies. Player-produced cargo remains the cheaper reusable industrial route.

| External Surface supply | First date | Money/unit | Per-company economy-day cap |
| --- | --- | --- | --- |
| STEL | 1950 | 100 | 256 t |
| RFMT | 1950 | 180 | 128 t |
| CHEM | 1950 | 220 | 128 kL |
| ELEC | 1965 | 500 | 64 t |
| BATY | 1965 | 350 | 64 t |
| SCMP | 1980 | 600 | 64 t |
| RBPT | 1980 | 750 | 32 t |
| PROP | 1980 | 250 | 64 kL |
| SMIK | 1980 | 1,500 | 16 t |

The 1965 imported Electronics/Batteries seed is explicit: local rare-earth/Lithium/Cobalt mining begins in 1980, but a 1970 capsule cannot depend on it. This is the only earlier external manufacturing exception; cargo meaning/form is unchanged. Caps are shared across all of a company's supply stations so cloning a station cannot multiply supply. Orders debit money once, reservations count toward the daily cap, cancellation returns unshipped material/money according to the single inventory owner. A refunded order cannot leave paid cargo behind.

| Milestone / earliest date | Required earlier route/access | Delivered construction/fuel inputs | Equipment/construction capital |
| --- | --- | --- | --- |
| Surface route / 1950 | Starting cash and owned Surface rail/lorry station | existing track/origin/commissioned portal route | 220,000 |
| Standard TBM train / 1950 | Surface route and portal commissioning bay | STEL 8 + RFMT 4 | 300,000 |
| First mine / 1950 | Standard TBM clears footprint/loading approach; conventional route | STEL 8 + RFMT 4 | 80,000 |
| First tube / 1970 | Standard TBM-cleared UG guideway/depot, conventional delivery and paid external power | STEL 12 + RFMT 8 + ELEC 4 + BATY 2 | 160,000 |
| Deep access / 1980 | Standard access, Deep-rated TBM and spoil train, commissioned Deep portal | STEL 12 + RFMT 8 + ELEC 4 | 440,000 |
| Collector/powerhouse/riser / 1980 | Deep-cleared utility footprints, conventional cargo route, commissioned portal for riser | STEL 24 + RFMT 16 + ELEC 8 + BATY 4 | 480,000 |
| Center/rocket / 1990 | Surface route, fueled launch pad | STEL 24 + RFMT 16 + SCMP 8 + ELEC 8 + PROP 16 | 900,000 |
| Probe / 1990 | Launch center/rocket plus orbital release mission | SCMP 4 + RFMT 5 + ELEC 3, including two pods and service preload | 104,000 |
| Mirror / 1990 | Launch mission, Surface solar target; no coordinate-only link | SMIK 4 + RFMT 2 (orbital), STEL 8 + ELEC 4 (Surface target) | 120,000 |
| Station nucleus / 1995 | Rocket cargo route and orbital construction site | SCMP 16 + RFMT 8 + ELEC 4 | 700,000 |
| Heavy robot / 1995 | Station nucleus and Surface-to-station deliveries | SCMP 6 + RBPT 4 + ELEC 2 | 200,000 |

Topology is `cash -> Surface route -> TBM -> mine/tube/Deep access -> geothermal`; separately `Surface route -> launch -> probe/mirror/station -> heavy robot`. Heat, tube cargo and asteroid Water Ice do not supply any prerequisite for their own first construction. A rocket fuel batch can use Surface `PCHM` before asteroid `PICE` exists; Group 9 expands the robot's native-manufacturing alternative. Each row's earliest date is the maximum of its technology, upstream access and paid supply dates; all ten mine receivers are present no later than extraction (Group 7). Normal Deep mining is not pulled earlier by the external seed.

The deterministic fixture topologically expands all 11 milestones, rejects missing/backward/self dependencies and verifies dates, physical cargo definitions and per-day seed caps. Group 14's reconciliation adds two expendable pods and one service preload explicitly. Capital totals **3,704,000**, paid supplies **76,380**; starting construction-proof cash **5,000,000** leaves **1,219,620** for the worked operating windows. This proves a funded finite build path, not a 45-year operating-profit forecast; long-term vehicle upkeep and ordinary freight revenue belong to balance acceptance. Group 13 uses a separate 1,000,000 ground-only scenario. No fixture invents free automatic revenue or spoil profit. Feature owners must validate real routes, clearing work, payment and calendar gates at commands; these table proofs do not deliver those behaviors.

## Group 11 - packet, inventory and income boundary

CargoPayment remains the sole freight-income authority. `PayTransfer` adds a virtual feeder share; `PayFinalDelivery` accounts the actually delivered slice; destruction of the payment helper settles route income. A portal, descent, launch-center job or arrival at an origin station is not a useful final delivery by itself. Layer transitions preserve the existing packet source, age, feeder share and paid-distance rules; extra layers do not create extra income or reset the source. No controller issues a separate cash reward for a packet whose carrier delivery is already being settled.

| Boundary | Material owner after commit | Income / consumption rule |
| --- | --- | --- |
| Mine output to vehicle | Vehicle owns the extracted packets; mine stock decreases once | No final income; extraction has already debited finite reserve |
| Vehicle to transfer station/center/descent/member job | Exactly one station/job owns that slice; count/source/age/share preserved | Transfer may carry existing virtual feeder share; handling charges are costs, no final payment |
| Last carrier to authorized useful receiver or construction/service intake | Receiver's bounded inventory owns the admitted quantity | Ordinary industrial-origin cargo is finalized once through CargoPayment for the accepted slice; overflow stays on vehicle |
| Construction/service reservation | Inventory remains authoritative; job holds a typed reservation | No second freight income and no material debit during estimate/reservation |
| Successful production/build/service commit | Inventory debited exactly once; product/build/service state credited atomically | No delivery reward at consumption; recipe disposition records account for consumed material |
| Cancellation before commit | Materials remain/return to their one valid owner | No duplicate packet, payment or refund; partial accepted quantities retain their earlier settlement |
| Paid Surface procurement to any destination | Company-supply packet/inventory retains its non-paying origin purpose | Purchase debited once; all transfers/final intake earn zero freight income and zero subsidy |
| Earth/Rock disposal | Receiver records actual disposed/aggregate quantities | Disposal debit only; zero freight income/subsidy, including the mandatory wagon leg |
| Heat/electric allocation | Shared utility ledger | No CargoPacket and no freight payment |

ConstructionInventory admits inputs by label/form and remaining capacity, reserves without consumption, and participates in an atomic commit only when the whole build/service operation fits. A delivery can pay while that inventory awaits other required materials; later consumption cannot pay again. Ordinary paid freight ending at a plant and its newly manufactured product are different packets with separate sources; a transferred unprocessed packet is not a new product. Bought company supplies cannot be relabelled into ordinary paying cargo through a station/center transfer; any transformation must debit the material and apply an explicitly permitted recipe, with spoil and virtual energy excluded.

Worked ordinary delivery: 10 SCMP units arrive, inventory has room for 6. Six are admitted/finalized once; four stay on the carrier. A build reserves 4 of the six, then cancellation releases that reservation: inventory still holds six and income remains the earlier six-unit settlement. Successful retry consumes four, leaving two; it issues no second settlement. For a bought company-supply packet the same conservation holds, with zero final income. Schema Foundation must retain the origin purpose, material ownership/reservations and stable references; Plans 6/9/11 own actual intake, cancellation and payment integration. This group defines that boundary without modifying Classic payment code, so no additional runtime regression is required.

## Group 12 - complete logical vehicle capability catalogue

[GetMultiLayerVehicleCapabilities](../../../../src/multilayer_vehicle_catalogue.cpp) is the read-only authority for all 19 model identities, dates, layers, movement domains, allowed cargo labels/classes, commissioning and service kinds. Model IDs 0..18 are profile-local content identities, **not EngineIDs**; original engine tables remain unchanged. Feature owners map them through the selected package and the eventual Space-category contract. `G` below means Surface/Underground/Deep transit; Deep is still gated by 1980. `U+D` is a dedicated layer-local tube guideway. TBM cutting permissions are Underground-only for Standard, Underground/Deep for Deep, even when Surface is a permitted commissioning/transit layer.

| Model ID / model | Date | Layers / movement compatibility | Cargo role | Commissioning / service |
| --- | --- | --- | --- | --- |
| 0 Standard TBM | 1950 | Surface/UG conventional rail and commissioned portals; no Deep | Cutter carries no freight; actual attached Earth/Rock wagons | Portal bay / excavated rail service |
| 1 Deep TBM | 1980 | G conventional rail/portals, Deep-rated work | Same separate spoil-wagon obligation | Portal bay / excavated rail service |
| 2 Earth hopper | 1950 | G conventional rail/portals | ERTH only; Bulk | Rail depot / rail service |
| 3 Rock hopper | 1950 | G conventional rail/portals | ROCK only; Bulk | Rail depot / rail service |
| 4 Conventional spoil hauler | 1950 | G conventional rail/portals | Locomotive hauls actual detached wagons, no hidden hold | Rail depot / rail service |
| 5 Mineral/heavy freight wagon | 1950 | G conventional rail/portals | Explicit ordinary ore/metal/packaged-product labels; Bulk/PieceGoods/Covered/Oversized; no liquids or URAN | Rail depot / rail service |
| 6 Sealed mineral wagon | 1980 | G conventional rail/portals | URAN/RARE; sealed PieceGoods/Covered/Hazardous | Rail depot / rail service |
| 7 Mine bulk truck | 1950 | G conventional roads, commissioned road/elevator links | Ordinary pourable ores/Earth/Rock; no URAN/RARE/liquids | Road depot / road service |
| 8 Sealed mineral carrier | 1980 | G conventional road/elevator links | URAN/RARE; sealed mineral role | Road depot / road service |
| 9 Mole-70 | 1970 | U+D dedicated tube; no ordinary rail, road or portal | Pourable ores/Earth/Rock, never excavation replacement | Powered tube depot / same-layer tube service |
| 10 Crate-75 | 1975 | U+D dedicated tube | BATY/ELEC/GOOD/SMIK; protected container products | Powered tube depot / same-layer tube service |
| 11 Cistern-80 | 1980 | U+D dedicated tube | CHEM/PROP; Liquid/Hazardous tanker | Powered tube depot / same-layer tube service |
| 12 Shield-85 | 1985 | U+D dedicated tube | URAN/RARE sealed mineral loads | Powered tube depot / same-layer tube service |
| 13 Forge-05 | 2005 | U+D dedicated tube | STEL/RFMT/SCMP/RBPT heavy solid products | Powered tube depot / same-layer tube service |
| 14 Surface launch rocket | 1990 | Surface-to-Space saved launch mission; no ground portals | Explicit solid construction payload; PROP is a separate fuel inventory | Launch center / launch recovery |
| 15 Mining probe | 1990 | Space orbital orders | NORE/RARE/WICE; appropriate covered/refrigerated hold | Rocket payload / orbital service |
| 16 Heavy robot | 1995 | Space orbital work | No paid freight hold; consumes separate reserved construction inputs | Orbital assembly / orbital service |
| 17 Orbital cargo hauler | 1995 | Space orbital orders | Explicit solids/ice and separate PROP tanker refit; no inferred form conversion | Orbital assembly / orbital service |
| 18 Return pod | 1990 | Space-to-Surface saved descent job | NORE/RARE/WICE, preserved packets | Rocket payload / launch recovery; no purchasable routed fleet |

Cargo roles use the exact code label lists as well as form/safety checks; a broad class alone does not permit another label. Mineral wagon refits preserve one physical hold mode at a time, and sealed hazardous cargo cannot enter an ordinary Bulk hold. Compatible existing Surface/underground tanker models supply liquid CHEM/PROP by conventional transport; their explicit first-party refit mapping and live engine verification belong to Plan 5, with no extra logical design budgeted here. The fixture verifies every new design's liquid labels require the Liquid class and all ten resources have an introduced conventional carrier. Native railtype compatibility/roadtype geometry and depot enforcement are still feature work.

Replacement requires the same movement domain, permitted current layer, cargo safety/form, available introduction date and correct commissioning/service facility. Standard-to-Deep TBM upgrade needs the portal bay and 1980; replacement never commissions a TBM at an ordinary depot. Clone/build/autoreplace/refit must each validate these rules; `CloneVehicle` is `NoTest`, so a successful generic estimate is insufficient. Capsules may replace only within a compatible tube cargo role; no tube/rail conversion, cross-layer tube movement or portal purchase bypass. Spoil wagons retain IDs/packets during exchange and cannot be silently replaced while loaded/reserved. Return pods are manufactured/service inventory deployed by a mission rather than cloned fleet entries. These contracts do not claim command enforcement or vehicle art is implemented.
