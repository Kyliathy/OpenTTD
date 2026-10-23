/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file transport_tube_useful_routes.cpp Autonomous1970 mine/capsule/conventional Surface intake through original native owners.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Groups15BB/15BC.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
 * Prepared excavated floor, completed machinery and commissioned reciprocal portals remain disclosed route-boundary inputs;
 * Group16 owns actual TBM excavation/bootstrap. No local vehicle arrival, cargo injection or order skip is supplied here.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "mock_environment.h"
#include "layer_window_fixture.h"
#include "layer_save_fixture.h"
#include "test_artifact_paths.h"
#include "tube_useful_tick_fixture.h"
#include "underground_fleet_fixture.h"
#include "movement_fixture.h"
#include "../transport_tube.h"
#include "../layer_industry_processor.h"
#include "../order_cmd.h"
#include "../order_func.h"
#include "../signal_func.h"
#include "../timer/timer_manager.h"
#include "../saveload/saveload_func.h"
#include "../gfx_func.h"
#include "../table/sprites.h"
#include "../safeguards.h"

/** Actual native1970 facilities and fronts; all movement, loading, transfers and final payment remain native-owned. */
class MoleUsefulRouteRig : public MineAdmissionRig {
	decltype(_engine_mngr.mappings) engines = _engine_mngr.mappings; ///< Restore original exact native engine identity scope after actual vehicles retire.
	decltype(_industile_mngr.mappings) industry_tiles = _industile_mngr.mappings; ///< Native processor definition scope, separate from actual material owners.
	PathfinderDefaults pathfinder; ///< Legal native budgets/retry settings for real autonomous movement.
	AutoRestoreBackup<AccelerationModel> acceleration{_settings_game.vehicle.train_acceleration_model, AccelerationModel::Realistic}; ///< Real native physical braking/traction.
	AutoRestoreBackup<uint8_t> freight_multiplier{_settings_game.vehicle.freight_trains, uint8_t{1}}; ///< Legal Coal mass, preserved by later ordinary loading.
	AutoRestoreBackup<UnitID> train_limit{_settings_game.vehicle.max_trains, UnitID{500}}; ///< Normal command admission for the two original fronts.
	AutoRestoreBackup<uint8_t> length_limit{_settings_game.vehicle.max_train_length, uint8_t{7}}; ///< Native two-part conventional train is not an oversize exception.
	AutoRestoreBackup<bool> distant{_settings_game.station.distant_join_stations, true}; ///< Explicit ordinary joined-station setting for separated modal berths.
	AutoRestoreBackup<bool> paths{_settings_game.pf.reserve_paths, true}; ///< Real depot departure and exclusive native path custody.
	AutoRestoreBackup<uint8_t> retry{_settings_game.pf.wait_for_pbs_path, uint8_t{2}}; ///< Legal saved retry minimum, not a fixture-only zero value.
	AutoRestoreBackup<int16_t> fluctuation{_economy.fluct, int16_t{1}}; ///< Ordinary non-recession finite production opportunity.
	AutoRestoreBackup<uint16_t> industry_scale{_settings_game.economy.industry_cargo_scale, uint16_t{100}}; ///< Normal native recipe scale.
	AutoRestoreBackup<uint8_t> indicators{_settings_client.gui.loading_indicators, uint8_t{0}}; ///< Suppress optional viewport text only, without altering native loading.
	AutoRestoreBackup<uint64_t> epoch{TimerGameTick::counter, uint64_t{0}}; ///< Isolated synchronized route epoch, advanced only by common opportunities.
	AutoRestoreBackup<TimerGameEconomy::Date> economy_date{TimerGameEconomy::date}; ///< Restore caller calendar/economy owners after real timer advancement.
	AutoRestoreBackup<TimerGameEconomy::DateFract> economy_fraction{TimerGameEconomy::date_fract}; ///< Daily vehicle work must advance across all native pool IDs.
	AutoRestoreBackup<TimerGameEconomy::Year> economy_year{TimerGameEconomy::year}; ///< Exact caller economy calendar metadata.
	AutoRestoreBackup<TimerGameEconomy::Month> economy_month{TimerGameEconomy::month}; ///< Exact caller economy month.
	AutoRestoreBackup<uint> economy_days{TimerGameEconomy::days_since_last_month}; ///< Native month bookkeeping remains timer-owned.
	AutoRestoreBackup<TimerGameCalendar::DateFract> calendar_fraction{TimerGameCalendar::date_fract}; ///< Restore caller calendar fraction; base fixture restores its date.
	AutoRestoreBackup<TimerGameCalendar::Year> calendar_year{TimerGameCalendar::year}; ///< Exact caller calendar year.
	AutoRestoreBackup<TimerGameCalendar::Month> calendar_month{TimerGameCalendar::month}; ///< Exact caller calendar month.
	AutoRestoreBackup<Money> train_price{_price[Price::BuildVehicleTrain], Money{10000}}; ///< Explicit positive fixture prices, not balance acceptance.
	AutoRestoreBackup<Money> wagon_price{_price[Price::BuildVehicleWagon], Money{1000}}; ///< Native wagon purchase must debit real company funds.
	AutoRestoreBackup<Money> rail_price{_price[Price::BuildRail], Money{100}}; ///< Every actual guideway cell has a positive command cost.
	AutoRestoreBackup<Money> depot_price{_price[Price::BuildDepotTrain], Money{500}}; ///< Both modal depot families use normal paid commands.
	AutoRestoreBackup<Money> station_price{_price[Price::BuildStationRail], Money{500}}; ///< Actual native station footprints remain paid.
public:
	Industry *mine = nullptr; ///< Command-funded finite Coal mine, with the original generated reserve claim.
	Industry *factory = nullptr; ///< Command-funded real CFMT Surface receiver of useful Coal input.
	Station *origin = nullptr; ///< Actual mine-side loading/packet owner.
	Station *interchange = nullptr; ///< Exact joined tube/conventional packet owner; no cross-layer catchment shortcut.
	Station *destination = nullptr; ///< Actual accepted Surface factory berth.
	Train *capsule = nullptr; ///< Real purchased1970 Mole, never relocated from its purchase depot.
	Train *freight = nullptr; ///< Real1950 conventional locomotive; only this mode traverses the portal.
	Train *wagon = nullptr; ///< Actual single receiving conventional Coal hold.
	RailType tube = INVALID_RAILTYPE; ///< Dedicated canonical native rail slot.
	UtilityProducerID source{UINT32_MAX}; ///< Sole original paid external producer; no native geothermal prerequisite.
	CargoType cargo = INVALID_CARGO; ///< Exact native Coal meaning, not a fixture-invented label.
	std::array<TileIndex, 2> tube_depots{}; ///< Actual physical route ends, used by normal service orders to reverse safely.
	std::array<TileIndex, 2> freight_depots{}; ///< Actual Underground/Surface conventional ends beyond their stations.
	TileIndex conventional_portal = INVALID_TILE; ///< Explicit commissioned conventional inter-layer transition.
	uint supplied = 0; ///< Observed real dispatch receipt; never a replacement cargo buffer.
	uint production_opportunities = 0; ///< Bounded original mine work opportunities before the one measured shipment.

	/** Fund all native1970 participants and command-build their complete routes on explicitly prepared floor. */
	MoleUsefulRouteRig() : MineAdmissionRig(0, true)
	{
		// The base prepares only immutable definitions/geology/floor. Every
		// actual dated purchase/funding below occurs at the genuine model date.
		TimerGameCalendar::SetDate(TimerGameCalendar::ConvertYMDToDate(TimerGameCalendar::Year{1970}, 0, 1), 0);
		TimerGameEconomy::SetDate(TimerGameEconomy::ConvertYMDToDate(TimerGameEconomy::Year{1970}, 0, 1), 0);
		this->PrepareFunding();
		// Ordinary town loading validates the original generator identity.
		// Supply the same native naming metadata as other full-world fixtures;
		// this does not create houses, cargo, money or a geological claim.
		(*Town::Iterate().begin())->townnametype = SPECSTR_TOWNNAME_START;
		REQUIRE(Command<Commands::BuildLayerMine>::Do(DoCommandFlag::Execute, this->request.tile, this->request.type, 0, this->request.deposit, 123).Succeeded());
		this->mine = *Industry::Iterate().begin(); this->Complete(*this->mine);
		_game_mode = GameMode::Normal; _pause_mode = {};
		ResetRailTypes(); ResetRoadTypes(); StationClass::Reset(); RebuildStationKdtree();
		ResetTubePowerState(); ResetUtilityPowerState();
		_engine_mngr.ResetToDefaultMapping(); SetupEngines();
		REQUIRE(InitialiseTBMEngineDefinitions(this->ActivatedContent()));
		REQUIRE(InitialiseUndergroundEngineDefinitions(this->ActivatedContent()));
		this->tube = InitialiseTubeRailType(this->ActivatedContent()); REQUIRE(this->tube != INVALID_RAILTYPE);
		REQUIRE(InitialiseTubeEngineDefinitions(this->ActivatedContent()));
		Company::Get(_current_company)->avail_railtypes.Set(this->tube);
		Engine::Get(GetTubeEngine(9))->company_avail.Set(_current_company);
		Engine::Get(GetUndergroundEngine(4))->company_avail.Set(_current_company);
		Engine::Get(GetUndergroundEngine(5))->company_avail.Set(_current_company);
		this->cargo = GetCargoTypeByLabel(CT_COAL); REQUIRE(this->cargo != INVALID_CARGO);
		REQUIRE(this->mine->produced[0].cargo == this->cargo);
		CargoSpec::Get(this->cargo)->weight = 16; CargoSpec::Get(this->cargo)->is_freight = true;
		CargoSpec::Get(this->cargo)->current_payment = Money{1000};
		Company::Get(_current_company)->clear_limit = 512U << 16;
		// TilePlaneY is the layer-local row. Physical stacked TileY would
		// address a different plane when passed back to the explicit TileXYL API.
		const int x = TileX(this->request.tile), y = TilePlaneY(this->request.tile) + 2;
		fmt::print("MOLE_ROUTE_SITE mine={},{} date=1970\n", x, y - 2);
		REQUIRE(x >= 5); REQUIRE(x <= 55); REQUIRE(y >= 4); REQUIRE(y <= 59);
		const int dx = x <= 32 ? 1 : -1, dy = y <= 32 ? 1 : -1;
		const int transfer_x = x + dx * 12;
		const auto origin_tile = TileXYL(x, y, MapLayer::Underground);
		const auto transfer_tile = TileXYL(transfer_x, y, MapLayer::Underground);
		this->tube_depots = {TileXYL(x - dx * 4, y, MapLayer::Underground), TileXYL(transfer_x + dx * 5, y, MapLayer::Underground)};
		this->origin = this->StationAtRoute(origin_tile, this->tube, Axis::X);
		this->interchange = this->StationAtRoute(transfer_tile, this->tube, Axis::X);
		this->DepotAt(this->tube_depots[0], this->tube, dx > 0 ? DiagDirection::SW : DiagDirection::NE);
		this->DepotAt(this->tube_depots[1], this->tube, dx > 0 ? DiagDirection::NE : DiagDirection::SW);
		// Build every real linear cell, preserving actual station/depot owners.
		// Both service ends are beyond the loading berths, so native orders alone
		// can reverse and complete an empty return without a pose/order override.
		for (int col = static_cast<int>(std::min(TileX(this->tube_depots[0]), TileX(this->tube_depots[1]))); col <= static_cast<int>(std::max(TileX(this->tube_depots[0]), TileX(this->tube_depots[1]))); ++col) {
			this->TrackAt(TileXYL(col, y, MapLayer::Underground), this->tube, Track::X);
		}
		const int rail_x = transfer_x + 3;
		const int berth_y = y + dy * 4;
		const int portal_y = y + dy * 10;
		const auto freight_berth = TileXYL(rail_x, std::min(berth_y, berth_y + dy), MapLayer::Underground);
		REQUIRE(this->StationAtRoute(freight_berth, RAILTYPE_RAIL, Axis::Y, this->interchange->index) == this->interchange);
		this->freight_depots = {TileXYL(rail_x, y + dy * 2, MapLayer::Underground), TileXYL(rail_x, portal_y + dy * 9, MapLayer::Surface)};
		this->DepotAt(this->freight_depots[0], RAILTYPE_RAIL, dy > 0 ? DiagDirection::SE : DiagDirection::NW);
		this->DepotAt(this->freight_depots[1], RAILTYPE_RAIL, dy > 0 ? DiagDirection::NW : DiagDirection::SE);
		this->conventional_portal = TileXYL(rail_x, portal_y, MapLayer::Underground);
		this->PortalAt(this->conventional_portal, dy > 0 ? DiagDirection::SE : DiagDirection::NW);
		REQUIRE(InitialiseLayerProcessorDefinitions(this->ActivatedContent()));
		const int factory_y = portal_y + dy * 5;
		const auto receiver = TileXYL(rail_x + 3, std::min(factory_y, factory_y + dy), MapLayer::Surface);
		REQUIRE(Command<Commands::BuildIndustry>::Do(DoCommandFlag::Execute, receiver, GetLayerProcessorIndustryType(MultiLayerRecipeID{"CFMT"}), 0, true, 123).Succeeded());
		this->factory = Industry::Get(GetIndustryIndex(receiver)); this->Complete(*this->factory);
		this->destination = this->StationAtRoute(TileXYL(rail_x, std::min(factory_y, factory_y + dy), MapLayer::Surface), RAILTYPE_RAIL, Axis::Y);
		REQUIRE(this->destination->goods[this->cargo].status.Test(GoodsEntry::State::Acceptance));
		// Each conventional segment ends at the real shaft or its paid depot.
		// No cell bridges the stacked coordinate gap, and no tube crosses it.
		for (int row = y + dy * 3; row != portal_y; row += dy) this->TrackAt(TileXYL(rail_x, row, MapLayer::Underground), RAILTYPE_RAIL, Track::Y);
		for (int row = portal_y + dy; row != portal_y + dy * 9; row += dy) this->TrackAt(TileXYL(rail_x, row, MapLayer::Surface), RAILTYPE_RAIL, Track::Y);
		const auto power_portal = TileAddWrap(this->tube_depots[0], 0, -2);
		this->PortalAt(power_portal, DiagDirection::NW);
		const std::array wire{TileAddWrap(power_portal, 0, 1), this->tube_depots[0]};
		this->Open(wire);
		const auto connection = ConnectTubeExternalPower(power_portal, this->tube_depots[0], wire);
		REQUIRE(connection.error == TubePowerError::None); this->source = connection.producer;
		this->capsule = this->BuyFront(this->tube_depots[0], GetTubeEngine(9));
		this->freight = this->BuyFront(this->freight_depots[0], GetUndergroundEngine(4));
		this->wagon = this->BuyFront(this->freight_depots[0], GetUndergroundEngine(5), this->cargo);
		REQUIRE(Command<Commands::MoveRailVehicle>::Do(DoCommandFlag::Execute, this->wagon->index, this->freight->index, false).Succeeded());
		REQUIRE(BindTubePowerVehicle(this->capsule->index, this->source).has_value());
		this->Orders(*this->capsule, this->origin->index, this->interchange->index, this->tube_depots, true);
		this->Orders(*this->freight, this->interchange->index, this->destination->index, this->freight_depots, false);
		fmt::print("MOLE_ROUTE_TARGET origin={}:{} berth={} order={}:{} dest={} cargo={}:{}\n", this->origin->index.base(), this->origin->xy.base(), origin_tile.base(),
			to_underlying(this->capsule->current_order.GetType()), this->capsule->current_order.GetDestination().base(), this->capsule->dest_tile.base(),
			this->capsule->cargo_type, this->cargo);
	}

	/** Retire actual native vehicles before their source/definition/map owners; no files or artifacts are deleted. */
	~MoleUsefulRouteRig()
	{
		// Native destruction owns packets, requests and station loading entries.
		while (Vehicle::GetNumItems() != 0) delete (*Vehicle::Iterate().begin())->First();
		ResetTubePowerState(); ResetUtilityPowerState(); _group_pool.CleanPool(); _engine_pool.CleanPool();
		_engine_mngr.mappings = std::move(this->engines); _industile_mngr.mappings = std::move(this->industry_tiles);
	}

	/** @param industry Actual command-funded footprint. Complete machinery only as disclosed prepared input, never cargo or reserve. */
	void Complete(Industry &industry)
	{
		// Native funding already owns every cell; this boundary supplies only
		// completed machinery and cannot bypass material/packet production.
		for (const auto tile : industry.location) if (industry.TileBelongsToIndustry(tile)) SetIndustryCompleted(tile);
	}

	/** @param tile Actual plain route cell. @param type Native tube/conventional domain. @param track Physical installed direction. */
	void TrackAt(TileIndex tile, RailType type, Track track)
	{
		// Existing exact station/depot cells retain their real IDs and roles;
		// plain gap cells alone need new paid guideway construction.
		if (IsRailStationTile(tile) || IsRailDepotTile(tile)) return;
		if (GetTileLayer(tile) != MapLayer::Surface) this->Open(std::array{tile});
		REQUIRE(Command<Commands::BuildRail>::Do(DoCommandFlag::Execute, tile, type, track, false).Succeeded());
	}

	/** @param tile Actual one-ended purchase/service cell. @param type Native movement domain. @param facing Real outward door direction. */
	void DepotAt(TileIndex tile, RailType type, DiagDirection facing)
	{
		const auto delta = TileIndexDiffCByDiagDir(facing);
		if (GetTileLayer(tile) != MapLayer::Surface) this->Open(std::array{tile, TileAddWrap(tile, delta.x, delta.y)});
		REQUIRE(Command<Commands::BuildRailDepot>::Do(DoCommandFlag::Execute, tile, type, facing).Succeeded());
	}

	/** @param tile Actual two-cell platform anchor. @param type Native movement domain. @param axis Real platform axis. @param join Existing same-layer station or NEW_STATION. @return Actual sole native packet/identity owner. */
	Station *StationAtRoute(TileIndex tile, RailType type, Axis axis, StationID join = NEW_STATION)
	{
		const int dx = axis == Axis::X ? 1 : 0, dy = axis == Axis::Y ? 1 : 0;
		if (GetTileLayer(tile) != MapLayer::Surface) this->Open(std::array{TileAddWrap(tile, -dx, -dy), tile, TileAddWrap(tile, dx, dy), TileAddWrap(tile, dx * 2, dy * 2)});
		REQUIRE(Command<Commands::BuildRailStation>::Do(DoCommandFlag::Execute, tile, type, axis, 1, 2, STAT_CLASS_DFLT, 0, join, false).Succeeded());
		return Station::GetByTile(tile);
	}

	/** @param lower Explicit prepared Underground housing. @param shaft Shaft direction at that end. Publish only the genuine reciprocal native pair and its accepted Connected record. */
	void PortalAt(TileIndex lower, DiagDirection shaft)
	{
		const auto door = TileIndexDiffCByDiagDir(ReverseDiagDir(shaft));
		this->Open(std::array{lower, TileAddWrap(lower, door.x, door.y)});
		const auto upper = MultiLayerMap::ProjectToSurface(lower);
		MakeRailLayerPortal(lower, _current_company, shaft, MapLayer::Surface, RAILTYPE_RAIL);
		MakeRailLayerPortal(upper, _current_company, ReverseDiagDir(shaft), MapLayer::Underground, RAILTYPE_RAIL);
		auto records = CaptureLayerPortalWorkRecords();
		LayerPortalWorkRecord pair; pair.pair_key = upper; pair.company = _current_company; pair.state = LayerPortalWorkState::Connected;
		records.push_back(pair); std::ranges::sort(records, {}, &LayerPortalWorkRecord::pair_key);
		REQUIRE(ValidateLayerPortalWorkRecords(records, GetCurrentMultiLayerProfileSelection(), GetGeologyWorldState()) == LayerPortalWorkError::None);
		StageLoadedLayerPortalWork(std::move(records)); AcceptLoadedLayerPortalWork();
	}

	/** @param depot Actual owned compatible native purchase cell. @param engine Original canonical engine identity. @param cargo Optional actual refit meaning. @return Command-purchased train part, retaining its original physical placement. */
	Train *BuyFront(TileIndex depot, EngineID engine, CargoType cargo = INVALID_CARGO)
	{
		const auto [cost, id, capacity, mail, capacities] = Command<Commands::BuildVehicle>::Do(DoCommandFlag::Execute, depot, engine, false, cargo, ClientID::Invalid);
		REQUIRE(cost.Succeeded()); return Train::Get(id);
	}

	/** @param front Actual purchased front. @param pickup Actual native loading station. @param drop Actual transfer/final station. @param depots Real start/end service depots, admitting ordinary reversal. @param transfer True preserves cargo for the joined conventional leg. */
	void Orders(Train &front, StationID pickup, StationID drop, const std::array<TileIndex, 2> &depots, bool transfer)
	{
		Order load; load.MakeGoToStation(pickup); load.SetUnloadType(OrderUnloadType::NoUnload); load.SetLoadType(OrderLoadType::LoadIfPossible);
		Order unload; unload.MakeGoToStation(drop); unload.SetUnloadType(transfer ? OrderUnloadType::Transfer : OrderUnloadType::UnloadIfPossible); unload.SetLoadType(OrderLoadType::NoLoad);
		Order far; far.MakeGoToDepot(Depot::GetByTile(depots[1])->index, {OrderDepotTypeFlag::PartOfOrders}, OrderNonStopFlag::NonStop, {});
		Order home; home.MakeGoToDepot(Depot::GetByTile(depots[0])->index, {OrderDepotTypeFlag::PartOfOrders}, OrderNonStopFlag::NonStop, {});
		// A normal four-order cycle reverses only in actual service depots.
		// No local arrival, current-order replacement or player skip supplies a leg.
		const std::array list{load, unload, far, home};
		for (size_t index = 0; index < list.size(); ++index) REQUIRE(Command<Commands::InsertOrder>::Do(DoCommandFlag::Execute, front.index, static_cast<VehicleOrderID>(index), list[index]).Succeeded());
		ProcessOrders(&front);
	}

	/** Advance one complete ordinary native vehicle opportunity, including the real deferred depot restart owner. */
	void Run()
	{
		// Match the real loop's native timers, including each pool ID's daily
		// slot. A frozen date fraction services front0 every tick and can prevent
		// an otherwise healthy metered capsule from ever leaving its depot.
		// The real vehicle loop owns loading, common power, stable pool ticks,
		// cargo aging and deferred depot/autoreplace restart. Calling front Tick
		// alone leaves correctly serviced trains stopped in its pending queue.
		RunUsefulTubeNativeOpportunity();
		// Observe each original whole consist after the native transaction; the
		// fixture neither replaces it nor repairs any physical/cargo state.
		for (auto *front : std::array{this->capsule, this->freight}) {
			REQUIRE_FALSE(front->vehstatus.Test(VehState::Crashed)); REQUIRE(IsTrainOnItsTiles(front));
		}
	}

	/** @param front Actual native capsule or receiving wagon. @return Its exact original packet IDs after checking native mine/station provenance and absence of purchased-supply purpose. */
	std::vector<CargoPacketID> CargoLineage(const Train &front) const
	{
		std::vector<CargoPacketID> result;
		// Every physical packet keeps the actual finite mine and first pickup
		// across mode changes. Quantity equality alone cannot prove its custody.
		for (const auto *packet : *front.cargo.Packets()) {
			CHECK(packet->GetSource() == Source{this->mine->index, SourceType::Industry});
			CHECK(packet->GetFirstStation() == this->origin->index); CHECK(packet->GetCompanySupplyOrder() == 0);
			result.push_back(packet->index);
		}
		return result;
	}
};

/** @param rig Actual original native route participants. @return Source/accounting/quantity/clock/physical/RNG observations without rebuilding or advancing any owner. */
static auto MoleUsefulWorldOwners(const MoleUsefulRouteRig &rig)
{
	/** @param front Actual original front or wagon. @return Native physical/packet/order observations; no independent pose or cargo owner. */
	const auto vehicle = [](const Train &front) {
		return std::tuple{front.index, front.tile, front.x_pos, front.y_pos, front.z_pos, front.direction, front.track,
			front.cur_speed, front.subspeed, front.progress, front.flags, front.vehstatus, front.cargo.StoredCount(),
			front.current_order.GetType(), front.current_order.GetDestination(), front.cur_real_order_index};
	};
	return std::tuple{ComputeUtilityPowerDigest(), ComputeTubePowerDigest(), ComputeTubePowerVehicleDigest(),
		Company::Get(_current_company)->money, Company::Get(_current_company)->cur_economy.income,
		Company::Get(_current_company)->cur_economy.delivered_cargo[rig.cargo], CargoPacket::GetNumItems(),
		rig.origin->goods[rig.cargo].TotalCount(), rig.interchange->goods[rig.cargo].TotalCount(), rig.factory->GetCargoAccepted(rig.cargo)->waiting,
		TimerGameTick::counter, TimerGameCalendar::date, TimerGameCalendar::date_fract, TimerGameEconomy::date, TimerGameEconomy::date_fract,
		vehicle(*rig.capsule), vehicle(*rig.freight), vehicle(*rig.wagon),
		std::array{_random.state[0], _random.state[1], _interactive_random.state[0], _interactive_random.state[1]}};
}

/**
 * Save an actual useful-route boundary without supplying another simulation opportunity.
 * @param rig Original actual mine/capsule/wagon/factory/source participants.
 * @param paths Invocation-owned generated save locations.
 * @param scene Diagnostic0 loaded moving capsule,1 moving conventional Underground,2 loaded Surface crossing,3 completed useful/empty return.
 * @param step Zero is the original; one is its actual uninterrupted next full native opportunity.
 * @param packet Original single finite-mine packet, legitimately consumed by the final scene.
 */
static void SaveMoleUsefulWorld(const MoleUsefulRouteRig &rig, const TestArtifactPaths &paths, unsigned scene, unsigned step, CargoPacketID packet)
{
	const auto before = MoleUsefulWorldOwners(rig);
	const auto *binding = GetTubePowerVehicle(rig.capsule->index); REQUIRE(binding != nullptr);
	SetMouseCursor(SPR_CURSOR_MOUSE, PAL_NONE);
	AutoRestoreBackup mode{_game_mode, GameMode::Menu}; AutoRestoreBackup pause{_pause_mode, PauseModes{PauseMode::Normal}};
	AutoRestoreBackup compression{_savegame_format, std::string{"lzma"}};
	const auto path = paths.Output("saves", fmt::format("mole-useful-{}-{}.sav", scene, step));
	REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(path), false) == SaveLoadResult::Ok);
	CHECK(MoleUsefulWorldOwners(rig) == before);
	fmt::print("MOLE_WORLD scene={} step={} utility={} binding={} invoice={} capsule={} freight={} wagon={} packet={} mine={} factory={} origin={} interchange={} destination={} source={} tick={} carry={} cap_count={} wagon_count={} factory_wait={} income={} delivered={} calendar={} calendar_fraction={} economy={} economy_fraction={} path={}\n",
		scene, step, ComputeUtilityPowerDigest(), ComputeTubePowerVehicleDigest(), ComputeTubePowerDigest(),
		rig.capsule->index.base(), rig.freight->index.base(), rig.wagon->index.base(), packet.base(), rig.mine->index.base(), rig.factory->index.base(),
		rig.origin->index.base(), rig.interchange->index.base(), rig.destination->index.base(), rig.source.base(), TimerGameTick::counter, binding->remainder_mj,
		rig.capsule->cargo.StoredCount(), rig.wagon->cargo.StoredCount(), rig.factory->GetCargoAccepted(rig.cargo)->waiting,
		Company::Get(_current_company)->cur_economy.income.base(), Company::Get(_current_company)->cur_economy.delivered_cargo[rig.cargo],
		TimerGameCalendar::date.base(), TimerGameCalendar::date_fract, TimerGameEconomy::date.base(), TimerGameEconomy::date_fract, path.string());
}

/** @param save_files True additionally records actual ordinary worlds and uninterrupted full-driver baselines; false retains the original native route-only producer. */
static void RunMoleUsefulRoute(bool save_files)
{
	MockEnvironment::Instance(); LoadGeneratedWindowTestLanguage(); MoleUsefulRouteRig rig;
	std::optional<LayerSaveContent> content; std::optional<TestArtifactPaths> paths;
	AutoRestoreBackup local{_local_company, _current_company};
	if (save_files) { content.emplace(rig.ActivatedContent(), true); paths.emplace(GetTestCaseArtifactPaths("tubes", "useful-files")); }
	std::array<bool, 3> saved{}; std::optional<unsigned> pending;
	const auto reserve = GetGeologyWorldState().deposits.Get(rig.request.deposit)->remaining_reserve;
	// Five normal74-tick work opportunities create20 genuine finite units.
	// Both original fronts remain player-stopped until the real source is ready;
	// no fixture-created CargoPacket or station stock supplies this shipment.
	for (uint opportunity = 0; opportunity < 5; ++opportunity) {
		for (uint tick = 0; tick < 74; ++tick) rig.Run();
		rig.origin->goods[rig.cargo].rating = UINT8_MAX; rig.origin->goods[rig.cargo].last_speed = 100;
		REQUIRE(ProduceLayerMineCargo(*rig.mine, 74, true).amount == 4);
		rig.supplied += DispatchLayerMineCargo(*rig.mine); ++rig.production_opportunities;
	}
	REQUIRE(rig.supplied == 20); REQUIRE(rig.origin->goods[rig.cargo].TotalCount() == 20);
	REQUIRE(GetGeologyWorldState().deposits.Get(rig.request.deposit)->remaining_reserve == reserve - 20);
	const auto income = Company::Get(_current_company)->cur_economy.income;
	const auto deliveries = Company::Get(_current_company)->cur_economy.delivered_cargo[rig.cargo];
	REQUIRE(Command<Commands::StartStopVehicle>::Do(DoCommandFlag::Execute, rig.capsule->index, true).Succeeded());
	REQUIRE(Command<Commands::StartStopVehicle>::Do(DoCommandFlag::Execute, rig.freight->index, true).Succeeded());
	uint ticks = 0; bool carried = false, transferred = false, crossed = false, capsule_return = false, freight_return = false;
	std::vector<CargoPacketID> original_packets;
	std::tuple<uint, uint, uint, uint, uint> previous_stock{20, 0, 0, 0, 0};
	// Native orders must earn every berth/shaft arrival. A strict bound turns
	// missing route, loading or reversal behavior into an observable failure.
	while (ticks++ < 12000) {
		rig.Run();
		const auto stock = std::tuple{rig.origin->goods[rig.cargo].TotalCount(), rig.capsule->cargo.StoredCount(),
			rig.interchange->goods[rig.cargo].TotalCount(), rig.wagon->cargo.StoredCount(), rig.factory->GetCargoAccepted(rig.cargo)->waiting};
		// Log only actual cargo-owner transitions while diagnosing the native
		// route. This observation never changes loading, orders or packet custody.
		if (stock != previous_stock) {
			fmt::print("MOLE_ROUTE_STOCK tick={} origin={} capsule={} interchange={} wagon={} factory={}\n", TimerGameTick::counter,
				std::get<0>(stock), std::get<1>(stock), std::get<2>(stock), std::get<3>(stock), std::get<4>(stock));
			previous_stock = stock;
		}
		carried |= rig.capsule->cargo.StoredCount() > 0;
		transferred |= rig.wagon->cargo.StoredCount() > 0;
		if (rig.capsule->cargo.StoredCount() > 0) {
			const auto lineage = rig.CargoLineage(*rig.capsule);
			if (original_packets.empty()) original_packets = lineage;
			CHECK(lineage == original_packets);
		}
		if (rig.wagon->cargo.StoredCount() > 0) CHECK(rig.CargoLineage(*rig.wagon) == original_packets);
		CHECK(std::get<0>(stock) + std::get<1>(stock) + std::get<2>(stock) + std::get<3>(stock) + std::get<4>(stock) == 20);
		// Transfers cannot earn final income or delivery credit. Native factory
		// acceptance alone may begin that settlement after the Surface crossing.
		if (rig.factory->GetCargoAccepted(rig.cargo)->waiting == 0) {
			CHECK(Company::Get(_current_company)->cur_economy.income == income);
			CHECK(Company::Get(_current_company)->cur_economy.delivered_cargo[rig.cargo] == deliveries);
		}
		// A complete actual conventional load crossing is required before its
		// Surface receiver consumes the real packet; tube coordinates never cross.
		if (rig.wagon->cargo.StoredCount() > 0 && GetTileLayer(rig.wagon->tile) == MapLayer::Surface) crossed = true;
		if (save_files) {
			// The next normal loop opportunity supplies the genuine paired
			// baseline; saving must not add hidden movement or skip an observation.
			if (pending.has_value()) { SaveMoleUsefulWorld(rig, *paths, *pending, 1, original_packets.front()); pending.reset(); }
			// Capture only earned moving plain-track positions, clear of station
			// loading and the shaft edge. One next opportunity cannot be supplied
			// by a fixture arrival, pose assignment or diagnostic scene number.
			const std::array ready{
				rig.capsule->cargo.StoredCount() == 20 && rig.capsule->cur_speed > 0 && IsPlainRailTile(rig.capsule->tile),
				rig.wagon->cargo.StoredCount() == 20 && rig.freight->cur_speed > 0 && IsPlainRailTile(rig.freight->tile) && GetTileLayer(rig.wagon->tile) == MapLayer::Underground,
				rig.wagon->cargo.StoredCount() == 20 && rig.freight->cur_speed > 0 && IsPlainRailTile(rig.wagon->tile) && GetTileLayer(rig.wagon->tile) == MapLayer::Surface};
			// Each diagnostic stage records its first actual native opportunity;
			// repeated visits must not overwrite an immutable original checkpoint.
			for (unsigned scene = 0; scene < saved.size(); ++scene) {
				if (saved[scene]) continue;
				if (!ready[scene]) continue;
				REQUIRE_FALSE(original_packets.empty()); SaveMoleUsefulWorld(rig, *paths, scene, 0, original_packets.front());
				saved[scene] = true; pending = scene;
			}
		}
		// Require actual empty home-depot arrivals after loaded departure and
		// crossing. Initial purchase placement cannot count as a returned train.
		if (carried && rig.capsule->IsInDepot() && rig.capsule->tile == rig.tube_depots[0]) capsule_return = true;
		if (crossed && rig.freight->IsInDepot() && rig.freight->tile == rig.freight_depots[0]) freight_return = true;
		// Final payment closes through native departure, not fixture deletion;
		// both empty complete routes must finish before this result is accepted.
		if (rig.factory->GetCargoAccepted(rig.cargo)->waiting == 20 && capsule_return && freight_return && rig.freight->cargo_payment == nullptr) break;
	}
	INFO(DescribeTrain(rig.capsule)); INFO(DescribeTrain(rig.freight));
	INFO(fmt::format("carried={} transferred={} crossed={} stocks={}/{}/{}/{}/{} orders={}:{} / {}:{} wait={}", carried, transferred, crossed,
		rig.origin->goods[rig.cargo].TotalCount(), rig.capsule->cargo.StoredCount(), rig.interchange->goods[rig.cargo].TotalCount(),
		rig.wagon->cargo.StoredCount(), rig.factory->GetCargoAccepted(rig.cargo)->waiting,
		rig.capsule->cur_real_order_index, rig.capsule->last_station_visited.base(), rig.freight->cur_real_order_index, rig.freight->last_station_visited.base(),
		to_underlying(GetTubePowerVehicle(rig.capsule->index)->wait)));
	REQUIRE(ticks < 12000); CHECK(carried); CHECK(transferred); CHECK(crossed);
	CHECK(capsule_return); CHECK(freight_return); REQUIRE_FALSE(original_packets.empty());
	CHECK(rig.capsule->cargo.StoredCount() == 0); CHECK(rig.wagon->cargo.StoredCount() == 0);
	CHECK(rig.origin->goods[rig.cargo].TotalCount() == 0); CHECK(rig.interchange->goods[rig.cargo].TotalCount() == 0);
	CHECK(GetUtilityPowerProducer(rig.source)->consumed_lifetime_j > 0);
	CHECK(GetTubePowerConnections().size() == 1);
	CHECK(Company::Get(_current_company)->cur_economy.delivered_cargo[rig.cargo] == deliveries + 20);
	const auto paid = Company::Get(_current_company)->cur_economy.income; REQUIRE(paid > income);
	// Two further real economy days permit empty repeat orders and service.
	// No original material or final payment can be recreated by those retries.
	for (uint repeat = 0; repeat < 148; ++repeat) {
		rig.Run();
		CHECK(Company::Get(_current_company)->cur_economy.income == paid);
		CHECK(Company::Get(_current_company)->cur_economy.delivered_cargo[rig.cargo] == deliveries + 20);
		CHECK(rig.factory->GetCargoAccepted(rig.cargo)->waiting == 20);
		CHECK(rig.capsule->cargo.StoredCount() == 0); CHECK(rig.wagon->cargo.StoredCount() == 0);
	}
	CHECK(ValidateTubePowerVehicles(CaptureTubePowerVehicles()) == TubePowerError::None);
	fmt::print("MOLE_USEFUL delivered=20 source=finite_mine year=1970 autonomous=true original_packets={} empty_returns=2 income={} consumed={} ticks={} tick={}\n",
		original_packets.size(), (paid - income).base(), GetUtilityPowerProducer(rig.source)->consumed_lifetime_j, ticks, TimerGameTick::counter);
	if (save_files) {
		REQUIRE(std::ranges::all_of(saved, std::identity{})); REQUIRE_FALSE(pending.has_value());
		SaveMoleUsefulWorld(rig, *paths, 3, 0, original_packets.front()); rig.Run(); SaveMoleUsefulWorld(rig, *paths, 3, 1, original_packets.front());
	}
}

/** Actual original Coal production feeds normal native travel and one useful Surface intake without any arrival/packet substitution. */
TEST_CASE("Mole1970 drives finite mine cargo through native transfer to Surface", "[tube-useful-mole-route]")
{
	RunMoleUsefulRoute(false);
}

/** Ordinary current files retain actual moving capsule, conventional crossing and completed useful worlds plus uninterrupted full-native next opportunities. */
TEST_CASE("Mole1970 writes actual useful-route native worlds", "[tube-useful-mole-save-file]")
{
	RunMoleUsefulRoute(true);
}
