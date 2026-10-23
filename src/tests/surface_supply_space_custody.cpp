/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file surface_supply_space_custody.cpp Legacy receipt admission in actual native Space owners and first-departure paths.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md actual packet ownership.
 * @see docs/zz-reach2/upgrades/2026-10/m2o23-1-come-space-h-cargo-custody.md complete owner inventory.
 * Native objects and original stock are declared components, not an operational fleet or protected saved-world continuation.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "native_cargo_custody_fixture.h"
#include "surface_supply_legacy_fixture.h"
#include "../hub_cargo.h"
#include "../safeguards.h"

/** Every applicable real Space compartment joins the same paid receipt while ordinary and unlinked A cargo stay distinct. */
TEST_CASE("Legacy shipped stock retains truthful history in every applicable native Space owner", "[surface-supply-space-custody]")
{
	MockEnvironment::Instance(); LoadGeneratedWindowTestLanguage(); NativeCargoCustodyRig rig; LegacySupplyComponent legacy(rig.station);
	const auto owner = GENERATE(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12); auto *station = Station::Get(rig.station);
	CargoLabel label{"RFMT"}; StationCargoList *target = nullptr;
	switch (owner) {
		case 0: target = &station->launch_hub->handling[0]->packets; break;
		case 1: rig.craft->cargo_type = GetCargoTypeByLabel(label); break;
		case 2: target = &rig.craft->manifest->supplies[1]; break;
		case 3: label = CargoLabel{"ELEC"}; target = &rig.craft->manifest->supplies[2]; break;
		case 4: label = CargoLabel{"STEL"}; target = &rig.pod->descent->packets; break;
		case 5: label = CargoLabel{"STEL"}; target = &rig.blueprint->job.inputs[0]; break;
		case 6: target = &rig.dock->module->stock[GetCargoTypeByLabel(label)]; break;
		case 7: target = &rig.dock->module->GetServiceJobs()[0]->inputs[0]; break;
		case 8: label = CargoLabel{"ELEC"}; target = &rig.dock->module->GetServiceJobs()[0]->inputs[1]; break;
		case 9: target = &rig.dock->module->GetServiceJobs()[1]->inputs[0]; break;
		case 10: label = CargoLabel{"ELEC"}; target = &rig.dock->module->GetServiceJobs()[1]->inputs[1]; break;
		case 11: label = CargoLabel{"STEL"}; target = &rig.factory->module->manufacture->inputs[0]; break;
		case 12: label = CargoLabel{"ELEC"}; target = &rig.factory->module->manufacture->inputs[3]; break;
	}
	// Keep genuine ordinary and unlinked native purpose controls in real goods
	// queues. They confer neither a legacy link nor another refundable receipt.
	rig.Add(station->goods[GetCargoTypeByLabel(CargoLabel{"STEL"})].GetOrCreateData().cargo, CargoLabel{"STEL"}, rig.station, 3);
	rig.Own(station->goods[GetCargoTypeByLabel(CargoLabel{"BATY"})].GetOrCreateData().cargo,
		legacy.Packet(2, false, 0, Source{_current_company.base(), SourceType::CompanySupply}));
	// Thirty original units fit every supported material's purchase cap;
	// a larger fixture must not weaken ELEC's real64-unit admission rule.
	auto *packet = legacy.Packet(30, true); packet->UpdateLoadingTile(station->xy);
	if (target != nullptr) { packet->UpdateUnloadingTile(station->xy); rig.Own(*target, packet); }
	else rig.craft->cargo.Append(packet);
	const auto facts = LegacyComponentPacketFacts(); const auto cash = Company::Get(_current_company)->money;
	const auto candidate = legacy.Candidate(30, 30, label); legacy.Restore(candidate);
	CHECK(CaptureSurfaceSupplyOrders() == candidate); CHECK(ValidateSurfaceSupplyOrderBook()); CHECK(LegacyComponentPacketFacts() == facts);
	CHECK(Company::Get(_current_company)->money == cash); CHECK(GetSurfaceSupplyOrderDayUsage(_current_company, label) == 30);
	CHECK(packet->GetFeederShare() == 0); CHECK(packet->GetSource().IsCompanySupply()); CHECK(packet->GetCompanySupplyOrder() == 1);
	CHECK(packet->HasCompanySupplyShipped()); CHECK(packet->MarkCompanySupplyShipped()); CHECK(CaptureSurfaceSupplyOrders() == candidate);
}

/** Propellant, generic Goods and robot-parts compartments cannot acquire construction-supply rights by a matching packet link. */
TEST_CASE("Legacy Space custody rejects impossible paid labels and unshipped nonstation ownership", "[surface-supply-space-custody]")
{
	MockEnvironment::Instance(); LoadGeneratedWindowTestLanguage(); NativeCargoCustodyRig rig; LegacySupplyComponent legacy(rig.station);
	const auto fault = GENERATE(0, 1, 2, 3, 4, 5); StationCargoList *target = nullptr;
	switch (fault) {
		case 0: target = &rig.craft->manifest->supplies[0]; break; // Actual PROP role, never RFMT.
		case 1: target = &rig.blueprint->job.inputs[1]; break; // Actual GOOD construction bill.
		case 2: target = &rig.factory->module->manufacture->inputs[2]; break; // Actual heavy RBPT input.
		case 3: target = &rig.craft->manifest->supplies[1]; break;
		case 4: target = &Station::Get(rig.station)->launch_hub->handling[0]->packets; break;
		case 5: target = &rig.dock->module->stock[GetCargoTypeByLabel(CargoLabel{"RFMT"})]; break;
	}
	const bool shipped = fault < 3; rig.Own(*target, legacy.Packet(100, shipped));
	const auto candidate = legacy.Candidate(100, shipped ? 100 : 0); const auto facts = LegacyComponentPacketFacts(); const auto cash = Company::Get(_current_company)->money;
	REQUIRE(StageLoadedSurfaceSupplyOrders(candidate, TimerGameEconomy::date)); legacy.ObserveWorld();
	CHECK_FALSE(AcceptLoadedSurfaceSupplyOrders(true)); CHECK(CaptureSurfaceSupplyOrders().records.empty());
	CHECK(LegacyComponentPacketFacts() == facts); CHECK(target->TotalCount() == 100); CHECK(Company::Get(_current_company)->money == cash);
}

/** Real load, reservation and hub paths retire each selected legacy unit exactly once, including return and repeat movement. */
TEST_CASE("Actual native reservation loading and hub departure preserve finite legacy rights through returns", "[surface-supply-space-custody]")
{
	MockEnvironment::Instance(); LoadGeneratedWindowTestLanguage(); NativeCargoCustodyRig rig; LegacySupplyComponent legacy(rig.station);
	const auto mode = GENERATE(0, 1, 2); auto *station = Station::Get(rig.station); const auto tile = station->xy;
	auto &source = station->goods[GetCargoTypeByLabel(CargoLabel{"RFMT"})].GetOrCreateData().cargo;
	auto &job = station->launch_hub->handling[0]->packets; auto &carrier = rig.train->cargo; rig.Track(job);
	rig.Own(source, legacy.Packet()); const auto original = legacy.Candidate(); legacy.Restore(original);
	const auto cash = Company::Get(_current_company)->money; const auto random = _random;
	if (mode == 0) REQUIRE(source.Load(30, &carrier, {}, tile) == 30);
	if (mode == 1) REQUIRE(source.Reserve(30, &carrier, {}, tile) == 30);
	if (mode == 2) {
		REQUIRE(HubCargoTransfer::Move(source, job, 30)); CHECK(ValidateSurfaceSupplyOrderBook());
		CHECK(GetSurfaceSupplyOrder(1)->Legacy()->dispatched == 30); REQUIRE(HubCargoTransfer::Move(job, carrier, 30, tile));
	}
	CHECK(source.AvailableCount() == 70); CHECK(source.ReservedCount() == (mode == 1 ? 30 : 0)); CHECK(carrier.TotalCount() == 30);
	CHECK(GetSurfaceSupplyOrder(1)->Legacy()->dispatched == 30); CHECK(source.CountUnshippedCompanySupply(1) == 70); CHECK(ValidateSurfaceSupplyOrderBook());
	const auto *carried = *carrier.Packets()->begin(); CHECK(carried->Count() == 30); CHECK(carried->HasCompanySupplyShipped()); CHECK(carried->GetFeederShare() == 0);
	if (mode == 1) {
		// Reservation already ended source eligibility. Loading ten of those
		// units and returning the other twenty cannot count a second departure.
		REQUIRE(source.Load(10, &carrier, {}, tile) == 10); CHECK(source.ReservedCount() == 20);
		CHECK(GetSurfaceSupplyOrder(1)->Legacy()->dispatched == 30); REQUIRE(carrier.Return(20, &source, StationID::Invalid(), tile) == 20);
		CHECK(source.ReservedCount() == 0); CHECK(source.AvailableCount() == 90); CHECK(carrier.TotalCount() == 10); CHECK(ValidateSurfaceSupplyOrderBook());
	}
	const auto in_carrier = carrier.TotalCount();
	REQUIRE(HubCargoTransfer::Move(carrier, job, in_carrier, tile)); REQUIRE(HubCargoTransfer::Move(job, carrier, in_carrier, tile));
	CHECK(GetSurfaceSupplyOrder(1)->Legacy()->dispatched == 30); REQUIRE(HubCargoTransfer::Move(carrier, source, in_carrier, tile));
	CHECK(source.TotalCount() == 100); CHECK(source.ReservedCount() == 0); CHECK(carrier.TotalCount() == 0); CHECK(job.TotalCount() == 0);
	CHECK(source.CountUnshippedCompanySupply(1) == 70); CHECK(ValidateSurfaceSupplyOrderBook());
	const auto returned = LegacyComponentPacketFacts(); auto expected_book = original; expected_book.records[0].Legacy()->dispatched = 30;
	CHECK_FALSE(source.RemoveUnshippedCompanySupply(1, 69)); CHECK(LegacyComponentPacketFacts() == returned); CHECK(CaptureSurfaceSupplyOrders() == expected_book);
	// Depart all100 actual units: only the70 untouched source units consume
	// new history, regardless of native routing/merge order of returned stock.
	REQUIRE(HubCargoTransfer::Move(source, carrier, 100, tile)); CHECK(source.TotalCount() == 0); CHECK(carrier.TotalCount() == 100);
	expected_book.records[0].Legacy()->dispatched = 100; CHECK(CaptureSurfaceSupplyOrders() == expected_book); CHECK(ValidateSurfaceSupplyOrderBook());
	REQUIRE(HubCargoTransfer::Move(carrier, source, 100, tile)); CHECK(source.CountUnshippedCompanySupply(1) == 0);
	CHECK_FALSE(source.RemoveUnshippedCompanySupply(1, 1)); CHECK(source.RemoveUnshippedCompanySupply(1, 0)); CHECK(source.TotalCount() == 100);
	// Every surviving returned fragment remains non-paying and ineligible;
	// compatible merges may reduce packet count but must preserve these facts.
	for (const auto &facts : LegacyComponentPacketFacts()) {
		CHECK(facts.source.IsCompanySupply()); CHECK(facts.order == 1); CHECK(facts.shipped); CHECK(facts.feeder_share == 0);
	}
	CHECK(CaptureSurfaceSupplyOrders() == expected_book); CHECK_FALSE(GetSurfaceSupplyOrder(1)->Legacy()->closed);
	CHECK(GetSurfaceSupplyOrderDayUsage(_current_company, CargoLabel{"RFMT"}) == 100); CHECK(Company::Get(_current_company)->money == cash);
	CHECK(_random.state[0] == random.state[0]); CHECK(_random.state[1] == random.state[1]); CHECK(ValidateSurfaceSupplyOrderBook());
}
