/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file tbm_area.cpp Area preview through the canonical planner before any command accepts it.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs4-tunnel-boring-machines.md, Group 4 (task 4.1)
 */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "tbm_fixture.h"
#include "../tbm.h"

#include "../safeguards.h"

/** A three-cell corridor straight out of the door, with the swept rectangle around it as accepted working space. */
class TBMAreaRig : public TBMAcquisitionRig {
public:
	std::vector<TileIndex> corridor; ///< Requested floor beyond the housing.
	TBMAreaRequest area; ///< Preview request for two Earth and two Rock hoppers.

	/** Build the outline through the real normalize/authorize owners. */
	TBMAreaRig()
	{
		const auto offset = TileIndexDiffCByDir(this->proof.exit_heading);
		const auto side = TileIndexDiffCByDir(ChangeDir(this->proof.exit_heading, DirDiff::Right90));
		std::vector<TileIndex> extra;
		// Rows from one behind the housing to one beyond the corridor end, each three cells wide.
		for (int along = -1; along <= 4; ++along) {
			for (int across = -1; across <= 1; ++across) {
				const auto tile = TileAddWrap(this->proof.destination, offset.x * along + side.x * across, offset.y * along + side.y * across);
				REQUIRE(IsValidTile(tile));
				const bool floor = across == 0 && along >= 1 && along <= 3;
				(floor ? this->corridor : extra).push_back(tile);
			}
		}
		const auto requested = NormalizeExcavationOutline(MapLayer::Underground, this->corridor);
		REQUIRE(requested.error == ExcavationError::None);
		this->area.outline = AuthorizeExcavationOutline(requested, extra, 0);
		REQUIRE(this->area.outline.error == ExcavationError::None);
		this->area.consist = {2, 2};
		this->area.model_id = 0;
		this->area.company = _current_company;
		this->area.origin = this->fixture.origin->index;
		this->area.start = {this->proof.destination, this->proof.exit_heading, std::vector<TileIndex>(4, this->proof.destination)};
	}
};

/** The preview reports passes, turn envelope, hardness, both spoils and support cost, and changes nothing. */
TEST_CASE("Area preview quotes the canonical plan for the actual consist", "[tbm-area]")
{
	TBMAreaRig rig;
	const auto before = rig.Digest();
	const auto preview = PreviewTBMArea(rig.area);
	REQUIRE(preview.consist_error == TBMAcquisitionError::None);
	REQUIRE(preview.error == ExcavationError::None);
	CHECK_FALSE(preview.partial);
	// The first cut's old/new 3x3 sweep opens corridor cells 1 and 2 together, so one more cut reaches cell 3.
	REQUIRE(preview.plan.route.size() == 2);
	CHECK(preview.plan.route[0].heading == rig.proof.exit_heading);
	CHECK(preview.plan.remaining_targets.empty());
	std::vector<TileIndex> swept;
	// Rebuild the swept union to cross-check every derived view of it.
	for (const auto &cut : preview.plan.route) swept.insert(swept.end(), cut.swept_tiles.begin(), cut.swept_tiles.end());
	std::ranges::sort(swept);
	swept.erase(std::unique(swept.begin(), swept.end()), swept.end());
	CHECK(std::ranges::find(preview.turn_envelope, rig.proof.destination) != preview.turn_envelope.end()); // The housing is cut but not requested floor.
	CHECK(preview.turn_envelope.size() + rig.corridor.size() == swept.size());
	const auto quote = QuoteExcavationCells(swept, 0);
	REQUIRE(quote.error == ExcavationError::None);
	CHECK(preview.totals.earth_t == quote.earth_t);
	CHECK(preview.totals.rock_t == quote.rock_t);
	CHECK(preview.totals.support_units == quote.support_units);
	CHECK(preview.totals.cost == quote.cost);
	CHECK(preview.hardness[0] + preview.hardness[1] + preview.hardness[2] + preview.hardness[3] == quote.new_cells);
	CHECK(preview.terms.earth_capacity_t == 128);
	CHECK(preview.terms.rock_capacity_t == 256);
	REQUIRE_FALSE(preview.terms.exchange_after.empty());
	CHECK(preview.terms.exchange_after.back() == 2); // The final return closes the route.
	CHECK(preview.proposal.route == preview.plan.route);
	CHECK(preview.proposal.requested_tiles == rig.area.outline.requested_tiles);
	CHECK(preview.proposal.commissioning_anchor == rig.proof.destination);
	CHECK(preview.prospective_job == ExcavationJobID{GetGeologyWorldState().next_job_id});
	CHECK(rig.Digest() == before);

	// The accepted preview is exactly what commissioning acquisition consumes.
	auto request = rig.fixture.request;
	request.proposal = preview.proposal;
	CHECK(PrepareTBMAcquisition(request, {2, 2}).error == TBMAcquisitionError::None);
	CHECK(rig.Digest() == before);
}

/** Each stage refuses before the next one runs, with the world unchanged. */
TEST_CASE("Area preview validates whole-unit capacity and mismatched starts", "[tbm-area]")
{
	TBMAreaRig rig;
	const auto before = rig.Digest();
	SECTION("a consist without a Rock wagon") {
		rig.area.consist = {2, 0};
		CHECK(PreviewTBMArea(rig.area).consist_error == TBMAcquisitionError::InvalidConsist);
	}
	SECTION("a start planned for a shorter consist") {
		rig.area.start.history.resize(3);
		const auto preview = PreviewTBMArea(rig.area);
		CHECK(preview.error == ExcavationError::InvalidRoute);
		CHECK(preview.failure_tile == rig.proof.destination);
	}
	SECTION("a sideways start out of the housing") {
		rig.area.start.heading = ChangeDir(rig.proof.exit_heading, DirDiff::Right90);
		CHECK(PreviewTBMArea(rig.area).error != ExcavationError::None);
	}
	SECTION("one hopper of each kind holds the reduced Soft pocket") {
		rig.area.consist = {1, 1};
		rig.area.start.history.resize(3, rig.proof.destination);
		// Halved Soft spoil now fits the smallest legal pair; zero Rock
		// capacity still refuses above, and independent limits are tested by passes.
		CHECK(PreviewTBMArea(rig.area).error == ExcavationError::None);
	}
	CHECK(rig.Digest() == before);
}
