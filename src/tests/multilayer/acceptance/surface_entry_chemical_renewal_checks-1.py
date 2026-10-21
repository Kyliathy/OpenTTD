"""Verify genuine paid replacement of the original expired chemical tanker.

@file surface_entry_chemical_renewal_checks-1.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CJ.
The full new27capacity purchase precedes the actual empty old-tanker sale.
No movement, cargo, useful-service retirement or future refund funds renewal.
"""
import argparse
import json
from pathlib import Path
import runpy
import shutil

from space_preview_checks import ROOT, chunks, identity
from surface_gateway_component_delivery_ledger import observe_salt_delivery
from surface_gateway_replay import RESTORE
from surface_regional_ledger import TABLE
from surface_salt_native import execute as native

DIRECTORY = Path(__file__).resolve().parent
FILES = runpy.run_path(str(DIRECTORY / "surface_entry_salt_delivery_checks-1.py"))["FILES"]
STATIC = runpy.run_path(str(DIRECTORY / "surface_entry_salt_window_checks-4.py"))
FOUNDATION = ROOT / "logs/surface-entry-salt-delivery-verification.json"
VERSIONS = ROOT / "test-cache/surface-entry/salt-window-source-versions/before-chemical-renewal/retained.json"
ROAD = "roadveh[0].common[0]."


def observe(path):
    """Observe exact old or genuinely renewed chemical role while retaining every original owner.

    @param path Actual original before/after renewal or ordinary static copy.
    @return Original paid/material owners plus physical chemical-carrier identity and life.
    """
    result = observe_salt_delivery(path)
    selected = [(key, row) for key, row in TABLE(path, b"VEHS").items() if row["type"] == 1
                and bytes.fromhex(row[ROAD + "name"]).decode("utf-8") == "Surface Chemical reserve"]
    vehicle, row = selected[0]
    assert len(selected) == 1
    engine, capacity = row[ROAD + "engine_type"], row[ROAD + "cargo_cap"]
    age, life = row[ROAD + "age"], row[ROAD + "max_age"]
    # Only the exact expired historical model or the actual newly purchased
    # model may own this role. Renaming an unrelated useful carrier cannot
    # supply renewal, physical capacity or unearned service life.
    assert (engine == 132 and capacity == 21 and age >= life) or (engine == 134 and capacity == 27 and age == 0)
    assert row[ROAD + "tile"] == 2443 and row["roadveh[0].state"] == 254 and row[ROAD + "vehstatus"] & 2
    assert sum(row[ROAD + "cargo.action_counts"]) == 0 and row[ROAD + "cargo.packets"] == []
    result["chemical_carrier"] = dict(vehicle=vehicle, engine=engine, capacity=capacity, age=age, life=life,
                                      held=0, tile=2443, home=True)
    return result


def verify(reference):
    """@param reference Exact artifact identity. @return True for current bytes; only explicitly retained pre-renewal native bytes may differ at the original path."""
    path = ROOT / reference["path"]
    if path.is_file():
        if identity(path) == reference:
            return True
    row, = [row for row in json.loads(VERSIONS.read_text(encoding="utf-8"))["products"]
            if row["source"] == reference["path"] and row["sha256"] == reference["sha256"]]
    assert identity(ROOT / row["path"])["sha256"] == reference["sha256"]
    return False


def renew(configuration, receipt, recover_run=None):
    """Execute the funded original replacement with exact native invoices and zero earned ticks.

    @param configuration Independently linked focused native application.
    @param receipt Exact original funded remaining-window quote after banked train sale.
    @param recover_run Successful native invocation to inspect without repeating payment.
    @return Original books, exact actual new empty carrier and native command identities.
    """
    predecessor = json.loads(receipt.read_text(encoding="utf-8"))
    assert predecessor["complete"] and predecessor["phase"] == "staged-remaining-query"
    admitted, = predecessor["operation"]["rows"]["remaining"]
    assert admitted["funded"] and admitted["replacement"] == 134 and admitted["chemical_capacity"] == 27
    files = [Path(__file__)] + [DIRECTORY / name for name in FILES]
    operation = native(configuration, "staged-chemical-renew", receipt, observe,
                       dict(remaining="REMAINING", renewal="CHEMICAL_RENEWAL", invoices="INVOICE", earned="EARNED"), files, recover_run)
    first, last, rows = operation["initial"], operation["final"], operation["rows"]
    assert all(json.loads(json.dumps(first[key])) == value for key, value in predecessor["operation"]["final"].items())
    assert first["chemical_carrier"]["engine"] == 132 and last["chemical_carrier"]["engine"] == 134
    assert first["totals"] == last["totals"] == {"STEL": 32, "GOOD": 50, "PROP": 12}
    assert first["salt_delivery"] == last["salt_delivery"] and first["service_lives"] == last["service_lives"]
    assert first["company"]["debt"] == last["company"]["debt"] == 5000000
    renewal, = rows["renewal"]
    assert len(rows["invoices"]) == 3 and not rows["earned"]
    assert [row["cost"] for row in rows["invoices"]] == [renewal["gross"], renewal["refund"], 0]
    assert renewal["gross"] == admitted["replacement_cost"] and renewal["refund"] < 0
    assert last["company"]["cash"] == first["company"]["cash"] - renewal["net"] >= 100000
    dates = [TABLE(ROOT / operation[key]["path"], b"DATE")[0] for key in ("before", "output")]
    assert all(dates[0][key] == dates[1][key] for key in dates[0] if key not in ("random_state[0]", "random_state[1]"))
    return dict(_documentation=__doc__, complete=True, configuration=configuration, phase="staged-chemical-renew", operation=operation)


def restore(configuration, receipt):
    """@param configuration Complete ordinary game configuration. @param receipt Actual original renewal output. @return Native static load preserving new carrier and every original owner."""
    predecessor = json.loads(receipt.read_text(encoding="utf-8"))
    assert predecessor["complete"] and predecessor["phase"] == "staged-chemical-renew"
    original = predecessor["operation"]["output"]
    verify(original)
    source = ROOT / original["path"]
    restored, operation = RESTORE["ORDINARY"](configuration, source, "chemical-renewal-static-" + configuration.lower())
    RESTORE["REVISION"](source, restored)
    pause = RESTORE["restore_pause"](source, restored)
    first, last = [chunks(path)[1] for path in (source, restored)]
    assert first.keys() == last.keys() and len(first) == 121
    assert all(first[key] == last[key] for key in first if key not in (b"GLOG", b"DATE"))
    assert observe(source) == observe(restored)
    return dict(_documentation=__doc__, complete=True, configuration=configuration, phase="restore", receipt=identity(receipt),
                operation=operation, original_pause=pause, source=identity(Path(__file__)))


def bind(retain):
    """@param retain Preserve new exact tested source/application authorities. @return Both actual renewals and ordinary static proofs; no chemical transport or new fuel is claimed."""
    foundation = json.loads(FOUNDATION.read_text(encoding="utf-8"))
    assert foundation["complete"] and foundation["remaining_window_funded"]
    verify(foundation["products"])
    previous = json.loads((ROOT / foundation["products"]["path"]).read_text(encoding="utf-8"))
    for product in previous["products"]:
        verify(product["retained"])
        verify(product["source"])
    products, processes, worlds, diagnostics, configurations = {}, set(), [], [], []
    for configuration in ("RelWithDebInfo", "Debug"):
        path = ROOT / f"logs/surface-entry-chemical-renewal-{configuration}.json"
        receipt = json.loads(path.read_text(encoding="utf-8"))
        operation = receipt["operation"]
        assert receipt["complete"] and operation["receipt"] == identity(ROOT / f"logs/surface-entry-remaining-fuel-query-{configuration}.json")
        for field in ("source", "before", "output", "application", "terminal", "retained_harness"):
            verify(operation[field])
        for reference in operation["sources"] + [operation["application"]]:
            assert verify(reference)
            products[reference["path"]] = reference
        native_world = chunks(ROOT / operation["output"]["path"])[1]
        final = observe(ROOT / operation["output"]["path"])
        assert json.loads(json.dumps(final)) == operation["final"]
        static_path = ROOT / f"logs/surface-entry-chemical-renewal-static-{configuration}.json"
        restored = json.loads(static_path.read_text(encoding="utf-8"))
        assert restored["complete"] and restored["receipt"] == identity(path)
        ordinary = restored["operation"]
        assert ordinary["source"] == operation["output"]
        for field in ("source", "output", "application", "terminal"):
            verify(ordinary[field])
        # Each process must have consumed exactly the selected original. The
        # ordinary runner's console projection retains every gameplay byte and
        # all DLL bytes of the independently linked complete application.
        for current, original, is_ordinary in ((operation, operation["source"], False), (ordinary, operation["output"], True)):
            for reference in current["native_processes"]:
                verify(reference)
                report = json.loads((ROOT / reference["path"]).read_text(encoding="utf-8"))
                assert report["exit_status"] == 0 and report["inputs"] == [original]
                if is_ordinary:
                    STATIC["verify_ordinary_application"](report["executable"], ordinary["application"])
                else:
                    assert report["executable"] == operation["application"]
                products[report["executable"]["path"]] = report["executable"]
                processes.add(reference["path"])
        products[ordinary["application"]["path"]] = ordinary["application"]
        assert observe(ROOT / ordinary["output"]["path"]) == final
        normalized = dict(chunks(ROOT / ordinary["output"]["path"])[1])
        normalized[b"DATE"] = native_world[b"DATE"]
        worlds.append([native_world, normalized])
        diagnostics.append(operation["rows"])
        configurations.append(dict(configuration=configuration, receipt=identity(path), output=operation["output"],
                                   company=final["company"], chemical_carrier=final["chemical_carrier"],
                                   salt_delivery=final["salt_delivery"], totals=final["totals"], ordinary=identity(static_path)))
    assert len(processes) == 4 and diagnostics[0] == diagnostics[1]
    for first, second in zip(*worlds, strict=True):
        assert first.keys() == second.keys() and len(first) == 121
        assert all(first[key] == second[key] for key in first if key != b"GLOG")
    result = dict(_documentation=__doc__, complete=True, chemical_renewed=True, chemicals_collected=False, fuel_procured=False,
                  native_processes=4, cross_build_gameplay_comparisons=240, ordinary_static_reloads=2,
                  foundation=identity(FOUNDATION), historical_versions=identity(VERSIONS), configurations=configurations, source=identity(Path(__file__)))
    if retain:
        directory = ROOT / "test-cache/surface-entry/chemical-renewal-products"
        directory.mkdir(parents=True, exist_ok=False)
        changed = {row["source"] for row in json.loads(VERSIONS.read_text(encoding="utf-8"))["products"]}
        copies = [product for product in previous["products"] if product["source"]["path"] not in changed]
        existing = {product["source"]["path"] for product in copies}
        for path in (Path(__file__), VERSIONS):
            products[identity(path)["path"]] = identity(path)
        for index, reference in enumerate(products.values()):
            if reference["path"] in existing:
                continue
            source = ROOT / reference["path"]
            target = directory / f"{index:03d}-{source.parent.name}-{source.name}"
            shutil.copyfile(source, target)
            assert identity(target)["sha256"] == reference["sha256"]
            copies.append(dict(source=reference, retained=identity(target)))
        manifest = directory / "retained.json"
        manifest.write_text(json.dumps(dict(_documentation=__doc__, predecessor=foundation["products"], products=copies), indent=2) + "\n", encoding="utf-8")
        result.update(products=identity(manifest), retained_products=len(copies))
    return result


def main():
    """Publish one immutable actual renewal, ordinary static proof or current binding."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--phase", choices=("renew", "restore", "bind"), required=True)
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
        value = renew(args.configuration, ROOT / args.receipt, args.recover_run) if args.phase == "renew" else restore(args.configuration, ROOT / args.receipt)
    output.write_text(json.dumps(value, indent=2) + "\n", encoding="utf-8")
    print(output.relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
