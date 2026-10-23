/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file geothermal_first_plant_rebuild.cpp Fresh command-funded machinery on the original retired paid first-plant site.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs18-geothermal-energy-and-industry.md Group1CCB/operating1.3B.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
 * Original saved geometry is borrowed, never restored. Replacement materials, labour and useful service follow1CCC/1CCD.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "geothermal_first_plant_fixture.h"
#include "test_artifact_paths.h"
#include "../console_internal.h"
#include "../geothermal_site_cmd.h"
#include "../geothermal_utility_cmd.h"
#include "../openttd.h"
#include "../saveload/saveload_func.h"
#include "../safeguards.h"

static std::function<void()> *_plant_rebuild_observer = nullptr; ///< Invocation-local startup observation; never a saved claim, industry, stock or power owner.

/** Borrowed original geometry and observed fresh native identities for one test invocation; this contains no gameplay inventory or alternative registry. */
struct PlantRebuildSites {
	std::vector<GeothermalSiteClaim> original; ///< Exact retained consumed native site rows from the ordinary retired input.
	std::array<GeothermalSiteID, 3> fresh{GeothermalSiteID{UINT32_MAX}, GeothermalSiteID{UINT32_MAX}, GeothermalSiteID{UINT32_MAX}}; ///< Native command-issued new claims, never caller-assigned identities.
};

/** @param arguments Ordinary native startup command. @return Success after observing the exact normally loaded retired original. */
static bool ObservePlantRebuild(std::span<std::string_view> arguments)
{
	REQUIRE(arguments.size() == 1); REQUIRE(_plant_rebuild_observer != nullptr); (*_plant_rebuild_observer)(); return true;
}

/** @param sites Original saved vacant machinery geometry. @return Exact native removal invoices for old installed domain cells strictly inside these footprints; unrelated paths and all retired energy histories remain original. */
static Money PrepareOriginalPlantReplacementFloor(const PlantRebuildSites &sites)
{
	Money invoices = 0;
	// A building cannot hide a surviving paid utility port. Remove only
	// actual owned overlays in its exact old footprint through native commands;
	// surrounding paid routes remain installed for explicit later reconnection.
	for (const auto &site : sites.original) {
		const auto *terms = GetMultiLayerGeothermalConstruction(site.site.kind); REQUIRE(terms != nullptr);
		for (uint y = 0; y < terms->height; ++y) for (uint x = 0; x < terms->width; ++x) {
			const auto tile = TileAddWrap(site.site.tile, x, y);
			for (const auto domain : {UtilityEnergyDomain::Heat, UtilityEnergyDomain::Electricity}) {
				const auto cell = GetGeothermalUtilityLink(tile, domain); if (!cell.has_value()) continue;
				REQUIRE(cell->owner == _current_company); const auto cash = Company::Get(_current_company)->money; const auto utility = ComputeUtilityPowerDigest();
				const auto quote = Command<Commands::RemoveGeothermalUtilityLink>::Do({}, tile, domain); REQUIRE(quote.Succeeded());
				CHECK(Company::Get(_current_company)->money == cash); CHECK(ComputeUtilityPowerDigest() == utility);
				const auto removal = Command<Commands::RemoveGeothermalUtilityLink>::Do(DoCommandFlag::Execute, tile, domain); REQUIRE(removal.Succeeded());
				CHECK(removal.GetCost() == quote.GetCost()); CHECK(Company::Get(_current_company)->money == cash - removal.GetCost()); invoices += removal.GetCost();
				CHECK_FALSE(GetGeothermalUtilityLink(tile, domain).has_value());
			}
			CHECK_FALSE(HasGeothermalUtilityCell(tile));
		}
	}
	return invoices;
}

/** @param original Exact old consumed native geometry/owner. @return Fresh actual native claim after a pure synchronized estimate and a real free planning command; no stock, work or source is created. */
static GeothermalSiteID PlanOriginalPlantReplacement(const GeothermalSiteClaim &original)
{
	const auto &request = original.site; const auto cash = Company::Get(_current_company)->money;
	const auto sites = ComputeGeothermalSiteDigest(), utility = ComputeUtilityPowerDigest(); const std::array random{_random.state[0], _random.state[1]};
	const auto quote = Command<Commands::PlanGeothermalSite>::Do({}, request.tile, request.vent, request.kind, request.endpoint, request.access);
	const auto admission = PrepareGeothermalSite(request); const auto planning = ClaimGeothermalSite(request, false);
	fmt::print("PLANT_REBUILD_ADMISSION kind={} floor_error={} claim_error={} tile={} mode={} pause={} actor={} request_owner={} quote_error={}\n",
		to_underlying(request.kind), to_underlying(admission.error), to_underlying(planning.error), admission.failure_tile.base(),
		to_underlying(_game_mode), _pause_mode.base(), _current_company.base(), request.owner.base(), quote.GetErrorMessage().base());
	REQUIRE(quote.Succeeded()); CHECK(ComputeGeothermalSiteDigest() == sites); CHECK(ComputeUtilityPowerDigest() == utility);
	CHECK(Company::Get(_current_company)->money == cash); CHECK((std::array{_random.state[0], _random.state[1]} == random));
	REQUIRE(Command<Commands::PlanGeothermalSite>::Do(DoCommandFlag::Execute, request.tile, request.vent, request.kind, request.endpoint, request.access).Succeeded());
	const auto *claim = GetGeothermalSiteClaimForTile(request.tile); REQUIRE(claim != nullptr); CHECK(claim->active); CHECK(claim->id > original.id);
	CHECK(claim->site.kind == request.kind); CHECK(claim->site.vent == request.vent); CHECK(claim->site.owner == request.owner);
	CHECK(claim->site.tile == request.tile); CHECK(claim->site.endpoint == request.endpoint); CHECK(claim->site.access == request.access);
	CHECK(Company::Get(_current_company)->money == cash); CHECK(ComputeUtilityPowerDigest() == utility); return claim->id;
}

/** @param original Exact original saved request, now occupied by its fresh actual cold claim. @param site Fresh native command-issued planning identity. @return Actual whole-capital native funding invoice, without material receipt, labour or a utility identity. */
static Money FundOriginalPlantReplacement(const GeothermalSiteClaim &original, GeothermalSiteID site)
{
	const auto &request = original.site; const auto *terms = GetMultiLayerGeothermalConstruction(request.kind); REQUIRE(terms != nullptr);
	const auto cash = Company::Get(_current_company)->money; const auto sites = ComputeGeothermalSiteDigest(), utility = ComputeUtilityPowerDigest();
	const auto count = Industry::GetNumItems(); const std::array random{_random.state[0], _random.state[1]}; CommandCost quote, result;
	// Collector and both dated generator families keep their actual separate
	// command owners; an old consumed blueprint cannot impersonate funding.
	if (request.kind == MultiLayerGeothermalConstructionKind::Collector) {
		quote = Command<Commands::BuildGeothermalCollector>::Do({}, request.tile, site, uint32_t{123});
	} else {
		quote = Command<Commands::BuildGeothermalGenerator>::Do({}, request.tile, site, uint32_t{123});
	}
	REQUIRE(quote.Succeeded()); CHECK(quote.GetCost() == Money{terms->capital_cost}); CHECK(Company::Get(_current_company)->money == cash);
	CHECK(ComputeGeothermalSiteDigest() == sites); CHECK(ComputeUtilityPowerDigest() == utility); CHECK(Industry::GetNumItems() == count);
	CHECK((std::array{_random.state[0], _random.state[1]} == random));
	if (request.kind == MultiLayerGeothermalConstructionKind::Collector) {
		result = Command<Commands::BuildGeothermalCollector>::Do(DoCommandFlag::Execute, request.tile, site, uint32_t{123});
	} else {
		result = Command<Commands::BuildGeothermalGenerator>::Do(DoCommandFlag::Execute, request.tile, site, uint32_t{123});
	}
	REQUIRE(result.Succeeded()); CHECK(result.GetCost() == quote.GetCost()); CHECK(Company::Get(_current_company)->money == cash - result.GetCost());
	CHECK(Industry::GetNumItems() == count + 1); CHECK_FALSE(GetGeothermalSiteClaim(site)->active); CHECK(ComputeUtilityPowerDigest() == utility);
	return result.GetCost();
}

/** @param sites Original and fresh observed native site identities. @param stage0 Original retired0, native vacant-footprint preparation1, fresh cold plans2, actual paid empty machinery3. @param capital Exact native funding charges. @param preparation Exact native original-footprint utility removal charges. @return Flat original/natural/retired/fresh construction observations; this query grants nothing. */
static std::string CapturePlantRebuild(const PlantRebuildSites &sites, uint stage0, Money capital, Money preparation)
{
	const auto &vent = GetGeologyWorldState().vents.front(); const auto utility = CaptureUtilityPowerState();
	std::string result = fmt::format("stage={},tick={},year={},cash={},invoices={},capital={},preparation={},vent={},capacity={},geology={},projects={},exchange={},supply={},risers={},utility={},claims={},industries={},generators={},collectors={},vehicles={},packets={},old_producers={},next_producer={},next_consumer={},next_link={},utility_links={},",
		stage0, TimerGameTick::counter, TimerGameCalendar::year.base(), Company::Get(_current_company)->money.base(), (capital + preparation).base(), capital.base(), preparation.base(), vent.id.base(), vent.thermal_capacity_w,
		ComputeGeologyStateDigest(GetGeologyWorldState()), ComputeTBMProjectDigest(), ComputeTBMExchangeDigest(), ComputeSurfaceSupplyDigest(), ComputeGeothermalRiserDigest(),
		ComputeUtilityPowerDigest(), CaptureGeothermalSiteClaims().size(), Industry::GetNumItems(), CaptureGeothermalGenerators().size(), CaptureGeothermalCollectors().size(),
		Vehicle::GetNumItems(), CargoPacket::GetNumItems(), utility.producers.size(), utility.next_producer, utility.next_consumer, utility.next_link, utility.links.size());
	// The old original bindings remain historical rows. New claims/machinery
	// report their own empty stock/work, even when the native pool reuses an ID.
	for (uint index = 0; index < sites.original.size(); ++index) {
		const auto &original = sites.original[index]; const auto *retained = GetGeothermalSiteClaim(original.id); REQUIRE(retained != nullptr); CHECK_FALSE(retained->active);
		CHECK(retained->site.vent == original.site.vent); CHECK(retained->site.tile == original.site.tile); CHECK(retained->site.kind == original.site.kind);
		CHECK(retained->site.owner == original.site.owner); CHECK(retained->site.endpoint == original.site.endpoint); CHECK(retained->site.access == original.site.access);
		IndustryID industry_id = IndustryID::Invalid(); uint32_t stock = 0, accepted = 0, work = 0, committed = 0;
		UtilityProducerID output{UINT32_MAX}; UtilityConsumerID input{UINT32_MAX};
		if (stage0 == 3) {
			const auto *industry = Industry::GetByTile(original.site.tile); REQUIRE(industry != nullptr); industry_id = industry->index;
			// Fresh native stock/history must be empty before paid replacement
			// transport. A reused Industry slot cannot inherit an old whole bill.
			for (const auto &slot : industry->accepted) {
				stock += slot.waiting;
				if (slot.history != nullptr) for (const auto &month : *slot.history) accepted += month.accepted;
			}
			CHECK(stock == 0); CHECK(accepted == 0); CHECK(industry->produced.empty());
			if (index == 0) {
				const auto *state = GetGeothermalCollectorState(industry_id); REQUIRE(state != nullptr); CHECK(state->site == sites.fresh[index]);
				work = state->work_ticks; committed = state->materials_committed; output = state->heat_producer;
				CHECK_FALSE(IsGeothermalCollectorOperational(industry_id));
				CHECK(PublishGeothermalCollectorHeat(industry_id, GetUtilityPowerInterval(), 1, false) != UtilityPowerError::None);
			} else {
				const auto *state = GetGeothermalGeneratorState(industry_id); REQUIRE(state != nullptr); CHECK(state->site == sites.fresh[index]);
				work = state->work_ticks; committed = state->materials_committed; output = state->electrical_producer; input = state->heat_consumer;
				CHECK(state->active_service_ticks == 0); CHECK(state->service_remainder == 0); CHECK_FALSE(state->has_operating_tick);
				CHECK_FALSE(IsGeothermalGeneratorConstructionComplete(industry_id));
			}
			CHECK(work == 0); CHECK(committed == 0); CHECK(output == UtilityProducerID{UINT32_MAX}); CHECK(input == UtilityConsumerID{UINT32_MAX});
		}
		result += fmt::format("old_site{}={},fresh_site{}={},kind{}={},tile{}={},endpoint{}={},access{}={},industry{}={},stock{}={},accepted{}={},work{}={},committed{}={},output{}={},input{}={},",
			index, original.id.base(), index, sites.fresh[index].base(), index, to_underlying(original.site.kind), index, original.site.tile.base(), index, original.site.endpoint.base(),
			index, to_underlying(original.site.access), index, industry_id.base(), index, stock, index, accepted, index, work, index, committed, index, output.base(), index, input.base());
	}
	CHECK_FALSE(HasGeothermalHeatSources());
	// Every old source is already terminal. Funding/planning cannot activate a
	// retired meter or change any consumed/generated history in this interval.
	for (const auto &producer : utility.producers) {
		CHECK_FALSE(producer.active); CHECK(producer.reserved_j == 0);
		result += fmt::format("old_source{}_generated={},old_source{}_consumed={},", producer.meter.producer_id.base(), producer.generated_lifetime_j,
			producer.meter.producer_id.base(), producer.consumed_lifetime_j);
	}
	return result;
}

/** @param paths Invocation-owned actual save location. @param sites Original/fresh observed native identities. @param stage Original0/prepared1/cold2/funded3. @param capital Actual native capital debits. @param preparation Actual native original-footprint utility removal debits. Retain a pure complete native world with canonical observations and both RNG authorities. */
static void SavePlantRebuild(const TestArtifactPaths &paths, const PlantRebuildSites &sites, uint stage, Money capital, Money preparation)
{
	const auto snapshot = CapturePlantRebuild(sites, stage, capital, preparation); const std::array random{_random.state[0], _random.state[1], _interactive_random.state[0], _interactive_random.state[1]};
	const auto path = paths.Output("saves", fmt::format("plant-rebuild-funded-{}.sav", stage));
	{
		AutoRestoreBackup mode{_game_mode, GameMode::Menu}; AutoRestoreBackup pause{_pause_mode, PauseModes{PauseMode::Normal}};
		AutoRestoreBackup format{_savegame_format, std::string{"lzma"}}; REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(path), false) == SaveLoadResult::Ok);
	}
	CHECK(CapturePlantRebuild(sites, stage, capital, preparation) == snapshot); CHECK((std::array{_random.state[0], _random.state[1], _interactive_random.state[0], _interactive_random.state[1]} == random));
	fmt::print("PLANT_REBUILD_WORLD snapshot={} path={}\n", snapshot, path.string());
}

/** The exact original retired world funds fresh native incarnations on unchanged paid floor/vent; no old paid bill, work, utility identity or cargo is inherited. */
TEST_CASE("Original retired first plant funds fresh empty native replacements", "[.geothermal-first-plant-rebuild-funding]")
{
	const char *input = std::getenv("OTTD_PLANT_REBUILD_INPUT"); REQUIRE(input != nullptr);
	const auto paths = GetTestCaseArtifactPaths("geothermal", "source-rebuild"); bool observed = false; std::exception_ptr failure;
	/** Borrow only ordinary-loaded retained rows and actual command results; this callback supplies no world, floor, capital, materials, labour or energy. */
	std::function<void()> inspect = [&] {
		try {
			AutoRestoreBackup actor{_current_company, _local_company}; REQUIRE_FALSE(_pause_mode.Test(PauseMode::Error));
			REQUIRE(TimerGameCalendar::year.base() == 2000); REQUIRE(GetGeologyWorldState().vents.size() == 1); REQUIRE(CargoPacket::GetNumItems() == 0);
			REQUIRE(CaptureGeothermalCollectors().empty()); REQUIRE(CaptureGeothermalGenerators().empty());
			PlantRebuildSites sites; sites.original = CaptureGeothermalSiteClaims(); REQUIRE(sites.original.size() == 3);
			REQUIRE(sites.original[0].site.kind == MultiLayerGeothermalConstructionKind::Collector);
			REQUIRE(sites.original[1].site.kind == MultiLayerGeothermalConstructionKind::Powerhouse);
			REQUIRE(sites.original[2].site.kind == MultiLayerGeothermalConstructionKind::MagmaPowerhouse);
			const auto geology = ComputeGeologyStateDigest(GetGeologyWorldState());
			const auto tick = TimerGameTick::counter; const auto cash = Company::Get(_current_company)->money; Money invoices = 0;
			SavePlantRebuild(paths, sites, 0, invoices, Money{0});
			const auto preparation = PrepareOriginalPlantReplacementFloor(sites); const auto utility = ComputeUtilityPowerDigest();
			SavePlantRebuild(paths, sites, 1, invoices, preparation);
			// Original saved requests select the same real paid floor/vent;
			// native commands allocate all new claims and capital-funded machinery.
			for (uint index = 0; index < sites.original.size(); ++index) sites.fresh[index] = PlanOriginalPlantReplacement(sites.original[index]);
			SavePlantRebuild(paths, sites, 2, invoices, preparation);
			for (uint index = 0; index < sites.original.size(); ++index) invoices += FundOriginalPlantReplacement(sites.original[index], sites.fresh[index]);
			CHECK(invoices == Money{720000}); CHECK(Company::Get(_current_company)->money == cash - invoices - preparation);
			CHECK(TimerGameTick::counter == tick); CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == geology); CHECK(ComputeUtilityPowerDigest() == utility);
			SavePlantRebuild(paths, sites, 3, invoices, preparation);
		} catch (...) { failure = std::current_exception(); }
		observed = true; _exit_game = true;
	};
	AutoRestoreBackup observer{_plant_rebuild_observer, &inspect}; IConsole::CmdRegister("plant_rebuild_funding", ObservePlantRebuild);
	const std::string config = GetTestRuntimeConfig().string();
	std::array<std::string_view, 17> arguments{"openttd_test.exe", "-v", "null:ticks=2", "-b", "8bpp-simple", "-s", "null", "-m", "null", "-x", "-X", "-I", "original_dos", "-c", config, "-g", input};
	CHECK(openttd_main(arguments) == 0); if (failure) std::rethrow_exception(failure); CHECK(observed);
}
