"""Convert retained M1 portal housings into aligned endpoint back/front sprites.

@file convert_portal_art.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs13-graphics-and-asset-pipeline.md Groups 12-13 and 79
@see docs/zz-reach2/upgrades/2026-10/r2mlvs3-sealed-portals-and-deep-rail.md
This deterministic source handoff preserves the physical renderer's existing
sorting/mask contract. It neither activates payloads nor changes saved phases.
"""

import argparse
import hashlib
import json

from PIL import Image, ImageDraw

import convert_m1_art as base
from convert_terrain_art import png_bytes
from sprite_quantize import front_mask


RECORD = base.ASSETS / "portal-generation-record.json"
REPORT = base.OUTPUT / "portal-details-manifest.json"
REVIEW = base.ASSETS / "portal-details-review.png"
DOORS = ["NE", "SE", "SW", "NW"]
STATES = ["sealed", "commissioning", "connected"]


def native_housing(image):
    """Fit an authored tile to its fixed south contact without inventing pixels.

    @param image One authored source cell with its aligned tile footing.
    @return RGBA 64x64 canvas whose floor tip meets the native south corner.
    @throws ValueError If the full one-tile housing exceeds the native height.
    """
    height = round(image.height * 64 / image.width)
    if height > 64:
        raise ValueError("portal housing exceeds its one-tile native canvas")
    scaled = image.resize((64, height), Image.Resampling.BOX)
    canvas = Image.new("RGBA", (64, 64))
    canvas.paste(scaled, (0, 64 - height))
    return canvas


def derive():
    """Derive static phases and physical endpoint parts from immutable sources.

    @return Derived coverage manifest and exact path-to-PNG-byte mapping.
    @throws ValueError If source hashes, phase names or world geometry disagree.
    """
    document = json.loads(RECORD.read_text(encoding="utf-8"))
    public = json.loads((base.ASSETS.parent / "multilayer_sprite_manifest.json").read_text(encoding="utf-8"))
    families = {row["asset_id"]: row for row in public["assets"]}
    outputs, files, cache = [], {}, {}
    identities = [row["asset_id"] for row in document["sources"]]
    if len(identities) != len(set(identities)):
        raise ValueError("duplicate portal source family")
    # Each source contributes both independent world-layer endpoints. Their
    # upper/lower rows never become a fabricated adjacent two-tile building.
    for source in document["sources"]:
        if base.digest(base.repository_path(source["path"])) != source["sha256"]:
            raise ValueError("immutable portal original changed")
        for reference in source["references"]:
            if base.digest(base.repository_path(reference["path"])) != reference["sha256"]:
                raise ValueError("immutable portal reference changed")
        family = families[source["asset_id"]]
        if source["endpoint_variants"] != ["upper", "lower"] or family["footprint_semantics"] != "paired_endpoint_tiles" or family["tile_variants"] != 2:
            # Pair coverage means two separate physical endpoint tiles on their
            # actual layers; an adjacent multi-tile interpretation changes sorting.
            raise ValueError("portal source disagrees with physical pair coverage")
        if source["grid"] != [4, 6] or source["states"] != STATES or family["states"] != STATES:
            # The actual saved owner supplies exactly these three phases; draft
            # generic states must not silently authorize a different source map.
            raise ValueError("portal source disagrees with real work phases")
        if family["directions"] != DOORS or family["frames"] != 1 or family["anchor"] != [-31, -33]:
            # Static housings require the four real door views and world north
            # corner shared by both sprite parts and by the native renderer.
            raise ValueError("portal direction/frame/anchor contract mismatch")
        original = Image.open(base.repository_path(source["path"])).convert("RGBA")
        if original.getchannel("A").getextrema()[0] != 0:
            raise ValueError("portal original has no transparent alpha")
        cells, boxes = base.source_cells(source)
        sheet = Image.new("P", (8 * 64, 6 * 64), 0)
        sheet.putpalette(base.PALETTE_BYTES)
        sprites = []
        # Back/front come from one aligned housing. Far fronts cover the entire
        # opaque roof/wall; near fronts use the established world-edge cut plane.
        for endpoint, variant in enumerate(["upper", "lower"]):
            for phase, state in enumerate(STATES):
                row = endpoint * 3 + phase
                for door, direction in enumerate(DOORS):
                    back = base.quantize(native_housing(cells[row][door]), cache)
                    keep = front_mask(door, 1, 64, 64, (31, 33), full_span=True)
                    front = back.copy()
                    front.putdata([value if (index % 64, index // 64) in keep else 0
                                   for index, value in enumerate(back.get_flattened_data())])
                    # Painted darkness belongs to the rear interior, not an
                    # opaque wall across a usable doorway. Source-reviewed
                    # aperture masks keep the jamb/roof pixels and clear only
                    # the actual near opening; sealed/far coverage is retained.
                    aperture = source["front_apertures"][variant].get(state, {}).get(direction)
                    if aperture is not None:
                        ImageDraw.Draw(front).polygon([tuple(point) for point in aperture], fill=0)
                    for part, pixels in [("back", back), ("front", front)]:
                        x, y = (door * 2 + (part == "front")) * 64, row * 64
                        sheet.paste(pixels, (x, y))
                        sprites.append({"asset_id": source["asset_id"], "endpoint": variant,
                                        "tile_variant": endpoint, "state": state, "direction": direction,
                                        "part": part, "frame": 0, "anchor": [-31, -33],
                                        "source": source["key"], "source_cell": [door, row], "box": [x, y, 64, 64]})
        path = base.OUTPUT / f"{source['key']}-authored.png"
        encoded = png_bytes(sheet)
        files[path] = encoded
        outputs.append({"asset_id": source["asset_id"], "path": path.relative_to(base.ROOT).as_posix(),
                        "source": source["path"], "source_sha256": source["sha256"], "source_boxes": boxes,
                        "sha256": hashlib.sha256(encoded).hexdigest(), "size": list(sheet.size),
                        "native_cell": [64, 64], "sprites": sprites})
    report = {"_documentation": "Owner: r2mlvs13-graphics-and-asset-pipeline.md Groups 12-13; DrawTile_LayerPortal owns runtime sorting",
              "schema_version": 1, "runtime_activated": False,
              "source_record": RECORD.relative_to(base.ROOT).as_posix(), "atlases": outputs,
              "pose_count": sum(len(row["sprites"]) for row in outputs)}
    return report, files


def publish(report, files):
    """Publish reproducible source handoff and native/enlarged review imagery.

    @param report Exact endpoint/state/direction/part source coverage.
    @param files Derived atlas paths mapped to their encoded bytes.
    @return None; publish or verify the declared repository-owned source artifacts.
    """
    for path, data in files.items():
        path.write_bytes(data)
    REPORT.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    bind_manifest(report)
    review = Image.new("RGB", (1048, len(report["atlases"]) * 1200), (44, 47, 51))
    draw = ImageDraw.Draw(review)
    # The native and 2x views show all four approaches with each endpoint/phase;
    # real vehicle occlusion remains a later native renderer acceptance check.
    for index, row in enumerate(report["atlases"]):
        image = Image.open(base.ROOT / row["path"]).convert("RGBA")
        y = index * 1200
        draw.text((12, y + 8), row["asset_id"] + ": paired back/front NE SE SW NW; upper/lower x sealed/commissioning/connected", fill="white")
        review.paste(image, (12, y + 28), image)
        large = image.resize((1024, 768), Image.Resampling.NEAREST)
        review.paste(large, (12, y + 420), large)
    review.save(REVIEW)


def bind_manifest(report):
    """Publish verified source coverage without changing packed IDs or payloads.

    @param report Exact real-phase endpoint/part atlas coverage from derive.
    @return None; publish or verify the declared repository-owned source artifacts.
    """
    path = base.ASSETS.parent / "multilayer_sprite_manifest.json"
    manifest = json.loads(path.read_text(encoding="utf-8"))
    sources = {row["asset_id"]: row for row in json.loads(RECORD.read_text(encoding="utf-8"))["sources"]}
    # Each declared building retains its public packing ownership and untouched
    # original candidate; this binds a separately selected complete source handoff.
    for output in report["atlases"]:
        row = next(row for row in manifest["assets"] if row["asset_id"] == output["asset_id"])
        source = sources[output["asset_id"]]
        original_paths = list(dict.fromkeys(row.get("authoring", {}).get("originals", []) + [source["path"]]))
        row["source"] = {"status": "authored", "path": source["path"], "sha256": source["sha256"],
                         "provenance": "first_party_original_builtin_image_gen", "rights": "first_party_generated_original"}
        row["authoring"] = {"date": source["date"], "status": "source_phase_and_part_coverage_verified_runtime_pending",
                            "source_record": report["source_record"], "originals": original_paths,
                            "atlas_manifest": REPORT.relative_to(base.ROOT).as_posix(),
                            "native_atlas": output["path"], "native_sha256": output["sha256"],
                            "staged_pose_count": len(output["sprites"]), "runtime_activated": False,
                            "coverage_complete": True}
    path.write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")


def check(expected, files):
    """Verify exact regeneration, native masks and complete phase/part coverage.

    @param expected Fresh source-derived atlas manifest.
    @param files Exact expected encoded atlas bytes.
    @throws ValueError If any published coverage or pixel invariant is lost.
    @return None; publish or verify the declared repository-owned source artifacts.
    """
    if json.loads(REPORT.read_text(encoding="utf-8")) != expected:
        raise ValueError("portal source manifest changed")
    # Compare source-derived bytes, then the physical mask contract independently;
    # an opaque far wall cannot be replaced by a near-mouth frame or empty part.
    for row in expected["atlases"]:
        path = base.repository_path(row["path"])
        if path.read_bytes() != files[path]:
            raise ValueError("portal conversion is not reproducible")
        image = Image.open(path)
        if image.mode != "P" or image.info.get("transparency") != 0 or image.getpalette() != base.PALETTE_BYTES:
            # Native building art uses the established DOS palette/alpha policy.
            raise ValueError("portal palette/transparency mismatch")
        if set(image.get_flattened_data()) & (base.FORBIDDEN - {0}):
            raise ValueError("portal uses reserved palette colors")
        signatures = {}
        for sprite in row["sprites"]:
            x, y, width, height = sprite["box"]
            pose = image.crop((x, y, x + width, y + height))
            if not any(pose.get_flattened_data()):
                raise ValueError("empty portal part")
            key = (sprite["endpoint"], sprite["state"], sprite["direction"], sprite["part"])
            if key in signatures:
                raise ValueError("duplicate portal coverage")
            signatures[key] = pose.tobytes()
        if len(signatures) != 48:
            raise ValueError("missing portal endpoint/phase/direction/part")
        for endpoint in ["upper", "lower"]:
            for state in STATES:
                for door in ["NE", "NW"]:
                    if signatures[(endpoint, state, door, "front")] != signatures[(endpoint, state, door, "back")]:
                        raise ValueError("far portal roof/wall loses opaque coverage")
                for door, point in [("SE", (42, 42)), ("SW", (22, 42))]:
                    if state == "sealed":
                        continue
                    front = signatures[(endpoint, state, door, "front")]
                    back = signatures[(endpoint, state, door, "back")]
                    position = point[1] * 64 + point[0]
                    if front[position] != 0 or back[position] == 0:
                        # A real open mouth must expose vehicles through the
                        # foreground while keeping its shaded rear interior.
                        raise ValueError("near portal opening is opaque or unregistered")
    print(f"Portal source conversion: {len(expected['atlases'])} families / {expected['pose_count']} endpoint parts; exact regeneration, phase coverage, palette and far-wall masks pass; runtime pending")


def main():
    """Publish or read-only check the focused native portal source handoff.

    @return None; print verification evidence and fail on invalid source art.
    """
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--check", action="store_true", help="verify exact conversion without writing")
    args = parser.parse_args()
    report, files = derive()
    if not args.check:
        publish(report, files)
    check(report, files)


if __name__ == "__main__":
    main()
