"""Verify actual paid Storage freight in a second bounded native burst.

@file orbital_activation_checks-2.py Native packet exchange, interrupted stock custody and ordinary resave.
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group4.
"""
import argparse
import json
from pathlib import Path
import runpy

from space_preview_checks import ROOT, chunks, identity, ordinary
from runner import current_run_context

shared = runpy.run_path(str(Path(__file__).with_name("orbital_activation_checks-1.py")))
native, EXACT = shared["native"], shared["EXACT"]


def main():
    """Continue the explicit preceding activation report using only its immutable actual paid output."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", default="RelWithDebInfo", choices=("RelWithDebInfo", "Debug"))
    parser.add_argument("--activation", type=Path, required=True, help="Exact orbital-module-activation.json from the preceding burst")
    args = parser.parse_args()
    activation = args.activation.resolve()
    assert activation.is_relative_to(ROOT)
    prepared = json.loads(activation.read_text(encoding="utf-8"))
    assert prepared["configuration"] == args.configuration
    source = ROOT / prepared["complete"]["path"]
    assert identity(source) == prepared["complete"]
    context = current_run_context("space", "module-freight-verify")
    freight = context.for_feature("space", "module-freight-prepare")
    native(freight, args.configuration, "freight", source)
    complete, partial = freight.saves / "module-freight-422.sav", freight.saves / "module-stocked-422.sav"
    continuation = context.for_feature("space", "module-freight-resume")
    native(continuation, args.configuration, "stored", partial)
    continued = continuation.saves / "module-freight-422.sav"
    expected, actual = chunks(complete)[1], chunks(continued)[1]
    # Interrupted physical custody must preserve every native packet, provenance, stock and financial receipt.
    for family in EXACT:
        assert expected[family] == actual[family], family
    ordinary_context = context.for_feature("space", "module-freight-ordinary")
    ordinary(ordinary_context, {"name": "module-stocked", "snapshot": identity(complete)}, args.configuration)
    resaved = ordinary_context.saves / "ordinary-resave.sav"
    actual = chunks(resaved)[1]
    # Loading a functional Storage module cannot grant stock again or regenerate consumed construction cargo.
    for family in EXACT:
        assert expected[family] == actual[family], family
    assert identity(source) == prepared["complete"]
    report = context.reports / "orbital-module-freight.json"
    report.write_text(json.dumps({"_documentation": "Owner: r2mlvs11-orbital-stations-and-manufacturing.md Group4; real paid Storage freight.",
        "configuration": args.configuration, "activation": identity(activation), "source": identity(source), "partial": identity(partial),
        "complete": identity(complete), "continued": identity(continued), "ordinary_resave": identity(resaved),
        "exact_owner_families": len(EXACT), "physical_stock_units": 1, "no_delivery_payment": True}, indent=2) + "\n", encoding="utf-8")
    print(f"Native module freight verified: {report.relative_to(ROOT)}", flush=True)


if __name__ == "__main__":
    main()
