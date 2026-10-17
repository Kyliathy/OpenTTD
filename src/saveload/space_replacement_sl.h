/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file space_replacement_sl.h Bounded append-only transport for actual paid replacement producers.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group8BC
 * Both native load and copied preview use this codec; semantic ownership is validated against the complete same-file snapshot.
 */
#ifndef SAVELOAD_SPACE_REPLACEMENT_SL_H
#define SAVELOAD_SPACE_REPLACEMENT_SL_H
#include "../space_production.h"
#include "saveload_error.hpp"

/** Compute the exact appended optional receipt extent.
 * @param intent Actual contained producer intent, empty for ordinary purchases/clones.
 * @return One strict presence byte plus twenty payload bytes when present.
 */
inline size_t SpaceReplacementTransportLength(const std::optional<SpaceReplacementIntent> &intent) { return intent ? 21 : 1; }

/** Serialize a previously semantically validated contained receipt without resolving native owners.
 * @param intent Actual optional producer receipt.
 * @param write Native big-endian writer accepting an unsigned value and exact byte width.
 */
template <typename Writer>
void WriteSpaceReplacementTransport(const std::optional<SpaceReplacementIntent> &intent, Writer &&write)
{
	write(intent.has_value(), 1);
	if (!intent) return;
	write(intent->source.vehicle.base(), 4); write(intent->source.generation, 8); write(intent->source_model, 2);
	write(intent->resume, 1); write(intent->settled, 1); write(intent->sale_credit, 4);
}

/** Decode detached bounded intent, granting no hull, retirement or sale authority.
 * @param read Family-bounded big-endian reader accepting an exact width; it rejects truncation before reading any field.
 * @return Canonical absent intent or one receipt with strict booleans; complete same-file semantic validation follows.
 */
template <typename Reader>
std::optional<SpaceReplacementIntent> ReadSpaceReplacementTransport(Reader &&read)
{
	const auto present = read(1);
	if (present > 1) SlErrorCorrupt("Invalid Space replacement presence");
	if (present == 0) return std::nullopt;
	SpaceReplacementIntent intent;
	intent.source = {VehicleID{static_cast<uint32_t>(read(4))}, read(8)}; intent.source_model = static_cast<uint16_t>(read(2));
	const auto resume = read(1), settled = read(1);
	// Arbitrary byte values must not normalize into a running request or a retirement receipt in either full load or copied preview.
	if (resume > 1 || settled > 1) SlErrorCorrupt("Invalid Space replacement flags");
	intent.resume = resume != 0; intent.settled = settled != 0; intent.sale_credit = static_cast<uint32_t>(read(4));
	return intent;
}
#endif
