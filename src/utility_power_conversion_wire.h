/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @see docs/zz-reach2/architecture/savegame/archi-savegame.md; original B adapters use explicit lineage capabilities.
 * @file utility_power_conversion_wire.h Exact428 auxiliary conversion metadata/provenance scalars; original utility rows retain their format.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs18-geothermal-energy-and-industry.md Group8DB.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs15-savegame-determinism-and-lifecycle.md.
 */
#ifndef UTILITY_POWER_CONVERSION_WIRE_H
#define UTILITY_POWER_CONVERSION_WIRE_H

#include <cstdint>

/** Actual whole shared ledger; only an unpublished load candidate is joined with these auxiliary fields. */
struct UtilityPowerState;

/** Fixed31-byte copied wire row; exact kind/unused-field checks precede native reference resolution. */
struct UtilityHeatConversionWire {
	uint8_t kind = 0; ///< Metadata0 or original thermal contribution1; no other interpretation exists.
	uint32_t output = 0; ///< Existing conversion-only electrical producer in both row kinds.
	uint32_t input = 0; ///< Metadata original HeatGenerator consumer; zero for an origin row.
	uint8_t efficiency = 0; ///< Metadata immutable percent1..100; zero for an origin row.
	uint8_t numerator = 0; ///< Metadata saved percent remainder0..99; zero for an origin row.
	uint64_t capacity_w = 0; ///< Metadata actual immutable electrical ceiling; zero for an origin row.
	uint32_t origin = 0; ///< Origin row's existing original thermal producer; zero in metadata.
	uint64_t heat_j = 0; ///< Origin row's positive lifetime actual input, never another spendable balance; zero in metadata.
};

/** Reset only unpublished full-load auxiliary header/rows during world reset or failed-load cleanup. */
void ResetUtilityHeatConversionLoadStaging();
/**
 * Join complete exact auxiliary data to an unpublished original utility candidate.
 * @param state This full load or independent preview's copied original utility state; never the live ledger.
 * @param preview True reads only the preview's copied auxiliary rows/metadata/companies.
 * Throws native corrupt-save refusal on omission, malformed fields, wrong references or inconsistent metadata.
 * Repeated checks replace only candidate extension fields, preserving one original owner and exact history.
 */
void JoinLoadedUtilityHeatConversions(UtilityPowerState &state, bool preview);

#endif /* UTILITY_POWER_CONVERSION_WIRE_H */
