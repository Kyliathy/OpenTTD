/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file selected_control_view_fixture.h Actual paid-world native icon/caption layout and component-state observations.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group84BD.2.
 * @see docs/zz-reach2/architecture/gui/archi-gui.md.
 * Client inspection creates no utility, machinery, stock, world command or calendar change.
 */
#ifndef TESTS_SELECTED_CONTROL_VIEW_FIXTURE_H
#define TESTS_SELECTED_CONTROL_VIEW_FIXTURE_H
#include "native_art_render_fixture.h"
#include "../3rdparty/nlohmann/json.hpp"
#include "../underground_gui.h"
#include "../multilayer_sprite.h"
#include "../company_func.h"
#include "../strings_func.h"
#include "../core/geometry_func.hpp"
#include "../widgets/underground_widget.h"

/** @param window Actual paid-world toolbar after native repaint. @param layer Explicit requested rock plane. @param mode Real component/player display state. @param paths Current ordinary observer's routed review destinations. Capture two complete widget frames and their original source projection without altering simulation owners. */
static void CaptureSelectedControlFrame(Window &window, MapLayer layer, std::string_view mode, const TestArtifactPaths &paths)
{
	nlohmann::json images = nlohmann::json::array();
	for (uint repeat = 0; repeat < 2; ++repeat) {
		const auto path = paths.images / fmt::format("selected-controls-{}-{}-{}.ppm", to_underlying(layer), mode, repeat);
		CaptureNativeArtWindow(window, path); images.push_back(path.generic_string());
	}
	nlohmann::json widgets = nlohmann::json::array();
	// The original real widget geometry locates each source pose. Native
	// transparent padding and RTL layout are retained; there is no pixel
	// search, painted caption, synthetic window or assumed icon coordinate.
	for (const WidgetID id : {WID_UC_TUBES, WID_UC_TUBE_GUIDEWAY, WID_UC_GEOTHERMAL, WID_UC_GEO_PLAN}) {
		const bool action = id == WID_UC_TUBE_GUIDEWAY || id == WID_UC_GEO_PLAN;
		if (mode == "closed" && action) continue; // A hidden child has no visible caption/icon rectangle; only the two real menu controls belong to the collapsed frame.
		const auto *widget = window.GetWidget<NWidgetCore>(id); const auto rect = widget->GetCurrentRect(); const auto icon = GetUndergroundConstructionWidgetSprite(window, id); REQUIRE(icon.has_value());
		const auto inner = rect.Shrink(WidgetDimensions::scaled.framerect); Point offset; Dimension size = GetSpriteSize(*icon, &offset); size.width -= offset.x; size.height -= offset.y;
		const bool rtl = _current_text_dir == TD_RTL; const int image_width = std::max<int>(GetSquareScaledSpriteSize(*icon).width, inner.Height());
		const auto image_rect = inner.WithWidth(image_width, rtl), text_rect = inner.Indent(image_width + WidgetDimensions::scaled.hsep_wide, rtl);
		const Point point{CentreBounds(image_rect.left, image_rect.right, size.width), CentreBounds(image_rect.top, image_rect.bottom, size.height)};
		const auto caption = window.GetWidgetString(id, widget->GetString()); const auto text_size = GetStringBoundingBox(caption, widget->GetFontSize());
		CHECK(text_size.width <= static_cast<uint>(text_rect.Width())); CHECK(text_size.height <= static_cast<uint>(text_rect.Height()));
		CHECK(image_rect.left >= rect.left); CHECK(image_rect.right <= rect.right); CHECK(window.width <= 640); CHECK(window.height <= 720);
		widgets.push_back({{"widget", id}, {"asset_id", id == WID_UC_TUBES || id == WID_UC_TUBE_GUIDEWAY ? "control.tube" : "control.geothermal"},
			{"visible", mode != "closed" || !action}, {"disabled", window.IsWidgetDisabled(id)}, {"pressed", window.IsWidgetLowered(id)}, {"sprite", *icon}, {"caption", caption},
			{"source_origin", {point.x - offset.x, point.y - offset.y}}, {"caption_size", {text_size.width, text_size.height}}, {"caption_rect", {text_rect.left, text_rect.top, text_rect.right, text_rect.bottom}},
			{"widget_rect", {rect.left, rect.top, rect.right, rect.bottom}}});
	}
	const nlohmann::json record{{"layer", to_underlying(layer)}, {"mode", mode}, {"paths", images}, {"width", window.width}, {"height", window.height}, {"widgets", widgets}};
	fmt::print("SELECTED_CONTROL_FRAME {}\n", record.dump());
}

/** @param paths Current ordinary observer's routed review destinations. Exercise actual closed/expanded/spectator native controls on both rock planes, restoring player/window state without commands or simulation time. */
static void CaptureSelectedControlViews(const TestArtifactPaths &paths)
{
	// Each real view borrows the original saved world. Menu expansion only
	// changes client layout; a spectator changes native action availability
	// without changing any original company, cash, owner, date or resource.
	for (const auto layer : {MapLayer::Underground, MapLayer::DeepUnderground}) {
		Window *window = ShowUndergroundConstructionToolbar(layer, GetCurrentMultiLayerProfileSelection()); REQUIRE(window != nullptr);
		CaptureSelectedControlFrame(*window, layer, "closed", paths);
		window->OnClick({}, WID_UC_TUBES, 1); window->OnClick({}, WID_UC_GEOTHERMAL, 1);
		CaptureSelectedControlFrame(*window, layer, "expanded", paths);
		{
			AutoRestoreBackup spectator{_local_company, COMPANY_SPECTATOR}; CaptureSelectedControlFrame(*window, layer, "spectator", paths);
			CHECK(window->IsWidgetDisabled(WID_UC_TUBE_GUIDEWAY)); CHECK(window->IsWidgetDisabled(WID_UC_GEO_PLAN));
		}
		window->Close(); Window::DeleteClosedWindows();
	}
}
#endif /* TESTS_SELECTED_CONTROL_VIEW_FIXTURE_H */
