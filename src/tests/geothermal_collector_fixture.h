/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file geothermal_collector_fixture.h Actual native funding/stock/work boundary fixture; explicitly supplied test floor does not prove transported paid excavation/bootstrap.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs17-geothermal-energy-and-industry.md Group1B
 */

#ifndef TESTS_GEOTHERMAL_COLLECTOR_FIXTURE_H
#define TESTS_GEOTHERMAL_COLLECTOR_FIXTURE_H

#include "excavation_fixture.h"
#include "../geothermal_collector.h"
#include "../industry.h"
#include "../industry_cmd.h"
#include "../industrytype.h"
#include "../newgrf_commons.h"
#include "../command_func.h"
#include "../town.h"
#include "../tile_cmd.h"
#include "../rail_map.h"
#include "../economy_func.h"
#include "../news_func.h"
#include "../core/backup_type.hpp"
#include "../safeguards.h"

/** Dispatch actual native industry participants once; fixture calls advance the synchronized clock explicitly. */
void OnTick_Industry();

/** Real native company, vent, industry and canonical cargo owners with declared fixture-open floor and explicit construction deliveries. */
class GeothermalCollectorRig : public ExcavationFixture {
	decltype(_industry_mngr.mappings) industries = _industry_mngr.mappings; ///< Caller native industry identities restored after isolated construction.
	decltype(_industile_mngr.mappings) tiles = _industile_mngr.mappings; ///< Caller native machinery identities.
	uint64_t tick = TimerGameTick::counter; ///< Caller simulation clock; fixture advances actual dispatcher ticks explicitly.
public:
	GeothermalSiteRequest request; ///< Original natural source and actual company/floor.
	GeothermalSiteID site{UINT32_MAX}; ///< Actual cold reservation, consumed only by native funding.
	/** @param serialized_native_slots Match distributed cargo slots for feature-owned real save fixtures. Install exact native definition and declare full excavated test floor without producing construction materials or energy. */
	explicit GeothermalCollectorRig(bool serialized_native_slots = false) : ExcavationFixture(serialized_native_slots)
	{
		REQUIRE(Industry::GetNumItems() == 0); REQUIRE(Town::GetNumItems() == 0);
		ResetIndustries(); ResetGeothermalCollectors(); ResetGeothermalSiteClaims();
		REQUIRE(InitialiseGeothermalCollectorDefinitions(this->ActivatedContent()));
		const auto &vent = GetGeologyWorldState().vents.front();
		this->request.owner = _current_company; this->request.vent = vent.id;
		this->request.tile = this->request.endpoint = TileXYL(vent.x, vent.y, vent.layer); this->request.access = DiagDirection::SE;
		std::vector<TileIndex> floor;
		// Unit boundary input supplies the whole floor/access only. Actual paid
		// excavation and Surface procurement/carriage retain master10.6 proof.
		for (int y = -1; y <= 3; ++y) {
			for (int x = -1; x <= 3; ++x) floor.push_back(TileAddWrap(this->request.tile, x, y));
		}
		this->Open(floor);
		const auto claim = ClaimGeothermalSite(this->request); REQUIRE(claim.error == GeothermalError::None); this->site = claim.id;
		REQUIRE(Town::CanAllocateItem()); Town::Create(TileXYL(10, 10, MapLayer::Surface)); RebuildTownKdtree();
	}
	/** Remove actual fixture machinery before its native map/company/geology owners, then restore canonical definitions, clock and caller policy. */
	~GeothermalCollectorRig()
	{
		for (Industry *industry : Industry::Iterate()) delete industry;
		InitNewsItemStructs(); _town_pool.CleanPool(); RebuildTownKdtree();
		ResetGeothermalCollectors(); ResetGeothermalSiteClaims(); ResetIndustries();
		_industry_mngr.mappings = std::move(this->industries); _industile_mngr.mappings = std::move(this->tiles); TimerGameTick::counter = this->tick;
	}
	/** @param estimate_only Select ordinary command-kernel pure estimate instead of actual charging/creation. @return Actual native complete command result, including once-only capital settlement. */
	CommandCost Fund(bool estimate_only = false)
	{
		return Command<Commands::BuildGeothermalCollector>::Unsafe<CommandCallback>(INVALID_STRING_ID, nullptr, false, estimate_only, this->request.tile,
			std::make_tuple(this->request.tile, this->site, uint32_t{123}));
	}
	/** @return Actual pool-owned collector funded by this fixture, or a failed assertion; no fixture industry/binding is fabricated. */
	Industry &Collector()
	{
		const auto id = FindGeothermalCollectorForVent(this->request.vent); REQUIRE(Industry::IsValidID(id)); return *Industry::Get(id);
	}
	/** @param count Actual synchronized simulation ticks to dispatch. No material or cash is fabricated, and duplicate calls within one tick remain subject to the producer's once-only meter. */
	void Tick(uint16_t count = 1)
	{
		AutoRestoreBackup pause(_pause_mode, PauseModes{});
		// Run native dispatcher ticks, rather than directly writing saved work or
		// construction flags; each actual industry receives one paid participant.
		for (uint16_t index = 0; index < count; ++index) { ++TimerGameTick::counter; OnTick_Industry(); }
	}
	/** @param label Frozen input meaning. @param offered Explicit unit-boundary delivered supply, distinct from later driven procurement proof. @return Exact actual native receipt, bounded by the one remaining bill. */
	uint32_t Deliver(CargoLabel label, uint32_t offered)
	{
		return ReceiveGeothermalCollectorCargo(this->Collector(), GetCargoTypeByLabel(label), offered);
	}
};

#endif /* TESTS_GEOTHERMAL_COLLECTOR_FIXTURE_H */
