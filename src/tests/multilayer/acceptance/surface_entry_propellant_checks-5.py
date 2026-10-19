"""Bind genuine PCHM bank/factory funding and actual native machinery construction.

@file surface_entry_propellant_checks-5.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15BID.2.
An empty ready factory is a real producer boundary, never useful fuel. Original
parked chemicals and construction products remain physical sole owners here.
"""
import argparse
import json
from pathlib import Path
import runpy

from space_preview_checks import ROOT, chunks, identity

REVISION = runpy.run_path(str(Path(__file__).with_name("surface_entry_checks-7.py")))["known_revision"]


def bind(stage):
    """@param stage Actual funding or completed real construction. @return Exact continuous both-build paid machinery proof."""
    foundation_path = ROOT / "logs/surface-entry-propellant-plan-verification.json"
    foundation = json.loads(foundation_path.read_text(encoding="utf-8")); assert foundation["complete"]
    checked_ids, processes, physical, build_rows, summaries = set(), set(), [], [], []

    def checked(reference):
        """@param reference Exact immediate artifact. @return Verified original repository-local path."""
        path = ROOT / reference["path"]; key = reference["path"], reference["sha256"]
        if key not in checked_ids:
            assert identity(path) == reference, path
            checked_ids.add(key)
        return path

    def walk(value):
        """@param value Immediate receipt/process tree. Validate exact native provenance without replaying earlier finance."""
        if isinstance(value, dict):
            if set(value) == {"path", "sha256"}:
                checked(value)
            else:
                for child in value.values():
                    walk(child)
        elif isinstance(value, list):
            for child in value:
                walk(child)

    for configuration in ("RelWithDebInfo", "Debug"):
        previous, = [entry["output"] for entry in foundation["configurations"] if entry["configuration"] == configuration]
        paths = [ROOT / f"logs/surface-entry-propellant-funded-{configuration}.json"]
        if stage == "construction":
            # Select each actual first construction output in sequence. A
            # partial3000tick burst cannot be skipped or replaced with a later
            # independently prepared factory that hides its costs/progress.
            for burst in range(1, 4):
                path = ROOT / f"logs/surface-entry-propellant-built-{configuration}-{burst}.json"
                paths.append(path)
                if json.loads(path.read_text(encoding="utf-8"))["operation"]["machinery"]["complete"]:
                    break
        snapshots, rows, ticks = [], [], 0
        for index, path in enumerate(paths):
            value = json.loads(path.read_text(encoding="utf-8")); walk(value)
            assert value["complete"] and value["configuration"] == configuration
            operation = value["operation"]; assert operation["source"] == previous
            source, before, output = [checked(operation[name]) for name in ("source", "before", "output")]
            old, first, last = [chunks(item)[1] for item in (source, before, output)]
            assert old.keys() == first.keys() == last.keys()
            assert all(old[key] == first[key] for key in old if key != b"GLOG")
            if old[b"GLOG"] != first[b"GLOG"]:
                REVISION(source, before)
            assert first[b"GLOG"] == last[b"GLOG"]
            assert operation["initial"]["reserves"] == operation["final"]["reserves"] == dict(steel=36, goods=14, chemicals=21)
            machinery = operation["machinery"]
            assert machinery["industry"] != 15 and len(machinery["cells"]) == 16
            assert machinery["consumed"] == machinery["produced"] == machinery["dispatched"] == 0
            if index == 0:
                assert value["phase"] == "fund" and not machinery["complete"]
                assert operation["final"]["company"] == dict(cash=110751, debt=5000000)
                assert operation["rows"]["funded"][0]["credit"] == 640000
            else:
                assert value["phase"] == "build"
                earned, = operation["rows"]["earned"]; ticks += earned["earned"]
                assert 0 < earned["earned"] <= 3000 and earned["debt"] == 5000000
                assert operation["machinery_before"]["industry"] == machinery["industry"]
            for reference in operation["native_processes"]:
                process = json.loads(checked(reference).read_text(encoding="utf-8")); walk(process)
                assert process["exit_status"] == 0; processes.add(reference["path"])
            previous = operation["output"]; snapshots.extend((first, last)); rows.append(operation["rows"])
        if stage == "construction":
            assert machinery["complete"] and ticks > 0
        physical.append(snapshots); build_rows.append(rows)
        summaries.append(dict(configuration=configuration, receipts=[identity(path) for path in paths], output=previous,
                              company=operation["final"]["company"], machinery=machinery, actual_earned_ticks=ticks))
    assert build_rows[0] == build_rows[1]
    comparisons = 0
    for first, second in zip(*physical, strict=True):
        assert first.keys() == second.keys() and all(first[key] == second[key] for key in first if key != b"GLOG")
        comparisons += 120
    return dict(_documentation=__doc__, complete=True, stage=stage, foundation=identity(foundation_path), configurations=summaries,
                native_processes=len(processes), exact_cross_build_gameplay_comparisons=comparisons,
                actual_empty_factory_ready=stage == "construction", useful_propellant_complete=False, source=identity(Path(__file__)))


def main():
    """Retain paid incomplete or earned ready machinery while keeping useful Propellant acceptance open."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--stage", choices=("funding", "construction"), required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    output = (ROOT / args.output).resolve()
    assert output.is_relative_to(ROOT / "logs") and not output.exists()
    output.write_text(json.dumps(bind(args.stage), indent=2) + "\n", encoding="utf-8")
    print(output.relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
