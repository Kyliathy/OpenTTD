# R2MLVS3MP-I - Spoil receiver design (master 5.8, Plan 5 Groups 8-9)

Owner: [Plan 5](r2mlvs5-spoil-wagons-and-haulage.md) Groups 8-9. Recipes: [r2mlvs1-d Group 7](r2mlvs1-d-recipes-and-bootstrap.md#group-7---mines-processors-and-bounded-receivers). Exchange and cycle: [r2mlvs3mp-h](r2mlvs3mp-h-exchange-design.md).

## What a receiver is

The Earth reclamation site and the Rock crusher are real industries in a station's catchment, as AD-6 requires ("final disposal can occur within its catchment"). They are **native first-party industry types**, installed the way Plan 4 installs its TBM engines:

- `InitialiseSpoilReceiverDefinitions` runs after GRF polishing, beside `InitialiseTBMEngineDefinitions`, and only when the exact first-party package is active.
- Both types use the package's GRF ID with reserved local IDs (industries `0x8000` Earth, `0x8001` Rock; tiles from `0x8000`). The saved IIDS/TIDS mappings carry their identity across reloads.
- The package file is unchanged. A new package would change its md5, and `CheckMultiLayerContentIdentity` refuses a changed package, so every retained multilayer save would stop loading.
- Placeholder art: each layout copies an original layout (coal mine for Earth, iron ore mine for Rock). Every distinct tile becomes a native tile spec that draws its original substitute and accepts the receiver's spoil at 8/8. Final art is Plan 13 Groups 37-38.
- Accept-only "black hole" industries in Temperate; never generated at map creation or at random; fundable by a company. Industries are already Surface-only (`CheckLayerAllowsConstruction`), so a below-ground sink cannot exist (8.4).

## Frozen recipe (r2mlvs1-d Group 7)

| Receiver | Batch | Stock cap | Intake per 74-tick interval | Fee | Outcome |
| --- | --- | --- | --- | --- | --- |
| Earth reclamation | ERTH 8 t | 1,024 t | 32 t | 2 Money/t at intake | 8 t disposed |
| Rock crusher, crushing | ROCK 8 t | 1,024 t; aggregate 1,024 t | 64 t | 3 Money/t at intake | 6 t aggregate + 2 t residue |
| Rock crusher, surplus disposal | ROCK 8 t | 1,024 t | the same 64 t | 4 Money/t at intake | 8 t disposed |

Interpretation recorded here: each interval processes up to the row's interval amount (Earth 4 batches, Rock 8 batches), so a working receiver keeps pace with its intake. Fees settle once, at intake, at the mode current then; no freight income (ERTH/ROCK pay 0) and no subsidy. Full aggregate pauses crushing; the founding company may switch the crusher to surplus disposal (`CmdSetSpoilReceiverMode`).

## Bounded intake (8.3)

- Stock is the industry's own `accepted[].waiting`. Extra saved state is a side record per receiver industry: kind, mode, interval timer, interval intake, aggregate, totals and revision.
- **Room** = min(stock cap - stock, interval intake left).
- `LoadUnloadVehicle`: before `Unload`, spoil marked Deliver beyond the station's total receiver room is reassigned to Transfer (Unload/Transfer orders) or Keep. This uses the same paths the not-accepted case uses.
- `DeliverGoodsToIndustry`: a receiver admits at most its room and charges the fee to the delivering company. Both steps use the same room function in the same tick, so `CargoDelivery` never removes more than the receivers admitted.
- `TriggerIndustryProduction` leaves receiver stock alone, so the default black-hole clearing never applies. `OnTick_Industry` advances the interval.

## Surface origin leg (8.1, 8.4)

`PrepareUnload` stages no spoil from a wagon whose leg is incomplete, so that cargo stays Keep:
- any wagon of a cutter consist;
- a set wagon not aboard the hauler that carries its set;
- a set that is not Loaded;
- a Loaded set before it has visited the project's origin, unless the station is that origin.

Spoil in other vehicles has already been transferred onward from an origin and moves freely. `CmdSetTBMProjectOrigin` (Plan 4 owner) replaces the origin with another company Surface railway station. Sets that already visited the old origin keep their visit.

## Persistence

`SaveLoadVersion::SpoilReceivers` adds `SPRH` (header) and `SPRR` (rows). Rows are staged on load and accepted after `AfterLoadGame` rebuilds industries. Every receiver industry needs exactly one row with in-range fields. `MULTILAYER_FEATURE_SPOIL_RECEIVERS` marks a save that has receivers, and the canonical diagnostic record uses the reserved `IndustryInventory` kind.

## Not here

Fees and stall reasons are published by Group 9, which also pauses excavation through the controller. Aggregate consumption by ground foundations belongs to a later construction owner. Receiver UI beyond the ordinary industry window is outside this group.
