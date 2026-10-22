"""Drive original PCHM funding and short genuine construction bursts.

@file surface_entry_propellant_checks-4.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15BID.2.
The selected21CHEM and36Steel/14Goods remain parked through this boundary.
Only original bank/funding/recipe commands and StateGameLoop may change owners.
"""
import argparse
import json
import os
from pathlib import Path
import runpy
import subprocess

from space_preview_checks import ROOT, chunks, identity
from surface_income_ledger import TABLE, observe, reserve_identity
from artifact_paths import create_run_context
from runner import run_native

FIELDS = runpy.run_path(str(Path(__file__).with_name("surface_entry_chemical_checks-2.py")))["fields"]


def machinery(path):
    """@param path Complete native world. @return Actual new factory identity, physical cells, recipe and empty inventory."""
    industries = TABLE(path, b"INDY")
    selected = [(key, row) for key, row in industries.items() if row["location.tile"] == 1639]
    if not selected:
        return None
    key, industry = selected[0]; assert len(selected) == 1 and industry["type"] == 25 and industry["founder"] == 0
    assert industry["location.w"] == industry["location.h"] == 4 and key != 15
    raw = chunks(path)[1]
    cells = []
    # The actual native map's type/index fields identify every machinery cell;
    # neither a rectangle alone nor a side-record completion flag supplies it.
    for tile in range(len(raw[b"MAPT"])):
        if raw[b"MAPT"][tile] >> 4 != 8:
            continue
        if int.from_bytes(raw[b"MAP2"][2 * tile:2 * tile + 2], "big") == key:
            cells.append(dict(tile=tile, complete=bool(raw[b"MAPO"][tile] & 128), stage=raw[b"MAPO"][tile] & 3,
                              counter=(raw[b"MAPO"][tile] >> 2) & 3))
    assert len(cells) == 16
    state, = [row for row in TABLE(path, b"LPCR").values() if row["industry"] == key]
    assert state["controller"] == 0 and state["recipe"] == int.from_bytes(b"PCHM", "little")
    assert all(row["count"] == 0 for row in TABLE(path, b"LPCC").values() if row["industry"] == key)
    assert all(value == 0 for name, value in industry.items() if name.endswith(".waiting"))
    return dict(industry=key, site=1639, cells=cells, complete=all(row["complete"] for row in cells), state=state,
                consumed=0, produced=0, dispatched=state["dispatched"])


def execute(configuration, receipt, phase):
    """@param configuration Actual focused native build. @param receipt Immediate original predecessor.
    @param phase Native matching credit/factory funding or one3000tick construction burst.
    @return Exact financial/material owners, actual command charges and native readiness.
    """
    selected = json.loads(receipt.read_text(encoding="utf-8"))
    assert selected["complete"] and selected["configuration"] == configuration
    assert selected["phase"] in ({"query"} if phase == "fund" else {"fund", "build"})
    foundation_path = ROOT / "logs/surface-entry-propellant-plan-verification.json"
    foundation = json.loads(foundation_path.read_text(encoding="utf-8")); assert foundation["complete"]
    source = ROOT / selected["operation"]["output"]["path"]; original = identity(source)
    assert original == selected["operation"]["output"]
    context = create_run_context("space", "surface-entry-propellant-" + phase)
    context.prepare_runtime()
    retained = context.directory / "harness.py"; retained.write_bytes(Path(__file__).read_bytes())
    (context.runtime_dir / "scripts/game_start.scr").write_text("surface_entry_propellant\n", encoding="utf-8")
    application = ROOT / "build" / configuration / "openttd_surface_supply_test.exe"
    log = context.logs / (phase + ".log")
    with log.open("wb") as sink:
        result = run_native(context, [str(application), "[.surface-entry-propellant]"], cwd=ROOT / "build", stdout=sink,
                            stderr=subprocess.STDOUT, timeout=110, env=dict(os.environ, OTTD_TEST_CONFIGURATION=configuration,
                            OTTD_SURFACE_ENTRY_SOURCE=str(source), OTTD_SURFACE_PROPELLANT_PHASE=phase))
    terminal = log.read_text(encoding="utf-8", errors="replace")
    assert result.returncode == 0 and "All tests passed" in terminal and "SURFACE_PROPELLANT_COMPLETE phase=" + phase in terminal, log
    assert identity(source) == original
    before, output = [context.saves / f"surface-propellant-{name}-440.sav" for name in ("before-" + phase, phase)]
    initial, final = observe(before), observe(output)
    assert initial["reserves"] == final["reserves"] == dict(steel=36, goods=14, chemicals=21)
    assert reserve_identity(initial) == reserve_identity(final)
    old, new = machinery(before), machinery(output); assert new is not None
    prefixes = dict(invoices="INVOICE", funded="FUNDED", earned="EARNED", expenses="EXPENSE", delivered="DELIVERED", readiness="READINESS")
    rows = {name: [FIELDS(line) for line in terminal.splitlines() if line.startswith("SURFACE_PROPELLANT_" + prefix + " ")]
            for name, prefix in prefixes.items()}
    if phase == "fund":
        funded, = rows["funded"]
        assert old is None and not new["complete"] and new["state"]["work_ticks"] == new["dispatched"] == 0
        assert all(not cell["complete"] and cell["stage"] == cell["counter"] == 0 for cell in new["cells"])
        assert funded["industry"] == new["industry"] and funded["credit"] == 640000 and funded["factory"] == 703125
        assert initial["company"] == dict(cash=173876, debt=4360000) and final["company"] == dict(cash=110751, debt=5000000)
        assert sum(row["cost"] for row in rows["invoices"]) == funded["factory"]
        assert initial["mine"] == final["mine"] and initial["chemical"] == final["chemical"] and initial["trains"] == final["trains"]
    else:
        earned, = rows["earned"]
        assert old is not None and old["industry"] == new["industry"]
        assert 0 < earned["earned"] <= 3000 and earned["reached"] == new["complete"]
        assert initial["company"]["debt"] == final["company"]["debt"] == earned["debt"] == 5000000
        assert sum(row["amount"] for row in rows["expenses"]) == earned["net_expense"] == initial["company"]["cash"] - final["company"]["cash"]
        assert rows["readiness"] == [dict(industry=new["industry"], complete=new["complete"])] and not rows["invoices"]
    return dict(receipt=identity(receipt), foundation=identity(foundation_path), source=original, before=identity(before), output=identity(output),
                application=identity(application), terminal=identity(log), rows=rows, initial=initial, final=final,
                machinery_before=old, machinery=new, retained_harness=identity(retained),
                native_processes=[identity(path) for path in sorted(context.reports.glob("native-*.json"))],
                sources=[identity(Path(__file__)), identity(ROOT / "src/tests/surface_entry_propellant.cpp"),
                         identity(ROOT / "src/tests/surface_entry_work.h"), identity(Path(__file__).with_name("surface_income_ledger.py"))])


def main():
    """Retain a paid or genuinely earned partial checkpoint without claiming useful Propellant yet."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), required=True)
    parser.add_argument("--source-receipt", type=Path, required=True)
    parser.add_argument("--phase", choices=("fund", "build"), required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    receipt, output = [(ROOT / path).resolve() for path in (args.source_receipt, args.output)]
    assert receipt.is_relative_to(ROOT / "logs") and output.is_relative_to(ROOT / "logs") and not output.exists()
    operation = execute(args.configuration, receipt, args.phase)
    output.write_text(json.dumps(dict(_documentation=__doc__, complete=True, configuration=args.configuration, phase=args.phase, operation=operation), indent=2) + "\n", encoding="utf-8")
    print(json.dumps(dict(output=output.relative_to(ROOT).as_posix(), phase=args.phase, company=operation["final"]["company"],
                          industry=operation["machinery"]["industry"], ready=operation["machinery"]["complete"])), flush=True)


if __name__ == "__main__":
    main()
