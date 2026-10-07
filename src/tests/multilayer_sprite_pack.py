"""Focused selected-M1 source range, crop and corruption fixtures.

@file multilayer_sprite_pack.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs13-graphics-and-asset-pipeline.md Groups79B/80B
@see docs/zz-reach2/upgrades/2026-10/r2mlvs3mp-master-plan.md checkpoint8.6
Read-only source fixtures; actual GRF loading is tested separately by its owner.
"""

import copy
import json
from pathlib import Path
import sys

from PIL import Image

sys.dont_write_bytecode = True
ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "media/multilayer"))
import pack_m1_art as art


def verify_source_refusals(public):
    """Exercise distinct source-state, native-bounds and palette refusals.

    @param public Actual public source contract with completed source verification.
    @return None; fail if a corrupted in-memory source input is accepted.
    """
    family = next(row for row in public["assets"] if row["asset_id"] == "vehicle.model-00")
    path = family["authoring"]["atlas_manifest"]
    document = json.loads(art.base.repository_path(path).read_text(encoding="utf-8"))
    # Report corruption reaches exact Cartesian/bounds checks against genuine
    # native source bytes; no changed hash or test-owned dummy bitmap hides it.
    for mutation, message in [("missing", "incomplete source coverage"), ("bounds", "source cell leaves atlas")]:
        invalid = copy.deepcopy(document)
        atlas = next(row for row in invalid["atlases"] if row["path"] == family["authoring"]["native_atlas"])
        if mutation == "missing":
            atlas["sprites"].pop()
        else:
            atlas["sprites"][0]["box"][0] = atlas["size"][0] - 1
        try:
            art.source_poses(family, {path: invalid}, {})
        except ValueError as error:
            assert message in str(error), "corruption hit a different guard"
            continue
        raise AssertionError("packed source corruption accepted: " + mutation)
    image = Image.open(art.base.repository_path(family["authoring"]["native_atlas"])).copy()
    image.putpixel((0, 0), min(art.base.FORBIDDEN - {0}))
    try:
        art.validate_source_image(image, family["asset_id"])
    except ValueError as error:
        assert "reserved source palette" in str(error)
        return
    raise AssertionError("illegal packed-source palette index accepted")


def main():
    """Check selected native layout and source-to-cropped-anchor round trips.

    @return None; print exact selected counts or fail a physical/source invariant.
    """
    public = json.loads(art.PUBLIC.read_text(encoding="utf-8"))
    scope = json.loads(art.SCOPE.read_text(encoding="utf-8"))
    ranges, bound = art.selected_ranges(public, scope)
    assert len(ranges) == 40
    assert sum(row["sprite_count"] for row in ranges) == 2311
    assert bound == 2404
    verify_source_refusals(public)
    families = {row["asset_id"]: row for row in public["assets"]}
    documents, images = {}, {}
    # Every source crop is reconstructed at its original canvas anchor. The
    # identity of opaque/transparent pixels proves cropping cannot move doors,
    # world contacts or UI fittings, including legitimate foreground holes.
    for row in ranges:
        family = families[row["asset_id"]]
        poses, image = art.source_poses(family, documents, images)
        for pose in poses.values():
            pixels, width, height, rx, ry = art.cropped_pose(image, pose)
            restored = Image.new("P", family["native_size"], 0)
            cropped = Image.new("P", (width, height), 0)
            cropped.putdata(pixels)
            restored.paste(cropped, (rx - family["anchor"][0], ry - family["anchor"][1]))
            x, y, sw, sh = pose["box"]
            assert restored.tobytes() == image.crop((x, y, x + sw, y + sh)).tobytes()
    print("Selected M1 pack fixtures: 40 families / 2311 exact source poses; stable native ranges, crop/anchor round trips and source corruption refusals pass")


if __name__ == "__main__":
    main()
