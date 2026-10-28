/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file cargo_source_digest.h Canonical full-provenance input for existing native semantic digests.
 * @see docs/zz-reach2/upgrades/2026-10/r2sff1-space-fixes-features-1.md Group16.
 */
#ifndef CARGO_SOURCE_DIGEST_H
#define CARGO_SOURCE_DIGEST_H
#include "source_type.h"

/**
 * Append source meaning to an existing digest without changing its integer byte order.
 * @tparam Sink Existing callable accepting one unsigned 64-bit scalar.
 * @param source Original native packet provenance, already subject to source admission.
 * @param add Existing digest's scalar sink. Legacy/ordinary input remains exactly type then ID.
 * Modern origins add a domain marker, full ID and all four label bytes in label order;
 * no host-endian reinterpretation or live resource lookup participates.
 */
template <typename Sink>
inline void AppendCargoSourceDigest(const Source &source, Sink &&add)
{
	add(to_underlying(source.type)); add(source.id);
	if (const auto origin = source.GetAsteroidOrigin()) {
		add(uint64_t{0x4153544F52494731}); add(origin->body);
		for (const auto byte : origin->mineral) add(byte);
	}
}
#endif /* CARGO_SOURCE_DIGEST_H */
