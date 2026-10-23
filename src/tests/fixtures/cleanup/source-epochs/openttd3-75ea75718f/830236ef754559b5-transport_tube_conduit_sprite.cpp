/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file transport_tube_conduit_sprite.cpp Original paid path edges and publication select packed ground branches.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Group13CD.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "transport_tube_power_fixture.h"
#include "geothermal_tube_work_fixture.h"
#include "../transport_tube_sprite.h"
#include "../multilayer_sprite.h"
#include "../core/random_func.hpp"
#include "../safeguards.h"

/**
 * Require the exact declared source-state ports, including the absence of every other branch.
 * @param tile Actual original corridor cell.
 * @param states Expected NE/SE/SW/NW states: absent0, powered1 or unpowered2.
 */
static void CheckConduitPorts(TileIndex tile, std::array<uint8_t, 4> states)
{
	const auto art = GetTubeConduitArt(tile); REQUIRE(art.has_value());
	// Check the complete authored edge matrix, rather than merely counting
	// visible branches and allowing a false cross-link to pass unnoticed.
	for (uint8_t port = 0; port < states.size(); ++port) {
		CHECK(art->ports[port] == (states[port] == 0 ? std::nullopt : GetMultiLayerSprite(MultiLayerSpriteAsset::TubeConduit, {states[port], 0, port})));
	}
}

/** Real saved path order, rather than neighbouring cells, selects all four authored branch directions. */
TEST_CASE("Tube conduit ports follow original saved bends and overlapping paid paths", "[tube-conduit-art-path]")
{
	TubePowerRig world;
	const std::array path{world.path[0], TileXYL(39, 39, MapLayer::Underground), TileXYL(39, 40, MapLayer::Underground),
		TileXYL(39, 41, MapLayer::Underground), TileXYL(38, 41, MapLayer::Underground), TileXYL(37, 41, MapLayer::Underground),
		TileXYL(37, 40, MapLayer::Underground), world.depot};
	world.Open(path);
	CHECK_FALSE(GetTubeConduitArt(path[0]).has_value()); // Warm the empty derived index before the first real connection.
	const auto preview = ConnectTubeExternalPower(world.portal, world.depot, path, false);
	REQUIRE(preview.error == TubePowerError::None); CHECK_FALSE(GetTubeConduitArt(path[0]).has_value());
	const auto first = ConnectTubeExternalPower(world.portal, world.depot, path); REQUIRE(first.error == TubePowerError::None);
	CheckConduitPorts(path[0], {0, 0, 2, 0}); // Adjacent depot is not the next saved cell; no fabricated short circuit.
	CheckConduitPorts(path[1], {2, 2, 0, 0});
	CheckConduitPorts(path[2], {0, 2, 0, 2});
	CheckConduitPorts(path[3], {2, 0, 0, 2});
	CheckConduitPorts(path[4], {2, 0, 2, 0});
	CheckConduitPorts(path[5], {0, 0, 2, 2});
	CheckConduitPorts(path[6], {0, 2, 2, 0});
	CheckConduitPorts(world.depot, {2, 0, 0, 0});
	const auto destination = TileXYL(40, 40, MapLayer::Underground);
	const std::array second_path{path[0], path[1], TileXYL(40, 39, MapLayer::Underground), destination};
	world.Open(second_path); world.Open(std::array{TileXYL(40, 41, MapLayer::Underground)}); // Native depot admission also requires its real excavated door approach.
	const auto depot = CmdBuildTrainDepot(DoCommandFlag::Execute, destination, world.tube, DiagDirection::SE);
	INFO("depot error=" << depot.GetErrorMessage().base()); REQUIRE(depot.Succeeded());
	const auto second = ConnectTubeExternalPower(world.portal, destination, second_path); REQUIRE(second.error == TubePowerError::None);
	REQUIRE(PublishTubeExternalPower(second.producer, GetUtilityPowerInterval(), 1) == TubePowerError::None);
	CheckConduitPorts(path[0], {0, 0, 1, 0});
	CheckConduitPorts(path[1], {1, 2, 1, 0}); // Shared powered edge and separate original dark turn retain independent source states.
	REQUIRE(CloseTubeExternalPower(second.producer) == TubePowerError::None);
	CheckConduitPorts(path[1], {2, 2, 0, 0}); CHECK_FALSE(GetTubeConduitArt(destination).has_value());
	REQUIRE(CloseTubeExternalPower(first.producer) == TubePowerError::None);
	CHECK_FALSE(GetTubeConduitArt(path[0]).has_value()); CHECK_FALSE(GetTubeConduitArt(world.depot).has_value());
	CHECK_FALSE(GetTubeConduitArt(INVALID_TILE).has_value());
	CHECK_FALSE(GetTubeConduitArt(MultiLayerMap::TileInLayer(path[0], MapLayer::Surface)).has_value());
}

/** Display queries preserve all original energy, fees, bindings and random state, including retained date-locked machinery. */
TEST_CASE("Tube conduit publication and retained identity never acquire work", "[tube-conduit-art-purity]")
{
	TubePowerRig world; world.Connect(); world.Offer(540);
	CheckConduitPorts(world.path[0], {0, 1, 0, 0});
	const auto utility = ComputeUtilityPowerDigest(), invoices = ComputeTubePowerDigest(), bindings = ComputeTubePowerVehicleDigest();
	const auto cash = Company::Get(_current_company)->money;
	const auto random = std::array{_random.state[0], _random.state[1]};
	// Repeated camera-like observations and rebuilding the derived index must
	// not publish another interval, settle an invoice or consume the unused grant.
	for (uint query = 0; query < 20; ++query) { ResetTubeConduitArtCache(); CheckConduitPorts(world.path[0], {0, 1, 0, 0}); }
	CHECK(ComputeUtilityPowerDigest() == utility); CHECK(ComputeTubePowerDigest() == invoices); CHECK(ComputeTubePowerVehicleDigest() == bindings);
	CHECK(Company::Get(_current_company)->money == cash); CHECK(std::array{_random.state[0], _random.state[1]} == random);
	AutoRestoreBackup calendar(TimerGameCalendar::date, TimerGameCalendar::ConvertYMDToDate(TimerGameCalendar::Year{1969}, 11, 31));
	CHECK(CheckTubeConstruction(world.path).Failed()); CheckConduitPorts(world.path[0], {0, 1, 0, 0});
}

/** Installed native geothermal electricity belongs to its separate family, even when it genuinely powers an original capsule inlet. */
TEST_CASE("Tube conduit artwork cannot invent external paths over native purchased cables", "[tube-conduit-art-native]")
{
	MockEnvironment::Instance(); GeothermalTubeWorkRig world; world.JoinDepots(); const auto source = world.Connect();
	const auto cable = TileXYL(34, 24, MapLayer::DeepUnderground);
	REQUIRE(GetGeothermalUtilityLink(cable, UtilityEnergyDomain::Electricity).has_value());
	REQUIRE(GetTubeGeothermalSourcePath(source.source).has_value());
	const auto utility = ComputeUtilityPowerDigest(), native = ComputeGeothermalGeneratorDigest(), sources = ComputeTubeGeothermalPowerDigest();
	CHECK_FALSE(GetTubeConduitArt(cable).has_value()); CHECK_FALSE(GetTubeConduitArt(world.inlets[0]).has_value());
	CHECK(ComputeUtilityPowerDigest() == utility); CHECK(ComputeGeothermalGeneratorDigest() == native); CHECK(ComputeTubeGeothermalPowerDigest() == sources);
}
