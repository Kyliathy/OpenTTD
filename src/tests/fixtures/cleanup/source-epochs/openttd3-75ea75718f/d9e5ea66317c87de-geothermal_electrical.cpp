/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file geothermal_electrical.cpp Actual native relay geometry, paid cable coexistence and bounded pure electrical path opportunities.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs18-geothermal-energy-and-industry.md Group5
 */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "geothermal_electrical_fixture.h"
#include "../landscape_cmd.h"
#include "../pbs.h"
#include "../safeguards.h"

/** Equal numerical relay accounts cannot bridge remote housings; every source and intermediate cable is ordinary paid native infrastructure. */
TEST_CASE("Electrical relay requires actual paid inlet and Underground cable paths", "[geothermal-electrical]")
{
	NativeElectricalRelayRig rig;
	CHECK_FALSE(FindGeothermalElectricalPath(rig.source, rig.destination_account, rig.source_port, 74).has_value());
	rig.InstallSourceCable();
	CHECK_FALSE(FindGeothermalElectricalPath(rig.source, rig.destination_account, rig.source_port, 74).has_value());
	const auto cash = Company::Get(rig.request.owner)->money;
	rig.InstallRelayCable(25);
	CHECK(Company::Get(rig.request.owner)->money == cash - Money{11000});
	CHECK_FALSE(FindGeothermalElectricalPath(rig.source, rig.destination_account, rig.source_port, 74).has_value());
	REQUIRE(ElectricalCableCommand(TileXYL(25, 20, MapLayer::Underground)).Succeeded());
	const auto utility = ComputeUtilityPowerDigest(), natural = ComputeGeologyStateDigest(GetGeologyWorldState()), construction = ComputeGeothermalRiserDigest();
	const auto random = std::array{_random.state[0], _random.state[1]};
	const auto path = FindGeothermalElectricalPath(rig.source, rig.destination_account, rig.source_port, 74);
	REQUIRE(path.has_value()); REQUIRE(path->legs.size() == 2);
	CHECK(path->legs[0].riser == rig.first); CHECK(path->legs[1].riser == rig.second);
	CHECK(path->legs[0].inlet.cells.size() == 2); CHECK(path->legs[1].inlet.cells.size() == 12);
	CHECK(path->capacity_w == 200000); CHECK(path->capacity_j == 399600); CHECK(path->topology_revision == GetUtilityLocalLinkRevision());
	CHECK(FindGeothermalElectricalPath(rig.source, rig.destination_account, rig.source_port, 1)->capacity_j == 5400);
	CHECK(ComputeUtilityPowerDigest() == utility); CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == natural); CHECK(ComputeGeothermalRiserDigest() == construction);
	CHECK(std::array{_random.state[0], _random.state[1]} == random);
	CHECK(CaptureUtilityPowerState().producers.empty()); CHECK(GetUtilityPowerConsumer(rig.destination_account)->meter.granted_energy_j == 0);
	CHECK(GetUtilityPowerLink(rig.first)->flow_j == 0); CHECK(GetUtilityPowerLink(rig.second)->flow_j == 0);
	CHECK(ValidateGeothermalUtilityLinks()); CHECK(ValidateUtilityPowerState(CaptureUtilityPowerState(), GetCurrentMultiLayerProfileSelection()) == UtilityPowerError::None);
}

/** New electrical geometry has explicit permitted planes and immutable costs; Heat, foreign transport, reserved housings and arbitrary industry cells refuse without writes. */
TEST_CASE("Electrical relay installation rejects unavailable native geometry atomically", "[geothermal-electrical]")
{
	NativeElectricalRelayRig rig;
	auto tile = TileXYL(25, 20, MapLayer::Underground); auto domain = UtilityEnergyDomain::Electricity;
	SECTION("Heat stays Deep") { domain = UtilityEnergyDomain::Heat; }
	SECTION("Surface has no rock cable") { tile = TileXYL(25, 20, MapLayer::Surface); }
	SECTION("Space has no rock cable") { tile = TileXYL(25, 20, MapLayer::Space); }
	SECTION("solid Underground") { tile = TileXYL(40, 40, MapLayer::Underground); }
	SECTION("generic machinery cell") { tile = TileAddWrap(rig.upper.port, 1, 0); }
	SECTION("reserved receiving housing") { tile = rig.destination; SetLayerPortalReservation(tile, true); }
	SECTION("foreign rail") { MakeRailNormal(tile, CompanyID{1}, TrackBits{Track::X}, RAILTYPE_RAIL); }
	SECTION("locked calendar") { TimerGameCalendar::date = TimerGameCalendar::ConvertYMDToDate(TimerGameCalendar::Year{1979}, 11, 31); }
	const auto digest = ComputeUtilityPowerDigest(), natural = ComputeGeologyStateDigest(GetGeologyWorldState()); const auto cash = Company::Get(rig.request.owner)->money;
	CHECK(ElectricalCableCommand(tile, domain).Failed()); CHECK(ComputeUtilityPowerDigest() == digest); CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == natural);
	CHECK(Company::Get(rig.request.owner)->money == cash);
}

/** Actual removal/rebuild produces fresh physical IDs, exclusions select only installed alternatives, and portal loss never repairs or regenerates an account. */
TEST_CASE("Electrical relay paths observe native split repair and original portal loss", "[geothermal-electrical]")
{
	NativeElectricalRelayRig rig; rig.InstallSourceCable(); rig.InstallRelayCable();
	const auto tile = TileXYL(25, 20, MapLayer::Underground); const auto old = GetGeothermalUtilityLink(tile, UtilityEnergyDomain::Electricity)->id;
	CHECK_FALSE(FindGeothermalElectricalPath(rig.source, rig.destination_account, rig.source_port, 74, std::array{old}).has_value());
	REQUIRE(ElectricalCableCommand(tile, UtilityEnergyDomain::Electricity, true).Succeeded());
	CHECK_FALSE(FindGeothermalElectricalPath(rig.source, rig.destination_account, rig.source_port, 74).has_value());
	REQUIRE(ElectricalCableCommand(tile).Succeeded()); CHECK(GetGeothermalUtilityLink(tile, UtilityEnergyDomain::Electricity)->id != old);
	REQUIRE(FindGeothermalElectricalPath(rig.source, rig.destination_account, rig.source_port, 74).has_value());
	SECTION("sealed original sender") { REQUIRE(ChangeLayerPortalConnection(rig.upper.portal, rig.request.owner, false, true) == LayerPortalWorkError::None); }
	SECTION("removed original housing") { REQUIRE(Command<Commands::LandscapeClear>::Do(DoCommandFlag::Execute, rig.upper.portal).Succeeded()); }
	SECTION("locked route") { TimerGameCalendar::date = TimerGameCalendar::ConvertYMDToDate(TimerGameCalendar::Year{1979}, 11, 31); }
	const auto digest = ComputeUtilityPowerDigest();
	CHECK_FALSE(FindGeothermalElectricalPath(rig.source, rig.destination_account, rig.source_port, 74).has_value()); CHECK(ComputeUtilityPowerDigest() == digest);
}

/** Sorted local traversal chooses installed residual alternatives and never invents a cable or unlimited duration when the preferred cell or paid edge is unavailable. */
TEST_CASE("Electrical relay alternatives retain bounded ordered physical references", "[geothermal-electrical]")
{
	NativeElectricalRelayRig rig; rig.InstallSourceCable(); rig.InstallRelayCable();
	const auto blocked = GetGeothermalUtilityLink(TileXYL(25, 20, MapLayer::Underground), UtilityEnergyDomain::Electricity)->id;
	// A real separately paid bypass creates a cycle in the installed local
	// network; stable cardinal BFS still visits each cell once per search.
	for (uint x = 24; x <= 26; ++x) REQUIRE(ElectricalCableCommand(TileXYL(x, 19, MapLayer::Underground)).Succeeded());
	const auto path = FindGeothermalElectricalPath(rig.source, rig.destination_account, rig.source_port, 74, std::array{blocked});
	REQUIRE(path.has_value()); REQUIRE(path->legs.size() == 2); CHECK(path->legs[1].inlet.cells.size() == 14);
	CHECK(std::ranges::find(path->legs[1].inlet.cells, blocked) == path->legs[1].inlet.cells.end());
	const auto repeat = FindGeothermalElectricalPath(rig.source, rig.destination_account, rig.source_port, 74, std::array{blocked});
	REQUIRE(repeat.has_value()); CHECK(repeat->legs[1].inlet.cells == path->legs[1].inlet.cells);
	CHECK_FALSE(FindGeothermalElectricalPath(rig.source, rig.destination_account, rig.source_port, 74, std::array{rig.second}).has_value());
	CHECK_FALSE(FindGeothermalElectricalPath(rig.source, rig.source, rig.source_port, 74).has_value());
	CHECK_FALSE(FindGeothermalElectricalPath(rig.source, rig.destination_account, rig.source_port, 0).has_value());
	CHECK_FALSE(FindGeothermalElectricalPath(rig.source, rig.destination_account, rig.source_port, MAX_UTILITY_INTERVAL_TICKS + 1).has_value());
}

/** Actual Underground transport commands and ordinary utility commands retain independent native ownership/track fields and exact installation expense. */
TEST_CASE("Underground relay cables preserve actual native road and rail metadata", "[geothermal-electrical]")
{
	NativeElectricalRelayRig rig; const auto tile = TileXYL(25, 20, MapLayer::Underground);
	SECTION("native rail") { REQUIRE(CmdBuildSingleRail(DoCommandFlag::Execute, tile, RAILTYPE_RAIL, Track::X, false).Succeeded()); }
	SECTION("native road") { REQUIRE(CmdBuildRoad(DoCommandFlag::Execute, tile, AxisToRoadBits(Axis::X), ROADTYPE_ROAD, DisallowedRoadDirections{}, TownID::Invalid()).Succeeded()); }
	const auto native = ElectricalNativeFloor(tile); const auto cash = Company::Get(rig.request.owner)->money;
	const auto before = ComputeUtilityPowerDigest();
	REQUIRE(ElectricalCableCommand(tile, UtilityEnergyDomain::Electricity, false, true).Succeeded()); CHECK(ComputeUtilityPowerDigest() == before);
	REQUIRE(ElectricalCableCommand(tile).Succeeded()); CHECK(ElectricalNativeFloor(tile) == native); CHECK(Company::Get(rig.request.owner)->money == cash - Money{1000});
	CHECK(GetUtilityPowerLink(GetGeothermalUtilityLink(tile, UtilityEnergyDomain::Electricity)->id)->local_layer == MapLayer::Underground);
}

/** A direct road elevator uses its own paid Deep inlet and Surface receiver; no Underground tile, utility or geological cell is opened as an intermediate relay. */
TEST_CASE("Direct electrical elevator leaves unrelated Underground untouched", "[geothermal-electrical]")
{
	NativeRiserRig rig(true); REQUIRE(rig.Fund().Succeeded()); rig.DeliverBill(); rig.Tick(296);
	const auto from = RegisterUtilityPowerConsumer(rig.request.owner, MapLayer::DeepUnderground, UtilityConsumerEffect::AssemblyWork, 1000000, {});
	const auto to = RegisterUtilityPowerConsumer(rig.request.owner, MapLayer::Surface, UtilityConsumerEffect::AssemblyWork, 1000000, {});
	REQUIRE(from.has_value()); REQUIRE(to.has_value()); REQUIRE(RegisterGeothermalRiserLink(rig.Riser().index, *from, *to).has_value());
	const auto underground = MultiLayerMap::TileInLayer(rig.request.portal, MapLayer::Underground);
	const auto native = ElectricalNativeFloor(underground); const auto natural = ComputeGeologyStateDigest(GetGeologyWorldState());
	const auto port = TileAddWrap(rig.request.port, 0, -1);
	REQUIRE(ElectricalCableCommand(port).Succeeded()); REQUIRE(ElectricalCableCommand(rig.request.port).Succeeded());
	const auto path = FindGeothermalElectricalPath(*from, *to, port, 74); REQUIRE(path.has_value()); CHECK(path->legs.size() == 1); CHECK(path->capacity_j == 399600);
	CHECK(ElectricalNativeFloor(underground) == native); CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == natural);
	CHECK_FALSE(GetGeothermalUtilityLink(underground, UtilityEnergyDomain::Electricity).has_value());
}
