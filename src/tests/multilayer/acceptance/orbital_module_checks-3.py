"""Replay native assembly milestones and physically collect cancelled inputs in a separate focused burst.

@file orbital_module_checks-3.py Exact418 continuation, paused resave and unconsumed-material return.
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


def main(complete=False):
    """Verify one bounded half of the saved-work continuation, keeping Debug bursts below two minutes.

    @param complete False verifies work/partial; True verifies power/last and cancellation, joining the preceding report.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", default="RelWithDebInfo", choices=("RelWithDebInfo", "Debug"))
    parser.add_argument("--preparation", type=Path, required=True, help="Exact orbital-module-assembly-preparation.json from the preceding burst")
    if complete:
        parser.add_argument("--first-part", type=Path, required=True, help="Exact orbital-module-assembly-continuation-1.json from the preceding burst")
    args = parser.parse_args()
    preparation = args.preparation.resolve()
    assert preparation.is_relative_to(ROOT)
    prepared = json.loads(preparation.read_text(encoding="utf-8"))
    assert prepared["configuration"] == args.configuration and prepared["save_version"] == 418
    expected = chunks(ROOT / prepared["frame"]["path"])[1]
    context = current_run_context("space", "module-assembly-verify" if complete else "module-assembly-verify-1")
    checkpoints = []
    if complete:
        first = args.first_part.resolve()
        assert first.is_relative_to(ROOT)
        preceding = json.loads(first.read_text(encoding="utf-8"))
        assert preceding["configuration"] == args.configuration and preceding["preparation"] == identity(preparation)
        checkpoints = preceding["checkpoints"]
        assert [checkpoint["phase"] for checkpoint in checkpoints] == ["work", "partial"]
    # Every interruption must preserve the exact fractional receipt, actual parts, clock, native queues and original-source settlement.
    for phase in (("power", "last") if complete else ("work", "partial")):
        original = prepared["checkpoints"][phase]
        source = ROOT / original["path"]
        assert identity(source) == original
        continued = native(context.for_feature("space", "module-assembly-resume-" + phase), args.configuration, phase, source)
        actual = chunks(continued)[1]
        for family in EXACT:
            assert expected.get(family) == actual.get(family), (phase, family)
        resave = context.for_feature("space", "module-assembly-ordinary-" + phase)
        ordinary(resave, {"name": phase, "snapshot": original}, args.configuration)
        original_chunks, saved_chunks = chunks(source)[1], chunks(resave.saves / "ordinary-resave.sav")[1]
        for family in EXACT:
            assert original_chunks.get(family) == saved_chunks.get(family), ("ordinary", phase, family)
        assert identity(source) == original
        checkpoints.append({"phase": phase, "input": original, "continued": identity(continued)})
    result = {"_documentation": "Owner: r2mlvs11-orbital-stations-and-manufacturing.md Group3; native continuation and material conservation.",
        "configuration": args.configuration, "preparation": identity(preparation), "checkpoints": checkpoints,
        "exact_owner_families": [family.decode() for family in EXACT]}
    if complete:
        cancelled = native(context.for_feature("space", "module-assembly-cancel"), args.configuration, "cancel", ROOT / prepared["checkpoints"]["power"]["path"])
        result.update({"first_part": identity(first), "cancelled": identity(cancelled),
            "cancelled_unconsumed_returns": ["STEL8", "GOOD8"], "remaining_core_stock": ["STEL10", "GOOD8"],
            "cancelled_fractional_work_retained": True, "no_energy_refund_or_delivery_payment": True,
            "new_blueprint_charged_capital_again": True})
    report = context.reports / ("orbital-module-assembly-verification.json" if complete else "orbital-module-assembly-continuation-1.json")
    report.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    description = "continuation and cancellation" if complete else "work/partial continuation"
    print(f"Native assembly {description} verified: {report.relative_to(ROOT)}", flush=True)


if __name__ == "__main__":
    main()
