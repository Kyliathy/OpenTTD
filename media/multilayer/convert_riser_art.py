"""Review original electrical riser hardware on its one real native2x2 layout.

@file convert_riser_art.py
@see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup3.md Group17DDC, explicit current conduit review input.
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group60C.
@see docs/zz-reach2/upgrades/2026-10/r2mlvs18-geothermal-energy-and-industry.md Groups5B/5.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Native Industry owns four machinery cells; artwork creates no shaft, port or energy.
"""

import argparse
import hashlib
import io
import json

from PIL import Image, ImageDraw

import convert_utility_art as utilities
import convert_facility_art as facilities


tube = utilities.tube
RECORD = utilities.RECORD
REVIEW = utilities.REVIEW
ASSET = "facility.utility-riser"
CANVAS = (192, 144)
CELLS = [[0, 0], [1, 0], [0, 1], [1, 1]]
ORIGINS = [(65, 15), (33, 31), (97, 31), (65, 47)]
FLOOR = [[96, 48], [160, 80], [96, 112], [32, 80]]
STATES = ("construction", "ready", "working", "blocked")
ATLAS = REVIEW / "utility-riser-authored.png"
MANIFEST = REVIEW / "utility-riser-source-manifest.json"


def indexed_canvas(size):
    """Create a shared-palette transparent native canvas without new art pixels.

    @param size Exact whole-model or native-cell dimensions.
    @return Empty indexed canvas; only selected original pixels may fill it.
    """
    image = Image.new("P", size, 0)
    image.putpalette(tube.base.PALETTE_BYTES)
    image.info["transparency"] = 0
    return image


def registration_signature(source):
    """Freeze the actual immutable model, construction stage and native registration.

    @param source Retained original with its complete measured native transform.
    @return SHA256 independent of workflow/review wording.
    """
    fields = {key: source[key] for key in ("sha256", "asset_id", "stage", "native_transform")}
    return hashlib.sha256(json.dumps(fields, sort_keys=True).encode("utf-8")).hexdigest()


def public_family(public=None):
    """Check the actual fixed four-cell electrical-only native geometry.

    @param public Optional isolated shared manifest for refusal checks.
    @return Exact checked riser family; access/portal identity never rotates it.
    """
    if public is None:
        public = json.loads(tube.PUBLIC.read_text(encoding="utf-8"))
    family = next(row for row in public["assets"] if row["asset_id"] == ASSET)
    notes = family.get("geometry_notes", {})
    # The delivered native definition has one row-major layout and four cells.
    # Heat, fake rotations, a generator work bill or completion by stage3 alone
    # would report behaviour the paid electrical machinery does not implement.
    if (family["consumer"] != 18 or family["geometry"] != "per_tile"
            or family["directions"] != ["none"] or family["states"] != list(STATES)
            or family["frames"] != 4 or family["footprint"] != [2, 2]
            or family["tile_variants"] != 4 or family["parts"] != ["back", "front"]
            or family["native_size"] != [64, 64] or family["anchor"] != [-31, -33]
            or family.get("energy_domain") != "Electricity"
            or notes.get("world_cells") != CELLS or notes.get("cell_origins") != [list(p) for p in ORIGINS]
            or notes.get("whole_canvas") != list(CANVAS) or notes.get("floor_corners") != FLOOR
            or notes.get("construction_ticks") != {"total": 296, "stage_length": 74, "stages": [0, 1, 2, 3], "completion_separate": True}):
        raise ValueError("riser source differs from actual native electrical layout/work")
    return family


def guide():
    """Write the labelled one-layout authoring reference before original source work.

    @return Guide path; blue geometry/labels never enter source or packed pixels.
    """
    page = Image.new("RGB", (1152, 960), (238, 244, 250))
    draw = ImageDraw.Draw(page)
    draw.text((24, 20), "ELECTRICAL RISER / ONE FIXED2x2 NATIVE LAYOUT / portal remains separate", fill=(20, 50, 85))
    # Four saved TIDS cells stay in their real row-major world positions;
    # no rotated door/access presentation can change native machinery geometry.
    for index, ((x, y), (ox, oy)) in enumerate(zip(CELLS, ORIGINS)):
        cx, cy = ox + 31, oy + 49
        diamond = [(cx, cy - 16), (cx + 32, cy), (cx, cy + 16), (cx - 32, cy)]
        draw.polygon([(px * 6, py * 6 + 48) for px, py in diamond], fill=(222, 234, 247), outline=(25, 100, 180), width=3)
        draw.text((cx * 6 - 25, cy * 6 + 45), f"{index}:({x},{y})", fill=(20, 50, 85))
    draw.text((24, 880), "Floor128x64; whole192x144; each stored64x64 cell anchor(-31,-33).296 actual work ticks; separate completion flag.", fill=(20, 50, 85))
    draw.text((24, 914), "Electrical cable attaches only to actual site.port and original adjacent portal; artwork builds neither a shaft nor floor.", fill=(20, 50, 85))
    path = utilities.GUIDES / "utility-riser-native-geometry-v1.png"
    page.save(path)
    return path


def native_original(key, document=None, public=None):
    """Select an entire retained riser original at its frozen native registration.

    @param key Exact retained whole-model candidate identity.
    @param document Optional isolated original record for refusal checks.
    @param public Optional isolated shared source contract.
    @return Complete indexed native campus and checked original metadata.
    """
    if document is None:
        document = json.loads(RECORD.read_text(encoding="utf-8"))
    source = tube.retained_candidate(key, document)
    spec = public_family(public)
    # One genuine original supplies one fixed physical model/stage. A conduit,
    # rejected raster or arbitrary camera label cannot fill a missing riser pose.
    if (source["asset_id"] != ASSET or source["status"].startswith("rejected")
            or source["stage"] not in (0, 1, 2, "ready") or source.get("direction") != "none"):
        raise ValueError("riser original has foreign geometry/stage or is rejected")
    expected = spec["geometry_notes"].get("approved_riser_registrations", {}).get(key)
    if expected is not None and expected != registration_signature(source):
        raise ValueError("riser original changes its reviewed registration")
    transform = source["native_transform"]
    width, height = transform["native_subject_size"]
    px, py = transform["native_position"]
    # Whole-model sampling keeps the actual128x64 native floor envelope and
    # bounded low machinery. Lossless four-cell partitioning follows separately.
    if (transform["native_size"] != list(CANVAS) or transform["resampling"] != "BOX"
            or width != 128 or not 64 <= height <= 96 or px not in (32, 33)
            or not 15 <= py <= 48 or py + height > 112):
        raise ValueError("riser original leaves its native floor envelope")
    with Image.open(tube.base.repository_path(source["path"])) as image:
        original = image.convert("RGBA")
        crop = transform["source_crop"]
        bounds = original.getchannel("A").point(lambda value: 255 if value >= 16 else 0).getbbox()
        # Complete isolated originals need real clearance and measured ordinary
        # rectangles. Clipping machinery or a floor corner cannot be masked away.
        if (bounds is None or min(bounds[0], bounds[1], original.width - bounds[2], original.height - bounds[3]) < 16
                or not 0 <= crop[0] <= bounds[0] < bounds[2] <= crop[2] <= original.width
                or not 0 <= crop[1] <= bounds[1] < bounds[3] <= crop[3] <= original.height):
            raise ValueError("riser crop clips its original or lacks transparent clearance")
        selected = original.crop(crop).resize((width, height), Image.Resampling.BOX)
    whole = Image.new("RGBA", CANVAS)
    whole.paste(selected, (px, py))
    return tube.base.quantize(whole, {}), source


def partition(whole):
    """Assign each original pixel exactly once to its real native machinery cell.

    @param whole Checked indexed192x144 whole model at its native registration.
    @return Four native64x64 pieces, proven to reconstruct the original exactly.
    """
    pieces = [indexed_canvas((64, 64)) for cell in CELLS]
    # Overlapping native canvases share one physical machine. Nearest floor
    # contact gives each original pixel one owner rather than cloning equipment.
    for y in range(whole.height):
        for x in range(whole.width):
            value = whole.getpixel((x, y))
            if value == 0:
                continue
            eligible = [index for index, (ox, oy) in enumerate(ORIGINS) if 0 <= x - ox < 64 and 0 <= y - oy < 64]
            if not eligible:
                raise ValueError(f"riser original pixel escapes native cells: {x},{y}")
            owner = min(eligible, key=lambda index: (abs(x - ORIGINS[index][0] - 31) + 2 * abs(y - ORIGINS[index][1] - 49), index))
            ox, oy = ORIGINS[owner]
            pieces[owner].putpixel((x - ox, y - oy), value)
    rebuilt = assemble(pieces)
    if rebuilt.tobytes() != whole.tobytes() or any(piece.getbbox() is None for piece in pieces):
        raise ValueError("riser partition loses pixels or a native machinery cell")
    return pieces


def assemble(pieces):
    """Reconstruct stored native sections independently with unique pixel ownership.

    @param pieces Four actual row-major indexed64x64 machinery sections.
    @return Complete indexed native whole model; duplicate original pixels refuse.
    """
    result = indexed_canvas(CANVAS)
    # Reconstruction reads only actual stored cell pixels and world offsets;
    # comparing a copied original would conceal dropped or duplicate ownership.
    for piece, (ox, oy) in zip(pieces, ORIGINS):
        for y in range(64):
            for x in range(64):
                value = piece.getpixel((x, y))
                if value == 0:
                    continue
                if result.getpixel((ox + x, oy + y)) != 0:
                    raise ValueError("riser native cells duplicate original pixels")
                result.putpixel((ox + x, oy + y), value)
    return result


def geometry_contact(whole, pieces, conduit_report=None, conduit_atlas=None):
    """Review stored native cells and external port geometry without inventing a portal.

    @param whole Checked original whole ready model at its fixed native registration.
    @param pieces Four independently stored native machinery sections.
    @param conduit_report Optional explicitly selected original conduit source metadata.
    @param conduit_atlas Matching explicit indexed conduit atlas; both inputs must be supplied together.
    @return PNG source review with actual cable contacts and labelled geometry overlays.
    """
    # Reject mixed input epochs before composing any review. A current guide
    # cannot combine selected metadata with an earlier workstation atlas.
    if (conduit_report is None) != (conduit_atlas is None):
        raise ValueError("riser geometry requires both selected conduit report and atlas")
    page = Image.new("RGB", (1536, 1720), (46, 50, 57))
    draw = ImageDraw.Draw(page)
    draw.text((18, 15), "RISER SOURCE / one fixed native2x2 layout / port and access do not rotate machinery / packing-runtime pending", fill="white")
    # Original and independent reconstruction must agree across all four
    # real industry cells; each section is reviewed at its actual native anchor.
    for column, image in enumerate((whole, assemble(pieces))):
        sprite = image.convert("RGBA").resize((768, 576), Image.Resampling.NEAREST)
        page.paste(sprite, (column * 768, 35), sprite)
        draw.text((column * 768 + 18, 40), "original" if column == 0 else "reconstructed stored cells", fill="white")
    for index, piece in enumerate(pieces):
        sprite = piece.convert("RGBA").resize((256, 256), Image.Resampling.NEAREST)
        page.paste(sprite, (index * 384, 600), sprite)
        draw.text((index * 384 + 18, 860), f"cell{index} world{CELLS[index]} anchor(-31,-33); front empty", fill="white")
    # Historical authoring retains its existing paired defaults; selected
    # execution supplies both current inputs and never enters this cache read.
    if conduit_report is None:
        conduit_report = json.loads(utilities.MANIFEST.read_text(encoding="utf-8"))
        conduit_atlas = utilities.ATLAS
    sprites = {}
    # Only the delivered electrical branch source explains physical riser
    # contacts; thermal colours/status never imply an inter-layer Heat route.
    with Image.open(conduit_atlas) as atlas:
        for row in conduit_report["sprites"]:
            if row["domain"] == "Electricity" and row["state"] == "connected":
                x, y, width, height = row["box"]
                sprites[row["direction"]] = atlas.crop((x, y, x + width, y + height)).convert("RGBA")
    delta = {"NE": (-1, 0), "SE": (0, 1), "SW": (1, 0), "NW": (0, -1)}
    opposite = {"NE": "SW", "SE": "NW", "SW": "NE", "NW": "SE"}
    examples = []
    # Actual site.port may be a boundary machinery cell adjoining an original
    # external portal. These eight diagrams explain contacts only; native
    # admission independently validates traffic/ownership/actual portal identity.
    for index, (x, y) in enumerate(CELLS):
        for direction, (wx, wy) in delta.items():
            if [x + wx, y + wy] in CELLS:
                continue
            examples.append((index, direction))
    for panel, (index, direction) in enumerate(examples):
        scene = Image.new("RGBA", CANVAS)
        ox, oy = ORIGINS[index]
        origin = (ox, oy + 33)
        dx, dy = utilities.PITCH[direction]
        neighbour = (origin[0] + dx, origin[1] + dy)
        scene.alpha_composite(sprites[direction], origin)
        scene.alpha_composite(sprites[opposite[direction]], neighbour)
        scene.alpha_composite(whole.convert("RGBA"))
        sprite = scene.resize((384, 288), Image.Resampling.NEAREST)
        left, top = panel % 4 * 384, 930 + panel // 4 * 380
        page.paste(sprite, (left, top), sprite)
        draw.text((left + 12, top - 16), f"boundary cell{index}/{direction}: separate external endpoint", fill="white")
    stream = io.BytesIO()
    page.save(stream, format="PNG")
    return stream.getvalue()


def equipment_signature(family):
    """Freeze reviewed glass interiors and original component ancestry with stage policy.

    @param family Retained riser family with four native glass registrations.
    @return SHA256 for the reviewed physical equipment/state composition contract.
    """
    record = json.loads(tube.base.repository_path(family["indicator_record"]).read_text(encoding="utf-8"))
    component = next(row for row in record["components"] if row["key"] == family["indicator_key"])
    selected = {key: family[key] for key in ("whole_selections", "indicator_targets", "construction_indicator_phase", "construction_stage3_policy")}
    selected["component"] = component
    return hashlib.sha256(json.dumps(selected, sort_keys=True).encode("utf-8")).hexdigest()


def source_inputs(document=None, public=None):
    """Bind the four original hardware stages and actual authored status-glass phases.

    @param document Optional isolated original record for refusal checks.
    @param public Optional isolated shared contract from the same operation.
    @return Checked family, four whole models/sections/ancestry and original glass phases.
    """
    if document is None:
        document = json.loads(RECORD.read_text(encoding="utf-8"))
    spec = public_family(public)
    family = next(row for row in document["families"] if row["asset_id"] == ASSET)
    required_fields = {"whole_selections", "indicator_record", "indicator_key", "indicator_targets",
                       "construction_indicator_phase", "construction_stage3_policy"}
    if not required_fields.issubset(family):
        raise ValueError("riser source is missing required stage/equipment fields")
    keys = {"0", "1", "2", "ready"}
    # A ready alias may supply native stage3 only. Earlier incomplete stages
    # require distinct actual hardware originals, and amber phase0 is fixed
    # while the separate native completed flag still denies operation.
    if (set(family["whole_selections"]) != keys or len(set(family["whole_selections"].values())) != 4
            or set(family["indicator_targets"]) != {"0", "1", "2", "3"}
            or family["construction_indicator_phase"] != 0
            or family["construction_stage3_policy"] != "ready_hardware_under_amber_phase0_until_native_completion"
            or spec["geometry_notes"].get("approved_riser_equipment") != equipment_signature(family)):
        raise ValueError("riser original stage/equipment approval mismatch")
    models = {}
    # Each stage keeps its own complete original and four physical sections;
    # a substituted label or unfrozen placement cannot manufacture coverage.
    for key, source_key in family["whole_selections"].items():
        whole, source = native_original(source_key, document, public)
        stage = "ready" if key == "ready" else int(key)
        if source["stage"] != stage:
            raise ValueError("riser original substitutes a construction stage")
        if source_key not in spec["geometry_notes"].get("approved_riser_registrations", {}):
            raise ValueError("riser original lacks reviewed native registration")
        models[key] = whole, partition(whole), source
    record = json.loads(tube.base.repository_path(family["indicator_record"]).read_text(encoding="utf-8"))
    component = next(row for row in record["components"] if row["key"] == family["indicator_key"])
    phases = facilities.indicator(component)
    # Only the reviewed tiny opaque fitting changes. An enlarged patch would
    # repaint the cabinet/frame or move its physical sorting/ground silhouette.
    for key, target in family["indicator_targets"].items():
        x, y, width, height = target
        whole = models["ready" if key == "3" else key][0]
        if width != 2 or height != 2:
            raise ValueError("riser glass leaves its reviewed2x2 interior")
        if not 0 <= x <= CANVAS[0] - 2 or not 0 <= y <= CANVAS[1] - 2:
            raise ValueError("riser glass registration leaves native canvas")
        if any(whole.getpixel((px, py)) == 0 for py in range(y, y + 2) for px in range(x, x + 2)):
            raise ValueError("riser glass would replace native transparency")
    return family, models, component, phases


def compose_glass(whole, component, target):
    """Select original equipment glass while preserving all other body pixels/opacity.

    @param whole Checked indexed whole hardware original for this native stage.
    @param component Genuine authored opaque status/phase glass component.
    @param target Reviewed tiny x,y,width,height physical interior.
    @return Indexed whole pose; every pixel outside the fitting stays exact.
    """
    x, y, width, height = target
    # A transparent donor must refuse as an equipment-opacity violation
    # before quantization; it cannot become an empty or partly erased fitting.
    if component.getchannel("A").getextrema()[0] < 128:
        raise ValueError("riser original glass changes native opacity")
    selected = tube.base.quantize(component.resize((width, height), Image.Resampling.BOX), {})
    result = whole.copy()
    # Only actual existing opaque glass receives sampled original component
    # pixels. A transparent donor cannot erase hardware or create a new hole.
    for dy in range(height):
        for dx in range(width):
            value = selected.getpixel((dx, dy))
            if value == 0 or whole.getpixel((x + dx, y + dy)) == 0:
                raise ValueError("riser original glass changes native opacity")
            result.putpixel((x + dx, y + dy), value)
    return result


def build(document=None, public=None):
    """Compose128 source poses from real hardware stages and original glass phases.

    @param document Optional isolated source record for corruption checks.
    @param public Optional isolated shared Sprite Contract.
    @return Complete source manifest and exact indexed atlas; no packed range assigned.
    """
    family, models, component, phases = source_inputs(document, public)
    atlas = indexed_canvas((512, 1024))
    sprites = []
    # Native construction frame means actual paid stage, while the completed
    # states use original visual glass phases only. Both parts retain explicit
    # four-cell identities; a solid industry has no pass-through foreground.
    for state_index, state in enumerate(STATES):
        for frame in range(4):
            stage = frame if state == "construction" else 3
            key = str(stage) if stage < 3 else "ready"
            whole, original_pieces, source = models[key]
            phase = 0 if state == "construction" else frame
            target = family["indicator_targets"][str(stage)]
            pose = compose_glass(whole, phases[state_index][phase], target)
            pieces = partition(pose)
            for part_index, part in enumerate(("back", "front")):
                for variant, cell in enumerate(CELLS):
                    x, y = (part_index * 4 + variant) * 64, (state_index * 4 + frame) * 64
                    piece = pieces[variant] if part == "back" else indexed_canvas((64, 64))
                    atlas.paste(piece, (x, y))
                    sprites.append({"direction": "none", "state": state, "frame": frame, "part": part,
                                    "tile_variant": variant, "world_cell": cell, "anchor": [-31, -33],
                                    "box": [x, y, 64, 64], "source": source["path"], "source_sha256": source["sha256"],
                                    "registration_sha256": registration_signature(source), "native_transform": source["native_transform"],
                                    "construction_stage": frame if state == "construction" else None,
                                    "indicator": {"path": component["path"], "sha256": component["sha256"],
                                                  "state": state, "phase": phase, "target": target},
                                    "foreground_policy": "solid_native_industry_back_owns_original_pixels_front_empty"})
    report = {"_documentation": "r2mlvs14-remaining-assets-and-runtime-integration.md60C; Plan18 paid native riser construction; archi-map-layers.md. Source only, packing/runtime pending.",
              "schema_version": 1, "asset_id": ASSET, "path": ATLAS.relative_to(tube.ROOT).as_posix(),
              "size": [512, 1024], "native_cell": [64, 64], "whole_canvas": list(CANVAS), "cell_origins": [list(p) for p in ORIGINS],
              "world_cells": CELLS, "anchor": [-31, -33], "pose_count": 128, "sprites": sprites,
              "construction_ticks": {"total": 296, "stage_length": 74, "stages": [0, 1, 2, 3], "completion_separate": True},
              "source_coverage_complete": True, "runtime_activated": False,
              "source_record": RECORD.relative_to(tube.ROOT).as_posix()}
    return report, tube.indexed_png(atlas)


def assembled_pose(report, atlas, state, frame, part="back"):
    """Reconstruct one whole pose from actual stored source atlas crops.

    @param report Complete128-pose source manifest with four native cell identities.
    @param atlas Actual decoded indexed source atlas.
    @param state Explicit construction/ready/working/blocked presentation.
    @param frame Actual native construction stage or original glass phase.
    @param part Stored back/front part; front must stay empty for solid machinery.
    @return Whole indexed pose assembled independently from its four stored sections.
    """
    pieces = []
    for variant in range(4):
        row = next(r for r in report["sprites"] if (r["state"], r["frame"], r["part"], r["tile_variant"]) == (state, frame, part, variant))
        x, y, width, height = row["box"]
        pieces.append(atlas.crop((x, y, x + width, y + height)))
    return assemble(pieces)


def validate(report, encoded, document=None, public=None):
    """Inspect exact identities, decoded native palette and original source ancestry.

    @param report Candidate complete source publication with native cell/stage mappings.
    @param encoded Actual indexed PNG atlas bytes.
    @param document Optional isolated original record for refusal checks.
    @param public Optional isolated shared contract for refusal checks.
    @return None; a valid source result proves no installed/operating riser or loaded sprite.
    """
    required = {(state, frame, part, variant, "none") for state in STATES for frame in range(4)
                for part in ("back", "front") for variant in range(4)}
    identities = [(r["state"], r["frame"], r["part"], r["tile_variant"], r["direction"]) for r in report["sprites"]]
    if len(identities) != len(required) or set(identities) != required:
        raise ValueError("incomplete riser source native pose identities")
    # Source coverage cannot replace either actual paid completion or runtime
    # binding; the stage clock remains exactly the original296/74 native owner.
    if (report.get("asset_id") != ASSET or report.get("pose_count") != 128
            or report.get("source_coverage_complete") is not True or report.get("runtime_activated") is not False
            or report.get("construction_ticks") != {"total": 296, "stage_length": 74, "stages": [0, 1, 2, 3], "completion_separate": True}):
        raise ValueError("riser source falsely claims runtime or changes native work")
    with Image.open(io.BytesIO(encoded)) as atlas:
        # Reserved palette animation or opaque zero would change the physical
        # machinery silhouette independently of the actual native producer.
        if (atlas.mode != "P" or atlas.size != (512, 1024) or atlas.info.get("transparency") != 0
                or atlas.getpalette()[:768] != tube.base.PALETTE_BYTES
                or set(atlas.get_flattened_data()) & (tube.base.FORBIDDEN - {0})):
            raise ValueError("riser source palette/canvas mismatch")
    expected, original_pixels = build(document, public)
    if report["sprites"] != expected["sprites"]:
        raise ValueError("riser source changes original/stage/cell ancestry")
    if encoded != original_pixels:
        raise ValueError("riser source changes original native pixels")


def state_contact(report, encoded):
    """Show all actual decoded construction/state/phase whole machinery poses.

    @param report Checked128-pose source publication.
    @param encoded Actual indexed atlas bytes, not simultaneous whole-model copies.
    @return Reproducible native-scale comparison PNG for sixteen whole source poses.
    """
    page = Image.new("RGB", (1536, 1300), (46, 50, 57))
    draw = ImageDraw.Draw(page)
    draw.text((18, 15), "PAID ELECTRICAL RISER SOURCE / fixed2x2 layout / stage3 is unfinished until296 ticks / runtime pending", fill="white")
    with Image.open(io.BytesIO(encoded)) as atlas:
        # Construction changes hardware; completed states change only reviewed
        # original glass. Comparing stored reconstruction exposes cell loss.
        for row, state in enumerate(STATES):
            for frame in range(4):
                pose = assembled_pose(report, atlas, state, frame).convert("RGBA").resize((384, 288), Image.Resampling.NEAREST)
                left, top = frame * 384, 52 + row * 310
                page.paste(pose, (left, top), pose)
                draw.text((left + 12, top - 16), f"{state} {'stage' if state == 'construction' else 'phase'}{frame}", fill="white")
    stream = io.BytesIO()
    page.save(stream, format="PNG")
    return stream.getvalue()


def main():
    """Generate native geometry references or review one whole source candidate.

    @return None; all outputs are repository-local source proof caches only.
    """
    parser = argparse.ArgumentParser()
    parser.add_argument("--guide", action="store_true")
    parser.add_argument("--candidate")
    parser.add_argument("--geometry", action="store_true")
    parser.add_argument("--publish", action="store_true")
    parser.add_argument("--bind", action="store_true")
    options = parser.parse_args()
    if options.guide:
        print(guide().relative_to(tube.ROOT).as_posix())
        return
    if options.candidate:
        whole, source = native_original(options.candidate)
        pieces = partition(whole)
        path = REVIEW / (options.candidate + "-native.png")
        path.write_bytes(tube.indexed_png(whole))
        image = whole.convert("RGBA").resize((1152, 864), Image.Resampling.NEAREST)
        page = Image.new("RGB", image.size, (46, 50, 57))
        page.paste(image, (0, 0), image)
        page.save(REVIEW / (options.candidate + "-review.png"))
        if options.geometry:
            (REVIEW / "utility-riser-geometry-contact.png").write_bytes(geometry_contact(whole, pieces))
        print(json.dumps({"path": path.as_posix(), "registration": registration_signature(source),
                          "bounds": whole.getbbox(), "cell_pixels": [sum(v != 0 for v in p.get_flattened_data()) for p in pieces]}))
        return
    report, encoded = build()
    validate(report, encoded)
    if options.publish:
        ATLAS.write_bytes(encoded)
        MANIFEST.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
        (REVIEW / "utility-riser-state-contact.png").write_bytes(state_contact(report, encoded))
    if options.bind:
        # A shared source binding needs the actual checked atlas publication;
        # it assigns neither a packed range nor a native rendering consumer.
        if not options.publish or ATLAS.read_bytes() != encoded:
            raise ValueError("riser source binding requires actual checked publication")
        public = json.loads(tube.PUBLIC.read_text(encoding="utf-8"))
        public_family(public)["source"].update(status="authored", path=report["path"], sha256=hashlib.sha256(encoded).hexdigest(),
                                              manifest=MANIFEST.relative_to(tube.ROOT).as_posix(), source_coverage_complete=True,
                                              runtime_activated=False, rights="first_party_original_imagegen")
        tube.PUBLIC.write_text(json.dumps(public, indent=2) + "\n", encoding="utf-8")
    print("Electrical riser source:128 poses, four fixed cells,296 paid work ticks; packing/runtime pending")


if __name__ == "__main__":
    main()
