/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file cargo_source_sl.cpp Bounded explicit natural-source fields under the original native CAPA owner.
 * @see docs/zz-reach2/upgrades/2026-10/r2sff1-space-fixes-features-1.md Group21.
 * These codecs grant no packet, field identity, mineral stock, payment or custody.
 */
#include "../stdafx.h"
#include "cargo_source_sl.h"
#include "../safeguards.h"

/** @copydoc EncodeCargoSourceAsteroid */
CargoSourceAsteroidWire EncodeCargoSourceAsteroid(const Source &source)
{
	CargoSourceAsteroidWire wire;
	if (const auto origin = source.GetAsteroidOrigin()) {
		wire.body = origin->body;
		// Explicit label significance is stable on every host. SlObject writes
		// the resulting U32 with its existing portable byte-order implementation.
		for (const auto byte : origin->mineral) wire.mineral = (wire.mineral << 8) | byte;
	}
	return wire;
}

/** @copydoc DecodeCargoPacketSource */
std::optional<Source> DecodeCargoPacketSource(Source scalar, CargoSourceAsteroidWire wire, Money feeder_share, SaveLoadVersion version)
{
	// Raw scalar decoding never has an optional host payload. Rejecting one
	// prevents a caller from hiding dual wire/host identities in a valid tag.
	if (!scalar.IsCanonical() || scalar.GetAsteroidOrigin()) return std::nullopt;
	if (version < SaveLoadVersion::AsteroidCargoOrigins) {
		// An older file cannot smuggle typed origin data through unknown keys;
		// actual header absence is checked separately in both loading modes.
		if (wire.body != UINT32_MAX || wire.mineral != 0) return std::nullopt;
		return IsValidCargoPacketSource(scalar, feeder_share, version) ? std::optional{scalar} : std::nullopt;
	}
	if (scalar.type != SourceType::Asteroid) {
		// Ordinary/company-supply provenance keeps its existing scalar identity
		// and credit rules, with no hidden natural-resource payload attached.
		if (wire.body != UINT32_MAX || wire.mineral != 0) return std::nullopt;
		return IsValidCargoPacketSource(scalar, feeder_share, version) ? std::optional{scalar} : std::nullopt;
	}
	if (scalar.id != Source::Invalid) return std::nullopt;
	CargoLabel mineral;
	// Four explicit bytes reconstruct the immutable label without copying a
	// host-endian struct or inferring a cargo ordinal from the active world.
	for (uint index = 0; index < mineral.size(); ++index) mineral[index] = static_cast<uint8_t>(wire.mineral >> ((3 - index) * 8));
	const auto source = Source::FromAsteroid(wire.body, mineral);
	return IsValidCargoPacketSource(source, feeder_share, version) ? std::optional{source} : std::nullopt;
}

/** @copydoc ValidateCargoPacketSourceSchema */
bool ValidateCargoPacketSourceSchema(std::span<const std::string_view> fields, SaveLoadVersion version)
{
	if (version < SaveLoadVersion::TableChunks) return true;
	// Quantity, source purpose and virtual credit cannot be synthesized from
	// table defaults. Both loader paths require every original named field.
	for (const std::string_view name : {"count", "source_type", "source_id", "feeder_share", "source", "source_xy"}) {
		if (std::ranges::count(fields, name) != 1) return false;
	}
	const bool modern = version >= SaveLoadVersion::AsteroidCargoOrigins;
	// Typed keys are mandatory in their own era and forbidden in older table
	// sources. Silently skipped newer bytes cannot masquerade as legacy history.
	for (const std::string_view name : {"asteroid_body", "asteroid_mineral"}) {
		if (std::ranges::count(fields, name) != (modern ? 1 : 0)) return false;
	}
	return true;
}
