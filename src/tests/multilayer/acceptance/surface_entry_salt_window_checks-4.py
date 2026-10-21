"""Verify ordinary static reload and bind the first paid capped Salt window.

@file surface_entry_salt_window_checks-4.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CIC.4.
Static copies prove loader ownership without earning extra ticks. Only the
selected original holds the actual new finite Salt and supplies later work.
"""
import argparse
import json
from pathlib import Path
import shutil

from space_preview_checks import ROOT, chunks, identity
from surface_gateway_component_delivery_ledger import observe_salt_window as observe
from surface_gateway_replay import RESTORE

FOUNDATION = ROOT / "logs/surface-entry-salt-conversion-verification.json"


def verify(reference):
    """@param reference Hash-qualified current or retained artifact. Refuse changed evidence."""
    assert identity(ROOT / reference["path"]) == reference


def verify_ordinary_application(actual, application):
    """Verify the existing Windows console projection against every original game byte.

    @param actual Executable identity recorded by the actual ordinary native process.
    @param application Source GUI application named by the existing ordinary wrapper.
    @return Nothing; only runner.py's exact PE subsystem projection is admitted.
    """
    verify(actual)
    verify(application)
    if actual == application:
        return
    source = ROOT / application["path"]
    launched = ROOT / actual["path"]
    encoded = bytearray(source.read_bytes())
    header = int.from_bytes(encoded[60:64], "little")
    assert encoded[header:header + 4] == b"PE\0\0"
    assert int.from_bytes(encoded[header + 92:header + 94], "little") in (2, 3)
    # runner.console_executable changes only the Windows subsystem field so
    # the headless ordinary game can emit logs. Every gameplay byte and all
    # matching runtime DLL bytes must still equal the genuine original build.
    encoded[header + 92:header + 94] = (3).to_bytes(2, "little")
    assert launched.read_bytes() == encoded
    original = {path.name: identity(path)["sha256"] for path in source.parent.glob("*.dll")}
    assert {path.name: identity(path)["sha256"] for path in launched.parent.glob("*.dll")} == original


def restore(configuration, receipt):
    """Load/resave actual paid Salt custody through the ordinary game without extra work.

    @param configuration Independently linked ordinary game application.
    @param receipt Exact selected original500tick purchase/work output.
    @return Native ordinary process, exact original custody and unchanged gameplay families.
    """
    predecessor = json.loads(receipt.read_text(encoding="utf-8"))
    assert predecessor["complete"] and predecessor["configuration"] == configuration
    original = predecessor["operation"]["output"]
    source = ROOT / original["path"]
    verify(original)
    restored, operation = RESTORE["ORDINARY"](configuration, source, "salt-window-static-" + configuration.lower())
    RESTORE["REVISION"](source, restored)
    pause = RESTORE["restore_pause"](source, restored)
    first, last = [chunks(path)[1] for path in (source, restored)]
    assert first.keys() == last.keys() and len(first) == 121
    assert all(first[key] == last[key] for key in first if key not in (b"GLOG", b"DATE"))
    assert observe(source) == observe(restored)
    return dict(_documentation=__doc__, complete=True, configuration=configuration, phase="restore",
                receipt=identity(receipt), operation=operation, original_pause=pause, source=identity(Path(__file__)))


def native(path, predecessor, processes, products):
    """Verify one genuinely linked paid original native stage.

    @param path Exact existing purchase or work receipt.
    @param predecessor Expected immediately preceding original receipt identity.
    @param processes Accumulated distinct native process report paths.
    @param products Current tested source/executable identities to preserve.
    @return Verified operation and full gameplay-family map.
    """
    receipt = json.loads(path.read_text(encoding="utf-8"))
    assert receipt["complete"]
    operation = receipt["operation"]
    assert operation["receipt"] == predecessor
    for field in ("source", "before", "output", "application", "terminal", "retained_harness"):
        verify(operation[field])
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
    return operation, chunks(ROOT / operation["output"]["path"])[1]


def bind(retain):
    """Bind independent purchase/work/static reload with exact current products.

    @param retain Whether to copy new current source/executable authorities.
    @return Actual partial finite-Salt custody, original selected worlds and remaining return obligation.
    """
    foundation = json.loads(FOUNDATION.read_text(encoding="utf-8"))
    verify(foundation["products"])
    previous = json.loads((ROOT / foundation["products"]["path"]).read_text(encoding="utf-8"))
    replaced = {"src/tests/surface_entry_resupply.cpp", "src/tests/multilayer/acceptance/surface_gateway_component_delivery_ledger.py"}
    replaced.update(f"build/{configuration}/openttd_surface_supply_test.exe" for configuration in ("RelWithDebInfo", "Debug"))
    for product in previous["products"]:
        verify(product["retained"])
        if product["source"]["path"] not in replaced:
            verify(product["source"])
    worlds, diagnostics, configurations, products, processes = [], [], [], {}, set()
    # Every real purchase precedes its one capped work interval. Restored
    # copies prove reconstruction only and never add Salt or cash to originals.
    for entry in foundation["configurations"]:
        configuration = entry["configuration"]
        predecessor = entry["receipt"]
        stages, rows = [], []
        for name in ("purchase", "work-01"):
            path = ROOT / f"logs/surface-entry-salt-{name}-{configuration}.json"
            operation, output = native(path, predecessor, processes, products)
            assert operation["source"] == json.loads((ROOT / predecessor["path"]).read_text(encoding="utf-8"))["operation"]["output"]
            predecessor = identity(path)
            stages.append(output)
            rows.append(operation["rows"])
        current = operation["final"]
        progress, = operation["rows"]["progress"]
        assert progress["earned"] == 500 and current["salt_consist"]["held"] == 60 and not current["salt_consist"]["home"]
        assert current["company"] == {"cash": 100864, "debt": 5000000}
        restored_path = ROOT / f"logs/surface-entry-salt-static-{configuration}.json"
        restored = json.loads(restored_path.read_text(encoding="utf-8"))
        assert restored["complete"] and restored["receipt"] == predecessor
        ordinary = restored["operation"]
        assert ordinary["source"] == operation["output"]
        for field in ("source", "output", "application", "terminal"):
            verify(ordinary[field])
        for reference in ordinary["native_processes"]:
            verify(reference)
            report = json.loads((ROOT / reference["path"]).read_text(encoding="utf-8"))
            assert report["exit_status"] == 0 and report["inputs"] == [operation["output"]]
            verify_ordinary_application(report["executable"], ordinary["application"])
            products[report["executable"]["path"]] = report["executable"]
            processes.add(reference["path"])
        products[ordinary["application"]["path"]] = ordinary["application"]
        authority = identity(ROOT / "src/tests/multilayer/support/runner.py")
        products[authority["path"]] = authority
        assert json.loads(json.dumps(observe(ROOT / ordinary["output"]["path"]))) == current
        normalized = dict(chunks(ROOT / ordinary["output"]["path"])[1])
        normalized[b"DATE"] = output[b"DATE"]
        stages.append(normalized)
        worlds.append(stages)
        diagnostics.append(rows)
        configurations.append(dict(configuration=configuration, receipt=predecessor, output=operation["output"],
                                   ordinary=identity(restored_path), company=current["company"], salt_consist=current["salt_consist"],
                                   service_lives=current["service_lives"], totals=current["totals"], earned_ticks=500))
    assert len(processes) == 6 and diagnostics[0] == diagnostics[1]
    for first, second in zip(*worlds, strict=True):
        assert first.keys() == second.keys() and len(first) == 121
        assert all(first[key] == second[key] for key in first if key != b"GLOG")
    result = dict(_documentation=__doc__, complete=True, first_window_complete=True, salt_return_complete=False,
                  salt_collected=60, salt_source=14, refund_earned=False, chemical_renewed=False, fuel_procured=False,
                  foundation=identity(FOUNDATION), configurations=configurations, native_processes=6,
                  cross_build_gameplay_comparisons=360, ordinary_static_reloads=2, source=identity(Path(__file__)))
    if retain:
        retained = ROOT / "test-cache/surface-entry/salt-window-products"
        assert not retained.exists()
        retained.mkdir(parents=True)
        copies = [product for product in previous["products"] if product["source"]["path"] not in replaced]
        existing = {product["source"]["path"] for product in copies}
        for index in (3, 4):
            reference = identity(Path(__file__).with_name(f"surface_entry_salt_window_checks-{index}.py"))
            products[reference["path"]] = reference
        # Keep all exact dated copies while retaining each new current code
        # and independently tested application once for subsequent return work.
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
    """Publish one ordinary static reload or the bound capped Salt window."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--mode", choices=("restore", "bind"), required=True)
    parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"))
    parser.add_argument("--receipt", type=Path)
    parser.add_argument("--retain", action="store_true")
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    output = ROOT / args.output
    assert output.resolve().is_relative_to(ROOT / "logs") and not output.exists()
    if args.mode == "restore":
        assert args.configuration is not None and args.receipt is not None and not args.retain
        result = restore(args.configuration, ROOT / args.receipt)
    else:
        result = bind(args.retain)
    output.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(output.relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
