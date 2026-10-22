"""Export complete original launch-center source coverage with explicit still aliases.

@file convert_launch_center_source.py
@see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup3.md Group17CB.
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group61F.
@see docs/zz-reach2/upgrades/2026-10/r2mlsp-z-z-z-z-z-q-launch-center-source.md.
This converter consumes the selected genuine components and unchanged material
originals. Indexed PNG decoding is source evidence, never packed-GRF or game
acceptance. Run the focused launch_facility_source_matrix_checks.py publisher
to qualify all positions before updating this family's public source metadata.
"""

import hashlib
import json

from PIL import Image, ImageDraw

from tools import launch_facility_states as states
from tools import launch_facility_foregrounds as occupied
from tools import launch_facility_art_contract as contract

base = states.base
ROOT = base.ROOT
POLICY = ROOT / "media/multilayer/source/space/launch-center-state-art-v2.json"
OUTPUT = ROOT / "logs/proofs/mlm/native-art/space/launch-center-source-v1"
PUBLIC = ROOT / "media/multilayer/contracts/multilayer_sprite_manifest.json"
RECORD = ROOT / "media/multilayer/source/space/space-generation-record.json"


def key(row):
    """Identify one actual public source position independently of pixel similarity.

    @param row Source metadata with direction, state, frame, actual cell and part.
    @return Hashable exact source-pose identity.
    """
    return tuple(row[field] for field in ("direction", "state", "frame", "tile_variant", "part"))


def pose(row):
    """Describe an explicit canonical source target in the established alias schema.

    @param row Canonical source cell/part metadata.
    @return Exact canonical direction/state/frame/cell/part reference.
    """
    return {field: row[field] for field in ("direction", "state", "frame", "tile_variant", "part")}


def write_json(path, document):
    """Write stable readable source evidence at an explicitly owned repository path.

    @param path Repository-local output path.
    @param document JSON-compatible source report or public metadata.
    @return None.
    """
    path.write_text(json.dumps(document, indent=2) + "\n", encoding="utf-8")


def checked_occupied_policy(policy):
    """Validate original occupied-cut policy and its independent retained fixture.

    @param policy Explicit version-two complete source-state handoff.
    @return Exact occupied contact/partition model after original input bindings pass.
    @throws ValueError On absent cuts, changed retained fixture or original geometry.
    """
    model = base.bound_json(policy["foreground_model"])
    fixture = base.bound_json(policy["foreground_fixture"])
    # State pixels and occupied partitions must extend the same original
    # models. A copied success flag cannot authorize changed source or geometry.
    if (policy["asset_id"] != contract.ASSET or not policy["occupied_foregrounds_complete"]
            or policy["foreground_policy"] != occupied.FOREGROUND_POLICY
            or not policy["state_art_complete"] or policy["construction_alias"] != "ready"
            or policy["frame_aliases"] != [0, 0, 0, 0]
            or policy["state_mapping"] != model["state_mapping"]
            or policy["status_art_checkpoint"] != model["status_art_checkpoint"]
            or not model["occupied_foregrounds_complete"] or fixture["model"] != policy["foreground_model"]
            or model["model_review"]["status"] != "approved_occupied_source_partitions"):
        raise ValueError("launch occupied source qualification does not match original state inputs")
    # The dated qualification identity remains historical policy metadata.
    # Actual composition/decoding checks the unchanged retained fixture and
    # original physical partition model; old success counters certify no run.
    occupied.checked_model(model)
    return model


def review_contacts(images, output):
    """Rebuild model-specific whole-view and exact glazing diagnostics from source pixels.

    @param images Actual composed state images indexed by genuine view.
    @param output Explicit owned source proof destination.
    @return Exact identities of the two independently rebuildable review contacts.
    """
    sheet = Image.new("RGB", (1920, 1920), "#29323c")
    detail = Image.new("RGB", (576, 672), "#29323c")
    labels, detail_labels = ImageDraw.Draw(sheet), ImageDraw.Draw(detail)
    # Whole views preserve physical continuity; enlarged glazing diagnostics
    # expose tiny genuine material changes without altering source artwork.
    for row, direction in enumerate(contract.VIEWS):
        cx, cy = states.CONTACTS[direction]
        for column, state in enumerate(("ready", "working", "blocked")):
            image = images[direction][state]
            panel = base.geometry_review(image, direction).resize((640, 448), Image.Resampling.NEAREST)
            sheet.paste(panel, (column * 640, row * 480 + 24))
            labels.text((column * 640 + 8, row * 480 + 8), f"{direction} {state}: source; diagnostic floor", fill="white")
            zoom = image.convert("RGB").crop((cx - 7, cy - 6, cx + 9, cy + 6)).resize((192, 144), Image.Resampling.NEAREST)
            detail.paste(zoom, (column * 192, row * 168 + 24))
            detail_labels.text((column * 192 + 8, row * 168 + 8), f"{direction} {state}: exact glass", fill="white")
    sheet.save(output / "source-contact.png")
    detail.save(output / "glazing-contact.png")
    return dict(whole=base.identity(output / "source-contact.png"), glazing=base.identity(output / "glazing-contact.png"))


def derive(output=OUTPUT, context=None):
    """Rebuild all logical positions from original images without private artifact inputs.

    @param output Exact owned launch-center source proof directory.
    @param context Explicit current source invocation with separate images/data/reports.
    @return Full source manifest with canonical indexed PNG locations and honest aliases.
    @throws ValueError On unsafe output location or invalid original/native source ownership.
    """
    if context is None:
        if output.resolve() != OUTPUT.resolve():
            raise ValueError("launch source conversion requires its explicit owned proof directory")
        reports, ownership_directory = output, output
    else:
        from regenerate_art_sources import source_directories
        output, reports = source_directories(output, context)
        ownership_directory = context.data
        ownership_directory.mkdir(parents=True, exist_ok=True)
    policy = json.loads(POLICY.read_text(encoding="utf-8"))
    # The plan requires physical occupied foreground source cuts even while
    # the current native stub hides Surface actors. Counted empty fronts alone
    # cannot satisfy that authoring gate or publish complete source coverage.
    if policy.get("occupied_foregrounds_complete") is not True:
        raise ValueError("required launch-center occupied foreground source is not yet qualified")
    model = checked_occupied_policy(policy)
    mapping = base.bound_json(model["state_mapping"])
    selection = base.bound_json(model["selected_bases"])
    output.mkdir(parents=True, exist_ok=True)
    canonical = []
    targets = {}
    evidence = {}
    whole_artifacts = {}
    ownership = {}
    view_images = {}
    # Each physical view is recomposed from its own selected originals. Alias
    # reuse is confined to the same native cell and part of that genuine view.
    for direction in contract.VIEWS:
        images, owners, cells, front, source_evidence = occupied.compose_states(model, direction)
        evidence[direction] = source_evidence
        view_images[direction] = images
        cell_path = ownership_directory / f"{direction.lower()}-cell-ownership.bin"
        component_path = ownership_directory / f"{direction.lower()}-component-ownership.bin"
        foreground_path = ownership_directory / f"{direction.lower()}-foreground.bin"
        cell_path.write_bytes(cells)
        component_path.write_bytes(owners)
        foreground_path.write_bytes(front)
        ownership[direction] = dict(cells=base.identity(cell_path), components=base.identity(component_path),
                                    foreground=base.identity(foreground_path))
        hashes = {}
        # Ready is the canonical still. New status images add a source entry
        # only when that actual cell's pixels differ, never by relabelling art.
        for state in ("ready", "working", "blocked"):
            path = output / f"{direction.lower()}-{state}-whole.png"
            images[state].save(path)
            whole_artifacts[direction, state] = base.identity(path)
            for row, image in occupied.cut_state(images[state], cells, front, direction, state):
                identity = (row["tile_variant"], row["part"], row["native_sha256"])
                if identity not in hashes:
                    hashes[identity] = len(canonical)
                    canonical.append((row, image))
                targets[direction, state, row["tile_variant"], row["part"]] = hashes[identity]
        # Atomic placement has no construction clock or partial facility.
        # Its logical source positions explicitly resolve to the same ready art.
        for tile in range(12):
            for part in base.PARTS:
                targets[direction, "construction", tile, part] = targets[direction, "ready", tile, part]
    columns = 12
    atlas = base.tiles.indexed_canvas((columns * 64, ((len(canonical) + columns - 1) // columns) * 64))
    rows = []
    # Canonical positions carry the actual indexed bytes once. Empty fronts
    # remain explicit source entries with genuine transparent index0 pixels.
    for index, (row, image) in enumerate(canonical):
        box = [index % columns * 64, index // columns * 64, 64, 64]
        atlas.paste(image, tuple(box[:2]))
        rows.append(dict(row, box=box, alias_of=None))
    atlas_path = output / "launch-center-authored.png"
    atlas.save(atlas_path, transparency=0)
    logical = []
    # Public coverage includes all frames even though no facility animation
    # producer exists. Explicit direct aliases retain native geometry and pose.
    for direction in contract.VIEWS:
        for state in states.STATES:
            for frame in range(4):
                for tile in range(12):
                    for part in base.PARTS:
                        target = rows[targets[direction, state, tile, part]]
                        row = dict(target, state=state, frame=frame)
                        row["alias_of"] = None if key(row) == key(target) else pose(target)
                        logical.append(row)
    report = dict(_documentation=__doc__, asset_id=contract.ASSET, states=list(states.STATES),
                  directions=list(contract.VIEWS), frames=4, tile_variants=12, parts=list(base.PARTS),
                  policy=base.identity(POLICY), state_mapping=policy["state_mapping"], selected_bases=mapping["selected_bases"],
                  foreground_model=policy["foreground_model"], occupied_foregrounds_complete=True,
                  geometry={direction: base.source.geometry(direction) for direction in contract.VIEWS},
                  evidence=evidence, canonical_parts=rows, sprites=logical,
                  logical_pose_count=len(logical), canonical_part_count=len(rows),
                  explicit_static_alias_count=sum(row["alias_of"] is not None for row in logical),
                  missing_pose_count=0, coverage_complete=True, packed=False, runtime_activated=False,
                  foreground_policy=occupied.FOREGROUND_POLICY, atlas=base.identity(atlas_path),
                  ownership=ownership, contacts=review_contacts(view_images, output),
                  whole_artifacts={direction: {state: whole_artifacts[direction, state] for state in ("ready", "working", "blocked")}
                                   for direction in contract.VIEWS})
    write_json(reports / "source-manifest.json", report)
    return report


def decode(report, atlas, context=None):
    """Decode every logical source position and refuse dishonest alias or native metadata.

    @param report Exact complete launch-center source manifest.
    @param atlas Actual stored indexed source PNG, opened independently after conversion.
    @param context Optional current invocation requiring exact selected image/data ownership.
    @return Logical metadata/image pairs for per-pose original-pixel reassembly.
    @throws ValueError On lost coverage, palette drift, false aliases or native cell changes.
    """
    required = {(direction, state, frame, tile, part) for direction in contract.VIEWS
                for state in states.STATES for frame in range(4) for tile in range(12) for part in base.PARTS}
    observed = [key(row) for row in report["sprites"]]
    # Exact source positions, not an aggregate count, protect real non-square
    # layouts and all empty foreground declarations from missing/duplicate rows.
    if (report["asset_id"] != contract.ASSET or len(observed) != 1536 or set(observed) != required
            or report["foreground_policy"] != occupied.FOREGROUND_POLICY
            or atlas.mode != "P" or atlas.getpalette() != list(base.source.site.art.base.PALETTE_BYTES)):
        raise ValueError("launch source coverage, visibility or indexed palette changed")
    canonical = {key(row): row for row in report["canonical_parts"]}
    if len(canonical) != len(report["canonical_parts"]):
        raise ValueError("launch source has duplicate canonical positions")
    # Canonical rows and explicit aliases must tell the same coverage story;
    # self-aliases or fabricated summary counts cannot create source completeness.
    if ({key(row) for row in report["sprites"] if row["alias_of"] is None} != set(canonical)
            or report["logical_pose_count"] != 1536 or report["canonical_part_count"] != len(canonical)
            or report["explicit_static_alias_count"] != sum(row["alias_of"] is not None for row in report["sprites"])
            or not report["coverage_complete"] or report["packed"] or report["runtime_activated"]):
        raise ValueError("launch source canonical coverage or acceptance scope changed")
    result = []
    policy = base.bound_json(report["policy"])
    model = checked_occupied_policy(policy)
    # Header provenance must name the same actual qualified cut, even when
    # all decoded pixel bytes happen to look valid on their own.
    if (report.get("occupied_foregrounds_complete") is not True
            or report["foreground_model"] != policy["foreground_model"]
            or report["state_mapping"] != policy["state_mapping"]
            or report["selected_bases"] != model["selected_bases"]):
        raise ValueError("launch source report changed its actual occupied-source provenance")
    maps = {}
    if context is not None:
        from regenerate_space_sources import selected_path
        selected_path(report["atlas"], context, "images")
    # Every decoded pixel must retain the independently qualified native cell
    # and physical part. Locally rehashed PNGs cannot move between those parts.
    for direction in contract.VIEWS:
        owner = report["ownership"][direction]
        for reference in owner.values():
            if context is not None:
                selected_path(reference, context, "data")
            if base.identity(ROOT / reference["path"]) != reference:
                raise ValueError("launch source original ownership artifact changed")
        cells = (ROOT / owner["cells"]["path"]).read_bytes()
        front = (ROOT / owner["foreground"]["path"]).read_bytes()
        native = base.source.geometry(direction)
        if (len(cells) != native["native_canvas"][0] * native["native_canvas"][1] or len(front) != len(cells)
                or hashlib.sha256(front).hexdigest() != model["views"][direction]["approved_foreground_sha256"]
                or hashlib.sha256(cells).hexdigest() != report["evidence"][direction]["original"]["cell_ownership_sha256"]):
            raise ValueError("launch source native ownership or reviewed physical cut changed")
        maps[direction] = (cells, front)
    # Each direct alias retains its original cell, part and view. Pixel reuse
    # cannot move machinery across native zones or turn a missing part into art.
    for row in report["sprites"]:
        target_key = key(row["alias_of"]) if row["alias_of"] is not None else key(row)
        target = canonical.get(target_key)
        if target is None:
            raise ValueError("launch source alias target is missing")
        native = base.source.geometry(row["direction"])
        tile = row["tile_variant"]
        if (row["asset_id"] != contract.ASSET or target["alias_of"] is not None or target["direction"] != row["direction"]
                or target["tile_variant"] != tile or target["part"] != row["part"]
                or target["frame"] != 0 or row["box"] != target["box"]
                or row["native_sha256"] != target["native_sha256"]
                or row["native_layout"] != native["native_rotation"]
                or row["world_cell"] != native["world_cells"][tile] or row["native_origin"] != native["native_cell_origins"][tile]
                or row["anchor"] != native["anchor"] or row["foreground_policy"] != occupied.FOREGROUND_POLICY):
            raise ValueError("launch source alias or native geometry changed")
        x, y, width, height = row["box"]
        if width != 64 or height != 64 or not (0 <= x <= atlas.width - 64 and 0 <= y <= atlas.height - 64):
            raise ValueError("launch source atlas rectangle changed")
        image = atlas.crop((x, y, x + width, y + height))
        count = sum(value != 0 for value in image.get_flattened_data())
        if (hashlib.sha256(image.tobytes()).hexdigest() != row["native_sha256"]
                or row["opaque_pixels"] != count or row["structurally_absent"] != (count == 0)):
            raise ValueError("launch source decoded pixels or foreground absence changed")
        cells, front = maps[row["direction"]]
        ox, oy = native["native_cell_origins"][tile]
        # Empty parts stay explicit; each real stored pixel must remain in
        # its actual cell and approved occupied back/front layer independently.
        for py in range(64):
            for px in range(64):
                if not image.getpixel((px, py)):
                    continue
                position = (oy + py) * native["native_canvas"][0] + ox + px
                if cells[position] != tile + 1 or front[position] != (row["part"] == "front"):
                    raise ValueError("launch source decoded pixel changes actual cell or occupied part")
        result.append((row, image))
    return result


def publish(report):
    """Publish this family's qualified source handoff without changing packed/runtime owners.

    @param report Fully qualified complete source report from the focused publisher.
    @return Exact current source-manifest identity.
    @throws ValueError On missing coverage or a changed atlas/contract.
    """
    policy = base.bound_json(report["policy"])
    if policy.get("occupied_foregrounds_complete") is not True:
        raise ValueError("occupied launch-center foreground review is required before source publication")
    checked_occupied_policy(policy)
    path = OUTPUT / "source-manifest.json"
    if json.loads(path.read_text(encoding="utf-8")) != report:
        raise ValueError("launch source report differs from its stored handoff")
    with Image.open(ROOT / report["atlas"]["path"]) as atlas:
        decode(report, atlas)
    public = json.loads(PUBLIC.read_text(encoding="utf-8"))
    row = next(row for row in public["assets"] if row["asset_id"] == contract.ASSET)
    contract.validate_launch_facility_contract(row)
    record = json.loads(RECORD.read_text(encoding="utf-8"))
    family = record["families"][contract.ASSET]
    model = base.bound_json(base.bound_json(family["selected_base"])["component_model"])
    primary = model["selected_components"]["workshop"]["original"]
    reference = base.identity(path)
    row["source"] = dict(status="authored", **primary, provenance="first_party_original_builtin_image_gen",
                         rights="first_party_generated_original", composition="whole_selected_component_originals_and_explicit_material_reuse",
                         composition_recipe=report["policy"])
    row["authoring"] = dict(date="2026-10-20", status="source_complete_runtime_pending", source_record=RECORD.relative_to(ROOT).as_posix(),
                            coverage_complete=True, runtime_activated=False, atlas_manifest=reference["path"],
                            atlas_manifest_sha256=reference["sha256"], native_atlas=report["atlas"]["path"],
                            native_sha256=report["atlas"]["sha256"], required_pose_count=1536, staged_pose_count=1536,
                            missing_pose_count=0, canonical_part_count=report["canonical_part_count"],
                            explicit_static_alias_count=report["explicit_static_alias_count"],
                            selected_owned_original_count=16, explicit_reused_original_count=2,
                            foreground_policy=occupied.FOREGROUND_POLICY, occupied_foregrounds_complete=True)
    family.update(status="source_complete_runtime_pending", source_coverage=True, source_manifest=reference)
    family.pop("source_candidate_manifest", None)
    write_json(PUBLIC, public)
    write_json(RECORD, record)
    return reference
