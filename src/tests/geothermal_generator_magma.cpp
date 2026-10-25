/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file geothermal_generator_magma.cpp Actual dated Magma machinery, paid work/service and two-model native contention.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs18-geothermal-energy-and-industry.md Groups8E/14B.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs1-f-utility-and-transfer-contracts.md Group19.
 * Prepared floor and explicit receiver-boundary materials leave the complete paid-TBM/carrier bootstrap to8F.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "geothermal_generator_fixture.h"
#include "../industry_map.h"
#include "../landscape_cmd.h"
#include "../safeguards.h"

/** The actual2000 cold claim, command, capability and native stock/work share one canonical dated model. */
TEST_CASE("Magma native construction preserves its exact2000 gate and higher paid bill", "[geothermal-magma-construction]")
{
	GeothermalGeneratorRig rig{false, GeothermalGeneratorModel::MagmaLoop};
	const auto type = GetGeothermalGeneratorIndustryType(GeothermalGeneratorModel::MagmaLoop);
	const auto *terms = GetMultiLayerGeothermalConstruction(rig.request.kind); REQUIRE(terms != nullptr);
	CHECK(terms->kind == MultiLayerGeothermalConstructionKind::MagmaPowerhouse);
	CHECK(terms->capital_cost == 360000); CHECK(terms->work_ticks == 888);
	CHECK(GetGeothermalGeneratorIndustryCapability(type)->asset_id == "facility.magma-powerhouse");
	const auto cash = Company::Get(_current_company)->money;
	TimerGameCalendar::date = TimerGameCalendar::ConvertYMDToDate(TimerGameCalendar::Year{1999}, 11, 31);
	CHECK(PrepareGeothermalSite(rig.request).error == GeothermalError::Unavailable);
	CHECK(ClaimGeothermalSite(rig.request).error == GeothermalError::Unavailable);
	CHECK(CheckLayerIndustryConstruction(rig.request.tile, type).Failed()); CHECK(rig.Fund().Failed());
	CHECK(GetGeothermalGeneratorIndustryType(GeothermalGeneratorModel::MagmaLoop) == type);
	CHECK(Industry::GetNumItems() == 0); CHECK(Company::Get(_current_company)->money == cash);
	TimerGameCalendar::date = TimerGameCalendar::ConvertYMDToDate(TimerGameCalendar::Year{2000}, 0, 1);
	CHECK(CheckLayerIndustryConstruction(rig.request.tile, type).Succeeded());
	CHECK(CheckLayerIndustryConstruction(MultiLayerMap::ProjectToSurface(rig.request.tile), type).Failed());
	CHECK(CmdBuildIndustry({}, rig.request.tile, type, 0, true, 123).Failed());
	REQUIRE(rig.Fund(true).Succeeded()); CHECK(Company::Get(_current_company)->money == cash);
	Company::Get(_current_company)->money = 359999; CHECK(rig.Fund().Failed()); CHECK(Industry::GetNumItems() == 0);
	Company::Get(_current_company)->money = cash; REQUIRE(rig.Fund().Succeeded());
	CHECK(Company::Get(_current_company)->money == cash - Money{360000});
	CHECK(rig.Generator().type == type); CHECK(rig.Generator().location.w == 4); CHECK(rig.Generator().location.h == 4);
	CHECK(rig.Deliver(CargoLabel{"STEL"}, 25) == 24); CHECK(rig.Deliver(CargoLabel{"RFMT"}, 16) == 16);
	CHECK(rig.Deliver(CargoLabel{"ELEC"}, 8) == 8); rig.Tick(100);
	CHECK(GetGeothermalGeneratorState(rig.Generator().index)->work_ticks == 0);
	CHECK(rig.Deliver(CargoLabel{"BATY"}, 4) == 4); rig.Tick(887);
	CHECK_FALSE(IsGeothermalGeneratorConstructionComplete(rig.Generator().index)); rig.Tick(1);
	CHECK(IsGeothermalGeneratorConstructionComplete(rig.Generator().index));
	CHECK(GetGeothermalGeneratorState(rig.Generator().index)->model == GeothermalGeneratorModel::MagmaLoop);
	CHECK(Company::Get(_current_company)->money == cash - Money{360000}); CHECK(CaptureUtilityPowerState().producers.empty());
}

/** Calendar/profile locks suspend actual arrived materials and prepaid labour without rewriting retained machinery. */
TEST_CASE("Magma construction pauses under older calendar or incompatible profile", "[geothermal-magma-locks]")
{
	GeothermalGeneratorRig rig{false, GeothermalGeneratorModel::MagmaLoop}; REQUIRE(rig.Fund().Succeeded()); rig.DeliverBill();
	const auto original = ComputeGeothermalGeneratorDigest(); const auto cash = Company::Get(_current_company)->money;
	const auto profile = GetCurrentMultiLayerProfileSelection();
	SECTION("1999 calendar") { TimerGameCalendar::date = TimerGameCalendar::ConvertYMDToDate(TimerGameCalendar::Year{1999}, 11, 31); }
	SECTION("profile deactivated") { ResetMultiLayerProfileSelection(); }
	rig.Tick(3); CHECK(ComputeGeothermalGeneratorDigest() == original); CHECK(Company::Get(_current_company)->money == cash);
	const auto type = GetGeothermalGeneratorIndustryType(GeothermalGeneratorModel::MagmaLoop);
	CHECK(CheckLayerIndustryConstruction(rig.request.tile, type).Failed());
	CHECK(CheckLayerIndustryConstruction(MultiLayerMap::ProjectToSurface(rig.request.tile), type).Failed());
	CHECK(rig.Deliver(CargoLabel{"STEL"}, 1) == 0); CHECK(ComputeGeothermalGeneratorDigest() == original);
	REQUIRE(RestoreMultiLayerProfileSelection(profile, LandscapeType::Temperate, rig.ActivatedContent()) == MultiLayerProfileError::None);
	TimerGameCalendar::date = TimerGameCalendar::ConvertYMDToDate(TimerGameCalendar::Year{2000}, 0, 1);
	rig.Tick(888); CHECK(IsGeothermalGeneratorConstructionComplete(rig.Generator().index)); CHECK(Company::Get(_current_company)->money == cash);
}

