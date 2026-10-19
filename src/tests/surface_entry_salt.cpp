/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file surface_entry_salt.cpp Pure genuine-world Salt, portal and spoil capital characterization.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15BE.1.
 * Native geological planning is public/free. Neither information nor an
 * unexecuted quote supplies permission, company funds or excavated rock floor.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../console_internal.h"
#include "../company_base.h"
#include "../company_func.h"
#include "../command_func.h"
#include "../excavation.h"
#include "../layer_portal_cmd.h"
#include "../tbm.h"
#include "../tbm_cmd.h"
#include "../spoil_receiver.h"
#include "../industry_cmd.h"
#include "../engine_base.h"
#include "../engine_func.h"
#include "../underground_vehicle.h"
#include "../rail_cmd.h"
#include "../industrytype.h"
#include "../tile_map.h"
#include "../openttd.h"
#include "../core/backup_type.hpp"
#include "../saveload/saveload_func.h"
#include "layer_save_fixture.h"
#include "test_artifact_paths.h"
#include "../safeguards.h"

static std::function<void()> *surface_salt_observer = nullptr; ///< Current genuine-world read-only callback; never a geological or financial authority.

/** Dispatch the current actual original game's pure access characterization.
 * @param args Parameter-free native console command.
 * @return True after the current observer ran.
 */
static bool ObserveSurfaceSalt(std::span<std::string_view> args)
{
	REQUIRE(args.size() == 1); REQUIRE(surface_salt_observer != nullptr);
	(*surface_salt_observer)(); return true;
}

/** Save original native owners at an unchanged actual clock.
 * @param path Fresh invocation-owned checkpoint.
 */
static void SaveSurfaceSalt(const std::filesystem::path &path)
{
	AutoRestoreBackup pause(_pause_mode, PauseModes{PauseMode::Normal});
	AutoRestoreBackup compression(_savegame_format, std::string{"lzma"});
	REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(path), false) == SaveLoadResult::Ok);
}

/** Rank real empty Surface sites near a public planning target without terrain mutation.
 * @param x Original layer-local target column.
 * @param y Original layer-local target row.
 * @return Native Surface candidates in deterministic distance/tile order.
 */
static std::vector<TileIndex> SurfaceSaltCandidates(uint x, uint y)
{
	std::vector<TileIndex> candidates;
	// Existing factories, roads, stations and parked products stay protected;
	// native portal rules require real clear original terrain at both ends.
	for (TileIndex tile : MultiLayerMap::IterateLayer(MapLayer::Surface)) if (IsTileType(tile, TileType::Clear)) candidates.push_back(tile);
	/** @param tile Real original Surface candidate. @return Physical approach distance then stable native tile identity. */
	const auto rank = [x, y](TileIndex tile) {
		return std::make_pair(std::abs(int(TileX(tile)) - int(x)) + std::abs(int(TileY(tile)) - int(y)), tile.base());
	};
	std::ranges::sort(candidates, {}, rank); return candidates;
}

/** Query actual legal native portal housing near this original finite Salt seam.
 * @param deposit Actual original public/free finite geological planning record.
 * @return First legal example Surface anchor; no quote is retained as spending authority.
 */
static TileIndex QuerySurfaceSaltPortals(const GeologyDeposit &deposit)
{
	REQUIRE(!deposit.spans.empty());
	const auto &span = deposit.spans.front(); const uint x = (span.first_x + span.last_x) / 2, y = span.y;
	const auto flags = CommandFlagsToDCFlags(GetCommandFlags<Commands::BuildLayerPortal>());
	TileIndex first = INVALID_TILE; uint admitted = 0;
	std::map<uint32_t, uint> refusals;
	// Search original native placement only. Distinct nearby housing anchors
	// expose real terrain constraints; future funding must requery selected
	// geometry after its Surface approach/disposal infrastructure is chosen.
	for (TileIndex tile : SurfaceSaltCandidates(x, y)) {
		for (const auto direction : {DiagDirection::NE, DiagDirection::SE, DiagDirection::SW, DiagDirection::NW}) {
			const auto quote = CmdBuildLayerPortal(flags, tile, TransportType::Rail, MapLayer::Underground, direction, RAILTYPE_RAIL, ROADTYPE_ROAD);
			if (quote.Failed()) { ++refusals[quote.GetErrorMessage().base()]; continue; }
			const auto target = MultiLayerMap::TileInLayer(tile, MapLayer::Underground);
			CHECK(GetGeologyWorldState().terrain.Get(target) == ExcavationState::Solid);
			fmt::print("SURFACE_SALT_PORTAL deposit={} anchor={} target={} direction={} cost={} solid=true\n",
				deposit.id.base(), tile.base(), target.base(), to_underlying(direction), quote.GetCost().base());
			if (first == INVALID_TILE) first = tile;
			++admitted; break;
		}
		if (admitted == 4) break;
	}
	REQUIRE(first != INVALID_TILE);
	for (const auto &[reason, count] : refusals) fmt::print("SURFACE_SALT_PORTAL_REFUSAL reason={} count={}\n", reason, count);
	return first;
}

/** Query a real buildable spoil receiver and its actual bounded operating terms.
 * @param kind Original Earth or Rock material owner.
 * @param portal Original example Surface anchor, not an already funded housing.
 */
static void QuerySurfaceSaltReceiver(SpoilReceiverKind kind, TileIndex portal)
{
	const auto type = GetSpoilReceiverIndustryType(kind); REQUIRE(type != IT_INVALID);
	const auto &recipe = GetSpoilReceiverRecipe(kind); bool found = false;
	// These are independent whole placement quotes, not simultaneous funded
	// footprints. Actual receiver funding must choose/requery each later site
	// against the world left by every preceding infrastructure purchase.
	for (TileIndex tile : SurfaceSaltCandidates(TileX(portal), TileY(portal))) {
		const auto quote = CmdBuildIndustry({}, tile, type, 0, true, 123);
		if (quote.Failed()) continue;
		fmt::print("SURFACE_SALT_RECEIVER kind={} type={} anchor={} cost={} batch={} capacity={} intake={} fee={} surplus_fee={} interval={}\n",
			to_underlying(kind), type, tile.base(), quote.GetCost().base(), recipe.batch_t, recipe.stock_cap_t,
			recipe.interval_intake_t, recipe.fee_per_t.base(), recipe.surplus_fee_per_t.base(), SPOIL_RECEIVER_INTERVAL_TICKS);
		found = true; break;
	}
	REQUIRE(found);
}

/** Query original haulage definitions and real Surface rail service placement.
 * @param portal Example original native portal anchor, still unbuilt.
 */
static void QuerySurfaceSaltHaulage(TileIndex portal)
{
	const auto salt = GetCargoTypeByLabel(CargoLabel{"SALT"});
	// Existing native rail definitions must be available under this actual
	// company/date and support both Surface and the first rock plane. Published
	// costs are information, not a fabricated depot or accepted purchase receipt.
	for (const auto model : {uint16_t{4}, uint16_t{5}}) {
		const auto engine = GetUndergroundEngine(model); REQUIRE(engine != EngineID::Invalid());
		CHECK(IsEngineBuildable(engine, VehicleType::Train, _current_company));
		const auto cargo = model == 4 ? INVALID_CARGO : salt;
		CHECK(CheckUndergroundVehicleCapability(engine, MapLayer::Surface, cargo).Succeeded());
		CHECK(CheckUndergroundVehicleCapability(engine, MapLayer::Underground, cargo).Succeeded());
		fmt::print("SURFACE_SALT_HAULAGE model={} engine={} cost={} capacity={}\n", model, engine.base(),
			Engine::Get(engine)->GetCost().base(), Engine::Get(engine)->GetDisplayDefaultCapacity());
	}
	const auto flags = CommandFlagsToDCFlags(GetCommandFlags<Commands::BuildRailDepot>()); bool found = false;
	// A genuine Surface depot quote proves service placement exists. The same
	// projected rock cell must still refuse construction while solid; it cannot
	// become a free underground service bay before the TBM earns its floor.
	for (TileIndex tile : SurfaceSaltCandidates(TileX(portal), TileY(portal))) {
		for (const auto direction : {DiagDirection::NE, DiagDirection::SE, DiagDirection::SW, DiagDirection::NW}) {
			const auto quote = CmdBuildTrainDepot(flags, tile, RAILTYPE_RAIL, direction);
			if (quote.Failed()) continue;
			const auto rock = MultiLayerMap::TileInLayer(tile, MapLayer::Underground);
			CHECK(CmdBuildTrainDepot(flags, rock, RAILTYPE_RAIL, direction).Failed());
			fmt::print("SURFACE_SALT_SERVICE anchor={} direction={} cost={} rock_refused=true\n", tile.base(), to_underlying(direction), quote.GetCost().base());
			found = true; break;
		}
		if (found) break;
	}
	REQUIRE(found);
}

/** Public geology and actual command estimates expose a real paid Salt access route without changing the authentic game. */
TEST_CASE("Authentic Surface products retain unchanged owners while Salt access is characterized", "[.surface-entry-salt]")
{
	const char *source = std::getenv("OTTD_SURFACE_ENTRY_SOURCE"); REQUIRE(source != nullptr);
	const auto paths = GetTestCaseArtifactPaths("space", "surface-entry-salt");
	const auto config = GetTestRuntimeConfig(); bool observed = false; std::exception_ptr failure;
	/** Preserve original assertions through native shutdown without authoring a replacement floor or failed quote refund. */
	std::function<void()> inspect = [&] {
		try {
			AutoRestoreBackup actor(_current_company, _local_company); REQUIRE(Company::IsValidID(_current_company));
			const auto *company = Company::Get(_current_company);
			REQUIRE(company->money == 699555); REQUIRE(company->current_loan == 2000000); CHECK(company->GetMaxLoan() == 5000000);
			SaveSurfaceSalt(paths.saves / "surface-entry-before-salt-440.sav");
			const GeologyDeposit *selected = nullptr; uint salt_count = 0;
			// Optional surveyed flags are information only. Actual canonical
			// Underground Salt records provide no claim or excavation permission;
			// their original finite reserves/solid spans remain untouched here.
			for (const auto &deposit : GetGeologyWorldState().deposits.Records()) {
				if (deposit.resource != CargoLabel{"SALT"}) continue;
				if (deposit.layer != MapLayer::Underground) continue;
				CHECK(deposit.claimant == CompanyID::Invalid()); CHECK(deposit.remaining_reserve == deposit.initial_reserve);
				fmt::print("SURFACE_SALT_DEPOSIT id={} layer={} initial={} remaining={} surveyed={} spans={}\n",
					deposit.id.base(), to_underlying(deposit.layer), deposit.initial_reserve, deposit.remaining_reserve, deposit.surveyed_companies, deposit.spans.size());
				for (const auto &span : deposit.spans) fmt::print("SURFACE_SALT_SPAN deposit={} y={} first={} last={}\n", deposit.id.base(), span.y, span.first_x, span.last_x);
				if (selected == nullptr) selected = &deposit;
				++salt_count;
			}
			REQUIRE(selected != nullptr);
			const auto portal = QuerySurfaceSaltPortals(*selected);
			QuerySurfaceSaltReceiver(SpoilReceiverKind::Earth, portal); QuerySurfaceSaltReceiver(SpoilReceiverKind::Rock, portal);
			QuerySurfaceSaltHaulage(portal);
			const auto *model = GetTBMCutterModel(0); REQUIRE(model != nullptr);
			const auto earth = GetTBMEngine(model->earth_wagon_model), rock = GetTBMEngine(model->rock_wagon_model);
			REQUIRE(earth != EngineID::Invalid()); REQUIRE(rock != EngineID::Invalid());
			fmt::print("SURFACE_SALT_MACHINERY model=0 machinery={} earth_engine={} earth_cost={} earth_capacity={} rock_engine={} rock_cost={} rock_capacity={} active_year={} idle_year={}\n",
				model->purchase_cost.base(), earth.base(), Engine::Get(earth)->GetCost().base(), Engine::Get(earth)->GetDisplayDefaultCapacity(),
				rock.base(), Engine::Get(rock)->GetCost().base(), Engine::Get(rock)->GetDisplayDefaultCapacity(), model->active_cost_per_year.base(), model->idle_cost_per_year.base());
			// There is intentionally no commissioned facility yet. The ordinary
			// command must refuse this unbuilt example rather than fabricate an
			// accepted machinery/route quote using copied portal metadata.
			const auto area = MultiLayerMap::TileInLayer(portal, MapLayer::Underground);
			CHECK(CmdCommissionTBM({}, portal, 0, 2, 2, StationID::Invalid(), area, area, false).Failed());
			SaveSurfaceSalt(paths.saves / "surface-entry-after-salt-440.sav");
			fmt::print("SURFACE_SALT_QUERIES_COMPLETE deposits={} cash={} debt={} maximum={} supplied_floor=false\n",
				salt_count, company->money.base(), company->current_loan.base(), company->GetMaxLoan().base()); observed = true;
		} catch (...) { failure = std::current_exception(); }
		_pause_mode.Set(PauseMode::Normal); _exit_game = true;
	};
	AutoRestoreBackup observer(surface_salt_observer, &inspect); IConsole::CmdRegister("surface_entry_salt", ObserveSurfaceSalt);
	const std::string config_arg = config.string();
	std::array<std::string_view, 17> args = {"openttd_surface_supply_test.exe", "-v", "null:ticks=2", "-b", "null", "-s", "null", "-m", "null", "-x", "-X", "-I", "original_dos", "-c", config_arg, "-g", source};
	CHECK(openttd_main(args) == 0); if (failure) std::rethrow_exception(failure); CHECK(observed);
}
