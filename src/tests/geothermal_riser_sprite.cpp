/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file geothermal_riser_sprite.cpp Actual paid native riser work and shared electrical flow select original machinery art purely.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group84BB.2.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
 * Supplied floor/materials and explicit external electricity are isolated state boundaries, never paid transported bootstrap evidence.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "geothermal_electrical_fixture.h"
#include "../geothermal_sprite.h"
#include "../ground_sprite.h"
#include "../industry_map.h"
#include "../landscape_cmd.h"
#include "../safeguards.h"

/** @param owner Actual machinery company, independent of the viewing actor. @return Complete observed construction/stock/history/utility/geology/cash/clock/RNG signature before a pure presentation burst. */
static std::string CaptureRiserSpriteOwners(CompanyID owner)
{
	std::string result = fmt::format("{}:{}:{}:{}:{}:{}:{}:{}:{}", ComputeGeothermalRiserDigest(), ComputeUtilityPowerDigest(),
		ComputeGeologyStateDigest(GetGeologyWorldState()), Company::Get(owner)->money.base(), TimerGameTick::counter,
		_random.state[0], _random.state[1], _interactive_random.state[0], _interactive_random.state[1]);
	// Every actual native stock slot and valid receipt period remains with its
	// Industry. Painting cannot debit materials or manufacture hidden history.
	for (const Industry *industry : Industry::Iterate()) {
		result += fmt::format("/{}:{}:{}", industry->index.base(), industry->counter, industry->valid_history);
		for (const auto &slot : industry->accepted) {
			result += fmt::format(":{}:{}:{}:{}", slot.cargo, slot.waiting, slot.accumulated_waiting, slot.last_accepted.base());
			if (slot.history == nullptr) continue;
			for (uint month = 0; month < slot.history->size(); ++month) {
				if (month != THIS_MONTH && (industry->valid_history & (uint64_t{1} << month)) == 0) continue; // Unwritten history has no saved receipt authority and cannot become test-owned state.
				result += fmt::format(":{}", (*slot.history)[month].accepted);
			}
		}
	}
	return result;
}

/** @param id Actual command-funded native riser. @param expected_state Independently required source state for the real work/portal/transfer boundary. Require all fixed world cells/parts/glass phases and unchanged native owners. */
static void CheckRiserSpriteCells(IndustryID id, uint8_t expected_state)
{
	const auto *native = GetGeothermalRiserState(id); REQUIRE(native != nullptr); const auto &industry = *Industry::Get(id);
	const auto before = CaptureRiserSpriteOwners(native->site.owner);
	// The one fixed2x2 layout does not rotate with access. Neither querying all
	// original parts nor changing only visual phase earns work or electrical flow.
	for (const auto visual : {uint64_t{0}, uint64_t{8}, uint64_t{16}, uint64_t{24}}) {
		for (uint y = 0; y < industry.location.h; ++y) {
			for (uint x = 0; x < industry.location.w; ++x) {
				const auto tile = TileAddWrap(industry.location.tile, x, y);
				const uint8_t frame = expected_state == 0 ? GetIndustryConstructionStage(tile) : GetGroundVisualFrame(GroundSpriteFamily::FacilityUtilityRiser, visual);
				for (const bool front : {false, true}) {
					const auto expected = GetGroundSprite(GroundSpriteFamily::FacilityUtilityRiser, {expected_state, frame, 0, static_cast<uint16_t>(y * industry.location.w + x), static_cast<uint8_t>(front)});
					REQUIRE(expected.has_value()); CHECK(GetGeothermalRiserSprite(tile, front, visual) == expected); CHECK(GetMultiLayerIndustrySprite(tile, front, visual) == expected);
				}
				const auto bounds = GetGeothermalIndustryBackBounds(tile); REQUIRE(bounds.has_value()); CHECK(bounds->extent.x == TILE_SIZE); CHECK(bounds->extent.y == TILE_SIZE); CHECK(bounds->extent.z == 64);
			}
		}
	}
	CHECK(CaptureRiserSpriteOwners(native->site.owner) == before);
}

/** Actual rail and direct-road funding/receipts/work earn every construction quarter and readiness without granting an electrical link. */
TEST_CASE("Native riser art preserves paid construction and fixed original footprint", "[geothermal-riser-art-construction]")
{
	const bool direct = GENERATE(false, true); NativeRiserRig rig{direct}; REQUIRE(rig.Fund().Succeeded()); const auto id = rig.Riser().index;
	CheckRiserSpriteCells(id, 0); rig.DeliverBill();
	for (uint stage = 1; stage < 4; ++stage) { // Each genuine74-tick paid quarter selects construction hardware; no completed tile bits are assigned by the test.
		rig.Tick(74); CheckRiserSpriteCells(id, 0);
	}
	rig.Tick(74); REQUIRE(IsGeothermalRiserOperational(id)); CheckRiserSpriteCells(id, 1); CHECK(GetUtilityPowerLinks().empty());
	CHECK_FALSE(GetGeothermalRiserSprite(INVALID_TILE, false, 0).has_value()); CHECK_FALSE(GetGeothermalIndustryBackBounds(INVALID_TILE).has_value());
	CHECK_FALSE(GetGeothermalRiserSprite(rig.destination, false, 0).has_value()); CHECK_FALSE(GetGeothermalIndustryBackBounds(rig.destination).has_value());
	{
		AutoRestoreBackup actor(_current_company, OWNER_NONE); CheckRiserSpriteCells(id, 1); // Another viewing actor cannot remove the actual company's earned readiness.
	}
	{
		AutoRestoreBackup date(TimerGameCalendar::date, TimerGameCalendar::ConvertYMDToDate(TimerGameCalendar::Year{1979}, TimerGameCalendar::Month{0}, TimerGameCalendar::Day{1}));
		CheckRiserSpriteCells(id, 3);
	}
}

/** Only real shared transfers animate both paid relay machines; interval expiry, sealing/reopening and native IndustryID reuse cannot retain old working art. */
TEST_CASE("Native riser art follows original electrical flow and physical lifecycle", "[geothermal-riser-art-flow]")
{
	NativeElectricalRelayRig rig{false, true}; const auto lower = rig.Riser().index;
	CheckRiserSpriteCells(lower, 1); CheckRiserSpriteCells(rig.upper_industry, 1); rig.InstallSourceCable(); rig.InstallRelayCable();
	{
		AutoRestoreBackup system(_current_company, OWNER_DEITY);
		REQUIRE(AllocateUtilityPower(GetUtilityPowerInterval(), std::array{UtilityPowerRequest{rig.source, 500000}, UtilityPowerRequest{rig.relay, 0}, UtilityPowerRequest{rig.destination_account, 500000}}) == UtilityPowerError::None);
	}
	const auto proposal = PrepareGeothermalPowerExport({{rig.source, GetUtilityPowerInterval()}, rig.destination_account, rig.source_port, 500000}); REQUIRE(proposal.error == UtilityPowerError::None);
	REQUIRE(ExportGeothermalPower(proposal).error == UtilityPowerError::None); REQUIRE(GetUtilityPowerLink(rig.first)->flow_j > 0); REQUIRE(GetUtilityPowerLink(rig.second)->flow_j > 0);
	CheckRiserSpriteCells(lower, 2); CheckRiserSpriteCells(rig.upper_industry, 2);
	{
		AutoRestoreBackup system(_current_company, OWNER_DEITY); REQUIRE(BeginUtilityPowerInterval() == UtilityPowerError::None);
	}
	CheckRiserSpriteCells(lower, 1); CheckRiserSpriteCells(rig.upper_industry, 1);
	REQUIRE(ChangeLayerPortalConnection(rig.request.portal, rig.request.owner, false, true) == LayerPortalWorkError::None); CheckRiserSpriteCells(lower, 3);
	REQUIRE(ChangeLayerPortalConnection(rig.request.portal, rig.request.owner, true, true) == LayerPortalWorkError::None); CheckRiserSpriteCells(lower, 1); CHECK_FALSE(GetUtilityPowerLink(rig.first)->active);
	REQUIRE(Command<Commands::LandscapeClear>::Do(DoCommandFlag::Execute, rig.request.tile).Succeeded());
	CHECK_FALSE(GetGeothermalRiserSprite(rig.request.tile, false, 0).has_value()); CHECK_FALSE(GetGeothermalIndustryBackBounds(rig.request.tile).has_value());
	// Demolition preserves installed cable ownership. Rebuilding must remove
	// the surviving paid port through its real command before funding a new
	// empty machinery footprint; the native occupancy guard remains intact.
	REQUIRE(ElectricalCableCommand(rig.request.port, UtilityEnergyDomain::Electricity, true).Succeeded());
	REQUIRE(rig.Fund().Succeeded()); CHECK(rig.Riser().index == lower); CheckRiserSpriteCells(lower, 0); CHECK_FALSE(GetUtilityPowerLink(rig.first)->active);
}
