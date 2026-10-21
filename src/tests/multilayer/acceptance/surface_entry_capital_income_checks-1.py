"""Pay and measure one genuine additional local passenger bus.

@file surface_entry_capital_income_checks-1.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CP.
Original command receipts, packets, service roles and books retain every owner;
the explicit additional service never relaxes historical default observers.
"""
import argparse
import json
from pathlib import Path
import runpy

from space_preview_checks import ROOT, chunks
from surface_gateway_component_delivery_ledger import observe_propellant_delivery
from surface_regional_ledger import TABLE
from surface_salt_native import execute as native

DIRECTORY = Path(__file__).resolve().parent
PREVIOUS = runpy.run_path(str(DIRECTORY / "surface_entry_propellant_delivery_checks-1.py"))
PHASES = {"buy": "staged-chemical-capital-buy", "work": "staged-chemical-capital-work"}


def observe(path):
    """@param path Actual original before/after one paid extra bus. @return Explicit sole additional Town0PASSowner and strict original11services/material/hulls."""
    return observe_propellant_delivery(path, allow_capital_service=True)


def execute(configuration, phase, receipt, recover_run=None):
    """Perform one funded native buy/configure/start or first bounded observation.

    @param configuration Independently linked actual native focused application.
    @param phase Separate paid startup or real3000tick work, never a replay of earlier work.
    @param receipt Immediately preceding actual original native stage.
    @param recover_run Existing successful invocation to inspect without repeating it.
    @return Actual invoice/profit, whole remaining protection and strict sole material/vehicle ownership.
    """
    predecessor = json.loads(receipt.read_text(encoding="utf-8"))
    assert predecessor["complete"] and predecessor["configuration"] == configuration
    assert predecessor["phase"] == ("staged-chemical-capital-query" if phase == "buy" else PHASES["buy"])
    if phase == "buy":
        chosen, = [row for row in predecessor["operation"]["rows"]["local_admission"] if row["funded"]]
        assert chosen["buses"] == 1 and chosen["reserve"] == 40000 and chosen["renewal"] == 25633
        assert chosen["cash"] >= chosen["target"] == 109454
    files = PREVIOUS["caller_files"]() + [Path(__file__), ROOT / "src/tests/surface_entry_capital_income.h",
        ROOT / "src/tests/surface_entry_module_capital.h", ROOT / "src/tests/surface_regional_layout.h",
        ROOT / "src/tests/surface_passenger_layout.h", ROOT / "src/engine.cpp", ROOT / "src/roadveh_cmd.cpp"]
    operation = native(configuration, PHASES[phase], receipt, observe,
        dict(invoices="INVOICE", paid="CAPITAL_PAID", protection="CAPITAL_PROTECTION", measured="CAPITAL_MEASURED",
             earned="EARNED", expenses="EXPENSE", delivered="DELIVERED", local_admission="CAPITAL_LOCAL_ADMISSION"),
        files, recover_run=recover_run, chemical_limit=3000)
    first, last, rows = operation["initial"], operation["final"], operation["rows"]
    assert all(json.loads(json.dumps(first[key])) == value for key, value in predecessor["operation"]["final"].items())
    assert first["totals"] == last["totals"] == {"STEL": 32, "GOOD": 50, "PROP": 38}
    assert last["new_fuel_at_center"] and not last["handling"]["jobs"]
    assert first["company"]["debt"] == last["company"]["debt"] == 5000000
    assert len(first["service_lives"]) == len(last["service_lives"]) == 11
    added = last["capital_service"]
    assert added["capacity"] == 40 and added["engine"] == 119 and not added["stopped"]
    before, output = [ROOT / operation[key]["path"] for key in ("before", "output")]
    elapsed = TABLE(output, b"DATE")[0]["tick_counter"] - TABLE(before, b"DATE")[0]["tick_counter"]
    if phase == "buy":
        assert elapsed == 0 and first["capital_service"] is None and added["held"] == 0
        paid, = rows["paid"]
        assert paid["gross"] == 6562 and first["company"]["cash"] - last["company"]["cash"] == 6562
        assert sum(row["cost"] for row in rows["invoices"]) == 6562
        assert first["service_lives"] == last["service_lives"] and last["company"]["cash"] == 118005
        # Native vehicle construction legitimately draws the two saved RNG
        # words. Every clock/calendar and other DATEfield must remain exact.
        old_date, new_date = [TABLE(path, b"DATE")[0] for path in (before, output)]
        assert {key for key in old_date if old_date[key] != new_date[key]} == {"random_state[0]", "random_state[1]"}
    else:
        assert elapsed == 3000 and first["capital_service"]["vehicle"] == added["vehicle"]
        protection, = rows["protection"]
        measured, = rows["measured"]
        earned, = rows["earned"]
        assert protection["renewal"] == measured["renewal"] == 25633
        assert protection["funded"] and protection["target"] <= first["company"]["cash"]
        assert earned["earned"] == measured["ticks"] == 3000 and measured["remaining_interval"] == 3000
        assert measured["protected_cash"] <= last["company"]["cash"]
        assert measured["profit_delta256"] == added["profit256"] - first["capital_service"]["profit256"]
        assert earned["cash_before"] - earned["cash"] == sum(row["amount"] for row in rows["expenses"])
        assert earned["cash"] == last["company"]["cash"] and not measured["module_capital_complete"]
    return operation


def main():
    """Publish one immutable original paid startup or bounded native income receipt."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), required=True)
    parser.add_argument("--phase", choices=tuple(PHASES), required=True)
    parser.add_argument("--receipt", type=Path, required=True)
    parser.add_argument("--recover-run")
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    output = ROOT / args.output
    assert output.resolve().is_relative_to(ROOT / "logs") and not output.exists()
    operation = execute(args.configuration, args.phase, ROOT / args.receipt, args.recover_run)
    output.write_text(json.dumps(dict(_documentation=__doc__, complete=True, configuration=args.configuration,
                                     phase=PHASES[args.phase], operation=operation), indent=2) + "\n", encoding="utf-8")
    print(output.relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
