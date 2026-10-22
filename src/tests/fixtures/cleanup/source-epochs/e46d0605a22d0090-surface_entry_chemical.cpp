/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file surface_entry_chemical.cpp Pure chemical-chain characterization in the authentic Surface-Salt world.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15BH.1.
 * Original commands own all placement, cargo capability and capital quotes.
 * No candidate, inventory, financial balance or work progress is installed.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../console_internal.h"
#include "../company_base.h"
#include "../company_func.h"
#include "../command_func.h"
#include "../industry_cmd.h"
#include "../industry.h"
#include "../industrytype.h"
#include "../layer_industry_processor.h"
#include "../multilayer_recipe.h"
#include "../engine_base.h"
#include "../engine_func.h"
#include "../articulated_vehicles.h"
#include "../underground_vehicle.h"
#include "../vehicle_cmd.h"
#include "../vehicle_base.h"
#include "../train.h"
#include "../station_base.h"
#include "../town.h"
#include "../cargopacket.h"
#include "../tile_map.h"
#include "../openttd.h"
#include "../core/backup_type.hpp"
#include "../saveload/saveload_func.h"
#include "layer_save_fixture.h"
#include "test_artifact_paths.h"
#include "surface_chemical_layout.h"
#include "../safeguards.h"

static std::function<void()> *surface_chemical_observer = nullptr; ///< Invocation callback, never a material or finance authority.

/** Dispatch the loaded genuine world's pure chemical-chain observer.
 * @param args Parameter-free native console arguments.
 * @return True after the invocation's observer ran.
 */
static bool ObserveSurfaceChemical(std::span<std::string_view> args)
{
	REQUIRE(args.size() == 1); REQUIRE(surface_chemical_observer != nullptr); (*surface_chemical_observer)(); return true;
}

/** Save the actual paused world without earning native work or moving freight.
 * @param path Fresh invocation-owned output, never the original predecessor.
 */
static void SaveSurfaceChemical(const std::filesystem::path &path)
{
	AutoRestoreBackup pause(_pause_mode, PauseModes{PauseMode::Normal});
	AutoRestoreBackup compression(_savegame_format, std::string{"lzma"});
	REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(path), false) == SaveLoadResult::Ok);
}

/** Query one original command inside its native estimate/nesting scope.
 * @tparam Tcmd Original native command identity.
 * @tparam Targs Complete original command argument types.
 * @param tile Actual action location, or zero for coordinate-free actions.
 * @param args Complete native arguments, including tiled command locations.
 * @return Unchanged original successful quote or original refusal.
 */
template <Commands Tcmd, typename... Targs>
static auto QuoteSurfaceChemical(TileIndex tile, Targs... args)
{
	return Command<Tcmd>::template Unsafe<CommandCallback>(INVALID_STRING_ID, nullptr, false, true, tile, std::make_tuple(args...));
}

/** Characterize real nonoverlapping factory candidates and their actual nearest-town constraint.
 * @param type Installed native Chemical works definition, shared by CHEM and PCHM.
 */
static void QuoteSurfaceChemicalSites(IndustryType type)
{
	const auto *spec = GetIndustrySpec(type); REQUIRE(spec->layouts.size() == 1); REQUIRE(spec->layouts.front().size() == 16);
	const auto *station = Station::Get(StationID{4});
	std::vector<TileIndex> candidates;
	for (TileIndex tile : MultiLayerMap::IterateLayer(MapLayer::Surface)) candidates.push_back(tile);
	/** Rank by the existing real Salt unloading station, then actual tile identity.
	 * @param tile Candidate Surface anchor.
	 * @return Stable distance and tile ordering, without changing terrain.
	 */
	const auto rank = [station](TileIndex tile) {
		return std::make_pair(std::abs(int(TileX(tile)) - int(TileX(station->xy))) +
			std::abs(int(TileY(tile)) - int(TileY(station->xy))), tile.base());
	};
	std::ranges::sort(candidates, {}, rank);
	std::map<TownID, uint> admitted;
	// Joint planning must retain whole16cell machinery and town identity. Two
	// independent successful quotes do not permit overlapping factories or a
	// second same-type factory in one town when the actual setting forbids it.
	for (TileIndex anchor : candidates) {
		if (TileX(anchor) + 4 >= Map::SizeX()) continue;
		if (TilePlaneY(anchor) + 4 >= Map::SizeY()) continue;
		const auto *town = ClosestTownFromTile(anchor, UINT_MAX); REQUIRE(town != nullptr);
		if (admitted[town->index] >= 8) continue;
		bool clear = true, catchment = false;
		// Native layout offsets own the footprint; empty-looking anchor tiles
		// cannot authorize clearance of a paid route or another actual facility.
		for (const auto &cell : spec->layouts.front()) {
			const auto tile = TileAddWrap(anchor, cell.ti.x, cell.ti.y);
			clear = clear && IsTileType(tile, TileType::Clear);
			catchment = catchment || station->TileIsInCatchment(tile);
		}
		if (!clear) continue;
		auto quote = QuoteSurfaceChemical<Commands::BuildIndustry>(anchor, anchor, type, uint32_t{0}, true, uint32_t{123});
		if (quote.Failed()) continue;
		++admitted[town->index];
		fmt::print("SURFACE_CHEMICAL_SITE anchor={} x={} y={} town={} cost={} salt_station_catchment={}\n",
			anchor.base(), TileX(anchor), TilePlaneY(anchor), town->index.base(), quote.GetCost().base(), catchment);
	}
	CHECK_FALSE(admitted.empty());
}

/** Query actual conventional carriers without extending any cargo/refit permission.
 * @param label Required original Oil, chemical or Propellant cargo meaning.
 */
static void QuoteSurfaceChemicalCarriers(CargoLabel label)
{
	const CargoType cargo = GetCargoTypeByLabel(label); REQUIRE(IsValidCargoType(cargo));
	// Exact refit masks and whole BuildVehicle estimates must both admit a
	// useful positive-capacity carrier. A liquid class alone grants no hull.
	for (const auto kind : {VehicleType::Road, VehicleType::Train}) {
		const TileIndex depot = kind == VehicleType::Road ? TileIndex{2443} : TileIndex{1373};
		uint available = 0;
		for (const auto *engine : Engine::IterateType(kind)) {
			if (!engine->CanCarryCargo()) continue;
			if (!IsEngineBuildable(engine->index, kind, _current_company)) continue;
			if (GetUndergroundEngineCapability(engine->index) != nullptr) continue;
			if (!GetUnionOfArticulatedRefitMasks(engine->index, true).Test(cargo)) continue;
			auto quote = QuoteSurfaceChemical<Commands::BuildVehicle>(depot, depot, engine->index, false, cargo, ClientID::Server);
			if (std::get<0>(quote).Failed()) continue;
			if (std::get<2>(quote) == 0) continue;
			++available;
			fmt::print("SURFACE_CHEMICAL_CARRIER cargo={} kind={} engine={} cost={} capacity={}\n", label.AsString(),
				static_cast<uint>(kind), engine->index.base(), std::get<0>(quote).GetCost().base(), std::get<2>(quote));
		}
		fmt::print("SURFACE_CHEMICAL_CARRIER_COUNT cargo={} kind={} available={}\n", label.AsString(), static_cast<uint>(kind), available);
	}
}

/** Record actual facility, natural Oil, bank and recoverable-asset terms without spending. */
static void CharacterizeSurfaceChemical()
{
	const auto *company = Company::Get(_current_company); REQUIRE(company->current_loan == 4250000); REQUIRE(company->money == 88274);
	const auto cash = company->money; const auto expenses = company->yearly_expenses;
	const auto vehicles = Vehicle::GetNumItems(), industries = Industry::GetNumItems(), packets = CargoPacket::GetNumItems();
	const IndustryType type = GetLayerProcessorIndustryType(MultiLayerRecipeID{"CHEM"}); REQUIRE(type != IT_INVALID);
	REQUIRE(IsLayerProcessorDefinition(type));
	fmt::print("SURFACE_CHEMICAL_WORLD type={} year={} cash={} debt={} ceiling={} towns={} multiple_per_town={}\n", type,
		TimerGameCalendar::year.base(), cash.base(), company->current_loan.base(), company->GetMaxLoan().base(), Town::GetNumItems(),
		_settings_game.economy.multiple_industry_per_town);
	// Both alternatives use one installed facility family but distinct physical
	// source identities are required by the original own-source delivery guard.
	for (const auto &record : GetMultiLayerManufacturingRecipes()) {
		if (record.facility != "Chemical works") continue;
		fmt::print("SURFACE_CHEMICAL_RECIPE id={} year={} work={} batches={} input_capacity={} output_capacity={}\n", record.recipe.id.AsString(),
			record.introduction_year.base(), record.recipe.work_economy_ticks, record.recipe.max_batches, record.input_capacity, record.output_capacity);
	}
	const CargoType oil = GetCargoTypeByLabel(CargoLabel{"OIL_"}); REQUIRE(IsValidCargoType(oil));
	// Only a genuinely generated ordinary producer can supply the selected Oil;
	// an input acceptance slot or a processor output cannot stand in for it.
	for (const auto *industry : Industry::Iterate()) {
		if (GetTileLayer(industry->location.tile) != MapLayer::Surface) continue;
		for (const auto &product : industry->produced) {
			if (product.cargo != oil) continue;
			fmt::print("SURFACE_CHEMICAL_OIL industry={} type={} anchor={} founder={} rate={} waiting={}\n", industry->index.base(),
				industry->type, industry->location.tile.base(), industry->founder.base(), product.rate, product.waiting);
		}
	}
	QuoteSurfaceChemicalSites(type);
	SurfaceChemicalLayout layout; layout.Prepare();
	const auto first = QuoteSurfaceChemical<Commands::BuildIndustry>(TileIndex{1566}, TileIndex{1566}, type, uint32_t{0}, true, uint32_t{123});
	const auto second = QuoteSurfaceChemical<Commands::BuildIndustry>(TileIndex{1639}, TileIndex{1639}, type, uint32_t{0}, true, uint32_t{123});
	REQUIRE(first.Succeeded()); REQUIRE(second.Succeeded());
	const auto oil_truck = QuoteSurfaceChemical<Commands::BuildVehicle>(TileIndex{2443}, TileIndex{2443}, EngineID{132}, false, oil, ClientID::Server);
	const auto chemical_truck = QuoteSurfaceChemical<Commands::BuildVehicle>(TileIndex{2443}, TileIndex{2443}, EngineID{132}, false,
		GetCargoTypeByLabel(CargoLabel{"CHEM"}), ClientID::Server);
	REQUIRE(std::get<0>(oil_truck).Succeeded()); REQUIRE(std::get<0>(chemical_truck).Succeeded());
	const auto recovery = QuoteSurfaceChemical<Commands::SellVehicle>(TileIndex{0}, VehicleID{7}, true, false, ClientID::Server); REQUIRE(recovery.Succeeded());
	const auto capital = first.GetCost() + layout.invoice + 2 * std::get<0>(oil_truck).GetCost() + std::get<0>(chemical_truck).GetCost();
	const Money buffer{100000}; const auto shortfall = capital + buffer - (cash - recovery.GetCost());
	const Money credit{((shortfall.base() + 9999) / 10000) * 10000};
	REQUIRE(company->current_loan + credit <= company->GetMaxLoan());
	fmt::print("SURFACE_CHEMICAL_CAPITAL first={} second={} infrastructure={} carriers={} initial_total={} cutter_recovery={} buffer={} credit={} future_headroom={}\n",
		first.GetCost().base(), second.GetCost().base(), layout.invoice.base(),
		(2 * std::get<0>(oil_truck).GetCost() + std::get<0>(chemical_truck).GetCost()).base(), capital.base(), -recovery.GetCost().base(),
		buffer.base(), credit.base(), (company->GetMaxLoan() - company->current_loan - credit).base());
	for (const auto label : {CargoLabel{"OIL_"}, CargoLabel{"CHEM"}, CargoLabel{"PROP"}}) QuoteSurfaceChemicalCarriers(label);
	// Idle assets are capital only after an admitted original sale. Book values
	// and machinery hidden outside a legal depot cannot become free cash.
	for (const auto id : {VehicleID{7}, VehicleID{13}}) {
		const auto *vehicle = Vehicle::Get(id); REQUIRE(vehicle->cargo.TotalCount() == 0);
		auto quote = QuoteSurfaceChemical<Commands::SellVehicle>(TileIndex{0}, id, true, false, ClientID::Server);
		fmt::print("SURFACE_CHEMICAL_RECOVERY vehicle={} tile={} admitted={} cost={} error={}\n", id.base(), vehicle->tile.base(),
			quote.Succeeded(), quote.GetCost().base(), quote.GetErrorMessage().base());
	}
	CHECK(company->money == cash); CHECK(company->current_loan == 4250000); CHECK(company->yearly_expenses == expenses);
	CHECK(Vehicle::GetNumItems() == vehicles); CHECK(Industry::GetNumItems() == industries); CHECK(CargoPacket::GetNumItems() == packets);
}

/** The actual saved world authorizes only native pure producer/route/capital quotes. */
TEST_CASE("Authentic Surface chemicals retain pure native placement transport and finance quotes", "[.surface-entry-chemical]")
{
	const char *source = std::getenv("OTTD_SURFACE_ENTRY_SOURCE"); REQUIRE(source != nullptr);
	const auto paths = GetTestCaseArtifactPaths("space", "surface-entry-chemical"); const auto config = GetTestRuntimeConfig();
	bool observed = false; std::exception_ptr failure;
	/** Preserve native assertions through ordinary shutdown without compensating grants. */
	std::function<void()> inspect = [&] {
		try {
			AutoRestoreBackup actor(_current_company, _local_company); REQUIRE(Company::IsValidID(_current_company));
			SaveSurfaceChemical(paths.saves / "surface-chemical-before-440.sav");
			CharacterizeSurfaceChemical();
			SaveSurfaceChemical(paths.saves / "surface-chemical-query-440.sav");
			fmt::print("SURFACE_CHEMICAL_COMPLETE pure=true supplied_stock=false\n"); observed = true;
		} catch (...) { failure = std::current_exception(); }
		_pause_mode.Set(PauseMode::Normal); _exit_game = true;
	};
	AutoRestoreBackup observer(surface_chemical_observer, &inspect); IConsole::CmdRegister("surface_entry_chemical", ObserveSurfaceChemical);
	const std::string config_arg = config.string();
	std::array<std::string_view, 17> args = {"openttd_surface_supply_test.exe", "-v", "null:ticks=2", "-b", "null", "-s", "null", "-m", "null", "-x", "-X", "-I", "original_dos", "-c", config_arg, "-g", source};
	CHECK(openttd_main(args) == 0); if (failure) std::rethrow_exception(failure); CHECK(observed);
}
