"""Bind paid pickup, genuine mine foundations and ordinary native continuation.

@file surface_entry_mine_checks-8.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15BF.3.
This closes the earned-floor/access prerequisite only. A whole admitted site
does not replace actual mine funding, construction, extraction or fuel work.
"""
import argparse
import json
from pathlib import Path
import runpy

from space_preview_checks import ROOT, chunks, identity

GALLERY = runpy.run_path(str(Path(__file__).with_name("surface_entry_mine_checks-4.py")))
REVISION = runpy.run_path(str(Path(__file__).with_name("surface_entry_checks-7.py")))


def bind(specification):
    """Verify actual paid facilities, original native owners and both-build ordinary replay.

    @param specification Exact accepted station-gate/facility/restore/replay receipts and retained products.
    @return Completed mine-floor/service prerequisite, with genuine mineral production still open.
    """
    spec = json.loads(specification.read_text())
    foundation = json.loads((ROOT / spec["gallery"]).read_text())
    assert foundation["complete"] and foundation["additional_floor_cells"] == 12
    aliases = {row["sha256"]: ROOT / row["path"] for row in spec["retained"]}
    processes, outputs, summaries, comparisons, replays = set(), [], [], 0, 0

    def checked(reference):
        """@param reference Original artifact identity. @return Its verified original or explicit retained path."""
        path = aliases.get(reference["sha256"], ROOT / reference["path"])
        assert identity(path)["sha256"] == reference["sha256"], path
        return path

    def walk(value):
        """@param value Immediate nested original evidence. Verify each identity without historical graph replay."""
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
        """@param path Original accepted JSON. @param configuration Its native build. @return Verified physical operation and child process."""
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

    # Every paid access operation consumes the exact completed physical work
    # checkpoint. Original and ordinary-resaved branches then earn the same
    # real simulation time while preserving the original jobs and finite stock.
    for entry in spec["configurations"]:
        configuration = entry["configuration"]
        source, = [row["output"] for row in foundation["configurations"] if row["configuration"] == configuration]
        gate, _ = operation(entry["station_gate"], configuration)
        assert (gate["cases"], gate["assertions"]) == (3, 3103) and gate["genuine_world_predecessor"] is False
        facilities, _ = operation(entry["facilities"], configuration)
        assert facilities["source"] == source
        assert facilities["mine"] == 6236 and facilities["station"] == 5 and facilities["capital"] == 616584
        assert sum(row["cost"] for row in facilities["invoices"]) == 755
        assert facilities["final"]["cash"] == 139332 and facilities["final"]["debt"] == 3500000
        assert facilities["final"]["parked_steel"] == 36 and facilities["final"]["parked_goods"] == 14
        final = facilities["output"]
        physical = [chunks(checked(final))[1]]
        restored, ordinary = operation(entry["restored"], configuration)
        assert ordinary["source"] == final and restored["earned_ticks"] == 0
        physical.append(chunks(checked(ordinary["output"]))[1])
        branch_outputs = []
        for path, predecessor in zip(entry["replays"], (final, ordinary["output"]), strict=True):
            op, _ = operation(path, configuration)
            assert op["source"] == predecessor and op["earned_ticks"] == 3000
            assert not op["work"]["owners"]["projects"] and op["work"]["salt_remaining"] == 24000 and len(op["work"]["floor"]) == 27
            before = checked(op["output"]).with_name("surface-entry-before-replay-440.sav")
            table = GALLERY["OWNERS"]["saved_table"]
            if table(checked(predecessor), b"GLOG") != table(before, b"GLOG"):
                REVISION["known_revision"](checked(predecessor), before)
            assert table(before, b"GLOG") == table(checked(op["output"]), b"GLOG")
            branch_outputs.append(chunks(checked(op["output"]))[1])
            physical.append(branch_outputs[-1])
        assert branch_outputs[0].keys() == branch_outputs[1].keys()
        assert all(branch_outputs[0][key] == branch_outputs[1][key] for key in branch_outputs[0] if key != b"GLOG")
        replays += len(branch_outputs[0]) - 1
        outputs.append(physical)
        summaries.append(dict(configuration=configuration, output=final, facilities=entry["facilities"], restored=entry["restored"], replays=entry["replays"]))
    for a, b in zip(*outputs, strict=True):
        assert a.keys() == b.keys() and all(a[key] == b[key] for key in a if key != b"GLOG")
        comparisons += len(a) - 1
    assert len(processes) == 10 and comparisons == 480 and replays == 240
    return dict(_documentation=__doc__, complete=True, specification=identity(specification), gallery=identity(ROOT / spec["gallery"]),
                configurations=summaries, native_processes=10, exact_cross_build_gameplay_comparisons=comparisons,
                exact_native_replay_gameplay_comparisons=replays, pickup_station=5, mine_anchor=6236, actual_mine_quote=616584,
                access_invoice=755, cash=139332, debt=3500000, salt_remaining=24000, floor_cells=27,
                supplied_stock_or_floor=False, actual_mine_funded=False, genuine_fuel_complete=False, source=identity(Path(__file__)))


def main():
    """Publish actual mine-floor/service acceptance from exact already completed native receipts."""
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
