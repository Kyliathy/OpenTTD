/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file transport_tube_recovery.cpp Actual electrical loss, topology and paid source recovery with native cargo/stop ownership.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Group7
 */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "transport_tube_meter_fixture.h"
#include "../safeguards.h"

/** @param rig Actual owned route fixture. @return Identity of one real explicitly supplied17-unit packet, retained through electrical loss/recovery. */
static CargoPacketID LoadTubeRecoveryCargo(TubeMeterRig &rig)
{
	REQUIRE(CargoPacket::CanAllocateItem());
	auto *packet = CargoPacket::Create(StationID::Invalid(), uint16_t{17}, Source{Source::Invalid, SourceType::Industry});
	packet->UpdateLoadingTile(rig.depot); rig.capsule->cargo.Append(packet, VehicleCargoList::MoveToAction::Keep);
	rig.capsule->CargoChanged(); return packet->index;
}

/** @param rig Actual paid source/route. @param target Native world goal for the capsule front. Drive bounded real ticks without fixture relocation. */
static void DriveRecoveryCapsule(TubeMeterRig &rig, TileIndex target)
{
	uint frames = 0;
	// Observe the actual front on its requested physical segment; a failed
	// source/route cannot become a success through a direct tile assignment.
	while (rig.capsule->tile != target && frames < 6000) { rig.Run(); ++frames; }
	INFO(DescribeTrain(rig.capsule)); REQUIRE(frames < 6000);
}

/** Missing cash holds a moving native capsule; restoration resumes its admitted motor without touching the player stop or packet owners. */
TEST_CASE("Tube moving power wait resumes actual motion and preserves packet custody", "[transport-tube-power-recovery]")
{
	MockEnvironment::Instance(); TubeMeterRig rig;
	const auto packet = LoadTubeRecoveryCargo(rig); rig.Bind(); rig.Start();
	DriveRecoveryCapsule(rig, TileXYL(38, 43, MapLayer::Underground));
	const auto tile = rig.capsule->tile; const auto x = rig.capsule->x_pos, y = rig.capsule->y_pos;
	const auto consumed = GetUtilityPowerProducer(rig.source)->consumed_lifetime_j, invoice = GetTubePowerConnection(rig.source)->invoiced_money;
	Company::Get(_current_company)->money = Money{0}; rig.Run(10);
	CHECK(rig.capsule->tile == tile); CHECK(rig.capsule->x_pos == x); CHECK(rig.capsule->y_pos == y); CHECK(rig.capsule->cur_speed == 0);
	CHECK_FALSE(rig.capsule->vehstatus.Test(VehState::Stopped));
	CHECK(GetUtilityPowerProducer(rig.source)->consumed_lifetime_j == consumed); CHECK(GetTubePowerConnection(rig.source)->invoiced_money == invoice);
	CHECK(rig.capsule->cargo.Packets()->front()->index == packet); CHECK(rig.capsule->cargo.StoredCount() == 17);
	Company::Get(_current_company)->money = Money{10000};
	DriveRecoveryCapsule(rig, TileXYL(38, 45, MapLayer::Underground));
	CHECK(GetTubePowerVehicle(rig.capsule->index)->wait == TubePowerWait::None);
	CHECK(rig.capsule->cargo.Packets()->front()->index == packet);
}

/** An actual guideway split removes electrical reach without relocating a capsule; rebuilding the segment restores the same physical route. */
TEST_CASE("Tube electrical component split holds actual train and rebuild restores service", "[transport-tube-power-recovery]")
{
	MockEnvironment::Instance(); TubeMeterRig rig;
	const auto packet = LoadTubeRecoveryCargo(rig); rig.Bind(); rig.Start();
	DriveRecoveryCapsule(rig, TileXYL(38, 44, MapLayer::Underground));
	const auto removed = TileXYL(38, 41, MapLayer::Underground);
	REQUIRE(CmdRemoveSingleRail(DoCommandFlag::Execute, removed, Track::Y).Succeeded());
	const auto tile = rig.capsule->tile; const auto consumed = GetUtilityPowerProducer(rig.source)->consumed_lifetime_j;
	rig.Run(20);
	CHECK(rig.capsule->tile == tile); CHECK(rig.capsule->cur_speed == 0); CHECK_FALSE(rig.capsule->vehstatus.Test(VehState::Stopped));
	CHECK(GetTubePowerVehicle(rig.capsule->index)->wait == TubePowerWait::Topology);
	CHECK(GetUtilityPowerProducer(rig.source)->consumed_lifetime_j == consumed); CHECK(rig.capsule->cargo.Packets()->front()->index == packet);
	REQUIRE(CmdBuildSingleRail(DoCommandFlag::Execute, removed, rig.tube, Track::Y, false).Succeeded());
	DriveRecoveryCapsule(rig, TileXYL(38, 46, MapLayer::Underground));
	CHECK(GetTubePowerVehicle(rig.capsule->index)->wait == TubePowerWait::None); CHECK(rig.capsule->cargo.Packets()->front()->index == packet);
}

/** A player stop remains authoritative through supply loss/restoration; reconnecting cannot silently issue a start command. */
TEST_CASE("Tube restored service preserves manual depot stop and queued unpowered custody", "[transport-tube-power-recovery]")
{
	TubeMeterRig rig; const auto packet = LoadTubeRecoveryCargo(rig); rig.Bind();
	Company::Get(_current_company)->money = Money{0}; rig.Run(20);
	Company::Get(_current_company)->money = Money{10000}; rig.Run(20);
	CHECK(rig.capsule->IsStoppedInDepot()); CHECK(rig.capsule->tile == rig.depot);
	CHECK(rig.capsule->cargo.Packets()->front()->index == packet); CHECK(GetUtilityPowerProducer(rig.source)->consumed_lifetime_j == 0);
	rig.Start(); Company::Get(_current_company)->money = Money{0}; rig.Run(20);
	CHECK_FALSE(rig.capsule->vehstatus.Test(VehState::Stopped)); CHECK(rig.capsule->tile == rig.depot);
	Company::Get(_current_company)->money = Money{10000}; DriveRecoveryCapsule(rig, TileXYL(38, 42, MapLayer::Underground));
	CHECK(rig.capsule->cargo.Packets()->front()->index == packet);
}

/** Eligible physical layout changes expire only unused current offers, preserving paid/consumed source history and invalidating stale mutation tickets. */
TEST_CASE("Tube physical topology change revokes unused original reservation without refund", "[transport-tube-power-recovery]")
{
	TubeMeterRig rig; rig.Bind(); rig.Start();
	++TimerGameTick::counter; PrepareTubePowerTick();
	REQUIRE(GetUtilityAllocation(rig.consumer)->granted_energy_j == 270);
	const auto old = PrepareUtilityMutation(UtilityMutationKind::Consume, {rig.consumer, GetUtilityPowerInterval()}, 270);
	REQUIRE(old.error == UtilityPowerError::None);
	const auto generated = GetUtilityPowerProducer(rig.source)->generated_lifetime_j, invoice = GetTubePowerConnection(rig.source)->invoiced_money;
	REQUIRE(CmdRemoveSingleRail(DoCommandFlag::Execute, TileXYL(38, 41, MapLayer::Underground), Track::Y).Succeeded());
	CHECK(GetUtilityAllocation(rig.consumer)->granted_energy_j == 0);
	CHECK(GetUtilityPowerProducer(rig.source)->generated_lifetime_j == generated); CHECK(GetUtilityPowerProducer(rig.source)->consumed_lifetime_j == 0);
	CHECK(GetTubePowerConnection(rig.source)->invoiced_money == invoice);
	CapsuleDepotService work(*rig.capsule);
	CHECK(CommitUtilityMutation(old, &work).error == UtilityPowerError::StaleMutation);
}

/** Native depot service remains held during loss, then resumes one real paid opportunity while preserving the player stop. */
TEST_CASE("Tube depot power restoration services the actual capsule without starting it", "[transport-tube-power-recovery]")
{
	TubeMeterRig rig; rig.Bind(); rig.capsule->reliability = 100;
	Company::Get(_current_company)->money = Money{0}; rig.Run(20);
	CHECK(rig.capsule->reliability == 100); CHECK(rig.capsule->IsStoppedInDepot());
	CHECK(GetUtilityPowerProducer(rig.source)->consumed_lifetime_j == 0);
	Company::Get(_current_company)->money = Money{10000}; rig.Run();
	CHECK(rig.capsule->reliability == rig.capsule->GetEngine()->reliability); CHECK(rig.capsule->IsStoppedInDepot());
	CHECK(GetUtilityPowerProducer(rig.source)->consumed_lifetime_j == 540);
}

/** Routine source allocation and read-only circuit queries do not invalidate the actual native route cache. */
TEST_CASE("Tube routine allocation leaves native physical path topology stable", "[transport-tube-power-recovery]")
{
	TubeMeterRig rig; rig.Bind();
	const auto revision = CSegmentCostCacheBase::s_rail_change_counter;
	rig.Run(100);
	// Repeated ordinary readings exercise a derived circuit; only real
	// layout/capability changes may notify the native physical pathfinder.
	for (uint view = 0; view < 20; ++view) CHECK(IsTubePowerConnected(rig.source, *rig.capsule));
	CHECK(CSegmentCostCacheBase::s_rail_change_counter == revision);
}

/** Actual company takeover retires physical service/bindings before identity changes while preserving shared consumed history and any unpaid final fraction. */
TEST_CASE("Tube native company takeover retains invoiced use and retires old power identity", "[transport-tube-power-recovery]")
{
	Company *buyer = nullptr;
	{
		TubeMeterRig rig; rig.Bind(); rig.capsule->reliability = 100; rig.Run();
		const auto consumed = GetUtilityPowerProducer(rig.source)->consumed_lifetime_j;
		const auto invoice = GetTubePowerConnection(rig.source)->invoiced_money;
		REQUIRE(Company::CanAllocateItem()); buyer = Company::Create();
		AutoRestoreBackup local(_local_company, CompanyID::Invalid());
		ChangeOwnershipOfCompanyItems(_current_company, buyer->index);
		CHECK_FALSE(GetTubePowerConnection(rig.source)->active);
		CHECK(GetTubePowerConnection(rig.source)->owner == GetUtilityPowerProducer(rig.source)->owner);
		CHECK(GetTubePowerVehicle(rig.capsule->index) == nullptr);
		CHECK(GetUtilityPowerProducer(rig.source)->consumed_lifetime_j == consumed);
		CHECK(GetTubePowerConnection(rig.source)->invoiced_money >= invoice);
		CHECK(ValidateTubePowerConnections(CaptureTubePowerConnections(), GetCurrentMultiLayerProfileSelection()) == TubePowerError::None);
		CHECK(ValidateTubePowerVehicles(CaptureTubePowerVehicles()) == TubePowerError::None);
	} // Real vehicles/facilities must be destroyed while their takeover company still exists.
	delete buyer;
}

/** A new connected guideway alternative restores both electrical reach and native routing after the original segment is physically removed. */
TEST_CASE("Tube actual alternative route recovers topology after a blocked primary guideway", "[transport-tube-power-recovery]")
{
	MockEnvironment::Instance(); TubeMeterRig rig; rig.Bind();
	const auto entry = TileXYL(38, 42, MapLayer::Underground), exit = TileXYL(38, 46, MapLayer::Underground);
	// The detour uses four native corners separated by distinct guideway
	// cells and a straight run, retaining the same owned layer-local domain.
	rig.BuildTrack(entry, rig.tube, Track::Upper);
	rig.BuildTrack(TileXYL(37, 42, MapLayer::Underground), rig.tube, Track::Lower);
	for (int y = 43; y <= 45; ++y) rig.BuildTrack(TileXYL(37, y, MapLayer::Underground), rig.tube, Track::Y);
	rig.BuildTrack(TileXYL(37, 46, MapLayer::Underground), rig.tube, Track::Left);
	rig.BuildTrack(exit, rig.tube, Track::Right);
	REQUIRE(CmdRemoveSingleRail(DoCommandFlag::Execute, TileXYL(38, 44, MapLayer::Underground), Track::Y).Succeeded());
	rig.Start(); DriveRecoveryCapsule(rig, TileXYL(37, 44, MapLayer::Underground));
	CHECK(IsTubePowerConnected(rig.source, *rig.capsule));
	DriveRecoveryCapsule(rig, TileXYL(38, 47, MapLayer::Underground));
	CHECK(GetTubePowerVehicle(rig.capsule->index)->wait == TubePowerWait::None);
}

/** A genuinely retired source gets a fresh paid identity; reconnection preserves native custody/carry without reviving any old offer. */
TEST_CASE("Tube retired source recovery uses a new paid identity and moves demand carry once", "[transport-tube-power-recovery]")
{
	MockEnvironment::Instance(); TubeMeterRig rig;
	const auto packet = LoadTubeRecoveryCargo(rig); rig.Bind(); rig.Start();
	DriveRecoveryCapsule(rig, TileXYL(38, 43, MapLayer::Underground));
	const auto original_source = rig.source;
	const auto original_consumer = rig.consumer;
	const auto used = GetUtilityPowerProducer(original_source)->consumed_lifetime_j;
	const auto carry = GetTubePowerVehicle(rig.capsule->index)->remainder_mj;
	REQUIRE(CloseTubeExternalPower(original_source) == TubePowerError::None); rig.Run();
	CHECK(rig.capsule->cur_speed == 0);
	const auto replacement = ConnectTubeExternalPower(rig.portal, rig.depot, rig.path);
	REQUIRE(replacement.error == TubePowerError::None); CHECK(replacement.producer != original_source);
	const auto ledger = ComputeUtilityPowerDigest(), bodies = ComputeTubePowerVehicleDigest();
	const auto quote = ReconnectTubePowerVehicle(rig.capsule->index, replacement.producer, false);
	REQUIRE(quote.has_value()); CHECK(ComputeUtilityPowerDigest() == ledger); CHECK(ComputeTubePowerVehicleDigest() == bodies);
	REQUIRE(ReconnectTubePowerVehicle(rig.capsule->index, replacement.producer) == quote);
	CHECK(GetTubePowerVehicle(rig.capsule->index)->remainder_mj == carry);
	CHECK(GetUtilityPowerProducer(original_source)->consumed_lifetime_j == used);
	CHECK(GetUtilityAllocation(original_consumer)->granted_energy_j == 0);
	rig.source = replacement.producer; rig.consumer = *quote;
	DriveRecoveryCapsule(rig, TileXYL(38, 45, MapLayer::Underground));
	CHECK(rig.capsule->cargo.Packets()->front()->index == packet);
	CHECK(ValidateTubePowerConnections(CaptureTubePowerConnections(), GetCurrentMultiLayerProfileSelection()) == TubePowerError::None);
	CHECK(ValidateTubePowerVehicles(CaptureTubePowerVehicles()) == TubePowerError::None);
}

/** Actual portal sealing retires an attached service before its revision changes; an insolvent owner retains historical fractional debt without usable energy. */
TEST_CASE("Tube actual portal loss retains unpaid history and held native service", "[transport-tube-power-recovery]")
{
	TubeMeterRig rig; rig.Bind(); rig.capsule->reliability = 100; rig.Run();
	REQUIRE(GetTubePowerConnection(rig.source)->usage_remainder_j == 540);
	const auto used = GetUtilityPowerProducer(rig.source)->consumed_lifetime_j;
	const auto invoice = GetTubePowerConnection(rig.source)->invoiced_money;
	Company::Get(_current_company)->money = Money{0};
	const auto physical = ComputeTubePowerDigest(), ledger = ComputeUtilityPowerDigest();
	REQUIRE(ChangeLayerPortalConnection(rig.portal, _current_company, false, false) == LayerPortalWorkError::None);
	CHECK(ComputeTubePowerDigest() == physical); CHECK(ComputeUtilityPowerDigest() == ledger);
	REQUIRE(ChangeLayerPortalConnection(rig.portal, _current_company, false, true) == LayerPortalWorkError::None);
	CHECK_FALSE(GetTubePowerConnection(rig.source)->active); CHECK_FALSE(GetUtilityPowerProducer(rig.source)->active);
	CHECK(GetTubePowerConnection(rig.source)->usage_remainder_j == 540);
	CHECK(GetTubePowerConnection(rig.source)->invoiced_money == invoice);
	CHECK(GetUtilityPowerProducer(rig.source)->consumed_lifetime_j == used);
	rig.capsule->reliability = 100; rig.Run();
	CHECK(rig.capsule->IsStoppedInDepot()); CHECK(rig.capsule->reliability == 100);
	CHECK(GetTubePowerVehicle(rig.capsule->index)->wait == TubePowerWait::Topology);
	CHECK(ValidateTubePowerConnections(CaptureTubePowerConnections(), GetCurrentMultiLayerProfileSelection()) == TubePowerError::None);
	CHECK(ValidateTubePowerVehicles(CaptureTubePowerVehicles()) == TubePowerError::None);
}
