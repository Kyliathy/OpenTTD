"""Draw reference-only native nozzle geometry for genuine rocket engine originals.

@file cargo_rocket_effect_guides.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group62F.
@see docs/zz-reach2/upgrades/2026-10/r2mlvs13-graphics-and-asset-pipeline.md source extraction rules.
These wires/labels are geometry inputs only, never gameplay art. Rebuild from
media/multilayer with python -m tools.cargo_rocket_effect_guides; output is under
logs/proofs/mlm/native-art/space/cargo-rocket-engine-v1/guides/.
"""

import json

from PIL import Image, ImageDraw

from tools import cargo_rocket_art as art
from tools import cargo_rocket_states as states

OUTPUT = art.ROOT / "logs/proofs/mlm/native-art/space/cargo-rocket-engine-v1/guides"


def draw_frame(frame):
    """Draw the fixed physical mouth and bounded pulse length at one reference scale.

    @param frame Declared0long/narrow or1short/broad genuine engine pulse.
    @return Exact template identity and source/native physical reference geometry.
    """
    image = Image.new("RGBA", (1024, 1024), (0, 0, 0, 0))
    draw = ImageDraw.Draw(image)
    draw.text((32, 24), "FRAME"+str(frame)+" / GEOMETRY ONLY / NO GUIDE BECOMES ART", fill="white")
    draw.text((32, 48), "CIRCULAR EMITTER AT CAMERA30deg: PROJECTED ELLIPSE2:1", fill="white")
    draw.rectangle((272, 224, 800, 896), outline="#8d9da8", width=3)
    draw.ellipse((320, 224, 704, 416), outline="#50dce7", width=5)
    tip_y = 800 if frame == 0 else 704
    draw.line((320, 320, 512, tip_y, 704, 320), fill="#eaaa65", width=4)
    draw.line((488, 320, 536, 320), fill="#ee6798", width=3)
    draw.line((512, 296, 512, 344), fill="#ee6798", width=3)
    draw.text((32, 960), "MOUTH CENTER512,320 -> NATIVE32,34 / UNIFORM48SOURCE PX PER NATIVE PX", fill="white")
    path = OUTPUT / ("engine-frame"+str(frame)+"-template.png")
    image.save(path)
    return dict(frame=frame, image=art.source.identity(path), source_mouth_center=[512, 320],
                source_emitter_ellipse=[320, 224, 704, 416], source_tail_tip=[512, tip_y],
                native_mount=[32, 34], native_envelope=[27, 32, 38, 46],
                role="physical emitter/contact/camera/pulse geometry only; never original effect pixels")


def rebuild():
    """Rebuild both declared pulse templates without authoring or selecting any effect.

    @return Bound reference layout report; no source or runtime completion claim.
    """
    states.checked_policy(art.read_json(states.POLICY))
    OUTPUT.mkdir(parents=True, exist_ok=True)
    # Both genuine pulse originals share the actual same nozzle mounting and
    # 2:1 emitter plane; only their native-clock cosmetic flame shape may vary.
    report = dict(_documentation=__doc__, policy=art.source.identity(states.POLICY),
                  frames=[draw_frame(frame) for frame in (0, 1)],
                  source_coverage=False, packed=False, runtime=False)
    (OUTPUT / "geometry-guides.json").write_text(json.dumps(report, indent=2)+"\n", encoding="utf-8")
    return report


if __name__ == "__main__":
    rebuild()
