/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file normal_ground_readiness_fixture.h Actual native funding/Surface readiness on untouched normal starting worlds.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs16-integrated-acceptance-and-balance.md Group6C.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
 */
#ifndef TESTS_NORMAL_GROUND_READINESS_FIXTURE_H
#define TESTS_NORMAL_GROUND_READINESS_FIXTURE_H

#include "normal_ground_newgame_fixture.h"
#include "../industry.h"
#include "../industry_cmd.h"
#include "../rail_cmd.h"
#include "../station_cmd.h"
#include "../spoil_receiver.h"
#include "../tbm.h"
#include "../newgrf_station.h"
#include <charconv>

/** @param action Exact proposed ordinary construction. @param tile Actual generated Surface cell. @param cost Pure native quote/refusal. */
inline void RecordNormalGroundQuote(std::string_view action, TileIndex tile, const CommandCost &cost)
{
	fmt::print("NORMAL_GROUND_QUOTE action={} tile={} ok={} cost={} error={}\n", action, tile.base(), cost.Succeeded() ? 1 : 0,
		cost.GetCost().base(), cost.GetErrorMessage().base());
}

/** @param portal Proposed naturally generated Surface entrance. @param show Retain all exact quotes,or stop at the first actual native refusal. @return Whether the complete proposed yard passes native estimates. */
inline bool QuoteNormalGroundYard(TileIndex portal, bool show)
{
	const int x = static_cast<int>(TileX(portal)), y = static_cast<int>(TilePlaneY(portal));
	const auto at = [&](int dx, int dy) { return TileXYL(static_cast<uint>(x + dx), static_cast<uint>(y + dy), MapLayer::Surface); };
	bool accepted = true;
	const auto record = [&](std::string_view action, TileIndex tile, const CommandCost &cost) {
		accepted &= cost.Succeeded();
		if (show) RecordNormalGroundQuote(action, tile, cost);
		return show || cost.Succeeded();
	};
	if (!record("portal", portal, Command<Commands::BuildLayerPortal>::Do({}, portal, TransportType::Rail,
		MapLayer::Underground, DiagDirection::SE, RAILTYPE_RAIL, ROADTYPE_ROAD))) return false;
	if (!record("station", at(0, -6), Command<Commands::BuildRailStation>::Do({}, at(0, -6), RAILTYPE_RAIL,
		Axis::Y, uint8_t{1}, uint8_t{3}, STAT_CLASS_DFLT, uint16_t{0}, NEW_STATION, false))) return false;
	if (!record("depot", at(0, -10), Command<Commands::BuildRailDepot>::Do({}, at(0, -10), RAILTYPE_RAIL, DiagDirection::SE))) return false;
	for (int dy = -9; dy < 0; ++dy) {
		if (dy >= -6 && dy < -3) continue;
		if (!record("rail", at(0, dy), Command<Commands::BuildRail>::Do({}, at(0, dy), RAILTYPE_RAIL, Track::Y, false))) return false;
	}
	for (const auto kind : {SpoilReceiverKind::Earth, SpoilReceiverKind::Rock}) {
		const auto type = GetSpoilReceiverIndustryType(kind);
		REQUIRE(type != IT_INVALID);
		uint width = 0;
		for (const auto &cell : GetIndustrySpec(type)->layouts.front()) width = std::max<uint>(width, cell.ti.x);
		const auto tile = at(-static_cast<int>(width) - 2, kind == SpoilReceiverKind::Earth ? -7 : -2);
		if (!record(kind == SpoilReceiverKind::Earth ? "earth" : "rock", tile,
			Command<Commands::BuildIndustry>::Do({}, tile, type, uint32_t{0}, true, uint32_t{1234}))) return false;
	}
	return accepted;
}

/** Quote the exact normal start's ordinary logistics and credit without paying/building or replacing any native owner. */
inline void QuoteNormalGroundReadiness()
{
	REQUIRE(_game_mode == GameMode::Normal);
	REQUIRE_FALSE(_networking);
	REQUIRE(Company::IsValidID(_local_company));
	const auto *company = Company::Get(_local_company);
	const char *entrance_text = std::getenv("OTTD_NORMAL_GROUND_ENTRANCE");
	REQUIRE(entrance_text != nullptr);
	const std::string_view entrance_argument{entrance_text};
	uint32_t entrance_value = 0;
	const auto parsed = std::from_chars(entrance_argument.data(), entrance_argument.data() + entrance_argument.size(), entrance_value);
	REQUIRE(parsed.ec == std::errc{});
	REQUIRE(parsed.ptr == entrance_argument.data() + entrance_argument.size());
	auto portal = TileIndex{entrance_value};
	REQUIRE(GetTileLayer(portal) == MapLayer::Surface);
	const uint x = TileX(portal), y = TilePlaneY(portal);
	REQUIRE(x >= 12);
	REQUIRE(y >= 12);
	const char *stage_text = std::getenv("OTTD_NORMAL_GROUND_STAGE");
	REQUIRE(stage_text != nullptr);
	const std::string_view stage{stage_text};
	REQUIRE((stage == "quote" || stage == "yard"));
	const auto paths = GetTestCaseArtifactPaths("acceptance", "normal-ground-readiness");
	REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(paths.Output("saves", "readiness-before.sav")), false) == SaveLoadResult::Ok);
	const auto before = std::tuple{company->money, company->current_loan, TimerGameTick::counter,
		_random.state[0], _random.state[1], ComputeGeologyStateDigest(GetGeologyWorldState()),
		Industry::GetNumItems(), Vehicle::GetNumItems()};
	AutoRestoreBackup actor{_current_company, _local_company};
	uint candidates = 0;
	bool selected = stage == "quote";
	if (stage == "yard") {
		portal = INVALID_TILE;
		for (uint row = 12; row + 12 < Map::SizeY() && portal == INVALID_TILE; ++row) {
			for (uint column = 12; column + 12 < Map::SizeX(); ++column) {
				const auto candidate = TileXYL(column, row, MapLayer::Surface);
				if (!IsTileType(candidate, TileType::Clear) || GetTileSlope(candidate) != SLOPE_FLAT) continue;
				++candidates;
				if (!QuoteNormalGroundYard(candidate, false)) continue;
				portal = candidate;
				selected = true;
				break;
			}
		}
		// A generated map need not contain this whole unprepared straight
		// yard. Retain the original exact refusal matrix as characterization;
		// paid terraforming/readiness remains a later required producer.
		if (!selected) portal = TileIndex{entrance_value};
	}
	const auto accepted = QuoteNormalGroundYard(portal, true);
	if (stage == "yard" && selected) CHECK(accepted);
	for (const auto kind : {SpoilReceiverKind::Earth, SpoilReceiverKind::Rock}) {
		const auto type = GetSpoilReceiverIndustryType(kind);
		fmt::print("NORMAL_GROUND_RECEIVER kind={} type={} capital={}\n", to_underlying(kind), type, GetIndustrySpec(type)->GetConstructionCost().base());
	}
	for (const auto &model : GetTBMCutterModels()) {
		fmt::print("NORMAL_GROUND_MODEL id={} purchase={} active={} idle={}\n", model.model_id, model.purchase_cost.base(),
			model.active_cost_per_year.base(), model.idle_cost_per_year.base());
	}
	CHECK((std::tuple{company->money, company->current_loan, TimerGameTick::counter,
		_random.state[0], _random.state[1], ComputeGeologyStateDigest(GetGeologyWorldState()),
		Industry::GetNumItems(), Vehicle::GetNumItems()} == before));
	REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(paths.Output("saves", "readiness-after.sav")), false) == SaveLoadResult::Ok);
	fmt::print("NORMAL_GROUND_READINESS stage={} portal={} cash={} loan={} max_loan={} tick={} candidates={} accepted={} selected={}\n", stage, portal.base(),
		company->money.base(), company->current_loan.base(), _economy.max_loan.base(), TimerGameTick::counter, candidates, accepted ? 1 : 0, selected ? 1 : 0);
	std::fflush(stdout);
}

/** Load the exact supplied original through the ordinary application and dispatch one bounded readiness stage. */
inline void RunNormalGroundReadiness()
{
	const char *input = std::getenv("OTTD_NORMAL_GROUND_INPUT");
	REQUIRE(input != nullptr);
	bool observed = false;
	std::exception_ptr failure;
	std::function<void()> observe = [&]() {
		try { QuoteNormalGroundReadiness(); observed = true; } catch (...) { failure = std::current_exception(); }
		_exit_game = true;
	};
	AutoRestoreBackup hook{_normal_ground_newgame_observer, &observe};
	IConsole::CmdRegister("normal_ground_ready", ObserveNormalGroundNewgame);
	std::vector<std::string> owned = {"openttd_test.exe", "-x", "-X", "-c", GetTestRuntimeConfig().string(),
		"-v", "win32:no_threads", "-b", "8bpp-simple", "-r", "640x480", "-s", "null", "-m", "null",
		"-I", "original_dos", "-d", "misc=4,sl=1", "-g", input};
	std::vector<std::string_view> arguments;
	for (const auto &value : owned) arguments.push_back(value);
	CHECK(openttd_main(arguments) == 0);
	if (failure) std::rethrow_exception(failure);
	REQUIRE(observed);
}

#endif /* TESTS_NORMAL_GROUND_READINESS_FIXTURE_H */
