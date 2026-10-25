/**
 * @file ground_station_selection.cpp Native station parent/child layouts and tile-local loading source selection.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group79FG.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
 * Command-created facilities/vehicles use explicit fixture floor and loading-list
 * boundary staging; this does not claim earned transport or actual viewport rendering.
 */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "mock_environment.h"
#include "layer_window_fixture.h"
#include "../ground_station_sprite.h"
#include "../station_func.h"
#include "../train_cmd.h"
#include "underground_fleet_fixture.h"

#include "ground_render_camera_fixture.h"
#include "../safeguards.h"

/** Native owned facilities and carriers, without a replacement catchment or loading-status owner. */
class GroundStationRig : public UndergroundFleetRig {
public:
	TileIndex tile; ///< Actual first cell of a command-built two-cell facility.
	Station *station; ///< Borrowed sole real native station/loading owner.

	/**
	 * Build one actual ordinary facility on disclosed fixture-opened floor.
	 * @param type Default Rail or Truck facility under test.
	 */
	explicit GroundStationRig(StationType type) : UndergroundFleetRig()
	{
		this->tile = this->Build(type, MapLayer::Underground);
		this->station = Station::GetByTile(this->tile);
	}

	/** Clear deliberately staged loading references before inherited ordinary vehicle/station teardown. */
	~GroundStationRig() { this->station->loading_vehicles.clear(); }

	/**
	 * Construct a real two-cell default facility through native admission.
	 * @param type Actual Rail or Truck construction command.
	 * @param layer Actual Surface or rock plane; only rock floor is fixture-opened.
	 * @return Actual first native station tile.
	 */
	TileIndex Build(StationType type, MapLayer layer)
	{
		const auto tile = TileXYL(12, 16, layer);
		if (layer != MapLayer::Surface) {
			// Expose only the actual footprint and its native end approaches;
			// this fixture setup does not claim player-paid excavation.
			if (type == StationType::Rail) {
				this->Open(std::array{TileAddWrap(tile, -1, 0), tile, TileAddWrap(tile, 1, 0), TileAddWrap(tile, 2, 0)});
			} else {
				this->Open(std::array{tile, TileAddWrap(tile, 1, 0), TileAddWrap(tile, 0, 1), TileAddWrap(tile, 1, 1)});
			}
		}
		if (type == StationType::Rail) {
			REQUIRE(CmdBuildRailStation(DoCommandFlag::Execute, tile, RAILTYPE_RAIL, Axis::X, 1, 2, STAT_CLASS_DFLT, 0, NEW_STATION, false).Succeeded());
		} else {
			REQUIRE(CmdBuildRoadStop(DoCommandFlag::Execute, tile, 2, 1, RoadStopType::Truck, false, DiagDirection::SE,
				ROADTYPE_ROAD, ROADSTOP_CLASS_DFLT, 0, NEW_STATION, false).Succeeded());
		}
		return tile;
	}
};

/**
 * Compare selected images and every sorting field with the actual native producer.
 * @param type Native default station family.
 * @param graphics Real native role/axis or entrance/through graphics.
 * @param state Exact accepted source state, including pure preview-only states.
 * @param frame Exact accepted source phase.
 */
static void CheckStationSourceLayout(StationType type, uint8_t graphics, uint8_t state, uint8_t frame)
{
	const auto layout = GetGroundStationArtLayout(type, graphics, state, frame);
	REQUIRE(layout.has_value());
	const auto *native = GetStationTileLayout(type, graphics);
	const auto expected = native->GetSequence();
	const auto view = layout->View();
	CHECK(view.ground.sprite == native->ground.sprite);
	CHECK(view.ground.pal == native->ground.pal);
	REQUIRE(view.GetSequence().size() == expected.size());
	const bool rail = type == StationType::Rail;
	// Preserve each exact native sorting box and child descriptor; comparing
	// to the live producer catches collapsed roofs, fabricated parents or an
	// attached glass image accidentally promoted into an independent building.
	for (uint8_t part = 0; part < expected.size(); ++part) {
		const auto &selected = view.GetSequence()[part];
		CHECK(selected.origin == expected[part].origin);
		CHECK(selected.extent == expected[part].extent);
		CHECK(selected.offset == expected[part].offset);
		CHECK(selected.IsParentSprite() == expected[part].IsParentSprite());
		CHECK(selected.image.pal == expected[part].image.pal);
		CHECK((selected.image.sprite & ~SPRITE_MASK) == (expected[part].image.sprite & ~SPRITE_MASK));
		CHECK((selected.image.sprite & SPRITE_MASK) == *GetGroundSprite(
			rail ? GroundSpriteFamily::FacilityRailStation : GroundSpriteFamily::FacilityRoadTerminal,
			{state, frame, static_cast<uint8_t>(rail ? graphics & 1 : graphics), static_cast<uint16_t>(rail ? graphics / 2 : 0), part}));
	}
}

/** All four native rail roles, both axes and source states/phases retain separate parent/roof/glass semantics. */
TEST_CASE("Selected station source preserves every native rail parent and glass child", "[ground-station-selection]")
{
	// Exhaust the complete qualified source matrix without inventing a world
	// construction stage. Preview-only states remain explicit pure source input.
	for (uint8_t graphics = 0; graphics < 8; ++graphics) {
		for (uint8_t state = 0; state < 4; ++state) {
			for (uint8_t frame = 0; frame < 4; ++frame) CheckStationSourceLayout(StationType::Rail, graphics, state, frame);
		}
	}
}

/** Four bay doors and two through axes preserve real parent order, omitting the absent third through parent. */
TEST_CASE("Selected terminal source preserves every native road sorting parent", "[ground-station-selection]")
{
	// Exhaust every accepted door/axis and source state/phase independently of
	// mutable road traffic; absent parents stay absent rather than sorting empty boxes.
	for (uint8_t graphics = 0; graphics < 6; ++graphics) {
		for (uint8_t state = 0; state < 4; ++state) {
			for (uint8_t frame = 0; frame < 4; ++frame) CheckStationSourceLayout(StationType::Truck, graphics, state, frame);
		}
	}
}

/** Actual loading train parts select working art only on their physical platform cells and never mutate custody. */
TEST_CASE("Selected rail station activity follows real tile-local loading parts", "[ground-station-selection]")
{
	MockEnvironment::Instance();
	LoadGeneratedWindowTestLanguage();
	GroundStationRig rig(StationType::Rail);
	const auto depot = rig.BuildDepot(MapLayer::Underground, false);
	Train *head = Train::From(rig.Buy(depot, 4));
	Train *wagon = Train::From(rig.Buy(depot, 5));
	REQUIRE(CmdMoveRailVehicle(DoCommandFlag::Execute, wagon->index, head->index, false).Succeeded());
	const auto other = TileAddWrap(rig.tile, 1, 0);
	const uint8_t graphics = GetStationGfx(rig.tile);
	const auto ready = GetGroundStationArtLayout(StationType::Rail, graphics, 1, 0);
	REQUIRE(ready.has_value());
	CHECK(GetGroundStationTileLayout(rig.tile, 24)->sequence[0].image.sprite == ready->sequence[0].image.sprite);
	rig.station->loading_vehicles.push_back(head);
	head->tile = other;
	wagon->tile = other;
	CHECK(GetGroundStationTileLayout(rig.tile, 24)->sequence[0].image.sprite == ready->sequence[0].image.sprite);
	wagon->tile = rig.tile;
	const auto cash = Company::Get(_current_company)->money;
	const auto geology = ComputeGeologyStateDigest(GetGeologyWorldState());
	const auto random = _random;
	const auto loading = rig.station->loading_vehicles;
	// A physical wagon on this tile activates its four actual authored working
	// phases even with the head on another platform cell; no station-wide cue is inferred.
	for (uint8_t frame = 0; frame < 4; ++frame) {
		const auto expected = GetGroundStationArtLayout(StationType::Rail, graphics, 2, frame);
		CHECK(GetGroundStationTileLayout(rig.tile, frame * 8)->sequence[0].image.sprite == expected->sequence[0].image.sprite);
	}
	CHECK(rig.station->loading_vehicles == loading);
	CHECK(wagon->cargo.StoredCount() == 0);
	CHECK(wagon->First() == head);
	CHECK(Company::Get(_current_company)->money == cash);
	CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == geology);
	CHECK(_random.state[0] == random.state[0]);
	CHECK(_random.state[1] == random.state[1]);
	head->tile = depot; // Restore staged physical occupancy before ordinary vehicle teardown.
	wagon->tile = depot;
}

/** Actual road loading cannot animate another stop tile, and unrelated train loading cannot impersonate road activity. */
TEST_CASE("Selected truck terminal activity follows actual matching loading carriers", "[ground-station-selection]")
{
	MockEnvironment::Instance();
	LoadGeneratedWindowTestLanguage();
	GroundStationRig rig(StationType::Truck);
	RoadVehicle *carrier = RoadVehicle::From(rig.Buy(rig.BuildDepot(MapLayer::Underground, true), 7));
	Train *train = Train::From(rig.Buy(rig.BuildDepot(MapLayer::Underground, false), 4));
	const auto road_depot = carrier->tile;
	const auto rail_depot = train->tile;
	const uint8_t graphics = GetStationGfx(rig.tile);
	const auto ready = GetGroundStationArtLayout(StationType::Truck, graphics, 1, 0);
	train->tile = rig.tile;
	rig.station->loading_vehicles.push_back(train);
	CHECK(GetGroundStationTileLayout(rig.tile, 24)->sequence[0].image.sprite == ready->sequence[0].image.sprite);
	rig.station->loading_vehicles.push_back(carrier);
	carrier->tile = TileAddWrap(rig.tile, 1, 0);
	CHECK(GetGroundStationTileLayout(rig.tile, 24)->sequence[0].image.sprite == ready->sequence[0].image.sprite);
	carrier->tile = rig.tile;
	const auto loading = rig.station->loading_vehicles;
	// Only the matching actual road part on this queried cell supplies working
	// phases; no copied stock or nearby vehicle search becomes an activity owner.
	for (uint8_t frame = 0; frame < 4; ++frame) {
		const auto expected = GetGroundStationArtLayout(StationType::Truck, graphics, 2, frame);
		CHECK(GetGroundStationTileLayout(rig.tile, frame * 8)->sequence[0].image.sprite == expected->sequence[0].image.sprite);
	}
	CHECK(rig.station->loading_vehicles == loading);
	CHECK(carrier->cargo.StoredCount() == 0);
	carrier->tile = road_depot; // Retire staged loading occupancy before normal fixture destruction.
	train->tile = rail_depot;
}

/** Existing geometry owners retain custom/Surface/Classic/fallback refusals without changing native station state. */
TEST_CASE("Selected station source refuses unrelated native tile and profile owners", "[ground-station-selection]")
{
	MockEnvironment::Instance();
	LoadGeneratedWindowTestLanguage();
	const auto type = GENERATE(StationType::Rail, StationType::Truck);
	GroundStationRig rig(type);
	CHECK_FALSE(GetGroundStationTileLayout(INVALID_TILE, 0));
	CHECK_FALSE(GetGroundStationTileLayout(TileXYL(8, 8, MapLayer::Underground), 0));
	const auto graphics = GetStationGfx(rig.tile);
	SetStationGfx(rig.tile, type == StationType::Rail ? 8 : 6);
	CHECK_FALSE(GetGroundStationTileLayout(rig.tile, 0));
	SetStationGfx(rig.tile, graphics);
	if (type == StationType::Rail) {
		SetCustomStationSpecIndex(rig.tile, 1);
		CHECK_FALSE(GetGroundStationTileLayout(rig.tile, 0));
		SetCustomStationSpecIndex(rig.tile, 0);
	} else {
		SetCustomRoadStopSpecIndex(rig.tile, 1);
		CHECK_FALSE(GetGroundStationTileLayout(rig.tile, 0));
		SetCustomRoadStopSpecIndex(rig.tile, 0);
	}
	CHECK_FALSE(GetGroundStationTileLayout(rig.Build(type, MapLayer::Surface), 0));
	CHECK(GetGroundStationTileLayout(rig.tile, 0).has_value());
	ResetMultiLayerProfileSelection();
	CHECK_FALSE(GetGroundStationTileLayout(rig.tile, 0));
}

/** Invalid source axes refuse; copying an owned layout retains native absent-parent and attached-glass behavior. */
TEST_CASE("Selected station source owns its spans and refuses invalid source axes", "[ground-station-selection]")
{
	CHECK_FALSE(GetGroundStationArtLayout(StationType::Bus, 0, 1, 0));
	CHECK_FALSE(GetGroundStationArtLayout(StationType::Rail, 8, 1, 0));
	CHECK_FALSE(GetGroundStationArtLayout(StationType::Truck, 6, 1, 0));
	CHECK_FALSE(GetGroundStationArtLayout(StationType::Rail, 0, 4, 0));
	CHECK_FALSE(GetGroundStationArtLayout(StationType::Rail, 0, 1, 4));
	CHECK(GetGroundStationArtLayout(StationType::Rail, 0, 1, 0)->count == 2);
	CHECK(GetGroundStationArtLayout(StationType::Truck, 4, 1, 0)->count == 2);
	GroundStationArtLayout retained;
	{
		const auto original = GetGroundStationArtLayout(StationType::Rail, 4, 1, 0);
		REQUIRE(original.has_value());
		retained = *original;
	}
	const auto view = retained.View();
	CHECK(view.GetSequence().data() == retained.sequence.data());
	REQUIRE(view.GetSequence().size() == 4);
	CHECK(view.GetSequence()[2].IsParentSprite());
	CHECK(view.GetSequence()[2].origin.z == 16);
	CHECK_FALSE(view.GetSequence()[3].IsParentSprite());
	CHECK(HasBit(view.GetSequence()[3].image.sprite, PALETTE_MODIFIER_TRANSPARENT));
	CHECK(view.GetSequence()[3].image.pal == PALETTE_TO_TRANSPARENT);
}

/** Default rail/road stations keep ordered foreground parents and native child images across view/visibility changes. */
TEST_CASE("Merged station parents retain actual tile ownership across cameras", "[merged-ground-camera]")
{
	PrepareGroundRenderCameraLanguage();
	const auto type = GENERATE(StationType::Rail, StationType::Truck); GroundStationRig rig(type);
	/** @return Actual tile-local sequence, including every retained parent/child image. */
	const auto query = [&] {
		const auto layout = GetGroundStationTileLayout(rig.tile, 24); REQUIRE(layout);
		std::vector<SpriteID> result{layout->ground.sprite};
		// Preserve every actual sequence member, including relative roof glass;
		// hidden flags must not reinterpret a foreground as another native parent.
		for (const auto &piece : layout->View().GetSequence()) result.push_back(piece.image.sprite);
		return result;
	};
	CheckGroundRenderCameraPurity(query);
}
