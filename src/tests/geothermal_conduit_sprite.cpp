/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file geothermal_conduit_sprite.cpp Actual paid utility cells own source branches/caps independently of camera, actor and energy.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group84BC.1.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
 * Explicit prepared floor is an isolated topology boundary; original paid-world rendering remains84BC.2/84C.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "excavation_fixture.h"
#include "utility_power_fixture.h"
#include "../command_func.h"
#include "../geothermal_conduit_sprite.h"
#include "../geothermal_utility.h"
#include "../geothermal_utility_cmd.h"
#include "../ground_sprite.h"
#include "../timer/timer_game_tick.h"
#include "../safeguards.h"

/** @param tile Actual world cell. @param domain Native paid network, independent of source variant ordering. @param remove Execute actual removal rather than capital installation. @return Real command-kernel result, including the ordinary company bill; no test-owned installed flag is assigned. */
static CommandCost ConduitArtCommand(TileIndex tile, UtilityEnergyDomain domain, bool remove = false)
{
	if (remove) return Command<Commands::RemoveGeothermalUtilityLink>::Unsafe<CommandCallback>(INVALID_STRING_ID, nullptr, false, false, tile, std::make_tuple(tile, domain));
	return Command<Commands::BuildGeothermalUtilityLink>::Unsafe<CommandCallback>(INVALID_STRING_ID, nullptr, false, false, tile, std::make_tuple(tile, domain));
}

/** @param tiles Actual prepared floor to observe, not another terrain owner. @return Exact shared/geology/native-floor/all-company cash/actor/clock/both-RNG signature surrounding a pure art read. */
static std::string CaptureConduitArtOwners(std::span<const TileIndex> tiles)
{
	std::string result = fmt::format("{}:{}:{}:{}:{}:{}:{}:{}", ComputeUtilityPowerDigest(), ComputeGeologyStateDigest(GetGeologyWorldState()), _current_company.base(), TimerGameTick::counter,
		_random.state[0], _random.state[1], _interactive_random.state[0], _interactive_random.state[1]);
	for (const auto *company : Company::Iterate()) result += fmt::format("/{}:{}", company->index.base(), company->money.base()); // Every actual wallet remains unchanged, including a company different from the viewing actor.
	// Querying/drawing a pipe cannot replace an existing native floor or any
	// transport field; public tile accessors avoid padding/private-map copies.
	for (const auto tile : tiles) {
		Tile native{tile}; const std::array<uint32_t, 10> fields{native.type(), native.height(), native.m1(), native.m2(), native.m3(), native.m4(), native.m5(), native.m6(), native.m7(), native.m8()};
		for (const auto field : fields) result += fmt::format(":{}", field);
	}
	return result;
}

/** @param tile Actual inspected world cell. @param masks Expected native NE/SE/SW/NW installed-neighbour masks in approved Heat0/Electricity1 source order; absence means no installed domain, while zero means four genuine caps. @param floor Actual surrounding native floor whose packed fields must remain exact. Verify the full branch/domain contract and unchanged simulation owners. */
static void CheckConduitArt(TileIndex tile, const std::array<std::optional<uint8_t>, 2> &masks, std::span<const TileIndex> floor)
{
	const auto before = CaptureConduitArtOwners(floor); const auto art = GetGeothermalConduitArt(tile);
	if (!masks[0].has_value() && !masks[1].has_value()) {
		CHECK_FALSE(art.has_value()); // No paid domain exists: an empty floor or cold coordinate cannot advertise even a capped installation.
	} else {
		REQUIRE(art.has_value());
		// Every domain/branch has its own original pose; enum Electricity0 must
		// not accidentally choose the source atlas's Heat0 variant or inherit
		// another domain's adjacency, ownership or power availability.
		for (uint8_t variant = 0; variant < masks.size(); ++variant) {
			for (uint8_t port = 0; port < 4; ++port) {
				if (!masks[variant].has_value()) { CHECK_FALSE(art->domains[variant][port].has_value()); continue; }
				const uint8_t state = (*masks[variant] & (1U << port)) != 0 ? 1 : 2;
				const auto expected = GetGroundSprite(GroundSpriteFamily::GeothermalConduit, {state, 0, port, variant}); REQUIRE(expected.has_value());
				CHECK(art->domains[variant][port] == expected);
			}
		}
	}
	CHECK(CaptureConduitArtOwners(floor) == before);
}

/** All16 physical masks in each real domain use genuine paid neighbours; actual removal/rebuild changes caps without resurrecting an old identity. */
TEST_CASE("Native geothermal conduit art follows every paid neighbour mask", "[geothermal-conduit-art-masks]")
{
	ExcavationFixture rig; UtilityPowerReset power;
	const uint8_t mask = GENERATE(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15);
	const auto domain = GENERATE(UtilityEnergyDomain::Heat, UtilityEnergyDomain::Electricity); const uint8_t variant = domain == UtilityEnergyDomain::Heat ? 0 : 1;
	const auto centre = TileXYL(25, 25, MapLayer::DeepUnderground); std::vector<TileIndex> floor;
	for (int y = -1; y <= 1; ++y) for (int x = -1; x <= 1; ++x) floor.push_back(TileAddWrap(centre, x, y)); // Explicit fixture floor supplies only this isolated nine-cell topology boundary.
	rig.Open(floor); CheckConduitArt(centre, {}, floor); REQUIRE(ConduitArtCommand(centre, domain).Succeeded());
	// Independent mask inputs install only their actual physical neighbours;
	// no registry row, source/rate, route, sprite or connection bit is fabricated.
	for (uint8_t port = 0; port < 4; ++port) {
		if ((mask & (1U << port)) == 0) continue;
		REQUIRE(ConduitArtCommand(TileAddByDiagDir(centre, static_cast<DiagDirection>(port)), domain).Succeeded());
	}
	std::array<std::optional<uint8_t>, 2> expected{}; expected[variant] = mask; CheckConduitArt(centre, expected, floor);
	const auto cache_builds = GetGeothermalUtilityCacheBuildCount(); CheckConduitArt(centre, expected, floor); CHECK(GetGeothermalUtilityCacheBuildCount() == cache_builds);
	// A real neighbour removal must cap the surviving end immediately; a paid
	// fresh repair reconnects visually while the original issued row stays dead.
	for (uint8_t port = 0; port < 4; ++port) {
		if ((mask & (1U << port)) == 0) continue;
		const auto neighbour = TileAddByDiagDir(centre, static_cast<DiagDirection>(port)); const auto old = GetGeothermalUtilityLink(neighbour, domain)->id;
		REQUIRE(ConduitArtCommand(neighbour, domain, true).Succeeded()); expected[variant] = static_cast<uint8_t>(mask & ~(1U << port)); CheckConduitArt(centre, expected, floor);
		REQUIRE(ConduitArtCommand(neighbour, domain).Succeeded()); CHECK(GetGeothermalUtilityLink(neighbour, domain)->id != old); CHECK_FALSE(GetUtilityPowerLink(old)->active);
		expected[variant] = mask; CheckConduitArt(centre, expected, floor); break;
	}
}

/** Coexisting actual Heat/electrical installations remain separate across companies and planes, even under another viewing actor and with no generation whatsoever. */
TEST_CASE("Native geothermal conduit art preserves domain company and rock plane separation", "[geothermal-conduit-art-domains]")
{
	ExcavationFixture rig; UtilityPowerReset power; const auto centre = TileXYL(25, 25, MapLayer::DeepUnderground); const auto upper = MultiLayerMap::TileInLayer(centre, MapLayer::Underground);
	std::vector<TileIndex> floor;
	for (const auto origin : {centre, upper}) for (int y = -1; y <= 1; ++y) for (int x = -1; x <= 1; ++x) floor.push_back(TileAddWrap(origin, x, y)); // Prepared floor is explicit on both planes; equal coordinates create no inter-layer installation.
	rig.Open(floor);
	REQUIRE(ConduitArtCommand(centre, UtilityEnergyDomain::Heat).Succeeded()); REQUIRE(ConduitArtCommand(centre, UtilityEnergyDomain::Electricity).Succeeded());
	REQUIRE(ConduitArtCommand(TileAddByDiagDir(centre, DiagDirection::NE), UtilityEnergyDomain::Heat).Succeeded());
	REQUIRE(ConduitArtCommand(TileAddByDiagDir(centre, DiagDirection::SE), UtilityEnergyDomain::Electricity).Succeeded());
	REQUIRE(Company::CanAllocateItem()); std::unique_ptr<Company> peer{Company::Create()}; peer->money = 1000000;
	{
		AutoRestoreBackup actor(_current_company, peer->index);
		REQUIRE(ConduitArtCommand(TileAddByDiagDir(centre, DiagDirection::SW), UtilityEnergyDomain::Heat).Succeeded());
		REQUIRE(ConduitArtCommand(TileAddByDiagDir(centre, DiagDirection::NW), UtilityEnergyDomain::Electricity).Succeeded());
		CheckConduitArt(centre, {uint8_t{1}, uint8_t{2}}, floor); // A foreign viewer sees the actual original company's connections; its neighbouring foreign pipes still cannot join them.
	}
	CheckConduitArt(centre, {uint8_t{1}, uint8_t{2}}, floor); CheckConduitArt(upper, {}, floor);
	REQUIRE(ConduitArtCommand(upper, UtilityEnergyDomain::Electricity).Succeeded()); CheckConduitArt(upper, {std::nullopt, uint8_t{0}}, floor);
	CHECK(ConduitArtCommand(upper, UtilityEnergyDomain::Heat).Failed());
	REQUIRE(ConduitArtCommand(TileAddByDiagDir(upper, DiagDirection::NE), UtilityEnergyDomain::Electricity).Succeeded()); CheckConduitArt(upper, {std::nullopt, uint8_t{1}}, floor);
	CheckConduitArt(centre, {uint8_t{1}, uint8_t{2}}, floor);
	CHECK_FALSE(GetGeothermalConduitArt(INVALID_TILE).has_value()); CHECK_FALSE(GetGeothermalConduitArt(MultiLayerMap::TileInLayer(centre, MapLayer::Surface)).has_value());
	CHECK(CaptureUtilityPowerState().producers.empty()); CHECK(CaptureUtilityPowerState().consumers.empty()); // Visible connectivity creates no supply, reservation or useful energy.
}
