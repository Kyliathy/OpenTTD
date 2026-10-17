# R2MLVS1 execution record - content contract

Owner: [r2mlvs1-gameplay-contracts-and-content.md](../../../zz-reach2/upgrades/2026-10/r2mlvs1-gameplay-contracts-and-content.md). Architecture: [archi-map-layers.md](../../../zz-reach2/architecture/map-layers/archi-map-layers.md). Evidence baseline: 2026-10-04; clean checkout at execution entry. This record distinguishes inspected definitions, provisional content decisions, and executed fixtures. It does not certify downstream facilities or art.

**Artifact-layout migration (2026-10-09):** this plan consumes the parent [Development artifact layout](../../../zz-reach2/upgrades/2026-10/r2mlvs-multilayer-vehicles-and-structures.md#development-artifact-layout), implemented by [R2FC](../../../zz-reach2/upgrades/2026-10/r2fc-file-cleanup.md). It supersedes earlier docs asset/review/helper placement and fixed runtime-config destinations below. New native homes use `mlm/runtime/`, generated feature reviews use `mlm/reviews/`, logs remain under `logs/`, and reusable code/curated inputs keep their code/test owners. Paths and focused native/cold verification are complete; R2FC records the results and retained-input identities. Repository-relative executable commands run from the checkout root through their context-aware wrappers. Historical pass records remain dated evidence and are not new acceptance claims. No gameplay task is completed by this migration and the separate post-demo fixture-pruning gate remains.

## Group 1 - live identity and format inventory

`SetupCargoForClimate` and `src/table/cargo_const.h` give these Classic Temperate slots. Slot 11 is deliberately invalid; the 12-slot original envelope contains **11 active meanings**.

| Runtime slot in unmodified Temperate | Label | Meaning |
| --- | --- | --- |
| 0 | PASS | Passengers |
| 1 | COAL | Coal |
| 2 | MAIL | Mail |
| 3 | OIL_ | Oil |
| 4 | LVST | Livestock |
| 5 | GOOD | Goods |
| 6 | GRAI | Grain |
| 7 | WOOD | Wood |
| 8 | IORE | Iron Ore |
| 9 | STEL | Steel |
| 10 | VALU | Valuables |
| 11 | invalid | Original unused slot; do not repurpose it |

Classic climate active counts are Temperate 11, Arctic 11, Tropic 12, Toyland 12. Arctic's empty slot is 8. `CORE` already means Copper Ore in Tropic. `BATT` already names Toyland item batteries; industrial battery packs below use a different label to preserve that definition.

| Registry / array | Inspected bound and preserved identity |
| --- | --- |
| Cargo | `NUM_CARGO=64`; `NUM_ORIGINAL_CARGO=12`; auto-refit `0xFD`, no-refit `0xFE`, invalid `0xFF` |
| Industry types | 37 original types at 0..36; 240 global slots; 128 types per NewGRF; invalid `0xFF` |
| Temperate industry types | 11: Coal mine 0, Power station 1, Sawmill 2, Forest 3, Oil refinery 4, Oil rig 5, Factory 6, Steel mill 8, Farm 9, Bank 12, Iron mine 18 |
| Industry tile definitions | 175 original definitions at 0..174; 512 global/per-GRF slots; invalid 512; no-animation/water-check sentinels `UINT16_MAX` |
| Industry cargo arrays | 16 accepted / 16 produced; original label initialization still has 3 accepted / 2 produced |
| Original engine tables | 116 rail, 88 road, 11 ship, 41 aircraft = 256 definitions across climates |
| Original engine offsets | Rail 0, Road 116, Ship 204, Aircraft 215; retain local IDs and saved engine mappings |
| Engine pool | `EngineID` uses `uint16_t`, maximum 64,000 entries, invalid `0xFFFF`; GRFID + vehicle type + local ID determines mapped identity |
| Layers | Surface=0, Underground=1, Space=2, Deep=3; display order does not change storage IDs |

Sources inspected: `src/cargo_type.h`, `src/cargotype.cpp` (`SetupCargoForClimate`, `BuildCargoLabelMap`), `src/table/cargo_const.h`, `src/industry_type.h`, `src/industrytype.h`, `src/table/build_industry.h` (`IndustryTypes`, `_origin_industry_specs`), `src/engine_type.h`, `src/engine.cpp` (`GetOriginalEngineCount`, `GetOriginalEngineOffset`, `Engine::Engine`), `src/table/engines.h`, `src/map_layer.h`.

Preservation rule: never insert into or reorder an original source table. Multilayer additions resolve by label and package/local identity; new cargos and carriers use slots outside their original envelopes. Classic activation keeps its tables and behavior. No catalogue row authorizes hard-coded runtime cargo slots. No source definitions changed in this group; verification was read-only characterization, with no unit tests needed for this documentation batch.

Group 3 source correction: `IndustryOverrideManager::AddEntityID` can reuse a climate-disabled original **runtime** industry slot before allocating above 36. This preserves the original source table and existing saves' configuration; its saved IIDS mapping owns the new profile's runtime meaning. Industry identity must therefore use package + local type, not the original enum at the mapped slot. The conservative 64-type budget remains valid. The dedicated spike checks that the mapped ID is unchanged after reload.

## Group 2 - draft cargo meanings

The labels here are first-party contract choices, not claims that ETI supplies matching definitions. An ETI adapter must map its one Uranium definition to this meaning or refuse the profile; a second Uranium cargo is forbidden. Nickel from asteroids shares `NORE`; rare-earth asteroid concentrate shares `RARE`. All mineral quantities below use one metric tonne per cargo unit. Sealed Uranium remains ore, not fabricated nuclear fuel.

| Label | Resource / physical form | Cargo classes | Conventional refit and tube compatibility | Useful Surface receiver / payment |
| --- | --- | --- | --- | --- |
| COAL | Coal, dry lump bulk | Bulk, NonPotable | Bulk wagon/truck; Mole-70 | Existing power station / ordinary final freight income |
| IORE | Iron Ore, dry bulk | Bulk, NonPotable | Mineral wagon/truck; Mole-70 | Existing steel mill / ordinary final freight income |
| CORE | Copper Ore, dry bulk | Bulk, NonPotable | Mineral wagon/truck; Mole-70 | Metal refinery / ordinary final freight income |
| BAUX | Bauxite, dry bulk | Bulk, NonPotable | Mineral wagon/truck; Mole-70 | Metal refinery / ordinary final freight income |
| SALT | Industrial Salt, dry crystals | Bulk, NonPotable | Mineral wagon/truck; Mole-70 | Chemical works / ordinary final freight income |
| LITH | Lithium Ore, dry bulk | Bulk, NonPotable | Deep-rated mineral wagon/truck; Mole-70 | Battery works / ordinary final freight income |
| NORE | Nickel Ore, dry bulk | Bulk, NonPotable | Deep-rated mineral wagon/truck; Mole-70 | Metal refinery / ordinary final freight income |
| COBT | Cobalt Ore, dry bulk | Bulk, NonPotable | Deep-rated mineral wagon/truck; Mole-70 | Battery works / ordinary final freight income |
| RARE | Rare Earth concentrate, covered dry granules | Bulk, Covered, NonPotable | Covered mineral wagon or sealed carrier; Shield-85 | Electronics works / ordinary final freight income |
| URAN | Uranium Ore, sealed drums of concentrate | PieceGoods, Covered, Hazardous, NonPotable | Sealed mineral carrier or sealed mineral wagon available in 1980; Shield-85 from 1985 | Nuclear plant / ordinary final freight income, distinct from metered power |

Refits use the explicit allowed-label list as well as form/classes. A generic Bulk refit cannot admit hazardous drums. Earth/Rock hoppers carry only their dedicated spoil label; tube transport cannot replace the mandatory TBM loaded-wagon journey to the origin Surface station. Layer and model-date checks remain separate from refit compatibility.

| Label | Additional cargo | Physical unit / classes | Compatibility / payment |
| --- | --- | --- | --- |
| ERTH | Earth | tonne; Bulk, NonPotable | Earth hopper; bulk truck; Mole-70 after mandatory wagon leg; disposal debit, no spoil subsidy |
| ROCK | Rock | tonne; Bulk, NonPotable | Rock hopper; bulk truck; Mole-70 after mandatory wagon leg; crusher/surplus disposal debit |
| RFMT | Refined Metals | tonne of ingots; PieceGoods | Flatbed/mineral freight; Forge-05; ordinary final freight income |
| CHEM | Chemicals | kilolitre of liquid, cargo weight 1 tonne; Liquid, Hazardous, NonPotable | Sealed tanker; Cistern-80; ordinary final freight income |
| BATY | Batteries | tonne of packaged industrial packs; PieceGoods, Covered | Covered freight; Crate-75; ordinary final freight income |
| ELEC | Electronics | tonne of protected assemblies; PieceGoods, Covered | Covered freight; Crate-75; ordinary final freight income |
| PROP | Propellant | kilolitre of stabilized liquid, cargo weight 1 tonne; Liquid, Hazardous, NonPotable | Sealed tanker; Cistern-80; ordinary final freight income |
| SCMP | Station Components | tonne of packaged structures; PieceGoods, Oversized | Heavy flatbed; Forge-05; rocket payload; ordinary final freight income |
| RBPT | Robotics Parts | tonne of packaged machinery; PieceGoods, Covered | Heavy covered freight; Forge-05; rocket payload; ordinary final freight income |
| SMIK | Solar Mirror Kits | tonne of folded packaged kits; PieceGoods, Covered | Covered freight; Crate-75; rocket payload; ordinary final freight income |
| WICE | Water Ice | tonne of frozen bulk; Bulk, Refrigerated, Potable | Insulated orbital hold; ore processing/refuel input; ordinary final freight income only at a useful final receiver |

Liquid Chemicals and Propellant never acquire PieceGoods merely to fit a container vehicle. Their recipe quantities are kilolitres, never drums; packing would require a separately agreed conversion. Existing `STEL` and `GOOD` retain their own units and definitions. Nuclear/solar/geothermal generation and vent heat are utility quantities, with no native cargo label or transport income. `POWR` is a reserved adapter possibility, excluded from the native active cargo count.

## Whole-catalogue provisional budgets

| Registry | Conservative envelope | Actual active meanings / headroom |
| --- | --- | --- |
| Native cargo | 12 original slots + 8 added mine meanings + 11 extra meanings = 31 occupied/reserved slots | 11 + 8 + 11 = **30 active**, 33 unused slots outside the envelope |
| Potential ETI power representation | 31 + 1 = 32 occupied/reserved slots | 31 active before any additional adapter requirements; external sets must be independently budgeted |
| Industry types | 37 originals + 10 underground/deep mines + 11 Surface plants + 6 geothermal plants = **64**, below 240 and below a 128-type package allowance for the 27 additions | 203 original-table headroom; 176 after this envelope |
| Industry tile definitions | 175 originals + at most 4 logical tile kinds per each of 27 new industry types = **283**, below 512 | 229 spare; footprint cells and animation frames are not separate logical tile kinds |
| Industry cargo arrays | Draft recipes limited to 4 distinct input cargos / 2 output cargos per facility | Below 16/16; heat/power budgets are separate fields |
| Vehicle definitions | 256 originals + 7 ground rail + 2 ground road + 5 tube + 5 space = **275** logical designs | Bound 64,000; tube/space pool and engine integration belong to feature owners |

The 11 new Surface plants are Earth receiver, Rock crusher/receiver, Metal refinery, Chemical works, Battery works, Electronics works, Component works, Robotics works, Mirror-kit works, Solar farm and Nuclear plant. The six geothermal plants are Vent Collector, Vent-Tap Powerhouse, Magma-Loop Power Station, Geothermal Smelter, Lithothermal Battery Works and Mantle Forge. Solar/native power is still provisional pending Group 3's delivery evidence and Group 21's feasibility decision.

Portal bays, exchange bays, rail/road stations and depots, launch centers and their hubs, utility links, tube tracks/terminals/depots, and orbital modules are dedicated facility/tile definitions. They consume no additional industry or cargo identity by default; their feature plans must report changes to this budget. The 19 vehicle designs include the saved return-pod design even though it is not a purchasable fleet. Tubes contain five independent model definitions rather than one refittable model.

Labels were checked against all four original climate tables: COAL/IORE/CORE retain existing meanings; all added labels are distinct from original labels, including `BATY` rather than `BATT`. The current loader permits arbitrary added definitions and compatible GRF replacement; this draft does not authorize those combinations. No runtime registration is delivered by Groups 1-2.

## Group 3 - delivery spike and exact identity boundary

The actual Debug dedicated binary ran both candidates on a 64x64 four-layer world, then loaded and saved each world again. Final successful run: 2026-10-04; four runs completed in approximately 12 seconds. The complete logs and generated saves use the `r2mlvs1-b-` evidence prefix. [Checked results](../../../zz-reach2/upgrades/2026-10/r2mlvs1-b-loader-results.txt) record:

| Candidate | Ore and base icon | Two-input processor | Carrier | Reload result |
| --- | --- | --- | --- | --- |
| Built-in fixture | CORE at slot 12; real base cargo sprite available | Runtime type 37; inputs CORE/COAL at 12/1; output STEL at 9; 2 stock layouts | Appended engine 256, local train ID 116; CORE hold 32; valid stock image index 2 | All reported definition identities unchanged |
| Bundled fixture | CORE at slot 12; same real base cargo sprite available | MLVP/local industry 0 maps to runtime type 7; same inputs/output/layout count | MLVP/local train 116 maps to engine 256; same cargo/hold/image | All reported definition identities unchanged |

**Selected path: bundled first-party NewGRF definitions, with native engine-owned gameplay and utilities.** The bundled candidate uses the existing staged property loaders and IIDS/EIDS/GRF save records. It needs a versioned package, reproducible generator/source, runtime installation under the searchable `newgrf` directory, and an exact profile-owned identity manifest. The built-in candidate additionally needs direct CargoSpec/IndustrySpec/Engine registration, loading-phase initialization of temporary engine/refit data, and its own permanent registration/persistence policy. Those parallel registration paths are unnecessary. No full catalogue was implemented during this spike.

Registration sources: `src/tests/multilayer_loader_fixture.py` generates the 336-byte fixture `src/tests/fixtures/content/multilayer_probe.grf` (GRFID `MLVP`, probe version 0). Its configs and saved worlds live in the same feature fixture directory. It contains Action 8 identity, Action 4 names, one cargo Action 0, an explicit label translation table, one industry Action 0 using a stock steel-mill substitute, and one extra train Action 0. The native candidate's isolated registrar and diagnostics live in `src/newgrf.cpp`, compiled **only** with `OPTION_MULTILAYER_LOADER_PROBE=ON` in Debug. Default builds exclude the mutation hook. The final profile's package identity/version is a separate Group 4/6 contract; the probe is not distributed as gameplay content.

The harness temporarily uses the existing repository-local `build/grftest.cfg`, `build/scripts/game_start.scr`, and `build/newgrf/test_road_type.grf`, restoring their original bytes in `finally`. Evidence, configs, generated command scripts and supplied/generated save inputs remain in this plan's directory; generated command scripts are removed after use. `-X` restricts search to local paths, and `-x` suppresses config saves. Loopback-only dedicated settings avoid external server advertising. No user-wide content was installed or inspected.

Two discoveries are now explicit contract requirements:

1. The industry mapper reuses climate-disabled runtime slots. Preserve original source definitions and saved package/local mappings rather than assuming every new runtime industry ID is above 36.
2. Loading a package once from an arbitrary absolute config path does not make it discoverable by the saved-content scanner. A supported delivery must install it in the runtime's repository-local `newgrf` search directory. The successful bundled reload used that discovery path.

`src/multilayer_content_identity.h` publishes `RequiredMultiLayerContent` and `CheckMultiLayerContentIdentity`. The profile supplies the exact GRFID, MD5 digest, definition version and unique required labels. Validate after loading candidates, before new-world/save acceptance. Refuse missing or inactive packages, repeated package ownership, the loader's Compatible flag, different bytes/version, missing cargo, duplicate required meanings and malformed requirements. Scan actual live CargoSpec slots; never trust the first-wins label map to prove uniqueness. Caller-owned profile state is published only after this returns `None`; Group 4 wires activation and Group 21 owns saved identity round trips.

Focused verification: `build/Debug/openttd_test.exe [multilayer-content-identity]` passed **17 assertions in 2 cases**. It exercises missing/inactive packages, compatible replacements, changed digest/version, duplicate package identity, malformed requirements, missing cargo, and two valid live slots with one required label while `GetCargoTypeByLabel` still returns the first. Both the lookup and validation use existing public cargo accessors; there is no second cargo registry. No broad regression was run.

Reproduction: configure the existing build with `-DVCPKG_MANIFEST_INSTALL=OFF -DOPTION_MULTILAYER_LOADER_PROBE=ON`, build Debug `openttd`, then run `python src/tests/multilayer_loader_fixture.py --run`. Build `openttd_test` for the focused identity tag. Turn the probe option OFF for normal builds. The spike verifies definitions, saved mappings, base-icon existence and stock layout/image references. It does **not** deliver all-input production, new gameplay vehicles, original art, profile persistence, or multiplayer acceptance.

## Group 4 - profile activation and persistence handoff

`src/multilayer_content.h/.cpp` now owns immutable profile lookup, the accepted world policy, candidate package preparation and activated-definition acceptance. Stable identities are Classic **0 / version 0**, Multilayer Industry **1 / version 1**, invalid **0xFF**. The first-party package is **MLVI / version 1**, installed as `newgrf/multilayer_industry.grf`; its exact MD5 and the 30 required labels are compiled from the same catalogue that produces the bytes. No unrelated package may supply an unreviewed adapter. System baseset GRFs are permitted; external industry/vehicle/static sets are refused until a named compatibility profile is tested.

`media/multilayer/cargo_catalogue.json` is the single definition source for 19 additions. The reproducible standard-library builder emits the GRF and immutable C++ manifest together; CMake builds and packages both through the ordinary local content path. Inputs/units/forms/classes follow Group 2. Both Earth and Rock have zero freight base payment; later disposal still charges its explicit cost. Cargo definitions reuse temporary base icons while original art remains with Plan 13. No mines, processors, new vehicles, utilities or excavation behavior are implemented by this cargo-definition family.

Source ordering discovery: FileScan stops at Action 8. Action 14 version metadata must precede Action 8, otherwise an apparently versioned package is read as version 0. The builder follows this ordering and the actual dedicated acceptance verifies version 1.

The new `game_creation.gameplay_profile` selector appears in random-world and heightmap controls and in new-game configuration (`classic` / `multilayer_industry`). It defaults to Classic. Existing widget IDs are retained by appending its ID. The input selector is deliberately `NotInSave`; it is not the saved authority. `MakeNewgameSettingsLive` carries the pending input into world creation. `GenerateWorld` prepares the resolved GRF list, loads actual definitions, validates identity/labels, then publishes policy before simulation generation. A failed candidate stops the dedicated process or returns the UI to the menu with a specific reason. Restart without new settings follows the accepted previous policy rather than a stale pending selector.

### Schema Foundation field handoff

| Authoritative field | Storage meaning | Legacy absence / validation |
| --- | --- | --- |
| `MultiLayerProfileSelection.id` | stable `uint8_t` profile identity | Classic=0; unknown values refuse |
| `version` | `uint16_t` profile contract version | 0 for Classic, exact immutable version for selected Multilayer |
| `legacy` | preserved legacy behavior flag | true when the profile record is absent; normal Multilayer activation sets false |
| `editor_override` | explicit saved scenario-author availability exception | false when absent/new; camera/editor visibility cannot infer it |
| Required content | saved GRF identity and label meanings validated against the selected profile | existing GRF serialization remains owned by its loader; the new profile record must not permit compatible substitution |

`InitializeGame` resets accepted policy to `{Classic,0,true,false}` before a candidate is loaded. The schema owner restores and validates a present record before dependent gameplay resumes. `GetCurrentMultiLayerProfileSelection` exposes a const policy, and new-world activation sets `{selected ID, exact version, selected ID == Classic, false}` only after validation. The no-record migration and editor-default fields are compiled/tested now; **no new chunk format is invented here and no explicit saved profile identity round trip is claimed**. Group 21 consumes Schema Foundation to implement that persistence.

Focused tests: `[multilayer-profile-selection]` passed **44 assertions in two cases**, including unsupported climates/sets, invalid ID, missing meanings/package, atomic refusal and a deliberately different cargo-slot arrangement. The existing `WindowDesc - NWidgetPart validity` case passed **330 assertions**, covering both updated generator layouts. New-game activation and dedicated creation use the same public preparation/acceptance routines; [six actual dedicated cases](../../../zz-reach2/upgrades/2026-10/r2mlvs1-c-profile-results.txt) verified the 11 Classic and 30 Multilayer label/slot maps, and refused Arctic, an external set, missing content, and changed package bytes without accepting a save. This proves shared activation and layout structure, not a live mouse-driven operator review. The explicit profile save/reload check remains in Group 21.

Reproduction: build Debug `openttd` and `openttd_test`, run the focused selection tag, then `python src/tests/multilayer_loader_fixture.py --profiles`. The harness restores existing config/startup bytes and the built package even after failure. Tests complete in well under two minutes.

## Group 5 - calendar and representation policy

Normal Multilayer Industry Deep access unlocks on **1 January 1980 in `TimerGameCalendar`**, including entry, portal/elevator construction and travel, digging, mine/facility/utility construction, stations/depots, vehicle operation/refit/replacement and tube service. Every feature owner must check the destination/world layer and shared policy, including AI/GS and clone/autoreplace paths. Deep geology may be generated before that date, and a camera may display it; neither permits an operation. Standard TBMs never gain Deep capability through this date gate.

Classic/legacy policy retains the existing playable layers. An explicit saved `editor_override` may bypass the Deep unlock; it is never inferred from current editor mode or camera selection and does not silently bypass model/factory introductions. Normal new worlds set that flag false. Unknown profile IDs/versions must refuse access. These are contract rules; per-command enforcement remains with the feature owners.

| Definition | First calendar date | Deep restriction |
| --- | --- | --- |
| Standard TBM / conventional spoil and mineral fleet | 1950-01-01 initial tuning default | Standard TBM excludes Deep; compatible conventional fleet needs 1980 for Deep use |
| Deep-rated TBM / sealed mineral carrier | 1980-01-01 | Deep predicate plus model compatibility |
| Mole-70 | 1970-01-01 | Underground first; Deep from 1980 |
| Crate-75 | 1975-01-01 | Underground first; Deep from 1980 |
| Cistern-80 | 1980-01-01 | Underground/Deep |
| Shield-85 | 1985-01-01 | Sealed transport; conventional Uranium carrier already exists in 1980 |
| Forge-05 | 2005-01-01 | Heavy capsule; Underground/Deep |
| Vent Collector / Vent-Tap Powerhouse / electrical riser | 1980-01-01 | Deep/excavated/commissioned-link prerequisites |
| Magma-Loop Power Station | 2000-01-01 | Shared finite vent heat allocation |
| Geothermal Smelter | 2010-01-01 | Transported ore plus direct heat |
| Lithothermal Battery Works | 2030-01-01 | Transported Lithium/Cobalt/Chemicals plus direct heat |
| Mantle Forge | 2045-01-01 | Transported Refined Metals/Electronics plus direct heat |

Work, stock, recurring charges and energy allocation use synchronized simulation/economy intervals. Calendar slowing/freezing changes technological introductions only; an economy counter or render frame cannot unlock a model. Group 6 supplies the typed Deep predicate, so a consumer cannot pass an economy date accidentally.

Tube budget: five distinct model identities, guideway/junction tiles, terminal, powered service depot, rail/road interchange and power connection. These dedicated facilities add no freight cargo or industry slot by default. Geothermal budget remains six industry definitions within the 64-type/283-tile conservative envelope, with utility conduit/riser/vent records outside cargo registries. Heat and electricity consume metered utility state; the native profile never introduces `POWR`. Chemicals and Propellant stay hazardous liquids in kilolitres (one tonne of game weight per unit); tanker/Cistern compatibility cannot be replaced by container/Crate compatibility. Source catalogue and builder validate these forms.

## Group 6 - public export and field fixture

The authoritative public header is [src/multilayer_content.h](../../../../src/multilayer_content.h). It now publishes all five named producer obligations: `MultiLayerGameplayProfile`, `LayerVehicleCapability`, `GetMultiLayerContentProfile`, `CheckDeepLayerAvailability`, and `UtilityPowerAccount`. Profile lookup returns a const definition or null for invalid IDs. Capability records have explicit transport/commissioning/service domains, cargo classes/labels, saved layer identities, a calendar introduction year, portal permission and spoil-leg policy; unset model/transport/facility values refuse implicit permission. Group 12 supplies the concrete model table.

The Deep predicate takes `MultiLayerProfileSelection` and **`TimerGameCalendar::Date`**. It preserves Classic access, checks exact version, refuses a spoofed legacy flag on a new profile, admits an explicit saved editor override, and otherwise compares with 1980-01-01. It does not inspect editor mode, camera, global economy date or frame counters. Command-level use remains with each feature owner.

Utility records use distinct producer/consumer strong IDs, explicit layer and saved interval identity/duration. Rates are watts, stock/demand/grants/unmet demand are `uint64_t` joules, and the generation remainder is millijoules (0..999). Unbound IDs use `UINT32_MAX`; an unset layer is invalid. Simulation/economy tick duration determines integration, with allocation/bounds/order still finalized in Groups 15-16. No utility registry or runtime allocation pass is invented by this declaration.

`build/Debug/openttd_test.exe [multilayer-content-contract]` passed **35 assertions in two cases**. The type fixture rejects an economy date at compile time, verifies before/on/after 1980 plus legacy/editor/invalid cases, preserves Space=2/Deep=3, checks const lookup and invalid/default capability/utility fields. Sources are in the existing CMake lists. `rg` verified actual exports and implementations before opening the Groups 7-19 entry gate.

## Continuation notes

Current task boundary: Plan 1 Groups 1-23 complete; normal-build validation passed and the plan is ready for its authorized final commit. The nearest Area Mandatory Reading is **Shared interfaces and feasibility records (Groups 20-23)**; its live save/command/display-order sources were reread after compaction. Solar draft types, sprite manifest and Schema Foundation now exist and their focused fixtures pass. Six real metadata/legacy cases and seven saved-policy/refusal cases passed; the native fallback contract burst passed 450 assertions. Plans 12/13/15 delivered only the master's initial M0 handoffs; their later gameplay/art/network work stays unchecked. See the consolidated evidence in [Content Freeze](r2mlvs1-g-content-freeze.md).

After compaction, re-read this plan's Global Mandatory Reading, AD-1, the MultiLayerMap ownership preamble and the cargo-label resolver declarations. For recipes, the relevant local sources are Industry's bounded stock fields and TriggerIndustryProduction: its default multipliers consume each input independently and cannot enforce all-input batches. New recipes use one native consumption owner; their pure planner does not install production behavior. Other local sources are the vehicle command declarations, CargoPayment, AD-15/16 and capsule/geothermal dates; reread only what the current group needs. Named symbols govern ranges: loader ranges moved after the opt-in probe was added. The obsolete assumption that new runtime industry slots always start at 37 must not be restored. The authorized final Git operation is the Plan 1 commit; no reset, checkout or other standalone Git write belongs to this execution.

Final discoveries to retain: registered chunk fixups run even when a chunk was absent, so metadata presence is checked at version-aware acceptance; preview must never stage live policy. Package identity is checked before ordinary network compatibility/sprites, while actual CargoSpec uniqueness is checked after loading. SolarMirrorID is independent of future space Vehicle IDs. Manifest counts are 141/58,059, with final art explicitly undelivered. The normal build disables the experimental loader probe.
