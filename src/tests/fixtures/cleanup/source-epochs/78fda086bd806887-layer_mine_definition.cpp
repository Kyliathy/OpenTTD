/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file layer_mine_definition.cpp Exact installed ten-resource catalogue and genuine native layouts/funding. @see docs/zz-reach2/upgrades/2026-10/r2mlvs6-mines-and-resource-chains.md, Group 9 */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "mine_fixture.h"
#include "../core/backup_type.hpp"

#include "../safeguards.h"

/** Shared world fixture installs the real catalogue over the same native IIDS identity and opens its actual manifest footprint. */
class MineDefinitionRig : public MineAdmissionRig {
	decltype(_industile_mngr.mappings) tile_mappings = _industile_mngr.mappings; ///< Restore caller TIDS identities after catalogue checks.

public:
	/** @param index Published resource whose real installed definition will be funded. */
	explicit MineDefinitionRig(size_t index = 0) : MineAdmissionRig(index)
	{
		const auto geology = ComputeGeologyStateDigest(GetGeologyWorldState());
		REQUIRE(InitialiseLayerMineDefinitions(this->ActivatedContent()));
		CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == geology);
		this->request.type = GetLayerMineIndustryType(GetMultiLayerMineRecipes()[index].resource);
		REQUIRE(this->request.type != IT_INVALID);
		this->site.clear();
		std::vector<TileIndex> missing_floor;
		// Nine manifest machinery cells plus four separate loading cells are admitted by the same whole-site funding owner.
		for (int y = 0; y < 5; ++y) {
			for (int x = 0; x < (y < 3 ? 3 : 2); ++x) {
				const TileIndex tile = TileAddWrap(this->request.tile, x, y);
				REQUIRE(IsValidTile(tile));
				this->site.push_back(tile);
				if (GetGeologyWorldState().terrain.Get(tile) != ExcavationState::Excavated) missing_floor.push_back(tile);
			}
		}
		if (!missing_floor.empty()) this->Open(missing_floor);
	}

	/** Restore actual caller tile identities; the parent destroys fixture industries before resetting their definitions. */
	~MineDefinitionRig() { _industile_mngr.mappings = std::move(this->tile_mappings); }
};

/** Existing IIDS/TIDS mappings and cargo labels resolve all ten definitions without any geology/industry generation. */
TEST_CASE("Native mine catalogue registers all ten exact resources and art families", "[layer-mine-definition]")
{
	MineDefinitionRig rig;
	std::set<IndustryType> types;
	std::set<StringID> names;
	std::set<std::string_view> assets;
	const auto recipes = GetMultiLayerMineRecipes();
	// Every resource retains a distinct actual runtime industry and logical art family, with no alias to a fallback name or cargo slot.
	for (size_t index = 0; index < recipes.size(); ++index) {
		const auto &record = recipes[index];
		const IndustryType type = GetLayerMineIndustryType(record.resource);
		REQUIRE(type != IT_INVALID);
		CHECK(types.insert(type).second);
		const IndustrySpec *spec = GetIndustrySpec(type);
		const auto *capability = GetLayerIndustryCapability(type);
		REQUIRE(capability != nullptr);
		CHECK(capability->resource == record.resource);
		CHECK(capability->layers.Test(record.layer));
		CHECK(capability->introduction_year == record.introduction_year);
		CHECK(capability->equipment_cost_factor == (record.layer == MapLayer::DeepUnderground ? 2 : 1));
		CHECK(assets.insert(capability->asset_id).second);
		CHECK(capability->asset_id == fmt::format("mine.{}", record.resource.AsString()));
		CHECK(names.insert(spec->name).second);
		CHECK(spec->produced_cargo[0] == GetCargoTypeByLabel(record.resource));
		CHECK(spec->layouts.front().size() == 9);
		// All nine native tiles keep one declared saved identity and a visible original art substitute until the r2mlvs13 family is packed.
		for (const auto &cell : spec->layouts.front()) {
			CHECK(cell.gfx >= NEW_INDUSTRYTILEOFFSET);
			const IndustryTileSpec *tile = GetIndustryTileSpec(cell.gfx);
			CHECK(tile->enabled);
			CHECK(tile->grf_prop.subst_id < NEW_INDUSTRYTILEOFFSET);
			CHECK(tile->grf_prop.local_id >= 0x8100 + index * 16);
			CHECK(tile->grf_prop.local_id < 0x8100 + index * 16 + 9);
		}
		CHECK_FALSE(CheckLayerIndustryConstruction(MultiLayerMap::ProjectToSurface(rig.request.tile), type).Succeeded());
	}
	const auto old_types = types;
	REQUIRE(InitialiseLayerMineDefinitions(rig.ActivatedContent()));
	CHECK_FALSE(InitialiseLayerMineDefinitions(GRFConfigList{}));
	CHECK(GetLayerMineIndustryType(CargoLabel{"ERTH"}) == IT_INVALID);
	CHECK(Industry::GetNumItems() == 0);
	// Re-polishing definitions must reuse each saved package-local mapping rather than allocating a second native catalogue.
	for (const auto &record : recipes) CHECK(old_types.contains(GetLayerMineIndustryType(record.resource)));
}

/** Actual installed native footprints fund and extract by the common finite path; Deep remains calendar-gated. */
TEST_CASE("Each installed mine funds actual machinery and uses common finite production", "[layer-mine-definition]")
{
	const size_t index = GENERATE(0, 1, 2, 3, 4, 5, 6, 7, 8, 9);
	MineDefinitionRig rig(index);
	const auto &recipe = GetMultiLayerMineRecipes()[index];
	rig.PrepareFunding();
	REQUIRE(rig.Fund(DoCommandFlag::Execute).Succeeded());
	Industry *industry = *Industry::Iterate().begin();
	CHECK(industry->location.w == 3);
	CHECK(industry->location.h == 3);
	// Construction completion touches only the real machinery; the extra loading floor is preserved independently.
	for (TileIndex tile : rig.site) {
		if (industry->TileBelongsToIndustry(tile)) SetIndustryCompleted(tile);
	}
	_game_mode = GameMode::Normal;
	_pause_mode = {};
	AutoRestoreBackup fluct(_economy.fluct, int16_t{1});
	AutoRestoreBackup scale(_settings_game.economy.industry_cargo_scale, uint16_t{100});
	const auto reserve = GetGeologyWorldState().deposits.Get(rig.request.deposit)->remaining_reserve;
	CHECK(ProduceLayerMineCargo(*industry, 74, true).amount == recipe.recipe.outputs[0].quantity);
	CHECK(GetGeologyWorldState().deposits.Get(rig.request.deposit)->remaining_reserve + industry->produced[0].waiting == reserve);
	CHECK(GetLayerMineState(industry->index)->deposit == rig.request.deposit);
	if (recipe.layer == MapLayer::DeepUnderground) {
		TimerGameCalendar::date = TimerGameCalendar::ConvertYMDToDate(TimerGameCalendar::Year{1979}, 11, 31);
		CHECK(ProduceLayerMineCargo(*industry, 74, true).error == LayerIndustryError::Unavailable);
	}
}
