/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file tbm_model.cpp Cutter policy and real articulated/wagon-boundary fixtures. @see docs/zz-reach2/upgrades/2026-10/r2mlvs4-tunnel-boring-machines.md, Group 1 */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "excavation_fixture.h"
#include "../tbm.h"
#include "../train.h"
#include "../vehicle_func.h"

#include "../safeguards.h"

/** Verify the native profile controls each cutter's date, layers, facilities and separate recurring charges. */
TEST_CASE("TBM models consume the native profile's immutable date and layer capability", "[tbm-model]")
{
	const MultiLayerProfileSelection policy{MultiLayerGameplayProfileID::MultilayerIndustry, 1, false, false};
	const auto before = TimerGameCalendar::ConvertYMDToDate(TimerGameCalendar::Year{1979}, 11, 31);
	const auto unlock = TimerGameCalendar::ConvertYMDToDate(TimerGameCalendar::Year{1980}, 0, 1);
	REQUIRE(GetTBMCutterModels().size() == 2);
	CHECK(IsTBMCutterAvailable(0, MapLayer::Underground, policy, before));
	CHECK_FALSE(IsTBMCutterAvailable(0, MapLayer::DeepUnderground, policy, unlock));
	CHECK_FALSE(IsTBMCutterAvailable(1, MapLayer::Underground, policy, before));
	CHECK(IsTBMCutterAvailable(1, MapLayer::DeepUnderground, policy, unlock));
	CHECK_FALSE(IsTBMCutterAvailable(1, MapLayer::Space, policy, unlock));
	CHECK_FALSE(IsTBMCutterAvailable(0, MapLayer::Invalid, policy, unlock));
	CHECK_FALSE(IsTBMCutterAvailable(0, MapLayer::Underground, {policy.id, 1, true, false}, unlock));
	CHECK_FALSE(IsTBMCutterAvailable(2, MapLayer::Underground, policy, unlock));
	CHECK_FALSE(IsTBMCutterAvailable(0, MapLayer::Surface, {}, unlock));
	CHECK_FALSE(IsTBMCutterAvailable(0, MapLayer::Underground, {policy.id, 2, false, false}, unlock));
	CHECK(GetTBMCutterModel(UINT16_MAX) == nullptr);
	// Both models must expose a viable idle charge and dedicated separately compatible Earth/Rock wagon designs.
	for (const auto &model : GetTBMCutterModels()) {
		CHECK(model.active_cost_per_year > model.idle_cost_per_year);
		CHECK(model.idle_cost_per_year > 0);
		CHECK(model.work_rate_denominator > 0);
		const auto capabilities = GetMultiLayerVehicleCapabilities();
		const auto earth = std::ranges::find(capabilities, model.earth_wagon_model, &LayerVehicleCapability::model_id);
		const auto rock = std::ranges::find(capabilities, model.rock_wagon_model, &LayerVehicleCapability::model_id);
		REQUIRE(earth != capabilities.end()); REQUIRE(rock != capabilities.end());
		CHECK(earth->cargo_labels.front() == CargoLabel{"ERTH"});
		CHECK(rock->cargo_labels.front() == CargoLabel{"ROCK"});
	}
}

/** Isolated real pool-owned chain; reference construction tests layout without claiming commissioned runtime movement. */
class TBMLayoutFixture {
public:
	ExcavationFixture world; ///< Real shared content/profile and company owner.
	Train *cutter = nullptr; ///< Actual front-engine reference.
	Train *support = nullptr; ///< Actual permanent articulated support reference.
	Train *earth = nullptr; ///< Real whole Earth wagon reference.
	Train *rock = nullptr; ///< Real whole Rock wagon reference.
	TBMProject project; ///< Fixed-prefix authority under test.
	/** Allocate only the fixture-owned reference chain through the real train pool. */
	TBMLayoutFixture()
	{
		REQUIRE(Vehicle::GetNumItems() == 0);
		REQUIRE(Vehicle::CanAllocateItem(4));
		cutter = Train::Create(); support = Train::Create(); earth = Train::Create(); rock = Train::Create();
		cutter->SetFrontEngine(); cutter->SetEngine(); support->SetArticulatedPart();
		earth->SetWagon(); rock->SetWagon();
		// Reference parts use actual pool and chain semantics but remain hidden, avoiding uninitialized renderer state.
		for (auto *part : {cutter, support, earth, rock}) { part->owner = _current_company; part->vehstatus.Set(VehState::Hidden); }
		cutter->SetNext(support); support->SetNext(earth); earth->SetNext(rock);
		earth->cargo_type = GetCargoTypeByLabel(CargoLabel{"ERTH"}); earth->cargo_cap = 64;
		rock->cargo_type = GetCargoTypeByLabel(CargoLabel{"ROCK"}); rock->cargo_cap = 128;
		project.cutter = cutter->index; project.company = _current_company; project.model_id = 0;
		project.permanent_parts = {cutter->index, support->index};
	}
	/** Release the fixture's sole four parts through actual pool cleanup and reset the real derived spatial hash. */
	~TBMLayoutFixture()
	{
		assert(Vehicle::GetNumItems() == 4);
		_vehicle_pool.CleanPool(); ResetVehicleHash();
	}
};

/** Verify immutable articulated machinery and both real wagon boundaries, including malformed ownership/cargo/layout refusals. */
TEST_CASE("TBM consist layout keeps permanent machinery separate from complete spoil units", "[tbm-consist]")
{
	TBMLayoutFixture fixture;
	const auto layout = InspectTBMConsist(*fixture.cutter, fixture.project);
	REQUIRE(layout.error == TBMBlockedReason::None);
	REQUIRE(layout.exchangeable_units.size() == 2);
	CHECK(layout.exchangeable_units[0].vehicle == fixture.earth->index);
	CHECK(layout.exchangeable_units[0].role == TBMConsistRole::EarthWagon);
	CHECK(layout.exchangeable_units[1].vehicle == fixture.rock->index);
	CHECK(layout.exchangeable_units[1].role == TBMConsistRole::RockWagon);
	CHECK(fixture.earth->GetPrevUnit() == fixture.cutter);
	fixture.project.permanent_parts.pop_back();
	CHECK(InspectTBMConsist(*fixture.cutter, fixture.project).error == TBMBlockedReason::InvalidConsist);
	fixture.project.permanent_parts.push_back(fixture.support->index);
	fixture.rock->cargo_type = GetCargoTypeByLabel(CargoLabel{"COAL"});
	CHECK(InspectTBMConsist(*fixture.cutter, fixture.project).error == TBMBlockedReason::InvalidConsist);
	fixture.rock->cargo_type = GetCargoTypeByLabel(CargoLabel{"ROCK"});
	fixture.support->cargo_cap = 1;
	CHECK(InspectTBMConsist(*fixture.cutter, fixture.project).error == TBMBlockedReason::InvalidConsist);
	fixture.support->cargo_cap = 0;
	fixture.earth->owner = CompanyID::Invalid();
	CHECK(InspectTBMConsist(*fixture.cutter, fixture.project).error == TBMBlockedReason::InvalidConsist);
	CHECK(fixture.project.job == ExcavationJobID{UINT32_MAX});
	CHECK(fixture.project.heading == Direction::Invalid);
	CHECK(fixture.project.phase_ticks == 0);
}
