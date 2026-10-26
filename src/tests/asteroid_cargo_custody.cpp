/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file asteroid_cargo_custody.cpp Original mined packets across six actual native containers after copied source retirement.
 * @see docs/zz-reach2/upgrades/2026-10/r2sff1-space-fixes-features-1.md Groups16/21; emitted packets now enter native custody with their complete typed origin.
 * Declared unit owners prove provenance/custody only, not paid commissioning or timed descent/service.
 */
#include "../stdafx.h"
#include "probe_mining_fixture.h"
#include "../asteroid_cargo.h"
#include "../space_mining_inventory.h"
#include "../hub_cargo.h"
#include "../launch_center.h"
#include "../space_pod.h"
#include "../space_structure.h"
#include "../orbital_station.h"
#include "../roadveh.h"
#include "../station_base.h"
#include "../safeguards.h"

/** Actual extraction earns one packet; controlled retirement changes only copied field metadata while native transfers preserve that packet. */
TEST_CASE("Mined cargo retains original historical provenance through six native owners", "[asteroid-cargo-custody]")
{
	ProbeExtractionRig rig;
	auto &craft = rig.Probe(); auto &body = rig.Attach(craft);
	const auto source = PrepareAsteroidMiningSource(body); REQUIRE(source);
	const auto origin = source->GetAsteroidOrigin(); REQUIRE(origin);
	// Work is earned through the ordinary mining opportunities, without assigning
	// reserve, cargo or a precompleted work counter in this custody fixture.
	for (uint tick = 0; tick < 74; ++tick) rig.Step();
	REQUIRE(craft.cargo.TotalCount() == 2);
	REQUIRE(CmdSetSpaceMission(DoCommandFlag::Execute, craft.index, UINT32_MAX).Succeeded());
	auto *packet = craft.cargo.Packets()->front();
	const auto packet_id = packet->index; const auto first = packet->GetFirstStation();
	const auto production = packet->GetSourceTile(); const auto credit = packet->GetFeederShare();
	const auto live = GetAsteroidField(); auto retired = live;
	const std::array exits{AsteroidExitRecord{body.id, {body.size, body.resource_class}}};
	const auto ledger = ApplyAsteroidExitCredit(retired.renewal, exits, retired.next_body_id); REQUIRE(ledger);
	retired.renewal = *ledger;
	RemoveAsteroidKnowledge(retired, body.id); REQUIRE(EraseAsteroidBody(retired, body.id));
	REQUIRE(ValidateAsteroidField(retired));
	CHECK(ValidateAsteroidPacketCustody(retired)); // The native writer already supplied the mineral; copied body retirement cannot erase that historical meaning.
	REQUIRE(packet->UpgradeLegacyAsteroidSource(origin->mineral));
	CHECK(packet->UpgradeLegacyAsteroidSource(origin->mineral));
	CHECK_FALSE(packet->UpgradeLegacyAsteroidSource(CargoLabel{"FAIL"}));
	const auto source_after = packet->GetSource();
	const auto company_money = Company::Get(_current_company)->money;
	/** Check the sole original packet and unchanged running field at every custody boundary; copied retirement supplies no live mutation or income. */
	auto audit = [&]() {
		CHECK(ValidateAsteroidPacketCustody(retired)); CHECK(GetAsteroidField() == live);
		CHECK(packet->index == packet_id); CHECK(packet->Count() == 2); CHECK(packet->GetSource() == source_after);
		CHECK(packet->GetSourceTile() == production); CHECK(packet->GetFirstStation() == first); CHECK(packet->GetFeederShare() == credit);
		CHECK(Company::Get(_current_company)->money == company_money);
	};
	audit();
	// These are explicit unit containers. Their ordinary packet owners and transfer
	// functions are real; no paid asset receipt or elapsed delivery is claimed.
	REQUIRE(SpaceStructure::CanAllocateItem());
	auto module = std::unique_ptr<SpaceStructure>(SpaceStructure::Create());
	module->state.kind = SpaceBuildKind::Core; module->module = std::make_unique<OrbitalModule>();
	REQUIRE(SpacePod::CanAllocateItem());
	auto pod = std::unique_ptr<SpacePod>(SpacePod::Create()); pod->descent = std::make_unique<SpaceDescentJob>();
	pod->descent->cargo = craft.cargo_type;
	const auto surface = TileXYL(20, 20, MapLayer::Surface);
	REQUIRE(Station::CanAllocateItem());
	auto station = std::unique_ptr<Station>(Station::Create(surface)); station->owner = _current_company;
	station->launch_hub = std::make_unique<LaunchCollectionHub>();
	auto &handling = station->launch_hub->handling.emplace_back(std::make_unique<SurfaceHandlingJob>());
	handling->cargo = craft.cargo_type;
	REQUIRE(Vehicle::CanAllocateItem());
	auto *road = RoadVehicle::Create(); road->owner = _current_company; road->cargo_type = craft.cargo_type; road->cargo_cap = 20; road->tile = surface;
	// Rig's native pool teardown owns this declared ordinary vehicle. Whole-packet
	// moves into empty containers preserve its ID without split/merge allocations.
	auto &stock = module->module->stock[craft.cargo_type];
	REQUIRE(HubCargoTransfer::Move(craft.cargo, stock, 2, craft.tile)); audit();
	REQUIRE(HubCargoTransfer::Move(stock, craft.cargo, 2, craft.tile)); audit();
	REQUIRE(HubCargoTransfer::ReleaseOrbital(craft.cargo, pod->descent->packets, 2, craft.tile)); audit();
	REQUIRE(HubCargoTransfer::Move(pod->descent->packets, handling->packets, 2)); audit();
	auto &goods = station->goods[craft.cargo_type].GetOrCreateData().cargo;
	REQUIRE(HubCargoTransfer::Move(handling->packets, goods, 2)); audit();
	REQUIRE(HubCargoTransfer::Move(goods, road->cargo, 2, surface)); audit();
	{
		AutoRestoreBackup<CargoType> wrong(road->cargo_type, GetCargoTypeByLabel(CargoLabel{"WICE"}));
		CHECK_FALSE(ValidateAsteroidPacketCustody(retired));
	}
	audit();
	// A deliberately aliased raw list entry exercises duplicate detection without
	// calling Append (which could merge/delete the original). Remove it before
	// native destruction; this corruption confers no extra cached quantity.
	auto &alias = *const_cast<CargoPacketList *>(craft.cargo.Packets());
	alias.push_back(packet);
	CHECK_FALSE(ValidateAsteroidPacketCustody(retired));
	alias.pop_back(); audit();
}
