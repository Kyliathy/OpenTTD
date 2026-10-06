/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file layer_surface_manufacturing.cpp Actual Surface factories and packet custody bootstrap fuel, components, robotics and mirror kits without orbital production. @see docs/zz-reach2/upgrades/2026-10/r2mlvs6-mines-and-resource-chains.md, Group 12.4 */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "processor_fixture.h"
#include "../train.h"
#include "../economy_base.h"

#include "../safeguards.h"

/** Command-funded facilities and one isolated actual packet carrier; this checks manufacture/custody, not the later driven fleet scenario. */
class SurfaceManufacturingRig : public ProcessorRig {
	AutoRestoreBackup<bool> multiple{_settings_game.economy.multiple_industry_per_town, true}; ///< Ordinary game option permits two distinct chemical works in the fixture town.
	std::map<IndustryID, Station *> stations; ///< Actual source/receiving station identities, never cargo inventories.
	Train *carrier = nullptr; ///< Pool-owned packet carrier; no test substitute for native factory stocks.
public:
	/** Fund the first refinery and isolate an actual packet-owning carrier. */
	SurfaceManufacturingRig() : ProcessorRig(MultiLayerRecipeID{"CFMT"}, 10, 30)
	{
		REQUIRE(Vehicle::GetNumItems() == 0);
		REQUIRE(Train::CanAllocateItem());
		this->carrier = Train::Create();
		this->carrier->owner = _current_company;
	}
	/** Remove the isolated packet carrier before the parent removes native factories and their physical stock. */
	~SurfaceManufacturingRig() { _vehicle_pool.CleanPool(); }
	/** @param primary Frozen family identity. @param x Surface anchor column. @param y Surface anchor row. @return Actual command-created completed factory. */
	Industry &Build(MultiLayerRecipeID primary, uint x, uint y)
	{
		AutoRestoreBackup mode(_game_mode, GameMode::Editor); // Suppress only opening news in this isolated world, as the other command fixtures do.
		const IndustryType type = GetLayerProcessorIndustryType(primary);
		REQUIRE(type != IT_INVALID);
		const TileIndex anchor = TileXYL(x, y, MapLayer::Surface);
		REQUIRE(CmdBuildIndustry(DoCommandFlag::Execute, anchor, type, 0, true, 123).Succeeded());
		Industry &factory = *Industry::GetByTile(anchor);
		for (TileIndex tile : factory.location) {
			if (factory.TileBelongsToIndustry(tile)) SetIndustryCompleted(tile);
		}
		return factory;
	}
	/** @param factory Actual receiving factory. @param label Explicit initial delivered raw shipment. @param units Given whole units; procurement/fleet operation is outside this material boundary fixture. */
	static void Receive(Industry &factory, CargoLabel label, uint32_t units)
	{
		REQUIRE(ReceiveLayerProcessorCargo(factory, GetCargoTypeByLabel(label), units) == units);
	}
	/** @param factory Actual native inventory/work owner. @param batches Whole declared batches to work through bounded intervals. */
	static void Work(Industry &factory, uint batches)
	{
		const auto *record = GetLayerProcessorRecipe(factory);
		REQUIRE(record != nullptr);
		// Credit one recipe interval at a time, retaining the actual bounded work bank instead of treating a long interval as unlimited future work.
		for (uint batch = 0; batch < batches; ++batch) AdvanceLayerProcessor(factory, record->recipe.work_economy_ticks);
	}
	/** @param factory Actual facility. @return Its real adjacent same-layer station. */
	Station &StationFor(Industry &factory)
	{
		const auto found = this->stations.find(factory.index);
		if (found != this->stations.end()) return *found->second;
		Station *station = this->StationAt(TileAddWrap(factory.location.tile, -1, 0));
		for (const auto &output : factory.produced) {
			station->goods[output.cargo].rating = UINT8_MAX;
			station->goods[output.cargo].last_speed = 100;
		}
		this->stations.emplace(factory.index, station);
		return *station;
	}
	/** @param source Actual producer. @param target Different actual receiver. @param label Product meaning. @param units Required whole units. */
	void Ship(Industry &source, Industry &target, CargoLabel label, uint16_t units)
	{
		Station &origin = this->StationFor(source);
		Station &destination = this->StationFor(target);
		const CargoType cargo = GetCargoTypeByLabel(label);
		DispatchLayerProcessorCargo(source);
		REQUIRE(origin.goods[cargo].HasData());
		const std::array next{destination.index};
		REQUIRE(origin.goods[cargo].GetData().cargo.Load(units, &this->carrier->cargo, next, origin.xy) == units);
		this->carrier->last_station_visited = destination.index;
		REQUIRE(CargoPayment::CanAllocateItem());
		CargoPayment *payment = CargoPayment::Create(this->carrier);
		this->carrier->cargo_payment = payment;
		this->carrier->cargo.Stage(true, destination.index, {}, OrderUnloadType::UnloadIfPossible, &destination.goods[cargo], cargo, payment, destination.xy);
		REQUIRE(this->carrier->cargo.Unload(units, &destination.goods[cargo].GetOrCreateData().cargo, cargo, payment, destination.xy) == units);
		CHECK(this->carrier->cargo.TotalCount() == 0);
		CHECK(payment->route_profit == 0); // Reduced cargo fixture is zero-priced; this is exact material custody, not a financial balance claim.
		delete payment;
	}
	/** @param factory Actual product owner. @param label Stable product meaning. @return All remaining product in its own output plus real source-station packets. */
	uint32_t Remaining(Industry &factory, CargoLabel label)
	{
		const CargoType cargo = GetCargoTypeByLabel(label);
		const auto output = std::ranges::find(factory.produced, cargo, &Industry::ProducedCargo::cargo);
		REQUIRE(output != factory.produced.end());
		return output->waiting + this->StationFor(factory).goods[cargo].TotalCount();
	}
};

/** Real recipes and packet transfers produce every first-orbital manufacturing requirement entirely on Surface from explicit earlier raw shipments. */
TEST_CASE("Surface manufacture bootstraps fuel components robotics and mirror kits", "[layer-surface-manufacturing]")
{
	SurfaceManufacturingRig rig;
	Industry &metal = *rig.industry;
	Industry &chemical = rig.Build(MultiLayerRecipeID{"CHEM"}, 22, 30);
	Industry &propellant = rig.Build(MultiLayerRecipeID{"CHEM"}, 34, 30);
	Industry &battery = rig.Build(MultiLayerRecipeID{"BATT"}, 46, 30);
	Industry &electronics = rig.Build(MultiLayerRecipeID{"ELEC"}, 10, 46);
	Industry &components = rig.Build(MultiLayerRecipeID{"COMP"}, 22, 46);
	Industry &robotics = rig.Build(MultiLayerRecipeID{"RBOT"}, 34, 46);
	Industry &mirrors = rig.Build(MultiLayerRecipeID{"MKIT"}, 46, 46);
	REQUIRE(SelectLayerProcessorRecipe(propellant, MultiLayerRecipeID{"PCHM"}, _current_company, true));
	SurfaceManufacturingRig::Receive(metal, CargoLabel{"CORE"}, 20);
	SurfaceManufacturingRig::Receive(metal, CargoLabel{"COAL"}, 5);
	SurfaceManufacturingRig::Receive(chemical, CargoLabel{"SALT"}, 6);
	SurfaceManufacturingRig::Receive(chemical, CargoLabel{"OIL_"}, 6);
	SurfaceManufacturingRig::Receive(battery, CargoLabel{"LITH"}, 2);
	SurfaceManufacturingRig::Receive(battery, CargoLabel{"COBT"}, 1);
	SurfaceManufacturingRig::Receive(electronics, CargoLabel{"RARE"}, 2);
	SurfaceManufacturingRig::Receive(components, CargoLabel{"STEL"}, 4);
	SurfaceManufacturingRig::Work(metal, 5);
	SurfaceManufacturingRig::Work(chemical, 3);
	rig.Ship(chemical, propellant, CargoLabel{"CHEM"}, 2);
	rig.Ship(chemical, battery, CargoLabel{"CHEM"}, 1);
	rig.Ship(chemical, components, CargoLabel{"CHEM"}, 2);
	rig.Ship(metal, electronics, CargoLabel{"RFMT"}, 4);
	rig.Ship(metal, components, CargoLabel{"RFMT"}, 4);
	rig.Ship(metal, robotics, CargoLabel{"RFMT"}, 1);
	SurfaceManufacturingRig::Work(propellant, 1);
	SurfaceManufacturingRig::Work(battery, 1);
	SurfaceManufacturingRig::Work(electronics, 2);
	SurfaceManufacturingRig::Work(components, 2);
	rig.Ship(battery, robotics, CargoLabel{"BATY"}, 1);
	rig.Ship(electronics, robotics, CargoLabel{"ELEC"}, 2);
	rig.Ship(electronics, mirrors, CargoLabel{"ELEC"}, 1);
	rig.Ship(components, mirrors, CargoLabel{"SCMP"}, 2);
	SurfaceManufacturingRig::Work(robotics, 1);
	SurfaceManufacturingRig::Work(mirrors, 1);
	CHECK(rig.Remaining(propellant, CargoLabel{"PROP"}) == 2);
	CHECK(rig.Remaining(components, CargoLabel{"SCMP"}) == 2);
	CHECK(rig.Remaining(robotics, CargoLabel{"RBPT"}) == 2);
	CHECK(rig.Remaining(mirrors, CargoLabel{"SMIK"}) == 1);
	CHECK(rig.Remaining(metal, CargoLabel{"RFMT"}) == 1);
	CHECK(rig.Remaining(chemical, CargoLabel{"CHEM"}) == 1);
	CHECK(rig.Remaining(battery, CargoLabel{"BATY"}) == 1);
	CHECK(rig.Remaining(electronics, CargoLabel{"ELEC"}) == 1);
	// Every factory and actual stock owner is Surface, every supplied raw unit was consumed, and no orbital product secretly seeds another recipe.
	for (const Industry *factory : Industry::Iterate()) {
		CHECK(GetTileLayer(factory->location.tile) == MapLayer::Surface);
		for (const auto &input : factory->accepted) CHECK(input.waiting == 0);
	}
	CHECK(GetLayerProcessorState(metal.index)->batches.at(MultiLayerRecipeID{"CFMT"}) == 5);
	CHECK(GetLayerProcessorState(chemical.index)->batches.at(MultiLayerRecipeID{"CHEM"}) == 3);
}
