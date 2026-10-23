/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file tube_cistern_haul.cpp Actual original Oil/Salt input carriage, native CHEM production and complete empty returns.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Group15DBC.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
 * Ordinary loading retains original source stock; no fixture reconstruction, arrival, order skip or raw/output grant is used.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "layer_window_fixture.h"
#include "layer_save_fixture.h"
#include "cistern_source_world_fixture.h"
#include "tube_useful_tick_fixture.h"
#include "test_artifact_paths.h"
#include "../vehicle_cmd.h"
#include "../company_func.h"
#include "../console_internal.h"
#include "../saveload/saveload_func.h"
#include "../safeguards.h"

static std::function<void()> *_cistern_haul_observer = nullptr; ///< Current isolated ordinary-load observer; never saved or a gameplay owner.

/** @param args Native console arguments. @return Command admission; invoke the registered observer only after ordinary startup accepts all original owners. */
static bool ObserveCisternHaul(std::span<std::string_view> args)
{
	if (args.empty()) return true;
	if (_cistern_haul_observer != nullptr) (*_cistern_haul_observer)();
	return true;
}

/** @param world Exact loaded source. @param expected Original stock/epoch/reserve/cash/loan fields. Verify the initial complete raw handoff before advancing any clocks or starting either front. */
static void CheckOriginalCisternStocks(const CisternSourceWorld &world, const std::vector<uint64_t> &expected)
{
	REQUIRE(TimerGameTick::counter == expected[0]); REQUIRE(TimerGameCalendar::year == TimerGameCalendar::Year{1980});
	REQUIRE(world.stations[0]->goods[world.cargos[0]].TotalCount() == expected[1]);
	REQUIRE(world.stations[2]->goods[world.cargos[1]].TotalCount() == expected[2]);
	REQUIRE(GetGeologyWorldState().deposits.Get(world.deposit)->remaining_reserve == expected[3]);
	const auto &company = *Company::Get(_current_company); REQUIRE(company.money.base() == static_cast<int64_t>(expected[4]));
	REQUIRE(company.current_loan.base() == static_cast<int64_t>(expected[5]));
	for (uint role = 0; role < 2; ++role) {
		REQUIRE(CisternSourceRawTotal(world, role) == expected[role + 1]);
		REQUIRE(CisternSourceAccounts(company, world.cargos[role]) == std::pair{Money{0}, uint64_t{0}});
	}
	for (const auto &slot : world.industries[1]->produced) REQUIRE(slot.waiting == 0);
	const auto count = CisternSourceValues("OTTD_CISTERN_STOCK_PACKET_COUNT", 1)[0]; REQUIRE(CargoPacket::GetNumItems() == count);
	const auto original = CisternSourceValues("OTTD_CISTERN_STOCK_PACKETS", static_cast<size_t>(count) * 6); size_t index = 0;
	// Match every original actual packet and age before work. Equal aggregate
	// stock cannot hide a replacement packet, changed purpose or source station.
	for (const auto *packet : CargoPacket::Iterate()) {
		const std::array<uint64_t, 6> actual{packet->index.base(), packet->Count(), packet->GetSource().id,
			packet->GetFirstStation().base(), packet->GetPeriodsInTransit(), packet->GetCompanySupplyOrder()};
		for (const auto value : actual) REQUIRE(value == original[index++]);
	}
}

/** @param world Exact original loaded participants. Advance one complete native vehicle/clock opportunity and the sole actual CHEM work owner, without additional raw production. */
static void RunCisternSourceHaul(const CisternSourceWorld &world)
{
	RunUsefulTubeNativeOpportunity(); UpdateLandscapingLimits(); AdvanceLayerProcessor(*world.industries[1], 1);
	// Both real fronts and their complete trailers must remain physically on
	// native tiles through the ordinary shaft, loading and empty return. Source
	// observations never repair a pose or isolate one vehicle from its peer.
	for (uint index : {0, 2}) {
		REQUIRE(IsTrainOnItsTiles(world.parts[index])); REQUIRE_FALSE(world.parts[index]->vehstatus.Test(VehState::Crashed));
	}
}

/** @param world Exact original loaded participants. @param raw Original Oil/Salt quantities. @param reserve Accepted finite reserve after stock production. Conserve every raw role, processor output and original provenance through real native loading/unloading. */
static void CheckCisternSourceConservation(const CisternSourceWorld &world, const std::array<uint64_t, 2> &raw, uint64_t reserve)
{
	const auto *processor = GetLayerProcessorState(world.industries[1]->index); REQUIRE(processor != nullptr);
	for (size_t role = 0; role < raw.size(); ++role) {
		CHECK(CisternSourceRawTotal(world, role) == raw[role]);
		CHECK(CisternSourceAccounts(*Company::Get(_current_company), world.cargos[role]).second <= raw[role]);
	}
	CHECK(GetGeologyWorldState().deposits.Get(world.deposit)->remaining_reserve == reserve);
	const auto manufactured = processor->produced.at(CargoLabel{"CHEM"});
	CHECK(processor->consumed.at(CT_OIL) == manufactured); CHECK(processor->consumed.at(CargoLabel{"SALT"}) == manufactured);
	CHECK(processor->dispatched == 0); CHECK(world.industries[1]->produced[0].waiting == manufactured);
	// No output has yet been dispatched for the later Cistern route. Every
	// surviving native packet must retain one original raw source/first station
	// and ordinary purpose, including genuine native loading splits and ages.
	for (const auto *packet : CargoPacket::Iterate()) {
		const bool oil = packet->GetSource() == Source{world.industries[0]->index, SourceType::Industry};
		if (!oil) REQUIRE(packet->GetSource() == Source{world.industries[2]->index, SourceType::Industry});
		CHECK(packet->GetFirstStation() == world.stations[oil ? 0 : 2]->index); CHECK(packet->GetCompanySupplyOrder() == 0);
	}
}

/** @param world Exact original source participants. @param expected Original accepted stock/epoch/reserve/cash/loan observations. @param paths New invocation-owned retained native output. Earn physical raw delivery, actual chemical output and both whole empty returns without skipping orders. */
static void EarnCisternChemicalSource(const CisternSourceWorld &world, const std::vector<uint64_t> &expected, const TestArtifactPaths &paths)
{
	CheckOriginalCisternStocks(world, expected);
	const std::array<uint64_t, 2> raw{expected[1], expected[2]}; const uint64_t reserve = expected[3];
	const std::array homes{world.parts[0]->tile, world.parts[2]->tile};
	const auto initial_income = CisternSourceAccounts(*Company::Get(_current_company), world.cargos[0]).first;
	for (uint index : {0, 2}) REQUIRE(Command<Commands::StartStopVehicle>::Do(DoCommandFlag::Execute, world.parts[index]->index, true).Succeeded());
	std::array<bool, 2> carried{}, returned{}; bool salt_surfaced = false; uint ticks = 0;
	// Each real normal order must earn pickup, same-layer CHEM acceptance,
	// service reversal and complete home storage. Material/account histories
	// are checked each opportunity, never inferred from a destination callback.
	while (ticks++ < 14000) {
		RunCisternSourceHaul(world); CheckCisternSourceConservation(world, raw, reserve);
		for (size_t role = 0; role < raw.size(); ++role) {
			auto *front = world.parts[role * 2], *wagon = world.parts[role * 2 + 1];
			// A loaded native wagon must actually move before it can establish
			// carriage. Merely loading inside a station cannot prove the route.
			if (!carried[role] && wagon->cargo.StoredCount() > 0 && front->cur_speed > 0) {
				carried[role] = true; fmt::print("CISTERN_HAUL_LOADED role={} tick={} units={} tile={}\n", role, TimerGameTick::counter, wagon->cargo.StoredCount(), wagon->tile.base());
			}
			const auto delivered = CisternSourceAccounts(*Company::Get(_current_company), world.cargos[role]).second;
			// Initial home presence and a front stored ahead of an outside tail
			// cannot satisfy return. Stop through the real player command only
			// after paid delivery, empty holds and whole native home storage.
			if (!returned[role] && delivered == raw[role] && front->IsChainInDepot() && front->tile == homes[role] &&
					wagon->cargo.StoredCount() == 0 && front->cargo_payment == nullptr) {
				if (!front->vehstatus.Test(VehState::Stopped)) REQUIRE(Command<Commands::StartStopVehicle>::Do(DoCommandFlag::Execute, front->index, true).Succeeded());
				returned[role] = true; fmt::print("CISTERN_HAUL_RETURN role={} tick={} delivered={}\n", role, TimerGameTick::counter, delivered);
			}
		}
		// Only the Salt wagon physically crosses from its original Underground
		// source. Same-layer station joining or a loaded Surface front cannot
		// impersonate passage of this actual loaded trailer through the shaft.
		if (world.parts[3]->cargo.StoredCount() > 0 && GetTileLayer(world.parts[3]->tile) == MapLayer::Surface) salt_surfaced = true;
		if (std::ranges::all_of(returned, std::identity{})) break;
	}
	INFO(DescribeTrain(world.parts[0])); INFO(DescribeTrain(world.parts[2]));
	REQUIRE(ticks < 14000); REQUIRE(std::ranges::all_of(carried, std::identity{})); REQUIRE(salt_surfaced);
	REQUIRE(std::ranges::all_of(returned, std::identity{}));
	const auto *processor = GetLayerProcessorState(world.industries[1]->index); REQUIRE(processor != nullptr);
	REQUIRE(processor->produced.at(CargoLabel{"CHEM"}) == 20); REQUIRE(CisternSourceInput(*world.industries[1], world.cargos[0]) == raw[0] - 20);
	REQUIRE(CisternSourceInput(*world.industries[1], world.cargos[1]) == 0);
	const auto accounts = std::array{CisternSourceAccounts(*Company::Get(_current_company), world.cargos[0]), CisternSourceAccounts(*Company::Get(_current_company), world.cargos[1])};
	REQUIRE(accounts[0].first > initial_income); REQUIRE(CargoPacket::GetNumItems() == 0);
	// Two further native economy days cannot replay a raw delivery, payment or
	// consumed recipe batch. Both original empty consists stay fully stored.
	for (uint empty_tick = 0; empty_tick < 148; ++empty_tick) {
		RunCisternSourceHaul(world); CheckCisternSourceConservation(world, raw, reserve);
		for (uint index : {0, 2}) { REQUIRE(world.parts[index]->IsChainInDepot()); REQUIRE(world.parts[index]->vehstatus.Test(VehState::Stopped)); }
	}
	for (size_t role = 0; role < raw.size(); ++role) CHECK(CisternSourceAccounts(*Company::Get(_current_company), world.cargos[role]) == accounts[role]);
	REQUIRE(processor->produced.at(CargoLabel{"CHEM"}) == 20); REQUIRE(CargoPacket::GetNumItems() == 0);
	const auto path = paths.Output("saves", "cistern-chemical-source.sav");
	const auto before = std::tuple{Company::Get(_current_company)->money, TimerGameTick::counter, ComputeLayerProcessorDigest(),
		std::array{_random.state[0], _random.state[1], _interactive_random.state[0], _interactive_random.state[1]}};
	AutoRestoreBackup mode{_game_mode, GameMode::Menu}; AutoRestoreBackup pause{_pause_mode, PauseModes{PauseMode::Normal}};
	AutoRestoreBackup compression{_savegame_format, std::string{"lzma"}};
	REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(path), false) == SaveLoadResult::Ok);
	CHECK((before == std::tuple{Company::Get(_current_company)->money, TimerGameTick::counter, ComputeLayerProcessorDigest(),
		std::array{_random.state[0], _random.state[1], _interactive_random.state[0], _interactive_random.state[1]}}));
	fmt::print("CISTERN_CHEMICAL_SOURCE year={} original_tick={} tick={} route_ticks={} oil={} salt={} chemicals={} unused_oil={} unused_salt={} returns=2 empty_ticks=148 surfaced=true income={} cash={} loan={} quarter_history={} reserve={} parts={},{},{},{} industries={},{},{} stations={},{},{} path={}\n",
		TimerGameCalendar::year.base(), expected[0], TimerGameTick::counter, ticks, accounts[0].second, accounts[1].second,
		processor->produced.at(CargoLabel{"CHEM"}), CisternSourceInput(*world.industries[1], world.cargos[0]), CisternSourceInput(*world.industries[1], world.cargos[1]),
		(accounts[0].first - initial_income).base(), Company::Get(_current_company)->money.base(), Company::Get(_current_company)->current_loan.base(), Company::Get(_current_company)->num_valid_stat_ent, reserve,
		world.parts[0]->index.base(), world.parts[1]->index.base(), world.parts[2]->index.base(), world.parts[3]->index.base(),
		world.industries[0]->index.base(), world.industries[1]->index.base(), world.industries[2]->index.base(),
		world.stations[0]->index.base(), world.stations[1]->index.base(), world.stations[2]->index.base(), path.string());
}

/** Real original raw packets must physically reach native CHEM before its useful chemical output can serve Cistern. */
TEST_CASE("Original Cistern Oil and Salt shipments earn actual chemical output and whole returns", "[.tube-cistern-source-haul-native]")
{
	const char *input = std::getenv("OTTD_CISTERN_SOURCE_INPUT"); REQUIRE(input != nullptr);
	const auto paths = GetTestCaseArtifactPaths("tubes", "cistern-source"); const auto config = GetTestRuntimeConfig();
	bool observed = false; std::exception_ptr failure;
	/** The normal application loader must restore the exact stopped stock world before either original front starts. */
	std::function<void()> inspect = [&] {
		try {
			AutoRestoreBackup actor{_current_company, _local_company}; AutoRestoreBackup pause{_pause_mode, PauseModes{}};
			const auto world = LoadCisternSourceWorld(); const auto expected = CisternSourceValues("OTTD_CISTERN_STOCK_VALUES", 6);
			EarnCisternChemicalSource(world, expected, paths);
		} catch (...) { failure = std::current_exception(); }
		observed = true; _exit_game = true;
	};
	AutoRestoreBackup observer{_cistern_haul_observer, &inspect}; IConsole::CmdRegister("cistern_haul_check", ObserveCisternHaul);
	const std::string config_path = config.string();
	std::array<std::string_view, 17> arguments = {"openttd_test.exe", "-v", "null:ticks=2", "-b", "8bpp-simple", "-s", "null", "-m", "null", "-x", "-X", "-I", "original_dos", "-c", config_path, "-g", input};
	CHECK(openttd_main(arguments) == 0); if (failure) std::rethrow_exception(failure); CHECK(observed);
}

/** @param path New invocation-owned native output. Preserve the complete loaded source without a timer, participant, inventory, account or RNG adjustment. */
static void SaveCisternReadyWorld(const std::filesystem::path &path)
{
	const auto before = std::tuple{Company::Get(_current_company)->money, TimerGameTick::counter, ComputeLayerProcessorDigest(),
		std::array{_random.state[0], _random.state[1], _interactive_random.state[0], _interactive_random.state[1]}};
	AutoRestoreBackup mode{_game_mode, GameMode::Menu}; AutoRestoreBackup pause{_pause_mode, PauseModes{PauseMode::Normal}};
	AutoRestoreBackup compression{_savegame_format, std::string{"lzma"}};
	REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(path), false) == SaveLoadResult::Ok);
	CHECK((before == std::tuple{Company::Get(_current_company)->money, TimerGameTick::counter, ComputeLayerProcessorDigest(),
		std::array{_random.state[0], _random.state[1], _interactive_random.state[0], _interactive_random.state[1]}}));
}

/** @param world Exact original useful source participants. @param expected Original final source observations. @param step Zero for untouched reload, one for a complete native opportunity. Check loaded histories/material/whole storage without replaying any source work. */
static void CheckCisternReadyWorld(const CisternSourceWorld &world, const std::vector<uint64_t> &expected, uint step)
{
	REQUIRE(TimerGameTick::counter == expected[0] + step); REQUIRE(TimerGameCalendar::year == TimerGameCalendar::Year{1980});
	const std::array<uint64_t, 2> raw{expected[1], expected[2]}; CheckCisternSourceConservation(world, raw, expected[9]);
	const auto &company = *Company::Get(_current_company);
	const auto *processor = GetLayerProcessorState(world.industries[1]->index); REQUIRE(processor != nullptr);
	REQUIRE(processor->produced.at(CargoLabel{"CHEM"}) == expected[3]);
	REQUIRE(CisternSourceInput(*world.industries[1], world.cargos[0]) == expected[4]);
	REQUIRE(CisternSourceInput(*world.industries[1], world.cargos[1]) == expected[5]);
	REQUIRE(CargoPacket::GetNumItems() == 0); REQUIRE(company.current_loan.base() == static_cast<int64_t>(expected[8]));
	REQUIRE(company.num_valid_stat_ent == expected[10]);
	// Pure reload preserves exact cash. A real following opportunity may debit
	// ordinary running costs, but cannot gain another freight payment or erase
	// the original Oil/Salt receipts across current and historical quarters.
	if (step == 0) REQUIRE(company.money.base() == static_cast<int64_t>(expected[7]));
	else REQUIRE(company.money.base() <= static_cast<int64_t>(expected[7]));
	for (size_t role = 0; role < raw.size(); ++role) {
		const auto accounts = CisternSourceAccounts(company, world.cargos[role]); REQUIRE(accounts.first.base() == static_cast<int64_t>(expected[6]));
		REQUIRE(accounts.second == raw[role]);
		auto *front = world.parts[role * 2], *wagon = world.parts[role * 2 + 1];
		REQUIRE(front->IsChainInDepot()); REQUIRE(front->vehstatus.Test(VehState::Stopped)); REQUIRE(wagon->cargo.StoredCount() == 0);
	}
}

/** Original chemical output, unused Oil and complete empty trains survive ordinary loading and a genuine following native opportunity. */
TEST_CASE("Cistern ready source preserves actual native owners after ordinary loading", "[.tube-cistern-source-ready-native]")
{
	const char *input = std::getenv("OTTD_CISTERN_SOURCE_INPUT"); REQUIRE(input != nullptr);
	const auto paths = GetTestCaseArtifactPaths("tubes", "cistern-source"); const auto config = GetTestRuntimeConfig();
	bool observed = false; std::exception_ptr failure;
	/** Inspect only original ordinary-loaded native owners; the optional next opportunity shares the actual source driver's code. */
	std::function<void()> inspect = [&] {
		try {
			AutoRestoreBackup actor{_current_company, _local_company}; AutoRestoreBackup pause{_pause_mode, PauseModes{}};
			const auto world = LoadCisternSourceWorld(); const auto expected = CisternSourceValues("OTTD_CISTERN_READY_VALUES", 11);
			const auto step = CisternSourceValues("OTTD_CISTERN_READY_STEP", 1)[0]; REQUIRE(step <= 1);
			CheckCisternReadyWorld(world, expected, 0);
			const auto before = paths.Output("saves", fmt::format("cistern-ready-{}-before.sav", step)); SaveCisternReadyWorld(before);
			if (step != 0) RunCisternSourceHaul(world);
			CheckCisternReadyWorld(world, expected, static_cast<uint>(step));
			const auto first = paths.Output("saves", fmt::format("cistern-ready-{}-first.sav", step)), repeat = paths.Output("saves", fmt::format("cistern-ready-{}-repeat.sav", step));
			SaveCisternReadyWorld(first); SaveCisternReadyWorld(repeat);
			// Observe the five real EIDS definitions after normal saved-engine
			// restoration. The file verifier uses these exact native identities
			// to bound the old randomized publication metadata compatibility change.
			for (uint16_t model = 9; model <= 13; ++model) {
				const auto *engine = Engine::Get(GetTubeEngine(model)); REQUIRE(engine != nullptr);
				fmt::print("CISTERN_CAPSULE_DATE model={} engine={} intro_date={} flags={} company_avail={}\n", model, engine->index.base(), engine->intro_date.base(), engine->flags.base(), engine->company_avail.base());
			}
			const auto *processor = GetLayerProcessorState(world.industries[1]->index); REQUIRE(processor != nullptr);
			fmt::print("CISTERN_READY step={} tick={} chemicals={} unused_oil={} unused_salt={} income={} cash={} loan={} reserve={} work={} revision={} digest={} before={} first={} repeat={}\n",
				step, TimerGameTick::counter, processor->produced.at(CargoLabel{"CHEM"}), CisternSourceInput(*world.industries[1], world.cargos[0]), CisternSourceInput(*world.industries[1], world.cargos[1]),
				CisternSourceAccounts(*Company::Get(_current_company), world.cargos[0]).first.base(), Company::Get(_current_company)->money.base(), Company::Get(_current_company)->current_loan.base(),
				GetGeologyWorldState().deposits.Get(world.deposit)->remaining_reserve, processor->work_ticks, processor->revision, ComputeLayerProcessorDigest(), before.string(), first.string(), repeat.string());
		} catch (...) { failure = std::current_exception(); }
		observed = true; _exit_game = true;
	};
	AutoRestoreBackup observer{_cistern_haul_observer, &inspect}; IConsole::CmdRegister("cistern_ready_check", ObserveCisternHaul);
	const std::string config_path = config.string();
	std::array<std::string_view, 17> arguments = {"openttd_test.exe", "-v", "null:ticks=2", "-b", "8bpp-simple", "-s", "null", "-m", "null", "-x", "-X", "-I", "original_dos", "-c", config_path, "-g", input};
	CHECK(openttd_main(arguments) == 0); if (failure) std::rethrow_exception(failure); CHECK(observed);
}
