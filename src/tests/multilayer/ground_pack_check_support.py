"""Shared receipt writer for focused selected-ground packing qualification.

@file ground_pack_check_support.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group79D.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
These checks qualify source conversion; they grant no native-loading or gameplay claim.
"""

import argparse
import json
from pathlib import Path
import sys
import time
import unittest

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / "media/multilayer"))
import pack_ground_art as ground
from tools import ground_art_handoff as handoff


def run_checks(case):
    """Execute one bounded suite and retain its actual result and input identities.

    @param case Documented unittest class containing only this active group's checks.
    @return None; exit nonzero when any check fails.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--report", required=True, help="Repository logs receipt path")
    args = parser.parse_args()
    target = handoff.bound(args.report)
    if not target.is_relative_to(ROOT / "logs"):
        raise ValueError("ground qualification receipts belong in repository logs")
    started = time.perf_counter()
    result = unittest.TextTestRunner(verbosity=2).run(unittest.defaultTestLoader.loadTestsFromTestCase(case))
    owners = [Path(sys.modules[case.__module__].__file__), Path(__file__),
              ROOT / "media/multilayer/pack_ground_art.py", ROOT / "media/multilayer/tools/ground_art_handoff.py",
              ROOT / "media/multilayer/pack_m1_art.py", handoff.SCOPE]
    report = {"_documentation": __doc__, "schema_version": 1, "suite": case.__name__,
              "passed": result.wasSuccessful(), "cases": result.testsRun,
              "failures": len(result.failures), "errors": len(result.errors),
              "seconds": round(time.perf_counter() - started, 3),
              "inputs": [{"path": path.relative_to(ROOT).as_posix(), "sha256": handoff.digest(path)} for path in owners],
              "native_loading_claimed": False, "runtime_activated": False}
    target.parent.mkdir(parents=True, exist_ok=True)
    target.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    raise SystemExit(0 if result.wasSuccessful() else 1)
