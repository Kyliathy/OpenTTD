/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file portal_date.cpp Shared Deep date gate across actual build, routing, activation and tile admission. @see docs/zz-reach2/upgrades/2026-10/r2mlvs3-sealed-portals-and-deep-rail.md, Group 6 */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "excavation_fixture.h"
#include "../layer_portal_cmd.h"
#include "../layer_portal_map.h"
#include "../landscape.h"
#include "../tile_cmd.h"
#include "../vehicle_func.h"
#include "../train.h"
#include "../roadveh.h"
#include "../rail.h"
#include "../road.h"
#include "../rail_map.h"
#include "../timer/timer_game_economy.h"
#include "../core/backup_type.hpp"
#include "../script/api/script_rail.hpp"
#include "../script/api/script_road.hpp"
#include "../game/game_instance.hpp"
#include "../script/api/script_companymode.hpp"
#include "../script/api/script_testmode.hpp"
#include "../safeguards.h"

/** Script producers compile the actual typed public methods; full Squirrel execution remains a separate acceptance scenario. */
TEST_CASE("Portal scripting public methods compile against shared command types", "[portal-script]")
{
	STATIC_REQUIRE(std::is_invocable_r_v<bool, decltype(&ScriptRail::BuildLayerPortal), TileIndex, SQInteger, SQInteger>);
	STATIC_REQUIRE(std::is_invocable_r_v<bool, decltype(&ScriptRoad::BuildLayerPortal), TileIndex, SQInteger, SQInteger>);
	STATIC_REQUIRE(std::is_invocable_r_v<bool, decltype(&ScriptRail::ConnectLayerPortal), TileIndex, bool>);
	STATIC_REQUIRE(std::is_invocable_r_v<bool, decltype(&ScriptRoad::ConnectLayerPortal), TileIndex, bool>);
}

/** Scope adapter for the actual ScriptObject active-instance owner, without substituting a script storage or command pipeline. */
class PortalScriptScope : public ScriptObject {
	ActiveInstance active; ///< Actual script actor scope, restored before its GameInstance is destroyed.
public:
	/** @param game Actual Game Script runtime whose storage/command policy is exercised. */
	explicit PortalScriptScope(GameInstance &game) : active(game) {}
};

/** Actual Game Script company/test scopes exercise script commands through the real command kernel, without a copied gate or mutation fallback. */
TEST_CASE("Portal script runtime estimates share the Deep calendar gate", "[portal-script]")
{
	ExcavationFixture fixture;
	ResetLayerPortalWorkState(); ResetRailTypes(); ResetRoadTypes();
	GameInstance game;
	PortalScriptScope active(game);
	ScriptCompanyMode company(static_cast<ScriptCompany::CompanyID>(_current_company.base()));
	ScriptTestMode estimate;
	ScriptRail::SetCurrentRailType(static_cast<ScriptRail::RailType>(RAILTYPE_RAIL));
	ScriptRoad::SetCurrentRoadType(static_cast<ScriptRoad::RoadType>(ROADTYPE_ROAD));
	AutoRestoreBackup pause(_pause_mode, PauseModes{}); // The real command kernel receives normal unpaused estimates; direct-body fixtures' graphics suppression does not substitute for script permission.
	const auto underground = TileXYL(20, 20, MapLayer::Underground);
	const auto deep = MultiLayerMap::TileInLayer(underground, MapLayer::DeepUnderground);
	fixture.Open(std::array{underground, TileAddByDiagDir(underground, DiagDirection::NW), deep, TileAddByDiagDir(deep, DiagDirection::SE)});
	const auto geo = ComputeGeologyStateDigest(GetGeologyWorldState());
	const auto funds = Company::Get(_current_company)->money;
	const auto rng = _random;
	TimerGameCalendar::date = TimerGameCalendar::ConvertYMDToDate(TimerGameCalendar::Year{1979}, 11, 31);
	CHECK_FALSE(ScriptRail::BuildLayerPortal(underground, 3, 1));
	CHECK_FALSE(ScriptRail::BuildLayerPortal(deep, 1, 3));
	CHECK_FALSE(ScriptRoad::BuildLayerPortal(underground, 3, 1));
	CHECK_FALSE(ScriptRoad::BuildLayerPortal(deep, 1, 3));
	CHECK_FALSE(ScriptRoad::BuildLayerPortal(TileXYL(20, 20, MapLayer::Surface), 3, 1));
	TimerGameCalendar::date = TimerGameCalendar::ConvertYMDToDate(TimerGameCalendar::Year{1980}, 0, 1);
	CHECK(ScriptRail::BuildLayerPortal(underground, 3, 1));
	CHECK(ScriptRail::BuildLayerPortal(deep, 1, 3));
	CHECK(ScriptRoad::BuildLayerPortal(underground, 3, 1));
	CHECK(ScriptRoad::BuildLayerPortal(deep, 1, 3));
	CHECK(ScriptRoad::BuildLayerPortal(TileXYL(20, 20, MapLayer::Surface), 3, 1));
	CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == geo); CHECK(GetLayerPortalWorkPairCount() == 0);
	CHECK(Company::Get(_current_company)->money == funds);
	CHECK(_random.state[0] == rng.state[0]); CHECK(_random.state[1] == rng.state[1]);
}

/** Every supported Deep rail/elevator origin and direction consumes the same real command gate. */
TEST_CASE("Deep portal build gate covers ascent descent and every door", "[portal-date]")
{
	ExcavationFixture fixture;
	ResetLayerPortalWorkState(); ResetRailTypes(); ResetRoadTypes();
	const auto before = TimerGameCalendar::ConvertYMDToDate(TimerGameCalendar::Year{1979}, 11, 31);
	const auto on = TimerGameCalendar::ConvertYMDToDate(TimerGameCalendar::Year{1980}, 0, 1);
	const auto geo = ComputeGeologyStateDigest(GetGeologyWorldState());
	// Rail has only Underground/Deep; road elevators preserve both upper origins. Each ordered pair covers all four actual shaft headings.
	for (TransportType transport : {TransportType::Rail, TransportType::Road}) {
		for (MapLayer upper : {MapLayer::Surface, MapLayer::Underground}) {
			if (transport == TransportType::Rail && upper == MapLayer::Surface) continue; // Direct Surface/Deep rail is deliberately outside the accepted matrix.
			for (bool ascent : {false, true}) {
				const auto source_layer = ascent ? MapLayer::DeepUnderground : upper;
				const auto target_layer = ascent ? upper : MapLayer::DeepUnderground;
				const auto source = TileXYL(20, 20, source_layer);
				for (DiagDirection direction : {DiagDirection::NE, DiagDirection::SE, DiagDirection::SW, DiagDirection::NW}) {
					if (source_layer != MapLayer::Surface) fixture.Open(std::array{source, TileAddByDiagDir(source, ReverseDiagDir(direction))});
					TimerGameCalendar::date = before;
					CHECK(CmdBuildLayerPortal({DoCommandFlag::Execute}, source, transport, target_layer, direction, RAILTYPE_RAIL, ROADTYPE_ROAD).Failed());
					CHECK(GetTileType(source) == TileType::Clear);
					TimerGameCalendar::date = on;
					CHECK(CmdBuildLayerPortal({}, source, transport, target_layer, direction, RAILTYPE_RAIL, ROADTYPE_ROAD).Succeeded());
				}
			}
		}
	}
	CHECK(CaptureLayerPortalWorkRecords().empty());
	CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) != geo); // Only explicit fixture floor preparation changed geology; every build above was refused or an estimate.
}

/** Saved Connected topology still gates actual entry, even with faster economy time; explicit editor and Classic policies remain separate. */
TEST_CASE("Deep portal activation entry and preview share saved calendar policy", "[portal-date]")
{
	ExcavationFixture fixture;
	ResetLayerPortalWorkState(); ResetRailTypes(); ResetRoadTypes();
	TransportType transport = TransportType::Rail;
	MapLayer upper = MapLayer::Underground;
	SECTION("Underground Deep rail") {}
	SECTION("Surface Deep road elevator") { transport = TransportType::Road; upper = MapLayer::Surface; }
	SECTION("Underground Deep road elevator") { transport = TransportType::Road; }
	const auto source = TileXYL(20, 20, upper);
	const auto target = MultiLayerMap::TileInLayer(source, MapLayer::DeepUnderground);
	std::vector<TileIndex> floor{target, TileAddByDiagDir(target, DiagDirection::SE)};
	if (upper == MapLayer::Underground) { floor.push_back(source); floor.push_back(TileAddByDiagDir(source, DiagDirection::NW)); }
	fixture.Open(floor);
	if (transport == TransportType::Rail) {
		MakeRailLayerPortal(source, _current_company, DiagDirection::SE, MapLayer::DeepUnderground, RAILTYPE_RAIL);
		MakeRailLayerPortal(target, _current_company, DiagDirection::NW, upper, RAILTYPE_RAIL);
	} else {
		MakeRoadLayerPortal(source, _current_company, DiagDirection::SE, MapLayer::DeepUnderground, ROADTYPE_ROAD);
		MakeRoadLayerPortal(target, _current_company, DiagDirection::NW, upper, ROADTYPE_ROAD);
	}
	LayerPortalWorkRecord record;
	record.pair_key = source; record.company = _current_company; record.state = LayerPortalWorkState::Connected;
	REQUIRE(ValidateLayerPortalWorkRecords(std::array{record}, GetCurrentMultiLayerProfileSelection(), GetGeologyWorldState()) == LayerPortalWorkError::None);
	StageLoadedLayerPortalWork({record}); AcceptLoadedLayerPortalWork();
	// Activation validates actual built approaches as well as floor; prepare both through their public map constructors.
	for (TileIndex end : {source, target}) {
		const auto door = TileAddByDiagDir(end, GetLayerPortalDoorDirection(end));
		if (transport == TransportType::Rail) MakeRailNormal(door, _current_company, Track::Y, RAILTYPE_RAIL);
		else MakeRoadNormal(door, AxisToRoadBits(Axis::Y), ROADTYPE_ROAD, INVALID_ROADTYPE, TownID::Invalid(), _current_company, OWNER_NONE);
	}
	REQUIRE(Vehicle::GetNumItems() == 0);
	Vehicle *reference = nullptr;
	if (transport == TransportType::Rail) {
		REQUIRE(Train::CanAllocateItem());
		auto *train = Train::Create(); train->SetArticulatedPart(); reference = train;
	} else {
		REQUIRE(RoadVehicle::CanAllocateItem());
		auto *road = RoadVehicle::Create(); road->roadtype = ROADTYPE_ROAD; reference = road;
	}
	reference->owner = _current_company; reference->vehstatus.Set(VehState::Hidden);
	const auto geo = ComputeGeologyStateDigest(GetGeologyWorldState()), work = ComputeLayerPortalWorkDigest();
	TimerGameCalendar::date = TimerGameCalendar::ConvertYMDToDate(TimerGameCalendar::Year{1979}, 11, 31);
	AutoRestoreBackup economy(TimerGameEconomy::date, TimerGameEconomy::ConvertYMDToDate(TimerGameEconomy::Year{2090}, 0, 1));
	AutoRestoreBackup speed(_settings_game.economy.minutes_per_calendar_year, uint16_t{0});
	// Both endpoint orders must refuse topology activation, ordinary paths and actual tile admission without moving the real reference part.
	for (TileIndex end : {source, target}) {
		CHECK(CheckLayerPortalAvailability(end) == LayerPortalWorkError::LockedDeep);
		CHECK(ChangeLayerPortalConnection(end, _current_company, true, false) == LayerPortalWorkError::LockedDeep);
		LayerPortalCommissionRequest request;
		request.portal = end; request.proposal.company = _current_company; request.proposal.model_id = 1;
		CHECK(CommissionLayerPortalTBM(request, true).error == LayerPortalWorkError::LockedDeep);
		CHECK(GetTileTrackStatus(end, transport, transport == TransportType::Road ? RoadTramType::Road : RoadTramType::Invalid).trackdirs.None());
		CHECK(VehicleEnterTile(reference, end, TileX(end) * TILE_SIZE, TileY(end) * TILE_SIZE).Test(VehicleEnterTileState::CannotEnter));
	}
	CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == geo); CHECK(ComputeLayerPortalWorkDigest() == work);
	fixture.SetEditorOverride(true);
	CHECK(CheckLayerPortalAvailability(source) == LayerPortalWorkError::None); CHECK(CheckLayerPortalAvailability(target) == LayerPortalWorkError::None);
	fixture.SetEditorOverride(false);
	TimerGameCalendar::date = TimerGameCalendar::ConvertYMDToDate(TimerGameCalendar::Year{1980}, 0, 1);
	CHECK(GetTileTrackStatus(source, transport, transport == TransportType::Road ? RoadTramType::Road : RoadTramType::Invalid).trackdirs.Any());
	CHECK(GetTileTrackStatus(target, transport, transport == TransportType::Road ? RoadTramType::Road : RoadTramType::Invalid).trackdirs.Any());
	if (transport == TransportType::Road) {
		ResetLayerPortalWorkState(); ResetMultiLayerProfileSelection();
		TimerGameCalendar::date = TimerGameCalendar::ConvertYMDToDate(TimerGameCalendar::Year{1950}, 0, 1);
		CHECK(CheckLayerPortalAvailability(source) == LayerPortalWorkError::None); CHECK(CheckLayerPortalAvailability(target) == LayerPortalWorkError::None);
		CHECK(GetTileTrackStatus(source, transport, RoadTramType::Road).trackdirs.Any());
	}
	// The actual pool was empty at entry and this fixture owns its sole reference; this is admission testing, not a fleet movement scenario.
	REQUIRE(Vehicle::GetNumItems() == 1); _vehicle_pool.CleanPool(); ResetVehicleHash(); // Match actual world teardown: released fixture parts cannot remain in the derived spatial owner cache.
	ResetLayerPortalWorkState();
}
