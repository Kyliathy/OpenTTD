"""Quote, physically return and genuinely renew four original near-due services.

@file surface_entry_capital_renewal_checks-1.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CQ.
No loaded body is sold and no future refund or fare contributes to admission.
Original eleven useful roles, additional bus and all material owners survive.
"""
import argparse
import json
from pathlib import Path
import runpy

from space_preview_checks import ROOT, chunks
from surface_regional_ledger import TABLE
from surface_salt_native import execute as native

DIRECTORY = Path(__file__).resolve().parent
PREVIOUS = runpy.run_path(str(DIRECTORY / "surface_entry_capital_income_checks-1.py"))
observe = PREVIOUS["observe"]
PHASES = {name: "staged-chemical-capital-renewal-" + name for name in ("query", "recall", "work", "pay")}
ROLES = {"Surface Regional passenger 1", "Surface Regional passenger 2", "Surface Regional passenger 3", "Surface Steel delivery"}


def execute(configuration, phase, receipt, recover_run=None):
    """Consume one immediately preceding original in the funded renewal chain.

    @param configuration Independently linked actual native focused application.
    @param phase Pure whole quote, native recall, bounded physical return or actual empty replacement settlement.
    @param receipt Exact previous original paid/work boundary.
    @param recover_run Successful invocation to inspect without replaying real commands/work.
    @return Native invoices/cargo progress, exact service lives and preserved original owners.
    """
    predecessor = json.loads(receipt.read_text(encoding="utf-8"))
    assert predecessor["complete"] and predecessor["configuration"] == configuration
    expected = {"query": "staged-chemical-capital-work", "recall": PHASES["query"], "work": PHASES["recall"], "pay": PHASES["work"]}
    assert predecessor["phase"] == expected[phase]
    if phase == "pay":
        previous, = predecessor["operation"]["rows"]["returned"]
        assert previous["empty_home"] and not previous["partial"]
    files = PREVIOUS["PREVIOUS"]["caller_files"]() + [Path(__file__), DIRECTORY / "surface_entry_capital_income_checks-1.py"]
    files += [ROOT / "src/tests" / name for name in ("surface_entry_capital_renewals.h", "surface_entry_capital_income.h",
        "surface_entry_module_capital.h", "surface_regional_layout.h", "surface_passenger_layout.h",
        "surface_entry_bus_renewal.cpp", "surface_entry_road_renewal.cpp")]
    files += [ROOT / "src" / name for name in ("engine.cpp", "roadveh_cmd.cpp")]
    operation = native(configuration, PHASES[phase], receipt, observe,
        dict(admission="RENEWAL_ADMISSION", recall="RENEWAL_RECALL", returned="RENEWAL_RETURN", paid="RENEWAL_PAID",
             settled="RENEWAL_SETTLED", invoices="INVOICE", earned="EARNED", expenses="EXPENSE", delivered="DELIVERED"),
        files, recover_run=recover_run, chemical_limit=3000)
    first, last, rows = operation["initial"], operation["final"], operation["rows"]
    assert json.loads(json.dumps(first)) == predecessor["operation"]["final"]
    assert first["totals"] == last["totals"] == {"STEL": 32, "GOOD": 50, "PROP": 38}
    assert last["new_fuel_at_center"] and not last["handling"]["jobs"]
    assert first["company"]["debt"] == last["company"]["debt"] == 5000000
    assert last["company"]["cash"] >= 40000
    assert first["capital_service"]["vehicle"] == last["capital_service"]["vehicle"] == 9
    assert len(first["service_lives"]) == len(last["service_lives"]) == 11
    admission, = rows["admission"]
    assert admission["funded"] and admission["gross"] == 25633 and admission["reserve"] == 40000
    assert admission["refunds_counted"] == admission["projected_fares"] == admission["credit"] == 0
    before, output = [ROOT / operation[key]["path"] for key in ("before", "output")]
    elapsed = TABLE(output, b"DATE")[0]["tick_counter"] - TABLE(before, b"DATE")[0]["tick_counter"]
    if phase == "query":
        assert first == last and chunks(before)[1] == chunks(output)[1] and len(rows["recall"]) == 4
    elif phase == "recall":
        assert elapsed == 0 and first["company"] == last["company"]
        assert all(row["cost"] == 0 for row in rows["invoices"]) and first["service_lives"] == last["service_lives"]
    elif phase == "work":
        returned, = rows["returned"]
        earned, = rows["earned"]
        assert 0 < elapsed == earned["earned"] == returned["ticks"] <= 3000
        assert last["company"]["cash"] >= 65633 and returned["partial"] != returned["empty_home"]
        assert earned["cash_before"] - earned["cash"] == sum(row["amount"] for row in rows["expenses"])
    else:
        assert elapsed == 0 and len(rows["paid"]) == 4
        settled, = rows["settled"]
        assert settled["gross"] == sum(row["gross"] for row in rows["paid"]) == 25633
        assert settled["refunds"] == sum(row["refund"] for row in rows["paid"])
        assert first["company"]["cash"] - last["company"]["cash"] == sum(row["cost"] for row in rows["invoices"])
        old_roles, new_roles = [{row["name"]: row for row in view["service_lives"] if row["name"] in ROLES} for view in (first, last)]
        assert old_roles.keys() == new_roles.keys() == ROLES
        # Each genuinely new body must preserve its named useful role and
        # model while resetting age only through the actual paid native buy.
        for name, row in new_roles.items():
            assert row["vehicle"] != old_roles[name]["vehicle"] and row["age"] == 0
            assert row["engine"] == old_roles[name]["engine"] and row["remaining"] == 5490
        assert all(row["empty"] and row["orders"] == 2 for row in rows["paid"])
    return operation


def main():
    """Publish one immutable actual near-due service renewal boundary under logs."""
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
