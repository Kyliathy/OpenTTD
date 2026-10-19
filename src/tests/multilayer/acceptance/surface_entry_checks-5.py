"""Verify actual native construction credit and its ordinary financial restoration.

@file surface_entry_checks-5.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15BC.1.
The original real New Game source remains immutable. IncreaseLoan alone supplies
additional cash with matching debt; no cargo, terrain, industry or vehicle is granted.
"""

import argparse
import json
import os
from pathlib import Path
import re
import runpy
import subprocess
import sys

from space_preview_checks import ROOT, chunks, identity
from artifact_paths import create_run_context
from runner import run_native, console_executable

sys.path.insert(0, str(ROOT / "src/tests"))
from tbm_logistics_save_fixture import saved_table
BASELINE = runpy.run_path(str(Path(__file__).with_name("surface_entry_checks-3.py")))


def native(configuration, source):
    """Exercise original pure estimates, cap refusal and synchronized bank borrowing.

    @param configuration Current independently linked focused native executable.
    @param source Exact authentic100,000cash/debt original, never an already borrowed output.
    @return Original native bank process, physical outputs and scalar receipt.
    """
    context = create_run_context("space", "surface-entry-credit")
    context.prepare_runtime()
    (context.runtime_dir / "scripts/game_start.scr").write_text("surface_entry_credit\n", encoding="utf-8")
    application = ROOT / "build" / configuration / "openttd_surface_supply_test.exe"
    original = identity(source)
    log = context.logs / "credit.log"
    with log.open("wb") as sink:
        result = run_native(context, [str(application), "[.surface-entry-credit]"], cwd=ROOT / "build",
                            stdout=sink, stderr=subprocess.STDOUT, timeout=110,
                            env=dict(os.environ, OTTD_TEST_CONFIGURATION=configuration,
                                     OTTD_SURFACE_ENTRY_SOURCE=str(source)))
    terminal = log.read_text(errors="replace")
    assert result.returncode == 0 and "All tests passed" in terminal and "SURFACE_ENTRY_CREDIT_COMPLETE" in terminal, log
    assert identity(source) == original
    before = context.saves / "surface-entry-before-credit-440.sav"
    query = context.saves / "surface-entry-credit-query-440.sav"
    after = context.saves / "surface-entry-credit-440.sav"
    assert chunks(before) == chunks(query)
    first, second = chunks(before)[1], chunks(after)[1]
    assert first.keys() == second.keys()
    assert {key for key in first if first[key] != second[key]} == {b"PLYR"}
    companies, changed = saved_table(before, b"PLYR"), saved_table(after, b"PLYR")
    assert companies.keys() == changed.keys()
    # Only the two bank-owned scalar fields may change. Identical expenses,
    # receipts, stock and clocks rule out a grant disguised as normal credit.
    for company, fields in companies.items():
        assert fields["money"] == fields["current_loan"] == 100000
        for name, value in fields.items():
            assert changed[company][name] == (2000000 if name in ("money", "current_loan") else value), name
    assert not saved_table(after, b"CAPA")
    proof = dict(configuration=configuration, source=original, application=identity(application), terminal=identity(log),
                 snapshots=[identity(before), identity(query), identity(after)],
                 native_processes=[identity(p) for p in sorted(context.reports.glob("native-*.json"))],
                 unchanged_native_families=len(first) - 1, initial_cash=100000, borrowed=1900000,
                 final_cash=2000000, final_debt=2000000, expense=0)
    return after, proof


def restore(configuration, source):
    """Restore the actual bank output through the ordinary game without repeating credit.

    @param configuration Complete ordinary application.
    @param source Original actual paid-bank output from this same invocation.
    @return Exact ordinary process and preserved native financial ownership.
    """
    context = create_run_context("space", "surface-entry-credit-restore")
    context.prepare_runtime()
    application = ROOT / "build" / configuration / "openttd.exe"
    executable = console_executable(context, application)
    original = identity(source)
    output = context.saves / "surface-entry-credit-restored-440.sav"
    (context.runtime_dir / "scripts/game_start.scr").write_text(
        f'pause\nset savegame_format lzma\nsave "{output.with_suffix("").as_posix()}"\nquit\n', encoding="utf-8")
    log = context.logs / "restore.log"
    with log.open("wb") as sink:
        result = run_native(context, [str(executable), "-v", "null:ticks=2", "-s", "null", "-m", "null", "-x", "-X",
                            "-I", "original_dos", "-c", str(context.config_path), "-g", str(source)], cwd=context.runtime_dir,
                            stdout=sink, stderr=subprocess.STDOUT, timeout=110,
                            env=dict(os.environ, OTTD_TEST_CONFIGURATION=configuration))
    assert result.returncode == 0 and output.is_file(), log
    assert identity(source) == original
    before, after = chunks(source)[1], chunks(output)[1]
    assert before.keys() == after.keys()
    changed = {key for key in before if before[key] != after[key]}
    # The compiled test records its current revision on load. Returning to the
    # unchanged ordinary application may append its original same-genesis
    # revision; no financial/clock/stock owner may change with that metadata.
    if b"GLOG" in changed:
        BASELINE["verify_revision_log"](source, output)
    assert changed <= {b"GLOG", b"VIEW"}, changed
    company, = saved_table(output, b"PLYR").values()
    assert company["money"] == company["current_loan"] == 2000000
    assert not any(company["yearly_expenses"]) and company["cur_economy[0].income"] == 0
    assert not saved_table(output, b"CAPA")
    return dict(source=original, output=identity(output), application=identity(application), terminal=identity(log),
                money=company["money"], loan=company["current_loan"],
                unchanged_native_families=len(before) - len(changed),
                native_processes=[identity(p) for p in sorted(context.reports.glob("native-*.json"))])


def main():
    """Run one bounded build's native borrowing and ordinary restoration."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), required=True)
    parser.add_argument("--source", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    source, target = [(ROOT / item).resolve() for item in (args.source, args.output)]
    assert source.is_relative_to(ROOT) and source.is_file()
    assert target.is_relative_to(ROOT / "logs") and not target.exists()
    after, credit = native(args.configuration, source)
    restoration = restore(args.configuration, after)
    target.write_text(json.dumps(dict(_documentation=__doc__, complete=True, configuration=args.configuration,
                                     native=credit, ordinary=restoration,
                                     sources=[identity(Path(__file__)), identity(ROOT / "src/tests/surface_entry_credit.cpp"),
                                              identity(ROOT / "src/misc_cmd.cpp"), identity(ROOT / "src/company_cmd.cpp")],
                                     factory_funding_complete=False), indent=2) + "\n", encoding="utf-8")
    print(target.relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
