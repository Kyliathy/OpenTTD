"""Bind genuine native first excavation, spoil haulage/disposal and ordinary replay.

@file surface_entry_access_checks-7.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15BE.3.
Already completed paid access is the sole predecessor. Native load/revision
history is checked independently from the exact physical continuation comparison.
"""
import argparse
import json
from pathlib import Path
import runpy

from space_preview_checks import ROOT, chunks, identity

DRIVE = runpy.run_path(str(Path(__file__).with_name("surface_entry_access_checks-5.py")))
REVISION = runpy.run_path(str(Path(__file__).with_name("surface_entry_checks-7.py")))


def bind(specification):
    """Validate exact actual work ancestry, conservation and both-build continuations.

    @param specification Original accepted process memberships and explicit retained products.
    @return Complete first-excavation acceptance, with genuine mining/fuel still unfinished.
    """
    spec = json.loads(specification.read_text())
    aliases = {row["sha256"]: ROOT / row["path"] for row in spec["retained"]}
    processes, outputs, summaries, comparisons, replay_comparisons = set(), [], [], 0, 0

    def checked(reference):
        """@param reference Exact original artifact. @return Its verified original or retained path."""
        path = aliases.get(reference["sha256"], ROOT / reference["path"])
        assert identity(path)["sha256"] == reference["sha256"], path
        return path

    def walk(value):
        """@param value Nested immediate native membership. Verify every identity without historical graph traversal."""
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
        """@param path Exact accepted operation. @param configuration Expected application build. @return Its verified physical process."""
        value = json.loads((ROOT / path).read_text())
        assert value["complete"] and value["configuration"] == configuration
        walk(value)
        op = value["operation"]
        child_op = op.get("process", op)
        for reference in child_op["native_processes"]:
            child = json.loads(checked(reference).read_text())
            assert child["exit_status"] == 0
            walk(child)
            processes.add(reference["path"])
        return op, child_op

    # Register exact dated wrapper copies before checking old memberships.
    # They preserve verified early phases whose later verifier gained the
    # explicit finite seam-debit check; no accepted child is replaced or rerun.
    for entry in spec["configurations"]:
        for path in [*entry["work"], entry["restored"], *entry["replays"]]:
            op = json.loads((ROOT / path).read_text())["operation"]
            reference = op.get("retained_harness")
            if reference:
                aliases[reference["sha256"]] = ROOT / reference["path"]
    for entry in spec["configurations"]:
        configuration = entry["configuration"]
        purchase = json.loads((ROOT / entry["machinery"]).read_text())["operation"]
        previous = purchase["output"]
        physical = []
        earned = 0
        for path in entry["work"]:
            op, native = operation(path, configuration)
            assert op["source"] == previous
            previous = op["output"]
            physical.append(chunks(checked(previous))[1])
            earned += op["earned_ticks"]
        assert earned == 6000
        complete = DRIVE["observe"](checked(previous))
        assert complete["ready"] and complete["job"]["cost"] == 450
        assert set(complete["floor"]) == set(purchase["reserved_solid_cells"])
        assert complete["spoil"]["earth"]["processed"] == 40 and complete["spoil"]["earth"]["waiting"] == 5
        rock = complete["spoil"]["rock"]
        assert rock["processed"] == 88 and rock["waiting"] == 2
        assert rock["receiver"]["aggregate_made"] == 66 and rock["receiver"]["residue"] == 22
        assert sum(row["receiver"]["fees"] for row in complete["spoil"].values()) == 360
        restored, native = operation(entry["restored"], configuration)
        assert native["source"] == previous and restored["earned_ticks"] == 0
        physical.append(chunks(checked(native["output"]))[1])
        replay = []
        for path, source in zip(entry["replays"], (previous, native["output"]), strict=True):
            op, _ = operation(path, configuration)
            assert op["source"] == source and op["earned_ticks"] == 3000 and op["work"]["ready"]
            before = checked(op["output"]).with_name("surface-entry-before-replay-440.sav")
            table = DRIVE["OWNERS"]["saved_table"]
            if table(checked(source), b"GLOG") != table(before, b"GLOG"):
                REVISION["known_revision"](checked(source), before)
            assert table(before, b"GLOG") == table(checked(op["output"]), b"GLOG")
            replay.append(chunks(checked(op["output"]))[1])
            physical.append(replay[-1])
        # The resaved branch has two individually validated revision entries:
        # the actual ordinary loader and the focused application returning to it.
        # Every other native owner must match the same earned3000tick branch.
        assert replay[0].keys() == replay[1].keys()
        assert all(replay[0][key] == replay[1][key] for key in replay[0] if key != b"GLOG")
        replay_comparisons += len(replay[0]) - 1
        outputs.append(physical)
        summaries.append(dict(configuration=configuration, output=previous, work=entry["work"], restored=entry["restored"],
                              replays=entry["replays"], accepted=complete))
    for a, b in zip(*outputs, strict=True):
        assert a.keys() == b.keys() and all(a[key] == b[key] for key in a if key != b"GLOG")
        comparisons += len(a) - 1
    assert len(processes) == 12 and comparisons == 720 and replay_comparisons == 240
    return dict(_documentation=__doc__, complete=True, specification=identity(specification), configurations=summaries,
                native_processes=12, exact_cross_build_gameplay_comparisons=comparisons, exact_replay_gameplay_comparisons=replay_comparisons,
                earned_work_ticks_per_build=6000, earned_replay_ticks_per_process=3000, floor_cells=15,
                earth_dug=45, rock_dug=90, native_cut_cost=450, receiver_fees=360, finite_salt_remaining=28000,
                supplied_stock_floor_or_progress=False, genuine_salt_mining_or_fuel_complete=False, source=identity(Path(__file__)))


def main():
    """Publish the exact first-cut/haulage proof without rerunning its original processes."""
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
