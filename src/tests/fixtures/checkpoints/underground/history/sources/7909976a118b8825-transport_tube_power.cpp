/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file transport_tube_power.cpp Bounded1970 source/connection/tariff and saved-owner refusal cases. @see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Group5 */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "transport_tube_power_fixture.h"
#include "../multilayer_state.h"
#include "../safeguards.h"

/** Actual paid physical connection is estimate-pure, independent of Deep and incapable of implicit/duplicate service. */
TEST_CASE("Tube paid service requires an explicit commissioned excavated1970 connection", "[transport-tube-power]")
{
	TubePowerRig rig;
	const auto cash = Company::Get(_current_company)->money;
	const auto ledger = ComputeUtilityPowerDigest(), physical = ComputeTubePowerDigest();
	const auto estimate = ConnectTubeExternalPower(rig.portal, rig.depot, rig.path, false);
	REQUIRE(estimate.error == TubePowerError::None);
	CHECK(ComputeUtilityPowerDigest() == ledger); CHECK(ComputeTubePowerDigest() == physical);
	CHECK(Company::Get(_current_company)->money == cash);
	CHECK_FALSE(CheckDeepLayerAvailability(GetCurrentMultiLayerProfileSelection(), TimerGameCalendar::date));
	SECTION("complete paid connection") {
		rig.Connect();
		CHECK(rig.source == estimate.producer);
		CHECK(Company::Get(_current_company)->money == cash - Money{20});
		CHECK(GetTubePowerConnection(rig.source)->invoiced_money == 20);
		CHECK(GetUtilityPowerProducer(rig.source)->meter.generation_energy_j == 0);
		CHECK(ConnectTubeExternalPower(rig.portal, rig.depot, rig.path).error == TubePowerError::InvalidTopology);
	}
	SECTION("disconnected path") {
		const std::array path{rig.path[0], TileXYL(37, 40, MapLayer::Underground), rig.depot};
		CHECK(ConnectTubeExternalPower(rig.portal, rig.depot, path).error == TubePowerError::InvalidTopology);
	}
	SECTION("no utility corridor") {
		CHECK(ConnectTubeExternalPower(rig.portal, rig.depot, {}).error == TubePowerError::InvalidTopology);
	}
	SECTION("sealed portal") {
		auto record = *GetLayerPortalWorkRecord(rig.portal); record.state = LayerPortalWorkState::Sealed;
		StageLoadedLayerPortalWork({record}); AcceptLoadedLayerPortalWork();
		CHECK(ConnectTubeExternalPower(rig.portal, rig.depot, rig.path).error == TubePowerError::InvalidTopology);
	}
	SECTION("early introduction") {
		AutoRestoreBackup date(TimerGameCalendar::date, TimerGameCalendar::ConvertYMDToDate(TimerGameCalendar::Year{1969}, 0, 1));
		CHECK(ConnectTubeExternalPower(rig.portal, rig.depot, rig.path).error == TubePowerError::Unavailable);
	}
	SECTION("lease funds") {
		Company::Get(_current_company)->money = Money{19};
		CHECK(ConnectTubeExternalPower(rig.portal, rig.depot, rig.path).error == TubePowerError::Funds);
		CHECK(ComputeUtilityPowerDigest() == ledger); CHECK(ComputeTubePowerDigest() == physical);
	}
}

/** Publication charges one actual service-day lease; whole physical consumption carries fractional usage independently of allocation. */
TEST_CASE("Tube actual granted service invoices once with exact saved tariff fractions", "[transport-tube-power]")
{
	TubePowerRig rig; rig.Connect();
	const auto cash = Company::Get(_current_company)->money;
	CHECK(PublishTubeExternalPower(rig.source, 1, 74, false) == TubePowerError::None);
	CHECK(GetUtilityPowerProducer(rig.source)->meter.generation_energy_j == 0);
	rig.Offer();
	CHECK(GetUtilityPowerProducer(rig.source)->meter.generation_energy_j == 199800);
	CHECK(GetUtilityAllocation(rig.consumer)->granted_energy_j == 39960);
	CHECK(Company::Get(_current_company)->money == cash);
	CHECK(PublishTubeExternalPower(rig.source, 1, 74) == TubePowerError::Ledger);
	rig.capsule->date_of_last_service = TimerGameEconomy::Date{99};
	CapsuleDepotService work(*rig.capsule);
	REQUIRE(ConsumeTubeExternalPower(rig.source, rig.consumer, 39960, work).error == UtilityPowerError::None);
	CHECK(rig.capsule->date_of_last_service == TimerGameEconomy::date);
	CHECK(Company::Get(_current_company)->money == cash - Money{39});
	CHECK(GetTubePowerConnection(rig.source)->usage_remainder_j == 960);
	CHECK(GetTubePowerConnection(rig.source)->invoiced_money == 59);
	CHECK(GetUtilityPowerProducer(rig.source)->consumed_lifetime_j == 39960);
	CHECK(ConsumeTubeExternalPower(rig.source, rig.consumer, 39960, work).error == UtilityPowerError::InsufficientGrant);
	CHECK(CloseTubeExternalPower(rig.source, false) == TubePowerError::None);
	CHECK(GetTubePowerConnection(rig.source)->usage_remainder_j == 960);
	REQUIRE(CloseTubeExternalPower(rig.source) == TubePowerError::None);
	CHECK(Company::Get(_current_company)->money == cash - Money{40});
	CHECK(GetTubePowerConnection(rig.source)->invoiced_money == 60);
	CHECK(GetTubePowerConnection(rig.source)->usage_remainder_j == 0);
	CHECK_FALSE(GetUtilityPowerProducer(rig.source)->active);
	CHECK(GetUtilityPowerProducer(rig.source)->consumed_lifetime_j == 39960);
	CHECK(GetUtilityAllocation(rig.consumer)->granted_energy_j == 0);
	CHECK(CloseTubeExternalPower(rig.source) == TubePowerError::Refused);
}

/** Financial/participant/capacity failures are complete no-ops; neither granted energy nor mechanical compatibility makes service free. */
TEST_CASE("Tube paid grant refusal preserves capsule ledger and invoice", "[transport-tube-power]")
{
	TubePowerRig rig; rig.Connect(); rig.Offer();
	rig.capsule->date_of_last_service = TimerGameEconomy::Date{99};
	CapsuleDepotService work(*rig.capsule);
	uint64_t quantity = 39960;
	SECTION("physical validation refusal") { work.admit = false; }
	SECTION("physical commit refusal") { work.commit = false; }
	SECTION("whole funds and fractional debt") { Company::Get(_current_company)->money = Money{39}; }
	SECTION("over grant") { quantity = 39961; }
	SECTION("zero work") { quantity = 0; }
	const auto cash = Company::Get(_current_company)->money;
	const auto ledger = ComputeUtilityPowerDigest(), physical = ComputeTubePowerDigest();
	CHECK(ConsumeTubeExternalPower(rig.source, rig.consumer, quantity, work).error != UtilityPowerError::None);
	CHECK(ComputeUtilityPowerDigest() == ledger); CHECK(ComputeTubePowerDigest() == physical);
	CHECK(Company::Get(_current_company)->money == cash);
	CHECK(rig.capsule->date_of_last_service == TimerGameEconomy::Date{99});
}

/** Exact physical path/revision and fraction persist through owner staging, with no repaired or refreshed grant. */
TEST_CASE("Tube physical paid owner rejects malformed saved references and preserves invoice", "[transport-tube-power]")
{
	TubePowerRig rig; rig.Connect(); rig.Offer();
	CapsuleDepotService work(*rig.capsule);
	REQUIRE(ConsumeTubeExternalPower(rig.source, rig.consumer, 39960, work).error == UtilityPowerError::None);
	const auto rows = CaptureTubePowerConnections();
	const auto physical = ComputeTubePowerDigest(), ledger = ComputeUtilityPowerDigest();
	ResetTubePowerState(); StageLoadedTubePowerConnections(rows);
	REQUIRE(AcceptLoadedTubePowerConnections());
	CHECK(ComputeTubePowerDigest() == physical); CHECK(ComputeUtilityPowerDigest() == ledger);
	auto malformed = rows;
	SECTION("invalid fraction") { malformed[0].usage_remainder_j = 1000; }
	SECTION("lost fractional debt") { malformed[0].usage_remainder_j = 0; }
	SECTION("corrupt whole invoice") { --malformed[0].invoiced_money; }
	SECTION("old commissioning revision") { ++malformed[0].portal_revision; }
	SECTION("foreign source") { malformed[0].producer = UtilityProducerID{999}; }
	SECTION("duplicate source") { malformed.push_back(malformed.front()); }
	SECTION("missing utility corridor") { malformed[0].utility_path.clear(); }
	SECTION("out of bounds portal") { malformed[0].portal = TileIndex{Map::Size()}; }
	StageLoadedTubePowerConnections(malformed);
	CHECK_FALSE(AcceptLoadedTubePowerConnections());
	CHECK(ComputeTubePowerDigest() == physical); CHECK(ComputeUtilityPowerDigest() == ledger);
}

/** New economy-day service and epoch rotation preserve used history/fractions while all unspent energy stays in the shared authority. */
TEST_CASE("Tube paid service rotates utility intervals without duplicate lease or usage", "[transport-tube-power]")
{
	TubePowerRig rig; rig.Connect(100000); rig.Offer(39960);
	CapsuleDepotService work(*rig.capsule);
	REQUIRE(ConsumeTubeExternalPower(rig.source, rig.consumer, 39960, work).error == UtilityPowerError::None);
	{
		AutoRestoreBackup system(_current_company, OWNER_DEITY);
		REQUIRE(BeginUtilityPowerInterval() == UtilityPowerError::None);
	}
	const auto cash = Company::Get(_current_company)->money;
	TimerGameEconomy::date = TimerGameEconomy::Date{101};
	CHECK(PublishTubeExternalPower(rig.source, 2, 74, false) == TubePowerError::None);
	CHECK(Company::Get(_current_company)->money == cash);
	REQUIRE(PublishTubeExternalPower(rig.source, 2, 74) == TubePowerError::None);
	CHECK(Company::Get(_current_company)->money == cash - Money{20});
	CHECK(PublishTubeExternalPower(rig.source, 2, 74) == TubePowerError::Ledger);
	CHECK(GetTubePowerConnection(rig.source)->usage_remainder_j == 960);
	CHECK(GetUtilityPowerProducer(rig.source)->consumed_lifetime_j == 39960);
	CHECK(GetUtilityPowerProducer(rig.source)->generated_lifetime_j == 399600);
	CHECK(ValidateTubePowerConnections(CaptureTubePowerConnections(), GetCurrentMultiLayerProfileSelection()) == TubePowerError::None);
}

/** Insufficient next-day lease cannot publish capacity or advance either saved financial/electrical owner. */
TEST_CASE("Tube next day insufficient lease leaves the generation interval open unchanged", "[transport-tube-power]")
{
	TubePowerRig rig; rig.Connect(); rig.Offer();
	{
		AutoRestoreBackup system(_current_company, OWNER_DEITY);
		REQUIRE(BeginUtilityPowerInterval() == UtilityPowerError::None);
	}
	TimerGameEconomy::date = TimerGameEconomy::Date{101};
	Company::Get(_current_company)->money = Money{19};
	const auto ledger = ComputeUtilityPowerDigest(), physical = ComputeTubePowerDigest();
	CHECK(PublishTubeExternalPower(rig.source, 2, 74) == TubePowerError::Funds);
	CHECK(ComputeUtilityPowerDigest() == ledger); CHECK(ComputeTubePowerDigest() == physical);
	CHECK(Company::Get(_current_company)->money == Money{19});
}

/** The common ledger enforces the paid-source participant boundary; directly supplied generic work cannot skip the service invoice. */
TEST_CASE("Tube paid original source cannot bypass its invoice through direct ledger consumption", "[transport-tube-power]")
{
	TubePowerRig rig; rig.Connect(); rig.Offer();
	CapsuleDepotService work(*rig.capsule);
	const auto proposal = PrepareUtilityMutation(UtilityMutationKind::Consume, {rig.consumer, 1}, 540);
	REQUIRE(proposal.error == UtilityPowerError::None);
	const auto ledger = ComputeUtilityPowerDigest(), invoice = ComputeTubePowerDigest();
	CHECK(CommitUtilityMutation(proposal, &work).error == UtilityPowerError::Refused);
	CHECK(ComputeUtilityPowerDigest() == ledger); CHECK(ComputeTubePowerDigest() == invoice);
	CHECK(ConsumeTubeExternalPower(rig.source, rig.consumer, 540, work).error == UtilityPowerError::None);
	CHECK(GetTubePowerConnection(rig.source)->usage_remainder_j == 540);
}
