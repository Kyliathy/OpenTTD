#!/usr/bin/env python3
"""M0 manifest/schema/placement fixtures; no final raster art is generated.

@file multilayer_sprite_contract.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs13-graphics-and-asset-pipeline.md
@see docs/zz-reach2/upgrades/2026-10/graphics/r2mlvs13-b-m1-authoring.md (measured native consumer geometry)
"""

import argparse
import hashlib
import json
from pathlib import Path
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parents[2]
ASSETS = ROOT / "media/multilayer/contracts"
MANIFEST = ASSETS / "multilayer_sprite_manifest.json"
DIRECTIONS = ["N", "NE", "E", "SE", "S", "SW", "W", "NW"]
DOORS = ["NE", "SE", "SW", "NW"]


def record(asset_id, consumer, kind, states, frames=1, footprint=(1, 1), directions=None):
    """Create one exact logical family contract without claiming delivered source art.

    @param asset_id Stable content/geometry family identity, never a runtime SpriteID.
    @param consumer Owning feature-plan number.
    @param kind Icon, terrain, vehicle or building placement policy.
    @param states Explicit complete gameplay-state names.
    @param frames Exact frame count per state and direction.
    @param footprint Static world tile dimensions; a portal's pair coverage means one independent tile per endpoint, and moving vehicles have no static floor.
    @param directions Explicit engine views; defaults depend on placement kind.
    @return One manifest record with pending source/packing status.
    """
    sizes = {"icon": (20, 20, 0, 0), "terrain": (64, 31, -31, 0),
             "vehicle": (64, 48, -32, -32), "building": (64, 64, -31, -33)}
    width, height, x, y = sizes[kind]
    views = directions if directions is not None else DIRECTIONS if kind == "vehicle" else DOORS if kind == "building" else ["none"]
    parts = ["back", "front"] if kind == "building" else ["body"]
    # Both native cutters have a real permanent articulated support part. Source coverage must supply that
    # separate moving part instead of drawing another cutter head or counting one whole-machine image twice.
    if asset_id in {"vehicle.model-00", "vehicle.model-01"}:
        parts.append("support")
    return {"asset_id": asset_id, "consumer": consumer, "kind": kind,
            "directions": views, "frames": frames, "states": states,
            "native_size": [width, height], "footprint": list(footprint),
            "footprint_semantics": "paired_endpoint_tiles" if asset_id.startswith("facility.portal.") else "world_tiles",
            "anchor": [x, y], "bounds": [0, 0, width, height], "parts": parts,
            "geometry": "per_tile" if kind == "building" else "per_view",
            "tile_variants": footprint[0] * footprint[1] if kind == "building" else 1,
            "palette": "dos_8bpp", "transparent_index": 0,
            "company_colors": "explicit_mask" if kind == "vehicle" else "none",
            "preview": "native_build_pose" if kind == "building" else "purchase_family" if kind == "vehicle" else "native_icon_or_overlay",
            "animation_colors": "forbidden", "source": {"status": "planned", "path": None,
            "provenance": "first_party_original", "rights": "original_to_be_authored"},
            "packed": {"namespace": "MLVI", "offset": None, "sha256": None}}


def build_manifest():
    """Expand every requested functional family before asset authoring.

    @return Manifest with exact directions/states/frames, protected legacy offsets and pending sources.
    """
    rows = []
    # Separate terrain/depth/resource rows preserve readable identities and never reuse a generic ore icon.
    for depth in ("underground", "deep"):
        rows.append(record(f"terrain.{depth}", 2, "terrain", ["solid", "cut_face", "excavating", "excavated"], 3))
    for cargo in ("COAL", "IORE", "CORE", "BAUX", "SALT", "LITH", "NORE", "COBT", "RARE", "URAN"):
        rows.append(record(f"survey.{cargo}", 2, "icon", ["unknown", "surveyed", "depleted"]))
        rows.append(record(f"mine.{cargo}", 6, "building", ["construction", "working", "blocked", "depleted"], 4, (3, 3)))
    # Consumer ordinals follow the split plan index: tubes17 and geothermal18.
    # Native model IDs and all physical/state geometry retain their existing meaning.
    # The nineteen model IDs match the public content catalogue; purchase previews are separate explicit assets.
    for model in range(19):
        owner = 4 if model < 2 else 5 if model < 5 else 7 if model < 9 else 17 if model < 14 else 9 if model in (14, 18) else 10 if model == 15 else 11
        states = ["empty", "partial", "full", "service", "blocked"] if model not in (0, 1, 4, 16) else ["idle", "moving", "working", "blocked"]
        if 9 <= model <= 13:
            # Native capsule load and power/service condition are independent.
            # A held or serviced capsule must retain its visible actual load.
            states = ["empty", "partial", "full", "service-empty", "service-partial", "service-full", "blocked-empty", "blocked-partial", "blocked-full"]
        space_states = {14: ["ready", "preparing", "ascent", "orbit", "recovery", "blocked", "service"],
                        15: ["free", "rendezvous", "attached", "mining", "full", "return", "service", "blocked"],
                        16: ["idle", "moving", "working", "service", "blocked"],
                        17: ["empty", "partial", "full", "moving", "service", "blocked"],
                        18: ["packed", "descending", "recovered", "blocked"]}
        states = space_states.get(model, states)
        # Machinery source tasks include real backing and ordinary service. These
        # are presentation views of existing train/controller facts, not new saved
        # work phases; the support remains permanently articulated in every view.
        if model < 2:
            states.extend(["reversing", "service"])
        elif model == 4:
            states.append("service")
        rows.append(record(f"vehicle.model-{model:02}", owner, "vehicle", states, 4 if model < 2 else 2, (0, 0)))
        rows.append(record(f"purchase.model-{model:02}", owner, "icon", ["available", "locked"]))
    # Each facility has an explicit footprint and back/front pair; local-ID packing belongs to its feature owner.
    facilities = [("portal.sealed", 3, (2, 1)), ("portal.deep-rail", 3, (2, 1)),
                  ("exchange", 5, (1, 1)), ("rail-station", 7, (3, 2)), ("road-terminal", 7, (2, 2)),
                  ("rail-service", 7, (1, 1)), ("road-service", 7, (1, 1)),
                  ("earth-receiver", 6, (2, 3)), ("rock-receiver", 6, (4, 4)),
                  ("metal-refinery", 6, (4, 4)), ("chemical-works", 6, (4, 4)),
                  ("battery-works", 6, (4, 4)), ("electronics-works", 6, (4, 4)),
                  ("component-works", 6, (4, 4)), ("robotics-works", 6, (4, 4)),
                  ("mirror-kit-works", 6, (4, 4)), ("solar-farm", 12, (3, 2)), ("nuclear-plant", 12, (4, 4)),
                  ("tube-terminal", 17, (1, 1)), ("tube-service", 17, (1, 1)), ("tube-interchange", 17, (3, 2)),
                  ("vent-collector", 18, (3, 3)), ("vent-powerhouse", 18, (4, 4)),
                  ("magma-powerhouse", 18, (4, 4)), ("heat-smelter", 18, (4, 4)),
                  ("heat-battery-works", 18, (4, 4)), ("mantle-forge", 18, (4, 4)),
                  ("utility-riser", 18, (2, 2)), ("launch-center", 9, (4, 4))]
    for name, owner, footprint in facilities:
        # A portal's real owner has exactly three saved work phases. Its housing
        # is static within each phase; controller waits have separate UI cues and
        # cannot introduce a fourth saved phase or fictitious animation frames.
        portal = name.startswith("portal.")
        states = ["sealed", "commissioning", "connected"] if portal else ["construction", "ready", "working", "blocked"]
        # The delivered collector publishes Heat instead of requesting it.
        # Its four construction frames follow real native stages; consumer
        # shortage semantics remain with the other, separately owned families.
        if owner == 18 and name != "vent-collector":
            states.append("heat_starved")
        # Native Surface spoil receivers expose one fixed industry layout. They
        # have no build rotation/entry direction; authored cells follow actual
        # world offsets rather than pretending to offer four depot approaches.
        receiver = name in ("earth-receiver", "rock-receiver")
        row = record(f"facility.{name}", owner, "building", states, 1 if portal else 4, footprint,
                     directions=["none"] if receiver else DOORS)
        if name == "vent-collector":
            row["state_owner"] = "Actual native collector construction/completion, IsGeothermalCollectorOperational and current shared Heat publication; no Heat consumer or copied energy balance"
            row["frame_owner"] = {"construction": "native_industry_construction_stage_0_to_3_while_uncompleted",
                                  "ready": "original_equipment_glass_phase_0_to_3",
                                  "working": "original_equipment_glass_phase_0_to_3",
                                  "blocked": "original_equipment_glass_phase_0_to_3"}
            row["construction_stages"] = [0, 1, 2, 3]
        if receiver:
            row["state_owner"] = "Native industry construction; ready=idle, working=existing bounded processing, blocked=full stock/aggregate or interval refusal"
            row["layout_owner"] = "InstallSpoilReceiverDefinition; one fixed native Surface layout"
        if name == "tube-terminal":
            # Native platforms have two real track axes and four default tile
            # roles per axis. Roles are alternatives for one built cell, not
            # four world cells or four depot-style directed entrances.
            row.update(directions=["NE", "SE"], tile_variants=4, variant_semantics="native_station_graphics_role",
                       native_graphics={"NE": [0, 2, 4, 6], "SE": [1, 3, 5, 7]})
        if portal:
            row["state_owner"] = "LayerPortalWorkState; controller blocked reason is independent presentation"
            row["endpoint_variants"] = ["upper", "lower"]
        rows.append(row)
    for shape in ("straight", "bend45", "junction", "end", "crossing", "conduit"):
        # Native guideway views name installed geometry: two straight axes or
        # the four Upper/Lower/Left/Right corner pieces. Neither travel reversal
        # nor a door label can stand in for a missing native shape.
        views = ["NE", "SE"] if shape == "straight" else ["N", "S", "W", "E"] if shape == "bend45" else ["none"] if shape in ("junction", "crossing") else DOORS
        row = record(f"tube.{shape}", 17, "terrain", ["construction", "powered", "unpowered"], directions=views)
        if shape in ("straight", "bend45", "junction", "end", "crossing"):
            # Separate rear and foreground shells allow moving capsules to
            # keep their actual track path and intended near-rim occlusion.
            row["parts"] = ["back", "front"]
        if shape == "junction":
            # Topology is a native TrackBits variant, not a housing rotation.
            # Six singles, X/Y crossing and the two independent parallel-curve
            # pairs retain their own families; every shared-edge layout remains.
            layouts = [mask for mask in range(1, 64) if mask.bit_count() >= 2 and mask not in (3, 12, 48)]
            row["tile_variants"] = len(layouts)
            row["variant_semantics"] = "native_trackbits_topology"
            row["native_layouts"] = layouts
        rows.append(row)
    vent = record("geothermal.vent", 18, "terrain", ["hidden", "surveyed", "excavated"], 4)
    # Geology has one immutable natural rate and no thermal consumer or tapped
    # machinery state. Survey information and paid excavation control visibility;
    # installed collector connection/idle status belongs to its separate family.
    vent.update(state_owner="GeothermalVentField membership, active same-layer/date-valid public survey and canonical paid ExcavationState; no thermal demand or copied source balance",
                frame_owner="original_thermal_detail_phase_0_to_3_visual_only",
                hidden_policy="fully_transparent_no_source_pixels",
                empty_allowed_states=["hidden"], render_layer="ground_overlay_preserves_hardness_and_mineral_markers",
                native_draw_offset=[0, 0])
    rows.append(vent)
    rows.append(record("geothermal.conduit", 18, "terrain", ["construction", "connected", "unconnected"], directions=DOORS))
    for name in ("nucleus", "dock", "storage", "workshop", "processor", "power", "fuel-store"):
        rows.append(record(f"module.{name}", 11, "building", ["blueprint", "partial", "complete", "blocked"], 2, (2, 2)))
    for name in ("nickel", "ice", "rare-earth"):
        rows.append(record(f"asteroid.{name}", 10, "vehicle", ["free", "attached", "mining", "depleted"], 4, (0, 0)))
    rows.append(record("solar.mirror", 12, "building", ["packed", "deploying", "operating", "idle", "service"], 4, (1, 1)))
    for name in ("survey", "excavation", "tbm-area", "tbm-turn45", "exchange", "mine", "space-build", "launch", "tube", "geothermal", "deep-lock", "future-lock"):
        rows.append(record(f"control.{name}", 7, "icon", ["enabled", "disabled", "pressed"]))
    for name in ("orbital-drop", "surface-collection"):
        rows.append(record(f"overlay.{name}", 9, "terrain", ["valid", "invalid"]))
    # Cargo-icon coverage follows the same single definition source as the shipped package, including eight added mined meanings.
    cargo_catalogue = json.loads((ROOT / "media/multilayer/cargo_catalogue.json").read_text(encoding="utf-8"))
    for cargo in (entry["label"] for entry in cargo_catalogue["cargoes"]):
        rows.append(record(f"cargo.{cargo}", 1, "icon", ["normal"]))
    # Append neighbor-mask families after every existing public planned offset. A cavity border is
    # selected from actual adjacent rock cells; it must not reuse a full rim around every open tile.
    for depth in ("underground", "deep"):
        rows.append(record(f"terrain.edges.{depth}", 2, "terrain", [f"mask_{mask:02x}" for mask in range(16)]))
    sheet = ROOT / "media/baseset/openttd/map_layers.png"
    next_offset = 74
    # Core overlays get an explicit future append range; current 0..65 and optional 66..73 are never occupied by this draft.
    for row in rows:
        if row["asset_id"].startswith(("terrain.", "survey.", "control.", "overlay.")):
            row["packed"]["namespace"] = "native_map_layer_extension"
            row["packed"]["offset"] = next_offset
            next_offset += len(row["directions"]) * row["frames"] * len(row["states"]) * len(row["parts"]) * row["tile_variants"]
    # Regional material art appends after the existing M1 block. It never
    # shifts the original core reservations or any previously bound family.
    for suffix, columns in (("hardness", ["solid", "cut_face", "excavating", "excavated"]),
                            ("hardness-edges", ["upper_left", "upper_right", "lower_right", "lower_left"])):
        family = record(f"terrain.{suffix}", 2, "terrain",
                        [f"{material}_{column}" for material in ("soft", "mixed", "hard", "basalt") for column in columns])
        family["packed"]["namespace"] = "native_geology_append"
        rows.append(family)
    return {"_documentation": "Owner: r2mlvs13-graphics-and-asset-pipeline.md; geometry: src/table/sprites.h and r2mlvs1-e-ground-and-space-budgets.md",
            "schema_version": 1, "handoff": "M0_contract_only", "direction_convention": "engine_viewport",
            "legacy": {"live_count": 66, "preserved_offsets": [0, 65], "unactivated_reserved_offsets": [66, 73],
                       "reference_sheet": "media/baseset/openttd/map_layers.png", "sha256": hashlib.sha256(sheet.read_bytes()).hexdigest()},
            "core_extension": {"planned_start": 74, "planned_count": next_offset - 74, "planned_end_exclusive": next_offset,
                               "activated": False, "runtime_loader_change": "later_packing_group"},
            "assets": rows}


def validate_manifest(document):
    """Reject incomplete identities, counts, placement and falsely delivered source/packing evidence.

    @param document Decoded M0 manifest from the producer artifact.
    @return Total exact sprite requirement count; raise ValueError on an invalid contract.
    """
    identities = set()
    count = 0
    # Validate each independent family before consumers assign runtime sprite references or author sheets.
    for row in document["assets"]:
        mandatory = {"asset_id", "consumer", "directions", "frames", "states", "native_size", "footprint", "anchor", "source", "packed", "tile_variants", "preview"}
        if not mandatory.issubset(row):
            raise ValueError("missing mandatory field")
        if row["asset_id"] in identities:
            raise ValueError("duplicate asset_id")
        identities.add(row["asset_id"])
        if row["frames"] < 1:
            raise ValueError("empty animation")
        if not row["states"] or len(set(row["states"])) != len(row["states"]):
            # Repeated/missing states silently drop a functional pose from later packing.
            raise ValueError("missing/duplicate state")
        if row["kind"] == "vehicle" and row["directions"] != DIRECTIONS:
            # Moving designs require every engine view, not rotated substitutes for absent silhouettes.
            raise ValueError("incomplete vehicle directions")
        if row["asset_id"] == "facility.vent-collector":
            # Native stage3 may still be unfinished. The completion flag,
            # rather than its final-looking hardware, permits source operation;
            # an extra starvation state would invent a thermal consumer.
            if (row["states"] != ["construction", "ready", "working", "blocked"] or row["frames"] != 4
                    or row.get("construction_stages") != [0, 1, 2, 3]
                    or row.get("frame_owner", {}).get("construction") != "native_industry_construction_stage_0_to_3_while_uncompleted"):
                raise ValueError("collector native construction/source state coverage mismatch")
        if row["asset_id"] == "geothermal.vent":
            # Natural information cannot reveal a field through unsurveyed solid
            # rock, become a Heat consumer, replace hardness or suppress minerals.
            if (row["states"] != ["hidden", "surveyed", "excavated"] or row["frames"] != 4
                    or row["native_size"] != [64, 31] or row["anchor"] != [-31, 0]
                    or row.get("hidden_policy") != "fully_transparent_no_source_pixels"
                    or row.get("empty_allowed_states") != ["hidden"]
                    or row.get("render_layer") != "ground_overlay_preserves_hardness_and_mineral_markers"
                    or row.get("native_draw_offset") != [0, 0]):
                raise ValueError("natural vent visibility/native overlay contract mismatch")
        if row["asset_id"] == "tube.straight":
            # Native X/Y topology and actual foreground occlusion require
            # both independently authored axes and both aligned shell parts;
            # reversing travel cannot stand in for a missing geometric view.
            if row["directions"] != ["NE", "SE"] or row["parts"] != ["back", "front"]:
                raise ValueError("straight tube native axis/part coverage mismatch")
        if row["asset_id"] == "tube.bend45":
            # Corners follow actual Upper/Lower/Left/Right track ownership;
            # arbitrary NE/SE/SW/NW doors would hide a missing native curve.
            if row["directions"] != ["N", "S", "W", "E"] or row["parts"] != ["back", "front"]:
                raise ValueError("curved tube native corner/part coverage mismatch")
        if row["asset_id"] == "tube.junction":
            # A shell is selected by the complete installed native topology;
            # arbitrary door rotations or omitted shared-edge layouts could
            # paint an absent arm or silently drop a legal junction.
            layouts = [mask for mask in range(1, 64) if mask.bit_count() >= 2 and mask not in (3, 12, 48)]
            if (row["directions"] != ["none"] or row["parts"] != ["back", "front"]
                    or row["tile_variants"] != 54 or row.get("native_layouts") != layouts
                    or row.get("variant_semantics") != "native_trackbits_topology"):
                raise ValueError("tube junction native topology/part coverage mismatch")
        if row["asset_id"] == "tube.end":
            # A directed physical endpoint has four genuine camera views,
            # unlike an undirected straight axis. Both cap parts keep the
            # existing native mouth anchor; art never supplies passability.
            if row["directions"] != DOORS or row["parts"] != ["back", "front"]:
                raise ValueError("tube end native facing/part coverage mismatch")
        if row["asset_id"] == "tube.crossing":
            # Native X/Y has one fixed camera geometry, independent of travel
            # reversal. Both parts must leave both real routes visible; door
            # rotations would fabricate extra crossing views and hide occlusion.
            if row["directions"] != ["none"] or row["parts"] != ["back", "front"]:
                raise ValueError("tube crossing native topology/part coverage mismatch")
        if row["asset_id"] == "tube.conduit":
            # The real utility corridor uses four center-to-edge branches on
            # the existing floor. A foreground shell would hide capsules;
            # missing branches would misrepresent an installed physical path.
            if row["directions"] != DOORS or row["parts"] != ["body"]:
                raise ValueError("tube conduit native branch/ground coverage mismatch")
        if row["asset_id"] == "facility.tube-service":
            # The actual native tube depot has one physical cell and four
            # directed entrances. Draft multi-tile artwork cannot silently
            # enlarge its buildable floor, capacity or approach geometry.
            if (row["directions"] != DOORS or row["parts"] != ["back", "front"]
                    or row["footprint"] != [1, 1] or row["tile_variants"] != 1):
                raise ValueError("tube service native depot geometry mismatch")
        if row["asset_id"] in ("facility.tube-terminal", "facility.tube-interchange"):
            # Real native station commands choose all eight default graphics
            # across two axes/four roles. An invented campus footprint or door
            # rotation cannot replace a missing through-platform source cell.
            # Joined interchange tube cells retain these same physical roles;
            # their conventional partners keep separate native cells/families.
            if (row["directions"] != ["NE", "SE"] or row["parts"] != ["back", "front"]
                    or row["footprint"] != [1, 1] or row["tile_variants"] != 4
                    or row.get("variant_semantics") != "native_station_graphics_role"
                    or row.get("native_graphics") != {"NE": [0, 2, 4, 6], "SE": [1, 3, 5, 7]}):
                raise ValueError("tube terminal native axis/cell-role mismatch")
        width, height = row["native_size"]
        if width <= 0 or height <= 0:
            # Empty canvases cannot provide native-size construction/purchase feedback.
            raise ValueError("empty native canvas")
        if row["bounds"] != [0, 0, width, height]:
            raise ValueError("bounds disagree with native canvas")
        if row["palette"] != "dos_8bpp" or row["transparent_index"] != 0:
            # Every first-deliverable family shares the same palette/transparency contract.
            raise ValueError("unsupported palette")
        if row["kind"] == "building":
            # Native receivers have one fixed world layout; tube platforms use
            # two true axes. Depot/portal families retain four real entrances.
            receiver = row["asset_id"] in ("facility.earth-receiver", "facility.rock-receiver")
            platform = row["asset_id"] in ("facility.tube-terminal", "facility.tube-interchange")
            expected_directions = ["none"] if receiver else ["NE", "SE"] if platform else DOORS
            if row["directions"] != expected_directions:
                raise ValueError("incomplete building directions")
        if row["source"]["status"] == "planned":
            if row["source"]["path"] is not None or row["packed"]["sha256"] is not None:
                # M0 placeholders may not claim a nonexistent final source or packed payload hash.
                raise ValueError("planned art falsely claims delivery")
        else:
            if not (ROOT / row["source"]["path"]).is_file():
                raise ValueError("missing delivered source")
        count += len(row["directions"]) * row["frames"] * len(row["states"]) * len(row["parts"]) * row["tile_variants"]
    required = {f"mine.{label}" for label in ("COAL", "IORE", "CORE", "BAUX", "SALT", "LITH", "NORE", "COBT", "RARE", "URAN")}
    required.update(f"vehicle.model-{model:02}" for model in range(19))
    required.update(("overlay.orbital-drop", "overlay.surface-collection", "solar.mirror", "facility.vent-powerhouse", "facility.mantle-forge"))
    if not required.issubset(identities):
        raise ValueError("missing required functional family")
    reference = ROOT / document["legacy"]["reference_sheet"]
    if hashlib.sha256(reference.read_bytes()).hexdigest() != document["legacy"]["sha256"]:
        raise ValueError("legacy reference changed")
    cursor = document["core_extension"]["planned_start"]
    # Validate the proposed append layout without occupying live sprites or the unactivated optional housing range.
    for row in document["assets"]:
        if row["packed"]["namespace"] != "native_map_layer_extension":
            continue
        if row["packed"]["offset"] != cursor or cursor < 74:
            raise ValueError("overlapping/protected native offset")
        cursor += len(row["directions"]) * row["frames"] * len(row["states"])
    if cursor != document["core_extension"]["planned_end_exclusive"]:
        raise ValueError("core range count changed")
    return count


def write_templates():
    """Publish code-native placement diagrams for M0 geometry, never final raster sprites.

    @return SVG path with exact labeled engine poses and building/foreground examples.
    """
    parts = ['<svg xmlns="http://www.w3.org/2000/svg" width="960" height="660" viewBox="0 0 960 660">',
             '<!-- Owner: r2mlvs13-graphics-and-asset-pipeline.md; M0 placement template, not final sprite art. -->',
             '<rect width="960" height="660" fill="#eff2f3"/>',
             '<g font-family="monospace" font-size="12" fill="#223344">',
             '<text x="20" y="20">M0 geometry only: engine directions, 64x48 vehicle canvas, anchor (-32,-32)</text>']
    vectors = [(0, -9), (14, -7), (20, 0), (14, 7), (0, 9), (-14, 7), (-20, 0), (-14, -7)]
    # Each named engine view has an explicit cutter/front vector; support stays behind it on the same anchor.
    for index, (name, (dx, dy)) in enumerate(zip(DIRECTIONS, vectors)):
        x, y = 60 + index * 112, 80
        parts.extend([f'<g id="tbm-{name}" transform="translate({x},{y})">',
                      '<rect x="-32" y="-32" width="64" height="48" fill="none" stroke="#6c7a80"/>',
                      f'<line x1="{-dx}" y1="{-dy}" x2="{dx}" y2="{dy}" stroke="#246b83" stroke-width="10"/>',
                      f'<circle cx="{dx}" cy="{dy}" r="7" fill="#df8a2a"/>',
                      '<path d="M-3,0 H3 M0,-3 V3" stroke="#000"/>',
                      f'<text x="-12" y="38">{name}</text></g>'])
    parts.append('<text x="20" y="155">Legal bends: SW -> W -> NW, advancing map centers (0,0),(1,0),(2,-1),(2,-2)</text>')
    # Isometric projection uses the live map axes; swept 3x3 envelopes are unioned rather than charging overlapping cells twice.
    centers = [(0, 0), (1, 0), (2, -1), (2, -2)]
    for index, (cx, cy) in enumerate(centers):
        x, y = 200 + (cy - cx) * 18, 220 + (cx + cy) * 9
        parts.append(f'<path id="bend-{index}" d="M{x},{y-27} l54,27 l-54,27 l-54,-27 Z" fill="none" stroke="#23818a"/>')
    parts.extend(['<g id="mine-3x3" transform="translate(420,225)">',
                  '<path d="M0,-48 L96,0 L0,48 L-96,0 Z" fill="#ced8cc" stroke="#456"/>',
                  '<path d="M-32,-32 L32,-32 L32,16 L-32,16 Z" fill="#adb7c0" stroke="#456"/>',
                  '<text x="-65" y="72">Mine 3x3 / states</text></g>',
                  '<g id="rocket-deploy-recover" transform="translate(650,225)">',
                  '<rect x="-32" y="-32" width="64" height="48" fill="none" stroke="#789"/>',
                  '<path d="M0,-28 L12,-12 L12,12 L-12,12 L-12,-12 Z" fill="#bac6d1" stroke="#456"/>',
                  '<text x="-70" y="72">Deploy/recovery anchor</text></g>',
                  '<g id="module-back-vehicle-front" transform="translate(835,225)">',
                  '<path id="module-back" d="M-32,-28 L32,-28 L32,16 L-32,16 Z" fill="#aebac4" stroke="#456"/>',
                  '<rect id="docked-vehicle" x="-25" y="-4" width="50" height="12" fill="#df8a2a"/>',
                  '<path id="module-front" d="M-32,2 L-20,2 L-20,16 L-32,16 Z M20,2 L32,2 L32,16 L20,16 Z" fill="#67818a"/>',
                  '<text x="-65" y="72">Craft behind front</text></g>'])
    # Native state poses keep the same ground anchor while geometry/status changes; they are templates, not authored art.
    for index, state in enumerate(("construction", "working", "blocked", "depleted")):
        x = 80 + index * 120
        color = "#b6c0c8" if state != "depleted" else "#7b858d"
        parts.extend([f'<g id="mine-state-{state}" transform="translate({x},440)">',
                      '<rect x="-31" y="-33" width="64" height="64" fill="none" stroke="#789"/>',
                      f'<path d="M-24,-20 H24 V16 H-24 Z" fill="{color}" stroke="#456"/>',
                      '<path d="M-3,0 H3 M0,-3 V3" stroke="#000"/>',
                      f'<text x="-40" y="52">{state}</text></g>'])
        # Add distinct native-size state cues while preserving the same canvas/ground anchor.
        if state == "working":
            parts.append(f'<circle cx="{x+18}" cy="426" r="4" fill="#df8a2a"/>')
        elif state == "blocked":
            parts.append(f'<path d="M{x-6},434 l12,12 M{x+6},434 l-12,12" stroke="#a33" stroke-width="2"/>')
        elif state == "construction":
            parts.append(f'<path d="M{x-22},422 l44,30 M{x-22},432 l32,20" stroke="#eff2f3"/>')
    parts.extend(['<g id="rocket-recovery-pose" transform="translate(650,440)">',
                  '<rect x="-32" y="-32" width="64" height="48" fill="none" stroke="#789"/>',
                  '<path d="M0,-28 L12,-12 L12,12 L-12,12 L-12,-12 Z" fill="#aebac4" stroke="#456"/>',
                  '<path d="M-20,14 H20" stroke="#456"/><text x="-40" y="52">Recovery</text></g>',
                  '<g id="module-vehicle-in-front" transform="translate(835,440)">',
                  '<path d="M-32,-28 H32 V16 H-32 Z" fill="#aebac4" stroke="#456"/>',
                  '<path d="M-32,2 H-20 V16 H-32 Z M20,2 H32 V16 H20 Z" fill="#67818a"/>',
                  '<rect x="-25" y="-4" width="50" height="12" fill="#df8a2a"/>',
                  '<text x="-65" y="52">Craft in front</text></g>',
                  '<text x="20" y="580">Per-tile building canvas 64x64, anchor (-31,-33); ground canvas 64x31, anchor (-31,0)</text>',
                  '<text x="20" y="605">Template colors communicate geometry only. Runtime swept-consist/animation/occlusion acceptance is pending.</text>',
                  '</g></svg>'])
    path = ASSETS / "multilayer_geometry_templates.svg"
    path.write_text("\n".join(parts), encoding="utf-8")
    return path


def main():
    """Generate or validate the producer artifacts and meaningful schema refusal cases.

    @return None; print concise evidence and fail on malformed contracts.
    """
    parser = argparse.ArgumentParser()
    parser.add_argument("--generate", action="store_true")
    args = parser.parse_args()
    if args.generate:
        document = build_manifest()
        MANIFEST.write_text(json.dumps(document, indent=2) + "\n", encoding="utf-8")
        write_templates()
    document = json.loads(MANIFEST.read_text(encoding="utf-8"))
    count = validate_manifest(document)
    # Mutated independent inputs ensure the validator refuses missing views, duplicates and fictitious final-source claims.
    for mutation in ("duplicate", "directions", "source", "tube_axis", "tube_front", "tube_corner", "tube_curve_front", "tube_junction_layout", "tube_junction_rotation", "tube_end_facing", "tube_end_front", "tube_crossing_rotation", "tube_crossing_front", "tube_conduit_branch", "tube_conduit_ground", "vent_shortage", "vent_hidden"):
        broken = json.loads(json.dumps(document))
        if mutation == "duplicate":
            broken["assets"].append(broken["assets"][0])
        elif mutation == "directions":
            next(row for row in broken["assets"] if row["kind"] == "vehicle")["directions"].pop()
        elif mutation == "source":
            broken["assets"][0]["source"]["path"] = "missing_final_sprite.png"
        elif mutation == "tube_axis":
            next(row for row in broken["assets"] if row["asset_id"] == "tube.straight")["directions"] = DOORS
        elif mutation == "tube_front":
            next(row for row in broken["assets"] if row["asset_id"] == "tube.straight")["parts"] = ["body"]
        elif mutation == "tube_corner":
            next(row for row in broken["assets"] if row["asset_id"] == "tube.bend45")["directions"] = DOORS
        elif mutation == "tube_curve_front":
            next(row for row in broken["assets"] if row["asset_id"] == "tube.bend45")["parts"] = ["body"]
        elif mutation == "tube_junction_layout":
            next(row for row in broken["assets"] if row["asset_id"] == "tube.junction")["native_layouts"].pop()
        elif mutation == "tube_junction_rotation":
            next(row for row in broken["assets"] if row["asset_id"] == "tube.junction")["directions"] = DOORS
        elif mutation == "tube_end_facing":
            next(row for row in broken["assets"] if row["asset_id"] == "tube.end")["directions"] = ["NE", "SE"]
        elif mutation == "tube_end_front":
            next(row for row in broken["assets"] if row["asset_id"] == "tube.end")["parts"] = ["body"]
        elif mutation == "tube_crossing_rotation":
            next(row for row in broken["assets"] if row["asset_id"] == "tube.crossing")["directions"] = DOORS
        elif mutation == "tube_crossing_front":
            next(row for row in broken["assets"] if row["asset_id"] == "tube.crossing")["parts"] = ["body"]
        elif mutation == "tube_conduit_branch":
            next(row for row in broken["assets"] if row["asset_id"] == "tube.conduit")["directions"] = ["NE", "SE"]
        elif mutation == "vent_shortage":
            next(row for row in broken["assets"] if row["asset_id"] == "geothermal.vent")["states"].append("heat_starved")
        elif mutation == "vent_hidden":
            next(row for row in broken["assets"] if row["asset_id"] == "geothermal.vent")["hidden_policy"] = "visible_unknown_badge"
        else:
            next(row for row in broken["assets"] if row["asset_id"] == "tube.conduit")["parts"] = ["back", "front"]
        try:
            validate_manifest(broken)
        except ValueError:
            continue
        raise AssertionError("invalid manifest accepted")
    tree = ET.parse(ASSETS / "multilayer_geometry_templates.svg")
    ids = {node.attrib["id"] for node in tree.iter() if "id" in node.attrib}
    assert {f"tbm-{name}" for name in DIRECTIONS}.issubset(ids)
    assert {"mine-3x3", "rocket-deploy-recover", "rocket-recovery-pose", "module-back", "docked-vehicle", "module-front", "module-vehicle-in-front"}.issubset(ids)
    assert {f"mine-state-{state}" for state in ("construction", "working", "blocked", "depleted")}.issubset(ids)
    print(f"M0 sprite contract: {len(document['assets'])} families, {count} required sprites; schema/refusal/template checks passed; final art pending")


if __name__ == "__main__":
    main()
