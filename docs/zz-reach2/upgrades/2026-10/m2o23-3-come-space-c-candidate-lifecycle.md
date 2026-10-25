# Candidate acceptance and reset ownership

Related contract: [native save architecture](../../architecture/savegame/archi-savegame.md)
and [merge Plan3](m2o23-3-come-space.md), Groups45-47. This records existing native
owners and joins; it is not another candidate store or rollback protocol.

## Dependency order

| Candidate / actual owner | Required same-file dependencies before rights are usable | Acceptance boundary |
| --- | --- | --- |
| Scoped original lineage and native metadata | Whole original identity/framing; exact required family/column set; profile/content and candidate geometry | Detached preflight before InitializeGame; accepted context remains bound through afterload; activated content admits metadata before dependent owners. |
| Native map, company, station, industry, engine, vehicle and CAPA pools | Original scalar schema and native pointer fixups; CDIM and logical cargo meanings when present | Existing native loaders/fixups establish original IDs; candidate geometry uses DecodeStoredTile, never the open world's dimensions. |
| Geology, portal work, TBM projects/exchange | Saved geological bounds, actual native trains, commissioned portal pair and assignment identities | Geology/portal roots reconstruct before native projects; projects before exchange; no camera-dependent ownership. |
| Supply SSOR and original B SSOH | Independently read DATE/MAPS/profile/company/station rows, exact receipt-era evidence, raw CAPA purpose/link/departure and complete ordinary/Space custody | Sole SurfaceSupplyOrderSnapshot stays unpublished until AcceptLoadedSurfaceSupplyOrders; no load debit, refund, quota restoration, packet or dispatch. |
| Space fleet, orders, manufacturing, manifests, pods and transitions | Original native engines/vehicles/company IDs, sites/leases, centers/hubs, cargo packets, timed handling and supported original content revision | Existing Space afterload joins owners in their established order, retaining A's reservation/replacement/heavy/contained-service facts. |
| Space structures, descents and processing | Actual hull/site/module IDs, configured cargo dimensions and packet custody; utility identities for recipes and contained jobs | Dimensions before descent mass; structures before processing; utility before recipe/services validation. |
| Utility UPLH/UPLD and UCVH/UCVD | Exact original epoch/issuers, companies/layers, raw meter/link/share/path rows and conversion numerator/provenance | UCV extends the same unpublished utility candidate. Initial adoption defers only the reverse active feed-to-native-plant reference, then ordinary full validation runs after plant/ports accept. |
| Cold site claims and collectors | Original geology/vent, company/date, consumed blueprint, actual industry and complete native delivered stock/work receipt | Claims before construction; no absent family can manufacture free machinery or original consumed materials. |
| Risers and generators GPGH/GPGD | Actual housing/industry/company, consumed site, exact construction/service receipt, utility input/output IDs and native stock | Risers before electrical links; generator construction after utility and sites; physical ports after heat-route adoption. Unfinished generator history remains distinct from interval participation. |
| Physical heat routes | Saved shared source slices, installed native utility paths and original collector/riser/plant boundaries | Complete physical validation after construction owners; no duplicated energy stock. |
| External Tube inlets | Original commissioned portal, native owned depot/platform/track/corridor, exact utility producer/consumer and invoice fraction | AcceptLoadedTubePowerConnections after native geometry and utility. |
| Geothermal TGSH/TGSD inlets | Original plant/output/consumed site, physical Deep port, forwarding producer/supply and native depot/station incarnation | Full utility and generator physical validation precede source acceptance; retired source history stays inert. |
| Capsule TUVD provider bindings | Actual front vehicle/engine/company and electrical consumer, original external or geothermal inlet, local-feed reference, demand carry and once-only tick | AcceptLoadedTubePowerVehicles last among power families, after both providers. A coincident reused native pool slot cannot replace an original reference. |

All of these validations finish before returning a usable loaded world or running
any simulation tick. Sequential native afterload may temporarily install an owner
needed by the next join; later failure follows the existing ReInit path, rather
than promising rollback to the previous world.

## Reset boundaries

- Ordinary world initialization calls ResetMultiLayerStateSchema and existing
  runtime resets. They discard the previous book and pending transports without
  refunding its money, recreating packets or carrying utility grants into a new map.
- Each copied attempt begins with LoadCheckData::Clear. Its family rows, flags,
  companies, geometry, dates and utility/provider transports are separate from the
  live world. Every refusal now discards accumulated copied rows while retaining
  its localized error and useful parser detail. Unreadable files also clear a
  preceding successful preview; no stale visible book/binding remains.
- Original lineage is scoped to one attempt and revoked before native Null
  cleanup. On full-load failure, SSOR revokes the canonical book and unpublished
  supply facts; utility revokes its state/UCV/path staging; generator, physical
  inlet, geothermal source and capsule handlers clear their own partly adopted
  rights and pending transports. Ordinary native ReInit still owns pool recovery.
- Successful copied previews retain their result until the UI clears it or starts
  another attempt. They never stage into runtime owners. Successful full adoption
  consumes each pending owner; its original scalar handlers reset at the next
  ordinary world initialization.

## Focused proof complete

Two focused cases/25192assertions pass on2026-10-24.

`[save-candidate-lifecycle]` passes10188assertions and alternates genuine A/B originals, refuses a raw future
plant model after copied transport accumulates, checks unreadable-file clearing
and then valid recovery. Its snapshot reads actual packet fragments, tagged book,
real cash/fractions/debt/expense arrays, RNG, tick, main-camera state and existing
native domain diagnostics. It neither restores state nor generates expected data.

`machinery_lineage_native_checks.py --binding-phase 0 --roundtrip --lifecycle`
passes15004assertions and additionally alternates full A440/B434/441loads, performs nine previews while each
actual world remains installed (with three further same-file pre-load previews), then refuses a missing active source-to-plant
reference after native numerical/supply adoption and explicitly reloads the valid
original. Native camera, cargo, accounts and both RNG streams are exact during
previews; after full recovery original simulation RNG, native stock/accounts and
existing domain digests must match. No interactive-RNG/camera rollback is promised
for world replacement. New outputs and reports are rebuildable in
`test-cache/merge-supply/group45-*`; native homes/process receipts keep their
existing mlm/runtime/reviews owners. Original saves remain byte-identical.

The isolated copied failure selects nonempty original B432GPGD; the catalogue B434
is genuinely empty and cannot supply a nonempty plant mutation. Native phase0uses
its separately retained genuine B434two-provider original. The headless copied
fixture has no main window; FindWindowById permits its absent view without calling
GetMainWindow's assertion. The native observer requires a real nonempty camera.
The final copied build changes only that isolated input selection; runtime and
native observer code match the preceding native build. Both exact executable
identities and this qualification are recorded in
`test-cache/merge-supply/group45-focused-verification.json`. Original43refusal
cases were spot-checked in the same focused burst; no full regression was run.
