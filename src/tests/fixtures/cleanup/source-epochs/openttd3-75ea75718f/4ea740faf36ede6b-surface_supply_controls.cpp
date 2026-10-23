/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file surface_supply_controls.cpp Actual station foldout/query/command flow; fixture station setup is not driven construction acceptance.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs6-mines-and-resource-chains.md Group13CCB.
 * @see docs/zz-reach2/architecture/gui/archi-gui.md.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "mock_environment.h"
#include "layer_window_fixture.h"
#include "surface_supply_fixture.h"
#include "../station_gui.h"
#include "../station_func.h"
#include "../station_base.h"
#include "../widgets/station_widget.h"
#include "../core/random_func.hpp"
#include "../video/video_driver.hpp"
#include "../table/strings.h"
#include "../safeguards.h"

/** Close actual editor/dropdown children while their native station and parent window still exist. */
class SurfaceSupplyTestUI : public LayerWindowTestUI {
public:
	/** @param owner Actual local player whose native station window is being tested. */
	explicit SurfaceSupplyTestUI(CompanyID owner) : LayerWindowTestUI(WindowClass::StationView, owner)
	{
		// Standalone query widgets call the ordinary driver's input-focus hook.
		// Install the native null driver when this isolated process has none;
		// starting that interface does not run its game loop or advance work.
		if (VideoDriver::GetInstance() == nullptr) DriverFactoryBase::SelectDriver("null", Driver::Type::Video);
	}
	/** Retire transient native editors before parent station-window fixture teardown. */
	~SurfaceSupplyTestUI()
	{
		CloseWindowByClass(WindowClass::QueryString);
		CloseWindowByClass(WindowClass::DropdownMenu);
		Window::DeleteClosedWindows();
	}
};

/** Ordinary source inspection, repeated paints and reopening cannot buy stock, reserve a cap or advance work/RNG. */
TEST_CASE("Station supply foldout queries canonical tariffs without simulation mutation", "[surface-supply-controls]")
{
	MockEnvironment::Instance(); LoadGeneratedWindowTestLanguage();
	SurfaceSupplyRig rig;
	SurfaceSupplyTestUI ui(_current_company);
	const Money cash = Company::Get(_current_company)->money;
	const auto digest = ComputeSurfaceSupplyDigest();
	const auto random = std::array{_random.state[0], _random.state[1]};
	const auto tick = TimerGameTick::counter;
	CHECK(GetSurfaceSupplyTerms(CargoLabel{"CHEM"}) == nullptr);
	CHECK_FALSE(GetSurfaceSupplyUnitPrice(CargoLabel{"CHEM"}).has_value());
	REQUIRE(GetSurfaceSupplyUnitPrice(CargoLabel{"STEL"}).has_value());
	CHECK(*GetSurfaceSupplyUnitPrice(CargoLabel{"STEL"}) == Money{100});
	ShowStationViewWindow(rig.origin->index);
	Window *window = FindWindowById(WindowClass::StationView, rig.origin->index); REQUIRE(window != nullptr);
	window->OnPaint();
	const auto height = window->height;
	window->OnClick({}, WID_SV_SUPPLIES, 1); window->OnPaint();
	CHECK(window->height > height); CHECK_FALSE(window->IsWidgetDisabled(WID_SV_SUPPLY_BUY));
	CHECK(window->GetWidgetString(WID_SV_SUPPLY_TERMS, STR_SURFACE_SUPPLIES).find("1950") != std::string::npos);
	CHECK(window->GetWidgetString(WID_SV_SUPPLY_LIMIT, STR_SURFACE_SUPPLIES).find("256") != std::string::npos);
	{
		AutoRestoreBackup actor{_current_company, OWNER_DEITY};
		window->OnPaint();
		CHECK(_current_company == OWNER_DEITY);
	}
	window->OnClick({}, WID_SV_SUPPLIES, 1); window->OnPaint();
	CloseWindowById(WindowClass::StationView, rig.origin->index); Window::DeleteClosedWindows();
	ShowStationViewWindow(rig.origin->index);
	window = FindWindowById(WindowClass::StationView, rig.origin->index); REQUIRE(window != nullptr); window->OnPaint();
	CHECK(Company::Get(_current_company)->money == cash); CHECK(ComputeSurfaceSupplyDigest() == digest);
	CHECK(CargoPacket::GetNumItems() == 0); CHECK(TimerGameTick::counter == tick);
	CHECK(std::array{_random.state[0], _random.state[1]} == random);
}

/** Actual native quantity/cargo callbacks buy once; native loading permanently excludes shipped units from the displayed frozen refund. */
TEST_CASE("Station supply controls post real purchases and exact remaining cancellation", "[surface-supply-controls-order]")
{
	MockEnvironment::Instance(); LoadGeneratedWindowTestLanguage();
	SurfaceSupplyRig rig;
	SurfaceSupplyTestUI ui(_current_company);
	ShowStationViewWindow(rig.origin->index);
	Window *window = FindWindowById(WindowClass::StationView, rig.origin->index); REQUIRE(window != nullptr);
	window->OnPaint(); window->OnClick({}, WID_SV_SUPPLIES, 1);
	window->OnDropdownSelect(WID_SV_SUPPLY_CARGO, 1, 0); // Canonical RFMT menu choice; no stock exists until the native command settles.
	const auto name = rig.origin->name;
	window->OnClick({}, WID_SV_RENAME, 1); // A preceding ordinary editor must finish before the quantity callback is marked as ours.
	window->OnClick({}, WID_SV_SUPPLY_QUANTITY, 1);
	window->OnQueryTextFinished(std::string{"6"}); window->OnPaint();
	CHECK(rig.origin->name == name);
	const Money before = Company::Get(_current_company)->money;
	const auto cursor = GetNextSurfaceSupplyOrderID();
	CHECK(rig.Queue(CargoLabel{"RFMT"}).AvailableCount() == 0);
	window->OnClick({}, WID_SV_SUPPLY_BUY, 1); window->OnPaint();
	REQUIRE(GetSurfaceSupplyOrder(cursor) != nullptr);
	CHECK(GetSurfaceSupplyOrder(cursor)->quantity == 6);
	CHECK(Company::Get(_current_company)->money == before - Money{1080});
	CHECK(rig.Queue(CargoLabel{"RFMT"}).AvailableCount() == 6);
	rig.Load(CargoLabel{"RFMT"}, 2); // Explicit actual loading boundary, not a driven collector/riser delivery claim.
	window->OnPaint();
	CHECK(window->GetWidgetString(WID_SV_SUPPLY_CANCEL, STR_SURFACE_SUPPLIES).find("4 units") != std::string::npos);
	CHECK_FALSE(window->IsWidgetDisabled(WID_SV_SUPPLY_CANCEL));
	window->OnClick({}, WID_SV_SUPPLY_CANCEL, 1); window->OnPaint();
	CHECK(GetSurfaceSupplyOrder(cursor)->closed); CHECK(GetSurfaceSupplyOrder(cursor)->refunded == 4);
	CHECK(GetSurfaceSupplyOrder(cursor)->dispatched == 2);
	CHECK(Company::Get(_current_company)->money == before - Money{360});
	CHECK(rig.Queue(CargoLabel{"RFMT"}).AvailableCount() == 0); CHECK(rig.carrier->cargo.TotalCount() == 2);
	CHECK(window->IsWidgetDisabled(WID_SV_SUPPLY_CANCEL));
	window->OnClick({}, WID_SV_SUPPLY_CANCEL, 1);
	CHECK(Company::Get(_current_company)->money == before - Money{360});
	CHECK(PrepareSurfaceSupplyOrder(rig.origin->index, CargoLabel{"RFMT"}, 1).day_used == 6);
}

/** Native calendar/funds/cap admission and complete editor parsing prevent an unusable request from silently settling another quantity. */
TEST_CASE("Station supply controls refuse date and malformed whole requests unchanged", "[surface-supply-controls-refusal]")
{
	MockEnvironment::Instance(); LoadGeneratedWindowTestLanguage();
	SurfaceSupplyRig rig;
	SurfaceSupplyTestUI ui(_current_company);
	ShowStationViewWindow(rig.origin->index);
	Window *window = FindWindowById(WindowClass::StationView, rig.origin->index); REQUIRE(window != nullptr);
	window->OnPaint(); window->OnClick({}, WID_SV_SUPPLIES, 1);
	window->OnDropdownSelect(WID_SV_SUPPLY_CARGO, 2, 0);
	TimerGameCalendar::date = TimerGameCalendar::ConvertYMDToDate(TimerGameCalendar::Year{1964}, 0, 1);
	window->OnPaint(); CHECK(window->IsWidgetDisabled(WID_SV_SUPPLY_BUY));
	CHECK(window->GetWidgetString(WID_SV_SUPPLY_TERMS, STR_SURFACE_SUPPLIES).find("1965") != std::string::npos);
	TimerGameCalendar::date = TimerGameCalendar::ConvertYMDToDate(TimerGameCalendar::Year{1965}, 0, 1);
	window->OnPaint(); CHECK_FALSE(window->IsWidgetDisabled(WID_SV_SUPPLY_BUY));
	const auto digest = ComputeSurfaceSupplyDigest(); const Money cash = Company::Get(_current_company)->money;
	// Each malformed complete input must remain a failed request; accepting a
	// numeric prefix or wrapping to a different amount would buy unintended stock.
	for (const std::string text : {"0", "65536", "2junk"}) {
		window->OnClick({}, WID_SV_SUPPLY_QUANTITY, 1); window->OnQueryTextFinished(text); window->OnPaint();
		CHECK(window->IsWidgetDisabled(WID_SV_SUPPLY_BUY));
		window->OnClick({}, WID_SV_SUPPLY_BUY, 1);
		CHECK(ComputeSurfaceSupplyDigest() == digest); CHECK(Company::Get(_current_company)->money == cash);
	}
	window->OnClick({}, WID_SV_SUPPLY_QUANTITY, 1); window->OnQueryTextFinished(std::string{"2"}); window->OnPaint();
	CHECK_FALSE(window->IsWidgetDisabled(WID_SV_SUPPLY_BUY));
	Company::Get(_current_company)->money = 999;
	window->OnPaint(); CHECK(window->IsWidgetDisabled(WID_SV_SUPPLY_BUY));
	CHECK(GetNextSurfaceSupplyOrderID().base() == 1); CHECK(CargoPacket::GetNumItems() == 0);
}

/** Actual same-XY hidden, foreign-owned and non-freight station windows cannot offer the Surface purchasing service. */
TEST_CASE("Station supply entry retains native owner layer and freight boundaries", "[surface-supply-controls-domain]")
{
	MockEnvironment::Instance(); LoadGeneratedWindowTestLanguage();
	SurfaceSupplyRig rig;
	std::unique_ptr<Company> foreign;
	SurfaceSupplyTestUI ui(_current_company);
	AutoRestoreBackup owner{rig.origin->owner};
	AutoRestoreBackup facilities{rig.origin->facilities};
	Station *station = rig.origin;
	SECTION("different actual company") {
		REQUIRE(Company::CanAllocateItem()); foreign.reset(Company::Create());
		rig.origin->owner = foreign->index; // Explicit native facility ownership boundary input, not a company takeover simulation.
	}
	SECTION("same coordinates on the rock plane") {
		station = rig.StationAt(TileXYL(20, 20, MapLayer::Underground));
		station->string_id = STR_SV_STNAME;
	}
	SECTION("bus-only facility") { rig.origin->facilities = {StationFacility::BusStop}; }
	const auto digest = ComputeSurfaceSupplyDigest(); const Money cash = Company::Get(_current_company)->money;
	ShowStationViewWindow(station->index);
	Window *window = FindWindowById(WindowClass::StationView, station->index); REQUIRE(window != nullptr);
	window->OnPaint();
	CHECK(window->IsWidgetDisabled(WID_SV_SUPPLIES)); CHECK(window->IsWidgetDisabled(WID_SV_SUPPLY_BUY));
	window->OnClick({}, WID_SV_SUPPLY_BUY, 1); // Stale/direct input must still recheck the actual native service domain.
	CHECK(ComputeSurfaceSupplyDigest() == digest); CHECK(Company::Get(_current_company)->money == cash);
	CHECK(CargoPacket::GetNumItems() == 0); CHECK(GetNextSurfaceSupplyOrderID().base() == 1);
}
