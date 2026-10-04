#!/usr/bin/env python3
"""M0 manifest/schema/placement fixtures; no final raster art is generated.

@file multilayer_sprite_contract.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs13-graphics-and-asset-pipeline.md
"""

import argparse
import hashlib
import json
from pathlib import Path
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parents[2]
ASSETS = ROOT / "docs/zz-reach2/upgrades/2026-10/assets"
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
    @param footprint World tile dimensions; moving vehicles use no static tile footprint.
    @param directions Explicit engine views; defaults depend on placement kind.
    @return One manifest record with pending source/packing status.
    """
    sizes = {"icon": (20, 20, 0, 0), "terrain": (64, 31, -31, 0),
             "vehicle": (64, 48, -32, -32), "building": (64, 64, -31, -33)}
    width, height, x, y = sizes[kind]
    views = directions if directions is not None else DIRECTIONS if kind == "vehicle" else DOORS if kind == "building" else ["none"]
    parts = ["back", "front"] if kind == "building" else ["body"]
    return {"asset_id": asset_id, "consumer": consumer, "kind": kind,
            "directions": views, "frames": frames, "states": states,
            "native_size": [width, height], "footprint": list(footprint),
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
    # The nineteen model IDs match the public content catalogue; purchase previews are separate explicit assets.
    for model in range(19):
        owner = 4 if model < 2 else 5 if model < 5 else 7 if model < 9 else 16 if model < 14 else 9 if model in (14, 18) else 10 if model == 15 else 11
        states = ["empty", "partial", "full", "service", "blocked"] if model not in (0, 1, 4, 16) else ["idle", "moving", "working", "blocked"]
        space_states = {14: ["ready", "preparing", "ascent", "orbit", "recovery", "blocked", "service"],
                        15: ["free", "rendezvous", "attached", "mining", "full", "return", "service", "blocked"],
                        16: ["idle", "moving", "working", "service", "blocked"],
                        17: ["empty", "partial", "full", "moving", "service", "blocked"],
                        18: ["packed", "descending", "recovered", "blocked"]}
        states = space_states.get(model, states)
        rows.append(record(f"vehicle.model-{model:02}", owner, "vehicle", states, 4 if model < 2 else 2, (0, 0)))
        rows.append(record(f"purchase.model-{model:02}", owner, "icon", ["available", "locked"]))
    # Each facility has an explicit footprint and back/front pair; local-ID packing belongs to its feature owner.
    facilities = [("portal.sealed", 3, (2, 1)), ("portal.deep-rail", 3, (2, 1)),
                  ("exchange", 5, (3, 2)), ("rail-station", 7, (3, 2)), ("road-terminal", 7, (2, 2)),
                  ("rail-service", 7, (3, 2)), ("road-service", 7, (2, 2)),
                  ("earth-receiver", 6, (3, 3)), ("rock-receiver", 6, (3, 3)),
                  ("metal-refinery", 6, (4, 4)), ("chemical-works", 6, (4, 4)),
                  ("battery-works", 6, (4, 4)), ("electronics-works", 6, (4, 4)),
                  ("component-works", 6, (4, 4)), ("robotics-works", 6, (4, 4)),
                  ("mirror-kit-works", 6, (4, 4)), ("solar-farm", 12, (3, 2)), ("nuclear-plant", 12, (4, 4)),
                  ("tube-terminal", 16, (2, 2)), ("tube-service", 16, (3, 2)), ("tube-interchange", 16, (3, 2)),
                  ("vent-collector", 17, (3, 3)), ("vent-powerhouse", 17, (4, 4)),
                  ("magma-powerhouse", 17, (4, 4)), ("heat-smelter", 17, (4, 4)),
                  ("heat-battery-works", 17, (4, 4)), ("mantle-forge", 17, (4, 4)),
                  ("utility-riser", 17, (2, 2)), ("launch-center", 9, (4, 4))]
    for name, owner, footprint in facilities:
        states = ["construction", "ready", "working", "blocked"]
        if owner == 17:
            states.append("heat_starved")
        rows.append(record(f"facility.{name}", owner, "building", states, 4, footprint))
    for shape in ("straight", "bend45", "junction", "end", "crossing", "conduit"):
        rows.append(record(f"tube.{shape}", 16, "terrain", ["construction", "powered", "unpowered"], directions=DOORS))
    rows.append(record("geothermal.vent", 17, "terrain", ["hidden", "surveyed", "tapped", "heat_starved"], 4))
    rows.append(record("geothermal.conduit", 17, "terrain", ["construction", "connected", "unconnected"], directions=DOORS))
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
    sheet = ROOT / "media/baseset/openttd/map_layers.png"
    next_offset = 74
    # Core overlays get an explicit future append range; current 0..65 and optional 66..73 are never occupied by this draft.
    for row in rows:
        if row["asset_id"].startswith(("terrain.", "survey.", "control.", "overlay.")):
            row["packed"]["namespace"] = "native_map_layer_extension"
            row["packed"]["offset"] = next_offset
            next_offset += len(row["directions"]) * row["frames"] * len(row["states"]) * len(row["parts"]) * row["tile_variants"]
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
        width, height = row["native_size"]
        if width <= 0 or height <= 0:
            # Empty canvases cannot provide native-size construction/purchase feedback.
            raise ValueError("empty native canvas")
        if row["bounds"] != [0, 0, width, height]:
            raise ValueError("bounds disagree with native canvas")
        if row["palette"] != "dos_8bpp" or row["transparent_index"] != 0:
            # Every first-deliverable family shares the same palette/transparency contract.
            raise ValueError("unsupported palette")
        if row["kind"] == "building" and row["directions"] != DOORS:
            # Per-tile buildings need four real approach views and coherent foreground cuts.
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
    for mutation in ("duplicate", "directions", "source"):
        broken = json.loads(json.dumps(document))
        if mutation == "duplicate":
            broken["assets"].append(broken["assets"][0])
        elif mutation == "directions":
            next(row for row in broken["assets"] if row["kind"] == "vehicle")["directions"].pop()
        else:
            broken["assets"][0]["source"]["path"] = "missing_final_sprite.png"
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
