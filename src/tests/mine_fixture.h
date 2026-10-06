/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file mine_fixture.h Shared native mine funding, geology and catchment fixture. @see docs/zz-reach2/upgrades/2026-10/r2mlvs6-mines-and-resource-chains.md, Groups 3-8 */

#ifndef TESTS_MINE_FIXTURE_H
#define TESTS_MINE_FIXTURE_H

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "excavation_fixture.h"
#include "../multilayer_industry.h"
#include "../industry.h"
#include "../industrytype.h"
#include "../newgrf_commons.h"
#include "../industry_cmd.h"
#include "../landscape_cmd.h"
#include "../town.h"
#include "../economy_func.h"
#include "../command_func.h"
#include "../station_map.h"
#include "../tile_cmd.h"

#include "../safeguards.h"

extern const IndustrySpec _origin_industry_specs[NEW_INDUSTRYOFFSET]; ///< Unmodified original definitions used only for fixture installation.

/** Real profile/geology fixture with one native IIDS definition, without funding or fabricating reserve authority. */
class MineAdmissionRig : public ExcavationFixture {
	decltype(_industry_mngr.mappings) mappings = _industry_mngr.mappings; ///< Caller identity mapping restored on teardown.
	Money industry_price = _price[Price::BuildIndustry]; ///< Caller machinery price restored after command fixtures.
	Money raw_price = _price[Price::BuildIndustryRaw]; ///< Caller alternative raw-industry price.
public:
	LayerMineRequest request; ///< Actual generated seam and fixture-installed native definition.
	std::vector<TileIndex> site; ///< Complete two-by-two machinery and two-by-two loading footprint.

	/**
	 * Install a native definition through IIDS and select its actual generated geological seam.
	 * @param index Content-owned mine recipe index, including the five Deep definitions.
	 * @param serialized_native_slots Match the distributed custom-cargo layout for full game-writer fixtures.
	 */
	explicit MineAdmissionRig(size_t index = 0, bool serialized_native_slots = false) : ExcavationFixture(serialized_native_slots)
	{
		REQUIRE(Industry::GetNumItems() == 0);
		ResetIndustries();
		ResetLayerIndustryState();
		_price[Price::BuildIndustry] = 10000;
		_price[Price::BuildIndustryRaw] = 10000;
		const auto &mine = GetMultiLayerMineRecipes()[index];
		IndustrySpec spec = _origin_industry_specs[0];
		spec.layouts = {{{{0, 0}, 0}, {{1, 0}, 0}, {{0, 1}, 0}, {{1, 1}, 0}}};
		spec.callback_mask = {};
		spec.behaviour = {};
		spec.accepts_cargo.fill(INVALID_CARGO);
		spec.produced_cargo.fill(INVALID_CARGO);
		spec.produced_cargo[0] = GetCargoTypeByLabel(mine.resource);
		spec.grf_prop = SubstituteGRFFileProps(0);
		spec.grf_prop.grfid = GetMultiLayerContentProfile(MultiLayerGameplayProfileID::MultilayerIndustry)->required_content->package.grfid;
		spec.grf_prop.local_id = LAYER_MINE_LOCAL_BASE + static_cast<uint16_t>(index);
		spec.grf_prop.override_id = IT_INVALID;
		_industry_mngr.SetEntitySpec(std::move(spec));
		this->request.type = _industry_mngr.GetID(LAYER_MINE_LOCAL_BASE + static_cast<uint16_t>(index),
			GetMultiLayerContentProfile(MultiLayerGameplayProfileID::MultilayerIndustry)->required_content->package.grfid);
		REQUIRE(this->request.type != IT_INVALID);
		const auto records = GetGeologyWorldState().deposits.Records();
		const auto found = std::ranges::find(records, mine.resource, &GeologyDeposit::resource);
		REQUIRE(found != records.end());
		this->request.deposit = found->id;
		this->request.company = _current_company;
		this->request.tile = TileXYL(found->spans.front().first_x, found->spans.front().y, mine.layer);
		// Build the complete fixture floor explicitly; a later missing corner must fail before any claim/tile mutation.
		for (int y = 0; y < 4; ++y) {
			for (int x = 0; x < 2; ++x) this->site.push_back(TileAddWrap(this->request.tile, x, y));
		}
		this->Open(this->site);
	}

	/** Remove funded native industries and restore caller definitions and identities before geological teardown. */
	~MineAdmissionRig()
	{
		// Fixture teardown removes its real industries before geology/company destruction, so claims cannot outlive their owners.
		for (Industry *industry : Industry::Iterate()) delete industry;
		_station_pool.CleanPool();
		_town_pool.CleanPool();
		RebuildTownKdtree();
		_price[Price::BuildIndustry] = this->industry_price;
		_price[Price::BuildIndustryRaw] = this->raw_price;
		ResetLayerIndustryState();
		ResetIndustries();
		_industry_mngr.mappings = std::move(this->mappings);
	}

	/**
	 * Replace one test-owned geological authority through its validated load boundary.
	 * @param snapshot Explicit fixture state; never a gameplay mutation path.
	 */
	void RestoreGeology(const GeologyStateSnapshot &snapshot)
	{
		GeologyWorldState state;
		REQUIRE(PrepareGeologyRestoration(snapshot, state) == GeologyError::None);
		StageLoadedGeologyState(std::move(state));
		AcceptLoadedGeologyState();
	}

	/** Create the actual Surface naming town and suppress opening news during isolated command-body fixtures. */
	void PrepareFunding()
	{
		REQUIRE(Town::GetNumItems() == 0);
		REQUIRE(Town::CanAllocateItem());
		Town::Create(TileXYL(10, 10, MapLayer::Surface));
		RebuildTownKdtree();
		_game_mode = GameMode::Editor;
	}

	/**
	 * Call the actual seam-specific funding command with this fixture's selected definition/site/deposit.
	 * @param flags Estimate or execution.
	 * @return Full authoritative command result; the fixture does not fabricate placement or a claim.
	 */
	CommandCost Fund(DoCommandFlags flags) const
	{
		return CmdBuildLayerMine(flags, this->request.tile, this->request.type, 0, this->request.deposit, 123);
	}

	/**
	 * Prepare a real one-cell railway station and rebuild its actual layer-local catchment.
	 * @param tile Fixture-owned world tile, outside the machinery/loading cells.
	 * @return Pool-owned station; fixture teardown removes it after the industries.
	 */
	Station *StationAt(TileIndex tile)
	{
		REQUIRE(Station::CanAllocateItem());
		if (GetTileLayer(tile) != MapLayer::Surface) this->Open(std::array{tile});
		Station *station = Station::Create(tile);
		station->owner = _current_company;
		station->town = *Town::Iterate().begin();
		station->facilities.Set(StationFacility::Train);
		MakeRailStation(tile, _current_company, station->index, Axis::X, 0, RAILTYPE_RAIL);
		station->train_station = TileArea(tile, 1, 1);
		station->spread.Add(tile);
		station->RecomputeCatchment();
		return station;
	}
};


#endif /* TESTS_MINE_FIXTURE_H */
