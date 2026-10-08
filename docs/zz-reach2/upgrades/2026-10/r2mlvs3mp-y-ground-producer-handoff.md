# Ground material and conventional fleet producer handoff

Owner: [master 9.2-9.4](r2mlvs3mp-master-plan.md#group-9---supply-the-later-integrations-shared-producers). Producers: [Plan 6](r2mlvs6-mines-and-resource-chains.md), [Plan 7](r2mlvs7-underground-construction-and-fleet.md), [Useful Power](r2mlvs3mp-x-useful-power-handoff.md). Architecture: [map layers](../../architecture/map-layers/archi-map-layers.md).

## Selected first-generator material scope

The immutable `GetMultiLayerBootstrapMilestones()` Geothermal row requires **24 Steel, 16 Refined Metals, 8 Electronics and 4 Batteries**, plus capital480,000 and prior Surface/Deep access. This handoff verifies actual material producers and the shared electrical recipe participant. Master10.3/10.6 retain paid procurement, physical plant/riser construction and driven conventional delivery; a material fixture cannot complete an operating generator.

`layer_surface_manufacturing.cpp` now funds actual Surface refinery, chemical, battery and electronics works plus the compatible original steelworks. Explicit earlier raw shipments produce exactly the complete frozen bill. Chemical and Refined Metals products pass through real source stations, carrier packets and final receiver admission before becoming Batteries/Electronics. The sole physical Industry stocks and station packets retain the resulting materials. No geothermal/tube/orbital output seeds production. This is manufacturing/custody evidence; initial procurement cost and driven route operation remain later acceptance.

| Required stock | Selected real producer | Earlier inputs |
| --- | --- | --- |
| 24 Steel | Compatible original Steel receiver | 24 Iron Ore; original one-to-one conversion |
| 16 Refined Metals | `CFMT` Surface metal refinery | 48 Copper Ore +12 Coal yields24;8 Refined Metals goes into Electronics |
| 8 Electronics | `ELEC` Surface electronics works | 8 Refined Metals +4 Rare Earth concentrate |
| 4 Batteries | `BATT` Surface battery works | 4 Lithium Ore +2 Cobalt Ore +2 Chemicals |
| 2 Chemicals used above | `CHEM` Surface chemical works | 2 Salt +2 Oil |

Selected Plan13 art families for master10.4 are `mine.COAL`, `mine.IORE`, `mine.CORE`, `mine.SALT`, `mine.LITH`, `mine.COBT`, `mine.RARE` and `facility.metal-refinery`, `facility.chemical-works`, `facility.battery-works`, `facility.electronics-works`. Mines retain their actual3x3 machinery layout and processors4x4; inspect exact tile identities before authoring. The original Steel receiver/Oil supplier keep reviewed existing base art. Reuse retained original graphics and atlas components where geometry permits. Unselected Bauxite/Nickel/Uranium receivers and component/robotics/mirror-kit manufacture are beyond this first-generator material scope. Nuclear/Coal useful-power receiver tasks remain pending in Plan6; this bootstrap does not require those sources.

The content contract also publishes earlier paid Surface Electronics/Batteries SKUs for the 1970 tube bootstrap. These definitions are not a delivered runtime procurement command. Its actual paid owner must be implemented with Plan16/17's first-build/service work; it cannot be replaced with free station inventory.

## Shared recipe transaction

`LayerRecipeCommitParticipant::GetElectricalGrant` returns a real `UtilityGrantID` for its exact inventory owner; the default refuses. A positive complete electrical batch revalidates the chosen recipe, actual physical layer, all inputs, every output, work/revision and current ledger proposal. `CommitUtilityMutation` joins the existing whole material participant to original-source settlement. Scalar quantities grant no custody; direct heat still refuses until Plan17 supplies its separate actual thermal participant.

Cancellation discards the transient recipe quote, then its calling job uses `ReleaseUnused` to expire its unused electrical reservation. Preparing a quote creates no reservation. Missing transported input, a full product slot, owner refusal or expired grant consumes no stock/work/energy. Cancellation changes ledger availability, so an earlier quote and release ticket cannot be replayed. Successful batches advance the material owner's revision and the saved utility operation sequence.

[Focused supply/recipe/baseline checks](../../../../logs/r2mlvs3mp-y-ground-supply-checks.log) pass **1,489 assertions /16 cases**: exact first-generator bill, real factory/packet custody, every material alternative, original Steel reuse, electrical whole-owner failure/success, zero-batch preservation and explicit cancellation/retry. [Build](../../../../logs/r2mlvs3mp-y-ground-supply-build-1.log) passes. Commands use `[layer-surface-geothermal-supply],[utility-power-recipe],[layer-recipe-batch],[layer-processor],[layer-baseline-receiver],[layer-surface-manufacturing],[multilayer-bootstrap]`. No full regression was run.

## Conventional fleet continuation

Plan7's existing public toolbar/filter, ordinary station/depot, native freight models and command guards pass [5,855 assertions /18 focused cases](../../../../logs/r2mlvs3mp-y-conventional-producer-checks-1.log). Run actual UI fixtures from `build/`, where their generated English language pack lives. The earlier repository-root invocation refused that language prerequisite; it is diagnostic evidence only.

Selected conventional art is `vehicle.model-05` mineral wagon, `vehicle.model-06` sealed mineral wagon, `vehicle.model-07` mine truck and `vehicle.model-08` sealed road carrier, with matching `purchase.model-*` previews. Native model4 is the already delivered hauler/ordinary freight locomotive and reuses its exact M1 body/purchase family. Plan13 Group19 must reconcile that delivered reuse before generating another locomotive. Selected facilities are `facility.rail-station` and `facility.road-terminal`; rail/road service depots reuse their delivered M1 families. Read actual per-cell/approach geometry before freezing the old draft rectangles. Tube/geothermal controls remain pending until their actual providers arrive, and final selected fleet/station/terminal art remains master10.4.
