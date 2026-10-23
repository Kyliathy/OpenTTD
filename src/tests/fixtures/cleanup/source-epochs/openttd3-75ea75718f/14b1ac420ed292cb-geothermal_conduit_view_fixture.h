/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file geothermal_conduit_view_fixture.h Pure ordinary paid-world conduit topology and native ground/body observations.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group84BC.2.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
 * Borrow issued local-cell records and native display options; never construct a convenient replacement world or utility owner.
 */
#ifndef TESTS_GEOTHERMAL_CONDUIT_VIEW_FIXTURE_H
#define TESTS_GEOTHERMAL_CONDUIT_VIEW_FIXTURE_H
#include "native_art_render_fixture.h"
#include "../3rdparty/nlohmann/json.hpp"
#include "../geothermal_conduit_sprite.h"
#include "../geothermal_utility.h"
#include "../multilayer_sprite.h"
#include <map>
#include <set>
#include <tuple>

/** @param tile Original paid physical cell. @return Current native geometry and independently borrowed issued domain/neighbour identities beside the actual selected branch sprites. */
static nlohmann::json ObserveGeothermalConduitCell(TileIndex tile)
{
	const auto art = GetGeothermalConduitArt(tile); REQUIRE(art.has_value());
	nlohmann::json domains = nlohmann::json::array();
	// Original shared records, rather than sprite offsets, identify the paid
	// domains and their adjacent participants for the independent pixel reader.
	for (const auto domain : {UtilityEnergyDomain::Heat, UtilityEnergyDomain::Electricity}) {
		const auto installed = GetGeothermalUtilityLink(tile, domain); if (!installed.has_value()) continue;
		const uint8_t variant = domain == UtilityEnergyDomain::Heat ? 0 : 1; nlohmann::json neighbours = nlohmann::json::array(), sprites = nlohmann::json::array();
		for (uint8_t direction = 0; direction < 4; ++direction) {
			const auto offset = TileIndexDiffCByDiagDir(static_cast<DiagDirection>(direction)); const auto other = TileAddWrap(tile, offset.x, offset.y);
			const auto adjacent = IsValidTile(other) ? GetGeothermalUtilityLink(other, domain) : std::nullopt;
			neighbours.push_back(adjacent.has_value() ? nlohmann::json{{"id", adjacent->id.base()}, {"owner", adjacent->owner.base()}, {"tile", other.base()}} : nlohmann::json{});
			REQUIRE(art->domains[variant][direction].has_value()); sprites.push_back(*art->domains[variant][direction]);
		}
		domains.push_back({{"variant", variant}, {"id", installed->id.base()}, {"owner", installed->owner.base()}, {"neighbours", neighbours}, {"sprites", sprites}});
	}
	CHECK_FALSE(GetGeothermalConduitArt(MultiLayerMap::TileInLayer(tile, MapLayer::Surface)).has_value());
	return {{"tile", tile.base()}, {"x", TileX(tile)}, {"y", TilePlaneY(tile)}, {"z", TilePixelHeight(tile)}, {"layer", to_underlying(GetTileLayer(tile))},
		{"type", to_underlying(GetTileType(tile))}, {"domains", domains}};
}

/** @param paths Current ordinary observer's routed images/reports. Capture bounded representatives of every actual installed layer/floor/domain combination, plus complete issued topology, without commands or simulation steps. */
static void CaptureGeothermalConduitViews(const TestArtifactPaths &paths)
{
	std::set<TileIndex> cells; const auto digest = ComputeUtilityPowerDigest();
	// Iterate issued links only. Retired rows remain terminal and nonlocal
	// riser/export edges never supply an invented floor cable or map scan.
	for (const auto &link : GetUtilityPowerLinks()) {
		if (!link.active || link.kind != UtilityLinkKind::LocalCell) continue; // Only a real surviving paid local installation can select this source family.
		cells.insert(link.source_endpoint);
	}
	REQUIRE_FALSE(cells.empty()); nlohmann::json topology = nlohmann::json::array();
	std::map<std::tuple<uint8_t, uint8_t, uint8_t>, std::pair<uint, TileIndex>> representatives;
	for (const auto tile : cells) {
		const auto row = ObserveGeothermalConduitCell(tile); topology.push_back(row); uint8_t domains = 0;
		for (const auto &domain : row["domains"]) domains |= 1U << domain["variant"].get<uint8_t>();
		const auto key = std::tuple{to_underlying(GetTileLayer(tile)), to_underlying(GetTileType(tile)), domains}; uint clear = 0;
		// A clear-ground representative is chosen from real native geometry,
		// before reading any pixels. Prefer open surroundings so another native
		// building's legitimate roof cannot masquerade as a missing cable.
		for (int y = -1; y <= 1; ++y) {
			for (int x = -1; x <= 1; ++x) {
				const auto nearby = TileAddWrap(tile, x, y);
				if (IsValidTile(nearby) && IsTileType(nearby, TileType::Clear)) ++clear; // Geometry-only rank preserves the paid world and never searches rendered pixels for a passing sample.
			}
		}
		const auto found = representatives.find(key);
		if (found == representatives.end() || clear > found->second.first) representatives[key] = {clear, tile}; // Every actual floor/domain/layer class gets one fixed best-open native witness, selected independently of its screenshot.
	}
	fmt::print("GEO_CONDUIT_TOPOLOGY {}\n", topology.dump());
	DisplayOptions labels = _display_opt;
	for (const auto flag : {DisplayOption::ShowStationNames, DisplayOption::ShowTownNames, DisplayOption::ShowSigns, DisplayOption::ShowWaypointNames}) labels.Reset(flag);
	AutoRestoreBackup display(_display_opt, labels);
	// Real client body visibility reveals floor composition while preserving
	// installed paths, native transport details, machinery and every saved
	// owner. Both complete repeated viewports must remain deterministic.
	for (const auto &[key, representative] : representatives) {
		const auto [open_neighbours, tile] = representative;
		for (const auto zoom : {ZoomLevel::Normal, ZoomLevel::In2x}) {
			for (const std::string_view mode : {"opaque", "hidden"}) {
				TransparencyOptions transparency = _transparency_opt, invisibility = _invisibility_opt;
				for (const auto flag : {TransparencyOption::Industries, TransparencyOption::Buildings}) {
					transparency.Reset(flag); invisibility.Reset(flag);
					if (mode == "hidden") { transparency.Set(flag); invisibility.Set(flag); }
				}
				AutoRestoreBackup transparent(_transparency_opt, transparency); AutoRestoreBackup invisible(_invisibility_opt, invisibility); nlohmann::json images = nlohmann::json::array();
				for (uint repeat = 0; repeat < 2; ++repeat) {
					const auto path = paths.images / fmt::format("conduit-{}-zoom-{}-{}-{}.ppm", tile.base(), to_underlying(zoom), mode, repeat);
					CaptureNativeArtViewport(tile, zoom, path); images.push_back(path.generic_string());
				}
				auto record = ObserveGeothermalConduitCell(tile); record["zoom"] = to_underlying(zoom); record["mode"] = mode; record["paths"] = images; record["open_neighbours"] = open_neighbours;
				fmt::print("GEO_CONDUIT_FRAME {}\n", record.dump());
			}
		}
	}
	CHECK(ComputeUtilityPowerDigest() == digest);
}
#endif /* TESTS_GEOTHERMAL_CONDUIT_VIEW_FIXTURE_H */
