/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file cargo_source_sl.h Shared native/copied cargo-source admission without live-world lookups.
 * @see docs/zz-reach2/upgrades/2026-10/r2sff1-space-fixes-features-1.md Group15; malformed tagged values must not be flattened by the legacy writer.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs6-mines-and-resource-chains.md Group13CB.
 * @see docs/zz-reach2/upgrades/2026-10/m2o28-1-merge-oct28.md Group 15; the source format comes from the accepted lineage, not the version number.
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
/** The source-format facts the cargo-source codec needs. The lineage owner supplies them; the codec never
 * compares a version number to decide a branch feature, because 441 means different things to different writers.
 */
struct CargoSourceFormat {
	SaveLoadVersion version = SaveLoadVersion::MinVersion; ///< Original envelope version, consulted only for the shared gates TableChunks, SpaceMiningVehicles and CompanySupplyPackets.
	bool typed_asteroid_origins = false; ///< Whether the source carries SaveCapability::AsteroidCargoOrigins (CAPA `asteroid_body`/`asteroid_mineral`).

	/** Describe a shared-era source, which never carries typed asteroid origins. Implicit on purpose, so the
	 * shared-gate callers keep naming their era; a pending Space fixes name (equal to MaxVersion) is a misuse.
	 * @param version Original shared-era version.
	 */
	CargoSourceFormat(SaveLoadVersion version) : version(version) { assert(version != SaveLoadVersion::MaxVersion); }
	/** @param version Original envelope version. @param typed_asteroid_origins Whether the source carries typed origins. */
	CargoSourceFormat(SaveLoadVersion version, bool typed_asteroid_origins) : version(version), typed_asteroid_origins(typed_asteroid_origins) {}
	/** @param source Accepted classification. @return Its envelope version and its own typed-origin capability. */
	static CargoSourceFormat FromLineage(const SaveLineageResult &source);
	/** @return The bound load attempt's format while loading, otherwise the current writer's. */
	static CargoSourceFormat Current();
};

/** @param source Original canonical provenance. @return Explicit optional-payload wire values, with canonical empty fields for non-asteroid sources; the caller separately validates its source format. */
CargoSourceAsteroidWire EncodeCargoSourceAsteroid(const Source &source);
/** @param scalar Original decoded type/U16 identity without an in-memory optional payload. @param wire Candidate fixed-width asteroid fields, or canonical defaults where the source has none. @param feeder_share Original virtual credit. @param format Candidate file's accepted source format. @return Unambiguous format-admitted source, leaving genuine legacy IDs for the later same-file-body migration. */
std::optional<Source> DecodeCargoPacketSource(Source scalar, CargoSourceAsteroidWire wire, Money feeder_share, const CargoSourceFormat &format);
/** @param fields Original decoded table field names, including unknown fields. @param format Candidate file's accepted source format. @return Whether required original fields are present, and the typed asteroid keys are present exactly when the source carries typed origins. */
bool ValidateCargoPacketSourceSchema(std::span<const std::string_view> fields, const CargoSourceFormat &format);

/** Validate serialized source meaning before a native packet or detached preview can be accepted.
 * @param source Candidate file's original source type and identity.
 * @param feeder_share Candidate file's original virtual transfer credit.
 * @param format Candidate file's accepted source format, never the open world's version.
 * @return Whether this format permits the source and its original provenance/credit bounds.
 */
inline bool IsValidCargoPacketSource(Source source, Money feeder_share, const CargoSourceFormat &format)
{
	const auto version = format.version;
	// A mismatched ordinary tag/asteroid payload or dual identity must fail
	// before the legacy writer could discard provenance and admit paying cargo.
	if (!source.IsCanonical()) return false;
	switch (source.type) {
		case SourceType::Industry:
		case SourceType::Town:
		case SourceType::Headquarters:
			return true; // Preserve original domains, invalidated identities and ordinary feeder accounting.
		case SourceType::Asteroid:
			// A source with typed origins requires the full typed origin. Others
			// retain only their original bounded scalar until migration; neither
			// format may borrow the other's representation or paying purpose.
			if (format.typed_asteroid_origins) {
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
