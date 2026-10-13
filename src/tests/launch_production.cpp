/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file launch_production.cpp Actual packets, rotated zones, receipts and native paid assembly/save; Plan9 Groups2B/8B. */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../launch_operations.h"
#include "../launch_center_cmd.h"
#include "../space_vehicle.h"
#include "../asteroid.h"
#include "../space_vehicle_cmd.h"
#include "../network/network_type.h"
#include "../vehicle_cmd.h"
#include "../hub_cargo.h"
#include "../multilayer_space_content.h"
#include "../station_base.h"
#include "../station_map.h"
#include "../station_gui.h"
#include "../station_func.h"
#include "../gui.h"
#include "../widgets/station_widget.h"
#include "../window_gui.h"
#include "../window_func.h"
#include "../blitter/factory.hpp"
#include "../spritecache.h"
#include "../fontcache.h"
#include "../company_base.h"
#include "../company_func.h"
#include "../command_func.h"
#include "../console_internal.h"
#include "../cargotype.h"
#include "../core/backup_type.hpp"
#include "../core/random_func.hpp"
#include "../openttd.h"
#include "../fios.h"
#include "../saveload/saveload_func.h"
#include "excavation_fixture.h"
#include "mock_environment.h"
#include "layer_save_fixture.h"
#include "test_artifact_paths.h"
#include "native_art_render_fixture.h"
#include <cstdlib>
#include <set>
#include <thread>
#include "../safeguards.h"

TEST_CASE("Launch zones partition each rotated native footprint and consumption retains packet accounting", "[launch-assembly-contract]")
{
	MockEnvironment::Instance(); ExcavationFixture world;
	const auto anchor = TileXYL(20, 20, MapLayer::Surface);
	for (uint8_t rotation = 0; rotation < 4; ++rotation) {
		std::set<TileIndex> all;
		REQUIRE(Station::CanAllocateItem());
		std::unique_ptr<Station> station{Station::Create(anchor)};
		station->owner = _current_company;
		station->launch_center = std::make_unique<LaunchCenterAnchor>(anchor, 1, rotation);
		const auto footprint = GetLaunchCenterFootprint(anchor, rotation);
		for (const auto zone : {LaunchFacilityZone::Pad, LaunchFacilityZone::Workshop, LaunchFacilityZone::Recovery, LaunchFacilityZone::Storage}) {
			const auto area = GetLaunchFacilityZone(anchor, rotation, zone); REQUIRE(area);
			for (auto tile : *area) { CHECK(footprint->Contains(tile)); CHECK(all.insert(tile).second); }
		}
		CHECK(all.size() == 12);
		const auto approach = *GetLaunchServiceApproach(*station); CHECK(IsAtLaunchServiceApproach(approach, *station));
		CHECK(IsAtLaunchServiceApproach({approach.x + SPACE_Q16 / 4, approach.y}, *station));
		CHECK_FALSE(IsAtLaunchServiceApproach({approach.x + SPACE_Q16 / 4, approach.y + 1}, *station));
		CHECK_FALSE(IsAtLaunchServiceApproach({approach.x + SPACE_Q16, approach.y}, *station));
	}
	CHECK_FALSE(GetLaunchFacilityZone(anchor, 4, LaunchFacilityZone::Pad));
	CHECK_FALSE(GetLaunchFacilityZone(anchor, 0, LaunchFacilityZone::End));
	StationCargoList stock{};
	REQUIRE(CargoPacket::CanAllocateItem());
	auto *packet = CargoPacket::Create(uint16_t{23}, uint16_t{7}, StationID{3}, anchor, Money{230});
	const auto id = packet->index; stock.Append(packet, StationID{4}); const auto seeds = _random;
	CHECK_FALSE(HubCargoTransfer::Consume(stock, 24)); CHECK_FALSE(HubCargoTransfer::Consume(stock, 0));
	REQUIRE(HubCargoTransfer::Consume(stock, 9)); CHECK(stock.AvailableCount() == 14); CHECK(stock.ReservedCount() == 0);
	const auto *remaining = stock.Packets()->begin()->second.front();
	CHECK(remaining->index == id); CHECK(remaining->Count() == 14); CHECK(remaining->GetFeederShare() == 140);
	CHECK(remaining->GetPeriodsInTransit() == 7); CHECK(stock.PeriodsInTransit() == 7);
	CHECK(remaining->GetSourceTile() == anchor); CHECK(remaining->GetFirstStation() == StationID{3}); CHECK(stock.Packets()->begin()->first == StationID{4});
	REQUIRE(HubCargoTransfer::Consume(stock, 14)); CHECK(stock.AvailableCount() == 0); CHECK(stock.Packets()->empty());
	CHECK(_random.state[0] == seeds.state[0]); CHECK(_random.state[1] == seeds.state[1]);
}
TEST_CASE("Paid production receipt and cross-family slots reject unearned or reused identities", "[launch-assembly-contract]")
{
	const LaunchCenterRef center{StationID{1}, 9}; const SpaceCraftRef craft{VehicleID{3}, 7};
	SpaceProductionRecord r{craft, {SpacePlacement::Assembling, center, center, TileIndex{20}, 148, 2, 14, 400000, false}};
	LaunchOperationsRecord op{center, {}}; op.slots.workshop = craft;
	CHECK(ValidateSpaceProductionRecords(std::array{r}, 64, 64));
	CHECK(ValidateLaunchProductionLinks(std::array{r}, std::array{op}, std::array{craft}, std::array{center}));
	CHECK_FALSE(ValidateSpaceProductionRecords(std::array{r, r}, 64, 64));
	for (uint16_t model : {14, 15, 19, 20}) { auto valid = r; valid.state.model = model; valid.state.capital_paid = GetSpaceVehicleConstruction(model)->capital; CHECK(ValidateSpaceProductionRecords(std::array{valid}, 64, 64)); }
	for (uint i = 0; i < 9; ++i) {
		auto bad = r;
		switch (i) {
			case 0: bad.craft.generation = 0; break;
			case 1: bad.state.capital_paid = 0; break;
			case 2: bad.state.content_version = 3; break;
			case 3: bad.state.model = 18; break;
			case 4: bad.state.remaining_ticks = 149; break;
			case 5: bad.state.remaining_ticks = 0; break;
			case 6: bad.state.surface_tile = TileIndex{4096}; break;
			case 7: bad.state.placement = SpacePlacement::Orbital; break;
			case 8: bad.state.origin.generation = 0; break;
		}
		CHECK_FALSE(ValidateSpaceProductionRecords(std::array{bad}, 64, 64));
	}
	auto stale = op; ++stale.slots.workshop.generation;
	CHECK_FALSE(ValidateLaunchProductionLinks(std::array{r}, std::array{stale}, std::array{craft}, std::array{center}));
	stale = op; stale.slots.workshop = {};
	CHECK_FALSE(ValidateLaunchProductionLinks(std::array{r}, std::array{stale}, std::array{craft}, std::array{center}));
	stale = op; stale.slots.service = craft; CHECK_FALSE(ValidateLaunchOperationRecords(std::array{stale}));
	stale = op; stale.slots.pad_queue.push_back(craft); CHECK_FALSE(ValidateLaunchOperationRecords(std::array{stale}));
	stale = op; ++stale.center.generation;
	CHECK_FALSE(ValidateLaunchProductionLinks(std::array{r}, std::array{stale}, std::array{craft}, std::array{center}));
	{
		std::vector<SpaceProductionRecord> fleet; std::vector<SpaceCraftRef> identities; LaunchOperationsRecord filled{center, {}};
		for (uint32_t i = 0; i < 8; ++i) {
			auto queued = r; queued.craft = {VehicleID{i}, uint64_t{i} + 1}; queued.state.placement = SpacePlacement::SurfaceStored; queued.state.remaining_ticks = 0;
			fleet.push_back(queued); identities.push_back(queued.craft); filled.slots.pad_queue.push_back(queued.craft);
		}
		CHECK(ValidateLaunchProductionLinks(fleet, std::array{filled}, identities, std::array{center}));
		filled.slots.pad_queue.push_back({VehicleID{8}, 9}); CHECK_FALSE(ValidateLaunchOperationRecords(std::array{filled}));
	}
	r.state.placement = SpacePlacement::Unhomed; r.state.home = {}; r.state.paused = true; op.slots.workshop = {};
	CHECK(ValidateSpaceProductionRecords(std::array{r}, 64, 64));
	CHECK(ValidateLaunchProductionLinks(std::array{r}, std::array{op}, std::array{craft}, std::array{center}));
	LoadCheckData preview; preview.space_production_rows.push_back(r); preview.launch_operations_rows.push_back(op); preview.Clear();
	CHECK(preview.space_production_rows.empty()); CHECK(preview.launch_operations_rows.empty());
}

static std::function<void()> *observer = nullptr;
static bool ObserveAssembly(std::span<std::string_view> args) { REQUIRE(args.size() == 1); REQUIRE(observer != nullptr); (*observer)(); return true; }
TEST_CASE("Native workshop charges once and resumes the same paid dormant hull", "[.launch-assembly]")
{
	const char *action_env = std::getenv("OTTD_LAUNCH_ASSEMBLY_ACTION"); REQUIRE(action_env != nullptr); const std::string_view action{action_env};
	const char *input = std::getenv("OTTD_LAUNCH_ASSEMBLY_SOURCE");
	const auto source = action == "prepare" ? FindTestRepositoryRoot() / "src/tests/fixtures/demos/space/space-launch-hub-1990-408.sav" : std::filesystem::path{input == nullptr ? "" : input};
	const auto paths = GetTestCaseArtifactPaths("space", "assembly"); const auto config = GetTestRuntimeConfig();
	bool observed = false; std::exception_ptr failure;
	std::function<void()> observe = [&] {
		try {
			AutoRestoreBackup actor(_current_company, _local_company);
			REQUIRE(ValidateSpaceVehicles()); REQUIRE(GetLaunchCenterReferences().size() == 1);
			auto center_ref = GetLaunchCenterReferences().front(); auto *center = ResolveLaunchCenter(center_ref); REQUIRE(center != nullptr);
			auto *company = Company::Get(_current_company);
			const auto steel = GetCargoTypeByLabel(CargoLabel{"STEL"}), goods = GetCargoTypeByLabel(CargoLabel{"GOOD"});
			auto advance = [&](uint ticks) {
				AutoRestoreBackup pause(_pause_mode, PauseModes{}); const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(30);
				for (uint i = 0; i < ticks;) {
					REQUIRE(std::chrono::steady_clock::now() < deadline); const auto before = GetAsteroidField().elapsed_ticks; const auto work = ComputeLaunchProductionDigest();
					StateGameLoop(); if (GetAsteroidField().elapsed_ticks == before) { CHECK(ComputeLaunchProductionDigest() == work); std::this_thread::sleep_for(std::chrono::milliseconds(1)); continue; }
					REQUIRE(GetAsteroidField().elapsed_ticks == before + 1); ++i;
				}
			};
			auto save = [&](std::string_view name) {
				AutoRestoreBackup pause(_pause_mode, PauseModes{PauseMode::Normal}); AutoRestoreBackup compression(_savegame_format, std::string{"lzma"});
				REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(paths.saves / std::string{name}), false) == SaveLoadResult::Ok);
			};
			if (action == "prepare") {
				REQUIRE(GetSpaceVehicleCount() == 0); const auto seeds = _random; const auto money = company->money;
				CHECK(CmdAssembleLaunchCraft({}, center_ref.station, center_ref.generation, 14).Failed());
				// Explicit fixture freight enters the actual StationCargoList. Gameplay has no stock grant.
				REQUIRE(CargoPacket::CanAllocateItem());
				center->goods[steel].GetOrCreateData().cargo.Append(CargoPacket::Create(center->index, uint16_t{100}, Source{Source::Invalid, SourceType::Industry}), StationID::Invalid());
				CHECK(CmdAssembleLaunchCraft({}, center_ref.station, center_ref.generation, 14).Failed());
				REQUIRE(CargoPacket::CanAllocateItem());
				center->goods[goods].GetOrCreateData().cargo.Append(CargoPacket::Create(center->index, uint16_t{100}, Source{Source::Invalid, SourceType::Industry}), StationID::Invalid());
				CHECK(CmdAssembleLaunchCraft({}, center_ref.station, center_ref.generation + 1, 14).Failed());
				for (uint16_t bad : {0, 16, 17, 18, 21}) CHECK(CmdAssembleLaunchCraft({}, center_ref.station, center_ref.generation, bad).Failed());
				{
					AutoRestoreBackup year(TimerGameCalendar::year, TimerGameCalendar::Year{1989}); CHECK(CmdAssembleLaunchCraft({}, center_ref.station, center_ref.generation, 14).Failed());
				}
				CHECK(company->money == money); CHECK(GetSpaceVehicleCount() == 0); CHECK(_random.state[0] == seeds.state[0]); CHECK(_random.state[1] == seeds.state[1]);
				{
					AutoRestoreBackup funds(company->money, Money{0}); AutoRestoreBackup finite(_settings_game.difficulty.infinite_money, false);
					const auto generation = GetNextSpaceVehicleGeneration(); const auto digest = ComputeLaunchProductionDigest();
					CHECK_FALSE(Command<Commands::AssembleLaunchCraft>::Post(center_ref.station, center_ref.generation, uint16_t{14}));
					CHECK(GetSpaceVehicleCount() == 0); CHECK(GetNextSpaceVehicleGeneration() == generation); CHECK(ComputeLaunchProductionDigest() == digest);
					CHECK(center->goods[steel].GetData().cargo.AvailableCount() == 100); CHECK(center->goods[goods].GetData().cargo.AvailableCount() == 100);
				}
				REQUIRE(BlitterFactory::SelectBlitter("8bpp-simple") != nullptr); GfxClearSpriteCache(); FontCache::ClearFontCaches(FONTSIZES_REQUIRED);
				ShowStationViewWindow(center->index); auto *station_window = FindWindowById(WindowClass::StationView, center->index.base()); REQUIRE(station_window != nullptr);
				station_window->OnClick({}, WID_SV_LAUNCH_ASSEMBLY, 1);
				auto *workshop_window = FindWindowById(WindowClass::LaunchAssembly, center->index.base()); REQUIRE(workshop_window != nullptr);
				Money capital = 0;
				for (uint16_t model : {14, 15, 19, 20}) {
					const auto &d = *GetSpaceVehicleConstruction(model); const auto before = company->money; const auto digest = ComputeLaunchProductionDigest();
					const auto quote = CmdAssembleLaunchCraft({}, center_ref.station, center_ref.generation, model);
					REQUIRE(quote.Succeeded()); CHECK(quote.GetCost() == Money{d.capital}); CHECK(company->money == before); CHECK(ComputeLaunchProductionDigest() == digest);
					CaptureNativeArtWindow(*workshop_window, paths.images / fmt::format("workshop-model-{}.ppm", model));
					workshop_window->OnClick({}, model == 14 ? 1 : model == 15 ? 2 : model == 19 ? 3 : 4, 1);
					CHECK(company->money == before - d.capital); capital += d.capital;
					auto *v = ResolveSpaceCraft(center->launch_operations->workshop); REQUIRE(v != nullptr); CHECK(GetSpaceVehicleModel(*v) == model);
					CHECK(CmdAssembleLaunchCraft({}, center_ref.station, center_ref.generation, model).Failed());
					CHECK(CmdRemoveLaunchCenter({}, center->launch_center->tile).Failed());
					CHECK(CmdStartStopVehicle({}, v->index, false).Failed()); CHECK(AssignSpaceMiningMission(*v, 0, false).Failed());
					CHECK(CmdSellVehicle({}, v->index, false, false, ClientID{}).Failed());
					CHECK(std::get<0>(CmdBuildVehicle({}, center->launch_center->tile, v->engine_type, false, v->cargo_type, ClientID{})).Failed());
					CHECK(std::get<0>(CmdCloneVehicle({}, center->launch_center->tile, v->index, false)).Failed());
					const auto state = v->construction; const auto counter = v->tick_counter; const auto profit = v->profit_this_year;
					v->Tick(); CHECK(v->construction == state); CHECK(v->tick_counter == counter); CHECK(v->profit_this_year == profit);
					REQUIRE(Command<Commands::PauseLaunchAssembly>::Post(v->index, v->generation, true)); advance(3); CHECK(v->construction.remaining_ticks == 148);
					REQUIRE(Command<Commands::PauseLaunchAssembly>::Post(v->index, v->generation, false));
					advance(model == 20 ? 37 : 148);
					if (model != 20) CHECK(v->construction.placement == SpacePlacement::SurfaceStored);
				}
				CHECK(capital == 660000); CHECK(center->goods[steel].GetData().cargo.AvailableCount() == 78); CHECK(center->goods[goods].GetData().cargo.AvailableCount() == 64);
				auto *v = ResolveSpaceCraft(center->launch_operations->workshop); REQUIRE(v != nullptr); const auto paid = v->construction;
				const auto anchor = center->launch_center->tile; const auto rotation = center->launch_center->rotation;
				ForgetLaunchCenter(*center); CHECK(v->construction.placement == SpacePlacement::Unhomed); CHECK(v->construction.remaining_ticks == paid.remaining_ticks);
				InstallNewLaunchCenter(*center, anchor, rotation); // Same original footprint, native station remains the owner.
				center_ref = GetLaunchCenterReference(*center); CHECK(center_ref != paid.home);
				CHECK(CmdRecoverLaunchCraft({}, paid.home.station, paid.home.generation, v->index, v->generation).Failed());
				const auto before = company->money; REQUIRE(Command<Commands::RecoverLaunchCraft>::Post(center_ref.station, center_ref.generation, v->index, v->generation));
				CHECK(company->money == before); CHECK(v->construction.origin == paid.origin); CHECK(v->construction.capital_paid == paid.capital_paid); CHECK(v->construction.remaining_ticks == 111);
				for (auto *stranded : SpaceVehicle::Iterate()) if (stranded->construction.placement == SpacePlacement::Unhomed) {
					REQUIRE(Command<Commands::RecoverLaunchCraft>::Post(center_ref.station, center_ref.generation, stranded->index, stranded->generation));
				}
				REQUIRE(ValidateSpaceVehicles()); save("assembly-partial-414.sav");
			}
			if (const char *corrupt = std::getenv("OTTD_LAUNCH_ASSEMBLY_CORRUPT")) {
				const auto digest = ComputeLaunchProductionDigest();
				REQUIRE(SaveOrLoad(source.string(), SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Ok);
				CHECK(SaveOrLoad(corrupt, SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Error);
				REQUIRE(SaveOrLoad(source.string(), SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Ok);
				CHECK(ComputeLaunchProductionDigest() == digest); fmt::print("LAUNCH_ASSEMBLY preview_reject=true authority_unchanged=true\n");
			}
			REQUIRE(center->launch_operations->workshop != SpaceCraftRef{}); advance(2200); REQUIRE(ValidateSpaceVehicles());
			CHECK(center->launch_operations->workshop == SpaceCraftRef{}); CHECK(GetSpaceVehicleCount() == 4); CHECK(center->launch_operations->pad_queue.size() == 1);
			for (const auto *v : SpaceVehicle::Iterate()) { CHECK(v->construction.placement == SpacePlacement::SurfaceStored); CHECK(v->construction.remaining_ticks == 0); CHECK(v->cargo.TotalCount() == 0); }
			save("assembly-complete-414.sav");
			// Exercise real takeover and destruction after retaining the clean continuation artifact.
			const auto paid_fleet = CaptureSpaceProduction(); const auto old_owner = center->owner;
			REQUIRE(Company::CanAllocateItem()); auto *receiver = Company::Create();
			ChangeOwnershipOfCompanyItems(old_owner, receiver->index);
			CHECK(center->owner == receiver->index); REQUIRE(ValidateSpaceVehicles());
			for (const auto &r : paid_fleet) { const auto *v = ResolveSpaceCraft(r.craft); REQUIRE(v != nullptr); CHECK(v->owner == receiver->index); CHECK(v->construction == r.state); }
			const auto deleted_ref = GetLaunchCenterReference(*center); const auto anchor = center->launch_center->tile; const auto rotation = center->launch_center->rotation;
			delete center; CHECK(ResolveLaunchCenter(deleted_ref) == nullptr);
			for (const auto &r : paid_fleet) { const auto *v = ResolveSpaceCraft(r.craft); REQUIRE(v != nullptr); CHECK(v->construction.placement == SpacePlacement::Unhomed); CHECK(v->construction.paused); CHECK(v->construction.origin == r.state.origin); CHECK(v->construction.remaining_ticks == r.state.remaining_ticks); }
			// Native pool reuse may occupy the identical StationID; it never inherits paid craft.
			center = Station::CreateAtIndex(deleted_ref.station, anchor); center->owner = receiver->index; center->facilities.Set(StationFacility::LaunchCenter);
			for (auto tile : *GetLaunchCenterFootprint(anchor, rotation)) { MakeStation(tile, center->owner, center->index, StationType::LaunchCenter, 0); center->spread.Add(tile); }
			InstallNewLaunchCenter(*center, anchor, rotation);
			CHECK(GetLaunchCenterReference(*center) != deleted_ref); CHECK_FALSE(HasLaunchCraft(GetLaunchCenterReference(*center)));
			for (const auto &r : paid_fleet) { CHECK(ResolveSpaceCraft(r.craft)->construction.home == LaunchCenterRef{}); CHECK(CmdRecoverLaunchCraft({}, deleted_ref.station, deleted_ref.generation, r.craft.vehicle, r.craft.generation).Failed()); }
			{ AutoRestoreBackup local_actor(_current_company, _local_company); SetLocalCompany(old_owner); } // Restore both local/current identities before native input resumes.
			fmt::print("LAUNCH_ASSEMBLY native=true paid_once=true dormant=true takeover=true deletion_reuse=true digest={}\n", ComputeLaunchProductionDigest());
		} catch (...) { failure = std::current_exception(); }
		observed = true; _exit_game = true;
	};
	AutoRestoreBackup hook(observer, &observe); IConsole::CmdRegister("launch_assembly_test", ObserveAssembly);
	const std::string source_path = source.string(), config_path = config.string();
	std::array<std::string_view, 17> args = {"openttd_test.exe", "-v", "null:ticks=2", "-b", "null", "-s", "null", "-m", "null", "-x", "-X", "-I", "original_dos", "-c", config_path, "-g", source_path};
	CHECK(openttd_main(args) == 0); if (failure) std::rethrow_exception(failure); CHECK(observed);
}
