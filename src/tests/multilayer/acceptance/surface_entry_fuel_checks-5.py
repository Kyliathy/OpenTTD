"""Execute the accepted genuine commercial purchase or one bounded native earning period.

@file surface_entry_fuel_checks-5.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15BI.2.
Every penny and cargo unit remains in its original native owner. A conditional
tariff bound cannot substitute for an actual accepted, paid final delivery.
"""
import argparse
import json
import os
from pathlib import Path
import runpy
import subprocess

from space_preview_checks import ROOT, chunks, identity
from artifact_paths import create_run_context, RunContext, identifier
from runner import run_native
from surface_income_ledger import TABLE, observe, reserve_identity

FIELDS = runpy.run_path(str(Path(__file__).with_name("surface_entry_chemical_checks-2.py")))["fields"]


def execute(configuration, receipt, phase, recover_run=None, reserve=100000):
    """Run one exact authorized native purchase/work boundary and retain its real owners.

    @param configuration Independently linked focused native game.
    @param receipt Exact original first chemical custody, or previous commerce output.
    @param phase Complete native funding, bounded earning, explicit service or matching bank settlement.
    @param recover_run Optional exact successful invocation whose receipt finalization failed; never reruns native work.
    @param reserve Explicit previously selected treasury reserve, preserving original default producer behavior.
    @return Actual invoices, native financial deltas, cargo and process/source identities.
    """
    selected = json.loads(receipt.read_text(encoding="utf-8"))
    assert selected["complete"] and selected["configuration"] == configuration
    sources = {"fund": ("collect",), "earn": ("fund", "earn", "coal-rewagon", "repay", "town-buy", "fleet-buy", "restore"), "coal-query": ("earn",),
               "coal-recall": ("coal-query",), "coal-settle": ("coal-recall",), "coal-rewagon": ("coal-settle",), "repay": ("earn",),
               "town-query": ("repay",), "town-credit": ("town-query",), "town-buy": ("town-credit",), "fleet-query": ("earn",),
               "fleet-recall": ("fleet-query",), "fleet-settle": ("fleet-recall",), "fleet-buy": ("fleet-settle",)}
    assert selected["phase"] in sources[phase]
    source = ROOT / selected["operation"]["output"]["path"]
    assert identity(source) == selected["operation"]["output"]
    foundation_path = ROOT / "logs/surface-entry-fuel-income-plan-verification.json"
    foundation = json.loads(foundation_path.read_text(encoding="utf-8"))
    assert foundation["complete"] and foundation["selected_income_route"]
    context = (RunContext(ROOT, "space", "surface-entry-income-" + phase, identifier(recover_run)) if recover_run
               else create_run_context("space", "surface-entry-income-" + phase))
    retained = context.directory / "harness.py"
    if not recover_run:
        context.prepare_runtime()
        retained.write_bytes(Path(__file__).read_bytes())
        (context.runtime_dir / "scripts/game_start.scr").write_text("surface_entry_income\n", encoding="utf-8")
    application = ROOT / "build" / configuration / "openttd_surface_supply_test.exe"
    assert reserve in (100000, 150000)
    environment = dict(os.environ, OTTD_TEST_CONFIGURATION=configuration, OTTD_SURFACE_ENTRY_SOURCE=str(source), OTTD_SURFACE_INCOME_PHASE=phase,
                       OTTD_SURFACE_INCOME_RESERVE=str(reserve))
    asset_selection = None
    if phase.startswith("fleet-"):
        recipe_path = ROOT / f"logs/surface-entry-fuel-commerce-plan-{configuration}.json"
        recipe = json.loads(recipe_path.read_text(encoding="utf-8"))["operation"]["quotes"]
        tiles = {row["tile"] for row in recipe["rail_tracks"]}
        # The original accepted64-wide map and quoted complete platforms select
        # existing owned assets only. Native code independently validates every
        # current tile, layer and owner before any conversion can be considered.
        for terminal in recipe["rail_termini"]:
            step = 1 if terminal["axis"] == 0 else 64
            tiles.update(terminal["anchor"] + index * step for index in range(terminal["length"]))
            if terminal["depot"] != 4294967295:
                tiles.add(terminal["depot"])
        for bridge in recipe["rail_bridges"]:
            tiles.update((bridge["near"], bridge["far"]))
        assert len(tiles) == 230
        environment["OTTD_SURFACE_INCOME_RAIL_TILES"] = " ".join(map(str, sorted(tiles)))
        asset_selection = dict(source=identity(recipe_path), tiles=sorted(tiles))
    original = identity(source)
    log = context.logs / (phase + ".log")
    if recover_run:
        # Recovery consumes only the exact already successful native report.
        # Its original executable, input and outputs must still be unchanged;
        # no replayed purchase or replaced log can repair a failed native run.
        report, = list(context.reports.glob("native-*.json"))
        completed = json.loads(report.read_text(encoding="utf-8"))
        assert completed["exit_status"] == 0 and completed["configuration"] == configuration
        assert original in completed["inputs"]
        for reference in completed["inputs"] + completed["artifacts"]:
            assert identity(ROOT / reference["path"]) == reference
        exit_status = completed["exit_status"]
    else:
        with log.open("wb") as sink:
            result = run_native(context, [str(application), "[.surface-entry-income]"], cwd=ROOT / "build", stdout=sink,
                                stderr=subprocess.STDOUT, timeout=110, env=environment)
        exit_status = result.returncode
    terminal = log.read_text(encoding="utf-8", errors="replace")
    assert exit_status == 0 and "All tests passed" in terminal and "SURFACE_INCOME_COMPLETE phase=" + phase in terminal, log
    assert identity(source) == original
    before, output = [context.saves / ("surface-income-" + name + "-440.sav") for name in ("before-" + phase, phase)]
    initial, final = [observe(path) for path in (before, output)]
    assert reserve_identity(initial) == reserve_identity(final)
    assert initial["reserves"] == final["reserves"] == dict(steel=36, goods=14, chemicals=21)
    expected_debt = 4650000 if phase == "fund" else selected["operation"]["final"]["company"]["debt"]
    assert initial["company"]["debt"] == expected_debt
    if phase not in ("repay", "town-credit", "fleet-buy"):
        assert final["company"]["debt"] == expected_debt
    prefixes = dict(invoices="INVOICE", stations="STATION", trains="TRAIN_PAID", capacities="TRAIN_CAPACITY", funded="FUNDED",
                    expenses="EXPENSE", earned="EARNED", delivered="DELIVERED", positions="POSITION", fuel_budgets="FUEL_BUDGET",
                    coal_candidates="COAL_CANDIDATE", coal_quote="COAL_QUOTE", coal_recall="COAL_RECALL", coal_rewagon="COAL_REWAGON", repayment="REPAY",
                    service_sources="SERVICE_SOURCE", service_quotes="SERVICE_QUOTE", service_capital="SERVICE_CAPITAL",
                    service_credit="SERVICE_CREDIT", service_bought="SERVICE_BOUGHT", fleet_conversion="FLEET_CONVERSION",
                    fleet_models="FLEET_MODEL", fleet_platforms="FLEET_PLATFORM", fleet_plan="FLEET_PLAN",
                    fleet_recall="FLEET_RECALL", fleet_retained="FLEET_RETAINED", fleet_renewed="FLEET_RENEWED", fleet_bought="FLEET_BOUGHT")
    rows = {name: [FIELDS(line) for line in terminal.splitlines() if line.startswith("SURFACE_INCOME_" + prefix + " ")]
            for name, prefix in prefixes.items()}
    if phase == "fund":
        assert rows["funded"] == [dict(sale=145769, infrastructure=44801, vehicles=126807, cash=156146, debt=4650000, supplied_stock=False)]
        assert sum(row["cost"] for row in rows["invoices"]) == 181985 - 156146
        assert len(rows["stations"]) == 6 and len(rows["trains"]) == 3
        assert not initial["trains"] and len(final["trains"]) == 3
        assert all(not any(row["held"].values()) for row in final["trains"])
        assert final["trains"][2]["capacity"]["COAL"] == 244
        assert initial["mine"] == final["mine"] and initial["chemical"] == final["chemical"]
    elif phase in ("earn", "coal-settle", "fleet-settle"):
        earned, = rows["earned"]
        assert 0 < earned["earned"] <= 27000 and earned["debt"] == expected_debt
        assert earned["cash_before"] == initial["company"]["cash"] and earned["cash"] == final["company"]["cash"]
        assert sum(row["amount"] for row in rows["expenses"]) == earned["net_expense"] == earned["cash_before"] - earned["cash"]
        assert earned["income"] == -sum(row["amount"] for row in rows["expenses"] if row["type"] in (7, 8, 9, 10))
        assert not rows["invoices"]
        assert [row["capacity"] for row in initial["trains"]] == [row["capacity"] for row in final["trains"]]
        if phase == "coal-settle":
            assert final["trains"][2]["stopped"] and final["trains"][2]["tile"] == 3161 and not any(final["trains"][2]["held"].values())
        if phase == "fleet-settle":
            assert [row["tile"] for row in final["trains"]] == [1638, 3224, 3161]
            assert all(row["stopped"] and not any(row["held"].values()) for row in final["trains"])
    elif phase == "coal-query":
        assert chunks(before) == chunks(output) and initial == final
        assert rows["coal_candidates"] and not rows["invoices"]
        quote, = rows["coal_quote"]
        assert quote["old_speed"] == 80 and quote["capacity"] == 30 and quote["count"] == 4
        assert quote["cash"] == initial["company"]["cash"] == 300439 and quote["invoice"] < 10000
    elif phase == "coal-recall":
        assert rows["coal_recall"] and sum(row["cost"] for row in rows["invoices"]) == 0
        assert initial == final
    elif phase == "town-query":
        assert chunks(before) == chunks(output) and initial == final
        assert len(rows["service_sources"]) == 3 and rows["service_quotes"] and len(rows["service_capital"]) == 1
        assert not rows["invoices"] and not rows["repayment"]
    elif phase == "town-credit":
        bank, = rows["service_credit"]
        assert bank == dict(amount=140000, cash_before=105143, debt_before=4420000, cash=245143, debt=4560000, ceiling=5000000, spendable=685143)
        assert final["company"] == dict(cash=245143, debt=4560000)
        assert rows["service_capital"][-1]["admitted"] and rows["service_capital"][-1]["credit"] == 0
        assert all(row["cost"] == 0 for row in rows["invoices"])
    elif phase == "town-buy":
        assert rows["service_bought"] == [dict(towns=1, invoice=140625, cash=104518, debt=4560000, supplied_stock=False)]
        assert sum(row["cost"] for row in rows["invoices"]) == 140625 == initial["company"]["cash"] - final["company"]["cash"]
        assert len(rows["service_quotes"]) == 1 and rows["service_quotes"][0]["admitted"]
    elif phase == "fleet-query":
        assert chunks(before) == chunks(output) and initial == final
        assert asset_selection and len(rows["fleet_plan"]) == 1 and len(rows["fleet_models"]) >= 3
        assert rows["fleet_plan"][0]["tiles"] == 230 and sum(row["cost"] for row in rows["fleet_conversion"]) == rows["fleet_plan"][0]["conversion"]
        assert not rows["invoices"] and not rows["repayment"]
        assert rows["fleet_plan"][0]["coal_extension"] and all(row["admitted"] for row in rows["fleet_platforms"])
    elif phase == "fleet-recall":
        assert len(rows["fleet_recall"]) == 3 and initial == final
        assert all(row["cost"] == 0 for row in rows["invoices"])
    elif phase == "fleet-buy":
        bought, = rows["fleet_bought"]
        assert len(rows["fleet_renewed"]) == 3 and len(rows["fleet_retained"]) == 28
        assert bought["ceiling"] == 5000000 and bought["credit"] % 10000 == 0
        assert initial["company"] == dict(cash=bought["cash_before"], debt=bought["debt_before"])
        assert final["company"] == dict(cash=bought["cash"], debt=bought["debt"])
        assert bought["cash"] == bought["cash_before"] + bought["credit"] - bought["invoice"] >= 150000
        assert bought["debt"] == bought["debt_before"] + bought["credit"] <= 5000000
        assert sum(row["cost"] for row in rows["invoices"]) == bought["invoice"]
        assert all(row["engine"] == 26 and row["speed"] == 264 and row["reliability"] > 50000 for row in rows["fleet_renewed"])
        # Every previously paid useful wagon must remain the very same native
        # vehicle. Only the approved four additional Coal wagons may be new.
        for old, new in zip(initial["trains"], final["trains"]):
            retained_wagons = {row["wagon"] for row in rows["fleet_retained"] if row["role"] == old["role"]}
            assert retained_wagons == set(old["chain"][1:-1]) and retained_wagons.issubset(new["chain"])
            assert old["stopped"] and not any(old["held"].values()) and not any(new["held"].values())
        assert initial["mine"] == final["mine"] and initial["chemical"] == final["chemical"]
    elif phase == "repay":
        bank, = rows["repayment"]
        assert bank["cash_before"] == initial["company"]["cash"] and bank["debt_before"] == initial["company"]["debt"]
        assert bank["cash"] == final["company"]["cash"] == bank["cash_before"] - bank["amount"]
        assert bank["debt"] == final["company"]["debt"] == bank["debt_before"] - bank["amount"]
        assert bank["ceiling"] == 5000000 and bank["buffer"] == reserve and reserve <= bank["cash"] < reserve + 10000
        assert bank["amount"] % 10000 == 0 and bank["annual_interest_reduction"] == bank["amount"] * 2 // 100
        assert bank["spendable"] == bank["cash_before"] + 5000000 - bank["debt_before"] == bank["cash"] + 5000000 - bank["debt"]
        first, last = [chunks(path)[1] for path in (before, output)]
        assert first.keys() == last.keys() and all(first[key] == last[key] for key in first if key != b"PLYR")
    else:
        repair, = rows["coal_rewagon"]
        quote, = rows["coal_quote"]
        assert initial["trains"][2]["stopped"] and initial["trains"][2]["tile"] == 3161
        assert initial["trains"][2]["capacity"]["COAL"] == 244 and final["trains"][2]["capacity"]["COAL"] == 124
        assert initial["trains"][2]["head"] == final["trains"][2]["head"] == repair["head"]
        assert repair["speed"] == 200 and repair["length"] == 48 and repair["engine"] == quote["engine"]
        assert all(engine == quote["engine"] for engine in final["trains"][2]["engines"][1:-1])
        assert sum(row["cost"] for row in rows["invoices"]) == repair["invoice"] == initial["company"]["cash"] - final["company"]["cash"]
    if phase in ("repay", "town-credit"):
        # Native bank settlement changes only matching money/debt, preserving
        # every other company field and all other saved families exactly.
        first, last = [chunks(path)[1] for path in (before, output)]
        assert all(first[key] == last[key] for key in first if key != b"PLYR")
        company_rows = [next(iter(TABLE(path, b"PLYR").values())) for path in (before, output)]
        assert {key: value for key, value in company_rows[0].items() if key not in ("money", "current_loan")} == {
            key: value for key, value in company_rows[1].items() if key not in ("money", "current_loan")}
    return dict(receipt=identity(receipt), arrangement=identity(foundation_path), source=original, before=identity(before), output=identity(output),
                application=identity(application), terminal=identity(log), rows=rows, initial=initial, final=final, asset_selection=asset_selection,
                retained_harness=identity(retained), native_processes=[identity(path) for path in sorted(context.reports.glob("native-*.json"))],
                sources=[identity(Path(__file__)), identity(Path(__file__).with_name("surface_income_ledger.py")),
                         identity(ROOT / "src/tests/surface_entry_income.cpp"), identity(ROOT / "src/tests/surface_fuel_rail_layout.h")])


def main():
    """Retain the native purchase/work boundary without claiming final fuel or launch completion."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), required=True)
    parser.add_argument("--source-receipt", type=Path, required=True)
    parser.add_argument("--phase", choices=("fund", "earn", "coal-query", "coal-recall", "coal-settle", "coal-rewagon", "repay", "town-query", "town-credit", "town-buy", "fleet-query", "fleet-recall", "fleet-settle", "fleet-buy"), required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--recover-run", help="Exact successful native run ID; inspect existing artifacts without rerunning work.")
    parser.add_argument("--reserve", choices=(100000, 150000), type=int, default=100000)
    args = parser.parse_args()
    receipt, output = [(ROOT / path).resolve() for path in (args.source_receipt, args.output)]
    assert receipt.is_relative_to(ROOT / "logs") and receipt.is_file()
    assert output.is_relative_to(ROOT / "logs") and not output.exists()
    operation = execute(args.configuration, receipt, args.phase, args.recover_run, args.reserve)
    output.write_text(json.dumps(dict(_documentation=__doc__, complete=True, phase=args.phase, configuration=args.configuration, operation=operation), indent=2) + "\n", encoding="utf-8")
    print(json.dumps(dict(output=output.relative_to(ROOT).as_posix(), phase=args.phase,
                          rows=operation["rows"]["funded"] or operation["rows"]["earned"] or operation["rows"]["repayment"] or
                          operation["rows"]["service_credit"] or operation["rows"]["service_bought"])), flush=True)


if __name__ == "__main__":
    main()
