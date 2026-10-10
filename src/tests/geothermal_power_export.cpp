/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file geothermal_power_export.cpp Feature exports on actual paid native risers, installed cable and the sole shared electrical owner.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs17-geothermal-energy-and-industry.md Group5
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs17-geothermal-energy-and-industry.md Group7 public signature fixture
 */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../geothermal.h"

// Check the advertised facade before any specialist fixture can introduce
// hidden declarations. Callers must receive complete Heat/utility schemas and
// exact allocation/export signatures from geothermal.h alone.
static_assert(std::is_default_constructible_v<GeothermalHeatAllocation>);
static_assert(std::is_default_constructible_v<GeothermalUtilityLink>);
static_assert(std::is_same_v<decltype(&AllocateVentHeat), UtilityPowerError (*)(uint64_t, std::span<const GeothermalHeatDemand>, bool)>);
static_assert(std::is_same_v<decltype(&PrepareGeothermalPowerExport), GeothermalPowerExportProposal (*)(const GeothermalPowerExportRequest &)>);
static_assert(std::is_same_v<decltype(&ExportGeothermalPower), UtilityMutationReceipt (*)(const GeothermalPowerExportProposal &)>);

#include "geothermal_electrical_fixture.h"
#include "utility_power_fixture.h"
#include "../landscape_cmd.h"
#include "../safeguards.h"

/** Actual paid two-riser fixture with explicit external boundary electricity; this does not supply an operating geothermal generator or transported bootstrap. */
class GeothermalExportRig : public NativeElectricalRelayRig {
public:
	/** @param relay_demand Actual intermediate request, zero when the complete path bypasses it. @param final_demand Actual final requested room. */
	explicit GeothermalExportRig(uint64_t relay_demand = 0, uint64_t final_demand = 600000) : NativeElectricalRelayRig(false, true)
	{
		this->InstallSourceCable(); this->InstallRelayCable();
		AutoRestoreBackup system(_current_company, OWNER_DEITY);
		REQUIRE(AllocateUtilityPower(GetUtilityPowerInterval(), std::array{UtilityPowerRequest{this->source, 500000},
			UtilityPowerRequest{this->relay, relay_demand}, UtilityPowerRequest{this->destination_account, final_demand}}) == UtilityPowerError::None);
	}
	/** @param maximum_j Positive requested upper bound, without a caller-supplied duration. @return Feature request retaining the actual registered source, port and final destination. */
	GeothermalPowerExportRequest Request(uint64_t maximum_j) const
	{
		return {{this->source, GetUtilityPowerInterval()}, this->destination_account, this->source_port, maximum_j};
	}
	/** @param request Complete feature request. @return Actual successful once-only shared settlement receipt. */
	UtilityMutationReceipt Transfer(const GeothermalPowerExportRequest &request) const
	{
		const auto proposal = PrepareGeothermalPowerExport(request); REQUIRE(proposal.error == UtilityPowerError::None);
		const auto receipt = ExportGeothermalPower(proposal); REQUIRE(receipt.error == UtilityPowerError::None);
		CHECK(receipt.quantity_j == proposal.request.quantity_j); return receipt;
	}
};

/** Pure feature discovery and one whole commit preserve original publication/window/source rights and never create an intermediate grant. */
TEST_CASE("Geothermal feature export retains the original metered source once", "[geothermal-power-export]")
{
	GeothermalExportRig rig;
	const auto before = CaptureUtilityPowerState(); const auto digest = ComputeUtilityPowerDigest();
	const auto natural = ComputeGeologyStateDigest(GetGeologyWorldState()), construction = ComputeGeothermalRiserDigest();
	const auto cash = Company::Get(rig.request.owner)->money; const auto random = std::array{_random.state[0], _random.state[1]};
	const auto proposal = PrepareGeothermalPowerExport(rig.Request(500000)); REQUIRE(proposal.error == UtilityPowerError::None);
	CHECK(proposal.request.quantity_j == 399600); REQUIRE(proposal.request.legs.size() == 2);
	CHECK(proposal.request.legs[0].edge == rig.first); CHECK(proposal.request.legs[1].edge == rig.second);
	CHECK(ComputeUtilityPowerDigest() == digest); CHECK(CaptureUtilityPowerState().next_share == before.next_share);
	const auto receipt = ExportGeothermalPower(proposal); REQUIRE(receipt.error == UtilityPowerError::None);
	CHECK(receipt.quantity_j == 399600); CHECK(receipt.operation_sequence == before.operation_sequence + 1);
	const auto after = CaptureUtilityPowerState(); CHECK(after.producers.size() == 1); CHECK(after.next_share == 2);
	CHECK(after.consumers[rig.source.base()].shares.front().transferred_j == 399600);
	CHECK(after.consumers[rig.relay.base()].shares.empty()); CHECK(after.consumers[rig.relay.base()].meter.granted_energy_j == 0);
	const auto &child = after.consumers[rig.destination_account.base()].shares.front();
	CHECK(child.producer == rig.boundary_producer); CHECK(child.parent == UtilityShareID{0}); CHECK(child.allocated_j == 399600);
	CHECK(after.links[rig.first.base()].flow_j == 399600); CHECK(after.links[rig.second.base()].flow_j == 399600);
	CHECK(after.producers.front().reserved_j == before.producers.front().reserved_j);
	CHECK(after.producers.front().meter.generation_energy_j == 1998000);
	CHECK(after.producers.front().generated_lifetime_j == before.producers.front().generated_lifetime_j);
	CHECK(after.producers.front().consumed_lifetime_j == 0); CHECK(after.local_link_revision == before.local_link_revision);
	const auto committed = ComputeUtilityPowerDigest(); CHECK(ExportGeothermalPower(proposal).error == UtilityPowerError::StaleMutation);
	CHECK(ComputeUtilityPowerDigest() == committed); CHECK(Company::Get(rig.request.owner)->money == cash);
	CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == natural); CHECK(ComputeGeothermalRiserDigest() == construction);
	CHECK(std::array{_random.state[0], _random.state[1]} == random);
	CHECK(ValidateUtilityPowerState(after, GetCurrentMultiLayerProfileSelection()) == UtilityPowerError::None);
}

/** Fresh explicit actions share one physical interval allowance; release never restores a spent cable/riser allowance or generates replacement power. */
TEST_CASE("Geothermal feature export shares physical bottlenecks across actions", "[geothermal-power-export]")
{
	GeothermalExportRig rig; rig.Transfer(rig.Request(100000)); rig.Transfer(rig.Request(100000));
	CHECK(CaptureUtilityPowerState().next_share == 2); CHECK(GetUtilityPowerConsumer(rig.destination_account)->shares.size() == 1);
	REQUIRE(CommitUtilityMutation(PrepareUtilityMutation(UtilityMutationKind::ReleaseUnused,
		{rig.destination_account, GetUtilityPowerInterval()}, 50000)).error == UtilityPowerError::None);
	const auto proposal = PrepareGeothermalPowerExport(rig.Request(500000)); REQUIRE(proposal.error == UtilityPowerError::None);
	CHECK(proposal.request.quantity_j == 199600); REQUIRE(ExportGeothermalPower(proposal).error == UtilityPowerError::None);
	CHECK(GetUtilityPowerLink(rig.first)->flow_j == 399600); CHECK(GetUtilityPowerLink(rig.second)->flow_j == 399600);
	const auto digest = ComputeUtilityPowerDigest(); CHECK(PrepareGeothermalPowerExport(rig.Request(1)).error == UtilityPowerError::InsufficientGrant);
	CHECK(ComputeUtilityPowerDigest() == digest); CHECK(GetUtilityPowerProducer(rig.boundary_producer)->reserved_j == 450000);
}

/** Imported continuation starts at its original arriving housing; staged and direct children retain distinct parents and actual use settles the original producer. */
TEST_CASE("Geothermal feature staged relay retains original parent and consumption", "[geothermal-power-export]")
{
	GeothermalExportRig rig(200000);
	auto first = rig.Request(100000); first.destination = rig.relay; rig.Transfer(first);
	GeothermalPowerExportRequest onward{{rig.relay, GetUtilityPowerInterval()}, rig.destination_account, rig.destination, 60000};
	rig.Transfer(onward); rig.Transfer(rig.Request(40000));
	const auto state = CaptureUtilityPowerState(); REQUIRE(state.consumers[rig.destination_account.base()].shares.size() == 2);
	CHECK(state.next_share == 4); CHECK(state.consumers[rig.relay.base()].shares.front().transferred_j == 60000);
	CHECK(state.consumers[rig.destination_account.base()].shares[0].parent == UtilityShareID{0});
	CHECK(state.consumers[rig.destination_account.base()].shares[1].parent == UtilityShareID{1});
	CHECK(state.links[rig.first.base()].flow_j == 140000); CHECK(state.links[rig.second.base()].flow_j == 100000);
	UtilityCompanyWork work(rig.request.owner);
	REQUIRE(CommitUtilityMutation(PrepareUtilityMutation(UtilityMutationKind::Consume,
		{rig.destination_account, GetUtilityPowerInterval()}, 20000), &work).error == UtilityPowerError::None);
	CHECK(GetUtilityPowerProducer(rig.boundary_producer)->consumed_lifetime_j == 20000);
	onward.source_port = rig.upper.port; onward.maximum_j = 1;
	const auto digest = ComputeUtilityPowerDigest(); CHECK(PrepareGeothermalPowerExport(onward).error == UtilityPowerError::InsufficientGrant);
	CHECK(ComputeUtilityPowerDigest() == digest);
	CHECK(ValidateUtilityPowerState(CaptureUtilityPowerState(), GetCurrentMultiLayerProfileSelection()) == UtilityPowerError::None);
}

/** Existing requested room bounds dispatch even when the actual source and installed physical route can carry more. */
TEST_CASE("Geothermal feature export cannot invent final demand", "[geothermal-power-export]")
{
	GeothermalExportRig rig(0, 70000); const auto receipt = rig.Transfer(rig.Request(500000)); CHECK(receipt.quantity_j == 70000);
	CHECK(GetUtilityPowerConsumer(rig.destination_account)->meter.unmet_energy_j == 0);
	const auto digest = ComputeUtilityPowerDigest(); CHECK(PrepareGeothermalPowerExport(rig.Request(1)).error == UtilityPowerError::InsufficientGrant);
	CHECK(ComputeUtilityPowerDigest() == digest); CHECK(GetUtilityPowerConsumer(rig.source)->shares.front().transferred_j == 70000);
}

/** Optional unavailable references select only a separately paid installed bypass; discovery remains deterministic and cannot replace a missing riser. */
TEST_CASE("Geothermal feature export uses actual selected cable alternatives", "[geothermal-power-export]")
{
	GeothermalExportRig rig;
	// These three additional paid cells connect around the excluded straight
	// relay cell, preserving the same original two physical transitions.
	for (uint x = 24; x <= 26; ++x) REQUIRE(ElectricalCableCommand(TileXYL(x, 19, MapLayer::Underground)).Succeeded());
	auto request = rig.Request(100000);
	const auto blocked = GetGeothermalUtilityLink(TileXYL(25, 20, MapLayer::Underground), UtilityEnergyDomain::Electricity)->id;
	request.excluded.push_back(blocked); const auto proposal = PrepareGeothermalPowerExport(request);
	REQUIRE(proposal.error == UtilityPowerError::None); REQUIRE(proposal.request.legs.size() == 2);
	CHECK(proposal.request.legs[1].cells.size() == 14);
	CHECK(std::ranges::find(proposal.request.legs[1].cells, blocked) == proposal.request.legs[1].cells.end());
	CHECK(PrepareGeothermalPowerExport(request).request == proposal.request);
	REQUIRE(ExportGeothermalPower(proposal).error == UtilityPowerError::None);
	request.excluded.push_back(rig.second); const auto digest = ComputeUtilityPowerDigest();
	CHECK(PrepareGeothermalPowerExport(request).error == UtilityPowerError::InvalidTopology); CHECK(ComputeUtilityPowerDigest() == digest);
}

/** Every feature-level reference, clock and actor refusal precedes endpoint publication and leaves the native materials, funds and randomness untouched. */
TEST_CASE("Geothermal feature export rejects unavailable requests without mutation", "[geothermal-power-export]")
{
	GeothermalExportRig rig; auto request = rig.Request(100000);
	SECTION("unknown source") { request.grant.consumer = UtilityConsumerID{UINT32_MAX}; }
	SECTION("unknown destination") { request.destination = UtilityConsumerID{UINT32_MAX}; }
	SECTION("self dispatch") { request.destination = rig.source; }
	SECTION("wrong source plane") { request.source_port = rig.destination; }
	SECTION("no source cell") { request.source_port = INVALID_TILE; }
	SECTION("zero quantity") { request.maximum_j = 0; }
	SECTION("excess quantity") { request.maximum_j = MAX_UTILITY_ACCOUNT_ENERGY_J + 1; }
	SECTION("old grant") { ++request.grant.interval; }
	SECTION("too many exclusions") { request.excluded.resize(MAX_UTILITY_INTERVAL_ACCOUNTS + 1); }
	SECTION("Heat cannot use electrical export") {
		const auto heat = RegisterUtilityHeatProducer(rig.request.owner); REQUIRE(heat.has_value());
		const auto consumer = RegisterUtilityHeatConsumer(rig.request.owner, UtilityHeatUse::DirectRecipe, 100000, std::array{*heat}); REQUIRE(consumer.has_value());
		request.grant.consumer = *consumer;
	}
	SECTION("foreign actor") { _current_company = CompanyID{1}; }
	SECTION("editor") { _game_mode = GameMode::Editor; }
	SECTION("pre 1980") { TimerGameCalendar::date = TimerGameCalendar::ConvertYMDToDate(TimerGameCalendar::Year{1979}, 11, 31); }
	SECTION("closed original portal") { REQUIRE(ChangeLayerPortalConnection(rig.upper.portal, rig.request.owner, false, true) == LayerPortalWorkError::None); }
	SECTION("removed original portal") { REQUIRE(Command<Commands::LandscapeClear>::Do(DoCommandFlag::Execute, rig.upper.portal).Succeeded()); }
	SECTION("missing relay cable") { REQUIRE(ElectricalCableCommand(TileXYL(25, 20, MapLayer::Underground), UtilityEnergyDomain::Electricity, true).Succeeded()); }
	const auto digest = ComputeUtilityPowerDigest(), natural = ComputeGeologyStateDigest(GetGeologyWorldState()), construction = ComputeGeothermalRiserDigest();
	const auto cash = Company::Get(rig.request.owner)->money; const auto random = std::array{_random.state[0], _random.state[1]};
	const auto proposal = PrepareGeothermalPowerExport(request); CHECK(proposal.error != UtilityPowerError::None);
	CHECK(ExportGeothermalPower(proposal).error != UtilityPowerError::None); CHECK(ComputeUtilityPowerDigest() == digest);
	CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == natural); CHECK(ComputeGeothermalRiserDigest() == construction);
	CHECK(Company::Get(rig.request.owner)->money == cash); CHECK(std::array{_random.state[0], _random.state[1]} == random);
}

/** Physical construction alone supplies no electrical allocation; only the common system-owned interval allocation may authorize a feature export. */
TEST_CASE("Geothermal feature export requires original shared allocation", "[geothermal-power-export]")
{
	NativeElectricalRelayRig rig(false, true); rig.InstallSourceCable(); rig.InstallRelayCable();
	const auto digest = ComputeUtilityPowerDigest();
	const auto proposal = PrepareGeothermalPowerExport({{rig.source, GetUtilityPowerInterval()}, rig.destination_account, rig.source_port, 100000});
	CHECK(proposal.error == UtilityPowerError::AllocationRequired); CHECK(ExportGeothermalPower(proposal).error != UtilityPowerError::None);
	CHECK(ComputeUtilityPowerDigest() == digest); CHECK(GetUtilityPowerLink(rig.first)->flow_j == 0); CHECK(GetUtilityPowerLink(rig.second)->flow_j == 0);
}

/** A prepared action is a sealed shared proposal; changing its fields or actual participants cannot authorize even one partial crossing. */
TEST_CASE("Geothermal feature export revalidates sealed whole actions", "[geothermal-power-export]")
{
	GeothermalExportRig rig; auto proposal = PrepareGeothermalPowerExport(rig.Request(100000)); REQUIRE(proposal.error == UtilityPowerError::None);
	SECTION("modified quantity") { ++proposal.request.quantity_j; }
	SECTION("modified destination") { proposal.request.destination = rig.relay; }
	SECTION("modified source port") { proposal.request.source_port = rig.request.port; }
	SECTION("modified physical path") { proposal.request.legs.pop_back(); }
	SECTION("changed actor") { _current_company = CompanyID{1}; }
	SECTION("changed date") { TimerGameCalendar::date = TimerGameCalendar::ConvertYMDToDate(TimerGameCalendar::Year{1979}, 11, 31); }
	SECTION("removed physical cable") { REQUIRE(ElectricalCableCommand(TileXYL(25, 20, MapLayer::Underground), UtilityEnergyDomain::Electricity, true).Succeeded()); }
	const auto digest = ComputeUtilityPowerDigest(); CHECK(ExportGeothermalPower(proposal).error != UtilityPowerError::None);
	CHECK(ComputeUtilityPowerDigest() == digest); CHECK(GetUtilityPowerLink(rig.first)->flow_j == 0);
	CHECK(GetUtilityPowerLink(rig.second)->flow_j == 0); CHECK(GetUtilityPowerConsumer(rig.destination_account)->meter.granted_energy_j == 0);
}

/** A direct paid elevator carries the same original electricity to Surface without installing, excavating or allocating an intermediate Underground network. */
TEST_CASE("Geothermal feature direct elevator leaves Underground untouched", "[geothermal-power-export]")
{
	NativeRiserRig rig(true); REQUIRE(rig.Fund().Succeeded()); rig.DeliverBill(); rig.Tick(296);
	const auto producer = RegisterUtilityPowerProducer(rig.request.owner, MapLayer::DeepUnderground, UtilityGenerationKind::External); REQUIRE(producer.has_value());
	REQUIRE(PublishUtilityGeneration(*producer, GetUtilityPowerInterval(), 1000000, 74) == UtilityPowerError::None);
	const auto source = RegisterUtilityPowerConsumer(rig.request.owner, MapLayer::DeepUnderground, UtilityConsumerEffect::AssemblyWork, 1000000, std::array{*producer});
	const auto target = RegisterUtilityPowerConsumer(rig.request.owner, MapLayer::Surface, UtilityConsumerEffect::AssemblyWork, 1000000, {});
	REQUIRE(source.has_value()); REQUIRE(target.has_value()); REQUIRE(RegisterGeothermalRiserLink(rig.Riser().index, *source, *target).has_value());
	const auto port = TileAddWrap(rig.request.port, 0, -1); REQUIRE(ElectricalCableCommand(port).Succeeded()); REQUIRE(ElectricalCableCommand(rig.request.port).Succeeded());
	{
		AutoRestoreBackup system(_current_company, OWNER_DEITY);
		REQUIRE(AllocateUtilityPower(GetUtilityPowerInterval(), std::array{UtilityPowerRequest{*source, 500000}, UtilityPowerRequest{*target, 500000}}) == UtilityPowerError::None);
	}
	const auto underground = MultiLayerMap::TileInLayer(rig.request.portal, MapLayer::Underground);
	const auto native = ElectricalNativeFloor(underground); const auto natural = ComputeGeologyStateDigest(GetGeologyWorldState());
	const auto proposal = PrepareGeothermalPowerExport({{*source, GetUtilityPowerInterval()}, *target, port, 500000}); REQUIRE(proposal.error == UtilityPowerError::None);
	CHECK(proposal.request.legs.size() == 1); CHECK(proposal.request.quantity_j == 399600); REQUIRE(ExportGeothermalPower(proposal).error == UtilityPowerError::None);
	CHECK(ElectricalNativeFloor(underground) == native); CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == natural);
	CHECK_FALSE(GetGeothermalUtilityLink(underground, UtilityEnergyDomain::Electricity).has_value());
	CHECK(GetUtilityPowerConsumer(*target)->shares.front().producer == *producer);
}
