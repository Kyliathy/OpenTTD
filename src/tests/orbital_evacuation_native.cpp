/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file orbital_evacuation_native.cpp Actual paid service/assembly cancellation with conserved packet provenance and native hull ownership.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Groups5B/5C.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../space_structure.h"
#include "../space_structure_cmd.h"
#include "../space_approach.h"
#include "../space_vehicle.h"
#include "../space_manifest.h"
#include "../orbital_service.h"
#include "../company_base.h"
#include "../company_func.h"
#include "../command_func.h"
#include "../console_internal.h"
#include "../core/backup_type.hpp"
#include "../core/random_func.hpp"
#include "../map_func.h"
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
#include <map>
#include <tuple>
#include "../safeguards.h"

/** Read-only provenance grouping allows legitimate native packet merging while requiring exact physical quantity and feeder-share conservation. */
using EvacuationCargoKey = std::tuple<uint8_t, uint8_t, uint32_t, uint16_t, uint32_t, uint16_t, uint16_t>;
/** Transient comparison totals; they own no packets or gameplay inventory and are never installed into the world. */
using EvacuationCargoTotals = std::map<EvacuationCargoKey, std::pair<uint64_t, Money>>;

/** Capture all actual packet owners in two sites, grouped by unchanged physical provenance rather than packet IDs that may legitimately merge.
 * @param source Actual source stock/jobs. @param destination Actual destination stock/jobs.
 * @return Exact quantity/feeder-share totals by cargo/source/first-station/tile/age/routing metadata.
 */
static EvacuationCargoTotals CaptureEvacuationCargo(const SpaceStructure &source, const SpaceStructure &destination)
{
	EvacuationCargoTotals totals;
	/** @param cargo Exact resolved physical label. @param stock One actual sole native owner to read. */
	auto append = [&](CargoType cargo, const StationCargoList &stock) {
		// Merge is allowed only within matching provenance; every retained fragment contributes its complete count and feeder share once.
		for (const auto &[hop, packets] : *stock.Packets()) for (const auto *packet : packets) {
			const auto origin = packet->GetSource();
			const EvacuationCargoKey key{cargo, to_underlying(origin.type), static_cast<uint32_t>(origin.id), packet->GetFirstStation().base(),
				packet->GetSourceTile().base(), packet->GetPeriodsInTransit(), hop.base()};
			auto &entry = totals[key]; entry.first += packet->Count(); entry.second += packet->GetFeederShare();
		}
	};
	// Include destination-held jobs as well, so evacuation cannot accidentally cancel or spend an unrelated service reservation there.
	for (const auto *site : {&source, &destination}) {
		const auto &definition = *GetSpaceConstructionDefinition(site->state.kind);
		for (uint i = 0; i < site->job.inputs.size(); ++i) append(GetCargoTypeByLabel(definition.inputs[i].label), site->job.inputs[i]);
		if (!site->module) continue;
		for (CargoType cargo : EnumRange(NUM_CARGO)) append(cargo, site->module->stock[cargo]);
		for (const auto *job : site->module->GetServiceJobs()) for (uint i = 0; i < job->inputs.size(); ++i) append(GetSpaceSupplyCargo(static_cast<SpaceSupply>(i + 1)), job->inputs[i]);
	}
	return totals;
}

static std::function<void()> *evacuation_observer = nullptr;
/** @param args Native console invocation. @return Whether the installed actual-world observer ran. */
static bool ObserveEvacuation(std::span<std::string_view> args)
{
	REQUIRE(args.size() == 1); REQUIRE(evacuation_observer != nullptr); (*evacuation_observer)(); return true;
}

/** Genuine paid checkpoints supply both held service pairs and partial assembly; actual public cancellation/transfers preserve every original owner. */
TEST_CASE("Native module evacuation preserves all unconsumed cargo and never refunds work or moves a hull", "[.orbital-evacuation]")
{
	const char *mode = std::getenv("OTTD_ORBITAL_EVACUATION_ACTION"), *input = std::getenv("OTTD_ORBITAL_EVACUATION_SOURCE");
	REQUIRE(mode != nullptr); REQUIRE(input != nullptr); const std::string_view action{mode};
	const auto paths = GetTestCaseArtifactPaths("space", "orbital-evacuation"); const auto config = GetTestRuntimeConfig();
	bool observed = false; std::exception_ptr failure;
	/** Observe only actual native owners and commands, carrying failures through ordinary game shutdown. */
	std::function<void()> observe = [&] {
		try {
			AutoRestoreBackup actor(_current_company, _local_company); REQUIRE(ValidateSpaceVehicles());
			auto &destination = *SpaceStructure::Get(SpaceStructureID{0}); REQUIRE(IsOrbitalModuleOperational(destination));
			SpaceStructure *source = nullptr;
			if (action == "construction") source = SpaceStructure::Get(SpaceStructureID{1});
			else {
				REQUIRE(action == "service");
				for (auto *site : SpaceStructure::Iterate()) if (site->state.kind == SpaceBuildKind::Dock) source = site;
			}
			REQUIRE(source != nullptr); const auto material = CaptureEvacuationCargo(*source, destination);
			const auto before = ComputeSpaceStructureDigest(), power = ComputeUtilityPowerDigest(), manifests = ComputeSpaceManifestDigest();
			const auto cash = Company::Get(_current_company)->money; const auto random = _random;
			const auto work = static_cast<const OrbitalConstructionState &>(source->job); const auto receipts = GetSpaceStructureRetirementHistory();
			std::vector<std::tuple<SpaceCraftRef, SpacePoint, CargoType, uint, Money>> hulls;
			// Capture personal holds/positions/profits separately from navigation, because cancelled construction loses only its exact now-invalid target claim.
			for (const auto *craft : SpaceVehicle::Iterate()) hulls.emplace_back(GetSpaceCraftReference(*craft), craft->position, craft->cargo_type, craft->cargo.TotalCount(), craft->profit_this_year);
			CHECK(CmdEvacuateOrbitalModule({}, source->index, source->generation, destination.index, destination.generation).Succeeded());
			CHECK(CmdEvacuateOrbitalModule({}, source->index, source->generation + 1, destination.index, destination.generation).Failed());
			CHECK(CmdEvacuateOrbitalModule({}, source->index, source->generation, destination.index, destination.generation + 1).Failed());
			CHECK(CmdEvacuateOrbitalModule({}, source->index, source->generation, source->index, source->generation).Failed());
			{ AutoRestoreBackup foreign(_current_company, COMPANY_SPECTATOR); CHECK(CmdEvacuateOrbitalModule({}, source->index, source->generation, destination.index, destination.generation).Failed()); }
			CHECK(ComputeSpaceStructureDigest() == before); CHECK(ComputeUtilityPowerDigest() == power); CHECK(ComputeSpaceManifestDigest() == manifests);
			CHECK(Company::Get(_current_company)->money == cash); CHECK(_random.state[0] == random.state[0]); CHECK(_random.state[1] == random.state[1]);
			if (action == "service") {
				REQUIRE(source->module->service.remaining_ticks != 0); REQUIRE(source->module->dock_service.remaining_ticks != 0);
				CHECK(ClassifyOrbitalModuleRemoval(*source, _current_company) == OrbitalRemovalBlock::Service);
			} else {
				REQUIRE(source->state.phase == SpaceStructurePhase::Blueprint); REQUIRE(source->job.work_ticks != 0);
				CHECK(ClassifyOrbitalModuleRemoval(*source, _current_company) == OrbitalRemovalBlock::Construction);
			}
			CHECK(CmdRemoveOrbitalModule({DoCommandFlag::Execute}, source->index, source->generation).Failed()); CHECK(ComputeSpaceStructureDigest() == before);
			REQUIRE(Command<Commands::EvacuateOrbitalModule>::Post(source->index, source->generation, destination.index, destination.generation));
			CHECK(CaptureEvacuationCargo(*source, destination) == material); CHECK(ComputeUtilityPowerDigest() == power); CHECK(ComputeSpaceManifestDigest() == manifests);
			CHECK(GetSpaceStructureRetirementHistory() == receipts); CHECK(source->job.work_ticks == work.work_ticks); CHECK(source->job.work_fraction == work.work_fraction);
			CHECK(source->job.work_remainder == work.work_remainder); CHECK(source->job.last_work_tick == work.last_work_tick); CHECK(source->job.last_work_interval == work.last_work_interval);
			CHECK(source->job.inputs[0].TotalCount() == 0); CHECK(source->job.inputs[1].TotalCount() == 0);
			// Every actual hull retains its own physical position and freight; only later explicit native orders may vacate a completed service module.
			for (const auto &[ref, position, cargo, quantity, profit] : hulls) {
				const auto *craft = ResolveSpaceCraft(ref); REQUIRE(craft != nullptr); CHECK(craft->position == position); CHECK(craft->cargo_type == cargo);
				CHECK(craft->cargo.TotalCount() == quantity); CHECK(craft->profit_this_year == profit);
			}
			if (action == "service") {
				CHECK(source->module->service.remaining_ticks == 0); CHECK(source->module->dock_service.remaining_ticks == 0);
				CHECK(ClassifyOrbitalModuleRemoval(*source, _current_company) == OrbitalRemovalBlock::Vehicle);
				CHECK(CmdRemoveOrbitalModule({DoCommandFlag::Execute}, source->index, source->generation).Failed());
			} else {
				CHECK(source->state.phase == SpaceStructurePhase::Cancelled);
				// A cancelled empty construction producer may retain physical hulls without a stale saved approach claim or a reusable current action.
				for (const auto *craft : SpaceVehicle::Iterate()) {
					if (craft->space_orders.cursor >= craft->space_orders.entries.size()) continue;
					const auto &target = craft->space_orders.entries[craft->space_orders.cursor].target;
					if (target.kind == SpaceOrderTargetKind::Blueprint && target.id == source->index.base() && target.generation == source->generation) {
						CHECK(craft->space_orders.blocked == SpaceOrderBlock::TargetRemoved); CHECK_FALSE(craft->space_orders.approach);
					}
				}
			}
			CHECK(Company::Get(_current_company)->money == cash); CHECK(_random.state[0] == random.state[0]); CHECK(_random.state[1] == random.state[1]);
			REQUIRE(ValidateSpaceVehicles());
			REQUIRE(BlitterFactory::SelectBlitter("8bpp-simple") != nullptr); GfxClearSpriteCache(); FontCache::ClearFontCaches(FONTSIZES_REQUIRED);
			ShowSpaceConstructionWindow(GetSpaceStructureReference(*source)); auto *window = FindWindowById(WindowClass::SpaceConstruction, 0); REQUIRE(window != nullptr);
			CaptureNativeArtWindow(*window, paths.images / "module-evacuated.ppm");
			AutoRestoreBackup pause(_pause_mode, PauseModes{PauseMode::Normal}); AutoRestoreBackup compression(_savegame_format, std::string{"lzma"});
			REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(paths.saves / "module-evacuated-422.sav"), false) == SaveLoadResult::Ok);
			fmt::print("ORBITAL_EVACUATION native=true phase={} actual_paid_inputs=true provenance_conserved=true no_work_refund=true no_hull_teleport=true\n", action);
		} catch (...) { failure = std::current_exception(); }
		observed = true; _exit_game = true;
	};
	AutoRestoreBackup hook(evacuation_observer, &observe); IConsole::CmdRegister("orbital_evacuation_test", ObserveEvacuation);
	const std::string source{input}, configuration = config.string();
	std::array<std::string_view, 17> args = {"openttd_test.exe", "-v", "null:ticks=2", "-b", "null", "-s", "null", "-m", "null", "-x", "-X", "-I", "original_dos", "-c", configuration, "-g", source};
	CHECK(openttd_main(args) == 0); if (failure) std::rethrow_exception(failure); CHECK(observed);
}
