# Orbital chain entry audit

Owner: [Space master plan](../../../zz-reach2/upgrades/2026-10/r2mlsp-space-master-plan.md), [Plan11 Group15](../../../zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md#group-15---presentation-and-acceptance).

Group14 is complete: [joined save verification](../../../../logs/orbital-save-boundaries-verification.json)
binds94accepted current processes and both configurations. Its declared codec
inventories and inherited starting-content limits remain explicit. Earlier
[save handoff](r2mlsp-z-z-z-z-a-orbital-save-boundaries.md) has exact receipts,
schema normalization and completed14.1/.2/.3 details. All processes are terminal;
no C++/production change, rebuild, subagent, Git write or full regression occurred.

The direct [entry audit](../../../../logs/orbital-chain-entry-audit.json) completes
15B.1, not original15.1/.2. Read only these current source boundaries:

- `space_logistics.cpp` preparation explicitly appends declared Surface packets.
- `launch_production.cpp` preparation appends100Steel and100Goods to the launch center.
- `manufacturing_reservation_native.cpp` declares missing initial Surface stock.
- `PurchaseSurfaceStartingContent` in `manufacturing_purchase_native.cpp` is a
  fixture packet append, despite its historical name. It is not a paid command.
- `ProcessorRig`/`SurfaceManufacturingRig` fund isolated facilities but force their
  construction complete; they do not supply an unassisted gameplay predecessor.
- `GetMultiLayerSurfaceSupplies` publishes real canonical prices, dates and daily
  per-company capacities, with an explicit definition-only comment. Current
  references supply economic/definition contracts; no runtime purchase command
  implements that external supply route.

These findings do not invalidate paid transport/construction/manufacture after
their declared inputs. They prevent promoting that limited evidence into a
continuous unassisted chain. No historical source/binary graph was reopened.

An asynchronous question asks whether Group15 must show physical Surface factory
production from raw materials, as written, or may use canonical paid external
Surface supplies. Neither route is currently accepted. Do not treat no response
as permission to relax original15.1/.2. Physical factory production remains the
written requirement unless the user chooses otherwise. A real paid supplier would
still need its actual producer/command, caps, native custody and persistence; the
existing pricing table or a fixture charge cannot stand in for it.

Next15B.2 must produce the selected legitimate Surface/launch entry.15B.3 verifies
its actual quantities, cash/capital, limits and packet owners in both builds before
the continuous Core/Dock/Assembly -> heavy-miner21 -> finite asteroid -> processor
-> useful output acceptance. Retained16 is not the mining role and cannot replace21.
No new acceptance code, runtime command or Surface world has been authored yet.

After compaction read Plan11 Global, relevant AD-12/map-layer/packet conservation,
Presentation Area, the nearest **Genuine Surface and launch entry prerequisite
Local** above15B, original15.1/.2, and this audit. The new bounded15B tasks keep
the missing entry producer separate from the later paid chain. Do not reread raster
histories, prior full proof graphs or unrelated global sources.

Next auxiliary prefix: `r2mlsp-z-z-z-z-c-` in this tracked, VSCode-excluded archive.
