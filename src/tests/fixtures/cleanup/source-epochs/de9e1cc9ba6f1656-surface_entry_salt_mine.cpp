/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file surface_entry_salt_mine.cpp Genuine native mine/empty-carrier capital quotes and matching bank credit.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15BG.1.
 * Original commands own all money/debt. Queries grant no mine, floor or cargo.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../console_internal.h"
#include "../company_base.h"
#include "../company_func.h"
#include "../command_func.h"
#include "../misc_cmd.h"
#include "../industry_cmd.h"
#include "../industry.h"
#include "../multilayer_industry.h"
#include "../multilayer_content.h"
#include "../multilayer_recipe.h"
#include "../underground_vehicle.h"
#include "../vehicle_cmd.h"
#include "../vehicle_base.h"
#include "../engine_base.h"
#include "../engine_func.h"
#include "../cargotype.h"
#include "../cargo_type.h"
#include "../station_base.h"
#include "../station_map.h"
#include "../station_cmd.h"
#include "../rail_map.h"
#include "../layer_portal_cmd.h"
#include "../depot_base.h"
#include "../tile_map.h"
#include "../openttd.h"
#include "../core/backup_type.hpp"
#include "../saveload/saveload_func.h"
#include "layer_save_fixture.h"
#include "test_artifact_paths.h"
#include "../safeguards.h"

static std::function<void()> *surface_salt_mine_observer = nullptr; ///< Current native callback, never a bank or stock owner.

/** Execute the current genuine game's native mine-capital observer.
 * @param args Parameter-free original native console arguments.
 * @return True after the invocation-owned callback ran.
 */
static bool ObserveSurfaceSaltMine(std::span<std::string_view> args)
{
	REQUIRE(args.size() == 1); REQUIRE(surface_salt_mine_observer != nullptr); (*surface_salt_mine_observer)(); return true;
}

/** Serialize the actual stopped world without adding work or inventory.
 * @param path Fresh invocation-owned native output, never its source.
 */
static void SaveSurfaceSaltMine(const std::filesystem::path &path)
{
	AutoRestoreBackup pause(_pause_mode, PauseModes{PauseMode::Normal});
	AutoRestoreBackup compression(_savegame_format, std::string{"lzma"});
	REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(path), false) == SaveLoadResult::Ok);
}

/** Estimate or execute one original synchronized command with normal settlement.
 * @tparam Tcmd Native command identity.
 * @tparam Targs Complete original native argument types.
 * @param estimate True invokes only the command kernel's pure estimate path.
 * @param tile Actual action tile, or zero for coordinate-free bank credit.
 * @param args All native arguments, including a tile when required.
 * @return Original command result and any actual invoice.
 */
template <Commands Tcmd, typename... Targs>
static auto SurfaceSaltMineCommand(bool estimate, TileIndex tile, Targs... args)
{
	auto result = Command<Tcmd>::template Unsafe<CommandCallback>(INVALID_STRING_ID, nullptr, false, estimate, tile, std::make_tuple(args...));
	INFO("Salt mine command=" << GetCommandName(Tcmd) << " error=" << ExtractCommandCost(result).GetErrorMessage().base());
	REQUIRE(ExtractCommandCost(result).Succeeded());
	if (!estimate) fmt::print("SURFACE_SALT_MINE_INVOICE command={} cost={}\n", GetCommandName(Tcmd), ExtractCommandCost(result).GetCost().base());
	return result;
}

/** Quote the actual admitted mine, empty carrier and existing native route/service.
 * @return Whole real capital needed, excluding the separately stated operating buffer.
 */
static Money QuoteSurfaceSaltMineCapital()
{
	const auto *company = Company::Get(_current_company); const auto cash = company->money; const auto expenses = company->yearly_expenses;
	const auto vehicles = Vehicle::GetNumItems(); const auto industries = Industry::GetNumItems();
	const auto salt = GetCargoTypeByLabel(CargoLabel{"SALT"}); REQUIRE(IsValidCargoType(salt));
	const auto type = GetLayerMineIndustryType(CargoLabel{"SALT"}); REQUIRE(type != IT_INVALID);
	const auto site = ValidateMineDepositClaim({TileIndex{6236}, type, 0, GeologyDepositID{4}, _current_company});
	REQUIRE(site.error == LayerIndustryError::None);
	// The whole site owner quotes exact capital even before sufficient cash.
	// The funding command also enforces actual affordability, so its refusal
	// must remain intact until matching native credit has really arrived.
	REQUIRE(site.cost > cash); CHECK(CmdBuildLayerMine({}, TileIndex{6236}, type, 0, GeologyDepositID{4}, 123).Failed());
	Money capital = site.cost;
	REQUIRE(Station::Get(StationID{5})->owner == _current_company); CHECK(GetTileLayer(Station::Get(StationID{5})->xy) == MapLayer::Underground);
	REQUIRE(Depot::GetByTile(TileIndex{1373})->xy == TileIndex{1373}); CHECK(GetTileOwner(TileIndex{1373}) == _current_company);
	CHECK(GetLayerPortalWorkState(TileIndex{1949}) == LayerPortalWorkState::Connected);
	// Refit estimates may create/sell temporary native test hulls. The original
	// command kernel owns nesting, preventing those sales becoming cash refunds.
	// One60unit wagon plus its locomotive fits the actual one-tile pickup.
	// Extra wagons would overhang the indispensable plain portal door approach.
	for (uint16_t model : {4, 5}) {
		const auto engine = GetUndergroundEngine(model); REQUIRE(engine != EngineID::Invalid());
		CHECK(IsEngineBuildable(engine, VehicleType::Train, _current_company));
		const auto cargo = model == 4 ? INVALID_CARGO : salt;
		CHECK(CheckUndergroundVehicleCapability(engine, MapLayer::Surface, cargo).Succeeded());
		CHECK(CheckUndergroundVehicleCapability(engine, MapLayer::Underground, cargo).Succeeded());
		const auto quote = SurfaceSaltMineCommand<Commands::BuildVehicle>(true, TileIndex{1373}, TileIndex{1373}, engine, false, cargo, ClientID::Server);
		capital += std::get<0>(quote).GetCost();
		fmt::print("SURFACE_SALT_MINE_CARRIER_QUOTE model={} engine={} cost={} capacity={}\n", model, engine.base(), std::get<0>(quote).GetCost().base(), std::get<2>(quote));
	}
	// The content owner supplies the finite extraction rate; this query must
	// never advance it, recreate a binding or consume the remaining seam.
	const auto recipes = GetMultiLayerMineRecipes();
	const auto recipe = std::ranges::find(recipes, CargoLabel{"SALT"}, &MultiLayerMineRecipe::resource); REQUIRE(recipe != recipes.end());
	CHECK(company->money == cash); CHECK(company->yearly_expenses == expenses); CHECK(Vehicle::GetNumItems() == vehicles); CHECK(Industry::GetNumItems() == industries);
	fmt::print("SURFACE_SALT_MINE_CAPITAL mine={} total={} salt_quantity={} work={} output_capacity={} ceiling={}\n", site.cost.base(), capital.base(),
		recipe->recipe.outputs.front().quantity, recipe->recipe.work_economy_ticks, recipe->output_capacity, company->GetMaxLoan().base());
	return capital;
}

/** Acquire only actual native credit needed for the freshly quoted capital and working buffer.
 * @param capital Actual mine plus one locomotive/one empty mineral wagon quote.
 */
static void BorrowSurfaceSaltMineCapital(Money capital)
{
	const auto *company = Company::Get(_current_company); REQUIRE(company->current_loan == 3500000);
	const auto cash = company->money, debt = company->current_loan; const auto expenses = company->yearly_expenses;
	const Money buffer{100000}; const auto shortfall = capital + buffer - cash; REQUIRE(shortfall > 0);
	const Money amount{((shortfall.base() + 9999) / 10000) * 10000};
	REQUIRE(debt + amount <= company->GetMaxLoan()); CHECK(cash + amount >= capital + buffer); CHECK(cash + amount - 10000 < capital + buffer);
	CHECK(SurfaceSaltMineCommand<Commands::IncreaseLoan>(true, TileIndex{0}, LoanCommand::Amount, amount).GetCost() == 0);
	CHECK(company->money == cash); CHECK(company->current_loan == debt);
	SurfaceSaltMineCommand<Commands::IncreaseLoan>(false, TileIndex{0}, LoanCommand::Amount, amount);
	CHECK(company->money == cash + amount); CHECK(company->current_loan == debt + amount); CHECK(company->yearly_expenses == expenses);
	fmt::print("SURFACE_SALT_MINE_CREDIT amount={} buffer={} cash={} debt={}\n", amount.base(), buffer.base(), company->money.base(), company->current_loan.base());
}

/** Restore the required plain portal door and connect its genuine paid approaches.
 * Only the near platform cell is removed, retaining native station5on its far
 * cell and ordinary paid rail beneath the former platform. No floor is added.
 */
static void RepairSurfaceSaltMineApproach()
{
	const TileIndex near{6109}, far{6173}, portal{1949};
	REQUIRE(GetStationIndex(near) == StationID{5}); REQUIRE(GetStationIndex(far) == StationID{5});
	CHECK(GetLayerPortalWorkState(portal) == LayerPortalWorkState::Sealed);
	CHECK(CmdChangeLayerPortalConnection({}, portal, true).Failed());
	SurfaceSaltMineCommand<Commands::RemoveFromRailStation>(true, near, near, near, true);
	SurfaceSaltMineCommand<Commands::RemoveFromRailStation>(false, near, near, near, true);
	CHECK(IsPlainRailTile(near)); CHECK(GetTileOwner(near) == _current_company); CHECK(GetStationIndex(far) == StationID{5});
	SurfaceSaltMineCommand<Commands::ChangeLayerPortalConnection>(true, portal, portal, true);
	SurfaceSaltMineCommand<Commands::ChangeLayerPortalConnection>(false, portal, portal, true);
	CHECK(GetLayerPortalWorkState(portal) == LayerPortalWorkState::Connected);
	fmt::print("SURFACE_SALT_MINE_APPROACH_REPAIRED plain={} station=5 station_tile={} platform_length=1 connected=true\n", near.base(), far.base());
}

/** Real access work and bank credit retain the authentic game's original material/capital owners. */
TEST_CASE("Authentic Salt mine access and funding use ordinary native commands", "[.surface-entry-salt-mine]")
{
	const char *source = std::getenv("OTTD_SURFACE_ENTRY_SOURCE"); REQUIRE(source != nullptr);
	const char *mode = std::getenv("OTTD_SURFACE_SALT_MINE_PHASE"); const std::string_view phase = mode == nullptr ? "credit" : mode;
	// A route repair is an explicit paid native step. It must never be hidden
	// inside a loan observer whose only permitted changes are cash and debt.
	REQUIRE((phase == "credit" || phase == "repair"));
	const auto paths = GetTestCaseArtifactPaths("space", "surface-entry-salt-mine"); const auto config = GetTestRuntimeConfig();
	bool observed = false; std::exception_ptr failure;
	/** Preserve original native failures through ordinary shutdown without supplying compensating gameplay state. */
	std::function<void()> inspect = [&] {
		try {
			AutoRestoreBackup actor(_current_company, _local_company); REQUIRE(Company::IsValidID(_current_company));
			SaveSurfaceSaltMine(paths.saves / fmt::format("surface-salt-mine-before-{}-440.sav", phase));
			if (phase == "repair") RepairSurfaceSaltMineApproach();
			else {
				const auto capital = QuoteSurfaceSaltMineCapital(); SaveSurfaceSaltMine(paths.saves / "surface-salt-mine-query-440.sav");
				BorrowSurfaceSaltMineCapital(capital);
				const auto type = GetLayerMineIndustryType(CargoLabel{"SALT"});
				const auto funded_quote = CmdBuildLayerMine({}, TileIndex{6236}, type, 0, GeologyDepositID{4}, 123);
				REQUIRE(funded_quote.Succeeded()); CHECK(funded_quote.GetCost() == 616584);
			}
			SaveSurfaceSaltMine(paths.saves / fmt::format("surface-salt-mine-{}-440.sav", phase));
			fmt::print("SURFACE_SALT_MINE_COMPLETE phase={} supplied_stock=false\n", phase); observed = true;
		} catch (...) { failure = std::current_exception(); }
		_pause_mode.Set(PauseMode::Normal); _exit_game = true;
	};
	AutoRestoreBackup observer(surface_salt_mine_observer, &inspect); IConsole::CmdRegister("surface_entry_salt_mine", ObserveSurfaceSaltMine);
	const std::string config_arg = config.string();
	std::array<std::string_view, 17> args = {"openttd_surface_supply_test.exe", "-v", "null:ticks=2", "-b", "null", "-s", "null", "-m", "null", "-x", "-X", "-I", "original_dos", "-c", config_arg, "-g", source};
	CHECK(openttd_main(args) == 0); if (failure) std::rethrow_exception(failure); CHECK(observed);
}
