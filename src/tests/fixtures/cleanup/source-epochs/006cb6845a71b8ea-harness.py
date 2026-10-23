"""Renew actual useful ore and Steel freight through original native owners.

@file surface_entry_road_renewal_checks-1.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15BIEL.
Native queries, real empty depot arrival, paid replacements and exact native
continuation protect all genuine36Steel/50Goods/42Propellant reserves.
"""
import argparse
import json
import os
from pathlib import Path
import runpy
import subprocess

from artifact_paths import RunContext, create_run_context, identifier
from runner import run_native
from space_preview_checks import ROOT, chunks, identity
from surface_fuel_refill_ledger import TABLE, observe, reserve_identity

DIRECTORY = Path(__file__).resolve().parent
FIELDS = runpy.run_path(str(DIRECTORY / "surface_entry_chemical_checks-2.py"))["fields"]
REVISION = runpy.run_path(str(DIRECTORY / "surface_entry_checks-7.py"))["known_revision"]
RESTORE_PAUSE = runpy.run_path(str(DIRECTORY / "surface_entry_fuel_checks-12.py"))["restore_pause"]
OLD_FUEL = runpy.run_path(str(DIRECTORY / "surface_entry_propellant_checks-8.py"))["fuel_identity"]
NEW_FUEL = runpy.run_path(str(DIRECTORY / "surface_entry_fuel_refill_checks-3.py"))["fuel_identity"]
GOODS = runpy.run_path(str(DIRECTORY / "surface_entry_goods_checks-1.py"))["goods_identity"]
PHASES = ("query", "recall", "work", "renew", "earn")


def execute(configuration, phase, receipt, recover_run=None):
    """@param configuration Independently linked current focused build.
    @param phase One complete native query, recall, work, renewal or replay boundary.
    @param receipt Exact immediate predecessor, excluding replay branches from selected funding.
    @param recover_run Successful existing native invocation; recovery never repeats work.
    @return Retained actual process, financial and whole-material evidence.
    """
    assert phase in PHASES
    predecessor = json.loads(receipt.read_text(encoding="utf-8")); assert predecessor["complete"] and predecessor["configuration"] == configuration
    original = predecessor["operation"]["output"]; source = ROOT / original["path"]; assert identity(source) == original
    scenario = "surface-entry-road-renewal-" + phase
    if recover_run is None:
        context = create_run_context("space", scenario); context.prepare_runtime()
        retained = context.directory / "harness.py"; retained.write_bytes(Path(__file__).read_bytes())
        (context.runtime_dir / "scripts/game_start.scr").write_text("surface_entry_road_renewal\n", encoding="utf-8")
    else:
        context = RunContext(ROOT, "space", scenario, identifier(recover_run)); retained = context.directory / "harness.py"; assert retained.is_file()
    application = ROOT / "build" / configuration / "openttd_surface_supply_test.exe"; log = context.logs / "native.log"
    if recover_run is None:
        with log.open("wb") as sink:
            result = run_native(context, [str(application), "[.surface-entry-road-renewal]"], cwd=ROOT / "build", stdout=sink,
                                stderr=subprocess.STDOUT, timeout=110, env=dict(os.environ, OTTD_TEST_CONFIGURATION=configuration,
                                OTTD_SURFACE_ENTRY_SOURCE=str(source), OTTD_SURFACE_ROAD_PHASE=phase))
        assert result.returncode == 0, log
    terminal = log.read_text(encoding="utf-8", errors="replace")
    assert "All tests passed" in terminal and f"SURFACE_ROAD_RENEWAL_COMPLETE phase={phase}" in terminal, log
    before, output = [context.saves / f"surface-road-{name}-440.sav" for name in ("before-" + phase, phase)]
    saved, first, last = [chunks(path)[1] for path in (source, before, output)]
    assert saved.keys() == first.keys() == last.keys() and len(first) == 121
    assert all(saved[key] == first[key] for key in saved if key not in (b"GLOG", b"DATE"))
    source_pause = None
    if saved[b"DATE"] != first[b"DATE"]:
        # A restored native due-worker job changes65to1only. Qualify that
        # owner transition without waiving any other DATE field or RNG seed.
        assert TABLE(source, b"DATE")[0]["pause_mode"] == 65 and TABLE(before, b"DATE")[0]["pause_mode"] == 1
        source_pause = RESTORE_PAUSE(before, source)
    if saved[b"GLOG"] != first[b"GLOG"]:
        REVISION(source, before)
    assert first[b"GLOG"] == last[b"GLOG"]
    rows = {name: [FIELDS(line) for line in terminal.splitlines() if line.startswith("SURFACE_ROAD_RENEWAL_" + prefix + " ")]
            for name, prefix in dict(plan="PLAN", models="MODEL", selected="SELECTED", recalls="RECALL_PLAN", invoices="INVOICE", renewed="RENEWED",
                                     custody="CUSTODY", earned="EARNED", expenses="EXPENSE", delivered="DELIVERED").items()}
    initial, final = observe(before), observe(output)
    assert reserve_identity(initial) == reserve_identity(final) and GOODS(initial) == GOODS(final)
    assert OLD_FUEL(initial) == OLD_FUEL(final) and NEW_FUEL(initial) == NEW_FUEL(final)
    assert initial["plant"] == final["plant"] and final["plant"]["produced"] == 42
    assert len(rows["custody"]) == 2 and all(row["capacity"] >= 18 and row["remaining"] > 0 for row in rows["custody"])
    assert initial["company"]["debt"] <= 5000000 and final["company"]["debt"] <= 5000000 and final["company"]["cash"] > 0
    old_date, new_date = TABLE(before, b"DATE")[0], TABLE(output, b"DATE")[0]
    if phase in ("work", "earn"):
        earned, = rows["earned"]; assert 0 <= earned["earned"] <= 3000
        assert new_date["tick_counter"] - old_date["tick_counter"] == earned["earned"]
        assert sum(row["amount"] for row in rows["expenses"]) == earned["net_expense"] == initial["company"]["cash"] - final["company"]["cash"]
        assert initial["company"]["debt"] == final["company"]["debt"] and not rows["invoices"]
        if earned["reached"]:
            assert phase == "work" and all(row["held"] == 0 and row["home"] and row["tile"] == 2443 for row in rows["custody"])
        if phase == "earn":
            assert earned["earned"] == 3000 and all(row["orders"] == 2 for row in rows["custody"])
    else:
        assert first[b"CAPA"] == last[b"CAPA"]
        assert all(first[key] == last[key] for key in (b"INDY", b"LPCR", b"LPCC", b"MAPT", b"MAPH"))
        invoice = sum(row["cost"] for row in rows["invoices"])
        credit = final["company"]["debt"] - initial["company"]["debt"]
        assert final["company"]["cash"] == initial["company"]["cash"] + credit - invoice
        if phase == "renew":
            plan, = rows["plan"]; assert credit == plan["credit"] and final["company"]["cash"] >= 150000
            assert len(rows["renewed"]) == 2 and all(row["empty"] and row["age"] == 0 and row["orders"] == 2 for row in rows["renewed"])
            assert len(TABLE(before, b"VEHS")) == len(TABLE(output, b"VEHS"))
            rng = {"random_state[0]", "random_state[1]"}
            assert {k: v for k, v in old_date.items() if k not in rng} == {k: v for k, v in new_date.items() if k not in rng}
            state, other = [old_date[f"random_state[{index}]"] for index in (0, 1)]
            # Exactly the two original BuildRoadVehicle calls draw RNG;
            # order copies, native empty sales and names create no game ticks.
            for _ in range(2):
                mixed = other ^ 0x1234567F
                state, other = (state + ((mixed >> 7) | (mixed << 25)) + 1) & 0xFFFFFFFF, (((state >> 3) | (state << 29)) - 1) & 0xFFFFFFFF
            assert [new_date[f"random_state[{index}]"] for index in (0, 1)] == [state, other]
        else:
            assert old_date == new_date and credit == invoice == 0
        if phase == "query":
            assert first == last and initial == final
            plan, = rows["plan"]; assert plan["trucks"] == 2 and plan["gross"] == sum(row["unit"] for row in rows["selected"])
            assert plan["cash"] + plan["credit"] - plan["gross"] >= plan["reserve"] == 150000
            assert plan["future_refund"] == 0 and len(rows["recalls"]) == 2
    # Original useful railway vehicles and wagon chains survive renewal;
    # only genuine work may change their pose, loading or ordinary age.
    assert [(row["head"], row["chain"]) for row in initial["trains"]] == [(row["head"], row["chain"]) for row in final["trains"]]
    processes = [identity(path) for path in sorted(context.reports.glob("native-*.json"))]; assert processes
    for reference in processes:
        report = json.loads((ROOT / reference["path"]).read_text(encoding="utf-8"))
        assert report["exit_status"] == 0 and report["inputs"] == [original] and report["executable"] == identity(application)
    assert identity(source) == original
    return dict(receipt=identity(receipt), source=original, before=identity(before), output=identity(output), application=identity(application),
                terminal=identity(log), rows=rows, initial=initial, final=final, source_pause=source_pause, retained_harness=identity(retained),
                native_processes=processes, sources=[identity(Path(__file__)), identity(ROOT / "src/tests/surface_entry_road_renewal.cpp"),
                identity(ROOT / "src/tests/surface_entry_work.h"), identity(DIRECTORY / "surface_fuel_refill_ledger.py"), identity(DIRECTORY / "surface_propellant_ledger.py")])


def main():
    """Retain one sequential original boundary without overwriting successful evidence."""
    parser = argparse.ArgumentParser(description=__doc__); parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), required=True)
    parser.add_argument("--phase", choices=PHASES, required=True); parser.add_argument("--receipt", type=Path, required=True)
    parser.add_argument("--recover-run"); parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args(); output = (ROOT / args.output).resolve(); assert output.is_relative_to(ROOT / "logs") and not output.exists()
    operation = execute(args.configuration, args.phase, (ROOT / args.receipt).resolve(), args.recover_run)
    output.write_text(json.dumps(dict(_documentation=__doc__, complete=True, configuration=args.configuration, phase=args.phase, operation=operation), indent=2) + "\n", encoding="utf-8")
    print(output.relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
