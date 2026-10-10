/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file utility_power_transfer.cpp Actual whole paid-path settlement and descendant lifecycle on native funded risers and installed cable.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs12-solar-farms-mirrors-and-eti.md Group8C.2
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs12-solar-farms-mirrors-and-eti.md Group8C.3
 */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "geothermal_electrical_fixture.h"
#include "utility_power_fixture.h"
#include "../utility_power_transfer.h"
#include "../utility_power_lineage.h"
#include "../landscape_cmd.h"
#include "../economy_func.h"
#include "../safeguards.h"

/** Actual paid relay plus one explicit boundary publication; no geothermal conversion or transported bootstrap is supplied by this fixture. */
class UtilityPathTransferRig : public NativeElectricalRelayRig {
public:
	/** @param relay_demand Existing intermediate requested room, zero for whole-route bypass. @param final_demand Existing final requested room, independently bounded by the shared consumer. */
	explicit UtilityPathTransferRig(uint64_t relay_demand = 0, uint64_t final_demand = 600000) : NativeElectricalRelayRig(false, true)
	{
		this->InstallSourceCable(); this->InstallRelayCable();
		AutoRestoreBackup system(_current_company, OWNER_DEITY);
		REQUIRE(AllocateUtilityPower(GetUtilityPowerInterval(), std::array{UtilityPowerRequest{this->source, 500000},
			UtilityPowerRequest{this->relay, relay_demand}, UtilityPowerRequest{this->destination_account, final_demand}}) == UtilityPowerError::None);
	}
	/** @param from Existing sending reservation. @param to Existing final request. @param port Explicit installed native source/arriving housing. @param quantity Exact positive whole request. @return Actual native path references with a deliberately different quote duration, which cannot change the source's saved74-tick capacity. */
	UtilityPathTransferRequest Request(UtilityConsumerID from, UtilityConsumerID to, TileIndex port, uint64_t quantity) const
	{
		const auto path = FindGeothermalElectricalPath(from, to, port, 7400); REQUIRE(path.has_value());
		UtilityPathTransferRequest request{{from, GetUtilityPowerInterval()}, to, port, quantity};
		// Translate references only; quoted capacity is deliberately omitted
		// because original publication, not a caller's duration, governs export.
		for (const auto &leg : path->legs) request.legs.push_back({leg.riser, leg.inlet.cells});
		return request;
	}
	/** @param quantity Exact whole Deep-to-Surface quantity. @return Complete actual two-riser route; the relay receives no invented demand. */
	UtilityPathTransferRequest Request(uint64_t quantity) const { return this->Request(this->source, this->destination_account, this->source_port, quantity); }
	/** @param request Exact existing native route and quantity. @return Successful once-only complete shared settlement receipt. */
	UtilityMutationReceipt Transfer(const UtilityPathTransferRequest &request) const
	{
		const auto proposal = PrepareUtilityPathTransfer(request); REQUIRE(proposal.error == UtilityPowerError::None);
		const auto receipt = CommitUtilityPathTransfer(proposal); REQUIRE(receipt.error == UtilityPowerError::None); return receipt;
	}
	/** Commit one real first import, its later continuation, a distinct direct arrival and20-kJ actual paid work; all history remains in the shared owner. */
	void BranchAndUse() const
	{
		this->Transfer(this->Request(this->source, this->relay, this->source_port, 100000));
		this->Transfer(this->Request(this->relay, this->destination_account, this->destination, 60000));
		this->Transfer(this->Request(40000));
		UtilityCompanyWork work(this->request.owner);
		REQUIRE(CommitUtilityMutation(PrepareUtilityMutation(UtilityMutationKind::Consume, {this->destination_account, GetUtilityPowerInterval()}, 20000), &work).error == UtilityPowerError::None);
	}
};

/** Every real crossing settles together while an empty-demand relay remains a topology reference and original generation/money/geology stay unchanged. */
TEST_CASE("Paid utility whole route commits once using the original publication window", "[utility-power-path-transfer]")
{
	UtilityPathTransferRig rig;
	const auto request = rig.Request(100000); const auto before = CaptureUtilityPowerState();
	const auto digest = ComputeUtilityPowerDigest(), construction = ComputeGeothermalRiserDigest(), natural = ComputeGeologyStateDigest(GetGeologyWorldState());
	const auto cash = Company::Get(rig.request.owner)->money; const auto random = std::array{_random.state[0], _random.state[1]};
	const auto offer = GetUtilityPathTransferOffer(rig.Request(500000));
	REQUIRE(offer.error == UtilityPowerError::None); CHECK(offer.quantity_j == 399600);
	CHECK(PrepareUtilityPathTransfer(rig.Request(399601)).error == UtilityPowerError::InsufficientGrant);
	const auto proposal = PrepareUtilityPathTransfer(request); REQUIRE(proposal.error == UtilityPowerError::None);
	CHECK(ComputeUtilityPowerDigest() == digest); CHECK(CaptureUtilityPowerState().next_share == before.next_share);
	const auto receipt = CommitUtilityPathTransfer(proposal); REQUIRE(receipt.error == UtilityPowerError::None);
	CHECK(receipt.quantity_j == 100000); CHECK(receipt.operation_sequence == before.operation_sequence + 1);
	const auto after = CaptureUtilityPowerState();
	CHECK(after.next_share == 2); CHECK(after.producers.size() == 1);
	CHECK(after.consumers[rig.source.base()].shares.front().transferred_j == 100000);
	CHECK(after.consumers[rig.relay.base()].shares.empty()); CHECK(after.consumers[rig.relay.base()].meter.granted_energy_j == 0);
	const auto &child = after.consumers[rig.destination_account.base()].shares.front();
	CHECK(child.producer == rig.boundary_producer); CHECK(child.parent == UtilityShareID{0}); CHECK(child.id == UtilityShareID{1});
	CHECK(child.allocated_j == 100000); CHECK(child.imported_link == rig.second); CHECK(child.segment.size() == 16);
	CHECK(after.links[rig.first.base()].flow_j == 100000); CHECK(after.links[rig.second.base()].flow_j == 100000);
	CHECK(after.producers.front().reserved_j == 500000); CHECK(after.producers.front().meter.generation_energy_j == 1998000);
	CHECK(after.producers.front().generated_lifetime_j == before.producers.front().generated_lifetime_j); CHECK(after.producers.front().consumed_lifetime_j == 0);
	CHECK(after.local_link_revision == before.local_link_revision);
	CHECK(ValidateUtilityPowerState(after, GetCurrentMultiLayerProfileSelection()) == UtilityPowerError::None);
	const auto committed = ComputeUtilityPowerDigest(); CHECK(CommitUtilityPathTransfer(proposal).error == UtilityPowerError::StaleMutation);
	CHECK(ComputeUtilityPowerDigest() == committed); CHECK(Company::Get(rig.request.owner)->money == cash);
	CHECK(ComputeGeothermalRiserDigest() == construction); CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == natural);
	CHECK(std::array{_random.state[0], _random.state[1]} == random);
}

/** A paid rate-based edge never becomes the older fixed-joule one-hop role after nonzero flow; otherwise unsigned residual subtraction could admit a cable-free import. */
TEST_CASE("Paid utility edges require the complete path transaction after actual flow", "[utility-power-path-transfer]")
{
	UtilityPathTransferRig rig(200000);
	CHECK(PrepareUtilityMutation(UtilityMutationKind::TransferLayer, {rig.source, GetUtilityPowerInterval()}, 1, rig.relay, rig.first).error == UtilityPowerError::InvalidTopology);
	rig.Transfer(rig.Request(100000));
	const auto digest = ComputeUtilityPowerDigest(), sequence = CaptureUtilityPowerState().operation_sequence;
	const auto proposal = PrepareUtilityMutation(UtilityMutationKind::TransferLayer, {rig.source, GetUtilityPowerInterval()}, 1, rig.relay, rig.first);
	CHECK(proposal.error == UtilityPowerError::InvalidTopology);
	CHECK(ComputeUtilityPowerDigest() == digest); CHECK(CaptureUtilityPowerState().operation_sequence == sequence);
	CHECK(GetUtilityPowerConsumer(rig.relay)->shares.empty()); CHECK(GetUtilityPowerLink(rig.first)->flow_j == 100000);
	CHECK(ValidateUtilityPowerState(CaptureUtilityPowerState(), GetCurrentMultiLayerProfileSelection()) == UtilityPowerError::None);
}

/** Separate actual actions share every original cell/edge residual, including released rights; exact same arrivals merge without issuing another identity. */
TEST_CASE("Paid utility whole transfers share one physical interval allowance", "[utility-power-path-transfer]")
{
	UtilityPathTransferRig rig;
	rig.Transfer(rig.Request(100000)); rig.Transfer(rig.Request(100000));
	CHECK(CaptureUtilityPowerState().next_share == 2); CHECK(GetUtilityPowerConsumer(rig.destination_account)->shares.size() == 1);
	CHECK(GetUtilityPowerConsumer(rig.destination_account)->shares.front().allocated_j == 200000);
	const auto release = PrepareUtilityMutation(UtilityMutationKind::ReleaseUnused, {rig.destination_account, GetUtilityPowerInterval()}, 50000);
	REQUIRE(CommitUtilityMutation(release).error == UtilityPowerError::None);
	const auto offer = GetUtilityPathTransferOffer(rig.Request(500000)); REQUIRE(offer.error == UtilityPowerError::None); CHECK(offer.quantity_j == 199600);
	rig.Transfer(rig.Request(offer.quantity_j));
	CHECK(GetUtilityPowerLink(rig.first)->flow_j == 399600); CHECK(GetUtilityPowerLink(rig.second)->flow_j == 399600);
	const auto digest = ComputeUtilityPowerDigest(); CHECK(GetUtilityPathTransferOffer(rig.Request(1)).error == UtilityPowerError::InsufficientGrant);
	CHECK(PrepareUtilityPathTransfer(rig.Request(1)).error == UtilityPowerError::InsufficientGrant); CHECK(ComputeUtilityPowerDigest() == digest);
	CHECK(GetUtilityPowerConsumer(rig.source)->shares.front().transferred_j == 399600);
	CHECK(GetUtilityPowerProducer(rig.boundary_producer)->reserved_j == 450000);
	CHECK(ValidateUtilityPowerState(CaptureUtilityPowerState(), GetCurrentMultiLayerProfileSelection()) == UtilityPowerError::None);
}

/** Staged continuation retains its exact imported parent while direct arrival remains distinct; original upstream flow is not counted again during use/re-export. */
TEST_CASE("Paid utility imported continuation retains exact original parent and settled use", "[utility-power-path-transfer]")
{
	UtilityPathTransferRig rig(200000);
	rig.Transfer(rig.Request(rig.source, rig.relay, rig.source_port, 100000));
	rig.Transfer(rig.Request(rig.relay, rig.destination_account, rig.destination, 60000));
	rig.Transfer(rig.Request(40000));
	const auto state = CaptureUtilityPowerState(); REQUIRE(state.consumers[rig.destination_account.base()].shares.size() == 2);
	CHECK(state.next_share == 4); CHECK(state.consumers[rig.relay.base()].shares.front().transferred_j == 60000);
	CHECK(state.consumers[rig.destination_account.base()].shares[0].parent == UtilityShareID{0});
	CHECK(state.consumers[rig.destination_account.base()].shares[1].parent == UtilityShareID{1});
	CHECK(state.links[rig.first.base()].flow_j == 140000); CHECK(state.links[rig.second.base()].flow_j == 100000);
	CHECK(state.producers.front().reserved_j == 500000);
	UtilityCompanyWork work(rig.request.owner); const auto cash = Company::Get(rig.request.owner)->money;
	const auto use = PrepareUtilityMutation(UtilityMutationKind::Consume, {rig.destination_account, GetUtilityPowerInterval()}, 20000);
	REQUIRE(CommitUtilityMutation(use, &work).error == UtilityPowerError::None);
	CHECK(Company::Get(rig.request.owner)->money == cash - Money{5});
	CHECK(GetUtilityPowerProducer(rig.boundary_producer)->consumed_lifetime_j == 20000); CHECK(GetUtilityPowerProducer(rig.boundary_producer)->reserved_j == 480000);
	CHECK(GetUtilityPowerLink(rig.first)->flow_j == 140000); CHECK(GetUtilityPowerLink(rig.second)->flow_j == 100000);
	CHECK(ValidateUtilityPowerState(CaptureUtilityPowerState(), GetCurrentMultiLayerProfileSelection()) == UtilityPowerError::None);
	// An installed alternative source cell cannot relocate a retained import
	// away from the original arriving housing merely because IDs still agree.
	const auto wrong_port = TileXYL(21, 20, MapLayer::Underground);
	const auto digest = ComputeUtilityPowerDigest();
	CHECK(PrepareUtilityPathTransfer(rig.Request(rig.relay, rig.destination_account, wrong_port, 1)).error == UtilityPowerError::InsufficientGrant);
	CHECK(ComputeUtilityPowerDigest() == digest);
}

/** Existing final requested room constrains the whole action even when source and every paid resource could carry more. */
TEST_CASE("Paid utility transfer cannot invent destination demand", "[utility-power-path-transfer]")
{
	UtilityPathTransferRig rig(0, 70000);
	const auto offer = GetUtilityPathTransferOffer(rig.Request(100000)); REQUIRE(offer.error == UtilityPowerError::None); CHECK(offer.quantity_j == 70000);
	const auto digest = ComputeUtilityPowerDigest(); CHECK(PrepareUtilityPathTransfer(rig.Request(70001)).error == UtilityPowerError::InsufficientGrant);
	CHECK(ComputeUtilityPowerDigest() == digest); rig.Transfer(rig.Request(70000));
	CHECK(GetUtilityPowerConsumer(rig.destination_account)->meter.unmet_energy_j == 0);
	CHECK(GetUtilityPathTransferOffer(rig.Request(1)).error == UtilityPowerError::InsufficientGrant);
}

/** Two actual original publications share one installed route; equal windows split exact parents while mixed windows cannot multiply physical capacity. */
TEST_CASE("Paid utility original producers share aggregate path capacity", "[utility-power-path-transfer]")
{
	NativeElectricalRelayRig rig(false, true); rig.InstallSourceCable(); rig.InstallRelayCable();
	uint32_t second_ticks = 74; uint64_t second_rate = 100000;
	SECTION("two compatible original windows") {}
	SECTION("different original windows") { second_ticks = 37; second_rate = 200000; }
	const auto first = RegisterUtilityPowerProducer(rig.request.owner, MapLayer::DeepUnderground, UtilityGenerationKind::External);
	const auto second = RegisterUtilityPowerProducer(rig.request.owner, MapLayer::DeepUnderground, UtilityGenerationKind::External);
	REQUIRE(first.has_value()); REQUIRE(second.has_value());
	REQUIRE(PublishUtilityGeneration(*first, GetUtilityPowerInterval(), 100000, 74) == UtilityPowerError::None);
	REQUIRE(PublishUtilityGeneration(*second, GetUtilityPowerInterval(), second_rate, second_ticks) == UtilityPowerError::None);
	const auto source = RegisterUtilityPowerConsumer(rig.request.owner, MapLayer::DeepUnderground, UtilityConsumerEffect::AssemblyWork, 1000000, std::array{*first, *second});
	REQUIRE(source.has_value()); REQUIRE(RemoveUtilityPowerLink(rig.first) == UtilityPowerError::None);
	const auto edge = RegisterGeothermalRiserLink(rig.Riser().index, *source, rig.relay); REQUIRE(edge.has_value()); rig.first = *edge;
	{
		AutoRestoreBackup system(_current_company, OWNER_DEITY);
		REQUIRE(AllocateUtilityPower(GetUtilityPowerInterval(), std::array{UtilityPowerRequest{*source, 500000}, UtilityPowerRequest{rig.relay, 0},
			UtilityPowerRequest{rig.destination_account, 600000}}) == UtilityPowerError::None);
	}
	const auto path = FindGeothermalElectricalPath(*source, rig.destination_account, rig.source_port, 74); REQUIRE(path.has_value());
	UtilityPathTransferRequest request{{*source, GetUtilityPowerInterval()}, rig.destination_account, rig.source_port, 500000};
	for (const auto &leg : path->legs) request.legs.push_back({leg.riser, leg.inlet.cells}); // Carry only actual original physical references into the shared request.
	const auto offer = GetUtilityPathTransferOffer(request); REQUIRE(offer.error == UtilityPowerError::None);
	CHECK(offer.quantity_j == (second_ticks == 74 ? 399600 : 199800));
	request.quantity_j = offer.quantity_j;
	REQUIRE(CommitUtilityPathTransfer(PrepareUtilityPathTransfer(request)).error == UtilityPowerError::None);
	const auto &children = GetUtilityPowerConsumer(rig.destination_account)->shares;
	CHECK(children.size() == (second_ticks == 74 ? 2 : 1)); CHECK(children.front().producer == *first); CHECK(children.front().allocated_j == 199800);
	if (second_ticks == 74) {
		CHECK(children.back().producer == *second); CHECK(children.back().allocated_j == 199800);
		CHECK(children.front().parent != children.back().parent);
	} // Matching original durations allow both exact parents, without issuing another generator or independent path allowance.
	CHECK(GetUtilityPowerLink(rig.first)->flow_j == offer.quantity_j); CHECK(GetUtilityPowerLink(rig.second)->flow_j == offer.quantity_j);
	request.quantity_j = 1; const auto digest = ComputeUtilityPowerDigest();
	CHECK(GetUtilityPathTransferOffer(request).error == UtilityPowerError::InsufficientGrant); CHECK(ComputeUtilityPowerDigest() == digest);
	CHECK(GetUtilityPowerProducer(*first)->reserved_j == 199800); CHECK(GetUtilityPowerProducer(*second)->reserved_j == 199800);
	CHECK(GetUtilityPowerProducer(rig.boundary_producer)->reserved_j == 0);
	CHECK(ValidateUtilityPowerState(CaptureUtilityPowerState(), GetCurrentMultiLayerProfileSelection()) == UtilityPowerError::None);
}

/** Source release cannot be reassigned by a physical path with more room; an open electrical epoch cannot produce any transfer proposal. */
TEST_CASE("Paid utility transfer requires allocated original free quantities", "[utility-power-path-transfer]")
{
	SECTION("released source allowance stays unavailable") {
		UtilityPathTransferRig rig;
		REQUIRE(CommitUtilityMutation(PrepareUtilityMutation(UtilityMutationKind::ReleaseUnused, {rig.source, GetUtilityPowerInterval()}, 350000)).error == UtilityPowerError::None);
		const auto offer = GetUtilityPathTransferOffer(rig.Request(500000)); REQUIRE(offer.error == UtilityPowerError::None); CHECK(offer.quantity_j == 150000);
		rig.Transfer(rig.Request(150000)); CHECK(GetUtilityPowerProducer(rig.boundary_producer)->reserved_j == 150000);
		CHECK(GetUtilityPathTransferOffer(rig.Request(1)).error == UtilityPowerError::InsufficientGrant);
	}
	SECTION("publication without allocation is insufficient") {
		NativeElectricalRelayRig rig(false, true); rig.InstallSourceCable(); rig.InstallRelayCable();
		const auto path = FindGeothermalElectricalPath(rig.source, rig.destination_account, rig.source_port, 74); REQUIRE(path.has_value());
		UtilityPathTransferRequest request{{rig.source, GetUtilityPowerInterval()}, rig.destination_account, rig.source_port, 1};
		for (const auto &leg : path->legs) request.legs.push_back({leg.riser, leg.inlet.cells}); // An installed path supplies no grant or allocation phase by itself.
		const auto digest = ComputeUtilityPowerDigest(); CHECK(PrepareUtilityPathTransfer(request).error == UtilityPowerError::AllocationRequired);
		CHECK(GetUtilityPathTransferOffer(request).error == UtilityPowerError::AllocationRequired); CHECK(ComputeUtilityPowerDigest() == digest);
	}
}

/** Modified references, amounts or sequencing never debit any endpoint or spend a shared operation receipt. */
TEST_CASE("Paid utility whole proposal rejects modification and intervening operations", "[utility-power-path-transfer]")
{
	UtilityPathTransferRig rig; auto proposal = PrepareUtilityPathTransfer(rig.Request(100000)); REQUIRE(proposal.error == UtilityPowerError::None);
	SECTION("quantity") { ++proposal.request.quantity_j; }
	SECTION("source port") { proposal.request.source_port = rig.request.port; }
	SECTION("ordered cable") { std::ranges::reverse(proposal.request.legs.back().cells); }
	SECTION("paid crossing") { proposal.request.legs.back().edge = rig.first; }
	SECTION("final request") { proposal.request.destination = rig.relay; }
	SECTION("epoch") { ++proposal.request.grant.interval; }
	SECTION("owner") { proposal.owner = CompanyID{1}; }
	SECTION("shared sequence") { REQUIRE(CommitUtilityMutation(PrepareUtilityMutation(UtilityMutationKind::ReleaseUnused, {rig.source, GetUtilityPowerInterval()}, 1)).error == UtilityPowerError::None); }
	const auto digest = ComputeUtilityPowerDigest(); const auto sequence = CaptureUtilityPowerState().operation_sequence;
	CHECK(CommitUtilityPathTransfer(proposal).error == UtilityPowerError::StaleMutation);
	CHECK(ComputeUtilityPowerDigest() == digest); CHECK(CaptureUtilityPowerState().operation_sequence == sequence);
	CHECK(GetUtilityPowerLink(rig.first)->flow_j == 0); CHECK(GetUtilityPowerLink(rig.second)->flow_j == 0);
}

/** Incomplete or forged installed paths fail as complete requests before any physical or accounting write. */
TEST_CASE("Paid utility whole preparation rejects invalid native participants", "[utility-power-path-transfer]")
{
	UtilityPathTransferRig rig; auto request = rig.Request(100000);
	SECTION("no paid crossings") { request.legs.clear(); }
	SECTION("missing last crossing") { request.legs.pop_back(); }
	SECTION("repeated crossing") { request.legs.push_back(request.legs.front()); }
	SECTION("reversed cable") { std::ranges::reverse(request.legs.back().cells); }
	SECTION("missing intervening cable") { request.legs.back().cells.erase(request.legs.back().cells.begin() + 5); }
	SECTION("foreign cell identity") { request.legs.back().cells[5] = UtilityLinkID{UINT32_MAX}; }
	SECTION("missing source port") { request.source_port = INVALID_TILE; }
	SECTION("wrong source plane") { request.source_port = rig.destination; }
	SECTION("old epoch") { --request.grant.interval; }
	SECTION("zero quantity") { request.quantity_j = 0; }
	SECTION("oversized quantity") { request.quantity_j = MAX_UTILITY_ACCOUNT_ENERGY_J + 1; }
	SECTION("calendar lock") { TimerGameCalendar::date = TimerGameCalendar::ConvertYMDToDate(TimerGameCalendar::Year{1979}, 11, 31); }
	SECTION("foreign actor") { _current_company = CompanyID{1}; }
	SECTION("editor cannot settle paid export") { _game_mode = GameMode::Editor; }
	SECTION("Heat cannot use a paid electrical route") {
		const auto heat = RegisterUtilityHeatProducer(rig.request.owner); REQUIRE(heat.has_value());
		const auto consumer = RegisterUtilityHeatConsumer(rig.request.owner, UtilityHeatUse::DirectRecipe, 100000, std::array{*heat}); REQUIRE(consumer.has_value());
		request.grant.consumer = *consumer;
	}
	const auto digest = ComputeUtilityPowerDigest(), construction = ComputeGeothermalRiserDigest(); const auto cash = Company::Get(rig.request.owner)->money;
	CHECK(PrepareUtilityPathTransfer(request).error != UtilityPowerError::None); CHECK(GetUtilityPathTransferOffer(request).error != UtilityPowerError::None);
	CHECK(ComputeUtilityPowerDigest() == digest); CHECK(ComputeGeothermalRiserDigest() == construction); CHECK(Company::Get(rig.request.owner)->money == cash);
}

/** Actual readiness is checked again at commit even when the native calendar changed without a utility operation; physical loss between calls cannot partially export. */
TEST_CASE("Paid utility whole commit revalidates actual readiness", "[utility-power-path-transfer]")
{
	UtilityPathTransferRig rig; const auto proposal = PrepareUtilityPathTransfer(rig.Request(100000)); REQUIRE(proposal.error == UtilityPowerError::None);
	SECTION("calendar changed without ledger sequence") { TimerGameCalendar::date = TimerGameCalendar::ConvertYMDToDate(TimerGameCalendar::Year{1979}, 11, 31); }
	SECTION("native cable removed before any import") { REQUIRE(ElectricalCableCommand(TileXYL(25, 20, MapLayer::Underground), UtilityEnergyDomain::Electricity, true).Succeeded()); }
	SECTION("original paid portal sealed before any import") { REQUIRE(ChangeLayerPortalConnection(rig.upper.portal, rig.request.owner, false, true) == LayerPortalWorkError::None); }
	const auto digest = ComputeUtilityPowerDigest(); const auto sequence = CaptureUtilityPowerState().operation_sequence;
	CHECK(CommitUtilityPathTransfer(proposal).error != UtilityPowerError::None); CHECK(ComputeUtilityPowerDigest() == digest);
	CHECK(CaptureUtilityPowerState().operation_sequence == sequence); CHECK(GetUtilityPowerLink(rig.first)->flow_j == 0); CHECK(GetUtilityPowerLink(rig.second)->flow_j == 0);
}

/** Released roots still spent their original publication; free-balance equality alone cannot admit a forged second set of source promises. */
TEST_CASE("Utility original root allocations include released promises", "[utility-power-path-transfer]")
{
	UtilityPathTransferRig rig; auto state = CaptureUtilityPowerState();
	state.consumers[rig.source.base()].shares.front().released_j = 500000; state.producers.front().reserved_j = 1500000;
	// Three individually plausible500-kJ promises leave1.5MJ free, fitting
	// the1.998-MJ source only if its already released500-kJ root is concealed.
	for (uint32_t index = 0; index < 3; ++index) {
		auto consumer = state.consumers[rig.source.base()]; consumer.meter.consumer_id = UtilityConsumerID{state.next_consumer++};
		consumer.shares.front().released_j = 0; consumer.shares.front().id = UtilityShareID{state.next_share++}; state.consumers.push_back(std::move(consumer));
	}
	CHECK(ValidateUtilityPowerState(state, GetCurrentMultiLayerProfileSelection()) == UtilityPowerError::InvalidQuantity);
	CHECK(GetUtilityPowerProducer(rig.boundary_producer)->reserved_j == 500000); CHECK(CaptureUtilityPowerState().next_share == 1);
}

/** Loss in any original branch/whole route expires only affected free descendants, preserving exact upstream debits, use and spent flow. */
TEST_CASE("Paid utility original topology loss expires complete descendant branches", "[utility-power-path-lifecycle]")
{
	UtilityPathTransferRig rig(200000); rig.BranchAndUse();
	bool upstream = false;
	SECTION("first original paid edge") { upstream = true; REQUIRE(ChangeLayerPortalConnection(rig.request.portal, rig.request.owner, false, true) == LayerPortalWorkError::None); }
	SECTION("first original source cable") { upstream = true; REQUIRE(ElectricalCableCommand(rig.source_port, UtilityEnergyDomain::Electricity, true).Succeeded()); }
	SECTION("first original machinery") { upstream = true; REQUIRE(Command<Commands::LandscapeClear>::Do(DoCommandFlag::Execute, rig.request.tile).Succeeded()); }
	SECTION("original receiving housing") { upstream = true; REQUIRE(Command<Commands::LandscapeClear>::Do(DoCommandFlag::Execute, rig.destination).Succeeded()); }
	SECTION("second original paid edge") { REQUIRE(ChangeLayerPortalConnection(rig.upper.portal, rig.request.owner, false, true) == LayerPortalWorkError::None); }
	SECTION("second original intervening cell") { REQUIRE(ElectricalCableCommand(TileXYL(25, 20, MapLayer::Underground), UtilityEnergyDomain::Electricity, true).Succeeded()); }
	SECTION("second original machinery") { REQUIRE(Command<Commands::LandscapeClear>::Do(DoCommandFlag::Execute, rig.upper.tile).Succeeded()); }
	const auto state = CaptureUtilityPowerState(); const auto &relay = state.consumers[rig.relay.base()].shares.front();
	CHECK(state.producers.front().reserved_j == (upstream ? 360000 : 400000)); CHECK(state.producers.front().consumed_lifetime_j == 20000);
	CHECK(state.producers.front().generated_lifetime_j == 1998000); CHECK(state.consumers[rig.source.base()].shares.front().transferred_j == 140000);
	CHECK(state.consumers[rig.source.base()].shares.front().released_j == 0); CHECK(relay.transferred_j == 60000); CHECK(relay.released_j == (upstream ? 40000 : 0));
	REQUIRE(state.consumers[rig.destination_account.base()].shares.size() == 2);
	CHECK(state.consumers[rig.destination_account.base()].shares[0].consumed_j == 20000); CHECK(state.consumers[rig.destination_account.base()].shares[0].released_j == 20000);
	CHECK(state.consumers[rig.destination_account.base()].shares[1].released_j == 60000);
	CHECK(state.links[rig.first.base()].flow_j == 140000); CHECK(state.links[rig.second.base()].flow_j == 100000);
	CHECK(GetUtilityAllocation(rig.destination_account)->granted_energy_j == 0);
	CHECK(ValidateUtilityPowerState(state, GetCurrentMultiLayerProfileSelection()) == UtilityPowerError::None);
	const auto digest = ComputeUtilityPowerDigest();
	CHECK(PrepareUtilityMutation(UtilityMutationKind::Consume, {rig.destination_account, GetUtilityPowerInterval()}, 1).error == UtilityPowerError::InsufficientGrant);
	CHECK(ComputeUtilityPowerDigest() == digest);
}

/** An actual paid replacement cell creates a fresh reference; it neither revives old descendants nor refunds their original upstream capacity. */
TEST_CASE("Paid utility repaired cable obtains fresh rights without reviving imports", "[utility-power-path-lifecycle]")
{
	UtilityPathTransferRig rig(200000); rig.BranchAndUse();
	const auto tile = TileXYL(25, 20, MapLayer::Underground); const auto old = GetGeothermalUtilityLink(tile, UtilityEnergyDomain::Electricity)->id;
	REQUIRE(ElectricalCableCommand(tile, UtilityEnergyDomain::Electricity, true).Succeeded());
	const auto released = GetUtilityPowerConsumer(rig.destination_account)->shares.front().released_j;
	REQUIRE(ElectricalCableCommand(tile).Succeeded()); CHECK(GetGeothermalUtilityLink(tile, UtilityEnergyDomain::Electricity)->id != old);
	CHECK(GetUtilityAllocation(rig.destination_account)->granted_energy_j == 0);
	CHECK(GetUtilityPowerConsumer(rig.destination_account)->shares.front().released_j == released);
	const auto offer = GetUtilityPathTransferOffer(rig.Request(500000)); REQUIRE(offer.error == UtilityPowerError::None); CHECK(offer.quantity_j == 259600);
	rig.Transfer(rig.Request(50000));
	CHECK(CaptureUtilityPowerState().next_share == 5); CHECK(GetUtilityPowerConsumer(rig.destination_account)->shares.size() == 3);
	CHECK(GetUtilityAllocation(rig.destination_account)->granted_energy_j == 50000); CHECK(GetUtilityPowerProducer(rig.boundary_producer)->reserved_j == 400000);
	CHECK(GetUtilityPowerLink(rig.first)->flow_j == 190000); CHECK(GetUtilityPowerLink(rig.second)->flow_j == 150000);
	CHECK(ValidateUtilityPowerState(CaptureUtilityPowerState(), GetCurrentMultiLayerProfileSelection()) == UtilityPowerError::None);
	REQUIRE(ElectricalCableCommand(tile, UtilityEnergyDomain::Electricity, true).Succeeded());
	CHECK(GetUtilityPowerProducer(rig.boundary_producer)->reserved_j == 350000); CHECK(GetUtilityPowerProducer(rig.boundary_producer)->consumed_lifetime_j == 20000);
	const auto digest = ComputeUtilityPowerDigest(); CHECK(ElectricalCableCommand(tile, UtilityEnergyDomain::Electricity, true).Failed());
	CHECK(ComputeUtilityPowerDigest() == digest); CHECK(GetUtilityPowerLink(rig.first)->flow_j == 190000);
}

/** Reopening original paid machinery requires a fresh metered edge; old descendant rights remain released through the same native housing's reuse. */
TEST_CASE("Paid utility reopened riser never revives its retired electrical identity", "[utility-power-path-lifecycle]")
{
	UtilityPathTransferRig rig(200000); rig.BranchAndUse(); const auto first = rig.first;
	REQUIRE(ChangeLayerPortalConnection(rig.request.portal, rig.request.owner, false, true) == LayerPortalWorkError::None);
	REQUIRE(ChangeLayerPortalConnection(rig.request.portal, rig.request.owner, true, true) == LayerPortalWorkError::None);
	CHECK_FALSE(GetUtilityPowerLink(first)->active); CHECK(GetUtilityAllocation(rig.destination_account)->granted_energy_j == 0);
	const auto replacement = RegisterGeothermalRiserLink(rig.Riser().index, rig.source, rig.relay); REQUIRE(replacement.has_value());
	CHECK(*replacement != first); rig.first = *replacement; CHECK(GetUtilityPowerLink(*replacement)->flow_j == 0);
	CHECK(GetUtilityPowerConsumer(rig.relay)->shares.front().released_j == 40000);
	rig.Transfer(rig.Request(50000));
	CHECK(GetUtilityPowerLink(first)->flow_j == 140000); CHECK(GetUtilityPowerLink(*replacement)->flow_j == 50000);
	CHECK(GetUtilityPowerLink(rig.second)->flow_j == 150000); CHECK(GetUtilityPowerProducer(rig.boundary_producer)->reserved_j == 360000);
	CHECK(GetUtilityAllocation(rig.destination_account)->granted_energy_j == 50000);
	CHECK(ValidateUtilityPowerState(CaptureUtilityPowerState(), GetCurrentMultiLayerProfileSelection()) == UtilityPowerError::None);
}

/** Original source retirement releases free quantities at every depth once, preserving generated/consumed lifetime and all physical transfer history. */
TEST_CASE("Paid utility source loss expires every original descendant once", "[utility-power-path-lifecycle]")
{
	UtilityPathTransferRig rig(200000); rig.BranchAndUse();
	REQUIRE(RemoveUtilityPowerProducer(rig.boundary_producer) == UtilityPowerError::None);
	CHECK(GetUtilityPowerProducer(rig.boundary_producer)->reserved_j == 0); CHECK(GetUtilityPowerProducer(rig.boundary_producer)->consumed_lifetime_j == 20000);
	CHECK(GetUtilityPowerProducer(rig.boundary_producer)->generated_lifetime_j == 1998000);
	CHECK(GetUtilityPowerConsumer(rig.source)->shares.front().released_j == 360000); CHECK(GetUtilityPowerConsumer(rig.relay)->shares.front().released_j == 40000);
	CHECK(GetUtilityPowerLink(rig.first)->flow_j == 140000); CHECK(GetUtilityPowerLink(rig.second)->flow_j == 100000);
	const auto digest = ComputeUtilityPowerDigest(); CHECK(RemoveUtilityPowerProducer(rig.boundary_producer) == UtilityPowerError::InvalidReference);
	CHECK(PrepareUtilityPathTransfer(rig.Request(1)).error == UtilityPowerError::InsufficientGrant); CHECK(ComputeUtilityPowerDigest() == digest);
	CHECK(ValidateUtilityPowerState(CaptureUtilityPowerState(), GetCurrentMultiLayerProfileSelection()) == UtilityPowerError::None);
}

/** Actual takeover moves one original source and installed cells while retiring paid edges; actual abandonment expires every remaining original free slice. */
TEST_CASE("Paid utility descendants follow actual whole-company lifecycle", "[utility-power-path-lifecycle]")
{
	UtilityPathTransferRig rig(200000); rig.BranchAndUse();
	const auto before = CaptureUtilityPowerState();
	bool abandon = false;
	SECTION("actual takeover") {}
	SECTION("actual company abandonment") { abandon = true; }
	std::unique_ptr<Company> successor;
	if (!abandon) {
		REQUIRE(Company::CanAllocateItem()); successor.reset(Company::Create()); successor->money = 1000000; successor->clear_limit = 64U << 16;
	}
	if (abandon) ChangeOwnershipOfCompanyItems(rig.request.owner, INVALID_OWNER);
	else ChangeOwnershipOfCompanyItems(rig.request.owner, successor->index);
	CHECK_FALSE(GetUtilityPowerLink(rig.first)->active); CHECK_FALSE(GetUtilityPowerLink(rig.second)->active);
	CHECK(GetUtilityPowerProducer(rig.boundary_producer)->reserved_j == (abandon ? 0 : 360000));
	CHECK(GetUtilityPowerProducer(rig.boundary_producer)->consumed_lifetime_j == 20000); CHECK(GetUtilityPowerProducer(rig.boundary_producer)->generated_lifetime_j == 1998000);
	CHECK(GetUtilityPowerLink(rig.first)->flow_j == 140000); CHECK(GetUtilityPowerLink(rig.second)->flow_j == 100000);
	CHECK_FALSE(GetUtilityAllocation(rig.destination_account).has_value());
	if (!abandon) {
		CHECK(GetUtilityPowerProducer(rig.boundary_producer)->owner == successor->index); CHECK(GetUtilityPowerConsumer(rig.relay)->owner == successor->index);
		CHECK(GetGeothermalUtilityLink(rig.source_port, UtilityEnergyDomain::Electricity)->owner == successor->index);
		CHECK_FALSE(GetGeothermalRiserState(rig.Riser().index)->portal_retired);
	}
	const auto after = CaptureUtilityPowerState();
	CHECK(after.operation_sequence == before.operation_sequence + 1); CHECK(after.local_link_revision == before.local_link_revision + 1);
	CAPTURE(abandon, static_cast<int>(ValidateUtilityPowerLineage(after, {Map::SizeX(), Map::SizeY(), MultiLayerMap::GetLayerCount()})),
		static_cast<int>(ValidateUtilityPowerState(after, GetCurrentMultiLayerProfileSelection(), false)), after.local_link_revision, after.operation_sequence);
	CHECK(ValidateUtilityPowerState(after, GetCurrentMultiLayerProfileSelection()) == UtilityPowerError::None);
}

/** Whole epoch rotation clears old graph/flows together; only fresh original publication/allocation may create new paid imports, and settled lifetime remains unchanged. */
TEST_CASE("Paid utility interval expiry cannot renew prior descendant rights", "[utility-power-path-lifecycle]")
{
	UtilityPathTransferRig rig(200000); rig.BranchAndUse(); const auto old = PrepareUtilityPathTransfer(rig.Request(1)); REQUIRE(old.error == UtilityPowerError::None);
	{
		AutoRestoreBackup system(_current_company, OWNER_DEITY); REQUIRE(BeginUtilityPowerInterval() == UtilityPowerError::None);
	}
	CHECK(CaptureUtilityPowerState().next_share == 0); CHECK(GetUtilityPowerConsumer(rig.destination_account)->shares.empty()); CHECK(GetUtilityPowerConsumer(rig.relay)->shares.empty());
	CHECK(GetUtilityPowerLink(rig.first)->flow_j == 0); CHECK(GetUtilityPowerLink(rig.second)->flow_j == 0);
	CHECK(GetUtilityPowerProducer(rig.boundary_producer)->reserved_j == 0); CHECK(GetUtilityPowerProducer(rig.boundary_producer)->consumed_lifetime_j == 20000);
	CHECK(CommitUtilityPathTransfer(old).error == UtilityPowerError::StaleMutation);
	REQUIRE(PublishUtilityGeneration(rig.boundary_producer, GetUtilityPowerInterval(), 1000000, 74) == UtilityPowerError::None);
	{
		AutoRestoreBackup system(_current_company, OWNER_DEITY);
		REQUIRE(AllocateUtilityPower(GetUtilityPowerInterval(), std::array{UtilityPowerRequest{rig.source, 500000}, UtilityPowerRequest{rig.relay, 0}, UtilityPowerRequest{rig.destination_account, 600000}}) == UtilityPowerError::None);
	}
	rig.Transfer(rig.Request(100000)); CHECK(GetUtilityPowerProducer(rig.boundary_producer)->generated_lifetime_j == 3996000);
	CHECK(GetUtilityPowerProducer(rig.boundary_producer)->consumed_lifetime_j == 20000); CHECK(GetUtilityPowerProducer(rig.boundary_producer)->reserved_j == 500000);
	CHECK(CaptureUtilityPowerState().next_share == 2); CHECK(ValidateUtilityPowerState(CaptureUtilityPowerState(), GetCurrentMultiLayerProfileSelection()) == UtilityPowerError::None);
}
