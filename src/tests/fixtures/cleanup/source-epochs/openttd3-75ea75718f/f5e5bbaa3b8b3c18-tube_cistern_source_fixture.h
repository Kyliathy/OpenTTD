/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file tube_cistern_source_fixture.h Actual ordinary-loaded Oil/Salt source participants for the1980 Cistern precursor.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Group15DBB.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
 * This borrowed view installs no cargo registry and grants no stock. Prepared floor, completed machinery and Connected shafts remain explicit boundaries.
 */
#ifndef TESTS_TUBE_CISTERN_SOURCE_FIXTURE_H
#define TESTS_TUBE_CISTERN_SOURCE_FIXTURE_H
#include "tube_useful_tick_fixture.h"
#include "movement_fixture.h"
#include "../excavation_state.h"
#include "../multilayer_industry.h"
#include "../layer_industry_processor.h"
#include "../layer_portal_map.h"
#include "../layer_portal_cmd.h"
#include "../station_cmd.h"
#include "../station_func.h"
#include "../station_base.h"
#include "../industry.h"
#include "../industry_cmd.h"
#include "../industrytype.h"
#include "../company_func.h"
#include "../rail_cmd.h"
#include "../vehicle_cmd.h"
#include "../underground_vehicle.h"
#include "../table/strings.h"
#include "../order_cmd.h"
#include "../order_func.h"
#include "../depot_base.h"
#include "../industry_map.h"

/** @param tiles Exact test-owned rock floor; Surface construction remains native. Prepare only disclosed terrain through the actual geology restore boundary, retaining all original reserves/claims. */
inline void OpenCisternSourceFloor(std::span<const TileIndex> tiles)
{
	auto snapshot = CaptureGeologyState(); GeologyWorldState candidate;
	REQUIRE(PrepareGeologyRestoration(snapshot, candidate) == GeologyError::None);
	// The authored floor is a scenario boundary, never excavated by a station
	// or track command. Original geological reserves and active source claims
	// remain unchanged when their packed floor permission is prepared.
	for (const auto tile : tiles) {
		if (GetTileLayer(tile) == MapLayer::Surface) continue;
		REQUIRE(candidate.terrain.Set(tile, ExcavationState::Excavated));
	}
	snapshot.packed.assign(candidate.terrain.Data().begin(), candidate.terrain.Data().end());
	REQUIRE(PrepareGeologyRestoration(snapshot, candidate) == GeologyError::None);
	StageLoadedGeologyState(std::move(candidate)); AcceptLoadedGeologyState();
}

/** Borrowed actual native source world; normal application pools own every train, station, industry, packet and account. */
class CisternSourceStockWorld {
public:
	Train *oil_front = nullptr; ///< Original retained native locomotive, never respawned from the carrier input.
	Train *oil_tanker = nullptr; ///< Original native Oil wagon, legally refitted empty from CHEM to OIL_.
	Train *salt_front = nullptr; ///< Separate command-purchased conventional Underground/Surface front.
	Train *salt_wagon = nullptr; ///< Actual solid model5 Salt hold; no liquid capability is added.
	Industry *oil_well = nullptr; ///< Original Oil-well producer, retaining its native rate and threshold.
	Industry *chemicals = nullptr; ///< Actual all-input CHEM inventory/work/output owner.
	Industry *salt_mine = nullptr; ///< Actual finite SALT machinery, bound to its original geological reserve.
	Station *oil_pickup = nullptr; ///< Actual original Oil packet's first station.
	Station *chemical_berth = nullptr; ///< Genuine Surface Oil/Salt acceptance and later chemical output berth.
	Station *salt_pickup = nullptr; ///< Actual Underground Salt packet's first station.
	CargoType oil = INVALID_CARGO; ///< Actual registered original Oil meaning.
	CargoType salt = INVALID_CARGO; ///< Actual registered finite Salt meaning.
	CargoType chemical = INVALID_CARGO; ///< Actual distributed Liquid Chemicals meaning.
	GeologyDepositID deposit{UINT32_MAX}; ///< Original finite source identity, never an industry ID.
	std::array<TileIndex, 2> oil_depots{TileXYL(4, 20, MapLayer::Surface), TileXYL(32, 20, MapLayer::Surface)}; ///< Native source home/far service ends.
	std::array<TileIndex, 2> salt_depots{INVALID_TILE, TileXYL(23, 21, MapLayer::Surface)}; ///< Actual Underground home and Surface far service end.

	/** @param front Original stored empty native front from the accepted carrier world. @param tanker Its original coupled liquid wagon. @param oil_type Original Oil Wells definition verified by the carrier gate. @param chemical_type Native CHEM definition from that same accepted world. Fund/build the Surface source before finite Salt production can be accidentally advanced by Oil opportunities. */
	CisternSourceStockWorld(Train &front, Train &tanker, IndustryType oil_type, IndustryType chemical_type) : oil_front(&front), oil_tanker(&tanker)
	{
		REQUIRE(front.IsChainInDepot()); REQUIRE(tanker.First() == &front); REQUIRE(tanker.cargo.StoredCount() == 0);
		REQUIRE(front.tile == this->oil_depots[0]); REQUIRE(Industry::GetNumItems() == 0);
		this->oil = GetCargoTypeByLabel(CT_OIL); this->salt = GetCargoTypeByLabel(CargoLabel{"SALT"}); this->chemical = GetCargoTypeByLabel(CargoLabel{"CHEM"});
		REQUIRE(this->oil != INVALID_CARGO); REQUIRE(this->salt != INVALID_CARGO); REQUIRE(this->chemical != INVALID_CARGO);
		REQUIRE(GetIndustrySpec(oil_type)->name == STR_INDUSTRY_NAME_OIL_WELLS);
		REQUIRE(chemical_type == GetLayerProcessorIndustryType(MultiLayerRecipeID{"CHEM"}));
		const auto [refit, capacity, mail, capacities] = Command<Commands::RefitVehicle>::Do(DoCommandFlag::Execute, tanker.index, this->oil, 0, false, true, 1);
		INFO(fmt::format("original tanker Oil refit error={}", refit.GetErrorMessage())); REQUIRE(refit.Succeeded()); REQUIRE(tanker.cargo_type == this->oil);
		this->oil_well = this->BuildIndustry(oil_type, TileXYL(7, 14, MapLayer::Surface));
		this->chemicals = this->BuildIndustry(chemical_type, TileXYL(26, 15, MapLayer::Surface));
		REQUIRE(this->oil_well->produced[0].cargo == this->oil); REQUIRE(this->oil_well->produced[0].waiting == 0);
		const auto *recipe = GetLayerProcessorRecipe(*this->chemicals); REQUIRE(recipe != nullptr);
		REQUIRE(recipe->recipe.id == MultiLayerRecipeID{"CHEM"});
		this->oil_pickup = this->BuildStation(TileXYL(8, 20, MapLayer::Surface));
		this->chemical_berth = this->BuildStation(TileXYL(26, 20, MapLayer::Surface));
		REQUIRE(this->chemical_berth->goods[this->oil].status.Test(GoodsEntry::State::Acceptance));
		this->BuildDepot(this->oil_depots[1], DiagDirection::NE);
		// The two real platforms and service depots own their cells. Only the
		// connecting gaps receive paid track; no station identity is painted over.
		for (uint x = 5; x < 32; ++x) this->BuildTrack(TileXYL(x, 20, MapLayer::Surface), Track::X);
		this->SetOrders(front, this->oil_pickup->index, this->chemical_berth->index, this->oil_depots);
	}

	/** @param type Actual original/native Surface definition. @param tile Accepted native footprint anchor. @return Actual command-funded owner, completed only as the disclosed initial machinery boundary and still empty. */
	Industry *BuildIndustry(IndustryType type, TileIndex tile)
	{
		const auto cost = Command<Commands::BuildIndustry>::Do(DoCommandFlag::Execute, tile, type, 0, true, 123);
		INFO(fmt::format("Cistern source industry type={} tile={} error={}", type, tile.base(), cost.GetErrorMessage())); REQUIRE(cost.Succeeded());
		auto *industry = Industry::GetByTile(tile); REQUIRE(industry != nullptr); this->Complete(*industry); return industry;
	}

	/** @param industry Actual funded machinery. Completion prepares no raw/output cargo, changes no native rate and advances no work. */
	void Complete(Industry &industry)
	{
		// Only actual machinery cells belong to this completed boundary; unused
		// layout cells cannot become a stock, acceptance or construction shortcut.
		for (const auto tile : industry.location) if (industry.TileBelongsToIndustry(tile)) SetIndustryCompleted(tile);
	}

	/** @param tile Exact native two-cell X berth, including both real end approaches. @param join Optional existing same-layer station. @return Real owned station whose native catchment supplies acceptance. */
	Station *BuildStation(TileIndex tile, StationID join = NEW_STATION)
	{
		std::array<TileIndex, 4> floor;
		for (int dx = -1; dx <= 2; ++dx) floor[dx + 1] = TileAddWrap(tile, dx, 0);
		OpenCisternSourceFloor(floor);
		const auto cost = Command<Commands::BuildRailStation>::Do(DoCommandFlag::Execute, tile, RAILTYPE_RAIL, Axis::X, 1, 2, STAT_CLASS_DFLT, 0, join, false);
		INFO(fmt::format("Cistern source berth={} error={}", tile.base(), cost.GetErrorMessage())); REQUIRE(cost.Succeeded()); return Station::GetByTile(tile);
	}

	/** @param tile Actual paid track cell. @param track Native straight/curve geometry. Build only real gaps; existing platforms keep their native identities. */
	void BuildTrack(TileIndex tile, Track track)
	{
		if (IsRailStationTile(tile)) return;
		OpenCisternSourceFloor(std::array{tile});
		const auto cost = Command<Commands::BuildRail>::Do(DoCommandFlag::Execute, tile, RAILTYPE_RAIL, track, false);
		INFO(fmt::format("Cistern source track={} error={}", tile.base(), cost.GetErrorMessage())); REQUIRE(cost.Succeeded());
	}

	/** @param tile Actual native owned depot. @param direction Native exit direction. Build paid service/reversal machinery on disclosed prepared floor. */
	void BuildDepot(TileIndex tile, DiagDirection direction)
	{
		const auto exit = TileIndexDiffCByDiagDir(direction); OpenCisternSourceFloor(std::array{tile, TileAddWrap(tile, exit.x, exit.y)});
		const auto cost = Command<Commands::BuildRailDepot>::Do(DoCommandFlag::Execute, tile, RAILTYPE_RAIL, direction);
		INFO(fmt::format("Cistern source depot={} error={}", tile.base(), cost.GetErrorMessage())); REQUIRE(cost.Succeeded());
	}

	/** @param front Original actual front. @param pickup Real source packet station. @param drop Actual CHEM acceptance station. @param depots Native home/far service ends. Install normal loading/unloading/service orders without moving a part or skipping an arrival. */
	void SetOrders(Train &front, StationID pickup, StationID drop, const std::array<TileIndex, 2> &depots)
	{
		Order load; load.MakeGoToStation(pickup); load.SetUnloadType(OrderUnloadType::NoUnload);
		Order unload; unload.MakeGoToStation(drop); unload.SetLoadType(OrderLoadType::NoLoad);
		Order far; far.MakeGoToDepot(Depot::GetByTile(depots[1])->index, {OrderDepotTypeFlag::PartOfOrders}, OrderNonStopFlag::NonStop, {});
		Order home; home.MakeGoToDepot(Depot::GetByTile(depots[0])->index, {OrderDepotTypeFlag::PartOfOrders}, OrderNonStopFlag::NonStop, {});
		const std::array list{load, unload, far, home};
		// Native order ownership must retain every real service stop; a helper
		// cannot earn source delivery by selecting an already-arrived index.
		for (size_t index = 0; index < list.size(); ++index) REQUIRE(Command<Commands::InsertOrder>::Do(DoCommandFlag::Execute, front.index, static_cast<VehicleOrderID>(index), list[index]).Succeeded());
		ProcessOrders(&front);
	}

	/** @param id Original geological SALT identity after the bounded Oil opportunity. Fund the actual native mine and its public two-wide loading approach without creating reserve or cargo. */
	void PrepareSalt(GeologyDepositID id)
	{
		const auto *record = GetGeologyWorldState().deposits.Get(id); REQUIRE(record != nullptr);
		REQUIRE(record->resource == CargoLabel{"SALT"}); REQUIRE(record->layer == MapLayer::Underground); REQUIRE_FALSE(record->spans.empty());
		const uint x = record->spans.front().first_x, y = record->spans.front().y, column = x + 8;
		REQUIRE(x >= 5); REQUIRE(column < 62);
		const auto site = TileXYL(x, y, MapLayer::Underground);
		const auto type = GetLayerMineIndustryType(CargoLabel{"SALT"}); REQUIRE(type != IT_INVALID);
		const LayerMineRequest request{site, type, 0, id, _current_company};
		const auto geometry = ValidateMineDepositClaim(request);
		// Native catalogue footprints evolve with their actual machinery. Read
		// the public admission's complete proposed geometry, even when rock is
		// still solid, instead of trusting an older fixed-size fixture room.
		REQUIRE_FALSE(geometry.machinery.empty()); REQUIRE_FALSE(geometry.loading.empty());
		std::vector<TileIndex> floor = geometry.machinery; floor.insert(floor.end(), geometry.loading.begin(), geometry.loading.end());
		OpenCisternSourceFloor(floor);
		const auto admission = ValidateMineDepositClaim(request);
		INFO(fmt::format("native Salt admission error={} failure_tile={} machinery={} loading={} cost={} available={} towns={}", to_underlying(admission.error), admission.failure_tile.base(), admission.machinery.size(), admission.loading.size(), admission.cost.base(), GetAvailableMoneyForCommand().base(), Town::GetNumItems()));
		REQUIRE(admission.error == LayerIndustryError::None);
		// The accepted author input must fund this actual machinery. A funding
		// refusal is never repaired by rewriting loaded cash or source inventory.
		REQUIRE(admission.cost <= GetAvailableMoneyForCommand());
		uint row = 0;
		// The outer real loading row is the source station. A stale fixed row
		// could overlap machinery or omit a protected approach; no station is
		// allowed to manufacture its missing cavity during construction.
		for (const auto tile : admission.loading) row = std::max(row, TilePlaneY(tile));
		REQUIRE(row > 22); REQUIRE(row < 62);
		const auto cost = Command<Commands::BuildLayerMine>::Do(DoCommandFlag::Execute, site, type, 0, id, 123);
		INFO(fmt::format("Cistern finite Salt site={},{} error={}", x, y, cost.GetErrorMessage())); REQUIRE(cost.Succeeded());
		this->deposit = id; this->salt_mine = Industry::GetByTile(site); REQUIRE(this->salt_mine != nullptr); this->Complete(*this->salt_mine);
		this->salt_pickup = this->BuildStation(TileXYL(x, row, MapLayer::Underground));
		REQUIRE(this->BuildStation(TileXYL(26, 21, MapLayer::Surface), this->chemical_berth->index) == this->chemical_berth);
		REQUIRE(this->chemical_berth->goods[this->salt].status.Test(GoodsEntry::State::Acceptance));
		this->salt_depots[0] = TileXYL(x - 4, row, MapLayer::Underground);
		this->BuildDepot(this->salt_depots[0], DiagDirection::SW); this->BuildDepot(this->salt_depots[1], DiagDirection::SW);
		const auto lower = TileXYL(x + 6, row, MapLayer::Underground); this->PreparePortal(lower);
		// This independent conventional route passes east of the Oil railway.
		// Its actual upper/right curves, shaft and same-layer shared acceptance
		// berth must remain physical; catchment cannot bridge the rock layer.
		for (uint rail_x = x - 3; rail_x < x + 6; ++rail_x) this->BuildTrack(TileXYL(rail_x, row, MapLayer::Underground), Track::X);
		this->BuildTrack(TileXYL(x + 7, row, MapLayer::Surface), Track::X);
		this->BuildTrack(TileXYL(column, row, MapLayer::Surface), Track::Upper);
		for (uint rail_y = 22; rail_y < row; ++rail_y) this->BuildTrack(TileXYL(column, rail_y, MapLayer::Surface), Track::Y);
		this->BuildTrack(TileXYL(column, 21, MapLayer::Surface), Track::Right);
		for (uint rail_x = 24; rail_x < column; ++rail_x) this->BuildTrack(TileXYL(rail_x, 21, MapLayer::Surface), Track::X);
		this->salt_front = this->Buy(this->salt_depots[0], GetUndergroundEngine(4));
		this->salt_wagon = this->Buy(this->salt_depots[0], GetUndergroundEngine(5), this->salt);
		REQUIRE(Command<Commands::MoveRailVehicle>::Do(DoCommandFlag::Execute, this->salt_wagon->index, this->salt_front->index, false).Succeeded());
		this->SetOrders(*this->salt_front, this->salt_pickup->index, this->chemical_berth->index, this->salt_depots);
	}

	/** @param lower Actual Underground conventional housing. Install only the disclosed reciprocal commissioned boundary; no TBM work or source cargo is claimed. */
	void PreparePortal(TileIndex lower)
	{
		const auto direction = DiagDirection::SW, door = ReverseDiagDir(direction);
		const auto delta = TileIndexDiffCByDiagDir(door); OpenCisternSourceFloor(std::array{lower, TileAddWrap(lower, delta.x, delta.y)});
		const auto upper = MultiLayerMap::ProjectToSurface(lower);
		MakeRailLayerPortal(lower, _current_company, direction, MapLayer::Surface, RAILTYPE_RAIL);
		MakeRailLayerPortal(upper, _current_company, ReverseDiagDir(direction), MapLayer::Underground, RAILTYPE_RAIL);
		auto records = CaptureLayerPortalWorkRecords(); LayerPortalWorkRecord pair;
		pair.pair_key = upper; pair.company = _current_company; pair.state = LayerPortalWorkState::Connected;
		records.push_back(pair); std::ranges::sort(records, {}, &LayerPortalWorkRecord::pair_key);
		REQUIRE(ValidateLayerPortalWorkRecords(records, GetCurrentMultiLayerProfileSelection(), GetGeologyWorldState()) == LayerPortalWorkError::None);
		StageLoadedLayerPortalWork(std::move(records)); AcceptLoadedLayerPortalWork();
	}

	/** @param depot Actual owned depot. @param engine Original/native mapped engine identity. @param cargo Optional exact existing refit meaning. @return Real command-purchased part retaining native position, capacity and owner. */
	Train *Buy(TileIndex depot, EngineID engine, CargoType cargo = INVALID_CARGO)
	{
		const auto [cost, id, capacity, mail, capacities] = Command<Commands::BuildVehicle>::Do(DoCommandFlag::Execute, depot, engine, false, cargo, ClientID::Invalid);
		INFO(fmt::format("Cistern source purchase engine={} error={}", engine.base(), cost.GetErrorMessage())); REQUIRE(cost.Succeeded()); return Train::Get(id);
	}

	/** Advance one complete native clock/vehicle opportunity while every newly purchased source front remains genuinely player-stopped. */
	void RunStopped()
	{
		RunUsefulTubeNativeOpportunity();
		// The ordinary game loop replenishes company construction limits once
		// per opportunity. Source preparation must earn that native recovery
		// while time passes, rather than rewrite a loaded clearing budget.
		UpdateLandscapingLimits();
		REQUIRE(this->oil_front->vehstatus.Test(VehState::Stopped)); REQUIRE(this->oil_front->IsChainInDepot());
		REQUIRE(IsTrainOnItsTiles(this->oil_front)); REQUIRE(this->oil_tanker->cargo.StoredCount() == 0);
		// Salt does not exist during the bounded original Oil opportunity; after
		// native acquisition it shares every real opportunity without moving or
		// loading before the source stock handoff is accepted.
		if (this->salt_front != nullptr) {
			REQUIRE(this->salt_front->vehstatus.Test(VehState::Stopped)); REQUIRE(this->salt_front->IsChainInDepot());
			REQUIRE(IsTrainOnItsTiles(this->salt_front)); REQUIRE(this->salt_wagon->cargo.StoredCount() == 0);
		}
	}
};
#endif /* TESTS_TUBE_CISTERN_SOURCE_FIXTURE_H */
