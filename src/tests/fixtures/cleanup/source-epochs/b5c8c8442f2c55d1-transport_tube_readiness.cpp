/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file transport_tube_readiness.cpp Actual native facility identity, model/cargo and pure physical inlet views.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs16-underground-transport-tubes.md Group8
 */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "layer_window_fixture.h"
#include "transport_tube_meter_fixture.h"
#include "../station_cmd.h"
#include "../safeguards.h"

/** Views resolve actual native depot/capsule/cargo owners and reject invalid, foreign, ordinary and wrong-layer references. */
TEST_CASE("Tube public facility view retains native identity and exact capsule cargo admission", "[transport-tube-readiness]")
{
	TubePrototypeRig rig;
	const auto depot = GetTubeTerminal(rig.depot, _current_company); REQUIRE(depot.has_value());
	CHECK(depot->depot == Depot::GetByTile(rig.depot)->index); CHECK(depot->station == StationID::Invalid());
	CHECK(depot->layer == MapLayer::Underground); CHECK(depot->guideway == rig.tube); CHECK(depot->platform_length_tiles == 0);
	const auto readiness = GetTubeNetworkReadiness(rig.depot, _current_company); REQUIRE(readiness.has_value());
	CHECK(readiness->inlets.empty()); CHECK(CanTubeTerminalServeVehicle(rig.depot, _current_company, GetTubeEngine(9)));
	CHECK(CanTubeTerminalServeVehicle(rig.depot, _current_company, GetTubeEngine(9), GetCargoTypeByLabel(CargoLabel{"COAL"})));
	CHECK_FALSE(CanTubeTerminalServeVehicle(rig.depot, _current_company, GetTubeEngine(9), static_cast<CargoType>(NUM_CARGO)));
	CHECK_FALSE(CanTubeTerminalServeVehicle(rig.depot, _current_company, EngineID::Invalid()));
	CHECK_FALSE(GetTubeTerminal(INVALID_TILE, _current_company).has_value());
	CHECK_FALSE(GetTubeTerminal(rig.depot, CompanyID::Invalid()).has_value());
	CHECK_FALSE(GetTubeTerminal(MultiLayerMap::TileInLayer(rig.depot, MapLayer::Surface), _current_company).has_value());
	const auto ordinary = TileXYL(45, 45, MapLayer::Underground); rig.BuildTrack(ordinary, RAILTYPE_RAIL);
	CHECK_FALSE(GetTubeTerminal(ordinary, _current_company).has_value());
	REQUIRE(Company::CanAllocateItem()); Company *foreign = Company::Create();
	CHECK_FALSE(GetTubeTerminal(rig.depot, foreign->index).has_value());
	CHECK_FALSE(GetTubeNetworkReadiness(rig.depot, foreign->index).has_value()); delete foreign;
}

/** A loading-platform view retains its canonical station ID/geometry and never copies or allocates an alternate packet buffer. */
TEST_CASE("Tube public loading terminal uses actual station platform and cargo owner", "[transport-tube-readiness]")
{
	LoadGeneratedWindowTestLanguage(); TubePrototypeRig rig;
	const auto tile = TileXYL(50, 44, MapLayer::Underground);
	rig.Open(std::array{tile, TileAddWrap(tile, 1, 0), TileAddWrap(tile, -1, 0), TileAddWrap(tile, 2, 0)});
	REQUIRE(CmdBuildRailStation(DoCommandFlag::Execute, tile, rig.tube, Axis::X, 1, 2, STAT_CLASS_DFLT, 0, NEW_STATION, false).Succeeded());
	const auto terminal = GetTubeTerminal(tile, _current_company); REQUIRE(terminal.has_value());
	CHECK(terminal->station == GetStationIndex(tile)); CHECK(terminal->depot == DepotID::Invalid()); CHECK(terminal->platform_length_tiles == 2);
	CHECK(GetTubeTerminal(TileAddWrap(tile, 1, 0), _current_company)->station == terminal->station);
	const auto stations = Station::GetNumItems(), packets = CargoPacket::GetNumItems();
	CHECK(CanTubeTerminalServeVehicle(tile, _current_company, GetTubeEngine(9)));
	CHECK(GetTubeNetworkReadiness(tile, _current_company)->inlets.empty());
	CHECK(Station::GetNumItems() == stations); CHECK(CargoPacket::GetNumItems() == packets);
}

/** Source/financial readiness follows actual owned guideways and cash while remaining pure across repeated UI/camera-independent queries. */
TEST_CASE("Tube public power readiness changes only with actual service finance or layout", "[transport-tube-readiness]")
{
	TubeMeterRig rig; rig.Bind();
	const auto ledger = ComputeUtilityPowerDigest(), physical = ComputeTubePowerDigest(), bindings = ComputeTubePowerVehicleDigest();
	const auto revision = CSegmentCostCacheBase::s_rail_change_counter;
	const auto money = Company::Get(_current_company)->money;
	// Repeated downstream inspections cover a remote real depot on this
	// connected guideway without registering another source/consumer or grant.
	for (uint view = 0; view < 20; ++view) {
		const auto readiness = GetTubeNetworkReadiness(rig.destination, _current_company); REQUIRE(readiness.has_value());
		REQUIRE(readiness->inlets.size() == 1); CHECK(readiness->inlets[0].source == rig.source);
		CHECK(readiness->inlets[0].lease_payable); CHECK(readiness->inlets[0].usage_payable);
	}
	CHECK(ComputeUtilityPowerDigest() == ledger); CHECK(ComputeTubePowerDigest() == physical); CHECK(ComputeTubePowerVehicleDigest() == bindings);
	CHECK(Company::Get(_current_company)->money == money); CHECK(CSegmentCostCacheBase::s_rail_change_counter == revision);
	TimerGameEconomy::date = TimerGameEconomy::Date{101}; Company::Get(_current_company)->money = Money{0};
	const auto unpaid = GetTubeNetworkReadiness(rig.depot, _current_company); REQUIRE(unpaid.has_value()); REQUIRE(unpaid->inlets.size() == 1);
	CHECK_FALSE(unpaid->inlets[0].lease_payable); CHECK_FALSE(unpaid->inlets[0].usage_payable);
	CHECK(ComputeUtilityPowerDigest() == ledger); CHECK(ComputeTubePowerDigest() == physical);
	Company::Get(_current_company)->money = money;
	const auto segment = TileXYL(38, 41, MapLayer::Underground);
	REQUIRE(CmdRemoveSingleRail(DoCommandFlag::Execute, segment, Track::Y).Succeeded());
	CHECK(GetTubeNetworkReadiness(rig.destination, _current_company)->inlets.empty());
	REQUIRE(CmdBuildSingleRail(DoCommandFlag::Execute, segment, rig.tube, Track::Y, false).Succeeded());
	CHECK(GetTubeNetworkReadiness(rig.destination, _current_company)->inlets.size() == 1);
	REQUIRE(CloseTubeExternalPower(rig.source) == TubePowerError::None);
	CHECK(GetTubeNetworkReadiness(rig.destination, _current_company)->inlets.empty());
}
