"""Earn only current genuine fuel capital through original useful services.

@file surface_entry_supply_capital_checks-1.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CI.
Existing native service work owns every tick, freight payment and expense.
The original stored materials, vehicles, debt and loan ceiling remain guarded.
"""
import argparse
import json
from pathlib import Path
import runpy

from space_preview_checks import ROOT, identity
from surface_gateway_component_delivery_ledger import observe_retained as observe
from surface_gateway_replay import load, replay

DIRECTORY = Path(__file__).resolve().parent
DRIVER = runpy.run_path(str(DIRECTORY / "surface_entry_component_delivery_checks-1.py"))["execute"]
FOUNDATION = ROOT / "logs/surface-entry-supply-admission-electric-verification.json"
STEM = "surface-entry-supply-capital"


def selected(receipt):
    """Read the exact genuine world selected by a complete native receipt.

    @param receipt Immutable original predecessor for one configuration.
    @return Existing receipt and exact observed native world, without advancing time.
    """
    value = load(receipt)
    reference = value["operation"]["output"]
    path = ROOT / reference["path"]
    assert identity(path) == reference
    snapshot = json.loads(json.dumps(observe(path)))
    assert snapshot["components_at_center"] and snapshot["company"]["debt"] == 5000000
    if "final" in value["operation"]:
        assert snapshot == value["operation"]["final"]
    return value, snapshot


def admit():
    """Verify current fuel-window prices, original products and useful-service guards.

    @return Both unchanged selected worlds and the actual current funded-work target.
    """
    foundation = load(FOUNDATION)
    manifest = ROOT / foundation["products"]["path"]
    assert identity(manifest) == foundation["products"]
    # Current admission products must retain both their current source bytes
    # and tested copies before any original world earns new paid service work.
    for product in json.loads(manifest.read_text(encoding="utf-8"))["products"]:
        assert identity(ROOT / product["source"]["path"]) == product["source"]
        assert identity(ROOT / product["retained"]["path"]) == product["retained"]
    configurations = []
    # The cheaper route is a real quoted conversion prerequisite, not an
    # already powered depot. Its entire purchase/work/reserve bill determines
    # the savings target independently in both native configurations.
    for entry in foundation["configurations"]:
        receipt = ROOT / entry["receipt"]["path"]
        assert identity(receipt) == entry["receipt"]
        value, snapshot = selected(receipt)
        windows = entry["refill"]["window"]
        target = min(window["target"] for window in windows)
        assert target == 160576 and snapshot["company"]["cash"] == 143673
        life = min(row["remaining"] for row in snapshot["service_lives"])
        assert len(snapshot["service_lives"]) == 11 and life == 509
        assert snapshot["company"]["cash"] >= 118061 and life >= 90
        configurations.append(dict(configuration=value["configuration"], receipt=identity(receipt),
                                   output=value["operation"]["output"], company=snapshot["company"],
                                   target=target, shortfall=target - snapshot["company"]["cash"],
                                   minimum_service_days=life, maximum_ticks_per_burst=3000))
    return dict(_documentation=__doc__, complete=True, admission_only=True, foundation=identity(FOUNDATION),
                configurations=configurations, source=identity(Path(__file__)))


def execute(configuration, phase, receipt, recover_run=None):
    """Earn or replay one actual bounded original service interval.

    @param configuration Independently linked actual focused application.
    @param phase Existing native component-delivery-service phase only.
    @param receipt Selected original or separately restored exact predecessor.
    @param recover_run Successful native invocation to recover without repeating work.
    @return Original native books, exact material ownership and actual service lives.
    """
    assert phase == "component-delivery-service"
    value, snapshot = selected(receipt)
    assert value["configuration"] == configuration
    assert snapshot["company"]["cash"] >= 118061
    assert min(row["remaining"] for row in snapshot["service_lives"]) >= 90
    selected_receipt = receipt
    if "final" not in value["operation"]:
        # An ordinary restore publishes original process/save identities.
        # Attach this explicit observer contract in a separate receipt without
        # rewriting either the ordinary export or any gameplay bytes.
        assert value["phase"] == "restore"
        annotated = json.loads(json.dumps(value))
        annotated["_documentation"] = __doc__
        annotated["ordinary_restore"] = identity(receipt)
        annotated["operation"]["final"] = snapshot
        selected_receipt = ROOT / f"logs/{STEM}-ordinary-source-{configuration}.json"
        assert not selected_receipt.exists()
        selected_receipt.write_text(json.dumps(annotated, indent=2) + "\n", encoding="utf-8")
    result = DRIVER(configuration, phase, selected_receipt, recover_run)
    earned, = result["rows"]["earned"]
    assert earned["earned"] == 3000 and earned["debt"] == 5000000
    assert result["initial"]["company"]["cash"] - earned["net_expense"] == result["final"]["company"]["cash"]
    assert len(result["final"]["service_lives"]) == 11
    assert min(row["remaining"] for row in result["final"]["service_lives"]) > 0
    result["sources"].append(identity(Path(__file__)))
    return result


def main():
    """Publish one admitted savings, bounded earned-work or ordinary replay receipt."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--mode", choices=("admit", "earn", "replay"), required=True)
    parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"))
    parser.add_argument("--receipt", type=Path)
    parser.add_argument("--recover-run")
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    output = ROOT / args.output
    assert output.resolve().is_relative_to(ROOT / "logs") and not output.exists()
    if args.mode == "admit":
        result = admit()
    else:
        assert args.configuration is not None and args.receipt is not None
        if args.mode == "earn":
            operation = execute(args.configuration, "component-delivery-service", ROOT / args.receipt, args.recover_run)
            result = dict(_documentation=__doc__, complete=True, configuration=args.configuration,
                          phase="component-delivery-service", operation=operation)
        else:
            assert args.recover_run is None
            result = replay(args.configuration, ROOT / args.receipt, STEM, "component-delivery-service", execute, observe)
    output.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(output.relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
