/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file tbm_feedback_haul_fixture.h Paced paid construction of a retained fifty-cell native haul route.
 * @see docs/zz-reach2/upgrades/2026-10/r2tbmf-tbm-fixes.md Group1B
 */

#ifndef TESTS_TBM_FEEDBACK_HAUL_FIXTURE_H
#define TESTS_TBM_FEEDBACK_HAUL_FIXTURE_H

#include "../rail_cmd.h"
#include "../tbm.h"
#include "../core/backup_type.hpp"
#include "../rail_map.h"
#include "../station_map.h"
#include "../track_func.h"
#include "../company_base.h"
#include "../company_func.h"
#include "../strings_func.h"
#include "../command_func.h"

/** An ordinary rail command to execute on a real unoccupied Surface cell. */
struct FeedbackRailOperation {
	TileIndex tile; ///< Actual source-plane cell; no portal or station is replaced.
	Track track; ///< Actual piece connecting both neighboring path cells.
	bool remove; ///< Remove the old short approach rather than add the paid detour.
};

/** Paced native construction sequence; it changes neither clearing budgets nor company funds directly. */
class FeedbackHaulBuilder {
	std::vector<FeedbackRailOperation> operations; ///< Full deterministic ordinary-command sequence.
	size_t cursor = 0; ///< Next command, advanced only after successful actual funding.
	CompanyID owner; ///< Existing demo controller, station and receiver owner.
	RailType railtype; ///< Actual source portal's compatible ordinary rail.
	uint ticks = 0; ///< Native elapsed time since construction started; normal clearing capacity replenishes naturally.
public:
	Money cost = 0; ///< Sum of actual command-kernel charges, never a replacement company balance.
	/**
	 * Plan a fifty-cell one-way route to the demo's unchanged Surface station.
	 * @param project Retained demo project; actual participants must remain stopped during construction.
	 */
	explicit FeedbackHaulBuilder(const TBMProject &project) : owner(project.company), railtype(GetRailType(project.portal))
	{
		REQUIRE(project.portal == TileXYL(13, 10, MapLayer::Surface));
		// Remove only the three unoccupied short approach pieces. Portal, station,
		// receivers and parked vehicle/cargo identities remain their native owners.
		for (uint y = 7; y <= 9; ++y) this->operations.push_back({TileXYL(13, y, MapLayer::Surface), Track::Y, true});
		std::vector<TileIndex> path{project.portal, TileXYL(13, 9, MapLayer::Surface)};
		// Preserve the original station approach and its existing turnaround
		// buffer. The detour adds forty-four transitions between the same ends;
		// moving the approach to the other side would test a different terminal.
		for (uint x = 14; x <= 35; ++x) path.push_back(TileXYL(x, 9, MapLayer::Surface));
		for (uint y = 8; y >= 7; --y) path.push_back(TileXYL(35, y, MapLayer::Surface));
		for (uint x = 34; x >= 13; --x) path.push_back(TileXYL(x, 7, MapLayer::Surface));
		for (uint y = 6; y >= 4; --y) path.push_back(TileXYL(13, y, MapLayer::Surface));
		REQUIRE(path.size() == 51);
		// Match both incoming and outgoing direction at every real corner; a
		// shorter extra branch would invalidate the measured haul distance.
		for (size_t i = 1; i + 1 < path.size(); ++i) {
			const auto entry = DiagdirBetweenTiles(path[i - 1], path[i]);
			const auto exit = DiagdirBetweenTiles(path[i], path[i + 1]);
			Track chosen = Track::Invalid;
			for (uint8_t value = 0; value < to_underlying(Track::End); ++value) {
				const Track candidate = static_cast<Track>(value);
				const auto direction = TrackEnterdirToTrackdir(candidate, entry);
				// The piece must admit this approach and lead to the next planned
				// cell; its display orientation alone is not evidence of connection.
				if (IsValidTrackdir(direction) && TrackdirToExitdir(direction) == exit) chosen = candidate;
			}
			REQUIRE(chosen != Track::Invalid);
			// Retain the exact original platform and its axis. A station cell is
			// already native paid infrastructure, never a newly funded rail piece.
			if (IsRailStationTile(path[i]) && GetStationIndex(path[i]) == project.origin && GetTileOwner(path[i]) == this->owner &&
				GetRailType(path[i]) == this->railtype && AxisToTrack(GetRailStationAxis(path[i])) == chosen) continue;
			// The final approach reuses the demo's already owned compatible piece.
			// It is real existing infrastructure, so neither a second construction
			// command nor a second capital charge belongs in the measured detour.
			if (IsPlainRailTile(path[i]) && GetTileOwner(path[i]) == this->owner && GetRailType(path[i]) == this->railtype && GetTrackBits(path[i]) == TrackBits{chosen}) continue;
			this->operations.push_back({path[i], chosen, false});
		}
	}
	/**
	 * Execute at most one ordinary funded command per150 actual simulation ticks.
	 * @param elapsed Native simulation frames; no budget, clock or money is written by this fixture.
	 * @return Whether every physical detour piece has been funded and installed.
	 */
	bool Tick(uint elapsed)
	{
		if (this->cursor == this->operations.size()) return true;
		this->ticks += elapsed;
		if (this->ticks < 150) return false;
		this->ticks -= 150;
		AutoRestoreBackup actor(_current_company, this->owner);
		const auto &operation = this->operations[this->cursor];
		const Money before = Company::Get(this->owner)->money;
		const auto result = operation.remove ?
			Command<Commands::RemoveRail>::Unsafe<CommandCallback>(INVALID_STRING_ID, nullptr, false, false, operation.tile, std::make_tuple(operation.tile, operation.track)) :
			Command<Commands::BuildRail>::Unsafe<CommandCallback>(INVALID_STRING_ID, nullptr, false, false, operation.tile, std::make_tuple(operation.tile, this->railtype, operation.track, false));
		INFO(fmt::format("paid haul route operation {} tile {} refusal {}", this->cursor, operation.tile.base(), result.Failed() ? GetString(result.GetErrorMessage()) : "none"));
		REQUIRE(result.Succeeded());
		CHECK(Company::Get(this->owner)->money == before - result.GetCost());
		this->cost += result.GetCost();
		++this->cursor;
		return this->cursor == this->operations.size();
	}
};

#endif /* TESTS_TBM_FEEDBACK_HAUL_FIXTURE_H */
