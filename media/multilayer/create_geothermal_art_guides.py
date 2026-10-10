"""Generate native geometry references for original geothermal source models.

@file create_geothermal_art_guides.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs13-graphics-and-asset-pipeline.md Groups54C/55/56.
@see docs/zz-reach2/upgrades/2026-10/r2mlvs18-geothermal-energy-and-industry.md Group8B.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
These labelled blue diagrams are authoring references, never native game artwork.
"""

import argparse
import json
from pathlib import Path

from PIL import Image, ImageDraw


ROOT = Path(__file__).resolve().parents[2]


def create_vent_guide():
    """Register eight visible original phases inside native64x31 ground diamonds.

    @return Path to the four-column/two-row source reference; hidden art stays absent.
    """
    page = Image.new("RGB", (1536, 1024), (242, 246, 250))
    draw = ImageDraw.Draw(page)
    # One reference cell represents one world tile, not another natural source.
    # The first row is information only; the second presupposes paid excavation.
    for row, state in enumerate(("SURVEY INFORMATION", "PAID EXCAVATED NATURAL FISSURE")):
        for frame in range(4):
            x, y = frame * 384, row * 512
            draw.text((x + 12, y + 18), state + " / ORIGINAL PHASE " + str(frame), fill=(25, 45, 75))
            corners = [(x + 192, y + 194), (x + 320, y + 256), (x + 192, y + 318), (x + 64, y + 256)]
            draw.polygon(corners, fill=(231, 239, 248), outline=(45, 105, 180), width=3)
            draw.line((x + 176, y + 256, x + 208, y + 256), fill=(45, 105, 180), width=2)
            draw.line((x + 192, y + 240, x + 192, y + 272), fill=(45, 105, 180), width=2)
            draw.text((x + 24, y + 350), "64x31 / anchor(-31,0) / overlay only", fill=(25, 45, 75))
            draw.text((x + 24, y + 370), "All pixels outside thermal detail are transparent", fill=(25, 45, 75))
    target = ROOT / "media/multilayer/guides/ground/vent-natural-phases-v1.png"
    target.parent.mkdir(parents=True, exist_ok=True)
    page.save(target)
    return target


def create_generator_guide(model):
    """Draw the verified whole-generator floor and native per-cell envelopes.

    @param model VentTap or MagmaLoop source design from the passive native provider.
    @return Path to a labelled geometry reference; no machinery pixels are authored.
    @throws ValueError If the public source row has drifted from the tested native layout.
    """
    names = {"VentTap": "facility.vent-powerhouse", "MagmaLoop": "facility.magma-powerhouse"}
    manifest = json.loads((ROOT / "media/multilayer/contracts/multilayer_sprite_manifest.json").read_text())
    asset = next(row for row in manifest["assets"] if row["asset_id"] == names[model])
    native = asset["geometry_notes"]["native_generator_geometry"]
    cells = [[x, y] for y in range(4) for x in range(4)]
    # The complete native world floor precedes source authoring. A guide must
    # refuse a changed row instead of approving new art against stale geometry.
    if asset["footprint"] != [4, 4] or native["world_cells"] != cells or native["model"] != model:
        raise ValueError("generator guide differs from verified native layout")
    page = Image.new("RGB", (1536, 1024), (242, 246, 250))
    draw = ImageDraw.Draw(page)
    scale, pad_x, pad_y = 4, 128, 64
    draw.text((28, 20), model + " / ACTUAL 4x4 WORLD FLOOR / FIXED 2:1 ISOMETRIC CAMERA", fill=(25, 45, 75))
    # Each diagram diamond is one real world cell of the same future IndustryID.
    # It does not authorize sixteen independently supplied or funded machines.
    for x, y in cells:
        cx, cy = 160 + 32 * (y - x), 80 + 16 * (x + y)
        corners = [(cx, cy - 16), (cx + 32, cy), (cx, cy + 16), (cx - 32, cy)]
        points = [(pad_x + px * scale, pad_y + py * scale) for px, py in corners]
        draw.polygon(points, fill=(231, 239, 248), outline=(45, 105, 180), width=2)
        draw.text((pad_x + cx * scale - 14, pad_y + cy * scale - 6), str(x) + "," + str(y), fill=(25, 45, 75))
    draw.text((28, 894), "Whole native review canvas320x224. Floor256x128; low machinery fits actual64x64 cell envelopes.", fill=(25, 45, 75))
    draw.text((28, 916), "Source base SE: electrical switchgear at world(3,0); heat pipe interface at world(0,3).", fill=(25, 45, 75))
    draw.text((28, 938), "Geometry and labels are references only. Original artwork must have real transparency, no blue grid or text.", fill=(25, 45, 75))
    target = ROOT / ("media/multilayer/guides/ground/generator-" + model.lower() + "-native-v1.png")
    target.parent.mkdir(parents=True, exist_ok=True)
    page.save(target)
    return target


def create_generator_views_guide(model):
    """Place genuine remaining model views in disjoint labelled source regions.

    @param model Verified dated design whose selected SE whole original is the identity reference.
    @return Path to the three-view registration guide; the fourth region is deliberately empty.
    """
    create_generator_guide(model)
    page = Image.new("RGB", (1536, 1024), (242, 246, 250))
    draw = ImageDraw.Draw(page)
    # Quarter turns describe new 3D renders of the same physical design. The
    # source converter later selects their disjoint crops without raster rotation.
    for index, (direction, turn) in enumerate((("NE", 3), ("SW", 1), ("NW", 2))):
        ox, oy = (index % 2) * 768, (index // 2) * 512
        draw.text((ox + 18, oy + 18), model + " / " + direction + " / GENUINE MODEL VIEW", fill=(25, 45, 75))
        for x in range(4):
            for y in range(4):
                cx, cy = ox + 384 + 64 * (y - x), oy + 184 + 32 * (x + y)
                corners = [(cx, cy - 32), (cx + 64, cy), (cx, cy + 32), (cx - 64, cy)]
                draw.polygon(corners, fill=(231, 239, 248), outline=(45, 105, 180), width=2)
        # These two contacts remain opposite physical corners after each
        # model turn; their labels must never be pasted into final source art.
        for x, y, label in ((3, 0, "ELECTRICAL"), (0, 3, "HEAT")):
            for unused in range(turn):
                x, y = 3 - y, x
            cx, cy = ox + 384 + 64 * (y - x), oy + 184 + 32 * (x + y)
            draw.ellipse((cx - 8, cy - 8, cx + 8, cy + 8), fill=(190, 105, 35))
            draw.text((cx + 12, cy - 6), label, fill=(25, 45, 75))
        draw.text((ox + 18, oy + 464), "Same selected model, fixed camera/light, 4x4 floor, complete transparent cutout", fill=(25, 45, 75))
    draw.text((788, 548), "EMPTY: original SE already selected; do not duplicate it", fill=(25, 45, 75))
    target = ROOT / ("media/multilayer/guides/ground/generator-" + model.lower() + "-views-v1.png")
    target.parent.mkdir(parents=True, exist_ok=True)
    page.save(target)
    return target


def create_generator_direction_guide(model, direction):
    """Specify physical equipment placement for one genuine whole-model turn.

    @param model Verified dated source design, retaining its selected SE identity.
    @param direction NE, SE, SW or NW physical orientation under the fixed camera.
    @return Labelled primitive geometry guide; no source-art pixels are modified.
    """
    create_generator_guide(model)
    turn = {"SE": 0, "SW": 1, "NW": 2, "NE": 3}[direction]
    page = Image.new("RGB", (1536, 1024), (242, 246, 250))
    draw = ImageDraw.Draw(page)
    draw.text((28, 20), model + " / TARGET " + direction + " / EQUIPMENT MUST TURN WITH THE MODEL", fill=(25, 45, 75))
    roles = {(0, 0): "HALL", (1, 0): "HALL", (0, 1): "HALL", (1, 1): "HALL",
             (0, 2): "EXCHANGER", (0, 3): "HEAT PORT", (1, 2): "EXCHANGER", (1, 3): "EXCHANGER",
             (2, 1): "TURBINE", (2, 2): "ALTERNATOR", (3, 1): "TURBINE", (3, 2): "ALTERNATOR",
             (3, 0): "SWITCHGEAR"}
    # Label the same logical equipment after each physical model turn. This
    # prevents a plausible unchanged hall from masquerading as a new view.
    for base_y in range(4):
        for base_x in range(4):
            x, y = base_x, base_y
            for unused in range(turn):
                x, y = 3 - y, x
            cx, cy = 768 + 128 * (y - x), 384 + 64 * (x + y)
            corners = [(cx, cy - 64), (cx + 128, cy), (cx, cy + 64), (cx - 128, cy)]
            label = roles.get((base_x, base_y), "ACCESS")
            fill = (217, 232, 247) if label == "HALL" else (239, 231, 212)
            draw.polygon(corners, fill=fill, outline=(45, 105, 180), width=2)
            draw.text((cx - 34, cy - 6), label, fill=(25, 45, 75))
    draw.text((28, 910), "One low hall, one turbine, one alternator, one exchanger, one switchgear. Same selected model.", fill=(25, 45, 75))
    draw.text((28, 934), "Fixed 2:1 camera and upper-left light; true alpha cutout. Labels/blue reference geometry never enter artwork.", fill=(25, 45, 75))
    target = ROOT / ("media/multilayer/guides/ground/generator-" + model.lower() + "-" + direction.lower() + "-v1.png")
    target.parent.mkdir(parents=True, exist_ok=True)
    page.save(target)
    return target


def create_generator_construction_guide(model, stage, bordered=False):
    """Register four construction edits against the already selected model views.

    @param model Verified dated model whose completed hardware supplies the edit targets.
    @param stage Native incomplete presentation stage0,1 or2; final stage3 reuses approved hardware.
    @param bordered Draw magenta frames outside fixed interior source windows, with generous safety gutters.
    @return Labelled four-view source-layout guide with explicit incomplete equipment intent.
    """
    create_generator_guide(model)
    stages = {
        0: "FOUNDATIONS: flat floor, low equipment plinths/conduits, temporary amber site panel; no hall or machinery",
        1: "ASSEMBLY: open hall frame, unconnected machinery shells on plinths, no roof or completed copper loop",
        2: "FIT OUT: partial roof with one open bay, mounted machinery, visibly incomplete return-pipe spool",
    }
    page = Image.new("RGB", (1536, 1024), (242, 246, 250))
    draw = ImageDraw.Draw(page)
    # All four views already exist as genuine whole models. Their construction
    # edits stay in disjoint regions and preserve each actual physical turn.
    for index, (direction, turn) in enumerate((("NE", 3), ("SE", 0), ("SW", 1), ("NW", 2))):
        ox, oy = (index % 2) * 768, (index // 2) * 512
        if bordered:
            draw.rectangle((ox + 24, oy + 24, ox + 744, oy + 488), outline=(255, 0, 255), width=6)
        draw.text((ox + 18, oy + 18), model + " / " + direction + " / NATIVE STAGE " + str(stage), fill=(25, 45, 75))
        for x in range(4):
            for y in range(4):
                cx, cy = ox + 384 + 64 * (y - x), oy + 184 + 32 * (x + y)
                corners = [(cx, cy - 32), (cx + 64, cy), (cx, cy + 32), (cx - 64, cy)]
                draw.polygon(corners, fill=(231, 239, 248), outline=(45, 105, 180), width=2)
        x, y = 3, 0
        # A visible temporary/installed site panel belongs to the same physical
        # equipment corner after each turn, rather than becoming a UI overlay.
        for unused in range(turn):
            x, y = 3 - y, x
        cx, cy = ox + 384 + 64 * (y - x), oy + 184 + 32 * (x + y)
        draw.ellipse((cx - 8, cy - 8, cx + 8, cy + 8), fill=(190, 105, 35))
        draw.text((cx + 12, cy - 6), "AMBER SITE PANEL", fill=(25, 45, 75))
        draw.text((ox + 18, oy + 450), stages[stage], fill=(25, 45, 75))
        draw.text((ox + 18, oy + 474), "Same4x4 floor/camera/light. Complete cutout inside this cell; labels are references only.", fill=(25, 45, 75))
    suffix = "-bordered-v1.png" if bordered else "-v1.png"
    target = ROOT / ("media/multilayer/guides/ground/generator-" + model.lower() + "-construction-" + str(stage) + suffix)
    target.parent.mkdir(parents=True, exist_ok=True)
    page.save(target)
    return target


def main():
    """Write an explicit geometry reference without generating any gameplay art.

    @return None; print the repository guide path.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--generator", choices=("VentTap", "MagmaLoop"), help="Verified dated whole-generator floor")
    parser.add_argument("--views", action="store_true", help="Three genuine remaining views in labelled disjoint regions")
    parser.add_argument("--view", choices=("NE", "SE", "SW", "NW"), help="One model turn with explicit equipment positions")
    parser.add_argument("--construction-stage", type=int, choices=(0, 1, 2), help="Four selected model views at one incomplete native stage")
    parser.add_argument("--bordered", action="store_true", help="Magenta safety frames outside construction source windows")
    arguments = parser.parse_args()
    # A remaining-view layout needs an explicitly verified dated model; the
    # natural vent's information states have no generator rotation semantics.
    if (arguments.views or arguments.view is not None or arguments.construction_stage is not None) and arguments.generator is None:
        parser.error("--views/--view/--construction-stage needs --generator")
    # Single-view and multi-view source briefs are exclusive, so the generated
    # reference cannot silently change meaning for an already retained prompt.
    if sum((arguments.views, arguments.view is not None, arguments.construction_stage is not None)) > 1:
        parser.error("choose --views, --view or --construction-stage")
    # Safety frames describe the construction sheet's fixed four-view crops;
    # silently adding them to a different guide would invalidate its source brief.
    if arguments.bordered and arguments.construction_stage is None:
        parser.error("--bordered needs --construction-stage")
    if arguments.construction_stage is not None:
        print(create_generator_construction_guide(arguments.generator, arguments.construction_stage, arguments.bordered))
        return
    if arguments.view is not None:
        print(create_generator_direction_guide(arguments.generator, arguments.view))
        return
    if arguments.views:
        print(create_generator_views_guide(arguments.generator))
        return
    print(create_generator_guide(arguments.generator) if arguments.generator else create_vent_guide())


if __name__ == "__main__":
    main()
