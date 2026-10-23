/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file geothermal_generator.cpp Actual native1980 definition, capital settlement, material shortage and synchronized work boundaries.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs18-geothermal-energy-and-industry.md Group8C.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "geothermal_generator_fixture.h"
#include "../industry_map.h"

/** The actual registered sixteen-cell machinery consumes the frozen bill, without granting future-model or utility capability. */
TEST_CASE("Native powerhouse definition matches frozen geometry and operating bounds", "[geothermal-generator-definition]")
{
	GeothermalGeneratorRig rig;
	const auto *terms = GetMultiLayerGeothermalGeneratorTerms(GeothermalGeneratorModel::VentTap); REQUIRE(terms != nullptr);
	const auto geometry = GetGeothermalGeneratorGeometry(terms->model); REQUIRE(geometry.has_value());
	const auto type = GetGeothermalGeneratorIndustryType(terms->model); REQUIRE(type != IT_INVALID);
	const auto *spec = GetIndustrySpec(type); REQUIRE(spec->layouts.size() == 1);
	CHECK(spec->layouts.front().size() == geometry->cells.size()); CHECK(terms->thermal_capacity_w == 1000000);
	CHECK(uint64_t{terms->thermal_capacity_w} * terms->efficiency_percent == uint64_t{terms->electrical_capacity_w} * 100);
	CHECK(terms->service_cost_per_day == 20); CHECK(GetGeothermalGeneratorIndustryCapability(type) != nullptr);
	const auto magma = GetGeothermalGeneratorIndustryType(GeothermalGeneratorModel::MagmaLoop); REQUIRE(magma != IT_INVALID);
	CHECK(magma != type); CHECK(GetMultiLayerGeothermalGeneratorTerms(GeothermalGeneratorModel::MagmaLoop) != nullptr);
	CHECK(GetGeothermalGeneratorIndustryCapability(magma)->introduction_year == TimerGameCalendar::Year{2000});
	CHECK(Industry::GetNumItems() == 0); CHECK(CaptureUtilityPowerState().producers.empty());
}

/** Native estimates preserve every participant; funding debits capital once and leaves actual machinery unfinished with no energy account. */
TEST_CASE("Native powerhouse funding settles original capital without construction or output", "[geothermal-generator-funding]")
{
	GeothermalGeneratorRig rig;
	const auto cash = Company::Get(_current_company)->money;
	const auto geology = ComputeGeologyStateDigest(GetGeologyWorldState()), sites = ComputeGeothermalSiteDigest();
	const auto random = std::array{_random.state[0], _random.state[1]};
	const auto quote = rig.Fund(true); REQUIRE(quote.Succeeded()); CHECK(quote.GetCost() == Money{180000});
	CHECK(Company::Get(_current_company)->money == cash); CHECK(Industry::GetNumItems() == 0);
	CHECK(ComputeGeothermalSiteDigest() == sites); CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == geology);
	CHECK(std::array{_random.state[0], _random.state[1]} == random);
	REQUIRE(rig.Fund().Succeeded()); auto &industry = rig.Generator();
	CHECK(Company::Get(_current_company)->money == cash - Money{180000});
	CHECK(industry.location.w == 4); CHECK(industry.location.h == 4); CHECK(industry.accepted.size() == 4); CHECK(industry.produced.empty());
	CHECK_FALSE(GetGeothermalSiteClaim(rig.site)->active); CHECK_FALSE(IsGeothermalGeneratorConstructionComplete(industry.index));
	CHECK(CaptureUtilityPowerState().producers.empty()); CHECK(rig.Fund().Failed());
	CHECK(Company::Get(_current_company)->money == cash - Money{180000});
	CHECK(ValidateGeothermalGenerators(CaptureGeothermalGenerators()));
}

/** Shared native command admission cannot bypass calendar, real funds, original ownership or a retired cold claim. */
TEST_CASE("Native powerhouse funding refuses unavailable original participants unchanged", "[geothermal-generator-refusal]")
{
	GeothermalGeneratorRig rig;
	std::unique_ptr<Company> foreign;
	AutoRestoreBackup actor{_current_company}; // Restore the actual original actor before the optional foreign company and fixture owners retire.
	SECTION("insufficient actual capital") { Company::Get(_current_company)->money = 179999; }
	SECTION("locked1979 calendar") { TimerGameCalendar::date = TimerGameCalendar::ConvertYMDToDate(TimerGameCalendar::Year{1979}, 11, 31); }
	SECTION("cancelled original blueprint") { REQUIRE(CancelGeothermalSite(rig.site) == GeothermalError::None); }
	SECTION("foreign funded actor") {
		REQUIRE(Company::CanAllocateItem()); foreign.reset(Company::Create()); foreign->money = 1000000; _current_company = foreign->index;
	}
	const auto cash = Company::Get(_current_company)->money;
	const auto sites = ComputeGeothermalSiteDigest(), geology = ComputeGeologyStateDigest(GetGeologyWorldState());
	const auto random = std::array{_random.state[0], _random.state[1]};
	CHECK(rig.Fund().Failed()); CHECK(Company::Get(_current_company)->money == cash); CHECK(Industry::GetNumItems() == 0);
	CHECK(ComputeGeothermalSiteDigest() == sites); CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == geology);
	CHECK(std::array{_random.state[0], _random.state[1]} == random); CHECK(CaptureGeothermalGenerators().empty());
}

/** The complete native transported bill is consumed once; late batteries, duplicate ticks and generic industry progress cannot complete a free powerhouse. */
TEST_CASE("Native powerhouse waits for its whole bill before592 prepaid work ticks", "[geothermal-generator-work]")
{
	GeothermalGeneratorRig rig; REQUIRE(rig.Fund().Succeeded()); auto &industry = rig.Generator();
	const auto cash = Company::Get(_current_company)->money;
	CHECK(rig.Deliver(CargoLabel{"STEL"}, 100) == 12); CHECK(rig.Deliver(CargoLabel{"RFMT"}, 8) == 8); CHECK(rig.Deliver(CargoLabel{"ELEC"}, 4) == 4);
	rig.Tick(100); CHECK(GetGeothermalGeneratorState(industry.index)->work_ticks == 0);
	CHECK(industry.GetCargoAccepted(GetCargoTypeByLabel(CargoLabel{"STEL"}))->waiting == 12);
	CHECK(rig.Deliver(CargoLabel{"BATY"}, 2) == 2); rig.Tick(296);
	CHECK(GetGeothermalGeneratorState(industry.index)->work_ticks == 296); CHECK_FALSE(IsGeothermalGeneratorConstructionComplete(industry.index));
	const auto digest = ComputeGeothermalGeneratorDigest();
	{
		AutoRestoreBackup pause{_pause_mode, PauseModes{}}; AdvanceGeothermalGenerator(industry);
		CHECK(ComputeGeothermalGeneratorDigest() == digest);
	}
	rig.Tick(296); CHECK(IsGeothermalGeneratorConstructionComplete(industry.index));
	for (const auto &input : GetMultiLayerGeothermalConstruction(MultiLayerGeothermalConstructionKind::Powerhouse)->inputs) {
		CHECK(industry.GetCargoAccepted(GetCargoTypeByLabel(input.label))->waiting == 0); CHECK(rig.Deliver(input.label, 1) == 0);
	}
	rig.Tick(100); CHECK(GetGeothermalGeneratorState(industry.index)->work_ticks == 592);
	CHECK(Company::Get(_current_company)->money == cash); CHECK(CaptureUtilityPowerState().producers.empty());
	CHECK(ValidateGeothermalGenerators(CaptureGeothermalGenerators()));
}

/** Temporary simulation/technology locks preserve the complete arrived bill; lifting a lock resumes prepaid work without a new invoice. */
TEST_CASE("Native powerhouse locked ticks preserve arrived construction supplies", "[geothermal-generator-work-locks]")
{
	GeothermalGeneratorRig rig; REQUIRE(rig.Fund().Succeeded()); rig.DeliverBill();
	const auto digest = ComputeGeothermalGeneratorDigest(); const auto cash = Company::Get(_current_company)->money;
	{
		AutoRestoreBackup pause{_pause_mode}; AutoRestoreBackup mode{_game_mode}; AutoRestoreBackup date{TimerGameCalendar::date};
		_pause_mode = {};
		SECTION("normal pause") { _pause_mode.Set(PauseMode::Normal); }
		SECTION("editor") { _game_mode = GameMode::Editor; }
		SECTION("calendar before Deep access") { TimerGameCalendar::date = TimerGameCalendar::ConvertYMDToDate(TimerGameCalendar::Year{1979}, 11, 31); }
		// These are actual native industry dispatch opportunities. Querying its
		// construction state in between cannot convert blocked time into labour.
		for (uint tick = 0; tick < 5; ++tick) {
			++TimerGameTick::counter; OnTick_Industry(); CHECK_FALSE(IsGeothermalGeneratorConstructionComplete(rig.Generator().index));
		}
		CHECK(ComputeGeothermalGeneratorDigest() == digest);
	}
	rig.Tick(592); CHECK(IsGeothermalGeneratorConstructionComplete(rig.Generator().index));
	CHECK(Company::Get(_current_company)->money == cash); CHECK(CaptureUtilityPowerState().producers.empty());
}

/** A detached candidate must retain its real native floor/materials and paid work; validation cannot fabricate or repair a machine. */
TEST_CASE("Native powerhouse staging rejects forged work and omitted original machinery", "[geothermal-generator-stage]")
{
	GeothermalGeneratorRig rig; REQUIRE(rig.Fund().Succeeded()); rig.DeliverBill(); rig.Tick(148);
	const auto saved = CaptureGeothermalGenerators(); const auto digest = ComputeGeothermalGeneratorDigest();
	REQUIRE(ValidateGeothermalGenerators(saved));
	StageLoadedGeothermalGenerators(saved); REQUIRE(AcceptLoadedGeothermalGenerators()); CHECK(ComputeGeothermalGeneratorDigest() == digest);
	for (unsigned fault = 0; fault < 7; ++fault) {
		auto rows = saved;
		// Each fault attacks a distinct native reference or paid receipt. None
		// may publish a partial family, rewrite stock or forgive its bad field.
		switch (fault) {
			case 0: rows.clear(); break;
			case 1: rows.push_back(rows.front()); break;
			case 2: rows.front().work_ticks = 593; break;
			case 3: rows.front().materials_committed = false; break;
			case 4: rows.front().site = GeothermalSiteID{UINT32_MAX}; break;
			case 5: rows.front().last_work_tick = TimerGameTick::counter + 1; break;
			case 6: rows.front().model = GeothermalGeneratorModel::MagmaLoop; break;
		}
		CHECK_FALSE(ValidateGeothermalGenerators(rows)); StageLoadedGeothermalGenerators(rows); CHECK_FALSE(AcceptLoadedGeothermalGenerators());
		CHECK(ComputeGeothermalGeneratorDigest() == digest);
	}
}
