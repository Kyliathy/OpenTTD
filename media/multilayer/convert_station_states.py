"""Assemble complete conventional station source coverage from retained native views.

@file convert_station_states.py
@see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup3.md Group17DD, immutable source-era reconstruction.
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md task24.3.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Default station construction is atomic. Preview aliases and original status
glass express availability/loading without inventing installed work or cargo.
"""

import argparse
import hashlib
import io
import json

from PIL import Image, ImageDraw, PngImagePlugin

import convert_facility_art as facilities
import convert_station_canopies as canopies
from tools.ground_station_contract import source_digest


views = canopies.views
loader = canopies.loader
OUTPUT = canopies.OUTPUT
PREFIX = "station-source"
STATES = ("construction", "ready", "working", "blocked")


def signature(value):
    """Bind an approved source registration to canonical JSON fields.

    @param value Geometry or original-material ancestry to identify.
    @return SHA256 of canonical sorted JSON, independent of dictionary order.
    """
    return hashlib.sha256(json.dumps(value, sort_keys=True).encode()).hexdigest()


def checked_components(record, public, source_only=False):
    """Verify native state ownership and fixed original-glass fitting registrations.

    @param record Station source selections or isolated refusal fixture.
    @param public Shared native station row with its authoritative source matrix.
    @param source_only Require the exact frozen original record; grants no current native qualification.
    @return Approved fitting geometry and original donor description.
    @throws ValueError On moved fittings, foreign donors or invented preview stages.
    """
    views.validate_station_contract(public)
    family = record["families"]["facility.rail-station"]
    geometry, component = family["state_component_geometry"], family["state_component_source"]
    # The original post/deck/roof registrations and feature-owned state policy
    # govern every status pixel. A merely opaque alternative position cannot
    # acquire approval or silently reinterpret a preview as installed work.
    if (signature(geometry) != family["approved_state_component_geometry_sha256"]
            or signature(component) != family["approved_state_component_source_sha256"]
            or component["geometry_sha256"] != signature(geometry)
            or geometry["state_owner"] != public["native_state_policy"]
            or geometry["phase_aliases"] != public["native_state_policy"]["frame_aliases"]
            or geometry["native_canvas"] != [64, 64] or geometry["anchor"] != [-31, -33]
            or set(geometry["cue_targets"]) != {str(gfx) for gfx in range(8)}):
        raise ValueError("station state component geometry or native ownership changed")
    # Native picker/load/departure/tick owners were inspected for this source
    # decision. A changed producer requires review before its old snapshot can
    # authorize subsequent packing or new presentation claims.
    if source_only:
        from station_source_facts import verify_original
        verify_original(record, "record", asset="facility.rail-station")
    else:
        # Current native source authoring still requires its actual presentation
        # owners. Frozen original art cannot certify changed loading/tick code.
        for name, digest in geometry["native_presentation_owner_sha256"].items():
            if source_digest(loader.base.ROOT / name) != digest:
                raise ValueError("station native preview/loading presentation owner changed")
    if component["source_record"] != facilities.RECORD.relative_to(loader.base.ROOT).as_posix() or component["source_key"] != "facility-status":
        raise ValueError("station status glass lacks its original approved donor")
    return geometry, component


def original_lenses(component):
    """Select genuine existing state/phase glass without copying a foreign housing.

    @param component Approved original status-glass owner/path/hash registration.
    @return Indexed2x1 glass samples for all four states and actual authored phases.
    @throws ValueError If the original donor changed or its native phases collapse.
    """
    donor = json.loads(facilities.RECORD.read_text(encoding="utf-8"))
    sources = [row for row in donor["components"] if row["key"] == component["source_key"]]
    if len(sources) != 1:
        raise ValueError("station status original is missing or ambiguous")
    source = sources[0]
    # The named donor must retain its exact original pixels. Similar colors
    # from another image cannot inherit the existing generated-art provenance.
    if source["path"] != component["path"] or source["sha256"] != component["sha256"]:
        raise ValueError("station status glass original ancestry changed")
    originals = facilities.indicator(source)
    result = {}
    # Only original opaque glass interiors supply material. BOX sampling retains
    # authored state/phase strengths; no hue shift, software glow or generated
    # placeholder may stand in for a missing loading/refusal source cue.
    for state, phases in zip(STATES, originals):
        result[state] = [loader.base.quantize(phase.resize((2, 1), Image.Resampling.BOX), {}) for phase in phases]
        if any(not all(image.get_flattened_data()) for image in result[state]):
            raise ValueError("station original status glass loses its opaque fitting")
    if len({image.tobytes() for image in result["working"]}) != 4:
        raise ValueError("station native loading glass phases collapse after palette conversion")
    return result


def apply_lens(original, lens, target):
    """Change only two approved opaque equipment-glass pixels in one native parent.

    @param original Indexed64x64 approved back/front parent.
    @param lens Indexed2x1 retained original state/phase glass material.
    @param target Exact approved parent/point registration for the genuine native cell.
    @return Indexed parent with original alpha, contacts and all other pixels exact.
    @throws ValueError On duplicate, transparent or out-of-canvas fittings.
    """
    points = target["points"]
    # These two physical pixels describe an inset equipment fitting. Reusing a
    # pixel twice or extending it into empty space could move the silhouette
    # or obscure the railway without altering the declared sprite dimensions.
    if len(points) != 2 or len({tuple(point) for point in points}) != 2 or lens.size != (2, 1):
        raise ValueError("station status glass fitting is missing or duplicated")
    pose = original.copy()
    for (x, y), value in zip(points, lens.get_flattened_data()):
        if not (0 <= x < 64 and 0 <= y < 64) or original.getpixel((x, y)) == 0 or value == 0:
            raise ValueError("station status glass must stay in its opaque native fitting")
        pose.putpixel((x, y), value)
    return pose


def base_sources(record):
    """Resolve all eight approved native ready views and structural source absences.

    @param record Immutable station original and component selections.
    @return Compiled geometry, indexed owners by graphics and exact prior artifact hashes.
    """
    geometry = views.load_geometry()
    reports = [views.build(record), canopies.build(record)]
    sources, artifacts = {}, []
    # Every real cell consumes its already proven genuine-axis source views.
    # Blank low-role roof/glass slots remain explicit structural absences;
    # no missing canopy is allowed to inherit that exemption.
    for report, files in reports:
        name = views.PREFIX if report["native_graphics"][0] == 0 else canopies.PREFIX
        atlas = Image.open(io.BytesIO(files[OUTPUT / f"{name}-atlas.png"]))
        for row in report["source_rows"]:
            sources.setdefault(row["native_graphics"], {})[row["part"]] = None if row["structurally_absent"] else atlas.crop(row["box"])
        artifacts.extend({"path": path.relative_to(loader.base.ROOT).as_posix(), "sha256": hashlib.sha256(payload).hexdigest()} for path, payload in files.items())
    return geometry, sources, artifacts


def compose(parts):
    """Render a source-only whole preview using the native parent/child order.

    @param parts Indexed back/front/optional roof/glass owners on one common canvas.
    @return Opaque-glass indexed comparison; native transparency belongs to later game proof.
    """
    whole = parts["back"].copy()
    # Only a comparison is composed here. Packed elevated roof and relative
    # transparent glass remain separate owners, preserving native train sorting.
    for part in ("front", "roof", "glass"):
        if parts[part] is not None:
            whole = canopies.overlay_original(whole, parts[part])
    return whole


def review_board(poses):
    """Compare all station states, native views and four authored loading phases.

    @param poses Whole previews indexed by state, native graphics and source frame.
    @return RGB native3x and four-phase source contact sheet.
    """
    board = Image.new("RGB", (2072, 1184), (228, 233, 237))
    draw = ImageDraw.Draw(board)
    draw.text((12, 8), "SOURCE ONLY: atomic build/refusal previews; ready/working actual installed owners. Fixed geometry, original status glass only; runtime pending", fill="black")
    # One row per state and one column per actual native cell makes every
    # source pose visible. The loading strip shows genuine phases; the other
    # rows intentionally repeat static aliases instead of fictional stages.
    for state_index, state in enumerate(STATES):
        y = 32 + state_index * 288
        for graphics in range(8):
            x = 12 + graphics * 256
            draw.text((x, y), f"{state} gfx{graphics} {'X' if graphics % 2 == 0 else 'Y'}", fill="black")
            original = poses[state, graphics, 0].convert("RGBA")
            large = original.resize((192, 192), Image.Resampling.NEAREST)
            board.paste(large, (x, y + 20), large)
            for frame in range(4):
                rgba = poses[state, graphics, frame].convert("RGBA")
                board.paste(rgba, (x + frame * 64, y + 220), rgba)
    return board


def build(record, public, source_only=False):
    """Build the complete512-slot source matrix, preserving all384 nonempty originals.

    @param record Selected station original, native-view and glass-component ancestry.
    @param public Authoritative shared facility.rail-station row.
    @param source_only Reconstruct independently frozen physical artwork without current native proof.
    @return Complete source report and deterministic native atlas/review artifacts.
    """
    geometry, component = checked_components(record, public, source_only=source_only)
    lenses = original_lenses(component)
    native, sources, prior_artifacts = base_sources(record)
    atlas = sources[0]["back"].copy().resize((2048, 1024), Image.Resampling.NEAREST)
    atlas.putdata([0] * (2048 * 1024))
    rows, poses = [], {}
    # The public matrix owns every state/frame/real role/axis/part. Static
    # aliases remain explicit rows; missing views or absent elevated sources
    # cannot be concealed by copying a whole model into another slot.
    for state_index, state in enumerate(STATES):
        for frame in range(4):
            phase = geometry["phase_aliases"][state][frame]
            for graphics in range(8):
                layout = native["layouts"][graphics]
                target = geometry["cue_targets"][str(graphics)]
                parts = {part: image.copy() if image is not None else None for part, image in sources[graphics].items()}
                owner = target["part"]
                parts[owner] = apply_lens(parts[owner], lenses[state][phase], target)
                # Equipment presentation must retain every original contact
                # and silhouette. Only the approved two-pixel fitting changes;
                # roof/glass and the other platform are byte-exact native views.
                allowed = {tuple(point) for point in target["points"]}
                before, after = sources[graphics][owner], parts[owner]
                for index, (old, new) in enumerate(zip(before.get_flattened_data(), after.get_flattened_data())):
                    if (index % 64, index // 64) not in allowed and old != new:
                        raise ValueError("station state changes material outside original equipment glass")
                if layout["role"] < 2:
                    views.validate_ready_pose(parts, layout)
                poses[state, graphics, frame] = compose(parts)
                for part_index, part in enumerate(views.PARTS):
                    x, y = (graphics * 4 + part_index) * 64, (state_index * 4 + frame) * 64
                    sprite = parts[part]
                    if sprite is not None:
                        atlas.paste(sprite, (x, y))
                    rows.append({"asset_id": "facility.rail-station", "direction": "NE" if layout["axis"] == "X" else "SE",
                                 "tile_variant": layout["role"], "native_graphics": graphics, "world_cell": [0, 0],
                                 "state": state, "frame": frame, "authored_phase": phase, "part": part,
                                 "structurally_absent": sprite is None, "box": [x, y, 64, 64], "anchor": [-31, -33],
                                 "native_packed_anchor": views.native_packed_anchor(layout, part),
                                 "source_pixels_sha256": hashlib.sha256(sprite.tobytes()).hexdigest() if sprite is not None else None})
    nonempty = sum(not row["structurally_absent"] for row in rows)
    # Genuine structural absences have exactly128 slots. A missing original or
    # extra view must not silently reduce the public complete-source requirement.
    if len(rows) != 512 or nonempty != 384:
        raise ValueError("station source state/view/phase/part coverage is incomplete")
    payload = views.png_bytes(atlas)
    files = {OUTPUT / f"{PREFIX}-authored.png": payload}
    board = io.BytesIO()
    metadata = PngImagePlugin.PngInfo()
    metadata.add_text("Owner", "r2mlvs14-remaining-assets-and-runtime-integration.md task24.3; archi-map-layers.md; complete source, runtime pending")
    review_board(poses).save(board, format="PNG", pnginfo=metadata, optimize=False, compress_level=9)
    files[OUTPUT / f"{PREFIX}-review.png"] = board.getvalue()
    animation = io.BytesIO()
    frames = []
    # The GIF is a deterministic source comparison. No native vehicle loading
    # or simulation clock is executed or claimed by showing these authored phases.
    for frame in range(4):
        page = Image.new("RGB", (1024, 128), (228, 233, 237))
        for graphics in range(8):
            rgba = poses["working", graphics, frame].convert("RGBA").resize((128, 128), Image.Resampling.NEAREST)
            page.paste(rgba, (graphics * 128, 0), rgba)
        frames.append(page)
    frames[0].save(animation, format="GIF", save_all=True, append_images=frames[1:], duration=240, loop=0, optimize=False)
    files[OUTPUT / f"{PREFIX}-working-review.gif"] = animation.getvalue()
    report = {"_documentation": "r2mlvs14-remaining-assets-and-runtime-integration.md task24.3; archi-map-layers.md",
              "asset_id": "facility.rail-station", "source_coverage_complete": True, "runtime_activated": False,
              "source": (OUTPUT / f"{PREFIX}-authored.png").relative_to(loader.base.ROOT).as_posix(),
              "source_sha256": hashlib.sha256(payload).hexdigest(), "native_size": [64, 64], "anchor": [-31, -33],
              "footprint": [1, 1], "tile_variants": 4, "states": list(STATES), "frames": 4,
              "directions": ["NE", "SE"], "parts": list(views.PARTS), "sprites": rows,
              "source_slots": 512, "nonempty_source_parts": 384, "structurally_absent_parts": 128,
              "native_state_policy": public["native_state_policy"], "equipment_geometry": geometry,
              "original_status_glass": component, "preserved_native_view_artifacts": prior_artifacts,
              "remaining": "Native packing, decoded-pixel loader and ordinary-game rendering under master10.4D"}
    report["artifacts"] = [{"path": path.relative_to(loader.base.ROOT).as_posix(), "sha256": hashlib.sha256(data).hexdigest()} for path, data in files.items()]
    files[OUTPUT / f"{PREFIX}-manifest.json"] = (json.dumps(report, indent=2) + "\n").encode("utf-8")
    return report, files


def validate_public_source(public, report):
    """Refuse public completion claims detached from the actual complete native atlas.

    @param public Published facility.rail-station family row.
    @param report Fresh deterministic full source coverage and artifact identities.
    @return None after exact source/public binding checks.
    """
    authoring = public.get("authoring", {})
    # Coverage alone is insufficient: source consumers need the exact atlas,
    # complete384 nonempty/128 absent mapping and unactivated native boundary.
    if (public["source"]["status"] != "authored" or not authoring.get("coverage_complete")
            or authoring.get("native_atlas") != report["source"]
            or authoring.get("native_sha256") != report["source_sha256"]
            or authoring.get("atlas_manifest") != (OUTPUT / f"{PREFIX}-manifest.json").relative_to(loader.base.ROOT).as_posix()
            or authoring.get("staged_pose_count") != 512 or authoring.get("nonempty_source_parts") != 384
            or authoring.get("structurally_absent_parts") != 128 or authoring.get("runtime_activated") is not False
            or public["packed"]["offset"] is not None):
        raise ValueError("station public source coverage or exact atlas binding changed")
    source_path = loader.base.repository_path(public["source"]["path"])
    if loader.base.digest(source_path) != public["source"].get("sha256"):
        raise ValueError("station public original source identity changed")


def publish_source(record, document, report):
    """Publish complete source metadata for this family while preserving every packed route.

    @param record Original station selections; only family source-delivery metadata changes.
    @param document Shared public manifest; only facility.rail-station metadata changes.
    @param report Fully validated complete source atlas/report from the selected originals.
    @return None; sprites remain unpacked and runtime inactive.
    """
    row = next(row for row in document["assets"] if row["asset_id"] == "facility.rail-station")
    family = record["families"]["facility.rail-station"]
    base = loader.original_candidate(record, "station-loader-x-base-v4")
    originals = [candidate["path"] for candidate in record["candidates"]
                 if candidate["id"] in ("station-loader-x-base-v4", "station-independent-parents-v2", "station-loader-y-rear-v1",
                                        "station-canopy-supports-v1", "station-canopy-roofs-v1")]
    originals.append(family["state_component_source"]["path"])
    row["source"] = {"status": "authored", "path": base["path"], "sha256": base["sha256"],
                     "provenance": "first_party_original_model_and_component_assembly", "rights": "first_party_generated_original"}
    row["authoring"].update(date="2026-10-12", status="complete_native_station_source_runtime_pending",
                            originals=originals, atlas_manifest=(OUTPUT / f"{PREFIX}-manifest.json").relative_to(loader.base.ROOT).as_posix(),
                            native_atlas=report["source"], native_sha256=report["source_sha256"],
                            staged_pose_count=512, nonempty_source_parts=384, structurally_absent_parts=128,
                            coverage_complete=True, runtime_activated=False, remaining=report["remaining"])
    family.update(status="complete_native_station_source_runtime_pending", source_coverage_complete=True,
                  atlas_manifest=row["authoring"]["atlas_manifest"], native_atlas=report["source"],
                  native_sha256=report["source_sha256"], remaining=report["remaining"])
    validate_public_source(row, report)
    loader.RECORD.write_text(json.dumps(record, indent=2) + "\n", encoding="utf-8")
    loader.PUBLIC.write_text(json.dumps(document, indent=2) + "\n", encoding="utf-8")


def main():
    """Publish the validated full source family without changing native packed ranges.

    @return None; any original/geometry refusal leaves published artifacts untouched.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    operation = parser.add_mutually_exclusive_group(required=True)
    operation.add_argument("--review", action="store_true")
    operation.add_argument("--publish", action="store_true")
    args = parser.parse_args()
    record = json.loads(loader.RECORD.read_text(encoding="utf-8"))
    public = json.loads(loader.PUBLIC.read_text(encoding="utf-8"))
    row = next(row for row in public["assets"] if row["asset_id"] == "facility.rail-station")
    report, files = build(record, row)
    OUTPUT.mkdir(parents=True, exist_ok=True)
    # Build and validate the entire matrix before publishing. Full source
    # coverage grants neither packed sprite IDs nor installed gameplay states.
    for path, payload in files.items():
        path.write_bytes(payload)
    if args.publish:
        publish_source(record, public, report)
    print(json.dumps({"source_slots": report["source_slots"], "nonempty_source_parts": 384,
                      "structurally_absent_parts": 128, "source_coverage_complete": True, "runtime_activated": False}))


if __name__ == "__main__":
    main()
