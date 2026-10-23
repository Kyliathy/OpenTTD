/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file native_art_loop_fixture.h Read-only raster observations of the actual loaded TBM/spoil loop.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs13-graphics-and-asset-pipeline.md Groups82/83/85B
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs3mp-master-plan.md checkpoints8.7/8.8
 */

#ifndef TESTS_NATIVE_ART_LOOP_FIXTURE_H
#define TESTS_NATIVE_ART_LOOP_FIXTURE_H

#include "test_artifact_paths.h"
#include "native_art_render_fixture.h"
#include "../multilayer_sprite.h"
#include "../excavation_state.h"
#include "../layer_portal_cmd.h"
#include "../spritecache.h"
#include "../map_func.h"
#include "../blitter/factory.hpp"
#include "../fontcache.h"
#include "../core/random_func.hpp"
#include "../company_base.h"
#include "../spoil_receiver.h"
#include "../timer/timer_game_tick.h"
#include "../vehicle_func.h"
#include "../vehicle_gui.h"
#include "../rail_map.h"
#include "../transparency.h"
#include <cstdlib>
#include <set>

/** An optional isolated observer records physical phases; the ordinary game loop retains every transition and packet owner. */
class NativeArtLoopProbe {
	std::filesystem::path directory; ///< Fixed repository review directory; never a user-wide screenshot path.
	std::string prefix; ///< Actual scenario identity inherited from the runner's already validated fixture output.
	std::set<std::string> captured; ///< Transient first-occurrence scene keys; never saved or consulted by gameplay.
	uint frames = 0; ///< Actual raster evidence count.
	uint32_t last_exchanges = 0; ///< Previously observed owner's exchange count, never a replacement cycle counter.
	bool source_bay = false; ///< Actual initial bay kind determines whether retreat crosses the commissioning pair or ends at a rock-layer depot.
	bool gallery = false; ///< Optional final M1 presentation/picking coverage; no simulation override.

	/**
	 * Capture the actual selected scene once at both native and 2x scales.
	 * @param stage Physical event name supplied by read-only native queries.
	 * @param centres Actual explicit world tiles, including independent portal endpoints where needed.
	 * @param project Existing actual project supplying identity/phase/company.
	 * @param assignment Existing actual hauler assignment supplying custody/phase.
	 * @param ticks Actual observed game frame count; it never advances a simulation owner.
	 */
	void Capture(std::string stage, std::span<const TileIndex> centres, const TBMProject &project, const TBMExchangeAssignment &assignment, uint ticks)
	{
		if (!this->captured.insert(stage).second) return;
		const auto geology = ComputeGeologyStateDigest(GetGeologyWorldState());
		const auto projects = CaptureTBMProjects();
		const auto assignments = CaptureTBMExchangeAssignments();
		const auto receivers = CaptureSpoilReceivers();
		const Money money = Company::Get(project.company)->money;
		SavedRandomSeeds before;
		SaveRandomSeeds(&before);
		std::vector<std::tuple<VehicleID, TileIndex, uint, CargoType, Direction>> packets;
		// Snapshot every actual physical vehicle's custody/position before drawing;
		// neither switching renderers nor a second viewport may move it or its cargo.
		for (const Vehicle *vehicle : Vehicle::Iterate()) packets.emplace_back(vehicle->index, vehicle->tile, vehicle->cargo.TotalCount(), vehicle->cargo_type, vehicle->direction);
		REQUIRE(BlitterFactory::SelectBlitter("8bpp-simple") != nullptr);
		GfxClearSpriteCache();
		FontCache::ClearFontCaches(FONTSIZES_REQUIRED);
		// Each centre belongs to a true world layer. Both real zoom levels use
		// the existing tile/vehicle sorter and preserve the authored mouth masks.
		for (size_t scene = 0; scene < centres.size(); ++scene) {
			for (const ZoomLevel zoom : {ZoomLevel::Normal, ZoomLevel::In2x}) {
				const std::string name = fmt::format("{}-{}-scene-{}-zoom-{}.ppm", this->prefix, stage, scene, to_underlying(zoom));
				CaptureNativeArtViewport(centres[scene], zoom, this->directory / name);
				++this->frames;
			}
		}
		// The same physical scene must retain occlusion and explicit layers when
		// existing building/industry transparency changes. Restore only UI options;
		// never hide a vehicle or remove its mouth/depot to manufacture visibility.
		if (this->gallery && (stage == "spanning-entry" || stage == "actual-disposal")) {
			TransparencyOptions options = _transparency_opt;
			options.Set(TransparencyOption::Buildings);
			options.Set(TransparencyOption::Industries);
			AutoRestoreBackup transparent(_transparency_opt, options);
			AutoRestoreBackup invisible(_invisibility_opt, TransparencyOptions{});
			for (size_t scene = 0; scene < centres.size(); ++scene) {
				for (const ZoomLevel zoom : {ZoomLevel::Normal, ZoomLevel::In2x}) {
					CaptureNativeArtViewport(centres[scene], zoom, this->directory / fmt::format("{}-{}-transparent-scene-{}-zoom-{}.ppm", this->prefix, stage, scene, to_underlying(zoom)));
					++this->frames;
				}
			}
		}
		// Direction queries use genuine current cargo/phase and actual native
		// GetImage calls. They are renderer previews, not claims of eight physical
		// headings or eight simulated movements in this fixed saved scenario.
		if (this->gallery && (stage == "initial-held" || stage == "actual-idle" || stage == "boring-phase-0" || stage == "loaded-cutter" || stage == "actual-service")) {
			const Train *cutter = Train::Get(project.cutter);
			std::vector<const Train *> units{cutter, cutter->Next()};
			std::set<TBMConsistRole> roles;
			// Show one real complete wagon per cargo, without duplicating every
			// repeated unit; all heads retain their current physical packet stock.
			for (const auto &unit : InspectTBMConsist(*cutter, project).exchangeable_units) {
				if (roles.insert(unit.role).second) units.push_back(Train::Get(unit.vehicle));
			}
			units.push_back(Train::Get(assignment.hauler));
			REQUIRE(units.size() <= 5);
			/** Paint genuine current-state native direction queries without changing body facing. */
			CaptureNativeArtCanvas(640, 360, this->directory / fmt::format("{}-{}-directions-scene-0-zoom-0.ppm", this->prefix, stage), [&] {
				// One row belongs to each distinct physical cutter/support/cargo/
				// hauler participant, retaining its real current loaded state.
				for (size_t row = 0; row < units.size(); ++row) {
					const Train *unit = units[row];
					REQUIRE(unit != nullptr);
					DrawString(0, 639, static_cast<int>(row * 68), fmt::format("Vehicle {} / actual cargo {} / directions N, NE, E, SE, S, SW, W, NW", unit->index.base(), unit->cargo.StoredCount()), TextColour::White);
					// Each cell changes only the renderer's explicit direction input;
					// the real unit, project phase and clock are shared by all eight.
					for (uint8_t direction = 0; direction < 8; ++direction) {
						VehicleSpriteSeq image;
						unit->GetImage(static_cast<Direction>(direction), EngineImageType::OnMap, &image);
						REQUIRE(image.count != 0);
						image.Draw(40 + direction * 80, static_cast<int>(row * 68 + 42), PAL_NONE, false);
					}
				}
			});
			++this->frames;
		}
		// A genuinely exposed boring body must be selectable on its rendered
		// layer and rejected on another layer at the same projected location.
		if (this->gallery && stage == "boring-phase-0") {
			const Train *cutter = Train::Get(project.cutter);
			Viewport view{};
			view.width = 640;
			view.height = 360;
			view.zoom = ZoomLevel::Normal;
			view.layer = GetVehicleDisplayLayer(cutter);
			view.virtual_left = (cutter->coord.left + cutter->coord.right) / 2 - ScaleByZoom(320, view.zoom);
			view.virtual_top = (cutter->coord.top + cutter->coord.bottom) / 2 - ScaleByZoom(180, view.zoom);
			const Vehicle *picked = CheckClickOnVehicle(view, 320, 180);
			REQUIRE(picked != nullptr);
			CHECK(picked->First() == cutter);
			view.layer = MapLayer::Surface;
			const Vehicle *other = CheckClickOnVehicle(view, 320, 180);
			CHECK((other == nullptr || other->First() != cutter));
			fmt::print("M1_GALLERY_PICK actual_body=true correct_layer=true wrong_layer_refused=true\n");
		}
		REQUIRE(BlitterFactory::SelectBlitter("null") != nullptr);
		GfxClearSpriteCache();
		FontCache::ClearFontCaches(FONTSIZES_REQUIRED);
		SavedRandomSeeds after;
		SaveRandomSeeds(&after);
		CHECK(std::ranges::equal(before.random.state, after.random.state));
		CHECK(std::ranges::equal(before.interactive_random.state, after.interactive_random.state));
		CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == geology);
		CHECK(CaptureTBMProjects() == projects);
		CHECK(CaptureTBMExchangeAssignments() == assignments);
		CHECK(CaptureSpoilReceivers() == receivers);
		CHECK(Company::Get(project.company)->money == money);
		size_t index = 0;
		for (const Vehicle *vehicle : Vehicle::Iterate()) {
			CHECK(packets[index++] == std::tuple(vehicle->index, vehicle->tile, vehicle->cargo.TotalCount(), vehicle->cargo_type, vehicle->direction));
		}
		const Train *cutter = Train::Get(project.cutter);
		// Retain actual projected positions and hidden/cargo/source identities so
		// inspection can distinguish legitimate mouth occlusion from missing art.
		for (const Train *part = cutter; part != nullptr; part = part->Next()) {
			fmt::print("M1_LOOP_PART stage={} id={} tile={} x={} y={} coord={},{},{},{} hidden={} cargo={} native_sprite={}\n",
				stage, part->index.base(), part->tile.base(), part->x_pos, part->y_pos,
				part->coord.left, part->coord.top, part->coord.right, part->coord.bottom, part->vehstatus.Test(VehState::Hidden),
				part->cargo.StoredCount(), GetMultiLayerTrainSprite(*part, part->direction, TimerGameTick::counter).value_or(0));
		}
		fmt::print("M1_LOOP_ART stage={} ticks={} frames={} phase={} hauler_phase={} exchanges={} cutter={} tile={} direction={} backwards={}\n",
			stage, ticks, this->frames, to_underlying(project.phase), to_underlying(assignment.phase), assignment.exchanges,
			project.cutter.base(), cutter->tile.base(), to_underlying(cutter->direction), cutter->IsDrivingBackwards());
	}

public:
	/**
	 * Prepare optional scene observation without altering the loaded game.
	 * @param output Already validated repository fixture output prefix; its basename names the review scenes.
	 * @param assignment Actual initial hauler assignment; subsequent exchange facts remain with that owner.
	 */
	NativeArtLoopProbe(const std::filesystem::path &output, const TBMExchangeAssignment &assignment) :
		directory(GetTestCaseArtifactPaths("tbm", "spoil-loop-underground").images),
		prefix(output.filename().string()), last_exchanges(assignment.exchanges)
	{
		std::filesystem::create_directories(this->directory);
		const auto *project = GetTBMProject(assignment.project);
		REQUIRE(project != nullptr);
		const auto bay = ResolveTBMExchangeBay(*project, project->exchange_bay);
		REQUIRE(bay.error == TBMExchangeError::None);
		this->source_bay = bay.kind == TBMExchangeBayKind::Source;
		this->gallery = std::getenv("OTTD_M1_GALLERY_ART") != nullptr;
		// The retained paused source supplies its genuine held pose before the
		// driver starts simulation. A paused game need not have its cutter's
		// individual Stopped flag set, so do not invent or demand that flag.
		if (this->gallery) {
			const std::array centre{Train::Get(project->cutter)->tile};
			this->Capture("initial-held", centre, *project, assignment, 0);
		}
	}

	/**
	 * Observe the completed ordinary tick and capture genuine first-occurrence physical phases.
	 * @param assignment Actual loaded public hauler assignment.
	 * @param ticks Actual native observer frame count, independent of render animation.
	 */
	void Observe(const TBMExchangeAssignment &assignment, uint ticks)
	{
		const auto *project = GetTBMProject(assignment.project);
		REQUIRE(project != nullptr);
		const Train *cutter = Train::Get(project->cutter);
		const Train *hauler = Train::Get(assignment.hauler);
		const auto key = GetLayerPortalPairKey(project->portal);
		REQUIRE(key.has_value());
		const TileIndex lower = GetLayerPortalPairTile(*key);
		const std::array portal_views{*key, lower};
		// A genuine zero-speed idle selector may occur during real waiting or
		// travel positioning. Observe that pose instead of toggling Stop for art.
		const auto asset = project->model_id == 0 ? MultiLayerSpriteAsset::VehicleModel00 : MultiLayerSpriteAsset::VehicleModel01;
		if (this->gallery && GetMultiLayerTrainSprite(*cutter, cutter->direction, TimerGameTick::counter) ==
			GetMultiLayerSprite(asset, {0, 0, static_cast<uint8_t>(cutter->direction), 0, 0})) {
			const std::array centre{cutter->tile};
			this->Capture("actual-idle", centre, *project, assignment, ticks);
		}
		// Service is observed only at an actual railway depot. A portal's depot
		// track state remains its magazine, so it cannot impersonate servicing.
		if (this->gallery && cutter->IsInDepot() && IsRailDepotTile(cutter->tile)) {
			const std::array centre{cutter->tile};
			this->Capture("actual-service", centre, *project, assignment, ticks);
		}
		bool upper_part = false, lower_part = false, lower_wagon = false;
		uint cutter_load = 0, hauler_load = 0;
		// Only this exact assigned consist can establish pocket entry/spanning.
		// Every detached wagon keeps its real physical identity and packet count.
		for (const Train *part = cutter; part != nullptr; part = part->Next()) {
			upper_part |= part->tile == *key;
			lower_part |= part->tile == lower;
			// A real detachable hopper in the destination housing proves more
			// than a cutter head crossing alone; articulation cannot stand in for it.
			lower_wagon |= part->tile == lower && part != cutter && !part->IsArticulatedPart();
			cutter_load += part->cargo.StoredCount();
		}
		for (const Train *part = hauler; part != nullptr; part = part->Next()) hauler_load += part->cargo.StoredCount();
		const bool backwards = cutter->IsDrivingBackwards();
		// Joint occupancy proves each real entry/retreat crossing, independently
		// of whether the head or trailing hoppers are occluded by the mouth roof.
		if (upper_part && lower_part) this->Capture(backwards ? "spanning-retreat" : "spanning-entry", portal_views, *project, assignment, ticks);
		if (lower_part && lower_wagon) this->Capture(backwards ? "pocket-retreat" : "pocket-entry", portal_views, *project, assignment, ticks);
		if (upper_part && backwards) this->Capture("upper-retreat", portal_views, *project, assignment, ticks);
		// A visible unblocked work head supplies actual drilling animation phases;
		// a hidden commissioning magazine or stopped controller cannot impersonate it.
		// The first visible flag can still coincide with the mouth roof's physical
		// overlap. Show drilling beyond that first cell, preserving genuine motion
		// and rendering rather than moving the train or removing the building.
		if (project->phase == TBMPhase::Bore && project->blocked == TBMBlockedReason::None && !cutter->vehstatus.Test(VehState::Hidden) &&
			GetTileLayer(cutter->tile) == GetTileLayer(lower) && DistanceManhattan(cutter->tile, lower) > 1) {
			const auto frame = GetMultiLayerVisualFrame(project->model_id == 0 ? MultiLayerSpriteAsset::VehicleModel00 : MultiLayerSpriteAsset::VehicleModel01, TimerGameTick::counter, false);
			const std::array centre{cutter->tile};
			this->Capture(fmt::format("boring-phase-{}", frame), centre, *project, assignment, ticks);
		}
		if (cutter_load != 0) {
			const std::array centre{cutter->tile};
			this->Capture("loaded-cutter", centre, *project, assignment, ticks);
		}
		if (assignment.exchanges > this->last_exchanges) {
			const std::array centres{cutter->tile, hauler->tile};
			this->Capture("actual-exchange", centres, *project, assignment, ticks);
			this->last_exchanges = assignment.exchanges;
		}
		// Surface transport/disposal requires the actual loaded hauler there;
		// co-located underground wagons or transfer-only stock cannot prove it.
		if (GetTileLayer(hauler->tile) == MapLayer::Surface && hauler_load != 0) {
			const std::array centre{hauler->tile};
			this->Capture("surface-loaded-haul", centre, *project, assignment, ticks);
		}
		if (assignment.phase == TBMHaulerPhase::DisposeOrTransfer) {
			const std::array centre{hauler->tile};
			this->Capture("actual-disposal", centre, *project, assignment, ticks);
		}
		// Initial empty positioning also uses ReturnEmpties. Record the loop's
		// return only after an actual loaded exchange and Surface disposal, so
		// a startup empty locomotive cannot substitute for returned real wagons.
		if (assignment.phase == TBMHaulerPhase::ReturnEmpties && hauler_load == 0 && assignment.exchanges != 0 && this->captured.contains("actual-disposal")) {
			const std::array centre{hauler->tile};
			this->Capture("empty-return", centre, *project, assignment, ticks);
		}
	}

	/** Report the actual captured phase set at the ordinary completed-loop boundary; no missing event is waived. */
	void Finish() const
	{
		// Both genuine bay kinds must show the real dig/load/couple/dispose/return
		// loop and all authored drilling phases. A depot loop physically retreats
		// to its underground bay rather than the upper commissioning portal.
		for (const auto stage : {"spanning-entry", "pocket-entry", "boring-phase-0", "boring-phase-1", "boring-phase-2", "boring-phase-3",
			"loaded-cutter", "actual-exchange", "surface-loaded-haul", "actual-disposal", "empty-return"}) {
			CHECK(this->captured.contains(stage));
		}
		if (this->source_bay) {
			CHECK(this->captured.contains("spanning-retreat"));
			CHECK(this->captured.contains("upper-retreat"));
		}
		if (this->gallery) {
			CHECK(this->captured.contains("initial-held"));
			if (!this->source_bay) CHECK(this->captured.contains("actual-service"));
			fmt::print("M1_GALLERY_COMPLETE actual_directions=8 transparency=true body_picking=true\n");
		}
		fmt::print("M1_LOOP_ART_COMPLETE scenes={} frames={} simulation_owners_unchanged_during_render=true\n", this->captured.size(), this->frames);
	}
};

#endif /* TESTS_NATIVE_ART_LOOP_FIXTURE_H */
