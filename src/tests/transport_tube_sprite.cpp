/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file transport_tube_sprite.cpp Actual command-built tube layouts and original-source presentation purity.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Group13B.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "transport_tube_meter_fixture.h"
#include "geothermal_tube_capsule_fixture.h"
#include "../transport_tube_sprite.h"
#include "../ground_sprite.h"
#include "../core/random_func.hpp"
#include "../safeguards.h"

/** Every command-built native plain layout must select its own exact authored arms and complete physical ends. */
TEST_CASE("Tube guideway art selects all63 native layouts without inventing junction arms", "[tube-guideway-art-geometry]")
{
	TubePrototypeRig world;
	// This one art matrix constructs63 layouts plus its independent ordinary
	// negative site. Declare enough native clearing quota for the whole fixture;
	// the default64-cell quota also pays for its pre-existing purchase depot.
	AutoRestoreBackup clearing(Company::Get(_current_company)->clear_limit, uint32_t{128} << 16);
	uint junction = 0;
	// Give each native layout an isolated actual tile, with real commands for
	// every segment. No synthetic track map or source-art-only DTO can pass.
	for (uint mask = 1; mask < 64; ++mask) {
		CAPTURE(mask);
		const TileIndex tile = TileXYL(44 + ((mask - 1) % 8) * 2, 44 + ((mask - 1) / 8) * 2, MapLayer::Underground);
		const TrackBits tracks{static_cast<uint8_t>(mask)};
		world.Open(std::array{tile});
		// Each installed bit must survive native admission on an interior cell;
		// map-border setbacks are separate from the geometry artwork contract.
		for (Track track : tracks) {
			const auto result = CmdBuildSingleRail(DoCommandFlag::Execute, tile, world.tube, track, false);
			CAPTURE(mask, tile, track, result.GetErrorMessage());
			REQUIRE(result.Succeeded());
		}
		const auto art = GetTubeGuidewayArt(tile); REQUIRE(art.has_value()); CHECK_FALSE(art->energized);
		uint pieces = 1;
		if (mask == 12 || mask == 48) pieces = 2; // Independent parallel curves must remain separate pieces rather than gain a shared junction.
		const auto ends = GetTubeGuidewayEnds(tile);
		CHECK(art->count == pieces + std::ranges::count(ends, true));
		// Original source matrix order is independent of the selector's branch
		// implementation: singles use their declared source views, followed by
		// crossing/parallel cases and the54 ascending authored junction masks.
		if (std::popcount(mask) == 1) {
			const uint track = std::countr_zero(mask);
			const auto asset = track < 2 ? GroundSpriteFamily::TubeStraight : GroundSpriteFamily::TubeBend45;
			CHECK(art->parts[0].back == *GetGroundSprite(asset, {2, 0, static_cast<uint8_t>(track < 2 ? track : track - 2)}));
		} else if (mask == 3) {
			CHECK(art->parts[0].back == *GetGroundSprite(GroundSpriteFamily::TubeCrossing, {2}));
		} else if (pieces == 2) {
			CHECK(art->parts[0].back != art->parts[1].back);
		} else {
			CHECK(art->parts[0].back == *GetGroundSprite(GroundSpriteFamily::TubeJunction, {2, 0, 0, static_cast<uint16_t>(junction)}));
			++junction;
		}
		// Every aligned front must remain in the same family/state/view/cell,
		// rather than accidentally selecting another mask or source state.
		for (uint index = 0; index < art->count; ++index) {
			CHECK(art->parts[index].front == art->parts[index].back + 1);
			// Every actual part needs a nonempty sorter box while retaining its
			// original ground anchor. A default/shifted new curve box must not
			// silently remove a rim or move source pixels onto another tile.
			const auto &bounds = art->parts[index].front_bounds;
			CHECK(bounds.origin.x + bounds.offset.x == 0); CHECK(bounds.origin.y + bounds.offset.y == 0);
			CHECK(bounds.origin.z + bounds.offset.z == 0);
			CHECK(bounds.extent.x > 0); CHECK(bounds.extent.y > 0); CHECK(bounds.extent.z > 0);
		}
	}
	CHECK(junction == 54);
	CHECK_FALSE(GetTubeGuidewayArt(INVALID_TILE).has_value());
	CHECK_FALSE(GetTubeGuidewayArt(world.depot).has_value());
	// Conventional rail must retain its own native art even after this complete
	// geometry matrix has installed the actual dedicated tube family.
	const auto ordinary = TileXYL(32, 52, MapLayer::Underground); world.Open(std::array{ordinary});
	const auto result = CmdBuildSingleRail(DoCommandFlag::Execute, ordinary, RAILTYPE_RAIL, Track::X, false);
	INFO("ordinary=" << ordinary.base() << " error=" << result.GetErrorMessage().base() << " type=" << to_underlying(GetTileType(ordinary)) <<
		" company=" << static_cast<uint>(_current_company.base()) << " rail_admitted=" << ValParamRailType(RAILTYPE_RAIL));
	REQUIRE(result.Succeeded());
	CHECK_FALSE(GetTubeGuidewayArt(ordinary).has_value());
}

/** Native physical neighbour mouths remove only the corresponding end, independently of rail signals and power allocation. */
TEST_CASE("Tube guideway fronts keep actual end adjacency and ground registration", "[tube-guideway-art-ends]")
{
	TubePrototypeRig world;
	const auto tile = TileXYL(48, 48, MapLayer::Underground);
	world.BuildTrack(tile, world.tube, Track::X);
	REQUIRE(GetTubeGuidewayArt(tile).has_value()); CHECK(GetTubeGuidewayArt(tile)->count == 3);
	world.BuildTrack(TileAddByDiagDir(tile, DiagDirection::NE), world.tube, Track::X);
	CHECK(GetTubeGuidewayArt(tile)->count == 2);
	world.BuildTrack(TileAddByDiagDir(tile, DiagDirection::SW), world.tube, Track::X);
	CHECK(GetTubeGuidewayArt(tile)->count == 1);
	const auto &bounds = GetTubeGuidewayFrontBounds();
	CHECK(bounds.origin.x + bounds.offset.x == 0); CHECK(bounds.origin.y + bounds.offset.y == 0);
	CHECK(bounds.origin.z + bounds.offset.z == 0); CHECK(bounds.extent.z == 4);
	// Retained actual guideways keep their native visual identity when the
	// calendar is unavailable for new construction. This observation grants
	// neither new track nor energy and cannot bypass command availability.
	AutoRestoreBackup calendar(TimerGameCalendar::date, TimerGameCalendar::ConvertYMDToDate(TimerGameCalendar::Year{1969}, 11, 31));
	CHECK(CheckTubeConstruction(std::array{tile}).Failed());
	REQUIRE(GetTubeGuidewayArt(tile).has_value()); CHECK_FALSE(GetTubeGuidewayArt(tile)->energized);
}

/** Rendering existing paid publication neither invoices again nor acquires a work grant, and explicit closure extinguishes it. */
TEST_CASE("Tube guideway art reads actual external publication without charging work", "[tube-guideway-art-external]")
{
	TubeMeterRig world; world.Bind();
	const auto tile = TileXYL(38, 43, MapLayer::Underground);
	CHECK_FALSE(GetTubeGuidewayArt(tile)->energized);
	world.Run();
	REQUIRE(GetUtilityPowerProducer(world.source)->meter.generation_energy_j != 0);
	CHECK(GetTubeGuidewayArt(tile)->energized);
	const auto utility = ComputeUtilityPowerDigest(), power = ComputeTubePowerDigest(), binding = ComputeTubePowerVehicleDigest();
	const auto cash = Company::Get(_current_company)->money;
	const auto random = std::array{_random.state[0], _random.state[1]};
	// Repeated camera-like observations may refresh a derived circuit only;
	// original energy, invoices, cash, bindings and simulation RNG must be exact.
	for (uint query = 0; query < 20; ++query) CHECK(GetTubeGuidewayArt(tile)->energized);
	CHECK(ComputeUtilityPowerDigest() == utility); CHECK(ComputeTubePowerDigest() == power); CHECK(ComputeTubePowerVehicleDigest() == binding);
	CHECK(Company::Get(_current_company)->money == cash); CHECK(std::array{_random.state[0], _random.state[1]} == random);
	REQUIRE(CloseTubeExternalPower(world.source) == TubePowerError::None);
	CHECK_FALSE(GetTubeGuidewayArt(tile)->energized);
}

/** Original native generation lights only its own physically reached guideway; cable loss/replacement retains energy and source identity. */
TEST_CASE("Tube guideway art follows original geothermal cable loss and repair", "[tube-guideway-art-geothermal]")
{
	MockEnvironment::Instance(); GeothermalTubeCapsuleRig world; world.JoinDepots();
	const auto source = world.Connect();
	const auto tile = TileXYL(32, 25, MapLayer::DeepUnderground);
	REQUIRE(GetTubeGuidewayArt(tile).has_value()); CHECK(GetTubeGuidewayArt(tile)->energized);
	const auto utility = ComputeUtilityPowerDigest(), native = ComputeGeothermalGeneratorDigest(), original = ComputeTubeGeothermalPowerDigest();
	const auto cash = Company::Get(_current_company)->money;
	// Pure source/path selection cannot earn another native generation interval,
	// pay operating service or alter the retained original inlet incarnation.
	for (uint query = 0; query < 20; ++query) CHECK(GetTubeGuidewayArt(tile)->energized);
	CHECK(ComputeUtilityPowerDigest() == utility); CHECK(ComputeGeothermalGeneratorDigest() == native); CHECK(ComputeTubeGeothermalPowerDigest() == original);
	CHECK(Company::Get(_current_company)->money == cash);
	const auto cable = TileXYL(34, 24, MapLayer::DeepUnderground);
	REQUIRE(Command<Commands::RemoveGeothermalUtilityLink>::Do(DoCommandFlag::Execute, cable, UtilityEnergyDomain::Electricity).Succeeded());
	CHECK_FALSE(GetTubeGuidewayArt(tile)->energized);
	world.Build(cable, UtilityEnergyDomain::Electricity);
	CHECK(GetTubeGuidewayArt(tile)->energized);
	CHECK(GetTubeGeothermalSource(source.source)->producer == world.output);
}
