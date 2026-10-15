# R2MLSP-X - Public generation and body-state handoff

Owner: [Space master plan](r2mlsp-space-master-plan.md), [Plan10](r2mlvs10-asteroids-and-mining-probes.md) Group7. Date:2026-10-15. Groups1-7 and their master items are complete. Next is Group8's existing probe-model binding; [band policy notes](r2mlsp-w-band-policy-notes.md) preserve the preceding static/moving separation gate.

## Published current contract

Plan10's `Public Handoff Contract` now names actual public owners, fields and estimate boundaries. `asteroid.h` owns the sole field, finite body records, feature-local generation, stable ID lookup and ordered movement. Shared `SpaceMotion` owns Q16 positions, signed per-day velocities and unsigned fractional remainders; bodies have no duplicate mission-phase field. One resource class maps to one canonical cargo, so initial/remaining whole-tonne reserves are single-cargo quantities. Zero remains a live, moving, indexed body. No in-play removal/replacement path exists.

The selected profile and asteroid catalogue own immutable generation/content/motion definitions. ASTR424 owns generated records and the frozen initial input; older records preserve unknown history. SPKN owns native scan receipts; ownerless previews confer no quantity disclosure. `space_vehicle.h` owns the actual native hull/mission, not a replacement `space.h` type. Estimate assignment changes no work, target, packet or money. Sentinel lookup and typed invalid navigation are distinct from the explicit mining-command recall sentinel. Later consumers retain their paid role/launch/service/descent, manufacturing and full mission acceptance gates.

## Current verification

No new build was needed for this documentation handoff. Existing current binaries compile and call the public field/generation/tuning contracts. The bounded generation and continuation wrappers were rerun in both configurations after Group6:

- Release generation: `mlm/reviews/space/asteroid-generation-verify/20261014-221530-3e3ff1a80f/reports/asteroid-generation.json`.
- Release new/legacy continuation: `mlm/reviews/space/asteroid-generation-continuation-verify/20261014-221605-7b4694bd8b/reports/asteroid-generation-continuation.json`.
- Debug generation: `mlm/reviews/space/asteroid-generation-verify/20261014-221643-66231116c9/reports/asteroid-generation.json`.
- Debug new/legacy continuation: `mlm/reviews/space/asteroid-generation-continuation-verify/20261014-221749-6bf8459f5e/reports/asteroid-generation-continuation.json`.

`logs/asteroid-handoff-verification.json` binds these four reports/30 native processes and85 identities audited at acceptance. Each configuration passes37,122 assertions/12 focused generation/content cases, actual Sparse/Normal/Dense and Classic world creation/reload, and new/legacy37-work-tick continuations. Independent replay and ordinary reload preserve all26 owner families; corresponding configurations pass260 owner comparisons. Current local density does not reroll fields or overwrite saved velocity/remainders. Generation version1 and schema424 remain unchanged. No full regression ran.

## Resume at Group8

After compaction, read Plan10 Global and the closest `Probe mission controller` Area for Groups8-13, rather than the completed generation Area. Keep AD-11, Map ownership, vehicle tick/service boundaries and the Area's relevant native cargo/role/service producers. Relocate headings/exports instead of following stale line ranges; mandatory reading is non-transitive. The actual mission is in `space_vehicle.h`; the public-generation contract is now fully described in Plan10's handoff table. Group7 completed without an additional compaction. Count meaningful Group8 compactions above that heading and split if more than three.

Retain `logs/asteroid-input-verification.json`, `asteroid-motion-verification.json`, `asteroid-depletion-verification.json` and `asteroid-band-verification.json` as immutable acceptance indexes. Their executable identities describe historical invocations; subsequent builds can replace mutable build paths. Source/artifact/report identities remain retained. Use two outer build workers and `/p:CL_MPCount=2`, bundle related source/fixture work, avoid full regressions and poll live processes for50 seconds. Next auxiliary prefix is `r2mlsp-y-`. No Git writes or sub-agents; preserve unrelated Demo2026-06-13 and immutable original inputs.
