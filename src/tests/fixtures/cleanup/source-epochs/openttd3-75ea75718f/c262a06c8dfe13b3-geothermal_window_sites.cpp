/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file geothermal_window_sites.cpp Actual window callbacks over original cold claims and native paid construction owners.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs18-geothermal-energy-and-industry.md Group15CB.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs7-underground-construction-and-fleet.md Group4C; actual mouse survey/site/funding input.
 * @see docs/zz-reach2/architecture/gui/archi-gui.md.
 * Prepared floor/native starting cash are explicit fixture inputs; these controls do not claim paid boring or driven supply acceptance.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "mock_environment.h"
#include "underground_view_fixture.h"
#include "tube_input_view_fixture.h"
#include "geothermal_generator_fixture.h"
#include "../geothermal_collector.h"
#include "../geothermal_site_cmd.h"
#include "../underground_gui.h"
#include "../toolbar_gui.h"
#include "../tilehighlight_func.h"
#include "../widgets/underground_widget.h"
#include "../table/strings.h"
#include "../safeguards.h"

/** Real native definitions/company/town with an explicitly cancelled initial fixture blueprint, leaving its immutable original history intact. */
class GeothermalWindowSiteRig : public GeothermalGeneratorRig {
	AutoRestoreBackup<CommandPauseLevel> pause_commands{_settings_game.construction.command_pause_level,
		CommandPauseLevel::AllActions}; ///< Existing underground_window.cpp command-fixture policy: keep effects paused while allowing actual posted construction through the normal dispatcher; restore the caller's real setting.
public:
	/** @param model Actual dated initial machinery context; no building or accepted stock is created. */
	explicit GeothermalWindowSiteRig(GeothermalGeneratorModel model = GeothermalGeneratorModel::VentTap) : GeothermalGeneratorRig(false, model)
	{
		ResetGeothermalCollectors(); REQUIRE(InitialiseGeothermalCollectorDefinitions(this->ActivatedContent()));
		REQUIRE(Command<Commands::CancelGeothermalSite>::Do(DoCommandFlag::Execute, this->site).Succeeded());
	}
	/** Retire actual industries before resetting collector references; base teardown then restores map/company/definition owners. */
	~GeothermalWindowSiteRig()
	{
		for (Industry *industry : Industry::Iterate()) delete industry; // Actual destruction retires the exact native construction participant before fixture definition restoration.
		ResetGeothermalCollectors();
	}
};

/** @return Actual expanded Deep construction window; this creates no native claim, building, material or utility identity. */
static Window *OpenGeothermalSiteWindow()
{
	Window *window = ShowUndergroundConstructionToolbar(MapLayer::DeepUnderground, GetCurrentMultiLayerProfileSelection());
	REQUIRE(window != nullptr); window->OnPaint(); const auto height = window->height;
	window->OnClick({}, WID_UC_GEOTHERMAL, 1); window->OnPaint(); CHECK(window->height > height);
	return window;
}

/**
 * Drive the actual menu and placement callbacks through a complete pure request.
 * @param window Actual existing construction window.
 * @param request Explicit original natural source/family/world geometry, not a reserved identity.
 */
static void DraftGeothermalSiteInWindow(Window &window, const GeothermalSiteRequest &request)
{
	window.OnDropdownSelect(WID_UC_GEO_FAMILY, to_underlying(request.kind), 0);
	window.OnDropdownSelect(WID_UC_GEO_VENT, request.vent.base(), 0);
	window.OnDropdownSelect(WID_UC_GEO_ACCESS, to_underlying(request.access), 0);
	window.OnPaint(); REQUIRE_FALSE(window.IsWidgetDisabled(WID_UC_GEO_PLAN));
	window.OnClick({}, WID_UC_GEO_PLAN, 1);
	window.OnPlaceObject({}, request.tile); window.OnPlaceObject({}, request.endpoint); window.OnPaint();
	REQUIRE_FALSE(window.IsWidgetDisabled(WID_UC_GEO_CONFIRM));
}

/** Real geothermal action press/release and window timers retain the parent's cursor until an explicit menu cancellation, without native work or payment. */
TEST_CASE("Geothermal provider tools survive real native mouse dispatch", "[geothermal-provider-native-input]")
{
	const auto action = GENERATE(WID_UC_GEO_SURVEY, WID_UC_GEO_PLAN, WID_UC_GEO_HEAT, WID_UC_GEO_ELECTRIC, WID_UC_GEO_RISER);
	MockEnvironment::Instance(); LoadGeneratedWindowTestLanguage(); GeothermalWindowSiteRig rig;
	UndergroundToolbarTestUI ui{_current_company};
	auto *view = new UndergroundTestViewport(); SetViewportLayer(view, MapLayer::DeepUnderground); AllocateToolbar();
	Window *window = ShowUndergroundConstructionToolbar(MapLayer::DeepUnderground, GetCurrentMultiLayerProfileSelection());
	REQUIRE(window != nullptr); window->OnPaint();
	const auto before = std::tuple{ComputeGeothermalSiteDigest(), ComputeGeothermalCollectorDigest(), ComputeGeothermalGeneratorDigest(),
		ComputeUtilityPowerDigest(), Company::Get(_current_company)->money, TimerGameTick::counter};
	FeedbackClick(*window, WID_UC_GEOTHERMAL); window->OnPaint();
	// Physical actions live on the utilities page. This explicit page selection
	// is fixture setup; real action clicks must still own the native cursor.
	if (action == WID_UC_GEO_HEAT || action == WID_UC_GEO_ELECTRIC || action == WID_UC_GEO_RISER) window->OnDropdownSelect(WID_UC_GEO_PAGE, 1, 0);
	window->OnPaint(); REQUIRE_FALSE(window->IsWidgetDisabled(action));
	FeedbackClick(*window, action); FeedbackPumpWindows(); REQUIRE(_thd.GetCallbackWnd() == window);
	FeedbackClick(*window, WID_UC_GEOTHERMAL); FeedbackPumpWindows(); CHECK(_thd.GetCallbackWnd() != window);
	CHECK((std::tuple{ComputeGeothermalSiteDigest(), ComputeGeothermalCollectorDigest(), ComputeGeothermalGeneratorDigest(),
		ComputeUtilityPowerDigest(), Company::Get(_current_company)->money, TimerGameTick::counter} == before));
}

/** Main/extra viewport mouse input surveys an original vent, respects canonical dates and funds exactly one native empty collector or generator after explicit confirmation. */
TEST_CASE("Geothermal sites use actual main and extra viewport mouse input", "[geothermal-provider-site-input]")
{
	const auto kind = GENERATE(MultiLayerGeothermalConstructionKind::Collector, MultiLayerGeothermalConstructionKind::Powerhouse,
		MultiLayerGeothermalConstructionKind::MagmaPowerhouse);
	const bool use_extra = GENERATE(false, true);
	MockEnvironment::Instance(); LoadGeneratedWindowTestLanguage();
	GeothermalWindowSiteRig rig{kind == MultiLayerGeothermalConstructionKind::MagmaPowerhouse ? GeothermalGeneratorModel::MagmaLoop : GeothermalGeneratorModel::VentTap};
	auto request = rig.request; request.kind = kind;
	const auto vent = GetGeologyWorldState().vents.front(); const auto natural = TileXYL(vent.x, vent.y, vent.layer);
	if (kind == MultiLayerGeothermalConstructionKind::Collector) {
		request.tile = request.endpoint = natural;
		std::vector<TileIndex> floor;
		// The collector's full floor/rim is explicit prepared input. A control
		// test cannot turn a planning click into free excavation or delivery.
		for (int y = -1; y <= 3; ++y) for (int x = -1; x <= 3; ++x) floor.push_back(TileAddWrap(natural, x, y));
		rig.Open(floor);
	}
	UndergroundToolbarTestUI ui{_current_company};
	auto *main = new UndergroundTestViewport(); SetViewportLayer(main, use_extra ? MapLayer::Surface : MapLayer::DeepUnderground);
	Window *extra = CreateTubeInputExtraViewport(use_extra ? MapLayer::DeepUnderground : MapLayer::Surface);
	Window *active = use_extra ? extra : main; Window *other = use_extra ? main : extra; AllocateToolbar();
	Window *window = ShowUndergroundConstructionToolbar(MapLayer::DeepUnderground, GetCurrentMultiLayerProfileSelection());
	REQUIRE(window != nullptr); window->OnPaint(); FeedbackClick(*window, WID_UC_GEOTHERMAL); window->OnPaint();
	// Canonical family/source/orientation are explicit setup choices. Arming,
	// surveying, world placement, confirmation and funding use real mouse input.
	window->OnDropdownSelect(WID_UC_GEO_FAMILY, to_underlying(kind), 0);
	window->OnDropdownSelect(WID_UC_GEO_ACCESS, to_underlying(request.access), 0);
	const auto *terms = GetMultiLayerGeothermalConstruction(kind); REQUIRE(terms != nullptr);
	TimerGameCalendar::date = TimerGameCalendar::ConvertYMDToDate(TimerGameCalendar::Year{terms->introduction_year.base() - 1}, 11, 31);
	window->OnPaint(); CHECK(window->IsWidgetDisabled(WID_UC_GEO_PLAN));
	FeedbackClick(*window, WID_UC_GEO_PLAN); FeedbackPumpWindows(); CHECK(_thd.GetCallbackWnd() != window);
	TimerGameCalendar::date = TimerGameCalendar::ConvertYMDToDate(terms->introduction_year, 0, 1);
	window->OnPaint(); REQUIRE_FALSE(window->IsWidgetDisabled(WID_UC_GEO_PLAN));
	const auto cash = Company::Get(_current_company)->money; const auto geology = ComputeGeologyStateDigest(GetGeologyWorldState()); const auto random = _random;
	const auto before = std::tuple{ComputeGeothermalSiteDigest(), ComputeGeothermalCollectorDigest(), ComputeGeothermalGeneratorDigest(), ComputeUtilityPowerDigest(), cash};
	FeedbackClick(*window, WID_UC_GEO_SURVEY); FeedbackPumpWindows(); REQUIRE(_thd.GetCallbackWnd() == window);
	MouseClickTubeWorldTile(*other, MultiLayerMap::ProjectToSurface(natural));
	CHECK(window->GetWidgetString(WID_UC_GEO_STATUS, STR_GEO_HELP).find(GetString(STR_UNDERGROUND_WRONG_LAYER)) != std::string::npos);
	MouseClickTubeWorldTile(*active, natural); window->OnPaint();
	CHECK(window->GetWidgetString(WID_UC_GEO_STATUS, STR_GEO_HELP).find("Natural heat capacity") != std::string::npos);
	FeedbackClick(*window, WID_UC_GEO_PLAN); FeedbackPumpWindows(); REQUIRE(_thd.GetCallbackWnd() == window);
	MouseClickTubeWorldTile(*active, request.tile); MouseClickTubeWorldTile(*active, request.endpoint); window->OnPaint();
	REQUIRE_FALSE(window->IsWidgetDisabled(WID_UC_GEO_CONFIRM));
	CHECK((std::tuple{ComputeGeothermalSiteDigest(), ComputeGeothermalCollectorDigest(), ComputeGeothermalGeneratorDigest(), ComputeUtilityPowerDigest(), Company::Get(_current_company)->money} == before));
	CHECK(std::ranges::equal(random.state, _random.state));
	FeedbackClick(*window, WID_UC_GEO_CONFIRM); FeedbackPumpWindows();
	const auto *claim = GetGeothermalSiteClaimForTile(request.tile); REQUIRE(claim != nullptr); CHECK(claim->id > rig.site); CHECK(claim->site.kind == kind);
	CHECK(Company::Get(_current_company)->money == cash);
	FeedbackClick(*window, WID_UC_GEO_SURVEY); FeedbackPumpWindows(); MouseClickTubeWorldTile(*active, request.tile); window->OnPaint();
	REQUIRE_FALSE(window->IsWidgetDisabled(WID_UC_GEO_FUND));
	CHECK(std::ranges::equal(random.state, _random.state));
	FeedbackClick(*window, WID_UC_GEO_FUND); FeedbackPumpWindows();
	const auto *industry = Industry::GetByTile(request.tile); REQUIRE(industry != nullptr);
	CHECK(Company::Get(_current_company)->money == cash - Money{terms->capital_cost});
	// Funding creates actual empty accepted slots; no UI path may supply the
	// construction bill or silently advance prepaid machinery work.
	for (const auto &slot : industry->accepted) CHECK(slot.waiting == 0);
	if (kind == MultiLayerGeothermalConstructionKind::Collector) {
		const auto *state = GetGeothermalCollectorState(industry->index); REQUIRE(state != nullptr); CHECK(state->work_ticks == 0); CHECK_FALSE(state->materials_committed);
	} else {
		const auto *state = GetGeothermalGeneratorState(industry->index); REQUIRE(state != nullptr); CHECK(state->work_ticks == 0); CHECK_FALSE(state->materials_committed);
	}
	CHECK(CaptureUtilityPowerState().producers.empty()); CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == geology);
	// The deliberate native funding command initializes Industry colour/counter
	// through simulation RNG. Subsequent presentation and cancellation must
	// preserve that resulting state, rather than undo legitimate construction.
	const auto funded_random = _random;
	const auto funded = std::tuple{ComputeGeothermalSiteDigest(), ComputeGeothermalCollectorDigest(), ComputeGeothermalGeneratorDigest(), ComputeUtilityPowerDigest(), Company::Get(_current_company)->money};
	FeedbackClick(*window, WID_UC_GEO_PLAN); FeedbackPumpWindows(); SetViewportLayer(active, MapLayer::Underground); CHECK(_thd.GetCallbackWnd() != window);
	CHECK((std::tuple{ComputeGeothermalSiteDigest(), ComputeGeothermalCollectorDigest(), ComputeGeothermalGeneratorDigest(), ComputeUtilityPowerDigest(), Company::Get(_current_company)->money} == funded));
	CHECK(std::ranges::equal(funded_random.state, _random.state));
}

/** Both real dated generators use pure window quotes, synchronized cancellation and one original paid funding action; no UI refresh supplies stock or work. */
TEST_CASE("Geothermal window plans cancels and funds the actual dated generators", "[geothermal-window-plan]")
{
	MockEnvironment::Instance(); LoadGeneratedWindowTestLanguage();
	const auto model = GENERATE(GeothermalGeneratorModel::VentTap, GeothermalGeneratorModel::MagmaLoop);
	GeothermalWindowSiteRig rig{model}; UndergroundToolbarTestUI ui{_current_company};
	const auto geology = ComputeGeologyStateDigest(GetGeologyWorldState()), utility = ComputeUtilityPowerDigest();
	const auto claims = ComputeGeothermalSiteDigest(); const auto cash = Company::Get(_current_company)->money; const auto random = _random;
	Window *window = OpenGeothermalSiteWindow(); DraftGeothermalSiteInWindow(*window, rig.request);
	window->OnPaint(); window->OnPaint();
	CHECK(ComputeGeothermalSiteDigest() == claims); CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == geology);
	CHECK(ComputeUtilityPowerDigest() == utility); CHECK(Industry::GetNumItems() == 0);
	CHECK(Company::Get(_current_company)->money == cash); CHECK(std::ranges::equal(random.state, _random.state));
	window->OnClick({}, WID_UC_GEO_CONFIRM, 1);
	const auto *claim = GetGeothermalSiteClaimForTile(rig.request.tile); REQUIRE(claim != nullptr); const auto first = claim->id;
	CHECK(first > rig.site); CHECK(claim->site.kind == rig.request.kind); CHECK(Company::Get(_current_company)->money == cash);
	window->OnDropdownSelect(WID_UC_GEO_SITE, first.base(), 0); window->OnPaint(); REQUIRE_FALSE(window->IsWidgetDisabled(WID_UC_GEO_CANCEL));
	window->OnClick({}, WID_UC_GEO_CANCEL, 1); CHECK_FALSE(GetGeothermalSiteClaim(first)->active);
	window->OnPaint(); CHECK(window->IsWidgetDisabled(WID_UC_GEO_FUND)); CHECK(Industry::GetNumItems() == 0);
	DraftGeothermalSiteInWindow(*window, rig.request); window->OnClick({}, WID_UC_GEO_CONFIRM, 1);
	claim = GetGeothermalSiteClaimForTile(rig.request.tile); REQUIRE(claim != nullptr); REQUIRE(claim->id > first); const auto selected = claim->id;
	window->OnDropdownSelect(WID_UC_GEO_SITE, selected.base(), 0); window->OnPaint(); REQUIRE_FALSE(window->IsWidgetDisabled(WID_UC_GEO_FUND));
	CHECK(window->GetWidgetString(WID_UC_GEO_STATUS, STR_GEO_HELP).find("Capital quote") != std::string::npos);
	window->OnClick({}, WID_UC_GEO_FUND, 1); REQUIRE(Industry::GetNumItems() == 1);
	const auto *industry = Industry::GetByTile(rig.request.tile); REQUIRE(industry != nullptr);
	const auto *state = GetGeothermalGeneratorState(industry->index); REQUIRE(state != nullptr);
	CHECK(state->site == selected); CHECK(state->model == model); CHECK(state->work_ticks == 0); CHECK_FALSE(state->materials_committed);
	for (const auto &slot : industry->accepted) CHECK(slot.waiting == 0); // Funding owns native empty accepted slots; inspection must not supply its transported construction bill.
	CHECK_FALSE(GetGeothermalSiteClaim(selected)->active); CHECK(CaptureUtilityPowerState().producers.empty());
	CHECK(Company::Get(_current_company)->money == cash - Money{GetMultiLayerGeothermalConstruction(rig.request.kind)->capital_cost});
	window->OnPaint(); CHECK(window->IsWidgetDisabled(WID_UC_GEO_FUND)); CHECK(window->IsWidgetDisabled(WID_UC_GEO_CANCEL));
	const auto funded = ComputeGeothermalGeneratorDigest(); const auto money = Company::Get(_current_company)->money;
	window->OnClick({}, WID_UC_GEO_FUND, 1); window->OnPaint(); CHECK(ComputeGeothermalGeneratorDigest() == funded); CHECK(Company::Get(_current_company)->money == money);
	CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == geology);
}

/** Public natural survey selects the original capacity; collector funding still creates only native empty stock and zero construction/source work. */
TEST_CASE("Geothermal window surveys the original vent and funds its actual collector", "[geothermal-window-collector]")
{
	MockEnvironment::Instance(); LoadGeneratedWindowTestLanguage(); GeothermalWindowSiteRig rig;
	const auto vent = GetGeologyWorldState().vents.front(); auto request = rig.request;
	request.kind = MultiLayerGeothermalConstructionKind::Collector; request.tile = request.endpoint = TileXYL(vent.x, vent.y, vent.layer);
	std::vector<TileIndex> floor;
	// Full natural collector floor/rim is explicit boundary input. The native
	// funding command still charges capital and supplies no bill or paid work.
	for (int y = -1; y <= 3; ++y) for (int x = -1; x <= 3; ++x) floor.push_back(TileAddWrap(request.tile, x, y));
	rig.Open(floor); UndergroundToolbarTestUI ui{_current_company}; Window *window = OpenGeothermalSiteWindow();
	const auto geology = ComputeGeologyStateDigest(GetGeologyWorldState()), claims = ComputeGeothermalSiteDigest(), utility = ComputeUtilityPowerDigest();
	const auto cash = Company::Get(_current_company)->money;
	window->OnClick({}, WID_UC_GEO_SURVEY, 1); window->OnPlaceObject({}, request.tile); window->OnPaint();
	CHECK(window->GetWidgetString(WID_UC_GEO_STATUS, STR_GEO_HELP).find("Natural heat capacity") != std::string::npos);
	CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == geology); CHECK(ComputeGeothermalSiteDigest() == claims); CHECK(ComputeUtilityPowerDigest() == utility);
	DraftGeothermalSiteInWindow(*window, request); window->OnClick({}, WID_UC_GEO_CONFIRM, 1);
	const auto *claim = GetGeothermalSiteClaimForTile(request.tile); REQUIRE(claim != nullptr);
	window->OnDropdownSelect(WID_UC_GEO_SITE, claim->id.base(), 0); window->OnPaint(); REQUIRE_FALSE(window->IsWidgetDisabled(WID_UC_GEO_FUND));
	window->OnClick({}, WID_UC_GEO_FUND, 1);
	const auto collector = FindGeothermalCollectorForVent(vent.id); REQUIRE(Industry::IsValidID(collector));
	const auto *state = GetGeothermalCollectorState(collector); REQUIRE(state != nullptr); CHECK(state->work_ticks == 0); CHECK_FALSE(state->materials_committed);
	for (const auto &slot : Industry::Get(collector)->accepted) CHECK(slot.waiting == 0); // Native empty slots prove a visible funding action supplied no material packets.
	CHECK(Company::Get(_current_company)->money == cash - Money{180000}); CHECK(CaptureUtilityPowerState().producers.empty());
	CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == geology);
}

/** Wrong-layer clicks, stale issued-plan selection and switching back to original excavation tools cancel only presentation and preserve actual ownership. */
TEST_CASE("Geothermal drafts and stale window selections preserve original owners", "[geothermal-window-placement]")
{
	MockEnvironment::Instance(); LoadGeneratedWindowTestLanguage(); GeothermalWindowSiteRig rig; UndergroundToolbarTestUI ui{_current_company};
	Window *window = OpenGeothermalSiteWindow(); const auto claims = ComputeGeothermalSiteDigest(), geology = ComputeGeologyStateDigest(GetGeologyWorldState());
	DraftGeothermalSiteInWindow(*window, rig.request);
	window->OnPlaceObject({}, MultiLayerMap::ProjectToSurface(rig.request.tile));
	CHECK(window->GetWidgetString(WID_UC_GEO_STATUS, STR_GEO_HELP).find("another layer") != std::string::npos);
	window->OnClick({}, WID_UC_SURVEY, 1); window->OnPaint(); CHECK(window->IsWidgetDisabled(WID_UC_GEO_CONFIRM));
	CHECK(ComputeGeothermalSiteDigest() == claims); CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == geology);
	DraftGeothermalSiteInWindow(*window, rig.request); window->OnClick({}, WID_UC_GEO_CONFIRM, 1);
	const auto *claim = GetGeothermalSiteClaimForTile(rig.request.tile); REQUIRE(claim != nullptr); const auto selected = claim->id;
	window->OnDropdownSelect(WID_UC_GEO_SITE, selected.base(), 0); window->OnPaint(); REQUIRE_FALSE(window->IsWidgetDisabled(WID_UC_GEO_FUND));
	REQUIRE(Command<Commands::CancelGeothermalSite>::Do(DoCommandFlag::Execute, selected).Succeeded());
	const auto cancelled = ComputeGeothermalSiteDigest(); const auto cash = Company::Get(_current_company)->money;
	window->OnPaint(); CHECK(window->IsWidgetDisabled(WID_UC_GEO_FUND)); window->OnClick({}, WID_UC_GEO_FUND, 1);
	CHECK(Industry::GetNumItems() == 0); CHECK(ComputeGeothermalSiteDigest() == cancelled); CHECK(Company::Get(_current_company)->money == cash);
	DraftGeothermalSiteInWindow(*window, rig.request); window->OnClick({}, WID_UC_GEOTHERMAL, 1); window->OnPaint();
	CHECK(window->IsWidgetDisabled(WID_UC_GEO_CONFIRM)); CHECK(ComputeGeothermalSiteDigest() == cancelled);
}

/** Live introduction/company/editor admission is re-queried by actual paint/action callbacks; an open draft and stale policy snapshot confer no permission. */
TEST_CASE("Geothermal window dates and company policy gate actual planning", "[geothermal-window-policy]")
{
	MockEnvironment::Instance(); LoadGeneratedWindowTestLanguage(); GeothermalWindowSiteRig rig{GeothermalGeneratorModel::MagmaLoop};
	UndergroundToolbarTestUI ui{_current_company}; Window *window = OpenGeothermalSiteWindow(); DraftGeothermalSiteInWindow(*window, rig.request);
	const auto claims = ComputeGeothermalSiteDigest(), geology = ComputeGeologyStateDigest(GetGeologyWorldState()); const auto random = _random;
	TimerGameCalendar::date = TimerGameCalendar::ConvertYMDToDate(TimerGameCalendar::Year{1999}, 11, 31);
	window->OnPaint(); CHECK(window->IsWidgetDisabled(WID_UC_GEO_PLAN)); CHECK(window->IsWidgetDisabled(WID_UC_GEO_CONFIRM)); window->OnClick({}, WID_UC_GEO_CONFIRM, 1);
	TimerGameCalendar::date = TimerGameCalendar::ConvertYMDToDate(TimerGameCalendar::Year{1979}, 11, 31);
	window->OnPaint(); CHECK(window->IsWidgetDisabled(WID_UC_GEO_SURVEY)); CHECK(window->IsWidgetDisabled(WID_UC_GEO_CONFIRM));
	TimerGameCalendar::date = TimerGameCalendar::ConvertYMDToDate(TimerGameCalendar::Year{2000}, 0, 1);
	{
		AutoRestoreBackup local{_local_company, COMPANY_SPECTATOR}; window->OnPaint(); CHECK(window->IsWidgetDisabled(WID_UC_GEO_PLAN)); CHECK(window->IsWidgetDisabled(WID_UC_GEO_CONFIRM));
	}
	{
		AutoRestoreBackup editor{_game_mode, GameMode::Editor}; window->OnPaint(); CHECK(window->IsWidgetDisabled(WID_UC_GEO_PLAN)); CHECK(window->IsWidgetDisabled(WID_UC_GEO_CONFIRM));
	}
	window->OnPaint(); CHECK_FALSE(window->IsWidgetDisabled(WID_UC_GEO_CONFIRM));
	CHECK(ComputeGeothermalSiteDigest() == claims); CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == geology); CHECK(std::ranges::equal(random.state, _random.state));
	CHECK(Industry::GetNumItems() == 0); CHECK(CaptureUtilityPowerState().producers.empty());
}
