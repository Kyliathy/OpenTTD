"""Convert selected M1 terrain details from immutable authored originals.

@file convert_terrain_art.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs13-graphics-and-asset-pipeline.md Groups 10-11 and 79
@see docs/zz-reach2/upgrades/2026-10/r2mlvs13-b-m1-authoring.md
Only deterministic crop, native resampling and DOS-palette conversion happen
here. Built-in image generation owns raster authoring; runtime activation is
reserved for the later owning integration groups.
"""

import argparse
import hashlib
import io
import json

from PIL import Image, ImageDraw

import convert_m1_art as base


RECORD = base.ASSETS / "terrain-generation-record.json"
REPORT = base.OUTPUT / "terrain-details-manifest.json"
REVIEW = base.ASSETS / "terrain-details-review.png"
STATES = ["unknown", "surveyed", "depleted"]
RESOURCES = {"underground": ["COAL", "IORE", "CORE", "BAUX", "SALT"],
             "deep": ["LITH", "NORE", "COBT", "RARE", "URAN"]}


def png_bytes(image):
    """Encode one deterministic indexed atlas without a filesystem mutation.

    @param image DOS-palette atlas with index zero reserved for transparency.
    @return Exact PNG bytes used for publication and reproducibility checks.
    """
    output = io.BytesIO()
    image.save(output, format="PNG", transparency=0, optimize=False, compress_level=9)
    return output.getvalue()


def derive():
    """Prepare all declared supplemental terrain sprites from retained pixels.

    @return Source document, derived manifest and path-to-PNG-byte mapping.
    @throws ValueError If originals, references or declared source coverage disagree.
    """
    document = json.loads(RECORD.read_text(encoding="utf-8"))
    required = {f"{depth}_{variant}" for depth in RESOURCES for variant in ("edges", "survey")}
    identities = [source["key"] for source in document["sources"]]
    if set(identities) != required or len(identities) != len(required):
        # Complete terrain handoff requires both geological depths and their
        # resource overlays; duplicates cannot impersonate a missing source.
        raise ValueError("missing or duplicate terrain original family")
    outputs = []
    files = {}
    cache = {}
    # Each retained source is verified before downstream conversion can select its
    # cells. A changed reference cannot silently change a source's provenance.
    for source in document["sources"]:
        path = base.repository_path(source["path"])
        if base.digest(path) != source["sha256"]:
            raise ValueError(f"immutable terrain source changed: {path}")
        for reference in source["references"]:
            if base.digest(base.repository_path(reference["path"])) != reference["sha256"]:
                raise ValueError("terrain reference hash mismatch")
        original = Image.open(path).convert("RGBA")
        if original.getchannel("A").getextrema()[0] != 0:
            raise ValueError("terrain original has no transparent alpha")
        cells, boxes = base.source_cells(source)
        depth, variant = source["key"].split("_", 1)
        # Edges cover every combination of adjacent solid neighbors. Survey
        # icons instead preserve each resource's three information states.
        if variant == "edges":
            if source["grid"] != [4, 4]:
                raise ValueError("missing sixteen neighbor masks")
            entries = [{"asset_id": f"terrain.edges.{depth}", "state": f"mask_{mask:02x}",
                        "neighbor_mask": mask, "direction": "none", "frame": 0,
                        "source_cell": [mask % 4, mask // 4],
                        "pixels": base.terrain(cells[mask // 4][mask % 4])}
                       for mask in range(16)]
            size, grid, anchor = (64, 31), (4, 4), [-31, 0]
        elif variant == "survey":
            if source["grid"] != [5, 3]:
                raise ValueError("missing five-resource three-state survey layout")
            if source["resources"] != RESOURCES[depth] or source["states"] != STATES:
                # Stable cargo meanings and disclosure states own this source
                # selection; column order cannot reveal another deposit type.
                raise ValueError("survey identities or information states changed")
            entries = [{"asset_id": f"survey.{label}", "state": state,
                        "direction": "none", "frame": 0, "source_cell": [column, row],
                        "pixels": base.fit(cells[row][column], (20, 20), (18, 18))}
                       for row, state in enumerate(STATES)
                       for column, label in enumerate(RESOURCES[depth])]
            size, grid, anchor = (20, 20), (5, 3), [0, 0]
        else:
            raise ValueError(f"unknown terrain detail variant: {variant}")
        sheet = Image.new("P", (size[0] * grid[0], size[1] * grid[1]), 0)
        sheet.putpalette(base.PALETTE_BYTES)
        sprites = []
        # Fixed canvases and common anchors preserve joins and prevent state
        # changes from shifting the resource marker or a bordering ground tile.
        for index, entry in enumerate(entries):
            pixels = base.quantize(entry.pop("pixels"), cache)
            x, y = index % grid[0] * size[0], index // grid[0] * size[1]
            sheet.paste(pixels, (x, y))
            sprites.append({**entry, "source": source["key"], "anchor": anchor,
                            "box": [x, y, *size]})
        output_path = base.OUTPUT / f"terrain-{depth}-{variant}.png"
        encoded = png_bytes(sheet)
        files[output_path] = encoded
        outputs.append({"path": output_path.relative_to(base.ROOT).as_posix(),
                        "sha256": hashlib.sha256(encoded).hexdigest(),
                        "source": source["path"], "source_boxes": boxes,
                        "size": list(sheet.size), "native_cell": list(size),
                        "palette": "dos_8bpp", "transparent_index": 0,
                        "sprites": sprites})
    report = {"_documentation": "Owner: r2mlvs13-graphics-and-asset-pipeline.md Groups 10-11; conversion: media/multilayer/convert_terrain_art.py",
              "schema_version": 1, "source_record": RECORD.relative_to(base.ROOT).as_posix(),
              "runtime_activated": False, "atlases": outputs,
              "pose_count": sum(len(row["sprites"]) for row in outputs),
              "mask_bits": ["screen_upper_left", "screen_upper_right", "screen_lower_right", "screen_lower_left"]}
    return document, report, files


def publish(report, files):
    """Write derived atlases and a labeled native-size review contact sheet.

    @param report Exact source/state/anchor coverage produced by derive.
    @param files Repository output paths mapped to deterministic PNG bytes.
    @return None; publish or verify the declared repository-owned source artifacts.
    """
    for path, data in files.items():
        path.write_bytes(data)
    REPORT.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    bind_manifest(report)
    width = max(row["size"][0] * 4 for row in report["atlases"]) + 24
    height = sum(row["size"][1] * 5 + 52 for row in report["atlases"]) + 12
    review = Image.new("RGB", (width, height), (44, 47, 51))
    draw = ImageDraw.Draw(review)
    y = 12
    # Both actual native size and nearest-neighbor enlargement expose unreadable
    # silhouettes, missing edges and accidental anchor drift before integration.
    for row in report["atlases"]:
        image = Image.open(base.ROOT / row["path"]).convert("RGBA")
        draw.text((12, y), row["path"].rsplit("/", 1)[-1], fill="white")
        review.paste(image, (12, y + 18), image)
        large = image.resize((image.width * 4, image.height * 4), Image.Resampling.NEAREST)
        review.paste(large, (12, y + image.height + 30), large)
        y += row["size"][1] * 5 + 52
    review.save(REVIEW)


def bind_manifest(report):
    """Bind authored terrain details to existing public families without activation.

    @param report Native atlas coverage and retained source references from derive.
    @return None; publish or verify the declared repository-owned source artifacts.
    """
    path = base.ASSETS.parent / "multilayer_sprite_manifest.json"
    manifest = json.loads(path.read_text(encoding="utf-8"))
    sources = {row["path"]: row for row in json.loads(RECORD.read_text(encoding="utf-8"))["sources"]}
    # One marker sheet serves five already declared meanings; their rows retain
    # individual coverage rather than introducing a second resource registry.
    for output in report["atlases"]:
        source = sources[output["source"]]
        for identity in sorted({sprite["asset_id"] for sprite in output["sprites"]}):
            row = next(row for row in manifest["assets"] if row["asset_id"] == identity)
            poses = [sprite for sprite in output["sprites"] if sprite["asset_id"] == identity]
            if {sprite["state"] for sprite in poses} != set(row["states"]):
                raise ValueError(f"incomplete source states for {identity}")
            row["source"] = {"status": "authored", "path": source["path"], "sha256": source["sha256"],
                             "provenance": "first_party_original_builtin_image_gen", "rights": "first_party_generated_original"}
            row["authoring"] = {"date": source["date"], "status": "source_coverage_verified_runtime_pending",
                                "source_record": report["source_record"], "originals": [source["path"]],
                                "atlas_manifest": REPORT.relative_to(base.ROOT).as_posix(),
                                "native_atlas": output["path"], "native_sha256": output["sha256"],
                                "staged_pose_count": len(poses), "runtime_activated": False,
                                "coverage_complete": True}
    path.write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")


def check(expected, files):
    """Validate exact reproducibility, coverage, alpha, palette and native bounds.

    @param expected Manifest freshly derived from the declared immutable sources.
    @param files Expected encoded atlases, regenerated without writes.
    @throws ValueError On changed bytes or invalid functional sprite coverage.
    @return None; publish or verify the declared repository-owned source artifacts.
    """
    if json.loads(REPORT.read_text(encoding="utf-8")) != expected:
        raise ValueError("terrain detail manifest disagrees with immutable sources")
    manifest = json.loads((base.ASSETS.parent / "multilayer_sprite_manifest.json").read_text(encoding="utf-8"))
    families = {row["asset_id"]: row for row in manifest["assets"]}
    # Each source state must remain a real nonempty authored pose. Comparing all
    # bytes also detects palette, alpha and crop changes hidden by similar previews.
    for row in expected["atlases"]:
        path = base.repository_path(row["path"])
        if path.read_bytes() != files[path]:
            raise ValueError(f"terrain conversion is not reproducible: {path}")
        image = Image.open(path)
        if image.mode != "P" or image.getpalette() != base.PALETTE_BYTES or image.info.get("transparency") != 0:
            # Bundled terrain must share the native palette and alpha convention
            # without introducing animation or company-color semantics.
            raise ValueError("terrain palette/transparency mismatch")
        if set(image.get_flattened_data()) & (base.FORBIDDEN - {0}):
            raise ValueError("terrain uses reserved palette indices")
        signatures = set()
        for sprite in row["sprites"]:
            family = families[sprite["asset_id"]]
            if sprite["anchor"] != family["anchor"] or row["native_cell"] != family["native_size"]:
                # The public world's contact point must remain authoritative
                # when pixels move from source sheets to native rendering.
                raise ValueError("terrain geometry disagrees with public family")
            x, y, width, height = sprite["box"]
            pose = image.crop((x, y, x + width, y + height))
            if not any(pose.get_flattened_data()):
                raise ValueError("empty terrain detail pose")
            if any(pose.getpixel(corner) for corner in [(0, 0), (width - 1, 0), (0, height - 1), (width - 1, height - 1)]):
                raise ValueError("opaque terrain canvas corner")
            signatures.add(pose.tobytes())
        if row["native_cell"] == [64, 31] and len(signatures) != 16:
            # Every distinct solid-neighbor combination needs its own visible
            # rim; duplicate masks would close open joins or hide real rock.
            raise ValueError("missing distinct neighboring edge poses")
    print(f"Terrain detail conversion: {len(expected['atlases'])} atlases / {expected['pose_count']} poses; exact regeneration, alpha, palette, coverage and bounds pass; runtime pending")


def main():
    """Publish or verify the focused terrain source handoff.

    @return None; report evidence and fail on invalid retained or derived art.
    """
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--check", action="store_true", help="verify exact derivation without writes")
    args = parser.parse_args()
    _document, report, files = derive()
    if not args.check:
        publish(report, files)
    check(report, files)


if __name__ == "__main__":
    main()
