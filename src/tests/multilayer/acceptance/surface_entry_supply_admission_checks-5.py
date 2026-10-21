"""Bind fresh current admission without replacing historical native evidence.

@file surface_entry_supply_admission_checks-5.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CH.3.
Exact process, executable, source and unchanged-world identities are required.
Quoted prices and routes supply no cargo, coupling, paid renewal or capital.
"""
import argparse
import json
from pathlib import Path
import shutil

from space_preview_checks import ROOT, chunks, identity
from surface_gateway_component_delivery_ledger import observe_retained as observe

DIRECTORY = Path(__file__).resolve().parent
FOUNDATION = ROOT / "logs/surface-entry-component-delivery-reload-verification.json"
MEMBERSHIP = ROOT / "logs/surface-entry-supply-admission-membership-verification.json"
CONFIGURATIONS = ("RelWithDebInfo", "Debug")


def verify(reference):
    """Require an unchanged exact product at its original location.

    @param reference Hash-qualified current source, save, application or process artifact.
    @return Nothing; a missing or changed product refuses this new boundary.
    """
    assert identity(ROOT / reference["path"]) == reference


def bind(prefix, retain):
    """Compare independent pure admission worlds and optionally retain current authorities.

    @param prefix Fresh receipt basename used by this explicitly selected query set.
    @param retain Whether to preserve each current tested source and executable once.
    @return Exact both-build findings with unfulfilled paid prerequisites kept explicit.
    """
    foundation = json.loads(FOUNDATION.read_text(encoding="utf-8"))
    membership = json.loads(MEMBERSHIP.read_text(encoding="utf-8"))
    assert foundation["complete"] and membership["foundation"] == identity(FOUNDATION)
    verify(foundation["products"])
    previous = json.loads((ROOT / foundation["products"]["path"]).read_text(encoding="utf-8"))
    replaced = {"src/tests/surface_entry_resupply.cpp"}
    replaced.update(f"build/{configuration}/openttd_surface_supply_test.exe" for configuration in CONFIGURATIONS)
    # Predecessor copies remain exact historical authorities. Current sources
    # must also match unless this explicitly named native implementation or its
    # independently rebuilt executables were superseded by this new query set.
    for product in previous["products"]:
        verify(product["retained"])
        if product["source"]["path"] not in replaced:
            verify(product["source"])
    summaries, worlds, diagnostics, processes, products = [], [], [], set(), {}
    # Each configuration starts from its selected original continuation and
    # chains modules before fuel admission. No copied or fixture-funded world
    # can substitute for the genuine material and financial lineage.
    for entry, member in zip(foundation["configurations"], membership["configurations"], strict=True):
        configuration = entry["configuration"]
        verify(entry["receipt"])
        verify(entry["output"])
        predecessor = entry["receipt"]
        stages, rows = [], []
        # Every pure stage must preserve all121native families, actual packet
        # provenance and finance, in addition to passing its process assertions.
        for boundary in ("modules", "refill"):
            path = ROOT / f"logs/{prefix}-{boundary}-{configuration}.json"
            receipt = json.loads(path.read_text(encoding="utf-8"))
            assert receipt["complete"] and receipt["configuration"] == configuration
            current = receipt["operation"]
            assert current["receipt"] == predecessor
            assert current["source"] == json.loads((ROOT / predecessor["path"]).read_text(encoding="utf-8"))["operation"]["output"]
            for key in ("source", "before", "output", "application", "terminal", "retained_harness"):
                verify(current[key])
            for reference in current["sources"]:
                verify(reference)
                products[reference["path"]] = reference
            products[current["application"]["path"]] = current["application"]
            for reference in current["native_processes"]:
                verify(reference)
                report = json.loads((ROOT / reference["path"]).read_text(encoding="utf-8"))
                assert report["exit_status"] == 0 and report["inputs"] == [current["source"]]
                assert report["executable"] == current["application"]
                processes.add(reference["path"])
            first, last = [chunks(ROOT / current[key]["path"])[1] for key in ("before", "output")]
            assert first == last and len(first) == 121
            assert json.loads(json.dumps(observe(ROOT / current["output"]["path"]))) == current["final"] == current["initial"]
            stages.append(last)
            rows.append(current["rows"])
            predecessor = identity(path)
        assert current["final"]["company"] == member["company"] == {"cash": 143673, "debt": 5000000}
        worlds.append(stages)
        diagnostics.append(rows)
        summaries.append(dict(configuration=configuration, receipt=predecessor, output=current["output"],
                              company=current["final"]["company"], totals=current["final"]["totals"],
                              cargo_abandoned=current["final"]["cargo_abandoned"], service_lives=current["final"]["service_lives"],
                              modules=rows[0], refill=rows[1]))
    assert len(processes) == 4 and diagnostics[0] == diagnostics[1]
    # Independent builds may record different game-version metadata only;
    # every gameplay family must agree at both unchanged admission boundaries.
    for first, second in zip(*worlds, strict=True):
        assert first.keys() == second.keys()
        assert all(first[key] == second[key] for key in first if key != b"GLOG")
    result = dict(_documentation=__doc__, complete=True, admission_only=True, fuel_procured=False,
                  funded_expansion=False, replacement_owned=False, full_logistics_bill_complete=False,
                  foundation=identity(FOUNDATION), membership=identity(MEMBERSHIP), configurations=summaries,
                  native_processes=4, cross_build_gameplay_comparisons=240, source=identity(Path(__file__)))
    if retain:
        retained = ROOT / "test-cache/surface-entry/supply-admission-products"
        assert not retained.exists()
        retained.mkdir(parents=True)
        copies = [product for product in previous["products"] if product["source"]["path"] not in replaced]
        existing = {product["source"]["path"] for product in copies}
        for index in range(1, 6):
            reference = identity(DIRECTORY / f"surface_entry_supply_admission_checks-{index}.py")
            products[reference["path"]] = reference
        # Retain each new current authority exactly once; predecessor copies
        # preserve their dated bytes and never become current executable proof.
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
    """Publish a fresh selected admission set, refusing overwritten reports or retained products."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--receipt-prefix", required=True)
    parser.add_argument("--retain", action="store_true")
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    assert Path(args.receipt_prefix).name == args.receipt_prefix
    output = ROOT / args.output
    assert output.resolve().is_relative_to(ROOT / "logs") and not output.exists()
    output.write_text(json.dumps(bind(args.receipt_prefix, args.retain), indent=2) + "\n", encoding="utf-8")
    print(output.relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
