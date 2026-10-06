/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file underground_filter.cpp Public explicit-context logical fleet filter, independent of runtime engine purchase. @see docs/zz-reach2/upgrades/2026-10/r2mlvs7-underground-construction-and-fleet.md, Group 5 */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "excavation_fixture.h"
#include "../underground_gui.h"
#include "../safeguards.h"

/** Direct callers receive immutable catalogue designs with separate tube/conventional roles and identical calendar policy. */
TEST_CASE("Public underground fleet filter separates domains and canonical dates without mutation", "[underground-filter]")
{
	using Signature = std::vector<const LayerVehicleCapability *> (*)(MapLayer, const MultiLayerProfileSelection &, TimerGameCalendar::Date, UndergroundFleetKind);
	STATIC_REQUIRE((std::is_same_v<decltype(&BuildUndergroundVehicleFilter), Signature>));
	ExcavationFixture fixture;
	const auto policy = GetCurrentMultiLayerProfileSelection();
	const auto before = ComputeGeologyStateDigest(GetGeologyWorldState());
	const auto cash = Company::Get(_current_company)->money;
	const auto initial = TimerGameCalendar::ConvertYMDToDate(TimerGameCalendar::Year{1950}, 0, 1);
	const auto deep = TimerGameCalendar::ConvertYMDToDate(TimerGameCalendar::Year{1980}, 0, 1);
	const auto late = TimerGameCalendar::ConvertYMDToDate(TimerGameCalendar::Year{2005}, 0, 1);
	const auto conventional = BuildUndergroundVehicleFilter(MapLayer::Underground, policy, initial, UndergroundFleetKind::ConventionalFreight);
	REQUIRE(conventional.size() == 5);
	const auto catalogue = GetMultiLayerVehicleCapabilities();
	// Every returned row must retain its original immutable identity and depot domain; cutters and capsules never leak in.
	for (const auto *model : conventional) {
		CHECK(model->transport != MultiLayerVehicleTransport::Tube);
		CHECK(model->commissioning != MultiLayerCommissioningFacility::RailPortalBay);
		const auto original = std::ranges::find(catalogue, model->model_id, &LayerVehicleCapability::model_id);
		REQUIRE(original != catalogue.end());
		CHECK(&*original == model);
	}
	CHECK(BuildUndergroundVehicleFilter(MapLayer::DeepUnderground, policy, initial, UndergroundFleetKind::ConventionalFreight).empty());
	CHECK(BuildUndergroundVehicleFilter(MapLayer::DeepUnderground, policy, deep, UndergroundFleetKind::ConventionalFreight).size() == 7);
	CHECK(BuildUndergroundVehicleFilter(MapLayer::Underground, policy, initial, UndergroundFleetKind::TubeCapsules).empty());
	CHECK(BuildUndergroundVehicleFilter(MapLayer::DeepUnderground, policy, deep, UndergroundFleetKind::TubeCapsules).size() == 3);
	const auto tubes = BuildUndergroundVehicleFilter(MapLayer::DeepUnderground, policy, late, UndergroundFleetKind::TubeCapsules);
	REQUIRE(tubes.size() == 5);
	// These are profile-owned logical tube rows, not invented runtime engines or authority to build a missing tube network.
	for (const auto *model : tubes) {
		CHECK(model->transport == MultiLayerVehicleTransport::Tube);
		CHECK_FALSE(model->conventional_portals);
		const auto original = std::ranges::find(catalogue, model->model_id, &LayerVehicleCapability::model_id);
		REQUIRE(original != catalogue.end());
		CHECK(&*original == model);
	}
	CHECK(BuildUndergroundVehicleFilter(MapLayer::Surface, policy, late, UndergroundFleetKind::ConventionalFreight).empty());
	CHECK(BuildUndergroundVehicleFilter(MapLayer::Space, policy, late, UndergroundFleetKind::TubeCapsules).empty());
	CHECK(BuildUndergroundVehicleFilter(MapLayer::Invalid, policy, late, UndergroundFleetKind::ConventionalFreight).empty());
	CHECK(BuildUndergroundVehicleFilter(MapLayer::Underground, policy, late, static_cast<UndergroundFleetKind>(255)).empty());
	auto invalid = policy;
	++invalid.version;
	CHECK(BuildUndergroundVehicleFilter(MapLayer::Underground, invalid, late, UndergroundFleetKind::ConventionalFreight).empty());
	CHECK(BuildUndergroundVehicleFilter(MapLayer::Underground, MultiLayerProfileSelection{}, late, UndergroundFleetKind::ConventionalFreight).empty());
	CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == before);
	CHECK(Company::Get(_current_company)->money == cash);
}
