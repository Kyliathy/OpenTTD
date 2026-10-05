# R2MLVS3MP-F - Solid-starter contract

Owner: [R2MLVS3MP](r2mlvs3mp-master-plan.md) Group 2. Implements the shared decision of [Plan 3 Group 8B](r2mlvs3-sealed-portals-and-deep-rail.md#group-8b---publish-the-narrow-solid-starter-geometry-contract) task 8B.1, [Plan 2 Group 14B](r2mlvs2-geology-and-excavation.md#group-14b---reconcile-canonical-planning-and-cuts-with-a-solid-commissioning-pocket) task 14B.1 and the consumer side of [Plan 4 Group 2B](r2mlvs4-tunnel-boring-machines.md#group-2b---prepare-complete-acquisition-without-premature-activation). Parent decisions: AD-3, AD-5; invariants I-2, I-4, I-13. Architecture: [map layers](../../architecture/map-layers/archi-map-layers.md).

Frozen 2026-10-05, before dependent code.

## The problem in one paragraph

A native rail portal into rock builds two housings. The destination housing D keeps Solid packed geology, and so does every cell of the 3x3 envelope around it (the pocket P, which includes the door cell D' in front of it). The canonical geology path assumed the cutter starts with its whole 3x3 body in open ground: `PrepareExcavationPass`, `ReserveExcavationJob`, `PrepareExcavationCut` and `PlanExcavationRoute` all refuse `MissingSupport` at D. Painting P open would hand out a free room (AD-3/AD-5 forbid it). The fix is a first advancing segment that sweeps and pays for P and the row beyond D' together, with the portal pair standing in for the missing open support.

## Physical geometry

| Name | Definition | Owner |
| --- | --- | --- |
| Source S | Accessible upper rail endpoint, the canonical pair key (`GetLayerPortalPairKey`) | Plan 3 |
| Source support Q | The 3x3 floor centred on S that `ValidateLayerPortalCommissioning` already checks: plain Clear or same-company compatible plain rail, excavated when on a rock layer, no vehicles | Plan 3 |
| Destination D | The other endpoint, on the lower rock layer; this is the job's `commissioning_anchor` | Plan 3 geometry, Plan 2 terrain |
| Exit heading H | `DiagDirToDir(GetLayerPortalDoorDirection(D))`; the housing track is straight, so cut 0 cannot start with a 45-degree turn | Plan 3 |
| Pocket P | `GetExcavationStarterPocket(D)`: the sorted 3x3 envelope centred on D on its own plane; absent if it would wrap or leave the plane | Plan 2 (shared helper; `GetLayerPortalWorkPocket` reuses it) |
| Straddling consist | While the cutter is at D the trailing parts are in the shaft or on Q; on the rock plane every trailing planning point projects to D (`history = {D, D, ...}`) | Plan 4 supplies the point count |

## When the solid-starter rules apply

They apply exactly while D's own packed cell is not Excavated: Solid before reservation, Excavating after reservation, Excavated after cut 0 commits. An Excavated anchor keeps today's open-starter behaviour, byte for byte. No new saved job field is introduced: the pair's work record already says which job it commissions, so a second pointer from the job back to the pair would be a duplicate authority.

## Issuer currency

`GetLayerPortalStarterIssuer(anchor, company, job)` is the only cross-owner query geology makes. It returns `None` and H only when all of these hold:

1. `anchor` is the non-key endpoint of a reciprocal rail pair with a native work record.
2. The record's company equals `company`, which is a valid company.
3. `CheckLayerPortalAvailability` passes (the 1980 Deep gate covers both ends).
4. Either the record is Sealed with no project/cutter and `job` is invalid (an unpublished proposal), or the record is Commissioning with `project == job` and `job` is valid (published work).

Geology maps the refusal to its own error: lost/invalid pair -> `MissingSupport` (the old characterization error, now precise), owner -> `InvalidOwner`, date -> `LockedDeep`, any other assignment/phase mismatch -> `StaleParticipant`. The failure tile is always D.

## First advancing segment (route[0]) rules

Checked by `CheckExcavationStarterGeometry` in pass preparation, at cut 0 and by the portal's load validator:

- `route[0].heading == H`, so the cutter moves D -> D'.
- Every cell of P lies in `route[0].swept_tiles`. With the straddling history this is the canonical 3x4 rectangle of 12 cells.
- Every `route[0].support_tiles` cell lies in P. The pocket is restricted occupancy, never floor.

## What each geology stage does in starter mode

| Stage | Open-starter rule replaced | Solid-starter rule |
| --- | --- | --- |
| `PlanExcavationRoute` | Initial support must be Excavated | Start must be D facing H with every history point at D; the first edge is forced straight; pocket cells count as new cells for scoring and estimate |
| `PrepareExcavationPass` | Anchor 3x3 and exchange path must be Excavated | Issuer current; each P cell is Solid (charged) or Excavated (free) and unclaimed; the exchange path may start at the housing D; cut 0 support is the pocket |
| `ReserveExcavationJob` | Anchor must be Excavated; non-Clear solid cells refuse | Issuer current; D (owned LayerPortal, Solid) may be swept and becomes Excavating like any other solid cell; its tile metadata is untouched |
| `PrepareExcavationCut` at cursor 0 | Previous 3x3, support and retreat must be Excavated | Issuer current and published; P cells must be Excavated or claimed by this job; D may be swept and retreat starts at D |
| `CommitExcavationCut` | unchanged | Opens all 12 cells once, debits once and pays once; D stays a LayerPortal tile with Excavated rock |

Ordinary construction is unchanged: `CheckExcavatedFootprint` still has no pocket parameter and still refuses every unopened pocket cell.

## Capacity consequence

Cut 0 charges 12 new cells instead of 3. Soft ground needs 72 t Earth and 144 t Rock, so a consist with one 64 t Earth and one 128 t Rock hopper refuses `InsufficientCapacity`. Two of each fit. This replaces the "cleared 3x3 commissioning area" assumption of [r2mlvs1-e](r2mlvs1-e-ground-and-space-budgets.md) line 32; Plan 4 Group 20 and Plan 15 Group 16 must measure the larger first-cut cost.

## Portal proof (Plan 3, consumed by Plan 4)

`IssueLayerPortalBootstrap(request)` is pure and runs the existing `ValidateLayerPortalCommissioning` prefix, then returns `LayerPortalBootstrapProof`: pair key and revision, company, model, origin, S, Q, D, H and P. It needs no route and no train, so it can be quoted before acquisition. `RevalidateLayerPortalBootstrap(proof, request)` re-issues and requires every field to match, so a stale revision, a changed source, model, date or origin all refuse. The proof grants no excavation, no vehicle admission and no purchase; actual project publication still goes through the Plan 4 owner.

## Load validation

`ValidateLayerPortalWorkRecords` (Plan 3) already binds a Commissioning record to its job and destination. It additionally requires: every open job whose anchor cell is unopened and whose cursor is 0 has a Commissioning record with `project == job`, and that job's route[0] obeys the first-segment rules. A save that breaks either is refused rather than resuming with a stale issuer.

## Plan 4 consumer obligations (Group 2B)

- Build the planning start pose from the proof: position D, heading H, history D repeated once per occupied consist tile (3..16).
- Size the consist so cut 0 fits both independent wagon groups; quote it through `PrepareExcavationPass`/`ReserveExcavationJob` estimates.
- Revalidate the proof and every participant immediately before acquisition; reserve the job while the pair is Sealed, then publish the Commissioning record in the same command, or cancel the job on rollback.
