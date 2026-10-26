/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file space_return_surface.cpp Real mined freight and native shell/recovery admission for nearest Surface returns.
 * @see docs/zz-reach2/upgrades/2026-10/r2sff1-space-fixes-features-1.md Group19.
 * Center metadata, completed shell receipts and legacy15's initial allowance are declared unit inputs; no paid procurement is claimed.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "space_return_fixture.h"
#include "../space_pod.h"
#include "../station_base.h"
#include "../station_map.h"
#include "../water_map.h"
#include "../safeguards.h"

/** Original facility/shell owners around the actual mining fixture; declared inputs never fabricate freight or arrival. */
class SurfaceReturnRig : public SpaceReturnRig {
public:
	std::vector<std::unique_ptr<Station>> centers; ///< Sole isolated Surface participants, removed after native hulls and shells.
	/** Start empty native recovery families before declared facility installation. */
	SurfaceReturnRig() { ResetLaunchCenters(); ResetSpacePods(); ResetSpaceDescents(); }
	/** Dispose actual isolated owners in custody order before the parent restores the original world. */
	~SurfaceReturnRig()
	{
		// This derived rig tears hulls down before its parent's cleanup can run.
		// Dispose original freight through its own containers first; bulk Vehicle
		// cleanup intentionally skips packet deletion during whole-world teardown.
		for (auto *vehicle : Vehicle::Iterate()) vehicle->cargo.Truncate();
		_vehicle_pool.CleanPool(); ResetVehicleHash(); _space_pod_pool.CleanPool(); centers.clear();
		ResetSpacePods(); ResetSpaceDescents(); ResetLaunchCenters();
	}
	/** @param x Declared Surface anchor column. @param y Declared Surface anchor row. @return Original installed center with real native tile ownership. */
	Station &Center(uint16_t x, uint16_t y)
	{
		const auto tile = TileXYL(x, y, MapLayer::Surface); REQUIRE(Station::CanAllocateItem());
		auto center = std::unique_ptr<Station>(Station::Create(tile)); center->owner = _current_company;
		center->facilities.Set(StationFacility::LaunchCenter);
		// This is explicit facility metadata, as in descent_custody.cpp. Each
		// original footprint cell belongs to the actual isolated Station owner.
		for (const auto cell : *GetLaunchCenterFootprint(tile, 0)) {
			MakeStation(cell, center->owner, center->index, StationType::LaunchCenter, 0); center->spread.Add(cell);
		}
		MakeRiver(TileAddXY(tile, 0, -1), 0); InstallNewLaunchCenter(*center, tile, 0);
		centers.push_back(std::move(center)); return *centers.back();
	}
	/** @param center Actual Surface owner. @return Exact typed center incarnation. */
	static SpaceOrderTarget Target(const Station &center)
	{
		const auto ref = GetLaunchCenterReference(center); return {SpaceOrderTargetKind::LaunchCenter, ref.station.base(), ref.generation};
	}
	/** @param craft Actual miner. @param center Original explicit return destination. Set only checked queue instructions and policy. */
	void SurfaceRoute(SpaceVehicle &craft, const Station &center)
	{
		this->Route(craft, 0, *core);
		const auto target = Target(center);
		REQUIRE(CmdEditSpaceOrder(DoCommandFlag::Execute, craft.index, craft.generation, SpaceOrderEdit::Replace, 1,
			SpaceOrderAction::Unload, target.kind, target.id, target.generation).Succeeded());
		REQUIRE(CmdSetSpaceMiningPolicy(DoCommandFlag::Execute, craft.index, craft.generation, 0, SpaceMiningPolicy::ReturnNearest).Succeeded());
	}
	/** @param craft Actual carrying hull. @param origin Declared shell's real original facility. Install one bounded complete shell receipt under its original native owner. */
	void Shell(SpaceVehicle &craft, const Station &origin)
	{
		REQUIRE(craft.manifest); REQUIRE(craft.manifest->pods.empty());
		SpacePodRecord row; row.ref = {SpacePodID{0}, 1};
		auto &s = row.state; s.owner = craft.owner; s.custody = SpacePodCustody::Craft;
		s.craft = GetSpaceCraftReference(craft); s.origin = GetLaunchCenterReference(origin);
		s.surface_tile = origin.launch_center->tile; s.position = craft.position;
		s.content_version = MULTILAYER_SPACE_CONTENT_VERSION; s.capital_paid = GetSpaceConstructionDefinition(SpaceBuildKind::Pod)->capital;
		REQUIRE(InstallLoadedSpacePods(std::span{&row, 1}, 2)); craft.manifest->pods.push_back(row.ref);
	}
	/** @param craft Actual running miner. @param center Original destination. Earn exact arrival through the existing Visit action, then restore its pending mining/return route. */
	void ReachCenter(SpaceVehicle &craft, const Station &center)
	{
		const auto target = Target(center);
		REQUIRE(CmdEditSpaceOrder(DoCommandFlag::Execute, craft.index, craft.generation, SpaceOrderEdit::Clear, 0,
			SpaceOrderAction::Visit, target.kind, target.id, target.generation).Succeeded());
		REQUIRE(CmdEditSpaceOrder(DoCommandFlag::Execute, craft.index, craft.generation, SpaceOrderEdit::Insert, 0,
			SpaceOrderAction::Visit, target.kind, target.id, target.generation).Succeeded());
		// Exact native navigation earns arrival without assigning pose, phase or
		// an artificial command quote from a moved copy of the hull.
		for (uint tick = 0; craft.space_orders.cursor < craft.space_orders.entries.size(); ++tick) { REQUIRE(tick < 5000); this->Step(); }
		this->SurfaceRoute(craft, center);
	}
};

/** Neither a near foreign/full receiver nor missing custody can hide the sole farther legitimate recovery owner. */
TEST_CASE("Surface return queries retain actual shells allowance and earned cargo", "[space-return-surface]")
{
	const uint16_t model = GENERATE(SPACE_PROBE_MODEL, SPACE_HEAVY_MINER_MODEL);
	SurfaceReturnRig rig; auto &craft = rig.Miner(model); rig.MineAtLeast(craft, 2);
	if (!craft.manifest) {
		// The legacy unit has no paid commissioning receipt: its initial service
		// allowance is explicitly declared, while all offered freight was mined.
		craft.manifest = std::make_unique<SpaceManifest>(); craft.manifest->returns_remaining = 2;
	}
	std::array<Station *, 3> centers{&rig.Center(4, 4), &rig.Center(24, 24), &rig.Center(44, 4)};
	/** @param a Declared center. @param b Declared center. @return Actual geometric order used only to arrange near-invalid/far-valid fixture inputs. */
	std::ranges::sort(centers, [&](const Station *a, const Station *b) {
		return SpaceDistanceSquared(craft.position, *GetLaunchCenterPlaneReference(a->launch_center->tile)) <
			SpaceDistanceSquared(craft.position, *GetLaunchCenterPlaneReference(b->launch_center->tile));
	});
	rig.SurfaceRoute(craft, *centers.back());
	CHECK_FALSE(FindNearestEligibleSpaceReturn(craft, SpaceReturnPurpose::Cargo)); // No fabricated shell for either role.
	rig.Shell(craft, *centers.back());
	REQUIRE(Company::CanAllocateItem()); auto foreign = std::unique_ptr<Company>(Company::Create());
	AutoRestoreBackup owner(centers[0]->owner, Owner{foreign->index});
	auto &full = centers[1]->goods[craft.cargo_type].GetOrCreateData().cargo;
	const auto capacity = GetLaunchStockCapacity(*centers[1], craft.cargo_type); REQUIRE(capacity <= UINT16_MAX);
	REQUIRE(CargoPacket::CanAllocateItem()); full.Append(CargoPacket::Create(static_cast<uint16_t>(capacity), uint16_t{0}, StationID::Invalid(), centers[1]->xy, Money{0}), StationID::Invalid());
	const auto fleet = ComputeSpaceVehicleDigest(), pods = ComputeSpacePodDigest(); const auto cash = Company::Get(craft.owner)->money;
	const auto allowance = craft.manifest->returns_remaining, pending = craft.manifest->pending_returns;
	const auto candidate = FindNearestEligibleSpaceReturn(craft, SpaceReturnPurpose::Cargo); REQUIRE(candidate);
	CHECK(candidate->target == SurfaceReturnRig::Target(*centers.back())); CHECK(candidate->quantity == craft.cargo.TotalCount());
	REQUIRE_FALSE(IsInsideLaunchCenterDropZone(craft.position, candidate->anchor));
	CHECK_FALSE(ExecuteSpaceReturnArrival(craft, candidate->target, SpaceReturnPurpose::Cargo));
	CHECK(ComputeSpaceVehicleDigest() == fleet); CHECK(ComputeSpacePodDigest() == pods); CHECK(Company::Get(craft.owner)->money == cash);
	CHECK(craft.manifest->returns_remaining == allowance); CHECK(craft.manifest->pending_returns == pending);
	{
		// Declared counter boundary isolates exhausted unreserved allowance from
		// absent shells; no fake descent job or spent cargo is created here.
		AutoRestoreBackup reserved(craft.manifest->pending_returns, craft.manifest->returns_remaining);
		CHECK_FALSE(FindNearestEligibleSpaceReturn(craft, SpaceReturnPurpose::Cargo));
		CHECK_FALSE(FindNearestEligibleSpaceReturn(craft, SpaceReturnPurpose::Service));
	}
	REQUIRE(HubCargoTransfer::Consume(full, capacity));
}

/** Native Visit and actual descent own movement, packet custody and eventual allowance debit; the selector performs none of them. */
TEST_CASE("Surface arrival releases and settles original mined cargo once", "[space-return-surface]")
{
	SurfaceReturnRig rig; auto &craft = rig.Miner(); rig.MineAtLeast(craft, 2);
	auto &center = rig.Center(24, 24); rig.Shell(craft, center);
	const auto target = SurfaceReturnRig::Target(center);
	rig.ReachCenter(craft, center);
	const auto units = craft.cargo.TotalCount(); const auto quota = craft.manifest->returns_remaining;
	const auto source = craft.cargo.Packets()->front()->GetSource(); const auto cash = Company::Get(craft.owner)->money;
	REQUIRE(ExecuteSpaceReturnArrival(craft, target, SpaceReturnPurpose::Cargo));
	CHECK(craft.cargo.TotalCount() == 0); CHECK(craft.manifest->pending_returns == 1); CHECK(craft.manifest->returns_remaining == quota);
	CHECK(center.goods[craft.cargo_type].TotalCount() == 0); CHECK(craft.manifest->pods.empty());
	// Only the original descent countdown may settle the original packet owner
	// and its pending return claim. The arrival adapter never grants recovery.
	for (uint tick = 0; tick < GetMultiLayerSpaceBudget().descent_ticks; ++tick) TickSpaceDescents();
	CHECK(center.goods[craft.cargo_type].TotalCount() == units); CHECK(craft.manifest->returns_remaining + 1 == quota);
	CHECK(craft.manifest->pending_returns == 0); CHECK(Company::Get(craft.owner)->money == cash);
	CHECK(center.goods[craft.cargo_type].GetData().cargo.Packets()->begin()->second.front()->GetSource() == source);
	const auto empty = FindNearestEligibleSpaceReturn(craft, SpaceReturnPurpose::Cargo); REQUIRE(empty); CHECK(empty->quantity == 0);
	CHECK(ExecuteSpaceReturnArrival(craft, target, SpaceReturnPurpose::Cargo)); CHECK(craft.manifest->returns_remaining + 1 == quota);
}

/** A Surface service destination cannot lend station stock to the hull's independent original onboard work owner. */
TEST_CASE("Surface service uses actual onboard parts after ordinary arrival", "[space-return-surface]")
{
	SurfaceReturnRig rig; auto &craft = rig.Miner(); const auto allowance = craft.manifest->returns_remaining;
	rig.MineAtLeast(craft, allowance + 1); rig.ReachAndHold(craft, *rig.core);
	// Spend the original heavy allowance through real positive packet handling;
	// no synthetic service need, pending descent or completed work is assigned.
	for (uint count = 0; count < allowance; ++count) {
		REQUIRE(CmdMoveOrbitalFreight(DoCommandFlag::Execute, rig.core->index, rig.core->generation,
			craft.index, craft.generation, craft.cargo_type, 1, false).Succeeded());
	}
	REQUIRE(CmdStartStopVehicle(DoCommandFlag::Execute, craft.index, true).Succeeded());
	auto &center = rig.Center(24, 24); rig.SurfaceRoute(craft, center);
	const auto target = SurfaceReturnRig::Target(center); const auto tuning = GetSpaceServiceTuning(SPACE_HEAVY_MINER_MODEL); REQUIRE(tuning);
	CHECK_FALSE(GetSpaceReturnCandidateAdmission(craft, target, SpaceReturnPurpose::Service));
	// Parts are explicit unit input in the actual manifest compartments. An
	// unrelated facility cannot satisfy or consume this immutable onboard bill.
	for (uint slot = 0; slot < tuning->inputs.size(); ++slot) {
		REQUIRE(CargoPacket::CanAllocateItem());
		craft.manifest->supplies[slot + 1].Append(CargoPacket::Create(uint16_t{tuning->inputs[slot]}, uint16_t{0}, StationID::Invalid(), center.xy, Money{0}), StationID::Invalid());
	}
	const auto fleet = ComputeSpaceVehicleDigest(); const auto completed = craft.manifest->services_completed;
	REQUIRE(GetSpaceReturnCandidateAdmission(craft, target, SpaceReturnPurpose::Service));
	CHECK_FALSE(ExecuteSpaceReturnArrival(craft, target, SpaceReturnPurpose::Service)); CHECK(ComputeSpaceVehicleDigest() == fleet);
	rig.ReachCenter(craft, center); rig.Step(); REQUIRE(IsSpaceMiningServiceWait(craft.space_orders));
	REQUIRE(ExecuteSpaceReturnArrival(craft, target, SpaceReturnPurpose::Service));
	CHECK(craft.manifest->service_ticks == tuning->ticks); CHECK(craft.manifest->returns_remaining == 0);
	// Original manifest work retains and consumes the same parts; the selection
	// adapter cannot shorten its countdown or grant the renewal early.
	for (uint tick = 0; tick < tuning->ticks; ++tick) TickSpaceManifests();
	CHECK(craft.manifest->returns_remaining == tuning->returns); CHECK(craft.manifest->services_completed == completed + 1);
	CHECK(craft.manifest->supplies[1].TotalCount() == 0); CHECK(craft.manifest->supplies[2].TotalCount() == 0);
	CHECK(craft.space_orders.cursor == 0);
}
