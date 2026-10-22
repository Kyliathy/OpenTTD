"""Shared ordinary/exact continuation of genuine eleven-service regional worlds.

@file surface_regional_replay.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Groups15BIEQC and subsequent real capital boundaries.
Only original commands and real native ticks change the selected world. Copies
prove identical behavior; they never contribute capital or material to it.
"""
import json
from pathlib import Path
import runpy

from space_preview_checks import ROOT, chunks, identity
from surface_regional_ledger import observe, lives

DIRECTORY = Path(__file__).resolve().parent
RELOAD = runpy.run_path(str(DIRECTORY / "surface_entry_fuel_checks-12.py"))
DRIVER = runpy.run_path(str(DIRECTORY / "surface_entry_regional_checks-1.py"))["execute"]
ROUTES = runpy.run_path(str(DIRECTORY / "surface_entry_regional_renewal_checks-1.py"))["all_orders"]
POLICY = runpy.run_path(str(DIRECTORY / "surface_entry_regional_checks-3.py"))["route_policy"]


def replay(configuration, predecessor, stem):
    """@param configuration Actual ordinary/focused pair. @param predecessor Exact current selected native receipt.
    @param stem Unique repository-local artifact basename for this real boundary.
    @return Exact ordinary restoration and3000tick native original/copy continuation, retaining only original earnings.
    """
    assert stem.replace("-", "").isalnum() and stem.isascii() and stem == stem.lower()
    value = json.loads(predecessor.read_text(encoding="utf-8")); assert value["complete"] and value["configuration"] == configuration
    original = value["operation"]["output"]; source = ROOT / original["path"]; assert identity(source) == original
    restored, operation = RELOAD["ORDINARY"](configuration, source, stem + "-" + configuration.lower())
    RELOAD["REVISION"](source, restored); pause = RELOAD["restore_pause"](source, restored)
    first, last = [chunks(path)[1] for path in (source, restored)]
    assert first.keys() == last.keys() and all(first[key] == last[key] for key in first if key not in (b"GLOG", b"DATE"))
    assert observe(source) == observe(restored) and ROUTES(source) == ROUTES(restored)
    restore_path = ROOT / f"logs/{stem}-restored-{configuration}.json"
    RELOAD["publish"](restore_path, dict(_documentation=__doc__, complete=True, configuration=configuration, phase="restore", operation=operation, original_pause=pause))
    worlds, rows, receipts = [], [], []
    # Follow each actual branch independently. Original and ordinary must
    # agree on all gameplay bytes after exactly the same real native interval.
    for receipt, suffix in ((predecessor, "original"), (restore_path, "ordinary")):
        current = DRIVER(configuration, "short", receipt); output = ROOT / current["output"]["path"]
        assert POLICY(ROUTES(source)) == POLICY(ROUTES(output))
        target = ROOT / f"logs/{stem}-continued-{configuration}-{suffix}.json"
        RELOAD["publish"](target, dict(_documentation=__doc__, complete=True, configuration=configuration, phase="short", operation=current))
        receipts.append(identity(target)); worlds.append(chunks(output)[1]); rows.append(current["rows"])
        print(configuration, suffix, current["rows"]["earned"], flush=True)
    assert rows[0] == rows[1] and worlds[0].keys() == worlds[1].keys()
    assert all(worlds[0][key] == worlds[1][key] for key in worlds[0] if key != b"GLOG")
    selected = json.loads((ROOT / receipts[0]["path"]).read_text(encoding="utf-8"))["operation"]
    return dict(_documentation=__doc__, complete=True, configuration=configuration, source_receipt=identity(predecessor),
                restore=identity(restore_path), continuations=receipts, output=selected["output"], company=selected["final"]["company"],
                service_lives=lives(ROOT / selected["output"]["path"]), additional_continuation_ticks=3000,
                rows=rows[0], native_processes=3, exact_native_replay_gameplay_comparisons=120, source=identity(Path(__file__)))
