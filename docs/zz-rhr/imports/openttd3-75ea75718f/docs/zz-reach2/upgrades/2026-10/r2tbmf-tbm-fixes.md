> **Historical donor snapshot — not an execution plan.** OpenTTD3 `75ea75718f`, imported2026-10-23.
> Original: `docs/zz-reach2/upgrades/2026-10/r2tbmf-tbm-fixes.md`; exact source SHA256 `e61e4181bec9be2976683d16d39be47ff398cb1c68e4c3f942125d53ff445566`.
> Current integration: [M2O23-1](../../../../../../../zz-reach2/upgrades/2026-10/m2o23-1-come-space.md); storage and evidence roles: [archi-historical-debug.md](../../../../../../../zz-reach2/architecture/archi-historical-debug.md). Original completion claims, instructions and human-review limits below remain dated; they grant no new execution, spending, publication or deletion authority.

# R2TBMF - TBM excavation fixes and usable tunnel workflows

Status: active execution,2026-10-09. The user clarified that queued messages had been staged by a harness error and explicitly selected **resume TBM fixes, then commit when complete**. The [master](r2mlvs3mp-master-plan.md) is paused again. Verified reproduction work is recorded in Group1A and the [checkpoint](../../../../../../../../../OpenTTD3/docs/zz-reach2/upgrades/2026-10/tbm-fixes/r2tbmf-b-reproduction-checkpoint.md); Groups1B and2 and the Group3 geological producers are verified; Group3B command and existing-machine UI integration is verified; Group3C intent/path presentation and both-layer input/reload acceptance are verified; Group4 movement safety and actual moving-file replay are verified. Group5 cutting pace and work-fraction migration are verified; Group5B whole-unit spoil tuning is verified; Group6 assigned-hauler advice/status is verified; Group7 resumable return/park is verified; Group7B final release/public track/reuse is verified; Group8 selected hangars/orders/reassignment is verified; Group9 far-exit construction, native input and fifty-cell public traffic are verified; Group10 geological variation, matching native art, survey and network acceptance are verified; Group11 final integration is current. The final commit must include every plan filename with newly completed `[X]` tasks, including any verified geothermal work preserved from the temporary queue diversion.

Owners: [Plan2 geology/excavation](r2mlvs2-geology-and-excavation.md), [Plan3 portals](r2mlvs3-sealed-portals-and-deep-rail.md), [Plan4 TBMs](r2mlvs4-tunnel-boring-machines.md), [Plan5 haulage](r2mlvs5-spoil-wagons-and-haulage.md), [Plan7 construction/fleet](r2mlvs7-underground-construction-and-fleet.md) and [Plan13 graphics](../../../../../../../../../OpenTTD3/docs/zz-reach2/upgrades/2026-10/r2mlvs13-graphics-and-asset-pipeline.md). Architecture: [map layers](../../architecture/map-layers/archi-map-layers.md), [vehicles](../../../../../../../../../OpenTTD3/docs/zz-reach2/architecture/vehicles/archi-vehicles.md), [rail pathfinding](../../../../../../../zz-reach2/architecture/pathfinder/archi-pathfinder-rail.md), [GUI](../../architecture/gui/archi-gui.md), [savegames](../../architecture/savegame/archi-savegame.md). Parent decisions and invariants remain in [R2MLVS](r2mlvs-multilayer-vehicles-and-structures.md); user-requested behavior changes below must be reconciled there during implementation.

## Global Mandatory Reading

Before starting implementation and after every context compaction, read this section, **User requirements and decisions**, the current group's nearest **Local Mandatory Reading**, and that group's tasks. Read only the named symbols/sections needed for the group. Reading is non-transitive: do not reload whole parent plans, unrelated geothermal/orbital work or historical test logs.

| Source | Required scope | Reason |
| --- | --- | --- |
| This plan | User requirements, evidence table, active group and completion conditions | Preserve every reported problem and distinguish observed behavior from an unverified diagnosis. |
| R2MLVS parent | AD-3..AD-6; I-2..I-8, I-12..I-15 | Actual vehicles, paid excavation, explicit layer addressing, safe retreat, atomic stock/cash and deterministic work. Read affected decisions again only when their behavior changes. |
| Plan2 / Plan3 / Plan4 / Plan5 | Public Handoff Contract or exported owner symbols relevant to the active group | One owner for geometry, commissioning, vehicle work, support, orders and wagon/cargo custody. |
| Plan15 | Existing feature save/preview and lifecycle contract only when introducing persistent fields or changing old-save behavior | Real files must preserve active work, routes, work remainders and actual vehicles. |

Execution rules:

- Execute groups top to bottom. Mark a task `[X]` only after its stated behavior and focused evidence pass; planning, a declaration or a numerical fixture does not complete player-facing behavior.
- Each group has at most eight tasks. After compaction record its count above the active group. Beyond three compactions, split that group into B/C/D groups with narrower Local Mandatory Reading rather than repeatedly loading unrelated context.
- Source/test filenames must not begin with `r2`. Temporary assisting Markdown/scratch follows the user's chronological prefix rule beside this plan; reusable runners live under `src/tests/multilayer/`. Logs use `logs/<feature>/<scenario>/<run-id>/`. New saves/reports/screenshots use `mlm/reviews/<feature>/<scenario>/<run-id>/`, with captures in feature-specific `images/` directories. Keep immutable inputs in purpose-named `src/tests/fixtures/` folders, reproducible proof in `logs/proofs/mlm/native-art/`, and native configuration/data under `mlm/runtime/`. The parent artifact-layout contract and R2FC own these paths.
- All created/modified source and test files need an architecture/owning-plan header. Every new/modified function documents its purpose, parameters and return value in JavaDoc style; classes/interfaces have JavaDoc headers. Explain compound conditions and nontrivial loops in terms of gameplay and affected ownership. Improve deficient documentation in touched code rather than copying undocumented patterns.
- Batch the active group's related implementation and test changes before compiling. The user explicitly requested larger steps, fewer builds and more related scenarios per verification cycle on2026-10-09. Run those scenarios together against the completed build; rebuild only for a completed source batch or an actual failure needing correction. Avoid full regressions; at most one justified broad regression per ten groups or at final closure. Split a case exceeding two minutes into focused files such as `...-1`. Wait at least50 seconds when polling running tests/builds and tolerate one minute of agent inactivity. After the reported out-of-memory incident, use one MSBuild project at a time with at most two compiler workers (`-j 1 -- /p:CL_MPCount=2 /nodeReuse:false`); do not overlap heavyweight native file suites with compilation.
- Retain supplied saves and existing work. Worktree writes remain inside authorized repositories. The earlier follow-up requested commits after fixes with every newly completed plan filename, but the latest supplied workspace instructions prohibit write Git commands, including commit. Preserve the requested message scope for a later explicitly authorized commit; perform no Git writes under the current instructions. The preceding geothermal finalization commit is already complete; no fixing-plan completion commit is due yet. Reset/checkout and unsolicited push/tag remain unauthorized.

**Artifact-layout migration (2026-10-09):** this plan consumes the parent [Development artifact layout](r2mlvs-multilayer-vehicles-and-structures.md#development-artifact-layout), implemented by [R2FC](r2fc-file-cleanup.md). It supersedes earlier docs asset/review/helper placement and fixed runtime-config destinations below. New native homes use `mlm/runtime/`, generated feature reviews use `mlm/reviews/`, logs remain under `logs/`, and reusable code/curated inputs keep their code/test owners. Paths and focused native/cold verification are complete; R2FC records the results and retained-input identities. Repository-relative executable commands run from the checkout root through their context-aware wrappers. Historical pass records remain dated evidence and are not new acceptance claims. No gameplay task is completed by this migration and the separate post-demo fixture-pruning gate remains.

## User requirements and decisions

1. **Construction entry:** replace the underground rail placeholder with a recognizable TBM/excavation icon, including the menu entry and relevant toolbar variants.
2. **Working tools and compact UX:** clicking **Excavation projects** must visibly open/select real projects. **Plan an excavation area** must guide the player through a working preview and command. The window must be substantially less bulky, with clear next actions and concise status.
3. **Predictable movement:** investigate the apparently random TBM path. Actual driving must follow the accepted geometry and intended travel orders. Legitimate area-coverage turns must be previewed and understandable; missing targets must produce a visible safe wait.
4. **Retreat and usable track:** keep the occupied/protected-support refusal while work requires it. Provide an obvious return/park operation. Once the entire consist has safely withdrawn and work claims have been released, ordinary trains and track construction must be allowed on the completed route under normal rail rules.
5. **Readable, varied geology:** distinguish soft and hard rock visually and introduce deterministic geological variation where current generation is uniform. Texture must reflect authoritative hardness, not create difficulty through rendering randomness.
6. **Pacing, confirmed by the user:** halve cutting speed, so equivalent cuts take **twice as long**. Halve both Earth and Rock per newly cleared cell, with the user-confirmed whole-unit exception below. Travel speed is a separate property and is not changed by this request. At uninterrupted steady cutting this means roughly one quarter of the previous spoil per simulation tick; haulage waits, movement and hardness can change observed project throughput.
**Whole-unit decision,2026-10-09:** the user rejected fractional cargo as unnecessary expansion of the native game and selected **keep1 tonne Earth for Basalt**. New per-cell Earth/Rock yields are Soft3/6, Mixed2/12, Hard1/20 and Basalt1/24. Retain ordinary integer cargo packets, capacities, loading, disposal and saved fields. No half-unit transport, production carry or fractional spoil save extension is required. Batch edits and use one focused verification burst per group; repeat checks only for actual failures or changed behavior.

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
| Digging/spoil | Native work rates are Standard1/1 and Deep3/2 geological-work ticks per simulation tick. `_cut_terms` currently yields Soft6/12, Mixed4/24, Hard2/40 and Basalt1/48 Earth/Rock per new cell. | Apply confirmed half-rate/half-yield change through authoritative owners and explicit save migration. The later user decision retains Basalt Earth at1 tonne; use the existing whole-unit cargo system. |
| Hauler warnings | `CheckOrders` counts station orders and emits “too few” below two every20 vehicle days. It does not consult the assigned TBM haulage controller; that controller has its own steering target. | Strong code-level explanation for false advice; reproduce an assigned healthy hauler and contrast missing/broken assignments. |
| TBM orders/portal exits | The TBM is already a native Train. Active project steering and ordinary depot orders coexist; retirement recognizes the source or a rail depot. `CmdBuildLayerPortal` accepts a source layer and target layer, but native paired endpoints currently require clear, flat, unoccupied sites. | Extend coherent orders, hangar admission and safe exit handover. Do not replace the real train or duplicate the portal registry. |
| Undefined string | Visible in the screenshot's bottom bar; no originating StringID is established. | Investigate executable/language/GRF mismatch and formatting/cursor/status paths; do not assume one from the image alone. |

## Execution queue

### Local Mandatory Reading - Reproduction and shipped state

Applies to Group1A. Read the evidence table and user requirements; `underground_gui.cpp` placement callbacks, `toolbar_gui.cpp` underground routing, `layer_portal_gui.cpp` controls, and the current demo checkpoint's launch/load details. Prior unit-test success is not evidence that a live click workflow works.

### Group 1A - Preserve and reproduce the confirmed baseline

Split from original Group1 on2026-10-09 so completed evidence is visible without claiming that every investigation or any gameplay fix has finished. Original broader requirements continue in Group1B.

- [X] 1A.1 Identify the user-confirmed demo from `d2um-underground-mines.md`, preserve its exact bytes, and record known launcher/config/content/language provenance and measured reproduction scale/date. Explicitly retain the unknown original screenshot executable hash rather than inventing it.
- [X] 1A.2 Reproduce Projects/Area clicks through ordinary native mouse hit testing and window timers. Retain actual cursor ownership, prompts and1280x733 captures; establish the momentary-button/placement-handler cancellation defect and bulky repeated empty rows.
- [X] 1A.3 Trace the bottom-bar undefined string to the retained company's zero default name and empty custom name; distinguish this saved fixture problem from unproven language mismatch.
- [X] 1A.4 Retain a bounded20,000-frame normal simulation trace of accepted route/heading, controller targets/phases, orders, reservations and every actual part's position/cargo. Observe the route completing20 cuts and four physical exchanges.
- [X] 1A.5 Measure the existing Soft active-cut ticks and separate Earth/Rock yields, distinguish seven zero-material overlap segments, and confirm the actual layer hardness histogram and renderer selection policy without modifying the original save.
- [X] 1A.6 Publish the native reproduction runner, retained minimal demo reference, exact findings and limitations in the checkpoint. Preserve startup script and original save/config hashes after the run.

Evidence: [reproduction checkpoint](../../../../../../../../../OpenTTD3/docs/zz-reach2/upgrades/2026-10/tbm-fixes/r2tbmf-b-reproduction-checkpoint.md), [native run] (historical reference: `../../../../logs/r2tbmf-a-native-feedback.log`), and actual captures under `mlm/reviews/<feature>/<scenario>/<run-id>/images/tbm-feedback-*.png`. These are diagnosis/baseline completions, not implemented UI, movement, rate or parking fixes.

### Local Mandatory Reading - Remaining movement, advice and haul-distance characterization

Applies to Group1B. Read the reproduction checkpoint's findings and remaining-work paragraphs, the retained native observer `src/tests/tbm_feedback_runtime.cpp`, actual `GetTBMCutterSteeringTarget`/hold logic, native `CheckOrders`/news references, existing portal Cancel/Retire/Connect controls and actual construction command charging/clearing limits. Reuse Group1A's confirmed input and snapshots. Do not repeat its complete trace or reread unrelated geothermal work merely to establish the same facts again.

### Group 1B - Finish the remaining actionable baseline

- [X] 1B.1 Compare the retained actual movement against the accepted support/coverage path and identify the exact confusing or failing transitions. Keep missing-target/junction divergence unconfirmed where the trace does not prove it.
- [X] 1B.2 Capture the live vehicle-labelled recurring advice, current TBM travel/order status, occupied-support refusal and existing return/retirement controls through native GUI/runtime. Record the absent return-and-park/alternate-hangar workflow explicitly; do not mistake cancellation or selling for the requested behavior.
- [X] 1B.3 Construct and retain a separate paid50-square haul fixture at a legal paced construction rate. Measure its actual fill, round-trip/exchange waits and cutting time before changing rates; preserve the original demo, station/receiver identity and cargo custody.
- [X] 1B.4 Update reproduction steps and affected ownership contracts for the remaining confirmed defects. All eight reports must have either reproducible evidence or an explicitly bounded unconfirmed finding before implementation; retain original-save/build uncertainties without blocking independent fixes unnecessarily.

Group1B closure,2026-10-09: [native evidence and measured long-haul handoff](../../../../../../../../../OpenTTD3/docs/zz-reach2/upgrades/2026-10/tbm-fixes/r2tbmf-b-reproduction-checkpoint.md#group1b-closure-and-implementation-handoff) records complete observed routing, live vehicle-labelled advice, actual protected-floor input refusal, current controls and the paid50-tile round trips. No actual fix is inferred from this diagnosis. **Group2 is current.**

### Local Mandatory Reading - Construction interface and actual commands

Applies to Groups2-3. Read `underground_gui.h/.cpp`, `widgets/underground_widget.h`, underground entry routing in `toolbar_gui.cpp`, `layer_portal_gui.cpp`, `tbm_controls.h/.cpp`, `tbm_area_preview.h`, `tbm_area.cpp`, `tbm_commission.cpp`, `construction_preview.h` and its implementation in `viewport.cpp`, and the relevant localized strings. Consult Plan7's entry/control contract and Plan13's existing control/purchase sprite family. Read viewport addressing/cursor ownership only for actual click/drag failures. There is no `construction_preview.cpp`; do not repeatedly search for that stale filename.

Group2 closed after **1** compaction. Preserve its baseline captures and original demo; its final evidence is in the interface checkpoint below.

### Group 2 - Make the interface compact and discoverable

- [X] 2.1 Define and implement a compact construction palette plus a real projects list/inspector. Show actual owned projects with TBM, layer, phase, progress and blocking reason; provide select/centre/open-vehicle actions. Keep new-project/area planning distinct from inspecting an existing project.
- [X] 2.2 Remove the permanent triple empty-area rows and display one contextual instruction. Collapse quote/material/working-space details until needed; use short controls, responsive sizing and scrolling where necessary. At the supplied screen size/scale, the default palette must leave substantially more usable map visible and keep all actionable controls reachable.
- [X] 2.3 Replace rail placeholder icons with a recognizable TBM/excavation asset in the main toolbar, dropdown, split/editor equivalents and relevant selection state. Reuse the native authored control/purchase family where legible; author/pack a new small icon only if required. Verify normal, pressed, disabled and scaled appearances.
- [X] 2.4 Make every tool change visibly state its next action and retain/clear selection deliberately. Explain wrong layer, missing compatible portal/TBM, missing origin, locked technology and unavailable resources next to the action. Fix the traced undefined-string issue and verify localized formatting/long labels.
- [X] 2.5 Verify with real GUI input and captures at the supplied resolution/UI scale plus a small-window and high-scale case. Check cancellation, Escape/right-click, switching layers and extra viewports. Do not close this group from widget constants or sprite manifests alone.

2.1-2.2 evidence,2026-10-09: [focused UI] (historical reference: `../../../../logs/r2tbmf-c-ui-focused.log`) passes736 assertions/11 cases; [browser ownership] (historical reference: `../../../../logs/r2tbmf-c-browser-focused.log`) passes38/one. [Native input] (historical reference: `../../../../logs/r2tbmf-c-native-ui.log`) and [real repaired-demo reload] (historical reference: `../../../../logs/r2tbmf-c-native-ui-reload.log`) verify actual list/vehicle/footprint selection, persistent company naming, Escape, extra-viewport layer cancellation and right-click window closure. The [default palette](../../../../../../../../../OpenTTD3/mlm/reviews/tbm/planning/import-20261009/images/tbm-ui-palette.png) shrinks786x569 to396x232 at1280x733/150%; expanded guidance fits264x200 at1024x600/100% and528x400 at1280x733/200%. [Project rows](../../../../../../../../../OpenTTD3/mlm/reviews/tbm/planning/import-20261009/images/tbm-ui-projects-click.png) wrap progress/holds and use native resize/scroll controls. Toolbar variants and final Group2 documentation remain in progress; existing-machine continuation, movement, pacing and parking remain unchecked later groups.

Group2 closure,2026-10-09: [interface checkpoint](../../../../../../../../../OpenTTD3/docs/zz-reach2/upgrades/2026-10/tbm-fixes/r2tbmf-c-interface-checkpoint.md) records all five tasks,1,048 focused assertions/15 cases,208 native-input/reload assertions, actual scaled/toolbar captures and the repaired playable demo. The original390 save/configuration remain unchanged.

### Local Mandatory Reading - Existing-machine and intent-aware planning

Applies to Group3. Read Group2's checkpoint, `tbm_area_preview.h`, `tbm_area.cpp`, `tbm_commission.cpp`, actual reservation/continuation exports in `excavation.h` and `excavation_jobs.cpp`, `tbm_project.cpp`/`tbm_support.cpp` admission and retreat owners, `tbm_gui.h`, and relevant placement callbacks in `underground_gui.cpp`. Revisit parent AD-3..AD-6 and I-2..I-8/I-12..I-15 only for changed continuation/retreat behavior. Reuse verified native input instead of repeating Group2 presentation or unrelated geothermal reading. No `construction_preview.cpp` exists; its owner remains `viewport.cpp`.

Group3 producer slice closed after **0** compactions. On2026-10-09 the original group was split at the discovered producer/command/intent boundaries. Original3.1 is below; original3.2/3.4 continue in3B, and original3.3/3.5 continue in3C. No player-facing acceptance requirement was removed.

### Group 3 - Make project and area planning work for the actual machine

- [X] 3.1 Trace and fix project selection, portal-side resolution and placement ownership. Projects opens the real list; selecting its row inspects the actual controller/vehicle and accepted geometry. Retain Group2 native-input evidence and the existing main/extra-view area command checks; full new/existing-machine acceptance continues in3C.
- [X] 3.2 Publish a pure continuation planner for an actual owned unfinished job. Replay its full accepted cutter/consist history, forecast only additional work beyond that endpoint, retain old live claims and progress, and refuse foreign/cancelled/malformed requests. Repeated previews must not alter vehicles, terrain, funds, stock or RNG.
- [X] 3.3 Publish atomic geological append admission using the existing job ID and revision. Preserve the original route prefix, cursor, partial work, receipts and all old exchange points; validate added geometry/support/capacity/claims and preallocate before publishing. Prove unchanged refusal, stale retry rejection and compatibility with existing claim reconstruction. This producer does not itself authorize a TBM command or prove physical travel.

Evidence: [continuation notes](../../../../../../../../../OpenTTD3/docs/zz-reach2/upgrades/2026-10/tbm-fixes/r2tbmf-d-replanning-notes.md), [optimized producer build] (historical reference: `../../../../logs/r2tbmf-d-append-build.log`) and [595 focused assertions/seven cases] (historical reference: `../../../../logs/r2tbmf-d-append-focused.log`). No new saved fields, job/vehicle identities, cargo store or private route owner were introduced.

### Local Mandatory Reading - Existing-machine command and UI integration

Applies to Group3B. Read [continuation notes D](../../../../../../../../../OpenTTD3/docs/zz-reach2/upgrades/2026-10/tbm-fixes/r2tbmf-d-replanning-notes.md), [current command/UI checkpoint E](../../../../../../../../../OpenTTD3/docs/zz-reach2/upgrades/2026-10/tbm-fixes/r2tbmf-e-command-ui-checkpoint.md), verified `PlanExcavationJobExtension` and `AppendExcavationJob`, `tbm_replanning.h`, actual `FindTBMProject`/extension/control admission in `tbm_project.cpp`, `tbm_cmd.h` and command registration, shared outline/confirmation in `tbm_area.cpp`/`tbm_commission.cpp`, `tbm_gui.h`/`tbm_projects_gui.cpp`, and area/quote placement callbacks in `underground_gui.cpp`. Existing retired-source reuse is already owned by `IsTBMConsistReusableAtPortal` and acquisition; preserve it. There is no `tbm_cmd.cpp`; commands live with their feature owners. Read only relevant synchronization/save validation when exposing the new command.

Group3B compaction count: **1**.

### Group 3B - Expose new-project and existing-machine planning through real commands

- [X] 3B.1 Implement explicit flows for a new portal-commissioned project and assigning/extending work for an eligible existing TBM. Reuse current command/geometry owners and native IDs; never silently purchase a second machine to make an existing-machine workflow appear successful.
- [X] 3B.2 Add the synchronized controller command and complete quote revalidation. Validate the actual paused/eligible consist, model/date, job, origin, capacity, support/turn geometry and current ownership before the geological append; publish the controller revision only after all preparation succeeds.
- [X] 3B.3 Present the real machine and exact additional price/material/work quote, including reuse of an eligible retired source machine. Make selection, pause/continue, cancellation and stale/invalid quote refusals actionable. Existing completed cuts and resumable partial work must remain intact.
- [X] 3B.4 Prove GUI/command parity and unchanged cash, reservations, RNG, stock and geometry on stale/invalid requests. Retain the actual machine, usable retreat and source/job association. Repeated confirmation cannot append twice or buy another cutter.

Group3B closure,2026-10-09: [checkpoint E](../../../../../../../../../OpenTTD3/docs/zz-reach2/upgrades/2026-10/tbm-fixes/r2tbmf-e-command-ui-checkpoint.md) records39,223 assertions/14 focused cases and143 assertions in the native40,000-tick demo journey. Actual mouse input pauses, quotes, cancels, redraws, confirms and resumes the same cutter; all33 cuts finish and additional receipts match90Earth/180Rock. Group3C still owns explicit intent/path presentation, both-layer driven acceptance and the extended save's real reload; pacing and the later fixes remain pending.

### Local Mandatory Reading - Explicit work intent and complete planning acceptance

Applies to Group3C. Read the preceding command/quote handoff and [intent checkpoint F](../../../../../../../../../OpenTTD3/docs/zz-reach2/upgrades/2026-10/tbm-fixes/r2tbmf-f-intent-checkpoint.md), canonical `excavation_route.cpp`/`excavation_outline.cpp` and physical turn/support checks, the construction preview owner in `viewport.cpp`, current `underground_gui.cpp` and the native input/loader observer. Reuse the verified palette and real project-selection flow. Read only affected save/native workflow fixtures for accepted extended jobs in both rock layers.

Current3C.3 handoff: [fresh/Deep checkpoint G](../../../../../../../../../OpenTTD3/docs/zz-reach2/upgrades/2026-10/tbm-fixes/r2tbmf-g-fresh-checkpoint.md), read after F while this acceptance task is active.

Group3C compaction count: **0**.

### Group 3C - Explain tunnel versus area intent and prove the whole planning workflow

- [X] 3C.1 Provide an understandable tunnel/corridor versus area-clear intent using canonical route/outline planning. Preview the actual cutter path, covered floor, charged turn/approach envelope, uncovered/refused cells, time and spoil before confirmation. A straight tunnel request must not silently become unexplained area sweeps.
- [X] 3C.2 Revalidate the complete intent-specific quote at command execution and preserve all producer/command refusal guarantees. Keep path/outline rendering read-only and saved geometry deterministic; intent must not bypass paid clearance, rail-turn or retreat rules.
- [X] 3C.3 Prove portal selection -> floor/path input -> preview -> confirm -> actual movement/cutting through GUI input in both rock layers, including existing-machine continuation, wrong-layer/occupied/locked/insufficient-capacity refusal and real file reload of an accepted plan. Unit producer tests alone cannot close these player-facing tasks.

Group3C progress,2026-10-09: [checkpoint F](../../../../../../../../../OpenTTD3/docs/zz-reach2/upgrades/2026-10/tbm-fixes/r2tbmf-f-intent-checkpoint.md) records21,125 focused assertions, actual numbered straight/area previews, both completed Underground extension journeys, and exact real-file reload continuations. Task3C.3 is now closed by [checkpoint G](../../../../../../../../../OpenTTD3/docs/zz-reach2/upgrades/2026-10/tbm-fixes/r2tbmf-g-fresh-checkpoint.md): both fresh intents in both rock layers, actual complete movement/return, exact accepted-file replays and focused refusal parity. No later movement, pacing, advice, parking or geology task is implied complete.

### Local Mandatory Reading - Planned geometry and native movement

Applies to Group4. Read the Group1 trace, `GetTBMCutterSteeringTarget` and paid support/path helpers in `tbm_support.cpp`, relevant `train_cmd.cpp` steering/hold transitions, `pathfinder/yapf/yapf_destrail.hpp`, `tbm_project.cpp`, `tbm_work.cpp`, and executing portions of `excavation_route.cpp`/`excavation_pass.cpp`. Inspect existing movement, corridor, coverage and turn tests; read the rail pathfinder architecture only for the exact native fallback involved.

Movement closure: [notes H](../../../../../../../../../OpenTTD3/docs/zz-reach2/upgrades/2026-10/tbm-fixes/r2tbmf-h-motion-notes.md); actual pause/target/junction failures, depot service, constrained-turn motion and moving-file reload evidence are verified.

Group4 compaction count: **1**.

### Group 4 - Make TBM travel match the accepted route

- [X] 4.1 Fix the traced divergence at its owner: planner geometry, target/trackdir selection, phase hold, reservation, backing or order arbitration. If the existing path is intentional, make its purpose and turn envelope explicit and correct any mismatch between user intent and accepted plan.
- [X] 4.2 Missing/stale/incompatible work targets must hold with a clear recoverable reason. Prevent unintended ordinary no-order/junction fallback while the work controller owns travel; preserve legitimate depot/service travel and all non-TBM trains.
- [X] 4.3 Prove actual head and every permanent/wagon part follow the accepted support through straight travel, two45-degree bends, backing, reused multi-branch track and constrained U-turn space. No stationary pivot, sharp-turn shortcut, backwards cutting or movement through solid rock may satisfy the test.
- [X] 4.4 Save/reload during approach, a bend and a wait. Compare route cursor, heading, part positions/reservations and next real cut with uninterrupted execution; repeated previews/rendering must not alter the result.

Group4 closure,2026-10-09: [notes H](../../../../../../../../../OpenTTD3/docs/zz-reach2/upgrades/2026-10/tbm-fixes/r2tbmf-h-motion-notes.md#group4-closure) records599,484 assertions/18 focused movement cases,259,548 assertions/7 constrained-turn/coverage cases, and actual saved approach/bend/wait continuations matching19,696/19,527/17,169 native frames exactly. Loaded service/stop/pause variants retain partial work and packets. The native area journey audits every actual cutter/wagon against paid accepted support, including reused branches. Repeated quotes and native rendering leave simulation state unchanged. No pacing, advice, parking or exitB completion is implied.

### Local Mandatory Reading - Cutting pace and spoil conservation

Applies to Group5. Read `tbm_model.cpp`, `AdvanceTBMWork` preparation/commit in `tbm_work.cpp`, `_cut_terms`/`GetExcavationCellHardness`/quote and cut operations in `excavation_cut.cpp`, their actual receipt/partial-work fields in `excavation.h` and save owners. Consult [frozen excavation terms](../../../../../../../../../OpenTTD3/docs/zz-reach2/upgrades/2026-10/r2mlvs1-e-ground-and-space-budgets.md#group-13---excavation-units-bounded-spoil-and-legal-heading-budgets), but reconcile historical examples with the actual measured runtime baseline; do not retune unrelated purchase/upkeep values from stale tables.

Group5 split on2026-10-09: cutting speed and spoil tuning have separate focused acceptance. The later whole-unit decision removes the proposed spoil-fraction migration. Group5 preserves original5.1 plus the rate portions of5.4/5.5. Group5B preserves original5.2/5.3 and the yield/haulage portions of5.4/5.5. No requirement is dropped or marked complete by this split.

Group5 compaction count: **0**.

Current rate/migration handoff: [notes J](../../../../../../../../../OpenTTD3/docs/zz-reach2/upgrades/2026-10/tbm-fixes/r2tbmf-j-pacing-notes.md). The old393 partial-work seeds are retained and must not be regenerated after tuning changes.

### Group 5 - Halve cutting speed and preserve partial work

- [X] 5.1 Apply the confirmed half work rate to both native cutters: Standard1/1→1/2 and Deep3/2→3/4, subject to the Group1 baseline. Preserve travel speed, geometry, active/idle accounting ownership and exactly once work per synchronized tick.
- [X] 5.2 Version and migrate active old work-rate fractions explicitly. Preserve completed terrain/cargo/use and partial geological work; an old Deep half-work remainder must remain a half. Apply slower future progress without replaying past cuts or permanently grandfathering the reported fast behavior. Refuse malformed old/new fractions in preview and full load.
- [X] 5.3 Verify twice the simulated active-cut time within integer-tick rounding for both models/hardness, interval subdivision, retry and real mid-cut old/new save-load. Exclude unchanged travel/waits and retain current running-cost ownership. The combined yield/50-square haulage measurement remains5B.4.

Group5 closure,2026-10-09: [notes J](../../../../../../../../../OpenTTD3/docs/zz-reach2/upgrades/2026-10/tbm-fixes/r2tbmf-j-pacing-notes.md#group5-closure) records142,906 assertions/19 focused pacing cases,360,807 assertions/2 affected physical service/U-turn checks,162,105 assertions covering both models/all hardnesses with fractional pauses, and16 actual malformed preview/load refusals. Actual393→394 partial-cut files preserve work/cargo/terrain; next cuts take exactly twice the old remaining time. Native saves containing newly earned Standard1/2 and Deep1/4 work reload and reach the same next cut exactly. Group5B is current; half spoil remains pending.

### Local Mandatory Reading - Whole-unit spoil and combined haulage balance

Applies to Group5B. Read the whole-unit decision above, [notes L](../../../../../../../../../OpenTTD3/docs/zz-reach2/upgrades/2026-10/tbm-fixes/r2tbmf-l-spoil-notes.md), `_cut_terms` and shared quote/prepare/estimate operations in `excavation_cut.cpp`. Inspect existing packet/capacity admission in `tbm_work.cpp` and old saved receipt/partial-work preservation only as needed. Reuse Group1B's retained50-square baseline and Group5's rate/migration evidence; do not repeat old movement or fractional-cargo investigations. No cargo schema change is required.

Group5B compaction count: **1**.

### Group 5B - Reduce spoil using native whole cargo units

Scope corrected by the user on2026-10-09: replace the proposed fractional-cargo work with a simple yield-table change. Basalt retains1 tonne Earth; its Rock output halves. No removed fractional feature is counted as delivered work.

- [X] 5B.1 Set new-cell Earth/Rock yields to Soft3/6, Mixed2/12, Hard1/20 and Basalt1/24 through the shared authoritative table. Include extra turn/approach/support cells; open overlap still produces none. Keep mining reserves, support charges and disposal prices unchanged.
- [X] 5B.2 Remove the unneeded fractional-cargo additions and retain existing native packets, capacities, loading, disposal and serialization. Verify the revised output through actual cuts and existing one-cargo-full/refusal handling, including Basalt's whole-unit exception.
- [X] 5B.3 Preserve completed old terrain, cargo, receipts and partial work. Apply the new table to future quotes/cuts without replaying completed work; retain safe accepted return boundaries and Group5's work-rate migration. Verify a representative existing-save continuation without inventing a cargo-format migration.
- [X] 5B.4 Repeat the retained50-square haulage scenario with slower cutting and reduced whole-unit yields. Report actual fill/wait/time/upkeep changes. Keep checks focused on this balance change and reuse earlier movement evidence.

5B.1-5B.2 evidence,2026-10-09: both optimized executables build; `[tbm-pacing],[tbm-work]` passes242,416 assertions/six cases in1.6 seconds ([focused log] (historical reference: `../../../../logs/r2tbmf-l-whole-spoil-focused.log`)). Actual first-face cuts cover both native models/all supported hardnesses with pause/resume, ordinary whole-packet counts, independent full-wagon waits and allocation refusal. The shared table changes output only; transport, receiver, job and cargo save schemas retain their original whole units. Fractional-only sources/tests and integrations are removed. Existing-save continuation and measured50-square balance remain5B.3/5B.4.

Group5B closure,2026-10-09: [notes L](../../../../../../../../../OpenTTD3/docs/zz-reach2/upgrades/2026-10/tbm-fixes/r2tbmf-l-spoil-notes.md#group5b-closure) and [native runner M](../../../../../../../../src/tests/fixtures/cleanup/source-epochs/openttd3-75ea75718f/a3eeb1042833d714-whole_spoil.py) retain two real393→394 continuations, with all saved owner bytes unchanged apart from Group5's equivalent work remainder. The retained50-square sample reaches20/20 cuts versus18/20 previously and waits7,622 versus11,931 ticks for empties. Current train upkeep is88,439 internal Money over20,000 ticks; the original baseline did not record a comparable upkeep ledger, so no numeric upkeep improvement is claimed. Prices were unchanged. The sample ends while final spoil still travels; track release remains Group7. Group6 is current.

### Local Mandatory Reading - Assigned hauler advice

Applies to Group6. Read `CheckOrders` in `order_cmd.cpp`, actual hauler assignment/phase/steering queries in `tbm_exchange.h/.cpp`, their loss/release hooks, and train advice/status formatting in `vehicle_gui.cpp`/`train_cmd.cpp`. Consult existing hauler cycle/logistics stall tests. Do not disable the global order-review setting.

Current owner findings: [notes N](../../../../../../../../../OpenTTD3/docs/zz-reach2/upgrades/2026-10/tbm-fixes/r2tbmf-n-advice-notes.md). Group6 compaction count: **0**.

### Group 6 - Remove false order warnings and show actual work status

- [X] 6.1 Reproduce the warning over multiple20-day review periods with a healthy assigned hauler; distinguish managed route legs from its ordinary station-order count.
- [X] 6.2 Make advice consult authoritative live assignment and route health. Suppress only the inapplicable “too few orders” advice for a valid managed cycle, while retaining missing origin/bay/destination, lost assignment, wrong owner and ordinary train warnings. Resume ordinary review immediately after release.
- [X] 6.3 Show meaningful current TBM/hauler work or travel status instead of misleading generic “No orders” while a real controller is driving. Keep speed, stopped/paused state and actionable holds accurate in vehicle/project views.
- [X] 6.4 Verify repeated healthy cycles produce no advice spam and every genuine failure still has a useful visible reason. Test release/reassignment, shared orders, reload and ordinary trains with too few orders.

6.1 evidence reuses the current M native20,000-tick continuations: each records14 too-few-orders entries for assigned hauler vehicle0 and14 for cutter5 while real work/haul cycles advance. `CheckOrders` ignores both controllers and counts ordinary station orders only; the hauler's empty-return destination belongs to its live assignment. No separate baseline rerun was needed. Advice/status implementation remains6.2-6.4.

Group6 closure,2026-10-09: [notes N](../../../../../../../../../OpenTTD3/docs/zz-reach2/upgrades/2026-10/tbm-fixes/r2tbmf-n-advice-notes.md#group6-closure) records418 assertions/three focused cases and38 assertions in the real loaded20,000-tick loop. False order warnings fall14→0 for each actual participant. Existing invalid/duplicate-order advice, missing destination/receiver/owner checks, shared ordinary peers, release and reassignment remain effective. [Working caption](../../../../../../../../../OpenTTD3/mlm/reviews/tbm/work-advice/import-20261009/images/tbm-advice-running.png) shows0mph boring with the hauler's real station destination; [empty return](../../../../../../../../../OpenTTD3/mlm/reviews/tbm/work-advice/import-20261009/images/tbm-advice-empty-return.png) shows6mph bringing empty hoppers and the cutter's concise empty-hopper wait. No saved state or transport behavior changed. Group7 is current.

### Local Mandatory Reading - Safe parking, orders and support release

Applies to Groups7-8. Read this plan's parking/orders contract; `IsTBMConsistWithdrawn`, `RetireTBMProject`, cancel/phase transitions in `tbm_project.cpp`; `tbm_controls.cpp`, `layer_portal_work.cpp`, corridor/support ownership in `tbm_support.cpp`, and actual exchange/cargo settlement in `tbm_exchange.cpp`. For orders, inspect `order_type.h`, `order_cmd.cpp`, `order_gui.cpp`, vehicle command/purchase protections, YAPF destination selection and feature save/network serialization. Preserve Plan3's sealed commissioning and Plan4/5's actual wagon/retreat contracts.

Group7 split on2026-10-09 after confirming separate resumable-parking and final-release boundaries. Original7.1 is split between7.1/7B.1, original7.2 remains7.2, original7.3/7.4 become7B.2/7B.3, and original7.5 is split between7.3/7B.4. No behavior is dropped or counted complete by this split. [Notes P](../../../../../../../../../OpenTTD3/docs/zz-reach2/upgrades/2026-10/tbm-fixes/r2tbmf-p-parking-notes.md) records the current physical owners.

Group7 compaction count: **1**.

### Group 7 - Return and park the actual consist while retaining resumable work

- [X] 7.1 Expose Return and park from the TBM vehicle and project views, with the actual original hangar/exchange destination and prerequisites visible. Keep Pause and Resume distinct; returning must preserve unfinished geometry and partial work.
- [X] 7.2 Drive the whole actual consist into its valid original portal magazine or compatible existing exchange depot. Settle loaded spoil through real haulage; retain every vehicle and packet throughout blocked return, occupied hangar and missing empty-wagon cases. Park only after the real complete consist is withdrawn and empty.
- [X] 7.3 Save/reload during return, partly inside the portal and parked. Preserve work, cargo, all parts and the ability to resume the same machine's accepted work. No selling, rebuying, teleportation or replacement work owner may stand in for physical parking.

Group7 closure,2026-10-09: [notes P](../../../../../../../../../OpenTTD3/docs/zz-reach2/upgrades/2026-10/tbm-fixes/r2tbmf-p-parking-notes.md#group7-closure) records actual empty/loaded source returns and paid underground-depot parking/resume, plus blocked train, lost destination, missing empties and partly occupied hauler arrival. Real project-window input requests parking; vehicle Start resumes the retained cut. Three actual395 files reproduce7,216/7,090/892 complete native frames exactly through the next cut. [Destination/action capture](../../../../../../../../../OpenTTD3/mlm/reviews/tbm/parking/import-20261009/images/tbm-parking-request.png) and [parked status](../../../../../../../../../OpenTTD3/mlm/reviews/tbm/parking/import-20261009/images/tbm-parking-parked.png) use native widgets. Parked work remains resumable and protected; final release/public track is Group7B, which is now current.

### Local Mandatory Reading - Final work release and public use of vacated track

Applies to Group7B. Read this plan's parking contract and the delivered Group7 handoff, `RetireTBMProject`/cancel transitions, `WithdrawLayerPortalWorkPair`, `ChangeLayerPortalConnection`, `IsLayerPortalMagazineBusy`, `IsTBMSupportInUse`, `CmdConvertTBMSupport`, real exchange-idle/release checks and reusable-source commissioning admission. A stored full consist is distinct from a train moving across the housing, but demolition must still protect it. Reuse earlier cargo and movement evidence; test only affected release/through-traffic behavior.

### Group 7B - Release finished work and make the completed route usable

Group7B compaction count: **1**. Current owner/implementation handoff: [notes R](../../../../../../../../../OpenTTD3/docs/zz-reach2/upgrades/2026-10/tbm-fixes/r2tbmf-r-release-notes.md).

- [X] 7B.1 Expose Finish/cancel and release controls with actual settlement prerequisites, distinct from resumable parking. Explain the blocking train/project and next action while support remains in use.
- [X] 7B.2 Release only safe, vacated work/support claims after the complete consist and associated logistics have cleared. Keep actual charged support conversion/removal; a hidden head cannot free an occupied tail or another project's claims.
- [X] 7B.3 Prove ordinary freight traverses the completed route and normal track building, junction work and support conversion succeed after release. Establish that a fully parked TBM does not block the portal's public through-route under the native magazine/side-bay contract; charge/excavate an additional bay only if existing storage cannot safely provide both functions.
- [X] 7B.4 Save/reload after release and public connection. Preserve the actual parked machine for selection and reuse without selling/rebuying; retain demolition/ownership guards and ordinary through traffic.

7B.3 evidence,2026-10-09: [notes R](../../../../../../../../../OpenTTD3/docs/zz-reach2/upgrades/2026-10/tbm-fixes/r2tbmf-r-release-notes.md#public-track-proof) and [focused log] (historical reference: `../../../../logs/r2tbmf-r-release-focused.log`) record66,213 assertions/three cases in0.83 seconds. A completed paid chamber's support is converted and ordinary connecting/junction track funded; all five parts of a released freight train travel through the original source to a real underground depot and back while every retained cutter part stays stored unchanged. Connection estimates remain pure and demolition still refuses the stored machine. Final release controls/unfinished-job settlement and actual post-release file replay remain7B.1/7B.2/7B.4.

Group7B closure,2026-10-09: [notes R](../../../../../../../../../OpenTTD3/docs/zz-reach2/upgrades/2026-10/tbm-fixes/r2tbmf-r-release-notes.md#group7b-closure) records actual final-release input, deferred cancellation with unchanged partial work/claims until physical settlement, paid support conversion and public connection beside the retained machine. Real pending396 cancellation and older395 parking files continue correctly. Released/Connected396 files retain cutter5, its six parts and all11 vehicles; paid common-track preparation and reuse purchase0 lead to another actual exchange after12,709/12,707 native frames. The portal now accepts a legal curved approach; unrelated track and occupied tails still refuse. Group8 is current.

### Local Mandatory Reading - Selected hangar orders

Applies to Group8. Read the delivered Group7/7B physical parking and release handoffs, this plan's parking/orders contract, actual order encoding/commands/UI, YAPF destinations, vehicle sale/clone/replacement restrictions and feature save/network serialization. Extend actual destination ownership without duplicating commissioning anchor, job progress or wagon custody.

### Group 8 - Add coherent vehicle orders and a selected hangar destination

Group8 closed after **2** compactions. Native-order/storage and first file proofs: [notes T](../../../../../../../../../OpenTTD3/docs/zz-reach2/upgrades/2026-10/tbm-fixes/r2tbmf-t-hangar-notes.md). Verified input/order handoff: [notes V](../../../../../../../../../OpenTTD3/docs/zz-reach2/upgrades/2026-10/tbm-fixes/r2tbmf-v-hangar-input-notes.md). Full closure: [notes W](../../../../../../../../../OpenTTD3/docs/zz-reach2/upgrades/2026-10/tbm-fixes/r2tbmf-w-order-control-notes.md#group8-closure). Group9 is current.

- [X] 8.1 Freeze the public hangar capability and destination identity with existing portal/depot owners. Separate commissioning anchor, work job, haulage origin and selected return destination; define actual capacity/occupancy and simultaneous public portal use. Charge/excavate a real additional bay if required by safe geometry.
- [X] 8.2 Implement vehicle-facing travel/return/park orders and clear controller arbitration. A return order safely suspends new cutting, completes or preserves the current atomic operation, then follows legal excavated track. Keep initial portal-only machine purchase and immutable cutter/support layout protections.
- [X] 8.3 Support choosing another compatible owned hangar and resuming/reassigning work from the parked machine. Preserve actual native IDs and cargo; no casting TileIndex to DepotID, private duplicate order queue, teleportation or controller reset may fake arrival.
- [X] 8.4 Serialize and synchronize new destination/intent state through the owning command/order/controller contract with explicit version handling. Cover shared orders, script calls, sale/clone/autoreplace restrictions, destination demolition, takeover and bankruptcy without orphaned claims or duplicate assignments.
- [X] 8.5 Drive and reload actual Go to hangar A/B, service, park and resume cases, including full/incompatible/foreign/removed destinations and a blocked route. A valid parking operation must leave ordinary through traffic usable under the agreed occupancy contract.

Group8 closure,2026-10-09: T/V/W retain actual native menus, both A/B directions, service, full loaded returns, ordinary station orders and missing-order waits. The combined X batch covers native takeover/bankruptcy, machinery guards, shared distinct cutters and adverse destinations. The remaining remote reuse case exposed an open-centre/solid-edge starter gap; its correction charges the whole remaining pocket through the existing first-cut owner. The failed case plus four affected starter cases pass37,519 assertions. The same cutter5/depot3 releases job1, carries public traffic beside its storage and starts job2 at B with purchase0. Its accepted398 file passes full load and nonmutating preview, then makes the first paid cut after967 real frames. Seven malformed397/398 preview/load probes refuse safely. Owning contracts are reconciled; Group9's active-tunnel exit construction remains pending.

### Local Mandatory Reading - Building the far exit

Applies to Group9. Read this plan's Portal B contract; `MultiLayerMap::IsPortalPairAllowed`/`GetPortalDestination`, `CmdBuildLayerPortal`/`PrepareNativePortalBuild`, rail toolbar destination/placement controls, commissioning/work-pair state, the Group8 destination owner, and actual support handover/occupancy checks. Consult Plan3's source/far-side and date contracts; modify relevant parent decisions only for the requested extension.

### Group 9 - Construct portals from either side and finish an end-to-end tunnel

Group9 closed after **1** compaction. Implementation, actual input/files and public-traffic evidence: [notes Y](../../../../../../../../../OpenTTD3/docs/zz-reach2/upgrades/2026-10/tbm-fixes/r2tbmf-y-exit-notes.md#group9-closure).

- [X] 9.1 Reproduce both Surface→Underground and Underground→Surface placement through the normal toolbar; expose destination, ownership, orientation, full footprint, approach, price and refusal on both ends. Reuse the existing pair matrix/command rather than creating a parallel portal type.
- [X] 9.2 Implement the safe exit-footprint handover sequence described above. Allow constructing B from already excavated Underground floor without requiring retirement back at A, while preventing demolition beneath any consist part or loss of the only retreat route.
- [X] 9.3 Commission an already excavated connection using its real facilities/approaches and the existing TBM where appropriate. No second cutter purchase, free cavity, bypassed Deep gate or automatic unsealing of unrelated unfinished portals is allowed.
- [X] 9.4 Execute the user's full scenario: start at A, tunnel approximately50 squares, build B from Underground, order the original TBM into B, settle remaining spoil, finish/release the tunnel, then drive ordinary freight through A↔B in both directions and build a legal track addition.
- [X] 9.5 Verify GUI/command/script parity, failed estimates/blocked footprints/funds refusal, wrong ownership and full save/reload during B construction, commissioning and withdrawal. Confirm every vehicle, paid cell, packet and portal pairing remains conserved.

Group9 closure,2026-10-09: [Y](../../../../../../../../../OpenTTD3/docs/zz-reach2/upgrades/2026-10/tbm-fixes/r2tbmf-y-exit-notes.md#group9-closure) verifies both native toolbar placement directions, a paid locally staged terminal handover, same-cutter parking at B and complete50-cell A-to-B public traffic in both directions with a paid junction. Both command/script journeys finish after60,239 native frames; the final failed-only journey passes114,328 assertions. Actual built/commissioned/withdrawing/moving/parked files reload and preserve machinery/work; the final public file reloads with both rail pairs usable. Corrected quote/refusal captures and genuine funds/owner/occupied/geometry refusals pass. No second TBM, fractional cargo or new persistent fields were introduced. Group10 is current; the master remains paused.

### Local Mandatory Reading - Geological variation and matching terrain art

Applies to Group10. Read `GeologyHardness`, `geology_generation.cpp`, `GetExcavationCellHardness`, geology capture/validation/digest/save code, and `GetMultiLayerTerrainSprite` in `multilayer_sprite.cpp`. Read the relevant authored terrain/edge families and immutable asset manifest/packing/loading sections in Plan13; use the established pipeline. Do not make mineral identity, hardness, excavation state or visible texture substitute for each other.

Group10 closed after **1** compaction. [Notes Z](../../../../../../../../../OpenTTD3/docs/zz-reach2/upgrades/2026-10/tbm-fixes/r2tbmf-z-geology-notes.md#group10-closure) retain the combined optimized game/test build, regional/legacy saves, material/survey art, generated-boundary retreat and actual two-process network continuation. Group11 is current.

### Group 10 - Make soft and hard rock varied and recognizable

- [X] 10.1 Define meaningful deterministic soft/mixed/hard regions for supported rock layers, with appropriate Deep/Basalt restrictions and explicit proportions/region scale. The geological seed/state owns difficulty; rendering consumes it. Preserve Surface RNG and bound generation/query work.
- [X] 10.2 Version the new geological representation and old-save policy. Preserve existing deposits, reserves, excavation, accepted work and measured cut difficulty on legacy maps unless a deliberate compatible migration is proven. New-map variation must be demonstrated; existing-map visual improvements must show its actual saved hardness rather than silently reroll it.
- [X] 10.3 Author or select clearly distinguishable soft/hard terrain and face/edge treatments through the existing asset system. Match actual geological hardness in rendering, survey text and work estimates, including partially excavating cells, zoom/scaling and both rock layers. Keep ordinary mineral overlays readable.
- [X] 10.4 Prove same-seed/reload/multiplayer determinism, different-region distribution, correct cut time/yield, preserved existing state and zero renderer-driven changes. Test that a Standard TBM can reach and recover from an unsupported hard/Basalt boundary without being stranded.
- [X] 10.5 Capture the user's existing map and representative new maps at the screenshot's view scale. Verify players can visually distinguish relevant hardness and confirm it by survey; include final packed runtime art, not only source images or a changed texture hash.

### Local Mandatory Reading - Integrated player acceptance and documentation

Applies to Group11. Read the preceding groups' retained evidence, this plan's completion conditions and the precise affected public contracts in Plans1-5/7/13/15. Reuse actual captured saves and GUI flows. Do not preload or execute unrelated paused master/geothermal groups.

### Group 11 - Verify the complete player workflow and update the owning contracts

Group11 compaction count: **1**. Final integration source/evidence handoff: [notes ZB](../../../../../../../../../OpenTTD3/docs/zz-reach2/upgrades/2026-10/tbm-fixes/r2tbmf-zb-final-notes.md). Optimized/Debug focused checks and the connected native-input/save/public-traffic journeys passed. All100 saved chunks match at11 corresponding checkpoints. Tasks11.1-11.4 are verified; only the published checkpoint's required player feedback remains pending in11.5. No build or test is running.

- [X] 11.1 Run a complete ordinary-game journey using the redesigned UI: find/select the TBM, preview and execute work, observe understandable movement and slower cuts, complete several hauler cycles without false advice, return/park, release and build/run ordinary rail.
- [X] 11.2 Repeat the A→50-square tunnel→B build→park at B→public traffic scenario, including actual saved intermediate states. Verify a normal depot/service visit and subsequent reuse of the same machine do not lose its work/cargo identity.
- [X] 11.3 Complete focused Debug/optimized and native save/preview/determinism checks for changed owners. Run one justified final affected integration regression, not repeated full suites. Retain malformed and legacy cases for any new persistent fields/fractions/orders/geology.
- [X] 11.4 Update affected architecture/public handoffs, frozen pacing/yield examples, screenshots, player instructions and completion claims in the owning plans. Explain how to return, release support, choose a hangar and construct the far exit; reconcile old claims that described narrower behavior.
- [ ] 11.5 Publish a reviewable playable checkpoint and compare it with the supplied screenshot. Record user feedback on usability, route clarity, visible geology and pacing. Leave unresolved feedback/required acceptance visibly pending; do not mark this whole plan complete from tests alone or silently resume R2MLVS3MP.
  - **Note, 2026-10-16.** The user reviewed the `tbm-work` checkpoint on 2026-10-15 and recorded feedback in [DemoNotes.txt](../../../../../../../Demo2026-06-13/DemoNotes.txt). [R2DF](r2df-demo-fixes-interface-movement-and-planning.md) and [R2DF2](r2df2-face-exchange-and-demo-acceptance.md) captured and addressed it. The refreshed checkpoint (save version 425) and the [review checklist](r2df2-b-acceptance-notes.md#review-checklist-task-116) are ready. This task stays unchecked until the user reviews the refreshed demo, and R2MLVS3MP stays paused.

## Completion conditions

All eight numbered user requirements, the additional visible formatting/status defects and the actual A-to-B tunnel workflow must have direct evidence. A green geometry test does not prove a working button, a parked icon does not prove the whole consist withdrew, and a saved target does not prove physical arrival. Every released segment must become genuinely usable while occupied/protected segments remain safe. Actual old/new saves must preserve vehicles, cargo, work remainders, portal ownership and deterministic behavior. New geology/art must be readable in the real game and obey the same hardness used by cutting.

The implementation record must identify the tested executable/content/config and link focused logs, native saves and GUI captures, with explicit remaining limitations. No implementation checkbox is marked complete merely by this planning pass. The broader master remains paused until the user explicitly resumes it.
