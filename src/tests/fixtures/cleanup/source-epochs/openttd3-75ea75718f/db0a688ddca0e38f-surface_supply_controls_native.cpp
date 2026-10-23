/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file surface_supply_controls_native.cpp Render actual loaded station supply widgets without injecting stock, purchases or simulation ticks.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs6-mines-and-resource-chains.md Group13CCB.
 * @see docs/zz-reach2/architecture/gui/archi-gui.md.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "test_artifact_paths.h"
#include "native_art_render_fixture.h"
#include "../surface_supply.h"
#include "../station_func.h"
#include "../station_base.h"
#include "../cargotype.h"
#include "../company_base.h"
#include "../company_func.h"
#include "../openttd.h"
#include "../console_internal.h"
#include "../core/random_func.hpp"
#include "../blitter/factory.hpp"
#include "../spritecache.h"
#include "../fontcache.h"
#include "../widgets/station_widget.h"
#include "../table/strings.h"
#include "../safeguards.h"

static std::function<void()> *_surface_supply_ui_observer = nullptr; ///< Isolated accepted-world observer; production widgets retain their ordinary event path.

/** @param arguments Zero-argument startup hook. @return Whether the fully loaded native widget observer executed. */
static bool ObserveSurfaceSupplyUI(std::span<std::string_view> arguments)
{
	REQUIRE(arguments.size() == 1); REQUIRE(_surface_supply_ui_observer != nullptr);
	(*_surface_supply_ui_observer)(); return true;
}

/** Real native raster and callbacks display original paid queues/history; this is stationary-stock UI verification, not driven construction acceptance. */
TEST_CASE("Loaded station supply controls render real prices and history without gameplay mutation", "[.surface-supply-controls-native]")
{
	const char *input = std::getenv("OTTD_SURFACE_SUPPLY_INPUT"); REQUIRE(input != nullptr);
	const auto paths = GetTestCaseArtifactPaths("surface-supply", "controls-native");
	const auto config = GetTestRuntimeConfig();
	bool observed = false; std::exception_ptr failure;
	/** Draw only actual loaded native source widgets; camera/layout edits must preserve every paid quantity, identity, bank balance, tick and RNG value. */
	std::function<void()> inspect = [&] {
		try {
			AutoRestoreBackup actor{_current_company, _local_company};
			const auto *first = GetSurfaceSupplyOrder(SurfaceSupplyOrderID{1}); REQUIRE(first != nullptr);
			const auto station = first->station;
			const auto before = ComputeSurfaceSupplyDigest(); const Money cash = Company::Get(_local_company)->money;
			const auto packet_count = CargoPacket::GetNumItems();
			std::array<uint, NUM_CARGO> waiting{};
			// Capture every real source queue, including ordinary freight, so a
			// viewer cannot accidentally append non-paying-purpose-free stock unnoticed.
			for (CargoType cargo : EnumRange(NUM_CARGO)) waiting[cargo] = Station::Get(station)->goods[cargo].TotalCount();
			const auto tick = TimerGameTick::counter; const auto random = std::array{_random.state[0], _random.state[1]};
			REQUIRE(BlitterFactory::SelectBlitter("8bpp-simple") != nullptr);
			GfxClearSpriteCache(); FontCache::ClearFontCaches(FONTSIZES_REQUIRED);
			ShowStationViewWindow(station);
			Window *window = FindWindowById(WindowClass::StationView, station); REQUIRE(window != nullptr);
			// Headless video has no live pixel target. Every actual paint must
			// run inside the native capture canvas, including its first refresh.
			CaptureNativeArtWindow(*window, paths.images / "paid-surface-collapsed.ppm");
			window->OnClick({}, WID_SV_SUPPLIES, 1);
			CaptureNativeArtWindow(*window, paths.images / "paid-surface-controls.ppm");
			CHECK_FALSE(window->IsWidgetDisabled(WID_SV_SUPPLY_BUY));
			window->OnDropdownSelect(WID_SV_SUPPLY_ORDERS, 2, 0);
			CaptureNativeArtWindow(*window, paths.images / "paid-surface-refunded-order.ppm");
			CHECK(window->IsWidgetDisabled(WID_SV_SUPPLY_CANCEL));
			CHECK(window->GetWidgetString(WID_SV_SUPPLY_HISTORY, STR_SURFACE_SUPPLIES).find("refunded 6") != std::string::npos);
			CHECK(ComputeSurfaceSupplyDigest() == before); CHECK(Company::Get(_local_company)->money == cash);
			CHECK(CargoPacket::GetNumItems() == packet_count);
			for (CargoType cargo : EnumRange(NUM_CARGO)) CHECK(Station::Get(station)->goods[cargo].TotalCount() == waiting[cargo]);
			CHECK(TimerGameTick::counter == tick); CHECK(std::array{_random.state[0], _random.state[1]} == random);
			fmt::print("SURFACE_SUPPLY_CONTROLS native_raster=true paid_history_unchanged=true actor={} steel_waiting={} packets={}\n",
				_local_company.base(), waiting[GetCargoTypeByLabel(CargoLabel{"STEL"})], packet_count);
		} catch (...) { failure = std::current_exception(); }
		observed = true; _exit_game = true;
	};
	AutoRestoreBackup observer{_surface_supply_ui_observer, &inspect};
	IConsole::CmdRegister("surface_supply_ui_check", ObserveSurfaceSupplyUI);
	const std::string config_path = config.string();
	std::array<std::string_view, 17> arguments = {"openttd_test.exe", "-v", "null:ticks=2", "-b", "8bpp-simple", "-s", "null", "-m", "null", "-x", "-X", "-I", "original_dos", "-c", config_path, "-g", input};
	CHECK(openttd_main(arguments) == 0); if (failure) std::rethrow_exception(failure); CHECK(observed);
}
