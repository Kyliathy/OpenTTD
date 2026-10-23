> **Historical donor snapshot — not an execution plan.** OpenTTD3 `75ea75718f`, imported2026-10-23.
> Original: `docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md`; exact source SHA256 `ed8e76841825b8984628bba1935529479485fb654799ad7141c3618764531e83`.
> Current integration: [M2O23-1](../../../../../../../zz-reach2/upgrades/2026-10/m2o23-1-come-space.md); storage and evidence roles: [archi-historical-debug.md](../../../../../../../zz-reach2/architecture/archi-historical-debug.md). Original completion claims, instructions and human-review limits below remain dated; they grant no new execution, spending, publication or deletion authority.

# R2MLVS17 - Underground Transport Tubes

Owner: [R2MLVS parent](r2mlvs-multilayer-vehicles-and-structures.md). Dispatch: [master10.8](r2mlvs3mp-master-plan.md). Architecture: [map layers](../../architecture/map-layers/archi-map-layers.md).

## Global Mandatory Reading

> **Execution protocol**
>
> - Read this Global Mandatory Reading section before the first task and again
>   after every context compaction.
> - Before starting or resuming a task group, read the nearest preceding Area
>   Mandatory Reading block whose `Applies to` label includes that group. Area
>   blocks appear immediately before the first group they govern.
> - A referenced heading or exported symbol is authoritative. Global navigation
>   hints and Area line ranges are for navigation only; if they drift, relocate
>   the named section or symbol rather than expanding to the whole file.
> - Mandatory reading is non-transitive. Do not preload a linked file's own
>   Mandatory Reading, Consult on Demand, or Planning Provenance entries unless
>   this plan names them directly for the current area.
> - Do not read Consult on Demand or Planning Provenance merely because those
>   sources are listed. Use a Consult on Demand source only when its trigger
>   occurs.
> - No created source file, test file or JSON file should start with same prefix as a planning MD file (such as r2). Only MD files can have that prefix. No MD file should be created anywhere in source directories unless specifically asked.
> - After EVERY CONTEXT COMPACTION, you must add a note above the task group
>   which is currently in execution. The note will say: "Context has compacted
>   X times while executing this group.". If the compaction occurs before any
>   meaningful work was done (this can happen when there is a lot of baggage
>   from the previous compaction), then and ONLY then you can skip this step.
> - When exceeding 3 context compactions, YOU MUST ask yourself if you're not 
>   chewing more than we can handle :). Heck, plans change! :). In such a case, 
>   the way out is to split the group on the spot,
>   in the plan, into more groups, 
>   by appending B, C, D as a suffixes to those groups (splitting sub-groups 
>   themselves is also allowed). When you do this, create Local Mandatory
>   Reading areas so you can optimize your context only for the
>   ACTUAL task at hand.
>   Before starting or resuming a task group, read the nearest preceding Area
>   Mandatory Reading block whose `Applies to` label includes that group. Area
>   blocks appear immediately before the first group they govern.

| File name | Line Ranges | Description |
| --- | --- | --- |
| [docs/zz-reach2/upgrades/2026-10/r2mlvs-multilayer-vehicles-and-structures.md](r2mlvs-multilayer-vehicles-and-structures.md) | 302-308 | `AD-15` supplies the canonical gameplay/ownership decision for every group of this plan. It constrains five original layer-local capsules, excavated guideways and explicit paid power; a consumer must not redefine it locally. |
| [src/map_layer.h](../../../../../../../../src/tests/fixtures/cleanup/source-epochs/29c86bf77e2c7b62-map_layer.h) | 78-85 | `MultiLayerMap` ownership preamble: `Map` owns packed storage/geometry and this controller owns layer semantics. Every group uses world tiles/explicit layer helpers; camera selection and ad hoc plane offsets cannot define gameplay. |
| [src/rail_type.h](../../../../../../../../src/tests/fixtures/cleanup/source-epochs/openttd3-75ea75718f/eb1fb8c28048bc1e-rail_type.h) | 16-39 | Rail labels, 64 rail slots and `RailTypes` define dedicated tube compatibility; a sprite or purchase filter is insufficient to segregate networks. |

Status: registry/prototype/construction/separation/junction/turning Groups1-4, paid service5, native metering6.1-6.3 and recovery7 are delivered and tested. Read-only public readiness8 is delivered; master10.6 and geothermal6.4 are complete; selected13-15 now execute under master10.7; terminal/interchange Group9 is delivered; five-model definitions10 and admission11 are delivered. Staged12B normal registration/synchronized service commands,12C controls and12D native orders/spoil boundaries complete Group12 and master10.5. Final runtime art/useful-route acceptance remains pending. Parent: [R2MLVS master](r2mlvs-multilayer-vehicles-and-structures.md). Prototype metadata is not the full operational handoff.

Current technical return,2026-10-19:selected13-15 runtime presentation,ordinary files/lifecycle and all five useful model/date routes are complete.16B/16BB/16C/16D now also complete Group16's actual paid TBM-to-tube floor,useful Surface transfer and same-world queue/paid-grid recovery/full file acceptance. The dated status above is historical;Group17's real peer/replay/performance and late comparison obligations,Group18 final reconciliation and human/operator acceptance retain their separate scopes. No whole-plan/M1 completion is inferred.

**Artifact-layout migration (2026-10-09):** this plan consumes the parent [Development artifact layout](r2mlvs-multilayer-vehicles-and-structures.md#development-artifact-layout), implemented by [R2FC](r2fc-file-cleanup.md). It supersedes earlier docs asset/review/helper placement and fixed runtime-config destinations below. New native homes use `mlm/runtime/`, generated feature reviews use `mlm/reviews/`, logs remain under `logs/`, and reusable code/curated inputs keep their code/test owners. Paths and focused native/cold verification are complete; R2FC records the results and retained-input identities. Repository-relative executable commands run from the checkout root through their context-aware wrappers. Historical pass records remain dated evidence and are not new acceptance claims. No gameplay task is completed by this migration and the separate post-demo fixture-pruning gate remains.

## Outcome and Boundaries

Build a distinct underground freight system inside TBM-cleared corridors and chambers. Dedicated powered tubes carry at least five original capsule models between underground terminals; ordinary trains and road vehicles do not use them.

## First-party vehicle catalogue

These five designs are required deliverables, not alternative names for refits. Dates and relative balance are proposed; each gets its own engine/model identity, stats, purchase sprite and eight-direction/load art. Full capacity values and cargo-unit conversion belong to r2mlvs1's tuning tables.

| Model / proposed introduction | Cargo role | Performance identity | Original visual identity |
| --- | --- | --- | --- |
| Mole-70 Bulk Capsule / 1970 | Coal, Iron/Copper/Bauxite/Salt and compatible deep ores; Rock/Earth outside the TBM's mandatory wagon leg | Cheap, large bulk hold, modest speed and high loaded energy use | Low segmented amber capsule with enclosed hopper ribs and visible load indicators |
| Crate-75 Container Capsule / 1975 | Goods, Batteries, Electronics and Solar Mirror Kits | Faster, medium hold; frequent manufacturing deliveries | Square-section green cargo pods with clamped containers and numbered doors |
| Cistern-80 Fluid Capsule / 1980 | Liquid Chemicals/Propellant and other declared compatible liquids | Medium speed, sealed liquid hold; specialized terminal service | Cyan twin-tank capsule with pressure bands, valves and level display |
| Shield-85 Mineral Capsule / 1985 | Uranium and designated sealed mineral loads | Small hold, expensive shielding/service; reliable valuable loads | Short octagonal capsule with thick pale shielding panels and mineral-specific load markings |
| Forge-05 Heavy Capsule / 2005 | Steel, Refined Metals, Station Components and Robotics Parts | Very high payload, low acceleration, long terminal dwell and high power demand | Wide dark machinery capsule with structural braces and a distinct heavy coupling frame |

The content profile defines one physical form per cargo label: Chemicals are liquid for Cistern-80 in this draft; Crate-75 does not refit to the same label as packaged Chemicals. A future packaged form would require an explicit conversion/label decision. Existing trains/trucks carry early Uranium until the Shield-85 arrives. Tube introduction before 1980 does not grant access to Deep Underground.

## Public Handoff Contract

Native launch calendar,2026-10-18 (Group15DCBA): `StartupOneEngine` uses each original capsule's exact1January catalogue launch date; `CalendarEnginesMonthlyLoop` publishes it on that month without an ordinary preview year. [`RestoreTubeEngineIntroduction`](../../../../src/transport_tube.h) reconciles old randomized native definition dates after normal `CopyTempEngineData`; only a date-valid previously pending public introduction changes flags/company availability. Future capsules remain unavailable and original/foreign definitions, engine/vehicle ages/reliability/hidden preferences, maps/cargo/clocks/accounts remain unchanged. The exact native ENGN compatibility comparison accompanies original source-file acceptance; fixture code must not set availability to bypass a native calendar refusal. Current execution evidence remains in15DCBA until both-build acceptance closes.

15CBB useful Crate producer,2026-10-18: actual1975 original Forest/Wood-train/Sawmill production supplies39 ordinary Goods. Conventional freight crosses into Underground, the paid layer-local model10 Crate transfers those original Goods, and separate conventional freight physically returns them to a real Surface town. All three complete whole-consist empty returns; native quarter rollover, source provenance and useful revenue remain with original owners. [The route receipt] (historical reference: `../../../../logs/tubes/crate-route/20261018-native-01/verification.json`) pins both independent routes and current affected producers/four targets. Prepared machinery/native town/floor/commissioned shafts remain explicit inputs; ordinary files/runtime silhouette remain15CC, and no TBM/bootstrap/human/balance claim is made.

15BC/15B closure,2026-10-18: the actual1970 useful Mole route now has ordinary opposite-reader files and native runtime silhouette/picking acceptance. [The file receipt] (historical reference: `../../../../logs/tubes/useful-files/20261018-native-01/mole-files-verification.json`) retains four actual cargo/route states and untouched full-driver baselines; [the raster receipt] (historical reference: `../../../../logs/tubes/useful-files/20261018-native-01/mole-raster-verification.json`) pins current native renderer/affected contracts, three126-chunk cross-reader world pairs and four RGB-exact frame pairs. Original20-unit Coal, paid movement and complete native owners remain intact. Prepared floor/completed machinery/commissioned portals are explicit inputs; CFMT receives fuel without a metal-output claim. Original15.2 retains four later models and the conventional Underground/Deep chain; Group16 owns actual TBM/bootstrap and human/runtime-gallery gates remain separate.

15A refusal acceptance,2026-10-18: actual `VehicleEnterTile_LayerPortal` now rechecks existing `CanTubeTrainEnterTile` for train parts before magazine/visibility handling. Route discovery already refused capsules at conventional shafts, but the native tile callback independently admitted both endpoints; the focused loaded-capsule test reproduces that gap and verifies its correction without relocation or changed packet/source/physical ownership. Solid/locked floors, independent model/Deep dates, mixed domains, protected work support, disconnected paid service and missing native grants all retain refusal. Affected ordinary freight, actual Deep crossings and TBM magazine travel pass. [The refusal handoff](r2mlvs17-e-tube-runtime.md#tube-refusal-and-shaft-entry-handoff) pins exact evidence and disclosed prepared inputs. Original15.1 is complete;15.2 useful introduction-date routes remain pending.

14EC native-file acceptance,2026-10-18: ordinary loading preserves both original competing fronts and distinct packets, complete native reservations and shared grants. One common preparation followed by both native fronts in stable pool order matches the actual uninterrupted queued and Heat-grant-starved opportunities. Repeated/opposite readers retain all126 chunks; first-load and continued owner comparisons retain39 complete authority chunks, with only independently exact native GLOG Load-event timing differing between separately loaded next-step baselines. Original cable-Topology waiting remains distinct compatibility coverage. [The queue file handoff](r2mlvs17-e-tube-runtime.md#queued-and-starved-file-handoff) records original commands/ticks/repair, dated producer identities and current reader evidence. No persistent field, source/packet owner, save version or artwork change. Original14.4/Group14 are complete; useful introduction-date routes, final gallery and actual host/client/human gates remain pending.

Group6BB's native433 source prefix adds [`transport_tube_geothermal.h`](../../../../src/transport_tube_geothermal.h): independently issued `TubeGeothermalSourceID`, pure connect/path queries, scoped root/source/company retirement, copied capture/digest and explicit staged native acceptance. The original plant's shared HeatConversion producer remains the sole generator; source metadata owns only its actual output/root/native-facility references. [Original source handoff](r2mlvs17-b-geothermal-source.md#original-geothermal-tube-source-handoff) defines active/retired load policy and aggregate local-feed ownership. Both-configuration source persistence, native capsule work and explicit provider switching are complete through6BC. Synchronized commands, player controls and current/older switched-file acceptance are complete through6BD. The [native capsule power handoff](r2mlvs17-b-geothermal-source.md#native-geothermal-capsule-power-handoff) closes original6.4 with pinned output identities, real provider switching, exact current/older file evidence and copied-preview isolation. Selected13-15 useful-route/lifecycle/art and actual host/client acceptance retain their later tasks.

Guideway presentation is delivered through13B by [transport_tube_sprite.h](../../../../../../../../src/tests/fixtures/cleanup/source-epochs/openttd3-75ea75718f/2e76a1e7d146ec94-transport_tube_sprite.h): actual flat excavated native track bits select packed aligned shells/end mouths, with pure original-source supply indication and existing native signal/reservation/picking ownership. [The native tube guideway handoff](r2mlvs17-e-tube-runtime.md#native-tube-guideway-handoff) records exact pixel/world evidence and explicit boundary fixtures.13CB adds pure `GetTubeTrainSprite` and `GetTubeEnginePurchaseSprite` through the existing native callbacks: all five actual models retain original packet loads across service/source waits, eight body facings and locked/available previews. [The capsule handoff](r2mlvs17-e-tube-runtime.md#native-capsule-body-and-purchase-handoff) records all720 body/ten purchase poses and visible current-world evidence in both builds. Complete visual/picking/useful routes and runtime-gallery promotion remain13D onward.


13CC publishes pure `GetBuiltTubeTerminal` and `GetTubeFacilityArt`: retained native facility identity, actual default station roles/axes and real joined freight membership select packed service/terminal/interchange parts. Original packets, service demand and source waits own state; current dated `GetTubeTerminal` retains permission authority. Existing native ANIT/m6 registration owns lamp refresh, with no source or inventory tick from rendering. [The native facility handoff](r2mlvs17-e-tube-runtime.md#native-tube-facility-handoff) records exact native scene/pixel/owner evidence and explicit fixture classifications. Full visual/picking and work-file/lifecycle acceptance retain their following groups.

13CD publishes pure `GetTubeConduitArt` over active original external corridors: only consecutive saved cells supply authored ground ports; original current publication supplies status. The derived index is invalidated by existing topology and accepted connection/load/reset owners. No canonical path, quantity or grant is copied or acquired. Native purchased geothermal links retain their separate family. [The native conduit handoff](r2mlvs17-e-tube-runtime.md#native-tube-conduit-handoff) pins full native world/frame/source-pixel evidence.

13DB's real five-model/eight-facing native travel and picking acceptance adds per-part physical curve foreground bounds to `TubeGuidewaySpritePart`. The actual quarter sorts far/side rims at their proper depth without moving source pixels, capsules or quantity owners. [The directional handoff](r2mlvs17-e-tube-runtime.md#native-directional-travel-and-picking-handoff) pins the real paid routes, original native worlds and exact visible/occluded source pixels. Complete joined/facility interaction remains13DC.

Partial public owner: [`src/transport_tube.h`](../../../../src/transport_tube.h) publishes tested registry/prototype queries and `TubeVehicleCapability`, `CanTubeTrainEnterTile`, `CheckTubeConstruction`, `CheckTubeConstructionArea`, `CheckTubeVehicleAdmission` and `CheckTubeRailConversion`. Group5 adds `ConnectTubeExternalPower`, `PublishTubeExternalPower`, `ConsumeTubeExternalPower`, `CloseTubeExternalPower` and saved physical/invoice queries, consuming the existing utility authority. Native metering adds `ComputeTubePowerDemand`, `BindTubePowerVehicle`, `ReconnectTubePowerVehicle`, separate power waits and rebuildable owned connectivity; actual387 bindings preserve admitted demand fractions and once-tick history. Group8 publishes `TubeTerminal`, `GetTubeTerminal`, `CanTubeTerminalServeVehicle` and `GetTubeNetworkReadiness` over actual native station/depot identities, cargo/date admission and stable physically reachable paid inlets. Queries allocate no grants, move no packets and create no invoices. Group10 installs all five distinct native capsule definitions with exact logical refits and native tuning through whole-family EIDS preflight. Model9 remains local0x8009;10..13 append0x800A..D. Group11 shares logical date/layer/refit policy with availability and depot lists, preflights clone/replacement domains/custody, and preserves loaded packets on failed refit or capacity reduction. Normal-world controls remain12. Saved386/387 inlets restore hidden canonical guideway/prototype definitions; new-world controls and full operational fleet/terminal activation remain pending.

Group9 uses ordinary command-built native Station/Depot identities and actual packet transfers for joined tube/rail/road berths. [`transport_tube_stock.h`](../../../../src/transport_tube_stock.h) publishes pure `GetTubeInterchangeTransferRoom`: the exact destination packet list and installed tube membership borrow `EXACT_STATION_SUPPLY_CAPACITY` (65,535 whole units per cargo), including loading reservations. Native packet unloading admits only available transfer room for every mode sharing that owner; full/old over-limit stock is retained, excess stays aboard, and final delivery/payment remains economy-owned. No additional buffer, saved family, source ID or grant is created.

Physical source geometry is separately published by [`transport_tube_model.h`](../../../../src/transport_tube_model.h): `TubeVehicleGeometry` borrows the canonical capability and exposes native rail length/part count through `GetTubeVehicleGeometry`. Staged10B verifies all five descriptors and actual model9 construction. Descriptor availability does not establish an installed engine, purchase/refit/date admission, model balance or operating fleet acceptance; those remain Groups10-12.

Group12B extends that prefix: `InitialiseTubeContentDefinitions` installs exact active native guideway/five-model content in ordinary NewGRF post-polish independently of pending saved tube objects, retaining hidden conventional-rail separation. [`transport_tube_cmd.h`](../../../../src/transport_tube_cmd.h) publishes synchronized `ConnectTubePower`, `CloseTubePower` and `SetTubeVehiclePower` (initial bind or explicit reconnect). `TubePowerCommandPath` transports at most the existing128 cells without length-controlled allocation; oversized requests retain refusal. Existing source APIs own leases/fractional usage, so command cost adds zero instead of charging again. Owner/date/topology/funds and real packet custody remain producer-owned. Current normal-world **definitions** supersede the saved-only restoration statement above;12C controls, full normal-newgame bootstrap and real multiplayer join acceptance remain pending.

Group12C exposes those actions through a fold-out in the existing Underground Construction window, composing `UndergroundTubeControls` from [`transport_tube_gui.h`](../../../../src/transport_tube_gui.h). Dedicated guideway/terminal/depot controls use native construction owners and tube captions; explicit bounded power-route/source/capsule choices call12B synchronized commands. `BuildUndergroundTubeFleetFilter` in [`underground_gui.h`](../../../../src/underground_gui.h) returns actual same-layer owned capsule fronts, and selection opens native vehicle/order/refit views. Five canonical date/cargo/capacity previews are passive. Native current wait/source queries explain unallocated or closed power without advancing work. This completes12.1; original12.2-.4 and full bootstrap/useful-route/visual acceptance retain their own tasks.

Group12D completes native mine/factory order and CargoDist integration through the existing station/packet/payment owners. Actual conventional freight carries the transferred original packets through a portal to a driven Surface factory arrival; empty returns and retries preserve one-time income. Mandatory TBM hauler commands and saved publication reject any actual capsule part in the conventional hauler chain; independent Earth/Rock capsule freight remains valid. This supersedes the earlier pending-controls/order statements. Full autonomous useful-route, normal-newgame/bootstrap and final raster acceptance retain their later dispatch; disclosed local arrivals are boundary inputs. [Both-configuration receipt] (historical reference: `../../../../logs/tubes/ordered-freight/20261016-native-01/verification.json`).

Company lifecycle amendment, 2026-10-18 (14D): after canonical vehicle/shared-account transfer, `ChangeTubePowerCompanyOwnership` retires every original tube service permission while preserving the same surviving front's powerless recovery association and millijoule carry. Existing held on-route reconnect requires an explicit fresh source, never an old grant. The sole shared ownership graph follows a live external binding's exact original source/recipient even after closed-source shares expire, excluding inactive bindings and foreign/invalid roles. Native feed endpoint lineage remains its original owner. Surviving takeover/waits retain actual cargo; terminal INVALID_OWNER liquidation follows native vehicle/packet destruction, retaining consumed/invoice history and denying reused company/vehicle IDs former permissions. No source, quantity owner or format field is added. [The handoff](r2mlvs17-e-tube-runtime.md#company-custody-and-permission-handoff) and [both-build receipt] (historical reference: `../../../../logs/tubes/company-lifecycle/20261018-native-01/verification.json`) record actual repeated ownership, native recovery, terminal loss and producer/file compatibility evidence.

## Implementation Tasks

Deepening pass, 2026-10-04: implementation groups use only SIMPLE, MEDIUM or HARD difficulty, with at most eight pending tasks each. Sketches describe proposed implementation contracts; verify the named earlier producers at each assumption gate. No gameplay or asset implementation is marked complete by this planning pass.

Groups are chronological and contain consecutive tasks of one difficulty. Each difficulty batch has at most eight tasks; area reading changes only when the implementation boundary or source of truth changes.

### Area Mandatory Reading - Tube infrastructure and capability separation

_Applies to Groups 1-4, including1B._

**Entry gate.** r2mlvs1 profile/cargo/date capabilities, r2mlvs2 excavation and r2mlvs3/4/5 safe portal/work-track contracts must be delivered for dependent construction. Dynamic utility implementation is needed only in the following area.
**ASSUMPTION: this was already built previously. If not, re-asses at runtime**. Verify the named prior public exports and their type/behavior fixtures with the current source before dependent work. A declaration alone is insufficient for runtime mutation. If missing, stop the dependent implementation and passing claims in Groups 1-4; independent characterization, test design and source verification may continue. Do not copy private stores, invent substitute IDs/inventory/energy or treat historical planning prose as delivered code.

| File name | Line Ranges | Description |
| --- | --- | --- |
| [src/rail.h](../../../../../../../../src/tests/fixtures/cleanup/source-epochs/openttd3-75ea75718f/6e6501df4baaa8f2-rail.h) | 178-182 | `powered_railtypes` and `compatible_railtypes` have distinct invariants; ordinary/TBM/tube locomotion needs explicit power and compatibility checks. |
| [src/rail.h](../../../../../../../../src/tests/fixtures/cleanup/source-epochs/openttd3-75ea75718f/6e6501df4baaa8f2-rail.h) | 239-257 | Railtype introduction/requirement fields constrain tube availability; the profile calendar unlock remains a shared additional gate. |
| [src/timer/timer_game_calendar.h](../../../../../../../../src/tests/fixtures/cleanup/source-epochs/openttd3-75ea75718f/1c2e911d6fe35ac2-timer_game_calendar.h) | 15-39 | `TimerGameCalendar` technology-clock contract and date conversions govern 1980/model/factory introductions; economic/realtime scheduling cannot unlock a technology. |
| [src/train.h](../../../../../../../../src/tests/fixtures/cleanup/source-epochs/openttd3-75ea75718f/45d43702b9f50718-train.h) | 45-72 | `ConsistChangeFlag`, `FreeTrainTrackReservation`, `TryPathReserve`, `NormalizeTrainVehInDepot` define cache/reservation rebuilding after actual wagon changes. |
| `src/multilayer_content.h` (planned public contract; link after implementation) | Live definitions; no fixed range | [r2mlvs1-gameplay-contracts-and-content.md](../../../../../../../../../OpenTTD3/docs/zz-reach2/upgrades/2026-10/r2mlvs1-gameplay-contracts-and-content.md) Groups 1-6, ending with Group 6's public-contract task, publish `MultiLayerGameplayProfile`, `LayerVehicleCapability`, `GetMultiLayerContentProfile`, `CheckDeepLayerAvailability`, `UtilityPowerAccount`. These exports/schema fields are authoritative for this area. Same-plan consumers follow those earlier groups; cross-plan consumers verify the producer's required implementation/tests as well. Proposed names alone do not prove delivery. |
| `src/excavation.h` (planned public contract; link after implementation) | Live definitions; no fixed range | [r2mlvs2-geology-and-excavation.md](r2mlvs2-geology-and-excavation.md) Groups 1-7, ending with Group 7's public-contract task, publish `ExcavationState`, `GeologyDeposit`, `ExcavationJob`, `CheckExcavatedFootprint`, `CommitExcavationCut`, `GeothermalVentField`. These exports/schema fields are authoritative for this area. Same-plan consumers follow those earlier groups; cross-plan consumers verify the producer's required implementation/tests as well. Proposed names alone do not prove delivery. |
| [src/layer_portal_cmd.h](../../../../../../../../src/tests/fixtures/cleanup/source-epochs/openttd3-75ea75718f/c9b88cad24d07722-layer_portal_cmd.h) | Live definitions; no fixed range | [r2mlvs3-sealed-portals-and-deep-rail.md](r2mlvs3-sealed-portals-and-deep-rail.md) Groups 1-7, ending with Group 7's public-contract task, publish `LayerPortalWorkState`, `GetLayerPortalWorkState`, `CheckLayerPortalWorkAccess`, `CommissionLayerPortalTBM`. These exports/schema fields are authoritative for this area. Same-plan consumers follow those earlier groups; cross-plan consumers verify the producer's required implementation/tests as well. Proposed names alone do not prove delivery. |
| `src/tbm.h` (planned public contract; link after implementation) | Live definitions; no fixed range | [r2mlvs4-tunnel-boring-machines.md](r2mlvs4-tunnel-boring-machines.md) Groups 1-5, ending with Group 5's public-contract task, publish `TBMProject`, `TBMWorkState`, `ValidateTBMAreaRoute`, `AdvanceTBMWork`. These exports/schema fields are authoritative for this area. Same-plan consumers follow those earlier groups; cross-plan consumers verify the producer's required implementation/tests as well. Proposed names alone do not prove delivery. |
| `src/tbm_exchange.h` (planned public contract; link after implementation) | Live definitions; no fixed range | [r2mlvs5-spoil-wagons-and-haulage.md](r2mlvs5-spoil-wagons-and-haulage.md) Groups 1-5, ending with Group 5's public-contract task, publish `TBMExchangeAssignment`, `ValidateTBMWagonExchange`, `ExecuteTBMWagonExchange`. These exports/schema fields are authoritative for this area. Same-plan consumers follow those earlier groups; cross-plan consumers verify the producer's required implementation/tests as well. Proposed names alone do not prove delivery. |
| [src/rail.h](../../../../../../../../src/tests/fixtures/cleanup/source-epochs/openttd3-75ea75718f/6e6501df4baaa8f2-rail.h) | 346-394 | Complete `IsCompatibleRail` and `HasPowerOnRail` overloads describe type compatibility, not runtime electrical grants. |
| [src/pathfinder/follow_track.hpp](../../../../../../../../src/tests/fixtures/cleanup/source-epochs/openttd3-75ea75718f/cd3d04de033f9093-follow_track.hpp) | 314-435 | Complete `CanEnterNewTile` controls route entry; enforce semantic tube/layer restrictions alongside actual movement checks. |
| [src/rail_cmd.cpp](../../../../../../../../src/tests/fixtures/cleanup/source-epochs/openttd3-75ea75718f/d0032938763a2a89-rail_cmd.cpp) | 1789-1838 | Complete `TileType::LayerPortal` branch in `CmdConvertRail` changes both ends; tube conversion must refuse before this paired mutation. |

Prove semantic tube/ordinary separation independently of broad railtype masks. Validate route discovery and actual entry, including paired rail conversion. A tube never performs excavation or replaces mandatory spoil hauling.

### Group 1 - Prove a separate tube capability over rail foundations

{{HARD}}
- [X] 1.1 Allocate the profile-selected tube rail label through the canonical railtype registry; do not hard-code a runtime slot or silently collide with external content.

1.1 registry closure (2026-10-08): exact admitted native package identity allocates `UTUB` through `AllocateRailType`, with canonical-row semantic authority and no cached/hard-coded slot. Missing content, foreign main/alternate claims and a full64-slot registry refuse unchanged; registry reset removes the derived native authority. Power/compatibility masks are explicit self-only metadata, separate from energy. Existing base guideway art is reused; the type stays hidden and loader/construction/engine activation awaits1B/2. [Both executables build] (historical reference: `../../../../logs/r2mlvs3mp-z-tube-registry-build-2.log`); [registry/native-engine/upstream-support checks] (historical reference: `../../../../logs/r2mlvs3mp-z-tube-registry-checks-2.log`) pass6,052 assertions/11 cases. A stale support assertion was corrected to the already implemented recurring-project rule: reuse exact owned track on excavated floor, while refusing foreign ownership, railtype or geometry. No support runtime behavior changed. Entry producers independently pass [380 assertions/12 cases] (historical reference: `../../../../logs/r2mlvs3mp-z-tube-prerequisite-checks.log`).1B is current; master10.1 remains unchecked until all dispatched foundations pass.

### Local Mandatory Reading - Native capsule prototype and semantic separation

_Applies to Group1B after the verified canonical registry producer1.1._

Read Global/Tube infrastructure Area and only `InitialiseTubeRailType`/`IsTubeRailType`, actual shared catalogue model9, canonical EIDS allocation in `InitialiseUndergroundEngineDefinitions`, rail compatibility/powered helpers, `CFollowTrackT::CanEnterNewTile`, actual train build/entry checks and existing reservation APIs. Verify1.1's focused registry fixture before allocating a native prototype. Keep the railtype hidden and ordinary runtime activation pending until construction/movement guards exist. Compatibility masks never supply energy; broad masks cannot erase the semantic capsule/ordinary domain boundary.

### Group 1B - Prototype capsules and enforce semantic separation

Resumed2026-10-08 at the user's request: the earlier GP-TG finalization skip is historical. This group is active again after delivered1.1; prototype validation keeps construction, paid electricity and full fleet activation with their subsequent owners.

{{HARD}}
- [X] 1.2 Prototype one self-propelled capsule, one tube tile and a junction using normal reservation/path foundations. Keep compatibility/powered rail masks explicit and separate from dynamic electrical supply.
- [X] 1.3 Demonstrate ordinary trains/TBMs refuse the tube and a capsule refuses conventional/temporary rail even if a NewGRF compatibility mask is broad. Publish the semantic capability predicate used by both paths.
- [X] 1.4 Publish the prototype `TubeVehicleCapability` declaration in planned `src/transport_tube.h` and freeze its engine/rail metadata/pathfinding fixture before power integration. Group 8 later completes the public handoff for all consumers.

1B closure,2026-10-08: model9 reuses native EIDS local0x8009, canonical catalogue names/dates/layers/exact bulk refits and existing base train art. Its bounded prototype physics are Singlehead/Maglev, speed100, power1000, weight20t, capacity60 native units, running factor100/electric class and purchase factor120; these are provisional engine parameters, never electrical grants or final five-model balance. The producer is explicitly called by the test fixture; normal loader/purchase-menu activation remains pending. Actual command-created depot/capsule, native junction follower and conflicting track reservation/release pass. Initial positions/open floor are declared fixture setup; no physical powered journey is claimed.

`CanTubeTrainEnterTile` checks every actual part independently of broad masks, model/profile/layer/Deep dates and portal exclusion. Native follower, actual train movement and tile-entry dispatcher consume the same predicate; acquired cutter/support/spoil and ordinary locomotive entry into tube track refuse. Definition preflight rejects missing cargo, foreign persisted mapping and retired registry without allocating or overwriting owners. [Debug prototype/registry/refusals] (historical reference: `../../../../logs/r2mlvs3mp-z-c-tube-prototype-checks-3.log`) and [optimized checks] (historical reference: `../../../../logs/r2mlvs3mp-z-c-tube-prototype-optimized-checks.log`) pass432 assertions/eight cases. [Affected physical movement/fleet/support boundaries] (historical reference: `../../../../logs/r2mlvs3mp-z-c-tube-movement-boundaries.log`) pass115,844/23 cases. Both game/test configurations compile; earlier header/direction/include-order/fixture-precondition failures are retained in diagnostic logs and are not passing evidence. Construction/purchase/rearrangement/paired conversion and runtime electricity remain with the following groups; master10.1 stays unchecked.

### Local Mandatory Reading - Tube construction and conversion ownership

_Applies to Group2; supplements Global/Tube infrastructure Area._

Read the delivered prototype capability/admission and only current `CmdBuildSingleRail`, `CmdBuildTrainDepot`, `CmdConvertRail`, `CheckExcavatedFootprint`, `CheckExcavationConstructionEntrance`, `CheckTBMSupportChange` and normal occupancy/reservation guards. Construction must validate full world footprints before mutation; Surface/Space/locked Deep and any paired portal conversion refuse. Loader/fleet activation remains staged until purchase/mixing and metered movement owners exist. Existing artwork is reused during capability/construction development.

### Group 2 - Guard tube construction and safe work-track conversion

Resumed2026-10-08 after completed1B: the previous GP-TG skip is historical; this group is now the active construction/purchase/conversion boundary.

{{HARD}}
- [X] 2.1 Freeze label/capability, permitted Underground/Deep layers, terminal/depot footprints and legal rail-realizable junction/curve geometry. Tube motion geometry is separate from TBM cutting turns.
- [X] 2.2 Call `CheckExcavatedFootprint` for all guideway, terminal, depot and utility tiles; apply the profile Deep date gate. Test GUI, script/direct command, copy and conversion on solid/Surface/Space/locked Deep ground.
- [X] 2.3 Before replacing temporary work rail, verify TBM/hauler withdrawal and release of its occupied/reserved retreat path. Revalidate the full conversion footprint before mutation; tubes never excavate rock.
- [X] 2.4 Refuse mixed capsule/conventional consists, ordinary portal/elevator entry and any cross-layer tube route at purchase, rearrangement, route discovery and actual movement boundaries. `CmdConvertRail` can rewrite both portal ends, so reject tube conversion of a portal/tunnel pair before any paired mutation.

Group2 closure,2026-10-08: the canonical `UTUB` prototype remains rock-layer/local only, introduced1970 with the separate Deep1980 gate. Guideways use the existing six plain-rail track bits and exclusive junction conflicts, without level crossings, paired shafts or ordinary tunnel/bridge transport. Each guideway cell is one tile; service depots are one native depot tile plus their actual direction's approach. Terminals reuse the submitted ordinary rail-platform rectangle, bounded by station spread and both axis approaches. Utility producers submit their complete explicit node/link footprint to the same public floor guard. The prototype uses the normal train length/heading owner; TBM's45-degree cutting limit is not a capsule motion rule.

Canonical excavation helpers check every foundation/approach, with no new terrain/camera authority. Ordinary rail/depot/station commands and shared GUI/direct/copied command arguments retain that guard. A complete native tube conversion is preflighted before its first write; later reserved/occupied cells, paired portal/tunnel/bridge/crossing and any unreleased work-support record refuse unchanged. A finished project alone does not erase its temporary traffic metadata: the existing paid support-conversion/removal owner must release it first, including under bankruptcy flags. Native purchase/refit and physical unit/chain rearrangement enforce exact model/cargo/domain before allocation or relinking; existing source/depot haulage retains its own ordinary domain.

[Debug native/prototype/GUI/refusal checks] (historical reference: `../../../../logs/r2mlvs3mp-z-c-tube-construction-final-checks.log`) and [optimized checks] (historical reference: `../../../../logs/r2mlvs3mp-z-c-tube-construction-optimized-checks.log`) pass2,852 assertions/18 cases. Actual shared toolbar callbacks reject solid/Surface/Space/locked Deep and construct only after explicit fixture-open floor/approaches; direct command paths cover copied parameters, real station/utility footprints, whole conversion, actual committed support and real bought/refitted/coupled vehicle identities. [Affected existing movement/portal/fleet/support/command checks] (historical reference: `../../../../logs/r2mlvs3mp-z-c-tube-construction-boundaries.log`) pass116,212/31 cases. Both game/test builds and whitespace checks pass. Earlier GUI failures were missing font/main-toolbar test setup and remain diagnostic evidence, not production regressions. No raw review raster was generated or deleted. Dedicated tube menus, actual paid power, useful transport, full Squirrel scenarios and final art remain later consumers; no full runtime bootstrap/energy claim is made.

### Local Mandatory Reading - Junction ownership and native recovery

_Applies to Group3; supplements Global/Tube infrastructure Area._

Read the delivered whole-consist tube admission and construction/conversion guards, existing `TryPathReserve`/`FreeTrainTrackReservation`, `TryReserveRailTrack`/`UnreserveRailTrack`, track-layout invalidation and actual train reversal/entry controllers. Use real existing train/PBS/path owners; do not add another lease or cargo registry. Tests may seed initial positions explicitly but must distinguish route/reservation boundaries from actual driven recovery. Electricity and normal-loader activation remain staged to the following power area.

Context has compacted 1 times while executing this group.

### Group 3 - Implement junction leases and blocked-route recovery

Resumed2026-10-08 after completed Group2: the previous GP-TG skip is historical. This group is now active for existing reservation ownership, layout invalidation and blocked-route recovery.

{{HARD}}
- [X] 3.1 Map each legal tube segment/junction to existing rail track bits and reservation ownership; define one capsule movement/length model and exclusive conflict rules.
- [X] 3.2 Implement deterministic reservation requests, release on cleared segments and invalidation after layout changes. A cached path cannot authorize entry onto a converted/non-tube tile.
- [X] 3.3 Define turnaround/service approach and a blocked-network stop state without instantaneous direction changes or disappearing cargo. Test two capsules contending at a junction and a closed branch.
- [X] 3.4 Test removal/conversion while occupied or reserved; refuse unsafe mutation or release only future reservations while retaining the actual capsule footprint.

Group3 closure,2026-10-08: the prototype is one physical native train unit of `VEHICLE_LENGTH` (eight pixels), using the six existing plain-rail bits and their ordinary overlapping-track conflicts. Two command-bought capsules leave different actual depots, reserve the shared Y/Right junction in stable native order, retain their distinct packet IDs and release cleared segments before the next capsule crosses. Normal removal retires only the future path and revalidates the remaining safe prefix; occupied removal/conversion and reserved conversion refuse. A warmed route stops before an actual converted conventional cell despite broadened mechanical masks, then resumes through real ticks after repair. No duplicate lease, movement or cargo registry exists.

Moving capsule reverse commands now queue normal braking under both acceleration settings. Physical backing preserves body facing, packet ID and native position, followed by real halt/service-depot entry. Missing/incompatible branches hold the capsule in its existing PBS wait instead of automatic line-end or timed-signal reversal; repair resumes through native retries, while an explicit manual stop survives repair. [Debug selected tube checks] (historical reference: `../../../../logs/r2mlvs3mp-z-c-tube-reservations-final-checks.log`) and [optimized checks] (historical reference: `../../../../logs/r2mlvs3mp-z-c-tube-reservations-optimized-checks.log`) pass13,709 assertions/22 cases; the new four-case physical subset passes10,857. [Affected actual TBM/portal/fleet/support boundaries] (historical reference: `../../../../logs/r2mlvs3mp-z-c-tube-reservations-boundaries.log`) pass115,844/23. Both game/test configurations build, and whitespace passes. Earlier fixtures requesting a hidden-depot path outside its departure handler, selecting an alternate safe depot, or leaving direct-command signal updates buffered were corrected; their diagnostic logs are retained. These are mechanical/reservation proofs with initial real cargo packets, not paid electricity, normal-loader activation, new screenshots or useful production-route acceptance.

### Local Mandatory Reading - Tube turning contract

_Applies to Group4; supplements Global/Tube infrastructure Area._

Read the delivered native capsule length and junction/recovery fixture, canonical tube rail flags, `Rail90DegTurnDisallowed` and the real follower's track-direction filtering. Freeze tube segment geometry using existing track bits and actual traversed cells. The tube policy must not depend on camera, TBM cutting state or the player's ordinary-rail sharp-turn setting. Existing capsule reversal keeps its physical body facing; final art and all-model activation remain later obligations.

### Group 4 - Infrastructure and capability contract

Resumed2026-10-08 after completed Group3: the earlier GP-TG finalization skip is historical. This group now freezes capsule turning geometry independently of excavation turns.

{{MEDIUM}}
- [X] 4.1 Freeze legal turning geometry independently of the TBM's separate 45-degree cutting constraint.

Group4 closure,2026-10-08: canonical `UTUB` now explicitly sets `Disallow90Deg`. Existing plain-rail segment bits admit straight and traversed45-degree heading steps and reject a direct sharp90-degree step even if the ordinary-rail setting permits it. This policy belongs to guideway movement, without reading TBM project/cut state. [Focused geometry] (historical reference: `../../../../logs/r2mlvs3mp-z-c-tube-geometry-checks.log`) passes1,045 assertions/two cases: the actual follower retains straight/45-degree choices and refuses the remaining sharp choice, while a command-bought capsule physically traverses SW/S/SE through distinct approach/junction/output cells. Backing keeps the body's facing through the existing train owner. [Complete selected foundations] (historical reference: `../../../../logs/r2mlvs3mp-z-c-tube-foundations-final-checks.log`) and [optimized foundations] (historical reference: `../../../../logs/r2mlvs3mp-z-c-tube-foundations-optimized-checks.log`) pass14,754 assertions/24 cases; both game/test builds and whitespace pass. This closes master10.1 only. Paid service, normal-loader/full fleet activation, art and actual useful production routes remain in later groups.

### Area Mandatory Reading - Paid tube power and movement recovery

_Applies to Groups 5-8._

**Entry gate.** Earlier Groups 1-3 supply the prototype `TubeVehicleCapability`, construction guard and reservation behavior; r2mlvs12 Useful Power Handoff supplies tested ledger/query and consume/release/transfer mutation operations.
**ASSUMPTION: this was already built previously. If not, re-asses at runtime**. Verify the named prior public exports and their type/behavior fixtures with the current source before dependent work. A declaration alone is insufficient for runtime mutation. If missing, stop the dependent implementation and passing claims in Groups 5-8; independent characterization, test design and source verification may continue. Do not copy private stores, invent substitute IDs/inventory/energy or treat historical planning prose as delivered code.

| File name | Line Ranges | Description |
| --- | --- | --- |
| `src/transport_tube.h` (planned public contract; link after implementation) | Live definitions; no fixed range | Earlier Group 1 publishes `TubeVehicleCapability`; Groups 2-3 supply its construction/reservation behavior. Verify those fixtures before power integration; Group 8 completes remaining public exports afterward. |
| `src/utility_power.h` (planned public contract; link after implementation) | Live definitions; no fixed range | [r2mlvs12](r2mlvs12-solar-farms-mirrors-and-eti.md) `Useful Power Handoff`, produced by its native-ledger, atomic-mutation and consumer-effect groups, supplies `PublishUtilityGeneration`, `AllocateUtilityPower`, `GetUtilityAllocation`, `UtilityGrantID`, `UtilityMutationKind`, `PrepareUtilityMutation`, `CommitUtilityMutation`. Verify its allocation, consume/release/layer-transfer, effect and save/retry fixtures before dependent power mutations; no private account writes or duplicate grid. |
| [src/ground_vehicle.cpp](../../../../../../../../src/tests/fixtures/cleanup/source-epochs/openttd3-75ea75718f/55ac089437075377-ground_vehicle.cpp) | 17-72 | Complete `PowerChanged` updates caches and may set a stopped flag; dynamic grid waits require explicit manual-stop-preserving recovery. |
| [src/train_cmd.cpp](../../../../../../../../src/tests/fixtures/cleanup/source-epochs/openttd3-75ea75718f/05bb9c5676913610-train_cmd.cpp) | 455-463 | Complete `Train::UpdateAcceleration` retains minimum acceleration; zero power alone cannot enforce an allocation-based movement refusal. |
| `src/geothermal.h` (planned public contract; link after implementation) | Live definitions; no fixed range | [r2mlvs18](r2mlvs18-geothermal-energy-and-industry.md) `Publish heat allocation and electrical export APIs` supplies `GeothermalHeatAllocation`, `GeothermalUtilityLink`, `AllocateVentHeat`, `ExportGeothermalPower`; its following generator groups supply operational generation. Required only for the later geothermal source-switch integration. |
| [src/train_cmd.cpp](../../../../../../../../src/tests/fixtures/cleanup/source-epochs/openttd3-75ea75718f/05bb9c5676913610-train_cmd.cpp) | 4229-4256 | Complete `Train::Tick` calls two locomotive substeps; tube demand/settlement needs one authoritative simulation-epoch hook. |

**Power sketch (proposed).** Compatibility, reserved track and an actual grant are separate requirements. Never charge from cache-refresh callbacks.

```text
one tube tick -> compute demand -> read/reserve shared grant
             -> guarded movement/service -> commit actual consumption once
grid loss -> separate wait reason -> safe hold with cargo retained
grid returns -> resume only a power-waiting capsule, preserve manual stop
```

**Scoped geothermal gate. ASSUMPTION: this was already built previously. If not, re-asses at runtime**. Only the later source-switch subtask in Group 6 requires r2mlvs18 generation/export. Verify its producer fixture or leave that integration pending; paid-grid work continues without a fake thermal source.

### Group 5 - Connect paid external power without a geothermal prerequisite

Resumed2026-10-08 after completed Groups1-4: the earlier GP-TG finalization skip is historical. This group now consumes the actual shared electrical authority and owns bounded paid source procurement; compatibility metadata and a mechanical driving fixture cannot stand in for that service.

{{HARD}}
- [X] 5.1 Consume r2mlvs12 `Useful Power Handoff` through `src/utility_power.h`: `PublishUtilityGeneration`, `AllocateUtilityPower`, `GetUtilityAllocation`. Verify generation/grant/effect/save fixtures before adding propulsion accounting.
- [X] 5.2 Build an explicit power connection at an accessible commissioned portal-side interchange. Validate owner, excavated footprint, layer/date and a bounded paid external service contract; coordinate proximity is not a power link.
- [X] 5.3 Publish external supply once per interval and settle tariff from actual granted/used service through the common utility authority. Refuse insufficient funds/capacity without creating free energy or duplicated charges.
- [X] 5.4 Test a 1970 paid-service source/grant fixture using the earlier capsule capability prototype; no Deep/1980/geothermal output is required. The real useful route is accepted only in the final area after movement, terminals and production models exist.

Group5 closure,2026-10-08: canonical100-kW/20-per-economy-day/1-per1,000-used-J service now validates an explicit contiguous excavated corridor from an owned Connected/date-valid rail portal to an owned tube depot/station. Estimate/refusal paths publish no IDs, energy or invoices; the native ledger integrates supply and owns every grant/used joule. Whole physical depot service and usage payment settle together, retaining a0..999 numerator and charging its final fraction once on closure. [Focused source/financial/owner checks] (historical reference: `../../../../logs/r2mlvs3mp-z-d-tube-power-checks-2.log`) pass1,268 assertions/12 cases; [earlier electrical/prototype prerequisites] (historical reference: `../../../../logs/r2mlvs3mp-z-d-power-prerequisites.log`) pass1,223/22. [Actual native files] (historical reference: `../../../../logs/r2mlvs3mp-z-d-tube-power-native-checks-3.log`) prove retained385 grant/effect loading, new386 grant/use/closure, exact four-chunk reloads, four malformed-preview refusals with valid retry, and [ordinary game loading/resaving] (historical reference: `../../../../logs/r2mlvs3mp-z-d-tube-power-normal-loader.log`). The original paid excavation and385 utility inputs/outputs are preserved. [Both Debug targets build] (historical reference: `../../../../logs/r2mlvs3mp-z-d-tube-power-build-7.log`); optimized checkpoint verification follows with dispatched power foundations. This is actual source/grant/service delivery, without claiming native metered propulsion, terminal integration or normal newgame useful-route acceptance. **Group6 is current; master10.2 remains unchecked.**

### Local Mandatory Reading - Native metered capsule ticks

_Applies to Group6._

Read this plan's Global and the Paid tube power Area's entry gate and tick sketch. Consume the newly delivered Group5 public source/invoice API, `utility_power.h` grant/mutation boundary and the frozen `TubeWork` unit contract in `multilayer_utility.cpp`. Inspect `Train::Tick`, both `TrainLocoHandler` substeps, actual `VehicleServiceInDepot`/depot departure, `CallVehicleTicks` and the simulation tick owner. A per-front cache callback cannot close the global allocation early or charge twice. Same-company source/capsule bindings and fractional demand need saved actual IDs; physical movement and service must refuse missing grants under both acceleration models and force-proceed. Only6.4 waits for the later geothermal producer. Preserve Group5's actual386 source/invoice files and completed385 ground/electrical/art evidence; their replay is a prefix, never later fleet/route acceptance.

### Group 6 - Meter movement and service demand through shared grants

Resumed2026-10-08 after paid external service Group5: the historical finalization exception is not a stop gate. Implement native metering6.1-6.3 now; retain6.4 until actual geothermal source/export delivery at master10.6.

{{HARD}}
- [X] 6.1 Implement `ComputeTubePowerDemand` from frozen model, load, motion/service phase and interval units. Use checked integer/remainder accounting and one front-vehicle tick hook.
- [X] 6.2 Reserve/read a utility grant before powered movement/service; settle actual consumption once. Railtype powered compatibility means mechanical compatibility, not available energy.
- [X] 6.3 Guard actual movement/departure under both acceleration models and force-proceed; zero cached horsepower alone is insufficient. Charge once per simulation epoch, never in `GetPower`, `CargoChanged` or `ConsistChanged` cache refreshes.
- [X] 6.4 After r2mlvs18 exists, attach its explicit geothermal electrical supply to the same consumer contract and test source switching. Missing geothermal blocks only that integration, not paid-grid movement.

6.1-6.3 closure,2026-10-08: complete actual-consist model/load/phase demand integrates checked27-ms units with saved millijoule carry. Native `CallVehicleTicks` prepares all capsule companies together; one front `TubeNativeTick` surrounds both locomotive substeps and complete paid settlement. Native service/departure/movement need admission under both acceleration models and force-proceed; cache callbacks never invoice. The common ledger rejects raw paid-source consumption outside its invoice participant. [Debug] (historical reference: `../../../../logs/r2mlvs3mp-z-e-tube-meter-final-checks-1.log`) and [optimized] (historical reference: `../../../../logs/r2mlvs3mp-z-e-tube-meter-optimized-checks.log`) selected meter/source/junction/geometry/utility/metadata checks pass18,470 assertions/28 cases, including actual partial-load travel/carry, native service, cash refusal and binding estimate/refusal purity. [Real387/retained386 files] (historical reference: `../../../../logs/r2mlvs3mp-z-e-tube-meter-native-checks-4.log`) prove exact binding/source/invoice/grant reload, next service, saved loss and five corrupt preview refusals with valid retry; [ordinary loading/resaving] (historical reference: `../../../../logs/r2mlvs3mp-z-e-tube-meter-normal-loader.log`) preserves all six families. A waiting binding correctly retains a retired source whose eligibility is empty. Earlier junction fixtures now use actual paid sources; native depot departure setup remains inside its funded work action, avoiding an ownerless hidden-depot reservation. Both game/test configurations build; [read-only version discovery preserves Git's index] (historical reference: `../../../../logs/r2mlvs3mp-z-e-read-only-build.log`).6.4 alone returns after master10.6. **Group7 is current; master10.2 remains unchecked.**

### Group 6B - Attach actual geothermal supply to native capsule metering

Master10.7 dispatch,2026-10-17: master10.6 and Plan18 selected17B now supply real paid1980/2000 generation, physical utilities/risers, controls/calendar/recovery and current native-save proof. Existing capsule binding/topology/settlement still require an external `TubePowerConnection`; a generated electrical number alone cannot satisfy6.4. Execute6BB,6BC and6BD in order. Preserve one shared utility authority, physical source/cable/guideway admission, existing invoices and manual stops. Complete original6.4 only after all staged native behavior and acceptance pass.

{{HARD}}
- [X] 6B.1 Complete6BB's actual native source/path/shared-owner handoff before capsule mutations.
- [X] 6B.2 Complete6BC's native binding, movement/service and explicit source switching without renewing old rights or duplicating payment.
- [X] 6B.3 Complete6BD's controls/native files/replay and public handoff, then close original6.4 and continue master10.7's selected13-15/runtime art/lifecycle scopes.

### Local Mandatory Reading - Actual geothermal capsule supply ownership

_Applies to6BB; supplements Global and Paid tube power Area._

Read parentAD-15/AD-16 and the completed Plan18 first-plant, calendar and selected-save handoffs. Verify actual generator/collector/riser/shared Heat/electrical public state and focused producer cases. Read `BindTubePowerVehicle`, `ReconnectTubePowerVehicle`, `PrepareTubePowerTick`, `AdvanceTubePowerTick`, native `TubeNativeTick`, `VehicleServiceInDepot`, `CallVehicleTicks`, existing external invoice participant and owned tube-circuit lookup. A physical inlet/path/capacity must be accountable; neither an arbitrary rate source nor a second grid/private copied budget is allowed. Consume the completed [Plan12-9C local-feed contract](portal-tbm/r2mlvs3mp-x-useful-power-handoff.md#current-local-feed-extension) and [native432 receipt] (historical reference: `../../../../logs/utilities/local-feed/20261017-native-01/verification.json`): `RegisterGeothermalLocalFeed`, exact terminal `UtilityTransferLeg`, original-source slices, aggregate cable limits and final afterload reference ordering. The original shared local-feed gap is closed;6BB.2 now owns the native tube physical-source/connection prefix. Its nonreused inlet identity, later vehicle binding and saved original source must not be conflated. Reserve a new save version only when implemented persistent/wire semantics require it. Do not preload later factory or orbital controls.

### Group 6BB - Deliver the physical geothermal supply handoff

{{HARD}}
- [X] 6BB.1 Establish the actual source/physical path/owner/date and bounded electrical-capacity contract for a native tube inlet, consuming tested existing producers and documenting precisely which shared-owner extension is needed.
- [X] 6BB.2 Complete staged6BBB/6BBC's native source identity/lifecycle and persistent shared-owner supply prefix with pure estimates, unchanged refusals and original-source attribution; no capsule work or runtime completion is inferred from a declaration or scalar grant.

6BB.1 contract/entry closure,2026-10-17: [exact entry proof] (historical reference: `../../../../logs/tubes/geothermal-source/20261017-entry-01/prerequisites.json`) passes33 cases/14,570 assertions per configuration through actual live source/export/ledger and existing tube metering/recovery/invoices. Direct Deep supply must retain one original generator/output identity, a real owned native tube inlet, explicit paid electrical cells and actual guideway connectivity; imported work must respect one aggregate cell/edge allowance. At characterization the shared owner admitted only riser-based physical path transfers. [Plan12-9C](r2mlvs12-solar-farms-mirrors-and-eti.md#group-9c---add-conserved-local-electrical-feeds-for-native-geothermal-consumers) now delivers native432's terminal local-feed role,61 focused cases/23,141 assertions per configuration and16 exact complete cross-build native pairs. Continue6BB.2 with that delivered primitive. Plan17 physical feed IDs must distinguish an original inlet incarnation from the generator's reusable serving role; saved vehicle binding cannot infer a new inlet by proximity after demolition/replacement. Keep source energy with the original shared producer, route grants with shared consumer/parent slices and fees with their original source owners.6BB.2/6BC/6BD/original6.4 remain unchecked.

### Local Mandatory Reading - Original geothermal inlet identity and supply retirement

_Applies to6BBB; supplements Global/Paid tube power Area and the Actual geothermal capsule supply ownership Local._

Consume completed Plan12-9C's local-feed handoff/receipt. Read actual `TubePowerConnection`, external source creation/retirement/company/tile callbacks, `RegisterUtilityPowerConsumer`, native generator/output/port queries and `GetTubeTerminal`. Keep an independently issued nonreused tube inlet identity, original native facility ID, generator/output and shared forwarding consumer references; these records own no energy/rate/invoice. Shared supply retirement must close only its original local feeds and unused forwarding rights, leaving the actual generator and other users intact. Scope tests to real paid native plants, depot/cable construction and the common allocator. Capsule work and persisted source/binding transport remain6BBC/6BC; do not preload their full implementations.

### Group 6BBB - Own actual geothermal tube inlet identities

6BB.2 staging,2026-10-17: native432 supplies the common terminal electrical path, but an original tube inlet still needs an identity independent of the generator's reusable producer. Separate reference ownership/lifecycle from its following native transport audit; neither stage completes capsule movement or6.4.

{{HARD}}
- [X] 6BBB.1 Implement bounded nonreused native geothermal inlet identities and pure connection admission from an actual completed dated plant/output through purchased electrical cable to an owned native tube facility. Acquire only the existing shared forwarding reservation, never generation, a private quantity or a dummy consumer solely for an ID.
- [X] 6BBB.2 Implement scoped shared-supply closure and original inlet/source/company lifecycle, preserving other generator users and spent history; verify both configurations with actual native fixtures, pure estimates/refusals and nonreused replacements. Keep this prefix private until6BBC supplies its required native transport.

6BBB closure,2026-10-17: independently issued original source IDs retain actual generator/output, forwarding consumer and native depot/platform identity without quantities, rates, invoices or demand carry. Both current test configurations pass25 focused cases/8,948 assertions ([source/binary/log receipt] (historical reference: `../../../../logs/tubes/geothermal-source/20261017-source-owner-01/verification.json`)), including both real models, actual kernel-paid depots/cables, estimate/refusal purity, scoped original-source transfer/closure, sibling rights, physical/source loss, replacement IDs, terminal receipts and actual live/closed-source takeover. The closed-source test exposed a real common-owner bug: connected retired current history now follows live original-source/conversion ownership without reactivating it or transferring unrelated older CompanyID tombstones. Heat, electrical, generator and paid-path ownership and existing local-feed cases pass. Native source persistence/canonical transport remains6BBC; no user source command, capsule movement, source-switch or original6.4 completion is inferred.

### Local Mandatory Reading - Geothermal inlet native transport and source handoff

_Applies to6BBC; supplements Global/Paid tube power Area and completed6BBB's exports/receipt._

Read only actual6BBB identity/geometry/supply contracts, existing tube header/table/staging and native copied-preview conventions, `MultiLayerStateSchema` feature dependencies/canonical records, actual source/company reset callbacks and final afterload ordering after original generators accept. Append the concrete version with implemented source wire fields. Preserve genuine432 local-feed files and earlier external-source/binding defaults. Native source records own references, not balances or tariff copies; active and retired references have explicit validation policies. Prove actual ordinary current/older files, independent malformed preview/full refusal/recovery and complete repeat/cross-build output comparison before handing6BB.2 to6BC.

### Group 6BBC - Persist the original geothermal tube source prefix

{{HARD}}
- [X] 6BBC.1 Implement explicit native source header/table, copied/full validation, reset/afterload, metadata dependencies and canonical digest for6BBB's real inlet identity; append the implemented version and preserve genuine older defaults and retired history.
- [X] 6BBC.2 Verify both game/test configurations, current/older ordinary saves, malformed refusal/recovery and the physical/shared contracts; publish the source handoff and close6BB.2/6B.1 only after both6BBB/6BBC pass. Capsule binding/work/switching remains6BC and controls/full integration remains6BD.

6BBC/6BB closure,2026-10-17: native433 persists original inlet identity, actual plant/output/root/facility references and retired history through explicit bounded TGSH/TGSD transport, copied/full validation, reset/afterload and an appended canonical root. Saved identity survives temporary cable/date/funds unavailability; active references cannot adopt replacement machinery, imported rights or another inlet. Both game/test configurations build and pass53 focused/native cases/12,922 assertions each, including four current checkpoints, five independent malformed preview/full refusal/recovery pairs, genuine432 empty-source migration and genuine386/387 external/binding migration. All22 cross-build output pairs match all126 complete native chunks; every repeated current output is exact and the original18 authority chunks survive first activation. The retained binding observer's explicit capsule Tick is separate from the generic observers that do not tick. [Complete source transport receipt] (historical reference: `../../../../logs/tubes/geothermal-source/20261017-source-wire-01/verification.json`) binds current sources, four binaries, all isolated logs and unchanged packed artwork. Completed6BBB plus6BBC close6BB.2/6B.1; actual capsule work/source switching remains6BC and controls/full original6.4 remains6BD. No full regression or Git write.

### Local Mandatory Reading - Native geothermal capsule work and switching

_Applies to6BC; supplements Global/Paid tube power Area and completed6BB handoff._

Read the completed [original source handoff](r2mlvs17-b-geothermal-source.md#original-geothermal-tube-source-handoff) and6BBC receipt, native binding/topology/prepared-tick/work settlement and existing source invoice participant. Reuse real command-built capsules, both acceleration models and canonical demand fractions. Physical installed-path constraints and the shared allocation must hold across companies/consists; source switching settles only unused old rights and transfers demand carry once. Actual generator service and external tariffs keep their original independent owners.

### Group 6BC - Meter native capsule work and switch actual sources

{{HARD}}
- [X] 6BC.1 Complete staged6BCB/6BCC/6BCD: integrate actual binding/topology/native transport, global allocation and once-only movement/depot work; verify shortages, force-proceed, manual stops and physical/source loss without a fake generator.
- [X] 6BC.2 Verify explicit external/geothermal switching and recovery with original consumed history, native service/tariff settlement, real cargo and exactly one current binding/carry.

6BC.2/6BC closure,2026-10-17: `ReconnectTubeGeothermalPowerVehicle` and the existing external reconnect admit an actual owned held capsule on the selected original component, acquire a fresh recipient, expire only unused old rights and move its sole demand fraction once. Distinct inlet incarnations may share one actual output; neither switching nor recovery renews old grants, closes a shared source or erases its invoice. A terminal receipt guard refuses before acquisition if complete external release cannot follow. Both game/test configurations build; each passes27 isolated case runs/15,989 assertions, including actual loaded round-trip switching and native service under both acceleration models, original tariff closure, same-generator inlet recovery and unchanged admission refusals ([switch receipt] (historical reference: `../../../../logs/tubes/geothermal-switch/20261017-switch-01/verification.json`)). This closes6BC.2 and6B.2 with existing434 fields. Actual synchronized source commands/controls and current/older switched-file acceptance remain6BD; original6.4/master10.7 remain unchecked.

6BC staging,2026-10-17: actual inspection found that binding needs an existing shared consumer plus its local feed, while the delivered APIs register them separately. A late feed refusal must not strand a consumer ID or require copying the utility ledger. Stage that shared-owner transaction first, then actual provider/binding/native transport, then global work; retain6BC.2 for explicit switching after movement works. No new source version is reserved for the existing consumer/feed pair.

### Local Mandatory Reading - Atomic original local-feed recipient admission

_Applies to6BCB; supplements Global/Paid tube power Area and the completed6BB handoff._

Read only `RegisterUtilityDomainConsumer`/`RegisterUtilityPowerConsumer`, `CheckUtilityPowerLink` and `RegisterGeothermalLocalFeed` in the sole utility owner, the original geothermal source/root exports, native paid plant/depot/cable fixture and current local-feed tests. Estimate the complete new consumer/feed pair before publishing either identity; acquire fallible storage before committing both. Keep native construction/date/path/owner/role validation shared with the existing inlet API. This prefix stores only existing433 utility records and owns no generation/quantity copy. Capsule binding, transport changes and movement remain6BCC/6BCD.

### Group 6BCB - Admit original local-feed recipients atomically

{{HARD}}
- [X] 6BCB.1 Add one shared-owner operation that estimates/commits a real unlinked TubeWork consumer and its original local feed together, preserving exact current owner/date/plant/path/role and bounded issuer/receipt rules; no refusal may publish only one identity.
- [X] 6BCB.2 Verify both test configurations with actual paid native source/inlet participants, unchanged estimates/refusals and real conserved aggregate feed transfers; publish this narrow transaction handoff before binding integration. Existing native433 fields remain unchanged and actual capsule work is not inferred.

6BCB closure,2026-10-17: `RegisterGeothermalLocalFeedRecipient` in the sole utility owner predicts/adopts a real unlinked TubeWork request and its original feed together. Both vector storage expansions precede either issuer; all original source/date/path/native role checks are shared with the existing inlet API. Refusal publishes neither consumer nor feed and execution advances one common receipt, with no generator/grant/quantity or new format. Both test configurations pass12 cases/7,227 assertions in eight isolated tags, including ten actual refusal setups, exact estimates/IDs/money/RNG, original attribution and aggregate purchased-cable transfer/release. [Atomic recipient receipt] (historical reference: `../../../../logs/tubes/geothermal-source/20261017-recipient-01/verification.json`) binds both test binaries, current source/controller/logs and unchanged artwork. [Narrow API handoff](r2mlvs17-b-geothermal-source.md#atomic-local-feed-recipient-handoff) is complete;6BCC actual provider/binding/transport is current, before6BCD native work. Current433 transport is unchanged and original6BC.1/.2/6.4 remain unchecked.

### Local Mandatory Reading - Original geothermal capsule binding and native transport

_Applies to6BCC; supplements Global/Paid tube power Area and completed6BCB._

Read the completed [atomic recipient handoff](r2mlvs17-b-geothermal-source.md#atomic-local-feed-recipient-handoff), actual source identity and current `TubePowerVehicle` binding/retirement/validation/digest, derived guideway component traversal and native table/preview/afterload ordering. Bind only actual owned native capsules, retaining distinct source incarnation/output/feed/consumer identities and existing fractions. Implement concrete transport fields with their version before gameplay may acquire new bindings. Current request/work code must safely refuse the new provider until6BCD enables complete native settlement; no command/UI activation occurs here.

Context has compacted 1 times while executing this group.

### Group 6BCC - Bind original geothermal providers with native transport

{{HARD}}
- [X] 6BCC.1 Integrate actual source/provider binding and same-layer owned guideway connectivity with the atomic consumer/feed acquisition, lifecycle/reset and original-source validation; persist actual new binding fields, metadata alternatives and canonical digest with explicit older defaults.
- [X] 6BCC.2 Verify current/older ordinary bindings and independent malformed preview/full refusal/recovery in both configurations, including active/retired source/facility identity and unchanged external behavior. Leave new powered work disabled until6BCD supplies its full shared/native boundary.

6BCC closure,2026-10-17: actual owned capsules bind original source/output/root/recipient/feed identities through their native guideway component. Native434 appends two explicit TUVD fields with genuine older MAX/MAX defaults; copied preview/full acceptance follows the actual owners. Scoped retirement and actual company/vehicle reuse preserve original source/sibling history. Native acceptance also corrected capsule traction/stop cache ordering after saved rail-label conversion. Both game/test configurations build; each passes78 isolated case runs/21,277 assertions, including five lifecycle checkpoints, five independent malformed preview/full recovery pairs and genuine433/387/386 inputs. All44 cross-build output pairs and five independent writer pairs match126 complete chunks ([final binding receipt] (historical reference: `../../../../logs/tubes/geothermal-bindings/20261017-binding-wire-01/verification.json`)). [Provider/binding handoff](r2mlvs17-b-geothermal-source.md#original-geothermal-capsule-binding-handoff) is complete. **Resumed after the requested IDE refresh on2026-10-17.6BCD actual work is current;6BC.1/.2/6.4/master10.7 remain unchecked. The temporary handover was read and removed as instructed.**

6BCC lifecycle checkpoint,2026-10-17: an actual closed source/feed with an unused live recipient exposed an ownership defect before binding implementation. Takeover moved the live recipient but left its original source under the old owner, invalidating the retained physical edge. Shared ownership traversal now includes original same-company endpoint references as well as energy/conversion lineage, while retaining ACTIVE-only component anchors to protect disconnected prior CompanyID tombstones. Both current test configurations pass16 focused cases/5,294 assertions ([observed failure and final receipt] (historical reference: `../../../../logs/tubes/geothermal-source/20261017-binding-lifecycle-01/verification.json`)). This verifies the actual lifecycle prerequisite;6BCC provider/binding/topology/native transport remains pending and no new version or powered work is inferred.

### Local Mandatory Reading - Aggregate original geothermal capsule work

_Applies to6BCD; supplements Global/Paid tube power Area and completed6BCC._

Read the completed [provider/binding handoff](r2mlvs17-b-geothermal-source.md#original-geothermal-capsule-binding-handoff), actual exports and saved fields, global prepare/allocation, local-feed whole-path prepare/commit and native work participant around both locomotive substeps/depot service. Aggregate real demand once per original root, import only original slices through each actual terminal feed and shared cable capacity, then settle only complete native work. Reuse the generator's paid service; external tariffs retain their original participant. Verify whole shortage/loss/manual-stop/force-proceed behavior before explicit switching6BC.2.

### Group 6BCD - Settle aggregate geothermal grants and actual native work

{{HARD}}
- [X] 6BCD.1 Integrate aggregate root/recipient requests, one common allocation, original local-feed transfers and once-only actual movement/depot work without a private budget, duplicate generation or external tariff on already paid native output.
- [X] 6BCD.2 Verify real native capsules/cargo under both acceleration models, shortages, force-proceed, manual stops and guideway/cable/source loss, retaining original quantities/history/fractions. Close6BC.1 only after6BCB/6BCC/6BCD pass, then execute original6BC.2 source switching before6BD.

6BCD closure,2026-10-17: the common native tick gathers real capsule demand once per original forwarding root, makes one shared electrical allocation, then imports complete original slices in ConsumerID order through each purchased terminal feed and aggregate cable allowance. Actual movement and depot work settle once; native generator service is not charged an external tariff. Testing found and fixed the depot repair guard that accepted only an external transaction. Both game/test configurations build; each passes33 isolated case runs/17,230 assertions, including loaded driven arrivals under both acceleration models, twelve real service jobs sharing5400J across one/two roots, shortages/force-proceed/manual-stop recovery and cable loss after allocation. Original cargo, generation/use/service history and demand fractions remain conserved ([work receipt] (historical reference: `../../../../logs/tubes/geothermal-work/20261017-work-01/verification.json`)). Existing434 transport is unchanged.6BCB/6BCC/6BCD now close6BC.1; original6BC.2 explicit source switching is next, before6BD and original6.4/master10.7. Prepared floor/materials remain disclosed boundary inputs, not complete useful-route/bootstrap acceptance.

### Local Mandatory Reading - Geothermal tube controls and native files

_Applies to6BD; supplements Global/Paid tube power Area and completed6BB/6BC handoffs._

Read actual synchronized tube commands/component source choices, current native binding/source transports and shared original-source grant serialization/afterload. Preserve genuine old inputs; make format changes explicit rather than fabricating headers. Verify current/repeated/cross-build next-step behavior and actual source choices on native worlds, keeping independent replay distinct from host/client acceptance. Runtime sprite/gallery and whole useful-route acceptance continue through master10.7's next selected groups.

### Group 6BD - Accept current source controls and saved switching

{{HARD}}
- [X] 6BD.1 Complete staged6BDB commands,6BDC controls and6BDD current/older native files in that order: actual source choices, waiting/partial use and continued switching in both configurations, with complete owner/input identities and explicit format classifications.
- [X] 6BD.2 Publish the selected native source-switch handoff and close6B/original6.4 only after all actual prerequisites, focused checks and unchanged existing external-service behavior pass.

6BD staging,2026-10-17: the actual command and GUI owners still accept external producers only, while source creation, binding and switching now have tested native geothermal owners. Separate synchronized command admission, player source choices and ordinary saved-file acceptance so each has a bounded reading/verification scope. Existing434 fields already describe both providers; do not invent a format change for new commands or transient dropdown state. Original6BD.1/.2 and6B/original6.4 remain unchecked until every staged producer passes.

### Local Mandatory Reading - Synchronized original geothermal source commands

_Applies to6BDB; supplements Global/Paid power Area and completed6BC handoff._

Read only `transport_tube_cmd.h/.cpp`, append-only `Commands`/command trait registration, actual connect/close/bind/reconnect exports in `transport_tube_geothermal.h` and `transport_tube.h`, and the existing native command fixture/serialization conventions. Source incarnation IDs must remain distinct from shared producer IDs. Commands delegate complete owner/date/path/refusal/payment to the existing original owners; estimates acquire no identities, grants, cargo or fees. Verify real kernel test/execute/serialization, both source-switch directions and unchanged external behavior. Player dropdown/placement choices and native file acceptance remain6BDC/6BDD.

### Group 6BDB - Expose original geothermal supply through native commands

{{HARD}}
- [X] 6BDB.1 Append synchronized connect/close and capsule-bind/reconnect commands for actual original geothermal inlets, retaining original typed identities and all existing physical/owner/date/payment admission.
- [X] 6BDB.2 Verify native command estimates/execution, bounded transport, stale/foreign/refused choices and actual provider switching in both configurations; no command may mint energy, duplicate fees or move packets.

6BDB closure,2026-10-17: append-only `ConnectTubeGeothermalPower`, `CloseTubeGeothermalPower` and `SetTubeVehicleGeothermalPower` commands delegate original native source, ownership/date/path and complete bind/switch retirement to their existing owners. Original inlet IDs remain distinct from shared producer IDs; no command adds energy, cargo movement or a second fee. Native geothermal errors no longer require an external portal/lease. Both game/test configurations build; each passes13 isolated cases/7,878 assertions, including real command-kernel estimates/execution, actual native repairs, both provider directions, stale/foreign/date/facility refusal, fixed typed arguments and unchanged external commands ([command receipt] (historical reference: `../../../../logs/tubes/geothermal-commands/20261017-command-01/verification.json`)). Matching source/controller/binary hashes reuse the nine already-passing Debug command cases; the moved actual switch fixture and native movement were rechecked. Network command field width was inspected, without claiming a host/client join. Existing434 transport and packed artwork remain unchanged. Current6BDC owns player choices/guidance, then6BDD owns ordinary switched-file acceptance; original6.4/master10.7 remain unchecked.

### Local Mandatory Reading - Native geothermal source choices and guidance

**Resumed after model switch,2026-10-17:** the user requested continuation at6BDC. Consume completed6BDB and [the previous checkpoint handover](r2mlvs17-d-handover.md); its stop instruction is superseded by this request. Neither6BDC task closes until native control and focused verification pass.

_Applies to6BDC; supplements Global/Paid power Area and completed6BDB._

Read only actual UndergroundTubeControls/source readiness helpers, underground widgets/strings and native control fixtures, plus completed synchronized source commands. Distinguish external service from each original geothermal inlet even when two share one generator. Let the player select the actual plant and tube inlet, consume existing purchased-cable admission, and display the actual selected/bound provider and wait. Queries/menus cannot acquire sources or alter cargo, grants, stop intent or dates. Saved work/replay acceptance remains6BDD and full runtime-art/useful-route acceptance remains later master10.7.

### Group 6BDC - Add actual geothermal inlet and capsule source controls

{{HARD}}
- [X] 6BDC.1 Integrate actual original geothermal inlet creation/closure and explicit capsule source choices into existing native controls, with clear source identity and live physical/service/wait guidance.
- [X] 6BDC.2 Verify native input/query purity, current ownership/date/path and stale selection refusal, both provider families and real command dispatch in both configurations; preserve existing external route construction and manual stops.

6BDC closure,2026-10-17: the existing tube fold-out selects an actual completed original Deep plant and tube facility, confirms existing purchased-cable admission through synchronized commands, and offers distinct external/original-geothermal inlet choices, including two inlets sharing one generator. A separate native bound-supply row keeps the proposed choice distinct from actual capsule permission. Live original model/date, cable, service-funds and actual work waits are pure; no menu allocates power, builds cable, moves cargo or changes manual stops. The connection command additionally transports the selected original output, refusing a recycled native plant ID or stale output in both command phases. Actual native window input/dispatch, real repair/Heat loss, wrapped request/provider text fit and affected external controls/readiness/command checks pass15 isolated cases/4,385 assertions in each configuration. All four game/test targets build ([receipt] (historical reference: `../../../../logs/tubes/geothermal-controls/20261017-controls-01/verification.json`)). Reproduce with `geothermal_tube_source_checks.py --config <configuration> --focus <focus>` for controls-source/switch/guidance/stale, external-controls and its construction/fleet/stale focuses, facility/terminal/external-readiness and command-source/switch/refusal/wire; each invocation is capped at110 seconds. Reports use `mlm/reviews/tubes/geothermal-source/<run>/reports/`, native logs use `logs/tubes/geothermal-source/<run>/`, and build/receipt evidence uses the linked geothermal-controls run. Native434 and packed artwork are unchanged. This closes controls only;6BDD ordinary switched-file acceptance and full runtime-art/useful-route/host-client acceptance retain their later scopes.

### Local Mandatory Reading - Ordinary native files with switched capsule work

_Applies to6BDD; supplements Global/Paid power Area and completed6BDB/6BDC._

Read only current source/binding/shared-grant native transports, copied preview/afterload ordering and existing ordinary writer/reader helpers. Use actual current native motion/service/source switches with original packet, fraction, invoice and generator identities; retain genuine434/433/387/386 inputs without rewriting them. Prove repeated/interrupted/cross-build next-step behavior and independent malformed preview/full refusal/recovery; camera/headless variants do not imply real host/client evidence. Keep every invocation below two minutes and all generated files within the existing review/log/runtime owners.

### Group 6BDD - Accept ordinary saved source switches and complete the handoff

{{HARD}}
- [X] 6BDD.1 Complete staged6BDDB current work/switch files, then6BDDC genuine earlier formats and malformed refusal/recovery, retaining exact owner/repeated/cross-build continuation in that order.
- [X] 6BDD.2 Publish commands/control/current-file evidence and original-source handoff; close6BD.1/.2,6B.3 and original6.4 only after6BDB/6BDC/6BDD and unchanged external behavior pass, then continue selected13-15 under master10.7.

6BDD/6BD/6B/original6.4 closure,2026-10-17: the [native geothermal capsule power handoff](r2mlvs17-b-geothermal-source.md#native-geothermal-capsule-power-handoff) publishes original typed identities, common aggregate grant/paid-path ownership, native/external invoice separation, actual source controls and pinned synchronized command arguments, current434/older file policy, complete replay evidence and the observed preview-reset/compression-envelope corrections. Completed6BDB/6BDC and both6BDDB/6BDDC stages supply real loaded native/external work, source switching, waits/service, unchanged external controls and genuine older history in both configurations. Final file evidence verifies132 native cases/11,196 assertions and four current builds without a format/art change. This closes the selected source integration6.4; master10.7 stays unchecked for selected13-15, operating-plant lifecycle and runtime-art review. Execute selected13 next, with no inferred host/client or personal acceptance.

6BDD staging,2026-10-17: separate current native work continuation from earlier-format/malformed acceptance before implementation. Existing434 binding acceptance proves idle references; it does not establish consumed work, billing fractions or post-switch continuation. The following groups retain all original6BDD requirements and keep each reading/verification scope bounded. No format increment is reserved for acceptance work.

### Local Mandatory Reading - Current native capsule work files

_Applies to6BDDB; supplements Global/Paid power Area and completed6BDC._

Read the existing serialized capsule/work/switch fixtures, current source/binding/shared-grant native transports and their copied preview/afterload ordering, plus the ordinary native writer/observer helpers. Save actual travel, held source switches, cable waiting and depot service with original packet/carry/invoice/generator references. Compare uninterrupted next steps against ordinary reloads in both builds, with exact current repeated/cross-build transport. Do not preload earlier-format mutation/repair or later host-client/art scopes.

### Group 6BDDB - Verify current ordinary capsule work and source switches

{{HARD}}
- [X] 6BDDB.1 Write bounded ordinary434 checkpoints of actual partially used native/external work, both source switches, physical power waits and native depot service, retaining original packets, fractions, invoices and generator identities.
- [X] 6BDDB.2 Verify pure repeated/cross-build reloads and exact next native steps against uninterrupted baselines in both configurations, with complete owner/input identities and unchanged retained inputs.

6BDDB closure,2026-10-17: both real native writers produce six current scenes and six uninterrupted next-tick baselines: native travel, both provider switches, cable waiting, native service and external service. All12 complete126-chunk worlds match independently between writers. Both reader builds accept both writers, preserving21 original source/shared/native/packet owner chunks on initial loading and every126 complete chunks on repeated current resaves. Next-tick continuation matches125 complete chunks, including the complete actual vehicle, with the remaining GLOG independently checked as the exact native Load action whose sole differing U64 tick is its original load boundary. No movement, energy, invoice or packet field is normalized. Current file acceptance passes98 isolated native case runs/8,166 assertions total (49/4,083 per reader configuration). The shared fixture now uses the ordinary complete loading refresh; canonical Coal mass and legal freight multiplier are explicit native physics inputs. This settles an observed fixture-only speed divergence without a production change. Affected native work and source controls pass another6 cases/4,688 assertions across both builds. Both test targets build; unchanged game binaries retain their verified6BDC identities ([receipt] (historical reference: `../../../../logs/tubes/geothermal-work-files/20261017-files-01/verification.json`)). Reproduce with `geothermal_tube_work_files.py --config <configuration> --phase write`, then `--phase reload|step --writer-report <exact writer report> --case 0..5`; each native child is capped110 seconds. Outputs/reports use `mlm/reviews/tubes/geothermal-work-files/<run>/`, native logs use `logs/tubes/geothermal-work-files/<run>/`, and aggregate evidence/build logs use the linked run. Native434 and packed artwork remain unchanged.6BDDC earlier/malformed files, original6BDD and full useful-route/art/host-client acceptance remain pending.

### Local Mandatory Reading - Earlier formats and malformed switched files

_Applies to6BDDC; supplements Global/Paid power Area and completed6BDDB._

Read only genuine434/433/387/386 inputs and their receipts, explicit older source/feed defaults, copied preview and complete afterload refusal ordering, and current work-file corruption helpers. Preserve actual older invoice/work history and all retained input hashes. Test independently malformed reference/role/fraction/work copies through preview and full-load refusal with valid retry; keep each invocation below two minutes. Complete the handoff/original6BDD only after both staged groups pass.

Context has compacted 1 times while executing this group.

### Group 6BDDC - Preserve earlier files and refuse malformed switched work

{{HARD}}
- [X] 6BDDC.1 Verify genuine retained434/433/387/386 inputs with exact older roles/history, repeated current transport and cross-build continuation, without rewriting version headers or original inputs.
- [X] 6BDDC.2 Verify independent malformed switched-file preview/full refusal and valid recovery, publish the exact file evidence, then return to6BDD.2 for the original-source handoff.

6BDDC.1 checkpoint,2026-10-17: opposite-build readers verify both genuine five-scene434 and four-scene433 original writers, plus genuine386/387 external history. All original receipt/input hashes and actual version headers remain unchanged. Each reader passes24 isolated native cases/1,659 assertions;11 independent cross-reader current-output pairs match every126 complete chunks ([earlier-file receipt] (historical reference: `../../../../logs/tubes/geothermal-compatibility/20261017-files-01/earlier-verification.json`)). Pre434 bindings restore explicit MAX/MAX native-source/feed defaults;386 creates no implicit capsule binding. Existing old external observers continue their disclosed legacy service before pure repeated-current checks. This completes only earlier-file compatibility; malformed switched-work6BDDC.2 remains current.

6BDDC closure,2026-10-17: both game/test configurations build and the [final aggregate] (historical reference: `../../../../logs/tubes/geothermal-compatibility/20261017-files-01/closure-verification.json`) verifies132 isolated native cases/11,196 assertions (66/5,598 per configuration),385 exact retained identities and11 cross-reader pairs matching all126 complete chunks. The unchanged [100-case core] (historical reference: `../../../../logs/tubes/geothermal-compatibility/20261017-files-01/verification.json`) covers genuine434/433/387/386 files, current motion/cable-wait continuations and seven independent switched-row faults: missing column, raw boolean, wrong source/feed, external role, invalid fraction and future work. Preview/full load reject precise owned field/structure/authority errors and recover the original packet, carry, invoice, source, cash, clock and manual stop before another valid preview. Correct LZMA envelopes exposed a real copied-preview bug: `LoadCheckData::Clear` now resets all three geothermal source fields; the observed duplicate-header failure is retained in the core receipt. Existing source/binding corruptors had mislabeled LZMA as zlib; all five historical fault classes per family now pass with valid original OTTX headers, independent decompression and only their owned TGSD/TUVD chunk changed. The32-case follow-up repairs this evidence gap without rewriting old receipts or inputs. Native434 and packed artwork remain unchanged. Reproduce the seven current faults with `geothermal_tube_work_refusals.py --config <configuration> --writer-report <original opposite-build work writer> --fault <fault>`; corrected older helpers use `geothermal_tube_source_files.py` / `geothermal_tube_binding_files.py --phase refuse --writer-report <original family writer> --fault <fault>`. Each child is capped110 seconds; logs and rebuildable reports remain under the linked `logs/tubes/` and `mlm/reviews/tubes/` run owners. Return to6BDD.2; useful-route/art/host-client acceptance remains later.

### Local Mandatory Reading - Power loss, topology and recovery

_Applies to Group7._

Read Global and the Paid tube power Area's entry gate. Consume tested Groups5/6 and `utility_power.h` original-source consume/release/retirement. Inspect `GetTubePowerPhase`, `PrepareTubePowerTick`, native movement/depot gates, `VehicleServiceInDepot`, actual rail/depot/station layout hooks and portal/company lifecycle callbacks. Keep power waits separate from player stops and cached horsepower. Derived components follow actual owned guideway geometry, revoke only unused offers after eligible changes and rebuild without per-interval YAPF invalidation. Preserve consumed energy, invoices, demand fractions and real packet identity; a retired source is historical identity, never a free replacement contract.387 waiting files and386/385 inputs are authoritative prefixes, without terminal/full fleet/bootstrap claims.

Context has compacted 1 times while executing this group.

### Group 7 - Recover power waits without changing manual stops

Resumed2026-10-08 after native metering6.1-6.3: retain the separate power wait and player stop, derive connectivity from actual owned guideways, and test physical split/repair, paid source replacement and exact historical accounting. The prior finalization skip is historical; this group is active.

{{HARD}}
- [X] 7.1 Represent grid waiting separately from a player stop. Existing `PowerChanged` can set Stopped without clearing it on restoration; reconnect only resumes a vehicle that was waiting for power, never one manually stopped.
- [X] 7.2 Brake/hold safely while preserving cargo/reservations and necessary service/cleanup ticks. Test moving, queued and depot-service capsules under loss and restoration of a grant.
- [X] 7.3 On power topology split/rejoin, rebuild affected connected components and revoke stale unconsumed grants through the common ledger. Preserve consumed/charged history and stable consumer ordering.
- [X] 7.4 Keep physical path topology stable across routine allocation intervals; invalidate YAPF only for actual eligible layout/capability changes, not every power query. Test blocked and alternative routes after infrastructure changes.

Group7 closure,2026-10-08: actual owned layer-local trackdir components derive electricity independently of native reservations/signals. Eligible physical layout changes revoke only unused original slices and lazily rebuild affected components; routine allocation/readiness does not invalidate YAPF. Native motion, queued departure and damaged depot service hold safely with packet/manual-stop custody retained, then resume admitted work after cash/guideway restoration. Paid source replacement creates fresh identities and moves demand carry once; portal sealing/company takeover retire old sources, retaining used/invoiced history and insolvent fractional debt. [Debug focused checks] (historical reference: `../../../../logs/r2mlvs3mp-z-f-tube-recovery-checks-6.log`) pass10,202 assertions/27 cases; [optimized recovery plus affected junction/geometry] (historical reference: `../../../../logs/r2mlvs3mp-z-f-tube-recovery-optimized-checks.log`) passes20,802/32. [Real387 recovery/retained386/387 files] (historical reference: `../../../../logs/r2mlvs3mp-z-f-tube-recovery-native-checks-2.log`) prove paid replacement, exact six-family reload, no repeated service, five malformed previews and [ordinary game loading] (historical reference: `../../../../logs/r2mlvs3mp-z-f-tube-meter-normal-loader.log`); earlier meter387 inputs remain hash-identical. [Debug] (historical reference: `../../../../logs/r2mlvs3mp-z-f-tube-recovery-build-8.log`) and [optimized] (historical reference: `../../../../logs/r2mlvs3mp-z-f-tube-recovery-optimized-build.log`) game/test targets build. Recovery tests now have a separate translation unit and shared actual-route fixture. **Group8 is current; master10.2 stays unchecked.**

### Local Mandatory Reading - Public tube readiness

_Applies to Group8._

Read Global and the Paid tube power Area's tested entry gate. Inspect `transport_tube.h`, the canonical native station/depot IDs and platform geometry, `ComputeTubePowerDemand`, actual saved source/binding queries and rebuildable guideway connectivity. Expose immutable borrowed/copied views with explicit company/layer/date and invalid-reference behavior; no query grants energy, registers a vehicle, moves cargo or issues an invoice. Native station packets remain the freight owner. Group8 publishes views over actual eligible facilities, while terminal commands/full five-model/normal-game activation remain later Groups9-12. Preserve historical386/387 state and Group7's exact recovery evidence.

### Group 8 - Publish tube construction and fleet contracts

Resumed2026-10-08 after recovery7: publish actual read-only facility/network views and verify earlier public boundaries before downstream controls. The prior finalization skip is historical.

{{HARD}}
- [X] 8.1 Publish `src/transport_tube.h` exports `TubeVehicleCapability`, `TubeTerminal`, `CheckTubeConstruction`, `ComputeTubePowerDemand` with layer/cargo/rail/power ownership and invalid-reference rules.
- [X] 8.2 Expose read-only network/terminal readiness for r2mlvs7 controls and conventional freight integration. Keep actual grants owned by `utility_power.h` and derived connectivity rebuildable.
- [X] 8.3 Register source/build entries and compile/run construction, separation and power-boundary fixtures before downstream UI claims operational tube support.

Group8 closure,2026-10-08: public native facility views retain actual station/depot IDs, explicit owner/layer, canonical UTUB and native platform length. Exact model/cargo/calendar admission reuses the construction/capability owner. Network views expose only physically reachable active paid inlets and current minimum lease/usage affordability, without acquiring consumers, grants or stock. Invalid/foreign/ordinary/wrong-layer facilities refuse; real guideway split/repair and retired source change readiness. [Debug] (historical reference: `../../../../logs/r2mlvs3mp-z-f-tube-readiness-checks-1.log`) and [optimized] (historical reference: `../../../../logs/r2mlvs3mp-z-g-tube-readiness-optimized-checks.log`) construction/separation/registry/native movement/power/recovery/readiness bursts pass25,086 assertions/50 cases. [Actual386/387 loaded-world queries] (historical reference: `../../../../logs/r2mlvs3mp-z-g-tube-readiness-native-checks.log`) preserve all four/six source/invoice/binding/ledger authority chunks and original inputs. [Debug targets] (historical reference: `../../../../logs/r2mlvs3mp-z-g-tube-readiness-build-3.log`) and [optimized targets/index preservation] (historical reference: `../../../../logs/r2mlvs3mp-z-g-tube-readiness-optimized-build.log`) pass. This completes dispatched paid-grid Groups5-8/master10.2 except the explicitly staged6.4; terminal commands, production five-model activation and final art remain later groups. **Master10.3 / Plan18 Group1 is current.**

### Area Mandatory Reading - Terminals, fleet and controls

_Applies to Groups 9-12, including staged10B._

**Entry gate.** The following public contracts must exist before dependent implementation in this area: `src/transport_tube.h`: `TubeVehicleCapability`, `TubeTerminal`, `CheckTubeConstruction`, `ComputeTubePowerDemand`.
Verify each public file/export/schema field with `rg` in the current workspace and inspect its command/field ownership. For a same-plan contract, compile its type/signature fixture or validate its artifact after the preceding producer groups. For cross-plan behavior, also run the producer's focused executable boundary test; declarations alone cannot authorize dependent mutations or runtime claims.
If a required export is absent, stop the dependent implementation, mutation and runtime claims in Groups 9-12. Read-only characterization, source/range verification, test-case design and asset-template work that does not assume the missing behavior may continue.
Do not bypass this gate by copying private pool/tile state, inventing local IDs or cargo registries, using a historical plan as a delivered implementation, opening user-wide content stores, substituting a second utility/accounting authority, or adding an unreviewed fallback path. Finish the named producer or revise the shared contract and its direct consumers first.

| File name | Line Ranges | Description |
| --- | --- | --- |
| [src/vehicle_cmd.h](../../../../../../../../src/tests/fixtures/cleanup/source-epochs/daa40c5cec658808-vehicle_cmd.h) | 20-42 | Build/refit/clone/depot-autoreplace commands and traits expose all purchase paths; `CloneVehicle` is `NoTest`, so do not rely on a generic estimate to enforce capability/commissioning. |
| [src/cargopacket.h](../../../../../../../../src/tests/fixtures/cleanup/source-epochs/openttd3-75ea75718f/4b67849d158c5190-cargopacket.h) | 596-603 | `StationCargoList::Reserve`, `Load`, `Truncate`, `Reroute` provide bounded station moves and destination reservations; hub inventory has one physical owner. |
| [src/train.h](../../../../../../../../src/tests/fixtures/cleanup/source-epochs/openttd3-75ea75718f/45d43702b9f50718-train.h) | 120-148 | `Train::Tick`, `GetRunningCost`, `ConsistChanged` and acceleration declarations own simulation progress, costs and consist caches; view state cannot advance cutting. |
| `src/transport_tube.h` (planned public contract; link after implementation) | Live definitions; no fixed range | [r2mlvs17-underground-transport-tubes.md](r2mlvs17-underground-transport-tubes.md) Groups 1-8, ending with Group 8's public-contract task, publish `TubeVehicleCapability`, `TubeTerminal`, `CheckTubeConstruction`, `ComputeTubePowerDemand`. These exports/schema fields are authoritative for this area. Same-plan consumers follow those earlier groups; cross-plan consumers verify the producer's required implementation/tests as well. Proposed names alone do not prove delivery. |
| `src/underground_gui.h` (planned public contract; link after implementation) | Live definitions; no fixed range | [r2mlvs7](r2mlvs7-underground-construction-and-fleet.md) `Publish underground UI entry points` supplies `ShowUndergroundConstructionToolbar` and `BuildUndergroundVehicleFilter`; required only for binding tube controls to that UI, not tube state/commands or power. |
| [Sprite Contract and delivery owner](../../../../../../../../../OpenTTD3/docs/zz-reach2/upgrades/2026-10/r2mlvs13-graphics-and-asset-pipeline.md#public-handoff-contract) | Live definitions; no fixed range | `Sprite Contract Handoff` defines manifest fields `asset_id`, `consumer`, `directions`, `frames`, `anchor`, `source`; the matching authoring/conversion/runtime groups deliver actual assets. Verify the requested family and packed loading fixture before graphics integration; template/manifest publication alone is not final art. |

**ASSUMPTION: this was already built previously. If not, re-asses at runtime**. This applies only to the producer contracts named in this area's gate, after their listed producer groups have run. Verify their actual exports and focused tests before using the sketches below; proposed types and helper names in sketches are not evidence of delivered code.

**Five-model integration.** Reuse the previously tested tube railtype/power path and add five separate canonical engine definitions. The same cargo label has one declared physical form; Cistern-80 liquid service cannot silently accept Crate-75 packaged goods. r2mlvs13 supplies each model's original directional/load art; missing art leaves that integration/visual acceptance pending, while definition/refit tests can run.

**Scoped toolbar gate. ASSUMPTION: this was already built previously. If not, re-asses at runtime**. Tube toolbar/fleet-filter binding consumes the r2mlvs7 exports above. Verify them and their focused UI fixture before integration; absent UI stops only that binding, while tube commands, terminals and model tests continue. Do not create a second underground toolbar owner.

**Scoped graphics gate. ASSUMPTION: this was already built previously. If not, re-asses at runtime**. Final graphics tasks in this area require the corresponding r2mlvs13 manifest entries, delivered sprites and packed loading fixture. Verify them before binding/review; if absent, stop only those art tasks and their acceptance claims, continue independent code/state tests, and never count placeholders as finished assets.

### Group 9 - Add terminals and accountable freight interchange

Resumed2026-10-16 under master10.5: the user requests the entire queue sequentially; the dated skip below is historical. Existing geometry, construction and paid-service owners supply9.1. Actual packet interchange9.2 and bounded stock/payment9.3 require new focused operational proof; no fixture arrival is a driven-route acceptance claim.

Goal-authorized skip (2026-10-08, master finalization): dispatched expansion work in this group is skipped under [finalization exception GP-TG](portal-tbm/r2mlvs3mp-z-a-finalization-ledger.md#gp-tg-tube-and-geothermal-expansion-requires-major-gap-planning). Existing delivered prefixes stay complete; unimplemented profile work remains pending. No skipped task is markedX.

{{HARD}}
- [X] 9.1 Build fully excavated tube loading terminals and powered service depots with explicit tube approach/berth capacity. Reuse canonical station/cargo identities where appropriate rather than a duplicate invisible stock store.
- [X] 9.2 Connect tube-to-rail/road interchange through layer-local packet transfer and compatible loading facilities. Joining facilities cannot let capsules traverse conventional tracks or grant same-XY cross-layer catchment.
- [X] 9.3 Define bounded stock/reservations, empty returns and transfer payment semantics; verify full receiving buffers retain cargo and the final cross-layer leg uses a conventional carrier.

9.1 closure,2026-10-16: current native prerequisites pass1,803 assertions/six readiness/platform/interchange cases, plus1,355 assertions/11 construction/paid-power cases. Actual normal commands inspect every floor and door/through approach; native Station/Depot IDs and real platform lengths define berth/service capacity. Readiness resolves physically connected paid service without allocating stock or power. Isolated sources use explicit fixture-open floors/commissioned portal input; no normal-game bootstrap or useful freight journey is inferred. [Current prerequisite proof] (historical reference: `../../../../logs/tubes/terminal-interchange/20261016-native-01/prerequisites.log`), [construction/power proof] (historical reference: `../../../../logs/tubes/terminal-interchange/20261016-native-01/construction-power.log`). Continue9.2 before9.3.

9.2 closure,2026-10-16: two focused native interchange cases pass336 assertions, including separate road and conventional locomotive/mineral-wagon sections. Actual finite mine output loads a purchased capsule through native station scheduling, transfers into the destination packet list without final-delivery income, then loads the conventional truck or real wagon through its separately built joined berth. A nearby independent road stop cannot take those packets; caps/remaining stock/reserve and layer-local catchment stay conserved. Fixture arrivals are explicit economy boundaries, not driven useful-route proof. [Operational packet boundary] (historical reference: `../../../../logs/tubes/terminal-interchange/20261016-native-01/interchange-Debug-3.log`). The first joined-fixture attempt correctly refused the disabled distant-join setting; the fixture now enables/restores that ordinary setting explicitly. Continue9.3: native transfer unloading currently lacks the finite supply queue bound, so full interchange stock needs admission before packet movement.

9.3/Group9 closure,2026-10-16: [current receipt] (historical reference: `../../../../logs/tubes/terminal-interchange/20261016-native-01/verification.json`) binds four passing native builds and both configurations' focused24 cases/17,208 assertions, each tag run separately. Four new cases cover actual road and conventional train packet handoff, independent-station refusal, full/one-unit-room queues, repeated blocked visits and pickup reservations. Native unloading now applies the existing finite supply queue bound to actual tube interchanges before moving transfer packets; retained cargo and virtual feeder shares remain native, with no bank income on repeated blocked transfers. A normal pickup frees exactly its real room. The receiving conventional wagon and original packet IDs/provenance cross to Surface through actual native ticks; the capsule stays Underground. Mine/interchange arrivals and the local placement at the receiving train's purchase depot are disclosed boundary setup, so Group15's complete useful-route/bootstrap proof remains pending. Existing processor/riser/baseline delivery, mine dispatch, layer freight accounting and spoil delivery all pass. No save format, sprite payload or immutable fixture changed; no commit was made. Earlier missing-language/join-setting/portal-approach fixture diagnostics are not accepted proof. Continue Group10 before11/12.

### Local Mandatory Reading - Capsule physical geometry before source art

_Applies to staged Group10B; supplements Terminals, fleet and controls Area._

Master10.4/Plan13 Groups50-53 need native geometry before their source art, while operating fleet activation remains master10.5. Read this plan's Global/Terminals fleet Area, canonical `LayerVehicleCapability` / `GetMultiLayerVehicleCapabilities`, actual `InitialiseTubeEngineDefinitions`, `RailVehicleInfo::shorten_factor` and `Train::ConsistChanged` length ownership. Publish only a passive borrowed-catalogue physical descriptor for one full native rail part per capsule; the existing model9 producer must consume it. Querying future model geometry cannot allocate/activate an engine, bypass dates/rail domains or copy a cargo/energy registry. Original Group10 definitions/stats/model validation remain unchecked. Use a narrow public header to avoid unnecessary broad root-header rebuilds.

### Group 10B - Freeze canonical capsule physical geometry for source authoring

Staged2026-10-10 before Plan13 task50.1 under master10.4. This bounded producer supplies source geometry only; it does not execute or close the operating fleet work in original Group10.

{{MEDIUM}}
- [X] 10B.1 Publish canonical catalogue-based immutable capsule geometry with borrowed capability, full native length and one physical part; reject non-tube/unknown logical models without mutation.
- [X] 10B.2 Bind the existing model9 native definition to that geometry without changing its price/cargo/physics/identity or activating models10-13.
- [X] 10B.3 Verify pure all-model queries and actual command-bought prototype length/part count in one focused producer burst, then hand geometry to Plan13 Groups50-53. Full definitions/purchase/refit/date/art/runtime validation remain original Group10/master10.5.

10B closure,2026-10-10: [`transport_tube_model.h`](../../../../src/transport_tube_model.h) publishes callable `GetTubeVehicleGeometry` over the sole canonical catalogue, with one full native8-unit part for each of the five designs. Model9 initialization explicitly consumes that spacing; actual command purchase proves one part and matching native cached length. Queries before/after installation preserve engine/vehicle/packet counts, funds and utility state, reject other movement domains/unknown IDs, and leave models10-13 uninstalled. Game/test targets build in both configurations; the combined model/prototype/readiness/platform/interchange burst passes2,111 assertions/twelve cases in each. [Acceptance identities] (historical reference: `../../../../logs/graphics/tube-source/20261010-model-geometry-01/verification.json`) bind14 sources and four binaries. No new save state or operating fleet activation was added. Original Group10 stays unchecked and must consume this geometry when its definitions arrive. Plan13 task50.1 source may now proceed.

### Local Mandatory Reading - Five operational capsule definitions

_Applies to original Group10 under master10.5; supplements Terminals, fleet and controls Area._

Read Global/Terminals Area, the delivered `transport_tube_model.h` geometry, `transport_tube_engine.cpp` native EIDS/model/refit initialization, `multilayer_bootstrap.h` canonical per-model energy envelopes, and the five existing catalogue/public art rows. Preserve model9's published EIDS identity and physical length; add the other four original identities only after complete cargo/mapping preflight. Normal train price/capacity/speed/caches remain their native owners. Verify each model against its exact eight-direction/load/purchase source and actually loaded packed range; source availability cannot invent a calendar or power grant. Existing prototype-only source tests must be reconciled to the delivered extension explicitly. No ad hoc cargo/energy catalogue or third-party definitions.

### Group 10 - Register the five original capsule definitions

Executing2026-10-16 under master10.5 after all Group9 tasks. The dated skip below is historical. All five definitions are being added atomically through the existing EIDS/engine owners, preserving model9. Native engine tuning is explicit below; dates/cargos/layers and energy keep their existing canonical catalogues. Runtime tube source bindings remain master10.7 as declared by the authored public rows. Native definitions, exact cargo forms, physical lengths and packed model-art coverage now pass; all six Group10 tasks areX.

| Model | Native capacity | Native speed | Traction hp | Empty tonnes | Purchase factor | Electric running factor |
| --- | --- | --- | --- | --- | --- | --- |
| Mole-70 | 60 | 100 | 1,000 | 20 | 120 | 100 |
| Crate-75 | 40 | 130 | 1,100 | 18 | 150 | 110 |
| Cistern-80 | 48 | 90 | 1,000 | 28 | 180 | 140 |
| Shield-85 | 24 | 90 | 950 | 45 | 220 | 160 |
| Forge-05 | 80 | 70 | 1,500 | 48 | 240 | 220 |

Native whole-unit capacities and speed/factors use ordinary train semantics; displayed speeds, cargo mass and Money remain their existing owners. Mole's delivered prototype values are exact. Shared metered envelopes remain model9 10/20kW,10 12/18,11 12/24,12 10/20,13 20/50, with mandatory20kW depot service unchanged.

Goal-authorized skip (2026-10-08, master finalization): dispatched expansion work in this group is skipped under [finalization exception GP-TG](portal-tbm/r2mlvs3mp-z-a-finalization-ledger.md#gp-tg-tube-and-geothermal-expansion-requires-major-gap-planning). Existing delivered prefixes stay complete; unimplemented profile work remains pending. No skipped task is markedX.

{{MEDIUM}}
- [X] 10.1 Register Mole-70 (1970) as an original bulk model with profile price/capacity/speed/loaded energy and compatible ore/Rock/Earth refits outside the mandatory TBM haul.
- [X] 10.2 Register Crate-75 (1975) as its own container model with Goods/Batteries/Electronics/Mirror Kit roles and distinct stats/silhouette.
- [X] 10.3 Register Cistern-80 (1980) with the profile liquid Chemicals/Propellant forms, liquid-service compatibility and its own tank stats/art.
- [X] 10.4 Register Shield-85 (1985) with Uranium/declared sealed-mineral capability, shielding/service price and distinct body; earlier conventional Uranium freight remains available.
- [X] 10.5 Register Forge-05 (2005) with heavy Steel/Refined Metals/Components/Robotics roles, high payload/demand and distinct braced body.
- [X] 10.6 Validate each model against the profile and its eight-direction/load/purchase manifest entries. Native content must provide all five without a third-party vehicle GRF.

Group10 closure,2026-10-16: [five-model receipt] (historical reference: `../../../../logs/tubes/five-models/20261016-native-01/verification.json`) binds four passing game/test builds and24 focused cases/9,568 assertions per configuration, with each tag isolated. Five actual command purchases prove distinct native identities, exact logical refit lists/dates/names, one full8-unit part, native capacity/speed/weight/traction/price and canonical empty/full packet-backed energy. Whole-family preflight preserves earlier definitions on a late missing input, foreign or duplicate persisted mapping; saved holes reconstruct before unmapped models allocate. Model9 local0x8009 and all original physics remain exact. All730 distinct body/purchase poses match public/source/actually loaded packed ranges; no sprite payload/original/content catalogue was changed. Native processor proof is regenerated after CMake registration changes (1,907 assertions/seven cases), retaining every prior compiled family/state/layout; the shared145-family art schema passes. Normal-world controls remain12 and runtime art binding remains master10.7 as the authored rows declare. No full regression, immutable fixture change or commit. Continue11.

### Local Mandatory Reading - Five-model command and replacement admission

_Applies to Group11 under master10.5; supplements Terminals, fleet and controls Area._

Read Global/Terminals Area, delivered Group10/`transport_tube_model.h`, `CheckTubeVehicleAdmission`, actual build/refit/clone/rearrangement and autoreplace preflights, and the ordinary build list filter. Test all five models using the day before/on each calendar introduction and both rock layers, with forced availability flags unable to bypass dates. Preserve actual cargo packets, capacity and paid bindings on wrong-form/mixed-domain/refit/replacement refusal; GUI filtering alone is insufficient. No runtime art activation is authorized by a purchase guard.

### Group 11 - Apply tube purchase, refit and replacement admission

Goal-authorized skip (2026-10-08, master finalization): dispatched expansion work in this group is skipped under [finalization exception GP-TG](portal-tbm/r2mlvs3mp-z-a-finalization-ledger.md#gp-tg-tube-and-geothermal-expansion-requires-major-gap-planning). Existing delivered prefixes stay complete; unimplemented profile work remains pending. No skipped task is markedX.

{{HARD}}
- [X] 11.1 Filter purchases by actual tube depot/terminal capability, model calendar date and unlocked layer; enforce the same rule in the command rather than only its list.
- [X] 11.2 Guard clone/refit/autoreplace and train rearrangement before allocating/changing parts. Refuse mixed consists, wrong cargo form and capacity reductions that would discard existing cargo.
- [X] 11.3 Test each model at the day before/on introduction and a valid Underground versus locked Deep depot. Shared railtype introduction cannot make a model or Deep access available early.

Group11 closure,2026-10-16: [admission receipt] (historical reference: `../../../../logs/tubes/admission/20261016-native-01/verification.json`) binds four passing builds and29 focused cases/13,446 assertions per configuration, each tag isolated. All five actual model purchases obey their day-before/on-introduction and independent Deep gates despite forced availability. Native depot windows use the same date/layer/domain policy; mock font/sprite/window setup remains explicit. Clone and whole-chain replacement preflight actual domains/destinations, labels and packet room before allocation. Loaded refits cannot relabel/discard stock, reduced-capacity renewals refuse, and healthy empty refit/clone plus loaded renewal pass with original packet identity and retired old power bindings. One real failure was fixed: refused capsule refits no longer run generic capacity refresh that could truncate the retained load after refusal. Refused operations preserve cash, both RNGs and paid bindings. Affected conventional fleet/admission/command paths and tube metering/prototypes pass. Actual processor producer7-case refresh preserves every family/layout and the public145-family schema passes after CMake registration changes. No save format, immutable input, sprite payload, full regression or commit changed. Continue12.

### Local Mandatory Reading - Tube tools, native orders and fleet controls

_Applies to Group12 under master10.5; supplements Terminals, fleet and controls Area._

Read Global/Terminals Area, delivered Groups9-11 and only `underground_gui.h`, the actual underground toolbar/provider and fleet filter implementations, native rail construction/build windows, `newgrf.cpp` native registry/engine initialization and the existing paid power connection/binding owners. Consume the existing toolbar and native station/orders/CargoDist/vehicle list; no competing underground toolbar, packet, reservation or electrical authority. New-game registration and controls must reach the same paid/excavated/date gates as commands, preserving hidden legacy restoration. Final tube/utility sprite activation remains master10.7; full useful-route/bootstrap acceptance is not inferred from a menu or fixture arrival. Preserve mandatory TBM wagon/origin haulage and test the native provider boundary before wiring consumers.

### Group 12 - Terminals, fleet and controls

Goal-authorized skip (2026-10-08, master finalization): dispatched expansion work in this group is skipped under [finalization exception GP-TG](portal-tbm/r2mlvs3mp-z-a-finalization-ledger.md#gp-tg-tube-and-geothermal-expansion-requires-major-gap-planning). Existing delivered prefixes stay complete; unimplemented profile work remains pending. No skipped task is markedX.

{{MEDIUM}}
- [X] 12.1 Add Transport Tubes to Underground Construction with dedicated guideway, terminal, depot, utility and vehicle-list controls. Show route, excavation and power failures and useful cargo/refit previews.
- [X] 12.2 Coordinate underground mine/factory orders and CargoDist transfer to conventional freight for cross-layer legs.
- [X] 12.3 Support empty returns and bounded buffers; transfer handling cannot generate duplicate delivery income.
- [X] 12.4 Preserve mandatory TBM spoil logistics: its spoil leaves in the same wagons behind a second locomotive and visits the originating Surface station. Tubes can carry other mine waste or later cargo legs but are not a substitute for that requirement.


### Local Mandatory Reading - Normal tube registration and synchronized service commands

_Applies to staged12B, the first backend portion of12.1._

Read Global/Terminals Area and12's Local, `newgrf.cpp` native post-polish initialization, `InitialiseTubeRailType`/`InitialiseTubeEngineDefinitions`, the exact content admission owner, `ConnectTubeExternalPower`, `CloseTubeExternalPower`, `BindTubePowerVehicle` and `ReconnectTubePowerVehicle`. Inspect existing command traits/serialization and actual connection/vehicle ownership preflights. Source APIs own all invoices/ledger transactions; command wrappers cannot charge the same costs twice, create a second utility account, infer permission from a camera or move packets. Keep native logical dates and hidden ordinary-rail separation, while enabling the existing native profile's new-world definitions. Compile/test these public commands before dependent controls.

Context has compacted 1 times while executing this group.

### Group 12B - Register normal tube definitions and paid service commands

Split2026-10-16 after Groups9-11 exposed missing normal-newworld activation and synchronized controls. Executes the backend portion of12.1 before12C; all original12 tasks and their acceptance remain required. Earlier finalization skips are historical for the resumed full queue.

{{HARD}}
- [X] 12B.1 Register canonical tube rail/five engines for normally activated exact first-party content, preserving saved mappings, profile/date restrictions, hidden ordinary-rail separation and old-save reconstruction.
- [X] 12B.2 Append synchronized owner-checked commands for paid inlet connection/closure and explicit capsule bind/reconnect, delegating quantities/invoices to existing atomic owners. Estimates/refusals allocate nothing and source self-charges must not be charged again by the command engine.
- [X] 12B.3 Verify normal registration and actual command/serialization/owner/date/topology/funds boundaries with focused native proof; publish the callable handoff for12C without claiming complete newgame/useful-route acceptance.

12B closure,2026-10-16: [verified receipt] (historical reference: `../../../../logs/tubes/commands/20261016-native-01/verification.json`) records all four native builds and40 isolated cases/17,577 assertions per configuration. Six new cases cover exact/missing/foreign registration without pending save metadata, pure estimates/refusals, exact single lease/fractional closeout, explicit loaded reconnect with original packets and retired grant, all-action ownership and actual bounded command buffers. Native paid service/movement/recovery/EIDS/date/ordinary admission remain passing. The actual processor producer refresh passes1,907 assertions/seven cases and retains identical family/layout/state rows;145-family public art schema passes. Initial build/fixture diagnostics are retained separately and corrected, never promoted as acceptance. No saved schema, immutable fixture, packed art or Git write changed. Continue12C; original12.1 and master10.5 remain unchecked.

### Local Mandatory Reading - Tube tools and fleet presentation

_Applies to staged12C after12B, completing12.1._

Read Global/Terminals Area, verified12B exports, the existing underground toolbar and logical filter, native rail/depot/station/build/vehicle windows and the actual tube facility/readiness/power-wait owners. Extend the existing underground entry and use feature-specific views over actual vehicles; no copied fleet registry, inventory or grant. Controls choose explicit world tiles/layers and call synchronized commands; view refreshes never advance work. Final tube/utility art stays master10.7.

### Group 12C - Connect tube tools and actual fleet controls

{{MEDIUM}}
- [X] 12C.1 Add the dedicated Transport Tubes entry and guideway/terminal/depot controls through existing construction owners, with canonical date/layer/whole-floor refusals and semantic separation from ordinary rail.
- [X] 12C.2 Expose explicit paid utility connect/close and capsule bind/reconnect controls over12B, showing actual route/excavation/power/funds failures without duplicating accounts or invoices.
- [X] 12C.3 Show actual capsule fleets, native order/vehicle controls and canonical cargo/refit/availability previews. Verify read-only presentation and actual command binding, then close original12.1.

12C/original12.1 closure,2026-10-16: [verified receipt] (historical reference: `../../../../logs/tubes/controls/20261016-native-01/verification.json`) binds four native builds and50 isolated cases/18,125 assertions per configuration. Four new native UI cases272 assertions exercise the actual underground fold-out, shared rail/station/depot picker identities and conventional return,1970/1980 gates, explicit paid connection/closure and loaded reconnect retaining the17-unit original packet. Fleet entries borrow actual same-layer owned capsule fronts; native vehicle/order/refit views and all five canonical date/cargo previews create no vehicle, grant or invoice. Wrong-layer/nonadjacent/lost portal inputs refuse; lost/hidden cursors cancel, design browsing preserves power guidance, and repeated painting preserves work/stock/cash/electric state. Existing service/recovery/commands and underground entry controls pass. Actual processor refresh remains1,907/seven with identical families;145-family public art schema passes. Initial fixture/include diagnostics are retained separately. Full normal-newgame, useful-route, real multiplayer and final raster acceptance remain later scopes. Continue12D before original12.2-.4/master10.5 closure.

### Local Mandatory Reading - Tube orders, empty returns and mandatory spoil

_Applies to staged12D, completing12.2-.4 after12C._

Read Global/Terminals Area, delivered9-12C, actual station loading/orders/CargoDist and transfer-payment owners, and the existing mandatory TBM hauler/origin flow. Use real native packets, reservations, receipts and vehicle identities; unjoined or same-XY other-layer catchment never moves stock. Retain explicit fixture-arrival versus driven-route classification; full useful-route/normal bootstrap acceptance remains the later dispatch.

Context has compacted 1 times while executing this group.

### Group 12D - Verify ordered freight and retained spoil separation

{{HARD}}
- [X] 12D.1 Verify native mine/factory orders and CargoDist handoff to actual conventional cross-layer freight, then close12.2.
- [X] 12D.2 Verify empty returns, bounded/reserved buffers and one-time final delivery income across retries, then close12.3.
- [X] 12D.3 Verify the real mandatory TBM wagon/secondary-hauler/origin leg remains protected from tube substitution, then close12.4 and master10.5 only after all staged12 work passes.

12D closure,2026-10-16: all four native targets build;14 isolated tags pass44 cases/523,422 assertions per configuration. The new ordered-freight case uses actual two-stop orders and six-handler native CargoDist publication: finite mine packets transfer automatically into the joined conventional wagon, retain their identities through actual portal movement, and reach an accepting Surface refinery through native train ticks. Transfer earns no final income; factory admission pays once, and three empty retries change neither stock nor income. Existing full/reserved queue and real driven TBM loop checks pass. A real admission defect was reproduced with seven failed assertions: commands and saved assignment publication admitted capsules as secondary spoil haulers. The existing exchange owner now rejects any canonical tube part in that physical chain before mutation/publication; independent Earth capsule cargo remains allowed. Local source/interchange arrivals, departure clearing/order skips and crossing depot start are explicit fixture inputs; full autonomous tube-route/new-game/bootstrap acceptance remains later. Processor geometry re-publishes from1,907 assertions/seven actual cases with identical family layouts;145-family asset contract passes. [Receipt] (historical reference: `../../../../logs/tubes/ordered-freight/20261016-native-01/verification.json`). No save-format change, full regression, fixture cleanup, Git write or human acceptance.

### Area Mandatory Reading - Art, persistence and acceptance

_Applies to Groups 13-18._

**Raster source workflow.** New or corrected guideway, junction, terminal, service/depot and capsule originals use Plan13's [framed ImageGen prompt and extraction checks](../../../../../../../../../OpenTTD3/docs/zz-reach2/upgrades/2026-10/r2mlvs13-graphics-and-asset-pipeline.md#framed-imagegen-sheets-and-safe-extraction). Use pure-magenta source frames, transparent clearance and verified rectangular crops, or separate view files. Preserve canonical guideway connectivity and every capsule direction/load/service pose; a four-view sheet does not redefine those matrices. Review model identity, cell joins and foreground apertures separately from crop safety. Exclude frames before native conversion; actual loaded movement and occlusion remain required.

**Entry gate.** The following public contracts must exist before dependent implementation in this area: `src/transport_tube.h`: `TubeVehicleCapability`, `TubeTerminal`, `CheckTubeConstruction`, `ComputeTubePowerDemand`.
Verify each public file/export/schema field with `rg` in the current workspace and inspect its command/field ownership. For a same-plan contract, compile its type/signature fixture or validate its artifact after the preceding producer groups. For cross-plan behavior, also run the producer's focused executable boundary test; declarations alone cannot authorize dependent mutations or runtime claims.
If a required export is absent, stop the dependent implementation, mutation and runtime claims in Groups 13-18. Read-only characterization, source/range verification, test-case design and asset-template work that does not assume the missing behavior may continue.
Do not bypass this gate by copying private pool/tile state, inventing local IDs or cargo registries, using a historical plan as a delivered implementation, opening user-wide content stores, substituting a second utility/accounting authority, or adding an unreviewed fallback path. Finish the named producer or revise the shared contract and its direct consumers first.

**Scoped integration gates.** The following producer dependencies apply only to the named implementation/test families. Verify each required public export and its focused producer test before those integrations; absent late producers do not block already independent features.

- `src/underground_gui.h` is required for the tube UI/player-workflow portion; command-only capsule/persistence tests may proceed in Groups 13, 15-17 (only the named integration/subscenario). If absent, stop only that integration/subscenario and its passing claim. Other independent code, art, persistence and tests in this area may continue.
- `src/geothermal.h` is required for the later geothermal-powered network portion; paid-grid bootstrap, tube art/persistence and independent routing tests may proceed in Groups 16-17 (only the named integration/subscenario). If absent, stop only that integration/subscenario and its passing claim. Other independent code, art, persistence and tests in this area may continue.

Do not emulate an absent producer with copied private state, fake inventories/energy, local IDs or a second registry/grid. A final combined closure/gallery/round-trip still waits for every required family; an independent passing case is not a pass for the missing integration.

| File name | Line Ranges | Description |
| --- | --- | --- |
| [src/saveload/vehicle_sl.cpp](../../../../../../../../src/tests/fixtures/cleanup/source-epochs/openttd3-75ea75718f/6db99340d5935853-vehicle_sl.cpp) | 1119-1196 | `VEHSChunkHandler` and registration construct the vehicle class from saved type before member loading; protect old enum meanings and validate new references. |
| [src/network/network_command.cpp](../../../../../../../../src/tests/fixtures/cleanup/source-epochs/openttd3-75ea75718f/cab07f5ceefcfdf4-network_command.cpp) | 256-288 | `NetworkExecuteLocalCommandQueue` executes agreed frame order/company authority; all jobs use deterministic simulation time. |
| [src/network/network_server.cpp](../../../../../../../../src/tests/fixtures/cleanup/source-epochs/openttd3-75ea75718f/6a45d374c71c770f-network_server.cpp) | 670-687 | `ServerNetworkGameSocketHandler::SendSync` compares RNG seeds, not a full gameplay checksum; validate orbital/geology/cargo ledgers in addition to a no-desync result. |
| `src/transport_tube.h` (planned public contract; link after implementation) | Live definitions; no fixed range | [r2mlvs17-underground-transport-tubes.md](r2mlvs17-underground-transport-tubes.md) Groups 1-8, ending with Group 8's public-contract task, publish `TubeVehicleCapability`, `TubeTerminal`, `CheckTubeConstruction`, `ComputeTubePowerDemand`. These exports/schema fields are authoritative for this area. Same-plan consumers follow those earlier groups; cross-plan consumers verify the producer's required implementation/tests as well. Proposed names alone do not prove delivery. |
| `src/underground_gui.h` (planned public contract; link after implementation) | Live definitions; no fixed range | [r2mlvs7-underground-construction-and-fleet.md](r2mlvs7-underground-construction-and-fleet.md) Groups 1-5, ending with Group 5's public-contract task, publish `ShowUndergroundConstructionToolbar`, `BuildUndergroundVehicleFilter`. These exports/schema fields are authoritative for this area. Same-plan consumers follow those earlier groups; cross-plan consumers verify the producer's required implementation/tests as well. Proposed names alone do not prove delivery. Read this producer for the tube UI/player-workflow portion; command-only capsule/persistence tests may proceed in Groups 13, 15-17 (only the named integration/subscenario); missing it blocks only that integration, not independent work. |
| `src/geothermal.h` (planned public contract; link after implementation) | Live definitions; no fixed range | [r2mlvs18-geothermal-energy-and-industry.md](r2mlvs18-geothermal-energy-and-industry.md) Groups 1-7, ending with Group 7's public-contract task, publish `GeothermalHeatAllocation`, `GeothermalUtilityLink`, `AllocateVentHeat`, `ExportGeothermalPower`. These exports/schema fields are authoritative for this area. Same-plan consumers follow those earlier groups; cross-plan consumers verify the producer's required implementation/tests as well. Proposed names alone do not prove delivery. Read this producer for the later geothermal-powered network portion; paid-grid bootstrap, tube art/persistence and independent routing tests may proceed in Groups 16-17 (only the named integration/subscenario); missing it blocks only that integration, not independent work. |

**ASSUMPTION: this was already built previously. If not, re-asses at runtime**. This applies only to the producer contracts named in this area's gate, after their listed producer groups have run. Verify their actual exports and focused tests before using the sketches below; proposed types and helper names in sketches are not evidence of delivered code.

### Group 13 - Art, persistence and acceptance

Resumed2026-10-17 after original6.4. The earlier finalization skip is historical. All required tube families are already packed; existing native owners still need their actual renderer bindings and bounded acceptance. Execute13B-13E in order; no source regeneration, full-gallery promotion or human acceptance is inferred.

{{MEDIUM}}
- [X] 13.1 Integrate tube straight/bend/junction/end sprites, terminal fronts/backs, depot/service animation and power indicators. Show capsules moving through readable transparent/cutaway tube art, with correct occlusion and selection.
- [X] 13.2 Save capsule orders/positions/cargo, service/power state and network references.

### Local Mandatory Reading - Native tube guideway rendering

_Applies to13B; supplements Global and Art/persistence/acceptance Area._

The delivered contract/evidence is in [native tube runtime notes](r2mlvs17-e-tube-runtime.md#native-tube-guideway-handoff); historical source-switch notes are already closed.

Read actual `transport_tube.h` construction/end/topology exports, `DrawTile_Rail`, existing `multilayer_sprite.h` pure pose selection, packed tube straight/bend/junction/end/crossing/conduit rows and the completed Plan13 Group47 native geometry/foreground contracts. Native track bits and original source/shared publication own geometry and supply; presentation cannot publish a grant, renew an invoice, alter a packet or scan map planes. Preserve the54 authored shared-edge mask order, independent parallel curves, native mouth adjacency and transparent capsule apertures. Use focused native selectors/real viewport checks in both builds, with no unrelated asset authoring or full regression.

### Group 13B - Bind actual guideways to packed tube shells

{{HARD}}
- [X] 13B.1 Select packed straight/bend/junction/crossing/end sprites from actual native track geometry and pure original-source publication/path state; preserve existing source/quantity owners. Actual service conduits follow13C with their facility/path owners.
- [X] 13B.2 Integrate native rail drawing with aligned back/foreground sorting and verify geometry, source loss/recovery, presentation purity and actual viewport pixels in both configurations.

13B closure,2026-10-18: packed native shells now replace ordinary rail on actual excavated flat tube tiles, using all63 native layouts,54 authored shared-edge masks, independent parallel/crossing cases and real adjacent end mouths. Complete backs and exact aligned near rims retain capsule apertures through native sorting; existing signals and optional reservation cues remain native. Current original publication/path reach determines the supply indication, separately from any particular capsule's selected grant/wait. Built guideways retain tube identity when new construction is date-locked. Bounded existing tile-loop visits refresh indicators without a new per-tick map scan, saved state or quantity owner. Both game/test configurations build and pass14 isolated cases/2,900 assertions total (seven/1,450 per configuration). All six cross-build before/after world pairs match every126 complete chunks;12 corresponding full-frame pairs are RGB-exact, and3,050 authored foreground pixels match their actual native palette/anchor at both zooms. A declared native reservation boundary fixture changes exactly one MAP2 U16 field (tile13920,0 to512), preserves all125 other chunks and shows54/216 actual cue pixels at1x/2x in both builds. It is separate from genuine queued-route persistence. All95 retained report/input/source/log identities and four binaries are pinned in the [closure receipt] (historical reference: `../../../../logs/tubes/guideway-art/20261017-native-01/verification.json`). The art matrix explicitly declares128 clearing cells because its depot plus63 layouts exhaust the original64-cell fixture quota; production clearing rules are unchanged. Reproduce isolated cases with `tube_sprite_checks.py --config <configuration> --focus geometry|ends|external|geothermal`; native views use `--focus raster --writer-report <original opposite-build work writer> --case 0|3`, and the declared reservation scene uses `--focus reservation --case 3`. Regenerate the immutable aggregate with `tube_sprite_report.py --reports <fourteen exact report paths> --output logs/tubes/guideway-art/<run>/verification.json`; native children are capped110 seconds. Generated scenes/logs retain `mlm/reviews/tubes/guideway-art/<run>/`, `logs/tubes/guideway-art/<run>/` and `mlm/runtime/` owners. Native434, packed artwork and all original inputs remain unchanged. Continue13C; capsule/facility/conduit art, full picking/occlusion/useful routes, runtime gallery promotion and human feedback remain their later scopes.

### Local Mandatory Reading - Native capsule and tube facility rendering

_Applies to13C; supplements Global and Art/persistence/acceptance Area._

Read completed13B, actual capsule capability/native engine mapping, packet/load/service/power wait queries and existing train/depot/station sprite sorting/picking. Consume packed model09-13 and tube service/terminal/interchange state/direction/cell/part rows without changing native lengths, dates, cargo admission or stock owners. Avoid reloading unrelated source-art history.

### Group 13C - Bind capsule and facility artwork to real native owners

{{HARD}}
- [X] 13C.1 Complete staged13CB: all five actual capsule model/load/movement/service/wait poses and purchase previews through existing native vehicle identity, rendering and picking.
- [X] 13C.2 Complete13CC facility parts/status, then13CD actual service conduits, verifying native geometry and packet/service/query purity in both configurations. Close13C only after all three stages pass.

13C staging,2026-10-18: actual packed contracts contain720 capsule body poses plus ten purchase poses,640 facility parts and12 conduit branches. Their different native owners need separate focused verification: train packets/service/power and purchase first, depot/station geometry/status second, original utility paths third. This preserves all original13C requirements while bounding each read/build/test scope.

### Local Mandatory Reading - Actual capsule body and purchase poses

_Applies to13CB; supplements Global and Art/persistence/acceptance Area._

Read `GetTubeVehicleCapability`, `TubeVehicleGeometry`, `GetTubePowerPhase`, active binding/wait and actual packet-count contracts, `GetMultiLayerTrainSprite`, `RefreshMultiLayerTrainArt`, native `Train::GetImage`/purchase callbacks and packed vehicle/purchase model09-13 rows. Consume source's9 states as load0/1/2 crossed with operating/service/blocked, eight actual body facings and two status-lamp phases; those phases do not move a hull or rotate a wheel. Preserve native full-part length, actual cargo forms, manual stops, once-tick work, model dates and foreign fallback. Use actual command-purchased models and public source/native service transitions; no private binding edits, synthetic model IDs or duplicate packet inventories. Facility/conduit paths remain13CC/13CD, full raster/picking acceptance13D.

Context has compacted 1 times while executing this group.

### Group 13CB - Bind all five actual capsule body and purchase families

{{HARD}}
- [X] 13CB.1 Integrate actual native capsule/load/service/wait/body-facing and locked/available purchase poses through existing train/purchase selectors, without changing physics, ownership, availability or stock.
- [X] 13CB.2 Verify all720 body/ten purchase poses and actual native selector/cache/packet/service/source behavior in both configurations, with focused current-world rendering and declared fixture classifications; close13C.1 after success.

13CB closure,2026-10-18: all720 body/ten purchase poses select the five actual native capsule identities and original packet/service/source owners. Native flipping occurs once; two lamp phases never move a hull or earn work. Genuine depot repair consumes540J once while preserving manual stop. Four targets build; fourteen isolated invocations pass36 cases/35,346 assertions (18/17,673 per configuration), including affected prior art and metering. Every render-boundary pair retains126 chunks; four opposite-build boundary pairs match126 chunks and eight full-frame pairs are RGB-exact.1,060 upper-hull pixels match their native palette/anchor. [The receipt] (historical reference: `../../../../logs/tubes/capsule-art/20261018-native-01/verification.json`) pins69 report/source/input identities and four binaries. Mechanical fixtures use existing mock bounds headers, which establish no pixels. Full-application captures drive original ordered capsules clear of depot roofs; explicit external-source recovery, travel and source loss/native braking precede the pure render boundary. The saved pause must clear for declared native travel. Version434, packed art, physics/dates/refits and original inputs remain unchanged. [The handoff](r2mlvs17-e-tube-runtime.md#native-capsule-body-and-purchase-handoff) records reproduction and fixture classifications.13C.1 is complete; continue13CC, then13CD. Full13D picking/directional review,13E files,14-15 and runtime-gallery/human acceptance retain their scopes.

### Local Mandatory Reading - Actual tube facility parts and status

_Applies to13CC; supplements Global and Art/persistence/acceptance Area._

Read completed13CB, original native tube depot/station/interchange identity queries, actual native part bounds and packed service/terminal/interchange direction/cell/state/frame/part rows. Reuse existing train/road/rail station packet owners and source waiting queries; foreground/presentation clocks cannot repair a capsule, load stock or acquire power. Source conduit branches remain13CD.

### Group 13CC - Bind actual tube depot terminal and interchange parts

{{HARD}}
- [X] 13CC.1 Integrate real native depot and terminal/interchange back/front/status parts, preserving authored cell geometry and native picking/transparency/occlusion.
- [X] 13CC.2 Verify native rendering/status animation and original packet/service/source/query purity in both configurations, with exact current-world evidence.

13CC closure,2026-10-18: actual native depot doors and all eight default station graphics select aligned service/terminal/interchange parts, preserving original packet, depot-service and source owners. Retained built identity is independent of current construction permission. Original native ANIT/m6 registration owns four-frame lamps; command publication and bounded tile loops register facilities, callbacks only dirty pixels, and native clearing retires registration. Healthy parking earns no repair; actual demand/loading and original full-queue/power waits select their source states. Instant native construction has no unfinished phase0. Four targets build. Thirty-six isolated invocations pass52 cases/31,772 assertions (26/15,886 per configuration). Five genuine scene pairs and ten cross-reader render boundaries match every126 chunks;20 full-frame pairs are RGB-exact and450 original status pixels match native palette/anchor. [The receipt] (historical reference: `../../../../logs/tubes/facility-art/20261018-native-01/verification.json`) verifies153 referenced identities and four binaries. Native434 and packed art remain unchanged. Fixture findings: canonical rail labels must precede tube allocation; the empty table otherwise assigns UTUB to nominal ordinary slot0. Explicit distant joining remains permission-owned. Actual native clear/rebuild spaces the diagnostic yard so a neighbouring depot roof cannot hide glass. The art view disables the optional client reservation overlay while preserving actual reservations; no pixel tolerance, mask or artwork was changed. Prepared floor/materials, wear and queue pressure remain declared boundary inputs; real orders/ticks earn loading arrival. [The facility handoff](r2mlvs17-e-tube-runtime.md#native-tube-facility-handoff) records exact reproduction and inputs. Continue13CD;13C.2, full13D picking/directional review,13E files,14-15 and runtime-gallery/human acceptance retain their scopes.

### Local Mandatory Reading - Actual tube service conduit paths

_Applies to13CD; supplements Global and Art/persistence/acceptance Area._

Read completed13B/13CC, original external physical path and native purchased electrical link queries, packed four-port conduit rows and actual clear/rail/facility ground drawing. Render only installed original source paths on their explicit layer, without inferring a wire from a nearby generator, granting energy or copying a private utility store. Whole geothermal utility/riser runtime promotion remains later Plan14-84.3.

Ownership clarification,2026-10-18: packed `tube.conduit` explicitly names `TubePowerConnection::utility_path` as its geometry owner. Native purchased heat/electricity cells name the separate `geothermal.conduit` family and retain Plan14-84.3 integration.13CD verifies those original native queries and refuses to duplicate their installations with external-service artwork. Active external path order alone selects branches; retained inactive invoices are history, and instant paid installation has no unfinished construction phase0.

### Group 13CD - Render original tube service conduit branches

{{HARD}}
- [X] 13CD.1 Integrate and verify actual original conduit ports/source states in both configurations, retain complete owner/input identities and close13C.2/original13C only after13CC also passes.

13CD/original13C closure,2026-10-18: active original external `utility_path` order now selects the four packed ground branches and current original publication state. A bounded derived edge index creates no installed path, routing join, inventory or energy owner. Nearby parallel cells are not joined; overlapping paid edges may share pixels while retaining independent original source states. Retired invoices paint no wire. Native paid geothermal links keep their separate artwork owner, verified through their actual installed/path queries. Rail/road/station insert below ground building backs; other floors use the common viewport ground stage, below sortable vehicles/buildings. Actual source commands dirty bounded path footprints; existing native tile loops expire indicators. Both game/test configurations build. Twenty-four isolated invocations pass34 cases/14,218 assertions (17/7,109 per configuration). Three independent writer worlds and six opposite-reader boundaries match every126 chunks,36 complete frame pairs are RGB-exact, and10,490 composed original source pixels match DOS palette/anchor. [The receipt] (historical reference: `../../../../logs/tubes/conduit-art/20261018-native-01/verification.json`) pins160 referenced identities and four binaries. An inherited allocated interval correctly refused late fixture publication; normal synchronized source preparation and an actual capsule tick publish the scene before pure capture. Shared seams compare the exact native ordered composition of adjacent authored halves, with no mask/tolerance. Native434/GRF remain unchanged. [The conduit handoff](r2mlvs17-e-tube-runtime.md#native-tube-conduit-handoff) records reproduction and explicit floor/portal/packet fixture classifications.13CC and13CD together close13C.2/original13C. Continue13D;13.1,13E/13.2, lifecycle14, useful routes15, native geothermal utility/gallery and human feedback retain their scopes.

### Local Mandatory Reading - Complete native tube visual acceptance

_Applies to13D; supplements Global and Art/persistence/acceptance Area._

Read only completed13B/13C contracts, actual paid source/capsule work fixtures and viewport pixel/picking observation helpers. Exercise real travel, load, service and source waits at authored directions with unchanged complete simulation owners. Keep ordinary runtime display evidence separate from useful-route bootstrap, full gallery promotion and human feedback.

### Group 13D - Accept the complete visible tube integration

{{HARD}}
- [X] 13D.1 Complete staged13DB actual directional travel/picking, then13DC joins/foreground/facility-state acceptance in both builds; retain exact input/binary/pixel/owner evidence and close13.1 only after13B/13C and both stages pass.

13D staging,2026-10-18: packed selector coverage is complete, while genuine physical headings and complete joins/status/picking evidence have different native observation boundaries. Execute13DB first, then13DC; the original13D.1/13.1 requirements remain unchanged. Direction previews alone cannot establish actual movement.

### Local Mandatory Reading - Actual capsule directional travel and picking

_Applies to13DB; supplements Global and Art/persistence/acceptance Area._

Read completed13CB/13CD handoffs, actual command purchase/model/cargo identity, native curved rail connectivity, normal capsule/source tick preparation, viewport bounds and `CheckClickOnVehicle`. Use original paid source/native orders and physical travel to earn headings. Capture whole current-world frames and exact original hull pixels at both zooms, with native correct-layer picking and wrong-layer refusal. Rendering boundaries preserve every complete original owner independently of the declared travel between them. All five models use a disclosed current available calendar; their separate useful introduction-date routes remain15.2. For the current curve-depth diagnosis, read only completed Plan13-47C native Upper/Lower/Left/Right source/foreground contract and `AppendTubeGuidewayPart`/per-part foreground bounds. A curve must sort in its actual quarter, with original anchors and native vehicle geometry retained. No coordinate/direction edits or static GetImage preview is physical movement. Joins/crossing foreground and complete native facility/status review follow13DC.

### Group 13DB - Accept genuine directional capsule travel and native selection

{{HARD}}
- [X] 13DB.1 Drive all five actual command-purchased models through a real paid curved route and return; verify actual authored headings, readable hulls and native correct/wrong-layer picking in both builds with complete original input/work/render identities.

13DB closure,2026-10-18: all five actual command-purchased2005-available models earn all eight native body facings through four real paid outbound corners, then return through normal order replacement/ticks with their original default cargo/packet17. Native correct-layer picking and wrong-Surface refusal pass at both zooms. A real renderer defect was reproduced: every curve inherited the straight guideway's tiny southeast sorting box, hiding the container capsule's north interior (87/204 pixels). Per-part actual native quarter bounds now retain original source anchors/height and expose all204 interior/all272 hull pixels in that view. Other shells retain their existing bounds; capsule physics, native lengths, admission and quantities are unchanged. Every non-company hull pixel is directly exact or covered by an exact original neighbouring foreground at that coordinate; both half-hull and half-interior legibility checks pass. Four targets build;28 isolated cases/78,902 assertions pass (14/39,451 each configuration), including the all63-layout anchor/nonempty-bounds invariant and affected body/service contracts. Five independent writer worlds,80 opposite-reader render boundaries and five real return worlds match all126 chunks;160 full-frame pairs are RGB-exact.104,890 visible hull pixels and370 original-rim occlusion pixels match their actual DOS source/anchor. [The receipt] (historical reference: `../../../../logs/tubes/travel-art/20261018-native-01/verification.json`) pins620 referenced identities and four binaries. Real paid round trips take1725/1377/1922/1976/2414 ticks for models9..13, consuming597,696/540,954/843,277/911,412/1,719,069J from original owners. Explicit surrounding floor/quota128/commissioned portal and loaded packet remain boundary inputs; useful introduction-date freight routes remain15.2. Native434/GRF remain unchanged. [The directional handoff](r2mlvs17-e-tube-runtime.md#native-directional-travel-and-picking-handoff) records reproduction and oracle/fixture findings. Continue13DC;13D.1/13.1 remain unchecked until actual joins/foreground/facility states also pass. No full regression, Git write, agents or deletion.

### Local Mandatory Reading - Complete tube joins and facility visual interaction

_Applies to13DC; supplements Global and Art/persistence/acceptance Area._

Read completed13DB movement/picking evidence, original guideway parts/native bounds and actual station/depot/interchange/source/packet/service scene writers. Verify foreground occlusion and joined native geometry through actual rendering and selection, retaining complete original work owners. Reuse original prepared-floor/service/queue fixture classifications. Consume current scoped evidence; dated prior receipts establish contracts, not current binary acceptance. Full gallery promotion and human review remain their own scopes.

### Group 13DC - Accept native foreground joins and complete facility status views

{{HARD}}
- [X] 13DC.1 Complete13DCB actual crossing/junction foreground travel, then13DCC native facility/loading/service/full-queue/source-state interaction and selection, preserving original owners in both builds; close13D.1/13.1 only after13DB and13B/13C also pass.

13DC staging,2026-10-18: actual crossing/shared-edge travel and facility status/window selection have separate native observation owners. Execute13DCB before13DCC; original13DC/13D.1/13.1 stay open until both pass.

### Local Mandatory Reading - Actual crossing and shared-edge capsule travel

_Applies to13DCB; supplements Global and Art/persistence/acceptance Area._

Read completed13DB directional handoff, actual native TrackBits/physical train track, original crossing/junction atlas rows and per-part foreground bounds. Extend the same real paid route through a native X/Y crossing and an actual shared-edge junction, with paid physical branches and real target orders. Keep source/capsule/packet owners unchanged during each complete raster/picking boundary. Source alpha oracle must resolve the actual native mask/variant/publication; no unused branch becomes a routed connection or grant. Actual physical movement must earn the crossing/junction view. Native facility stock/status/window scopes remain13DCC.

### Group 13DCB - Accept real crossing and shared-edge foreground travel

{{HARD}}
- [X] 13DCB.1 Drive actual native capsules through paid crossing/junction geometry and verify foreground visibility, native selection and original packet/source/work purity in both builds with exact original input/world/frame/source evidence.

13DCB closure,2026-10-18: real paid X/Y crossing3 and shared-edge junction5 now have actual model10/13 travel/picking/foreground acceptance in both builds. Captured crossing SE uses the original physical Y2; captured junction E uses actual Upper4 within complete tile mask5. Native chosen track owns physical inspection, while original full tile mask/publication owns source selection. Paid branch ends stay outside each bounded rim inspection; unused crossing X does not become a capsule turn or work grant. No further production or artwork change was needed beyond completed13DB quarter depth. Four targets build;16 isolated cases/40,782 assertions pass (8/20,391 per build), including current geometry/ends/body/service contracts. Two independent writer worlds,32 opposite-reader boundaries and two genuine paid return worlds match all126 chunks;64 complete frame pairs agree.46,050 visible hull pixels and110 original-rim-covered pixels are exact. [The receipt] (historical reference: `../../../../logs/tubes/joins-art/20261018-native-01/verification.json`) pins300 referenced identities and four binaries. Both original models retain default cargo/packet17; their1377/2414-tick returns consume540,954/1,719,069J. Explicit surrounding floor/quota128/portal and packet remain boundary inputs. Native434/GRF are unchanged. [The joined handoff](r2mlvs17-e-tube-runtime.md#native-crossing-and-junction-handoff) records exact reproduction. Continue13DCC;13DC.1/13D.1/13.1 remain unchecked until complete native facility/status/click acceptance also passes. No agents, Git write, deletion or full regression.

### Local Mandatory Reading - Complete native facility status and click interaction

_Applies to13DCC; supplements Global and Art/persistence/acceptance Area._

Read completed13DCB, actual facility scene writers/pure packet/source state queries, native `ClickTile`/depot/station window identity and vehicle hidden/picking policy. Reuse actual ready/service/loading/interchange/full-queue scenes and add genuine original-source closure before pure capture. Native tile clicks must open the actual native facility; hidden depot bodies remain hidden. Controls/window construction cannot move stock, acquire source work or advance a clock. Preserve original fixture classifications, complete worlds and native DOS status pixels; whole gallery/human feedback remain separate.

Context has compacted 1 times while executing this group.

### Group 13DCC - Accept complete facility status and native selection

{{HARD}}
- [X] 13DCC.1 Verify actual facility loading/service/stock/source states and native window/body selection in both builds, retaining original packets/owners and exact frames; close13DC.1/13D.1/13.1 only after13DCB and all earlier visual stages also pass.

13DCC closure, 2026-10-18: the six actual native ready-depot, pending-repair, driven-loading, conventional-interchange, full-queue and source-closed scenes now pass native facility window selection and layer-aware body picking in both builds. Actual source commands close both original offers; original packets, plant and financial history remain intact. Hidden depot bodies remain unpickable, while the exposed loading body selects on its true layer and refuses the Surface view. Native tile clicks open the actual depot/station window after capture, without moving stock or advancing work. No production change was required. Four targets build; 42 isolated invocations pass 58 cases/32,620 assertions (29/16,310 per configuration). Six independent writer pairs and 12 opposite-reader boundary pairs retain every one of 126 native chunks; 24 complete frame pairs and 540 original status pixels are exact. [The receipt] (historical reference: `../../../../logs/tubes/facility-interaction/20261018-native-01/verification.json`) pins 182 referenced identities and four binaries. Native434 and packed artwork remain unchanged. Prepared floor/materials, wear and queue pressure remain disclosed boundary inputs; actual orders/ticks earn loading arrival. [The interaction handoff](r2mlvs17-e-tube-runtime.md#native-facility-interaction-handoff) records reproduction. Completed13B/13C/13DB/13DCB and this group together close13DC.1/13D.1/original13.1. Continue13E files, then14 lifecycle and15 useful routes. Runtime gallery, human feedback and pruning remain pending; no full regression or Git write.

### Local Mandatory Reading - Native tube orders and power files

_Applies to13E; supplements Global and Art/persistence/acceptance Area._

Read completed6BDDB/6BDDC file evidence and actual ordinary vehicle/orders/station/source/binding/shared utility transports. Verify original orders/positions/packets/service/power/network references independently of derived renderer caches; preserve genuine earlier saves. Extend only missing focused native cases, leaving lifecycle14 and useful-route15 separate.

### Group 13E - Accept authoritative capsule work and network files

{{HARD}}
- [X] 13E.1 Verify current/repeated/next-step native orders, positions, cargo, service/power and original network references in both configurations, then close13.2 with explicit earlier-file classifications and complete owner evidence.

13E closure, 2026-10-18: current Debug and optimized readers accept the opposite retained6BDDB writer's actual434 travel, both source-switch, cable-wait and native/external service files. No new native source or fixture world was needed. Twenty-four focused invocations pass48 native cases/3,336 assertions (24/1,668 per configuration). First loads preserve all39 complete authoritative chunks, including complete vehicle position/motion/service/current order, ordinary order list, packet, depot/station, physical map/portal references and original power/shared utility owners. Repeated saves retain all126 chunks. Each next native opportunity matches the uninterrupted baseline in125 complete chunks, with the diagnostic GLOG Load action's original U64 load tick independently checked and every other byte exact. All24 corresponding cross-reader output pairs match all126 chunks. [The receipt] (historical reference: `../../../../logs/tubes/work-state-files/20261018-native-01/verification.json`) pins156 current identities, the four accepted game/test binaries and1,251 retained historical artifact identities, preserving genuine434/433/387/386 defaults and semantic malformed refusal/recovery evidence. Rendering/path lookup caches own no orders, packets or electricity. Native434 and artwork remain unchanged. [The file handoff](r2mlvs17-e-tube-runtime.md#authoritative-capsule-file-handoff) records reproduction and explicit fixture classifications. This closes13.2 and original Group13. Continue14 lifecycle, then15 useful introduction-date routes; runtime gallery, host/client and human feedback remain separate.

### Group 14 - Rebuild tube caches and preserve recovery on lifecycle changes

Resumed2026-10-18 after completed13E/original Group13. The earlier finalization skip is historical. Actual external/native source components and canonical native load/lifecycle hooks already exist. Extend only missing focused evidence in original task order. Stage14B below for14.1's pure loaded-cache boundary;14.2 topology invalidation,14.3 ownership and14.4 queued/power-starved files remain separate subsequent tasks. No whole-group checkbox is inferred from an earlier source fixture.

{{HARD}}
- [X] 14.1 Load authoritative capsule orders/position/cargo and utility grant state before rebuilding connectivity/path/energy lookup caches. Verify rebuild cannot create power or move cargo.
- [X] 14.2 Invalidate affected network components on guideway/interchange/depot changes and use stable traversal order; keep global rescans out of the per-tick path.
- [X] 14.3 Handle bankruptcy/ownership change with canonical company and utility lifecycle hooks. Stop or reassign capsules deterministically, retaining cargo through surviving ownership/waits with explicit fresh-source recovery; terminal no-successor liquidation uses canonical vehicle/packet destruction and preserves consumed/invoice history.
- [X] 14.4 Round-trip a queued junction and power-starved route, then compare next-tick grants, reservations and positions.

### Local Mandatory Reading - Loaded tube cache purity

_Applies to14B/original14.1; supplements Global and Art/persistence/acceptance Area._

Read completed13E authoritative file handoff, actual `AfterLoadGame` acceptance order, `AcceptLoadedTubePowerConnections`, `AcceptLoadedTubeGeothermalPowerState`, `AcceptLoadedTubePowerVehicles`, `ResetTubePowerTopology` and both original-source circuit queries. Reuse the genuine six work/switch/wait/service files. After native owners accept, compare full before/after worlds around repeated cold/warm connectivity lookups and cache resets. No native tick, topology mutation, allocation or repair may occur in this boundary. Retain original orders/physical vehicle/packets/service/invoices/grants and explicit fixture classifications. Native topology edits, ownership and queued-route continuation remain14.2-14.4.

### Group 14B - Verify loaded connectivity cache rebuilding is pure

{{HARD}}
- [X] 14B.1 Prove both original source families rebuild only derived loaded connectivity from accepted native owners, retaining all complete world chunks through repeated cold/warm lookups in both builds; close14.1 only after authoritative load ordering and existing native next-step behavior remain verified.

14B/original14.1 closure, 2026-10-18: accepted native vehicle/order/packet/map/facility owners precede shared grants, original external/native source acceptance and saved capsule bindings. The new focused cache action then queries both actual whole-consist and occupied-track components, resets only derived topology three times and repeats cold/warm lookups without a native work opportunity. External reach remains positive; native reach reflects genuine cable loss in scene3, so an always-false lookup cannot pass. All six retained work/switch/wait/service worlds preserve every126 complete chunks across each cache boundary and all39 original owner chunks on loading. Twelve cross-reader boundary pairs also match every126 chunks. Four targets build;14 focused invocations pass16 native cases/1,476 assertions (8/738 per configuration), including affected next-step continuation. [The receipt] (historical reference: `../../../../logs/tubes/cache-rebuild/20261018-native-01/verification.json`) pins87 referenced identities and four binaries,36 explicit cache resets and36 immediate warm lookup rounds. No production change, source allocation, packet movement or format/art change was needed. [The cache handoff](r2mlvs17-e-tube-runtime.md#loaded-cache-rebuild-handoff) records reproduction. Continue14.2 topology invalidation,14.3 ownership and14.4 queued/starved files in order; useful routes15 and runtime gallery/human gates remain separate.

### Local Mandatory Reading - Physical tube component invalidation

_Applies to14C/original14.2; supplements Global and Art/persistence/acceptance Area._

Read completed14B cache handoff, actual `YapfNotifyTrackLayoutChange`, rail/station/depot command notifications, `MarkTubeCircuitsDirty`, `InvalidateTubePowerTopology` and original source/facility retirement hooks. Reuse delivered recovery/late-loss tests for actual guideway split, unused grant expiry and restored original route. Add only missing genuine interchange/depot or unaffected-component evidence. Mutations must use native commands; stable bounded trackdir traversal derives reach, never another network owner or a per-tick map-plane scan. Preserve consumed history, original packets and explicit prepared-input classifications. Ownership14.3 and queued/starved file continuation14.4 remain subsequent tasks.

### Group 14C - Verify physical component edits invalidate native derived reach

{{HARD}}
- [X] 14C.1 Verify actual guideway/interchange/depot edits expire affected derived components and unused rights in stable native order, retain unrelated components/history and avoid routine global rescans; close14.2 only after both source families and existing native recovery remain checked.

14C/original14.2 closure, 2026-10-18: actual guideway split/rebuild, tube platform/conventional interchange edits and destination-depot replacement now exercise both original source families and both selected consumer types. Native rail/station/depot notifications dirty affected components and expire only their unused shared rights; generated/consumed history, external invoices, original native source identity and packet17 remain intact. Distant unrelated guideway/conventional berth edits retain an unaffected original grant. Repeated queries and ordinary allocation retain native route revision; stable map/set/trackdir traversal is bounded to dirty components, with no per-tick map-plane scan. Four targets build;16 isolated invocations pass34 native cases/18,480 assertions (17/9,240 per configuration), including prior actual moving recovery, source loss, late unused-slice revocation and binding topology. [The receipt] (historical reference: `../../../../logs/tubes/component-edits/20261018-native-01/verification.json`) pins41 current referenced identities and four binaries. No production change was required. The station fixture explicitly removes only an unused branch cable before construction, obeying the native no-hidden-utility-port rule; the original inlet/path remains unchanged. Native434/GRF are unchanged. [The component handoff](r2mlvs17-e-tube-runtime.md#physical-component-edit-handoff) records reproduction and findings. Continue14.3 ownership, then14.4 queued/starved files and15 useful routes; runtime gallery and human gates remain separate.

### Local Mandatory Reading - Canonical company and tube permission lifecycle

_Applies to14D/original14.3; supplements Global and Art/persistence/acceptance Area._

Read actual `ChangeOwnershipOfCompanyItems` vehicle/utility/tube hook order, `ChangeTubePowerCompanyOwnership`, native geothermal source retirement, original shared utility ownership and `RetireTubePowerVehicle`. Reuse existing company-takeover/source-lifecycle and recipient-retirement tests. Extend missing actual loaded-capsule takeover/recovery and bankruptcy/removal evidence through canonical hooks, preserving surviving native packet/order/position identity and consumed/invoice/carry history. Declare native entity/permission ID reuse separately; no old source permission may revive on a successor or a replacement facility. Characterize canonical terminal company liquidation separately from a surviving takeover, without inventing ownerless vehicles or a second cargo store. Queued/starved files14.4 remain the next task.

### Group 14D - Verify company changes and original power permission recovery

{{HARD}}
- [X] 14D.1 Verify actual loaded capsule/source ownership changes and bankruptcy/removal through canonical company/utility hooks, preserving surviving cargo and original financial/work history with explicit terminal-liquidation and recovery policies; close14.3 only after both original source families and unchanged native ownership contracts pass.

14D/original14.3 closure, 2026-10-18: canonical takeover now retains a surviving actual front's powerless association and original demand carry while every old tube service permission/unused grant retires. Both live and already-closed sources, expired current shares and repeated buyers preserve native packet17, ordinary orders/position and exact original financial history. Explicit fresh-source/reconnect commands earn new paid native travel and arrival. A reproduced stranded-on-track defect required preserving the recovery association; a further closed-external-source ownership mismatch required coupling only its live typed binding's original producer/recipient in the sole shared ownership graph. No quantities, source permissions or inactive tombstones are created/reactivated. Canonical terminal liquidation destroys actual vehicles/packets, expires unused repair grants and preserves generated/consumed history; real company/vehicle ID reuse cannot bind former services. This dated native terminal policy clarifies14.3's surviving-cargo requirement rather than inventing ownerless stock. Four targets build;22 isolated invocations pass60 native cases/23,900 assertions (30/11,950 per configuration), including shared utility/Heat/feed/path company producers and focused original-file cache/next-step compatibility. All39 original file owner chunks,126 pure cache chunks and four126-chunk cross-reader output pairs match. [The receipt] (historical reference: `../../../../logs/tubes/company-lifecycle/20261018-native-01/verification.json`) pins79 referenced identities, four binaries and both reproduced failures. Native434 and packed artwork remain unchanged. [The company handoff](r2mlvs17-e-tube-runtime.md#company-custody-and-permission-handoff) records explicit prepared-input classifications and reproduction. Continue14.4 queued/starved files, then15 useful routes; gallery, actual host/client and human gates remain separate.

### Local Mandatory Reading - Genuine queued and powerless native route files

_Applies to14E/original14.4; supplements Global and Art/persistence/acceptance Area._

Read completed14B/14C/14D and13E handoffs, actual native depot/path reservation driver and existing `transport_tube_reservations.cpp` fixtures. Verify existing producer signatures/tests before depending on queue behavior. A genuine queued junction must be earned by normal orders/ticks and actual competing reservations; the13B manual reservation display fixture is not this scenario. Reuse the genuine13E cable-wait checkpoint for independent powerless-route coverage where its original14.4 fields are already established. Compare complete original vehicles/orders/packets, shared grants, native reservation map fields and positions across repeated/opposite-reader loads and the next actual common opportunity. Keep all participating fronts in the same real stable native tick order; no private relocation, fabricated grant or second packet/reservation owner. Source/cargo bootstrap and five-model introduction-date useful routes remain15.

### Group 14E - Accept real queued junction and powerless-route continuation

{{HARD}}
- [X] 14E.1 Produce actual ordered competing-junction custody, then verify queued and genuine power-wait native files with repeated/opposite-reader reload and exact next common grants/reservations/positions; close14.4 only after complete native owner evidence in both builds.

14E staging, 2026-10-18: separate the actual competing-route/checkpoint producer14EB from ordinary full-file/repeated/all-front continuation14EC before implementation. The existing reservation producer passes9,549 assertions/3cases in each current build; it earns real order-driven movement and native path claims rather than private reservation bits. Native PBS retry settings must use legal saved values (wait_for_pbs_path minimum2, path_backoff_interval minimum1), avoiding afterload clamp differences. Original14E.1/14.4 stay unchecked until both stages pass. A genuine native Heat-path loss can additionally supply a grant-starved checkpoint with electrical geometry intact; retain13E cable waiting as distinct earlier compatibility evidence.

### Local Mandatory Reading - Actual competing route and native queue checkpoints

_Applies to14EB; supplements Global, Art/persistence/acceptance Area and14E native file scope._

Read actual reservation producer, native GetReservedTrackbits/GetTrainForReservation and original ordered-front tick driver, plus the existing serialized geothermal work writer pattern. Reuse original native sources/depots and actual shared output, buy both real fronts and physical junction/target through commands, and earn queue ownership by normal orders/ticks. Use legal native PBS/physics settings and original packet IDs. Save actual queued and genuine grant-starved states plus uninterrupted common next opportunities; repair through the original pipe owner and demonstrate actual queue clearing. No physical relocation, reservation-bit write, synthetic energy or second source ledger. Ordinary readers/repeated/all-front acceptance belong14EC.

Context has compacted 1 times while executing this group.

### Group 14EB - Produce real competing junction and grant-starved checkpoints

{{HARD}}
- [X] 14EB.1 Earn actual powered competing-junction custody and original-Heat-loss grant starvation, write complete paired current native checkpoints/next opportunities, then restore the original path and drive both original packets through normal queue clearing in both builds.

14EB closure, 2026-10-18: actual bought Upper+Right junction32,25 (mask36) merges both original native source depots into a paid common destination. Normal orders/ticks earn a real departed reservation winner while the other powered started front is natively Stuck. Removing only the original Heat link produces actual Grant waiting with both electrical components intact; repairing it clears both original packet17 arrivals in139 real common opportunities. Four targets build; two independent native writers pass9,406 assertions/two cases, and all five original world pairs match every126 complete native chunks. [The producer receipt] (historical reference: `../../../../logs/tubes/queue-files/20261018-native-01/producer-verification.json`) pins current artifacts/builds, unchanged434/artwork and explicit prepared-floor/material/packet boundaries. No physical relocation, reservation write or synthetic output. Continue14EC ordinary/repeated/opposite-reader/all-front file acceptance;14E.1/original14.4 stay unchecked.

### Local Mandatory Reading - Full native queued and starved file consumers

_Applies to14EC; supplements Global and Art/persistence/acceptance Area._

Read completed14EB actual checkpoints and existing ordinary full-application work observer. Load all authoritative participants before rebuilding derived caches, tick every original front in stable native order through one common allocation, and compare full vehicles/orders/packets/grants/reservations/positions against each genuine uninterrupted baseline. Preserve original source/pipe/packet histories, retained earlier inputs and precise native GLOG load timing; no ownership/quantity normalization. Complete original14.4 only after repeated/opposite-reader and next-step evidence in both builds.

### Group 14EC - Accept ordinary queued and grant-starved file continuation

{{HARD}}
- [X] 14EC.1 Verify14EB current files through repeated and opposite native readers, including all-front next common opportunities and complete reservation/grant/position transport; retain original cable-wait compatibility and close14E.1/original14.4 only after full owner evidence passes.

14EC/14E/original14.4 closure,2026-10-18: both current native readers preserve both actual competing front/packet identities through ordinary queued, genuine Heat-grant-starved and actually cleared files. Repeat saves preserve126 complete chunks; next common opportunities tick both native fronts in original pool order after one shared allocation and match39 complete owner chunks,125 complete world chunks and independently exact diagnostic GLOG load timing. Original cable-Topology waiting passes separate current compatibility. Four targets build;28 native cases/2,496 assertions pass (14/1,248 per build), and all14 cross-reader boundary pairs match every126 chunks. [The current receipt] (historical reference: `../../../../logs/tubes/queue-files/20261018-native-01/verification.json`) pins exact reports, immutable producer inputs and current binaries/builds. Native434/artwork unchanged; no production mutation or quantity normalization. Original Group14 is now complete. Continue15.1 refusals, then15.2 useful introduction-date routes; runtime gallery and human/operator gates remain separate.

### Group 15 - Art, persistence and acceptance

Goal-authorized skip (2026-10-08, master finalization): dispatched expansion work in this group is skipped under [finalization exception GP-TG](portal-tbm/r2mlvs3mp-z-a-finalization-ledger.md#gp-tg-tube-and-geothermal-expansion-requires-major-gap-planning). Existing delivered prefixes stay complete; unimplemented profile work remains pending. No skipped task is markedX.

{{MEDIUM}}
- [X] 15.1 Test solid/unlocked-layer refusals, pre-1980 Deep access, ordinary train/tube incompatibility, unsafe work-track replacement, disconnected power and illegal cross-layer routing.
- [X] 15.2 Run one useful freight route with each of the five models at its introduction date. Include Underground-to-Deep chains with explicit conventional transfer and distinguish all five silhouettes in runtime captures.

15 staging,2026-10-18: execute15A refusal evidence before useful-route15.2. Existing construction/conversion/admission/native power producers cover solid and locked floors, dates, incompatible domains, protected support and disconnected grants. Add a focused actual command-bought capsule refusal against an independently valid opposite rock-layer tube and real portal endpoints, preserving original packet/physical/source owners. These adversarial boundary cases do not establish a useful freight route.

### Local Mandatory Reading - Tube refusal and layer separation boundaries

_Applies to15A/original15.1; supplements Global and Art/persistence/acceptance Area._

Read actual construction/conversion/prototype/admission-date/power/meter tests and public CheckTubeConstruction, CheckTubeRailConversion, CheckTubeVehicleAdmission, CanTubeTrainEnterTile. Existing native tile entry/followers must deny cross-layer and conventional portal use independently of broad mechanical masks; no fixture relocation or private reservation changes in new cases. Use actual command-built valid Underground/Deep tube cells and original native portal/source owners, with exact packet/physical/geology/cash/RNG/utility snapshots before refusals. Run each focused tag separately in both builds; empty selections and pooled combined-case state do not pass. Preserve prepared boundary-input classification and leave15.2 useful/bootstrap proof pending.

### Group 15A - Verify original tube refusal boundaries

{{MEDIUM}}
- [X] 15A.1 Verify all original15.1 refusal boundaries in both native configurations, adding only missing actual opposite-layer/portal-entry coverage with unchanged original owners; mark15.1 only after focused evidence passes.

15A/original15.1 closure,2026-10-18: all eight isolated tube refusal scopes and three affected conventional portal/calendar/TBM scopes pass in both builds, totaling76 native cases/381,540 assertions (38/190,770 per build). The new loaded real-capsule case checks a legal opposite rock-layer tube and both original conventional shaft endpoints while retaining original packet17, physical/orders, source, geology, cash, clock and RNG. It exposed two missing native shaft-callback refusals; the callback now delegates existing whole-consist domain admission before magazine/visibility handling, with no new saved field or quantity owner. Four targets build; [the receipt] (historical reference: `../../../../logs/tubes/refusal-boundaries/20261018-native-01/verification.json`) pins current reports/builds and the reproduced failure. Native434/GRF unchanged. Prepared floor/commissioned portal/stock and broad mechanical masks are explicit boundary inputs. Continue15.2 useful introduction-date routes; rendering fixtures and refusal checks do not establish that result.

15.2 staging,2026-10-18: execute one model at a time at its actual introduction date. Start15B with1970 Mole: the delivered finite Underground Coal mine, real tube transfer, conventional wagon/portal and1950 Surface refinery intake already have separate producers. The earlier12D fixture explicitly relocates local arrivals and skips orders; it is a producer gate, not this autonomous route.15BB must earn every arrival/departure/transfer through normal orders, physical track and ticks with paid external power.15BC accepts its actual ordinary worlds and runtime silhouette. Then stage1975 Crate,1980 Cistern,1985 Shield and2005 Forge in order with verified real cargo source/receiver availability; later conventional Underground/Deep chains must respect the1980 gate. Original15.2 remains unchecked until all five useful routes, explicit conventional inter-layer transfers and distinct actual captures pass. Disclosed prepared floor/machinery/commissioned portals in these bounded route cases do not close Group16's actual TBM excavation/bootstrap scenario.

### Local Mandatory Reading - First autonomous useful Mole route

_Applies to15B/15BB; supplements Global and Art/persistence/acceptance Area._

Read existing TubeInterchangeRig/TubeFactoryOrdersRig and their explicit Arrive/OrderedArrival/FinishOrdered/DriveFreightToSurface staging limitations; reuse native station/loading/packet/payment owners without those pose/skip operations. Read MineAdmissionRig, actual ProduceLayerMineCargo/DispatchLayerMineCargo, CFMT native processor admission and1970 model9/1950 model4/5 calendar contracts. Verify the focused ordered-freight/mine-production/processor producers before dependent mutations. Build actual paid native tube/conventional geometry and external source, start actual purchased fronts through commands, and advance all original fronts in stable native order after one common power preparation and ordinary loading scheduler. Use actual finite mine packets and exact station/recipient receipts; no synthetic arrival, current-order replacement, cargo injection or free geothermal bootstrap. Full TBM floor/first-plant bootstrap remains Group16.15BC adds ordinary file/runtime capture consumers after the route producer.

### Group 15B - Deliver the1970 Mole useful route

{{HARD}}
- [X] 15B.1 Complete15BB's autonomous native useful route, then15BC's ordinary file/runtime silhouette acceptance; retain exact original packet/source/payment identities and actual1970 availability.

### Group 15BB - Drive original Mole cargo through actual native orders

{{HARD}}
- [X] 15BB.1 Build actual1970 tube/source/interchange and conventional Surface receiver geometry, earn every loaded/empty arrival by normal native orders/ticks, and verify conserved original finite mine packets, one paid grid source and one final factory intake/payment in both builds.

15BB closure,2026-10-18: the actual1970 Mole loads20 genuine finite Coal units, transfers the same original packet to a joined conventional wagon, and that wagon physically crosses the original shaft to native Surface CFMT intake. Both complete empty home-depot returns and two further real economy days preserve20 received units and exactly one55-Money final income. The sole original external source consumes683,997J; no geothermal bootstrap, local arrival, order skip or cargo injection. Native timers and CallVehicleTicks own daily processing, loading, common power, movement and deferred depot restart. Fixture-driver errors exposed a stacked TileY misuse, omitted deferred restart and frozen day fraction; these were corrected without production changes. Four targets build; both independent runs match exact cargo transition ticks491/783/1655/2039, route2330/final2848, and18 focused producer/route cases pass45,962 assertions (nine/22,981 each). [The producer receipt] (historical reference: `../../../../logs/tubes/useful-routes/20261018-native-01/mole-producer-verification.json`) pins current sources/binaries/logs. Native434/artwork unchanged. Prepared floor/completed machinery/commissioned portals and explicit positive fixture prices remain boundary inputs, separate from Group16 TBM/bootstrap and balance. CFMT receives useful Coal fuel; no metal-output claim is made without its Copper co-input. Continue15BC ordinary native files and real Mole silhouette;15B.1/original15.2 remain unchecked.

### Local Mandatory Reading - Autonomous Mole files and actual runtime capture

_Applies to15BC; supplements Global and Art/persistence/acceptance Area._

Read completed15BB handoff and current MoleUsefulRouteRig/native full timer/CallVehicleTicks driver. The older14EC step-all observer advances only raw fronts and does not match this complete driver; do not substitute it for next-opportunity baselines. Reuse ordinary full-application observer/save patterns and native viewport/body-pixel helpers, with exact original20-unit packet identities,1970 engine/cargo mappings, current native orders, source/binding/invoice/utility clocks and conventional position/ownership. Capture actual loaded moving capsule and conventional crossing worlds, preserve complete native chunks through repeated/opposite-reader and next common opportunity, and raster the actual Mole with existing packed source/anchor/picking rules. No fixture pose or manual stock grant. Actual driver/bootstrap boundaries and CFMT input-only scope remain disclosed. Other four models and Underground/Deep conventional chain retain later tasks.

Context has compacted 1 times while executing this group.

### Group 15BC - Accept ordinary useful Mole files and runtime silhouette

{{HARD}}
- [X] 15BC.1 Write actual loaded-capsule/conventional-crossing/final useful-world checkpoints and full-driver next opportunities, then verify current ordinary repeated/opposite-reader transport and complete participant continuation in both builds.
- [X] 15BC.2 Verify actual loaded moving Mole runtime pixels/picking and world purity from the preserved route, then close15B.1 only after its exact native file/render evidence passes; original15.2 remains pending for the other models and inter-layer chain.

15BC.1 file checkpoint,2026-10-18: two independent current writers retain four genuine useful-route states and each uninterrupted next full-native opportunity. Both opposite readers preserve39 complete authority chunks, repeat all126 and match125 next-opportunity chunks plus independently exact diagnostic GLOG load timing. All eight original writer and16 reader boundary pairs retain126 complete chunks. Four targets build;36 focused writer/reader/affected route cases pass90,720 assertions (18/45,360 each), with114 exact referenced identities in [the immutable file receipt] (historical reference: `../../../../logs/tubes/useful-files/20261018-native-01/mole-files-verification.json`). The only setup refusal was a fixture town's unset name generator; canonical native metadata fixes its new writer, with invalid old inputs retained unmodified. No physics/quantity normalization or production change. Native434/GRF unchanged. Continue15BC.2 actual Mole runtime pixels/picking;15B.1/original15.2 remain unchecked.

15BC/15B closure,2026-10-18: both opposite ordinary readers earn the same real moving20-unit Mole midpoint in28 complete native opportunities, consuming10,080J before pure capture. Actual normal/In2x repeated native rendering and correct-layer picking/Surface refusal preserve every126 complete chunks; all three cross-reader pre/post/final world pairs and four whole640x360 RGB frame pairs match exactly. Original hull accounting covers1,080 visible pixels and10 exact source-rim occlusions across builds, preserving every non-company hull point and all53/212 interior points per zoom/build. No pose, pixel mask, tolerance, art regeneration or production change. Both configurations pass12 focused native cases/57,556 assertions (six/28,778 each), and all four targets build. [The immutable raster receipt] (historical reference: `../../../../logs/tubes/useful-files/20261018-native-01/mole-raster-verification.json`) pins exact current reports/source/binaries/builds and the original15BC.1 file receipt. Native434/GRF unchanged. Agent inspection reviewed the real capture; human acceptance is not inferred. Continue1975 Crate, then1980 Cistern,1985 Shield,2005 Forge and the explicit conventional Underground/Deep chain. Original15.2 and Group16/bootstrap/runtime-gallery/operator gates remain pending.

### Local Mandatory Reading - Actual1975 Goods source before Crate routing

_Applies to15C/15CB; supplements Global and Art/persistence/acceptance Area._

Read actual native original industry production/distribution in OnTick_Industry/TransportIndustryGoods and native economy TriggerIndustryProduction, original Forest/Sawmill definitions and cargo-label resolution. Read the completed15BB full-native timer/CallVehicleTicks driver and actual station/order/build helpers, preserving real depot restart and source provenance. Native1975 manufacturing recipes do not yet supply Electronics/Batteries/mirror kits; purchased construction-purpose packets cannot replace ordinary paying Goods. Verify a real original Surface Forest-to-Sawmill Wood shipment and Goods output through native production, normal orders/loading and physical travel before dependent Crate routing. A normal funded town or explicitly disclosed prepared native town/house commands must establish actual same-layer Goods acceptance; no raw acceptance flags or invented cargo. Prepared completed industry footprints/town buildings remain boundary inputs, not earned cargo or upstream bootstrap. Keep this Surface producer separate from the subsequent actual conventional-to-tube-to-conventional route.

### Group 15C - Deliver the1975 Crate useful route

{{HARD}}
- [X] 15C.1 Complete15CB's actual1975 source/receiver and15CBB's autonomous Crate route, then15CC ordinary files/runtime silhouette; preserve original Goods provenance and useful town delivery without purchased-purpose income.

### Group 15CB - Earn actual1975 Goods and verify its receiver

{{HARD}}
- [X] 15CB.1 Verify actual native1975 Surface Wood production, driven Sawmill intake/Goods output and a real Goods-accepting town boundary in both builds before dependent Crate routing.

15CB closure,2026-10-18: both independent actual1975 original Forest/Wood-train/Sawmill runs earn39 Goods and the complete empty return. Unchanged original Forest dispatch requires more than30 units; three real13-unit batches over767 industry opportunities supply39, never a lowered threshold or injected stock. Native ordinary orders physically deliver both20/19-unit wagon loads over3,385 route ticks and earn84 Money Wood income. A real company-funded native town has29 generated houses and clear berth3117 with actual Goods catchment acceptance. Reduced fixture startup now resolves original industry/tile/house/engine cargo labels and initializes native global house counts, as normal startup does; all initial refusals/crash remain dated diagnostics. No production change or actual Crate/town delivery claim. Two current test targets build and two isolated cases/30,028 assertions pass (one/15,014 each), with identical original observations in [the immutable source receipt] (historical reference: `../../../../logs/tubes/goods-source/20261018-native-01/verification.json`). Native434/GRF unchanged. EarnTubeGoodsSource retains this exact source world for its next consumer; prepared completed machinery/native town and positive prices remain boundary inputs. Original15CB.2 is moved unchanged in scope to15CBB.1 below before route implementation, keeping source/route reading and compaction scope bounded.

15C staging,2026-10-18: the ordinary native Forest/Sawmill path is the candidate1975 Goods source; it must pass15CB.1 before any dependent useful-route claim. Native Surface Electronics/Battery manufacturing and mirror kits unlock1980, while earlier paid seeds retain construction purpose. Do not advance their dates or erase purpose to manufacture a passing1975 case. Actual source and town/house boundary setup stay disclosed;15CC files/runtime capture is staged only after this route's complete participants exist. Other later models and the explicit Underground/Deep chain remain original15.2.

15CB whole-return strengthening,2026-10-18: the first dependent native sale correctly refused a locomotive whose wagon was still entering the home depot (crate-route030551-80b691ceda). The source gate now requires native IsChainInDepot, rather than locomotive-only IsInDepot, and both builds earn the entire stored consist after3,402 route ticks. Two current native cases/30,130 assertions (one/15,065 each) pass with the same39 Goods,84 Wood income and real town demand in [the strengthened immutable receipt] (historical reference: `../../../../logs/tubes/goods-source/20261018-native-01/verification-2.json`). The earlier receipt remains a dated observation; this stronger receipt is the current source gate. Crate retirement additionally stops the actual full stored train through StartStopVehicle before its native sale; no tail is relocated or deleted to earn the return.

### Local Mandatory Reading - Autonomous1975 Crate and original Goods custody

_Applies to15CBB; supplements Global and Art/persistence/acceptance Area._

Read completed15CB's source receipt and EarnTubeGoodsSource/TubeGoodsSourceRig: original39 Goods remain at the same Sawmill after two actual Wood shipments and empty return. Its generated native town supplies real Goods demand at berth3117; later construction must preserve actual acceptance. Read only15BB's native paid track/station/portal/order helpers, shared full-native timer/CallVehicleTicks driver and existing joined tube/conventional transfer stock owners. Build an actual Surface-to-Underground conventional feed, layer-local1975 model10 Crate route and conventional return to Surface town; the capsule never traverses either shaft. Retain original Sawmill source and first station, ordinary packet split/rejoin identity, payment and complete native participant ownership. Any Wood-train retirement must use actual native empty-depot sale after its earned return. No fixture cargo, arrival, order skip or source-purpose change. Preserve prepared floor/commissioned portal/town boundary classification; paid TBM/bootstrap remains Group16. Verify current source gate before dependent mutation; its immutable receipt becomes dated source/binary evidence after later test changes.

### Group 15CBB - Drive the actual1975 Crate useful route

{{HARD}}
- [X] 15CBB.1 Drive that original Goods through conventional Underground transfer, the paid layer-local Crate route and a real conventional Surface town delivery, retaining native packet/payment identities and empty returns; no source injection, relocated arrival or order skip. This is original15CB.2's unchanged scope, split after the completed source gate.

15CBB closure,2026-10-18: actual1975 source production and two driven Wood shipments earn39 original Sawmill Goods. Separate conventional feed, original paid Underground Crate and final conventional Surface train physically deliver all39 to the native town, earn137 Money useful Goods income and complete all three whole-consist empty returns. Native movement consumes1,217,315J; both runs match route3,510/final7,827 ticks and one real retained quarter. Current/history native accounting proves no pre-final transfer revenue and no repeat after148 additional empty opportunities, without freezing or normalizing time/quantities. Estimate-selected real berth44,39 and native level crossings preserve all29 demand houses; current acceptance and actual town ownership are rechecked. Source train retirement uses actual stop/sale after its whole stored return. Initial station refusal exposed missing prepared end approaches; native sale exposed and strengthened the source gate's locomotive-only return; current-quarter scalar failures correctly reflected native history rollover. No production or art changes. Four targets build; eight focused native route/source/Mole/transfer cases pass178,576 assertions (four/89,288 each) in [the immutable route receipt] (historical reference: `../../../../logs/tubes/crate-route/20261018-native-01/verification.json`). Native434/GRF unchanged. Continue15CC ordinary files and actual Crate silhouette before15C.1; other model/date routes, Underground/Deep chain, TBM/bootstrap and human/runtime-gallery/operator gates remain pending.

### Local Mandatory Reading - Actual Crate files and runtime silhouette

_Applies to15CC; supplements Global and Art/persistence/acceptance Area._

Read completed15CBB's route receipt and current CrateUsefulRouteRig/CrateNativeAccounts/EarnTubeGoodsSource plus the shared full-native timer/CallVehicleTicks helper. Preserve five actual train parts, three fronts, original Sawmill/Forest/native town and every live Goods packet, including legitimate native split identities. Native quarter turnover is real: current and historical company income/delivery records are owners, never a scalar to reset. Read only15BC's ordinary full-application observer/save/baseline patterns and actual packed body/palette/viewport/picking helpers; its fixed1970 Coal/20-unit/single-packet expectations do not apply. Capture loaded conventional Surface/Underground feed, actual moving Crate, loaded conventional Underground/Surface delivery and final useful/empty-return worlds, each with untouched next full-driver opportunity. Require repeated/opposite-reader and complete participant continuation, including town/industry/accounting owners and precise diagnostic load-event timing. Then raster the actual loaded Crate with original packed source/anchor/rim rules, correct-layer picking and pure complete worlds. No fixture pose, stock grant, original-purpose rewrite, clock/financial normalization or synthetic arrival. Prepared source machinery/town/floor/commissioned portals and the source producer's bounded opportunities remain disclosed; actual TBM/bootstrap belongs to Group16.

Context has compacted 1 times while executing this group.

### Group 15CC - Accept ordinary useful Crate files and runtime silhouette

{{HARD}}
- [X] 15CC.1 Write actual loaded conventional feed/Crate/conventional delivery and final useful-world checkpoints with uninterrupted full-native next opportunities, then verify current ordinary repeated/opposite-reader loading and complete participant continuation in both builds.
- [X] 15CC.2 Verify the actual loaded moving Crate's native pixels/picking and complete world purity from its original preserved route, then close15C.1 only after exact native file/render evidence passes; original15.2 retains the three later models and inter-layer chain.

15CC.1 closure,2026-10-18: six actual loaded modal/final worlds and six untouched next full-native opportunities pass both ordinary opposite readers and repeats. Fifty complete first-load owner chunks include original industries/towns/company current-and-history accounts, packets, clocks/RNG, cargo payments and all native definition mappings; repeats preserve126 chunks and next opportunities preserve125 plus independently precise GLOG Load timing. All36 independent writer/reader complete-world pairs match126 chunks. Four targets build;56 focused native cases pass291,350 assertions (28/145,675 each) with172 retained identities in [the immutable file receipt] (historical reference: `../../../../logs/tubes/crate-files/20261018-native-01/verification.json`). Producer-only diagnostics exposed missing company face metadata (two legitimate load-time RNG draws), invalid zero tile-loop cursor and reduced native cargo/catalogue/price setup. Correct producer initialization now uses native original cargo definitions, complete public native catalogues, valid face/cursor and native tariffs/recurring costs from saved unit inflation; readers perform no repair. Fresh actual source/Crate/Mole bursts pass. Original39 Goods remain conserved; current native source Wood income426 and final Goods income844 replace earlier fixture-tariff observations, with1,187,878J and unchanged route3,510/final7,827 ticks/one retained quarter. Earlier receipts/diagnostic files remain immutable dated evidence. Native434/packed art unchanged;15CC.2 and15C.1 stay pending.

15CC/15C closure,2026-10-18: both opposite ordinary readers retain39 original Goods and earn the same actual loaded moving Crate inspection point in six full-native opportunities, tick5,459 to5,465 and2,418J. Normal/In2x repeated native frames and correct-layer picking/Surface refusal preserve all126 complete native chunks. Three cross-reader before/after/final world pairs and four complete640x360 RGB frame pairs match. Every239 non-company hull point is accounted for at each zoom/build:238 visible/one exact original rim at normal,952 visible/four rim at2x, with every155/620 interior point visible. Total2,380 visible/10 rim pixels pass without pose, masks, tolerance or art/production changes. Both builds pass12 focused raster/body/geometry/Crate/loaded-step cases/120,642 assertions (six/60,321 each), and all four targets build. [The immutable raster receipt] (historical reference: `../../../../logs/tubes/crate-files/20261018-native-01/raster-verification.json`) pins107 identities and the original file receipt; exact final reports/logs are in raster-run-manifest.json. Agent inspection reviewed the native capture; human acceptance is not inferred. Original15.2 retains Cistern1980, Shield1985, Forge2005 and the conventional Underground/Deep chain; actual TBM/bootstrap and runtime-gallery/operator gates remain pending.

### Local Mandatory Reading - Actual1980 liquid source before Cistern routing

_Applies to15D/15DB; supplements Global and Art/persistence/acceptance Area._

Read completed15CC's native content/economy initialization findings, shared full-native timer/CallVehicleTicks helper and original-owner conservation patterns. Read Cistern model11's actual Liquid CHEM/PROP contract, CHEM's1950 two-Salt/two-Oil to two-Chemicals recipe and PCHM's1980 two-Chemicals to two-Propellant recipe in multilayer_vehicle_catalogue.cpp/multilayer_recipe.cpp. Verify live ProduceLayerMineCargo, AdvanceLayerProcessor/DispatchLayerProcessorCargo and native original Oil-well production/distribution; reduced label-only fixtures do not supply correct custom cargo physics/refits. Prefer actual ordinary native content startup before any new source command or work; a prepared empty-world transport input may initialize the app, but earns no cargo or useful-source claim. Verify a real original liquid wagon's native purchase/refit and conventional portal admission; do not broaden model5's solid-cargo contract, copy a registry, invent tanker IDs or normalize loaded cargo definitions. All Salt/Oil must be earned by native producers and physically delivered through normal orders/actual holds before chemical output can pass. Preserve unused inputs, every real packet and original source provenance. Prepared floor/completed machinery/commissioned portals remain disclosed boundaries; actual TBM/bootstrap remains Group16. Current source work reads only these symbols and source setup; route/render details stay with later locals.

### Group 15D - Deliver the1980 Cistern useful route

{{HARD}}
- [X] 15D.1 Complete15DB's actual1980 liquid source,15DC's autonomous Cistern/conventional useful route and15DD ordinary files/runtime silhouette, preserving genuine CHEM/PROP form and original source/receiver ownership. Original15.2 stays pending for Shield/Forge and the Underground/Deep chain.

### Group 15DB - Earn actual1980 Chemicals and verify liquid carriage

{{HARD}}
- [X] 15DB.1 Verify the actual native source/receiver and conventional liquid-carrier contracts, then earn chemical output from finite Salt and original Oil production through physically driven conventional delivery and the real CHEM processor. No input/output stock grant, purpose rewrite, manual arrival or order skip.
- [X] 15DB.2 Verify the complete original source world, legitimate remaining inputs/packets, useful chemical output and whole empty returns in both builds, retaining the same actual participants for the Cistern consumer. Mark the source gate only after exact focused native evidence passes.

15DB staging,2026-10-18: ordinary startup and native original-tanker admission are a separate delivered prerequisite; the remaining source combines two raw producers on different layers and a real all-input processor. Execute15DBA-D in order, retaining each actual native world. This keeps initialization, finite source production, physical input carriage and final acceptance bounded without reducing either original source task. All source tasks remain unchecked until their full native output/return scope passes.

### Local Mandatory Reading - Native Cistern carrier and source-owner prerequisite

_Applies to15DBA; supplements Global and Art/persistence/acceptance Area._

Read tube_cistern_source.cpp/cistern_source_checks.py only for empty native input, native saved-setting defaults/StartupEconomy, original Oil-wagon label/refit selection and pure1980 source estimates. Actual ordinary loading supplies custom cargo properties; reduced unit CargoSpec fields cannot supply liquid semantics. Read current mine-producer/factory-producer executable tags as the cross-plan gate. This stage creates no source stock and proves no shaft travel; native template/carrier worlds must agree completely across builds.

### Group 15DBA - Accept ordinary native liquid-carrier prerequisites

{{HARD}}
- [X] 15DBA.1 Verify native default scenario initialization, actual ordinary Liquid CHEM properties, original tanker purchase/refit, pure actual1980 source estimates and current finite-mine/processor producer gates in both builds; retain exact complete empty/carrier worlds without claiming useful source production.

15DBA closure,2026-10-18: ordinary startup supplies native Liquid/freight CHEM and selects immutable original tanker engine30 by actual label/refit mask. Real purchases/coupling create head0/tank1 with30-kilolitre Chemicals capacity, stored empty and no packets. Native default saved settings include unfrozen calendar and nonzero pathfinder budgets; StartupEconomy initializes actual saved economics before work. Pure1980 Oil Wells11/CHEM25 estimates preserve cash/RNG/industry count, and actual Underground SALT deposit4 starts29,32 for the pinned seed. Both complete empty/carrier cross-build worlds retain126 chunks. Current extraction/processor producer tags also pass. Eighteen focused native cases/2,390 assertions pass (nine/1,195 each), with successful current test builds and exact reports in [the immutable carrier receipt] (historical reference: `../../../../logs/tubes/cistern-source/20261018-native-01/carrier-verification.json`). No source stock, actual shaft crossing, chemical output or useful Cistern route is claimed. Execute15DBB from these original carrier worlds; parent15DB.1/.2 and15D.1 remain unchecked.

15DBA strengthening,2026-10-18: actual three-facility source construction exposed insufficient original author funds. The empty scenario now authors16000000 before any purchase/production while retaining native prices/defaults. Both refreshed carrier/source-owner sequences again pass18 cases/2390 assertions with two complete126-chunk cross-build world pairs. [Carrier receipt A2] (historical reference: `../../../../logs/tubes/cistern-source/20261018-native-01/carrier-verification-2.json`) retains the new exact original inputs; A1 remains immutable dated evidence. Current source work consumes A2 opaquely after subsequent test builds, never repairs loaded cash or inventory.

### Local Mandatory Reading - Earn original Cistern precursor stocks

_Applies to15DBB; supplements Global and Art/persistence/acceptance Area._

Read completed15DBA's exact receipt and accepted carrier worlds, current ordinary observer and full-native timer/CallVehicleTicks helper. Reacquire the two original stored empty carrier parts after ordinary loading; no unit fixture or cargo registry reset. Current native queries select original Oil Wells type11 at7,14, real CHEM type25 at26,15 and finite Underground SALT deposit4 whose first span starts29,32 for the pinned seed. Treat those as observed identities, revalidate source meanings and use native IDs from the actual retained report, never hard-coded assumptions. Source helper reads native three-by-three mine machinery/two-by-two loading admission, actual original Oil rate/dispatch minimum and ProduceLayerMineCargo/DispatchLayerMineCargo. Native Oil funding estimates already permit1980 despite its world-generation Before1950 flag. Complete machinery/floor/reciprocal Connected shafts are explicit prepared boundaries. Earn Oil before funding the Salt mine if using bounded OnTick_Industry, so native mine production is not accidentally advanced twice. Then five real74-tick opportunities can earn20 finite Salt as in15BB; no output/rate/stock/arrival grant. Keep both trains stopped, preserve every original packet/age/purpose and native zero source-delivery counters. Prepare native routes/normal orders and save this actual stock world for15DBC.

Context has compacted 1 times while executing this group.

### Group 15DBB - Prepare and earn actual1980 Salt and Oil stocks

{{HARD}}
- [X] 15DBB.1 In the accepted ordinary carrier world, fund actual Oil/CHEM/finite Salt participants, prepare disclosed floor/shafts, command-build native stations/rail/two conventional consists and orders, and earn bounded actual Oil/Salt through their original producers while the trains remain stopped. Retain the same complete original stock world in both builds; no useful chemical output is claimed yet.

15DBB closure,2026-10-18: both ordinary observers retain the opposite A2 original carrier and command-fund three native facilities, three stations, the second real two-part conventional train and normal rail/orders. Actual Oil production earns26 units in366 native opportunities; five further74-tick finite-mine opportunities earn20 Salt and debit32000->31980. At tick736 both complete trains remain stopped/empty, original packets retain source/first station/purpose0, company cash4875832/loan0 and source delivery/income/CHEM output remain zero. Source opportunities include the normal native UpdateLandscapingLimits recovery; no loaded construction budget is rewritten. Both builds pass4770 assertions/one isolated case each, with all126 complete native output chunks exact across builds. [Immutable stock receipt] (historical reference: `../../../../logs/tubes/cistern-source/20261018-native-01/stock-verification.json`) pins reports/inputs/builds/provenance. Floor/completed machinery/Connected shaft remain explicit prepared boundaries; physical input carriage and chemical output are current15DBC, so parent15DB.1/.2 stay pending.

### Local Mandatory Reading - Drive precursor inputs into actual Chemicals

_Applies to15DBC; supplements Global and Art/persistence/acceptance Area._

Read completed15DBB's exact original stock-world receipt and native participants/orders/packet observations. The original Oil tanker and a legal solid Salt wagon keep distinct native cargo contracts; only conventional freight crosses the prepared shaft. Drive the two original fronts with full native clocks/loading/power/aging/deferred restart, advancing the actual CHEM processor once per real opportunity. Conserve each raw role across original source stocks/queues/holds/native accepted inputs and the actual2+2->2 output, retaining genuine unused Oil/Salt. Read current plus valid historical company accounts through any native quarter; do not normalize clocks or income. Complete both whole-consist empty home returns and bounded empty opportunities without replay, then preserve the exact same native world for the Cistern route.

### Group 15DBC - Earn actual useful Chemicals through native input transport

{{HARD}}
- [X] 15DBC.1 Drive the original stopped Oil/Salt shipments through normal native orders and the conventional shaft into actual CHEM inputs, earn real all-input chemical output, preserve every source/packet/payment/unused-input owner and both whole empty home returns in both builds. Close original15DB.1 only after this native output exists.

15DBC/original15DB.1 closure,2026-10-18: ordinary opposite-build loading retains every original raw packet/participant. Native orders physically carry26 Oil and20 Salt, including the loaded Salt trailer through the conventional shaft, into actual CHEM. The real2+2->2 processor earns20 Chemicals, retaining6 unused Oil/0 Salt and unchanged finite reserve31980. Oil/Salt whole empty home returns occur at ticks2886/3331;148 subsequent native opportunities preserve income446 and output without replay. Original tick736 -> final3479 includes2595 route opportunities. Actual cash4875931/loan0 and current/history native accounts are retained; no packets remain and output is undispatched. Both builds pass57339 assertions/one isolated case each, matching all126 complete native output chunks. [Immutable haul receipt] (historical reference: `../../../../logs/tubes/cistern-source/20261018-native-01/haul-verification.json`) retains exact original stock inputs/current sources/builds. Cistern movement and useful chemical receiver remain15DC; final source file/current-owner/four-target acceptance is current15DBD, so parent15DB.2 remains pending.

### Local Mandatory Reading - Verify the complete native Cistern source

_Applies to15DBD; supplements Global and Art/persistence/acceptance Area._

Read completed15DBA-C exact receipts/current source implementation and retained worlds, only the affected finite-mine/processor/ordinary loader tests and current build identities. Check both configurations' complete source states, exact original loaded transitions and actual native histories; no newly reconstructed source can replace the original retained world. Keep prepared machinery/floor/shaft and bounded production classifications explicit. Aggregate acceptance must not claim a Cistern movement, later receiver operation, paid TBM/bootstrap or human review.

### Group 15DBD - Accept and hand off the complete actual1980 liquid source

{{HARD}}
- [X] 15DBD.1 Verify current complete source/stock/unused-input/accounting/whole-return evidence and focused affected owners in both builds, build all four targets and retain an immutable exact source handoff; close15DB.2 only after every source condition passes.

15DBD/original15DB.2 closure,2026-10-18: both current ordinary readers consume the exact opposite completed source worlds from15DBC, preserving all126 original native chunks including GLOG before any work. Separate zero/one full-native opportunities retain20 Chemicals/6 unused Oil/0 Salt, native accounts/actual cash, unchanged reserve31980 and both whole stopped empty consists. Complete repeated and six cross-build before/output/repeat world pairs match all126 chunks. Current finite-mine/processor gates and file checks pass18 focused native cases/3096 assertions (nine/1548 each); all four current game/test targets build. [Immutable complete source handoff] (historical reference: `../../../../logs/tubes/cistern-source/20261018-native-01/verification.json`) retains exact A2/stock/haul lineage, original source inputs, current reports/binaries/packed art and build logs. No source is reconstructed, raw work replayed or loaded clock/account normalized. Prepared floor/machinery/Connected shaft remain disclosed; current15DC must earn actual Cistern/conventional CHEM carriage and useful PCHM output from these same original worlds. Source closure does not complete15D.1, Cistern files/raster or TBM/bootstrap/human acceptance.

### Local Mandatory Reading - Actual1980 Cistern useful route

_Applies to15DC; supplements Global and Art/persistence/acceptance Area._

Read completed15DB's exact native source/receiver/carrier receipt and same-world helper before dependent work. Reuse only completed15CC's actual paid modal construction/orders/power and full-native driving patterns. Cistern11 remains layer-local; conventional liquid wagons alone cross commissioned shafts. Use actual CHEM output as the paying ordinary shipment and a real compatible PCHM processor intake at1980 as its useful receiver, preserving every actual source/packet/payment identity. Earn all transfers, reversals and whole empty returns; conserve any chemical/propellant inventories and genuine unused upstream inputs without synthetic completion. Native recurring costs/tariffs and quarter history remain owners. Do not preload15DD files/art helpers until this route has its full original participants.

### Group 15DC - Drive the actual1980 Cistern useful route

{{HARD}}
- [X] 15DC.1 Drive the original Chemicals through conventional feed, a paid layer-local Cistern and conventional Surface processor delivery, proving useful native intake, single accounting, original physical/provenance conservation and complete empty returns in both builds.

15DC staging,2026-10-18: retain the exact completed source world and every upstream original participant. Stage native paid route/receiver acquisition separately from physical modal operation so ordinary content/date/carrier/construction refusals cannot be hidden inside a large movement case. Execute15DCB then15DCC; original15DC.1 remains unchecked until both pass.

### Local Mandatory Reading - Native capsule launch dates

_Applies to15DCBA; supplements Global and Art/persistence/acceptance Area._

Read AD-15's five canonical launch years, native StartupOneEngine and CopyTempEngineData, and GetTubeVehicleCapability/ConfigureTubeEngine. The actual ordinary source at1980-02-17 exposes Cistern's random284-day introduction delay; mechanical fixtures that forced company availability did not cover normal startup. Correct only canonical native capsule definition publication dates/availability through public native startup/load ownership. Preserve original source maps, clocks, vehicle ages/cargo, accounts and quantities. Exact original saved inputs remain immutable; a saved old randomized tube definition requires an explicit bounded ENGN compatibility comparison, never fixture availability or registry repair. Focus before/on-date tests for all five models, existing conventional startup and ordinary original source loading before resuming route construction. No new saved field/version or invented early unlock.

### Group 15DCBA - Honor original capsule calendar dates under ordinary startup and loading

{{HARD}}
- [X] 15DCBA.1 Correct ordinary native startup, live monthly publication and old saved capsule definition publication to each original canonical launch date, verify all five before/on-date boundaries and conventional isolation, and strengthen current exact-original source acceptance with the precise ENGN compatibility change before dependent route acquisition.

15DCBA closure,2026-10-18: actual normal startup/load exposed Cistern's randomized284-day delay and the live month loop's ordinary preview-year wait. Canonical native originals now use exact startup dates, existing monthly public publication and bounded normal saved-definition reconciliation. Both configurations pass all-five before/on startup340/live monthly370/saved restoration350, noncapsule isolation40 and conventional startup32 assertions, each in its own process. Current source-file/mine/processor gates raise complete acceptance to28 native cases/5380 assertions (14/2690 each), with all four successful game/test targets. The exact original source worlds remain inputs:125 complete non-ENGN chunks match,271 engine rows are accounted for, and only four actual native rows require canonical date correction; Cistern alone becomes date-valid available, future models remain locked, all other engine bytes exact. Repeated/six cross-build full-world pairs retain126 exact chunks. [Strengthened immutable source handoff] (historical reference: `../../../../logs/tubes/cistern-source/20261018-native-01/verification-2.json`) retains the same original source lineage and current corrected normal resaves. Source quantities/clock/accounts remain3479/20CHEM/6Oil/income446/cash4875931/reserve31980, without fixture repair. Parent AD-15/public handoff/architecture document this policy. Earlier source1 and failed/intermediate diagnostics remain immutable dated evidence. Resume15DCB actual command acquisition; no Cistern route-ready world or movement is accepted by this prerequisite.

### Local Mandatory Reading - Prepare the original Cistern route

_Applies to15DCB; supplements Global and Art/persistence/acceptance Area._

Read completed15DBD's exact source handoff and original source-world outputs, Cistern11's native liquid/date/refit definition, ordinary engine/rail availability and native CHEM/PCHM recipe selection. Reuse15CC's physical same-layer joined berths, conventional shafts, actual rail/depot/orders and paid ConnectTubeExternalPower only as construction patterns; no fixture registry/availability/budget reset is permitted after loading. Preserve original Oil/Salt trains/facilities and six unused Oil. New original liquid wagons alone may use conventional shafts. All new fronts stay stopped and empty; native20CHEM remains undispatched. Command-fund an actual1980 compatible PCHM facility, prepare disclosed floor/shaft boundaries and retain the exact complete paid ready world for15DCC.

One-town receiver configuration,2026-10-18: the exact original source has one native town and its CHEM facility. PCHM is another alternative of that same native facility type, so default one-industry-per-type/town refuses a second works. This private route scenario explicitly enables the ordinary saved `economy.multiple_industry_per_town` option through synchronized `ChangeSetting` before real receiver funding. Retain this declared native game-rule choice in preparation evidence; do not inject another town, change availability, reduce native prices or grant cash/materials. All original source participants/quantities remain unchanged.

### Group 15DCB - Prepare the paid native Cistern route and useful receiver

{{HARD}}
- [X] 15DCB.1 In the exact accepted original source world, command-fund the actual PCHM receiver, modal rail/berths/depots, two conventional liquid consists, model11 Cistern and paid external power under ordinary native content/date rules. Retain complete cross-build ready worlds with original source/input/participant owners and every front stopped/empty; no useful route movement is claimed yet.

15DCB closure,2026-10-18: both current ordinary observers consume the exact opposite completed source worlds under source2's native launch-calendar compatibility. Real commands enable the declared saved multi-industry-per-town option, fund a distinct Chemical Works/select PCHM, build actual modal rail/berths/depots and buy two conventional original-tanker consists plus native model11 Cistern with paid external power. All four original source parts/three facilities/stations remain; total9parts/4industries/6stations and power producer0 are actual observed identities. At tick3479 all fronts stay whole/stopped/empty, source20CHEM remains undispatched,6unusedOil/reserve/source income446 remain intact, cash3612580/loan0 after real purchases. Both builds pass528 assertions/one isolated case each and every complete output chunk matches (126). [Immutable paid-preparation receipt] (historical reference: `../../../../logs/tubes/cistern-route/20261018-native-01/prepare-verification.json`) retains original source inputs, exact new ready worlds and current builds. Prepared floor/machinery/Connected shafts and the explicit native game-rule choice remain disclosed. Current15DCC must load these actual ready worlds and earn physical useful movement/three empty returns;15DC.1 remains unchecked.

### Local Mandatory Reading - Drive the same original Cistern route

_Applies to15DCC; supplements Global and Art/persistence/acceptance Area._

Read completed15DCB's exact original nine-part/four-industry/six-station ready-world receipt, actual native orders/power binding and borrowed participant view. Dispatch only the genuinely earned20CHEM through its original producer/station, then use the complete native clock/vehicle/power/loading/payment driver and real PCHM work once per opportunity. Conserve original Chemicals across every actual buffer/queue/hold and PCHM consumption/Propellant output, preserving upstream unused Oil/reserve/accounts. Observe loaded conventional Surface/Underground feed, layer-local paid Cistern and Underground/Surface delivery from actual movement. Complete three whole empty home returns and bounded empty opportunities without replay; retain the same native world for15DD. No source fixture, pose, order skip or synthetic useful intake.

### Group 15DCC - Earn the useful Cistern modal shipment and empty returns

{{HARD}}
- [X] 15DCC.1 Drive the exact paid ready world through actual20CHEM conventional feed, layer-local powered Cistern and conventional Surface PCHM delivery, earning real useful Propellant output, single native final accounting, conserved original owners and three complete empty returns in both builds. Close original15DC.1 only after the full useful route passes.

15DCC/original15DC.1 closure,2026-10-18: both ordinary opposite-ready-world readers retain all9 actual parts/4industries/6stations/paid source and upstream originals. Normal first station service and actual producer dispatch (no rating/packet grant) earn20CHEM across conventional Surface/Underground feed, moving layer-local Cistern and conventional Underground/Surface delivery into real PCHM20PROP. Lifetime source Oil20/Salt20 consumption, one CHEM20 dispatch,6unusedOil/reserve31980 and all packet provenance remain exact. Native CHEM income427/total873, cash3607859/loan0 and real quarterhistory1 are retained; paid consumption2054808J. Actual loaded scenes occur at ticks5402/5937/6793/7641/7971, including genuine partial loading. Whole empty returns feed8950/delivery9768/Cistern9849 plus148 native empty opportunities reach9997 from original3479,6370 route opportunities, without replay. Both builds pass223220 assertions/one isolated case each and all126 complete useful-world chunks match. [Immutable useful-route receipt] (historical reference: `../../../../logs/tubes/cistern-route/20261018-native-01/verification.json`) pins original preparation inputs, exact current sources/binaries and native outputs. Prepared floor/machinery/shafts and explicit multi-industry option remain disclosed; ordinary files/runtime silhouette are current15DD, so15D.1 remains pending. No paid TBM/bootstrap, gallery or human acceptance is claimed.

### Local Mandatory Reading - Actual Cistern files and runtime silhouette

_Applies to15DD; supplements Global and Art/persistence/acceptance Area._

Read completed15DC's exact original-world receipt, shared native driver and native current/history/cargo-definition observations. Reuse15CC ordinary full-app read/preview/repeat/opposite-reader/complete-next-opportunity and original packed body/rim/palette/picking patterns, adapting only actual model11, liquid forms, participants and original states. Include every live upstream source and genuine remaining input/output owner; Crate's fixed five-part/39-unit/town expectations do not apply. Original-only saved maps/clock/RNG/economy/catalogue owners must pass without repair. Inspect actual loaded moving Cistern pixels from native movement before closing15D.1; no pose, stock, tolerance, source-art or bootstrap substitution.

### Group 15DD - Accept ordinary useful Cistern files and runtime silhouette

{{HARD}}
- [X] 15DD.1 Write actual loaded conventional feed/Cistern/conventional delivery and final useful-world checkpoints plus untouched complete native next opportunities; verify repeated/opposite-reader loading and every participant's continuation in both builds.
- [X] 15DD.2 Verify the actual loaded moving Cistern's native pixels/picking and complete-world purity, then close15D.1 only after exact file/render evidence passes. Original15.2 retains Shield/Forge and the explicit conventional Underground/Deep chain.

15DD.1 staging,2026-10-18: the nine-part/four-industry/six-station source adds raw historical inventories, two recipe owners and paid service beyond Crate's simpler world. Execute15DDB original writer/baselines then15DDC ordinary loaded continuation; parent15DD.1 remains unchecked until both pass. Original15DD.2 native raster follows those complete original files.

### Local Mandatory Reading - Original useful Cistern file writer

_Applies to15DDB; supplements Global and Art/persistence/acceptance Area._

Read completed15DCC's exact useful/preparation receipts, actual native scenes/participants and EarnCisternUsefulRoute/RunCisternUsefulOpportunity. Reuse only15CC's canonical observation/pure save/untouched next-native-opportunity writer patterns, not its fixed quantities, participant count or town-demand assumptions. Observe all nine original parts, all four source/PCHM industries and native recipe history, all six stations, finite reserve/unused Oil, paid root/binding/invoice, current and valid historical company accounts, original cargo/engine definitions and both clock/RNG owners. Every scene and next opportunity must come from the same actual native driver; save/observation creates no work or state repair. Write all five actual modal loaded states (including genuine partial loading) plus final useful state and each untouched next opportunity. Reader/render helpers remain outside this writer task.

### Group 15DDB - Write actual useful Cistern worlds and native baselines

{{HARD}}
- [X] 15DDB.1 Add lossless named original-owner observations and pure native saving to the actual ordinary-loaded useful route, retaining six real modal/final checkpoints and untouched complete next opportunities in both builds. Verify current actual route/source boundaries, complete cross-build world pairs and save purity without reconstructing any participant or advancing a hidden extra tick.

15DDB closure,2026-10-18: both ordinary-loaded original ready worlds earn the same five real loaded modal scenes and final useful state, retaining each untouched next complete native opportunity. Pure saving preserves all named source/recipe/vehicle/packet/account/definition/clock/power observations and both RNGs; interactive RNG is correctly checked as client-local purity, not a native serialized field. Both current test targets build; four isolated native cases pass893206 assertions (446603 per configuration). All twelve original/baseline cross-build pairs match every126 native chunks; both writer final states also match current route-only outputs and the sealed useful-route originals completely. [Writer receipt] (historical reference: `../../../../logs/tubes/cistern-files/20261018-native-01/producer-verification.json`) retains exact inputs,634..654 named observations per state and build/source identities. No original participant reconstruction, hidden tick, native434/artwork change or bootstrap claim. Continue15DDC ordinary files;15DD.1/15D.1/master10.7 remain unchecked.

### Local Mandatory Reading - Ordinary original useful Cistern file continuation

_Applies to15DDC; supplements Global and Art/persistence/acceptance Area._

Read completed15DDB's exact original writer points/snapshots and shared full-native driver, plus15CC's ordinary full-app loading/preview/repeat/opposite-reader/complete-next patterns only. Reacquire exact original nine-part/four-industry/six-station participants and source/receiver/power identities; no builder or cargo registry runs. Check complete native maps/catalogues, clocks/RNG, raw source/recipe/packet/payment/utility owners before and after pure preview and repeated resave. Compare each opposite reader's one complete native opportunity with its actual untouched writer baseline, including precise native GLOG load-event timing rather than wholesale exclusions. Retain every original file/identity unchanged; runtime raster remains15DD.2 after file acceptance.

### Group 15DDC - Accept ordinary useful Cistern loading and full-native continuation

{{HARD}}
- [X] 15DDC.1 Verify every original modal/final state through ordinary native loading, pure preview, complete repeated resave and exact opposite-writer next opportunity in both builds; retain complete current owner/source/four-target evidence and close15DD.1 only when all original files and continuations pass.

15DDC/original15DD.1 closure,2026-10-18: all six genuine loaded/final original worlds pass ordinary opposite-reader loading, pure copied preview, repeated resave and one complete shared native opportunity in both builds. Every first load, repeat and uninterrupted/resumed comparison preserves all126 complete chunks includingGLOG exactly;24 cross-reader output pairs and24 fresh-current/original writer pairs are exact. All four targets build;52 isolated native cases pass913126 assertions (26/456563 per configuration), including current writer and unchanged actual useful route boundaries. [File receipt] (historical reference: `../../../../logs/tubes/cistern-files/20261018-native-01/verification.json`) pins original inputs, current sources/binaries/reports and corrected reader-run-manifest-2.json. The first shell index used unavailable Windows PowerShell IO.Path.GetRelativePath; its native cases all passed, and the index was rebuilt from exact successful reports through pathlib without repeating tests or altering evidence. No native434/artwork change, owner repair or paid TBM/bootstrap claim. Continue15DD.2 actual moving Cistern pixels/picking;15D.1/master10.7 remain unchecked.

### Local Mandatory Reading - Original moving Cistern runtime acceptance

_Applies to15DDE/original15DD.2; supplements Global and Art/persistence/acceptance Area._

Read completed15DDC's exact ordinary-file receipt and original moving scene2, borrowed native participants/snapshot and shared RunCisternUsefulOpportunity. Reuse only15CC.2's actual movement-to-clear-track, native viewport/palette/body/rim/picking and whole-world purity patterns, adapting model11, its actual20CHEM, physical participants and liquid definition. Inspect the original Cistern packed body and real foreground masks; no art rewrite, tolerance change, pose/packet/order relocation, private tick or stock substitute. Actual movement to the inspection point advances all original fronts, clocks, paid power and both real processor owners. Preserve every original upstream source, packet provenance, native accounts and undispatched PCHM output. Raster/picking/repeats must be pure against all126 native chunks and client RNG. Keep human model-identity/gallery and paid TBM/bootstrap acceptance separate.

### Group 15DDE - Accept the actual loaded moving Cistern silhouette

{{HARD}}
- [X] 15DDE.1 Inspect actual moving loaded model11 through ordinary opposite native readers at normal and doubled zoom, verifying original packed pixels/rims/palette/picking and complete-world purity in both builds; close15DD.2/15D.1 only after this and completed15DDB/15DDC pass.

15DDE/original15DD.2/15D.1 closure,2026-10-18: exact opposite ordinary readers load the accepted moving20CHEM Cistern and earn107 complete shared native opportunities6793->6900/49113J before actual clear-track inspection. Clearance resolves every genuine tube platform cell in both joined modal stations, rather than their conventional anchor or a single-cell assumption. Both native zooms account for all273 non-company hull points (268 visible/five exact original rim;1072/20 doubled), with every188/752 interior pixel visible. Actual native picking/wrong-layer refusal, all four complete repeated640x360 RGB frames, all126 pure world chunks and three cross-build world pairs pass. Four targets build;12 isolated native cases/471542 assertions pass, including current body/guideway/useful-route/scene2 continuation boundaries. [Raster receipt] (historical reference: `../../../../logs/tubes/cistern-files/20261018-native-01/raster-verification.json`) pins current evidence and immutable original artwork. First61-op anchor preflight and the guarded two-cell-platform diagnostic remain excluded; no art/tolerance/quantity/pose/order change. Completed source, paid useful route, ordinary files and actual runtime silhouette close15D.1. Continue1985 Shield then2005 Forge and the explicit conventional Underground/Deep chain; original15.2/master10.7, actual TBM/bootstrap, human/gallery/operator gates remain unchecked.

### Local Mandatory Reading - Actual1985 sealed-mineral source and conventional Deep transfer

_Applies to15E/15EB; supplements Global and Art/persistence/acceptance Area._

Read actual model12/canonical1985 launch and conventional model4/6 capabilities in multilayer_vehicle_catalogue.cpp/transport_tube_engine.cpp, the RARE mine/ELEC recipe rows in multilayer_recipe.cpp, native mine/processor public handoffs and only corresponding producer tests. URAN's logical Nuclear plant receiver is not a delivered native receiver: layer-baseline-receiver explicitly expects IT_INVALID. Use actual finite Deep RARE and native Surface Electronics works instead, preserving its real accepted inventory; do not invent a Nuclear facility or claim Electronics output without genuinely transported RFMT. Reuse only native original-definition/calendar/price/funding/finite production/ordinary-file preparation patterns from completed15DB, not its liquid cargos, participants or1980 date. New1985 scenario date/funding precedes all work; never retime or top up a loaded accepted world. Prepare disclosed floor/machinery/Connected conventional shafts only; native command purchases/service/physical work and one owner per reserve/stock remain mandatory. Run focused mine/processor/conventional sealed-carrier/date boundaries before dependent work. Ordinary Capsule stays Underground; conventional freight must physically connect the Deep source to Underground and then Surface. Actual TBM excavation/bootstrap remains Group16.

### Group 15E - Deliver the1985 Shield useful route and conventional Deep chain

{{HARD}}
- [X] 15E.1 Complete15EB actual1985 finite sealed-mineral source,15EC autonomous Shield/conventional useful delivery including real Deep/Underground and Underground/Surface transit, then15ED ordinary files/runtime silhouette. Retain genuine RARE and actual Electronics accepted inventory without claiming unearned manufacture. Original15.2 stays pending until Forge and all required chains pass.

### Group 15EB - Prepare and accept an actual1985 RARE source

{{HARD}}
- [X] 15EB.1 Verify native dated sealed carriers/source/receiver owners, fund actual finite Deep RARE machinery through native commands and earn a bounded20-unit original stock through genuine production/reserve debit; preserve an ordinary native original world before route movement.
- [X] 15EB.2 Verify that exact original source through ordinary repeated/opposite native readers and untouched next opportunities in both builds, retaining original definitions/clocks/accounts/reserve and all actual participants before route construction.

15EB.1 closure,2026-10-18: normal native1985 loading supplies actual prices/definitions and exact Shield availability before commands fund the real Deep RARE mine, Surface ELEC receiver, Deep depot and conventional4+6 sealed carrier. One native industry dispatcher per full opportunity earns20 undispatched RARE in740 opportunities, reserve32000->31980, extracted20/dispatched0, retained cash5286541/loan0/income0. Actual carrier remains whole stopped/empty; receiver has no inputs/output. Both current test targets build;12 isolated native cases/11496 assertions pass (6/5748 each), including current finite/processor/receiver/calendar producer gates. Both empty-author and actual-source pairs match all126 native chunks ([stock receipt] (historical reference: `../../../../logs/tubes/shield-source/20261018-native-01/stock-verification.json`)). Native434/artwork unchanged; floor/completed machinery and initial16m author funds remain disclosed boundaries. Continue staged15EBB before original15EB.2/15EC; no useful delivery, Electronics manufacture or TBM/bootstrap claim.

### Local Mandatory Reading - Original1985 Shield source files

_Applies to15EBB/original15EB.2; supplements Global and Art/persistence/acceptance Area._

Read completed15EB.1's exact original stock receipt/roles and native RunUsefulTubeNativeOpportunity plus the source's single OnTick_Industry opportunity. Borrow the original two parts/two industries/deposit/Deep depot; no source builder, cargo registry, date/funding reset or regenerated geology may run in readers. Observe actual reserve/output/work and ELEC input/recipe history, native cargo/engine definitions, both clocks, company accounts and saved RNG; separately protect unsaved client RNG through preview/save. Compare all126 chunks on original/repeat/opposite source reads and each genuine untouched next complete source opportunity. Consume closed Cistern ordinary-file patterns only as needed; its nine-part/liquid/account expectations and render helpers do not apply. Explicitly retain missing RFMT/no Electronics output and source undispatched20RARE. After source acceptance, actual route may continue finite source work through the native dispatcher; new extraction must remain in the original reserve/physical/accounting equation rather than being hidden or frozen locally.

### Group 15EBB - Accept the exact original Shield source through ordinary files

{{HARD}}
- [X] 15EBB.1 Verify ordinary preview/repeated/opposite native source loads and genuine untouched next opportunities, retaining complete original ownership and both source/definition/clock/account authorities in both builds; close15EB.2 only after exact file and current target evidence passes.

15EBB/original15EB.2 closure,2026-10-18: actual original two-part/two-industry source worlds pass normal opposite-reader loading, pure copied preview, repeated resave and the untouched next full source opportunity. Shared native clocks/vehicles plus exactly one industry dispatcher advance740->741 and original mine work0->1/revision741->742; stock20/reserve31980/accounts and every remaining canonical observation stay unchanged. All original/repeat/next comparisons and six independent cross-build pairs preserve every126 complete native chunks includingGLOG. Four targets build;22 isolated native cases/13042 assertions pass (11/6521 each), including refreshed actual source and finite/processor/receiver/calendar gates ([source-file receipt] (historical reference: `../../../../logs/tubes/shield-source/20261018-native-01/verification.json`)). No owner reconstruction, metadata/date/account repair, quantity normalization, native434/artwork change or useful-route claim. Both15EB tasks areX; continue15EC real paid modal preparation/movement, preserving ongoing native extraction rather than freezing the live source.

### Local Mandatory Reading - Actual useful Shield modal movement

_Applies to15EC; supplements Global and Art/persistence/acceptance Area._

Read completed15EB exact source/participant/file receipt, actual native sealed wagon/model12 admission and ordinary order/loading/packet/payment owners. Keep the source's original reserve/mine history and all unused stock. Fund the real Surface ELEC receiver, conventional Deep-to-Underground feed, local Underground Shield and conventional Underground-to-Surface delivery through actual commands. Use only full native shared clock/vehicle/power work and original producer dispatch; no pose/order/packet/rating grant. Final useful receipt means native RARE acceptance/payment and whole empty returns; absent transported RFMT must leave Electronics manufacturing explicitly unclaimed. Ordinary files and actual packed silhouette remain15ED after useful native movement.

### Group 15EC - Drive actual paid Shield and conventional inter-layer delivery

{{HARD}}
- [X] 15EC.1 Prepare the actual paid modal fleet/topology/native ELEC receiver in the exact accepted1985 source world, then earn original RARE pickup, physical conventional Deep/Underground transfer, layer-local Shield work and conventional Surface useful intake/payment with whole empty returns and conserved native history.

15EC.1 staging,2026-10-18: source15EB already funds/retains the real Surface ELEC receiver and original Deep carrier. Execute15ECB command-paid still-stopped modal geometry/fleet/service before15ECC actual useful transport. Native mine work continues through the one shared dispatcher;20 is the original checkpoint stock, not a private cap or permission to freeze subsequent production. Track original reserve plus every real physical RARE owner and final useful intake throughout. Parent15EC.1 stays unchecked until both stages pass.

### Local Mandatory Reading - Original Shield paid modal preparation

_Applies to15ECB; supplements Global and Art/persistence/acceptance Area._

Read completed15EBB exact original source/roles/canonical source observations and current native conventional4/6, model12, paid source/bind and real order/command boundaries. Reuse only Cistern's command-paid topology/depot/order/source preparation patterns, not its liquid cargo, nine parts, raw keeper or extra receiver construction. Retain actual source carrier, finite mine/claim and already funded ELEC receiver; prepare disclosed conventional Connected Deep/Underground and Underground/Surface shafts through their public owner, preserving original floor/reserves. Conventional feed crosses Deep/Underground, Shield stays Underground, final conventional freight crosses Underground/Surface. Commands alone buy remaining vehicles/guideways/stations/depots and actual paid external service. Every participant stays whole stopped/empty with original source20/dispatched0 and no energy use. No source regeneration, receiver refund/recreation, pose/order/date/cash/stock repair or useful delivery claim.

### Group 15ECB - Prepare the paid original-world Shield modal route

{{HARD}}
- [X] 15ECB.1 In the exact ordinary-loaded accepted source world, retain original source/receiver/carrier and command-fund actual conventional Deep/Underground feed, local Shield, conventional Surface delivery, all real orders/facilities and paid power; preserve a still-stopped native ready-world pair with source/history/account/quantity conservation in both builds.

15ECB closure,2026-10-18: both exact opposite source boundary0 worlds retain original mine0/ELEC1/carrier0+1 and command-fund actual Deep source/UG joined modal stations, local model12, conventional delivery4+6, paid track/depots and synchronized external source/binding. All five parts remain whole stopped/empty, original source20/reserve31980/extracted20/dispatched0/work0 and clocks740 remain exact; no energy is used. Native cash4833946/loan0 records actual452595 preparation cost. Two freight shaft pairs plus the separate external-service housing are disclosed Connected prepared-floor boundaries, preserving every original claim; portal keys1412/1827/6479 are observed native identities. Both test targets build; six isolated native cases/1828 assertions pass, including unchanged opposite source0->1 continuation. Ready worlds match all126 native chunks ([preparation receipt] (historical reference: `../../../../logs/tubes/shield-route/20261018-native-01/prepare-verification.json`)). Native434/artwork unchanged. Continue15ECC actual freight/continuing finite extraction; original15EC.1, useful intake/returns, runtime silhouette and TBM/bootstrap remain pending.

### Local Mandatory Reading - Actual useful Shield movement and continuing finite extraction

_Applies to15ECC; supplements Global and Art/persistence/acceptance Area._

Read completed15ECB exact original ready worlds and full shared RunShieldSourceOpportunity, actual station/packet/receipt and paid power owners. Run every original front/clock and both real industries once per common opportunity; native mine extraction continues under its real reserve/output cap. Dispatch only real output through normal earned station service. Conserve original initial reserve across remaining deposit, all industry/queue/hold RARE and genuine final ELEC intake/recorded recipe consumption. Native quarters own money/deliveries; no private scalar income or source freeze. Actual minimum useful delivery must be earned through physical Deep/Underground feed, local loaded Shield and conventional Surface intake, followed by whole empty home returns with all remaining native cargo accounted. Preserve absent RFMT/no Electronics manufacturing claim. No pose/order/packet/rating/clock/cash repair.15ED ordinary full-world files/raster follows actual useful acceptance.

### Group 15ECC - Earn real Shield freight and complete conventional layer transitions

Context has compacted 1 times while executing this group.

{{HARD}}
- [X] 15ECC.1 Drive the original paid modal route with continuing finite native extraction, earn genuine Surface ELEC RARE intake/payment and complete whole empty home returns while accounting for every reserve/stock/packet/history owner; close15EC.1 only after both15ECB/15ECC pass in both builds.

15ECC/original15EC.1 closure,2026-10-18: exact opposite ordinary ready worlds earn all five real loaded scenes and three whole empty home returns through full shared native clocks/vehicles/industry work. Both independent runs deliver24 original RARE to actual Surface ELEC intake, earn798 native gross freight income and consume1916000J from the original paid producer. Continuing extraction reaches203, exactly24 received +179 genuinely queued; reserve31797/source buffer0/dispatched203 and five original native packets retain provenance. No RFMT is supplied or Electronics manufactured. All three fronts stop only after useful intake and actual empty home storage;148 additional complete native opportunities preserve intake/income while source work continues. Final tick7527/year1985/cash4829512/loan0 and native current/old quarter history match. Both test targets build; two focused native cases/428806 assertions and every126 complete final chunks pass ([useful receipt] (historical reference: `../../../../logs/tubes/shield-route/20261018-native-01/verification.json`)). Native434/artwork unchanged. Prepared floor/machinery/Connected shafts remain boundary inputs;15ED ordinary files/silhouette and actual TBM/bootstrap/human/gallery remain pending.

### Local Mandatory Reading - Original useful Shield files and runtime silhouette

_Applies to15ED; supplements Global and Art/persistence/acceptance Area._

Read completed15EC exact useful worlds and actual original owner/source/clock/account observations. Adapt only completed Cistern ordinary writer/read/preview/repeat/opposite-next/raster/picking patterns to actual Shield model12, sealed RARE, native source/receiver and both conventional layer transitions. Preserve every real participant and all original reserve/packet/payment/power history. Every original/baseline must use the same full native driver. Inspect actual moving loaded model12 pixels with original packed art and exact real rims; no pose/stock/art/tolerance substitution. Keep prepared floor/machinery/shafts and actual TBM/bootstrap/human/gallery gates explicit.

### Group 15ED - Accept ordinary useful Shield files and native silhouette

{{HARD}}
- [X] 15ED.1 Retain every actual loaded conventional layer/modal and final useful original state plus untouched complete next native opportunities; verify ordinary pure preview/repeated/opposite-reader continuation and complete owner transport in both builds.
- [X] 15ED.2 Inspect actual loaded moving Shield native pixels/picking and all126-chunk render purity at both zooms, then close15E.1 only after all source/route/file/render evidence passes.

15ED staging,2026-10-18: retain original15ED.1/.2 as aggregate obligations. Execute bounded15EDB original boundary writer,15EDC ordinary reader acceptance, then15EDE actual runtime silhouette. Continuing finite source dispatch and five legitimate final queued packets require full packet/queue/source/account observations; do not copy Cistern's final packet-free assumption or freeze the mine for file equality.

### Local Mandatory Reading - Original useful Shield boundary production

_Applies to15EDB; supplements Global and Art/persistence/acceptance Area._

Read completed15ECC exact useful/ready receipts, borrowed LoadShieldUsefulWorld and single RunShieldUsefulOpportunity/CheckShieldUsefulConservation. Adapt only Cistern's real original boundary writer and complete source/modal observations. Retain five actual loaded modal scenes plus final whole-empty world and untouched next complete opportunities, each through the same original full native route. Capture every actual packet/queue/hold, source reserve/work/claim, native receipt/current/history account, paid power and original owner digest. Save/preview observation cannot consume clocks/RNG or repair source/packet/pose/order/account state. All original/native next worlds retain126 complete chunks; later readers consume exact accepted writers opaquely.

### Group 15EDB - Retain actual original Shield modal file boundaries

{{HARD}}
- [X] 15EDB.1 Produce both-build original loaded modal/final worlds and untouched full native next-opportunity baselines, with complete canonical owner observations, pure native saves and independent final useful-route equality.

15EDB closure,2026-10-18: both exact opposite paid worlds retain actual loaded sealed feed Deep/UG, moving24RARE Shield, sealed delivery UG/Surface and final useful7527, each with precisely its untouched next complete source/dispatch opportunity. Complete canonical observations include all five parts, native source/recipe/industry histories, every current/old delivery bucket, four station queues, every genuine split packet/feeder share and original power/binding/invoice. Both RNGs remain pure through saving; every12 cross-build original/baseline pair preserves all126 chunks and two current route-only final pairs match accepted useful history exactly. Four focused cases/858574 assertions and both test builds pass ([producer receipt] (historical reference: `../../../../logs/tubes/shield-files/20261018-native-01/producer-verification.json`)). The first Python expectation incorrectly used Space2 for native Deep3; retained native logs prove correct physical layer and the expectation is corrected. Native434/artwork unchanged; ordinary15EDC readers/parent15ED.1 and15EDE raster remain pending.

### Local Mandatory Reading - Ordinary useful Shield file acceptance

_Applies to15EDC; supplements Global and Art/persistence/acceptance Area._

Read completed15EDB exact writers/complete canonical observations and same borrowed full-native continuation driver. Adapt only Cistern's ordinary preview/inspect/repeat/opposite-reader and uninterrupted-next comparison. Every live packet, remaining reserve, source work, payment/grant, original vehicle/order/station/industry/account/clock owner must survive; all126 chunks including native GLOG must be compared, with a narrowly independently checked actual load-event timing difference only if reproduced. No constructors, normalized owners or omitted extraction/dispatch. Native source continues once per common opportunity. Original15ED.1 closes only after writer and reader evidence passes both configurations.

### Group 15EDC - Accept ordinary Shield modal files and complete continuation

{{HARD}}
- [X] 15EDC.1 Prove all six actual boundaries through pure preview, repeated/opposite current readers and one complete original-native continuation, preserving full owner/126-chunk worlds and unchanged original inputs; close15ED.1 only after15EDB/15EDC pass.

15EDC/original15ED.1 closure,2026-10-18: all six accepted original loaded/final states pass ordinary first load, pure copied preview, repeated save and untouched full-native source/dispatch continuation in both configurations. Complete426..521-field observations and every126 native chunks includingGLOG remain exact;24 opposite-reader world pairs and24 fresh/original writer pairs pass. Current writers/routes preserve accepted actual useful history,179 remaining RARE/five original queued packets and every original source/account/power owner without normalization. All four targets build;52 focused native cases/874984 assertions pass ([file receipt] (historical reference: `../../../../logs/tubes/shield-files/20261018-native-01/verification.json`)). One last optimized pair initially failed during runtime copying because D: was full, before native startup; the same pair passes after reversible NTFS compression of four build PDBs and new native-runtime directories. One verified duplicate GRF in that failed unreceipted runtime was individually removed under the user's ten-file allowance; distribution/evidence remain intact. Native434/artwork unchanged. Continue15EDE actual moving model12 pixels/picking; original15ED.2/15E.1 and TBM/bootstrap/human/gallery remain pending.

### Local Mandatory Reading - Actual moving Shield runtime silhouette

_Applies to15EDE; supplements Global and Art/persistence/acceptance Area._

Read completed15EDC exact original loaded model12 scene and native Cistern raster/picking helpers only as needed. Follow actual moving Shield through the same complete source/dispatch driver to a measured clear midpoint relative to every actual two-cell tube platform, not conventional Station.xy. Compare original loaded native hull/interior pixels, exact rims, physical picking, both zooms, repeated/opposite RGB and all126 chunks through pure rendering. No pose/cargo/artwork/tolerance substitutions. Retain genuine continuing extraction and original stock/packet/power history. Close original15ED.2/15E.1 only after source/useful/files/raster all pass; actual TBM/bootstrap/human/gallery stay pending.

### Group 15EDE - Verify actual moving Shield native silhouette

{{HARD}}
- [X] 15EDE.1 Verify real loaded moving model12 pixels/picking at both native zooms and complete render purity/repeats, then close original15ED.2/15E.1 with exact retained producer/file evidence.

15EDE/original15ED.2/15E.1 closure,2026-10-18: exact opposite ordinary loaded24RARE model12 worlds earn113 full source/dispatch opportunities (4097->4210/61020J), then render a measured clear actual midpoint with all original owners retained. Real full-capacity artwork/picking, both zooms, every126 render-purity/cross-build chunks and four whole640x360 RGB pairs pass. All259 non-company hull points and176 interior points remain exact/visible (1036/704 doubled), with zero unexplained/occluded pixels. First raster's native render passed but its shared Python matcher assumed partial-load artwork; the matcher now consumes the observed native state2/full pose while retaining older default partial1. Both retained original Cistern partial-load frame pairs reproduce unchanged original counts ([compatibility artifact] (historical reference: `../../../../logs/tubes/shield-files/20261018-native-01/partial-pixel-compatibility.json`)); no artwork, pixel mask, tolerance or visibility bound changed. Four targets build;12 focused raster/body/geometry/useful-route/loaded-next native cases/454004 assertions and three complete126-chunk cross-build pairs pass ([raster receipt] (historical reference: `../../../../logs/tubes/shield-files/20261018-native-01/raster-verification.json`)). Native434/packed artwork unchanged. Actual finite source/useful modal movement/files/raster now close Shield, including real conventional Deep/Underground and Underground/Surface crossings. Original15.2 still requires actual2005 Forge; actual paid TBM/bootstrap, runtime gallery and human/operator gates remain pending. Stage the remaining Forge producer into bounded source/route/files/raster groups before Group16; this closure does not skip it.

### Local Mandatory Reading - Actual2005 Forge source and native prerequisites

_Applies to15F/15FBA; supplements Global and Art/persistence/acceptance Area._

Read original15.2, parentAD15 canonical launch calendar, actual model13/4/5 cargo definitions in multilayer_vehicle_catalogue.cpp, multilayer_recipe.cpp CORE/COAL mines/CFMT/ELEC and native installed processor/mine/cargo/engine owners. Forge's RFMT cargo is genuinely produced by CFMT:4CORE+1COAL->2RFMT, with actual ordinary Surface ELEC acceptance of RFMT. Native intake/payment is useful freight, but absent RARE cannot manufacture Electronics. Reuse only Shield's normal empty-world author/full-content startup and focused finite/processor/calendar gates; preserve its accepted1985 worlds if extracting a shared empty-author helper. New2005 author date and explicit initial funds precede every purchase/work step. No reduced native registry, copied cargo properties, date/availability grant, output/packet/account repair or invented receiver. Public signatures alone do not pass required focused native gates. Actual TBM/bootstrap stays Group16.

### Group 15F - Deliver the2005 Forge useful route

{{HARD}}
- [X] 15F.1 Complete15FBA/15FBB/15FBC original native finite CORE/COAL source,15FCB/15FCC/15FCD real input carriage/CFMT RFMT output,15FDB/15FDC actual paid Forge/conventional useful delivery and15FEB/15FEC/15FEE ordinary files/runtime silhouette, then close original15.2 only with all five completed model routes and explicit conventional Deep-chain evidence.

### Group 15FBA - Establish actual2005 Forge native prerequisites

{{HARD}}
- [X] 15FBA.1 Verify focused finite/processor/conventional freight/calendar owners and retain an ordinary2005 empty native author/definition gate in both builds, including actual model13 launch/refits, model4/5 raw/RFMT carriage and native CFMT/ELEC definitions, before constructing source quantities.

15FBA closure,2026-10-18: normal2005 native loading confirms actual model13 availability on1January/capacity80/heavy refits, conventional4/5 raw/RFMT compatibility and native Surface CFMT24/ELEC27 definitions. Original generated CORE deposit2 atUG49,9 and Coal deposit0 atUG8,9 each retain32000 reserve; no machinery, vehicle, station, work, stock or packet exists. Shared WriteTubeUsefulSourceTemplate extracts only the unchanged original Shield empty author, with explicit initial2005/32m funds before work. Both native test targets build;18 focused cases/11930 assertions pass, including five independently isolated finite/processor/receiver/calendar/conventional gates per build and original Shield producer checks. Two independent2005 empty/normal-loaded pairs and four current/original1985 Shield empty/stock pairs retain every126 chunks ([prerequisite receipt] (historical reference: `../../../../logs/tubes/forge-source/20261018-native-01/prerequisite-verification.json`)). Native434/artwork unchanged. Continue15FBB actual paid raw machinery/stocks; original15F.1/15.2 and RFMT manufacture/useful freight/files/raster/TBM/bootstrap remain pending.

### Local Mandatory Reading - Actual finite Forge raw stocks

_Applies to15FBB; supplements Global and Art/persistence/acceptance Area._

Read completed15FBA exact empty native world/definition/producer receipt, actual COAL/CORE claim geometry, paid BuildLayerMine/BuildIndustry/4+5 carrier commands and single full native clock/front/industry driver. Fund two real Underground finite mines, Surface CFMT and two stopped empty actual raw carriers. Earn bounded native raw stocks without station/rating/packet grants; conserve each original generated reserve/work/claim and retain genuine empty CFMT inputs/output. Completed machinery/prepared floor are disclosed initial boundaries, not TBM excavation. No useful intake/manufacture yet; stock targets are checkpoints and cannot freeze subsequent production.

### Group 15FBB - Earn actual2005 finite CORE and Coal stocks

{{HARD}}
- [X] 15FBB.1 Fund actual original finite mines/Surface refinery/stopped raw carriers and earn native CORE/COAL stocks, retaining exact ordinary original worlds with both independent reserve/physical/history/account equations.

15FBB closure,2026-10-18: exact opposite accepted2005 normal-loaded empty worlds retain original CORE2/Coal0 deposits and command-fund actual mines0/1, Surface CFMT2 and four coupled4+5 raw parts0..3 in real Underground homes45,13/4,13. One shared complete clock/front/industry opportunity earns40CORE +40Coal after740 opportunities, with each reserve32000->31960/extracted40/dispatched0 and every real carrier whole stopped/empty. CFMT has no input/output/batches; no station/packet or useful income exists. Native cash21119986/loan0 records actual funding and native costs from initial32m. Both test targets build;16 focused stock/current-definition/finite/processor/receiver/calendar/conventional cases/78786 assertions pass. Both actual stock worlds and four current/accepted empty prerequisite pairs retain every126 native chunks ([stock receipt] (historical reference: `../../../../logs/tubes/forge-source/20261018-native-01/stock-verification.json`)). A compile-only signed snapshot conversion and a Python source-inventory path were corrected; failed diagnostic attempts are retained, with no active failure. Native434/artwork unchanged. Continue15FBC ordinary original raw-stock files before buying raw routes or claiming RFMT manufacture/useful Forge movement; original15F.1/15.2 and TBM/bootstrap/human/gallery remain pending.

### Local Mandatory Reading - Original Forge raw-source files

_Applies to15FBC; supplements Global and Art/persistence/acceptance Area._

Read completed15FBB original roles/stocks and its one complete native opportunity. Borrow all real source owners; retain source0/untouched1 baselines and native definitions/accounts/reserves/work/claims/RNG. Adapt only Shield source-file preview/repeat/opposite-next patterns, including both finite source histories and actual CFMT owner. No builders or quantity/time repair in readers; complete126 chunks remain mandatory.

### Group 15FBC - Accept original Forge raw-source files

{{HARD}}
- [X] 15FBC.1 Prove both original raw stocks through pure ordinary preview/repeated/opposite-reader and untouched complete native next opportunities before physical refinery feeding.

15FBC closure,2026-10-18: both exact opposite original raw-stock worlds retain all four real parts/three industries/two claims/reserves, native definitions/accounts/clocks and complete canonical owner observations. Pure original0 and untouched next1 advance both mine work counters0->1 through the single full native driver, preserving40CORE/40Coal/reserves31960/emptyCFMT/stopped carriers. First loads, copied preview, repeats and opposite-reader complete continuation preserve every126 native chunks includingGLOG exactly; no diagnostic exclusion, builder, inventory/clock/RNG repair or source freeze participates. All four targets build;22 focused native cases/82042 assertions and six all126 cross-build world pairs pass ([source-file receipt] (historical reference: `../../../../logs/tubes/forge-source/20261018-native-01/verification.json`)). Native434/artwork unchanged. Continue15FCB actual paid conventional input routes before15FCC raw delivery/manufactured RFMT; original15F.1/15.2 and useful Forge movement/files/raster/TBM/bootstrap/human/gallery remain pending.

### Local Mandatory Reading - Actual Forge refinery input carriage

_Applies to15FCB/15FCC; supplements Global and Art/persistence/acceptance Area._

Read completed15FBC exact original raw world and native conventional freight/stations/Connected portal/CFMT work owners. First command-fund still-stopped real routes from both Underground mines to Surface CFMT, preserving every original stock/claim/carrier. Then run all native clocks/fronts/industry work and actual dispatch/earned service once per common opportunity; no rating/packet/arrival grant. Physically deliver both real inputs and earn RFMT through exact4CORE+1COAL->2RFMT batches. Retain unused inputs, continuing finite extraction, actual station/hold packets and source/current/old company accounts. Stop raw haulers only after whole empty home return; no source freeze, inventory normalization or manufactured RFMT assignment.

### Group 15FCB - Prepare paid conventional Forge refinery input routes

{{HARD}}
- [X] 15FCB.1 Retain original sources/refinery/carriers while command-funding both conventional Underground/Surface input routes and preserving exact still-stopped raw stocks with paid native costs.

15FCB closure,2026-10-18: exact opposite accepted raw-source0 worlds retain original mines0/1, Surface CFMT2, four coupled parts0..3 and both finite claims. Native commands pay16320 for actual rail, two Underground source platforms, two distinct same-layer Surface CFMT platforms sharing station2, far depots and normal pickup/delivery/service orders. Disclosed Connected shaft pairs850/891 and prepared floor retain every original claim. Native tick740/40CORE/40Coal/reserves31960each, all mine/processor states and every original physical pose remain unchanged; all carriers stay whole stopped/empty with no queue/packet/income. Native cash21103666/loan0 records real costs. Both test targets build; six focused preparation/original-source-next cases/2656 assertions and every126 ready-world chunks pass ([preparation receipt] (historical reference: `../../../../logs/tubes/forge-raw-route/20261018-native-01/prepare-verification.json`)). Native434/artwork unchanged. Continue15FCC actual native pickup/carriage/CFMT manufacture/whole empty returns; paid preparation does not prove physical delivery, produced RFMT or TBM/bootstrap.

### Group 15FCC - Manufacture genuine native RFMT from physically delivered inputs

{{HARD}}
- [X] 15FCC.1 Earn actual CORE/COAL pickup/carriage/Surface CFMT intake and bounded genuine RFMT production, retaining all source/unused-input/packet/recipe/account owners and whole empty raw returns.

15FCC closure,2026-10-18: both exact opposite prepared original worlds physically carry native CORE/Coal through their Underground/Surface routes into Surface CFMT. Four first loaded layer scenes, native64CORE/128Coal paid intake,16 actual4CORE+1Coal->2RFMT batches and32 genuine RFMT pass. Whole empty home returns occur at8553/9552;148 further full native opportunities finish at9700/year2005 without replayed intake/payment. Both sources continue normally: each extracts/dispatches524, retains31476 reserve, with460CORE/396Coal waiting; actual CFMT retains112 unused Coal/zero CORE and two genuine packets remain. Native current/old quarters retain4384 gross freight income/cash21106824/loan0. Pure saving preserves513 named owner observations and separate client RNG. Both test targets build; two isolated native cases/638068 assertions and every126 complete output chunks pass ([manufacturing receipt] (historical reference: `../../../../logs/tubes/forge-raw-route/20261018-native-01/verification.json`)). Initial diagnostic left the review save paused while manually driving vehicles, so native mines correctly refused work/dispatch; the caller now explicitly resumes normal gameplay and the common driver asserts that precondition. No producer workaround, inventory/rating/pose/order/clock/cash repair, native434/art change or TBM/bootstrap claim. Continue15FCD original manufactured-cargo files;15F.1/15.2/master10.7 and downstream Forge modal route/files/raster remain pending.

### Local Mandatory Reading - Original produced Forge cargo files

_Applies to15FCD; supplements Global and Art/persistence/acceptance Area._

Read completed15FCC original RFMT producer world, full native driver and complete reserve/raw/recipe/current-history observations. Adapt only Cistern actual-output file acceptance, with both finite mines still operating normally and all unused raw inventory preserved. Ordinary preview/repeat/opposite-next must retain all126 chunks and every real raw carrier/industry/station/packet/account/power owner before buying the dependent Forge modal fleet.

### Group 15FCD - Accept the original genuine RFMT producer through ordinary files

{{HARD}}
- [X] 15FCD.1 Verify original manufactured RFMT and untouched complete next native opportunities through pure repeated/opposite native files, retaining both finite sources and raw-input histories.

15FCD closure,2026-10-18: both exact opposite original32RFMT worlds retain all513 native owner observations and every126 complete chunk through original0 saving, copied preview, repeated loading/resaving and opposite-reader complete continuation. Untouched9700->9701 baselines advance both mine work counters6->7/revisions10179->10180 once, with no new reserve debit, raw intake, recipe batch or payment. Original112unusedCoal/31476reserves/460CORE+396Coal queues/two packets/actual16batches/current+old company histories remain exact; no source/route reconstruction, quantity/rating/pose/order/clock/account repair or native434/art change participates. Both test targets build;12 isolated native cases/642000 assertions, six all126 cross-build file pairs and both fresh/original complete manufacturing pairs pass ([produced-file receipt] (historical reference: `../../../../logs/tubes/forge-produced/20261018-native-01/verification.json`)). No diagnostic chunk exclusion. Continue15FDB paid stopped Forge modal preparation, then15FDC useful RFMT/ELEC intake;15F.1/15.2/master10.7, downstream files/raster and actual TBM/bootstrap/human/gallery/operator gates remain pending.

### Local Mandatory Reading - Actual paid useful Forge modal route

_Applies to15FDB/15FDC; supplements Global and Art/persistence/acceptance Area._

Read completed15FCD exact RFMT/roles and existing native model13/4/5, paid source/binding, conventional portal and ELEC acceptance owners. Command-fund still-stopped actual conventional feed to Underground, local Forge and conventional Surface ELEC delivery without rebuilding upstream source owners. Then use the same complete native source/dispatch driver to earn real loaded modal movement, RFMT intake/payment and three whole empty returns. Both finite sources and CFMT continue under native caps; all original raw stock, actual batches, queues/holds/packets, power and current/old quarters remain conserved. Absent RARE means no Electronics-output claim. No pose/order/rating/packet/clock/cash repair; prepared floor/Connected shafts remain boundaries.

### Group 15FDB - Prepare the paid original-world Forge modal route

{{HARD}}
- [X] 15FDB.1 Retain original manufactured RFMT world and command-fund actual still-stopped model13/conventional modal fleet, ELEC receiver, orders, facilities and paid source/binding with original source/account conservation.

15FDB closure,2026-10-18: exact opposite original manufactured source0 worlds pay1334927 in native costs for a NEW4+5 RFMT feed, real model13/capacity80, final4+5 freight, Surface ELEC, genuine joined modal facilities/orders and synchronized external service/binding. The existing Coal conventional corridor/depots remain actual paid owners; native12-row spread admits the joined Underground Coal/Forge berth without altering either original raw keeper. All original mine/refinery/physical-carrier/queue/packet/receipt observations remain exactly equal; native9700/32RFMT/112unusedCoal/31476reserves/460CORE+396Coal queues/two packets/income4384/loan0 are retained, native cash19771897 records the paid costs. All five fronts/nine actual parts remain whole stopped/empty with zero lifetime energy; no useful arrival is claimed. Four Connected pairs include the two preserved originals and two disclosed new preparation boundaries; prepared floor/new paid ELEC completion remain separate from TBM/bootstrap. Both test targets build; six focused preparation/original-produced-next cases/4952 assertions,695 pure ready observations and all126 complete ready chunks pass ([preparation receipt] (historical reference: `../../../../logs/tubes/forge-route/20261018-native-01/prepare-verification.json`)). Current original9700->9701 files still advance both mine work6->7 without cargo/account changes. No native434/art change, source freeze or quantity/rating/pose/order/clock/cash repair. Continue15FDC actual useful Forge movement;15F.1/15.2/master10.7 and later files/raster/TBM/bootstrap/human/gallery/operator gates remain pending.

### Group 15FDC - Earn actual useful Forge freight and whole empty returns

{{HARD}}
- [X] 15FDC.1 Drive actual original RFMT through both modal transfers into useful native Surface ELEC intake/payment, completing whole empty home returns while conserving continuing source/recipe/packet/account/power owners.

15FDC closure,2026-10-18: both exact opposite paid ready worlds carry all32 original native RFMT through actual Surface/Underground feed, moving model13 Forge and conventional Surface ELEC intake. Five genuine loaded scenes,323 additional gross freight income and4392252J metered service pass. Whole empty feed/Forge/final-delivery homes17526/17760/17981 then148 full stopped opportunities finish at18129/year2005 without replayed intake/payment. Both finite mines continue extracting/dispatching979 each, reserves31021 each, actual raw queues915CORE/851Coal and two genuine packets; original raw keepers remain stopped, CFMT16batches/112unusedCoal retained. RARE remains absent, RFMT consumed0/Electronics output0. Current+two old quarters retain total native income4707/cash19763456/loan0; no operating-profit claim. Both test targets build; six isolated useful/original-produced-next cases/1095352 assertions,781 pure final observations and every126 complete useful chunks pass ([useful receipt] (historical reference: `../../../../logs/tubes/forge-route/20261018-native-01/verification.json`)). Default strict original RFMT-file consumers remain exact9700->9701/all126 after explicit actual-CFMT-packet admission in useful mode. No source/route reconstruction, source freeze, cargo/rating/pose/order/clock/account repair or native434/art change. Continue15FEB original useful loaded/final world writing, then15FEC ordinary readers/15FEE actual moving Forge pixels;15F.1/15.2/master10.7 and actual TBM/bootstrap/human/gallery/operator gates remain pending.

### Local Mandatory Reading - Original useful Forge files and actual silhouette

_Applies to15FEB/15FEC/15FEE; supplements Global and Art/persistence/acceptance Area._

Read completed15FDC exact real useful world and one complete native source driver. Adapt Shield's original modal writer, ordinary preview/repeat/opposite-next and actual moving raster/picking patterns only to original Forge model13/RFMT and actual load state/capacity. Retain every raw/processed/unused input, finite reserve, batch, packet/queue/hold, station/vehicle, power/account/clock owner. All126 complete chunks, both RNG purity, actual two-cell platform clearance, exact original hull/interior/rim pixels, both zooms and whole RGB repeats remain required. No art/pose/quantity/tolerance substitutions. Close original15.2 only with all five actual introduction-date route/file/silhouette receipts and completed Shield conventional Deep-chain evidence; Group16/bootstrap/human/gallery stay pending.

Native moving-world discovery,2026-10-18: nine actual train parts do not imply nine total vehicle-pool entries. `SlVehicleEffect`/`VEHSChunkHandler` also save genuine transient visual effects. Preserve every effect ID and serialized pose/sprite/animation field, require exactly nine trains and no unrelated vehicle class, and compare all126 chunks including those effects. Never delete or normalize effects to satisfy a fixed pool-count assumption; stopped final worlds naturally return to nine total entries.

### Group 15FEB - Retain actual original Forge modal file boundaries

{{HARD}}
- [X] 15FEB.1 Retain each actual loaded modal/final original world and untouched complete next native opportunity with pure saves and complete canonical owner observations in both builds.

15FEB closure,2026-10-18: the same actual original Forge route now retains five genuine first loaded states12409/13237/13882/15850/16183 and useful final18129, each with precisely its untouched next complete opportunity. Original final remains exactly the sealed32RFMT useful world, with final18130 only its explicit baseline. Canonical858..1050-field snapshots retain all nine train parts, four industries, five stations, raw/recipe/receiver/packet/account/power owners and every actual saved visual-effect field. Moving scene2 naturally has nine/ten effects across its original/next boundaries; scene4 has seven, while final has zero. No effect deletion/normalization or native434/art change participates. Both test targets build; four isolated writer/current-useful cases/2189418 assertions, twelve complete126-chunk cross-build world pairs and both current/original useful-final pairs pass ([writer receipt] (historical reference: `../../../../logs/tubes/forge-files/20261018-native-01/producer-verification.json`)). Initial native writer already passed, but its Python validator assumed nine total vehicles; nativeVEHS/SlVehicleEffect ownership established the corrected nine-train-plus-real-effects contract. Continue15FEC ordinary preview/repeat/opposite-reader/complete continuation for all six original worlds, then15FEE actual Forge raster/picking.15F.1/15.2/master10.7 and TBM/bootstrap/human/gallery/operator gates remain pending.

### Group 15FEC - Accept ordinary useful Forge files and continuation

{{HARD}}
- [X] 15FEC.1 Prove pure ordinary preview/repeated/opposite current reads and full original-native continuation for every real modal/final boundary, preserving all original owners and126 chunks.

15FEC closure,2026-10-18: every original loaded/final Forge scene passes ordinary opposite-build loading, copied preview, repeated saving and complete native next continuation. Nine actual train parts plus each native saved effect survive intact, including scene2 original9/next10 effects and scene4 seven; no normalization or chunk exclusions. All four targets build;52 isolated native cases/2226446 assertions pass, with24 complete126-chunk cross-reader pairs and24 fresh-current/original writer pairs exact ([file receipt] (historical reference: `../../../../logs/tubes/forge-files/20261018-native-01/verification.json`)). Full858..1050-field observations retain both finite sources,32 genuine RFMT/actual transfers/receiver,112unusedCoal, all recipe/native quarter/packet/power owners and separate client-RNG purity. Current route/writer finals still match sealed18129 useful world; final18130 remains precisely its full native baseline. All reader cases passed first attempt, including the targeted moving-effect preflight. Native434/artwork unchanged; no source/route reconstruction, producer freeze or quantity/rating/pose/order/clock/account repair. Continue15FEE actual moving model13 pixels/picking, then audit all five model obligations before15F.1/original15.2; master10.7, actual TBM/bootstrap and human/gallery/operator gates remain pending.

Context has compacted 1 times while executing this group.

### Group 15FEE - Verify the real moving Forge silhouette and close all five useful models

{{HARD}}
- [X] 15FEE.1 Verify actual loaded moving model13 native pixels/picking at both zooms with pure complete worlds/repeated RGB, then close15F.1/original15.2 only after all five actual model/date/source/useful/file/render obligations pass.

15FEE closure,2026-10-18: both exact opposite original scene2 Forge worlds earn130 complete native opportunities13882->14012/112320J to a real clear tile5644, speed70, retaining32RFMT of80capacity and every genuine saved effect. Actual native partial-state1 sprites/picking pass both zooms: all281 original non-company hull points are visible or covered by exactly two original rim points; all152 interior points remain visible. Both repeated640x360 RGB frames and opposite builds match completely; all126 native chunks, canonical owners and separate client RNG stay pure through drawing/picking. All four targets build;12 focused native cases/1130812 assertions pass ([Forge raster receipt] (historical reference: `../../../../logs/tubes/forge-files/20261018-native-01/raster-verification.json`)). Native434/packed art unchanged; no source, pose, load, order or tolerance repair.

The final [five-model audit] (historical reference: `../../../../logs/tubes/forge-files/20261018-native-01/five-model-verification.json`) verifies31 sealed receipts/1446 original report/log/world/art/capture identities. Actual1970 Mole20Coal/55income,1975 Crate39Goods/844income,1980 Cistern20CHEM/427income,1985 Shield24RARE/798income and2005 Forge32RFMT/323income retain their source/file lineages, whole empty returns and both native runtime captures. Every original runtime hull/interior/rim match and full RGB repetition is rechecked. All five normalized observed contours differ; a common partial/SW/frame0 comparison independently rules out recolor/translation-only variants. Shield's same conventional parts carry genuine Deep RARE to Underground and return empty to Deep, with local Underground Shield transfer and separate conventional Surface ELEC delivery; no capsule crosses a layer. Six fresh isolated launch-startup/live-publication/saved-definition cases add2120 assertions across both builds. Original Mole/Crate first-load authority/GLOG exceptions and canonical-price correction remain exactly their accepted file contracts, not a blanket all126 first-load claim. Prepared floor/machinery/Connected shafts, missing-coinput output and operating-profit limits remain disclosed. This closes15F.1/original15.2 only. Return to the remaining master10.7 operating-plant1.3B/runtime-art scopes before its9.5B lifecycle return and10.8 actual final-use integration. Group16 paid TBM corridor/cavern conversion/bootstrap and human/operator/network gates stay pending.

### Group 16 - Demonstrate cleared ground becoming a useful tube route

{{HARD}}
- [X] 16.1 Use a real TBM to clear a curved corridor and large terminal cavern, withdraw cutter/hauler and remove only safe temporary support before constructing tubes.
- [X] 16.2 Load a useful cargo, run a capsule to a layer-local interchange and deliver via conventional freight to Surface. Verify actual wagon spoil hauling from the excavation remains a separate completed leg.
- [X] 16.3 Save/reload while capsules queue and reservations change; compare packet/position/grant ledgers and demonstrate recoverable loss/restoration of paid-grid supply.

### Local Mandatory Reading - Paid non-rectangular native clearing and tube replacement

_Applies to16B; supplements Global/Art,persistence and acceptance Area._

Read Plan4-15B's actual paid construction receipt and current21.4,RunUndergroundExcavationWorkflow's observe/control distinction and real disposal/empty-return completion,PreviewTBMExtension/CmdExtendTBMProject and IsTBMSupportInUse. The initial generated world/company/commissioned spoil logistics are disclosed inputs;native paid cutting must earn every final tube cell. Extend the existing real unfinished job through reviewed commands to form a non-rectangular chamber/corridor union,retain45-degree advancing turns and full physical spoil conservation,and withdraw the original cutter/hauler before ordinary safe support removal. New fixtures must not paint floor,force train phases/positions or truncate stock. Use src/tests/tube_tbm_clearance_fixture.h with the focused tbm_reuse.cpp case;logs use logs/tubes/tbm-final-use/,native files/reports use mlm/reviews/tubes/tbm-final-use/. One project/four workers;isolated110-second native children.

### Group 16B - Earn irregular cleared floor and native tube construction

{{HARD}}
- [X] 16B.1 Drive actual paid wide cutting plus reviewed native extensions into a non-rectangular terminal/corridor footprint;settle all actual Earth/Rock and withdraw the whole original trains before safe support removal.
- [X] 16B.2 Construct a real curved tube route,terminal caverns and native service depots entirely on that earned floor;verify both readers and retain the exact same-world handoff before closing16.1/Plan4-21.4.

16B/original16.1 closure,2026-10-19:the real original cutter earns an L-shaped requested floor through one reviewed native extension,63paid cuts/50legal advancing bends,375Earth/750Rock physically delivered to Surface and3750work. Whole actual cutter/hauler withdrawal precedes safe removal and native curved tube/two-terminal/two-depot construction,2850total replacement invoices. An unrequested corner remains solid;ordinary construction preserves all geological/finished work owners. Both current four-worker readers pass74270assertions/two isolated cases,writing byte-identical complete126-chunk original434worlds ([receipt] (historical reference: `../../../../logs/tubes/tbm-final-use/20261019-native-01/clearance-verification.json`),SHA256 c27f4c8e95f314c04de4b6250bc169cced7fbe673f6d0114917cf0ba051f6929). Exact originals are selected by that receipt's records,not discovered as latest. Reproduce tube_tbm_return.py --config Debug/RelWithDebInfo --focus clearing,then tube_tbm_clearance_report.py with the explicit clearance-inputs.json and a new receipt beneath logs/tubes/tbm-final-use/. No production/save/GRF/art change;initial generated company/commissioned spoil logistics remain disclosed inputs. Continue16C from these actual saved worlds without recreating floor or work;16.2/16.3/wholeGroup16 and master10.8 stay pending.

### Local Mandatory Reading - Complete original catalogue in paid-world files

_Applies to16BB;supplements16B's Local/Global/Art,persistence and acceptance Area._

The first16C opposite-reader attempt found an incomplete initial fixture catalogue:unbought capsule engines were absent from ENGN,so loading reconstructed definitions beyond CopyTempEngineData's saved prefix without availability. Read InitialiseTubeContentDefinitions,StartupEngines,CopyTempEngineData/RestoreTubeEngineIntroduction and IsEngineBuildable. Correct only initial generated scenario catalogue setup through native startup before paid cutting;do not grant flags in a loaded observer or change a production policy to repair fixture omissions. Preserve the first16B receipt as dated construction history. Regenerate actual both-reader paid construction originals and retain a read-only opposite-reader catalogue/date probe before16C continues.

### Group 16BB - Preserve the complete native catalogue in original paid-world files

{{HARD}}
- [X] 16BB.1 Initialize the complete scenario catalogue through native engine startup before any paid cuts,and regenerate both actual construction worlds without private availability grants.
- [X] 16BB.2 Verify all126original construction chunks and pure opposite-reader introduced/future capsule/conventional availability,then replace the explicit16C input handoff while preserving dated receipts.

16BB closure,2026-10-19:the initial generated scenario now invokes public native tube definition setup and StartupEngines before paid cutting. Both complete original construction runs retain the same37131ticks/63cuts/50bends/375Earth/750Rock/3750work/2850replacement,with all126complete chunks equal. Two pure opposite-reader application loads independently verify introduced Mole/Crate/Cistern and conventional catalogue admission while future Shield/Forge remain locked;no availability flag is granted by an observer. Current4isolated cases/74378assertions pass ([replacement handoff] (historical reference: `../../../../logs/tubes/tbm-final-use/20261019-native-01/clearance-verification-2.json`),SHA256 b3709dadebb2ceb998b2d7bef7a038a96e7b8287ca1f7c57c5265b0f0383a226). The prior c27f4c8e receipt remains immutable dated construction history and is superseded only as16C's input selection. Reproduce with the explicit clearance-inputs-2.json through tube_tbm_clearance_report.py;catalogue probes use tube_tbm_catalogue.py and each exact opposite construction report. Use tube_tbm_useful.py --clearance-receipt/--clearance-sha256 for this explicit replacement,not a latest-file search. No production/save/GRF/art change;continue16C.

### Local Mandatory Reading - Useful freight on the original paid tube floor

_Applies to16C; supplements Global/Art,persistence and acceptance Area._

Consume16BB's explicit replacement of16B's exact paid cleared/construction world without rerunning excavation in its loaded continuation. Read actual native mine funding/construction/production,ordinary capsule/conventional orders and common RunUsefulTubeNativeOpportunity,joined-station packet custody and paid external power admission. Use src/tests/tube_tbm_useful_fixture.h and src/tests/multilayer/tube_tbm_useful.py;the b3709dade replacement handoff above supersedes c27f4c8e as this input. Prepared source cargo,painted floor,forced station arrivals,private source/packet/vehicle writes or stock truncation cannot establish useful operation. Preserve the original paid spoil receivers,completed job,withdrawn trains and finite seam throughout freight. Ordinary file inputs remain immutable and new outputs use the same declared feature paths.

### Group 16C - Deliver useful freight through the paid curved route

{{HARD}}
- [X] 16C.1 Fund native source,paid external service and conventional interchange/Surface receiver on the original earned floor;earn actual cargo and load a native capsule without a prepared cargo substitute.
- [X] 16C.2 Drive the original capsule and separate conventional carrier to useful Surface delivery/whole empty returns,verify packet provenance,finite reserve,cash/power and preserved completed spoil leg in both readers,then close16.2.

16C.1 closure,2026-10-19:both opposite-reader originals now fund real mine,machinery,Connected conventional shaft,Surface CFMT fuel acceptance,fleet and paid electrical service without altering any earned floor. Actual landscape/industry/common native opportunities finish construction,extract10finite Coal and load9units after3227ticks;the remaining unit stays in native stock. The recoverable opening balance is18000 after14000ore loss already charged by original excavation,so17990remaining+10extracted conserve the actual mining source. Both current readers pass248assertions/two isolated cases and all126complete loaded-ready chunks match ([handoff] (historical reference: `../../../../logs/tubes/tbm-final-use/20261019-native-01/useful-prepare-verification.json`),SHA256 13d536cf642e679636c17fbf28d30a0d91380029e87709ed44404bdd9e8126a2). Continue16C.2 from these exact output worlds;no repeated clearing,funding or private stock/arrival manipulation may supply useful delivery. Native434/GRF/art/production remain unchanged.

16C/original16.2 closure,2026-10-19:both opposite readers continue the exact native loaded-ready worlds without rerunning excavation,funding or source grants. Native StateGameLoop ordering now includes station/industry/company/link landscape maintenance after clock,tile-loop and vehicle work;the focused continuation exposed the earlier observer's missing station-acceptance maintenance,and no production fix was needed. Nine actual Coal units traverse the paid curved capsule route,transfer to the original conventional wagon,cross the real Surface shaft and earn51native freight income. Both whole original carriers return empty through ordinary home-stop orders;125extracted+17875remaining retain the18000post-excavation source,and all continuing physical Coal/provenance/spoil owners balance on every tick. Consumed original paid power reaches1071279J with2551native invoices. Both current readers pass76316assertions/two isolated cases;the two useful-final and untouched whole-next pairs each retain all126complete chunks exactly ([handoff] (historical reference: `../../../../logs/tubes/tbm-final-use/20261019-native-01/useful-delivery-verification.json`),SHA256 9420fa1df3b943b6acb1fc8d4a1d44f7f103a57a500f6896defeeca36bd09662). Continue16D actual same-world queue/files/paid-grid loss/restoration. WholeGroup16/master10.8 and normal-newgame/full balance/human/network/operator gates stay pending.

### Local Mandatory Reading - Original paid-route queues,files and power recovery

_Applies to16D; supplements Global/Art,persistence and acceptance Area._

Consume16C's exact ordinary original worlds. Read current native queue/reservation and source-binding save owners,the existing full-opportunity loader and exact complete chunk comparisons. Retain an actual competing-capsule queue and native paid-grid loss/restoration from these same paid participants;boundary-only queue fixtures and another prepared-floor route are separate historical evidence. Compare original packet/physical/grant owners and uninterrupted full next opportunities through both readers,with only individually field-validated native revision diagnostics admitted.

### Group 16D - Accept original paid-route queue files and paid-grid recovery

{{HARD}}
- [X] 16D.1 Retain actual queued/reserved and paid-grid-starved original capsule worlds;restore supply through ordinary commands and earn resumed intact useful freight.
- [X] 16D.2 Verify original repeat/opposite-reader full files,next-opportunity packet/position/grant/reservation comparisons and sealed paid construction/useful lineage,then close16.3/Group16 and return master10.8.

16D.1 closure,2026-10-19:the original useful world buys a real second Mole at its installed power inlet,then physically drives it empty through the far reversal depot to its paid side berth. Actual native loading/PBS signals create a loaded120-unit queue after254ticks:peer14owns the merge reservation and original11waits. Ordinary CloseTubePower retires the actual paid service,holding both positions,orders,packet fragments/carry and historical consumed energy through20full native opportunities. A fresh actual paid service1 reconnects the same loaded capsules without refunding source0's1429839consumedJ;2848native recovery opportunities deliver all120additional units to the original Surface receiver(129total),with719280Jconsumed from the fresh source. Both capsules complete whole empty home returns;all finite Coal and original375Earth/750Rock spoil owners remain conserved. Both current four-worker readers pass146548assertions/two isolated cases and all eight original/full-next pairs retain every126complete chunks ([handoff] (historical reference: `../../../../logs/tubes/tbm-final-use/20261019-native-01/queue-verification.json`),SHA256 8f52c4afddab198b36f44b48cfba2ca3ccffe59a4878ae44adc569a4ab86ebe0). Native reservation waits may hold a front at its final platform before the signal;do not assume it already entered the signal tile. Initial native binding correctly requires the installed inlet,and signal heads explicitly face both real approaches. Continue16D.2 ordinary original queue/starved/restored/recovered file replay;wholeGroup16/master10.8 remain pending.

16D/original16.3/Group16 closure,2026-10-19:all four actual queue/starved/restored/recovered originals now pass ordinary opposite-reader cold loads,repeat writes,reloads of their ordinary outputs and untouched whole native next opportunities. Current24isolated native file cases/2816assertions pass40complete126-chunk original/repeat/next comparisons and20direct complete126-chunk cross-reader pairs;no field or diagnostic exception is used ([final handoff] (historical reference: `../../../../logs/tubes/tbm-final-use/20261019-native-01/files-verification.json`),SHA256 554c9fa44ef20e5eecdcd77bf88bd91acac14f9912b0adae0194e7bed5f7bce2). The handoff binds exact original paid cutting/spoil/whole withdrawal/final construction,finite mine/native loading,useful Surface delivery and actual queue/paid-source recovery histories without recreating them for file tests. Reproduce each tube_tbm_files.py --config/--queue-report/--stage invocation selected by files-inputs.json,then tube_tbm_files_report.py with that explicit index and an absent receipt output beneath logs/tubes/tbm-final-use/. Each phase reuses one serial runtime home for its three bounded native children. Read-only selected-gallery-20261019 validation still passes69families/10557poses/2209native frames after these test-only changes ([log] (historical reference: `../../../../logs/tubes/tbm-final-use/20261019-native-01/gallery-read-only-1.log`));native434/GRF/sourceart/gameplay remain unchanged. All processes are reaped. Return master10.8 completion review,then11.3 actual host/client scope;wholePlan17/M1 and human/operator/network/full-performance conditions remain distinct.

### Group 17 - Compare tube replay and later geothermal supply

{{HARD}}
- [ ] 17.1 Run matching host/client or deterministic replay fixtures for all five models and compare state at fixed ticks, including hidden views and model-date boundaries.
- [ ] 17.2 Measure capacity, service, paid power and throughput against ordinary Underground rail/road; hand results to r2mlvs16 without declaring balance from static prices.
- [ ] 17.3 After geothermal export is delivered, repeat a network powered by that source and compare single grid credit/consumption. Keep this late case pending without blocking the independent paid-grid evidence.

### Group 18 - Art, persistence and acceptance

{{SIMPLE}}
- [ ] 18.1 Record the implemented public exports, focused test commands/results and remaining runtime/art/network gaps in this plan; retain original supplied saves and repository-local evidence.
- [ ] 18.2 Update affected architecture/handoff documentation to describe delivered behavior and reconcile the parent index only when this plan's Completion Conditions are met. Keep every unfinished task unchecked.

## Consult on Demand

| Trigger | File name | Start at | Expand only when | Consultation purpose |
| --- | --- | --- | --- | --- |
| A powered capsule cannot reserve a route on compatible tubes | [../../architecture/pathfinder/archi-pathfinder-rail.md](../../../../../../../zz-reach2/architecture/pathfinder/archi-pathfinder-rail.md) | `Segment cache` and `Reservation` | Tube capability, topology and power checks pass but cached search/reservation differs | Locate rail cache/reservation invalidation ownership while keeping capsules segregated from ordinary rails and layer portals. |
| A required public export is absent and its owner/location is disputed | [r2mlvs-multilayer-vehicles-and-structures.md](r2mlvs-multilayer-vehicles-and-structures.md) | `Requirements and ownership` and `Plan Index` | The named producer confirms an intentional contract/ownership change | Find the responsible producer and reconcile its public handoff with direct consumers; the hub cannot substitute for missing implementation. |
| Current behavior contradicts a historical subsystem description | [../../architecture/map-layers/archi-map-layers.md](../../architecture/map-layers/archi-map-layers.md) | `Invariants` and the relevant named source in `Code map` | The live symbol and executable test do not explain the difference | Locate layer addressing or lifecycle rationale. Live code/tests govern delivered behavior; this architecture page does not prove future interfaces exist. |

## Planning Provenance

This is not an execution reading queue. Do not preload it after context compaction. It is non-transitive; ranges record planning use, not runtime authority.

| File name | Sections or Line Ranges used while planning | Planning contribution |
| --- | --- | --- |
| [docs/zz-reach2/architecture/initial-inventory.md](../../../../../../../zz-reach2/architecture/initial-inventory.md) | 185-220 | `Command as the only world mutation`, `Pool-indexed game objects`, `Tile map as a packed array` and `File-suffix modules` oriented command/pool/header ownership and source registration. This historical inventory is a location survey, not authority for current layer behavior. |
| [docs/zz-reach2/architecture/@archi-ttd-level0.md](../../../../../../../../../OpenTTD3/docs/zz-reach2/architecture/@archi-ttd-level0.md) | 24-63 | `How the program fits together` and `Source and runtime boundaries` located shared simulation, data/presentation, CMake targets and subsystem handoffs used to choose the live reading sources. |
| [docs/zz-reach2/upgrades/2026-10/r2mlvs-multilayer-vehicles-and-structures.md](r2mlvs-multilayer-vehicles-and-structures.md) | 119-140 | `Requirements and ownership` assigned this plan's feature responsibilities; it shaped the decomposition without making every sibling plan a mandatory reading dependency. |
| [src/rail_type.h](../../../../../../../../src/tests/fixtures/cleanup/source-epochs/openttd3-75ea75718f/eb1fb8c28048bc1e-rail_type.h) | 16-39 | The inspected live boundary helped separate characterization, public-contract production, implementation and acceptance batches. It materially constrained five original layer-local capsules, excavated guideways and explicit paid power; this entry records planning influence rather than an additional reading obligation. |
| [User planning-format instructions] (historical reference: `<C:/Users/Axonn/.codex/attachments/cd59b20a-3a96-4d54-b4bd-1e8f37639e7d/Pasted text.txt>`) | `Global Mandatory Reading`, `Area Mandatory Reading`, `Consult on Demand`, `Planning Provenance`, `Final validation` | Supplies the exact protocol, chronological difficulty batches of at most eight tasks, reading gates and validation format. |
| [src/ground_vehicle.cpp](../../../../../../../../src/tests/fixtures/cleanup/source-epochs/openttd3-75ea75718f/55ac089437075377-ground_vehicle.cpp) | 17-72 | Deeper planning pass, 2026-10-04: Power cache updates and stopped-state behavior required metered movement admission and manual-stop-preserving recovery. |

## Completion Conditions

Five original functional vehicles operate on a distinct underground tube network built only after excavation. Their roles/art/stats are distinguishable, power and cargo transfers are accountable, early play needs no Deep energy, and tubes cannot bypass the 1980 unlock or the real TBM wagon haulage loop.
