/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file multilayer_content.cpp Typed public handoff and calendar-boundary fixtures. @see docs/zz-reach2/upgrades/2026-10/r2mlvs1-gameplay-contracts-and-content.md */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../multilayer_content.h"
#include "../timer/timer_game_economy.h"

#include "../safeguards.h"

/** Verify deep access uses calendar boundary and explicit saved policy. */
TEST_CASE("Deep access uses calendar boundary and explicit saved policy", "[multilayer-content-contract]")
{
	const auto before = TimerGameCalendar::ConvertYMDToDate(TimerGameCalendar::Year{1979}, 11, 31);
	const auto on = TimerGameCalendar::ConvertYMDToDate(TimerGameCalendar::Year{1980}, 0, 1);
	const auto after = TimerGameCalendar::ConvertYMDToDate(TimerGameCalendar::Year{1980}, 0, 2);
	MultiLayerProfileSelection selected{MultiLayerGameplayProfileID::MultilayerIndustry, 1, false, false};
	CHECK_FALSE(CheckDeepLayerAvailability(selected, before));
	CHECK(CheckDeepLayerAvailability(selected, on));
	CHECK(CheckDeepLayerAvailability(selected, after));

	MultiLayerProfileSelection legacy;
	CHECK(CheckDeepLayerAvailability(legacy, before));
	CHECK(CheckDeepLayerAvailability(legacy, on));
	CHECK(CheckDeepLayerAvailability(legacy, after));
	selected.editor_override = true;
	CHECK(CheckDeepLayerAvailability(selected, before));
	selected.version = 2;
	CHECK_FALSE(CheckDeepLayerAvailability(selected, before));
	selected.version = 1;
	selected.legacy = true;
	CHECK_FALSE(CheckDeepLayerAvailability(selected, after));
	selected.id = MultiLayerGameplayProfileID::Invalid;
	CHECK_FALSE(CheckDeepLayerAvailability(selected, after));
	legacy.version = 1;
	CHECK_FALSE(CheckDeepLayerAvailability(legacy, after));

	// A caller cannot substitute an economic date even if its numeric day count is beyond 1980.
	STATIC_REQUIRE_FALSE(std::is_invocable_v<decltype(&CheckDeepLayerAvailability), const MultiLayerProfileSelection &, TimerGameEconomy::Date>);
	STATIC_REQUIRE(std::is_invocable_v<decltype(&CheckDeepLayerAvailability), const MultiLayerProfileSelection &, TimerGameCalendar::Date>);
}

/** Verify public content contracts keep invalid capability and utility references explicit. */
TEST_CASE("Public content contracts keep invalid capability and utility references explicit", "[multilayer-content-contract]")
{
	const auto *profile = GetMultiLayerContentProfile(MultiLayerGameplayProfileID::MultilayerIndustry);
	REQUIRE(profile != nullptr);
	CHECK(profile->version == 1);
	CHECK(profile->supported_layers.Test(MapLayer::Space));
	CHECK(profile->supported_layers.Test(MapLayer::DeepUnderground));
	CHECK(to_underlying(MapLayer::Space) == 2);
	CHECK(to_underlying(MapLayer::DeepUnderground) == 3);
	CHECK(GetMultiLayerContentProfile(static_cast<MultiLayerGameplayProfileID>(2)) == nullptr);

	LayerVehicleCapability unset;
	CHECK(unset.model_id == UINT16_MAX);
	CHECK(unset.transport == MultiLayerVehicleTransport::Invalid);
	CHECK(unset.commissioning == MultiLayerCommissioningFacility::Invalid);
	CHECK(unset.service == MultiLayerServiceFacility::Invalid);
	CHECK_FALSE(unset.conventional_portals);
	CHECK(unset.cargo_labels.empty());

	UtilityPowerAccount account;
	CHECK(account.producer_id.base() == UINT32_MAX);
	CHECK(account.consumer_id.base() == UINT32_MAX);
	CHECK(account.layer == MapLayer::Invalid);
	CHECK(account.generation_energy_j == 0);
	CHECK(account.granted_energy_j == 0);
	CHECK(account.remainder_mj == 0);
	STATIC_REQUIRE_FALSE(std::is_same_v<UtilityProducerID, UtilityConsumerID>);
	STATIC_REQUIRE(std::is_same_v<decltype(account.generation_energy_j), uint64_t>);
	STATIC_REQUIRE(std::is_same_v<decltype(GetMultiLayerContentProfile(MultiLayerGameplayProfileID::Classic)), const MultiLayerGameplayProfile *>);
}
