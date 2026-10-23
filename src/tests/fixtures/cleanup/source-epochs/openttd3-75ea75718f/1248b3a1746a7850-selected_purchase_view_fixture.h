/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file selected_purchase_view_fixture.h Actual selected capsule purchase renderer on an unchanged original paid world.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Groups84CB/84C.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
 * Preview canvases exercise DrawTrainEngine/GetRailIcon; they do not represent a purchase transaction or a complete purchase window.
 */
#ifndef TESTS_SELECTED_PURCHASE_VIEW_FIXTURE_H
#define TESTS_SELECTED_PURCHASE_VIEW_FIXTURE_H

#include "native_art_render_fixture.h"
#include "../3rdparty/nlohmann/json.hpp"
#include "../company_base.h"
#include "../company_func.h"
#include "../engine_base.h"
#include "../engine_func.h"
#include "../engine_gui.h"
#include "../multilayer_sprite.h"
#include "../transport_tube.h"
#include "../train.h"

/**
 * Draw all five actual native engine purchase previews with real viewing-company availability.
 * @param paths Current ordinary observer's routed review/image destinations.
 * @return None; restore the viewing actor and leave every saved owner, engine flag, date, clock and RNG unchanged.
 */
static void CaptureSelectedPurchaseViews(const TestArtifactPaths &paths)
{
	const CompanyID original = _local_company;
	REQUIRE(Company::IsValidID(original));
	// A spectator supplies genuine unavailable preview policy without changing
	// introduction dates or saved company engine availability. The original
	// actor retains whatever unlocked/locked states its actual paid world owns.
	for (const bool spectator : {false, true}) {
		AutoRestoreBackup actor{_local_company, spectator ? COMPANY_SPECTATOR : original};
		const std::string mode = spectator ? "spectator" : "owner";
		nlohmann::json records = nlohmann::json::array(), images = nlohmann::json::array();
		// Both complete canvases use the ordinary train purchase renderer; the
		// verifier compares every RGB byte and every original nontransparent
		// purchase pixel, rather than substituting a directly pasted source image.
		for (uint repeat = 0; repeat < 2; ++repeat) {
			const auto path = paths.images / fmt::format("selected-purchase-{}-{}.ppm", mode, repeat);
			/** Draw the five actual engine identities through their unchanged native purchase call chain. */
			CaptureNativeArtCanvas(640, 80, path, [&] {
				// Each model keeps its persisted engine mapping and actual native
				// availability. Rendering cannot grant purchase permission to any
				// later model or forge a supported engine for an unrelated family.
				for (uint16_t model = 9; model <= 13; ++model) {
					const EngineID engine = GetTubeEngine(model); REQUIRE(Engine::IsValidID(engine));
					// Company validity gates the same buildability query used by
					// GetRailIcon; a spectator must show locked art without querying
					// a nonexistent company or changing saved availability flags.
					const bool available = Company::IsValidID(_local_company) && IsEngineBuildable(engine, VehicleType::Train, _local_company);
					const auto sprite = GetMultiLayerEnginePurchaseSprite(engine, available); REQUIRE(sprite.has_value());
					const int left = (model - 9) * 128, x = left + 64, y = 40;
					Point offset; GetSpriteSize(*sprite, &offset);
					DrawTrainEngine(left, left + 127, x, y, engine, PAL_NONE, EngineImageType::Purchase);
					if (repeat == 0) records.push_back({{"asset_id", fmt::format("purchase.model-{:02}", model)}, {"model", model}, {"engine", engine.base()},
						{"available", available}, {"state", available ? 0 : 1}, {"sprite", *sprite}, {"draw_anchor", {x, y}}, {"native_trimmed_origin", {x + offset.x, y + offset.y}}});
				}
			});
			images.push_back(path.generic_string());
		}
		fmt::print("SELECTED_PURCHASE_FRAME {}\n", nlohmann::json{{"mode", mode}, {"actor", _local_company.base()}, {"paths", images}, {"models", records}}.dump());
	}
	CHECK(_local_company == original);
}

#endif /* TESTS_SELECTED_PURCHASE_VIEW_FIXTURE_H */
