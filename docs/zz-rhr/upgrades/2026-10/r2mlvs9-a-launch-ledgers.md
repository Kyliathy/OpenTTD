# R2MLVS9 - Reusable rocket and persistent probe ledgers

Owner: [launch centers and rockets](../../../zz-reach2/upgrades/2026-10/r2mlvs9-launch-centers-and-rockets.md), Group9.
Numerical authority: [Plan1 Space revision2 budget](r2mlvs1-e-ground-and-space-budgets.md#space-economy-revision---2026-10-12),
`src/multilayer_space_content.h/.cpp` and `src/multilayer_bootstrap.h/.cpp`.
Focused numerical fixture: `src/tests/launch_economics.cpp`, `[launch-economics]`.
Actual ownership evidence: assembly/flight/descent/service reports indexed by
`logs/space-survey-verification.json`, paid repeat/resupply reports indexed by
`logs/space-resupply-verification.json`, and the two construction-probe deliveries
indexed by `logs/space-construction-verification.json`.

All money is uninflated internal Money. Goods is priced per crate and weighs
eight sixteenths of a tonne; Steel weighs sixteen. Numerical scenarios use
uninterrupted work. Travel/wait upkeep, clearing, conventional freight and
unlisted facility inputs need their actual route ledger. Native transfer,
launch, descent and hub handling create no freight income.

## One rocket, two flights

The center costs500,000 plus native clearing. Its empty facility grants no
stock. The rocket consumes STEL12 + GOOD16 once (3,120 in published seed inputs)
and charges400,000 capital once. Its20t dry hull and separate40kL fuel
compartment do not reduce its30t payload allowance.

| Charge/state | First round trip | Second round trip with the same hull |
| --- | ---: | ---: |
| New rocket hull capital | 400,000 | 0 |
| New rocket hull materials | 3,120 | 0 |
| Fuel purchased | 16kL /4,000 | 14kL /3,500 |
| Fuel burned | 10kL outbound +5kL recovery | 10kL outbound +5kL recovery |
| Fuel retained after return | 1kL | 0kL |
| Accepted mission fee | 5,000 | 5,000 |
| Pad upkeep,148 preparation +74 turnaround ticks | 1,500 | 1,500 |
| Flight operating cash | 10,500 | 10,000 |

Total rocket capital, embodied materials and these two operating windows:
**423,620**, before the center and payload. Exact15kL procurement instead
costs10,250 per flight; the two-flight total is identical. Arrival needs one
simulation dispatch tick in addition to nominal phase work; blocked approaches
may add travel or waiting, so nominal666 ticks is not an elapsed-time promise.

The pad remains reserved throughout the round trip, including its222-tick
outbound and return legs; upkeep is charged only during preparation/turnaround.
The mission fee commits when preparation is accepted. Cancellation during
preparation retains cargo and unburned fuel; the fee and elapsed pad work remain
paid. After outbound burn, cancellation follows the paid recovery leg. Recovery
returns the same Vehicle/generation and paid construction receipt. A genuinely
new replacement hull requires its own400,000 and STEL12/GOOD16; reuse does not.

## One deployed mining probe, two cargo returns and service

The mining probe consumes STEL4/GOOD8 (1,360 inputs) and100,000 capital. Two
paid pod shells consume STEL2/GOOD4 (680 inputs) and4,000 shell capital.
RFMT1/ELEC1 service preload costs680. Total first payload inputs cost2,720;
body/pod capital is104,000. Payload mass is8t body +4t shells +2t service
=14/30t. The rocket deploys that paid native probe once; each return uses one
separately paid passive shell. The probe remains orbital after both descents.

| Two-return operating item | Money |
| --- | ---: |
| Initial16kL fuel purchase,1kL retained | 4,000 |
| Two shell material recipes | 680 |
| Two shell assembly capital charges | 4,000 |
| RFMT1/ELEC1 service preload | 680 |
| Forty tonnes mined in20 complete74-tick batches | 10,000 |
| One accepted rocket mission | 5,000 |
| Rocket preparation/turnaround pad work | 1,500 |
| Forty tonnes of Surface collection handling | 40 |
| Historical purchase/mining/handling subtotal | **25,900** |
| Actual148-tick onboard service at500/day | **1,000** |
| Subtotal including completed service work | **26,900** |

The earlier25,900 figure omitted service upkeep. It remains useful as the
purchase/mining/handling subtotal, but completed onboard service costs another
1,000. Travel/wait upkeep, the probe body/materials and center/rocket purchases
are additional. Per-tick remainders preserve those charges across interruption;
the native descent/service fixture checks all148 service ticks.

The subtotal already includes4,000 of shell capital and680 of shell materials,
plus680 of service parts. Do not add the104,000 payload-capital or2,720 payload-input
rows wholesale to it. Combining all explicit native purchases once gives
500,000 center +400,000 rocket +3,120 rocket inputs +100,000 probe +1,360 probe
inputs +26,900 operating/service =**1,031,380**, before clearing, route expenses
and travel/wait upkeep. The center still requires freight delivery of every
subsequent stock item; its construction grants no free supply.

Full20t freight holds block further extraction. After two completed descents,
the saved quota is exhausted; service requires actual onboard RFMT/ELEC packets
and148 work ticks. A missing part, active descent or paused service blocks
renewal. Both spent shells must also be replaced for two more descents, at2,340
each including materials and shell capital. Supply them to the existing probe
through a paid resupply round trip using the same rocket. This buys neither a
second mining probe nor a second copy of the mined cargo. The native414 repeat
fixture verifies a third paid mined return, interrupted service and lost/reused
receiver refusal; it complements this numerical budget.

## Two material launches for the first Core

Commission two construction probes at100,000 and STEL4/GOOD8 each. Keep the
same rocket hull for both flights. The first probe carries16 Steel; the second
carries24 Goods crates. Their separate RFMT1/ELEC1 preloads remain owned stock.

| Payload | Builder dry hull | Service preload | Construction freight | Total |
| --- | ---: | ---: | ---: | ---: |
| Steel delivery | 8t | 2t | 16t | 26/30t |
| Goods delivery | 8t | 2t | 12t /24 crates | 22/30t |

Each probe's freight is within its20t hold. The Goods limit is40 current crates,
not20 crates. Both vehicles persist after delivery. The blueprint receives the
actual Steel/Goods packets without needing a prior orbital station. Core
capital, construction and activation remain separate requirements; a launch
manifest never grants a preassembled Core.

| First-Core project item | Money |
| --- | ---: |
| Two builder hull capital charges | 200,000 |
| Two builder material recipes | 2,720 |
| Two service preloads | 1,360 |
| Core STEL16/GOOD24 freight | 4,480 |
| Core capital | 700,000 |
| Thirty kL of fuel across two flights | 7,500 |
| Two accepted mission fees | 10,000 |
| Two pad preparation/turnaround windows | 3,000 |
| Project subtotal | **929,060** |

The subtotal excludes the existing center/rocket, freight travel/waiting and
construction-probe work upkeep. Nominal sequential work is296 builder assembly
+1,332 rocket phase ticks +740 construction +74 activation =2,442 ticks,
excluding initial rocket assembly and dispatch/approach overhead. This is a
budget for the completed project. Current415 native construction evidence proves
both paid deliveries and interrupted first-frame work; it does **not** yet
prove completed Core activation or orbital manufacturing.

## Validation boundary

`[launch-economics]` checks public recipe prices, separated hull/fuel/payload
budgets, retained fuel, operating subtotals and both first-Core manifests. The
compile-only `src/tests/launch_contract.cpp` guards the generation-qualified
public transfer signatures and sole packet-owner types. Actual native producers
cover capacity exhaustion, unavailable service, same-hull reuse, repeated release,
interruption and cargo conservation. These focused checks do not close final
art, complete orbital facility integration, multiplayer or full-plan acceptance.
