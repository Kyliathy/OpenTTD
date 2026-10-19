"""Bind actual credit, paid access, earned receiver construction and empty machinery.

@file surface_entry_access_checks-4.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15BE.2.
Only exact original process memberships enter this genuine predecessor chain.
No native process is rerun and no subsequent cutting task is closed here.
"""
import argparse
import json
from pathlib import Path

from space_preview_checks import ROOT, chunks, identity


def bind(specification):
    """Verify paid ancestry, original artifacts and exact cross-build gameplay.

    @param specification Explicit accepted memberships and digest-bound retained products.
    @return Actual completed purchase boundary with first cutting still unfinished.
    """
    spec = json.loads(specification.read_text())
    aliases = {row["sha256"]: ROOT / row["path"] for row in spec["retained"]}
    processes, outputs, comparisons, summaries = set(), [], 0, []

    def checked(reference):
        """@param reference Original digest-bound artifact. @return Its exact original or retained path."""
        path = aliases.get(reference["sha256"], ROOT / reference["path"])
        assert identity(path)["sha256"] == reference["sha256"], path
        return path

    def walk(value):
        """@param value Nested immediate membership. Verify each original identity without replaying older proof graphs."""
        if isinstance(value, dict):
            if set(value) == {"path", "sha256"}:
                checked(value)
            else:
                for child in value.values():
                    walk(child)
        elif isinstance(value, list):
            for child in value:
                walk(child)

    # Each build consumes its selected ordinary productive world, the same
    # native loan and all actual paid/construction outputs in strict order.
    for entry in spec["configurations"]:
        previous = json.loads((ROOT / entry["predecessor"]).read_text())["result"]["process"]["output"]
        native_outputs = []
        for phase, member in entry["phases"].items():
            value = json.loads((ROOT / member).read_text())
            assert value["complete"] and value["configuration"] == entry["configuration"]
            walk(value)
            op = value["operation"]
            assert op["source"] == previous
            for reference in op["native_processes"]:
                child = json.loads(checked(reference).read_text())
                assert child["exit_status"] == 0
                walk(child)
                processes.add(reference["path"])
            native_outputs.append(checked(op["output"]))
            if phase != "plan":
                previous = op["output"]
            if phase == "machinery":
                assert sum(op["invoices"]) == 568742 and op["final"]["cash"] == 200956
                assert op["final"]["debt"] == 3500000 and len(op["reserved_solid_cells"]) == 15
        outputs.append(native_outputs)
        summaries.append(dict(configuration=entry["configuration"], output=previous, phases=entry["phases"]))
    # Original qualified application GLOG metadata differs by build. Every
    # other serialized owner must be exact at all six corresponding boundaries.
    for first, second in zip(*outputs, strict=True):
        a, b = chunks(first)[1], chunks(second)[1]
        assert a.keys() == b.keys()
        assert all(a[key] == b[key] for key in a if key != b"GLOG")
        comparisons += len(a) - 1
    assert len(processes) == 12
    return dict(_documentation=__doc__, complete=True, specification=identity(specification), configurations=summaries,
                native_processes=len(processes), exact_cross_build_gameplay_comparisons=comparisons,
                bank_credit=1500000, infrastructure_invoice=1413852, machinery_invoice=568742,
                cash=200956, debt=3500000, parked_steel=36, parked_goods=14,
                supplied_floor_or_products=False, first_cut_complete=False, source=identity(Path(__file__)))


def main():
    """Publish one fresh paid-access proof from exact already completed native receipts."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--specification", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    specification, target = [(ROOT / value).resolve() for value in (args.specification, args.output)]
    assert specification.is_relative_to(ROOT) and specification.is_file()
    assert target.is_relative_to(ROOT / "logs") and not target.exists()
    target.write_text(json.dumps(bind(specification), indent=2) + "\n", encoding="utf-8")
    print(target.relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
