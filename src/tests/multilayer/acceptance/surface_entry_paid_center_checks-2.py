"""Bind the genuine paid center, exclusive hub and ordinary useful continuation.

@file surface_entry_paid_center_checks-2.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15BIEQK.
All original128shipment units remain in real carrier custody. A facility alone
does not prove material delivery, paid hulls, flight or an orbital Core.
"""
import argparse
import json
from pathlib import Path
import shutil

from space_preview_checks import ROOT, chunks, identity
from surface_gateway_facility_ledger import observe, facility
from surface_regional_ledger import observe as observe_regional, lives
from surface_regional_replay import replay

DIRECTORY = Path(__file__).resolve().parent
FOUNDATION = ROOT / "logs/surface-entry-funded-gateway-verification.json"
PRODUCTS = ROOT / "test-cache/surface-entry/funded-gateway-products/retained.json"
STEM = "surface-entry-paid-center"


def load(path):
    """@param path Complete immutable acceptance receipt. @return Accepted receipt object."""
    value = json.loads(path.read_text(encoding="utf-8")); assert value["complete"]
    return value


def validate(operation, observe_fn=observe):
    """@param operation Actual native boundary. @param observe_fn Explicit sole-owner contract for its stage.
    @return Original gameplay families after full source/process/current application verification.
    """
    for field in ("source", "before", "output", "application", "terminal", "retained_harness"):
        reference = operation[field]; assert identity(ROOT / reference["path"]) == reference
    for reference in operation["sources"] + operation["native_processes"]:
        if identity(ROOT / reference["path"]) == reference:
            continue
        # The two pure admissions used the exact preserved decoder before its
        # empty VehicleID sentinel was corrected. Their absent-center branch
        # is identical; this qualifies no paid or native/source replacement.
        assert reference["path"] == "src/tests/multilayer/acceptance/surface_gateway_facility_ledger.py"
        old = ROOT / "test-cache/surface-entry/paid-center-source-versions/before-empty-slot-fix-ledger.py"
        assert identity(old)["sha256"] == reference["sha256"]
        assert not operation["rows"]["paid"] and operation["initial"]["launch_facility"]["center"] is None
    for reference in operation["native_processes"]:
        report = json.loads((ROOT / reference["path"]).read_text(encoding="utf-8"))
        assert report["exit_status"] == 0 and report["inputs"] == [operation["source"]] and report["executable"] == operation["application"]
    output = ROOT / operation["output"]["path"]
    assert json.loads(json.dumps(observe_fn(output))) == operation["final"]
    return chunks(output)[1]


def bind(paid=False):
    """@param paid Whether actual paid construction follows the pure admission. @return Exact both-build facility/payment/material lineage."""
    foundation = load(FOUNDATION); phases = ["query", "center"] if paid else ["query"]
    summaries, worlds, rows, processes = [], [], [], set()
    # Each build follows its own original world; all phases must agree before
    # a further command or physical material-delivery producer may depend on it.
    for entry in foundation["configurations"]:
        configuration = entry["configuration"]; previous = ROOT / entry["receipt"]["path"]
        stages, comparable, references = [], [], []
        for phase in phases:
            path = ROOT / f"logs/{STEM}-{phase}-{configuration}.json"; value = load(path)
            operation = value["operation"]
            assert value["configuration"] == configuration and operation["receipt"] == identity(previous)
            assert operation["source"] == load(previous)["operation"]["output"]
            stages.append(validate(operation)); comparable.append(operation["rows"]); references.append(identity(path))
            processes.update(reference["path"] for reference in operation["native_processes"]); previous = path
        company = operation["final"]["company"]
        summaries.append(dict(configuration=configuration, receipt=identity(previous), output=operation["output"], company=company,
                              global_period=40, spendable=company["cash"] + 5000000 - company["debt"],
                              facility=operation["final"]["launch_facility"], rows=operation["rows"], receipts=references))
        worlds.append(stages); rows.append(comparable)
    assert rows[0] == rows[1]
    for first, second in zip(*worlds, strict=True):
        assert first.keys() == second.keys() and len(first) == 121
        assert all(first[key] == second[key] for key in first if key != b"GLOG")
    return dict(_documentation=__doc__, complete=True, actual_center_built=paid, actual_launch_complete=False,
                foundation=identity(FOUNDATION), configurations=summaries, native_processes=len(processes),
                cross_build_gameplay_comparisons=len(phases) * 120, source=identity(Path(__file__)))


def final():
    """@return Paid center/hub, exact ordinary continuation and current tested products before cargo movement."""
    purchase = bind(True); summaries, worlds, rows, processes, applications = [], [], [], set(), {}
    # Ordinary restoration independently validates the new native center/hub
    # families. Both following real3000tick branches must preserve exact gameplay.
    for entry in purchase["configurations"]:
        configuration = entry["configuration"]; path = ROOT / f"logs/{STEM}-reload-{configuration}.json"; value = load(path)
        assert value["source_receipt"] == entry["receipt"] and value["additional_continuation_ticks"] == 3000
        restore = load(ROOT / value["restore"]["path"])["operation"]
        original, restored = [chunks(ROOT / restore[field]["path"])[1] for field in ("source", "output")]
        normalized = dict(restored); normalized[b"DATE"] = original[b"DATE"]; stages, comparable = [normalized], []
        processes.update(reference["path"] for reference in restore["native_processes"])
        for reference in value["continuations"]:
            assert identity(ROOT / reference["path"]) == reference
            operation = load(ROOT / reference["path"])["operation"]
            stages.append(validate(operation, observe_regional)); comparable.append(operation["rows"])
            assert facility(ROOT / operation["output"]["path"]) == entry["facility"]
            processes.update(item["path"] for item in operation["native_processes"])
            applications[operation["application"]["path"]] = operation["application"]
        assert comparable[0] == comparable[1]
        snapshot = observe(ROOT / value["output"]["path"]); company = snapshot["company"]
        assert company == value["company"] and snapshot["launch_facility"] == entry["facility"]
        service = lives(ROOT / value["output"]["path"]); assert min(row["remaining"] for row in service) > 732
        summaries.append(dict(configuration=configuration, receipt=value["continuations"][0], output=value["output"], company=company,
                              global_period=40, additional_continuation_ticks=3000, spendable=company["cash"] + 5000000 - company["debt"],
                              facility=entry["facility"], service_lives=service, actual_rows=comparable[0], reload_receipt=identity(path)))
        worlds.append(stages); rows.append(comparable)
    assert rows[0] == rows[1] and len(processes) == 6 and len(applications) == 2
    for first, second in zip(*worlds, strict=True):
        assert first.keys() == second.keys() and len(first) == 121
        assert all(first[key] == second[key] for key in first if key != b"GLOG")
    previous = json.loads(PRODUCTS.read_text(encoding="utf-8")); copies = []
    replaced = set(applications) | {"src/tests/surface_entry_gateway.cpp"}
    # Only this actually tested native producer and its two focused binaries
    # replace prior current products. All old bytes remain in prior immutable
    # copies, and every other inherited source must still match exactly.
    for product in previous["products"]:
        assert identity(ROOT / product["retained"]["path"]) == product["retained"]
        if product["source"]["path"] not in replaced:
            assert identity(ROOT / product["source"]["path"]) == product["source"]; copies.append(product)
    retained = ROOT / "test-cache/surface-entry/paid-center-products"; assert not retained.exists(); retained.mkdir(parents=True)
    sources = [ROOT / path for path in applications] + [ROOT / "src/tests/surface_entry_gateway.cpp", DIRECTORY / "surface_gateway_facility_ledger.py",
                                                      DIRECTORY / "surface_entry_paid_center_checks-1.py", Path(__file__)]
    for index, source in enumerate(sources):
        target = retained / f"{index:02d}-{source.parent.name}-{source.name}"; shutil.copyfile(source, target)
        assert identity(source)["sha256"] == identity(target)["sha256"]; copies.append(dict(source=identity(source), retained=identity(target)))
    manifest = retained / "retained.json"
    manifest.write_text(json.dumps(dict(_documentation=__doc__, products=copies, predecessor=identity(PRODUCTS)), indent=2) + "\n", encoding="utf-8")
    return dict(_documentation=__doc__, complete=True, actual_center_complete=True, actual_launch_complete=False,
                foundation=identity(ROOT / "logs/surface-entry-paid-center-purchase-verification.json"), configurations=summaries,
                native_processes=purchase["native_processes"] + len(processes), products=identity(manifest),
                cross_build_gameplay_comparisons=360, exact_replay_gameplay_comparisons=240, source=identity(Path(__file__)))


def main():
    """Retain only the completed next actual facility boundary, never repeating successful native work."""
    parser = argparse.ArgumentParser(description=__doc__); parser.add_argument("--kind", choices=("plan", "purchase", "reload", "final"), required=True)
    parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug")); parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args(); output = (ROOT / args.output).resolve(); assert output.is_relative_to(ROOT / "logs") and not output.exists()
    if args.kind == "reload":
        value = replay(args.configuration, ROOT / f"logs/{STEM}-center-{args.configuration}.json", STEM)
    elif args.kind == "final":
        value = final()
    else:
        value = bind(args.kind == "purchase")
    output.write_text(json.dumps(value, indent=2) + "\n", encoding="utf-8"); print(output.relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
