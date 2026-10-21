/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/**
 * @file launch_facility_sprite.cpp Original per-cell launch hardware and zone-scoped foreground drawing.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group62JB
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs9-launch-centers-and-rockets.md task16.2
 * The accepted foreground model retains original roofs/near arms and gray
 * cavity backgrounds. Sorting changes no pixel, world custody or work clock.
 */
#include "stdafx.h"
#include "launch_facility_sprite.h"
#include "launch_operations.h"
#include "space_vehicle.h"
#include "space_transition.h"
#include "space_manifest.h"
#include "space_pod.h"
#include "space_replacement.h"
#include "station_base.h"
#include "station_map.h"
#include "landscape.h"
#include "viewport_func.h"
#include "openttd.h"
#include <algorithm>
#include "safeguards.h"

/** Resolve only a cell of the actual native footprint and its disjoint visitor zone.
 * @param center Existing native station with an optional launch facility.
 * @param tile Requested physical Surface cell, without a copied geometry registry.
 * @return Original pad/workshop/recovery/storage zone, or absence outside the facility.
 */
static std::optional<LaunchFacilityZone> FacilityZone(const Station &center, TileIndex tile)
{
	// The source belongs to the immutable Surface footprint; joined rail/road
	// tiles and numerically overlapping cells on other layers cannot borrow it.
	if (!center.launch_center || !IsValidTile(tile) || GetTileLayer(tile) != MapLayer::Surface) return std::nullopt;
	const auto &anchor = *center.launch_center;
	const auto footprint = GetLaunchCenterFootprint(anchor.tile, anchor.rotation);
	if (!footprint || !footprint->Contains(tile)) return std::nullopt;
	// Four native disjoint zones are the sole physical ownership map. This
	// bounded query neither scans actors nor creates an art-local facility map.
	for (auto zone : {LaunchFacilityZone::Pad, LaunchFacilityZone::Workshop, LaunchFacilityZone::Recovery, LaunchFacilityZone::Storage}) {
		const auto area = GetLaunchFacilityZone(anchor.tile, anchor.rotation, zone);
		// A native area must exist and own this exact cell before its visitor
		// scope can authorize foreground sorting or workshop state cues.
		if (area && area->Contains(tile)) return zone;
	}
	return std::nullopt;
}

/** Read the original exclusive workshop's real job without earning an opportunity.
 * @param center Native station owning the exact generation-qualified operational slot.
 * @return Ready1, admitted work2 or retained job refusal/pause3; other cells stay ready.
 */
static uint8_t WorkshopState(const Station &center)
{
	if (!center.launch_operations) return 1;
	const auto home = GetLaunchCenterReference(center);
	const auto area = GetLaunchFacilityZone(center.launch_center->tile, center.launch_center->rotation, LaunchFacilityZone::Workshop);
	if (!area) return 1;
	const auto &slots = *center.launch_operations;
	const auto *hull = ResolveSpaceCraft(slots.workshop);
	// Only this actual unfinished owned body may operate this workshop. A stale
	// slot or another facility's body supplies no work or wait indication.
	if (hull != nullptr && hull->owner == center.owner && hull->construction.home == home && hull->construction.placement == SpacePlacement::Assembling && hull->construction.surface_tile == area->tile && hull->construction.remaining_ticks > 0) {
		if (hull->construction.paused) return 3;
		// The native producer holds the final replacement tick until the exact
		// original body/assets can settle. Art must not paint that refusal as work.
		if (hull->construction.remaining_ticks == 1 && hull->construction.replacement &&
			!PrepareSpaceReplacement(*hull->construction.replacement, GetSpaceVehicleModel(*hull), hull->owner, {{}, {}, home})) return 3;
		return 2;
	}
	const auto *service = ResolveSpaceCraft(slots.service);
	// Earth service owns the original countdown and material admission. An
	// onboard service timer or a different center cannot operate these windows.
	if (service != nullptr && service->owner == center.owner && service->construction.home == home && service->construction.placement == SpacePlacement::SurfaceStored && service->construction.surface_tile == area->tile && service->manifest && service->manifest->service_location == SpaceServiceLocation::EarthWorkshop && service->manifest->service_ticks > 0) {
		return IsSpaceServiceWorkTick(*service) ? 2 : 3;
	}
	const auto *pod = ResolveSpacePod(slots.pod_workshop);
	// The paid original shell has one exclusive assembly slot. Stored/carried
	// shells are separate custody and cannot create a second workshop occupant.
	if (pod != nullptr && pod->state.owner == center.owner && pod->state.home == home && pod->state.custody == SpacePodCustody::Assembling && pod->state.surface_tile == area->tile && pod->state.remaining_ticks > 0) return pod->state.paused ? 3 : 2;
	return 1;
}

/** @copydoc GetLaunchFacilityArtPose */
std::optional<LaunchArtPose> GetLaunchFacilityArtPose(const Station &center, TileIndex tile, uint8_t part)
{
	if (part > 1) return std::nullopt;
	const auto zone = FacilityZone(center, tile);
	if (!zone) return std::nullopt;
	const auto &anchor = *center.launch_center;
	const auto footprint = *GetLaunchCenterFootprint(anchor.tile, anchor.rotation);
	const uint x = TileX(tile) - TileX(anchor.tile), y = TilePlaneY(tile) - TilePlaneY(anchor.tile);
	return LaunchArtPose{LaunchFleetSpriteAsset::FacilityLaunchCenter,
		*zone == LaunchFacilityZone::Workshop ? WorkshopState(center) : uint8_t{1}, 0, anchor.rotation,
		static_cast<uint8_t>(y * footprint.w + x), part};
}

/** @copydoc GetLaunchFacilityArtBounds */
std::optional<SpriteBounds> GetLaunchFacilityArtBounds(const Station &center, TileIndex tile, uint8_t part)
{
	if (part > 1) return std::nullopt;
	const auto zone = FacilityZone(center, tile);
	if (!zone) return std::nullopt;
	const auto area = *GetLaunchFacilityZone(center.launch_center->tile, center.launch_center->rotation, *zone);
	const int x = (static_cast<int>(TileX(area.tile)) - static_cast<int>(TileX(tile))) * static_cast<int>(TILE_SIZE);
	const int y = (static_cast<int>(TilePlaneY(area.tile)) - static_cast<int>(TilePlaneY(tile))) * static_cast<int>(TILE_SIZE);
	if (part == 0) {
		// Cavity/background pixels stay behind a visitor anywhere in this same
		// native zone. The inverse offset retains the source cell's exact anchor.
		return SpriteBounds{{static_cast<int8_t>(x), static_cast<int8_t>(y), -1},
			{static_cast<uint8_t>(area.w * TILE_SIZE), static_cast<uint8_t>(area.h * TILE_SIZE), 1},
			{static_cast<int8_t>(-x), static_cast<int8_t>(-y), 1}};
	}
	// The far corner of this original zone sorts its genuine near-side cuts
	// after its own eight-unit hull clearance. Other zones retain native order;
	// there is no facility-wide paint-over pass or displaced source pixel.
	const auto near_x = static_cast<int8_t>(x + area.w * TILE_SIZE - 1);
	const auto near_y = static_cast<int8_t>(y + area.h * TILE_SIZE - 1);
	return SpriteBounds{{near_x, near_y, 8}, {1, 1, 1}, {static_cast<int8_t>(-near_x), static_cast<int8_t>(-near_y), -8}};
}

/** @copydoc DrawLaunchFacilityTile */
void DrawLaunchFacilityTile(const TileInfo &tile)
{
	// Only actual Surface launch station cells may enter this renderer. Other
	// station types and layers retain their existing native graphics path.
	if (!IsValidTile(tile.tile) || GetTileLayer(tile.tile) != MapLayer::Surface || !IsTileType(tile.tile, TileType::Station) || GetStationType(tile.tile) != StationType::LaunchCenter) return;
	DrawGroundSprite(SPR_CONCRETE_GROUND, PAL_NONE);
	if (IsInvisibilitySet(TransparencyOption::Buildings)) return;
	const auto *center = Station::GetByTile(tile.tile);
	// Two separately sortable original source parts let a real native visitor
	// appear between its own zone's background and physical roof/near arm.
	for (uint8_t part = 0; part < 2; ++part) {
		const auto pose = GetLaunchFacilityArtPose(*center, tile.tile, part);
		const auto bounds = GetLaunchFacilityArtBounds(*center, tile.tile, part);
		if (!pose || !bounds) continue; // Missing geometry/coverage cannot borrow another native family.
		const auto sprite = ResolveLaunchSprite(*pose);
		if (sprite) AddSortableSpriteToDraw(*sprite, PAL_NONE, tile, *bounds, IsTransparencySet(TransparencyOption::Buildings));
	}
}

/** @copydoc IsLaunchSurfaceCraftArtVisible */
bool IsLaunchSurfaceCraftArtVisible(const Vehicle &vehicle)
{
	if (vehicle.type != VehicleType::Space) return false;
	const auto &craft = *SpaceVehicle::From(&vehicle);
	// Only the qualified original rocket acquires Surface presentation. A
	// carried passenger, workshop body, other role or orbital hull stays with
	// its existing visibility/custody; no native Hidden flag is rewritten.
	if (GetSpaceVehicleModel(craft) != 14 || GetSpaceVehicleLayer(craft) != MapLayer::Surface ||
		!GetLaunchRocketArtPose(craft, GetLaunchRocketArtDirection(craft))) return false;
	const auto *center = ResolveLaunchCenter(craft.construction.home);
	if (center == nullptr || center->owner != craft.owner || !center->launch_operations) return false;
	const auto tile = craft.construction.surface_tile;
	// Presentation must use the actual station cell and projected native hull,
	// not only a historical Surface coordinate retained during carried custody.
	if (!IsValidTile(tile) || craft.tile != tile || !IsTileType(tile, TileType::Station) ||
		GetStationType(tile) != StationType::LaunchCenter || GetStationIndex(tile) != center->index) return false;
	const auto ref = GetSpaceCraftReference(craft);
	const auto &slots = *center->launch_operations;
	// The accepted workshop shell completely hides its stored body. Storage
	// queues retain the native inventory/list owner; this gate reveals only an
	// actually occupied launch/recovery plate with qualified source fits.
	if (slots.workshop == ref || slots.service == ref) return false;
	if (craft.construction.placement == SpacePlacement::SurfaceStored) {
		const auto pad = GetLaunchFacilityZone(center->launch_center->tile, center->launch_center->rotation, LaunchFacilityZone::Pad);
		return pad && pad->tile == tile && !IsSpaceFlightActive(craft) && slots.pad_active == SpaceCraftRef{} && !slots.pad_queue.empty() && slots.pad_queue.front() == ref;
	}
	if (craft.construction.placement != SpacePlacement::InTransit) return false;
	// An active Surface leg still owns its exact original reservation. Missing
	// or foreign flight/center identities cannot reveal another paid body.
	if (!craft.transition || slots.pad_active != ref || craft.transition->recovery != craft.construction.home) return false;
	const auto phase = craft.transition->phase;
	if (phase == SpaceFlightPhase::Preparing || phase == SpaceFlightPhase::Ascent) {
		const auto pad = GetLaunchFacilityZone(center->launch_center->tile, center->launch_center->rotation, LaunchFacilityZone::Pad);
		return pad && pad->tile == tile;
	}
	if (phase == SpaceFlightPhase::Turnaround) {
		const auto recovery = GetLaunchFacilityZone(center->launch_center->tile, center->launch_center->rotation, LaunchFacilityZone::Recovery);
		return recovery && recovery->tile == tile;
	}
	return false;
}

/** Last rendered-workshop observations only; native station references remain the sole registry.
 * This bounded disposable cache has no saved clock, job, reservation or work admission.
 */
static std::vector<std::pair<LaunchCenterRef, uint8_t>> _launch_facility_cues;

/** @copydoc RefreshLaunchFacilityArt */
void RefreshLaunchFacilityArt()
{
	// Menus/bootstrap own no playable facility view; discarding observations
	// cannot create, retire or advance any native job or paid station identity.
	if (_game_mode != GameMode::Normal && _game_mode != GameMode::Editor) { _launch_facility_cues.clear(); return; }
	std::vector<std::pair<LaunchCenterRef, uint8_t>> current;
	current.reserve(GetLaunchCenterReferences().size());
	// Read only the existing bounded registry. A changed native cue invalidates
	// its original twelve map cells even when a paused command earns zero ticks.
	// Stable cues request no new redraw; removed centers already dirty their tiles.
	for (const auto ref : GetLaunchCenterReferences()) {
		const auto *center = ResolveLaunchCenter(ref);
		if (center == nullptr) continue;
		const uint8_t state = WorkshopState(*center);
		/** @param observed Prior disposable render observation. @return Whether it names this exact native incarnation. */
		const auto same_center = [ref](const auto &observed) { return observed.first == ref; };
		const auto previous = std::ranges::find_if(_launch_facility_cues, same_center);
		if (previous == _launch_facility_cues.end() || previous->second != state) center->MarkTilesDirty(false);
		current.emplace_back(ref, state);
	}
	_launch_facility_cues.swap(current);
}

/** @copydoc GetLaunchCraftArtBounds */
SpriteBounds GetLaunchCraftArtBounds(const Vehicle &vehicle)
{
    auto bounds = vehicle.bounds;
    if (vehicle.type != VehicleType::Space) return bounds;
    const auto &craft = *SpaceVehicle::From(&vehicle);
    // Qualified whole-body source anchors describe the native center contact.
    // Preserve the physical sorting box, but cancel its legacy negative XY
    // origin for drawing and picking, so a Surface rocket rests on its plate.
    if (GetSpaceVehicleModel(craft) == 14 && GetLaunchRocketArtPose(craft, GetLaunchRocketArtDirection(craft))) {
        bounds.offset.x -= bounds.origin.x;
        bounds.offset.y -= bounds.origin.y;
    }
    return bounds;
}
