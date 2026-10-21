"""Read-only launch-center source geometry from the actual native producer.

@file launch_facility_art_contract.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group61B.
@see docs/zz-reach2/upgrades/2026-10/r2mlvs9-launch-centers-and-rockets.md Group2B/task16.2.
These art descriptors allocate no station, craft, inventory, work or SpriteID.
The focused native fixture independently verifies every declared rectangle.
"""

from copy import deepcopy


ASSET = "facility.launch-center"
VIEWS = ("NE", "SE", "SW", "NW")
ZONES = ("pad", "workshop", "recovery", "storage")
# Native quarter turns move the same exclusive work areas, rather than
# mirroring a picture or adding four cells to make the site square.
ZONE_RECTS = (
    ((0, 0, 1, 2), (1, 0, 2, 2), (3, 0, 1, 2), (0, 2, 4, 1)),
    ((1, 0, 2, 1), (1, 1, 2, 2), (1, 3, 2, 1), (0, 0, 1, 4)),
    ((3, 1, 1, 2), (1, 1, 2, 2), (0, 1, 1, 2), (0, 0, 4, 1)),
    ((0, 3, 2, 1), (0, 1, 2, 2), (0, 0, 2, 1), (2, 0, 1, 4)),
)


def launch_facility_contract():
    """Describe native cell layouts and the owners of visible work and occupants.

    @return Fresh JSON-compatible fields for this existing public asset only.
    """
    layouts = {}
    # Each physical rotation has its own world cells. Source labels name the
    # corresponding quarter turn; they cannot change gameplay orientation.
    for rotation, view in enumerate(VIEWS):
        width, height = (4, 3) if rotation % 2 == 0 else (3, 4)
        layouts[view] = dict(rotation=rotation, footprint=[width, height],
                             zones={name: list(rect) for name, rect in zip(ZONES, ZONE_RECTS[rotation])})
    return dict(consumer=9, kind="building", footprint=[4, 3], tile_variants=12,
                directions=list(VIEWS), native_rotations=layouts,
                native_projection=dict(owner="RemapCoords in src/landscape.h; native pixels before ZOOM_BASE scaling",
                                       x_step=[-32, 16], y_step=[32, 16]),
                variant_semantics="row_major_world_cells_within_each_rotated_footprint",
                native_layout_owner="LAUNCH_CENTER_LONG_EDGE/LAUNCH_CENTER_SHORT_EDGE; GetLaunchCenterFootprint/GetLaunchFacilityZone",
                native_state_owner="Station::launch_operations; SpaceConstructionState::remaining_ticks/paused; SpaceManifest::service_ticks/service_paused; SpacePodState::remaining_ticks/paused; SpaceTransition::phase/blocked/remaining_ticks",
                native_frame_policy={
                    "construction": "placement_preview_only; CmdBuildLaunchCenter installs a complete facility, no facility construction clock",
                    "ready": "static empty facility; logical frames alias the same selected hardware",
                    "working": "only actual workshop/service/pod work or SpaceTransition preparation/turnaround selects declared equipment parts",
                    "blocked": "static hardware/status from actual native admission/wait reason; no work or queue advancement"},
                native_occupant_policy="LaunchOperations workshop/service/pod_workshop/pad_active/pad_queue reference existing paid actors; rocket/probe/pod art stays separate",
                native_inventory_policy="Original station/hub/vehicle CargoPackets only; storage art creates no stock or freight delivery",
                native_water_policy="At least one actual orthogonal navigable Surface water edge; source art fixes no water side and contains no decorative water",
                native_freight_policy="Joined or explicitly linked same-company Surface rail/lorry members retain their own footprints; no baked track, road, vehicle or station extension")


def validate_launch_facility_contract(row):
    """Reject draft geometry or source policies that invent native facility state.

    @param row Public family row, including unrelated families left untouched.
    @return None; raise ValueError on a launch-center contract mismatch.
    """
    if row["asset_id"] != ASSET:
        return
    expected = launch_facility_contract()
    expected.update(native_size=[64, 64], anchor=[-31, -33], bounds=[0, 0, 64, 64],
                    geometry="per_tile", parts=["back", "front"], frames=4,
                    states=["construction", "ready", "working", "blocked"])
    # All geometry and owner fields belong to one verified native facility.
    # A source-only draft must not silently create slots, inventory or work.
    if any(row.get(key) != value for key, value in expected.items()):
        raise ValueError("native launch-center geometry/state contract changed")


def source_brief():
    """Freeze the single physical model before selecting a raster original.

    @return Planned authoring brief with actual layout and explicit open gates.
    """
    return dict(
        _documentation="Owner: r2mlvs14-remaining-assets-and-runtime-integration.md Group61B; geometry: r2mlvs9-launch-centers-and-rockets.md Group2B.",
        asset_id=ASSET, contract=deepcopy(launch_facility_contract()),
        model=dict(pad="One empty 1x2 launch plate with a slim service gantry",
                   workshop="One low 2x2 assembly/service hangar, no second building",
                   recovery="One separate empty 1x2 recovery plate",
                   storage="One low 4x1 covered freight/storage strip, empty of cargo",
                   height_policy="Keep each cell's registered pixels inside its native 64x64 canvas; validate the selected original before approving height",
                   style="Existing M1 chunky isometric machinery; pale gray/cream, charcoal, dark blue glazing, restrained ochre hardware; upper-left light",
                   exclusions=["rocket", "probe", "pod", "cargo", "people", "road", "track", "water", "labels", "invented equipment"]),
        native_cell=dict(size=[64, 64], anchor=[-31, -33], floor_steps=[[-32, 16], [32, 16]]),
        source_coverage=False, packed=False, runtime=False)
