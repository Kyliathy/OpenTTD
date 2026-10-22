"""Run original focused Ground semantic corruptions against explicit current native captures.

@file ground_pixel_refusal_support.py Real captured raster copies; no new native acceptance or baseline.
@see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup3.md Group20BB.
"""
import argparse
import json
from pathlib import Path
import sys
import time
import unittest

from ground_viewport_review import ROOT, SOURCE_ROLES, checked_result
from ground_review_sources import GroundSourceViews
from artifact_paths import contained
from artifact_rebuild import identity


def scratch_root(name):
    """Resolve a repository-owned temporary-copy parent before a semantic test writes/deletes files.

    @param name Fixed test-owner basename beneath cleanup scratch.
    @return Existing contained directory, refusing links outside the checkout before any write.
    """
    path = contained(ROOT / "test-cache/cleanup" / name, ROOT)
    path.mkdir(parents=True, exist_ok=True)
    return path


def run_checks(case, mode):
    """Select a current actual capture and run only this original bounded corruption suite.

    @param case Original focused unittest class; every case copies only its own actual scene.
    @param mode Exact current native mode whose original semantic obligations are being checked.
    @return None; report/terminal stay under test-cache and any failed semantic case exits nonzero.
    """
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--capture-result", type=Path, required=True)
    parser.add_argument("--report", type=Path, required=True)
    args = parser.parse_args()
    path = contained(args.capture_result.resolve(), ROOT / "mlm/reviews")
    reference = identity(path, ROOT)
    raw = json.loads(path.read_text(encoding="utf-8"))
    document = checked_result(reference, mode, raw["configuration"], ROOT)
    target = contained(args.report.resolve(), ROOT / "test-cache")
    terminal = target.with_suffix(".txt")
    if target.exists() or terminal.exists():
        raise ValueError("focused Ground semantic report exists; select a fresh test output")
    target.parent.mkdir(parents=True, exist_ok=True)
    selector = json.loads((ROOT / document["source_selector"]["path"]).read_text(encoding="utf-8"))
    inputs = {role: document["source_inputs"][role] for role in SOURCE_ROLES}
    module = sys.modules[case.__module__]
    module.EXECUTION = document["actual_execution"]["path"]
    started = time.monotonic()
    with GroundSourceViews(ROOT, selector["run_id"], inputs) as source_view:
        case.source_view = source_view
        with terminal.open("x", encoding="utf-8") as stream:
            result = unittest.TextTestRunner(stream=stream, verbosity=2).run(unittest.defaultTestLoader.loadTestsFromTestCase(case))
        source_view.verify_current()
    report = dict(_documentation=__doc__, kind="ground_current_pixel_semantic_checks", passed=result.wasSuccessful(),
        mode=mode, cases=result.testsRun, seconds=round(time.monotonic() - started, 3),
        current_capture=reference, actual_execution=document["actual_execution"], terminal=identity(terminal, ROOT),
        inputs=[identity(Path(module.__file__), ROOT), identity(Path(__file__), ROOT)],
        native_execution_performed=False, new_baseline_created=False, human_acceptance=False)
    target.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(dict(passed=report["passed"], cases=report["cases"], seconds=report["seconds"], report=identity(target, ROOT)), indent=2))
    raise SystemExit(0 if result.wasSuccessful() else 1)
