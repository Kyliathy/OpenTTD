"""Freshly admit and earn the actual loaded Salt train's short depot return.

@file surface_entry_salt_window_checks-5.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CID.1/.2.
The original500ticks and paid train are retained. Only a separately quoted
<=200tick continuation may advance its actual60Source14Salt toward home.
"""
import argparse
import json
from pathlib import Path
import runpy

from space_preview_checks import ROOT, chunks
from surface_gateway_component_delivery_ledger import observe_salt_window as observe
from surface_salt_native import execute as native

DIRECTORY = Path(__file__).resolve().parent
FILES = runpy.run_path(str(DIRECTORY / "surface_entry_component_delivery_checks-1.py"))["FILES"]


def execute(configuration, phase, receipt, recover_run=None):
    """Admit or earn one separately bounded actual full-Salt return interval.

    @param configuration Actual independently rebuilt focused application.
    @param phase Query or return, mapped to explicit native phases.
    @param receipt Exact original500tick output or accepted pure return query.
    @param recover_run Successful native invocation to inspect without repeating work.
    @return Actual fresh current costs and unchanged or earned physical depot-return custody.
    """
    predecessor = json.loads(receipt.read_text(encoding="utf-8"))
    expected = ("staged-work", "staged-return-work") if phase == "query" else ("staged-return-query",)
    assert predecessor["complete"] and predecessor["phase"] in expected
    assert predecessor["operation"]["final"]["salt_consist"]["held"] == 60
    assert not predecessor["operation"]["final"]["salt_consist"]["home"]
    if phase == "return":
        admitted, = predecessor["operation"]["rows"]["peak"]
        assert admitted["funded"] and admitted["ticks"] == 200 and admitted["refund_counted"] == 0
    prefixes = dict(peak="PEAK", progress="ELECTRIC_PROGRESS", earned="EARNED", expenses="EXPENSE", delivered="DELIVERED", invoices="INVOICE")
    files = [Path(__file__)] + [DIRECTORY / name for name in FILES]
    native_phase = "staged-return-query" if phase == "query" else "staged-return-work"
    current = native(configuration, native_phase, receipt, observe, prefixes, files, recover_run)
    initial, final, rows = current["initial"], current["final"], current["rows"]
    assert json.loads(json.dumps(initial)) == predecessor["operation"]["final"]
    assert initial["totals"] == final["totals"] == {"STEL": 32, "GOOD": 50, "PROP": 12}
    assert initial["salt_consist"]["held"] == final["salt_consist"]["held"] == 60
    assert initial["company"]["debt"] == final["company"]["debt"] == 5000000
    assert final["company"]["cash"] >= 100000 and not rows["invoices"]
    peak, = rows["peak"]
    assert peak["ticks"] == 200 and peak["head_cost"] == peak["wagon_cost"] == peak["infrastructure"] == peak["refund_counted"] == 0
    assert not peak["conversion_pending"]
    if phase == "query":
        assert initial == final
        assert chunks(ROOT / current["before"]["path"])[1] == chunks(ROOT / current["output"]["path"])[1]
    else:
        earned, = rows["earned"]
        progress, = rows["progress"]
        assert 0 < earned["earned"] <= 200 and progress["earned"] == earned["earned"]
        assert progress["collected"] == final["salt_consist"]["home"]
        assert final["company"]["cash"] == initial["company"]["cash"] - earned["net_expense"]
        assert earned["net_expense"] == sum(row["amount"] for row in rows["expenses"])
        assert sum(row["amount"] for row in rows["expenses"] if row["amount"] > 0) <= peak["work_bound"]
    return current


def main():
    """Publish one immutable actual return query or capped native continuation."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), required=True)
    parser.add_argument("--phase", choices=("query", "return"), required=True)
    parser.add_argument("--receipt", type=Path, required=True)
    parser.add_argument("--recover-run")
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    output = ROOT / args.output
    assert output.resolve().is_relative_to(ROOT / "logs") and not output.exists()
    operation = execute(args.configuration, args.phase, ROOT / args.receipt, args.recover_run)
    phase = "staged-return-query" if args.phase == "query" else "staged-return-work"
    output.write_text(json.dumps(dict(_documentation=__doc__, complete=True, configuration=args.configuration,
                                      phase=phase, operation=operation), indent=2) + "\n", encoding="utf-8")
    print(output.relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
