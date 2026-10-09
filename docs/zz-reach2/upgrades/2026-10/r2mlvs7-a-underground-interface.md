# R2MLVS7 execution evidence

Owner: [Underground Construction and Fleet](r2mlvs7-underground-construction-and-fleet.md).
This ledger records completed behavior and the boundaries that remain pending.

**Artifact-layout migration (2026-10-09):** this plan consumes the parent [Development artifact layout](r2mlvs-multilayer-vehicles-and-structures.md#development-artifact-layout), implemented by [R2FC](r2fc-file-cleanup.md). It supersedes earlier docs asset/review/helper placement and fixed runtime-config destinations below. New native homes use `mlm/runtime/`, generated feature reviews use `mlm/reviews/`, logs remain under `logs/`, and reusable code/curated inputs keep their code/test owners. Paths and focused native/cold verification are complete; R2FC records the results and retained-input identities. Repository-relative executable commands run from the checkout root through their context-aware wrappers. Historical pass records remain dated evidence and are not new acceptance claims. No gameplay task is completed by this migration and the separate post-demo fixture-pruning gate remains.

## Retained evidence and diagnostic cleanup

The 2026-10-06 handoff retains final evidence ec/ed/ee, ft/fu, gk/gl/gm and
gn/go/gp/gq/gr/gs/gt/gu/gv. Together they cover excavation/conservation, freight,
all sixteen UI contexts, both editor continuations, affected controls, both
playable builds, matching language packs and the actual retained-save loader.
Ninety superseded build/refusal/debugging logs were removed after those passing
results. Earlier probe names below are historical diagnostics whose useful
findings are preserved in prose; they are not additional current acceptance.
The two native save fixtures remain intact. Cleanup covers this plan's logs;
other workspace changes retain their existing ownership.

## Reading and producer notes

Global anchors are AD-8, `MultiLayerMap` ownership and `Window::OnPlaceObject`.
For Groups 1-5 the authoritative area is Underground construction interface.
Line ranges have drifted; use the named exports. The mandatory entry exports
exist, with geology, portal/TBM work, exchange and industry retaining their
existing owners. No `transport_tube.h` or `geothermal.h` exists; their actual
tool/fleet integrations stay pending.

The executable entry burst used `[multilayer-content-contract],
[excavation-footprint],[excavation-cut],[portal-work-state],[portal-contract],
[tbm-area],[tbm-work],[tbm-exchange-commit],[layer-industry-admission],
[layer-recipe-batch],[layer-mine-production]`. It passed 40,880 assertions
across 31 cases in `r2mlvs7-b-producer-boundaries.log`. This is focused producer
evidence, not final construction/fleet acceptance.

## Group 1 - Underground construction interface

Surveys read the public/free geological planning
information; they cannot excavate, claim a seam or allocate energy. Projects
reuse the portal window. Exchange placement uses a real selected project and
the owning command. Conventional facilities reuse their ordinary construction
tools and whole-footprint excavation guards.

The Debug build passed in `r2mlvs7-e-underground-window-build.log`; focused
`[underground-window],[excavation-construction]` passed 80 assertions in three
cases (`r2mlvs7-f-underground-window-checks.log`). Real windows opened on both
rock planes, reused their layer identities, refreshed the canonical 1979/1980
boundary, and refused cross-plane survey clicks without geology/cash changes.
The first build exposed a missing Company include, then another concurrent
Debug build held an MSBuild tracking-file lock. Both were resolved before any
new binary was tested. Logs c/d are not passing evidence. This group did not
compact. Ordinary rail icons/pickers are substitutes, not final art acceptance.

## Group 2 - Shared entry paths

The configured airport shortcut opens the explicit rock-layer interface; the
editor uses its industry slot because its toolbar has no airport slot. Surface
retains its ordinary airport/industry entries. Both use the same existing
toolbar classes, including the narrow-screen split row. Scenario road tools
remain with their existing public editor toolbar; no company commissioning
or servicing bypass was added.

The final focused toolbar case passed 23 assertions in one case
(`r2mlvs7-m-minimum-toolbar-checks.log`, build l). Earlier fixture widths did
not force a split with the one-pixel mock icons; the final test explicitly
resizes the real toolbar before switching its row. Logs i/k contain failed
fixture assertions, not passing evidence. This group did not compact.

## Group 3 scope refinement

Separate placement context, whole-footprint quotes (3B) and canonical area
selection (3C). Their implementation owners differ, so new Local Mandatory
Reading blocks restrict each group's reading. Original task order is retained.

## Group 3 - Placement context

`SetViewportLayer` cancels unfinished GUI cursor/drag ownership before any band
remapping. This is shared by main and extra viewports and preserves posted
commands' explicit world tiles. The native toolbar raises its selected tool
and restores its selection prompt on cancellation. No simulation or queued
command is rewritten. The real viewport/cursor test passed 22 assertions in
one case (`r2mlvs7-o-placement-context-checks.log`, successful build n).
This group did not compact.

## Group 3B - Whole-footprint and canonical material quotes

The existing construction guard attaches local-only translated layer and
plane-coordinate details for its first blocked cell. Permission, cost, tile
and reserve behavior remain with the same canonical guard. Both rock layers'
actual station command estimates preserve all state, include both approaches,
and price every platform cell once after excavation.

The TBM owner publishes `src/tbm_area_preview.h` / `PreviewTBMCommissionArea`.
The exact command reuses that quote: canonical requested floor, charged turn
space, route/pass terms and future Earth/Rock/work/support totals stay with
`PreviewTBMArea`, while the acquisition owner quotes machinery/wagons. The
explicit company must still match the current command actor; a UI cannot
preview on behalf of a foreign actor to acquire permissions.

Focused quote and affected commissioning cases passed 704 assertions in six
cases (`r2mlvs7-q-footprint-quote-checks.log`, build p). Selected-area display
is separated into 3.2B in Group 3C, where its owning UI is implemented. This
group did not compact.

## Group 3C - Canonical area selection

Real rock-side portal selection and floor dragging consume the tested native
area quote. Purchase and future geological work charges are displayed separately
alongside both spoil quantities, support and synchronized work. The renderer
consumes only cursor-owned derived covered floor, actual extra sweep and
uncovered cells. Replacement/cancellation clears every role together.

Shared portal UI helpers select real compatible catalogue cutters and actual
owned Surface disposal origins; their public names moved to
`GetLayerPortalDefaultTBMModel` / `GetLayerPortalDefaultTBMOrigin` in
`map_layer_gui.h`, preserving the old controls' sole selection implementation.
No duplicate catalogue, route planner, tile authority or work participant exists.

Initial real area/placement/portal checks passed 112 assertions in three cases
(s, build r). The final straight/chamber area cases passed 253 assertions in two
cases (u, build t), including actual canonical 45-degree advances, exact overlay
roles and unchanged native digest. A cached preview never authorizes purchase;
confirmation posts the owning command, which revalidates it. Final raster-art
acceptance remains pending. This group did not compact. Actual viewport command
acceptance is narrowed into Group 3D before later integrations.

## Group 3D - Actual viewport command acceptance

The test picks world tiles using native scroll/projection and the ordinary
mouse-event dispatcher, then submits the actual area confirmation command.
Both main and extra viewport paths purchase the same Underground job; the extra
case keeps Surface in the main viewport. Tile-centred navigation to Deep before
confirmation cancels cursor/quote state and preserves the complete native digest.
Stale release/confirmation events cannot publish a project.

Direct tile centring and vehicle following bypassed `SetViewportLayer`; both
now cancel unfinished placement before changing their displayed world band.
Initialization also incorrectly calculated virtual dimensions using requested
zoom after clamping the actual zoom. It now uses the clamped value, preserving
native picking/scroll geometry before a resize.

The final focused case passed 210 assertions in three scenarios (af, build ae).
Earlier x/z crashed because a camera update invoked raster repaint before the
test's sprite-sorter/video-driver startup; the fixture now publishes the native
scroll projection for picking only. The native exception diagnostic walks the
saved fault context, which identified that missing test startup. Logs ab/ad
revealed the real clamped-zoom geometry mismatch and are not passing evidence.
This group did not compact. No full regression or final raster-art claim was made.

## Group 4 - Calendar controls and scoped missing integrations

No `transport_tube.h` or `geothermal.h` exists. Only dependent actual tools and
later factory/utility integrations remain pending. Native cutter names/years
come directly from `GetMultiLayerVehicleCapabilities`; Deep access is read
from the canonical profile. The same toolbar is reached through normal/split
and shortcut/editor entry paths, and native area commands still check dates.
Focused relevant date/entry/quote checks passed 132 assertions in four cases
(ah, build ag). This group did not compact.

## Group 5 - Public UI and logical fleet handoff

`underground_gui.h` publishes explicit rock-layer/profile toolbar entry and
the exact layer/profile/calendar/kind fleet filter. Conventional rail/road
depot designs remain separate from tube-domain designs; portal-only cutters
stay in their project interface. Invalid domains/profiles/layers and earlier
Deep dates return no rows. Results point into the immutable catalogue, never
copy or allocate model/engine identities, and never confer purchase permission.

The catalogue producer check passed 181 assertions (ai); direct signature,
filter and real-window cases passed 106 assertions in three cases (al, build ak).
The initial filter fixture used an iterator/pointer comparison rejected by
MSVC; its corrected immutable-address check passed before completion. This
group did not compact. Tube/geothermal command bindings remain pending.

## Group 6 - Conventional facilities

Both rock planes build ordinary rail loading platforms, freight road terminals
and depots only after their complete floor and approaches are excavated. The
native commands preserve normal StationIDs/orders, same-layer catchment,
company joining permission and station spread. Actual finite mine packets are
queued through the existing dispatcher in all four joined/separate scenarios.
Carrier loading is split into 6.3B and waits for the actual engines in Group 7B.

The focused command and TBM service/purchase producer burst passed 940
assertions in seven cases (ax, build aw). Earlier failures came from fixture
pause, skipped station-index teardown, unchecked pool expansion and the
disabled distant-join setting. Ordinary station deletion now retires packet,
catchment and spatial references before fixture owners disappear. No game
station rules were relaxed. This group compacted once.

## Group 7 - Actual conventional definitions

Five native freight designs append into the existing pinned-package EIDS range,
with models 2/3 retaining the delivered hopper mappings. Definitions consume
canonical catalogue names, dates and exact refit labels from the shared cargo
registry. The plan records all ten mine resources and both spoil transports.
Tube/orbital IDs gain no ordinary engine definitions. Conventional engines use
ordinary costs/service/vehicle owners; final matching graphics remain pending.

Focused reconstruction and transport coverage passed 230 assertions in two
cases (bb, build ba). The original stock engine and four TBM definitions remain
unchanged, repeated reconstruction retains identities, and missing content
refuses. Group 7B next applies the shared profile/date/layer/cargo predicate to
all command and UI consumers. This group did not compact.

## Group 7B - Actual commands, UI and loading

One shared native capability predicate now serves engine availability, depot
buy lists, refit lists and actual buy/refit/NoTest clone/whole-chain renewal
commands. Refit keeps ordinary mixed-consist selection/skip behavior while
refusing unsupported native-only cargo. Native startup uses the published date
instead of random delays; original engines keep their startup behavior.

Actual native trucks service, group/list, clone and renew normally on both rock
layers; actual mineral wagon replacement refuses incompatible cargo before
temporary allocation and replaces a compatible sealed load. A native locomotive
with real hopper units is assigned through the TBM exchange owner, and the
ordinary vehicle caption shows the live project identity and actual layer.
The real Deep purchase window and command agree before/after 1980.

Four joined/separate terminals load finite mine packets into actual purchased
native trucks through normal station orders and LoadUnloadStation (6.3B).
The fixture's arrival positioning is not driven route/Surface delivery evidence.
The final fleet/loading/UI burst passed 568 assertions in six cases (bi, bh);
startup and affected fleet/purchase checks passed 355 assertions in six cases
(bm, bl). Initial bd expected unchanged clone cash, bg omitted the GUI's normal
road stop-position flag, and bk lacked roadtype startup in a rail-only fixture;
these failures were corrected and retained as diagnostics, not passing evidence.
This group did not compact.

## Group 7C - Actual physical admission

Command-purchased native trucks travel through both timed Deep elevator pairs.
The fixture uses ordinary portal construction, both owned road approaches and
the explicit connection command; building a housing alone correctly leaves it
Sealed. Actual front/follower pool references cover articulated admission only,
without claiming an articulated native purchase model. Trams and earlier Deep
dates refuse. Real depot acquisition and paid TBM support retain ordinary
railtype power; temporary appearance grants no electrical compatibility.

Final admission and translated quote checks passed 4,706 assertions in five
cases (ca, bz); standalone portal producer cases passed 147 assertions (bx, bw).
Earlier bo/bq/bs omitted Surface/rock fixture separation, connection, portal
teardown and follower heading. The bv combined producer run stopped in a
modal error because no language was loaded for command cell formatting; its
task-owned process was inspected and terminated. The guard now formats local
cell details only with a loaded language, preserving stable headless refusal.
No full regression was run. This group did not compact.

## Groups 8-9 - Scoped late providers and independent airport guard

`transport_tube.h` / `geothermal.h` remain absent. The graphics manifest is M0;
required native vehicle/station/terminal/depot source rows remain planned. A
reserved control offset and contract-template fixture do not deliver final art.
Tube controls, final occlusion/gallery and full visible Surface-delivery
acceptance stay unchecked. Existing native command/event/loading/physical
checks remain independent functional evidence.

The ordinary airport command's Surface-only domain passed 64 assertions in
one focused case (cc, cb), under both native and Classic policies, with unchanged
allocation/geology/cash/RNG. These groups did not compact.

## Group 10 - Command and station isolation

Actual malformed station/purchase/refit/clone requests refuse without partial
allocation. Three identical-coordinate stations have separate IDs and exact
same-layer catchment. The actual cargo purchase dropdown offers Coal but no
Water Ice, matching the native command's exact-label restriction.

The deliberately adjacent raw storage-band join reproduced a real catchment
assertion (cf). Station spread now explicitly rejects different physical layers
before joining mutation; raw Y proximity cannot merge their cargo/orders. Final
boundary and affected facility checks passed 447 assertions in five cases (ch,
cg). No ordinary same-layer rule was relaxed. This group did not compact.

## Group 10B - Actual native fleet persistence

The save fixture initializes dense original engine mappings before appending
the native family and uses the distributed cargo-slot layout. A command-bought
truck drives to its actual mine terminal and loads real finite packets. Actual
Surface disposal receivers, a commissioned project and a native locomotive
with bought/coupled hoppers supply the controller-owned association. Both
ordinary vehicles have real station/service orders.

The full writer case passed 378 assertions (ck, cj). The actual game-loader
round trip passed in eight seconds (cm; detailed cn; playable Debug build cl).
Exact EIDS, VEHS, ORDL and native project/exchange transport matches. Read-only
console diagnostics resolve seven real engine definitions, loaded cargo,
orders, association and both native filters. Source save and startup bytes
remain immutable/restored. No capsule persistence is claimed. This group did
not compact.

## Groups 11-12 - Native calendar checks and delivered handoff

Native calendar, actual toolbar and canonical chamber-preview cases passed
301 assertions in four cases in the final focused burst (cp, Debug build co).
Actual tube introduction controls, geothermal timelines and the five-vehicle
final gallery remain gated; no native ordinary engine registration stands in
for capsule delivery.

The plan's public handoff now lists actual UI/filter, native engine/capability,
canonical area quote, derived cursor preview and shared portal-default exports,
plus the read-only loaded-fleet diagnostic. Focused commands/results and retained
input/resave locations are recorded in Group 12. Architecture pages for vehicles,
map layers and viewport placement describe the delivered owners and behavior.
Final graphics, the full visible-controls mine-to-Surface workflow, combined
multiplayer/network and large-map acceptance remain unclaimed. Completion
Conditions are unmet, so the parent plan's completion index stays open.

The final Debug GUI/test build succeeded (co), followed by the playable
RelWithDebInfo build (cq). Both configurations' 66 packs match generated ABI
`0x748A1284`. The actual optimized loader smoke passed in two seconds (cr,
detailed cs), resolving native mappings/filters, loaded mine cargo/orders and
the active association. It restored the startup bytes, preserved the source
hash and wrote no new save. `play.ps1`'s default configuration is rebuilt.
Group 12 compacted twice, including the final handoff and evidence cleanup;
Group 11 did not compact.

## Resumed Group 9B - Connected excavation and exchange

Final art is a scoped visual gate, not a reason to stop functional workflow
9.3/9.4. The plan now splits connected excavation/exchange (9B), continued
mine freight in that same world (9C), and viewport/toolbar/editor repetition
(9D), each with focused local reading. Groups 9B/9C/9D are now complete; the
evidence below records each independent native boundary.

New actual scenario/event helpers are `underground_workflow_fixture.h`,
`underground_view_fixture.h` and `underground_workflow.cpp`. The existing
portal widget enum moved unchanged to `widgets/layer_portal_widget.h`; the
existing tested viewport picker is shared rather than duplicated. The fixture
starts from generated solid rock, a real company/accepted definitions and
funded Surface disposal logistics. After visible workflow entry it never
stages phases, teleports vehicles, opens rock or fabricates cargo.

The large chamber exposed real missing cutter routing: YAPF had no project
destination, and its segment target detection skipped plain paid support.
`GetTBMCutterSteeringTarget`, native destination recognition and the existing
cached vehicle destination now derive from the live project/job, with shared
segment caching disabled for that search. The diagnostic sharp-turn-enabled
run completed 82 assertions (dy, build dx); it is not default-policy acceptance.

Default-policy ea reproduced a second problem at cut 6: the quote accepted
a support bend forbidden by the ordinary rail rule. `CheckTBMSupportRailTurns`
applies that exact rule before purchase in the shared quote/command owner;
a readable refusal directs the player to a straight corridor or sharp-turn
settings. Final build ec and focused ed passed 240 assertions in two cases:
normal-rule straight and enabled wide excavation/haul, plus atomic UI/command
refusal. Actual receiver processing/stock equals all committed spoil and actual
vehicle packets are empty. Affected real cutter/viewport/area controls passed
34,026 assertions in eight cases (ee). Group 9.3A is complete; 9C next continues
that world with mine freight, rather than treating excavation as its delivery.
First receiver setup overlapped the full Rock layout with the
portal (da); positioning uses actual layout width. The Surface line also leaves
room for the whole consist. Native daily scheduling slots and tick counter are
advanced/restored by the fixture. Earlier failed probes remain diagnostics.

## Resumed Groups 9C/9D - Visible freight and complete context matrix

The same physically excavated world continues through visible machine retirement,
support conversion/demolition, native mine funding, road terminal/depot/portal
construction, purchase, two real refits, station orders and start. Actual landscape
construction, industry production/dispatch and native vehicle ticks deliver mineral
packets to the Surface receiver and empty the carrier. Reserve decrease equals
extracted output; remaining mine/station/vehicle stocks and real receiver intake
history reconcile exactly. The ordinary Coal receiver consumes intake immediately,
so waiting stock alone cannot account for delivery; useful-power production remains
with its undelivered utility producer. Excavation/refusal/freight passed 483 assertions
in three cases (fu, build ft).

The connected route exposed a missing road-stub completion callback. Actual portal
construction now posts the ordinary paid road-bit command beside each existing
approach, matching other road structures; it never opens a sealed connection or
builds a road on empty rock. Native connection still requires both actual approaches.
The headless initial scenario starts with dry Surface land above sea level so real
landscape ticks cannot flood test logistics. Checked-STL assertions now record their
native stack before MSVC fail-fast, retaining normal failure behavior.

All sixteen rock-layer/main-extra/full-split/menu-keyboard combinations independently
complete digging, spoil disposal and mineral Surface delivery (4,520 assertions, gm).
Deep uses a generated Rare seam, the actual sealed carrier and Electronics works;
Underground uses Coal and the bulk truck. Order destinations use the real viewport
picker: select the plane before arming Go To, respect its automatic active state,
and move overlapping dialogs beside the world. The main camera stays Surface during
extra-view placement. Split controls use the native narrow-toolbar arrangement under
the one-pixel mock icon geometry; this is functional coverage, not final-art approval.

Deep's first twelve-cell Hard cut yields 480 tonnes of Rock. Two native 128-tonne
hoppers could not hold it, so shared `GetDefaultTBMConsist(MapLayer)` now supplies
two Earth/four Rock hoppers for Deep, preserving Underground's two/two. Both actual
commissioning interfaces use the same native defaults and revalidate purchase,
physical length and capacity. Existing Deep scenario logistics buy the matching
empty units and have actual upper assembly floor and a four-tile Surface platform;
the target mine floor is still solid until the visible machine excavates it.

Both editor cases pass company claim/fleet restrictions, pure survey and visible
ordinary road authoring on the actual dug floor, then normal-game mineral freight
in that same world (621 assertions, gl). Editor mine purchase is deliberately
disabled and the actual claim command refuses its non-company actor. Keeping the
saved authoring flag does not permit normal-game construction on remaining solid
rock. Groups 9.3/9.4 and their subgroups are complete. Group 9C compacted once;
Group 9D did not compact. Final art, tube/geothermal integration and combined
multiplayer/large-map acceptance remain unclaimed.

The final focused affected-control burst passed 63,037 assertions in six cases
(gp, builds gn/go). The final test-world teardown restores derived funding order
and rebinds the standard cargo span to restored vector storage; connected freight
and both editor continuations pass after that isolation check (gs, build gr).
Playable Debug and RelWithDebInfo builds succeeded (gn/gq). Each configuration's
66 language packs match generated ABI `0xFCDD824B` (gt). The optimized actual
loader smoke passed in two seconds (gu, detailed gv), resolving all seven native
models, both filters, real mine cargo/orders and the active hauler association.
It restored startup bytes, preserved the retained source hash and wrote no new
save. The default `play.ps1` configuration is rebuilt.
