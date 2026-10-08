# R2TBMF - TBM excavation fixes and usable tunnel workflows

Status: implementation plan,2026-10-09. No implementation task below is complete. This plan responds to the user's eight gameplay reports and the supplied screenshot. The user requested finishing the current geothermal group and stopping the broader queue: [R2MLVS3MP](r2mlvs3mp-master-plan.md) is paused after verified Plan17 Group4, before Group5. This document does not resume that queue.

Owners: [Plan2 geology/excavation](r2mlvs2-geology-and-excavation.md), [Plan3 portals](r2mlvs3-sealed-portals-and-deep-rail.md), [Plan4 TBMs](r2mlvs4-tunnel-boring-machines.md), [Plan5 haulage](r2mlvs5-spoil-wagons-and-haulage.md), [Plan7 construction/fleet](r2mlvs7-underground-construction-and-fleet.md) and [Plan13 graphics](r2mlvs13-graphics-and-asset-pipeline.md). Architecture: [map layers](../../architecture/map-layers/archi-map-layers.md), [vehicles](../../architecture/vehicles/archi-vehicles.md), [rail pathfinding](../../architecture/pathfinder/archi-pathfinder-rail.md), [GUI](../../architecture/gui/archi-gui.md), [savegames](../../architecture/savegame/archi-savegame.md). Parent decisions and invariants remain in [R2MLVS](r2mlvs-multilayer-vehicles-and-structures.md); user-requested behavior changes below must be reconciled there during implementation.

## Global Mandatory Reading

Before starting implementation and after every context compaction, read this section, **User requirements and decisions**, the current group's nearest **Local Mandatory Reading**, and that group's tasks. Read only the named symbols/sections needed for the group. Reading is non-transitive: do not reload whole parent plans, unrelated geothermal/orbital work or historical test logs.

| Source | Required scope | Reason |
| --- | --- | --- |
| This plan | User requirements, evidence table, active group and completion conditions | Preserve every reported problem and distinguish observed behavior from an unverified diagnosis. |
| R2MLVS parent | AD-3..AD-6; I-2..I-8, I-12..I-15 | Actual vehicles, paid excavation, explicit layer addressing, safe retreat, atomic stock/cash and deterministic work. Read affected decisions again only when their behavior changes. |
| Plan2 / Plan3 / Plan4 / Plan5 | Public Handoff Contract or exported owner symbols relevant to the active group | One owner for geometry, commissioning, vehicle work, support, orders and wagon/cargo custody. |
| Plan14 | Existing feature save/preview and lifecycle contract only when introducing persistent fields or changing old-save behavior | Real files must preserve active work, routes, fractional amounts and actual vehicles. |

Execution rules:

- Execute groups top to bottom. Mark a task `[X]` only after its stated behavior and focused evidence pass; planning, a declaration or a numerical fixture does not complete player-facing behavior.
- Each group has at most eight tasks. After compaction record its count above the active group. Beyond three compactions, split that group into B/C/D groups with narrower Local Mandatory Reading rather than repeatedly loading unrelated context.
- Source/test filenames must not begin with `r2`. Assisting Markdown or scratch files belong beside this plan with chronological `r2tbmf-a-...`, `r2tbmf-b-...` names. Logs go in repository `logs/`; actual saves go under an appropriate `src/tests/fixtures/` feature directory, and review images under `assets/reviews/`.
- All created/modified source and test files need an architecture/owning-plan header. Every new/modified function documents its purpose, parameters and return value in JavaDoc style; classes/interfaces have JavaDoc headers. Explain compound conditions and nontrivial loops in terms of gameplay and affected ownership. Improve deficient documentation in touched code rather than copying undocumented patterns.
- Test the active group in focused bursts. Avoid full regressions; at most one justified broad regression per ten groups or at final closure. Split a case exceeding two minutes into focused files such as `...-1`. Wait at least50 seconds when polling running tests/builds and tolerate one minute of agent inactivity. Finish source edits before starting a verification build; do not run heavyweight native file suites alongside compilation.
- Retain supplied saves and existing work. Worktree writes remain inside authorized repositories. The user's2026-10-09 instruction authorizes the current finalization commits in all workspace repositories, with the filenames of plans containing newly completed `[X]` tasks in the message; it is not a standing authorization for later unrelated Git writes. Reset/checkout and unsolicited push/tag remain unauthorized.

## User requirements and decisions

1. **Construction entry:** replace the underground rail placeholder with a recognizable TBM/excavation icon, including the menu entry and relevant toolbar variants.
2. **Working tools and compact UX:** clicking **Excavation projects** must visibly open/select real projects. **Plan an excavation area** must guide the player through a working preview and command. The window must be substantially less bulky, with clear next actions and concise status.
3. **Predictable movement:** investigate the apparently random TBM path. Actual driving must follow the accepted geometry and intended travel orders. Legitimate area-coverage turns must be previewed and understandable; missing targets must produce a visible safe wait.
4. **Retreat and usable track:** keep the occupied/protected-support refusal while work requires it. Provide an obvious return/park operation. Once the entire consist has safely withdrawn and work claims have been released, ordinary trains and track construction must be allowed on the completed route under normal rail rules.
5. **Readable, varied geology:** distinguish soft and hard rock visually and introduce deterministic geological variation where current generation is uniform. Texture must reflect authoritative hardness, not create difficulty through rendering randomness.
6. **Pacing, confirmed by the user:** halve cutting speed, so equivalent cuts take **twice as long**. Halve both Earth and Rock per newly cleared cell. Travel speed is a separate property and is not changed by this request. At uninterrupted steady cutting this means roughly one quarter of the previous spoil per simulation tick; haulage waits, movement and hardness can change observed project throughput.
7. **Useful advice:** stop the recurring false “too few orders” warning for a correctly assigned automatic hauler. Keep real missing/invalid destinations and ordinary-train order warnings actionable.
8. **Vehicle orders and alternate exits:** let the TBM receive understandable travel/return orders, including a chosen compatible hangar. Support Surface↔Underground portal construction from either side. After tunnelling roughly50 squares from entry A, the player must be able to build exit B, park the same actual TBM there, finish the connection and run ordinary trains end to end.

Additional screenshot findings are in scope: three simultaneous “No excavation area selected” rows; “No active excavation project selected” despite a visible project-labelled train; the standard train window says “20 mph - No orders”; the bottom bar displays “(undefined string)”. The screenshot does not prove that the project is invalid or that movement is random. Reproduce the interaction and loaded asset/language state before assigning a cause.

### Parking and orders contract to implement

The current source portal already has a depot-like magazine: code recognizes the whole train stopped in `Track::Depot` on its source tile. This supports the user's hangar mental model, but does not prove every portal can currently act as an interchangeable destination. Current retirement accepts the project's source or a genuine rail depot and requires a finished project, idle exchange, and an empty withdrawn consist.

Expose a **compatible TBM hangar** as a player concept backed by real native storage and explicit capability checks. Support the original source and an eligible alternative portal/depot with actual safe access. Keep the immutable commissioning/job anchor distinct from a chosen return/parking destination. No teleportation, disappearing parts, fabricated depot IDs or blanket clearing of protection is permitted. A parked TBM must retain its native vehicle identity, cargo history and reusable machinery capability.

Separate **Pause**, **Return and park** (preserve resumable work), and **Finish/cancel and release** (release future work claims after withdrawal and haulage settlement). Explain why support cannot yet be converted. Completed common rail must become usable without selling the TBM. Parking must not block the portal's public through-route: establish a real magazine/side-bay occupancy contract, charging and excavating any additional footprint if existing geometry cannot safely provide both functions.

Ordinary travel orders and the work controller need one explicit arbitration rule. A player return/service order safely interrupts cutting and takes precedence after the current atomic operation. The geological job still owns cut geometry/progress; the vehicle/order owner owns travel intent. Returning must never authorize backwards cutting or reset accepted work. Shared orders, scripts, cloning and replacement must not duplicate a work assignment.

### Portal B construction contract to implement

Reuse existing reciprocal portal commands and layer policy. The current matrix and command already support both Surface and Underground as source contexts; the missing end-to-end workflow may involve UI, plain-ground admission, commissioning, parking and project handover rather than a missing pair type.

Portal placement over active temporary support needs an explicit safe handover. Plan a paid exit footprint, move the whole TBM clear into a valid staging position, and release/convert only the vacated necessary pieces while retaining a safe route. Build and commission B without purchasing another TBM or opening free rock. Then direct the original consist to B and release the completed tunnel. If that sequence needs a new bounded handover command, keep its mutation with the existing job/support/portal owners and quote all costs first. Returning all the way to A must not be the only way to construct B. Direct Surface↔Deep rail remains outside this request; preserve its current restriction and Deep's calendar gate.

## Evidence and diagnosis at planning time

The screenshot was supplied in this conversation on2026-10-09; no local image filename is assumed. Code observations below describe the current worktree, not a reproduction of the user's running executable.

| Report | Current evidence | Classification and next proof |
| --- | --- | --- |
| Rail icon | `toolbar_gui.cpp` explicitly selects `SPR_IMG_BUILDRAIL` for underground airport-slot/menu/editor entries. `ControlExcavation` already exists in the native control sprite system. | Confirmed presentation defect; select a legible native TBM icon and verify all entry points. |
| Projects appears inactive | `UndergroundConstructionWindow::OnClick` arms `WID_UC_PROJECT`; `OnPlaceObject` expects a portal click and then opens its window. It is not a project list. | Confirmed mismatch with the label's expected interaction; reproduce selection/feedback in the user's state. |
| Area planning appears inactive | Area mode first requires a portal on the window's layer, then a rectangular drag. It resolves a canonical pair key/default model/origin and calls `PreviewTBMCommissionArea`; confirmation posts `CommissionTBM`. | Existing commissioning flow, not demonstrated assignment to the visible existing TBM. Trace cursor ownership, pair-side selection, defaults, rejection display and active-project handling. |
| Bulky/duplicated window | `_underground_widgets` uses several520-pixel minimum rows and three empty-area status rows; large image/text buttons and informational rows are always present. | Confirmed layout issue; real scaled UI capture is required after redesign. |
| Apparent wandering | `GetTBMCutterSteeringTarget` supplies planned targets to native train/YAPF handling. Work/wait phases rely on their movement hold; service/depot orders can override project steering. | Cause unproven. Compare intended route/heading/phase with actual head and every following part, including junctions and missing-target fallback. |
| Support refusal/retreat | Screenshot says temporary support is still used by a boring project. `ControlTBMProject(Cancel)` initiates a return; `RetireTBMProject` has full withdrawal/empty/exchange checks. Portal controls expose cancel/retire/support conversion. | Protection can be correct; the accessible return/release workflow and alternate hangar are incomplete or unclear. Verify completion, cancellation and pause separately. |
| Uniform-looking rock | Generated deposits use Soft for Underground and Hard for Deep; `GetExcavationCellHardness` uses the same layer defaults elsewhere. `GetMultiLayerTerrainSprite` selects layer/state/edge/texture, not hardness. | Confirmed generation and rendering limitation. Existing texture variants do not communicate material difficulty. |
| Digging/spoil | Native work rates are Standard1/1 and Deep3/2 geological-work ticks per simulation tick. `_cut_terms` currently yields Soft6/12, Mixed4/24, Hard2/40 and Basalt1/48 Earth/Rock per new cell. | Apply confirmed half-rate/half-yield change through authoritative owners and explicit save migration. Basalt's half-tonne Earth needs deterministic fractional accounting. |
| Hauler warnings | `CheckOrders` counts station orders and emits “too few” below two every20 vehicle days. It does not consult the assigned TBM haulage controller; that controller has its own steering target. | Strong code-level explanation for false advice; reproduce an assigned healthy hauler and contrast missing/broken assignments. |
| TBM orders/portal exits | The TBM is already a native Train. Active project steering and ordinary depot orders coexist; retirement recognizes the source or a rail depot. `CmdBuildLayerPortal` accepts a source layer and target layer, but native paired endpoints currently require clear, flat, unoccupied sites. | Extend coherent orders, hangar admission and safe exit handover. Do not replace the real train or duplicate the portal registry. |
| Undefined string | Visible in the screenshot's bottom bar; no originating StringID is established. | Investigate executable/language/GRF mismatch and formatting/cursor/status paths; do not assume one from the image alone. |

## Execution queue

### Local Mandatory Reading - Reproduction and shipped state

Applies to Group1. Read the evidence table and user requirements; `underground_gui.cpp` placement callbacks, `toolbar_gui.cpp` underground routing, `layer_portal_gui.cpp` controls, and the current demo checkpoint's launch/load details. Inspect the actual build/config/content/language owners only as needed to match the running game. Prior unit-test success is not evidence that a live click workflow works.

### Group 1 - Reproduce the reported game and establish a baseline

- [ ] 1.1 Preserve the user's affected save if available and identify the launched executable, build revision, config, language, GRF/content identity, UI scale, layer, game year and speed. Reproduce from retained native demo fixtures while waiting for any missing save; request only the specific missing artifact needed to resolve a remaining discrepancy.
- [ ] 1.2 Reproduce all eight reports through normal GUI input. Record visible prompts, selected portal/project/train, command admission/refusal and whether a click armed placement or opened a window. Capture the bottom-bar undefined string and identify its originating formatting or content mismatch.
- [ ] 1.3 Retain a bounded trace of planned job segments/heading, controller phase and target, native orders/reservations and every consist part during the apparent wandering. Separate planned coverage turns from unintended train routing; record the exact failing or confusing transition.
- [ ] 1.4 Measure normal-speed active cut ticks, per-cut Earth/Rock, wagon fill, exchange waits and a representative50-square hauler round trip. Confirm the existing hardness distribution and rendering mapping without mutating the user's original save.
- [ ] 1.5 Publish reproduction steps and a minimal native fixture for each confirmed defect. Resolve the affected owning plan/architecture contract before changing it; retain unconfirmed causes as open investigation, not completed fixes.

### Local Mandatory Reading - Construction interface and actual commands

Applies to Groups2-3. Read `underground_gui.h/.cpp`, `widgets/underground_widget.h`, underground entry routing in `toolbar_gui.cpp`, `layer_portal_gui.cpp`, `tbm_controls.h/.cpp`, `tbm_area_preview.h`, `tbm_area.cpp`, `tbm_commission.cpp`, `construction_preview.h/.cpp`, and the relevant localized strings. Consult Plan7's entry/control contract and Plan13's existing control/purchase sprite family. Read viewport addressing/cursor ownership only for actual click/drag failures.

### Group 2 - Make the interface compact and discoverable

- [ ] 2.1 Define and implement a compact construction palette plus a real projects list/inspector. Show actual owned projects with TBM, layer, phase, progress and blocking reason; provide select/centre/open-vehicle actions. Keep new-project/area planning distinct from inspecting an existing project.
- [ ] 2.2 Remove the permanent triple empty-area rows and display one contextual instruction. Collapse quote/material/working-space details until needed; use short controls, responsive sizing and scrolling where necessary. At the supplied screen size/scale, the default palette must leave substantially more usable map visible and keep all actionable controls reachable.
- [ ] 2.3 Replace rail placeholder icons with a recognizable TBM/excavation asset in the main toolbar, dropdown, split/editor equivalents and relevant selection state. Reuse the native authored control/purchase family where legible; author/pack a new small icon only if required. Verify normal, pressed, disabled and scaled appearances.
- [ ] 2.4 Make every tool change visibly state its next action and retain/clear selection deliberately. Explain wrong layer, missing compatible portal/TBM, missing origin, locked technology and unavailable resources next to the action. Fix the traced undefined-string issue and verify localized formatting/long labels.
- [ ] 2.5 Verify with real GUI input and captures at the supplied resolution/UI scale plus a small-window and high-scale case. Check cancellation, Escape/right-click, switching layers and extra viewports. Do not close this group from widget constants or sprite manifests alone.

### Group 3 - Make project and area planning work for the actual machine

- [ ] 3.1 Trace and fix project selection, portal-side resolution, placement ownership and drag/release events. Clicking Projects must open the real list; choosing a project must select its actual controller/vehicle and inspect its current geometry.
- [ ] 3.2 Implement explicit flows for a new portal-commissioned project and assigning/extending work for an eligible existing TBM. Reuse current command/geometry owners and native IDs; never silently purchase a second machine to make an existing-machine workflow appear successful.
- [ ] 3.3 Provide an understandable tunnel/corridor versus area-clear intent, using canonical route/outline planning. Preview actual cutter path, covered floor, charged turn/approach envelope, uncovered/refused cells, time and spoil before confirmation. A straight tunnel request must not silently become unexplained area sweeps.
- [ ] 3.4 Revalidate the entire quote at command execution and return the exact refusal/actionable next step. Preserve atomic cash, reservations, RNG, stock and existing completed cuts on stale/invalid quotes. New work must retain the real machine, usable retreat and correct source/job association.
- [ ] 3.5 Prove portal selection→area drag→preview→confirm→actual movement/cutting through GUI input in both rock layers, including existing-machine continuation, wrong-layer/occupied/locked/insufficient-capacity refusal and reload of an accepted plan.

### Local Mandatory Reading - Planned geometry and native movement

Applies to Group4. Read the Group1 trace, `GetTBMCutterSteeringTarget` and paid support/path helpers in `tbm_support.cpp`, relevant `train_cmd.cpp` steering/hold transitions, `pathfinder/yapf/yapf_destrail.hpp`, `tbm_project.cpp`, `tbm_work.cpp`, and executing portions of `excavation_route.cpp`/`excavation_pass.cpp`. Inspect existing movement, corridor, coverage and turn tests; read the rail pathfinder architecture only for the exact native fallback involved.

### Group 4 - Make TBM travel match the accepted route

- [ ] 4.1 Fix the traced divergence at its owner: planner geometry, target/trackdir selection, phase hold, reservation, backing or order arbitration. If the existing path is intentional, make its purpose and turn envelope explicit and correct any mismatch between user intent and accepted plan.
- [ ] 4.2 Missing/stale/incompatible work targets must hold with a clear recoverable reason. Prevent unintended ordinary no-order/junction fallback while the work controller owns travel; preserve legitimate depot/service travel and all non-TBM trains.
- [ ] 4.3 Prove actual head and every permanent/wagon part follow the accepted support through straight travel, two45-degree bends, backing, reused multi-branch track and constrained U-turn space. No stationary pivot, sharp-turn shortcut, backwards cutting or movement through solid rock may satisfy the test.
- [ ] 4.4 Save/reload during approach, a bend and a wait. Compare route cursor, heading, part positions/reservations and next real cut with uninterrupted execution; repeated previews/rendering must not alter the result.

### Local Mandatory Reading - Cutting pace and spoil conservation

Applies to Group5. Read `tbm_model.cpp`, `AdvanceTBMWork` preparation/commit in `tbm_work.cpp`, `_cut_terms`/`GetExcavationCellHardness`/quote and cut operations in `excavation_cut.cpp`, their actual receipt/partial-work fields in `excavation.h` and save owners. Consult [frozen excavation terms](r2mlvs1-e-ground-and-space-budgets.md#group-13---excavation-units-bounded-spoil-and-legal-heading-budgets), but reconcile historical examples with the actual measured runtime baseline; do not retune unrelated purchase/upkeep values from stale tables.

### Group 5 - Halve cutting speed and Earth/Rock output

- [ ] 5.1 Apply the confirmed half work rate to both native cutters: Standard1/1→1/2 and Deep3/2→3/4, subject to the Group1 baseline. Preserve travel speed, geometry, active/idle accounting ownership and exactly once work per synchronized tick.
- [ ] 5.2 Halve Earth and Rock for every newly cleared cell, including extra turn/approach/support-envelope cells. Keep the two cargo types separate; open overlap produces none. Do not halve unrelated mineral mining, disposal prices or support material/cash requirements.
- [ ] 5.3 Introduce canonical saved fractional spoil accounting where needed: Basalt Earth1→0.5 cannot round down independently on every cell or round back up to1. Quotes and atomic cuts must share the same exact rule; define retained carry and cancellation/retirement settlement without inventing wagon capacity or silently losing fractions.
- [ ] 5.4 Version and migrate active old saves explicitly. Preserve completed terrain/cargo/use, partial geological work and work-rate fractions (e.g. an old Deep half-work remainder must remain a half). Apply slower future progress and new-yield rules through accepted owner state, revalidating future estimates without replaying past cuts or permanently grandfathering the reported fast behavior.
- [ ] 5.5 Verify twice the simulated active-cut time within integer-tick rounding and half each spoil type over representative complete cell sets, including odd Basalt counts, mixed hardness, interval subdivision, retry, full one-cargo wagons and real mid-cut/carry save-load. Exclude unchanged travel/waits from the rate comparison. Repeat the measured50-square haulage scenario and report actual time/fill/wait/upkeep changes rather than promising the hauler can keep up at any distance or silently compensating by changing costs.

### Local Mandatory Reading - Assigned hauler advice

Applies to Group6. Read `CheckOrders` in `order_cmd.cpp`, actual hauler assignment/phase/steering queries in `tbm_exchange.h/.cpp`, their loss/release hooks, and train advice/status formatting in `vehicle_gui.cpp`/`train_cmd.cpp`. Consult existing hauler cycle/logistics stall tests. Do not disable the global order-review setting.

### Group 6 - Remove false order warnings and show actual work status

- [ ] 6.1 Reproduce the warning over multiple20-day review periods with a healthy assigned hauler; distinguish managed route legs from its ordinary station-order count.
- [ ] 6.2 Make advice consult authoritative live assignment and route health. Suppress only the inapplicable “too few orders” advice for a valid managed cycle, while retaining missing origin/bay/destination, lost assignment, wrong owner and ordinary train warnings. Resume ordinary review immediately after release.
- [ ] 6.3 Show meaningful current TBM/hauler work or travel status instead of misleading generic “No orders” while a real controller is driving. Keep speed, stopped/paused state and actionable holds accurate in vehicle/project views.
- [ ] 6.4 Verify repeated healthy cycles produce no advice spam and every genuine failure still has a useful visible reason. Test release/reassignment, shared orders, reload and ordinary trains with too few orders.

### Local Mandatory Reading - Safe parking, orders and support release

Applies to Groups7-8. Read this plan's parking/orders contract; `IsTBMConsistWithdrawn`, `RetireTBMProject`, cancel/phase transitions in `tbm_project.cpp`; `tbm_controls.cpp`, `layer_portal_work.cpp`, corridor/support ownership in `tbm_support.cpp`, and actual exchange/cargo settlement in `tbm_exchange.cpp`. For orders, inspect `order_type.h`, `order_cmd.cpp`, `order_gui.cpp`, vehicle command/purchase protections, YAPF destination selection and feature save/network serialization. Preserve Plan3's sealed commissioning and Plan4/5's actual wagon/retreat contracts.

### Group 7 - Return to the existing hangar and release finished track

- [ ] 7.1 Expose clear Return and park / Finish or cancel and release controls from the TBM vehicle and project views, with the actual destination and prerequisites visible. Distinguish pause/resumable work from releasing a completed/cancelled project.
- [ ] 7.2 Drive the whole actual consist into its valid original portal magazine or compatible existing depot. Settle loaded spoil through real haulage; retain the vehicle and packets throughout blocked return, occupied hangar and missing empty-wagon cases.
- [ ] 7.3 Release only safe, vacated work/support reservations after the full consist and associated logistics have cleared. Offer actual charged support conversion/removal where necessary. A hidden/stopped engine alone must not free an occupied tail or erase another project's claims.
- [ ] 7.4 Prove an ordinary freight train traverses the completed route and that normal track building, junction work and legal support conversion succeed after release. Before safe release, preserve the guard and explain the specific blocking train/project and next action.
- [ ] 7.5 Reload during return, partly inside the portal and after parking/release; retain all parts, cargo and eligibility. Show the parked TBM can be selected and reused without selling/rebuying it.

### Group 8 - Add coherent vehicle orders and a selected hangar destination

- [ ] 8.1 Freeze the public hangar capability and destination identity with existing portal/depot owners. Separate commissioning anchor, work job, haulage origin and selected return destination; define actual capacity/occupancy and simultaneous public portal use. Charge/excavate a real additional bay if required by safe geometry.
- [ ] 8.2 Implement vehicle-facing travel/return/park orders and clear controller arbitration. A return order safely suspends new cutting, completes or preserves the current atomic operation, then follows legal excavated track. Keep initial portal-only machine purchase and immutable cutter/support layout protections.
- [ ] 8.3 Support choosing another compatible owned hangar and resuming/reassigning work from the parked machine. Preserve actual native IDs and cargo; no casting TileIndex to DepotID, private duplicate order queue, teleportation or controller reset may fake arrival.
- [ ] 8.4 Serialize and synchronize new destination/intent state through the owning command/order/controller contract with explicit version handling. Cover shared orders, script calls, sale/clone/autoreplace restrictions, destination demolition, takeover and bankruptcy without orphaned claims or duplicate assignments.
- [ ] 8.5 Drive and reload actual Go to hangar A/B, service, park and resume cases, including full/incompatible/foreign/removed destinations and a blocked route. A valid parking operation must leave ordinary through traffic usable under the agreed occupancy contract.

### Local Mandatory Reading - Building the far exit

Applies to Group9. Read this plan's Portal B contract; `MultiLayerMap::IsPortalPairAllowed`/`GetPortalDestination`, `CmdBuildLayerPortal`/`PrepareNativePortalBuild`, rail toolbar destination/placement controls, commissioning/work-pair state, the Group8 destination owner, and actual support handover/occupancy checks. Consult Plan3's source/far-side and date contracts; modify relevant parent decisions only for the requested extension.

### Group 9 - Construct portals from either side and finish an end-to-end tunnel

- [ ] 9.1 Reproduce both Surface→Underground and Underground→Surface placement through the normal toolbar; expose destination, ownership, orientation, full footprint, approach, price and refusal on both ends. Reuse the existing pair matrix/command rather than creating a parallel portal type.
- [ ] 9.2 Implement the safe exit-footprint handover sequence described above. Allow constructing B from already excavated Underground floor without requiring retirement back at A, while preventing demolition beneath any consist part or loss of the only retreat route.
- [ ] 9.3 Commission an already excavated connection using its real facilities/approaches and the existing TBM where appropriate. No second cutter purchase, free cavity, bypassed Deep gate or automatic unsealing of unrelated unfinished portals is allowed.
- [ ] 9.4 Execute the user's full scenario: start at A, tunnel approximately50 squares, build B from Underground, order the original TBM into B, settle remaining spoil, finish/release the tunnel, then drive ordinary freight through A↔B in both directions and build a legal track addition.
- [ ] 9.5 Verify GUI/command/script parity, failed estimates/blocked footprints/funds refusal, wrong ownership and full save/reload during B construction, commissioning and withdrawal. Confirm every vehicle, paid cell, packet and portal pairing remains conserved.

### Local Mandatory Reading - Geological variation and matching terrain art

Applies to Group10. Read `GeologyHardness`, `geology_generation.cpp`, `GetExcavationCellHardness`, geology capture/validation/digest/save code, and `GetMultiLayerTerrainSprite` in `multilayer_sprite.cpp`. Read the relevant authored terrain/edge families and immutable asset manifest/packing/loading sections in Plan13; use the established pipeline. Do not make mineral identity, hardness, excavation state or visible texture substitute for each other.

### Group 10 - Make soft and hard rock varied and recognizable

- [ ] 10.1 Define meaningful deterministic soft/mixed/hard regions for supported rock layers, with appropriate Deep/Basalt restrictions and explicit proportions/region scale. The geological seed/state owns difficulty; rendering consumes it. Preserve Surface RNG and bound generation/query work.
- [ ] 10.2 Version the new geological representation and old-save policy. Preserve existing deposits, reserves, excavation, accepted work and measured cut difficulty on legacy maps unless a deliberate compatible migration is proven. New-map variation must be demonstrated; existing-map visual improvements must show its actual saved hardness rather than silently reroll it.
- [ ] 10.3 Author or select clearly distinguishable soft/hard terrain and face/edge treatments through the existing asset system. Match actual geological hardness in rendering, survey text and work estimates, including partially excavating cells, zoom/scaling and both rock layers. Keep ordinary mineral overlays readable.
- [ ] 10.4 Prove same-seed/reload/multiplayer determinism, different-region distribution, correct cut time/yield, preserved existing state and zero renderer-driven changes. Test that a Standard TBM can reach and recover from an unsupported hard/Basalt boundary without being stranded.
- [ ] 10.5 Capture the user's existing map and representative new maps at the screenshot's view scale. Verify players can visually distinguish relevant hardness and confirm it by survey; include final packed runtime art, not only source images or a changed texture hash.

### Local Mandatory Reading - Integrated player acceptance and documentation

Applies to Group11. Read the preceding groups' retained evidence, this plan's completion conditions and the precise affected public contracts in Plans1-5/7/13/14. Reuse actual captured saves and GUI flows. Do not preload or execute unrelated paused master/geothermal groups.

### Group 11 - Verify the complete player workflow and update the owning contracts

- [ ] 11.1 Run a complete ordinary-game journey using the redesigned UI: find/select the TBM, preview and execute work, observe understandable movement and slower cuts, complete several hauler cycles without false advice, return/park, release and build/run ordinary rail.
- [ ] 11.2 Repeat the A→50-square tunnel→B build→park at B→public traffic scenario, including actual saved intermediate states. Verify a normal depot/service visit and subsequent reuse of the same machine do not lose its work/cargo identity.
- [ ] 11.3 Complete focused Debug/optimized and native save/preview/determinism checks for changed owners. Run one justified final affected integration regression, not repeated full suites. Retain malformed and legacy cases for any new persistent fields/fractions/orders/geology.
- [ ] 11.4 Update affected architecture/public handoffs, frozen pacing/yield examples, screenshots, player instructions and completion claims in the owning plans. Explain how to return, release support, choose a hangar and construct the far exit; reconcile old claims that described narrower behavior.
- [ ] 11.5 Publish a reviewable playable checkpoint and compare it with the supplied screenshot. Record user feedback on usability, route clarity, visible geology and pacing. Leave unresolved feedback/required acceptance visibly pending; do not mark this whole plan complete from tests alone or silently resume R2MLVS3MP.

## Completion conditions

All eight numbered user requirements, the additional visible formatting/status defects and the actual A-to-B tunnel workflow must have direct evidence. A green geometry test does not prove a working button, a parked icon does not prove the whole consist withdrew, and a saved target does not prove physical arrival. Every released segment must become genuinely usable while occupied/protected segments remain safe. Actual old/new saves must preserve vehicles, cargo, work, fractions, portal ownership and deterministic behavior. New geology/art must be readable in the real game and obey the same hardness used by cutting.

The implementation record must identify the tested executable/content/config and link focused logs, native saves and GUI captures, with explicit remaining limitations. No implementation checkbox is marked complete merely by this planning pass. The broader master remains paused until the user explicitly resumes it.
