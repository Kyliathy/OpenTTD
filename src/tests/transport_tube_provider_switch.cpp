/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file transport_tube_provider_switch.cpp Whole held-capsule provider switching with real original generation, native service and scoped unused rights.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Groups6BC/6BDB.
 * Prepared floor/portal approaches, real packet inputs and a valid saved demand fraction are explicit component inputs; production scheduler and driven-route proof remain34-35.
 */
#include "../stdafx.h"
#include <any> // Load standard allocation internals before fixture safeguards prohibit raw application allocation.
#include "../3rdparty/catch2/catch.hpp"
#include "geothermal_tube_capsule_fixture.h"
#include "transport_tube_power_fixture.h"
#include "../transport_tube_cmd.h"
#include "../transport_tube_sprite.h"
#include "../ground_sprite.h"
#include "../layer_portal_cmd.h"
#include "../layer_portal_map.h"
#include "../track_func.h"
#include "../road.h"
#include "../safeguards.h"

/** Actual generator, paid native/external services and connected depots; no feature-local energy balance or production tick driver. */
class TubeProviderSwitchRig : public GeothermalTubeCapsuleRig {
public:
	TubeGeothermalPowerResult native; ///< Original completed generator/root/inlet association.
	UtilityProducerID external{UINT32_MAX}; ///< Separately paid original external lease at the same inlet.
	/** Prepare disclosed floor/approaches through native portal/rail commands, then buy the real lease and original native association. */
	TubeProviderSwitchRig()
	{
		ResetLayerPortalWorkState(); this->JoinDepots();
		const auto portal = TileXYL(32, 22, MapLayer::DeepUnderground);
		const auto upper = MultiLayerMap::TileInLayer(portal, MapLayer::Underground);
		std::vector<TileIndex> floor;
		// Both real endpoint footprints and approach rims are component inputs;
		// the native connection controller still requires the complete paired geometry.
		for (const auto layer : {MapLayer::DeepUnderground, MapLayer::Underground}) {
			for (int y = 20; y <= 23; ++y) {
				for (int x = 30; x <= 34; ++x) floor.push_back(TileXYL(x, y, layer));
			}
		}
		this->Open(floor);
		REQUIRE(CmdBuildLayerPortal(DoCommandFlag::Execute, upper, TransportType::Rail,
			MapLayer::DeepUnderground, DiagDirection::SE, RAILTYPE_RAIL, ROADTYPE_ROAD).Succeeded());
		// Native approach tiles, ownership and commissioning stamps must agree;
		// no numeric Connected record or guessed portal revision is installed.
		for (const auto endpoint : {portal, upper}) {
			REQUIRE(CmdBuildSingleRail(DoCommandFlag::Execute, GetLayerPortalDoorTile(endpoint), RAILTYPE_RAIL,
				AxisToTrack(DiagDirToAxis(GetLayerPortalShaftDirection(endpoint))), false).Succeeded());
		}
		REQUIRE(ChangeLayerPortalConnection(portal, _current_company, true, true) == LayerPortalWorkError::None);
		const std::array path{TileXYL(32, 23, MapLayer::DeepUnderground), this->inlets[0]};
		const auto lease = ConnectTubeExternalPower(portal, this->inlets[0], path);
		REQUIRE(lease.error == TubePowerError::None); this->external = lease.producer; this->native = this->Connect();
	}
	/** Release only the explicit portal fixture while inherited native vehicle/source/company owners still exist. */
	~TubeProviderSwitchRig() { ResetLayerPortalWorkState(); }
	/** @param capsule Actual stopped owned front. @param geothermal Select the original native inlet instead of the paid external lease. @param flags Pure quote or actual synchronized command execution. @return Complete provider switch or unchanged refusal. */
	CommandCost Switch(const Train &capsule, bool geothermal, DoCommandFlags flags = {})
	{
		return geothermal ? Command<Commands::SetTubeVehicleGeothermalPower>::Do(flags, capsule.index, this->native.source, true) :
			Command<Commands::SetTubeVehiclePower>::Do(flags, capsule.index, this->external, true);
	}
	/** @param capsule Actual bound held capsule. @param geothermal Current provider family. @param quantity Actual shared request, bounded by paid output and installed cable. Publish native components and allocate once without activating the later production scheduler. */
	void AllocateFor(const Train &capsule, bool geothermal, uint64_t quantity)
	{
		++TimerGameTick::counter;
		{
			AutoRestoreBackup system{_current_company, OWNER_DEITY}; REQUIRE(BeginUtilityPowerInterval() == UtilityPowerError::None);
		}
		const auto interval = GetUtilityPowerInterval();
		PublishGeothermalHeatSources(interval, 1); RunGeothermalGenerators(interval, 1);
		REQUIRE(PublishTubeExternalPower(this->external, interval, 1) == TubePowerError::None);
		const auto *binding = GetTubePowerVehicle(capsule.index); REQUIRE(binding != nullptr);
		std::vector<UtilityPowerRequest> requests{{binding->consumer, quantity}};
		if (geothermal) requests.push_back({this->native.supply, quantity});
		{
			AutoRestoreBackup system{_current_company, OWNER_DEITY}; REQUIRE(AllocateUtilityPower(interval, requests) == UtilityPowerError::None);
		}
		if (geothermal) {
			const auto path = GetTubeGeothermalSourcePath(this->native.source); REQUIRE(path.has_value());
			UtilityPathTransferRequest transfer{{this->native.supply, interval}, binding->consumer, this->plant_port, quantity};
			transfer.legs.push_back({binding->local_feed, path->cells});
			REQUIRE(CommitUtilityPathTransfer(PrepareUtilityPathTransfer(transfer)).error == UtilityPowerError::None);
		}
	}
};

/** Every physical capsule keeps its own loaded hull through both actual provider commands; a new binding awaits native allocation. */
TEST_CASE("Five capsule hulls retain loaded identity while switching both providers", "[merged-tube-provider-art]")
{
	const uint16_t model = GENERATE(9, 10, 11, 12, 13);
	const bool native_first = GENERATE(false, true);
	TubeProviderSwitchRig rig;
	AutoRestoreBackup date(TimerGameCalendar::date, TimerGameCalendar::ConvertYMDToDate(TimerGameCalendar::Year{2050}, 0, 1));
	AutoRestoreBackup year(TimerGameCalendar::year, TimerGameCalendar::Year{2050});
	Engine::Get(GetTubeEngine(model))->company_avail.Set(_current_company);
	const auto [result, id, capacity, mail, capacities] = CmdBuildVehicle(DoCommandFlag::Execute,
		rig.inlets[0], GetTubeEngine(model), false, INVALID_CARGO, ClientID::Invalid);
	REQUIRE(result.Succeeded());
	Train &capsule = *Train::Get(id);
	REQUIRE(CargoPacket::CanAllocateItem());
	auto *packet = CargoPacket::Create(StationID::Invalid(), uint16_t{17}, Source{Source::Invalid, SourceType::Industry});
	capsule.cargo.Append(packet);
	REQUIRE((native_first ? BindTubeGeothermalPowerVehicle(id, rig.native.source) : BindTubePowerVehicle(id, rig.external)).has_value());
	const std::array families{GroundSpriteFamily::VehicleModel09, GroundSpriteFamily::VehicleModel10,
		GroundSpriteFamily::VehicleModel11, GroundSpriteFamily::VehicleModel12, GroundSpriteFamily::VehicleModel13};
	// Initial binding and each committed replacement await the sole scheduler.
	// Query every facing without changing packets, provider rights or work clocks.
	for (uint replacement = 0; replacement < 3; ++replacement) {
		// Initial geothermal registration explicitly reports unavailable; both
		// replacement paths start with the original neutral NoConnection wait.
		const auto expected_wait = replacement == 0 && native_first ? TubePowerWait::Unavailable : TubePowerWait::NoConnection;
		REQUIRE(GetTubePowerVehicle(id)->wait == expected_wait);
		const auto owners = std::tuple{ComputeUtilityPowerDigest(), ComputeTubePowerVehicleDigest(), ComputeTubePowerDigest(),
			ComputeTubeGeothermalPowerDigest(), Company::Get(_current_company)->money, TimerGameTick::counter,
			std::array{_random.state[0], _random.state[1], _interactive_random.state[0], _interactive_random.state[1]}};
		for (uint8_t view = 0; view < 8; ++view) {
			CHECK(GetMultiLayerTrainSprite(capsule, static_cast<Direction>(view), 8) == GetGroundSprite(families[model - 9], {7, 1, view}));
		}
		CHECK(capsule.cargo.StoredCount() == 17);
		CHECK(capsule.cargo.Packets()->front() == packet);
		CHECK(owners == std::tuple{ComputeUtilityPowerDigest(), ComputeTubePowerVehicleDigest(), ComputeTubePowerDigest(),
			ComputeTubeGeothermalPowerDigest(), Company::Get(_current_company)->money, TimerGameTick::counter,
			std::array{_random.state[0], _random.state[1], _interactive_random.state[0], _interactive_random.state[1]}});
		if (replacement < 2) REQUIRE(rig.Switch(capsule, replacement == 0 ? !native_first : native_first, DoCommandFlag::Execute).Succeeded());
	}
}

/** Real generation and native depot service leave original consumed history and unused grants; switching transfers only the one retained demand numerator. */
TEST_CASE("Held capsule switches providers without renewing original rights", "[tube-provider-switch-component]")
{
	const bool native_first = GENERATE(false, true); TubeProviderSwitchRig rig; Train *capsule = rig.Buy();
	// Disclosed real packet input stays in the existing carrier owner throughout
	// both switches; no fixture inventory or automatic transfer is substituted.
	REQUIRE(CargoPacket::CanAllocateItem());
	auto *packet = CargoPacket::Create(StationID::Invalid(), uint16_t{17}, Source{Source::Invalid, SourceType::Industry});
	const auto packet_id = packet->index; capsule->cargo.Append(packet);
	const auto bound = native_first ? BindTubeGeothermalPowerVehicle(capsule->index, rig.native.source) : BindTubePowerVehicle(capsule->index, rig.external);
	REQUIRE(bound.has_value());
	// A disclosed valid saved fraction exercises nonzero carry conservation;
	// restoring metadata creates no source, grant, work, stock or invoice.
	auto rows = CaptureTubePowerVehicles(); REQUIRE(rows.size() == 1); rows.front().remainder_mj = 553;
	StageLoadedTubePowerVehicles(rows, GetTubePowerPreparedTick()); REQUIRE(AcceptLoadedTubePowerVehicles());
	rig.AllocateFor(*capsule, native_first, 2400);
	CapsuleDepotService work{*capsule};
	const auto settled = native_first ? CommitUtilityMutation(PrepareUtilityMutation(UtilityMutationKind::Consume,
		{*bound, GetUtilityPowerInterval()}, 540), &work) : ConsumeTubeExternalPower(rig.external, *bound, 540, work);
	REQUIRE(settled.error == UtilityPowerError::None);
	REQUIRE(GetUtilityAllocation(*bound)->granted_energy_j == 1860);
	const auto old = *GetTubePowerVehicle(capsule->index);
	const auto utility = ComputeUtilityPowerDigest(), bindings = ComputeTubePowerVehicleDigest(), metadata = ComputeTubeGeothermalPowerDigest();
	const auto output = *GetUtilityPowerProducer(old.source); const auto external = *GetTubePowerConnection(rig.external);
	const auto native = ComputeGeothermalGeneratorDigest(); const auto cash = Company::Get(_current_company)->money; const auto random = _random;
	REQUIRE(rig.Switch(*capsule, !native_first).Succeeded());
	CHECK(ComputeUtilityPowerDigest() == utility); CHECK(ComputeTubePowerVehicleDigest() == bindings);
	REQUIRE(rig.Switch(*capsule, !native_first, DoCommandFlag::Execute).Succeeded());
	const auto current = *GetTubePowerVehicle(capsule->index); CHECK(current.consumer != old.consumer); CHECK(current.remainder_mj == 553);
	const auto historical = CaptureTubePowerVehicles(); REQUIRE(historical.size() == 2); CHECK_FALSE(historical.front().active); CHECK(historical.front().remainder_mj == 0);
	CHECK(GetUtilityPowerConsumer(old.consumer)->shares.front().consumed_j == 540); CHECK(GetUtilityPowerConsumer(old.consumer)->shares.front().released_j == 1860);
	CHECK(GetUtilityPowerProducer(old.source)->consumed_lifetime_j == output.consumed_lifetime_j);
	CHECK(GetUtilityPowerProducer(old.source)->generated_lifetime_j == output.generated_lifetime_j);
	CHECK(GetTubePowerConnection(rig.external)->invoiced_money == external.invoiced_money);
	CHECK(GetTubePowerConnection(rig.external)->usage_remainder_j == external.usage_remainder_j);
	CHECK(Company::Get(_current_company)->money == cash); CHECK(ComputeGeothermalGeneratorDigest() == native);
	CHECK(ComputeTubeGeothermalPowerDigest() == metadata); CHECK(std::ranges::equal(random.state, _random.state));
	CHECK(capsule->vehstatus.Test(VehState::Stopped)); CHECK(capsule->cur_speed == 0);
	CHECK(capsule->cargo.TotalCount() == 17); CHECK(CargoPacket::GetIfValid(packet_id) == packet);
	const auto grant = GetUtilityAllocation(current.consumer); CHECK((!grant.has_value() || grant->granted_energy_j == 0));
	const auto switched = ComputeUtilityPowerDigest(); CHECK(rig.Switch(*capsule, !native_first).Failed());
	CHECK(rig.Switch(*capsule, !native_first, DoCommandFlag::Execute).Failed()); CHECK(ComputeUtilityPowerDigest() == switched);
	REQUIRE(rig.Switch(*capsule, native_first, DoCommandFlag::Execute).Succeeded());
	CHECK(GetTubePowerVehicle(capsule->index)->remainder_mj == 553);
	CHECK(GetTubePowerVehicle(capsule->index)->consumer != old.consumer); CHECK(Company::Get(_current_company)->money == cash);
	CHECK(capsule->cargo.TotalCount() == 17); CHECK(CargoPacket::GetIfValid(packet_id) == packet);
	CHECK(ValidateTubePowerVehicles(CaptureTubePowerVehicles()) == TubePowerError::None);
	CHECK(ValidateUtilityPowerState(CaptureUtilityPowerState(), GetCurrentMultiLayerProfileSelection()) == UtilityPowerError::None);
}

/** Late loss of the requested new provider cannot release the original held capsule's live unused external grant or its already paid history. */
TEST_CASE("Held provider switch refuses a lost new path unchanged", "[tube-provider-switch-refusal-component]")
{
	TubeProviderSwitchRig rig; Train *capsule = rig.Buy(); REQUIRE(BindTubePowerVehicle(capsule->index, rig.external).has_value());
	rig.AllocateFor(*capsule, false, 2400); const auto consumer = GetTubePowerVehicle(capsule->index)->consumer;
	AutoRestoreBackup actor{_current_company}; AutoRestoreBackup speed{capsule->cur_speed};
	SECTION("missing native generator") {
		REQUIRE(Command<Commands::LandscapeClear>::Unsafe<CommandCallback>(INVALID_STRING_ID, nullptr, false, false, rig.plant_port, std::make_tuple(rig.plant_port)).Succeeded());
	}
	SECTION("missing purchased cable") {
		const auto tile = TileXYL(35, 24, MapLayer::DeepUnderground);
		REQUIRE(Command<Commands::RemoveGeothermalUtilityLink>::Unsafe<CommandCallback>(INVALID_STRING_ID, nullptr, false, false, tile,
			std::make_tuple(tile, UtilityEnergyDomain::Electricity)).Succeeded());
	}
	SECTION("foreign actor") { _current_company = OWNER_NONE; }
	SECTION("retired selected source") { REQUIRE(CloseTubeGeothermalPower(rig.native.source) == TubePowerError::None); }
	SECTION("moving capsule") { capsule->cur_speed = 1; }
	SECTION("insufficient shared receipt room for registration and original release") {
		// A late replacement needs both actual shared operations. Refuse before
		// acquiring a recipient if the old live grant could not then be released.
		auto state = CaptureUtilityPowerState(); state.operation_sequence = UINT64_MAX - 2;
		StageLoadedUtilityPowerState(std::move(state)); REQUIRE(AcceptLoadedUtilityPowerState());
	}
	const auto utility = ComputeUtilityPowerDigest(), bindings = ComputeTubePowerVehicleDigest(), metadata = ComputeTubeGeothermalPowerDigest();
	const auto cash = Company::Get(capsule->owner)->money; const auto random = _random;
	// Both synchronized phases revalidate the new path before changing the old
	// binding. A failed candidate cannot steal its original2400J reservation.
	for (const auto flags : {DoCommandFlags{}, DoCommandFlags{DoCommandFlag::Execute}}) {
		CHECK(rig.Switch(*capsule, true, flags).Failed()); CHECK(ComputeUtilityPowerDigest() == utility);
		CHECK(ComputeTubePowerVehicleDigest() == bindings); CHECK(ComputeTubeGeothermalPowerDigest() == metadata);
	}
	{
		// Foreign actors cannot query another company's usable allocation; inspect
		// the unchanged original grant through its actual owner after refusal.
		AutoRestoreBackup owner{_current_company, capsule->owner};
		const auto allocation = GetUtilityAllocation(consumer); REQUIRE(allocation.has_value()); CHECK(allocation->granted_energy_j == 2400);
	}
	CHECK(Company::Get(capsule->owner)->money == cash);
	CHECK(std::ranges::equal(random.state, _random.state)); CHECK(GetTubePowerVehicle(capsule->index)->consumer == consumer);
}


/** Native source commands add/close original references without another invoice or generation event, and scope closure to the selected inlet. */
TEST_CASE("Native provider commands retain original machinery and payment custody", "[tube-provider-command-source-component]")
{
	TubeProviderSwitchRig rig;
	const auto before = CaptureTubeGeothermalPowerState(); const auto utility = CaptureUtilityPowerState();
	const auto ledger = ComputeUtilityPowerDigest(), metadata = ComputeTubeGeothermalPowerDigest(), machinery = ComputeGeothermalGeneratorDigest();
	const auto cash = Company::Get(_current_company)->money; const auto random = _random;
	const auto quote = Command<Commands::ConnectTubeGeothermalPower>::Do({}, rig.generator, rig.inlets[1], rig.output);
	REQUIRE(quote.Succeeded()); CHECK(quote.GetCost() == 0); CHECK(ComputeUtilityPowerDigest() == ledger); CHECK(ComputeTubeGeothermalPowerDigest() == metadata);
	REQUIRE(Command<Commands::ConnectTubeGeothermalPower>::Do(DoCommandFlag::Execute, rig.generator, rig.inlets[1], rig.output).Succeeded());
	const auto state = CaptureTubeGeothermalPowerState(); REQUIRE(state.sources.size() == before.sources.size() + 1);
	const auto second = state.sources.back(); CHECK(second.id.base() == before.next_source); CHECK(second.supply.base() == utility.next_consumer);
	CHECK(CaptureUtilityPowerState().next_producer == utility.next_producer); CHECK(GetUtilityPowerConsumer(second.supply)->shares.empty());
	const auto connected = ComputeUtilityPowerDigest(), associated = ComputeTubeGeothermalPowerDigest();
	CHECK(Command<Commands::ConnectTubeGeothermalPower>::Do(DoCommandFlag::Execute, rig.generator, rig.inlets[1], rig.output).Failed());
	REQUIRE(Command<Commands::CloseTubeGeothermalPower>::Do({}, second.id).Succeeded()); CHECK(ComputeUtilityPowerDigest() == connected); CHECK(ComputeTubeGeothermalPowerDigest() == associated);
	REQUIRE(Command<Commands::CloseTubeGeothermalPower>::Do(DoCommandFlag::Execute, second.id).Succeeded());
	CHECK_FALSE(GetTubeGeothermalSource(second.id)->active); CHECK_FALSE(GetUtilityPowerConsumer(second.supply)->active);
	CHECK(GetTubeGeothermalSource(rig.native.source)->active); CHECK(GetUtilityPowerConsumer(rig.native.supply)->active);
	CHECK(ComputeGeothermalGeneratorDigest() == machinery); CHECK(Company::Get(_current_company)->money == cash); CHECK(std::ranges::equal(random.state, _random.state));
}

/** A queued source request includes the nonreused original output; a stale output cannot adopt a coincident native machinery ID or create an account. */
TEST_CASE("Native provider command refuses stale output references unchanged", "[tube-provider-command-stale-component]")
{
	TubeProviderSwitchRig rig; const auto ledger = ComputeUtilityPowerDigest(), metadata = ComputeTubeGeothermalPowerDigest();
	const auto cash = Company::Get(_current_company)->money; const auto random = _random;
	// Both synchronized phases must validate the original coupled output before
	// allocating source/root identities, independently of matching IndustryIDs.
	for (const auto flags : {DoCommandFlags{}, DoCommandFlags{DoCommandFlag::Execute}}) {
		CHECK(Command<Commands::ConnectTubeGeothermalPower>::Do(flags, rig.generator, rig.inlets[1], UtilityProducerID{UINT32_MAX}).Failed());
		CHECK(Command<Commands::ConnectTubeGeothermalPower>::Do(flags, rig.generator, rig.inlets[1], rig.external).Failed());
		CHECK(ComputeUtilityPowerDigest() == ledger); CHECK(ComputeTubeGeothermalPowerDigest() == metadata);
	}
	CHECK(Company::Get(_current_company)->money == cash); CHECK(std::ranges::equal(random.state, _random.state));
}

/** Frozen command identities and typed source arguments survive native bounded network buffers without aliasing utility producers and physical inlet incarnations. */
TEST_CASE("Native provider commands retain frozen typed network arguments", "[tube-provider-command-wire-component]")
{
	static_assert(to_underlying(Commands::ConnectTubeGeothermalPower) == 220);
	static_assert(to_underlying(Commands::CloseTubeGeothermalPower) == 221);
	static_assert(to_underlying(Commands::SetTubeVehicleGeothermalPower) == 222);
	static_assert(to_underlying(Commands::End) == 223);
	static_assert(!std::is_same_v<TubeGeothermalSourceID, UtilityProducerID>);
	using Connect = CommandTraits<Commands::ConnectTubeGeothermalPower>::Args;
	const Connect connect{IndustryID{3}, TileIndex{400}, UtilityProducerID{7}};
	CHECK(EndianBufferReader::ToValue<Connect>(EndianBufferWriter<CommandDataBuffer>::FromValue(connect)) == connect);
	using Close = CommandTraits<Commands::CloseTubeGeothermalPower>::Args;
	const Close close{TubeGeothermalSourceID{6}};
	CHECK(EndianBufferReader::ToValue<Close>(EndianBufferWriter<CommandDataBuffer>::FromValue(close)) == close);
	using Bind = CommandTraits<Commands::SetTubeVehicleGeothermalPower>::Args;
	const Bind bind{VehicleID{9}, TubeGeothermalSourceID{6}, true};
	CHECK(EndianBufferReader::ToValue<Bind>(EndianBufferWriter<CommandDataBuffer>::FromValue(bind)) == bind);
}


/** Distinct original inlets can share one native converter; switching selects the exact new root/feed and never treats producer equality as repeated choice. */
TEST_CASE("Held native capsule switches distinct original inlets sharing one output", "[tube-provider-switch-inlet-component]")
{
	TubeProviderSwitchRig rig; Train *capsule = rig.Buy(); REQUIRE(BindTubeGeothermalPowerVehicle(capsule->index, rig.native.source).has_value());
	rig.AllocateFor(*capsule, true, 2400); const auto old = *GetTubePowerVehicle(capsule->index);
	rig.native = rig.Connect(1); REQUIRE(GetTubeGeothermalSource(rig.native.source)->producer == old.source);
	const auto utility = ComputeUtilityPowerDigest(); const auto cash = Company::Get(_current_company)->money;
	REQUIRE(rig.Switch(*capsule, true).Succeeded()); CHECK(ComputeUtilityPowerDigest() == utility);
	REQUIRE(rig.Switch(*capsule, true, DoCommandFlag::Execute).Succeeded());
	const auto current = *GetTubePowerVehicle(capsule->index); CHECK(current.source == old.source);
	CHECK(current.geothermal_source != old.geothermal_source); CHECK(current.geothermal_source == rig.native.source);
	CHECK(current.consumer != old.consumer); CHECK(current.local_feed != old.local_feed);
	CHECK(GetUtilityPowerConsumer(old.consumer)->shares.front().released_j == 2400);
	CHECK(GetUtilityPowerLink(old.local_feed)->flow_j == 2400); CHECK_FALSE(GetUtilityPowerLink(old.local_feed)->active);
	CHECK(GetTubeGeothermalSource(old.geothermal_source)->active); CHECK(GetTubeGeothermalSource(current.geothermal_source)->active);
	CHECK(Company::Get(_current_company)->money == cash);
	CHECK(ValidateTubePowerVehicles(CaptureTubePowerVehicles()) == TubePowerError::None);
}
