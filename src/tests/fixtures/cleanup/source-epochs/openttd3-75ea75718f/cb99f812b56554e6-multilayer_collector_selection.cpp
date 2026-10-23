/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file multilayer_collector_selection.cpp Actual collector cell/stage/Heat presentation and read-only boundaries.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group79C
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md
 */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "geothermal_collector_fixture.h"
#include "utility_power_fixture.h"
#include "../multilayer_sprite.h"
#include "../core/random_func.hpp"
#include "../safeguards.h"

/**
 * Check every actual collector cell/part without changing any gameplay owner.
 * @param industry Actual command-funded machinery; its location supplies physical cell indices.
 * @param state Expected construction/ready/publishing/blocked presentation.
 * @param frame Expected native construction stage or visual equipment phase.
 * @param view Actual access-side orientation, independent of camera position.
 * @param clock Read-only equipment clock used by the selector.
 */
static void CheckCollectorCells(const Industry &industry, uint8_t state, uint8_t frame, uint8_t view, uint64_t clock)
{
	const auto construction = ComputeGeothermalCollectorDigest(), sites = ComputeGeothermalSiteDigest();
	const auto geology = ComputeGeologyStateDigest(GetGeologyWorldState()), utility = ComputeUtilityPowerDigest();
	const auto money = Company::Get(_current_company)->money;
	const auto tick = TimerGameTick::counter;
	SavedRandomSeeds before; SaveRandomSeeds(&before);
	// All nine real cells retain the same native origin and aligned parts. A
	// render query cannot finish construction, consume inputs or publish Heat.
	for (uint y = 0; y < industry.location.h; ++y) {
		for (uint x = 0; x < industry.location.w; ++x) {
			const auto tile = TileAddXY(industry.location.tile, x, y);
			const auto cell = static_cast<uint16_t>(y * industry.location.w + x);
			for (const bool front : {false, true}) {
				const auto expected = GetMultiLayerSprite(MultiLayerSpriteAsset::FacilityVentCollector,
					{state, frame, view, cell, static_cast<uint8_t>(front)});
				REQUIRE(expected.has_value());
				CHECK(GetMultiLayerCollectorSprite(tile, front, clock) == expected);
				CHECK(GetMultiLayerIndustrySprite(tile, front, clock) == expected);
			}
		}
	}
	SavedRandomSeeds after; SaveRandomSeeds(&after);
	CHECK(std::ranges::equal(before.random.state, after.random.state));
	CHECK(std::ranges::equal(before.interactive_random.state, after.interactive_random.state));
	CHECK(ComputeGeothermalCollectorDigest() == construction); CHECK(ComputeGeothermalSiteDigest() == sites);
	CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == geology); CHECK(ComputeUtilityPowerDigest() == utility);
	CHECK(Company::Get(_current_company)->money == money); CHECK(TimerGameTick::counter == tick);
}

/** Genuine access-side claims and paid construction select all views/stages; fixture-supplied inputs do not prove transported bootstrap. */
TEST_CASE("Collector art follows actual native cells access and paid construction stages", "[collector-art-selection]")
{
	// Each orientation is a separate actual command-funded industry and claim.
	// Recreating the isolated world avoids fabricating a graphics-owned rotation.
	for (uint8_t view = 0; view < 4; ++view) {
		GeothermalCollectorRig rig;
		UtilityPowerReset utility;
		REQUIRE(CancelGeothermalSite(rig.site) == GeothermalError::None);
		rig.request.access = static_cast<DiagDirection>(view);
		const auto claim = ClaimGeothermalSite(rig.request); REQUIRE(claim.error == GeothermalError::None); rig.site = claim.id;
		REQUIRE(rig.Fund().Succeeded());
		Industry &industry = rig.Collector();
		CHECK_FALSE(GetMultiLayerCollectorSprite(INVALID_TILE, false, 0));
		CHECK_FALSE(GetMultiLayerCollectorSprite(TileAddXY(industry.location.tile, 3, 0), false, 0));
		CheckCollectorCells(industry, 0, 0, view, UINT64_MAX);
		// Only the native construction participant consumes the exact whole bill
		// and earns labour; the source's amber construction phase stays fixed.
		for (const auto &input : GetMultiLayerGeothermalConstruction(MultiLayerGeothermalConstructionKind::Collector)->inputs) {
			REQUIRE(rig.Deliver(input.label, input.quantity) == input.quantity);
		}
		for (uint8_t stage = 1; stage < 4; ++stage) {
			rig.Tick(74);
			CheckCollectorCells(industry, 0, stage, view, UINT64_MAX);
		}
		rig.Tick(74); REQUIRE(IsGeothermalCollectorOperational(industry.index));
		CheckCollectorCells(industry, 1, 1, view, 8);
	}
}

/** Current real Heat publication chooses working lamps; stale/retired meters and abandoned machinery retain their actual refusal. */
TEST_CASE("Collector art reads current shared Heat without creating generation", "[collector-art-selection]")
{
	GeothermalCollectorRig rig;
	UtilityPowerReset utility;
	AutoRestoreBackup pause(_pause_mode, PauseModes{});
	REQUIRE(rig.Fund().Succeeded());
	for (const auto &input : GetMultiLayerGeothermalConstruction(MultiLayerGeothermalConstructionKind::Collector)->inputs) {
		REQUIRE(rig.Deliver(input.label, input.quantity) == input.quantity);
	}
	rig.Tick(296); Industry &industry = rig.Collector();
	CheckCollectorCells(industry, 1, 0, 1, 0);
	REQUIRE(PublishGeothermalCollectorHeat(industry.index, GetUtilityPowerInterval(), 1) == UtilityPowerError::None);
	// Different clocks animate authored glass only. No offer is consumed or
	// multiplied by repeated foreground/background queries or hidden cameras.
	for (const uint64_t clock : {uint64_t{0}, uint64_t{8}, uint64_t{24}, UINT64_MAX}) {
		CheckCollectorCells(industry, 2, GetMultiLayerVisualFrame(MultiLayerSpriteAsset::FacilityVentCollector, clock, false), 1, clock);
	}
	{
		// Only the synchronized utility driver closes and rotates intervals.
		// Empty allocations curtail this unused offer without inventing a consumer.
		AutoRestoreBackup system(_current_company, OWNER_DEITY);
		REQUIRE(AllocateUtilityHeat(GetUtilityPowerInterval(), {}) == UtilityPowerError::None);
		REQUIRE(AllocateUtilityPower(GetUtilityPowerInterval(), {}) == UtilityPowerError::None);
		REQUIRE(BeginUtilityPowerInterval() == UtilityPowerError::None);
	}
	CheckCollectorCells(industry, 1, 2, 1, 16);
	const auto producer = GetGeothermalCollectorState(industry.index)->heat_producer;
	REQUIRE(RemoveUtilityPowerProducer(producer) == UtilityPowerError::None);
	CheckCollectorCells(industry, 3, 3, 1, 24);
	ChangeGeothermalCollectorOwnership(_current_company, CompanyID::Invalid());
	CheckCollectorCells(industry, 3, 0, 1, 0);
}
