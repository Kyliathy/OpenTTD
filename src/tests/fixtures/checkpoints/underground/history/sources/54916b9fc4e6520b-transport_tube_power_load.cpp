/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file transport_tube_power_load.cpp Actual native paid inlet/grant/use/recovery files, preserving the original paid excavation world. @see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Groups5-7 */

#include "../stdafx.h"
#include "test_artifact_paths.h"
#include <cstdlib>
#include <filesystem>
#include "../3rdparty/catch2/catch.hpp"
#include "transport_tube_power_fixture.h"
#include "layer_save_fixture.h"
#include "../openttd.h"
#include "../console_internal.h"
#include "../saveload/saveload_func.h"
#include "../fios.h"

#include "../safeguards.h"

static std::function<void()> *_tube_power_observer = nullptr; ///< Isolated accepted-game observer; ordinary loader and saved feature owners remain unchanged.

/** @param arguments Exact zero-argument fixture startup call. @return Whether its actual loaded-world observer executed. */
static bool ObserveTubePower(std::span<std::string_view> arguments)
{
	REQUIRE(arguments.size() == 1); REQUIRE(_tube_power_observer != nullptr);
	(*_tube_power_observer)(); return true;
}

/** Real files establish physical and invoice persistence, independently of the copied unit-test staging boundary. */
TEST_CASE("Native paid tube connection grant and fractional invoice survive actual files", "[.transport-tube-power-file]")
{
	const auto root = FindTestRepositoryRoot();
	const auto directory = GetTestCaseArtifactPaths("transport-tubes", "native-reload").saves;
	const char *input_text = std::getenv("OTTD_TUBE_POWER_INPUT"), *mode_text = std::getenv("OTTD_TUBE_POWER_CASE");
	REQUIRE(input_text != nullptr); REQUIRE(mode_text != nullptr);
	const auto input = ResolveTestInputPath(input_text); const std::string_view mode{mode_text};
	REQUIRE(std::filesystem::exists(input)); REQUIRE(std::filesystem::exists(directory));
	bool observed = false; std::exception_ptr failure;
	/** Inspect or construct service only after complete ordinary-world acceptance; no geology or private ledger write supplies it. */
	std::function<void()> inspect = [&] {
		try {
			REQUIRE(GetCurrentMultiLayerProfileSelection().id == MultiLayerGameplayProfileID::MultilayerIndustry);
			AutoRestoreBackup actor(_current_company, _local_company);
			/** @param name Fixed test-owned output basename. Save the real world without refreshing grants, service or any invoice. */
			const auto save = [&](std::string_view name) {
				const auto ledger = ComputeUtilityPowerDigest(), physical = ComputeTubePowerDigest();
				AutoRestoreBackup compression(_savegame_format, std::string{"lzma"});
				const char *output_prefix = std::getenv("OTTD_TUBE_POWER_OUTPUT_PREFIX");
				const auto output = output_prefix == nullptr ? std::string{name} : std::string{output_prefix} + std::string{name};
				REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(directory / output), false) == SaveLoadResult::Ok);
				CHECK(ComputeUtilityPowerDigest() == ledger); CHECK(ComputeTubePowerDigest() == physical);
			};
			if (mode == "create") {
				REQUIRE(CaptureTubePowerConnections().empty()); REQUIRE_FALSE(HasUtilityPowerState());
				const auto tube = InitialiseTubeRailType(_grfconfig);
				REQUIRE(tube != INVALID_RAILTYPE); REQUIRE(InitialiseTubeEngineDefinitions(_grfconfig));
				Company::Get(_current_company)->avail_railtypes.Set(tube);
				Engine::Get(GetTubeEngine(9))->company_avail.Set(_current_company);
				TileIndex endpoint = INVALID_TILE, depot = INVALID_TILE;
				DiagDirection depot_direction = DiagDirection::Invalid;
				std::vector<TileIndex> wire;
				// Select only an actual Connected Underground approach with nearby
				// already paid excavated floor. No cell is opened by this observer.
				for (const auto &record : CaptureLayerPortalWorkRecords()) {
					if (record.company != _current_company || record.state != LayerPortalWorkState::Connected) continue;
					const auto rock = GetLayerPortalPairTile(record.pair_key);
					if (GetTileLayer(rock) != MapLayer::Underground) continue;
					const auto door = GetLayerPortalDoorTile(rock);
					std::vector<std::vector<TileIndex>> candidates{{door}};
					std::set<TileIndex> visited{door};
					// Search only bounded already excavated owned/local floor. Real
					// depot approach commands decide which chamber geometry fits;
					// service setup never clears rock or releases a cutter's support.
					for (size_t index = 0; index < candidates.size() && index < 512; ++index) {
						const auto path = candidates[index]; const auto candidate = path.back();
						if (IsTileType(candidate, TileType::Clear)) {
							for (const auto direction : EnumRange(DiagDirection::End)) {
								if (CmdBuildTrainDepot({}, candidate, tube, direction).Failed()) continue;
								endpoint = rock; depot = candidate; depot_direction = direction; wire = path; break;
							}
						}
						if (depot != INVALID_TILE) break;
						if (path.size() >= MAX_TUBE_POWER_PATH) continue;
						// Adjacent floor adds one physical utility segment; occupied
						// foreign infrastructure and solid rock cannot extend the wire.
						for (const auto direction : EnumRange(DiagDirection::End)) {
							const auto next = TileAddByDiagDir(candidate, direction);
							if (visited.contains(next)) continue;
							if (IsTileType(next, TileType::TunnelBridge)) continue;
							if (!IsTileType(next, TileType::Clear) && GetTileOwner(next) != _current_company.base()) continue;
							if (CheckTubeConstruction(std::array{next}).Failed()) continue;
							visited.insert(next); auto extended = path; extended.push_back(next); candidates.push_back(std::move(extended));
						}
					}
					if (depot != INVALID_TILE) break;
				}
				REQUIRE(depot != INVALID_TILE);
				REQUIRE(CmdBuildTrainDepot(DoCommandFlag::Execute, depot, tube, depot_direction).Succeeded());
				const auto [cost, id, capacity, mail, capacities] = CmdBuildVehicle(DoCommandFlag::Execute, depot, GetTubeEngine(9), false, INVALID_CARGO, ClientID::Invalid);
				REQUIRE(cost.Succeeded()); Train *capsule = Train::Get(id);
				const auto service = ConnectTubeExternalPower(endpoint, depot, wire);
				REQUIRE(service.error == TubePowerError::None);
				const std::array sources{service.producer};
				const auto consumer = RegisterUtilityPowerConsumer(_current_company, MapLayer::Underground, UtilityConsumerEffect::TubeWork, 39960, sources);
				REQUIRE(consumer.has_value());
				REQUIRE(PublishTubeExternalPower(service.producer, 1, 74) == TubePowerError::None);
				{
					AutoRestoreBackup system(_current_company, OWNER_DEITY);
					const std::array requests{UtilityPowerRequest{*consumer, 39960}};
					REQUIRE(AllocateUtilityPower(1, requests) == UtilityPowerError::None);
				}
				save("tube-power-granted.sav");
				CapsuleDepotService work(*capsule);
				REQUIRE(ConsumeTubeExternalPower(service.producer, *consumer, 39960, work).error == UtilityPowerError::None);
				save("tube-power-consumed.sav");
				REQUIRE(CloseTubeExternalPower(service.producer) == TubePowerError::None);
				save("tube-power-closed.sav");
			// Real binding/service continuation uses saved capsules and source
			// identities; service wait is verified independently of a route fixture.
			} else if (mode == "bind-service" || mode == "bound-service" || mode == "resume-service" || mode == "bound-wait" || mode == "wait-read") {
				const auto rows = CaptureTubePowerConnections(); REQUIRE(rows.size() == 1);
				const auto &row = rows.front();
				Train *capsule = nullptr;
				// Resolve the actual saved prototype front at its original depot;
				// the observer never invents vehicle or ledger identities.
				for (Train *train : Train::Iterate()) {
					if (train->IsFrontEngine() && train->tile == row.interchange && GetTubeVehicleCapability(train->engine_type).has_value()) capsule = train;
				}
				REQUIRE(capsule != nullptr);
				if (mode == "bind-service") {
					REQUIRE(CaptureTubePowerVehicles().empty());
					REQUIRE(BindTubePowerVehicle(capsule->index, row.producer).has_value());
				} else {
					REQUIRE(CaptureTubePowerVehicles().size() == 1);
					CHECK(ValidateTubePowerVehicles(CaptureTubePowerVehicles()) == TubePowerError::None);
					CHECK(GetUtilityPowerProducer(row.producer)->consumed_lifetime_j == 40500);
					CHECK(row.usage_remainder_j == (mode == "wait-read" ? 0 : 500));
				}
				// Initial/resumed service and source loss each advance one actual
				// native tick; pure file readers renew neither work nor allocation.
				if (mode != "bound-service" && mode != "wait-read") {
					capsule->reliability = 100;
					if (mode == "bound-wait") REQUIRE(CloseTubeExternalPower(row.producer) == TubePowerError::None);
					++TimerGameTick::counter; PrepareTubePowerTick();
					REQUIRE(capsule->Tick());
					CHECK(capsule->vehstatus.Test(VehState::Stopped));
					if (mode == "bound-wait") {
						CHECK(capsule->reliability == 100);
						CHECK(GetTubePowerVehicle(capsule->index)->wait == TubePowerWait::Topology);
						CHECK(GetUtilityPowerProducer(row.producer)->consumed_lifetime_j == 40500);
					} else {
						CHECK(capsule->reliability == capsule->GetEngine()->reliability);
						CHECK(GetUtilityPowerProducer(row.producer)->consumed_lifetime_j == (mode == "bind-service" ? 40500 : 41040));
					}
				}
				if (mode == "wait-read") {
					CHECK_FALSE(GetUtilityPowerProducer(row.producer)->active);
					// Retained Group6 files name a missing grant; the topology-aware
					// successor names the retired source. Both are saved powerless
					// waits and stay unchanged until the next real shared request tick.
					CHECK((GetTubePowerVehicle(capsule->index)->wait == TubePowerWait::Grant ||
						GetTubePowerVehicle(capsule->index)->wait == TubePowerWait::Topology));
					CHECK(capsule->reliability == 100);
				}
				const auto before = ComputeUtilityPowerDigest(), bindings = ComputeTubePowerVehicleDigest();
				REQUIRE(capsule->Tick()); // Same actual saved epoch must not repeat paid native service.
				CHECK(ComputeUtilityPowerDigest() == before); CHECK(ComputeTubePowerVehicleDigest() == bindings);
				save(fmt::format("tube-meter-{}.sav", mode));
			// A real retained waiting world recovers through a new paid source and
			// consumer. Its old used-energy/invoice identities remain historical;
			// the ordinary reader neither renews that source nor repeats service.
			} else if (mode == "recover-source" || mode == "recovery-read") {
				const auto rows = CaptureTubePowerConnections();
				REQUIRE(rows.size() == (mode == "recover-source" ? 1 : 2));
				const auto original = rows.front();
				CHECK_FALSE(original.active);
				CHECK(GetUtilityPowerProducer(original.producer)->consumed_lifetime_j == 40500);
				Train *capsule = nullptr;
				// Resolve existing saved native custody without manufacturing a
				// vehicle or moving it to the new inlet's coordinate.
				for (Train *train : Train::Iterate()) {
					if (train->IsFrontEngine() && train->tile == original.interchange && GetTubeVehicleCapability(train->engine_type).has_value()) capsule = train;
				}
				REQUIRE(capsule != nullptr);
				if (mode == "recover-source") {
					const auto replacement = ConnectTubeExternalPower(original.portal, original.interchange, original.utility_path);
					REQUIRE(replacement.error == TubePowerError::None);
					REQUIRE(ReconnectTubePowerVehicle(capsule->index, replacement.producer).has_value());
					++TimerGameTick::counter; PrepareTubePowerTick(); REQUIRE(capsule->Tick());
				}
				REQUIRE(CaptureTubePowerVehicles().size() == 2);
				const auto *binding = GetTubePowerVehicle(capsule->index); REQUIRE(binding != nullptr);
				CHECK(binding->source != original.producer); CHECK(binding->wait == TubePowerWait::None);
				CHECK(capsule->IsStoppedInDepot()); CHECK(capsule->reliability == capsule->GetEngine()->reliability);
				CHECK(GetUtilityPowerProducer(binding->source)->consumed_lifetime_j == 540);
				CHECK(GetTubePowerConnection(binding->source)->usage_remainder_j == 540);
				CHECK(GetTubePowerConnection(original.producer)->invoiced_money == original.invoiced_money);
				CHECK(ValidateTubePowerConnections(CaptureTubePowerConnections(), GetCurrentMultiLayerProfileSelection()) == TubePowerError::None);
				CHECK(ValidateTubePowerVehicles(CaptureTubePowerVehicles()) == TubePowerError::None);
				const auto ledger = ComputeUtilityPowerDigest(), bodies = ComputeTubePowerVehicleDigest();
				REQUIRE(capsule->Tick()); CHECK(ComputeUtilityPowerDigest() == ledger); CHECK(ComputeTubePowerVehicleDigest() == bodies);
				save(fmt::format("tube-meter-{}.sav", mode));
			} else if (mode == "readiness") {
				const auto rows = CaptureTubePowerConnections(); REQUIRE_FALSE(rows.empty());
				const auto ledger = ComputeUtilityPowerDigest(), physical = ComputeTubePowerDigest(), bindings = ComputeTubePowerVehicleDigest();
				// Inspect actual accepted saved facilities/inlets without advancing
				// the simulation or renewing an old source, consumer or work offer.
				for (const auto &row : rows) {
					const auto view = GetTubeNetworkReadiness(row.interchange, row.owner); REQUIRE(view.has_value());
					CHECK(view->terminal.tile == row.interchange); CHECK(view->terminal.layer == GetTileLayer(row.interchange));
					/** @param inlet Actual immutable reachable source. @return Whether this installed paid identity remains reachable. */
					const auto original_inlet = [&](const TubeInletReadiness &inlet) { return inlet.source == row.producer; };
					if (row.active) CHECK(std::ranges::any_of(view->inlets, original_inlet));
				}
				CHECK(ComputeUtilityPowerDigest() == ledger); CHECK(ComputeTubePowerDigest() == physical); CHECK(ComputeTubePowerVehicleDigest() == bindings);
				save("tube-readiness-loaded.sav");
			} else if (mode == "preview") {
				const auto ledger = ComputeUtilityPowerDigest(), physical = ComputeTubePowerDigest();
				const auto *corrupt_text = std::getenv("OTTD_TUBE_POWER_CORRUPT"); REQUIRE(corrupt_text != nullptr);
				const auto corrupt = std::filesystem::path{corrupt_text};
				CHECK(SaveOrLoad(corrupt.string(), SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Error);
				CHECK(ComputeUtilityPowerDigest() == ledger); CHECK(ComputeTubePowerDigest() == physical);
				REQUIRE(SaveOrLoad(input.string(), SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Ok);
				CHECK(_load_check_data.tube_power_rows.size() == 1);
				CHECK(_load_check_data.tube_vehicle_rows.size() == CaptureTubePowerVehicles().size());
				CHECK(ComputeUtilityPowerDigest() == ledger); CHECK(ComputeTubePowerDigest() == physical);
			} else {
				const auto rows = CaptureTubePowerConnections(); REQUIRE(rows.size() == 1);
				const auto &row = rows.front(); const auto *source = GetUtilityPowerProducer(row.producer); REQUIRE(source != nullptr);
				REQUIRE(GetTubeRailType() != INVALID_RAILTYPE); REQUIRE(GetTubeEngine(9) != EngineID::Invalid());
				CHECK(ValidateTubePowerConnections(rows, GetCurrentMultiLayerProfileSelection()) == TubePowerError::None);
				CHECK(row.active == (mode != "closed"));
				CHECK(row.usage_remainder_j == (mode == "consumed" ? 960 : 0));
				CHECK(row.invoiced_money == (mode == "granted" ? 20 : mode == "consumed" ? 59 : 60));
				CHECK(source->consumed_lifetime_j == (mode == "granted" ? 0 : 39960));
				CHECK(source->meter.generation_energy_j == 199800);
				save(fmt::format("tube-power-{}-reloaded.sav", mode));
			}
			fmt::print("TUBE_POWER_FILE case={} physical={:016X} ledger={:016X} actual_native=true\n", mode, ComputeTubePowerDigest(), ComputeUtilityPowerDigest());
		} catch (...) { failure = std::current_exception(); }
		observed = true; _exit_game = true;
	};
	AutoRestoreBackup observer(_tube_power_observer, &inspect);
	IConsole::CmdRegister("tube_power_check", ObserveTubePower);
	const std::string config = (GetTestRuntimeConfig()).string(), input_path = input.string();
	std::array<std::string_view, 17> arguments = {"Debug/openttd_test.exe", "-v", "null:ticks=2", "-b", "8bpp-simple", "-I", "original_dos",
		"-s", "null", "-m", "null", "-x", "-X", "-c", config, "-g", input_path};
	CHECK(openttd_main(arguments) == 0);
	if (failure) std::rethrow_exception(failure);
	CHECK(observed);
}
