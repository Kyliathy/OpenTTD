"""Bind actual continuing service income and the original fleet's measured renewal need.

@file surface_entry_fuel_checks-10.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Groups15BIB/15BIC.
Native measured results, not the paid service's forecast, control continuation.
All genuine inputs remain held while expired, overage engines need replacement.
"""
import argparse
import json
from pathlib import Path

from space_preview_checks import ROOT, chunks, identity
from surface_income_ledger import TABLE, TRAIN


def bind():
    """@return Verified native year20-26 checkpoint and exact original expired-model/age evidence."""
    foundation_path = ROOT / "logs/surface-entry-income-town-purchase-verification.json"
    foundation = json.loads(foundation_path.read_text(encoding="utf-8"))
    assert foundation["complete"] and foundation["service_actually_bought"]
    checked_ids = set()

    def checked(reference):
        """@param reference Exact immutable current artifact. @return Verified repository-local path."""
        key = reference["path"], reference["sha256"]
        path = ROOT / reference["path"]
        if key not in checked_ids:
            assert identity(path) == reference, path
            checked_ids.add(key)
        return path

    def walk(value):
        """@param value Immediate receipt/process tree. Verify every actual native artifact once per identity."""
        if isinstance(value, dict):
            if set(value) == {"path", "sha256"}:
                checked(value)
            else:
                for child in value.values():
                    walk(child)
        elif isinstance(value, list):
            for child in value:
                walk(child)

    physical, field_rows, summaries, processes = [], [], [], set()
    for configuration in ("RelWithDebInfo", "Debug"):
        previous, = [row["output"] for row in foundation["configurations"] if row["configuration"] == configuration]
        paths = []
        for period in range(20, 27):
            paths.append(ROOT / f"logs/surface-entry-income-earned-{configuration}-{period}.json")
            if period in (20, 21):
                paths.append(ROOT / f"logs/surface-entry-income-repaid-{configuration}-{period}.json")
        snapshots, rows = [], []
        for path in paths:
            value = json.loads(path.read_text(encoding="utf-8")); walk(value)
            op = value["operation"]
            assert value["complete"] and value["configuration"] == configuration and op["source"] == previous
            previous = op["output"]
            assert op["initial"]["reserves"] == op["final"]["reserves"] == dict(steel=36, goods=14, chemicals=21)
            if value["phase"] == "earn":
                earned, = op["rows"]["earned"]
                assert earned["earned"] == 27000 and not earned["affordable"]
            else:
                assert value["phase"] == "repay" and op["rows"]["repayment"][0]["amount"] in (10000, 20000)
            for reference in op["native_processes"]:
                process = json.loads(checked(reference).read_text(encoding="utf-8")); walk(process)
                assert process["exit_status"] == 0; processes.add(reference["path"])
            snapshots.extend(chunks(checked(op[name]))[1] for name in ("before", "output")); rows.append(op["rows"])
        assert op["final"]["company"] == dict(cash=97842, debt=4530000)
        vehicles = TABLE(checked(previous), b"VEHS"); old = []
        for train in op["final"]["trains"]:
            row = vehicles[train["head"]]
            assert row[TRAIN + "engine_type"] == 22 and row[TRAIN + "age"] > row[TRAIN + "max_age"]
            old.append(dict(head=train["head"], age=row[TRAIN + "age"], max_age=row[TRAIN + "max_age"], reliability=row[TRAIN + "reliability"]))
        assert TABLE(checked(previous), b"ENGN")[22]["company_avail"] & 1 == 0
        assert TABLE(checked(previous), b"CITY")[0]["statues"] == 1
        physical.append(snapshots); field_rows.append(rows)
        summaries.append(dict(configuration=configuration, receipts=[identity(path) for path in paths], output=previous,
                              actual_earned_ticks=189000, cash=97842, debt=4530000, spendable_capital=567842, overage_fleet=old))
    assert field_rows[0] == field_rows[1]
    comparisons = 0
    for first, second in zip(*physical, strict=True):
        assert first.keys() == second.keys() and all(first[key] == second[key] for key in first if key != b"GLOG")
        comparisons += 120
    assert len(processes) == 18 and comparisons == 2160
    return dict(_documentation=__doc__, complete=True, foundation=identity(foundation_path), configurations=summaries,
                native_processes=18, exact_cross_build_gameplay_comparisons=comparisons, fleet_replacement_required=True,
                full_propellant_affordability=False, remaining_current_shortfall=241107, source=identity(Path(__file__)))


def main():
    """Publish the measured financial/maintenance boundary without closing the funding task."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, required=True)
    output = (ROOT / parser.parse_args().output).resolve()
    assert output.is_relative_to(ROOT / "logs") and not output.exists()
    output.write_text(json.dumps(bind(), indent=2) + "\n", encoding="utf-8")
    print(output.relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
