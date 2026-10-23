/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file geothermal_town_service.cpp Whole native connection, immutable output accounting and complete physical refusal boundaries.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs18-geothermal-energy-and-industry.md Group15BBB.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
 * Explicit fixture floors/portal approaches/materials are separate from paid first-plant acceptance.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "mock_environment.h"
#include "layer_window_fixture.h"
#include "geothermal_town_service_fixture.h"
#include "../safeguards.h"

/** The actual command buffer retains only typed existing machinery and recipient identities, never caller-created accounts or energy. */
TEST_CASE("Geothermal town connection arguments use native network transport", "[geothermal-service-serialization]")
{
	using Args = CommandTraits<Commands::ConnectGeothermalTownService>::Args;
	const Args args{IndustryID{3}, IndustryID{4}, IndustryID{5}, StationID{6}, TownID{7}};
	CHECK(EndianBufferReader::ToValue<Args>(EndianBufferWriter<CommandDataBuffer>::FromValue(args)) == args);
}

/** Both dated actual generators connect through one/two paid directions without generation at registration or a duplicate town offer. */
TEST_CASE("Native geothermal town connection publishes all participants once", "[geothermal-service-connection]")
{
	const bool direct = GENERATE(false, true);
	const auto model = GENERATE(GeothermalGeneratorModel::VentTap, GeothermalGeneratorModel::MagmaLoop);
	MockEnvironment::Instance(); LoadGeneratedWindowTestLanguage();
	GeothermalTownServiceRig rig{direct, model};
	const auto before = CaptureUtilityPowerState(); const auto digest = ComputeUtilityPowerDigest();
	REQUIRE(IsGeothermalGeneratorConstructionComplete(rig.generator)); REQUIRE(HasGeothermalGeneratorInstalledPorts(rig.generator));
	const auto *machine = GetGeothermalGeneratorState(rig.generator); REQUIRE(machine != nullptr);
	INFO("model=" << static_cast<uint>(to_underlying(model)) << " direct=" << direct << " output=" << machine->electrical_producer.base() << " active=" << machine->active_service_ticks);
	REQUIRE(GetUtilityPowerProducer(machine->electrical_producer) != nullptr);
	REQUIRE(ValidateUtilityPowerState(before, GetCurrentMultiLayerProfileSelection()) == UtilityPowerError::None);
	INFO("town authority range=" << static_cast<uint>(_settings_game.economy.dist_local_authority));
	CHECK(ClosestTownFromTile(rig.platform, _settings_game.economy.dist_local_authority) == Town::Get(rig.service.town));
	const auto native = ComputeGeothermalGeneratorDigest(), geology = ComputeGeologyStateDigest(GetGeologyWorldState());
	const auto cash = Company::Get(_current_company)->money; const auto random = _random;
	const auto proposed = ConnectGeothermalTownService(rig.service, false); REQUIRE(proposed.error == UtilityPowerError::None);
	REQUIRE(rig.Connect().Succeeded()); CHECK(ComputeUtilityPowerDigest() == digest);
	REQUIRE(rig.Connect(DoCommandFlag::Execute).Succeeded());
	const auto after = CaptureUtilityPowerState();
	CHECK(after.next_consumer == before.next_consumer + (direct ? 2 : 3)); CHECK(after.next_link == before.next_link + (direct ? 1 : 2));
	CHECK(after.next_producer == before.next_producer); CHECK(after.next_share == before.next_share);
	CHECK(after.operation_sequence == before.operation_sequence + 1); CHECK(after.interval == before.interval);
	CHECK(ComputeGeothermalGeneratorDigest() == native); CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == geology);
	CHECK(Company::Get(_current_company)->money == cash); CHECK(std::ranges::equal(random.state, _random.state));
	const auto *town = GetUtilityPowerConsumer(proposed.destination); REQUIRE(town != nullptr);
	CHECK(town->town == rig.service.town); CHECK(town->town_terminal == rig.service.terminal); CHECK(town->town_supply == proposed.source);
	CHECK(town->shares.empty()); CHECK(town->meter.granted_energy_j == 0); CHECK_FALSE(town->town_effect_applied);
	const auto output = GetGeothermalGeneratorState(rig.generator)->electrical_producer;
	CHECK(GetUtilityPowerProducer(output)->generated_lifetime_j == (model == GeothermalGeneratorModel::VentTap ? 4050 : 9450));
	CHECK(GetUtilityPowerProducer(output)->consumed_lifetime_j == 0);
	const auto connected = ComputeUtilityPowerDigest(); CHECK(rig.Connect().Failed()); CHECK(rig.Connect(DoCommandFlag::Execute).Failed());
	CHECK(ComputeUtilityPowerDigest() == connected);
	rig.Operate(); town = GetUtilityPowerConsumer(proposed.destination);
	CHECK(town->town_bonus_percent == 25); CHECK(town->town_effect_applied);
	CHECK(GetUtilityPowerProducer(output)->consumed_lifetime_j == 135);
	CHECK(GetUtilityPowerLink(proposed.links[0])->flow_j == 135);
	if (!direct) CHECK(GetUtilityPowerLink(proposed.links[1])->flow_j == 135);
	if (direct) CHECK(GetGeologyWorldState().terrain.Get(TileXYL(44, 12, MapLayer::Underground)) == ExcavationState::Solid);
	const auto settled = ComputeUtilityPowerDigest(); const auto growth = Town::Get(rig.service.town)->growth_rate;
	RunUtilityTownServices(); CHECK(ComputeUtilityPowerDigest() == settled); CHECK(Town::Get(rig.service.town)->growth_rate == growth);
	CHECK(ValidateUtilityPowerState(CaptureUtilityPowerState(), GetCurrentMultiLayerProfileSelection()) == UtilityPowerError::None);
}

/** A late missing physical/recipient participant cannot leave earlier accounts or links issued; both command phases refuse unchanged. */
TEST_CASE("Native geothermal connection refuses whole invalid selections", "[geothermal-service-refusal]")
{
	MockEnvironment::Instance(); LoadGeneratedWindowTestLanguage();
	GeothermalTownServiceRig rig; AutoRestoreBackup actor{_current_company}; AutoRestoreBackup mode{_game_mode};
	SECTION("missing actual generator") { rig.service.generator = IndustryID::Invalid(); }
	SECTION("missing first machinery") { rig.service.first_riser = IndustryID::Invalid(); }
	SECTION("missing Underground sender") { rig.service.second_riser = IndustryID::Invalid(); }
	SECTION("reversed actual route") { std::swap(rig.service.first_riser, rig.service.second_riser); }
	SECTION("missing actual terminal") { rig.service.terminal = StationID::Invalid(); }
	SECTION("missing actual town") { rig.service.town = TownID::Invalid(); }
	SECTION("foreign actor") { _current_company = COMPANY_SPECTATOR; }
	SECTION("editor policy") { _game_mode = GameMode::Editor; }
	SECTION("locked calendar") { TimerGameCalendar::date = TimerGameCalendar::ConvertYMDToDate(TimerGameCalendar::Year{1979}, 11, 31); }
	SECTION("actual sealed pair") { REQUIRE(Command<Commands::ChangeLayerPortalConnection>::Do(DoCommandFlag::Execute,
		GetGeothermalRiserState(rig.service.second_riser)->site.portal, false).Succeeded()); }
	SECTION("actual removed relay cable") {
		const auto tile = rig.relay_cable[rig.relay_cable.size() / 2];
		REQUIRE(Command<Commands::RemoveGeothermalUtilityLink>::Unsafe<CommandCallback>(INVALID_STRING_ID, nullptr, false, false, tile,
			std::make_tuple(tile, UtilityEnergyDomain::Electricity)).Succeeded());
	}
	SECTION("actual station facility removed") { REQUIRE(Command<Commands::RemoveFromRailStation>::Do(DoCommandFlag::Execute, rig.platform, rig.platform, false).Succeeded()); }
	const auto utility = ComputeUtilityPowerDigest(), native = ComputeGeothermalGeneratorDigest(), geology = ComputeGeologyStateDigest(GetGeologyWorldState());
	const auto random = _random;
	// Each invalid request traverses both quote and execution with the same
	// native owners; a late bad terminal/path cannot consume an earlier cursor.
	for (const auto flags : {DoCommandFlags{}, DoCommandFlags{DoCommandFlag::Execute}}) {
		CHECK(rig.Connect(flags).Failed()); CHECK(ComputeUtilityPowerDigest() == utility);
		CHECK(ComputeGeothermalGeneratorDigest() == native); CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == geology);
		CHECK(std::ranges::equal(random.state, _random.state));
	}
}

/** The complete route needs all prospective slots; exhaustion before its last town account never registers a partial source or relay. */
TEST_CASE("Native geothermal connection refuses complete retained storage exhaustion", "[geothermal-service-capacity]")
{
	MockEnvironment::Instance(); LoadGeneratedWindowTestLanguage(); GeothermalTownServiceRig rig;
	// Fill only real zero-demand native records through the public issuer.
	// Leaving two slots would admit a partial source/relay but not the town;
	// the whole command must instead preserve every original identity.
	const auto original_count = CaptureUtilityPowerState().consumers.size();
	for (size_t count = original_count; count < MAX_UTILITY_INTERVAL_ACCOUNTS - 2; ++count) {
		REQUIRE(RegisterUtilityPowerConsumer(_current_company, MapLayer::Surface, UtilityConsumerEffect::AssemblyWork, 0, {}).has_value());
	}
	const auto digest = ComputeUtilityPowerDigest(); CHECK(rig.Connect().Failed()); CHECK(rig.Connect(DoCommandFlag::Execute).Failed());
	CHECK(ComputeUtilityPowerDigest() == digest);
}
