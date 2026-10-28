# Mining policy and continuation transport: execution notes

Owner: [Space fixes plan](r2sff1-space-fixes-features-1.md), Group23.
Prerequisites: [native continuation](r2sff1-n-mining-continuation.md) and
[field transport](r2sff1-p-field-transport.md).

## Delivered contract

Save version **443 / SpaceMiningContinuation** extends original SPOR orders
from14 to15 bytes, adding one policy byte. Older versions retain their exact
row formulas and default to LegacyReturn. Queue target/cursor, incarnation,
wait, partial work and shared approach remain original owners.

The original VEHS Space description and independent copied description share
14 explicit continuation scalars, totaling40 bytes per Space row: phase,
mineral U32, prior body U32, effective-target presence/kind/ID/generation,
departure presence/x/y, retry, waiting reason, route cursor and travel budget.
`saveload/space_mining_sl.*` adapts optional/label values without copying host
optional memory. Its per-row transport resets before every full/copied load.
Current fields are mandatory/unique even when no Space rows exist; genuine old
tables forbid future continuation keys, including generic skipped fields.
Unknown enums, invalid option flags, hidden absent-option values and excessive
retry/route/travel fail before gameplay reconstruction.

`space_mining_state.h` holds lightweight discriminators used by the same native
`SpaceMission` and copied preview. It creates no additional mission owner.
`SpaceMiningContinuationContext` and `ValidateSpaceMiningContinuation` share
native/candidate phase, original queue, pinned mineral, effective target,
physical departure and issued-history admission. Native facts come from actual
owners; copied facts come only from this candidate's identities, configured
cargo meanings, paid role, manifest and generation cursors. The pure overload
of `GetSpaceReturnRoute` preserves the same original Visit/handling boundary.

ASTR preview now retains bounded occupied-lease metadata. Current preview checks
both directions against actual copied native Mining rows, rejecting orphaned,
duplicate or mismatched leases. Active pursuit and partial Survey still require
their real surviving body. Detached/Waiting original targets may remain issued
history. Started replacement mining uses its effective body; a pin is retained
even with an empty hold. No instruction may name a future body ID.

Copied LCTR now retains its actual issuance cursor, including retired centers.
Effective receiver history may remain inert after removal; an unissued future
incarnation cannot borrow a later receiver. Candidate cleanup clears both this
cursor and occupied body leases. Controller arrival revalidation still owns
real capacity, ownership, berth and handling rights.

After reader checks passed, `SetSpaceMiningPolicy` was appended to the original
command enum and registered as VehicleManagement. Existing IDs and layouts stay
intact. Its14-byte argument layout retains native VehicleID, U64 incarnation,
queue index and policy. UI controls remain Group30. Existing vehicle/order
digests include every contained decision and configured per-Mine policy; inert
legacy state retains its previous digest stream.

## Verified results

Final RelWithDebInfo build: `logs/space-fixes/build-mining-wire-final.log`.
Executable SHA256:
`c0956d9838925316c3dc61da5787aa4649aa95c67364f16a3baef88ad13c0392`.
Focused tags `[space-mining-wire],[space-mining-policy],`
`[space-mining-continuation],[space-return-target]` passed
**23,886 assertions in20 cases** under one second. Log:
`logs/space-fixes/mining-wire-registered-final.log`.

Components cover every continuation phase, canonical absence, active mineral
pinning with an empty hold, wrong/missing candidate mineral definitions, future
original/prior/effective body IDs, unsupported role, stale/future receiver
generation, removed original target with a real replacement, safe waits,
orphaned leases, queue clearing and all explicit scalar/schema bounds. These
are declared component checkpoints, not paid-world departure evidence.

The actual405 mining,440 factory and442 native files passed full load, preview
and443 resave. Logs under `logs/space-fixes/`:
`mining-wire-legacy-405.log`, `mining-wire-old-orders-440.log` and
`mining-wire-old-orders-442.log`. Independent original-schema comparisons prove
all mission/pose/work/stop/extraction fields persist. All old SPOR queue bytes,
incarnations, cursor, approach and14-byte targets persist, adding LegacyReturn
only. The genuine pre-SPOR405 file receives inert empty queues and the existing
`VehicleID+1` migration, without issuing new work or changing native cargo.

A declared pinned-policy native fixture retains the original actual405 hull,
Mining work/lease, pose and cargo while adding explicit current intent. It
passed full load, preview and native resave:
`logs/space-fixes/mining-wire-pinned-accepted.log`. This is transport evidence;
it does not claim a player issued that fixture's initial order.

Thirteen malformed cases passed both native preview and full refusal:
missing mineral field; future keys in genuine442; unknown phase; invalid
presence; hidden departure; excessive retry/route/travel; wrong mineral;
future prior body; unknown policy; reverse orphaned lease; wrong effective
target. All26 native checks passed before command registration with executable
`ecaf5eff645528af0aec9a138bbf4cc41b1562a4501806bd5da81ac56308c6d5`.
Logs: `logs/space-fixes/mining-wire-<case>-preview.log` and `-full.log`.
The final executable repeats missing-schema and genuine-old future-payload
refusals after the population-independent header check: logs
`mining-wire-final-schema-<mode>.log`, `mining-wire-final-old-<mode>.log`.

The actual registered player command also passed in the retained native game:
stale incarnation refused; exact current Mine changed FullLoadNearest to
ReturnNearest; original work, pose, stopped choice, pin, cargo and all other
owners persisted. It saved and reloaded with exact copied queue/intent values:
`mining-policy-native-command.log` (**135 assertions**) and
`mining-policy-native-reload.log`. Independent comparison proves exactly one
SPOR policy byte changed, with every original target/cursor/generation and all
mission/stop/continuation scalars unchanged. Native marker:
`MINING_POLICY_EDIT stale_refused=true original_work_preserved=true`.

## Rebuilding evidence and next gate

Long-term tooling is beside tests; artifacts stay under `test-cache/space-fixes/`
and logs under `logs/space-fixes/`. The native harness keeps a110-second deadline.
Run `space_fixes_checks.py --source <repo save> --cargo-origin --run <fresh-name>`
for native source/queue/field checks. Add `--mining-policy` only for an existing
current Mine to exercise the actual command before native resave.
`space_mining_wire_checks.py --source <current mining save> --case <case>
--run <fresh-name>` rebuilds one declared checkpoint or bounded refusal pair;
old-payload requires a genuine442 source. Its read-only `--target <native save>`
mode compares original missions, stop flags, queues and intent; `--policy-edit`
expects exactly one actual ReturnNearest edit.

Group24 still owns different-open-world preview isolation, combined migration
and the producer gate in both configurations. Actual pre-exit release, native
retirement/spawn and ordinary loop activation remain Groups25-29. No exit or
removal writer was enabled by this group.
