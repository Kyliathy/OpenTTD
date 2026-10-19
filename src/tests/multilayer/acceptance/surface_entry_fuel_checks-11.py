"""Bind the real freight renewal plan and paid empty-depot replacement.

@file surface_entry_fuel_checks-11.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15BIC.
Original commands own all purchases, rail conversion, wagon coupling and credit.
The immediate maintenance proof supplies the continuous genuine world lineage.
"""
import argparse
import json
from pathlib import Path
import runpy

from space_preview_checks import ROOT, chunks, identity

REVISION = runpy.run_path(str(Path(__file__).with_name("surface_entry_checks-7.py")))["known_revision"]


def bind(stage):
    """@param stage Pure arrangement or actual paid renewal. @return Exact both-build native producer proof."""
    foundation_path = ROOT / "logs/surface-entry-income-maintenance-verification.json"
    foundation = json.loads(foundation_path.read_text(encoding="utf-8"))
    assert foundation["complete"] and foundation["fleet_replacement_required"]
    phases = ("plan",) if stage == "plan" else ("plan", "recall", "settle", "buy")
    checked_ids, processes, physical, rows_by_build, summaries = set(), set(), [], [], []
    retained_path = ROOT / "test-cache/surface-entry/income-fleet-native-products/retained.json"
    retained = json.loads(retained_path.read_text(encoding="utf-8"))
    aliases = {row["sha256"]: ROOT / row["path"] for row in retained["retained"]}
    # Each invocation retained its exact executed Python harness before work.
    # Later receipt-only fixes may change the live file without rewriting that
    # original producer or replaying already completed paid commands.
    for configuration in ("RelWithDebInfo", "Debug"):
        for phase in phases:
            receipt = ROOT / f"logs/surface-entry-income-fleet-{phase}-{configuration}.json"
            reference = json.loads(receipt.read_text(encoding="utf-8"))["operation"]["retained_harness"]
            assert identity(ROOT / reference["path"]) == reference
            aliases[reference["sha256"]] = ROOT / reference["path"]

    def checked(reference):
        """@param reference Immediate immutable artifact. @return Verified repository-local original path."""
        key = reference["path"], reference["sha256"]
        path = ROOT / reference["path"]
        if key not in checked_ids:
            if identity(path) != reference:
                assert reference["sha256"] in aliases, path
                assert identity(aliases[reference["sha256"]])["sha256"] == reference["sha256"]
            checked_ids.add(key)
        return path

    def walk(value):
        """@param value Immediate receipt/process tree. Verify every original artifact once without replaying older producers."""
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
        previous, = [row["output"] for row in foundation["configurations"] if row["configuration"] == configuration]
        snapshots, rows, receipts = [], [], []
        for phase in phases:
            path = ROOT / f"logs/surface-entry-income-fleet-{phase}-{configuration}.json"
            value = json.loads(path.read_text(encoding="utf-8")); walk(value)
            operation = value["operation"]
            assert value["complete"] and value["configuration"] == configuration
            assert value["phase"] == "fleet-" + ("query" if phase == "plan" else phase)
            assert operation["source"] == previous
            source, before, output = [checked(operation[name]) for name in ("source", "before", "output")]
            first, loaded, last = [chunks(item)[1] for item in (source, before, output)]
            assert first.keys() == loaded.keys() == last.keys()
            assert all(first[key] == loaded[key] for key in first if key != b"GLOG")
            if first[b"GLOG"] != loaded[b"GLOG"]:
                REVISION(source, before)
            assert loaded[b"GLOG"] == last[b"GLOG"]
            if phase == "plan":
                assert loaded == last and len(loaded) == 121
                plan, = operation["rows"]["fleet_plan"]
                assert plan["head"] == 26 and plan["heads"] == 3 and plan["coal_extension"]
                assert plan["capital"] == 168503 and plan["credit"] == 230000 and plan["buffer"] == 150000
                assert all(item["admitted"] for item in operation["rows"]["fleet_platforms"])
            elif phase == "settle":
                assert all(item["stopped"] and not any(item["held"].values()) for item in operation["final"]["trains"])
            elif phase == "buy":
                bought, = operation["rows"]["fleet_bought"]
                assert bought["cash"] >= 150000 and bought["debt"] <= 5000000 and not bought["supplied_stock"]
                assert [len(item["chain"]) for item in operation["final"]["trains"]] == [14, 14, 10]
                assert len(operation["rows"]["fleet_retained"]) == 28
            for reference in operation["native_processes"]:
                process = json.loads(checked(reference).read_text(encoding="utf-8")); walk(process)
                assert process["exit_status"] == 0; processes.add(reference["path"])
            previous = operation["output"]
            snapshots.extend((loaded, last)); rows.append(operation["rows"]); receipts.append(identity(path))
        physical.append(snapshots); rows_by_build.append(rows)
        summaries.append(dict(configuration=configuration, receipts=receipts, output=previous))
    assert rows_by_build[0] == rows_by_build[1]
    comparisons = 0
    for first, second in zip(*physical, strict=True):
        assert first.keys() == second.keys() and all(first[key] == second[key] for key in first if key != b"GLOG")
        comparisons += 120
    assert len(processes) == 2 * len(phases) and comparisons == 240 * len(phases)
    return dict(_documentation=__doc__, complete=True, stage=stage, foundation=identity(foundation_path), configurations=summaries,
                native_processes=len(processes), exact_cross_build_gameplay_comparisons=comparisons,
                actual_renewal_complete=stage == "purchase", full_propellant_affordability=False,
                retained=identity(retained_path), source=identity(Path(__file__)))


def main():
    """Retain the bounded renewal producer without claiming final fuel funding or earned future income."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--stage", choices=("plan", "purchase"), required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    output = (ROOT / args.output).resolve()
    assert output.is_relative_to(ROOT / "logs") and not output.exists()
    output.write_text(json.dumps(bind(args.stage), indent=2) + "\n", encoding="utf-8")
    print(output.relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
