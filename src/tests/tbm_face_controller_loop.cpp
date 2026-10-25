/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file tbm_face_controller_loop.cpp Isolated actual long Standard face loop with a bounded no-progress diagnostic.
 * @see docs/zz-reach2/upgrades/2026-10/m2o23-2-come-space.md Group23B.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs18-geothermal-energy-and-industry.md Group8FA.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "tbm_source_loop_fixture.h"
#include "tbm_face_loop_watch.h"
#include "layer_window_fixture.h"
#include "../map_layer_gui.h"
#include "../viewport_func.h"
#include "../viewport_type.h"
#include "../safeguards.h"

static const NWidgetPart _face_loop_view_widgets[] = {
	NWidget(NWID_VIEWPORT, Colours::Invalid, 0), SetMinimalSize(320, 200), SetResize(1, 1),
}; ///< Actual native viewport widget; no alternate simulation layer is introduced.
static WindowDesc _face_loop_view_desc(
	WindowPosition::Manual, {}, 320, 200, WindowClass::ExtraViewport, WindowClass::None, {}, _face_loop_view_widgets);

/** Native camera owner for visible/hidden equality; raster artwork qualification remains in the later gallery groups. */
class TBMFaceLoopViewport final : public Window {
public:
	/** @param tile Existing fixture tile at which the actual viewport opens. */
	explicit TBMFaceLoopViewport(TileIndex tile) : Window(_face_loop_view_desc)
	{
		this->InitNested(0);
		this->GetWidget<NWidgetViewport>(0)->InitializeViewport(this, tile, ZoomLevel::Viewport);
	}
};

/** One frame's original native ownership, accounting, randomness and full train poses; camera state is excluded. */
struct TBMFaceLoopFrame {
	uint64_t project, exchange, receivers; ///< Canonical state-owner digests, not synthetic work stores.
	uint32_t cursor; ///< Committed original geological cuts.
	uint64_t partial; ///< Original unfinished geological work.
	Money cash; ///< Actual company balance after the native frame.
	uint32_t random0, random1; ///< Simulation RNG must be independent of the selected camera layer.
	std::string cutter, hauler; ///< Actual part IDs, positions, tracks, direction, speed and custody evidence.
	/** @param other Same native frame with another camera policy. @return Exact simulation equality. */
	bool operator==(const TBMFaceLoopFrame &other) const = default;
};

/**
 * Capture original train/packet evidence in fixed chain/list order without mutating it.
 * @param head Actual native front engine.
 * @return Poses, all actual packet identities/metadata and the four cargo action counts for every attached part.
 */
static std::string DescribeFaceLoopTrain(const Train *head)
{
	std::string result = DescribeTrain(head);
	// Inspect the whole actual chain so a hidden tail or a reassigned cargo action cannot escape equality checks.
	for (const Train *part = head; part != nullptr; part = part->Next()) {
		using Action = VehicleCargoList::MoveToAction;
		result += fmt::format("|{}:{}:{}/{}/{}/{}", part->index.base(), part->cargo_type,
			part->cargo.ActionCount(Action::Transfer), part->cargo.ActionCount(Action::Deliver),
			part->cargo.ActionCount(Action::Keep), part->cargo.ActionCount(Action::Load));
		// Compare each original fragment, including its provenance and feeder credit, rather than totals alone.
		for (const CargoPacket *packet : *part->cargo.Packets()) {
			result += fmt::format(";{}:{}:{}:{}:{}:{}:{}:{}:{}", packet->index.base(), packet->Count(),
				packet->GetSourceTile().base(), packet->GetFirstStation().base(), packet->GetNextHop().base(),
				packet->GetPeriodsInTransit(), packet->GetFeederShare().base(), packet->GetCompanySupplyOrder(), packet->HasCompanySupplyShipped());
		}
	}
	return result;
}

/**
 * Task 6.6: a Standard project on a long straight route, driven only by the game's vehicle step, serves every frozen
 * breakpoint at its face: cut, wait, hauler arrives, transfer, disposal, empty return, next breakpoint. The only bay
 * visit is the empty final return. Native camera selection changes no work owner.
 * @param visible True follows the actual cutter layer; false keeps the native viewport on Space.
 * @return Every native frame for exact replay comparison.
 */
static std::vector<TBMFaceLoopFrame> RunFaceLoop(bool visible)
{
	SourceLoop loop(LoopStart::Long);
	LayerWindowTestUI ui(WindowClass::ExtraViewport, _current_company);
	Window *view = new TBMFaceLoopViewport(loop.source);
	std::vector<TBMFaceLoopFrame> frames;
	uint visible_frames = 0;
	TBMFaceLoopWatch watch;
	bool lost_checked = false;
	uint last_progress = 0;
	auto previous_progress = std::pair{loop.Job().cursor, loop.Cycle().exchanges};
	// Each loop step is one game tick; the bound only ends a stalled run.
	while (!loop.Delivered() && loop.ticks < 200000) {
		const MapLayer layer = visible ? GetVehicleDisplayLayer(loop.cutter) : MapLayer::Space;
		SetViewportLayer(view, layer);
		REQUIRE(view->viewport->layer == layer);
		ScrollWindowToTile(MultiLayerMap::TileInLayer(loop.cutter->tile, layer), view, true);
		{
			// This focused target has mock sprite headers, not raster/palette assets. Keep native camera positioning
			// while clipping its screen refresh; art qualification remains the later real-renderer gallery's job.
			AutoRestoreBackup clipped_screen(_screen.width, 0);
			UpdateViewportPosition(view, 0);
		}
		// Count genuine non-stored cutter exposure; a camera pointed at a hidden magazine cannot certify visible play.
		if (visible && !loop.cutter->vehstatus.Test(VehState::Hidden)) ++visible_frames;
		AdvanceSpoilReceiver(*loop.earth);
		AdvanceSpoilReceiver(*loop.rock);
		CallVehicleTicks();
		++loop.ticks;
		REQUIRE_FALSE(loop.cutter->vehstatus.Test(VehState::Crashed));
		REQUIRE_FALSE(loop.hauler->vehstatus.Test(VehState::Crashed));
		watch.Observe(loop.ticks, loop.Project(), loop.Cycle(), *loop.cutter, *loop.hauler);
		frames.push_back({ComputeTBMProjectDigest(), ComputeTBMExchangeDigest(), ComputeSpoilReceiverDigest(),
			loop.Job().cursor, loop.Job().partial_work_ticks, Company::Get(_current_company)->money,
			_random.state[0], _random.state[1], DescribeFaceLoopTrain(loop.cutter), DescribeFaceLoopTrain(loop.hauler)});
		const auto progress = std::pair{loop.Job().cursor, loop.Cycle().exchanges};
		if (progress != previous_progress) { previous_progress = progress; last_progress = loop.ticks; }
		// A healthy cut/haulage interval is far shorter than12,000 native frames.
		// Preserve the exact physical hold early instead of spending a200,000-frame
		// timeout repeating an already stalled cycle without any paid progress.
		if (loop.ticks - last_progress >= 12000) {
			INFO(watch.Describe());
			INFO("cutter " << DescribeTrain(loop.cutter) << "\nhauler " << DescribeTrain(loop.hauler));
			const auto goal = GetTBMHaulerSteeringTarget(*loop.hauler);
			INFO("goal=" << (goal ? goal->tile.base() : UINT32_MAX) << " dirs=" << (goal ? goal->trackdirs.base() : 0));
			FAIL("No paid cut or actual exchange during12,000 native frames");
		}
		// Task 6.5: a hauler held on its stand is at its managed destination, even if a path search had called it lost.
		if (!lost_checked && IsTBMAwaitingFaceTransfer(loop.Project().phase) && IsTBMHaulerHeld(*loop.hauler) && loop.Cycle().phase == TBMHaulerPhase::AwaitExchange) {
			loop.hauler->vehicle_flags.Set(VehicleFlag::PathfinderLost);
			CHECK(GetTBMOrderProblem(*loop.hauler) == std::optional<StringID>{INVALID_STRING_ID});
			loop.hauler->vehicle_flags.Reset(VehicleFlag::PathfinderLost);
			lost_checked = true;
		}
	}
	INFO(watch.Describe());
	REQUIRE(loop.Delivered());
	const auto &breakpoints = loop.Job().pass_terms->exchange_after;
	REQUIRE(breakpoints.size() >= 2);
	CHECK(lost_checked);
	// Every breakpoint was served at the face, the final one included; the cutter never went to the bay before then.
	CHECK(std::vector<uint32_t>(breakpoints.begin(), breakpoints.end()) == watch.face_transfers);
	CHECK(watch.bay_exchanges.empty());
	CHECK(watch.bay_returns.empty());
	CHECK_FALSE(watch.unserved_load);
	CHECK_FALSE(watch.moved_while_waiting);
	CHECK_FALSE(watch.kept_heading_on_return);
	CHECK_FALSE(watch.moved_while_obstructed);
	CHECK_FALSE(watch.order_problem);
	CHECK(loop.Project().phase == TBMPhase::Completed);
	CHECK(SourceLoop::Aboard(loop.cutter) == SpoilTonnes{});
	CHECK(SourceLoop::Aboard(loop.hauler) == SpoilTonnes{});
	CHECK(loop.Received() == loop.Committed());
	CHECK(loop.hauler->GetNumOrders() == 1); // The cycle never wrote the player's orders.
	if (visible) CHECK(visible_frames > 0);
	return frames;
}

/** Every native frame of complete loaded/empty face logistics is identical with its real camera visible or hidden. */
TEST_CASE("A Standard face loop delivers and returns identically under visible and hidden cameras", "[tbm-face-driven]")
{
	const auto visible = RunFaceLoop(true);
	const auto hidden = RunFaceLoop(false);
	REQUIRE_FALSE(visible.empty());
	REQUIRE(visible.size() == hidden.size());
	// Compare at the same physical tick so a delayed transfer or temporary cargo/cash/RNG difference cannot be
	// hidden by matching final totals. Stop at the first differing frame to preserve useful bounded diagnostics.
	for (size_t frame = 0; frame < visible.size(); ++frame) {
		INFO("native frame " << frame);
		REQUIRE(visible[frame] == hidden[frame]);
	}
}
