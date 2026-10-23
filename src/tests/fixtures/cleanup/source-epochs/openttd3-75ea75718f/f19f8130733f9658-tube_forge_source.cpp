/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file tube_forge_source.cpp Actual2005 native prerequisites for the finite CORE/Coal to RFMT Forge route.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Groups15FBA/15FBB/15FBC.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
 * The empty author has no source quantities. Normal application loading alone supplies native definitions and prices.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "layer_window_fixture.h"
#include "layer_save_fixture.h"
#include "tube_source_template_fixture.h"
#include "forge_source_world_fixture.h"
#include "forge_source_snapshot_fixture.h"
#include "test_artifact_paths.h"
#include "../engine_base.h"
#include "../company_base.h"
#include "../industry.h"
#include "../industrytype.h"
#include "../layer_industry_processor.h"
#include "../multilayer_industry.h"
#include "../transport_tube.h"
#include "../underground_vehicle.h"
#include "../train.h"
#include "../cargopacket.h"
#include "../industry_cmd.h"
#include "../industry_map.h"
#include "../rail_cmd.h"
#include "../vehicle_cmd.h"
#include "../train_cmd.h"
#include "../command_func.h"
#include "../company_func.h"
#include "../console_internal.h"
#include "../timer/timer_game_calendar.h"
#include "../timer/timer_game_economy.h"
#include "../timer/timer_game_tick.h"
#include "../saveload/saveload_func.h"
#include "../safeguards.h"

/** The disclosed2005 initial calendar/funds precede every real source purchase, stock, packet and transport accomplishment. */
TEST_CASE("Forge source starts with an empty original2005 native world", "[tube-forge-source-template]")
{
	const auto path = WriteTubeUsefulSourceTemplate(2005, "forge-source", "forge-empty-native.sav", 32000000);
	fmt::print("FORGE_SEED year=2005 vehicles=0 industries=0 packets=0 useful_source=false path={}\n", path.string());
}

/** @param paths Invocation-owned complete native output. Require actual dated/refittable native machinery and recipe definitions without buying participants or granting quantities. */
static void CheckForgeSourceDefinitions(const TestArtifactPaths &paths)
{
	REQUIRE(TimerGameCalendar::year == TimerGameCalendar::Year{2005}); REQUIRE(TimerGameEconomy::year == TimerGameEconomy::Year{2005}); REQUIRE(TimerGameTick::counter == 0);
	REQUIRE(Vehicle::GetNumItems() == 0); REQUIRE(Industry::GetNumItems() == 0); REQUIRE(Station::GetNumItems() == 0); REQUIRE(CargoPacket::GetNumItems() == 0);
	const auto &company = *Company::Get(_current_company); REQUIRE(company.money == Money{32000000}); REQUIRE(company.current_loan == Money{0}); REQUIRE(company.cur_economy.income == Money{0});
	const auto *capsule = Engine::Get(GetTubeEngine(13)); REQUIRE(capsule != nullptr); REQUIRE(capsule->company_avail.Test(_current_company));
	REQUIRE(capsule->intro_date == TimerGameCalendar::ConvertYMDToDate(TimerGameCalendar::Year{2005}, 0, 1)); REQUIRE(capsule->info.base_intro == capsule->intro_date);
	const auto capability = GetTubeVehicleCapability(capsule->index); REQUIRE(capability.has_value()); REQUIRE(capability->catalogue->model_id == 13);
	const auto *head = Engine::Get(GetUndergroundEngine(4)), *wagon = Engine::Get(GetUndergroundEngine(5)); REQUIRE(head != nullptr); REQUIRE(wagon != nullptr);
	REQUIRE(head->company_avail.Test(_current_company)); REQUIRE(wagon->company_avail.Test(_current_company));
	const auto coal = GetCargoTypeByLabel(CargoLabel{"COAL"}), copper = GetCargoTypeByLabel(CargoLabel{"CORE"}), refined = GetCargoTypeByLabel(CargoLabel{"RFMT"});
	REQUIRE(coal != INVALID_CARGO); REQUIRE(copper != INVALID_CARGO); REQUIRE(refined != INVALID_CARGO);
	// Both finite raw inputs and the processed cargo must remain actual native
	// freight refits. A broad cargo class or a capsule picture grants no carriage.
	for (const auto cargo : {coal, copper, refined}) {
		REQUIRE(wagon->info.refit_mask.Test(cargo));
		REQUIRE(CheckUndergroundVehicleCapability(wagon->index, MapLayer::Underground, cargo).Succeeded());
		REQUIRE(CheckUndergroundVehicleCapability(wagon->index, MapLayer::Surface, cargo).Succeeded());
	}
	REQUIRE(capsule->info.refit_mask.Test(refined));
	// Forge's four original heavy meanings remain exact native refits; no
	// source-specific whitelist can silently replace the shared public catalogue.
	for (const auto label : {CargoLabel{"STEL"}, CargoLabel{"RFMT"}, CargoLabel{"SCMP"}, CargoLabel{"RBPT"}}) {
		const auto cargo = GetCargoTypeByLabel(label); REQUIRE(cargo != INVALID_CARGO); REQUIRE(capsule->info.refit_mask.Test(cargo));
	}
	const auto refinery = GetLayerProcessorIndustryType(MultiLayerRecipeID{"CFMT"}), receiver = GetLayerProcessorIndustryType(MultiLayerRecipeID{"ELEC"});
	REQUIRE(refinery != IT_INVALID); REQUIRE(receiver != IT_INVALID); REQUIRE(IsLayerProcessorDefinition(refinery)); REQUIRE(IsLayerProcessorDefinition(receiver));
	const auto &refinery_spec = *GetIndustrySpec(refinery), &receiver_spec = *GetIndustrySpec(receiver);
	REQUIRE(std::ranges::find(refinery_spec.accepts_cargo, coal) != refinery_spec.accepts_cargo.end()); REQUIRE(std::ranges::find(refinery_spec.accepts_cargo, copper) != refinery_spec.accepts_cargo.end());
	REQUIRE(std::ranges::find(refinery_spec.produced_cargo, refined) != refinery_spec.produced_cargo.end()); REQUIRE(std::ranges::find(receiver_spec.accepts_cargo, refined) != receiver_spec.accepts_cargo.end());
	// The actual generated deposits and mine definitions must exist before
	// source construction can select a real claim; this pure query creates no
	// reserve, machinery, floor, work or cargo packet.
	for (const auto label : {CargoLabel{"CORE"}, CargoLabel{"COAL"}}) {
		const auto type = GetLayerMineIndustryType(label); REQUIRE(type != IT_INVALID);
		const auto deposits = GetGeologyWorldState().deposits.Records(); const auto deposit = std::ranges::find(deposits, label, &GeologyDeposit::resource);
		REQUIRE(deposit != deposits.end()); REQUIRE(deposit->layer == MapLayer::Underground); REQUIRE_FALSE(deposit->spans.empty()); REQUIRE(deposit->remaining_reserve == deposit->initial_reserve);
		fmt::print("FORGE_RAW label={} cargo={} type={} deposit={} reserve={} x={} y={}\n", label.AsString(), GetCargoTypeByLabel(label), type, deposit->id.base(), deposit->remaining_reserve, deposit->spans.front().first_x, deposit->spans.front().y);
	}
	const auto before = std::tuple{TimerGameTick::counter, company.money, ComputeLayerMineDigest(), ComputeLayerProcessorDigest(),
		std::array{_random.state[0], _random.state[1], _interactive_random.state[0], _interactive_random.state[1]}};
	const auto path = paths.Output("saves", "forge-native-prerequisites.sav");
	{
		AutoRestoreBackup mode{_game_mode, GameMode::Menu}; AutoRestoreBackup pause{_pause_mode, PauseModes{PauseMode::Normal}};
		AutoRestoreBackup compression{_savegame_format, std::string{"lzma"}};
		REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(path), false) == SaveLoadResult::Ok);
	}
	CHECK((before == std::tuple{TimerGameTick::counter, company.money, ComputeLayerMineDigest(), ComputeLayerProcessorDigest(),
		std::array{_random.state[0], _random.state[1], _interactive_random.state[0], _interactive_random.state[1]}}));
	fmt::print("FORGE_NATIVE year=2005 tick=0 cash={} loan=0 vehicles=0 industries=0 stations=0 packets=0 capsule={} wagon={} coal={} copper={} refined={} refinery={} receiver={} capacity={} path={}\n",
		company.money.base(), capsule->index.base(), wagon->index.base(), coal, copper, refined, refinery, receiver, capsule->VehInfo<RailVehicleInfo>().capacity, path.string());
}

/** @param industry Actual command-funded native footprint. Complete its real cells only as a disclosed initial machinery boundary, without work, inputs or output. */
static void CompleteForgeSourceMachinery(Industry &industry)
{
	// Native commands already own this machinery and its paid cost. Initial
	// construction completion never creates cargo or opens any additional rock.
	for (const auto tile : industry.location) if (industry.TileBelongsToIndustry(tile)) SetIndustryCompleted(tile);
}

/** @param expected Exact original CORE/Coal deposit identities and initial reserve observations. @return Actual command-funded mines/refinery/whole stopped empty raw carriers; the builder advances no clock or industry work. */
static ForgeSourceWorld BuildForgeRawStockWorld(const std::vector<uint64_t> &expected)
{
	ForgeSourceWorld world;
	world.cargos = {GetCargoTypeByLabel(CargoLabel{"CORE"}), GetCargoTypeByLabel(CargoLabel{"COAL"}), GetCargoTypeByLabel(CargoLabel{"RFMT"})};
	for (const auto cargo : world.cargos) REQUIRE(cargo != INVALID_CARGO);
	// Each source uses its exact original generated deposit, public complete
	// claim geometry, paid mine/depot and separately coupled real4+5 carrier.
	// Preparing the disclosed floor never regenerates reserves or earlier claims.
	for (size_t index = 0; index < world.deposits.size(); ++index) {
		REQUIRE(expected[index] < UINT32_MAX); world.deposits[index] = GeologyDepositID{static_cast<uint32_t>(expected[index])};
		const auto *deposit = GetGeologyWorldState().deposits.Get(world.deposits[index]); REQUIRE(deposit != nullptr); REQUIRE_FALSE(deposit->spans.empty());
		const auto label = CargoSpec::Get(world.cargos[index])->label; REQUIRE(deposit->resource == label); REQUIRE(deposit->layer == MapLayer::Underground);
		REQUIRE(deposit->remaining_reserve == expected[index + 2]); REQUIRE(deposit->initial_reserve == expected[index + 2]);
		const auto x = deposit->spans.front().first_x, y = deposit->spans.front().y; REQUIRE(x >= 5); REQUIRE(x < 57);
		const auto site = TileXYL(x, y, MapLayer::Underground); const auto type = GetLayerMineIndustryType(label); REQUIRE(type != IT_INVALID);
		const LayerMineRequest request{site, type, 0, world.deposits[index], _current_company}; const auto geometry = ValidateMineDepositClaim(request);
		REQUIRE_FALSE(geometry.machinery.empty()); REQUIRE_FALSE(geometry.loading.empty());
		std::vector<TileIndex> floor = geometry.machinery; floor.insert(floor.end(), geometry.loading.begin(), geometry.loading.end()); OpenShieldSourceFloor(floor);
		const auto admission = ValidateMineDepositClaim(request);
		INFO(fmt::format("Forge raw{} mine error={} quote={} cash={}", label.AsString(), to_underlying(admission.error), admission.cost.base(), GetAvailableMoneyForCommand().base()));
		REQUIRE(admission.error == LayerIndustryError::None); REQUIRE(admission.cost <= GetAvailableMoneyForCommand());
		REQUIRE(Command<Commands::BuildLayerMine>::Do(DoCommandFlag::Execute, site, type, 0, world.deposits[index], 123).Succeeded());
		world.industries[index] = Industry::GetByTile(site); REQUIRE(world.industries[index] != nullptr); CompleteForgeSourceMachinery(*world.industries[index]);
		uint row = 0;
		// Actual admitted loading cells define the carrier's future approach;
		// a guessed footprint cannot place a native depot inside machinery.
		for (const auto tile : admission.loading) row = std::max(row, TilePlaneY(tile));
		REQUIRE(row < 62); world.homes[index] = TileXYL(x - 4, row, MapLayer::Underground);
		OpenShieldSourceFloor(std::array{world.homes[index], TileAddWrap(world.homes[index], 1, 0)});
		REQUIRE(Command<Commands::BuildRailDepot>::Do(DoCommandFlag::Execute, world.homes[index], RAILTYPE_RAIL, DiagDirection::SW).Succeeded());
		const auto [head_cost, head_id, head_capacity, head_mail, head_capacities] = Command<Commands::BuildVehicle>::Do(DoCommandFlag::Execute, world.homes[index], GetUndergroundEngine(4), false, INVALID_CARGO, ClientID::Invalid);
		INFO(fmt::format("Forge raw{} front error={}", label.AsString(), head_cost.GetErrorMessage())); REQUIRE(head_cost.Succeeded());
		const auto [hold_cost, hold_id, hold_capacity, hold_mail, hold_capacities] = Command<Commands::BuildVehicle>::Do(DoCommandFlag::Execute, world.homes[index], GetUndergroundEngine(5), false, world.cargos[index], ClientID::Invalid);
		INFO(fmt::format("Forge raw{} wagon error={}", label.AsString(), hold_cost.GetErrorMessage())); REQUIRE(hold_cost.Succeeded());
		REQUIRE(Command<Commands::MoveRailVehicle>::Do(DoCommandFlag::Execute, hold_id, head_id, false).Succeeded());
		world.parts[index * 2] = Train::Get(head_id); world.parts[index * 2 + 1] = Train::Get(hold_id);
		REQUIRE(world.parts[index * 2 + 1]->First() == world.parts[index * 2]); REQUIRE(world.parts[index * 2 + 1]->cargo_type == world.cargos[index]); REQUIRE(world.parts[index * 2 + 1]->cargo_cap >= 40);
	}
	const auto refinery_type = GetLayerProcessorIndustryType(MultiLayerRecipeID{"CFMT"}); REQUIRE(refinery_type != IT_INVALID);
	const auto refinery_site = TileXYL(32, 21, MapLayer::Surface);
	const auto cost = Command<Commands::BuildIndustry>::Do(DoCommandFlag::Execute, refinery_site, refinery_type, 0, true, 123);
	INFO(fmt::format("Forge actual CFMT funding error={}", cost.GetErrorMessage())); REQUIRE(cost.Succeeded());
	world.industries[2] = Industry::GetByTile(refinery_site); REQUIRE(world.industries[2] != nullptr); CompleteForgeSourceMachinery(*world.industries[2]);
	const auto *recipe = GetLayerProcessorRecipe(*world.industries[2]); REQUIRE(recipe != nullptr); REQUIRE(recipe->recipe.id == MultiLayerRecipeID{"CFMT"});
	return world;
}

/** @param world Actual original paid raw-source participants. Preserve each finite reserve/physical/history equation and genuine stopped empty carriers; CFMT has no delivered input or manufactured output. */
static void CheckForgeRawStockConservation(const ForgeSourceWorld &world)
{
	REQUIRE(Vehicle::GetNumItems() == 4); REQUIRE(Industry::GetNumItems() == 3); REQUIRE(Station::GetNumItems() == 0); REQUIRE(CargoPacket::GetNumItems() == 0);
	// These two disjoint native mines own separate finite quantities. No
	// station or hold exists to hide a missing reserve debit or duplicated unit.
	for (size_t index = 0; index < world.deposits.size(); ++index) {
		const auto *deposit = GetGeologyWorldState().deposits.Get(world.deposits[index]); REQUIRE(deposit != nullptr);
		const auto *mine = GetLayerMineState(world.industries[index]->index); REQUIRE(mine != nullptr); REQUIRE(mine->deposit == world.deposits[index]);
		REQUIRE(world.industries[index]->produced[0].cargo == world.cargos[index]); const auto units = world.industries[index]->produced[0].waiting;
		REQUIRE(deposit->remaining_reserve + units == deposit->initial_reserve); REQUIRE(mine->extracted == units); REQUIRE(mine->dispatched == 0);
		const auto *front = world.parts[index * 2]; REQUIRE(front->IsChainInDepot()); REQUIRE(front->tile == world.homes[index]); REQUIRE(front->vehstatus.Test(VehState::Stopped)); REQUIRE(IsTrainOnItsTiles(front));
		REQUIRE(world.parts[index * 2 + 1]->First() == front);
	}
	for (const auto *part : world.parts) REQUIRE(part->cargo.StoredCount() == 0);
	// Raw stocks are still undispatched. The real processor cannot obtain
	// inputs, execute a batch or produce RFMT merely because a source exists.
	for (const auto &entry : world.industries[2]->accepted) REQUIRE(entry.waiting == 0);
	for (const auto &entry : world.industries[2]->produced) REQUIRE(entry.waiting == 0);
	const auto *processor = GetLayerProcessorState(world.industries[2]->index); REQUIRE(processor != nullptr); REQUIRE(processor->dispatched == 0);
	for (const auto &[recipe, count] : processor->batches) REQUIRE(count == 0);
	for (const auto &[cargo, count] : processor->consumed) REQUIRE(count == 0);
	for (const auto &[cargo, count] : processor->produced) REQUIRE(count == 0);
	const auto &company = *Company::Get(world.parts[0]->owner); REQUIRE(company.current_loan == Money{0}); REQUIRE(company.money > Money{0}); REQUIRE(company.cur_economy.income == Money{0}); REQUIRE(company.num_valid_stat_ent == 0);
}

/** @param world Exact native raw-source participants. @return Read-only original quantities, claims/work digests, real physical carrier poses, clocks/accounts and both RNGs for save purity; no owner is normalized. */
static auto CaptureForgeRawStock(const ForgeSourceWorld &world)
{
	const auto &company = *Company::Get(world.parts[0]->owner);
	std::vector<uint64_t> values{TimerGameTick::counter, static_cast<uint64_t>(TimerGameCalendar::date.base()), TimerGameCalendar::date_fract,
		static_cast<uint64_t>(TimerGameEconomy::date.base()), TimerGameEconomy::date_fract, static_cast<uint64_t>(company.money.base()), static_cast<uint64_t>(company.cur_economy.income.base()),
		static_cast<uint64_t>(company.cur_economy.expenses.base()), ComputeLayerMineDigest(), ComputeLayerProcessorDigest(), _random.state[0], _random.state[1], _interactive_random.state[0], _interactive_random.state[1]};
	// Both independently generated reserves accompany actual native buffers;
	// paid claims and work/counters remain covered by their original digests.
	for (size_t index = 0; index < world.deposits.size(); ++index) {
		values.push_back(world.deposits[index].base()); values.push_back(GetGeologyWorldState().deposits.Get(world.deposits[index])->remaining_reserve);
	}
	for (const auto *industry : world.industries) {
		values.push_back(industry->index.base()); values.push_back(industry->counter);
		for (const auto &entry : industry->accepted) values.push_back(entry.waiting);
		for (const auto &entry : industry->produced) values.push_back(entry.waiting);
	}
	// Saving a quantity cannot relocate a real front/tail or silently change
	// its stored membership, orders, capacity, cargo meaning or stop condition.
	for (const auto *part : world.parts) {
		for (const uint64_t value : {uint64_t{part->index.base()}, uint64_t{part->First()->index.base()}, uint64_t{part->tile.base()}, static_cast<uint64_t>(part->x_pos), static_cast<uint64_t>(part->y_pos), static_cast<uint64_t>(part->z_pos),
			uint64_t{part->cargo_type}, uint64_t{part->cargo.StoredCount()}, uint64_t{part->cargo_cap}, uint64_t{part->vehstatus.base()}, uint64_t{part->cur_speed}, uint64_t{part->cur_real_order_index}}) values.push_back(value);
	}
	return values;
}

/** @param paths Invocation-owned original raw world. Fund actual original sources and earn both native40-unit checkpoints with the one shared complete source opportunity, without moving a carrier. */
static void EarnForgeRawStock(const TestArtifactPaths &paths)
{
	REQUIRE(TimerGameCalendar::year == TimerGameCalendar::Year{2005}); REQUIRE(TimerGameTick::counter == 0); REQUIRE(Vehicle::GetNumItems() == 0); REQUIRE(Industry::GetNumItems() == 0);
	const auto &company = *Company::Get(_current_company); REQUIRE(company.money == Money{32000000}); REQUIRE(company.current_loan == Money{0}); REQUIRE(company.cur_economy.income == Money{0});
	const auto expected = ShieldSourceValues("OTTD_FORGE_RAW_ORIGINAL", 4); const auto world = BuildForgeRawStockWorld(expected); CheckForgeRawStockConservation(world); uint opportunities = 0;
	// Both real native sources must reach the bounded initial checkpoint. This
	// diagnostic limit never freezes either source or privately caps later work;
	// the same complete clock/front/industry owner advances each exactly once.
	while ((world.industries[0]->produced[0].waiting < 40 || world.industries[1]->produced[0].waiting < 40) && opportunities < 2048) {
		RunShieldSourceOpportunity(); ++opportunities; CheckForgeRawStockConservation(world);
	}
	REQUIRE(opportunities < 2048); REQUIRE(world.industries[0]->produced[0].waiting == 40); REQUIRE(world.industries[1]->produced[0].waiting == 40);
	REQUIRE(TimerGameCalendar::year == TimerGameCalendar::Year{2005});
	const auto before = CaptureForgeRawStock(world); const auto path = paths.Output("saves", "forge-source-raw-stock.sav");
	{
		AutoRestoreBackup mode{_game_mode, GameMode::Menu}; AutoRestoreBackup pause{_pause_mode, PauseModes{PauseMode::Normal}};
		AutoRestoreBackup compression{_savegame_format, std::string{"lzma"}};
		REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(path), false) == SaveLoadResult::Ok);
	}
	CHECK(CaptureForgeRawStock(world) == before); CheckForgeRawStockConservation(world);
	fmt::print("FORGE_STOCK year=2005 tick={} opportunities={} copper=40 coal=40 copper_reserve={} coal_reserve={} cash={} loan=0 income=0 parts={},{},{},{} industries={},{},{} deposits={},{} homes={},{} cargos={},{},{} path={}\n",
		TimerGameTick::counter, opportunities, GetGeologyWorldState().deposits.Get(world.deposits[0])->remaining_reserve, GetGeologyWorldState().deposits.Get(world.deposits[1])->remaining_reserve, company.money.base(),
		world.parts[0]->index.base(), world.parts[1]->index.base(), world.parts[2]->index.base(), world.parts[3]->index.base(), world.industries[0]->index.base(), world.industries[1]->index.base(), world.industries[2]->index.base(),
		world.deposits[0].base(), world.deposits[1].base(), world.homes[0].base(), world.homes[1].base(), world.cargos[0], world.cargos[1], world.cargos[2], path.string());
}

static std::function<void()> *_forge_source_observer = nullptr; ///< Invocation-owned startup callback, never another source or saved quantity owner.

/** @param args Ordinary startup console arguments. @return Console admission; invoke only the current borrowed native observer. */
static bool ObserveForgeSource(std::span<std::string_view> args)
{
	if (args.empty()) return true;
	REQUIRE(_forge_source_observer != nullptr); (*_forge_source_observer)(); return true;
}

/** Normal native loading alone supplies actual2005 cargo/engine/recipe definitions and preserves an empty original author world. */
TEST_CASE("Forge ordinary2005 loading supplies original native source definitions", "[.tube-forge-source-definitions-native]")
{
	const char *input = std::getenv("OTTD_FORGE_SOURCE_INPUT"); REQUIRE(input != nullptr);
	const auto paths = GetTestCaseArtifactPaths("tubes", "forge-source"); const auto config = GetTestRuntimeConfig(); bool observed = false; std::exception_ptr failure;
	/** Inspect only genuine normal-loaded definitions; no registry, purchase, work, clock or account repair runs here. */
	std::function<void()> inspect = [&] {
		try { AutoRestoreBackup actor{_current_company, _local_company}; CheckForgeSourceDefinitions(paths); } catch (...) { failure = std::current_exception(); }
		observed = true; _exit_game = true;
	};
	AutoRestoreBackup observer{_forge_source_observer, &inspect}; IConsole::CmdRegister("forge_source_definitions_check", ObserveForgeSource);
	const std::string config_path = config.string();
	std::array<std::string_view, 17> arguments = {"openttd_test.exe", "-v", "null:ticks=2", "-b", "8bpp-simple", "-s", "null", "-m", "null", "-x", "-X", "-I", "original_dos", "-c", config_path, "-g", input};
	CHECK(openttd_main(arguments) == 0); if (failure) std::rethrow_exception(failure); CHECK(observed);
}

/** Actual native sources earn finite CORE/Coal stocks while all original paid carriers remain whole stopped and the real refinery remains unfed. */
TEST_CASE("Forge original sources earn native finite Copper Ore and Coal stocks", "[.tube-forge-source-stock-native]")
{
	const char *input = std::getenv("OTTD_FORGE_SOURCE_INPUT"); REQUIRE(input != nullptr);
	const auto paths = GetTestCaseArtifactPaths("tubes", "forge-source"); const auto config = GetTestRuntimeConfig(); bool observed = false; std::exception_ptr failure;
	/** Ordinary loading owns all real definitions/prices; actual commands and the shared native dispatcher alone may fund and produce original stock. */
	std::function<void()> inspect = [&] {
		try { AutoRestoreBackup actor{_current_company, _local_company}; AutoRestoreBackup pause{_pause_mode, PauseModes{}}; EarnForgeRawStock(paths); } catch (...) { failure = std::current_exception(); }
		observed = true; _exit_game = true;
	};
	AutoRestoreBackup observer{_forge_source_observer, &inspect}; IConsole::CmdRegister("forge_source_stock_check", ObserveForgeSource);
	const std::string config_path = config.string();
	std::array<std::string_view, 17> arguments = {"openttd_test.exe", "-v", "null:ticks=2", "-b", "8bpp-simple", "-s", "null", "-m", "null", "-x", "-X", "-I", "original_dos", "-c", config_path, "-g", input};
	CHECK(openttd_main(arguments) == 0); if (failure) std::rethrow_exception(failure); CHECK(observed);
}

/** @param world Exact original raw participants. Require original undispatched stocks alongside every native reserve/claim/empty carrier/refinery owner. */
static void CheckForgeOriginalSourceStock(const ForgeSourceWorld &world)
{
	CheckForgeRawStockConservation(world); const auto expected = ShieldSourceValues("OTTD_FORGE_SOURCE_STOCKS", 2);
	for (size_t index = 0; index < world.deposits.size(); ++index) REQUIRE(world.industries[index]->produced[0].waiting == expected[index]);
	REQUIRE(TimerGameCalendar::year == TimerGameCalendar::Year{2005});
}

/** @param world Exact ordinary-loaded original raw source. @param path Invocation-owned native output. Pure-save all canonical original owners and separately preserve unsaved client RNG. */
static void SaveForgeSourceWorld(const ForgeSourceWorld &world, const std::filesystem::path &path)
{
	const auto before = CaptureForgeSourceWorld(world); const std::array interactive{_interactive_random.state[0], _interactive_random.state[1]};
	{
		AutoRestoreBackup mode{_game_mode, GameMode::Menu}; AutoRestoreBackup pause{_pause_mode, PauseModes{PauseMode::Normal}};
		AutoRestoreBackup compression{_savegame_format, std::string{"lzma"}};
		REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(path), false) == SaveLoadResult::Ok);
	}
	CHECK(CaptureForgeSourceWorld(world) == before); CHECK((std::array{_interactive_random.state[0], _interactive_random.state[1]} == interactive));
}

/** @param boundaries Write pure original and untouched full-native next baselines; false performs normal preview/inspect/repeat/continuation from those exact original participants. */
static void ObserveForgeSourceFiles(bool boundaries)
{
	const char *input = std::getenv("OTTD_FORGE_SOURCE_READ_INPUT"); REQUIRE(input != nullptr);
	const char *action_text = std::getenv("OTTD_FORGE_SOURCE_ACTION"), *snapshot_text = std::getenv("OTTD_FORGE_SOURCE_SNAPSHOT");
	const auto paths = GetTestCaseArtifactPaths("tubes", "forge-source"); const auto config = GetTestRuntimeConfig(); bool observed = false; std::exception_ptr failure;
	/** Borrow exact normal-loaded pools; no stock builder, floor restoration, registry, clock, cash or quantity repair may run in file observers. */
	std::function<void()> inspect = [&] {
		try {
			AutoRestoreBackup actor{_current_company, _local_company}; auto world = LoadForgeSourceWorld(); CheckForgeOriginalSourceStock(world);
			if (boundaries) {
				AutoRestoreBackup pause{_pause_mode, PauseModes{}};
				// Boundary1 is precisely the next shared complete clock/front/
				// industry opportunity. Both finite mines advance once, while
				// boundary0 saving adds no work or hidden source/funding credit.
				for (uint step = 0; step < 2; ++step) {
					if (step != 0) { RunShieldSourceOpportunity(); CheckForgeOriginalSourceStock(world); }
					const auto path = paths.Output("saves", fmt::format("forge-source-boundary-{}.sav", step)); SaveForgeSourceWorld(world, path);
					fmt::print("FORGE_SOURCE_WORLD step={} tick={} parts={},{},{},{} references={},{},{},{},{},{},{} snapshot={} path={}\n", step, TimerGameTick::counter,
						world.parts[0]->index.base(), world.parts[1]->index.base(), world.parts[2]->index.base(), world.parts[3]->index.base(),
						world.industries[0]->index.base(), world.industries[1]->index.base(), world.industries[2]->index.base(), world.deposits[0].base(), world.deposits[1].base(), world.homes[0].base(), world.homes[1].base(),
						CaptureForgeSourceWorld(world), path.string());
				}
			} else {
				REQUIRE(action_text != nullptr); REQUIRE(snapshot_text != nullptr); const std::string_view action{action_text}; const std::string expected{snapshot_text};
				// Only an explicit next full opportunity may alter native work;
				// ordinary inspection/preview cannot enter the constructor/writer.
				REQUIRE((action == "inspect" || action == "step")); const auto actual = CaptureForgeSourceWorld(world);
				if (actual != expected) fmt::print("FORGE_SOURCE_SNAPSHOT actual={}\n", actual);
				REQUIRE(actual == expected); const std::array interactive{_interactive_random.state[0], _interactive_random.state[1]};
				REQUIRE(SaveOrLoad(input, SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Ok);
				world = LoadForgeSourceWorld(); REQUIRE(CaptureForgeSourceWorld(world) == expected);
				CHECK((std::array{_interactive_random.state[0], _interactive_random.state[1]} == interactive)); const auto tick = TimerGameTick::counter;
				if (action == "step") {
					AutoRestoreBackup pause{_pause_mode, PauseModes{}}; RunShieldSourceOpportunity(); CheckForgeOriginalSourceStock(world); CHECK(TimerGameTick::counter == tick + 1);
				}
				const auto path = paths.Output("saves", fmt::format("{}-{}.sav", std::filesystem::path{input}.stem().string(), action)); SaveForgeSourceWorld(world, path);
				fmt::print("FORGE_SOURCE_READ action={} original_parts=4 native_industries=3 preview_pure=true full_native_clock=true snapshot={} path={}\n", action, CaptureForgeSourceWorld(world), path.string());
			}
		} catch (...) { failure = std::current_exception(); }
		observed = true; _exit_game = true;
	};
	AutoRestoreBackup observer{_forge_source_observer, &inspect}; IConsole::CmdRegister(boundaries ? "forge_source_boundaries_check" : "forge_source_file_check", ObserveForgeSource);
	const std::string config_path = config.string();
	std::array<std::string_view, 17> arguments = {"openttd_test.exe", "-v", "null:ticks=2", "-b", "8bpp-simple", "-s", "null", "-m", "null", "-x", "-X", "-I", "original_dos", "-c", config_path, "-g", input};
	CHECK(openttd_main(arguments) == 0); if (failure) std::rethrow_exception(failure); CHECK(observed);
}

/** Original ordinary raw-source loading writes pure complete boundaries and exactly the next common native opportunity. */
TEST_CASE("Original Forge raw source writes untouched complete native boundaries", "[.tube-forge-source-boundaries-native]") { ObserveForgeSourceFiles(true); }

/** Original ordinary preview/repeat/complete continuation preserves both real finite source owners without reconstruction. */
TEST_CASE("Original Forge raw source files preserve native owners and complete continuation", "[.tube-forge-source-read-native]") { ObserveForgeSourceFiles(false); }
