/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file space_structure_type.h Native orbital site identities. */
#ifndef SPACE_STRUCTURE_TYPE_H
#define SPACE_STRUCTURE_TYPE_H
#include "core/pool_type.hpp"

using SpaceStructureID = PoolID<uint16_t, struct SpaceStructureIDTag, 1024, UINT16_MAX>;
struct SpaceStructureRef {
	SpaceStructureID structure = SpaceStructureID::Invalid();
	uint64_t generation = 0;
	bool operator==(const SpaceStructureRef &) const = default;
};
#endif
