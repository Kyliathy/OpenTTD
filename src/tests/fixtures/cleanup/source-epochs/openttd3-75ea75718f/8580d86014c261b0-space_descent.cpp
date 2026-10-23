/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file space_descent.cpp Real mining, paid pod custody, target loss and onboard service fixtures. */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../launch_center.h"
#include "../launch_center_cmd.h"
#include "../launch_operations.h"
#include "../space_vehicle.h"
#include "../space_vehicle_cmd.h"
#include "../space_manifest.h"
#include "../space_pod.h"
#include "../space_transition.h"
#include "../asteroid.h"
#include "../hub_cargo.h"
#include "../station_base.h"
#include "../company_base.h"
#include "../company_func.h"
#include "../company_manager_face.h"
#include "../command_func.h"
#include "../water_cmd.h"
#include "../landscape.h"
#include "../console_internal.h"
#include "../core/backup_type.hpp"
#include "../openttd.h"
#include "../fios.h"
#include "../saveload/saveload_func.h"
#include "excavation_fixture.h"
#include "mock_environment.h"
#include "layer_save_fixture.h"
#include "test_artifact_paths.h"
#include <cstdlib>
#include <thread>
#include "../safeguards.h"

TEST_CASE("Paid descent transport binds one pod and reserves each return allowance once", "[space-logistics-contract]")
{
	const LaunchCenterRef center{StationID{1}, 4}; const SpaceCraftRef craft{VehicleID{2}, 7};
	SpaceDescentRecord r; r.pod = {SpacePodID{3}, 9}; r.id = 2; r.center = center; r.sender = craft.vehicle; r.sender_generation = craft.generation;
	r.cargo = CargoType{0}; r.quantity = 10; r.remaining_ticks = 111; r.release_point = {20 * SPACE_Q16, 20 * SPACE_Q16}; r.packets.push_back({CargoPacketID{5}, StationID::Invalid()});
	SpacePodRecord pod; pod.ref = r.pod; pod.state.custody = SpacePodCustody::Descent; pod.state.position = r.release_point;
	SpaceManifestRecord manifest; manifest.craft = craft; manifest.returns_remaining = 2; manifest.pending_returns = 1;
	LaunchCenterRecord anchor{center.station, {TileIndex{20 + 20 * 64}, center.generation, 0}};
	CHECK(ValidateSpaceDescentRecords(std::array{r}, 3, 64, 64));
	CHECK(ValidateSpaceDescentLinks(std::array{r}, std::array{pod}, std::array{manifest}, std::array{anchor}, 64));
	CHECK_FALSE(ValidateSpaceDescentRecords(std::array{r, r}, 3, 64, 64));
	for (uint i = 0; i < 10; ++i) {
		auto bad = r;
		switch (i) {
			case 0: bad.pod.generation = 0; break;
			case 1: bad.id = 3; break;
			case 2: bad.center.generation = 0; break;
			case 3: bad.sender_generation = 0; break;
			case 4: bad.quantity = 41; break;
			case 5: bad.remaining_ticks = 149; break;
			case 6: bad.age_ticks = Ticks::CARGO_AGING_TICKS; break;
			case 7: bad.blocked = SpaceDescentBlock::End; break;
			case 8: bad.packets.clear(); break;
			case 9: bad.release_point.x = -1; break;
		}
		CHECK_FALSE(ValidateSpaceDescentRecords(std::array{bad}, 3, 64, 64));
	}
	manifest.pending_returns = 0; CHECK_FALSE(ValidateSpaceDescentLinks(std::array{r}, std::array{pod}, std::array{manifest}, std::array{anchor}, 64)); manifest.pending_returns = 1;
	CHECK_FALSE(ValidateSpaceDescentLinks(std::array{r}, std::array{pod}, std::array{manifest}, {}, 64)); r.blocked = SpaceDescentBlock::TargetLost;
	CHECK(ValidateSpaceDescentLinks(std::array{r}, std::array{pod}, std::array{manifest}, {}, 64));
	CHECK_FALSE(ValidateSpaceDescentLinks({}, std::array{pod}, std::array{manifest}, {}, 64));
	LoadCheckData preview; preview.space_descents_present = true; preview.next_space_descent_id = 3; preview.space_descent_rows.push_back(r); preview.launch_center_preview_rows.push_back(anchor); preview.Clear();
	CHECK_FALSE(preview.space_descents_present); CHECK(preview.next_space_descent_id == 1); CHECK(preview.space_descent_rows.empty()); CHECK(preview.launch_center_preview_rows.empty());
}
TEST_CASE("Descent preserves horizontal cargo history for Earth and Space origins", "[space-logistics-contract]")
{
	MockEnvironment::Instance(); ExcavationFixture world;
	for (const auto layer : {MapLayer::Surface, MapLayer::Space}) {
		const auto origin = TileXYL(20, 20, layer), release = TileXYL(25, 24, MapLayer::Space), earth = TileXYL(25, 24, MapLayer::Surface), destination = TileXYL(30, 29, MapLayer::Surface);
		VehicleCargoList outbound{}, train{}; StationCargoList pod{};
		REQUIRE(CargoPacket::CanAllocateItem()); auto *cp = CargoPacket::Create(uint16_t{10}, uint16_t{7}, StationID::Invalid(), origin, Money{100}); cp->UpdateLoadingTile(origin); outbound.Append(cp);
		CHECK(cp->GetDistance(release) == 9); REQUIRE(HubCargoTransfer::ReleaseOrbital(outbound, pod, 10, release)); CHECK(outbound.TotalCount() == 0);
		REQUIRE(HubCargoTransfer::Move(pod, train, 10, earth)); cp = train.Packets()->front(); CHECK(cp->GetDistance(earth) == 9); CHECK(cp->GetDistance(destination) == 19);
		CHECK(cp->GetSourceTile() == origin); CHECK(cp->GetPeriodsInTransit() == 7); CHECK(cp->GetFeederShare() == 100); CHECK(cp->GetFirstStation() == StationID::Invalid());
	}
}

static std::function<void()> *descent_observer = nullptr;
static bool ObserveDescent(std::span<std::string_view> args) { REQUIRE(args.size() == 1); REQUIRE(descent_observer != nullptr); (*descent_observer)(); return true; }
TEST_CASE("Real mined packets descend once through paid shells and survive target loss and service reload", "[.space-descent]")
{
	const char *action_env = std::getenv("OTTD_SPACE_DESCENT_ACTION"), *input = std::getenv("OTTD_SPACE_DESCENT_SOURCE"); REQUIRE(action_env != nullptr); REQUIRE(input != nullptr);
	const std::string_view action{action_env}; const std::filesystem::path source{input}; const auto paths = GetTestCaseArtifactPaths("space", "descent"); const auto config = GetTestRuntimeConfig();
	bool observed = false; std::exception_ptr failure;
	std::function<void()> observe = [&] {
		try {
			AutoRestoreBackup actor(_current_company, _local_company); REQUIRE(ValidateSpaceVehicles());
			LaunchCenterRef center_ref; for (const auto ref : GetLaunchCenterReferences()) if (ResolveLaunchCenter(ref)->owner == _current_company) { REQUIRE(center_ref == LaunchCenterRef{}); center_ref = ref; }
			auto *center = ResolveLaunchCenter(center_ref); REQUIRE(center != nullptr); auto *company = Company::Get(_current_company);
			auto find = [&](uint16_t model) -> SpaceVehicle * { for (auto *v : SpaceVehicle::Iterate()) if (GetSpaceVehicleModel(*v) == model) return v; return nullptr; };
			auto *rocket = find(14); auto *miner = find(15); REQUIRE((rocket && miner)); REQUIRE(miner->manifest);
			auto advance = [&](uint ticks) {
				AutoRestoreBackup pause(_pause_mode, PauseModes{}); const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(30);
				for (uint i = 0; i < ticks;) { REQUIRE(std::chrono::steady_clock::now() < deadline); const auto before = GetAsteroidField().elapsed_ticks;
					StateGameLoop(); if (GetAsteroidField().elapsed_ticks == before) { std::this_thread::sleep_for(std::chrono::milliseconds(1)); continue; }
					REQUIRE(GetAsteroidField().elapsed_ticks == before + 1); ++i;
				}
			};
			auto save = [&](std::string_view name) { REQUIRE(ValidateSpaceVehicles()); AutoRestoreBackup pause(_pause_mode, PauseModes{PauseMode::Normal}); AutoRestoreBackup compression(_savegame_format, std::string{"lzma"});
				REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(paths.saves / std::string{name}), false) == SaveLoadResult::Ok); };
			if (action == "prepare") {
				REQUIRE(miner->manifest->pods.size() == 2); CHECK(miner->manifest->returns_remaining == 2); CHECK(miner->manifest->supplies[1].TotalCount() == 1); CHECK(miner->manifest->supplies[2].TotalCount() == 1);
				REQUIRE(Command<Commands::CarryLaunchProbe>::Post(rocket->index, rocket->generation, miner->index, miner->generation, false));
				REQUIRE(Command<Commands::BeginLaunchFlight>::Post(rocket->index, rocket->generation, SpaceOrderTargetKind::LaunchCenter, uint32_t{center_ref.station.base()}, center_ref.generation)); advance(667);
				REQUIRE(miner->construction.placement == SpacePlacement::Orbital);
				const auto &field = GetAsteroidField(); REQUIRE_FALSE(field.bodies.empty());
				const auto target = std::min_element(field.bodies.begin(), field.bodies.end(), [&](const auto &a, const auto &b) { return SpaceDistanceSquared(a.motion.position, miner->position) < SpaceDistanceSquared(b.motion.position, miner->position); })->id;
				CHECK_FALSE(GetAsteroidQuantityDisclosure(_current_company, target)); // Quantity disclosure never gates actual unsurveyed mining.
				REQUIRE(Command<Commands::EditSpaceOrder>::Post(miner->index, miner->generation, SpaceOrderEdit::Insert, uint8_t{0}, SpaceOrderAction::Mine, SpaceOrderTargetKind::Body, target.base(), uint64_t{0}));
				REQUIRE(Command<Commands::EditSpaceOrder>::Post(miner->index, miner->generation, SpaceOrderEdit::Insert, uint8_t{1}, SpaceOrderAction::Visit, SpaceOrderTargetKind::LaunchCenter, uint32_t{center_ref.station.base()}, center_ref.generation));
				uint steps = 0; while (miner->space_orders.cursor != miner->space_orders.entries.size()) { REQUIRE(++steps < 20000); advance(1); }
				REQUIRE(miner->cargo.TotalCount() == 20); CHECK(miner->position == *GetLaunchCenterPlaneReference(center->launch_center->tile));
				const auto shell = miner->manifest->pods.front(); const auto money = company->money; const auto digest = ComputeSpaceVehicleDigest();
				REQUIRE(CanReleaseOrbitalCargo(GetSpaceCraftReference(*miner), center_ref, shell, 10)); CHECK(company->money == money); CHECK(ComputeSpaceVehicleDigest() == digest);
				CHECK_FALSE(CanReleaseOrbitalCargo(GetSpaceCraftReference(*miner), {center_ref.station, center_ref.generation + 1}, shell, 10));
				REQUIRE(Company::CanAllocateItem()); auto *foreign = Company::Create(); SetCompanyManagerFaceStyle(foreign->face, 0); foreign->money = 10000000; // Explicit fixture funds and valid presentation, never a stock or ownership grant.
				LaunchCenterRef foreign_ref;
				{
					AutoRestoreBackup foreign_actor(_current_company, foreign->index);
					const auto point = *GetLaunchCenterPlaneReference(center->launch_center->tile);
					// Try every rotation, then add paid canal access to an otherwise flat, empty site.
					// The retained terrain need not supply two naturally usable waterside footprints.
					for (uint pass = 0; pass < 2 && foreign_ref == LaunchCenterRef{}; ++pass)
					for (uint y = 1; y + 4 < Map::SizeY() && foreign_ref == LaunchCenterRef{}; ++y)
					for (uint x = 1; x + 4 < Map::SizeX() && foreign_ref == LaunchCenterRef{}; ++x)
					for (uint8_t rotation = 0; rotation < 4 && foreign_ref == LaunchCenterRef{}; ++rotation) {
						if (!IsInsideLaunchCenterDropZone(point, {static_cast<int32_t>(x * SPACE_Q16), static_cast<int32_t>(y * SPACE_Q16)})) continue;
						const auto tile = TileXYL(x, y, MapLayer::Surface);
						const auto area = GetLaunchCenterFootprint(tile, rotation); REQUIRE(area);
						if (pass == 1 && !HasLaunchCenterWaterAccess(*area)) {
							bool flat_empty = true;
							for (const auto cell : *area) if ((!IsTileType(cell, TileType::Clear) && !IsTileType(cell, TileType::Trees)) || !IsTileFlat(cell) || GetTileZ(cell) != GetTileZ(tile)) flat_empty = false;
							if (!flat_empty) continue;
							bool canal_built = false;
							for (uint side = 0; side < 4 && !canal_built; ++side) {
								const uint edge = side < 2 ? area->h : area->w;
								for (uint offset = 0; offset < edge && !canal_built; ++offset) {
									const int water_x = static_cast<int>(x) + (side == 0 ? -1 : side == 1 ? area->w : static_cast<int>(offset));
									const int water_y = static_cast<int>(y) + (side == 2 ? -1 : side == 3 ? area->h : static_cast<int>(offset));
									if (water_x < 1 || water_y < 1 || water_x + 1 >= static_cast<int>(Map::SizeX()) || water_y + 1 >= static_cast<int>(Map::SizeY())) continue;
									const auto water = TileXYL(water_x, water_y, MapLayer::Surface);
									if ((!IsTileType(water, TileType::Clear) && !IsTileType(water, TileType::Trees)) || CmdBuildCanal({}, water, water, WaterClass::Canal, false).Failed()) continue;
									const auto canal = CmdBuildCanal({DoCommandFlag::Execute}, water, water, WaterClass::Canal, false); REQUIRE(canal.Succeeded()); SubtractMoneyFromCompany(foreign->index, canal); canal_built = true;
								}
							}
							if (!canal_built) continue;
						}
						const auto quote = CmdBuildLaunchCenter({}, tile, rotation, NEW_STATION, true); if (quote.Failed()) continue;
						const auto built = CmdBuildLaunchCenter({DoCommandFlag::Execute}, tile, rotation, NEW_STATION, true); REQUIRE(built.Succeeded()); SubtractMoneyFromCompany(foreign->index, built);
						for (const auto ref : GetLaunchCenterReferences()) if (ResolveLaunchCenter(ref)->owner == foreign->index) foreign_ref = ref;
					}
				}
				REQUIRE(foreign_ref != LaunchCenterRef{}); CHECK(IsInsideLaunchCenterDropZone(miner->position, *GetLaunchCenterPlaneReference(ResolveLaunchCenter(foreign_ref)->launch_center->tile)));
				CHECK_FALSE(CanReleaseOrbitalCargo(GetSpaceCraftReference(*miner), foreign_ref, shell, 10)); REQUIRE(ValidateSpaceVehicles());
				REQUIRE(center->goods[miner->cargo_type].TotalCount() == 0); REQUIRE(CargoPacket::CanAllocateItem());
				center->goods[miner->cargo_type].GetOrCreateData().cargo.Append(CargoPacket::Create(center->index, uint16_t{1024}, Source{Source::Invalid, SourceType::Industry}), StationID::Invalid()); // Explicit capacity fixture stock.
				CHECK_FALSE(CanReleaseOrbitalCargo(GetSpaceCraftReference(*miner), center_ref, shell, 10)); CHECK(miner->cargo.TotalCount() == 20); CHECK(miner->manifest->pending_returns == 0);
				REQUIRE(HubCargoTransfer::Consume(center->goods[miner->cargo_type].GetOrCreateData().cargo, 1024));
				REQUIRE(Command<Commands::ReleaseOrbitalCargo>::Post(miner->index, miner->generation, center_ref.station, center_ref.generation, shell.pod, shell.generation, uint16_t{10}));
				CHECK(company->money == money); CHECK(miner->cargo.TotalCount() == 10); CHECK(miner->manifest->pending_returns == 1); CHECK(miner->manifest->returns_remaining == 2);
				CHECK_FALSE(CanReleaseOrbitalCargo(GetSpaceCraftReference(*miner), center_ref, shell, 10)); advance(37); save("descent-partial-414.sav");
			}
			if (const char *corrupt = std::getenv("OTTD_SPACE_DESCENT_CORRUPT")) {
				const auto digest = ComputeSpaceVehicleDigest(); REQUIRE(SaveOrLoad(source.string(), SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Ok);
				CHECK(SaveOrLoad(corrupt, SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Error);
				REQUIRE(SaveOrLoad(source.string(), SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Ok); CHECK(ComputeSpaceVehicleDigest() == digest); fmt::print("SPACE_DESCENT preview_reject=true authority_unchanged=true\n");
			}
			if (action != "service") {
				REQUIRE(CaptureSpaceDescents().size() == 1); const auto first = CaptureSpaceDescents().front(); REQUIRE(first.remaining_ticks == 111);
				const auto second_shell = miner->manifest->pods.front(); REQUIRE(Command<Commands::ReleaseOrbitalCargo>::Post(miner->index, miner->generation, center_ref.station, center_ref.generation, second_shell.pod, second_shell.generation, uint16_t{10}));
				CHECK(miner->manifest->pending_returns == 2); CHECK(miner->manifest->returns_remaining == 2); CHECK(CmdServiceLaunchProbe({}, miner->index, miner->generation, false).Failed());
				const auto old = center_ref; const auto anchor = center->launch_center->tile; const auto rotation = center->launch_center->rotation;
				ForgetLaunchCenter(*center); REQUIRE(ValidateSpaceVehicles()); advance(9); CHECK(ResolveSpacePod(first.pod)->descent->remaining_ticks == 111);
				InstallNewLaunchCenter(*center, anchor, rotation); center_ref = GetLaunchCenterReference(*center); CHECK(center_ref != old);
				for (auto *v : SpaceVehicle::Iterate()) if (v->construction.placement == SpacePlacement::Unhomed) REQUIRE(Command<Commands::RecoverLaunchCraft>::Post(center_ref.station, center_ref.generation, v->index, v->generation));
				for (auto *pod : SpacePod::Iterate()) if (pod->state.custody == SpacePodCustody::Unhomed) REQUIRE(Command<Commands::RecoverLaunchPod>::Post(center_ref.station, center_ref.generation, pod->index, pod->generation));
				for (const auto &r : CaptureSpaceDescents()) {
					CHECK(CmdRetargetSpaceDescent({}, r.pod.pod, r.pod.generation, old.station, old.generation).Failed());
					REQUIRE(Command<Commands::RetargetSpaceDescent>::Post(r.pod.pod, r.pod.generation, center_ref.station, center_ref.generation));
				}
				advance(111); CHECK(miner->manifest->pending_returns == 1); CHECK(miner->manifest->returns_remaining == 1); CHECK(CmdServiceLaunchProbe({}, miner->index, miner->generation, false).Failed());
				advance(37); CHECK(miner->manifest->pending_returns == 0); CHECK(miner->manifest->returns_remaining == 0); CHECK(miner->manifest->pods.empty());
				CHECK(ResolveSpacePod(first.pod) == nullptr); CHECK(ResolveSpacePod(second_shell) == nullptr); CHECK(SpacePod::GetNumItems() == 8);
				REQUIRE(Command<Commands::ServiceLaunchProbe>::Post(miner->index, miner->generation, false));
				const auto profit = miner->profit_this_year; const auto remainder = miner->mission.cost_remainder;
				CHECK(IsSpaceServiceWorkTick(*miner)); advance(37);
				CHECK(miner->profit_this_year == profit - Money{250} * 256); CHECK(miner->mission.cost_remainder == remainder);
				save("descent-service-414.sav");
			}
			REQUIRE(miner->manifest->service_ticks == 111); CHECK(miner->manifest->service_location == SpaceServiceLocation::Onboard);
			const auto profit = miner->profit_this_year; const auto remainder = miner->mission.cost_remainder;
			CHECK(IsSpaceServiceWorkTick(*miner)); advance(111);
			CHECK(miner->profit_this_year == profit - Money{750} * 256); CHECK(miner->mission.cost_remainder == remainder); CHECK_FALSE(IsSpaceServiceWorkTick(*miner));
			CHECK(miner->manifest->returns_remaining == 2); CHECK(miner->manifest->pending_returns == 0); CHECK(miner->manifest->services_completed == 1);
			CHECK(miner->manifest->supplies[1].TotalCount() == 0); CHECK(miner->manifest->supplies[2].TotalCount() == 0); CHECK(miner->cargo.TotalCount() == 0);
			CHECK(center->goods[miner->cargo_type].TotalCount() == 20); CHECK(CaptureSpaceDescents().empty()); REQUIRE(ValidateSpaceVehicles()); save("descent-complete-414.sav");
			uint64_t extracted = 0, packets = 0;
			for (const auto &body : GetAsteroidField().bodies) extracted += body.initial_reserve - body.remaining_reserve;
			for (const auto *cp : CargoPacket::Iterate()) if (cp->GetSource().type == SourceType::Asteroid) packets += cp->Count();
			CHECK(extracted == 20); CHECK(packets == 20);
			SpacePod *stored = nullptr; for (auto *pod : SpacePod::Iterate()) if (pod->state.custody == SpacePodCustody::CenterStored) { stored = pod; break; } REQUIRE(stored != nullptr);
			REQUIRE(Command<Commands::MoveLaunchPod>::Post(center_ref.station, center_ref.generation, rocket->index, rocket->generation, stored->index, stored->generation, false));
			REQUIRE(Command<Commands::AssembleLaunchPod>::Post(center_ref.station, center_ref.generation));
			const auto replacement = center->launch_operations->pod_workshop; CHECK(replacement.pod == SpacePodID{0}); CHECK(replacement.generation > 10);
			CHECK(ResolveSpacePod({replacement.pod, uint64_t{1}}) == nullptr); CHECK(CmdRetargetSpaceDescent({}, replacement.pod, uint64_t{1}, center_ref.station, center_ref.generation).Failed());
			fmt::print("SPACE_DESCENT native=true conservation=true target_loss=true service_once=true digest={}\n", ComputeSpaceVehicleDigest());
		} catch (...) { failure = std::current_exception(); }
		observed = true; _exit_game = true;
	};
	AutoRestoreBackup hook(descent_observer, &observe); IConsole::CmdRegister("space_descent_test", ObserveDescent);
	const std::string source_path = source.string(), config_path = config.string(); std::array<std::string_view, 17> args = {"openttd_test.exe", "-v", "null:ticks=2", "-b", "null", "-s", "null", "-m", "null", "-x", "-X", "-I", "original_dos", "-c", config_path, "-g", source_path};
	CHECK(openttd_main(args) == 0); if (failure) std::rethrow_exception(failure); CHECK(observed);
}
