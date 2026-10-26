# Actual full-hold automatic returns

Owner: [Space fixes plan](r2sff1-space-fixes-features-1.md),31B.3.
This builds on [original role/partial return evidence](r2sff1-z-e-mining-roles.md).
Implicit Surface descent31B.4 and remaining presentation/replay31B.5 are recorded
in [Surface/descent evidence](r2sff1-z-g-surface-return.md); this receipt covers
full-hold module handling.

Both current configurations pass these original native actors:

| Role | Actual full holding | Actual ordinary opportunities | Assertions |
| --- | --- | --- | --- |
| Original15 |20 units|1,383|8,398|
| Original21 |40 units|1,387|8,426|

Each input is the actual second-source `policy-retarget.sav` from the preceding
partial scenario, with one genuine original-source batch and an existing
explicit module Unload route. This trace assigns no new geometry, cargo,
reserve, clock, instruction, quota or pose. Its predecessor's edge geometry was
explicitly declared, while attachment/extraction/retargeting were actual native
work. Nine genuine second-source batches fill the remaining holding. The
original body's finite reserve debit and lifetime extraction increase equal
precisely the added18/36 units. Full Load policy remains saved throughout;
there is no ReturnNearest click to force the result.

Original continuation releases its native lease, safely departs, selects an
admitted nearest original Structure, travels to its actual Core0/berth0 and
invokes the real physical packet transfer. Whole immutable Source quantities
from both bodies enter the original stock once. Original15 keeps its native
module allowance semantics;21 spends exactly one genuine positive return.
Both retain the original mass-derived cargo capacity and complete the original
Mine/Unload cursor. A still-live source is not artificially retired merely
because its miner has returned.

Four actual phases are saved: second-source attachment, newly full holding,
receiver travel and completed unload. Separate current applications match the
uninterrupted next ordinary opportunity's13 owner observations at every phase.
Another no-op load/preview/save application per phase preserves complete CPP,
mission/stop/intent/queue and ASTR lifecycle/RNG state. All four next-step and
all four transport applications pass for each role in both configurations;
every next-step observer has63 assertions. Current transport remains445.

The initial full-hold capture showed another status-only defect: native
extraction had released the full lease and published Return, but saved
SeekMineral intent persists until the next controller opportunity. The inspector
incorrectly said Seeking at40/40. It now shows Detaching safely for that actual
saved transition. The correction changes no simulation owner; capture checks
retain all13 observations. Reviewed new full-hold capture shows Full Load still
selected,40/40 cargo, zero next work and Detaching safely. Earlier Mining status
after attachment remains correct.

Current builds include focused native, ordinary game and ordinary test targets
in both configurations. Logs are `build-role-full-status-{release|debug}.log`
and the separate `build-role-full-ordinary-test-release.log`. Only changed
objects and their consumers were rebuilt; no full regression was run.

Actual traces:

```powershell
python -B src/tests/multilayer/acceptance/space_fixes_checks.py --source test-cache/space-fixes/role-light-partial-release/policy-retarget.sav --mining-controls full-light --run role-full-status-light-release
python -B src/tests/multilayer/acceptance/space_fixes_checks.py --source test-cache/space-fixes/role-heavy-preserved-release/policy-retarget.sav --mining-controls full --run role-full-status-heavy-release
```

Debug uses `--configuration Debug`, source
`role-light-partial-debug/policy-retarget.sav` or
`policy-module-route-debug/policy-retarget.sav`, and run names
`role-full-{light|heavy}-debug`. The inherited heavy source is a genuine earlier
accepted native phase; no source bytes were altered for these tests.

Actual replay and complete no-op transport:

```powershell
python -B src/tests/multilayer/acceptance/space_policy_replay_checks.py --source test-cache/space-fixes/role-full-status-light-release --role 15 --full-hold --run role-full-replay-light-release
python -B src/tests/multilayer/acceptance/space_policy_replay_checks.py --source test-cache/space-fixes/role-full-status-heavy-release --full-hold --run role-full-replay-heavy-release
```

Debug uses the corresponding actual `role-full-{light|heavy}-debug` source,
`--configuration Debug` and `role-full-replay-{light|heavy}-debug` run. Every run
identity must be fresh. Reports bind actual source/configuration/executable
hashes; runtime/saves/original-pixel captures live under
`test-cache/space-fixes/<run>/`, native/driver logs under
`logs/space-fixes/<run>{|-driver}.log`. Aggregate transport receipts append
`-transport.json`. Lossless PPM-to-PNG conversion follows prior evidence.

The first `role-full-{light|heavy}-release` traces passed all native simulation
checks but predate the corrected status caption. Retain their captures to show
that discovered presentation defect; use `role-full-status-*-release` as current
visual evidence. Scoped whitespace check is `role-full-whitespace.log`.
