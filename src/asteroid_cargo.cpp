/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file asteroid_cargo.cpp Issued historical natural origins with independent live/candidate membership.
 * @see docs/zz-reach2/upgrades/2026-10/r2sff1-space-fixes-features-1.md Group16.
 */
#include "stdafx.h"
#include "asteroid_cargo.h"
#include "cargopacket.h"
#include "safeguards.h"

/** @param mineral Immutable original cargo label. @return Its natural resource class, independent of the active CargoSpec ordinal mapping. */
static std::optional<AsteroidClass> AsteroidMineralClass(CargoLabel mineral)
{
	const auto &definitions = GetMultiLayerAsteroidDefinitions();
	for (uint8_t value = 0; value < definitions.size(); ++value) if (definitions[value].cargo == mineral) return static_cast<AsteroidClass>(value);
	return std::nullopt;
}

/** @param origin Original full packet provenance. @param next Issuance from the same field/candidate. @param live_class Class of the surviving exact ID, or absence for an issued historical body. @return Whether history is admissible without resurrecting a resource owner. */
static bool ValidateOrigin(const AsteroidCargoOrigin &origin, uint32_t next, std::optional<AsteroidClass> live_class)
{
	const auto mineral_class = AsteroidMineralClass(origin.mineral);
	// A future/sentinel ID or unknown mineral cannot become historical simply
	// because lookup fails. Surviving bodies additionally prove exact composition.
	if (origin.body >= next || !mineral_class) return false;
	return !live_class || *live_class == *mineral_class;
}

/** @copydoc ValidateAsteroidCargoOrigin(const AsteroidCargoOrigin &, const AsteroidField &) */
bool ValidateAsteroidCargoOrigin(const AsteroidCargoOrigin &origin, const AsteroidField &field)
{
	const auto *body = FindAsteroidBody(field, OrbitalBodyID{origin.body});
	return ValidateOrigin(origin, field.next_body_id, body == nullptr ? std::nullopt : std::optional{body->resource_class});
}

/** @copydoc ValidateAsteroidCargoOrigin(const AsteroidCargoOrigin &, std::span<const AsteroidIdentityRecord>, uint32_t) */
bool ValidateAsteroidCargoOrigin(const AsteroidCargoOrigin &origin, std::span<const AsteroidIdentityRecord> bodies, uint32_t next_body_id)
{
	const auto *body = FindAsteroidIdentity(bodies, OrbitalBodyID{origin.body});
	return ValidateOrigin(origin, next_body_id, body == nullptr ? std::nullopt : std::optional{body->resource_class});
}

/** @param id Original old 16-bit body reference. @param resource_class Class from that same owner's surviving body. @return Explicit legacy-derived meaning, or refusal for a malformed class; this does not guess missing retired-body history. */
static std::optional<AsteroidCargoOrigin> LegacyOrigin(SourceID id, AsteroidClass resource_class)
{
	const auto &definitions = GetMultiLayerAsteroidDefinitions();
	if (to_underlying(resource_class) >= definitions.size()) return std::nullopt;
	return AsteroidCargoOrigin{id, definitions[to_underlying(resource_class)].cargo};
}

/** @copydoc ResolveAsteroidCargoOrigin(const Source &, const AsteroidField &) */
std::optional<AsteroidCargoOrigin> ResolveAsteroidCargoOrigin(const Source &source, const AsteroidField &field)
{
	// The source tag and scalar/payload representation must agree before either
	// modern historical meaning or an old surviving-body receipt is considered.
	if (source.type != SourceType::Asteroid || !source.IsCanonical()) return std::nullopt;
	if (const auto origin = source.GetAsteroidOrigin()) return ValidateAsteroidCargoOrigin(*origin, field) ? origin : std::nullopt;
	if (source.id >= MAX_ORBITAL_BODIES) return std::nullopt;
	const auto *body = FindAsteroidBody(field, OrbitalBodyID{source.id});
	if (body == nullptr) return std::nullopt;
	const auto origin = LegacyOrigin(source.id, body->resource_class);
	return origin && ValidateAsteroidCargoOrigin(*origin, field) ? origin : std::nullopt;
}

/** @copydoc ResolveAsteroidCargoOrigin(const Source &, std::span<const AsteroidIdentityRecord>, uint32_t) */
std::optional<AsteroidCargoOrigin> ResolveAsteroidCargoOrigin(const Source &source, std::span<const AsteroidIdentityRecord> bodies, uint32_t next_body_id)
{
	// Candidate provenance never borrows a same-numbered body from the open
	// world, even when old transport needs its original mineral reconstructed.
	if (source.type != SourceType::Asteroid || !source.IsCanonical()) return std::nullopt;
	if (const auto origin = source.GetAsteroidOrigin()) return ValidateAsteroidCargoOrigin(*origin, bodies, next_body_id) ? origin : std::nullopt;
	if (source.id >= MAX_ORBITAL_BODIES) return std::nullopt;
	const auto *body = FindAsteroidIdentity(bodies, OrbitalBodyID{source.id});
	if (body == nullptr) return std::nullopt;
	const auto origin = LegacyOrigin(source.id, body->resource_class);
	return origin && ValidateAsteroidCargoOrigin(*origin, bodies, next_body_id) ? origin : std::nullopt;
}

/** @copydoc NormalizeLegacyAsteroidCargoPackets */
bool NormalizeLegacyAsteroidCargoPackets(const AsteroidField &field)
{
	std::vector<std::pair<CargoPacket *, CargoLabel>> upgrades;
	// Preflight the whole original native packet family before changing any
	// provenance. A missing old source body cannot be repaired from a cargo slot,
	// guessed class, regenerated field or a different open world's resource.
	for (auto *packet : CargoPacket::Iterate()) {
		const auto source = packet->GetSource();
		if (source.type != SourceType::Asteroid) continue;
		const auto origin = ResolveAsteroidCargoOrigin(source, field);
		if (!origin) return false;
		if (!source.GetAsteroidOrigin()) upgrades.emplace_back(packet, origin->mineral);
	}
	// Every identity/mineral was admitted above. In-place qualification changes
	// only original provenance, never packet ownership, units or financial history.
	for (const auto &[packet, mineral] : upgrades) {
		const bool changed = packet->UpgradeLegacyAsteroidSource(mineral); assert(changed);
		if (!changed) return false;
	}
	return true;
}
