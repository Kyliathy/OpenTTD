/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file tube_shield_source.cpp Original1985 native-content source, paid sealed carrier and actual finite Deep RARE production.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Groups15EB/15EBB/15FBA shared empty native author.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
 * Prepared floor/completed machinery are explicit boundaries. No useful delivery, Electronics manufacture or TBM bootstrap is claimed.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "mock_environment.h"
#include "layer_window_fixture.h"
#include "layer_save_fixture.h"
#include "excavation_fixture.h"
#include "tube_useful_tick_fixture.h"
#include "tube_source_template_fixture.h"
#include "movement_fixture.h"
#include "test_artifact_paths.h"
#include "shield_source_world_fixture.h"
#include "../company_manager_face.h"
#include "../engine_func.h"
#include "../underground_vehicle.h"
#include "../tbm.h"
#include "../space_vehicle.h"
#include "../transport_tube.h"
#include "../industry.h"
#include "../industrytype.h"
#include "../industry_cmd.h"
#include "../industry_map.h"
#include "../layer_industry_processor.h"
#include "../multilayer_industry.h"
#include "../town.h"
#include "../train.h"
#include "../cargopacket.h"
#include "../road.h"
#include "../timer/timer_game_calendar.h"
#include "../timer/timer_game_economy.h"
#include "../timer/timer_game_tick.h"
#include "../newgrf_station.h"
#include "../rail_cmd.h"
#include "../rail_map.h"
#include "../vehicle_cmd.h"
#include "../train_cmd.h"
#include "../vehicle_func.h"
#include "../command_func.h"
#include "../economy_func.h"
#include "../settings_internal.h"
#include "../console_internal.h"
#include "../fios.h"
#include "../saveload/saveload_func.h"
#include "../gfx_func.h"
#include "../table/sprites.h"
#include "../safeguards.h"

extern TileIndex _cur_tileloop_tile; ///< Native nonzero landscape cursor established before empty-world transport.
/** Initialize the ordinary native economy before any scenario purchase; no private price table is installed. */
void StartupEconomy();
/** Advance every real industry through its native dispatcher once per complete shared opportunity. */
void OnTick_Industry();

/** @copydoc WriteTubeUsefulSourceTemplate */
std::filesystem::path WriteTubeUsefulSourceTemplate(int year, std::string_view scenario, std::string_view basename, int64_t initial_money)
{
	MockEnvironment::Instance(); LoadGeneratedWindowTestLanguage(); AutoRestoreBackup game_settings{_settings_game};
	// Establish real saved defaults before any participant exists. Reduced
	// unit-test calendar, pathfinder or vehicle limits cannot define this world.
	for (const auto &entry : GetSaveLoadSettingTable()) {
		const auto *setting = GetSettingDesc(entry);
		if (!setting->flags.Test(SettingFlag::NotInSave)) setting->ResetToDefault(&_settings_game);
	}
	ExcavationFixture rig(true);
	AutoRestoreBackup calendar_date{TimerGameCalendar::date}; AutoRestoreBackup calendar_year{TimerGameCalendar::year};
	AutoRestoreBackup calendar_month{TimerGameCalendar::month}; AutoRestoreBackup calendar_fraction{TimerGameCalendar::date_fract};
	AutoRestoreBackup economy_date{TimerGameEconomy::date}; AutoRestoreBackup economy_year{TimerGameEconomy::year};
	AutoRestoreBackup economy_month{TimerGameEconomy::month}; AutoRestoreBackup economy_fraction{TimerGameEconomy::date_fract};
	AutoRestoreBackup economy{_economy}; AutoRestoreBackup prices{_price}; AutoRestoreBackup tick{TimerGameTick::counter, uint64_t{0}};
	AutoRestoreBackup raw_industry{_settings_game.construction.raw_industry_construction, uint8_t{1}};
	AutoRestoreBackup tile_loop{_cur_tileloop_tile, TileIndex{1}}; AutoRestoreBackup local{_local_company, _current_company};
	AutoRestoreBackup mappings{_engine_mngr.mappings}; LayerSaveContent content(rig.ActivatedContent(), true);
	TimerGameCalendar::SetDate(TimerGameCalendar::ConvertYMDToDate(TimerGameCalendar::Year{year}, 0, 1), 0);
	TimerGameEconomy::SetDate(TimerGameEconomy::ConvertYMDToDate(TimerGameEconomy::Year{year}, 0, 1), 0);
	_economy.inflation_prices = _economy.inflation_payment = uint64_t{1} << 16;
	_settings_game.vehicle.max_trains = 500; _settings_game.vehicle.max_train_length = 7;
	_settings_game.vehicle.freight_trains = 1; _settings_game.vehicle.train_acceleration_model = AccelerationModel::Realistic;
	ResetFaces(); SetCompanyManagerFaceStyle(Company::Get(_current_company)->face, 0);
	Company::Get(_current_company)->money = initial_money; // Explicit initial author funds precede every purchase/work step; never a loaded top-up or profitability claim.
	REQUIRE(Town::CanAllocateItem()); auto *town = Town::Create(TileXYL(10, 10, MapLayer::Surface)); town->townnametype = SPECSTR_TOWNNAME_START; RebuildTownKdtree();
	ResetRailTypes(); ResetRoadTypes(); StationClass::Reset(); ResetIndustries(); _engine_mngr.ResetToDefaultMapping(); SetupEngines();
	// Normal catalogue/economy startup precedes native loading and every later
	// command. The empty author fixture's reduced registry carries no cargo
	// whose handling could rely on those temporary definition properties.
	REQUIRE(InitialiseTBMEngineDefinitions(rig.ActivatedContent())); REQUIRE(InitialiseUndergroundEngineDefinitions(rig.ActivatedContent()));
	REQUIRE(InitialiseSpaceEngineDefinition()); REQUIRE(InitialiseTubeRailType(rig.ActivatedContent()) != INVALID_RAILTYPE);
	REQUIRE(InitialiseTubeEngineDefinitions(rig.ActivatedContent())); SetYearEngineAgingStops(); StartupEngines(); StartupEconomy();
	REQUIRE(Vehicle::GetNumItems() == 0); REQUIRE(Industry::GetNumItems() == 0); REQUIRE(CargoPacket::GetNumItems() == 0);
	const auto paths = GetTestCaseArtifactPaths("tubes", scenario); const auto path = paths.Output("saves", basename);
	SetMouseCursor(SPR_CURSOR_MOUSE, PAL_NONE);
	AutoRestoreBackup mode{_game_mode, GameMode::Menu}; AutoRestoreBackup pause{_pause_mode, PauseModes{PauseMode::Normal}};
	AutoRestoreBackup compression{_savegame_format, std::string{"lzma"}};
	REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(path), false) == SaveLoadResult::Ok);
	_engine_pool.CleanPool();
	// Retire only the empty author's metadata town before its isolated
	// geology/company lifetime ends; later readers use their actual loaded town.
	delete town; RebuildTownKdtree();
	return path;
}

/** The empty authored1985 world supplies normal content loading, with no source machinery, stock, packet or transport accomplishment. */
TEST_CASE("Shield source starts with an empty original1985 native world", "[tube-shield-source-template]")
{
	const auto path = WriteTubeUsefulSourceTemplate(1985, "shield-source", "shield-empty-native.sav", 16000000);
	fmt::print("SHIELD_SEED year=1985 vehicles=0 industries=0 packets=0 useful_source=false path={}\n", path.string());
}

/** @param tiles Exact scenario-owned rock floor. Prepare only declared terrain through public restoration, preserving every original reserve and claim. */
void OpenShieldSourceFloor(std::span<const TileIndex> tiles)
{
	auto snapshot = CaptureGeologyState(); GeologyWorldState candidate;
	REQUIRE(PrepareGeologyRestoration(snapshot, candidate) == GeologyError::None);
	// Machinery/depot commands cannot excavate their missing cavity. Only
	// this disclosed scenario floor boundary prepares the real original cells.
	for (const auto tile : tiles) {
		if (GetTileLayer(tile) == MapLayer::Surface) continue;
		REQUIRE(candidate.terrain.Set(tile, ExcavationState::Excavated));
	}
	snapshot.packed.assign(candidate.terrain.Data().begin(), candidate.terrain.Data().end());
	REQUIRE(PrepareGeologyRestoration(snapshot, candidate) == GeologyError::None);
	StageLoadedGeologyState(std::move(candidate)); AcceptLoadedGeologyState();
}

/** @param industry Actual command-funded machinery. Complete only its real cells as an explicit initial boundary, without supplying stock or work. */
static void CompleteShieldSourceMachinery(Industry &industry)
{
	for (const auto tile : industry.location) if (industry.TileBelongsToIndustry(tile)) SetIndustryCompleted(tile);
}

/** Advance the identical complete source opportunity for the real stock writer and ordinary readers; industry work runs once after native clocks/vehicles. */
void RunShieldSourceOpportunity()
{
	RunUsefulTubeNativeOpportunity(); UpdateLandscapingLimits(); OnTick_Industry();
}

/** @param paths Invocation-owned actual source-world output. Fund original native owners and earn finite RARE while the real Deep sealed carrier stays stopped/empty. */
static void EarnShieldSourceStock(const TestArtifactPaths &paths)
{
	REQUIRE(TimerGameCalendar::year == TimerGameCalendar::Year{1985}); REQUIRE(TimerGameTick::counter == 0);
	REQUIRE(Vehicle::GetNumItems() == 0); REQUIRE(Industry::GetNumItems() == 0); REQUIRE(CargoPacket::GetNumItems() == 0);
	const auto cargo = GetCargoTypeByLabel(CargoLabel{"RARE"}); REQUIRE(cargo != INVALID_CARGO);
	const auto *capsule = Engine::Get(GetTubeEngine(12)); REQUIRE(capsule != nullptr);
	REQUIRE(capsule->company_avail.Test(_current_company)); REQUIRE(capsule->info.refit_mask.Test(cargo));
	const auto *wagon = Engine::Get(GetUndergroundEngine(6)); REQUIRE(wagon != nullptr);
	REQUIRE(wagon->company_avail.Test(_current_company)); REQUIRE(wagon->info.refit_mask.Test(cargo));
	REQUIRE(CheckUndergroundVehicleCapability(wagon->index, MapLayer::DeepUnderground, cargo).Succeeded());
	const auto type = GetLayerMineIndustryType(CargoLabel{"RARE"}), receiver_type = GetLayerProcessorIndustryType(MultiLayerRecipeID{"ELEC"});
	REQUIRE(type != IT_INVALID); REQUIRE(receiver_type != IT_INVALID);
	const auto deposits = GetGeologyWorldState().deposits.Records(); const auto selected = std::ranges::find(deposits, CargoLabel{"RARE"}, &GeologyDeposit::resource);
	REQUIRE(selected != deposits.end()); REQUIRE(selected->layer == MapLayer::DeepUnderground); REQUIRE_FALSE(selected->spans.empty());
	const auto deposit = selected->id; const auto initial_reserve = selected->remaining_reserve;
	const auto x = selected->spans.front().first_x, y = selected->spans.front().y; REQUIRE(x >= 5); REQUIRE(x < 57);
	const auto site = TileXYL(x, y, MapLayer::DeepUnderground); const LayerMineRequest request{site, type, 0, deposit, _current_company};
	const auto geometry = ValidateMineDepositClaim(request); REQUIRE_FALSE(geometry.machinery.empty()); REQUIRE_FALSE(geometry.loading.empty());
	std::vector<TileIndex> floor = geometry.machinery; floor.insert(floor.end(), geometry.loading.begin(), geometry.loading.end()); OpenShieldSourceFloor(floor);
	const auto admission = ValidateMineDepositClaim(request);
	INFO(fmt::format("native Shield RARE source error={} quote={} available={}", to_underlying(admission.error), admission.cost.base(), GetAvailableMoneyForCommand().base()));
	REQUIRE(admission.error == LayerIndustryError::None); REQUIRE(admission.cost <= GetAvailableMoneyForCommand());
	REQUIRE(Command<Commands::BuildLayerMine>::Do(DoCommandFlag::Execute, site, type, 0, deposit, 123).Succeeded());
	auto *mine = Industry::GetByTile(site); REQUIRE(mine != nullptr); CompleteShieldSourceMachinery(*mine);
	const auto receiver_site = TileXYL(44, 34, MapLayer::Surface);
	const auto receiver_cost = Command<Commands::BuildIndustry>::Do(DoCommandFlag::Execute, receiver_site, receiver_type, 0, true, 123);
	INFO(fmt::format("native Shield ELEC receiver error={}", receiver_cost.GetErrorMessage())); REQUIRE(receiver_cost.Succeeded());
	auto *receiver = Industry::GetByTile(receiver_site); REQUIRE(receiver != nullptr); CompleteShieldSourceMachinery(*receiver);
	const auto *recipe = GetLayerProcessorRecipe(*receiver); REQUIRE(recipe != nullptr); REQUIRE(recipe->recipe.id == MultiLayerRecipeID{"ELEC"});
	uint row = 0;
	// Use actual admitted loading geometry for the later real source approach;
	// stale fixed mine dimensions cannot place the carrier inside machinery.
	for (const auto tile : admission.loading) row = std::max(row, TilePlaneY(tile));
	REQUIRE(row < 62); const auto home = TileXYL(x - 4, row, MapLayer::DeepUnderground);
	OpenShieldSourceFloor(std::array{home, TileAddWrap(home, 1, 0)});
	REQUIRE(Command<Commands::BuildRailDepot>::Do(DoCommandFlag::Execute, home, RAILTYPE_RAIL, DiagDirection::SW).Succeeded());
	const auto [head_cost, head_id, head_capacity, head_mail, head_capacities] = Command<Commands::BuildVehicle>::Do(DoCommandFlag::Execute, home, GetUndergroundEngine(4), false, INVALID_CARGO, ClientID::Invalid);
	INFO(fmt::format("native Deep Shield feed front error={}", head_cost.GetErrorMessage())); REQUIRE(head_cost.Succeeded());
	const auto [hold_cost, hold_id, hold_capacity, hold_mail, hold_capacities] = Command<Commands::BuildVehicle>::Do(DoCommandFlag::Execute, home, wagon->index, false, cargo, ClientID::Invalid);
	INFO(fmt::format("native Deep sealed carrier error={}", hold_cost.GetErrorMessage())); REQUIRE(hold_cost.Succeeded());
	REQUIRE(Command<Commands::MoveRailVehicle>::Do(DoCommandFlag::Execute, hold_id, head_id, false).Succeeded());
	const auto *head = Train::Get(head_id), *hold = Train::Get(hold_id); REQUIRE(hold->First() == head); REQUIRE(hold->cargo_type == cargo); REQUIRE(hold->cargo_cap >= 20);
	REQUIRE(mine->produced[0].cargo == cargo); REQUIRE(mine->produced[0].waiting == 0); uint opportunities = 0;
	// Each complete native opportunity advances clocks/all actual vehicles
	// and then the one ordinary industry dispatcher. Only the finite mine
	// owner may debit reserve into output; no station packet or work is granted.
	while (mine->produced[0].waiting < 20 && opportunities < 2048) {
		RunShieldSourceOpportunity(); ++opportunities;
		REQUIRE(head->IsChainInDepot()); REQUIRE(head->vehstatus.Test(VehState::Stopped)); REQUIRE(IsTrainOnItsTiles(head)); REQUIRE(hold->cargo.StoredCount() == 0);
		REQUIRE(CargoPacket::GetNumItems() == 0);
		REQUIRE(GetGeologyWorldState().deposits.Get(deposit)->remaining_reserve + mine->produced[0].waiting == initial_reserve);
	}
	REQUIRE(opportunities < 2048); REQUIRE(mine->produced[0].waiting == 20);
	const auto *history = GetLayerMineState(mine->index); REQUIRE(history != nullptr); REQUIRE(history->extracted == 20); REQUIRE(history->dispatched == 0);
	REQUIRE(GetGeologyWorldState().deposits.Get(deposit)->remaining_reserve == initial_reserve - 20);
	// The real Electronics receiver has no transported inputs. Native work
	// cannot manufacture its absent RFMT, consume the retained source RARE or
	// claim any useful delivery/payment before the modal route exists.
	for (const auto &entry : receiver->accepted) REQUIRE(entry.waiting == 0);
	for (const auto &entry : receiver->produced) REQUIRE(entry.waiting == 0);
	const auto &company = *Company::Get(_current_company); REQUIRE(company.cur_economy.income == Money{0}); REQUIRE(company.num_valid_stat_ent == 0);
	REQUIRE(company.current_loan == Money{0}); REQUIRE(company.money > Money{0}); REQUIRE(TimerGameCalendar::year == TimerGameCalendar::Year{1985});
	const auto before = std::tuple{TimerGameTick::counter, company.money, ComputeLayerMineDigest(), ComputeLayerProcessorDigest(),
		std::array{_random.state[0], _random.state[1], _interactive_random.state[0], _interactive_random.state[1]}};
	const auto path = paths.Output("saves", "shield-source-stock.sav");
	{
		AutoRestoreBackup mode{_game_mode, GameMode::Menu}; AutoRestoreBackup pause{_pause_mode, PauseModes{PauseMode::Normal}};
		AutoRestoreBackup compression{_savegame_format, std::string{"lzma"}};
		REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(path), false) == SaveLoadResult::Ok);
	}
	CHECK((before == std::tuple{TimerGameTick::counter, company.money, ComputeLayerMineDigest(), ComputeLayerProcessorDigest(),
		std::array{_random.state[0], _random.state[1], _interactive_random.state[0], _interactive_random.state[1]}}));
	fmt::print("SHIELD_STOCK year=1985 tick={} opportunities={} units=20 reserve_before={} reserve={} extracted=20 dispatched=0 cash={} loan=0 income=0 parts={},{} industries={},{} deposit={} home={} cargo={} pickup_x={} pickup_y={} packets=0 path={}\n",
		TimerGameTick::counter, opportunities, initial_reserve, GetGeologyWorldState().deposits.Get(deposit)->remaining_reserve, company.money.base(),
		head_id.base(), hold_id.base(), mine->index.base(), receiver->index.base(), deposit.base(), home.base(), cargo, x, row, path.string());
}

static std::function<void()> *_shield_source_observer = nullptr; ///< Invocation-local native startup callback; never a saved source or quantity owner.

/** @param args Native console arguments after ordinary loading. @return Command admission; execute only the invocation's original source observer. */
static bool ObserveShieldSource(std::span<std::string_view> args)
{
	if (args.empty()) return true;
	REQUIRE(_shield_source_observer != nullptr); (*_shield_source_observer)(); return true;
}

/** Native prices/definitions/date/capabilities must fund original Deep machinery and earn20 finite RARE without moving the stopped actual sealed carrier. */
TEST_CASE("Shield original source earns actual native finite Deep RARE stock", "[.tube-shield-source-stock-native]")
{
	const char *input = std::getenv("OTTD_SHIELD_SOURCE_INPUT"); REQUIRE(input != nullptr);
	const auto paths = GetTestCaseArtifactPaths("tubes", "shield-source"); const auto config = GetTestRuntimeConfig(); bool observed = false; std::exception_ptr failure;
	/** Normal application loading owns all content properties and prices before actual source funding or work; no reduced producer fixture runs in this observer. */
	std::function<void()> inspect = [&] {
		try {
			AutoRestoreBackup actor{_current_company, _local_company}; AutoRestoreBackup pause{_pause_mode, PauseModes{}};
			EarnShieldSourceStock(paths);
		} catch (...) { failure = std::current_exception(); }
		observed = true; _exit_game = true;
	};
	AutoRestoreBackup observer{_shield_source_observer, &inspect}; IConsole::CmdRegister("shield_source_stock_check", ObserveShieldSource);
	const std::string config_path = config.string();
	std::array<std::string_view, 17> arguments = {"openttd_test.exe", "-v", "null:ticks=2", "-b", "8bpp-simple", "-s", "null", "-m", "null", "-x", "-X", "-I", "original_dos", "-c", config_path, "-g", input};
	CHECK(openttd_main(arguments) == 0); if (failure) std::rethrow_exception(failure); CHECK(observed);
}

/** @param world Exact original loaded source participants. Preserve undispatched stock/reserve and absent receiver inputs/output before any useful route exists. */
static void CheckShieldSourceStock(const ShieldSourceWorld &world)
{
	const auto expected = ShieldSourceValues("OTTD_SHIELD_SOURCE_FACTS", 2);
	REQUIRE(world.industries[0]->produced[0].waiting == expected[0]); REQUIRE(GetGeologyWorldState().deposits.Get(world.deposit)->remaining_reserve == expected[1]);
	const auto *mine = GetLayerMineState(world.industries[0]->index); REQUIRE(mine != nullptr); REQUIRE(mine->extracted == expected[0]); REQUIRE(mine->dispatched == 0);
	for (const auto &entry : world.industries[1]->accepted) REQUIRE(entry.waiting == 0);
	for (const auto &entry : world.industries[1]->produced) REQUIRE(entry.waiting == 0);
	const auto &company = *Company::Get(world.parts[0]->owner); REQUIRE(company.cur_economy.income == Money{0}); REQUIRE(company.num_valid_stat_ent == 0); REQUIRE(company.current_loan == Money{0});
	REQUIRE(TimerGameCalendar::year == TimerGameCalendar::Year{1985}); REQUIRE(CargoPacket::GetNumItems() == 0);
}

/** @param world Exact original source participants. @param path Invocation-owned native output. Pure-save complete original owners and independent client RNG without a hidden work opportunity. */
static void SaveShieldSourceWorld(const ShieldSourceWorld &world, const std::filesystem::path &path)
{
	const auto before = CaptureShieldSourceWorld(world); const std::array interactive{_interactive_random.state[0], _interactive_random.state[1]};
	{
		AutoRestoreBackup mode{_game_mode, GameMode::Menu}; AutoRestoreBackup pause{_pause_mode, PauseModes{PauseMode::Normal}};
		AutoRestoreBackup compression{_savegame_format, std::string{"lzma"}};
		REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(path), false) == SaveLoadResult::Ok);
	}
	CHECK(CaptureShieldSourceWorld(world) == before); CHECK((std::array{_interactive_random.state[0], _interactive_random.state[1]} == interactive));
}

/** @param boundaries Write original/untouched-next source baselines instead of ordinary inspection/repeat/continuation. Both paths load only exact original native participants. */
static void ObserveShieldSourceFiles(bool boundaries)
{
	const char *input = std::getenv("OTTD_SHIELD_SOURCE_READ_INPUT"); REQUIRE(input != nullptr);
	const char *action_text = std::getenv("OTTD_SHIELD_SOURCE_ACTION"), *snapshot_text = std::getenv("OTTD_SHIELD_SOURCE_SNAPSHOT");
	const auto paths = GetTestCaseArtifactPaths("tubes", "shield-source"); const auto config = GetTestRuntimeConfig(); bool observed = false; std::exception_ptr failure;
	/** Reacquire original native pools before copied preview or optional work; no constructor, registry, date or funding repair participates. */
	std::function<void()> inspect = [&] {
		try {
			AutoRestoreBackup actor{_current_company, _local_company};
			auto world = LoadShieldSourceWorld(); CheckShieldSourceStock(world);
			if (boundaries) {
				AutoRestoreBackup pause{_pause_mode, PauseModes{}};
				// Boundary1 is exactly the next common opportunity from the same
				// original native input, including all actual source/receiver work.
				// Saving boundary0 advances nothing and supplies no hidden credit.
				for (uint step = 0; step < 2; ++step) {
					if (step != 0) { RunShieldSourceOpportunity(); CheckShieldSourceStock(world); }
					const auto path = paths.Output("saves", fmt::format("shield-source-boundary-{}.sav", step)); SaveShieldSourceWorld(world, path);
					fmt::print("SHIELD_SOURCE_WORLD step={} tick={} parts={},{} references={},{},{},{} snapshot={} path={}\n", step, TimerGameTick::counter,
						world.parts[0]->index.base(), world.parts[1]->index.base(), world.industries[0]->index.base(), world.industries[1]->index.base(), world.deposit.base(), world.home.base(), CaptureShieldSourceWorld(world), path.string());
				}
			} else {
				REQUIRE(action_text != nullptr); REQUIRE(snapshot_text != nullptr); const std::string_view action{action_text}; const std::string expected{snapshot_text};
				// Only an explicit next common opportunity may alter source work.
				// Inspection/preview cannot enter the construction/writer branch.
				REQUIRE((action == "inspect" || action == "step"));
				const auto actual = CaptureShieldSourceWorld(world);
				if (actual != expected) fmt::print("SHIELD_SOURCE_SNAPSHOT actual={}\n", actual);
				REQUIRE(actual == expected); const std::array interactive{_interactive_random.state[0], _interactive_random.state[1]};
				REQUIRE(SaveOrLoad(input, SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Ok);
				world = LoadShieldSourceWorld(); REQUIRE(CaptureShieldSourceWorld(world) == expected);
				CHECK((std::array{_interactive_random.state[0], _interactive_random.state[1]} == interactive));
				const auto tick = TimerGameTick::counter;
				if (action == "step") {
					AutoRestoreBackup pause{_pause_mode, PauseModes{}}; RunShieldSourceOpportunity(); CheckShieldSourceStock(world); CHECK(TimerGameTick::counter == tick + 1);
				}
				const auto path = paths.Output("saves", fmt::format("{}-{}.sav", std::filesystem::path{input}.stem().string(), action)); SaveShieldSourceWorld(world, path);
				fmt::print("SHIELD_SOURCE_READ action={} original_parts=2 native_industries=2 preview_pure=true full_native_clock=true snapshot={} path={}\n", action, CaptureShieldSourceWorld(world), path.string());
			}
		} catch (...) { failure = std::current_exception(); }
		observed = true; _exit_game = true;
	};
	AutoRestoreBackup observer{_shield_source_observer, &inspect}; IConsole::CmdRegister(boundaries ? "shield_source_boundaries_check" : "shield_source_file_check", ObserveShieldSource);
	const std::string config_path = config.string();
	std::array<std::string_view, 17> arguments = {"openttd_test.exe", "-v", "null:ticks=2", "-b", "8bpp-simple", "-s", "null", "-m", "null", "-x", "-X", "-I", "original_dos", "-c", config_path, "-g", input};
	CHECK(openttd_main(arguments) == 0); if (failure) std::rethrow_exception(failure); CHECK(observed);
}

/** Ordinary original source loading writes pure complete boundaries and its genuine untouched next shared opportunity. */
TEST_CASE("Original Shield source writes complete untouched native boundaries", "[.tube-shield-source-boundaries-native]") { ObserveShieldSourceFiles(true); }

/** Ordinary source loading/preview/repeat and one complete opportunity retain every original native owner without reconstruction. */
TEST_CASE("Original Shield source files retain native owners and complete continuation", "[.tube-shield-source-read-native]") { ObserveShieldSourceFiles(false); }
