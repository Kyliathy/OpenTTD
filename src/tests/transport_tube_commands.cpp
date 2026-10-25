/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file transport_tube_commands.cpp Native command estimates, payment custody and normal tube registration boundaries.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Group12B.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
 * Explicit commissioned fixture geometry does not establish normal-game bootstrap or useful-route acceptance.
 */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "transport_tube_power_fixture.h"
#include "../transport_tube_model.h"
#include "../transport_tube_cmd.h"
#include "../command_func.h"
#include "../multilayer_state.h"
#include "../safeguards.h"

/**
 * Check that a binding has no usable electricity before its next real allocation.
 * @param consumer Exact existing/new/retired binding identity; an unallocated or retired account may have no view.
 */
static void CheckTubeCommandHasNoGrant(UtilityConsumerID consumer)
{
	const auto allocation = GetUtilityAllocation(consumer);
	// Both absent allocation and a zero available balance represent no usable
	// electricity; binding/recovery must never create a positive reservation.
	CHECK((!allocation.has_value() || allocation->granted_energy_j == 0));
}

/** Normal content installation requires no saved tube state and keeps all five identities/date restrictions. */
TEST_CASE("Normal exact content installs five tube definitions without saved tube metadata", "[transport-tube-normal-registration]")
{
	TBMEngineFixture world;
	REQUIRE(GetPendingMultiLayerStateSchema() == nullptr);
	REQUIRE(GetTubeRailType() == INVALID_RAILTYPE);
	const auto count = Engine::GetNumItems();
	SECTION("exact active content") {
		REQUIRE(InitialiseTubeContentDefinitions(world.package));
		CHECK(Engine::GetNumItems() == count + 5);
		CHECK(GetRailTypeInfo(GetTubeRailType())->flags.Test(RailTypeFlag::Hidden));
		// Each original keeps its own EIDS identity. Repeated post-polish setup
		// must neither allocate new models nor depend on a saved service source.
		for (uint16_t model = 9; model <= 13; ++model) {
			const auto capability = GetTubeVehicleCapability(GetTubeEngine(model));
			REQUIRE(capability.has_value());
			CHECK(capability->catalogue->model_id == model);
			CHECK(Engine::Get(capability->engine)->grf_prop.local_id == 0x8000 + model);
		}
		CHECK(InitialiseTubeContentDefinitions(world.package));
		CHECK(Engine::GetNumItems() == count + 5);
	}
	SECTION("foreign package refuses before registry allocation") {
		world.package.front()->ident.grfid = GrfID{"FAKE"};
		CHECK_FALSE(InitialiseTubeContentDefinitions(world.package));
		CHECK(GetTubeRailType() == INVALID_RAILTYPE);
		CHECK(Engine::GetNumItems() == count);
	}
	SECTION("absent package refuses before registry allocation") {
		CHECK_FALSE(InitialiseTubeContentDefinitions({}));
		CHECK(GetTubeRailType() == INVALID_RAILTYPE);
		CHECK(Engine::GetNumItems() == count);
	}
}

/** Synchronized connection uses the existing physical lease invoice exactly once, including its estimate. */
TEST_CASE("Tube service commands preserve estimates and charge one native lease", "[transport-tube-commands]")
{
	TubePowerRig rig;
	const TubePowerCommandPath path{rig.path};
	const auto cash = Company::Get(_current_company)->money;
	const auto ledger = ComputeUtilityPowerDigest(), physical = ComputeTubePowerDigest();
	const auto estimate = Command<Commands::ConnectTubePower>::Do({}, rig.portal, rig.depot, path);
	REQUIRE(estimate.Succeeded());
	CHECK(estimate.GetCost() == 0);
	CHECK(ComputeUtilityPowerDigest() == ledger);
	CHECK(ComputeTubePowerDigest() == physical);
	CHECK(Company::Get(_current_company)->money == cash);
	REQUIRE(Command<Commands::ConnectTubePower>::Do(DoCommandFlag::Execute, rig.portal, rig.depot, path).Succeeded());
	const auto rows = CaptureTubePowerConnections();
	REQUIRE(rows.size() == 1);
	CHECK(Company::Get(_current_company)->money == cash - Money{20});
	CHECK(rows.front().invoiced_money == 20);
	CHECK(GetUtilityPowerProducer(rows.front().producer)->meter.generation_energy_j == 0);
	CHECK(Command<Commands::ConnectTubePower>::Do(DoCommandFlag::Execute, rig.portal, rig.depot, path).Failed());
	CHECK(Company::Get(_current_company)->money == cash - Money{20});
}

/** Every execution path shares ownership/date/topology/funds checks before source IDs or money are acquired. */
TEST_CASE("Tube synchronized service refuses invalid admission without state changes", "[transport-tube-command-refusal]")
{
	TubePowerRig rig;
	TubePowerCommandPath path{rig.path};
	SECTION("insufficient lease") { Company::Get(_current_company)->money = Money{19}; }
	SECTION("disconnected corridor") { path.count = 0; }
	SECTION("solid corridor") { path.cells[0] = TileXYL(35, 39, MapLayer::Underground); }
	SECTION("oversized serialized corridor") { path.count = MAX_TUBE_POWER_PATH + 1; }
	SECTION("unintroduced service") {
		TimerGameCalendar::date = TimerGameCalendar::ConvertYMDToDate(TimerGameCalendar::Year{1969}, 0, 1);
	}
	SECTION("wrong company") { _current_company = OWNER_NONE; }
	const auto ledger = ComputeUtilityPowerDigest(), physical = ComputeTubePowerDigest();
	const auto owner = rig.capsule->owner;
	const auto cash = Company::Get(owner)->money;
	// Estimates and real commands must refuse the same malformed request;
	// a failed preview cannot consume a source identity or reserve electricity.
	for (DoCommandFlags flags : {DoCommandFlags{}, DoCommandFlags{DoCommandFlag::Execute}}) {
		CHECK(Command<Commands::ConnectTubePower>::Do(flags, rig.portal, rig.depot, path).Failed());
	}
	CHECK(ComputeUtilityPowerDigest() == ledger);
	CHECK(ComputeTubePowerDigest() == physical);
	CHECK(Company::Get(owner)->money == cash);
	_current_company = owner;
}

/** Explicit binding never grants energy; synchronized closure settles a fractional usage invoice only once. */
TEST_CASE("Tube vehicle bind and close commands retain source accounting custody", "[transport-tube-command-binding]")
{
	TubePowerRig rig;
	rig.Connect();
	const auto cash = Company::Get(_current_company)->money;
	const auto physical = ComputeTubePowerDigest(), ledger = ComputeUtilityPowerDigest();
	const auto vehicles = ComputeTubePowerVehicleDigest();
	REQUIRE(Command<Commands::SetTubeVehiclePower>::Do({}, rig.capsule->index, rig.source, false).Succeeded());
	CHECK(ComputeTubePowerDigest() == physical);
	CHECK(ComputeUtilityPowerDigest() == ledger);
	CHECK(ComputeTubePowerVehicleDigest() == vehicles);
	REQUIRE(Command<Commands::SetTubeVehiclePower>::Do(DoCommandFlag::Execute, rig.capsule->index, rig.source, false).Succeeded());
	const auto *binding = GetTubePowerVehicle(rig.capsule->index);
	REQUIRE(binding != nullptr);
	CheckTubeCommandHasNoGrant(binding->consumer);
	CHECK(Company::Get(_current_company)->money == cash);
	CHECK(Command<Commands::SetTubeVehiclePower>::Do(DoCommandFlag::Execute, rig.capsule->index, rig.source, false).Failed());
	CHECK(Command<Commands::SetTubeVehiclePower>::Do(DoCommandFlag::Execute, rig.capsule->index, rig.source, true).Failed());
	rig.Offer();
	CapsuleDepotService work(*rig.capsule);
	REQUIRE(ConsumeTubeExternalPower(rig.source, rig.consumer, 540, work).error == UtilityPowerError::None);
	CHECK(GetTubePowerConnection(rig.source)->usage_remainder_j == 540);
	REQUIRE(Command<Commands::CloseTubePower>::Do({}, rig.source).Succeeded());
	CHECK(Company::Get(_current_company)->money == cash);
	CHECK(GetTubePowerConnection(rig.source)->usage_remainder_j == 540);
	REQUIRE(Command<Commands::CloseTubePower>::Do(DoCommandFlag::Execute, rig.source).Succeeded());
	CHECK(Company::Get(_current_company)->money == cash - Money{1});
	CHECK(GetTubePowerConnection(rig.source)->invoiced_money == 21);
	CHECK(GetTubePowerConnection(rig.source)->usage_remainder_j == 0);
	CHECK(Command<Commands::CloseTubePower>::Do(DoCommandFlag::Execute, rig.source).Failed());
	CHECK(Company::Get(_current_company)->money == cash - Money{1});
	const auto original_consumer = binding->consumer;
	REQUIRE(CargoPacket::CanAllocateItem());
	auto *packet = CargoPacket::Create(StationID::Invalid(), uint16_t{17}, Source{Source::Invalid, SourceType::Industry});
	rig.capsule->cargo.Append(packet, VehicleCargoList::MoveToAction::Keep);
	const auto packet_id = packet->index;
	const TubePowerCommandPath path{rig.path};
	REQUIRE(Command<Commands::ConnectTubePower>::Do(DoCommandFlag::Execute, rig.portal, rig.depot, path).Succeeded());
	const auto connections = CaptureTubePowerConnections();
	REQUIRE(connections.size() == 2);
	const auto replacement = connections.back().producer;
	CHECK(replacement != rig.source);
	const auto before_reconnect = ComputeTubePowerVehicleDigest();
	REQUIRE(Command<Commands::SetTubeVehiclePower>::Do({}, rig.capsule->index, replacement, true).Succeeded());
	CHECK(ComputeTubePowerVehicleDigest() == before_reconnect);
	REQUIRE(Command<Commands::SetTubeVehiclePower>::Do(DoCommandFlag::Execute, rig.capsule->index, replacement, true).Succeeded());
	const auto *rebound = GetTubePowerVehicle(rig.capsule->index);
	REQUIRE(rebound != nullptr);
	CHECK(rebound->source == replacement);
	CHECK(rebound->consumer != original_consumer);
	CheckTubeCommandHasNoGrant(rebound->consumer);
	CheckTubeCommandHasNoGrant(original_consumer);
	CHECK(rig.capsule->cargo.StoredCount() == 17);
	CHECK(rig.capsule->cargo.Packets()->front()->index == packet_id);
	CHECK(Company::Get(_current_company)->money == cash - Money{21});
}

/** Every service/binding action checks the synchronized actor rather than inferring permission from a source or vehicle ID. */
TEST_CASE("Tube synchronized source and capsule commands reject a different actor", "[transport-tube-command-owner]")
{
	TubePowerRig rig;
	rig.Connect();
	const auto cash = Company::Get(_current_company)->money;
	const auto ledger = ComputeUtilityPowerDigest(), physical = ComputeTubePowerDigest();
	const auto vehicles = ComputeTubePowerVehicleDigest();
	{
		AutoRestoreBackup actor{_current_company, OWNER_NONE};
		// Both modes must check the current company before allocating a binding
		// or retiring an existing paid source; typed identities convey no authority.
		for (DoCommandFlags flags : {DoCommandFlags{}, DoCommandFlags{DoCommandFlag::Execute}}) {
			CHECK(Command<Commands::CloseTubePower>::Do(flags, rig.source).Failed());
			CHECK(Command<Commands::SetTubeVehiclePower>::Do(flags, rig.capsule->index, rig.source, false).Failed());
			CHECK(Command<Commands::SetTubeVehiclePower>::Do(flags, rig.capsule->index, rig.source, true).Failed());
		}
	}
	CHECK(ComputeUtilityPowerDigest() == ledger);
	CHECK(ComputeTubePowerDigest() == physical);
	CHECK(ComputeTubePowerVehicleDigest() == vehicles);
	CHECK(Company::Get(_current_company)->money == cash);
}

/** Actual command traits serialize explicit world cells and typed source/vehicle identities for replay and networking. */
TEST_CASE("Tube service command arguments round trip through native network buffers", "[transport-tube-command-serialization]")
{
	static_assert(to_underlying(Commands::ConnectTubePower) == 217);
	static_assert(to_underlying(Commands::CloseTubePower) == 218);
	static_assert(to_underlying(Commands::SetTubeVehiclePower) == 219);
	using Connect = CommandTraits<Commands::ConnectTubePower>::Args;
	const Connect connect{TileIndex{123}, TileIndex{456}, TubePowerCommandPath{std::array{TileIndex{124}, TileIndex{456}}}};
	CHECK(EndianBufferReader::ToValue<Connect>(EndianBufferWriter<CommandDataBuffer>::FromValue(connect)) == connect);
	using Close = CommandTraits<Commands::CloseTubePower>::Args;
	const Close close{UtilityProducerID{7}};
	CHECK(EndianBufferReader::ToValue<Close>(EndianBufferWriter<CommandDataBuffer>::FromValue(close)) == close);
	using Bind = CommandTraits<Commands::SetTubeVehiclePower>::Args;
	const Bind bind{VehicleID{8}, UtilityProducerID{7}, true};
	CHECK(EndianBufferReader::ToValue<Bind>(EndianBufferWriter<CommandDataBuffer>::FromValue(bind)) == bind);
	TubePowerCommandPath oversized;
	oversized.count = MAX_TUBE_POWER_PATH + 1;
	const auto oversized_buffer = EndianBufferWriter<CommandDataBuffer>::FromValue(oversized);
	CHECK(oversized_buffer.size() == sizeof(uint16_t));
	CHECK(EndianBufferReader::ToValue<TubePowerCommandPath>(oversized_buffer).count == oversized.count);
	TubePowerCommandPath longest;
	longest.count = MAX_TUBE_POWER_PATH;
	// Exercise the actual maximum wire request, ensuring the last world cell is
	// retained rather than accidentally dropped at the fixed network boundary.
	for (uint16_t index = 0; index < longest.count; ++index) longest.cells[index] = TileIndex{index + 123U};
	CHECK(EndianBufferReader::ToValue<TubePowerCommandPath>(EndianBufferWriter<CommandDataBuffer>::FromValue(longest)) == longest);
}
