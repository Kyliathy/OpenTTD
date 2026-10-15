/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file orbital_evacuation.cpp Isolated physical packet bounds for whole evacuation; not a paid gameplay predecessor.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Groups5B/5C.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../space_structure.h"
#include "../space_structure_cmd.h"
#include "../hub_cargo.h"
#include "../core/backup_type.hpp"
#include "../core/random_func.hpp"
#include "../map_func.h"
#include "excavation_fixture.h"
#include "../safeguards.h"

/** @param kind Isolated canonical static recipe. @param index Native boundary identity. @param x Plane X. @param y Plane Y. @return Detached consumed test receipt with no physical inventory. */
static SpaceStructureRecord EvacuationBoundaryFrame(SpaceBuildKind kind, uint16_t index, uint16_t x, uint16_t y)
{
	const auto &definition = *GetSpaceConstructionDefinition(kind);
	SpaceStructureRecord row; row.ref = {SpaceStructureID{index}, uint64_t{index} + 1}; row.state.owner = _current_company;
	row.state.kind = kind; row.state.x = x; row.state.y = y; row.state.capital_paid = definition.capital;
	row.state.phase = SpaceStructurePhase::FrameComplete; row.work_ticks = definition.work_ticks;
	row.consumed = {static_cast<uint16_t>(definition.inputs[0].quantity), static_cast<uint16_t>(definition.inputs[1].quantity)};
	return row;
}

/** Actual packet/container command fixture with explicit isolated receipts/quantities; native delivered acceptance uses a separate genuine paid save. */
class OrbitalEvacuationRig {
	ExcavationFixture world;
	AutoRestoreBackup<TimerGameCalendar::Year> year{TimerGameCalendar::year, TimerGameCalendar::Year{1995}};
public:
	/** @param alternative Add a second completed Core that keeps replacement Storage valid when the source Core is removed. */
	OrbitalEvacuationRig(bool alternative = true)
	{
		ResetUtilityPowerState(); ResetSpaceStructures();
		TimerGameCalendar::date = TimerGameCalendar::ConvertYMDToDate(TimerGameCalendar::Year{1995}, 0, 1);
		std::vector rows{EvacuationBoundaryFrame(SpaceBuildKind::Core, 0, 8, 4), EvacuationBoundaryFrame(SpaceBuildKind::Storage, 1, 11, 4)};
		if (alternative) rows.push_back(EvacuationBoundaryFrame(SpaceBuildKind::Core, 2, 13, 4));
		REQUIRE(InstallLoadedSpaceStructures(rows, rows.size() + 1));
		// Independent paid-frame boundary Cores finish together; activation itself grants no packet or usable energy.
		for (auto *site : SpaceStructure::Iterate()) if (site->state.kind == SpaceBuildKind::Core) REQUIRE(CmdActivateOrbitalCore({DoCommandFlag::Execute}, site->index, site->generation).Succeeded());
		for (uint tick = 0; tick < 74; ++tick) TickOrbitalConstructionInventory();
		auto &storage = *SpaceStructure::Get(SpaceStructureID{1}); REQUIRE(CmdActivateOrbitalModule({DoCommandFlag::Execute}, storage.index, storage.generation).Succeeded());
		for (uint tick = 0; tick < 74; ++tick) TickOrbitalConstructionInventory();
	}
	/** Delete only these isolated test quantities through the actual container API before native owner teardown. */
	~OrbitalEvacuationRig()
	{
		// Boundary packets never escape into another fixture or a gameplay checkpoint; cleanup accounts for every possible source/destination owner.
		for (auto *site : SpaceStructure::Iterate()) {
			for (auto &slot : site->job.inputs) if (slot.TotalCount() != 0) HubCargoTransfer::Consume(slot, slot.TotalCount());
			if (!site->module) continue;
			for (auto &slot : site->module->stock) if (slot.TotalCount() != 0) HubCargoTransfer::Consume(slot, slot.TotalCount());
			for (auto *job : site->module->GetServiceJobs()) for (auto &slot : job->inputs) if (slot.TotalCount() != 0) HubCargoTransfer::Consume(slot, slot.TotalCount());
		}
		_space_structure_pool.CleanPool(); ResetSpaceStructures(); ResetUtilityPowerState();
	}
	/** Create explicit isolated boundary data, never a claimed transported input.
	 * @param site Actual test container owner. @param cargo Physical label. @param quantity Isolated whole units.
	 * @param age Test provenance age. @param share Test feeder share. @return Exact packet identity for conservation assertions.
	 */
	CargoPacketID Stock(SpaceStructure &site, CargoType cargo, uint16_t quantity, uint16_t age, Money share)
	{
		REQUIRE(CargoPacket::CanAllocateItem());
		auto *packet = CargoPacket::Create(quantity, age, StationID::Invalid(), TileXYL(site.state.x, site.state.y, MapLayer::Space), share);
		const auto id = packet->index; site.module->stock[cargo].Append(packet, StationID::Invalid()); return id;
	}
};

/** A full later label preserves every earlier label; whole success retains original packet age, source and feeder share with no energy or payment. */
TEST_CASE("Full replacement Storage refuses whole evacuation before any source label moves", "[orbital-evacuation-contract]")
{
	OrbitalEvacuationRig rig; auto &source = *SpaceStructure::Get(SpaceStructureID{0}); auto &destination = *SpaceStructure::Get(SpaceStructureID{1});
	const auto steel = GetCargoTypeByLabel(CargoLabel{"STEL"}), goods = GetCargoTypeByLabel(CargoLabel{"GOOD"});
	const auto packet = rig.Stock(source, steel, 2, 7, Money{17}); rig.Stock(source, goods, 3, 8, Money{29}); rig.Stock(destination, goods, 1024, 9, Money{103});
	REQUIRE(ValidateSpaceStructures()); const auto before = ComputeSpaceStructureDigest(), power = ComputeUtilityPowerDigest();
	const auto cash = Company::Get(_current_company)->money; const auto random = _random;
	CHECK(CmdEvacuateOrbitalModule({}, source.index, source.generation, destination.index, destination.generation).Failed());
	CHECK(CmdEvacuateOrbitalModule({DoCommandFlag::Execute}, source.index, source.generation, destination.index, destination.generation).Failed());
	CHECK(ComputeSpaceStructureDigest() == before); CHECK(ComputeUtilityPowerDigest() == power); CHECK(source.module->stock[steel].TotalCount() == 2); CHECK(destination.module->stock[steel].TotalCount() == 0);
	// Remove only isolated boundary fill through the actual packet API, leaving exactly enough physical destination capacity for the complete operation.
	REQUIRE(HubCargoTransfer::Consume(destination.module->stock[goods], 3));
	const auto admitted = ComputeSpaceStructureDigest(); CHECK(CmdEvacuateOrbitalModule({}, source.index, source.generation, destination.index, destination.generation).Succeeded()); CHECK(ComputeSpaceStructureDigest() == admitted);
	REQUIRE(CmdEvacuateOrbitalModule({DoCommandFlag::Execute}, source.index, source.generation, destination.index, destination.generation).Succeeded());
	CHECK(source.module->stock[steel].TotalCount() == 0); CHECK(source.module->stock[goods].TotalCount() == 0);
	CHECK(destination.module->stock[steel].TotalCount() == 2); CHECK(destination.module->stock[goods].TotalCount() == 1024);
	CHECK(CargoPacket::Get(packet)->Count() == 2); CHECK(CargoPacket::Get(packet)->GetPeriodsInTransit() == 7); CHECK(CargoPacket::Get(packet)->GetFeederShare() == Money{17});
	CHECK(CargoPacket::Get(packet)->GetSourceTile() == TileXYL(source.state.x, source.state.y, MapLayer::Space));
	CHECK(ComputeUtilityPowerDigest() == power); CHECK(GetSpaceStructureRetirementHistory() == SpaceStructureRetirementHistory{});
	CHECK(Company::Get(_current_company)->money == cash); CHECK(_random.state[0] == random.state[0]); CHECK(_random.state[1] == random.state[1]); REQUIRE(ValidateSpaceStructures());
}

/** A destination whose sole Core is the proposed source is unsafe even while its current capacity looks available. */
TEST_CASE("Evacuation cannot strand last-Core inventory inside its own dependent", "[orbital-evacuation-contract]")
{
	OrbitalEvacuationRig rig(false); auto &source = *SpaceStructure::Get(SpaceStructureID{0}); auto &destination = *SpaceStructure::Get(SpaceStructureID{1});
	rig.Stock(source, GetCargoTypeByLabel(CargoLabel{"STEL"}), 1, 3, Money{0});
	const auto before = ComputeSpaceStructureDigest(), power = ComputeUtilityPowerDigest();
	CHECK(CmdEvacuateOrbitalModule({DoCommandFlag::Execute}, source.index, source.generation, destination.index, destination.generation).Failed());
	CHECK(ComputeSpaceStructureDigest() == before); CHECK(ComputeUtilityPowerDigest() == power); REQUIRE(ValidateSpaceStructures());
}
