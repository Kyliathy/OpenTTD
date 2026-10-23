/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file surface_supply_load.cpp Complete paid history/purpose validation and genuine ordinary426 save continuations.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs6-mines-and-resource-chains.md Group13CC.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs15-savegame-determinism-and-lifecycle.md.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "mock_environment.h"
#include "layer_window_fixture.h"
#include "surface_supply_fixture.h"
#include "layer_save_fixture.h"
#include "test_artifact_paths.h"
#include "../hub_cargo.h"
#include "../surface_supply_cmd.h"
#include "../saveload/saveload_func.h"
#include "../console_internal.h"
#include "../openttd.h"
#include "../core/random_func.hpp"
#include "../gfx_func.h"
#include "../table/sprites.h"
#include "../table/strings.h"
#include "../safeguards.h"

/** Complete staged history validates against actual source packets before publication; malformed scalar and physical references preserve live authority. */
TEST_CASE("Surface paid history restoration rejects incomplete and forged purchase rights", "[surface-supply-load]")
{
	MockEnvironment::Instance(); LoadGeneratedWindowTestLanguage();
	SurfaceSupplyRig rig;
	const auto order = PrepareSurfaceSupplyOrder(rig.origin->index, CargoLabel{"RFMT"}, 10);
	REQUIRE(PlaceSurfaceSupplyOrder(order, true) == SurfaceSupplyError::None);
	const auto saved = CaptureSurfaceSupplyState();
	const auto digest = ComputeSurfaceSupplyDigest();
	REQUIRE(ValidateSurfaceSupplyState(saved, true) == SurfaceSupplyError::None);
	StageLoadedSurfaceSupplyState(saved); REQUIRE(AcceptLoadedSurfaceSupplyState());
	CHECK(ComputeSurfaceSupplyDigest() == digest);
	// Each malformed field independently attacks a different retained purchase
	// entitlement. None may publish a fresh cursor, smaller bill or repaired owner.
	for (unsigned fault = 0; fault < 15; ++fault) {
		INFO("fault=" << fault);
		auto changed = saved;
		auto &row = changed.orders[0];
		switch (fault) {
			case 0: changed.next_order = 1; break;
			case 1: row.id = SurfaceSupplyOrderID{2}; break;
			case 2: row.label = CargoLabel{"CHEM"}; break;
			case 3: row.quantity = 0; break;
			case 4: row.quantity = 129; break;
			case 5: row.refunded = 11; break;
			case 6: row.dispatched = 11; break;
			case 7: row.unit_price = 0; break;
			case 8: row.unit_price = Money{INT64_MAX}; break;
			case 9: row.calendar_date = 0; break;
			case 10: row.economy_day = -1; break;
			case 11: row.owner = CompanyID{MAX_COMPANIES}; break;
			case 12: row.station = StationID::Invalid(); break;
			case 13: row.refunded = 1; row.closed = true; break;
			case 14: row.dispatched = 1; break; // Actual ten unshipped units cannot borrow a rewritten nine-unit refund remainder.
		}
		CHECK(ValidateSurfaceSupplyState(changed, true) != SurfaceSupplyError::None);
		StageLoadedSurfaceSupplyState(std::move(changed)); CHECK_FALSE(AcceptLoadedSurfaceSupplyState());
		CHECK(CaptureSurfaceSupplyState().orders == saved.orders);
		CHECK(ComputeSurfaceSupplyDigest() == digest);
	}
	REQUIRE(CargoPacket::CanAllocateItem());
	CargoPacket *orphan = CargoPacket::Create(rig.origin->index, 1, Source{Source::Invalid, SourceType::Industry});
	orphan->SetCompanySupplyOrder(order.next_order.base()); // Explicit malformed real pool fragment, deliberately in no physical owner.
	CHECK(ValidateSurfaceSupplyState(saved, true) == SurfaceSupplyError::Corrupt);
	delete orphan;
	CHECK(ValidateSurfaceSupplyState(saved, true) == SurfaceSupplyError::None);
	CHECK(PrepareSurfaceSupplyOrder(rig.origin->index, CargoLabel{"RFMT"}, 1).day_used == 10);
}

/** Write native stationary physical queues with real registered slots; no mock carrier is serialized as a playable engine. */
TEST_CASE("Surface purchased history and actual source stock write complete native426 saves", "[surface-supply-save-file]")
{
	MockEnvironment::Instance(); LoadGeneratedWindowTestLanguage();
	SurfaceSupplyRig rig(true);
	LayerSaveContent content(rig.ActivatedContent());
	AutoRestoreBackup local{_local_company, _current_company};
	(*Town::Iterate().begin())->townnametype = SPECSTR_TOWNNAME_START;
	// Default station names are needed by ordinary post-load sign rebuilding;
	// the boundary fixture otherwise only needs native station/catchment identities.
	for (Station *station : Station::Iterate()) station->string_id = STR_SV_STNAME;
	for (const auto &[label, quantity] : std::array{
		std::pair{CargoLabel{"STEL"}, uint16_t{10}}, std::pair{CargoLabel{"RFMT"}, uint16_t{10}},
		std::pair{CargoLabel{"ELEC"}, uint16_t{2}}, std::pair{CargoLabel{"BATY"}, uint16_t{3}}}) {
		const auto quote = PrepareSurfaceSupplyOrder(rig.origin->index, label, quantity);
		REQUIRE(PlaceSurfaceSupplyOrder(quote, true) == SurfaceSupplyError::None);
	}
	const auto metal = GetCargoTypeByLabel(CargoLabel{"RFMT"});
	auto &relay = rig.second->goods[metal].GetOrCreateData().cargo;
	REQUIRE(HubCargoTransfer::Move(rig.Queue(CargoLabel{"RFMT"}), relay, 4));
	REQUIRE(HubCargoTransfer::Move(relay, rig.Queue(CargoLabel{"RFMT"}), 2));
	REQUIRE(CancelSurfaceSupplyOrder(PrepareSurfaceSupplyCancellation(SurfaceSupplyOrderID{2}), true) == SurfaceSupplyError::None);
	const auto electronic = GetCargoTypeByLabel(CargoLabel{"ELEC"});
	auto &used = rig.second->goods[electronic].GetOrCreateData().cargo;
	REQUIRE(HubCargoTransfer::Move(rig.Queue(CargoLabel{"ELEC"}), used, 2));
	REQUIRE(HubCargoTransfer::Consume(used, 2)); // Explicit native consumption boundary; actual construction recipient drive remains13CD.
	REQUIRE(CancelSurfaceSupplyOrder(PrepareSurfaceSupplyCancellation(SurfaceSupplyOrderID{3}), true) == SurfaceSupplyError::None);
	REQUIRE(ValidateSurfaceSupplyState(CaptureSurfaceSupplyState(), true) == SurfaceSupplyError::None);
	const auto digest = ComputeSurfaceSupplyDigest();
	const auto paths = GetTestCaseArtifactPaths("surface-supply", "native-save");
	const auto path = paths.Output("saves", "paid-source-stock-426.sav");
	SetMouseCursor(SPR_CURSOR_MOUSE, PAL_NONE);
	AutoRestoreBackup menu{_game_mode, GameMode::Menu};
	AutoRestoreBackup pause{_pause_mode, PauseModes{PauseMode::Normal}};
	AutoRestoreBackup compression{_savegame_format, std::string{"lzma"}};
	REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(path), false) == SaveLoadResult::Ok);
	CHECK(ComputeSurfaceSupplyDigest() == digest);
	fmt::print("SURFACE_SUPPLY_SAVE path={} digest={:016X} orders=4 next=5 fixture_stations=true driven_bootstrap=false\n", path.string(), digest);
}

static std::function<void()> *_surface_supply_file_observer = nullptr; ///< Test observer runs only after ordinary native afterload has accepted every real owner.

/** @param arguments Zero-argument startup command. @return Whether the actual accepted-world observer ran. */
static bool ObserveSurfaceSupplyFile(std::span<std::string_view> arguments)
{
	REQUIRE(arguments.size() == 1); REQUIRE(_surface_supply_file_observer != nullptr);
	(*_surface_supply_file_observer)(); return true;
}

/** Ordinary full/preview readers retain native paid history, refuse corruption, recover valid input and settle a continued cancellation exactly once. */
TEST_CASE("Surface paid history survives actual ordinary loading and independent previews", "[.surface-supply-native-file]")
{
	const char *input = std::getenv("OTTD_SURFACE_SUPPLY_INPUT"); REQUIRE(input != nullptr);
	const char *action = std::getenv("OTTD_SURFACE_SUPPLY_ACTION"); REQUIRE(action != nullptr);
	const std::string_view mode{action};
	const auto paths = GetTestCaseArtifactPaths("surface-supply", "native-file");
	const auto config = GetTestRuntimeConfig();
	bool observed = false; std::exception_ptr failure;
	/** Inspect only genuine accepted authority; preview uses copied file transport and must preserve all live history, stock, money, RNG and clock. */
	std::function<void()> inspect = [&] {
		try {
			AutoRestoreBackup actor{_current_company, _local_company};
			const auto before = ComputeSurfaceSupplyDigest();
			const auto clock = TimerGameTick::counter;
			const auto random = std::array{_random.state[0], _random.state[1]};
			if (mode == "legacy") {
				CHECK(CaptureSurfaceSupplyState().orders.empty()); CHECK(GetNextSurfaceSupplyOrderID().base() == 1);
			} else {
				REQUIRE(CaptureSurfaceSupplyState().orders.size() == 4);
				REQUIRE(ValidateSurfaceSupplyState(CaptureSurfaceSupplyState(), true) == SurfaceSupplyError::None);
				CHECK(GetNextSurfaceSupplyOrderID().base() == 5);
				CHECK(GetSurfaceSupplyOrder(SurfaceSupplyOrderID{2})->refunded == 6);
				CHECK(GetSurfaceSupplyOrder(SurfaceSupplyOrderID{2})->dispatched == 4);
				CHECK(GetSurfaceSupplyOrder(SurfaceSupplyOrderID{3})->dispatched == 2);
				CHECK(GetSurfaceSupplyOrder(SurfaceSupplyOrderID{3})->refunded == 0);
				CHECK(GetSurfaceSupplyOrder(SurfaceSupplyOrderID{3})->closed);
			}
			if (mode == "preview" || mode == "full-refusal" || mode == "native-refusal") {
				const char *bad = std::getenv("OTTD_SURFACE_SUPPLY_CORRUPT"); REQUIRE(bad != nullptr);
				const Money cash = Company::Get(_current_company)->money;
				if (mode == "preview") {
					CHECK(SaveOrLoad(bad, SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Error);
					CHECK(ComputeSurfaceSupplyDigest() == before); CHECK(Company::Get(_current_company)->money == cash);
					CHECK(TimerGameTick::counter == clock); CHECK(std::array{_random.state[0], _random.state[1]} == random);
					REQUIRE(SaveOrLoad(input, SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Ok);
					CHECK(_load_check_data.surface_supply_state.orders.size() == 4); CHECK(ComputeSurfaceSupplyDigest() == before);
				} else {
					if (mode == "native-refusal") REQUIRE(SaveOrLoad(bad, SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Ok);
					REQUIRE(SaveOrLoad(bad, SaveLoadOperation::Load, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::ReInit);
					CHECK(CaptureSurfaceSupplyState().orders.empty());
					REQUIRE(SaveOrLoad(input, SaveLoadOperation::Load, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Ok);
					CHECK(ComputeSurfaceSupplyDigest() == before);
				}
			} else if (mode == "continue") {
				const auto quote = PrepareSurfaceSupplyCancellation(SurfaceSupplyOrderID{1});
				REQUIRE(quote.error == SurfaceSupplyError::None); CHECK(quote.quantity == 10); CHECK(quote.refund == Money{1000});
				const Money cash = Company::Get(_current_company)->money;
				CHECK(CancelSurfaceSupplyOrder(quote, false) == SurfaceSupplyError::None); CHECK(Company::Get(_current_company)->money == cash);
				REQUIRE(CancelSurfaceSupplyOrder(quote, true) == SurfaceSupplyError::None);
				CHECK(Company::Get(_current_company)->money == cash + Money{1000});
				CHECK(CancelSurfaceSupplyOrder(quote, true) == SurfaceSupplyError::Stale);
				CHECK(PrepareSurfaceSupplyOrder(quote.order.station, CargoLabel{"STEL"}, 1).day_used == 10);
			} else if (mode == "continued") {
				CHECK(GetSurfaceSupplyOrder(SurfaceSupplyOrderID{1})->closed);
				CHECK(GetSurfaceSupplyOrder(SurfaceSupplyOrderID{1})->refunded == 10);
				CHECK(PrepareSurfaceSupplyCancellation(SurfaceSupplyOrderID{1}).error == SurfaceSupplyError::Stale);
			}
			REQUIRE(ValidateSurfaceSupplyState(CaptureSurfaceSupplyState(), true) == SurfaceSupplyError::None);
			AutoRestoreBackup pause{_pause_mode, PauseModes{PauseMode::Normal}};
			AutoRestoreBackup compression{_savegame_format, std::string{"lzma"}};
			const auto saved = paths.Output("saves", fmt::format("paid-surface-{}.sav", mode));
			REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(saved), false) == SaveLoadResult::Ok);
			fmt::print("SURFACE_SUPPLY_FILE action={} digest={:016X} path={}\n", mode, ComputeSurfaceSupplyDigest(), saved.string());
		} catch (...) { failure = std::current_exception(); }
		observed = true; _exit_game = true;
	};
	AutoRestoreBackup observer{_surface_supply_file_observer, &inspect};
	IConsole::CmdRegister("surface_supply_check", ObserveSurfaceSupplyFile);
	const std::string config_path = config.string();
	std::array<std::string_view, 17> arguments = {"openttd_test.exe", "-v", "null:ticks=2", "-b", "8bpp-simple", "-s", "null", "-m", "null", "-x", "-X", "-I", "original_dos", "-c", config_path, "-g", input};
	CHECK(openttd_main(arguments) == 0); if (failure) std::rethrow_exception(failure); CHECK(observed);
}
