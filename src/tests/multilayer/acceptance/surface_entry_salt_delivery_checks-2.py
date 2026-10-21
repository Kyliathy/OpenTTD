"""Bind original Salt delivery, banked sale, remaining quote and ordinary restoration.

@file surface_entry_salt_delivery_checks-2.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CIE.3/.4.
Historical delivery applications retain exact preserved bytes. Only original
outputs supply future chemical work; ordinary static copies prove loading.
"""
import argparse
import json
from pathlib import Path
import runpy
import shutil

from space_preview_checks import ROOT, chunks, identity
from surface_gateway_component_delivery_ledger import observe_salt_delivery as observe
from surface_gateway_replay import RESTORE
from surface_salt_native import execute as native

DIRECTORY = Path(__file__).resolve().parent
STATIC = runpy.run_path(str(DIRECTORY / "surface_entry_salt_window_checks-4.py"))
FILES = runpy.run_path(str(DIRECTORY / "surface_entry_salt_delivery_checks-1.py"))["FILES"]
FOUNDATION = ROOT / "logs/surface-entry-salt-return-verification.json"
VERSIONS = [ROOT / f"test-cache/surface-entry/salt-window-source-versions/{name}/retained.json"
            for name in ("before-salt-delivery", "before-remaining-fuel-query")]


def verify(reference):
    """Validate current bytes or one explicitly preserved historical native authority.

    @param reference Exact native source, process, application or saved artifact identity.
    @return Whether these bytes also match the current authority at its original path.
    """
    path = ROOT / reference["path"]
    if path.is_file():
        if identity(path) == reference:
            return True
    candidates = []
    # Only the two explicit before-edit manifests may resolve historical
    # delivery applications. No stale-source wildcard or altered receipt is
    # admitted merely because some unrelated retained file has matching bytes.
    for manifest in VERSIONS:
        for row in json.loads(manifest.read_text(encoding="utf-8"))["products"]:
            if row["source"] == reference["path"] and row["sha256"] == reference["sha256"]:
                candidates.append(row)
    assert candidates, reference
    assert all(identity(ROOT / row["path"])["sha256"] == reference["sha256"] for row in candidates)
    return False


def remaining(configuration, receipt, recover_run=None):
    """Quote only remaining carrier/fuel costs from actual post-sale cash.

    @param configuration Actual independently linked focused application.
    @param receipt Genuine empty-train sale receipt from the original branch.
    @param recover_run Existing successful pure invocation to inspect without repeating it.
    @return Unchanged physical owners and complete bounded remaining-cost disclosure.
    """
    predecessor = json.loads(receipt.read_text(encoding="utf-8"))
    assert predecessor["complete"] and predecessor["phase"] == "staged-empty-sale"
    files = [Path(__file__)] + [DIRECTORY / name for name in FILES]
    files += [DIRECTORY / name for name in ("surface_entry_chemical_checks-4.py", "surface_propellant_ledger.py", "surface_regional_ledger.py")]
    operation = native(configuration, "staged-remaining-query", receipt, observe,
                       dict(remaining="REMAINING", routes="ROUTE", invoices="INVOICE", earned="EARNED"), files, recover_run)
    assert json.loads(json.dumps(operation["initial"])) == predecessor["operation"]["final"]
    assert operation["initial"] == operation["final"]
    assert chunks(ROOT / operation["before"]["path"])[1] == chunks(ROOT / operation["output"]["path"])[1]
    row, = operation["rows"]["remaining"]
    assert row["reserve"] == 100000 and row["salt_delivered"] == 60 and row["salt_train_sold"]
    assert row["refund_counted"] == row["credit_counted"] == row["infrastructure"] == 0
    assert row["target"] == row["replacement_cost"] + row["handling"] + row["work_allowance"] + row["reserve"]
    assert row["funded"] == (row["cash"] >= row["target"])
    assert not operation["rows"]["invoices"] and not operation["rows"]["earned"]
    return dict(_documentation=__doc__, complete=True, configuration=configuration, phase="staged-remaining-query", operation=operation)


def restore(configuration, receipt):
    """Prove ordinary loading of the selected original after real delivery and sale.

    @param configuration Actual complete ordinary game application.
    @param receipt Selected original remaining-cost quote, with no added work.
    @return Ordinary process identities and exact paused original owner preservation.
    """
    predecessor = json.loads(receipt.read_text(encoding="utf-8"))
    assert predecessor["complete"] and predecessor["phase"] == "staged-remaining-query"
    original = predecessor["operation"]["output"]
    verify(original)
    source = ROOT / original["path"]
    restored, operation = RESTORE["ORDINARY"](configuration, source, "salt-delivery-static-" + configuration.lower())
    RESTORE["REVISION"](source, restored)
    pause = RESTORE["restore_pause"](source, restored)
    first, last = [chunks(path)[1] for path in (source, restored)]
    assert first.keys() == last.keys() and len(first) == 121
    assert all(first[key] == last[key] for key in first if key not in (b"GLOG", b"DATE"))
    assert observe(source) == observe(restored)
    return dict(_documentation=__doc__, complete=True, configuration=configuration, phase="restore",
                receipt=identity(receipt), operation=operation, original_pause=pause, source=identity(Path(__file__)))


def stage(path, predecessor, processes, products):
    """Check one exact current original boundary and its actual native process.

    @param path Existing actual receipt, never regenerated for binding.
    @param predecessor Expected immediately preceding original receipt identity.
    @param processes Distinct accepted process report paths.
    @param products Exact current authority identities to retain.
    @return Verified original operation and complete output gameplay-family map.
    """
    receipt = json.loads(path.read_text(encoding="utf-8"))
    assert receipt["complete"]
    operation = receipt["operation"]
    assert operation["receipt"] == predecessor
    assert operation["source"] == json.loads((ROOT / predecessor["path"]).read_text(encoding="utf-8"))["operation"]["output"]
    for field in ("source", "before", "output", "application", "terminal", "retained_harness"):
        verify(operation[field])
    for reference in operation["sources"] + [operation["application"]]:
        if verify(reference):
            products[reference["path"]] = reference
    for reference in operation["native_processes"]:
        verify(reference)
        report = json.loads((ROOT / reference["path"]).read_text(encoding="utf-8"))
        assert report["exit_status"] == 0 and report["inputs"] == [operation["source"]]
        assert report["executable"] == operation["application"]
        processes.add(reference["path"])
    assert json.loads(json.dumps(observe(ROOT / operation["output"]["path"]))) == operation["final"]
    return operation, chunks(ROOT / operation["output"]["path"])[1]


def bind(retain):
    """Bind the complete actual delivery/sale chain without claiming chemical renewal or fuel.

    @param retain Whether to preserve exact current tested products for the next original producer.
    @return Independent native, cross-build and ordinary-load acceptance with current capital.
    """
    foundation = json.loads(FOUNDATION.read_text(encoding="utf-8"))
    assert foundation["complete"] and foundation["salt_return_complete"]
    verify(foundation["products"])
    previous = json.loads((ROOT / foundation["products"]["path"]).read_text(encoding="utf-8"))
    changed = set()
    for manifest in VERSIONS:
        changed.update(row["source"] for row in json.loads(manifest.read_text(encoding="utf-8"))["products"])
    for product in previous["products"]:
        verify(product["retained"])
        verify(product["source"])
    products, processes, worlds, diagnostics, configurations = {}, set(), [], [], []
    for configuration in ("RelWithDebInfo", "Debug"):
        predecessor = identity(ROOT / f"logs/surface-entry-salt-return-04-{configuration}.json")
        stages, rows = [], []
        names = ("salt-delivery-query-01", "salt-delivery-order", "salt-delivery-work-01", "salt-empty-sale", "remaining-fuel-query")
        for name in names:
            path = ROOT / f"logs/surface-entry-{name}-{configuration}.json"
            operation, output = stage(path, predecessor, processes, products)
            stages.append(output)
            rows.append(operation["rows"])
            predecessor = identity(path)
        final = operation["final"]
        admission, = operation["rows"]["remaining"]
        assert admission["funded"] and final["company"]["cash"] >= 100000
        assert final["salt_delivery"]["delivered"] == 60 and "salt_consist" not in final
        path = ROOT / f"logs/surface-entry-salt-delivery-static-{configuration}.json"
        restored = json.loads(path.read_text(encoding="utf-8"))
        assert restored["complete"] and restored["receipt"] == predecessor
        ordinary = restored["operation"]
        assert ordinary["source"] == operation["output"]
        for field in ("source", "output", "application", "terminal"):
            verify(ordinary[field])
        for reference in ordinary["native_processes"]:
            verify(reference)
            report = json.loads((ROOT / reference["path"]).read_text(encoding="utf-8"))
            assert report["exit_status"] == 0 and report["inputs"] == [operation["output"]]
            STATIC["verify_ordinary_application"](report["executable"], ordinary["application"])
            products[report["executable"]["path"]] = report["executable"]
            processes.add(reference["path"])
        products[ordinary["application"]["path"]] = ordinary["application"]
        assert json.loads(json.dumps(observe(ROOT / ordinary["output"]["path"]))) == final
        normalized = dict(chunks(ROOT / ordinary["output"]["path"])[1])
        normalized[b"DATE"] = output[b"DATE"]
        stages.append(normalized)
        worlds.append(stages)
        diagnostics.append(rows)
        configurations.append(dict(configuration=configuration, receipt=predecessor, output=operation["output"],
                                   company=final["company"], salt_delivery=final["salt_delivery"],
                                   service_lives=final["service_lives"], totals=final["totals"], ordinary=identity(path)))
    assert len(processes) == 12 and diagnostics[0] == diagnostics[1]
    comparisons = 0
    for first, second in zip(*worlds, strict=True):
        assert first.keys() == second.keys() and len(first) == 121
        assert all(first[key] == second[key] for key in first if key != b"GLOG")
        comparisons += len(first) - 1
    result = dict(_documentation=__doc__, complete=True, salt_delivered=True, empty_train_sold=True,
                  reserve_restored=True, remaining_window_funded=True, chemical_renewed=False, fuel_procured=False,
                  foundation=identity(FOUNDATION), historical_versions=[identity(path) for path in VERSIONS],
                  native_processes=12, cross_build_gameplay_comparisons=comparisons, ordinary_static_reloads=2,
                  configurations=configurations, source=identity(Path(__file__)))
    if retain:
        retained = ROOT / "test-cache/surface-entry/salt-delivery-products"
        retained.mkdir(parents=True, exist_ok=False)
        copies = [product for product in previous["products"] if product["source"]["path"] not in changed]
        existing = {product["source"]["path"] for product in copies}
        for path in [Path(__file__), DIRECTORY / "surface_entry_salt_delivery_checks-1.py"] + VERSIONS:
            products[identity(path)["path"]] = identity(path)
        # Reuse unchanged prior authorities; preserve only current tested new
        # sources and applications. Historical versions stay at exact original
        # manifests and never masquerade as current implementations.
        for index, reference in enumerate(products.values()):
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
    """Publish a fresh pure quote, ordinary static proof or complete actual delivery binding."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--phase", choices=("remaining", "restore", "bind"), required=True)
    parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"))
    parser.add_argument("--receipt", type=Path)
    parser.add_argument("--recover-run")
    parser.add_argument("--retain", action="store_true")
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    output = ROOT / args.output
    assert output.resolve().is_relative_to(ROOT / "logs") and not output.exists()
    if args.phase == "bind":
        value = bind(args.retain)
    else:
        assert args.configuration is not None and args.receipt is not None
        value = remaining(args.configuration, ROOT / args.receipt, args.recover_run) if args.phase == "remaining" else restore(args.configuration, ROOT / args.receipt)
    output.write_text(json.dumps(value, indent=2) + "\n", encoding="utf-8")
    print(output.relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
