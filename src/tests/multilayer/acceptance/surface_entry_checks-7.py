"""Verify one bounded actual funding, ordinary construction or readiness process.

@file surface_entry_checks-7.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15BC.2/.3.
Each invocation owns exactly one native process. Products, original sources and
receipts stay explicit; construction never receives supplied progress or cargo.
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
from runner import run_native

sys.path.insert(0, str(ROOT / "src/tests"))
from tbm_logistics_save_fixture import saved_table
ORDINARY = runpy.run_path(str(Path(__file__).with_name("surface_supply_delivery_checks-2.py")))


def known_revision(source, output):
    """Admit a single load revision already recorded in this same game's history.

    @param source Exact ordinary construction checkpoint.
    @param output Focused observer's paused save with its known compiled revision.
    """
    before, after = saved_table(source, b"GLOG"), saved_table(output, b"GLOG")
    assert set(after) == set(before) | {len(before)}
    for key, value in before.items():
        assert after[key] == value
    appended = after[len(before)]
    assert appended["at"] == 1 and appended["action.count"] == 1 and appended["action[0].ct"] == 1
    assert appended["tick"] == saved_table(source, b"DATE")[0]["tick_counter"]
    # The same focused application funded these factories earlier. Its exact
    # original revision record must already exist; arbitrary settings/cheat or
    # new revision rows cannot hide behind a general GLOG exception.
    expected = {key: value for key, value in appended.items() if key != "tick"}
    assert any({key: value for key, value in row.items() if key != "tick"} == expected for row in before.values())


def empty_outputs(path, selected):
    """Independently inspect exact original industry stocks and complete production histories.

    @param path Actual native save, never an invented inventory snapshot.
    @param selected Original newly funded industry IDs.
    @return Exact selected native industry fields.
    """
    industries = saved_table(path, b"INDY")
    result = {key: industries[key] for key in selected}
    # Empty output histories prevent a supplied or silently produced shipment
    # from being passed off as later physical ore/Steel/Goods transport.
    for fields in result.values():
        for name, value in fields.items():
            if name.endswith((".waiting", ".production", ".transported")):
                assert value == 0, (name, value)
        assert fields["founder"] == 0
    assert not saved_table(path, b"CAPA")
    return result


def native(configuration, source, ready):
    """Execute ordinary native funding or a read-only completed-factory observation.

    @param configuration Independently linked focused application.
    @param source Exact bank output or actual ordinary construction checkpoint.
    @param ready True requires earned completion and performs no funding.
    @return Original native outputs, scalar receipts and process membership.
    """
    context = create_run_context("space", "surface-entry-factories")
    context.prepare_runtime()
    (context.runtime_dir / "scripts/game_start.scr").write_text("surface_entry_factories\n", encoding="utf-8")
    application = ROOT / "build" / configuration / "openttd_surface_supply_test.exe"
    original = identity(source)
    log = context.logs / "factories.log"
    environment = dict(os.environ, OTTD_TEST_CONFIGURATION=configuration, OTTD_SURFACE_ENTRY_SOURCE=str(source))
    if ready:
        environment["OTTD_SURFACE_ENTRY_READY"] = "true"
    with log.open("wb") as sink:
        result = run_native(context, [str(application), "[.surface-entry-factories]"], cwd=ROOT / "build",
                            stdout=sink, stderr=subprocess.STDOUT, timeout=110, env=environment)
    terminal = log.read_text(errors="replace")
    assert result.returncode == 0 and "All tests passed" in terminal and "SURFACE_ENTRY_FACTORIES_COMPLETE" in terminal, log
    assert identity(source) == original
    output = context.saves / "surface-entry-factories-observed-440.sav"
    selected = {key for key, value in saved_table(output, b"INDY").items() if value["founder"] == 0}
    assert len(selected) == 2
    fields = empty_outputs(output, selected)
    invoices = []
    if not ready:
        before = context.saves / "surface-entry-before-factories-440.sav"
        previous = before
        # Every quote is pure against its own immediately preceding world;
        # every actual invoice creates exactly one new industry and debits the
        # original company once. Existing generated industries remain exact.
        for index in range(2):
            query = context.saves / f"surface-entry-factory-{index}-query-440.sav"
            paid = context.saves / f"surface-entry-factory-{index}-paid-440.sav"
            assert chunks(previous) == chunks(query)
            old, new = saved_table(query, b"INDY"), saved_table(paid, b"INDY")
            assert len(new) == len(old) + 1
            for key, value in old.items():
                assert new[key] == value, key
            company_before, = saved_table(query, b"PLYR").values()
            company_after, = saved_table(paid, b"PLYR").values()
            invoice = company_before["money"] - company_after["money"]
            assert invoice > 0 and company_after["current_loan"] == company_before["current_loan"] == 2000000
            expenses = sum(company_after["yearly_expenses"]) - sum(company_before["yearly_expenses"])
            assert expenses == invoice
            date_before, = saved_table(query, b"DATE").values()
            date_after, = saved_table(paid, b"DATE").values()
            # Real industry creation draws native appearance/animation RNG;
            # construction commands still earn no clock or tile-loop work.
            for name, value in date_before.items():
                if not name.startswith("random_state["):
                    assert date_after[name] == value, name
            invoices.append(dict(index=index, industry=next(iter(new.keys() - old.keys())), invoice=invoice))
            previous = paid
        assert chunks(previous) == chunks(output)
    else:
        before, after = chunks(source)[1], chunks(output)[1]
        # Re-entering the focused observer can append its compiled revision.
        # Its paused checkpoint changes only DATE.pause_mode, never earns a
        # tick. Protect every other DATE field, all cargo, finance and tiles.
        assert {key for key in before if before[key] != after[key]} <= {b"GLOG", b"VIEW", b"DATE"}
        if before[b"GLOG"] != after[b"GLOG"]:
            known_revision(source, output)
        date_before, = saved_table(source, b"DATE").values()
        date_after, = saved_table(output, b"DATE").values()
        assert date_before["pause_mode"] == 0 and date_after["pause_mode"] == 1
        for name, value in date_before.items():
            if name != "pause_mode":
                assert date_after[name] == value, name
    return dict(source=original, output=identity(output), application=identity(application), terminal=identity(log),
                snapshots=[identity(p) for p in sorted(context.saves.glob("*.sav"))],
                factories=fields, invoices=invoices,
                native_processes=[identity(p) for p in sorted(context.reports.glob("native-*.json"))])


def advance(configuration, source):
    """Earn a bounded construction interval through the unchanged ordinary game loop.

    @param configuration Complete ordinary application, with no compiled test hooks.
    @param source Actual incomplete factory checkpoint.
    @return Exact native process, clocks and ordinary financial ledger.
    """
    output, proof = ORDINARY["ordinary"](configuration, source, "surface-entry-construction", True)
    initial, = saved_table(source, b"DATE").values()
    final, = saved_table(output, b"DATE").values()
    assert 0 < final["tick_counter"] - initial["tick_counter"] <= 3000
    assert final["id"] == initial["id"]
    selected = {key for key, value in saved_table(source, b"INDY").items() if value["founder"] == 0}
    fields = empty_outputs(output, selected)
    old, = saved_table(source, b"PLYR").values()
    new, = saved_table(output, b"PLYR").values()
    assert old["current_loan"] == new["current_loan"] == 2000000
    assert old["money"] - new["money"] == sum(new["yearly_expenses"]) - sum(old["yearly_expenses"])
    assert new["cur_economy[0].income"] == 0
    # Native founder/date and selected footprints survive normal construction;
    # monthly animation/counters/history bookkeeping may legitimately advance.
    for key in selected:
        original = saved_table(source, b"INDY")[key]
        for name in ("location.tile", "location.w", "location.h", "type", "founder", "construction_date", "construction_type"):
            assert fields[key][name] == original[name], name
    proof.update(ticks=final["tick_counter"] - initial["tick_counter"], cash=new["money"], debt=new["current_loan"],
                 initial_date=initial["date"], final_date=final["date"], factories=fields)
    return proof


def main():
    """Run exactly one selected bounded process and retain fresh explicit membership."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), required=True)
    parser.add_argument("--phase", choices=("fund", "advance", "ready"), required=True)
    parser.add_argument("--source", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    source, target = [(ROOT / item).resolve() for item in (args.source, args.output)]
    assert source.is_relative_to(ROOT) and source.is_file()
    assert target.is_relative_to(ROOT / "logs") and not target.exists()
    operation = advance(args.configuration, source) if args.phase == "advance" else native(args.configuration, source, args.phase == "ready")
    target.write_text(json.dumps(dict(_documentation=__doc__, complete=True, configuration=args.configuration,
                                     phase=args.phase, operation=operation,
                                     sources=[identity(Path(__file__)), identity(ROOT / "src/tests/surface_entry_factories.cpp")]), indent=2) + "\n", encoding="utf-8")
    print(target.relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
