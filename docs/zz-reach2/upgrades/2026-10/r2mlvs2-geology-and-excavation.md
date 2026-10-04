# R2MLVS2 - Geology and Excavation

Status: proposed. Parent: [R2MLVS master](r2mlvs-multilayer-vehicles-and-structures.md). Depends on r2mlvs1 and r2mlvs14 Group 1.

## Outcome and reading

Replace visual bedrock with persistent solid/excavated terrain and finite resource deposits. Read master AD-2, AD-7 and I-1..5/I-9; [map layers](../../architecture/map-layers/archi-map-layers.md), [landscape](../../architecture/landscape/archi-landscape.md), `src/map_layer.cpp`, `src/clear_cmd.cpp`, `src/landscape.cpp` and `src/genworld.cpp`. Consult construction command implementations for each guarded action.

## Group 1 - Storage and deterministic geology

- [ ] 1.1 Choose a compact excavation-state encoding and sparse deposit/job records. Document tile-bit ownership versus auxiliary storage; estimate maximum-map memory before committing. Two bits over two 4096x4096 rock planes cost 8 MiB before metadata; an extra byte costs 32 MiB. Measure the actual design.
- [ ] 1.2 Define finite deposits by layer, resource, shape, reserve, hardness and stable ID. Avoid one heavyweight object per ore tile; specify deterministic overlap and claim rules.
- [ ] 1.3 Generate five resource types per underground layer using separate saved feature seeds. Validate placement, accessible mine footprints and minimum map size. Do not change Surface RNG consumption.
- [ ] 1.4 Add save/load descriptors and feature-disabled defaults alongside the data. Defer neither job progress nor generated reserves until the final save audit.
- [ ] 1.5 Specify survey visibility and pricing. The initial survey overlay may expose deposit locations for planning; revealed information does not excavate tiles or spawn productive mines. Save any information that affects company gameplay.
- [ ] 1.6 Generate persistent Deep geothermal vent fields with a bounded thermal capacity and usable surrounding plant/utility footprints, in addition to all five Deep mineral types. Read master AD-16 and r2mlvs17's vent contract; survey discovery or TBM clearing alone neither generates power nor duplicates vent capacity.

## Group 2 - Excavation and construction contracts

- [ ] 2.1 Implement a common full-footprint excavation check used by rails, roads, tubes, terminals, stops, stations, depots, signals, industries, geothermal plants/utilities, objects and portal approaches. Treat the portal work pocket as an explicit restricted exception. Deep operations additionally obey the 1980 calendar gate.
- [ ] 2.2 Guard demolition/auto-clear, area tools, conversion, terraform, editor operations and script entry points. Editor terrain painting, if provided, is an explicit editor feature with no accidental gameplay bypass.
- [ ] 2.3 Implement reserve/claim/advance/cancel transitions for connected excavation jobs. A job cannot consume the same rock as another TBM; cancellation preserves completed excavation and paid work.
- [ ] 2.4 Define one atomic cut operation with excavation progress, support requirements, material yield and resource accounting. The vehicle controller calls it; UI rendering never advances it.
- [ ] 2.5 Support user-defined area outlines, branch cuts and multi-tile chambers for any underground use, including tubes and geothermal factories. Expand them into reachable passes under the TBM's maximum 45-degree turn rule, validating swept footprint, turn space, return route and spoil capacity. Do not restrict clearing to a future permanent rail alignment or predesignated mine footprint.
- [ ] 2.6 Demolition leaves excavated ground. Resource-bearing cut material is accounted as reserve consumption or retained seam according to the content contract; it must not be extracted again by a later mine.

## Group 3 - Presentation and validation

- [ ] 3.1 Add solid faces, cavity floor, edges/corners, survey overlay, job footprint and progress display through r2mlvs13 assets. Solid rock and a dark open chamber must be visually distinguishable.
- [ ] 3.2 Test all construction families on solid, active and excavated footprints, including one blocked tile in a larger footprint and a placement through an extra viewport.
- [ ] 3.3 Test two competing jobs, partial progress, cancellation, insufficient capacity/funds and save/reload without reserve duplication.
- [ ] 3.4 Verify deterministic generation on square/rectangular maps, all ten guaranteed deposits, void edges and minimum-size failure. Compare feature-disabled Surface digests with the prior baseline.
- [ ] 3.5 Measure memory and active-job tick work on representative large maps; retain results for r2mlvs15.
- [ ] 3.6 Verify vent generation does not replace any required mineral type or pre-excavate a plant. Test area clearing with diagonal turns, irregular outlines and occupied boundaries; reject illegal corner cuts and pre-1980 Deep jobs without terrain/RNG mutation.

## Completion conditions

Every ordinary building/traversal path respects solid rock. Excavation changes only through its authoritative transaction, conserves yields and resumes from saves. All required deposits can support an excavated mine and transport access; simply revealing or clearing a tile cannot create free production.
