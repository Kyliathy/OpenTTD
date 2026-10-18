/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file cargo_source_sl.h Shared native/copied cargo-source admission without live-world lookups.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs6-mines-and-resource-chains.md Group13CB.
 * This predicate grants no cargo, purchase or ownership rights.
 */
#ifndef SAVELOAD_CARGO_SOURCE_SL_H
#define SAVELOAD_CARGO_SOURCE_SL_H

#include "saveload.h"
#include "../space.h"

/** Validate serialized source meaning before a native packet or detached preview can be accepted.
 * @param source Candidate file's original source type and identity.
 * @param feeder_share Candidate file's original virtual transfer credit.
 * @param version Candidate file's genuine save era, never the open world's version.
 * @return Whether this era permits the source and its original provenance/credit bounds.
 */
inline bool IsValidCargoPacketSource(Source source, Money feeder_share, SaveLoadVersion version)
{
	switch (source.type) {
		case SourceType::Industry:
		case SourceType::Town:
		case SourceType::Headquarters:
			return true; // Preserve original domains, invalidated identities and ordinary feeder accounting.
		case SourceType::Asteroid:
			// Finite orbital provenance requires its delivered era and a real bounded body domain; it cannot masquerade as an ordinary industry.
			return version >= SaveLoadVersion::SpaceMiningVehicles && source.id < MAX_ORBITAL_BODIES;
		case SourceType::CompanySupply:
			// Retired buyers retain the non-paying purpose. A fake owner, an older era or any stored feeder credit cannot authorize paid material custody.
			return version >= SaveLoadVersion::CompanySupplyPackets &&
				(source.id == Source::Invalid || source.id < MAX_COMPANIES) && feeder_share == 0;
		default:
			return false;
	}
}

#endif /* SAVELOAD_CARGO_SOURCE_SL_H */
