/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */
/**
 * @file utility_scheduler_mixed_fixture.h One native world for competing Tube, town, factory and processor work.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs18-geothermal-energy-and-industry.md.
 * Floor, receiver-boundary materials and consumed orbital frames are explicit
 * fixture inputs; no excavation, freight delivery or frame procurement is claimed.
 */
#ifndef TESTS_UTILITY_SCHEDULER_MIXED_FIXTURE_H
#define TESTS_UTILITY_SCHEDULER_MIXED_FIXTURE_H

#include "../stdafx.h"
#include <any> // Standard allocation internals precede fixture safeguards.
#include "../3rdparty/catch2/catch.hpp"
#include "mock_environment.h"
#include "movement_fixture.h"
#include "../space_structure.h"
#include "../space_structure_cmd.h"
#include "../orbital_manufacturing.h"
#include "../orbital_recipe.h"
#include "../hub_cargo.h"
#include "../tbm.h"
#include "../underground_vehicle.h"
#include "../engine_base.h"
#include "../engine_func.h"
#include "../train.h"
#include "../group.h"
#include "../vehicle_cmd.h"
#include "../depot_base.h"
#include "../order_cmd.h"
#include "../order_func.h"
#include "../signal_func.h"
#include "../landscape_cmd.h"
#include "../rail_map.h"
#include "../transport_tube_geothermal.h"
#include "geothermal_town_service_fixture.h"
#include "../safeguards.h"

/** Actual paid generator, direct town export, bought guideway/capsules and original orbital work owners sharing one native driver. */
class UtilitySchedulerMixedRig : public GeothermalTownServiceRig {
	decltype(_engine_mngr.mappings) mappings = _engine_mngr.mappings; ///< Caller engine transport restored after native hull teardown.
	AutoRestoreBackup<UnitID> trains{_settings_game.vehicle.max_trains, UnitID{500}}; ///< Ordinary native purchase limit.
	AutoRestoreBackup<uint8_t> length{_settings_game.vehicle.max_train_length, uint8_t{7}}; ///< Ordinary consist length limit.
	AutoRestoreBackup<Money> train_price{_price[Price::BuildVehicleTrain], Money{10000}}; ///< Explicit nonzero purchase baseline admits the contention fleet within the original million; every hull still pays its native model factor.
	AutoRestoreBackup<Money> depot_price{_price[Price::BuildDepotTrain], Money{10000}}; ///< Every depot pays its actual kernel invoice.
	AutoRestoreBackup<Money> rail_price{_price[Price::BuildRail], Money{1000}}; ///< Actual joining track invoice.
	AutoRestoreBackup<TimerGameCalendar::Year> year{TimerGameCalendar::year, TimerGameCalendar::Year{2000}}; ///< Actual dated source and model launch.
	AutoRestoreBackup<PauseModes> pause{_pause_mode, PauseModes{}}; ///< Normal synchronized opportunities throughout this owned world.
	AutoRestoreBackup<uint8_t> steel_weight{CargoSpec::Get(GetCargoTypeByLabel(CargoLabel{"STEL"}))->weight, uint8_t{16}}; ///< Explicit canonical one-tonne material dimension for native factory admission.
	PathfinderDefaults pathfinder; ///< Ordinary native route defaults, restored with this world.

	/** @param kind Actual canonical frame recipe. @param id Explicit isolated pool identity. @param x Space-local column. @param y Space-local row.
	 * @return Consumed-frame restore input; no physical frame construction claim.
	 */
	static SpaceStructureRecord Frame(SpaceBuildKind kind, uint16_t id, uint16_t x, uint16_t y)
	{
		const auto &recipe = *GetSpaceConstructionDefinition(kind); SpaceStructureRecord row;
		row.ref = {SpaceStructureID{id}, uint64_t{id} + 1}; row.state.owner = _current_company; row.state.kind = kind;
		row.state.x = x; row.state.y = y; row.state.capital_paid = recipe.capital; row.state.phase = SpaceStructurePhase::FrameComplete;
		row.work_ticks = recipe.work_ticks;
		row.consumed = {static_cast<uint16_t>(recipe.inputs[0].quantity), static_cast<uint16_t>(recipe.inputs[1].quantity)};
		return row;
	}

public:
	std::array<TileIndex, 2> inlets; ///< Real facing depots and their bought connecting guideway.
	SpaceStructure *core = nullptr, *assembly = nullptr, *processor = nullptr; ///< Actual independent Space consumers supplied by one native Core.
	TubeGeothermalSourceID source{UINT32_MAX}; ///< Original shared forwarding root; every capsule uses this same source.
	UtilityConsumerID town_consumer{UINT32_MAX}; ///< Actual original Surface town recipient, obtained from whole connection preflight.

	/** @param model Actual dated native generator whose real output determines the contention case. */
	explicit UtilitySchedulerMixedRig(GeothermalGeneratorModel model) : GeothermalTownServiceRig(true, model)
	{
		REQUIRE(Engine::GetNumItems() == 0); REQUIRE(Vehicle::GetNumItems() == 0); REQUIRE(SpaceStructure::GetNumItems() == 0); REQUIRE(Depot::GetNumItems() == 0);
		ResetSpaceStructures();
		REQUIRE(InitialiseTubeRailType(this->ActivatedContent()) != INVALID_RAILTYPE);
		REQUIRE(InitialiseTBMEngineDefinitions(this->ActivatedContent()));
		REQUIRE(InitialiseUndergroundEngineDefinitions(this->ActivatedContent()));
		REQUIRE(InitialiseTubeEngineDefinitions(this->ActivatedContent()));
		Company::Get(_current_company)->avail_railtypes.Set(GetTubeRailType());
		Engine::Get(GetTubeEngine(9))->company_avail.Set(_current_company);
		this->inlets = {TileXYL(32, 24, MapLayer::DeepUnderground), TileXYL(32, 26, MapLayer::DeepUnderground)};
		const std::array cells{this->plant_port, TileXYL(35, 24, MapLayer::DeepUnderground), TileXYL(34, 24, MapLayer::DeepUnderground),
			TileXYL(33, 24, MapLayer::DeepUnderground), this->inlets[0], TileXYL(32, 25, MapLayer::DeepUnderground), this->inlets[1]};
		this->Open(cells);
		// Facing purchased depots expose one ordinary native route. Their actual
		// guideway coexists with paid local cells rather than replacing either owner.
		for (size_t index = 0; index < this->inlets.size(); ++index) {
			const auto tile = this->inlets[index];
			REQUIRE(Command<Commands::BuildRailDepot>::Unsafe<CommandCallback>(INVALID_STRING_ID, nullptr, false, false, tile,
				std::make_tuple(tile, GetTubeRailType(), index == 0 ? DiagDirection::SE : DiagDirection::NW)).Succeeded());
		}
		const auto track = TileXYL(32, 25, MapLayer::DeepUnderground);
		REQUIRE(Command<Commands::BuildRail>::Unsafe<CommandCallback>(INVALID_STRING_ID, nullptr, false, false, track,
			std::make_tuple(track, GetTubeRailType(), Track::Y, false)).Succeeded());
		// Existing town/source cells retain their original identity and payment;
		// only missing local cells buy another native installation.
		for (const auto tile : cells) {
			if (!GetGeothermalUtilityLink(tile, UtilityEnergyDomain::Electricity).has_value()) this->Build(tile, UtilityEnergyDomain::Electricity);
		}
		const auto town = ConnectGeothermalTownService(this->service, false); REQUIRE(town.error == UtilityPowerError::None);
		this->town_consumer = town.destination;
		REQUIRE(this->Connect(DoCommandFlag::Execute).Succeeded());
		const auto connected = ConnectTubeGeothermalPower(this->generator, this->inlets[0]);
		REQUIRE(connected.error == TubePowerError::None); this->source = connected.source;
		const std::array rows{Frame(SpaceBuildKind::Core, 0, 4, 4), Frame(SpaceBuildKind::Assembly, 1, 7, 6),
			Frame(SpaceBuildKind::Storage, 2, 5, 7), Frame(SpaceBuildKind::Steelworks, 3, 1, 4)};
		REQUIRE(InstallLoadedSpaceStructures(rows, 5));
		this->core = SpaceStructure::Get(SpaceStructureID{0}); this->assembly = SpaceStructure::Get(SpaceStructureID{1});
		this->processor = SpaceStructure::Get(SpaceStructureID{3});
		// Every isolated frame earns activation through its native work owner and
		// the sole common scheduler. No work, electrical grant or source is injected.
		for (auto *site : {this->core, this->assembly, SpaceStructure::Get(SpaceStructureID{2}), this->processor}) {
			REQUIRE(CmdActivateOrbitalModule(DoCommandFlag::Execute, site->index, site->generation).Succeeded());
			for (uint tick = 0; tick < 148 && !IsOrbitalModuleOperational(*site); ++tick) {
				++TimerGameTick::counter; PrepareTubePowerTick(); TickOrbitalConstructionInventory();
			}
			REQUIRE(IsOrbitalModuleOperational(*site));
		}
		// Both real facilities compete for the original Core's bounded panel
		// output. Their Deep counterparts cannot borrow this separate Space offer.
		for (auto *site : {this->assembly, this->processor}) {
			REQUIRE(CmdConnectOrbitalServices(DoCommandFlag::Execute, site->index, site->generation, this->core->index, this->core->generation).Succeeded());
		}
		for (const auto &input : GetSpaceVehicleConstruction(16)->inputs) this->Add(*this->core, input.label, input.quantity); // One complete explicit receiver-boundary factory bill.
		this->Add(*this->processor, CargoLabel{"NORE"}, 4);
		REQUIRE(CmdQueueOrbitalManufacture(DoCommandFlag::Execute, this->assembly->index, this->assembly->generation,
			this->core->index, this->core->generation, 16).Succeeded());
		REQUIRE(CmdControlOrbitalRecipe(DoCommandFlag::Execute, this->processor->index, this->processor->generation, 0,
			static_cast<uint8_t>(OrbitalRecipeControl::Start)).Succeeded());
	}

	/** Remove original physical cargo/hulls before their pool identities, then restore caller engine transport. */
	~UtilitySchedulerMixedRig()
	{
		// Native deletion owns order, group and binding retirement before the
		// inherited generator, stations, company and physical world disappear.
		while (Vehicle::GetNumItems() != 0) delete (*Vehicle::Iterate().begin())->First();
		// Remove each actual paid depot before this world's map disappears.
		// Merely replacing its tiles would leak native DepotIDs into the next
		// replay and falsely attribute different source incarnations to a camera.
		for (const auto inlet : this->inlets) {
			if (!IsRailDepotTile(inlet)) continue;
			AutoRestoreBackup actor(_current_company, CompanyID{GetTileOwner(inlet)});
			CHECK(Command<Commands::LandscapeClear>::Unsafe<CommandCallback>(INVALID_STRING_ID, nullptr, false, false, inlet, std::make_tuple(inlet)).Succeeded());
		}
		CHECK(Depot::GetNumItems() == 0);
		ResetVehicleHash(); _group_pool.CleanPool(); _engine_pool.CleanPool(); _engine_mngr.mappings = std::move(this->mappings);
		// Consume through each actual sole packet container so a subsequent
		// isolated world cannot retain references to this world's retired stock.
		for (auto *site : SpaceStructure::Iterate()) {
			if (!site->module) continue;
			for (auto &stock : site->module->stock) if (stock.TotalCount() != 0) HubCargoTransfer::Consume(stock, stock.TotalCount());
			if (site->module->manufacture) {
				for (auto &stock : site->module->manufacture->inputs) if (stock.TotalCount() != 0) HubCargoTransfer::Consume(stock, stock.TotalCount());
			}
		}
		_space_structure_pool.CleanPool(); ResetSpaceStructures(); ResetTubePowerState();
	}

	/** @param site Sole native stock owner. @param label Explicit fixture cargo label. @param quantity Received native unit packets; no delivery claim. */
	void Add(SpaceStructure &site, CargoLabel label, uint16_t quantity)
	{
		const auto cargo = GetCargoTypeByLabel(label); REQUIRE(IsValidCargoType(cargo)); REQUIRE(CargoPacket::CanAllocateItem());
		site.module->stock[cargo].Append(CargoPacket::Create(quantity, uint16_t{7}, StationID::Invalid(), TileXYL(2, 2, MapLayer::Surface), Money{quantity * 10}), StationID::Invalid());
	}

	/** @param service Leave this actual capsule manually stopped with declared native damage for whole depot service. @return Command-paid capsule bound to the original geothermal path, with native orders only for the motion case. */
	Train *Buy(bool service = false)
	{
		const auto cash = Company::Get(_current_company)->money;
		const auto [result, id, capacity, mail, capacities] = Command<Commands::BuildVehicle>::Unsafe<CommandCallback>(INVALID_STRING_ID, nullptr, false, false, this->inlets[0],
			std::make_tuple(this->inlets[0], GetTubeEngine(9), false, INVALID_CARGO, ClientID{1}));
		INFO("capsule refusal=" << result.GetErrorMessage().base() << " cash=" << cash);
		REQUIRE(result.Succeeded()); REQUIRE(result.GetCost() > Money{0}); CHECK(Company::Get(_current_company)->money == cash - result.GetCost());
		auto *capsule = Train::Get(id); REQUIRE(BindTubeGeothermalPowerVehicle(id, this->source).has_value());
		if (service) {
			capsule->reliability = 100; VehicleServiceInDepot(capsule); REQUIRE(capsule->reliability == 100); // Unpaid native service must preserve the explicit damage boundary input.
			return capsule;
		}
		Order order; order.MakeGoToDepot(Depot::GetByTile(this->inlets[1])->index, {OrderDepotTypeFlag::PartOfOrders}, OrderNonStopFlag::NonStop, {OrderDepotActionFlag::Halt});
		REQUIRE(CmdInsertOrder(DoCommandFlag::Execute, id, 0, order).Succeeded()); ProcessOrders(capsule);
		REQUIRE(Command<Commands::StartStopVehicle>::Do(DoCommandFlag::Execute, id, true).Succeeded());
		return capsule;
	}

	/** @param capsules Actual native fronts offered one ordinary simulation tick after the common allocation; an empty set still runs native town/orbital owners. */
	void Opportunity(std::span<Train *const> capsules = {})
	{
		UpdateSignalsInBuffer(); ++TimerGameTick::counter; PrepareTubePowerTick();
		// Every actual front consumes only its original complete allocation;
		// orbital packet reservation, work and recipe owners then settle theirs.
		for (auto *capsule : capsules) REQUIRE(capsule->Tick());
		TickOrbitalManufacturingReservations(); TickOrbitalManufacturingWork(); TickOrbitalRecipes();
	}
};

#endif /* TESTS_UTILITY_SCHEDULER_MIXED_FIXTURE_H */
