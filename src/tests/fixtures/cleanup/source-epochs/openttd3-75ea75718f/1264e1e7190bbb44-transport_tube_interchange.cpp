/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file transport_tube_interchange.cpp Actual native packet interchange at command-built tube/road berths.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Group9.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Group12D native orders/CargoDist/empty-return proof.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
 * Fixture arrivals exercise the economy boundary; driven useful-route acceptance remains Group15.
 */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "layer_window_fixture.h"
#include "movement_fixture.h"
#include "transport_tube_fixture.h"
#include "../economy_func.h"
#include "../economy_base.h"
#include "../station_supply.h"
#include "../transport_tube_stock.h"
#include "../train_cmd.h"
#include "../order_cmd.h"
#include "../order_func.h"
#include "../layer_industry_processor.h"
#include "../industry_map.h"
#include "../linkgraph/linkgraph.h"
#include "../linkgraph/linkgraphjob.h"
#include "../linkgraph/linkgraphschedule.h"
#include "../safeguards.h"

/** Actual mine, purchased carriers and two native stations; all cargo stays in native packet lists. */
class TubeInterchangeRig : public TubePrototypeRig {
	AutoRestoreBackup<bool> distant{_settings_game.station.distant_join_stations, true}; ///< Explicitly enable the ordinary nonadjacent station-join setting for these real separated berths.
public:
	TileIndex tube_berth = TileXYL(18, 44, MapLayer::Underground); ///< Actual destination capsule platform.
	TileIndex road_berth = TileXYL(21, 44, MapLayer::Underground); ///< Separate conventional freight cell.
	Station *origin = nullptr; ///< Sole packet owner of the mine-side tube loading platform.
	Station *interchange = nullptr; ///< Native destination platform and optional joined truck station.
	Station *road = nullptr; ///< Native road packet owner; sharing requires an actual explicit join.
	CargoType cargo = INVALID_CARGO; ///< Actual finite mine output, resolved through native cargo definitions.
	Train *capsule = nullptr; ///< Command-purchased native tube vehicle, never a substitute stock record.
	Vehicle *truck = nullptr; ///< Command-purchased conventional vehicle receiving the onward leg.

	/**
	 * Build physical loading cells through ordinary commands on explicit fixture-open floor.
	 * @param joined Whether the actual road stop explicitly joins the destination StationID.
	 */
	explicit TubeInterchangeRig(bool joined = true)
	{
		this->OpenRailSite();
		REQUIRE(CmdBuildRailStation(DoCommandFlag::Execute, this->RailSite(), this->tube, Axis::X,
			1, 2, STAT_CLASS_DFLT, 0, NEW_STATION, false).Succeeded());
		this->origin = Station::GetByTile(this->RailSite());
		// Only the real two-cell berth and its two through approaches are opened;
		// joining a cargo owner never excavates the gap between different modes.
		for (int along = -1; along <= 2; ++along) this->Open(std::array{TileAddWrap(this->tube_berth, along, 0)});
		REQUIRE(CmdBuildRailStation(DoCommandFlag::Execute, this->tube_berth, this->tube, Axis::X,
			1, 2, STAT_CLASS_DFLT, 0, NEW_STATION, false).Succeeded());
		this->interchange = Station::GetByTile(this->tube_berth);
		this->Open(std::array{this->road_berth, TileAddWrap(this->road_berth, 0, 1)});
		REQUIRE(CmdBuildRoadStop(DoCommandFlag::Execute, this->road_berth, 1, 1, RoadStopType::Truck,
			false, DiagDirection::SE, ROADTYPE_ROAD, ROADSTOP_CLASS_DFLT, 0,
			joined ? this->interchange->index : NEW_STATION, false).Succeeded());
		this->road = Station::GetByTile(this->road_berth);
		this->cargo = this->industry->produced[0].cargo;
		this->capsule = this->BuyCapsule();
		this->truck = this->Buy(this->BuildDepot(MapLayer::Underground, true), 7, this->cargo);
	}

	/**
	 * Produce and dispatch actual finite mine material through its existing station supply owner.
	 * @param intervals Bounded number of real production opportunities, each using the canonical74-tick duration.
	 * @return Whole units admitted to the native source station.
	 */
	uint Supply(uint intervals = 20)
	{
		this->origin->goods[this->cargo].rating = UINT8_MAX;
		this->origin->goods[this->cargo].last_speed = 100;
		uint supplied = 0;
		// Repeated producer opportunities supply more than one vehicle can hold;
		// actual reserve/work/history and exact station receipts remain the owners.
		for (uint interval = 0; interval < intervals; ++interval) {
			REQUIRE(ProduceLayerMineCargo(*this->industry, 74, true).amount > 0);
			supplied += DispatchLayerMineCargo(*this->industry);
		}
		return supplied;
	}

	/**
	 * Enter the ordinary loading owner at an explicit fixture arrival boundary.
	 * @param vehicle Actual purchased carrier; its packet list remains intact.
	 * @param tile Actual compatible platform/road cell.
	 * @param station Native station owning that cell and its cargo queue.
	 * @param unload Native unload policy, including explicit transfer.
	 * @param load Native load policy, including no-load empty departure.
	 */
	void Arrive(Vehicle *vehicle, TileIndex tile, Station *station, OrderUnloadType unload, OrderLoadType load)
	{
		REQUIRE(GetStationIndex(tile) == station->index);
		// The explicit arrival includes every actual wagon, so native cargo tiles
		// refer to the real berth rather than a detached hold left in its depot.
		uint along = 0;
		for (Vehicle *part = vehicle; part != nullptr; part = part->Next()) {
			part->tile = TileAddWrap(tile, along++, 0);
			part->x_pos = TileX(part->tile) * TILE_SIZE + TILE_SIZE / 2;
			part->y_pos = TileY(part->tile) * TILE_SIZE + TILE_SIZE / 2;
			part->z_pos = 0;
			part->vehstatus.Reset({VehState::Stopped, VehState::Hidden});
			if (part->type == VehicleType::Train) Train::From(part)->track = Track::X;
			part->UpdatePosition();
		}
		vehicle->last_station_visited = station->index;
		vehicle->current_order.MakeLoading(false);
		vehicle->current_order.SetUnloadType(unload);
		vehicle->current_order.SetLoadType(load);
		PrepareUnload(vehicle);
	}

	/**
	 * Complete bounded native loading and settle its native payment object once.
	 * @param vehicle Actual arriving carrier with an active native loading/payment entry.
	 * @param station Actual station whose economy scheduler advances loading.
	 */
	void Finish(Vehicle *vehicle, Station *station)
	{
		// A bounded station scheduler burst exercises gradual loading, reservations
		// and transfer accounting without replacing movement or packet ownership.
		for (uint tick = 0; tick < 1000 && !vehicle->vehicle_flags.Test(VehicleFlag::LoadingFinished); ++tick) LoadUnloadStation(station);
		REQUIRE(vehicle->vehicle_flags.Test(VehicleFlag::LoadingFinished));
		delete vehicle->cargo_payment;
		station->loading_vehicles.remove(vehicle);
	}

	/**
	 * Prove the same receiving conventional consist carries its packets through a native layer crossing.
	 * @param head Actual purchased locomotive whose real wagon just loaded at the interchange.
	 * @param depot Actual purchased-from depot used as an explicit physical start for this separate boundary.
	 * The local berth-to-depot arrival is fixture staging; the complete useful route remains Group15.
	 */
	void DriveFreightToSurface(Train *head, TileIndex depot)
	{
		PathfinderDefaults pathfinder;
		AutoRestoreBackup<uint64_t> tick{TimerGameTick::counter, uint64_t{0}};
		const TileIndex lower = TileXYL(40, 46, MapLayer::Underground);
		const TileIndex upper = MultiLayerMap::ProjectToSurface(lower);
		this->Open(std::array{lower, TileAddWrap(lower, 0, -1)});
		MakeRailLayerPortal(lower, _current_company, DiagDirection::SE, MapLayer::Surface, RAILTYPE_RAIL);
		MakeRailLayerPortal(upper, _current_company, DiagDirection::NW, MapLayer::Underground, RAILTYPE_RAIL);
		// A commissioned reciprocal pair is explicit physical boundary input,
		// preserving the real loader's validation and original portal records.
		auto records = CaptureLayerPortalWorkRecords();
		LayerPortalWorkRecord opened;
		opened.pair_key = upper; opened.company = _current_company; opened.state = LayerPortalWorkState::Connected;
		records.push_back(opened);
		std::ranges::sort(records, {}, &LayerPortalWorkRecord::pair_key);
		REQUIRE(ValidateLayerPortalWorkRecords(records, GetCurrentMultiLayerProfileSelection(), GetGeologyWorldState()) == LayerPortalWorkError::None);
		StageLoadedLayerPortalWork(std::move(records));
		AcceptLoadedLayerPortalWork();
		// Build one unbranched ordinary route from the real lower depot to a
		// Surface line end. No tube tile, portal shortcut or new cargo owner participates.
		for (int y = 41; y < 46; ++y) this->BuildTrack(TileXYL(40, y, MapLayer::Underground), RAILTYPE_RAIL, Track::Y);
		for (int y = 47; y <= 50; ++y) {
			const TileIndex tile = TileXYL(40, y, MapLayer::Surface);
			// The ordered-freight continuation may already own the line end as an
			// actual conventional station. Preserve its native berth instead of replacing it.
			if (IsRailStationTile(tile)) continue;
			REQUIRE(CmdBuildSingleRail(DoCommandFlag::Execute, tile, RAILTYPE_RAIL, Track::Y, false).Succeeded());
		}
		CHECK(CanTubeTrainEnterTile(*head, lower));
		CHECK_FALSE(CanTubeTrainEnterTile(*this->capsule, lower));
		// Begin this crossing boundary inside the real purchase depot. Only pose
		// changes: the same IDs, packet lists, provenance and received quantities remain.
		for (Train *part = head; part != nullptr; part = part->Next()) {
			part->tile = depot;
			part->track = Track::Depot;
			part->x_pos = TileX(depot) * TILE_SIZE + TILE_SIZE / 2;
			part->y_pos = TileY(depot) * TILE_SIZE + TILE_SIZE / 2;
			part->z_pos = 0;
			part->vehstatus.Set(VehState::Hidden);
			part->UpdatePosition();
		}
		head->vehstatus.Set(VehState::Stopped);
		head->current_order.MakeDummy();
		head->cur_speed = 0;
		head->ConsistChanged(CCF_LOADUNLOAD);
		head->CargoChanged();
		UpdateSignalsInBuffer();
		REQUIRE(CmdStartStopVehicle(DoCommandFlag::Execute, head->index, true).Succeeded());
		uint ticks = 0;
		// Stop only after the last actual wagon reaches Surface, with a strict
		// time bound so a failed physical route cannot hang the focused suite.
		while (GetTileLayer(head->Last()->tile) != MapLayer::Surface && ticks < 6000) {
			++TimerGameTick::counter;
			REQUIRE(head->Tick());
			REQUIRE_FALSE(head->vehstatus.Test(VehState::Crashed));
			REQUIRE(IsTrainOnItsTiles(head));
			++ticks;
		}
		INFO(DescribeTrain(head));
		REQUIRE(ticks < 6000);
		CHECK(GetTileLayer(head->tile) == MapLayer::Surface);
	}
};

/** A real capsule transfers finite packets once; a conventional truck takes the onward leg from the shared queue. */
TEST_CASE("Tube interchange transfers actual native packets to a conventional carrier", "[transport-tube-interchange]")
{
	bool rail = false;
	SECTION("conventional truck") {}
	SECTION("conventional locomotive and mineral wagon") { rail = true; }
	MockEnvironment::Instance();
	LoadGeneratedWindowTestLanguage();
	TubeInterchangeRig rig;
	Vehicle *outbound = rig.truck;
	Vehicle *hold = rig.truck;
	TileIndex outbound_berth = rig.road_berth;
	TileIndex rail_depot = INVALID_TILE;
	if (rail) {
		outbound_berth = TileAddWrap(rig.tube_berth, 0, 3);
		// Build a distinct conventional berth on opened floor, explicitly joined
		// to the same packet owner; the tube rail cells retain their own domain.
		for (int along = -1; along <= 2; ++along) rig.Open(std::array{TileAddWrap(outbound_berth, along, 0)});
		REQUIRE(CmdBuildRailStation(DoCommandFlag::Execute, outbound_berth, RAILTYPE_RAIL, Axis::X,
			1, 2, STAT_CLASS_DFLT, 0, rig.interchange->index, false).Succeeded());
		rail_depot = rig.BuildDepot(MapLayer::Underground, false);
		outbound = rig.Buy(rail_depot, 4);
		hold = rig.Buy(rail_depot, 5, rig.cargo);
		REQUIRE(CmdMoveRailVehicle(DoCommandFlag::Execute, hold->index, outbound->index, false).Succeeded());
	}
	const uint supplied = rig.Supply();
	const auto reserve = rig.Reserve();
	rig.Arrive(rig.capsule, rig.RailSite(), rig.origin, OrderUnloadType::NoUnload, OrderLoadType::LoadIfPossible);
	rig.Finish(rig.capsule, rig.origin);
	const uint carried = rig.capsule->cargo.TotalCount();
	REQUIRE(carried > 0);
	CHECK(carried <= rig.capsule->cargo_cap);
	CHECK(carried + rig.origin->goods[rig.cargo].TotalCount() == supplied);
	const Money money = Company::Get(_current_company)->money;
	const auto delivered = Company::Get(_current_company)->cur_economy.delivered_cargo[rig.cargo];
	rig.Arrive(rig.capsule, rig.tube_berth, rig.interchange, OrderUnloadType::Transfer, OrderLoadType::NoLoad);
	rig.Finish(rig.capsule, rig.interchange);
	CHECK(rig.capsule->cargo.TotalCount() == 0);
	CHECK(rig.interchange->goods[rig.cargo].TotalCount() == carried);
	CHECK(Company::Get(_current_company)->money == money);
	CHECK(Company::Get(_current_company)->cur_economy.delivered_cargo[rig.cargo] == delivered);
	rig.Arrive(outbound, outbound_berth, rig.interchange, OrderUnloadType::NoUnload, OrderLoadType::LoadIfPossible);
	rig.Finish(outbound, rig.interchange);
	CHECK(hold->cargo.TotalCount() > 0);
	CHECK(hold->cargo.TotalCount() <= hold->cargo_cap);
	CHECK(rig.origin->goods[rig.cargo].TotalCount() + rig.interchange->goods[rig.cargo].TotalCount() + hold->cargo.TotalCount() == supplied);
	CHECK(rig.Reserve() == reserve);
	CHECK(Company::Get(_current_company)->money == money);
	CHECK_FALSE(CanTubeTrainEnterTile(*rig.capsule, outbound_berth));
	CHECK_FALSE(rig.interchange->TileIsInCatchment(MultiLayerMap::ProjectToSurface(rig.tube_berth)));
	if (rail) {
		const uint carried_onward = hold->cargo.TotalCount();
		const VehicleID carrier = hold->index;
		std::vector<CargoPacketID> packets;
		// Retain actual packet identities and their original loading station;
		// aggregate counts alone could hide a replacement cargo buffer or recreated load.
		for (const CargoPacket *packet : *hold->cargo.Packets()) packets.push_back(packet->index);
		rig.DriveFreightToSurface(Train::From(outbound), rail_depot);
		CHECK(hold->index == carrier);
		CHECK(hold->cargo.TotalCount() == carried_onward);
		CHECK(GetTileLayer(hold->tile) == MapLayer::Surface);
		CHECK(GetTileLayer(rig.capsule->tile) == MapLayer::Underground);
		std::vector<CargoPacketID> crossed;
		// Native crossing changes physical layer while every received packet
		// still identifies its original industry and first station.
		for (const CargoPacket *packet : *hold->cargo.Packets()) {
			crossed.push_back(packet->index);
			CHECK(packet->GetFirstStation() == rig.origin->index);
			CHECK(packet->GetSource() == Source{rig.industry->index, SourceType::Industry});
		}
		CHECK(crossed == packets);
	}
}

/** A nearby independent road stop cannot acquire stock from an unjoined tube station. */
TEST_CASE("Tube interchange requires the actual joined station cargo identity", "[transport-tube-interchange]")
{
	MockEnvironment::Instance();
	LoadGeneratedWindowTestLanguage();
	TubeInterchangeRig rig(false);
	REQUIRE(rig.road != rig.interchange);
	const uint supplied = rig.Supply();
	rig.Arrive(rig.capsule, rig.RailSite(), rig.origin, OrderUnloadType::NoUnload, OrderLoadType::LoadIfPossible);
	rig.Finish(rig.capsule, rig.origin);
	const uint carried = rig.capsule->cargo.TotalCount();
	REQUIRE(carried > 0);
	rig.Arrive(rig.capsule, rig.tube_berth, rig.interchange, OrderUnloadType::Transfer, OrderLoadType::NoLoad);
	rig.Finish(rig.capsule, rig.interchange);
	rig.Arrive(rig.truck, rig.road_berth, rig.road, OrderUnloadType::NoUnload, OrderLoadType::LoadIfPossible);
	rig.Finish(rig.truck, rig.road);
	CHECK(rig.truck->cargo.TotalCount() == 0);
	CHECK(rig.road->goods[rig.cargo].TotalCount() == 0);
	CHECK(rig.interchange->goods[rig.cargo].TotalCount() == carried);
	CHECK(rig.origin->goods[rig.cargo].TotalCount() + carried == supplied);
}

/** Full and partially full native queues retain the refused capsule load and create no final-delivery income. */
TEST_CASE("Tube interchange transfer preserves cargo at its finite native queue bound", "[transport-tube-stock]")
{
	const uint free_room = GENERATE(0U, 1U);
	MockEnvironment::Instance();
	LoadGeneratedWindowTestLanguage();
	TubeInterchangeRig rig;
	const uint supplied = rig.Supply();
	rig.Arrive(rig.capsule, rig.RailSite(), rig.origin, OrderUnloadType::NoUnload, OrderLoadType::LoadIfPossible);
	rig.Finish(rig.capsule, rig.origin);
	const uint carried = rig.capsule->cargo.TotalCount();
	REQUIRE(carried > free_room);
	auto &queue = rig.interchange->goods[rig.cargo].GetOrCreateData().cargo;
	// Existing queued packets are explicit stock input, with a genuine source;
	// this sets receiver pressure without inventing an alternate quantity ledger.
	REQUIRE(CargoPacket::CanAllocateItem());
	queue.Append(CargoPacket::Create(rig.origin->index, static_cast<uint16_t>(EXACT_STATION_SUPPLY_CAPACITY - free_room),
		Source{rig.industry->index, SourceType::Industry}), StationID::Invalid());
	const uint64_t total = supplied + queue.TotalCount();
	const Money money = Company::Get(_current_company)->money;
	rig.Arrive(rig.capsule, rig.tube_berth, rig.interchange, OrderUnloadType::Transfer, OrderLoadType::NoLoad);
	rig.Finish(rig.capsule, rig.interchange);
	CHECK(queue.TotalCount() == EXACT_STATION_SUPPLY_CAPACITY);
	CHECK(rig.capsule->cargo.TotalCount() == carried - free_room);
	CHECK(Company::Get(_current_company)->money == money);
	// Repeated blocked visits can neither discard the retained load nor turn
	// virtual transfer credit into a new final-delivery bank payment.
	for (uint visit = 0; visit < 3; ++visit) {
		rig.Arrive(rig.capsule, rig.tube_berth, rig.interchange, OrderUnloadType::Transfer, OrderLoadType::NoLoad);
		rig.Finish(rig.capsule, rig.interchange);
		CHECK(queue.TotalCount() == EXACT_STATION_SUPPLY_CAPACITY);
		CHECK(rig.capsule->cargo.TotalCount() == carried - free_room);
		CHECK(Company::Get(_current_company)->money == money);
	}
	CHECK(rig.origin->goods[rig.cargo].TotalCount() + queue.TotalCount() + rig.capsule->cargo.TotalCount() == total);
	// A normal conventional pickup releases actual queue room. The next capsule
	// visit transfers exactly that room, retaining any still-blocked remainder.
	rig.Arrive(rig.truck, rig.road_berth, rig.road, OrderUnloadType::NoUnload, OrderLoadType::LoadIfPossible);
	rig.Finish(rig.truck, rig.road);
	const uint picked_up = rig.truck->cargo.TotalCount();
	REQUIRE(picked_up > 0);
	rig.Arrive(rig.capsule, rig.tube_berth, rig.interchange, OrderUnloadType::Transfer, OrderLoadType::NoLoad);
	rig.Finish(rig.capsule, rig.interchange);
	CHECK(rig.capsule->cargo.TotalCount() == carried - free_room - std::min(picked_up, carried - free_room));
	CHECK(rig.origin->goods[rig.cargo].TotalCount() + queue.TotalCount() + rig.capsule->cargo.TotalCount() + picked_up == total);
	CHECK(Company::Get(_current_company)->money == money);
}

/** Native loading reservations consume the same bounded room until they become actual carried cargo. */
TEST_CASE("Tube interchange stock room includes native pickup reservations", "[transport-tube-stock]")
{
	MockEnvironment::Instance();
	LoadGeneratedWindowTestLanguage();
	TubeInterchangeRig rig;
	auto &queue = rig.interchange->goods[rig.cargo].GetOrCreateData().cargo;
	REQUIRE(CargoPacket::CanAllocateItem());
	queue.Append(CargoPacket::Create(rig.origin->index, UINT16_MAX,
		Source{rig.industry->index, SourceType::Industry}), StationID::Invalid());
	const std::array<StationID, 0> next{};
	REQUIRE(queue.Reserve(1, &rig.truck->cargo, next, rig.road_berth) == 1);
	CHECK(rig.truck->cargo.ReservedCount() == 1);
	CHECK(GetTubeInterchangeTransferRoom(rig.interchange->index, rig.cargo, &queue) == 0);
	CHECK(GetTubeInterchangeTransferRoom(rig.road->index, rig.cargo, &queue) == 0);
	CHECK_FALSE(GetTubeInterchangeTransferRoom(rig.origin->index, rig.cargo, &queue).has_value());
	REQUIRE(queue.Load(1, &rig.truck->cargo, next, rig.road_berth) == 1);
	CHECK(GetTubeInterchangeTransferRoom(rig.interchange->index, rig.cargo, &queue) == 1);
	CHECK(queue.TotalCount() + rig.truck->cargo.TotalCount() == EXACT_STATION_SUPPLY_CAPACITY);
}

/** Native mine -> capsule -> conventional wagon -> Surface refinery, with ordinary order lists and computed CargoDist flows. */
class TubeFactoryOrdersRig : public TubeInterchangeRig {
	PathfinderDefaults pathfinder; ///< Keep ordinary nonzero pathfinder defaults through the entire Surface arrival, beyond the inherited crossing helper's scope.
	LinkGraphSettings saved_graph = _settings_game.linkgraph; ///< Restore only this fixture's explicit ordinary CargoDist tuning.
	decltype(_industile_mngr.mappings) tile_mapping = _industile_mngr.mappings; ///< Restore native processor tile identities before inherited industry teardown.
	AutoRestoreBackup<Money> tariff{CargoSpec::Get(this->cargo)->current_payment, Money{1000}}; ///< Nonzero native cargo price makes duplicate final payments observable.
	AutoRestoreBackup<int16_t> fluct{_economy.fluct, int16_t{1}}; ///< Explicit ordinary non-recession processor conditions, as in the existing ProcessorRig.
	AutoRestoreBackup<uint16_t> scale{_settings_game.economy.industry_cargo_scale, uint16_t{100}}; ///< Actual ordinary processor input/output scale, not the uninitialized standalone default.
public:
	TileIndex freight_berth = TileAddWrap(this->tube_berth, 0, 3); ///< Joined ordinary railway platform on the actual Underground station.
	TileIndex factory_berth = TileXYL(40, 50, MapLayer::Surface); ///< Actual Surface line-end station, reachable from the ordinary portal.
	TileIndex freight_depot = INVALID_TILE; ///< Actual conventional purchase/start depot for the disclosed crossing boundary.
	Industry *factory = nullptr; ///< Command-funded existing first-party Surface refinery; no invented underground factory.
	Station *destination = nullptr; ///< Native accepted-cargo/catchment owner beside the refinery.
	Station *ghost = nullptr; ///< Independent same-XY Surface station; no order or connection supplies it.
	Train *freight = nullptr; ///< Real ordinary locomotive; the same wagon makes the physical Surface crossing.
	Train *wagon = nullptr; ///< Sole actual conventional hold receiving the capsule's packets.

	/** Build only explicit fixture geometry and native content through existing construction/ownership APIs. */
	TubeFactoryOrdersRig()
	{
		// These are the normal documented algorithm defaults, with asymmetric
		// freight explicitly enabled; no flow or destination is injected by the test.
		_settings_game.linkgraph.distribution_default = DistributionType::Asymmetric;
		_settings_game.linkgraph.recalc_time = 32;
		_settings_game.linkgraph.recalc_interval = 8;
		_settings_game.linkgraph.accuracy = 16;
		_settings_game.linkgraph.demand_size = 100;
		_settings_game.linkgraph.demand_distance = 100;
		_settings_game.linkgraph.short_path_saturation = 80;
		REQUIRE(InitialiseLayerProcessorDefinitions(this->ActivatedContent()));
		const TileIndex anchor = TileXYL(42, 50, MapLayer::Surface);
		const IndustryType type = GetLayerProcessorIndustryType(MultiLayerRecipeID{"CFMT"});
		REQUIRE(type != IT_INVALID);
		REQUIRE(Command<Commands::BuildIndustry>::Do(DoCommandFlag::Execute, anchor, type, 0, true, 123).Succeeded());
		this->factory = Industry::Get(GetIndustryIndex(anchor));
		// Construction completion is explicit fixture input. This case tests
		// real freight acceptance, not transported first-plant bootstrap or a new-game start.
		for (TileIndex tile : this->factory->location) {
			if (this->factory->TileBelongsToIndustry(tile)) SetIndustryCompleted(tile);
		}
		REQUIRE(CmdBuildRailStation(DoCommandFlag::Execute, this->factory_berth, RAILTYPE_RAIL, Axis::Y,
			1, 2, STAT_CLASS_DFLT, 0, NEW_STATION, false).Succeeded());
		this->destination = Station::GetByTile(this->factory_berth);
		REQUIRE(this->destination->goods[this->cargo].status.Test(GoodsEntry::State::Acceptance));
		REQUIRE(this->factory->GetCargoAccepted(this->cargo) != this->factory->accepted.end());
		const TileIndex mirror = MultiLayerMap::ProjectToSurface(this->tube_berth);
		REQUIRE(CmdBuildRailStation(DoCommandFlag::Execute, mirror, RAILTYPE_RAIL, Axis::X,
			1, 2, STAT_CLASS_DFLT, 0, NEW_STATION, false).Succeeded());
		this->ghost = Station::GetByTile(mirror);
		// A separate conventional platform joins the exact native packet owner;
		// its fully opened approach never grants the capsule ordinary rail permission.
		for (int along = -1; along <= 2; ++along) this->Open(std::array{TileAddWrap(this->freight_berth, along, 0)});
		REQUIRE(CmdBuildRailStation(DoCommandFlag::Execute, this->freight_berth, RAILTYPE_RAIL, Axis::X,
			1, 2, STAT_CLASS_DFLT, 0, this->interchange->index, false).Succeeded());
		this->freight_depot = this->BuildDepot(MapLayer::Underground, false);
		this->freight = Train::From(this->Buy(this->freight_depot, 4));
		this->wagon = Train::From(this->Buy(this->freight_depot, 5, this->cargo));
		REQUIRE(CmdMoveRailVehicle(DoCommandFlag::Execute, this->wagon->index, this->freight->index, false).Succeeded());
		this->Orders(*this->capsule, this->origin->index, this->interchange->index);
		this->Orders(*this->freight, this->interchange->index, this->destination->index);
	}

	/** Restore only fixture tuning/identity metadata; inherited owners then retire actual trains, stations and industries. */
	~TubeFactoryOrdersRig()
	{
		_settings_game.linkgraph = this->saved_graph;
		_industile_mngr.mappings = std::move(this->tile_mapping);
	}

	/**
	 * Install an actual native two-stop empty-return order cycle.
	 * @param train Real purchased front, never a separate route record.
	 * @param from Actual loading station.
	 * @param to Actual unloading station; ordinary CargoDist chooses transfer versus final delivery.
	 */
	void Orders(Train &train, StationID from, StationID to)
	{
		Order pickup; pickup.MakeGoToStation(from);
		pickup.SetUnloadType(OrderUnloadType::NoUnload);
		pickup.SetLoadType(OrderLoadType::LoadIfPossible);
		Order drop; drop.MakeGoToStation(to);
		drop.SetUnloadType(OrderUnloadType::UnloadIfPossible);
		drop.SetLoadType(OrderLoadType::NoLoad);
		REQUIRE(CmdInsertOrder(DoCommandFlag::Execute, train.index, 0, pickup).Succeeded());
		REQUIRE(CmdInsertOrder(DoCommandFlag::Execute, train.index, 1, drop).Succeeded());
		ProcessOrders(&train);
	}

	/**
	 * Reach a local station at the disclosed fixture arrival boundary, preserving its real ordered policy.
	 * @param train Actual native front; every physical part remains in its existing chain.
	 * @param tile Actual berth tile.
	 * @param station Actual native packet/flow owner.
	 * @param axis Actual platform axis for the fixture pose.
	 */
	void OrderedArrival(Train &train, TileIndex tile, Station &station, Axis axis = Axis::X)
	{
		REQUIRE(train.current_order.IsType(OT_GOTO_STATION));
		REQUIRE(train.current_order.GetDestination() == station.index);
		uint along = 0;
		// Local arrival positions are explicit input, not a claimed driven tube
		// journey. All packet, order, station and vehicle IDs remain native and unchanged.
		for (Train *part = &train; part != nullptr; part = part->Next()) {
			part->tile = TileAddWrap(tile, axis == Axis::X ? along : 0, axis == Axis::Y ? along : 0);
			++along;
			part->x_pos = TileX(part->tile) * TILE_SIZE + TILE_SIZE / 2;
			part->y_pos = TileY(part->tile) * TILE_SIZE + TILE_SIZE / 2;
			part->z_pos = 0;
			part->track = axis == Axis::X ? Track::X : Track::Y;
			part->vehstatus.Reset({VehState::Stopped, VehState::Hidden});
			part->UpdatePosition();
		}
		TimerGameTick::counter += 74; // Explicit bounded arrival time supplies ordinary observed service statistics.
		train.last_station_visited = station.index;
		train.BeginLoading();
	}

	/**
	 * Complete the actual loading scheduler and close its native payment once.
	 * @param train Actual front currently loading.
	 * @param station Its real loading station.
	 * Fixture order progression uses the ordinary player skip command; full autonomous tube travel remains Group15.
	 */
	void FinishOrdered(Train &train, Station &station)
	{
		// Gradual loading/reservations must finish within a bounded actual
		// station scheduler burst; a failed receiver cannot hang a native test.
		for (uint tick = 0; tick < 1000 && !train.vehicle_flags.Test(VehicleFlag::LoadingFinished); ++tick) LoadUnloadStation(&station);
		REQUIRE(train.vehicle_flags.Test(VehicleFlag::LoadingFinished));
		{
			AutoRestoreBackup mode{_game_mode, GameMode::Menu}; // Suppress standalone money text effects only; native CargoPayment still settles real bank/feeder accounts.
			delete train.cargo_payment;
		}
		train.LeaveStation();
		const auto next = static_cast<VehicleOrderID>((train.cur_real_order_index + 1) % train.GetNumOrders());
		REQUIRE(CmdSkipToOrder(DoCommandFlag::Execute, train.index, next).Succeeded());
		// The fixture's next local arrival presupposes leaving this platform.
		// Clear only that transient departure operation; the real skip command
		// above owns the order indices/list, and no cargo or payment is recreated.
		train.current_order.Free();
		train.flags.Reset(VehicleRailFlag::LeavingStation);
		ProcessOrders(&train);
	}

	/** Calculate and publish the real native CargoDist job over observed order edges; no flow table is supplied by the fixture. */
	void ComputeFlows()
	{
		const LinkGraphID graph = this->origin->goods[this->cargo].link_graph;
		REQUIRE(graph != LinkGraphID::Invalid());
		REQUIRE(this->interchange->goods[this->cargo].link_graph == graph);
		REQUIRE(this->destination->goods[this->cargo].link_graph == graph);
		REQUIRE(LinkGraphJob::CanAllocateItem());
		auto job = std::unique_ptr<LinkGraphJob>(LinkGraphJob::Create(*LinkGraph::Get(graph)));
		LinkGraphSchedule::Run(job.get()); // Synchronous invocation of the real six native handlers isolates algorithm/publication from worker timing.
		REQUIRE(job->IsJobCompleted());
		job.reset(); // The actual native join/destructor publishes validated station flows.
		const auto &flows = this->interchange->goods[this->cargo].GetData().flows;
		const auto found = flows.find(this->origin->index);
		REQUIRE(found != flows.end());
		CHECK(found->second.GetShare(this->destination->index) > 0);
		CHECK(this->ghost->goods[this->cargo].link_graph == LinkGraphID::Invalid());
	}
};

/** Native orders and computed CargoDist move real finite mine cargo through a capsule interchange to a paid Surface factory input. */
TEST_CASE("Tube ordered CargoDist freight reaches a real Surface refinery once with empty returns", "[transport-tube-ordered-freight]")
{
	MockEnvironment::Instance(); LoadGeneratedWindowTestLanguage();
	AutoRestoreBackup<uint8_t> indicators{_settings_client.gui.loading_indicators, uint8_t{0}};
	TubeFactoryOrdersRig rig;
	// Empty observed services establish the actual native order edges first.
	// Local arrivals and explicit order skips remain declared boundary inputs.
	rig.OrderedArrival(*rig.capsule, rig.RailSite(), *rig.origin); rig.FinishOrdered(*rig.capsule, *rig.origin);
	rig.OrderedArrival(*rig.capsule, rig.tube_berth, *rig.interchange); rig.FinishOrdered(*rig.capsule, *rig.interchange);
	rig.OrderedArrival(*rig.freight, rig.freight_berth, *rig.interchange); rig.FinishOrdered(*rig.freight, *rig.interchange);
	rig.OrderedArrival(*rig.freight, rig.factory_berth, *rig.destination, Axis::Y); rig.FinishOrdered(*rig.freight, *rig.destination);
	const uint supplied = rig.Supply();
	INFO("Ordered freight: real mine stock supplied");
	rig.ComputeFlows();
	INFO("Ordered freight: real native CargoDist flows published");
	rig.OrderedArrival(*rig.capsule, rig.RailSite(), *rig.origin); rig.FinishOrdered(*rig.capsule, *rig.origin);
	const uint carried = rig.capsule->cargo.TotalCount();
	INFO("Ordered freight: capsule loaded its source packets");
	REQUIRE(carried > 0);
	const Money cash = Company::Get(_current_company)->money;
	const auto delivered = Company::Get(_current_company)->cur_economy.delivered_cargo[rig.cargo];
	rig.OrderedArrival(*rig.capsule, rig.tube_berth, *rig.interchange); rig.FinishOrdered(*rig.capsule, *rig.interchange);
	CHECK(rig.capsule->cargo.TotalCount() == 0);
	CHECK(rig.capsule->current_order.GetDestination() == rig.origin->index);
	CHECK(rig.interchange->goods[rig.cargo].TotalCount() == carried);
	CHECK(Company::Get(_current_company)->money == cash);
	CHECK(Company::Get(_current_company)->cur_economy.delivered_cargo[rig.cargo] == delivered);
	CHECK(rig.ghost->goods[rig.cargo].TotalCount() == 0);
	rig.OrderedArrival(*rig.freight, rig.freight_berth, *rig.interchange); rig.FinishOrdered(*rig.freight, *rig.interchange);
	const uint received = rig.wagon->cargo.TotalCount();
	INFO("Ordered freight: conventional wagon loaded the transferred packets");
	REQUIRE(received > 0);
	CHECK(rig.freight->current_order.GetDestination() == rig.destination->index);
	CHECK(Company::Get(_current_company)->money == cash);
	std::vector<CargoPacketID> packets;
	// Preserve exact original lineage across the real conventional portal
	// crossing; aggregate stock equality cannot prove packet custody alone.
	for (const CargoPacket *packet : *rig.wagon->cargo.Packets()) {
		packets.push_back(packet->index);
		CHECK(packet->GetFirstStation() == rig.origin->index);
		CHECK(packet->GetSource() == Source{rig.industry->index, SourceType::Industry});
	}
	rig.DriveFreightToSurface(rig.freight, rig.freight_depot);
	INFO("Ordered freight: conventional wagon reached Surface");
	std::vector<CargoPacketID> crossed;
	// The actual crossing must preserve each received packet before the
	// final delivery consumes it; a recreated load could otherwise hide loss.
	for (const CargoPacket *packet : *rig.wagon->cargo.Packets()) crossed.push_back(packet->index);
	CHECK(crossed == packets);
	uint ticks = 0;
	// Continue the same physical train to its actual Surface order destination;
	// final arrival is observed from native movement, not fixture repositioning.
	while (!rig.freight->current_order.IsType(OT_LOADING) && ticks < 3000) {
		++TimerGameTick::counter; REQUIRE(rig.freight->Tick()); ++ticks;
	}
	REQUIRE(ticks < 3000);
	REQUIRE(rig.freight->last_station_visited == rig.destination->index);
	INFO("Ordered freight: native movement reached the actual refinery station");
	rig.FinishOrdered(*rig.freight, *rig.destination);
	const Money paid = Company::Get(_current_company)->money;
	CHECK(paid > cash);
	CHECK(rig.wagon->cargo.TotalCount() == 0);
	CHECK(rig.factory->GetCargoAccepted(rig.cargo)->waiting == received);
	CHECK(Company::Get(_current_company)->cur_economy.delivered_cargo[rig.cargo] == delivered + received);
	CHECK(rig.freight->current_order.GetDestination() == rig.interchange->index);
	CHECK(GetTileLayer(rig.capsule->tile) == MapLayer::Underground);
	CHECK(rig.origin->goods[rig.cargo].TotalCount() + rig.interchange->goods[rig.cargo].TotalCount() + received == supplied);
	// Repeated empty delivery visits cannot settle the same packet or credit
	// factory stock again; the real order list remains the sole route owner.
	for (uint visit = 0; visit < 3; ++visit) {
		REQUIRE(CmdSkipToOrder(DoCommandFlag::Execute, rig.freight->index, VehicleOrderID{1}).Succeeded());
		ProcessOrders(rig.freight);
		rig.OrderedArrival(*rig.freight, rig.factory_berth, *rig.destination, Axis::Y);
		rig.FinishOrdered(*rig.freight, *rig.destination);
		CHECK(Company::Get(_current_company)->money == paid);
		CHECK(rig.factory->GetCargoAccepted(rig.cargo)->waiting == received);
		CHECK(Company::Get(_current_company)->cur_economy.delivered_cargo[rig.cargo] == delivered + received);
	}
}
