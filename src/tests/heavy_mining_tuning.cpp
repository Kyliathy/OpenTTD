/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file heavy_mining_tuning.cpp Cross-contract role, work/service and legacy-budget invariants before heavy runtime integration.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group9
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../space_mining_tuning.h"
#include "../space_orders.h"
#include "../multilayer_space_content.h"
#include "../safeguards.h"

/** Existing paid small miners retain the delivered work/hold/service budget; unrelated roles do not gain extraction capability. */
TEST_CASE("Per-miner definitions preserve the delivered small miner and construction role", "[heavy-mining-tuning]")
{
	const auto *probe = GetSpaceMiningTuning(15); REQUIRE(probe != nullptr); const auto &old = GetMultiLayerSpaceBudget();
	CHECK(probe->hold_sixteenths == old.probe_hold_sixteenths);
	CHECK(probe->extraction_units == old.mining_units); CHECK(probe->extraction_ticks == old.mining_interval);
	CHECK(probe->active_per_day == old.active_per_day); CHECK(probe->idle_per_day == old.idle_per_day);
	CHECK(probe->service_ticks == old.service_ticks); CHECK(probe->service_metal == 1); CHECK(probe->service_electronics == 1);
	CHECK(probe->returns_per_service == old.returns_per_service); CHECK(probe->starting_pods == old.starting_pods);
	// A content query cannot silently repurpose saved construction/hauler/survey bodies or a future unknown model as miners.
	for (uint16_t model : {uint16_t{14}, uint16_t{16}, uint16_t{17}, uint16_t{18}, uint16_t{19}, uint16_t{20}, uint16_t{22}, uint16_t{UINT16_MAX}}) CHECK(GetSpaceMiningTuning(model) == nullptr);
	CHECK_FALSE(IsSpaceOrderAllowed(16, {SpaceOrderAction::Mine, {SpaceOrderTargetKind::Body, 0, 0}}));
}

/** Heavy work scales throughput without shortening fill time; doubled real service/upkeep gives a separately accountable two-return work ledger. */
TEST_CASE("Heavy miner work and service form one bounded two-return operating contract", "[heavy-mining-tuning]")
{
	const auto *probe = GetSpaceMiningTuning(15), *heavy = GetSpaceMiningTuning(SPACE_HEAVY_MINER_MODEL); REQUIRE(probe != nullptr); REQUIRE(heavy != nullptr);
	const uint32_t small_fill = probe->hold_sixteenths / 16 / probe->extraction_units * probe->extraction_ticks;
	const uint32_t heavy_fill = heavy->hold_sixteenths / 16 / heavy->extraction_units * heavy->extraction_ticks;
	CHECK(heavy_fill == small_fill); CHECK(heavy_fill == 740); CHECK(heavy->hold_sixteenths == 2 * probe->hold_sixteenths);
	CHECK(heavy->service_ticks == 2 * probe->service_ticks); CHECK(heavy->service_metal == 2 * probe->service_metal); CHECK(heavy->service_electronics == 2 * probe->service_electronics);
	CHECK(heavy->returns_per_service == probe->returns_per_service); CHECK(heavy->starting_pods == probe->starting_pods);
	const uint64_t active_ticks = uint64_t{heavy_fill} * heavy->returns_per_service + heavy->service_ticks;
	CHECK(active_ticks == 1776); CHECK(active_ticks * heavy->active_per_day / 74 == 24000);
	CHECK(heavy->idle_per_day == 2 * probe->idle_per_day);
}
