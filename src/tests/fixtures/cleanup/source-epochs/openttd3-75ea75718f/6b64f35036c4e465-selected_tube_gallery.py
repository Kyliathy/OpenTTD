"""Refresh existing native tube scene producers from exact sealed original writer inputs.

@file selected_tube_gallery.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group84CC.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
This sequential controller selects existing observers, never rebuilds their accepted worlds and creates no sprite/gameplay registry.
"""
import argparse
import json
from pathlib import Path
import subprocess
import sys

sys.dont_write_bytecode = True
from geothermal_machinery_raster import ROOT, identity, verify_art_references
from runner import current_run_context
sys.path.insert(0, str(ROOT / "media/multilayer"))
from evidence import EvidenceSet

SELECTION = ROOT / "logs/proofs/mlm/native-art/selected-runtime-20261018/evidence.json"
STRUCTURES = (
    ("tube-guideway-acceptance", "tube_sprite_checks.py", ("raster", "reservation"), "case"),
    ("tube-capsule-acceptance", "tube_capsule_sprite_checks.py", ("raster",), "case"),
    ("tube-facility-acceptance", "tube_facility_sprite_checks.py", ("raster",), "scene"),
    ("tube-conduit-acceptance", "tube_conduit_sprite_checks.py", ("raster",), "scene"),
)


def structure_jobs(configuration):
    """Select exact accepted presentation originals while retaining their disclosed setup boundary.

    @param configuration Current native Debug or optimized reader; each old report already selects its opposite-writer original.
    @return Ordered existing observer commands and sealed receipt/writer identities; no source-only prefix can enter gallery scope.
    """
    selected = EvidenceSet(SELECTION); jobs = []
    for role, script, focuses, dimension in STRUCTURES:  # Earlier family closures own the immutable original scenes and their fixture classifications, not a newly constructed convenient world.
        path = selected.selected("reports", role); receipt = json.loads(path.read_text(encoding="utf-8"))
        for report in receipt["reports"]:  # Re-observe every previously accepted scene/state for this family and reader; skip old builders and unrelated focused checks explicitly.
            if report["configuration"] != configuration or report["focus"] not in focuses:
                continue
            writer = report["native"]["writer"]; assert identity(ROOT / writer["path"]) == writer
            args = ["--config", configuration, "--focus", report["focus"], "--" + dimension, str(report[dimension]), "--writer-report", writer["path"]]
            jobs.append({"name": role + "-" + report["focus"] + "-" + str(report[dimension]), "script": "src/tests/multilayer/" + script, "arguments": args,
                         "sealed_receipt": identity(path), "writer": writer,
                         "fixture_classification": receipt.get("fixture_classifications", receipt.get("observed_fixture_failures", receipt["_documentation"]))})
    assert len(jobs) == 13, "current selected structure observation matrix is incomplete"
    return jobs


def capture(configuration, output):
    """Execute the bounded original tube scene matrix sequentially and retain explicit progress.

    @param configuration Actual compiled current native reader configuration.
    @param output Absent per-reader progress/final report under repository logs; every successful observation is recorded immediately.
    @return None; any stale original, failed native observer or changed current source refuses the final matrix.
    """
    output = (ROOT / output).resolve(); assert output.is_relative_to(ROOT / "logs") and not output.exists()
    context = current_run_context("graphics", "selected-tube-gallery"); jobs = structure_jobs(configuration)
    result = {"_documentation": "r2mlvs14-remaining-assets-and-runtime-integration.md84CC.1; existing current native presentation observers from exact immutable original worlds;useful-route scope remains separate",
              "configuration": configuration, "controller": identity(__file__), "binary": identity(ROOT / f"build/{configuration}/openttd_test.exe"),
              "public": identity(ROOT / "media/multilayer/contracts/multilayer_sprite_manifest.json"), "selection": identity(SELECTION),
              "jobs": jobs, "observations": [], "complete": False}
    output.parent.mkdir(parents=True, exist_ok=True); output.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    for index, job in enumerate(jobs):  # Only one native process runs at a time; immediate explicit progress preserves completed current evidence across interruptions without latest-run discovery.
        command = [sys.executable, "-B", str(ROOT / job["script"]), *job["arguments"]]
        process = subprocess.run(command, cwd=ROOT, capture_output=True, timeout=110, check=False)
        log = context.logs / f"{configuration}-{index:02}-{job['name']}.log"; log.write_bytes(process.stdout + process.stderr)
        assert process.returncode == 0, log
        path = Path(process.stdout.decode("utf-8").splitlines()[-1]); report = json.loads(path.read_text(encoding="utf-8"))
        verify_art_references(report, {})
        assert report["configuration"] == configuration and report["binary"] == result["binary"]
        result["observations"].append({"job": job["name"], "report": identity(path), "controller_log": identity(log)})
        output.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
        print(f"{configuration} {index + 1}/{len(jobs)} {job['name']}: passed", flush=True)
    assert identity(ROOT / result["public"]["path"]) == result["public"]
    result["complete"] = True; output.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"report": identity(output), "observations": len(jobs)}), flush=True)


def main():
    """Run one bounded selected tube structure observation matrix for the named reader.

    @return None; explicit original identities and an absent output are mandatory.
    """
    parser = argparse.ArgumentParser(description=__doc__); parser.add_argument("--config", choices=("Debug", "RelWithDebInfo"), required=True)
    parser.add_argument("--output", required=True); args = parser.parse_args(); capture(args.config, args.output)


if __name__ == "__main__":
    main()
