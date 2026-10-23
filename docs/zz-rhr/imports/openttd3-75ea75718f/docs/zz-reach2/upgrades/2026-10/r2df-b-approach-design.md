> **Historical donor snapshot — not an execution plan.** OpenTTD3 `75ea75718f`, imported2026-10-23.
> Original: `docs/zz-reach2/upgrades/2026-10/r2df-b-approach-design.md`; exact source SHA256 `1c4727f052425decb3ed0fac75e00aced44bc905dadda9d0a802d6e8c88e6ff3`.
> Current integration: [M2O23-1](../../../../../../../zz-reach2/upgrades/2026-10/m2o23-1-come-space.md); storage and evidence roles: [archi-historical-debug.md](../../../../../../../zz-reach2/architecture/archi-historical-debug.md). Original completion claims, instructions and human-review limits below remain dated; they grant no new execution, spending, publication or deletion authority.

# R2DF-B - Approach planning design, 2026-10-16

Companion of [R2DF](r2df-demo-fixes-interface-movement-and-planning.md) Groups 8-9 (task 8.6, extended by Group 9), with the change-of-work-area contract of Groups 11-12 (task 12.7). It records how `PlanExcavationApproach` works, how the TBM quotes use it, where both depart from the plan's sketches and why, and the measured search effort that shows the default limits suffice.

## What the producer returns

`PlanExcavationApproach(start, target, model_id, limits, continuing_job, solid_start, exact_goal)` in `src/excavation_route.cpp`, declared in `src/excavation.h` beside `ExcavationRouteProposal`.

- **Input.** `target.requested_tiles` is the floor. `target.authorized_tiles` is what the caller already accepts: floor, ring, housing 3x3 and earlier work. That set may be **in pieces**: a floor away from its housing is not connected to it until the approach joins them.
- **Output.** `ExcavationApproachProposal`:
  - `extra_tiles`: the canonical cells, outside the existing authorization, that a legal route needs;
  - `route`: the route that proves those cells suffice;
  - `end`: the pose after that route;
  - `expansions`: the search effort spent;
  - `error` and `failure_tile`: the refusal and its tile.
- **Empty result.** When the existing authorization already lets the planner cover the floor, the proposal is empty: no route, no cells, `end` = start. The quote then stays exactly what it was before approaches existed.

## Deviation from the sketch: area approaches include the turning space

The sketch aimed the approach at the floor cell nearest the start and stopped there. That cannot fix screenshot 05.

- **Why the sketch fails.** In the demo's request, (13,11)-(20,11) beside the door of (13,10), the floor starts in front of the door. The straight exit cut already touches the nearest cell, so the sketch would return an empty approach. What the floor really lacks is turning space: a 3x3 cutter cannot turn 90 degrees inside a one-cell ring. Group 1 recorded the planner's refusal there as `OutsideAuthorization`, and the test reproduces it.
- **Area mode.**
  1. Plan the coverage inside the existing authorization. If it covers everything, nothing is needed.
  2. Otherwise plan the coverage again inside the box: the start's trail and the floor, grown by the margin and clipped to the map.
  3. Every swept cell of that route outside the existing authorization is an extra cell: approach and turning space together.
  - Both attempts reuse `PlanExcavationRouteInternal`, and so `SearchExcavationTarget`. That gives the same starter, cleared-set and support rules, with no second search.
- **Exact mode (tunnels).** One exact-goal search from the start to the requested cell nearest the start, through the internal planner's tunnel branch.

## Goal rule

- **Which cell.** The goal is the requested cell nearest the start. Area mode measures "near" by `PlanningDistance`, where the body covering the cell is enough. Exact mode measures it by `DistanceMax`, where the centre must stand on the cell.
- **Ties.** Equally near cells resolve to the lowest `TileIndex`, so every peer picks the same one.
- **What the goal is used for.** In area mode only for refusals: "too far to plan" is shown at the goal. Coverage itself keeps the planner's canonical target order.

## Margin

`APPROACH_MARGIN` is 4 cells, the plan's value. It fits the tightest U-turn: four 45-degree bends on consecutive advances move the cutter centre 3 cells sideways, plus 1 for its body.

A box larger than one job's area (`MAX_EXCAVATION_JOB_TILES`, 4096 cells) refuses with `Limit` at the goal ("too far to plan; draw a closer area"). That also bounds an angled tunnel: a diagonal end more than about 55 cells away refuses as too far.

## Leaving a housing: straight on, open or not (Group 9)

Support rail leaves a portal housing only through its door, straight on: `GetTBMWorkRailPath` has no path for a diagonal first advance. The planner already forced that for an unopened (solid) housing.

- **What the native run found.** For a housing that a released project had already opened, the planner could turn on the first cut. The demo's re-commission at (13,10) then proposed a diagonal first cut, which no support can carry.
- **Rule.** A start pose whose whole consist trail still stands on its own tile is in its housing. Its first advance is straight on, opened or not. Starts that have left their tile (an accepted route's end) are not affected.

## No 90-degree support rail (R8's second finding, settled in Group 9)

Group 1 found that, under `forbid_90_deg` (on by default in this fork, and on in the demo), door-anchored floors were refused with "needs sharp rail bends".

- **Cause.** The planner's advances turn at most 45 degrees each, but the support rail is laid as straight pieces and corner zigzags. For every diagonal advance, `GetTBMWorkRailPath` passes through one of two side cells. Its fixed choice ("turn first", "straight first" after the housing) could form a 90-degree bend for some bend sequences, depending on the zigzag phase.
- **Group 8 tried a planner rule, removed in Group 9.**
  - Group 8 added `spaced_bends`: no two same-way bends in a row. It was sufficient, but far stricter than the rail.
  - On the demo the strip beside the door became a 23-cut loop with 67 extra cells that even crossed the housing.
  - The rule, its 7-cell margin and its plumbing are gone.
- **Fix: the support path chooses its side cell.** Where the rail type forbids 90-degree turns, `GetTBMWorkRailPath` keeps the usual side cell unless its pieces would cross the piece before them. Then it passes the other side cell.
- **Evidence (scratch enumeration of `GetTBMWorkRailPath` and `CheckTBMSupportRailTurns`).** For every heading sequence of up to 10 advances that leaves the housing straight:
  - a legal side assignment exists;
  - the greedy choice above always finds one;
  - it never changes a path that was already legal.

  20,000 random routes of 10-200 advances gave no failure either.
- **Consequences.**
  - Every 45-degree route the planner proposes from a housing has drivable support.
  - Support already laid in existing saves keeps its cells.
  - Where 90-degree turns are allowed nothing changes.

## Refusals

An approach is all or nothing: a refusal carries no route and no cells. In order:

1. **Unsafe limits, generator, layer, locked Deep.** Refused before any search, exactly as `PlanExcavationRoute`: `Limit`, `InvalidPolicy`, `MixedLayers`, `LockedDeep`.
2. **Bad continuing job.** A foreign, finished, malformed or full job refuses as `PlanExcavationJobExtension` does. A continuing job may not claim a solid start. A fresh start that the caller called open but that is an unopened housing refuses with `MissingSupport`.
3. **Exhausted budget.** `Limit`.
4. **Something in the way.** `SearchExcavationTarget` keeps, in `ExcavationRouteProposal::blocker`, the refused cell nearest the unreached target. The edge of the working box is not a blocker. Blockers are a building (`Conflict`), another project's claim or active cut (`ActiveCut`), a foreign seam or company track (`Conflict` or `InvalidOwner`), and the map edge (`InvalidTile`). That cell is the failure tile the quote highlights.
5. **No blocker, but the box edge refused.** The floor needs more room than the margin: `Limit` at the goal.

A building on a floor cell that is already open does not block anything. Open floor counts as covered, and the cutter covers its neighbours from the other side. Tests that need an impossible request use another project's claimed cell instead.

## How the TBM quotes use it (Group 9)

- **Commission (`PlanTBMCommission`).**
  - Start: the cutter in the housing with the measured planning points, `solid_start` true.
  - `MakeTBMWorkOutlineWithApproach` builds floor, ring and housing as `MakeTBMWorkOutline` does, but without its refusal of a floor away from the housing. It adds the approach and returns one connected, unquoted outline.
  - The commission then quotes and authorizes it as before.
  - A refused approach sets the preview's error and failure tile, and lists the whole floor as unreachable.
- **Extension (`PreviewTBMExtension`).**
  - Start: the pose after every accepted cut, from `ReplayExcavationRoute` (shared with `PlanExcavationJobExtension`).
  - `solid_start` false, with the job as the continuing job.
  - The builder does not quote, because earlier work may be mid-cut and a quote of it refuses.
- **Angled tunnels.** The pose overload of `MakeTBMTunnelOutline` keeps the straight centreline for an end exactly ahead. For any other end it builds the floor (cutter centres) and work (sweeps) from an exact-goal approach. The internal tunnel branch reaches such an end by the exact-goal search instead of refusing it.
- **Review (Groups 9-10).**
  - `TBMAreaPreview` and `TBMExtensionPreview` carry a `TBMApproachSummary` taken from the final route: the cells it sweeps beyond the base, the cutter positions that use them, and their estimate.
  - Both fingerprints mix the summary, so a changed approach cell makes the reviewed confirmation stale.
  - The palette shows the summary on its own line. Refusals read as map edge, building, other work or too far (`GetTBMApproachRefusal`).

## Change of work area (Groups 11-12)

The contract for the Plan2 (geology) and Plan4 (TBM) handoff updates in R2DF2 task 11.3.

- **Kept prefix.**
  - A change keeps every completed cut (`route[0, cursor)`) with its ground, completed statistics, cargo and money.
  - The job keeps its identity; only its revision moves.
  - With no completed cut there is nothing to keep: the first cut out of the housing belongs to the commission, so the change refuses ("has not finished its first cut") and Pause and extend remains.
- **Dropped remainder.**
  - The planned cuts not made yet are dropped.
  - Their claims end, and their never-opened cells go back to Solid unless a new cut reuses them; opened floor never reverts.
  - Authorization that only the dropped cuts used is withdrawn. Requested floor that is already open stays requested.
- **New cuts.**
  - The new remainder starts from the pose after the completed cuts (`ReplayExcavationRoute`). It is planned against a copy of the job cut to that prefix (`PlanExcavationJobReplacement`), so dropped cuts are neither open floor nor free claims.
  - The approach planner continues the same prefix (`kept_cuts`), and the approach is charged like any other.
  - `QuoteExcavationCells(..., own_job)` charges the job's own unopened claimed cells as rock, so a reused dropped cell is paid for again and nothing is free.
- **Exchanges.**
  - Breakpoints up to the cursor stay.
  - The new ones are seeded by the spoil aboard (`ExcavationLoadStart`, measured with task 4.2's helper), or by zero when an exchange is already due at the cursor. A nearly full load makes the exchange due at once.
- **Face work.** Partial work on the cut in progress is kept when the new remainder starts with that very cut, and abandoned otherwise. The quote states the abandoned share.
- **No pause; cursor-bound quote.**
  - Change work area is available while working, paused, returning or parked. It is not available once finished or being released.
  - The quote binds the completed-cut count, not the job revision, which moves every work tick, so a running machine can be replanned. The quote goes stale only when another cut completes (`STR_ERROR_TBM_CHANGE_STALE`).
- **Unchanged phase.**
  - The face the cutter works from (`route[cursor - 1]`) does not move, so `CmdChangeTBMWork` keeps the controller's phase, heading and work remainder and only bumps the project revision.
  - A boring cutter's next cut is the new route's first; a returning or parked machine goes back to that cut when it resumes.
- **Release and plan new work.** New area on the portal of a finished project asks whether to release it. Yes posts the ordinary Retire, whose refusal shows as usual, and then arms the same portal for a commission that reuses the stored machine.
- **Support at the face.**
  - The face's support piece was laid by the last completed cut, aimed at the old next cut.
  - The support path keeps a side cell a laid piece already leads to. A straight face therefore takes a turning first cut "straight first", on the track the cutter stands on.
  - When the new remainder still needs another piece, the change lays a junction piece at the face (`PrepareTBMFaceSupport`), so a returning or parked machine can drive back to it.
  - A train standing on the face cannot switch pieces, so that case refuses: choose work that starts the way the machine faces, or return and park it first.
- **Return from the face.** Return and park or Cancel pressed during Bore or AdvanceSupport backs the consist out (`BackUpTBMConsist`), like the automatic return. The pathfinder's choice could leave a cutter that was rolling onto new support at its dead end.
- **Evidence.**
  - `[demo-fix-change-work]` (geology) and `[demo-fix-change-work-ui]` (command and windows).
  - Native `change` run `20261016-011136-dc49659e86`: a change while boring, then while parked, each next cut completed as planned.
  - Its two checkpoints replay identically.

## Measured expansions

`[demo-fix-approach]`, default limits 4096/8192/256/1024. The fixture's door is at (20,20) Underground, facing SE, with 4 planning points.

| Case | Floor | Cuts | Extra cells | Expansions |
| --- | --- | --- | --- | --- |
| Screenshot 05, strip beside the door | (20,21)-(27,21) | 7 | 9 | 19 |
| Floor behind the housing | (19,14)-(21,15) | 12 | 34 | 227 |
| Diagonal offset | (25,26)-(27,27) | 11 | 33 | 248 |
| Exact goal, off-axis tunnel end | (24,26) | 7 | 41 | 361 |
| R8, from the door widening right | (20,21)-(23,28) | 0 (base covers it) | 0 | 55 |
| Budget refusal | (20,21)-(27,21), budget 12 | - | - | 15 |

- **Headroom.** The largest count, 361, is under 5% of one attempt's 8192 expansions, so the default limits suffice for these shapes. A refusal spends at most twice the budget: the attempt inside the base outline, then the one in the box.
- **Exact goal.** Its extra count is high because its base is the single end cell.
- **The demo itself (task 9.7, native).** The beside-exit request at (13,10) is accepted with 7 cuts and 9 approach cells (£180 of £570), with 90-degree turns forbidden. It digs to completion with every consist part on usable rail.
