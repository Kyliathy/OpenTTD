# Native utility reservation and settlement contract

Owner: [Plan 12 Groups 7C-9](../r2mlvs12-solar-farms-mirrors-and-eti.md#group-7c---define-reservation-policy-and-complete-ledger-fixtures). Dispatch: [master 9.1](../r2mlvs3mp-master-plan.md#group-9---supply-the-later-integrations-shared-producers). Frozen ownership: [Content Freeze](../r2mlvs1-g-content-freeze.md#group-20---transaction-and-saved-state-ownership). Save authority: [Plan 15](../r2mlvs15-savegame-determinism-and-lifecycle.md).

**Artifact-layout migration (2026-10-09):** this plan consumes the parent [Development artifact layout](../r2mlvs-multilayer-vehicles-and-structures.md#development-artifact-layout), implemented by [R2FC](../r2fc-file-cleanup.md). It supersedes earlier docs asset/review/helper placement and fixed runtime-config destinations below. New native homes use `mlm/runtime/`, generated feature reviews use `mlm/reviews/`, logs remain under `logs/`, and reusable code/curated inputs keep their code/test owners. Paths and focused native/cold verification are complete; R2FC records the results and retained-input identities. Repository-relative executable commands run from the checkout root through their context-aware wrappers. Historical pass records remain dated evidence and are not new acceptance claims. No gameplay task is completed by this migration and the separate post-demo fixture-pruning gate remains.

## Delivered ledger boundary

`utility_power.h/.cpp` owns electrical source and consumer IDs, canonical meters,
explicit eligibility, interval phase and original source reservation slices.
`UPLH`/`UPLD`, version381 and feature bit1024 preserve these fields. Full acceptance
waits for actual companies, world layers and saved profile/calendar; preview uses
an independent copied transport. No display cache can recreate missing energy.

The [focused owner burst](../../../../../logs/r2mlvs3mp-u-power-save-owner-checks-1.log)
passes555 assertions/eight cases. The [actual file burst](../../../../../logs/r2mlvs3mp-u-power-native-file-checks-1.log)
passes ten isolated native application cases: three phases through two reloads,
five malformed preview refusals with a good retry, and next-interval continuation
through another load. Both concrete utility chunks remain byte-exact across
unchanged loads. Fractional generation27mJ resumes as54mJ after another real
publication; a removed54J source leaves the other27J available.

## Explicit accounting order

| Phase | Authority and required behavior |
| --- | --- |
| Publication | Each physical source owner publishes its actual bounded rate/duration once, under its company, using the current ledger epoch. Zero supply still counts as publication. Unpublished sources contribute zero. |
| Allocation | One synchronized `OWNER_DEITY` phase closes publication, after all requested producers/consumers have been gathered. The frozen allocator sorts explicit IDs; every consumer receives a current zero baseline before requested grants are applied. Reserved energy is a promise, not actual use. |
| Settlement | Group8 prepares against current epoch, owner, saved free source slices and operation sequence. Complete participants revalidate physical/inventory admission before commit. A refused participant changes no material, money, work, source slice or sequence. |
| Effects | Group9 applies the accepted bounded effect in the same whole transaction as consumption. Reading a meter or showing a powered icon never applies a benefit. |
| Rotation | The synchronized monthly owner finishes its settlement phase, then calls `BeginUtilityPowerInterval` exactly once. Remaining whole supply/promises expire; only source millijoule carry and persistent identities survive. New sources built during a closed phase wait for the next publication phase. |

This ordering is one coordinator contract, not several `Priority::None` timer
registrations. The existing publication/allocation/rotation APIs enforce their
phase and actor gates. Integration with an actual power-producing industry or
tube job follows its owning plan; this ledger checkpoint introduces no producer
that obtains free energy through a camera, wall clock or unrelated timer.

## Settlement obligations for Group8

For each source slice, `free = allocated - consumed - released`; every subtraction
is checked before arithmetic. Current source `reserved_j` equals the sum of free
slices. Actual consumption removes a free quantity and records settled use once.
An unused release expires only free quantity; it never reverses consumed work or
republishes generation. Source loss retires the ID, removes eligibility and
releases its unused slices, preserving other origins and already consumed units.

`UtilityGrantID` must identify an exact consumer reservation and epoch. Each
prepared mutation carries its exact kind, quantity, company, current saved
operation sequence and, for transfer, explicit destination/commissioned link.
Preparation changes nothing. Commit rechecks the proposal against live authority
and the actual participant. A successful commit advances the saved sequence;
the same proposal then refuses, including after a save/reload. Failure keeps the
sequence and every participant unchanged. A callback cannot receive an energy
debit before it has accepted all of its material/output/work changes.

A layer transfer preserves the original generation IDs, debits source availability
and credits bounded destination availability through one explicit eligible link.
Physical construction/commissioning belongs to its builder; runtime link identity
and metered flow belong to the utility ledger. Geometric proximity supplies no
link. Imported electricity cannot re-export in the same interval and never calls
`PublishUtilityGeneration` at the destination. Lost or invalid endpoints release
uncommitted flow without refunding already settled units.

Typed mutation/link records, the shared transaction participant, actual operation
sequence persistence and consumer effects are producer obligations for Groups8/9.
This document defines their policy and records ledger readiness; the full Useful
Power Handoff and master9.1 remain pending until those implementations and their
focused refusal/save/effect fixtures pass.

## Current implementation checkpoint

Groups8.1/8.2 now deliver typed transient grants/proposals, Consume/ReleaseUnused,
whole-participant commit, saved operation sequence and source lifetime totals.
Version382 extends the original381 transport with exact older-format migration.
Six native migration/settlement cases preserve consumed/released chunks and reject
the original issued ticket after loading. The actual recipe executor accepts only
an explicit participant-owned token; nine focused recipe/mutation cases pass366
assertions, including complete-owner failures and cross-layer refusal. Explicit
commissioned links, TransferLayer and transfer/loss/save cases remain8.3/8.4;
consumer effects and the full handoff remain Group9.

Directed transfers now pass five actual native topology/file cases and645 owner
assertions across nine focused cases. Version383 preserves original generator
IDs, directed endpoints/revisions/flow and imported/transferred slices, with
381/382 readers. Portal sealing expires only unused imports; rebuilding uses a
new identity and cannot revive old credit. Same-interval import re-export refuses.
The rebuilt normal executable loads/resaves current directed-flow chunks exactly.
Group8B is complete; consumer effects and the full handoff remain Group9.
