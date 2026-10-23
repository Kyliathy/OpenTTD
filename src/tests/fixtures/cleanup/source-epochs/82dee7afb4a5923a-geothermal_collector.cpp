/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file geothermal_collector.cpp Actual capital/stock/once-per-tick native construction, shortage recovery and source readiness boundaries.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs17-geothermal-energy-and-industry.md Group1B
 */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "geothermal_collector_fixture.h"
#include "../utility_power.h"
#include "../landscape_cmd.h"
#include "../multilayer_state.h"
#include "../safeguards.h"

/** Ordinary command estimates preserve all owners, while actual execution settles full capital once and consumes only the original cold reservation. */
TEST_CASE("Geothermal actual native funding settles capital once without completing construction", "[geothermal-collector]")
{
	GeothermalCollectorRig rig;
	const auto cash = Company::Get(_current_company)->money;
	const auto geology = ComputeGeologyStateDigest(GetGeologyWorldState()), power = ComputeUtilityPowerDigest(), cold = ComputeGeothermalSiteDigest();
	const auto random = std::array{_random.state[0], _random.state[1]};
	const auto estimate = rig.Fund(true); REQUIRE(estimate.Succeeded()); CHECK(estimate.GetCost() == Money{180000});
	CHECK(Company::Get(_current_company)->money == cash); CHECK(Industry::GetNumItems() == 0); CHECK(ComputeGeothermalSiteDigest() == cold);
	CHECK(std::array{_random.state[0], _random.state[1]} == random);
	const auto result = rig.Fund(); REQUIRE(result.Succeeded()); CHECK(result.GetCost() == estimate.GetCost());
	CHECK(Company::Get(_current_company)->money == cash - Money{180000}); CHECK_FALSE(GetGeothermalSiteClaim(rig.site)->active);
	const auto &industry = rig.Collector(); REQUIRE(industry.accepted.size() == 3); CHECK(industry.produced.empty());
	CHECK(industry.location.w == 3); CHECK(industry.location.h == 3); CHECK(industry.owner == OWNER_NONE); CHECK(industry.founder == _current_company);
	CHECK_FALSE(IsGeothermalCollectorOperational(industry.index)); CHECK(GetGeothermalCollectorState(industry.index)->work_ticks == 0);
	CHECK(rig.Fund().Failed()); CHECK(Company::Get(_current_company)->money == cash - Money{180000});
	CHECK(ClaimGeothermalSite(rig.request).error != GeothermalError::None);
	CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == geology); CHECK(ComputeUtilityPowerDigest() == power);
}

/** Full quote refusal precedes native identity, source, cash, RNG and machinery changes. */
TEST_CASE("Geothermal actual collector funding refuses cash date owner cancelled claim or occupied floor", "[geothermal-collector]")
{
	GeothermalCollectorRig rig;
	SECTION("short cash") { Company::Get(_current_company)->money = 179999; }
	SECTION("pre1980") { TimerGameCalendar::date = TimerGameCalendar::ConvertYMDToDate(TimerGameCalendar::Year{1979}, 11, 31); }
	SECTION("foreign actor") { _current_company = OWNER_DEITY; }
	SECTION("cancelled reservation") { REQUIRE(CancelGeothermalSite(rig.site) == GeothermalError::None); }
	SECTION("occupied corner") { MakeRailNormal(TileAddWrap(rig.request.tile, 2, 2), rig.request.owner, TrackBits{Track::X}, RAILTYPE_RAIL); }
	const auto cold = ComputeGeothermalSiteDigest(), geology = ComputeGeologyStateDigest(GetGeologyWorldState());
	const auto random = std::array{_random.state[0], _random.state[1]};
	const auto cash = Company::Get(rig.request.owner)->money;
	CHECK(CmdBuildGeothermalCollector(DoCommandFlag::Execute, rig.request.tile, rig.site, 123).Failed());
	CHECK(Industry::GetNumItems() == 0); CHECK(FindGeothermalCollectorForVent(rig.request.vent) == IndustryID::Invalid());
	CHECK(Company::Get(rig.request.owner)->money == cash); CHECK(ComputeGeothermalSiteDigest() == cold);
	CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == geology); CHECK(std::array{_random.state[0], _random.state[1]} == random);
}

/** Complete real inputs are consumed together once; shortage/date/pause and duplicate native ticks retain exact work, native stock and source capacity. */
TEST_CASE("Geothermal construction uses the whole native bill and296 paid simulation ticks", "[geothermal-collector]")
{
	GeothermalCollectorRig rig; REQUIRE(rig.Fund().Succeeded()); auto &industry = rig.Collector();
	const auto geology = ComputeGeologyStateDigest(GetGeologyWorldState()), power = ComputeUtilityPowerDigest();
	const auto cash = Company::Get(_current_company)->money;
	CHECK(rig.Deliver(CargoLabel{"STEL"}, 100) == 8); CHECK(rig.Deliver(CargoLabel{"RFMT"}, 100) == 4);
	rig.Tick(300); CHECK(GetGeothermalCollectorState(industry.index)->work_ticks == 0);
	CHECK(industry.GetCargoAccepted(GetCargoTypeByLabel(CargoLabel{"STEL"}))->waiting == 8);
	CHECK(industry.GetCargoAccepted(GetCargoTypeByLabel(CargoLabel{"RFMT"}))->waiting == 4);
	CHECK(Command<Commands::LandscapeClear>::Do(DoCommandFlag::Execute, rig.request.tile).Failed());
	CHECK(rig.Deliver(CargoLabel{"ELEC"}, 100) == 2); CHECK(rig.Deliver(CargoLabel{"STEL"}, 1) == 0);
	rig.Tick(); REQUIRE(GetGeothermalCollectorState(industry.index)->materials_committed); CHECK(GetGeothermalCollectorState(industry.index)->work_ticks == 1);
	// Native accepted history preserves actual delivery after construction
	// consumes stock. There is no private inventory or delivery disappearance.
	for (const auto &input : GetMultiLayerGeothermalConstruction(MultiLayerGeothermalConstructionKind::Collector)->inputs) {
		const auto slot = industry.GetCargoAccepted(GetCargoTypeByLabel(input.label)); REQUIRE(slot != industry.accepted.end());
		CHECK(slot->waiting == 0); REQUIRE(slot->history != nullptr); CHECK((*slot->history)[THIS_MONTH].accepted == input.quantity);
	}
	{
		AutoRestoreBackup pause(_pause_mode, PauseModes{});
		AdvanceGeothermalCollector(industry); AdvanceGeothermalCollector(industry); CHECK(GetGeothermalCollectorState(industry.index)->work_ticks == 1);
	}
	TimerGameCalendar::date = TimerGameCalendar::ConvertYMDToDate(TimerGameCalendar::Year{1979}, 11, 31);
	rig.Tick(10); CHECK(GetGeothermalCollectorState(industry.index)->work_ticks == 1); CHECK_FALSE(IsGeothermalCollectorOperational(industry.index));
	TimerGameCalendar::date = TimerGameCalendar::ConvertYMDToDate(TimerGameCalendar::Year{1980}, 0, 1);
	rig.Tick(294); CHECK(GetGeothermalCollectorState(industry.index)->work_ticks == 295); CHECK_FALSE(IsGeothermalCollectorOperational(industry.index));
	rig.Tick(); CHECK(GetGeothermalCollectorState(industry.index)->work_ticks == 296); CHECK(IsGeothermalCollectorOperational(industry.index));
	for (const auto tile : industry.location) CHECK(IsIndustryCompleted(tile)); // Every actual3x3 machinery cell completes at the same paid boundary.
	CHECK(rig.Deliver(CargoLabel{"ELEC"}, 1) == 0); rig.Tick(50); CHECK(GetGeothermalCollectorState(industry.index)->work_ticks == 296);
	CHECK(Company::Get(_current_company)->money == cash); CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == geology); CHECK(ComputeUtilityPowerDigest() == power);
}

/** Saved construction transport restores only actual native machinery/work/stock references; shortages remain recoverable without repeating capital or inventing heat. */
TEST_CASE("Geothermal actual construction staging retains shortages and native once-only history", "[geothermal-collector-save]")
{
	GeothermalCollectorRig rig; REQUIRE(rig.Fund().Succeeded()); auto &industry = rig.Collector();
	REQUIRE(rig.Deliver(CargoLabel{"STEL"}, 8) == 8);
	const auto rows = CaptureGeothermalCollectors(); const auto before = ComputeGeothermalCollectorDigest(); const auto cash = Company::Get(_current_company)->money;
	REQUIRE(ValidateGeothermalCollectors(rows)); ResetGeothermalCollectors(); StageLoadedGeothermalCollectors(rows); REQUIRE(AcceptLoadedGeothermalCollectors());
	CHECK(ComputeGeothermalCollectorDigest() == before); CHECK(Company::Get(_current_company)->money == cash);
	rig.Tick(10); CHECK(GetGeothermalCollectorState(industry.index)->work_ticks == 0);
	REQUIRE(rig.Deliver(CargoLabel{"RFMT"}, 4) == 4); REQUIRE(rig.Deliver(CargoLabel{"ELEC"}, 2) == 2); rig.Tick(100);
	const auto active = CaptureGeothermalCollectors(); const auto active_digest = ComputeGeothermalCollectorDigest();
	ResetGeothermalCollectors(); StageLoadedGeothermalCollectors(active); REQUIRE(AcceptLoadedGeothermalCollectors());
	CHECK(ComputeGeothermalCollectorDigest() == active_digest);
	{
		AutoRestoreBackup pause(_pause_mode, PauseModes{}); AdvanceGeothermalCollector(industry);
		CHECK(GetGeothermalCollectorState(industry.index)->work_ticks == 100); // Saved tick history forbids a duplicate labour credit on immediate retry.
	}
	rig.Tick(196); CHECK(IsGeothermalCollectorOperational(industry.index));
	CHECK(Company::Get(_current_company)->money == cash); CHECK(industry.GetCargoAccepted(GetCargoTypeByLabel(CargoLabel{"STEL"}))->waiting == 0);
}

/** Scalar, reference, material and native completion corruption refuses wholesale, preserving existing published construction and native stocks. */
TEST_CASE("Geothermal actual construction rejects corrupt saved participants", "[geothermal-collector-save]")
{
	GeothermalCollectorRig rig; REQUIRE(rig.Fund().Succeeded()); auto &industry = rig.Collector();
	auto rows = CaptureGeothermalCollectors();
	SECTION("missing native participant") { rows.clear(); }
	SECTION("invalid industry") { rows[0].industry = IndustryID{63999}; }
	SECTION("missing consumed blueprint") { rows[0].site = GeothermalSiteID{255}; }
	SECTION("free material receipt") { rows[0].materials_committed = true; }
	SECTION("excess work") { rows[0].work_ticks = 297; rows[0].materials_committed = true; rows[0].has_work_tick = true; }
	SECTION("duplicate native participant") { rows.push_back(rows.front()); }
	SECTION("free native tile completion") { SetIndustryCompleted(rig.request.tile); }
	const auto digest = ComputeGeothermalCollectorDigest();
	CHECK_FALSE(ValidateGeothermalCollectors(rows)); StageLoadedGeothermalCollectors(rows); CHECK_FALSE(AcceptLoadedGeothermalCollectors());
	CHECK(ComputeGeothermalCollectorDigest() == digest); CHECK_FALSE(IsGeothermalCollectorOperational(industry.index));
}

/** Real takeover/demolition/rebuild preserves natural identity/capacity and cavity while native cargo/work remain under their original owners. */
TEST_CASE("Geothermal actual collector lifecycle retains paid work and retires only machinery", "[geothermal-collector-lifecycle]")
{
	GeothermalCollectorRig rig; REQUIRE(rig.Fund().Succeeded()); auto &industry = rig.Collector(); const auto id = industry.index;
	const auto natural = GetGeologyWorldState().vents.front(); const auto geology = ComputeGeologyStateDigest(GetGeologyWorldState()), power = ComputeUtilityPowerDigest();
	REQUIRE(rig.Deliver(CargoLabel{"STEL"}, 8) == 8); REQUIRE(rig.Deliver(CargoLabel{"RFMT"}, 4) == 4); REQUIRE(rig.Deliver(CargoLabel{"ELEC"}, 2) == 2);
	rig.Tick(148); const auto before = *GetGeothermalCollectorState(id);
	REQUIRE(Company::CanAllocateItem()); Company *successor = Company::Create(); successor->money = 1000000; successor->clear_limit = 64U << 16; const auto new_owner = successor->index;
	ChangeOwnershipOfCompanyItems(rig.request.owner, new_owner);
	REQUIRE(GetGeothermalCollectorState(id) != nullptr); CHECK(GetGeothermalCollectorState(id)->owner == new_owner);
	CHECK(GetGeothermalCollectorState(id)->work_ticks == before.work_ticks); CHECK(GetGeothermalCollectorState(id)->last_work_tick == before.last_work_tick);
	_current_company = new_owner; rig.Tick(148); CHECK(IsGeothermalCollectorOperational(id));
	CHECK(Command<Commands::LandscapeClear>::Do(DoCommandFlag::Execute, rig.request.tile).Succeeded());
	CHECK_FALSE(Industry::IsValidID(id)); CHECK(FindGeothermalCollectorForVent(natural.id) == IndustryID::Invalid());
	CHECK(GetGeologyWorldState().vents.front().id == natural.id); CHECK(GetGeologyWorldState().vents.front().thermal_capacity_w == natural.thermal_capacity_w);
	CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == geology); CHECK_FALSE(GetGeothermalSiteClaim(rig.site)->active);
	auto request = rig.request; request.owner = new_owner; const auto rebuilt = ClaimGeothermalSite(request); REQUIRE(rebuilt.error == GeothermalError::None);
	CHECK(rebuilt.id != rig.site); CHECK(ComputeUtilityPowerDigest() == power);
	CHECK_FALSE(IsGeothermalCollectorOperational(id)); CHECK(GetGeologyWorldState().terrain.Get(request.tile) == ExcavationState::Excavated);
	_current_company = rig.request.owner; delete successor;
}

/** Paused/editor or abandoned actual construction retains its bill/work/source binding without earning free labour or electricity; original stock/work/source ownership remains recoverable through the native lifecycle. */
TEST_CASE("Geothermal interrupted collector holds construction and exclusive source through abandonment", "[geothermal-collector-lifecycle]")
{
	GeothermalCollectorRig rig; REQUIRE(rig.Fund().Succeeded()); auto &industry = rig.Collector();
	REQUIRE(rig.Deliver(CargoLabel{"STEL"}, 8) == 8); REQUIRE(rig.Deliver(CargoLabel{"RFMT"}, 4) == 4); REQUIRE(rig.Deliver(CargoLabel{"ELEC"}, 2) == 2);
	++TimerGameTick::counter; AdvanceGeothermalCollector(industry); CHECK(GetGeothermalCollectorState(industry.index)->work_ticks == 0);
	{
		AutoRestoreBackup mode(_game_mode, GameMode::Editor); rig.Tick(5); CHECK(GetGeothermalCollectorState(industry.index)->work_ticks == 0);
	}
	rig.Tick(20); REQUIRE(GetGeothermalCollectorState(industry.index)->work_ticks == 20); const auto before = *GetGeothermalCollectorState(industry.index);
	ChangeOwnershipOfCompanyItems(rig.request.owner, INVALID_OWNER);
	REQUIRE(GetGeothermalCollectorState(industry.index) != nullptr); CHECK(GetGeothermalCollectorState(industry.index)->owner == CompanyID::Invalid());
	rig.Tick(10); CHECK(GetGeothermalCollectorState(industry.index)->work_ticks == before.work_ticks);
	CHECK_FALSE(IsGeothermalCollectorOperational(industry.index)); CHECK(FindGeothermalCollectorForVent(rig.request.vent) == industry.index);
	CHECK(ClaimGeothermalSite(rig.request).error != GeothermalError::None); CHECK(ValidateGeothermalCollectors(CaptureGeothermalCollectors()));
	const auto schema = CaptureMultiLayerStateSchema(); CHECK((schema.feature_flags & MULTILAYER_FEATURE_GEOTHERMAL_COLLECTORS) != 0);
	CHECK(ValidateMultiLayerStateReferences(schema) == MultiLayerStateError::None);
	for (const auto dependency : {MULTILAYER_FEATURE_GEOTHERMAL_SITES, MULTILAYER_FEATURE_VENTS, MULTILAYER_FEATURE_GEOLOGY}) {
		auto missing = schema; missing.feature_flags &= ~dependency; CHECK(ValidateMultiLayerStateReferences(missing) == MultiLayerStateError::UnsupportedFeatures); // Actual collector metadata cannot omit its original blueprint/natural source owners.
	}
}
