"""Stage retained M1 ground controls while keeping verification explicit.

@file convert_control_art.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs13-graphics-and-asset-pipeline.md Group 78B
@see docs/zz-reach2/upgrades/2026-10/r2mlvs3mp-master-plan.md checkpoint 8.5
Default execution bakes source art only. --check is a separate focused check;
--verify-and-bind publishes verified coverage after that check passes.
"""

import argparse
import hashlib
import json

from PIL import Image, ImageDraw

import convert_m1_art as base
from convert_terrain_art import png_bytes


RECORD = base.ASSETS / "control-generation-record.json"
REPORT = base.OUTPUT / "control-details-manifest.json"
REVIEW = base.ASSETS / "ground-controls-details-review.png"
STATES = ["enabled", "disabled", "pressed"]


def derive():
    """Bake exact selected source glyphs at native size and fixed UI anchors.

    @return Source coverage report and deterministic output-path/PNG-byte map.
    @throws ValueError If immutable inputs or selected public geometry disagree.
    """
    document = json.loads(RECORD.read_text(encoding="utf-8"))
    source = document["source"]
    if base.digest(base.repository_path(source["path"])) != source["sha256"]:
        raise ValueError("immutable ground control original changed")
    # Every input is retained locally; pre-existing cargo art is reused as-is,
    # never resized or reinterpreted as a new cargo identity during control work.
    for reference in source["references"] + document["cargo_reuse"]:
        if base.digest(base.repository_path(reference["path"])) != reference["sha256"]:
            raise ValueError("immutable control/cargo reference changed")
    public = json.loads((base.ASSETS.parent / "multilayer_sprite_manifest.json").read_text(encoding="utf-8"))
    families = {row["asset_id"]: row for row in public["assets"]}
    selected = document["families"]
    if source["grid"] != [len(selected), 3] or source["states"] != STATES:
        # Rows are genuine authored active/refused/selected presentations, not
        # simulated work phases or sparse placeholders for absent icon states.
        raise ValueError("ground control source coverage disagrees with brief")
    cells, boxes = base.source_cells(source)
    outputs, files, palette_cache = [], {}, {}
    # Existing native offsets stay with each control family. Only its three
    # source glyphs are staged; packing/runtime never follows sheet column order.
    for selection in selected:
        family = families[selection["asset_id"]]
        if family["directions"] != ["none"] or family["frames"] != 1 or family["states"] != STATES:
            # Menu glyphs have no world heading and cannot imply vehicle turning.
            raise ValueError("ground control native state/direction mismatch")
        if family["native_size"] != [20, 20] or family["anchor"] != [0, 0]:
            # UI canvas/anchor remains separate from physical world sorting.
            raise ValueError("ground control native canvas/anchor mismatch")
        sheet = Image.new("P", (60, 20), 0)
        sheet.putpalette(base.PALETTE_BYTES)
        sprites = []
        column = selection["column"]
        for row, state in enumerate(STATES):
            image = cells[row][column]
            bounds = image.getchannel("A").getbbox()
            if bounds is None:
                raise ValueError("missing control glyph")
            native = base.quantize(base.fit(image.crop(bounds), (20, 20), (18, 18)), palette_cache)
            sheet.paste(native, (row * 20, 0))
            sprites.append({"state": state, "direction": "none", "frame": 0, "part": "body", "tile_variant": 0,
                            "anchor": [0, 0], "box": [row * 20, 0, 20, 20], "source_cell": [column, row],
                            "source_box": boxes[row * len(selected) + column]})
        path = base.OUTPUT / (selection["asset_id"].replace(".", "-") + "-authored.png")
        encoded = png_bytes(sheet)
        files[path] = encoded
        outputs.append({"asset_id": selection["asset_id"], "path": path.relative_to(base.ROOT).as_posix(),
                        "sha256": hashlib.sha256(encoded).hexdigest(), "source": source["path"], "source_sha256": source["sha256"],
                        "size": [60, 20], "native_cell": [20, 20], "anchor": [0, 0], "sprites": sprites})
    return {"_documentation": "Owner: r2mlvs13-graphics-and-asset-pipeline.md Group78B; native control source, runtime pending",
            "schema_version": 1, "source_record": RECORD.relative_to(base.ROOT).as_posix(), "runtime_activated": False,
            "pose_count": len(selected) * 3, "atlases": outputs, "world_preview_reuse": document["world_preview_reuse"]}, files


def bind_manifest(report, verified):
    """Publish staged or verified source coverage without touching packed IDs.

    @param report Actual native control coverage and source-derived byte hashes.
    @param verified True only after the explicit focused source check has passed.
    @return None; update only selected source/authoring fields in the public file.
    """
    public_path = base.ASSETS.parent / "multilayer_sprite_manifest.json"
    public = json.loads(public_path.read_text(encoding="utf-8"))
    source = json.loads(RECORD.read_text(encoding="utf-8"))["source"]
    # A low-power bake does not silently claim test completion. The source path
    # is real, while coverage/verification stays pending until the focused burst.
    for atlas in report["atlases"]:
        family = next(row for row in public["assets"] if row["asset_id"] == atlas["asset_id"])
        family["source"] = {"status": "authored" if verified else "authored_partial", "path": source["path"],
                            "sha256": source["sha256"], "provenance": "first_party_original_builtin_image_gen",
                            "rights": "first_party_generated_original"}
        family["authoring"] = {"date": "2026-10-07", "status": "source_coverage_verified_runtime_pending" if verified else "source_staged_verification_deferred_low_power",
                               "source_record": report["source_record"], "originals": [source["path"]],
                               "atlas_manifest": REPORT.relative_to(base.ROOT).as_posix(), "native_atlas": atlas["path"],
                               "native_sha256": atlas["sha256"], "staged_pose_count": len(atlas["sprites"]),
                               "coverage_complete": verified, "runtime_activated": False}
    public_path.write_text(json.dumps(public, indent=2) + "\n", encoding="utf-8")


def publish(report, files):
    """Write staged native glyphs and review while leaving verification pending.

    @param report Derived seven-family/three-state source coverage.
    @param files Exact native indexed PNG bytes mapped to repository outputs.
    @return None; bake review/native artifacts and mark their public status staged.
    """
    for path, encoded in files.items():
        path.write_bytes(encoded)
    REPORT.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    make_review(report, False)
    bind_manifest(report, False)


def make_review(report, verified):
    """Render native and enlarged glyphs with an accurate source-review status.

    @param report Selected control atlas paths and native state records.
    @param verified Whether focused source checks have actually passed.
    @return None; update the review image only, keeping native source pixels exact.
    """
    review = Image.new("RGB", (748, len(report["atlases"]) * 142 + 38), (44, 47, 51))
    draw = ImageDraw.Draw(review)
    status = "source checks passed; runtime pending" if verified else "verification deferred"
    draw.text((12, 10), "M1 ground controls: enabled / disabled-refusal / pressed; 20x20 + enlarged; " + status, fill="white")
    # Native glyphs and enlarged exact pixels share a row so readability and
    # source-state differences can be inspected without starting a game or test.
    for index, atlas in enumerate(report["atlases"]):
        image = Image.open(base.repository_path(atlas["path"])).convert("RGBA")
        y = index * 142 + 38
        draw.text((12, y + 6), atlas["asset_id"], fill="white")
        review.paste(image, (12, y + 28), image)
        # The review uses 5x instead of changing the glyph's native footprint.
        large = image.resize((300, 100), Image.Resampling.NEAREST)
        review.paste(large, (112, y + 28), large)
    review.save(REVIEW)


def validate_glyphs(image, atlas):
    """Validate independent native glyph identities, bounds and palette policy.

    @param image Indexed source atlas supplied by the producer or corruption fixture.
    @param atlas Declared three native control poses and their anchor/cell geometry.
    @return None; reject invalid geometry, missing states or forbidden colors.
    """
    if image.mode != "P" or image.info.get("transparency") != 0 or image.getpalette() != base.PALETTE_BYTES:
        # DOS alpha/color ownership remains identical for active/refused/selected
        # UI states; another palette cannot redefine their visual semantics.
        raise ValueError("ground control palette/transparency mismatch")
    if set(image.get_flattened_data()) & (base.FORBIDDEN - {0}):
        raise ValueError("ground control uses reserved palette colors")
    identities = [(row["state"], row["frame"], row["direction"]) for row in atlas["sprites"]]
    if identities != [(state, 0, "none") for state in STATES]:
        raise ValueError("missing or duplicated native ground control state")
    poses = []
    # Validate each actual recorded cell rather than assuming a perfect source
    # strip; out-of-image crops can otherwise manufacture transparent UI art.
    for row in atlas["sprites"]:
        x, y, width, height = row["box"]
        if [width, height] != [20, 20] or row["anchor"] != [0, 0] or not (0 <= x < x + width <= image.width and 0 <= y < y + height <= image.height):
            # A control retains its real native canvas/anchor inside the atlas.
            raise ValueError("ground control native bounds/anchor mismatch")
        poses.append(image.crop((x, y, x + width, y + height)))
    if any(pose.getbbox() is None for pose in poses) or len({pose.tobytes() for pose in poses}) != 3:
        # Large source-state highlights must survive native reduction; copied
        # active glyphs cannot impersonate refused or selected controls.
        raise ValueError("missing or duplicated native control state")


def check(report, files):
    """Validate complete native control coverage in a separate focused burst.

    @param report Fresh source-derived coverage expected from retained originals.
    @param files Exact expected indexed PNG bytes for deterministic regeneration.
    @return None; raise ValueError on incomplete, stale or invalid native controls.
    """
    if json.loads(REPORT.read_text(encoding="utf-8")) != report:
        raise ValueError("ground control source report changed")
    # Each control has all three genuinely different source states; native
    # indexed colors/alpha and fixed anchors must match the shared UI contract.
    for atlas in report["atlases"]:
        path = base.repository_path(atlas["path"])
        if path.read_bytes() != files[path]:
            raise ValueError("ground control conversion is not reproducible")
        image = Image.open(path)
        validate_glyphs(image, atlas)
    print("Ground control source conversion: 7 families / 21 native states; source coverage, palette, anchors and distinct states pass; runtime pending")


def main():
    """Bake sources or explicitly verify them after the user resumes testing.

    @return None; print staged or verified status without conflating the two.
    """
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--check", action="store_true", help="focused read-only verification")
    parser.add_argument("--verify-and-bind", action="store_true", help="verify then publish completed source coverage")
    args = parser.parse_args()
    report, files = derive()
    if args.check or args.verify_and_bind:
        # Verification is an explicit operation, never a side effect of baking
        # during the user-requested low-power development window.
        check(report, files)
        if args.verify_and_bind:
            bind_manifest(report, True)
            make_review(report, True)
            record = json.loads(RECORD.read_text(encoding="utf-8"))
            record["verification"]["status"] = "source_conversion_verified_runtime_pending"
            RECORD.write_text(json.dumps(record, indent=2) + "\n", encoding="utf-8")
    else:
        publish(report, files)
        print("Ground controls staged: 7 families / 21 poses; focused verification and runtime pending")


if __name__ == "__main__":
    main()
