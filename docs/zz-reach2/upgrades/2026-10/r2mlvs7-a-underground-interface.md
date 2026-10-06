# R2MLVS7 execution evidence

Owner: [Underground Construction and Fleet](r2mlvs7-underground-construction-and-fleet.md).
This ledger records completed behavior and the boundaries that remain pending.

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

Implementation in progress. Surveys read the public/free geological planning
information; they cannot excavate, claim a seam or allocate energy. Projects
reuse the portal window. Exchange placement uses a real selected project and
the owning command. Conventional facilities reuse their ordinary construction
tools and whole-footprint excavation guards.
