/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file geothermal_tube_capsule_fixture.h Actual paid native plant/inlet and command-purchased production capsule binding participants.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Group6BCC.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
 * Prepared floor/materials and explicit nonzero prices are boundary inputs, separate from paid-TBM/bootstrap and powered-route acceptance.
 */
#ifndef TESTS_GEOTHERMAL_TUBE_CAPSULE_FIXTURE_H
#define TESTS_GEOTHERMAL_TUBE_CAPSULE_FIXTURE_H

#include "geothermal_tube_power_fixture.h"
#include "../tbm.h"
#include "../underground_vehicle.h"
#include "../engine_base.h"
#include "../engine_func.h"
#include "../vehicle_cmd.h"
#include "../vehicle_func.h"
#include "../train.h"
#include "../group.h"

/** Actual2000 paid source and owned production capsules, retaining original native pool, engine and cargo owners. */
class GeothermalTubeCapsuleRig : public GeothermalTubePowerRig {
	decltype(_engine_mngr.mappings) mappings = _engine_mngr.mappings; ///< Restore caller EIDS mappings after native fleet teardown.
	AutoRestoreBackup<UnitID> trains{_settings_game.vehicle.max_trains, UnitID{500}}; ///< Explicit normal native purchase limit.
	AutoRestoreBackup<uint8_t> length{_settings_game.vehicle.max_train_length, uint8_t{7}}; ///< Ordinary whole-consist length bound.
	AutoRestoreBackup<Money> train_price{_price[Price::BuildVehicleTrain], Money{100000}}; ///< Actual kernel purchase invoices must have a nonzero baseline.
	AutoRestoreBackup<Money> rail_price{_price[Price::BuildRail], Money{1000}}; ///< Real physical joining track purchase, separate from electrical cells.
	AutoRestoreBackup<TimerGameCalendar::Year> year{TimerGameCalendar::year, TimerGameCalendar::Year{2000}}; ///< Match the actual Magma fixture date during native engine setup.
public:
	/** @param serialized Match shipped native content/engine slots before any actual vehicle is purchased. */
	explicit GeothermalTubeCapsuleRig(bool serialized = false) : GeothermalTubePowerRig(GeothermalGeneratorModel::MagmaLoop, serialized)
	{
		REQUIRE(Engine::GetNumItems() == 0); REQUIRE(Vehicle::GetNumItems() == 0);
		if (serialized) {
			_engine_mngr.ResetToDefaultMapping(); SetupEngines(); // Genuine whole-world engine transport reserves original identities before appending actual first-party definitions.
		}
		REQUIRE(InitialiseTBMEngineDefinitions(this->ActivatedContent()));
		REQUIRE(InitialiseUndergroundEngineDefinitions(this->ActivatedContent()));
		REQUIRE(InitialiseTubeEngineDefinitions(this->ActivatedContent()));
		const auto engine = GetTubeEngine(9); REQUIRE(engine != EngineID::Invalid());
		Engine::Get(engine)->company_avail.Set(_current_company); // Explicit ordinary company availability does not bypass canonical model/date/cargo admission in the command.
	}
	/** Retire actual native chains/orders/bindings before their source/facilities disappear, then restore caller engine identities. */
	~GeothermalTubeCapsuleRig()
	{
		// Native destruction owns packet/order/group cleanup and invokes actual
		// recipient retirement before engine and inherited facility pools vanish.
		while (Vehicle::GetNumItems() != 0) delete (*Vehicle::Iterate().begin())->First();
		_group_pool.CleanPool(); _engine_pool.CleanPool(); _engine_mngr.mappings = std::move(this->mappings);
	}
	/** @param inlet Existing actual owned tube purchase depot index. @return Kernel-paid production capsule, with its original native identity and default real cargo capability. */
	Train *Buy(size_t inlet = 0)
	{
		const auto cash = Company::Get(_current_company)->money;
		const auto [result, id, capacity, mail, capacities] = Command<Commands::BuildVehicle>::Unsafe<CommandCallback>(INVALID_STRING_ID, nullptr, false, false, this->inlets[inlet],
			std::make_tuple(this->inlets[inlet], GetTubeEngine(9), false, INVALID_CARGO, ClientID{1}));
		INFO("native capsule engine=" << GetTubeEngine(9).base() << " refusal=" << result.GetErrorMessage().base() << " cash=" << cash);
		REQUIRE(result.Succeeded()); REQUIRE(result.GetCost() > Money{0});
		CHECK(Company::Get(_current_company)->money == cash - result.GetCost());
		return Train::Get(id);
	}
	/** Rebuild the second original fixture depot facing the first, then buy the actual intervening guideway; the first source inlet is unchanged. */
	void JoinDepots()
	{
		REQUIRE(Vehicle::GetNumItems() == 0);
		const auto depot = this->inlets[1];
		REQUIRE(Command<Commands::LandscapeClear>::Unsafe<CommandCallback>(INVALID_STRING_ID, nullptr, false, false, depot, std::make_tuple(depot)).Succeeded());
		REQUIRE(Command<Commands::RemoveGeothermalUtilityLink>::Unsafe<CommandCallback>(INVALID_STRING_ID, nullptr, false, false, depot,
			std::make_tuple(depot, UtilityEnergyDomain::Electricity)).Succeeded());
		REQUIRE(Command<Commands::BuildRailDepot>::Unsafe<CommandCallback>(INVALID_STRING_ID, nullptr, false, false, depot,
			std::make_tuple(depot, GetTubeRailType(), DiagDirection::NW)).Succeeded());
		this->Build(depot, UtilityEnergyDomain::Electricity);
		const auto track = TileXYL(32, 25, MapLayer::DeepUnderground);
		REQUIRE(Command<Commands::BuildRail>::Unsafe<CommandCallback>(INVALID_STRING_ID, nullptr, false, false, track,
			std::make_tuple(track, GetTubeRailType(), Track::Y, false)).Succeeded());
	}
};

#endif /* TESTS_GEOTHERMAL_TUBE_CAPSULE_FIXTURE_H */
