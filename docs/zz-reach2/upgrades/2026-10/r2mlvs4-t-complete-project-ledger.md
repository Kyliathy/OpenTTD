# R2MLVS4-T - Complete paid excavation project ledger

Owners: [TBM task 20.1](r2mlvs4-tunnel-boring-machines.md#group-20---recovery-presentation-and-acceptance)
and [master checkpoint 7.8](r2mlvs3mp-master-plan.md).
Recipient: [integrated economics Group 16](r2mlvs15-integrated-acceptance-and-balance.md#group-16---measure-complete-ground-project-economics).
Architecture: [map layers](../../architecture/map-layers/archi-map-layers.md).
Measured: 2026-10-06, Debug test binary at `4d65cbd31d` plus these working changes.

## Declared starting conditions and time basis

The actual native profile/package, generated geology seed 420 and 1980-01-01
calendar policy match the existing visible excavation scenarios. The Surface
is dry land. Deep starts with an explicitly existing excavated Underground
assembly/access floor; the target Deep floor is solid. No target cells are
opened, cargo fabricated, vehicles teleported or phases forced after play starts.

Medium construction/running difficulty, unit price/payment inflation and the
current canonical GRF price modifiers feed `RecomputePrices`. The normal price
owner computes invoices; no second economic table supplies charges. Every
initial portal, line/depot, origin, receiver and hauler/wagon command goes
through the actual command kernel and debits company cash. This differs from
the ordinary workflow's declared pre-owned logistics scenario.

Each project requests the same three-by-nine floor, quotes and pays all 36
swept cells including its starter/working space, and completes nine cuts.
The actual stopped cutter waits 74 native ticks before the player starts it.
Ticks are physical simulation steps, with 74 per economy-day scheduling cycle;
the technology calendar remains fixed. Neither wall-clock test runtime nor
viewport presentation supplies time.

## Observed native invoices

All monetary values are internal base currency units. Setup includes the
receivers' construction expenses even though the ordinary finance owner records
those invoices under Other. Receiver intake fees below subtract those initial
expenses before summing disposal/processing costs.

| Observed quantity | Standard Underground | Harder Deep |
| --- | ---: | ---: |
| Earth / Rock hoppers | 2 / 2 | 2 / 4 |
| Total simulation ticks | 10,620 | 32,926 |
| Economy-day equivalents | 143.51 | 444.95 |
| Cuts / source exchanges | 9 / 2 | 9 / 3 |
| Swept cells | 36 | 36 |
| Setup investment | 2,101,454 | 2,106,196 |
| TBM whole-consist purchase | 403,124 | 804,686 |
| Paid cut/support | 1,080 | 2,880 |
| Observed active / idle controller ticks | 8,593 / 87 | 24,180 / 157 |
| Active / idle operating cost | 38,176 / 39 | 232,758 / 151 |
| Actual haulage operating cost | 952 | 2,964 |
| Receiver intake fees | 1,728 | 4,464 |
| Total cash investment and expense | 2,546,553 | 3,154,099 |
| Remaining cash from 10,000,000 | 7,453,447 | 6,845,901 |
| Earth / Rock delivered | 216 t / 432 t | 72 t / 1,440 t |
| Spoil transport revenue | 0 | 0 |

The active/idle monetary split uses the controller owner's complete observed
duration quote. Its combined integer carry is assigned to the idle component;
their exact sum is the actual native recurring invoice. Daily bucket resets do
not erase measured elapsed durations. The native owners flush the final partial
day before the cash comparison, including the hauler's existing accrued running
ticks. Setup, purchase, cut/support, recurring operation, haulage and intake fees
sum exactly to the cash decrease in both scenarios.

Every generated tonne is at an actual Surface receiver: its stock plus recorded
disposed/crushed quantity matches the job, and all real vehicle packets are
empty. The assets remain owned; no sale proceeds or notional salvage value are
included. This is a measured cash ledger, not a whole-economy payback verdict.

## Focused verification and handoff limits

These figures supersede the first measurements after the Plan 5 real-loader
checks exposed weightless fixture spoil and a missing production cache refresh.
Physical workflows now use the shipped spoil weight/freight properties and
native freight multiplier 1, and every paid cut rebuilds train mass and
acceleration immediately. The corrected focused mass/shaft/economics/workflow
burst passes 139,580 assertions / 14 cases in
[Plan 5 s](r2mlvs5-s-native-mass-and-shaft-checks.log). Both cash ledgers still
balance exactly, with unchanged purchases, swept cells and delivered quantities.

Initial evidence, retained for history:

`[tbm-economics]` passes 26,630 assertions / one case with two generated layer
contexts in under four seconds ([r](r2mlvs4-r-project-cost-checks.log), Debug
test build [q](r2mlvs4-q-project-cost-build.log)).
The economics, ordinary excavation, both editor freight continuations and
script commissioning pass together: 27,534 assertions / four cases in about
11 seconds ([s](r2mlvs4-s-project-and-workflow-checks.log)). No full regression
was run.

These results complete Plan 4 20.1 and master 7.8. Plan 15 receives the actual
inputs while its runner, deliberate receiver-saturation, payback/sensitivity,
performance and final-art tasks remain pending. Recurring underground-bay
exchange and every wait-state file/replay remain master 7.1; a complete paid
source-side loop does not close that scope. No canonical tuning table changed.
