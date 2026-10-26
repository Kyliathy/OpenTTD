# R2SFF1 baseline observations

Execution evidence for [the Space fixes plan](r2sff1-space-fixes-features-1.md),
Group 1. Ownership follows [GUI invariants](../../architecture/gui/archi-gui.md#invariants)
and the plan's native field/Vehicle/packet boundaries. Measurements below are
separate from proposed repairs; pending observations are not passing tests.

## Inputs and limits

- Actual workstation: primary display 1920x1080; working area 1920x1050.
- The original screenshot save and active `build/play.cfg`/`build/hotkeys.cfg`
  are unavailable. The retained config records a 1920x1027 client, automatic
  GUI scale (`-1`) and default font sizes (`0`). The screenshot's approximately
  800-pixel details panel suggests 200% scale; this is an inference, not a read
  of the original process. Reproduce at explicit 200% and compare 100%.
- R1 shows the paused status bar; R3 is cropped above that bar and does not
  establish pause state. Unpause is a prerequisite for observing construction,
  independent of any additional queue/material wait.
- The expected game applications are present. An initial path lookup incorrectly
  reported them absent; explicit directory enumeration and hashing resolved this.

| Input | SHA256 |
| --- | --- |
| `build/RelWithDebInfo/openttd.exe` | `a87898e2cceba6e1802c3de4fbfdcef727c1f0689f9ac8d877da1781cbd36f33` |
| `build/Debug/openttd.exe` | `9365ab97f75c4d849841279f0b572d96310c52528ce28db5b6d2842ce1cb58d7` |
| Entry 23: `space-core-build-415.sav` | `45f71ef532c131b4485e8c015e1e7fad0519e78a534f8d52aced84b17ab165fb` |
| Entry 31: `space-factory-440.sav` | `e063202b825bf16d7fa0b536b482740e1201e0ef29050bc957fbf3301da9a53e` |

Entry 31 has the Core/unfinished Storage/Assembly combination visible in R1.
Entry 23 is the retained in-progress Core control. These retained sources cannot
establish the exact original user's sequence. Read-only source inspection finds
no Alt+4 binding; Debug claims Alt+1/2; `ShowGameOptions` closes and recreates its
window. Retained `hotkeys.cfg` has those old Debug bindings. Neither the client
toggle setting nor its implementation exists yet.

## Rebuildable evidence

The focused `openttd_space_fixes_test` target links the existing engine and only
the active observation/motion fixtures. Build logs belong in `logs/space-fixes/`.
Run `python -B src/tests/multilayer/acceptance/space_fixes_checks.py --run <fresh-id>`
after building that target. Optional `--save`, `--scale`, `--hotkeys fresh` and
`--configuration Debug` select one bounded case; each has a 110-second deadline
and tolerates a minute with no agent activity. Runtime copies, captures and
hash-bound reports are recreated under `test-cache/space-fixes/<fresh-id>/`;
native output is in `logs/space-fixes/<fresh-id>.log`. Inputs are never rewritten.

## Lifecycle assumptions to replace

- `space.cpp::AdvanceAxis` reflects on exact arrival or overshoot of each travel
  envelope boundary; the envelope is the middle half of Y with a two-tile margin,
  and X excludes the two-tile margin. It is not the actual map edge.
- `asteroid.cpp::ValidateAsteroidField` requires the original population, original
  mineral shares and `body.id == vector index`. Disclosure, surveys and direct
  body lookup also treat live count as an ID bound.
- `space.cpp::SpaceSpatialIndex` allocates locations by count and indexes by ID.
- `space_mining_inventory.cpp` narrows body identity into 16-bit `SourceID`;
  packet provenance must be upgraded before retired/replacement IDs become live.
- `asteroid_motion.cpp` deliberately tests reflection, exact-boundary velocity
  reversal and corner reflection. Those requirements will be superseded by
  straight motion, while fractional travel, determinism, conservation and
  bounded pursuit checks remain valuable.

## Measured baseline, 2026-10-23

The authoritative release runs are `baseline-factory-retained-final` (200%,
retained hotkeys) and `baseline-core-fresh-final` (100%, fresh hotkeys). Both
completed in under seven seconds. Their reports bind the exact test executable,
config and source hashes; earlier runs were harness development. In particular,
the initial F2 observer incorrectly looked up window number zero. The real
identity is `GameOptionsWindowNumber::GameOptions` (4); the final runs use it.

| Observation | Measured result |
| --- | --- |
| Construction at 200% | Origin (0,44), minimum/current 1462x1068, client 1920x1027, normal font 20px. |
| Fully/partly beyond client | IDs 25-38: Load RFMT, Load ELEC, Reserve Core materials, Connect, Next berth, Next source, Disconnect, Remove, Evacuate, Next destination, Next model, Next input source, Queue manufacture, Cancel manufacture. IDs 21-24 also overlap the usable status-bar region. |
| Construction at 100% | Origin (0,22), minimum/current 731x534, normal font 10px. |
| Inspector at 200% | Origin (22,72), 1542x1036; also exceeds client height. New controls must not extend this fixed-height pattern. |
| Alt+4, fresh and retained | Surface remains Surface. |
| F2, fresh and retained | First dispatch opens Game Options; second leaves it open. |
| Debug Alt+1/2 | `baseline-debug-retained-200` and `baseline-debug-fresh-200` match global actions 14/15. Release matches neither; no money action was invoked. |

Factory checkpoint: native tick 7046, paused. Builder ID18/generation4 has one
Visit Body0 action at cursor0; ID19/generation5 has completed that Visit at
cursor1. Neither targets construction. Core ID0/generation1 at (4,4) is active,
740 work ticks, consumed Steel16/Goods24, with its 30,000W panel enabled.
Storage ID1/generation2 at (7,4) has work0, Steel0/Goods0 and no assigned builder.
Assembly ID2/generation3 at (3,2) is operational and linked to Core0/generation1.
After 74 ordinary unpaused ticks, tick7120, Storage remains correctly unchanged;
newly manufactured builder ID20/generation6 exists with an empty queue and hold.
All three builders are unstopped and have no active service. This is consistent
with the three idle probes in the report, but does not identify its exact save.

Core control: tick3388, paused; Core ID0/generation1 at (4,4) holds Steel16 and
Goods24 with work37. Builders ID18/generation4 and ID19/generation5 both have
Construct Blueprint0/generation1 at cursor0. After 74 ordinary unpaused ticks,
tick3462, work is109, materials remain reserved and consumed counts remain0/0.
The shared approach/worker owner admits one builder; the second waits. The
72 committed work opportunities are observed behavior, not an assumption that
every elapsed tick must grant construction. Group4 will characterize admission
and the missing instruction/material presentation in detail.

`baseline-reflection.log` passes 41 assertions: at the upper X envelope boundary
one step reflects 222 Q16 units and reverses +16384 to -16384; all four corners
reflect each axis by157 units and reverse both signs. Fractional carries remain
29 and40 respectively. This records the old behavior for intentional replacement,
not acceptance of the newly requested lifecycle.

## Execution reading notes

The runtime handoff's permanent-body/reflection statements are historical policy
explicitly replaced by this plan. Its paid hull, packet, command and service
ownership remains authoritative. Mandatory reading is non-transitive. After a
compaction, read the plan's global block and only the current area's relevant
source sections; do not reload unrelated financial-continuation history.
