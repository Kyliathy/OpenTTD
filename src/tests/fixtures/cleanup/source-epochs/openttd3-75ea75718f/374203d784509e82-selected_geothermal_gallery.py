"""Observe exact paid geothermal originals once and reuse their existing native art verifiers.

@file selected_geothermal_gallery.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Groups84CD/84C.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
No builder, native quantity, source bitmap or second runtime registry is supplied by this controller.
"""
import argparse
import json
import os
from pathlib import Path
import subprocess
import sys

sys.dont_write_bytecode = True
from geothermal_machinery_raster import ROOT, RECEIPTS, identity, verify_art_references
from geothermal_conduit_raster import paid_original
from tube_sprite_report import compare_worlds
from runner import current_run_context
from selected_natural_vent_pixels import natural_vent_pixels
sys.path.insert(0, str(ROOT / "media/multilayer"))
from evidence import EvidenceSet

CASES = ("funded", "ready", "vent-working", "magma-working", "heat-removed", "solid", "retired")


def original_for(configuration, case):
    """Select one exact retained original through its existing sealed producer owner.

    @param configuration Current native reader configuration; paid checkpoint selection retains its original opposite-writer lineage.
    @param case Named paid scene, original solid baseline or actual paid retired plant with the same surviving natural source.
    @return Exact source lineage and ordinary observer scene; no latest-file discovery or world rebuilding occurs.
    """
    if case == "solid":
        selected = EvidenceSet(ROOT / "logs/proofs/mlm/native-art/selected-runtime-20261018/evidence.json")
        path = selected.selected("reports", "natural-vent-acceptance"); closure = json.loads(path.read_text(encoding="utf-8"))
        old = next(row for row in closure["configurations"][configuration]["raster_reports"] if row["path"].endswith(f"-{configuration}-solid.json"))
        assert identity(ROOT / old["path"]) == old
        native = json.loads((ROOT / old["path"]).read_text(encoding="utf-8"))
        return {"receipt": identity(path), "report": old, "input": native["input"]}, "solid"
    if case == "retired":
        name, sha = RECEIPTS["funded"]; path = ROOT / name; assert identity(path)["sha256"] == sha
        closure = json.loads(path.read_text(encoding="utf-8"))
        # Each funding writer explicitly records the opposite lifecycle writer
        # it borrowed. Selecting that same configuration's retained retirement
        # checkpoint therefore gives this reader the opposite original writer.
        record = next(row for row in closure["records"] if row["configuration"] == configuration)
        point = record["retired_checkpoint"]; assert point["stage"] == 3 and point["snapshot"]["generators"] == point["snapshot"]["collectors"] == 0
        return {"receipt": identity(path), "report": record["retired_report"], "input": point["input"], "snapshot": point["snapshot"]}, "magma-loop"
    return paid_original(configuration, case)


def capture(configuration, case, output, native_report=None):
    """Capture one ordinary native world and recheck the same report through applicable existing verifiers.

    @param configuration Actual current native Debug or optimized reader.
    @param case Exact named original source/construction/service/removal/baseline/retirement scene.
    @param output Absent progress/final report under repository logs.
    @param native_report Optional exact current same-binary ordinary observation for rechecking, never an implicit latest run.
    @return None; all complete owners, original inputs, native source/binary identities and actual source pixels must pass.
    """
    output = (ROOT / output).resolve(); assert output.is_relative_to(ROOT / "logs") and not output.exists()
    original, scene = original_for(configuration, case); assert identity(ROOT / original["input"]["path"]) == original["input"]
    context = current_run_context("graphics", "selected-geothermal-gallery")
    common = case in ("funded", "ready", "vent-working", "magma-working")
    flags = {"OTTD_GEO_MACHINERY_INTERACTION": "1" if common else "0", "OTTD_GEO_CONDUIT_CAPTURE": "1" if case in CASES[:5] else "0",
             "OTTD_GEO_CONTROL_CAPTURE": "1" if case in ("ready", "magma-working") else "0", "OTTD_GEO_RISER_CAPTURE": "1" if case == "retired" else "0",
             "OTTD_GEO_MACHINERY_CAPTURE": "0", "OTTD_SELECTED_PURCHASE_CAPTURE": "0"}
    result = {"_documentation": "r2mlvs14-remaining-assets-and-runtime-integration.md84CD; one actual original native observation shared by existing pixel/purity verifiers;retired scene uses the existing pre-generator riser observer mode and claims no operating generator",
              "configuration": configuration, "case": case, "scene": scene, "original": original, "flags": flags, "controller": identity(__file__), "roles": {}, "complete": False}
    output.parent.mkdir(parents=True, exist_ok=True); output.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    if native_report is None:
        command = [sys.executable, "-B", str(ROOT / "src/tests/multilayer/utilities/geothermal_window_raster_checks.py"), "--config", configuration, "--scene", scene, "--input", str(ROOT / original["input"]["path"])]
        process = subprocess.run(command, cwd=ROOT, env=dict(os.environ, **flags), capture_output=True, timeout=110, check=False)
        log = context.logs / f"{configuration}-{case}-native-controller.log"; log.write_bytes(process.stdout + process.stderr); assert process.returncode == 0, log
        path = Path(process.stdout.decode("utf-8").splitlines()[-1]); result["native_controller_log"] = identity(log)
    else:
        path = ROOT / native_report
    native = json.loads(path.read_text(encoding="utf-8")); verify_art_references(native, {})
    assert native["configuration"] == configuration and native["scene"] == scene and native["input"] == original["input"]
    assert native["binary"] == identity(ROOT / f"build/{configuration}/openttd_test.exe") and native["case_runs"] == 1
    assert native["all_complete_native_chunks_equal"] and compare_worlds(native["before"], native["after"]) == 126
    result["native_report"] = identity(path); output.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    result["natural_vent"] = natural_vent_pixels(native)
    result["natural_vent_verifier"] = identity(ROOT / "src/tests/multilayer/selected_natural_vent_pixels.py")
    roles = []
    if common:
        roles.append(("machinery", "geothermal_machinery_interaction.py"))
    if case in CASES[:5]:
        roles.append(("conduit", "geothermal_conduit_raster.py"))
    if case in ("ready", "magma-working"):
        roles.append(("controls", "selected_control_raster.py"))
    for role, script in roles:  # Every applicable verifier consumes this exact already-complete ordinary observation; it cannot launch another native world or replace a missing pose with an older receipt.
        command = [sys.executable, "-B", str(ROOT / "src/tests/multilayer" / script), "--config", configuration, "--case", case, "--native-report", str(path.relative_to(ROOT))]
        process = subprocess.run(command, cwd=ROOT, capture_output=True, timeout=110, check=False)
        log = context.logs / f"{configuration}-{case}-{role}-verifier.log"; log.write_bytes(process.stdout + process.stderr); assert process.returncode == 0, log
        proof = Path(process.stdout.decode("utf-8").splitlines()[-1]); report = json.loads(proof.read_text(encoding="utf-8")); verify_art_references(report, {})
        assert report["native_report"] == identity(path)
        result["roles"][role] = {"report": identity(proof), "controller_log": identity(log)}
        output.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    assert identity(ROOT / original["input"]["path"]) == original["input"]
    result["complete"] = True; output.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"report": identity(output), "roles": list(result["roles"]), "native": result["native_report"]}), flush=True)


def main():
    """Select one exact current paid geothermal or natural-vent observation.

    @return None; all generated output is explicit and source/gameplay owners remain unchanged.
    """
    parser = argparse.ArgumentParser(description=__doc__); parser.add_argument("--config", choices=("Debug", "RelWithDebInfo"), required=True)
    parser.add_argument("--case", choices=CASES, required=True); parser.add_argument("--native-report"); parser.add_argument("--output", required=True)
    args = parser.parse_args(); capture(args.config, args.case, args.output, args.native_report)


if __name__ == "__main__":
    main()
