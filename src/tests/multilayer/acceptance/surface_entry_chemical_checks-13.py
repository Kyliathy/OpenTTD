"""Bind useful genuine chemical collection and ordinary reload/native continuation.

@file surface_entry_chemical_checks-13.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15BH.3.
All useful CHEM comes from the paid original factory and transported ingredients.
Ordinary/focused load revisions are individually checked; no ledger is waived.
"""
import argparse
import json
from pathlib import Path
import runpy

from space_preview_checks import ROOT, chunks, identity
from surface_chemical_ledger import TABLE, chemical_ledger, salt_ledger

LOAD_REVISION = runpy.run_path(str(Path(__file__).with_name("surface_entry_checks-7.py")))


def bind(specification):
    """Verify actual useful21CHEM, both ordinary applications and own-save replay.

    @param specification Exact immutable both-build work/restore/replay membership and retained aliases.
    @return Complete chemical group acceptance, leaving fuel/launch and final Group15open.
    """
    spec = json.loads(specification.read_text(encoding="utf-8"))
    foundation = json.loads((ROOT / spec["production"]).read_text(encoding="utf-8"))
    assert foundation["complete"] and foundation["chemicals_produced"] == 14
    aliases = {row["sha256"]: ROOT / row["path"] for row in spec["retained"]}
    processes, outputs, summaries = set(), [], []

    def checked(reference):
        """@param reference Exact original identity. @return Verified original or explicitly retained source/product."""
        path = aliases.get(reference["sha256"], ROOT / reference["path"])
        assert identity(path)["sha256"] == reference["sha256"], path
        return path

    def walk(value):
        """@param value Immediate receipt or native report. Verify artifacts without historical group replay."""
        if isinstance(value, dict):
            if set(value) == {"path", "sha256"}:
                checked(value)
            else:
                for child in value.values():
                    walk(child)
        elif isinstance(value, list):
            for child in value:
                walk(child)

    def operation(name, configuration):
        """@param name Exact selected receipt path. @param configuration Expected build. @return Verified original operation."""
        value = json.loads((ROOT / name).read_text(encoding="utf-8"))
        assert value["complete"] and value["configuration"] == configuration
        walk(value)
        op = value["operation"]
        for reference in op.get("process", op)["native_processes"]:
            process = json.loads(checked(reference).read_text(encoding="utf-8"))
            assert process["exit_status"] == 0
            walk(process)
            processes.add(reference["path"])
        return op

    replay_comparisons = 0
    # Only real loading/travel after actual production can establish physical
    # useful cargo. Ordinary restore and both continuations protect that same21.
    for entry in spec["configurations"]:
        configuration = entry["configuration"]
        previous, = [row["output"] for row in foundation["configurations"] if row["configuration"] == configuration]
        physical, earned = [], 0
        for index, name in enumerate(entry["work"]):
            op = operation(name, configuration)
            assert op["source"] == previous
            assert op["earned_ticks"] == (3000 if index == 0 else 732)
            earned += op["earned_ticks"]
            previous = op["output"]
            physical.append(chunks(checked(previous))[1])
        assert len(physical) == 2 and earned == 3732
        selected = previous
        chemicals, salt = chemical_ledger(checked(selected)), salt_ledger(checked(selected))
        assert chemicals["produced"] == 42 and chemicals["useful_custody"] and chemicals["reserve"]["held"] == 21
        assert salt["delivered"] == 60 and salt["wagon_held"] == 0
        restored = operation(entry["restored"], configuration)
        assert restored["source"] == selected and restored["earned_ticks"] == 0 and restored["initial"] == restored["final"]
        assert restored["chemicals"]["useful_custody"]
        physical.append(chunks(checked(restored["output"]))[1])
        branches = []
        for name, source in zip(entry["replays"], (selected, restored["output"]), strict=True):
            op = operation(name, configuration)
            assert op["source"] == source and op["earned_ticks"] == 3000
            assert op["chemicals"]["produced"] == 60 and op["chemicals"]["useful_custody"]
            assert op["chemicals"]["reserve"]["held"] == 21 and op["final"]["debt"] == 4650000
            assert op["final"]["parked_steel"] == 36 and op["final"]["parked_goods"] == 14
            before = checked(op["output"]).with_name("surface-entry-before-replay-440.sav")
            if TABLE(checked(source), b"GLOG") != TABLE(before, b"GLOG"):
                LOAD_REVISION["known_revision"](checked(source), before)
            assert TABLE(before, b"GLOG") == TABLE(checked(op["output"]), b"GLOG")
            branches.append(chunks(checked(op["output"]))[1])
            physical.append(branches[-1])
        assert branches[0].keys() == branches[1].keys()
        assert all(branches[0][key] == branches[1][key] for key in branches[0] if key != b"GLOG")
        replay_comparisons += len(branches[0]) - 1
        outputs.append(physical)
        summaries.append(dict(configuration=configuration, output=selected, work=entry["work"], restored=entry["restored"],
                              replays=entry["replays"], chemicals=chemicals, salt=salt))
    comparisons = 0
    for a, b in zip(*outputs, strict=True):
        assert a.keys() == b.keys() and all(a[key] == b[key] for key in a if key != b"GLOG")
        comparisons += len(a) - 1
    assert len(processes) == 10 and comparisons == 600 and replay_comparisons == 240
    return dict(_documentation=__doc__, complete=True, specification=identity(specification), production=identity(ROOT / spec["production"]),
                configurations=summaries, native_processes=10, exact_cross_build_gameplay_comparisons=comparisons,
                exact_native_replay_gameplay_comparisons=replay_comparisons, collected_chemicals=21, source_industry=15,
                original_chemical_production=42, replay_chemical_production=60, supplied_stock_or_pose=False,
                actual_propellant_or_launch_complete=False, source=identity(Path(__file__)))


def main():
    """Publish genuine useful chemical custody with both ordinary reload/replay proofs."""
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
