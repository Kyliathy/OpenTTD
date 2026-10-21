"""Review eight genuine ready views against the four unchanged launch-source cuts.

@file cargo_rocket_ready_fit.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group62D.
@see docs/zz-reach2/upgrades/2026-10/r2mlsp-space-master-plan.md Plan9task16.2.
All scenes are source-only diagrams; no actual craft, custody or game draw is added.
Run from media/multilayer with python -m tools.cargo_rocket_ready_fit.
"""

import hashlib
import json

from PIL import Image, ImageDraw

from tools import cargo_rocket_art as art
from tools import cargo_rocket_directions as directions

OUTPUT = directions.OUTPUT / "ready-fit"
FIXTURE = art.ROOT / "src/tests/multilayer/cargo-rocket-ready-fit-fixture.json"
FACILITY_HEADINGS = ("NE", "SE", "SW", "NW")
ZONES = ("pad", "workshop", "recovery")


def checked_ready_inputs(model, document):
    """Require the complete genuine ready subset before four-view source fit review.

    @param model Actual ready-direction descriptor or detached refusal input.
    @param document Exact Space source owner or detached refusal input.
    @return None.
    @throws ValueError On missing genuine headings or changed source/public owners.
    """
    directions.checked_model(model, document)
    # Four selected facility-facing diagonals cannot waive missing cardinal
    # rocket headings; this checkpoint follows all eight genuine originals.
    if set(model["selected"]) != set(art.DIRECTIONS) or not model["genuine_ready_views_complete"]:
        raise ValueError("complete eight-heading rocket ready source required")
    art.checked_predecessor(document, art.read_json(art.PUBLIC))
    art.checked_model(art.read_json(art.MODEL))


def observations():
    """Measure each matching genuine rocket inside the original physical zone order.

    @return Per-view fit observations, exact source evidence and RGB source diagrams.
    """
    document = art.source.read_record()
    model = art.read_json(directions.MODEL)
    checked_ready_inputs(model, document)
    source_model = art.read_json(art.MODEL)
    fit_model = art.read_json(art.ROOT / source_model["launch_fit_reference"]["path"])
    results, scenes = {}, {}
    # Each facility view uses the corresponding actual rocket heading, never
    # the NE reference rotated/mirrored or a diagnostic model15 substitute.
    for heading in FACILITY_HEADINGS:
        actor, evidence = directions.direction_view(heading, document, model)
        fixture = art.occupied.compose_states(fit_model, heading)
        entries = []
        # A separate scene per real rocket-handling zone avoids creating three
        # simultaneous hulls or moving native custody to an illustration mount.
        for zone in ZONES:
            scene, entry = art.fit_scene(actor, fit_model, heading, zone, fixture)
            scenes[(heading, zone)] = scene
            entries.append(entry)
        results[heading] = dict(rocket_heading=heading, native_pixels_sha256=evidence["native_pixels_sha256"],
                                foreground_sha256=fixture[4]["foreground_sha256"],
                                whole_ready_sha256=hashlib.sha256(fixture[0]["ready"].tobytes()).hexdigest(), zones=entries)
    return results, scenes


def rebuild():
    """Save reproducible four-view source-fit diagrams without changing any original.

    @return Bound fit report; complete phase/packed/game evidence remains false.
    """
    results, scenes = observations()
    OUTPUT.mkdir(parents=True, exist_ok=True)
    artifacts = []
    # Aspect-preserving native x2 panels expose physical occlusion as authored;
    # a closed roof may hide a whole stored hull without inventing a reveal cut.
    for heading in FACILITY_HEADINGS:
        contact = Image.new("RGB", (1280, 992), "#29323c")
        draw = ImageDraw.Draw(contact)
        for index, zone in enumerate(ZONES):
            x, y = index % 2 * 640, index // 2 * 496
            draw.text((x+8, y+8), heading+" "+zone+" / corresponding genuine rocket / SOURCE FIT ONLY", fill="white")
            contact.paste(scenes[(heading, zone)].resize((640, 448), Image.Resampling.NEAREST), (x, y+40))
        path = OUTPUT / (heading.lower()+"-source-fit.png")
        contact.save(path)
        artifacts.append(art.source.identity(path))
    model = art.read_json(art.MODEL)
    report = dict(_documentation=__doc__, direction_model=art.source.identity(directions.MODEL),
                  facility_model=model["launch_fit_reference"], matching_genuine_headings=list(FACILITY_HEADINGS),
                  observations=results, artifacts=artifacts, ready_source_fit_only=True,
                  source_coverage=False, packed=False, runtime=False)
    (OUTPUT / "source-fit.json").write_text(json.dumps(report, indent=2)+"\n", encoding="utf-8")
    return report


if __name__ == "__main__":
    rebuild()
