/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file utility_power_lineage.cpp Shared parent/physical-path contract on actual paid relay owners; proposed snapshots prove validation, not committed export.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs12-solar-farms-mirrors-and-eti.md Group8C.1
 */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "geothermal_electrical_fixture.h"
#include "../utility_power_lineage.h"
#include "../safeguards.h"

/** Actual installed paid geometry and shared publication/allocation; later child quantities are explicitly unpublished contract proposals. */
class UtilityLineageRig : public NativeElectricalRelayRig {
public:
	GeothermalElectricalPath path; ///< Actual deterministic installed two-riser route.
	/** Fund/complete/install real native machines/cables and allocate an explicit boundary electrical source; no export or actual plant is claimed. */
	UtilityLineageRig() : NativeElectricalRelayRig(false, true)
	{
		this->InstallSourceCable(); this->InstallRelayCable();
		AutoRestoreBackup system(_current_company, OWNER_DEITY);
		REQUIRE(AllocateUtilityPower(GetUtilityPowerInterval(), std::array{UtilityPowerRequest{this->source, 500000}, UtilityPowerRequest{this->relay, 100000}, UtilityPowerRequest{this->destination_account, 200000}}) == UtilityPowerError::None);
		const auto route = FindGeothermalElectricalPath(this->source, this->destination_account, this->source_port, 74);
		REQUIRE(route.has_value()); this->path = *route;
	}
	/** @param leg Actual paid transition index. @return Exact new installed inlet cells followed by that original native edge. */
	std::vector<UtilityLinkID> Segment(size_t leg) const
	{
		auto segment = this->path.legs[leg].inlet.cells; segment.push_back(this->path.legs[leg].riser); return segment;
	}
	/** @return Unpublished100-kJ first import,60-kJ child and20-kJ proposed use; native/shared live state stays unmodified for contract tests. */
	UtilityPowerState Proposal() const
	{
		auto state = CaptureUtilityPowerState(); auto &root = state.consumers[this->source.base()].shares.front();
		REQUIRE(root.id == UtilityShareID{0}); root.transferred_j = 100000;
		UtilityPowerShare first{this->boundary_producer, 100000, 0, 0, 60000, this->first};
		first.id = UtilityShareID{1}; first.parent = root.id; first.segment = this->Segment(0);
		UtilityPowerShare second{this->boundary_producer, 60000, 20000, 0, 0, this->second};
		second.id = UtilityShareID{2}; second.parent = first.id; second.segment = this->Segment(1);
		state.consumers[this->relay.base()].shares.push_back(first); state.consumers[this->relay.base()].meter.granted_energy_j = 100000; state.consumers[this->relay.base()].meter.unmet_energy_j = 0;
		state.consumers[this->destination_account.base()].shares.push_back(second); state.consumers[this->destination_account.base()].meter.granted_energy_j = 60000; state.consumers[this->destination_account.base()].meter.unmet_energy_j = 140000;
		state.producers[this->boundary_producer.base()].reserved_j -= 20000; state.producers[this->boundary_producer.base()].consumed_lifetime_j = 20000;
		state.links[this->first.base()].flow_j = 100000; state.links[this->second.base()].flow_j = 60000; state.next_share = 3;
		return state;
	}
};

/** Equal producer/final-edge imports retain distinct exact parents and incoming paths, conserving free quantities without counting ancestors' flows twice. */
TEST_CASE("Shared utility lineage distinguishes staged and direct paid relay arrivals", "[utility-power-lineage]")
{
	UtilityLineageRig rig; auto state = rig.Proposal(); const auto before = ComputeUtilityPowerDigest();
	REQUIRE(ValidateUtilityPowerState(state, GetCurrentMultiLayerProfileSelection()) == UtilityPowerError::None);
	auto direct = state.consumers[rig.destination_account.base()].shares.front();
	direct.id = UtilityShareID{3}; direct.parent = UtilityShareID{0}; direct.consumed_j = 0; direct.segment = rig.Segment(0);
	const auto second = rig.Segment(1); direct.segment.insert(direct.segment.end(), second.begin(), second.end());
	state.consumers[rig.source.base()].shares.front().transferred_j += 60000;
	// Canonical incoming-right order differs from issued identity order. Both
	// paths end at the same edge, but only the exact parent can own each debit.
	state.consumers[rig.destination_account.base()].shares.insert(state.consumers[rig.destination_account.base()].shares.begin(), direct);
	state.consumers[rig.destination_account.base()].meter.granted_energy_j += 60000; state.consumers[rig.destination_account.base()].meter.unmet_energy_j -= 60000;
	state.links[rig.first.base()].flow_j += 60000; state.links[rig.second.base()].flow_j += 60000; ++state.next_share;
	CHECK(ValidateUtilityPowerState(state, GetCurrentMultiLayerProfileSelection()) == UtilityPowerError::None);
	CHECK(state.producers.front().meter.generation_energy_j == 1998000);
	CHECK(state.producers.front().reserved_j == 480000); CHECK(state.producers.front().consumed_lifetime_j == 20000);
	CHECK(ComputeUtilityPowerDigest() == before); CHECK(GetUtilityPowerLink(rig.first)->flow_j == 0); CHECK(GetUtilityPowerLink(rig.second)->flow_j == 0);
}

/** Forged parent, original producer, missing path/identity and doubled physical capacity refuse before any live owner changes. */
TEST_CASE("Shared utility lineage refuses forged or circulating paid path rights", "[utility-power-lineage]")
{
	UtilityLineageRig rig; auto state = rig.Proposal();
	SECTION("parent cycle") { state.consumers[rig.relay.base()].shares.front().parent = UtilityShareID{2}; }
	SECTION("unmatched donor debit") { ++state.consumers[rig.source.base()].shares.front().transferred_j; }
	SECTION("duplicate shared identity") { state.consumers[rig.destination_account.base()].shares.front().id = UtilityShareID{1}; }
	SECTION("missing shared identity") { ++state.next_share; }
	SECTION("missing inlet cell") { state.consumers[rig.destination_account.base()].shares.front().segment.erase(state.consumers[rig.destination_account.base()].shares.front().segment.begin()); }
	SECTION("wrong original source") { state.consumers[rig.destination_account.base()].shares.front().producer = UtilityProducerID{1}; }
	SECTION("repeated physical cell") { state.consumers[rig.destination_account.base()].shares.front().segment.insert(state.consumers[rig.destination_account.base()].shares.front().segment.begin(), rig.path.legs[0].inlet.cells.front()); }
	SECTION("wrong final edge") { state.consumers[rig.destination_account.base()].shares.front().imported_link = rig.first; }
	SECTION("inflated physical flow") { ++state.links[rig.first.base()].flow_j; }
	const auto before = ComputeUtilityPowerDigest();
	CHECK(ValidateUtilityPowerState(state, GetCurrentMultiLayerProfileSelection()) != UtilityPowerError::None); CHECK(ComputeUtilityPowerDigest() == before);
}

/** An ancestor's physical loss invalidates unused descendants while keeping consumed energy, spent edge flow and donor debits exactly conserved. */
TEST_CASE("Shared utility lineage audits ancestor loss without donor refund", "[utility-power-lineage]")
{
	UtilityLineageRig rig; auto state = rig.Proposal(); state.links[rig.first.base()].active = false;
	CHECK(ValidateUtilityPowerState(state, GetCurrentMultiLayerProfileSelection()) != UtilityPowerError::None);
	state.consumers[rig.relay.base()].shares.front().released_j = 40000;
	state.consumers[rig.destination_account.base()].shares.front().released_j = 40000;
	state.producers.front().reserved_j -= 80000;
	CHECK(ValidateUtilityPowerState(state, GetCurrentMultiLayerProfileSelection()) == UtilityPowerError::None);
	CHECK(state.consumers[rig.source.base()].shares.front().transferred_j == 100000); CHECK(state.links[rig.first.base()].flow_j == 100000);
	CHECK(state.consumers[rig.destination_account.base()].shares.front().consumed_j == 20000); CHECK(state.producers.front().consumed_lifetime_j == 20000);
}

/** Electrical and thermal root issuance share one epoch identity space; only complete rotation may reuse a slice number. */
TEST_CASE("Shared utility lineage identity spans both domains until whole epoch rotation", "[utility-power-lineage]")
{
	UtilityLineageRig rig;
	const auto heat = RegisterUtilityHeatProducer(rig.request.owner); REQUIRE(heat.has_value());
	const auto consumer = RegisterUtilityHeatConsumer(rig.request.owner, UtilityHeatUse::DirectRecipe, 1000, std::array{*heat}); REQUIRE(consumer.has_value());
	REQUIRE(PublishUtilityHeatGeneration(*heat, GetUtilityPowerInterval(), 1000000, 74) == UtilityPowerError::None);
	{
		AutoRestoreBackup system(_current_company, OWNER_DEITY);
		REQUIRE(AllocateUtilityHeat(GetUtilityPowerInterval(), std::array{UtilityPowerRequest{*consumer, 1000}}) == UtilityPowerError::None);
	}
	const auto state = CaptureUtilityPowerState(); CHECK(state.next_share == 2);
	CHECK(state.consumers[rig.source.base()].shares.front().id == UtilityShareID{0}); CHECK(state.consumers[consumer->base()].shares.front().id == UtilityShareID{1});
	CHECK(ValidateUtilityPowerState(state, GetCurrentMultiLayerProfileSelection()) == UtilityPowerError::None);
	{
		AutoRestoreBackup system(_current_company, OWNER_DEITY); REQUIRE(BeginUtilityPowerInterval() == UtilityPowerError::None);
	}
	CHECK(CaptureUtilityPowerState().next_share == 0); CHECK(CaptureUtilityPowerState().consumers[consumer->base()].shares.empty());
}

/** Two original sources cannot multiply one cable/riser's physical allowance by presenting different synchronized windows in the same closed epoch. */
TEST_CASE("Shared utility lineage rejects mixed source windows on the same physical resource", "[utility-power-lineage]")
{
	UtilityLineageRig rig; auto state = rig.Proposal(); auto second = state.producers.front();
	second.meter.producer_id = UtilityProducerID{1}; second.meter.economy_ticks = 37; second.meter.generation_energy_j = 999000;
	second.generated_lifetime_j = 999000; second.consumed_lifetime_j = 0; second.reserved_j = 1000;
	state.producers.push_back(second); ++state.next_producer;
	UtilityPowerShare root{UtilityProducerID{1}, 1000, 0, 0, 1000}; root.id = UtilityShareID{3};
	state.consumers[rig.source.base()].shares.push_back(root); state.consumers[rig.source.base()].linked_producers.push_back(UtilityProducerID{1});
	state.consumers[rig.source.base()].meter.demand_energy_j += 1000; state.consumers[rig.source.base()].meter.granted_energy_j += 1000;
	UtilityPowerShare child{UtilityProducerID{1}, 1000, 0, 0, 0, rig.second}; child.id = UtilityShareID{4}; child.parent = root.id;
	child.segment = rig.Segment(0); const auto tail = rig.Segment(1); child.segment.insert(child.segment.end(), tail.begin(), tail.end());
	state.consumers[rig.destination_account.base()].shares.push_back(child);
	state.consumers[rig.destination_account.base()].meter.granted_energy_j += 1000; state.consumers[rig.destination_account.base()].meter.unmet_energy_j -= 1000;
	state.links[rig.first.base()].flow_j += 1000; state.links[rig.second.base()].flow_j += 1000; state.next_share = 5;
	CHECK(ValidateUtilityPowerState(state, GetCurrentMultiLayerProfileSelection()) == UtilityPowerError::InvalidInterval);
	CHECK(CaptureUtilityPowerState().producers.size() == 1); CHECK(GetUtilityPowerLink(rig.first)->flow_j == 0);
}

/** Older one-hop quantity/route views acquire exact parents without a new energy event; a broken original reference leaves the entire unpublished input unchanged. */
TEST_CASE("Shared utility lineage migrates genuine one-hop shapes atomically", "[utility-power-lineage]")
{
	NativeRiserRig rig;
	const auto producer = RegisterUtilityPowerProducer(rig.request.owner, MapLayer::DeepUnderground, UtilityGenerationKind::External); REQUIRE(producer.has_value());
	const auto from = RegisterUtilityPowerConsumer(rig.request.owner, MapLayer::DeepUnderground, UtilityConsumerEffect::AssemblyWork, 1000, std::array{*producer});
	const auto to = RegisterUtilityPowerConsumer(rig.request.owner, MapLayer::Underground, UtilityConsumerEffect::AssemblyWork, 1000, {});
	REQUIRE(from.has_value()); REQUIRE(to.has_value());
	const auto edge = RegisterUtilityPowerLink(*from, *to, rig.request.portal, 500); REQUIRE(edge.has_value());
	REQUIRE(PublishUtilityGeneration(*producer, GetUtilityPowerInterval(), 1000, 74) == UtilityPowerError::None);
	{
		AutoRestoreBackup system(_current_company, OWNER_DEITY);
		REQUIRE(AllocateUtilityPower(GetUtilityPowerInterval(), std::array{UtilityPowerRequest{*from, 1000}, UtilityPowerRequest{*to, 1000}}) == UtilityPowerError::None);
	}
	const auto transfer = PrepareUtilityMutation(UtilityMutationKind::TransferLayer, {*from, GetUtilityPowerInterval()}, 400, *to, *edge);
	REQUIRE(CommitUtilityMutation(transfer).error == UtilityPowerError::None);
	auto legacy = CaptureUtilityPowerState(); legacy.next_share = 0;
	// This copy intentionally retains exactly the older wire shape. Actual
	// native earlier-format parsing/migration is verified by the file wrapper.
	for (auto &consumer : legacy.consumers) {
		for (auto &share : consumer.shares) { share.id = UtilityShareID{UINT32_MAX}; share.parent = UtilityShareID{UINT32_MAX}; share.segment.clear(); }
	}
	SECTION("complete migration") {
		REQUIRE(MigrateUtilityPowerLineage(legacy) == UtilityPowerError::None);
		CHECK(ValidateUtilityPowerState(legacy, GetCurrentMultiLayerProfileSelection()) == UtilityPowerError::None);
		CHECK(legacy.next_share == 2); CHECK(legacy.consumers[to->base()].shares.front().parent == UtilityShareID{0});
		CHECK(legacy.consumers[to->base()].shares.front().segment == std::vector{*edge}); CHECK(legacy.producers.front().reserved_j == 1000);
	}
	SECTION("broken original reference") {
		legacy.consumers[to->base()].shares.front().imported_link = UtilityLinkID{99};
		CHECK(MigrateUtilityPowerLineage(legacy) != UtilityPowerError::None); CHECK(legacy.next_share == 0);
		CHECK(legacy.consumers[from->base()].shares.front().id == UtilityShareID{UINT32_MAX}); CHECK(legacy.links.front().flow_j == 400);
	}
}
