# M1 asset baking continuation

Owner: [R2MLVS3MP task 8.3](r2mlvs3mp-master-plan.md); source owner: [Plan 13](r2mlvs13-graphics-and-asset-pipeline.md) Groups 14-18 and selected M1 authoring/conversion groups. Updated 2026-10-07.

The user requests reuse of existing graphics, permits tooling to cut/register irregular sheets and modest animation jitter, and defers personal art review until assets and further features are integrated. Continue authoring/baking the selected M1 scope without waiting for that feedback. Do not claim native gameplay acceptance from source sheets.

## Completed Standard source

Plan 13 Groups 14B/14C/14D and original 14.3 pass: **384/384** native poses, both physical parts, six states, four genuinely authored palette phases, eight genuine original headings. Seven new tool outputs supplement retained originals; shared status indicators and inverse rolling phases are reused. All originals remain immutable. The public source manifest is complete for Standard, packed/runtime pending.

The source converter now accepts reviewed explicit cell rectangles for uneven/touching generated gutters and several registered equipment regions on the own-part immutable chassis. This excludes generated body/coupler drift. It selects real authored pixels; no padding missing states with frame 0. Focused native checks verify every part/state/view has four distinct phases and backward phases match forward phases in inverse order. Corruption checks reject missing, overlapping and out-of-bounds cells. Evidence: [body](../../../../logs/r2mlvs13-j-standard-body-source-checks.log), [combined model](../../../../logs/r2mlvs13-k-standard-model-source-checks.log).

## Next scope and scoped reading

Plan 13 Group 15 now passes: **384/384 Deep poses**, preserving its selected old body and corrected genuine rear views. One new support original reuses Standard cart geometry with red/graphite reinforcement. Shared authored equipment pixels supply the remaining phases on the Deep model's own immutable body/support. Native frame-zero reference uses the retained initial atlas plus source-original hashes; transparent translation aligns old inconsistent ground contacts at y=40 without painting or changing the original atlas. Evidence: [Deep body](../../../../logs/r2mlvs13-l-deep-body-source-checks.log), [Deep support](../../../../logs/r2mlvs13-l-deep-support-source-checks.log).

Plan 13 Groups 16-18 now pass: **80 poses each** for Earth, Rock and the actual hauler. Original cargo/models and corrected actual source axes are retained, with state-specific chassis references keeping real load pixels intact. Shared authored steel phases supply frame one; the hauler adds two real cyan service cues and stays a plain secondary engine. `src/underground_vehicle.cpp` already owns actual native Singlehead diesel model 4. Evidence: [Earth](../../../../logs/r2mlvs13-m-earth-source-checks.log), [Rock](../../../../logs/r2mlvs13-o-rock-source-checks.log), [hauler](../../../../logs/r2mlvs13-p-hauler-source-checks.log).

Purchase previews reuse the five original available icons exactly; one new tool original supplies five grayscale/amber-padlock locked poses. Padlocks are purchase UI only and never vehicle bodies, as clarified to the user. [UI provenance](assets/mlvs/purchase-generation-record.json), [review](assets/mlvs/purchase-details-review.png), and [checks](../../../../logs/r2mlvs13-n-purchase-source-checks.log) retain all ten poses. Total closed vehicle slice: **1,008 rolling-stock/support poses + ten UI poses**. Master **8.3 is checked**; 8.4 is the next checkpoint.

Next: Plan 13 Group 23 (exchange), Group 26 (service), then Groups 37-38 (Earth/Rock receivers), in that order under master 8.4. Read master Global/current Group 8/nearest Local M1 asset slice, child Global/Source generation Area and the nearest facility/industry authoring Local before resuming after compaction. Read only executing family briefs/manifest/sources and actual consumer footprint/occlusion. Do not preload later utility work.

Future vehicle integration must use rolling wagon phases only during actual movement and frame zero while stopped; actual cargo quantity chooses empty/partial/full ahead of optional empty service/wait views. Cutter reversing reuses real rolling phases inversely with unchanged cutter heading. Animation is presentation only. Packed/native callbacks, actual train coupling/occlusion, sound and user feedback remain later scopes; source sheets are not runtime evidence.

The existing **14-source / 17-atlas / 247-pose** initial art pass remains immutable and reusable. Deep's former missing support/animation and remaining wagon/hauler source gaps are delivered. Original counts **40/384**, **200/384** and earlier held-static vehicle notes are stale; both cutters have **768 combined poses**, with 240 haulage poses additionally complete. Whole-profile count remains **143 families / 57,051** required sprites; this is not an M1 completion count. Nine new immutable tool outputs supplement retained artwork, not a wholesale model replacement.

Source-selection scratch scripts/JSON were staged beside this master under `r2mlvs3mp-o-` and cleaned after their exact registrations were retained in durable provenance. The reusable focused fixture is `src/tests/multilayer_vehicle_art.py --model vehicle.model-04 --part body`; use the executing model/part, not full regressions. Selected originals/provenance/reviews live in `assets/mlvs/`; native atlases in `media/multilayer/sprites/m1/`. New evidence logs stay in repository `logs/`. No Git mutation is authorized. No user-wide files are modified; the recorded imagegen cache exception remains confined to built-in outputs copied into this repository.

## Source gallery and reproducible checks

These native/enlarged GIFs are source inspections, with user feedback deferred. They do not capture the running game.

| Model | Pose count | Moving/load review | Additional review |
| --- | ---: | --- | --- |
| Standard cutter/support | 384 | [body moving](assets/mlvs/vehicle-model-00-body-moving-animation-review.gif), [support moving](assets/mlvs/vehicle-model-00-support-moving-animation-review.gif) | [body boring](assets/mlvs/vehicle-model-00-body-working-animation-review.gif), [support machinery](assets/mlvs/vehicle-model-00-support-working-animation-review.gif) |
| Deep cutter/support | 384 | [body moving](assets/mlvs/vehicle-model-01-body-moving-animation-review.gif), [support moving](assets/mlvs/vehicle-model-01-support-moving-animation-review.gif) | [body boring](assets/mlvs/vehicle-model-01-body-working-animation-review.gif), [support machinery](assets/mlvs/vehicle-model-01-support-working-animation-review.gif) |
| Earth hopper | 80 | [full](assets/mlvs/vehicle-model-02-body-full-animation-review.gif), [partial](assets/mlvs/vehicle-model-02-body-partial-animation-review.gif) | [empty](assets/mlvs/vehicle-model-02-body-empty-animation-review.gif) |
| Rock hopper | 80 | [full](assets/mlvs/vehicle-model-03-body-full-animation-review.gif), [partial](assets/mlvs/vehicle-model-03-body-partial-animation-review.gif) | [empty](assets/mlvs/vehicle-model-03-body-empty-animation-review.gif) |
| Actual spoil hauler | 80 | [moving](assets/mlvs/vehicle-model-04-body-moving-animation-review.gif) | [service](assets/mlvs/vehicle-model-04-body-service-animation-review.gif), [exchange/work](assets/mlvs/vehicle-model-04-body-working-animation-review.gif) |
| Purchase UI | 10 | [available/locked native sheet](assets/mlvs/purchase-details-review.png) | Padlocks are UI only. |

From the repository root, `python -B media/multilayer/convert_vehicle_art.py --check` and `python -B media/multilayer/convert_purchase_art.py --check` verify exact regeneration without writes. The [final source handoff burst](../../../../logs/r2mlvs13-q-vehicle-source-handoff.log) also verifies all original 14 sources/17 atlases/247 poses remain unchanged and the 143-family/57,051-sprite public schema passes. Source-only code changed, so no C++ rebuild or broad gameplay regression was needed. Future packing must preserve reviewed source hashes and public offsets; no packed hash or runtime activation is claimed here.
