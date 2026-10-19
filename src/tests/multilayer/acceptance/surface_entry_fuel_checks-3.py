"""Bind actual Propellant capital and original freight payment/carrier boundaries.

@file surface_entry_fuel_checks-3.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15BI.1.
This partial characterization selects no commercial route and earns no income.
Both actual pure-world queries must preserve every physical/financial owner.
"""
import argparse
import json
from pathlib import Path

from space_preview_checks import ROOT, chunks, identity


def bind(specification):
    """Verify original payment gates and the real budget/provider/carrier snapshot.

    @param specification Exact both-build receipts and retained executable/source aliases.
    @return Partial original financial characterization with route/funding still open.
    """
    spec = json.loads(specification.read_text(encoding="utf-8"))
    foundation = json.loads((ROOT / spec["chemicals"]).read_text(encoding="utf-8"))
    assert foundation["complete"] and foundation["collected_chemicals"] == 21
    aliases = {row["sha256"]: ROOT / row["path"] for row in spec["retained"]}
    processes, physical, summaries, quote_rows = set(), [], [], []

    def checked(reference):
        """@param reference Exact original identity. @return Verified original or explicitly retained artifact."""
        path = aliases.get(reference["sha256"], ROOT / reference["path"])
        assert identity(path)["sha256"] == reference["sha256"], path
        return path

    def walk(value):
        """@param value Immediate receipt/process tree. Verify identities without replaying older groups."""
        if isinstance(value, dict):
            if set(value) == {"path", "sha256"}:
                checked(value)
            else:
                for child in value.values():
                    walk(child)
        elif isinstance(value, list):
            for child in value:
                walk(child)

    # The ordinary positive-tariff boundary and actual unchanged world are
    # independently required in each configuration before selecting new spending.
    for entry in spec["configurations"]:
        values = [json.loads((ROOT / entry[name]).read_text(encoding="utf-8")) for name in ("producer", "query")]
        for value in values:
            assert value["complete"] and value["configuration"] == entry["configuration"]
            walk(value)
            for reference in value["operation"]["native_processes"]:
                process = json.loads(checked(reference).read_text(encoding="utf-8"))
                assert process["exit_status"] == 0
                walk(process)
                processes.add(reference["path"])
        gate, op = [value["operation"] for value in values]
        assert gate["cases"] == 1 and gate["assertions"] == 88
        source, = [row["output"] for row in foundation["configurations"] if row["configuration"] == entry["configuration"]]
        assert op["source"] == source and op["simulation_ticks"] == 0 and op["unchanged_native_families"] == 121
        before, after = [chunks(checked(op[name]))[1] for name in ("before", "output")]
        assert before == after
        capital, = op["quotes"]["capital"]
        assert capital["cash"] == 181985 and capital["debt"] == 4650000 and capital["ceiling"] == 5000000
        assert capital["factory"] == 703125 and capital["tanker"] == 4593 and capital["buffer"] == 100000
        assert capital["headroom"] == 350000 and capital["empty_train_sale"] == 145769
        assert capital["available_after_sale"] == 677754 and capital["shortfall"] == 129964
        assert not op["selected_income_route"] and not op["supplied_stock_or_capital"]
        physical.append((before, after))
        quote_rows.append(op["quotes"])
        summaries.append(dict(configuration=entry["configuration"], source=source, query=entry["query"], quotes=op["quotes"]))
    assert quote_rows[0] == quote_rows[1]
    comparisons = 0
    for a, b in zip(*physical, strict=True):
        assert a.keys() == b.keys() and all(a[key] == b[key] for key in a if key != b"GLOG")
        comparisons += len(a) - 1
    assert len(processes) == 4 and comparisons == 240
    return dict(_documentation=__doc__, complete=True, specification=identity(specification), chemicals=identity(ROOT / spec["chemicals"]),
                configurations=summaries, native_processes=4, exact_cross_build_gameplay_comparisons=comparisons,
                producer_cases_per_build=1, producer_assertions_per_build=88, shortfall_including_buffer=129964,
                selected_income_route=False, actual_new_finance_or_fuel_complete=False, source=identity(Path(__file__)))


def main():
    """Publish partial actual budget/carrier characterization without closing the task."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--specification", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    specification, target = [(ROOT / path).resolve() for path in (args.specification, args.output)]
    assert specification.is_relative_to(ROOT / "logs") and specification.is_file()
    assert target.is_relative_to(ROOT / "logs") and not target.exists()
    target.write_text(json.dumps(bind(specification), indent=2) + "\n", encoding="utf-8")
    print(target.relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
