"""Assemble complete native terminal sources with actual preview/loading ownership.

@file convert_terminal_states.py
@see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup3.md Group17DD, immutable source-era reconstruction.
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md task25.3.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Atomic build/refusal poses remain previews. Installed loading reads the native
RoadVehicle owner; original glass phases never create cargo or simulation time.
"""

import argparse
import hashlib
import io
import json

from PIL import Image, ImageDraw, PngImagePlugin

import convert_facility_art as facilities
import convert_terminal_views as views
from tools.ground_terminal_contract import source_digest


terminal = views.terminal
OUTPUT = terminal.OUTPUT
PREFIX = "terminal-source"
STATES = ("construction", "ready", "working", "blocked")


def checked_components(record, public, source_only=False):
    """Require approved visible fittings, native state owners and original material ancestry.

    @param record Terminal original and state-component registrations.
    @param public Authoritative shared facility.road-terminal source contract.
    @param source_only Require the exact original record; grants no current native qualification.
    @return Approved fixed geometry and immutable status-glass donor description.
    @throws ValueError Before publication on changed owners, fittings or packed boundaries.
    """
    terminal.validate_terminal_contract(public)
    family = record["families"]["facility.road-terminal"]
    geometry, component = family["state_component_geometry"], family["state_component_source"]
    # Explicit original registrations and feature-owned state govern every
    # cue. A changed owner or already packed family requires its own review;
    # source publication cannot silently reinterpret installed native behavior.
    if (terminal.signature(geometry) != family["approved_state_component_geometry_sha256"]
            or terminal.signature(component) != family["approved_state_component_source_sha256"]
            or component["geometry_sha256"] != terminal.signature(geometry)
            or geometry["state_owner"] != public["native_state_policy"]
            or geometry["phase_aliases"] != public["native_state_policy"]["frame_aliases"]
            or geometry["native_canvas"] != [64, 64] or geometry["anchor"] != [-31, -33]
            or set(geometry["cue_targets"]) != {str(graphics) for graphics in range(6)}
            or public["packed"]["offset"] is not None):
        raise ValueError("terminal state geometry, ownership or unpacked boundary changed")
    # Hidden -X/-Y fronts retain their real original rear walls in every phase.
    # A conveniently visible replacement lamp would change the selected model.
    for graphics in range(6):
        target = geometry["cue_targets"][str(graphics)]
        if (target is not None) != views.front_visible(graphics):
            raise ValueError("terminal status glass changes genuine facade visibility")
        if target is not None:
            expected = "parent-0" if graphics == 4 else "parent-1"
            if target["part"] != expected or len(target["points"]) != 1:
                raise ValueError("terminal status glass changes its actual housing parent")
    if source_only:
        from station_source_facts import verify_original
        verify_original(record, "record", asset="facility.road-terminal")
    else:
        # New native presentation still requires its actual current owners;
        # historical physical art is a separate reconstruction role.
        for name, digest in geometry["native_presentation_owner_sha256"].items():
            if source_digest(terminal.ROOT / name) != digest:
                raise ValueError("terminal native preview/loading presentation owner changed")
    if component["source_record"] != facilities.RECORD.relative_to(terminal.ROOT).as_posix() or component["source_key"] != "facility-status":
        raise ValueError("terminal status glass lacks its approved original donor")
    return geometry, component


def original_lenses(component):
    """Reduce retained original glass to the actual one-pixel native equipment fitting.

    @param component Approved immutable facility-status original path/hash registration.
    @return Four state-indexed lists of four opaque original1x1 material phases.
    @throws ValueError On changed ancestry or collapsed authored working phases.
    """
    donor = json.loads(facilities.RECORD.read_text(encoding="utf-8"))
    sources = [row for row in donor["components"] if row["key"] == component["source_key"]]
    if len(sources) != 1:
        raise ValueError("terminal status original is missing or ambiguous")
    source = sources[0]
    # Similar colors from another sheet cannot inherit the actual first-party
    # glass provenance. The existing facility converter verifies original bytes.
    if source["path"] != component["path"] or source["sha256"] != component["sha256"]:
        raise ValueError("terminal status glass original ancestry changed")
    originals = facilities.indicator(source)
    result = {}
    # Only authored glass interiors are BOX sampled. No hue shift, added glow
    # or software-painted placeholder supplies a missing state/working phase.
    for state, phases in zip(STATES, originals):
        result[state] = [terminal.base.quantize(image.resize((1, 1), Image.Resampling.BOX), {}) for image in phases]
        if any(image.getpixel((0, 0)) == 0 for image in result[state]):
            raise ValueError("terminal original status glass loses its opaque fitting")
    if len({image.tobytes() for image in result["working"]}) != 4:
        raise ValueError("terminal native loading glass phases collapse after palette conversion")
    return result


def apply_lens(original, lens, target):
    """Change only the existing amber equipment-glass pixel without enlarging its fitting.

    @param original Indexed64x64 approved genuine front housing.
    @param lens Indexed1x1 original state/phase glass sample.
    @param target Exact approved housing parent and single native glass coordinate.
    @return Indexed parent with original alpha, contacts and all other material exact.
    @throws ValueError On a moved, transparent, enlarged or non-glass fitting.
    """
    if len(target["points"]) != 1 or lens.size != (1, 1):
        raise ValueError("terminal status glass fitting is missing or enlarged")
    x, y = target["points"][0]
    # The selected SE fitting occupies one actual amber pixel. Extending the
    # lens onto adjacent wall steel would invent geometry to improve a preview.
    if not (0 <= x < 64 and 0 <= y < 64) or lens.getpixel((0, 0)) == 0:
        raise ValueError("terminal status glass leaves its opaque native fitting")
    value = original.getpixel((x, y))
    red, green, blue = original.getpalette()[3 * value:3 * value + 3]
    if value == 0 or red < 140 or green < 60 or blue >= 80:
        raise ValueError("terminal status glass must remain inside original amber equipment glass")
    pose = original.copy()
    pose.putpixel((x, y), lens.getpixel((0, 0)))
    return pose


def base_sources(record):
    """Resolve six approved genuine native views while preserving their exact prior artifacts.

    @param record Immutable terminal source and native-component registrations.
    @return Compiled geometry, three indexed parents per view and prior artifact identities.
    """
    report, files = views.build(record)
    atlas = Image.open(io.BytesIO(files[OUTPUT / f"{views.PREFIX}-atlas.png"]))
    sources = {}
    # Actual structural through absences remain None throughout conversion;
    # a missing bay rail cannot acquire the same exemption from state aliasing.
    for row in report["source_rows"]:
        x, y, width, height = row["box"]
        sources.setdefault(row["native_graphics"], {})[row["part"]] = None if row["structurally_absent"] else atlas.crop((x, y, x + width, y + height))
    artifacts = [{"path": path.relative_to(terminal.ROOT).as_posix(), "sha256": hashlib.sha256(payload).hexdigest()} for path, payload in files.items()]
    return terminal.load_geometry(), sources, artifacts


def review_board(poses):
    """Compare every native state/view and four genuine loading phases at fixed scale.

    @param poses Whole source comparisons indexed by state, graphics and frame.
    @return RGB native3x contact sheet with explicit hidden-front phase aliases.
    """
    board = Image.new("RGB", (1560, 1184), (228, 233, 237))
    draw = ImageDraw.Draw(board)
    draw.text((12, 8), "SOURCE ONLY: atomic build/refusal previews; native installed ready/loading owners; hidden front glass stays hidden; runtime pending", fill="black")
    # Every cell and state is shown, including deliberately static hidden rear
    # views. Their apparent lack of flashing must not be mistaken for missing
    # source coverage or an excuse to relocate hardware onto the outside wall.
    for state_index, state in enumerate(STATES):
        y = 32 + state_index * 288
        for graphics in range(6):
            x = 12 + graphics * 256
            draw.text((x, y), f"{state} gfx{graphics}" + (" hidden glass" if not views.front_visible(graphics) else ""), fill="black")
            rgba = poses[state, graphics, 0].convert("RGBA").resize((192, 192), Image.Resampling.NEAREST)
            board.paste(rgba, (x, y + 20), rgba)
            for frame in range(4):
                rgba = poses[state, graphics, frame].convert("RGBA")
                board.paste(rgba, (x + frame * 64, y + 220), rgba)
    return board


def build(record, public, source_only=False):
    """Build all288 native state/frame/view/parent slots with32 true structural absences.

    @param record Selected genuine originals, native registrations and glass ancestry.
    @param public Authoritative shared facility.road-terminal row.
    @param source_only Reconstruct independently frozen physical artwork without current native proof.
    @return Complete source report and deterministic atlas/review artifact mapping.
    """
    geometry, component = checked_components(record, public, source_only=source_only)
    lenses = original_lenses(component)
    native, sources, prior_artifacts = base_sources(record)
    atlas = Image.new("P", (1152, 1024), 0)
    atlas.putpalette(terminal.base.PALETTE_BYTES)
    rows, poses = [], {}
    # The public matrix owns every required slot, including static preview and
    # hidden-front aliases. Only actual visible equipment glass may change;
    # road/ground, physical parents and native sorting remain independent.
    for state_index, state in enumerate(STATES):
        for frame in range(4):
            phase = geometry["phase_aliases"][state][frame]
            for graphics in range(6):
                layout = native["layouts"][graphics]
                parts = {part: image.copy() if image is not None else None for part, image in sources[graphics].items()}
                target = geometry["cue_targets"][str(graphics)]
                if target is not None:
                    parts[target["part"]] = apply_lens(parts[target["part"]], lenses[state][phase], target)
                # Validate all native contacts and access after changing color.
                # Original amber inventory was already checked in base_sources;
                # state glass legitimately uses different original palette hues.
                poses[state, graphics, frame] = views.validate_view([parts[part] for part in terminal.PARTS], layout, check_original_glass=False)
                for part_index, part in enumerate(terminal.PARTS):
                    x, y = (graphics * 3 + part_index) * 64, (state_index * 4 + frame) * 64
                    sprite = parts[part]
                    if sprite is not None:
                        atlas.paste(sprite, (x, y))
                    rows.append({"asset_id": "facility.road-terminal", "direction": layout["direction"], "native_graphics": graphics,
                                 "tile_variant": 0, "world_cell": [0, 0], "state": state, "frame": frame,
                                 "authored_phase": phase if target is not None else 0, "equipment_glass_visible": target is not None,
                                 "part": part, "structurally_absent": sprite is None, "box": [x, y, 64, 64], "anchor": [-31, -33],
                                 "native_packed_anchor": views.packed_anchor(layout, part_index),
                                 "source_pixels_sha256": hashlib.sha256(sprite.tobytes()).hexdigest() if sprite is not None else None})
    nonempty = sum(not row["structurally_absent"] for row in rows)
    # Only through parent2 is absent. Complete-source publication must retain
    # all16 real parents in each of the16 declared state/frame combinations.
    if len(rows) != 288 or nonempty != 256:
        raise ValueError("terminal source state/view/phase/parent coverage is incomplete")
    payload = terminal.png_bytes(atlas)
    files = {OUTPUT / f"{PREFIX}-authored.png": payload}
    board = io.BytesIO()
    metadata = PngImagePlugin.PngInfo()
    metadata.add_text("Owner", "r2mlvs14-remaining-assets-and-runtime-integration.md task25.3; archi-map-layers.md; complete source, runtime pending")
    review_board(poses).save(board, format="PNG", pnginfo=metadata, optimize=False, compress_level=9)
    files[OUTPUT / f"{PREFIX}-review.png"] = board.getvalue()
    animation = io.BytesIO()
    frames = []
    # This repeatable source animation compares authored glass only. It does
    # not execute native loading, advance a clock or prove vehicle depth sorting.
    for frame in range(4):
        page = Image.new("RGB", (768, 128), (228, 233, 237))
        for graphics in range(6):
            rgba = poses["working", graphics, frame].convert("RGBA").resize((128, 128), Image.Resampling.NEAREST)
            page.paste(rgba, (graphics * 128, 0), rgba)
        frames.append(page)
    frames[0].save(animation, format="GIF", save_all=True, append_images=frames[1:], duration=240, loop=0, optimize=False)
    files[OUTPUT / f"{PREFIX}-working-review.gif"] = animation.getvalue()
    report = {"_documentation": "r2mlvs14-remaining-assets-and-runtime-integration.md task25.3; archi-map-layers.md",
              "asset_id": "facility.road-terminal", "source_coverage_complete": True, "runtime_activated": False,
              "source": (OUTPUT / f"{PREFIX}-authored.png").relative_to(terminal.ROOT).as_posix(),
              "source_sha256": hashlib.sha256(payload).hexdigest(), "native_size": [64, 64], "anchor": [-31, -33],
              "footprint": [1, 1], "tile_variants": 1, "states": list(STATES), "frames": 4,
              "directions": [layout["direction"] for layout in native["layouts"]], "parts": list(terminal.PARTS), "sprites": rows,
              "source_slots": 288, "nonempty_source_parts": 256, "structurally_absent_parts": 32,
              "native_state_policy": public["native_state_policy"], "equipment_geometry": geometry,
              "original_status_glass": component, "preserved_native_view_artifacts": prior_artifacts,
              "remaining": "Native packing, decoded-pixel loader and ordinary-game rendering under master10.4D"}
    report["artifacts"] = [{"path": path.relative_to(terminal.ROOT).as_posix(), "sha256": hashlib.sha256(data).hexdigest()} for path, data in files.items()]
    files[OUTPUT / f"{PREFIX}-manifest.json"] = (json.dumps(report, indent=2) + "\n").encode("utf-8")
    return report, files


def validate_public_source(public, report):
    """Bind complete public source claims to the exact generated atlas and retained original.

    @param public Shared terminal family row after source publication.
    @param report Fresh complete source report with all actual native slots.
    @return None after exact source/public validation; runtime remains inactive.
    """
    authoring = public.get("authoring", {})
    # Counts alone cannot identify a usable source. Every complete claim binds
    # the exact native atlas and structural absences without allocating a range.
    if (public["source"]["status"] != "authored" or not authoring.get("coverage_complete")
            or authoring.get("native_atlas") != report["source"] or authoring.get("native_sha256") != report["source_sha256"]
            or authoring.get("atlas_manifest") != (OUTPUT / f"{PREFIX}-manifest.json").relative_to(terminal.ROOT).as_posix()
            or authoring.get("staged_pose_count") != 288 or authoring.get("nonempty_source_parts") != 256
            or authoring.get("structurally_absent_parts") != 32 or authoring.get("runtime_activated") is not False
            or public["packed"]["offset"] is not None):
        raise ValueError("terminal public source coverage or exact atlas binding changed")
    if terminal.base.digest(terminal.base.repository_path(public["source"]["path"])) != public["source"].get("sha256"):
        raise ValueError("terminal public original source identity changed")


def publish_source(record, document, report):
    """Publish full terminal source metadata through its existing shared family owner.

    @param record Original terminal selections; only family delivery metadata changes.
    @param document Shared public manifest; only facility.road-terminal metadata changes.
    @param report Fully validated complete source report from genuine originals.
    @return None; no packed sprite allocation or native activation occurs.
    """
    row = next(row for row in document["assets"] if row["asset_id"] == "facility.road-terminal")
    family = record["families"]["facility.road-terminal"]
    base = next(candidate for candidate in record["candidates"] if candidate["id"] == "terminal-se-loader-v1")
    selected = {selection["source"] for selection in family["ready_parts"]}
    originals = [candidate["path"] for candidate in record["candidates"] if candidate["id"] in selected]
    originals.append(family["state_component_source"]["path"])
    row["source"] = {"status": "authored", "path": base["path"], "sha256": base["sha256"],
                     "provenance": "first_party_original_model_and_component_assembly", "rights": "first_party_generated_original"}
    row["authoring"].update(date="2026-10-12", status="complete_native_terminal_source_runtime_pending", originals=originals,
                            atlas_manifest=(OUTPUT / f"{PREFIX}-manifest.json").relative_to(terminal.ROOT).as_posix(),
                            native_atlas=report["source"], native_sha256=report["source_sha256"], staged_pose_count=288,
                            nonempty_source_parts=256, structurally_absent_parts=32, coverage_complete=True,
                            runtime_activated=False, remaining=report["remaining"])
    family.update(status="complete_native_terminal_source_runtime_pending", source_coverage_complete=True,
                  atlas_manifest=row["authoring"]["atlas_manifest"], native_atlas=report["source"], native_sha256=report["source_sha256"],
                  remaining=report["remaining"])
    validate_public_source(row, report)
    terminal.RECORD.write_text(json.dumps(record, indent=2) + "\n", encoding="utf-8")
    terminal.PUBLIC.write_text(json.dumps(document, indent=2) + "\n", encoding="utf-8")


def main():
    """Publish the complete validated source matrix without changing native packed routes.

    @return None; source/refusal validation precedes any artifact publication.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    operation = parser.add_mutually_exclusive_group(required=True)
    operation.add_argument("--review", action="store_true")
    operation.add_argument("--publish", action="store_true")
    args = parser.parse_args()
    record = json.loads(terminal.RECORD.read_text(encoding="utf-8"))
    document = json.loads(terminal.PUBLIC.read_text(encoding="utf-8"))
    public = next(row for row in document["assets"] if row["asset_id"] == "facility.road-terminal")
    report, files = build(record, public)
    OUTPUT.mkdir(parents=True, exist_ok=True)
    # All actual state/view/phase/parent sources validate before accepted
    # artifacts change. Native gameplay activation remains its later owner.
    for path, payload in files.items():
        path.write_bytes(payload)
    if args.publish:
        publish_source(record, document, report)
    print(json.dumps({"source_slots": 288, "nonempty_source_parts": 256, "structurally_absent_parts": 32,
                      "source_coverage_complete": True, "runtime_activated": False}))


if __name__ == "__main__":
    main()
