"""Earn short sequential batches of genuine launch savings and preserve renewal margin.

@file surface_entry_capital_checks-2.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15BIEK.2.
Only actual selected native work contributes capital. Every annual step leaves
service time for a later genuine empty recall and paid vehicle replacement.
"""
import argparse
import json
from pathlib import Path
import runpy

from space_preview_checks import ROOT, identity

DRIVER = runpy.run_path(str(Path(__file__).resolve().with_name("surface_entry_capital_checks-1.py")))


def boundary(configuration, phase, previous, destination, observe_fn=DRIVER["observe"], lives_fn=DRIVER["lives"]):
    """@param configuration Actual focused build. @param phase Original earnings or matching repayment.
    @param previous Immediate selected predecessor. @param destination Unique immutable receipt.
    @param observe_fn Explicit current material observer. @param lives_fn Every actually admitted useful-service age.
    @return Accepted actual boundary; successful existing work is never repeated.
    """
    if destination.exists():
        value = json.loads(destination.read_text(encoding="utf-8")); assert value["complete"] and value["configuration"] == configuration and value["phase"] == phase
        assert value["operation"]["receipt"] == identity(previous)
        assert identity(ROOT / value["operation"]["output"]["path"]) == value["operation"]["output"]
    else:
        operation = DRIVER["execute"](configuration, phase, previous, observe_fn=observe_fn, lives_fn=lives_fn)
        value = dict(_documentation=__doc__, complete=True, configuration=configuration, phase=phase, operation=operation)
        destination.write_text(json.dumps(value, indent=2) + "\n", encoding="utf-8")
    company = value["operation"]["final"]["company"]
    print(configuration, phase, destination.name, company, "spendable", company["cash"] + 5000000 - company["debt"], flush=True)
    return value


def execute(configuration, receipt, start, periods, observe_fn=DRIVER["observe"], lives_fn=DRIVER["lives"]):
    """@param configuration Actual focused build. @param receipt Current selected original treasury.
    @param start Completed native annual periods. @param periods At most two further bounded periods.
    @param observe_fn Explicit current material observer; original strict default is preserved.
    @param lives_fn Every current useful-service age; original eight-service default is preserved.
    @return Genuine receipts and either complete funding or the next explicit native service boundary.
    """
    value = json.loads(receipt.read_text(encoding="utf-8")); assert value["complete"] and value["configuration"] == configuration
    assert periods in (1, 2)
    previous, references, completed = receipt, [], start
    company = value["operation"]["final"]["company"]
    # Whole native cash plus unused original bank capacity is spendable.
    # Actual repayments preserve that total while reducing later interest.
    for period in range(start, start + periods):
        if company["cash"] + 5000000 - company["debt"] >= 1870000:
            break
        service = lives_fn(ROOT / value["operation"]["output"]["path"])
        if min(row["remaining"] for row in service) <= 732:
            break
        if company["cash"] >= 160000:
            path = ROOT / f"logs/surface-entry-capital-repaid-{configuration}-{period}.json"
            value = boundary(configuration, "repay", previous, path, observe_fn, lives_fn); previous = path; references.append(identity(path))
        path = ROOT / f"logs/surface-entry-capital-earned-{configuration}-{period + 1}.json"
        value = boundary(configuration, "earn", previous, path, observe_fn, lives_fn); previous = path; references.append(identity(path)); completed = period + 1
        company = value["operation"]["final"]["company"]
        # A measured unprofitable interval is a service prerequisite, not
        # authorization to spend decades running an ineffective income plan.
        if value["operation"]["rows"]["earned"][0]["net_expense"] >= 0:
            break
    service = lives_fn(ROOT / value["operation"]["output"]["path"])
    spendable = company["cash"] + 5000000 - company["debt"]
    return dict(_documentation=__doc__, complete=True, configuration=configuration, source_receipt=identity(receipt), from_period=start, last_period=completed,
                receipts=references, last_receipt=identity(previous), output=value["operation"]["output"], company=company, spendable=spendable,
                target=1870000, complete_capital=spendable >= 1870000, service_lives=service, renewal_due=min(row["remaining"] for row in service) <= 732,
                actual_launch_complete=False, sources=[identity(Path(__file__)), identity(Path(__file__).with_name("surface_entry_capital_checks-1.py")),
                                                      identity(Path(observe_fn.__code__.co_filename)), identity(Path(lives_fn.__code__.co_filename))])


def main():
    """Retain at most two native annual periods, stopping at the actual funding or service boundary."""
    parser = argparse.ArgumentParser(description=__doc__); parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), required=True)
    parser.add_argument("--receipt", type=Path, required=True); parser.add_argument("--from-period", type=int, required=True)
    parser.add_argument("--service", choices=("original", "regional"), default="original")
    parser.add_argument("--periods", type=int, choices=(1, 2), default=2); parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args(); output = (ROOT / args.output).resolve(); assert args.from_period >= 0 and output.is_relative_to(ROOT / "logs") and not output.exists()
    callbacks = {}
    if args.service == "regional":
        from surface_regional_ledger import observe, lives
        callbacks = dict(observe_fn=observe, lives_fn=lives)
    output.write_text(json.dumps(execute(args.configuration, (ROOT / args.receipt).resolve(), args.from_period, args.periods, **callbacks), indent=2) + "\n", encoding="utf-8")
    print(output.relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
