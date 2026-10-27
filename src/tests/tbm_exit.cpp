/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file tbm_exit.cpp Paid fifty-cell excavation, local support handover and ordinary end-to-end traffic.
 * @see docs/zz-reach2/upgrades/2026-10/r2tbmf-tbm-fixes.md Groups9/11
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md
 * @see docs/zz-reach2/architecture/vehicles/archi-vehicles.md
 * @see docs/zz-reach2/upgrades/2026-10/r2lbf-linux-build-fixes.md Group 3 (member order portable to GCC/Clang)
 */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include <filesystem>
#include "mock_environment.h"
#include "underground_depot_fixture.h"
#include "depot_file_fixture.h"
#include "layer_window_fixture.h"
#include "../tbm_hangar.h"
#include "../order_cmd.h"
#include "../landscape_cmd.h"
#include "../company_cmd.h"
#include "../signal_func.h"
#include "../core/random_func.hpp"
#include "../script/api/script_rail.hpp"
#include "../script/api/script_companymode.hpp"
#include "../script/api/script_testmode.hpp"
#include "../script/api/script_execmode.hpp"
#include "../script/script_suspend.hpp"
#include "../game/game_instance.hpp"

#include "../safeguards.h"

/** Actual GameScript actor for portal estimate/execute parity through the public native command pipeline. */
class ExitScriptScope : public ScriptObject {
	ActiveInstance active; ///< Restored before the owning runtime is destroyed.
public:
	/** @param game Actual script instance. */
	explicit ExitScriptScope(GameInstance &game) : active(game) {}
	/** @return Actual last native command result after the ordinary script suspension. */
	static bool LastSucceeded() { return GetLastCommandRes(); }
};

/** Own the actual initial logistics and one paid straight fifty-cell job; no physical or geological state is synthesized. */
class TBMExitJourney {
	NativeDepotEconomy economy; ///< Native prices and normal company construction allowance.
public:
	UndergroundWorkflowWorld world; ///< Real generated floor and command-funded original logistics.
	LayerSaveContent content; ///< Actual activated package/engine identities for native snapshot files, after the world exists.
	TBMProjectID project{UINT32_MAX}; ///< Actual controller allocated by commissioning.
	VehicleID cutter = VehicleID::Invalid(); ///< Same machine retained through B and public opening.
	ExcavationJobID job{UINT32_MAX}; ///< Immutable original A-anchored job.
	TileIndex exit = INVALID_TILE; ///< Last accepted cutter centre on the rock layer.
	DiagDirection shaft = DiagDirection::Invalid; ///< B shaft points away from the retained A-side approach.
	std::string side; ///< Evidence namespace only; never a gameplay destination.

	// Defined before the constructor: a member with a deduced return type must be defined before its first use (GCC/Clang).
	/** @tparam Tcmd Actual synchronized command. @param args Its ordinary native arguments. @return Successful command result and real invoice. */
	template <Commands Tcmd, typename... Args>
	auto Do(Args... args)
	{
		auto result = Command<Tcmd>::Do(DoCommandFlag::Execute, args...);
		INFO("exit command=" << GetCommandName(Tcmd) << " error=" << ExtractCommandCost(result).GetErrorMessage().base());
		REQUIRE(ExtractCommandCost(result).Succeeded());
		return result;
	}

	/** @param side Evidence namespace for the reciprocal construction. @param cells Accepted initial length; final GUI acceptance extends44 to50. @param regional Use current new-map geology for the final integrated journey. */
	explicit TBMExitJourney(std::string side, uint cells = 50, bool regional = false) :
		world(MapLayer::Underground, true, true, regional ? GEOLOGY_HARDNESS_VERSION : 0), content(this->world.ActivatedContent(), true), side(std::move(side))
	{
		this->world.simulate_company_limits = true;
		this->Do<Commands::RenameCompany>("Exit Construction");
		this->shaft = GetLayerPortalDoorDirection(this->world.rock);
		const auto delta = TileIndexDiffCByDiagDir(this->shaft);
		this->exit = TileAddWrap(this->world.rock, delta.x * cells, delta.y * cells);
		REQUIRE(IsInnerTile(this->exit));
		const auto quote = PreviewTBMCommissionArea(_current_company, this->world.portal, 0, 2, 2, this->world.origin->index,
			this->world.rock, this->exit, false, ExcavationPlanIntent::StraightTunnel);
		REQUIRE(quote.purchase.Succeeded());
		REQUIRE(quote.request.proposal.route.size() == cells);
		this->Do<Commands::ConfirmTBMCommission>(this->world.portal, 0, 2, 2, this->world.origin->index,
			this->world.rock, this->exit, quote.fingerprint, ExcavationPlanIntent::StraightTunnel);
		const auto projects = CaptureTBMProjects();
		REQUIRE(projects.size() == 1);
		this->project = projects.front().id;
		this->cutter = projects.front().cutter;
		this->job = projects.front().job;
		this->Do<Commands::AssignTBMHauler>(this->project, this->world.hauler->index);
		Order origin;
		origin.MakeGoToStation(this->world.origin->index);
		this->Do<Commands::InsertOrder>(this->world.hauler->index, VehicleOrderID{0}, origin);
		this->Save("accepted");
		this->Do<Commands::StartStopVehicle>(this->cutter, true);
		this->Do<Commands::StartStopVehicle>(this->world.hauler->index, true);
	}


	/** @param state Genuinely reached stage. @return Nothing; optional recorder retains the actual native world. */
	void Save(std::string_view state) const { DepotFileRecorder{}.Save(fmt::format("exit-{}-{}", this->side, state)); }

	/** @param done Read-only actual-state checkpoint. @param limit Bounded native frames. @return Nothing; report actual train/controller state on a stall. */
	void DriveUntil(const std::function<bool()> &done, uint limit = 250000)
	{
		UpdateSignalsInBuffer(); // Finish the normal post-command signal batch before dispatching physical vehicle ticks.
		uint frames = 0;
		// The complete native world owns movement, work, wagon exchanges and
		// station disposal. This observer never advances an owner separately.
		while (!done() && frames < limit) {
			this->world.Tick();
			++frames;
			if (_pause_mode.Test(PauseMode::Error) || Train::Get(this->cutter)->vehstatus.Test(VehState::Crashed) || this->world.hauler->vehstatus.Test(VehState::Crashed)) break;
		}
		INFO("frames=" << frames << " cutter=" << DescribeTrain(Train::Get(this->cutter)) << " hauler=" << DescribeTrain(this->world.hauler));
		if (const auto *work = GetTBMProject(this->project)) {
			INFO("phase=" << uint{to_underlying(work->phase)} << " blocked=" << uint{to_underlying(work->blocked)} << " cursor=" << GetExcavationJob(work->job)->cursor);
			if (const auto *haul = GetTBMExchangeAssignmentForHauler(this->world.hauler->index)) {
				INFO("hauler phase=" << to_underlying(haul->phase) << " wait=" << to_underlying(haul->wait) << " exchanges=" << haul->exchanges);
				REQUIRE(done());
			}
			REQUIRE(done());
		} else {
			REQUIRE(done());
		}
	}

	/** Finish all paid cuts, physically retreat a few cells and stop the complete real consist locally. */
	void Stage()
	{
		/** @return Whether the last real atomic cut completed the fifty-cell plan. */
		const auto cut = [&]() { return GetExcavationJob(this->job)->state == ExcavationJobState::Completed; };
		this->DriveUntil(cut);
		CHECK(CheckTBMExitHandover(this->exit, this->shaft, RAILTYPE_RAIL).Failed());
		for (TileIndex clicked : {this->exit, MultiLayerMap::TileInLayer(this->exit, MapLayer::Surface)}) {
			const bool underground = GetTileLayer(clicked) == MapLayer::Underground;
			for (DoCommandFlags flags : {DoCommandFlags{}, DoCommandFlags{DoCommandFlag::Execute}}) {
				CHECK(Command<Commands::BuildLayerPortal>::Do(flags, clicked, TransportType::Rail,
					underground ? MapLayer::Surface : MapLayer::Underground, underground ? this->shaft : ReverseDiagDir(this->shaft), RAILTYPE_RAIL, INVALID_ROADTYPE).Failed());
			}
		}
		/** @return Whether native backing has cleared enough space for braking before the new housing and its door. */
		const auto clear = [&]() { return DistanceManhattan(Train::Get(this->cutter)->tile, this->exit) >= 5; };
		this->DriveUntil(clear);
		this->Do<Commands::StartStopVehicle>(this->cutter, true);
		/** @return Whether the actual stop flag and native braking have completed. */
		const auto stopped = [&]() { return Train::Get(this->cutter)->cur_speed == 0; };
		this->DriveUntil(stopped);
		REQUIRE(CheckTBMExitHandover(this->exit, this->shaft, RAILTYPE_RAIL).Succeeded());
		CHECK(GetTileLayer(Train::Get(this->cutter)->tile) == MapLayer::Underground);
		CHECK(DistanceManhattan(Train::Get(this->cutter)->tile, this->exit) < 12); // Construction never required withdrawal all the way to A.
		this->Save("local-stage");
	}
};

/** Retain a real paid44-cell current-geology plan; the ordinary application must append the final six cells through GUI input. */
TEST_CASE("Final integrated TBM input starts from an actual regional plan", "[.tbm-final-seed]")
{
	MockEnvironment::Instance();
	LoadGeneratedWindowTestLanguage();
	TBMExitJourney journey("final", 44, true);
	CHECK(GetGeologyWorldState().hardness_version == GEOLOGY_HARDNESS_VERSION);
	CHECK(GetExcavationJob(journey.job)->cursor == 0);
	CHECK(GetTBMProject(journey.project)->cutter == journey.cutter);
}

/** The same real machine and wagons build the far connection through either reciprocal command context, then clear it for normal trains. */
TEST_CASE("A fifty-cell tunnel hands its far end to B before the same TBM parks there", "[tbm-exit-journey]")
{
	MockEnvironment::Instance();
	LoadGeneratedWindowTestLanguage();
	const bool underground = GENERATE(true, false);
	TBMExitJourney journey(underground ? "underground" : "surface");
	journey.Stage();
	const TileIndex surface = MultiLayerMap::TileInLayer(journey.exit, MapLayer::Surface);
	const TileIndex clicked = underground ? journey.exit : surface;
	const MapLayer target = underground ? MapLayer::Surface : MapLayer::Underground;
	const DiagDirection shaft = underground ? journey.shaft : ReverseDiagDir(journey.shaft);
	journey.Do<Commands::BuildRail>(surface, RAILTYPE_RAIL, Track::Y, false);
	for (DoCommandFlags flags : {DoCommandFlags{}, DoCommandFlags{DoCommandFlag::Execute}}) {
		CHECK(Command<Commands::BuildLayerPortal>::Do(flags, clicked, TransportType::Rail, target, shaft, RAILTYPE_RAIL, INVALID_ROADTYPE).Failed());
	}
	journey.Do<Commands::LandscapeClear>(surface); // Paid ordinary removal of the deliberately conflicting far-side footprint.
	const auto work = *GetTBMProject(journey.project);
	const auto geological_job = *GetExcavationJob(journey.job);
	const auto supports = CaptureTBMSupport();
	const auto fleet = Vehicle::GetNumItems();
	const auto money = Company::Get(_current_company)->money;
	const auto random = _random;
	const auto geology = ComputeGeologyStateDigest(GetGeologyWorldState());
	const auto controllers = ComputeTBMProjectDigest();
	const auto quote = Command<Commands::BuildLayerPortal>::Do({}, clicked, TransportType::Rail, target, shaft, RAILTYPE_RAIL, INVALID_ROADTYPE);
	REQUIRE(quote.Succeeded());
	CHECK(Company::Get(_current_company)->money == money);
	CHECK(_random.state[0] == random.state[0]);
	CHECK(_random.state[1] == random.state[1]);
	CHECK(CaptureTBMSupport() == supports);
	CHECK(*GetExcavationJob(journey.job) == geological_job);
	for (DoCommandFlags flags : {DoCommandFlags{}, DoCommandFlags{DoCommandFlag::Execute}}) {
		CHECK(Command<Commands::BuildLayerPortal>::Do(flags, clicked, TransportType::Rail, target, ReverseDiagDir(shaft), RAILTYPE_RAIL, INVALID_ROADTYPE).Failed());
		CHECK(Command<Commands::LandscapeClear>::Do(flags, journey.exit).Failed()); // The explicit handover must not weaken ordinary protected-support demolition.
	}
	{
		REQUIRE(Company::CanAllocateItem());
		std::unique_ptr<Company> foreign{Company::Create()};
		foreign->avail_railtypes = Company::Get(work.company)->avail_railtypes; // A valid rival rail operator must reach the actual ownership refusal.
		AutoRestoreBackup actor(_current_company, foreign->index);
		CHECK(CheckTBMExitHandover(journey.exit, journey.shaft, RAILTYPE_RAIL).Failed());
		for (DoCommandFlags flags : {DoCommandFlags{}, DoCommandFlags{DoCommandFlag::Execute}}) {
			CHECK(Command<Commands::BuildLayerPortal>::Do(flags, clicked, TransportType::Rail, target, shaft, RAILTYPE_RAIL, INVALID_ROADTYPE).GetErrorMessage() == STR_ERROR_AREA_IS_OWNED_BY_ANOTHER);
		}
	}
	{
		AutoRestoreBackup actor(_current_company, work.company); // ScriptCompanyMode restores its script actor; this scope also restores the native test caller.
		GameInstance game;
		ExitScriptScope active(game);
		ScriptCompanyMode company(static_cast<ScriptCompany::CompanyID>(_current_company.base()));
		AutoRestoreBackup unpaused(_pause_mode, PauseModes{});
		ScriptRail::SetCurrentRailType(static_cast<ScriptRail::RailType>(RAILTYPE_RAIL));
		{
			ScriptTestMode estimate;
			CHECK(ScriptRail::BuildLayerPortal(clicked, to_underlying(target), to_underlying(shaft)));
			CHECK_FALSE(ScriptRail::BuildLayerPortal(clicked, to_underlying(target), to_underlying(ReverseDiagDir(shaft))));
		}
		{
			AutoRestoreBackup funds(Company::Get(_current_company)->money, Money{0});
			ScriptExecMode execute;
			CHECK_FALSE(ScriptRail::BuildLayerPortal(clicked, to_underlying(target), to_underlying(shaft)));
			CHECK(CaptureTBMSupport() == supports);
			CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == geology);
			CHECK(ComputeTBMProjectDigest() == controllers);
		}
		if (!underground) {
			ScriptExecMode execute;
			bool suspended = false;
			try {
				ScriptRail::BuildLayerPortal(clicked, to_underlying(target), to_underlying(shaft));
			} catch (Script_Suspend &) {
				suspended = true;
			}
			REQUIRE(suspended);
			REQUIRE(ExitScriptScope::LastSucceeded());
		}
	}
	if (underground) {
		const auto built = journey.Do<Commands::BuildLayerPortal>(clicked, TransportType::Rail, target, shaft, RAILTYPE_RAIL, INVALID_ROADTYPE);
		CHECK(built.GetCost() == quote.GetCost());
	}
	CHECK(money - Company::Get(_current_company)->money == quote.GetCost());
	REQUIRE(GetLayerPortalPairTile(surface) == journey.exit);
	CHECK(GetTBMSupportTile(journey.exit) == nullptr);
	CHECK(CaptureTBMSupport().size() + 1 == supports.size());
	CHECK(*GetExcavationJob(journey.job) == geological_job);
	CHECK(GetTBMProject(journey.project)->portal == work.portal);
	CHECK(Vehicle::GetNumItems() == fleet);
	journey.Save("built");
	const TileIndex door = GetLayerPortalDoorTile(surface);
	// Fifty cells put B near the map's southern edge. A funded ordinary
	// Surface curve leaves room for both required station approaches.
	Track bend = Track::Invalid;
	for (Trackdir direction : DiagdirReachesTrackdirs(GetLayerPortalDoorDirection(surface))) {
		if (TrackdirToExitdir(direction) == DiagDirection::NE) bend = TrackdirToTrack(direction);
	}
	REQUIRE(bend != Track::Invalid);
	const TileIndex link = TileAddWrap(door, -1, 0);
	const TileIndex station_tile = TileAddWrap(door, -2, 0);
	journey.Do<Commands::BuildRail>(door, RAILTYPE_RAIL, bend, false);
	journey.Do<Commands::BuildRail>(link, RAILTYPE_RAIL, Track::X, false);
	journey.Do<Commands::BuildRailStation>(station_tile, RAILTYPE_RAIL, Axis::X, uint8_t{1}, uint8_t{1}, STAT_CLASS_DFLT, uint16_t{0}, NEW_STATION, false);
	const StationID destination = Station::GetByTile(station_tile)->index;
	journey.Do<Commands::ChangeLayerPortalConnection>(surface, true);
	journey.Save("commissioned");
	journey.Do<Commands::SendTBMToHangar>(journey.cutter, surface, false);
	journey.Save("withdrawing");
	/** @return Whether every actual cutter part has reached B after original-origin cargo settlement. */
	const auto parked = [&]() { return GetTBMProject(journey.project)->phase == TBMPhase::ParkedHangar; };
	journey.DriveUntil(parked);
	CHECK(IsTBMWholeInHangar(*Train::Get(journey.cutter)));
	CHECK(Train::Get(journey.cutter)->tile == surface);
	CHECK(GetTBMProject(journey.project)->permanent_parts == work.permanent_parts);
	CHECK(*GetExcavationJob(journey.job) == geological_job);
	journey.Save("parked");
	/** @return Whether all actual loaded logistics and protected-route occupants have settled. */
	const auto settled = [&]() { return GetTBMProjectReleaseProblem(journey.project) == INVALID_STRING_ID; };
	journey.DriveUntil(settled);
	journey.Do<Commands::ControlTBMProject>(journey.project, TBMProjectAction::Retire);
	for (const auto &support : CaptureTBMSupport()) journey.Do<Commands::ConvertTBMSupport>(support.tile);
	journey.Do<Commands::ChangeLayerPortalConnection>(journey.world.portal, true);
	journey.Do<Commands::BuildRail>(geological_job.route[25].cutter_position, RAILTYPE_RAIL, Track::Left, false);
	CHECK(GetTBMProjectForCutter(journey.cutter) == nullptr);
	CHECK(GetTBMExchangeAssignmentForHauler(journey.world.hauler->index) == nullptr);
	journey.Save("released");
	for (StationID stop : {destination, journey.world.origin->index}) {
		Order order;
		order.MakeGoToStation(stop);
		order.SetLoadType(OrderLoadType::NoLoad);
		order.SetUnloadType(OrderUnloadType::NoUnload);
		journey.Do<Commands::InsertOrder>(journey.world.hauler->index, journey.world.hauler->GetNumOrders(), order);
	}
	if (journey.world.hauler->vehstatus.Test(VehState::Stopped)) journey.Do<Commands::StartStopVehicle>(journey.world.hauler->index, true);
	/** @return Whether ordinary station orders have carried the unassigned freight train through A and B. */
	const auto outward_arrival = [&]() { return journey.world.hauler->last_station_visited == destination; };
	journey.DriveUntil(outward_arrival);
	/** @return Whether the same ordinary freight train has returned through B and A to its original station. */
	const auto inward_arrival = [&]() { return journey.world.hauler->last_station_visited == journey.world.origin->index; };
	journey.DriveUntil(inward_arrival);
	CHECK(IsTBMWholeInHangar(*Train::Get(journey.cutter)));
	CHECK(Vehicle::GetNumItems() == fleet);
	journey.Save("traffic");
	fmt::print("EXIT_JOURNEY side={} cutter={} job={} cells={} ticks={}\n", journey.side, journey.cutter.base(), journey.job.base(), geological_job.cursor, journey.world.ticks);
}
