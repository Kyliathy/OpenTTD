"""Bind actual paid powered Salt geography and preserve current native products.

@file surface_entry_salt_window_checks-2.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CIC.1.
Conversion updates existing rail type only. It creates no floor, train, Salt,
chemical, fuel, company cash or completed original logistics obligation.
"""
import argparse
import json
from pathlib import Path
import shutil

from space_preview_checks import ROOT, chunks, identity
from surface_gateway_component_delivery_ledger import observe_retained as observe

FOUNDATION = ROOT / "logs/surface-entry-staged-admission-verification.json"


def verify(reference):
    """@param reference Exact current or retained product. Refuse missing or changed bytes."""
    assert identity(ROOT / reference["path"]) == reference


def bind(retain):
    """Bind matching native original route payments and optional exact product retention.

    @param retain Whether to preserve newly tested current code and executables.
    @return Both-build actual663payment, powered-route proof and unchanged original materials.
    """
    foundation = json.loads(FOUNDATION.read_text(encoding="utf-8"))
    assert foundation["complete"] and foundation["first_window_funded"]
    verify(foundation["products"])
    previous = json.loads((ROOT / foundation["products"]["path"]).read_text(encoding="utf-8"))
    replaced = {"src/tests/surface_entry_resupply.cpp"}
    replaced.update(f"build/{configuration}/openttd_surface_supply_test.exe" for configuration in ("RelWithDebInfo", "Debug"))
    for product in previous["products"]:
        verify(product["retained"])
        if product["source"]["path"] not in replaced:
            verify(product["source"])
    worlds, rows, configurations, products, processes = [], [], [], {}, set()
    # Each actual original quote supplies exactly one paid conversion;
    # another configuration or ordinary-copy world supplies no capital.
    for entry in foundation["configurations"]:
        configuration = entry["configuration"]
        path = ROOT / f"logs/surface-entry-salt-conversion-{configuration}.json"
        receipt = json.loads(path.read_text(encoding="utf-8"))
        assert receipt["complete"] and receipt["configuration"] == configuration
        operation = receipt["operation"]
        assert operation["receipt"] == entry["receipt"] and operation["source"] == entry["output"]
        verify(entry["receipt"])
        for field in ("source", "before", "output", "application", "terminal", "retained_harness"):
            verify(operation[field])
        for reference in operation["sources"]:
            verify(reference)
            products[reference["path"]] = reference
        products[operation["application"]["path"]] = operation["application"]
        for reference in operation["native_processes"]:
            verify(reference)
            report = json.loads((ROOT / reference["path"]).read_text(encoding="utf-8"))
            assert report["exit_status"] == 0 and report["inputs"] == [entry["output"]]
            assert report["executable"] == operation["application"]
            processes.add(reference["path"])
        initial, final = operation["initial"], operation["final"]
        assert final["company"] == {"cash": 137004, "debt": 5000000}
        assert initial["company"]["cash"] - final["company"]["cash"] == 663
        assert json.loads(json.dumps(observe(ROOT / operation["output"]["path"]))) == final
        converted, = operation["rows"]["converted"]
        assert converted["tiles"] == 13 and converted["cost"] == 663 and converted["earned"] == 0
        assert converted["powered_route"] and converted["connected_portal"]
        worlds.append(chunks(ROOT / operation["output"]["path"])[1])
        rows.append(operation["rows"])
        configurations.append(dict(configuration=configuration, receipt=identity(path), output=operation["output"], company=final["company"],
                                   totals=final["totals"], service_lives=final["service_lives"], converted=converted))
    assert len(processes) == 2 and rows[0] == rows[1]
    assert worlds[0].keys() == worlds[1].keys()
    assert all(worlds[0][key] == worlds[1][key] for key in worlds[0] if key != b"GLOG")
    result = dict(_documentation=__doc__, complete=True, conversion_paid=True, salt_train_owned=False,
                  salt_trip_proved=False, fuel_procured=False, foundation=identity(FOUNDATION), configurations=configurations,
                  native_processes=2, cross_build_gameplay_comparisons=120, source=identity(Path(__file__)))
    if retain:
        retained = ROOT / "test-cache/surface-entry/salt-conversion-products"
        assert not retained.exists()
        retained.mkdir(parents=True)
        copies = [product for product in previous["products"] if product["source"]["path"] not in replaced]
        existing = {product["source"]["path"] for product in copies}
        for index in (1, 2):
            reference = identity(Path(__file__).with_name(f"surface_entry_salt_window_checks-{index}.py"))
            products[reference["path"]] = reference
        # Retain each current authority once while preserving every dated
        # predecessor product at its already accepted immutable location.
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
    """Publish the paid route boundary while leaving unpurchased logistics unchecked."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--retain", action="store_true")
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    output = ROOT / args.output
    assert output.resolve().is_relative_to(ROOT / "logs") and not output.exists()
    output.write_text(json.dumps(bind(args.retain), indent=2) + "\n", encoding="utf-8")
    print(output.relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
