"""Bind ordinary reload and exact native continuations of genuinely transported Surface Salt.

@file surface_entry_salt_mine_checks-9.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15BG.4.
Actual finite mining continues during replay while the original60Salt stays
in paid Surface custody. Qualified load revisions are verified independently.
"""
import argparse
import json
from pathlib import Path
import runpy

from space_preview_checks import ROOT, chunks, identity

HAUL = runpy.run_path(str(Path(__file__).with_name("surface_entry_salt_mine_checks-6.py")))
REVISION = runpy.run_path(str(Path(__file__).with_name("surface_entry_checks-7.py")))


def bind(specification):
    """Verify exact original Salt custody, ordinary loads and matched native future branches.

    @param specification Selected real-haulage foundation, reload/replay receipts and retained products.
    @return Completed genuine Salt prerequisite with fuel/launch work still open.
    """
    spec = json.loads(specification.read_text())
    foundation = json.loads((ROOT / spec["haulage"]).read_text())
    assert foundation["complete"] and foundation["surface_salt"] == 60
    aliases = {row["sha256"]: ROOT / row["path"] for row in spec["retained"]}
    processes, outputs, summaries, comparisons, replay_comparisons = set(), [], [], 0, 0

    def checked(reference):
        """@param reference Exact original artifact. @return Its verified original or explicit retained path."""
        path = aliases.get(reference["sha256"], ROOT / reference["path"])
        assert identity(path)["sha256"] == reference["sha256"], path
        return path

    def walk(value):
        """@param value Immediate native receipt. Verify all original artifacts without historical graph replay."""
        if isinstance(value, dict):
            if set(value) == {"path", "sha256"}:
                checked(value)
            else:
                for child in value.values():
                    walk(child)
        elif isinstance(value, list):
            for child in value:
                walk(child)

    def operation(path, configuration):
        """@param path Accepted original JSON. @param configuration Expected application build. @return Verified operation/native child."""
        value = json.loads((ROOT / path).read_text())
        assert value["complete"] and value["configuration"] == configuration
        walk(value)
        op = value["operation"]
        native = op.get("process", op)
        for reference in native["native_processes"]:
            child = json.loads(checked(reference).read_text())
            assert child["exit_status"] == 0
            walk(child)
            processes.add(reference["path"])
        return op, native

    # Direct and ordinary-resaved inputs must describe the same actual paid
    # world. Each future branch earns exactly3000native ticks, preserving all
    # original60vehicle-owned Salt while finite industry production continues.
    for entry in spec["configurations"]:
        configuration = entry["configuration"]
        source, = [row["output"] for row in foundation["configurations"] if row["configuration"] == configuration]
        restored, ordinary = operation(entry["restored"], configuration)
        assert ordinary["source"] == source and restored["earned_ticks"] == 0
        assert restored["initial"] == restored["final"] and restored["final"]["surface_custody"]
        physical = [chunks(checked(ordinary["output"]))[1]]
        branch_outputs = []
        for path, previous in zip(entry["replays"], (source, ordinary["output"]), strict=True):
            op, _ = operation(path, configuration)
            assert op["source"] == previous and op["earned_ticks"] == 3000
            assert op["native"]["surface_custody"] and op["native"]["wagon"]["held"] == 60
            assert op["native"]["mine"]["reserve"] == 23516 and op["native"]["mine"]["mine"]["extracted"] == 484
            assert op["final"]["debt"] == 4250000 and op["final"]["parked_steel"] == 36 and op["final"]["parked_goods"] == 14
            before = checked(op["output"]).with_name("surface-entry-before-replay-440.sav")
            table = HAUL["OWNERS"]["saved_table"]
            if table(checked(previous), b"GLOG") != table(before, b"GLOG"):
                REVISION["known_revision"](checked(previous), before)
            assert table(before, b"GLOG") == table(checked(op["output"]), b"GLOG")
            branch_outputs.append(chunks(checked(op["output"]))[1])
            physical.append(branch_outputs[-1])
        assert branch_outputs[0].keys() == branch_outputs[1].keys()
        assert all(branch_outputs[0][key] == branch_outputs[1][key] for key in branch_outputs[0] if key != b"GLOG")
        replay_comparisons += len(branch_outputs[0]) - 1
        outputs.append(physical)
        summaries.append(dict(configuration=configuration, source=source, restored=entry["restored"], replays=entry["replays"]))
    for a, b in zip(*outputs, strict=True):
        assert a.keys() == b.keys() and all(a[key] == b[key] for key in a if key != b"GLOG")
        comparisons += len(a) - 1
    assert len(processes) == 6 and comparisons == 360 and replay_comparisons == 240
    return dict(_documentation=__doc__, complete=True, specification=identity(specification), haulage=identity(ROOT / spec["haulage"]), configurations=summaries,
                native_processes=6, exact_cross_build_gameplay_comparisons=comparisons, exact_native_replay_gameplay_comparisons=replay_comparisons,
                surface_salt=60, original_reserve=23679, continued_reserve=23516, continued_extraction=484, native_replay_ticks=3000,
                supplied_stock_or_pose=False, actual_surface_fuel_complete=False, source=identity(Path(__file__)))


def main():
    """Publish genuine Salt ordinary-reload acceptance from exact already completed native receipts."""
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
