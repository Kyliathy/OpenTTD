# Native receiver selection: execution notes

Owner: [Space fixes plan](r2sff1-space-fixes-features-1.md), Group 19.
Prerequisites: [policy](r2sff1-k-mining-policy.md) and
[native mining targets](r2sff1-l-mining-targets.md).

## Delivered work

Group 19 is complete. Execution stopped at the user's request before Group 20.
`space_return_target.h/.cpp` have been added and registered in `src/CMakeLists.txt`.
Route, navigation admission, nearest selection and arrival execution are now
implemented and verified. Final focused build evidence is
`logs/space-fixes/build-space-return-final.log`. The shared mining fixture and
both original intercept cases passed with the receiver cases below.

Current exports are `GetSpaceReturnCandidateAdmission`,
`FindNearestEligibleSpaceReturn` and `ExecuteSpaceReturnArrival`. The original
providers now share their preflights with `CanNavigateOrbitalCargoRelease`,
`CanNavigateOrbitalFreightUnload`, `CanNavigateOrbitalService` and
`CanNavigateSpaceOnboardService`. Existing commands still run arrived admission
and their original real commit/work owners. No copied Vehicle is repositioned
or forced into Holding to make a quote succeed.

The selector considers the complete permitted native owner family and ranks
exact squared distance, target kind, ID and generation. A module uses its
canonical approach anchor for ranking; real Dock slot admission remains with
the shared owner. Surface batches use actual min(40, held) units and choose an
actually admitted carried pod by stable ID/generation. Module returns offer
the whole actual hold. Empty returns carry quantity0 and spend no shell/quota;
they still require real owned destinations and actual arrival. Automatic service
requires genuinely exhausted allowance and no pending returns, so missing pods
cannot trigger repeated unnecessary service bills.

`ExecuteSpaceReturnArrival` refreshes route/admission, then calls the original
descent commit, module exchange command or service command with the real actor.
It changes no position, mission phase or queue cursor. Positive Surface freight
retains its drop circle; empty Surface returns and center/onboard service reach
the exact anchor. Service starts never count as service completion.

The current route query reads the original current Mine policy. It records the
first subsequent Unload or ReturnService domain and exact target, plus the
original intervening Visit range. Another work action is a boundary, not
permission to skip to a later receiver. Selection itself must complete no
queue entry. LegacyReturn stays exact; configured policies may search compatible
owned facilities within that allowed domain. With no explicit directive, small
miners use Surface recovery; a heavy requires actual carried shells for an
implicit Surface route, otherwise it needs its explicit module route. Nearest
may find another owned real center after home loss; legacy coordinate-only home
return remains with its existing controller and cannot invent a receiver.

## Native facts inspected

- `CanReleaseOrbitalCargo` in `space_descent.cpp` checks actual craft/center/pod
  generations, same ownership, Orbital placement, supported15/20/21 role,
  manifest/service/remaining-versus-pending quota, exact unique carried pod,
  compatible cargo, positive quantity <=40, weight/mass, monotonic descent ID,
  recovery job cap, actual receiving stock headroom and current drop circle.
  Heavy additionally keeps its physical freight-capacity bound. The final
  `HubCargoTransfer::CanMove` preflights real fragments. Factor its geometry
  requirement without changing these paid/native admission rules.
- `CmdMoveOrbitalFreight` in `space_structure.cpp` owns real module exchange.
  Navigation must share ownership, operational function, cargo compatibility,
  stock/fragment capacity, service exclusion, processing revision headroom and
  heavy quota. Arrival adds exact berth/position and heavy Holding. The existing
  command supports its existing non-heavy handling paths; do not silently add
  a new billing model or widen roles just because a label matches.
- Surface return releases at most40 units per actual pod batch. A heavy hold
  may require multiple original batches. Module unload currently offers the
  whole hold once. Revalidate each actual arrival/commit; no predicted headroom
  reservation belongs to the selector.
- Empty return is navigation with no positive packet transaction. Do not offer
  a fictitious one-unit cargo quote to make an empty craft pass. Arrival with
  no freight must not spend a pod, allowance or service bill.
- `ReadyCraft`/`CmdServiceOrbitalProbe` in `orbital_service.cpp` own real module
  service. Preserve same owner, operational service function, supported paid
  role/content, manifest/pending returns, actual job exclusion, original power
  source, real RFMT/ELEC inputs and history bounds. The new heavy requires
  exhausted allowance; old roles retain their original native admission.
- `CmdServiceLaunchProbe` in `space_manifest.cpp` provides **onboard** work for
  an orbital craft, consuming its own manifest parts. It requires a held Mine,
  idle queue or physically reached ReturnService, Holding, no pending returns,
  exhausted allowance and actual parts; the center's stock is not its service
  material owner. Earth workshop work is a separate SurfaceStored path and
  must not be invented by orbital navigation.
- `GetTargetSlots` now also accounts for each retained service job. A surviving
  exact service craft is the original slot occupant; a missing/generation-mismatched
  craft leaves that slot blocked. This is independent of which asking craft
  first contributes the slot geometry, and a recycled VehicleID cannot inherit
  the old job's berth. Focused lost-owner/second-Dock-berth checks passed.

## Verified results

`src/tests/space_mining_fixture.h::FactoryMiningRig` extracts Group18's actual
factory-heavy participant and adds an explicitly declared legacy15 option. It
reuses original field, actual extraction, public queue edits and common movement.
Parent frames/parts/content remain declared unit inputs, not gameplay procurement
claims. `space_mining_intercept_native.cpp` now uses this shared helper; both
focused cases passed together with Group19 checks.

Delivered focused coverage:

- Module domain/Visit preservation, another-work boundary, exact legacy target,
  actual mined freight on both roles, near incompatible Assembly, full/foreign
  receivers, farther eligible stock, stable ties and stale generations.
- Surface navigation versus real drop arrival, original shell ownership,
  min40 batches, full/foreign centers, no pod, quota/pending-return refusal,
  unchanged quoted native owners and real original commit.
- Module/onboard service parts, genuinely exhausted allowance, pending-return
  refusal, no consumer/parts/allowance changes during quotes, and actual
  arrived starts through the existing commands. Actual original work completes
  both service paths and renews allowance only on the final tick. Retained lost
  service jobs block only their own berth; Dock's second berth stays usable.

`space_return_target.cpp`, `space_return_surface.cpp` and
`space_return_service.cpp` exercise these boundaries, together with both
`space_mining_intercept_native.cpp` cases. The final combined tags
`[space-return-target],[space-return-surface],[space-return-service],[space-mining-intercept-native]`
passed **17,719 assertions in 11 cases** in approximately 1.4 seconds.
Log: `logs/space-fixes/space-return-final.log`.

The initial equal-distance test proposed illegal Core sites in the travel
strip. The fixture now uses horizontal symmetry in the legal top static strip;
native placement checks and the craft's actual initial pose are preserved.
The corrected case also proves actual numeric pool-ID reuse with a different
generation cannot satisfy the removed original receiver.

The retained demo spotcheck ran with:

```powershell
python src/tests/multilayer/acceptance/space_fixes_checks.py --asteroid-preview --construction waiting --run return-admission-legacy
```

It passed 274 assertions in 7.125 seconds, under the 110 second harness deadline.
Log: `logs/space-fixes/return-admission-legacy.log`; rebuildable runtime and
metadata: `test-cache/space-fixes/return-admission-legacy/`.
Source `src/tests/fixtures/demos/space/space-factory-440.sav` SHA256:
`e063202b825bf16d7fa0b536b482740e1201e0ef29050bc957fbf3301da9a53e`.
Focused executable SHA256:
`e6a7d2a7f393b396469f6f60d319fd0051f4fba86c773d4d034e66e77bd62521`.
This check preserves the original demo and does not claim reproduction of the
unavailable personal save or completion of separate financial trials.

Use actual native visit/ticks when proving arrival. Declared isolated frames,
shell receipts or counter boundary inputs must be labelled as unit setup, never
paid-world evidence. `ManufacturingMaterializationRig` already supplies real
operational Core/Assembly/Storage owners. Its `Add` supplies explicitly declared
unit packet stock. Additional unit frames can use the real blueprint identity
producer plus explicitly declared complete-frame receipts and actual activation;
do not invent an issued generation. `InstallLoadedSpacePods` can install bounded
declared shell receipts with the real issuance cursor; cross-links still need
the actual manifest and center owners.

## Integration constraints

Factor navigation versus arrival in the **existing** release/exchange/service
owners. Never move a copied Vehicle to force an arrival-sensitive quote to pass.
Read-only selection grants no berth, inventory, quota, parts or payment. Original
commands/commit functions must still rerun complete physical admission at arrival.

Group20 owns actual ordered Visit progression and cursor completion. It may need
a bounded index into the original queue while the Mine remains held for service
or continuation; that would be contained mission intent, not a second route
queue. Persist any such additional field in Group23. A candidate derived from a
future ReturnService/Unload domain does not mean that original instruction has
already executed, and may not silently skip intervening Visits or another work
action. Preserve a real current source-berth admission until the next departure
query can use it.

After compaction, read the Global Mandatory Reading and the new nearest
**Original return and service admission** block above Group19. It names the
actual provider functions discovered here; read only the receiver domain being
implemented. The broad former Groups17-20 reading block now applies to17-18;
Group20 also has its own focused integration block.
