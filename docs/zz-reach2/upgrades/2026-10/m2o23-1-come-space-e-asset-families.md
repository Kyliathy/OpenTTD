# M2O23-1 — Logical asset disposition and protected packing boundary

Owner: [Part1 Group8](m2o23-1-come-space.md#group-8---freeze-logical-asset-ownership-before-allocating-offsets). Architecture: [Graphics candidates, expectations and publication](../../architecture/archi-historical-debug.md#graphics-candidates-expectations-and-publication). This inventory selects no new pixels, publishes no GRF and creates no asset catalogue. Existing public media contracts, scopes, converters and native drawing owners remain authoritative.

## Complete family comparison

The frozen [merge-asset-families.json](../../../../src/tests/fixtures/graphics/merge-asset-families.json) records all148 IDs in the union, source hashes/retained destinations, exact public pose-domain comparisons, representative differing/common keys, actual A/B bindings, current or donor native draw boundaries and the existing A producer/packer route. Read only the rows needed for the current family; this851903-byte audit is not a mandatory whole-file read or a runtime dependency.

A declares148 families, B145; **B adds no logical family ID**. A-only `vehicle.model-19`, `vehicle.model-20` and `vehicle.model-21` stay intact. B's90 packed families equal A's43 selected M1 families plus47 selected Ground families. A also protects19 selected Space/orbital/launch families, for109 selected families total. The39 other declarations remain unactivated/reserved. A missing selection never releases their published/reserved ownership.

Comparison uses `(asset_id, state, frame, direction, tile, part)`, then native canvas, anchor, footprint, palette and part rules. It does not infer equality from filenames, total pose count, historical “runtime activated” flags or the whole-GRF checksum recorded in each family. Those bundle checksums necessarily differ across the branches and cannot identify a changed individual source.

| Disposition | Count | Required action |
| --- | --- | --- |
| Identical declared source/geometry identity | 82 | Retain A's existing logical/range owner; preserve source ancestry and reserved declarations. Historical runtime metadata still does not certify this merged executable. |
| A-owned with donor native integration | 45 | Keep A's matching source/pose identity and native packing range. Integrate B's actual draw/provider/controller behavior through A's producer and native consumer boundaries. |
| Source conflict requiring explicit later family review | 2 | Battery Works and Electronics Works: retain both originals and freeze canonical publication until Group48 resolves the examples below. Other families and simulation work continue. |
| A delivered source over B's older placeholder | 16 | Preserve A's current Space/orbital/launch source and geometry. Do not restore B's older placeholder shapes or claimed future company-color policy. |
| A-only delivered family | 3 | Preserve models19/20/21 and all their current native consumers. |
| Donor-only append | 0 | No new family/range is allocated by this inventory. Group49 may append only if later verified pose requirements actually require a distinct new family. |

Sixteen ground families have selected A pixels but require B's incoming native consumer integration: the two feature controls; Vent/Magma powerhouses; tube terminal, service and interchange; utility riser; vent and geothermal conduit; straight/bend/junction/end/crossing/conduit tube art. Their exact `donor_draw_contract` names the real `DrawTile_Rail`, `DrawTile_Station`, `DrawTile_Industry`, `DrawTile_Clear` or underground-window boundary and selector. The audit verified all12 distinct donor draw boundaries against exact retained source epochs. Existing A ground-family dispatch and M1/Space draw symbols also resolve; these static checks are not rendered acceptance.

## Two bounded source conflicts

Both pairs have512 identical logical pose keys, the same declared native anchors and the same geometry. Equality of their matrices does **not** choose the pixels. Begin Group48's review at `(construction, 0, none, 0, back)` and `(working, 0, none, 0, back)`, expanding across occupied cells/parts only if those sources or component recipes differ. These are comparison keys, not a claim that every logical layer contains opaque pixels.

| Family | A original and SHA256 | B retained original and SHA256 | Decision boundary |
| --- | --- | --- | --- |
| facility.battery-works | `battery-works-machinery-a-v3.png`, `9148245cea839dcbb2e97657ff7b73acebe5eaaaf688b76eba698aeb12f5e0af` | `battery-works-machinery-v2.png`, `4fc387dd67c04dd9cd0db5fca3eb5514f5d5696ee3cf3c7e8823bc7522526cf3` | Preserve A's current published family while comparing actual original component selection and independent geometry expectations; do not pick B because its old receipt says accepted. |
| facility.electronics-works | `electronics-works-machinery-a-v2.png`, `e9db13c79b60c4868db62181860fc76ddbda62e8336269a44910d4229a07737a` | `electronics-works-machinery-v3.png`, `b719411122531e1aa8c45b51af28e3636924e77daaaaff917da95d3925f78fab` | Same rule; filename version3 is not evidence that the donor source supersedes A's selected compatible component layout. |

Exact paths, complete original/component lists and source identities are in the corresponding audit rows. Native geometry authority remains [native-processor-geometry.json](../../../../media/multilayer/contracts/native-processor-geometry.json); original source composition remains each retained `processor-generation-record.json`. No regenerated changed output may become its own expected baseline. If both actual variants remain equally valid and the difference is a human art preference, present the concrete comparison for the user's choice at that later task.

## Protected destination ranges

All offsets below are relative to `SPR_MAP_LAYER_BASE`; ends are exclusive. Keep every current header/scope and unselected published slot. Group49 consumes the live scopes again before allocation; this table is the inspected Group8 handoff, not another numeric registry.

| Existing owner | Protected interval | Authority |
| --- | --- | --- |
| M0 and reserved core | `[0,74)` | M1 scope protects0-65 and reserves66-73; omission cannot activate/recycle them. |
| Original M1 families and gaps | `[74,3588)` | `m1-pack-scope.json::preserved_ranges`, generated `MULTILAYER_M1_SPRITE_COUNT`. |
| Independent Space preview | `[3588,3599)` | Nine asteroid previews plus two survey poses; preserve PNG/NFO and native sprite references. |
| Final Space | `[3599,5167)` | `space-pack-scope.json`, `SPACE_SPRITE_START/END`. |
| Orbital | `[5167,6143)` | `orbital-pack-scope.json`, `ORBITAL_SPRITE_START/END`. |
| Launch | `[6143,7791)` | `launch-pack-scope.json`, `LAUNCH_FLEET_SPRITE_START/END`. |
| Ground | `[7791,22045)` | `ground-pack-scope.json`, `GROUND_SPRITE_START/END`; last occupied selected slot22044. |

B's expanded M1 block ends17847 and places `mine.COAL` at3599, directly over A's final Space start. Its `vehicle.model-09` at7943 must resolve to A's existing14183 range instead. These are relocation facts, not reasons to overlay a donor header, GRF, NFO or public contract. No whole donor packed payload is an acceptable import unit. Same-family source/pose identity is sufficient to retain A's established offset; shared names do not force a second family.

## Existing producers and R2FC placement

| Family set | Existing conversion and packing owner | Durable inputs / generated result |
| --- | --- | --- |
| Original43 M1 families | `regenerate_art_sources.produce_source/produce`, existing stage-specific converters and `pack_m1_art.derive` | Original `media/multilayer/source/m1/` records/pixels; run-local candidate images/data/reports. Superseded terrain and reserved slots stay protected. |
| Ground47 families | `regenerate_ground_sources.converter/build_source` and `pack_ground_art.derive` | Existing Ground records/pixels and independent fixture expectations; candidate source manifests/atlases stay in explicit current-run owners. |
| Mines, processors, conventional fleet and stations | Existing `convert_mine_states`, `convert_material_mine_art`, processor/conventional/station modules selected by `regenerate_ground_sources.converter` | Adapt the existing pure producer for the selected family; keep native geometry/part policy and A's current range. |
| Tube fleet/infrastructure | Existing `convert_tube_capsule_states`, `convert_tube_art/curves/junctions/ends/crossings/conduits/terminals/facilities/interchanges` | Original capsule/tube records, geometry and all view/load/state source pixels; no source regeneration or altered train length to fit art. |
| Geothermal/utility/feature controls | Existing vent/generator/riser/utility/control converters selected by `regenerate_ground_sources.ENERGY` | Existing generator/utility/feature-control records; historical log-path atlases are generated ancestry and must be rebuilt, not retained as authored originals. |
| Space/orbital/launch | `regenerate_space_sources.produce_source/produce_packing` and its existing collection-specific packers | A's exact current originals and protected native bindings; B's earlier placeholders supply no replacement assets. |

Group2 already retained competing donor originals under `media/multilayer/source/legacy/openttd3-75ea75718f/` or reused exact existing bytes. The audit reads B's original public manifest from that retained owner, not B's caches. Its per-family source records distinguish authored media from generated `logs/proofs/...` references, including four M1 derived images misleadingly listed in historical `authoring.originals`. Those references remain reproducible ancestry, not missing original-art blockers or permission to revive old log ownership.

The sole `mlm/review-producers.json` catalogue and existing `RunContext`/materialization APIs continue to schedule source candidates and prerequisites. Ordinary recipes never publish canonical assets; explicit reviewed publication remains [Plan4 Group62](m2o23-4-come-space.md#group-62---publish-the-verified-canonical-asset-bundle-explicitly). No native screenshot, old report hash, generated expected image or unchanged source file alone certifies a merged executable.

## Reproduction and next gate

Run `python -B src/tests/multilayer/asset_merge_inventory.py`. It reads only A's live contracts and exact retained donor aliases and writes `test-cache/merge-assets/family-inventory.json` in about two seconds. It cannot publish canonical media or overwrite the frozen fixture. Group8's reviewed observation is retained as the audit fixture linked above; later observations compare against it and may not silently rebase its decisions.

All declared actual authored originals are present; source hashes and all logical pose domains were compared. The two processor source choices are explicitly pending Group48, and all16 incoming draw paths remain subject to actual renderer integration in Plan3 and native qualification in Plan4. These bounded holds do not stop Group9's independently classified imports, but neither task may overwrite those canonical source selections to bypass the hold.
