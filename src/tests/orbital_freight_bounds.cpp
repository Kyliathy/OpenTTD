/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file orbital_freight_bounds.cpp Isolated native packet/capacity refusal boundaries; never a paid gameplay predecessor.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs9-launch-centers-and-rockets.md Group11C.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../orbital_freight.h"
#include "../space_vehicle.h"
#include "../space_manifest.h"
#include "../space_structure.h"
#include "../space_structure_cmd.h"
#include "../space_approach.h"
#include "../hub_cargo.h"
#include "../company_base.h"
#include "../engine_base.h"
#include "../core/backup_type.hpp"
#include "../core/random_func.hpp"
#include "../openttd.h"
#include <filesystem>
#include "excavation_fixture.h"
#include "layer_save_fixture.h"
#include "mock_environment.h"
#include "../safeguards.h"

/** @param kind Canonical isolated module recipe. @param index Explicit test identity. @param x Adjacent test plane X.
 * @return Consumed boundary receipt with no inventory; no native gameplay proof imports this constructed data.
 */
static SpaceStructureRecord FreightBoundaryFrame(SpaceBuildKind kind, uint16_t index, uint16_t x)
{
	const auto &definition = *GetSpaceConstructionDefinition(kind);
	SpaceStructureRecord row; row.ref = {SpaceStructureID{index}, uint64_t{index} + 1}; row.state.owner = _current_company;
	row.state.kind = kind; row.state.x = x; row.state.y = 4; row.state.capital_paid = definition.capital;
	row.state.phase = SpaceStructurePhase::FrameComplete; row.work_ticks = definition.work_ticks;
	row.consumed = {static_cast<uint16_t>(definition.inputs[0].quantity), static_cast<uint16_t>(definition.inputs[1].quantity)};
	return row;
}

/** Explicit isolated hull/packet/receipt arrangement for boundary tests, separate from genuine paid-world native acceptance. */
class OrbitalFreightBoundaryRig {
	ExcavationFixture world;
	LayerSaveContent content{world.ActivatedContent()};
	AutoRestoreBackup<TimerGameCalendar::Year> year{TimerGameCalendar::year, TimerGameCalendar::Year{1995}};
	EngineID engine = EngineID::Invalid();
public:
	SpaceVehicle *rocket = nullptr; ///< One constructed boundary hull; never exported as a gameplay fixture.
	/** Register canonical native definitions and actual contained stock; isolated receipts grant no transported inputs or claimed gameplay progress. */
	OrbitalFreightBoundaryRig()
	{
		MockEnvironment::Instance(); REQUIRE(Vehicle::GetNumItems() == 0); ResetUtilityPowerState(); ResetSpaceStructures();
		TimerGameCalendar::date = TimerGameCalendar::ConvertYMDToDate(TimerGameCalendar::Year{1995}, 0, 1);
		const std::array rows{FreightBoundaryFrame(SpaceBuildKind::Core, 0, 8), FreightBoundaryFrame(SpaceBuildKind::Storage, 1, 11), FreightBoundaryFrame(SpaceBuildKind::Power, 2, 6)};
		REQUIRE(InstallLoadedSpaceStructures(rows, 4));
		auto &core = *SpaceStructure::Get(SpaceStructureID{0}); REQUIRE(CmdActivateOrbitalCore({DoCommandFlag::Execute}, core.index, core.generation).Succeeded());
		// Activation advances only the existing public countdown; the isolated recipe receipt still provides no free cargo or usable energy.
		for (uint tick = 0; tick < GetMultiLayerSpaceBudget().core_activation_ticks; ++tick) TickOrbitalConstructionInventory();
		// Each extension retains its actual shared Core dependency and original role-specific functionality.
		for (uint16_t index : {1, 2}) {
			auto &site = *SpaceStructure::Get(SpaceStructureID{index}); REQUIRE(CmdActivateOrbitalModule({DoCommandFlag::Execute}, site.index, site.generation).Succeeded());
		}
		for (uint tick = 0; tick < GetMultiLayerSpaceBudget().core_activation_ticks; ++tick) TickOrbitalConstructionInventory();
		REQUIRE(GetSpaceEngine(14) == EngineID::Invalid()); REQUIRE(EnsureSpaceEngineDefinition(14)); engine = GetSpaceEngine(14);
		REQUIRE(Vehicle::CanAllocateItem()); rocket = SpaceVehicle::Create(); rocket->owner = _current_company; rocket->engine_type = engine; rocket->generation = 1;
		rocket->construction.placement = SpacePlacement::InTransit; rocket->mission.phase = SpaceMissionPhase::Holding;
		rocket->manifest = std::make_unique<SpaceManifest>(); rocket->transition = std::make_unique<SpaceTransition>();
		rocket->transition->phase = SpaceFlightPhase::OrbitalDelivery; rocket->transition->layer = MapLayer::Space;
		rocket->transition->outbound_burned = true; rocket->transition->missions_started = 1;
		rocket->transition->source = rocket->transition->recovery = {StationID{0}, 1};
		rocket->transition->destination = {SpaceOrderTargetKind::Structure, core.index.base(), core.generation};
		// Boundary geometry starts at the actual Core slot. Native shared arbitration must still admit this hull; the test never manufactures an admitted request.
		rocket->position = GetSpaceStructureApproach(core.state); rocket->tile = TileXYL(core.state.x, core.state.y + 3, MapLayer::Space);
		rocket->cargo_type = GetCargoTypeByLabel(CargoLabel{"STEL"}); rocket->cargo_cap = GetSpaceFreightCapacity(14, rocket->cargo_type);
		REQUIRE(RequestSpaceApproach(*rocket, rocket->transition->destination, true));
		{ AutoRestoreBackup pause(_pause_mode, PauseModes{}); PrepareSpaceApproachTick(); }
		REQUIRE(GetSpaceStructureBerth(*rocket, core));
		// Constructed boundary freight uses the same loading-tile transition as native station-to-vehicle moves, preserving its real in-vehicle packet state.
		auto *freight = Packet(2, 7, Money{17}); freight->UpdateLoadingTile(TileXYL(1, 1, MapLayer::Surface)); rocket->cargo.Append(freight);
		Stock(rocket->manifest->supplies[0], 5, 8, Money{19}); Stock(rocket->manifest->supplies[1], 1, 9, Money{23}); Stock(rocket->manifest->supplies[2], 1, 10, Money{29});
	}
	/** Account for every explicit boundary quantity before native pool cleanup; no constructed receipt or packet escapes this isolated fixture. */
	~OrbitalFreightBoundaryRig()
	{
		if (rocket != nullptr) {
			rocket->cargo.Truncate();
			for (auto &stock : rocket->manifest->supplies) if (stock.TotalCount() != 0) HubCargoTransfer::Consume(stock, stock.TotalCount());
		}
		// All possible module destinations are actual containers; teardown consumes only isolated test data and cannot retire a gameplay shipment.
		for (auto *site : SpaceStructure::Iterate()) {
			if (!site->module) continue;
			for (auto &stock : site->module->stock) if (stock.TotalCount() != 0) HubCargoTransfer::Consume(stock, stock.TotalCount());
		}
		_vehicle_pool.CleanPool(); _space_structure_pool.CleanPool(); ResetSpaceStructures(); ResetUtilityPowerState();
		if (engine != EngineID::Invalid()) delete Engine::Get(engine);
	}
	/** @param quantity Explicit isolated units. @param age Boundary provenance age. @param share Boundary feeder share.
     * @return Newly constructed test packet; it is never presented as purchased, mined or transported gameplay input.
     */
	CargoPacket *Packet(uint16_t quantity, uint16_t age, Money share)
	{
		REQUIRE(CargoPacket::CanAllocateItem());
		return CargoPacket::Create(quantity, age, StationID::Invalid(), TileXYL(1, 1, MapLayer::Surface), share);
	}
	/** @param stock Exact native station-style test owner. @param quantity Isolated units. @param age Distinct test provenance age. @param share Whole test feeder share. */
	void Stock(StationCargoList &stock, uint16_t quantity, uint16_t age, Money share)
	{
		stock.Append(Packet(quantity, age, share), StationID::Invalid());
	}
};

/** A full later RFMT label refuses the whole original freight/part set before an earlier STEL packet can leave. */
TEST_CASE("Module freight preflights every physical label before moving any packet", "[orbital-freight-boundary]")
{
	OrbitalFreightBoundaryRig rig; auto &rocket = *rig.rocket; auto &core = *SpaceStructure::Get(SpaceStructureID{0}); const auto target = GetSpaceStructureReference(core);
	const auto steel = rocket.cargo_type, metal = GetSpaceSupplyCargo(SpaceSupply::RefinedMetal), electronics = GetSpaceSupplyCargo(SpaceSupply::Electronics);
	rig.Stock(core.module->stock[metal], 1024, 11, Money{103});
	const auto before = ComputeSpaceStructureDigest(), manifest = ComputeSpaceManifestDigest(), power = ComputeUtilityPowerDigest();
	const auto money = Company::Get(_current_company)->money; const auto seeds = _random; const auto packet = rocket.cargo.Packets()->front()->index;
	CHECK(GetOrbitalModuleFreightBlock(rocket, target, false) == SpaceFlightBlock::ReceiverFull); CHECK(GetOrbitalModuleFreightBlock(rocket, target, true) == SpaceFlightBlock::ReceiverFull);
	CHECK_FALSE(CommitOrbitalModuleFreight(rocket, target)); CHECK(ComputeSpaceStructureDigest() == before); CHECK(ComputeSpaceManifestDigest() == manifest);
	CHECK(rocket.cargo.TotalCount() == 2); CHECK(core.module->stock[steel].TotalCount() == 0); CHECK(rocket.manifest->supplies[1].TotalCount() == 1); CHECK(rocket.manifest->supplies[2].TotalCount() == 1);
	REQUIRE(HubCargoTransfer::Consume(core.module->stock[metal], 1)); const auto quoted = ComputeSpaceStructureDigest();
	CHECK(GetOrbitalModuleFreightBlock(rocket, target, true) == SpaceFlightBlock::None); CHECK(ComputeSpaceStructureDigest() == quoted);
	REQUIRE(CommitOrbitalModuleFreight(rocket, target)); CHECK(rocket.cargo.TotalCount() == 0); CHECK(core.module->stock[steel].TotalCount() == 2);
	CHECK(core.module->stock[metal].TotalCount() == 1024); CHECK(core.module->stock[electronics].TotalCount() == 1);
	CHECK(rocket.manifest->supplies[0].TotalCount() == 5); CHECK(rocket.manifest->supplies[1].TotalCount() == 0); CHECK(rocket.manifest->supplies[2].TotalCount() == 0);
	CHECK(CargoPacket::Get(packet)->GetPeriodsInTransit() == 7); CHECK(CargoPacket::Get(packet)->GetFeederShare() == Money{17}); CHECK(CargoPacket::Get(packet)->GetSourceTile() == TileXYL(1, 1, MapLayer::Surface));
	CHECK_FALSE(CommitOrbitalModuleFreight(rocket, target)); CHECK(ComputeUtilityPowerDigest() == power); CHECK(Company::Get(_current_company)->money == money);
	CHECK(_random.state[0] == seeds.state[0]); CHECK(_random.state[1] == seeds.state[1]);
}

/** Conservative64-reference admission cannot rely on a possible native merge to fit a65th fragment. */
TEST_CASE("Module freight preserves the native per-label packet-reference bound", "[orbital-freight-boundary]")
{
	OrbitalFreightBoundaryRig rig; auto &rocket = *rig.rocket; auto &core = *SpaceStructure::Get(SpaceStructureID{0}); const auto target = GetSpaceStructureReference(core);
	// Distinct actual ages prevent merges, isolating fragment capacity from the much larger whole-unit capacity.
	for (uint16_t fragment = 0; fragment < 64; ++fragment) rig.Stock(core.module->stock[rocket.cargo_type], 1, 100 + fragment, Money{fragment});
	const auto before = ComputeSpaceStructureDigest(), manifest = ComputeSpaceManifestDigest();
	CHECK(GetOrbitalModuleFreightBlock(rocket, target, true) == SpaceFlightBlock::ReceiverFull); CHECK_FALSE(CommitOrbitalModuleFreight(rocket, target));
	CHECK(ComputeSpaceStructureDigest() == before); CHECK(ComputeSpaceManifestDigest() == manifest); CHECK(rocket.cargo.TotalCount() == 2);
	REQUIRE(HubCargoTransfer::Consume(core.module->stock[rocket.cargo_type], 1));
	CHECK(GetOrbitalModuleFreightBlock(rocket, target, true) == SpaceFlightBlock::None); REQUIRE(CommitOrbitalModuleFreight(rocket, target));
	CHECK(core.module->stock[rocket.cargo_type].TotalCount() == 65); CHECK(core.module->stock[rocket.cargo_type].Packets()->begin()->second.size() == 64);
}

/** Freight and a service compartment sharing one physical label must fit together; individually valid slices cannot overfill the sole module owner. */
TEST_CASE("Module freight aggregates shared labels across existing compartments", "[orbital-freight-boundary]")
{
	OrbitalFreightBoundaryRig rig; auto &rocket = *rig.rocket; auto &core = *SpaceStructure::Get(SpaceStructureID{0}); const auto target = GetSpaceStructureReference(core);
	const auto metal = GetSpaceSupplyCargo(SpaceSupply::RefinedMetal);
	// This isolated input arrangement labels the existing two-unit freight owner RFMT, separately from its actual one-unit RFMT service compartment.
	rocket.cargo_type = metal; rocket.cargo_cap = GetSpaceFreightCapacity(14, metal); rig.Stock(core.module->stock[metal], 1022, 11, Money{103});
	const auto before = ComputeSpaceStructureDigest(), manifest = ComputeSpaceManifestDigest();
	CHECK(GetOrbitalModuleFreightBlock(rocket, target, true) == SpaceFlightBlock::ReceiverFull); CHECK_FALSE(CommitOrbitalModuleFreight(rocket, target));
	CHECK(ComputeSpaceStructureDigest() == before); CHECK(ComputeSpaceManifestDigest() == manifest); CHECK(rocket.cargo.TotalCount() == 2); CHECK(rocket.manifest->supplies[1].TotalCount() == 1);
	REQUIRE(HubCargoTransfer::Consume(core.module->stock[metal], 1)); CHECK(GetOrbitalModuleFreightBlock(rocket, target, true) == SpaceFlightBlock::None);
	REQUIRE(CommitOrbitalModuleFreight(rocket, target)); CHECK(core.module->stock[metal].TotalCount() == 1024); CHECK(rocket.cargo.TotalCount() == 0); CHECK(rocket.manifest->supplies[1].TotalCount() == 0);
}

/** Exact owner/incarnation, actual stock role, passenger/service custody and the precise saved berth all participate in whole admission. */
TEST_CASE("Module freight refuses stale inert foreign and incompatible custody", "[orbital-freight-boundary]")
{
	OrbitalFreightBoundaryRig rig; auto &rocket = *rig.rocket; auto &core = *SpaceStructure::Get(SpaceStructureID{0}); const auto target = GetSpaceStructureReference(core);
	const auto before = ComputeSpaceStructureDigest(), manifest = ComputeSpaceManifestDigest();
	CHECK(GetOrbitalModuleFreightBlock(rocket, {target.structure, target.generation + 100}, false) == SpaceFlightBlock::TargetLost);
	{ AutoRestoreBackup owner(core.state.owner, OWNER_NONE); CHECK(GetOrbitalModuleFreightBlock(rocket, target, false) == SpaceFlightBlock::TargetLost); }
	{ AutoRestoreBackup activation(core.module->activation_ticks, uint16_t{1}); CHECK(GetOrbitalModuleFreightBlock(rocket, target, false) == SpaceFlightBlock::ReceiverBusy); }
	CHECK(GetOrbitalModuleFreightBlock(rocket, GetSpaceStructureReference(*SpaceStructure::Get(SpaceStructureID{2})), false) == SpaceFlightBlock::ReceiverBusy);
	const auto storage = GetSpaceStructureReference(*SpaceStructure::Get(SpaceStructureID{1}));
	CHECK(GetOrbitalModuleFreightBlock(rocket, storage, false) == SpaceFlightBlock::None); CHECK(GetOrbitalModuleFreightBlock(rocket, storage, true) == SpaceFlightBlock::ReceiverBusy);
	{ AutoRestoreBackup passenger(rocket.manifest->passenger, SpaceCraftRef{VehicleID{1}, 2}); CHECK(GetOrbitalModuleFreightBlock(rocket, target, false) == SpaceFlightBlock::ReceiverBusy); }
	{ AutoRestoreBackup service(rocket.manifest->service_ticks, uint16_t{1}); CHECK(GetOrbitalModuleFreightBlock(rocket, target, false) == SpaceFlightBlock::ReceiverBusy); }
	CHECK(ComputeSpaceStructureDigest() == before); CHECK(ComputeSpaceManifestDigest() == manifest); CHECK(rocket.cargo.TotalCount() == 2);
}
