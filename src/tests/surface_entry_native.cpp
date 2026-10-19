/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file surface_entry_native.cpp Pure native factory/launch queries in an authentic starting world.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15BB.
 * The original input comes from ordinary New Game. This observer never supplies
 * money, floor, packets, facilities, construction progress or vehicles.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../console_internal.h"
#include "../company_base.h"
#include "../company_func.h"
#include "../industry.h"
#include "../industrytype.h"
#include "../industry_cmd.h"
#include "../cargotype.h"
#include "../cargopacket.h"
#include "../layer_industry_baseline.h"
#include "../layer_industry_processor.h"
#include "../launch_center_cmd.h"
#include "../multilayer_recipe.h"
#include "../station_type.h"
#include "../tile_map.h"
#include "../openttd.h"
#include "../core/backup_type.hpp"
#include "../core/random_func.hpp"
#include "../saveload/saveload_func.h"
#include "layer_save_fixture.h"
#include "test_artifact_paths.h"
#include "../safeguards.h"

static std::function<void()> *surface_entry_observer = nullptr; ///< Non-owning current invocation callback, never a gameplay controller.

/** Execute this invocation's actual post-load pure inspection.
 * @param args Parameter-free registered native console command.
 * @return True after the current original world was inspected.
 */
static bool ObserveSurfaceEntry(std::span<std::string_view> args)
{
	REQUIRE(args.size() == 1); REQUIRE(surface_entry_observer != nullptr);
	(*surface_entry_observer)(); return true;
}

/** Serialize actual native owners without simulation work or a financial mutation.
 * @param path Fresh invocation-owned output.
 */
static void SaveSurfaceEntry(const std::filesystem::path &path)
{
	AutoRestoreBackup pause(_pause_mode, PauseModes{PauseMode::Normal});
	AutoRestoreBackup compression(_savegame_format, std::string{"lzma"});
	REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(path), false) == SaveLoadResult::Ok);
}

/** Read one actual configured cargo's label for an independent route report.
 * @param cargo Runtime native slot, already validated by its definition owner.
 * @return Exact four original label characters, never a second cargo registry.
 */
static std::string SurfaceEntryLabel(CargoType cargo)
{
	const auto label = CargoSpec::Get(cargo)->label;
	return {char(label[0]), char(label[1]), char(label[2]), char(label[3])};
}

/** Identify the enabled original Surface factory converting Steel into Goods.
 * @return Existing definition identity; no facility or construction state is created.
 */
static IndustryType SurfaceEntryGoodsType()
{
	const auto steel = GetCargoTypeByLabel(CargoLabel{"STEL"}), goods = GetCargoTypeByLabel(CargoLabel{"GOOD"});
	// The actual preserved original definition owns this conversion. A foreign
	// callback or a same-name facility cannot provide the selected input route.
	for (IndustryType type = 0; type < NUM_INDUSTRYTYPES; ++type) {
		const auto &spec = *GetIndustrySpec(type);
		if (!spec.enabled) continue;
		if (spec.grf_prop.HasGrfFile()) continue;
		if (std::ranges::find(spec.accepts_cargo, steel) == spec.accepts_cargo.end()) continue;
		if (std::ranges::find(spec.produced_cargo, goods) != spec.produced_cargo.end()) return type;
	}
	return IT_INVALID;
}

/** Query a real buildable Surface factory location and its owning command's price.
 * @param family Stable report role, not a runtime industry identity.
 * @param type Actual enabled original/native definition.
 */
static void QuerySurfaceEntryFactory(std::string_view family, IndustryType type)
{
	REQUIRE(type != IT_INVALID);
	const auto &spec = *GetIndustrySpec(type);
	TileIndex anchor = INVALID_TILE; CommandCost quote;
	// Search actual existing empty Surface terrain only. Each candidate uses
	// the production placement/availability query; no floor or facility is built
	// to compensate for an unsuitable original world.
	for (TileIndex tile : MultiLayerMap::IterateLayer(MapLayer::Surface)) {
		if (!IsTileType(tile, TileType::Clear)) continue;
		const auto candidate = CmdBuildIndustry({}, tile, type, 0, true, 123);
		if (candidate.Failed()) continue;
		anchor = tile; quote = candidate; break;
	}
	REQUIRE(anchor != INVALID_TILE);
	fmt::print("SURFACE_ENTRY_FACTORY family={} type={} anchor={} cost={}\n", family, type, anchor.base(), quote.GetCost().base());
	// Record exact active input/output meanings and multipliers. Native custom
	// processors also publish their all-input recipe separately below; their
	// disabled independent-input multipliers cannot be mistaken for production.
	for (size_t input = 0; input < spec.accepts_cargo.size(); ++input) {
		if (!IsValidCargoType(spec.accepts_cargo[input])) continue;
		for (size_t output = 0; output < spec.produced_cargo.size(); ++output) {
			if (!IsValidCargoType(spec.produced_cargo[output])) continue;
			fmt::print("SURFACE_ENTRY_CONVERSION family={} input={} output={} multiplier={}\n", family,
				SurfaceEntryLabel(spec.accepts_cargo[input]), SurfaceEntryLabel(spec.produced_cargo[output]), spec.input_cargo_multiplier[input][output]);
		}
	}
}

/** Actual complete-loader inspection, with pure commands and unchanged original native owners. */
TEST_CASE("Authentic Surface world exposes pure factory fuel and launch-entry contracts", "[.surface-entry-native]")
{
	const char *source = std::getenv("OTTD_SURFACE_ENTRY_SOURCE"); REQUIRE(source != nullptr);
	const auto paths = GetTestCaseArtifactPaths("space", "surface-entry-queries");
	const auto config = GetTestRuntimeConfig(); bool observed = false; std::exception_ptr failure;
	/** Inspect after normal reconstruction, and preserve assertion exceptions through ordinary native shutdown. */
	std::function<void()> inspect = [&] {
		try {
			AutoRestoreBackup actor(_current_company, _local_company);
			REQUIRE(Company::IsValidID(_current_company));
			const auto *company = Company::Get(_current_company);
			REQUIRE(company->money == 100000); REQUIRE(company->current_loan == 100000);
			const auto random = _random;
			const auto industries = Industry::GetNumItems(), packets = CargoPacket::GetNumItems();
			SaveSurfaceEntry(paths.saves / "surface-entry-before-queries-440.sav");
			QuerySurfaceEntryFactory("STEL", GetLayerBaselineReceiverType(CargoLabel{"IORE"}));
			QuerySurfaceEntryFactory("GOOD", SurfaceEntryGoodsType());
			QuerySurfaceEntryFactory("CHEM", GetLayerProcessorIndustryType(MultiLayerRecipeID{"CHEM"}));
			TileIndex launch = INVALID_TILE; CommandCost launch_quote; uint8_t launch_rotation = 0;
			std::map<StringID, uint32_t> launch_refusals;
			// The launch owner must find real permitted Surface ground itself;
			// a current catalogue entry alone cannot authorize a prefab facility.
			for (TileIndex tile : MultiLayerMap::IterateLayer(MapLayer::Surface)) {
				// Every real orientation is queried before classifying this world
				// as needing paid terrain work; a fixed camera-facing footprint
				// cannot establish that no ordinary launch placement is possible.
				for (uint8_t rotation = 0; rotation < 4; ++rotation) {
					const auto candidate = CmdBuildLaunchCenter({}, tile, rotation, NEW_STATION, false);
					if (candidate.Failed()) { ++launch_refusals[candidate.GetErrorMessage()]; continue; }
					launch = tile; launch_quote = candidate; launch_rotation = rotation; break;
				}
				if (launch != INVALID_TILE) break;
			}
			fmt::print("SURFACE_ENTRY_LAUNCH found={} anchor={} rotation={} cost={} maximum_loan={}\n", launch != INVALID_TILE,
				launch.base(), launch_rotation, launch_quote.GetCost().base(), company->GetMaxLoan().base());
			// Native refusals are required route information, not permission to
			// invent a launch slab. Later terrain work must pay its own commands.
			for (const auto &[reason, count] : launch_refusals) fmt::print("SURFACE_ENTRY_LAUNCH_REFUSAL reason={} count={}\n", reason.base(), count);
			// Only actual selected Surface fuel recipes describe the missing
			// raw-material chain. This publishes no stock, work or procurement.
			for (const auto &record : GetMultiLayerManufacturingRecipes()) {
				if (record.layer != MapLayer::Surface) continue;
				if (record.facility != "Chemical works") continue;
				for (const auto &input : record.recipe.inputs) {
					fmt::print("SURFACE_ENTRY_FUEL_INPUT recipe={}{}{}{} cargo={} quantity={}\n", char(record.recipe.id[0]), char(record.recipe.id[1]), char(record.recipe.id[2]), char(record.recipe.id[3]),
						SurfaceEntryLabel(GetCargoTypeByLabel(input.label)), input.quantity);
				}
			}
			CHECK(company->money == 100000); CHECK(company->current_loan == 100000);
			CHECK(Industry::GetNumItems() == industries); CHECK(CargoPacket::GetNumItems() == packets);
			CHECK(_random.state[0] == random.state[0]); CHECK(_random.state[1] == random.state[1]);
			SaveSurfaceEntry(paths.saves / "surface-entry-after-queries-440.sav");
			fmt::print("SURFACE_ENTRY_QUERIES_COMPLETE original_company=true pure=true\n"); observed = true;
		} catch (...) { failure = std::current_exception(); }
		_pause_mode.Set(PauseMode::Normal); _exit_game = true;
	};
	AutoRestoreBackup observer(surface_entry_observer, &inspect);
	IConsole::CmdRegister("surface_entry_inspect", ObserveSurfaceEntry);
	const std::string config_arg = config.string();
	std::array<std::string_view, 17> args = {"openttd_surface_supply_test.exe", "-v", "null:ticks=2", "-b", "null", "-s", "null", "-m", "null", "-x", "-X", "-I", "original_dos", "-c", config_arg, "-g", source};
	CHECK(openttd_main(args) == 0); if (failure) std::rethrow_exception(failure); CHECK(observed);
}
