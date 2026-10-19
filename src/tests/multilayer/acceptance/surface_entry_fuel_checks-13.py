"""Earn at most two actual economic periods with the renewed freight fleet.

@file surface_entry_fuel_checks-13.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15BIB.2.
Each native payment/work process is independently bounded. Short batches retain
the continuous selected lineage and stop only at fresh actual affordability.
"""
import argparse
import json
from pathlib import Path
import runpy

from space_preview_checks import ROOT, identity

DRIVER = runpy.run_path(str(Path(__file__).with_name("surface_entry_fuel_checks-5.py")))["execute"]


def boundary(configuration, source, phase, destination):
    """Retain or resume one exact original economic boundary without replaying paid work.

    @param configuration Actual independently linked native game.
    @param source Immediate selected native predecessor receipt.
    @param phase Original repayment or real earned work.
    @param destination Unique retained process receipt.
    @return Verified complete native receipt and its original output.
    """
    previous = json.loads(source.read_text(encoding="utf-8"))
    if destination.exists():
        value = json.loads(destination.read_text(encoding="utf-8"))
        assert value["complete"] and value["configuration"] == configuration and value["phase"] == phase
        assert value["operation"]["receipt"] == identity(source)
        assert value["operation"]["source"] == previous["operation"]["output"]
        assert identity(ROOT / value["operation"]["output"]["path"]) == value["operation"]["output"]
    else:
        operation = DRIVER(configuration, source, phase, reserve=150000)
        value = dict(_documentation=__doc__, complete=True, configuration=configuration, phase=phase, operation=operation)
        destination.write_text(json.dumps(value, indent=2) + "\n", encoding="utf-8")
    print(configuration, phase, destination.name, value["operation"]["final"]["company"], flush=True)
    return value


def execute(configuration, start_period, periods):
    """@param configuration Actual native build. @param start_period Selected completed annual receipt.
    @param periods One or two further bounded periods. @return Continuous work/repayment membership and fresh affordability.
    """
    foundation_path = ROOT / "logs/surface-entry-income-fleet-reload-verification.json"
    foundation = json.loads(foundation_path.read_text(encoding="utf-8"))
    assert foundation["complete"] and foundation["actual_reliable_income"]
    previous = ROOT / f"logs/surface-entry-income-earned-{configuration}-{start_period}.json"
    paths, affordable, completed_period = [], False, start_period
    # Native cash above the selected reserve is repaid in matching10000bank
    # steps. Only subsequent actual work can realize reduced interest; no
    # quoted saving or replay branch contributes to the selected company's cash.
    for period in range(start_period, start_period + periods):
        value = json.loads(previous.read_text(encoding="utf-8"))
        company = value["operation"]["final"]["company"]
        if company["cash"] >= 160000:
            paid = ROOT / f"logs/surface-entry-income-renewed-repaid-{configuration}-{period}.json"
            boundary(configuration, previous, "repay", paid); previous = paid; paths.append(identity(paid))
        earned = ROOT / f"logs/surface-entry-income-earned-{configuration}-{period + 1}.json"
        value = boundary(configuration, previous, "earn", earned)
        paths.append(identity(earned)); previous = earned; completed_period = period + 1
        affordable = value["operation"]["rows"]["earned"][0]["affordable"]
        if affordable:
            break
    return dict(_documentation=__doc__, complete=True, configuration=configuration, foundation=identity(foundation_path),
                from_period=start_period, last_period=completed_period, receipts=paths, output=value["operation"]["output"],
                affordable=affordable, source=identity(Path(__file__)))


def main():
    """Retain one short sequential batch; leave full funding unchecked until actual current affordability passes."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), required=True)
    parser.add_argument("--from-period", type=int, required=True)
    parser.add_argument("--periods", type=int, choices=(1, 2), default=2)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    output = (ROOT / args.output).resolve()
    assert args.from_period >= 27 and output.is_relative_to(ROOT / "logs") and not output.exists()
    output.write_text(json.dumps(execute(args.configuration, args.from_period, args.periods), indent=2) + "\n", encoding="utf-8")
    print(output.relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
