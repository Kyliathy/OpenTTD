"""Admit genuine bounded reserve recovery using only existing paid freight service.

@file surface_entry_capital_recovery_checks-1.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CR.
Actual onboard/waiting cargo motivates the proposal but never pays its current
cost-only quote. The remaining module base and working shortfall stay explicit.
"""
import argparse
import json
from pathlib import Path
import runpy

from space_preview_checks import ROOT, chunks
from surface_gateway_material_ledger import FACTORY
from surface_regional_ledger import TABLE
from surface_salt_native import execute as native

DIRECTORY = Path(__file__).resolve().parent
observe = runpy.run_path(str(DIRECTORY / "surface_entry_capital_income_checks-1.py"))["observe"]
BASE = runpy.run_path(str(DIRECTORY / "surface_entry_propellant_delivery_checks-1.py"))


def saved_consists(path):
    """@param path Actual unchanged native world. @return Independently decoded linked-body capacities and physical packet quantities for every original rail consist."""
    vehicles, packets = [TABLE(path, key) for key in (b"VEHS", b"CAPA")]
    labels = FACTORY["observe_factory"](path)["cargo_labels"]
    prefix = "train[0].common[0]."
    seen, result = set(), {}
    for head, row in vehicles.items():
        if row["type"] != 0:
            continue
        if not bytes.fromhex(row[prefix + "name"]).decode("utf-8").startswith("Surface Income "):
            continue
        key, capacity, held = head, {}, {}
        # Follow saved one-based native references and claim every real body
        # once. A disconnected or duplicated wagon cannot become extra capacity.
        while key is not None:
            assert key not in seen
            seen.add(key)
            item = vehicles[key]
            label = labels[item[prefix + "cargo_type"]]
            capacity[label] = capacity.get(label, 0) + item[prefix + "cargo_cap"]
            held[label] = held.get(label, 0) + sum(packets[reference - 1]["count"] for reference in item[prefix + "cargo.packets"])
            following = item[prefix + "next"]
            key = following - 1 if following else None
        result.update({(head, label): (count, held[label]) for label, count in capacity.items() if count})
    assert len(seen) == 35
    return result


def caller_files():
    """@return Exact current financial/native routing authorities and strict material/service observer graph."""
    files = BASE["caller_files"]() + [Path(__file__), DIRECTORY / "surface_entry_capital_income_checks-1.py"]
    files += [ROOT / "src/tests" / name for name in ("surface_entry_capital_recovery.h", "surface_entry_capital_renewals.h",
        "surface_entry_capital_income.h", "surface_entry_module_capital.h", "surface_regional_layout.h", "surface_passenger_layout.h")]
    files += [ROOT / "src" / name for name in ("engine.cpp", "roadveh_cmd.cpp", "train_cmd.cpp", "order_cmd.cpp")]
    return files


def execute(configuration, receipt, recover_run=None):
    """Run one pure current freight and whole cost-only recovery admission.

    @param configuration Independently linked focused native application.
    @param receipt Actual completed four-service renewal original, unchanged by this proposal.
    @param recover_run Successful pure invocation to inspect without replaying it.
    @return Exact current cash, sole cargo custody, paid consist/demand and fully funded bounded operating ceiling.
    """
    predecessor = json.loads(receipt.read_text(encoding="utf-8"))
    assert predecessor["complete"] and predecessor["phase"] == "staged-chemical-capital-renewal-pay"
    operation = native(configuration, "staged-chemical-capital-recovery-query", receipt, observe,
        dict(admission="RECOVERY_ADMISSION", routes="RECOVERY_ROUTE", consists="RECOVERY_CONSIST",
             demand="RECOVERY_DEMAND", proposal="RECOVERY_PROPOSAL"), caller_files(), recover_run=recover_run)
    assert operation["initial"] == operation["final"]
    assert json.loads(json.dumps(operation["initial"])) == predecessor["operation"]["final"]
    assert chunks(ROOT / operation["before"]["path"])[1] == chunks(ROOT / operation["output"]["path"])[1]
    admission, = operation["rows"]["admission"]
    proposal, = operation["rows"]["proposal"]
    assert admission["funded"] and admission["cash"] == 91207 and admission["working_shortfall"] == 8793
    assert admission["reserve"] == 40000 and admission["target"] <= admission["cash"]
    assert admission["purchases"] == admission["future_income_counted"] == admission["refunds_counted"] == admission["credit"] == 0
    assert admission["module_base_shortfall"] == 418793 and admission["module_reserve"] == 150000
    assert proposal["wagons"] == 32 and proposal["heads"] == 3 and proposal["bodies"] == 35
    assert not proposal["guaranteed_income"] and not proposal["module_capital_complete"]
    assert proposal["added_vehicles"] == proposal["changed_orders"] == proposal["purchase_cost"] == 0
    consists = {(row["vehicle"], row["label"]): (row["capacity"], row["held"]) for row in operation["rows"]["consists"]}
    assert consists == saved_consists(ROOT / operation["output"]["path"])
    assert consists[49, "GOOD"] == (300, 300)
    assert len(operation["rows"]["routes"]) == 6
    assert min(row["remaining"] for row in operation["final"]["service_lives"]) == 3940
    return operation


def main():
    """Publish one immutable read-only current reserve-recovery admission under logs."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), required=True)
    parser.add_argument("--receipt", type=Path, required=True)
    parser.add_argument("--recover-run")
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    output = ROOT / args.output
    assert output.resolve().is_relative_to(ROOT / "logs") and not output.exists()
    operation = execute(args.configuration, ROOT / args.receipt, args.recover_run)
    output.write_text(json.dumps(dict(_documentation=__doc__, complete=True, configuration=args.configuration,
                                     phase="staged-chemical-capital-recovery-query", operation=operation), indent=2) + "\n", encoding="utf-8")
    print(output.relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
