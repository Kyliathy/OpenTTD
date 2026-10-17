/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file space_mining_tuning.h Immutable per-miner operating contracts, separate from physical hull/packet/work ownership.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group9
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs1-e-ground-and-space-budgets.md Heavy mining contract
 */
#ifndef SPACE_MINING_TUNING_H
#define SPACE_MINING_TUNING_H

#include <cstdint>

inline constexpr uint16_t SPACE_HEAVY_MINER_MODEL = 21; ///< Appended mining role; existing cargo-less construction robot16 keeps its meaning.

/** Immutable operating quantities for an actual native mining role.
 * This definition owns no cargo, reserve, service progress, pod or return claim.
 * Existing model15 derives its values from the delivered shared Space budget.
 * Heavy21 becomes commissionable only after its versioned paid producer exists.
 */
struct SpaceMiningTuning {
	uint16_t model; ///< Canonical append-only profile model, never an EngineID or VehicleID.
	uint16_t hold_sixteenths; ///< Complete freight mass bound; physical CargoSpec weights remain authoritative.
	uint16_t extraction_units; ///< Maximum whole source units committed in one completed extraction interval.
	uint16_t extraction_ticks; ///< Synchronized working ticks per extraction interval.
	uint16_t active_per_day; ///< Active extraction/service upkeep, accrued with the original native remainder.
	uint16_t idle_per_day; ///< Travel/wait upkeep; rendered frames earn no work or cost opportunities.
	uint16_t service_ticks; ///< Actual earned service work required before renewing an exhausted allowance.
	uint8_t service_metal; ///< Real RFMT packets consumed once at completed service.
	uint8_t service_electronics; ///< Real ELEC packets consumed once at completed service.
	uint8_t returns_per_service; ///< Completed recovery batches allowed by an actual paid initial/service completion.
	uint8_t starting_pods; ///< Pod capacity only; this definition never assembles or grants a physical shell.
};

/** Query one immutable mining operating contract without registering an engine or changing any native state.
 * @param model Canonical logical model; construction16, hauler17 and survey19 have no mining contract.
 * @return Existing15 or appended21 tuning, or nullptr for every other role/unknown model.
 */
const SpaceMiningTuning *GetSpaceMiningTuning(uint16_t model);

#endif /* SPACE_MINING_TUNING_H */
