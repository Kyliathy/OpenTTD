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

Status, 2026-10-08: actual paid commissioning, TBM/wagon entry/backing/exchange/lifecycle, Deep date/script paths, genuine370/current385 occupied saves and selected final portal art are delivered.12.1 retains real tube/riser integration and18.2 retains full closure. [Final audit](portal-tbm/r2mlvs3mp-z-b-final-closure-audit.md) records authorized goal exceptions; engineering completion and parent status remain pending. Original core checkpoint `ee1cf35a97` and supplied files remain intact. Parent: [R2MLVS master](r2mlvs-multilayer-vehicles-and-structures.md).

**Artifact-layout migration (2026-10-09):** this plan consumes the parent [Development artifact layout](r2mlvs-multilayer-vehicles-and-structures.md#development-artifact-layout), implemented by [R2FC](r2fc-file-cleanup.md). It supersedes earlier docs asset/review/helper placement and fixed runtime-config destinations below. New native homes use `mlm/runtime/`, generated feature reviews use `mlm/reviews/`, logs remain under `logs/`, and reusable code/curated inputs keep their code/test owners. Paths and focused native/cold verification are complete; R2FC records the results and retained-input identities. Repository-relative executable commands run from the checkout root through their context-aware wrappers. Historical pass records remain dated evidence and are not new acceptance claims. No gameplay task is completed by this migration and the separate post-demo fixture-pruning gate remains.

## Outcome and Boundaries

New portal exits begin blocked; a controlled rail commissioning path admits a TBM. Excavated Deep Underground gains rail access through Underground from the proposed 1 January 1980 calendar unlock.

## Public Handoff Contract

R2TBMF9 verified handoff,2026-10-09 ([Y](tbm-fixes/r2tbmf-y-exit-notes.md#group9-closure)): reciprocal `BuildLayerPortal` may replace one terminal temporary rail piece after `CheckTBMExitHandover` proves completed accepted cuts, fully paid/open pocket, correct straight mouth, compatible ownership/gauge and the entire stopped actual TBM clear on the retained approach. Both endpoint/approach checks and all costs are quoted before publication; `CompleteTBMExitHandover` removes only that support row after both native housings and the existing pair owner publish. Ordinary clearing keeps its occupied/protected-support refusal. An already excavated B can connect through the existing command without buying another cutter; A and remaining work support keep their original ownership until final settlement/release. No new saved fields or direct Surface/Deep rail pair is introduced.

R2TBMF8 hangar handoff,2026-10-09: `DescribeTBMHangar` and the synchronized `SendTBMToHangar` command use genuine native Depot/Order identities for an eligible ordinary rail depot or upper portal magazine. Portal registration stores the allocated DepotID in existing map fields since397; no tile-to-depot cast, new cavity or second portal registry is used. Existing portal storage admits one cutter and its own exchange participant; ordinary traffic uses the independent public track and native movement occupancy. Both endpoint model/date/layer capability, ownership, unfinished work and actual magazine occupants are checked. Native demolition/takeover/removal preserve or clear the real depot and its orders. Whole parking, public through traffic, final release and new paid commissioning at B are verified in [T](tbm-fixes/r2tbmf-t-hangar-notes.md), [V](tbm-fixes/r2tbmf-v-hangar-input-notes.md) and [W](tbm-fixes/r2tbmf-w-order-control-notes.md#group8-closure). Completed-terminal far-exit construction from either side, local whole-machine staging and public traffic are now verified by [Group9](tbm-fixes/r2tbmf-y-exit-notes.md#group9-closure).

R2TBMF7B.3 storage/public-track handoff,2026-10-09: `ChangeLayerPortalConnection` admits an unassigned whole stopped same-company native train stored in the upper rail housing's existing magazine while requiring its public track and approaches vacated. Every part must satisfy native depot storage; moving/partly stored trains, work bindings, other claims and ordinary demolition retain their guards. Actual five-part freight crosses this original portal both ways while the same retired cutter remains stored, with paid conversion/track construction verified in [R](tbm-fixes/r2tbmf-r-release-notes.md). Group7B also verifies final release input and actual released/connected-file reuse; compatible curved rail approaches are accepted when they join the door. Alternate-hangar/far-exit workflow remains Groups8-9.

Delivered public owner: `src/layer_portal_cmd.h`. `LayerPortalWorkState`, `GetLayerPortalWorkState`, `CheckLayerPortalWorkAccess`, `CommissionLayerPortalTBM` compile against the actual pair/geological/vehicle owners. Since R2MLVS3MP master 6.6 (verified at the end-of-Group-7 milestone), `CommissionLayerPortalTBM` hands the purchase to Plan 4's `CommissionTBMProject`, which binds the pair through `BindLayerPortalWorkCommissioning`; work admission needs the live project. No substitute runtime mutation is authorized. Downstream plans consume this public contract rather than private pools or copied tables.

Delivered AI/GS owner: `src/script/api/script_rail.hpp`, `ScriptRail::CommissionTBM(TileIndex, SQInteger, SQInteger, SQInteger, StationID, TileIndex, TileIndex, bool)`. It accepts the actual upper rail portal, canonical model, separate Earth/Rock counts, owned Surface origin and requested rock-plane area/partial policy. Its synchronized command retains profile/calendar, full-footprint, capacity, logistics, funds and publication authority; script test mode allocates nothing. Actual Game Script command execution and generated AI/GS bindings are verified in Group 17.

Recurring-machine handoff, 2026-10-06: `LayerPortalCommissionRequest::reuse_cutter` is transient command/quote input, also captured in `LayerPortalBootstrapProof`. The existing commissioning owner selects only an owned healthy empty native machine withdrawn at the same Sealed source after retirement. Source occupancy exempts only that complete revalidated chain; the destination, other vehicles, reservations, owner/calendar/model and project guards remain unchanged. The starter pocket may reuse only exact owned compatible plain rail on already excavated floor. No additional command ID, saved field, purchase or gameplay registry is introduced. Native guards and the complete driven recurring-depot consumer are verified in [Plan 5 Group 13B](r2mlvs5-spoil-wagons-and-haulage.md#group-13b---drive-recurring-underground-bay-exchanges).

Solid-starter proof (Group 8B, implemented 2026-10-05, focused tests pending the milestone build): `LayerPortalBootstrapProof`, `IssueLayerPortalBootstrap`, `RevalidateLayerPortalBootstrap`, `LayerPortalStarterIssuer` and `GetLayerPortalStarterIssuer` in `src/layer_portal_cmd.h` (`src/layer_portal_bootstrap.cpp`). The proof is pure and quotable before any TBM exists. Geology resolves the current issuer only through `GetLayerPortalStarterIssuer`. Rules: [r2mlvs3mp-f](portal-tbm/r2mlvs3mp-f-solid-starter-contract.md).

## Implementation Tasks

Execution coordination: [R2MLVS3MP](r2mlvs3mp-master-plan.md) schedules this plan's remaining groups with their actual producers. It separates the early sealed-start geometry handoff from complete commissioning and returns here for runtime, tube/riser, graphics and persistence closure. Existing X marks remain historical delivered evidence; this scheduling amendment completes no implementation.

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
- [X] 5.3B After r2mlvs4 publishes and tests actual TBM role/consist authorization, integrate successful assigned-TBM entry/retreat without exposing ordinary routing or allowing unrelated followers.
  - R2MLVS3MP status, 2026-10-05: implemented; passed the R2MLVS3MP milestone burst (`r2mlvs3mp-g-burst4-all-results.txt`; historical file unavailable) on 2026-10-05. `CheckLayerPortalWorkAccess` now grants Commissioning-pair admission only when a live TBM project (`GetTBMProjectForCutter`) owns the pair's cutter and job, the entering part has a `GetTBMConsistRole`, and the chain still matches its machinery (`InspectTBMConsist`). Without such a project it still returns `ControllerUnavailable`. Unrelated trains, followers and altered chains are refused, and ordinary track status stays hidden. Test: `src/tests/portal_tbm_access.cpp` (`[portal-tbm-access]`), run at the R2MLVS3MP milestone after master Group 4. Actual playable entry and retreat movement remain with master Group 7.
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

_Applies to Group 8; Groups 8B and 9-12 have their own reading blocks below._

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
- [X] 8.1 Add a portal-associated commissioning control/facility on accessible ground; require rail transport and conventional compatible railtype, excluding tubes and road/elevator origins.
- [X] 8.1A Add the actual either-end portal control window with saved status, validated connection actions and a disabled commissioning purchase control. Publish/read-only validate the accessible conventional-rail assembly site; successful purchase/control activation stays in 8.1 pending r2mlvs4.
- [X] 8.2 Validate model/depth/date, full purchase/assembly footprint, free shaft, two cargo-specific wagon capacities and the required company-owned Surface origin station before reserving a project.
- [X] 8.2A Deliver the read-only portal-side prefix: catalogue model/layer/date, conventional rail, complete source 3x3 floor, free/unreserved shaft, separate positive proposed Earth/Rock capacities and actual owned Surface rail-station reference. Keep complete route/first-cut/backed actual wagon validation in 8.2 pending its controller producer.
- [X] 8.3 Define a depot-compatible commissioning construction boundary or factor a validated train-construction helper with r2mlvs4. `CmdBuildVehicle` requires a depot and its rail builder reads depot direction; never temporarily change the portal tile type to satisfy that precondition.
- [X] 8.4 Define commissioning resource acquisition order and cancellation: acquire project/vehicle/packet capacity through the owning APIs before charging or assigning the pair. Keep r2mlvs4 vehicle creation as an explicit integration seam.
- [X] 8.5 Publish initial-entry and source-side exchange constraints for r2mlvs4/5: first safe cut, backed retreat with actual wagons and unloaded/empty return must work before any underground exchange bay exists.

Group 8 status (2026-10-05, R2MLVS3MP master 6.6): 8.1-8.3 are implemented and verified at the R2MLVS3MP end-of-Group-7 milestone (all 108 project tags (`r2mlvs3mp-l-burst4-all-results.txt`; historical file unavailable) pass; the window control compiles in the Debug game build).
- 8.1: the portal window's Commission TBM control is active for the local company's empty Sealed rail pair. It buys a cutter with two hoppers of each kind, for a straight six-cell corridor out of the door, naming the nearest company Surface railway station, through `CmdCommissionTBM`. Road and elevator ends and the rock side refuse.
- 8.2: everything is validated before any reservation: the portal preflight, then Plan 4's complete route, pass, capacity, funds and receiver checks.
- 8.3: assembly uses Plan 4's factored `BuildTBMConsistAtSite` on the validated source site; the portal tile type is never changed.
- New: `BindLayerPortalWorkCommissioning` binds exactly the Sealed pair revision the preflight saw, as one record change, and notifies routing.
- Tests: `[tbm-commission]` (r2mlvs4 Group 2.4). Not covered here: an area tool with a price preview (Group 13), and the first entry into the pocket (Group 15, master 7.3).

Controller handoff: pure portal/site/model validation -> actual complete route/pass/retreat validation -> reserve owner-specific geometric/project/vehicle/wagon/packet capacity without publication -> recheck pair revision, source floor, funds and actual participant revisions -> commit actual acquisition/payment/job/assignment once -> notify routing. Any refusal must discard all unpublished capacity and preserve cash, RNG, packets and pair authority. Cancellation must first stop and fully withdraw the actual consist; unload/account for owned spoil through the Plan 5 exchange owner, then cancel the real geological job and clear pair assignment once. No direct vehicle-pool edits or depot-type substitution is allowed. Plan 4 must factor its real railway construction entrypoint for this validated source assembly site; `CmdBuildVehicle` requires a genuine depot, so 8.3 waited for Plan 4's factored builder, now delivered as `BuildTBMConsistAtSite`. Plan 2's existing first-cut support requires open starter floor: Plan 4 must reconcile the actual sealed-pocket first cut and paid supports with that producer rather than paint a chamber open. Before any underground bay, loaded retreat and separate Earth/Rock wagon exchange must remain backed to the actual Surface origin, returning empty wagons without any normal traffic permission. Actual acquisition/exchange and first-cut runtime are pending; portal-side prefix evidence is `r2mlvs3-k-commissioning-results.txt`.

### Area Mandatory Reading - Sealed-start geometry handoff

_Applies to Group 8B._

Read master AD-3/AD-5 and R2MLVS3MP Group 2. In live source inspect `ValidateLayerPortalCommissioning`, `LayerPortalWorkRecord`, `GetLayerPortalWorkPocket` in `src/layer_portal_cmd.h`/`src/layer_portal_work.cpp`, and `ExcavationCommissioningPocket`/`ExcavationJob` in `src/excavation.h`. These are the physical/geometric owners; the handoff below is a new producer obligation. It must not require a finished work controller merely to quote source-side support. Actual project publication and vehicle admission still require the tested Plan 4 owner.

### Group 8B - Publish the narrow solid-starter geometry contract

{{HARD}}
- [X] 8B.1 Define a typed portal-issued bootstrap proposal/proof containing canonical pair/revision, owner, model/date, explicit source support, bounded destination pocket and origin reference. Freeze its validation and invalid-reference semantics with Plan 2 Group 14B and Plan 4 Group 2B before dependent code.
- [X] 8B.2 Implement pure issuance/revalidation from the actual accessible rail pair. Distinguish source-side physical support and restricted destination occupancy from excavated destination floor; preserve both housings, normal routing and packed rock.
- [X] 8B.3 Publish the exact public location/signatures and update direct geology/controller consumers. Quote issuance must work before full TBM acquisition; later assignment/publication must still resolve actual train/project references through their owners.
- [X] 8B.4 Test stale pair revision, foreign/blocked source, wrong facility/model/date and unchanged estimate/refusal state. Compile the Plan 2 consumer fixture; claim only this geometry contract, not a paid cut, live entry or exchange.

**R2MLVS3MP status, 2026-10-05.** 8B.1 is frozen in [r2mlvs3mp-f](portal-tbm/r2mlvs3mp-f-solid-starter-contract.md), with the typed proof in `src/layer_portal_cmd.h`. 8B.2-8B.4 are implemented in `src/layer_portal_bootstrap.cpp`, plus the shared pocket helper and load binding in `layer_portal_work.cpp`. They passed the R2MLVS3MP milestone burst (`r2mlvs3mp-g-burst4-all-results.txt`; historical file unavailable) on 2026-10-05 (`[portal-bootstrap]`, `src/tests/portal_bootstrap.cpp`). The burst corrected the test's door-cell check: the door cell D' touches the housing and lies inside the pocket. Claimed scope: the geometry/proof contract only, not a paid cut, live entry or exchange.

### Area Mandatory Reading - Commissioned pair and lifecycle integration

_Applies to Groups 9-12._

Read master AD-3/AD-4, the preceding Commissioning and bootstrap area's acquisition/cancellation handoff, and the applicable R2MLVS3MP checkpoint. Inspect the live `ChangeLayerPortalConnection`, `CheckLayerPortalLifecycle`, `CheckLayerPortalWorkAccess` and `CommissionLayerPortalTBM` owners. Assigned-consist integration requires Plan 4's tested role/controller/retreat functions and Plan 5's tested exchange/recovery functions; unavailable consumers leave only their dependent scenarios pending. Group 12's actual tube/riser integration additionally requires the scoped Plan 16/17 producers scheduled by R2MLVS3MP; catalogue exclusions alone cannot complete it.

### Group 9 - Connect only a cleared and safely reserved pair

{{HARD}}
- [X] 9.1 Implement a pure connection check for both excavated approaches, compatible track, cleared work reservations and current date/owner. Recheck immediately before changing work state.
- [X] 9.2 Refuse an unsafe connectivity change while any actual train part occupies either endpoint/approach; preserve each part position and reservation. No occupied transition is supported; real fleet spanning-consist integration stays in 9.3/15.3.
- [X] 9.3 Commit the connected state once and invalidate both endpoints. Exercise a fully withdrawn TBM then normal freight, plus a spanning-consist refusal that leaves its route recoverable.
- [X] 9.3A Deliver the synchronized `CmdChangeLayerPortalConnection` command, pure estimates, one canonical revision change and both-end invalidation; test actual spatial-owner occupancy refusal and idempotent connection. Full TBM/freight/spanning-consist scenarios remain in 9.3 pending r2mlvs4/5.

Evidence: `r2mlvs3-l-connection-results.txt`, 299 assertions/four focused cases including commissioning prefix and date boundaries. The occupied reference is an actual pool-owned train part registered with `Vehicle::UpdatePosition`; no synthetic occupancy cache or actual fleet scenario is claimed.

Group 9.3 status (2026-10-05, R2MLVS3MP master 7.2): verified at the milestone. In `[portal-recovery]` "A withdrawn pair connects for freight and refuses a spanning consist":
- a real project completes its cut and exchange and is retired, which withdraws the pair;
- the cutter is sold, the released support is converted to permanent track, and the Surface approach gets axis track;
- a hauler with parts at both ends keeps the pair Sealed and stays where it stands;
- once it has gone, the connection succeeds, and both ends show track to ordinary routing again.

No code change was needed. A freight train actually driving through the connected pair is Group 15's connected crossing (master 7.3).

### Group 10 - Commissioning and bootstrap

{{MEDIUM}}
- [X] 10.1 Apply blocked-exit behavior to road tunnels/elevators. They can connect excavated approaches but cannot commission TBMs or clear rock. Existing legacy portals retain prior behavior under their saved profile.

Evidence: `r2mlvs3-m-road-results.txt` exercises all three existing road origins through the actual build/connect/commissioning APIs, requiring real open floor and same-company axis roads without geological credit. Classic legacy date/routing remains covered by the date fixture.

### Group 11 - Recover commissioning projects before lifecycle mutation

{{HARD}}
- [X] 11.1 Define a transition table for cancel, remove, rail conversion, company transfer and blocked source approach. Separate an empty pair from a pair occupied by cutter/wagons.
- [X] 11.2 For occupied/assigned pairs, refuse destructive changes until the controller reaches a recoverable stopped state; use the public project cancellation/reassignment contract, not direct vehicle-pool edits.
- [X] 11.3 For an empty safe pair, release project/work claims and update both endpoint ownership/type/accounting together. Test failure at either endpoint never leaves a half-pair.
- [X] 11.3A Deliver safe unassigned metadata removal from either end, paired rail conversion, once-only paired company transfer/accounting and explicitly sealed ownerless road retention. Preserve packed geology and validate retained ownerless saved authority. Actual assigned-project release stays in 11.3/11.4.
- [X] 11.4 Integrate the table with the real r2mlvs4/5 controller/exchange once delivered; until then retain its dependent scenarios as pending and verify only the available portal-state boundary.

| Operation | Empty, unassigned, unoccupied pair | Assigned or occupied pair |
| --- | --- | --- |
| Seal/cancel | Validate both ends, release visible ordinary reservations before sealing. | Refuse until actual controller stops, backs out with wagons, accounts for spoil and cancels the geometric job. |
| Remove | Validate ownership/both ends/doors/claims; retire one record and both housings/accounting, retaining rock. | Refuse without moving parts, releasing claims or changing cash. |
| Rail conversion | Validate both housings, date and free reservations; change both types/accounting and stamp once. | Refuse until actual controller releases/revalidates work. |
| Company transfer | Flush old-owner signals, update one record and both physical owners/counters once. Empty road bankruptcy retains explicit ownerless Sealed housing; rail bankruptcy removes the empty pair. | Refuse the portal callback unchanged. Actual company deletion/orphan recovery must be integrated with Plan 4 before live commissioning can be enabled. |
| Blocked source approach | No entry/connection/assembly permission; safe empty removal remains available. | Keep project/reference state recoverable and require the actual support/controller pause/cancellation path. |

Evidence: `r2mlvs3-n-lifecycle-results.txt`, 113 assertions/four cases using real paired commands, real company callback/accounting and assigned geological/train references. Direct command fixtures flush real queued signals before their company/world teardown. No occupied TBM bankruptcy recovery or project/exchange cancellation runtime claim is made.

Group 11.3/11.4 status (2026-10-05, R2MLVS3MP master 6.4): implemented and verified at the milestone. Design: [r2mlvs3mp-j](portal-tbm/r2mlvs3mp-j-lifecycle-design.md).
- 11.3: `WithdrawLayerPortalWorkPair(tile, job, cutter, execute)` releases a Commissioning pair as one record change. It requires:
  - exactly that binding;
  - a closed job (completed, cancelled or retired);
  - no part or claim on the rock-side end or its door.

  The withdrawn consist may still stand at the Surface key. Plan 4's Retire calls it in the same action.
  - The released pair is an ordinary empty pair, so 11.3A's paired removal takes it whole from either end once the train has gone.
  - A refusal at either end leaves both ends.
- 11.4, the table with the real controller and exchange:
  - Seal/cancel of an assigned pair is Plan 4 Cancel, then the return, the spoil exchange, Cancelled, and Retire, which withdraws the pair.
  - Remove and convert stay refused until then.
  - Company transfer:
    - A merger now moves an assigned or occupied pair with its project, job, trains and stations (`TransferLayerPortalWorkPair` needs an empty pair only for ownerless road retention), and only once.
    - Company removal first lets the TBM owner cancel the erased project's job and release its pair (`ReleaseLayerPortalWorkBinding`); the empty rail pair is then removed with the company.
    - A direct removal callback on a pair still assigned keeps refusing unchanged.
- Supporting change: a Commissioning record may name a closed job while its cutter returns.
- Master 6.5 additions ([ground audit](portal-tbm/r2mlvs3mp-k-ground-persistence-audit.md)): `ForgetLayerPortalWorkCutter` and `ForgetLayerPortalWorkOrigin`, called by the TBM deletion hooks, clear a deleted cutter or origin station from a Commissioning record at once. The record keeps its job binding and still loads; a reused ID never rebinds.
- Tests: `src/tests/portal_recovery.cpp` (`[portal-recovery]`).
- Not covered here: connecting the released pair to ordinary freight (9.3, master 7.2), and the occupied spanning-consist cases (master Group 7).

### Group 12 - Commissioning and bootstrap

Goal-authorized skip (2026-10-08, master finalization): dispatched expansion work in this group is skipped under [finalization exception GP-TG](portal-tbm/r2mlvs3mp-z-a-finalization-ledger.md#gp-tg-tube-and-geothermal-expansion-requires-major-gap-planning). Only the tube/riser or final-use portion is skipped; independent completed work and turn/recovery checks retain their evidence. No skipped task is markedX.

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

Context has compacted 1 times while executing this group.

### Group 13 - Controls, art and acceptance

{{MEDIUM}}
- [X] 13.1 Show sealed face, TBM-only commissioning and connected traffic states in portal preview, tooltip and sprites.
- [X] 13.1A Deliver actual either-end click controls, saved status/date label and tooltip, plus validated synchronized Connect/Seal actions. Commissioning purchase stays disabled until its real controller exists. State-preview/final sprites remain in 13.1 under the verified Plan 13 art gate (`facility.portal.sealed` source/packed path/hash are still absent).

13.1 closure (2026-10-07, master 8.7): the pair window renders both actual endpoint layers, door directions and saved Sealed/Commissioning/Connected sprite states; the construction picker correctly previews a newly Sealed pair. Status/tooltips and traffic permissions retain their existing owners. The selected packed portal art and real crossing captures are verified by Plan 13 Group 82. [Focused portal checks](../../../../logs/r2mlvs3mp-s-portal-preview-checks.log) pass 94 assertions / four cases, and [native controls capture](../../../../logs/r2mlvs3mp-s-portal-preview-runtime.log) passes 97 assertions / one case. The [actual Commissioning pair preview](../../../../logs/proofs/mlm/native-art/portals/picker/images/m1-controls-portal.png) was inspected for endpoint placement and labels; user art review remains deferred.

### Group 14 - Controls, art and acceptance

{{SIMPLE}}
- [X] 14.1 Update road/elevator far-side errors to name excavation as the missing step. Connection checks validate both geological foundations before approach track compatibility, returning the existing explicit excavation error for a solid far face; covered by `r2mlvs3-p-controls-results.txt`.

### Group 15 - Controls, art and acceptance

{{MEDIUM}}
- [X] 15.1 Test every allowed/refused layer pair, all four directions, Deep build tools, both-end removal/conversion and attempted road/elevator commissioning. `r2mlvs3-q-acceptance-results.txt`: 350 assertions/seven cases through actual matrix/command/lifecycle boundaries. Fleet movement cases remain separately pending below.
- [X] 15.2 Run a TBM first-entry and retreat scenario, normal train refusal before opening, then freight crossing after opening.
- [X] 15.3 Test reversal and signals with a consist spanning each new rail pair.

Group 15 status (2026-10-05, R2MLVS3MP master 7.3): 15.2 and 15.3 pass by real movement, with all 111 project tags
(burst (`r2mlvs3mp-l-movement-burst3-all.txt`; historical file unavailable)). The design is [r2mlvs3mp-m](portal-tbm/r2mlvs3mp-m-movement-design.md).
- 15.2, `[tbm-movement]` (`src/tests/tbm_movement.cpp`), "A freight train crosses the tunnel its TBM bored once the
  pair is opened":
  - The started TBM leaves its magazine, bores three cuts and backs in for its exchange.
  - The project retires, the pair is Sealed again, the cutter is sold and its support becomes permanent track.
  - A freight train driven up to the Sealed door finds a line end, turns back to its depot and never enters the pair.
  - After the player's connect command, the same train drives through the pair to the end of the last cut and back.
  - An ordinary train is also turned back at a Commissioning pair's door, with the cutter consist in its magazine.
- 15.3, `[portal-movement]` (`src/tests/portal_movement.cpp`), Surface to Underground and Underground to Deep:
  - A freight train crosses a Connected pair and comes back.
  - Reversed while it spans the pair, it stays whole on its tiles.
  - The two-way block signals at both ends show red while it spans the pair and green once it is home.
- The movement tags check every tick that no train crashes and that every part stands on the tile its position names.

### Local Mandatory Reading - Actual occupied portal files

_Applies to Group 16; supplements Controls, art and acceptance above._

Read the delivered Plan 5 actual source/depot file closures, retained genuine version-370 evidence immediately below this group and only the selected native portal work/vehicle/job/reference fixups. Inspect `GetLayerPortalPairTile`, packed endpoint/connected/reservation accessors, the complete native map chunk family and the actual application loader. Reuse immutable paid full worlds where they satisfy a precise portal state; physically observe assigned cutter/wagons in the work pocket and both sides of its pair. Preserve original legacy inputs and check that migration keeps Classic usable edges without opening new rock. No copied rows or file relabeling complete this scope.

### Group 16 - Validate saved portal state and occupied references

{{HARD}}
- [X] 16.1 Create separate Sealed, Commissioning and Connected fixtures and compare pair keys, work status, date policy and endpoint geometry after reload.
- [X] 16.1A Verify actual Sealed/Connected repeated files (`r2mlvs3-e-save-results.txt`) and all three public owner-state round trips with actual geological/train references (`r2mlvs3-d-state-results.txt`). Actual Commissioning vehicle-file integration remains in 16.1/16.2 pending r2mlvs4/5.
- [X] 16.2 After r2mlvs4/5 integration, save with a TBM/wagons in the work pocket and with a train spanning the pair; check part positions, reservations and the next legal transition after reload.
- [X] 16.3 Load genuine version-370 rail/elevator fixtures under legacy policy, with separately authored version-370 road-tunnel coverage where original road tunnels are absent. Verify no state migration opens new solid rock or removes an existing usable legacy route.
- [X] 16.3A Reload the unchanged supplied version-370 `build/save/camera-deep-repaired.sav`, plus separate owned version-370 rail/road/elevator topology and repeated Native ownerless-road files. This particular camera save has no portal tiles; genuine portal worlds were subsequently located and verified in 16.3. `r2mlvs3-r-save-results.txt`: five actual loads, exact endpoint metadata/work/geology and retained supplied bytes.

Follow-up correction: genuine `build/save/makinpr2.sav` and `makinpr2-connected.sav` both contain five rail pairs and four Deep road elevators. `r2mlvs3-y-original-results.txt` records their four actual migration/repeat loads and a fifth separately authored road-tunnel load. The accepted-world diagnostic queries actual `GetTileTrackStatus` at both ends, proving all original rail/elevator ordinary edges remain usable. Eighteen complete original endpoint field sets remain exact; Classic creates neither portal work rows nor packed geology. Original input/startup/config hashes are unchanged. Eight focused state/routing/lifecycle cases pass 285 assertions (`r2mlvs3-z-legacy-diagnostic-results.txt`). The earlier assertion that original portal fixtures were absent was based on checking only the empty camera save and is superseded by this evidence.

16.1-16.2 occupied-file closure, 2026-10-06: genuine paid full Deep cutter/wagon movement produces separate stopped Commissioning, actual work-pocket wagon and actual cutter-chain spanning inputs. Producer passes 156,202 assertions / one case in 3.720 seconds ([ax](../../../../logs/r2mlvs3-ax-native-pocket-file-writer.log)); the physical states occur after 1 / 2,851 / 139 frames. One/two full application loads reconstruct the exact physical parts, reservations, original packet age/quantity, jobs/projects/support, pair geometry, queues, receipts, cost and date state. The same next physical exchange occurs after 2,164 stopped, 10,223 pocket and 2,099 cutter-spanning frames ([ay](../../../../logs/r2mlvs3-ay-stopped-1.log), [ba](../../../../logs/r2mlvs3-ba-work-pocket-1.log), [bb](../../../../logs/r2mlvs3-bb-cutter-spanning-1.log), each with paired `-2` evidence). No pose, phase or cargo is manufactured. Existing real Connected hauler-spanning source/depot files also resume identically.

Separate preserved empty native375 Sealed and Connected inputs load/reload through the current application with unchanged pair keys, whole work rows, all endpoint fields/height, complete saved profile/date policy and calendar technology date ([bd](../../../../logs/r2mlvs3-bd-1.log), [paired](../../../../logs/r2mlvs3-bd-2.log), [be](../../../../logs/r2mlvs3-be-1.log), [paired](../../../../logs/r2mlvs3-be-2.log)). The complete packed comparison now covers 40 native owners, adding every map dimension/height/extended-field chunk. All 24 accepted source/depot families and both complete visibility triples pass this stronger offline audit ([az](../../../../logs/r2mlvs3-az-complete-portal-map-owner-checks.log)); historical unaccepted cargo-weight prototypes remain preserved and excluded rather than waived.

Fresh actual genuine370 original-portal and original-connected copies retain their supplied source hashes and each load/reload with all 18 complete endpoint field sets unchanged ([bh](../../../../logs/r2mlvs3-bh-1.log), [paired](../../../../logs/r2mlvs3-bh-2.log), [bi](../../../../logs/r2mlvs3-bi-1.log), [paired](../../../../logs/r2mlvs3-bi-2.log)). Native track-status queries preserve five/five usable rail pairs and four/four usable Deep elevators, with no invalid pair. Both stay Classic/legacy, with zero Native work pairs, generated reserves or packed rock; entire original map dimensions/type/height bytes remain exact. Earlier independently authored old road-tunnel coverage stays explicitly separate. Portal-only startup/save uses two declared null-video ticks, keeping each paired compatibility burst under ten seconds. Original supplied files are unchanged; new binary fixtures use non-planning names. Continuation: [aw](r2mlvs3-aw-occupied-portal-continuation.md). Final sprites and occupied multiplayer acceptance remain later master scopes.

### Group 17 - Controls, art and acceptance

{{MEDIUM}}
- [X] 17.1 Test 31 December 1979 refusal and 1 January 1980 success through every Deep rail/elevator path, both build directions, scripting and commissioning.
- [X] 17.1A Exercise the actual shared build/connection/routing/tile-admission/commissioning-refusal gate at both endpoints and all four headings. Publish/compile AI/GS `ScriptRail`/`ScriptRoad` BuildLayerPortal/ConnectLayerPortal bindings, and execute the script APIs through actual Game Script company/test scopes and the real command kernel at both dates. `r2mlvs3-s-date-script-results.txt`: 282 assertions/four cases. Full scripted fleet scenarios and successful actual TBM commissioning remain in 17.1 pending their controller; this test does not claim execution of a supplied Squirrel game script.
- [X] 17.2 Check a legacy pre-1980 save still follows its explicit compatibility mode and changing economy speed cannot unlock technology early. Existing original370/owned legacy file loads retain Classic policy; actual legacy pre-1980 elevator queries and advanced economy/frozen calendar tests use the shared gate.

Group 17.1 status (2026-10-05, R2MLVS3MP master 7.5): the commissioning part is verified at the milestone. `[tbm-commission]` "A Deep cutter commissions at an Underground portal only from 1 January 1980" builds an Underground-to-Deep rail pair on opened Underground ground. Commissioning there is refused on 31 December 1979 with the Deep message and nothing changed, refused for the standard cutter, and succeeds on 1 January 1980. Still open in 17.1: a script path for commissioning (no script binding exists yet) and actual fleet movement through Deep pairs (master Group 7B).

17.1 closure, 2026-10-06: `ScriptRail::CommissionTBM` is exported to AI/GS and forwards validated integer fields to the existing synchronized command. Actual Game Script company/test/execute scopes verify pure estimates, pre-1980 refusal with unchanged money/pools/geology/RNG, and a successful paid native Deep consist/job/project publication with the ordinary script suspension protocol. Actual started rail traffic and native bulk trucks at both Surface/Deep and Underground/Deep elevators remain outside Deep on 31 December 1979 despite a 2090 economic date, then complete descent and return on 1 January 1980. The existing build/activation/script matrix covers both endpoint orders and all four headings, plus explicit legacy/editor policies and economy-speed independence. Debug game/test builds pass in [ar](../../../../logs/r2mlvs3-ar-script-fleet-date-build.log); eight focused tags pass 34,886 assertions / 18 cases in under two seconds ([as](../../../../logs/r2mlvs3-as-calendar-command-matrix.log)). The test fatal reporter preserves the native error/abort and emits text/stacks without a modal box; it changes no video or simulation state. Full supplied-Squirrel-script acceptance remains with the ground runner; this is actual API/runtime command execution.

Context has compacted 1 times while executing this group.

### Group 18 - Controls, art and acceptance

{{SIMPLE}}
- [X] 18.1 Record the implemented public exports, focused test commands/results and remaining runtime/art/network gaps in this plan; retain original supplied saves and repository-local evidence.
- [ ] 18.2 Update affected architecture/handoff documentation to describe delivered behavior and reconcile the parent index only when this plan's Completion Conditions are met. Keep every unfinished task unchecked.
- [X] 18.2A Update map-layer/savegame architecture and the delivered public handoff; keep parent completion and every gated integration task unchanged until full Completion Conditions are actually met.

Finalization reconciliation, 2026-10-08: delivered public work/commissioning/script owners and current385 occupied/cancelled continuation, final selected states/occlusion and current Deep/profile/control/lifecycle checks are mapped in [the final audit](portal-tbm/r2mlvs3mp-z-b-final-closure-audit.md). Both final builds and162,516 boundary assertions/38 cases per configuration pass. Actual replacement-origin command recovery is tested, with no GUI picker claim.18.2 is **authorized skipped for goal finalization** because12.1 needs the multi-group GP-TG producers; it remains unchecked and Plan3 remains incomplete. GP-NJ/GP-BT/GP-PF and deferred personal feedback do not become accepted runtime evidence. Earlier absence statements below describe their historical snapshot; this paragraph and live checkboxes govern current delivery.

## Execution evidence and remaining gates

Deferred tasks and their exact remaining work are tracked in [R2MLVS99 - Leftovers](r2mlvs99-leftovers.md). This plan remains incomplete; its unfinished checkboxes and parent completion status are unchanged.

Public owner: `src/layer_portal_cmd.h`; actual canonical store: `src/layer_portal_work.cpp`. Delivered work queries/records/pockets, `CheckLayerPortalAvailability`, `CheckLayerPortalWorkAccess`, `CommissionLayerPortalTBM` request/result, `CmdChangeLayerPortalConnection`, safe empty lifecycle/capacity APIs and actual AI/GS BuildLayerPortal/ConnectLayerPortal consumers all compile in Debug. Save version 375's LPWH/LPWK and diagnostic root publish after accepted content/geology. Native sealed construction never opens rock, and ordinary movement/routing remains independently gated by phase/date.

Final focused command: `build/Debug/openttd_test.exe` selecting `[portal-build]`, `[portal-routing]`, `[portal-connect]`, `[portal-lifecycle]`, `[portal-date]`, `[portal-script]`, `[portal-profile]`, `[portal-work-state]`, `[portal-contract]`, `[portal-road]`, `[portal-quote]`, `[portal-acceptance]`, two relevant excavation tags and five existing layer/WindowDesc boundaries. `r2mlvs3-t-final-results.txt`: **29 cases, 1,498 assertions passed**, approximately 0.4 seconds. A combined run exposed fixture-only stale vehicle hashes; actual pool teardown now calls the real `ResetVehicleHash`, and the same combined burst passed afterward. Readonly saved Connected edge validation also uses bounded actual door helpers rather than asserting on malformed edge addresses. Debug `openttd_test` and `openttd` builds pass. No full regression was run.

Actual loader evidence: eleven initial native/state/refusal cases (`e`), five original/owned legacy and ownerless round trips (`r`), one targeted older Native ownerless migration (`u`), and four separate Underground/Deep Sealed/Connected loads plus repeats (`x`): 21 actual creation/load/refusal cases. The older ownerless pair becomes explicitly Sealed/unassigned without a new company, cavity or traffic authority. Retained inputs/startup/config bytes are unchanged. Public copied-state tests cover all three work phases with real geological/train references; actual occupied TBM/wagon save and movement scenarios remain pending. Final small follow-ups passed 134 metadata/state assertions (`v`) after replacing save-copy allocation with the public count query, and 244 construction/state assertions (`w`) after publication rechecked actual profile/company/pair authority, including Classic refusal. `git diff --check` and fixture syntax checks pass.

Named producer gates remain in 5.3B, 12.1, 13.1, 15.2-15.3, 16.1-16.2 and 17.1; 8.1-8.3, 9.3 and 11.3-11.4 closed at the R2MLVS3MP end-of-Group-7 milestone. Plan 4 must deliver actual TBM engine/consist authorization, site construction, paid first solid-pocket cut and recoverable project lifecycle; Plan 5 supplies actual source-side wagon/packet exchange. Plans 16/17 own actual capsule/interchange/riser consumers. Plan 13's state-face/Deep sprites still have planned source/packed fields. Genuine version-370 portal migration is now verified; actual new TBM/spanning-consist acceptance remains pending its producers. Multiplayer joins/deterministic occupied-controller transitions and final rendered controls/art review are not claimed. Full plan completion remains gated. The repository now contains core checkpoint `ee1cf35a97`; no Git write was performed during this legacy follow-up.

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
