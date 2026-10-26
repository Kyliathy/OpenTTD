/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file asteroid_lifecycle_native.cpp Actual ordinary tick ordering with original retained native hulls, cargo and saved field ownership.
 * @see docs/zz-reach2/upgrades/2026-10/r2sff1-space-fixes-features-1.md Group29.
 * Only last-contained geometry and due timers are declared. No source identity, reserve, cargo, paid job or extraction result is fabricated. This is bounded lifecycle acceptance, not a financial or elapsed-wall-time trial.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "asteroid_lifecycle_native.h"
#include "../asteroid.h"
#include "../asteroid_lifecycle.h"
#include "../space_vehicle.h"
#include "../space_mining_continuation.h"
#include "../space_work_cost.h"
#include "../vehicle_cmd.h"
#include "../company_base.h"
#include "../company_func.h"
#include "../cargopacket.h"
#include "../openttd.h"
#include "../gfx_func.h"
#include "../window_func.h"
#include "../map_layer_gui.h"
#include "../core/backup_type.hpp"
#include "../timer/timer_game_tick.h"
#include "../saveload/saveload_func.h"
#include <thread>
#include <filesystem>
#include "../safeguards.h"

/** @return Copied original packet identity/source/quantity only; genuine ordinary opportunities may advance age/accounting through their existing owners. */
static std::vector<std::tuple<CargoPacketID, Source, uint16_t>> ObserveLifecycleCargo()
{
	std::vector<std::tuple<CargoPacketID, Source, uint16_t>> result;
	// Include every allocated original packet, not only a selected Space hold;
	// retired history must remain identical without borrowing a new live ID.
	for (const auto *packet : CargoPacket::Iterate()) result.emplace_back(packet->index, packet->GetSource(), packet->Count());
	return result;
}

/** @copydoc RunSpaceLifecycleGameOpportunity */
void RunSpaceLifecycleGameOpportunity(SpaceWorkSample *sample)
{
	const auto before = GetAsteroidField(); const auto tick = TimerGameTick::counter;
	const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(30);
	// At most one completed opportunity is accepted. Paused retries preserve the
	// original field and clock instead of earning hidden timer or body progress.
	for (;;) {
		REQUIRE(std::chrono::steady_clock::now() < deadline);
		if (sample != nullptr) { ScopedSpaceWork work(*sample); StateGameLoop(); }
		else StateGameLoop();
		if (GetAsteroidField().elapsed_ticks == before.elapsed_ticks + 1) break;
		REQUIRE(GetAsteroidField() == before); REQUIRE(TimerGameTick::counter == tick); std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
	CHECK(TimerGameTick::counter == tick + 1); CHECK(ValidateAsteroidField(GetAsteroidField())); CHECK(ValidateNativeSpaceOrders());
}

/** @copydoc VerifyAsteroidLifecycleNative */
void VerifyAsteroidLifecycleNative(const char *output)
{
	REQUIRE(_game_mode == GameMode::Normal); REQUIRE(GetAsteroidField().renewal.credits == 0);
	auto *body = GetMutableAsteroidBody(OrbitalBodyID{0}); REQUIRE(body); REQUIRE(body->mining_slot != VehicleID::Invalid());
	auto *miner = SpaceVehicle::GetIfValid(body->mining_slot); REQUIRE(miner); REQUIRE(miner->mission.phase == SpaceMissionPhase::Mining);
	const auto cargo = ObserveLifecycleCargo(); REQUIRE_FALSE(cargo.empty()); const auto source = miner->mission.target;
	// Stop the actual original hulls through their native command. Safety must
	// still release a threatened lease, while later native ticks retain their
	// original idle-upkeep contract and cannot mine or navigate during this run.
	for (auto *craft : SpaceVehicle::Iterate()) {
		AutoRestoreBackup actor(_current_company, craft->owner);
		if (!craft->vehstatus.Test(VehState::Stopped)) REQUIRE(CmdStartStopVehicle(DoCommandFlag::Execute, craft->index, true).Succeeded());
	}
	auto field = GetAsteroidField(); const auto bounds = *GetSpacePlaneBounds(field.width, field.height);
	auto *first = FindAsteroidBody(field, source); REQUIRE(first); first->motion.velocity_x = -16384; first->motion.velocity_y = 0;
	const auto west = ForecastAsteroidExit(*first, bounds); REQUIRE(west); first->motion = west->last_safe_motion;
	auto *corner = FindAsteroidBody(field, OrbitalBodyID{3}); REQUIRE(corner); REQUIRE(corner->mining_slot == VehicleID::Invalid());
	corner->motion.position = bounds.max; corner->motion.velocity_x = 11585; corner->motion.velocity_y = 11585; corner->motion.remainder_x = 73; corner->motion.remainder_y = 73;
	const std::array events{AsteroidExitRecord{first->id, {first->size, first->resource_class}}, AsteroidExitRecord{corner->id, {corner->size, corner->resource_class}}};
	const auto initial = field; const auto expected = ApplyAsteroidExitCredit(field.renewal, events, field.next_body_id); REQUIRE(expected);
	REQUIRE(InstallAsteroidField(std::move(field))); UpdateAttachedSpaceCraftTransforms(); const auto retained_point = miner->position;
	SetViewportLayer(GetMainWindow(), MapLayer::Surface); AutoRestoreBackup running(_pause_mode, PauseModes{});
	RunSpaceLifecycleGameOpportunity(); CHECK(GetAsteroidField().renewal == *expected); CHECK(GetAsteroidField().next_body_id == initial.next_body_id);
	CHECK_FALSE(GetMutableAsteroidBody(source)); CHECK_FALSE(GetMutableAsteroidBody(OrbitalBodyID{3})); CHECK(miner->position == retained_point);
	CHECK(miner->vehstatus.Test(VehState::Stopped)); CHECK(HasSpaceMiningContinuationOrder(*miner)); CHECK(ObserveLifecycleCargo() == cargo);
	const auto frozen = GetAsteroidField(); const auto paused_tick = TimerGameTick::counter;
	{
		AutoRestoreBackup paused(_pause_mode, PauseModes{PauseMode::Normal});
		// Real paused callbacks may update GUI/script machinery, but cannot earn
		// even one fraction of the saved field countdown or another exit receipt.
		for (uint attempt = 0; attempt < 4; ++attempt) StateGameLoop();
		CHECK(GetAsteroidField() == frozen); CHECK(TimerGameTick::counter == paused_tick);
	}
	{
		AutoRestoreBackup editor(_game_mode, GameMode::Editor); StateGameLoop();
		CHECK(GetAsteroidField() == frozen); CHECK(TimerGameTick::counter == paused_tick);
	}
	// Declare one further unleased exit and a one-tick deadline. Its actual
	// ordinary opportunity must incorporate the new latest size before renewal.
	field = GetAsteroidField(); auto *next = FindAsteroidBody(field, OrbitalBodyID{4}); REQUIRE(next); REQUIRE(next->mining_slot == VehicleID::Invalid());
	const auto next_exit = ForecastAsteroidExit(*next, bounds); REQUIRE(next_exit); next->motion = next_exit->last_safe_motion;
	field.renewal.ticks_remaining = 1; const auto issued = field.next_body_id;
	const std::array last_event{AsteroidExitRecord{next->id, {next->size, next->resource_class}}};
	const auto credited = ApplyAsteroidExitCredit(field.renewal, last_event, issued); REQUIRE(credited);
	const auto due = AdvanceAsteroidRenewalTimer(*credited, issued); REQUIRE(due); const auto ticket = BeginAsteroidRenewalAttempt(*due, issued); REQUIRE(ticket); REQUIRE(ticket->size);
	const auto renewed = FinishAsteroidRenewalAttempt(*due, *ticket, true, issued); REQUIRE(renewed); REQUIRE(InstallAsteroidField(std::move(field)));
	RunSpaceLifecycleGameOpportunity(); CHECK(GetAsteroidField().renewal == *renewed); CHECK(GetAsteroidField().next_body_id == issued + 1);
	const auto *born = GetMutableAsteroidBody(OrbitalBodyID{issued}); REQUIRE(born); CHECK(born->size == *ticket->size); CHECK(born->motion.remainder_x == 0); CHECK(born->motion.remainder_y == 0);
	const auto insertion = *born;
	// Exercise the actual local fast-forward switch. Every completed ordinary
	// opportunity still consumes one saved tick, independent of camera/calendar
	// and the wall-clock frequency at which the video driver offers calls.
	AutoRestoreBackup speed(_game_speed); ChangeGameSpeed(true); CHECK(_game_speed != 100);
	RunSpaceLifecycleGameOpportunity(); CHECK(GetMutableAsteroidBody(insertion.id)->motion == StepSpaceMotionStraight(insertion.motion, bounds)->proposed);
	CHECK(GetAsteroidField().renewal == *AdvanceAsteroidRenewalTimer(*renewed, issued + 1)); ChangeGameSpeed(false);
	SetViewportLayer(GetMainWindow(), MapLayer::Space); const auto previous = GetAsteroidField().renewal; RunSpaceLifecycleGameOpportunity();
	CHECK(GetAsteroidField().renewal == *AdvanceAsteroidRenewalTimer(previous, issued + 1)); CHECK(GetAsteroidField().next_body_id == issued + 1); CHECK(ObserveLifecycleCargo() == cargo);
	const auto saved = (std::filesystem::path{output} / "lifecycle-current.sav").string();
	{
		AutoRestoreBackup paused(_pause_mode, PauseModes{PauseMode::Normal});
		REQUIRE(SaveOrLoad(saved, SaveLoadOperation::Save, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Ok);
		REQUIRE(SaveOrLoad(saved, SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Ok);
	}
	fmt::print("ASTEROID_LIFECYCLE ordinary_opportunities=4 hidden=true paused_frozen=true editor_frozen=true stopped_safe=true fast_forward_switch=true exits=3 births=1 original_packet_sources_quantities=true declared_geometry_deadline=true\n");
}
