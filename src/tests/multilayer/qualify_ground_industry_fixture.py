"""Qualify actual current eleven-industry fixture creation and native checkpoint reload.

@file qualify_ground_industry_fixture.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group79FHB.1.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Explicit fixture capital/floor and construction-only scenes qualify transport
of the review scene. Full state/source-pixel/foreground acceptance remains open.
"""

import json

from ground_pack_check_support import ROOT, handoff
from run_ground_consumer_checks import preserved
from tools.ground_payload_binding import checked_reference, reference

RUN = ROOT / "logs/graphics/selected-ground-runtime/20261021-industry-01"
DIRECTORY = ROOT / "logs/proofs/mlm/native-art/ground/ground-consumers-v1"


def scene(proof):
    """Resolve only the exact native frame/checkpoint producer from a successful run.

    @param proof Actual native execution proof with current or retained executable provenance.
    @return Native scene description and exact frame files keyed by owned native scene names.
    """
    observed = proof["observation"]
    files = {row["path"]: checked_reference(row) for row in observed["captures"]}
    metadata = [path for path in files.values() if path.name == "industry-fixture.json"]
    if len(metadata) != 1:
        raise ValueError("native fixture lacks its exact scene metadata")
    document = handoff.read_json(metadata[0].relative_to(ROOT).as_posix())
    images = {path.name: path for path in files.values() if path.suffix in (".ppm", ".idx")}
    return document, images


def qualify():
    """Require both actual creation and replacement-free reload with exact unchanged native scene pixels.

    @return Construction/checkpoint qualification, explicitly excluding full art or economic acceptance.
    """
    construction_path = DIRECTORY / "industry-fixture-construction.json"
    construction = handoff.read_json(construction_path.relative_to(ROOT).as_posix())
    native = []
    # Keep actual source epochs distinct: construction used retained pre-reload
    # images, while the new observer independently reloads those exact saves.
    # Every rendered RGB/index byte and native physical identity must survive.
    for configuration in ("Debug", "RelWithDebInfo"):
        constructed = next(row for row in construction["native"] if row["configuration"] == configuration)
        checked_reference(constructed["retained_application"])
        created_path = checked_reference(constructed["execution"])
        created = handoff.read_json(created_path.relative_to(ROOT).as_posix())
        reload_path = RUN / ("reload-" + configuration.lower()) / "reload.json"
        reloaded = handoff.read_json(reload_path.relative_to(ROOT).as_posix())
        observed = reloaded["observation"]
        # Require the actual hidden native case and a completed, replacement-
        # free eleven-industry reload; process exit alone does not prove it.
        if (reloaded.get("passed") is not True or reloaded["mode"] != "reload" or observed["exit_code"] != 0
                or observed["assertions"] != 339 or observed["native_construction_industries"] != 11):
            raise ValueError("current selected industry native reload is incomplete")
        for row in reloaded["inputs"] + [observed["application"], observed["source"], observed["terminal"], observed["runtime_payload"]]:
            checked_reference(row)
        before, old_images = scene(created)
        after, new_images = scene(reloaded)
        if before["frames"] != after["frames"] or after.get("loaded_checkpoint") is not True:
            raise ValueError("native fixture reload changed actual identities/geometry or rebuilt its facilities")
        matched = []
        # Match every industry's own raster and index capture, not the retained
        # collector bootstrap or a nearby tile standing in for new machinery.
        for frame in before["frames"]:
            for suffix in (".ppm", ".idx"):
                name = frame["basename"] + suffix
                if old_images[name].read_bytes() != new_images[name].read_bytes():
                    raise ValueError("native fixture reload changed rendered construction pixels: " + name)
                matched.append({"before": reference(old_images[name]), "after": reference(new_images[name])})
        application = checked_reference(observed["application"])
        retained = DIRECTORY / "industry-reload-bin" / configuration / application.name
        retained.parent.mkdir(parents=True, exist_ok=True)
        if retained.exists() and retained.read_bytes() != application.read_bytes():
            raise ValueError("retained native reload image already has another identity")
        if not retained.exists():
            retained.write_bytes(application.read_bytes())
        native.append({"configuration": configuration, "construction": reference(created_path), "reload": reference(reload_path),
                       "executed_application": observed["application"], "retained_application": reference(retained),
                       "construction_assertions": 511, "reload_assertions": 339, "exact_render_pairs": matched})
    baseline = (RUN / "before.json").relative_to(ROOT).as_posix()
    transitions, count = preserved(baseline)
    inputs = ["src/tests/ground_industry_runtime.cpp", "src/tests/ground_industry_runtime_fixture.h",
              "src/tests/multilayer/ground_runtime_checks-1.py", "CMakeLists.txt"]
    result = {"_documentation": __doc__, "passed": True, "native": native, "construction": reference(construction_path),
              "inputs": [reference(handoff.bound(path)) for path in inputs], "baseline": reference(handoff.bound(baseline)),
              "transitions": transitions, "preserved_file_count": count, "native_industries": 11,
              "fixture_capital": 1000000000, "fixture_floor": True, "native_checkpoint_reload_verified": True,
              "native_construction_capture_verified": True, "full_state_coverage": False,
              "source_pixel_verification": "pending", "transported_bootstrap_claimed": False}
    (DIRECTORY / "industry-fixture-transport.json").write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    return result


if __name__ == "__main__":
    print(json.dumps({key: value for key, value in qualify().items() if key in
          ("passed", "native_industries", "native_checkpoint_reload_verified", "full_state_coverage", "source_pixel_verification")}))
