/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file geothermal_controls.cpp Synchronized cold-site commands and pure native-backed geothermal inspection.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs18-geothermal-energy-and-industry.md Group15B.
 * @see docs/zz-reach2/architecture/gui/archi-gui.md.
 * Prepared native boundary floors/materials do not replace the separate complete paid first-plant receipt.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "geothermal_generator_operating_fixture.h"
#include "geothermal_collector_fixture.h"
#include "../geothermal_site_cmd.h"
#include "../geothermal_controls.h"
#include "../safeguards.h"

/** @param request Original actual geometry and source. @param flags Pure quote or real command execution. @return Native command result using the synchronized current company. */
static CommandCost PlanNativeGeothermalSite(const GeothermalSiteRequest &request, DoCommandFlags flags = {})
{
	return Command<Commands::PlanGeothermalSite>::Do(flags, request.tile, request.vent, request.kind, request.endpoint, request.access);
}

/** Actual network command buffers retain explicit world geometry, immutable family and original typed site identity. */
TEST_CASE("Geothermal site command arguments round trip through native buffers", "[geothermal-site-command-serialization]")
{
	using Plan = CommandTraits<Commands::PlanGeothermalSite>::Args;
	const Plan plan{TileIndex{123}, GeothermalVentID{4}, MultiLayerGeothermalConstructionKind::MagmaPowerhouse, TileIndex{124}, DiagDirection::SE};
	CHECK(EndianBufferReader::ToValue<Plan>(EndianBufferWriter<CommandDataBuffer>::FromValue(plan)) == plan);
	using Cancel = CommandTraits<Commands::CancelGeothermalSite>::Args;
	const Cancel cancel{GeothermalSiteID{9}};
	CHECK(EndianBufferReader::ToValue<Cancel>(EndianBufferWriter<CommandDataBuffer>::FromValue(cancel)) == cancel);
}

/** A different live company cannot cancel an original site or seize its protected floor through either command phase. */
TEST_CASE("Geothermal site commands preserve foreign planning ownership", "[geothermal-site-command-ownership]")
{
	GeothermalGeneratorRig rig;
	REQUIRE(Company::CanAllocateItem()); std::unique_ptr<Company> foreign{Company::Create()}; foreign->money = 1000000;
	AutoRestoreBackup actor{_current_company, foreign->index};
	const auto sites = ComputeGeothermalSiteDigest(), geology = ComputeGeologyStateDigest(GetGeologyWorldState());
	// Both phases retain the existing owner's exact floor reservation. A new
	// valid company still cannot release or replace another company's blueprint.
	for (const auto flags : {DoCommandFlags{}, DoCommandFlags{DoCommandFlag::Execute}}) {
		CHECK(Command<Commands::CancelGeothermalSite>::Do(flags, rig.site).Failed());
		CHECK(PlanNativeGeothermalSite(rig.request, flags).Failed());
		CHECK(ComputeGeothermalSiteDigest() == sites); CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == geology);
	}
}

/** Synchronized planning/cancellation retains original immutable model/site history; estimates reserve no floor or capacity. */
TEST_CASE("Native geothermal planning commands preserve quote and cancellation custody", "[geothermal-site-commands]")
{
	const auto model = GENERATE(GeothermalGeneratorModel::VentTap, GeothermalGeneratorModel::MagmaLoop);
	GeothermalGeneratorRig rig{false, model};
	REQUIRE(CancelGeothermalSite(rig.site) == GeothermalError::None);
	const auto geology = ComputeGeologyStateDigest(GetGeologyWorldState()), utility = ComputeUtilityPowerDigest();
	const auto sites = ComputeGeothermalSiteDigest(); const auto random = _random;
	const auto cash = Company::Get(_current_company)->money;
	REQUIRE(PlanNativeGeothermalSite(rig.request).Succeeded()); CHECK(ComputeGeothermalSiteDigest() == sites);
	REQUIRE(PlanNativeGeothermalSite(rig.request, DoCommandFlag::Execute).Succeeded());
	const auto *claim = GetGeothermalSiteClaimForTile(rig.request.tile); REQUIRE(claim != nullptr);
	const auto id = claim->id; CHECK(id > rig.site); CHECK(claim->site.kind == rig.request.kind); CHECK(claim->site.owner == _current_company);
	CHECK(PlanNativeGeothermalSite(rig.request, DoCommandFlag::Execute).Failed());
	const auto active = ComputeGeothermalSiteDigest();
	REQUIRE(Command<Commands::CancelGeothermalSite>::Do({}, id).Succeeded()); CHECK(ComputeGeothermalSiteDigest() == active);
	REQUIRE(Command<Commands::CancelGeothermalSite>::Do(DoCommandFlag::Execute, id).Succeeded());
	CHECK_FALSE(GetGeothermalSiteClaim(id)->active); CHECK(GetGeothermalSiteClaimForTile(rig.request.tile) == nullptr);
	CHECK(Command<Commands::CancelGeothermalSite>::Do(DoCommandFlag::Execute, id).Failed());
	CHECK(Company::Get(_current_company)->money == cash); CHECK(Industry::GetNumItems() == 0);
	CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == geology); CHECK(ComputeUtilityPowerDigest() == utility);
	CHECK(std::ranges::equal(random.state, _random.state));
}

/** The collector uses the same synchronized planner, then the existing once-only capital/material/work owner. */
TEST_CASE("Native collector funding consumes a synchronized original plan", "[geothermal-site-command-funding]")
{
	GeothermalCollectorRig rig; REQUIRE(CancelGeothermalSite(rig.site) == GeothermalError::None);
	REQUIRE(PlanNativeGeothermalSite(rig.request, DoCommandFlag::Execute).Succeeded());
	const auto *site = GetGeothermalSiteClaimForTile(rig.request.tile); REQUIRE(site != nullptr); rig.site = site->id;
	const auto before = GetGeothermalControlInspection(rig.request.tile); CHECK(before.site == site); CHECK(before.industry == nullptr);
	const auto cash = Company::Get(_current_company)->money; REQUIRE(rig.Fund().Succeeded());
	const auto after = GetGeothermalControlInspection(rig.request.tile);
	REQUIRE(after.collector != nullptr); REQUIRE(after.industry != nullptr); REQUIRE(after.site != nullptr);
	CHECK_FALSE(after.site->active); CHECK(after.collector->work_ticks == 0); CHECK(after.heat == nullptr);
	CHECK(Company::Get(_current_company)->money == cash - Money{180000});
	CHECK(Command<Commands::CancelGeothermalSite>::Do(DoCommandFlag::Execute, rig.site).Failed());
}

/** Invalid actor/mode/date/geometry is rejected by the actual command before any reservation, stock, money or geology changes. */
TEST_CASE("Native geothermal site commands reject invalid participants unchanged", "[geothermal-site-command-refusal]")
{
	GeothermalGeneratorRig rig; REQUIRE(CancelGeothermalSite(rig.site) == GeothermalError::None);
	AutoRestoreBackup actor{_current_company}; AutoRestoreBackup mode{_game_mode};
	SECTION("locked calendar") { TimerGameCalendar::date = TimerGameCalendar::ConvertYMDToDate(TimerGameCalendar::Year{1979}, 11, 31); }
	SECTION("wrong layer") { rig.request.tile = MultiLayerMap::ProjectToSurface(rig.request.tile); }
	SECTION("missing original source") { rig.request.vent = GeothermalVentID{UINT32_MAX}; }
	SECTION("invalid endpoint") { rig.request.endpoint = INVALID_TILE; }
	SECTION("unsupported kind") { rig.request.kind = static_cast<MultiLayerGeothermalConstructionKind>(255); }
	SECTION("deity has no company") { _current_company = OWNER_DEITY; }
	SECTION("explicit editor refusal") { _game_mode = GameMode::Editor; }
	SECTION("profile deactivated") { ResetMultiLayerProfileSelection(); }
	const auto sites = ComputeGeothermalSiteDigest(), geology = ComputeGeologyStateDigest(GetGeologyWorldState());
	const auto random = _random;
	// Both command phases must preserve every owner for the same invalid request;
	// testing execution prevents a quote-only guard from hiding partial mutation.
	for (const auto flags : {DoCommandFlags{}, DoCommandFlags{DoCommandFlag::Execute}}) {
		CHECK(PlanNativeGeothermalSite(rig.request, flags).Failed()); CHECK(ComputeGeothermalSiteDigest() == sites);
		CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == geology); CHECK(Industry::GetNumItems() == 0);
		CHECK(std::ranges::equal(random.state, _random.state));
	}
}

/** Live inspection borrows exact machinery and shared meters; repeated and other-layer views never allocate accounts or advance work. */
TEST_CASE("Geothermal controls inspect actual shared owners without mutation", "[geothermal-control-inspection]")
{
	const auto model = GENERATE(GeothermalGeneratorModel::VentTap, GeothermalGeneratorModel::MagmaLoop);
	GeothermalGeneratorOperatingRig rig{false, true, 1000000, model}; rig.Operate();
	const auto native = ComputeGeothermalGeneratorDigest(), utility = ComputeUtilityPowerDigest(); const auto random = _random;
	const auto view = GetGeothermalControlInspection(rig.plant_port);
	REQUIRE(view.generator != nullptr); REQUIRE(view.site != nullptr); REQUIRE(view.vent != nullptr);
	CHECK(view.generator == GetGeothermalGeneratorState(rig.generator)); CHECK(view.industry == Industry::Get(rig.generator));
	CHECK(view.heat_input == GetUtilityPowerConsumer(view.generator->heat_consumer));
	CHECK(view.electricity == GetUtilityPowerProducer(view.generator->electrical_producer));
	CHECK(view.heat == GetUtilityPowerProducer(GetGeothermalCollectorState(rig.collector)->heat_producer));
	CHECK(view.vent->id == rig.request.vent); CHECK(view.heat->consumed_lifetime_j == 27000);
	CHECK(GetGeothermalControlInspection(MultiLayerMap::ProjectToSurface(rig.plant_port)).generator == nullptr);
	CHECK(GetGeothermalControlInspection(INVALID_TILE).vent == nullptr);
	CHECK(GetGeothermalControlInspection(rig.plant_port).electricity == view.electricity);
	CHECK(ComputeGeothermalGeneratorDigest() == native); CHECK(ComputeUtilityPowerDigest() == utility);
	CHECK(std::ranges::equal(random.state, _random.state));
}

/** Facility tools follow synchronized calendar/company/layer admission; native command revalidation remains mandatory. */
TEST_CASE("Geothermal facility controls expose only the live dated native scope", "[geothermal-control-availability]")
{
	GeothermalGeneratorRig rig;
	CHECK(IsGeothermalFacilityToolAvailable(MultiLayerGeothermalConstructionKind::Powerhouse, MapLayer::DeepUnderground, _current_company));
	CHECK_FALSE(IsGeothermalFacilityToolAvailable(MultiLayerGeothermalConstructionKind::MagmaPowerhouse, MapLayer::DeepUnderground, _current_company));
	CHECK(IsGeothermalFacilityToolAvailable(MultiLayerGeothermalConstructionKind::Riser, MapLayer::Underground, _current_company));
	CHECK_FALSE(IsGeothermalFacilityToolAvailable(MultiLayerGeothermalConstructionKind::Powerhouse, MapLayer::Underground, _current_company));
	CHECK_FALSE(IsGeothermalFacilityToolAvailable(MultiLayerGeothermalConstructionKind::Collector, MapLayer::Surface, _current_company));
	CHECK_FALSE(IsGeothermalFacilityToolAvailable(MultiLayerGeothermalConstructionKind::Collector, MapLayer::DeepUnderground, CompanyID::Invalid()));
	TimerGameCalendar::date = TimerGameCalendar::ConvertYMDToDate(TimerGameCalendar::Year{2000}, 0, 1);
	CHECK(IsGeothermalFacilityToolAvailable(MultiLayerGeothermalConstructionKind::MagmaPowerhouse, MapLayer::DeepUnderground, _current_company));
	AutoRestoreBackup mode{_game_mode, GameMode::Editor};
	CHECK_FALSE(IsGeothermalFacilityToolAvailable(MultiLayerGeothermalConstructionKind::Collector, MapLayer::DeepUnderground, _current_company));
}
