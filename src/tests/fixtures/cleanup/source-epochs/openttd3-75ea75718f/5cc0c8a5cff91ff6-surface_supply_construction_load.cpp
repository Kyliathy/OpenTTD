/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file surface_supply_construction_load.cpp Ordinary loading continues actual purchased construction trains and native work without repeating invoices or materials.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs6-mines-and-resource-chains.md Group13CD.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs15-savegame-determinism-and-lifecycle.md.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "construction_supply_fixture.h"
#include "../console_internal.h"
#include "../saveload/saveload_func.h"
#include "../openttd.h"

static std::function<void()> *_construction_supply_observer = nullptr; ///< Process-local observer; no saved or runtime gameplay authority.

/** @param arguments Exact startup console invocation. @return Success only after the accepted ordinary world has been inspected. */
static bool ObserveConstructionSupply(std::span<std::string_view> arguments)
{
	REQUIRE(arguments.size() == 1); REQUIRE(_construction_supply_observer != nullptr);
	(*_construction_supply_observer)(); return true;
}

/** Actual ordinary startup accepts original acquired native engines, paid packet provenance and construction work, then continues their real owners. */
TEST_CASE("Paid construction freight survives ordinary unfinished and continued saves", "[.surface-supply-construction-native]")
{
	const char *input = std::getenv("OTTD_CONSTRUCTION_INPUT"); REQUIRE(input != nullptr);
	const char *stage_text = std::getenv("OTTD_CONSTRUCTION_STAGE"); REQUIRE(stage_text != nullptr);
	const char *action_text = std::getenv("OTTD_CONSTRUCTION_ACTION"); REQUIRE(action_text != nullptr);
	const char *cash_text = std::getenv("OTTD_CONSTRUCTION_CASH"); REQUIRE(cash_text != nullptr);
	const std::string_view stage{stage_text}, action{action_text};
	// Refuse a mistyped harness boundary instead of silently accepting an
	// empty stock/work expectation that was never part of the native scenario.
	REQUIRE((stage == "purchased" || stage == "outbound" || stage == "delivered" || stage == "partial" || stage == "complete"));
	REQUIRE((action == "inspect" || action == "continue"));
	const auto paths = GetTestCaseArtifactPaths("surface-supply", "construction");
	const auto config = GetTestRuntimeConfig();
	bool observed = false; std::exception_ptr failure;
	/** Inspect actual accepted native stock/work; continuation advances only the original train, stations and industry dispatcher. */
	std::function<void()> inspect = [&] {
		try {
			AutoRestoreBackup actor{_current_company, _local_company};
			AutoRestoreBackup pause{_pause_mode, PauseModes{}};
			PathfinderDefaults pathfinder;
			int64_t expected_cash = 0;
			const auto parsed = std::from_chars(cash_text, cash_text + strlen(cash_text), expected_cash);
			REQUIRE(parsed.ec == std::errc{}); REQUIRE(parsed.ptr == cash_text + strlen(cash_text));
			const auto collectors = CaptureGeothermalCollectors(); const auto risers = CaptureGeothermalRisers();
			REQUIRE(collectors.size() + risers.size() == 1);
			const bool collector = !collectors.empty();
			const auto id = collector ? collectors.front().industry : risers.front().industry;
			auto &industry = *Industry::Get(id);
			const auto *bill = GetMultiLayerGeothermalConstruction(collector ? MultiLayerGeothermalConstructionKind::Collector : MultiLayerGeothermalConstructionKind::Riser);
			REQUIRE(ValidateSurfaceSupplyState(CaptureSurfaceSupplyState(), true) == SurfaceSupplyError::None);
			REQUIRE(GetSurfaceSupplyOrderHistory().size() == bill->inputs.size());
			Station *origin = Station::Get(GetSurfaceSupplyOrderHistory().front().station);
			Station *destination = nullptr; Train *head = nullptr;
			// These are actual restored native owners, selected by their roles;
			// no fixture reconstructs a train, station, recipient or cargo packet.
			for (Station *station : Station::Iterate()) {
				if (GetTileLayer(station->xy) == MapLayer::DeepUnderground) { REQUIRE(destination == nullptr); destination = station; }
			}
			for (Train *train : Train::Iterate()) {
				if (train->IsFrontEngine()) { REQUIRE(head == nullptr); head = train; }
			}
			REQUIRE(head != nullptr); REQUIRE(destination != nullptr);
			REQUIRE(head->engine_type == GetUndergroundEngine(4));
			CHECK(head->GetNumOrders() == 2); CHECK(GetTileLayer(origin->xy) == MapLayer::Surface);
			CHECK(Company::Get(_current_company)->money == Money{expected_cash});
			const auto history = CaptureSurfaceSupplyState(); const auto digest = ComputeSurfaceSupplyDigest();
			const uint16_t work = collector ? collectors.front().work_ticks : risers.front().work_ticks;
			CHECK(work == (stage == "partial" ? 148 : stage == "complete" ? 296 : 0));
			uint aboard = 0;
			for (const Train *part = head; part != nullptr; part = part->Next()) aboard += part->cargo.TotalCount();
			uint expected_units = 0;
			for (const auto &term : bill->inputs) {
				const auto cargo = GetCargoTypeByLabel(term.label); expected_units += term.quantity;
				CHECK(origin->goods[cargo].TotalCount() == (stage == "purchased" ? term.quantity : 0));
				CHECK(destination->goods[cargo].TotalCount() == 0);
				CHECK(industry.GetCargoAccepted(cargo)->waiting == (stage == "delivered" ? term.quantity : 0));
			}
			CHECK(aboard == (stage == "outbound" ? expected_units : 0));
			for (const auto &row : history.orders) {
				CHECK(row.refunded == 0); CHECK(row.dispatched == (stage == "purchased" ? 0 : row.quantity));
			}
			// Independent preview must not reload, unload, bill, advance work or
			// mutate accepted live state; normal full acceptance has already run.
			const auto clock = TimerGameTick::counter;
			REQUIRE(SaveOrLoad(input, SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Ok);
			CHECK(ComputeSurfaceSupplyDigest() == digest); CHECK(TimerGameTick::counter == clock);
			if (action == "continue") {
				if (stage == "purchased") REQUIRE(CmdStartStopVehicle(DoCommandFlag::Execute, head->index, true).Succeeded());
				// Saved source/outbound packets complete their actual orders through
				// both crossings. Delivered/partial saves retain that completed trip.
				if (stage == "purchased" || stage == "outbound") DriveConstructionSupply(*head, *origin, *destination, destination->index, true);
				for (uint16_t remaining = work; remaining < 296; ++remaining) { ++TimerGameTick::counter; OnTick_Industry(); }
				CHECK((collector ? IsGeothermalCollectorOperational(id) : IsGeothermalRiserOperational(id)));
				CHECK(CargoPacket::GetNumItems() == 0);
				for (const auto &term : bill->inputs) CHECK(industry.GetCargoAccepted(GetCargoTypeByLabel(term.label))->waiting == 0);
				CHECK(Company::Get(_current_company)->money == Money{expected_cash}); CHECK(head->profit_this_year == Money{0});
				CHECK(GetNextSurfaceSupplyOrderID().base() == history.next_order);
				REQUIRE(ValidateSurfaceSupplyState(CaptureSurfaceSupplyState(), true) == SurfaceSupplyError::None);
			}
			AutoRestoreBackup saving_pause{_pause_mode, PauseModes{PauseMode::Normal}};
			AutoRestoreBackup compression{_savegame_format, std::string{"lzma"}};
			const auto saved = paths.Output("saves", fmt::format("{}-{}.sav", std::filesystem::path{input}.stem().string(), action));
			REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(saved), false) == SaveLoadResult::Ok);
			fmt::print("CONSTRUCTION_SUPPLY_NATIVE_SAVE kind={} stage={} action={} cash={} path={}\n", collector ? "collector" : "riser", stage, action, expected_cash, saved.string());
		} catch (...) { failure = std::current_exception(); }
		observed = true; _exit_game = true;
	};
	AutoRestoreBackup observer{_construction_supply_observer, &inspect};
	IConsole::CmdRegister("construction_supply_check", ObserveConstructionSupply);
	const std::string config_path = config.string();
	std::array<std::string_view, 17> arguments = {"openttd_test.exe", "-v", "null:ticks=2", "-b", "8bpp-simple", "-s", "null", "-m", "null", "-x", "-X", "-I", "original_dos", "-c", config_path, "-g", input};
	CHECK(openttd_main(arguments) == 0); if (failure) std::rethrow_exception(failure); CHECK(observed);
}
