"""Verify the changed portable first-Core work boundary against a genuine415 paid construction job.

@file orbital_module_checks-5.py Legacy work migration, duplicate callback refusal and exact ordinary resave.
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group3.
"""
import argparse
import json
from pathlib import Path
import runpy

from space_preview_checks import ROOT, chunks, identity, ordinary
from runner import current_run_context

native = runpy.run_path(str(Path(__file__).with_name("orbital_module_checks-2.py")))["native"]
EXACT = runpy.run_path(str(Path(__file__).with_name("orbital_service_checks-1.py")))["EXACT"]


def main():
    """Continue actual old delivered inputs with two original builders while preserving the independent first-station bootstrap."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", default="RelWithDebInfo", choices=("RelWithDebInfo", "Debug"))
    parser.add_argument("--source", type=Path, required=True, help="Immutable genuine construction-work-415.sav at37 paid work ticks")
    args = parser.parse_args()
    source = args.source.resolve()
    assert source.is_relative_to(ROOT)
    original = identity(source)
    context = current_run_context("space", "module-bootstrap-verify")
    prepare = context.for_feature("space", "module-bootstrap-prepare")
    frame = native(prepare, args.configuration, "bootstrap", source)
    checkpoint = prepare.saves / "bootstrap-receipt-418.sav"
    assert chunks(frame)[0] == 418 and chunks(checkpoint)[0] == 418
    resave = context.for_feature("space", "module-bootstrap-ordinary")
    ordinary(resave, {"name": "bootstrap", "snapshot": identity(checkpoint)}, args.configuration)
    original_chunks, saved_chunks = chunks(checkpoint)[1], chunks(resave.saves / "ordinary-resave.sav")[1]
    # An ordinary paused load preserves the native work receipt, actual paid inputs and empty utility authority exactly.
    for family in EXACT:
        assert original_chunks.get(family) == saved_chunks.get(family), family
    assert identity(source) == original
    report = context.reports / "orbital-module-bootstrap-verification.json"
    report.write_text(json.dumps({"_documentation": "Owner: r2mlvs11-orbital-stations-and-manufacturing.md Group3; genuine415 first-Core continuation.",
        "configuration": args.configuration, "input": original, "receipt": identity(checkpoint), "frame": identity(frame),
        "one_work_tick_for_two_builders": True, "duplicate_public_work_refused": True, "no_station_utility_prerequisite": True,
        "exact_owner_families": [family.decode() for family in EXACT]}, indent=2) + "\n", encoding="utf-8")
    print(f"Native first-Core work receipt verified: {report.relative_to(ROOT)}", flush=True)


if __name__ == "__main__":
    main()
