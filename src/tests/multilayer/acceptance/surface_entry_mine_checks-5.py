"""Bind genuinely reused machinery, paid support conversion and earned Salt mine gallery.

@file surface_entry_mine_checks-5.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15BF.2.
Only selected actual command/work receipts enter this chain. The earlier
unconverted-support obstruction remains a diagnostic branch, not acceptance.
"""
import argparse
import json
from pathlib import Path

from space_preview_checks import ROOT, chunks, identity


def bind(specification):
    """Verify actual zero-repurchase ancestry and exact finite gallery work in both builds.

    @param specification Explicit preparation handoff, work memberships and retained products.
    @return Completed paid-work boundary with mine/pickup construction still unfinished.
    """
    spec = json.loads(specification.read_text())
    foundation = json.loads((ROOT / spec["preparation"]).read_text())
    assert foundation["complete"] and foundation["paid_retained_cutter"] == 7
    aliases = {row["sha256"]: ROOT / row["path"] for row in spec["retained"]}
    processes, outputs, summaries, comparisons = set(), [], [], 0

    def checked(reference):
        """@param reference Original digest-bound artifact. @return Its verified exact original or retained path."""
        path = aliases.get(reference["sha256"], ROOT / reference["path"])
        assert identity(path)["sha256"] == reference["sha256"], path
        return path

    def walk(value):
        """@param value Original immediate membership. Verify each exact identity without historical producer replay."""
        if isinstance(value, dict):
            if set(value) == {"path", "sha256"}:
                checked(value)
            else:
                for child in value.values():
                    walk(child)
        elif isinstance(value, list):
            for child in value:
                walk(child)

    # Consume the actual retired predecessor, preserve every original hull,
    # and earn additional floor only through the selected native game ticks.
    for entry in spec["configurations"]:
        previous, = [row["retired_source"] for row in foundation["configurations"] if row["configuration"] == entry["configuration"]]
        earned, native_outputs = 0, []
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
            assert len(op["work"]["owners"]["trains"]) == 11
            if index == 0:
                assert value["phase"] == "commission" and op["invoices"] == [75, 75, 0, 0]
            earned += op["earned_ticks"]
            previous = op["output"]
            native_outputs.append(chunks(checked(previous))[1])
        assert earned == 6000 and op["work"]["ready"]
        assert op["final"]["cash"] == 140087 and op["final"]["debt"] == 3500000
        assert op["final"]["parked_steel"] == 36 and op["final"]["parked_goods"] == 14
        work = op["work"]
        assert work["floor"] == [4096 + y * 64 + x for y in range(29, 38) for x in range(28, 31)]
        assert work["salt_remaining"] == 24000
        assert work["spoil"]["earth"]["dug"] == 81 and work["spoil"]["earth"]["processed"] == 80 and work["spoil"]["earth"]["waiting"] == 1
        assert work["spoil"]["rock"]["dug"] == 162 and work["spoil"]["rock"]["processed"] == 160 and work["spoil"]["rock"]["waiting"] == 2
        assert sum(row["receiver"]["fees"] for row in work["spoil"].values()) == 648
        assert work["spoil"]["rock"]["receiver"]["aggregate_made"] == 120 and work["spoil"]["rock"]["receiver"]["residue"] == 40
        outputs.append(native_outputs)
        summaries.append(dict(configuration=entry["configuration"], output=previous, work=entry["work"], final=work))
    for a, b in zip(*outputs, strict=True):
        assert a.keys() == b.keys() and all(a[key] == b[key] for key in a if key != b"GLOG")
        comparisons += len(a) - 1
    assert len(processes) == 8 and comparisons == 480
    return dict(_documentation=__doc__, complete=True, specification=identity(specification), preparation=identity(ROOT / spec["preparation"]),
                configurations=summaries, native_processes=8, exact_cross_build_gameplay_comparisons=comparisons,
                support_conversion_cost=150, machine_repurchase=0, earned_ticks_per_build=6000, additional_floor_cells=12,
                additional_earth=36, additional_rock=72, additional_cut_cost=360, salt_remaining=24000,
                cash=140087, debt=3500000, actual_mine_funded=False, supplied_stock_floor_or_progress=False, source=identity(Path(__file__)))


def main():
    """Publish the exact genuinely worked gallery handoff without native reruns."""
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
