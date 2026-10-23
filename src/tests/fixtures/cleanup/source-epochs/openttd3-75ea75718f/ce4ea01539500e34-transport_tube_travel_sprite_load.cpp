/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file transport_tube_travel_sprite_load.cpp Genuine capsule headings and native picking on an original paid curved route.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Groups13DB/13DCB.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
 * Prepared geological floor and original commissioned portal are declared boundary inputs, not useful-route bootstrap acceptance.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include <charconv>
#include "test_artifact_paths.h"
#include "layer_window_fixture.h"
#include "layer_save_fixture.h"
#include "native_art_render_fixture.h"
#include "geothermal_tube_switch_fixture.h"
#include "../transport_tube_sprite.h"
#include "../multilayer_sprite.h"
#include "../rail.h"
#include "../track_func.h"
#include "../vehicle_gui.h"
#include "../newgrf_station.h"
#include "../station_func.h"
#include "../openttd.h"
#include "../console_internal.h"
#include "../saveload/saveload_func.h"
#include "../blitter/factory.hpp"
#include "../fontcache.h"
#include "../spritecache.h"
#include "../safeguards.h"

/** @param name Required independent decimal process expectation. @return Complete checked unsigned native value. */
static uint64_t TubeTravelExpected(const char *name)
{
	const char *text = std::getenv(name); REQUIRE(text != nullptr);
	uint64_t value = 0; const auto parsed = std::from_chars(text, text + strlen(text), value);
	REQUIRE(parsed.ec == std::errc{}); REQUIRE(parsed.ptr == text + strlen(text)); return value;
}

/** @return Explicit plain0/joined1 fixture choice; older retained plain writer invocations default to0 without altering native geometry. */
static uint64_t TubeTravelLayout()
{
	const auto layout = std::getenv("OTTD_TUBE_TRAVEL_LAYOUT") == nullptr ? 0 : TubeTravelExpected("OTTD_TUBE_TRAVEL_LAYOUT");
	REQUIRE(layout <= 1); return layout;
}

/** @param path New repository-owned ordinary current file. Save all original owners without earning another native opportunity. */
static void SaveTubeTravelWorld(const std::filesystem::path &path)
{
	AutoRestoreBackup menu(_game_mode, GameMode::Menu);
	AutoRestoreBackup pause(_pause_mode, PauseModes{PauseMode::Normal});
	AutoRestoreBackup compression(_savegame_format, std::string{"lzma"});
	REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(path), false) == SaveLoadResult::Ok);
}

/** @param capsule Actual original physical part. @return Quantity/reference/packet, physical movement and RNG owners, independent of display caches. */
static auto TubeTravelWorldOwners(const Train &capsule)
{
	return std::tuple{ComputeUtilityPowerDigest(), ComputeTubePowerDigest(), ComputeTubePowerVehicleDigest(),
		ComputeTubeGeothermalPowerDigest(), ComputeGeothermalGeneratorDigest(), CargoPacket::GetNumItems(),
		Company::Get(_current_company)->money, TimerGameTick::counter, capsule.tile, capsule.x_pos, capsule.y_pos, capsule.z_pos,
		capsule.direction, capsule.cur_speed, capsule.flags, capsule.vehstatus, capsule.cargo.StoredCount(),
		std::array{_random.state[0], _random.state[1], _interactive_random.state[0], _interactive_random.state[1]}};
}

/** @return Explicit contiguous four-turn paid route, including both actual native depot endpoints. */
static std::vector<TileIndex> TubeTravelRoute()
{
	std::vector<TileIndex> route{TileXYL(32, 24, MapLayer::DeepUnderground)};
	// Four real outbound turns earn every body facing. Reversing a train may
	// back the same hull, so return alone cannot establish missing directions.
	for (uint y = 25; y <= 31; ++y) route.push_back(TileXYL(32, y, MapLayer::DeepUnderground));
	for (uint x = 31; x >= 22; --x) route.push_back(TileXYL(x, 31, MapLayer::DeepUnderground));
	for (uint y = 30; y >= 20; --y) route.push_back(TileXYL(22, y, MapLayer::DeepUnderground));
	for (uint x = 23; x <= 29; ++x) route.push_back(TileXYL(x, 20, MapLayer::DeepUnderground));
	for (uint y = 21; y <= 25; ++y) route.push_back(TileXYL(29, y, MapLayer::DeepUnderground));
	return route;
}

/** @param a Actual route cell. @param b Its next consecutive same-layer cell. @return Native cardinal edge, refusing any disconnected fixture. */
static DiagDirection TubeTravelPort(TileIndex a, TileIndex b)
{
	for (const auto port : {DiagDirection::NE, DiagDirection::SE, DiagDirection::SW, DiagDirection::NW}) {
		if (TileAddByDiagDir(a, port) == b) return port;
	}
	FAIL("Native travel fixture must have consecutive cardinal cells"); return DiagDirection::Invalid;
}

/** Original paid owners construct each real model's independent route and native order before writing the full current world. */
TEST_CASE("Native tube travel writes a real paid four-turn model route", "[tube-travel-art-write]")
{
	const auto model = TubeTravelExpected("OTTD_TUBE_TRAVEL_MODEL"); REQUIRE(model >= 9); REQUIRE(model <= 13);
	const auto layout = TubeTravelLayout();
	MockEnvironment::Instance(); LoadGeneratedWindowTestLanguage(); ResetRailTypes(); GeothermalTubeSwitchRig rig{true};
	AutoRestoreBackup calendar(TimerGameCalendar::date, TimerGameCalendar::ConvertYMDToDate(TimerGameCalendar::Year{2005}, 0, 1));
	AutoRestoreBackup year(TimerGameCalendar::year, TimerGameCalendar::Year{2005});
	AutoRestoreBackup clearing(Company::Get(_current_company)->clear_limit, uint32_t{128} << 16);
	AutoRestoreBackup freight(_settings_game.vehicle.freight_trains, uint8_t{1});
	const auto coal = GetCargoTypeByLabel(CT_COAL); REQUIRE(coal != INVALID_CARGO);
	CargoSpec::Get(coal)->weight = 16; CargoSpec::Get(coal)->is_freight = true;
	LayerSaveContent content{rig.ActivatedContent(), true}; AutoRestoreBackup local(_local_company, _current_company);
	StationClass::Reset(); RebuildStationKdtree(); (*Town::Iterate().begin())->townnametype = SPECSTR_TOWNNAME_START;
	// The inherited compact destination blocks the longer actual route. Remove
	// only that unused native depot and paid branch; the original inlet stays.
	REQUIRE(Command<Commands::LandscapeClear>::Do(DoCommandFlag::Execute, rig.inlets[1]).Succeeded());
	for (uint y : {25U, 26U}) REQUIRE(Command<Commands::RemoveGeothermalUtilityLink>::Do(DoCommandFlag::Execute,
		TileXYL(32, y, MapLayer::DeepUnderground), UtilityEnergyDomain::Electricity).Succeeded());
	const auto route = TubeTravelRoute(); std::vector<TileIndex> floor;
	// Explicit surrounding floor keeps rock-wall foreground out of the body
	// inspection sites; native construction still pays for every track/depot.
	for (uint y = 19; y <= 32; ++y) for (uint x = 21; x <= 33; ++x) floor.push_back(TileXYL(x, y, MapLayer::DeepUnderground));
	rig.Open(floor);
	// Consecutive entry/exit mouths determine one real native track per cell.
	// An artwork heading cannot change rail topology or skip a corner tile.
	for (size_t index = 1; index + 1 < route.size(); ++index) {
		const auto tile = route[index];
		if (IsPlainRailTile(tile)) REQUIRE(Command<Commands::LandscapeClear>::Do(DoCommandFlag::Execute, tile).Succeeded());
		const auto entrance = TubeTravelPort(tile, route[index - 1]), exit = TubeTravelPort(tile, route[index + 1]);
		TrackBits tracks;
		// Both actual outward mouths must belong to this one native piece;
		// entering-direction convenience masks would reverse a curved corner.
		for (Track track : TRACK_BIT_ALL) {
			if (TrackExitdirToTrackdir(track, entrance) != Trackdir::Invalid && TrackExitdirToTrackdir(track, exit) != Trackdir::Invalid) tracks.Set(track); // Select exactly the native piece joining the two installed neighbours, never an art-only curve.
		}
		REQUIRE(std::popcount(tracks.base()) == 1);
		for (Track track : tracks) REQUIRE(CmdBuildSingleRail(DoCommandFlag::Execute, tile, GetTubeRailType(), track, false).Succeeded());
	}
	REQUIRE(CmdBuildTrainDepot(DoCommandFlag::Execute, route.back(), GetTubeRailType(), DiagDirection::NW).Succeeded());
	rig.inlets[1] = route.back();
	if (layout == 1) {
		// Build genuine independent X crossing arms and a real shared-edge
		// junction before any vehicle exists. Longer paid branches keep their
		// terminal mouths outside the bounded body/rim inspection neighbourhood.
		std::vector<TileIndex> extra_floor;
		for (uint y = 27; y <= 32; ++y) for (uint x = 34; x <= 35; ++x) extra_floor.push_back(TileXYL(x, y, MapLayer::DeepUnderground));
		rig.Open(extra_floor);
		for (uint x = 30; x <= 34; ++x) REQUIRE(CmdBuildSingleRail(DoCommandFlag::Execute, TileXYL(x, 28, MapLayer::DeepUnderground),
			GetTubeRailType(), Track::X, false).Succeeded());
		for (uint x = 32; x <= 34; ++x) REQUIRE(CmdBuildSingleRail(DoCommandFlag::Execute, TileXYL(x, 31, MapLayer::DeepUnderground),
			GetTubeRailType(), Track::X, false).Succeeded());
		CHECK(GetTrackBits(TileXYL(32, 28, MapLayer::DeepUnderground)).base() == 3);
		CHECK(GetTrackBits(TileXYL(32, 31, MapLayer::DeepUnderground)).base() == 5);
	}
	const auto engine = GetTubeEngine(static_cast<uint16_t>(model)); REQUIRE(engine != EngineID::Invalid()); Engine::Get(engine)->company_avail.Set(_current_company);
	const auto [cost, id, capacity, mail, capacities] = Command<Commands::BuildVehicle>::Unsafe<CommandCallback>(INVALID_STRING_ID, nullptr, false, false,
		rig.inlets[0], std::make_tuple(rig.inlets[0], engine, false, INVALID_CARGO, ClientID{1}));
	INFO("model=" << model << " purchase error=" << cost.GetErrorMessage().base()); REQUIRE(cost.Succeeded()); REQUIRE(cost.GetCost() > Money{0});
	Train *capsule = Train::Get(id); const auto packet = rig.Load(*capsule);
	REQUIRE(BindTubeGeothermalPowerVehicle(capsule->index, rig.geothermal.source).has_value()); rig.Start(*capsule, route.back());
	SetMouseCursor(SPR_CURSOR_MOUSE, PAL_NONE); const auto owners = TubeTravelWorldOwners(*capsule);
	const auto paths = GetTestCaseArtifactPaths("tubes", layout == 0 ? "travel-art" : "joins-art"); const auto output = paths.Output("saves", fmt::format("tube-travel-model-{}.sav", model));
	SaveTubeTravelWorld(output); CHECK(TubeTravelWorldOwners(*capsule) == owners);
	fmt::print("TUBE_TRAVEL_SAVE model={} layout={} vehicle={} packet={} cargo={} tick={} utility={} invoices={} binding={} geothermal={} generator={} path={}\n",
		model, layout, capsule->index.base(), packet.base(), capsule->cargo_type, TimerGameTick::counter, ComputeUtilityPowerDigest(), ComputeTubePowerDigest(),
		ComputeTubePowerVehicleDigest(), ComputeTubeGeothermalPowerDigest(), ComputeGeothermalGeneratorDigest(), output.string());
}

/** @param capsule Actual ordered front. Advance only normal synchronized preparation and a complete native physical tick. */
static void TickTubeTravel(Train &capsule)
{
	AutoRestoreBackup native_work(_pause_mode, PauseModes{});
	UpdateSignalsInBuffer(); ++TimerGameTick::counter; PrepareTubePowerTick();
	REQUIRE(capsule.Tick()); REQUIRE_FALSE(capsule.vehstatus.Test(VehState::Crashed)); REQUIRE(IsTrainOnItsTiles(&capsule));
}

/** @param capsule Actual exposed moving capsule. @param zoom Native viewport scale. Check real selection and wrong-layer refusal without changing the camera or body. */
static void PickTubeTravelCapsule(const Train &capsule, ZoomLevel zoom)
{
	Viewport view{}; view.width = 640; view.height = 360; view.zoom = zoom; view.layer = GetVehicleDisplayLayer(&capsule);
	view.virtual_left = (capsule.coord.left + capsule.coord.right) / 2 - ScaleByZoom(320, zoom);
	view.virtual_top = (capsule.coord.top + capsule.coord.bottom) / 2 - ScaleByZoom(180, zoom);
	const auto *picked = CheckClickOnVehicle(view, 320, 180); REQUIRE(picked != nullptr); CHECK(picked->First() == &capsule);
	view.layer = MapLayer::Surface; const auto *other = CheckClickOnVehicle(view, 320, 180);
	CHECK((other == nullptr || other->First() != &capsule)); // Identical projected coordinates cannot select this Deep capsule through another layer.
}

/**
 * Inspect a representative real midpoint rather than the first sprite rotation at a corner entrance.
 * @param capsule Actual moving native part on one plain track piece; no position is assigned here.
 * @param facing Current real native body facing after its existing flip policy.
 * @return Whether the part has reached the native piece's midpoint within a two-world-pixel Manhattan radius.
 * The doubled centres average actual native edge mouths:0/15 at boundaries and8 on the other axis.
 */
static bool IsTubeTravelInspectionPoint(const Train &capsule, Direction facing)
{
	const auto mask = capsule.track.base(); // The physical train owns its selected piece; a junction's complete native mask must not substitute for that choice.
	Point centre;
	switch (mask) {
		case 1: case 2:
			if (!IsDiagonalDirection(facing)) return false;
			centre = {16, 16}; break;
		case 4: centre = {8, 8}; break;
		case 8: centre = {23, 23}; break;
		case 16: centre = {23, 8}; break;
		case 32: centre = {8, 23}; break;
		default: return false;
	}
	// Native sprite rotation begins before a curve midpoint. A diagonal body
	// still entering/leaving that curve cannot substitute for its actual bend.
	if (mask > 2 && IsDiagonalDirection(facing)) return false;
	const int x = capsule.x_pos % TILE_SIZE, y = capsule.y_pos % TILE_SIZE;
	return abs(2 * x - centre.x) + abs(2 * y - centre.y) <= 4;
}

/**
 * Capture one earned physical heading without another source, service, packet or train opportunity.
 * @param capsule Actual visible moving front, retaining its real load and native direction.
 * @param model Original capability model, independently pinned by the writer.
 * @param paths This native process's explicit review outputs.
 * @param facing Actual native body facing after its existing flip policy.
 */
static void CaptureTubeTravelHeading(Train &capsule, uint64_t model, const TestArtifactPaths &paths, Direction facing)
{
	const auto view = to_underlying(facing); const auto before = paths.Output("saves", fmt::format("tube-travel-{}-before.sav", view));
	const auto owners = TubeTravelWorldOwners(capsule); SaveTubeTravelWorld(before);
	const auto sprite = GetTubeTrainSprite(capsule, facing, TimerGameTick::counter); REQUIRE(sprite.has_value());
	// Native body bounds determine the source anchor independently of image
	// matching. Whole repeated views retain casing, body and every background.
	const Point origin = RemapCoords(TileX(capsule.tile) * TILE_SIZE + TILE_SIZE / 2, TileY(capsule.tile) * TILE_SIZE + TILE_SIZE / 2, TilePixelHeight(capsule.tile));
	const Point position = RemapCoords(capsule.x_pos + capsule.bounds.origin.x + capsule.bounds.offset.x,
		capsule.y_pos + capsule.bounds.origin.y + capsule.bounds.offset.y, capsule.z_pos + capsule.bounds.origin.z + capsule.bounds.offset.z);
	// Only neighbouring actual guideway fronts can legitimately cover this
	// hull. Publish their original geometry/poses for independent exact-pixel
	// occlusion checks, rather than masking curve noses or demanding bellies
	// through a near rim. These bounded queries earn no source/work opportunity.
	for (int dy = -1; dy <= 1; ++dy) for (int dx = -1; dx <= 1; ++dx) {
		const auto tile = TileAddWrap(capsule.tile, dx, dy); const auto art = GetTubeGuidewayArt(tile);
		if (!art.has_value()) continue;
		REQUIRE(art->count == 1); // This real simple route has one continuous native piece, with no invented endpoint or shared-edge junction.
		const Point neighbour = RemapCoords(TileX(tile) * TILE_SIZE + TILE_SIZE / 2, TileY(tile) * TILE_SIZE + TILE_SIZE / 2, TilePixelHeight(tile));
		fmt::print("TUBE_TRAVEL_RIM view={} mask={} energized={} dx={} dy={} front={}\n", view, GetTrackBits(tile).base(), static_cast<uint>(art->energized),
			(neighbour.x - origin.x) / static_cast<int>(ZOOM_BASE), (neighbour.y - origin.y) / static_cast<int>(ZOOM_BASE), art->parts[0].front);
	}
	for (const auto zoom : {ZoomLevel::Normal, ZoomLevel::In2x}) {
		for (uint repeat = 0; repeat < 2; ++repeat) CaptureNativeArtViewport(capsule.tile, zoom,
			paths.images / fmt::format("tube-travel-{}-zoom-{}-{}.ppm", view, to_underlying(zoom), repeat));
		PickTubeTravelCapsule(capsule, zoom);
	}
	REQUIRE(capsule.sprite_cache.sprite_seq.count == 1); CHECK(capsule.sprite_cache.sprite_seq.seq[0].sprite == *sprite);
	const auto after = paths.Output("saves", fmt::format("tube-travel-{}-after.sav", view)); SaveTubeTravelWorld(after); CHECK(TubeTravelWorldOwners(capsule) == owners);
	fmt::print("TUBE_TRAVEL_VIEW model={} view={} direction={} frame={} sprite={} tile={} mask={} track={} speed={} tick={} dx={} dy={} picked=true wrong_layer_refused=true before={} after={}\n",
		model, view, to_underlying(capsule.direction), (TimerGameTick::counter / 8) % 2, *sprite, capsule.tile.base(), GetTrackBits(capsule.tile).base(), capsule.track.base(), capsule.cur_speed, TimerGameTick::counter,
		(position.x - origin.x) / static_cast<int>(ZOOM_BASE), (position.y - origin.y) / static_cast<int>(ZOOM_BASE), before.string(), after.string());
}

static std::function<void()> *_tube_travel_observer = nullptr; ///< Isolated ordinary-startup observer; never an active simulation owner.

/** @param arguments Exact zero-argument startup command. @return Whether the registered ordinary loaded-world observer ran. */
static bool ObserveTubeTravelArt(std::span<std::string_view> arguments)
{
	REQUIRE(arguments.size() == 1); REQUIRE(_tube_travel_observer != nullptr); (*_tube_travel_observer)(); return true;
}

/** Actual ordered paid travel earns all body facings; each raster/picking boundary independently preserves complete native owners. */
TEST_CASE("Native tube travel earns eight real body facings and native layer picking", "[.tube-travel-art-runtime]")
{
	const char *input = std::getenv("OTTD_TUBE_TRAVEL_INPUT"); REQUIRE(input != nullptr);
	const auto model = TubeTravelExpected("OTTD_TUBE_TRAVEL_MODEL"); REQUIRE(model >= 9); REQUIRE(model <= 13);
	const auto layout = TubeTravelLayout();
	const auto source = ResolveTestInputPath(std::filesystem::path{input}); const auto config = GetTestRuntimeConfig(); const auto paths = GetTestArtifactPaths("tubes", layout == 0 ? "travel-art" : "joins-art");
	bool observed = false; std::exception_ptr failure;
	/** Drive only declared real native work between independently pure heading captures. */
	std::function<void()> capture = [&] {
		try {
			AutoRestoreBackup actor(_current_company, _local_company);
			if (layout == 1) {
				CHECK(GetTrackBits(TileXYL(32, 28, MapLayer::DeepUnderground)).base() == 3);
				CHECK(GetTrackBits(TileXYL(32, 31, MapLayer::DeepUnderground)).base() == 5);
			}
			const auto vehicle = TubeTravelExpected("OTTD_TUBE_TRAVEL_VEHICLE"); REQUIRE(vehicle < UINT32_MAX);
			Train *capsule = Train::GetIfValid(VehicleID{static_cast<uint32_t>(vehicle)}); REQUIRE(capsule != nullptr);
			const auto capability = GetTubeVehicleCapability(capsule->engine_type); REQUIRE(capability.has_value()); CHECK(capability->catalogue->model_id == model);
			CHECK(capsule->cargo_type == TubeTravelExpected("OTTD_TUBE_TRAVEL_CARGO"));
			CHECK(TimerGameTick::counter == TubeTravelExpected("OTTD_TUBE_TRAVEL_TICK"));
			CHECK(ComputeUtilityPowerDigest() == TubeTravelExpected("OTTD_TUBE_TRAVEL_UTILITY")); CHECK(ComputeTubePowerDigest() == TubeTravelExpected("OTTD_TUBE_TRAVEL_INVOICES"));
			CHECK(ComputeTubePowerVehicleDigest() == TubeTravelExpected("OTTD_TUBE_TRAVEL_BINDING")); CHECK(ComputeTubeGeothermalPowerDigest() == TubeTravelExpected("OTTD_TUBE_TRAVEL_GEOTHERMAL"));
			CHECK(ComputeGeothermalGeneratorDigest() == TubeTravelExpected("OTTD_TUBE_TRAVEL_GENERATOR"));
			REQUIRE(capsule->cargo.Packets()->size() == 1); const auto packet = capsule->cargo.Packets()->front()->index;
			CHECK(packet.base() == TubeTravelExpected("OTTD_TUBE_TRAVEL_PACKET")); CHECK(capsule->cargo.StoredCount() == 17);
			const auto *physical = GetTubeGeothermalSource(GetTubePowerVehicle(capsule->index)->geothermal_source); REQUIRE(physical != nullptr);
			const auto producer = physical->producer; const auto used_before = GetUtilityPowerProducer(producer)->consumed_lifetime_j;
			REQUIRE(BlitterFactory::SelectBlitter("8bpp-simple") != nullptr); GfxClearSpriteCache(); FontCache::ClearFontCaches(FONTSIZES_REQUIRED);
			uint8_t seen = 0; uint steps = 0; const auto destination = TubeTravelRoute().back(); const auto start = TimerGameTick::counter;
			// This bounded real route supplies every heading before entering its
			// destination. No position/direction or private work field is assigned.
			while (!(capsule->IsStoppedInDepot() && capsule->tile == destination) && steps < 6000) {
				TickTubeTravel(*capsule); ++steps;
				if (!IsPlainRailTile(capsule->tile) || capsule->vehstatus.Test(VehState::Hidden) || capsule->cur_speed == 0) continue; // Only actual exposed motion may establish a physical body view.
				const Direction facing = capsule->flags.Test(VehicleRailFlag::Flipped) ? ReverseDir(capsule->direction) : capsule->direction;
				const auto bit = uint8_t{1} << to_underlying(facing);
				if ((seen & bit) != 0) continue;
				if (!IsTubeTravelInspectionPoint(*capsule, facing)) continue;
				// Joined acceptance must actually traverse the crossing rather than
				// substitute a nearby simple straight with the same body direction.
				if (layout == 1 && facing == Direction::SE && capsule->tile != TileXYL(32, 28, MapLayer::DeepUnderground)) continue;
				// The first departing body still intersects its depot roof. Wait
				// for actual movement to clear that neighbourhood, never move art.
				if (DistanceManhattan(capsule->tile, TubeTravelRoute().front()) < 3) continue;
				REQUIRE(GetTubePowerVehicle(capsule->index)->wait == TubePowerWait::None);
				CaptureTubeTravelHeading(*capsule, model, paths, facing); seen |= bit;
			}
			REQUIRE(steps < 6000); REQUIRE(seen == 255); REQUIRE(capsule->IsStoppedInDepot());
			// Normal order replacement and Start earn a complete return while the
			// original seventeen packets stay on their native part.
			REQUIRE(CmdDeleteOrder(DoCommandFlag::Execute, capsule->index, 0).Succeeded());
			Order order; order.MakeGoToDepot(Depot::GetByTile(TubeTravelRoute().front())->index, {OrderDepotTypeFlag::PartOfOrders}, OrderNonStopFlag::NonStop, {OrderDepotActionFlag::Halt});
			REQUIRE(CmdInsertOrder(DoCommandFlag::Execute, capsule->index, 0, order).Succeeded()); ProcessOrders(capsule);
			REQUIRE(Command<Commands::StartStopVehicle>::Do(DoCommandFlag::Execute, capsule->index, true).Succeeded());
			while (!(capsule->IsStoppedInDepot() && capsule->tile == TubeTravelRoute().front()) && steps < 12000) { TickTubeTravel(*capsule); ++steps; }
			REQUIRE(steps < 12000); REQUIRE(capsule->IsStoppedInDepot()); CHECK(capsule->cargo.StoredCount() == 17);
			REQUIRE(capsule->cargo.Packets()->size() == 1); CHECK(capsule->cargo.Packets()->front()->index == packet);
			const auto final = paths.Output("saves", "tube-travel-return.sav"); SaveTubeTravelWorld(final);
			const auto used = GetUtilityPowerProducer(producer)->consumed_lifetime_j - used_before; REQUIRE(used != 0);
			fmt::print("TUBE_TRAVEL_DONE model={} views={} native_ticks={} start={} end={} used_j={} returned=true packet_retained=true final={}\n", model, seen, steps, start, TimerGameTick::counter, used, final.string());
			REQUIRE(BlitterFactory::SelectBlitter("null") != nullptr); GfxClearSpriteCache(); FontCache::ClearFontCaches(FONTSIZES_REQUIRED);
		} catch (...) { failure = std::current_exception(); }
		observed = true; _exit_game = true;
	};
	AutoRestoreBackup observer(_tube_travel_observer, &capture); IConsole::CmdRegister("tube_travel_art", ObserveTubeTravelArt);
	const std::string config_path = config.string(), source_path = source.string();
	std::array<std::string_view, 17> arguments = {"openttd_test.exe", "-v", "null:ticks=2", "-b", "8bpp-simple", "-I", "original_dos",
		"-s", "null", "-m", "null", "-x", "-X", "-c", config_path, "-g", source_path};
	CHECK(openttd_main(arguments) == 0); if (failure) std::rethrow_exception(failure); CHECK(observed);
}
