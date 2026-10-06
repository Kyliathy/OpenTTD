# Source logistics file continuation

Owner: [R2MLVS5 Group 13A](r2mlvs5-spoil-wagons-and-haulage.md#group-13a---save-and-resume-actual-source-side-logistics), dispatched by [master 7BB](r2mlvs3mp-master-plan.md#group-7bb---complete-logistics-and-wait-state-reloads). This evidence does not complete the task.

The real writer produced five physically reached Deep cases: stopped startup,
awaiting exchange, Surface disposal, empty return and a hauler spanning the
Surface/Underground pair. All five version-380 inputs load and resave through
the actual Debug game (the five `r2mlvs5-e-*-loader.log` files). Original hashes
and startup script bytes were preserved.

The named table decoder in `src/tests/tbm_logistics_save_fixture.py` narrows
the three nonidentical `VEHS` chunks to one acceleration byte each: hauler 7
changes 48 to 16 in await-exchange/spanning, and cutter 0 changes 16 to 4 in
disposal. Every other vehicle field is exact. EIDS, ordinary order lists,
projects, support and exchange descriptors are exact in all five files.

The reduced `ExcavationFixture` installs labels with zero cargo weight, whereas
the distributed package declares weight 16 and freight status through
`media/multilayer/build_content.py` Action 0 properties 0x0F and 0x15. The
loader rebuilds consist weight and acceleration from those actual package
properties. The shared physical workflow now applies the pinned spoil properties
before buying or moving any vehicle. Fresh `source-native-deep-*` inputs
preserve the earlier files, but exposed an additional production cache defect.
Recording is opt-in through `OTTD_TBM_LOGISTICS_RECORD`; routine focused checks
never overwrite retained inputs.

The isolated application test runs `openttd_main`, the actual native content
loader and normal `GameLoop` using the null driver's fast frame loop. A test-only
console startup callback saves the untouched loaded baseline and starts stopped
participants through their synchronized command owners. A read-only frame timer
saves the next atomic physical exchange. Two isolated loads of stopped startup
reach the same exchange in 2,164 frames with 35 exact authoritative chunks.

The moving cases reveal two production defects. `TBMCutParticipant::Commit`
appends real spoil without rebuilding train mass/acceleration, so loading a
populated cutter changes its physical cache even with correct cargo properties.
It now updates both caches in the same paid cut. `CheckTrainsLengths` measures
stacked world coordinates across a valid portal pair as a broken coupling;
real loading consequently sets `PauseMode::Error` on a spanning cutter. Its
spacing check now unfolds only actual paired portal tiles into continuous shaft
coordinates, preserving ordinary malformed-consist detection. The runtime test
refuses an Error pause instead of clearing it. Focused actual crossing tests
exercise the same validator in forward and reverse movement.

The physical workflow also pins the native freight multiplier to 1. Unit-test
settings start at zero, which erases freight mass even after cargo weights are
supplied; the real loader validates that setting to its minimum of 1. The
focused fixes/workflow/economics burst passes 139,580 assertions / 14 cases
([s](r2mlvs5-s-native-mass-and-shaft-checks.log)). Corrected standard/Deep
whole-project invoices are 2,546,553 / 3,154,099 over 10,620 / 32,926 frames;
the [Plan 4 ledger](r2mlvs4-t-complete-project-ledger.md) supersedes its initial
weightless measurements. Purchases, cut costs, swept geometry and quantities
are unchanged. Fresh five-file recording passes 33,101 assertions
([t](r2mlvs5-t-fixed-source-writer.log)); empty return now follows actual disposal,
rather than the identically named initial empty phase.

Fresh `source-fixed-deep-*` files pass exact vehicle reconstruction and the same
next real exchange through one and two actual application loads. All 35 selected
authoritative chunks match, including packet ages/identity, wagon chains,
project/support/geology, receivers, company accounts, date and RNG. Original
input hashes and startup script bytes remain unchanged. Each isolated process
completes in under two minutes; normal runs take about 20 seconds.

| Actual input state | Frames to next exchange | First / second real-load logs |
| --- | ---: | --- |
| Stopped startup | 2,164 | [u-1](r2mlvs5-u-stopped-1.log), [u-2](r2mlvs5-u-stopped-2.log) |
| AwaitExchange | 2 | [v-1](r2mlvs5-v-await-exchange-1.log), [v-2](r2mlvs5-v-await-exchange-2.log) |
| Surface disposal | 10,125 | [w-1](r2mlvs5-w-dispose-origin-1.log), [w-2](r2mlvs5-w-dispose-origin-2.log) |
| Empty return after disposal | 462 | [x-1](r2mlvs5-x-return-empties-1.log), [x-2](r2mlvs5-x-return-empties-2.log) |
| Actual Surface/Underground span | 179 | [y-1](r2mlvs5-y-spanning-1.log), [y-2](r2mlvs5-y-spanning-2.log) |

The optimized playable executable builds and pairs language packs
([z](r2mlvs5-z-optimized-mass-and-shaft-build.log)). Plan 5 13A.1 is checked;
whole 13.2A remains pending. Its physically reachable recoverable stalls and
lifecycle/capacity/queue reachability audit are scoped into Groups 13AB/13AC,
each with a local reading boundary. Next is 13AB.1, genuine Surface-origin loss
and command-driven recovery. Expand reachable waits through actual actions and
distinguish guarded combinations.

13AB.1 is subsequently complete. `source-stall-deep-lost-origin.sav` captures
actual paid platform demolition during a loaded Surface trip, with phase
VisitOrigin, wait LostOrigin, exchange count 1 and the same real carried set.
The producer completes demolition, observed loss, ordinary ghost-sign reuse
and the full physical delivery loop: 90,341 assertions
([af](r2mlvs5-af-origin-reuse-checks.log)). Supplying an explicit station ID
initially required the optional distant-join setting; ordinary invalid-ID
construction correctly reuses the nearby deleted sign on its original rail.
No preference or private station/project fields were changed to recover it.
The test-only application observer derives the original straight platform axis
from retained owned rail and its length from the actual loaded hauler, then
uses the same owning construction command. Both actual loads reach exchange 2
after 10,835 frames with exact vehicle reconstruction and 35 exact next-state
chunks ([ah-1](r2mlvs5-ah-lost-origin-1.log),
[ah-2](r2mlvs5-ah-lost-origin-2.log)). The first retained input was written before
the initial rebuild-argument refusal; its actual saved loss is unchanged and
now has successful producer and application-resumption evidence. Current task:
13AB.2, actual receiver saturation/loss and command-driven recovery.

13AB.2 is also complete through naturally bounded intake. The actual Deep
haulage fills the receiver's frozen interval allowance while real spoil stays
aboard. The native wait is saved, and ordinary processing ticks reopen intake;
the same load completes disposal and physical empty return before another
exchange. The producer passes 57,415 assertions
([aj](r2mlvs5-aj-receiver-stall-writer.log)). Both actual application loads reach
exchange 2 after 9,940 frames with 35 exact next-state chunks and unchanged
physical reconstruction ([ak-1](r2mlvs5-ak-saturated-receiver-1.log),
[ak-2](r2mlvs5-ak-saturated-receiver-2.log)). No artificial receiver stock, timer,
position or wait flag is used. Current task: 13AB.3, actual route interruption
and repair.

13AB.3 now completes the recoverable-stall group. Its accepted input is
`source-route-deep-blocked-line.sav`, with native pathfinder defaults and a
genuine paid conventional fork in the declared accessible Underground approach.
The train has a valid route before paid removal of one ordinary Surface rail
cell; the native search then reports BlockedLine while the real loaded set
stays attached. Paid repair plus the ordinary reverse command gives the train
a fresh physical route choice. A bare single-track scene skips route searching;
an earlier fork prototype also had a zero native search budget. Their retained
inputs/failed logs are historical characterization, not accepted wait proof.
The corrected producer passes 92,338 assertions
([at](r2mlvs5-at-valid-line-writer.log)). One and two real application loads
reach exchange 2 after exactly 11,154 frames, with exact vehicle reconstruction
and 35 identical next-state chunks ([aw-1](r2mlvs5-aw-blocked-line-1.log),
[aw-2](r2mlvs5-aw-blocked-line-2.log)). The runtime repair walks the actual
owned straight Surface approach to its existing access portal, restores exactly
one missing rail cell and reverses by command; it infers no free rock cavity.

Current task: Group 13AC remaining lifecycle/capacity/queue waits and guarded
combinations, then source-family closure. Source 13.2A and parent 13.2 remain
unchecked until their whole named scope passes. Recurring underground bays and
their file proof remain 13B/13C after source closure.
Recurring underground-depot proof remains Group 13B; it cannot be replaced by
these source-side files. Historical phase/wait row staging tests establish
schema validation, not physical resumption.

Group 13AC's ordinary detach/recouple and short-empties cancellation files are
recorded in [be](r2mlvs5-be-wait-reachability.md). The actual two-hauler FIFO file
also passes: both normal application loads serve the existing first request
after two native frames, preserve the peer's actual cycle and reconstruct all
vehicle fields plus the same 35 next-state owner chunks
([bl-1](r2mlvs5-bl-occupied-bay-1.log), [bl-2](r2mlvs5-bl-occupied-bay-2.log)).
Its producer passes 26,682 assertions ([bk](r2mlvs5-bk-queue-cycle-checks.log)).
The input was written before correcting an overstrict assertion that assumed
the peer had never served an earlier physical arrival; the input remains
unchanged and the corrected complete producer passes.
