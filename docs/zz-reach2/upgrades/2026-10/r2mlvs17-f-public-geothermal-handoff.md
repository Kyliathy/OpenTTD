# Public geothermal producer handoff

Owner: [Plan17 Group7](r2mlvs17-geothermal-energy-and-industry.md#group-7---publish-heat-allocation-and-electrical-export-apis), under [master10.3](r2mlvs3mp-master-plan.md). Architecture: [map layers](../../architecture/map-layers/archi-map-layers.md). Delivered feature evidence: [electrical export](r2mlvs17-d-power-export-notes.md), [shared403 power contract](portal-tbm/r2mlvs3mp-x-useful-power-handoff.md#current-paid-path-extension). Later operating acceptance: [first-plant specification](r2mlvs17-e-first-plant-bootstrap.md).

## Current execution

Tasks7.1-7.3 areX; Group7 and master10.3 are complete, with master10.4 selected ground asset integration next. Both game/test configurations build and each passes11,510 focused Heat/physical/export/recipe assertions/20 cases. Optimized writer `20261010-051628-29400cace1` and Debug writer `20261010-052245-c623495029` retain all twelve original5.4 digests/thirty-five authority chunks. [Final verification](../../../../logs/geothermal/public-api/20261010-handoff-01/verification.json) binds67 source identities, four binaries, eight unchanged retained inputs and both actual native writer reports. All build/native handles are terminal; no process is live. After compaction follow the master current cursor and the actual executing child's Global/Local reading. This completed handoff's public headers remain the authority below; do not restart historical Group5/native batches.

## One callable facade

Callers include [geothermal.h](../../../../src/geothermal.h). It supplies the complete `GeothermalHeatAllocation` and `GeothermalUtilityLink` schemas and callable `PrepareVentHeatAllocation` / `AllocateVentHeat`, `ConsumeGeothermalHeat`, `PrepareGeothermalPowerExport` / `ExportGeothermalPower`, alongside the retained site/claim API. Specialist declarations stay in their existing heat/utility headers and are included by the facade; implementations/registries remain with their existing source owners.

`geothermal_type.h` holds the unchanged `GeothermalError` enum so `geothermal_utility.h` no longer needs the facade. This preserves dependency direction: the facade includes public specialists, and specialists include their narrow types/shared utility dependency. A signature fixture in `geothermal_power_export.cpp` checks complete public schemas and exact function-pointer types **before** any specialist/native test fixture is included. The fixture therefore detects a facade that works only through incidental includes.

## Identity, units and ownership

| Public quantity/reference | Owner and meaning |
| --- | --- |
| Natural `GeothermalVentField.id` | Geology2 retains the original renewable rate-limited source; claims/collectors do not clone it. |
| `GeothermalSiteID` / `GeothermalSiteClaim` | Plan17 cold-site owner retains original natural source, company and explicit excavated geometry. A claim is no paid building or energy offer. |
| Collector/riser `IndustryID` | Native Industry owns machinery and actual accepted stock/history; feature records hold construction receipts/physical references. |
| `UtilityProducerID`, `UtilityConsumerID`, `UtilityLinkID` | Common native utility owner issues identities. Source/consumer domain, role and explicit layer remain immutable. Installed link views create no private counters/capacity. |
| `UtilityGrantID` | Exact current shared consumer/epoch. Domain comes from the actual native participant; a Heat action cannot spend electricity with a matching numeric ID. |
| `GeothermalUtilityLink.capacity_w` | Actual installed rate in watts; no stored/generated joules. Heat stays Deep-local; installed electricity can relay through paid original risers. |
| `GeothermalHeatAllocation` | Transient complete feasible demand/path/source proposal with exact epoch, ledger sequence and topology revision. Its shares are proposals; only the common owner publishes reservations. |
| `GeothermalPowerExportRequest` / proposal | Original grant, explicit installed input, final requested consumer, joule bound and selected complete physical references. The proposal aliases the shared sealed DTO and creates no second live balance. |

Profile1 owns immutable definitions/material/rate terms; geology2 owns natural fields; Plan17 owns actual physical sources/sites/construction/path eligibility and consumer integration. Plan12's shared native owner retains both domains' generation, allocations, consumed/released history, interval/carry, original parent slices and operation sequence. Allocation never uses selected viewport, catchment or coincident coordinates as a connection.

Generation integrates actual watts over synchronized27-ms ticks with saved0..999 millijoule carry. Demand/grant/use/release/export are whole joules in the exact saved epoch; technology unlocks use the calendar date. Physical path quotes cannot substitute a caller duration for an original source publication. Heat-to-electric conversion and its carry will belong to the actual operating generator transaction; this handoff does not implement it.

## Prepare, allocate, consume and cancel

For Heat, register the actual installed consumer port through `RegisterGeothermalHeatConsumer`, retaining its immutable direct-recipe/generator role. Its live `GeothermalHeatParticipant` reports `EnergyDomain()==Heat`, validates complete feasible demand without mutation and supplies the whole actual benefit transaction at consume. `GeothermalHeatDemand` holds this live callback and exact native request; callbacks are transient and never saved as work.

The synchronized system actor prepares/allocates all feasible requests for the open shared epoch through `PrepareVentHeatAllocation` / `AllocateVentHeat`. Stable ConsumerID/source order and actual installed paths bound one original source budget; no private fair cursor or per-building copied vent exists. A missing, infeasible or wrong-role whole participant receives zero feasible demand; repeated/guessed epochs and invalid native references refuse whole admission. `ConsumeGeothermalHeat(grant, quantity_j, participant)` revalidates native Heat custody and original physical routes, then commits actual work and source use together through the common mutation owner.

The caller that cancels unperformed work releases only its actual unused Heat grant through `PrepareUtilityHeatMutation(ReleaseUnused, grant, quantity_j)` and `CommitUtilityMutation`. It does not refund consumed heat, revive removed paths, reset epoch/carry or invent another source publication. Unused electrical grants similarly use `PrepareUtilityMutation(ReleaseUnused, ...)`. Cold-site cancellation uses `CancelGeothermalSite` and preserves natural vent/terrain/history; actual funded machinery/stock removal belongs to its native lifecycle owner.

For electricity, `PrepareGeothermalPowerExport` selects one actual installed paid path and asks the common owner for the bounded admissible whole action. `ExportGeothermalPower` commits that exact sealed action once; source debit, all newly traversed physical allowance and final availability publish in one shared operation. Empty-demand intermediate accounts acquire no grant. Imported continuation retains its original producer/parent and actual arriving housing. Consumed or released rights never restore a spent physical allowance. Modified/stale/replayed proposals refuse unchanged; a new preparation is a distinct requested action.

Portal/cell/source/company loss expires affected unused rights through their original shared owner, retaining actual consumed history and spent flow. Repaired/reconnected infrastructure receives fresh references and does not revive released promises. Current403 path/parent transport and independent/native acceptance remain the existing shared producer; changing public header reachability needs no save-version change.

## Shared recipe participant

Plan6 callers use `PrepareLayerRecipeBatch(recipe, inventory)` with the actual `LayerRecipeInventoryView`, then `CommitLayerRecipeBatch(recipe, inventory, participant, proposal)`. `LayerRecipeCommitParticipant::Inventory()` must return that same physical owner; `GetElectricalGrant` exposes only its actual consumer/epoch. `Validate` rechecks the entire stock/work/output action without mutation, and `Commit` either applies the complete action/owner revision or preserves every quantity on refusal.

The recipe owner recomputes all deltas before settlement. Positive electrical use is prepared with the real grant and commits through `CommitUtilityMutation` with the actual recipe transaction participant. Missing input, full output, unavailable grant, wrong layer/owner or stale revision cannot consume only one resource. Cancellation releases the original unused grant under its caller's policy; no recipe invents utility IDs, grants or source quantities.

Actual direct-heat recipe commits still refuse positive scalar heat. [Plan6 Group13B](r2mlvs6-mines-and-resource-chains.md#group-13b---integrate-the-actual-thermal-owner) must integrate whole Heat/electric/material custody before those recipes run. Generator conversion, later smelter/battery/forge definitions and paid Surface procurement13C remain their named tasks. A public Heat participant makes these integrations callable; it does not complete them.

## Verification boundary

Existing Group5 checks prove actual shared capacity/export/refusals on paid native risers and both-configuration native retry/reload. Group6 verifies the frozen supply graph and specifies the later real first build. This group's both-configuration build/signature and focused Heat/physical/export/recipe checks verify the facade itself; the actual native writers remain exact against completed Group5. Operating generation and conventionally transported first-build proof remain master10.6; no feedback or pruning gate is waived.
