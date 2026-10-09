# Underground demo checkpoint - 2026-10-08

Owner: [portal/TBM master](../r2mlvs3mp-master-plan.md), completed through10.2 and geothermal10.3 prefixes1.1/1.2/1.3A,1B and2B. Architecture: [map layers](../../../architecture/map-layers/archi-map-layers.md). Player controls: [current how-to](../r2mlvs-how-to.md). This record prepares a bounded demo while retaining the master's unfinished tasks.

**Artifact-layout migration (2026-10-09):** this plan consumes the parent [Development artifact layout](../r2mlvs-multilayer-vehicles-and-structures.md#development-artifact-layout), implemented by [R2FC](../r2fc-file-cleanup.md). It supersedes earlier docs asset/review/helper placement and fixed runtime-config destinations below. New native homes use `mlm/runtime/`, generated feature reviews use `mlm/reviews/`, logs remain under `logs/`, and reusable code/curated inputs keep their code/test owners. Paths and focused native/cold verification are complete; R2FC records the results and retained-input identities. Repository-relative executable commands run from the checkout root through their context-aware wrappers. Historical pass records remain dated evidence and are not new acceptance claims. No gameplay task is completed by this migration and the separate post-demo fixture-pruning gate remains.

## What can be demonstrated

| Feature | Current player-facing scope |
| --- | --- |
| TBM excavation | Portal window commissions2–60-cell straight corridors. Underground Construction's **Plan an excavation area** previews and commissions larger reachable floor plans, including legal advancing bends. Cut/support/spoil/cost owners are real. |
| Longer rail routes | Excavated ground can carry ordinary rail after safe machine withdrawal and support release/conversion. Map geometry and valid cleared approaches limit construction. The area planner caps one authorized job at4,096 cells and one accepted advancing pass at1,024 segments; these are ceilings, not a guarantee that an arbitrary rectangle is reachable. |
| Spoil loop | Actual Earth/Rock wagons exchange with the hauler, reach Surface receivers, dispose their loads and return empty. The selected demo already has its starting logistics. |
| Mineral freight | Native mines, ordinary freight trains/wagons/trucks, stations and connected portals are implemented. Funding needs the matching surveyed deposit, excavated machinery footprint, owner, funds and date. Source quantities remain finite and actual cargo packets carry output. |
| Surface industry | Metal refinery, chemical, battery and electronics works are implemented, with original Steel/Oil suppliers reused. Additional native component/robotics/mirror-kit definitions exist; orbital deployment is outside this demo. Selected new industry/conventional fleet art remains10.4. |
| Tubes | Native guideway/separation/junction/reservation, paid power, prototype capsule movement/service/recovery and public readiness are tested. Normal tube tool activation, complete five-model fleet/terminals, final art and useful-route acceptance remain10.4–10.8. Do not advertise a complete player-accessible tube system yet. |
| Geothermal | Natural Deep vents, cold claims, an actual paid/material-built collector and the shared native Heat ledger exist. Physical heat routing, operating powerhouse/electric export, transported first-build acceptance and normal geothermal tools remain pending. |

Underground resources are Coal, Iron Ore, Copper Ore, Bauxite and Salt. Deep resources are Lithium Ore, Nickel Ore, Cobalt Ore, Rare Earth concentrate and Uranium Ore. The new profile's Deep access and operation unlock on1 January1980; a vent is a separate renewable heat source, not an eleventh transported mineral. Use the **Multilayer Industry** profile, Temperate climate and its pinned bundled content. Original supplied Classic saves retain their legacy policy and do not become excavation-profile games.

## Paused demo and launching it

For the player walkthrough, use [Underground mines demo](../d2um-underground-mines.md). The launcher now has a permanent `underground-mines` demo entry and the shortcut `.\play.ps1 demo`. An identical [repository snapshot](../../../../../src/tests/fixtures/haulage/planning/underground-demo-390.sav) preserves the starting world for a checkpoint commit/tag; the named launcher falls back to it if the generated build/save copy is absent.

[Focused launcher checks](../../../../../logs/r2mlvs3mp-z-m-demo-launch-checks.log) verify named/number/path selection, Debug/optimized choices, repository fallback and explicit missing-demo handling, preserving both world hashes, play.cfg and the Git index. Interactive menu selection and invocation from the documentation directory also pass. These are launch/navigation checks; they do not close the pending gameplay acceptance tasks.

The prepared save is [underground-demo-390.sav](../../../../../build/save/underground-demo-390.sav). From the repository root:

```powershell
.\play.ps1 build\save\underground-demo-390.sav
```

The launcher selects the optimized RelWithDebInfo executable and repository-local play configuration. The game starts paused. Inspect the active Underground project and its portal window, then unpause to show the spoil exchange/haulage loop. Save personal changes under a new name to retain this starting point. For an independent new game, use `.\play.ps1 1` and follow the how-to's portal/receiver/hauler setup.

The starting file is the retained native385 bent-route AwaitExchange acceptance world. [Preparation helper](../../../../../src/tests/multilayer/acceptance/demo_checkpoint.py) loads it using the ordinary optimized executable, pauses it and writes390; a second ordinary load/resave verifies the current format. Nine natural/project/support/exchange chunks remain byte-exact across both operations, and the original385 input SHA256 is unchanged. Evidence: [preparation](../../../../../logs/r2mlvs3mp-z-l-demo-checkpoint.log), [migration](../../../../../logs/r2mlvs3mp-z-l-demo-migration.log) and [reload](../../../../../logs/r2mlvs3mp-z-l-demo-reload.log). This checks real loading and saves; it does not substitute for personal GUI playtesting or normal-newgame end-to-end acceptance11.4.

Prepared demo SHA256: `16ff76021341cf734b0e5ccc7b906abf90705481bbc2c031d6c01fd8eab61dbf`.

## Shared native Heat handoff and focused checks

[utility_power.h](../../../../../src/utility_power.h) publishes `RegisterUtilityHeatProducer`, `RegisterUtilityHeatConsumer`, `PublishUtilityHeatGeneration`, `AllocateUtilityHeat`, `GetUtilityHeatAllocation` and `PrepareUtilityHeatMutation`. `CommitUtilityMutation` requires the actual whole benefit participant's matching Heat domain and direct/generator role. One native owner issues IDs and stores fractions, reservations, use, expiry and history; geothermal physical readiness/connectivity retains its own owner. Interval rotation is still the shared electrical driver's responsibility until actual thermal scheduling is integrated.

The new explicit save boundary is390. UPLD records their immutable domain; UPLH independently records electrical/heat phase completion. Pre390 electrical saves default only to Electricity. Source/cold/collector authority remains in its existing families. No actual operating generator or electricity export is inferred from shared accounting tests.

Capsules require an active metered electrical grant for movement and service. The tested early source is explicit paid external service at an owned commissioned portal, with a100-kW capacity,20 Money per economy day standing charge and1 Money per1,000 consumed joules using saved fractional tariff carry. Missing supply, disconnected topology or unaffordable work causes a recoverable wait, preserving cargo and manual stop. An owned geothermal plant is unnecessary for this bootstrap; the later geothermal source switch remains Plan16 task6.4. The planned gameplay benefit is independent layer-local freight shuttles, with bulk, container, fluid, sealed-mineral and heavy cargo roles; a demonstrated advantage over conventional rail still needs complete-fleet balance/useful-route acceptance.

Focused Debug and optimized heat/electrical checks each pass1,247 assertions/23 cases, and affected tube power/movement/recovery each pass10,089 assertions/23 cases. The native390 suite proves genuine389 migration, actual shared thermal reservations/expense settlement, exact reload, ordinary game resave and six malformed-copy preview refusals. Original input hashes and electrical/natural/native stock lineage are preserved. See the [Group2B closure](../r2mlvs17-geothermal-energy-and-industry.md#group-2b---supply-the-missing-shared-native-heat-domain) for the exact retained logs. No full regression was run.

## What “corrupt previews” means

These are deliberately modified **test copies** of good saves: wrong energy domains or consumer roles, invalid cross-domain links, inconsistent phase flags or a missing required field. “Preview” means the loader's independent validation when inspecting a save before accepting its world. Expected rejection proves malformed data cannot replace the running ledger or create fresh resources. The original saves are preserved; ordinary good-save reload/resave succeeds. This phrase describes negative tests and does not report spontaneous corruption of player saves or damaged sprite previews.

## Repository checkpoint disposition

Development started from `9d1f6c80de7767991ec011d5f39b9ebbd49e313d` (`16.0-beta4-43-g9d1f6c80de`). The user authorized committing the current tube/geothermal work and demo launcher/snapshot on2026-10-08 with the checkpoint tag **`mlv-demo2`**. Resolve that tag for the checkpoint source version; the earlier development-base hash does not include this work. The commit message lists every plan containing newly completed tasks.

The checkpoint game/test builds are recorded in [optimized](../../../../../logs/r2mlvs3mp-z-l-checkpoint-optimized-build.log) and [Debug](../../../../../logs/r2mlvs3mp-z-l-checkpoint-debug-build.log) logs. The optimized game executable SHA256 is `bbac4febbd2bb21bf39e7b6f3955d482cb181a30fc0239528533ea2832cee216`. Before staging the checkpoint commit, read-only build/verification preserved Git index SHA256 `64315e1b3bb2780b34be1032d1f884bbe91bdf88226ad7bfed99a01b781da136`; this is historical precommit evidence. Final source changes after the focused test bursts clarify comments only; executable behavior is unchanged.

Demo checkpoint name: **`mlv-demo2`**. When distributing it, retain the source changes, bundled selected art and matching executable together. A short personal GUI pass of the prepared demo remains the player-review step. Normal-newgame bootstrap, multiplayer joining, maximum-workload performance, complete tube fleet/tools and actual geothermal plant/export remain explicitly unverified or unfinished. The master's next producer is **Plan17 Group2**, actual bounded thermal source intervals, under10.3; future tasks remain unchecked.
