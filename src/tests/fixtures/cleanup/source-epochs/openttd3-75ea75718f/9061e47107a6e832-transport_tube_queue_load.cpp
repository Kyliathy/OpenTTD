/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file transport_tube_queue_load.cpp Real competing native fronts produce queued and grant-starved current file checkpoints.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Group14EB.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
 * Prepared floor/materials and loaded17-unit packets remain explicit boundary inputs, separate from useful-route/bootstrap acceptance.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "layer_window_fixture.h"
#include "layer_save_fixture.h"
#include "test_artifact_paths.h"
#include "geothermal_tube_switch_fixture.h"
#include "../pbs.h"
#include "../rail.h"
#include "../gfx_func.h"
#include "../table/sprites.h"
#include "../saveload/saveload_func.h"
#include "../safeguards.h"

/** Two command-bought native competitors share one original generator and a real common-exit junction. */
class TubeQueueFileRig : public GeothermalTubeSwitchRig {
	AutoRestoreBackup<bool> paths{_settings_game.pf.reserve_paths, true}; ///< Native depot departures must obtain exclusive real paths.
	AutoRestoreBackup<AccelerationModel> acceleration{_settings_game.vehicle.train_acceleration_model, AccelerationModel::Realistic}; ///< Actual native braking, not fixture relocation.
	AutoRestoreBackup<uint8_t> freight{_settings_game.vehicle.freight_trains, uint8_t{1}}; ///< Legal native loaded-mass setting, unchanged by afterload clamps.
	AutoRestoreBackup<uint8_t> retry{_settings_game.pf.wait_for_pbs_path, uint8_t{2}}; ///< Legal minimum retry policy retained in native settings transport.
	AutoRestoreBackup<uint8_t> backoff{_settings_game.pf.path_backoff_interval, uint8_t{20}}; ///< Normal nonzero native routing retry interval.
public:
	TileIndex junction = TileXYL(32, 25, MapLayer::DeepUnderground); ///< Actual shared NE exit for incoming NW/SE fronts.
	TileIndex destination = TileXYL(30, 25, MapLayer::DeepUnderground); ///< Native halt depot beyond the shared branch.
	std::array<Train *, 2> capsules{}; ///< Borrowed original fronts in stable actual purchase order, never new movement owners.
	std::array<CargoPacketID, 2> packets{}; ///< Original distinct native packet identities, each retaining17 units.
	std::array<TubeGeothermalSourceID, 2> sources{}; ///< Independent original inlet permissions drawing from the same sole producer.

	/** Buy the real merge, destination and competitors from original prepared native source facilities. */
	TubeQueueFileRig() : GeothermalTubeSwitchRig(true)
	{
		const auto coal = GetCargoTypeByLabel(CT_COAL); REQUIRE(coal != INVALID_CARGO);
		CargoSpec::Get(coal)->weight = 16; CargoSpec::Get(coal)->is_freight = true;
		this->Open(std::array{this->destination, TileXYL(31, 25, MapLayer::DeepUnderground)});
		REQUIRE(Command<Commands::RemoveRail>::Do(DoCommandFlag::Execute, this->junction, Track::Y).Succeeded());
		// Native Upper joins NW to NE; Right joins SE to NE. Both arrivals
		// compete for the same actual exit, while each has its own valid path.
		REQUIRE(Command<Commands::BuildRail>::Do(DoCommandFlag::Execute, this->junction, GetTubeRailType(), Track::Upper, false).Succeeded());
		REQUIRE(Command<Commands::BuildRail>::Do(DoCommandFlag::Execute, this->junction, GetTubeRailType(), Track::Right, false).Succeeded());
		REQUIRE(GetTrackBits(this->junction) == TrackBits{Track::Upper, Track::Right});
		REQUIRE(Command<Commands::BuildRail>::Do(DoCommandFlag::Execute, TileXYL(31, 25, MapLayer::DeepUnderground), GetTubeRailType(), Track::X, false).Succeeded());
		REQUIRE(Command<Commands::BuildRailDepot>::Do(DoCommandFlag::Execute, this->destination, GetTubeRailType(), DiagDirection::SW).Succeeded());
		this->sources = {this->geothermal.source, this->Connect(1).source};
		// Separate genuine inlet/feed identities draw only the original plant's
		// common output. Purchase order also supplies the stable native tick order.
		for (size_t index = 0; index < this->capsules.size(); ++index) {
			this->capsules[index] = this->Buy(index); this->packets[index] = this->Load(*this->capsules[index]);
			REQUIRE(GetTubeGeothermalSource(this->sources[index])->producer == this->output);
			REQUIRE(BindTubeGeothermalPowerVehicle(this->capsules[index]->index, this->sources[index]).has_value());
			this->Start(*this->capsules[index], this->destination);
		}
		REQUIRE(this->packets[0] != this->packets[1]);
	}
	/** Retire actual fronts through their native owners before clearing this fixture's additional destination, then allow ordinary base source/facility teardown. */
	~TubeQueueFileRig()
	{
		// The extra target can hold both original fronts. Native vehicle cleanup
		// owns their packets/requests before normal demolition may clear it;
		// the inherited teardown then handles only its original two inlets.
		while (Vehicle::GetNumItems() != 0) delete (*Vehicle::Iterate().begin())->First();
		if (IsRailDepotTile(this->destination)) CHECK(Command<Commands::LandscapeClear>::Do(DoCommandFlag::Execute, this->destination).Succeeded());
	}

	/** Check actual packet/physical/shared owners without repairing references, advancing time or allocating work. */
	void CheckOwners() const
	{
		// Every original front and packet must remain distinct and physically
		// valid, even while one waits or both lack a current electrical grant.
		for (size_t index = 0; index < this->capsules.size(); ++index) {
			const auto &capsule = *this->capsules[index];
			REQUIRE(IsTrainOnItsTiles(&capsule)); REQUIRE_FALSE(capsule.vehstatus.Test(VehState::Crashed));
			REQUIRE(capsule.cargo.StoredCount() == 17); REQUIRE(capsule.cargo.Packets()->size() == 1);
			CHECK(capsule.cargo.Packets()->front()->index == this->packets[index]);
			const auto *binding = GetTubePowerVehicle(capsule.index); REQUIRE(binding != nullptr);
			CHECK(binding->geothermal_source == this->sources[index]);
		}
		CHECK(ValidateTubePowerVehicles(CaptureTubePowerVehicles()) == TubePowerError::None);
		CHECK(ValidateTubeGeothermalPowerState(CaptureTubeGeothermalPowerState(), CaptureUtilityPowerState()));
		CHECK(ValidateUtilityPowerState(CaptureUtilityPowerState(), GetCurrentMultiLayerProfileSelection()) == UtilityPowerError::None);
	}

	/** Give both actual fronts one common real allocation and stable native opportunity, retaining all original custody. */
	void RunQueue()
	{
		this->Run(this->capsules); this->CheckOwners();
	}

	/** @return The actual departed winner when the other powered started front is natively stuck at its original entrance, or no earned queue. */
	std::optional<size_t> QueuedWinner() const
	{
		const auto reserved = GetReservedTrackbits(this->junction);
		// Either actual purchase may win native arbitration. Observe its own
		// installed incoming piece; do not assign priority, a path or a pose.
		for (size_t index = 0; index < this->capsules.size(); ++index) {
			const auto track = index == 0 ? Track::Upper : Track::Right;
			if (!reserved.Test(track)) continue;
			if (GetTrainForReservation(this->junction, track) != this->capsules[index]) continue;
			const auto &moving = *this->capsules[index], &waiting = *this->capsules[1 - index];
			// Actual track departure and native Stuck distinguish competition from
			// a startup timer, an already completed arrival or a hidden pose fixture.
			if (moving.IsInDepot() || moving.tile == this->inlets[index] || !waiting.IsInDepot() ||
					waiting.tile != this->inlets[1 - index] || waiting.vehstatus.Test(VehState::Stopped) ||
					!waiting.flags.Test(VehicleRailFlag::Stuck)) continue;
			const auto *first = GetTubePowerVehicle(moving.index), *second = GetTubePowerVehicle(waiting.index);
			// Both energy permissions must be satisfied; the waiting cause here
			// is the real shared path, separately from the later grant-starved file.
			if (first == nullptr || second == nullptr || first->wait != TubePowerWait::None || second->wait != TubePowerWait::None) continue;
			return index;
		}
		return std::nullopt;
	}
};

/**
 * Save one complete actual queued/starved/cleared world without mutating any owner.
 * @param rig Actual original fronts, sources and packets.
 * @param paths Invocation-owned generated save/report locations.
 * @param primary Observed real reservation winner, used only for the existing file-observer expectation tuple.
 * @param phase Diagnostic scene6 queued,7 grant-starved or8 genuinely cleared; never a simulation phase override.
 * @param step Zero is the original state; one is its uninterrupted next common native opportunity.
 */
static void SaveTubeQueueWorld(const TubeQueueFileRig &rig, const TestArtifactPaths &paths, size_t primary, unsigned phase, unsigned step)
{
	rig.CheckOwners(); const auto &capsule = *rig.capsules[primary];
	const auto *row = GetTubePowerVehicle(capsule.index); REQUIRE(row != nullptr);
	const auto utility = ComputeUtilityPowerDigest(), source = ComputeTubeGeothermalPowerDigest(), native = ComputeGeothermalGeneratorDigest();
	const auto binding = ComputeTubePowerVehicleDigest(), invoice = ComputeTubePowerDigest();
	SetMouseCursor(SPR_CURSOR_MOUSE, PAL_NONE);
	AutoRestoreBackup menu{_game_mode, GameMode::Menu}; AutoRestoreBackup pause{_pause_mode, PauseModes{PauseMode::Normal}};
	AutoRestoreBackup compression{_savegame_format, std::string{"lzma"}};
	const auto path = paths.Output("saves", fmt::format("tube-queue-{}-{}.sav", phase, step));
	REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(path), false) == SaveLoadResult::Ok);
	CHECK(ComputeUtilityPowerDigest() == utility); CHECK(ComputeTubeGeothermalPowerDigest() == source); CHECK(ComputeGeothermalGeneratorDigest() == native);
	CHECK(ComputeTubePowerVehicleDigest() == binding); CHECK(ComputeTubePowerDigest() == invoice);
	fmt::print("GEOTHERMAL_WORK_SAVE phase={} step={} source={} binding={} utility={} native={} invoice={} packet={} vehicle={} tick={} carry={} stopped={} path={}\n",
		phase, step, source, binding, utility, native, invoice, rig.packets[primary].base(), capsule.index.base(), TimerGameTick::counter,
		row->remainder_mj, capsule.vehstatus.Test(VehState::Stopped) ? 1 : 0, path.string());
}

/** Both independent actual writers earn reservations/shortage/repair and preserve original packets through normal complete arrivals. */
TEST_CASE("Native tube queues write real competing and grant starved worlds", "[tube-queue-save-file]")
{
	MockEnvironment::Instance(); LoadGeneratedWindowTestLanguage(); ResetRailTypes(); TubeQueueFileRig rig;
	LayerSaveContent content{rig.ActivatedContent(), true}; AutoRestoreBackup local{_local_company, _current_company};
	(*Town::Iterate().begin())->townnametype = SPECSTR_TOWNNAME_START;
	const auto paths = GetTestCaseArtifactPaths("tubes", "queue-files");
	std::optional<size_t> winner; uint ticks = 0;
	// Normal competing orders and depot/path drivers must earn a visible
	// real-track reservation owner and a separately powered stuck competitor.
	while (ticks++ < 1500) {
		rig.RunQueue(); winner = rig.QueuedWinner(); if (winner.has_value()) break;
	}
	REQUIRE(ticks < 1500); REQUIRE(winner.has_value()); const auto primary = *winner;
	const auto *moving = rig.capsules[primary], *waiting = rig.capsules[1 - primary];
	fmt::print("TUBE_QUEUED primary={} secondary={} primary_packet={} secondary_packet={} junction={} mask={} winner={} powered=true stuck=true tick={}\n",
		moving->index.base(), waiting->index.base(), rig.packets[primary].base(), rig.packets[1 - primary].base(), rig.junction.base(),
		GetTrackBits(rig.junction).base(), moving->index.base(), TimerGameTick::counter);
	SaveTubeQueueWorld(rig, paths, primary, 6, 0); rig.RunQueue(); SaveTubeQueueWorld(rig, paths, primary, 6, 1);
	REQUIRE(Command<Commands::RemoveGeothermalUtilityLink>::Do(DoCommandFlag::Execute, rig.pipe[1], UtilityEnergyDomain::Heat).Succeeded());
	rig.RunQueue();
	// Actual original Heat loss provides no electrical grant while both
	// original inlet/cable components remain physically valid. No output rate,
	// balance, private flag or vehicle coordinate is changed by the fixture.
	for (const auto *capsule : rig.capsules) {
		const auto *row = GetTubePowerVehicle(capsule->index); REQUIRE(row != nullptr);
		CHECK(row->wait == TubePowerWait::Grant); CHECK(capsule->cur_speed == 0);
		CHECK(IsTubeGeothermalPowerConnected(row->geothermal_source, *capsule));
	}
	fmt::print("TUBE_QUEUE_STARVED electrical_connected=true native_grant=false packets_intact=true tick={}\n", TimerGameTick::counter);
	SaveTubeQueueWorld(rig, paths, primary, 7, 0); rig.RunQueue(); SaveTubeQueueWorld(rig, paths, primary, 7, 1);
	rig.Build(rig.pipe[1], UtilityEnergyDomain::Heat); ticks = 0;
	// Repair only the original purchased path, then let the existing native
	// queue clear. Both original halted arrivals prove neither entry was a
	// disconnected decorative arm or a permanent fixture-induced deadlock.
	while (ticks++ < 6000) {
		rig.RunQueue();
		// A single arrival cannot certify queue clearing; both original halted
		// fronts must finish their own ordinary destination order.
		if (rig.capsules[0]->IsStoppedInDepot() && rig.capsules[1]->IsStoppedInDepot()) break;
	}
	REQUIRE(ticks < 6000);
	for (const auto *capsule : rig.capsules) {
		CHECK(capsule->tile == rig.destination); CHECK(capsule->cargo.StoredCount() == 17);
	}
	CHECK(GetReservedTrackbits(rig.junction).None());
	SaveTubeQueueWorld(rig, paths, primary, 8, 0);
	fmt::print("TUBE_QUEUE_CLEARED both_arrived=true reservation_released=true original_packets=true ticks={} tick={}\n", ticks, TimerGameTick::counter);
}
