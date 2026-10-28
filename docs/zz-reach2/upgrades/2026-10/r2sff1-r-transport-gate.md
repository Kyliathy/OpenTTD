# Joined Space transport producer gate

Owner: [Space fixes plan](r2sff1-space-fixes-features-1.md), Group24.
Prerequisites: [cargo](r2sff1-o-cargo-transport.md),
[field](r2sff1-p-field-transport.md), [mission](r2sff1-q-mining-transport.md).

## Certified boundary

Groups21-24's **441 cargo origin,442 field lifecycle and443 policy/continuation**
readers pass the focused joined gate in RelWithDebInfo and Debug. Future groups
may build actual safety release/event transactions using these owners. This
gate enables no retirement, creation, timer tick or normal-loop lifecycle writer.
Real event save/reload remains Group31 after Groups25-29 integrate it.

Full-load audit confirms `AfterLoadGame` accepts ASTR before legacy packet
qualification, then accepts launch owners, SPOR and SPKN. Pods, manifests,
transitions, paid production, cargo dimensions, descents and structures follow.
Final `ValidateSpaceVehicles` joins native queues, leases, all physical custody
and original owners only after these dependencies exist. Installing the copied
queue early checks actual engine roles; it does not select targets or work.
No normalization was moved ahead of its required original owners.

Preview completes chunk decoding before candidate validation. Current mission
checks consume only copied field/role/manifest/cargo definitions and generation
cursors. Old packet qualification stays after same-file custody/mineral joins.
All transient identity, occupied-lease, generation and old-origin receipts clear
between candidates. Native owners and the four global RNG words remain separate
from this copied state; complete field equality also covers its four RNG words.

## Refinement found while joining

The former aggregate renewal bounds could accept an underfunded known live Large
replacement, overcharge a known live Small, or claim renewal history while every
original body still survived. Such a history cannot earn its initial credit.
`ValidateAsteroidField` now counts surviving original IDs and the actual1/3/5
costs of known live new IDs. Only retired replacements retain an unknown cost;
their remaining historical cost is bounded separately. New issuance requires
at least one original terminal exit. Pure and actual native refusals cover these
conditions. No genuine original fixed-population save or earlier live writer
could emit these impossible replacement histories.

The old Group22 sparse artifacts that preserved every original plus a new Large
are **historical transport-only artifacts**, superseded by this stricter gate.
Current sparse fixture starts with an unleased original Medium terminal input
and declares alternating Small/Medium history, ending with one fully paid Small
at ID70000 and two credits. It retains all original paid hulls and packets.
The retired-origin variant additionally removes original Large0 and gains its
five credits without postponing the existing timer. Its original natural packets
keep typed body0 history, and SPKN70000 joins the surviving new live identity.
Both are declared component inputs, not executed69,991 spawns, paid procurement
or earned scans. No replay/chronology claim is made for aggregate fixtures.

## Verified results

Final focused executable hashes:

| Configuration | SHA256 |
| --- | --- |
| RelWithDebInfo | `abca5a3079486ce364970583f4433ea70bf1f2541597478c8d25e8956774021c` |
| Debug | `94f443df6ef0757c27212c64fe2d114df614caa140a61bb818d5ff61c389db94` |

Build logs: `logs/space-fixes/build-credit-gate-release-final.log` and
`build-credit-gate-debug-final.log`.

Focused tags `[asteroid-field-wire],[asteroid-live-validation],`
`[asteroid-cargo-wire],[space-mining-wire],[space-mining-continuation]` pass
**17,211 assertions in25 cases in each configuration**. Logs:
`transport-credit-gate-release-final.log`, `transport-credit-gate-debug-final.log`.
They include pre-departure, armed/disarmed/due/failed renewal and waiting/effective
mission component checkpoints. Complete bytes/state, next timer and next attempt
agree after round-trip; those pure checks create no native lifecycle event.

Seven faults pass paired full-load refusal and valid-corrupt-valid preview in
each configuration: missing source mineral field, knowledge of removed body0,
wrong effective mission, missing candidate companies, missing current schema
with zero Space hulls, underfunded known replacement and impossible bootstrap
history. **All28 native refusal checks pass**, each process under5.3 seconds.
Every preview keeps a semantically different actual440 factory world open.
That world has company0 and live body0; it cannot supply either missing candidate
identity. Candidate role/mission data, high body70000 and original packet history
must come from the candidate. Snapshots compare fields, companies, queued/effective
intent, leases, complete sources, cargo meanings and role/home metadata after
each preview and a final return to the original source's preview.

Native purity checks compare original packet IDs/amounts/accounting fields,
complete field/renewal, eight Space family digests, clock, global RNG and actual
native pool counts. Logs under `logs/space-fixes/` are
`gate-<release|debug>-<case>-<preview|full>.log`; release mission uses
`mission-fixed`, and the zero-hull case uses `empty-schema`. Full refusals always
tear down their own separate disposable application.

Five accepted inputs load, preview and resave in each configuration: actual old
Classic, actual no-field world, ownerless404 survey preview, actual405 mining,
and current funded sparse/retired-source component. **All10 accepted runs pass**,
each under4.1 seconds. Logs: `gate-<release|debug>-accept-<name>.log`, where names
are `classic`, `no-field`, `preview-404`, `mining-405`, `retired-current`.
The404 surveys remain ownerless; migration creates no native hull or cargo.
Independent original-schema comparisons confirm original mission/stop/work,
queue/incarnation and field bytes, plus full current lifecycle trailer/RNG.
All19 original405 packets and82 units survive, including18 natural packets and
one ordinary packet. Retired body0 never aliases live vector slot0/body1 or the
new live body70000. No live-body tombstone is installed.

## Rebuilding evidence

Long-term tooling stays under `src/tests/multilayer/acceptance/`. Runtime copies
and JSON reports stay under `test-cache/space-fixes/`; logs stay under
`logs/space-fixes/`. Every native process retains the110-second harness deadline.
Use fresh run names; an outer driver log must have a different basename from
the native harness's own `<run>.log` to avoid Windows file-handle conflicts.

1. `space_fixes_checks.py --save space-mining-holds-405.sav --cargo-origin
   --run <base>` produces the original-current base.
2. `asteroid_field_wire_checks.py --source <base>/cargo-origins-current.sav
   --case sparse --run <sparse>` rebuilds the funded aggregate fixture.
3. Its `--case retired-origin` with the accepted sparse resave produces the
   combined high-ID/retired-cargo candidate. It requires no active lease on body0.
4. For mission candidates, load/resave actual `space-mining-working-405.sav`,
   then run `space_mining_wire_checks.py --case pinned` on that resave.
5. Load/resave `src/tests/fixtures/content/r2mlvs1-i-normal-classic.sav` for the
   zero-hull current schema case. The no-field source is its `normal-native`
   sibling. Original inputs remain unchanged.
6. `space_transport_gate_checks.py --candidate <current save> --open-world
   src/tests/fixtures/demos/space/space-factory-440.sav --case <case>
   --configuration <configuration> --run <fresh-name>` rebuilds one paired
   refusal. Cases: `source`, `field`, `company`, `mission`, `zero-hull-schema`,
   `known-cost`, `bootstrap`. Mission uses the pinned candidate; bootstrap uses
   the original-current working-miner base; other field/source cases use the
   funded retired candidate. The zero-hull case uses the Classic resave.

The harness's `--preview-source <other save>` mode can also check an independent
valid candidate alone while preserving the actual open world. Read-only field,
packet and mission comparison modes remain documented in notes p/q. Unit tests
remain focused on current producers; no full regression or separate financial
trial was run. The normal game executable still belongs to final Group34's build.
