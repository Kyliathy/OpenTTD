"""Bind actual full-Salt return through independently priced short intervals.

@file surface_entry_salt_window_checks-6.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CID.3.
Historical native versions remain exact. Current original60Salt depot custody
is proved separately from reserved loading, future sales and chemical delivery.
"""
import argparse
import json
from pathlib import Path
import runpy
import shutil

from space_preview_checks import ROOT, chunks, identity
from surface_gateway_component_delivery_ledger import observe_salt_window as observe

DIRECTORY = Path(__file__).resolve().parent
PREVIOUS = runpy.run_path(str(DIRECTORY / "surface_entry_salt_window_checks-4.py"))
FOUNDATION = ROOT / "logs/surface-entry-salt-window-verification.json"
VERSIONS = ROOT / "test-cache/surface-entry/salt-window-source-versions/before-adaptive-return/retained.json"


def verify(reference):
    """Verify an exact current product or its one explicitly preserved original version.

    @param reference Native source, process, save or executable identity.
    @return Whether this authority is also the current original source file.
    """
    path = ROOT / reference["path"]
    if path.is_file():
        if identity(path) == reference:
            return True
    versions = json.loads(VERSIONS.read_text(encoding="utf-8"))["products"]
    exact, = [product for product in versions if product["source"] == reference]
    retained = identity(ROOT / exact["retained"]["path"])
    assert retained == exact["retained"] and retained["sha256"] == reference["sha256"]
    return False


def stage(path, predecessor, processes, products):
    """Verify one linked pure admission or actual bounded original return interval.

    @param path Exact existing original receipt.
    @param predecessor Required immediately preceding selected original receipt identity.
    @param processes Accumulated distinct accepted process report paths.
    @param products Current tested source/executable identities to retain.
    @return Original operation and complete output-family map.
    """
    receipt = json.loads(path.read_text(encoding="utf-8"))
    assert receipt["complete"]
    operation = receipt["operation"]
    assert operation["receipt"] == predecessor
    assert operation["source"] == json.loads((ROOT / predecessor["path"]).read_text(encoding="utf-8"))["operation"]["output"]
    for key in ("source", "before", "output", "application", "terminal", "retained_harness"):
        verify(operation[key])
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
    first, last = [chunks(ROOT / operation[key]["path"])[1] for key in ("before", "output")]
    if receipt["phase"] == "staged-return-query":
        assert first == last and operation["initial"] == operation["final"]
    else:
        earned, = operation["rows"]["earned"]
        peak, = operation["rows"]["peak"]
        assert 0 < earned["earned"] <= peak["ticks"] <= 200
        assert operation["final"]["company"]["cash"] == operation["initial"]["company"]["cash"] - earned["net_expense"]
    return operation, last


def bind(retain):
    """Bind all actual return caps, the explicit refusal and ordinary static reloads.

    @param retain Whether to copy exact current authorities for later genuine delivery.
    @return Full actual60Salt depot return, original finance and still-unfulfilled sale/fuel obligations.
    """
    foundation = json.loads(FOUNDATION.read_text(encoding="utf-8"))
    verify(foundation["products"])
    previous = json.loads((ROOT / foundation["products"]["path"]).read_text(encoding="utf-8"))
    replaced = {"src/tests/surface_entry_resupply.cpp"}
    replaced.update(f"build/{configuration}/openttd_surface_supply_test.exe" for configuration in ("RelWithDebInfo", "Debug"))
    for product in previous["products"]:
        verify(product["retained"])
        if product["source"]["path"] not in replaced:
            verify(product["source"])
    worlds, diagnostics, configurations, processes, products = [], [], [], set(), {}
    # Each new query prices the actual previous original output. Declined
    # intervals advance nothing; smaller later caps retain all already earned
    # loading and movement rather than replaying an earlier paid state.
    for entry in foundation["configurations"]:
        configuration = entry["configuration"]
        predecessor = entry["receipt"]
        names = ("return-current-query", "return", "return-query-03", "return-02",
                 "return-query-04", "return-03", "return-query-05", "return-04")
        stages, rows, ticks = [], [], 0
        first_return = None
        for name in names:
            path = ROOT / f"logs/surface-entry-salt-{name}-{configuration}.json"
            operation, output = stage(path, predecessor, processes, products)
            if name == "return":
                first_return = identity(path)
            if operation["rows"]["earned"]:
                earned, = operation["rows"]["earned"]
                ticks += earned["earned"]
            predecessor = identity(path)
            stages.append(output)
            rows.append(operation["rows"])
        assert ticks == 602
        refusal = ROOT / f"logs/surface-entry-salt-return-query-02-{configuration}.json"
        refused, refused_world = stage(refusal, first_return, processes, products)
        refused_peak, = refused["rows"]["peak"]
        assert not refused_peak["funded"] and refused_peak["shortfall"] == 7
        stages.append(refused_world)
        rows.append(refused["rows"])
        current = operation["final"]
        assert current["company"] == {"cash": 100397, "debt": 5000000}
        assert current["salt_consist"]["home"] and current["salt_consist"]["held"] == 60
        progress, = operation["rows"]["progress"]
        assert progress["stored"] == 60 and progress["reserved"] == 0 and progress["collected"]
        restored_path = ROOT / f"logs/surface-entry-salt-return-static-{configuration}.json"
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
            PREVIOUS["verify_ordinary_application"](report["executable"], ordinary["application"])
            products[report["executable"]["path"]] = report["executable"]
            processes.add(reference["path"])
        products[ordinary["application"]["path"]] = ordinary["application"]
        assert json.loads(json.dumps(observe(ROOT / ordinary["output"]["path"]))) == current
        normalized = dict(chunks(ROOT / ordinary["output"]["path"])[1])
        normalized[b"DATE"] = output[b"DATE"]
        stages.append(normalized)
        worlds.append(stages)
        diagnostics.append(rows)
        configurations.append(dict(configuration=configuration, receipt=predecessor, output=operation["output"],
                                   company=current["company"], salt_consist=current["salt_consist"], service_lives=current["service_lives"],
                                   totals=current["totals"], additional_earned_ticks=ticks, ordinary=identity(restored_path)))
    assert len(processes) == 20 and diagnostics[0] == diagnostics[1]
    comparisons = 0
    for first, second in zip(*worlds, strict=True):
        assert first.keys() == second.keys() and len(first) == 121
        assert all(first[key] == second[key] for key in first if key != b"GLOG")
        comparisons += len(first) - 1
    result = dict(_documentation=__doc__, complete=True, salt_return_complete=True, salt_delivered_to_chemicals=False,
                  refund_earned=False, chemical_renewed=False, fuel_procured=False, foundation=identity(FOUNDATION),
                  historical_versions=identity(VERSIONS), configurations=configurations, native_processes=20,
                  cross_build_gameplay_comparisons=comparisons, ordinary_static_reloads=2, source=identity(Path(__file__)))
    if retain:
        retained = ROOT / "test-cache/surface-entry/salt-return-products"
        assert not retained.exists()
        retained.mkdir(parents=True)
        copies = [product for product in previous["products"] if product["source"]["path"] not in replaced]
        existing = {product["source"]["path"] for product in copies}
        for path in (Path(__file__), VERSIONS):
            reference = identity(path)
            products[reference["path"]] = reference
        # Retain only current tested authorities here. Earlier versions remain
        # at their exact explicitly checked immutable predecessor locations.
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
    """Publish exact full-Salt return without claiming empty sale, chemicals or fuel."""
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
