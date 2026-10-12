# Space demo - asteroids and native mining probes

The [bundled underground and Space guide](d2um-underground-mines.md) is the main
playtest walkthrough and includes the new `space-hub` checkpoint.

From the repository root:

```powershell
.\play.ps1 space
```

The save opens **paused on Outer Space** in 1990. Unpause and use fast-forward.
Three purchased model 15 company probes pursue moving targets, attach, extract
finite resources into their own 20-tonne holds and return to orbital staging.
Each starts with its own resource class. You can assign two probes to the same
body to see slot contention, or retarget an empty probe to a different resource.

For immediate mining, or to inspect already full holds:

```powershell
.\play.ps1 space-mining
.\play.ps1 space-holds
```

Click an asteroid to inspect its class, current position and remaining/initial
reserve. Click a probe or its inspector row to select it. The inspector lists each native
vehicle's mission and cargo hold; its selected line shows target and work progress.
**Mine selected asteroid** assigns the selected body to the selected owned probe.
An occupied slot makes it wait nearby until the slot frees or approach times out.
**Recall selected probe** detaches and returns home with its cargo intact.
Retargeting a partially loaded probe requires the same cargo class. Full holds
refuse further mining. Click another probe before issuing its command.

Extraction takes 74 working ticks per 2 tonnes: ten economy days for a full hold.
Mining costs 500 internal pounds/day; travel, waiting and holding cost 100/day,
with saved fractional accrual. The three demo purchases debit 120,000 from the
existing company; setup retires the earlier survey controllers. Holds contain
real native cargo packets with extraction-plane origins and ageing. No income
is awarded for mining or returning to staging.

Switch to Surface or Underground while running, then return to Outer Space.
Bodies, missions, cargo age and costs keep advancing. Pause freezes them.
Save experiments under a new name in `build/save`; reloading preserves leases,
partial work, pursuit deadlines, cargo and fractional costs. Retained snapshots
remain immutable. `space_view` recenters the first body and opens the inspector.

## Completed features

- Actual Space-layer population with stable body IDs and three finite resources:
  metallic/Nickel Ore, carbonaceous/Water Ice and rich/Rare Earth concentrate.
- Small/Medium/Large bodies, independent headings/speeds, reflected edges and
  corners, deterministic motion and no impacts or resource respawning.
- The exact Sparse/Normal/Dense population table through 14,336 bodies. New
  Multilayer Industry worlds use the saved Sparse/Normal/Dense new-game dropdown,
  defaulting to Normal. Classic disables the selector and generates no bodies.
- Native Space Vehicle/Engine identities, company group statistics/unit numbers
  and operating accounts. Existing vehicle save meanings 0..5 stay unchanged.
- Moving-target pursuit with a fixed adaptive timeout, one exclusive native
  lease per asteroid, attached translation, timed extraction and bounded holds.
- Conserved cargo packets: reserve debit equals actual hold credit. Full holds
  and unavailable packet capacity cannot debit more reserve.
- Owner-validated synchronized mission/recall commands, native picking and a
  live inspector. Three ordinary paused mining snapshots are launchable above.
- Version 405 fleet/lease/provenance persistence and continued loading of the
  retained 403 underground and 404 survey worlds.
- Native preview sprites: nine asteroid variants and travel/attached probe art.
  [Sprite sheet](../../../../media/baseset/openttd/space_preview.png); final Plan13
  directional/state families and launch/drop/descent art remain pending.

## Return-path boundary

The full holds **remain in orbit**. Native Surface launch-center construction,
explicit freight membership and its collection overlay are delivered in
`play.ps1 space-hub`. Physical rocket commissioning, orbital cargo release,
descent pods, timed Surface handling and paid service/repeat sorties remain pending. Returning to staging creates no
Surface facility or delivery. The planned next transfer is probe hold -> saved
descent job -> Surface launch-center storage -> onward transport. Cargo changes
owner once at each handoff and earns income through the normal useful delivery.

The older repeating survey demonstration is retained separately:

```powershell
.\play.ps1 space-survey
.\play.ps1 space-attached
```

Those controllers survey without extraction, costs or company fleet identity.
Ordinary older worlds stay empty in Space until an explicit prepared demo adds
its field. `play.ps1 demo` still opens the underground tour.
