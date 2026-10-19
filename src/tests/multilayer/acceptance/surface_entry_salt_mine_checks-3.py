"""Bind actual portal approach repair, pure capital quotes and matching Salt-mine credit.

@file surface_entry_salt_mine_checks-3.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15BG.1.
The ordinary repaired route and real bank loan precede any funded mine. This
proof never treats the unused ceiling or a pure price query as granted stock.
"""
import argparse
import json
from pathlib import Path

from space_preview_checks import ROOT, chunks, identity


def bind(specification):
    """Verify the exact genuine predecessor, native repair and minimum paid-capital loan.

    @param specification Explicit selected predecessor/repair/credit memberships and retained products.
    @return Completed native capital boundary, with actual mine funding still open.
    """
    spec = json.loads(specification.read_text())
    aliases = {row["sha256"]: ROOT / row["path"] for row in spec["retained"]}
    processes, outputs, summaries, comparisons = set(), [], [], 0

    def checked(reference):
        """@param reference Exact original artifact identity. @return Its verified original or retained path."""
        path = aliases.get(reference["sha256"], ROOT / reference["path"])
        assert identity(path)["sha256"] == reference["sha256"], path
        return path

    def walk(value):
        """@param value Immediate native receipt. Verify each exact original identity without historical graph replay."""
        if isinstance(value, dict):
            if set(value) == {"path", "sha256"}:
                checked(value)
            else:
                for child in value.values():
                    walk(child)
        elif isinstance(value, list):
            for child in value:
                walk(child)

    # Each build uses its real paid-facility checkpoint, repairs only the
    # required door/platform boundary, then borrows against actual quotes.
    # No failed diagnostic output or same-seed replacement world can enter.
    for entry in spec["configurations"]:
        source = json.loads((ROOT / entry["predecessor"]).read_text())["operation"]["output"]
        physical = []
        previous = source
        for phase in ("repair", "credit"):
            path = entry[phase]
            value = json.loads((ROOT / path).read_text())
            assert value["complete"] and value["configuration"] == entry["configuration"]
            walk(value)
            op = value["operation"]
            assert op["source"] == previous and op["simulation_ticks"] == 0
            for reference in op["native_processes"]:
                child = json.loads(checked(reference).read_text())
                assert child["exit_status"] == 0
                walk(child)
                processes.add(reference["path"])
            previous = op["output"]
            physical.append(chunks(checked(previous))[1])
            if phase == "repair":
                assert sum(row["cost"] for row in op["invoices"]) == 90 and op["portal"]["state"] == 2
            else:
                assert chunks(checked(op["before"])) == chunks(checked(op["query"]))
                assert op["capital"] == 781143 and op["mine_quote"] == 616584
                assert op["credit"] == 750000 and op["buffer"] == 100000 and op["ceiling"] == 5000000
                assert op["final"]["cash"] == 889242 and op["final"]["debt"] == 4250000
                assert op["final"]["parked_steel"] == 36 and op["final"]["parked_goods"] == 14
        outputs.append(physical)
        summaries.append(dict(configuration=entry["configuration"], output=previous, repair=entry["repair"], credit=entry["credit"]))
    for a, b in zip(*outputs, strict=True):
        assert a.keys() == b.keys() and all(a[key] == b[key] for key in a if key != b"GLOG")
        comparisons += len(a) - 1
    assert len(processes) == 4 and comparisons == 240
    return dict(_documentation=__doc__, complete=True, specification=identity(specification), configurations=summaries,
                native_processes=4, exact_cross_build_gameplay_comparisons=comparisons,
                paid_route_repair=90, native_credit=750000, cash=889242, debt=4250000, unchanged_ceiling=5000000,
                actual_mine_funded=False, supplied_stock_or_floor=False, source=identity(Path(__file__)))


def main():
    """Publish the exact original bank/route boundary without rerunning native work."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--specification", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    specification, target = [(ROOT / item).resolve() for item in (args.specification, args.output)]
    assert specification.is_relative_to(ROOT) and specification.is_file()
    assert target.is_relative_to(ROOT / "logs") and not target.exists()
    target.write_text(json.dumps(bind(specification), indent=2) + "\n", encoding="utf-8")
    print(target.relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
