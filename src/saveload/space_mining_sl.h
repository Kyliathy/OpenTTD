/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file space_mining_sl.h Explicit native mission continuation scalar adaptation; no independent mission owner.
 * @see docs/zz-reach2/upgrades/2026-10/r2sff1-space-fixes-features-1.md Group23.
 */
#ifndef SAVELOAD_SPACE_MINING_SL_H
#define SAVELOAD_SPACE_MINING_SL_H
#include "../space_vehicle.h"
#include "saveload.h"

/** Per-row explicit transport of the sole mission's optional/label fields; padding is never serialized. */
struct SpaceMiningContinuationWire {
	uint8_t phase = 0;
	uint32_t mineral = 0, previous_body = UINT32_MAX;
	uint8_t effective_present = 0, effective_kind = 0;
	uint32_t effective_id = UINT32_MAX;
	uint64_t effective_generation = 0;
	uint8_t departure_present = 0;
	int32_t departure_x = 0, departure_y = 0;
	uint16_t retry_ticks = 0;
	uint8_t waiting = 0, route_cursor = UINT8_MAX;
	uint32_t travel_ticks = 0;
};
/** @param state Original mission intent. @return Explicit portable scalar values; no native memory representation is written. */
SpaceMiningContinuationWire EncodeSpaceMiningContinuation(const SpaceMiningContinuationState &state);
/** @param wire Complete original scalar row. @return Canonical bounded intent or refusal, before native/same-file queue/lease joins. */
std::optional<SpaceMiningContinuationState> DecodeSpaceMiningContinuation(const SpaceMiningContinuationWire &wire);
/** @param fields Actual native/copied Space table keys. @param version Genuine file version. @return Whether current fields are complete/unique, or genuinely absent in older files. */
bool ValidateSpaceMiningContinuationSchema(std::span<const std::string_view> fields, SaveLoadVersion version);
struct LoadCheckData;
/** @param data Complete current same-file candidate owners. @param legacy_departure_supported Whether the genuine source era permits contained unqueued local departure. @return Whether intent, queues and both directions of actual mining leases agree without reading any open-world owner. */
bool ValidateSpaceMiningContinuationPreview(const LoadCheckData &data, bool legacy_departure_supported = true);
#endif /* SAVELOAD_SPACE_MINING_SL_H */
