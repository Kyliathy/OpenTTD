/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file tbm_reuse.cpp Reusing cleared ground after support (Group 15, shared removal/replacement and existing road and
 * rail cases): support is never replaced in place, removed support leaves open ground for ordinary roads or rails,
 * and converted support is ordinary track. Group15B drives actual paid clearing/withdrawal before native tube, mine and road construction.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs4-tunnel-boring-machines.md, Group 15
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
 */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "tbm_fixture.h"
#include "../tbm.h"
#include "../tbm_cmd.h"
#include "../rail_cmd.h"
#include "../road.h"
#include "../road_cmd.h"
#include "../road_map.h"
#include "../command_func.h"
#include "underground_depot_fixture.h"
#include "layer_window_fixture.h"
#include "../transport_tube.h"
#include "../station_cmd.h"
#include "tube_tbm_clearance_fixture.h"
#include "tube_tbm_useful_fixture.h"
#include "tube_tbm_queue_fixture.h"
#include "ground_network_fixture.h"
#include "ground_network_commands_fixture.h"
#include "ground_network_active_fixture.h"
#include "ground_network_pause_fixture.h"
#include "ground_network_clock_fixture.h"
#include "ground_network_power_fixture.h"
#include "ground_network_recovery_fixture.h"
#include "ground_network_thermal_fixture.h"
#include "normal_ground_newgame_fixture.h"
#include "normal_ground_readiness_fixture.h"
#include "normal_ground_freight_fixture.h"
#include "normal_ground_yard_fixture.h"
#include "normal_ground_machinery_fixture.h"
#include "normal_ground_boring_fixture.h"
#include "normal_ground_access_fixture.h"
#include "normal_ground_deep_fixture.h"
#include "normal_ground_irregular_fixture.h"
#include "normal_ground_layout_fixture.h"

#include "../table/strings.h"

#include "../safeguards.h"

/** Actual independent application peers capture complete native owners after real agreed commands at fixed network frames. @see r2mlvs15-savegame-determinism-and-lifecycle.md17B. */
TEST_CASE("Real paid ground peers retain complete state at agreed network frames", "[.ground-network-peer]")
{
	RunGroundNetworkPeer();
}

/** A genuinely joined company client sends rename/stop/resume through normal networking;both actual peers retain the same original packets and physical state. @see r2mlvs15-savegame-determinism-and-lifecycle.md17C. */
TEST_CASE("Real company client commands control the original paid capsule", "[.ground-network-company]")
{
	RunGroundNetworkPeer(ObserveGroundNetworkCommands);
}

/** A fresh client reconstructs the same earned active excavation and original waiting wagon owners,then follows actual native next opportunities. @see r2mlvs15-savegame-determinism-and-lifecycle.md18B. */
TEST_CASE("Real joining ground peers preserve original paid active work", "[.ground-network-active]")
{
	RunGroundNetworkActivePeer();
}

/** Original paid tube/thermal jobs transfer into an independent native client and retain complete initial/next/agreed-frame owners. @see r2mlvs15-savegame-determinism-and-lifecycle.md18C. */
TEST_CASE("Real joining peers preserve delivered paid tube and thermal work", "[.ground-network-delivered]")
{
	RunGroundNetworkPeer({}, true);
}

/** Actual reconstruction and bounded later native work retain six full peer checkpoints. @see r2mlvs16-integrated-acceptance-and-balance.md21EC. */
TEST_CASE("Real joining performance peers retain reconstructed and later native owners", "[.ground-network-performance]")
{
	RunGroundNetworkPeer({}, true, true, {8, 16, 24, 32}, 16);
}

/** Real native server Pause commands freeze both peers' complete gameplay while network processing continues,then native console unpause resumes matching paid work. @see r2mlvs15-savegame-determinism-and-lifecycle.md19B. */
TEST_CASE("Real paid ground peers freeze and resume through native pause commands", "[.ground-network-pause]")
{
	RunGroundNetworkPausePeer();
}

/** Public next-game settings activate before fresh paid owners exist,then an original native machine earns unfinished work in that clock mode. @see r2mlvs15-savegame-determinism-and-lifecycle.md19C. */
TEST_CASE("Legal native starting time modes retain actual paid ground work", "[.ground-network-clock-source]")
{
	ProduceGroundNetworkClockWorld();
}

/** Real native peers retain legal starting clocks and original unfinished paid work. @see r2mlvs15-savegame-determinism-and-lifecycle.md Group19C.2. */
TEST_CASE("Native peers continue original paid work under legal starting clocks", "[.ground-network-clock-peer]")
{
	RunGroundNetworkActivePeer(true);
}

/** Actual agreed source retirement holds original loaded hulls and settled history without losing packets. @see r2mlvs15-savegame-determinism-and-lifecycle.md21B. */
TEST_CASE("Native paid tube peers retain loaded custody after actual power loss", "[.ground-network-power-loss]")
{
	RunGroundNetworkPowerLossPeer();
}

/** Actual paid replacement/reconnection recovers original cargo through native conventional Surface freight. @see r2mlvs15-savegame-determinism-and-lifecycle.md21C. */
TEST_CASE("Native paid tube peers recover original cargo through fresh paid service", "[.ground-network-power-recovery]")
{
	RunGroundNetworkPowerRecoveryPeer();
}

/** Actual original paid riser closure/recommissioning preserves history and restores useful Surface service. @see r2mlvs15-savegame-determinism-and-lifecycle.md21D. */
TEST_CASE("Native paid thermal peers recover original riser service", "[.ground-network-thermal-riser]")
{
	RunGroundNetworkThermalRiserPeer();
}

/** Normal GUI-capable application generation owns profile,terrain,company and initial loan before any paid predecessor exists. @see r2mlvs16-integrated-acceptance-and-balance.md6B. */
TEST_CASE("Ordinary new game creates genuine ground starting owners", "[.normal-ground-newgame]")
{
	RunNormalGroundNewgame();
}

/** Actual original normal-world quotes/credit precede any paid Surface logistics mutation. @see r2mlvs16-integrated-acceptance-and-balance.md6C. */
TEST_CASE("Original normal game exposes native ground readiness", "[.normal-ground-readiness]")
{
	RunNormalGroundReadiness();
}

/** Native paid conventional freight provides real normal-start financing. @see r2mlvs16-integrated-acceptance-and-balance.md6CB. */
TEST_CASE("Original normal game builds ordinary paid freight", "[.normal-ground-freight]")
{
	RunNormalGroundFreight();
}

/** Real production,normal movement and CargoPayment earn the original company's capital. @see r2mlvs16-integrated-acceptance-and-balance.md6CB. */
TEST_CASE("Original normal freight earns native income", "[.normal-ground-earnings]")
{
	RunNormalGroundFreightEarnings();
}

/** Native earned capital pays for actual Surface disposal/logistics without opening rock. @see r2mlvs16-integrated-acceptance-and-balance.md6CC. */
TEST_CASE("Original financed normal game builds paid Surface yard", "[.normal-ground-yard]")
{
	RunNormalGroundYard();
}

/** Native public purchase/commissioning binds actual machinery to the normally paid yard. @see r2mlvs16-integrated-acceptance-and-balance.md6CDB. */
TEST_CASE("Original normal yard acquires actual Standard machinery", "[.normal-ground-machinery]")
{
	RunNormalGroundMachinery();
}

/** Full native frames cut and settle actual normally financed Standard work. @see r2mlvs16-integrated-acceptance-and-balance.md6CDCB. */
TEST_CASE("Original normal Standard project earns floor through paid work", "[.normal-ground-boring]")
{
	RunNormalGroundBoring();
}

/** Native public release/conversion makes the actual earned floor accessible. @see r2mlvs16-integrated-acceptance-and-balance.md6CDCC. */
TEST_CASE("Original settled normal work becomes ordinary Underground access", "[.normal-ground-access]")
{
	RunNormalGroundAccess();
}

/** Actual Deep readiness consumes only the original paid Standard access. @see r2mlvs16-integrated-acceptance-and-balance.md6CDD. */
TEST_CASE("Original normal game acquires Deep machinery on earned floor", "[.normal-ground-deep]")
{
	RunNormalGroundDeep();
}

/** Native full frames work the actual Deep readiness world without resetting prior Standard ownership. @see r2mlvs16-integrated-acceptance-and-balance.md6DB. */
TEST_CASE("Original normal Deep project cuts and settles through earned access", "[.normal-ground-deep-work]")
{
	RunNormalGroundBoring(1, "normal-ground-deep-work");
}

/** Native canonical irregular approach and real retained machinery keep their normal-world predecessors. @see r2mlvs16-integrated-acceptance-and-balance.md6DCB. */
TEST_CASE("Original settled normal world prepares irregular machinery reuse", "[.normal-ground-irregular]")
{
	RunNormalGroundIrregular();
}

/** Native full frames execute the actual retained-machine irregular job and its real haulage. @see r2mlvs16-integrated-acceptance-and-balance.md6DCC. */
TEST_CASE("Original normal irregular project earns its actual bent floor", "[.normal-ground-irregular-work]")
{
	RunNormalGroundBoring(0, "normal-ground-irregular-work");
}

/** Support stays rail until it is removed or converted; afterwards the open floor takes ordinary roads and rails. */
TEST_CASE("Actual normal irregular floor carries a paid native modal layout", "[.normal-ground-layout]")
{
	RunNormalGroundModalLayout();
}

TEST_CASE("Cleared support ground becomes ordinary road or rail after release", "[tbm-reuse]")
{
	TBMWorkRig rig;
	rig.BoreOneCut();
	ResetRoadTypes();
	const TileIndex cell = rig.Job().route[0].cutter_position;
	const Track laid = *GetTrackBits(cell).begin();
	const RoadBits across = laid == Track::X ? ROAD_Y : ROAD_X; // A road across the support would make a level crossing.

	/**
	 * Try a road across the cell.
	 * @return The command result.
	 */
	const auto build_road = [&]() { return CmdBuildRoad(DoCommandFlag::Execute, cell, across, ROADTYPE_ROAD, {}, TownID::Invalid()); };
	// In use, the support is never turned into a crossing in place.
	CHECK(build_road().GetErrorMessage() == STR_ERROR_TBM_SUPPORT_CONVERT_FIRST);
	CHECK(IsPlainRailTile(cell));

	SECTION("released but unconverted support still refuses replacement in place") {
		rig.Reload(TBMPhase::Completed);
		CHECK(build_road().GetErrorMessage() == STR_ERROR_TBM_SUPPORT_CONVERT_FIRST);
		CHECK(GetTBMSupportTile(cell) != nullptr);
	}
	SECTION("removed support leaves open ground that takes an ordinary road") {
		rig.Reload(TBMPhase::Completed);
		REQUIRE(Command<Commands::RemoveRail>::Do(DoCommandFlag::Execute, cell, laid).Succeeded());
		CHECK(IsTileType(cell, TileType::Clear));
		CHECK(GetGeologyWorldState().terrain.Get(cell) == ExcavationState::Excavated);
		CHECK(CmdBuildRoad(DoCommandFlag::Execute, cell, ROAD_X, ROADTYPE_ROAD, {}, TownID::Invalid()).Succeeded());
		CHECK(IsTileType(cell, TileType::Road));
	}
	SECTION("removed support leaves open ground that takes ordinary rail") {
		rig.Reload(TBMPhase::Completed);
		REQUIRE(Command<Commands::RemoveRail>::Do(DoCommandFlag::Execute, cell, laid).Succeeded());
		CHECK(Command<Commands::BuildRail>::Do(DoCommandFlag::Execute, cell, GetRailType(rig.proof.destination), laid, false).Succeeded());
		CHECK(GetTBMSupportTile(cell) == nullptr); // Ordinary track carries no support restriction.
	}
	SECTION("converted support is ordinary track that may become a crossing") {
		rig.Reload(TBMPhase::Completed);
		REQUIRE(Command<Commands::ConvertTBMSupport>::Do(DoCommandFlag::Execute, cell).Succeeded());
		CHECK(build_road().Succeeded());
		CHECK(IsLevelCrossingTile(cell));
	}
}

/** An actual paid original machine earns the non-rectangular curved tube floor before complete safe withdrawal and native final construction. */
TEST_CASE("Paid TBM extensions earn an irregular curved tube terminal route", "[tube-tbm-clearing]")
{
	MockEnvironment::Instance();
	LoadGeneratedWindowTestLanguage();
	TubeTBMClearanceRig rig;
	rig.Clear();
	rig.BuildTubes();
	rig.SaveReady();
}

/** Actual ordinary loading retains paid excavation while native commands and work earn original mine cargo in a real capsule. */
TEST_CASE("Original paid TBM floor earns useful capsule cargo through native machinery", "[.tube-tbm-useful-prepare]")
{
	/** Build only actual subsequent useful participants in the original loaded paid world;no excavation fixture is instantiated here. */
	RunTubeTBMLoadedObserver([] {
		TubeTBMUsefulWorld world;
		const auto original = TubeTBMValues("OTTD_TUBE_TBM_OWNERS");
		REQUIRE(Company::Get(_current_company)->money.base() == static_cast<int64_t>(original[9]));
		AutoRestoreBackup distant{_settings_game.station.distant_join_stations, true};
		world.Prepare();
		world.Do<Commands::StartStopVehicle>(world.capsule->index, true);
		world.Do<Commands::StartStopVehicle>(world.freight->index, true);
		uint ticks = 0;
		// Native landscape construction,finite production,station dispatch and
		// ordinary loading must all complete before any cargo-ready claim.
		while (ticks < 12000) {
			world.Tick();
			++ticks;
			if (world.capsule->cargo.StoredCount() > 0) break;
		}
		INFO("native mine cargo wait ticks=" << ticks << " capsule=" << DescribeTrain(world.capsule));
		REQUIRE(ticks < 12000);
		REQUIRE(world.capsule->cargo.StoredCount() > 0);
		REQUIRE(IsLayerIndustryComplete(*world.mine));
		REQUIRE(GetLayerMineState(world.mine->index)->extracted > 0);
		world.CheckSpoil();
		const auto *mine = GetLayerMineState(world.mine->index);
		const auto *deposit = GetGeologyWorldState().deposits.Get(mine->deposit);
		// Paid cutting already debited ore intersected by the actual excavation.
		// Mining must conserve the remaining native seam rather than recreate
		// that earlier lost material as a second cargo output.
		CHECK(world.opening_reserve == deposit->remaining_reserve + mine->extracted);
		// Actual mine provenance and first native pickup travel in every loaded
		// packet;matching tonnage alone cannot establish useful source custody.
		for (const auto *packet : *world.capsule->cargo.Packets()) {
			CHECK(packet->GetSource() == Source{world.mine->index, SourceType::Industry});
			CHECK(packet->GetFirstStation() == world.origin->index);
			CHECK(packet->GetCompanySupplyOrder() == 0);
		}
		world.Save("loaded-ready");
		fmt::print("TUBE_TBM_USEFUL_PREPARED ticks={} capsule={} freight={} wagon={} mine={} factory={} source={} loaded={} extracted={} remaining={} cash={} opening={} excavation_loss={}\n",
			ticks, world.capsule->index.base(), world.freight->index.base(), world.wagon->index.base(), world.mine->index.base(), world.factory->index.base(),
			world.power.base(), world.capsule->cargo.StoredCount(), mine->extracted, deposit->remaining_reserve, Company::Get(_current_company)->money.base(),
			world.opening_reserve, deposit->initial_reserve - world.opening_reserve);
	});
}

/** Ordinary loading must retain the canonical public native catalogue without fixture availability grants or any new construction. */
TEST_CASE("Original paid tube world retains native introduced and future engine dates", "[.tube-tbm-catalogue]")
{
	/** Inspect the real loaded original catalogue only;no builder,production or controller step is invoked. */
	RunTubeTBMLoadedObserver([] {
		TubeTBMUsefulWorld world;
		const auto before = std::tuple{ComputeGeologyStateDigest(GetGeologyWorldState()), Company::Get(_current_company)->money,
			_random.state[0], _random.state[1], TimerGameTick::counter, Vehicle::GetNumItems()};
		// Availability follows each actual original launch date;the saved1980
		// world may offer Mole,Crate and Cistern while keeping Shield/Forge locked.
		for (uint16_t model = 9; model <= 13; ++model) {
			const auto engine = GetTubeEngine(model);
			const auto capability = GetTubeVehicleCapability(engine);
			REQUIRE(capability.has_value());
			const bool introduced = TimerGameCalendar::date >= TimerGameCalendar::ConvertYMDToDate(capability->catalogue->introduction_year, 0, 1);
			CHECK(IsEngineBuildable(engine, VehicleType::Train, _current_company) == introduced);
		}
		CHECK(IsEngineBuildable(GetUndergroundEngine(4), VehicleType::Train, _current_company));
		CHECK(IsEngineBuildable(GetUndergroundEngine(5), VehicleType::Train, _current_company));
		CHECK((std::tuple{ComputeGeologyStateDigest(GetGeologyWorldState()), Company::Get(_current_company)->money,
			_random.state[0], _random.state[1], TimerGameTick::counter, Vehicle::GetNumItems()} == before));
		fmt::print("TUBE_TBM_CATALOGUE native_public_dates=1 future_locked=1 unchanged_world=1\n");
	});
}

/** Original earned cargo must cross the paid curved tube route and native modal transfer before useful conventional Surface settlement and empty home returns. */
TEST_CASE("Original paid curved tube cargo earns useful Surface freight and empty returns", "[.tube-tbm-useful-delivery]")
{
	/** Continue only the ordinary-loaded original cargo-ready participants;no excavation,funding,source production grant or arrival substitution runs here. */
	RunTubeTBMLoadedObserver([] {
		TubeTBMUsefulWorld world;
		world.ResolveUseful();
		const auto *company = Company::Get(_current_company);
		const auto income = company->cur_economy.income;
		const auto deliveries = company->cur_economy.delivered_cargo[world.cargo];
		const auto initial_loaded = world.capsule->cargo.StoredCount();
		const auto initial_energy = GetUtilityPowerProducer(world.power)->consumed_lifetime_j;
		const auto initial_invoice = GetTubePowerConnection(world.power)->invoiced_money;
		bool freight_loaded = false, crossed = false, capsule_returned = false, freight_returned = false, freight_halt_order = false;
		// A normal stop-at-home depot order closes this actual one-trip capsule
		// observation after its native far service reversal. It cannot skip any
		// berth,transfer,return or loading opportunity.
		world.Do<Commands::ModifyOrder>(world.capsule->index, VehicleOrderID{3}, MOF_DEPOT_ACTION, static_cast<uint16_t>(OrderDepotAction::Stop));
		uint ticks = 0;
		// Require actual useful native intake/payment and whole original empty
		// home returns. Source production continues under its own finite owner;
		// it is never truncated to make a final conservation equation pass.
		while (ticks < 16000) {
			world.Tick();
			++ticks;
			world.CheckConservation();
			if (world.wagon->cargo.StoredCount() > 0) freight_loaded = true;
			// Only a genuinely loaded conventional trip receives the ordinary
			// stop-at-home action;an earlier empty scouting cycle must keep running
			// until it physically collects this actual transfer.
			if (!freight_halt_order && world.wagon->cargo.StoredCount() > 0) {
				world.Do<Commands::ModifyOrder>(world.freight->index, VehicleOrderID{3}, MOF_DEPOT_ACTION, static_cast<uint16_t>(OrderDepotAction::Stop));
				freight_halt_order = true;
			}
			// The loaded native wagon must actually reach Surface before useful
			// acceptance. A same-layer tube or shared station catchment is no
			// substitute for this real conventional portal crossing.
			if (world.wagon->cargo.StoredCount() > 0 && GetTileLayer(world.wagon->tile) == MapLayer::Surface) crossed = true;
			// Count only whole empty returned chains at their real home service
			// depots;initial purchase placement cannot close either loaded leg.
			if (world.capsule->IsChainInDepot() && world.capsule->tile == world.tube_depots[0] && world.capsule->cargo.StoredCount() == 0) capsule_returned = true;
			if (freight_loaded && world.freight->IsChainInDepot() && world.freight->tile == world.freight_depots[0] && world.wagon->cargo.StoredCount() == 0 && world.freight->cargo_payment == nullptr) freight_returned = true;
			// All independent transport,settlement and return obligations must
			// complete together;positive station stock alone is not useful freight.
			if (crossed && capsule_returned && freight_returned && world.factory->GetCargoAccepted(world.cargo)->waiting >= initial_loaded && company->cur_economy.income > income) break;
		}
		INFO("useful actual ticks=" << ticks << " capsule=" << DescribeTrain(world.capsule) << " freight=" << DescribeTrain(world.freight));
		INFO("loaded=" << freight_loaded << " crossed=" << crossed << " returned=" << capsule_returned << ',' << freight_returned
			<< " received=" << world.factory->GetCargoAccepted(world.cargo)->waiting << " orders=" << world.freight->cur_real_order_index);
		REQUIRE(ticks < 16000);
		CHECK(freight_loaded);
		CHECK(crossed);
		CHECK(capsule_returned);
		CHECK(freight_returned);
		CHECK(company->cur_economy.delivered_cargo[world.cargo] >= deliveries + initial_loaded);
		CHECK(GetUtilityPowerProducer(world.power)->consumed_lifetime_j > initial_energy);
		CHECK(GetTubePowerConnection(world.power)->invoiced_money > initial_invoice);
		world.CheckConservation();
		world.Save("useful-final");
		const auto *mine = GetLayerMineState(world.mine->index);
		fmt::print("TUBE_TBM_USEFUL_DELIVERED ticks={} useful={} income={} extracted={} remaining={} energy={} invoice={} capsule_return=1 freight_return=1 crossed_surface=1\n",
			ticks, world.factory->GetCargoAccepted(world.cargo)->waiting, (company->cur_economy.income - income).base(), mine->extracted,
			GetGeologyWorldState().deposits.Get(mine->deposit)->remaining_reserve, GetUtilityPowerProducer(world.power)->consumed_lifetime_j,
			GetTubePowerConnection(world.power)->invoiced_money);
		// This next complete native opportunity is the untouched continuation
		// baseline for later ordinary-reader acceptance;the writer adds no
		// independent packet,position or reservation reconstruction.
		world.Tick();
		world.CheckConservation();
		world.Save("useful-final-next");
	});
}

/** Two actually loaded capsules queue at a native merge on the original paid floor,survive real paid-source loss and resume useful conventional Surface freight. */
TEST_CASE("Original paid tube route retains actual queue and paid grid recovery", "[.tube-tbm-queue-recovery]")
{
	/** Continue only the accepted original useful-final world;additional native purchases must reuse its earned floor,finite cargo and completed spoil owners. */
	RunTubeTBMLoadedObserver([] {
		TubeTBMQueueWorld world;
		world.PrepareQueue();
		uint queued_ticks = 0;
		// Actual native loading,departures and path signals must create the
		// shared reservation/loaded wait;no test calls TryPathReserve or writes
		// a vehicle's track,position,orders,packets or saved wait flags.
		while (queued_ticks < 16000) {
			world.Tick();
			++queued_ticks;
			world.CheckConservation(world.peer);
			if (world.ObserveQueue()) break;
		}
		INFO("actual merge wait ticks=" << queued_ticks << " first=" << DescribeTrain(world.capsule) << " peer=" << DescribeTrain(world.peer));
		REQUIRE(queued_ticks < 16000);
		world.Save("queued");
		world.Tick();
		world.CheckConservation(world.peer);
		world.Save("queued-next");
		world.LosePower();
		world.Save("starved");
		world.Tick();
		world.CheckConservation(world.peer);
		world.Save("starved-next");
		const auto intake = world.factory->GetCargoAccepted(world.cargo)->waiting;
		const auto previous_energy = GetUtilityPowerProducer(world.previous_source)->consumed_lifetime_j;
		world.RestorePower();
		world.Save("restored");
		world.Tick();
		world.CheckConservation(world.peer);
		world.Save("restored-next");
		uint recovery_ticks = 0;
		// Both original loaded shipments must complete native transfer and
		// conventional Surface acceptance after paid service restoration.
		// Whole capsule home returns also prove the queue clears physically.
		while (recovery_ticks < 24000) {
			world.Tick();
			++recovery_ticks;
			world.CheckConservation(world.peer);
			const bool primary_home = world.capsule->IsChainInDepot() && world.capsule->tile == world.tube_depots[0] && world.capsule->cargo.StoredCount() == 0;
			const bool peer_home = world.peer->IsChainInDepot() && world.peer->tile == world.branch && world.peer->cargo.StoredCount() == 0;
			// Independent native packet intake and both whole returned capsules
			// must agree;clearing a reservation alone is not recovered freight.
			if (primary_home && peer_home && world.factory->GetCargoAccepted(world.cargo)->waiting >= intake + world.queued_units) break;
		}
		INFO("actual recovery ticks=" << recovery_ticks << " first=" << DescribeTrain(world.capsule) << " peer=" << DescribeTrain(world.peer)
			<< " freight=" << DescribeTrain(world.freight) << " intake=" << world.factory->GetCargoAccepted(world.cargo)->waiting << " required=" << intake + world.queued_units);
		REQUIRE(recovery_ticks < 24000);
		CHECK(GetUtilityPowerProducer(world.previous_source)->consumed_lifetime_j == previous_energy);
		CHECK(GetUtilityPowerProducer(world.power)->consumed_lifetime_j > 0);
		world.Save("recovered");
		fmt::print("TUBE_TBM_QUEUE_RECOVERY queue_ticks={} recovery_ticks={} peer={} winner={} waiting={} queued_units={} useful={} old_source={} fresh_source={} old_energy={} fresh_energy={}\n",
			queued_ticks, recovery_ticks, world.peer->index.base(), world.winner->index.base(), world.waiting->index.base(), world.queued_units,
			world.factory->GetCargoAccepted(world.cargo)->waiting, world.previous_source.base(), world.power.base(), previous_energy,
			GetUtilityPowerProducer(world.power)->consumed_lifetime_j);
		world.Tick();
		world.CheckConservation(world.peer);
		world.Save("recovered-next");
	});
}

/** Ordinary application loading and one complete native opportunity must retain original actual queue/power/cargo/PBS owners without replaying purchases or recovery commands. */
TEST_CASE("Original paid tube queue files preserve full native owners and continuation", "[.tube-tbm-queue-file]")
{
	/** Inspect or step only the exact loaded original;all phase/input expectations come from the independent immutable file report. */
	RunTubeTBMLoadedObserver([] {
		TubeTBMQueueWorld world;
		world.ResolveUseful(false);
		const auto request = TubeTBMValues("OTTD_TUBE_TBM_FILE");
		REQUIRE(request.size() == 2);
		world.peer = Train::Get(VehicleID{static_cast<uint32_t>(request[0])});
		REQUIRE(request[1] <= 1);
		const char *label = std::getenv("OTTD_TUBE_TBM_FILE_LABEL");
		REQUIRE(label != nullptr);
		ValidateTestArtifactIdentifier(label);
		world.CheckConservation(world.peer);
		if (request[1] == 1) world.Tick();
		world.CheckConservation(world.peer);
		world.Save(label);
		// Repeated pure writing from the same loaded world may not acquire a
		// grant,age a packet,advance a train or rebuild saved reservations.
		if (request[1] == 0) world.Save(fmt::format("{}-repeat", label));
		fmt::print("TUBE_TBM_FILE label={} step={} full_native_owners=1\n", label, request[1]);
	});
}

/**
 * Execute one ordinary paid construction/control action after its native admission.
 * @tparam Tcmd Existing synchronized command identity; no alternate construction owner is introduced.
 * @tparam Targs Native typed argument types.
 * @param args Actual command arguments on the caller's earned floor.
 * @return Native result after success and the ordinary signal-update batch.
 */
template <Commands Tcmd, typename... Targs>
static auto NonRailReuseCommand(Targs... args)
{
	auto result = Command<Tcmd>::Do(DoCommandFlag::Execute, args...);
	REQUIRE(ExtractCommandCost(result).Succeeded());
	UpdateSignalsInBuffer();
	return result;
}

/** Real paid cutting and final disposal keep an active return route intact before safely withdrawn support becomes native tube facilities. */
TEST_CASE("Paid native excavation becomes tube infrastructure only after actual withdrawal", "[tbm-nonrail-reuse]")
{
	MockEnvironment::Instance();
	LoadGeneratedWindowTestLanguage();
	NativeDepotEconomy economy;
	UndergroundWorkflowWorld world(MapLayer::Underground, true, true);
	LayerSaveContent content(world.ActivatedContent(), true);
	world.simulate_company_limits = true;
	AutoRestoreBackup<bool> turns(_settings_game.pf.forbid_90_deg, false);
	const auto tube = InitialiseTubeRailType(world.ActivatedContent());
	REQUIRE(tube != INVALID_RAILTYPE);
	Company::Get(_current_company)->avail_railtypes.Set(tube);
	const Money initial = Company::Get(_current_company)->money;
	bool protected_retreat = false;
	/**
	 * Probe ordinary commands once real cutting has earned a vacant return-support cell.
	 * @param observed Same natively stepped world; actions use only public command owners and may not change it on refusal.
	 */
	const auto protect_retreat = [&](const UndergroundWorkflowWorld &observed) {
		if (protected_retreat) return;
		const auto projects = CaptureTBMProjects();
		if (projects.empty()) return;
		const auto &project = projects.front();
		if (IsTBMProjectFinished(project.phase)) return;
		if (GetTileLayer(Train::Get(project.cutter)->tile) != observed.layer) return;
		// A vacant laid cell behind actual work proves protection of the future
		// retreat, rather than merely the ordinary refusal to demolish a vehicle.
		for (const auto &support : CaptureTBMSupport()) {
			if (support.project != project.id) continue;
			if (EnsureNoVehicleOnGround(support.tile).Failed()) continue;
			REQUIRE(IsTBMSupportInUse(support.tile));
			const auto track = GetTrackBits(support.tile);
			const auto before = std::tuple{ComputeGeologyStateDigest(GetGeologyWorldState()), ComputeTBMProjectDigest(),
				ComputeTBMExchangeDigest(), Company::Get(_current_company)->money, _random.state[0], _random.state[1]};
			CHECK(Command<Commands::ConvertRail>::Do({}, support.tile, support.tile, tube, false).Failed());
			CHECK(Command<Commands::ConvertRail>::Do(DoCommandFlag::Execute, support.tile, support.tile, tube, false).Failed());
			CHECK(Command<Commands::RemoveRail>::Do(DoCommandFlag::Execute, support.tile, *track.begin()).Failed());
			CHECK(GetTrackBits(support.tile) == track);
			CHECK(GetRailType(support.tile) == RAILTYPE_RAIL);
			CHECK((std::tuple{ComputeGeologyStateDigest(GetGeologyWorldState()), ComputeTBMProjectDigest(),
				ComputeTBMExchangeDigest(), Company::Get(_current_company)->money, _random.state[0], _random.state[1]} == before));
			protected_retreat = true;
			return;
		}
	};
	const auto id = RunUndergroundExcavationWorkflow(world, 0, {}, protect_retreat, true);
	REQUIRE(protected_retreat);
	const auto project = *GetTBMProject(id);
	const auto job = *GetExcavationJob(project.job);
	REQUIRE(project.phase == TBMPhase::Completed);
	REQUIRE(job.state == ExcavationJobState::Completed);
	REQUIRE(job.completed_work_cost > 0);
	REQUIRE(job.completed_earth_t > 0);
	REQUIRE(job.completed_rock_t > 0);
	ChargeTBMProjectCosts(id);
	world.hauler->OnNewEconomyDay();
	REQUIRE(initial - Company::Get(_current_company)->money >= job.completed_work_cost);
	NonRailReuseCommand<Commands::ReleaseTBMHauler>(world.hauler->index);
	NonRailReuseCommand<Commands::StartStopVehicle>(world.hauler->index, true);
	NonRailReuseCommand<Commands::StartStopVehicle>(project.cutter, true);
	NonRailReuseCommand<Commands::ControlTBMProject>(id, TBMProjectAction::Retire);
	REQUIRE(GetTBMProject(id) == nullptr);
	const auto geology = ComputeGeologyStateDigest(GetGeologyWorldState());
	/** @return Actual retained physical/order/cargo observations; construction may not relocate either train or replace its empty-return history. */
	const auto train_state = [&]() {
		std::vector<std::tuple<VehicleID, TileIndex, int, int, int, TrackBits, uint, uint>> result;
		for (const auto *front : {Train::Get(project.cutter), world.hauler}) { // Preserve every original cutter and hauler part, not just the visible front's position.
			for (const Train *part = front; part != nullptr; part = part->Next()) {
				result.emplace_back(part->index, part->tile, part->x_pos, part->y_pos, part->z_pos, part->track,
					part->GetNumOrders(), part->cargo.TotalCount());
			}
		}
		return result;
	};
	const auto retained_trains = train_state();
	Money replacement_cost = 0;
	const Money before_replacement = Company::Get(_current_company)->money;
	// All temporary rails were earned by this exact finished paid job. Only
	// ordinary safe removal after whole withdrawal may expose their open floor.
	for (const auto &support : CaptureTBMSupport()) {
		REQUIRE(support.project == project.id);
		REQUIRE_FALSE(IsTBMSupportInUse(support.tile));
		const auto tracks = GetTrackBits(support.tile);
		for (const auto track : tracks) { // Remove each real native rail piece through its owner; no map-type overwrite or private support erasure creates the final-use footprint.
			replacement_cost += ExtractCommandCost(NonRailReuseCommand<Commands::RemoveRail>(support.tile, track)).GetCost();
		}
		CHECK(GetTBMSupportTile(support.tile) == nullptr);
	}
	REQUIRE(CaptureTBMSupport().empty());
	const auto terminal = TileAddWrap(world.site, 4, 3);
	const auto depot = TileAddWrap(world.site, 4, 7);
	// This small real terminal/depot occupies the original wide chamber. The
	// wider curved useful route and irregular acceptance remain Group21/Plan17.
	for (const int row : {2, 5, 6}) {
		const auto tile = TileAddWrap(world.site, 4, row);
		REQUIRE(GetGeologyWorldState().terrain.Get(tile) == ExcavationState::Excavated);
		replacement_cost += ExtractCommandCost(NonRailReuseCommand<Commands::BuildRail>(tile, tube, Track::Y, false)).GetCost();
	}
	const auto quote_cash = Company::Get(_current_company)->money;
	const auto quote = Command<Commands::BuildRailStation>::Do({}, terminal, tube, Axis::Y, uint8_t{1}, uint8_t{2},
		STAT_CLASS_DFLT, uint16_t{0}, NEW_STATION, false);
	REQUIRE(quote.Succeeded());
	CHECK(Company::Get(_current_company)->money == quote_cash);
	replacement_cost += ExtractCommandCost(NonRailReuseCommand<Commands::BuildRailStation>(terminal, tube, Axis::Y, uint8_t{1}, uint8_t{2},
		STAT_CLASS_DFLT, uint16_t{0}, NEW_STATION, false)).GetCost();
	replacement_cost += ExtractCommandCost(NonRailReuseCommand<Commands::BuildRailDepot>(depot, tube, DiagDirection::NW)).GetCost();
	CHECK(IsRailDepotTile(depot));
	CHECK(GetRailType(depot) == tube);
	CHECK(IsRailStationTile(terminal));
	CHECK(GetRailType(terminal) == tube);
	CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == geology);
	// The mine occupies a separate three-by-three footprint in this same earned
	// chamber. Funding may acquire its original seam claim, but cannot invent
	// floor, consume reserve or alter the completed paid excavation history.
	const auto seam = GetGeologyWorldState().deposits.Find(world.site);
	const auto mine_type = GetLayerMineIndustryType(world.resource);
	auto expected_snapshot = CaptureGeologyState();
	const auto expected_deposit = std::ranges::find(expected_snapshot.deposits, seam, &GeologyDeposit::id);
	REQUIRE(expected_deposit != expected_snapshot.deposits.end());
	REQUIRE(expected_deposit->claimant == CompanyID::Invalid());
	expected_deposit->claimant = _current_company;
	GeologyWorldState expected_geology;
	REQUIRE(PrepareGeologyRestoration(expected_snapshot, expected_geology) == GeologyError::None);
	const auto before_mine_quote = std::tuple{ComputeGeologyStateDigest(GetGeologyWorldState()), ComputeLayerMineDigest(),
		Industry::GetNumItems(), Company::Get(_current_company)->money, _random.state[0], _random.state[1]};
	REQUIRE(Command<Commands::BuildLayerMine>::Do({}, world.site, mine_type, uint32_t{0}, seam, uint32_t{1234}).Succeeded());
	CHECK((std::tuple{ComputeGeologyStateDigest(GetGeologyWorldState()), ComputeLayerMineDigest(),
		Industry::GetNumItems(), Company::Get(_current_company)->money, _random.state[0], _random.state[1]} == before_mine_quote));
	replacement_cost += ExtractCommandCost(NonRailReuseCommand<Commands::BuildLayerMine>(world.site, mine_type, uint32_t{0}, seam, uint32_t{1234})).GetCost();
	const auto mines = CaptureLayerMines();
	REQUIRE(mines.size() == 1);
	CHECK(mines.front().deposit == seam);
	CHECK(mines.front().company == _current_company);
	CHECK(mines.front().extracted == 0);
	CHECK(mines.front().dispatched == 0);
	CHECK(Industry::Get(mines.front().industry)->location.tile == world.site);
	CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == ComputeGeologyStateDigest(expected_geology));
	const auto road = TileAddWrap(world.site, 0, 6);
	replacement_cost += ExtractCommandCost(NonRailReuseCommand<Commands::BuildRoad>(road, ROAD_X, ROADTYPE_ROAD, DisallowedRoadDirections{}, TownID::Invalid())).GetCost();
	CHECK(IsTileType(road, TileType::Road));
	const auto open_chamber = TileAddWrap(world.site, 3, 8);
	CHECK(IsTileType(open_chamber, TileType::Clear));
	CHECK(GetGeologyWorldState().terrain.Get(open_chamber) == ExcavationState::Excavated);
	CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == ComputeGeologyStateDigest(expected_geology));
	CHECK(replacement_cost > 0);
	CHECK(before_replacement - Company::Get(_current_company)->money == replacement_cost);
	CHECK(train_state() == retained_trains);
	CHECK(*GetExcavationJob(job.id) == job);
	fmt::print("TBM_NONRAIL_REUSE ticks={} cuts={} earth={} rock={} work={} replacement={} protected_retreat=1 withdrawn=1 terminal={} depot={} mine={} road={} open_chamber={}\n",
		world.ticks, job.cursor, job.completed_earth_t, job.completed_rock_t, job.completed_work_cost, replacement_cost, terminal.base(), depot.base(),
		mines.front().industry.base(), road.base(), open_chamber.base());
}
