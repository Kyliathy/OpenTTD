/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file geothermal_generator_fixture.h Actual powerhouse funding/stock/work on explicitly prepared test floor; full paid bootstrap remains Plan18 Group8F.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs18-geothermal-energy-and-industry.md Groups8C/8E.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group84BB.
 */
#ifndef TESTS_GEOTHERMAL_GENERATOR_FIXTURE_H
#define TESTS_GEOTHERMAL_GENERATOR_FIXTURE_H

#include "excavation_fixture.h"
#include "../geothermal_generator.h"
#include "../industry.h"
#include "../industry_cmd.h"
#include "../industrytype.h"
#include "../command_func.h"
#include "../newgrf_commons.h"
#include "../town.h"
#include "../news_func.h"
#include "../timer/timer_game_tick.h"
#include "../core/backup_type.hpp"
#include "../safeguards.h"

/** Dispatch actual native industry participants after explicitly advancing the synchronized test clock. */
void OnTick_Industry();

/** Real native company, original cold site and actual accepted stock/work; this fixture supplies no heat or electricity. */
class GeothermalGeneratorRig : public ExcavationFixture {
	decltype(_industry_mngr.mappings) industries = _industry_mngr.mappings; ///< Caller native industry identities.
	decltype(_industile_mngr.mappings) tiles = _industile_mngr.mappings; ///< Caller native tile identities.
	AutoRestoreBackup<uint64_t> clock{TimerGameTick::counter}; ///< Restore only explicit synchronized test work.
public:
	GeothermalSiteRequest request; ///< Actual original site, away from its vent; later heat requires a real pipe.
	GeothermalSiteID site{UINT32_MAX}; ///< Real bounded cold reservation consumed only by funding.
	/** @param serialized_native_slots Match actual shipped cargo slots for ordinary-save fixtures. @param model Canonical dated machinery; its fixture starts at its own introduction date, while boundary tests explicitly move the calendar before funding/work. @param access Actual cold-site approach direction, defaulting to the original SE; alternate art views still require native whole-site admission and funding. */
	explicit GeothermalGeneratorRig(bool serialized_native_slots = false, GeothermalGeneratorModel model = GeothermalGeneratorModel::VentTap, DiagDirection access = DiagDirection::SE) : ExcavationFixture(serialized_native_slots)
	{
		REQUIRE(Industry::GetNumItems() == 0); REQUIRE(Town::GetNumItems() == 0);
		ResetIndustries(); ResetGeothermalGenerators(); ResetGeothermalSiteClaims();
		REQUIRE(InitialiseGeothermalGeneratorDefinitions(this->ActivatedContent()));
		this->request.vent = GetGeologyWorldState().vents.front().id; this->request.owner = _current_company;
		const auto *operating = GetMultiLayerGeothermalGeneratorTerms(model); REQUIRE(operating != nullptr);
		this->request.kind = operating->construction;
		TimerGameCalendar::date = TimerGameCalendar::ConvertYMDToDate(GetMultiLayerGeothermalConstruction(this->request.kind)->introduction_year, 0, 1);
		this->request.tile = this->request.endpoint = TileXYL(36, 24, MapLayer::DeepUnderground); this->request.access = access;
		std::vector<TileIndex> floor;
		// The exact full floor and outward approach rim are fixture input. This
		// tests paid native machinery, not a claim that its footprint was TBM-cut.
		for (int y = -1; y <= 4; ++y) {
			for (int x = -1; x <= 4; ++x) floor.push_back(TileAddWrap(this->request.tile, x, y));
		}
		this->Open(floor);
		const auto claim = ClaimGeothermalSite(this->request); REQUIRE(claim.error == GeothermalError::None); this->site = claim.id;
		REQUIRE(Town::CanAllocateItem()); Town::Create(TileXYL(10, 10, MapLayer::Surface)); RebuildTownKdtree();
	}
	/** Retire actual native machinery before its original company/map/geology and restore definition identity. */
	~GeothermalGeneratorRig()
	{
		// Release every actual machinery/catchment binding before its native town and world disappear.
		for (Industry *industry : Industry::Iterate()) delete industry;
		InitNewsItemStructs(); _town_pool.CleanPool(); RebuildTownKdtree();
		ResetGeothermalGenerators(); ResetGeothermalSiteClaims(); ResetIndustries();
		_industry_mngr.mappings = std::move(this->industries); _industile_mngr.mappings = std::move(this->tiles);
	}
	/** @param estimate_only Select the ordinary kernel's pure estimate. @return Actual once-only native funding result and complete capital charge. */
	CommandCost Fund(bool estimate_only = false)
	{
		return Command<Commands::BuildGeothermalGenerator>::Unsafe<CommandCallback>(INVALID_STRING_ID, nullptr, false, estimate_only, this->request.tile,
			std::make_tuple(this->request.tile, this->site, uint32_t{123}));
	}
	/** @return Actual pool-owned funded machinery; no test substitutes a local industry or stock owner. */
	Industry &Generator()
	{
		REQUIRE(Industry::GetNumItems() == 1); return **Industry::Iterate().begin();
	}
	/** @param label Canonical real construction meaning. @param quantity Explicit receiver-boundary supply; later driven first-plant proof retains Group8F. @return Exact actual native stock/history receipt. */
	uint32_t Deliver(CargoLabel label, uint32_t quantity)
	{
		return ReceiveGeothermalGeneratorCargo(this->Generator(), GetCargoTypeByLabel(label), quantity);
	}
	/** Deliver only the existing frozen bill through actual stock/history receipt owners; no work or energy is granted. */
	void DeliverBill()
	{
		// Explicit receiver-boundary input covers the whole frozen bill, without buying or transporting it.
		for (const auto &input : GetMultiLayerGeothermalConstruction(this->request.kind)->inputs) {
			REQUIRE(this->Deliver(input.label, input.quantity) == input.quantity);
		}
	}
	/** @param count Actual synchronized native industry dispatcher opportunities; duplicate-tick behavior remains with the producer. */
	void Tick(uint16_t count)
	{
		AutoRestoreBackup pause{_pause_mode, PauseModes{}};
		// Earn each paid labour opportunity through the real dispatcher rather than editing progress.
		for (uint16_t index = 0; index < count; ++index) { ++TimerGameTick::counter; OnTick_Industry(); }
	}
};

#endif /* TESTS_GEOTHERMAL_GENERATOR_FIXTURE_H */
