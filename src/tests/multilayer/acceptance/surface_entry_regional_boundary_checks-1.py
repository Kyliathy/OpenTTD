"""Bind any explicit current regional capital boundary through ordinary replay and product retention.

@file surface_entry_regional_boundary_checks-1.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15BIEQD and subsequent actual financial boundaries.
An explicit native foundation, annual index and immutable product set prevent
stale predecessor assumptions. Only selected actual native work supplies capital.
"""
import argparse
import json
from pathlib import Path
import shutil

from space_preview_checks import ROOT, chunks, identity
from surface_regional_ledger import observe, lives
from surface_regional_replay import replay


def reload(configuration, foundation_path):
    """@param configuration Actual ordinary/focused build pair. @param foundation_path Accepted current cross-build native income chain.
    @return Ordinary/exact replay of that precise selected current treasury.
    """
    foundation = json.loads(foundation_path.read_text(encoding="utf-8")); assert foundation["complete"]
    entry, = [row for row in foundation["configurations"] if row["configuration"] == configuration]
    receipt = ROOT / entry["receipt"]["path"]; assert identity(receipt) == entry["receipt"]
    period = foundation["last_period"]; assert isinstance(period, int)
    result = replay(configuration, receipt, f"surface-entry-treasury-boundary-{period}")
    result["foundation"] = identity(foundation_path); result["global_period"] = period
    return result


def final(foundation_path, products_path):
    """@param foundation_path Accepted current native financial chain. @param products_path Exact current previously retained product manifest.
    @return Both-build preserved treasury, native service ages and immutable current tested products.
    """
    foundation = json.loads(foundation_path.read_text(encoding="utf-8")); assert foundation["complete"]
    period = foundation["last_period"]; stem = f"surface-entry-treasury-boundary-{period}"
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
            operation = json.loads((ROOT / reference["path"]).read_text(encoding="utf-8"))["operation"]
            for field in ("source", "before", "output", "application", "terminal", "retained_harness"):
                assert identity(ROOT / operation[field]["path"]) == operation[field]
            for item in operation["sources"] + operation["native_processes"]:
                assert identity(ROOT / item["path"]) == item
            stages.append(chunks(ROOT / operation["output"]["path"])[1]); comparable.append(operation["rows"])
            processes.update(item["path"] for item in operation["native_processes"])
        assert comparable[0] == comparable[1]
        source = ROOT / value["output"]["path"]; assert observe(source)["company"] == value["company"]
        company = value["company"]; service = lives(source); assert len(service) == 11
        summaries.append(dict(configuration=configuration, receipt=value["continuations"][0], output=value["output"], company=company,
                              global_period=period, additional_continuation_ticks=3000, spendable=company["cash"] + 5000000 - company["debt"],
                              service_lives=service, renewal_due=min(row["remaining"] for row in service) <= 732, reload_receipt=identity(path)))
        worlds.append(stages); rows.append(comparable)
    assert rows[0] == rows[1] and len(processes) == 6
    for first, second in zip(*worlds, strict=True):
        assert first.keys() == second.keys() and len(first) == 121 and all(first[key] == second[key] for key in first if key != b"GLOG")
    previous = json.loads(products_path.read_text(encoding="utf-8")); copies = list(previous["products"])
    # Reuse only byte-identical current products. A changed native producer
    # needs its own new tested build contract before this boundary can retain it.
    for product in copies:
        assert identity(ROOT / product["source"]["path"]) == product["source"]
        assert identity(ROOT / product["retained"]["path"]) == product["retained"]
    retained = ROOT / f"test-cache/surface-entry/treasury-boundary-{period}-products"; assert not retained.exists(); retained.mkdir(parents=True)
    source = Path(__file__); target = retained / source.name; shutil.copyfile(source, target)
    assert identity(source)["sha256"] == identity(target)["sha256"]; copies.append(dict(source=identity(source), retained=identity(target)))
    manifest = retained / "retained.json"
    manifest.write_text(json.dumps(dict(_documentation=__doc__, products=copies, predecessor=identity(products_path)), indent=2) + "\n", encoding="utf-8")
    return dict(_documentation=__doc__, complete=True, foundation=identity(foundation_path), configurations=summaries, native_processes=6,
                products=identity(manifest), cross_build_gameplay_comparisons=360, exact_replay_gameplay_comparisons=240,
                actual_capital_complete=all(row["spendable"] >= 1870000 for row in summaries), actual_launch_complete=False, source=identity(source))


def main():
    """Publish one exact current treasury replay or its retained both-build boundary."""
    parser = argparse.ArgumentParser(description=__doc__); parser.add_argument("--kind", choices=("reload", "final"), required=True)
    parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug")); parser.add_argument("--foundation", type=Path, required=True)
    parser.add_argument("--products", type=Path); parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args(); output = (ROOT / args.output).resolve(); assert output.is_relative_to(ROOT / "logs") and not output.exists()
    if args.kind == "reload":
        assert args.configuration is not None; value = reload(args.configuration, (ROOT / args.foundation).resolve())
    else:
        assert args.products is not None; value = final((ROOT / args.foundation).resolve(), (ROOT / args.products).resolve())
    output.write_text(json.dumps(value, indent=2) + "\n", encoding="utf-8"); print(output.relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
