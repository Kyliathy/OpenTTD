"""Accept real due-local-bus renewal, ordinary continuation and exact current products.

@file surface_entry_regional_renewal_checks-3.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15BIEQC.
Paid replacements preserve all healthy service and launch stock. Actual funds
remain below full launch admission; no source-plan aggregate closes here.
"""
import argparse
import json
from pathlib import Path
import runpy
import shutil

from space_preview_checks import ROOT, chunks, identity
from surface_regional_ledger import observe, lives
from surface_regional_replay import replay

DIRECTORY = Path(__file__).resolve().parent


def final():
    """@return Both-build genuine local renewal and ordinary/exact continuation with immutable current tooling/native products."""
    foundation_path = ROOT / "logs/surface-entry-local-renewal-purchase-verification.json"
    foundation = json.loads(foundation_path.read_text(encoding="utf-8")); assert foundation["complete"]
    summaries, worlds, rows, processes, versions = [], [], [], set(), {}
    for configuration in ("RelWithDebInfo", "Debug"):
        path = ROOT / f"logs/surface-entry-local-renewal-reload-{configuration}.json"
        value = json.loads(path.read_text(encoding="utf-8")); assert value["complete"] and value["configuration"] == configuration
        expected = ROOT / f"logs/surface-entry-local-renewal-renew-{configuration}.json"; assert value["source_receipt"] == identity(expected)
        restore = json.loads((ROOT / value["restore"]["path"]).read_text(encoding="utf-8"))["operation"]
        original, restored = [chunks(ROOT / restore[field]["path"])[1] for field in ("source", "output")]
        normalized = dict(restored); normalized[b"DATE"] = original[b"DATE"]; stages = [normalized]
        processes.update(reference["path"] for reference in restore["native_processes"])
        for phase in ("query", "recall", "work-1", "renew"):
            receipt = ROOT / f"logs/surface-entry-local-renewal-{phase}-{configuration}.json"
            operation = json.loads(receipt.read_text(encoding="utf-8"))["operation"]
            for field in ("retained_harness", "retained_wrapper"):
                reference = operation[field]; assert identity(ROOT / reference["path"]) == reference; versions[reference["sha256"]] = reference
            processes.update(reference["path"] for reference in operation["native_processes"])
        comparable = []
        for reference in value["continuations"]:
            assert identity(ROOT / reference["path"]) == reference
            operation = json.loads((ROOT / reference["path"]).read_text(encoding="utf-8"))["operation"]
            for field in ("source", "before", "output", "application", "terminal", "retained_harness"):
                assert identity(ROOT / operation[field]["path"]) == operation[field]
            stages.append(chunks(ROOT / operation["output"]["path"])[1]); comparable.append(operation["rows"])
            processes.update(item["path"] for item in operation["native_processes"])
            retained = operation["retained_harness"]; versions[retained["sha256"]] = retained
        assert comparable[0] == comparable[1]
        source = ROOT / value["output"]["path"]; assert observe(source)["company"] == value["company"]
        service = lives(source); assert len(service) == 11 and min(row["remaining"] for row in service) > 732
        company = value["company"]
        summaries.append(dict(configuration=configuration, receipt=value["continuations"][0], output=value["output"], company=company,
                              global_period=26, additional_continuation_ticks=3000, spendable=company["cash"] + 5000000 - company["debt"],
                              service_lives=service, reload_receipt=identity(path)))
        worlds.append(stages); rows.append(comparable)
    assert rows[0] == rows[1] and len(processes) == 14
    for first, second in zip(*worlds, strict=True):
        assert first.keys() == second.keys() and len(first) == 121 and all(first[key] == second[key] for key in first if key != b"GLOG")
    previous_manifest = ROOT / "test-cache/surface-entry/capital-boundary-26-products/retained.json"
    previous = json.loads(previous_manifest.read_text(encoding="utf-8")); copies = list(previous["products"])
    # Existing identical native and financial products retain their original
    # immutable copies; this producer adds only its current exact sources.
    for product in copies:
        assert identity(ROOT / product["source"]["path"]) == product["source"]
        assert identity(ROOT / product["retained"]["path"]) == product["retained"]
    retained = ROOT / "test-cache/surface-entry/local-renewal-products"; assert not retained.exists(); retained.mkdir(parents=True)
    products = [ROOT / "src/tests/surface_entry_bus_renewal.cpp", DIRECTORY / "surface_entry_bus_checks-1.py", DIRECTORY / "surface_regional_replay.py"]
    products += [DIRECTORY / f"surface_entry_regional_renewal_checks-{index}.py" for index in (1, 2, 3)]
    for index, source in enumerate(products):
        target = retained / f"{index:02d}-{source.name}"; shutil.copyfile(source, target)
        assert identity(source)["sha256"] == identity(target)["sha256"]; copies.append(dict(source=identity(source), retained=identity(target)))
    query_proof = json.loads((ROOT / "logs/surface-entry-local-renewal-plan-verification.json").read_text(encoding="utf-8"))
    old_binder = ROOT / "test-cache/surface-entry/local-renewal-source-versions/before-order-fix-checks-2.py"
    assert identity(old_binder)["sha256"] == query_proof["source"]["sha256"]
    manifest = retained / "retained.json"
    manifest.write_text(json.dumps(dict(_documentation=__doc__, products=copies, predecessor=identity(previous_manifest),
                                      invocation_driver_versions=list(versions.values()), original_query_binder=identity(old_binder)), indent=2) + "\n", encoding="utf-8")
    return dict(_documentation=__doc__, complete=True, actual_local_renewal_complete=True, actual_capital_complete=False, actual_launch_complete=False,
                foundation=identity(foundation_path), configurations=summaries, native_processes=len(processes), products=identity(manifest),
                cross_build_gameplay_comparisons=360, exact_replay_gameplay_comparisons=240, source=identity(Path(__file__)))


def main():
    """Publish current ordinary proof or the exact due-local-renewal handoff, preserving every successful native action."""
    parser = argparse.ArgumentParser(description=__doc__); parser.add_argument("--kind", choices=("reload", "final"), required=True)
    parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug")); parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args(); output = (ROOT / args.output).resolve(); assert output.is_relative_to(ROOT / "logs") and not output.exists()
    if args.kind == "reload":
        assert args.configuration is not None
        value = replay(args.configuration, ROOT / f"logs/surface-entry-local-renewal-renew-{args.configuration}.json", "surface-entry-local-renewal")
    else:
        value = final()
    output.write_text(json.dumps(value, indent=2) + "\n", encoding="utf-8"); print(output.relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
