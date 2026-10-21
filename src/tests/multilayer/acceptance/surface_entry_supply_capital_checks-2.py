"""Bind real bounded fuel savings and preserve a still-unfunded original world.

@file surface_entry_supply_capital_checks-2.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CI.
Partial earned work and native expenses remain actual. A completed evidence
report grants neither funded capital nor completed original savings tasks.
"""
import argparse
import json
from pathlib import Path
import shutil

from space_preview_checks import ROOT, chunks, identity
from surface_gateway_component_delivery_ledger import observe_retained as observe
from surface_gateway_replay import load, validate

FOUNDATION = ROOT / "logs/surface-entry-supply-admission-electric-verification.json"
ADMISSION = ROOT / "logs/surface-entry-supply-capital-admission-verification.json"
STEM = "surface-entry-supply-capital"


def verify(reference):
    """@param reference Exact current native product. Refuse missing or changed authorities."""
    assert identity(ROOT / reference["path"]) == reference


def stage(path, predecessor, processes, products):
    """Verify a linked native stage without counting its diagnostic as gameplay authority.

    @param path Existing immutable stage receipt.
    @param predecessor Exact immediately preceding genuine receipt identity.
    @param processes Accumulated distinct native process report paths.
    @param products Current tested source/executable identities to preserve.
    @return Verified operation and complete output-family map.
    """
    operation = load(path)["operation"]
    assert operation["receipt"] == predecessor
    assert operation["source"] == load(ROOT / predecessor["path"])["operation"]["output"]
    for key in ("source", "before", "output", "application", "terminal", "retained_harness"):
        verify(operation[key])
    for reference in operation["sources"]:
        verify(reference)
        products[reference["path"]] = reference
    products[operation["application"]["path"]] = operation["application"]
    for reference in operation["native_processes"]:
        verify(reference)
        report = json.loads((ROOT / reference["path"]).read_text(encoding="utf-8"))
        assert report["exit_status"] == 0 and report["inputs"] == [operation["source"]]
        assert report["executable"] == operation["application"]
        processes.add(reference["path"])
    assert json.loads(json.dumps(observe(ROOT / operation["output"]["path"]))) == operation["final"]
    assert operation["initial"]["company"]["debt"] == operation["final"]["company"]["debt"] == 5000000
    return operation, chunks(ROOT / operation["output"]["path"])[1]


def bind(work_count, retain):
    """Validate bounded income, exact replay and fresh current funding refusal in both builds.

    @param work_count Actual original service bursts preceding ordinary replay.
    @param retain Whether to copy exact current authorities for the next prerequisite.
    @return Explicit current original state, actual books and incomplete fuel funding.
    """
    foundation = load(FOUNDATION)
    admission = load(ADMISSION)
    assert admission["foundation"] == identity(FOUNDATION)
    previous_path = ROOT / foundation["products"]["path"]
    verify(foundation["products"])
    previous = json.loads(previous_path.read_text(encoding="utf-8"))
    for product in previous["products"]:
        verify(product["source"])
        verify(product["retained"])
    processes, products, worlds, diagnostics, configurations = set(), {}, [], [], []
    # Follow every paid original tick interval in order. Replayed copy income
    # can prove determinism but cannot increase original spendable capital.
    for entry in admission["configurations"]:
        configuration = entry["configuration"]
        predecessor = entry["receipt"]
        stages, rows, income, net_expense = [], [], 0, 0
        for index in range(1, work_count + 1):
            path = ROOT / f"logs/{STEM}-work-{index:02d}-{configuration}.json"
            operation, output = stage(path, predecessor, processes, products)
            earned, = operation["rows"]["earned"]
            assert earned["earned"] == 3000 and earned["debt"] == 5000000
            assert operation["final"]["company"]["cash"] == earned["cash_before"] - earned["net_expense"]
            income += earned["income"]
            net_expense += earned["net_expense"]
            predecessor = identity(path)
            stages.append(output)
            rows.append(operation["rows"])
        summary = load(ROOT / f"logs/{STEM}-replay-{configuration}.json")
        selected, replay_stages, replay_rows, replay_processes, applications = validate(summary, dict(receipt=predecessor), observe)
        processes.update(replay_processes)
        products.update(applications)
        for reference in summary["continuations"]:
            for authority in load(ROOT / reference["path"])["operation"]["sources"]:
                products[authority["path"]] = authority
        earned, = replay_rows[0]["earned"]
        income += earned["income"]
        net_expense += earned["net_expense"]
        predecessor = summary["continuations"][0]
        stages.extend(replay_stages)
        rows.extend(replay_rows)
        # Quotes after actual original continuation must still preserve every
        # native family. They disclose the current shortfall instead of treating
        # gross passenger/freight receipts as net capital or produced fuel.
        for boundary in ("modules", "refill"):
            path = ROOT / f"logs/{STEM}-current-{boundary}-{configuration}.json"
            operation, output = stage(path, predecessor, processes, products)
            assert chunks(ROOT / operation["before"]["path"])[1] == output
            assert operation["initial"] == operation["final"]
            predecessor = identity(path)
            stages.append(output)
            rows.append(operation["rows"])
        target = min(window["target"] for window in operation["rows"]["window"])
        current = operation["final"]
        assert current["company"]["cash"] == entry["company"]["cash"] - net_expense
        configurations.append(dict(configuration=configuration, receipt=predecessor, output=operation["output"],
                                   company=current["company"], income=income, net_expense=net_expense,
                                   earned_ticks=(work_count + 1) * 3000, target=target,
                                   shortfall=max(0, target - current["company"]["cash"]),
                                   funded=current["company"]["cash"] >= target, service_lives=current["service_lives"],
                                   totals=current["totals"], cargo_abandoned=current["cargo_abandoned"]))
        worlds.append(stages)
        diagnostics.append(rows)
    assert diagnostics[0] == diagnostics[1]
    # A game-version metadata row may differ between independent builds;
    # original gameplay, restore and replay maps must otherwise match exactly.
    comparisons = 0
    for first, second in zip(*worlds, strict=True):
        assert first.keys() == second.keys() and len(first) == 121
        assert all(first[key] == second[key] for key in first if key != b"GLOG")
        comparisons += len(first) - 1
    result = dict(_documentation=__doc__, complete=True, savings_complete=all(row["funded"] for row in configurations),
                  fuel_procured=False, conversion_paid=False, replacement_owned=False,
                  foundation=identity(FOUNDATION), admission=identity(ADMISSION), configurations=configurations,
                  native_processes=len(processes), cross_build_gameplay_comparisons=comparisons,
                  exact_original_copy_comparisons=240, source=identity(Path(__file__)))
    if retain:
        retained = ROOT / "test-cache/surface-entry/supply-capital-products"
        assert not retained.exists()
        retained.mkdir(parents=True)
        copies = list(previous["products"])
        existing = {product["source"]["path"] for product in copies}
        for index in (1, 2):
            reference = identity(Path(__file__).with_name(f"surface_entry_supply_capital_checks-{index}.py"))
            products[reference["path"]] = reference
        # Preserve each newly tested authority once; accepted predecessor
        # copies remain exact rather than being overwritten by this boundary.
        for index, reference in enumerate(products.values()):
            verify(reference)
            if reference["path"] in existing:
                continue
            source = ROOT / reference["path"]
            target = retained / f"{index:03d}-{source.parent.name}-{source.name}"
            shutil.copyfile(source, target)
            assert identity(target)["sha256"] == reference["sha256"]
            copies.append(dict(source=reference, retained=identity(target)))
            existing.add(reference["path"])
        manifest = retained / "retained.json"
        manifest.write_text(json.dumps(dict(_documentation=__doc__, predecessor=foundation["products"], products=copies), indent=2) + "\n", encoding="utf-8")
        result["products"] = identity(manifest)
        result["retained_products"] = len(copies)
    return result


def main():
    """Publish exact current savings evidence while keeping unfunded tasks explicitly open."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--work-count", type=int, required=True)
    parser.add_argument("--retain", action="store_true")
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    assert args.work_count > 0
    output = ROOT / args.output
    assert output.resolve().is_relative_to(ROOT / "logs") and not output.exists()
    output.write_text(json.dumps(bind(args.work_count, args.retain), indent=2) + "\n", encoding="utf-8")
    print(output.relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
