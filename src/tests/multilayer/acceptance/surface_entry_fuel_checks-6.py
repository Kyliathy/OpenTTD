"""Bind original commercial purchases and thirteen real native economic periods.

@file surface_entry_fuel_checks-6.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15BI.2.
This accepted financial checkpoint proves actual income and history rollover,
while full PCHM affordability and the later native service refinement stay open.
"""
import argparse
import json
from pathlib import Path

from space_preview_checks import ROOT, chunks, identity


def bind():
    """@return Verified native purchase/earning lineage and exact both-build gameplay/accounting checkpoint."""
    aliases = {}
    for name in ("income-funding-rel-products", "income-year-one-products", "income-thirteen-years-products"):
        manifest = json.loads((ROOT / "test-cache/surface-entry" / name / "retained.json").read_text(encoding="utf-8"))
        for row in manifest["retained"]:
            aliases[row["sha256"]] = ROOT / row["path"]

    def checked(reference):
        """@param reference Exact original artifact. @return Verified original or explicitly retained immutable alias."""
        path = aliases.get(reference["sha256"], ROOT / reference["path"])
        assert identity(path)["sha256"] == reference["sha256"], path
        return path

    def walk(value):
        """@param value Immediate receipt/process tree. Verify every actual native artifact without replaying prior groups."""
        if isinstance(value, dict):
            if set(value) == {"path", "sha256"}:
                checked(value)
            else:
                for child in value.values():
                    walk(child)
        elif isinstance(value, list):
            for child in value:
                walk(child)

    processes, snapshots, summaries = set(), [], []
    for configuration in ("RelWithDebInfo", "Debug"):
        paths = [ROOT / f"logs/surface-entry-income-funded-{configuration}.json"] + [ROOT / f"logs/surface-entry-income-earned-{configuration}-{period}.json" for period in range(1, 14)]
        previous, physical, rows = None, [], []
        total_income, total_ticks = 0, 0
        for index, path in enumerate(paths):
            value = json.loads(path.read_text(encoding="utf-8")); walk(value)
            assert value["complete"] and value["configuration"] == configuration
            op = value["operation"]
            if previous is not None:
                assert op["source"] == previous
            previous = op["output"]
            assert op["initial"]["reserves"] == op["final"]["reserves"] == dict(steel=36, goods=14, chemicals=21)
            assert op["initial"]["company"]["debt"] == op["final"]["company"]["debt"] == 4650000
            if index:
                earned, = op["rows"]["earned"]
                assert value["phase"] == "earn" and earned["earned"] == 27000 and not earned["affordable"]
                assert earned["cash"] == op["final"]["company"]["cash"]
                assert earned["net_expense"] == sum(row["amount"] for row in op["rows"]["expenses"])
                assert earned["cash_before"] - earned["cash"] == earned["net_expense"]
                total_ticks += earned["earned"]; total_income += earned["income"]
            else:
                assert value["phase"] == "fund" and op["final"]["company"]["cash"] == 156146
            for reference in op["native_processes"]:
                process = json.loads(checked(reference).read_text(encoding="utf-8")); walk(process)
                assert process["exit_status"] == 0; processes.add(reference["path"])
            physical.extend(chunks(checked(op[name]))[1] for name in ("before", "output"))
            rows.append(op["rows"])
        assert total_ticks == 351000
        assert op["final"]["company"]["cash"] == 300439
        snapshots.append((physical, rows))
        summaries.append(dict(configuration=configuration, receipts=[identity(path) for path in paths], output=previous,
                              actual_earned_ticks=total_ticks, actual_freight_income=total_income, cash=300439, debt=4650000, net_since_funding=144293))
    assert snapshots[0][1] == snapshots[1][1]
    comparisons = 0
    for first, second in zip(snapshots[0][0], snapshots[1][0], strict=True):
        assert first.keys() == second.keys() and len(first) == 121
        assert all(first[key] == second[key] for key in first if key != b"GLOG")
        comparisons += 120
    assert len(processes) == 28 and comparisons == 3360
    return dict(_documentation=__doc__, complete=True, configurations=summaries, native_processes=28,
                exact_cross_build_gameplay_comparisons=comparisons, full_propellant_affordability=False, source=identity(Path(__file__)))


def main():
    """Publish the genuine commercial checkpoint without closing the remaining finance task."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, required=True)
    output = (ROOT / parser.parse_args().output).resolve()
    assert output.is_relative_to(ROOT / "logs") and not output.exists()
    output.write_text(json.dumps(bind(), indent=2) + "\n", encoding="utf-8")
    print(output.relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
