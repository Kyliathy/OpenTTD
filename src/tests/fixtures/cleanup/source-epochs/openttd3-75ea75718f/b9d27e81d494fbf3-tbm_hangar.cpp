/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file tbm_hangar.cpp Actual selected-hangar movement, preserved work and native service.
 * @see docs/zz-reach2/upgrades/2026-10/r2tbmf-tbm-fixes.md Group8
 * @see docs/zz-reach2/architecture/vehicles/archi-vehicles.md
 */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include <filesystem>
#include "mock_environment.h"
#include "underground_depot_fixture.h"
#include "depot_file_fixture.h"
#include "layer_window_fixture.h"
#include "../tbm_hangar.h"
#include "../core/random_func.hpp"
#include "../order_cmd.h"
#include "../landscape_cmd.h"
#include "../tbm_gui.h"
#include "../script/api/script_vehicle.hpp"
#include "../script/api/script_companymode.hpp"
#include "../script/api/script_testmode.hpp"
#include "../script/api/script_execmode.hpp"
#include "../script/script_suspend.hpp"
#include "../game/game_instance.hpp"
#include "../autoreplace_cmd.h"

#include "../safeguards.h"

/** Loaded work visits a separately built real destination and resumes without replacing its machine or geological job. */
TEST_CASE("A TBM unloads and parks at a selected portal before resuming its real cut", "[tbm-selected-hangar]")
{
	MockEnvironment::Instance();
	LoadGeneratedWindowTestLanguage();
	UndergroundDepotWorkflow workflow;
	workflow.Prepare();
	const TBMProjectID id = workflow.StartFurther();
	/** @return Whether real cutting has generated spoil and begun another unfinished face. */
	const auto cutting = [&]() {
		const auto *work = GetTBMProject(id);
		const auto *job = GetExcavationJob(work->job);
		return work->phase == TBMPhase::Bore && job->completed_earth_t != 0 && job->partial_work_ticks >= 20;
	};
	workflow.DriveUntil(cutting);
	const auto before = *GetExcavationJob(GetTBMProject(id)->job);
	const auto machinery = GetTBMProject(id)->permanent_parts;
	const size_t vehicles = Vehicle::GetNumItems(), depots = Depot::GetNumItems();
	const auto geology = ComputeGeologyStateDigest(GetGeologyWorldState()), controllers = ComputeTBMProjectDigest();
	const auto cash = Company::Get(_current_company)->money;
	const auto seed0 = _random.state[0], seed1 = _random.state[1];
	Train *front = Train::Get(workflow.first.cutter);
	const auto destination = DescribeTBMHangar(*front, workflow.connection);
	REQUIRE(destination.error == TBMHangarError::None);
	REQUIRE(Command<Commands::SendTBMToHangar>::Do({}, front->index, workflow.connection, false).Succeeded());
	CHECK(Depot::GetNumItems() == depots);
	CHECK(Vehicle::GetNumItems() == vehicles);
	CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == geology);
	CHECK(ComputeTBMProjectDigest() == controllers);
	CHECK(Company::Get(_current_company)->money == cash);
	CHECK(_random.state[0] == seed0);
	CHECK(_random.state[1] == seed1);
	workflow.Do<Commands::SendTBMToHangar>(front->index, workflow.connection, false);
	REQUIRE(front->current_order.IsType(OT_GOTO_DEPOT));
	const DepotID selected = front->current_order.GetDestination().ToDepotID();
	REQUIRE(Depot::Get(selected)->xy == destination.tile);
	CHECK(*GetExcavationJob(before.id) == before);
	DepotFileRecorder{}.Save("hangar-return");
	/** @return Whether every actual part has arrived at the ordered portal and native Halt completed. */
	const auto parked = [&]() { return GetTBMProject(id)->phase == TBMPhase::ParkedHangar; };
	workflow.DriveUntil(parked);
	CHECK(front->tile == destination.tile);
	CHECK(front->vehstatus.Test(VehState::Stopped));
	CHECK(IsTBMWholeInHangar(*front));
	CHECK(front->current_order.IsType(OT_DUMMY));
	CHECK(GetTBMProject(id)->permanent_parts == machinery);
	CHECK(*GetExcavationJob(before.id) == before);
	CHECK(Vehicle::GetNumItems() == vehicles);
	CHECK(GetLayerPortalHangarDepotIndex(destination.tile) == selected);
	DepotFileRecorder{}.Save("hangar-parked");
	workflow.Do<Commands::ControlTBMProject>(id, TBMProjectAction::Resume);
	/** @return Whether the same actual machine has physically rejoined and completed its retained partial face. */
	const auto resumed = [&]() { return GetExcavationJob(before.id)->cursor > before.cursor; };
	workflow.DriveUntil(resumed);
	CHECK(GetTBMProject(id)->cutter == front->index);
	CHECK(GetTBMProject(id)->job == before.id);
	CHECK(GetExcavationJob(before.id)->cursor == before.cursor + 1);
	CHECK(GetTBMProject(id)->permanent_parts == machinery);
	CHECK(Vehicle::GetNumItems() == vehicles);
	fmt::print("HANGAR_RESUMED cutter={} depot={} job={} partial={} ticks={}\n", front->index.base(), selected.base(), before.id.base(), before.partial_work_ticks, workflow.world.ticks);
}

/** The selected service command retains the ordinary native depot visit and the same paid unfinished work. */
TEST_CASE("A TBM visits a selected native depot for service and rejoins its work", "[tbm-hangar-service]")
{
	MockEnvironment::Instance();
	LoadGeneratedWindowTestLanguage();
	UndergroundDepotWorkflow workflow;
	workflow.Prepare();
	const auto id = workflow.StartFurther();
	/** @return Whether the actual cutter is at an unfinished face away from storage. */
	const auto cutting = [&]() { return GetTBMProject(id)->phase == TBMPhase::Bore && GetExcavationJob(GetTBMProject(id)->job)->partial_work_ticks >= 20; };
	workflow.DriveUntil(cutting);
	const auto before = *GetExcavationJob(GetTBMProject(id)->job);
	Train *front = Train::Get(workflow.first.cutter);
	workflow.Do<Commands::SendTBMToHangar>(front->index, workflow.connection, false); // The explicit service request below must supersede an unfinished selected-hangar trip.
	workflow.Do<Commands::SendTBMToHangar>(front->index, workflow.depot, true);
	REQUIRE_FALSE(front->current_order.GetDepotActionType().Test(OrderDepotActionFlag::Halt));
	bool serviced = false;
	/** @return Whether native depot entry completed the actual service order. */
	const auto entered = [&]() {
		serviced = front->tile == workflow.depot && front->IsChainInDepot() && !front->current_order.IsType(OT_GOTO_DEPOT);
		return serviced;
	};
	workflow.DriveUntil(entered);
	CHECK(*GetExcavationJob(before.id) == before);
	/** @return Whether native departure and approach reach the next real cut of the original job. */
	const auto resumed = [&]() { return GetExcavationJob(before.id)->cursor > before.cursor; };
	workflow.DriveUntil(resumed);
	CHECK(serviced);
	CHECK(GetTBMProject(id)->cutter == front->index);
	CHECK(GetTBMProject(id)->job == before.id);
}

/** Real GameScript actor scope; the native command and normal suspension protocol retain execution ownership. */
class HangarScriptScope : public ScriptObject {
	ActiveInstance active; ///< Restored before the owning GameInstance is destroyed.
public:
	/** @param game Actual script runtime used to invoke the public vehicle API. */
	explicit HangarScriptScope(GameInstance &game) : active(game) {}
	/** @return The runtime's actual most recent synchronized command result. */
	static bool LastSucceeded() { return GetLastCommandRes(); }
};

/** Ordinary station travel keeps the sole native list and suspends actual cutting until explicit Resume. */
TEST_CASE("TBM native station orders suspend and resume the same partial cut", "[tbm-native-orders]")
{
	MockEnvironment::Instance();
	LoadGeneratedWindowTestLanguage();
	UndergroundDepotWorkflow workflow;
	workflow.Prepare();
	const auto id = workflow.StartFurther();
	/** @return Whether the actual work controller has begun a real unfinished face. */
	const auto cutting = [&]() { return GetTBMProject(id)->phase == TBMPhase::Bore && GetExcavationJob(GetTBMProject(id)->job)->partial_work_ticks >= 20; };
	workflow.DriveUntil(cutting);
	Train *front = Train::Get(workflow.first.cutter);
	const auto before = *GetExcavationJob(GetTBMProject(id)->job);
	const auto machinery = GetTBMProject(id)->permanent_parts;
	const auto fleet = Vehicle::GetNumItems();
	Order station;
	station.MakeGoToStation(workflow.world.origin->index);
	station.SetLoadType(OrderLoadType::NoLoad);
	station.SetUnloadType(OrderUnloadType::NoUnload); // This explicit travel visit retains the same actual spoil packets.
	workflow.Do<Commands::InsertOrder>(front->index, VehicleOrderID{0}, station);
	CHECK(GetTBMProject(id)->phase == TBMPhase::FollowOrders);
	const TileIndex face = front->tile;
	/** @return Whether native orders have physically taken the cutter away from its retained face. */
	const auto away = [&]() { return front->tile != face && front->current_order.IsType(OT_GOTO_STATION); };
	workflow.DriveUntil(away);
	CHECK(*GetExcavationJob(before.id) == before);
	DepotFileRecorder{}.Save("orders-travel");
	/** @return Whether the actual train reached the requested ordinary Surface station. */
	const auto arrived = [&]() { return front->last_station_visited == workflow.world.origin->index && IsRailStationTile(front->GetMovingFront()->tile); };
	workflow.DriveUntil(arrived);
	CHECK(*GetExcavationJob(before.id) == before);
	CHECK(GetTBMProject(id)->phase == TBMPhase::FollowOrders);
	DepotFileRecorder{}.Save("orders-station");
	workflow.Do<Commands::DeleteOrder>(front->index, VehicleOrderID{0});
	/** @return Whether an exhausted native list holds safely instead of choosing an arbitrary rail branch. */
	const auto waiting = [&]() { return GetTBMTravelHoldReason(*front) == TBMBlockedReason::MissingOrders; };
	workflow.DriveUntil(waiting);
	CHECK(IsTBMHoldingTrain(*front));
	CHECK(*GetExcavationJob(before.id) == before);
	DepotFileRecorder{}.Save("orders-wait");
	workflow.Do<Commands::InsertOrder>(front->index, VehicleOrderID{0}, station);
	workflow.Do<Commands::ControlTBMProject>(id, TBMProjectAction::Resume);
	CHECK(front->GetNumOrders() == 1); // Work resume keeps the native list for a later explicit handoff.
	/** @return Whether the same actual train has legally rejoined and completed its retained partial face. */
	const auto resumed = [&]() { return GetExcavationJob(before.id)->cursor > before.cursor; };
	workflow.DriveUntil(resumed);
	CHECK(GetExcavationJob(before.id)->cursor == before.cursor + 1);
	CHECK(GetTBMProject(id)->permanent_parts == machinery);
	CHECK(Vehicle::GetNumItems() == fleet);
	workflow.Do<Commands::SkipToOrder>(front->index, VehicleOrderID{0});
	CHECK(GetTBMProject(id)->phase == TBMPhase::FollowOrders); // Even the same single order can deliberately take control again.
	fmt::print("NATIVE_ORDERS cutter={} job={} partial={} native_ticks={}\n", front->index.base(), before.id.base(), before.partial_work_ticks, workflow.world.ticks);
}

/** Shared orders cannot lend TBM capability, and removing the actual destination must safely preserve the accepted job. */
TEST_CASE("Hangar scripts and ordinary orders share admission and survive real destination removal", "[tbm-hangar-orders]")
{
	MockEnvironment::Instance();
	LoadGeneratedWindowTestLanguage();
	UndergroundDepotWorkflow workflow;
	workflow.Prepare();
	const auto id = workflow.StartFurther();
	Train *front = Train::Get(workflow.first.cutter);
	const auto before = *GetExcavationJob(GetTBMProject(id)->job);
	const auto controllers = ComputeTBMProjectDigest(), geology = ComputeGeologyStateDigest(GetGeologyWorldState());
	const auto fleet = Vehicle::GetNumItems(), depots = Depot::GetNumItems();
	const auto money = Company::Get(_current_company)->money;
	const auto random = _random;
	GameInstance game;
	HangarScriptScope active(game);
	ScriptCompanyMode company(static_cast<ScriptCompany::CompanyID>(_current_company.base()));
	AutoRestoreBackup unpaused(_pause_mode, PauseModes{});
	{
		ScriptTestMode estimate;
		CHECK(ScriptVehicle::SendTBMToHangar(front->index, workflow.connection, false));
		CHECK_FALSE(ScriptVehicle::SendTBMToHangar(workflow.world.hauler->index, workflow.connection, false));
		CHECK_FALSE(ScriptVehicle::SendTBMToHangar(front->index, INVALID_TILE, false));
	}
	CHECK(ComputeTBMProjectDigest() == controllers);
	CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == geology);
	CHECK(Vehicle::GetNumItems() == fleet);
	CHECK(Depot::GetNumItems() == depots);
	CHECK(Company::Get(_current_company)->money == money);
	CHECK(_random.state[0] == random.state[0]);
	CHECK(_random.state[1] == random.state[1]);
	{
		ScriptExecMode execute;
		bool suspended = false;
		try {
			ScriptVehicle::SendTBMToHangar(front->index, workflow.connection, false);
		} catch (Script_Suspend &resume) {
			suspended = true;
			CHECK(resume.GetSuspendCallback() != nullptr);
		}
		REQUIRE(suspended);
		REQUIRE(HangarScriptScope::LastSucceeded());
	}
	const auto destination = GetTBMOrderedHangar(*front);
	REQUIRE(destination.error == TBMHangarError::None);
	REQUIRE(destination.tile == workflow.connection);
	CHECK(*GetExcavationJob(before.id) == before);
	Order order;
	order.MakeGoToDepot(destination.depot, OrderDepotTypeFlag::PartOfOrders, OrderNonStopFlag::NonStop, OrderDepotActionFlag::Halt);
	workflow.Do<Commands::InsertOrder>(front->index, VehicleOrderID{0}, order);
	const auto hauler_orders = workflow.world.hauler->GetNumOrders();
	for (DoCommandFlags flags : {DoCommandFlags{}, DoCommandFlags{DoCommandFlag::Execute}}) {
		CHECK(Command<Commands::InsertOrder>::Do(flags, workflow.world.hauler->index, VehicleOrderID{0}, order).Failed());
		CHECK(Command<Commands::CloneOrder>::Do(flags, CO_SHARE, workflow.world.hauler->index, front->index).Failed());
		CHECK(Command<Commands::CloneOrder>::Do(flags, CO_COPY, workflow.world.hauler->index, front->index).Failed());
	}
	CHECK(workflow.world.hauler->GetNumOrders() == hauler_orders);
	CHECK(Vehicle::GetNumItems() == fleet);
	CHECK(CaptureTBMProjects().size() == 1);

	// The actual unoccupied remote portal is removed through its normal paid
	// command. Depot destruction must invalidate both current and list orders.
	workflow.Do<Commands::LandscapeClear>(workflow.connection);
	CHECK(Depot::GetIfValid(destination.depot) == nullptr);
	CHECK(front->current_order.IsType(OT_DUMMY));
	CHECK(front->GetOrder(VehicleOrderID{0})->IsType(OT_DUMMY));
	CHECK(front->vehstatus.Test(VehState::Stopped));
	CHECK(GetTBMTravelHoldReason(*front) == TBMBlockedReason::MissingHangar);
	CHECK(IsTBMHoldingTrain(*front));
	workflow.world.Tick();
	CHECK(*GetExcavationJob(before.id) == before);
	CHECK(Vehicle::GetNumItems() == fleet);
	workflow.Do<Commands::SendTBMToHangar>(front->index, workflow.world.portal, false);
	/** @return Whether the actual whole stored cutter completed its valid replacement destination. */
	const auto recovered = [&]() { return GetTBMProject(id)->phase == TBMPhase::ParkedHangar; };
	workflow.DriveUntil(recovered);
	CHECK(IsTBMWholeInHangar(*front));
	CHECK(*GetExcavationJob(before.id) == before);
	fmt::print("HANGAR_ORDERS cutter={} removed_depot={} retained_job={}\n", front->index.base(), destination.depot.base(), before.id.base());
}

/** Loaded ordinary travel must return from its real location without assuming that the cutter is still at its face. */
TEST_CASE("A loaded TBM can interrupt station travel for every return intent", "[tbm-hangar-acceptance]")
{
	MockEnvironment::Instance();
	LoadGeneratedWindowTestLanguage();
	UndergroundDepotWorkflow workflow;
	workflow.Prepare();
	const auto id = workflow.StartFurther();
	/** @return Whether actual cutting has loaded the wagons and retained another unfinished face. */
	const auto loaded = [&]() {
		const auto *work = GetTBMProject(id);
		const auto *job = GetExcavationJob(work->job);
		return work->phase == TBMPhase::Bore && job->completed_earth_t != 0 && job->partial_work_ticks >= 20;
	};
	workflow.DriveUntil(loaded);
	Train *front = Train::Get(workflow.first.cutter);
	const auto before = *GetExcavationJob(GetTBMProject(id)->job);
	const auto machinery = GetTBMProject(id)->permanent_parts;
	const auto fleet = Vehicle::GetNumItems();
	Order station;
	station.MakeGoToStation(workflow.world.origin->index);
	station.SetLoadType(OrderLoadType::NoLoad);
	station.SetUnloadType(OrderUnloadType::NoUnload);
	workflow.Do<Commands::InsertOrder>(front->index, VehicleOrderID{0}, station);
	/** @return Whether native station arrival has taken the same loaded cutter away from its work floor. */
	const auto arrived = [&]() { return front->last_station_visited == workflow.world.origin->index && front->current_order.IsType(OT_LOADING); };
	workflow.DriveUntil(arrived);
	CHECK(*GetExcavationJob(before.id) == before);
	TBMPhase destination = TBMPhase::Parked;
	bool cancel = false;
	SECTION("return and park") {
		workflow.Do<Commands::ControlTBMProject>(id, TBMProjectAction::ReturnAndPark);
	}
	SECTION("cancel and settle") {
		cancel = true;
		destination = TBMPhase::Cancelled;
		workflow.Do<Commands::ControlTBMProject>(id, TBMProjectAction::Cancel);
	}
	SECTION("chosen hangar") {
		destination = TBMPhase::ParkedHangar;
		workflow.Do<Commands::SendTBMToHangar>(front->index, workflow.connection, false);
	}
	SECTION("native list halt") {
		destination = TBMPhase::ParkedHangar;
		const DepotID depot = RegisterTBMHangarDepot(*front, workflow.connection);
		REQUIRE(depot != DepotID::Invalid());
		Order halt;
		halt.MakeGoToDepot(depot, OrderDepotTypeFlag::PartOfOrders, OrderNonStopFlag::NonStop, OrderDepotActionFlag::Halt);
		workflow.Do<Commands::InsertOrder>(front->index, VehicleOrderID{1}, halt);
		workflow.Do<Commands::SkipToOrder>(front->index, VehicleOrderID{1});
	}
	/** @return Whether the requested intent reached full physical storage after actual cargo settlement. */
	const auto settled = [&]() { return GetTBMProject(id)->phase == destination; };
	workflow.DriveUntil(settled);
	CHECK(IsTBMWholeInHangar(*front));
	CHECK(GetTBMProject(id)->permanent_parts == machinery);
	CHECK(Vehicle::GetNumItems() == fleet);
	const auto *after = GetExcavationJob(before.id);
	CHECK(after->cursor == before.cursor);
	CHECK(after->completed_earth_t == before.completed_earth_t);
	CHECK(after->completed_rock_t == before.completed_rock_t);
	if (cancel) CHECK(after->state == ExcavationJobState::Cancelled);
	else CHECK(*after == before);
}

/** A registered remote magazine keeps the actual machine protected, then allows public use and new work after settlement. */
TEST_CASE("A TBM releases at the remote hangar and commissions its next job there", "[tbm-hangar-acceptance][tbm-hangar-reuse]")
{
	MockEnvironment::Instance();
	LoadGeneratedWindowTestLanguage();
	UndergroundDepotWorkflow workflow;
	workflow.Prepare();
	const auto id = workflow.StartFurther();
	Train *front = Train::Get(workflow.first.cutter);
	const auto job_id = GetTBMProject(id)->job;
	const auto machinery = GetTBMProject(id)->permanent_parts;
	const auto fleet = Vehicle::GetNumItems();
	workflow.Do<Commands::SendTBMToHangar>(front->index, workflow.connection, false);
	/** @return Whether native movement and Halt have stored the whole actual train at B. */
	const auto parked = [&]() { return GetTBMProject(id)->phase == TBMPhase::ParkedHangar; };
	workflow.DriveUntil(parked);
	REQUIRE(IsTBMWholeInHangar(*front));
	const auto depot = GetLayerPortalHangarDepotIndex(workflow.connection);
	const auto retained = ComputeTBMProjectDigest();
	// Native commands, including script-callable commands, cannot dismantle,
	// replace, clone or sell an assigned machine merely because B has a DepotID.
	for (DoCommandFlags flags : {DoCommandFlags{}, DoCommandFlags{DoCommandFlag::Execute}}) {
		CHECK(Command<Commands::SellVehicle>::Do(flags, front->index, true, false, ClientID::Invalid).Failed());
		CHECK(Command<Commands::MoveRailVehicle>::Do(flags, front->Last()->index, VehicleID::Invalid(), false).Failed());
		CHECK(Command<Commands::AutoreplaceVehicle>::Do(flags, front->index).Failed());
		CHECK(ExtractCommandCost(Command<Commands::CloneVehicle>::Do(flags, workflow.connection, front->index, true)).Failed());
		CHECK(Command<Commands::LandscapeClear>::Do(flags, workflow.connection).Failed());
	}
	CHECK(ComputeTBMProjectDigest() == retained);
	CHECK(Vehicle::GetNumItems() == fleet);
	/** @return Whether the original logistics owner has settled so final release is admissible. */
	const auto releasable = [&]() { return GetTBMProjectReleaseProblem(id) == INVALID_STRING_ID; };
	workflow.DriveUntil(releasable);
	workflow.Do<Commands::ControlTBMProject>(id, TBMProjectAction::Retire);
	CHECK(GetTBMProject(id) == nullptr);
	CHECK(GetExcavationJob(job_id)->state == ExcavationJobState::Cancelled);
	CHECK(Depot::Get(depot)->xy == workflow.connection);
	CHECK(IsTBMConsistReusableAtPortal(*front, workflow.connection, _current_company, workflow.first.model_id));
	DepotFileRecorder{}.Save("remote-released");

	Order service;
	service.MakeGoToDepot(GetDepotIndex(workflow.depot), OrderDepotTypeFlag::PartOfOrders, OrderNonStopFlag::NonStop, {});
	Order origin;
	origin.MakeGoToStation(workflow.world.origin->index);
	origin.SetLoadType(OrderLoadType::NoLoad);
	origin.SetUnloadType(OrderUnloadType::NoUnload);
	workflow.Do<Commands::InsertOrder>(workflow.world.hauler->index, VehicleOrderID{0}, service);
	workflow.Do<Commands::InsertOrder>(workflow.world.hauler->index, VehicleOrderID{1}, origin);
	if (workflow.world.hauler->vehstatus.Test(VehState::Stopped)) workflow.Do<Commands::StartStopVehicle>(workflow.world.hauler->index, true);
	bool upper_seen = false, lower_seen = false;
	// This is now an ordinary train using its ordinary list. Observe real
	// public traffic beside the retained whole cutter, including save validity.
	for (uint tick = 0; tick < 15000 && !(upper_seen && lower_seen); ++tick) {
		workflow.world.Tick();
		for (const Train *part = workflow.world.hauler; part != nullptr; part = part->Next()) {
			upper_seen |= part->tile == workflow.connection;
			lower_seen |= part->tile == GetLayerPortalPairTile(workflow.connection);
		}
		REQUIRE(IsTBMWholeInHangar(*front));
		REQUIRE_FALSE(workflow.world.hauler->vehstatus.Test(VehState::Crashed));
	}
	REQUIRE(upper_seen);
	REQUIRE(lower_seen);
	// Withdraw ordinary traffic before changing B from public service to a
	// new work anchor. This is the existing explicit portal-owner operation.
	workflow.Do<Commands::SendVehicleToDepot>(workflow.world.hauler->index, DepotCommandFlags{}, VehicleListIdentifier{});
	for (uint tick = 0; tick < 15000 && !workflow.world.hauler->IsStoppedInDepot(); ++tick) workflow.world.Tick();
	REQUIRE(workflow.world.hauler->IsStoppedInDepot());
	workflow.Do<Commands::ChangeLayerPortalConnection>(workflow.connection, false);
	const TileIndex lower = GetLayerPortalPairTile(workflow.connection);
	// The bay lies north-west of B. New east-side work leaves that existing
	// facility outside the full cutter envelope; a straight northern corridor
	// would correctly refuse because its swept footprint intersects the depot.
	const TileIndex start = lower;
	const TileIndex end = TileAddWrap(lower, 6, -4);
	const auto consist = GetDefaultTBMConsist(MapLayer::Underground);
	/** @return The commission quote for the new east-side work at B. */
	const auto quote_remote = [&] {
		return PreviewTBMCommissionArea(_current_company, workflow.connection, workflow.first.model_id,
			consist.earth_wagons, consist.rock_wagons, workflow.world.origin->index, start, end, false, ExcavationPlanIntent::Area);
	};
	// Since R2DF Group 7 a quote names the first existing-track piece its route's support lacks (r2df task 7.3), instead
	// of accepting a route its cut would refuse. The player funds those pieces by ordinary construction on the already
	// open floor and quotes again, as UndergroundDepotWorkflow::FundFurtherRoute does; the refused quote carries the route.
	const auto unfunded = quote_remote();
	if (unfunded.purchase.Failed()) {
		REQUIRE(unfunded.purchase.GetErrorMessage() == STR_ERROR_TBM_COMMISSION_SUPPORT_PIECE);
		std::vector<size_t> pieces;
		workflow.BuildRailPath(GetTBMWorkRailPath(unfunded.area.proposal, unfunded.area.proposal.route.size(), pieces), true);
	}
	const auto quote = quote_remote();
	INFO("remote quote error=" << quote.purchase.GetErrorMessage().base() << " geology=" << uint{to_underlying(quote.area.error)}
		<< " failure_tile=" << quote.area.failure_tile.base());
	REQUIRE(quote.purchase.Succeeded());
	REQUIRE(quote.request.reuse_cutter == front->index);
	CHECK(quote.purchase.GetCost() == 0);
	std::vector<size_t> ends;
	workflow.BuildRailPath(GetTBMWorkRailPath(quote.request.proposal, quote.request.proposal.route.size(), ends), true);
	// Paid ordinary track can change the quote's topology; review once more
	// before the synchronized confirmation rather than accepting a stale quote.
	const auto prepared = PreviewTBMCommissionArea(_current_company, workflow.connection, workflow.first.model_id,
		consist.earth_wagons, consist.rock_wagons, workflow.world.origin->index, start, end, false, ExcavationPlanIntent::Area);
	REQUIRE(prepared.purchase.Succeeded());
	workflow.Do<Commands::ConfirmTBMCommission>(workflow.connection, workflow.first.model_id, consist.earth_wagons, consist.rock_wagons,
		workflow.world.origin->index, start, end, prepared.fingerprint, ExcavationPlanIntent::Area);
	const auto *next = GetTBMProjectForCutter(front->index);
	REQUIRE(next != nullptr);
	CHECK(next->job != job_id);
	CHECK(next->portal == workflow.connection);
	CHECK(next->permanent_parts == machinery);
	CHECK(Vehicle::GetNumItems() == fleet);
	workflow.Do<Commands::SetTBMExchangeBay>(next->id, workflow.depot);
	workflow.Do<Commands::AssignTBMHauler>(next->id, workflow.world.hauler->index);
	const auto next_job = next->job;
	const auto first_cut = EstimateExcavationCut(next_job);
	REQUIRE(first_cut.error == ExcavationError::None);
	CHECK(ResolveExcavationStarter(*GetExcavationJob(next_job)).required);
	CHECK(first_cut.new_cells != 0); // Reusing the open centre never grants free clearance around the magazine.
	auto missing_issuer = CaptureLayerPortalWorkRecords();
	std::erase_if(missing_issuer, [next_job](const LayerPortalWorkRecord &record) { return record.project == next_job; });
	CHECK(ValidateLayerPortalWorkRecords(missing_issuer, GetCurrentMultiLayerProfileSelection(), GetGeologyWorldState()) == LayerPortalWorkError::InvalidReference);
	DepotFileRecorder{}.Save("remote-reassigned");
	workflow.Do<Commands::StartStopVehicle>(front->index, true);
	workflow.Do<Commands::StartStopVehicle>(workflow.world.hauler->index, true);
	/** @return Whether the reused B machine completed a real new excavation cut. */
	const auto cutting = [&]() { return GetExcavationJob(next_job)->completed_earth_t != 0; };
	workflow.DriveUntil(cutting);
	CHECK(GetExcavationJob(next_job)->completed_earth_t == first_cut.earth_t);
	CHECK(GetExcavationJob(next_job)->completed_rock_t == first_cut.rock_t);
	for (TileIndex tile : GetExcavationCutterEnvelope(lower)) CHECK(GetGeologyWorldState().terrain.Get(tile) == ExcavationState::Excavated);
}

/** Real company lifecycle transfers or removes the registered native depot together with its project and actual fleet. */
TEST_CASE("Registered TBM hangars follow native company lifecycle", "[tbm-hangar-acceptance]")
{
	MockEnvironment::Instance();
	LoadGeneratedWindowTestLanguage();
	UndergroundDepotWorkflow workflow;
	workflow.Prepare();
	const auto id = workflow.StartFurther();
	Train *front = Train::Get(workflow.first.cutter);
	workflow.Do<Commands::SendTBMToHangar>(front->index, workflow.connection, false);
	const auto destination = GetTBMOrderedHangar(*front);
	REQUIRE(destination.error == TBMHangarError::None);
	const auto job = GetTBMProject(id)->job;
	const auto old_owner = front->owner;
	const auto fleet = Vehicle::GetNumItems();
	REQUIRE(Company::CanAllocateItem());
	std::unique_ptr<Company> buyer{Company::Create()};
	AutoRestoreBackup local(_local_company, buyer->index);
	SECTION("takeover preserves native identities") {
		ChangeOwnershipOfCompanyItems(old_owner, buyer->index);
		CHECK(front->owner == buyer->index);
		CHECK(GetTBMProject(id)->company == buyer->index);
		CHECK(GetExcavationJob(job)->company == buyer->index);
		CHECK(GetTileOwner(destination.tile) == buyer->index);
		CHECK(GetTBMOrderedHangar(*front).depot == destination.depot);
		CHECK(Vehicle::GetNumItems() == fleet);
		AutoRestoreBackup actor(_current_company, buyer->index);
		/** @return Whether the buyer's actual machine completes its inherited native order. */
		const auto parked = [&]() { return GetTBMProject(id)->phase == TBMPhase::ParkedHangar; };
		workflow.DriveUntil(parked);
		CHECK(IsTBMWholeInHangar(*front));
		ChangeOwnershipOfCompanyItems(buyer->index, old_owner); // Restore live fleet ownership before the temporary buyer is destroyed.
	}
	SECTION("bankruptcy releases storage and controller references") {
		ChangeOwnershipOfCompanyItems(old_owner, INVALID_OWNER);
		CHECK(GetTBMProject(id) == nullptr);
		CHECK(Depot::GetIfValid(destination.depot) == nullptr);
		CHECK_FALSE(IsLayerPortalTile(destination.tile));
		CHECK(GetExcavationJob(job)->state == ExcavationJobState::Cancelled);
		CHECK(CaptureTBMExchangeAssignments().empty());
		CHECK(CaptureTBMWagonSets().empty());
	}
}

/** Sharing native travel instructions preserves separate real work identities and cannot overfill another portal magazine. */
TEST_CASE("Hangar admission and shared travel keep distinct actual cutters", "[tbm-hangar-acceptance]")
{
	MockEnvironment::Instance();
	LoadGeneratedWindowTestLanguage();
	UndergroundDepotWorkflow workflow;
	workflow.Prepare();
	const auto first_id = workflow.StartFurther();
	Train *first = Train::Get(workflow.first.cutter);
	std::vector<std::pair<TileIndex, TrackBits>> approaches;
	// Remove only ordinary, vacated public approaches. The work claims and
	// the whole real cutter remain untouched while native pathfinding waits.
	for (TileIndex housing : {workflow.connection, GetLayerPortalPairTile(workflow.connection)}) {
		const TileIndex door = GetLayerPortalDoorTile(housing);
		REQUIRE(IsPlainRailTile(door));
		approaches.emplace_back(door, GetTrackBits(door));
		for (Track track : approaches.back().second) workflow.Do<Commands::RemoveRail>(door, track);
	}
	const auto waiting_job = *GetExcavationJob(GetTBMProject(first_id)->job);
	workflow.Do<Commands::SendTBMToHangar>(first->index, workflow.connection, false);
	for (uint tick = 0; tick < 1200; ++tick) workflow.world.Tick();
	CHECK(GetTBMProject(first_id)->phase == TBMPhase::TravelToHangar);
	CHECK(first->tile != workflow.connection);
	CHECK(*GetExcavationJob(waiting_job.id) == waiting_job);
	CHECK_FALSE(first->vehstatus.Test(VehState::Crashed));
	for (const auto &[door, tracks] : approaches) {
		for (Track track : tracks) workflow.Do<Commands::BuildRail>(door, RAILTYPE_RAIL, track, false);
	}
	/** @return Whether the first real cutter occupies B's entire storage bay. */
	const auto parked = [&]() { return GetTBMProject(first_id)->phase == TBMPhase::ParkedHangar; };
	workflow.DriveUntil(parked);
	const auto first_job = *GetExcavationJob(GetTBMProject(first_id)->job);
	const TileIndex portal = TileXYL(40, 40, MapLayer::Surface);
	workflow.Do<Commands::BuildLayerPortal>(portal, TransportType::Rail, MapLayer::Underground, DiagDirection::NW, RAILTYPE_RAIL, ROADTYPE_ROAD);
	const TileIndex lower = GetLayerPortalPairTile(portal);
	const auto quote = PreviewTBMCommissionArea(_current_company, portal, workflow.first.model_id, 2, 2, workflow.world.origin->index,
		lower, TileAddWrap(lower, 0, -6), false, ExcavationPlanIntent::StraightTunnel);
	REQUIRE(quote.purchase.Succeeded());
	workflow.Do<Commands::ConfirmTBMCommission>(portal, workflow.first.model_id, 2, 2, workflow.world.origin->index,
		lower, TileAddWrap(lower, 0, -6), quote.fingerprint, ExcavationPlanIntent::StraightTunnel);
	const auto projects = CaptureTBMProjects();
	REQUIRE(projects.size() == 2);
	const auto &second_project = projects.back();
	Train *second = Train::Get(second_project.cutter);
	REQUIRE(second->index != first->index);
	const auto second_job = *GetExcavationJob(second_project.job);
	CHECK(DescribeTBMHangar(*second, workflow.connection).error == TBMHangarError::Occupied);
	// Declare a different available native gauge in this fixture, then build
	// its genuine paid depot; the cutter's compatibility stays authoritative.
	AutoRestoreBackup gauges(Company::Get(_current_company)->avail_railtypes);
	Company::Get(_current_company)->avail_railtypes.Set(RAILTYPE_MONO);
	const TileIndex incompatible = TileXYL(3, 3, MapLayer::Surface);
	workflow.Do<Commands::BuildRailDepot>(incompatible, RAILTYPE_MONO, DiagDirection::SE);
	CHECK(DescribeTBMHangar(*first, incompatible).error == TBMHangarError::IncompatibleRail);
	const auto controllers = ComputeTBMProjectDigest(), geology = ComputeGeologyStateDigest(GetGeologyWorldState());
	const auto cash = Company::Get(_current_company)->money;
	const auto fleet = Vehicle::GetNumItems(), depots = Depot::GetNumItems();
	for (DoCommandFlags flags : {DoCommandFlags{}, DoCommandFlags{DoCommandFlag::Execute}}) {
		CHECK(Command<Commands::SendTBMToHangar>::Do(flags, second->index, workflow.connection, false).Failed());
		CHECK(Command<Commands::SendTBMToHangar>::Do(flags, first->index, portal, false).Failed()); // The other live commissioning pocket is not interchangeable storage.
		CHECK(Command<Commands::SendTBMToHangar>::Do(flags, first->index, INVALID_TILE, false).Failed());
		CHECK(Command<Commands::SendTBMToHangar>::Do(flags, first->index, incompatible, false).Failed());
		CHECK(Command<Commands::SendTBMToHangar>::Do(flags, workflow.world.hauler->index, workflow.connection, false).Failed());
	}
	REQUIRE(Company::CanAllocateItem());
	std::unique_ptr<Company> foreign{Company::Create()};
	{
		AutoRestoreBackup actor(_current_company, foreign->index);
		for (DoCommandFlags flags : {DoCommandFlags{}, DoCommandFlags{DoCommandFlag::Execute}}) {
			CHECK(Command<Commands::SendTBMToHangar>::Do(flags, first->index, workflow.connection, false).Failed());
		}
	}
	CHECK(ComputeTBMProjectDigest() == controllers);
	CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == geology);
	CHECK(Company::Get(_current_company)->money == cash);
	CHECK(Vehicle::GetNumItems() == fleet);
	CHECK(Depot::GetNumItems() == depots);
	Order station;
	station.MakeGoToStation(workflow.world.origin->index);
	station.SetLoadType(OrderLoadType::NoLoad);
	station.SetUnloadType(OrderUnloadType::NoUnload);
	workflow.Do<Commands::InsertOrder>(first->index, VehicleOrderID{0}, station);
	workflow.Do<Commands::CloneOrder>(CO_SHARE, second->index, first->index);
	CHECK(first->orders == second->orders);
	CHECK(GetTBMProject(first_id)->cutter == first->index);
	CHECK(GetTBMProject(second_project.id)->cutter == second->index);
	CHECK(GetTBMProject(first_id)->phase == TBMPhase::FollowOrders);
	CHECK(GetTBMProject(second_project.id)->phase == TBMPhase::FollowOrders);
	CHECK(*GetExcavationJob(first_job.id) == first_job);
	CHECK(*GetExcavationJob(second_job.id) == second_job);
	CHECK(CaptureTBMProjects().size() == 2);
	workflow.Do<Commands::CloneOrder>(CO_UNSHARE, second->index, VehicleID::Invalid());
	CHECK(first->orders != second->orders);
	CHECK(GetTBMProject(second_project.id)->job == second_job.id);
}
