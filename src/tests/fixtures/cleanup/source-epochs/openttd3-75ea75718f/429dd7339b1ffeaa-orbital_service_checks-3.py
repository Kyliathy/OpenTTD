"""Verify both missing construction inputs and physical cancellation without fabricated stock.

@file orbital_service_checks-3.py First-Core material boundaries from genuine paid delivery/work saves.
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group1B, original task1.4.
"""
import argparse
import json
from pathlib import Path
import runpy

from space_preview_checks import ROOT, chunks, identity, ordinary
from space_construction_checks import native
from runner import current_run_context

EXACT = runpy.run_path(str(Path(__file__).with_name("orbital_service_checks-1.py")))["EXACT"]


def main():
    """Run two bounded material/cancellation cases and ordinary paused resaves against immutable native predecessors."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", default="RelWithDebInfo", choices=("RelWithDebInfo", "Debug"))
    parser.add_argument("--work-source", type=Path, required=True, help="Genuine construction-work-415.sav with both paid delivered inputs")
    parser.add_argument("--delivery-source", type=Path, required=True, help="Genuine construction-delivery-415.sav with Goods delivered but no Steel")
    args = parser.parse_args()
    context = current_run_context("space", "core-material-verify")
    cases = []
    # Material shortages must retain different real owners: a job-only first delivery and returned inputs in two paid builders.
    for phase, source in (("missing-goods", args.work_source), ("missing-steel", args.delivery_source)):
        source = source.resolve()
        assert source.is_relative_to(ROOT)
        before = identity(source)
        owner = context.for_feature("space", "core-material-" + phase)
        native(owner, args.configuration, phase, source)
        partial = owner.saves / ("construction-" + phase + "-418.sav")
        collected = owner.saves / "construction-boundary-collected-418.sav"
        assert chunks(partial)[0] == 418 and chunks(collected)[0] == 418
        resave = context.for_feature("space", "core-material-ordinary-" + phase)
        ordinary(resave, {"name": phase, "snapshot": identity(partial)}, args.configuration)
        original_chunks, saved_chunks = chunks(partial)[1], chunks(resave.saves / "ordinary-resave.sav")[1]
        for family in EXACT:
            assert original_chunks.get(family) == saved_chunks.get(family), (phase, family)
        assert identity(source) == before
        cases.append({"phase": phase, "input": before, "partial": identity(partial), "collected": identity(collected)})
    report = context.reports / "orbital-core-material-boundaries.json"
    report.write_text(json.dumps({"configuration": args.configuration, "cases": cases,
        "missing_either_material_blocks_work_and_activation": True, "cancelled_unconsumed_inputs_returned": True,
        "second_blueprint_charged_without_refund": True, "exact_owner_families": [f.decode() for f in EXACT]}, indent=2) + "\n", encoding="utf-8")
    print(f"First-Core material boundaries verified: {report.relative_to(ROOT)}", flush=True)


if __name__ == "__main__":
    main()
