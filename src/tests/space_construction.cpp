/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file space_construction.cpp First orbital material ownership, paid delivery and interrupted construction.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Groups1/1B, task1.4.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../space_structure.h"
#include "../space_structure_cmd.h"
#include "../space_vehicle.h"
#include "../space_vehicle_cmd.h"
#include "../space_manifest.h"
#include "../space_transition.h"
#include "../launch_center_cmd.h"
#include "../asteroid.h"
#include "../company_base.h"
#include "../company_func.h"
#include "../company_manager_face.h"
#include "../economy_func.h"
#include "../station_base.h"
#include "../command_func.h"
#include "../console_internal.h"
#include "../core/backup_type.hpp"
#include "../core/random_func.hpp"
#include "../openttd.h"
#include "../fios.h"
#include "../saveload/saveload_func.h"
#include "../blitter/factory.hpp"
#include "../spritecache.h"
#include "../fontcache.h"
#include "../window_func.h"
#include "../window_gui.h"
#include "layer_save_fixture.h"
#include "test_artifact_paths.h"
#include "native_art_render_fixture.h"
#include <cstdlib>
#include <thread>
#include "../safeguards.h"

TEST_CASE("Orbital static sites reserve footprint and approach outside the body strip", "[space-construction-contract]")
{
	SpaceStructureState a; a.x = 4; a.y = 12;
	CHECK(IsSpaceStructureSiteValid(a, 64, 64)); a.y = 13; CHECK_FALSE(IsSpaceStructureSiteValid(a, 64, 64));
	a.y = 47; CHECK_FALSE(IsSpaceStructureSiteValid(a, 64, 64)); a.y = 48; CHECK(IsSpaceStructureSiteValid(a, 64, 64));
	a.y = 60; CHECK(IsSpaceStructureSiteValid(a, 64, 64)); a.y = 61; CHECK_FALSE(IsSpaceStructureSiteValid(a, 64, 64));
	a.y = 4; a.x = 61; CHECK(IsSpaceStructureSiteValid(a, 64, 64)); a.x = 62; CHECK_FALSE(IsSpaceStructureSiteValid(a, 64, 64));
	a.x = 4; auto b = a; b.x = 7; CHECK_FALSE(SpaceStructureSitesOverlap(a, b)); b.x = 6; CHECK(SpaceStructureSitesOverlap(a, b));
	b = a; b.y = 7; CHECK(SpaceStructureSitesOverlap(a, b)); b.x = 5; CHECK_FALSE(SpaceStructureSitesOverlap(a, b));
	CHECK_FALSE(IsSpaceStructureSiteValid(a, 65, 64)); CHECK_FALSE(IsSpaceStructureSiteValid(a, 64, 8192));
	for (uint size : {64U, 128U, 256U, 512U, 1024U, 2048U, 4096U}) { a.y = size / 4 - 4; CHECK(IsSpaceStructureSiteValid(a, size, size)); ++a.y; CHECK_FALSE(IsSpaceStructureSiteValid(a, size, size)); }
}
TEST_CASE("Construction receipts bind exact recipe inputs and never activate a frame", "[space-construction-contract]")
{
	SpaceStructureRecord r; r.ref = {SpaceStructureID{0}, 1}; r.state.owner = Owner{0}; r.state.x = 4; r.state.y = 4; r.state.capital_paid = 700000;
	/** @param row Detached paid receipt. @return Whether its exact material/identity bounds are accepted without native mutation. */
	auto valid = [&](const auto &row) { return ValidateSpaceStructureRecords(std::array{row}, 2, 64, 64); };
	CHECK(valid(r)); auto bad = r; bad.ref.generation = 2; CHECK_FALSE(valid(bad));
	bad = r; bad.state.capital_paid = 0; CHECK_FALSE(valid(bad)); bad = r; bad.work_ticks = 1; CHECK_FALSE(valid(bad));
	r.quantities = {16, 24}; r.inputs[0] = {{CargoPacketID{1}, StationID::Invalid()}}; r.inputs[1] = {{CargoPacketID{2}, StationID::Invalid()}}; r.work_ticks = 37;
	CHECK(valid(r)); bad = r; bad.inputs[1] = bad.inputs[0]; CHECK_FALSE(valid(bad)); bad = r; bad.quantities[1] = 23; CHECK_FALSE(valid(bad));
	bad = r; bad.consumed[0] = 16; CHECK_FALSE(valid(bad)); bad = r; bad.work_ticks = 740; CHECK_FALSE(valid(bad));
	r.state.phase = SpaceStructurePhase::Cancelled; r.quantities[1] = 8; CHECK(valid(r)); r.quantities[1] = 0; r.inputs[1].clear(); CHECK(valid(r));
	r.state.phase = SpaceStructurePhase::FrameComplete; r.work_ticks = 740; r.quantities = {}; r.inputs = {}; r.consumed = {16, 24}; CHECK(valid(r));
	bad = r; bad.consumed[1] = 23; CHECK_FALSE(valid(bad)); bad = r; bad.age_ticks = Ticks::CARGO_AGING_TICKS; CHECK_FALSE(valid(bad));
	SpaceOrderRecord order{VehicleID{1}, 1, {{SpaceOrder{SpaceOrderAction::Construct, {SpaceOrderTargetKind::Blueprint, 0, 1}}}}};
	order.queue.started = true; order.queue.timer_ticks = 1480; order.queue.blocked = SpaceOrderBlock::MaterialsMissing;
	CHECK(ValidateSpaceOrderRecords(std::array{order}, 2)); order.queue.entries[0].action = SpaceOrderAction::Visit; CHECK_FALSE(ValidateSpaceOrderRecords(std::array{order}, 2));
	LoadCheckData data; data.space_structure_rows.push_back(r); data.space_structures_present = true; data.next_space_structure_generation = 2; data.Clear();
	CHECK(data.space_structure_rows.empty()); CHECK_FALSE(data.space_structures_present); CHECK(data.next_space_structure_generation == 1);
}

static std::function<void()> *observer = nullptr;
/** @param args Native console invocation. @return Whether the installed construction observer ran. */
static bool Observe(std::span<std::string_view> args) { REQUIRE(args.size() == 1); REQUIRE(observer != nullptr); (*observer)(); return true; }
TEST_CASE("Earth builders deliver and retain the first orbital construction job", "[.space-construction]")
{
	const char *mode = std::getenv("OTTD_SPACE_CONSTRUCTION_ACTION"), *input = std::getenv("OTTD_SPACE_CONSTRUCTION_SOURCE"); REQUIRE(mode != nullptr); REQUIRE(input != nullptr);
	const std::string_view action{mode}; const std::filesystem::path source{input}; const auto paths = GetTestCaseArtifactPaths("space", "construction"); const auto config = GetTestRuntimeConfig();
	bool observed = false; std::exception_ptr failure;
	/** Execute only in the real loaded world; propagate observer failures through normal native shutdown. */
	std::function<void()> observe = [&] {
		try {
			AutoRestoreBackup actor(_current_company, _local_company); REQUIRE(ValidateSpaceVehicles());
			auto *company = Company::Get(_current_company);
			SpaceVehicle *rocket = nullptr, *builder = nullptr;
			// Resolve paid native rocket/builders from the immutable predecessor instead of synthesizing orbital materials or hulls.
			for (auto *v : SpaceVehicle::Iterate()) { if (GetSpaceVehicleModel(*v) == 14) rocket = v; if (GetSpaceVehicleModel(*v) == 20 && builder == nullptr) builder = v; }
			REQUIRE((rocket && builder)); const auto center_ref = rocket->construction.home; auto *center = ResolveLaunchCenter(center_ref); REQUIRE(center != nullptr);
			const auto steel = GetCargoTypeByLabel(CargoLabel{"STEL"}), goods = GetCargoTypeByLabel(CargoLabel{"GOOD"});
			/** @param ticks Real synchronized economy opportunities; render/fast-forward yields earn no work. */
			auto advance = [&](uint ticks) {
				AutoRestoreBackup pause(_pause_mode, PauseModes{}); const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(90);
				// Count authoritative controller advancement rather than loop iterations, with a bounded failure deadline.
				for (uint i = 0; i < ticks;) { REQUIRE(std::chrono::steady_clock::now() < deadline); const auto before = GetAsteroidField().elapsed_ticks; StateGameLoop();
					if (GetAsteroidField().elapsed_ticks == before) { std::this_thread::sleep_for(std::chrono::milliseconds(1)); continue; } REQUIRE(GetAsteroidField().elapsed_ticks == before + 1); ++i; }
			};
			/** @param done Actual native transition predicate; bounded waiting never advances a separate travel/work controller. */
			auto until = [&](auto done) { for (uint i = 0; !done(); ++i) { REQUIRE(i < 20000); advance(1); } };
			/** @param name Isolated native checkpoint filename, saved while paused after owner validation. */
			auto save = [&](std::string_view name) { REQUIRE(ValidateSpaceVehicles()); AutoRestoreBackup pause(_pause_mode, PauseModes{PauseMode::Normal}); AutoRestoreBackup compression(_savegame_format, std::string{"lzma"}); REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(paths.saves / std::string{name}), false) == SaveLoadResult::Ok); };
			/** @param v Actual paid builder. @param s Exact native blueprint incarnation to receive its physical cargo. */
			auto order = [&](SpaceVehicle &v, const SpaceStructure &s) { REQUIRE(Command<Commands::EditSpaceOrder>::Post(v.index, v.generation, SpaceOrderEdit::Insert, static_cast<uint8_t>(v.space_orders.entries.size()), SpaceOrderAction::Construct, SpaceOrderTargetKind::Blueprint, uint32_t{s.index.base()}, s.generation)); };
			REQUIRE(BlitterFactory::SelectBlitter("8bpp-simple") != nullptr); GfxClearSpriteCache(); FontCache::ClearFontCaches(FONTSIZES_REQUIRED);
			/** @param name Actual software-rendered native window capture filename; no replacement artwork is generated. */
			auto capture = [&](std::string_view name) { ShowSpaceConstructionWindow(); auto *w = FindWindowById(WindowClass::SpaceConstruction, 0); REQUIRE(w != nullptr); CaptureNativeArtWindow(*w, paths.images / std::string{name}); };
			if (const char *corrupt = std::getenv("OTTD_SPACE_CONSTRUCTION_CORRUPT")) {
				const auto digest = ComputeSpaceStructureDigest(); REQUIRE(SaveOrLoad(source.string(), SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Ok);
				CHECK(SaveOrLoad(corrupt, SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Error);
				CHECK(ComputeSpaceStructureDigest() == digest); fmt::print("SPACE_CONSTRUCTION preview_reject=true authority_unchanged=true\n"); observed = true; _exit_game = true; return;
			}
			if (action == "prepare") {
				REQUIRE(SpaceStructure::GetNumItems() == 0); const auto tile = TileXYL(4, 4, MapLayer::Space);
				CHECK(CmdBuildSpaceBlueprint({}, tile).Failed()); // Retained bootstrap is1990, before Core introduction.
				TimerGameCalendar::SetDate(TimerGameCalendar::ConvertYMDToDate(TimerGameCalendar::Year{1995}, 0, 1), 0);
				TimerGameEconomy::SetDate(TimerGameEconomy::ConvertYMDToDate(TimerGameEconomy::Year{1995}, 0, 1), 0);
				const auto money = company->money; const auto digest = ComputeSpaceStructureDigest(); const auto random = _random;
				CHECK(CmdBuildSpaceBlueprint({}, tile).Succeeded()); CHECK(CmdBuildSpaceBlueprint({}, TileXYL(4, 4, MapLayer::Surface)).Failed()); CHECK(CmdBuildSpaceBlueprint({}, TileXYL(4, Map::SizeY() / 4, MapLayer::Space)).Failed());
				CHECK(company->money == money); CHECK(ComputeSpaceStructureDigest() == digest); CHECK(_random.state[0] == random.state[0]); CHECK(_random.state[1] == random.state[1]);
				ShowSpaceConstructionWindow(); FindWindowById(WindowClass::SpaceConstruction, 0)->OnPlaceObject({}, tile);
				REQUIRE(SpaceStructure::GetNumItems() == 1); CHECK(company->money == money - Money{700000}); CHECK(CmdBuildSpaceBlueprint({}, tile).Failed());
				auto &s = *SpaceStructure::Get(SpaceStructureID{0}); CHECK(s.job.inputs[0].TotalCount() == 0); CHECK(s.job.inputs[1].TotalCount() == 0);
				REQUIRE(Command<Commands::MoveLaunchFreight>::Post(center_ref.station, center_ref.generation, builder->index, builder->generation, goods, uint16_t{16}, true));
				REQUIRE(Command<Commands::CarryLaunchProbe>::Post(rocket->index, rocket->generation, builder->index, builder->generation, false));
				REQUIRE(Command<Commands::BeginLaunchFlight>::Post(rocket->index, rocket->generation, SpaceOrderTargetKind::LaunchCenter, uint32_t{center_ref.station.base()}, center_ref.generation)); advance(667);
				order(*builder, s); until([&] { return builder->space_orders.blocked == SpaceOrderBlock::MaterialsMissing; });
				CHECK(s.job.inputs[1].TotalCount() == 24); CHECK(builder->cargo.TotalCount() == 0); CHECK(s.job.work_ticks == 0); advance(74); CHECK(s.job.work_ticks == 0);
				save("construction-delivery-415.sav"); capture("construction-missing.ppm");
			}
			auto &site = *SpaceStructure::Get(SpaceStructureID{0});
			// These two focused branches exercise real incomplete/returned material, separately from the later complete-frame path.
			if (action == "missing-goods" || action == "missing-steel") {
				REQUIRE(site.state.phase == SpaceStructurePhase::Blueprint); REQUIRE(site.module == nullptr);
				if (action == "missing-steel") {
					CHECK(site.job.inputs[0].TotalCount() == 0); CHECK(site.job.inputs[1].TotalCount() == 24);
					advance(74); CHECK(site.job.work_ticks == 0); CHECK(CmdActivateOrbitalCore({}, site.index, site.generation).Failed()); save("construction-missing-steel-418.sav");
					REQUIRE(Command<Commands::CancelSpaceBlueprint>::Post(site.index, site.generation));
					REQUIRE(Command<Commands::EditSpaceOrder>::Post(builder->index, builder->generation, SpaceOrderEdit::Clear, uint8_t{0}, SpaceOrderAction::Visit, SpaceOrderTargetKind::Body, uint32_t{0}, uint64_t{0}));
					REQUIRE(Command<Commands::CollectSpaceConstructionInputs>::Post(site.index, site.generation, builder->index, builder->generation, goods, uint16_t{24}));
					CHECK(builder->cargo.TotalCount() == 24); CHECK(site.job.inputs[1].TotalCount() == 0); CHECK((site.job.consumed == std::array<uint16_t, 2>{0, 0}));
				} else {
					REQUIRE(site.job.work_ticks == 37); SpaceVehicle *second = nullptr;
					// The genuine partial-work predecessor has two paid builders at this one precise delivery approach.
					for (auto *v : SpaceVehicle::Iterate()) if (GetSpaceVehicleModel(*v) == 20 && v != builder) second = v;
					REQUIRE(second != nullptr); REQUIRE(Command<Commands::CancelSpaceBlueprint>::Post(site.index, site.generation));
					for (auto *v : {builder, second}) REQUIRE(Command<Commands::EditSpaceOrder>::Post(v->index, v->generation, SpaceOrderEdit::Clear, uint8_t{0}, SpaceOrderAction::Visit, SpaceOrderTargetKind::Body, uint32_t{0}, uint64_t{0}));
					const auto cash = company->money;
					REQUIRE(Command<Commands::CollectSpaceConstructionInputs>::Post(site.index, site.generation, builder->index, builder->generation, steel, uint16_t{16}));
					REQUIRE(Command<Commands::CollectSpaceConstructionInputs>::Post(site.index, site.generation, second->index, second->generation, goods, uint16_t{24}));
					CHECK(company->money == cash); CHECK((site.job.consumed == std::array<uint16_t, 2>{0, 0}));
					REQUIRE(Command<Commands::BuildSpaceBlueprint>::Post(TileXYL(4, 4, MapLayer::Space), SpaceBuildKind::Core)); CHECK(company->money == cash - Money{700000});
					auto &next = *SpaceStructure::Get(SpaceStructureID{1}); order(*builder, next); until([&] { return next.job.inputs[0].TotalCount() == 16; });
					advance(74); CHECK(next.job.inputs[1].TotalCount() == 0); CHECK(next.job.work_ticks == 0); CHECK(next.module == nullptr); CHECK(second->cargo.TotalCount() == 24);
					CHECK(CmdActivateOrbitalCore({}, next.index, next.generation).Failed()); save("construction-missing-goods-418.sav");
					REQUIRE(Command<Commands::CancelSpaceBlueprint>::Post(next.index, next.generation));
					REQUIRE(Command<Commands::CollectSpaceConstructionInputs>::Post(next.index, next.generation, builder->index, builder->generation, steel, uint16_t{16}));
					CHECK(builder->cargo.TotalCount() == 16); CHECK(second->cargo.TotalCount() == 24); CHECK((next.job.consumed == std::array<uint16_t, 2>{0, 0}));
				}
				save("construction-boundary-collected-418.sav"); fmt::print("SPACE_CONSTRUCTION native=true missing_material=true cancelled_unconsumed_balanced=true no_free_activation=true\n"); observed = true; _exit_game = true; return;
			}
			if (action == "ownership") {
				const auto original_owner = site.state.owner; const auto before = CaptureSpaceStructures()[0];
				REQUIRE(Company::CanAllocateItem()); auto *other = Company::Create(); SetCompanyManagerFaceStyle(other->face, 0); other->money = 10000000; const auto other_id = other->index;
				{ AutoRestoreBackup foreign(_current_company, other_id); CHECK(CmdCancelSpaceBlueprint({}, site.index, site.generation).Failed()); }
				ChangeOwnershipOfCompanyItems(original_owner, other_id); CHECK(site.state.owner == other_id); CHECK(builder->owner == other_id); REQUIRE(ValidateSpaceVehicles());
				CHECK(CaptureSpaceStructures()[0].inputs == before.inputs); delete Company::Get(original_owner); REQUIRE(Company::CanAllocateItem()); auto *reused = Company::Create(); SetCompanyManagerFaceStyle(reused->face, 0); reused->money = 10000000; CHECK(reused->index == original_owner);
				{ AutoRestoreBackup local_actor(_current_company, _local_company); SetLocalCompany(reused->index); }
				CHECK(CmdCancelSpaceBlueprint({}, site.index, site.generation).Failed());
				ChangeOwnershipOfCompanyItems(other_id, INVALID_OWNER); delete Company::Get(other_id);
				CHECK(site.state.owner == OWNER_NONE); CHECK(CaptureSpaceStructures()[0].inputs == before.inputs); CHECK(site.job.inputs[1].TotalCount() == 24); CHECK(site.job.work_ticks == 0);
				save("construction-unowned-415.sav"); fmt::print("SPACE_CONSTRUCTION native=true takeover=true company_reuse=true removed_company_packets_retained=true\n"); observed = true; _exit_game = true; return;
			}
			if (action == "cancel" || action == "cancel-continue") {
				if (action == "cancel") {
					const auto digest = ComputeSpaceStructureDigest(); CHECK(CmdCancelSpaceBlueprint({}, site.index, site.generation + 1).Failed()); CHECK(ComputeSpaceStructureDigest() == digest);
					REQUIRE(Command<Commands::CancelSpaceBlueprint>::Post(site.index, site.generation)); advance(1); CHECK(builder->space_orders.blocked == SpaceOrderBlock::TargetRemoved);
					CHECK(CmdBuildSpaceBlueprint({}, TileXYL(4, 4, MapLayer::Space)).Failed());
					capture("construction-cancelled.ppm"); auto *w = FindWindowById(WindowClass::SpaceConstruction, 0); REQUIRE(w != nullptr); w->OnClick({}, 7, 1);
					REQUIRE(Command<Commands::EditSpaceOrder>::Post(builder->index, builder->generation, SpaceOrderEdit::Insert, uint8_t{0}, SpaceOrderAction::Visit, SpaceOrderTargetKind::Body, uint32_t{0}, uint64_t{0}));
					until([&] { return builder->space_orders.cursor == builder->space_orders.entries.size(); }); REQUIRE(builder->position != GetSpaceStructureApproach(site.state));
					w->OnClick({}, 7, 1); w->OnClick({}, 9, 1); advance(1); save("construction-cancelled-415.sav");
				}
				REQUIRE(site.state.phase == SpaceStructurePhase::Cancelled); CHECK(site.job.inputs[1].TotalCount() == 24);
				until([&] { return builder->space_orders.cursor == builder->space_orders.entries.size(); }); REQUIRE(builder->position == GetSpaceStructureApproach(site.state));
				if (site.job.inputs[0].TotalCount() != 0) {
					SpaceVehicle *second = nullptr; for (auto *v : SpaceVehicle::Iterate()) if (GetSpaceVehicleModel(*v) == 20 && v != builder) second = v; REQUIRE(second != nullptr);
					REQUIRE(Command<Commands::CollectSpaceConstructionInputs>::Post(site.index, site.generation, second->index, second->generation, steel, uint16_t{16})); CHECK(second->cargo.TotalCount() == 16); CHECK(site.job.consumed[0] == 0);
				}
				REQUIRE(Command<Commands::CollectSpaceConstructionInputs>::Post(site.index, site.generation, builder->index, builder->generation, goods, uint16_t{24}));
				CHECK(builder->cargo.TotalCount() == 24); CHECK(site.job.inputs[1].TotalCount() == 0); CHECK(site.job.consumed[1] == 0); CHECK(CmdBuildSpaceBlueprint({}, TileXYL(4, 4, MapLayer::Space)).Succeeded());
				save("construction-cancel-complete-415.sav"); capture("construction-collected.ppm"); fmt::print("SPACE_CONSTRUCTION native=true cancellation=true conserved=true\n"); observed = true; _exit_game = true; return;
			}
			if (action != "continue") {
				const auto old_count = GetSpaceVehicleCount(); REQUIRE(Command<Commands::AssembleLaunchCraft>::Post(center_ref.station, center_ref.generation, uint16_t{20})); advance(148); REQUIRE(GetSpaceVehicleCount() == old_count + 1);
				SpaceVehicle *second = nullptr; for (auto *v : SpaceVehicle::Iterate()) if (GetSpaceVehicleModel(*v) == 20 && v != builder) second = v; REQUIRE(second != nullptr);
				REQUIRE(Command<Commands::MoveLaunchFreight>::Post(center_ref.station, center_ref.generation, second->index, second->generation, steel, uint16_t{16}, false));
				REQUIRE(Command<Commands::MoveSpaceSupply>::Post(center_ref.station, center_ref.generation, rocket->index, rocket->generation, uint8_t{0}, uint16_t{15}, false));
				REQUIRE(Command<Commands::CarryLaunchProbe>::Post(rocket->index, rocket->generation, second->index, second->generation, false));
				CHECK(GetSpacePayloadMass(*rocket) <= 30 * 16);
				REQUIRE(Command<Commands::BeginLaunchFlight>::Post(rocket->index, rocket->generation, SpaceOrderTargetKind::LaunchCenter, uint32_t{center_ref.station.base()}, center_ref.generation)); advance(667);
				order(*second, site); until([&] { return second->cargo.TotalCount() == 0; }); CHECK(site.job.work_ticks == 0); advance(37); REQUIRE(site.job.work_ticks == 37);
				CHECK(site.job.inputs[0].TotalCount() == 16); CHECK(site.job.inputs[1].TotalCount() == 24); CHECK(site.job.consumed[0] == 0);
				save("construction-work-415.sav"); capture("construction-working.ppm");
			}
			REQUIRE(site.job.work_ticks == 37); const auto profit = builder->profit_this_year; advance(703);
			CHECK(site.job.work_ticks == 740); CHECK(site.state.phase == SpaceStructurePhase::FrameComplete); CHECK(site.job.inputs[0].TotalCount() == 0); CHECK(site.job.inputs[1].TotalCount() == 0);
			CHECK(site.job.consumed[0] == 16); CHECK(site.job.consumed[1] == 24); CHECK(builder->profit_this_year == profit - Money{4750} * 256);
			CHECK(CmdCancelSpaceBlueprint({}, site.index, site.generation).Failed()); CHECK(rocket->manifest->fuel_burned == 30); CHECK(rocket->transition->missions_completed == 2);
			advance(74); CHECK(site.job.work_ticks == 740); CHECK(site.job.consumed[1] == 24); save("construction-frame-415.sav"); capture("construction-frame.ppm");
			fmt::print("SPACE_CONSTRUCTION native=true paid_launches=2 builders=2 steel=16 goods=24 work_ticks=740 frame_only=true\n");
		} catch (...) { failure = std::current_exception(); }
		observed = true; _exit_game = true;
	};
	AutoRestoreBackup hook(observer, &observe); IConsole::CmdRegister("space_construction_test", Observe);
	const std::string source_path = source.string(), config_path = config.string(); std::array<std::string_view, 17> args = {"openttd_test.exe", "-v", "null:ticks=2", "-b", "null", "-s", "null", "-m", "null", "-x", "-X", "-I", "original_dos", "-c", config_path, "-g", source_path};
	CHECK(openttd_main(args) == 0); if (failure) std::rethrow_exception(failure); CHECK(observed);
}
