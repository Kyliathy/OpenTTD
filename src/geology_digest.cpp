/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file geology_digest.cpp Portable authoritative terrain/seam/vent/job diagnostics without hashing caches or padding. @see docs/zz-reach2/upgrades/2026-10/r2mlvs2-geology-and-excavation.md; r2mlvs15-savegame-determinism-and-lifecycle.md */

#include "stdafx.h"
#include "excavation.h"

#include "safeguards.h"

/** Explicit-width FNV-1a writer over actual owner fields, with no intermediate per-tile record allocations. */
class GeologyDigestWriter {
	uint64_t digest = 14695981039346656037ULL; ///< Fixed portable diagnostic root.
public:
	/**
	 * Fold an explicit-width unsigned value in canonical big-endian order.
	 * @param value Actual integer/reference bits.
	 * @param width Exact authoritative byte count, independent of host representation.
	 */
	void Value(uint64_t value, uint width)
	{
		// Explicit order/width excludes compiler padding, pointer addresses and native endianness from determinism diagnostics.
		for (uint i = width; i != 0; --i) {
			this->digest ^= (value >> ((i - 1) * 8)) & 255;
			this->digest *= 1099511628211ULL;
		}
	}
	/**
	 * Fold one canonical tile vector with its cardinality, preserving empty/nonempty geometry distinction.
	 * @param tiles Actual saved floor/route envelope, never a rebuilt lookup cache.
	 */
	void Tiles(std::span<const TileIndex> tiles)
	{
		this->Value(tiles.size(), 4);
		for (TileIndex tile : tiles) this->Value(tile.base(), 4); // Actual canonical order gives route/floor references one stable meaning.
	}
	/** @return Complete diagnostic value without exposing mutable writer state. */
	uint64_t Result() const { return this->digest; }
};

/**
 * Hash authoritative field order/counts while retaining sparse owner identity and separate packed terrain lifetime.
 * @param state Actual accepted/prepared value; validation belongs to its producer before this diagnostic read.
 * @return Portable complete geological state digest.
 */
uint64_t ComputeGeologyStateDigest(const GeologyWorldState &state)
{
	GeologyDigestWriter writer;
	writer.Value(0x47454f535f563031ULL, 8); // Domain GEOS_V01 prevents conflating this owner stream with metadata/other feature hashes.
	writer.Value(state.generator_version, 2);
	// Legacy worlds keep their existing diagnostic stream. New regional
	// worlds include the complete immutable difficulty rule and seed.
	if (state.hardness_version != 0) {
		writer.Value(state.hardness_version, 2);
		writer.Value(state.hardness_seed, 8);
	}
	writer.Value(Map::SizeX(), 4); writer.Value(Map::SizeY(), 4);
	writer.Value(state.terrain.PlaneSize(), 4); writer.Value(state.terrain.Data().size(), 4);
	// Packed cell bits are authoritative; no per-tile object, tile infrastructure field or cache order is included.
	for (uint8_t byte : state.terrain.Data()) writer.Value(byte, 1);
	writer.Value(state.deposits.NextID().base(), 4); writer.Value(state.deposits.Records().size(), 4);
	// Stable sparse IDs and explicit shape counts distinguish depleted/deleted seams from rebuilt bucket storage.
	for (const auto &deposit : state.deposits.Records()) {
		writer.Value(deposit.id.base(), 4); writer.Value(to_underlying(deposit.layer), 1);
		for (char byte : deposit.resource) writer.Value(static_cast<uint8_t>(byte), 1);
		writer.Value(to_underlying(deposit.hardness), 1); writer.Value(deposit.claimant.base(), 1);
		writer.Value(deposit.initial_reserve, 8); writer.Value(deposit.remaining_reserve, 8); writer.Value(deposit.surveyed_companies, 8);
		writer.Value(deposit.spans.size(), 2);
		for (const auto &span : deposit.spans) {
			writer.Value(span.y, 2); writer.Value(span.first_x, 2); writer.Value(span.last_x, 2);
		}
	}
	writer.Value(state.next_vent_id, 4); writer.Value(state.vents.size(), 4);
	// Natural thermal capacity remains its own geological identity and is never inferred from mineral overlap.
	for (const auto &vent : state.vents) {
		writer.Value(vent.id.base(), 4); writer.Value(vent.generation_version, 2); writer.Value(to_underlying(vent.layer), 1);
		writer.Value(vent.x, 2); writer.Value(vent.y, 2); writer.Value(vent.width, 1); writer.Value(vent.height, 1); writer.Value(vent.thermal_capacity_w, 8);
	}
	writer.Value(state.next_job_id, 4); writer.Value(state.jobs.size(), 4);
	// Actual progress/receipts/authorization define replay behavior; the rebuilt exclusive claim map carries no second authority.
	for (const auto &job : state.jobs) {
		writer.Value(job.id.base(), 4); writer.Value(job.company.base(), 1); writer.Value(job.cutter.base(), 4); writer.Value(job.origin.base(), 2);
		writer.Value(job.model_id, 2); writer.Value(job.commissioning_anchor.base(), 4); writer.Value(to_underlying(job.state), 1); writer.Value(to_underlying(job.pause_reason), 1);
		writer.Value(job.cursor, 4); writer.Value(job.partial_work_ticks, 8); writer.Value(job.revision, 8);
		writer.Value(job.completed_earth_t, 8); writer.Value(job.completed_rock_t, 8); writer.Value(static_cast<uint64_t>(job.completed_work_cost.base()), 8);
		writer.Value(job.pass_terms.has_value(), 1);
		if (job.pass_terms) {
			const auto &pass = *job.pass_terms;
			writer.Value(pass.earth_capacity_t, 8); writer.Value(pass.rock_capacity_t, 8); writer.Value(pass.fingerprint, 8);
			writer.Tiles(pass.exchange_path); writer.Value(pass.exchange_after.size(), 2);
			for (uint16_t prefix : pass.exchange_after) writer.Value(prefix, 2);
		}
		writer.Tiles(job.requested_tiles); writer.Tiles(job.authorized_tiles); writer.Value(job.route.size(), 4);
		for (const auto &cut : job.route) {
			writer.Value(cut.cutter_position.base(), 4); writer.Value(to_underlying(cut.heading), 1); writer.Tiles(cut.swept_tiles); writer.Tiles(cut.support_tiles);
		}
	}
	return writer.Result();
}
