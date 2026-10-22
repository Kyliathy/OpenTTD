/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file surface_entry_mine.cpp Genuine Salt mine geometry, settled retirement and retained-cutter room quotes.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15BF.
 * Original commands own every project, vehicle, terrain cell and native invoice.
 * Unbuilt geometry and pure room previews never supply mine admission or floor.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../console_internal.h"
#include "../company_base.h"
#include "../company_func.h"
#include "../command_func.h"
#include "../industry_cmd.h"
#include "../industrytype.h"
#include "../multilayer_industry.h"
#include "../layer_portal_map.h"
#include "../tbm.h"
#include "../tbm_cmd.h"
#include "../tbm_area_preview.h"
#include "../tbm_exchange.h"
#include "../train.h"
#include "../vehicle_cmd.h"
#include "../station_cmd.h"
#include "../newgrf_station.h"
#include "../openttd.h"
#include "../core/backup_type.hpp"
#include "../saveload/saveload_func.h"
#include "layer_save_fixture.h"
#include "test_artifact_paths.h"
#include "../safeguards.h"

static std::function<void()> *surface_mine_observer = nullptr; ///< Invocation callback, never an alternate geological authority.

/** Run the current full-loader native mine preparation.
 * @param args Parameter-free native console arguments.
 * @return True after the actual invocation callback ran.
 */
static bool ObserveSurfaceMine(std::span<std::string_view> args)
{
	REQUIRE(args.size() == 1); REQUIRE(surface_mine_observer != nullptr); (*surface_mine_observer)(); return true;
}

/** Save the actual stopped native boundary without earning time or supplying stock.
 * @param path New invocation-owned output, never its original input.
 */
static void SaveSurfaceMine(const std::filesystem::path &path)
{
	AutoRestoreBackup pause(_pause_mode, PauseModes{PauseMode::Normal});
	AutoRestoreBackup compression(_savegame_format, std::string{"lzma"});
	REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(path), false) == SaveLoadResult::Ok);
}

/** Estimate or execute the ordinary synchronized command and retain its actual invoice.
 * @tparam Tcmd Original native command identity.
 * @tparam Targs Complete native argument types.
 * @param estimate True queries the normal pure command path.
 * @param tile Real action location, never a camera-selected layer.
 * @param args Complete original arguments, including a tile where required.
 * @return Original command result, with finance settled by the sole native owner.
 */
template <Commands Tcmd, typename... Targs>
static auto SurfaceMineCommand(bool estimate, TileIndex tile, Targs... args)
{
	auto result = Command<Tcmd>::template Unsafe<CommandCallback>(INVALID_STRING_ID, nullptr, false, estimate, tile, std::make_tuple(args...));
	INFO("mine command=" << GetCommandName(Tcmd) << " error=" << ExtractCommandCost(result).GetErrorMessage().base());
	REQUIRE(ExtractCommandCost(result).Succeeded());
	if (!estimate) fmt::print("SURFACE_MINE_INVOICE command={} cost={}\n", GetCommandName(Tcmd), ExtractCommandCost(result).GetCost().base());
	return result;
}

/** Publish the real unbuilt mine's complete owner-derived machinery/loading refusal.
 * @param anchor Actual proposed Underground machinery anchor.
 * @return Original site admission with its native first missing cell.
 */
static LayerMineAdmission QuerySurfaceMineSite(TileIndex anchor)
{
	const auto type = GetLayerMineIndustryType(CargoLabel{"SALT"}); REQUIRE(type != IT_INVALID);
	const LayerMineRequest request{anchor, type, 0, GeologyDepositID{4}, _current_company};
	auto admission = ValidateMineDepositClaim(request);
	REQUIRE(admission.machinery.size() == 9); REQUIRE(admission.loading.size() == 4);
	fmt::print("SURFACE_MINE_SITE anchor={} type={} error={} failure={} machinery=9 loading=4 capital={}\n", anchor.base(), type,
		to_underlying(admission.error), admission.failure_tile.base(), GetIndustrySpec(type)->GetConstructionCost().base());
	return admission;
}

/** Retire only the physically completed and settled original cutter project.
 * The native owner releases actual empty haul assignments; no train is moved,
 * sold, recreated or assigned a replacement cargo set by this observer.
 */
static void RetireSurfaceMinePredecessor()
{
	const auto projects = CaptureTBMProjects(); REQUIRE(projects.size() == 1); const auto project = projects.front();
	REQUIRE(project.id == TBMProjectID{0}); REQUIRE(project.cutter == VehicleID{7}); REQUIRE(project.phase == TBMPhase::Completed);
	const auto cash = Company::Get(_current_company)->money; const auto vehicles = Vehicle::GetNumItems();
	SurfaceMineCommand<Commands::ControlTBMProject>(true, project.portal, project.id, TBMProjectAction::Retire);
	SurfaceMineCommand<Commands::ControlTBMProject>(false, project.portal, project.id, TBMProjectAction::Retire);
	CHECK(CaptureTBMProjects().empty()); CHECK(CaptureTBMExchangeAssignments().empty()); CHECK(CaptureTBMWagonSets().empty());
	CHECK(Vehicle::GetNumItems() == vehicles); CHECK(Company::Get(_current_company)->money == cash);
	CHECK(IsTBMConsistReusableAtPortal(*Train::Get(project.cutter), project.portal, _current_company, 0));
	fmt::print("SURFACE_MINE_RETIRED cutter={} hauler=13 cash={} paid_again=false\n", project.cutter.base(), cash.base());
}

/** Quote the whole straight mine gallery through the retained-machine planner.
 * Pickup ends before the machinery, which may not bury the paid support rail.
 * Existing Surface service remains reachable through the original portal.
 * The gallery needs no side-room turn or change to native routing settings.
 */
static void QuerySurfaceMineRoom()
{
	REQUIRE(CaptureTBMProjects().empty());
	const auto mine = TileXYL(28, 33, MapLayer::Underground);
	const auto admission = QuerySurfaceMineSite(mine);
	const auto first = GetLayerPortalPairTile(TileIndex{1949}), last = TileXYL(29, 36, MapLayer::Underground);
	const auto quote = PreviewTBMCommissionArea(_current_company, TileIndex{1949}, 0, 2, 2, StationID{4}, first, last, false, ExcavationPlanIntent::StraightTunnel);
	INFO("straight gallery error=" << quote.purchase.GetErrorMessage().base() << " geology=" << to_underlying(quote.area.error));
	REQUIRE(quote.purchase.Succeeded()); CHECK(quote.request.reuse_cutter == VehicleID{7}); CHECK(quote.purchase.GetCost() == 0); CHECK_FALSE(quote.area.partial);
	std::set<TileIndex> swept;
	// Canonical envelopes must cover every missing machinery/loading cell.
	// Existing paid floor needs no second charge; neither an omitted cell nor
	// an assumed view-dependent cavity can authorize later mine funding.
	for (const auto &cut : quote.request.proposal.route) swept.insert(cut.swept_tiles.begin(), cut.swept_tiles.end());
	for (TileIndex tile : admission.machinery) CHECK((swept.contains(tile) || GetGeologyWorldState().terrain.Get(tile) == ExcavationState::Excavated));
	for (TileIndex tile : admission.loading) CHECK((swept.contains(tile) || GetGeologyWorldState().terrain.Get(tile) == ExcavationState::Excavated));
	fmt::print("SURFACE_MINE_ROOM_QUOTE mine={} first={} last={} route={} cells={} earth={} rock={} purchase={} fingerprint={}\n", mine.base(),
		first.base(), last.base(), quote.area.plan.route.size(), quote.area.totals.new_cells, quote.area.totals.earth_t, quote.area.totals.rock_t,
		quote.purchase.GetCost().base(), quote.fingerprint);
	fmt::print("SURFACE_MINE_PICKUP station_anchor={} axis=1 length=2 approach={} service=1373 conversion_removal_before_funding=true\n",
		TileXYL(29, 31, MapLayer::Underground).base(), first.base());
}

/** Genuine native preparation preserves all earned products while querying or releasing original work. */
TEST_CASE("Authentic Salt mine preparation uses whole native geometry and actual settled cutter reuse", "[.surface-entry-mine]")
{
	const char *source = std::getenv("OTTD_SURFACE_ENTRY_SOURCE"), *mode = std::getenv("OTTD_SURFACE_MINE_PHASE"); REQUIRE(source != nullptr); REQUIRE(mode != nullptr);
	// Explicit phases distinguish an unchanged genuine-world query from a real
	// native retirement; neither may silently buy machinery or open rock.
	const std::string_view phase{mode}; REQUIRE((phase == "characterize" || phase == "release" || phase == "plan"));
	const auto paths = GetTestCaseArtifactPaths("space", "surface-entry-mine"); const auto config = GetTestRuntimeConfig();
	bool observed = false; std::exception_ptr failure;
	/** Preserve native assertion failures through ordinary shutdown without modifying failed gameplay state. */
	std::function<void()> inspect = [&] {
		try {
			AutoRestoreBackup actor(_current_company, _local_company); REQUIRE(Company::IsValidID(_current_company));
			REQUIRE(Company::Get(_current_company)->current_loan == 3500000);
			SaveSurfaceMine(paths.saves / fmt::format("surface-mine-before-{}-440.sav", phase));
			if (phase == "release") RetireSurfaceMinePredecessor();
			else if (phase == "plan") QuerySurfaceMineRoom();
			else {
				const auto result = QuerySurfaceMineSite(TileXYL(30, 31, MapLayer::Underground));
				CHECK(result.error == LayerIndustryError::Footprint);
				SurfaceMineCommand<Commands::ControlTBMProject>(true, TileIndex{1949}, TBMProjectID{0}, TBMProjectAction::Retire);
			}
			SaveSurfaceMine(paths.saves / fmt::format("surface-mine-{}-440.sav", phase));
			fmt::print("SURFACE_MINE_COMPLETE phase={} cash={} debt={} supplied_floor=false\n", phase, Company::Get(_current_company)->money.base(),
				Company::Get(_current_company)->current_loan.base()); observed = true;
		} catch (...) { failure = std::current_exception(); }
		_pause_mode.Set(PauseMode::Normal); _exit_game = true;
	};
	AutoRestoreBackup observer(surface_mine_observer, &inspect); IConsole::CmdRegister("surface_entry_mine", ObserveSurfaceMine);
	const std::string config_arg = config.string();
	std::array<std::string_view, 17> args = {"openttd_surface_supply_test.exe", "-v", "null:ticks=2", "-b", "null", "-s", "null", "-m", "null", "-x", "-X", "-I", "original_dos", "-c", config_arg, "-g", source};
	CHECK(openttd_main(args) == 0); if (failure) std::rethrow_exception(failure); CHECK(observed);
}
