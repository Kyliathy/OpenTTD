/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file geothermal_calendar.cpp Actual geothermal command, construction, operation and live-control calendar boundaries.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs18-geothermal-energy-and-industry.md Group15D.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md and architecture/gui/archi-gui.md.
 * These isolated native boundaries disclose prepared floor and receiver stock; the separate paid-world receipts remain authoritative for bootstrap.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "mock_environment.h"
#include "underground_view_fixture.h"
#include "geothermal_generator_operating_fixture.h"
#include "geothermal_collector_fixture.h"
#include "geothermal_riser_fixture.h"
#include "../geothermal_controls.h"
#include "../geothermal_site_cmd.h"
#include "../underground_gui.h"
#include "../gfx_func.h"
#include "../widgets/underground_widget.h"
#include "../timer/timer_game_economy.h"
#include "../safeguards.h"

/** Explicit accepted world policies; editor authoring permission remains separate from actually running the game. */
enum class GeothermalCalendarPolicy : uint8_t { Native, OverrideNormal, ClassicLegacy, Editor };

/** Restore the actual clocks, speed and accepted world policy before native fixture teardown. */
class GeothermalCalendarScope {
	const ExcavationFixture &rig; ///< Original content identity used by the public restore boundary.
	MultiLayerProfileSelection original = GetCurrentMultiLayerProfileSelection(); ///< Accepted policy to restore, without copying an industry or utility registry.
	AutoRestoreBackup<TimerGameCalendar::Date> calendar{TimerGameCalendar::date}; ///< Synchronized technology clock.
	AutoRestoreBackup<TimerGameEconomy::Date> economy{TimerGameEconomy::date}; ///< Independent economic clock.
	AutoRestoreBackup<TimekeepingUnits> units{_settings_game.economy.timekeeping_units}; ///< Native calendar/wallclock economy mode.
	AutoRestoreBackup<uint16_t> calendar_speed{_settings_game.economy.minutes_per_calendar_year}; ///< Includes an explicitly frozen calendar.
	AutoRestoreBackup<uint16_t> simulation_speed{_game_speed}; ///< Real presentation/simulation pacing setting.
	AutoRestoreBackup<GameMode> mode{_game_mode}; ///< Editor mode cannot execute normal paid construction or production.
public:
	/**
	 * Apply a real dated world policy without changing native machinery, stock or energy.
	 * @param rig Actual isolated machinery/content fixture.
	 * @param model Native model whose own introduction boundary is exercised.
	 * @param before Select the last day before introduction or its first available day.
	 * @param clock One of three independent economy/calendar/pacing combinations.
	 * @param policy Explicit native, author-override, preserved Classic or editor context.
	 */
	GeothermalCalendarScope(const ExcavationFixture &rig, GeothermalGeneratorModel model, bool before, uint clock, GeothermalCalendarPolicy policy) : rig(rig)
	{
		const auto year = GetMultiLayerGeothermalConstruction(GetMultiLayerGeothermalGeneratorTerms(model)->construction)->introduction_year;
		TimerGameCalendar::date = before ? TimerGameCalendar::ConvertYMDToDate(TimerGameCalendar::Year{year.base() - 1}, 11, 31) :
			TimerGameCalendar::ConvertYMDToDate(year, 0, 1);
		// An old economic date with a frozen calendar and a future economic
		// date at slow/fast pacing must never stand in for the technology clock.
		_settings_game.economy.timekeeping_units = clock == 0 ? TimekeepingUnits::Calendar : TimekeepingUnits::Wallclock;
		_settings_game.economy.minutes_per_calendar_year = clock == 0 ? 12 : clock == 1 ? 0 : 120;
		_game_speed = clock == 0 ? 100 : clock == 1 ? 400 : 2000;
		TimerGameEconomy::date = TimerGameEconomy::ConvertYMDToDate(TimerGameEconomy::Year{clock == 1 ? 1900 : 2090}, 0, 1);
		auto selection = this->original;
		if (policy == GeothermalCalendarPolicy::ClassicLegacy) selection = {};
		selection.editor_override = policy == GeothermalCalendarPolicy::OverrideNormal || policy == GeothermalCalendarPolicy::Editor; // Saved author access permits inspecting Deep; it does not advance either generator's introduction date or enable editor production.
		REQUIRE(RestoreMultiLayerProfileSelection(selection, LandscapeType::Temperate, rig.ActivatedContent()) == MultiLayerProfileError::None);
		_game_mode = policy == GeothermalCalendarPolicy::Editor ? GameMode::Editor : GameMode::Normal;
	}
	/** Restore accepted policy while the original native content and map still exist. */
	~GeothermalCalendarScope()
	{
		CHECK(RestoreMultiLayerProfileSelection(this->original, LandscapeType::Temperate, this->rig.ActivatedContent()) == MultiLayerProfileError::None);
	}
};

/** @param before Whether the actual model date is still locked. @param policy Explicit world/editor policy. @return Expected native paid facility admission, independent of all economy and pacing choices. */
static bool IsCalendarModelAdmitted(bool before, GeothermalCalendarPolicy policy)
{
	// Both dated machinery and a real running native world are required.
	// An author-access flag alone never creates an early plant or legacy economy.
	return !before && (policy == GeothermalCalendarPolicy::Native || policy == GeothermalCalendarPolicy::OverrideNormal);
}

/** Synchronized cold planning revalidates the actual calendar/profile in both phases, without spending stock, cash or RNG on refusal. */
TEST_CASE("Geothermal planning follows calendar under independent economy and saved policies", "[geothermal-calendar-planning]")
{
	const auto model = GENERATE(GeothermalGeneratorModel::VentTap, GeothermalGeneratorModel::MagmaLoop);
	const auto clock = GENERATE(0U, 1U, 2U);
	const auto policy = GENERATE(GeothermalCalendarPolicy::Native, GeothermalCalendarPolicy::OverrideNormal, GeothermalCalendarPolicy::ClassicLegacy, GeothermalCalendarPolicy::Editor);
	const auto before = GENERATE(true, false);
	CAPTURE(model, clock, policy, before);
	GeothermalGeneratorRig rig{false, model}; REQUIRE(CancelGeothermalSite(rig.site) == GeothermalError::None);
	GeothermalCalendarScope context{rig, model, before, clock, policy};
	const bool admitted = IsCalendarModelAdmitted(before, policy);
	const auto sites = ComputeGeothermalSiteDigest(), geology = ComputeGeologyStateDigest(GetGeologyWorldState()), utility = ComputeUtilityPowerDigest();
	const auto cash = Company::Get(_current_company)->money; const auto random = _random;
	// Each phase resolves the real source/floor under the current accepted
	// policy; a successful estimate must not reserve an identity for execution.
	for (const auto flags : {DoCommandFlags{}, DoCommandFlags{DoCommandFlag::Execute}}) {
		const auto result = Command<Commands::PlanGeothermalSite>::Do(flags, rig.request.tile, rig.request.vent, rig.request.kind, rig.request.endpoint, rig.request.access);
		CHECK(result.Succeeded() == admitted);
		if (!admitted || !flags.Test(DoCommandFlag::Execute)) CHECK(ComputeGeothermalSiteDigest() == sites); // Refusal and estimates retain every original reservation; only accepted execution may publish a new plan.
	}
	CHECK(Company::Get(_current_company)->money == cash); CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == geology);
	CHECK(ComputeUtilityPowerDigest() == utility); CHECK(std::ranges::equal(random.state, _random.state)); CHECK(Industry::GetNumItems() == 0);
}

/** A retained original site cannot bypass a locked model date, preserved legacy policy or editor mode at actual capital settlement. */
TEST_CASE("Geothermal funding revalidates dated original sites under independent clocks", "[geothermal-calendar-funding]")
{
	const auto model = GENERATE(GeothermalGeneratorModel::VentTap, GeothermalGeneratorModel::MagmaLoop);
	const auto clock = GENERATE(0U, 1U, 2U);
	const auto policy = GENERATE(GeothermalCalendarPolicy::Native, GeothermalCalendarPolicy::OverrideNormal, GeothermalCalendarPolicy::ClassicLegacy, GeothermalCalendarPolicy::Editor);
	const auto before = GENERATE(true, false);
	CAPTURE(model, clock, policy, before);
	GeothermalGeneratorRig rig{false, model}; GeothermalCalendarScope context{rig, model, before, clock, policy};
	const bool admitted = IsCalendarModelAdmitted(before, policy);
	const auto sites = ComputeGeothermalSiteDigest(), geology = ComputeGeologyStateDigest(GetGeologyWorldState()), utility = ComputeUtilityPowerDigest();
	const auto cash = Company::Get(_current_company)->money; const auto random = _random;
	CHECK(rig.Fund(true).Succeeded() == admitted); CHECK(ComputeGeothermalSiteDigest() == sites); CHECK(Company::Get(_current_company)->money == cash);
	CHECK(std::ranges::equal(random.state, _random.state));
	CHECK(rig.Fund().Succeeded() == admitted);
	if (admitted) {
		const auto *terms = GetMultiLayerGeothermalConstruction(rig.request.kind);
		CHECK(Company::Get(_current_company)->money == cash - terms->capital_cost);
		CHECK(GetGeothermalGeneratorState(rig.Generator().index)->work_ticks == 0);
	} else {
		CHECK(Industry::GetNumItems() == 0); CHECK(Company::Get(_current_company)->money == cash); CHECK(ComputeGeothermalSiteDigest() == sites);
	}
	CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == geology); CHECK(ComputeUtilityPowerDigest() == utility);
	if (!admitted) CHECK(std::ranges::equal(random.state, _random.state)); // Successful native industry creation owns its deterministic execution randomness; refused funding must leave it untouched.
}

/** Native construction retains the actual arrived whole bill during technology/policy locks and resumes the same prepaid machine afterward. */
TEST_CASE("Geothermal actual construction obeys calendar without losing paid material", "[geothermal-calendar-construction]")
{
	const auto model = GENERATE(GeothermalGeneratorModel::VentTap, GeothermalGeneratorModel::MagmaLoop);
	const auto clock = GENERATE(0U, 1U, 2U);
	const auto policy = GENERATE(GeothermalCalendarPolicy::Native, GeothermalCalendarPolicy::OverrideNormal, GeothermalCalendarPolicy::ClassicLegacy, GeothermalCalendarPolicy::Editor);
	const auto before = GENERATE(true, false);
	CAPTURE(model, clock, policy, before);
	GeothermalGeneratorRig rig{false, model}; REQUIRE(rig.Fund().Succeeded()); rig.DeliverBill();
	const auto id = rig.Generator().index; const auto initial = ComputeGeothermalGeneratorDigest(); const auto cash = Company::Get(_current_company)->money;
	const bool admitted = IsCalendarModelAdmitted(before, policy);
	{
		GeothermalCalendarScope context{rig, model, before, clock, policy}; rig.Tick(1);
		const auto *state = GetGeothermalGeneratorState(id); REQUIRE(state != nullptr);
		CHECK(state->work_ticks == (admitted ? 1 : 0)); CHECK(state->materials_committed == admitted);
		if (!admitted) CHECK(ComputeGeothermalGeneratorDigest() == initial);
		// Every original accepted slot retains its delivered amount until the
		// single whole-bill transaction commits; another clock earns no labour.
		for (const auto &input : GetMultiLayerGeothermalConstruction(rig.request.kind)->inputs) {
			CHECK(rig.Generator().GetCargoAccepted(GetCargoTypeByLabel(input.label))->waiting == (admitted ? 0 : input.quantity));
		}
		CHECK(Company::Get(_current_company)->money == cash); CHECK(CaptureUtilityPowerState().producers.empty());
	}
	rig.Tick(1); CHECK(rig.Generator().index == id); CHECK(GetGeothermalGeneratorState(id)->work_ticks == (admitted ? 2 : 1));
	CHECK(Company::Get(_current_company)->money == cash);
}

/** The actual common driver grants exactly one original Heat/conversion/service opportunity at unlock; blocked time is never back-billed. */
TEST_CASE("Geothermal actual operation obeys calendar independently of economy and pacing", "[geothermal-calendar-operation]")
{
	const auto model = GENERATE(GeothermalGeneratorModel::VentTap, GeothermalGeneratorModel::MagmaLoop);
	const auto clock = GENERATE(0U, 1U, 2U);
	const auto policy = GENERATE(GeothermalCalendarPolicy::Native, GeothermalCalendarPolicy::OverrideNormal, GeothermalCalendarPolicy::ClassicLegacy, GeothermalCalendarPolicy::Editor);
	const auto before = GENERATE(true, false);
	CAPTURE(model, clock, policy, before);
	GeothermalGeneratorOperatingRig rig{false, true, 1000000, model}; rig.Operate();
	const auto original = *GetGeothermalGeneratorState(rig.generator); const auto cash = Company::Get(_current_company)->money;
	const auto generated = GetUtilityPowerProducer(original.electrical_producer)->generated_lifetime_j;
	const auto source = GetGeothermalCollectorState(rig.collector)->heat_producer; const auto used = GetUtilityPowerProducer(source)->consumed_lifetime_j;
	const auto *terms = GetMultiLayerGeothermalGeneratorTerms(model); const bool admitted = IsCalendarModelAdmitted(before, policy);
	{
		GeothermalCalendarScope context{rig, model, before, clock, policy}; rig.Operate();
		const auto *state = GetGeothermalGeneratorState(rig.generator); REQUIRE(state != nullptr);
		CHECK(state->active_service_ticks == original.active_service_ticks + (admitted ? 1 : 0));
		CHECK(GetUtilityPowerProducer(original.electrical_producer)->generated_lifetime_j == generated + (admitted ? uint64_t{27000} * terms->efficiency_percent / 100 : 0));
		CHECK(GetUtilityPowerProducer(source)->consumed_lifetime_j == used + (admitted ? 27000 : 0));
		if (!admitted) { CHECK(*state == original); CHECK(Company::Get(_current_company)->money == cash); }
	}
	const auto active = GetGeothermalGeneratorState(rig.generator)->active_service_ticks; rig.Operate();
	CHECK(GetGeothermalGeneratorState(rig.generator)->active_service_ticks == active + 1);
	CHECK(GetGeothermalGeneratorState(rig.generator)->electrical_producer == original.electrical_producer);
	CHECK(GetGeothermalGeneratorState(rig.generator)->heat_consumer == original.heat_consumer);
}

/** An already-open real window re-queries current model and world policy; stale selection cannot fund a locked machine or consume RNG. */
TEST_CASE("Geothermal live controls follow current calendar and explicit policies", "[geothermal-calendar-window]")
{
	MockEnvironment::Instance(); LoadGeneratedWindowTestLanguage();
	const auto model = GENERATE(GeothermalGeneratorModel::VentTap, GeothermalGeneratorModel::MagmaLoop);
	const auto clock = GENERATE(0U, 1U, 2U);
	const auto policy = GENERATE(GeothermalCalendarPolicy::Native, GeothermalCalendarPolicy::OverrideNormal, GeothermalCalendarPolicy::ClassicLegacy, GeothermalCalendarPolicy::Editor);
	const auto before = GENERATE(true, false);
	CAPTURE(model, clock, policy, before);
	GeothermalGeneratorRig rig{false, model}; UndergroundToolbarTestUI ui{_current_company};
	Window *window = ShowUndergroundConstructionToolbar(MapLayer::DeepUnderground, GetCurrentMultiLayerProfileSelection()); REQUIRE(window != nullptr);
	window->OnPaint(); window->OnClick({}, WID_UC_GEOTHERMAL, 1);
	window->OnDropdownSelect(WID_UC_GEO_FAMILY, to_underlying(rig.request.kind), 0); window->OnDropdownSelect(WID_UC_GEO_SITE, rig.site.base(), 0);
	GeothermalCalendarScope context{rig, model, before, clock, policy}; const bool admitted = IsCalendarModelAdmitted(before, policy);
	const auto sites = ComputeGeothermalSiteDigest(), geology = ComputeGeologyStateDigest(GetGeologyWorldState()), utility = ComputeUtilityPowerDigest();
	const auto cash = Company::Get(_current_company)->money; SavedRandomSeeds seeds; SaveRandomSeeds(&seeds);
	window->OnPaint(); window->OnPaint();
	CHECK(IsGeothermalFacilityToolAvailable(rig.request.kind, MapLayer::DeepUnderground, _local_company) == admitted);
	CHECK(window->IsWidgetDisabled(WID_UC_GEO_PLAN) == !admitted); CHECK(window->IsWidgetDisabled(WID_UC_GEO_FUND) == !admitted);
	if (!admitted) window->OnClick({}, WID_UC_GEO_FUND, 1);
	SavedRandomSeeds after; SaveRandomSeeds(&after);
	CHECK(ComputeGeothermalSiteDigest() == sites); CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == geology); CHECK(ComputeUtilityPowerDigest() == utility);
	CHECK(Company::Get(_current_company)->money == cash); CHECK(Industry::GetNumItems() == 0);
	CHECK(std::ranges::equal(seeds.random.state, after.random.state)); CHECK(std::ranges::equal(seeds.interactive_random.state, after.interactive_random.state));
}

/**
 * Refuse an otherwise valid paid collector/riser command in explicit editor mode before any original participant changes.
 * @tparam Rig Actual collector or original-portal riser fixture, using its real command kernel.
 * @param rig Fully admissible native company/floor/source or connected-portal input.
 * @param clock Independent economic and pacing setting; technology is already unlocked.
 */
template <typename Rig> static void CheckEditorGeothermalFunding(Rig &rig, uint clock)
{
	REQUIRE(rig.Fund(true).Succeeded());
	GeothermalCalendarScope context{rig, GeothermalGeneratorModel::VentTap, false, clock, GeothermalCalendarPolicy::Editor};
	const auto sites = ComputeGeothermalSiteDigest(), geology = ComputeGeologyStateDigest(GetGeologyWorldState()), utility = ComputeUtilityPowerDigest();
	const auto cash = Company::Get(_current_company)->money; const auto count = Industry::GetNumItems(); SavedRandomSeeds seeds; SaveRandomSeeds(&seeds);
	CHECK(rig.Fund(true).Failed()); CHECK(rig.Fund().Failed());
	SavedRandomSeeds after; SaveRandomSeeds(&after);
	CHECK(Industry::GetNumItems() == count); CHECK(Company::Get(_current_company)->money == cash);
	CHECK(ComputeGeothermalSiteDigest() == sites); CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == geology); CHECK(ComputeUtilityPowerDigest() == utility);
	CHECK(std::ranges::equal(seeds.random.state, after.random.state)); CHECK(std::ranges::equal(seeds.interactive_random.state, after.interactive_random.state));
}

/** All three paid geothermal funding commands share the normal-game rule; an explicit author-access flag creates no editor invoice or machinery. */
TEST_CASE("Geothermal collector and original riser refuse actual editor funding unchanged", "[geothermal-calendar-editor-funding]")
{
	const auto clock = GENERATE(0U, 1U, 2U); CAPTURE(clock);
	SECTION("collector") { GeothermalCollectorRig rig; CheckEditorGeothermalFunding(rig, clock); }
	SECTION("original portal riser") { NativeRiserRig rig; CheckEditorGeothermalFunding(rig, clock); }
}
