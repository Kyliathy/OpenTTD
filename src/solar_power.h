/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file solar_power.h Draft M0 solar-source/assignment contract; operational behavior remains Plan 12 work. @see docs/zz-reach2/upgrades/2026-10/r2mlvs12-solar-farms-mirrors-and-eti.md */

#ifndef SOLAR_POWER_H
#define SOLAR_POWER_H

#include "multilayer_content.h"
#include "industry_type.h"

/** Stable deployed mirror identity owned by solar state; not an extra routed vehicle/engine design. */
using SolarMirrorID = StrongType::Typedef<uint32_t, struct SolarMirrorIDTag, StrongType::Compare>;

/** Solar-owned source fields; generated/allocated electricity remains with the one shared utility ledger. */
struct SolarGenerationAccount {
	IndustryID industry = IndustryID::Invalid(); ///< Actual first-party eligible Surface solar industry reference.
	UtilityProducerID utility_producer{UINT32_MAX}; ///< Existing shared ledger reference, not a new solar grid.
	uint64_t accounting_interval = 0; ///< Source interval published once to the utility owner.
	uint64_t baseline_energy_j = 0; ///< Unaugmented available joules for this interval, at most one quarter of the utility account bound.
	uint16_t mirror_remainder = 0; ///< Saved bonus numerator remainder, 0..9,999 basis-point units.
};

/** One saved assignment, with real references and no stacking at one target. */
struct MirrorAssignment {
	SolarMirrorID mirror_id{UINT32_MAX}; ///< Solar-owned deployed object; launch payload/phase ownership remains with Plan 9.
	IndustryID target = IndustryID::Invalid(); ///< Target deletion clears assignment before the next source publication.
	UtilityProducerID target_producer{UINT32_MAX}; ///< Must match the source's shared utility account.
	bool operational = false; ///< Feature owner validates launch/deployment/service/ownership before claiming coverage.
};

/** Proposed source output before monthly statistics and shared utility publication; no live mutation. */
struct SolarMirrorOutput {
	uint64_t baseline_j = 0; ///< Original available production, never last month's already-boosted output.
	uint64_t bonus_j = 0; ///< At most three times baseline; zero baseline gives zero bonus.
	uint64_t effective_j = 0; ///< Baseline plus bonus, published exactly once.
	uint16_t mirror_remainder = 0; ///< New saved fractional bonus remainder.
};

/**
 * Draft pure source-side augmentation signature; implementation and runtime tests follow Content Freeze.
 * Valid full coverage gives baseline + 3 x baseline, at most 4x; partial coverage
 * uses basis points with checked integer carry. Missing/inactive/mismatched targets
 * give unchanged baseline and clear obsolete assignment carry; invalid arithmetic
 * refuses. No freight packet, town effect, ledger debit or historical output changes.
 * @param source Validated eligible Surface solar identity/baseline/remainder for one interval.
 * @param assignment Optional unique launched/maintained assignment; references must match the source.
 * @param coverage_basis_points Validated active interval fraction, 0..10,000, never a render-time fraction.
 * @return Proposed baseline/bonus/effective joules and carry, or no result for invalid bounds.
 */
std::optional<SolarMirrorOutput> ComputeMirrorSolarOutput(const SolarGenerationAccount &source, const MirrorAssignment *assignment, uint16_t coverage_basis_points);

#endif /* SOLAR_POWER_H */
