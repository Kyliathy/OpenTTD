"""Bind genuine savings, real service limits and ordinary exact continuation.

@file surface_entry_continuation_checks-2.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15BIEO.3.
All selected capital comes from original income and matching native repayments;
ordinary replay branches supply no useful funds or materials to that lineage.
"""
import argparse
import json
from pathlib import Path
import runpy
import shutil

from space_preview_checks import ROOT, chunks, identity
from surface_fuel_refill_ledger import observe

DIRECTORY = Path(__file__).resolve().parent
RELOAD = runpy.run_path(str(DIRECTORY / "surface_entry_fuel_checks-12.py"))
REPLAY = runpy.run_path(str(DIRECTORY / "surface_entry_road_renewal_checks-1.py"))["execute"]
LIVES = runpy.run_path(str(DIRECTORY / "surface_entry_capital_checks-1.py"))["lives"]


def compare(worlds):
    """@param worlds Matching independently built stage lists. Assert every gameplay family agrees."""
    assert len(worlds) == 2 and len(worlds[0]) == len(worlds[1])
    for first, second in zip(*worlds, strict=True):
        assert first.keys() == second.keys() and len(first) == 121 and all(first[key] == second[key] for key in first if key != b"GLOG")


def reload(configuration):
    """@param configuration Actual ordinary/focused application pair.
    @return Original savings restored by the ordinary game and continued by identical3000tick native branches.
    """
    predecessor = ROOT / f"logs/surface-entry-capital-earned-{configuration}-16.json"
    value = json.loads(predecessor.read_text(encoding="utf-8")); source = ROOT / value["operation"]["output"]["path"]
    assert identity(source) == value["operation"]["output"]
    restored, operation = RELOAD["ORDINARY"](configuration, source, "surface-capital-restore-" + configuration.lower())
    RELOAD["REVISION"](source, restored); pause = RELOAD["restore_pause"](source, restored)
    first, last = [chunks(path)[1] for path in (source, restored)]
    assert first.keys() == last.keys() and all(first[key] == last[key] for key in first if key not in (b"GLOG", b"DATE"))
    assert observe(source) == observe(restored)
    restore_path = ROOT / f"logs/surface-entry-continuation-restored-{configuration}.json"
    RELOAD["publish"](restore_path, dict(_documentation=__doc__, complete=True, configuration=configuration, phase="restore", operation=operation, original_pause=pause))
    branches, rows, receipts = [], [], []
    # Reuse the accepted current whole-fuel3000tick road boundary; the ordinary
    # branch verifies replay and never increases the selected original funds.
    for path, suffix in ((predecessor, "original"), (restore_path, "ordinary")):
        continuation = REPLAY(configuration, "earn", path)
        receipt = ROOT / f"logs/surface-entry-continuation-continued-{configuration}-{suffix}.json"
        RELOAD["publish"](receipt, dict(_documentation=__doc__, complete=True, configuration=configuration, phase="earn", operation=continuation))
        receipts.append(identity(receipt)); branches.append(chunks(ROOT / continuation["output"]["path"])[1]); rows.append(continuation["rows"])
    assert rows[0] == rows[1] and branches[0].keys() == branches[1].keys()
    assert all(branches[0][key] == branches[1][key] for key in branches[0] if key != b"GLOG")
    selected = json.loads((ROOT / receipts[0]["path"]).read_text(encoding="utf-8"))["operation"]
    return dict(_documentation=__doc__, complete=True, configuration=configuration, restore=identity(restore_path), continuations=receipts,
                output=selected["output"], company=selected["final"]["company"], service_lives=LIVES(ROOT / selected["output"]["path"]), rows=rows[0],
                native_processes=3, exact_native_replay_gameplay_comparisons=120)


def final():
    """@return Both ordinary restorations and exact native continuations, with immutable tested-product retention."""
    foundation_path = ROOT / "logs/surface-entry-continuation-boundary-16-verification.json"
    foundation = json.loads(foundation_path.read_text(encoding="utf-8")); assert foundation["complete"] and foundation["renewal_due"]
    worlds, rows, summaries, processes = [], [], [], set()
    for entry in foundation["configurations"]:
        configuration = entry["configuration"]; path = ROOT / f"logs/surface-entry-continuation-reload-{configuration}.json"
        value = json.loads(path.read_text(encoding="utf-8")); assert value["complete"] and value["configuration"] == configuration
        restore = json.loads((ROOT / value["restore"]["path"]).read_text(encoding="utf-8"))["operation"]; assert restore["source"] == entry["output"]
        original, resaved = [chunks(ROOT / restore[field]["path"])[1] for field in ("source", "output")]
        normalized = dict(resaved); normalized[b"DATE"] = original[b"DATE"]
        stages, comparable = [normalized], []
        processes.update(reference["path"] for reference in restore["native_processes"])
        for reference in value["continuations"]:
            receipt = ROOT / reference["path"]; assert identity(receipt) == reference
            continuation = json.loads(receipt.read_text(encoding="utf-8"))["operation"]
            for field in ("source", "before", "output", "application", "terminal", "retained_harness"):
                assert identity(ROOT / continuation[field]["path"]) == continuation[field]
            stages.append(chunks(ROOT / continuation["output"]["path"])[1]); comparable.append(continuation["rows"])
            processes.update(reference["path"] for reference in continuation["native_processes"])
        assert comparable[0] == comparable[1]
        assert all(stages[1][key] == stages[2][key] for key in stages[1] if key != b"GLOG")
        summaries.append(dict(configuration=configuration, receipt=value["continuations"][0], output=value["output"], company=value["company"],
                              service_lives=value["service_lives"], spendable=value["company"]["cash"] + 5000000 - value["company"]["debt"], reload_receipt=identity(path)))
        worlds.append(stages); rows.append(comparable)
    compare(worlds); assert rows[0] == rows[1] and len(processes) == 6
    previous_products = ROOT / "test-cache/surface-entry/passenger-renewal-products/retained.json"
    previous = json.loads(previous_products.read_text(encoding="utf-8")); aliases = []
    for product in previous["products"]:
        assert identity(ROOT / product["retained"]["path"]) == product["retained"]
        assert identity(ROOT / product["source"]["path"]) == product["source"]
        aliases.append(product)
    retained = ROOT / "test-cache/surface-entry/service-boundary-products"; assert not retained.exists(); retained.mkdir(parents=True)
    products = [ROOT / "src/tests/surface_entry_launch.cpp", ROOT / "src/tests/surface_entry_road_renewal.cpp"] + [DIRECTORY / f"surface_entry_capital_checks-{index}.py" for index in (1, 2)] + [DIRECTORY / f"surface_entry_continuation_checks-{index}.py" for index in (1, 2)]
    copies = []
    for index, source in enumerate(products):
        target = retained / f"{index:02d}-{source.name}"; shutil.copyfile(source, target)
        assert identity(source)["sha256"] == identity(target)["sha256"]; copies.append(dict(source=identity(source), retained=identity(target)))
    manifest = retained / "retained.json"; manifest.write_text(json.dumps(dict(_documentation=__doc__, products=copies, aliases=aliases), indent=2) + "\n", encoding="utf-8")
    return dict(_documentation=__doc__, complete=True, actual_capital_complete=False, actual_launch_complete=False, renewal_due=True,
                foundation=identity(foundation_path), native_processes=6, cross_build_gameplay_comparisons=360, exact_native_replay_gameplay_comparisons=240,
                configurations=summaries, retained=identity(manifest), source=identity(Path(__file__)))


def main():
    """Publish one ordinary replay pair or the final both-build continuing-savings handoff."""
    parser = argparse.ArgumentParser(description=__doc__); parser.add_argument("--kind", choices=("reload", "final"), required=True)
    parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug")); parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args(); output = (ROOT / args.output).resolve(); assert output.is_relative_to(ROOT / "logs") and not output.exists()
    result = reload(args.configuration) if args.kind == "reload" else final()
    RELOAD["publish"](output, result); print(output.relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
