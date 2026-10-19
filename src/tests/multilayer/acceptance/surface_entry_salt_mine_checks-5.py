"""Bind actual paid finite Salt mine and earned complete construction in both builds.

@file surface_entry_salt_mine_checks-5.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15BG.2.
Original native work may produce finite output after the last machinery cell
completes. Neither a completed flag nor Salt stock is installed by the harness.
"""
import argparse
import json
from pathlib import Path

from space_preview_checks import ROOT, chunks, identity


def bind(specification):
    """Verify exact matched-credit ancestry, real mine payment and earned finite construction.

    @param specification Selected credit foundation, native funded/construction receipts and retained products.
    @return Actual completed mine prerequisite, with Surface transport still unfinished.
    """
    spec = json.loads(specification.read_text())
    foundation = json.loads((ROOT / spec["credit"]).read_text())
    assert foundation["complete"] and foundation["debt"] == 4250000
    aliases = {row["sha256"]: ROOT / row["path"] for row in spec["retained"]}
    processes, outputs, summaries, comparisons = set(), [], [], 0

    def checked(reference):
        """@param reference Exact original identity. @return Its verified original or explicit retained path."""
        path = aliases.get(reference["sha256"], ROOT / reference["path"])
        assert identity(path)["sha256"] == reference["sha256"], path
        return path

    def walk(value):
        """@param value Immediate original evidence. Verify all exact identities without historical graph replay."""
        if isinstance(value, dict):
            if set(value) == {"path", "sha256"}:
                checked(value)
            else:
                for child in value.values():
                    walk(child)
        elif isinstance(value, list):
            for child in value:
                walk(child)

    # A real paid native mine consumes its exact actual loan output, then
    # earns two bounded game-loop intervals. Its original finite output is
    # accepted only through the independent reserve/stock/packet ledger.
    for entry in spec["configurations"]:
        previous, = [row["output"] for row in foundation["configurations"] if row["configuration"] == entry["configuration"]]
        physical, earned = [], 0
        for index, path in enumerate(entry["work"]):
            value = json.loads((ROOT / path).read_text())
            assert value["complete"] and value["configuration"] == entry["configuration"]
            walk(value)
            op = value["operation"]
            assert op["source"] == previous
            for reference in op["native_processes"]:
                child = json.loads(checked(reference).read_text())
                assert child["exit_status"] == 0
                walk(child)
                processes.add(reference["path"])
            if index == 0:
                assert value["phase"] == "fund" and op["native"]["mine"]["extracted"] == 0 and op["native"]["complete"] is False
                assert op["final"]["cash"] == 272658
            earned += op["earned_ticks"]
            previous = op["output"]
            physical.append(chunks(checked(previous))[1])
        assert earned == 6000 and op["native"]["complete"]
        assert op["native"]["reserve"] == 23841 and op["native"]["waiting"] == op["native"]["mine"]["extracted"] == 159
        assert op["native"]["mine"]["dispatched"] == 0 and not op["native"]["dispatched_packets"]
        assert op["final"]["debt"] == 4250000 and op["final"]["parked_steel"] == 36 and op["final"]["parked_goods"] == 14
        outputs.append(physical)
        summaries.append(dict(configuration=entry["configuration"], output=previous, work=entry["work"], mine=op["native"], products=op["final"]))
    for a, b in zip(*outputs, strict=True):
        assert a.keys() == b.keys() and all(a[key] == b[key] for key in a if key != b"GLOG")
        comparisons += len(a) - 1
    assert len(processes) == 6 and comparisons == 360
    return dict(_documentation=__doc__, complete=True, specification=identity(specification), credit=identity(ROOT / spec["credit"]), configurations=summaries,
                native_processes=6, exact_cross_build_gameplay_comparisons=comparisons, mine=14, deposit=4, funding=616584,
                earned_ticks_per_build=6000, complete_machinery_cells=9, mined_salt=159, remaining_salt=23841,
                supplied_stock_or_progress=False, actual_surface_salt_custody=False, source=identity(Path(__file__)))


def main():
    """Publish completed genuine mine construction acceptance without replaying its native processes."""
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
