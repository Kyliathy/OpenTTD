/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/**
 * @file launch_facility_sprite.cpp Native launch source cells, original work admission and physical sorting bounds.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group62JB
 * Explicit isolated station/actor/input components below claim no paid recipe,
 * ordinary gameplay or completed facility render/selection acceptance.
 */
#include "../stdafx.h"
#include "probe_mining_fixture.h"
#include "../launch_facility_sprite.h"
#include "../launch_operations.h"
#include "../space_manifest.h"
#include "../space_transition.h"
#include "../space_pod.h"
#include "../station_base.h"
#include "../station_map.h"
#include "../map_layer_gui.h"
#include "../viewport_type.h"
#include "../vehicle_func.h"
#include "../vehicle_gui.h"
#include <set>
#include "../safeguards.h"

/** One actual component station plus original native hull/pod work owners. */
class LaunchFacilityArtRig : public ProbeExtractionRig {
public:
	std::unique_ptr<Station> center; ///< Single native Station; no art-local operational owner.
	/** Establish an empty isolated facility identity; no build command or procurement is claimed. */
	LaunchFacilityArtRig()
	{
		REQUIRE(SpacePod::GetNumItems() == 0); REQUIRE(Station::CanAllocateItem()); REQUIRE(GetLaunchCenterReferences().empty());
		const auto anchor = TileXYL(20, 20, MapLayer::Surface);
		center.reset(Station::Create(anchor)); center->owner = _current_company;
		InstallNewLaunchCenter(*center, anchor, 0); // Existing owner installs the component's exact incarnation; no paid build is asserted.
	}
	/** Retire only declared component owners before the base fixture restores its original world. */
	~LaunchFacilityArtRig() { _space_pod_pool.CleanPool(); center.reset(); }
	/** Create an actual component hull at this workshop's existing native custody cell.
	 * @param model Delivered native Earth role with its original Engine definition.
	 * @return Same native Vehicle with explicit fixture work/placement; no paid acceptance is implied.
	 */
	SpaceVehicle &WorkshopCraft(uint16_t model)
	{
		REQUIRE(EnsureSpaceEngineDefinition(model)); auto &craft = Probe(4, false);
		craft.engine_type = GetSpaceEngine(model); craft.construction.model = model;
		craft.construction.home = GetLaunchCenterReference(*center);
		craft.construction.surface_tile = GetLaunchFacilityZone(center->launch_center->tile, center->launch_center->rotation, LaunchFacilityZone::Workshop)->tile;
		craft.construction.placement = SpacePlacement::Assembling; craft.construction.remaining_ticks = 2;
		return craft;
	}
	/** Read one original workshop source cue through the public selector.
	 * @return Qualified native source state; missing coverage fails this component case.
	 */
	uint8_t WorkshopState() const
	{
		const auto area = GetLaunchFacilityZone(center->launch_center->tile, center->launch_center->rotation, LaunchFacilityZone::Workshop);
		REQUIRE(area); const auto pose = GetLaunchFacilityArtPose(*center, area->tile, 0); REQUIRE(pose); return pose->state;
	}
};

/** Every actual source cell retains its anchor and its own zone's foreground scope in every rotation. */
TEST_CASE("Launch facility cell poses and bounds preserve four real footprints and local visitor zones", "[launch-facility-art]")
{
	LaunchFacilityArtRig rig; std::set<SpriteID> sprites;
	// Independent source views follow native rotated row-major footprints; each
	// part keeps its cell contact despite a derived sorting origin elsewhere in its zone.
	for (uint8_t rotation = 0; rotation < 4; ++rotation) {
		rig.center->launch_center->rotation = rotation;
		const auto footprint = *GetLaunchCenterFootprint(rig.center->launch_center->tile, rotation);
		uint8_t index = 0;
		for (TileIndex tile : footprint) {
			for (uint8_t part = 0; part < 2; ++part) {
				const auto pose = GetLaunchFacilityArtPose(*rig.center, tile, part); REQUIRE(pose);
				CHECK(pose->asset == LaunchFleetSpriteAsset::FacilityLaunchCenter); CHECK(pose->state == 1); CHECK(pose->frame == 0);
				CHECK(pose->direction == rotation); CHECK(pose->tile == index); CHECK(pose->part == part);
				const auto sprite = ResolveLaunchSprite(*pose); REQUIRE(sprite); CHECK(sprites.insert(*sprite).second);
				const auto bounds = GetLaunchFacilityArtBounds(*rig.center, tile, part); REQUIRE(bounds);
				CHECK(bounds->origin.x + bounds->offset.x == 0); CHECK(bounds->origin.y + bounds->offset.y == 0); CHECK(bounds->origin.z + bounds->offset.z == 0);
				// Locate the original disjoint native zone independently of the art
				// helper, then require its exact limits rather than a global front pass.
				for (auto zone : {LaunchFacilityZone::Pad, LaunchFacilityZone::Workshop, LaunchFacilityZone::Recovery, LaunchFacilityZone::Storage}) {
					const auto area = *GetLaunchFacilityZone(footprint.tile, rotation, zone);
					if (!area.Contains(tile)) continue;
					const int world_x = TileX(tile) * TILE_SIZE + bounds->origin.x;
					const int world_y = TileY(tile) * TILE_SIZE + bounds->origin.y;
					if (part == 0) {
						CHECK(world_x == TileX(area.tile) * TILE_SIZE); CHECK(world_y == TileY(area.tile) * TILE_SIZE);
						CHECK(bounds->extent.x == area.w * TILE_SIZE); CHECK(bounds->extent.y == area.h * TILE_SIZE); CHECK(bounds->origin.z == -1);
					} else {
						CHECK(world_x == (TileX(area.tile) + area.w) * TILE_SIZE - 1); CHECK(world_y == (TileY(area.tile) + area.h) * TILE_SIZE - 1);
						CHECK(bounds->extent.x == 1); CHECK(bounds->extent.y == 1); CHECK(bounds->origin.z == 8);
					}
				}
			}
			++index;
		}
		CHECK(index == 12);
	}
	CHECK(sprites.size() == 96);
	const auto outside = TileXYL(19, 20, MapLayer::Surface);
	CHECK_FALSE(GetLaunchFacilityArtPose(*rig.center, outside, 0)); CHECK_FALSE(GetLaunchFacilityArtBounds(*rig.center, outside, 0));
	CHECK_FALSE(GetLaunchFacilityArtPose(*rig.center, TileXYL(20, 20, MapLayer::Space), 0));
	CHECK_FALSE(GetLaunchFacilityArtPose(*rig.center, INVALID_TILE, 0));
	CHECK_FALSE(GetLaunchFacilityArtPose(*rig.center, rig.center->launch_center->tile, 2));
	rig.center->launch_center->rotation = 4; CHECK_FALSE(GetLaunchFacilityArtPose(*rig.center, rig.center->launch_center->tile, 0));
}

/** Only original owned workshop assembly can select a working cue; all unrelated cells stay ready. */
TEST_CASE("Launch workshop assembly cues retain original pause replacement and identity refusals", "[launch-facility-art]")
{
	LaunchFacilityArtRig rig; auto &craft = rig.WorkshopCraft(14);
	rig.center->launch_operations->workshop = GetSpaceCraftReference(craft);
	const auto before = craft.construction; const auto random = _random;
	const auto area = *GetLaunchFacilityZone(rig.center->launch_center->tile, 0, LaunchFacilityZone::Workshop);
	const auto footprint = *GetLaunchCenterFootprint(rig.center->launch_center->tile, 0);
	// A local job changes only its own source cells; pad/recovery/storage keep
	// their static hardware and acquire no job, cargo or independent timer.
	for (auto tile : footprint) {
		const auto pose = GetLaunchFacilityArtPose(*rig.center, tile, 1); REQUIRE(pose);
		CHECK(pose->state == (area.Contains(tile) ? 2 : 1)); CHECK(pose->frame == 0);
	}
	CHECK(craft.construction == before); CHECK(_random.state[0] == random.state[0]); CHECK(_random.state[1] == random.state[1]);
	craft.construction.paused = true; CHECK(rig.WorkshopState() == 3);
	craft.construction.paused = false; craft.construction.remaining_ticks = 1;
	craft.construction.replacement = SpaceReplacementIntent{}; CHECK(rig.WorkshopState() == 3); CHECK(craft.construction.remaining_ticks == 1);
	craft.construction.replacement.reset(); CHECK(rig.WorkshopState() == 2);
	++rig.center->launch_operations->workshop.generation; CHECK(rig.WorkshopState() == 1);
	--rig.center->launch_operations->workshop.generation; ++craft.construction.home.generation; CHECK(rig.WorkshopState() == 1);
	--craft.construction.home.generation; rig.center->owner = OWNER_NONE; CHECK(rig.WorkshopState() == 1);
}

/** The actual native service API and paid-shell custody remain the work/cue owners. */
TEST_CASE("Launch workshop service and pod cues read original inputs and exclusive custody", "[launch-facility-art]")
{
	LaunchFacilityArtRig rig; auto &craft = rig.WorkshopCraft(15);
	craft.construction.placement = SpacePlacement::SurfaceStored; craft.construction.remaining_ticks = 0;
	craft.manifest = std::make_unique<SpaceManifest>(); auto &manifest = *craft.manifest;
	rig.center->launch_operations->service = GetSpaceCraftReference(craft);
	const auto tuning = GetSpaceServiceTuning(15); REQUIRE(tuning); manifest.service_ticks = tuning->ticks;
	CHECK(rig.WorkshopState() == 3); CHECK_FALSE(IsSpaceServiceWorkTick(craft));
	// These explicitly supplied component packets test original admission only;
	// no paid-world supply chain or simulation opportunity is asserted here.
	for (uint8_t input = 0; input < 2; ++input) {
		REQUIRE(CargoPacket::CanAllocateItem());
		manifest.supplies[input + 1].Append(CargoPacket::Create(tuning->inputs[input], uint16_t{0}, rig.center->index, rig.center->launch_center->tile, Money{0}), StationID::Invalid());
	}
	REQUIRE(IsSpaceServiceWorkTick(craft)); CHECK(rig.WorkshopState() == 2); CHECK(manifest.service_ticks == tuning->ticks);
	manifest.service_paused = true; CHECK(rig.WorkshopState() == 3);
	manifest.service_paused = false; manifest.service_location = SpaceServiceLocation::Onboard; CHECK(rig.WorkshopState() == 1);
	rig.center->launch_operations->service = {};
	REQUIRE(SpacePod::CanAllocateItem()); auto *pod = SpacePod::Create(); pod->generation = 7;
	pod->state.owner = rig.center->owner; pod->state.home = GetLaunchCenterReference(*rig.center);
	pod->state.surface_tile = craft.construction.surface_tile; pod->state.custody = SpacePodCustody::Assembling; pod->state.remaining_ticks = 2;
	rig.center->launch_operations->pod_workshop = GetSpacePodReference(*pod);
	const auto original = pod->state; CHECK(rig.WorkshopState() == 2); CHECK(pod->state == original);
	pod->state.paused = true; CHECK(rig.WorkshopState() == 3);
	pod->state.custody = SpacePodCustody::CenterStored; CHECK(rig.WorkshopState() == 1);
	++rig.center->launch_operations->pod_workshop.generation; CHECK(rig.WorkshopState() == 1);
}

/** The original hidden rocket draws/selects only at its exact current Surface pad or recovery plate. */
TEST_CASE("Surface rocket presentation and native picking preserve layers slots flags and custody", "[launch-facility-art]")
{
	LaunchFacilityArtRig rig; auto &craft = rig.WorkshopCraft(14);
	craft.construction.placement = SpacePlacement::SurfaceStored; craft.construction.remaining_ticks = 0;
	craft.vehstatus.Set(VehState::Hidden);
	const auto ref = GetSpaceCraftReference(craft); auto &slots = *rig.center->launch_operations;
	slots.pad_queue.push_back(ref);
	// Explicitly installed component station cells test the real map/picking
	// owner. This fixture grants no paid build, materials, command or earned tick.
	for (uint8_t rotation = 0; rotation < 4; ++rotation) {
		rig.center->launch_center->rotation = rotation;
		const auto footprint = *GetLaunchCenterFootprint(rig.center->launch_center->tile, rotation);
		for (auto tile : footprint) MakeStation(tile, rig.center->owner, rig.center->index, StationType::LaunchCenter, rotation);
		craft.construction.surface_tile = GetLaunchFacilityZone(footprint.tile, rotation, LaunchFacilityZone::Pad)->tile;
		craft.UpdateSpacePosition();
		const auto before = craft.construction; const auto status = craft.vehstatus;
		REQUIRE(IsLaunchSurfaceCraftArtVisible(craft)); CHECK(GetVehicleDisplayLayer(&craft) == MapLayer::Surface);
		Viewport view{}; view.width = 640; view.height = 360; view.zoom = ZoomLevel::Min; view.layer = MapLayer::Surface;
		view.virtual_left = craft.coord.left - 100; view.virtual_top = craft.coord.top - 100;
		CHECK(CheckClickOnVehicle(view, 100, 100) == &craft);
		view.layer = MapLayer::Space; CHECK(CheckClickOnVehicle(view, 100, 100) == nullptr);
		view.layer = MapLayer::Surface; craft.vehstatus.Set(VehState::Unclickable); CHECK(CheckClickOnVehicle(view, 100, 100) == nullptr);
		craft.vehstatus.Reset(VehState::Unclickable);
		CHECK(craft.construction == before); CHECK(craft.vehstatus == status); CHECK(slots.pad_queue.front() == ref);
	}
	++slots.pad_queue.front().generation; CHECK_FALSE(IsLaunchSurfaceCraftArtVisible(craft)); --slots.pad_queue.front().generation;
	++craft.construction.home.generation; CHECK_FALSE(IsLaunchSurfaceCraftArtVisible(craft)); --craft.construction.home.generation;
	rig.center->owner = OWNER_NONE; CHECK_FALSE(IsLaunchSurfaceCraftArtVisible(craft)); rig.center->owner = craft.owner;
	slots.pad_active = ref; CHECK_FALSE(IsLaunchSurfaceCraftArtVisible(craft)); slots.pad_queue.clear();
	craft.construction.placement = SpacePlacement::InTransit; craft.transition = std::make_unique<SpaceTransition>();
	auto &flight = *craft.transition; flight.recovery = craft.construction.home; flight.layer = MapLayer::Surface;
	flight.phase = SpaceFlightPhase::Preparing; flight.remaining_ticks = 148;
	CHECK(IsLaunchSurfaceCraftArtVisible(craft));
	flight.remaining_ticks = 0; flight.blocked = SpaceFlightBlock::TargetLost; CHECK(IsLaunchSurfaceCraftArtVisible(craft));
	flight.phase = SpaceFlightPhase::Ascent; flight.remaining_ticks = 222; flight.outbound_burned = true; CHECK(IsLaunchSurfaceCraftArtVisible(craft));
	flight.phase = SpaceFlightPhase::OrbitalDelivery; flight.layer = MapLayer::Space; CHECK_FALSE(IsLaunchSurfaceCraftArtVisible(craft));
	flight.phase = SpaceFlightPhase::ReturnRecovery; flight.recovery_burned = true; CHECK_FALSE(IsLaunchSurfaceCraftArtVisible(craft));
	flight.phase = SpaceFlightPhase::Turnaround; flight.layer = MapLayer::Surface; flight.remaining_ticks = 74;
	craft.construction.surface_tile = GetLaunchFacilityZone(rig.center->launch_center->tile, rig.center->launch_center->rotation, LaunchFacilityZone::Recovery)->tile;
	craft.UpdateSpacePosition(); REQUIRE(IsLaunchSurfaceCraftArtVisible(craft));
	const auto before = flight; CHECK(IsLaunchSurfaceCraftArtVisible(craft)); CHECK(flight == before); CHECK(craft.vehstatus.Test(VehState::Hidden));
	++slots.pad_active.generation; CHECK_FALSE(IsLaunchSurfaceCraftArtVisible(craft)); --slots.pad_active.generation;
	craft.construction.placement = SpacePlacement::Carried; CHECK_FALSE(IsLaunchSurfaceCraftArtVisible(craft));
	craft.construction.placement = SpacePlacement::Assembling; CHECK_FALSE(IsLaunchSurfaceCraftArtVisible(craft));
	craft.construction.placement = SpacePlacement::SurfaceStored; slots.pad_active = {}; slots.pad_queue.push_back(ref);
	CHECK_FALSE(IsLaunchSurfaceCraftArtVisible(craft)); // Recovery storage is not the queue head's physical pad cell.
	craft.engine_type = GetSpaceEngine(15); craft.construction.model = 15; CHECK_FALSE(IsLaunchSurfaceCraftArtVisible(craft));
}

/** Disposable cue observation must see paused job changes without creating work, a timer or another registry. */
TEST_CASE("Facility cue refresh observes native paused work without changing simulation owners", "[launch-facility-art]")
{
	LaunchFacilityArtRig rig; auto &craft = rig.WorkshopCraft(14);
	rig.center->launch_operations->workshop = GetSpaceCraftReference(craft);
	const auto tick = TimerGameTick::counter; const auto random = _random;
	const auto center = ComputeLaunchCenterDigest(), hub = ComputeLaunchHubDigest();
	const auto original = craft.construction;
	RefreshLaunchFacilityArt(); RefreshLaunchFacilityArt();
	CHECK(craft.construction == original); CHECK(rig.WorkshopState() == 2); CHECK(ComputeLaunchCenterDigest() == center);
	craft.construction.paused = true; // Explicit retained native input change, with no simulation opportunity.
	const auto paused = craft.construction;
	const auto paused_center = ComputeLaunchCenterDigest(); // The native center digest includes original production/paused fields.
	RefreshLaunchFacilityArt(); RefreshLaunchFacilityArt();
	CHECK(craft.construction == paused); CHECK(rig.WorkshopState() == 3);
	CHECK(ComputeLaunchCenterDigest() == paused_center); CHECK(ComputeLaunchHubDigest() == hub);
	CHECK(rig.center->launch_operations->workshop == GetSpaceCraftReference(craft));
	CHECK(craft.cargo.TotalCount() == 0); CHECK(TimerGameTick::counter == tick);
	CHECK(_random.state[0] == random.state[0]); CHECK(_random.state[1] == random.state[1]);
}
