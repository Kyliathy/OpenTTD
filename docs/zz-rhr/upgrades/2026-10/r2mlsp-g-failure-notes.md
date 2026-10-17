# R2MLSP-G - Module Freight Failure Acceptance Notes

Owner: [space master](../../../zz-reach2/upgrades/2026-10/r2mlsp-space-master-plan.md) and
[Plan9 Group11C](../../../zz-reach2/upgrades/2026-10/r2mlvs9-launch-centers-and-rockets.md#group-11c---verify-direct-module-freight-contention-and-target-loss).
Started:2026-10-14. Group11C/task11.4 is complete in both configurations.
Next is Plan9 Group12; historical unbuilt/pending sections below retain their
dated development context and are superseded by this final checkpoint.

## Final423 handoff - 2026-10-14

Both game/test configurations are423. Release and Debug each pass494 focused
assertions/18 cases, seven positive phases (including five exact interrupted/
paused26-family continuations), eleven actual paid-world failure phases and
six malformed full-load/independent-preview refusals. Debug's21 bounded wrapper
steps all pass in `logs/module-flight-debug-progress.json`; its original
workshop PNG was inspected. All lost phases recover the original payload once,
retaining15 fuel for unstarted cancellation or burning only the funded10/5
round trip after ascent. Native data remains in its single actual packet owner.

Final exact proof: `logs/module-flight-verification.json` (35 reports,199
artifacts,81 actual process records). Earlier Release positive/failure proofs
and all immutable418/421/422 inputs remain untouched. No build/test is running.
Source tasks11.3/11.4 and master Groups11/11C are checked; Group11B stays checked.
Continue at Group12 after its Global/nearest Launch Area. Inspect/reuse actual
launch-center/drop/recovery commands and descent owner before implementing any
missing release geometry/capacity admission. Later art/network/manufacturing
and all remaining source-plan completion conditions stay open.

## Required reading and delivered predecessor

Plan9 Global, Launch/payload Area, closest Module freight failure Local and
the Operational module freight Local/handoff have been read. Parent AD10,
packet/cache/no-payment boundaries and Plan11's public module/approach producer
remain the same inspected contracts. Read only those relevant sections after
compaction. Do not preload later descent/art or whole linked plans.

Group11/task11.3 now passes in RelWithDebInfo:295 assertions/14 cases,7 native
phases, five interrupted and paused26-family continuations, actual workshop
selection and original rocket15/gen1 recovery. Source seed418 has six completed
missions,90 fuel burned and empty onboard supplies;15 real Surface fuel packs
the new trip. The final ledger is7 completed,105 burned. Exact423 proof:
`logs/module-flight-release-verification.json` (7 reports,28 bound artifacts).
Both game/test Release binaries are423; Debug binaries remain422. Deferred
Debug consumer validation belongs in the next substantial batch. Earlier
418/421/422 inputs and every frozen proof remain immutable.

## Current unbuilt source

Created/registered `src/tests/orbital_freight_bounds.cpp`: three isolated cases
for whole-label atomic refusal/success,64-fragment admission, exact generation/
owner/stock role/berth and exclusive passenger/service custody. Its explicit
constructed test frames, hull and packets are boundary data, never a claimed
paid native gameplay predecessor. Public native module activation and shared
approach arbitration still supply their real container/slot behavior. Cleanup
accounts for all isolated packets and restores fixture content/engine mappings.
This new file has not compiled or run. No build/test process is running.

### Completed source/fixture batch, unverified

Added label aggregation to the isolated bounds (four cases total). New
orbital_freight_failure.cpp uses actual paid builders/queues for Core berth
contention, actual empty Storage activation/departures/removal/reused ID for
Preparing/Ascent/Delivery/Approach loss, and explicit cancelled recovery with
all original2STEL/1RFMT/1ELEC retained. A separate actual second-rocket
commissioning case checks occupied-pad and over-capacity hold refusal without
another mission fee or private cargo fill.

Wrappers4/5/6/8 split contention, each target-loss phase/recovery and pad
contention into bounded native/ordinary bursts. Wrapper7 has three two-case
batches for framed future generation, early/incompatible receipt, successful
receipt retaining payload, wrong shared request and older422 Structure-domain
refusal; every case requires full loader and independent preview refusal.
All eight wrappers parse; new producers are registered. The positive native
observer now also compares copied raw cargo-reference count with its actual
sender, so the U32 preview decode is exercised explicitly in the next batch.
The original Release positive proof remains dated evidence of its exact binary/
source hashes; this batch will issue its own current evidence rather than
rewrite the earlier proof. Focused Release test build is next; no gameplay
source/schema edit was needed for these negative fixtures so far. Debug423
build follows after correcting any focused source/fixture failures.

## Next substantial batch

### Release acceptance passed, Debug pending

Current Release focused contracts pass494 assertions/18 cases (four new
isolated packet bounds). The first bound run caught only a fixture omission:
Vehicle freight must call UpdateLoadingTile before Append, setting its native
in_vehicle state; this is corrected. The test include<filesystem> was also
added before the existing save-fixture header. No product defect was found.

Eleven genuine native failure phases pass: two builder/rocket contention,
one second paid rocket/pad/over-capacity-hold refusal, four actual empty Storage
removal/reused-ID phases and four explicit cancelled recoveries. Every ordinary
paused resave matches26 owner families. All six framed malformed claims refuse
both full loading and independent preview (12 refusals); the full loader's
generic Invalid native Space fleet is a valid late authority boundary for
future generation/retained payload and is now explicitly recognized. Copied
raw cargo-reference counts are checked against actual original sender lists.

Exact current Release reports:

- contention prepare: `mlm/reviews/space/module-contention-prepare-verify/20261014-073316-a6dd8f1082/reports/orbital-module-contention.json`
- contention continue: `mlm/reviews/space/module-contention-continue-verify/20261014-073332-004482a842/reports/orbital-module-contention.json`
- paid pad/hold: `mlm/reviews/space/module-pad-contention-verify/20261014-073359-c2c5527969/reports/orbital-module-pad-contention.json`
- lost preparing: `mlm/reviews/space/module-lost-preparing-verify/20261014-073435-c393421c1c/reports/orbital-module-target-lost.json`
- lost ascent: `mlm/reviews/space/module-lost-ascent-verify/20261014-073440-64a782df6d/reports/orbital-module-target-lost.json`
- lost delivery: `mlm/reviews/space/module-lost-delivery-verify/20261014-073445-5216b73fb0/reports/orbital-module-target-lost.json`
- lost approach: `mlm/reviews/space/module-lost-approach-verify/20261014-073449-2a1fc6c401/reports/orbital-module-target-lost.json`
- recovery preparing: `mlm/reviews/space/module-lost-recovery-preparing-verify/20261014-073855-f200c0dae9/reports/orbital-module-lost-recovery.json`
- recovery ascent: `mlm/reviews/space/module-lost-recovery-ascent-verify/20261014-073859-c2311904f5/reports/orbital-module-lost-recovery.json`
- recovery delivery: `mlm/reviews/space/module-lost-recovery-delivery-verify/20261014-073904-f253e2b674/reports/orbital-module-lost-recovery.json`
- recovery approach: `mlm/reviews/space/module-lost-recovery-approach-verify/20261014-073908-562e2223be/reports/orbital-module-lost-recovery.json`
- malformed1: `mlm/reviews/space/module-flight-refusals-1/20261014-073655-e9dff4b2f8/reports/orbital-module-flight-refusals.json`
- malformed2: `mlm/reviews/space/module-flight-refusals-2/20261014-073725-985593027e/reports/orbital-module-flight-refusals.json`
- malformed3: `mlm/reviews/space/module-flight-refusals-3/20261014-073733-be742a6df0/reports/orbital-module-flight-refusals.json`

Build/test logs: `logs/module-failure-release-build-3.log`,
`logs/module-failure-release-contracts-final.log`. Earlier failed logs remain
truthful evidence, not pass records. Next: one consolidated Debug423 game/test
build, then focused494 contracts, positive direct flight and five interrupted/
paused continuations, the same failure phases and malformed refusal bursts.
Do not overwrite the frozen Release positive proof. Keep11.4 unchecked until
this batch's current evidence is assembled; later source-plan gates stay open.

1. Finish isolated source review (canonical engine and empty native Vehicle pool
   setup/teardown) and add remaining label alias/reservation/custody boundaries
   only where they exercise distinct failure behavior.
2. Add bounded native actual builder/rocket berth contention from genuine418,
   then explicit paid Storage activation and actual builder departures for
   target-removal cases. Test Preparing, Ascent and OrbitalDelivery loss using
   real empty removal/reused IDs; preserve the original loaded cargo until
   cancelled recovery. Ascent cancellation policy must be inspected: currently
   direct cancel is available only in Preparing/OrbitalDelivery, so lost ascent
   should finish that leg before explicit return.
3. Characterize whole full-hold/module admission using isolated boundary data
   separately from paid-world positive/replay proofs; no synthetic fill may
   become an accepted gameplay checkpoint or proof of transported cargo.
4. Add correctly framed423 malformed cases: future module generation, wrong
   flight/request domain or phase, receipt retaining actual freight/parts,
   incompatible success flags, and explicit older-version Structure refusal.
   Every case needs full native loader and independent preview refusal; bind
   the exact positive predecessor rather than finding a latest output.
5. Bundle these source/fixtures before one Release/Debug validation batch,
   two outer MSVC workers and CL_MPCount2. Each native wrapper burst stays below
   two minutes. Update only completed source tasks, then the master. No full
   regression and no per-edit builds.

High commenting, no Git write operations and the user's unrelated untracked
`docs/Demo2026-06-13/` remain standing constraints. Group11C has not yet compacted
since its meaningful implementation started; add1 above11C after compaction,
without changing completed Group11's count1 or any completed Plan11 group.
