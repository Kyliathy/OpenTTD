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

from space_preview_checks import ROOT, identity
from artifact_paths import create_run_context
from runner import run_native
from surface_income_ledger import observe, reserve_identity

FIELDS = runpy.run_path(str(Path(__file__).with_name("surface_entry_chemical_checks-2.py")))["fields"]


def execute(configuration, receipt, phase):
    """Run one exact authorized native purchase/work boundary and retain its real owners.

    @param configuration Independently linked focused native game.
    @param receipt Exact original first chemical custody, or previous commerce output.
    @param phase Complete native funding or one bounded earning period.
    @return Actual invoices, native financial deltas, cargo and process/source identities.
    """
    selected = json.loads(receipt.read_text(encoding="utf-8"))
    assert selected["complete"] and selected["configuration"] == configuration
    assert selected["phase"] == "collect" if phase == "fund" else selected["phase"] in ("fund", "earn")
    source = ROOT / selected["operation"]["output"]["path"]
    assert identity(source) == selected["operation"]["output"]
    foundation_path = ROOT / "logs/surface-entry-fuel-income-plan-verification.json"
    foundation = json.loads(foundation_path.read_text(encoding="utf-8"))
    assert foundation["complete"] and foundation["selected_income_route"]
    context = create_run_context("space", "surface-entry-income-" + phase)
    context.prepare_runtime()
    retained = context.directory / "harness.py"
    retained.write_bytes(Path(__file__).read_bytes())
    (context.runtime_dir / "scripts/game_start.scr").write_text("surface_entry_income\n", encoding="utf-8")
    application = ROOT / "build" / configuration / "openttd_surface_supply_test.exe"
    original = identity(source)
    log = context.logs / (phase + ".log")
    with log.open("wb") as sink:
        result = run_native(context, [str(application), "[.surface-entry-income]"], cwd=ROOT / "build", stdout=sink,
                            stderr=subprocess.STDOUT, timeout=110,
                            env=dict(os.environ, OTTD_TEST_CONFIGURATION=configuration, OTTD_SURFACE_ENTRY_SOURCE=str(source), OTTD_SURFACE_INCOME_PHASE=phase))
    terminal = log.read_text(encoding="utf-8", errors="replace")
    assert result.returncode == 0 and "All tests passed" in terminal and "SURFACE_INCOME_COMPLETE phase=" + phase in terminal, log
    assert identity(source) == original
    before, output = [context.saves / ("surface-income-" + name + "-440.sav") for name in ("before-" + phase, phase)]
    initial, final = [observe(path) for path in (before, output)]
    assert reserve_identity(initial) == reserve_identity(final)
    assert initial["reserves"] == final["reserves"] == dict(steel=36, goods=14, chemicals=21)
    assert initial["company"]["debt"] == final["company"]["debt"] == 4650000
    prefixes = dict(invoices="INVOICE", stations="STATION", trains="TRAIN_PAID", capacities="TRAIN_CAPACITY", funded="FUNDED",
                    expenses="EXPENSE", earned="EARNED", delivered="DELIVERED", positions="POSITION")
    rows = {name: [FIELDS(line) for line in terminal.splitlines() if line.startswith("SURFACE_INCOME_" + prefix + " ")]
            for name, prefix in prefixes.items()}
    if phase == "fund":
        assert rows["funded"] == [dict(sale=145769, infrastructure=44801, vehicles=126807, cash=156146, debt=4650000, supplied_stock=False)]
        assert sum(row["cost"] for row in rows["invoices"]) == 181985 - 156146
        assert len(rows["stations"]) == 6 and len(rows["trains"]) == 3
        assert not initial["trains"] and len(final["trains"]) == 3
        assert all(not any(row["held"].values()) for row in final["trains"])
        assert initial["mine"] == final["mine"] and initial["chemical"] == final["chemical"]
    else:
        earned, = rows["earned"]
        assert 0 < earned["earned"] <= 27000 and earned["debt"] == 4650000
        assert earned["cash_before"] == initial["company"]["cash"] and earned["cash"] == final["company"]["cash"]
        assert sum(row["amount"] for row in rows["expenses"]) == earned["net_expense"] == earned["cash_before"] - earned["cash"]
        assert earned["income"] == -sum(row["amount"] for row in rows["expenses"] if row["type"] in (7, 8, 9, 10))
        assert not rows["invoices"]
    return dict(receipt=identity(receipt), arrangement=identity(foundation_path), source=original, before=identity(before), output=identity(output),
                application=identity(application), terminal=identity(log), rows=rows, initial=initial, final=final,
                retained_harness=identity(retained), native_processes=[identity(path) for path in sorted(context.reports.glob("native-*.json"))],
                sources=[identity(Path(__file__)), identity(Path(__file__).with_name("surface_income_ledger.py")),
                         identity(ROOT / "src/tests/surface_entry_income.cpp"), identity(ROOT / "src/tests/surface_fuel_rail_layout.h")])


def main():
    """Retain the native purchase/work boundary without claiming final fuel or launch completion."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), required=True)
    parser.add_argument("--source-receipt", type=Path, required=True)
    parser.add_argument("--phase", choices=("fund", "earn"), required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    receipt, output = [(ROOT / path).resolve() for path in (args.source_receipt, args.output)]
    assert receipt.is_relative_to(ROOT / "logs") and receipt.is_file()
    assert output.is_relative_to(ROOT / "logs") and not output.exists()
    operation = execute(args.configuration, receipt, args.phase)
    output.write_text(json.dumps(dict(_documentation=__doc__, complete=True, phase=args.phase, configuration=args.configuration, operation=operation), indent=2) + "\n", encoding="utf-8")
    print(json.dumps(dict(output=output.relative_to(ROOT).as_posix(), phase=args.phase, rows=operation["rows"]["funded"] or operation["rows"]["earned"])), flush=True)


if __name__ == "__main__":
    main()
