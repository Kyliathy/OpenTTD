/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/**
 * @file space_fixes_native.cpp Reproducible observations of retained Space demos.
 * @see docs/zz-reach2/upgrades/2026-10/r2sff1-space-fixes-features-1.md Groups1-8.
 * @see docs/zz-reach2/upgrades/2026-10/r2sff1-space-fixes-features-1.md Group11; legacy active fields seed their contained renewal owner before observation.
 * @see docs/zz-reach2/upgrades/2026-10/r2sff1-space-fixes-features-1.md Group14B; real retained-file preview preserves the open world.
 * @see docs/zz-reach2/upgrades/2026-10/r2sff1-space-fixes-features-1.md Groups16-17; copied packet evidence and pure policy refusal for original unsupported roles.
 * @see docs/zz-reach2/upgrades/2026-10/r2sff1-space-fixes-features-1.md Group29; actual normal-loop lifecycle acceptance runs separately from unchanged-world observers.
 * @see docs/zz-reach2/upgrades/2026-10/r2sff1-space-fixes-features-1.md Group30B; actual policy input retains original hull/cargo/work owners.
 * @see docs/zz-reach2/upgrades/2026-10/r2sff1-space-fixes-features-1.md Group31B; explicit original role selection and full-hold/handling acceptance.
 * @see docs/zz-reach2/upgrades/2026-10/r2sff1-space-fixes-features-1.md Group31C; actual terminal exits and saved native availability/refusal waits.
 * @see docs/zz-reach2/upgrades/2026-10/r2sff1-space-fixes-features-1.md Group31D; inspect original paid service parts, allowance and shell owners before choosing recovery scenarios.
 * @see docs/zz-reach2/upgrades/2026-10/r2sff1-space-fixes-features-1.md Group31E; naturally earned exit/timer/birth checkpoints and real shared-driver scheduling.
 * @see docs/zz-reach2/upgrades/2026-10/r2sff1-space-fixes-features-1.md Group32B; declared historical issuance prefix followed by real native high-ID birth/extraction.
 * @see docs/zz-reach2/architecture/gui/archi-gui.md Invariants.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../asteroid.h"
#include "../fios.h"
#include "../saveload/saveload_func.h"
#include "../space_vehicle.h"
#include "../space_vehicle_cmd.h"
#include "../space_mining_tuning.h"
#include "../space_mining_continuation.h"
#include "../space_mining_target.h"
#include "../space_departure.h"
#include "../space_approach.h"
#include "../space_manifest.h"
#include "../space_pod.h"
#include "../space_transition.h"
#include "../launch_center.h"
#include "../launch_center_cmd.h"
#include "../station_base.h"
#include "../multilayer_space_content.h"
#include "../space_structure.h"
#include "../space_structure_cmd.h"
#include "../utility_power.h"
#include "../space_work_cost.h"
#include "../3rdparty/nlohmann/json.hpp"
#include "../vehicle_cmd.h"
#include "../orbital_station.h"
#include "../space_construction_gui.h"
#include "../company_func.h"
#include "../company_base.h"
#include "../cargopacket.h"
#include "../console_internal.h"
#include "../core/backup_type.hpp"
#include "../openttd.h"
#include "../blitter/factory.hpp"
#include "../spritecache.h"
#include "../fontcache.h"
#include "../gfx_func.h"
#include "../window_func.h"
#include "../window_gui.h"
#include "../map_layer_gui.h"
#include "../viewport_type.h"
#include "../zoom_func.h"
#include "../hotkeys.h"
#include "../ini_type.h"
#include "../gui.h"
#include "../toolbar_gui.h"
#include "../video/null_v.h"
#include "../map_layer.h"
#include "../settings_type.h"
#include "../core/string_consumer.hpp"
#include "../widgets/settings_widget.h"
#include "mock_environment.h"
#include "underground_view_fixture.h"
#include "space_construction_trace.h"
#include "asteroid_cargo_native.h"
#include "asteroid_lifecycle_native.h"
#include "../widgets/space_structure_widget.h"
#include "../widgets/space_survey_widget.h"
#include "../table/strings.h"
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <thread>
#include "../safeguards.h"

static std::function<void()> *space_fixes_observer = nullptr;

/** Minimal retained window for exercising modal/non-closable input boundaries. */
struct SpaceShortcutTestWindow : Window {
	/**
	 * Create an actual window with the selected native close/focus policy.
	 * @param desc Retained descriptor, including modal/non-closable flags.
	 * @param number Exact identity to exercise through shortcut dispatch.
	 * @param parent_window Optional real parent whose modal child must remain active.
	 */
	SpaceShortcutTestWindow(WindowDesc &desc, WindowNumber number, Window *parent_window = nullptr) : Window(desc)
	{
		this->InitNested(number);
		this->parent = parent_window;
	}
};

/** Retained hotkey migration is exact, preserving intentionally customized and empty bindings. */
TEST_CASE("Only obsolete single Debug hotkeys migrate away from layer shortcuts", "[space-shortcut-config]")
{
	const std::array<std::string, 3> names{"crash_game", "money", "update_coordinates"};
	// Each legacy Debug action has a different digit; arbitrary entries sharing
	// a global group or an ALT modifier must not be rewritten incidentally.
	for (size_t i = 0; i < names.size(); ++i) {
		const uint16_t legacy = static_cast<uint16_t>(WKC_ALT | ('0' + i)), replacement = legacy | WKC_CTRL;
		HotkeyList list{"global", {Hotkey(replacement, names[i], 17)}};
		IniFile ini;
		auto &item = ini.GetOrCreateGroup("global").GetOrCreateItem(names[i]);
		item.SetValue(fmt::format("ALT+{}", i));
		const auto original = item.value;
		list.Load(ini);
		CHECK(list.CheckMatch(legacy) == -1);
		CHECK(list.CheckMatch(replacement) == 17);
		CHECK(item.value == original);
		item.SetValue("CTRL+SHIFT+M");
		list.Load(ini);
		CHECK(list.CheckMatch(WKC_CTRL | WKC_SHIFT | 'M') == 17);
		CHECK(list.CheckMatch(replacement) == -1);
		item.SetValue(fmt::format("ALT+{},SHIFT+F9", i));
		list.Load(ini);
		CHECK(list.CheckMatch(legacy) == 17);
		CHECK(list.CheckMatch(WKC_SHIFT | WKC_F9) == 17);
		item.SetValue("");
		list.Load(ini);
		CHECK(list.CheckMatch(legacy) == -1);
		CHECK(list.CheckMatch(replacement) == -1);
	}
}

/** Partial allocations use the same actual editor toolbar while unavailable named layers remain ignored. */
TEST_CASE("Direct layer shortcuts respect one and three plane allocations", "[space-shortcut-layers]")
{
	MockEnvironment::Instance();
	LoadGeneratedWindowTestLanguage();
	AutoRestoreBackup mode(_game_mode, GameMode::Editor);
	// Live loaders now expand historical maps to four planes. The map owner's
	// explicit allocation fixture is therefore the actual boundary for testing
	// unavailable named layers, without editing a loaded world's paid assets.
	for (uint planes : {1U, 3U}) {
		Map::Allocate(64, 64, planes);
		InitializeLandscape();
		UndergroundToolbarTestUI ui(_current_company);
		auto *main = new UndergroundTestViewport();
		AllocateToolbar();
		HandleKeypress(WKC_ALT | '4', 0);
		CHECK(main->viewport->layer == (planes == 1 ? MapLayer::Surface : MapLayer::Space));
		const auto before = main->viewport->layer;
		HandleKeypress(WKC_ALT | '3', 0);
		CHECK(main->viewport->layer == before);
		HandleKeypress(WKC_ALT | '2', 0);
		CHECK(main->viewport->layer == (planes == 1 ? MapLayer::Surface : MapLayer::Underground));
		HandleKeypress(WKC_ALT | '1', 0);
		CHECK(main->viewport->layer == MapLayer::Surface);
	}
}

/** Exercise actual normal/editor dispatch, retained window identity and client-only input policy. */
static void VerifySpaceFixesShortcuts()
{
	CHECK(_settings_client.gui.hotkey_toggles_window);
	if (const char *expected = std::getenv("OTTD_SPACE_FIXES_EXPECT_LAYERS")) {
		const auto layers = ParseInteger<uint>(expected);
		REQUIRE(layers.has_value());
		REQUIRE(MultiLayerMap::GetLayerCount() == *layers);
	}
	AutoRestoreBackup toggling(_settings_client.gui.hotkey_toggles_window, true);
	const bool custom = std::getenv("OTTD_SPACE_FIXES_CUSTOM") != nullptr;
	const auto *company = Company::GetIfValid(_local_company);
	const Money balance = company == nullptr ? Money{0} : company->money;
	static constexpr NWidgetPart parts[] = {NWidget(WWT_PANEL, Colours::DarkBlue), SetMinimalSize(100, 40), EndContainer()};
	static WindowDesc cover_desc(WindowPosition::Center, {}, 0, 0, WindowClass::Textfile, WindowClass::None, {}, parts);
	static WindowDesc modal_desc(WindowPosition::Center, {}, 0, 0, WindowClass::QueryString, WindowClass::None, {WindowDefaultFlag::Modal}, parts);
	static WindowDesc protected_desc(WindowPosition::Center, {}, 0, 0, WindowClass::GameOptions, WindowClass::None, {WindowDefaultFlag::NoClose}, parts);
	Window *main = GetMainWindow();
	ShowExtraViewportWindow(TileXYL(4, 4, MapLayer::Surface));
	Window *extra = FindWindowById(WindowClass::ExtraViewport, 0);
	REQUIRE(extra != nullptr);
	const auto extra_layer = extra->viewport->layer;
	const Point extra_position{extra->viewport->scrollpos_x, extra->viewport->scrollpos_y};
	const std::array<MapLayer, 4> layers{MapLayer::Surface, MapLayer::Underground, MapLayer::DeepUnderground, MapLayer::Space};
	// Both actual toolbar classes consume named layer actions. Enum ordinals
	// differ from display order, and changing the main view must not move extras.
	for (GameMode mode : {GameMode::Normal, GameMode::Editor}) {
		AutoRestoreBackup game_mode(_game_mode, mode);
		CloseWindowByClass(WindowClass::MainToolbar);
		Window::DeleteClosedWindows();
		AllocateToolbar();
		SetViewportLayer(main, MapLayer::Surface);
		const Point plane_origin{main->viewport->scrollpos_x, main->viewport->scrollpos_y};
		for (size_t i = 0; i < layers.size(); ++i) {
			const uint16_t digit = static_cast<uint16_t>(custom && i == 3 ? '9' : '1' + i);
			const auto previous = main->viewport->layer;
			HandleKeypress(WKC_ALT | digit, 0);
			CHECK(main->viewport->layer == (MultiLayerMap::IsValidLayer(layers[i]) ? layers[i] : previous));
			CHECK(extra->viewport->layer == extra_layer);
			CHECK(Point{extra->viewport->scrollpos_x, extra->viewport->scrollpos_y} == extra_position);
		}
		HandleKeypress(WKC_ALT | '1', 0);
		CHECK(Point{main->viewport->scrollpos_x, main->viewport->scrollpos_y} == plane_origin);
		SelectMainViewportLayer(MapLayer::Invalid);
		CHECK(main->viewport->layer == MapLayer::Surface);
		if (mode == GameMode::Normal) {
			HandleKeypress('N', 'n');
			CHECK(main->viewport->layer == GetNextMapLayer(MapLayer::Surface, MultiLayerMap::GetLayerCount()));
		}
		CloseWindowByClass(WindowClass::GameOptions);
		HandleKeypress(WKC_F2, 0);
		Window *options = FindWindowById(WindowClass::GameOptions, GameOptionsWindowNumber::GameOptions);
		REQUIRE(options != nullptr);
		// Game Options has a higher native z-priority than ordinary viewports.
		// Use a real retained cover at the same priority, so it actually covers
		// the options window before testing raise-versus-close dispatch.
		auto *cover = new SpaceShortcutTestWindow(cover_desc, 123);
		HandleKeypress(WKC_F2, 0);
		REQUIRE(FindWindowById(WindowClass::GameOptions, GameOptionsWindowNumber::GameOptions) == options);
		HandleKeypress(WKC_F2, 0);
		REQUIRE(FindWindowById(WindowClass::GameOptions, GameOptionsWindowNumber::GameOptions) == nullptr);
		cover->Close();
		HandleKeypress(WKC_F2, 0);
		options = FindWindowById(WindowClass::GameOptions, GameOptionsWindowNumber::GameOptions);
		REQUIRE(options != nullptr);
		options->flags.Set(WindowFlag::Sticky);
		HandleKeypress(WKC_F2, 0);
		CHECK(FindWindowById(WindowClass::GameOptions, GameOptionsWindowNumber::GameOptions) == options);
		options->flags.Reset(WindowFlag::Sticky);
		{
			AutoRestoreBackup disabled(_settings_client.gui.hotkey_toggles_window, false);
			HandleKeypress(WKC_F2, 0);
			HandleKeypress(WKC_F2, 0);
			CHECK(FindWindowById(WindowClass::GameOptions, GameOptionsWindowNumber::GameOptions) == options);
		}
		// Text input in the real Advanced filter wins before toolbar/global keys.
		options->OnClick({}, WID_GO_TAB_ADVANCED, 1);
		const auto layer = main->viewport->layer;
		HandleKeypress('N', 'n');
		CHECK(main->viewport->layer == layer);
		options->OnClick({}, WID_GO_TAB_GENERAL, 1);
		CloseWindowByClass(WindowClass::GameOptions);
	}
	CloseWindowByClass(WindowClass::MainToolbar);
	CloseWindowByClass(WindowClass::ExtraViewport);
	Window::DeleteClosedWindows();
	AllocateToolbar();
	const uint16_t money_key = custom ? WKC_CTRL | WKC_SHIFT | 'M' : WKC_CTRL | WKC_ALT | '1';
	CHECK(main->window_desc.hotkeys->CheckMatch(WKC_ALT | '1') == -1);
#if defined(_DEBUG)
	CHECK(main->window_desc.hotkeys->CheckMatch(money_key) >= 0);
#else
	CHECK(main->window_desc.hotkeys->CheckMatch(money_key) == -1);
#endif
	HandleKeypress(WKC_F2, 0);
	Window *options = FindWindowById(WindowClass::GameOptions, GameOptionsWindowNumber::GameOptions);
	REQUIRE(options != nullptr);
	auto *modal = new SpaceShortcutTestWindow(modal_desc, 123, options);
	HandleKeypress(WKC_F2, 0);
	CHECK(FindWindowById(WindowClass::GameOptions, GameOptionsWindowNumber::GameOptions) == options);
	CHECK(FindWindowById(WindowClass::QueryString, 123) == modal);
	modal->Close();
	options->Close();
	Window::DeleteClosedWindows();
	auto *protected_window = new SpaceShortcutTestWindow(protected_desc, GameOptionsWindowNumber::GameOptions);
	HandleKeypress(WKC_F2, 0);
	CHECK(FindWindowById(WindowClass::GameOptions, GameOptionsWindowNumber::GameOptions) == protected_window);
	protected_window->Close();
	Window::DeleteClosedWindows();
	// Layer/window input must not trigger the old Debug money action, including
	// when a retained configuration originally assigned it to Alt+1.
	CHECK((company == nullptr || company->money == balance));
	fmt::print("SHORTCUTS normal_editor=true custom={} allocated_layers={} preserved_window_identity=true modal_pinned_protected=true\n", custom, MultiLayerMap::GetLayerCount());
}

/**
 * Invoke the observer after normal native loading has established every owner.
 * @param args Native console arguments; this command accepts none.
 * @return Whether the registered test observer ran.
 */
static bool ObserveSpaceFixes(std::span<std::string_view> args)
{
	REQUIRE(args.size() == 1);
	REQUIRE(space_fixes_observer != nullptr);
	(*space_fixes_observer)();
	return true;
}

/**
 * Capture the actual window, including controls beyond the client-area boundary.
 * @param window Existing native window; drawing must not advance simulation.
 * @param path Disposable PPM filename supplied by the repository-local harness.
 */
static void CaptureSpaceFixesWindow(Window &window, const std::filesystem::path &path)
{
	std::vector<uint8_t> pixels(static_cast<size_t>(window.width) * window.height);
	DrawPixelInfo dpi{pixels.data(), 0, 0, window.width, window.height, window.width, ZoomLevel::Min};
	AutoRestoreBackup screen(_screen, dpi);
	AutoRestoreBackup drawing(_cur_dpi, &dpi);
	AutoRestoreBackup left(window.left, 0);
	AutoRestoreBackup top(window.top, 0);
	AutoRestoreBackup animation(_screen_disable_anim, true);
	window.OnPaint();
	std::ofstream output(path, std::ios::binary);
	REQUIRE(output.good());
	output << "P6\n" << window.width << " " << window.height << "\n255\n";
	// Preserve native palette pixels so the capture can establish real text and
	// widget clipping without synthesizing a replacement interface.
	for (uint8_t pixel : pixels) {
		const auto &colour = _cur_palette.palette[pixel];
		const std::array<uint8_t, 3> rgb{colour.r, colour.g, colour.b};
		output.write(reinterpret_cast<const char *>(rgb.data()), rgb.size());
	}
	REQUIRE(output.good());
}

/**
 * Click the real on-screen control through native input and hit testing.
 * @param window Existing visible window whose position supplies screen coordinates.
 * @param widget Expected action; a hidden control cannot pass this boundary.
 */
static void ClickSpaceFixesWidget(Window &window, WidgetID widget)
{
	const Rect rect = window.GetWidget<NWidgetBase>(widget)->GetCurrentRect();
	const Point point{(rect.left + rect.right) / 2, (rect.top + rect.bottom) / 2};
	REQUIRE(GetWidgetFromPos(&window, point.x, point.y) == widget);
	// A complete press/release preserves the normal window/modal dispatcher and
	// proves the measured rectangle is an actionable control, not a hidden node.
	for (bool down : {false, true, false}) {
		_cursor.pos = {window.left + point.x, window.top + point.y};
		_cursor.in_window = true;
		_left_button_down = down;
		if (!down) _left_button_clicked = false;
		HandleMouseEvents();
	}
}

/** @param craft Original inspected native hull. @return Detached identities, immutable source and quantities from its actual holding; no packet or cargo owner is created. */
static std::vector<std::tuple<CargoPacketID, Source, uint16_t>> ObserveMiningControlCargo(const SpaceVehicle &craft)
{
	std::vector<std::tuple<CargoPacketID, Source, uint16_t>> result;
	// Every actual held fragment participates, including distinct retired-body
	// origins that cannot merge merely because their mineral is the same.
	for (const auto *packet : *craft.cargo.Packets()) result.emplace_back(packet->index, packet->GetSource(), packet->Count());
	return result;
}

/** Check actual policy controls using original native mouse dispatch and a retained demo.
 * @param scenario "edit" prefers an actual heavy miner; "edit-light" requires an original15; "unsupported" selects an actual deployed non-miner without inventing a role.
 * @param output Private repository artifact directory for native captures and policy-current.sav.
 */
static void VerifySpaceMiningPolicyControls(std::string_view scenario, const char *output)
{
	REQUIRE(BlitterFactory::SelectBlitter("8bpp-simple") != nullptr); GfxClearSpriteCache(); FontCache::ClearFontCaches(FONTSIZES_REQUIRED);
	AutoRestoreBackup running(_pause_mode, PauseModes{}); AutoRestoreBackup shift(_shift_pressed, false); AutoRestoreBackup control(_ctrl_pressed, false);
	SpaceVehicle *selected = nullptr;
	// Prefer an existing heavy miner for the live input case; the original
	// legacy role remains supported. Unsupported checks select a real other
	// role, never a copied craft with a forged engine/model or ownership.
	for (auto *craft : SpaceVehicle::Iterate()) {
		if (craft->owner != _local_company || craft->construction.placement != SpacePlacement::Orbital) continue;
		const bool miner = GetSpaceMiningTuning(GetSpaceVehicleModel(*craft)) != nullptr;
		if ((scenario == "unsupported") == miner) continue;
		// Explicit role acceptance must select an existing original15; changing
		// an engine/model would bypass its actual paid custody and tuning.
		if (scenario == "edit-light" && GetSpaceVehicleModel(*craft) != SPACE_PROBE_MODEL) continue;
		if (selected == nullptr || (GetSpaceVehicleModel(*craft) == SPACE_HEAVY_MINER_MODEL && GetSpaceVehicleModel(*selected) != SPACE_HEAVY_MINER_MODEL)) selected = craft;
	}
	REQUIRE(selected != nullptr); const auto reference = GetSpaceCraftReference(*selected);
	ShowSpaceSurveyWindow(reference); auto *window = FindWindowById(WindowClass::SpaceSurvey, 0); REQUIRE(window);
	const std::array<WidgetID, 3> choices{WID_SS_POLICY_LEGACY, WID_SS_POLICY_RETURN, WID_SS_POLICY_FULL};
	/** @param name Stable native artifact name. Capture the complete bounded controls and the last wrapped row without offering any simulation opportunity. */
	auto capture = [&](std::string_view name) {
		ShowSpaceSurveyWindow(reference); const auto field = ComputeAsteroidFieldDigest(GetAsteroidField()), fleet = ComputeSpaceVehicleDigest(), orders = ComputeSpaceOrdersDigest();
		const auto tick = TimerGameTick::counter; CHECK(window->left >= 0); CHECK(window->top >= GetMainViewTop());
		CHECK(window->left + window->width <= _screen.width); CHECK(window->top + window->height <= GetMainViewBottom());
		// A visible rectangle must remain a native hit target even when the
		// command deliberately disables it for this actor/instruction.
		for (const auto id : choices) {
			const auto rect = window->GetWidget<NWidgetBase>(id)->GetCurrentRect();
			CHECK(GetWidgetFromPos(window, (rect.left + rect.right) / 2, (rect.top + rect.bottom) / 2) == id);
			CHECK(window->top + rect.bottom < GetMainViewBottom());
		}
		auto *scroll = window->GetScrollbar(WID_SS_SCROLLBAR); scroll->SetPosition(0);
		CaptureSpaceFixesWindow(*window, std::filesystem::path(output) / fmt::format("{}-top.ppm", name));
		scroll->SetPosition(scroll->GetCount()); CaptureSpaceFixesWindow(*window, std::filesystem::path(output) / fmt::format("{}-bottom.ppm", name)); scroll->SetPosition(0);
		CHECK(ComputeAsteroidFieldDigest(GetAsteroidField()) == field); CHECK(ComputeSpaceVehicleDigest() == fleet);
		CHECK(ComputeSpaceOrdersDigest() == orders); CHECK(TimerGameTick::counter == tick);
	};
	if (scenario == "unsupported") {
		for (const auto id : choices) CHECK(window->IsWidgetDisabled(id));
		const auto fleet = ComputeSpaceVehicleDigest(), orders = ComputeSpaceOrdersDigest(); ClickSpaceFixesWidget(*window, WID_SS_POLICY_FULL);
		CHECK(ComputeSpaceVehicleDigest() == fleet); CHECK(ComputeSpaceOrdersDigest() == orders); capture("policy-unsupported");
		fmt::print("MINING_CONTROLS unsupported_role=true original_actor=true disabled_input_inert=true\n"); return;
	}
	REQUIRE((scenario == "edit" || scenario == "edit-light"));
	// Establish the existing Mine through actual UI commands when this old
	// retained mission has no current typed instruction. This explicitly restarts
	// its queue under the existing tooltip contract; policy edits below do not.
	if (selected->space_orders.cursor >= selected->space_orders.entries.size() || selected->space_orders.entries[selected->space_orders.cursor].action != SpaceOrderAction::Mine) {
		if (!selected->space_orders.entries.empty()) ClickSpaceFixesWidget(*window, WID_SS_CLEAR);
		REQUIRE_FALSE(window->IsWidgetDisabled(WID_SS_MINE)); ClickSpaceFixesWidget(*window, WID_SS_MINE);
	}
	REQUIRE_FALSE(selected->vehstatus.Test(VehState::Stopped)); uint32_t opportunities = 0;
	// Real ordinary pursuit/extraction earns a partial holding and partial next
	// interval. No cargo, position, lease, started flag or work count is assigned.
	while (!selected->space_orders.started || selected->mission.phase != SpaceMissionPhase::Mining || selected->cargo.TotalCount() == 0 || selected->mission.work_ticks == 0) {
		REQUIRE(opportunities++ < 3000); RunSpaceLifecycleGameOpportunity(); REQUIRE(ResolveSpaceCraft(reference) == selected);
	}
	REQUIRE(selected->cargo.TotalCount() < selected->cargo_cap); ShowSpaceSurveyWindow(reference);
	const auto index = selected->space_orders.cursor; REQUIRE(index < selected->space_orders.entries.size()); REQUIRE(selected->space_orders.entries[index].action == SpaceOrderAction::Mine);
	// Each real press/release posts the registered checked command. Compare
	// complete original queue fields apart from that one saved policy, while
	// retaining native cargo/work/pose/cost and every original resource/RNG word.
	for (uint8_t value = 0; value < choices.size(); ++value) {
		const auto policy = static_cast<SpaceMiningPolicy>(value); const auto before = selected->space_orders; auto expected = before; expected.entries[index].mining_policy = policy;
		const auto mission = selected->mission; const auto point = selected->position, home = selected->home; const auto status = selected->vehstatus;
		const auto cargo = ObserveMiningControlCargo(*selected); const auto field = GetAsteroidField(); const auto tick = TimerGameTick::counter; const auto money = Company::Get(selected->owner)->money;
		const auto random = std::array{_random.state[0], _random.state[1], _interactive_random.state[0], _interactive_random.state[1]};
		REQUIRE_FALSE(window->IsWidgetDisabled(choices[value])); ClickSpaceFixesWidget(*window, choices[value]);
		CHECK(selected->space_orders == expected); CHECK(selected->mission.target == mission.target); CHECK(selected->mission.phase == mission.phase);
		CHECK(selected->mission.approach_ticks_left == mission.approach_ticks_left); CHECK(selected->mission.work_ticks == mission.work_ticks);
		CHECK(selected->mission.travel_remainder == mission.travel_remainder); CHECK(selected->mission.cost_remainder == mission.cost_remainder);
		CHECK(selected->position == point); CHECK(selected->home == home); CHECK(selected->vehstatus == status); CHECK(ObserveMiningControlCargo(*selected) == cargo);
		CHECK(GetAsteroidField() == field); CHECK(TimerGameTick::counter == tick); CHECK(Company::Get(selected->owner)->money == money);
		CHECK(std::array{_random.state[0], _random.state[1], _interactive_random.state[0], _interactive_random.state[1]} == random);
		for (uint8_t choice = 0; choice < choices.size(); ++choice) CHECK(window->IsWidgetLowered(choices[choice]) == (choice == value));
		capture(fmt::format("policy-choice-{}", value));
	}
	const auto saved_queue = selected->space_orders; const auto saved_fleet = ComputeSpaceVehicleDigest();
	CHECK(CmdSetSpaceMiningPolicy(DoCommandFlag::Execute, selected->index, selected->generation + 1, index, SpaceMiningPolicy::ReturnNearest).Failed());
	CHECK(selected->space_orders == saved_queue); CHECK(ComputeSpaceVehicleDigest() == saved_fleet);
	const auto saved = (std::filesystem::path(output) / "policy-current.sav").string();
	{
		AutoRestoreBackup paused(_pause_mode, PauseModes{PauseMode::Normal});
		REQUIRE(SaveOrLoad(saved, SaveLoadOperation::Save, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Ok);
		REQUIRE(SaveOrLoad(saved, SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Ok);
	}
	{
		// Native input requires its current UI actor to equal the local observer,
		// including a real spectator. Change neither the original hull's owner
		// nor any company record when exercising this foreign-view context.
		AutoRestoreBackup spectator(_local_company, COMPANY_SPECTATOR); AutoRestoreBackup actor(_current_company, COMPANY_SPECTATOR); ShowSpaceSurveyWindow(reference);
		for (const auto id : choices) CHECK(window->IsWidgetDisabled(id));
		ClickSpaceFixesWidget(*window, WID_SS_POLICY_RETURN); CHECK(selected->space_orders == saved_queue); CHECK(ComputeSpaceVehicleDigest() == saved_fleet); capture("policy-foreign-observer");
	}
	ShowSpaceSurveyWindow(reference); ClickSpaceFixesWidget(*window, WID_SS_CLEAR); ClickSpaceFixesWidget(*window, WID_SS_VISIT);
	REQUIRE_FALSE(selected->space_orders.entries.empty()); CHECK(selected->space_orders.entries.front().action == SpaceOrderAction::Visit);
	for (const auto id : choices) CHECK(window->IsWidgetDisabled(id));
	const auto visit = selected->space_orders; ClickSpaceFixesWidget(*window, WID_SS_POLICY_FULL); CHECK(selected->space_orders == visit); capture("policy-non-mine");
	fmt::print("MINING_CONTROLS model={} ordinary_opportunities={} actual_mouse_choices=3 saved_full=true work_cargo_pose_cost_preserved=true stale_generation_rejected=true foreign_non_mine_disabled=true\n", GetSpaceVehicleModel(*selected), opportunities);
}

/** @param owner Original observer company, whose actual balance participates. @return Synchronized next-decision owners: clock, field, fleet, orders, structures, manifests, pods, descents, launch centers, transitions, both global simulation RNG words and balance. Cosmetic interactive RNG is deliberately not a saved simulation prerequisite. */
static std::array<uint64_t, 13> ObserveMiningPolicyDecision(CompanyID owner)
{
	return {TimerGameTick::counter, ComputeAsteroidFieldDigest(GetAsteroidField()), ComputeSpaceVehicleDigest(), ComputeSpaceOrdersDigest(),
		ComputeSpaceStructureDigest(), ComputeSpaceManifestDigest(), ComputeSpacePodDigest(), ComputeSpaceDescentDigest(), ComputeLaunchCenterDigest(),
		ComputeSpaceTransitionDigest(), _random.state[0], _random.state[1], static_cast<uint64_t>(Company::Get(owner)->money)};
}

/** Project only the exact original attachment's last safe map-edge geometry.
 * @param craft Actual leased native miner; velocity, reserve, work, cargo, queue and native clocks remain unchanged. The actual next opportunity must release it and later retire the body.
 */
static void StageMiningControlEdge(SpaceVehicle &craft)
{
	REQUIRE(craft.mission.phase == SpaceMissionPhase::Mining); const auto *live = GetMutableAsteroidBody(craft.mission.target); REQUIRE(live);
	const auto original = *live; REQUIRE(original.mining_slot == craft.index); const auto bounds = *GetSpacePlaneBounds(Map::SizeX(), Map::SizeY());
	const auto exit = ForecastAsteroidExit(original, bounds); REQUIRE(exit);
	// Choose the pure map-edge component, preserving the original heading/carry.
	// This is declared geometry rather than elapsed travel through intervening
	// modules. Native safety still reads every actual static obstacle next tick.
	const auto attachment = ForecastSpaceAttachmentSafety(original.motion, *exit, craft.mission.attachment_offset, bounds, {});
	REQUIRE(attachment); REQUIRE(attachment->first_unsafe_tick); REQUIRE(*attachment->first_unsafe_tick != 0);
	const auto checkpoint = ProjectSpaceMotionStraight(original.motion, bounds, *attachment->first_unsafe_tick - 1); REQUIRE(checkpoint); REQUIRE_FALSE(checkpoint->crossed_edges.Any());
	auto field = GetAsteroidField(); REQUIRE(field.surveys.empty());
	{ auto *body = FindAsteroidBody(field, original.id); REQUIRE(body); body->motion = checkpoint->proposed; }
	REQUIRE(InstallAsteroidField(std::move(field))); UpdateAttachedSpaceCraftTransforms(); REQUIRE(bounds.Contains(craft.position));
	CHECK(GetMutableAsteroidBody(original.id)->motion.velocity_x == original.motion.velocity_x); CHECK(GetMutableAsteroidBody(original.id)->motion.velocity_y == original.motion.velocity_y);
}

/** Follow an actual saved Full Load partial holding across native edge departure, another real source and nearest original receiving work.
 * @param output Private directory receiving native phase captures, actual saves and the next-opportunity replay oracle. Only edge geometry is projected; no cargo, reserve, arrival, quota, paid asset or simulation time is assigned.
 * @param model Original supported mining role to resolve from this actual saved world;21 preserves the existing heavy scenario and15 uses its own native holding/allowance rules.
 * @param full_hold True consumes an actual saved second-source attachment and earns its full native hold/automatic return; false retains the partial edge/explicit-policy scenario.
 */
static void VerifySpaceMiningPolicyContinuation(const char *output, uint16_t model = SPACE_HEAVY_MINER_MODEL, bool full_hold = false)
{
	SpaceVehicle *craft = nullptr;
	// The input is the preceding actual role's policy-current.sav, not a newly
	// commissioned substitute. Preserve its original incarnation and paid owners.
	for (auto *candidate : SpaceVehicle::Iterate()) {
		// Resolve this original role without borrowing another incarnation's
		// cargo, manifest or finite extraction contract.
		if (candidate->owner == _local_company && candidate->construction.placement == SpacePlacement::Orbital && GetSpaceVehicleModel(*candidate) == model) { craft = candidate; break; }
	}
	REQUIRE(craft); REQUIRE(GetSpaceMiningTuning(model)); REQUIRE_FALSE(craft->vehstatus.Test(VehState::Stopped));
	if (model == SPACE_HEAVY_MINER_MODEL) { REQUIRE(craft->manifest); REQUIRE(craft->manifest->returns_remaining != 0); }
	const auto reference = GetSpaceCraftReference(*craft); const auto index = craft->space_orders.cursor;
	REQUIRE(index < craft->space_orders.entries.size()); REQUIRE(craft->space_orders.entries[index].action == SpaceOrderAction::Mine);
	REQUIRE(craft->space_orders.entries[index].mining_policy == SpaceMiningPolicy::FullLoadNearest); REQUIRE(craft->space_orders.started);
	REQUIRE(craft->cargo.TotalCount() == GetSpaceMiningTuning(model)->extraction_units);
	// Partial-edge inputs retain real incomplete work; the full-hold case starts
	// at the preceding genuine new attachment, before its first work opportunity.
	if (!full_hold) REQUIRE(craft->mission.work_ticks != 0);
	REQUIRE(BlitterFactory::SelectBlitter("8bpp-simple") != nullptr); GfxClearSpriteCache(); FontCache::ClearFontCaches(FONTSIZES_REQUIRED);
	AutoRestoreBackup running(_pause_mode, PauseModes{}); AutoRestoreBackup shift(_shift_pressed, false); AutoRestoreBackup control(_ctrl_pressed, false);
	ShowSpaceSurveyWindow(reference); auto *window = FindWindowById(WindowClass::SpaceSurvey, 0); REQUIRE(window);
	std::ofstream expected(std::filesystem::path(output) / "policy-next.tsv"); REQUIRE(expected.good());
	expected << "# Owner: r2sff1-space-fixes-features-1.md Groups30B/31B; one actual next opportunity per saved phase,13 synchronized scalar observations\n";
	std::optional<std::string> pending; uint32_t opportunities = 0;
	/** Advance one ordinary opportunity and record only an already saved phase's actual next result; this observer owns no simulation counter or decision. */
	auto opportunity = [&] {
		INFO("policy continuation opportunities=" << opportunities << " phase=" << unsigned{to_underlying(craft->mission.continuation.phase)} << " wait=" << unsigned{to_underlying(craft->mission.continuation.waiting)} << " body=" << craft->mission.target.base());
		REQUIRE(opportunities++ < 5000); RunSpaceLifecycleGameOpportunity(); REQUIRE(ResolveSpaceCraft(reference) == craft);
		if (pending) {
			expected << *pending;
			for (const auto value : ObserveMiningPolicyDecision(craft->owner)) expected << '\t' << value;
			expected << '\n'; expected.flush(); REQUIRE(expected.good()); pending.reset();
		}
	};
	/** @param name Explicit native phase name. Save actual current owners, capture the bounded inspector, and await the next genuine opportunity for the replay oracle. */
	auto checkpoint = [&](std::string name) {
		REQUIRE_FALSE(pending); ShowSpaceSurveyWindow(reference);
		const auto before = ObserveMiningPolicyDecision(craft->owner);
		auto *scroll = window->GetScrollbar(WID_SS_SCROLLBAR); scroll->SetPosition(0);
		CaptureSpaceFixesWindow(*window, std::filesystem::path(output) / (name + "-top.ppm"));
		scroll->SetPosition(scroll->GetCount()); CaptureSpaceFixesWindow(*window, std::filesystem::path(output) / (name + "-bottom.ppm")); scroll->SetPosition(0);
		const auto saved = (std::filesystem::path(output) / (name + ".sav")).string();
		{
			AutoRestoreBackup paused(_pause_mode, PauseModes{PauseMode::Normal});
			REQUIRE(SaveOrLoad(saved, SaveLoadOperation::Save, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Ok);
			REQUIRE(SaveOrLoad(saved, SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Ok);
		}
		CHECK(ObserveMiningPolicyDecision(craft->owner) == before); pending = std::move(name);
	};
	const auto first = OrbitalBodyID{craft->space_orders.entries[index].target.id}; const auto units = craft->cargo.TotalCount();
	if (full_hold) {
		// Only the genuine preceding second-source save supplies this route and
		// attachment. No queue edit, projected geometry or physical arrival is
		// needed to make an actual full hold choose its original receiver domain.
		REQUIRE(craft->mission.phase == SpaceMissionPhase::Mining); REQUIRE(craft->mission.target != first);
		REQUIRE(craft->space_orders.entries.size() == size_t{index} + 2);
		REQUIRE(craft->space_orders.entries[index + 1].action == SpaceOrderAction::Unload);
		REQUIRE(craft->space_orders.entries[index + 1].target.kind == SpaceOrderTargetKind::Structure);
	} else {
		// Nearest policy preserves its declared destination domain. Establish this
		// case's explicit original module Unload rather than assuming an implicit
		// Surface route or another role's shells authorize a Dock return.
		// Like every real queue edit this restarts incomplete work. Earn subsequent
		// attachment/work through ordinary opportunities before claiming preservation.
		SpaceStructure *route_site = nullptr;
		for (auto *site : SpaceStructure::Iterate()) {
			// Only an existing owned operational mineral receiver can establish the
			// instruction. Actual nearest/arrival admission still rechecks capacity.
			if (site->state.owner == craft->owner && IsOrbitalModuleOperational(*site) && GetOrbitalModuleCargoCapacity(*site, craft->cargo_type) != 0) { route_site = site; break; }
		}
		REQUIRE(route_site); REQUIRE(craft->space_orders.entries.size() == size_t{index} + 1);
		const auto route_cargo = ObserveMiningControlCargo(*craft); const auto route_point = craft->position;
		REQUIRE(CmdEditSpaceOrder(DoCommandFlag::Execute, craft->index, craft->generation, SpaceOrderEdit::Insert,
			static_cast<uint8_t>(craft->space_orders.entries.size()), SpaceOrderAction::Unload, SpaceOrderTargetKind::Structure,
			route_site->index.base(), route_site->generation).Succeeded());
		CHECK(ObserveMiningControlCargo(*craft) == route_cargo); CHECK(craft->position == route_point);
		while (craft->mission.phase != SpaceMissionPhase::Mining || craft->mission.work_ticks == 0) opportunity();
		REQUIRE(craft->mission.target == first); const auto first_cargo = ObserveMiningControlCargo(*craft);
		StageMiningControlEdge(*craft); checkpoint("policy-before-departure"); opportunity();
		CHECK(craft->mission.work_ticks == 0); CHECK(ObserveMiningControlCargo(*craft) == first_cargo); CHECK(craft->space_orders.cursor == index);
		CHECK(craft->mission.continuation.previous_body == first); checkpoint("policy-after-departure");
		// Actual ordinary local movement/pursuit must earn another live same-mineral
		// attachment while the original Mine and its original cargo remain held.
		while (craft->mission.phase != SpaceMissionPhase::Mining || craft->mission.target == first) opportunity();
	}
	const auto second = craft->mission.target; const auto *second_body = GetMutableAsteroidBody(second); REQUIRE(second_body); REQUIRE(second_body->mining_slot == craft->index);
	const auto reserve_before = second_body->remaining_reserve; const auto extracted_before = craft->extracted_units;
	CHECK(CargoSpec::Get(craft->cargo_type)->label == craft->mission.continuation.mineral); CHECK(craft->space_orders.cursor == index);
	CHECK(craft->cargo.TotalCount() == units); checkpoint("policy-retarget");
	// Earn a real second-source batch, then test the configured partial return.
	// A slow diagonal source may remain live long after its attachment becomes
	// unsafe; waiting for its centre now could legitimately fill the small hold.
	// Its actual later retirement is checked after the original unload instead.
	if (full_hold) {
		REQUIRE(reserve_before >= craft->cargo_cap - units);
		// Earn every additional batch through the original74-opportunity owner.
		// The loop stops before a later return can empty the just-filled holding.
		while (craft->cargo.TotalCount() < craft->cargo_cap) opportunity();
		REQUIRE(craft->cargo.TotalCount() == craft->cargo_cap);
		CHECK(craft->extracted_units - extracted_before == craft->cargo_cap - units);
		const auto *source = GetMutableAsteroidBody(second); REQUIRE(source);
		CHECK(source->remaining_reserve + craft->cargo_cap - units == reserve_before);
		checkpoint("policy-full-hold");
	} else {
		while (craft->cargo.TotalCount() <= units) opportunity();
		REQUIRE(craft->cargo.TotalCount() < craft->cargo_cap);
	}
	const auto holding = ObserveMiningControlCargo(*craft); const auto cargo_type = craft->cargo_type;
	std::vector<std::pair<Source, uint32_t>> origins;
	// Aggregate only copied source quantities for later custody comparison.
	// The original packets remain solely in the actual native holding.
	for (const auto &[packet, source, quantity] : holding) {
		const auto origin = source.GetAsteroidOrigin(); REQUIRE(origin); CHECK((origin->body == first.base() || origin->body == second.base()));
		/** @param item Previous copied source total. @return Whether the complete immutable original source, including full body/mineral, agrees. */
		const auto found = std::ranges::find_if(origins, [&](const auto &item) { return item.first == source; });
		if (found == origins.end()) origins.emplace_back(source, quantity); else found->second += quantity;
	}
	REQUIRE(origins.size() == 2);
	if (!full_hold) {
		ShowSpaceSurveyWindow(reference); REQUIRE_FALSE(window->IsWidgetDisabled(WID_SS_POLICY_RETURN));
		const auto point = craft->position; const auto work = craft->mission.work_ticks; const auto cost = craft->mission.cost_remainder;
		ClickSpaceFixesWidget(*window, WID_SS_POLICY_RETURN); REQUIRE(craft->space_orders.entries[index].mining_policy == SpaceMiningPolicy::ReturnNearest);
		CHECK(craft->position == point); CHECK(craft->mission.work_ticks == work); CHECK(craft->mission.cost_remainder == cost); CHECK(ObserveMiningControlCargo(*craft) == holding);
		StageMiningControlEdge(*craft); opportunity();
	}
	std::optional<SpaceStructureRef> receiver; std::vector<uint32_t> before_stock; uint8_t berth = UINT8_MAX;
	const auto allowance = craft->manifest ? craft->manifest->returns_remaining : uint8_t{0};
	// Native continuation must select, travel, acquire a real berth and invoke
	// original receiving work. The test never calls that operation or assigns a
	// Holding pose; a visible genuine wait is a failing unmet return precondition.
	while (craft->cargo.TotalCount() != 0) {
		const auto target = craft->mission.continuation.effective_target;
		// Record the real receiver once native continuation has chosen its exact
		// Structure incarnation; unrelated phases cannot supply a stock owner.
		if (!receiver && craft->mission.continuation.phase == MiningContinuationPhase::ReturnCargo && target && target->kind == SpaceOrderTargetKind::Structure) {
			const auto nearest = FindNearestEligibleSpaceReturn(*craft, SpaceReturnPurpose::Cargo); REQUIRE(nearest); CHECK(nearest->target == *target);
			receiver = SpaceStructureRef{SpaceStructureID{static_cast<uint16_t>(target->id)}, target->generation}; const auto *site = ResolveSpaceStructure(*receiver); REQUIRE(site); REQUIRE(site->module);
			// Preserve existing same-origin module stock as well as incoming
			// freight; native packet merging may legitimately retire a fragment ID.
			for (const auto &[source, quantity] : origins) {
				uint32_t total = 0;
				// Station stock groups original packets by next hop. Count the
				// complete source across every real group without moving custody.
				for (const auto &[hop, packets] : *site->module->stock[cargo_type].Packets()) for (const auto *packet : packets) if (packet->GetSource() == source) total += packet->Count();
				before_stock.push_back(total);
			}
			checkpoint("policy-return-travel");
		}
		// Only actual approach admission can establish the arrival slot. Keep
		// its identity across the transfer that legitimately releases the berth.
		if (receiver && craft->space_orders.approach && craft->space_orders.approach->slot != UINT8_MAX) berth = craft->space_orders.approach->slot;
		opportunity();
	}
	REQUIRE(receiver); const auto *site = ResolveSpaceStructure(*receiver); REQUIRE(site); REQUIRE(site->module); REQUIRE(berth != UINT8_MAX);
	const auto arrival = GetSpaceStructureApproachSlot(*site, berth); REQUIRE(arrival); CHECK(craft->position == *arrival);
	// Original heavy positive module handling spends one return. A legacy15
	// module unload cannot inherit that heavy-only charge or receive a manifest.
	if (model == SPACE_HEAVY_MINER_MODEL) CHECK(craft->manifest->returns_remaining + 1 == allowance);
	else CHECK((craft->manifest ? craft->manifest->returns_remaining : uint8_t{0}) == allowance);
	CHECK(craft->cargo_cap == GetSpaceFreightCapacity(model, cargo_type)); CHECK(craft->space_orders.cursor > index);
	// The sole original module inventory now owns each complete historical
	// source quantity. Neither a retagged body nor a zero-cost parallel stock can
	// satisfy the original physical packet move/quota receipt.
	for (size_t origin = 0; origin < origins.size(); ++origin) {
		uint32_t total = 0;
		// Inspect every real next-hop list because packet transfer may merge
		// fragments while retaining their full immutable production source.
		for (const auto &[hop, packets] : *site->module->stock[cargo_type].Packets()) for (const auto *packet : packets) if (packet->GetSource() == origins[origin].first) total += packet->Count();
		CHECK(total == before_stock[origin] + origins[origin].second);
	}
	// Both original centres must eventually cross the real edge through ordinary
	// movement. No manual retirement, altered heading or live tombstone stands in
	// for their genuine terminal events and historical packet provenance.
	// The partial case projected this terminal geometry. A full-hold source may
	// properly remain live after unloading, so it must not be artificially retired.
	if (!full_hold) while (GetMutableAsteroidBody(first) != nullptr || GetMutableAsteroidBody(second) != nullptr) opportunity();
	checkpoint("policy-after-unload"); opportunity(); CHECK_FALSE(pending);
	fmt::print("MINING_POLICY_CONTINUATION model={} ordinary_opportunities={} first={} second={} real_receiver={} real_berth={} distinct_origins=2 native_quota_spent={} full_hold={} declared_edge_geometry={} synthetic_arrival=false\n", model, opportunities, first.base(), second.base(), receiver->structure.base(), berth, model == SPACE_HEAVY_MINER_MODEL ? 1 : 0, full_hold, !full_hold);
}

/** Exercise the original15's implicit Surface return and actual paid-pod descent.
 * @param output Private phase saves/captures and uninterrupted next-opportunity oracle. A retained original paid manifest/shell supplies admission; no holding, allowance, arrival or descent clock is assigned.
 */
static void VerifySpaceMiningSurfaceReturn(const char *output)
{
	SpaceVehicle *craft = nullptr;
	// Choose the deployed original15 from the actual input application. Another
	// role, stored hull or fabricated manifest cannot supply Surface return rights.
	for (auto *candidate : SpaceVehicle::Iterate()) {
		if (candidate->owner == _local_company && candidate->construction.placement == SpacePlacement::Orbital && GetSpaceVehicleModel(*candidate) == SPACE_PROBE_MODEL) { craft = candidate; break; }
	}
	REQUIRE(craft); REQUIRE(craft->manifest); REQUIRE_FALSE(craft->manifest->pods.empty());
	REQUIRE(craft->manifest->returns_remaining > craft->manifest->pending_returns);
	REQUIRE(craft->mission.phase == SpaceMissionPhase::Mining); REQUIRE(craft->cargo.TotalCount() == GetSpaceMiningTuning(SPACE_PROBE_MODEL)->extraction_units);
	const auto reference = GetSpaceCraftReference(*craft); const auto index = craft->space_orders.cursor;
	REQUIRE(index < craft->space_orders.entries.size()); REQUIRE(craft->space_orders.entries[index].action == SpaceOrderAction::Mine);
	REQUIRE(craft->space_orders.entries[index].mining_policy == SpaceMiningPolicy::FullLoadNearest); REQUIRE(craft->space_orders.started);
	const auto original = ObserveMiningControlCargo(*craft); REQUIRE_FALSE(original.empty());
	const auto origin = std::get<1>(original.front()); const auto units = craft->cargo.TotalCount(); const auto cargo = craft->cargo_type;
	const auto allowance = craft->manifest->returns_remaining, pending_before = craft->manifest->pending_returns;
	const auto carried_before = craft->manifest->pods.size(); uint32_t opportunities = 0;
	REQUIRE(BlitterFactory::SelectBlitter("8bpp-simple") != nullptr); GfxClearSpriteCache(); FontCache::ClearFontCaches(FONTSIZES_REQUIRED);
	AutoRestoreBackup running(_pause_mode, PauseModes{}); AutoRestoreBackup shift(_shift_pressed, false); AutoRestoreBackup control(_ctrl_pressed, false);
	ShowSpaceSurveyWindow(reference); auto *window = FindWindowById(WindowClass::SpaceSurvey, 0); REQUIRE(window);
	std::ofstream expected(std::filesystem::path(output) / "policy-next.tsv"); REQUIRE(expected.good());
	expected << "# Owner: r2sff1-space-fixes-features-1.md Group31B; original15 Surface next opportunities,13 synchronized owner observations\n";
	std::optional<std::string> next;
	/** Offer one genuine normal opportunity; record only the already saved boundary's next original-owner result. */
	auto opportunity = [&] {
		REQUIRE(opportunities++ < 5000); RunSpaceLifecycleGameOpportunity(); REQUIRE(ResolveSpaceCraft(reference) == craft);
		if (next) {
			expected << *next;
			for (const auto value : ObserveMiningPolicyDecision(craft->owner)) expected << '\t' << value;
			expected << '\n'; expected.flush(); REQUIRE(expected.good()); next.reset();
		}
	};
	/** @param phase Actual native state being saved. Capture top/final row and preserve complete original gameplay owners across native save/preview. */
	auto checkpoint = [&](std::string phase) {
		REQUIRE_FALSE(next); const auto before = ObserveMiningPolicyDecision(craft->owner); ShowSpaceSurveyWindow(reference);
		auto *scroll = window->GetScrollbar(WID_SS_SCROLLBAR); scroll->SetPosition(0);
		CaptureSpaceFixesWindow(*window, std::filesystem::path(output) / (phase + "-top.ppm"));
		scroll->SetPosition(scroll->GetCount()); CaptureSpaceFixesWindow(*window, std::filesystem::path(output) / (phase + "-bottom.ppm")); scroll->SetPosition(0);
		const auto saved = (std::filesystem::path(output) / (phase + ".sav")).string();
		{
			AutoRestoreBackup paused(_pause_mode, PauseModes{PauseMode::Normal});
			REQUIRE(SaveOrLoad(saved, SaveLoadOperation::Save, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Ok);
			REQUIRE(SaveOrLoad(saved, SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Ok);
		}
		CHECK(ObserveMiningPolicyDecision(craft->owner) == before); next = std::move(phase);
	};
	ClickSpaceFixesWidget(*window, WID_SS_POLICY_RETURN); REQUIRE(craft->space_orders.entries[index].mining_policy == SpaceMiningPolicy::ReturnNearest);
	CHECK(ObserveMiningControlCargo(*craft) == original);
	const auto route = GetSpaceReturnRoute(*craft); REQUIRE(route); REQUIRE(route->domain == SpaceOrderTargetKind::LaunchCenter);
	StageMiningControlEdge(*craft); checkpoint("policy-surface-before-departure");
	std::optional<SpaceReturnCandidate> receiver;
	// Observe navigation-only admission before the native arrival opportunity.
	// A genuine in-circle receiver can complete immediately; do not require a
	// synthetic distant travel state merely to make its target observable.
	while (craft->cargo.TotalCount() != 0) {
		if (!receiver && craft->mission.continuation.phase == MiningContinuationPhase::ReturnCargo) {
			const auto before = ObserveMiningPolicyDecision(craft->owner); receiver = FindNearestEligibleSpaceReturn(*craft, SpaceReturnPurpose::Cargo);
			REQUIRE(receiver); CHECK(ObserveMiningPolicyDecision(craft->owner) == before);
			REQUIRE(receiver->target.kind == SpaceOrderTargetKind::LaunchCenter); REQUIRE(receiver->quantity == units);
			const auto *shell = ResolveSpacePod(receiver->pod); REQUIRE(shell); REQUIRE(shell->state.capital_paid != 0);
			REQUIRE(shell->state.custody == SpacePodCustody::Craft); REQUIRE(shell->state.craft == reference);
			checkpoint("policy-surface-before-release");
		}
		opportunity();
	}
	REQUIRE(receiver); const auto shell_ref = receiver->pod;
	const LaunchCenterRef center_ref{StationID{static_cast<uint16_t>(receiver->target.id)}, receiver->target.generation};
	const auto *shell = ResolveSpacePod(shell_ref); REQUIRE(shell); REQUIRE(shell->descent); const auto &job = *shell->descent;
	CHECK(job.center == center_ref); CHECK(job.sender == craft->index); CHECK(job.sender_generation == craft->generation);
	CHECK(job.sender_model == SPACE_PROBE_MODEL); CHECK(job.cargo == cargo); CHECK(job.quantity == units);
	CHECK(job.release_point == craft->position); CHECK(IsInsideLaunchCenterDropZone(job.release_point, receiver->anchor));
	CHECK(craft->manifest->returns_remaining == allowance); CHECK(craft->manifest->pending_returns == pending_before + 1);
	CHECK(craft->manifest->pods.size() + 1 == carried_before); CHECK(craft->space_orders.cursor > index);
	uint32_t physical = 0;
	// The pod's sole real packet container must hold exactly this original
	// source quantity; navigation/arrival may neither duplicate nor retag it.
	for (const auto &[hop, packets] : *job.packets.Packets()) for (const auto *packet : packets) {
		CHECK(packet->GetSource() == origin); physical += packet->Count();
	}
	CHECK(physical == units); CHECK(job.blocked == SpaceDescentBlock::None);
	// Normal event order releases during Vehicle ticks, then earns the first
	// descent opportunity later in that same StateGameLoop. Count that real
	// opportunity once rather than resetting its already advanced native timer.
	const auto duration = GetMultiLayerSpaceBudget().descent_ticks; CHECK(job.remaining_ticks + 1 == duration);
	checkpoint("policy-surface-descent"); uint32_t descent_opportunities = 1;
	/** @return Actual full-origin quantity in the original receiver's next-hop lists; absence allocates no stock owner. */
	auto received = [&]() -> uint32_t {
		const auto *center = ResolveLaunchCenter(center_ref); REQUIRE(center);
		if (!center->goods[cargo].HasData()) return 0;
		uint32_t total = 0;
		// Sum existing complete-source fragments across original routing groups;
		// ordinary native freight may legitimately merge compatible fragments.
		for (const auto &[hop, packets] : *center->goods[cargo].GetData().cargo.Packets()) for (const auto *packet : packets) if (packet->GetSource() == origin) total += packet->Count();
		return total;
	};
	// Only actual ordinary native descent ticks may settle the paid shell and
	// pending claim. No direct TickSpaceDescents call or completed job is assigned.
	while (const auto *current = ResolveSpacePod(shell_ref)) {
		REQUIRE(current->descent); REQUIRE(current->descent->blocked == SpaceDescentBlock::None);
		const auto remaining = current->descent->remaining_ticks; const auto before_stock = received();
		opportunity(); ++descent_opportunities;
		if (const auto *after = ResolveSpacePod(shell_ref)) {
			REQUIRE(after->descent); CHECK(after->descent->remaining_ticks + 1 == remaining);
			CHECK(craft->manifest->returns_remaining == allowance); CHECK(craft->manifest->pending_returns == pending_before + 1);
		} else {
			CHECK(remaining == 1); CHECK(received() == before_stock + units);
		}
	}
	CHECK(descent_opportunities == duration); CHECK(craft->manifest->returns_remaining + 1 == allowance);
	CHECK(craft->manifest->pending_returns == pending_before); CHECK(craft->cargo.TotalCount() == 0);
	checkpoint("policy-surface-settled"); opportunity(); CHECK_FALSE(next);
	fmt::print("MINING_SURFACE_RETURN original_model=15 ordinary_opportunities={} actual_units={} paid_shell={} receiver={} native_descent_opportunities={} immutable_source_preserved=true native_quota_spent=1 synthetic_arrival=false\n", opportunities, units, shell_ref.pod.base(), center_ref.station.base(), descent_opportunities);
}

/** Inspect actual receiver/actor preconditions without granting stock or offering a simulation opportunity.
 * @return Nothing; assertions prove all13 original owner observations remain unchanged while stdout records the retained world's native stock, capacities and finite mineral reserves.
 */
static void ObserveMiningReceiverPreconditions()
{
	const auto before = ObserveMiningPolicyDecision(_local_company);
	const auto mineral = GetCargoTypeByLabel(CargoLabel{"NORE"}); REQUIRE(IsValidCargoType(mineral));
	// Enumerate every retained site, including inactive frames. A nearby record
	// alone must not be mistaken for usable general freight capacity.
	for (const auto *site : SpaceStructure::Iterate()) {
		fmt::print("RECEIVER_INVENTORY id={} generation={} kind={} phase={} owner={} operational={} capacity={} stock={} removal_block={} x={} y={}\n",
			site->index.base(), site->generation, to_underlying(site->state.kind), to_underlying(site->state.phase), site->state.owner.base(),
			IsOrbitalModuleOperational(*site), GetOrbitalModuleCargoCapacity(*site, mineral), site->module ? site->module->stock[mineral].TotalCount() : 0,
			to_underlying(ClassifyOrbitalModuleRemoval(*site, _local_company)), site->state.x, site->state.y);
		// Read every actual nonempty label before choosing an activation or
		// evacuation precondition. A declared frame supplies no spare parts.
		if (site->module) for (CargoType cargo : EnumRange(NUM_CARGO)) {
			if (site->module->stock[cargo].TotalCount() == 0) continue;
			fmt::print("RECEIVER_STOCK id={} cargo={} units={}\n", site->index.base(), cargo, site->module->stock[cargo].TotalCount());
		}
	}
	// Original hull identity, holding and paid allowance determine which native
	// preparation route is legal; no other model or free manifest can substitute.
	for (const auto *craft : SpaceVehicle::Iterate()) {
		fmt::print("RECEIVER_ACTOR id={} generation={} model={} owner={} placement={} phase={} units={} allowance={} pending={} queue_cursor={} queue_size={}\n",
			craft->index.base(), craft->generation, GetSpaceVehicleModel(*craft), craft->owner.base(), to_underlying(craft->construction.placement),
			to_underlying(craft->mission.phase), craft->cargo.TotalCount(), craft->manifest ? craft->manifest->returns_remaining : 0,
			craft->manifest ? craft->manifest->pending_returns : 0, craft->space_orders.cursor, craft->space_orders.entries.size());
		if (craft->manifest) fmt::print("SERVICE_INVENTORY actor={} content_version={} services_completed={} remaining_work={} pods={} RFMT={} ELEC={} mining_block={}\n",
			craft->index.base(), craft->construction.content_version, craft->manifest->services_completed, craft->manifest->service_ticks,
			craft->manifest->pods.size(), craft->manifest->supplies[1].TotalCount(), craft->manifest->supplies[2].TotalCount(), to_underlying(GetSpaceMiningBlock(*craft)));
	}
	// Existing station stock is the only permissible Earth procurement source.
	// Report actual parts and construction labels without creating Goods entries
	// or assuming a nearby station has an operational launch-center identity.
	for (const auto *center : Station::Iterate()) {
		if (!center->launch_center) continue;
		for (const auto label : {CargoLabel{"RFMT"}, CargoLabel{"ELEC"}, CargoLabel{"STEL"}, CargoLabel{"GOOD"}}) {
			const auto cargo = GetCargoTypeByLabel(label); REQUIRE(IsValidCargoType(cargo));
			fmt::print("SERVICE_STATION station={} owner={} cargo={} units={}\n", center->index.base(), center->owner.base(), cargo, center->goods[cargo].HasData() ? center->goods[cargo].GetData().cargo.TotalCount() : 0);
		}
	}
	// Finite source quantities bound preparation. Reporting them never converts
	// size-based renewal credit into cargo or replenishes a depleted reserve.
	for (const auto &body : GetAsteroidField().bodies) fmt::print("RECEIVER_MINERAL body={} class={} reserve={}\n", body.id.base(), to_underlying(body.resource_class), body.remaining_reserve);
	CHECK(ObserveMiningPolicyDecision(_local_company) == before);
}

/** Prepare a real whole-batch receiver refusal with original finite extraction and native module transfers.
 * @param output Private current preparation checkpoint, captures and final saved next-opportunity oracle. Each process offers at most4000 ordinary opportunities; later processes resume its actual save without assigning stock, cargo, geometry, clocks, allowance or work.
 */
static void PrepareMiningReceiverRefusal(const char *output)
{
	SpaceVehicle *craft = nullptr; SpaceStructure *receiver = nullptr;
	// The original small miner's module return does not spend the heavy-only
	// allowance. This makes repeated paid-world transfers legal without adding
	// service stock, free shells or a second quota owner during preparation.
	for (auto *candidate : SpaceVehicle::Iterate()) {
		if (candidate->owner == _local_company && candidate->construction.placement == SpacePlacement::Orbital && GetSpaceVehicleModel(*candidate) == SPACE_PROBE_MODEL) { craft = candidate; break; }
	}
	REQUIRE(craft); const auto reference = GetSpaceCraftReference(*craft);
	const auto mineral = GetCargoTypeByLabel(CargoLabel{"NORE"}); REQUIRE(IsValidCargoType(mineral));
	for (auto *site : SpaceStructure::Iterate()) {
		if (site->state.owner == craft->owner && IsOrbitalModuleOperational(*site) && GetOrbitalModuleCargoCapacity(*site, mineral) != 0) { receiver = site; break; }
	}
	REQUIRE(receiver); REQUIRE(receiver->module); REQUIRE(craft->manifest);
	const auto receiver_ref = GetSpaceStructureReference(*receiver); const auto allowance = craft->manifest->returns_remaining;
	const auto initial_stock = receiver->module->stock[mineral].TotalCount(); const auto capacity = GetOrbitalModuleCargoCapacity(*receiver, mineral);
	const auto initial_holding = craft->cargo.TotalCount(); const auto initial_extracted = craft->extracted_units;
	AutoRestoreBackup actor(_current_company, craft->owner); AutoRestoreBackup running(_pause_mode, PauseModes{});
	// Explicitly stop other deployed actors through the original command before
	// the long preparation. Their native safety/upkeep still run; this preserves
	// the original heavy's paid allowance instead of silently spending its return.
	for (auto *other : SpaceVehicle::Iterate()) {
		if (other != craft && other->owner == craft->owner && other->construction.placement == SpacePlacement::Orbital && !other->vehstatus.Test(VehState::Stopped)) {
			REQUIRE(CmdStartStopVehicle(DoCommandFlag::Execute, other->index, true).Succeeded());
		}
	}
	/** Insert a real receiver Visit followed by original handling; admission still owns physical arrival and whole stock capacity. */
	auto append_unload = [&] {
		// Visit the original Core before handling. Even when its receiving buffer
		// later fills, this native navigation retains a real reached source berth;
		// any newly activated alternative will then be demonstrably farther away.
		REQUIRE(CmdEditSpaceOrder(DoCommandFlag::Execute, craft->index, craft->generation, SpaceOrderEdit::Insert,
			static_cast<uint8_t>(craft->space_orders.entries.size()), SpaceOrderAction::Visit, SpaceOrderTargetKind::Structure,
			receiver_ref.structure.base(), receiver_ref.generation).Succeeded());
		REQUIRE(CmdEditSpaceOrder(DoCommandFlag::Execute, craft->index, craft->generation, SpaceOrderEdit::Insert,
			static_cast<uint8_t>(craft->space_orders.entries.size()), SpaceOrderAction::Unload, SpaceOrderTargetKind::Structure,
			receiver_ref.structure.base(), receiver_ref.generation).Succeeded());
	};
	// Only the first actual input lacks the explicit module route. This real
	// queue edit releases incomplete work under the original edit contract;
	// subsequent processes retain their genuine in-flight work and holding.
	if (craft->space_orders.cursor < craft->space_orders.entries.size() && craft->space_orders.entries.size() == size_t{craft->space_orders.cursor} + 1) append_unload();
	uint32_t opportunities = 0;
	// Each complete native Mine/Unload earns a real finite20-unit batch. Only a
	// completed original queue may be replaced for another cycle. Native search
	// may wait for actual renewals; no field refill or timer jump is allowed.
	while (opportunities < 4000) {
		const auto &intent = craft->mission.continuation;
		if (intent.phase == MiningContinuationPhase::Waiting && intent.waiting == MiningContinuationWait::NoReceiver) break;
		if (craft->space_orders.cursor >= craft->space_orders.entries.size()) {
			REQUIRE(craft->cargo.TotalCount() == 0);
			// Query while the physically reached receiver berth remains admitted.
			// Clearing first would discard the real source-berth departure proof
			// and falsely make every target unreachable from this exact point.
			if (const auto target = FindNearestEligibleMiningTarget(*craft, CargoLabel{"NORE"})) {
				REQUIRE(CmdEditSpaceOrder(DoCommandFlag::Execute, craft->index, craft->generation, SpaceOrderEdit::Clear, 0,
					SpaceOrderAction::Mine, SpaceOrderTargetKind::Body, 0, 0).Succeeded());
				REQUIRE(CmdEditSpaceOrder(DoCommandFlag::Execute, craft->index, craft->generation, SpaceOrderEdit::Insert, 0,
					SpaceOrderAction::Mine, SpaceOrderTargetKind::Body, target->base(), 0).Succeeded());
				append_unload(); REQUIRE(CmdSetSpaceMiningPolicy(DoCommandFlag::Execute, craft->index, craft->generation, 0, SpaceMiningPolicy::FullLoadNearest).Succeeded());
			}
		}
		RunSpaceLifecycleGameOpportunity(); ++opportunities; REQUIRE(ResolveSpaceCraft(reference) == craft);
		REQUIRE(ResolveSpaceStructure(receiver_ref) == receiver); REQUIRE(craft->manifest->returns_remaining == allowance);
	}
	CHECK(receiver->module->stock[mineral].TotalCount() + craft->cargo.TotalCount() == initial_stock + initial_holding + craft->extracted_units - initial_extracted);
	const bool complete = craft->mission.continuation.phase == MiningContinuationPhase::Waiting && craft->mission.continuation.waiting == MiningContinuationWait::NoReceiver;
	uint32_t fragments = 0;
	// Original whole handling admits at most64 transport fragments across the
	// receiver and incoming batch, even when quantity headroom remains. Count
	// real packet references rather than assigning stock to the quantity cap.
	for (const auto &[hop, packets] : *receiver->module->stock[mineral].Packets()) { (void)hop; fragments += static_cast<uint32_t>(packets.size()); }
	const auto incoming_fragments = craft->cargo.Packets()->size();
	if (complete) {
		REQUIRE(craft->cargo.TotalCount() == GetSpaceFreightCapacity(SPACE_PROBE_MODEL, mineral));
		INFO("actual receiver fragments=" << fragments << " incoming fragments=" << incoming_fragments);
		// A genuine full receiver is bounded by original quantity and transport
		// admission. This must not mislabel an unrelated berth/service refusal.
		REQUIRE((receiver->module->stock[mineral].TotalCount() + craft->cargo.TotalCount() > capacity || fragments + incoming_fragments > 64));
		const auto holding = ObserveMiningControlCargo(*craft); const auto point = craft->position; const auto queue = craft->space_orders.entries;
		const auto before_query = ObserveMiningPolicyDecision(craft->owner); CHECK_FALSE(FindNearestEligibleSpaceReturn(*craft, SpaceReturnPurpose::Cargo));
		CHECK(ObserveMiningPolicyDecision(craft->owner) == before_query); CHECK_FALSE(craft->mission.continuation.effective_target);
		// Actual refusal must retain packets/allowance and bounded saved retries;
		// the native controller alone may choose any farther permitted receiver.
		for (uint sample = 0; sample < 3; ++sample) {
			const auto retry = craft->mission.continuation.retry_ticks; REQUIRE(retry != 0); RunSpaceLifecycleGameOpportunity(); ++opportunities;
			CHECK(craft->position == point); CHECK(ObserveMiningControlCargo(*craft) == holding); CHECK(craft->space_orders.entries == queue);
			CHECK(craft->manifest->returns_remaining == allowance); CHECK(craft->mission.continuation.retry_ticks + 1 == retry);
		}
	}
	const std::string name = complete ? "policy-no-receiver" : "policy-receiver-prepared";
	REQUIRE(BlitterFactory::SelectBlitter("8bpp-simple") != nullptr); GfxClearSpriteCache(); FontCache::ClearFontCaches(FONTSIZES_REQUIRED);
	ShowSpaceSurveyWindow(reference); auto *window = FindWindowById(WindowClass::SpaceSurvey, 0); REQUIRE(window);
	const auto before_save = ObserveMiningPolicyDecision(craft->owner); auto *scroll = window->GetScrollbar(WID_SS_SCROLLBAR);
	scroll->SetPosition(0); CaptureSpaceFixesWindow(*window, std::filesystem::path(output) / (name + "-top.ppm"));
	scroll->SetPosition(scroll->GetCount()); CaptureSpaceFixesWindow(*window, std::filesystem::path(output) / (name + "-bottom.ppm"));
	{
		AutoRestoreBackup paused(_pause_mode, PauseModes{PauseMode::Normal}); const auto path = (std::filesystem::path(output) / (name + ".sav")).string();
		REQUIRE(SaveOrLoad(path, SaveLoadOperation::Save, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Ok);
		REQUIRE(SaveOrLoad(path, SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Ok);
	}
	CHECK(ObserveMiningPolicyDecision(craft->owner) == before_save);
	if (complete) {
		RunSpaceLifecycleGameOpportunity(); ++opportunities; std::ofstream expected(std::filesystem::path(output) / "policy-next.tsv"); REQUIRE(expected.good());
		expected << "# Owner: r2sff1-space-fixes-features-1.md Group31C; actual full receiver refusal,13 native observations\npolicy-no-receiver";
		for (const auto value : ObserveMiningPolicyDecision(craft->owner)) expected << '\t' << value;
		expected << '\n'; expected.flush(); REQUIRE(expected.good());
	}
	fmt::print("RECEIVER_PREPARATION stock_before={} stock_after={} capacity={} actual_holding={} ordinary_opportunities={} allowance={} complete={} receiver_fragments={} incoming_fragments={} assigned_inventory=false assigned_geometry=false\n",
		initial_stock, receiver->module->stock[mineral].TotalCount(), capacity, craft->cargo.TotalCount(), opportunities, allowance, complete, fragments, incoming_fragments);
}

/** Activate an original paid Storage frame and observe real return to a farther compatible receiver.
 * @param output Native activation/travel/unload phase saves, actual captures and their separate next-opportunity oracle. Input is the genuinely prepared NoReceiver checkpoint at the visited full Core; no facility, material, holding or arrival is assigned.
 */
static void VerifyMiningReceiverFallback(const char *output)
{
	SpaceVehicle *craft = nullptr; SpaceStructure *full = nullptr; SpaceStructure *alternative = nullptr;
	// Resolve original paid actors and frames only. The unactivated completed
	// Storage supplies no receiving capacity until its actual command/work finishes.
	for (auto *candidate : SpaceVehicle::Iterate()) {
		if (candidate->owner == _local_company && candidate->construction.placement == SpacePlacement::Orbital && GetSpaceVehicleModel(*candidate) == SPACE_PROBE_MODEL) { craft = candidate; break; }
	}
	REQUIRE(craft); REQUIRE(craft->manifest); REQUIRE(craft->mission.continuation.waiting == MiningContinuationWait::NoReceiver);
	const auto reference = GetSpaceCraftReference(*craft); const auto holding = ObserveMiningControlCargo(*craft); const auto units = craft->cargo.TotalCount();
	const auto cargo = craft->cargo_type; const auto allowance = craft->manifest->returns_remaining; const auto queue = craft->space_orders.entries;
	REQUIRE(units == GetSpaceFreightCapacity(SPACE_PROBE_MODEL, cargo)); REQUIRE(queue.size() == 3); REQUIRE(queue[1].action == SpaceOrderAction::Visit);
	for (auto *site : SpaceStructure::Iterate()) {
		if (site->state.owner != craft->owner) continue;
		if (site->state.kind == SpaceBuildKind::Core && IsOrbitalModuleOperational(*site)) full = site;
		if (site->state.kind == SpaceBuildKind::Storage && site->state.phase == SpaceStructurePhase::FrameComplete && !site->module) alternative = site;
	}
	REQUIRE(full); REQUIRE(alternative); REQUIRE(alternative->state.capital_paid > 0);
	REQUIRE(craft->position == GetSpaceStructureApproach(full->state)); REQUIRE(GetSpaceStructureBerth(*craft, *full));
	CHECK_FALSE(FindNearestEligibleSpaceReturn(*craft, SpaceReturnPurpose::Cargo));
	const auto alternative_ref = GetSpaceStructureReference(*alternative); const auto full_stock = full->module->stock[cargo].TotalCount();
	AutoRestoreBackup actor(_current_company, craft->owner); AutoRestoreBackup running(_pause_mode, PauseModes{});
	// The original constructor still physically occupies this finished frame's
	// one approach. Give that actual hull a real Visit before activation; ordinary
	// movement alone frees the berth, with all spent construction receipts retained.
	SpaceVehicle *builder = nullptr;
	for (auto *candidate : SpaceVehicle::Iterate()) {
		if (candidate->owner == craft->owner && GetSpaceVehicleModel(*candidate) == 20 && candidate->position == GetSpaceStructureApproach(alternative->state)) { builder = candidate; break; }
	}
	REQUIRE(builder); REQUIRE(builder->space_orders.cursor == builder->space_orders.entries.size()); REQUIRE(builder->cargo.TotalCount() == 0);
	std::optional<OrbitalBodyID> visit;
	// A real existing distant body supplies navigation only, not a new mineral
	// grant. The constructor's visit cannot take the miner's extraction lease.
	for (const auto &body : GetAsteroidField().bodies) {
		if (body.mining_slot == VehicleID::Invalid() && SpaceDistanceSquared(builder->position, body.motion.position) > uint64_t{16} * SPACE_Q16 * SPACE_Q16) { visit = body.id; break; }
	}
	REQUIRE(visit);
	REQUIRE(CmdEditSpaceOrder(DoCommandFlag::Execute, builder->index, builder->generation, SpaceOrderEdit::Clear, 0, SpaceOrderAction::Visit, SpaceOrderTargetKind::Body, 0, 0).Succeeded());
	REQUIRE(CmdEditSpaceOrder(DoCommandFlag::Execute, builder->index, builder->generation, SpaceOrderEdit::Insert, 0, SpaceOrderAction::Visit, SpaceOrderTargetKind::Body, visit->base(), 0).Succeeded());
	const auto before_query = ObserveMiningPolicyDecision(craft->owner);
	REQUIRE(CmdActivateOrbitalModule({}, alternative_ref.structure, alternative_ref.generation).Succeeded());
	CHECK(ObserveMiningPolicyDecision(craft->owner) == before_query);
	REQUIRE(CmdActivateOrbitalModule(DoCommandFlag::Execute, alternative_ref.structure, alternative_ref.generation).Succeeded());
	std::ofstream expected(std::filesystem::path(output) / "policy-next.tsv"); REQUIRE(expected.good());
	expected << "# Owner: r2sff1-space-fixes-features-1.md Group31C; actual paid receiver activation and fallback,13 native observations\n";
	uint32_t opportunities = 0;
	/** Offer one original ordinary opportunity; no activation, movement or handling producer is called directly. */
	auto opportunity = [&] {
		REQUIRE(opportunities++ < 2000); RunSpaceLifecycleGameOpportunity(); REQUIRE(ResolveSpaceCraft(reference) == craft);
		CHECK(craft->space_orders.entries == queue); CHECK(craft->manifest->returns_remaining == allowance); CHECK(full->module->stock[cargo].TotalCount() == full_stock);
	};
	/** @param name Actual phase name. Save and preview without changing owners, then record its one genuine next opportunity. */
	auto checkpoint = [&](std::string_view name) {
		const auto before = ObserveMiningPolicyDecision(craft->owner); ShowSpaceSurveyWindow(reference);
		auto *window = FindWindowById(WindowClass::SpaceSurvey, 0); REQUIRE(window); auto *scroll = window->GetScrollbar(WID_SS_SCROLLBAR);
		scroll->SetPosition(0); CaptureSpaceFixesWindow(*window, std::filesystem::path(output) / fmt::format("{}-top.ppm", name));
		scroll->SetPosition(scroll->GetCount()); CaptureSpaceFixesWindow(*window, std::filesystem::path(output) / fmt::format("{}-bottom.ppm", name));
		{
			AutoRestoreBackup paused(_pause_mode, PauseModes{PauseMode::Normal}); const auto path = (std::filesystem::path(output) / fmt::format("{}.sav", name)).string();
			REQUIRE(SaveOrLoad(path, SaveLoadOperation::Save, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Ok);
			REQUIRE(SaveOrLoad(path, SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Ok);
		}
		CHECK(ObserveMiningPolicyDecision(craft->owner) == before); opportunity(); expected << name;
		for (const auto value : ObserveMiningPolicyDecision(craft->owner)) expected << '\t' << value;
		expected << '\n'; expected.flush(); REQUIRE(expected.good());
	};
	REQUIRE(BlitterFactory::SelectBlitter("8bpp-simple") != nullptr); GfxClearSpriteCache(); FontCache::ClearFontCaches(FONTSIZES_REQUIRED);
	// Original activation earns its own saved countdown while receiver retries
	// remain bounded. No resource event or part grant is forged to wake the hull.
	while (!IsOrbitalModuleOperational(*alternative)) { opportunity(); CHECK(ObserveMiningControlCargo(*craft) == holding); }
	const auto nearest = FindNearestEligibleSpaceReturn(*craft, SpaceReturnPurpose::Cargo); REQUIRE(nearest);
	REQUIRE(nearest->target == SpaceOrderTarget{SpaceOrderTargetKind::Structure, alternative_ref.structure.base(), alternative_ref.generation});
	const auto full_distance = SpaceDistanceSquared(craft->position, GetSpaceStructureApproach(full->state));
	const auto alternative_distance = SpaceDistanceSquared(craft->position, nearest->anchor); CHECK(alternative_distance > full_distance);
	REQUIRE(alternative->module->stock[cargo].TotalCount() == 0); checkpoint("policy-receiver-activated");
	// Wait for actual native reselection, then actual movement and original
	// packet transfer. The query above neither assigns a berth nor moves cargo.
	while (!craft->mission.continuation.effective_target) opportunity();
	REQUIRE(*craft->mission.continuation.effective_target == nearest->target); checkpoint("policy-receiver-travel");
	while (craft->cargo.TotalCount() != 0) opportunity();
	CHECK(alternative->module->stock[cargo].TotalCount() == units); CHECK(craft->space_orders.cursor == queue.size());
	// Every original source contributes exactly its held amount. Native merging
	// may change fragment identities, but cannot rewrite historical provenance.
	for (const auto &[id, source, quantity] : holding) {
		(void)id; (void)quantity; uint32_t aboard = 0, received = 0;
		for (const auto &[other_id, other_source, other_quantity] : holding) { (void)other_id; if (other_source == source) aboard += other_quantity; }
		for (const auto &[hop, packets] : *alternative->module->stock[cargo].Packets()) for (const auto *packet : packets) { (void)hop; if (packet->GetSource() == source) received += packet->Count(); }
		CHECK(received == aboard);
	}
	checkpoint("policy-receiver-unloaded");
	fmt::print("MINING_RECEIVER_FALLBACK original_model=15 ordinary_opportunities={} actual_units={} full_receiver={} alternative={} farther=true native_quota_spent=0 original_paid_activation=true synthetic_arrival=false\n",
		opportunities, units, full->index.base(), alternative_ref.structure.base());
}

/** Save a genuine refusal phase and record its one actual next ordinary decision.
 * @param craft Original inspected hull whose native owners remain authoritative.
 * @param output Private phase-save/capture directory.
 * @param phase Unique saved phase name, also used as the replay-oracle key.
 * @param expected Open invocation-owned TSV receiving13 synchronized owner observations after one real ordinary opportunity.
 */
static void CheckpointMiningRefusal(SpaceVehicle &craft, const char *output, std::string_view phase, std::ofstream &expected)
{
	const auto before = ObserveMiningPolicyDecision(craft.owner); ShowSpaceSurveyWindow(GetSpaceCraftReference(craft));
	auto *window = FindWindowById(WindowClass::SpaceSurvey, 0); REQUIRE(window); auto *scroll = window->GetScrollbar(WID_SS_SCROLLBAR);
	scroll->SetPosition(0); CaptureSpaceFixesWindow(*window, std::filesystem::path(output) / fmt::format("{}-top.ppm", phase));
	scroll->SetPosition(scroll->GetCount()); CaptureSpaceFixesWindow(*window, std::filesystem::path(output) / fmt::format("{}-bottom.ppm", phase));
	{
		AutoRestoreBackup paused(_pause_mode, PauseModes{PauseMode::Normal}); const auto path = (std::filesystem::path(output) / fmt::format("{}.sav", phase)).string();
		REQUIRE(SaveOrLoad(path, SaveLoadOperation::Save, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Ok);
		REQUIRE(SaveOrLoad(path, SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Ok);
	}
	CHECK(ObserveMiningPolicyDecision(craft.owner) == before); RunSpaceLifecycleGameOpportunity(); expected << phase;
	for (const auto value : ObserveMiningPolicyDecision(craft.owner)) expected << '\t' << value;
	expected << '\n'; expected.flush(); REQUIRE(expected.good());
}

/** Measure genuine bounded retry work while the original miner has no compatible mineral target.
 * @param output Private directory for actual counters and native owner evidence; no geometry, stock, timing, work or paid counters are assigned.
 */
static void VerifyNativeMiningRetryWork(const char *output)
{
	SpaceVehicle *craft = nullptr; size_t participants = 0;
	// Retain the actual original heavy miner and count real native participants;
	// stopping other original actors prevents unrelated commands selecting targets.
	for (auto *candidate : SpaceVehicle::Iterate()) {
		++participants;
		if (GetSpaceVehicleModel(*candidate) == SPACE_HEAVY_MINER_MODEL) craft = candidate;
	}
	REQUIRE(craft); REQUIRE(craft->mission.continuation.waiting == MiningContinuationWait::NoCompatibleAsteroid);
	REQUIRE(craft->mission.continuation.phase == MiningContinuationPhase::Waiting);
	const auto cargo = ObserveMiningControlCargo(*craft); const auto queue = craft->space_orders.entries; const auto profit = craft->profit_this_year;
	for (auto *other : SpaceVehicle::Iterate()) {
		if (other == craft) continue;
		AutoRestoreBackup actor(_current_company, other->owner);
		if (!other->vehstatus.Test(VehState::Stopped)) REQUIRE(CmdStartStopVehicle(DoCommandFlag::Execute, other->index, true).Succeeded());
	}
	constexpr uint32_t duration = 5 * 74; REQUIRE(GetAsteroidField().renewal.ticks_remaining > duration);
	const auto initial = GetAsteroidField(); SpaceWorkSample sample; uint64_t body_steps = 0;
	AutoRestoreBackup running(_pause_mode, PauseModes{});
	// Five complete retry periods must not turn the safe wait into an every-tick
	// nearest scan. Genuine matching exits may legitimately add an early wake.
	for (uint32_t tick = 0; tick < duration; ++tick) {
		RunSpaceLifecycleGameOpportunity(&sample); body_steps += GetAsteroidField().bodies.size();
		CHECK(ObserveMiningControlCargo(*craft) == cargo); CHECK(craft->space_orders.entries == queue);
		CHECK(craft->mission.continuation.waiting == MiningContinuationWait::NoCompatibleAsteroid);
	}
	uint64_t retired = 0;
	for (size_t size = 0; size < 3; ++size) retired += GetAsteroidField().renewal.exited_by_size[size] - initial.renewal.exited_by_size[size];
	const auto selections = sample.Get(SpaceWorkKind::MiningSelections);
	CHECK(selections > 0); CHECK(selections <= (duration + 73) / 74 + retired + 1);
	CHECK(sample.Get(SpaceWorkKind::BodySteps) == body_steps); CHECK(sample.Get(SpaceWorkKind::IndexUpdates) == body_steps);
	CHECK(sample.Get(SpaceWorkKind::InterceptForecasts) <= selections * initial.bodies.size());
	CHECK(sample.Get(SpaceWorkKind::AttachmentVisits) <= uint64_t{12} * duration * participants);
	CHECK(sample.Get(SpaceWorkKind::RetirementParticipants) <= retired * participants);
	CHECK(sample.Get(SpaceWorkKind::RenewalAttempts) == 0); CHECK(craft->profit_this_year < profit);
	nlohmann::json report{{"_documentation", "r2sff1-space-fixes-features-1.md33; only real StateGameLoop work counted; genuine NoCompatibleAsteroid wait"},
		{"ordinary_opportunities", duration}, {"actual_participants", participants}, {"matching_or_other_retirements", retired}, {"mining_selections", selections},
		{"body_steps", body_steps}, {"index_updates", sample.Get(SpaceWorkKind::IndexUpdates)}, {"craft_ticks", sample.Get(SpaceWorkKind::CraftTicks)},
		{"attachment_visits", sample.Get(SpaceWorkKind::AttachmentVisits)}, {"retirement_participants", sample.Get(SpaceWorkKind::RetirementParticipants)},
		{"exit_forecasts", sample.Get(SpaceWorkKind::ExitForecasts)}, {"intercept_forecasts", sample.Get(SpaceWorkKind::InterceptForecasts)},
		{"renewal_attempts", sample.Get(SpaceWorkKind::RenewalAttempts)}, {"original_cargo_queue_preserved", true}, {"original_idle_upkeep_charged", true}};
	std::ofstream saved(std::filesystem::path(output) / "native-retry-work.json"); REQUIRE(saved.good()); saved << report.dump(2) << '\n'; saved.flush(); REQUIRE(saved.good());
	fmt::print("MINING_RETRY_WORK opportunities={} selections={} retired={} participants={} actual_state_loop=true\n", duration, selections, retired, participants);
}

/** Prepare a real high-ID cargo holding after a clearly declared historical issuance prefix.
 * @param output Private source-stage saves and native event evidence. Existing live IDs/packets, credits, timers, streams and paid permits are never rewritten by the prefix declaration.
 */
static void PrepareNativeHighIDPeerSource(const char *output)
{
	SpaceVehicle *craft = nullptr;
	// The earlier real after-credit source owns one retired original Large body
	// and exactly four paid heavy-miner units. No replacement hull is commissioned.
	for (auto *candidate : SpaceVehicle::Iterate()) if (GetSpaceVehicleModel(*candidate) == SPACE_HEAVY_MINER_MODEL) { craft = candidate; break; }
	REQUIRE(craft); REQUIRE(craft->manifest); REQUIRE(craft->cargo.TotalCount() == 4);
	REQUIRE(GetAsteroidField().next_body_id == 10); REQUIRE(GetAsteroidField().renewal.credits == 5);
	REQUIRE(GetAsteroidField().renewal.exited_by_size == std::array<uint64_t, 3>{0, 0, 1});
	const auto cargo = ObserveMiningControlCargo(*craft); const auto extracted = craft->extracted_units;
	/** @return Original paid return, pending, service, shell and supply counters; no observation grants inventory. */
	auto permits = [&] {
		return std::array<uint64_t, 7>{craft->manifest->returns_remaining, craft->manifest->pending_returns, craft->manifest->services_completed,
			craft->manifest->pods.size(), craft->manifest->supplies[0].TotalCount(), craft->manifest->supplies[1].TotalCount(), craft->manifest->supplies[2].TotalCount()};
	};
	const auto original_permits = permits();
	// Preserve all actual paid actors while preventing another original miner
	// from using the new source during this bounded high-ID boundary preparation.
	for (auto *other : SpaceVehicle::Iterate()) {
		AutoRestoreBackup actor(_current_company, other->owner);
		if (!other->vehstatus.Test(VehState::Stopped)) REQUIRE(CmdStartStopVehicle(DoCommandFlag::Execute, other->index, true).Succeeded());
	}
	AutoRestoreBackup actor(_current_company, craft->owner); AutoRestoreBackup running(_pause_mode, PauseModes{});
	auto field = GetAsteroidField(); const auto original_ledger = field.renewal;
	constexpr uint32_t high_id = 65'536; constexpr uint64_t historical_turnover = high_id - 10;
	// This is an explicit historical-prefix schema fixture, not elapsed travel.
	// Alternating retired Small/Medium replacements can be funded by the actual
	// original Large exit; each completed pair returns the same five credits.
	// No original live ID is renamed and neither independent stream is advanced.
	field.next_body_id = high_id;
	field.renewal.exited_by_size[0] += historical_turnover / 2;
	field.renewal.exited_by_size[1] += historical_turnover / 2;
	REQUIRE(ValidateAsteroidField(field)); REQUIRE(InstallAsteroidField(std::move(field)));
	CHECK(GetAsteroidField().renewal.credits == original_ledger.credits); CHECK(GetAsteroidField().renewal.ticks_remaining == original_ledger.ticks_remaining);
	CHECK(GetAsteroidField().renewal.timer_rng.state[0] == original_ledger.timer_rng.state[0]); CHECK(GetAsteroidField().renewal.timer_rng.state[1] == original_ledger.timer_rng.state[1]);
	CHECK(GetAsteroidField().renewal.candidate_rng.state[0] == original_ledger.candidate_rng.state[0]); CHECK(GetAsteroidField().renewal.candidate_rng.state[1] == original_ledger.candidate_rng.state[1]);
	CHECK(ObserveMiningControlCargo(*craft) == cargo); CHECK(permits() == original_permits);
	/** @param phase Actual preparation boundary, whose unchanged owners must pass native save/check. */
	auto save = [&](std::string_view phase) {
		const auto before = ObserveMiningPolicyDecision(craft->owner); AutoRestoreBackup paused(_pause_mode, PauseModes{PauseMode::Normal});
		const auto path = (std::filesystem::path(output) / fmt::format("{}.sav", phase)).string();
		REQUIRE(SaveOrLoad(path, SaveLoadOperation::Save, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Ok);
		REQUIRE(SaveOrLoad(path, SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Ok);
		CHECK(ObserveMiningPolicyDecision(craft->owner) == before);
	};
	uint32_t opportunities = 0;
	/** Offer one actual ordinary opportunity; no producer clock or paid-work counter is installed. */
	auto opportunity = [&] {
		INFO("high-ID preparation opportunities=" << opportunities << " phase=" << unsigned{to_underlying(craft->mission.phase)} << " target=" << craft->mission.target.base()
			<< " blocked=" << unsigned{to_underlying(craft->space_orders.blocked)} << " continuation=" << unsigned{to_underlying(craft->mission.continuation.phase)}
			<< " wait=" << unsigned{to_underlying(craft->mission.continuation.waiting)} << " stopped=" << craft->vehstatus.Test(VehState::Stopped) << " position=" << craft->position.x << ',' << craft->position.y);
		if (opportunities == 4999) save("high-id-budget-diagnostic");
		REQUIRE(opportunities++ < 5000); RunSpaceLifecycleGameOpportunity(); CHECK(permits() == original_permits);
	};
	save("high-id-declared-prefix");
	// The complete remaining saved interval is earned under the ordinary loop.
	// Native creation alone issues65536 with finite reserve and a canonical entry.
	while (GetAsteroidField().next_body_id == high_id) opportunity();
	REQUIRE(GetAsteroidField().next_body_id == high_id + 1);
	{ const auto *body = GetMutableAsteroidBody(OrbitalBodyID{high_id}); REQUIRE(body); REQUIRE(body->resource_class == AsteroidClass::Metallic); }
	save("high-id-born");
	REQUIRE(CmdEditSpaceOrder(DoCommandFlag::Execute, craft->index, craft->generation, SpaceOrderEdit::Clear, 0, SpaceOrderAction::Mine, SpaceOrderTargetKind::Body, 0, 0).Succeeded());
	REQUIRE(CmdEditSpaceOrder(DoCommandFlag::Execute, craft->index, craft->generation, SpaceOrderEdit::Insert, 0, SpaceOrderAction::Mine, SpaceOrderTargetKind::Body, high_id, 0).Succeeded());
	REQUIRE(CmdSetSpaceMiningPolicy(DoCommandFlag::Execute, craft->index, craft->generation, 0, SpaceMiningPolicy::FullLoadNearest).Succeeded());
	// The original queue-edit command already starts the actual hull. A second
	// start/stop toggle here would stop it again and manufacture a false deadlock.
	REQUIRE_FALSE(craft->vehstatus.Test(VehState::Stopped));
	const auto initial_reserve = GetMutableAsteroidBody(OrbitalBodyID{high_id})->remaining_reserve;
	// Actual native pursuit, admission and all74 work opportunities alone emit
	// the new packet. A high numerical ID cannot substitute for physical arrival.
	while (craft->extracted_units == extracted) opportunity();
	REQUIRE(craft->mission.target == OrbitalBodyID{high_id}); REQUIRE(craft->cargo.TotalCount() == 8);
	CHECK(craft->extracted_units == extracted + 4); CHECK(GetMutableAsteroidBody(OrbitalBodyID{high_id})->remaining_reserve == initial_reserve - 4);
	const auto actual_holding = ObserveMiningControlCargo(*craft);
	for (const auto &packet : cargo) CHECK(std::ranges::find(actual_holding, packet) != actual_holding.end());
	uint16_t high_units = 0;
	// Source identity, not packet ordering or a16-bit ordinary station scalar,
	// distinguishes the new finite batch from the original retired-body history.
	for (const auto &[packet, origin, units] : actual_holding) {
		const auto natural = origin.GetAsteroidOrigin();
		if (natural && natural->body == high_id) { CHECK(natural->mineral == CargoLabel{"NORE"}); high_units += units; }
	}
	REQUIRE(high_units == 4); REQUIRE(CmdStartStopVehicle(DoCommandFlag::Execute, craft->index, true).Succeeded());
	save("high-id-extracted");
	std::vector<OrbitalBodyID> funding;
	// The original Large already exited. Two unleased original Medium exits
	// add six real weights before the next attempt, retaining credit even if
	// that attempt buys a Large. Geometry alone earns none of that credit.
	for (const auto *body : GetAsteroidBodiesInIDOrder(GetAsteroidField())) {
		if (body->id.base() < 10 && body->size == AsteroidSize::Medium && body->mining_slot == VehicleID::Invalid()) funding.push_back(body->id);
	}
	REQUIRE(funding.size() == 2); field = GetAsteroidField();
	// Preserve each original heading/stock; the real next ordinary field pass
	// must sort and retire these original centres before any due creation.
	for (const auto id : funding) {
		auto *body = FindAsteroidBody(field, id); const auto exit = ForecastAsteroidExit(*body, *GetSpacePlaneBounds(field.width, field.height)); REQUIRE(exit); body->motion = exit->last_safe_motion;
	}
	REQUIRE(InstallAsteroidField(std::move(field))); const auto before_funding = GetAsteroidField().next_body_id;
	opportunity(); for (const auto id : funding) REQUIRE(GetMutableAsteroidBody(id) == nullptr);
	// A due attempt may coincide with the funding exit. Otherwise earn its
	// existing deadline; additional credit is forbidden from postponing it.
	while (GetAsteroidField().next_body_id == before_funding) opportunity();
	REQUIRE(GetAsteroidField().renewal.credits >= 3); REQUIRE(GetAsteroidField().renewal.ticks_remaining >= GetAsteroidRenewalDelayBounds().first);
	CHECK(ObserveMiningControlCargo(*craft) == actual_holding);
	REQUIRE(CmdStartStopVehicle(DoCommandFlag::Execute, craft->index, true).Succeeded()); opportunity();
	REQUIRE(craft->mission.phase == SpaceMissionPhase::Mining); REQUIRE(craft->mission.work_ticks > 0);
	StageMiningControlEdge(*craft); CHECK(ObserveMiningControlCargo(*craft) == actual_holding); CHECK(permits() == original_permits);
	save("policy-high-id-current");
	fmt::print("HIGH_ID_PEER_SOURCE declared_historical_turnover={} original_ids_packets_preserved=true native_birth={} native_extraction_units={} ordinary_opportunities={} funded_countdown={} cargo={}\n",
		historical_turnover, high_id, high_units, opportunities, GetAsteroidField().renewal.ticks_remaining, craft->cargo.TotalCount());
}

/** Exercise the shared production scheduler without the null driver's unpaced MainLoop.
 * The selected application's original null driver still owns its assets/windows.
 * This observer supplies no replacement simulation loop or manufactured clock.
 */
class SpaceLifecycleSchedulingDriver : public VideoDriver_Null {
public:
	/** Retain the real scheduler's single-thread path and defer drawing beyond this bounded observation. */
	SpaceLifecycleSchedulingDriver()
	{
		this->is_game_threaded = false;
		this->next_game_tick = std::chrono::steady_clock::now();
		this->next_draw_tick = std::chrono::steady_clock::time_point::max();
	}
	/** @return True suppresses the headless InputLoop path; drawing remains deferred, so the original running console script is never reentered. */
	bool HasGUI() const override { return true; }
	/** @param duration Actual wall-time observation window. @return Completed original field opportunities, each checked against the real saved countdown. */
	uint64_t Observe(std::chrono::milliseconds duration)
	{
		const auto started = GetAsteroidField().elapsed_ticks;
		const auto deadline = std::chrono::steady_clock::now() + duration;
		// Repeated real driver callbacks offer time according to GetGameInterval;
		// each admitted ordinary opportunity must debit exactly one saved tick.
		while (std::chrono::steady_clock::now() < deadline) {
			const auto before = GetAsteroidField(); this->Tick(); const auto &after = GetAsteroidField();
			REQUIRE(after.elapsed_ticks <= before.elapsed_ticks + 1);
			if (after.elapsed_ticks == before.elapsed_ticks) { CHECK(after == before); }
			else {
				REQUIRE(before.renewal.ticks_remaining > 1); CHECK(after.renewal == *AdvanceAsteroidRenewalTimer(before.renewal, before.next_body_id));
				CHECK(after.next_body_id == before.next_body_id);
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}
		return GetAsteroidField().elapsed_ticks - started;
	}
};

/** Observe actual fast-forward cadence on an already naturally armed saved phase.
 * @param output Private artifact directory; the report records measured opportunities rather than invented elapsed travel.
 */
static void VerifySpaceLifecycleScheduling(const char *output)
{
	REQUIRE(GetAsteroidField().renewal.ticks_remaining > 600);
	AutoRestoreBackup running(_pause_mode, PauseModes{}); AutoRestoreBackup speed(_game_speed);
	// The test console runs inside SwitchToMode's load callback, before the
	// outer dispatcher clears its completed request. Nested real GameLoop calls
	// must see that load as dispatched rather than reload the source recursively.
	AutoRestoreBackup dispatched(_switch_mode, SwitchMode::None);
	AutoRestoreBackup limit(_settings_client.gui.fast_forward_speed_limit, uint16_t{1000});
	ChangeGameSpeed(false); SpaceLifecycleSchedulingDriver scheduler;
	const auto normal = scheduler.Observe(std::chrono::milliseconds(600)); REQUIRE(normal > 0);
	ChangeGameSpeed(true); CHECK(_game_speed == 1000);
	const auto accelerated = scheduler.Observe(std::chrono::milliseconds(600)); CHECK(accelerated > normal);
	{
		AutoRestoreBackup paused(_pause_mode, PauseModes{PauseMode::Normal}); const auto before = ObserveMiningPolicyDecision(_local_company);
		CHECK(scheduler.Observe(std::chrono::milliseconds(120)) == 0); CHECK(ObserveMiningPolicyDecision(_local_company) == before);
	}
	// Resuming offers only the original next scheduled opportunity. A long pause
	// cannot turn into a batch of timer decrements or extra replacement attempts.
	const auto resumed = scheduler.Observe(std::chrono::milliseconds(120)); REQUIRE(resumed > 0);
	fmt::print("LIFECYCLE_SCHEDULER normal_600ms={} fast_600ms={} resumed_120ms={} real_driver_tick=true paused_frozen=true single_decrement=true output={}\n", normal, accelerated, resumed, output);
}

/** Earn and save an uninterrupted native lifecycle from the original last-safe attachment.
 * @param output Private directory for actual phase saves, per-opportunity observations and independent next-step oracles.
 * @param hidden True selects the Surface viewport; false selects Space. Both runs use identical saved actors, commands and ordinary opportunities.
 */
static void VerifySpaceLifecyclePhases(const char *output, bool hidden)
{
	SpaceVehicle *craft = nullptr;
	// Resolve the original paid heavy miner by its role, retaining its actual
	// partial holding and Full Load queue from the earlier declared geometry save.
	for (auto *candidate : SpaceVehicle::Iterate()) {
		if (candidate->owner == _local_company && GetSpaceVehicleModel(*candidate) == SPACE_HEAVY_MINER_MODEL) { craft = candidate; break; }
	}
	REQUIRE(craft); REQUIRE(craft->mission.phase == SpaceMissionPhase::Mining); REQUIRE(craft->cargo.TotalCount() == 4);
	REQUIRE(HasSpaceMiningContinuationOrder(*craft)); REQUIRE(GetAsteroidField().renewal.credits == 0);
	const auto reference = GetSpaceCraftReference(*craft); const auto source = craft->mission.target;
	const auto source_body = *GetMutableAsteroidBody(source); REQUIRE(source_body.size == AsteroidSize::Large);
	const auto original_cargo = ObserveMiningControlCargo(*craft); const auto queue = craft->space_orders.entries;
	const auto initial_issued = GetAsteroidField().next_body_id; const auto initial_elapsed = GetAsteroidField().elapsed_ticks;
	const auto bounds = *GetSpacePlaneBounds(Map::SizeX(), Map::SizeY());
	REQUIRE(BlitterFactory::SelectBlitter("8bpp-simple") != nullptr); GfxClearSpriteCache(); FontCache::ClearFontCaches(FONTSIZES_REQUIRED);
	SetViewportLayer(GetMainWindow(), hidden ? MapLayer::Surface : MapLayer::Space);
	AutoRestoreBackup running(_pause_mode, PauseModes{});
	std::ofstream expected(std::filesystem::path(output) / "policy-next.tsv"); REQUIRE(expected.good());
	std::ofstream trace(std::filesystem::path(output) / "lifecycle-opportunities.tsv"); REQUIRE(trace.good());
	expected << "# Owner: r2sff1-space-fixes-features-1.md31E; actual next decisions, no assigned geometry or timer\n";
	std::optional<std::string> pending; uint32_t opportunities = 0;
	/** @param phase Actual saved boundary whose next ordinary opportunity will be recorded without advancing here. */
	auto checkpoint = [&](std::string_view phase) {
		REQUIRE_FALSE(pending); const auto before = ObserveMiningPolicyDecision(craft->owner);
		ShowSpaceSurveyWindow(reference); auto *window = FindWindowById(WindowClass::SpaceSurvey, 0); REQUIRE(window);
		CaptureSpaceFixesWindow(*window, std::filesystem::path(output) / fmt::format("{}-top.ppm", phase));
		{
			AutoRestoreBackup paused(_pause_mode, PauseModes{PauseMode::Normal}); const auto path = (std::filesystem::path(output) / fmt::format("{}.sav", phase)).string();
			REQUIRE(SaveOrLoad(path, SaveLoadOperation::Save, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Ok);
			REQUIRE(SaveOrLoad(path, SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Ok);
		}
		CHECK(ObserveMiningPolicyDecision(craft->owner) == before); pending = phase;
	};
	/** Offer exactly one ordinary opportunity and independently reconcile real exit credit, countdown, candidate publication and finite motion. */
	auto opportunity = [&] {
		REQUIRE(opportunities++ < 5000); const auto before = GetAsteroidField(); std::vector<AsteroidExitRecord> exits;
		// Forecast only this next straight step. Geometry forecasts earn no credit;
		// the actual field owner below must perform every retirement and birth.
		for (const auto *body : GetAsteroidBodiesInIDOrder(before)) {
			const auto step = StepSpaceMotionStraight(body->motion, bounds); REQUIRE(step);
			if (step->crossed_edges.Any()) exits.push_back({body->id, {body->size, body->resource_class}});
		}
		auto ledger = ApplyAsteroidExitCredit(before.renewal, exits, before.next_body_id); REQUIRE(ledger);
		if (before.renewal.credits != 0) ledger = AdvanceAsteroidRenewalTimer(*ledger, before.next_body_id);
		REQUIRE(ledger); const auto ticket = BeginAsteroidRenewalAttempt(*ledger, before.next_body_id);
		RunSpaceLifecycleGameOpportunity(); REQUIRE(ResolveSpaceCraft(reference) == craft); const auto &after = GetAsteroidField();
		const bool born = after.next_body_id != before.next_body_id; CHECK(after.next_body_id == before.next_body_id + (born ? 1U : 0U));
		if (ticket) ledger = FinishAsteroidRenewalAttempt(*ledger, *ticket, born, before.next_body_id);
		else CHECK_FALSE(born);
		REQUIRE(ledger); CHECK(after.renewal == *ledger); CHECK(craft->space_orders.entries == queue);
		// Existing surviving resources retain their immutable identity/class/size,
		// move once with the original heading, and can lose reserve only natively.
		for (const auto &old : before.bodies) {
			const auto *live = FindAsteroidBody(after, old.id); if (live == nullptr) continue;
			CHECK(live->motion == StepSpaceMotionStraight(old.motion, bounds)->proposed);
			CHECK(live->initial_reserve == old.initial_reserve); CHECK(live->remaining_reserve <= old.remaining_reserve);
			CHECK(live->resource_class == old.resource_class); CHECK(live->size == old.size);
		}
		if (born) {
			const auto *body = FindAsteroidBody(after, OrbitalBodyID{before.next_body_id}); REQUIRE(body); REQUIRE(ticket); REQUIRE(ticket->size);
			CHECK(body->size == *ticket->size); CHECK(body->size != after.renewal.last_exit->size);
			CHECK(body->initial_reserve > 0); CHECK(body->remaining_reserve == body->initial_reserve);
			CHECK(body->mining_slot == VehicleID::Invalid()); CHECK(body->survey_slot == INVALID_SPACE_SURVEY);
			CHECK(body->motion.remainder_x == 0); CHECK(body->motion.remainder_y == 0);
		}
		const auto observed = ObserveMiningPolicyDecision(craft->owner); trace << opportunities;
		for (const auto value : observed) trace << '\t' << value;
		trace << '\n';
		if (pending) {
			expected << *pending; for (const auto value : observed) expected << '\t' << value;
			expected << '\n'; expected.flush(); REQUIRE(expected.good()); pending.reset();
		}
	};
	checkpoint("policy-lifecycle-before-detachment"); opportunity();
	CHECK(craft->mission.phase != SpaceMissionPhase::Mining); CHECK(craft->mission.work_ticks == 0);
	CHECK(ObserveMiningControlCargo(*craft) == original_cargo); CHECK(GetMutableAsteroidBody(source)->mining_slot == VehicleID::Invalid());
	checkpoint("policy-lifecycle-after-departure"); opportunity();
	// The original centre must actually exit before credit is earned. No due
	// timer, velocity, counter, stock, cargo or paid permit is assigned in this trace.
	while (GetMutableAsteroidBody(source) != nullptr) opportunity();
	REQUIRE(GetAsteroidField().renewal.credits >= *GetAsteroidRenewalCost(source_body.size));
	const auto armed = GetAsteroidField().renewal.ticks_remaining; const auto delays = GetAsteroidRenewalDelayBounds();
	REQUIRE(armed >= delays.first); REQUIRE(armed <= delays.second);
	checkpoint("policy-lifecycle-after-credit"); opportunity();
	while (GetAsteroidField().renewal.ticks_remaining != 1) opportunity();
	checkpoint("policy-lifecycle-before-expiry"); opportunity();
	// A naturally refused attempt retains its credits and earns another complete
	// interval. This bounded input normally has clear entry; never force success.
	while (GetAsteroidField().next_body_id == initial_issued) opportunity();
	checkpoint("policy-lifecycle-after-birth"); opportunity(); trace.flush(); REQUIRE(trace.good());
	CHECK(GetAsteroidField().elapsed_ticks == initial_elapsed + opportunities);
	fmt::print("LIFECYCLE_PHASES ordinary_opportunities={} armed_delay={} hidden={} geometry_assigned=false timer_assigned=false original_cargo_at_departure=true original_actor_queue=true births={}\n", opportunities, armed, hidden, GetAsteroidField().next_body_id - initial_issued);
}

/** Remove an empty real receiver while the original loaded miner is genuinely approaching it.
 * @param output Private immediate-removal/wait phase saves, captures and actual next-opportunity oracle. The existing paid activated Storage and departed constructor come from31C.2's original native travel checkpoint; no pool object is deleted by this observer.
 */
static void VerifyMiningReceiverRemoval(const char *output)
{
	SpaceVehicle *craft = nullptr;
	// Follow the original15's real in-flight continuation; a new hull or an
	// unrelated fixed destination cannot substitute for this saved approach.
	for (auto *candidate : SpaceVehicle::Iterate()) {
		if (candidate->owner == _local_company && candidate->construction.placement == SpacePlacement::Orbital && GetSpaceVehicleModel(*candidate) == SPACE_PROBE_MODEL) { craft = candidate; break; }
	}
	REQUIRE(craft); REQUIRE(craft->manifest); REQUIRE(craft->mission.continuation.effective_target);
	const auto target = *craft->mission.continuation.effective_target; REQUIRE(target.kind == SpaceOrderTargetKind::Structure);
	const SpaceStructureRef removed{SpaceStructureID{static_cast<uint16_t>(target.id)}, target.generation};
	const auto *site = ResolveSpaceStructure(removed); REQUIRE(site); REQUIRE(site->state.kind == SpaceBuildKind::Storage); REQUIRE(IsOrbitalModuleOperational(*site));
	REQUIRE(craft->position != GetSpaceStructureApproach(site->state)); REQUIRE(ClassifyOrbitalModuleRemoval(*site, craft->owner) == OrbitalRemovalBlock::None);
	// The actual constructor has departed and all original inventory/jobs are
	// empty. Removal must never achieve this by discarding an accountable item.
	for (const auto &stock : site->module->stock) REQUIRE(stock.TotalCount() == 0);
	const auto reference = GetSpaceCraftReference(*craft); const auto holding = ObserveMiningControlCargo(*craft);
	const auto point = craft->position; const auto queue = craft->space_orders.entries; const auto cursor = craft->space_orders.cursor;
	REQUIRE(craft->space_orders.blocked == SpaceOrderBlock::ApproachBusy);
	const auto allowance = craft->manifest->returns_remaining; REQUIRE(craft->cargo.TotalCount() == GetSpaceFreightCapacity(SPACE_PROBE_MODEL, craft->cargo_type));
	AutoRestoreBackup actor(_current_company, craft->owner); AutoRestoreBackup running(_pause_mode, PauseModes{});
	const auto before = ObserveMiningPolicyDecision(craft->owner);
	REQUIRE(CmdRemoveOrbitalModule({}, removed.structure, removed.generation).Succeeded()); CHECK(ObserveMiningPolicyDecision(craft->owner) == before);
	REQUIRE(CmdRemoveOrbitalModule(DoCommandFlag::Execute, removed.structure, removed.generation).Succeeded());
	CHECK_FALSE(ResolveSpaceStructure(removed)); CHECK(ResolveSpaceCraft(reference) == craft); CHECK(ObserveMiningControlCargo(*craft) == holding);
	CHECK(craft->position == point); CHECK(craft->manifest->returns_remaining == allowance); CHECK(craft->space_orders.entries == queue); CHECK(craft->space_orders.cursor == cursor);
	const auto after = ObserveMiningPolicyDecision(craft->owner);
	// Checked removal may update its native history/requests, but earns no field
	// opportunity, random draw, packet output or capital refund.
	for (const size_t owner : {size_t{0}, size_t{1}, size_t{10}, size_t{11}, size_t{12}}) CHECK(after[owner] == before[owner]);
	CHECK(craft->space_orders.blocked == SpaceOrderBlock::TargetRemoved); CHECK(ValidateNativeSpaceOrders());
	const auto before_stale = ObserveMiningPolicyDecision(craft->owner);
	CHECK(CmdRemoveOrbitalModule({}, removed.structure, removed.generation).Failed());
	CHECK(CmdMoveOrbitalFreight(DoCommandFlag::Execute, removed.structure, removed.generation, craft->index, craft->generation, craft->cargo_type, static_cast<uint16_t>(craft->cargo.TotalCount()), false).Failed());
	CHECK_FALSE(ExecuteSpaceReturnArrival(*craft, target, SpaceReturnPurpose::Cargo)); CHECK_FALSE(GetSpaceReturnCandidateAdmission(*craft, target, SpaceReturnPurpose::Cargo));
	CHECK(ObserveMiningPolicyDecision(craft->owner) == before_stale); CHECK_FALSE(FindNearestEligibleSpaceReturn(*craft, SpaceReturnPurpose::Cargo));
	REQUIRE(BlitterFactory::SelectBlitter("8bpp-simple") != nullptr); GfxClearSpriteCache(); FontCache::ClearFontCaches(FONTSIZES_REQUIRED);
	std::ofstream expected(std::filesystem::path(output) / "policy-next.tsv"); REQUIRE(expected.good());
	expected << "# Owner: r2sff1-space-fixes-features-1.md Group31C; actual checked receiver removal and next named wait,13 native observations\n";
	CheckpointMiningRefusal(*craft, output, "policy-receiver-removed", expected);
	REQUIRE(craft->mission.continuation.phase == MiningContinuationPhase::Waiting); REQUIRE(craft->mission.continuation.waiting == MiningContinuationWait::NoReceiver);
	CHECK_FALSE(craft->space_orders.approach); CHECK_FALSE(craft->mission.continuation.effective_target); CHECK(craft->mission.continuation.retry_ticks == GetSpaceMiningTargetRetryDelay());
	// No compatible receiver remains in this declared domain: the other real
	// Core is transport-full. Retry waits retain the actual holding and safe pose.
	for (uint sample = 0; sample < 3; ++sample) {
		const auto retry = craft->mission.continuation.retry_ticks; RunSpaceLifecycleGameOpportunity(); CHECK(craft->mission.continuation.retry_ticks + 1 == retry);
		CHECK(craft->position == point); CHECK(ObserveMiningControlCargo(*craft) == holding); CHECK(craft->manifest->returns_remaining == allowance);
	}
	CheckpointMiningRefusal(*craft, output, "policy-receiver-removed-wait", expected);
	fmt::print("MINING_RECEIVER_REMOVAL original_model=15 removed={} generation={} checked_empty_removal=true original_holding_preserved=true stale_arrival_rejected=true no_receiver_wait=true bounded_retry=true ordinary_opportunities=5\n", removed.structure.base(), removed.generation);
}

/** Stop, safely release, resume and explicitly Recall an original partially loaded Full Load miner.
 * @param output Private stopped/resumed/Recall/stable phase saves, captures and actual next-opportunity oracle.
 * @param model Original15 or21 from its genuine partially worked31B input. Only contained terminal geometry is declared; original commands and ordinary opportunities alone change stopped state, safety, movement, upkeep or cancellation.
 */
static void VerifyMiningStopRecall(const char *output, uint16_t model)
{
	SpaceVehicle *craft = nullptr;
	// Keep the actual paid incarnation, holding and admitted original Mine.
	for (auto *candidate : SpaceVehicle::Iterate()) {
		if (candidate->owner == _local_company && candidate->construction.placement == SpacePlacement::Orbital && GetSpaceVehicleModel(*candidate) == model) { craft = candidate; break; }
	}
	REQUIRE(craft); REQUIRE(GetSpaceMiningTuning(model)); REQUIRE(craft->mission.phase == SpaceMissionPhase::Mining); REQUIRE(craft->mission.work_ticks != 0);
	REQUIRE(craft->cargo.TotalCount() == GetSpaceMiningTuning(model)->extraction_units); const auto capacity = craft->cargo_cap;
	REQUIRE_FALSE(craft->vehstatus.Test(VehState::Stopped)); const auto reference = GetSpaceCraftReference(*craft);
	const auto holding = ObserveMiningControlCargo(*craft); const auto orders = craft->space_orders.entries; const auto cursor = craft->space_orders.cursor;
	REQUIRE(cursor < orders.size()); REQUIRE(orders[cursor].mining_policy == SpaceMiningPolicy::FullLoadNearest); REQUIRE(craft->space_orders.started);
	const auto source_id = craft->mission.target; const auto work = craft->mission.work_ticks; const auto extracted = craft->extracted_units;
	uint32_t reserve = 0;
	{ const auto *source = GetMutableAsteroidBody(source_id); REQUIRE(source); REQUIRE(source->mining_slot == craft->index); reserve = source->remaining_reserve; }
	AutoRestoreBackup actor(_current_company, craft->owner); AutoRestoreBackup running(_pause_mode, PauseModes{});
	const auto before_stop = ObserveMiningPolicyDecision(craft->owner);
	REQUIRE(CmdStartStopVehicle({}, craft->index, true).Succeeded()); CHECK(ObserveMiningPolicyDecision(craft->owner) == before_stop);
	REQUIRE(CmdStartStopVehicle(DoCommandFlag::Execute, craft->index, true).Succeeded()); REQUIRE(craft->vehstatus.Test(VehState::Stopped));
	CHECK(craft->mission.work_ticks == work); CHECK(ObserveMiningControlCargo(*craft) == holding);
	StageMiningControlEdge(*craft); const auto stopped_point = craft->position; const auto profit = craft->profit_this_year;
	uint32_t opportunities = 0;
	/** Offer one native opportunity while preserving already emitted cargo and lifetime work; partial uncommitted work may be released only by actual safety/Recall. */
	auto opportunity = [&] {
		REQUIRE(opportunities++ < 400); RunSpaceLifecycleGameOpportunity(); REQUIRE(ResolveSpaceCraft(reference) == craft);
		CHECK(ObserveMiningControlCargo(*craft) == holding); CHECK(craft->extracted_units == extracted); CHECK(craft->cargo_cap == capacity);
	};
	opportunity();
	REQUIRE(craft->vehstatus.Test(VehState::Stopped)); REQUIRE(craft->mission.continuation.phase == MiningContinuationPhase::Detaching);
	CHECK(craft->mission.work_ticks == 0); CHECK_FALSE(craft->space_orders.approach); CHECK_FALSE(craft->space_orders.started);
	const auto detached = craft->mission.continuation;
	// Independent field safety must release the unsafe lease even though this
	// actual hull remains stopped. No stopped movement/controller/retry work is
	// earned, while the original native idle upkeep still belongs to each tick.
	for (uint sample = 0; sample < 2; ++sample) {
		opportunity(); CHECK(craft->vehstatus.Test(VehState::Stopped)); CHECK(craft->position == stopped_point); CHECK(craft->mission.continuation == detached);
		CHECK(craft->space_orders.entries == orders); CHECK(craft->space_orders.cursor == cursor);
	}
	CHECK(craft->profit_this_year < profit);
	{ const auto *source = GetMutableAsteroidBody(source_id); REQUIRE(source); CHECK(source->mining_slot == VehicleID::Invalid()); CHECK(source->remaining_reserve == reserve); }
	REQUIRE(BlitterFactory::SelectBlitter("8bpp-simple") != nullptr); GfxClearSpriteCache(); FontCache::ClearFontCaches(FONTSIZES_REQUIRED);
	std::ofstream expected(std::filesystem::path(output) / "policy-next.tsv"); REQUIRE(expected.good());
	expected << "# Owner: r2sff1-space-fixes-features-1.md Group31D; original stopped/resumed/checked Recall next decisions,13 synchronized observations\n";
	/** @param phase Actual native phase name. Save/capture without earning work, then record its one original next opportunity. */
	auto checkpoint = [&](std::string_view phase) {
		CheckpointMiningRefusal(*craft, output, phase, expected); ++opportunities; CHECK(ObserveMiningControlCargo(*craft) == holding); CHECK(craft->extracted_units == extracted);
	};
	checkpoint("policy-stopped-edge"); CHECK(craft->vehstatus.Test(VehState::Stopped)); CHECK(craft->position == stopped_point);
	const auto before_resume = ObserveMiningPolicyDecision(craft->owner);
	REQUIRE(CmdStartStopVehicle({}, craft->index, true).Succeeded()); CHECK(ObserveMiningPolicyDecision(craft->owner) == before_resume);
	REQUIRE(CmdStartStopVehicle(DoCommandFlag::Execute, craft->index, true).Succeeded()); CHECK_FALSE(craft->vehstatus.Test(VehState::Stopped));
	// A resumed flag alone does not prove movement. Observe actual local safe
	// departure through the original controller before issuing explicit Recall.
	while (craft->position == stopped_point) { REQUIRE(opportunities < 160); opportunity(); }
	CHECK(craft->space_orders.entries == orders); CHECK(craft->space_orders.cursor == cursor); checkpoint("policy-resumed-edge");
	const auto before_stale = ObserveMiningPolicyDecision(craft->owner);
	CHECK(CmdSetSpaceMissionChecked(DoCommandFlag::Execute, craft->index, craft->generation + 1, INVALID_ORBITAL_BODY.base()).Failed());
	CHECK(ObserveMiningPolicyDecision(craft->owner) == before_stale);
	REQUIRE(CmdSetSpaceMissionChecked(DoCommandFlag::Execute, craft->index, craft->generation, INVALID_ORBITAL_BODY.base()).Succeeded());
	CHECK(craft->space_orders.entries.empty()); CHECK(craft->mission.continuation == SpaceMiningContinuationState{}); CHECK_FALSE(HasSpaceMiningContinuationOrder(*craft)); CHECK(craft->mission.work_ticks == 0);
	// Declare only the old unleased body's final-contained geometry. Its real
	// next exit exercises matching resource notification after cancellation;
	// no direct Notify call or replacement actor/target is manufactured.
	{
		auto field = GetAsteroidField(); REQUIRE(field.surveys.empty()); auto *body = FindAsteroidBody(field, source_id); REQUIRE(body); REQUIRE(body->mining_slot == VehicleID::Invalid());
		const auto bounds = *GetSpacePlaneBounds(field.width, field.height); const auto exit = ForecastAsteroidExit(*body, bounds); REQUIRE(exit); REQUIRE(exit->ticks_until_exit); body->motion = exit->last_safe_motion;
		REQUIRE(InstallAsteroidField(std::move(field))); UpdateAttachedSpaceCraftTransforms();
	}
	checkpoint("policy-recall-requested"); CHECK_FALSE(GetMutableAsteroidBody(source_id));
	// Cross the entire old retry interval after a real matching exit. Recall's
	// cleared original queue/intent can neither reacquire a body nor manufacture
	// a new automatic Mine; ordinary legacy Return/Holding may still move safely.
	for (uint16_t elapsed = 0; elapsed < GetSpaceMiningTargetRetryDelay() + 2; ++elapsed) {
		opportunity(); CHECK(craft->space_orders.entries.empty()); CHECK_FALSE(HasSpaceMiningContinuationOrder(*craft));
		CHECK(craft->mission.continuation == SpaceMiningContinuationState{}); CHECK_FALSE(craft->space_orders.approach);
		CHECK_FALSE((craft->mission.phase == SpaceMissionPhase::Approach || craft->mission.phase == SpaceMissionPhase::Mining));
	}
	checkpoint("policy-recall-stable");
	fmt::print("MINING_STOP_RECALL model={} ordinary_opportunities={} stopped_safety_released=true stopped_idle_upkeep_preserved=true actual_resume_movement=true checked_recall=true stale_generation_inert=true actual_old_source_exit=true cancelled_mine_reacquisitions=0 original_holding_preserved=true declared_terminal_geometry=true\n", model, opportunities);
}

/** Capture immutable Source totals across actual service-part endpoints without retaining packet pointers.
 * @param owners Existing native containers whose combined provenance is observed. These copied totals have no saved or gameplay authority.
 * @return One aggregate per actual complete Source; legitimate native packet splitting/merging cannot appear as lost provenance.
 */
static std::vector<std::pair<Source, uint64_t>> ObserveServiceOrigins(std::initializer_list<const StationCargoList *> owners)
{
	using Origin = std::pair<Source, uint64_t>; std::vector<Origin> result;
	// Count every routing bucket and fragment across the real endpoints. Merge
	// by immutable Source only in this detached observation, never in native stock.
	for (const auto *owner : owners) for (const auto &[hop, packets] : *owner->Packets()) for (const auto *packet : packets) {
		(void)hop; const auto found = std::ranges::find(result, packet->GetSource(), &Origin::first);
		if (found == result.end()) result.emplace_back(packet->GetSource(), packet->Count()); else found->second += packet->Count();
	}
	return result;
}

/** Compare complete copied service-part provenance independently of native merge order.
 * @param actual Totals copied after an actual physical transfer.
 * @param expected Totals copied before that transfer; neither set owns any cargo.
 */
static void CheckServiceOrigins(const std::vector<std::pair<Source, uint64_t>> &actual, const std::vector<std::pair<Source, uint64_t>> &expected)
{
	using Origin = std::pair<Source, uint64_t>; CHECK(actual.size() == expected.size());
	// Each original Source must retain its whole combined quantity, even when
	// native packet identities were legitimately merged at the destination.
	for (const auto &[source, quantity] : expected) {
		const auto found = std::ranges::find(actual, source, &Origin::first); REQUIRE(found != actual.end()); CHECK(found->second == quantity);
	}
}

/** Fund and complete an original paid service owner, then resume the held Full Load Mine.
 * @param output Private supplied/started/working/completed/resumed native phases and actual next-opportunity oracle.
 * @param model Original15's incoming paid rocket supplies its actual parts/shells; original21 physically transfers its existing onboard parts into the underfunded Core and uses its sole real powered service job. No stock, shell, allowance, work, grant or physical arrival is assigned.
 */
static void VerifyMiningServiceResume(const char *output, uint16_t model)
{
	REQUIRE((model == SPACE_PROBE_MODEL || model == SPACE_HEAVY_MINER_MODEL)); const bool module_case = model == SPACE_HEAVY_MINER_MODEL;
	SpaceVehicle *craft = nullptr; SpaceStructure *site = nullptr; OrbitalServiceJob *job = nullptr;
	// Resolve the unchanged original actor from its actual31D.1 held Mine wait.
	for (auto *candidate : SpaceVehicle::Iterate()) {
		if (candidate->owner == _local_company && candidate->construction.placement == SpacePlacement::Orbital && GetSpaceVehicleModel(*candidate) == model) { craft = candidate; break; }
	}
	REQUIRE(craft); REQUIRE(craft->manifest); auto &manifest = *craft->manifest; const auto reference = GetSpaceCraftReference(*craft);
	const auto queue = craft->space_orders.entries; const auto cursor = craft->space_orders.cursor; const auto tuning = GetSpaceServiceTuning(model); REQUIRE(tuning);
	REQUIRE(cursor < queue.size()); REQUIRE(queue[cursor].action == SpaceOrderAction::Mine); REQUIRE(queue[cursor].mining_policy == SpaceMiningPolicy::FullLoadNearest);
	REQUIRE(craft->space_orders.blocked == SpaceOrderBlock::ServiceRequired); REQUIRE(craft->mission.phase == SpaceMissionPhase::Holding);
	REQUIRE(manifest.returns_remaining == 0); REQUIRE(manifest.pending_returns == 0); REQUIRE(manifest.service_ticks == 0); REQUIRE(craft->cargo.TotalCount() == 0);
	const auto services_before = manifest.services_completed; const auto extracted_before = craft->extracted_units;
	AutoRestoreBackup actor(_current_company, craft->owner); AutoRestoreBackup running(_pause_mode, PauseModes{});
	uint32_t opportunities = 0, service_opportunities = 0; uint64_t service_joules = 0, producer_before = 0, module_services_before = 0;
	/** Offer exactly one ordinary opportunity while retaining the original held instruction and incarnation. */
	auto opportunity = [&] {
		REQUIRE(opportunities++ < 5000); RunSpaceLifecycleGameOpportunity(); REQUIRE(ResolveSpaceCraft(reference) == craft);
		CHECK(craft->space_orders.entries == queue); CHECK(craft->space_orders.cursor == cursor);
	};
	if (module_case) {
		// Existing precise physical arrival, not a copied pose, admits the real
		// stock transfer. Preserve both endpoints' complete original part Sources.
		for (auto *candidate : SpaceStructure::Iterate()) {
			if (candidate->state.owner == craft->owner && candidate->state.kind == SpaceBuildKind::Core && IsOrbitalModuleOperational(*candidate) && GetSpaceStructureBerth(*craft, *candidate)) { site = candidate; break; }
		}
		REQUIRE(site); REQUIRE(GetSpaceStructureApproachCapacity(*site) == 1); module_services_before = site->module->services_completed;
		for (uint slot = 0; slot < 2; ++slot) {
			const auto cargo = GetSpaceSupplyCargo(static_cast<SpaceSupply>(slot + 1)); auto &stock = site->module->stock[cargo]; auto &onboard = manifest.supplies[slot + 1];
			REQUIRE(stock.TotalCount() < tuning->inputs[slot]); const auto missing = static_cast<uint16_t>(tuning->inputs[slot] - stock.TotalCount());
			REQUIRE(onboard.AvailableCount() >= missing); const auto before = ObserveServiceOrigins({&stock, &onboard}); const auto units = stock.TotalCount() + onboard.TotalCount();
			const auto owners = ObserveMiningPolicyDecision(craft->owner);
			REQUIRE(CmdMoveOrbitalSupply({}, site->index, site->generation, craft->index, craft->generation, static_cast<uint8_t>(slot + 1), missing, false).Succeeded());
			CHECK(ObserveMiningPolicyDecision(craft->owner) == owners);
			REQUIRE(CmdMoveOrbitalSupply(DoCommandFlag::Execute, site->index, site->generation, craft->index, craft->generation, static_cast<uint8_t>(slot + 1), missing, false).Succeeded());
			CHECK(stock.TotalCount() == tuning->inputs[slot]); CHECK(stock.TotalCount() + onboard.TotalCount() == units); CheckServiceOrigins(ObserveServiceOrigins({&stock, &onboard}), before);
		}
		job = site->module->GetServiceJobs()[0]; REQUIRE(job->remaining_ticks == 0);
	} else {
		SpaceVehicle *rocket = nullptr;
		// The already paid separate rocket targets this original incarnation and
		// carries the actual two shells/parts. Ordinary flight alone may deliver it.
		for (auto *candidate : SpaceVehicle::Iterate()) {
			if (candidate->owner == craft->owner && GetSpaceVehicleModel(*candidate) == 14 && candidate->transition && candidate->manifest &&
				candidate->transition->destination == SpaceOrderTarget{SpaceOrderTargetKind::Craft, craft->index.base(), craft->generation}) { rocket = candidate; break; }
		}
		REQUIRE(rocket); REQUIRE_FALSE(rocket->transition->resupplied); const auto rocket_ref = GetSpaceCraftReference(*rocket); const auto pods = rocket->manifest->pods;
		REQUIRE(pods.size() == 2); REQUIRE(manifest.pods.empty()); std::array<std::vector<std::pair<Source, uint64_t>>, 2> parts;
		for (uint slot = 0; slot < 2; ++slot) {
			REQUIRE(manifest.supplies[slot + 1].TotalCount() == 0); REQUIRE(rocket->manifest->supplies[slot + 1].TotalCount() == tuning->inputs[slot]);
			parts[slot] = ObserveServiceOrigins({&manifest.supplies[slot + 1], &rocket->manifest->supplies[slot + 1]});
		}
		while (!rocket->transition->resupplied) { opportunity(); REQUIRE(ResolveSpaceCraft(rocket_ref) == rocket); CHECK(manifest.returns_remaining == 0); CHECK(manifest.service_ticks == 0); CHECK(craft->extracted_units == extracted_before); }
		CHECK(rocket->manifest->pods.empty()); REQUIRE(manifest.pods.size() == pods.size());
		// Original shell identities and paid receipts cross custody once; no new
		// pod or allowance is manufactured by resupply or by this observer.
		for (const auto pod_ref : pods) {
			CHECK(std::ranges::find(manifest.pods, pod_ref) != manifest.pods.end()); const auto *pod = ResolveSpacePod(pod_ref); REQUIRE(pod);
			CHECK(pod->state.custody == SpacePodCustody::Craft); CHECK(pod->state.craft == reference); CHECK(pod->state.capital_paid > 0);
		}
		for (uint slot = 0; slot < 2; ++slot) {
			CHECK(rocket->manifest->supplies[slot + 1].TotalCount() == 0); CHECK(manifest.supplies[slot + 1].TotalCount() == tuning->inputs[slot]);
			CheckServiceOrigins(ObserveServiceOrigins({&manifest.supplies[slot + 1], &rocket->manifest->supplies[slot + 1]}), parts[slot]);
		}
	}
	REQUIRE(BlitterFactory::SelectBlitter("8bpp-simple") != nullptr); GfxClearSpriteCache(); FontCache::ClearFontCaches(FONTSIZES_REQUIRED);
	std::ofstream expected(std::filesystem::path(output) / "policy-next.tsv"); REQUIRE(expected.good());
	expected << "# Owner: r2sff1-space-fixes-features-1.md Group31D; actual funded service and resumed held Mine,13 synchronized observations\n";
	const auto funded_pods = manifest.pods; const std::array<uint, 2> funded_onboard{manifest.supplies[1].TotalCount(), manifest.supplies[2].TotalCount()};
	/** @param phase Actual saved phase name. Capture/save without work, then record its one ordinary next opportunity. */
	auto checkpoint = [&](std::string_view phase) { CheckpointMiningRefusal(*craft, output, phase, expected); ++opportunities; CHECK(craft->space_orders.entries == queue); CHECK(craft->space_orders.cursor == cursor); };
	checkpoint("policy-service-supplied"); CHECK(manifest.returns_remaining == 0); CHECK(manifest.services_completed == services_before); CHECK(craft->extracted_units == extracted_before);
	if (module_case) {
		REQUIRE(CmdServiceOrbitalProbe(DoCommandFlag::Execute, site->index, site->generation, craft->index, craft->generation, OrbitalServiceControl::Start).Succeeded());
		REQUIRE(job->model == model); REQUIRE(job->craft == reference); REQUIRE(job->remaining_ticks == tuning->ticks);
		const auto *producer = GetUtilityPowerProducer(site->module->producer); REQUIRE(producer); producer_before = producer->consumed_lifetime_j;
		for (uint slot = 0; slot < 2; ++slot) { CHECK(job->inputs[slot].TotalCount() == tuning->inputs[slot]); CHECK(site->module->stock[GetSpaceSupplyCargo(static_cast<SpaceSupply>(slot + 1))].TotalCount() == 0); }
	} else {
		REQUIRE(CmdServiceLaunchProbe(DoCommandFlag::Execute, craft->index, craft->generation, false).Succeeded()); REQUIRE(manifest.service_ticks == tuning->ticks);
	}
	/** @return The actual original selected service owner's saved remaining work; no local counter supplies progress. */
	auto remaining = [&]() -> uint16_t { return module_case ? job->remaining_ticks : manifest.service_ticks; };
	/** @param previous Actual remaining work before one ordinary opportunity. Check its one native debit and, for module work, its actual common-ledger settled grant. */
	auto check_service_step = [&](uint16_t previous) {
		CHECK(remaining() + 1 == previous); ++service_opportunities; CHECK(craft->extracted_units == extracted_before);
		if (module_case) {
			const auto *consumer = GetUtilityPowerConsumer(site->module->consumer); REQUIRE(consumer); uint64_t consumed = 0;
			// Shares are native settled lineage for this Core's sole service
			// consumer; other consumers retain their independent source spending.
			for (const auto &share : consumer->shares) consumed += share.consumed_j;
			CHECK(consumed == GetMultiLayerSpaceBudget().work_j_per_tick); service_joules += consumed;
		}
	};
	checkpoint("policy-service-started"); check_service_step(tuning->ticks);
	// Work is earned only by ordinary opportunities through original parts and
	// native onboard/common-power ownership. No producer is stepped directly.
	while (remaining() > tuning->ticks / 2) { const auto before = remaining(); opportunity(); check_service_step(before); }
	const auto partial = remaining(); checkpoint("policy-service-working"); check_service_step(partial);
	while (remaining() != 0) { const auto before = remaining(); opportunity(); check_service_step(before); }
	CHECK(service_opportunities == tuning->ticks); CHECK(manifest.services_completed == services_before + 1); CHECK(manifest.returns_remaining == tuning->returns); CHECK(manifest.pending_returns == 0);
	if (module_case) {
		CHECK(site->module->services_completed == module_services_before + 1); CHECK(service_joules == uint64_t{tuning->ticks} * GetMultiLayerSpaceBudget().work_j_per_tick);
		const auto *producer = GetUtilityPowerProducer(site->module->producer); REQUIRE(producer); CHECK(producer->consumed_lifetime_j >= producer_before + service_joules);
		// Module work consumes only its reserved bill; remaining original onboard
		// parts stay in their distinct compartments and cannot be spent twice.
		for (uint slot = 0; slot < 2; ++slot) { CHECK(job->inputs[slot].TotalCount() == 0); CHECK(manifest.supplies[slot + 1].TotalCount() == funded_onboard[slot]); }
	} else for (uint slot = 0; slot < 2; ++slot) CHECK(manifest.supplies[slot + 1].TotalCount() == 0);
	CHECK(manifest.pods == funded_pods);
	checkpoint("policy-service-completed");
	// The held Full Load instruction resumes its original native admission,
	// actual pursuit, real lease and finite extraction; a restored allowance
	// alone is not evidence that work resumed on an actual compatible body.
	while (craft->mission.phase != SpaceMissionPhase::Mining) opportunity();
	REQUIRE(craft->cargo.TotalCount() == 0); const auto body = craft->mission.target; uint32_t reserve = 0;
	{
		const auto *source = GetMutableAsteroidBody(body); REQUIRE(source); reserve = source->remaining_reserve; REQUIRE(source->mining_slot == craft->index);
	}
	while (craft->cargo.TotalCount() == 0) opportunity();
	const auto units = GetSpaceMiningTuning(model)->extraction_units; CHECK(craft->cargo.TotalCount() == units); CHECK(craft->extracted_units == extracted_before + units);
	{
		// Field publication invalidates borrowed pointers. Reacquire only this
		// stable actual source after the ordinary extraction opportunities.
		const auto *source = GetMutableAsteroidBody(body); REQUIRE(source); CHECK(source->remaining_reserve + units == reserve);
	}
	const auto origin = Source::FromAsteroid(body.base(), CargoSpec::Get(craft->cargo_type)->label);
	for (const auto &[packet, actual, quantity] : ObserveMiningControlCargo(*craft)) { (void)packet; (void)quantity; CHECK(actual == origin); }
	checkpoint("policy-service-resumed"); CHECK(manifest.services_completed == services_before + 1); CHECK(manifest.returns_remaining == tuning->returns);
	fmt::print("MINING_SERVICE_RESUME model={} ordinary_opportunities={} native_service_opportunities={} actual_service_joules={} actual_units={} actual_body={} original_paid_supplies=true original_full_mine_resumed=true service_completed_once=true\n", model, opportunities, service_opportunities, service_joules, units, body.base());
}

/** Exercise original pending, shell and service-part waits after genuine paid returns.
 * @param scenario service-pending uses the actual15 descent checkpoint; service-shell uses its settled checkpoint and earns real onboard service; service-parts uses the genuine414 two-return/incoming-resupply world; service-module uses21's actual full-return checkpoint and its underfunded original Core.
 * @param output Private native status/save and actual next-opportunity oracle. No cargo, part, shell, allowance, physical placement or work countdown is assigned by preparation.
 */
static void VerifyMiningServiceWait(std::string_view scenario, const char *output)
{
	const bool module_case = scenario == "service-module"; const bool pending_case = scenario == "service-pending";
	const bool shell_case = scenario == "service-shell"; const bool parts_case = scenario == "service-parts";
	REQUIRE((module_case || pending_case || shell_case || parts_case));
	const uint16_t model = module_case ? SPACE_HEAVY_MINER_MODEL : SPACE_PROBE_MODEL;
	SpaceVehicle *craft = nullptr; SpaceStructure *site = nullptr;
	// Resolve the retained role/incarnation. A legacy hull without a manifest
	// cannot acquire another actor's genuine return expenditure or service parts.
	for (auto *candidate : SpaceVehicle::Iterate()) {
		if (candidate->owner == _local_company && candidate->construction.placement == SpacePlacement::Orbital && GetSpaceVehicleModel(*candidate) == model) { craft = candidate; break; }
	}
	REQUIRE(craft); REQUIRE(craft->manifest); REQUIRE(craft->mission.phase == SpaceMissionPhase::Holding); REQUIRE(craft->manifest->service_ticks == 0);
	const auto reference = GetSpaceCraftReference(*craft); auto &manifest = *craft->manifest; const auto holding = ObserveMiningControlCargo(*craft);
	const auto services_before = manifest.services_completed; const auto extracted = craft->extracted_units;
	REQUIRE(craft->cargo.TotalCount() == 0);
	AutoRestoreBackup actor(_current_company, craft->owner); AutoRestoreBackup running(_pause_mode, PauseModes{});
	uint32_t opportunities = 0, service_opportunities = 0; uint8_t mine = craft->space_orders.cursor;
	/** Offer one actual ordinary opportunity and preserve this original holding/extraction count throughout every blocked/service state. */
	auto opportunity = [&] {
		REQUIRE(opportunities++ < 600); RunSpaceLifecycleGameOpportunity(); REQUIRE(ResolveSpaceCraft(reference) == craft);
		CHECK(ObserveMiningControlCargo(*craft) == holding); CHECK(craft->extracted_units == extracted); CHECK(craft->mission.work_ticks == 0);
	};
	if (parts_case) {
		// This original414 source really spent two20-unit returns and already
		// queued its next Mine before launching the separate paid resupply rocket.
		REQUIRE(manifest.returns_remaining == 0); REQUIRE(manifest.pending_returns == 0); REQUIRE(manifest.pods.empty());
		REQUIRE(manifest.supplies[1].TotalCount() == 0); REQUIRE(manifest.supplies[2].TotalCount() == 0); REQUIRE(craft->extracted_units == 40);
		REQUIRE(mine < craft->space_orders.entries.size()); REQUIRE(craft->space_orders.entries[mine].action == SpaceOrderAction::Mine);
		REQUIRE(CmdServiceLaunchProbe({}, craft->index, craft->generation, false).Failed());
	} else {
		REQUIRE(craft->space_orders.cursor == craft->space_orders.entries.size());
		const auto bounds = *GetSpacePlaneBounds(Map::SizeX(), Map::SizeY()); std::optional<OrbitalBodyID> target;
		// Queue a surviving finite original mineral body with time beyond this
		// bounded service observation. Service admission still forbids extraction;
		// neither choosing an order nor making it Full Load grants a lease/work.
		for (const auto id : RankMiningTargetCandidates(GetAsteroidField().bodies, craft->position, CargoLabel{"NORE"})) {
			const auto *body = GetMutableAsteroidBody(id); REQUIRE(body); const auto exit = ForecastAsteroidExit(*body, bounds); REQUIRE(exit);
			if (body->mining_slot == VehicleID::Invalid() && exit->ticks_until_exit && *exit->ticks_until_exit > 600) { target = id; break; }
		}
		REQUIRE(target);
		REQUIRE(CmdEditSpaceOrder(DoCommandFlag::Execute, craft->index, craft->generation, SpaceOrderEdit::Clear, 0, SpaceOrderAction::Mine, SpaceOrderTargetKind::Body, 0, 0).Succeeded());
		if (module_case) {
			// Preserve real Core arrival with an explicit Visit before the next
			// Mine. It re-establishes the same actual berth through normal arbitration
			// after queue editing, without assigning a physical approach or pose.
			for (auto *candidate : SpaceStructure::Iterate()) {
				if (candidate->state.owner == craft->owner && candidate->state.kind == SpaceBuildKind::Core && IsOrbitalModuleOperational(*candidate) && craft->position == GetSpaceStructureApproach(candidate->state)) { site = candidate; break; }
			}
			REQUIRE(site); REQUIRE(manifest.returns_remaining == 0); REQUIRE(manifest.pending_returns == 0);
			REQUIRE(CmdEditSpaceOrder(DoCommandFlag::Execute, craft->index, craft->generation, SpaceOrderEdit::Insert, 0, SpaceOrderAction::Visit, SpaceOrderTargetKind::Structure, site->index.base(), site->generation).Succeeded()); mine = 1;
		} else mine = 0;
		REQUIRE(CmdEditSpaceOrder(DoCommandFlag::Execute, craft->index, craft->generation, SpaceOrderEdit::Insert, mine, SpaceOrderAction::Mine, SpaceOrderTargetKind::Body, target->base(), 0).Succeeded());
		if (module_case) REQUIRE(CmdEditSpaceOrder(DoCommandFlag::Execute, craft->index, craft->generation, SpaceOrderEdit::Insert, 2, SpaceOrderAction::Unload, SpaceOrderTargetKind::Structure, site->index.base(), site->generation).Succeeded());
	}
	REQUIRE(CmdSetSpaceMiningPolicy(DoCommandFlag::Execute, craft->index, craft->generation, mine, SpaceMiningPolicy::FullLoadNearest).Succeeded());
	const auto queue = craft->space_orders.entries;
	const auto initial_block = pending_case ? SpaceOrderBlock::ReturnsPending : SpaceOrderBlock::ServiceRequired;
	// Visit/arbitration and ordinary Mine dispatch must publish the actual named
	// wait. A timeout, removed body or different obligation cannot close this case.
	while (craft->space_orders.cursor != mine || craft->space_orders.blocked != initial_block) { REQUIRE(opportunities < 20); opportunity(); }
	if (pending_case) {
		REQUIRE(manifest.pending_returns == manifest.returns_remaining); REQUIRE(manifest.pending_returns != 0); REQUIRE(manifest.pods.empty());
		CHECK(CmdServiceLaunchProbe({}, craft->index, craft->generation, false).Failed());
	}
	if (module_case) {
		REQUIRE(GetSpaceStructureBerth(*craft, *site)); REQUIRE(GetSpaceStructureApproachCapacity(*site) == 1);
		const auto tuning = GetSpaceServiceTuning(model); REQUIRE(tuning);
		// Real onboard parts remain in their separate compartments; they do not
		// silently fund this selected module's distinct two-part service owner.
		for (uint slot = 0; slot < 2; ++slot) {
			const auto cargo = GetSpaceSupplyCargo(static_cast<SpaceSupply>(slot + 1));
			REQUIRE(site->module->stock[cargo].TotalCount() < tuning->inputs[slot]); REQUIRE(manifest.supplies[slot + 1].TotalCount() >= tuning->inputs[slot]);
		}
		const auto before = ObserveMiningPolicyDecision(craft->owner);
		CHECK(CmdServiceOrbitalProbe({}, site->index, site->generation, craft->index, craft->generation, OrbitalServiceControl::Start).Failed());
		CHECK_FALSE(FindNearestEligibleSpaceReturn(*craft, SpaceReturnPurpose::Service));
		SpaceVehicle *other = nullptr;
		// This real same-world constructor has a funded one-part bill, but the
		// actual heavy owns the sole Core berth. Isolate unavailable physical
		// admission without moving a second hull or inventing another reservation.
		for (auto *candidate : SpaceVehicle::Iterate()) {
			if (candidate->owner == craft->owner && candidate->construction.placement == SpacePlacement::Orbital && GetSpaceVehicleModel(*candidate) == 20 && !candidate->vehstatus.Test(VehState::Stopped)) { other = candidate; break; }
		}
		REQUIRE(other); const auto other_tuning = GetSpaceServiceTuning(20); REQUIRE(other_tuning);
		for (uint slot = 0; slot < 2; ++slot) REQUIRE(site->module->stock[GetSpaceSupplyCargo(static_cast<SpaceSupply>(slot + 1))].TotalCount() >= other_tuning->inputs[slot]);
		CHECK_FALSE(CanAcquireSpaceApproach(*other, {SpaceOrderTargetKind::Structure, site->index.base(), site->generation}));
		CHECK_FALSE(CanNavigateOrbitalService(GetSpaceCraftReference(*other), GetSpaceStructureReference(*site))); CHECK(ObserveMiningPolicyDecision(craft->owner) == before);
	}
	if (shell_case) {
		REQUIRE(manifest.returns_remaining == 0); REQUIRE(manifest.pending_returns == 0); REQUIRE(manifest.pods.empty());
		const auto tuning = GetSpaceServiceTuning(model); REQUIRE(tuning);
		REQUIRE(CmdServiceLaunchProbe(DoCommandFlag::Execute, craft->index, craft->generation, false).Succeeded()); REQUIRE(manifest.service_ticks == tuning->ticks);
		// Genuine paid onboard work consumes its original complete bill once.
		// It renews allowance but never manufactures a separately paid shell.
		while (manifest.service_ticks != 0) { const auto remaining = manifest.service_ticks; opportunity(); ++service_opportunities; CHECK(manifest.service_ticks + 1 == remaining); }
		CHECK(service_opportunities == tuning->ticks); CHECK(manifest.services_completed == services_before + 1); CHECK(manifest.returns_remaining == tuning->returns);
		CHECK(manifest.supplies[1].TotalCount() == 0); CHECK(manifest.supplies[2].TotalCount() == 0); CHECK(manifest.pods.empty());
		opportunity(); REQUIRE(craft->space_orders.blocked == SpaceOrderBlock::PodsMissing); CHECK(CmdServiceLaunchProbe({}, craft->index, craft->generation, false).Failed());
	}
	const auto block = craft->space_orders.blocked; const auto allowance = manifest.returns_remaining;
	REQUIRE(BlitterFactory::SelectBlitter("8bpp-simple") != nullptr); GfxClearSpriteCache(); FontCache::ClearFontCaches(FONTSIZES_REQUIRED);
	std::ofstream expected(std::filesystem::path(output) / "policy-next.tsv"); REQUIRE(expected.good());
	expected << "# Owner: r2sff1-space-fixes-features-1.md Group31D; original paid return/service/shell wait,13 native observations\n";
	const auto phase = fmt::format("policy-{}", scenario); CheckpointMiningRefusal(*craft, output, phase, expected); ++opportunities;
	// Incoming original resupply may genuinely arrive during the parts case;
	// it grants no allowance or hidden service work. All blocked Mines stay held.
	for (uint sample = 0; sample < 3; ++sample) { opportunity(); CHECK(craft->space_orders.blocked == block); CHECK(manifest.service_ticks == 0); CHECK(manifest.returns_remaining == allowance); }
	CHECK(craft->space_orders.entries == queue); CHECK(craft->space_orders.cursor == mine); CHECK_FALSE(craft->space_orders.started);
	if (!shell_case) CHECK(manifest.services_completed == services_before);
	fmt::print("MINING_SERVICE_WAIT model={} scenario={} block={} ordinary_opportunities={} actual_preparation_service_opportunities={} genuine_return_expenditure=true held_full_mine=true unearned_work=false original_holding_preserved=true\n", model, scenario, to_underlying(block), opportunities, service_opportunities);
}

/** Observe resource-specific wake behavior through real native exits while original mining waits safely.
 * @param output Private unrelated-exit/before-wake/after-wake phase saves, captures and actual next-opportunity oracle.
 * @param model Original15 or21 loaded from its genuine31C.1 NoCompatibleAsteroid save. Only contained terminal geometry is declared; events, credit, countdown, retry and history changes belong exclusively to ordinary native opportunities.
 */
static void VerifyMiningMineralWake(const char *output, uint16_t model)
{
	SpaceVehicle *craft = nullptr;
	// Follow the actual waiting role and paid holding; another incarnation may
	// not borrow this saved mineral pin, lease history or original instruction.
	for (auto *candidate : SpaceVehicle::Iterate()) {
		if (candidate->owner == _local_company && candidate->construction.placement == SpacePlacement::Orbital && GetSpaceVehicleModel(*candidate) == model) { craft = candidate; break; }
	}
	REQUIRE(craft); REQUIRE(craft->mission.continuation.phase == MiningContinuationPhase::Waiting);
	REQUIRE(craft->mission.continuation.waiting == MiningContinuationWait::NoCompatibleAsteroid);
	const auto reference = GetSpaceCraftReference(*craft); const auto holding = ObserveMiningControlCargo(*craft);
	const auto queue = craft->space_orders.entries; const auto cursor = craft->space_orders.cursor; const auto point = craft->position;
	const auto first = craft->mission.continuation.previous_body; const auto *original = GetMutableAsteroidBody(first); REQUIRE(original);
	const auto resource = original->resource_class; const auto bounds = *GetSpacePlaneBounds(Map::SizeX(), Map::SizeY());
	const auto exit = ForecastAsteroidExit(*original, bounds); REQUIRE(exit); REQUIRE(exit->ticks_until_exit); REQUIRE(*exit->ticks_until_exit > 4);
	std::optional<OrbitalBodyID> unrelated;
	// Use a surviving original different-mineral body. Its real exit must earn
	// size credit without waking a wait pinned to the original holding's mineral.
	for (const auto &body : GetAsteroidField().bodies) {
		if (body.resource_class != resource && body.mining_slot == VehicleID::Invalid()) { unrelated = body.id; break; }
	}
	REQUIRE(unrelated); AutoRestoreBackup running(_pause_mode, PauseModes{});
	/** @param id Existing unleased body whose unchanged heading is projected to its final-contained point. No time, stock, credit, RNG or retry owner is changed by declaring this geometry. */
	auto stage_terminal = [&](OrbitalBodyID id) {
		auto field = GetAsteroidField(); REQUIRE(field.surveys.empty()); const auto ledger = field.renewal; const auto elapsed = field.elapsed_ticks;
		auto *body = FindAsteroidBody(field, id); REQUIRE(body); REQUIRE(body->mining_slot == VehicleID::Invalid());
		const auto forecast = ForecastAsteroidExit(*body, bounds); REQUIRE(forecast); REQUIRE(forecast->ticks_until_exit); body->motion = forecast->last_safe_motion;
		REQUIRE(InstallAsteroidField(std::move(field))); UpdateAttachedSpaceCraftTransforms(); CHECK(GetAsteroidField().renewal == ledger); CHECK(GetAsteroidField().elapsed_ticks == elapsed);
	};
	/** Assert unchanged paid freight/order/pose while native clocks and ordinary idle upkeep remain free to advance. */
	auto check_wait = [&] {
		REQUIRE(ResolveSpaceCraft(reference) == craft); CHECK(ObserveMiningControlCargo(*craft) == holding); CHECK(craft->position == point);
		CHECK(craft->space_orders.entries == queue); CHECK(craft->space_orders.cursor == cursor); CHECK_FALSE(craft->space_orders.approach);
		CHECK(craft->mission.continuation.waiting == MiningContinuationWait::NoCompatibleAsteroid); CHECK_FALSE(craft->mission.continuation.effective_target);
	};
	const auto unrelated_retry = craft->mission.continuation.retry_ticks; REQUIRE(unrelated_retry > 3); stage_terminal(*unrelated);
	RunSpaceLifecycleGameOpportunity(); CHECK_FALSE(GetMutableAsteroidBody(*unrelated)); CHECK(craft->mission.continuation.retry_ticks + 1 == unrelated_retry); check_wait();
	REQUIRE(BlitterFactory::SelectBlitter("8bpp-simple") != nullptr); GfxClearSpriteCache(); FontCache::ClearFontCaches(FONTSIZES_REQUIRED);
	std::ofstream expected(std::filesystem::path(output) / "policy-next.tsv"); REQUIRE(expected.good());
	expected << "# Owner: r2sff1-space-fixes-features-1.md Group31C; actual mineral-specific native exit wake,13 synchronized observations\n";
	CheckpointMiningRefusal(*craft, output, "policy-mineral-unrelated-exit", expected); check_wait();
	stage_terminal(first); const auto matching_retry = craft->mission.continuation.retry_ticks; REQUIRE(matching_retry > 1);
	const auto credit = GetAsteroidField().renewal.credits; REQUIRE(GetAsteroidField().renewal.ticks_remaining > 3);
	// This saved next opportunity really retires the matching source. Its native
	// publication wakes retry before Vehicle ticks; the failed actual reselection
	// rearms74 rather than silently consuming the previous retry's next tick.
	CheckpointMiningRefusal(*craft, output, "policy-mineral-before-wake", expected);
	CHECK_FALSE(GetMutableAsteroidBody(first)); CHECK(GetAsteroidField().renewal.credits > credit);
	CHECK(craft->mission.continuation.retry_ticks == GetSpaceMiningTargetRetryDelay()); check_wait();
	CheckpointMiningRefusal(*craft, output, "policy-mineral-after-wake", expected);
	CHECK(craft->mission.continuation.retry_ticks + 1 == GetSpaceMiningTargetRetryDelay()); check_wait();
	fmt::print("MINING_MINERAL_WAKE model={} unrelated_body={} matching_body={} unrelated_retry_preserved=true actual_matching_exit_woke=true native_retry_rearmed={} original_holding_preserved=true ordinary_opportunities=4 declared_terminal_geometry=true\n",
		model, unrelated->base(), first.base(), GetSpaceMiningTargetRetryDelay());
}

/** Observe a genuine missing-mineral wait after native exits remove matching availability.
 * @param output Private native wait capture/save and actual next-opportunity oracle.
 * @param model Original15 or21, resolved from its unchanged retained partial-holding input. Only final-contained geometry is declared; original reserves, cargo, credit and retry timing are never assigned.
 */
static void VerifySpaceMiningNoMineral(const char *output, uint16_t model)
{
	SpaceVehicle *craft = nullptr;
	// Use the original deployed incarnation; another role cannot borrow its
	// actual holding, native allowance or existing Full Load instruction.
	for (auto *candidate : SpaceVehicle::Iterate()) {
		if (candidate->owner == _local_company && candidate->construction.placement == SpacePlacement::Orbital && GetSpaceVehicleModel(*candidate) == model) { craft = candidate; break; }
	}
	REQUIRE(craft); REQUIRE(craft->mission.phase == SpaceMissionPhase::Mining);
	const auto reference = GetSpaceCraftReference(*craft); const auto first = craft->mission.target; const auto cursor = craft->space_orders.cursor;
	REQUIRE(cursor < craft->space_orders.entries.size()); REQUIRE(craft->space_orders.entries[cursor].mining_policy == SpaceMiningPolicy::FullLoadNearest);
	REQUIRE(craft->space_orders.started); REQUIRE(craft->cargo.TotalCount() == GetSpaceMiningTuning(model)->extraction_units);
	const auto orders = craft->space_orders.entries; const auto holding = ObserveMiningControlCargo(*craft);
	const auto *source = GetMutableAsteroidBody(first); REQUIRE(source); const auto resource = source->resource_class;
	REQUIRE(GetAsteroidField().renewal.credits == 0); REQUIRE(GetAsteroidField().renewal.ticks_remaining == 0);
	AutoRestoreBackup running(_pause_mode, PauseModes{}); StageMiningControlEdge(*craft);
	auto field = GetAsteroidField(); const auto bounds = *GetSpacePlaneBounds(field.width, field.height); std::vector<OrbitalBodyID> terminal;
	// Preserve each original heading/reserve and declare final-contained centres
	// only for the other matching mineral bodies. The ordinary field owner alone
	// will retire them, remove their live rows and arm genuine size credit.
	for (auto &body : field.bodies) {
		if (body.id == first || body.resource_class != resource) continue;
		REQUIRE(body.mining_slot == VehicleID::Invalid()); const auto exit = ForecastAsteroidExit(body, bounds); REQUIRE(exit); REQUIRE(exit->ticks_until_exit);
		body.motion = exit->last_safe_motion; terminal.push_back(body.id);
	}
	REQUIRE_FALSE(terminal.empty()); REQUIRE(InstallAsteroidField(std::move(field))); UpdateAttachedSpaceCraftTransforms();
	CHECK(ObserveMiningControlCargo(*craft) == holding); uint32_t opportunities = 0;
	/** Offer one real ordinary opportunity; the local count bounds observation and supplies no private simulation progress. */
	auto opportunity = [&] {
		REQUIRE(opportunities++ < 5000); RunSpaceLifecycleGameOpportunity(); REQUIRE(ResolveSpaceCraft(reference) == craft);
		CHECK(craft->space_orders.entries == orders); CHECK(craft->space_orders.cursor == cursor); CHECK(ObserveMiningControlCargo(*craft) == holding);
	};
	// Stop at the first actual named native wait. An unrelated service/receiver
	// refusal is an unmet source precondition, not proof of absent mineral.
	while (craft->mission.continuation.phase != MiningContinuationPhase::Waiting) opportunity();
	REQUIRE(craft->mission.continuation.waiting == MiningContinuationWait::NoCompatibleAsteroid);
	CHECK_FALSE(craft->space_orders.approach); CHECK_FALSE(ResolveEffectiveSpaceOrderTarget(*craft)); CHECK(craft->mission.work_ticks == 0);
	CHECK_FALSE(craft->vehstatus.Test(VehState::Stopped)); CHECK(craft->mission.continuation.retry_ticks == GetSpaceMiningTargetRetryDelay());
	for (const auto body : terminal) CHECK_FALSE(GetMutableAsteroidBody(body));
	CHECK(GetAsteroidField().renewal.credits != 0); REQUIRE(GetAsteroidField().renewal.ticks_remaining > 4);
	// A centre may still be live after its attachment became unsafe. It remains
	// non-interceptable rather than being manually erased to obtain this wait.
	if (const auto *body = GetMutableAsteroidBody(first)) {
		CHECK(body->mining_slot == VehicleID::Invalid()); const auto exit = ForecastAsteroidExit(*body, bounds); REQUIRE(exit); REQUIRE(exit->ticks_until_exit); REQUIRE(*exit->ticks_until_exit > 4);
	}
	const auto before_query = ObserveMiningPolicyDecision(craft->owner);
	CHECK_FALSE(FindNearestEligibleMiningTarget(*craft, craft->mission.continuation.mineral));
	CHECK(ObserveMiningPolicyDecision(craft->owner) == before_query); const auto point = craft->position;
	// Before the freshly armed interval or any matching terminal event, ordinary
	// waits consume one saved retry tick and neither move nor reacquire a lease.
	for (uint sample = 0; sample < 3; ++sample) {
		const auto retry = craft->mission.continuation.retry_ticks; REQUIRE(retry != 0); opportunity();
		CHECK(craft->position == point); CHECK(craft->mission.continuation.retry_ticks + 1 == retry);
		CHECK(craft->mission.continuation.waiting == MiningContinuationWait::NoCompatibleAsteroid);
	}
	REQUIRE(BlitterFactory::SelectBlitter("8bpp-simple") != nullptr); GfxClearSpriteCache(); FontCache::ClearFontCaches(FONTSIZES_REQUIRED);
	const auto before = ObserveMiningPolicyDecision(craft->owner); ShowSpaceSurveyWindow(reference);
	auto *window = FindWindowById(WindowClass::SpaceSurvey, 0); REQUIRE(window); auto *scroll = window->GetScrollbar(WID_SS_SCROLLBAR);
	scroll->SetPosition(0); CaptureSpaceFixesWindow(*window, std::filesystem::path(output) / "policy-no-mineral-top.ppm");
	scroll->SetPosition(scroll->GetCount()); CaptureSpaceFixesWindow(*window, std::filesystem::path(output) / "policy-no-mineral-bottom.ppm");
	const auto saved = (std::filesystem::path(output) / "policy-no-mineral.sav").string();
	{
		AutoRestoreBackup paused(_pause_mode, PauseModes{PauseMode::Normal});
		REQUIRE(SaveOrLoad(saved, SaveLoadOperation::Save, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Ok);
		REQUIRE(SaveOrLoad(saved, SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Ok);
	}
	CHECK(ObserveMiningPolicyDecision(craft->owner) == before); opportunity();
	std::ofstream expected(std::filesystem::path(output) / "policy-next.tsv"); REQUIRE(expected.good());
	expected << "# Owner: r2sff1-space-fixes-features-1.md Group31C; actual saved NoCompatibleAsteroid next opportunity,13 original owner observations\npolicy-no-mineral";
	for (const auto value : ObserveMiningPolicyDecision(craft->owner)) expected << '\t' << value;
	expected << '\n'; expected.flush(); REQUIRE(expected.good());
	fmt::print("MINING_NO_MINERAL model={} ordinary_opportunities={} matching_exits={} original_holding_preserved=true safe_named_wait=true bounded_retry=true declared_terminal_geometry=true\n", model, opportunities, terminal.size());
}

/** Compare one real post-load opportunity with its uninterrupted saved-phase oracle.
 * @param source Immutable phase save beside the originating policy-next.tsv.
 * @param output Private replay directory for native status and reachable-last-row captures.
 * @param model Original role whose restored status is captured; default21 preserves the preceding heavy replay inputs.
 */
static void VerifySpaceMiningPolicyReplay(const char *source, const char *output, uint16_t model = SPACE_HEAVY_MINER_MODEL)
{
	const std::filesystem::path saved{source}; const auto phase = saved.stem().string();
	std::ifstream input(saved.parent_path() / "policy-next.tsv"); REQUIRE(input.good());
	std::optional<std::array<uint64_t, 13>> expected; std::string line;
	// Match only this actual saved phase. Headers and other checkpoints have no
	// authority over its single next decision and are never applied to the world.
	while (std::getline(input, line)) {
		std::istringstream row(line); std::string name; row >> name;
		if (name != phase) continue;
		REQUIRE_FALSE(expected); expected.emplace();
		for (auto &value : *expected) REQUIRE(static_cast<bool>(row >> value));
	}
	REQUIRE(expected); REQUIRE(Company::IsValidID(_local_company));
	AutoRestoreBackup running(_pause_mode, PauseModes{});
	RunSpaceLifecycleGameOpportunity(); const auto actual = ObserveMiningPolicyDecision(_local_company);
	// Report the first differing original owner without hiding later failures;
	// independent field RNG, native cargo/work and both global words participate.
	for (size_t owner = 0; owner < actual.size(); ++owner) {
		INFO("saved phase=" << phase << " synchronized observation=" << owner);
		CHECK(actual[owner] == (*expected)[owner]);
	}
	SpaceVehicle *craft = nullptr;
	for (auto *candidate : SpaceVehicle::Iterate()) {
		// Follow the same actual paid hull; a reused pool ID cannot replace the
		// original actor restored through the ordinary save decoder.
		if (candidate->owner == _local_company && candidate->construction.placement == SpacePlacement::Orbital && GetSpaceVehicleModel(*candidate) == model) { craft = candidate; break; }
	}
	REQUIRE(craft); REQUIRE(BlitterFactory::SelectBlitter("8bpp-simple") != nullptr); GfxClearSpriteCache(); FontCache::ClearFontCaches(FONTSIZES_REQUIRED);
	ShowSpaceSurveyWindow(GetSpaceCraftReference(*craft)); auto *window = FindWindowById(WindowClass::SpaceSurvey, 0); REQUIRE(window);
	const auto before = ObserveMiningPolicyDecision(_local_company); auto *scroll = window->GetScrollbar(WID_SS_SCROLLBAR);
	CHECK(window->top + window->height <= GetMainViewBottom()); CHECK(window->left + window->width <= _screen.width);
	// A reloaded policy must remain reachable through its original retained
	// controls at the requested font/client scale, even with wrapped status rows.
	for (WidgetID id : {WID_SS_POLICY_LEGACY, WID_SS_POLICY_RETURN, WID_SS_POLICY_FULL}) {
		const auto rect = window->GetWidget<NWidgetBase>(id)->GetCurrentRect();
		CHECK(window->top + rect.bottom < GetMainViewBottom()); CHECK(window->left + rect.right < _screen.width);
		CHECK(GetWidgetFromPos(window, (rect.left + rect.right) / 2, (rect.top + rect.bottom) / 2) == id);
	}
	scroll->SetPosition(0); CaptureSpaceFixesWindow(*window, std::filesystem::path(output) / "policy-replay-top.ppm");
	scroll->SetPosition(scroll->GetCount()); CaptureSpaceFixesWindow(*window, std::filesystem::path(output) / "policy-replay-bottom.ppm");
	CHECK(scroll->GetPosition() + scroll->GetCapacity() >= scroll->GetCount());
	CHECK(ObserveMiningPolicyDecision(_local_company) == before);
	fmt::print("MINING_POLICY_REPLAY phase={} next_opportunity=true synchronized_owners=13 actual_status_last_row_capture=true\n", phase);
}

/** @copydoc CaptureConstructionViews */
void CaptureConstructionViews(const SpaceVehicle &builder, const SpaceStructure &site, std::string_view phase)
{
	const char *output = std::getenv("OTTD_SPACE_FIXES_OUTPUT");
	REQUIRE(output != nullptr);
	REQUIRE(BlitterFactory::SelectBlitter("8bpp-simple") != nullptr);
	GfxClearSpriteCache();
	FontCache::ClearFontCaches(FONTSIZES_REQUIRED);
	const auto fleet = ComputeSpaceVehicleDigest(), structures = ComputeSpaceStructureDigest();
	const auto field = ComputeAsteroidFieldDigest(GetAsteroidField()), power = ComputeUtilityPowerDigest();
	const auto manifest = ComputeSpaceManifestDigest();
	AutoRestoreBackup pause(_pause_mode, PauseModes{});
	const auto builder_text = GetSpaceConstructionBuilderText(builder);
	REQUIRE_FALSE(builder_text.empty());
	CHECK(builder_text.front().find(GetSpaceConstructionReasonText(GetOrbitalConstructionReadiness(builder).block)) != std::string::npos);
	// Native string parameters are typed: passing raw text to a StringID slot
	// must fail the fixture even if the window geometry and world digests match.
	for (const auto &line : builder_text) CHECK(line.find("(invalid parameter)") == std::string::npos);
	ShowSpaceConstructionWindow(GetSpaceStructureReference(site));
	ShowSpaceSurveyWindow(GetSpaceCraftReference(builder));
	// Both native windows must fit the actual client. Scroll to the final row
	// as well as the initial status so captures prove all content is reachable.
	for (const auto cls : {WindowClass::SpaceConstruction, WindowClass::SpaceSurvey}) {
		auto *window = FindWindowById(cls, 0);
		REQUIRE(window != nullptr);
		CHECK(window->left >= 0);
		CHECK(window->top >= GetMainViewTop());
		CHECK(window->left + window->width <= _screen.width);
		CHECK(window->top + window->height <= GetMainViewBottom());
		const auto name = cls == WindowClass::SpaceConstruction ? "construction" : "inspector";
		auto *scroll = window->GetScrollbar(cls == WindowClass::SpaceConstruction ? WID_SC_SCROLLBAR : 9);
		scroll->SetPosition(0);
		CaptureSpaceFixesWindow(*window, std::filesystem::path(output) / fmt::format("{}-{}.ppm", phase, name));
		scroll->SetPosition(scroll->GetCount());
		CaptureSpaceFixesWindow(*window, std::filesystem::path(output) / fmt::format("{}-{}-bottom.ppm", phase, name));
		scroll->SetPosition(0);
		if (cls == WindowClass::SpaceSurvey) {
			// Every retained inspector action remains hit-testable on the visible
			// client even after long readiness text has expanded the scroll content.
			for (WidgetID id : std::array<WidgetID, 13>{1, 2, 3, 4, 5, 6, 7, 8, 10, 11, WID_SS_POLICY_LEGACY, WID_SS_POLICY_RETURN, WID_SS_POLICY_FULL}) {
				const Rect rect = window->GetWidget<NWidgetBase>(id)->GetCurrentRect();
				CHECK(GetWidgetFromPos(window, (rect.left + rect.right) / 2, (rect.top + rect.bottom) / 2) == id);
				CHECK(window->top + rect.bottom < GetMainViewBottom());
			}
		}
		CloseWindowByClass(cls);
	}
	Window::DeleteClosedWindows();
	// Opening, painting, scrolling, closing and unrelated camera selection must
	// preserve the same construction/cargo outcome at this simulation boundary.
	SetViewportLayer(GetMainWindow(), MapLayer::Surface);
	SetViewportLayer(GetMainWindow(), MapLayer::Space);
	CHECK(ComputeSpaceVehicleDigest() == fleet);
	CHECK(ComputeSpaceStructureDigest() == structures);
	CHECK(ComputeAsteroidFieldDigest(GetAsteroidField()) == field);
	CHECK(ComputeUtilityPowerDigest() == power);
	CHECK(ComputeSpaceManifestDigest() == manifest);
}

/**
 * Verify pages, wrapping, selection and real command clicks against loaded owners.
 * @param window Native construction window; all paid sites come from the source save.
 * @param output Repository-local directory for native page captures.
 */
static void VerifySpaceFixesLayout(Window &window, const std::filesystem::path &output)
{
	const std::array<std::vector<WidgetID>, 5> actions{{
		{WID_SC_PLACE, WID_SC_QUEUE, WID_SC_CANCEL, WID_SC_CLEAR, WID_SC_ACTIVATE, WID_SC_RESERVE, WID_SC_VISIT},
		{WID_SC_LOAD_STEEL, WID_SC_LOAD_GOODS, WID_SC_UNLOAD, WID_SC_COLLECT_STEEL, WID_SC_COLLECT_GOODS, WID_SC_UNLOAD_METAL, WID_SC_UNLOAD_ELECTRONICS, WID_SC_LOAD_METAL, WID_SC_LOAD_ELECTRONICS},
		{WID_SC_NEXT_SERVICE, WID_SC_NEXT_BERTH, WID_SC_START_SERVICE, WID_SC_CANCEL_SERVICE, WID_SC_PAUSE_SERVICE, WID_SC_RESUME_SERVICE, WID_SC_TOGGLE_PANEL, WID_SC_COMMISSION},
		{WID_SC_CONNECT, WID_SC_NEXT_SOURCE, WID_SC_DISCONNECT, WID_SC_REMOVE, WID_SC_EVACUATE, WID_SC_NEXT_DESTINATION},
		{WID_SC_NEXT_MODEL, WID_SC_NEXT_INPUT_SOURCE, WID_SC_QUEUE_MANUFACTURE, WID_SC_CANCEL_MANUFACTURE},
	}};
	const auto structures = ComputeSpaceStructureDigest();
	const auto vehicles = ComputeSpaceVehicleDigest();
	const auto field = ComputeAsteroidFieldDigest(GetAsteroidField());
	// Every actual site's content must survive paging and scrolling. Selecting a
	// completed Core or Assembly must not reinterpret the unfinished Storage row.
	for (const auto *site : SpaceStructure::Iterate()) {
		if (site->state.owner != _local_company) continue;
		ShowSpaceConstructionWindow(GetSpaceStructureReference(*site));
		const auto selection = window.GetWidgetString(WID_SC_SITE, STR_SPACE_CONSTRUCTION_SELECT);
		for (size_t page = 0; page < actions.size(); ++page) {
			ClickSpaceFixesWidget(window, WID_SC_PAGE);
			CloseWindowByClass(WindowClass::DropdownMenu);
			window.OnDropdownSelect(WID_SC_PAGE, static_cast<int>(page), -1);
			REQUIRE(window.top >= GetMainViewTop());
			REQUIRE(window.top + window.height <= GetMainViewBottom());
			REQUIRE(window.left + window.width <= _screen.width);
			CHECK(window.GetWidgetString(WID_SC_SITE, STR_SPACE_CONSTRUCTION_SELECT) == selection);
			// Check each original action's actual hit rectangle; inactive pages may
			// retain widget nodes but cannot substitute for reachable active actions.
			for (WidgetID id : actions[page]) {
				const Rect rect = window.GetWidget<NWidgetBase>(id)->GetCurrentRect();
				CHECK(window.top + rect.bottom < GetMainViewBottom());
				CHECK(window.left + rect.right < _screen.width);
				CHECK(GetWidgetFromPos(&window, (rect.left + rect.right) / 2, (rect.top + rect.bottom) / 2) == id);
			}
			auto *scroll = window.GetScrollbar(WID_SC_SCROLLBAR);
			scroll->SetPosition(scroll->GetCount());
			CaptureSpaceFixesWindow(window, output / fmt::format("site-{}-page-{}.ppm", site->index.base(), page));
			scroll->SetPosition(0);
		}
	}
	CHECK(ComputeSpaceStructureDigest() == structures);
	CHECK(ComputeSpaceVehicleDigest() == vehicles);
	CHECK(ComputeAsteroidFieldDigest(GetAsteroidField()) == field);
	CHECK(window.nested_root->resize_x == 1);
	CHECK(window.nested_root->resize_y == 1);
	const auto old_capacity = window.GetScrollbar(WID_SC_SCROLLBAR)->GetCapacity();
	ResizeWindow(&window, 0, -static_cast<int>(window.height), false, false);
	CHECK(window.GetScrollbar(WID_SC_SCROLLBAR)->GetCapacity() <= old_capacity);
	CHECK(window.GetScrollbar(WID_SC_SCROLLBAR)->GetPosition() <= std::max(0, window.GetScrollbar(WID_SC_SCROLLBAR)->GetCount() - window.GetScrollbar(WID_SC_SCROLLBAR)->GetCapacity()));
	// The bottom service row must dispatch its real synchronized panel action.
	// Toggle twice while paused, retaining the original operating state and stock.
	for (const auto *site : SpaceStructure::Iterate()) {
		if (site->state.kind != SpaceBuildKind::Core) continue;
		if (!IsOrbitalModuleOperational(*site)) continue;
		ShowSpaceConstructionWindow(GetSpaceStructureReference(*site));
		window.OnDropdownSelect(WID_SC_PAGE, to_underlying(SpaceConstructionPage::Service), -1);
		REQUIRE_FALSE(window.IsWidgetDisabled(WID_SC_TOGGLE_PANEL));
		const bool enabled = site->module->power_enabled;
		ClickSpaceFixesWidget(window, WID_SC_TOGGLE_PANEL);
		CHECK(site->module->power_enabled != enabled);
		ClickSpaceFixesWidget(window, WID_SC_TOGGLE_PANEL);
		CHECK(site->module->power_enabled == enabled);
		break;
	}
	{
		AutoRestoreBackup width(_screen.width, 1280);
		AutoRestoreBackup height(_screen.height, 720);
		ScreenSizeChanged();
		ReInitAllWindows(false);
		CHECK(window.left + window.width <= _screen.width);
		CHECK(window.top + window.height <= GetMainViewBottom());
		CaptureSpaceFixesWindow(window, output / "changed-resolution.ppm");
	}
	ScreenSizeChanged();
	ReInitAllWindows(false);
	fmt::print("LAYOUT pages=5 native_hit_testing=true paint_owners_unchanged=true\n");
}

/**
 * Record exact queue and material owners without changing player instructions.
 * @param phase Human-readable boundary before or after ordinary unpaused ticks.
 */
static void ReportSpaceFixesState(std::string_view phase)
{
	fmt::print("STATE {} tick={} paused={}\n", phase, GetAsteroidField().elapsed_ticks, !_pause_mode.None());
	// The report distinguishes idle paid builders from a stalled Construct action;
	// every queue entry retains its exact target domain and incarnation.
	for (const auto *craft : SpaceVehicle::Iterate()) {
		if (GetSpaceVehicleModel(*craft) != 20) continue;
		fmt::print("BUILDER id={} generation={} stopped={} placement={} position={},{} cargo={} cargo_type={} cursor={} started={} blocked={} timer={} service_ticks={} service_paused={} eligible={}\n",
			craft->index.base(), craft->generation, craft->vehstatus.Test(VehState::Stopped), to_underlying(craft->construction.placement),
			craft->position.x, craft->position.y, craft->cargo.TotalCount(), craft->cargo_type,
			craft->space_orders.cursor, craft->space_orders.started, to_underlying(craft->space_orders.blocked), craft->space_orders.timer_ticks,
			craft->manifest ? craft->manifest->service_ticks : 0, craft->manifest ? craft->manifest->service_paused : false,
			IsSpaceConstructionWorkTick(*craft));
		for (const auto &order : craft->space_orders.entries) {
			fmt::print("ORDER action={} kind={} id={} generation={}\n", to_underlying(order.action), to_underlying(order.target.kind), order.target.id, order.target.generation);
		}
	}
	// Input packets and committed work are independent evidence: neither an empty
	// builder nor an inactive frame alone proves a failed construction controller.
	for (const auto *site : SpaceStructure::Iterate()) {
		fmt::print("SITE id={} generation={} kind={} phase={} location={},{} work={} steel={} goods={} consumed={},{}\n",
			site->index.base(), site->generation, to_underlying(site->state.kind), to_underlying(site->state.phase), site->state.x, site->state.y,
			site->job.work_ticks, site->job.inputs[0].TotalCount(), site->job.inputs[1].TotalCount(), site->job.consumed[0], site->job.consumed[1]);
		if (site->module) {
			fmt::print("MODULE active={} panel_enabled={} panel_watts={} source_id={} source_generation={} work_fraction={} work_remainder={}\n",
				IsOrbitalModuleOperational(*site), site->module->power_enabled, GetOrbitalModulePanelRate(*site),
				site->module->utility_source.structure.base(), site->module->utility_source.generation, site->job.work_fraction, site->job.work_remainder);
		}
	}
}

/**
 * Preview the immutable retained candidate using the native decoder while preserving the open world's original owners.
 * @param source Repository-local retained save already loaded by this focused process.
 */
static void VerifyAsteroidIdentityPreview(const char *source)
{
	const auto field = GetAsteroidField();
	const auto fleet = ComputeSpaceVehicleDigest(), structures = ComputeSpaceStructureDigest();
	const auto power = ComputeUtilityPowerDigest(), manifest = ComputeSpaceManifestDigest();
	const auto clock = TimerGameTick::counter;
	const auto random = std::array{_random.state[0], _random.state[1], _interactive_random.state[0], _interactive_random.state[1]};
	uint unsupported_roles = 0;
	// Actual retained non-mining roles cannot acquire a Mine policy, including
	// in a mixed query/execute call. The surrounding native digests prove these
	// direct pending-command queries change no hull, work or packet ownership.
	for (const auto *craft : SpaceVehicle::Iterate()) {
		if (GetSpaceMiningTuning(GetSpaceVehicleModel(*craft)) != nullptr) continue;
		AutoRestoreBackup actor(_current_company, craft->owner);
		CHECK(CmdSetSpaceMiningPolicy(DoCommandFlags{DoCommandFlag::Execute, DoCommandFlag::QueryCost}, craft->index,
			craft->generation, 0, SpaceMiningPolicy::FullLoadNearest).Failed());
		++unsupported_roles;
	}
	fmt::print("POLICY_NATIVE unsupported_roles={} query_only=true\n", unsupported_roles);
	REQUIRE(SaveOrLoad(source, SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Ok);
	CHECK(_load_check_data.asteroid_present);
	CHECK(_load_check_data.asteroid_body_count == field.bodies.size());
	CHECK(_load_check_data.asteroid_next_body_id == field.next_body_id);
	CHECK(_load_check_data.asteroid_body_rows == GetAsteroidIdentityRecords(field));
	CHECK(ValidateAsteroidIdentityRecords(_load_check_data.asteroid_body_rows, _load_check_data.asteroid_next_body_id));
	CHECK(ValidateAsteroidKnowledgeRecords(_load_check_data.asteroid_knowledge_rows, _load_check_data.asteroid_body_rows));
	CHECK(_load_check_data.native_cargo_reference_rows.size() ==
		_load_check_data.native_station_cargo_refs.size() + _load_check_data.native_ordinary_vehicle_cargo_refs.size());
	// Compare copied CAPA evidence to these same original loaded packets only
	// after native preview has independently admitted the candidate. This tests
	// decoding; the preview validator itself cannot borrow these runtime owners.
	for (const auto &row : _load_check_data.native_cargo_packet_rows) {
		const auto *packet = CargoPacket::GetIfValid(row.packet); REQUIRE(packet != nullptr);
		CHECK(row.quantity == packet->Count()); CHECK(row.source == packet->GetSource());
		CHECK(row.source_tile == packet->GetSourceTile()); CHECK(row.first_station == packet->GetFirstStation());
		CHECK(row.feeder_share == packet->GetFeederShare());
	}
	CHECK(GetAsteroidField() == field);
	CHECK(ComputeSpaceVehicleDigest() == fleet);
	CHECK(ComputeSpaceStructureDigest() == structures);
	CHECK(ComputeUtilityPowerDigest() == power);
	CHECK(ComputeSpaceManifestDigest() == manifest);
	CHECK(TimerGameTick::counter == clock);
	CHECK(std::array{_random.state[0], _random.state[1], _interactive_random.state[0], _interactive_random.state[1]} == random);
	fmt::print("ASTEROID_PREVIEW live_rows={} issued={} original_world_unchanged=true\n", _load_check_data.asteroid_body_rows.size(), _load_check_data.asteroid_next_body_id);
}

/** Load the immutable demo through the actual application and observe GUI/queue behavior. */
TEST_CASE("Retained Space demos expose native window input and construction state", "[.space-fixes-native]")
{
	const char *source = std::getenv("OTTD_SPACE_FIXES_SOURCE");
	const char *config = std::getenv("OTTD_SPACE_FIXES_CONFIG");
	const char *output = std::getenv("OTTD_SPACE_FIXES_OUTPUT");
	REQUIRE(source != nullptr);
	REQUIRE(config != nullptr);
	REQUIRE(output != nullptr);
	bool observed = false;
	std::exception_ptr failure;
	/** Observe loaded owners; any exception is rethrown after normal native shutdown. */
	std::function<void()> observe = [&] {
		try {
			AutoRestoreBackup actor(_current_company, _local_company);
			REQUIRE(ValidateSpaceVehicles());
			// A genuine legacy active field must reach observation through one-time
			// renewal seeding; Classic's absent field retains its canonical empty state.
			if (GetAsteroidField().generation_version != 0) CHECK(GetAsteroidField().renewal.initialized);
			if (std::getenv("OTTD_SPACE_FIXES_ASTEROID_PREVIEW") != nullptr) VerifyAsteroidIdentityPreview(source);
			if (std::getenv("OTTD_SPACE_FIXES_PREVIEW_CANDIDATE") != nullptr) {
				VerifySpaceTransportIsolation(source); observed = true; _exit_game = true; return;
			}
			if (const char *scenario = std::getenv("OTTD_SPACE_FIXES_MINING_CONTROLS")) {
				if (std::string_view{scenario} == "receiver-inventory") ObserveMiningReceiverPreconditions();
				else if (std::string_view{scenario} == "receiver-prepare") PrepareMiningReceiverRefusal(output);
				else if (std::string_view{scenario} == "receiver-fallback") VerifyMiningReceiverFallback(output);
				else if (std::string_view{scenario} == "receiver-remove") VerifyMiningReceiverRemoval(output);
				else if (std::string_view{scenario} == "mineral-wake") VerifyMiningMineralWake(output, SPACE_HEAVY_MINER_MODEL);
				else if (std::string_view{scenario} == "mineral-wake-light") VerifyMiningMineralWake(output, SPACE_PROBE_MODEL);
				else if (std::string_view{scenario} == "resume-service") VerifyMiningServiceResume(output, SPACE_HEAVY_MINER_MODEL);
				else if (std::string_view{scenario} == "resume-service-light") VerifyMiningServiceResume(output, SPACE_PROBE_MODEL);
				else if (std::string_view{scenario} == "stop-recall") VerifyMiningStopRecall(output, SPACE_HEAVY_MINER_MODEL);
				else if (std::string_view{scenario} == "stop-recall-light") VerifyMiningStopRecall(output, SPACE_PROBE_MODEL);
				else if (std::string_view{scenario} == "lifecycle") VerifySpaceLifecyclePhases(output, false);
				else if (std::string_view{scenario} == "lifecycle-hidden") VerifySpaceLifecyclePhases(output, true);
				else if (std::string_view{scenario} == "lifecycle-speed") VerifySpaceLifecycleScheduling(output);
				else if (std::string_view{scenario} == "prepare-high-id") PrepareNativeHighIDPeerSource(output);
				else if (std::string_view{scenario} == "work-retry") VerifyNativeMiningRetryWork(output);
				else if (std::string_view{scenario}.starts_with("service-")) VerifyMiningServiceWait(scenario, output);
				else if (std::string_view{scenario} == "continue") VerifySpaceMiningPolicyContinuation(output);
				else if (std::string_view{scenario} == "continue-light") VerifySpaceMiningPolicyContinuation(output, SPACE_PROBE_MODEL);
				else if (std::string_view{scenario} == "full") VerifySpaceMiningPolicyContinuation(output, SPACE_HEAVY_MINER_MODEL, true);
				else if (std::string_view{scenario} == "full-light") VerifySpaceMiningPolicyContinuation(output, SPACE_PROBE_MODEL, true);
				else if (std::string_view{scenario} == "surface") VerifySpaceMiningSurfaceReturn(output);
				else if (std::string_view{scenario} == "no-mineral") VerifySpaceMiningNoMineral(output, SPACE_HEAVY_MINER_MODEL);
				else if (std::string_view{scenario} == "no-mineral-light") VerifySpaceMiningNoMineral(output, SPACE_PROBE_MODEL);
				else if (std::string_view{scenario} == "replay") VerifySpaceMiningPolicyReplay(source, output);
				else if (std::string_view{scenario} == "replay-light") VerifySpaceMiningPolicyReplay(source, output, SPACE_PROBE_MODEL);
				else VerifySpaceMiningPolicyControls(scenario, output);
				observed = true; _exit_game = true; return;
			}
			if (std::getenv("OTTD_SPACE_FIXES_ASTEROID_LIFECYCLE") != nullptr) {
				VerifyAsteroidLifecycleNative(output); observed = true; _exit_game = true; return;
			}
			if (std::getenv("OTTD_SPACE_FIXES_CARGO_ORIGIN") != nullptr) {
				VerifyAsteroidCargoNative(source, output); observed = true; _exit_game = true; return;
			}
			if (const char *scenario = std::getenv("OTTD_SPACE_FIXES_CONSTRUCTION")) {
				RunSpaceConstructionTrace(scenario);
				observed = true;
				_exit_game = true;
				return;
			}
			if (std::getenv("OTTD_SPACE_FIXES_SHORTCUTS") != nullptr) VerifySpaceFixesShortcuts();
			if (std::getenv("OTTD_SPACE_FIXES_SHORTCUTS_ONLY") != nullptr) {
				observed = true;
				_exit_game = true;
				return;
			}
			fmt::print("CLIENT width={} height={} scale={} normal_font={} paused={}\n", _screen.width, _screen.height, _gui_scale, GetCharacterHeight(FontSize::Normal), !_pause_mode.None());
			SetViewportLayer(GetMainWindow(), MapLayer::Surface);
			HandleKeypress(WKC_ALT | '4', 0);
			fmt::print("INPUT alt4_space={}\n", GetMainWindow()->viewport->layer == MapLayer::Space);
			fmt::print("INPUT global_alt1_match={} global_alt2_match={}\n", GetMainWindow()->window_desc.hotkeys->CheckMatch(WKC_ALT | '1'), GetMainWindow()->window_desc.hotkeys->CheckMatch(WKC_ALT | '2'));
			HandleKeypress(WKC_F2, 0);
			fmt::print("INPUT f2_first_open={}\n", FindWindowById(WindowClass::GameOptions, GameOptionsWindowNumber::GameOptions) != nullptr);
			HandleKeypress(WKC_F2, 0);
			fmt::print("INPUT f2_second_open={}\n", FindWindowById(WindowClass::GameOptions, GameOptionsWindowNumber::GameOptions) != nullptr);
			CloseWindowByClass(WindowClass::GameOptions);
			Window::DeleteClosedWindows();
			REQUIRE(BlitterFactory::SelectBlitter("8bpp-simple") != nullptr);
			GfxClearSpriteCache();
			FontCache::ClearFontCaches(FONTSIZES_REQUIRED);
			ShowSpaceConstructionWindow();
			auto *window = FindWindowById(WindowClass::SpaceConstruction, 0);
			REQUIRE(window != nullptr);
			fmt::print("WINDOW construction left={} top={} width={} height={} min_width={} min_height={}\n", window->left, window->top, window->width, window->height, window->nested_root->smallest_x, window->nested_root->smallest_y);
			// Characterize every original action by its stable widget identity; later
			// layout work keeps these command mappings while changing presentation.
			for (int id = 1; id <= 38; ++id) {
				const auto *widget = window->GetWidget<NWidgetBase>(id);
				fmt::print("WIDGET id={} left={} top={} width={} height={} outside_client={}\n", id, widget->pos_x, widget->pos_y, widget->current_x, widget->current_y, int(window->top + widget->pos_y + widget->current_y) > _screen.height);
			}
			CaptureSpaceFixesWindow(*window, std::filesystem::path(output) / "construction.ppm");
			if (std::getenv("OTTD_SPACE_FIXES_LAYOUT") != nullptr) VerifySpaceFixesLayout(*window, output);
			const auto inspector_field = ComputeAsteroidFieldDigest(GetAsteroidField()), inspector_fleet = ComputeSpaceVehicleDigest(), inspector_orders = ComputeSpaceOrdersDigest();
			const auto inspector_tick = TimerGameTick::counter;
			ShowSpaceSurveyWindow();
			auto *inspector = FindWindowById(WindowClass::SpaceSurvey, 0);
			REQUIRE(inspector != nullptr);
			fmt::print("WINDOW inspector left={} top={} width={} height={}\n", inspector->left, inspector->top, inspector->width, inspector->height);
			CHECK(inspector->left >= 0); CHECK(inspector->top >= GetMainViewTop());
			CHECK(inspector->left + inspector->width <= _screen.width); CHECK(inspector->top + inspector->height <= GetMainViewBottom());
			// The new policy row shares the original bounded client, even when
			// unsupported roles disable editing. Geometry cannot hide its controls.
			for (const WidgetID id : {WID_SS_POLICY_LEGACY, WID_SS_POLICY_RETURN, WID_SS_POLICY_FULL}) {
				const Rect rect = inspector->GetWidget<NWidgetBase>(id)->GetCurrentRect();
				CHECK(GetWidgetFromPos(inspector, (rect.left + rect.right) / 2, (rect.top + rect.bottom) / 2) == id);
				CHECK(inspector->top + rect.bottom < GetMainViewBottom());
			}
			CaptureSpaceFixesWindow(*inspector, std::filesystem::path(output) / "inspector.ppm");
			CHECK(ComputeAsteroidFieldDigest(GetAsteroidField()) == inspector_field); CHECK(ComputeSpaceVehicleDigest() == inspector_fleet);
			CHECK(ComputeSpaceOrdersDigest() == inspector_orders); CHECK(TimerGameTick::counter == inspector_tick);
			ReportSpaceFixesState("before");
			const uint64_t target = GetAsteroidField().elapsed_ticks + 74;
			const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(60);
			{
				AutoRestoreBackup pause(_pause_mode, PauseModes{});
				// Count simulation opportunities, not rendering calls; this reproduces
				// normal unpaused construction without introducing another work clock.
				while (GetAsteroidField().elapsed_ticks < target) {
					REQUIRE(std::chrono::steady_clock::now() < deadline);
					StateGameLoop();
					std::this_thread::sleep_for(std::chrono::milliseconds(1));
				}
				ReportSpaceFixesState("after-74-unpaused");
			}
			observed = true;
		} catch (...) { failure = std::current_exception(); }
		_exit_game = true;
	};
	AutoRestoreBackup hook(space_fixes_observer, &observe);
	IConsole::CmdRegister("space_fixes_observe", ObserveSpaceFixes);
	const char *resolution = std::getenv("OTTD_SPACE_FIXES_RESOLUTION");
	std::array<std::string_view, 19> args{"openttd_space_fixes_test.exe", "-v", "null:ticks=2", "-b", "null", "-s", "null", "-m", "null", "-x", "-X", "-I", "original_dos", "-r", resolution == nullptr ? "1920x1027" : resolution, "-c", config, "-g", source};
	CHECK(openttd_main(args) == 0);
	if (failure) std::rethrow_exception(failure);
	CHECK(observed);
}
