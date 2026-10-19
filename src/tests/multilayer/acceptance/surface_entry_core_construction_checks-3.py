"""Verify ordinary genuine operational Core continuation and retain tested products.

@file surface_entry_core_construction_checks-3.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CD.
The actual paid frame/consumed inputs/shared panel remain original owners;
ordinary service cannot create Dock, Assembly, new hulls or material stock.
"""
import argparse
import json
from pathlib import Path
import runpy
import shutil

from space_preview_checks import ROOT, identity
from surface_gateway_construction_ledger import observe
from surface_gateway_replay import load, replay, validate

DIRECTORY = Path(__file__).resolve().parent
DRIVER = runpy.run_path(str(DIRECTORY / "surface_entry_core_construction_checks-1.py"))
BIND = runpy.run_path(str(DIRECTORY / "surface_entry_core_construction_checks-2.py"))
STEM = "surface-entry-core-construction"
FOUNDATION = ROOT / f"logs/{STEM}-built-verification.json"
PRODUCTS = ROOT / "test-cache/surface-entry/core-resupply-products/retained.json"


def final():
    """@return Genuine operational first Core, ordinary exact replay/shared panel production and current exact tested products."""
    foundation = load(FOUNDATION); current = BIND["bind"]("built")
    assert json.loads(json.dumps(current["configurations"])) == foundation["configurations"] and current["native_processes"] == 4
    summaries, worlds, rows, processes, applications = [], [], [], set(), {}
    # Both builds restore/resume actual completed work and activation, retaining
    # original once-only material consumption and real native panel production.
    # No copied world contributes a second site, balance, packet or watt source.
    for entry in foundation["configurations"]:
        configuration = entry["configuration"]; path = ROOT / f"logs/{STEM}-reload-{configuration}.json"; value = load(path)
        selected, stages, comparable, native, apps = validate(value, entry, observe)
        snapshot = observe(ROOT / selected["output"]["path"])
        assert snapshot["operational"] and snapshot["core"]["row"]["consumed"] == [16, 24]
        assert snapshot["panel"]["a"] == 30000 and snapshot["panel"]["lifetime_generated"] > 0 and snapshot["panel"]["lifetime_consumed"] == 0
        assert min(row["remaining"] for row in snapshot["service_lives"]) > 732
        company = snapshot["company"]
        summaries.append(dict(configuration=configuration, receipt=value["continuations"][0], output=value["output"], company=company,
                              spendable=company["cash"] + 5000000 - company["debt"], core=snapshot["core"], space_orders=snapshot["space_orders"],
                              totals=snapshot["totals"], embodied_inputs=snapshot["embodied_inputs"], orbital_consumed=snapshot["orbital_consumed"],
                              paid_craft=snapshot["paid_craft"], panel=snapshot["panel"], operational=True, manifests=snapshot["manifests"], flight=snapshot["flight"],
                              service_lives=snapshot["service_lives"], original_wagons=32, additional_continuation_ticks=3000, reload_receipt=identity(path)))
        worlds.append(stages); rows.append(comparable); processes.update(native); applications.update(apps)
    assert rows[0] == rows[1] and len(processes) == 6 and len(applications) == 2
    for first, second in zip(*worlds, strict=True):
        assert first.keys() == second.keys() and len(first) == 121
        assert all(first[key] == second[key] for key in first if key != b"GLOG")
    previous = json.loads(PRODUCTS.read_text(encoding="utf-8")); copies = []; replaced = set(applications) | BIND["CHANGED"]
    for product in previous["products"]:
        assert identity(ROOT / product["retained"]["path"]) == product["retained"]
        if product["source"]["path"] not in replaced:
            assert identity(ROOT / product["source"]["path"]) == product["source"]; copies.append(product)
    retained = ROOT / "test-cache/surface-entry/core-construction-products"; assert not retained.exists(); retained.mkdir(parents=True)
    sources = [ROOT / path for path in applications] + [ROOT / path for path in sorted(BIND["CHANGED"])]
    sources += [DIRECTORY / "surface_gateway_construction_ledger.py"]
    sources += [DIRECTORY / f"surface_entry_core_construction_checks-{index}.py" for index in (1, 2, 3)]
    # Only declared newly tested replacements and construction-specific helpers
    # are copied. Earlier source/copy pairs stay exact and remain available.
    for index, source in enumerate(sources):
        target = retained / f"{index:02d}-{source.parent.name}-{source.name}"; shutil.copyfile(source, target)
        assert identity(source)["sha256"] == identity(target)["sha256"]; copies.append(dict(source=identity(source), retained=identity(target)))
    manifest = retained / "retained.json"
    manifest.write_text(json.dumps(dict(_documentation=__doc__, products=copies, predecessor=identity(PRODUCTS)), indent=2) + "\n", encoding="utf-8")
    return dict(_documentation=__doc__, complete=True, original_operational_core=True, core_constructed=True, foundation=identity(FOUNDATION),
                configurations=summaries, native_processes=current["native_processes"] + len(processes), products=identity(manifest),
                cross_build_gameplay_comparisons=360, exact_replay_gameplay_comparisons=240, source=identity(Path(__file__)))


def main():
    """Publish actual ordinary operational-Core replay or the complete current both-build retained handoff."""
    parser = argparse.ArgumentParser(description=__doc__); parser.add_argument("--kind", choices=("reload", "final"), required=True)
    parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug")); parser.add_argument("--output", type=Path, required=True); args = parser.parse_args()
    output = ROOT / args.output; assert output.resolve().is_relative_to(ROOT / "logs") and not output.exists()
    if args.kind == "reload":
        predecessor = ROOT / f"logs/{STEM}-build-{args.configuration}.json"
        value = replay(args.configuration, predecessor, STEM, "construction-service", DRIVER["execute"], observe)
    else:
        value = final()
    output.write_text(json.dumps(value, indent=2) + "\n", encoding="utf-8"); print(output.relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
