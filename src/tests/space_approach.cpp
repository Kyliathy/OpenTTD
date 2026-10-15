/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file space_approach.cpp Shared slot contention, physical blockers and bounded saved-priority contracts.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs8-space-world-and-construction.md Group11.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group4B.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../space_approach.h"
#include "../space_vehicle.h"
#include "../space_vehicle_cmd.h"
#include "../space_structure.h"
#include "../space_structure_cmd.h"
#include "../space_manifest.h"
#include "../asteroid.h"
#include "../company_base.h"
#include "../command_func.h"
#include "../console_internal.h"
#include "../core/backup_type.hpp"
#include "../core/random_func.hpp"
#include "../openttd.h"
#include "../fios.h"
#include "../saveload/saveload_func.h"
#include "../timer/timer_game_tick.h"
#include "../map_func.h"
#include "layer_save_fixture.h"
#include "test_artifact_paths.h"
#include <cstdlib>
#include <thread>
#include "../safeguards.h"

/** @param id Explicit detached native hull ID. @param tick Saved request priority. @param target Physical target.
 * @return Waiting boundary data; this does not create or commission a gameplay hull.
 */
static SpaceApproachRecord ApproachRow(uint32_t id, uint64_t tick, SpaceOrderTarget target)
{
	return {VehicleID{id}, uint64_t{id} + 1, {target, tick, UINT8_MAX, {2 * SPACE_Q16, 2 * SPACE_Q16}}};
}

/** Exercise stable arbitration independently of input order, preserving physical occupants and existing reservations. */
TEST_CASE("Shared Space approaches preserve one owner and stable request priority", "[space-approach-contract]")
{
	const SpaceOrderTarget site{SpaceOrderTargetKind::Structure, 7, 10}, body{SpaceOrderTargetKind::Body, 7, 0};
	const std::array slots{SpaceApproachSlot{site, 0, {4 * SPACE_Q16, 5 * SPACE_Q16}}};
	const std::array simultaneous{ApproachRow(9, 30, site), ApproachRow(2, 30, site)};
	auto plan = PlanSpaceApproaches(simultaneous, slots); REQUIRE(plan);
	CHECK((*plan)[0].request.slot == UINT8_MAX); CHECK((*plan)[1].request.slot == 0);
	auto reversed = simultaneous; std::ranges::reverse(reversed);
	auto reordered = PlanSpaceApproaches(reversed, slots); REQUIRE(reordered);
	CHECK((*reordered)[0].request == (*plan)[1].request); CHECK((*reordered)[1].request == (*plan)[0].request);
	auto older = simultaneous; older[0].request.request_tick = 29;
	plan = PlanSpaceApproaches(older, slots); REQUIRE(plan); CHECK((*plan)[0].request.slot == 0); CHECK((*plan)[1].request.slot == UINT8_MAX);
	auto retained = simultaneous; retained[0].request.slot = 0;
	plan = PlanSpaceApproaches(retained, slots); REQUIRE(plan); CHECK((*plan)[0].request.slot == 0);
	auto occupied = slots; occupied[0].occupant = VehicleID{55};
	plan = PlanSpaceApproaches(simultaneous, occupied); REQUIRE(plan);
	CHECK((*plan)[0].request.slot == UINT8_MAX); CHECK((*plan)[1].request.slot == UINT8_MAX);
	occupied[0].occupant = VehicleID{9};
	plan = PlanSpaceApproaches(simultaneous, occupied); REQUIRE(plan);
	CHECK((*plan)[0].request.slot == 0); CHECK((*plan)[1].request.slot == UINT8_MAX);
	occupied[0].blocked = true; plan = PlanSpaceApproaches(simultaneous, occupied); REQUIRE(plan); CHECK((*plan)[0].request.slot == UINT8_MAX);
	const std::array two_slots{SpaceApproachSlot{site, 1, {5 * SPACE_Q16, 5 * SPACE_Q16}}, slots[0]};
	plan = PlanSpaceApproaches(simultaneous, two_slots); REQUIRE(plan); CHECK((*plan)[0].request.slot == 1); CHECK((*plan)[1].request.slot == 0);
	const std::array domains{simultaneous[1], ApproachRow(9, 30, body)};
	const std::array domain_slots{slots[0], SpaceApproachSlot{body, 0, {10 * SPACE_Q16, 20 * SPACE_Q16}}};
	plan = PlanSpaceApproaches(domains, domain_slots); REQUIRE(plan); CHECK((*plan)[0].request.slot == 0); CHECK((*plan)[1].request.slot == 0);
	plan = PlanSpaceApproaches(simultaneous, {}); REQUIRE(plan); CHECK((*plan)[0].request.slot == UINT8_MAX); CHECK((*plan)[1].request.slot == UINT8_MAX);
	CHECK_FALSE(PlanSpaceApproaches(std::array{simultaneous[0], simultaneous[0]}, slots));
	CHECK_FALSE(PlanSpaceApproaches(simultaneous, std::array{slots[0], slots[0]}));
	auto duplicate = retained; duplicate[1].request.slot = 0; CHECK_FALSE(PlanSpaceApproaches(duplicate, slots));
}

/** Reject malformed saved requests before native or preview installation, including duplicate grants and future priority. */
TEST_CASE("Shared Space approach transport rejects stale and ambiguous ownership", "[space-approach-contract]")
{
	SpaceOrderRecord row{VehicleID{2}, 3, {}};
	row.queue.approach = ApproachRow(2, 30, {SpaceOrderTargetKind::Structure, 7, 10}).request;
	CHECK(ValidateSpaceOrderRecords(std::array{row}, 4)); CHECK(ValidateSpaceApproachRecords(std::array{row}, 30, 64, 64));
	CHECK_FALSE(ValidateSpaceApproachRecords(std::array{row}, 29, 64, 64));
	auto invalid = row; invalid.queue.approach->target.generation = 0; CHECK_FALSE(ValidateSpaceOrderRecords(std::array{invalid}, 4));
	invalid = row; invalid.queue.approach->target.kind = SpaceOrderTargetKind::Craft; CHECK_FALSE(ValidateSpaceOrderRecords(std::array{invalid}, 4));
	invalid = row; invalid.queue.approach->staging.x = -1; CHECK_FALSE(ValidateSpaceOrderRecords(std::array{invalid}, 4));
	invalid = row; invalid.queue.approach->staging.y = 64 * SPACE_Q16; CHECK_FALSE(ValidateSpaceApproachRecords(std::array{invalid}, 30, 64, 64));
	invalid = row; invalid.queue.approach->slot = 2; CHECK_FALSE(ValidateSpaceOrderRecords(std::array{invalid}, 4));
	auto second = row; second.vehicle = VehicleID{4}; second.generation = 5;
	row.queue.approach->slot = 0; second.queue.approach->slot = 0;
	CHECK_FALSE(ValidateSpaceOrderRecords(std::array{row, second}, 6));
	second.queue.approach->slot = UINT8_MAX; CHECK(ValidateSpaceOrderRecords(std::array{row, second}, 6));
	invalid = row; invalid.queue.approach->target = {SpaceOrderTargetKind::Body, 7, 0}; invalid.queue.approach->slot = 1;
	CHECK_FALSE(ValidateSpaceOrderRecords(std::array{invalid}, 4));
}

static std::function<void()> *approach_observer = nullptr;
/** @param args Console invocation. @return Whether the ordinary loaded-world observer ran. */
static bool ObserveApproaches(std::span<std::string_view> args)
{
	REQUIRE(args.size() == 1); REQUIRE(approach_observer != nullptr); (*approach_observer)(); return true;
}

/** Exercise actual paid native hulls through simultaneous requests, queued interruption, departure and target cancellation. */
TEST_CASE("Paid native craft queue for one physical approach and resume exactly", "[.space-approaches]")
{
	const char *input = std::getenv("OTTD_SPACE_APPROACH_SOURCE"), *mode = std::getenv("OTTD_SPACE_APPROACH_ACTION");
	REQUIRE(input != nullptr); REQUIRE(mode != nullptr); const std::string_view action{mode};
	const auto paths = GetTestCaseArtifactPaths("space", "approaches"); const auto config = GetTestRuntimeConfig();
	bool observed = false; std::exception_ptr failure;
	/** Run only inside the ordinary native game, propagating assertions after clean shutdown. */
	std::function<void()> observe = [&] {
		try {
			AutoRestoreBackup actor(_current_company, _local_company);
			REQUIRE(ValidateSpaceVehicles());
			if (const char *corrupt = std::getenv("OTTD_SPACE_APPROACH_CORRUPT")) {
				const auto orders = ComputeSpaceOrdersDigest(), structures = ComputeSpaceStructureDigest(), utility = ComputeUtilityPowerDigest();
				const auto money = Company::Get(_current_company)->money; const auto random = _random;
				CHECK(SaveOrLoad(corrupt, SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Error);
				CHECK(ComputeSpaceOrdersDigest() == orders); CHECK(ComputeSpaceStructureDigest() == structures); CHECK(ComputeUtilityPowerDigest() == utility);
				CHECK(Company::Get(_current_company)->money == money); CHECK(_random.state[0] == random.state[0]); CHECK(_random.state[1] == random.state[1]);
				fmt::print("SPACE_APPROACH native=true preview_rejected=true authority_unchanged=true\n"); observed = true; _exit_game = true; return;
			}
			std::vector<SpaceVehicle *> craft;
			// Select real paid deployed hulls in native ID order; no test hull, cargo or position is injected into the loaded world.
			for (auto *v : SpaceVehicle::Iterate()) {
				if (v->owner == _current_company && v->construction.placement == SpacePlacement::Orbital && v->construction.content_version == MULTILAYER_SPACE_CONTENT_VERSION &&
					(GetSpaceVehicleModel(*v) == 15 || GetSpaceVehicleModel(*v) == 20) && v->manifest && v->manifest->service_ticks == 0) craft.push_back(v);
			}
			REQUIRE(craft.size() >= 2); craft.resize(2);
			auto &core = *SpaceStructure::Get(SpaceStructureID{0}); auto &storage = *SpaceStructure::Get(SpaceStructureID{1});
			/** Advance actual simulation opportunities; rendering and timer waits cannot advance reservation priority.
			 * @param ticks Native synchronized ticks to observe.
			 */
			auto advance = [&](uint ticks) {
				AutoRestoreBackup pause(_pause_mode, PauseModes{}); const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(90);
				for (uint done = 0; done < ticks;) {
					REQUIRE(std::chrono::steady_clock::now() < deadline); const auto before = GetAsteroidField().elapsed_ticks; StateGameLoop();
					if (GetAsteroidField().elapsed_ticks == before) { std::this_thread::sleep_for(std::chrono::milliseconds(1)); continue; }
					REQUIRE(GetAsteroidField().elapsed_ticks == before + 1); ++done;
				}
			};
			/** @param name Native checkpoint basename within this invocation's repository evidence home. */
			auto save = [&](std::string_view name) {
				REQUIRE(ValidateSpaceVehicles()); AutoRestoreBackup pause(_pause_mode, PauseModes{PauseMode::Normal});
				AutoRestoreBackup compression(_savegame_format, std::string{"lzma"});
				REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(paths.saves / std::string{name}), false) == SaveLoadResult::Ok);
			};
			/** @param hull Actual paid hull. @param site Actual owned physical target whose approach is requested. */
			auto visit = [&](SpaceVehicle &hull, const SpaceStructure &site) {
				REQUIRE(Command<Commands::EditSpaceOrder>::Post(hull.index, hull.generation, SpaceOrderEdit::Clear, uint8_t{0}, SpaceOrderAction::Visit, SpaceOrderTargetKind::Body, uint32_t{0}, uint64_t{0}));
				REQUIRE(Command<Commands::EditSpaceOrder>::Post(hull.index, hull.generation, SpaceOrderEdit::Insert, uint8_t{0}, SpaceOrderAction::Visit, SpaceOrderTargetKind::Structure, uint32_t{site.index.base()}, site.generation));
			};
			if (action == "band-check") {
				const auto structures = ComputeSpaceStructureDigest(), orders = ComputeSpaceOrdersDigest();
				const auto field = GetAsteroidField(); const auto random = _random;
				const auto money = Company::Get(_current_company)->money;
				const uint32_t width = Map::SizeX(), height = Map::SizeY();
				// A free north-site quote proves company/date/capacity admission, so the following refusals specifically exercise strip geometry.
				CHECK(CmdBuildSpaceBlueprint({}, TileXYL(width / 2, 4, MapLayer::Space), SpaceBuildKind::Core).Succeeded());
				// The real native placement command must reject the body strip and a north approach entering it, without buying or reserving anything.
				for (uint32_t y : {height / 4, 3 * height / 4 - 1, height / 4 - 3}) {
					CHECK(CmdBuildSpaceBlueprint({}, TileXYL(width / 2, y, MapLayer::Space), SpaceBuildKind::Core).Failed());
				}
				CHECK(ComputeSpaceStructureDigest() == structures); CHECK(ComputeSpaceOrdersDigest() == orders);
				CHECK(GetAsteroidField() == field); CHECK(Company::Get(_current_company)->money == money);
				CHECK(_random.state[0] == random.state[0]); CHECK(_random.state[1] == random.state[1]);
				save("approach-complete-422.sav");
				fmt::print("SPACE_APPROACH native=true band_policy=true estimate_pure=true\n");
				observed = true; _exit_game = true; return;
			}
			if (action == "body-prepare" || action == "body-continue") {
				AsteroidBody *body = nullptr;
				if (action == "body-prepare") {
					// Choose an actual unused natural body; a test may not mint a body, clear somebody else's mining lease or replace its reserve.
					for (const auto &candidate : GetAsteroidField().bodies) {
						if (candidate.mining_slot != VehicleID::Invalid() || candidate.survey_slot != INVALID_SPACE_SURVEY) continue;
						bool used = false;
						for (const auto *v : SpaceVehicle::Iterate()) {
							if (v->mission.target == candidate.id) used = true;
							const auto &q = v->space_orders;
							if (q.cursor < q.entries.size() && q.entries[q.cursor].target.kind == SpaceOrderTargetKind::Body && q.entries[q.cursor].target.id == candidate.id.base()) used = true;
						}
						if (!used) { body = GetMutableAsteroidBody(candidate.id); break; }
					}
					REQUIRE(body != nullptr);
					// Both genuine paid builders can visit a body but cannot acquire mining authority from a navigation reservation.
					for (auto *v : craft) {
						REQUIRE(Command<Commands::EditSpaceOrder>::Post(v->index, v->generation, SpaceOrderEdit::Clear, uint8_t{0}, SpaceOrderAction::Visit, SpaceOrderTargetKind::Body, uint32_t{0}, uint64_t{0}));
						REQUIRE(Command<Commands::EditSpaceOrder>::Post(v->index, v->generation, SpaceOrderEdit::Insert, uint8_t{0}, SpaceOrderAction::Visit, SpaceOrderTargetKind::Body, body->id.base(), uint64_t{0}));
					}
					advance(2); save("approach-body-queued-422.sav");
				} else {
					REQUIRE(craft[0]->space_orders.approach); REQUIRE(craft[0]->space_orders.approach->target.kind == SpaceOrderTargetKind::Body);
					body = GetMutableAsteroidBody(OrbitalBodyID{craft[0]->space_orders.approach->target.id});
				}
				REQUIRE(body != nullptr); REQUIRE(craft[0]->space_orders.approach); REQUIRE(craft[1]->space_orders.approach);
				CHECK(craft[0]->space_orders.approach->slot == 0); CHECK(craft[1]->space_orders.approach->slot == UINT8_MAX);
				const auto reserve = body->remaining_reserve; const auto priority = craft[1]->space_orders.approach->request_tick;
				const auto before = body->motion.position;
				advance(37); CHECK(body->motion.position != before); CHECK(body->remaining_reserve == reserve);
				CHECK(body->mining_slot == VehicleID::Invalid()); CHECK(body->survey_slot == INVALID_SPACE_SURVEY);
				CHECK(craft[1]->space_orders.approach->request_tick == priority);
				SpacePoint destination;
				REQUIRE(GetSpaceApproachDestination(*craft[0], destination) == SpaceOrderBlock::None);
				CHECK((destination == SpacePoint{body->motion.position.x + SPACE_Q16, body->motion.position.y - SPACE_Q16 / 2}));
				// Cancel each native request through synchronized queue edits, preserving the moving body's physical lease/reserve owners.
				for (auto *v : craft) {
					REQUIRE(Command<Commands::EditSpaceOrder>::Post(v->index, v->generation, SpaceOrderEdit::Clear, uint8_t{0}, SpaceOrderAction::Visit, SpaceOrderTargetKind::Body, uint32_t{0}, uint64_t{0}));
					CHECK_FALSE(v->space_orders.approach);
				}
				save("approach-body-complete-422.sav");
				fmt::print("SPACE_APPROACH native=true body_contention=true body_motion=true no_mining_lease=true\n");
				observed = true; _exit_game = true; return;
			}
			if (action == "prepare") {
				const auto orders = ComputeSpaceOrdersDigest(); const auto money = Company::Get(_current_company)->money;
				CHECK(RequestSpaceApproach(*craft[0], {SpaceOrderTargetKind::Structure, core.index.base(), core.generation}, false));
				CHECK(ComputeSpaceOrdersDigest() == orders); CHECK(Company::Get(_current_company)->money == money);
				// The paid predecessor retains a builder at Core after service. Vacate it through real movement before testing simultaneous free-slot priority.
				for (auto *v : SpaceVehicle::Iterate()) {
					if (v->owner == _current_company && v->construction.placement == SpacePlacement::Orbital && v->position == GetSpaceStructureApproach(core.state)) visit(*v, storage);
				}
				advance(2);
				for (const auto *v : SpaceVehicle::Iterate()) CHECK(v->position != GetSpaceStructureApproach(core.state));
				visit(*craft[1], core); visit(*craft[0], core); // Reverse request submission; native-ID tie breaking remains authoritative.
				advance(2);
				REQUIRE(craft[0]->space_orders.approach); REQUIRE(craft[1]->space_orders.approach);
				CHECK(craft[0]->space_orders.approach->request_tick == craft[1]->space_orders.approach->request_tick);
				CHECK(craft[0]->space_orders.approach->slot == 0); CHECK(craft[1]->space_orders.approach->slot == UINT8_MAX);
				save("approach-queued-422.sav");
			} else REQUIRE(action == "continue");
			REQUIRE(craft[0]->space_orders.approach); REQUIRE(craft[1]->space_orders.approach);
			const auto priority = craft[1]->space_orders.approach->request_tick;
			// The winner actually reaches and retains the one physical berth; a waiting hull never pushes it aside.
			for (uint elapsed = 0; !GetSpaceStructureBerth(*craft[0], core); ++elapsed) { REQUIRE(elapsed < 2000); advance(1); }
			CHECK_FALSE(GetSpaceStructureBerth(*craft[1], core)); CHECK(craft[1]->space_orders.approach->request_tick == priority);
			visit(*craft[0], storage);
			// Departure, not mere cancellation of a reservation, frees physical occupancy for the next queued request.
			for (uint elapsed = 0; !GetSpaceStructureBerth(*craft[1], core); ++elapsed) { REQUIRE(elapsed < 2000); advance(1); }
			CHECK(craft[1]->space_orders.approach->request_tick == priority);
			SpaceStructure *cancelled = nullptr;
			const auto &definition = *GetSpaceConstructionDefinition(SpaceBuildKind::Storage);
			// Find a legal adjacent paid blueprint through native command estimates; this bounded local search grants no geometry bypass.
			for (int dy = -int{definition.height}; dy <= 3 && cancelled == nullptr; ++dy) {
				for (int dx = -int{definition.width}; dx <= 3; ++dx) {
					const int x = int{core.state.x} + dx, y = int{core.state.y} + dy;
					if (x < 0 || y < 0) continue;
					const auto tile = TileXYL(x, y, MapLayer::Space);
					if (CmdBuildSpaceBlueprint({}, tile, SpaceBuildKind::Storage).Failed()) continue;
					REQUIRE(Command<Commands::BuildSpaceBlueprint>::Post(tile, SpaceBuildKind::Storage));
					for (auto *site : SpaceStructure::Iterate()) if (site->state.x == x && site->state.y == y) cancelled = site;
					break;
				}
			}
			REQUIRE(cancelled != nullptr); visit(*craft[0], *cancelled); visit(*craft[1], *cancelled); advance(2);
			REQUIRE(Command<Commands::CancelSpaceBlueprint>::Post(cancelled->index, cancelled->generation)); advance(1);
			CHECK_FALSE(craft[0]->space_orders.approach); CHECK_FALSE(craft[1]->space_orders.approach);
			CHECK(craft[0]->space_orders.blocked == SpaceOrderBlock::TargetRemoved); CHECK(craft[1]->space_orders.blocked == SpaceOrderBlock::TargetRemoved);
			save("approach-complete-422.sav");
			fmt::print("SPACE_APPROACH native=true queued_reload=true physical_contention=true target_loss=true\n");
		} catch (...) { failure = std::current_exception(); }
		observed = true; _exit_game = true;
	};
	AutoRestoreBackup hook(approach_observer, &observe); IConsole::CmdRegister("space_approach_test", ObserveApproaches);
	const std::string source{input}, configuration = config.string();
	std::array<std::string_view, 17> args = {"openttd_test.exe", "-v", "null:ticks=2", "-b", "null", "-s", "null", "-m", "null", "-x", "-X", "-I", "original_dos", "-c", configuration, "-g", source};
	CHECK(openttd_main(args) == 0); if (failure) std::rethrow_exception(failure); CHECK(observed);
}
