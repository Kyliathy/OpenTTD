# Native Space vehicle compatibility

Execution contract, 2026-10-12. This records Plan8 Groups1-3 before changing the
saved vehicle enum. Space is a fifth company category with its own native class;
it never uses Aircraft, Effect or a parallel fleet/cargo registry.

| Meaning | Saved/runtime byte | Company slot | Compatibility policy |
| --- | ---: | ---: | --- |
| Train | 0 | 0 | Preserve |
| Road | 1 | 1 | Preserve |
| Ship | 2 | 2 | Preserve |
| Aircraft | 3 | 3 | Preserve flight/airport logic |
| Effect | 4 | none | Preserve; never a company vehicle |
| Disaster | 5 | none | Preserve; never a company vehicle |
| Space | 6 | 4 | Append; reject in saves before the native Space version |

`CompanyEnd=4` remains a historical four-mode boundary. Fleet owners use
`COMPANY_VEHICLE_TYPES` and `CompanyVehicleIndex`; company `VehicleTypeIndexArray`
storage has five dense slots, while arrays explicitly ending at `End` use raw
saved type indices. Group statistics, free unit numbers, EngineOverrideManager
and survey counts share that mapping. Depot geometry and Surface screensaver
iteration deliberately retain the four legacy modes: orbital vehicles have no
ordinary depot or Surface pathfinder.

| Assumption/owner | Policy |
| --- | --- |
| VEHS class decoder, `saveload/vehicle_sl.cpp` | Append versioned native class; retain all six existing constructors/descriptors |
| Engine metadata and EIDS, `engine_base.h`, `engine.cpp`, `newgrf.cpp` | Extend the canonical variant/mapping; immutable model15 supplies native properties |
| Company groups/unit allocation, `company_base.h`, `group_cmd.cpp` | Extend dense storage and explicit company iteration; retain old indices |
| Fleet window identity, `vehiclelist.cpp` | Preserve old bits exactly; unused bit22 distinguishes Space, without reducing the20-bit index |
| Expense arrays, `economy_type.h` | Preserve all13 meanings; purchase uses NewVehicles, orbital operating costs use Other explicitly |
| Native tick/cargo age, `vehicle.cpp` | Extend virtual dispatch/age branch; reject NewGRF sound/callback dispatch for Space |
| Orders, `order_cmd.cpp` | Orbital missions own typed body targets; reject ordinary station/depot orders and shared-order/clone operations |
| Service/depot, `vehicle_cmd.cpp`, `company_cmd.cpp` | Explicit orbital policy; reject ordinary depot/refit/build operations |
| Script APIs, `script/api` | Preserve four published script mode values; exclude Space from legacy script vehicle/engine validity |
| NewGRF feature mapping, `newgrf.cpp` | Space maps to Invalid; no invented feature IDs or aircraft callbacks |
| CargoDist/transport modes, linkgraph/station loading | Preserve four modes; orbital holds do not create station links or route shipments |
| Source provenance, `source_type.h`, CAPA | Append Asteroid source kind; bounded body IDs fit16bits; never cast to IndustryID |
| Graphical fleet/detail/build windows | Orbital inspector adapter; preserve legacy layouts and reject unsupported entry points |
| Company infrastructure | No orbital facilities counted as airports; construction/launch counts await actual facility producer |

Required executable evidence: historical byte/dense-index and all-kind fleet
window collision tests, ordinary old underground403/Space-survey404 loading,
native Space VEHS/EIDS/CAPA round trips, deterministic hidden-view continuation,
and existing state/content producer gates. Existing six type descriptors remain
unchanged, so their old fixture records must retain identity on reload.

The delivered P2 increment provides native orbital mining in explicitly prepared
demo worlds. Demo setup retires survey records and purchases native probes through
the canonical company owner. Extracted cargo stays in the probe hold at its
orbital staging point. Launch construction, physical rocket commissioning,
Surface descent, station delivery/payment and automatic repeat sorties remain
separate producer work; no staging point represents a free Surface facility.

## Indexed consumer inventory

The following inventory distinguishes dense company storage from deliberately
retained Surface UI/transport assumptions. A default fifth entry does not grant
an unsupported GUI, depot, station, callback or script operation.

| Owner | Indexed fields / dispatch | Native Space policy |
| --- | --- | --- |
| `company_base.h`, `group_cmd.cpp` | `group_all`, `group_default`, `freeunits`; clear/count/profit/autoreplace iteration | Five dense slots, explicit company enumeration; native primary/engine counting |
| `engine.cpp`, `engine_base.h` | Original counts/offsets, Engine variant, override mappings, SetupEngines | Space has zero original four-mode engines; native variant and package/local EIDS mapping append its definition |
| `engine_gui.h`, `build_vehicle_gui.cpp` | Sort criteria/order/hidden/cargo filters, functions/listings, sprite offsets | Dense storage; ordinary build-window entry refuses Space before using old mode widgets |
| `depot_gui.cpp` | Depot/purchase cell sizes, clone icons and initialization loop | Legacy four-mode facility geometry; no orbital depot exists |
| `economy.cpp` | Gradual loading wait-time table | Native probes never enter station loading or CargoDist; default Space entry is unreachable |
| `company_gui.cpp` | Livery empty-group labels, vehicle-count labels and display iteration | Existing Surface presentation retained; native count/hold presentation uses the orbital inspector |
| `group_gui.cpp` | Group WindowDesc table | Four legacy descriptors; Space redirects before indexing |
| `vehicle.cpp` | No-depot strings, engine counting/type predicate, free-unit limits, tick/sound/cargo-age dispatch | Explicit native ownership/64-unit bound/age scheduling; depot dispatch refuses and NewGRF callbacks/sounds are skipped |
| `vehicle_cmd.cpp`, `vehicle_func.h` | Six build/sell/refit/autoreplace/depot message arrays | Dense storage; explicit mission commands, native stopped/empty-home sale, normal start/stop; unsupported depot/refit/clone paths refuse before legacy offsets |
| `vehicle_gui.cpp`, `vehicle_gui_base.h` | Grouping, depot labels, list descriptors, zoom/message/sprite tables | Four-mode list descriptors retained; Space picking/view/list entry redirects to its inspector |
| `vehiclelist.cpp` | Two type bits and20 index bits | Existing bits unchanged; previously unused bit22 marks Space |
| `saveload/vehicle_sl.cpp` | First-engine fallback table and six old subtype handlers | Space fallback is invalid; strict native identity validation, appended handler and preallocation fleet bound; old subtype handlers retain fields |
| `survey.cpp` | Mode strings and fleet enumeration | Explicit fifth `space` category |
| `smallmap_gui.cpp` | All-type colour table | Raw seven-type table appends Space colour; Effect/Disaster remain at4/5 |
| `screensaver.cpp`, `story.cpp` | Legacy company-range iteration/selection | Preserve four Surface modes; no Space selection through those APIs |
| `script/api/script_vehicle*.cpp`, `script_engine.cpp` | Script validity/list filters and published four-mode enums | Exclude native Space from legacy script APIs, including deity engine visibility |
| `newgrf.cpp`, `network/network_command.cpp` | Feature switch and command trait registration | No Space NewGRF feature; appended native mission command uses ordinary agreed company/frame dispatch |
| `source_type.h`, `saveload/cargopacket_sl.cpp` | Source-kind byte and bounded source ID | Append Asteroid=3 only from405; native hold/source/plane validation rejects aliases, orphans and duplicate packet ownership |

`ASTR`405 appends a32-bit native VehicleID lease to each body;404 records acquire
the native invalid sentinel on load. `VEHS` appends a `space` struct while all old
type handlers retain their columns. The compatibility wrapper compares every
old descriptor/record byte after removing only that verified empty new column.
Native load initializes viewport indexing once through the common vehicle pass;
orbital cache initialization must not insert a second copy before that pass.
