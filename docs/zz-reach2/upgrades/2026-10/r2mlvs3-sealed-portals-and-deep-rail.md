# R2MLVS3 - Sealed Portals and Deep Rail

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
| [docs/zz-reach2/upgrades/2026-10/r2mlvs-multilayer-vehicles-and-structures.md](r2mlvs-multilayer-vehicles-and-structures.md) | 204-212 | `AD-3` supplies the canonical gameplay/ownership decision for every group of this plan. It constrains paired sealed topology, rail-only commissioning and the 1980 Deep gate; a consumer must not redefine it locally. |
| [docs/zz-reach2/upgrades/2026-10/r2mlvs-multilayer-vehicles-and-structures.md](r2mlvs-multilayer-vehicles-and-structures.md) | 214-220 | `AD-4` supplies the canonical gameplay/ownership decision for every group of this plan. It constrains paired sealed topology, rail-only commissioning and the 1980 Deep gate; a consumer must not redefine it locally. |
| [src/map_layer.h](../../../../src/map_layer.h) | 78-85 | `MultiLayerMap` ownership preamble: `Map` owns packed storage/geometry and this controller owns layer semantics. Every group uses world tiles/explicit layer helpers; camera selection and ad hoc plane offsets cannot define gameplay. |
| [src/map_layer.h](../../../../src/map_layer.h) | 281-315 | `IsPortalPairAllowed`, `GetPortalTargetLayer`, `CheckLayerAllowsConstruction`, `CheckTilesShareLayer` define live connection/construction authority; new profile checks must preserve legacy behavior. |

Status: available portal core, controls, script commands and persistence delivered and tested; exact completed tasks are marked X. Actual TBM/wagon/controller, capsule/utility integration, original occupied legacy portal fixtures and final state art remain gated and unchecked. Parent completion index is unchanged because this plan's full Completion Conditions are not met. Parent: [R2MLVS master](r2mlvs-multilayer-vehicles-and-structures.md). Depends on r2mlvs2.

## Outcome and Boundaries

New portal exits begin blocked; a controlled rail commissioning path admits a TBM. Excavated Deep Underground gains rail access through Underground from the proposed 1 January 1980 calendar unlock.

## Public Handoff Contract

Delivered public owner: `src/layer_portal_cmd.h`. `LayerPortalWorkState`, `GetLayerPortalWorkState`, `CheckLayerPortalWorkAccess`, `CommissionLayerPortalTBM` compile against the actual pair/geological/vehicle owners. Work admission and purchase return `ControllerUnavailable` until r2mlvs4/5 provide actual TBM role/consist and acquisition/exchange APIs; no substitute runtime mutation is authorized. Downstream plans consume this public contract rather than private pools or copied tables.

## Implementation Tasks

Deepening pass, 2026-10-04: implementation groups use only SIMPLE, MEDIUM or HARD difficulty, with at most eight pending tasks each. Sketches describe proposed implementation contracts; verify the named earlier producers at each assumption gate. No gameplay or asset implementation is marked complete by this planning pass.

Groups are chronological and contain consecutive tasks of one difficulty. Each difficulty batch has at most eight tasks; area reading changes only when the implementation boundary or source of truth changes.

### Area Mandatory Reading - Portal state and connection matrix

_Applies to Groups 1-7._

**Entry gate.** The following public contracts must exist before dependent implementation in this area: `src/multilayer_content.h`: `MultiLayerGameplayProfile`, `LayerVehicleCapability`, `GetMultiLayerContentProfile`, `CheckDeepLayerAvailability`, `UtilityPowerAccount`; `src/excavation.h`: `ExcavationState`, `GeologyDeposit`, `ExcavationJob`, `CheckExcavatedFootprint`, `CommitExcavationCut`, `GeothermalVentField`.
Verify each public file/export/schema field with `rg` in the current workspace and inspect its command/field ownership. For a same-plan contract, compile its type/signature fixture or validate its artifact after the preceding producer groups. For cross-plan behavior, also run the producer's focused executable boundary test; declarations alone cannot authorize dependent mutations or runtime claims.
If a required export is absent, stop the dependent implementation, mutation and runtime claims in Groups 1-7. Read-only characterization, source/range verification, test-case design and asset-template work that does not assume the missing behavior may continue.
Do not bypass this gate by copying private pool/tile state, inventing local IDs or cargo registries, using a historical plan as a delivered implementation, opening user-wide content stores, substituting a second utility/accounting authority, or adding an unreviewed fallback path. Finish the named producer or revise the shared contract and its direct consumers first.

| File name | Line Ranges | Description |
| --- | --- | --- |
| [src/map_layer.cpp](../../../../src/map_layer.cpp) | 95-139 | `MultiLayerMap::IsPortalPairAllowed` and `GetPortalTargetLayer` are complete live pair/destination rules; extend them profile-selectively for Deep rail. |
| [src/layer_portal_map.h](../../../../src/layer_portal_map.h) | 63-137 | `GetLayerPortalTargetLayer`, pair/kind and reservation accessors own the portal map representation; state changes must preserve paired topology and reservations. |
| [src/timer/timer_game_calendar.h](../../../../src/timer/timer_game_calendar.h) | 15-39 | `TimerGameCalendar` technology-clock contract and date conversions govern 1980/model/factory introductions; economic/realtime scheduling cannot unlock a technology. |
| [src/tests/map_layer.cpp](../../../../src/tests/map_layer.cpp) | 269-291 | Complete Deep road-only rule test is a legacy baseline; add profile-selective expectations rather than erasing Classic behavior. |
| `src/multilayer_content.h` (planned public contract; link after implementation) | Live definitions; no fixed range | [r2mlvs1-gameplay-contracts-and-content.md](r2mlvs1-gameplay-contracts-and-content.md) Groups 1-6, ending with Group 6's public-contract task, publish `MultiLayerGameplayProfile`, `LayerVehicleCapability`, `GetMultiLayerContentProfile`, `CheckDeepLayerAvailability`, `UtilityPowerAccount`. These exports/schema fields are authoritative for this area. Same-plan consumers follow those earlier groups; cross-plan consumers verify the producer's required implementation/tests as well. Proposed names alone do not prove delivery. |
| `src/excavation.h` (planned public contract; link after implementation) | Live definitions; no fixed range | [r2mlvs2-geology-and-excavation.md](r2mlvs2-geology-and-excavation.md) Groups 1-7, ending with Group 7's public-contract task, publish `ExcavationState`, `GeologyDeposit`, `ExcavationJob`, `CheckExcavatedFootprint`, `CommitExcavationCut`, `GeothermalVentField`. These exports/schema fields are authoritative for this area. Same-plan consumers follow those earlier groups; cross-plan consumers verify the producer's required implementation/tests as well. Proposed names alone do not prove delivery. |
| [src/layer_portal_cmd.cpp](../../../../src/layer_portal_cmd.cpp) | 438-503 | Complete `CmdBuildLayerPortal` is the current paired mutation boundary; its ordinary far-end clear must be replaced by restricted sealed-housing creation on solid rock. |
| [src/layer_portal_cmd.cpp](../../../../src/layer_portal_cmd.cpp) | 218-228 | `GetTileTrackStatus_LayerPortal` defines ordinary routing; sealed state hides its edge without altering pair geometry. |
| [src/layer_portal_map.h](../../../../src/layer_portal_map.h) | 200-212 | Complete `GetLayerPortalNextTile` remains the physical cross-layer neighbor authority; permission belongs to routing and work-access validation. |
| [src/layer_portal_cmd.cpp](../../../../src/layer_portal_cmd.cpp) | 349-360 | Complete `NotifyRailPortalChanged` updates signals and YAPF after both endpoint state changes commit. |
| [src/layer_portal_cmd.cpp](../../../../src/layer_portal_cmd.cpp) | 311-347 | Complete `VehicleEnterTile_LayerPortal` is the actual admission boundary; cached routes do not replace assigned-consist validation. |
| [src/pbs.cpp](../../../../src/pbs.cpp) | 150-202 | Complete `UnreserveRailTrack` requires existing track status during release; drain reservations before hiding a connected edge. |

**ASSUMPTION: this was already built previously. If not, re-asses at runtime**. This applies only to the producer contracts named in this area's gate, after their listed producer groups have run. Verify their actual exports and focused tests before using the sketches below; proposed types and helper names in sketches are not evidence of delivered code.

**Portal-state sketch (proposed).** State controls permission; stored pair geometry remains valid in every state. `GetLayerPortalNextTile` must continue to identify the paired endpoint even when ordinary routing refuses the connection.

```mermaid
stateDiagram-v2
    [*] --> Sealed: paired housings built
    Sealed --> Commissioning: validated rail project assigned
    Commissioning --> Sealed: safely withdrawn and cancelled
    Commissioning --> Connected: cleared approaches and released work reservation
    Sealed --> Connected: separately validated pre-cleared approaches
```

```text
ordinary edge = paired state is Connected AND ordinary track/date rules pass
work entry    = assigned project + assigned TBM + valid commissioning pocket
physical next tile = existing paired-end geometry
```

### Group 1 - Portal state and connection matrix

{{MEDIUM}}
- [X] 1.1 Extend the profile-specific construction/pair matrix to Underground <-> Deep rail.
- [X] 1.2 Keep Surface <-> Underground rail/road and existing road elevators.
- [X] 1.3 Preserve layer IDs and the direct elevator's untouched intervening tile.

### Group 2 - Define one saved portal work state per pair

{{HARD}}
- [X] 2.1 Choose the canonical pair key from existing endpoint geometry; store Sealed/Commissioning/Connected, owner and assigned project/vehicle reference once. Both ends query that same authority.
- [X] 2.2 Specify the restricted shaft/work-pocket footprint and which fields are derived connectivity/reservation caches. Do not mark its rock as generally excavated.
- [X] 2.3 Add versioned descriptors and legacy defaults: existing legacy portals load as their previous usable connection, while new-profile unfinished pairs keep their saved work state.
- [X] 2.4 Test pair queries from either endpoint, invalid/missing pair references and all three state round trips before routing consumes the state.

### Group 3 - Portal state and connection matrix

{{MEDIUM}}
- [X] 3.1 Add an Underground rail destination picker and accurate command quotes.

Context has compacted 1 times while executing this group.

### Group 4 - Preflight and create sealed pairs atomically

{{HARD}}
- [X] 4.1 Extract a read-only build proposal validating both endpoints, target pair, owner, date, geology, occupied approaches and existing reservations. Quote all clearing/housing costs without changing either tile.
- [X] 4.2 Replace the unconditional far-side ordinary-clear path for solid terrain with a narrowly scoped portal-housing operation. It may create the sealed shaft representation but cannot create an excavated chamber or expose a general excavation bypass.
- [X] 4.3 Reserve fallible pair/work-state capacity before mutation, then commit both ends, work state and infrastructure accounting together. Test second-end refusal and bounded capacity refusal leave the first end, funds and RNG unchanged; handle allocation exceptions before mutation.

Execution refinement: Native endpoint sites must be prepared plain Clear ground; existing objects/infrastructure require their own explicit safe clearing commands first. Restricted far-side housing changes only map metadata, keeping packed Solid ground and excavation/accounting authority untouched. Direct command-body tests verify returned costs and unchanged cash; the normal command kernel owns payment. Evidence: `r2mlvs3-g-build-results.txt`, 167 assertions/four cases. The actual capacity fixture uses 4,096 validated physical pairs; allocator exceptions are handled before writes, without a global allocator fault-injection claim.

### Group 5 - Expose sealed routing without losing physical pair geometry

{{HARD}}
- [X] 5.1 Initialize portals into solid rock as Sealed. When approaches are already excavated, require a separate validated connection action; neither action grants excavation credit.
- [X] 5.2 Make ordinary track-status and path/signal queries return no usable edge for Sealed/Commissioning pairs. Retain `GetLayerPortalNextTile` as the physical neighbor authority; do not break saved pair geometry to hide an edge.
- [X] 5.3 Define `CheckLayerPortalWorkAccess` as a distinct vehicle/project-aware permission and recheck actual movement so cached paths, force-proceed and unrelated followers cannot admit ordinary traffic. Until the actual TBM role/consist owner exists, return `ControllerUnavailable` for an otherwise valid assignment.
- [ ] 5.3B After r2mlvs4 publishes and tests actual TBM role/consist authorization, integrate successful assigned-TBM entry/retreat without exposing ordinary routing or allowing unrelated followers.
- [X] 5.4 Drain/release ordinary reservations while old track status is still valid before hiding an edge: `UnreserveRailTrack` asserts the track exists. Then commit connectivity and notify both ends through signal/pathfinder invalidation. Test a cached follower and an unrelated actual train reference; occupied real TBM scenarios remain with 5.3B/15.2.

### Group 6 - Use the shared date gate on every Deep entry

{{HARD}}
- [X] 6.1 Consume r2mlvs1 `CheckDeepLayerAvailability`; apply it to Underground/Deep rail build, activation and work-access commissioning validation in both directions. Group 7's commissioning entrypoint must consume this same query.
- [X] 6.2 Apply the same query to Surface/Deep and Underground/Deep road elevator build, activation and entry. Test both endpoints so an ascent-side tool cannot bypass the rule.
- [X] 6.3 Require the shared excavation/construction guard to cover Deep actions and publish operation-gate requirements for later mines/geothermal plants. Existing feature owners implement their actual production paths; do not claim those future paths are already wired.
- [X] 6.4 Keep geological preview read-only while locked. Test legacy and saved editor exceptions separately from normal profile behavior and prove economy-speed changes do not alter the unlock.

Operation handoff: every Deep mine/geothermal/tube producer must call the existing `CheckDeepLayerAvailability` with accepted saved policy and synchronized calendar time before production/work/entry, as well as the shared complete excavation construction guard for foundations/approaches. An electrical riser must additionally reference an already Connected, date-valid actual portal. These later production owners are not implemented here. Evidence: `r2mlvs3-i-date-results.txt`, 261 assertions/three focused cases; all four shaft headings and both build orders, both-end activation/track/admission refusal, unchanged locked geology/work, saved editor exception, legacy elevators and independently advanced economy date/frozen calendar speed.

### Group 7 - Publish portal work access and commissioning boundary

{{HARD}}
- [X] 7.1 Publish `LayerPortalWorkState`, `GetLayerPortalWorkState` and `CheckLayerPortalWorkAccess` in existing `src/layer_portal_cmd.h`, with pair ownership, invalid-reference and read-only query rules.
- [X] 7.2 Publish `CommissionLayerPortalTBM` request/result contract for portal-side validation, assigned project identity and controller handoff. The next area implements portal-side behavior; r2mlvs4 supplies actual TBM vehicle creation/work and r2mlvs5 the exchange behavior.
- [X] 7.3 Compile the public contract fixture, register any new source and record exactly which behaviors are tested. If the public location changes, update all consumers before marking this handoff ready.

Evidence: `r2mlvs3-j-contract-results.txt` compiles the public signatures and verifies canonical upper-end identity, real command/company ownership, unchanged estimates/execute refusals, no allocated job/cutter IDs and no work/geology/cash mutation. Successful TBM role admission, purchase, actual packet/wagon exchange and first solid-pocket cut remain dependent on the named producers.

### Area Mandatory Reading - Commissioning and bootstrap

_Applies to Groups 8-12._

**Entry gate.** The following public contracts must exist before dependent implementation in this area: `src/layer_portal_cmd.h`: `LayerPortalWorkState`, `GetLayerPortalWorkState`, `CheckLayerPortalWorkAccess`, `CommissionLayerPortalTBM`.
Verify each public file/export/schema field with `rg` in the current workspace and inspect its command/field ownership. For a same-plan contract, compile its type/signature fixture or validate its artifact after the preceding producer groups. For cross-plan behavior, also run the producer's focused executable boundary test; declarations alone cannot authorize dependent mutations or runtime claims.
If a required export is absent, stop the dependent implementation, mutation and runtime claims in Groups 8-12. Read-only characterization, source/range verification, test-case design and asset-template work that does not assume the missing behavior may continue.
Do not bypass this gate by copying private pool/tile state, inventing local IDs or cargo registries, using a historical plan as a delivered implementation, opening user-wide content stores, substituting a second utility/accounting authority, or adding an unreviewed fallback path. Finish the named producer or revise the shared contract and its direct consumers first.

| File name | Line Ranges | Description |
| --- | --- | --- |
| [src/layer_portal_cmd.cpp](../../../../src/layer_portal_cmd.cpp) | 438-503 | `CmdBuildLayerPortal` validates and clears both ends before paired execution. Add sealing/availability atomically; never grant a free far-side cavern. |
| [src/train_cmd.h](../../../../src/train_cmd.h) | 18-27 | Build/sell/move/reverse train commands and traits are the public commissioning/rearrangement boundary; restrict specialist capabilities through every applicable command. |
| [src/layer_portal_cmd.cpp](../../../../src/layer_portal_cmd.cpp) | 218-228 | `GetTileTrackStatus_LayerPortal` is the ordinary traversable-edge contract; sealed ends must disappear from normal pathfinding and signals. |
| [src/layer_portal_cmd.h](../../../../src/layer_portal_cmd.h) | Live definitions; no fixed range | [r2mlvs3-sealed-portals-and-deep-rail.md](r2mlvs3-sealed-portals-and-deep-rail.md) Groups 1-7, ending with Group 7's public-contract task, publish `LayerPortalWorkState`, `GetLayerPortalWorkState`, `CheckLayerPortalWorkAccess`, `CommissionLayerPortalTBM`. These exports/schema fields are authoritative for this area. Same-plan consumers follow those earlier groups; cross-plan consumers verify the producer's required implementation/tests as well. Proposed names alone do not prove delivery. |
| [src/vehicle_cmd.cpp](../../../../src/vehicle_cmd.cpp) | 104-239 | Complete `CmdBuildVehicle` requires a depot and handles temporary estimate construction; commissioning must supply a valid factored construction boundary. |
| [src/layer_portal_cmd.cpp](../../../../src/layer_portal_cmd.cpp) | 362-418 | Complete `ClearTile_LayerPortal` owns paired demolition; occupied commissioning projects must be resolved/refused before either end changes. |

**ASSUMPTION: this was already built previously. If not, re-asses at runtime**. This applies only to the producer contracts named in this area's gate, after their listed producer groups have run. Verify their actual exports and focused tests before using the sketches below; proposed types and helper names in sketches are not evidence of delivered code.

**Staged integration.** The earlier portal groups supply state and public access checks. This area can implement those validations before a TBM exists. Actual purchase/entry, source-side wagon exchange and occupied-project lifecycle scenarios additionally consume r2mlvs4 `TBMProject`/`TBMWorkState` and r2mlvs5 `TBMExchangeAssignment`/`ExecuteTBMWagonExchange` once their producer implementations and boundary tests exist. **ASSUMPTION: this was already built previously. If not, re-asses at runtime**. For those integrations only, verify the public headers/tests; stop the dependent mutation/scenario if absent, continue pure portal tests, and never substitute fake wagons or a second project store.

Creation order: preflight both endpoints -> reserve required record capacity -> create paired housing/work state -> update accounting -> publish routing invalidation. Ordinary `LandscapeClear` cannot be used to bore the far side after r2mlvs2's solid-rock guard lands.

### Group 8 - Build portal-side commissioning validation

{{HARD}}
- [ ] 8.1 Add a portal-associated commissioning control/facility on accessible ground; require rail transport and conventional compatible railtype, excluding tubes and road/elevator origins.
- [X] 8.1A Add the actual either-end portal control window with saved status, validated connection actions and a disabled commissioning purchase control. Publish/read-only validate the accessible conventional-rail assembly site; successful purchase/control activation stays in 8.1 pending r2mlvs4.
- [ ] 8.2 Validate model/depth/date, full purchase/assembly footprint, free shaft, two cargo-specific wagon capacities and the required company-owned Surface origin station before reserving a project.
- [X] 8.2A Deliver the read-only portal-side prefix: catalogue model/layer/date, conventional rail, complete source 3x3 floor, free/unreserved shaft, separate positive proposed Earth/Rock capacities and actual owned Surface rail-station reference. Keep complete route/first-cut/backed actual wagon validation in 8.2 pending its controller producer.
- [ ] 8.3 Define a depot-compatible commissioning construction boundary or factor a validated train-construction helper with r2mlvs4. `CmdBuildVehicle` requires a depot and its rail builder reads depot direction; never temporarily change the portal tile type to satisfy that precondition.
- [X] 8.4 Define commissioning resource acquisition order and cancellation: acquire project/vehicle/packet capacity through the owning APIs before charging or assigning the pair. Keep r2mlvs4 vehicle creation as an explicit integration seam.
- [X] 8.5 Publish initial-entry and source-side exchange constraints for r2mlvs4/5: first safe cut, backed retreat with actual wagons and unloaded/empty return must work before any underground exchange bay exists.

Controller handoff: pure portal/site/model validation -> actual complete route/pass/retreat validation -> reserve owner-specific geometric/project/vehicle/wagon/packet capacity without publication -> recheck pair revision, source floor, funds and actual participant revisions -> commit actual acquisition/payment/job/assignment once -> notify routing. Any refusal must discard all unpublished capacity and preserve cash, RNG, packets and pair authority. Cancellation must first stop and fully withdraw the actual consist; unload/account for owned spoil through the Plan 5 exchange owner, then cancel the real geological job and clear pair assignment once. No direct vehicle-pool edits or depot-type substitution is allowed. Plan 4 must factor its real railway construction entrypoint for this validated source assembly site; `CmdBuildVehicle` currently requires a genuine depot, so 8.3 remains pending. Plan 2's existing first-cut support requires open starter floor: Plan 4 must reconcile the actual sealed-pocket first cut and paid supports with that producer rather than paint a chamber open. Before any underground bay, loaded retreat and separate Earth/Rock wagon exchange must remain backed to the actual Surface origin, returning empty wagons without any normal traffic permission. Actual acquisition/exchange and first-cut runtime are pending; portal-side prefix evidence is `r2mlvs3-k-commissioning-results.txt`.

### Group 9 - Connect only a cleared and safely reserved pair

{{HARD}}
- [X] 9.1 Implement a pure connection check for both excavated approaches, compatible track, cleared work reservations and current date/owner. Recheck immediately before changing work state.
- [X] 9.2 Refuse an unsafe connectivity change while any actual train part occupies either endpoint/approach; preserve each part position and reservation. No occupied transition is supported; real fleet spanning-consist integration stays in 9.3/15.3.
- [ ] 9.3 Commit the connected state once and invalidate both endpoints. Exercise a fully withdrawn TBM then normal freight, plus a spanning-consist refusal that leaves its route recoverable.
- [X] 9.3A Deliver the synchronized `CmdChangeLayerPortalConnection` command, pure estimates, one canonical revision change and both-end invalidation; test actual spatial-owner occupancy refusal and idempotent connection. Full TBM/freight/spanning-consist scenarios remain in 9.3 pending r2mlvs4/5.

Evidence: `r2mlvs3-l-connection-results.txt`, 299 assertions/four focused cases including commissioning prefix and date boundaries. The occupied reference is an actual pool-owned train part registered with `Vehicle::UpdatePosition`; no synthetic occupancy cache or actual fleet scenario is claimed.

### Group 10 - Commissioning and bootstrap

{{MEDIUM}}
- [X] 10.1 Apply blocked-exit behavior to road tunnels/elevators. They can connect excavated approaches but cannot commission TBMs or clear rock. Existing legacy portals retain prior behavior under their saved profile.

Evidence: `r2mlvs3-m-road-results.txt` exercises all three existing road origins through the actual build/connect/commissioning APIs, requiring real open floor and same-company axis roads without geological credit. Classic legacy date/routing remains covered by the date fixture.

### Group 11 - Recover commissioning projects before lifecycle mutation

{{HARD}}
- [X] 11.1 Define a transition table for cancel, remove, rail conversion, company transfer and blocked source approach. Separate an empty pair from a pair occupied by cutter/wagons.
- [X] 11.2 For occupied/assigned pairs, refuse destructive changes until the controller reaches a recoverable stopped state; use the public project cancellation/reassignment contract, not direct vehicle-pool edits.
- [ ] 11.3 For an empty safe pair, release project/work claims and update both endpoint ownership/type/accounting together. Test failure at either endpoint never leaves a half-pair.
- [X] 11.3A Deliver safe unassigned metadata removal from either end, paired rail conversion, once-only paired company transfer/accounting and explicitly sealed ownerless road retention. Preserve packed geology and validate retained ownerless saved authority. Actual assigned-project release stays in 11.3/11.4.
- [ ] 11.4 Integrate the table with the real r2mlvs4/5 controller/exchange once delivered; until then retain its dependent scenarios as pending and verify only the available portal-state boundary.

| Operation | Empty, unassigned, unoccupied pair | Assigned or occupied pair |
| --- | --- | --- |
| Seal/cancel | Validate both ends, release visible ordinary reservations before sealing. | Refuse until actual controller stops, backs out with wagons, accounts for spoil and cancels the geometric job. |
| Remove | Validate ownership/both ends/doors/claims; retire one record and both housings/accounting, retaining rock. | Refuse without moving parts, releasing claims or changing cash. |
| Rail conversion | Validate both housings, date and free reservations; change both types/accounting and stamp once. | Refuse until actual controller releases/revalidates work. |
| Company transfer | Flush old-owner signals, update one record and both physical owners/counters once. Empty road bankruptcy retains explicit ownerless Sealed housing; rail bankruptcy removes the empty pair. | Refuse the portal callback unchanged. Actual company deletion/orphan recovery must be integrated with Plan 4 before live commissioning can be enabled. |
| Blocked source approach | No entry/connection/assembly permission; safe empty removal remains available. | Keep project/reference state recoverable and require the actual support/controller pause/cancellation path. |

Evidence: `r2mlvs3-n-lifecycle-results.txt`, 113 assertions/four cases using real paired commands, real company callback/accounting and assigned geological/train references. Direct command fixtures flush real queued signals before their company/world teardown. No occupied TBM bankruptcy recovery or project/exchange cancellation runtime claim is made.

### Group 12 - Commissioning and bootstrap

{{MEDIUM}}
- [ ] 12.1 Do not treat tube guideways as eligible rail commissioning infrastructure or allow tube capsules through rail portals/elevators. Supply tube/utility interchanges on excavated accessible ground; a geothermal electrical riser may attach only to an already commissioned, date-valid portal through r2mlvs17.
- [X] 12.1A Reject tube/capsule and ordinary wagon logical models at the actual commissioning catalogue boundary. Publish the Connected/date/excavation requirement for later tube/utility/riser owners; actual capsule movement/interchanges remain in 12.1 pending r2mlvs16/17.

### Area Mandatory Reading - Controls, art and acceptance

_Applies to Groups 13-18._

**Entry gate.** The following public contracts must exist before dependent implementation in this area: `src/layer_portal_cmd.h`: `LayerPortalWorkState`, `GetLayerPortalWorkState`, `CheckLayerPortalWorkAccess`, `CommissionLayerPortalTBM`.
Verify each public file/export/schema field with `rg` in the current workspace and inspect its command/field ownership. For a same-plan contract, compile its type/signature fixture or validate its artifact after the preceding producer groups. For cross-plan behavior, also run the producer's focused executable boundary test; declarations alone cannot authorize dependent mutations or runtime claims.
If a required export is absent, stop the dependent implementation, mutation and runtime claims in Groups 13-18. Read-only characterization, source/range verification, test-case design and asset-template work that does not assume the missing behavior may continue.
Do not bypass this gate by copying private pool/tile state, inventing local IDs or cargo registries, using a historical plan as a delivered implementation, opening user-wide content stores, substituting a second utility/accounting authority, or adding an unreviewed fallback path. Finish the named producer or revise the shared contract and its direct consumers first.

| File name | Line Ranges | Description |
| --- | --- | --- |
| [src/tests/map_layer.cpp](../../../../src/tests/map_layer.cpp) | 374-427 | Complete permitted/refused portal matrix tests currently restrict rail to Surface/Underground; add explicit new-profile and calendar cases. |
| [src/tests/map_layer.cpp](../../../../src/tests/map_layer.cpp) | 430-442 | Complete display-order test retains immutable Space=2 and Deep=3 storage meanings; future UI/profile work cannot reorder saved IDs. |
| [src/saveload/vehicle_sl.cpp](../../../../src/saveload/vehicle_sl.cpp) | 1119-1196 | `VEHSChunkHandler` and registration construct the vehicle class from saved type before member loading; protect old enum meanings and validate new references. |
| [src/layer_portal_cmd.h](../../../../src/layer_portal_cmd.h) | Live definitions; no fixed range | [r2mlvs3-sealed-portals-and-deep-rail.md](r2mlvs3-sealed-portals-and-deep-rail.md) Groups 1-7, ending with Group 7's public-contract task, publish `LayerPortalWorkState`, `GetLayerPortalWorkState`, `CheckLayerPortalWorkAccess`, `CommissionLayerPortalTBM`. These exports/schema fields are authoritative for this area. Same-plan consumers follow those earlier groups; cross-plan consumers verify the producer's required implementation/tests as well. Proposed names alone do not prove delivery. |
| [Sprite Contract and delivery owner](r2mlvs13-graphics-and-asset-pipeline.md#public-handoff-contract) | Live definitions; no fixed range | `Sprite Contract Handoff` defines manifest fields `asset_id`, `consumer`, `directions`, `frames`, `anchor`, `source`; the matching authoring/conversion/runtime groups deliver actual assets. Verify the requested family and packed loading fixture before graphics integration; template/manifest publication alone is not final art. |

**ASSUMPTION: this was already built previously. If not, re-asses at runtime**. This applies only to the producer contracts named in this area's gate, after their listed producer groups have run. Verify their actual exports and focused tests before using the sketches below; proposed types and helper names in sketches are not evidence of delivered code.

**Acceptance staging.** Portal matrix, date and empty-pair persistence checks can run before the fleet. First entry/retreat, spanning TBM consists and real wagon exchange require the r2mlvs4/5 public implementations described in the preceding integration gate; art review requires r2mlvs13's matching assets. Missing producers leave only those scenarios pending, with no synthetic runtime-pass claim.

**Scoped graphics gate. ASSUMPTION: this was already built previously. If not, re-asses at runtime**. Final graphics tasks in this area require the corresponding r2mlvs13 manifest entries, delivered sprites and packed loading fixture. Verify them before binding/review; if absent, stop only those art tasks and their acceptance claims, continue independent code/state tests, and never count placeholders as finished assets.

### Group 13 - Controls, art and acceptance

{{MEDIUM}}
- [ ] 13.1 Show sealed face, TBM-only commissioning and connected traffic states in portal preview, tooltip and sprites.
- [X] 13.1A Deliver actual either-end click controls, saved status/date label and tooltip, plus validated synchronized Connect/Seal actions. Commissioning purchase stays disabled until its real controller exists. State-preview/final sprites remain in 13.1 under the verified Plan 13 art gate (`facility.portal.sealed` source/packed path/hash are still absent).

### Group 14 - Controls, art and acceptance

{{SIMPLE}}
- [X] 14.1 Update road/elevator far-side errors to name excavation as the missing step. Connection checks validate both geological foundations before approach track compatibility, returning the existing explicit excavation error for a solid far face; covered by `r2mlvs3-p-controls-results.txt`.

### Group 15 - Controls, art and acceptance

{{MEDIUM}}
- [X] 15.1 Test every allowed/refused layer pair, all four directions, Deep build tools, both-end removal/conversion and attempted road/elevator commissioning. `r2mlvs3-q-acceptance-results.txt`: 350 assertions/seven cases through actual matrix/command/lifecycle boundaries. Fleet movement cases remain separately pending below.
- [ ] 15.2 Run a TBM first-entry and retreat scenario, normal train refusal before opening, then freight crossing after opening.
- [ ] 15.3 Test reversal and signals with a consist spanning each new rail pair.

### Group 16 - Validate saved portal state and occupied references

{{HARD}}
- [ ] 16.1 Create separate Sealed, Commissioning and Connected fixtures and compare pair keys, work status, date policy and endpoint geometry after reload.
- [X] 16.1A Verify actual Sealed/Connected repeated files (`r2mlvs3-e-save-results.txt`) and all three public owner-state round trips with actual geological/train references (`r2mlvs3-d-state-results.txt`). Actual Commissioning vehicle-file integration remains in 16.1/16.2 pending r2mlvs4/5.
- [ ] 16.2 After r2mlvs4/5 integration, save with a TBM/wagons in the work pocket and with a train spanning the pair; check part positions, reservations and the next legal transition after reload.
- [ ] 16.3 Load original version-370 rail/road/elevator fixtures under legacy policy. Verify no state migration opens new solid rock or removes an existing usable legacy route.
- [X] 16.3A Reload the unchanged supplied version-370 `build/save/camera-deep-repaired.sav`, plus separate owned version-370 rail/road/elevator topology and repeated Native ownerless-road files. Supplied original has no portal tiles, so original occupied portal-route preservation in 16.3 remains pending suitable fixtures. `r2mlvs3-r-save-results.txt`: five actual loads, exact endpoint metadata/work/geology and retained supplied bytes.

### Group 17 - Controls, art and acceptance

{{MEDIUM}}
- [ ] 17.1 Test 31 December 1979 refusal and 1 January 1980 success through every Deep rail/elevator path, both build directions, scripting and commissioning.
- [X] 17.1A Exercise the actual shared build/connection/routing/tile-admission/commissioning-refusal gate at both endpoints and all four headings. Publish/compile AI/GS `ScriptRail`/`ScriptRoad` BuildLayerPortal/ConnectLayerPortal bindings, and execute the script APIs through actual Game Script company/test scopes and the real command kernel at both dates. `r2mlvs3-s-date-script-results.txt`: 282 assertions/four cases. Full scripted fleet scenarios and successful actual TBM commissioning remain in 17.1 pending their controller; this test does not claim execution of a supplied Squirrel game script.
- [X] 17.2 Check a legacy pre-1980 save still follows its explicit compatibility mode and changing economy speed cannot unlock technology early. Existing original370/owned legacy file loads retain Classic policy; actual legacy pre-1980 elevator queries and advanced economy/frozen calendar tests use the shared gate.

### Group 18 - Controls, art and acceptance

{{SIMPLE}}
- [X] 18.1 Record the implemented public exports, focused test commands/results and remaining runtime/art/network gaps in this plan; retain original supplied saves and repository-local evidence.
- [ ] 18.2 Update affected architecture/handoff documentation to describe delivered behavior and reconcile the parent index only when this plan's Completion Conditions are met. Keep every unfinished task unchecked.
- [X] 18.2A Update map-layer/savegame architecture and the delivered public handoff; keep parent completion and every gated integration task unchanged until full Completion Conditions are actually met.

## Execution evidence and remaining gates

Public owner: `src/layer_portal_cmd.h`; actual canonical store: `src/layer_portal_work.cpp`. Delivered work queries/records/pockets, `CheckLayerPortalAvailability`, `CheckLayerPortalWorkAccess`, `CommissionLayerPortalTBM` request/result, `CmdChangeLayerPortalConnection`, safe empty lifecycle/capacity APIs and actual AI/GS BuildLayerPortal/ConnectLayerPortal consumers all compile in Debug. Save version 375's LPWH/LPWK and diagnostic root publish after accepted content/geology. Native sealed construction never opens rock, and ordinary movement/routing remains independently gated by phase/date.

Final focused command: `build/Debug/openttd_test.exe` selecting `[portal-build]`, `[portal-routing]`, `[portal-connect]`, `[portal-lifecycle]`, `[portal-date]`, `[portal-script]`, `[portal-profile]`, `[portal-work-state]`, `[portal-contract]`, `[portal-road]`, `[portal-quote]`, `[portal-acceptance]`, two relevant excavation tags and five existing layer/WindowDesc boundaries. `r2mlvs3-t-final-results.txt`: **29 cases, 1,498 assertions passed**, approximately 0.4 seconds. A combined run exposed fixture-only stale vehicle hashes; actual pool teardown now calls the real `ResetVehicleHash`, and the same combined burst passed afterward. Readonly saved Connected edge validation also uses bounded actual door helpers rather than asserting on malformed edge addresses. Debug `openttd_test` and `openttd` builds pass. No full regression was run.

Actual loader evidence: eleven initial native/state/refusal cases (`e`), five original/owned legacy and ownerless round trips (`r`), one targeted older Native ownerless migration (`u`), and four separate Underground/Deep Sealed/Connected loads plus repeats (`x`): 21 actual creation/load/refusal cases. The older ownerless pair becomes explicitly Sealed/unassigned without a new company, cavity or traffic authority. Retained inputs/startup/config bytes are unchanged. Public copied-state tests cover all three work phases with real geological/train references; actual occupied TBM/wagon save and movement scenarios remain pending. Final small follow-ups passed 134 metadata/state assertions (`v`) after replacing save-copy allocation with the public count query, and 244 construction/state assertions (`w`) after publication rechecked actual profile/company/pair authority, including Classic refusal. `git diff --check` and fixture syntax checks pass.

Named producer gates remain in 5.3B, 8.1-8.3, 9.3, 11.3-11.4, 12.1, 13.1, 15.2-15.3, 16.1-16.3 and 17.1. Plan 4 must deliver actual TBM engine/consist authorization, site construction, paid first solid-pocket cut and recoverable project lifecycle; Plan 5 supplies actual source-side wagon/packet exchange. Plans 16/17 own actual capsule/interchange/riser consumers. Plan 13's state-face/Deep sprites still have planned source/packed fields. Original version-370 occupied portal/fleet evidence is absent; the supplied original has no portal tiles and separate owned topology is labelled accordingly. Multiplayer joins/deterministic occupied-controller transitions and final rendered controls/art review are not claimed. Full plan completion and a Plan 3 commit are deferred until those required gates close; the sole Git write in this execution was the explicitly requested Plan 2 checkpoint commit.

## Consult on Demand

| Trigger | File name | Start at | Expand only when | Consultation purpose |
| --- | --- | --- | --- | --- |
| Converting one rail portal end changes its pair or consist incorrectly | [../../../../src/rail_cmd.cpp](../../../../src/rail_cmd.cpp) | `CmdConvertRail` | Pair validation or infrastructure/consist accounting fails after the new profile extension | Locate the live paired conversion contract; do not edit one end before whole-pair validation. |
| A required public export is absent and its owner/location is disputed | [r2mlvs-multilayer-vehicles-and-structures.md](r2mlvs-multilayer-vehicles-and-structures.md) | `Requirements and ownership` and `Plan Index` | The named producer confirms an intentional contract/ownership change | Find the responsible producer and reconcile its public handoff with direct consumers; the hub cannot substitute for missing implementation. |
| Current behavior contradicts a historical subsystem description | [../../architecture/map-layers/archi-map-layers.md](../../architecture/map-layers/archi-map-layers.md) | `Invariants` and the relevant named source in `Code map` | The live symbol and executable test do not explain the difference | Locate layer addressing or lifecycle rationale. Live code/tests govern delivered behavior; this architecture page does not prove future interfaces exist. |

## Planning Provenance

This is not an execution reading queue. Do not preload it after context compaction. It is non-transitive; ranges record planning use, not runtime authority.

| File name | Sections or Line Ranges used while planning | Planning contribution |
| --- | --- | --- |
| [docs/zz-reach2/architecture/initial-inventory.md](../../architecture/initial-inventory.md) | 185-220 | `Command as the only world mutation`, `Pool-indexed game objects`, `Tile map as a packed array` and `File-suffix modules` oriented command/pool/header ownership and source registration. This historical inventory is a location survey, not authority for current layer behavior. |
| [docs/zz-reach2/architecture/@archi-ttd-level0.md](../../architecture/@archi-ttd-level0.md) | 24-63 | `How the program fits together` and `Source and runtime boundaries` located shared simulation, data/presentation, CMake targets and subsystem handoffs used to choose the live reading sources. |
| [docs/zz-reach2/upgrades/2026-10/r2mlvs-multilayer-vehicles-and-structures.md](r2mlvs-multilayer-vehicles-and-structures.md) | 119-140 | `Requirements and ownership` assigned this plan's feature responsibilities; it shaped the decomposition without making every sibling plan a mandatory reading dependency. |
| [src/map_layer.h](../../../../src/map_layer.h) | 281-315 | The inspected live boundary helped separate characterization, public-contract production, implementation and acceptance batches. It materially constrained paired sealed topology, rail-only commissioning and the 1980 Deep gate; this entry records planning influence rather than an additional reading obligation. |
| [User planning-format instructions](<C:/Users/Axonn/.codex/attachments/cd59b20a-3a96-4d54-b4bd-1e8f37639e7d/Pasted text.txt>) | `Global Mandatory Reading`, `Area Mandatory Reading`, `Consult on Demand`, `Planning Provenance`, `Final validation` | Supplies the exact protocol, chronological difficulty batches of at most eight tasks, reading gates and validation format. |
| [src/pbs.cpp](../../../../src/pbs.cpp) | 150-202 | Deeper planning pass, 2026-10-04: Reservation-release assertions shaped state-change ordering and the separate assigned-TBM work permission. |

## Completion Conditions

Creating a tunnel never produces an immediately usable unexcavated network. A first TBM can safely enter and retreat without an underground depot. Deep rail and both elevator access routes obey the 1980 calendar unlock in the new profile; tube tracks cannot commission a TBM. Existing legacy routes retain their documented compatibility behavior.
