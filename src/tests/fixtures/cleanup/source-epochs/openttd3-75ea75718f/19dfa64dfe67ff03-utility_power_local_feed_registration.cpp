/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file utility_power_local_feed_registration.cpp Atomic real recipient/feed admission and conserved original-source transfers, without capsule-work claims.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Group6BCB.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Group6BCC original recipient lifecycle.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs12-solar-farms-mirrors-and-eti.md Group9C.
 * Prepared floor/materials are explicit fixture boundaries, separate from paid-TBM/bootstrap acceptance.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "geothermal_tube_power_fixture.h"
#include "../safeguards.h"

/** One complete shared registration predicts and issues only the actual request/feed pair, with no generation, grant, source metadata or financial side effect. */
TEST_CASE("Original local-feed recipient registration is one shared operation", "[utility-local-feed-recipient]")
{
	GeothermalTubePowerRig rig; const auto source = rig.Connect();
	const auto original = CaptureUtilityPowerState(); const auto digest = ComputeUtilityPowerDigest();
	const auto physical = ComputeTubeGeothermalPowerDigest(), native = ComputeGeothermalGeneratorDigest();
	const auto cash = Company::Get(_current_company)->money; const auto random = _random;
	const auto quote = RegisterGeothermalLocalFeedRecipient(rig.generator, source.supply, rig.inlets[0], 10000, false); REQUIRE(quote.has_value());
	CHECK(quote->consumer.base() == original.next_consumer); CHECK(quote->feed.base() == original.next_link);
	CHECK(ComputeUtilityPowerDigest() == digest); CHECK(ComputeTubeGeothermalPowerDigest() == physical);
	const auto actual = RegisterGeothermalLocalFeedRecipient(rig.generator, source.supply, rig.inlets[0], 10000); REQUIRE(actual.has_value());
	CHECK(actual->consumer == quote->consumer); CHECK(actual->feed == quote->feed);
	const auto state = CaptureUtilityPowerState();
	CHECK(state.next_consumer == original.next_consumer + 1); CHECK(state.next_link == original.next_link + 1);
	CHECK(state.operation_sequence == original.operation_sequence + 1); CHECK(state.next_producer == original.next_producer);
	CHECK(state.next_share == original.next_share); CHECK(state.local_link_revision == original.local_link_revision);
	const auto *consumer = GetUtilityPowerConsumer(actual->consumer); const auto *feed = GetUtilityPowerLink(actual->feed);
	REQUIRE(consumer != nullptr); REQUIRE(feed != nullptr);
	CHECK(consumer->active); CHECK(consumer->owner == _current_company); CHECK(consumer->effect == UtilityConsumerEffect::TubeWork);
	CHECK(consumer->linked_producers.empty()); CHECK(consumer->shares.empty()); CHECK(consumer->capacity_j == 10000);
	CHECK(consumer->meter.granted_energy_j == 0); CHECK(feed->kind == UtilityLinkKind::ElectricalFeed);
	CHECK(feed->generator == rig.generator); CHECK(feed->source == source.supply); CHECK(feed->destination == actual->consumer);
	CHECK(feed->source_endpoint == rig.plant_port); CHECK(feed->destination_endpoint == rig.inlets[0]); CHECK(feed->flow_j == 0);
	CHECK(feed->capacity_j == 0); CHECK(feed->riser_capacity_w == 0);
	CHECK(ComputeGeothermalGeneratorDigest() == native); CHECK(ComputeTubeGeothermalPowerDigest() == physical);
	CHECK(Company::Get(_current_company)->money == cash); CHECK(std::ranges::equal(random.state, _random.state));
	CHECK(ValidateUtilityPowerState(state, GetCurrentMultiLayerProfileSelection()) == UtilityPowerError::None);
	CHECK(ValidateTubeGeothermalPowerState(CaptureTubeGeothermalPowerState(), state));
}

/** Every real owner/model/facility/path/role/issuer refusal leaves both original IDs, source metadata, quantities, money and RNG unchanged in estimate and execute modes. */
TEST_CASE("Original local-feed recipient refusal cannot strand a consumer", "[utility-local-feed-recipient-refusal]")
{
	std::unique_ptr<Company> foreign;
	GeothermalTubePowerRig rig; const auto original = rig.Connect();
	AutoRestoreBackup actor(_current_company); AutoRestoreBackup date(TimerGameCalendar::date);
	auto supply = original.supply; auto generator = rig.generator; auto inlet = rig.inlets[0]; uint64_t capacity = 10000;
	SECTION("unknown original forwarding root") { supply = UtilityConsumerID{UINT32_MAX}; }
	SECTION("actual collector is not original electrical machinery") { generator = rig.collector; }
	SECTION("cable cell is not a native recipient") { inlet = TileXYL(34, 24, MapLayer::DeepUnderground); }
	SECTION("purchased trunk is missing") {
		const auto tile = TileXYL(35, 24, MapLayer::DeepUnderground);
		REQUIRE(Command<Commands::RemoveGeothermalUtilityLink>::Unsafe<CommandCallback>(INVALID_STRING_ID, nullptr, false, false, tile,
			std::make_tuple(tile, UtilityEnergyDomain::Electricity)).Succeeded());
	}
	SECTION("zero-work placeholder") { capacity = 0; }
	SECTION("unbounded request ceiling") { capacity = MAX_UTILITY_ACCOUNT_ENERGY_J + 1; }
	SECTION("foreign actual company") { REQUIRE(Company::CanAllocateItem()); foreign.reset(Company::Create()); _current_company = foreign->index; }
	SECTION("actual Magma model is locked") { TimerGameCalendar::date = TimerGameCalendar::ConvertYMDToDate(TimerGameCalendar::Year{1999}, 0, 1); }
	SECTION("original forwarding root is retired") { REQUIRE(CloseTubeGeothermalPower(original.source) == TubePowerError::None); }
	SECTION("shared terminal receipt cannot advance") {
		auto state = CaptureUtilityPowerState(); state.operation_sequence = UINT64_MAX - 1;
		StageLoadedUtilityPowerState(std::move(state)); REQUIRE(AcceptLoadedUtilityPowerState());
	}
	const auto state = CaptureUtilityPowerState(); const auto digest = ComputeUtilityPowerDigest();
	const auto physical = ComputeTubeGeothermalPowerDigest(), native = ComputeGeothermalGeneratorDigest();
	const auto cash = Company::Get(_current_company)->money; const auto random = _random;
	// Both command modes must fail before either issuer changes; a failed feed
	// cannot leave a real but orphaned work request behind in the shared owner.
	for (const bool execute : {false, true}) {
		CHECK_FALSE(RegisterGeothermalLocalFeedRecipient(generator, supply, inlet, capacity, execute).has_value());
		CHECK(ComputeUtilityPowerDigest() == digest); CHECK(CaptureUtilityPowerState().next_consumer == state.next_consumer);
		CHECK(CaptureUtilityPowerState().next_link == state.next_link); CHECK(ComputeTubeGeothermalPowerDigest() == physical);
		CHECK(ComputeGeothermalGeneratorDigest() == native); CHECK(Company::Get(_current_company)->money == cash);
		CHECK(std::ranges::equal(random.state, _random.state));
	}
}

/** Two real shared recipients retain the same original generator/root and compete for one purchased cable opportunity; releasing unused work rights never restores spent cable capacity. */
TEST_CASE("Atomic original recipients share the sole paid cable allowance", "[utility-local-feed-recipient-transfer]")
{
	GeothermalTubePowerRig rig; const auto source = rig.Connect();
	const auto first = RegisterGeothermalLocalFeedRecipient(rig.generator, source.supply, rig.inlets[0], 10000);
	const auto second = RegisterGeothermalLocalFeedRecipient(rig.generator, source.supply, rig.inlets[0], 10000);
	REQUIRE(first.has_value()); REQUIRE(second.has_value());
	rig.Allocate(std::array{UtilityPowerRequest{source.supply, 9000}, UtilityPowerRequest{first->consumer, 3000}, UtilityPowerRequest{second->consumer, 3000}});
	const auto path = GetTubeGeothermalSourcePath(source.source); REQUIRE(path.has_value());
	/** @param recipient Actual acquired consumer/feed pair. @param quantity Exact original-source transfer, not fabricated generation. @return Pure complete physical request sharing every original cable cell. */
	const auto request = [&](const UtilityLocalFeedRecipient &recipient, uint64_t quantity) {
		UtilityPathTransferRequest value{{source.supply, GetUtilityPowerInterval()}, recipient.consumer, rig.plant_port, quantity};
		value.legs.push_back({recipient.feed, path->cells}); return value;
	};
	REQUIRE(CommitUtilityPathTransfer(PrepareUtilityPathTransfer(request(*first, 3000))).error == UtilityPowerError::None);
	REQUIRE(CommitUtilityMutation(PrepareUtilityMutation(UtilityMutationKind::ReleaseUnused, {first->consumer, GetUtilityPowerInterval()}, 1000)).error == UtilityPowerError::None);
	const auto offer = GetUtilityPathTransferOffer(request(*second, 3000)); REQUIRE(offer.error == UtilityPowerError::None); CHECK(offer.quantity_j == 2400);
	const auto before = ComputeUtilityPowerDigest(); CHECK(PrepareUtilityPathTransfer(request(*second, 2401)).error == UtilityPowerError::InsufficientGrant);
	CHECK(ComputeUtilityPowerDigest() == before);
	REQUIRE(CommitUtilityPathTransfer(PrepareUtilityPathTransfer(request(*second, 2400))).error == UtilityPowerError::None);
	CHECK(GetUtilityPowerLink(first->feed)->flow_j == 3000); CHECK(GetUtilityPowerLink(second->feed)->flow_j == 2400);
	CHECK(GetUtilityPowerConsumer(first->consumer)->shares.front().producer == rig.output);
	CHECK(GetUtilityPowerConsumer(second->consumer)->shares.front().producer == rig.output);
	CHECK(GetUtilityPowerProducer(rig.output)->consumed_lifetime_j == 0); // This prefix imports/releases actual promises; native capsule work remains6BCD.
	CHECK(ValidateUtilityPowerState(CaptureUtilityPowerState(), GetCurrentMultiLayerProfileSelection()) == UtilityPowerError::None);
	CHECK(ValidateTubeGeothermalPowerState(CaptureTubeGeothermalPowerState(), CaptureUtilityPowerState()));
}

/** An actual still-live recipient anchors its original closed-feed history through takeover even before any energy slice exists; numeric company reuse must not split that retained native relationship. */
TEST_CASE("Unused original recipient takeover retains its closed physical source lineage", "[utility-local-feed-recipient-company]")
{
	std::unique_ptr<Company> successor;
	GeothermalTubePowerRig rig; const auto source = rig.Connect();
	const auto recipient = RegisterGeothermalLocalFeedRecipient(rig.generator, source.supply, rig.inlets[0], 10000); REQUIRE(recipient.has_value());
	REQUIRE(GetUtilityPowerConsumer(source.supply)->shares.empty()); REQUIRE(GetUtilityPowerConsumer(recipient->consumer)->shares.empty());
	REQUIRE(CloseTubeGeothermalPower(source.source) == TubePowerError::None);
	CHECK_FALSE(GetUtilityPowerLink(recipient->feed)->active); CHECK(GetUtilityPowerConsumer(recipient->consumer)->active);
	REQUIRE(Company::CanAllocateItem()); successor.reset(Company::Create()); successor->money = 1000000; successor->clear_limit = 64U << 16;
	ChangeOwnershipOfCompanyItems(_current_company, successor->index);
	CHECK(GetUtilityPowerConsumer(recipient->consumer)->owner == successor->index);
	CHECK(GetUtilityPowerConsumer(source.supply)->owner == successor->index);
	CHECK_FALSE(GetUtilityPowerConsumer(source.supply)->active); CHECK_FALSE(GetTubeGeothermalSource(source.source)->active);
	CHECK(GetTubeGeothermalSource(source.source)->owner == successor->index);
	CHECK(ValidateUtilityPowerState(CaptureUtilityPowerState(), GetCurrentMultiLayerProfileSelection()) == UtilityPowerError::None);
	CHECK(ValidateTubeGeothermalPowerState(CaptureTubeGeothermalPowerState(), CaptureUtilityPowerState()));
}
