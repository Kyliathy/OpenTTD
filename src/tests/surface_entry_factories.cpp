/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file surface_entry_factories.cpp Actual paid Surface factory funding and construction observation.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15BC.
 * The input is the real new-game company after native bank credit. Original
 * BuildIndustry owns invoices, identity and incomplete tiles. Only the ordinary
 * game loop may advance construction; this observer never sets progress/stock.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../console_internal.h"
#include "../company_base.h"
#include "../company_func.h"
#include "../command_func.h"
#include "../industry_cmd.h"
#include "../industry_map.h"
#include "../industrytype.h"
#include "../layer_industry_baseline.h"
#include "../cargopacket.h"
#include "../tile_map.h"
#include "../openttd.h"
#include "../core/backup_type.hpp"
#include "../saveload/saveload_func.h"
#include "layer_save_fixture.h"
#include "test_artifact_paths.h"
#include "../safeguards.h"

static std::function<void()> *surface_factory_observer = nullptr; ///< Current loaded-world callback, never a construction owner.

/** Dispatch the current observation through the normal loaded-game console.
 * @param args Parameter-free registered command arguments.
 * @return True after inspecting the loaded original world.
 */
static bool ObserveSurfaceFactories(std::span<std::string_view> args)
{
	REQUIRE(args.size() == 1); REQUIRE(surface_factory_observer != nullptr);
	(*surface_factory_observer)(); return true;
}

/** Save actual native owners without simulation work.
 * @param path Fresh invocation-owned output, never the original input.
 */
static void SaveSurfaceFactories(const std::filesystem::path &path)
{
	AutoRestoreBackup pause(_pause_mode, PauseModes{PauseMode::Normal});
	AutoRestoreBackup compression(_savegame_format, std::string{"lzma"});
	REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(path), false) == SaveLoadResult::Ok);
}

/** Resolve the original non-GRF factory definition for the actual selected input/output.
 * @param input Native cargo label accepted by this production chain.
 * @param output Native cargo label physically produced after delivery.
 * @return Existing definition identity; this does not construct a facility.
 */
static IndustryType FindSurfaceFactoryType(CargoLabel input, CargoLabel output)
{
	const auto from = GetCargoTypeByLabel(input), to = GetCargoTypeByLabel(output);
	// Preserve original production semantics: unrelated callback definitions
	// or facilities producing the same cargo from other inputs cannot substitute.
	for (IndustryType type = 0; type < NUM_INDUSTRYTYPES; ++type) {
		const auto &spec = *GetIndustrySpec(type);
		if (!spec.enabled) continue;
		if (spec.grf_prop.HasGrfFile()) continue;
		if (std::ranges::find(spec.accepts_cargo, from) == spec.accepts_cargo.end()) continue;
		if (std::ranges::find(spec.produced_cargo, to) != spec.produced_cargo.end()) return type;
	}
	return IT_INVALID;
}

/** Run the actual synchronized whole funding command, including ordinary company settlement.
 * @param anchor Explicit Surface candidate, freshly checked in this actual world.
 * @param type Original enabled factory definition.
 * @param estimate True leaves all native owners unchanged.
 * @return Original complete quote/invoice or unchanged refusal.
 */
static CommandCost FundSurfaceFactory(TileIndex anchor, IndustryType type, bool estimate)
{
	return Command<Commands::BuildIndustry>::Unsafe<CommandCallback>(INVALID_STRING_ID, nullptr, false, estimate,
		anchor, std::make_tuple(anchor, type, uint32_t{0}, true, uint32_t{123}));
}

/** Select a legal original factory site near its actual physical predecessor.
 * @param type Actual factory definition to fund.
 * @param predecessor Native raw provider or the newly paid Steel factory.
 * @return Closest permitted Surface anchor with deterministic tile tie breaking.
 */
static TileIndex SelectSurfaceFactorySite(IndustryType type, TileIndex predecessor)
{
	std::vector<TileIndex> candidates;
	for (TileIndex tile : MultiLayerMap::IterateLayer(MapLayer::Surface)) candidates.push_back(tile);
	/** Rank sites by future physical freight distance without editing terrain.
	 * @param tile Real candidate Surface anchor.
	 * @return Distance then native tile identity for deterministic selection.
	 */
	const auto rank = [predecessor](TileIndex tile) {
		return std::make_pair(std::abs(int(TileX(tile)) - int(TileX(predecessor))) +
			std::abs(int(TileY(tile)) - int(TileY(predecessor))), tile.base());
	};
	std::ranges::sort(candidates, {}, rank);
	// Placement and affordability remain the real command's decision. Search
	// empty terrain first to leave natural industries and town routes intact.
	for (TileIndex tile : candidates) {
		if (!IsTileType(tile, TileType::Clear)) continue;
		if (FundSurfaceFactory(tile, type, true).Succeeded()) return tile;
	}
	return INVALID_TILE;
}

/** Inspect the entire original facility footprint and its actual cargo owners.
 * @param industry Native funded industry, with founder/date owned by BuildIndustry.
 * @param complete Required readiness; false checks original zero-progress construction.
 */
static void CheckSurfaceFactory(const Industry &industry, bool complete)
{
	CHECK(industry.founder == _local_company); CHECK(GetTileLayer(industry.location.tile) == MapLayer::Surface);
	CHECK(IsLayerIndustryComplete(industry) == complete);
	size_t cells = 0;
	// Layout rectangles contain unused cells. Every machinery cell belonging to
	// this exact industry must meet readiness; a completed neighbour cannot count.
	for (TileIndex tile : industry.location) {
		if (!industry.TileBelongsToIndustry(tile)) continue;
		++cells; CHECK(IsIndustryCompleted(tile) == complete);
		if (!complete) {
			CHECK(GetIndustryConstructionStage(tile) == 0); CHECK(GetIndustryConstructionCounter(tile) == 0);
		}
	}
	REQUIRE(cells != 0);
	// Both new output factories must stay empty until later physical freight.
	// All histories count, because native months rotate current production rows.
	for (const auto &cargo : industry.produced) {
		CHECK(cargo.waiting == 0);
		for (const auto &history : cargo.history) { CHECK(history.production == 0); CHECK(history.transported == 0); }
	}
	for (const auto &cargo : industry.accepted) CHECK(cargo.waiting == 0);
	fmt::print("SURFACE_ENTRY_FACTORY_STATE id={} type={} anchor={} founder={} date={} cells={} complete={}\n",
		industry.index.base(), industry.type, industry.location.tile.base(), industry.founder.base(),
		industry.construction_date.base(), cells, complete);
}

/** Native funding creates new incomplete Steel/Goods factories; later observation requires earned readiness. */
TEST_CASE("Authentic Surface factories retain actual invoices and native construction", "[.surface-entry-factories]")
{
	const char *source = std::getenv("OTTD_SURFACE_ENTRY_SOURCE"); REQUIRE(source != nullptr);
	const bool observe = std::getenv("OTTD_SURFACE_ENTRY_READY") != nullptr;
	const auto paths = GetTestCaseArtifactPaths("space", "surface-entry-factories");
	const auto config = GetTestRuntimeConfig(); bool observed = false; std::exception_ptr failure;
	/** Preserve failed assertions through ordinary application shutdown without compensating gameplay writes. */
	std::function<void()> inspect = [&] {
		try {
			AutoRestoreBackup actor(_current_company, _local_company);
			REQUIRE(Company::IsValidID(_current_company));
			const auto steel_type = FindSurfaceFactoryType(CargoLabel{"IORE"}, CargoLabel{"STEL"});
			const auto goods_type = FindSurfaceFactoryType(CargoLabel{"STEL"}, CargoLabel{"GOOD"});
			REQUIRE(steel_type != IT_INVALID); REQUIRE(goods_type != IT_INVALID);
			const auto *company = Company::Get(_current_company);
			if (observe) {
				size_t found = 0;
				// Only factories funded by this actual company are the selected
				// output producers. Naturally generated originals remain ancestry.
				for (const auto *industry : Industry::Iterate()) {
					if (industry->founder != _local_company) continue;
					if (industry->type != steel_type && industry->type != goods_type) continue; // Other future facilities cannot establish this chain's readiness.
					CheckSurfaceFactory(*industry, true); ++found;
				}
				CHECK(found == 2); CHECK(company->current_loan == 2000000);
			} else {
				REQUIRE(company->money == 2000000); REQUIRE(company->current_loan == 2000000);
				REQUIRE(Industry::GetNumItems() == 10); REQUIRE(CargoPacket::GetNumItems() == 0);
				TileIndex predecessor = INVALID_TILE;
				const auto iron = GetCargoTypeByLabel(CargoLabel{"IORE"});
				// The selected source is a naturally generated original raw mine,
				// not a factory fixture or a supplied train-load of ore.
				for (const auto *industry : Industry::Iterate()) {
					if (!industry->accepted.empty()) continue;
					for (const auto &cargo : industry->produced) if (cargo.cargo == iron) predecessor = industry->location.tile;
				}
				REQUIRE(predecessor != INVALID_TILE);
				SaveSurfaceFactories(paths.saves / "surface-entry-before-factories-440.sav");
				const std::array types{steel_type, goods_type};
				// Each later site is selected against the actual preceding paid
				// world. A stale quote before another footprint is built cannot
				// authorize spending or overlapping facility ownership.
				for (size_t index = 0; index < types.size(); ++index) {
					const auto type = types[index];
					const auto anchor = SelectSurfaceFactorySite(type, predecessor); REQUIRE(anchor != INVALID_TILE);
					const auto quote = FundSurfaceFactory(anchor, type, true); REQUIRE(quote.Succeeded());
					SaveSurfaceFactories(paths.saves / fmt::format("surface-entry-factory-{}-query-440.sav", index));
					const auto cash = company->money, debt = company->current_loan;
					const auto expense = company->yearly_expenses[0][ExpensesType::Other];
					const auto count = Industry::GetNumItems();
					const auto paid = FundSurfaceFactory(anchor, type, false); REQUIRE(paid.Succeeded());
					CHECK(paid.GetCost() == quote.GetCost()); CHECK(company->money == cash - paid.GetCost());
					CHECK(company->current_loan == debt); CHECK(company->yearly_expenses[0][ExpensesType::Other] == expense + paid.GetCost());
					REQUIRE(Industry::GetNumItems() == count + 1);
					const auto *industry = Industry::GetByTile(anchor); REQUIRE(industry->type == type);
					CheckSurfaceFactory(*industry, false);
					fmt::print("SURFACE_ENTRY_FACTORY_PAID index={} id={} type={} anchor={} predecessor={} invoice={} cash={} debt={}\n",
						index, industry->index.base(), type, anchor.base(), predecessor.base(), paid.GetCost().base(), company->money.base(), debt.base());
					SaveSurfaceFactories(paths.saves / fmt::format("surface-entry-factory-{}-paid-440.sav", index));
					predecessor = anchor;
				}
				CHECK(CargoPacket::GetNumItems() == 0);
			}
			SaveSurfaceFactories(paths.saves / "surface-entry-factories-observed-440.sav");
			fmt::print("SURFACE_ENTRY_FACTORIES_COMPLETE ready={} cash={} debt={}\n", observe, company->money.base(), company->current_loan.base()); observed = true;
		} catch (...) { failure = std::current_exception(); }
		_pause_mode.Set(PauseMode::Normal); _exit_game = true;
	};
	AutoRestoreBackup callback(surface_factory_observer, &inspect);
	IConsole::CmdRegister("surface_entry_factories", ObserveSurfaceFactories);
	const std::string config_arg = config.string();
	std::array<std::string_view, 17> args = {"openttd_surface_supply_test.exe", "-v", "null:ticks=2", "-b", "null", "-s", "null", "-m", "null", "-x", "-X", "-I", "original_dos", "-c", config_arg, "-g", source};
	CHECK(openttd_main(args) == 0); if (failure) std::rethrow_exception(failure); CHECK(observed);
}
