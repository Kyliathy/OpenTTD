# Renewal ledger, deterministic attempts and persistence gate

Owner: [Space fixes plan](r2sff1-space-fixes-features-1.md), Group 11.
Timer ownership: [game-loop architecture](../../architecture/game-loop/archi-game-loop.md#timergametick).

## State and accounting

`AsteroidField::renewal` contains the original ledger: explicit initialization,
cumulative exits by size, pending weighted credit, optional latest size/mineral,
remaining simulation opportunities and both complete two-word RNG streams.
Weights are Small=1, Medium=3, Large=5. They never describe resource reserve,
cargo packets, company cash or another live-body registry.

Pure validation checks each history addition against the total issued body-ID
count before summing. Total exits cannot exceed 32-bit issuance, and weighted
earnings are bounded by five times that count. Credit cannot exceed those
earnings. Latest metadata must match an actually recorded size and a known
resource class. Zero credit disarms the timer; positive credit with zero ticks
represents one due interval. The later field/lifecycle owners additionally join
these counters to actual live identities and prevent retiring an old ID twice.

`ApplyAsteroidExitCredit` takes a bounded copied terminal batch, rejects duplicate
or unissued IDs, and processes ascending IDs. The highest ID in that batch gives
the latest size/class. A subsequent tick may legitimately make a lower old ID
the latest exit. The field owner must preflight that every event still describes
a live body; the ledger deliberately retains no per-retired-ID tombstone set.

## Timer and candidate streams

The initializer uses saved field seed domains 4 and 5 for timer and candidate
streams. Existing generation domains 1/2/3 and their literal accepted body
records remain unchanged. Default zero-initialized storage is explicit absence,
not initialization. No helper draws from global or interactive RNG.

Timer bounds are rounded inward from 20,000-60,000 milliseconds using
`MILLISECONDS_PER_TICK`: 741-2222 opportunities at the current 27 ms interval.
First credit arms once. Additional exits preserve the existing deadline,
including a due interval. Ordinary countdown calls consume no randomness. The
live owner in Group 29 must skip countdown on the same opportunity that first
arms the timer; only following opportunities count toward the sampled delay.
Paused frames invoke no countdown, and fast-forward supplies native opportunities
faster without changing the sampled tick count.

`BeginAsteroidRenewalAttempt` is a pure proposal from positive due state. It
copies the candidate RNG and draws exactly one ticket, even if no different
affordable size exists. Stable Small/Medium/Large order maps that ticket to an
eligible size. Group 27 must sample mineral, reserve, entry and heading using a
ticket-local generator, rather than consuming extra saved candidate draws.

`FinishAsteroidRenewalAttempt` verifies that exact proposal against its original
due state. It publishes the one candidate draw, debits only a genuinely admitted
creation, and samples a fresh independent timer whenever credit remains. A failed
attempt retains every credit. Zero remaining credit disarms without a timer
draw. The next state cannot finish the old proposal again while its timer is
armed. An exhausted ID cursor permits failure/retry, never a successful debit.
The later atomic field owner publishes one such result per due interval; pure
repeated queries do not themselves consume RNG or create a body.

## Pristine bridge before the new wire format

New fields are initialized during generation. Installation of canonical old
absence seeds once from the saved feature seed, without guessing past exits or
rerolling anything. For now, native field validation accepts only that pristine
seeded ledger or canonical pre-installation absence. Evolved detached ledgers
used by the pure tests cannot be installed in a live field while the old ASTR
writer lacks their fields.

Group 22 must replace this explicit pristine-only gate with the full versioned
renewal validation and persist both words of both streams alongside all counters,
latest metadata, timer and issuance cursor. Until then the pristine stream state
is completely determined by the already-hashed field seed; no active renewal
transition or independent unsaved RNG history is published. Group 28/29 still
owns real retirement, creation and tick integration.

## Verification status

The focused RelWithDebInfo build passed. `[asteroid-renewal]` passed 25,313
assertions across five cases in less than one second. `asteroid_renewal.cpp` covers the delayed
5→2→1→0 credit sequence, Small-only unaffordability, successful/failed attempt
stream independence, stable batch ordering, stale proposals, counter overflow,
ID exhaustion, exact tick bounds and the old-install/pristine-writer gate.
Logs belong under `logs/space-fixes/`; run only `[asteroid-renewal]` from the
focused native test executable. A retained demo load also checks that active
legacy fields have initialized their contained renewal owner before observation.

That actual retained `space-factory-440.sav` load passed in 4.281 seconds,
including the native construction waiting observation. Evidence:
`logs/space-fixes/asteroid-renewal.log`,
`logs/space-fixes/renewal-legacy-load.log` and
`test-cache/space-fixes/renewal-legacy-load/report.json`.
The tested executable SHA256 was
`c50f9e165ce45fc5c8a8e9d3d684b9b3aa1b7e90d695f59983c358b94cce84d2`.
No live retirement, replacement or timer advancement is enabled by this group.
