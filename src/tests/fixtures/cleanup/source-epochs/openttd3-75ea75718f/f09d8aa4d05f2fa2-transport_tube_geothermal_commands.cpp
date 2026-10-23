/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file transport_tube_geothermal_commands.cpp Actual synchronized geothermal inlet/capsule command admission and original-source custody.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Groups6BDB/6BDC.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../transport_tube_cmd.h"
#include "geothermal_tube_switch_fixture.h"
#include "../safeguards.h"

/** The real command kernel creates/retire references only; construction, generator service, actual repairs and original energy retain their existing owners. */
TEST_CASE("Native geothermal source commands preserve payment and generation custody", "[tube-geothermal-command-source]")
{
	MockEnvironment::Instance(); GeothermalTubeWorkRig rig; auto *capsule = rig.Buy(); const auto packet = rig.Load(*capsule);
	const auto cash = Company::Get(_current_company)->money;
	const auto native = ComputeGeothermalGeneratorDigest(), metadata = ComputeTubeGeothermalPowerDigest(), utility = ComputeUtilityPowerDigest();
	const auto generated = GetUtilityPowerProducer(rig.output)->generated_lifetime_j;
	const auto args = std::make_tuple(rig.generator, rig.inlets[0], rig.output);
	const auto quote = Command<Commands::ConnectTubeGeothermalPower>::Unsafe<CommandCallback>(INVALID_STRING_ID, nullptr, false, true, rig.inlets[0], args);
	REQUIRE(quote.Succeeded()); CHECK(quote.GetCost() == Money{0}); CHECK(ComputeTubeGeothermalPowerDigest() == metadata); CHECK(ComputeUtilityPowerDigest() == utility);
	REQUIRE(Command<Commands::ConnectTubeGeothermalPower>::Unsafe<CommandCallback>(INVALID_STRING_ID, nullptr, false, false, rig.inlets[0], args).Succeeded());
	const auto sources = CaptureTubeGeothermalPowerState(); REQUIRE(sources.sources.size() == 1); const auto source = sources.sources.front();
	CHECK(source.generator == rig.generator); CHECK(source.producer == rig.output); CHECK(Company::Get(_current_company)->money == cash);
	CHECK(GetUtilityPowerProducer(rig.output)->generated_lifetime_j == generated); CHECK(ComputeGeothermalGeneratorDigest() == native);
	CHECK_FALSE(GetUtilityAllocation(source.supply).has_value());
	CHECK(Command<Commands::ConnectTubeGeothermalPower>::Do(DoCommandFlag::Execute, rig.generator, rig.inlets[0], rig.output).Failed());
	const auto binding = ComputeTubePowerVehicleDigest(), before_bind = ComputeUtilityPowerDigest();
	REQUIRE(Command<Commands::SetTubeVehicleGeothermalPower>::Do({}, capsule->index, source.id, false).Succeeded());
	CHECK(ComputeTubePowerVehicleDigest() == binding); CHECK(ComputeUtilityPowerDigest() == before_bind);
	REQUIRE(Command<Commands::SetTubeVehicleGeothermalPower>::Unsafe<CommandCallback>(INVALID_STRING_ID, nullptr, false, false, capsule->tile,
		std::make_tuple(capsule->index, source.id, false)).Succeeded());
	CHECK(Company::Get(_current_company)->money == cash); CHECK(capsule->cargo.Packets()->front()->index == packet);
	capsule->reliability = 100; rig.Run(std::array{capsule}); REQUIRE(capsule->reliability == capsule->GetEngine()->reliability);
	const auto used = GetUtilityPowerProducer(rig.output)->consumed_lifetime_j; CHECK(used == 540);
	const auto before_close = ComputeUtilityPowerDigest(), before_source = ComputeTubeGeothermalPowerDigest(); const auto close_cash = Company::Get(_current_company)->money;
	REQUIRE(Command<Commands::CloseTubeGeothermalPower>::Do({}, source.id).Succeeded());
	CHECK(ComputeUtilityPowerDigest() == before_close); CHECK(ComputeTubeGeothermalPowerDigest() == before_source);
	REQUIRE(Command<Commands::CloseTubeGeothermalPower>::Unsafe<CommandCallback>(INVALID_STRING_ID, nullptr, false, false, source.inlet, std::make_tuple(source.id)).Succeeded());
	CHECK_FALSE(GetTubeGeothermalSource(source.id)->active); CHECK(GetUtilityPowerProducer(rig.output)->active); CHECK(IsGeothermalGeneratorConstructionComplete(rig.generator));
	CHECK(GetUtilityPowerProducer(rig.output)->consumed_lifetime_j == used); CHECK(Company::Get(_current_company)->money == close_cash);
	CHECK(Command<Commands::CloseTubeGeothermalPower>::Do(DoCommandFlag::Execute, source.id).Failed());
	CHECK(capsule->cargo.Packets()->front()->index == packet); CHECK(capsule->cargo.StoredCount() == 17);
	CHECK(ValidateTubePowerVehicles(CaptureTubePowerVehicles()) == TubePowerError::None);
}

/** Both synchronized source choices act on actual held native custody and preserve original invoices, consumed energy and the one admitted fraction. */
TEST_CASE("Native capsule commands switch both actual provider families", "[tube-geothermal-command-switch]")
{
	MockEnvironment::Instance(); GeothermalTubeSwitchRig rig; auto *capsule = rig.Buy(); const auto packet = rig.Load(*capsule);
	const bool initially_native = GENERATE(false, true);
	const auto initial = initially_native ? Command<Commands::SetTubeVehicleGeothermalPower>::Do(DoCommandFlag::Execute, capsule->index, rig.geothermal.source, false) :
		Command<Commands::SetTubeVehiclePower>::Do(DoCommandFlag::Execute, capsule->index, rig.external, false);
	REQUIRE(initial.Succeeded()); rig.Start(*capsule, rig.inlets[1]); rig.StopOnRoute(*capsule);
	const auto old = *GetTubePowerVehicle(capsule->index); REQUIRE(old.remainder_mj != 0);
	const auto ledger = ComputeUtilityPowerDigest(), bindings = ComputeTubePowerVehicleDigest(), invoice = ComputeTubePowerDigest();
	const auto cash = Company::Get(_current_company)->money;
	const auto quote = initially_native ? Command<Commands::SetTubeVehiclePower>::Do({}, capsule->index, rig.external, true) :
		Command<Commands::SetTubeVehicleGeothermalPower>::Do({}, capsule->index, rig.geothermal.source, true);
	REQUIRE(quote.Succeeded()); CHECK(ComputeUtilityPowerDigest() == ledger); CHECK(ComputeTubePowerVehicleDigest() == bindings);
	const auto actual = initially_native ? Command<Commands::SetTubeVehiclePower>::Unsafe<CommandCallback>(INVALID_STRING_ID, nullptr, false, false, capsule->tile,
		std::make_tuple(capsule->index, rig.external, true)) : Command<Commands::SetTubeVehicleGeothermalPower>::Unsafe<CommandCallback>(INVALID_STRING_ID, nullptr, false, false, capsule->tile,
		std::make_tuple(capsule->index, rig.geothermal.source, true));
	REQUIRE(actual.Succeeded()); CHECK(actual.GetCost() == Money{0}); const auto current = *GetTubePowerVehicle(capsule->index);
	CHECK(current.consumer != old.consumer); CHECK(current.remainder_mj == old.remainder_mj); CHECK(CaptureTubePowerVehicles().front().remainder_mj == 0);
	CHECK(ComputeTubePowerDigest() == invoice); CHECK(Company::Get(_current_company)->money == cash); CHECK(capsule->cargo.Packets()->front()->index == packet);
	CHECK(capsule->vehstatus.Test(VehState::Stopped)); CHECK(ValidateTubePowerVehicles(CaptureTubePowerVehicles()) == TubePowerError::None);
}

/** Current native owner, date, physical path and original identity remain authoritative through both command modes; an explicit source number conveys no permission. */
TEST_CASE("Native geothermal source commands refuse stale and unauthorized inputs unchanged", "[tube-geothermal-command-refusal]")
{
	MockEnvironment::Instance(); GeothermalTubeWorkRig rig; auto *capsule = rig.Buy(); const auto source = rig.Connect();
	AutoRestoreBackup actor(_current_company); AutoRestoreBackup date(TimerGameCalendar::date);
	auto generator = rig.generator; auto inlet = rig.inlets[1]; auto identity = source.source;
	SECTION("foreign actor") { _current_company = OWNER_NONE; }
	SECTION("lost original generator") { generator = IndustryID::Invalid(); }
	SECTION("missing native inlet") { inlet = TileXYL(31, 26, MapLayer::DeepUnderground); }
	SECTION("locked original machinery") { TimerGameCalendar::date = TimerGameCalendar::ConvertYMDToDate(TimerGameCalendar::Year{1999}, 0, 1); }
	SECTION("unknown source") { identity = TubeGeothermalSourceID{999}; }
	const auto owner = capsule->owner; const auto cash = Company::Get(owner)->money;
	const auto utility = ComputeUtilityPowerDigest(), metadata = ComputeTubeGeothermalPowerDigest(), bindings = ComputeTubePowerVehicleDigest();
	// Each malformed family is exercised in both native estimate and execute
	// modes; only genuinely invalid inputs are sent to the matching command.
	for (const DoCommandFlags flags : {DoCommandFlags{}, DoCommandFlags{DoCommandFlag::Execute}}) {
		if (identity.base() == 999) {
			CHECK(Command<Commands::CloseTubeGeothermalPower>::Do(flags, identity).Failed());
			CHECK(Command<Commands::SetTubeVehicleGeothermalPower>::Do(flags, capsule->index, identity, false).Failed());
		} else {
			CHECK(Command<Commands::ConnectTubeGeothermalPower>::Do(flags, generator, inlet, rig.output).Failed());
		}
		if (_current_company != owner) {
			CHECK(Command<Commands::CloseTubeGeothermalPower>::Do(flags, source.source).Failed());
			CHECK(Command<Commands::SetTubeVehicleGeothermalPower>::Do(flags, capsule->index, source.source, false).Failed());
		}
	}
	CHECK(ComputeUtilityPowerDigest() == utility); CHECK(ComputeTubeGeothermalPowerDigest() == metadata); CHECK(ComputeTubePowerVehicleDigest() == bindings);
	CHECK(Company::Get(owner)->money == cash);
}

/** Native command payloads preserve distinct fixed-width original identities without variable-length allocation or changing an existing command ID. */
TEST_CASE("Native geothermal source commands retain bounded typed wire arguments", "[tube-geothermal-command-wire]")
{
	using Connect = CommandTraits<Commands::ConnectTubeGeothermalPower>::Args;
	const Connect connect{IndustryID{42}, TileIndex{1234}, UtilityProducerID{76}};
	const auto connect_wire = EndianBufferWriter<CommandDataBuffer>::FromValue(connect);
	CHECK(EndianBufferReader::ToValue<Connect>(connect_wire) == connect); CHECK(connect_wire.size() == sizeof(IndustryID::BaseType) + 2 * sizeof(uint32_t));
	using Close = CommandTraits<Commands::CloseTubeGeothermalPower>::Args;
	const Close close{TubeGeothermalSourceID{23}};
	const auto close_wire = EndianBufferWriter<CommandDataBuffer>::FromValue(close);
	CHECK(EndianBufferReader::ToValue<Close>(close_wire) == close); CHECK(close_wire.size() == sizeof(uint32_t));
	using Bind = CommandTraits<Commands::SetTubeVehicleGeothermalPower>::Args;
	const Bind bind{VehicleID{9}, TubeGeothermalSourceID{23}, true};
	const auto bind_wire = EndianBufferWriter<CommandDataBuffer>::FromValue(bind);
	CHECK(EndianBufferReader::ToValue<Bind>(bind_wire) == bind);
	CHECK(to_underlying(Commands::ConnectTubeGeothermalPower) == to_underlying(Commands::ConnectGeothermalTownService) + 1);
}
