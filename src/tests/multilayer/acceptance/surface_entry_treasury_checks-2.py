"""Preserve an explicit current regional treasury through ordinary reload and exact continuation.

@file surface_entry_treasury_checks-2.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15BIEQB.
Actual native books, all eleven useful services and complete launch stock remain
authoritative. Product retention reuses identical already retained native binaries.
"""
import argparse
import json
from pathlib import Path
import runpy
import shutil

from space_preview_checks import ROOT, chunks, identity
from surface_regional_ledger import observe, lives

DIRECTORY = Path(__file__).resolve().parent
RELOAD = runpy.run_path(str(DIRECTORY / "surface_entry_fuel_checks-12.py"))
REGIONAL = runpy.run_path(str(DIRECTORY / "surface_entry_regional_checks-3.py"))
DRIVER = runpy.run_path(str(DIRECTORY / "surface_entry_regional_checks-1.py"))["execute"]


def reload(configuration, foundation_path):
    """@param configuration Actual ordinary/focused native pair. @param foundation_path Complete current cross-build capital receipt.
    @return Exact current ordinary restoration and genuine3000tick original/copy continuation.
    """
    foundation = json.loads(foundation_path.read_text(encoding="utf-8")); assert foundation["complete"]
    entry, = [row for row in foundation["configurations"] if row["configuration"] == configuration]
    predecessor = ROOT / entry["receipt"]["path"]; assert identity(predecessor) == entry["receipt"]
    source = ROOT / entry["output"]["path"]; assert identity(source) == entry["output"]
    assert observe(source)["company"] == entry["company"] and len(lives(source)) == 11
    period = foundation["last_period"]; stem = f"surface-entry-capital-boundary-{period}"
    restored, operation = RELOAD["ORDINARY"](configuration, source, stem + "-" + configuration.lower())
    RELOAD["REVISION"](source, restored); pause = RELOAD["restore_pause"](source, restored)
    first, last = [chunks(path)[1] for path in (source, restored)]
    assert first.keys() == last.keys() and all(first[key] == last[key] for key in first if key not in (b"GLOG", b"DATE"))
    assert observe(source) == observe(restored) and REGIONAL["service_orders"](source) == REGIONAL["service_orders"](restored)
    restore_path = ROOT / f"logs/{stem}-restored-{configuration}.json"
    RELOAD["publish"](restore_path, dict(_documentation=__doc__, complete=True, configuration=configuration, phase="restore", operation=operation, original_pause=pause))
    branches, rows, receipts = [], [], []
    # A copied ordinary game contributes no capital. Its native continuation
    # must reproduce the selected original's every gameplay family and account.
    for receipt, suffix in ((predecessor, "original"), (restore_path, "ordinary")):
        current = DRIVER(configuration, "short", receipt)
        output = ROOT / current["output"]["path"]
        assert REGIONAL["route_policy"](REGIONAL["service_orders"](source)) == REGIONAL["route_policy"](REGIONAL["service_orders"](output))
        target = ROOT / f"logs/{stem}-continued-{configuration}-{suffix}.json"
        RELOAD["publish"](target, dict(_documentation=__doc__, complete=True, configuration=configuration, phase="short", operation=current))
        receipts.append(identity(target)); branches.append(chunks(output)[1]); rows.append(current["rows"])
        print(configuration, suffix, current["rows"]["earned"], flush=True)
    assert rows[0] == rows[1] and branches[0].keys() == branches[1].keys()
    assert all(branches[0][key] == branches[1][key] for key in branches[0] if key != b"GLOG")
    selected = json.loads((ROOT / receipts[0]["path"]).read_text(encoding="utf-8"))["operation"]
    return dict(_documentation=__doc__, complete=True, configuration=configuration, foundation=identity(foundation_path), global_period=period,
                additional_continuation_ticks=3000, restore=identity(restore_path), continuations=receipts, output=selected["output"],
                company=selected["final"]["company"], service_lives=lives(ROOT / selected["output"]["path"]), rows=rows[0],
                native_processes=3, exact_native_replay_gameplay_comparisons=120)


def final(foundation_path):
    """@param foundation_path Accepted current cross-build capital chain.
    @return Both-build ordinary/replay boundary, all current owners and exact retained current financial tooling.
    """
    foundation = json.loads(foundation_path.read_text(encoding="utf-8")); assert foundation["complete"]
    period = foundation["last_period"]; stem = f"surface-entry-capital-boundary-{period}"
    summaries, worlds, rows, processes = [], [], [], set()
    for configuration in ("RelWithDebInfo", "Debug"):
        path = ROOT / f"logs/{stem}-reload-{configuration}.json"
        value = json.loads(path.read_text(encoding="utf-8")); assert value["complete"] and value["foundation"] == identity(foundation_path)
        restore = json.loads((ROOT / value["restore"]["path"]).read_text(encoding="utf-8"))["operation"]
        original, restored = [chunks(ROOT / restore[field]["path"])[1] for field in ("source", "output")]
        normalized = dict(restored); normalized[b"DATE"] = original[b"DATE"]; stages = [normalized]
        processes.update(reference["path"] for reference in restore["native_processes"])
        comparable = []
        for reference in value["continuations"]:
            assert identity(ROOT / reference["path"]) == reference
            current = json.loads((ROOT / reference["path"]).read_text(encoding="utf-8"))["operation"]
            for field in ("source", "before", "output", "application", "terminal", "retained_harness"):
                assert identity(ROOT / current[field]["path"]) == current[field]
            for item in current["sources"] + current["native_processes"]:
                assert identity(ROOT / item["path"]) == item
            stages.append(chunks(ROOT / current["output"]["path"])[1]); comparable.append(current["rows"])
            processes.update(item["path"] for item in current["native_processes"])
        assert comparable[0] == comparable[1]
        company = value["company"]; service = value["service_lives"]
        summaries.append(dict(configuration=configuration, receipt=value["continuations"][0], output=value["output"], company=company,
                              global_period=period, additional_continuation_ticks=3000, spendable=company["cash"] + 5000000 - company["debt"],
                              service_lives=service, renewal_due=min(row["remaining"] for row in service) <= 732, reload_receipt=identity(path)))
        worlds.append(stages); rows.append(comparable)
    assert rows[0] == rows[1] and len(processes) == 6
    for first, second in zip(*worlds, strict=True):
        assert first.keys() == second.keys() and len(first) == 121 and all(first[key] == second[key] for key in first if key != b"GLOG")
    old_manifest = ROOT / "test-cache/surface-entry/regional-service-products/retained.json"
    old = json.loads(old_manifest.read_text(encoding="utf-8")); copies = list(old["products"])
    # Reuse byte-identical already retained native products, then preserve all
    # four current financial tools before the next vehicle producer changes them.
    for product in copies:
        assert identity(ROOT / product["source"]["path"]) == product["source"]
        assert identity(ROOT / product["retained"]["path"]) == product["retained"]
    retained = ROOT / f"test-cache/surface-entry/capital-boundary-{period}-products"; assert not retained.exists(); retained.mkdir(parents=True)
    for index, name in enumerate(("surface_entry_capital_checks-1.py", "surface_entry_capital_checks-2.py", "surface_entry_treasury_checks-1.py", "surface_entry_treasury_checks-2.py")):
        source = DIRECTORY / name; target = retained / f"{index:02d}-{name}"; shutil.copyfile(source, target)
        assert identity(source)["sha256"] == identity(target)["sha256"]; copies.append(dict(source=identity(source), retained=identity(target)))
    manifest = retained / "retained.json"
    manifest.write_text(json.dumps(dict(_documentation=__doc__, products=copies, native_product_foundation=identity(old_manifest)), indent=2) + "\n", encoding="utf-8")
    return dict(_documentation=__doc__, complete=True, foundation=identity(foundation_path), configurations=summaries,
                actual_capital_complete=all(entry["spendable"] >= 1870000 for entry in summaries), actual_launch_complete=False,
                native_processes=6, products=identity(manifest), cross_build_gameplay_comparisons=360, exact_replay_gameplay_comparisons=240,
                source=identity(Path(__file__)))


def main():
    """Publish one current ordinary boundary or the exact retained financial/service handoff."""
    parser = argparse.ArgumentParser(description=__doc__); parser.add_argument("--kind", choices=("reload", "final"), required=True)
    parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug")); parser.add_argument("--foundation", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True); args = parser.parse_args()
    output = (ROOT / args.output).resolve(); assert output.is_relative_to(ROOT / "logs") and not output.exists()
    if args.kind == "reload":
        assert args.configuration is not None; value = reload(args.configuration, (ROOT / args.foundation).resolve())
    else:
        value = final((ROOT / args.foundation).resolve())
    output.write_text(json.dumps(value, indent=2) + "\n", encoding="utf-8"); print(output.relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
