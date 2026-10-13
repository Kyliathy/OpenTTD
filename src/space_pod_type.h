/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file space_pod_type.h Typed identities for passive paid pod shells. */
#ifndef SPACE_POD_TYPE_H
#define SPACE_POD_TYPE_H
#include "core/pool_type.hpp"

using SpacePodID = PoolID<uint16_t, struct SpacePodIDTag, 1024, UINT16_MAX>;
struct SpacePodRef {
	SpacePodID pod = SpacePodID::Invalid();
	uint64_t generation = 0;
	bool operator==(const SpacePodRef &) const = default;
};
#endif
