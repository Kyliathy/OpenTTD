"""Bind genuine empty recall, paid heads and useful original freight service.

@file surface_entry_renewal_checks-2.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15BIEE.2.
Original native wagon identities, orders, packet owners and company books are
the acceptance boundary. Unsold value and alternative branches provide no cash.
"""
import argparse
import json
from pathlib import Path

from space_preview_checks import ROOT, chunks, identity
from surface_propellant_ledger import observe


def execute(arrival_bursts):
    """@param arrival_bursts Actual selected bounded arrival receipts. @return Complete both-build native replacement, conserved wagons and measured income."""
    foundation_path = ROOT / "logs/surface-entry-freight-refresh-plan-verification.json"
    foundation = json.loads(foundation_path.read_text(encoding="utf-8")); assert foundation["complete"]
    processes, builds, summaries, verified = set(), [], [], set()

    def verify(reference):
        """@param reference Original immutable artifact or current tested source. Require exact bytes once, without replaying a command."""
        key = (reference["path"], reference["sha256"])
        if key not in verified:
            assert identity(ROOT / reference["path"]) == reference; verified.add(key)

    for entry in foundation["configurations"]:
        configuration = entry["configuration"]; previous = ROOT / entry["receipt"]["path"]
        predecessor = json.loads(previous.read_text(encoding="utf-8"))["operation"]
        initial = predecessor["final"]["company"]; credit = net_cost = work_expense = income = ticks = 0
        receipts, physical, rows = [], [], []
        selections = [("recall", f"logs/surface-entry-freight-refresh-recalled-{configuration}.json")]
        selections += [("arrive", f"logs/surface-entry-freight-refresh-arrived-{configuration}-{index}.json") for index in range(1, arrival_bursts + 1)]
        selections += [("buy", f"logs/surface-entry-freight-refresh-bought-{configuration}.json"), ("earn", f"logs/surface-entry-freight-refresh-earned-{configuration}-1.json")]
        # Only the immediately selected predecessor may supply the next actual
        # native state. Recall consumes time and earns delivery normally;
        # replacement requires genuine empty arrival and matching bank credit.
        for phase, name in selections:
            path = ROOT / name; value = json.loads(path.read_text(encoding="utf-8"))
            assert value["complete"] and value["configuration"] == configuration and value["phase"] == phase
            operation = value["operation"]
            assert operation["receipt"] == identity(previous) and operation["source"] == predecessor["output"]
            for field in ("source", "before", "output", "application", "terminal", "retained_harness"):
                verify(operation[field])
            for reference in operation["sources"]:
                verify(reference)
            for reference in operation["native_processes"]:
                verify(reference); report = json.loads((ROOT / reference["path"]).read_text(encoding="utf-8"))
                assert report["exit_status"] == 0 and report["inputs"] == [operation["source"]] and report["executable"] == operation["application"]
                assert report["configuration"] == configuration; processes.add(reference["path"])
            assert observe(ROOT / operation["before"]["path"]) == operation["initial"]
            assert observe(ROOT / operation["output"]["path"]) == operation["final"]
            if phase in ("arrive", "earn"):
                earned, = operation["rows"]["earned"]
                ticks += earned["earned"]; work_expense += earned["net_expense"]; income += earned["income"]
            if phase == "buy":
                purchase, = operation["rows"]["purchase"]; credit += purchase["credit"]; net_cost += purchase["net_cost"]
                assert operation["initial"]["traffic_halted"] and not operation["final"]["traffic_halted"]
                wagons = operation["rows"]["wagons"]
                assert len(wagons) == len({row["vehicle"] for row in wagons}) == 32
            if phase == "earn":
                delivered = {row["label"]: row["amount"] for row in operation["rows"]["delivered"]}
                assert all(delivered[label] > 0 for label in ("COAL", "GRAI", "LVST", "GOOD"))
                assert earned["earned"] == 27000 and earned["net_expense"] < 0
            physical.append(chunks(ROOT / operation["output"]["path"])[1]); rows.append(operation["rows"]); receipts.append(identity(path))
            previous = path; predecessor = operation
        company = predecessor["final"]["company"]
        assert company["cash"] == initial["cash"] + credit - net_cost - work_expense and company["debt"] == initial["debt"] + credit <= 5000000
        builds.append((physical, rows))
        summaries.append(dict(configuration=configuration, receipts=receipts, receipt=identity(previous), output=predecessor["output"], company=company,
                              matching_credit=credit, net_purchase=net_cost, earned_ticks=ticks, actual_income=income, net_earned=-work_expense,
                              renewed_income=predecessor["rows"]["earned"], renewed_deliveries=predecessor["rows"]["delivered"]))
    assert builds[0][1] == builds[1][1]
    for first, second in zip(builds[0][0], builds[1][0], strict=True):
        assert first.keys() == second.keys() and all(first[key] == second[key] for key in first if key != b"GLOG")
    return dict(_documentation=__doc__, complete=True, actual_renewal_complete=True, useful_wagons_retained=32, foundation=identity(foundation_path),
                native_processes=len(processes), cross_build_gameplay_comparisons=len(builds[0][0]) * 120,
                configurations=summaries, sources=[identity(Path(__file__))])


def main():
    """Rebuild the focused renewal proof from exact accepted native outputs without replaying purchases."""
    parser = argparse.ArgumentParser(description=__doc__); parser.add_argument("--arrivals", type=int, required=True)
    parser.add_argument("--output", type=Path, required=True); args = parser.parse_args(); output = (ROOT / args.output).resolve()
    assert args.arrivals > 0 and output.is_relative_to(ROOT / "logs") and not output.exists()
    output.write_text(json.dumps(execute(args.arrivals), indent=2) + "\n", encoding="utf-8")
    print(output.relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
