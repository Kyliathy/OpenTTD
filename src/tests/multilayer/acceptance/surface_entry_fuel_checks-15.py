"""Verify ordinary reload and identical continuation at genuine full fuel affordability.

@file surface_entry_fuel_checks-15.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15BIB.3.
The original selected branch supplies every earned penny and physical input.
An ordinary resave branch proves persistence without contributing extra capital.
"""
import argparse
import json
from pathlib import Path
import runpy

from space_preview_checks import ROOT, chunks, identity
from surface_income_ledger import TABLE, observe, reserve_identity

RELOAD = runpy.run_path(str(Path(__file__).with_name("surface_entry_fuel_checks-12.py")))


def execute():
    """@return Complete both-build original financial reload, exact next work and current affordable outputs."""
    foundation_path = ROOT / "logs/surface-entry-income-capital-verification.json"
    foundation = json.loads(foundation_path.read_text(encoding="utf-8"))
    assert foundation["complete"] and foundation["full_propellant_affordability"]
    physical, summaries, processes, build_rows = [], [], set(), []
    for configuration in ("RelWithDebInfo", "Debug"):
        entry, = [row for row in foundation["configurations"] if row["configuration"] == configuration]
        original_receipt = ROOT / f"logs/surface-entry-income-earned-{configuration}-{entry['through_period']}.json"
        original = json.loads(original_receipt.read_text(encoding="utf-8"))["operation"]
        assert original["output"] == entry["output"]
        source = ROOT / original["output"]["path"]; assert identity(source) == original["output"]
        restored, restore = RELOAD["ORDINARY"](configuration, source, "income-capital-" + configuration.lower())
        RELOAD["REVISION"](source, restored)
        pause = RELOAD["restore_pause"](source, restored)
        old, loaded = [chunks(path)[1] for path in (source, restored)]
        assert old.keys() == loaded.keys() and all(old[key] == loaded[key] for key in old if key not in (b"GLOG", b"DATE"))
        first, last = observe(source), observe(restored)
        assert first == last and reserve_identity(first) == reserve_identity(last)
        restored_receipt = ROOT / f"logs/surface-entry-income-capital-restored-{configuration}.json"
        RELOAD["publish"](restored_receipt, dict(_documentation=__doc__, complete=True, configuration=configuration, phase="restore",
                                               operation=dict(**restore, initial=first, final=last)))
        continuations, references, rows = [], [], []
        # Independently continue the actual current game and its ordinary
        # resave. Native affordability may stop at the next74tick checkpoint;
        # both must choose the same real work and preserve the whole fuel bill.
        for receipt, suffix in ((original_receipt, "continued"), (restored_receipt, "replay")):
            operation = RELOAD["EARN"](configuration, receipt, "earn", reserve=150000)
            destination = ROOT / f"logs/surface-entry-income-capital-{suffix}-{configuration}.json"
            RELOAD["publish"](destination, dict(_documentation=__doc__, complete=True, configuration=configuration, phase="earn", operation=operation))
            earned, = operation["rows"]["earned"]
            company = operation["final"]["company"]
            assert 0 < earned["earned"] <= 27000 and earned["affordable"]
            assert company["cash"] + 5000000 - company["debt"] >= earned["requirement"]
            before, output = [ROOT / operation[name]["path"] for name in ("before", "output")]
            if TABLE(ROOT / operation["source"]["path"], b"GLOG") != TABLE(before, b"GLOG"):
                RELOAD["LOAD_REVISION"](ROOT / operation["source"]["path"], before)
            assert TABLE(before, b"GLOG") == TABLE(output, b"GLOG")
            continuations.append(chunks(output)[1]); rows.append(operation["rows"]); references.append(identity(destination))
            for reference in operation["native_processes"]:
                assert identity(ROOT / reference["path"]) == reference
                process = json.loads((ROOT / reference["path"]).read_text(encoding="utf-8")); assert process["exit_status"] == 0
                processes.add(reference["path"])
        assert rows[0] == rows[1] and continuations[0].keys() == continuations[1].keys()
        assert all(continuations[0][key] == continuations[1][key] for key in continuations[0] if key != b"GLOG")
        for reference in restore["native_processes"]:
            assert identity(ROOT / reference["path"]) == reference
            process = json.loads((ROOT / reference["path"]).read_text(encoding="utf-8")); assert process["exit_status"] == 0
            processes.add(reference["path"])
        normalized = dict(loaded); normalized[b"DATE"] = old[b"DATE"]
        physical.append([normalized] + continuations); build_rows.append(rows)
        selected = json.loads((ROOT / references[0]["path"]).read_text(encoding="utf-8"))["operation"]
        summaries.append(dict(configuration=configuration, restore=identity(restored_receipt), continuations=references,
                              output=selected["output"], company=selected["final"]["company"], fresh_requirement=rows[0]["earned"][0]["requirement"],
                              original_restore_pause=pause))
        print(configuration, "full fuel affordability survives ordinary reload and exact native continuation", flush=True)
    assert build_rows[0] == build_rows[1]
    comparisons = 0
    for first, second in zip(*physical, strict=True):
        assert first.keys() == second.keys() and all(first[key] == second[key] for key in first if key != b"GLOG")
        comparisons += 120
    assert comparisons == 360 and len(processes) == 6
    return dict(_documentation=__doc__, complete=True, foundation=identity(foundation_path), configurations=summaries,
                native_processes=6, exact_cross_build_gameplay_comparisons=360, exact_native_replay_gameplay_comparisons=240,
                full_propellant_affordability=True, actual_propellant_or_launch_complete=False,
                sources=[identity(Path(__file__)), identity(Path(__file__).with_name("surface_entry_fuel_checks-12.py")),
                         identity(Path(__file__).with_name("surface_entry_fuel_checks-5.py"))])


def main():
    """Close actual finance persistence while leaving physical Propellant and launch work to their own producers."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, required=True)
    output = (ROOT / parser.parse_args().output).resolve()
    RELOAD["publish"](output, execute())
    print(output.relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
