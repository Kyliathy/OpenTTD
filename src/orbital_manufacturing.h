/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file orbital_manufacturing.h Paid reservations, common-grant work and once-only native hull materialization.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Groups7/8/8BC
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group13K; read-only original manufacturing admission/receipt disclosure.
 */
#ifndef ORBITAL_MANUFACTURING_H
#define ORBITAL_MANUFACTURING_H
#include "orbital_station.h"

struct SpaceStructureRecord; ///< Existing native snapshot owner, defined in space_structure.h.

/** Transient original factory admission result; never saved work or permission to commit. */
enum class OrbitalManufactureWait : uint8_t { None, Inactive, Materials, Power, Output };

/** Query an existing request through the original facility/input/output preflight.
 * @param site Actual Assembly; the current actor must own any inspected pending job.
 * @return Actual limiting family, or None for idle/completed/admitted work; no owner is changed.
 */
OrbitalManufactureWait GetOrbitalManufactureWait(const SpaceStructure &site);

/** Read only this actual job's current paid-work receipt for its authored display.
 * @param site Original owned Assembly and its sole native manufacturing job.
 * @return Actual positive saved work when this normal unpaused epoch earned it, otherwise absence.
 */
std::optional<uint32_t> GetOrbitalManufactureVisualWork(const SpaceStructure &site);

/** Admit replacement through the original complete physical manufacturing gate, optionally superseding this exact old product's empty completed receipt.
 * @param flags Native estimate/execution; QueryCost remains pure.
 * @param factory Actual owned Assembly, whose existing producer retains material/work/serial authority.
 * @param inventory Actual adjacent input inventory at the old hull's precise handover berth.
 * @param model Existing canonical target recipe role.
 * @param replacement Exact original incarnation and pending retirement context; it grants no temporary hull or early sale proceeds.
 * @return Full new capital cost, or unchanged refusal; execution publishes one actual contained request.
 */
CommandCost QueueOrbitalReplacement(DoCommandFlags flags, SpaceStructure &factory, SpaceStructure &inventory, uint16_t model, const SpaceReplacementIntent &replacement);

/** Append demand only for an actual paid queued reservation or unfinished work batch with complete physical material/berth admission.
 * Idle facilities, completed receipts and jobs awaiting materialization request no speculative electricity.
 * @param requests Existing common allocator input; this function allocates no energy, packet, work or hull.
 */
void AppendOrbitalManufacturingRequests(std::vector<UtilityPowerRequest> &requests);
/** Reserve a queued job's complete actual packet batch only after this normal tick's real common grant.
 * Repeated callbacks, pauses and loading perform no reservation. This boundary consumes no energy or material and creates no hull.
 */
void TickOrbitalManufacturingReservations();
/** Advance real grant-paid work and atomically materialize one hull after complete pool/engine/unit/berth/input admission.
 * Completed work and original inputs wait unchanged on exhausted admission; paused/load/repeated callbacks cannot duplicate work or output.
 */
void TickOrbitalManufacturingWork();
/** Validate original factory/job issuance and once-only completed hull links within the same snapshot.
 * @param rows Actual or copied native factories, including retained completed jobs.
 * @param products Same-snapshot native hull commissioning receipts.
 * @param next_structure Same-snapshot next issued structure generation, including retired factories.
 * @param next_vehicle Same-snapshot next issued hull generation, including removed products.
 * @return Whether provenance, completion receipts and unique job-to-hull ownership agree without replaying manufacture.
 */
bool ValidateOrbitalManufacturingProductionLinks(std::span<const SpaceStructureRecord> rows, std::span<const SpaceProductionRecord> products,
	uint64_t next_structure, uint64_t next_vehicle);
/** Validate copied manufacturing ownership within its own structure snapshot and utility epoch.
 * @param rows Exact copied native structures, never active-world substitutes.
 * @param interval Same-file common allocation epoch, or UINT64_MAX for structural-only validation.
 * @return Whether every request, recipe, source and physical packet binding is bounded and owned once.
 */
bool ValidateOrbitalManufacturingRecords(std::span<const SpaceStructureRecord> rows, uint64_t interval = UINT64_MAX);
#endif
