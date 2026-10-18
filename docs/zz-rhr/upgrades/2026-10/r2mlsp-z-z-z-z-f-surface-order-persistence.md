# Native Surface order persistence completed

Owners: [Plan6 Group13CCC](../../../zz-reach2/upgrades/2026-10/r2mlvs6-mines-and-resource-chains.md#group-13ccc---persist-the-sole-native-order-authority), [Space master](../../../zz-reach2/upgrades/2026-10/r2mlsp-space-master-plan.md). Previous producer: [detached order model](r2mlsp-z-z-z-z-e-surface-order-model.md).

## Checkpoint and next task

**13CCC.1-.3 and original13CC.1 are X.**13CB/13CCB were already X. Source plan, master checkbox and both dispatch paragraphs are updated. Continue **13CCD actual payment/cancellation, once-only dispatch and lifecycle**; no13CCD source work has begun. Original13C.2-.4/13CC.2-.4,13CD driven construction/ordinary game application, Plan11 15B/15/16 and master closure remain unchecked. Physical Surface factory production remains selected as written. No full regression, Git write or main game rebuild occurred. No live session remains.

The user explicitly confirmed **keep physical Surface factory production as written** for final Group15 acceptance. Paid supplies retain their original bootstrap role and cannot replace that final factory proof. The route question is resolved; do not ask it again.13CCD's already planned bootstrap producer remains independent work.

## Mandatory reading after compaction

Read Plan6 Global protocol and relevant AD-7/map-controller/industry-command ownership; Resource production Area; **Actual paid Surface commands and lifecycle Local above13CCD**, its referenced Paid Surface order authority/bootstrap Local and Plan1 Group10 four-label contract. Then only current delivered admission/model/runtime APIs and relevant native command finance, station tick/destruction/rating and company loss/takeover owners. Mandatory reading is non-transitive: no art or old evidence graph replay. The map ownership preamble moved after StoredLayerTilePosition; locate `Layer meaning for the stacked map` / `Map owns the tile storage`.

13CCC finished with one meaningful compaction.13CCD starts at zero. Add/increment the active group note after meaningful compaction; split after exceeding three.

## Delivered native boundary

- `surface_supply_order_runtime.h/.cpp`: sole private installed commitment/history snapshot, independent unpublished SSOR/DATE/MAPS observations, reset/capture/read/next-ID/current-economy-day usage/whole-native validation/stage/adopt. Complete original profile/company/station/date/map joins precede publication. Success clears staging. Reset is wired in multilayer_state.cpp; final adoption in afterload.cpp. Native packets remain all physical stock; loading never creates expense, refund, packet or dispatch.
- `surface_supply_order_wire.h/.cpp`: fixed bounded little-endian SSOR. Header14 = next-ID8 + signed economy-day4 + count2. Record53 = id8,payer1,owner1,station2,x2,y2,label4,quantity2,cost8,inflation8,calendar-year4,economy-day4,packet-buyer2,state1,issued-packet4. Max16384records. Exact shape before allocation; signed-money overflow refusal. Model owns semantic validity; historical packet IDs own no live stock/refund right.
- Schema**440 SurfaceSupplyOrders**, required SSOR RIFF, shared native/copied decoder in saveload/surface_supply_order_sl.cpp. Copied candidate uses only its own DATE/MAPS/PLYR/STNN/profile. Genuine pre440 absence installs empty metadata/next-ID1, no imports.
- Map::DecodeStoredTile owns detached packed geometry; MultiLayerMap alone gives canonical layer meaning. Map::SizeY is already per-plane height; use TileX/TilePlaneY. Native MAPS allocates MAX_MAP_LAYERS even for fewer stored planes; saved order origins remain Surface-only.
- Current full/copied MAPS requires actual dim_x/dim_y/layers, DATE actual economy_date, station base actual owner/xy/facilities. Shared SlStationBase/copied guard excludes constructor defaults; normal/waypoint discriminator mismatch refuses. Fios preview presence/values and station tile/facilities reset per copied file. Duplicate family/date/map evidence refuses.
- `surface_supply_cmd.h`: appended Place/Cancel ABI registered in command.cpp and network/network_command.cpp. **Both estimate/execute still refuse unchanged**. Actual behavior remains13CCD. Enum domain remains below uint8 capacity.
- Original StationCargoList::TruncateAbandonedOffers protects CompanySupply in place, including invalidated buyer. Unrestricted destruction/removal stays original; ordinary-only same-seed RNG is unchanged. **Rating hook stays inactive until13CCD.** No second inventory.

## Exact verification

[Both-build proof](../../../../logs/surface-supply-order-persistence-verification.json), SHA `31ef9c6b238d10283ef4cb0e4b75c599e1e784c0fead2f6fec63eae4f48c1198`; [build/retention receipt](../../../../logs/surface-supply-order-persistence-build-verification.json), SHA `de1a7bd191c995c9a6237467ecbb7f6511d0bf82695bb33510bde332c551eb9c`.

**94actual native processes:** each configuration passes23focused cases/1951assertions in one component process, four valid-file processes and21malformed full/copied pairs (42processes). Tests declare persistence metadata, never actually bought supplies or an unassisted predecessor. Both game and complete model test applications remain hash-identical. Old model applications retained under mlm/reviews/surface-supply/order-persistence-build/20261018-1/original-binaries; receipt logs/surface-supply-order-persistence-retained-tests.json.

Build target **openttd_surface_supply_test**, EXCLUDE_FROM_ALL, links the actual openttd_lib engine with only10fixture/support sources. It follows the existing focused Space-art pattern. Explicit two-worker builds: final RelWithDebInfo build6 and Debug build1. Normal test/game targets remain available. Main games were not relinked. Retain the exact current focused applications before13CCD replaces them.

Final members:

- Components: logs/surface-supply-order-persistence-components-release-4.json and -components-debug-1.json.
- Valid files: logs/surface-supply-order-persistence-{prepare,empty,restore,resave}-release-2.json and corresponding -debug-1.json.
- Refusals: logs/surface-supply-order-persistence-refusal-<case>-release-2.json / -debug-1.json, all21cases from checks-6.CASES. Release missing-day-field uses an earlier fixture at another path with identical source SHA; binder verifies exact content equivalence and current application/source/process identities.
- Final binder: `python src/tests/multilayer/acceptance/surface_supply_checks-7.py --output <fresh logs file>`. Existing final proof must not be overwritten. It verifies all original artifacts, same records in both configurations, genuine older-input ancestry, retained test products and unchanged game apps; it executes no gameplay.

Genuine438 input comes from original orbital-processor-inspected.json reports in logs/orbital-processor-verification.json. Both configurations' after-file SHA is `e04743f82d1b2bbff83a9fbfda549f0279e8af2946c51234317e14f95159bcf8`. These worlds inherit historical declared Surface stock and are persistence-only inputs. Current declared440 fixtures in both builds have SHA `2f09e02c414d5705a91e72ad07ac8bbfe494d005e45e5a43ad1034947eda9ad0`.

## Tests and diagnostics to carry forward

Shared actual command-created fixture is src/tests/surface_supply_fixture.h. Load MockEnvironment/generated language **before** CargoSpecs change. Owner tests use own copied128x64/tile3880=(40,30)/day100 against a different live64x64/(20,20)/day0 world, proving no live borrowing. Wire tests use independent67byte literal, fixed53byte record, explicit original packed geometry and original station-container controls.

Hidden `[.surface-supply-order-native]` enters real openttd_main and post-load surface_supply_order_test. Valid phases prepare/inspect/inspect-empty; refusal phases preview-refusal/full-refusal. Save and validation are actual native paths, not the unchanged ordinary game applications (that remains13CD). A ReInit caller must replace the partial world; this terminal observer explicitly sets PauseMode::Normal before exiting so the current iteration never ticks partial station tiles.

Superseded diagnostics: initial missing network-traits include fixed; full test build2 stopped during unrelated scenario compilation and replaced by focused target; packet component corrected capacity preflight/value initialization; terminal refusal harness corrected pause after ReInit. Final proof uses corrected products only. Current commands-disabled assertions and proof flags are transitional:13CCD must deliberately update affected tests/proofs when enabling actual finance/dispatch, while retaining these dated products and never counting declared restore fixtures as paid stock.

Next assisting prefix: `r2mlsp-z-z-z-z-g-` in this tracked VSCode-excluded archive. No source/test/JSON filename starts r2. No permission for Git writes; current user AGENTS forbids them.
