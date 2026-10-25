/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */
/**
 * @file utility_scheduler_visibility.cpp Exact common native interval replay with actual Deep or Space camera selection.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs18-geothermal-energy-and-industry.md.
 * The mixed fixture declares construction boundary inputs. Native camera refresh
 * uses clipped drawing and mock sprite headers; this is not raster acceptance.
 */
#include "../stdafx.h"
#include <filesystem> // Standard file APIs used by generated-language setup precede fixture safeguards.
#include "utility_scheduler_mixed_fixture.h"
#include "layer_window_fixture.h"
#include "../map_layer_gui.h"
#include "../viewport_func.h"
#include "../viewport_type.h"
#include "../core/random_func.hpp"
#include "../safeguards.h"

static const NWidgetPart _scheduler_view_widgets[] = {
	NWidget(NWID_VIEWPORT, Colours::Invalid, 0), SetMinimalSize(320, 200), SetResize(1, 1),
}; ///< Actual native viewport widget, independent of simulation owners.
static WindowDesc _scheduler_view_desc(
	WindowPosition::Manual, {}, 320, 200, WindowClass::ExtraViewport, WindowClass::None, {}, _scheduler_view_widgets);

/** Native camera whose real layer selection exercises ordinary view state without supplying energy or work. */
class SchedulerReplayViewport final : public Window {
public:
	/** @param tile Actual native capsule's initial depot tile. */
	explicit SchedulerReplayViewport(TileIndex tile) : Window(_scheduler_view_desc)
	{
		this->InitNested(0);
		this->GetWidget<NWidgetViewport>(0)->InitializeViewport(this, tile, ZoomLevel::Viewport);
	}
};

/** Exact simulation/accounting/physical observations; camera position is deliberately not synchronized world authority. */
struct SchedulerReplayFrame {
	Money cash; ///< Actual native company balance after real service and work.
	std::array<uint64_t, 17> fields; ///< Fixed authoritative digests, RNG, native pose and original capsule identity/load.
	/** @param other Corresponding independently executed interval. @return Full original-owner and native physical equality. */
	bool operator==(const SchedulerReplayFrame &other) const = default;
};

/** @param visible Follow the actual moving capsule on Deep; false observes Space instead. @return All128original common/native interval frames. */
static std::vector<SchedulerReplayFrame> ReplaySchedulerWithCamera(bool visible)
{
	UtilitySchedulerMixedRig rig{GeothermalGeneratorModel::VentTap}; auto *capsule = rig.Buy(), *manual = rig.Buy(true);
	LayerWindowTestUI ui(WindowClass::ExtraViewport, _current_company); Window *view = new SchedulerReplayViewport(capsule->tile);
	std::vector<SchedulerReplayFrame> frames; uint exposed = 0;
	// Every iteration offers all actual consumers one normal native tick. The
	// camera observes either the moving world or another plane; it cannot select
	// source publication, money, work, cargo, movement or random-number ownership.
	for (uint tick = 0; tick < 128; ++tick) {
		const auto layer = visible ? MapLayer::DeepUnderground : MapLayer::Space;
		SetViewportLayer(view, layer); REQUIRE(view->viewport->layer == layer);
		ScrollWindowToTile(MultiLayerMap::TileInLayer(capsule->tile, layer), view, true);
		{
			AutoRestoreBackup clipped_screen(_screen.width, 0); // Exercise actual camera positioning without unavailable unit raster assets.
			UpdateViewportPosition(view, 0);
		}
		rig.Opportunity(std::array{capsule, manual});
		if (!capsule->vehstatus.Test(VehState::Hidden)) ++exposed;
		CHECK(manual->vehstatus.Test(VehState::Stopped)); CHECK_FALSE(capsule->vehstatus.Test(VehState::Crashed));
		frames.push_back({Company::Get(_current_company)->money,
			{ComputeUtilityPowerDigest(), ComputeGeothermalGeneratorDigest(), ComputeGeothermalCollectorDigest(),
			ComputeTubeGeothermalPowerDigest(), ComputeTubePowerDigest(), ComputeTubePowerVehicleDigest(), ComputeSpaceStructureDigest(),
			ComputeGeologyStateDigest(GetGeologyWorldState()), _random.state[0], _random.state[1], capsule->index.base(), capsule->tile.base(),
			static_cast<uint64_t>(capsule->x_pos), static_cast<uint64_t>(capsule->y_pos), capsule->cur_speed,
			capsule->cargo.TotalCount(), manual->reliability}});
	}
	CHECK(exposed > 0); CHECK(GetGeothermalGeneratorState(rig.generator)->active_service_ticks >= 128);
	CHECK(rig.processor->module->processing->paid_lifetime_j > 0); CHECK(rig.assembly->module->manufacture->work_ticks > 0);
	return frames;
}

/** Real native capsule/town/factory/recipe intervals have identical state, physical work and random/cash history with their layer visible or hidden. */
TEST_CASE("Common native utility intervals are independent of actual camera layer", "[utility-scheduler-visible-hidden]")
{
	MockEnvironment::Instance(); LoadGeneratedWindowTestLanguage();
	const auto visible = ReplaySchedulerWithCamera(true); const auto hidden = ReplaySchedulerWithCamera(false);
	REQUIRE(visible.size() == hidden.size());
	// Compare each original opportunity, not merely a final total that could
	// conceal a different grant order, skipped consumer or duplicated work.
	for (size_t index = 0; index < visible.size(); ++index) {
		INFO("native interval " << index); CHECK(visible[index].cash == hidden[index].cash);
		// Report the first differing authoritative owner rather than hiding a
		// replay divergence behind an unprintable aggregate comparison.
		for (size_t field = 0; field < visible[index].fields.size(); ++field) {
			INFO("authoritative field " << field); REQUIRE(visible[index].fields[field] == hidden[index].fields[field]);
		}
	}
}
