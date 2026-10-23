/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file geothermal_riser_fixture.h Actual native funding and construction fixture with explicitly supplied excavated floor and Connected portal input.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs18-geothermal-energy-and-industry.md Group5B
 */

#ifndef TESTS_GEOTHERMAL_RISER_FIXTURE_H
#define TESTS_GEOTHERMAL_RISER_FIXTURE_H

#include "excavation_fixture.h"
#include "../geothermal_riser.h"
#include "../industry.h"
#include "../industrytype.h"
#include "../industry_cmd.h"
#include "../command_func.h"
#include "../layer_portal_cmd.h"
#include "../layer_portal_map.h"
#include "../rail_map.h"
#include "../rail.h"
#include "../rail_cmd.h"
#include "../road.h"
#include "../road_cmd.h"
#include "../road_map.h"
#include "../depot_base.h"
#include "../station_base.h"
#include "../station_map.h"
#include "../track_func.h"
#include "../newgrf_commons.h"
#include "../utility_power.h"
#include "../town.h"
#include "../news_func.h"
#include "../timer/timer_game_tick.h"
#include "../core/backup_type.hpp"
#include "../safeguards.h"

/** Dispatch actual all-company native construction; fixtures advance its synchronized clock explicitly. */
void OnTick_Industry();

/** Actual company, native industry/stock and reciprocal portal owners; supplied floor/materials do not prove the later paid transported bootstrap. */
class NativeRiserRig : public ExcavationFixture {
	decltype(_industry_mngr.mappings) industries = _industry_mngr.mappings; ///< Caller canonical native identity allocations.
	decltype(_industile_mngr.mappings) tiles = _industile_mngr.mappings; ///< Caller native machinery tile identities.
	uint64_t tick = TimerGameTick::counter; ///< Caller simulation clock restored after each focused boundary.
public:
	GeothermalRiserRequest request; ///< Actual command geometry on the sending rock plane.
	TileIndex destination = INVALID_TILE; ///< Actual reciprocal receiving housing.
	/** @param direct_elevator Use direct Deep/Surface road access instead of Deep/Underground rail. @param serialized_native_slots Match shipped cargo slots for native file tests. Install definitions and command-built portal/approach inputs on explicitly supplied floor, without inventing machinery funding or work. */
	explicit NativeRiserRig(bool direct_elevator = false, bool serialized_native_slots = false) : ExcavationFixture(serialized_native_slots)
	{
		REQUIRE(Industry::GetNumItems() == 0);
		REQUIRE(Town::GetNumItems() == 0);
		REQUIRE(Depot::GetNumItems() == 0);
		ResetIndustries(); ResetGeothermalRisers(); ResetUtilityPowerState(); ResetLayerPortalWorkState();
		ResetRailTypes(); ResetRoadTypes();
		REQUIRE(InitialiseGeothermalRiserDefinitions(this->ActivatedContent()));
		this->request = {_current_company, TileXYL(20, 20, MapLayer::DeepUnderground), TileXYL(21, 20, MapLayer::DeepUnderground),
			TileXYL(21, 20, MapLayer::DeepUnderground), DiagDirection::SE};
		const auto upper = direct_elevator ? MapLayer::Surface : MapLayer::Underground;
		this->destination = MultiLayerMap::TileInLayer(this->request.portal, upper);
		std::vector<TileIndex> floor;
		// Explicit fixture floor supplies both actual rock endpoints and the
		// complete loading approach; direct elevators leave Underground solid.
		for (const auto layer : {MapLayer::DeepUnderground, upper}) {
			if (layer == MapLayer::Surface) continue;
			for (uint y = 18; y <= 24; ++y) {
				for (uint x = 18; x <= 24; ++x) floor.push_back(TileXYL(x, y, layer));
			}
		}
		this->Open(floor);
		REQUIRE(Town::CanAllocateItem()); Town::Create(TileXYL(10, 10, MapLayer::Surface)); RebuildTownKdtree();
		REQUIRE(CmdBuildLayerPortal(DoCommandFlag::Execute, this->destination, direct_elevator ? TransportType::Road : TransportType::Rail,
			MapLayer::DeepUnderground, DiagDirection::SE, RAILTYPE_RAIL, ROADTYPE_ROAD).Succeeded());
		// Real native approach commands install the same rail/road geometry and
		// infrastructure counts used by later sealing, takeover and demolition.
		for (const auto endpoint : {this->request.portal, this->destination}) {
			const auto door = GetLayerPortalDoorTile(endpoint);
			if (direct_elevator) {
				REQUIRE(CmdBuildRoad(DoCommandFlag::Execute, door, AxisToRoadBits(DiagDirToAxis(GetLayerPortalShaftDirection(endpoint))),
					ROADTYPE_ROAD, DisallowedRoadDirections{}, TownID::Invalid()).Succeeded());
			} else {
				REQUIRE(CmdBuildSingleRail(DoCommandFlag::Execute, door, RAILTYPE_RAIL, AxisToTrack(DiagDirToAxis(GetLayerPortalShaftDirection(endpoint))), false).Succeeded());
			}
		}
		REQUIRE(ChangeLayerPortalConnection(this->request.portal, _current_company, true, true) == LayerPortalWorkError::None);
	}
	/** Delete only actual fixture machinery before its company/map owners and restore native definitions/clock; retained caller inputs remain untouched. */
	~NativeRiserRig()
	{
		for (Industry *industry : Industry::Iterate()) delete industry;
		_station_pool.CleanPool();
		for (Depot *depot : Depot::Iterate()) delete depot;
		InitNewsItemStructs(); _town_pool.CleanPool(); RebuildTownKdtree();
		ResetGeothermalRisers(); ResetUtilityPowerState(); ResetLayerPortalWorkState(); ResetIndustries();
		_industry_mngr.mappings = std::move(this->industries);
		_industile_mngr.mappings = std::move(this->tiles);
		TimerGameTick::counter = this->tick;
	}
	/** @param estimate_only Select ordinary command-kernel estimate rather than capital settlement. @return Whole native funding result, with no fabricated machinery or stock. */
	CommandCost Fund(bool estimate_only = false)
	{
		return Command<Commands::BuildGeothermalRiser>::Unsafe<CommandCallback>(INVALID_STRING_ID, nullptr, false, estimate_only, this->request.tile,
			std::make_tuple(this->request.tile, this->request.portal, this->request.port, this->request.access, uint32_t{123}));
	}
	/** @return Actual native machinery funded through the real command, or a failed assertion; no fixture-side industry is fabricated. */
	Industry &Riser()
	{
		const auto id = FindGeothermalRiserForPortal(this->request.portal);
		REQUIRE(Industry::IsValidID(id));
		return *Industry::Get(id);
	}
	/** @param label Frozen delivered construction meaning. @param offered Explicit boundary input, distinct from later driven procurement. @return Exact native whole-unit stock/history receipt. */
	uint32_t Deliver(CargoLabel label, uint32_t offered)
	{
		return ReceiveGeothermalRiserCargo(this->Riser(), GetCargoTypeByLabel(label), offered);
	}
	/** Deliver the full frozen bill through native receipt owners; this explicit boundary supply is separate from the real-packet tests and later driven bootstrap. */
	void DeliverBill()
	{
		// Every canonical input receives its own actual native stock slot; no
		// fixture receipt, work flag or construction-completion bit is fabricated.
		for (const auto &input : GetMultiLayerGeothermalConstruction(MultiLayerGeothermalConstructionKind::Riser)->inputs) {
			REQUIRE(this->Deliver(input.label, input.quantity) == input.quantity);
		}
	}
	/** @param tile Explicit empty fixture floor outside machinery/public approaches. @return Actual layer-local rail station with rebuilt native industry catchment, deleted by fixture teardown. */
	Station *StationAt(TileIndex tile)
	{
		REQUIRE(Station::CanAllocateItem());
		if (GetTileLayer(tile) != MapLayer::Surface) this->Open(std::array{tile});
		Station *station = Station::Create(tile);
		station->owner = _current_company;
		station->town = *Town::Iterate().begin();
		station->facilities.Set(StationFacility::Train);
		MakeRailStation(tile, _current_company, station->index, Axis::X, 0, RAILTYPE_RAIL);
		station->train_station = TileArea(tile, 1, 1);
		station->spread.Add(tile);
		station->RecomputeCatchment();
		return station;
	}
	/** @param count Native synchronized ticks to dispatch. Advance only the simulation clock; the actual industry owner earns work and consumes delivered stock. */
	void Tick(uint16_t count = 1)
	{
		AutoRestoreBackup pause(_pause_mode, PauseModes{});
		// Actual all-company ticks retain the once-only labour guard, and no
		// direct state/receipt/completion assignment substitutes for construction.
		for (uint16_t index = 0; index < count; ++index) { ++TimerGameTick::counter; OnTick_Industry(); }
	}
};

#endif /* TESTS_GEOTHERMAL_RISER_FIXTURE_H */
