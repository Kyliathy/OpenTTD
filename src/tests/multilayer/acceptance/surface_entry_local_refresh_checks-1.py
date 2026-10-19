"""Renew the current due local buses without borrowing an earlier cycle's orders.

@file surface_entry_local_refresh_checks-1.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15BIEQI.
Native commands own all payments, passenger delivery, empty arrival and renewal.
Only selected original annual service contributes capital; copies prove equality.
"""
import argparse
import json
from pathlib import Path
import runpy
import shutil

from space_preview_checks import ROOT, chunks, identity
from surface_regional_ledger import TABLE, observe, lives
from surface_regional_replay import replay

DIRECTORY = Path(__file__).resolve().parent
BUS = runpy.run_path(str(DIRECTORY / "surface_entry_bus_checks-1.py"))["execute"]
OLD = runpy.run_path(str(DIRECTORY / "surface_entry_regional_renewal_checks-1.py"))
STEM = "surface-entry-local-refresh"
FOUNDATION = ROOT / "logs/surface-entry-treasury-boundary-39-verification.json"
PRODUCTS = ROOT / "test-cache/surface-entry/treasury-boundary-39-products/retained.json"


def load(path):
    """@param path Existing immutable receipt. @return Its complete accepted object."""
    value = json.loads(path.read_text(encoding="utf-8")); assert value["complete"]
    return value


def receipt(phase, configuration):
    """@param phase Unique current-cycle phase. @param configuration Actual build. @return Immutable receipt destination."""
    return ROOT / f"logs/{STEM}-{phase}-{configuration}.json"


def validate(operation):
    """@param operation Actual native process receipt. @return Independently decoded gameplay families after exact identity checks."""
    for field in ("source", "before", "output", "application", "terminal", "retained_harness"):
        reference = operation[field]; assert identity(ROOT / reference["path"]) == reference
    # Current acceptance requires each actual producer and observer byte, as
    # well as the native process report. Historical-source waivers are absent.
    for reference in operation["sources"] + operation["native_processes"]:
        assert identity(ROOT / reference["path"]) == reference
    result = ROOT / operation["output"]["path"]
    assert json.loads(json.dumps(observe(result))) == operation["final"]
    return chunks(result)[1]


def execute(configuration, phase, previous, recover_run=None):
    """@param configuration Actual independently linked build. @param phase Original local command/work phase.
    @param previous Exact selected predecessor. @param recover_run Successful native run to inspect without repeating work.
    @return Original material/payment proof with eight healthy services and current order restoration.
    """
    operation = BUS(configuration, phase, previous, recover_run, observe_fn=observe)
    before, output = [ROOT / operation[field]["path"] for field in ("before", "output")]
    old, new = [OLD["all_orders"](path) for path in (before, output)]
    assert OLD["healthy"](before) == OLD["healthy"](output)
    local = [name for name in old if name.startswith("Surface Launch passenger ")]; assert len(local) == 3
    if phase != "work":
        # Same-tick local commands preserve every healthy road, rail head and
        # wagon row exactly. Only real work may move useful healthy services.
        excluded = {old[name]["vehicle"] for name in local}
        a, b = TABLE(before, b"VEHS"), TABLE(output, b"VEHS")
        assert all(b[key] == row for key, row in a.items() if key not in excluded)
    if phase == "renew":
        baseline = load(receipt("query", configuration))["operation"]
        original = OLD["all_orders"](ROOT / baseline["output"]["path"])
        # Native CO_COPY preserves actual current timetable fields; restoring
        # loading flags and deleting only the temporary halt resumes commerce.
        for name in local:
            assert old[name]["count"] == 3 and new[name]["count"] == 2
            expected = json.loads(json.dumps(old[name]["orders"]))
            for index in range(2):
                expected[index]["flags"] = original[name]["orders"][index]["flags"]
            assert expected == new[name]["orders"] and old[name]["vehicle"] != new[name]["vehicle"]
        assert all(row["age"] == 0 for row in lives(output) if row["name"] in local)
    wrapper = (ROOT / operation["retained_harness"]["path"]).with_name("local-refresh-driver.py")
    if wrapper.exists():
        assert wrapper.read_bytes() == Path(__file__).read_bytes()
    else:
        wrapper.write_bytes(Path(__file__).read_bytes())
    operation["sources"].append(identity(Path(__file__)))
    operation["sources"].append(identity(DIRECTORY / "surface_entry_regional_renewal_checks-1.py"))
    operation["retained_wrapper"] = identity(wrapper); operation["service_lives"] = lives(output)
    return dict(_documentation=__doc__, complete=True, configuration=configuration, phase=phase, operation=operation)


def bind(kind, work_count):
    """@param kind Pure query, empty custody or actual paid purchase. @param work_count Number of real bounded return intervals.
    @return Exact current-cycle lineage and cross-build gameplay equality before dependent action.
    """
    foundation = load(FOUNDATION); phases = ["query"]
    if kind != "query":
        phases += ["recall"] + [f"work-{index}" for index in range(1, work_count + 1)]
    if kind == "purchase":
        phases.append("renew")
    summaries, worlds, rows, processes = [], [], [], set()
    # Both independent builds must follow the same original paid lineage;
    # copied replay worlds never become funding predecessors.
    for entry in foundation["configurations"]:
        configuration = entry["configuration"]; previous = ROOT / entry["receipt"]["path"]
        stages, comparable, references = [], [], []
        for phase in phases:
            path = receipt(phase, configuration); current = load(path)["operation"]
            assert current["receipt"] == identity(previous) and current["source"] == load(previous)["operation"]["output"]
            assert identity(ROOT / current["retained_wrapper"]["path"]) == current["retained_wrapper"]
            stages.append(validate(current)); comparable.append(current["rows"]); references.append(identity(path))
            processes.update(reference["path"] for reference in current["native_processes"]); previous = path
        if kind == "custody":
            assert all(row["home"] and row["held"] == 0 and row["tile"] == 2984 for row in current["rows"]["custody"])
        company = current["final"]["company"]
        summaries.append(dict(configuration=configuration, receipt=identity(previous), output=current["output"], company=company,
                              global_period=39, spendable=company["cash"] + 5000000 - company["debt"],
                              receipts=references, service_lives=lives(ROOT / current["output"]["path"]), actual_rows=current["rows"]))
        worlds.append(stages); rows.append(comparable)
    assert rows[0] == rows[1]
    for first, second in zip(*worlds, strict=True):
        assert first.keys() == second.keys() and len(first) == 121
        assert all(first[key] == second[key] for key in first if key != b"GLOG")
    return dict(_documentation=__doc__, complete=True, foundation=identity(FOUNDATION), phase=kind, configurations=summaries,
                work_count=work_count, native_processes=len(processes), cross_build_gameplay_comparisons=len(phases) * 120,
                actual_launch_complete=False, source=identity(Path(__file__)))


def final(work_count):
    """@param work_count Actual completed return intervals. @return Paid renewal, useful ordinary annual replay and exact tested products."""
    purchase = bind("purchase", work_count); summaries, worlds, rows, processes = [], [], [], set()
    # Whole ordinary restoration and both annual branches must preserve every
    # owner. Only each first, original branch supplies the selected treasury.
    for entry in purchase["configurations"]:
        configuration = entry["configuration"]; value = load(receipt("reload", configuration))
        assert value["source_receipt"] == entry["receipt"] and value["additional_continuation_ticks"] == 27000
        restore = load(ROOT / value["restore"]["path"])["operation"]
        original, restored = [chunks(ROOT / restore[field]["path"])[1] for field in ("source", "output")]
        normalized = dict(restored); normalized[b"DATE"] = original[b"DATE"]; stages, comparable = [normalized], []
        processes.update(reference["path"] for reference in restore["native_processes"])
        for reference in value["continuations"]:
            assert identity(ROOT / reference["path"]) == reference
            operation = load(ROOT / reference["path"])["operation"]
            stages.append(validate(operation)); comparable.append(operation["rows"])
            processes.update(item["path"] for item in operation["native_processes"])
        assert comparable[0] == comparable[1]
        earned, = comparable[0]["earned"]; assert earned["earned"] == 27000 and earned["net_expense"] < 0
        assert {row["label"] for row in comparable[0]["delivered"] if row["amount"] > 0} == {"PASS", "COAL", "LVST", "GOOD", "GRAI", "IORE", "STEL"}
        service = lives(ROOT / value["output"]["path"]); assert min(row["remaining"] for row in service) > 732
        company = value["company"]
        summaries.append(dict(configuration=configuration, receipt=value["continuations"][0], output=value["output"], company=company,
                              global_period=40, additional_continuation_ticks=27000, service_lives=service, annual_rows=comparable[0],
                              spendable=company["cash"] + 5000000 - company["debt"], reload_receipt=identity(receipt("reload", configuration))))
        worlds.append(stages); rows.append(comparable)
    assert rows[0] == rows[1] and len(processes) == 6
    for first, second in zip(*worlds, strict=True):
        assert first.keys() == second.keys() and len(first) == 121
        assert all(first[key] == second[key] for key in first if key != b"GLOG")
    previous = json.loads(PRODUCTS.read_text(encoding="utf-8")); unique = {}
    # Reuse exact retained products, collapsing only identical duplicate records
    # left by the generic financial boundary's identical self-copy.
    for product in previous["products"]:
        assert identity(ROOT / product["source"]["path"]) == product["source"]
        assert identity(ROOT / product["retained"]["path"]) == product["retained"]
        key = product["source"]["path"]
        if key in unique:
            assert unique[key]["source"] == product["source"]
        else:
            unique[key] = product
    retained = ROOT / "test-cache/surface-entry/local-refresh-products"; assert not retained.exists(); retained.mkdir(parents=True)
    source = Path(__file__); target = retained / source.name; shutil.copyfile(source, target)
    assert identity(source)["sha256"] == identity(target)["sha256"]
    products = list(unique.values()) + [dict(source=identity(source), retained=identity(target))]
    manifest = retained / "retained.json"
    manifest.write_text(json.dumps(dict(_documentation=__doc__, products=products, predecessor=identity(PRODUCTS)), indent=2) + "\n", encoding="utf-8")
    return dict(_documentation=__doc__, complete=True, actual_local_refresh_complete=True,
                actual_capital_complete=all(row["spendable"] >= 1870000 for row in summaries), actual_launch_complete=False,
                foundation=identity(ROOT / "logs/surface-entry-local-refresh-purchase-verification.json"), configurations=summaries,
                native_processes=purchase["native_processes"] + len(processes), products=identity(manifest),
                cross_build_gameplay_comparisons=360, exact_replay_gameplay_comparisons=240, source=identity(source))


def main():
    """Publish one current sequential boundary; successful native work and receipts remain immutable."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--kind", choices=("query", "recall", "work", "renew", "bind-query", "bind-custody", "bind-purchase", "reload", "final"), required=True)
    parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug")); parser.add_argument("--receipt", type=Path)
    parser.add_argument("--recover-run"); parser.add_argument("--work-count", type=int, choices=range(1, 5), default=1)
    parser.add_argument("--output", type=Path, required=True); args = parser.parse_args()
    output = (ROOT / args.output).resolve(); assert output.is_relative_to(ROOT / "logs") and not output.exists()
    if args.kind.startswith("bind-"):
        value = bind(args.kind.removeprefix("bind-"), args.work_count)
    elif args.kind == "reload":
        value = replay(args.configuration, receipt("renew", args.configuration), STEM, phase="earn")
    elif args.kind == "final":
        value = final(args.work_count)
    else:
        value = execute(args.configuration, args.kind, (ROOT / args.receipt).resolve(), args.recover_run)
    output.write_text(json.dumps(value, indent=2) + "\n", encoding="utf-8"); print(output.relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
