"""Probe native catalogue/date availability after ordinary loading of actual paid construction.

@file tube_tbm_catalogue.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Group16BB.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
The observer is read-only and cannot repair a missing saved availability record.
"""

import argparse
import json
from pathlib import Path
import sys

sys.dont_write_bytecode = True
from construction_provider_return import ROOT, identity
from tube_tbm_useful import OWNER_KEYS, SOURCES, native_loaded
from runner import current_run_context


def probe(configuration, construction_report):
    """Require public/future dates from the original opposite writer without state repair.

    @param configuration Current native reader,opposite the actual construction writer.
    @param construction_report Explicit completed original paid construction report.
    @return Read-only ordinary-load report with unchanged original input identity.
    """
    record = json.loads(construction_report.read_text(encoding="utf-8"))
    assert record["configuration"] != configuration
    assert record["native_version"] == 434 and record["focus"] == "clearing"
    original = record["original_ready"]
    source = ROOT / original["path"]
    assert identity(source) == original
    context = current_run_context("tubes", "tbm-final-use")
    tag = "[.tube-tbm-catalogue]"
    environment = {"OTTD_TUBE_TBM_INPUT": str(source),
                   "OTTD_TUBE_TBM_OWNERS": ",".join(str(record["owners"][key]) for key in OWNER_KEYS)}
    raw, proof = native_loaded(context, configuration, tag, environment, "catalogue")
    assert "TUBE_TBM_CATALOGUE native_public_dates=1 future_locked=1 unchanged_world=1" in raw
    assert identity(source) == original
    report = {
        "_documentation": "r2mlvs17-underground-transport-tubes.md16BB;pure opposite-reader original paid-world introduced/future native capsule and conventional catalogue admission;no availability,world or clock repair",
        "configuration": configuration, "focus": "catalogue", "tag": tag, "native_version": 434,
        "native": proof, "original_input": original, "construction_report": identity(construction_report),
        "controller": identity(__file__), "native_loader": identity(ROOT / "src/tests/multilayer/tube_tbm_useful.py"),
        "binary": identity(ROOT / f"build/{configuration}/openttd_test.exe"),
        "sources": [identity(ROOT / name) for name in SOURCES],
    }
    path = context.output("reports", f"tube-tbm-{configuration}-catalogue.json")
    path.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    return path


def main():
    """Select one exact paid construction original for a read-only catalogue probe.

    @return None;changed originals,missing availability or unlocked future models refuse.
    """
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--config", choices=("Debug", "RelWithDebInfo"), required=True)
    parser.add_argument("--construction-report", type=Path, required=True)
    args = parser.parse_args()
    print(probe(args.config, args.construction_report), flush=True)


if __name__ == "__main__":
    main()
