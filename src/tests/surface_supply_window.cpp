/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file surface_supply_window.cpp Actual StationView delegation and canonical supply request purity.
 * @see docs/zz-reach2/architecture/gui/archi-gui.md retained native windows.
 * @see docs/zz-reach2/upgrades/2026-10/m2o23-1-come-space.md Groups14B-14C.
 * Declared component worlds exercise original callbacks; no art capture or saved-world acceptance is claimed.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "surface_supply_legacy_fixture.h"
#include "native_cargo_custody_fixture.h"
#include "../widgets/station_widget.h"
#include "../surface_supply_cmd.h"
#include "../command_func.h"
#include "../timer/timer_game_tick.h"
#include "../video/video_driver.hpp"
#include "../safeguards.h"

/** Actual production station acceptance entry point; no replacement receiver. */
extern void OnTick_Station();

/** Original StationView with native screen/language setup and modal cleanup before its world disappears. */
class SupplyWindowScope {
	/** Supply the real headless focus callback owner once for this test process.
	 * @return Existing component company for native window setup.
	 * Quantity editors require VideoDriver's focus callbacks even with an empty
	 * clip. The native null driver is selected only when absent; its loop is
	 * never run, and no game/frame/tick dispatcher is invoked here.
	 */
	static CompanyID PrepareVideo()
	{
		if (VideoDriver::GetInstance() == nullptr) DriverFactoryBase::SelectDriver("null:ticks=0", Driver::Type::Video);
		return _current_company;
	}
	AutoRestoreBackup<CommandPauseLevel> pause_commands{_settings_game.construction.command_pause_level, CommandPauseLevel::NoConstruction}; ///< Explicit UI component policy permits management while keeping graphics/gameplay ticks paused.
	LayerWindowTestUI ui{WindowClass::StationView, PrepareVideo()}; ///< Owns only native presentation lifetime.
public:
	Window *window = nullptr; ///< Actual native parent, never a controller mock or private test installer.
	/** @param station Actual command-created Surface station identity. */
	explicit SupplyWindowScope(StationID station)
	{
		ShowStationViewWindow(station); this->window = FindWindowById(WindowClass::StationView, station);
		REQUIRE(this->window != nullptr); this->window->OnPaint();
	}
	/** Close child queries before the original parent destructor invokes its callbacks. */
	~SupplyWindowScope()
	{
		CloseWindowByClass(WindowClass::DropdownMenu); CloseWindowByClass(WindowClass::QueryString);
		CloseWindowByClass(WindowClass::ErrorMessage); Window::DeleteClosedWindows();
	}
	/** Select one offered history option through the actual parent callbacks.
	 * @param index Temporary option index, never the receipt identity itself.
	 */
	void SelectOrder(int index)
	{
		this->window->OnClick({}, WID_SV_SUPPLY_ORDERS, 1); this->window->OnDropdownSelect(WID_SV_SUPPLY_ORDERS, index, 0);
		CloseWindowByClass(WindowClass::DropdownMenu); Window::DeleteClosedWindows();
	}
	/** Complete one native quantity dialog without accessing controller drafts.
	 * @param text Whole input, including deliberately malformed cases.
	 */
	void Quantity(std::string text)
	{
		this->window->OnClick({}, WID_SV_SUPPLY_QUANTITY, 1); this->window->OnQueryTextFinished(text);
		CloseWindowByClass(WindowClass::QueryString); Window::DeleteClosedWindows();
	}
};

/** The real UI preserves exact high native intents and the user-selected Pending-only refund rule. */
TEST_CASE("Station supply controls post exact uint64 intents and refuse refunds after actual delivery", "[surface-supply-window]")
{
	MockEnvironment::Instance(); LoadGeneratedWindowTestLanguage(); SurfaceSupplyAdmissionRig rig; LegacySupplyComponent history(rig.station);
	const uint64_t id = (uint64_t{1} << 40) + 7; history.Restore({id, {}}); SupplyWindowScope ui(rig.station); auto *window = ui.window;
	const auto cash = Company::Get(_current_company)->money; const bool deliver = GENERATE(false, true);
	CHECK(window->GetWidget<NWidgetStacked>(WID_SV_SUPPLY_OUTER)->shown_plane == 0);
	CHECK(window->GetWidget<NWidgetStacked>(WID_SV_SUPPLY_STACK)->shown_plane == SZSP_NONE);
	window->OnClick({}, WID_SV_SUPPLIES, 1); CHECK(window->GetWidget<NWidgetStacked>(WID_SV_SUPPLY_STACK)->shown_plane == 0);
	window->OnClick({}, WID_SV_SUPPLY_BUY, 1); REQUIRE(GetSurfaceSupplyOrder(id) != nullptr);
	CHECK(GetSurfaceSupplyOrder(id)->Native()->state == SurfaceSupplyOrderState::Pending); CHECK(GetNextSurfaceSupplyOrderID() == id + 1);
	CHECK(Company::Get(_current_company)->money == cash - 100); ui.SelectOrder(0); window->OnPaint();
	CHECK_FALSE(window->IsWidgetDisabled(WID_SV_SUPPLY_CANCEL));
	CHECK(window->GetWidgetString(WID_SV_SUPPLY_ORDERS, INVALID_STRING_ID) == GetString(STR_SURFACE_SUPPLY_ORDER_NUMBER, id,
		GetString(CargoSpec::Get(GetCargoTypeByLabel(CargoLabel{"STEL"}))->name)));
	if (deliver) {
		AutoRestoreBackup counter(TimerGameTick::counter, TimerGameTick::TickCounter{0}); OnTick_Station();
		CHECK(GetSurfaceSupplyOrder(id)->Native()->state == SurfaceSupplyOrderState::Fulfilled);
	}
	CHECK_FALSE(PrepareLegacySurfaceSupplyCancellation(id)); CHECK(CmdCancelLegacySurfaceSupplyOrder(DoCommandFlag::Execute, id, 0).Failed());
	// Invoke the still-enabled pre-delivery button before repaint. Actual
	// command recapture, not a painted enabled state, decides refund permission.
	window->OnClick({}, WID_SV_SUPPLY_CANCEL, 1); window->OnPaint();
	CHECK(GetSurfaceSupplyOrder(id)->Native()->state == (deliver ? SurfaceSupplyOrderState::Fulfilled : SurfaceSupplyOrderState::Cancelled));
	CHECK(Company::Get(_current_company)->money == cash - (deliver ? 100 : 0)); CHECK(window->IsWidgetDisabled(WID_SV_SUPPLY_CANCEL));
	CHECK(GetSurfaceSupplyOrderDayUsage(_current_company, CargoLabel{"STEL"}) == (deliver ? 1 : 0)); CHECK(ValidateSurfaceSupplyOrderBook());
}

/** Rendering and malformed/delayed quantity callbacks never buy a previous valid quantity or rename the station. */
TEST_CASE("Station supply observation and invalid quantity callbacks leave all simulation owners unchanged", "[surface-supply-window]")
{
	MockEnvironment::Instance(); LoadGeneratedWindowTestLanguage(); SurfaceSupplyAdmissionRig rig; LegacySupplyComponent history(rig.station); SupplyWindowScope ui(rig.station);
	auto *window = ui.window; const auto name = Station::Get(rig.station)->name;
	const auto cash = Company::Get(_current_company)->money; const auto book = CaptureSurfaceSupplyOrders(); const auto packets = LegacyComponentPacketFacts();
	const auto random = _random; const auto ticks = TimerGameTick::counter;
	ui.Quantity("3"); window->OnClick({}, WID_SV_SUPPLY_QUANTITY, 1); window->OnClick({}, WID_SV_SUPPLY_BUY, 1);
	CHECK(CaptureSurfaceSupplyOrders() == book); window->OnQueryTextFinished(std::nullopt);
	CloseWindowByClass(WindowClass::QueryString); Window::DeleteClosedWindows();
	const auto invalid = GENERATE(std::string{"0"}, std::string{"65536"}, std::string{"4x"}, std::string{}); ui.Quantity(invalid);
	CHECK(window->GetWidgetString(WID_SV_SUPPLY_QUOTE, INVALID_STRING_ID) == GetString(STR_SURFACE_SUPPLY_BAD_QUANTITY));
	window->OnClick({}, WID_SV_SUPPLY_BUY, 1); // Deliberately no repaint since the invalid callback.
	// Sizing/text/paint and malformed menu options remain observational even
	// when repeated; unrelated parent callbacks must not receive supply indices.
	for (int repeat = 0; repeat < 3; ++repeat) {
		window->ReInit(); window->OnPaint(); window->OnDropdownSelect(WID_SV_SUPPLY_ORDERS, INT_MAX, 0);
		for (auto widget : {WID_SV_SUPPLY_TERMS, WID_SV_SUPPLY_LIMIT, WID_SV_SUPPLY_HISTORY, WID_SV_SUPPLY_CANCEL})
			CHECK_FALSE(window->GetWidgetString(widget, INVALID_STRING_ID).empty());
	}
	CHECK(window->IsWidgetDisabled(WID_SV_SUPPLY_BUY)); CHECK(Station::Get(rig.station)->name == name);
	CHECK(CaptureSurfaceSupplyOrders() == book); CHECK(LegacyComponentPacketFacts() == packets); CHECK(Company::Get(_current_company)->money == cash);
	CHECK(TimerGameTick::counter == ticks); CHECK(_random.state[0] == random.state[0]); CHECK(_random.state[1] == random.state[1]);
	// With no active quantity editor, the original parent still owns renaming.
	window->OnQueryTextFinished(std::string{"Canonical Supply Station"}); CHECK(Station::Get(rig.station)->name == "Canonical Supply Station");
}

/** A menu remembers exact offered IDs across actual retirement; it never reinterprets its option as the next row's identity. */
TEST_CASE("Station supply delayed history selection cannot retarget a pruned high order", "[surface-supply-window]")
{
	MockEnvironment::Instance(); LoadGeneratedWindowTestLanguage(); SurfaceSupplyAdmissionRig rig; LegacySupplyComponent history(rig.station);
	const uint64_t id = (uint64_t{1} << 40) + 19; history.Restore({id, {}}); SupplyWindowScope ui(rig.station); auto *window = ui.window;
	window->OnClick({}, WID_SV_SUPPLY_BUY, 1); REQUIRE(GetSurfaceSupplyOrder(id) != nullptr); window->OnPaint();
	window->OnClick({}, WID_SV_SUPPLY_CANCEL, 1); REQUIRE(GetSurfaceSupplyOrder(id)->Native()->state == SurfaceSupplyOrderState::Cancelled);
	window->OnClick({}, WID_SV_SUPPLY_ORDERS, 1); // Original offered option0 still means the old exact high ID.
	AutoRestoreBackup day(TimerGameEconomy::date, TimerGameEconomy::Date{TimerGameEconomy::date.base() + 1});
	REQUIRE(Command<Commands::PlaceSurfaceSupplyOrder>::Unsafe<CommandCallback>(INVALID_STRING_ID, nullptr, false, false, INVALID_TILE,
		std::make_tuple(rig.station, uint32_t{0x5354454C}, uint16_t{1}, id + 1)).Succeeded());
	REQUIRE(GetSurfaceSupplyOrder(id) == nullptr); const auto book = CaptureSurfaceSupplyOrders(); const auto cash = Company::Get(_current_company)->money;
	window->OnDropdownSelect(WID_SV_SUPPLY_ORDERS, 0, 0); window->OnPaint();
	CHECK(window->GetWidgetString(WID_SV_SUPPLY_ORDERS, INVALID_STRING_ID) == GetString(STR_SURFACE_SUPPLY_SELECT_ORDER));
	CHECK(window->IsWidgetDisabled(WID_SV_SUPPLY_CANCEL)); CHECK(CaptureSurfaceSupplyOrders() == book); CHECK(Company::Get(_current_company)->money == cash);
	CHECK(GetSurfaceSupplyOrder(id + 1)->Native()->state == SurfaceSupplyOrderState::Pending);
}

/** Delayed menus re-resolve real station/player identities, and layout follows loss/recovery without a gameplay action. */
TEST_CASE("Station supply stale selections and visibility cannot borrow foreign ownership", "[surface-supply-window]")
{
	MockEnvironment::Instance(); LoadGeneratedWindowTestLanguage(); SurfaceSupplyAdmissionRig rig; LegacySupplyComponent history(rig.station);
	history.Restore(history.Candidate(10, 10)); SupplyWindowScope ui(rig.station); auto *window = ui.window; ui.SelectOrder(0);
	CHECK(window->GetWidgetString(WID_SV_SUPPLY_HISTORY, INVALID_STRING_ID) == GetString(STR_SURFACE_SUPPLY_HISTORY, 10, 10, 0, GetString(STR_SURFACE_SUPPLY_OPEN)));
	window->OnPaint(); CHECK_FALSE(window->IsWidgetDisabled(WID_SV_SUPPLY_CANCEL));
	CHECK(window->GetWidgetString(WID_SV_SUPPLY_CANCEL, INVALID_STRING_ID) == GetString(STR_SURFACE_SUPPLY_LEGACY_REFUND, 0, Money{0}));
	const auto cash = Company::Get(_current_company)->money; const auto book = CaptureSurfaceSupplyOrders(); const auto packets = LegacyComponentPacketFacts();
	window->OnClick({}, WID_SV_SUPPLY_ORDERS, 1);
	{
		AutoRestoreBackup owner(Station::Get(rig.station)->owner, Owner{OWNER_NONE});
		window->OnDropdownSelect(WID_SV_SUPPLY_ORDERS, 0, 0); window->OnClick({}, WID_SV_SUPPLY_BUY, 1); window->OnPaint();
		CHECK(window->GetWidget<NWidgetStacked>(WID_SV_SUPPLY_OUTER)->shown_plane == SZSP_NONE);
		CHECK(window->IsWidgetDisabled(WID_SV_SUPPLY_BUY)); CHECK(window->IsWidgetDisabled(WID_SV_SUPPLY_CANCEL));
	}
	CloseWindowByClass(WindowClass::DropdownMenu); Window::DeleteClosedWindows(); window->OnPaint();
	CHECK(window->GetWidget<NWidgetStacked>(WID_SV_SUPPLY_OUTER)->shown_plane == 0);
	{
		AutoRestoreBackup position(Station::Get(rig.station)->xy, TileXYL(20, 20, MapLayer::Underground)); window->OnPaint();
		CHECK(window->GetWidget<NWidgetStacked>(WID_SV_SUPPLY_OUTER)->shown_plane == SZSP_NONE);
	}
	window->OnPaint(); CHECK(CaptureSurfaceSupplyOrders() == book); CHECK(LegacyComponentPacketFacts() == packets); CHECK(Company::Get(_current_company)->money == cash);
	// Original launch controls remain present and disabled for this ordinary
	// station with no launch-center authority; supply composition owns neither.
	CHECK(window->IsWidgetDisabled(WID_SV_LAUNCH_ASSEMBLY)); CHECK(window->IsWidgetDisabled(WID_SV_LAUNCH_COLLECTION));
	CHECK(window->GetWidgetString(WID_SV_LAUNCH_HUB, INVALID_STRING_ID) == GetString(STR_LAUNCH_HUB_NONE));
}

/** The rendered old-source refund amount is an exact request; a delayed click cannot silently settle changed physical stock. */
TEST_CASE("Station legacy refund controls reject stale displayed stock and then post the exact new amount", "[surface-supply-window]")
{
	MockEnvironment::Instance(); LoadGeneratedWindowTestLanguage(); NativeCargoCustodyRig rig; LegacySupplyComponent legacy(rig.station);
	auto *station = Station::Get(rig.station); auto &queue = station->goods[GetCargoTypeByLabel(CargoLabel{"RFMT"})].GetOrCreateData().cargo;
	rig.Own(queue, legacy.Packet()); legacy.Restore(legacy.Candidate()); SupplyWindowScope ui(rig.station); auto *window = ui.window; ui.SelectOrder(0); window->OnPaint();
	const auto cash = Company::Get(_current_company)->money; CHECK_FALSE(window->IsWidgetDisabled(WID_SV_SUPPLY_CANCEL));
	CHECK(window->GetWidgetString(WID_SV_SUPPLY_CANCEL, INVALID_STRING_ID) == GetString(STR_SURFACE_SUPPLY_LEGACY_REFUND, 100, Money{2500}));
	REQUIRE(queue.Reserve(40, &rig.train->cargo, {}, station->xy) == 40); const auto changed = CaptureSurfaceSupplyOrders(); const auto facts = LegacyComponentPacketFacts();
	window->OnClick({}, WID_SV_SUPPLY_CANCEL, 1); // No new render after the actual first departure.
	CHECK(CaptureSurfaceSupplyOrders() == changed); CHECK(LegacyComponentPacketFacts() == facts); CHECK(Company::Get(_current_company)->money == cash);
	CHECK(window->GetWidgetString(WID_SV_SUPPLY_CANCEL, INVALID_STRING_ID) == GetString(STR_SURFACE_SUPPLY_LEGACY_REFUND, 60, Money{1500}));
	window->OnClick({}, WID_SV_SUPPLY_CANCEL, 1); CHECK(GetSurfaceSupplyOrder(1)->Legacy()->closed); CHECK(GetSurfaceSupplyOrder(1)->Legacy()->refunded == 60);
	CHECK(queue.AvailableCount() == 0); CHECK(queue.ReservedCount() == 40); CHECK(rig.train->cargo.TotalCount() == 40);
	CHECK(Company::Get(_current_company)->money == cash + 1500); CHECK(GetSurfaceSupplyOrderDayUsage(_current_company, CargoLabel{"RFMT"}) == 100);
	window->OnPaint(); CHECK(window->IsWidgetDisabled(WID_SV_SUPPLY_CANCEL)); CHECK(ValidateSurfaceSupplyOrderBook());
	REQUIRE(rig.train->cargo.Return(40, &queue, StationID::Invalid(), station->xy) == 40); CHECK(queue.CountUnshippedCompanySupply(1) == 0);
	window->OnClick({}, WID_SV_SUPPLY_CANCEL, 1); CHECK(Company::Get(_current_company)->money == cash + 1500); CHECK(queue.TotalCount() == 40);
}
