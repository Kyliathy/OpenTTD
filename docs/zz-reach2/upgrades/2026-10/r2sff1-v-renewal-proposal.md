# Bounded asteroid renewal proposal

Owner: [Space fixes plan](r2sff1-space-fixes-features-1.md), Group27.

## Producer boundary

`asteroid_renewal.h/.cpp` exposes `PlanAsteroidRenewal` and a named result with
Group11's exact copied attempt/resulting candidate stream. The saved stream
advances by one ticket only when commit later closes a due attempt, including
failure. Size selection already belongs to that ticket. Ineligible sizes cause
no local body-generation draws. This resolves the original27.1 wording without
changing Group11's delivered/tested contract.

An eligible ticket seeds one local generator. Exactly six local draws supply
canonical weighted class, inclusive finite reserve, heading, speed, inward-side
choice and coordinate. The fixed alternatives try eight separated coordinates
per inward side, at most16; occupancy failure draws nothing further. A corner
must enter both touched edges strictly inward. Original static swept geometry,
body envelopes and bounded actual hull-centre snapshots decide clearance.
Malformed field/bounds/hull snapshots refuse. No native pool is consulted.

Ready descriptions have an explicit invalid/unissued body ID, full new finite
reserve, zero movement remainders and empty survey/mining leases. They create
no knowledge. Proposal inspection never changes either saved stream, global
RNG, an original reserve, issuance or credit. The latest exit contributes only
its size exclusion; neither its old point nor its mineral is inherited.

Group28 must obtain original native geometry/hull snapshots, revalidate the
proposal and atomically admit its original field/index/ledger changes. Capacity
and issuance remain commit gates. This producer has no insertion, debit, timer
publication or normal-loop caller. No new saved fields/version are needed.

## Verification

RelWithDebInfo and Debug each pass **33,263 assertions in13 cases** with
`[asteroid-renewal-proposal],[asteroid-renewal],[asteroid-field-wire] --warn NoTests`.
Logs: `logs/space-fixes/renewal-proposal-gate-{release|debug}.log`.
Four new cases cover every reachable eligible-size mask (`0,1,2,3,5,6`), latest
size exclusion, zero/disarmed input, exact copied ticket, deterministic replay,
all three minerals/all24 canonical velocity pairs, four entry sides and corners,
a64x256 empty live field, occupied alternatives, complete placement refusal and
the independent rearming/failed-credit contract. Original field equality and
the four global RNG words remain unchanged by inspection.

Full-word checkpoint seeds provide motion coverage: consecutive small raw seeds
exercise only12 velocity pairs because of the original RNG's seed/rotation
structure. The fixed checkpoint spread covers all24 without weakening checks
or adding production draws. Corner enumeration is a finite test driver; the
production planner retains its16-alternative maximum.

Test fields declare canonical content and exact funded exit/known Small-birth
history to represent every credit threshold. They retain credit conservation,
known live costs and an actual retired original before issuance. These are
component inputs, not executed spawns, elapsed waits, native cargo or paid-world
events. Supported narrow-map and empty-field cases mint no minimum population.

Both ordinary/focused test source lists register `asteroid_renewal_proposal.cpp`.
Rebuild target `openttd_space_fixes_test`, with configurations sequentially.
Build logs: `logs/space-fixes/build-renewal-proposal-final-release.log` and
`build-renewal-proposal-debug.log`. CRLF-aware `git diff --check` passes.

| Configuration | Focused executable SHA256 |
| --- | --- |
| RelWithDebInfo | `e9b2ee5d99a727ba14396c3d34832e30d53b522d932dd5a979929362ccb431fa` |
| Debug | `879ff601a656a4acd73ac65dfea24a820d719cfa096bae0ef5606690785fb48f` |
