"""Earn at most two native economic periods toward the initial launch milestone.

@file surface_entry_launch_savings_checks-3.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15BIED.2.
Actual freight/passenger payments and matching native repayments alone advance
the selected treasury. No forecast or replay branch contributes useful capital.
"""
import argparse
import json
from pathlib import Path
import runpy

from space_preview_checks import ROOT, identity

BANK = runpy.run_path(str(Path(__file__).with_name("surface_entry_launch_savings_checks-1.py")))["execute"]
EARN = runpy.run_path(str(Path(__file__).with_name("surface_entry_launch_savings_checks-2.py")))["execute"]


def boundary(configuration, phase, previous, destination):
    """@param configuration Actual native build. @param phase Original matching repayment or bounded earned work.
    @param previous Immutable immediate selected predecessor. @param destination Unique retained native receipt.
    @return Complete actual boundary; an existing accepted receipt is reused without repaying or earning twice.
    """
    if destination.exists():
        value = json.loads(destination.read_text(encoding="utf-8"))
        assert value["complete"] and value["configuration"] == configuration and value["phase"] == phase
        assert value["operation"]["receipt"] == identity(previous)
        assert identity(ROOT / value["operation"]["output"]["path"]) == value["operation"]["output"]
    else:
        driver = BANK if phase == "repay" else EARN
        operation = driver(configuration, phase, previous)
        value = dict(_documentation=__doc__, complete=True, configuration=configuration, phase=phase, operation=operation)
        destination.write_text(json.dumps(value, indent=2) + "\n", encoding="utf-8")
    print(configuration, phase, destination.name, value["operation"]["final"]["company"], flush=True)
    return value


def execute(configuration, receipt, start_period, periods):
    """@param configuration Actual focused native application. @param receipt Exact selected current native treasury receipt.
    @param start_period Number of completed savings periods. @param periods Maximum one or two further native annual periods.
    @return Continuous actual receipts and whether the explicit initial325000spendable milestone is funded.
    """
    foundation_path = ROOT / "logs/surface-entry-launch-savings-plan-verification.json"
    foundation = json.loads(foundation_path.read_text(encoding="utf-8")); assert foundation["complete"]
    previous = receipt; paths = []; completed = start_period
    value = json.loads(previous.read_text(encoding="utf-8"))
    assert value["complete"] and value["configuration"] == configuration
    company = value["operation"]["final"]["company"]
    # Spendable capital includes only actual cash and original unused loan
    # capacity. Repayment preserves it; interest reduction is earned later.
    for period in range(start_period, start_period + periods):
        if company["cash"] + 5000000 - company["debt"] >= 325000:
            break
        if company["cash"] >= 160000:
            destination = ROOT / f"logs/surface-entry-launch-savings-repaid-{configuration}-{period}.json"
            value = boundary(configuration, "repay", previous, destination)
            previous = destination; paths.append(identity(destination))
        destination = ROOT / f"logs/surface-entry-launch-savings-earned-{configuration}-{period + 1}.json"
        value = boundary(configuration, "earn", previous, destination)
        previous = destination; paths.append(identity(destination)); completed = period + 1
        company = value["operation"]["final"]["company"]
    return dict(_documentation=__doc__, complete=True, configuration=configuration, foundation=identity(foundation_path), source_receipt=identity(receipt),
                from_period=start_period, last_period=completed, receipts=paths, last_receipt=identity(previous), output=value["operation"]["output"],
                company=company, spendable=company["cash"] + 5000000 - company["debt"], milestone_reached=company["cash"] + 5000000 - company["debt"] >= 325000,
                complete_launch=False, source=identity(Path(__file__)))


def main():
    """Retain one short sequential batch and leave full launch affordability open."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), required=True)
    parser.add_argument("--receipt", type=Path, required=True)
    parser.add_argument("--from-period", type=int, required=True)
    parser.add_argument("--periods", type=int, choices=(1, 2), default=2)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args(); output = (ROOT / args.output).resolve()
    assert args.from_period >= 0 and output.is_relative_to(ROOT / "logs") and not output.exists()
    output.write_text(json.dumps(execute(args.configuration, (ROOT / args.receipt).resolve(), args.from_period, args.periods), indent=2) + "\n", encoding="utf-8")
    print(output.relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
