"""Bind actual finite Salt, new chemical custody and genuine empty-carrier settlement.

@file surface_entry_resupply_checks-4.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15BIEG.2.
Only exact selected native predecessors contribute stock, work or capital.
Historical pure-query products are verified through explicit retained aliases.
"""
import argparse
import json
from pathlib import Path

from space_preview_checks import ROOT, chunks, identity
from surface_resupply_ledger import observe


def execute():
    """@return Verified both-build original refill lineage, finite conversion, actual books and sole useful custody."""
    foundation_path = ROOT / "logs/surface-entry-resupply-plan-verification.json"
    foundation = json.loads(foundation_path.read_text(encoding="utf-8")); assert foundation["complete"]
    retained_path = ROOT / "test-cache/surface-entry/resupply-plan-products/retained.json"
    retained = json.loads(retained_path.read_text(encoding="utf-8"))
    aliases = {(row["original"]["path"], row["original"]["sha256"]): row["retained"] for row in retained["files"]}
    checked, processes, builds, summaries = set(), set(), [], []

    def verify(reference):
        """@param reference Exact current artifact or explicitly retained original product. Verify once without replaying commands."""
        key = reference["path"], reference["sha256"]
        if key not in checked:
            if identity(ROOT / reference["path"]) != reference:
                alias = aliases[key]; assert alias["sha256"] == reference["sha256"] and identity(ROOT / alias["path"]) == alias
            checked.add(key)

    for entry in foundation["configurations"]:
        configuration = entry["configuration"]; previous = ROOT / entry["receipt"]["path"]
        predecessor = json.loads(previous.read_text(encoding="utf-8"))["operation"]
        initial = predecessor["final"]["company"]; credit = invoice = expenses = ticks = income = 0
        receipts, physical, rows = [identity(previous)], [], []
        # The first quote used the original query-only binary. Retained exact
        # products preserve that admission while subsequent original commands
        # alone perform paid recall, finite haulage, chemistry and empty sale.
        for field in ("source", "before", "output", "application", "terminal", "retained_harness"):
            verify(predecessor[field])
        for reference in predecessor["sources"] + predecessor["native_processes"]:
            verify(reference)
        processes.update(reference["path"] for reference in predecessor["native_processes"])
        physical.append(chunks(ROOT / predecessor["output"]["path"])[1]); rows.append(predecessor["rows"])
        selections = [("recall", f"resupply-recall-{configuration}")]
        selections += [("traffic-work", f"resupply-traffic-{configuration}-{index}") for index in (1, 2)]
        selections += [("buy-salt", f"resupply-salt-bought-{configuration}"), ("salt-work", f"resupply-salt-collected-{configuration}-1"),
                       ("chemical-order", f"resupply-chemical-ordered-{configuration}"), ("chemical-source-work", f"resupply-chemical-source-{configuration}-1"),
                       ("salt-delivery-order", f"resupply-salt-delivery-ordered-{configuration}")]
        selections += [("refill-work", f"resupply-chemicals-collected-{configuration}-{index}") for index in (1, 2)]
        selections += [("settle", f"resupply-settled-{configuration}")]
        for phase, name in selections:
            path = ROOT / f"logs/surface-entry-{name}.json"; value = json.loads(path.read_text(encoding="utf-8")); operation = value["operation"]
            assert value["complete"] and value["configuration"] == configuration and value["phase"] == phase
            assert operation["receipt"] == identity(previous) and operation["source"] == predecessor["output"]
            for field in ("source", "before", "output", "application", "terminal", "retained_harness"):
                verify(operation[field])
            for reference in operation["sources"]:
                verify(reference)
            for reference in operation["native_processes"]:
                verify(reference); report = json.loads((ROOT / reference["path"]).read_text(encoding="utf-8"))
                assert report["exit_status"] == 0 and report["inputs"] == [operation["source"]] and report["executable"] == operation["application"]
                processes.add(reference["path"])
            for field, snapshot in (("before", "initial"), ("output", "final")):
                assert json.loads(json.dumps(observe(ROOT / operation[field]["path"]))) == operation[snapshot]
            invoice += sum(row["cost"] for row in operation["rows"]["invoices"])
            for work in operation["rows"]["earned"]:
                expenses += work["net_expense"]; ticks += work["earned"]; income += work["income"]
            if phase == "buy-salt":
                purchase, = operation["rows"]["purchase"]; credit += purchase["credit"]
            if phase == "settle":
                settled, = operation["rows"]["settled"]
                assert settled["empty_sale"] and settled["resumed"] == 3 and settled["chemicals"] == 21
            physical.append(chunks(ROOT / operation["output"]["path"])[1]); rows.append(operation["rows"]); receipts.append(identity(path))
            previous, predecessor = path, operation
        final = predecessor["final"]; refill = final["chemical_refill"]
        assert refill["new_delivered_salt"] == 60 and refill["new_held_chemicals"] == 21 and not refill["carriers"]
        assert final["chemical_feed"]["held"] == 21 and final["chemical_feed"]["stopped"] and final["chemical_feed"]["depot"] and final["chemical_feed"]["tile"] == 2443
        assert not final["traffic_halted"] and final["plant"]["chemicals"] == 1 and final["plant"]["produced"] == 20
        assert final["company"]["cash"] == initial["cash"] + credit - invoice - expenses and final["company"]["debt"] == initial["debt"] + credit <= 5000000
        builds.append((physical, rows))
        summaries.append(dict(configuration=configuration, receipts=receipts, receipt=identity(previous), output=predecessor["output"], company=final["company"],
                              chemical_feed=final["chemical_feed"], chemical_refill=refill, actual_credit=credit, net_command_cost=invoice,
                              earned_ticks=ticks, actual_income=income, net_work_expense=expenses, mine=final["mine"], original_chemical=final["original_chemical"]))
    assert builds[0][1] == builds[1][1]
    for first, second in zip(builds[0][0], builds[1][0], strict=True):
        assert first.keys() == second.keys() and all(first[key] == second[key] for key in first if key != b"GLOG")
    return dict(_documentation=__doc__, complete=True, actual_refill_complete=True, actual_launch_complete=False, foundation=identity(foundation_path),
                retained_products=identity(retained_path), native_processes=len(processes), cross_build_gameplay_comparisons=len(builds[0][0]) * 120,
                configurations=summaries, source=identity(Path(__file__)))


def main():
    """Retain only actual selected original stock and capital, keeping future Propellant and launch obligations open."""
    parser = argparse.ArgumentParser(description=__doc__); parser.add_argument("--output", type=Path, required=True)
    output = (ROOT / parser.parse_args().output).resolve(); assert output.is_relative_to(ROOT / "logs") and not output.exists()
    output.write_text(json.dumps(execute(), indent=2) + "\n", encoding="utf-8")
    print(output.relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
