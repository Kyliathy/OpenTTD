# R2MLVS6 - Mines and Resource Chains

Status: proposed. Parent: [R2MLVS master](r2mlvs-multilayer-vehicles-and-structures.md). Depends on r2mlvs2/5 and the r2mlvs1 content contract.

## Outcome and reading

Make all ten proposed mine resources productive and transportable to useful Surface consumers. Deep mines obey the 1980 calendar unlock; r2mlvs17 adds geothermal power and later direct-heat receivers without replacing the five Deep mineral types or removing their Surface receivers. Read master AD-4/7/16 and its Resource and content catalogue; [industry architecture](../../architecture/industries/archi-industries.md), [industry construction](../../architecture/industries/archi-industry-construction.md), [link graph](../../architecture/linkgraph/archi-linkgraph.md), `src/industry_cmd.cpp`, `src/newgrf_industries.cpp`, station catchment and industry saveload.

## Group 1 - Layer-aware industry lifecycle

- [ ] 1.1 Audit Surface assumptions in construction, generation, `TileLoop_Industry`, climate/water behavior, effects, proximity callbacks, town association, closure, news, industry lists and scripts. Record each change; removing the Surface assertion alone is insufficient.
- [ ] 1.2 Add a layer/capability whitelist for new mine and processor definitions. Ordinary NewGRF industries remain Surface unless explicitly supported; fallback/substitute graphics do not grant placement permission.
- [ ] 1.3 Implement mine funding only on fully excavated footprints touching a suitable unclaimed deposit. Validate access/loading footprint, ownership/claim, reserve availability and full cost before construction.
- [ ] 1.4 Bind mine and deposit with stable IDs. Define depletion, closure, abandonment and rebuilding so rebuilding cannot reset the original reserve or claim the same seam twice.
- [ ] 1.5 Keep station acceptance/catchment and cargo generation within the industry's own layer. Provide explicit layer names and navigation in lists, news and vehicle orders.
- [ ] 1.6 Apply the shared Deep calendar availability rule to funding, automatic construction, activation, production and script entry points. Scenario/editor authoring must not silently enable operating Deep industries before 1980 in normal new-profile play.

## Group 2 - Resource production and consumers

- [ ] 2.1 Implement Coal, Iron Ore, Copper Ore, Bauxite and Salt mines in Underground, with distinct cargo identities, buildings and readable overlays.
- [ ] 2.2 Implement Lithium Ore, Nickel Ore, Cobalt Ore, Rare Earth concentrate and Uranium mines in Deep Underground. Enforce the intended equipment/depth capability and harder access costs.
- [ ] 2.3 Implement bounded extraction: decrement reserve only when output enters accountable inventory; full buffers pause production. Define supply/service modifiers without making mandatory transport impossible at startup.
- [ ] 2.4 Supply Surface metal, chemical, battery, electronics and component works plus nuclear/useful-power acceptance. Use compatible existing Coal/Iron receivers where available. Recipes follow r2mlvs1 and fit engine cargo limits.
- [ ] 2.5 Add demand, payment/refit and CargoDist integration. Origin/destination distances use plane coordinates and actual cross-layer connectivity; no artificial stacked-Y payment bonus or cross-layer station catchment.
- [ ] 2.6 Connect products to launch fuel, construction components, Robotics Parts and Solar Mirror Kits. Surface manufacture supplies the first orbital facility without requiring an orbital predecessor.
- [ ] 2.7 Expose the shared industry/recipe interfaces needed by r2mlvs17's 2010 smelter, 2030 battery works and 2045 Mantle Forge. Keep mineral reserve extraction, direct heat use, transported inputs and produced output separately accountable; all five Deep minerals retain an earlier useful Surface receiver.
- [ ] 2.8 Integrate tube terminal cargo/refits with r2mlvs16. Mine/factory freight may transfer to tubes on its own layer and back to conventional transport for inter-layer movement; tubes neither replace mandatory TBM spoil-wagon hauling nor open a pre-1980 Deep route.

## Group 3 - Presentation and acceptance

- [ ] 3.1 Show resource, remaining reserve, production, output storage, supply requirements and depletion status. Integrate ten identifiable mine variants and Surface receiver graphics from r2mlvs13.
- [ ] 3.2 Test each mine's allowed/refused layer and footprint, duplicate claims, depletion, demolition/rebuild, full output, no nearby station and same-XY stations on other layers.
- [ ] 3.3 Operate all ten resources through trains/trucks and portals/elevators to Surface receivers, verifying actual acceptance and resulting products. At least one Deep chain must use two rail transitions and another the road elevator.
- [ ] 3.4 Save/reload during construction, production, depletion and cargo in transit; verify reserve + produced/consumed totals and no Surface-only assertion/effect leakage.
- [ ] 3.5 Record complete chain quantities, income, transit costs and bottlenecks for balancing. Missing consumers or invisible placeholder industries prevent completion.

## Completion conditions

There are five distinct usable resources on each rock layer. Each has a built mine, a compatible vehicle, a genuine cross-layer route and a Surface consumer with visible economic effect. Generation, production and rebuilding cannot duplicate deposits or leak cargo through vertical catchment.
