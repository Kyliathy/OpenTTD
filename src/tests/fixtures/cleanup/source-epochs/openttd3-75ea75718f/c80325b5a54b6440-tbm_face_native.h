/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file tbm_face_native.h Face-loop state shared by the native observers that drive the face exchange in the real game
 * and reload its saved checkpoints. Both read the same fields and digests, so a reload is compared with the
 * uninterrupted run row for row.
 * @see docs/zz-reach2/upgrades/2026-10/r2df2-face-exchange-and-demo-acceptance.md Groups 8 and 9
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md
 */

#ifndef TESTS_TBM_FACE_NATIVE_H
#define TESTS_TBM_FACE_NATIVE_H

#include "../company_base.h"
#include "../train.h"
#include "../tbm.h"
#include "../tbm_exchange.h"
#include "../excavation.h"
#include "../layer_portal_cmd.h"
#include "../layer_portal_map.h"
#include "../core/format.hpp"
#include "../core/random_func.hpp"
#include "../timer/timer_game_tick.h"
#include <optional>
#include <string>
#include <utility>

/** Ticks a checkpoint's watch waits for the next transition before it reports the horizon instead (r2df2 Group 9). */
static constexpr uint FACE_WATCH_TICKS = 5000;

/** One tick's view of the face loop, compared before and after each exchange (r2df2 Group 8). */
struct FaceLoopSample {
	TBMPhase phase = TBMPhase::Commissioning; ///< Cutter project phase.
	TBMBlockedReason blocked = TBMBlockedReason::None; ///< Saved hold of the project.
	uint64_t phase_ticks = 0; ///< Ticks in that phase: the dwell once the cutter waits at its face.
	TBMMeetingKind meeting = TBMMeetingKind::Bay; ///< Where the cutter meets its hauler now.
	std::optional<TBMFaceStand> stand; ///< Stand behind the cutter, when one exists.
	TBMHaulerPhase hauler_phase = TBMHaulerPhase::ReturnEmpties; ///< Hauler cycle phase.
	TBMHaulerWait hauler_wait = TBMHaulerWait::None; ///< Hauler hold.
	uint32_t exchanges = 0; ///< Completed exchanges of the hauler.
	uint32_t cursor = 0; ///< Completed cuts.
	uint cutter_aboard = 0; ///< Spoil units aboard the cutter consist.
	uint hauler_aboard = 0; ///< Spoil units aboard the hauler.
	size_t cutter_packets = 0; ///< Cargo packets aboard the cutter consist.
	size_t hauler_packets = 0; ///< Cargo packets aboard the hauler.
	Money money = 0; ///< Owner's cash.
	TileIndex cutter_tile = INVALID_TILE; ///< Cutter front tile.
	TileIndex hauler_tile = INVALID_TILE; ///< Hauler front tile.
	TileIndex hauler_last_tile = INVALID_TILE; ///< Hauler's last part tile.
	Trackdir hauler_trackdir = Trackdir::Invalid; ///< Hauler front trackdir.
	uint16_t hauler_speed = 0; ///< Hauler's current speed; zero once it stands on its stand.
	bool hauler_on_pair = false; ///< A part of the hauler stands on either end of the project's portal pair.
};

/**
 * Count the spoil and the packets aboard one train.
 * @param front Actual front engine.
 * @return Units aboard and the number of packets holding them.
 */
inline std::pair<uint, size_t> CountAboard(const Train &front)
{
	uint units = 0;
	size_t packets = 0;
	for (const Train *part = &front; part != nullptr; part = part->Next()) {
		units += part->cargo.TotalCount();
		packets += part->cargo.Packets()->size();
	}
	return {units, packets};
}

/**
 * Digest every train part's simulation state, for runs that differ only in presentation (r2df2 task 8.3) and for a
 * reload compared with the uninterrupted run (task 9.4).
 * @return FNV-1a hash over each part's identity, position, motion, status and cargo.
 */
inline uint64_t DigestTrains()
{
	uint64_t hash = 14695981039346656037ULL;
	/** @param value Next quantity, mixed byte by byte. */
	const auto mix = [&hash](uint64_t value) {
		for (uint shift = 0; shift < 64; shift += 8) {
			hash ^= (value >> shift) & 0xFF;
			hash *= 1099511628211ULL;
		}
	};
	// Every part counts, in pool order: a wagon left behind or a different speed changes the digest.
	for (const Train *part : Train::Iterate()) {
		mix(part->index.base());
		mix(part->tile.base());
		mix(static_cast<uint32_t>(part->x_pos));
		mix(static_cast<uint32_t>(part->y_pos));
		mix(static_cast<uint32_t>(part->z_pos));
		mix(to_underlying(part->direction));
		mix(part->track.base());
		mix(part->cur_speed);
		mix(part->progress);
		mix(part->vehstatus.base());
		mix(part->cargo.TotalCount());
	}
	return hash;
}

/**
 * Name a tile with its plane coordinates and layer for the face-loop rows.
 * @param tile Any tile, possibly invalid.
 * @return "index(x,y,layer)", or "-" for an invalid tile.
 */
inline std::string DescribeTile(TileIndex tile)
{
	if (!IsValidTile(tile)) return "-";
	return fmt::format("{}({},{},{})", tile.base(), TileX(tile), TilePlaneY(tile), to_underlying(GetTileLayer(tile)));
}

/**
 * Read the face loop's state after the last tick.
 * @param project The followed project.
 * @return The sample; hauler fields keep their defaults while the project has no hauler.
 */
inline FaceLoopSample SampleFaceLoop(const TBMProject &project)
{
	FaceLoopSample sample;
	sample.phase = project.phase;
	sample.blocked = project.blocked;
	sample.phase_ticks = project.phase_ticks;
	sample.meeting = GetTBMMeetingKind(project);
	sample.stand = FindTBMFaceStand(project);
	const ExcavationJob *job = GetExcavationJob(project.job);
	sample.cursor = job == nullptr ? 0 : job->cursor;
	sample.money = Company::Get(project.company)->money;
	if (const Train *cutter = Train::GetIfValid(project.cutter); cutter != nullptr) {
		sample.cutter_tile = cutter->tile;
		std::tie(sample.cutter_aboard, sample.cutter_packets) = CountAboard(*cutter);
	}
	const TBMExchangeAssignment *assignment = GetTBMExchangeAssignment(project.id);
	if (assignment == nullptr) return sample;
	sample.hauler_phase = assignment->phase;
	sample.hauler_wait = assignment->wait;
	sample.exchanges = assignment->exchanges;
	if (const Train *hauler = Train::GetIfValid(assignment->hauler); hauler != nullptr) {
		sample.hauler_tile = hauler->tile;
		sample.hauler_last_tile = hauler->Last()->tile;
		sample.hauler_trackdir = hauler->GetVehicleTrackdir();
		sample.hauler_speed = hauler->cur_speed;
		std::tie(sample.hauler_aboard, sample.hauler_packets) = CountAboard(*hauler);
		const TileIndex pair = IsLayerPortalTile(project.portal) ? GetLayerPortalPairTile(project.portal) : INVALID_TILE;
		// Any part on either end of the pair counts: the hauler passes through it on its way in and out.
		for (const Train *part = hauler; part != nullptr; part = part->Next()) {
			if (part->tile == project.portal || part->tile == pair) sample.hauler_on_pair = true;
		}
	}
	return sample;
}

/**
 * Describe what changed between two samples, in the terms the controller steps in: either train's phase, the
 * hauler's hold, the project's hold, an exchange or a completed cut.
 * @param before State on the earlier tick.
 * @param after State on the later tick.
 * @return The changed fields as "name=old->new" words, or empty when none changed.
 */
inline std::string DescribeFaceTransition(const FaceLoopSample &before, const FaceLoopSample &after)
{
	std::string changed;
	if (before.phase != after.phase) changed += fmt::format(" cutter={}->{}", to_underlying(before.phase), to_underlying(after.phase));
	if (before.blocked != after.blocked) changed += fmt::format(" blocked={}->{}", to_underlying(before.blocked), to_underlying(after.blocked));
	if (before.hauler_phase != after.hauler_phase) changed += fmt::format(" hauler={}->{}", to_underlying(before.hauler_phase), to_underlying(after.hauler_phase));
	if (before.hauler_wait != after.hauler_wait) changed += fmt::format(" wait={}->{}", to_underlying(before.hauler_wait), to_underlying(after.hauler_wait));
	if (before.exchanges != after.exchanges) changed += fmt::format(" exchanges={}->{}", before.exchanges, after.exchanges);
	if (before.cursor != after.cursor) changed += fmt::format(" cursor={}->{}", before.cursor, after.cursor);
	return changed.empty() ? changed : changed.substr(1);
}

/**
 * Describe the owners a reload must reproduce: the tick clock, the game random state, the owner's money and the
 * project, exchange, geology, portal-work and train digests.
 * @param project The followed project.
 * @return One line of "name=value" words.
 */
inline std::string DescribeFaceOwners(const TBMProject &project)
{
	return fmt::format("counter={} random={:08x}{:08x} money={} projects={:016x} exchange={:016x} geology={:016x} portal={:016x} trains={:016x}",
		TimerGameTick::counter, _random.state[0], _random.state[1], Company::Get(project.company)->money.base(), ComputeTBMProjectDigest(),
		ComputeTBMExchangeDigest(), ComputeGeologyStateDigest(GetGeologyWorldState()), ComputeLayerPortalWorkDigest(), DigestTrains());
}

#endif /* TESTS_TBM_FACE_NATIVE_H */
