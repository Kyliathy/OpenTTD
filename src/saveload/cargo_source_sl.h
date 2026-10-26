/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file cargo_source_sl.h Shared native/copied cargo-source admission without live-world lookups.
 * @see docs/zz-reach2/upgrades/2026-10/r2sff1-space-fixes-features-1.md Group15; malformed tagged values must not be flattened by the legacy writer.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs6-mines-and-resource-chains.md Group13CB.
 * This predicate grants no cargo, purchase or ownership rights.
 */
#ifndef SAVELOAD_CARGO_SOURCE_SL_H
#define SAVELOAD_CARGO_SOURCE_SL_H

#include "saveload.h"
#include "../space.h"
#include "../multilayer_space_content.h"
#include <span>
#include <string_view>

/** Explicit fixed-width CAPA transport adaptation; never serialized as host memory or stored as a second live packet source. */
struct CargoSourceAsteroidWire {
	uint32_t body = UINT32_MAX; ///< Canonical absent body for ordinary and genuine old sources.
	uint32_t mineral = 0; ///< Four label bytes encoded in big-endian significance; native U32 transport owns actual byte writing.
};
/** @param source Original canonical provenance. @return Explicit optional-payload wire values, with canonical empty fields for non-asteroid sources; the caller separately validates its source era. */
CargoSourceAsteroidWire EncodeCargoSourceAsteroid(const Source &source);
/** @param scalar Original decoded type/U16 identity without an in-memory optional payload. @param wire Candidate fixed-width asteroid fields, or canonical defaults in genuinely older files. @param feeder_share Original virtual credit. @param version Candidate file's actual save era. @return Unambiguous era-admitted source, leaving genuine legacy IDs for the later same-file-body migration. */
std::optional<Source> DecodeCargoPacketSource(Source scalar, CargoSourceAsteroidWire wire, Money feeder_share, SaveLoadVersion version);
/** @param fields Original decoded table field names, including unknown fields. @param version Candidate file's actual era. @return Whether required original fields and versioned asteroid fields are present, and newer payload keys are absent in genuine older sources. */
bool ValidateCargoPacketSourceSchema(std::span<const std::string_view> fields, SaveLoadVersion version);

/** Validate serialized source meaning before a native packet or detached preview can be accepted.
 * @param source Candidate file's original source type and identity.
 * @param feeder_share Candidate file's original virtual transfer credit.
 * @param version Candidate file's genuine save era, never the open world's version.
 * @return Whether this era permits the source and its original provenance/credit bounds.
 */
inline bool IsValidCargoPacketSource(Source source, Money feeder_share, SaveLoadVersion version)
{
	// A mismatched ordinary tag/asteroid payload or dual identity must fail
	// before the legacy writer could discard provenance and admit paying cargo.
	if (!source.IsCanonical()) return false;
	switch (source.type) {
		case SourceType::Industry:
		case SourceType::Town:
		case SourceType::Headquarters:
			return true; // Preserve original domains, invalidated identities and ordinary feeder accounting.
		case SourceType::Asteroid:
			// Modern natural provenance requires the full typed origin. Older
			// sources retain only their original bounded scalar until migration;
			// neither era may borrow the other's representation or paying purpose.
			if (version >= SaveLoadVersion::AsteroidCargoOrigins) {
				const auto origin = source.GetAsteroidOrigin();
				return origin && std::ranges::any_of(GetMultiLayerAsteroidDefinitions(),
					/** @param definition Immutable natural class. @return Whether it supplies this original stable mineral meaning. */
					[&](const auto &definition) { return definition.cargo == origin->mineral; });
			}
			return version >= SaveLoadVersion::SpaceMiningVehicles && !source.GetAsteroidOrigin() && source.id < MAX_ORBITAL_BODIES;
		case SourceType::CompanySupply:
			// Retired buyers retain the non-paying purpose. A fake owner, an older era or any stored feeder credit cannot authorize paid material custody.
			return version >= SaveLoadVersion::CompanySupplyPackets &&
				(source.id == Source::Invalid || source.id < MAX_COMPANIES) && feeder_share == 0;
		default:
			return false;
	}
}

#endif /* SAVELOAD_CARGO_SOURCE_SL_H */
