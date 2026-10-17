/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file descent_custody.cpp Isolated full-storage and retired-center descent boundaries, separate from paid-world checkpoints.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs9-launch-centers-and-rockets.md Group13.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group9CBB; declared40t heavy recovery boundaries.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../launch_center.h"
#include "../launch_center_cmd.h"
#include "../space_pod.h"
#include "../station_base.h"
#include "../station_map.h"
#include "../water_map.h"
#include "../company_base.h"
#include "../hub_cargo.h"
#include "excavation_fixture.h"
#include "mock_environment.h"
#include "../safeguards.h"

/** Explicit isolated packet/pod quantities; this rig never writes a gameplay save or claims material-paid production. */
class DescentCustodyBoundaryRig {
	ExcavationFixture world;
public:
	std::unique_ptr<Station> center;
	SpacePodRef shell;
	CargoType cargo = INVALID_CARGO;
	/** Construct isolated native packet containers and an absent sender's recovery at a two-tick boundary.
	 * @param heavy Whether this declared unit scenario uses the immutable21 receipt and40t instead of the original unknown ten-unit job.
	 */
	explicit DescentCustodyBoundaryRig(bool heavy = false)
	{
		ResetLaunchCenters(); ResetSpacePods(); ResetSpaceDescents(); cargo = GetCargoTypeByLabel(CargoLabel{"NORE"}); REQUIRE(cargo != INVALID_CARGO);
		const auto tile = TileXYL(20, 20, MapLayer::Surface); REQUIRE(Station::CanAllocateItem()); center.reset(Station::Create(tile));
		center->owner = _current_company; center->facilities.Set(StationFacility::LaunchCenter);
		// Build only isolated ownership/tile metadata; real placement and material-paid gameplay remain the native fixture's responsibility.
		for (const auto cell : *GetLaunchCenterFootprint(tile, 0)) { MakeStation(cell, center->owner, center->index, StationType::LaunchCenter, 0); center->spread.Add(cell); }
		MakeRiver(TileAddXY(tile, 0, -1), 0); InstallNewLaunchCenter(*center, tile, 0);
		REQUIRE(SpacePod::CanAllocateItem()); auto *pod = SpacePod::Create(); pod->generation = 1; shell = GetSpacePodReference(*pod);
		pod->state.owner = _current_company; pod->state.custody = SpacePodCustody::Descent; pod->state.position = *GetLaunchCenterPlaneReference(tile);
		pod->descent = std::make_unique<SpaceDescentJob>(); auto &job = *pod->descent;
		job.id = 1; job.center = GetLaunchCenterReference(*center); job.release_point = pod->state.position; job.cargo = cargo; job.quantity = heavy ? 40 : 10; job.remaining_ticks = 2;
		if (heavy) { job.sender_model = 21; job.unit_weight = 16; job.freight_mass = 640; }
		job.age_ticks = Ticks::CARGO_AGING_TICKS - 1; REQUIRE(CargoPacket::CanAllocateItem());
		job.packets.Append(CargoPacket::Create(job.quantity, uint16_t{7}, StationID::Invalid(), tile, Money{100}), StationID::Invalid());
	}
	/** Retire only isolated objects and packet quantities before the focused world's company/map teardown. */
	~DescentCustodyBoundaryRig() { _space_pod_pool.CleanPool(); center.reset(); ResetSpacePods(); ResetSpaceDescents(); ResetLaunchCenters(); }
};

/** A full target retains the sole cargo owner, canonical age/provenance and reservation until actual inventory headroom exists. */
TEST_CASE("Full descent storage ages retained packets and completes once after capacity returns", "[descent-custody-contract]")
{
	MockEnvironment::Instance(); const bool heavy = GENERATE(false, true); DescentCustodyBoundaryRig rig(heavy); auto *pod = ResolveSpacePod(rig.shell); REQUIRE(pod != nullptr); const uint quantity = heavy ? 40 : 10;
	auto &stock = rig.center->goods[rig.cargo].GetOrCreateData().cargo; const auto cap = GetLaunchStockCapacity(*rig.center, rig.cargo);
	REQUIRE(CargoPacket::CanAllocateItem()); stock.Append(CargoPacket::Create(static_cast<uint16_t>(cap), uint16_t{0}, StationID::Invalid(), rig.center->xy, Money{0}), StationID::Invalid());
	const auto cash = Company::Get(_current_company)->money; TickSpaceDescents(); TickSpaceDescents();
	REQUIRE(ResolveSpacePod(rig.shell) != nullptr); CHECK(pod->descent->remaining_ticks == 0); CHECK(pod->descent->blocked == SpaceDescentBlock::StorageFull);
	CHECK(pod->descent->packets.TotalCount() == quantity); CHECK(GetIncomingSpaceDescent(GetLaunchCenterReference(*rig.center), rig.cargo) == quantity);
	const auto records = CaptureSpaceDescents(); REQUIRE(records.size() == 1); auto *packet = CargoPacket::Get(records.front().packets.front().first);
	CHECK(packet->GetPeriodsInTransit() == 8); CHECK(packet->GetFeederShare() == 100); CHECK(packet->GetSourceTile() == rig.center->xy);
	TickSpaceDescents(); CHECK(stock.TotalCount() == cap); CHECK(pod->descent->packets.TotalCount() == quantity);
	REQUIRE(HubCargoTransfer::Consume(stock, cap)); TickSpaceDescents(); CHECK(ResolveSpacePod(rig.shell) == nullptr); CHECK(stock.TotalCount() == quantity);
	CHECK(GetIncomingSpaceDescent(GetLaunchCenterReference(*rig.center), rig.cargo) == 0); TickSpaceDescents(); CHECK(stock.TotalCount() == quantity); CHECK(Company::Get(_current_company)->money == cash);
}

/** An isolated retired identity cannot inherit recovery even when its native Station pool slot is reused; an explicit authorized command resumes the existing owner. */
TEST_CASE("Retired center generation retains descent until explicit retarget and never duplicates cargo", "[descent-custody-contract]")
{
	MockEnvironment::Instance(); const bool heavy = GENERATE(false, true); DescentCustodyBoundaryRig rig(heavy); const uint quantity = heavy ? 40 : 10; const auto old = GetLaunchCenterReference(*rig.center); const auto tile = rig.center->launch_center->tile;
	ForgetLaunchCenter(*rig.center); TickSpaceDescents(); auto *pod = ResolveSpacePod(rig.shell); REQUIRE(pod != nullptr);
	CHECK(pod->descent->remaining_ticks == 2); CHECK(pod->descent->blocked == SpaceDescentBlock::TargetLost); CHECK(pod->descent->packets.TotalCount() == quantity);
	InstallNewLaunchCenter(*rig.center, tile, 0); const auto current = GetLaunchCenterReference(*rig.center); CHECK(current.station == old.station); CHECK(current.generation > old.generation);
	TickSpaceDescents(); CHECK(pod->descent->remaining_ticks == 2); CHECK(rig.center->goods[rig.cargo].TotalCount() == 0);
	CHECK_FALSE(CanRetargetSpaceDescent(rig.shell, old)); CHECK(CmdRetargetSpaceDescent({}, rig.shell.pod, rig.shell.generation, old.station, old.generation).Failed());
	REQUIRE(CmdRetargetSpaceDescent({DoCommandFlag::Execute}, rig.shell.pod, rig.shell.generation, current.station, current.generation).Succeeded());
	CHECK(pod->descent->blocked == SpaceDescentBlock::None); TickSpaceDescents(); CHECK(pod->descent->remaining_ticks == 1); TickSpaceDescents();
	CHECK(ResolveSpacePod(rig.shell) == nullptr); CHECK(rig.center->goods[rig.cargo].TotalCount() == quantity); TickSpaceDescents(); CHECK(rig.center->goods[rig.cargo].TotalCount() == quantity);
}

/** A foreign receiver never advances or receives the declared heavy load, and restored ownership still requires explicit re-admission of the retained job. */
TEST_CASE("Foreign recovery retains every heavy packet until explicit owned retarget", "[descent-custody-contract]")
{
	MockEnvironment::Instance(); DescentCustodyBoundaryRig rig(true); const auto center = GetLaunchCenterReference(*rig.center); auto *pod = ResolveSpacePod(rig.shell); REQUIRE(pod != nullptr);
	const auto original_owner = rig.center->owner; rig.center->owner = CompanyID{1}; TickSpaceDescents();
	CHECK(pod->descent->remaining_ticks == 2); CHECK(pod->descent->blocked == SpaceDescentBlock::TargetLost); CHECK(pod->descent->packets.TotalCount() == 40); CHECK(rig.center->goods[rig.cargo].TotalCount() == 0);
	CHECK_FALSE(CanRetargetSpaceDescent(rig.shell, center)); rig.center->owner = original_owner; TickSpaceDescents(); CHECK(pod->descent->remaining_ticks == 2);
	REQUIRE(CmdRetargetSpaceDescent({DoCommandFlag::Execute}, rig.shell.pod, rig.shell.generation, center.station, center.generation).Succeeded());
	TickSpaceDescents(); TickSpaceDescents(); CHECK(ResolveSpacePod(rig.shell) == nullptr); CHECK(rig.center->goods[rig.cargo].TotalCount() == 40);
	TickSpaceDescents(); CHECK(rig.center->goods[rig.cargo].TotalCount() == 40);
}
