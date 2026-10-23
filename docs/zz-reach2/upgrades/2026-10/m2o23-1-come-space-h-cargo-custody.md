# M2O23-1-H — Actual packet owners and custody admission

This is the Groups13-19 implementation handoff for [the merge foundation](m2o23-1-come-space.md), under [native map-layer ownership](../../architecture/map-layers/archi-map-layers.md). Source inspection identifies physical containers; it does not certify a loaded world, purchase, runtime tick or protected Space continuation. Named live fields and exports are authoritative; navigation ranges may drift.

## Physical owner inventory

| Actual owner | Physical packet boundary | Cargo and source-queue interpretation |
| --- | --- | --- |
| Native station goods | `Station::goods[cargo].GetData().cargo`, only when `HasData()` | The real goods-slot cargo and this station's actual ID. Never allocate data during observation. |
| Every native vehicle, including Space hulls and reserved freight | `Vehicle::cargo` across `Vehicle::Iterate()` | The vehicle's actual `cargo_type`; invalid StationID. Reserved Load-range packets live here, while station `reserved_count` is a cache, not another packet container. |
| Surface launch handling | `Station::launch_hub->handling[*]->packets` | Each actual job's `cargo`; invalid StationID even though the containing hub belongs to a station. |
| Spacecraft supplies | `SpaceVehicle::manifest->supplies[0..SPACE_SUPPLY_COUNT)` | `GetSpaceSupplyCargo` gives Propellant/RFMT/ELEC meanings. These compartments are separate from the same hull's ordinary freight. |
| Passive pod descent | `SpacePod::descent->packets`, only when the job exists | The descent job's actual `cargo`; invalid StationID. Shell and manifest pod references are not additional cargo stores. |
| Orbital construction | `SpaceStructure::job.inputs[0..2)` | The exact `GetSpaceConstructionDefinition(state.kind)->inputs` labels; invalid StationID. Missing definitions refuse. |
| Completed orbital stock, including processor inputs/outputs | `SpaceStructure::module->stock[cargo]` | Actual array cargo slot; invalid StationID. `module->processing` owns recipe/work metadata, not another physical inventory. |
| Both orbital service berths | `module->GetServiceJobs()` and each job's two `inputs` | RFMT then ELEC through `GetSpaceSupplyCargo`; invalid StationID. The second berth is not an alias of the first. |
| A's contained manufacturing batch | `module->manufacture->inputs[0..MAX_ORBITAL_MANUFACTURE_INPUTS)` | Immutable `GetSpaceVehicleConstruction(job.model)->inputs` label order; invalid StationID. Unknown recipes, excessive definitions or nonempty unused slots refuse. All four heavy-input slots must be accounted for. |

The field survey covered actual `StationCargoList`/`VehicleCargoList` members and packet-reference aliases in native headers/implementations, excluding fixture snapshots. Native stock count caches, saved/copied packet-ID vectors, manifests' craft/pod references and recipe-progress metadata are not additional owners. `InsertOrbitalPacketOwners` remains the existing orbital subset admission helper; it must not be replaced with a global traversal that double-counts owners already inserted by its caller.

## One full native traversal

Group13 has implemented `NativeCargoPacketVisitor` and `VisitNativeCargoPackets` in `src/cargo_custody.h/.cpp`, adapted from B's observed boundary. The visitor receives each actual packet, its physical container's cargo type and the original goods-queue StationID only for station goods. It reads every routing bucket, rejects null references and unresolved nonempty container meanings, and never creates goods data, changes a queue, consumes RNG or treats packet-pool membership as ownership.

A's `ValidateAsteroidPacketCustody` in `space_vehicle.cpp` now consumes the shared full walk, including the previously omitted manufacturing inputs. Its original asteroid-specific natural source, layer, first-station, label, duplicate and final-orphan rules remain intact. Do not replace the larger Space feature validators or confer operational permission on a component-only owner fixture.

## Separate receipt policy and physical publication

Group13B now supplies `SurfaceSupplyPacketFacts`, `SurfaceSupplyPacketLookup` and `SurfaceSupplyCustodyCheck` in the existing order model. It invokes `ValidateSurfaceSupplyRecords` against the same immutable `SurfaceSupplyRecordContext`, then streams actual owner facts and an ordered complete allocated-row pass through one candidate lookup. An independent original packet count prevents incomplete enumeration from passing. Later copied adapters must supply their own original context/rows; no active-world fallback or second complete packet inventory is permitted.

For each linked legacy receipt, validate exact ID, canonical label, permanent non-paying purpose with unavailable original buyer, positive quantity, finite refunded/dispatched bounds and unique physical ownership. Never-shipped stock may remain eligible only in its original source queue while its receipt is open; preserve B's owner-loss exception only for an actual same-candidate Surface goods queue, without granting a new refund owner. An Underground queue, missing station, carrier or job cannot masquerade as never-shipped source stock. Keep totals bounded before subtraction/addition:

```text
outstanding <= purchased - refunded
unshipped <= purchased - refunded - dispatched
outstanding - unshipped <= dispatched
```

After real owner traversal, inspect allocated packet facts only to detect unsupported flags and missing ownership. That final scan must never invent a container or source queue. All visited references must be unique. Paid orphans include unlinked A CompanySupply, while ordinary orphan/source checks remain with their existing native owners; the supply checker grants ordinary orphan rows no custody or payment. Ordinary cargo and unlinked A CompanySupply keep their existing source/payment semantics; historical native `issued_packet` is not a live custody link. Explicitly document which existing native orphan/ordinary-source checks remain with their original owners.

Group13C has wired the complete checker to the existing staged adoption boundary and removed the temporary legacy rejection only after all reference/custody checks pass. Keep same-file DATE/MAPS joins and unchanged installed state on refusal. Raise complete-history staging to65535 while retaining16384 native/new-purchase policy and the unchanged old A440 decoder bound. Old wire decoding and the new writer remain Plans3-4 work.

Group13D exercises linked stock in actual Space owners and the normal staged adoption path for first load/reservation/hub departure, repeated departure and returned stock. These are declared native components, not evidence that a genuine original B file already loads. New purchases retain A's Pending-only refund rule. Group15 now supplies exact saved-tariff legacy cancellation through the native command kernel; Group16 now preserves lifetime legacy history, source-sign eligibility, native Pending refunds and actual takeover/loss rules without credit on legacy source loss.

## Native component acceptance handoff

Groups13C/13D are complete at the declared component boundary. `surface_supply_adoption.cpp` uses the production DATE/MAPS staging and admission path for valid source stock, malformed references/history, paid orphans, duplicate actual owners and all65535 retained legacy rows. It checks unchanged stock, cash, caps and installed history on refusal. New-purchase admission and the original A440 transport remain bounded independently of imported history.

`surface_supply_space_custody.cpp` covers13 applicable physical destinations: handling, spacecraft freight, RFMT/ELEC supplies, descent, STEL construction, module stock, both RFMT/ELEC service berths and the heavy manufacture STEL/ELEC slots. PROP, GOOD and RBPT roles cannot borrow an RFMT receipt. Actual ordinary and unlinked A stock remain controls. The first attempt's100-unit ELEC declaration correctly failed its64-unit cap; the corrected independent30-unit component preserves that production rule.

Real station Load/Reserve and Hub transfers now have successful linked-runtime proof through the normal installer. A100-unit purchase departs30 units, including partial reservation-to-load and reservation return. Repeated carrier/handling movement adds no history. Returned stock remains non-paying; only the70 untouched units qualify for source selection. A stale69-unit removal refuses unchanged. Departing all100 next counts only those70 new units, and final return leaves no refundable stock, no credit and no automatic closure. These cases closed the successful-native-path deferrals recorded by12B; the later completed lifecycle and cancellation acceptance is recorded below.

Rebuild `openttd_surface_supply_test` in Debug, then run from `build/` with filters `[surface-supply-adoption],[surface-supply-owner],[surface-supply-dispatch]` or `[surface-supply-space-custody],[native-cargo-custody]`, compact reporter and an absolute `--out` beneath `test-cache/merge-supply/`. The respective scoped passes were12 cases/1172 assertions and6 cases/1328 assertions, each under four seconds. Original-save decoding, copied custody adapters, optimized acceptance and the merged writer remain Plans3-4 work. No protected saved-world continuation was loaded or advanced.

## Supply and cargo closure through Group19

All component tasks through19 are complete. These public owners are the downstream contract; do not import a donor vector, alternate packet inventory, self-credit command or second tube-capacity calculation.

| Public owner | Authoritative exports and invariants |
| --- | --- |
| [Canonical order model](../../../../src/surface_supply_order.h) | `SurfaceSupplyOrderRecord`, `SurfaceSupplyOrderSnapshot`, `FindSurfaceSupplyOrderRecord`, `CountSurfaceSupplyDayUsage`, `SurfaceSupplyCustodyCheck`: exact uint64 identity, tagged original evidence, lifetime dense legacy prefix and separately bounded native/new admission. |
| [Native runtime](../../../../src/surface_supply_order_runtime.h) and [command traits](../../../../src/surface_supply_cmd.h) | `PrepareLegacySurfaceSupplyCancellation`, `ValidateSurfaceSupplyOrderBook`, `ReconcileSurfaceSupplyStation`, `ChangeSurfaceSupplyOrderCompanyOwnership`, `CmdCancelLegacySurfaceSupplyOrder`: native finance settles one exact frozen refund; source/company loss closes legacy rights without credit, takeover transfers original history, new purchases retain Pending-only refunds. |
| [Native packet owner](../../../../src/cargopacket.h) and [hub movement](../../../../src/hub_cargo.h) | `GetCompanySupplyOrder`, `HasCompanySupplyShipped`, `CountUnshippedCompanySupply`, `RemoveUnshippedCompanySupply`, native Load/Reserve/Shift and `HubCargoTransfer::Move/Consume`: actual custody and permanent non-paying purpose survive all movement, consumption and return. Partial Deliver-to-Transfer splits only the requested boundary and refuses pool exhaustion before route/action mutation. |
| [Single tube-capacity interface](../../../../src/transport_tube_cargo.h) | `TUBE_TERMINAL_TRANSFER_CAPACITY` remains1024; `GetTubeStationTransferRoom` and `GetTubeInterchangeTransferRoom` share one widened calculation. Staging counts queued reservations and pending actual arrivals; unload validates the exact destination without counting pending transfers again. A non-applicable queue is distinct from a full tube queue. |
| [Transient native controls](../../../../src/surface_supply_gui.h) | `SurfaceSupplyControls` renders canonical quotes, stores exact offered IDs and checks stale legacy display facts before posting the checked command. It owns no stock, caps, history or money. |

Focused executable coverage is registered in both the existing general test owner and selected `openttd_surface_supply_test` owner. Current passes: Group15 refund/window12 cases802 assertions; Group16 actual lifecycle11/414; Group17 paying/non-paying/Space/partial-transfer12/1567; Group18 actual tube/ordinary rail-road7/1052; Group19 continuous cross-owner1/92. Each invocation finished below four seconds. Rebuild the selected Debug target, run from `build/`, and select only the active area's tags (`[surface-supply-refund]`, `[surface-supply-legacy-lifecycle]`, `[surface-supply-legacy-delivery]`, `[cargo-partial-transfer]`, `[transport-tube-room]` or `[surface-supply-cross-owner]`). Use compact output with an absolute destination under `test-cache/merge-supply/`; build transcripts and failed-attempt diagnostics are disposable there. Do not invoke hidden original-save or protected Space continuation tests as a component shortcut.

The continuous Group19 scenario starts with an explicit original100-unit legacy component. Thirty leave source; the actual command refunds the70 untouched units once. A real command-built tube station already containing1000 ordinary units admits24 and holds six aboard, then admits the remaining six after ordinary stock is consumed through the native owner. The same30 shipped units pass through conventional carrier, Surface handling, Space manifest, orbital stock and service input. Ten are consumed;20 return to the original source with no fresh cancellation eligibility. Full custody validation follows every ownership boundary; original day usage remains100 and total cash changes only by the frozen1750 refund. Floor, stock and empty carrier/module bodies are declared components. This proves physical conservation/admission, not driven travel, paid construction of orbital services or original-file loading.

No supply/cargo component execution is deferred for lack of a build. Remaining Plans3-4 work is actual B/A legacy decoding, copied same-file packet/owner joins, merged writing, original-world roundtrips and optimized/integration acceptance. Protected Space cash/tick seals and the provisional Tube fleet stay unchanged; this handoff grants no continuation permission.

## Sources inspected

| File name | Sections or Line Ranges used while planning | Planning contribution |
| --- | --- | --- |
| [B native traversal](../../../../../OpenTTD3/src/cargo_custody.cpp) | `VisitStationCargoPackets`, `VisitNativeCargoPackets` | Base traversal and actual station-versus-job distinction; missing A manufacture ownership requires extension. |
| [A asteroid audit](../../../../src/space_vehicle.cpp) | `ValidateAsteroidPacketCustody` | Identifies the existing full walk to factor and its source-specific rules to preserve. |
| [Orbital native owners](../../../../src/orbital_station.h) | `OrbitalConstructionJob`, `OrbitalServiceJob`, `OrbitalManufactureJob`, `OrbitalModule` | Separates physical construction/service/manufacture/stock containers from processing metadata. |
| [Orbital subset join](../../../../src/orbital_station.cpp) | `InsertOrbitalPacketOwners` | Already includes manufacturing for its bounded subset; a global replacement would double-count its caller's earlier owners. |
| [Copied owner join](../../../../src/saveload/space_cargo_preview.cpp) | `ValidateSpaceCargoPreviewOwners` | Confirms A's manufacturing transport and broader native/feature exclusivity without making copied ID vectors a second inventory. |
| [Vehicle supplies](../../../../src/space_manifest.h) | `SpaceManifest`, `SpaceManifestRecord`, `GetSpaceSupplyCargo` | Distinguishes actual compartments from hull freight and copied records. |
| [Pod lifetime](../../../../src/space_pod.h) | `SpacePod`, `SpacePodState` | Only the actual descent job owns packets; carrying references and paid shells do not. |
| [Launch and descent jobs](../../../../src/launch_center.h) | `SpaceDescentJob`, `SurfaceHandlingJob`, `LaunchCollectionHub` | Real job cargo meanings and physical packet fields. |

These sources record planning use, not an execution reading queue; they are non-transitive and must not be preloaded after compaction. The main plan's nearest Area selects the contracts required for the current batch.
