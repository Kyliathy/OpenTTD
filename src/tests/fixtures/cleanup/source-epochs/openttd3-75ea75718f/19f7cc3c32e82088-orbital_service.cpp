/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file orbital_service.cpp Paid native Core service, zero-grant pauses and exact continuation.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group1B.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../space_structure.h"
#include "../space_structure_cmd.h"
#include "../space_vehicle.h"
#include "../space_manifest.h"
#include "../space_transition.h"
#include "../launch_center_cmd.h"
#include "../asteroid.h"
#include "../company_base.h"
#include "../company_func.h"
#include "../company_manager_face.h"
#include "../economy_func.h"
#include "../command_func.h"
#include "../console_internal.h"
#include "../core/backup_type.hpp"
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

/** @return Detached paid operational Core with a single valid reserved service; creates no native cargo or utility energy. */
static SpaceStructureRecord ServiceRecord()
{
	SpaceStructureRecord r; r.ref = {SpaceStructureID{0}, 1}; r.state.owner = Owner{0}; r.state.x = 4; r.state.y = 4;
	r.state.capital_paid = 700000; r.state.phase = SpaceStructurePhase::FrameComplete; r.work_ticks = 740; r.consumed = {16, 24};
	r.module.emplace(); r.module->producer = UtilityProducerID{0}; r.module->consumer = UtilityConsumerID{0};
	auto &job = r.module->service; job.craft = {VehicleID{1}, 2}; job.remaining_ticks = 148;
	job.quantities = {1, 1}; job.inputs[0] = {{CargoPacketID{1}, StationID::Invalid()}}; job.inputs[1] = {{CargoPacketID{2}, StationID::Invalid()}};
	return r;
}

TEST_CASE("Contained Core service conserves reserved parts and exact typed identity", "[orbital-service-contract]")
{
	auto r = ServiceRecord(); CHECK(ValidateSpaceStructureRecords(std::array{r}, 2, 64, 64));
	r.module->service.remaining_ticks = 149; CHECK_FALSE(ValidateSpaceStructureRecords(std::array{r}, 2, 64, 64));
	r = ServiceRecord(); r.module->service.quantities[0] = 0; r.module->service.inputs[0].clear(); CHECK_FALSE(ValidateSpaceStructureRecords(std::array{r}, 2, 64, 64));
	r = ServiceRecord(); r.module->stock[0] = r.module->service.inputs[0]; r.module->quantities[0] = 1; CHECK_FALSE(ValidateSpaceStructureRecords(std::array{r}, 2, 64, 64));
	r = ServiceRecord(); r.module->service.remaining_ticks = 0; CHECK_FALSE(ValidateSpaceStructureRecords(std::array{r}, 2, 64, 64));
	r = ServiceRecord(); r.module->activation_ticks = 1; CHECK_FALSE(ValidateSpaceStructureRecords(std::array{r}, 2, 64, 64));
	r = ServiceRecord(); r.module->producer = UtilityProducerID{UINT32_MAX}; CHECK_FALSE(ValidateSpaceStructureRecords(std::array{r}, 2, 64, 64));
	r = ServiceRecord(); auto second = r; second.ref = {SpaceStructureID{1}, 2}; second.state.x = 10;
	second.module->service.inputs[0][0].first = CargoPacketID{3}; second.module->service.inputs[1][0].first = CargoPacketID{4};
	CHECK_FALSE(ValidateSpaceStructureRecords(std::array{r, second}, 3, 64, 64));
}

TEST_CASE("Core service accepts only its original typed utility binding and preserves retired company history", "[orbital-service-contract]")
{
	auto row = ServiceRecord(); UtilityPowerState power;
	UtilityPowerProducer source; source.meter.producer_id = UtilityProducerID{0}; source.meter.layer = MapLayer::Space;
	source.owner = CompanyID{0}; source.kind = UtilityGenerationKind::OrbitalPanel; power.producers.push_back(source);
	UtilityPowerConsumer consumer; consumer.meter.consumer_id = UtilityConsumerID{0}; consumer.meter.layer = MapLayer::Space;
	consumer.owner = CompanyID{0}; consumer.effect = UtilityConsumerEffect::AssemblyWork; consumer.capacity_j = 750;
	consumer.linked_producers = {UtilityProducerID{0}}; power.consumers.push_back(consumer);
	CHECK(ValidateOrbitalServiceUtilityRecords(std::array{row}, power));
	power.producers[0].owner = CompanyID{1}; CHECK_FALSE(ValidateOrbitalServiceUtilityRecords(std::array{row}, power));
	power.producers[0] = source; power.producers[0].meter.layer = MapLayer::Surface; CHECK_FALSE(ValidateOrbitalServiceUtilityRecords(std::array{row}, power));
	power.producers[0] = source; power.consumers[0].effect = UtilityConsumerEffect::TubeWork; CHECK_FALSE(ValidateOrbitalServiceUtilityRecords(std::array{row}, power));
	power.consumers[0] = consumer; power.consumers[0].linked_producers = {UtilityProducerID{1}}; CHECK_FALSE(ValidateOrbitalServiceUtilityRecords(std::array{row}, power));
	power.consumers[0] = consumer; row.module->service.last_work_interval = 2; CHECK_FALSE(ValidateOrbitalServiceUtilityRecords(std::array{row}, power));
	row.module->service.last_work_interval = 0; row.state.owner = OWNER_NONE;
	// Actual company removal retires the shared accounts and clears eligibility, retaining original lifetime use and reserved material.
	power.producers[0].active = false; power.producers[0].consumed_lifetime_j = 27750;
	power.consumers[0].active = false; power.consumers[0].linked_producers.clear();
	CHECK(ValidateOrbitalServiceUtilityRecords(std::array{row}, power)); CHECK(power.producers[0].consumed_lifetime_j == 27750);
}

static std::function<void()> *service_observer = nullptr;
/** @param args Console invocation. @return Whether the registered native service observer ran. */
static bool ObserveService(std::span<std::string_view> args)
{
	REQUIRE(args.size() == 1); REQUIRE(service_observer != nullptr); (*service_observer)(); return true;
}

TEST_CASE("A genuine Earth-delivered Core services a paid probe with actual parts and shared power", "[.orbital-service]")
{
	const char *mode = std::getenv("OTTD_ORBITAL_SERVICE_ACTION"), *input = std::getenv("OTTD_ORBITAL_SERVICE_SOURCE");
	REQUIRE(mode != nullptr); REQUIRE(input != nullptr); const std::string_view action{mode};
	const auto paths = GetTestCaseArtifactPaths("space", "service"); const auto config = GetTestRuntimeConfig();
	bool observed = false; std::exception_ptr failure;
	/** Execute inside the actual loaded world; preserve Catch failures across normal native shutdown. */
	std::function<void()> observe = [&] {
		try {
			AutoRestoreBackup actor(_current_company, _local_company);
			auto &site = *SpaceStructure::Get(SpaceStructureID{0}); REQUIRE(IsOrbitalModuleOperational(site));
			auto &module = *site.module; auto &job = module.service; SpaceVehicle *craft = ResolveSpaceCraft(job.craft); SpaceVehicle *rocket = nullptr;
			// Reuse the same actual paid Earth rocket; retained builders have embodied assembly parts, not spare service stock.
			for (auto *v : SpaceVehicle::Iterate()) if (GetSpaceVehicleModel(*v) == 14) rocket = v;
			if (craft == nullptr) {
				// Select an actually paid deployed builder; a separate paid resupply below provides its service compartments.
				for (auto *v : SpaceVehicle::Iterate()) {
					if (GetSpaceVehicleModel(*v) == 20 && v->position == GetSpaceStructureApproach(site.state) && v->manifest) craft = v;
				}
			}
			REQUIRE(craft != nullptr); REQUIRE(rocket != nullptr); auto &manifest = *craft->manifest; const auto ref = GetSpaceCraftReference(*craft);
			/** @param ticks Actual synchronized native opportunities, independent of fast-forward/render callbacks. */
			auto advance = [&](uint ticks) {
				AutoRestoreBackup pause(_pause_mode, PauseModes{}); const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(90);
				// Only the real controller's next economy tick counts; yielding without simulation must not earn service work.
				for (uint done = 0; done < ticks;) {
					REQUIRE(std::chrono::steady_clock::now() < deadline); const auto before = GetAsteroidField().elapsed_ticks; StateGameLoop();
					if (GetAsteroidField().elapsed_ticks == before) { std::this_thread::sleep_for(std::chrono::milliseconds(1)); continue; }
					REQUIRE(GetAsteroidField().elapsed_ticks == before + 1); ++done;
				}
			};
			/** @param name Isolated native checkpoint name. */
			auto save = [&](std::string_view name) {
				REQUIRE(ValidateSpaceVehicles()); AutoRestoreBackup pause(_pause_mode, PauseModes{PauseMode::Normal});
				AutoRestoreBackup compression(_savegame_format, std::string{"lzma"});
				REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(paths.saves / std::string{name}), false) == SaveLoadResult::Ok);
			};
			/** @param control One explicit operation on this precise native berth. */
			auto control = [&](OrbitalServiceControl control) { REQUIRE(Command<Commands::ServiceOrbitalProbe>::Post(site.index, site.generation, ref.vehicle, ref.generation, control)); };
			/** @return Actual shared original-source lifetime settlement, re-queried after each mutation. */
			auto used = [&] { return GetUtilityPowerProducer(module.producer)->consumed_lifetime_j; };
			if (const char *corrupt = std::getenv("OTTD_ORBITAL_SERVICE_CORRUPT")) {
				const auto digest = ComputeSpaceStructureDigest(), power = ComputeUtilityPowerDigest(); const auto money = Company::Get(_current_company)->money;
				CHECK(SaveOrLoad(corrupt, SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Error);
				CHECK(ComputeSpaceStructureDigest() == digest); CHECK(ComputeUtilityPowerDigest() == power); CHECK(Company::Get(_current_company)->money == money);
				fmt::print("ORBITAL_SERVICE preview_rejected=true authority_unchanged=true\n"); observed = true; _exit_game = true; return;
			}
			if (action == "prepare") {
				REQUIRE(module.producer == UtilityProducerID{UINT32_MAX}); REQUIRE(job.remaining_ticks == 0);
				const auto digest = ComputeSpaceStructureDigest(), power = ComputeUtilityPowerDigest();
				CHECK(CmdCommissionOrbitalServices({}, site.index, site.generation).Succeeded());
				CHECK(ComputeSpaceStructureDigest() == digest); CHECK(ComputeUtilityPowerDigest() == power);
				REQUIRE(Command<Commands::CommissionOrbitalServices>::Post(site.index, site.generation)); CHECK(used() == 0);
				CHECK(CmdCommissionOrbitalServices({}, site.index, site.generation).Failed());
				CHECK(CmdServiceOrbitalProbe({}, site.index, site.generation, ref.vehicle, ref.generation, OrbitalServiceControl::Start).Failed());
				const auto home = rocket->construction.home; const auto missions = rocket->transition->missions_completed;
				// These actual Surface-owned parts and15 fuel units take a paid full flight/recovery before entering the probe.
				for (uint8_t supply : {uint8_t{1}, uint8_t{2}}) REQUIRE(Command<Commands::MoveSpaceSupply>::Post(home.station, home.generation, rocket->index, rocket->generation, supply, uint16_t{1}, false));
				REQUIRE(Command<Commands::MoveSpaceSupply>::Post(home.station, home.generation, rocket->index, rocket->generation, uint8_t{0}, uint16_t{15}, false));
				REQUIRE(Command<Commands::BeginLaunchFlight>::Post(rocket->index, rocket->generation, SpaceOrderTargetKind::Craft, uint32_t{ref.vehicle.base()}, ref.generation));
				advance(667); CHECK(rocket->transition->missions_completed == missions + 1); CHECK(manifest.supplies[1].TotalCount() == 1); CHECK(manifest.supplies[2].TotalCount() == 1); CHECK(used() == 0);
				REQUIRE(Command<Commands::MoveOrbitalSupply>::Post(site.index, site.generation, ref.vehicle, ref.generation, uint8_t{1}, uint16_t{1}, false));
				const auto metal_only = ComputeSpaceStructureDigest();
				CHECK(CmdServiceOrbitalProbe({}, site.index, site.generation, ref.vehicle, ref.generation, OrbitalServiceControl::Start).Failed()); CHECK(ComputeSpaceStructureDigest() == metal_only);
				REQUIRE(Command<Commands::MoveOrbitalSupply>::Post(site.index, site.generation, ref.vehicle, ref.generation, uint8_t{1}, uint16_t{1}, true));
				REQUIRE(Command<Commands::MoveOrbitalSupply>::Post(site.index, site.generation, ref.vehicle, ref.generation, uint8_t{2}, uint16_t{1}, false));
				const auto electronics_only = ComputeSpaceStructureDigest();
				CHECK(CmdServiceOrbitalProbe({}, site.index, site.generation, ref.vehicle, ref.generation, OrbitalServiceControl::Start).Failed()); CHECK(ComputeSpaceStructureDigest() == electronics_only);
				REQUIRE(Command<Commands::MoveOrbitalSupply>::Post(site.index, site.generation, ref.vehicle, ref.generation, uint8_t{1}, uint16_t{1}, false));
				const auto ready = ComputeSpaceStructureDigest(), utility = ComputeUtilityPowerDigest();
				CHECK(CmdServiceOrbitalProbe({}, site.index, site.generation, ref.vehicle, ref.generation, OrbitalServiceControl::Start).Succeeded());
				CHECK(ComputeSpaceStructureDigest() == ready); CHECK(ComputeUtilityPowerDigest() == utility);
				control(OrbitalServiceControl::Start); CHECK(job.inputs[0].TotalCount() == 1); CHECK(job.inputs[1].TotalCount() == 1);
				const auto profit = craft->profit_this_year;
				advance(37); CHECK(job.remaining_ticks == 111); CHECK(used() == 27750); CHECK(craft->profit_this_year == profit - Money{250} * 256); save("service-work-418.sav");
			}
			const auto serial = job.services_at_start;
			if (action == "ownership") {
				const auto original = site.state.owner; const auto before = CaptureSpaceStructures()[0].module->service.inputs;
				REQUIRE(Company::CanAllocateItem()); auto *other = Company::Create(); SetCompanyManagerFaceStyle(other->face, 0); other->money = 10000000; const auto owner = other->index;
				// A real foreign company cannot operate, pause or steal the current berth before actual takeover.
				{ AutoRestoreBackup foreign(_current_company, owner); CHECK(CmdServiceOrbitalProbe({}, site.index, site.generation, ref.vehicle, ref.generation, OrbitalServiceControl::Pause).Failed()); }
				ChangeOwnershipOfCompanyItems(original, owner); CHECK(site.state.owner == owner); CHECK(craft->owner == owner); REQUIRE(ValidateSpaceVehicles());
				CHECK(GetUtilityPowerProducer(module.producer)->owner == owner); CHECK(used() == 27750);
				ChangeOwnershipOfCompanyItems(owner, INVALID_OWNER); delete Company::Get(owner);
				CHECK(site.state.owner == OWNER_NONE); CHECK(ResolveSpaceCraft(ref) == nullptr); CHECK(CaptureSpaceStructures()[0].module->service.inputs == before);
				REQUIRE(Company::CanAllocateItem()); auto *reused = Company::Create(); SetCompanyManagerFaceStyle(reused->face, 0); reused->money = 10000000; CHECK(reused->index == owner);
				// Reusing the removed company's numeric ID cannot revive its original Core, craft or retired electrical grant.
				{ AutoRestoreBackup foreign(_current_company, owner); CHECK(CmdCommissionOrbitalServices({}, site.index, site.generation).Failed()); CHECK(CmdServiceOrbitalProbe({}, site.index, site.generation, ref.vehicle, ref.generation, OrbitalServiceControl::Resume).Failed()); }
				advance(74); CHECK(job.remaining_ticks == 111); CHECK(used() == 27750);
				CHECK(job.inputs[0].TotalCount() == 1); CHECK(job.inputs[1].TotalCount() == 1); save("service-unowned-418.sav");
				fmt::print("ORBITAL_SERVICE native=true foreign_refused=true takeover=true lost_craft_parts_retained=true company_reuse_refused=true\n"); observed = true; _exit_game = true; return;
			}
			if (action == "cancel") {
				REQUIRE(job.remaining_ticks == 111); const auto spent = used(); control(OrbitalServiceControl::Cancel);
				CHECK(job.inputs[0].TotalCount() == 0); CHECK(job.inputs[1].TotalCount() == 0); CHECK(job.remaining_ticks == 0);
				CHECK(module.stock[GetSpaceSupplyCargo(SpaceSupply::RefinedMetal)].TotalCount() == 1);
				CHECK(module.stock[GetSpaceSupplyCargo(SpaceSupply::Electronics)].TotalCount() == 1);
				advance(74); CHECK(used() == spent); CHECK(manifest.services_completed == serial); save("service-cancelled-418.sav");
			} else {
				// Replays starting before the manual-pause boundary must perform the same74 paid idle ticks as the uninterrupted path.
				if (action == "prepare" || action == "work") {
					control(OrbitalServiceControl::Pause); const auto spent = used(); const auto profit = craft->profit_this_year; advance(74);
					CHECK(job.remaining_ticks == 111); CHECK(used() == spent); CHECK(craft->profit_this_year == profit - Money{100} * 256); save("service-paused-418.sav");
				}
				// Earlier checkpoints still need the37-tick zero-generation interval; the power checkpoint already includes it.
				if (action == "prepare" || action == "work" || action == "paused") {
					control(OrbitalServiceControl::Resume);
					REQUIRE(Command<Commands::SetOrbitalPanelPaused>::Post(site.index, site.generation, true));
					const auto profit = craft->profit_this_year;
					advance(37); CHECK(job.remaining_ticks == 111); CHECK(used() == 27750); CHECK(craft->profit_this_year == profit - Money{50} * 256); CHECK_FALSE(IsOrbitalServiceWorkTick(*craft)); save("service-power-418.sav");
				}
				if (action != "last") {
					REQUIRE(Command<Commands::SetOrbitalPanelPaused>::Post(site.index, site.generation, false));
					advance(110); CHECK(job.remaining_ticks == 1); CHECK(manifest.services_completed == serial);
					CHECK(job.inputs[0].TotalCount() == 1); CHECK(job.inputs[1].TotalCount() == 1); CHECK(used() == 110250); save("service-last-418.sav");
				}
				advance(1); CHECK(job.remaining_ticks == 0); CHECK(job.inputs[0].TotalCount() == 0); CHECK(job.inputs[1].TotalCount() == 0);
				CHECK(used() == 111000); CHECK(manifest.services_completed == serial + 1); CHECK(manifest.returns_remaining == 2); CHECK(module.services_completed == 1);
				advance(74); CHECK(used() == 111000); CHECK(manifest.services_completed == serial + 1);
				REQUIRE(BlitterFactory::SelectBlitter("8bpp-simple") != nullptr); GfxClearSpriteCache(); FontCache::ClearFontCaches(FONTSIZES_REQUIRED);
				ShowSpaceConstructionWindow(); auto *window = FindWindowById(WindowClass::SpaceConstruction, 0); REQUIRE(window != nullptr);
				CaptureNativeArtWindow(*window, paths.images / "core-service-ready.ppm"); save("service-complete-418.sav");
			}
			fmt::print("ORBITAL_SERVICE native=true reserved_real_parts=true shared_power=true no_allowance_stacking=true\n");
		} catch (...) { failure = std::current_exception(); }
		observed = true; _exit_game = true;
	};
	AutoRestoreBackup hook(service_observer, &observe); IConsole::CmdRegister("orbital_service_test", ObserveService);
	const std::string source_path{input}, config_path = config.string();
	std::array<std::string_view, 17> args = {"openttd_test.exe", "-v", "null:ticks=2", "-b", "null", "-s", "null", "-m", "null", "-x", "-X", "-I", "original_dos", "-c", config_path, "-g", source_path};
	CHECK(openttd_main(args) == 0); if (failure) std::rethrow_exception(failure); CHECK(observed);
}
