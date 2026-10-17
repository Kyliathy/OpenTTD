# R2MLSP-U - Shared motion and attachment publication

Owner: [Space master plan](../../../zz-reach2/upgrades/2026-10/r2mlsp-space-master-plan.md), [Plan10](../../../zz-reach2/upgrades/2026-10/r2mlvs10-asteroids-and-mining-probes.md) Group3. Date:2026-10-14. [Generation input notes](r2mlsp-t-generation-inputs-notes.md) preserve accepted Group2 schema424 evidence; those tasks and the master item are checked. The final index is `logs/asteroid-input-verification.json` with20 reports/64 native processes and182 audited referenced identities.

## Implemented and accepted batch

Group3's six tasks and its master item are checked after both configurations passed. Group4 also completed its documentation reconciliation; Group5 is next. Existing body reflection, stable ID storage, fractional travel and native save fields were already delivered. Heading/speed literals are now selected-profile catalogue exports with the same eight equal direction slots and three equal speed slots, feature domains and draw order. Velocity validation consumes those same heading records. `MultiLayerSpaceTuning` in `multilayer_content.h/.cpp` publishes the existing Q16 scale,74-tick economy day, middle-half strip and two-tile body-centre margin through the Industry profile; Classic exposes no tuning. Shared geometry consumes that record. Defaults, generation version1 and save schema424 remain unchanged.

`AdvanceAxis` checks its bounded preconditions and reflected candidate before publishing the coordinate. Supported body velocity guarantees one step is smaller than the axis span. Exact arrival reverses its outward component and fractional magnitude survives reflection; corners reflect both axes independently. This does not introduce random turns, wrap or an orbital integrator.

Group3's rendering wording was reconciled with the actual native renderer: `SpaceWorldPoint` projects published Q16 positions to integer sprite pixels. There is no shared sub-tick interpolation API, and this batch introduces no new rendering clock. Task3.3 now explicitly requires read-only projection and confines any interpolation to presentation. Drawing never advances motion, leases or resources. The broader presentation review remains in later groups.

`asteroid_motion.cpp::UpdateAttachedSpaceCraftTransforms` publishes every existing native Mining lease after body motion and index updates, before `CallVehicleTicks` can run any craft's work decisions. A saved target alone grants no attachment: deployment, Mining phase and the actual matching body lease are required. Lost/foreign leases remain the mission controller's responsibility. Translation retains the mission's saved offset and updates the actual native position/hash. The existing per-craft transform remains idempotent. Existing attached observation transforms also publish together before any preview decisions.

New focused `src/tests/asteroid_motion.cpp` covers all24 heading/speed combinations, every tick's cumulative displacement/remainder, maximum carry, exact arrival, all four corner signs and attachment envelope. A literal seed/position/vector/resource fixture comes from the accepted pre-change424 native seed12345 Normal world. Numerical fixtures use the actual shared motion/travel functions and the declared frozen conservative deadline, including32-tile fastest drift and three far generated starters. They count travel upkeep in addition to the25,900 two-return collection ledger; they do not claim the later paid pursuit/attachment controller acceptance.

`asteroid_motion_native.cpp` uses existing immutable404 attached-preview and405 working-native sources. It stages only explicit bounded body geometry in an invocation copy, translates existing attachments, saves before the edge/corner, observes `TickAsteroidField` before any craft work, then advances37 real additional working ticks with Surface selected. Four bounded file2 cases (`preview-edge`, `preview-corner`, `native-edge`, `native-corner`) compare independent resume and ordinary resave across all26 current owner families. Native cases use an already existing matching lease, never create a hull or resources. The direct producer step is an explicitly isolated geometry observation, not an extra production simulation tick.

## Accepted verification and resume

Wrappers are `src/tests/multilayer/acceptance/asteroid_motion_checks-1.py` (focused) and `asteroid_motion_checks-2.py --case onecase` (one native boundary). Their syntax, registrations and `git diff --check` pass. The consolidated Release build completed after one incremental correction batch: two missing test includes, catalogue-driven velocity validation, preview publication ordering and the correct fastest diagonal fixture11585/remainder40. Successful build log: `logs/asteroid-motion-release-build-2.log`. The Debug product/test build also passed as `logs/asteroid-motion-debug-build.log`, with two outer workers and `/p:CL_MPCount=2`. No configuration was tested during its own linking. Both completed configurations passed the same bounded cases and exact corresponding saved phases.

Release accepted44,368 assertions/16 focused cases and3,684 boundary assertions/eight native cases. Five reports cover13 native processes and46 audited referenced identities. Exact report paths:

- `mlm/reviews/space/asteroid-motion-verify/20261014-211734-80c7886d22/reports/asteroid-motion.json`.
- `mlm/reviews/space/asteroid-motion-boundary-preview-edge/20261014-211745-ef4d312664/reports/asteroid-motion-boundary.json`.
- `mlm/reviews/space/asteroid-motion-boundary-preview-corner/20261014-211759-ab32e03db1/reports/asteroid-motion-boundary.json`.
- `mlm/reviews/space/asteroid-motion-boundary-native-edge/20261014-211805-de26b8dfea/reports/asteroid-motion-boundary.json`.
- `mlm/reviews/space/asteroid-motion-boundary-native-corner/20261014-211821-210bf86a4d/reports/asteroid-motion-boundary.json`.

## Worked trajectory and cost records

On64x64, shared centre bounds are X2..61 and Y18..45 tiles. Units below are Q16 unless marked as tiles. All velocities are per74-tick economy day; remainders are unsigned fractional travel.

| Fixture | Before | After one tick |
| --- | --- | --- |
| Accepted native seed12345 first body | Position(30,30) tiles; velocity(5792,-5792); remainders(0,0) | Position(30 tiles+78,30 tiles-78); velocity unchanged; remainders(20,20) |
| Fast cardinal at upper X edge | X61 tiles; velocity X16384; remainder X73 | X61 tiles-222; velocity X-16384; remainder X29 |
| Fast diagonal at upper corner | Position(61,45) tiles; velocity(11585,11585); remainders(73,73) | Position(61 tiles-157,45 tiles-157); velocity(-11585,-11585); remainders(40,40) |
| Attachment at that corner | Saved offset(1,-0.5) tiles | Position(62 tiles-157,44.5 tiles-157); same offset and surviving lease |

The initial32-tile, fastest-away numerical pursuit arrives in632 ticks. Conservative three-tile/day closing takes790 ticks plus148 arrival-margin ticks, within the1480 minimum allowance. Two geometry-only collections and returns account for2765 pursuit/return ticks, adding3736 money with accrual remainder36 to the25900 collection ledger, or29636 before additional actual wait time. Paid descent/service waits and full mission acceptance remain with their controller groups. The distant64x4096 generated triple from preference(10,10) lies beyond32 tiles; all three fastest-outward cases reach their current translated target within the frozen extended allowance.

Debug focused report: `mlm/reviews/space/asteroid-motion-verify/20261014-213518-be68f52a43/reports/asteroid-motion.json`. Its four boundary runs are `20261014-213521-c735301552` (preview-edge), `20261014-213546-d81caf22ee` (preview-corner), `20261014-213606-a893274e8f` (native-edge) and `20261014-213632-26145685f2` (native-corner), in each corresponding boundary family above. Debug matches the44,368/16 focused and3,684/eight boundary counts. `logs/asteroid-motion-verification.json` binds all10 reports/26 processes,96 referenced identities audited at acceptance and520 cross-configuration comparisons (four cases, five phases,26 owners). Build executable identities describe those accepted invocations; future builds may replace the mutable build paths. No full regression ran.

After compaction, reread Plan10 Global and closest `Asteroid generation and motion` Area, then relevant AD-11, Map ownership, tick boundary, Randomizer and revised probe budget sections only. Shared reflection defaults are Plan8's named heading near434; its broader Group10 construction/envelope/approach tasks remain open. Current canonical mission is in `space_vehicle.h`, not the historical proposed `space.h` row. Group3 completed without an additional compaction; count meaningful compactions above its heading, split if more than three. Next auxiliary prefix is `r2mlsp-v-`. Preserve unrelated Demo2026-06-13 files, immutable original fixtures and old accepted proofs. No Git writes or sub-agents.
