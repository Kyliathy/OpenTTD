# R2SFF1 construction diagnosis and verification

Owner: [Space fixes plan](r2sff1-space-fixes-features-1.md), Groups 4-8.
Native admission and paid work remain in `space_orders.cpp` and
`orbital_station.cpp`; [GUI invariants](../../architecture/gui/archi-gui.md#invariants)
prohibit manufacturing progress from presentation. The original screenshot save
is unavailable; these results concern the closest retained tours, not an exact
reproduction of the user's original sequence.

## Group 4: observed first boundaries

`construction-missing-inputs` loads retained `space-factory-440.sav`. Builder
ID18/generation4 initially has a current Visit Body0, while Storage ID1/generation2
has no delivered inputs and no work. The first missing prerequisite is a current
Construct instruction. The native edit estimate succeeds without changing
vehicle or structure digests. The actual Queue button appends Construct at
index1, but existing native edit semantics restart the queue at index0; the Visit
remains first. The explicit Clear then Queue sequence produces exactly one
current Construct targeting Blueprint1/generation2 and starts the builder.

After33 unpaused opportunities the craft holds the actual Storage berth and the
queue reports `MaterialsMissing` (block9). Its hold is empty, and both recipe
input containers have total/available0. Core0/generation1 is operational with
its30,000W panel enabled. After148 opportunities, field tick7046→7194, work and
all inputs remain0. No lost controller transition was observed: material
admission correctly refuses construction without the actual packets.

`construction-reserved-inputs` loads `space-module-reserved-418.sav` (SHA256
`879ca1ba9850bdda6d6b62c8e9b757f0aca5e643c15522ed05a80b4837cb4aa5`).
The same builder/site identities have actual Steel8/Goods8 reserved. Following
the same Clear then Queue path advances Storage from0 to89 work ticks during148
ordinary unpaused opportunities, field tick6573→6721. Travel/admission uses the
earlier opportunities. Both material containers retain8, consumed counts stay0,
and the current queue has no block. The normal allocator registers Core's actual
assembly consumer and pays for work through that owner.

After paid work, an unspent allocation of0 is expected: it is not evidence of
missing power. The last work receipt equals the native current tick and
`GetSpaceConstructionVisualWork` returns89. Similarly, field elapsed ticks and
`TimerGameTick::counter` are different clocks in these saves; logs label both.

Both native cases passed179 assertions, in4.359s and4.109s. Their test executable
SHA256 is `a3cc53d827c18de2d46d1f0bd17d644a374222ccb4d1d8b7d3858cfaad0202b8`.
The negative case checks a failed material prerequisite; it does not leave an
intentionally failing regression test or claim a broken work producer.

## Group 5: queue and identity contracts verified

`construction-queue-contracts` passed 226 assertions in 4.391s. Estimates and
mixed Execute/QueryCost calls preserve hulls, queues, field, manifests, utility
accounts, money, RNG and clock. Stale craft/site generations reject execution
without mutation. Editing an already working job preserves its earned progress.
A real Stop command followed by painting and estimates leaves the craft stopped;
an explicit successful queue edit starts it and ordinary work resumes.

`construction-service-held` passed 43 assertions in 1.797s against the retained
Core service checkpoint. Estimate, Execute and mixed flags all refuse queue edits
on the actual service-held construction probe, preserving supplies and receipts.

`construction-target-reuse` passed 117 assertions in 2.985s. Actual commands remove
the untouched, empty Storage blueprint and buy a replacement in the same native
slot1. Its generation changes2→4. The original Construct entry still names1/2
and remains `TargetRemoved` immediately and after the next ordinary tick. A stale
edit is refused; cargo stays with the original craft and replacement work stays0.
Removal refunds no money, and the new blueprint charges the actual quoted cost.
No pool slot, generation, stock or balance was assigned directly by the fixture.

These runs use test executable SHA256
`780bb4bb4dbd9a7ff11d9a3bfabb34cdd417c9f185e9ad6cb16b8f8d555f0975`.
No demonstrated command, arrival or deadline defect required a controller repair.
The original final-tick arrival protection and explicit edit semantics remain.

## Group 6 readiness and interruption results

`GetOrbitalConstructionReadiness` shares the native hull and deterministic
worker predicates, reports the exact configured site incarnation, and reads
the original inputs/approach/power owners. `Ready` is presentation only;
`IsSpaceConstructionWorkTick` still authorizes work. A current committed receipt
reports `Working` even after its grant is spent. Repeated queries preserve the
fleet, structures, field, utility ledger, manifests, company cash, tick and RNG.

Focused release results:

| Case | Evidence | Result |
| --- | --- | --- |
| Actual final delivery | `construction-delivery-readiness` | 350 assertions, 4.141 seconds. Two real trips load returned Steel/Goods from the Core. Missing Goods holds work at zero; final delivery still earns zero; the next ordinary tick earns exactly one. |
| Fractional work interruptions | `construction-interruption-readiness-v2` | 741 assertions, 6.500 seconds. Power loss, Stop, approach release/travel and actual paid service retain `62914` fractional units and remainder `420`; resumed work advances once. Service completes once using real returned parts. A second work callback in the same unpaused opportunity is refused. |
| Empty job explanation | `construction-missing-readiness` | 194 assertions, 4.172 seconds. The original unsupplied wait now reports `MaterialsMissing` without altering any owner. |
| Existing exact energy contract | `construction-energy-contract.log` | 3,019 assertions across the existing two `[orbital-assembly-contract]` cases, including every split of 750 J and the final 30 J cap. |

The interruption source is immutable `space-module-power-418.sav`, SHA256
`fdc60df03b0df3f6bcdeec80e3316073c4db4bf4ca7935f1348614e76450ba6c`.
It predates saved approach requests: immediately after load the first missing
prerequisite is approach admission. Five ordinary ticks restore that reservation
and expose the disabled panel. The first diagnostic run incorrectly expected
power immediately; its three failed expectations are retained in
`construction-interruption-readiness.log`. No production defect caused them.

The delivery source is immutable `space-module-cancelled-418.sav`, SHA256
`8af71a343be4a1dbb1b3c957c4f48a96ec96e38c13dd467062b52534b0f0f6fc`.
The new paid blueprint receives original returned packets; no input or work is
fabricated. Active reserved recipe inputs cannot be withdrawn and restored as
an independent player operation: collection requires cancellation. Consequently
material restoration is tested through the legal initially missing/delivered
boundary; interruption of an active job retains its exclusive packets.

Delivery/energy checks used executable SHA256
`0d0cf73a108ef65ddd5a35b48adde3de8c78b321467dc59ecf672136229c6a3e`.
The corrected interruption and missing-input cases used
`f8b061bae0b45a4ca5a687340aa2920de3e417781dc0d942a5ad7bd42a4a1bcd`.
No material, worker preparation or power producer repair was justified. Existing
native sampling before delivery and exact fractional accounting remain intact.

## Group 7 reload and shared-worker results

Four separately run checkpoint/control pairs passed with no differing fields:
`construction-reload-queued`, `construction-reload-materials`,
`construction-reload-power` and `construction-reload-working`. The native writer
saves the actual command-produced checkpoint; a separate application process
loads those exact bytes. Both sides apply identical legal remedies and run
405 ordinary ticks. Full fleet/queue/packet, structure, field, manifest and
utility digests match immediately and at completion, together with company
cash, hull profit/upkeep, RNG, exact generations, consumed inputs and work/energy
receipts. Partial work completes at 296 ticks with zero fraction/remainder;
the final five ticks consume no second recipe or construction grant.

These paired runs use executable SHA256
`ea0aa19477c3c8dd685bff0c18b40b9cd64764f6a95aa346c433d855b2231511`.
Each native process takes 8-11 seconds. Each `-reload` output directory contains
`comparison.json`; its paired `-checkpoint` directory contains the rebuildable
`checkpoint.sav`. Input/save/configuration/executable hashes remain in the
ordinary per-process reports. No immutable demo was overwritten.

`construction-two-workers` passes 2,980 assertions in 34.032 seconds using
executable SHA256
`9c4684d4bf7654ca0c532a2535ebca83413a47b2c1e843a314eea33751d435f8`.
Two actual rocket trips import eight Steel and eight Goods using real launch
stock and fuel, supplying a second paid Storage job. Builders `18/4` and `19/5`
reach distinct jobs `2/3` and `3/4`, both sharing Core `0/1`. With both eligible,
the lower native ID earns the only 750 J step; the other reports `BuilderBusy`
and cannot borrow its visual receipt. Pause earns nothing. Stopping the first
builder gives the second five steps; resuming the first restores stable priority.

The first builder then visits the Core while retaining a subsequent Construct
instruction. Native evacuation returns its real unconsumed materials; native
removal retires the now-unoccupied target while retaining its two spent work
steps in history. Its old order reports `TargetRemoved`; the second job advances
five further steps. This is actual target retirement, not a forged generation
or deletion of accountable packets. No controller repair was needed.

Rebuild one pair at a time with
`python -B src/tests/multilayer/acceptance/construction_reload_checks.py --stage <queued|materials|power|working> --run <fresh-prefix>`.
Run shared-worker arbitration with
`python -B src/tests/multilayer/acceptance/space_fixes_checks.py --construction workers --save space-module-cancelled-418.sav --run <fresh-worker-id>`.
The original screenshot's personal save remains unavailable; these results
qualify retained native tours and current checkpoints, not an exact historical
reproduction of that image.

## Required follow-up

Group 5 retained native explicit-edit behavior: edits restart the queue and
start the hull. An appended action must not silently replace earlier player
instructions. No command/arrival repair is justified by the observed paths;
the stopped/service/stale-identity/query checks now pass. Group 6 supplies the
read-only readiness view from the existing predicates. Group 7 verified
continuation through current save/reload and two eligible builders.
Group 8 must explain an absent/current-later Construct action, missing actual
materials and other real waits. Do not alter work accounting to make an idle
or unsupplied probe appear productive.

## Rebuildable focused cases

Build `openttd_space_fixes_test`, then run separately:

```powershell
python -B src/tests/multilayer/acceptance/space_fixes_checks.py --construction waiting --run <fresh-wait-id>
python -B src/tests/multilayer/acceptance/space_fixes_checks.py --construction working --save space-module-reserved-418.sav --run <fresh-work-id>
python -B src/tests/multilayer/acceptance/space_fixes_checks.py --construction queue --save space-module-reserved-418.sav --run <fresh-queue-id>
python -B src/tests/multilayer/acceptance/space_fixes_checks.py --construction service --save space-core-service-417.sav --run <fresh-service-id>
python -B src/tests/multilayer/acceptance/space_fixes_checks.py --construction removed --run <fresh-reuse-id>
python -B src/tests/multilayer/acceptance/space_fixes_checks.py --construction delivery --save space-module-cancelled-418.sav --run <fresh-delivery-id>
python -B src/tests/multilayer/acceptance/space_fixes_checks.py --construction interruptions --save space-module-power-418.sav --run <fresh-interruption-id>
# From build/: run only the existing exact construction-energy contract.
.\RelWithDebInfo\openttd_space_fixes_test.exe '[orbital-assembly-contract]'
```

`space_construction_trace.cpp` owns these observations; the common native harness
loads each immutable save through the actual application before invoking it.
Logs are `logs/space-fixes/<run>.log`; exact input/configuration/executable hashes
are in `test-cache/space-fixes/<run>/report.json`. Every process remains capped
at110 seconds. The observations log complete queues, exact incarnations,
approach requests/slots, stopped/crashed/service flags, input packet counts,
Core power and actual work receipts. No source save is rewritten.
