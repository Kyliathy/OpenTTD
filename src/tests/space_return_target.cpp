/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file space_return_target.cpp Original route, native module capacity, exact identity and actual arrived freight checks.
 * @see docs/zz-reach2/upgrades/2026-10/r2sff1-space-fixes-features-1.md Group19.
 */
#include "../stdafx.h"
#include "space_return_fixture.h"
#include "../space_manifest.h"
#include "../safeguards.h"

/** Visits and another work action remain real queue boundaries; nearest policy changes only receiver choice within the established domain. */
TEST_CASE("Return route preserves original visits domain and legacy target", "[space-return-target]")
{
	SpaceReturnRig rig; auto &craft = rig.Miner(SPACE_PROBE_MODEL);
	REQUIRE(CmdEditSpaceOrder(DoCommandFlag::Execute, craft.index, craft.generation, SpaceOrderEdit::Insert, 1,
		SpaceOrderAction::Visit, SpaceOrderTargetKind::Body, 1, 0).Succeeded());
	const auto original_queue = craft.space_orders;
	auto route = GetSpaceReturnRoute(craft); REQUIRE(route);
	CHECK(route->visits_begin == 1); CHECK(route->handling_index == 2);
	CHECK(route->domain == SpaceOrderTargetKind::Structure); CHECK(route->specified == SpaceReturnRig::Target(*rig.core)); CHECK_FALSE(route->nearest);
	REQUIRE(CmdSetSpaceMiningPolicy(DoCommandFlag::Execute, craft.index, craft.generation, 0, SpaceMiningPolicy::ReturnNearest).Succeeded());
	route = GetSpaceReturnRoute(craft); REQUIRE(route); CHECK(route->nearest);
	CHECK(craft.space_orders.cursor == original_queue.cursor); CHECK(craft.space_orders.entries[1] == original_queue.entries[1]);
	REQUIRE(CmdEditSpaceOrder(DoCommandFlag::Execute, craft.index, craft.generation, SpaceOrderEdit::Replace, 1,
		SpaceOrderAction::Mine, SpaceOrderTargetKind::Body, 1, 0).Succeeded());
	CHECK_FALSE(GetSpaceReturnRoute(craft)); // A second Mine cannot be skipped to borrow its later Unload.
}

/** Actual mined holdings on each original role query whole native stock/ownership without transferring or paying for an unarrived return. */
TEST_CASE("Nearest module return skips incompatible full and foreign owners without mutations", "[space-return-target]")
{
	const uint16_t model = GENERATE(SPACE_PROBE_MODEL, SPACE_HEAVY_MINER_MODEL);
	SpaceReturnRig rig; auto &craft = rig.Miner(model); rig.MineAtLeast(craft, 2);
	REQUIRE(CmdSetSpaceMiningPolicy(DoCommandFlag::Execute, craft.index, craft.generation, 0, SpaceMiningPolicy::ReturnNearest).Succeeded());
	const auto fleet = ComputeSpaceVehicleDigest(), structures = ComputeSpaceStructureDigest(), power = ComputeUtilityPowerDigest();
	const auto field = GetAsteroidField(); const auto clock = TimerGameTick::counter;
	const auto selected = FindNearestEligibleSpaceReturn(craft, SpaceReturnPurpose::Cargo); REQUIRE(selected);
	CHECK(selected->target == SpaceReturnRig::Target(*rig.storage));
	CHECK(selected->quantity == craft.cargo.TotalCount());
	CHECK_FALSE(GetSpaceReturnCandidateAdmission(craft, SpaceReturnRig::Target(*rig.assembly), SpaceReturnPurpose::Cargo));
	CHECK_FALSE(ExecuteSpaceReturnArrival(craft, selected->target, SpaceReturnPurpose::Cargo));
	CHECK(ComputeSpaceVehicleDigest() == fleet); CHECK(ComputeSpaceStructureDigest() == structures); CHECK(ComputeUtilityPowerDigest() == power);
	CHECK(GetAsteroidField() == field); CHECK(TimerGameTick::counter == clock);
	const auto capacity = GetOrbitalModuleCargoCapacity(*rig.storage, craft.cargo_type);
	rig.Add(*rig.storage, CargoLabel{"NORE"}, capacity);
	const auto fallback = FindNearestEligibleSpaceReturn(craft, SpaceReturnPurpose::Cargo); REQUIRE(fallback);
	CHECK(fallback->target == SpaceReturnRig::Target(*rig.core));
	REQUIRE(HubCargoTransfer::Consume(rig.storage->module->stock[craft.cargo_type], capacity));
	auto &far_core = rig.Frame(SpaceBuildKind::Core, 1, 57);
	REQUIRE(Company::CanAllocateItem()); auto foreign = std::unique_ptr<Company>(Company::Create());
	{
		AutoRestoreBackup owner(rig.storage->state.owner, Owner{foreign->index});
		AutoRestoreBackup core_owner(rig.core->state.owner, Owner{foreign->index});
		REQUIRE(IsOrbitalModuleOperational(*rig.storage)); // Its actual adjacent Core belongs to the same real foreign company.
		CHECK(SpaceDistanceSquared(craft.position, GetSpaceStructureApproach(rig.storage->state)) < SpaceDistanceSquared(craft.position, GetSpaceStructureApproach(far_core.state)));
		const auto owned = FindNearestEligibleSpaceReturn(craft, SpaceReturnPurpose::Cargo); REQUIRE(owned);
		CHECK(owned->target == SpaceReturnRig::Target(far_core));
	}
	REQUIRE(CmdSetSpaceMiningPolicy(DoCommandFlag::Execute, craft.index, craft.generation, 0, SpaceMiningPolicy::LegacyReturn).Succeeded());
	const auto legacy = FindNearestEligibleSpaceReturn(craft, SpaceReturnPurpose::Cargo); REQUIRE(legacy);
	CHECK(legacy->target == SpaceReturnRig::Target(*rig.core)); // The nearer now-empty Storage does not replace an exact legacy directive.
	const auto stale = SpaceOrderTarget{SpaceOrderTargetKind::Structure, rig.core->index.base(), rig.core->generation + 1};
	CHECK_FALSE(GetSpaceReturnCandidateAdmission(craft, stale, SpaceReturnPurpose::Cargo));
}

/** Exact distances choose stable native IDs; retirement and numeric slot reuse cannot satisfy the original generation. */
TEST_CASE("Receiver ties and stale generations preserve original target meaning", "[space-return-target]")
{
	SpaceReturnRig rig; auto &craft = rig.Miner(SPACE_PROBE_MODEL);
	REQUIRE(craft.position.x % SPACE_Q16 == 0);
	const uint16_t column = static_cast<uint16_t>(craft.position.x / SPACE_Q16);
	REQUIRE(column >= 8); REQUIRE(column + 7 < Map::SizeX());
	// Horizontal symmetry keeps both complete footprints and south berths in
	// the legal top static strip; the miner's actual starting pose is unchanged.
	auto &first = rig.Frame(SpaceBuildKind::Core, column - 4, 10);
	auto &second = rig.Frame(SpaceBuildKind::Core, column + 4, 10);
	REQUIRE(CmdSetSpaceMiningPolicy(DoCommandFlag::Execute, craft.index, craft.generation, 0, SpaceMiningPolicy::ReturnNearest).Succeeded());
	CHECK(SpaceDistanceSquared(craft.position, GetSpaceStructureApproach(first.state)) == SpaceDistanceSquared(craft.position, GetSpaceStructureApproach(second.state)));
	const auto tie = FindNearestEligibleSpaceReturn(craft, SpaceReturnPurpose::Cargo); REQUIRE(tie);
	const auto old = SpaceReturnRig::Target(first); CHECK(tie->target == old);
	REQUIRE(CmdEditSpaceOrder(DoCommandFlag::Execute, craft.index, craft.generation, SpaceOrderEdit::Replace, 1,
		SpaceOrderAction::Unload, SpaceOrderTargetKind::Structure, first.index.base(), first.generation).Succeeded());
	REQUIRE(CmdSetSpaceMiningPolicy(DoCommandFlag::Execute, craft.index, craft.generation, 0, SpaceMiningPolicy::LegacyReturn).Succeeded());
	const auto x = first.state.x, y = first.state.y;
	REQUIRE(CmdRemoveOrbitalModule(DoCommandFlag::Execute, first.index, first.generation).Succeeded());
	auto &replacement = rig.Frame(SpaceBuildKind::Core, x, y);
	CHECK(replacement.index.base() == old.id);
	CHECK(replacement.generation != old.generation);
	CHECK_FALSE(GetSpaceReturnCandidateAdmission(craft, old, SpaceReturnPurpose::Cargo));
	CHECK_FALSE(FindNearestEligibleSpaceReturn(craft, SpaceReturnPurpose::Cargo));
}

/** Actual original Visit/arbitration must establish arrival before the new adapter can call the old physical exchange owner. */
TEST_CASE("Real module arrival commits original mined packets only after full readmission", "[space-return-target]")
{
	const uint16_t model = GENERATE(SPACE_PROBE_MODEL, SPACE_HEAVY_MINER_MODEL);
	SpaceReturnRig rig; auto &craft = rig.Miner(model); rig.MineAtLeast(craft, 2);
	const auto units = craft.cargo.TotalCount(); const auto source = craft.cargo.Packets()->front()->GetSource();
	rig.ReachAndHold(craft, *rig.core);
	const auto quota = craft.manifest ? craft.manifest->returns_remaining : 0;
	const auto money = Company::Get(craft.owner)->money;
	REQUIRE(ExecuteSpaceReturnArrival(craft, SpaceReturnRig::Target(*rig.core), SpaceReturnPurpose::Cargo));
	CHECK(craft.cargo.TotalCount() == 0); CHECK(rig.core->module->stock[craft.cargo_type].TotalCount() == units);
	CHECK(rig.core->module->stock[craft.cargo_type].Packets()->begin()->second.front()->GetSource() == source);
	CHECK(Company::Get(craft.owner)->money == money);
	if (model == SPACE_HEAVY_MINER_MODEL) CHECK(craft.manifest->returns_remaining + 1 == quota);
	CHECK(craft.space_orders.cursor == 0); // The arrival adapter grants no cursor completion before the continuation controller owns it.
}
