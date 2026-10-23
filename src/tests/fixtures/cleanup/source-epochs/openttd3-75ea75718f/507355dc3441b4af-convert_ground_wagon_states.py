"""Assemble complete ordinary wagon sources from immutable authored components.

@file convert_ground_wagon_states.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group20E
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group20B shared own-model purchase conversion
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group21C shared original material/purchase conversion
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group22 shared own-model purchase conversion
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md
Native Train packets, movement, depot occupancy and reservation holds own state.
This source handoff allocates no packed range and activates no native renderer.
"""

import argparse
import hashlib
import io
import json

from PIL import Image, ImageDraw

import convert_control_art as controls
import convert_ground_freight_objects as objects
from convert_terrain_art import png_bytes


fleet = objects.fleet
STATES = ("empty", "partial", "full", "service", "blocked")
REPORT = fleet.OUTPUT / "vehicle-model-05-source-manifest.json"


def signature(value):
    """Identify approved physical registrations independently of review flags.

    @param value Explicit body/component source geometry to freeze.
    @return SHA256 of stable JSON; no gameplay or publication state is changed.
    """
    return hashlib.sha256(json.dumps(value, sort_keys=True).encode()).hexdigest()


def checked_geometry(document, public):
    """Require complete own-model component registrations and real state owners.

    @param document Retained conventional fleet source record.
    @param public Shared native sprite contract, optionally an isolated fixture.
    @return Approved geometry and selected original component registrations.
    """
    spec = fleet.public_family("vehicle.model-05", public)
    family = document["families"]["vehicle.model-05"]
    geometry, components = family["state_component_geometry"], family["state_components"]
    if spec["company_colors"] != "explicit_mask":
        raise ValueError("ordinary wagon explicit company mask is unavailable")
    if family["native_cargo_labels"] != spec["native_cargo_labels"]:
        raise ValueError("ordinary wagon source label mapping differs from native capability")
    if signature(geometry) != family["approved_state_component_geometry_sha256"]:
        raise ValueError("ordinary wagon state geometry changed after approval")
    if signature(components) != family["approved_state_components_sha256"]:
        raise ValueError("ordinary wagon component ancestry changed after approval")
    # Original body geometry and native packet/state policy govern every cue;
    # a plausible lamp may not reinterpret a portal magazine or loaded wagon.
    if (geometry["body_registrations"] != family["approved_base_registration"]
            or geometry["state_owner"] != spec["native_state_policy"]
            or components["geometry_sha256"] != signature(geometry)
            or geometry["native_canvas"] != [64, 48] or geometry["anchor"] != [-32, -32]):
        raise ValueError("ordinary wagon component does not belong to the fixed body")
    required_lenses = {state + "/" + str(frame) for state in ("service", "blocked") for frame in range(2)}
    if set(components["gear"]) != set(fleet.DIRECTIONS):
        raise ValueError("ordinary wagon lacks a genuine gear heading")
    if set(components["lenses"]) != required_lenses:
        raise ValueError("ordinary wagon lacks an authored status phase")
    return geometry, components


def original_component(selection, kind, view, document, cache):
    """Extract a complete approved source rectangle before native component use.

    @param selection Exact original key, heading crop and measured alpha bounds.
    @param kind Required own-model running_gear or status_lens source kind.
    @param view Genuine heading or explicit status/frame identity.
    @param document Retained source record; no external private owner is accepted.
    @param cache Original RGBA images cached by immutable source path.
    @return Whole original subject and its selected provenance row.
    """
    candidates = [row for row in document["candidates"] if row["key"] == selection["key"]]
    if len(candidates) != 1:
        raise ValueError("missing or ambiguous wagon state original")
    candidate = candidates[0]
    # A foreign vehicle or rejected generated sheet cannot supply convincing
    # pixels to this wagon; both physical kind and actual view stay explicit.
    if (candidate["asset_id"] != "vehicle.model-05" or candidate["component_kind"] != kind
            or not candidate["status"].startswith("selected_")):
        raise ValueError("foreign or rejected wagon state component")
    expected = {key: value for key, value in selection.items() if key != "key"}
    if candidate.get("component_views", {}).get(view) != expected:
        raise ValueError("wagon state component has the wrong source heading")
    # Original and reference ancestry are checked before cache lookup. Paths
    # resolve only inside this repository, never the tool's private work cache.
    for identity in [candidate] + candidate["references"]:
        if fleet.base.digest(fleet.base.repository_path(identity["path"])) != identity["sha256"]:
            raise ValueError("immutable wagon state original/reference changed")
    path = candidate["path"]
    if path not in cache:
        cache[path] = Image.open(fleet.base.repository_path(path)).convert("RGBA")
    original = cache[path]
    left, top, right, bottom = selection["source_window"]
    if not (0 <= left < right <= original.width and 0 <= top < bottom <= original.height):
        raise ValueError("wagon state crop leaves its original")
    crop = original.crop((left, top, right, bottom))
    bounds = crop.getchannel("A").point(lambda alpha: 255 if alpha >= 16 else 0).getbbox()
    if bounds is None:
        raise ValueError("empty wagon state component")
    if list(bounds) != selection["whole_subject_bounds_in_window"]:
        raise ValueError("wagon state whole subject bounds changed")
    clearance = min(bounds[0], bounds[1], crop.width - bounds[2], crop.height - bounds[3])
    # Whole visible subjects retain safe margins before choosing a wheel/lens
    # interior. A clipped original or separator cannot be repaired downstream.
    if selection["alpha_threshold"] != 16 or selection["minimum_clearance"] != 16 or clearance < 16:
        raise ValueError("wagon state whole subject clearance changed")
    if any(alpha >= 16 and red > 220 and green < 60 and blue > 220
           for red, green, blue, alpha in crop.get_flattened_data()):
        raise ValueError("opaque separator enters wagon state source")
    return crop.crop(bounds), candidate


def state_sources(document, geometry, components):
    """Convert all eight authored gear views and four original glass interiors.

    @param document Retained own-model immutable original registrations.
    @param geometry Approved fixed native axle/status/company attachments.
    @param components Explicit selected source views and isotropic glass crops.
    @return Gear image/evidence and status index/evidence dictionaries.
    """
    gears, lenses, cache = {}, {}, {}
    # Every camera has genuine original axle pixels. No mirrored wheel donor
    # or duplicated missing heading can pad the complete eight-way matrix.
    for direction in fleet.DIRECTIONS:
        selection = components["gear"][direction]
        subject, candidate = original_component(selection, "running_gear", direction, document, cache)
        native = fleet.base.fit(subject, (64, 48), fleet.ENVELOPES[direction])
        indexed = fleet.base.quantize(native, {})
        for x, y, width, height in geometry["gear_regions"][direction]:
            if any(native.getpixel((px, py))[3] < 128 for py in range(y, y + height) for px in range(x, x + width)):
                raise ValueError("authored wheel interior is not opaque")
        gears[direction] = indexed, {"source": candidate["path"], "source_sha256": candidate["sha256"],
                                     "source_view": direction, "selection": selection}
    # Glass interiors are explicitly nondirectional source components. Their
    # complete original squares are verified before sampling the opaque centre;
    # changing brightness requires another authored phase, never palette cycling.
    for identity, selection in components["lenses"].items():
        _subject, candidate = original_component(selection, "status_lens", identity, document, cache)
        left, top, right, bottom = selection["interior_crop"]
        window = selection["source_window"]
        if not (window[0] <= left < right <= window[2] and window[1] <= top < bottom <= window[3]):
            raise ValueError("wagon status interior leaves its selected view")
        original = cache[candidate["path"]].crop((left, top, right, bottom))
        if original.getchannel("A").getextrema()[0] < 128:
            raise ValueError("wagon status interior includes transparent padding")
        pixel = fleet.base.quantize(original.resize((1, 1), Image.Resampling.BOX), {}).getpixel((0, 0))
        lenses[identity] = pixel, {"source": candidate["path"], "source_sha256": candidate["sha256"],
                                   "source_state_frame": identity, "selection": selection}
    if len({row[0] for row in lenses.values()}) != 4:
        raise ValueError("authored wagon status phases collapse at native palette scale")
    return gears, lenses


def apply_components(body, direction, state, frame, geometry, gears, lenses):
    """Place only approved livery, rotating axle and actual empty-status pixels.

    @param body Exact selected body/load pose with original foreground retained.
    @param direction Actual native engine heading.
    @param state Packet-backed load or actual empty service/reservation hold.
    @param frame Authored zero/one visual phase; runtime movement owns selection.
    @param geometry Fixed opaque original body attachment regions.
    @param gears Genuine heading-specific alternate wheel source pixels.
    @param lenses Four independently authored original status-glass samples.
    @return Indexed pose and its exact company/gear/status component evidence.
    """
    # Unknown presentation identities cannot inherit a plausible donor and
    # conceal an absent state, heading or independently authored phase.
    if direction not in fleet.DIRECTIONS or state not in STATES or frame not in (0, 1):
        raise ValueError("unknown ordinary wagon state/view/frame")
    image = body.copy()
    company = []
    x, y, width, height = geometry["company_regions"][direction]
    # Explicit company livery maps only the original painted panel's selected
    # pixels. Steel, cargo, wheel highlights and status lenses never inherit
    # company colours from unrestricted nearest-palette quantization.
    for py in range(y, y + height):
        for px in range(x, x + width):
            original = body.getpixel((px, py))
            if original == 0:
                raise ValueError("company fitting leaves original wagon opacity")
            rgb = fleet.base.PALETTE[original]
            shade = 198 + max(0, min(7, round(sum(rgb) / (3 * 255) * 7)))
            image.putpixel((px, py), shade)
            company.append({"target": [px, py], "original_index": original, "palette_index": shade})
    evidence = {"company_mask": company, "gear": None, "status": None}
    # Only the rolling frame uses the authored alternate axles. Native
    # speed/stopping selects this frame later; loaded wagons keep their
    # actual packet presentation while motion changes the wheel interiors.
    if state in ("empty", "partial", "full") and frame == 1:
        donor, source = gears[direction]
        changes = 0
        for x, y, width, height in geometry["gear_regions"][direction]:
            for py in range(y, y + height):
                for px in range(x, x + width):
                    selected = donor.getpixel((px, py))
                    if body.getpixel((px, py)) == 0 or selected == 0 or py >= 39:
                        raise ValueError("rolling cue changes wagon alpha or ground contact")
                    changes += selected != body.getpixel((px, py))
                    image.putpixel((px, py), selected)
        if changes == 0:
            raise ValueError("authored wheel phase is a duplicate native frame")
        evidence["gear"] = {**source, "regions": geometry["gear_regions"][direction]}
    if state in ("service", "blocked"):
        # Empty depot/reservation cues keep original stopped axles in both
        # phases. Packet-backed load states never lose cargo to a status lamp.
        selected, source = lenses[state + "/" + str(frame)]
        x, y, width, height = geometry["status_regions"][direction]
        for py in range(y, y + height):
            for px in range(x, x + width):
                if body.getpixel((px, py)) == 0:
                    raise ValueError("status attachment leaves original wagon opacity")
                image.putpixel((px, py), selected)
        evidence["status"] = {**source, "region": geometry["status_regions"][direction]}
    image.info["transparency"] = 0
    return image, evidence


def purchase_source(body, components, public, asset_id="purchase.model-05"):
    """Reuse the selected own-model preview and genuine calendar-lock original.

    @param body Selected unmodified native E body, retaining its original livery.
    @param components Approved own-model preview and existing lock registration.
    @param public Shared native purchase family geometry and availability states.
    @param asset_id Actual05/06 wagon or07/08 road purchase; default preserves05.
    @return Complete two-pose indexed UI atlas and original source evidence.
    """
    if asset_id not in ("purchase.model-05", "purchase.model-06", "purchase.model-07", "purchase.model-08"):
        raise ValueError("unsupported conventional wagon purchase source")
    matches = [row for row in public["assets"] if row["asset_id"] == asset_id]
    if len(matches) != 1:
        raise ValueError("ordinary wagon purchase family is unavailable")
    spec, source = matches[0], components["purchase"]
    # A world sprite cannot replace the20x20 purchase control, and the existing
    # calendar-lock owner must remain the only availability presentation source.
    if (spec["native_size"] != [20, 20] or spec["anchor"] != [0, 0]
            or spec["states"] != ["available", "locked"] or spec["frames"] != 1
            or spec["directions"] != ["none"] or spec["company_colors"] != "none"
            or source["body_direction"] != "E" or source["maximum_subject_size"] != [18, 6]
            or source["lock_record"] != controls.RECORD.relative_to(fleet.base.ROOT).as_posix()
            or source["lock_family"] != "control.future-lock" or source["lock_target"] != [12, 13, 7, 7]):
        raise ValueError("ordinary wagon purchase geometry or original lock changed")
    report, files = controls.derive()
    lock = next(row for row in report["atlases"] if row["asset_id"] == "control.future-lock")
    glyph = Image.open(io.BytesIO(files[fleet.base.repository_path(lock["path"])])).crop((0, 0, 20, 20)).convert("RGBA")
    badge = glyph.crop(glyph.getchannel("A").getbbox()).resize((7, 7), Image.Resampling.BOX)
    original = body.convert("RGBA")
    available = fleet.base.fit(original.crop(original.getchannel("A").getbbox()), (20, 20), (18, 6))
    locked = available.copy()
    locked.alpha_composite(badge, (12, 13))
    atlas = Image.new("P", (40, 20), 0)
    atlas.putpalette(fleet.base.PALETTE_BYTES)
    atlas.info["transparency"] = 0
    source_record = json.loads(controls.RECORD.read_text(encoding="utf-8"))["source"]
    sprites = []
    # Both purchase presentations retain the complete same wagon above a
    # separate UI badge. Availability artwork creates no date/capability rule.
    for index, (state, image) in enumerate((("available", available), ("locked", locked))):
        atlas.paste(fleet.base.quantize(image, {}), (20 * index, 0))
        sprites.append({"state": state, "frame": 0, "direction": "none", "tile_variant": 0,
                        "part": "body", "anchor": [0, 0], "box": [20 * index, 0, 20, 20],
                        "lock_component": None if index == 0 else {"source": source_record["path"],
                        "source_sha256": source_record["sha256"], "target": source["lock_target"],
                        "source_record": source["lock_record"], "family": source["lock_family"]}})
    return atlas, sprites


def build(document=None, public=None):
    """Assemble exactly1,360 body and two purchase poses with explicit ancestry.

    @param document Optional isolated retained fleet record for source refusals.
    @param public Optional isolated shared body/purchase geometry contract.
    @return Complete source manifest and exact proof artifact bytes; no activation.
    """
    if document is None:
        document = json.loads(fleet.RECORD.read_text(encoding="utf-8"))
    if public is None:
        public = json.loads(fleet.base.PUBLIC.read_text(encoding="utf-8"))
    geometry, components = checked_geometry(document, public)
    family = document["families"]["vehicle.model-05"]
    gear, lenses = state_sources(document, geometry, components)
    object_report, files = objects.build(document, public)
    base_report = json.loads(files[fleet.OUTPUT / "vehicle-model-05-base-source-manifest.json"])
    bulk_report = json.loads(files[fleet.OUTPUT / "vehicle-model-05-bulk-source-manifest.json"])
    bodies, loads = {}, {}
    # Every camera begins with its actual fixed chassis. All later state/refit
    # aliases remain explicit native variant identities over those same bytes.
    base_atlas = Image.open(io.BytesIO(files[fleet.base.repository_path(base_report["path"])]))
    for row in base_report["sprites"]:
        x, y, width, height = row["box"]
        bodies[row["direction"]] = base_atlas.crop((x, y, x + width, y + height)), row
    for report in (bulk_report, object_report):
        atlas = Image.open(io.BytesIO(files[fleet.base.repository_path(report["path"])]))
        for row in report["sprites"]:
            x, y, width, height = row["box"]
            loads[row["cargo_label"], row["state"], row["direction"]] = atlas.crop((x, y, x + width, y + height)), row
    atlas = Image.new("P", (512, len(family["native_cargo_labels"]) * len(STATES) * 2 * 48), 0)
    atlas.putpalette(fleet.base.PALETTE_BYTES)
    atlas.info["transparency"] = 0
    sprites = []
    # The complete matrix preserves all actual cargo meanings. Empty/service/
    # blocked exteriors are declared visual aliases; partial/full always retain
    # the appropriate genuine selected cargo and original foreground geometry.
    for variant, label in enumerate(family["native_cargo_labels"]):
        for state_index, state in enumerate(STATES):
            for frame in range(2):
                for direction_index, direction in enumerate(fleet.DIRECTIONS):
                    body, ancestry = loads[label, state, direction] if state in ("partial", "full") else bodies[direction]
                    image, evidence = apply_components(body, direction, state, frame, geometry, gear, lenses)
                    x = direction_index * 64
                    y = ((variant * len(STATES) + state_index) * 2 + frame) * 48
                    atlas.paste(image, (x, y))
                    sprites.append({"state": state, "frame": frame, "direction": direction, "tile_variant": variant,
                                    "cargo_label": label, "part": "body", "anchor": [-32, -32], "box": [x, y, 64, 48],
                                    "body_source": bodies[direction][1], "load_source": ancestry if state in ("partial", "full") else None,
                                    "components": evidence, "visual_alias": None if state in ("partial", "full") else "all actual cargo labels share empty " + state})
    purchase, purchase_rows = purchase_source(bodies["E"][0], components, public)
    # Purchase art uses the exact selected E chassis, independently of its UI
    # lock. Retain that own-model ancestry beside both actual purchase states.
    for row in purchase_rows:
        row["body_source"] = bodies["E"][1]
    atlases = []
    # World and purchase consumers retain independent canvases/counts while
    # sharing only this model's explicit original source and palette owners.
    for asset_id, name, image, rows in (("vehicle.model-05", "vehicle-model-05-authored.png", atlas, sprites),
                                        ("purchase.model-05", "purchase-model-05-authored.png", purchase, purchase_rows)):
        path = fleet.OUTPUT / name
        encoded = png_bytes(image)
        files[path] = encoded
        atlases.append({"asset_id": asset_id, "path": path.relative_to(fleet.base.ROOT).as_posix(),
                        "sha256": hashlib.sha256(encoded).hexdigest(), "size": list(image.size), "sprites": rows})
    # One representative cargo page displays all actual motion/status phases;
    # earlier per-refit contacts retain every distinct partial/full material.
    for state in STATES:
        board = Image.new("RGB", (1048, 1120), (42, 46, 50))
        for frame in range(2):
            selected = [row for row in sprites if row["cargo_label"] == "COAL" and row["state"] == state and row["frame"] == frame]
            page = fleet.base_contact(atlas, selected, "Model05 " + state + " phase" + str(frame),
                                      caption="SOURCE ONLY; native1x/4x; " + state + " authored phase" + str(frame) + "; actual Train owns selection")
            board.paste(page, (0, frame * 560))
        stream = io.BytesIO()
        board.save(stream, format="PNG", optimize=False, compress_level=9)
        files[fleet.OUTPUT / ("vehicle-model-05-" + state + "-phases-contact.png")] = stream.getvalue()
    board = Image.new("RGB", (360, 160), (42, 46, 50))
    draw = ImageDraw.Draw(board)
    # Native and enlarged previews expose model readability and the separate
    # calendar badge without changing the UI sprite's actual20x20 dimensions.
    for index, row in enumerate(purchase_rows):
        x, y, width, height = row["box"]
        image = purchase.crop((x, y, x + width, y + height)).convert("RGBA")
        draw.text((12 + index * 180, 12), row["state"] + " native1x/4x", fill="white")
        board.paste(image, (20 + index * 180, 40), image)
        large = image.resize((80, 80), Image.Resampling.NEAREST)
        board.paste(large, (70 + index * 180, 40), large)
    stream = io.BytesIO()
    board.save(stream, format="PNG", optimize=False, compress_level=9)
    files[fleet.OUTPUT / "purchase-model-05-contact.png"] = stream.getvalue()
    report = {"_documentation": "Owner: r2mlvs14-remaining-assets-and-runtime-integration.md Group20E; complete wagon source, runtime pending",
              "schema_version": 1, "source_record": fleet.RECORD.relative_to(fleet.base.ROOT).as_posix(),
              "body_pose_count": len(sprites), "purchase_pose_count": len(purchase_rows),
              "source_coverage_complete": True, "packed": False, "runtime_activated": False,
              "geometry_sha256": signature(geometry), "components_sha256": signature(components),
              "native_state_policy": family["native_state_policy"], "atlases": atlases}
    files[REPORT] = (json.dumps(report, indent=2) + "\n").encode()
    return report, files


def publish_source(document, public, report, files):
    """Bind verified complete originals through the existing shared source owner.

    @param document Current retained fleet source/component registrations.
    @param public Current shared manifest; all existing packed fields are retained.
    @param report Fresh complete body/purchase manifest from build().
    @param files Fresh exact source artifact bytes from the same build().
    @return None; refuse stale proof before writing either source-family binding.
    """
    # Source publication must consume every current verified artifact together;
    # absent/stale contacts or atlases cannot inherit a metadata-only approval.
    for path, encoded in files.items():
        if path.read_bytes() != encoded:
            raise ValueError("ordinary wagon source publication requires exact artifacts")
    # Complete source publication requires both actual consumers together;
    # a partial world matrix cannot borrow complete purchase metadata.
    if report["body_pose_count"] != 1360 or report["purchase_pose_count"] != 2 or not report["source_coverage_complete"]:
        raise ValueError("ordinary wagon source publication requires complete coverage")
    family = document["families"]["vehicle.model-05"]
    keys = set(family["selections"].values())
    keys.update(row["source"] for row in family["approved_bulk_materials"].values())
    keys.update(row["key"] for row in family["state_components"]["gear"].values())
    keys.update(row["key"] for row in family["state_components"]["lenses"].values())
    originals = [row["path"] for row in document["candidates"] if row["key"] in keys]
    # Selected exterior aliases share physical source ancestry only. Each
    # actual cargo label keeps its separate source-matrix/native variant slot.
    for component in family["oriented_components"].values():
        originals.extend(row["source_path"] for row in component["registration"]["views"].values())
    originals.append(report["atlases"][1]["sprites"][1]["lock_component"]["source"])
    originals = list(dict.fromkeys(originals))
    # Only the complete model05 world/UI source metadata changes; this does not
    # allocate a range, change gameplay capability or activate native drawing.
    for atlas in report["atlases"]:
        spec = next(row for row in public["assets"] if row["asset_id"] == atlas["asset_id"])
        source = atlas["sprites"][0]["body_source"]
        spec["source"] = {"status": "authored", "path": source["source_path"], "sha256": source["source_sha256"],
                          "provenance": "first_party_original_model_and_component_assembly", "rights": "first_party_generated_original"}
        spec["authoring"] = {"date": "2026-10-11", "status": "complete_native_original_wagon_source_runtime_pending",
                             "source_record": report["source_record"], "originals": originals,
                             "atlas_manifest": REPORT.relative_to(fleet.base.ROOT).as_posix(), "native_atlas": atlas["path"],
                             "native_sha256": atlas["sha256"], "staged_pose_count": len(atlas["sprites"]),
                             "coverage_complete": True, "runtime_activated": False}
    fleet.base.PUBLIC.write_text(json.dumps(public, indent=2) + "\n", encoding="utf-8")


def main():
    """Publish or verify complete source proofs without claiming native delivery.

    @return None; print exact coverage or refuse any changed source artifact.
    """
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    modes = parser.add_mutually_exclusive_group()
    modes.add_argument("--publish", action="store_true", help="write reproducible body/purchase source proofs only")
    modes.add_argument("--publish-source", action="store_true", help="bind complete source only after exact proof verification")
    args = parser.parse_args()
    document = json.loads(fleet.RECORD.read_text(encoding="utf-8"))
    public = json.loads(fleet.base.PUBLIC.read_text(encoding="utf-8"))
    report, files = build(document, public)
    # Only repository-local ignored proof artifacts are writable here. The
    # shared manifest and shipped payload require their later explicit owners.
    for path, encoded in files.items():
        if args.publish:
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_bytes(encoded)
        if path.read_bytes() != encoded:
            raise ValueError("complete wagon source artifact differs from retained originals")
    if args.publish_source:
        publish_source(document, public, report, files)
    print(f"Wagon sources: {report['body_pose_count']} body + {report['purchase_pose_count']} purchase poses; {len(files)} exact artifacts; packing/runtime pending")


if __name__ == "__main__":
    main()
