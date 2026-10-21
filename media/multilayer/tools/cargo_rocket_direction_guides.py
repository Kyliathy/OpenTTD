"""Build labelled engine-heading templates for genuine rocket yaw originals.

@file cargo_rocket_direction_guides.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group62.2.
@see docs/zz-reach2/upgrades/2026-10/r2mlvs13-graphics-and-asset-pipeline.md Framed ImageGen sheets.
Geometry, labels and arrows are references only and never become source artwork.
Run from media/multilayer with python -m tools.cargo_rocket_direction_guides.
"""

import json
import math

from PIL import Image, ImageDraw

from tools import cargo_rocket_art as art

OUTPUT = art.ROOT / "logs/proofs/mlm/native-art/space/cargo-rocket-directions-v1"
CONTRACT = art.ROOT / "media/multilayer/source/space/cargo-rocket-directions-v1.json"
FRAMES = ((24, 24, 744, 488), (792, 24, 1512, 488),
          (24, 536, 744, 1000), (792, 536, 1512, 1000))
CROPS = ((32, 32, 736, 480), (800, 32, 1504, 480),
         (32, 544, 736, 992), (800, 544, 1504, 992))
VIEWS = {
    "N": (-45, "service/access face centered on front visible hull"),
    "NE": (0, "original service/access face on screen-right visible hull"),
    "E": (45, "service/access face at right tangent, nearly edge-on"),
    "SE": (90, "service/access face hidden on rear-right; visible hull plain"),
    "S": (135, "service/access face hidden on rear; visible hull plain"),
    "SW": (180, "service/access face hidden on rear-left; visible hull plain"),
    "W": (225, "service/access face at left tangent, nearly edge-on"),
    "NW": (270, "same service/access face on screen-left visible hull"),
}


def write_template(name, directions):
    """Draw a four-cell canonical frame with fixed source contact/body envelopes.

    @param name Explicit template basename inside the rocket proof directory.
    @param directions Four actual engine headings, ordered top-left to bottom-right.
    @return Exact template identity plus measured layout-reference rectangles.
    """
    image = Image.new("RGBA", (1536, 1024), (0, 0, 0, 0))
    draw = ImageDraw.Draw(image)
    slots = []
    # Each label supplies one physical yaw relative to the selected NE hull.
    # Magenta frames separate original views; cyan body/contact guides are
    # layout-only and must be absent from the generated transparent cutouts.
    for direction, frame, crop in zip(directions, FRAMES, CROPS):
        left, top, right, bottom = frame
        origin = (left+104, top+20)
        contact = (origin[0]+32*8, origin[1]+32*8)
        draw.rectangle(frame, outline="#ff00ff", width=6)
        draw.text((left+12, top+10), direction + " / yaw " + str(VIEWS[direction][0]), fill="white")
        draw.rectangle((origin[0]+16*8, origin[1]+2*8, origin[0]+48*8, origin[1]+38*8), outline="#5adce4", width=2)
        draw.line((contact[0]-24, contact[1], contact[0]+24, contact[1]), fill="#ee6798", width=2)
        draw.line((contact[0], contact[1]-24, contact[0], contact[1]+24), fill="#ee6798", width=2)
        # The small arrow labels the engine viewport heading only. The upright
        # rocket yaws around its vertical axis; it must not tilt along this arrow.
        angle = math.radians(art.DIRECTIONS.index(direction)*45-90)
        point = (right-62, bottom-62)
        endpoint = (point[0]+round(math.cos(angle)*30), point[1]+round(math.sin(angle)*30))
        draw.line((*point, *endpoint), fill="white", width=3)
        draw.ellipse((endpoint[0]-3, endpoint[1]-3, endpoint[0]+3, endpoint[1]+3), fill="white")
        draw.text((left+12, bottom-24), "GROUND CONTACT32,32 / GUIDE ONLY", fill="#ee6798")
        slots.append(dict(direction=direction, frame=list(frame), requested_crop=list(crop),
                          contact=list(contact), yaw_from_ne=VIEWS[direction][0], physical_face=VIEWS[direction][1]))
    OUTPUT.mkdir(parents=True, exist_ok=True)
    path = OUTPUT / (name+".png")
    image.save(path)
    return dict(image=art.source.identity(path), slots=slots)


def rebuild():
    """Rebuild the two explicit heading guides without modifying source selections.

    @return Guide report; retained descriptor selection is never overwritten.
    """
    model = art.read_json(art.MODEL)
    art.checked_model(model)
    report = dict(_documentation=__doc__, source_model=art.source.identity(art.MODEL),
                  base_fixture=art.source.identity(art.ROOT / "src/tests/multilayer/cargo-rocket-base-fixture.json"),
                  views={name: dict(yaw_from_ne=value[0], physical_face=value[1]) for name, value in VIEWS.items()},
                  batches=[write_template("n-e-se-s-template", ["N", "E", "SE", "S"]),
                           write_template("sw-w-nw-ne-template", ["SW", "W", "NW", "NE"])],
                  isolated=[write_isolated_template(name) for name in art.DIRECTIONS if name != "NE"],
                  policy="Clockwise physical yaw around upright hull axis relative to original NE; model service face is the yaw landmark, independent of native flight trajectory. Do not rotate/mirror flat pixels or change screen light.",
                  source_coverage=False, packed=False, runtime=False)
    (OUTPUT / "geometry-guides.json").write_text(json.dumps(report, indent=2)+"\n", encoding="utf-8")
    return report


def write_isolated_template(direction):
    """Label a single-view fallback with unchanged physical contact and body bounds.

    @param direction Actual new engine heading from the declared yaw contract.
    @return Exact template identity; all colored guides remain reference-only.
    """
    image = Image.new("RGBA", (800, 800), (0, 0, 0, 0))
    draw = ImageDraw.Draw(image)
    draw.text((24, 24), direction + " / YAW FROM NE " + str(VIEWS[direction][0]) + " / GEOMETRY ONLY", fill="white")
    draw.text((24, 48), VIEWS[direction][1], fill="white")
    ox, oy = 144, 208
    draw.rectangle((ox, oy, ox+64*8, oy+48*8), outline="#9ba6af", width=2)
    draw.rectangle((ox+16*8, oy+2*8, ox+48*8, oy+38*8), outline="#5adce4", width=2)
    draw.line((ox+29*8, oy+32*8, ox+35*8, oy+32*8), fill="#ee6798", width=2)
    draw.line((ox+32*8, oy+29*8, ox+32*8, oy+35*8), fill="#ee6798", width=2)
    draw.text((24, 752), "64x48 / ANCHOR(-32,-32) / FOOT-PLANE CENTER32,32 / NO ARTWORK", fill="white")
    path = OUTPUT / (direction.lower()+"-isolated-template.png")
    image.save(path)
    return dict(direction=direction, image=art.source.identity(path), contact=[400, 464],
                yaw_from_ne=VIEWS[direction][0], physical_face=VIEWS[direction][1])


if __name__ == "__main__":
    rebuild()
