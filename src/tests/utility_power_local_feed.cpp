/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file utility_power_local_feed.cpp Conserved local electrical routing through real paid native plant, depot and cable owners.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs12-solar-farms-mirrors-and-eti.md Group9C.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
 * Floor and delivered construction materials are explicit fixture inputs, separate from paid-world bootstrap acceptance.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "geothermal_generator_operating_fixture.h"
#include "../utility_power_transfer.h"
#include "../utility_power_lineage.h"
#include "../rail_cmd.h"
#include "../rail_map.h"
#include "../landscape_cmd.h"
#include "../safeguards.h"

#include "utility_power_local_feed_fixture.h"

/** Actual native conversion remains the only publication while local routing preserves exact original parent history and retry identity. */
TEST_CASE("Native local feed transfers original geothermal output once", "[utility-power-local-feed]")
{
	NativeLocalFeedRig rig{GENERATE(GeothermalGeneratorModel::VentTap, GeothermalGeneratorModel::MagmaLoop)}; const auto before = CaptureUtilityPowerState();
	const auto native = ComputeGeothermalGeneratorDigest(); const auto cash = Company::Get(_current_company)->money;
	const auto proposal = PrepareUtilityPathTransfer(rig.Request(0, 3000)); REQUIRE(proposal.error == UtilityPowerError::None);
	const auto digest = ComputeUtilityPowerDigest(); REQUIRE(CommitUtilityPathTransfer(proposal).error == UtilityPowerError::None);
	const auto state = CaptureUtilityPowerState(); const auto &child = state.consumers[rig.recipients[0].base()].shares.front();
	CHECK(child.producer == rig.output); CHECK(child.parent == before.consumers[rig.supply.base()].shares.front().id);
	CHECK(child.allocated_j == 3000); CHECK(child.imported_link == rig.feeds[0]); CHECK(state.next_producer == before.next_producer);
	CHECK(state.producers[rig.output.base()].generated_lifetime_j == before.producers[rig.output.base()].generated_lifetime_j);
	CHECK(state.producers[rig.output.base()].reserved_j == before.producers[rig.output.base()].reserved_j);
	CHECK(ComputeUtilityPowerDigest() != digest); const auto committed = ComputeUtilityPowerDigest();
	CHECK(CommitUtilityPathTransfer(proposal).error == UtilityPowerError::StaleMutation); CHECK(ComputeUtilityPowerDigest() == committed);
	CHECK(ComputeGeothermalGeneratorDigest() == native); CHECK(Company::Get(_current_company)->money == cash);
	CHECK(ValidateUtilityPowerState(state, GetCurrentMultiLayerProfileSelection()) == UtilityPowerError::None);
}

/** Shared paid cable capacity remains spent across separate destination branches and explicit unused-grant release. */
TEST_CASE("Native local feed branches share one cable opportunity", "[utility-power-local-feed]")
{
	NativeLocalFeedRig rig; rig.Transfer(0, 3000);
	const auto release = PrepareUtilityMutation(UtilityMutationKind::ReleaseUnused, {rig.recipients[0], GetUtilityPowerInterval()}, 1000);
	REQUIRE(CommitUtilityMutation(release).error == UtilityPowerError::None);
	const auto offer = GetUtilityPathTransferOffer(rig.Request(1, 9450)); REQUIRE(offer.error == UtilityPowerError::None); CHECK(offer.quantity_j == 2400);
	const auto before = ComputeUtilityPowerDigest(); CHECK(PrepareUtilityPathTransfer(rig.Request(1, 2401)).error == UtilityPowerError::InsufficientGrant);
	CHECK(ComputeUtilityPowerDigest() == before); rig.Transfer(1, 2400);
	CHECK(GetUtilityPathTransferOffer(rig.Request(0, 1)).error == UtilityPowerError::InsufficientGrant);
	CHECK(GetUtilityPowerLink(rig.feeds[0])->flow_j == 3000); CHECK(GetUtilityPowerLink(rig.feeds[1])->flow_j == 2400);
	CHECK(ValidateUtilityPowerState(CaptureUtilityPowerState(), GetCurrentMultiLayerProfileSelection()) == UtilityPowerError::None);
}

/** Missing cable, altered native recipient and unsupported legacy one-hop requests refuse before any source or destination mutation. */
TEST_CASE("Native local feed refuses incomplete or forged physical paths", "[utility-power-local-feed-refusal]")
{
	NativeLocalFeedRig rig; auto request = rig.Request(0, 1);
	SECTION("missing cable") { request.legs.front().cells.erase(request.legs.front().cells.begin() + 1); }
	SECTION("wrong inlet") { request.destination = rig.recipients[1]; }
	SECTION("wrong source port") { request.source_port = rig.inlets[0]; }
	SECTION("duplicate feed") {
		CHECK_FALSE(RegisterGeothermalLocalFeed(rig.generator, rig.supply, rig.recipients[0], rig.inlets[0]).has_value()); return;
	}
	SECTION("legacy bypass") {
		CHECK(PrepareUtilityMutation(UtilityMutationKind::TransferLayer, {rig.supply, GetUtilityPowerInterval()}, 1,
			rig.recipients[0], rig.feeds[0]).error == UtilityPowerError::InvalidTopology); return;
	}
	const auto before = ComputeUtilityPowerDigest(); CHECK(PrepareUtilityPathTransfer(request).error != UtilityPowerError::None);
	CHECK(GetUtilityPathTransferOffer(request).error != UtilityPowerError::None); CHECK(ComputeUtilityPowerDigest() == before);
}

/** Original generator removal expires unused imported rights and retires the exact attachment before a native pool identity can be reused. */
TEST_CASE("Native local feed retains spent history after plant demolition", "[utility-power-local-feed-loss]")
{
	NativeLocalFeedRig rig; rig.Transfer(0, 3000);
	REQUIRE(Command<Commands::LandscapeClear>::Unsafe<CommandCallback>(INVALID_STRING_ID, nullptr, false, false, rig.plant_port, std::make_tuple(rig.plant_port)).Succeeded());
	CHECK_FALSE(GetUtilityPowerLink(rig.feeds[0])->active); CHECK_FALSE(GetUtilityPowerLink(rig.feeds[1])->active);
	CHECK(GetUtilityPowerLink(rig.feeds[0])->flow_j == 3000); CHECK_FALSE(GetUtilityPowerProducer(rig.output)->active);
	CHECK(ValidateUtilityPowerState(CaptureUtilityPowerState(), GetCurrentMultiLayerProfileSelection()) == UtilityPowerError::None);
}

/** Direct original producer retirement closes its physical feeds before clearing eligibility, retaining transferred history and refusing any inherited future rights. */
TEST_CASE("Native local feed source removal retires only its original attachments", "[utility-power-local-feed-loss]")
{
	NativeLocalFeedRig rig; rig.Transfer(0, 3000); const auto before = ComputeUtilityPowerDigest();
	REQUIRE(RemoveUtilityPowerProducer(rig.output, false) == UtilityPowerError::None); CHECK(ComputeUtilityPowerDigest() == before);
	REQUIRE(RemoveUtilityPowerProducer(rig.output) == UtilityPowerError::None);
	CHECK_FALSE(GetUtilityPowerLink(rig.feeds[0])->active); CHECK_FALSE(GetUtilityPowerLink(rig.feeds[1])->active);
	CHECK(GetUtilityPowerLink(rig.feeds[0])->flow_j == 3000);
	CHECK(GetUtilityPowerConsumer(rig.recipients[0])->shares.front().released_j == 3000);
	CHECK(ValidateUtilityPowerState(CaptureUtilityPowerState(), GetCurrentMultiLayerProfileSelection()) == UtilityPowerError::None);
}

/** A removed original cable expires only unused descendants; replacing it cannot recover rights already spent on the remaining shared trunk. */
TEST_CASE("Native local feed cable repair preserves original spent capacity", "[utility-power-local-feed-loss]")
{
	NativeLocalFeedRig rig; rig.Transfer(0, 3000);
	const auto tile = TileXYL(35, 24, MapLayer::DeepUnderground);
	const auto original = GetGeothermalUtilityLink(tile, UtilityEnergyDomain::Electricity)->id;
	REQUIRE(Command<Commands::RemoveGeothermalUtilityLink>::Unsafe<CommandCallback>(INVALID_STRING_ID, nullptr, false, false, tile,
		std::make_tuple(tile, UtilityEnergyDomain::Electricity)).Succeeded());
	CHECK_FALSE(GetUtilityPowerLink(original)->active); CHECK(GetUtilityPowerLink(rig.feeds[0])->active);
	CHECK(GetUtilityPowerConsumer(rig.recipients[0])->shares.front().released_j == 3000);
	CHECK(ValidateUtilityPowerState(CaptureUtilityPowerState(), GetCurrentMultiLayerProfileSelection()) == UtilityPowerError::None);
	rig.Build(tile, UtilityEnergyDomain::Electricity);
	CHECK(GetGeothermalUtilityLink(tile, UtilityEnergyDomain::Electricity)->id != original);
	const auto offer = GetUtilityPathTransferOffer(rig.Request(1, 9450)); REQUIRE(offer.error == UtilityPowerError::None); CHECK(offer.quantity_j == 2400);
}

/** Native depot removal must retire its original attachment before a replacement on the same tile can receive a fresh nonreused feed identity. */
TEST_CASE("Native local feed depot replacement cannot inherit an old inlet", "[utility-power-local-feed-loss]")
{
	NativeLocalFeedRig rig; rig.Transfer(0, 3000);
	const auto tile = rig.inlets[0];
	REQUIRE(Command<Commands::LandscapeClear>::Unsafe<CommandCallback>(INVALID_STRING_ID, nullptr, false, false, tile, std::make_tuple(tile)).Succeeded());
	CHECK_FALSE(GetUtilityPowerLink(rig.feeds[0])->active); CHECK(GetUtilityPowerLink(rig.feeds[1])->active);
	CHECK(GetUtilityPowerConsumer(rig.recipients[0])->shares.front().released_j == 3000);
	const auto rebuilt = CmdBuildTrainDepot(DoCommandFlag::Execute, tile, GetTubeRailType(), DiagDirection::SE);
	CHECK(rebuilt.Failed()); // Native building admission requires removing the original coexisting cable before constructing a replacement facility.
	REQUIRE(Command<Commands::RemoveGeothermalUtilityLink>::Unsafe<CommandCallback>(INVALID_STRING_ID, nullptr, false, false, tile,
		std::make_tuple(tile, UtilityEnergyDomain::Electricity)).Succeeded());
	REQUIRE(Command<Commands::BuildRailDepot>::Unsafe<CommandCallback>(INVALID_STRING_ID, nullptr, false, false, tile,
		std::make_tuple(tile, GetTubeRailType(), DiagDirection::SE)).Succeeded());
	rig.Build(tile, UtilityEnergyDomain::Electricity);
	CHECK_FALSE(GetUtilityPowerLink(rig.feeds[0])->active);
	const auto replacement = RegisterGeothermalLocalFeed(rig.generator, rig.supply, rig.recipients[0], tile);
	REQUIRE(replacement.has_value()); CHECK(*replacement != rig.feeds[0]);
	CHECK(ValidateUtilityPowerState(CaptureUtilityPowerState(), GetCurrentMultiLayerProfileSelection()) == UtilityPowerError::None);
}

/** Copied native transport rejects wrong original machine roles, local endpoints, parent paths and aggregate rates without repairing the live owner. */
TEST_CASE("Native local feed copied lineage refuses forged ownership", "[utility-power-local-feed-lineage]")
{
	NativeLocalFeedRig rig; rig.Transfer(0, 3000); const auto digest = ComputeUtilityPowerDigest();
	const auto state = CaptureUtilityPowerState(); REQUIRE(ValidateUtilityPowerState(state, GetCurrentMultiLayerProfileSelection(), false) == UtilityPowerError::None);
	// Each independent candidate corrupts one business invariant. Validation
	// must refuse before any unpublished transport can replace the real owner.
	for (unsigned fault = 0; fault < 7; ++fault) {
		INFO("copied local-feed fault=" << fault);
		auto bad = state; auto &feed = bad.links[rig.feeds[0].base()]; auto &child = bad.consumers[rig.recipients[0].base()].shares.front();
		switch (fault) {
			case 0: feed.kind = UtilityLinkKind::ElectricalRiser; break;
			case 1: feed.generator = IndustryID::Invalid(); break;
			case 2: feed.portal_revision = 1; break;
			case 3: feed.destination_endpoint = rig.inlets[1]; break;
			case 4: child.segment.erase(child.segment.begin() + 1); break;
			case 5: feed.flow_j = 3001; break;
			case 6: bad.consumers[rig.recipients[0].base()].linked_producers.push_back(rig.output); break;
		}
		// Copied preview supplies its own dimensions separately from numerical
		// validation; geometry-free ledger checks cannot detect a missing cell.
		CHECK((ValidateUtilityPowerState(bad, GetCurrentMultiLayerProfileSelection(), false) != UtilityPowerError::None ||
			ValidateUtilityPowerLineage(bad, {64, 64, 4}) != UtilityPowerError::None));
		CHECK(ComputeUtilityPowerDigest() == digest);
	}
}
