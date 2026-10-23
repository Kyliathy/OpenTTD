/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file geothermal_first_plant_fixture.h One actual paid Surface logistics world and untouched natural-vent construction targets; no fixture excavation or material/energy injection.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs18-geothermal-energy-and-industry.md Group8FA.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs18-e-first-plant-bootstrap.md.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
 */
#ifndef TESTS_GEOTHERMAL_FIRST_PLANT_FIXTURE_H
#define TESTS_GEOTHERMAL_FIRST_PLANT_FIXTURE_H

#include <filesystem>
#include "underground_depot_fixture.h"
#include "layer_save_fixture.h"
#include "../geothermal_generator.h"
#include "../geothermal_collector.h"
#include "../geothermal_riser.h"
#include "../surface_supply.h"
#include "../space_vehicle.h"
#include "../transport_tube_model.h"
#include "../core/format.hpp"

/** Declared future world geometry only; actual native commands, original natural source and geology retain all admission/work/quantity authority. */
struct FirstPlantLayout {
	GeothermalVentField vent; ///< Original unmodified generated source, retained by value across native geological transactions.
	TileIndex upper_export = INVALID_TILE; ///< Later Surface/Underground pair on the Standard cutter's paid chamber.
	TileIndex deep_commission = INVALID_TILE; ///< Later Underground/Deep assembly pair on paid Standard floor.
	TileIndex deep_export = INVALID_TILE; ///< Later Underground/Deep freight/export pair on both actually paid floors.
	TileIndex upper_riser = INVALID_TILE; ///< Two-cell-square sending machinery beside the later upper export pair.
	TileIndex upper_riser_port = INVALID_TILE; ///< Its actual adjacent west port, within the declared machinery.
	TileIndex deep_riser = INVALID_TILE; ///< Two-cell-square sending machinery beside the later Deep export pair.
	TileIndex deep_riser_port = INVALID_TILE; ///< Its actual adjacent west port, within the declared machinery.
	TileIndex collector = INVALID_TILE; ///< Three-cell-square construction contacting the original natural field.
	TileIndex powerhouse = INVALID_TILE; ///< First four-cell-square1980 machinery, separate from source and riser.
	TileIndex magma = INVALID_TILE; ///< Separately paid2000 comparison machinery; clearing this declared extra floor is charged with the same original work.
	TileIndex deep_start = INVALID_TILE; ///< Inclusive complete later Deep requested chamber, including service/freight/utility space.
	TileIndex deep_end = INVALID_TILE; ///< Inclusive complete later Deep requested chamber; the canonical planner also charges its swept rim/approaches.
};

/**
 * Describe the original declared first-plant geometry without constructing a world.
 * @param vent Actual unmodified natural source, from the initial world or ordinary afterload.
 * @return Desired world tiles only; floor, capital, construction and utility admission stay with native owners.
 */
inline FirstPlantLayout DescribeFirstPlantLayout(const GeothermalVentField &vent)
{
	FirstPlantLayout layout; layout.vent = vent;
	const auto x = layout.vent.x, y = layout.vent.y;
	layout.upper_export = TileXYL(x + 4, y - 7, MapLayer::Surface);
	layout.deep_commission = TileXYL(x + 1, y - 5, MapLayer::Underground);
	layout.deep_export = TileXYL(x + 4, y - 1, MapLayer::Underground);
	layout.upper_riser = TileXYL(x + 2, y - 7, MapLayer::Underground);
	layout.upper_riser_port = TileXYL(x + 3, y - 7, MapLayer::Underground);
	layout.deep_riser = TileXYL(x + 2, y - 2, MapLayer::DeepUnderground);
	layout.deep_riser_port = TileXYL(x + 3, y - 1, MapLayer::DeepUnderground);
	layout.collector = TileXYL(x, y, MapLayer::DeepUnderground);
	layout.powerhouse = TileXYL(x + 4, y + 2, MapLayer::DeepUnderground);
	layout.magma = TileXYL(x + 4, y + 7, MapLayer::DeepUnderground);
	layout.deep_start = TileXYL(x - 1, y - 2, MapLayer::DeepUnderground);
	layout.deep_end = TileXYL(x + 10, y + 11, MapLayer::DeepUnderground);
	return layout;
}

/**
 * Release settled actual Deep work, convert its paid support and fund the declared export access pairs.
 * @param id Original loaded completed project; no replacement fixture is constructed.
 * @return Actual command invoices already debited once by the native company owner.
 */
Money ReleaseLoadedFirstPlant(TBMProjectID id);

/** One retained native world; the Standard cutter must earn Underground access before any Deep assembly or first-plant construction. */
class GeothermalFirstPlantRig {
	NativeDepotEconomy economy; ///< Native medium prices/unit inflation established before any initial infrastructure invoice.
public:
	UndergroundWorkflowWorld world{MapLayer::Underground, true, true, GEOLOGY_HARDNESS_VERSION, 420, TileIndexDiffC{0, -8}}; ///< Actual initial paid Surface station/receivers/hauler and sealed upper pair; every new rock floor remains solid.
	LayerSaveContent content{this->world.ActivatedContent(), true}; ///< Exact selected native package scope for ordinary definition polishing throughout this retained world; the outer world still owns all actual physical quantities.
	AutoRestoreBackup<bool> turns{_settings_game.pf.forbid_90_deg, false}; ///< Explicit legal rail policy for wide chambers; geological cutter advances retain the canonical45-degree limit.
	FirstPlantLayout layout; ///< Pure desired positions, never a second site/job/material/energy owner.
	Money commands = 0; ///< Additional ordinary native command invoices after initial setup.
	/** Retain the original natural source and declare all future paid floors without changing a geological cell, packet or utility meter. */
	GeothermalFirstPlantRig()
	{
		this->world.simulate_company_limits = true;
		ResetGeothermalGenerators(); ResetGeothermalCollectors(); ResetGeothermalRisers(); ResetGeothermalSiteClaims();
		ResetUtilityPowerState(); ResetSurfaceSupplyOrders();
		// Ordinary NewGRF polishing installs the whole native definition set.
		// Match that same catalogue before saving, so application afterload need
		// not append missing passive engine/industry identities. These owners
		// create definitions only: no building, floor, cargo or utility quantity.
		REQUIRE(InitialiseSpaceEngineDefinition());
		REQUIRE(InitialiseTubeContentDefinitions(this->world.ActivatedContent()));
		REQUIRE(InitialiseGeothermalCollectorDefinitions(this->world.ActivatedContent()));
		REQUIRE(InitialiseGeothermalRiserDefinitions(this->world.ActivatedContent()));
		REQUIRE(InitialiseGeothermalGeneratorDefinitions(this->world.ActivatedContent()));
		REQUIRE(GetGeologyWorldState().vents.size() == 1);
		this->layout = DescribeFirstPlantLayout(GetGeologyWorldState().vents.front());
		REQUIRE(this->layout.vent.thermal_capacity_w == 1000000);
		REQUIRE(CaptureTBMProjects().empty()); REQUIRE(CaptureUtilityPowerState().producers.empty());
	}
	/** @param start Inclusive declared requested floor. @param end Inclusive same-plane corner. @return Complete actual world-tile rectangle; this query never opens or claims ground. */
	static std::vector<TileIndex> Rectangle(TileIndex start, TileIndex end)
	{
		REQUIRE(IsValidTile(start)); REQUIRE(IsValidTile(end)); REQUIRE(GetTileLayer(start) == GetTileLayer(end));
		std::vector<TileIndex> result;
		// Every requested world cell is recorded once, before the real planner
		// adds its own necessary turning/rim/approach envelope and prices its union.
		for (uint y = TilePlaneY(start); y <= TilePlaneY(end); ++y) {
			for (uint x = TileX(start); x <= TileX(end); ++x) result.push_back(TileXYL(x, y, GetTileLayer(start)));
		}
		return result;
	}
	/** @tparam Tcmd Existing synchronized command. @tparam Targs Its public typed arguments. @param args Actual ordinary command inputs. @return Complete paid native result; no direct map/controller writes supply any part. */
	template <Commands Tcmd, typename... Targs>
	auto Do(Targs... args)
	{
		auto result = Command<Tcmd>::Do(DoCommandFlag::Execute, args...); UpdateSignalsInBuffer();
		INFO("first-plant command=" << GetCommandName(Tcmd) << " error=" << ExtractCommandCost(result).GetErrorMessage().base());
		REQUIRE(ExtractCommandCost(result).Succeeded()); this->commands += ExtractCommandCost(result).GetCost(); return result;
	}
};

#endif /* TESTS_GEOTHERMAL_FIRST_PLANT_FIXTURE_H */
