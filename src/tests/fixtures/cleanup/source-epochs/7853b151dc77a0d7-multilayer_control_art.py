"""Focused native ground-control state and source-corruption fixtures.

@file multilayer_control_art.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs13-graphics-and-asset-pipeline.md Group 78B
@see docs/zz-reach2/upgrades/2026-10/r2mlvs3mp-master-plan.md checkpoint 8.5
Fixtures are read-only: corrupt independent in-memory copies, never originals.
"""

import copy
import json
from pathlib import Path
import sys

from PIL import Image

sys.dont_write_bytecode = True
ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "media/multilayer"))
import convert_control_art as art


def main():
    """Verify all seven actual control glyphs and independent refusal cases.

    @return None; print native source evidence or fail on accepted corruption.
    """
    report, files = art.derive()
    art.check(report, files)
    public = json.loads((art.base.ASSETS.parent / "multilayer_sprite_manifest.json").read_text(encoding="utf-8"))
    for atlas in report["atlases"]:
        family = next(row for row in public["assets"] if row["asset_id"] == atlas["asset_id"])
        assert family["authoring"]["native_sha256"] == atlas["sha256"]
        assert family["authoring"]["staged_pose_count"] == 3
        assert family["authoring"]["runtime_activated"] is False
    first = report["atlases"][0]
    image = Image.open(art.base.repository_path(first["path"]))
    # Each mutation reaches a distinct production guard. A missing state is not
    # inferred from a hash mismatch; bounds and palette policy are checked on
    # the supplied native records/pixels themselves without filesystem writes.
    for mutation, message in [("missing", "missing or duplicated native ground"),
                              ("bounds", "native bounds/anchor"), ("palette", "reserved palette")]:
        invalid, pixels = copy.deepcopy(first), image.copy()
        if mutation == "missing":
            invalid["sprites"].pop()
        elif mutation == "bounds":
            invalid["sprites"][0]["box"][0] = image.width - 1
        else:
            illegal = min(art.base.FORBIDDEN - {0})
            pixels.putpixel((10, 10), illegal)
        try:
            art.validate_glyphs(pixels, invalid)
        except ValueError as error:
            assert message in str(error), "corruption reached the wrong refusal guard"
            continue
        raise AssertionError("native control corruption was accepted: " + mutation)
    print("Ground control fixtures: 7 families / 21 poses; public source bindings and missing-state/bad-bounds/illegal-color refusals pass")


if __name__ == "__main__":
    main()
