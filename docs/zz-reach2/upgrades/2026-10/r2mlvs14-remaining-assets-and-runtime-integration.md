# R2MLVS14 - Remaining assets and runtime integration

Owner: [R2MLVS parent](r2mlvs-multilayer-vehicles-and-structures.md). Dispatch: [R2MLVS3MP](r2mlvs3mp-master-plan.md). Completed producer: [Plan13 - Completed graphics foundations and assets](r2mlvs13-graphics-and-asset-pipeline.md). Architecture: [map layers](../../architecture/map-layers/archi-map-layers.md).

Split,2026-10-10: this continuation owns all169 previously unchecked tasks. Original task/group IDs remain stable, including mixed55.3 and79C.3-79C.4. Completed tasks and evidence remain in Plan13. Existing persistence, acceptance, tube and geothermal Plans14-17 are now15-18. This reorganization grants no implementation or art acceptance.

**Collector Group79C/master10.4B and chemical works40C source are complete. The user requested an underground demo before continuing the selected source queue. Next implementation is Group41 battery works under master10.4C.** Vent-Tap55.3/55B, Magma-Loop56, utilities/risers60, controls78C, selected fleet19-22, station/terminal24-25, all seven selected mines and Metal39/39C/39D retain their completed source evidence. Collector native loading and ordinary-game display are delivered; later source families still require master10.4D packing/runtime integration.

## Global Mandatory Reading

> **Execution protocol**
>
> - Read this Global Mandatory Reading section before the first task and again
>   after every context compaction.
> - Before starting or resuming a task group, read the nearest preceding Area
>   Mandatory Reading block whose `Applies to` label includes that group. Area
>   blocks appear immediately before the first group they govern.
> - A referenced heading or exported symbol is authoritative. Global navigation
>   hints and Area line ranges are for navigation only; if they drift, relocate
>   the named section or symbol rather than expanding to the whole file.
> - Mandatory reading is non-transitive. Do not preload a linked file's own
>   Mandatory Reading, Consult on Demand, or Planning Provenance entries unless
>   this plan names them directly for the current area.
> - Do not read Consult on Demand or Planning Provenance merely because those
>   sources are listed. Use a Consult on Demand source only when its trigger
>   occurs.
> - Per the user's 2026-10-07 instruction, store all new build/test/application
>   capture logs in the repository `logs/` directory. Preserve owning-plan
>   basenames and link them here with `../../../../logs/<basename>.log`.
>   Earlier documentation logs were removed by the user; retain historical
>   result summaries without recreating those files. Markdown notes and scratch
>   scripts still belong beside their owning plan.
> - No created source file, test file or JSON file should start with same prefix as a planning MD file (such as r2). Only MD files can have that prefix. No MD file should be created anywhere in source directories unless specifically asked.
> - After EVERY CONTEXT COMPACTION, you must add a note above the task group
>   which is currently in execution. The note will say: "Context has compacted
>   X times while executing this group.". If the compaction occurs before any
>   meaningful work was done (this can happen when there is a lot of baggage
>   from the previous compaction), then and ONLY then you can skip this step.
> - When exceeding 3 context compactions, YOU MUST ask yourself if you're not
>   chewing more than we can handle :). Heck, plans change! :). In such a case,
>   the way out is to split the group on the spot,
>   in the plan, into more groups,
>   by appending B, C, D as a suffixes to those groups (splitting sub-groups
>   themselves is also allowed). When you do this, create Local Mandatory
>   Reading areas so you can optimize your context only for the
>   ACTUAL task at hand.
>   Before starting or resuming a task group, read the nearest preceding Area
>   Mandatory Reading block whose `Applies to` label includes that group. Area
>   blocks appear immediately before the first group they govern.

| Source | Relevant sections | Purpose |
| --- | --- | --- |
| [Plan13 completed contract](r2mlvs13-graphics-and-asset-pipeline.md#public-handoff-contract) | Public Handoff Contract and only the executing family's completed producer | Consume established schema/geometry/source boundaries without redoing completed tasks. |
| [R2MLVS parent](r2mlvs-multilayer-vehicles-and-structures.md#development-artifact-layout) | Development artifact layout; AD-1; applicable ownership invariants | Preserve repository-local source/runtime/review homes and feature-owned state. |
| This plan and [master queue](r2mlvs3mp-master-plan.md) | Current dispatch and nearest applicable Area/Local | Resume the correct remaining scope without treating partial art as runtime delivery. |

Before raster generation/editing, read the [canonical framed ImageGen prompt and extraction checks](r2mlvs13-graphics-and-asset-pipeline.md#framed-imagegen-sheets-and-safe-extraction) plus the installed imagegen skill. After compaction, read only relevant completed producer sections. Preserve all existing native ranges, immutable originals and MLVI gameplay identities. Use focused combined checks and the master's build cadence.

## Outcome and Boundaries

**Delivered definition/queue handoff,2026-10-12:** Plan1 Group12B publishes
`multilayer_space_content.h` revision2 recipes, mass, budgets and paid-facility
admission; Plan8 Group6B publishes native `space_orders.h` queue/generation
ownership. Save410 carries exact `SDEF` content identity and separate bounded
`SPOR` transport, preserving ground/profile/package1 and paid old objects.
Both builds and focused native save/replay/refusal gates pass; see
`logs/space-content-orders-verification.json`. These handoffs grant no missing
commissioning, survey, blueprint, processing, descent, service, UI/art or network
producer. Consume their public definitions/commands; retain unfinished tasks.

**Space revision,2026-10-12:** the current
[three-probe and Steel/Goods contract](r2mlvs1-d-recipes-and-bootstrap.md#space-economy-revision---2026-10-12)
requires distinct mining, survey and construction silhouettes, scan/work/load
states, Steel/Goods cargo views, specialized Steelworks/Goodsworks and
material-shortage/construction-progress blueprint states. New63B/63C tasks
retain these additions; Processor75 must cover both specialized functions.
Existing temporary asteroid/probe sprites are delivered preview art, not final
acceptance. Underground source work remains paused; this note updates Space
requirements without reopening that queue.

Complete the remaining source families, deterministic conversion, native consumers and runtime review for the original complete graphics profile. The [shared asset-family requirements](r2mlvs13-graphics-and-asset-pipeline.md#asset-families) remain authoritative. Collector source/payload evidence is complete through Plan13 task79C.2; actual native loading and ordinary-game rendering remain here. User feedback and later-profile tasks retain their unchecked scope and historical dispositions.

## Public Handoff Contract

Consume the existing `media/multilayer/contracts/multilayer_sprite_manifest.json` schema from Plan13: `asset_id`, `consumer`, `directions`, `frames`, `anchor`, `source` and validated per-family fields. This continuation extends source/payload/runtime coverage through the same owners and public paths; it creates no second manifest, inventory, utility ledger or rendering authority. Completed foundational exports stay in Plan13; new asset delivery and full-profile acceptance belong here.

## Implementation Tasks

### Area Mandatory Reading - Source generation and reproducible conversion

_Applies to Groups 9-81._

**Entry gate.** The following public contracts must exist before dependent implementation in this area: `media/multilayer/contracts/multilayer_sprite_manifest.json`: `asset_id`, `consumer`, `directions`, `frames`, `anchor`, `source`.
Verify each public file/export/schema field with `rg` in the current workspace and inspect its command/field ownership. For a same-plan contract, compile its type/signature fixture or validate its artifact after the preceding producer groups. For cross-plan behavior, also run the producer's focused executable boundary test; declarations alone cannot authorize dependent mutations or runtime claims.
If a required export is absent, stop the dependent implementation, mutation and runtime claims in Groups 9-81. Read-only characterization, source/range verification, test-case design and asset-template work that does not assume the missing behavior may continue.
Do not bypass this gate by copying private pool/tile state, inventing local IDs or cargo registries, using a historical plan as a delivered implementation, opening user-wide content stores, substituting a second utility/accounting authority, or adding an unreviewed fallback path. Finish the named producer or revise the shared contract and its direct consumers first.

**Image-generation gate.** Verify that the execution harness exposes the required image-generation tool and read the installed imagegen skill before generating/editing raster art. If unavailable, stop only raster-generation/edit tasks in this area; manifest checks, placement templates, palette/anchor validation and packing-tool work may continue. Do not substitute unauthorized providers, private caches or a different image-editing route to bypass the skill/tool prerequisite.

| File name | Line Ranges | Description |
| --- | --- | --- |
| [media/multilayer/tools/sprite_quantize.py](../../../../media/multilayer/tools/sprite_quantize.py) | 379-411 | `build_sheet` and `sprite_lines` implement deterministic native packing/anchors and NFO emission; preserve the repeatable source-to-payload contract. |
| [media/baseset/openttd/CMakeLists.txt](../../../../media/baseset/openttd/CMakeLists.txt) | 6-67 | Complete `if(GRFCODEC_FOUND)` and `create_grf_command` block defines PNG/NFO generation inputs; validate the separate checked-in fallback packaging path too. |
| [media/baseset/CMakeLists.txt](../../../../media/baseset/CMakeLists.txt) | 16-34 | Complete payload list and conditional generated-payload marking retain the checked-in `openttd.grf` path when the generator is unavailable. |
| `media/multilayer/contracts/multilayer_sprite_manifest.json` (planned public contract; link after implementation) | Live definitions; no fixed range | [r2mlvs13-graphics-and-asset-pipeline.md](r2mlvs13-graphics-and-asset-pipeline.md) Groups 1-8, ending with Group 8's public-contract task, publish `asset_id`, `consumer`, `directions`, `frames`, `anchor`, `source`. These exports/schema fields are authoritative for this area. Same-plan consumers follow those earlier groups; cross-plan consumers verify the producer's required implementation/tests as well. Proposed names alone do not prove delivery. |

**ASSUMPTION: this was already built previously. If not, re-asses at runtime**. This applies only to the producer contracts named in this area's gate, after their listed producer groups have run. Verify their actual exports and focused tests before using the sketches below; proposed types and helper names in sketches are not evidence of delivered code.

**Conversion sketch.** Asset generation selects original pixels; deterministic processing begins from those retained originals. A later runtime correction updates source/manifest and reruns conversion, never patches only the distributed binary.

#### Framed ImageGen sheets and safe extraction

Read the canonical [prompt, fixed magenta-frame geometry and extraction/model checks](r2mlvs13-graphics-and-asset-pipeline.md#framed-imagegen-sheets-and-safe-extraction) before new raster work. The completed plan retains the measured trial and immutable prompt provenance. Apply its per-output checks and separate-file fallback without regenerating approved sources.

```mermaid
flowchart LR
    Template[Approved geometry and style] --> Original[Selected original raster]
    Original --> Quantize[Palette and anchor-preserving crop]
    Quantize --> Validate[Coverage and mask validation]
    Validate --> Pack[Stable IDs and GRF payload]
    Pack --> Runtime[Real gameplay review]
    Runtime --> Original
```

**ASSUMPTION: this was already built previously. If not, re-asses at runtime**. The preceding Sprite Contract Handoff supplies exact schema/counts/anchors. Verify the manifest fixture before writing the converter; stop dependent packing on an absent/invalid manifest and continue independent source/template work without inventing offsets.

Context has compacted 1 times while executing this group.


### Local Mandatory Reading - Later-family authoring preparation

_Applies to Group 9B when dispatched by a later milestone; master M1 does not dispatch it._

Read this plan's Global and Source generation Area, then only the nonselected manifest families and their actual consumer geometry. Use the installed imagegen skill, repository provenance and original immutability rules. Do not re-author already selected M1 sources.


### Group 9B - Prepare remaining profile asset families

{{MEDIUM}}
- [ ] 9B.1 Apply the image-generation skill/tool and repository provenance workflow to the remaining profile families when their actual consumer contracts are ready.
- [ ] 9B.2 Publish bounded approved geometry/style briefs for each remaining family/model, preserving the original whole-profile authoring requirement.
- [ ] 9B.3 Retain immutable selected originals and independent revision history for those families before deterministic conversion.


### Local Mandatory Reading - Remaining ground family originals

_Applies to remaining Groups19-46; completed families remain in Plan13._

Read this plan's Global and Source generation Area, the executing family's manifest row, and only its actual vehicle/industry/station geometry owner. Use the completed Plan13 style/registration contract and canonical framed prompt. Preserve the selected model across every required view/state; do not redo completed exchange, service or receiver families. Missing geometry/runtime producers block only their dependent source claims.

### Group 19 - Author underground freight locomotive

Resumed2026-10-11 under master10.4C after related controls78C. [The selected conventional handoff](portal-tbm/r2mlvs3mp-y-ground-producer-handoff.md#conventional-fleet-continuation) identifies native model4 as the already delivered hauler/ordinary freight locomotive. Reconcile and verify its existing body/purchase sources and native geometry before marking19 complete; do not generate a duplicate locomotive or reinterpret mineral wagon model5 as traction. The historical finalization skip is superseded for this selected scope.

{{HARD}}
- [X] 19.1 For underground freight locomotive, derive a single-model brief from its approved manifest/template: conventional mineral-freight traction silhouette. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [X] 19.2 Produce the required directions/rotations for underground freight locomotive from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [X] 19.3 Add running/idle/service and purchase views for underground freight locomotive, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.

Closure,2026-10-11: verified reuse of the delivered M1 compact diesel shunter, native model4, brick-red cab/charcoal short hood/ochre steps and cream headlights. Its64x48 canvas,(-32,-32) anchor, eight genuine directions and fixed couplers/chassis remain exact. The focused source audit regenerates all80 poses (five states/two frames/eight views), checks distinct frames, unchanged opacity and all pixels outside declared equipment regions, and rejects absent families, mismatched reports and broken crop tables. Tiny selected lamps/running gear from other approved M1 originals supply components only; no cutter body is substituted. Existing available/locked20x20 purchase originals regenerate exactly. The native engine/purchase/exchange-art boundary passes7,571 assertions/seven cases. All82 packed sprite pixels/anchors match these exact atlases at1136..1215 and1216..1217; the current loaded payload identity and existing bindings remain unchanged. The model-specific body/purchase atlases are retained contact sheets. No new raster, native build, full regression or personal-review acceptance is claimed. [Reuse brief, provenance and receipt](../../../../logs/graphics/freight-locomotive/20261011-reuse-01/verification.json).

### Local Mandatory Reading - Ordinary mineral wagon originals

_Applies to Group20; sealed rail wagon model06 follows in20B, and sealed road carrier model08 remains22._

Read Global/Source generation Area, the selected conventional handoff, public `vehicle.model-05`/`purchase.model-05`, actual `GetUndergroundEngine(5)` definition/cargo/length and native direction/coupler consumers. Consume Plan13's completed style/registration and canonical framed prompt plus the installed imagegen skill. Preserve the delivered Earth/Rock hoppers (models02/03); model05 is separate ordinary mineral freight. Freeze the selected original's camera, anchors, wheel contacts and cargo opening before load/state variants. Source-only coverage grants no runtime activation.


### Group 20 - Author mineral wagon

Resumed2026-10-11 under master10.4C after verified19 reuse. This group owns selected ordinary rail mineral wagon model05 and its purchase family. The distinct selected sealed rail wagon model06 is bounded20B below; all original selected-fleet scope remains in the child plan.

{{HARD}}
- [X] 20.1 For mineral wagon, derive a single-model brief from its approved manifest/template: profile-compatible ore load presentation. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [ ] 20.2 Produce the required directions/rotations for mineral wagon from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [X] 20.3 Add each required refit/load and purchase view for mineral wagon, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.

20.1 selection,2026-10-11: actual model05 is the1950 unpowered full-length8-unit/60-capacity mineral and heavy freight wagon, with17 live cargo labels. Selected olive open gondola E original v2 retains three side bays, pale rim, two two-axle bogies, two couplers and one A-end brakewheel. Measured rectangular alpha16 crop excludes all source magenta and retains at least26px transparent clearance; native64x48/(-32,-32) review yields32x10 opaque bounds on the fixed40px baseline. Flat v1 is retained as rejected. [Immutable originals, exact built-in prompts, hashes, native transform and selected brief](../../../../media/multilayer/source/ground/fleet-generation-record.json). Seven directions, all other states/refits and purchase remain pending; source coverage, packing and runtime remain false. The existing native producer boundary for19 also verifies model05 identity/refit slots.

20.2 closure,2026-10-11: all eight genuine empty frame-zero views are selected from three immutable originals; the flat first E draft remains rejected, and the later western sheet's repeated E is explicitly excluded. Neighboring views preserve three side bays, pale rim, two bogies/four axles, both couplers and one physical A-end brakewheel, with fixed screen light. All measured rectangles have alpha16 clearance>=16 and no opaque magenta. Native DOS palette/transparent index0,64x48/(-32,-32), direction envelopes and40px wheel baseline pass eight focused source cases in4.023s; all three base artifacts regenerate exactly. [Contact sheet](../../../../logs/proofs/mlm/native-art/ground/vehicle-model-05-base-contact.png), [base receipt](../../../../logs/graphics/mineral-wagon/20261011-base-01/verification.json). At this base closure the draft body row had80 poses;20C below resolves all17 actual refits. Full states/refits and packed/runtime delivery remain separate.

20.3 is complete through20C-20E,2026-10-11. Inspection found17 actual native model05 cargo labels, including heavy/packaged freight, beyond the old brief's apparent ore-only scope. The bounded groups below delivered that complete existing source requirement before20B sealed rail wagon/21 mine truck/22 sealed road carrier. This child-only split introduced no master task and changed no gameplay cargo capability. Packing/native activation remain10.4D.

### Local Mandatory Reading - Ordinary wagon bulk refits

_Applies to Group20C only; supplements the completed Group20 base geometry._

Read Global/Source generation Area, actual `_mineral_cargo`/model05 in `src/multilayer_vehicle_catalogue.cpp`, model05's native refit/capacity/length definition in `src/underground_vehicle.cpp`, the public body/purchase rows, `fleet-generation-record.json` selected registrations and `convert_ground_fleet_art.py`. Verify the existing focused `[underground-engine]` producer boundary; the current19 receipt already records that pass. Resolve the source matrix for all17 actual labels, actual packet-backed fill states, per-view cargo aperture/foreground and native wheel/contact ownership before new loads. Read canonical framed prompt/installed imagegen skill before raster work. Reuse approved components only when their retained pixels fit the actual wagon. Current native `GetMultiLayerTrainSprite` binds model4 only; new model05 source never claims activation before10.4D. The catalogue currently admits RARE and later heavy/packaged labels on model05; artwork cannot silently remove or redefine those capabilities.

### Group 20C - Author ordinary wagon bulk refits

Bounded2026-10-11 from compound20.3 after actual model05 inspection. Empty body/eight headings are complete; original20.3 closes only after20C-20E.

{{HARD}}
- [X] 20C.1 Freeze the full17-label refit/source mapping and actual cargo/state/length ownership. Resolve compatible shared-manifest variants and per-view cargo apertures/occlusion with fixed chassis, wheel and coupler registration; retain honest missing-source coverage.
- [X] 20C.2 Deliver genuine COAL/IORE partial/full presentations for all eight views using the selected body and original cargo pixels; verify readability and body/contacts outside admitted load regions.
- [X] 20C.3 Deliver genuine CORE/BAUX partial/full presentations for all eight views with the same fixed body and independently retained cargo identities.
- [X] 20C.4 Deliver genuine SALT/LITH partial/full presentations for all eight views; retain distinct materials at native palette scale without recoloring another cargo procedurally.
- [X] 20C.5 Deliver genuine NORE/COBT partial/full presentations for all eight views; verify complete bulk-refit coverage and source-only contacts before advancing.

20C.1 closure,2026-10-11: the shared manifest now declares17 actual native cargo-label variants/1,360 required body poses, retaining every live model05 label and all prior packed fields. Source-only state policy reads real Train packets/capacity, actual depot occupancy and the existing head reservation-Stuck flag; loaded packets take priority over empty service/hold cues. Existing native8-unit length/shorten0 and actual movement own placement/gear phases. Eight approved bulk full/partial apertures remain within original body opacity, partial fits full, and raised protected-load envelopes preserve foreground rims/wheels/couplers. [Geometry guide](../../../../media/multilayer/guides/ground/mineral-wagon-cargo-apertures-v1.png) and `fleet-generation-record.json` freeze the exact body/geometry signatures. Isotropic original granule interiors may supply bulk material only; crates/ingots need actual oriented components. Source-only catalogue/schema gate145 families/54,701 required sprites passes. No cargo original, full family coverage, new range or native model05 activation is claimed by this geometry step.

20C.2-20C.5 closure,2026-10-11: two retained built-in originals supply eight independently authored isotropic material interiors: COAL/IORE/CORE/BAUX/SALT/LITH/NORE/COBT. Whole-patch windows preserve alpha16 clearance>=16 and contain no opaque magenta; selected interior crops retain original opaque granules. Deterministic BOX sampling places those pixels inside each genuine body heading's approved full hold, with partial selecting its rear bank from the same material coordinates. No whole-body rotation, procedural recoloring or directional object substitution occurs. All128 frame-zero load poses remain distinct/readable, preserve chassis opacity, wheel/coupler pixels outside the hold and exact foreground steel; filling adds grains without repainting the existing bank. Fourteen artifacts regenerate exactly. Eleven distinct bulk boundary cases plus eight base cases pass; the additional pointer refusal and exact artifact recheck ran only after adding that source-matrix guard. Shared schema/refusal checks145/54,701 pass. All43 existing native packed families/routes, payload and four binaries remain exact; no C++build/full regression or native model05 activation. [Closure receipt](../../../../logs/graphics/mineral-wagon/20261011-refits-01/verification.json), [native material comparison](../../../../logs/proofs/mlm/native-art/ground/vehicle-model-05-bulk-materials-contact.png). The staged matrix is264/1,360 (128 loads plus136 explicit empty-frame0 aliases);1,096 source poses, all purchase art and packing/runtime remain pending. Original20.3 stays unchecked through20D/20E.

### Local Mandatory Reading - Ordinary wagon protected and heavy freight refits

_Applies to Group20D only._

Read Global/Source generation Area,20C's delivered public refit/aperture mapping and selected fixed body, and only the actual RARE/RFMT/STEL/SCMP/RBPT/ELEC/BATY/SMIK/GOOD catalogue/packet definitions and reusable retained cargo components. Preserve all17 native admitted labels and fixed vehicle geometry; source cargo layers are presentation, never new inventory or handling jobs. Canonical framed prompt and installed imagegen skill apply to new raster work. Keep every required partial/full heading and native foreground cut explicit.

Context has compacted 1 times while executing this group.

### Group 20D - Author ordinary wagon heavy and packaged refits

{{HARD}}
- [X] 20D.1 Deliver native-readable RARE protected-container and RFMT/STEL ingot partial/full presentations in all eight headings, retaining their distinct original cargo identities and actual model05 capabilities.
- [X] 20D.2 Deliver SCMP/RBPT component/robotics partial/full presentations in all eight headings with correct cargo occlusion and unchanged body/contact pixels outside load regions.
- [X] 20D.3 Deliver ELEC/BATY protected packaged partial/full presentations in all eight headings; reuse exact approved crate components only where source/geometry evidence permits.
- [X] 20D.4 Deliver SMIK/GOOD partial/full presentations in all eight headings and verify all heavy/packaged refits against the complete17-label source mapping.

20D closure,2026-10-11: all nine remaining refits have144 genuine partial/full frame-zero load poses across eight headings. Six protected cargo labels explicitly reuse retained complete closed packages; SCMP reuses the retained slab module; RFMT/STEL share the actual single cast bar from two new selected originals. Labels and native variant slots remain distinct. The first multi-ingot sheet is retained/rejected for inconsistent visible inventory. Whole-object extraction preserves original alpha and measured clearance (packages>=20px, new ingots>=30px; unchanged legacy modules>=14px against their approved8px reuse minimum). Native1x/4x review and15 focused cases pass, including correct rear partial bank, far-to-near full depth, exact foreground steel, unchanged contacts, actual native forms, wrong/missing headings and forbidden source/projection substitutes. All25 artifacts regenerate exactly;43 packed routes, existing payload and four native binaries remain exact. [Closure receipt](../../../../logs/graphics/mineral-wagon/20261011-objects-01/verification.json). The staged body matrix is408/1,360 with952 motion/status poses missing; purchase, whole source coverage and packing/runtime remain pending. Original20.3 stays unchecked through20E. No C++build/full regression or native model05 activation occurred.

### Local Mandatory Reading - Ordinary wagon states, purchase and source closure

_Applies to Group20E only._

Read Global/Source generation Area, only20C/20D's actual source matrix and fixed native registrations, ordinary native Train movement/depot/hold and packet owners, and public body/purchase state rows. Verify real state admission before authoring cues; no invented blocked reason, independent load quantity or animation clock. Source art remains inactive until10.4D. Read canonical framed prompt/installed imagegen skill for new originals; retain immutable component/purchase provenance and exact coverage/refusal checks.

### Group 20E - Close ordinary wagon states and purchase sources

{{HARD}}
- [X] 20E.1 Deliver both required authored native frame phases plus actual service/blocked presentations, modifying only declared moving/status components and preserving packet-backed load priority, chassis alpha, wheel contacts and couplers.
- [X] 20E.2 Deliver selected available/locked20x20 purchase originals for native model05 with exact provenance, alpha/palette and model readability checks.
- [X] 20E.3 Verify complete body/refit/state/view/frame and purchase source coverage, exact regeneration and independent source refusal cases. Publish the shared source handoff and close original20.3; preserve all existing packed ranges and leave new packing/runtime for10.4D.

20E closure,2026-10-11: the shared source handoff now binds all1,360 body poses (17 actual cargo labels, five states, two authored phases, eight genuine headings) and both20x20 purchase poses. Two new wheel originals supply only frozen axle interiors; four glass interiors from the selected replacement status sheet supply two cyan service/two red actual reservation-hold phases over static gear. The first status sheet is retained/rejected for opaque magenta. Measured whole-source clearance is>=17px for gear and>=117px for selected glass, with zero opaque separators in selected windows. Explicit company remap uses only the two reviewed original painted-panel pixels per view. Purchase art reuses the own-model empty/E original and genuine existing calendar-lock badge, preserving every available-body pixel. All five native1x/4x phase contacts and purchase contact pass;15 distinct focused source cases pass, including independent all-pose pixel deltas, cargo/alpha/contacts, original ancestry, complete source/phase coverage, native phase collapse and stale-proof publication refusal. A faulty negative crop fixture was corrected to touch the measured subject edge; its final boundary burst passes. All34 artifacts regenerate exactly, retaining25 earlier base/bulk/object artifacts byte-for-byte. Shared schema145/54,701,43 existing native routes/payload, four binaries and model04 reuse remain exact. [Closure receipt](../../../../logs/graphics/mineral-wagon/20261011-states-01/verification.json). Original20.3 isX; no missing model05 source pose remains. No C++build/full regression/new packed range or native model05 activation is claimed. Resume selected20B.1 sealed rail wagon before21/22.

### Local Mandatory Reading - Sealed rail mineral wagon originals

_Applies to Group20B only._

Read Global/Source generation Area, the selected conventional handoff, public `vehicle.model-06`/`purchase.model-06`, actual `GetUndergroundEngine(6)` and its sealed cargo/date/layer/length consumers. Reuse Group20's camera/registration tooling where actual geometry agrees, preserving a distinct enclosed rail body and real sealed-cargo identity. Read the canonical framed prompt and installed imagegen skill before raster work. Sealed road model08 remains Group22; no omitted family or copied cargo owner is authorized.

### Group 20B - Author sealed mineral rail wagon

Bounded2026-10-11 to account explicitly for the selected conventional handoff's separate model06 before advancing to mine trucks. This preserves original10.4's selected fleet scope without adding a master task.

{{HARD}}
- [X] 20B.1 Freeze actual model06 rail geometry/cargo capability and select one distinct enclosed mineral-wagon original with retained prompt/provenance and native-scale review.
- [X] 20B.2 Deliver all eight genuine directions with fixed couplers, wheel contacts and anchors; preserve the selected enclosed body across neighboring views.
- [X] 20B.3 Deliver required cargo/service/blocked and available/locked purchase presentations using only declared authored components; verify complete native source coverage and model-specific contact sheets before packing.

20B.1-20B.2 closure,2026-10-11: actual model06 remains the1980 full-length8/shorten0 rail wagon, capacity40/speed80/weight24, with only native URAN sealed drums and RARE covered granules on its existing permitted ground layers. Four retained originals supply eight genuine closed blue-gray views: two roof hatches, five ribbed side bays, two two-axle bogies, two coupling ends and one A-end handbrake. The crowded W cell/repeated E in the opposite-side sheet are excluded; an isolated W original supplies safe complete coverage. All measured alpha16 source windows retain>=17px margin and zero opaque magenta. Native1x/4x review, six base boundary cases and one complete ordinary-wagon compatibility case pass; a test's initial URAN-form assumption was corrected to the actual catalogue's sealed_drums. Shared camera tooling refuses foreign model silhouettes in either direction; all34 completed model05 artifacts remain exact. Source schema145/54,701,43 packed bindings/payload and four binaries remain exact. [Base receipt](../../../../logs/graphics/sealed-wagon/20261011-base-01/verification.json). Base coverage is8/80 poses;72 load/motion/status and both purchase views remain20B.3. Public model06 source stays planned; no packed range/native activation or new C++build/full regression. Equipment plates are a frozen bilateral layout (reference side near A, opposite side near B); the A-end handbrake never changes end.


20B.3 closure,2026-10-11: all80 body/two purchase sources are authored through the shared manifest, with no missing pose. The own06 cover/hatches/alpha/couplers/contact row remain exact. Two actual same-heading approved conventional axle-interior samples retarget only onto inspected06 wheel faces; no05 shell, chassis or cargo is copied. Four original nondirectional cyan/red glass phases supply actual empty service/hold cues over static gear. One/two original M1 amber-glass fitting pixels communicate packet-backed partial/full while both native URAN drums/RARE granules stay sealed inside. Company remap uses only actual visible blue plaques; N/S side fittings are naturally hidden. Own06 E preview and retained future-lock original preserve every available preview pixel; its18x6 fit has17 opaque columns at the native alpha threshold. All five native1x/4x phase contacts and purchase contact pass. Twelve distinct state/refusal cases plus one ordinary source compatibility case pass; the purchase fixture's opaque-width assumption was corrected to the actual source alpha. Together with six base cases and the shared compatibility case, Group20B has19 distinct focused cases. All12 sealed artifacts regenerate exactly, preserving the three prior base artifacts and all34 ordinary wagon artifacts. Sharedschema145/54,701,43 native packed routes/payload and four binaries remain exact. [Complete sealed-source receipt](../../../../logs/graphics/sealed-wagon/20261011-states-01/verification.json). Group20B is complete; no C++build/full regression, new range or native model06 activation occurred. Resume selected21.1.

### Local Mandatory Reading - Mine truck originals and native road geometry

_Applies to Group21 only._

Read Global/Source generation Area, selected conventional handoff, public `vehicle.model-07`/`purchase.model-07`, actual `GetUndergroundEngine(7)`/`InitialiseUndergroundEngineDefinitions` road fields, exact `_bulk_cargo` labels/forms and native road image/length/contact consumers. Keep the compact road cab/open freight body distinct from rail wagons and sealed road model08. Reuse approved original isotropic cargo interiors and generic glass only through explicit native geometry/provenance; Earth/Rock retain their actual cargo meanings and mandatory TBM rail-wagon haulage owner. No copied registry or art-driven cargo transfer. Read canonical framed prompt and installed imagegen skill before new raster. Freeze actual road wheel/contact/anchor and capability geometry before refit/status variants; source remains inactive until10.4D.

Context has compacted 1 times while executing this group.

### Group 21 - Author mine truck

Selected dispatch resumed,2026-10-11, under master10.4C after complete ordinary/sealed rail source groups. The earlier master finalization skip is historical; all three selected model07 source tasks below are active in order. Packing/runtime remain10.4D and actual native consumers.

{{HARD}}
- [X] 21.1 For mine truck, derive a single-model brief from its approved manifest/template: compact underground road freight body. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [X] 21.2 Produce the required directions/rotations for mine truck from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [X] 21.3 Add required load/refit/service and purchase views for mine truck, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion. Bounded execution is21B cargo followed by21C state/purchase; both source groups are complete.

21.1-21.2 closure,2026-10-11: four selected immutable originals supply all eight genuine views of the compact yellow-cab/teal-open-bed, three-axle rigid road truck. The shallow perspective prototype and angled nominal N cell are excluded; isolated vertical N corrects the latter. Measured alpha16 clearance is at least16px, with zero opaque separators; SW uses a measured rectangle extending below its omitted nominal frame while retaining35px actual clearance and no neighbor. Native1x/4x continuity review passes, fixed64x48/(-32,-32)/contact40/full8-unit road geometry is frozen. Seven new source/refusal cases and two complete-source wagon compatibility cases pass; all46 model05/06 artifacts, existing native payload/manifests and four binaries remain exact. Public schema145/55,421 preserves the ten actual model07 dry-bulk meanings and road-owned state policy. [Base receipt](../../../../logs/graphics/mine-truck/20261011-base-01/verification.json). This is eight empty frame-zero sources, not800 complete body poses; public source coverage, purchase and native activation remain pending. No C++build/full regression. Continue21B.1.

### Local Mandatory Reading - Mine truck bulk holds and material originals

_Applies to Group21B; implements the load/refit portion of21.3._

Read Global/Source generation Area, completed21.1-21.2 base receipt and actual07 registrations, public07/native ten-label contract, and only the existing bulk donor extraction owner in `convert_ground_fleet_art.py`. Inspect actual truck bed floor/rim pixels before defining per-heading apertures. Reuse approved original isotropic granule interiors with explicit source/geometry evidence; inspect retained M1 Earth/Rock grains before any new raster. Do not copy rail-body masks, recolor cargo or change actual TBM spoil transport authority. Freeze road cargo geometry with body registrations before conversion; all source coverage remains incomplete until21C.

### Group 21B - Fill the actual mine truck bulk holds

{{HARD}}
- [X] 21B.1 Freeze actual truck partial/full hold apertures and foreground rims against all eight selected native road bodies; retain unchanged cab, steel, tyres, alpha and contact.
- [X] 21B.2 Register original material interiors for all ten actual native cargo labels, reusing approved grains only with exact provenance and physical cargo-form checks.
- [X] 21B.3 Convert and review all160 partial/full frame-zero refit poses plus80 explicitly aliased empty slots; verify focused geometry/provenance refusals and exact existing-source preservation.

21B closure, 2026-10-11: `convert_mine_truck_art.py` publishes 160 partial/full frame-zero poses and 80 explicit empty aliases across all ten actual cargo refits. The road bed has its own frozen floor polygons and retained foreground rims; cab, steel, tyres, alpha and contact remain exact. Eight approved mineral interiors and two genuine M1 Earth/Rock interiors supply unchanged original material pixels, with no new raster, heap outline, copied rail mask or cargo recoloring. All ten partial/full native1x/4x contact pages, geometry and material comparisons were inspected. Ten distinct bulk/refusal cases and two complete-source wagon compatibility cases pass. All 17 truck artifacts regenerate exactly; 46 wagon and three prior truck-base artifacts, 43 native packed routes/payload and four binaries remain exact. Schema145/55,421 passes. [Bulk source receipt](../../../../logs/graphics/mine-truck/20261011-bulk-01/verification.json). Coverage is240/800 body slots, with560 still missing; whole21.3, purchase and runtime remain pending. No C++build/full regression. Continue21C.1.

### Local Mandatory Reading - Mine truck road states and purchase

_Applies to Group21C; completes the state/purchase portion of21.3._

Read Global/Source generation Area,21B source receipt and frozen truck geometry, public07/purchase07, actual road head movement/blocked counter and `RoadVehicle::IsInDepot`/`IsRoadDepotTile`. Loaded packets take presentation priority; moving tyre phases require actual moving head, while empty service/hold glass keeps tyres static. Inspect own road wheel/roof/blue-door pixels; shared generic original material may not import rail chassis or another body. Read existing status-glass and future-lock source owners only as needed. Publish complete source only with800 genuine body slots and two own purchase states, exact regenerated artifacts and refusal checks. Packing/native binding remains10.4D.

### Group 21C - Complete mine truck states and purchase sources

{{HARD}}
- [X] 21C.1 Register and convert original tyre-motion, empty service/traffic-hold and explicit company-color parts against frozen road geometry, preserving packet-load priority and fixed body contacts.
- [X] 21C.2 Derive both own model07 purchase states from its approved E original and retained future-lock component, preserving the complete available preview.
- [X] 21C.3 Review and verify all800 body/two purchase sources and exact artifacts; publish truthful source coverage and close original21.3 only after focused refusal/preservation checks pass.

21C/original21.3 closure, 2026-10-11: `convert_mine_truck_states.py` publishes800 body/two purchase source poses through the shared manifest, with26 exact artifacts and12 retained originals. Three genuine road hub pixels in six headings use approved same-heading neutral metal interiors; N/S end elevations hide the hub faces and retain explicit physical-occlusion phase aliases. No rail shape or fictitious side wheel is introduced. Four original cyan/red glass samples use the actual roof fitting for empty service/traffic hold while tyres stay static. Actual packet loads keep their material; explicit company masks touch only original cab-door plaque footprints, including native BOX blends. Own E purchase and separate original future-lock preserve every available preview pixel. All five native1x/4x phase pages and purchase were inspected;12 state/refusal cases pass in14.518s. The whole truck source has31 distinct focused cases: seven base, ten bulk, twelve state and two shared wagon compatibility cases. All46 wagon and17 prior truck bulk artifacts,43 native packed routes/payload and four binaries remain exact; schema145/55,421 passes. [Complete mine truck source receipt](../../../../logs/graphics/mine-truck/20261011-states-01/verification.json). Public07/purchase07 are authored/source-complete with packed fields null and runtimefalse. No C++build/full regression or native activation. Original21 and both child groups are complete; continue22.1.

### Local Mandatory Reading - Sealed road carrier originals and native identity

_Applies to Group22 only._

Read Global/Source generation Area, selected conventional handoff, public `vehicle.model-08`/`purchase.model-08`, actual `GetUndergroundEngine(8)`/`InitialiseUndergroundEngineDefinitions`, native model08 cargo labels/forms/date/layers and ordinary road length/image/state owners. The sealed road carrier needs its own cab, closed freight enclosure and road tyre geometry; model06 rail shell and model07 open bed are not body donors. Reuse only compatible camera/tooling and explicitly registered original material/glass/lock interiors. Freeze native road contacts/anchors and the sealed exterior before variants; actual packet-backed indicators must never open the cargo enclosure. Canonical framed prompt and installed imagegen skill apply before raster. Source completion does not activate native ranges; packing/display remains10.4D.


### Group 22 - Author sealed mineral carrier

Selected dispatch clarification,2026-10-11: this group owns sealed road carrier model08 and its purchase family. The distinct sealed rail wagon model06 is Group20B; do not omit either model or conflate native road and rail geometry. The historical finalization skip is superseded when this selected group is reached under master10.4C.

{{HARD}}
- [X] 22.1 For sealed mineral carrier, derive a single-model brief from its approved manifest/template: deep-capable closed cargo body. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [X] 22.2 Produce the required directions/rotations for sealed mineral carrier from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [X] 22.3 Add required sealed-load/service and purchase views for sealed mineral carrier, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.

22 closure, 2026-10-11: four selected body originals supply all eight genuine views of the distinct brick-red cab, silver-gray closed box and three road axles. Wrong longitudinal roof joints in the first N/S cells were corrected to the same transverse joint; unselected end cells and continuity repeats cannot supply native headings. Every measured alpha16 source margin is at least43px, with no opaque separator. `convert_sealed_road_states.py` publishes80 body/two purchase poses through the shared manifest, using nine retained originals and12 exact artifacts. Actual URAN drums/RARE covered granules share one closed exterior, capacity24/year1980 and road-owned state policy. One/two brighter original M1 amber glass samples (source phase1, fixed material palette192) indicate packet-backed partial/full in the roof module; the enclosure stays shut. Six genuine headings use three original neutral road-hub material points; N/S hubs are physically hidden and explicitly alias their appearance. Empty service/traffic glass preserves stopped tyres; company colors touch only own cab-door plaques. Own E purchase and genuine future-lock retain every available preview pixel. All five native1x/4x phase pages and purchase pass review. Sixteen new source/refusal checks pass8.614s; three complete-source compatibility cases pass25.647s, preserving all72 model05/06/07 artifacts. Schema145/55,421,43 native packed routes/payload and four binaries remain exact. [Complete sealed road source receipt](../../../../logs/graphics/sealed-road/20261011-source-01/verification.json). Public08/purchase08 are authored/source complete, packednull/runtimefalse. No C++build/full regression or new native activation. Continue24.1.

### Local Mandatory Reading - Native conventional station tile contract

_Applies to Group24B, the geometry prerequisite of24.1._

Read Global/Source generation Area, `GetStationTileLayout`, `RailStationTileLayout<StationType::Rail>::Iterator::operator*`, `CmdBuildRailStation`, `GetRailStationAxis` and `CheckExcavationConstructionArea`. Inspect only default rail layouts0..7 in `src/table/station_land.h`, actual parent/child `DrawTileSprites`/`SpriteBounds` and Plan7's ordinary facility fixture. The native station has four modular roles on two axes, with rear/front platform pieces and a separate elevated roof on canopy roles; it has no fixed3x2 footprint or four directed entrances. Construction/refusal art is preview-only because ordinary stations build atomically. Publish read-only geometry and focused native proof before dependent originals; source-only metadata creates no station, cargo or draw state. Keep current43 packed ranges and completed vehicle sources exact.

Context has compacted 1 times while executing this group.

### Group 24B - Publish actual conventional station source geometry

This bounded prerequisite executes the geometry portion of24.1 before original raster selection. It preserves all original24 tasks and future native packing/display obligations; no master checkpoint is added.

{{HARD}}
- [X] 24B.1 Export read-only default station geometry from the actual native layout owner: all eight graphics IDs, four roles, two axes, platform/roof bounds and safe refusal for foreign/custom/Surface or invalid tile queries.
- [X] 24B.2 Publish a checked source geometry descriptor and reconcile the draft station art matrix to modular cells, actual approaches, separate elevated roof ownership and truthful preview/loading state semantics.
- [X] 24B.3 Run focused native geometry/construction/approach and source-contract checks, preserving existing packed art; hand the verified descriptor to original24.1-24.3.

24B closure,2026-10-11: the compiled read-only export projects all eight actual default rail layouts, four roles/two axes and native back/front/elevated roof bounds, retaining the unusual Y rear-pillar height.48 actual variable station configurations cover both rock layers/axes, short/long and odd/even platforms, independently refusing missing near or far end approaches before allocation. Native geometry/domain/conservation and existing normal-station packet checks pass3,319 assertions/four cases. Eight focused source-contract/refusal checks pass0.385s. The shared station row now has one modular cell, four native roles, two axes and three independently sorted parts:384 source slots, including64 explicitly absent roof slots. Construction/refusal are preview-only; installed ready/working follow actual native loading. All144 other public rows,43 packed routes/payload,84 completed vehicle artifacts,28 unique original inputs and three other native binaries remain exact. Only the focused Debug test executable was rebuilt; the initial fixture assertion log is diagnostic, not accepted evidence. [Complete geometry handoff receipt](../../../../logs/graphics/rail-station/20261011-geometry-01/verification.json) and [checked native descriptor](../../../../media/multilayer/contracts/conventional-station-geometry.json). Schema145/55,037. No new source art, range or runtime activation is claimed. Continue original24.1.

### Local Mandatory Reading - Native canopy glass source ownership

_Applies to Group24C, the child-sprite prerequisite of24.2/24.3._

Read the actual default canopy layouts4..7 in `src/table/station_land.h`, `DrawCommonTileSeq` in `src/sprite.cpp`, `DrawTileSeqStruct`, the compiled24B descriptor/export and its focused tests, then the shared `facility.rail-station` source row. The three native sorting parents remain back/front/roof; canopy glass is a fourth source part attached to the roof parent at its actual screen offset and transparent palette, never a fourth bounding box. Publish compiled child provenance before dependent canopy originals. Keep the selected24.1 X base pixels, all existing fleet originals and43 packed routes exact. Do not replace the native glass behavior with an opaque roof or unaligned stock glass.

### Group 24C - Preserve the native canopy glass child in the source matrix

This bounded source prerequisite extends24B's three-parent geometry with the actual child sprite. The earlier384-slot/three-source-part draft did not give that native transparent child its own original. Preserve original24.1-24.3 and their remaining source/runtime obligations; no master task is added.

{{HARD}}
- [X] 24C.1 Export and verify the actual canopy glass child's screen offset and transparent palette/flag alongside the three existing native parent bounds, with no new station or sorting authority.
- [X] 24C.2 Reconcile the shared source matrix to back/front/roof/glass, preserving native roof-child attachment and genuine roof/glass absences on bare/loading-building roles; update dependent source contracts without changing packed ranges.
- [X] 24C.3 Run one focused native and source-contract burst, preserve the selected X base and existing packed/fleet artifacts, then return the complete parent/child geometry handoff to24.2.

24C closure,2026-10-11: all four actual canopy layouts expose their native glass screen offset(0,0), transparency flag and PALETTE_TO_TRANSPARENT through the compiled read-only geometry export. Three bounding-box parents remain unchanged; glass attaches to the elevated roof as a native child. The shared source matrix now supplies back/front/roof/glass:512 slots,128 genuine roof/glass absences and384 nonempty parts; schema145/55,165. Native geometry/child/construction/approach and existing station packet checks pass3,355 assertions/four cases. Nine source-contract plus seven selected-base checks pass1.137s. All four24.1 base PNGs,144 other public rows,43 packed routes/payload,84 completed vehicle artifacts,28 unique original inputs and three other native binaries remain exact. One focused Debug test build supplies the child proof; its new binary is recorded. Base report metadata alone now names absent glass and the complete512-slot requirement. [Complete native glass source handoff](../../../../logs/graphics/rail-station/20261011-glass-01/verification.json). No new source view, packed range or runtime activation. Continue original24.2; the previous three-source-part/384-slot draft is historical.

### Local Mandatory Reading - Conventional rail freight station geometry and sources

_Applies to Group24 only._

Read Global/Source generation Area, selected conventional handoff, completed24B/24C native descriptor/proof, public `facility.rail-station`, and actual Plan7 conventional rail station construction/query/drawing consumers. The old3x2/six-variant/four-view draft is not native geometry. Consume the delivered M1 station/depot/body/status source and occlusion helpers only where the actual geometry agrees. Preserve real train clearance, platform/loader back/front, elevated roof and native transparent glass-child ownership, paid excavated-floor admission and actual preview/loading state owners. Freeze measured native cells and approaches before generating dependent originals; the canonical framed prompt and installed imagegen skill apply before raster. Source completion does not activate packing/display under10.4D. Before canopy originals or packing, read only `RemapCoords`, `AddSortableSpriteToDraw`, `AddChildSpriteScreen` and `ViewportDrawParentSprites`, then the checked `native_packed_anchors`: common source frames use(-31,-33), packed parents compensate actual native origin/offset, and relative glass uses(0,0) from the roof image left/top. Keep original sorting boxes and native child displacement exact.


Context has compacted 1 times while executing this group.

### Group 24 - Author underground rail freight station

Selected dispatch resumed,2026-10-11, under master10.4C after completed conventional vehicle sources. The earlier master finalization skip is historical; all three selected station source tasks below are active in order. Inspect actual native cell/approach geometry before replacing the old draft3x2 source footprint. Packing/runtime remain10.4D and their native consumers.

{{HARD}}
- [X] 24.1 For underground rail freight station, derive a single-model brief from its approved manifest/template: loading platform and mine loader aligned to rail. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [X] 24.2 Produce the required directions/rotations for underground rail freight station from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [X] 24.3 Add all approaches/loading and back/front parts for underground rail freight station, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.


24.1 base closure,2026-10-11: the guide-only fourth original selects the genuine X loading-building silhouette on its two native platform strips. One corrugated roof/vent, three louver panels, two outside posts, inset amber glass and contained square loader nozzle remain one rear building; the front strip is plain gray steel with its inner safety line. The first framed and two approximate-geometry candidates are retained/rejected, never converted as approved poses. The isolated original has209px minimum alpha16 clearance and zero opaque magenta; uniform BOX placement produces64x64 at(-31,-33), with861 rear and420 front original pixels, no roof in this native role. All four ledge contacts, empty rail centre, actual checked train-centre foreground clearance, exact disjoint parent reconstruction and native1x/4x/three-cell placement pass seven focused checks0.972s. [Selected original and exact prompt history](../../../../media/multilayer/source/ground/station-generation-record.json); [complete base receipt](../../../../logs/graphics/rail-station/20261011-base-01/verification.json). Public source remains planned/incomplete with explicit selected-base metadata. All144 other family records,43 packed routes/payload,84 completed vehicle artifacts,28 unique original inputs and four current binaries remain exact. No C++build/full regression or new runtime activation. Continue24.2; Y, bare/canopy roles, elevated canopy roofs and preview/loading phases remain required.

24.2 partial view checkpoint,2026-10-11: all four actual bare/loading-building cells on X/Y now have genuine ready0 source, with eight original parents and eight structural roof/glass absences. Coupled Y scene candidates are retained/rejected for relocated hardware/unsafe native contacts or gap. Independent source parents fix actual native placement. The corrected framed sheet supplies Y plain front and X/Y bare rear ledges; its short loader remains excluded, replaced by one separately authored genuine Y rear with the declared one-vent/three-louver/two-post/glass/nozzle inventory. Measured ordinary crop windows have22/77/77px alpha16 clearance and zero opaque magenta; the isolated Y loader has164px clearance. Visible magenta haze is transparent RGB, not opaque model paint. The Y front box[0,42,43,64] retains all original pixels and clears the actual rail centre; the old one-pixel-up placement refuses. Four native1x/4x views, all native ledge contacts, train-centre foreground clearance, role identity, immutable inputs and exact regeneration pass eight focused checks6.849s. [Complete partial-view receipt](../../../../logs/graphics/rail-station/20261011-base-views-01/verification.json) and [exact prompts/provenance](../../../../media/multilayer/source/ground/station-generation-record.json). Approved X base PNGs,144 other public rows,43 packed routes,84 fleet artifacts,28 unique original inputs and four binaries stay exact. No C++build/full regression/new activation.24.2 stays unchecked until canopy4..7 support/roof/glass views pass;24.3 and full512-slot source closure remain pending.

24.2 native image-origin checkpoint,2026-10-12: actual native renderer reading confirms parent sprite position includes RemapCoords(origin+offset), while relative glass is drawn from roof image left/top. The public source contract now freezes packed front anchors X(-53,-44)/Y(-9,-44), elevated roof(-31,-17) and glass(0,0), preserving common source(-31,-33) and all three original sorting boxes/child displacement. Applying a common packed offset to every part would double-shift the front/roof/glass. Nineteen focused contract/view checks pass8.256s; all six low-view PNGs, four X-base PNGs,43 packed routes,84 fleet artifacts and four binaries stay exact. Only source/packing metadata changes; no C++build or activation. [Native origin handoff receipt](../../../../logs/graphics/rail-station/20261012-anchor-01/verification.json).24.2 stays unchecked for canopy originals; actual packed/decoded/game proof remains10.4D.

24.2 closure,2026-10-12: all eight actual native ready0 cells now have their genuine role/axis sources,24 nonempty parts and eight structural roof/glass absences. Two retained framed originals supply eight independently registered support posts and four complementary canopy roofs; all twelve measured rectangles have at least48px alpha16 clearance and zero opaque magenta. Actual deckz2/roofz16 support contacts, original native sorting heights7/X versus2/Y, all roof corners, matching neighboring A/B ridges and four panes per roof are verified. Neutral structural steel and authored opaque cyan pane pixels are disjoint and reconstruct the exact original roof; glass stays the one native transparent roof child. Native1x/4x comparisons show each source owner and adjoining halves. Twenty-seven focused canopy/view/contract checks pass25.304s, including displaced/duplicated posts, missing/relabelled views, missing panes, shifted roofs and rejected/frame-contaminated originals. All approved low-view/X-base PNGs,43 packed routes/payload,84 fleet artifacts and four binaries remain exact. No C++build/full regression or activation. [Complete view receipt](../../../../logs/graphics/rail-station/20261012-canopy-01/verification.json); [retained originals, exact prompts and native registrations](../../../../media/multilayer/source/ground/station-generation-record.json).24.3 remains unchecked for preview/loading states and full512-slot coverage; packing/decoded/ordinary-game rendering remains10.4D.

24.3/full Group24 source closure,2026-10-12: the complete native atlas has512 explicit source slots,384 nonempty original parts and128 genuine low-role roof/glass absences. Actual picker/command reading confirms a completed atomic build preview; construction/ready/refusal use static phase0 aliases, with no installed construction stages or Blocked flag. Actual Station.loading_vehicles/Train part tile ownership determines installed ready/working; four retained cyan glass phases may use the existing TimerGameTick::counter solely for presentation. Only two approved opaque equipment-glass pixels change per native cell, using the immutable M1 facility-status original; all other geometry, alpha, contacts, separate roofs/glass and foreground train-centre pixels stay exact. Full state/view/phase/part atlas, native3x contact sheet and four-phase loading GIF regenerate exactly. Nineteen focused full-source/contract checks pass12.629s; the preceding27-case canopy/view/contract burst also passed. Public source coverage is authored/complete, packed offset remains null and runtime inactive. All144 other public rows, prior approved station views,43 packed ranges/GRF,84 fleet artifacts and four binaries remain exact. No C++build/full regression. [Full station source receipt](../../../../logs/graphics/rail-station/20261012-states-01/verification.json). Continue selected Group25, then the original selected supply families; master10.4C/D remain unchecked.

### Local Mandatory Reading - Native road-terminal drawing and construction producer

_Applies to Group25B; staged before source task25.1._

Read Global/Source generation Area and only `GetStationTileLayout(StationType::Truck, graphics)`, the six native truck sequences in `src/table/station_land.h`, `CmdBuildRoadStop`, `GetBayRoadStopDir`, `GetDriveThroughStopAxis`, `IsCustomRoadStopSpecIndex` and `CheckExcavationConstructionArea`. Preserve the actual three bay/two drive-through sorting parents, ground/road-overlay ownership and one-tile modular cells. Read completed24B/24C geometry exports and focused fixture setup only for the proof/publication pattern, with explicit native town/pool initialization and MockEnvironment before language loading. Source registration must compensate real native parent origins through RemapCoords without rewriting sorting boxes. A native descriptor/test does not authorize free rock clearing or packed art.

### Group 25B - Verify native road-terminal geometry before dependent originals

Staged,2026-10-12: original25.1's draft2x2/four-tile/two-part rectangle disagrees with the actual default road-stop renderer. This bounded producer group precedes original25; no master scope is added or removed.

{{HARD}}
- [X] 25B.1 Export all six actual truck-stop layouts through a read-only native descriptor/query, retaining four bay entrance directions, both drive-through axes, exact parent bounds/order/offsets and explicit third-parent absence. Refuse fallback graphics, custom stops, other station types, Surface and Classic eligibility.
- [X] 25B.2 Compile and verify actual normal road-stop construction on both rock layers, variable footprints and all six native layouts. Refuse an incomplete footprint or any required entrance/through approach before allocation; preserve geology, cash and RNG for read-only/refused boundaries. Fixture-open floors remain explicit test setup.
- [X] 25B.3 Publish the complete successful native geometry proof and reconcile only the public road-terminal source matrix/anchors/state ownership. Verify source-count/parent/axis/fallback/false-evidence refusals and preserve all completed station/fleet sources and existing packed routes before original25.1 authoring.

25B closure,2026-10-12: the read-only native export/query preserves all six actual truck-stop layouts, three bay/two through parents, entrance/axis, exact sequence bounds/offsets and genuine third-parent absence. Actual normal commands pass96 configurations across both rock layers, all six layouts, both holdout orders and1x1/1x2/2x1/2x3 footprints. Incomplete floor/entrance/through-end cells refuse before Station/RoadStop allocation; geological/cash boundaries and read-only query RNG remain exact. Actual purchased mine trucks load real station packets at explicit fixture arrival, preserving conservation; driven route acceptance is not claimed. Native proof passes4,850 assertions/five cases. Eighteen native-source/contract checks pass1.284s, including stale/fallback/partial/duplicate proof and topology/anchor/state refusals. The public road-terminal row alone now has six actual layouts, one-tile cells and three independent parent slots:288 total,32 genuine absences,256 required nonempty; shared145-family requirement54,941. Parent image offsets compensate actual native origins without changing sorting. All123 protected prior artifacts, including full station/fleet source, shipped GRF/manifests and three other binaries, remain exact. The focused Debug test executable changes; an initial fixture conversion warning was fixed by a harness-only rebuild before one native test burst. No full regression/raster generation/packed range/activation. [Complete road-terminal producer receipt](../../../../logs/graphics/road-terminal/20261012-geometry-01/verification.json). Resume original25.1 against this compiled geometry.

### Local Mandatory Reading - Road mine terminal native geometry and sources

_Applies to Group25 only._

Read Global/Source generation Area, selected conventional handoff, public `facility.road-terminal`, actual Plan7 road-stop construction/excavation admission, native default truck-stop layouts and picker/world drawing. Freeze actual bay versus drive-through cells, true entrance directions/axis, approaches and variable station grouping before replacing draft footprints or generating originals. Inspect `CmdBuildRoadStop`, `GetStationTileLayout`, `GetBayRoadStopDir`, `GetDriveThroughStopAxis`, `StationPickerDrawSprite` and the road-stop branch of `DrawTile_Station`; use the current native geometry proof pattern from completed24B/24C only where applicable. Status and motion may read existing native road-vehicle/loading owners, never a copied station inventory or art-local timer. Reuse the reviewed station/fleet materials and retained M1 service/status sources only when actual geometry and provenance agree. Canonical framed prompt/installed imagegen skill apply to new raster; source coverage grants no native packed range or runtime activation under10.4D.

Context has compacted 1 times while executing this group.

### Group 25 - Author road mine terminal

Selected dispatch resumed,2026-10-12, under master10.4C after completed conventional station Group24 source. The earlier finalization skip is historical; all three road-terminal source tasks below remain active in order. Establish actual native per-cell/approach geometry before replacing the old draft footprint. Packing/runtime remain10.4D.

{{HARD}}
- [X] 25.1 For road mine terminal, derive a single-model brief from its approved manifest/template: truck loading and turning access. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [X] 25.2 Produce the required directions/rotations for road mine terminal from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [X] 25.3 Add all approaches/loading and back/front parts for road mine terminal, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.


25.1 base closure,2026-10-12: the selected genuine SE bay comprises three independently registered original parents on native64x64/anchor(-31,-33). Two three-post open safety rails come from measured clean windows of the first framed original; its two-louver housing remains excluded. A separate geometry-guided housing supplies the correct one vent/three louvers/two posts/amber glass/recessed nozzle inventory. The attempted louver sheet correction stays entirely rejected: two louvers, zero transparent crop clearance and opaque magenta contamination. Selected component clearances78/116/373px have zero opaque magenta; BOX sampling preserves whole original components within2% uniform-aspect rounding. All actual native strip contacts, both native SE entry lanes, palette, exact regeneration and rejection/registration-drift gates pass13 focused checks4.194s. Native1x/4x comparison shows all three parents and their open one-tile model. Taller open-rail post silhouettes fit the unchanged10-high native sorting envelope; no ground/road/truck is baked into the source. All124 prior station/fleet/source/native identities remain exact, including the current four binaries. No native build/full regression/new activation. [Complete SE base receipt](../../../../logs/graphics/road-terminal/20261012-base-01/verification.json); [immutable originals/prompts/registrations](../../../../media/multilayer/source/ground/terminal-generation-record.json).25.2/25.3 remain unchecked for five other native layouts and full288-slot source closure; packing/game display remains10.4D.

25.2 closure,2026-10-12: all six genuine native layouts now have16 independently registered nonempty ready0 parents and two actual through-parent absences. Four immutable originals supply ten unique physical components; six bounded reuses preserve exact whole pixels and native RemapCoords translations at the same world orientation/footprint/facade. NE/NW genuine rear housings keep front hardware hidden; SE/SW and throughX show their correctly oriented loading faces. One equipment-sheet rail has the wrong axis and remains excluded; the corrected rail original supplies four genuine X13/X11/X16/Y16 components. All ten selected ordinary rectangles have at least33px alpha16 clearance and zero opaque magenta. Native contacts, both directed bay-mouth lanes, through-rail vehicle-centre clearance, exact SE preservation, all nine artifact regenerations and ancestry/missing-view/crop/registration/access refusals pass21 focused checks8.926s. All128 prior station/fleet/base/native identities and144 other public rows remain exact. No native build/full regression/packed range/activation. [Complete view receipt](../../../../logs/graphics/road-terminal/20261012-views-01/verification.json); [six-view source comparison](../../../../logs/proofs/mlm/native-art/ground/terminal-ready-views-review.png).25.3 remains unchecked for preview/loading phases and complete288-slot source handoff; actual decoded/game sorting remains10.4D.

25.3/full Group25 source closure,2026-10-12: the complete terminal atlas covers288 explicit state/frame/view/parent slots,256 nonempty originals and32 actual through-parent absences. Atomic construction/refusal remain static preview aliases; installed ready/loading retain actual Station.loading_vehicles/RoadVehicle tile ownership and existing TimerGameTick::counter presentation only. One retained M1 facility-status glass pixel changes only in genuine visible front housings gfx1/2/4; four working phases remain distinct at whole native view scale. Rear gfx0/3/5 preserve exact original hidden-front aliases rather than relocated hardware. All other pixels, alpha, native contacts, six independent layouts, entrances and through-rail vehicle-centre clearance stay exact. Complete atlas/contact sheet/loading GIF regenerate exactly and public authored metadata identifies their original assembly without a packed range or activation. Twenty-three distinct focused checks have passing final results:16 state/view cases passed in the first13.742s burst, and all seven contract cases passed1.014s after replacing the stale planned-only fixture with exact authored-source validation. The initial failure log is retained. All137 prior station/fleet/base/view/native identities and144 other public rows remain exact. No C++build/full regression/new runtime activation. [Complete source closure receipt](../../../../logs/graphics/road-terminal/20261012-states-01/verification.json); [full source comparison](../../../../logs/proofs/mlm/native-art/ground/terminal-source-review.png). Group25 is complete for source only; native packing/decoded pixels/ordinary-game rendering remain10.4D. Continue selected Coal mine27.1.

### Local Mandatory Reading - Native selected mine geometry and source states

_Applies to staged27B and selected mine Groups27/28/29/31/32/34/35; master10.4C._

Read Global/Source generation Area, the selected conventional/material handoff, Plan6 Public Handoff Contract and the current mine row. Inspect `InitialiseLayerMineDefinitions`, `InstallLayerMineDefinition`, `GetLayerMineIndustryType`, `ValidateMineDepositClaim`, `GetLayerMineOperationalStatus`, `IsLayerIndustryComplete` and `DrawTile_Industry`. Read `layer_mine_definition.cpp` and `layer_mine_presentation.cpp` only for their actual native definition/funding/state proof. Installed mines have one fixed3x3 layout and separate2x2 loading floor; there is no native four-way mine orientation setting. Verify the compiled exact nine machinery cells before reconciling those draft views. Native construction stages, geological reserve, Industry output, mine lifecycle and synchronized tick remain their existing owners. Reuse existing M1 materials/components only with exact original ancestry and compatible physical geometry. Unselected mines/processors remain pending; new source gives no packed range or runtime activation. Canonical framed prompt/installed imagegen skill apply before new raster work.

### Group 27B - Verify native mine layout and presentation before selected originals

Staged,2026-10-12, before original27.1: native definitions already own nine actual machinery cells and one fixed layout. The public four-view draft must be reconciled through complete compiled evidence, rather than authoring unused rotations or inventing a saved orientation. This child prerequisite preserves the master's original selected scope.

{{HARD}}
- [X] 27B.1 Verify and emit actual installed mine catalogue geometry, all nine cell coordinates/IIDS/TIDS ownership and the single native layout. Exercise real funding/finite production, incomplete native machinery/loading-floor refusals and existing read-only presentation states in one focused native burst; preserve cash, geology and pool allocation on refusal.
- [X] 27B.2 Publish the successful native geometry/state handoff and reconcile only the seven dispatched public mine families to their actual fixed layout, construction stages and operating/paused/depleted owners. Preserve original native fallback art and packed identities; no new rendering activation or quantity owner.
- [X] 27B.3 Validate complete source matrices and stale/partial/duplicate/foreign geometry or state refusals, retaining all completed station/terminal/fleet sources and packed ranges. Return to original27.1 only after this source producer passes.

27B closure,2026-10-12: actual installed catalogue/funding projection confirms all ten exact IIDS/TIDS mine definitions have one fixed3x3 layout, nine row-major machinery cells and separate loading floor(0,3)/(1,3)/(0,4)/(1,4). All ten definitions fund real machinery and extract through the sole finite reserve/output owner;14 selected missing machinery/loading-floor configurations refuse estimates/execution without changing cash, geology, RNG or pool allocation. Existing native presentation verifies construction/availability, pause/output-full/pickup and depletion with final stored cargo retained. One focused native build/run passes2,715 assertions/six cases. The checked public geometry/state handoff reconciles only the seven selected mine drafts to fixed direction none, nine cells/two source parts and288 required slots each; native construction stages0..3 differ from static paused/depleted aliases and existing synchronized working presentation. Shared145-family total is48,893. Twenty-five distinct focused source/contract checks have passing final results:22 passed in the first2.133s burst, then the three affected building-direction boundary checks passed0.390s after adding selected mines to the existing fixed-layout convention. Original failures are retained. All140 protected previous artifacts and138 other public family rows remain exact. Only the focused Debug test executable changes; no runtime sprite binding, raster, packed range or full regression. [Complete native geometry handoff receipt](../../../../logs/graphics/mine-source/20261012-geometry-01/verification.json). Return to original Coal mine27.1; all selected mine source tasks remain unchecked until genuine artwork passes.

### Group 27 - Author Coal mine

Selected dispatch resumed,2026-10-12, under master10.4C after completed terminal Group25 source. The earlier finalization skip is historical. Complete staged27B's compiled native geometry/state handoff before base27.1; required views mean actual installed layout geometry, never an invented mine orientation. Packing/runtime remain10.4D.

{{HARD}}
- [X] 27.1 For Coal mine, derive a single-model brief from its approved manifest/template: Underground machinery/loading silhouette with Coal identity. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [X] 27.2 Produce the required directions/rotations for Coal mine from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [X] 27.3 Add construction/working/paused/depleted layout states for Coal mine, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.


27.1/27.2 base and actual-view closure,2026-10-12: the completed native Coal model has all nine genuine fixed-layout machinery roles and18 independently stored source parts at64x64/anchor(-31,-33). Eight new cell originals come from four selected immutable rasters; the compatible control hut reproduces its exact original M1 native pixels/material, with no copied foreign campus. The single required native view is none; no unused four-way rotation is authored. The first framed sheet has unsafe upper crops; the height-corrected second sheet has opaque magenta and stays rejected. Separate chute/filter originals supply low genuine geometry; the earlier chute with shifted south-floor contact remains rejected. All eight selected ordinary source rectangles have at least25px alpha16 clearance and zero opaque magenta. Whole-original uniform62px registration meets all actual floor corners/south tip and unchanged16x16x20 sorting volume; the shipped M1 hut remains exact64px original. Existing native industry underlay/upper-part convention preserves original material and exact reconstruction. Native1x/3x assembled campus plus independent cell/part comparison, exact four-artifact regeneration, missing/relabelled/changed/rejected/foreign-reuse/contact/height/palette refusals pass15 focused checks6.308s. All294 prior source/payload/binary identities and144 other public families stay exact. No C++build/full regression/new packed range/activation. [Complete base/view receipt](../../../../logs/graphics/coal-mine/20261012-base-01/verification.json); [retained originals/prompts/native registrations](../../../../media/multilayer/source/ground/mine-generation-record.json); [native source comparison](../../../../logs/proofs/mlm/native-art/ground/coal-mine-base-review.png).27.3 remains unchecked for actual native construction stages0..3 and working/paused/depleted full288-slot source closure; native decoded/game proof remains10.4D.

### Local Mandatory Reading - Coal mine construction originals

_Applies to staged27C before original27.3 source closure._

Read Global/Source generation Area, the compiled `mine.COAL` geometry/state row and only `GetIndustryConstructionStage`/`IsLayerIndustryComplete` plus the completed27.1/27.2 original registrations. Stage0 is a paid one-tile machinery foundation, stages1/2 are genuine unfinished/finishing apparatus, and stage3 is the exact completed base. The original M1 Earth receiver's one-cell construction yard and unfinished control hut may supply compatible foundation/hut components only with exact raw/native ancestry and floor contacts; no soil bank, shaft, foreign machine or copied whole industry substitutes for missing Coal equipment. Preserve all nine actual machinery cells, separate four loading cells, uniform source registration and unchanged16x16x20 native sorting. Read the canonical framed prompt/installed imagegen skill before raster work, retain whole ordinary source rectangles and all rejected attempts, and validate every stage without touching approved base bytes. Native quantities, work and time remain their actual owners; new source neither opens rock nor activates a sprite range.

Context has compacted 1 times while executing this group.

### Group 27C - Deliver actual Coal mine construction originals

Staged,2026-10-12, under original27.3: bounded construction work precedes operating/depleted cues and aggregate source publication. The full original27.3 remains unchecked until this group and27D complete. No master task is added.

{{HARD}}
- [X] 27C.1 Establish exact compatible stage0 foundation and stage1 control-hut reuse from retained M1 originals; retain the completed stage3 model and define the native per-cell construction-stage matrix without inventing work or floor.
- [X] 27C.2 Author/select genuine stage1 unfinished sources for all eight new Coal machinery roles, preserving their base footprints, intended equipment identity, fixed camera/materials and clean native registration.
- [X] 27C.3 Author/select genuine stage2 finishing apparatus, assemble all four actual construction stages for all nine cells/back/fronts, and pass focused exact-origin, stage-coverage, contact, height and rejection proofs while preserving completed base/packed identities.

27C closure,2026-10-12: all72 native construction source slots are complete across actual stages0..3, nine machinery cells and both independent parts. Exact compatible M1 foundation/unfinished-hut originals and original finishing glass retain native ancestry;16 genuine new unfinished/finishing machinery components use complete ordinary rectangles, at least16px alpha16 clearance, zero opaque magenta, whole uniform registration and unchanged20-high sorting. Stage3 is byte-exact approved base. Wrong-count/wrong-axis/Coal-containing/unsafe-clearance/over-height attempts remain excluded or rejected with immutable originals/prompts. All10 focused checks pass32.436s, including stage/matrix/origin/contact/height/palette/refusal/preservation proof. All305 protected artifacts, four binaries,43 packed ranges/GRF and145 public rows remain exact. No C++build/full regression/activation. [Construction receipt](../../../../logs/graphics/coal-mine/20261012-construction-01/verification.json); [four-stage source comparison](../../../../logs/proofs/mlm/native-art/ground/coal-mine-construction-review.png). Continue27D; original27.3 remains unchecked until complete operating/depleted source coverage.

### Local Mandatory Reading - Coal mine operating cues and complete source handoff

_Applies to staged27D and return to original27.3._

Read Global/Source generation Area, completed27C's selected construction matrices and the current `mine.COAL` native state policy. `GetLayerMineOperationalStatus`, the actual geological reserve, `Industry::produced` output and existing synchronized visual counter own working/paused/depleted presentation. Depletion never erases final stored output. Reuse immutable M1 equipment glass only at the genuine control-hut fitting; a missing depleted/off cue needs an original component, never software recolor or a green-ready lamp relabelled as depletion. Validate all288 source slots, native per-cell construction phases and static paused/depleted aliases before publishing complete source through the existing shared manifest. Source closure does not grant a packed range, native art binding or ordinary-game acceptance under10.4D.

### Group 27D - Complete Coal mine operating states and source coverage

{{HARD}}
- [X] 27D.1 Author/select the original missing depleted/off equipment cue and bind genuine working/paused/depleted glass to the actual native control fitting and state/phase owners; retain all apparatus, floor and stored-output identity outside declared presentation parts.
- [X] 27D.2 Assemble/validate the full288-slot native source atlas, model-specific construction/state contact sheet and four-phase working comparison. Preserve every approved base/construction original and packed identity; publish source only, then mark original27.3 complete.

27D/aggregate27.3 source closure,2026-10-12: the Coal native source atlas contains all288 nonempty original parts at1152x1024 across the actual fixed layout, nine machinery cells, four construction stages and working/paused/depleted phases. Four retained M1 working glass phases and static red paused glass join a genuine new neutral-gray depleted/off original only at the actual control-hut fitting; apparatus, floor, alpha and Coal outside that fitting remain exact, including final stored output on depletion. The full16-pose model contact sheet and four-phase working GIF regenerate exactly. Ten focused checks pass29.067s; the two affected live/public atlas binding and forged-report refusals pass13.616s after hardening. All333 protected artifacts, approved construction/base originals, four binaries,43packed ranges/GRF and144 other public rows remain exact. Full source metadata is published through the existing shared Coal row with no packed offset/native activation. No C++build/full regression. [Full Coal source receipt](../../../../logs/graphics/coal-mine/20261012-states-01/verification.json); [complete source comparison](../../../../logs/proofs/mlm/native-art/ground/coal-mine-source-review.png); [originals/executed built-in prompts and components](../../../../media/multilayer/source/ground/mine-generation-record.json). All original27 tasks and staged27C/27D tasks areX; proceed selected Iron mine28. Master10.4C/D remain pending for the remaining selected families and native packing/game proof.

### Local Mandatory Reading - Selected Iron mine originals and compatible components

_Applies to Group28 under master10.4C._

Read Global/Source generation Area, the compiled `mine.IORE` geometry/state row, Plan6 Public Handoff Contract and its Iron Ore capability/resource row. Consume completed27B's exact native funding/state proof and only the needed approved Coal/M1 component registrations. Iron has the same actual fixed3x3/nine-cell machinery layout and separate four loading cells, one view none,64x64 canvas/anchor(-31,-33),20-high sorting and288 required source slots. Its cargo identity is Iron Ore, never Coal or Refined Metals. Reuse genuine compatible material, machinery and empty construction components only through explicit per-cell physical roles and immutable original ancestry; no copied whole industry or relabelled Coal stock. Read the canonical framed prompt/installed imagegen skill before new raster originals. Retain native reserve/output/work/tick ownership, original approved Coal bytes and source-only boundaries; packing/game proof remains10.4D.

### Group 28 - Author Iron mine

Selected dispatch resumed,2026-10-12, under master10.4C after full Coal source27/27C/27D closure. The earlier finalization skip is historical. Consume completed27B native IORE geometry/state proof, preserve compatible per-cell original ancestry, and deliver genuine Iron Ore identity before source closure. Required view is the one actual native none layout; no unused mine rotations. Packing/runtime remain10.4D.

{{HARD}}
- [X] 28.1 For Iron mine, derive a single-model brief from its approved manifest/template: Underground machinery/loading silhouette with Iron identity. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [X] 28.2 Produce the required directions/rotations for Iron mine from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [X] 28.3 Add construction/working/paused/depleted layout states for Iron mine, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.


28 source closure,2026-10-12: genuine rust-brown/gray Iron Ore originals supply seven cargo-bearing native machinery cells; compatible no-cargo filter and exact original M1 hut complete the fixed nine-cell model/18base parts. The first sheet lacks upper clearance; the first bunker gains wrong long legs and its chute has a skewed south-floor contact, so those components remain excluded with immutable originals/prompts. Corrected framed/isolated originals use complete clean rectangles, minimum17px alpha16 clearance, zero opaque magenta, whole uniform registration and unchanged20-high native sorting. An explicit per-cell compatible graph reuses genuine empty unfinished/finishing apparatus and M1 foundation/hut originals; stage3 is the exact Iron base. All288 source parts,16pose model/contact review and four-phase working GIF regenerate exactly. Original working/paused/depleted glass changes only the real hut fitting; all machinery/ground/Iron stock outside it remains exact and depletion retains final output. Fourteen focused checks pass47.629s:11active source checks plus three affected Coal preservation boundaries.340 protected prior artifacts, all Coal family/original identities, four binaries,43packed ranges/GRF and144other public rows remain exact; one protected focused-test boundary file is explicitly updated to recognize only subsequent complete live native source handoffs. No C++build/full regression/packed range/runtime activation. [Iron source receipt](../../../../logs/graphics/iron-mine/20261012-base-01/verification.json); [model-specific source review](../../../../logs/proofs/mlm/native-art/ground/iron-mine-source-review.png); [retained original prompts/components](../../../../media/multilayer/source/ground/mine-generation-record.json). Continue selected Copper29; master10.4C/D remain unchecked for remaining selected families and native game proof.

### Local Mandatory Reading - Selected Copper mine originals and compatible components

_Applies to Group29 under master10.4C._

Read Global/Source generation Area, the current compiled `mine.CORE` geometry/state row, Plan6 Public Handoff Contract and native Copper Ore resource identity. Consume completed27B native producer proof and only relevant Iron28/Coal27 per-cell original registrations plus `convert_material_mine_art.py`'s exact native/mineral/reuse boundaries. Copper retains one actual fixed3x3/nine-cell layout and separate four loading cells, view none,64x64/anchor(-31,-33),20-high sorting and288 source slots. Author genuine Copper Ore identity; never relabel Iron/Coal stock or refined copper bars. Reuse compatible empty apparatus/M1 hut and no-cargo filter only through explicit per-cell roles/original ancestry. Read the canonical framed prompt/installed imagegen skill before raster work, preserve all approved Iron/Coal pixels and native quantity/tick owners, and publish source only. Packing/game proof remains10.4D.

### Group 29 - Author Copper mine

Selected dispatch resumed,2026-10-12, under master10.4C after complete Iron28 source. The earlier finalization skip is historical. Actual compiled CORE geometry/state proof and compatible individual physical components precede genuine Copper Ore authoring; the one required view is none. Source closure gives no packed range/native activation.

{{HARD}}
- [X] 29.1 For Copper mine, derive a single-model brief from its approved manifest/template: Underground machinery/loading silhouette with Copper identity. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [X] 29.2 Produce the required directions/rotations for Copper mine from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [X] 29.3 Add construction/working/paused/depleted layout states for Copper mine, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.


29 source closure,2026-10-12: genuine orange/brown/green raw Copper Ore originals supply seven cargo-bearing native apparatus cells; exact compatible no-cargo filter and M1 hut complete all nine cells/18base parts. Four new immutable rasters have complete ordinary model rectangles with minimum36px alpha16 clearance, zero opaque magenta, whole uniform registration and unchanged20-high native sorting. Generated source-margin guide captions are excluded by measured ordinary rectangles, with no erased/repainted model pixels. The existing explicit per-cell compatible graph supplies genuine empty construction apparatus/M1 foundation/hut; completed stage3 is the exact Copper base. All288 source parts,16pose cell/state review and four-phase working GIF regenerate exactly. All completed machinery/floor/Copper Ore outside the original hut fitting remain exact across working/paused/depleted, retaining final physical output. Eleven focused active checks pass17.645s; the affected previous Iron preservation boundary passes13.951s.357 protected artifacts, all Coal/Iron family/original identities, four binaries,43packed ranges/GRF and144other public rows remain exact; one protected reusable focused-test file explicitly learns to validate subsequent complete live source handoffs while preserving predecessor family documents. No C++build/full regression/native range/activation. [Copper source receipt](../../../../logs/graphics/copper-mine/20261012-base-01/verification.json); [source model review](../../../../logs/proofs/mlm/native-art/ground/copper-mine-source-review.png); [retained originals and executed prompts](../../../../media/multilayer/source/ground/mine-generation-record.json). Continue selected Salt31; Bauxite30 is outside the dispatched first-generator material subset and remains unchecked. Master10.4C/D retain remaining selected source/native proof obligations.

### Group 30 - Author Bauxite mine

Goal-authorized skip (2026-10-08, master finalization): dispatched expansion work in this group is skipped under [finalization exception GP-TG](portal-tbm/r2mlvs3mp-z-a-finalization-ledger.md#gp-tg-tube-and-geothermal-expansion-requires-major-gap-planning). Existing delivered prefixes stay complete; unimplemented profile work remains pending. No skipped task is markedX.

{{HARD}}
- [ ] 30.1 For Bauxite mine, derive a single-model brief from its approved manifest/template: Underground machinery/loading silhouette with Bauxite identity. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [ ] 30.2 Produce the required directions/rotations for Bauxite mine from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [ ] 30.3 Add construction/working/paused/depleted layout states for Bauxite mine, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.


### Local Mandatory Reading - Selected Salt mine originals and compatible components

_Applies to Group31 under master10.4C._

Read Global/Source generation Area, the compiled `mine.SALT` geometry/state row, Plan6 Public Handoff Contract and native Salt recipe/resource identity. Consume completed27B native producer proof, Copper29's compatible per-cell original handoff and the relevant `convert_material_mine_art.py` native/mineral/reuse boundaries. Salt retains one actual fixed3x3/nine-cell layout plus four separate loading cells, view none,64x64/anchor(-31,-33),20-high sorting and288 required source parts. Author genuine white/off-white Salt crystals; never relabel metallic ore, Coal or refined cargo. Reuse compatible empty machinery/M1 hut/no-cargo filter only through explicit physical roles and immutable original ancestry. Read the canonical framed prompt/installed imagegen skill before raster work; preserve all approved predecessor sources and native reserve/output/work/tick owners. Source closure grants no packed range or native game display under10.4D.

### Group 31 - Author Salt mine

Selected dispatch resumed,2026-10-12, under master10.4C after complete Copper29 source. Actual SALT native geometry/state proof and compatible individual components precede genuine Salt authoring. The one required view is none; packing/native activation remain10.4D.

{{HARD}}
- [X] 31.1 For Salt mine, derive a single-model brief from its approved manifest/template: Underground machinery/loading silhouette with Salt identity. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [X] 31.2 Produce the required directions/rotations for Salt mine from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [X] 31.3 Add construction/working/paused/depleted layout states for Salt mine, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.


31 source closure,2026-10-12: genuine white/off-white coarse Salt crystals identify seven cargo-bearing native apparatus cells; exact compatible filter/M1 hut complete the nine-cell/18base-part fixed model. Four selected immutable rasters use complete ordinary rectangles, minimum20px alpha16 clearance, zero opaque magenta, whole uniform registration and unchanged20-high sorting. Three rejected conveyor originals retain their actual height/south-contact refusals; the fourth genuinely corrected low centered-floor model passes, with no clipping, squeeze, tolerance change or sorting enlargement. Caption-free technical guides and exact original reference crops have retained ancestry. Explicit compatible individual empty construction apparatus/M1 foundation/hut/glass supply native stages0..2; stage3 is the exact Salt base. All288 source parts,16pose contact/state review and four-phase working GIF regenerate exactly. Only actual hut glass varies in completed states, preserving Salt stock/ground/alpha and final output on depletion. Eleven focused active checks pass17.479s. All370 protected artifacts, Coal/Iron/Copper family/original identities, four binaries,43packed ranges/GRF and144other public rows stay exact. No C++build/full regression/helper/test-code change/packed range/activation. [Salt source receipt](../../../../logs/graphics/salt-mine/20261012-base-01/verification.json); [model source review](../../../../logs/proofs/mlm/native-art/ground/salt-mine-source-review.png); [retained originals/executed prompts](../../../../media/multilayer/source/ground/mine-generation-record.json). Continue selected Lithium32; master10.4C/D retain remaining selected source and native game-proof obligations.

### Local Mandatory Reading - Selected Lithium mine originals and compatible components

_Applies to Group32 under master10.4C._

Read Global/Source generation Area, the compiled `mine.LITH` geometry/state row, Plan6 Public Handoff Contract and actual Lithium Ore recipe/capability. Consume completed27B native producer proof, Salt31's compatible per-cell source handoff and relevant `convert_material_mine_art.py` mineral/reuse/native boundaries. Lithium is a Deep Underground resource gated at1980; art never creates access, reserves or production before that native owner admits it. Keep one fixed3x3/nine-cell layout plus four separate loading cells, view none,64x64/anchor(-31,-33),20-high sorting and288 source parts. Author genuine raw Lithium Ore distinct from white Salt/other ores and finished Batteries. Reuse compatible empty machinery/M1 hut/no-cargo filter only with explicit physical roles and immutable original ancestry. Read the canonical framed prompt/installed imagegen skill before raster work; preserve all predecessor pixels and native quantity/work/tick owners. Packing and actual game display remain10.4D.

Context has compacted 1 times while executing this group.

### Group 32 - Author Lithium mine

Selected dispatch resumed,2026-10-12, under master10.4C after complete Salt31 source. Consume native LITH fixed geometry/state proof and preserve actual Deep/1980 capability ownership while authoring genuine raw Lithium Ore. Only native view none is required; source publication never activates a native sprite range or mine before its date/layer gate.

{{HARD}}
- [X] 32.1 For Lithium mine, derive a single-model brief from its approved manifest/template: Deep machinery/loading silhouette with Lithium identity. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [X] 32.2 Produce the required directions/rotations for Lithium mine from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [X] 32.3 Add construction/working/paused/depleted layout states for Lithium mine, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.


Group32 source closure,2026-10-12: all three tasks areX. Seven genuinely authored Lithium-bearing machinery cells plus exact compatible M1 hut/no-cargo filter produce288 nonempty native source parts,18 completed base parts and the full construction/working/paused/depleted review. All11 focused checks pass in18.475s;390 prior artifacts,144 other public rows and all predecessor families/original registrations remain exact. Actual Deep/1980 capability, reserves, output and synchronized tick retain their native owners. Four immutable built-in ImageGen originals retain executed prompts/reference hashes. [Verification receipt](../../../../logs/graphics/lithium-mine/20261012-base-01/verification.json) and [source review](../../../../logs/proofs/mlm/native-art/ground/lithium-mine-source-review.png) identify the complete source. Packed offset remainsnull/runtimefalse; actual game delivery remains master10.4D.

### Group 33 - Author Nickel mine

Goal-authorized skip (2026-10-08, master finalization): dispatched expansion work in this group is skipped under [finalization exception GP-TG](portal-tbm/r2mlvs3mp-z-a-finalization-ledger.md#gp-tg-tube-and-geothermal-expansion-requires-major-gap-planning). Existing delivered prefixes stay complete; unimplemented profile work remains pending. No skipped task is markedX.

{{HARD}}
- [ ] 33.1 For Nickel mine, derive a single-model brief from its approved manifest/template: Deep machinery/loading silhouette with Nickel identity. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [ ] 33.2 Produce the required directions/rotations for Nickel mine from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [ ] 33.3 Add construction/working/paused/depleted layout states for Nickel mine, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.


### Local Mandatory Reading - Selected Cobalt mine originals and compatible components

_Applies to Group34 under master10.4C._

Read Global/Source generation Area, the current compiled `mine.COBT` geometry/state row, Plan6 Public Handoff Contract and actual Cobalt Ore recipe/capability. Consume completed27B's native producer proof, Lithium32's compatible per-cell source handoff and only the applicable `convert_material_mine_art.py` mineral/reuse/native boundaries. Cobalt remains Deep Underground/1980 and supplies the native Battery works recipe. Preserve the fixed3x3/nine machinery cells, separate four loading cells, view none,64x64/anchor(-31,-33),20-high sorting and288 source parts. Author genuine raw Cobalt Ore distinct from Lithium/Salt/other ores and finished Batteries. Reuse compatible empty apparatus/M1 hut/no-cargo filter only with explicit physical roles and immutable original ancestry. Read the canonical framed prompt/installed imagegen skill before raster work; preserve predecessor pixels and native reserves/output/work/tick. Packing and actual game display remain10.4D.

### Group 34 - Author Cobalt mine

Selected dispatch resumed,2026-10-12, under master10.4C after complete Lithium32 source. Consume actual fixed native geometry/state and Deep/1980 capability while authoring genuine Cobalt Ore. The historical finalization skip is superseded for this dispatched source group; packing/runtime remain10.4D.

{{HARD}}
- [X] 34.1 For Cobalt mine, derive a single-model brief from its approved manifest/template: Deep machinery/loading silhouette with Cobalt identity. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [X] 34.2 Produce the required directions/rotations for Cobalt mine from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [X] 34.3 Add construction/working/paused/depleted layout states for Cobalt mine, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.


Group34 source closure,2026-10-12: all three tasks areX. Seven genuine Cobalt Ore-bearing apparatus cells plus exact compatible M1 hut/no-cargo filter supply288 nonempty native source parts,18 completed base parts and complete construction/working/paused/depleted review. All11 focused checks pass in17.511s;401 prior artifacts,144 other public rows and every predecessor family/original registration remain exact. Actual Deep/1980 capability and reserve/output/work/tick ownership are unchanged. Four selected immutable built-in ImageGen originals retain exact executed prompts/reference hashes. The first sheet is retained as rejected for its hopper's native height; the corrected sheet preserves approved low apparatus geometry. [Verification receipt](../../../../logs/graphics/cobalt-mine/20261012-base-01/verification.json) and [source review](../../../../logs/proofs/mlm/native-art/ground/cobalt-mine-source-review.png) retain evidence. Packed offset remainsnull/runtimefalse; actual game delivery remains master10.4D.

### Local Mandatory Reading - Selected Rare-earth mine originals and compatible components

_Applies to Group35 under master10.4C._

Read Global/Source generation Area, the current compiled `mine.RARE` geometry/state row, Plan6 Public Handoff Contract and actual Rare Earth concentrate recipe/capability. Consume completed27B's native producer proof, Cobalt34's compatible per-cell handoff and only the applicable `convert_material_mine_art.py` mineral/reuse/native boundaries. Rare Earth concentrate retains its Deep Underground/1980 gate, native two-unit mine output and Electronics works receiver. Preserve fixed3x3/nine machinery cells, four separate loading cells, view none,64x64/anchor(-31,-33),20-high sorting and288 source parts. Author genuine raw concentrate distinct from the other minerals and finished Electronics. Reuse compatible empty apparatus/M1 hut/no-cargo filter only through explicit physical roles and immutable original ancestry. Read the canonical framed prompt/installed imagegen skill before raster work; preserve predecessor pixels and native reserve/output/work/tick owners. Packing and actual game display remain10.4D.

### Group 35 - Author Rare-earth mine

Selected dispatch resumed,2026-10-12, under master10.4C after complete Cobalt34 source. Consume actual fixed native geometry/state and Deep/1980 capability while authoring genuine Rare Earth concentrate. The historical finalization skip is superseded for this dispatched source group; packing/runtime remain10.4D.

{{HARD}}
- [X] 35.1 For Rare-earth mine, derive a single-model brief from its approved manifest/template: Deep machinery/loading silhouette with Rare-earth identity. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [X] 35.2 Produce the required directions/rotations for Rare-earth mine from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [X] 35.3 Add construction/working/paused/depleted layout states for Rare-earth mine, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.


Group35 source closure,2026-10-12: all three tasks areX. Seven genuine Rare Earth concentrate-bearing apparatus cells plus exact compatible M1 hut/no-cargo filter supply288 nonempty native source parts,18 completed base parts and complete construction/working/paused/depleted review. All11 focused checks pass in17.885s;413 prior artifacts,144 other public rows and every predecessor family/original registration remain exact. Actual Deep/1980 capability and reserve/output/work/tick ownership are unchanged. Four immutable selected built-in ImageGen originals retain exact executed prompts/reference hashes; all seven cargo-bearing cells pass actual native contacts/height on their first attempt. [Verification receipt](../../../../logs/graphics/rare-earth-mine/20261012-base-01/verification.json) and [source review](../../../../logs/proofs/mlm/native-art/ground/rare-earth-mine-source-review.png) retain evidence. Packed offset remainsnull/runtimefalse; actual game delivery remains master10.4D.

### Group 36 - Author Uranium mine

Goal-authorized skip (2026-10-08, master finalization): dispatched expansion work in this group is skipped under [finalization exception GP-TG](portal-tbm/r2mlvs3mp-z-a-finalization-ledger.md#gp-tg-tube-and-geothermal-expansion-requires-major-gap-planning). Existing delivered prefixes stay complete; unimplemented profile work remains pending. No skipped task is markedX.

{{HARD}}
- [ ] 36.1 For Uranium mine, derive a single-model brief from its approved manifest/template: Deep machinery/loading silhouette with Uranium identity. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [ ] 36.2 Produce the required directions/rotations for Uranium mine from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [ ] 36.3 Add construction/working/paused/depleted layout states for Uranium mine, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.


### Local Mandatory Reading - Native selected Surface processor geometry and presentation

_Applies to staged Group39B before dependent originals39-42._

Read Global/Source generation Area, Plan6 Public Handoff Contract, the selected conventional/material handoff and current four processor rows. Inspect `InstallLayerProcessorDefinition`, `GetLayerProcessorIndustryType`, `GetLayerProcessorRecipe`, `IsLayerProcessorActive`, `NativeLayerProcessorInventory::Capture`, `PrepareLayerRecipeBatch`, `AdvanceLayerProcessor`, `IsLayerIndustryComplete` and `DrawTile_Industry`. Actual definitions have one fixed4x4 layout; confirm all sixteen native TIDS cells with command-funded industries before replacing draft art rotations. Availability, selected recipes, Industry input/output, work and synchronized tick retain their owners. Work may bank while inputs are missing, so work_ticks alone cannot imply operating machinery. Presentation must read the same stock snapshot/planner as production and never commit a batch. Preserve prior source/packed payloads and native binaries; route the focused new executable to an isolated repository build-output directory. No new art/range/runtime claim is authorized by a declaration or uncompiled layout.

### Group 39B - Freeze native processor geometry and derived state before originals

Staged prerequisite,2026-10-12: inspection found draft four-way source directions for installed single-layout processors and no published derived operating status. Complete this bounded child producer before original39.1; original source tasks and master10.4C remain unchecked.

{{HARD}}
- [X] 39B.1 Prove the four selected actual Surface processor definitions, all sixteen ordered machinery cells, fixed native layout/capability and complete command-funded footprint; preserve native estimates, stocks, costs and RNG ownership.
- [X] 39B.2 Publish a read-only native processor presentation result from the existing availability, selected recipe, actual inventory snapshot and common batch planner. Prove idle/working/full/pause/construction/date and invalid-owner boundaries without mutating physical cargo, work/history or synchronized state.
- [X] 39B.3 Publish and validate the compiled geometry/state source contract, reconcile only the four selected public source matrices and reproducible builder, and pass focused native/source false-proof refusals while preserving all earlier art, packed ranges and binaries. Original source coverage and runtime binding remain pending.

Group39B closure,2026-10-12: all three prerequisite tasks areX. The isolated Debug executable passes1,907 assertions/7 native cases, including actual positive-cost quotes and all sixteen funded cells for each selected processor, shared atomic recipe behavior and pure construction/idle/working/full/pause/date/foreign/missing/malformed presentation. Exhausted native production history now correctly reports a full hold without changing production. Seven new source-boundary checks plus the directly affected mine matrix case pass (8 cases/2.284s). Four selected public rows and the reproducible builder now use one fixed4x4 layout,view none,64x64/anchor(-31,-33),20-high sorting and512 required source slots per family; shared requirements are42,749. All424 prior artifact identities, all seven mine family documents,141 unrelated public rows and four previous executables remain exact. The temporary CMake output override was removed. [Compiled/source verification receipt](../../../../logs/graphics/processor-source/20261012-geometry-01/verification.json) and [native descriptor](../../../../media/multilayer/contracts/native-processor-geometry.json) retain the precise proof. Original processor source, packed ranges and ordinary-game rendering remain pending.

### Local Mandatory Reading - Selected Surface processor originals

_Applies to Groups39-42 under master10.4C after39B._

Read Global/Source generation Area, completed39B's compiled native geometry/state handoff, Plan6 Public Handoff Contract and only the current processor's recipes. Consume the actual one-layout/sixteen-cell geometry and derived status API; retain Industry inventory, paid funding, recipe choice/work, date and synchronized tick ownership. Reuse an individual compatible original only through exact physical role/camera/anchor ancestry. Read the canonical framed prompt/installed imagegen skill before generating raster art. Preserve all selected mine and earlier source bytes. Source originals grant no packed sprite range or ordinary-game display under10.4D.

### Group 39 - Author metal refinery

Selected dispatch resumed,2026-10-12, under master10.4C after complete native39B. Consume the compiled fixed sixteen-cell geometry and pure native stock/work status before selecting actual metal-refinery originals. The historical finalization skip is superseded for this dispatched source group; source tasks remain unchecked until their own complete review passes.

{{HARD}}
- [X] 39.1 For metal refinery, derive a single-model brief from its approved manifest/template: profile-selected Surface receiver footprint and recipe cues. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [X] 39.2 Produce the required directions/rotations for metal refinery from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [X] 39.3 Add construction/working/full/idle states for metal refinery, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.

Bounded execution,2026-10-12: selected base/direction tasks39.1-39.2 dispatch39C below; complete original39.3 through39D afterward. Native39B is complete. All original requirements remain intact; a base-only proof cannot close source coverage or master10.4C.

### Local Mandatory Reading - Metal refinery completed base originals

_Applies to Group39C, completing original39.1-39.2._

Read Global/Source generation Area, completed39B's exact `facility.metal-refinery` contract and current CFMT/BFMT/NFMT recipe rows. Read only needed M1 control-hut/no-cargo filter ancestry and compatible mine construction registrations. Construction originals explicitly missing guards, covers or mesh cannot be relabelled as a completed apparatus. Author genuine completed empty fittings for the common ore/Coal apparatus and new furnace/casting/finishing/cooling/loading/service equipment; retain only explicit physical-role-compatible reuse. No fixed cargo pile can impersonate the native selected alternative or actual stored quantity. Keep16 cells/view none/64x64/anchor(-31,-33)/20-high sorting, ordinary source rectangles and current native ownership. Read the canonical framed prompt/installed imagegen skill before raster work. Completed base has32 source parts; full512 source and native display remain39D/10.4D.

### Group 39C - Select the genuine completed metal refinery base

{{HARD}}
- [X] 39C.1 Freeze a recipe-compatible sixteen-cell model brief and exact component reuse graph; author/select genuine completed empty receiving/crushing/screening apparatus and distinct refinery equipment with immutable prompts/references.
- [X] 39C.2 Register every actual native cell and its independent back/front source parts. Verify whole original crops, physical inventory, low geometry, four floor contacts, camera/material continuity and source-only native-scale review without changing the installed footprint or sorting volume.
- [X] 39C.3 Publish a reproducible32-part completed base handoff, pass focused missing/foreign/changed-source/reuse/geometry refusals and preservation checks, then mark original39.1-39.2. Keep original39.3 and full source/packing/runtime pending.

Group39C completed-base closure,2026-10-12: all three tasks and original39.1-39.2 areX. Twelve genuine completed empty equipment cells plus four explicit compatible component reuses supply all sixteen actual native cells and32 nonempty independent back/front parts. Five selected immutable built-in ImageGen originals retain executed prompts/reference hashes; partially selected support-sheet cells13/15 remain valid while rejected floor cells8/14 use new genuine isolated originals. All actual four contacts/south tips/20-high volumes, completed inventory/empty fittings and native-scale model continuity pass. Eight focused checks pass in12.263s, preserving448 prior source/native artifacts, exact mine-record bytes,144 other public rows and all five prior verified executables. [Base verification receipt](../../../../logs/graphics/metal-refinery/20261012-base-01/verification.json), [native-scale review](../../../../logs/proofs/mlm/native-art/ground/metal-refinery-base-review.png) and [immutable originals/prompts](../../../../media/multilayer/source/ground/processor-generation-record.json) retain evidence. Full512 source coverage remainsfalse; original39.3/39D construction/operating poses and master10.4D packing/game proof remain pending. No C++build or full regression.

### Local Mandatory Reading - Metal refinery construction and derived operating poses

_Applies to Group39D, completing original39.3 after39C._

Read Global/Source generation Area, completed39C's selected physical model/original registrations and39B's native construction/ready/working/blocked owner policy. Reuse compatible original unfinished apparatus only through explicit per-cell physical/stage ancestry. New refinery equipment needs genuine unfinished stages matching its completed form; no erased body, generic copied campus or finishing-stage relabel supplies them. M1 original construction/status material may serve the identical control fitting only. Keep every apparatus/floor and actual Industry stock/work unchanged in operating state variants. Verify all512 required source slots, native phase aliases and intentional original underlay/front overlap before source publication. Native packing/decoded pixels/ordinary game remain10.4D.

Context has compacted 1 times while executing this group.

### Group 39D - Complete genuine metal refinery construction and operating source

{{HARD}}
- [X] 39D.1 Author/select genuine construction stages for the new refinery machinery and bind compatible original foundation/unfinished apparatus through explicit per-cell physical roles and unchanged native geometry.
- [X] 39D.2 Assemble native ready/working/full-or-paused state cues from original material without altering machinery/floor or introducing painted stock/work authority; preserve per-cell construction and synchronized working/static phase aliases.
- [X] 39D.3 Publish complete512-slot metal refinery source, review native state/foreground/contact sheets and pass focused provenance/coverage/false-proof/preservation checks. Mark original39.3 only after this full source proof; packing/game evidence stays pending.


Group39D closure,2026-10-12: all three staged tasks and original39.3 areX. Genuine unfinished/finishing originals supply seven new equipment cells per stage; eight compatible empty apparatus targets per stage and exact original M1 foundation/hut/glass retain individual physical ancestry. All512 required slots are nonempty at2048x1024; construction stage3 preserves the completed32-part base exactly. Ready/blocked alias native phase0, while four genuine cyan working phases change only the existing3x3 control fitting; no apparatus/floor/stock/work changes. All10 focused checks pass in31.692s, including the directly affected base/full-source handoff boundary. All462 earlier artifacts, seven completed-original candidate registrations,144 unrelated public rows, exact mine record and five existing verified executables remain unchanged. [Source verification receipt](../../../../logs/graphics/metal-refinery/20261012-states-01/verification.json), [state review](../../../../logs/proofs/mlm/native-art/ground/metal-refinery-source-review.png) and [independent construction cells](../../../../logs/proofs/mlm/native-art/ground/metal-refinery-source-construction-cells.png) retain the evidence. No C++build or full regression. Source coverage is complete; packed ranges/runtime activation remain absent under master10.4D.

### Local Mandatory Reading - Chemical works completed model and base

_Applies to Groups40/40B under master10.4C after completed39D._

Read Global/Source generation Area and39B's compiled `facility.chemical-works` single-layout sixteen-cell/state contract. Read only current CHEM/PCHM recipes and Plan6 Public Handoff Contract. Derive the complete empty apparatus model from the real Salt/Oil-to-Chemicals and Chemicals-to-Propellant alternatives, keeping all quantities/work in Industry. Reuse individual compatible original control/filter/service apparatus only with explicit native geometry, physical-role and raw ancestry; a copied refinery campus or painted stock cannot establish chemical equipment. Read the canonical framed prompt/installed imagegen skill before raster generation. Preserve completed metal/refinery, mine and earlier source/payload bytes and all five verified executables. Full512 source remains40C; packing/native display remain10.4D.

### Group 40 - Author chemical works

Selected source dispatch resumed,2026-10-12, after complete Metal39D. The historical finalization skip is superseded for this selected family. Staged40B completes original40.1-40.2;40C completes original40.3 after genuine construction/operating source. All original requirements remain intact.

{{HARD}}
- [X] 40.1 For chemical works, derive a single-model brief from its approved manifest/template: profile-selected Surface receiver footprint and recipe cues. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [X] 40.2 Produce the required directions/rotations for chemical works from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [X] 40.3 Add construction/working/full/idle states for chemical works, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.


### Group 40B - Select the genuine completed chemical works base

{{HARD}}
- [X] 40B.1 Freeze the actual sixteen-cell single-model chemical works brief and explicit individual compatible reuse graph from current native geometry/recipes; keep empty equipment and quantity/work ownership clear.
- [X] 40B.2 Author/select genuinely completed new chemical apparatus at native scale, preserving approved model inventory, fixed view, safe original rectangles, floor contacts and foreground parts.
- [X] 40B.3 Publish the reproducible32-part completed base and pass focused original/model/refusal/preservation checks; mark original40.1-40.2 only after this base review. Full512 source and runtime remain pending.

Group40B closure,2026-10-12: all three staged tasks and original40.1-40.2 areX. Seven genuine new completed chemical apparatus originals and nine explicitly compatible individual/local/M1 cells supply32 nonempty source parts at one fixed4x4 layout/view none. Closed fluid vessels and empty dry fittings communicate the real CHEM/PCHM alternatives without depicting stock or replacing Industry recipe/work. Corrected native day-tank symmetry, low reactor piping/compact drive, two same-level exchanger tubes and six fully installed short filter plates fit the unchanged64x64/anchor(-31,-33)/20-high envelope. Eight focused checks pass in7.205s. All487 prior artifact identities, the complete Metal family/candidate registrations, exact mine record,144 unrelated public rows and five native executables remain exact. [Completed-base receipt](../../../../logs/graphics/chemical-works/20261012-base-01/verification.json) and [native-scale model review](../../../../logs/proofs/mlm/native-art/ground/chemical-works-base-review.png) retain the evidence. Minimum selected crop clearance17px, zero opaque separator pixels. No C++build or full regression. Full512 coverage remainsfalse; original40.3/40C and master10.4D packing/native display remain pending.

### Local Mandatory Reading - Chemical works construction and derived operating poses

_Applies to Group40C, completing original40.3 after40B._

Read Global/Source generation Area, completed40B's physical model/original registrations and39B's native construction/ready/working/blocked policy. Consume compatible unfinished apparatus only through exact per-cell/stage ancestry; genuinely new equipment needs matching unfinished/finishing originals. Preserve every completed base pixel. Use original M1 lamp material only at its identical physical fitting, retaining Industry recipe/stock/work and synchronized/static phase aliases. Verify all512 source slots plus foreground/native contacts before publication; native packing/decoded pixels/ordinary game remain10.4D.

### Group 40C - Complete genuine chemical works construction and operating source

{{HARD}}
- [X] 40C.1 Author/select matching genuine construction stages for new chemical apparatus and bind compatible individual foundation/unfinished originals through explicit stage/geometry ancestry.
- [X] 40C.2 Assemble derived ready/working/full-or-paused original status cues without changing apparatus/floor or introducing stock/work authority; preserve actual native per-tile construction and phase aliases.
- [X] 40C.3 Publish/review complete512-slot chemical works source and pass focused provenance/coverage/refusal/preservation checks; mark original40.3 only after full source proof. Native packing/display remain10.4D.


Group40C closure,2026-10-12: all three staged tasks and original40.3 areX. The interrupted commit retained13/14 new construction cells; one built-in ImageGen edit completes the finishing filter press with six installed plates and its final handwheel absent. Fourteen genuine new stage components, twelve exact individual same-stage donor mappings, local aliases8->5/10->3 and original M1 foundation/hut/glass supply all512 nonempty source parts at2048x1024. Completed32-part base pixels remain exact; working changes only the existing3x3 glass fitting through four original phases; ready/blocked retain static0 aliases. All17 focused chemical base/state cases pass in27.245s, including corrupt ancestry/stage/coverage/payload refusals and preservation of499 prior artifacts,144 other public rows, the complete Metal family and completed candidates. [Source receipt](../../../../logs/graphics/chemical-works/20261012-states-01/verification.json) retains prompt/reference/cache/raw identities and reviewed source outputs. The earlier diagnostic run exposed an unchanged test mutation; the corrected foreign-family refusal and final burst pass. Source closure preceded the separately requested demo rebuild. Packed ranges/runtime remain absent. At the user's request, prepare [the underground demo](d2um-underground-mines.md) next; resume selected Group41 afterwards. No further source family is dispatched by this demo request.

### Group 41 - Author Surface battery works

Goal-authorized skip (2026-10-08, master finalization): dispatched expansion work in this group is skipped under [finalization exception GP-TG](portal-tbm/r2mlvs3mp-z-a-finalization-ledger.md#gp-tg-tube-and-geothermal-expansion-requires-major-gap-planning). Existing delivered prefixes stay complete; unimplemented profile work remains pending. No skipped task is markedX.

{{HARD}}
- [ ] 41.1 For Surface battery works, derive a single-model brief from its approved manifest/template: profile-selected Surface receiver footprint and recipe cues. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [ ] 41.2 Produce the required directions/rotations for Surface battery works from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [ ] 41.3 Add construction/working/full/idle states for Surface battery works, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.


### Group 42 - Author electronics works

Goal-authorized skip (2026-10-08, master finalization): dispatched expansion work in this group is skipped under [finalization exception GP-TG](portal-tbm/r2mlvs3mp-z-a-finalization-ledger.md#gp-tg-tube-and-geothermal-expansion-requires-major-gap-planning). Existing delivered prefixes stay complete; unimplemented profile work remains pending. No skipped task is markedX.

{{HARD}}
- [ ] 42.1 For electronics works, derive a single-model brief from its approved manifest/template: profile-selected Surface receiver footprint and recipe cues. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [ ] 42.2 Produce the required directions/rotations for electronics works from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [ ] 42.3 Add construction/working/full/idle states for electronics works, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.


### Group 43 - Author component manufacture

Goal-authorized skip (2026-10-08, master finalization): dispatched expansion work in this group is skipped under [finalization exception GP-TG](portal-tbm/r2mlvs3mp-z-a-finalization-ledger.md#gp-tg-tube-and-geothermal-expansion-requires-major-gap-planning). Existing delivered prefixes stay complete; unimplemented profile work remains pending. No skipped task is markedX.

{{HARD}}
- [ ] 43.1 For component manufacture, derive a single-model brief from its approved manifest/template: profile-selected Surface receiver footprint and recipe cues. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [ ] 43.2 Produce the required directions/rotations for component manufacture from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [ ] 43.3 Add construction/working/full/idle states for component manufacture, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.


### Group 44 - Author robotics manufacture

Goal-authorized skip (2026-10-08, master finalization): dispatched expansion work in this group is skipped under [finalization exception GP-TG](portal-tbm/r2mlvs3mp-z-a-finalization-ledger.md#gp-tg-tube-and-geothermal-expansion-requires-major-gap-planning). Existing delivered prefixes stay complete; unimplemented profile work remains pending. No skipped task is markedX.

{{HARD}}
- [ ] 44.1 For robotics manufacture, derive a single-model brief from its approved manifest/template: profile-selected Surface receiver footprint and recipe cues. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [ ] 44.2 Produce the required directions/rotations for robotics manufacture from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [ ] 44.3 Add construction/working/full/idle states for robotics manufacture, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.


### Group 45 - Author mirror-kit manufacture

Goal-authorized skip (2026-10-08, master finalization): dispatched expansion work in this group is skipped under [finalization exception GP-TG](portal-tbm/r2mlvs3mp-z-a-finalization-ledger.md#gp-tg-tube-and-geothermal-expansion-requires-major-gap-planning). Existing delivered prefixes stay complete; unimplemented profile work remains pending. No skipped task is markedX.

{{HARD}}
- [ ] 45.1 For mirror-kit manufacture, derive a single-model brief from its approved manifest/template: profile-selected Surface receiver footprint and recipe cues. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [ ] 45.2 Produce the required directions/rotations for mirror-kit manufacture from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [ ] 45.3 Add construction/working/full/idle states for mirror-kit manufacture, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.


### Group 46 - Author nuclear plant

Goal-authorized skip (2026-10-08, master finalization): dispatched expansion work in this group is skipped under [finalization exception GP-TG](portal-tbm/r2mlvs3mp-z-a-finalization-ledger.md#gp-tg-tube-and-geothermal-expansion-requires-major-gap-planning). Existing delivered prefixes stay complete; unimplemented profile work remains pending. No skipped task is markedX.

{{HARD}}
- [ ] 46.1 For nuclear plant, derive a single-model brief from its approved manifest/template: profile-selected Surface receiver footprint and recipe cues. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [ ] 46.2 Produce the required directions/rotations for nuclear plant from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [ ] 46.3 Add construction/working/full/idle states for nuclear plant, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.


### Local Mandatory Reading - Actual dated generator source geometry

_Applies to Groups55-56; supplements Source generation Area._

Read only the two public generator manifest rows, `MultiLayerGeothermalConstructionKind`/`GetMultiLayerGeothermalConstruction`, `PrepareGeothermalSite`, current registered IIDS/TIDS in `src/geothermal_definitions.cpp`, and verified Plan18 Group8B/`GetGeothermalGeneratorGeometry`; operating Group8 remains pending. Actual paid footprint/native cell layout and profile-owned construction terms must precede dependent originals; historical manifest sketches alone do not establish a delivered native type. If a dated geometry producer is missing, supply a bounded passive producer in its owning Plan18 group before authoring, retaining whole operating activation/transported bootstrap for master10.6. Reuse fixed camera/light and genuine original status components after geometry is proven; never substitute the collector body or silently activate an unfunded generator.

For resumed55.3 and later generator originals, use the [framed-sheet prompt and extraction checks](#framed-imagegen-sheets-and-safe-extraction). The bordered stage2 trial passed layout extraction but failed model identity; it remains excluded. Collector packing/display79C remains ahead of further generator authoring in the master queue.



For MagmaLoop source, the verified passive2000 descriptor supplies the canonical4x4 cells and four native presentation stages. It intentionally has no first-build invoice. Do not copy VentTap's592-tick duration or create a second paid construction contract during art authoring; native2000 work timing/materials remain operating Plan18 Group8. Source stage0..3 identifies incomplete hardware only until that owner binds actual work.

### Group 55 - Author Vent-Tap Powerhouse

Completed prerequisites for this mixed group are retained in [r2mlvs13-graphics-and-asset-pipeline.md](r2mlvs13-graphics-and-asset-pipeline.md#group-55---author-vent-tap-powerhouse). Original task IDs remain unchanged.

Dispatched under resumed master10.4. Plan18 staged8B verifies both dated4x4/sixteen-cell native geometries before55.1. Tasks55.1-55.3 and the explicitly staged collector display79C are complete;55B below retains the four verified source checkpoints. Whole operating generator/transported bootstrap remains master10.6.

{{HARD}}
- [X] 55.3 Add construction/working/heat-starved states for Vent-Tap Powerhouse, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.


### Local Mandatory Reading - Powerhouse construction and equipment states

_Applies to Group55B; completes original55.3 after verified collector79C._

Read this plan's Global/Source generation Area and Actual dated generator source geometry Local, the two generator public rows, and only the selected VentTap family/candidates in `geothermal-generation-record.json`. Read `convert_generator_art.native_base`, `partition_model`, `ready_models`, the executing construction-source helper and its focused checks. Geometry remains the verified Plan18 Group8B producer; no C++ geometry/operating change is required by source review. Preserve the four frozen ready registrations, exact native4x4/sixteen-cell ownership, all43 current packed ranges and immutable originals. Read the framed-sheet convention and installed imagegen skill before new raster work. The user's permanent permission and replacement AGENTS instructions on2026-10-11 authorize writes into `$CODEX_HOME` or `.codex/generated_images`, including imagegen's default cache. The earlier approval gate is resolved; preserve all other repository/Git restrictions.

Context has compacted 1 times while executing this group.

### Group 55B - Complete Powerhouse source in bounded steps

Original55.3 and55B.1-55B.4 are complete. This stages source authoring only; operating activation/transported bootstrap remains master10.6 and packing/runtime stays with the original selected-asset queue.

{{HARD}}
- [X] 55B.1 Review and freeze the eight retained stage0/1 views against approved model identity and native cell ownership. Correct only measured unfrozen registrations, preserve every original pixel, and reject unsupported equipment/footprint changes. Publish the scoped contacts and focused source/refusal checks; keep complete construction/state coverage false.
- [X] 55B.2 Produce and review genuine isolated stage2 views for NE/SE/SW/NW from their approved ready targets, preserving one hall/turbine/alternator/exchanger/switchgear. Freeze all twelve incomplete views and declare native stage3's approved-ready hardware alias under amber construction glass. Do not select either rejected stage2 sheet.
- [X] 55B.3 Author the missing genuine heat-starved glass component and compose all declared states/four equipment phases through the established original-pixel source pipeline. Native stages use four148-tick quarters with completion separate at592; other states change only declared glass/moving parts. Preserve every other original pixel and all16 native cells.
- [X] 55B.4 Review the complete model/state/phase contact matrix and exact2,560-pose coverage, run focused source/refusal/contract checks, retain original/proof identities and publish source completion. Mark55.3 only after every source requirement passes; leave operating generator and later packing/runtime obligations pending.

55B.1 closure,2026-10-10: all eight retained stage0/1 views and128 native cells are reviewed/frozen. Stage1 NE's previously unfrozen placement moves from(33,51) to(33,50), preserving its original bytes/crop/scale while fixing the otherwise unowned pixel(128,175). Ready registrations stay exact. `construction_models` binds each original's actual paid stage, direction and frozen registration; missing stage2, ready substitution, drift, duplicate/missing views and altered original identities refuse. Eight focused construction cases pass13.073 seconds; thirteen ready compatibility cases pass8.122 seconds; public145/53,921 validation passes. [Scoped source identities and contact evidence](../../../../logs/graphics/geothermal-source/20261010-venttap-construction-01/verification.json) retain the actual8-view/128-cell proof. All43 packed ranges, shipped GRF and four native binaries remain unchanged; complete generator source/runtime flags stay false.55B.2 resumes on2026-10-11 with permanent cache-write authorization. Four isolated edit prompts and exact targets are prepared in the continuation.

55B.2 closure,2026-10-11: six isolated built-in imagegen edits retain exact prompts/reference/cache/source hashes. NEv1 is excluded for its clipped near floor; NEv2 is excluded for the wrong roof opening and loose pipe. Selected NEv3/SEv1/SWv1/NWv1 retain the genuine whole model, central unfinished roof and disconnected copper connection. All four native/enlarged/cell contacts pass model review, and all64 new cells preserve exact sampled pixels. Measured SE/SW placement is(33,57); NE is(33,58), NW(33,63). All twelve incomplete registrations are frozen; native stage3 explicitly aliases approved ready hardware under amber phase0, with completion separate at592. [Combined twelve-view receipt](../../../../logs/proofs/mlm/native-art/ground/venttap-construction-0-1-2.json) retains registrations and contact identity. Equipment composition and full2,560-pose source coverage remain55B.3-55B.4; source/runtime flags stay false.

55B.3-55B.4 closure,2026-10-11: the genuine four-phase purple Heat-shortage original and existing M1 status original fill all sixteen frozen lamp interiors. The assembler preserves every non-glass indexed pixel, opacity and unique native owner; stage3 retains amber until native completion. All four complete state/phase contacts are reviewed. Ten focused construction cases pass18.928 seconds and thirteen state/source/refusal cases pass48.479 seconds; public145/53,921 validation passes. Source publication reproduces the same atlas/four contacts exactly, then marks public source coverage true with2,560 poses and runtime false. [Closure identities and evidence](../../../../logs/graphics/geothermal-source/20261011-venttap-states-01/verification.json) retain all originals/components/artifacts. All43 packed ranges, runtime manifest, shipped GRF and four native binaries remain unchanged. No C++ build or full regression was run. Original55.3 and all55B tasks areX;56.1 is next.

### Group 56 - Author Magma-Loop Power Station

Resumed2026-10-11 under master10.4C after complete55.3. The earlier finalization skip is historical; execute all three source tasks using verified Plan18 Group8B geometry. Actual operating registration and transported bootstrap remain master10.6.

{{HARD}}
- [X] 56.1 For Magma-Loop Power Station, derive a single-model brief from its approved manifest/template: 2000 closed-loop plant distinct from the early powerhouse. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [X] 56.2 Produce the required directions/rotations for Magma-Loop Power Station from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [X] 56.3 Add construction/working/heat-starved states for Magma-Loop Power Station, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.


56.1-56.2 source closure,2026-10-11: five immutable originals supply four selected genuine ready views, with NEv1 excluded for shortened vessel proportions and corrected NEv2 selected after the combined review. Exactly two closed-loop vessels, one enclosed integrated turbo-generator, one flat control hut and one switchgear remain in each physical turn. All four native/enlarged/cell contacts and the combined ready matrix are reviewed; all64 actual cells reconstruct their originals losslessly. The public source registrations are frozen; full source/state coverage and runtime remain false pending56.3. No native build or56 unit burst has run yet.

56 closure,2026-10-11: seventeen immutable whole originals retain sixteen selected hardware views and one excluded short-vessel NE draft. Twelve genuine incomplete views supply stages0..2; stage3 aliases approved ready hardware under amber phase0. All native/full contacts and sixteen2x2 glass interiors are reviewed; existing M1/purple Heat components supply all five states/four phases. Exact2,560-pose source publication reproduces the reviewed atlas/four contacts. Thirteen focused source/refusal cases pass66.542 seconds and public145/53,921 validation passes. Decoded native poses preserve every original non-glass indexed pixel and opacity; all256 hardware sections reconstruct the sixteen originals. MagmaLoop retains no invented work duration or1980 invoice; actual operating terms/activation stay Plan18 Group8. [Source closure identities](../../../../logs/graphics/geothermal-source/20261011-magmaloop-states-01/verification.json) retain exact source/component/guide/artifact identities. Source coverage is true, packing/runtime false; all43 packed ranges, shipped GRF/runtime manifest and four native binaries are unchanged. No C++ build or full regression.56.1-56.3 areX;60.1 is next.

### Local Mandatory Reading - Remaining plant and orbital originals

_Applies to Groups57-78._

Read this plan's Global and Source generation Area, the executing family's manifest row, and its feature owner's delivered geometry/state contract. Geothermal geometry is owned by Plan18; orbital and solar providers retain Plans8-12. Consult only that family's completed Plan13 source/style examples and the canonical framed prompt. Keep unimplemented providers and missing source/packing/runtime coverage explicitly pending.

### Group 57 - Author Geothermal Smelter

{{HARD}}
- [ ] 57.1 For Geothermal Smelter, derive a single-model brief from its approved manifest/template: 2010 ore/direct-heat furnace footprint. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [ ] 57.2 Produce the required directions/rotations for Geothermal Smelter from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [ ] 57.3 Add construction/working/heat-starved/full states for Geothermal Smelter, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.


### Group 58 - Author Lithothermal Battery Works

{{HARD}}
- [ ] 58.1 For Lithothermal Battery Works, derive a single-model brief from its approved manifest/template: 2030 chemical/battery processing footprint. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [ ] 58.2 Produce the required directions/rotations for Lithothermal Battery Works from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [ ] 58.3 Add construction/working/heat-starved/full states for Lithothermal Battery Works, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.


### Group 59 - Author Mantle Forge

{{HARD}}
- [ ] 59.1 For Mantle Forge, derive a single-model brief from its approved manifest/template: 2045 heavy component machinery footprint. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [ ] 59.2 Produce the required directions/rotations for Mantle Forge from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [ ] 59.3 Add construction/working/heat-starved/full states for Mantle Forge, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.


### Group 60 - Author heat/electric utilities and risers

Resumed2026-10-11 under master10.4C after completed generator source55-56. The earlier finalization skip is historical. Freeze actual Heat/electrical per-cell and paid2x2 riser geometry before dependent originals; operating plant/transported bootstrap remains master10.6.

Context has compacted 1 times while executing this group.

Execution split,2026-10-11: deliver the installed per-cell conduit family in60B, then the separate paid electrical riser in60C. These bounded child groups preserve all60.1-60.3 requirements; the original aggregate boxes remain pending until both families pass. No master checkpoint is added.

{{HARD}}
- [X] 60.1 For heat/electric utilities and risers, derive a single-model brief from its approved manifest/template: visually different thermal and electrical links. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [X] 60.2 Produce the required directions/rotations for heat/electric utilities and risers from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [X] 60.3 Add connected/broken/idle conduit and portal-riser states for heat/electric utilities and risers, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.

### Local Mandatory Reading - Installed utility conduit originals

_Applies to Group60B._

Read this plan's Global/Source generation Area, public `geothermal.conduit`, `GeothermalUtilityLink`/`GetGeothermalUtilityLink`/`GetGeothermalUtilityPortOwner` and the actual cardinal path/coexistence implementation in `geothermal_utility.cpp`. Read the frozen domain/layer/rate terms in `multilayer_utility.cpp`, completed Plan18 Group3B/4 handoff and only the applicable focused native path tests. The completed `tube.conduit` converter supplies extraction/anchor conventions, not geothermal topology or electrical authority. Read Plan13's canonical framed prompt and the imagegen skill before originals. Separate coexisting Heat/Electricity domains; no path pixels create installation, flow or grants.

### Group 60B - Author installed Heat and electrical conduits

{{HARD}}
- [X] 60B.1 Reconcile the public domain/branch/state contract with actual installed cardinal cells; freeze the brief, contacts and coexistence geometry, then select a readable original base for each distinct domain with immutable prompt/provenance.
- [X] 60B.2 Produce genuine required physical branch views from those selected models, retaining fixed native anchors and matching adjacent-cell contacts; compare turns, joins, isolated cells and both domains together.
- [X] 60B.3 Author connected/unconnected and construction presentation, retain exact source selections and foreground policy, and validate complete decoded source coverage plus focused corruption refusals. Keep packing/runtime activation pending.

60B closure,2026-10-11: twelve immutable built-in originals retain two distinct domain models, genuine X/Y views, construction previews and capped fittings. Twenty-four decoded native branch/state/domain poses pass twelve focused source/refusal cases in4.346 seconds; actual paid path/coexistence/port boundary passes761 assertions/seven cases. Four original cap compositions preserve every other connected-body indexed pixel and all opacity. All16 neighbour masks, actual32/16 joins and both domains together are reviewed; atlas/source manifest and three contacts reproduce identically. Public schema validates145 families/53,805 required poses after correcting distinct conduit domains and electrical-only riser states. [Source closure identities](../../../../logs/graphics/utility-source/20261011-conduit-01/verification.json) retain original prompts/cache/reference identities and all source artifacts. Source coverage is true; packing/runtime false. All43 packed ranges, shipped payload/runtime manifest, native binaries and completed generator source are unchanged. No C++ build or full regression.60B.1-60B.3 areX;60C.1 is next, and original mixed60.1-60.3 remain pending.

### Local Mandatory Reading - Paid electrical riser originals

_Applies to Group60C._

Read this plan's Global/Source generation Area, public `facility.utility-riser`, `GeothermalRiserRequest`/`GeothermalRiserState`/`IsGeothermalRiserOperational`, actual native2x2 layout and frozen296-tick paid construction terms. Read completed Plan18 Groups5B/5 only for machinery geometry, original portal binding and electrical readiness. Read Plan13's canonical framed prompt and the imagegen skill before originals; use generator/collector partition examples only as necessary. The riser transfers electricity through an actual commissioned pair; it has no Heat-starved consumer state and never creates a shaft or excavates floor through artwork.

### Group 60C - Author paid electrical riser machinery

{{HARD}}
- [X] 60C.1 Freeze the electrical-only native2x2 machinery/port/portal brief and correct source states, then select a complete base silhouette with immutable prompt/provenance.
- [X] 60C.2 Produce genuine required machinery views at fixed native cell anchors and compare equipment identity, adjacent portal contacts and foreground policy.
- [X] 60C.3 Author actual construction/readiness/working/blocked presentation, validate complete source coverage and focused corruption refusals, and close aggregate60 only after both60B and60C pass. Keep packing/runtime activation pending.


60C/60 closure,2026-10-11: the actual native definition has one fixed row-major2x2 layout; access/portal attachment does not rotate it. Public riser source now correctly has one view and electrical construction/ready/working/blocked states, yielding128 poses. Five retained built-in originals include four selected hardware stages and excluded nine-cell-floor draftv1. Corrected readyv2 and stages0..2 retain every original indexed pixel in sixteen lossless native sections. Four reviewed2x2 glass interiors reuse actual M1 four-state/four-phase originals; all sixteen decoded whole poses preserve nonglass body pixels/opacity, with solid native industry back and empty front. Stage3 aliases ready hardware under amber phase0, retaining separate completion at296 native ticks/four74-tick stages. Native definition/admission/construction boundary passes4,172 assertions/ten cases. Final twelve source/refusal cases pass22.641 seconds; the affected conduit-record case and public145/53,421 schema/refusals pass. Atlas/manifest/state contact regenerate identically. [Exact source closure identities](../../../../logs/graphics/utility-source/20261011-riser-01/verification.json) retain original prompts/references/cache identities, frozen registrations and reviewed source artifacts. Earlier diagnostic logs retain transparent-donor refusal and generic fixed-layout whitelist corrections. Source coverage is true; packing/runtime false. Conduit/generator source,43 native packed families, payload/runtime manifest and four native binaries are preserved; no C++ build/full regression.60C.1-60C.3 and aggregate60.1-60.3 areX. Continue related78C, then the selected fleet/facility/supply source families before master10.4D packing.

### Group 61 - Author Space Launch Center

{{HARD}}
- [ ] 61.1 For Space Launch Center, derive a single-model brief from its approved manifest/template: complete pad/assembly/recovery footprint with Surface freight access. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [ ] 61.2 Produce the required directions/rotations for Space Launch Center from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [ ] 61.3 Add all rotations/preparation/launch/occupied foreground parts for Space Launch Center, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.


### Group 62 - Author cargo rocket

{{HARD}}
- [ ] 62.1 For cargo rocket, derive a single-model brief from its approved manifest/template: reusable hull and payload/service identity. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [ ] 62.2 Produce the required directions/rotations for cargo rocket from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [ ] 62.3 Add loading/preparing/ascent/orbit/recovery/service views for cargo rocket, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.


### Group 63 - Author small mining probe

{{HARD}}
- [ ] 63.1 For small mining probe, derive a single-model brief from its approved manifest/template: compact hook/mining robot silhouette. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [ ] 63.2 Produce the required directions/rotations for small mining probe from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [ ] 63.3 Add free/attached/mining/loaded/service directional views for small mining probe, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.


### Local Mandatory Reading - Survey and construction probe art

_Applies to Groups63B/63C._ Read this plan's Global and the Space fleet authoring
Area, the shared Space economy revision, actual role/geometry/state exports
from Plans8-11, and the approved sprite manifest/template contract. Geometry
and role states must be frozen before directions. New families use the same
source/provenance, conversion, packed-load and native runtime review owners;
recoloring the mining placeholder cannot finish either distinct model.

### Group 63B - Author survey probe

{{HARD}}

- [ ] 63B.1 Freeze the native model19 brief and distinct scanner silhouette; author/select an original at native scale with source/prompt/provenance and consistent anchors.
- [ ] 63B.2 Produce the required directions and free/approach/scanning/return/service states from that model; validate continuity and scan readability against actual saved role states.
- [ ] 63B.3 Publish validated source/contact sheets and manifest entries to the existing conversion/packed/runtime pipeline. Final acceptance requires native ordinary-game captures; this task alone claims source delivery only.

### Group 63C - Author construction probe

{{HARD}}

- [ ] 63C.1 Freeze the native model20 brief and distinct manipulator/material-carrier silhouette; author/select an original at native scale with source/prompt/provenance and consistent anchors.
- [ ] 63C.2 Produce required directions and empty/Steel/Goods-loaded/constructing/return/service states, preserving native cargo units/weight and actual approach/work geometry.
- [ ] 63C.3 Publish validated source/contact sheets and manifest entries through the existing pipeline; bind load/work states to native role/cargo queries and retain later packed/runtime acceptance.

### Group 64 - Author cargo return pod

{{HARD}}
- [ ] 64.1 For cargo return pod, derive a single-model brief from its approved manifest/template: separate saved descent unit. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [ ] 64.2 Produce the required directions/rotations for cargo return pod from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [ ] 64.3 Add release/descent/recovery frames for cargo return pod, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.


### Group 65 - Author heavy mining robot

{{HARD}}
- [ ] 65.1 For heavy mining robot, derive a single-model brief from its approved manifest/template: larger station-built extraction machinery. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [ ] 65.2 Produce the required directions/rotations for heavy mining robot from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [ ] 65.3 Add free/attached/mining/loaded/service directional views for heavy mining robot, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.


### Group 66 - Author orbital hauler

{{HARD}}
- [ ] 66.1 For orbital hauler, derive a single-model brief from its approved manifest/template: cargo carrier distinct from miners. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [ ] 66.2 Produce the required directions/rotations for orbital hauler from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [ ] 66.3 Add direction/load/docking/service and purchase views for orbital hauler, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.


### Group 67 - Author metallic asteroid

{{HARD}}
- [ ] 67.1 For metallic asteroid, derive a single-model brief from its approved manifest/template: readable Nickel composition with stable body footprint. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [ ] 67.2 Produce the required directions/rotations for metallic asteroid from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [ ] 67.3 Add required size/attachment/mining/depleted variants for metallic asteroid, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.


### Group 68 - Author carbonaceous asteroid

{{HARD}}
- [ ] 68.1 For carbonaceous asteroid, derive a single-model brief from its approved manifest/template: readable Water Ice composition with stable body footprint. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [ ] 68.2 Produce the required directions/rotations for carbonaceous asteroid from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [ ] 68.3 Add required size/attachment/mining/depleted variants for carbonaceous asteroid, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.


### Group 69 - Author rich asteroid

{{HARD}}
- [ ] 69.1 For rich asteroid, derive a single-model brief from its approved manifest/template: readable Rare Earth composition with stable body footprint. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [ ] 69.2 Produce the required directions/rotations for rich asteroid from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [ ] 69.3 Add required size/attachment/mining/depleted variants for rich asteroid, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.


### Group 70 - Author Core module

{{HARD}}
- [ ] 70.1 For Core module, derive a single-model brief from its approved manifest/template: distinct station anchor footprint and connector/dock anchors. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [ ] 70.2 Produce the required directions/rotations for Core module from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [ ] 70.3 Add blueprint/partial/complete/blocked functional states for Core module, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.


### Group 71 - Author Dock module

{{HARD}}
- [ ] 71.1 For Dock module, derive a single-model brief from its approved manifest/template: distinct berth footprint and connector/dock anchors. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [ ] 71.2 Produce the required directions/rotations for Dock module from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [ ] 71.3 Add blueprint/partial/complete/blocked functional states for Dock module, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.


### Group 72 - Author Storage module

{{HARD}}
- [ ] 72.1 For Storage module, derive a single-model brief from its approved manifest/template: distinct bounded inventory footprint and connector/dock anchors. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [ ] 72.2 Produce the required directions/rotations for Storage module from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [ ] 72.3 Add blueprint/partial/complete/blocked functional states for Storage module, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.


### Group 73 - Author Utility module

{{HARD}}
- [ ] 73.1 For Utility module, derive a single-model brief from its approved manifest/template: distinct power/service footprint and connector/dock anchors. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [ ] 73.2 Produce the required directions/rotations for Utility module from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [ ] 73.3 Add blueprint/partial/complete/blocked functional states for Utility module, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.


### Group 74 - Author Assembly module

{{HARD}}
- [ ] 74.1 For Assembly module, derive a single-model brief from its approved manifest/template: distinct robot manufacture footprint and connector/dock anchors. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [ ] 74.2 Produce the required directions/rotations for Assembly module from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [ ] 74.3 Add blueprint/partial/complete/blocked functional states for Assembly module, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.


### Group 75 - Author Processor module

{{HARD}}
- [ ] 75.1 For Processor module, derive a single-model brief from its approved manifest/template: distinct material conversion footprint and connector/dock anchors. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [ ] 75.2 Produce the required directions/rotations for Processor module from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [ ] 75.3 Add blueprint/partial/complete/blocked functional states for Processor module, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.


### Group 76 - Author native solar farm

{{HARD}}
- [ ] 76.1 For native solar farm, derive a single-model brief from its approved manifest/template: Surface panel layout and utility connection. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [ ] 76.2 Produce the required directions/rotations for native solar farm from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [ ] 76.3 Add construction/normal/boosted/idle states for native solar farm, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.


### Group 77 - Author solar mirror and kit

{{HARD}}
- [ ] 77.1 For solar mirror and kit, derive a single-model brief from its approved manifest/template: deployed reflective structure distinct from cargo kit. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [ ] 77.2 Produce the required directions/rotations for solar mirror and kit from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [ ] 77.3 Add packed/deploying/aiming/operating/idle states for solar mirror and kit, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.


### Group 78 - Author construction and cargo controls

Goal-authorized skip (2026-10-08, master finalization): dispatched expansion work in this group is skipped under [finalization exception GP-TG](portal-tbm/r2mlvs3mp-z-a-finalization-ledger.md#gp-tg-tube-and-geothermal-expansion-requires-major-gap-planning). Existing delivered prefixes stay complete; unimplemented profile work remains pending. No skipped task is markedX.

Resumed2026-10-11 for the related master10.4C source subset in78C below. Completed ground78B remains in Plan13. Original78.1-78.3 retain their whole-profile/orbital scope and stay pending until all portions pass.

{{HARD}}
- [ ] 78.1 For construction and cargo controls, derive a single-model brief from its approved manifest/template: native-size layer/cargo identities with separate collection/drop circles. Produce and select the base silhouette at native scale; record source/prompt/provenance and resolve geometry before making variants.
- [ ] 78.2 Produce the required directions/rotations for construction and cargo controls from that selected form, using labelled engine-view templates and fixed anchors/contact points. Compare neighboring views for continuity; do not synthesize views by rotating a flat image.
- [ ] 78.3 Add menu/cursor/locked/blocked/turn-preview and resource icons for construction and cargo controls, changing only declared moving/load/status parts. Build a model-specific contact sheet and check missing views, anchor drift, silhouette readability and intended foreground cuts before handing these originals to conversion.

### Local Mandatory Reading - Tube and geothermal control source

_Applies to Group78C._

Read this plan's Global/Source generation Area and public `control.tube`/`control.geothermal` rows. Read only completed Plan13 Group78B control-source/palette/anchor conventions and the actual Plan17/18 command availability/ownership guards needed to describe the two identities. Read the canonical framed prompt and imagegen skill before raster work. Native20x20 enabled/disabled/pressed source does not imply that later tube/plant toolbar consumers are implemented; those runtime bindings remain master10.5/10.6. Existing date/turn/commissioning/cargo/layer controls stay delivered; orbital collection/drop scope remains pending.

### Group 78C - Author the related tube and geothermal control subset

{{HARD}}
- [X] 78C.1 Freeze briefs and public20x20 geometry for the two actual feature identities, reuse the completed ground control style and retain immutable original prompts/provenance.
- [X] 78C.2 Author enabled/disabled/pressed source for both families at fixed anchors; compare model identity and native readability without inventing rotations or operating providers.
- [X] 78C.3 Publish exact six-pose source/contact evidence with focused palette/ancestry/coverage refusals; retain packing/runtime and original whole78 completion separately.


78C closure,2026-10-11: six retained isolated built-in originals supply native20x20 enabled/disabled/pressed tube and geothermal menu identities. Both models retain fixed centers and identical per-family bounds; authored red refusal and cyan/pale pressed accents remain distinct at native scale. Plan7 owns shared UI consumer7; actual Plans17/18 commands retain admission. Ten focused original/palette/coverage/ancestry/refusal cases pass1.490 seconds. Both native atlases, two source manifests and the reviewed contact reproduce identically; every public packed field remains unchanged, including the existing optional152/155 declarations. The scoped tube/date/floor/geothermal command boundary passes524 assertions/four cases. [Exact source identities](../../../../logs/graphics/feature-controls/20261011-source-01/verification.json) retain prompts/cache/reference hashes, frozen registrations and artifacts. Source coverage is true; packing/runtime and whole original78 remain pending. No C++ build/full regression; earlier source/payload/native binaries remain unchanged.78C.1-78C.3 areX; next19.1 reconciles the already delivered native model4 locomotive reuse before new wagon/truck originals.

Observed native fixture issue for upcoming master10.5: an initially broader six-case selector first lacked generated language at repository root, then crashed in Windows Uniscribe font layout during `CmdBuildRailStation` when rerun from `build/`. The standalone terminal test calls `LoadGeneratedWindowTestLanguage` without the `MockEnvironment::Instance()` initialization used by the following GUI test; this is a suspected fixture-order dependency, not a demonstrated gameplay regression. Preserve [root diagnostic](../../../../logs/graphics/feature-controls/20261011-source-01/native-boundary.log) and [build-directory stack trace](../../../../logs/graphics/feature-controls/20261011-source-01/native-boundary-build-directory.log). Repair/verify standalone fixture initialization with the actual Plan17 integration burst; neither failed broader run is acceptance evidence for78C. The four explicitly scoped command cases and source checks above pass.

### Local Mandatory Reading - Delivered collector packing and actual display

_Applies to staged Group79C; consumes only the source-complete collector before unfinished generator source work._

Read this plan's Global, the Source generation Area's packing exports and only collector-relevant runtime ownership from the Runtime integration Area. Inspect `m1-pack-scope.json`, the public `facility.vent-collector` row and `collector-source-manifest.json`; source completeness and native anchors/cells must be real. Read `pack_m1_art.selected_ranges`, `source_poses`, `format_header`, `derive`, `publish` and `bind_packed_payload`, the base-set PNG/NFO/CMake routes and current generated native ranges. Preserve protected0..65, reserved slots, all existing42-family ranges, both generated and checked-in payload paths and immutable MLVI gameplay definitions.

For display, read `GetMultiLayerSprite`, `GetMultiLayerReceiverSprite`, native `DrawTile_Industry`, `GetIndustryConstructionStage`/`IsIndustryCompleted`, `GetGeothermalCollectorState`, `IsGeothermalCollectorOperational` and the actual collector/claim/shared-Heat state owners needed by the selected state. Reuse actual industry location/access/work/input/Heat records; do not invent a graphics-owned stock or operating generator. Inspect existing focused native sprite-loading and collector save/constructor fixtures only. A normal-game capture must disclose fixture-supplied materials and cannot claim the later Surface-transported bootstrap.

Completed79C.1-79C.2 are in [Plan13](r2mlvs13-graphics-and-asset-pipeline.md#group-79c---pack-and-display-the-delivered-collector):43 packed families/3495 selected poses, including1152 collector poses at2436 and native bound3588. Preserve the collector append and all42 earlier ranges. Completed79C.3-79C.4 below verify `GetMultiLayerCollectorSprite`/`GetMultiLayerIndustrySprite` and ordinary display; reuse their actual loading/selection/capture evidence. Payload decoding alone still cannot establish an untested runtime consumer.


### Group 79C - Pack and display the delivered collector

Completed prerequisites for this mixed group are retained in [r2mlvs13-graphics-and-asset-pipeline.md](r2mlvs13-graphics-and-asset-pipeline.md#group-79c---pack-and-display-the-delivered-collector). Original task IDs remain unchanged.

Explicitly staged by master10.4B after verified source54, before further unfinished55-56 originals. This closes only the collector subset of packing/runtime obligations; original whole Groups79-84 and aggregate master10.4 remain pending for their other selected families.

{{HARD}}
- [X] 79C.3 Bind the packed family to real native collector industry cells and construction/completion/current Heat states in the actual industry draw path. Visual clocks change presentation only; camera/animation selection must preserve work, cargo, funds, claims, Heat and RNG. Do not activate an operating powerhouse or replace its missing producer.
79C.3 closure,2026-10-10: actual paid Deep collector cells consume the appended family through `GetMultiLayerIndustrySprite`/`DrawTile_Industry`. Four real access-side claims, four earned construction stages, ready/current/stale/retired Heat and abandonment pass17,367 assertions/13 focused Debug cases, including existing receiver and shared selector checks. The native observer passes106 assertions and captures14 actual viewports/seven current saves; rendering preserves construction, native stock, cash, claims, utility/geology, clock and both RNG owners. Ordinary Debug construction/completion screenshots contain exact native24x24 machinery windows and preserve all eight checked native owner chunks on resave. Actual copied-payload loading verifies3,561 source-exact sprites. Evidence: [focused checks](../../../../logs/graphics/collector-integration/20261010-runtime-01/focused-Debug.log), [native report](../../../../mlm/reviews/geothermal/collector-art/20261010-collector-runtime-01/reports/native-Debug.json). Fixture-supplied floor/materials remain disclosed.79C.4 completes optimized checks and final scoped binding below.

- [X] 79C.4 After the packing and consumer changes, build both configurations once and run a focused combined source/packing/loading/selection/collector burst. Load a current native collector world in the ordinary game and capture the actual construction/completed machinery plus native source-pixel identities, with fixture staging disclosed. Close master10.4B only after this loaded evidence passes, then resume55.3. No full regression or transported first-plant claim is required by this art slice.

79C completion,2026-10-10: both Debug/RelWithDebInfo game and test builds pass. Focused selectors/native construction/shared Heat pass17,367 assertions/13 cases per configuration; actual copied-payload loaders pass60,559 assertions and3,561 exact sprites per configuration, including66 protected and3,495 selected. Packing/original/state checks pass14/17/15 cases; the public contract passes145 families/53,921 required sprites. The native observer captures14 real viewports/seven current saves; ordinary Debug construction/completion and optimized completion match exact native24x24 machinery pixels and all eight checked construction/stock/claim/vent/utility chunks on resave. The shared public/runtime handoff now binds43 families/3,495 poses, appending the1,152 collector poses while preserving all42 earlier ranges and MLVI gameplay identities. Missing completion, stale pixels and changed executable proofs refuse without changing published manifests. [Complete current identities and evidence](../../../../logs/graphics/collector-integration/20261010-runtime-01/verification.json) retain the actual builds, sources, reports and original payload. Fixture-open floor/material delivery remains explicit; transported bootstrap, operating generators and personal review are not claimed. Master10.4B isX; resume55.3 after its generator Local and framed ImageGen reading.


### Local Mandatory Reading - Remaining conversion and packing

_Applies to Groups79-81._

Read this plan's Global/Source generation Area and the executing selected families' manifest/source reports. Consume completed Plan13 packing exports, stable native ranges and actual generated/cached payload proofs; extend the same owners. Read only the relevant quantizer/packer/build paths, preserving existing sprite indices, anchors, palette rules and immutable source provenance. Native loading and gameplay presentation require their own current evidence.

### Group 79 - Convert and pack from stable originals

Goal-authorized skip (2026-10-08, master finalization): dispatched expansion work in this group is skipped under [finalization exception GP-TG](portal-tbm/r2mlvs3mp-z-a-finalization-ledger.md#gp-tg-tube-and-geothermal-expansion-requires-major-gap-planning). Existing delivered prefixes stay complete; unimplemented profile work remains pending. No skipped task is markedX.

{{HARD}}
- [ ] 79.1 Extend deterministic quantization/cropping to consume manifest records, preserve index-0 transparency and handle reserved company/animation colors explicitly. Preserve world anchor when transparent margins are cropped.
- [ ] 79.2 Generate back/front parts and NFO/content-package metadata in stable manifest order with pinned tool versions. Keep existing published sprite offsets unchanged.
- [ ] 79.3 Pack the selected core/content routes and regenerate required GRF/checksum metadata. Compare two clean conversions from the same originals/manifest for identical outputs.
- [ ] 79.4 Verify a build without the asset generator loads the checked-in generated payload and resolves the same IDs/counts; source-only success is not distributed-asset acceptance.


### Group 80 - Validate asset structure and palette deterministically

Goal-authorized skip (2026-10-08, master finalization): dispatched expansion work in this group is skipped under [finalization exception GP-TG](portal-tbm/r2mlvs3mp-z-a-finalization-ledger.md#gp-tg-tube-and-geothermal-expansion-requires-major-gap-planning). Existing delivered prefixes stay complete; unimplemented profile work remains pending. No skipped task is markedX.

{{HARD}}
- [ ] 80.1 Validate required record fields, unique IDs, expected direction/frame/state Cartesian coverage and exact native dimensions/anchors before packing. Report the failing asset ID and source.
- [ ] 80.2 Check transparent index, forbidden palette ranges, intended company-color pixels and empty/missing frames against the declared per-asset policy.
- [ ] 80.3 Compare manifest packed references and output hashes against emitted sprite count/offset metadata; reject stale or unreferenced gameplay assets.
- [ ] 80.4 Add small corruption fixtures for one missing frame, bad bounds and illegal palette entry. Validate authored transparent holes against explicit masks/templates; arbitrary legitimate holes still need human visual review.


### Group 81 - Source generation and reproducible conversion

Goal-authorized skip (2026-10-08, master finalization): dispatched expansion work in this group is skipped under [finalization exception GP-TG](portal-tbm/r2mlvs3mp-z-a-finalization-ledger.md#gp-tg-tube-and-geothermal-expansion-requires-major-gap-planning). Existing delivered prefixes stay complete; unimplemented profile work remains pending. No skipped task is markedX.

{{MEDIUM}}
- [ ] 81.1 Keep simulation state independent of animation timing.


### Area Mandatory Reading - Runtime integration and review

_Applies to Groups 82-89._

**Entry gate.** The following public contracts must exist before dependent implementation in this area: `media/multilayer/contracts/multilayer_sprite_manifest.json`: `asset_id`, `consumer`, `directions`, `frames`, `anchor`, `source`.
Verify each public file/export/schema field with `rg` in the current workspace and inspect its command/field ownership. For a same-plan contract, compile its type/signature fixture or validate its artifact after the preceding producer groups. For cross-plan behavior, also run the producer's focused executable boundary test; declarations alone cannot authorize dependent mutations or runtime claims.
If a required export is absent, stop the dependent implementation, mutation and runtime claims in Groups 82-89. Read-only characterization, source/range verification, test-case design and asset-template work that does not assume the missing behavior may continue.
Do not bypass this gate by copying private pool/tile state, inventing local IDs or cargo registries, using a historical plan as a delivered implementation, opening user-wide content stores, substituting a second utility/accounting authority, or adding an unreviewed fallback path. Finish the named producer or revise the shared contract and its direct consumers first.

**Scoped integration gates.** The following producer dependencies apply only to the named implementation/test families. Verify each required public export and its focused producer test before those integrations; absent late producers do not block already independent features.

- `src/excavation.h` is required for the M1 excavation/TBM/exchange integration and corresponding runtime captures; full-gallery/placeholder retirement awaits every required family in Groups 82-83, 85-89 (only the named integration/subscenario). If absent, stop only that integration/subscenario and its passing claim. Other independent code, art, persistence and tests in this area may continue.
- `src/layer_portal_cmd.h` is required for the M1 excavation/TBM/exchange integration and corresponding runtime captures; full-gallery/placeholder retirement awaits every required family in Groups 82-83, 85-89 (only the named integration/subscenario). If absent, stop only that integration/subscenario and its passing claim. Other independent code, art, persistence and tests in this area may continue.
- `src/tbm.h` is required for the M1 excavation/TBM/exchange integration and corresponding runtime captures; full-gallery/placeholder retirement awaits every required family in Groups 82-83, 85-89 (only the named integration/subscenario). If absent, stop only that integration/subscenario and its passing claim. Other independent code, art, persistence and tests in this area may continue.
- `src/tbm_exchange.h` is required for the M1 excavation/TBM/exchange integration and corresponding runtime captures; full-gallery/placeholder retirement awaits every required family in Groups 82-83, 85-89 (only the named integration/subscenario). If absent, stop only that integration/subscenario and its passing claim. Other independent code, art, persistence and tests in this area may continue.
- `src/multilayer_industry.h` is required for the mine/conventional-fleet integration and corresponding runtime captures; full-gallery/placeholder retirement awaits every required family in Groups 84-89 (only the named integration/subscenario). If absent, stop only that integration/subscenario and its passing claim. Other independent code, art, persistence and tests in this area may continue.
- `src/underground_gui.h` is required for the mine/conventional-fleet integration and corresponding runtime captures; full-gallery/placeholder retirement awaits every required family in Groups 84-89 (only the named integration/subscenario). If absent, stop only that integration/subscenario and its passing claim. Other independent code, art, persistence and tests in this area may continue.
- `src/space.h` is required for the orbital/solar integration and corresponding runtime captures; full-gallery/placeholder retirement awaits every required family in Groups 84-89 (only the named integration/subscenario). If absent, stop only that integration/subscenario and its passing claim. Other independent code, art, persistence and tests in this area may continue.
- `src/launch_center.h` is required for the orbital/solar integration and corresponding runtime captures; full-gallery/placeholder retirement awaits every required family in Groups 84-89 (only the named integration/subscenario). If absent, stop only that integration/subscenario and its passing claim. Other independent code, art, persistence and tests in this area may continue.
- `src/asteroid.h` is required for the orbital/solar integration and corresponding runtime captures; full-gallery/placeholder retirement awaits every required family in Groups 84-89 (only the named integration/subscenario). If absent, stop only that integration/subscenario and its passing claim. Other independent code, art, persistence and tests in this area may continue.
- `src/orbital_station.h` is required for the orbital/solar integration and corresponding runtime captures; full-gallery/placeholder retirement awaits every required family in Groups 84-89 (only the named integration/subscenario). If absent, stop only that integration/subscenario and its passing claim. Other independent code, art, persistence and tests in this area may continue.
- `src/solar_power.h` is required for the orbital/solar integration and corresponding runtime captures; full-gallery/placeholder retirement awaits every required family in Groups 84-89 (only the named integration/subscenario). If absent, stop only that integration/subscenario and its passing claim. Other independent code, art, persistence and tests in this area may continue.
- `src/transport_tube.h` is required for the tube/geothermal integration and corresponding runtime captures; full-gallery/placeholder retirement awaits every required family in Groups 84-89 (only the named integration/subscenario). If absent, stop only that integration/subscenario and its passing claim. Other independent code, art, persistence and tests in this area may continue.
- `src/geothermal.h` is required for the tube/geothermal integration and corresponding runtime captures; full-gallery/placeholder retirement awaits every required family in Groups 84-89 (only the named integration/subscenario). If absent, stop only that integration/subscenario and its passing claim. Other independent code, art, persistence and tests in this area may continue.

Do not emulate an absent producer with copied private state, fake inventories/energy, local IDs or a second registry/grid. A final combined closure/gallery/round-trip still waits for every required family; an independent passing case is not a pass for the missing integration.

| File name | Line Ranges | Description |
| --- | --- | --- |
| [src/map_layer_gui.h](../../../../src/map_layer_gui.h) | 67-73 | `GetVehicleDisplayLayer` is the public visible/pickable-layer contract; keep ordinary aircraft/ships/disasters on Surface and place orbital units explicitly. |
| [src/tests/map_layer.cpp](../../../../src/tests/map_layer.cpp) | 430-442 | Complete display-order test retains immutable Space=2 and Deep=3 storage meanings; future UI/profile work cannot reorder saved IDs. |
| [src/network/network_command.cpp](../../../../src/network/network_command.cpp) | 256-288 | `NetworkExecuteLocalCommandQueue` executes agreed frame order/company authority; all jobs use deterministic simulation time. |
| `media/multilayer/contracts/multilayer_sprite_manifest.json` (planned public contract; link after implementation) | Live definitions; no fixed range | [r2mlvs13-graphics-and-asset-pipeline.md](r2mlvs13-graphics-and-asset-pipeline.md) Groups 1-8, ending with Group 8's public-contract task, publish `asset_id`, `consumer`, `directions`, `frames`, `anchor`, `source`. These exports/schema fields are authoritative for this area. Same-plan consumers follow those earlier groups; cross-plan consumers verify the producer's required implementation/tests as well. Proposed names alone do not prove delivery. |
| `src/excavation.h` (planned public contract; link after implementation) | Live definitions; no fixed range | [r2mlvs2-geology-and-excavation.md](r2mlvs2-geology-and-excavation.md) Groups 1-7, ending with Group 7's public-contract task, publish `ExcavationState`, `GeologyDeposit`, `ExcavationJob`, `CheckExcavatedFootprint`, `CommitExcavationCut`, `GeothermalVentField`. These exports/schema fields are authoritative for this area. Same-plan consumers follow those earlier groups; cross-plan consumers verify the producer's required implementation/tests as well. Proposed names alone do not prove delivery. Read this producer for the M1 excavation/TBM/exchange integration and corresponding runtime captures; full-gallery/placeholder retirement awaits every required family in Groups 82-83, 85-89 (only the named integration/subscenario); missing it blocks only that integration, not independent work. |
| [src/layer_portal_cmd.h](../../../../src/layer_portal_cmd.h) | Live definitions; no fixed range | [r2mlvs3-sealed-portals-and-deep-rail.md](r2mlvs3-sealed-portals-and-deep-rail.md) Groups 1-7, ending with Group 7's public-contract task, publish `LayerPortalWorkState`, `GetLayerPortalWorkState`, `CheckLayerPortalWorkAccess`, `CommissionLayerPortalTBM`. These exports/schema fields are authoritative for this area. Same-plan consumers follow those earlier groups; cross-plan consumers verify the producer's required implementation/tests as well. Proposed names alone do not prove delivery. Read this producer for the M1 excavation/TBM/exchange integration and corresponding runtime captures; full-gallery/placeholder retirement awaits every required family in Groups 82-83, 85-89 (only the named integration/subscenario); missing it blocks only that integration, not independent work. |
| `src/tbm.h` (planned public contract; link after implementation) | Live definitions; no fixed range | [r2mlvs4-tunnel-boring-machines.md](r2mlvs4-tunnel-boring-machines.md) Groups 1-5, ending with Group 5's public-contract task, publish `TBMProject`, `TBMWorkState`, `ValidateTBMAreaRoute`, `AdvanceTBMWork`. These exports/schema fields are authoritative for this area. Same-plan consumers follow those earlier groups; cross-plan consumers verify the producer's required implementation/tests as well. Proposed names alone do not prove delivery. Read this producer for the M1 excavation/TBM/exchange integration and corresponding runtime captures; full-gallery/placeholder retirement awaits every required family in Groups 82-83, 85-89 (only the named integration/subscenario); missing it blocks only that integration, not independent work. |
| `src/tbm_exchange.h` (planned public contract; link after implementation) | Live definitions; no fixed range | [r2mlvs5-spoil-wagons-and-haulage.md](r2mlvs5-spoil-wagons-and-haulage.md) Groups 1-5, ending with Group 5's public-contract task, publish `TBMExchangeAssignment`, `ValidateTBMWagonExchange`, `ExecuteTBMWagonExchange`. These exports/schema fields are authoritative for this area. Same-plan consumers follow those earlier groups; cross-plan consumers verify the producer's required implementation/tests as well. Proposed names alone do not prove delivery. Read this producer for the M1 excavation/TBM/exchange integration and corresponding runtime captures; full-gallery/placeholder retirement awaits every required family in Groups 82-83, 85-89 (only the named integration/subscenario); missing it blocks only that integration, not independent work. |
| `src/multilayer_industry.h` (planned public contract; link after implementation) | Live definitions; no fixed range | [r2mlvs6-mines-and-resource-chains.md](r2mlvs6-mines-and-resource-chains.md) Groups 1-6, ending with Group 6's public-contract task, publish `LayerIndustryCapability`, `ValidateMineDepositClaim`, `ProduceLayerMineCargo`, `PrepareLayerRecipeBatch`, `CommitLayerRecipeBatch`. These exports/schema fields are authoritative for this area. Same-plan consumers follow those earlier groups; cross-plan consumers verify the producer's required implementation/tests as well. Proposed names alone do not prove delivery. Read this producer for the mine/conventional-fleet integration and corresponding runtime captures; full-gallery/placeholder retirement awaits every required family in Groups 84-89 (only the named integration/subscenario); missing it blocks only that integration, not independent work. |
| `src/underground_gui.h` (planned public contract; link after implementation) | Live definitions; no fixed range | [r2mlvs7-underground-construction-and-fleet.md](r2mlvs7-underground-construction-and-fleet.md) Groups 1-5, ending with Group 5's public-contract task, publish `ShowUndergroundConstructionToolbar`, `BuildUndergroundVehicleFilter`. These exports/schema fields are authoritative for this area. Same-plan consumers follow those earlier groups; cross-plan consumers verify the producer's required implementation/tests as well. Proposed names alone do not prove delivery. Read this producer for the mine/conventional-fleet integration and corresponding runtime captures; full-gallery/placeholder retirement awaits every required family in Groups 84-89 (only the named integration/subscenario); missing it blocks only that integration, not independent work. |
| `src/space.h` (planned public contract; link after implementation) | Live definitions; no fixed range | [r2mlvs8-space-world-and-construction.md](r2mlvs8-space-world-and-construction.md) Groups 1-7, ending with Group 7's public-contract task, publish `SpaceVehicle`, `OrbitalBodyID`, `SpaceMission`, `SpaceStructure`, `SpaceTransition`. These exports/schema fields are authoritative for this area. Same-plan consumers follow those earlier groups; cross-plan consumers verify the producer's required implementation/tests as well. Proposed names alone do not prove delivery. Read this producer for the orbital/solar integration and corresponding runtime captures; full-gallery/placeholder retirement awaits every required family in Groups 84-89 (only the named integration/subscenario); missing it blocks only that integration, not independent work. |
| `src/launch_center.h` (planned public contract; link after implementation) | Live definitions; no fixed range | [r2mlvs9-launch-centers-and-rockets.md](r2mlvs9-launch-centers-and-rockets.md) Groups 1-7, ending with Group 7's public-contract task, publish `LaunchCenterAnchor`, `LaunchCollectionHub`, `SurfaceHandlingJob`, `SpaceDescentJob`, `CanReleaseOrbitalCargo`, `CommitOrbitalCargoRelease`. These exports/schema fields are authoritative for this area. Same-plan consumers follow those earlier groups; cross-plan consumers verify the producer's required implementation/tests as well. Proposed names alone do not prove delivery. Read this producer for the orbital/solar integration and corresponding runtime captures; full-gallery/placeholder retirement awaits every required family in Groups 84-89 (only the named integration/subscenario); missing it blocks only that integration, not independent work. |
| `src/asteroid.h` (planned public contract; link after implementation) | Live definitions; no fixed range | [r2mlvs10-asteroids-and-mining-probes.md](r2mlvs10-asteroids-and-mining-probes.md) Groups 1-7, ending with Group 7's public-contract task, publish `AsteroidBody`, `AdvanceAsteroidField`, plus `SpaceVehicle`, `SpaceMission`, `AssignSpaceMiningMission` and `SpaceVehicle::Tick` in `src/space_vehicle.h`. These exports/schema fields are authoritative for this area. Same-plan consumers follow those earlier groups; cross-plan consumers verify the producer's required implementation/tests as well. Proposed names alone do not prove delivery. Read this producer for the orbital/solar integration and corresponding runtime captures; full-gallery/placeholder retirement awaits every required family in Groups 84-89 (only the named integration/subscenario); missing it blocks only that integration, not independent work. |
| `src/orbital_station.h` (planned public contract; link after implementation) | Live definitions; no fixed range | [r2mlvs11-orbital-stations-and-manufacturing.md](r2mlvs11-orbital-stations-and-manufacturing.md) Groups 1-6, ending with Group 6's public-contract task, publish `OrbitalModule`, `OrbitalConstructionJob`, `ReserveOrbitalConstructionInputs`, `CommitOrbitalConstructionStep`. These exports/schema fields are authoritative for this area. Same-plan consumers follow those earlier groups; cross-plan consumers verify the producer's required implementation/tests as well. Proposed names alone do not prove delivery. Read this producer for the orbital/solar integration and corresponding runtime captures; full-gallery/placeholder retirement awaits every required family in Groups 84-89 (only the named integration/subscenario); missing it blocks only that integration, not independent work. |
| `src/solar_power.h` (planned public contract; link after implementation) | Live definitions; no fixed range | [r2mlvs12-solar-farms-mirrors-and-eti.md](r2mlvs12-solar-farms-mirrors-and-eti.md) Groups 1-6, ending with Group 6's public-contract task, publish `SolarGenerationAccount`, `MirrorAssignment`, `ComputeMirrorSolarOutput`. These exports/schema fields are authoritative for this area. Same-plan consumers follow those earlier groups; cross-plan consumers verify the producer's required implementation/tests as well. Proposed names alone do not prove delivery. Read this producer for the orbital/solar integration and corresponding runtime captures; full-gallery/placeholder retirement awaits every required family in Groups 84-89 (only the named integration/subscenario); missing it blocks only that integration, not independent work. |
| `src/transport_tube.h` (planned public contract; link after implementation) | Live definitions; no fixed range | [r2mlvs17-underground-transport-tubes.md](r2mlvs17-underground-transport-tubes.md) Groups 1-8, ending with Group 8's public-contract task, publish `TubeVehicleCapability`, `TubeTerminal`, `CheckTubeConstruction`, `ComputeTubePowerDemand`. These exports/schema fields are authoritative for this area. Same-plan consumers follow those earlier groups; cross-plan consumers verify the producer's required implementation/tests as well. Proposed names alone do not prove delivery. Read this producer for the tube/geothermal integration and corresponding runtime captures; full-gallery/placeholder retirement awaits every required family in Groups 84-89 (only the named integration/subscenario); missing it blocks only that integration, not independent work. |
| `src/geothermal.h` (planned public contract; link after implementation) | Live definitions; no fixed range | [r2mlvs18-geothermal-energy-and-industry.md](r2mlvs18-geothermal-energy-and-industry.md) Groups 1-7, ending with Group 7's public-contract task, publish `GeothermalHeatAllocation`, `GeothermalUtilityLink`, `AllocateVentHeat`, `ExportGeothermalPower`. These exports/schema fields are authoritative for this area. Same-plan consumers follow those earlier groups; cross-plan consumers verify the producer's required implementation/tests as well. Proposed names alone do not prove delivery. Read this producer for the tube/geothermal integration and corresponding runtime captures; full-gallery/placeholder retirement awaits every required family in Groups 84-89 (only the named integration/subscenario); missing it blocks only that integration, not independent work. |

**ASSUMPTION: this was already built previously. If not, re-asses at runtime**. This applies only to the producer contracts named in this area's gate, after their listed producer groups have run. Verify their actual exports and focused tests before using the sketches below; proposed types and helper names in sketches are not evidence of delivered code.


### Local Mandatory Reading - Remaining runtime families and review

_Applies to Groups84-89._

Read this plan's Global/Runtime integration Area, the executing family's completed source/payload handoff in Plan13 or this continuation, and only its real feature state/rendering owners. Reuse selected native M1 evidence without reopening completed tasks. Require current loaded pixels, actual state transitions and family-specific captures for new runtime claims; defer missing producers and operator feedback explicitly.

### Group 84 - Runtime integration and review

Goal-authorized skip (2026-10-08, master finalization): dispatched expansion work in this group is skipped under [finalization exception GP-TG](portal-tbm/r2mlvs3mp-z-a-finalization-ledger.md#gp-tg-tube-and-geothermal-expansion-requires-major-gap-planning). Existing delivered prefixes stay complete; unimplemented profile work remains pending. No skipped task is markedX.

{{MEDIUM}}
- [ ] 84.1 Integrate all ten mines and the underground conventional fleet after their r2mlvs6/7 code is delivered.
- [ ] 84.2 Integrate orbital launch, probe, module and solar families after their corresponding r2mlvs8..12 producers are delivered.
- [ ] 84.3 Integrate tube/geothermal art after r2mlvs17/18 producers are delivered; this later work does not block M1 art.


### Group 85 - Runtime integration and review

{{SIMPLE}}
- [ ] 85.1 Capture native-scale and zoomed scenes across directions, terrain boundaries, load/production states, vehicle overlap and building transparency.


### Group 86 - Runtime integration and review

{{MEDIUM}}
- [ ] 86.1 Use real runtime frames for animation/occlusion claims.
- [ ] 86.2 Check each launch center/module footprint as a complete site, not isolated tiles.
- [ ] 86.3 Verify correct vehicle hiding at bays, portals and docks, drawing on the correct layer and clickable moving bodies.


### Group 87 - Runtime integration and review

Goal finalization disposition, 2026-10-08: selected M1 technical acceptance is complete in85B and freshly verified in [the final art check](../../../../logs/r2mlvs3mp-z-b-final-art-verification.log). The user explicitly permits skipping human operator gates; personal feedback remains **deferred**, never recorded as approval. Later-family whole-profile gallery/feedback tasks stay unchecked, with target tube/geothermal interop art under GP-TG. This exception applies only to goal finalization, not full-profile art acceptance.

{{SIMPLE}}
- [ ] 87.1 Produce a compact contact sheet and in-game scenario gallery linked to manifest IDs.
- [ ] 87.2 Record technical acceptance and user art feedback separately; do not inherit prior plans' skipped art approval.


### Group 88 - Runtime integration and review

{{MEDIUM}}
- [ ] 88.1 Replace all milestone placeholders, document retained limitations and confirm distributed assets match the reviewed hashes.


### Group 89 - Runtime integration and review

{{SIMPLE}}
- [ ] 89.1 Capture all five tube models in motion/load/service states, geothermal power/direct-smelting/battery/forge states at their valid dates, TBM area-clearing bends and off-center cargo drops with train/lorry collection. Include overlapping area overlays, map edges and blocked/locked states in the runtime gallery.
- [ ] 89.2 Record the implemented public exports, focused test commands/results and remaining runtime/art/network gaps in this plan; retain original supplied saves and repository-local evidence.
- [ ] 89.3 Update affected architecture/handoff documentation to describe delivered behavior and reconcile the parent index only when this plan's Completion Conditions are met. Keep every unfinished task unchecked.

## Consult on Demand

| Trigger | File name | Start at | Expand only when | Consultation purpose |
| --- | --- | --- | --- | --- |
| Reviewed sprites are absent or a fallback GRF ships old artwork | [../../architecture/basesets/archi-basesets.md](../../architecture/basesets/archi-basesets.md) | `Graphics` | The manifest, generated package/hash and live sprite IDs match but runtime resolution differs | Locate extra-GRF/base-set resolution and missing-sprite fallback; a contact sheet is not proof the payload was distributed. |
| A required public export is absent and its owner/location is disputed | [r2mlvs-multilayer-vehicles-and-structures.md](r2mlvs-multilayer-vehicles-and-structures.md) | `Requirements and ownership` and `Plan Index` | The named producer confirms an intentional contract/ownership change | Find the responsible producer and reconcile its public handoff with direct consumers; the hub cannot substitute for missing implementation. |
| Current behavior contradicts a historical subsystem description | [../../architecture/map-layers/archi-map-layers.md](../../architecture/map-layers/archi-map-layers.md) | `Invariants` and the relevant named source in `Code map` | The live symbol and executable test do not explain the difference | Locate layer addressing or lifecycle rationale. Live code/tests govern delivered behavior; this architecture page does not prove future interfaces exist. |

## Planning Provenance

This is not an execution reading queue. Do not preload it after context compaction. It is non-transitive; ranges record planning use, not runtime authority.

| File name | Sections or Line Ranges used while planning | Planning contribution |
| --- | --- | --- |
| [docs/zz-reach2/architecture/initial-inventory.md](../../architecture/initial-inventory.md) | 185-220 | `Command as the only world mutation`, `Pool-indexed game objects`, `Tile map as a packed array` and `File-suffix modules` oriented command/pool/header ownership and source registration. This historical inventory is a location survey, not authority for current layer behavior. |
| [docs/zz-reach2/architecture/@archi-ttd-level0.md](../../architecture/@archi-ttd-level0.md) | 24-63 | `How the program fits together` and `Source and runtime boundaries` located shared simulation, data/presentation, CMake targets and subsystem handoffs used to choose the live reading sources. |
| [docs/zz-reach2/upgrades/2026-10/r2mlvs-multilayer-vehicles-and-structures.md](r2mlvs-multilayer-vehicles-and-structures.md) | 119-140 | `Requirements and ownership` assigned this plan's feature responsibilities; it shaped the decomposition without making every sibling plan a mandatory reading dependency. |
| [src/table/sprites.h](../../../../src/table/sprites.h) | 380-409 | The inspected live boundary helped separate characterization, public-contract production, implementation and acceptance batches. It materially constrained manifest-owned sprite identity/geometry and final runtime art evidence; this entry records planning influence rather than an additional reading obligation. |
| [User planning-format instructions](<C:/Users/Axonn/.codex/attachments/cd59b20a-3a96-4d54-b4bd-1e8f37639e7d/Pasted text.txt>) | `Global Mandatory Reading`, `Area Mandatory Reading`, `Consult on Demand`, `Planning Provenance`, `Final validation` | Supplies the exact protocol, chronological difficulty batches of at most eight tasks, reading gates and validation format. |
| [media/multilayer/tools/sprite_quantize.py](../../../../media/multilayer/tools/sprite_quantize.py) | 379-411 | Deeper planning pass, 2026-10-04: Stable packing and metadata emission shaped per-model originals, coverage validation and reproducible conversion. |

## Completion Conditions

Every required building, vehicle, resource and gameplay-critical state has packaged graphics, valid directions/anchors and a runtime capture. Repeatable conversion works from retained sources. Offline sheets, generated originals and a successful compiler run alone do not establish art acceptance.
