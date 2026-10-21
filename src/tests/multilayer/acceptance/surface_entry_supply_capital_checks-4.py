"""Bind conditional first-stage cash while retaining every deferred paid gate.

@file surface_entry_supply_capital_checks-4.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CIB.2.
An admitted short work cap does not prove Salt travel, asset resale or complete
fuel funding. Every later paid step still requires actual native cash admission.
"""
import argparse
import json
from pathlib import Path
import shutil

from space_preview_checks import ROOT, chunks, identity
from surface_gateway_component_delivery_ledger import observe_retained as observe

FOUNDATION = ROOT / "logs/surface-entry-supply-capital-partial-verification.json"


def verify(reference):
    """@param reference Exact current or retained product identity. Refuse changed evidence."""
    assert identity(ROOT / reference["path"]) == reference


def bind(retain):
    """Compare independently quoted first-stage liquidity and optional exact products.

    @param retain Whether to retain current tested authorities for actual paid route work.
    @return Both unchanged original worlds and explicitly conditional first-window funding.
    """
    foundation = json.loads(FOUNDATION.read_text(encoding="utf-8"))
    assert foundation["complete"] and not foundation["savings_complete"]
    verify(foundation["products"])
    previous = json.loads((ROOT / foundation["products"]["path"]).read_text(encoding="utf-8"))
    replaced = {"src/tests/surface_entry_resupply.cpp"}
    replaced.update(f"build/{configuration}/openttd_surface_supply_test.exe" for configuration in ("RelWithDebInfo", "Debug"))
    # Only the explicitly rebuilt native producer/executables supersede old
    # current bytes; all predecessor copies remain immutable historical proof.
    for product in previous["products"]:
        verify(product["retained"])
        if product["source"]["path"] not in replaced:
            verify(product["source"])
    configurations, worlds, rows, processes, products = [], [], [], set(), {}
    # The selected actual original savings world, never its independently
    # replayed copy, supplies every price, age, cash and packet observation.
    for entry in foundation["configurations"]:
        configuration = entry["configuration"]
        path = ROOT / f"logs/surface-entry-staged-admission-{configuration}.json"
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
        first, last = [chunks(ROOT / operation[field]["path"])[1] for field in ("before", "output")]
        assert first == last and len(first) == 121
        assert json.loads(json.dumps(observe(ROOT / operation["output"]["path"]))) == operation["initial"] == operation["final"]
        assert operation["final"]["company"] == entry["company"]
        peak, = operation["rows"]["peak"]
        assert peak["funded"] and not peak["monthly_boundary"] and peak["refund_counted"] == 0
        assert peak["cash"] == 137667 and peak["reserve"] == 100000 and peak["ticks"] == 500
        configurations.append(dict(configuration=configuration, receipt=identity(path), output=operation["output"],
                                   company=entry["company"], peak=peak, service_lives=entry["service_lives"], totals=entry["totals"]))
        worlds.append(last)
        rows.append(operation["rows"])
    assert len(processes) == 2 and rows[0] == rows[1]
    assert worlds[0].keys() == worlds[1].keys()
    assert all(worlds[0][key] == worlds[1][key] for key in worlds[0] if key != b"GLOG")
    result = dict(_documentation=__doc__, complete=True, admission_only=True, first_window_funded=True,
                  salt_trip_proved=False, full_gross_fuel_window_funded=False, refund_earned=False,
                  conversion_paid=False, fuel_procured=False, foundation=identity(FOUNDATION), configurations=configurations,
                  native_processes=2, cross_build_gameplay_comparisons=120, source=identity(Path(__file__)))
    if retain:
        retained = ROOT / "test-cache/surface-entry/staged-admission-products"
        assert not retained.exists()
        retained.mkdir(parents=True)
        copies = [product for product in previous["products"] if product["source"]["path"] not in replaced]
        existing = {product["source"]["path"] for product in copies}
        for index in (3, 4):
            reference = identity(Path(__file__).with_name(f"surface_entry_supply_capital_checks-{index}.py"))
            products[reference["path"]] = reference
        # Carry forward exact immutable copies and retain each new tested
        # current authority once, preserving dated executables separately.
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
    """Publish conditional admission without overwriting any genuine work or proof."""
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
