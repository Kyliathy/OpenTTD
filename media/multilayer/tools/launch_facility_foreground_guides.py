"""Review occupied source contacts without moving actual native launch custody.

@file launch_facility_foreground_guides.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group61EA.
@see docs/zz-reach2/upgrades/2026-10/r2mlsp-z-z-z-z-z-q-launch-center-source.md.
Bounds are diagnostic rectangles from the public separate-actor source canvas,
not actor art or a gameplay capture. Retained facility originals stay unchanged.
Run with python -m tools.launch_facility_foreground_guides from media/multilayer
to rebuild the explicit ignored foreground-guide outputs.
"""

import json

from PIL import Image, ImageDraw

from tools import launch_facility_base as base

ROOT = base.ROOT
MODEL = ROOT / "media/multilayer/source/space/launch-center-foreground-model-v1.json"
OUTPUT = ROOT / "logs/proofs/mlm/native-art/space/launch-center-foreground-v1"
COLOURS = {"pad": "#ff8060", "workshop": "#50e0ff", "recovery": "#80ed90", "storage": "#f6df58"}


def project_cell(native, cell):
    """Project an actual world-cell center through the existing viewport basis.

    @param native Actual source geometry with verified native floor origin.
    @param cell Real rotated world cell inside this facility footprint.
    @return Native screen contact for that cell center, without world mutation.
    """
    x, y = cell
    ox, oy = native["floor_origin"]
    return [ox + 32 * (y - x), oy + 16 * (x + y) + 16]


def component_detail(component, selected, direction):
    """Enlarge exact isolated source pixels for physical near-surface selection.

    @param component Existing workshop, storage or pad component name.
    @param selected Exact reviewed original receipt/registration selection.
    @param direction Actual genuine view whose physical surfaces are inspected.
    @return Diagnostic PNG identity; no source crop or pixel changes are published.
    """
    candidate = base.bound_json(selected["generation_receipt"])
    plane, registration = base.source.registered_component(candidate)
    x0, y0, x1, y1 = registration["native_subject_bounds"]
    crop = plane.crop((x0, y0, x1, y1))
    background = Image.new("RGB", crop.size, "#29323c")
    mask = Image.new("L", crop.size)
    mask.putdata([255 if value else 0 for value in crop.get_flattened_data()])
    background.paste(crop.convert("RGB"), (0, 0), mask)
    enlarged = background.resize((crop.width * 8, crop.height * 8), Image.Resampling.NEAREST)
    guide = Image.new("RGB", (enlarged.width + 48, enlarged.height + 64), "#29323c")
    guide.paste(enlarged, (40, 48))
    labels = ImageDraw.Draw(guide)
    labels.text((8, 8), f"{direction} {component}: exact isolated indexed pixels x8; bounds [{x0},{y0},{x1},{y1})", fill="white")
    # Native coordinate rulers support deliberate physical-face polygons.
    # They sit outside the enlarged subject and never become source artwork.
    for x in range((x0 + 9) // 10 * 10, x1, 10):
        px = 40 + (x - x0) * 8
        labels.text((px - 6, 28), str(x), fill="white")
    for y in range((y0 + 9) // 10 * 10, y1, 10):
        py = 48 + (y - y0) * 8
        labels.text((8, py), str(y), fill="white")
    path = OUTPUT / f"{direction.lower()}-{component}-surface-detail.png"
    guide.save(path)
    return base.identity(path)


def write_guides(model):
    """Show native custody and physical source mounting separately in every genuine view.

    @param model Explicit source contact brief whose foreground regions remain pending.
    @return Exact output identities for source review, never a foreground acceptance receipt.
    """
    selection = base.bound_json(model["selected_bases"])
    OUTPUT.mkdir(parents=True, exist_ok=True)
    artifacts = []
    # Each equipment zone gets an independent panel, avoiding overlapping
    # diagnostic rectangles that could obscure the original physical surfaces.
    for direction, view in model["views"].items():
        recipe = base.bound_json(selection["selected_views"][direction])
        component_model = base.bound_json(recipe["component_model"])
        selected = component_model["selected_components"] if direction == "NE" else component_model["selected_components"][direction]
        whole, owners, cells, evidence = base.compose(recipe)
        sheet = Image.new("RGB", (1280, 992), "#29323c")
        labels = ImageDraw.Draw(sheet)
        for index, component in enumerate(COLOURS):
            contact = view["contacts"][component]
            panel = base.geometry_review(whole, direction)
            pen = ImageDraw.Draw(panel)
            physical = contact["physical_display_contact"]
            custody = contact["native_custody_contact"]
            colour = COLOURS[component]
            pen.rectangle(contact["declared_actor_canvas_box"], outline=colour)
            pen.line([tuple(custody), tuple(physical)], fill="white")
            pen.ellipse((custody[0] - 2, custody[1] - 2, custody[0] + 2, custody[1] + 2), outline="white")
            pen.line([(physical[0] - 3, physical[1]), (physical[0] + 3, physical[1])], fill=colour)
            pen.line([(physical[0], physical[1] - 3), (physical[0], physical[1] + 3)], fill=colour)
            x, y = index % 2 * 640, index // 2 * 496
            sheet.paste(panel.resize((640, 448), Image.Resampling.NEAREST), (x, y + 40))
            labels.text((x + 10, y + 8), f"{direction} {component}: diagnostic source canvas; no actor pixels", fill=colour)
            labels.text((x + 10, y + 23), f"white custody {custody}; coloured physical contact {physical}; regions pending", fill="white")
            if component != "recovery":
                artifacts.append(component_detail(component, selected[component], direction))
        path = OUTPUT / f"{direction.lower()}-occupied-contact-guide.png"
        sheet.save(path)
        artifacts.append(base.identity(path))
    document = dict(_documentation="r2mlvs14-remaining-assets-and-runtime-integration.md Group61EA.1; diagnostic contacts only, foreground regions/cuts not qualified.",
                    model=base.identity(MODEL), artifacts=artifacts,
                    occupied_foregrounds_complete=False, source_coverage=False, packed=False, runtime=False)
    (OUTPUT / "contact-guides.json").write_text(json.dumps(document, indent=2) + "\n", encoding="utf-8")
    return artifacts


def main():
    """Rebuild only explicit diagnostic source contacts from the curated brief.

    @return None; original source images, native custody and plan flags stay unchanged.
    """
    write_guides(json.loads(MODEL.read_text(encoding="utf-8")))


if __name__ == "__main__":
    main()
