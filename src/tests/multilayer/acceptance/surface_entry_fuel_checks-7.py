"""Bind the pure Coal service plan or the actual empty-wagon replacement.

@file surface_entry_fuel_checks-7.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15BI.2.
The same original paid head and real native route survive this bounded repair.
Unsold book values, supplied positions and discarded loads cannot fund it.
"""
import argparse
import json
from pathlib import Path

from space_preview_checks import ROOT, chunks, identity


def bind(stage):
    """@param stage Pure pre-purchase plan or complete earned empty service boundary. @return Exact both-build proof."""
    foundation_path = ROOT / "logs/surface-entry-income-history-verification.json"
    foundation = json.loads(foundation_path.read_text(encoding="utf-8"))
    assert foundation["complete"] and not foundation["full_propellant_affordability"]
    phases = ("query",) if stage == "plan" else ("query", "recall", "settle", "rewagon")
    processes, physical, field_rows, summaries = set(), [], [], []

    def walk(value):
        """@param value Immediate receipt/process tree. Verify original retained artifact membership."""
        if isinstance(value, dict):
            if set(value) == {"path", "sha256"}:
                assert identity(ROOT / value["path"]) == value, value
            else:
                for child in value.values():
                    walk(child)
        elif isinstance(value, list):
            for child in value:
                walk(child)

    for configuration in ("RelWithDebInfo", "Debug"):
        previous, = [row["output"] for row in foundation["configurations"] if row["configuration"] == configuration]
        snapshots, rows, receipts = [], [], []
        for phase in phases:
            path = ROOT / f"logs/surface-entry-income-coal-{phase}-{configuration}.json"
            value = json.loads(path.read_text(encoding="utf-8")); walk(value)
            op = value["operation"]
            assert value["complete"] and value["configuration"] == configuration and value["phase"] == "coal-" + phase
            assert op["source"] == previous
            previous = op["output"]
            assert op["initial"]["reserves"] == op["final"]["reserves"] == dict(steel=36, goods=14, chemicals=21)
            assert op["initial"]["company"]["debt"] == op["final"]["company"]["debt"] == 4650000
            before, after = [chunks(ROOT / op[name]["path"])[1] for name in ("before", "output")]
            if phase == "query":
                assert before == after and len(before) == 121
                assert op["rows"]["coal_candidates"] == [dict(engine=29, cost=1031, capacity=30, speed=0, count=4)]
                assert op["rows"]["coal_quote"] == [dict(engine=29, invoice=4124, capacity=30, count=4, old_speed=80, depot=3161, cash=300439, debt=4650000)]
            elif phase == "settle":
                assert op["final"]["trains"][2]["stopped"] and op["final"]["trains"][2]["tile"] == 3161
                assert not any(op["final"]["trains"][2]["held"].values())
            elif phase == "rewagon":
                assert op["initial"]["trains"][2]["capacity"]["COAL"] == 244 and op["final"]["trains"][2]["capacity"]["COAL"] == 124
                repair, = op["rows"]["coal_rewagon"]
                assert repair["head"] == 38 and repair["engine"] == 29 and repair["speed"] == 200 and repair["length"] == 48
                sales = [row for row in op["rows"]["invoices"] if row["command"] == "CmdSellVehicle"]
                buys = [row for row in op["rows"]["invoices"] if row["command"] == "CmdBuildVehicle"]
                assert len(sales) == len(buys) == 4 and all(row["cost"] < 0 for row in sales)
                assert sum(row["cost"] for row in buys) == 4124
            for reference in op["native_processes"]:
                process = json.loads((ROOT / reference["path"]).read_text(encoding="utf-8")); walk(process)
                assert process["exit_status"] == 0; processes.add(reference["path"])
            snapshots.extend((before, after)); rows.append(op["rows"]); receipts.append(identity(path))
        physical.append(snapshots); field_rows.append(rows)
        summaries.append(dict(configuration=configuration, receipts=receipts, output=previous))
    assert field_rows[0] == field_rows[1]
    comparisons = 0
    for first, second in zip(*physical, strict=True):
        assert first.keys() == second.keys() and all(first[key] == second[key] for key in first if key != b"GLOG")
        comparisons += 120
    assert len(processes) == 2 * len(phases) and comparisons == 240 * len(phases)
    return dict(_documentation=__doc__, complete=True, stage=stage, foundation=identity(foundation_path), configurations=summaries,
                native_processes=len(processes), exact_cross_build_gameplay_comparisons=comparisons,
                actual_replacement_complete=stage == "replacement", full_propellant_affordability=False, source=identity(Path(__file__)))


def main():
    """Retain the exact original plan or real service boundary without claiming final fuel funding."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--stage", choices=("plan", "replacement"), required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    output = (ROOT / args.output).resolve()
    assert output.is_relative_to(ROOT / "logs") and not output.exists()
    output.write_text(json.dumps(bind(args.stage), indent=2) + "\n", encoding="utf-8")
    print(output.relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
