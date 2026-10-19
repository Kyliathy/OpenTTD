"""Verify ordinary original gateway restoration without owning gameplay state.

@file surface_gateway_replay.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CB and later original gateway boundaries.
Callers supply explicit native drivers and sole-owner observers. Copied worlds
prove equality; only the selected original branch supplies later acceptance.
"""
import json
from pathlib import Path
import runpy

from space_preview_checks import ROOT, chunks, identity

DIRECTORY = Path(__file__).resolve().parent
RESTORE = runpy.run_path(str(DIRECTORY / "surface_entry_fuel_checks-12.py"))


def load(path):
    """@param path Complete accepted receipt. @return Existing immutable receipt object."""
    value = json.loads(path.read_text(encoding="utf-8")); assert value["complete"]
    return value


def replay(configuration, predecessor, stem, phase, execute, observe):
    """@param configuration Actual ordinary/focused pair. @param predecessor Selected original native checkpoint.
    @param stem Caller-specific immutable receipt prefix. @param phase Explicit native useful-service continuation.
    @param execute Actual caller's native driver. @param observe Explicit sole-owner contract.
    @return Exact ordinary restoration and matching original/copy native continuations.
    """
    original = load(predecessor)["operation"]["output"]; source = ROOT / original["path"]; assert identity(source) == original
    restored, operation = RESTORE["ORDINARY"](configuration, source, stem + "-" + configuration.lower())
    RESTORE["REVISION"](source, restored); pause = RESTORE["restore_pause"](source, restored)
    first, last = [chunks(path)[1] for path in (source, restored)]
    assert first.keys() == last.keys() and all(first[key] == last[key] for key in first if key not in (b"GLOG", b"DATE"))
    assert observe(source) == observe(restored)
    restore_path = ROOT / f"logs/{stem}-restored-{configuration}.json"
    RESTORE["publish"](restore_path, dict(_documentation=__doc__, complete=True, configuration=configuration, phase="restore", operation=operation, original_pause=pause))
    worlds, rows, receipts = [], [], []
    # Each independent branch earns real work from its own loaded state. Exact
    # replay equality cannot supply a second packet, loan or progress receipt
    # to the selected original branch used by the next gameplay producer.
    for receipt, suffix in ((predecessor, "original"), (restore_path, "ordinary")):
        current = execute(configuration, phase, receipt)
        target = ROOT / f"logs/{stem}-continued-{configuration}-{suffix}.json"
        RESTORE["publish"](target, dict(_documentation=__doc__, complete=True, configuration=configuration, phase=phase, operation=current))
        receipts.append(identity(target)); worlds.append(chunks(ROOT / current["output"]["path"])[1]); rows.append(current["rows"])
        print(configuration, suffix, current["rows"]["earned"], flush=True)
    assert rows[0] == rows[1] and worlds[0].keys() == worlds[1].keys()
    assert all(worlds[0][key] == worlds[1][key] for key in worlds[0] if key != b"GLOG")
    selected = load(ROOT / receipts[0]["path"])["operation"]
    return dict(_documentation=__doc__, complete=True, configuration=configuration, source_receipt=identity(predecessor), restore=identity(restore_path),
                continuations=receipts, output=selected["output"], company=selected["final"]["company"], additional_continuation_ticks=3000,
                source=identity(Path(__file__)))


def validate(summary, entry, observe):
    """@param summary Actual ordinary/replay summary. @param entry Exact accepted predecessor summary.
    @param observe Explicit current sole-owner contract. @return Selected actual operation, three owner stages, rows, native process identities and focused applications.
    """
    assert summary["source_receipt"] == entry["receipt"] and summary["additional_continuation_ticks"] == 3000
    assert summary["source"] == identity(Path(__file__))
    assert identity(ROOT / summary["restore"]["path"]) == summary["restore"]
    restore = load(ROOT / summary["restore"]["path"])["operation"]
    original, restored = [chunks(ROOT / restore[field]["path"])[1] for field in ("source", "output")]
    normalized = dict(restored); normalized[b"DATE"] = original[b"DATE"]
    stages, rows, processes, applications, selected = [normalized], [], set(), {}, None
    for field in ("source", "output", "application", "terminal"):
        reference = restore[field]; assert identity(ROOT / reference["path"]) == reference
    for reference in restore["native_processes"]:
        assert identity(ROOT / reference["path"]) == reference; processes.add(reference["path"])
    # Every continuation must still match its native invocation, current
    # producer/observer bytes and saved physical owners; comparing only final
    # diagnostics would miss a changed executable or an unowned packet list.
    for reference in summary["continuations"]:
        assert identity(ROOT / reference["path"]) == reference
        operation = load(ROOT / reference["path"])["operation"]
        for field in ("source", "before", "output", "application", "terminal", "retained_harness"):
            item = operation[field]; assert identity(ROOT / item["path"]) == item
        for item in operation["sources"] + operation["native_processes"]:
            assert identity(ROOT / item["path"]) == item
        for item in operation["native_processes"]:
            native = json.loads((ROOT / item["path"]).read_text(encoding="utf-8"))
            assert native["exit_status"] == 0 and native["inputs"] == [operation["source"]] and native["executable"] == operation["application"]
            processes.add(item["path"])
        output = ROOT / operation["output"]["path"]; snapshot = observe(output)
        assert json.loads(json.dumps(snapshot)) == operation["final"]
        stages.append(chunks(output)[1]); rows.append(operation["rows"]); applications[operation["application"]["path"]] = operation["application"]
        if selected is None:
            selected = operation
    assert len(rows) == 2 and rows[0] == rows[1] and len(processes) == 3
    assert selected["output"] == summary["output"]
    return selected, stages, rows, processes, applications
