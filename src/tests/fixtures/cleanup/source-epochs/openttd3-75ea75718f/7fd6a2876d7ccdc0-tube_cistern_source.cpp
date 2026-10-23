/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file tube_cistern_source.cpp Native-content startup and original liquid-carrier prerequisite for the1980 Cistern source.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Group15DB.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
 * An empty prepared world earns no cargo; actual ordinary loading supplies all distributed cargo/engine properties before source commands.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include <charconv>
#include "mock_environment.h"
#include "layer_window_fixture.h"
#include "layer_save_fixture.h"
#include "excavation_fixture.h"
#include "test_artifact_paths.h"
#include "../company_manager_face.h"
#include "../engine_func.h"
#include "../underground_vehicle.h"
#include "../tbm.h"
#include "../space_vehicle.h"
#include "../transport_tube.h"
#include "../industry.h"
#include "../industrytype.h"
#include "../industry_cmd.h"
#include "../layer_industry_processor.h"
#include "../town.h"
#include "../train.h"
#include "../cargopacket.h"
#include "../road.h"
#include "../timer/timer_game_calendar.h"
#include "../timer/timer_game_economy.h"
#include "../timer/timer_game_tick.h"
#include "../newgrf_station.h"
#include "../newgrf_roadstop.h"
#include "../rail_cmd.h"
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
#include "../table/strings.h"
#include "tube_cistern_source_fixture.h"
#include "../safeguards.h"

extern TileIndex _cur_tileloop_tile; ///< Native nonzero landscape cursor established before the empty transport save.
/** Native new-world economy initialization shared with world generation; this fixture owns no separate tariff/price table. */
void StartupEconomy();
/** Advance actual industry producers once through the same native entry used by the game loop. */
void OnTick_Industry();

/** Write only an empty author-prepared world; this transport input cannot establish a paying source or a useful Cistern shipment. */
TEST_CASE("Cistern source starts with an empty native-content transport world", "[tube-cistern-source-template]")
{
	MockEnvironment::Instance(); LoadGeneratedWindowTestLanguage(); AutoRestoreBackup game_settings{_settings_game};
	// A useful route must inherit real saved game defaults, including calendar
	// pacing and pathfinder budgets. Reset through the native descriptors as
	// ordinary initialization does, before constructing any world participant;
	// unrelated client-only settings remain outside this authored scenario.
	for (const auto &entry : GetSaveLoadSettingTable()) {
		const auto *setting = GetSettingDesc(entry);
		if (!setting->flags.Test(SettingFlag::NotInSave)) setting->ResetToDefault(&_settings_game);
	}
	ExcavationFixture rig(true);
	AutoRestoreBackup calendar_date{TimerGameCalendar::date}; AutoRestoreBackup calendar_year{TimerGameCalendar::year};
	AutoRestoreBackup calendar_month{TimerGameCalendar::month}; AutoRestoreBackup calendar_fraction{TimerGameCalendar::date_fract};
	AutoRestoreBackup economy_date{TimerGameEconomy::date}; AutoRestoreBackup economy_year{TimerGameEconomy::year};
	AutoRestoreBackup economy_month{TimerGameEconomy::month}; AutoRestoreBackup economy_fraction{TimerGameEconomy::date_fract};
	AutoRestoreBackup economy{_economy}; AutoRestoreBackup tick{TimerGameTick::counter, uint64_t{0}};
	AutoRestoreBackup prices{_price};
	AutoRestoreBackup raw_industry{_settings_game.construction.raw_industry_construction, uint8_t{1}};
	AutoRestoreBackup tile_loop{_cur_tileloop_tile, TileIndex{1}}; AutoRestoreBackup local{_local_company, _current_company};
	AutoRestoreBackup mappings{_engine_mngr.mappings}; LayerSaveContent content(rig.ActivatedContent(), true);
	TimerGameCalendar::SetDate(TimerGameCalendar::ConvertYMDToDate(TimerGameCalendar::Year{1980}, 0, 1), 0);
	TimerGameEconomy::SetDate(TimerGameEconomy::ConvertYMDToDate(TimerGameEconomy::Year{1980}, 0, 1), 0);
	_economy.inflation_prices = _economy.inflation_payment = uint64_t{1} << 16;
	// Save ordinary legal scenario options before any native acquisition;
	// zero-initialized unit-test train limits cannot define the player world.
	_settings_game.vehicle.max_trains = 500; _settings_game.vehicle.max_train_length = 7;
	_settings_game.vehicle.freight_trains = 1; _settings_game.vehicle.train_acceleration_model = AccelerationModel::Realistic;
	ResetFaces(); SetCompanyManagerFaceStyle(Company::Get(_current_company)->face, 0);
	Company::Get(_current_company)->money = 16000000; // Explicit author funds cover all three native source facilities; never loaded cash repair, freight revenue or a profitability claim.
	REQUIRE(Town::CanAllocateItem()); auto *town = Town::Create(TileXYL(10, 10, MapLayer::Surface)); town->townnametype = SPECSTR_TOWNNAME_START; RebuildTownKdtree();
	ResetRailTypes(); ResetRoadTypes(); StationClass::Reset(); ResetIndustries();
	_engine_mngr.ResetToDefaultMapping(); SetupEngines();
	// Normal startup owns the catalogue and engine availability before any
	// later ordinary application purchase. This empty world carries no train
	// or cargo whose physics could rely on reduced unit-test CargoSpec fields.
	REQUIRE(InitialiseTBMEngineDefinitions(rig.ActivatedContent())); REQUIRE(InitialiseUndergroundEngineDefinitions(rig.ActivatedContent()));
	REQUIRE(InitialiseSpaceEngineDefinition()); REQUIRE(InitialiseTubeRailType(rig.ActivatedContent()) != INVALID_RAILTYPE);
	REQUIRE(InitialiseTubeEngineDefinitions(rig.ActivatedContent())); StartupEngines();
	StartupEconomy();
	REQUIRE(Vehicle::GetNumItems() == 0); REQUIRE(Industry::GetNumItems() == 0); REQUIRE(CargoPacket::GetNumItems() == 0);
	const auto paths = GetTestCaseArtifactPaths("tubes", "cistern-source");
	const auto path = paths.Output("saves", "cistern-empty-native.sav");
	SetMouseCursor(SPR_CURSOR_MOUSE, PAL_NONE);
	AutoRestoreBackup mode{_game_mode, GameMode::Menu}; AutoRestoreBackup pause{_pause_mode, PauseModes{PauseMode::Normal}};
	AutoRestoreBackup compression{_savegame_format, std::string{"lzma"}};
	REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(path), false) == SaveLoadResult::Ok);
	fmt::print("CISTERN_SEED year=1980 vehicles=0 industries=0 packets=0 useful_source=false path={}\n", path.string());
	_engine_pool.CleanPool();
	// Retire this empty metadata-only town before its author fixture ends.
	delete town; RebuildTownKdtree();
}

/** @param paths Invocation-owned native output locations. Check original tanker refitting under actual loaded content; no source/output stock is supplied. */
static void CheckNativeCisternCarrier(const TestArtifactPaths &paths)
{
	REQUIRE(TimerGameCalendar::year == TimerGameCalendar::Year{1980}); REQUIRE(Vehicle::GetNumItems() == 0); REQUIRE(Industry::GetNumItems() == 0);
	REQUIRE(_settings_game.economy.minutes_per_calendar_year == CalendarTime::DEF_MINUTES_PER_YEAR);
	REQUIRE(_settings_game.pf.yapf.max_search_nodes > 0); REQUIRE(_economy.fluct > 0);
	const auto chemical = GetCargoTypeByLabel(CargoLabel{"CHEM"}); REQUIRE(chemical != INVALID_CARGO);
	const auto *contract = GetMultiLayerCargoContract(CargoLabel{"CHEM"}); REQUIRE(contract != nullptr); REQUIRE(contract->form == MultiLayerCargoForm::Liquid);
	REQUIRE(CargoSpec::Get(chemical)->classes.Test(CargoClass::Liquid)); REQUIRE(CargoSpec::Get(chemical)->is_freight); REQUIRE(CargoSpec::Get(chemical)->weight > 0);
	EngineID tanker = EngineID::Invalid();
	// Select an actual original oil wagon by its native immutable label and
	// active refit mask, never by a guessed ID or a widened solid-wagon policy.
	for (const auto *engine : Engine::IterateType(VehicleType::Train)) {
		const auto *label = std::get_if<CargoLabel>(&engine->info.cargo_label);
		if (label == nullptr) continue;
		if (*label != CargoLabel{"OIL_"}) continue;
		if (!engine->GetGRFID().Empty()) continue;
		if (engine->VehInfo<RailVehicleInfo>().railveh_type != RailVehicleType::Wagon) continue;
		if (!engine->info.refit_mask.Test(chemical)) continue;
		tanker = engine->index; break;
	}
	REQUIRE(tanker != EngineID::Invalid()); REQUIRE(CheckUndergroundVehicleCapability(tanker, MapLayer::Underground, chemical).Succeeded());
	const auto depot = TileXYL(4, 20, MapLayer::Surface);
	REQUIRE(Command<Commands::BuildRailDepot>::Do(DoCommandFlag::Execute, depot, RAILTYPE_RAIL, DiagDirection::SW).Succeeded());
	const auto [head_cost, head_id, head_capacity, head_mail, head_capacities] = Command<Commands::BuildVehicle>::Do(DoCommandFlag::Execute, depot, GetUndergroundEngine(4), false, INVALID_CARGO, ClientID::Invalid);
	INFO(fmt::format("native hauler={} purchase error={}", GetUndergroundEngine(4).base(), head_cost.GetErrorMessage()));
	REQUIRE(head_cost.Succeeded());
	const auto [tank_cost, tank_id, tank_capacity, tank_mail, tank_capacities] = Command<Commands::BuildVehicle>::Do(DoCommandFlag::Execute, depot, tanker, false, chemical, ClientID::Invalid);
	INFO(fmt::format("original tanker={} purchase/refit error={}", tanker.base(), tank_cost.GetErrorMessage()));
	REQUIRE(tank_cost.Succeeded()); REQUIRE(Command<Commands::MoveRailVehicle>::Do(DoCommandFlag::Execute, tank_id, head_id, false).Succeeded());
	const auto *head = Train::Get(head_id), *tank = Train::Get(tank_id); REQUIRE(tank->First() == head); REQUIRE(tank->cargo_type == chemical); REQUIRE(tank->cargo_cap > 0);
	REQUIRE(head->IsChainInDepot()); REQUIRE(CargoPacket::GetNumItems() == 0);
	IndustryType oil_type = IT_INVALID;
	// Select the real original Oil-well producer from the native catalogue;
	// neither a guessed type nor an Oil-refinery output can impersonate it.
	for (IndustryType type = 0; type < NEW_INDUSTRYOFFSET; ++type) {
		if (GetIndustrySpec(type)->name == STR_INDUSTRY_NAME_OIL_WELLS) { oil_type = type; break; }
	}
	REQUIRE(oil_type != IT_INVALID);
	const auto chemical_type = GetLayerProcessorIndustryType(MultiLayerRecipeID{"CHEM"}); REQUIRE(chemical_type != IT_INVALID);
	const auto quote_random = std::array{_random.state[0], _random.state[1]}; const auto quote_cash = Company::Get(_current_company)->money;
	// Source admission is a pure prerequisite. Neither estimate may allocate
	// machinery, spend the carrier's remaining funds or advance production RNG
	// before the actual source owns its accepted native participants.
	const auto oil_quote = Command<Commands::BuildIndustry>::Do({}, TileXYL(7, 14, MapLayer::Surface), oil_type, 0, true, 123);
	INFO(fmt::format("original Oil-well1980 estimate error={}", oil_quote.GetErrorMessage())); REQUIRE(oil_quote.Succeeded());
	const auto chemical_quote = Command<Commands::BuildIndustry>::Do({}, TileXYL(26, 15, MapLayer::Surface), chemical_type, 0, true, 123);
	INFO(fmt::format("native CHEM1980 estimate error={}", chemical_quote.GetErrorMessage())); REQUIRE(chemical_quote.Succeeded());
	REQUIRE(Industry::GetNumItems() == 0); REQUIRE(Company::Get(_current_company)->money == quote_cash);
	REQUIRE((std::array{_random.state[0], _random.state[1]} == quote_random));
	const auto deposits = GetGeologyWorldState().deposits.Records();
	const auto salt = std::ranges::find(deposits, CargoLabel{"SALT"}, &GeologyDeposit::resource);
	REQUIRE(salt != deposits.end()); REQUIRE(salt->layer == MapLayer::Underground); REQUIRE_FALSE(salt->spans.empty());
	fmt::print("CISTERN_SOURCES oil_type={} chemical_type={} salt_deposit={} x={} y={} useful_source=false\n", oil_type, chemical_type,
		salt->id.base(), salt->spans.front().first_x, salt->spans.front().y);
	const auto path = paths.Output("saves", "cistern-original-carrier.sav");
	AutoRestoreBackup mode{_game_mode, GameMode::Menu}; AutoRestoreBackup pause{_pause_mode, PauseModes{PauseMode::Normal}};
	AutoRestoreBackup compression{_savegame_format, std::string{"lzma"}};
	REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(path), false) == SaveLoadResult::Ok);
	fmt::print("CISTERN_CARRIER year=1980 original_engine={} head={} tanker={} cargo={} capacity={} packets=0 useful_source=false path={}\n", tanker.base(), head_id.base(), tank_id.base(), chemical, tank->cargo_cap, path.string());
}

static std::function<void()> *_cistern_source_observer = nullptr; ///< Ordinary startup observation, never cargo/arrival/accounting authority.

/** @param arguments Actual startup console tokens. @return Success after the native carrier prerequisite observer completes. */
static bool ObserveCisternSource(std::span<std::string_view> arguments)
{
	REQUIRE(arguments.size() == 1); REQUIRE(_cistern_source_observer != nullptr); (*_cistern_source_observer)(); return true;
}

/** Ordinary application loading must provide real distributed liquid properties before the Cistern source can purchase a carrier. */
TEST_CASE("Cistern source uses an original tanker under ordinary native content", "[.tube-cistern-carrier-native]")
{
	const char *input = std::getenv("OTTD_CISTERN_SOURCE_INPUT"); REQUIRE(input != nullptr);
	const auto paths = GetTestCaseArtifactPaths("tubes", "cistern-source"); const auto config = GetTestRuntimeConfig();
	bool observed = false; std::exception_ptr failure;
	/** Run only after normal native loading accepts original package/cargo/engine owners; do not install a fixture registry. */
	std::function<void()> inspect = [&] {
		try { AutoRestoreBackup actor{_current_company, _local_company}; CheckNativeCisternCarrier(paths); }
		catch (...) { failure = std::current_exception(); }
		observed = true; _exit_game = true;
	};
	AutoRestoreBackup observer{_cistern_source_observer, &inspect}; IConsole::CmdRegister("cistern_source_check", ObserveCisternSource);
	const std::string config_path = config.string();
	std::array<std::string_view, 17> arguments = {"openttd_test.exe", "-v", "null:ticks=2", "-b", "8bpp-simple", "-s", "null", "-m", "null", "-x", "-X", "-I", "original_dos", "-c", config_path, "-g", input};
	CHECK(openttd_main(arguments) == 0); if (failure) std::rethrow_exception(failure); CHECK(observed);
}

/** @param name Required exact original carrier-report field. @return Fully parsed unsigned native identity; missing/truncated values fail before source construction. */
static uint64_t CisternSourceOriginalValue(const char *name)
{
	const char *text = std::getenv(name); REQUIRE(text != nullptr); uint64_t value = 0;
	const auto parsed = std::from_chars(text, text + strlen(text), value);
	REQUIRE(parsed.ec == std::errc{}); REQUIRE(parsed.ptr == text + strlen(text)); return value;
}

/** @param world Actual funded native source and retained original carrier. @param deposit Original geological Salt identity. @param paths Invocation-owned native stock world. Earn raw stock while every train remains player-stopped; no useful chemical output is claimed. */
static void EarnStoppedCisternSourceStocks(CisternSourceStockWorld &world, GeologyDepositID deposit, const TestArtifactPaths &paths)
{
	const auto minimum = GetIndustrySpec(world.oil_well->type)->minimal_cargo; uint oil_opportunities = 0;
	// The first native lot must meet both the real Oil dispatch threshold and
	// enough input for20 Chemicals. Oil runs before the finite mine is funded,
	// so the shared industry entry cannot secretly advance Salt production too.
	while ((world.oil_well->produced[0].waiting < 20 || world.oil_well->produced[0].waiting <= minimum) && oil_opportunities++ < 2048) {
		world.RunStopped(); OnTick_Industry();
	}
	REQUIRE(oil_opportunities <= 2048); const uint oil_units = world.oil_well->produced[0].waiting; REQUIRE(oil_units >= 20);
	world.oil_pickup->goods[world.oil].rating = UINT8_MAX; world.oil_pickup->goods[world.oil].last_speed = 100;
	uint oil_cells = 0;
	// Only actual original Oil-well cells perform native dispatch. A source
	// packet cannot be manufactured from a guessed footprint or station amount.
	for (const auto tile : world.oil_well->location) if (world.oil_well->TileBelongsToIndustry(tile)) {
		++oil_cells; _tile_type_procs[TileType::Industry]->tile_loop_proc(tile);
	}
	REQUIRE(oil_cells > 0); REQUIRE(world.oil_well->produced[0].waiting == 0);
	REQUIRE(world.oil_pickup->goods[world.oil].TotalCount() == oil_units);
	world.PrepareSalt(deposit);
	const auto reserve = GetGeologyWorldState().deposits.Get(deposit)->remaining_reserve; uint supplied = 0;
	// Five real74-tick opportunities earn the finite20-unit lot through the
	// same sole production/dispatch owners as the existing mine route. Neither
	// native work nor reserve debit is replaced by a station stock assignment.
	for (uint opportunity = 0; opportunity < 5; ++opportunity) {
		for (uint tick = 0; tick < 74; ++tick) world.RunStopped();
		world.salt_pickup->goods[world.salt].rating = UINT8_MAX; world.salt_pickup->goods[world.salt].last_speed = 100;
		REQUIRE(ProduceLayerMineCargo(*world.salt_mine, 74, true).amount == 4);
		supplied += DispatchLayerMineCargo(*world.salt_mine);
	}
	REQUIRE(supplied == 20); REQUIRE(world.salt_pickup->goods[world.salt].TotalCount() == supplied);
	REQUIRE(GetGeologyWorldState().deposits.Get(deposit)->remaining_reserve == reserve - supplied);
	REQUIRE(world.oil_pickup->goods[world.oil].TotalCount() == oil_units);
	// Stopped raw stock cannot earn input delivery or chemical output. Inspect
	// every native inventory slot, including alternative recipe products, so
	// an inherited legacy rate cannot manufacture a hidden useful shipment.
	for (const auto &slot : world.chemicals->accepted) CHECK(slot.waiting == 0);
	for (const auto &slot : world.chemicals->produced) CHECK(slot.waiting == 0);
	const auto &company = *Company::Get(_current_company); CHECK(company.cur_economy.income == Money{0});
	CHECK(company.cur_economy.delivered_cargo[world.oil] == 0); CHECK(company.cur_economy.delivered_cargo[world.salt] == 0);
	uint oil_packets = 0, salt_packets = 0;
	// Preserve every actual original packet, including distinct ages from the
	// five finite batches. No packet may change source/first station/purpose or
	// be merged into a fake single-packet fixture expectation.
	for (const auto *packet : CargoPacket::Iterate()) {
		if (packet->GetSource() == Source{world.oil_well->index, SourceType::Industry}) {
			oil_packets += packet->Count(); CHECK(packet->GetFirstStation() == world.oil_pickup->index);
		} else {
			REQUIRE(packet->GetSource() == Source{world.salt_mine->index, SourceType::Industry});
			salt_packets += packet->Count(); CHECK(packet->GetFirstStation() == world.salt_pickup->index);
		}
		CHECK(packet->GetCompanySupplyOrder() == 0);
		fmt::print("CISTERN_STOCK_PACKET id={} units={} source={} station={} age={} purpose={}\n", packet->index.base(), packet->Count(),
			packet->GetSource().id, packet->GetFirstStation().base(), packet->GetPeriodsInTransit(), packet->GetCompanySupplyOrder());
	}
	CHECK(oil_packets == oil_units); CHECK(salt_packets == supplied);
	fmt::print("CISTERN_STOCK_FINANCE cash={} loan={} income=0\n", company.money.base(), company.current_loan.base());
	const auto original = std::tuple{company.money, CargoPacket::GetNumItems(), TimerGameTick::counter,
		std::array{_random.state[0], _random.state[1], _interactive_random.state[0], _interactive_random.state[1]}};
	const auto path = paths.Output("saves", "cistern-source-stocks.sav");
	AutoRestoreBackup mode{_game_mode, GameMode::Menu}; AutoRestoreBackup pause{_pause_mode, PauseModes{PauseMode::Normal}};
	AutoRestoreBackup compression{_savegame_format, std::string{"lzma"}};
	REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(path), false) == SaveLoadResult::Ok);
	CHECK((std::tuple{company.money, CargoPacket::GetNumItems(), TimerGameTick::counter,
		std::array{_random.state[0], _random.state[1], _interactive_random.state[0], _interactive_random.state[1]}} == original));
	fmt::print("CISTERN_STOCKS tick={} oil={} salt={} oil_opportunities={} salt_opportunities=5 reserve_before={} reserve_after={} parts={},{},{},{} industries={},{},{} stations={},{},{} source_income=0 moving=0 chemical_output=0 path={}\n",
		TimerGameTick::counter, oil_units, supplied, oil_opportunities, reserve, GetGeologyWorldState().deposits.Get(deposit)->remaining_reserve,
		world.oil_front->index.base(), world.oil_tanker->index.base(), world.salt_front->index.base(), world.salt_wagon->index.base(),
		world.oil_well->index.base(), world.chemicals->index.base(), world.salt_mine->index.base(),
		world.oil_pickup->index.base(), world.chemical_berth->index.base(), world.salt_pickup->index.base(), path.string());
}

/** An accepted original carrier world must supply all real native participants before stopped precursor stocks are earned. */
TEST_CASE("Cistern precursor stocks come from actual Oil and finite Salt", "[.tube-cistern-source-stocks-native]")
{
	const char *input = std::getenv("OTTD_CISTERN_SOURCE_INPUT"); REQUIRE(input != nullptr);
	const auto paths = GetTestCaseArtifactPaths("tubes", "cistern-source"); const auto config = GetTestRuntimeConfig();
	bool observed = false; std::exception_ptr failure;
	/** Reacquire only original accepted parts; all source commands/work execute in the same ordinary native world. */
	std::function<void()> inspect = [&] {
		try {
			AutoRestoreBackup actor{_current_company, _local_company}; AutoRestoreBackup pause{_pause_mode, PauseModes{}};
			const auto head_id = CisternSourceOriginalValue("OTTD_CISTERN_SOURCE_HEAD"), tank_id = CisternSourceOriginalValue("OTTD_CISTERN_SOURCE_TANK");
			const auto original_engine = CisternSourceOriginalValue("OTTD_CISTERN_SOURCE_ORIGINAL_ENGINE");
			const auto oil_type = CisternSourceOriginalValue("OTTD_CISTERN_SOURCE_OIL_TYPE"), chemical_type = CisternSourceOriginalValue("OTTD_CISTERN_SOURCE_CHEM_TYPE");
			const auto deposit = CisternSourceOriginalValue("OTTD_CISTERN_SOURCE_DEPOSIT");
			REQUIRE(head_id < UINT32_MAX); REQUIRE(tank_id < UINT32_MAX); REQUIRE(original_engine < UINT16_MAX);
			REQUIRE(oil_type < IT_INVALID); REQUIRE(chemical_type < IT_INVALID); REQUIRE(deposit < UINT32_MAX);
			auto *head = Train::GetIfValid(VehicleID{static_cast<uint32_t>(head_id)}), *tank = Train::GetIfValid(VehicleID{static_cast<uint32_t>(tank_id)});
			REQUIRE(head != nullptr); REQUIRE(tank != nullptr); REQUIRE(tank->First() == head); REQUIRE(head->engine_type == GetUndergroundEngine(4));
			REQUIRE(tank->engine_type.base() == original_engine); REQUIRE(tank->cargo_type == GetCargoTypeByLabel(CargoLabel{"CHEM"}));
			REQUIRE(Vehicle::GetNumItems() == 2); REQUIRE(Industry::GetNumItems() == 0); REQUIRE(CargoPacket::GetNumItems() == 0);
			CisternSourceStockWorld world(*head, *tank, static_cast<IndustryType>(oil_type), static_cast<IndustryType>(chemical_type));
			EarnStoppedCisternSourceStocks(world, GeologyDepositID{static_cast<uint32_t>(deposit)}, paths);
		} catch (...) { failure = std::current_exception(); }
		observed = true; _exit_game = true;
	};
	AutoRestoreBackup observer{_cistern_source_observer, &inspect}; IConsole::CmdRegister("cistern_stocks_check", ObserveCisternSource);
	const std::string config_path = config.string();
	std::array<std::string_view, 17> arguments = {"openttd_test.exe", "-v", "null:ticks=2", "-b", "8bpp-simple", "-s", "null", "-m", "null", "-x", "-X", "-I", "original_dos", "-c", config_path, "-g", input};
	CHECK(openttd_main(arguments) == 0); if (failure) std::rethrow_exception(failure); CHECK(observed);
}
