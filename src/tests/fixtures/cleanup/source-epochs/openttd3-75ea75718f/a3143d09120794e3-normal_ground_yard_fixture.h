/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file normal_ground_yard_fixture.h Real paid Surface logistics on exact normally generated and financed worlds.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs16-integrated-acceptance-and-balance.md Group6CC.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
 */
#ifndef TESTS_NORMAL_GROUND_YARD_FIXTURE_H
#define TESTS_NORMAL_GROUND_YARD_FIXTURE_H

#include "normal_ground_freight_fixture.h"
#include "../landscape_cmd.h"
#include "../layer_portal_map.h"

/** Pay every actual predecessor at the native quoted site;leave both rock planes solid. */
inline void BuildNormalGroundYard()
{
	REQUIRE(_game_mode == GameMode::Normal); REQUIRE_FALSE(_networking);
	REQUIRE(Company::IsValidID(_local_company));
	AutoRestoreBackup actor{_current_company, _local_company};
	const auto *company = Company::Get(_current_company);
	const auto opening = company->money;
	REQUIRE(company->current_loan == Money{300000});
	const bool paused = _pause_mode.Test(PauseMode::Normal);
	if (paused) REQUIRE(Command<Commands::Pause>::Post(PauseMode::Normal, false));
	REQUIRE(_pause_mode.None());
	const auto geology = ComputeGeologyStateDigest(GetGeologyWorldState());
	const char *site_text = std::getenv("OTTD_NORMAL_GROUND_ENTRANCE"); REQUIRE(site_text != nullptr);
	const std::string_view argument{site_text}; uint32_t value = 0;
	const auto parsed = std::from_chars(argument.data(), argument.data() + argument.size(), value);
	REQUIRE(parsed.ec == std::errc{}); REQUIRE(parsed.ptr == argument.data() + argument.size());
	const auto portal = TileIndex{value};
	REQUIRE(GetTileLayer(portal) == MapLayer::Surface);
	const int x = TileX(portal), y = TilePlaneY(portal);
	REQUIRE(x >= 12); REQUIRE(y >= 12);
	const auto at = [&](int dx, int dy) { return TileXYL(x + dx, y + dy, MapLayer::Surface); };
	// Ordinary paid Surface clearing is a prerequisite of native housing's
	// Clear endpoint rule. It never opens destination rock or prepares a room.
	for (const auto tile : {portal, at(0, -1)}) {
		REQUIRE((IsTileType(tile, TileType::Clear) || IsTileType(tile, TileType::Trees)));
		if (IsTileType(tile, TileType::Trees)) PayNormalFreightCommand<Commands::LandscapeClear>("site_clear", tile);
	}
	const auto before_quote = std::tuple{company->money, company->current_loan, TimerGameTick::counter,
		_random.state[0], _random.state[1], ComputeGeologyStateDigest(GetGeologyWorldState())};
	REQUIRE(QuoteNormalGroundYard(portal, true));
	CHECK((std::tuple{company->money, company->current_loan, TimerGameTick::counter,
		_random.state[0], _random.state[1], ComputeGeologyStateDigest(GetGeologyWorldState())} == before_quote));
	std::array<IndustryID, 2> receivers{IndustryID::Invalid(), IndustryID::Invalid()};
	for (const auto kind : {SpoilReceiverKind::Earth, SpoilReceiverKind::Rock}) {
		const auto type = GetSpoilReceiverIndustryType(kind); REQUIRE(type != IT_INVALID);
		uint width = 0;
		for (const auto &cell : GetIndustrySpec(type)->layouts.front()) width = std::max<uint>(width, cell.ti.x);
		const auto site = at(-static_cast<int>(width) - 2, kind == SpoilReceiverKind::Earth ? -7 : -2);
		std::set<IndustryID> prior; for (const auto *industry : Industry::Iterate()) prior.insert(industry->index);
		PayNormalFreightCommand<Commands::BuildIndustry>(kind == SpoilReceiverKind::Earth ? "earth_receiver" : "rock_receiver",
			site, type, uint32_t{0}, true, uint32_t{1234});
		const Industry *created = nullptr;
		for (const auto *industry : Industry::Iterate()) if (!prior.contains(industry->index)) {
			REQUIRE(created == nullptr); created = industry;
		}
		REQUIRE(created != nullptr);
		CHECK(created->type == type); CHECK(created->founder == _current_company);
		const auto *receiver = GetSpoilReceiver(created->index);
		REQUIRE(receiver != nullptr); CHECK(receiver->kind == kind);
		CHECK(receiver->disposed_t == 0); CHECK(receiver->crushed_t == 0); CHECK(receiver->fees == 0);
		receivers[to_underlying(kind)] = created->index;
		fmt::print("NORMAL_GROUND_YARD_RECEIVER kind={} industry={} tile={}\n", to_underlying(kind), created->index.base(), site.base());
	}
	PayNormalFreightCommand<Commands::BuildRailStation>("yard_station", at(0, -6), RAILTYPE_RAIL,
		Axis::Y, uint8_t{1}, uint8_t{3}, STAT_CLASS_DFLT, uint16_t{0}, NEW_STATION, false);
	PayNormalFreightCommand<Commands::BuildRailDepot>("yard_depot", at(0, -10), RAILTYPE_RAIL, DiagDirection::SE);
	for (int offset = -9; offset < 0; ++offset) {
		if (offset >= -6 && offset < -3) continue;
		PayNormalFreightCommand<Commands::BuildRail>("yard_rail", at(0, offset), RAILTYPE_RAIL, Track::Y, false);
	}
	PayNormalFreightCommand<Commands::BuildLayerPortal>("portal", portal, TransportType::Rail,
		MapLayer::Underground, DiagDirection::SE, RAILTYPE_RAIL, ROADTYPE_ROAD);
	const auto origin = GetStationIndex(at(0, -6));
	const auto *station = Station::Get(origin);
	CHECK(station->owner == _current_company);
	for (const auto kind : {SpoilReceiverKind::Earth, SpoilReceiverKind::Rock}) {
		const auto cargo = GetCargoTypeByLabel(GetSpoilReceiverRecipe(kind).cargo);
		REQUIRE(IsValidCargoType(cargo));
		CHECK(station->goods[cargo].status.Test(GoodsEntry::State::Acceptance));
	}
	const auto target = MultiLayerMap::TileInLayer(portal, MapLayer::Underground);
	CHECK(IsTileType(portal, TileType::LayerPortal)); CHECK(IsTileType(target, TileType::LayerPortal));
	CHECK(GetLayerPortalWorkState(portal) == LayerPortalWorkState::Sealed);
	CHECK(GetGeologyWorldState().terrain.Get(target) == ExcavationState::Solid);
	CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == geology);
	CHECK(company->current_loan == Money{300000});
	if (paused) REQUIRE(Command<Commands::Pause>::Post(PauseMode::Normal, true));
	const auto paths = GetTestCaseArtifactPaths("acceptance", "normal-ground-yard");
	for (const auto name : {"yard-paid.sav", "yard-paid-repeat.sav"}) {
		REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(paths.Output("saves", name)), false) == SaveLoadResult::Ok);
	}
	fmt::print("NORMAL_GROUND_YARD portal={} target={} origin={} depot={} earth={} rock={} opening_cash={} cash={} loan={} tick={}\n",
		portal.base(), target.base(), origin.base(), at(0, -10).base(), receivers[0].base(), receivers[1].base(), opening.base(),
		company->money.base(), company->current_loan.base(), TimerGameTick::counter);
	std::fflush(stdout);
}

/** Load the funded original through the real application and dispatch public paid construction. */
inline void RunNormalGroundYard()
{
	const char *input = std::getenv("OTTD_NORMAL_GROUND_INPUT"); REQUIRE(input != nullptr);
	bool observed = false; std::exception_ptr failure;
	std::function<void()> build = [&]() {
		try { BuildNormalGroundYard(); observed = true; } catch (...) { failure = std::current_exception(); }
		_exit_game = true;
	};
	AutoRestoreBackup hook{_normal_ground_newgame_observer, &build};
	IConsole::CmdRegister("normal_ground_yard", ObserveNormalGroundNewgame);
	std::vector<std::string> owned = {"openttd_test.exe", "-x", "-X", "-c", GetTestRuntimeConfig().string(),
		"-v", "win32:no_threads", "-b", "8bpp-simple", "-r", "640x480", "-s", "null", "-m", "null",
		"-I", "original_dos", "-d", "misc=4,sl=1", "-g", input};
	std::vector<std::string_view> arguments; for (const auto &value : owned) arguments.push_back(value);
	CHECK(openttd_main(arguments) == 0);
	if (failure) std::rethrow_exception(failure);
	REQUIRE(observed);
}

#endif /* TESTS_NORMAL_GROUND_YARD_FIXTURE_H */
