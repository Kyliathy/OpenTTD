# Mine industry lifecycle execution record

This record accompanies [R2MLVS6](../../../zz-reach2/upgrades/2026-10/r2mlvs6-mines-and-resource-chains.md),
[AD-7](../../../zz-reach2/upgrades/2026-10/r2mlvs-multilayer-vehicles-and-structures.md#ad-7---deposits-mines-and-transport-are-distinct-owners-2-6-7)
and [map layer architecture](../../../zz-reach2/architecture/map-layers/archi-map-layers.md).
It records live code findings, implementation decisions and focused validation;
historical navigation ranges are not implementation evidence.

**Artifact-layout migration (2026-10-09):** this plan consumes the parent [Development artifact layout](../../../zz-reach2/upgrades/2026-10/r2mlvs-multilayer-vehicles-and-structures.md#development-artifact-layout), implemented by [R2FC](../../../zz-reach2/upgrades/2026-10/r2fc-file-cleanup.md). It supersedes earlier docs asset/review/helper placement and fixed runtime-config destinations below. New native homes use `mlm/runtime/`, generated feature reviews use `mlm/reviews/`, logs remain under `logs/`, and reusable code/curated inputs keep their code/test owners. Paths and focused native/cold verification are complete; R2FC records the results and retained-input identities. Repository-relative executable commands run from the checkout root through their context-aware wrappers. Historical pass records remain dated evidence and are not new acceptance claims. No gameplay task is completed by this migration and the separate post-demo fixture-pruning gate remains.

## Group 1: lifecycle characterization

### Funded placement and identity (1.1)

`CmdBuildIndustry` validates enabled definitions and layer construction policy,
then passes each candidate layout to `CreateNewIndustryHelper`. Direct generation
uses `CreateNewIndustry`, which calls the helper without passing the command's
Surface restriction. The helper checks the entire layout through
`CheckExcavationConstruction`, conflicts, town, land/water/vehicle occupancy,
industry and tile callbacks, slopes, platform levelling and pool capacity.
Only execution allocates the stable pool-owned `IndustryID` and calls
`DoCreateNewIndustry`; estimates do not allocate an industry.

The industry owns `location.tile` and its layer-bounded `TileArea`. Layout
validation uses `TileAddWrap`, but construction uses raw tile differences;
accepted geometry must be computed once and reused before writing any tiles.
Special water-check tiles are validation cells rather than machinery. The mine
request additionally needs the complete loading approach, the explicit deposit
reference and a real claiming company. Random Surface prospecting cannot choose
a geological mine site. Clearing costs are presently discarded by the helper,
and `DoCreateNewIndustry` ignores clearing command results after identity and
industry lists have already changed. Mines require a fully prepared transaction
before this boundary. Native mine placement must skip Surface platform levelling.

Callback inputs include world tiles, selected layout, founder, creation reason
and seeded random bits. Native layer definitions must have explicit package and
local-definition identities and disable inherited Surface location callbacks;
ordinary GRFs retain their Surface policy. Substitute graphics grant no authority.

### Tile loops, effects and production (1.2)

| Boundary | Live behavior | Required mine behavior |
| --- | --- | --- |
| `TileLoop_Industry` | Asserts Surface, runs water handling, then construction, dispatch and graphics-based effects. | Capability admission; construction works on excavated rock; no water, ambient smoke, sparks or inherited farm behavior underground. |
| Industry animation | Fallback graphics can invoke Surface-specific effects and randomization. | Native mine artwork/animation is presentation only; disable inherited effects until actual assets are delivered. |
| `OnTick_Industry` | Iterates every industry independently of the viewed layer; excludes editor production. | Retain scheduling independence; apply date/capability checks to native operations. |
| `ProduceIndustryGoodsHelper` | Adds rates with a 16-bit clamp. | Extract the minimum of work, finite reserve and free output capacity; debit only accountable output. |
| `IndustryProductionCallback` | May change input/output stock independently of the native rate path. | One native all-input batch owner; never invoke both owners for the same batch. |
| `TransportIndustryGoods` | Removes output first, reduces it during recession, counts production at dispatch, then attempts station supply. | Keep finite stock when there is no receiver; account extraction once, dispatch only transported quantities, with exact packet admission. |
| Climate/water/fields/trees | Built-in Surface policies and graphics behavior. | Capability-disabled for mines; Surface processors preserve supported Surface behavior. |

### Association, closure and access (1.3)

`PopulateStationsNearby` scans a layer-bounded area through
`ForAllStationsAroundTiles`; station catchment contains world tiles. Thus existing
catchment traversal is already physically local to the industry layer. Explicit
same-layer checks are still required at supply/intake boundaries to reject stale
or externally supplied associations. `Station::AddIndustryToDeliver` measures
plane distance, which deliberately ignores layer; distance is not admission.

`ClosestTownFromTile` already projects onto Surface for town identity. The
industry's town is a naming/rating reference, not a cargo bridge. The existing
distance helpers already use plane coordinates; do not add stacked-Y freight
distance or create new distance authorities. `ScriptIndustry::GetLocation`
returns the world tile unchanged; script construction passes the shared command.
Industry views/directories use the saved location for scrolling. Visible layer
names, reserve information and navigation are later UI obligations.

`Industry::~Industry` iterates its saved footprint and removes associations,
news, subsidies and source references. `MakeWaterKeepingClass` preserves land
and the separate excavation authority survives tile clearing, but no mine binding
currently exists. Mine demolition must release its claim without deleting or
replenishing the deposit; closure with buffered output requires explicit policy
so finite cargo is not silently deleted. Pool cleanup and loading must not issue
gameplay reserve mutations. Company ownership changes need the same binding and
claim authority.

Required failure fixtures for the unsupported assumptions:

- Fund an ordinary industry on excavated Underground/Deep floor: refuse unchanged;
  a whitelisted native mine will instead require matching seam and date admission.
- Generate the same unsupported definition through the shared helper: refuse
  before allocation, rather than bypass the command's Surface check.
- Include one solid corner or loading approach: refuse before clearing anything.
- Attach a second mine to an already bound same-company seam: refuse even though
  the geological company's `Claim` operation is idempotent.
- Produce with one tonne of reserve or a nearly full output: preserve every tonne
  and avoid the legacy clamp; save/retry cannot extract twice.
- Supply without a station, with exhausted packet capacity, or a same-XY
  station on another layer: retain mine output and create no cargo/demand edge.
- Tick a hidden rock-layer mine: construction/production must not hit the
  Surface assertion or emit inherited Surface effects.
- Demolish and rebuild after partial depletion: reuse the original deposit ID
  and remaining reserve, release the old industry binding, preserve the cavity.

## Group 2: change ledger (2.1)

The implementation ledger records each completed change with its group, owner
and validation. Admission, identity/claim transaction, ticking, finite extraction,
dispatch, recipes, intake, persistence and presentation are separate obligations.
Removing the TileLoop assertion alone cannot complete the lifecycle work.

## Group 3: native admission implementation

`multilayer_industry.h` and `layer_industry_policy.cpp` derive an immutable
whitelist from the ten published mine recipes. Only exact enabled native
package/local definitions with the declared single output and no competing
production callback/input owner qualify. Original and ordinary GRF industries
keep Surface policy; neither cargo resemblance nor substituted artwork qualifies.

`ValidateMineDepositClaim` derives all machinery and a two-wide, two-long loading
approach, checks the entire excavated footprint, the selected seam's layer,
resource, claim, remaining reserve and actual machinery contact, then checks
clear flat foundations and company-owned conventional loading access. It quotes
machinery plus clearing and doubles Deep equipment cost; estimates create no
identity, tiles or geological claim. Publication and actual funding commit remain
the following groups' responsibilities.

Debug compilation succeeded. `[layer-industry-admission]` passed 99 assertions
in four cases, covering exact identity/output, unsupported layers, a missing
loading corner, claimed/exhausted/invalid seams, unchanged estimates and the
1980 calendar boundary. Build evidence: `r2mlvs6-b-build.log` and
`r2mlvs6-c-admission-build.log` in this directory.

## Group 4: atomic funding and rebuilding

`BuildLayerMine` is appended to the command enumeration, preserving earlier IDs.
Its request carries a world anchor, exact definition/layout and selected deposit
ID. The existing funding UI sends this command for native mines, including when
legacy raw industries use random prospecting. All other construction/generation
paths hit the same whitelist in `CreateNewIndustryHelper`; a geological mine
without a real company and explicit seam is refused before allocation.

Complete site, clear-floor cost, machinery cost, affordability, town and pool
admission precede identity allocation. The binding allocates its bookkeeping
before acquiring a stamped exclusive geological claim. Failure destroys only
the unplaced zero-footprint candidate. Admitted native foundations are already
empty and flat: `DoCreateNewIndustry` reuses those exact cells and writes them
without nested execution-time clearing or platform terraforming. Consequently
there is no fallible clearing step after the first native construction write.
Legacy Surface placement retains its existing clearing behavior.

`LayerMineState` holds actual IndustryID/deposit/company references and historical
counters; reserve remains solely geological and output solely in the industry's
existing produced slots. The geology owner supplies `ClaimLayerMineDeposit`,
`ReleaseLayerMineDeposit` and `ConsumeLayerMineReserve`, which reject stale stamps
and repeated claims/debits. Demolition releases the claim and retains the seam,
remaining reserve and independent packed cavity. A normal claimant can demolish
an empty mine; stored finite output refuses demolition. Legacy random closure,
rates, water and graphics-driven Surface effects cannot run on a bound mine.
Whole-world industry initialization clears side bindings without generating ore.

Debug build passed. The focused admission/geology/funding burst passed 239
assertions in eight cases, including estimate purity, one actual industry and
claim, missing machinery/loading corners, competing claims, depleted seams,
insufficient funds, buffered-output demolition refusal and partially depleted
demolition/rebuild. Build evidence: `r2mlvs6-d-funding-build.log` and
`r2mlvs6-e-funding-checks.log`.

Native definition registration remains Group 9 and save publication is required
before enabling those definitions in real games. The current fixtures install
one definition through the existing IIDS owner to validate the boundary; they
do not substitute for a completed runtime catalogue or useful transport chain.

## Producer gate evidence (initial verification details)

## Group 5: lifecycle, layer names and shared availability

The company takeover/removal boundary transfers the actual geological claim and
native IndustryID binding together, including exhausted seams. Removal releases
the company claim and marks the physical mine abandoned; its existing output
remains in the industry. An abandoned physical binding still excludes another
mine on the same seam until the empty site is demolished. Empty abandoned sites
can be cleared by a real company without creating fresh geological material.
Depletion retains its claim and output rather than entering random legacy
closure; Group 7 installs the extraction-driven phase transition.

Industry names include their actual rock layer. Native station/waypoint cached
names likewise include the operating layer, propagating into orders and lists.
Opening news uses the industry name, and existing world-tile navigation already
selects the correct viewport layer. The station association boundary explicitly
rejects another layer even if a stale association would share plane coordinates.

`IsLayerIndustryTechnologyAvailable` owns the native calendar rule used by the
command, funding availability, construction activation and script queries. An
editor override cannot operate Deep minerals before 1980. Company scripts
transmit the selected seam to `BuildLayerMine`; geological prospecting and deity
claims are refused. Native mine ticking excludes the legacy rate owner until
the finite producer in Group 7 is installed.

The whitelist also identifies seven Surface processor families by their native
IIDS identity and the exact union of their content-owned alternative recipes'
input/output meanings. They retain Surface permission and use ordinary industry
funding; the seam command explicitly refuses processors. Actual processor
registration and execution remain Group 10.

Focused mine checks passed 346 assertions in 12 cases. A preceding compatibility
burst covering lifecycle/geology/funding plus `[spoil-receiver-definition]` passed
530 assertions in 10 cases. `r2mlvs6-g-lifecycle-checks.log` and
`r2mlvs6-h-capability-build.log` record successful Debug builds. The failed
`r2mlvs6-f-lifecycle-build.log` records the protected legacy NewGRF news-string
range check: the new string was moved outside all contiguous legacy ranges,
and the next builds passed. Future native strings must be appended outside
those ranges.

## Producer gate evidence (initial verification)

On 2026-10-05 the three required public headers and every named export were
present. Content owns profile selection/calendar availability; excavation owns
packed terrain, finite deposits and claims; the exchange owner moves actual
saved wagon sets without a substitute inventory. The geological world accessor
is deliberately read-only: a live mine transaction needs an explicit geological
owner operation, rather than mutating a captured save snapshot.

Focused executable validation with `build/Debug/openttd_test.exe`:

- `[multilayer-content-contract],[excavation-contract],[excavation-cut],`
  `[excavation-cut-reserve],[tbm-exchange-consumer],[tbm-exchange-commit]`:
  7,573 assertions in 10 cases passed.
- `[excavation-footprint],[geology-claims]`: 52 assertions in two cases passed.

The first invocation also included `[excavation-guard]`, which is not an exported
test tag; `[excavation-footprint]` was run separately to validate actual footprint
behavior. These are focused producer checks, not a full regression.

## Group 6: public industry and batch handoff

The public header keeps geological extraction separate from delivered-input
processing. Recipe preparation borrows an explicit `LayerRecipeInventoryView`;
execution requires the same actual view and a `LayerRecipeCommitParticipant`
that commits all inputs, outputs and work together. It can be implemented by a
later orbital owner without creating another industry pool or utility account.
The type/signature/default fixture passed 27 assertions in one case after the
user resumed mine work, recorded in `r2mlvs6-aa-contract-checks.log`.

## Group 7: finite production and batch execution

Finite extraction uses real `Industry::produced` stock and the claimed
`GeologyDeposit`. One synchronized tick earns exactly the published recipe
work. `LayerMineState::work_fraction` explicitly preserves fractional work
with the recipe's output quantity as denominator; it is a saved work remainder,
not fractional cargo. This avoids rounding each small extraction upward and
changing the content-owned rate. Work is bounded to one recipe interval.
Paused, unfinished, stopped and calendar-locked mines preserve all balances.

The shared batch executor recomputes every input/output/work delta and compares
the entire actual owner view before invoking its non-failing commit participant.
It refuses scalar-only heat/power commits. The current electrical participant and staged-work extension are recorded under Group13; all electrical authority stays with the shared ledger.
This scoped gate does not block ordinary material recipes. The native tile loop
currently completes machinery and returns before legacy station dispatch, so
Group 8 must install exact finite dispatch; it must not re-enable the lossy
legacy `TransportIndustryGoods` path.

The shared real funding/geology fixture now lives in `src/tests/mine_fixture.h`.
`[layer-mine-production],[layer-recipe-batch]` passed 647 assertions in nine
cases. The content producer's `[multilayer-recipe],[multilayer-manufacturing]`
passed 112 assertions in four cases. Evidence is recorded in
`r2mlvs6-ab-production-build.log`, `r2mlvs6-ac-production-checks.log` and
`r2mlvs6-ad-recipe-producer-checks.log`. These checks cover all ten actual
mine definitions, real economy ticking, exact work carry, partial depletion,
full/corrupt output, paused/stopped/date-locked operation, complete multi-input
and multi-output batches, wide work counters, stale/edited/foreign quotes,
participant refusal and scalar-only utility refusal.

## Group 8: exact dispatch and packet capacity

`src/station_supply.h` publishes a station-owned pure admission ticket and
exact integer commit. Native finite queue admission stops at `UINT16_MAX`
whole units per cargo, reads actual packet-backed totals, preserves older
larger queues and never overwrites station `amount_fract`. The commit passes
whole units shifted by eight bits through `UpdateStationWaiting`, so the
existing fraction stays unchanged. The actual native source, catchment,
layer, company, service, rating, queue and packet room are revalidated.

Mine tickets bind the source industry identity, saved revision, physical
output and monthly transported count to the selected station admission.
Only the exact station receipt is subtracted from output. Depleted mines may
dispatch their last stock. Production history is now credited once at reserve
extraction; dispatch changes only transported history. Narrow history limits
are admitted before addition. Cargo-scale percentage and recession act on
earned work before extraction, using `LayerMineState::scale_fraction` modulo
200 instead of RNG or post-production loss.

`[layer-mine-dispatch],[layer-mine-production]` passed 912 assertions in nine
cases (`r2mlvs6-af-dispatch-checks.log`). The actual full packet-pool case is
isolated in `src/tests/layer_mine_dispatch_pool.cpp`; its optimized run passed
37 assertions in one case in 1.5 seconds (`r2mlvs6-ah-packet-pool-checks.log`).
Empty real allocations filled the actual hard pool limit without cargo or
invented counters. Refusal preserved output; freeing one actual slot allowed
exactly four units into real station packets. The optimized test/game build
passed (`r2mlvs6-ag-dispatch-release-build.log`). Full save/load retry was moved
explicitly to Group 16, where side-state persistence is implemented; transient
dispatch tickets are not savegame data.

Continuation reading for this group: the plan's Global Mandatory Reading and
the Resource production and consumers Area block; specifically
`Industry::ProducedCargo`, `TransportIndustryGoods`, `UpdateStationWaiting`,
`CanMoveGoodsToStation`, `PrepareExactStationSupply`, `CommitExactStationSupply`
and the new mine production/dispatch functions. Other Area sources relate to
later intake, vehicle link-graph and utility work. The native tile loop already
bypasses legacy Surface dispatch, so retain that separate path.

## Group 9: native catalogue and footprints

`src/layer_industry_definitions.cpp` now installs all ten native mine and tile
families after normal GRF polishing, using exact first-party identity and the
existing IIDS/TIDS owners. Each native layout is nine machinery cells in the
manifest's 3x3 rectangle, plus the admission owner's separate 2x2 loading
approach. The ten logical art bindings are `mine.COAL` through `mine.URAN`,
published read-only in `LayerIndustryCapability::asset_id`. The manifest's
mine source/packed fields remain planned/null: original visible art substitutes
are retained and no packed mine-art completion is claimed. Own mine names are
appended after protected legacy string ranges.

Native rates/callbacks/random generation are inert; claims and remaining reserve
are never created by definition installation. Known native IDs with an invalid
capability cannot fall through ordinary Surface funding. Native ticking holds
an unbound/restoring mine instead of entering legacy Surface behavior. Full
side-state persistence still belongs to Group 16.

The `[layer-mine-definition]` fixture installs actual definitions, verifies
all ten label/IIDS/TIDS/asset mappings and funds/produces through the same
real command and geological owners. Debug compilation and the focused
`[layer-mine-definition]` burst passed 1,108 assertions in two cases, recorded
in `r2mlvs6-ak-definitions-build.log` and `r2mlvs6-am-definition-checks.log`.
No task group has compacted during the resumed work so far.

The startup repair now also pairs language packs beside each Windows Debug or
RelWithDebInfo playable executable in CMake's post-build event. Both configurations
were paired successfully (`r2mlvs6-ai-paired-debug-build.log`,
`r2mlvs6-aj-paired-release-build.log`). After mine-name generation changed shared
`build/lang/english.lng` to `0x7EAF128B`, the previous executable's paired
`0x0A21A2BC` pack still allowed startup with `build/play.cfg` (exit 0). This
protects playable snapshots when only tests rebuild the shared string table.

Continuation reading for Group 9: Global Mandatory Reading; Resource production
and consumers Area block; exact mine definitions/capabilities and
`InitialiseSpoilReceiverDefinitions` as the existing native IIDS/TIDS installer
pattern. The public sprite handoff and actual `mine.*` manifest entries govern
footprints; do not reread the entire graphics plan or edit the concurrent art
work. Native artwork remains a later acceptance gap.

## Group 10: native processors and final delivery

Seven real Surface facility families now install through the existing IIDS/TIDS
owners with 4x4 machinery, exact runtime input/output unions and logical
`facility.*` bindings. Original visible tile substitutes remain until packed
factory art is delivered. The three-label legacy translation arrays are not
runtime acceptance storage: a focused fixture caught that distinction before
playable factory registration. Native installation uses the actual sixteen
acceptance slots after GRF polishing.

`src/layer_industry_processor.h/.cpp` owns recipe choice, bounded rational work,
revision and historical counters. Physical cargo stays exclusively in
`Industry::accepted/produced`. A native adapter executes all-input batches via
the Group 7 producer; actual intake history, stock room and monthly output
history are admitted before any narrow write. Paused, unfinished, unavailable
and output-full facilities preserve material. Alternative selection is a
synchronized command controlled by the funding company/deity; the industry
window exposes available alternatives and their actual quantities. Demolition
cannot discard incoming or produced stock. Company takeover transfers recipe
control, without replacing stock or immutable founding history.

`CargoPayment::PayFinalDelivery` now returns the exact accepted whole-unit
receipt. `CargoDelivery` removes only those units, retains refused freight
aboard and settles feeder shares only for admitted units. `VehicleCargoList::Unload`
reports the actual packet debit. Native processor intake requires no deferred
legacy arrival callback: its stock and delivered flag commit immediately, and
its common batch owner runs on actual industry ticks. Existing Earth/Rock
definitions, fees, intake limits and interval processing remain in r2mlvs5.

Focused Debug processing plus final delivery/spoil compatibility passed 9,923
assertions in ten cases (`r2mlvs6-be-processor-delivery-checks.log`). This includes
all ten frozen Surface alternatives, missing inputs, output/history capacity,
pause, controller/calendar selection, exact station packets, source rejection,
an almost-full refinery and actual spoil wagons with only one intake unit left
per receiver. Debug test/game builds passed (`r2mlvs6-bd-arrival-build.log`).
These are packet/material boundary checks; they do not claim the later complete
fleet, cross-layer scenario, persistence or actual artwork.

Task 10.3 remains gated: `src/utility_power.h` and its tested native generation/
allocation producer are absent. No new Uranium acceptance or substitute grid
was installed. The scoped gate permits subsequent independent material work.
Group 10 compacted once during implementation. Continuation reads should use
Global AD-7/controller/command anchors and the Resource production and consumers
Area block; relocate symbols because older line hints are stale. Read the utility
producer only when that handoff actually appears, not its complete later plan.

## Group 11: existing baseline receiver compatibility

`src/layer_industry_baseline.h/.cpp` resolves actual Coal/Iron labels and
reuses existing compatible definitions. The original steelworks has one Iron
input, one Steel output, zero independent production rate, no GRF callback
owner and multiplier 256 (one to one). Names and substitute graphics grant no
adapter authority. Other multipliers, products or callback owners are refused;
the frozen profile supplies no alternative Steel recipe/ID to guess on failure.
The original accept-only Coal definition matches its input, but resolution
alone grants no useful-power generation/allocation. That integration is pending.

The selected-profile Steel path bounds conversion by real incoming Iron,
physical Steel space and monthly production room. It retains excess input,
credits production once and uses the existing exact station packet owner.
The actual industry dispatcher retries held Iron after space returns; intake
records delivery independently and borrows no deferred legacy callback pointer.
Demolition/random closure cannot discard stored Iron or Steel. Classic and
unsupported external definitions retain their existing production owners.
No new industry, cargo, recipe or utility identity is allocated by this reuse.

The focused original-receiver, packet and partial-spoil burst passed 2,029
assertions in six cases (`r2mlvs6-bm-baseline-delivery-checks.log`), including
real Iron packets, actual tick conversion and full output/exact supply retry.
Debug builds are recorded in `r2mlvs6-bg-baseline-build.log`,
`r2mlvs6-bj-baseline-supply-build.log` and `r2mlvs6-bl-baseline-tick-build.log`.
Group 11 did not compact. Its material and useful-power acceptance tasks were
split explicitly so completed Steel work cannot conceal the missing power gate.

## Group 12: freight accounting and Surface manufacturing

`CargoPacket::GetDistance` now projects its saved travel vector into plane
coordinates before calculating paid movement. The existing persisted vector
keeps its stacked encoding: same-layer station unload/load pairs telescope
the aboard vector's storage offset to the original source layer. Removing that
offset prevents a vertical vehicle leg from paying for a free move within a
transfer station. No save field/version change or cargo migration is needed.
The largest supported map dimension and every Surface/Underground/Deep endpoint
combination passed. Same-XY foreign-layer stations gain neither finite output
nor a demand edge; the actual supplied source graph has one node, with no
invented vehicle service.

Partial intake additionally preserves the original real packet, source station
and remaining feeder share. One accepted unit from an eight-unit packet with
80 transfer-share units settles ten and leaves seven units/70 shares aboard.
These custody/geometry cases passed 199 assertions in six cases
(`r2mlvs6-br-freight-checks.log`).

The connected Surface manufacture fixture funds eight actual factories,
including separate chemical/propellant works to respect self-source refusal.
Initial explicit raw shipments are CORE 20, COAL 5, SALT 6, OIL_ 6, STEL 4,
RARE 2, LITH 2 and COBT 1. Common batches and real source-station/carrier/
receiver packets leave PROP 2, SCMP 2, RBPT 2 and SMIK 1, plus RFMT/CHEM/BATY/
ELEC one each. Every factory is Surface and all raw input buffers are empty;
no orbital product supplies another recipe. This actual material/custody
boundary passed 173 assertions in one case (`r2mlvs6-bt-surface-chain-checks.log`).
Given initial shipments and a zero-priced reduced cargo fixture are explicit:
this is not a procurement-cost or physically driven fleet acceptance claim.

Native freight integration remains pending. r2mlvs7 Group 7's mine trucks,
mineral wagons, sealed carriers and actual purchase/refit fixtures are unchecked;
`src/underground_gui.h` is absent. r2mlvs1 publishes logical model IDs only.
Group 12 tasks were split to show that dependency without blocking independent
stock, income-coordinate, manufacturing and persistence work. No new EIDS,
vehicle registry or service edge was fabricated. Group 12 has not compacted.

Continuation notes: read the Resource production and consumers Area block,
actual `CargoPacket::GetDistance`, final-delivery receipts and source-supply
owner for this group; future save work must preserve the current cargo-vector
encoding. Group 16 must validate exact processor slot/map keys before its
private adapter uses `.at()`. Mine abandonment currently preserves output but
dispatch requires a valid company, so Group 16's lifecycle/recovery cases must
resolve that stranded-output policy rather than silently deleting it.

## Group 13: delivered batch API and blocked utility participant

**Current electrical boundary, 2026-10-16.** The absent-utility statements below describe the original delivery baseline. The shared electrical owner and whole-grant recipe participant are now delivered. [Plan11 Group10](../../../zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md#group-10---bind-orbital-inventory-to-the-shared-recipe-executor) additionally publishes [recipe-bound staged work](../../../../src/layer_recipe_work.h): actual atomic electrical mutations earn private pending progress, then the same original all-input/output executor settles it once without charging electricity twice. Existing whole-grant and unpowered participants retain their paths. Both configurations pass 2,447 focused assertions/23 cases (`logs/orbital-recipe-work-verification.json`); the index binds actual sources, executables and terminal logs. This shared producer supplies no orbital packet owner or processing save state; Plan11 Groups10B/10C own those consumers. Direct thermal commits remain outside this staged electrical API.

All six frozen geothermal choices now have pure quantity/refusal checks for
supplied heat/power, a missing transported role and full output. Positive scalar
energy plans do not authorize a commit: the actual utility owner remains absent.
`[layer-recipe-heat-boundary]` passed 126 assertions in one case
(`r2mlvs6-by-heat-contract-checks.log`). The standalone fixtures allocate actual
four-layer geometry, correcting an earlier test-order dependency on prior mine
fixtures for Deep validity. Neither geology nor utility authority is created.

The delivered callable signatures are:

```cpp
LayerRecipeBatchProposal PrepareLayerRecipeBatch(const MultiLayerRecipe &recipe,
    const LayerRecipeInventoryView &inventory);
LayerRecipeBatchReceipt CommitLayerRecipeBatch(const MultiLayerRecipe &recipe,
    const LayerRecipeInventoryView &inventory, LayerRecipeCommitParticipant &participant,
    const LayerRecipeBatchProposal &proposal);
```

| Delivered field/boundary | Ownership and lifetime |
| --- | --- |
| `snapshot.recipe/layer/revision/active` | Actual facility choice, layer, saved stamp and construction/calendar/pause permission; the owner supplies them. |
| `snapshot.inputs/outputs/work_ticks` | Pure view of the sole physical input/output/work owner, with whole-unit capacities; the copy grants no custody. |
| `snapshot.heat_j/power_j` | Read-only proposed quantities; current electrical commits require the same participant's actual native grant. Positive scalar quantities alone grant no permission. |
| `inventory.ElectricalWork` / `participant.ElectricalWork` | Optional pointers to the same noncopyable shared paid-work receipt. Ordinary callers expose none; a copied scalar work count cannot substitute. |
| `LayerRecipeElectricalWorkRecord` | Immutable selected recipe/rate/consumer, bounded already-consumed pending work, sequence and per-opportunity use. This record is physical progress, never a usable or refundable energy account; native consumers must validate their actual incarnation joins. |
| `proposal.owner/definition/before` | Borrowed real view and immutable content record plus expected snapshot, valid only within the same simulation action; never saved. |
| `proposal.plan/inputs/outputs/work_ticks` | Recomputed all-role batch count and exact deltas. Missing input/output/work returns zero without consumption. |
| `participant.Inventory/Validate/Commit` | Same actual view, pure complete revalidation and one non-allocating/non-failing commit; refusal preserves all quantities, success advances saved revision. |
| `receipt.error/batches` | Exact complete result; stale, foreign, modified or refused proposals grant no physical mutation. |

r2mlvs11's SpaceStructure and r2mlvs18's geothermal facility can implement the
same material view/participant rather than requiring an Industry pointer. Actual
utility-token integration must wait for r2mlvs12's tested `UtilityGrantID`, epoch
and Consume/ReleaseUnused/TransferLayer mutation handoff. Its plan explicitly
assigns those native exports to Groups 7-9; the header is absent. No copied
`UtilityPowerAccount`, invented token or second allocator bypasses that gate.
Grant prepare/cancel/atomic consumption and real save/retry acceptance remain
unchecked in Group 13. No scheduling, plant, heat allocation or orbital identity
is created by this documentation. Group 13 did not compact.

## Group 14: separate accounting and scoped missing integrations

Geology remains the exclusive finite reserve owner, mine output becomes real
station packets only by exact receipt, and processors distinguish accepted stock
from recorded consumption/products. Heat/power never use the reserve debit or
freight-payment paths. The native Surface Lithium/Nickel/Cobalt/Rare Earth chains
exist, but the required Uranium useful-power chain is pending. `src/transport_tube.h`
is absent, so tube/refit transfer is neither implemented nor claimed; no substitute
tube inventory exists. Group 14 did not compact and adds no runtime mutation.

## Group 15 - Actual status and control windows

Mine status derives from real construction, calendar, company, pause, reserve,
output capacity and same-layer station admission. The information window shows
resource, remaining reserve, extraction/supply totals, finite storage and
machinery/loading/pickup requirements. Processor input stores remain distinct
from selected per-batch quantities; all four Metal-family inputs are visible.
Viewing/painting consumes neither work nor material.

Focused mine admission/definition/production/dispatch/status cases passed
2,477 assertions in 19 cases (r2mlvs6-cb-presentation-lifecycle-checks.log).
Actual mine, four-input refinery and portal click/paint cases passed 81
assertions in three cases (r2mlvs6-co-window-paint-checks.log). The shared test
font now supplies non-null glyph metadata and mock sprites have valid positive
dimensions, satisfying the real painter's contracts before empty clipping.
These tests claim control layout/refresh only, not raster-art acceptance.
All ten mine and seven receiver art families remain planned; the native
fleet/player-toolbar gate still blocks the combined driven-route workflow.

Group 15 compacted once. Its global and nearest local reading was rechecked.
Group 16 is split into mine bindings, processor counters and lifecycle recovery
so geological claims, delivered stocks and transport packets keep their owners.

## Group 16 - Delivered finite mine persistence

Save version 379 appends mandatory LMNH/LMNR transport and metadata feature 256.
Load/preview check exact family presence and counts; preview reset clears its
independent mine fields between files. Final activation validates actual native
definition/resource/layer, bounded excavated machinery touching the seam,
geological claimant, active/depleted/abandoned phase, rational/rate work bounds,
revision and extracted-minus-dispatched equals actual output. A prior rebuilt
mine's geological debit remains part of initial-minus-remaining reserve; opening
or restoring a new mine never renews that finite balance.

Construction/partial/exhausted bindings restore through the public owner without
material mutation. Focused save/restoration tests passed 495 assertions in three
cases (r2mlvs6-df-mine-save-checks.log); the later machinery/contact validation
passed 403 assertions in two focused owner cases (r2mlvs6-dq-mine-geometry-load-checks.log).
Actual game reload/save preserved LMNH/LMNR/GDEP/GSPN bytes for all three states
(r2mlvs6-dk-mine-game-roundtrips.log). Missing header, mismatched count and invalid
company refused with no output save (r2mlvs6-do-mine-refusals.log). Source hashes
and the original startup script are retained. No utility grant or station ticket
is serialized. This group did not compact.

## Group 16B - Delivered processor persistence

Version 380 appends LPCH/LPCR/LPCC and metadata feature 512. Staging owns only
choice/work/counters; physical stock remains in Industry and existing packet
chunks. Final validation requires exact frozen family/role keys, checked batch
arithmetic, actual input/output unions, finite stock, valid control/work/revision
and product history minus actual dispatch equals stored products. Explicit zero
keys preserve the shared executor's non-allocating commit contract.

Focused restoration/malformed/save/carrier cases passed 2,131 assertions in three
cases (r2mlvs6-eh-processor-carrier-checks.log). Final actual reloads now pass and
16.2 is checked. The first actual load refused CORE input
as an invalid label: the shared unit fixture intentionally packs required labels
tightly, whereas build_content.py preserves original slots 0..11 and starts custom
labels at 12. The required baseline list has only eleven entries (unused Temperate
Food is omitted). Ordinary Industry/Vehicle saves store runtime cargo IDs, so full
disk fixtures now explicitly request the distributed layout; compact unit fixtures
remain useful to prove label-based gameplay. This is fixture setup, not a new
gameplay cargo registry or a runtime cargo migration.

The full-save fixture uses actual command-funded factories and actual batches
to fill output; exact supply then StationCargoList::Load places produced RFMT in
a real original-engine wagon. It makes no native-fleet purchase/refit/driven-route
passing claim. Actual packet bytes and a stable-label stock/choice/history digest
were compared through the complete loader. Missing-input/full-output checkpoints
retained the stock/choice/history digest and side/packet bytes
(r2mlvs6-el-processor-game-roundtrips.log; its earlier transit attempt failed).
The final wagon checkpoint passed 54 focused assertions
(r2mlvs6-er-carrier-save-checks.log), then actual game reload/save retained its
digest and all packet bytes (r2mlvs6-es-processor-carrier-roundtrip.log).
Missing header, foreign choice and forged product counters all refused without
an output save. Bare full-world fixtures also require the ordinary dense engine
pool and a valid station naming StringID; normal gameplay already provides both.
Source hashes and the startup script are preserved. This group did not compact.

## Group 16C - Delivered lifecycle acceptance

Company removal exposed stranded mine output: invalid company ownership stopped
both extraction and the only exact dispatch path, while demolition correctly
refused nonempty stock. Abandoned mines now retain stopped extraction and allow
existing output through actual eligible same-layer stations; empty machinery
can then be removed/rebuilt against the unchanged remaining geological reserve.
Exhausted mine revision stamps saturate across company changes instead of wrapping.
An abandoned processor's first valid recipe-selection actor can recover control
without changing any input/output stock; estimates and deity actions never claim
control. Deity/editor founding history is preserved while its recipe controller
is normalized to no company. Registration is idempotent so repeated creation
hooks cannot reset saved control, work or historical counters.

Actual Surface/Deep/Space viewport selection leaves Deep extraction at four units
per 74 real economy ticks. Deep tile loops create no Surface effects/vehicles,
and a same-XY Surface station receives no native stock. Explicit stopped output
remains protected against demolition; abandoned stock dispatches to a successor's
real same-layer station, then command demolition/rebuild preserves original and
remaining reserve plus geological packed state and the existing station packets.
Recipe-control estimates are pure; actual recovery retains all delivered
alternatives and deity never becomes a saved company controller.

Final focused lifecycle, relevant processor/exact-dispatch and real window cases
passed 1,263 assertions across 16 cases (r2mlvs6-ey-industry-recovery-checks.log).
The final abandoned-pickup path also retains any surviving script-selected
transport exclusivity; releasing a mining claim grants no bypass of that
independent restriction. Its focused lifecycle recheck passed 180 assertions
in five cases (r2mlvs6-fd-recovery-exclusivity-checks.log).
No full regression was run. This group did not compact. The earlier
r2mlvs6-eu-industry-lifecycle-checks.log ran an old binary after a failed fixture
build and contains no tests; it is not validation evidence.

## Group 17 - Handoff and explicit incomplete acceptance

The main plan now names delivered public exports and their sole physical/state
owners, lists focused commands/evidence, and retains each grid/fleet/tube/art
gate. Industry construction, cargo payment and savegame architecture describe
native admission, exact receipts, preserved travel encoding, staged families
and recovery. The parent is not reconciled as complete.

Raw Surface-chain inputs and final material outputs are recorded; the reduced
fixture is zero-priced and does not establish procurement, income or driven
transit costs. Useful Coal/Uranium power, typed heat grants, native conventional
freight service/portal/refit models, tubes and final assets remain missing.
All ten mine and seven processor asset source records are still planned, with
no path/packed delivery. Full financial/all-resource/network acceptance is
unfinished; corresponding tasks remain unchecked.

Final Debug/RelWithDebInfo GUI builds passed and paired their language snapshots
(r2mlvs6-ez-debug-game-build.log, r2mlvs6-fa-release-game-build.log). The final
exclusivity change rebuilt both again (r2mlvs6-fc-recovery-exclusivity-build.log,
r2mlvs6-fe-release-recovery-build.log). Actual fresh-game startup with the real
build/play.cfg and hidden null media exited zero for both configurations
(r2mlvs6-fb-Debug-startup-err.log and r2mlvs6-fb-RelWithDebInfo-startup-err.log).
These are startup checks, not final raster rendering or driven-route acceptance.
No supplied saves were overwritten and no full regression was run.

## Paused portal crash investigation

Mine implementation was paused at Group 6 on 2026-10-06. Opening a real newly
built sealed portal reproduced the original crash through `ClickTile`,
`ShowLayerPortalWindow`, `Window::InitNested` and `NWidgetLeaf` construction.
The status and corridor widgets declared `WWT_TEXT` with `Colours::DarkGreen`.
The widget owner requires `Colours::Invalid` for plain text and throws
`WWT_TEXT should not have a colour` otherwise. Both widgets now inherit the
enclosing panel's background using the required colour sentinel.

`src/tests/portal_window.cpp` builds a real native sealed pair and clicks both
ends. It verifies window construction, positive dimensions, control refresh
and reuse of the canonical pair's single window. Real generated English
strings participate in sizing; an empty drawing clip avoids a raster-asset
dependency. `[portal-window]` passed 14 assertions in one case, recorded in
`r2mlvs6-z-portal-window-checks.log`. Debug game/test and RelWithDebInfo game
builds passed; `r2mlvs6-z-portal-window-build.log` and
`r2mlvs6-z-portal-game-release-build.log` record the rebuilds.

The separate startup language error followed the mine test build regenerating
shared `build/lang` packs while playable binaries still held the previous
string-table version. Rebuilding both playable configurations synchronized
them with `build/generated/table/strings.h`; generated English uses version
`0x0A21A2BC`. A diagnostic config placed beside this plan incorrectly redirected
the game's working/config search directory here and caused additional language
popups. That diagnostic config was removed. Startup probes using the real
`build/play.cfg` passed in both configurations, including the actual launcher
GUI executable and a fresh Multilayer Industry game with null video/sound/music.
This startup check does not exercise raster rendering or TBM commissioning.

The crash repair completed no additional mine-plan tasks. At that checkpoint
Group 6 was unchecked and the mine plan was paused. The subsequent explicit
user request resumed the mine plan; Group 6's validation is recorded above.

## Reading notes for continuation

Global anchors: AD-7, `MultiLayerMap` ownership and `industry_cmd.h` commands.
Local anchors for Groups 1-6: actual public producer definitions and the named
industry lifecycle functions. Most plan line ranges have drifted; relocate
symbols. Town projection, plane distance and layer-bounded area iteration already
work; do not reread or replace those owners without contrary live evidence.
