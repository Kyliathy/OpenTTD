"""Review retained original capsule views against native train geometry.

@file convert_tube_capsules.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs13-graphics-and-asset-pipeline.md Groups49-53.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Only explicit original crops/registration and palette conversion create source pixels.
"""

import argparse
import json

from PIL import Image, ImageDraw

import convert_tube_art as tube
import convert_vehicle_art as vehicles


VECTORS = {"N": (0, -16), "NE": (16, -8), "E": (32, 0), "SE": (16, 8),
           "S": (0, 16), "SW": (-16, 8), "W": (-32, 0), "NW": (-16, -8)}


def native_pose(candidate, direction=None):
    """Resample an explicit original view onto its fixed native canvas.

    @param candidate Checked retained original with measured native source crops.
    @param direction Actual view for a multi-view original; omitted for a single view.
    @return Indexed64x48 source pose and explicit source registration.
    """
    if direction is None:
        direction = candidate.get("direction")
    if direction not in VECTORS:
        raise ValueError("native capsule source requires an actual explicit view")
    if direction in candidate.get("excluded_views", []):
        raise ValueError("known excluded capsule source view")
    # A multi-view original can supply only its explicitly registered cells;
    # another frame or a correctly named but absent heading is never fallback art.
    if "views" in candidate and direction not in candidate["views"]:
        raise ValueError("missing actual capsule original view")
    transform = candidate if "views" not in candidate else candidate["views"][direction]
    public = json.loads(tube.PUBLIC.read_text(encoding="utf-8"))
    family = next((row for row in public["assets"] if row["asset_id"] == candidate["asset_id"]), None)
    # A crop must name this actual original view. Relabelling another heading
    # or allowing arbitrary translations could conceal missing geometry.
    # The tested physical producer covers canonical capsule models; ordinary
    # and orbital public families cannot acquire this native geometry merely
    # by supplying a plausible image/canvas or copying a view label.
    if (transform.get("direction", candidate.get("direction")) != direction or family is None
            or family["consumer"] != 17 or family["kind"] != "vehicle"
            or family["native_size"] != [64, 48] or family["anchor"] != [-32, -32]):
        raise ValueError("native capsule source view/model mismatch")
    image = Image.open(tube.base.repository_path(candidate["path"])).convert("RGBA")
    left, top, right, bottom = transform["native_crop"]
    if not (0 <= left < right <= image.width and 0 <= top < bottom <= image.height):
        raise ValueError("native capsule crop leaves original")
    original = image.crop((left, top, right, bottom))
    if original.getchannel("A").getbbox() is None:
        raise ValueError("empty native capsule original view")
    maximum = transform.get("native_maximum", vehicles.ENVELOPES[direction])
    # The low capsule keeps the real full-length projected chassis: narrower
    # diagonal/end envelopes preserve its physical proportions without growing
    # to the maximum safe rectangle used by taller conventional locomotives.
    if len(maximum) != 2 or any(value <= 0 or value > bound for value, bound in zip(maximum, vehicles.ENVELOPES[direction])):
        raise ValueError("native capsule maximum exceeds its physical envelope")
    subject_size = transform.get("native_subject_size")
    if subject_size is None:
        fitted = tube.base.fit(original, (64, 48), maximum)
    else:
        # Each independent original has an explicitly reviewed projected
        # chassis/roof rectangle. Source-view sampling corrects its scale to
        # the fixed native projection; it never rotates/mirrors a raster or
        # supplies a missing front/rear face from another heading.
        if len(subject_size) != 2 or any(value <= 0 or value > bound for value, bound in zip(subject_size, maximum)):
            raise ValueError("native capsule subject exceeds its declared view envelope")
        fitted = Image.new("RGBA", (64, 48))
        fitted.paste(original.resize(tuple(subject_size), Image.Resampling.BOX), ((64 - subject_size[0]) // 2, 40 - subject_size[1]))
    dx, dy = transform["native_translation"]
    if abs(dx) > 4 or abs(dy) > 8:
        raise ValueError("native capsule registration leaves its contact envelope")
    canvas = Image.new("RGBA", (64, 48))
    canvas.paste(fitted, (dx, dy))
    pixels = tube.base.quantize(canvas, {})
    return pixels, {"direction": direction, "native_crop": [left, top, right, bottom], "native_translation": [dx, dy],
                    "native_maximum": list(maximum), "native_subject_size": subject_size}


def review_candidate(key, direction=None):
    """Expose native scale, projected chassis contacts and adjacent view spacing.

    @param key Exact immutable source key from the shared ground provenance record.
    @param direction Explicit actual heading for a multi-view source original.
    @return Source-only review report path; no complete family/runtime claim is made.
    """
    candidate = tube.retained_candidate(key)
    pixels, transform = native_pose(candidate, direction)
    direction = transform["direction"]
    native_path = tube.REVIEW / f"{key}-{direction}-native.png"
    native_path.parent.mkdir(parents=True, exist_ok=True)
    native_path.write_bytes(tube.indexed_png(pixels))
    board = Image.new("RGB", (1000, 640), (48, 52, 58))
    draw = ImageDraw.Draw(board)
    draw.text((12, 12), f"{key}: true {direction}; native64x48 anchor(-32,-32); candidate only", fill="white")
    image = pixels.convert("RGBA")
    board.paste(image.resize((512, 384), Image.Resampling.NEAREST), (20, 50), image.resize((512, 384), Image.Resampling.NEAREST))
    board.paste(image, (600, 150), image)
    # Native projected full-length spacing shows alpha overlap separately from
    # the chassis line. These guides never become game art or alter movement.
    dx, dy = VECTORS[direction]
    for offset in (-1, 0, 1):
        x, y = 600 + offset * dx, 370 + offset * dy
        board.paste(image, (x, y), image)
    draw.line((632 - dx, 402 - dy, 632 + dx, 402 + dy), fill=(45, 200, 220), width=1)
    draw.text((550, 440), "1x original poses; cyan projected chassis axis is proof only", fill="white")
    review_path = tube.REVIEW / f"{key}-{direction}-contact.png"
    board.save(review_path)
    report = {"_documentation": "Plan13 Groups49-53; archi-map-layers.md. Original capsule base/view review, complete source/packing/runtime pending.",
              "asset_id": candidate["asset_id"], "source": candidate["path"], "source_sha256": candidate["sha256"],
              "native_transform": transform, "native_size": [64, 48], "anchor": [-32, -32],
              "native_bounds": list(pixels.getbbox()), "native_path": native_path.relative_to(tube.ROOT).as_posix(),
              "contact_path": review_path.relative_to(tube.ROOT).as_posix(), "source_coverage_complete": False,
              "runtime_activated": False}
    target = tube.REVIEW / f"{key}-{direction}-review.json"
    target.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    return target


def review_views(asset_id="vehicle.model-09"):
    """Present all eight selected original model views before load/status authoring.

    @param asset_id Public capsule family whose eight genuine base views are selected.
    @return Native base-view report, explicitly incomplete for full state/phase coverage.
    """
    document = json.loads(tube.RECORD.read_text(encoding="utf-8"))
    matches = [row for row in document["families"] if row["asset_id"] == asset_id]
    if len(matches) != 1:
        raise ValueError("missing or duplicate actual capsule source family")
    family = matches[0]
    name = family["name"].split()[0]
    stem = name.split("-")[0].lower()
    # Artifact names remain model-owned plain words inside the fixed proof
    # directory. A source display name cannot become an arbitrary output path.
    if not stem.isascii() or not stem.isalpha():
        raise ValueError("invalid native capsule proof name")
    required = len(family["states"]) * family["frames"] * len(family["directions"]) * len(family["parts"])
    selections = family["selections"]
    # Every selected heading needs its own genuine original. Eight rows with
    # duplicated headings or a service/load state cannot establish base geometry.
    if (len(selections) != 8 or {row["direction"] for row in selections} != set(vehicles.DIRECTIONS)
            or any(row["state"] != "empty" for row in selections)):
        raise ValueError("incomplete actual capsule base view coverage")
    atlas = Image.new("P", (512, 48), 0)
    atlas.putpalette(tube.base.PALETTE_BYTES)
    board = Image.new("RGB", (2048, 410), (48, 52, 58))
    draw = ImageDraw.Draw(board)
    draw.text((12, 12), f"{name}: eight genuine empty model views; source-only geometry,{required}-pose load/status/purchase handoff is separate", fill="white")
    sprites = []
    # Stable actual engine view order makes adjacent facings/lighting and the
    # unequal true front/rear end faces visible before any state component exists.
    for index, direction in enumerate(vehicles.DIRECTIONS):
        selection = next(row for row in selections if row["direction"] == direction)
        candidate = tube.retained_candidate(selection["source"], document)
        if candidate["status"] not in ("selected", "partially_selected"):
            raise ValueError("unselected native capsule original")
        pixels, transform = native_pose(candidate, direction)
        atlas.paste(pixels, (index * 64, 0))
        image = pixels.convert("RGBA")
        enlarged = image.resize((256, 192), Image.Resampling.NEAREST)
        board.paste(enlarged, (index * 256, 70), enlarged)
        board.paste(image, (index * 256 + 96, 300), image)
        draw.text((index * 256 + 112, 45), direction, fill="white")
        sprites.append({"state": "empty", "direction": direction, "frame": 0, "part": "body",
                        "box": [index * 64, 0, 64, 48], "anchor": [-32, -32],
                        "source": candidate["path"], "source_sha256": candidate["sha256"],
                        "native_transform": transform, "native_bounds": list(pixels.getbbox())})
    atlas_path = tube.REVIEW / f"{stem}-empty-views.png"
    contact_path = tube.REVIEW / f"{stem}-empty-views-contact.png"
    atlas_path.write_bytes(tube.indexed_png(atlas))
    board.save(contact_path)
    report = {"_documentation": "Plan13 Groups49-53; archi-map-layers.md. Complete eight-direction base geometry only; full source/purchase/packing/runtime separate.",
              "asset_id": asset_id, "path": atlas_path.relative_to(tube.ROOT).as_posix(),
              "contact_path": contact_path.relative_to(tube.ROOT).as_posix(), "sprites": sprites,
              "staged_pose_count": 8, "required_pose_count": required,
              "source_coverage_complete": False, "runtime_activated": False}
    target = tube.REVIEW / f"{stem}-empty-views-manifest.json"
    target.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    return target


def main():
    """Review one explicit source view without inventing missing model coverage.

    @return None; print the generated native candidate report path.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    modes = parser.add_mutually_exclusive_group(required=True)
    modes.add_argument("--candidate", help="Exact retained original source key")
    modes.add_argument("--model-views", action="store_true", help="Review all eight selected actual model views")
    parser.add_argument("--direction", choices=vehicles.DIRECTIONS, help="Required actual view for a multi-view original")
    parser.add_argument("--asset", help="Explicit public family for --model-views; candidate identity comes from its own original")
    args = parser.parse_args()
    if args.model_views:
        if args.direction is not None:
            parser.error("a direction selector requires one candidate")
        print(review_views(args.asset or "vehicle.model-09"))
    else:
        if args.asset is not None:
            parser.error("an asset selector requires --model-views")
        print(review_candidate(args.candidate, args.direction))


if __name__ == "__main__":
    main()
