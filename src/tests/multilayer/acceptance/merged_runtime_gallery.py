"""Capture exact retained visible worlds through the actual merged renderer.

@file merged_runtime_gallery.py Original poses and provenance stay distinct from current drawing.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md
"""
import json
import os
from pathlib import Path
import re
import subprocess
import sys
import uuid
from PIL import Image

ROOT = Path(__file__).resolve().parents[4]
sys.path.insert(0, str(ROOT / "src/tests/multilayer/support"))
from artifact_paths import create_run_context
from artifact_rebuild import identity
from native_build import verify_source_epoch
from normal_ground_merge import checked, read, CONTENT_SHA
from runner import run_native, save_native_capture
from savegame_tables import compare_merged_worlds

ORIGINALS = "src/tests/fixtures/checkpoints/underground/openttd3-75ea75718f/runtime-gallery-originals.json"


def select_capsule(name, root=ROOT):
    """Resolve an already-visible actual original shipment, without earning or assigning another pose.

    @param name Mole, Crate, Cistern, Shield or Forge original selection.
    @param root Checkout containing preserved original receipts and saves.
    @return Exact source and independently sealed original model/view/classification.
    """
    row = json.loads((root / ORIGINALS).read_text())["capsules"][name]
    accepted = read(row["historical_acceptance"], root)
    original = read(row["historical_raster"], root)
    # A retained shipment must be the independently sealed original pose and
    # model. Substituting another visible save would sever its paid provenance.
    if row["original_view"]["model"] != row["model"] or original["view"]["before"] != row["original_source"] or row["source"]["sha256"] != row["original_source"]["sha256"]:
        raise ValueError("original capsule pose/source provenance differs")
    if row["classification"] != accepted["fixture_classification"]:
        raise ValueError("original capsule setup classification differs")
    checked(row["source"], root)
    return dict(name=name, kind="capsule", origin="B", **row)


def execute(build_reference, selection, root=ROOT):
    """Draw one exact ordinary original and compare all merged owners around the raster boundary.

    @param build_reference Current selected ground viewport executable/build/source epoch.
    @param selection Explicit source, kind, original lineage and optional capsule model.
    @param root Checkout containing retained inputs and the current renderer.
    @return Native render/PNG/picking/full128-family purity report identity.
    """
    build = read(build_reference, root)
    epoch = read(build["source_epoch"], root)
    verify_source_epoch(epoch, root, build["tool_profile"]["path"])
    source = selection["source"]
    context = create_run_context("multilayer-art", "merged-runtime-gallery", run_id="merge69-" + selection["name"] + "-" + uuid.uuid4().hex[:10], root=root)
    context.prepare_runtime()
    grf = context.runtime_dir / "baseset/openttd.grf"
    if identity(grf, root)["sha256"] != CONTENT_SHA:
        raise ValueError("gallery uses different published canonical graphics")
    originals = json.loads((root / "src/tests/fixtures/saveload/lineage-originals.json").read_text())["observations"]
    bootstrap = next(row for row in originals if row["origin"] == "B" and row["version"] == 427)
    bootstrap_reference = {key: bootstrap[key] for key in ("path", "sha256")}
    (context.runtime_dir / "scripts/game_start.scr").write_text("merged_gallery\n")
    environment = {key: value for key, value in os.environ.items() if not key.startswith("OTTD_")}
    environment.update(OTTD_TEST_CONFIGURATION=build["configuration"], OTTD_GALLERY_SOURCE=str(checked(source, root)),
        OTTD_GALLERY_DIGEST=source["sha256"], OTTD_GALLERY_ORIGIN=selection["origin"], OTTD_GALLERY_KIND=selection["kind"],
        OTTD_GALLERY_BOOTSTRAP=str(checked(bootstrap_reference, root)))
    if "model" in selection:
        environment["OTTD_GALLERY_MODEL"] = str(selection["model"])
    image = build["targets"]["openttd_ground_viewport_test"]
    terminal, stdout = context.evidence / "native.txt", context.evidence / "stdout.txt"
    with stdout.open("wb") as sink:
        run_native(context, [str(checked(image, root)), "[.merged-runtime-gallery]", "--reporter", "compact", "--out", str(terminal)],
            cwd=root / "build", env=environment, stdout=sink, stderr=subprocess.STDOUT, timeout=60, check=True)
    actual = terminal.read_text(encoding="utf-8", errors="replace")
    count = re.search(r"Passed 1 test case with (\d+) assertions?", actual)
    if count is None:
        raise ValueError("gallery native case did not pass")
    families = compare_merged_worlds(context.saves / "before.sav", context.saves / "after.sav")
    compare_merged_worlds(context.saves / "before.sav", context.saves / "before-repeat.sav")
    compare_merged_worlds(context.saves / "after.sav", context.saves / "after-repeat.sav")
    outputs = []
    # Every complete native repeated raster must match before lossless container
    # conversion. A copied frame or an atlas cannot replace this actual drawing.
    for first in sorted(context.images.glob("*-0.ppm")):
        repeated = first.with_name(first.name[:-6] + "-1.ppm")
        with Image.open(first) as original, Image.open(repeated) as other:
            # Both dimensions and all native pixels must repeat; a cropped
            # viewport cannot qualify unchanged drawing or selection geometry.
            if original.size != other.size or original.tobytes() != other.tobytes():
                raise ValueError("native repeated gallery pixels differ: " + first.name)
            target = first.with_suffix(".png")
            save_native_capture(context, original, first, target)
        outputs.append(dict(native=identity(first, root), repeated=identity(repeated, root), png=identity(target, root)))
    if not outputs:
        raise ValueError("gallery drew no native frames")
    checked(source, root); checked(image, root)
    verify_source_epoch(epoch, root, build["tool_profile"]["path"])
    processes = list(context.reports.glob("native-*.json"))
    if len(processes) != 1:
        raise ValueError("gallery requires one exact owned native process")
    report = context.reports / "runtime-gallery.json"
    report.write_text(json.dumps(dict(_documentation=__doc__, complete=True, kind="actual_native_merged_runtime_gallery",
        build=build_reference, source=selection, runtime_grf=identity(grf, root), process=identity(processes[0], root),
        terminal=identity(terminal, root), stdout=identity(stdout, root), image=image, native_cases=1,
        native_assertions=int(count[1]), frames=outputs, compared_families=families, simulation_ticks=0,
        pose_supplied=False, human_acceptance=False,
        saves={path.stem: identity(path, root) for path in sorted(context.saves.glob("*.sav"))}), indent=2) + "\n")
    return identity(report, root)
