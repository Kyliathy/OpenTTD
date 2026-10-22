"""Bind real regional admission, original construction, empty purchase and measured service.

@file surface_entry_regional_checks-1.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15BIEQA.
Every claim follows actual original saves, commands, packets and paid books.
Forecast demand and fares never supply native purchase admission.
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
from surface_regional_ledger import TABLE, observe, reserve_identity, lives

DIRECTORY = Path(__file__).resolve().parent
FIELDS = runpy.run_path(str(DIRECTORY / "surface_entry_chemical_checks-2.py"))["fields"]
REVISION = runpy.run_path(str(DIRECTORY / "surface_entry_checks-7.py"))["known_revision"]
RESTORE_PAUSE = runpy.run_path(str(DIRECTORY / "surface_entry_fuel_checks-12.py"))["restore_pause"]
OLD_FUEL = runpy.run_path(str(DIRECTORY / "surface_entry_propellant_checks-8.py"))["fuel_identity"]
NEW_FUEL = runpy.run_path(str(DIRECTORY / "surface_entry_fuel_refill_checks-3.py"))["fuel_identity"]
GOODS = runpy.run_path(str(DIRECTORY / "surface_entry_goods_checks-1.py"))["goods_identity"]


def native_rng(before, output, draws):
    """@param before Original loaded DATE. @param output Paid same-tick DATE. @param draws Exact native station/body draws.
    Verify every clock, pause and RNG field; construction cannot supply elapsed service.
    """
    old, new = TABLE(before, b"DATE")[0], TABLE(output, b"DATE")[0]
    rng = {"random_state[0]", "random_state[1]"}
    assert {k: v for k, v in old.items() if k not in rng} == {k: v for k, v in new.items() if k not in rng}
    state, other = [old[f"random_state[{index}]"] for index in (0, 1)]
    # Only the actual paid station/body constructors draw these native values.
    for _ in range(draws):
        mixed = other ^ 0x1234567F
        state, other = (state + ((mixed >> 7) | (mixed << 25)) + 1) & 0xFFFFFFFF, (((state >> 3) | (state << 29)) - 1) & 0xFFFFFFFF
    assert [new[f"random_state[{index}]"] for index in (0, 1)] == [state, other]


def execute(configuration, phase, receipt, recover_run=None):
    """@param configuration Actual independently linked focused build.
    @param phase Pure quote, admitted infrastructure, empty purchase, annual service or short replay.
    @param receipt Exact immediate selected predecessor. @param recover_run Existing native run to inspect without rerunning.
    @return Sole-owner launch reserves, useful service, exact native books and process provenance.
    """
    assert phase in ("civic-query", "civic", "query", "infrastructure", "buy", "earn", "short")
    predecessor = json.loads(receipt.read_text(encoding="utf-8")); assert predecessor["complete"] and predecessor["configuration"] == configuration
    original = predecessor["operation"]["output"]; source = ROOT / original["path"]; assert identity(source) == original
    scenario = "surface-entry-regional-" + phase
    if recover_run is None:
        context = create_run_context("space", scenario); context.prepare_runtime()
        retained = context.directory / "harness.py"; retained.write_bytes(Path(__file__).read_bytes())
        (context.runtime_dir / "scripts/game_start.scr").write_text("surface_entry_regional\n", encoding="utf-8")
    else:
        context = RunContext(ROOT, "space", scenario, identifier(recover_run)); retained = context.directory / "harness.py"; assert retained.is_file()
    application = ROOT / "build" / configuration / "openttd_surface_supply_test.exe"; log = context.logs / "native.log"
    if recover_run is None:
        with log.open("wb") as sink:
            result = run_native(context, [str(application), "[.surface-entry-regional]"], cwd=ROOT / "build", stdout=sink,
                                stderr=subprocess.STDOUT, timeout=110, env=dict(os.environ, OTTD_TEST_CONFIGURATION=configuration,
                                OTTD_SURFACE_ENTRY_SOURCE=str(source), OTTD_SURFACE_REGIONAL_PHASE=phase))
        assert result.returncode == 0, log
    terminal = log.read_text(encoding="utf-8", errors="replace")
    assert "All tests passed" in terminal and f"SURFACE_REGIONAL_COMPLETE phase={phase}" in terminal, log
    before, output = [context.saves / f"surface-regional-{name}-440.sav" for name in ("before-" + phase, phase)]
    saved, first, last = [chunks(path)[1] for path in (source, before, output)]
    assert saved.keys() == first.keys() == last.keys() and len(first) == 121
    assert all(saved[key] == first[key] for key in saved if key not in (b"GLOG", b"DATE"))
    source_pause = None
    if saved[b"DATE"] != first[b"DATE"]:
        # Only the native due-worker restore may clear65to1; all other DATE,
        # RNG and actual tick fields remain exact at the original load boundary.
        assert TABLE(source, b"DATE")[0]["pause_mode"] == 65 and TABLE(before, b"DATE")[0]["pause_mode"] == 1
        source_pause = RESTORE_PAUSE(before, source)
    if saved[b"GLOG"] != first[b"GLOG"]:
        REVISION(source, before)
    assert first[b"GLOG"] == last[b"GLOG"]
    rows = {name: [FIELDS(line) for line in terminal.splitlines() if line.startswith("SURFACE_REGIONAL_" + token + " ")]
            for name, token in dict(civic="CIVIC", civic_paid="CIVIC_PAID", trees="TREE", stops="STOP", roads="ROAD", layout="LAYOUT", plan="PLAN", stations="STATION", invoices="INVOICE",
                                    infrastructure="INFRASTRUCTURE", buses="BUS", purchase="PURCHASE", earned="EARNED", expenses="EXPENSE",
                                    delivered="DELIVERED", profit="PROFIT", vehicle_profit="VEHICLE_PROFIT").items()}
    initial, final = observe(before), observe(output)
    assert reserve_identity(initial) == reserve_identity(final) and GOODS(initial) == GOODS(final)
    assert OLD_FUEL(initial) == OLD_FUEL(final) and NEW_FUEL(initial) == NEW_FUEL(final) and initial["plant"] == final["plant"]
    old_service, new_service = lives(before), lives(output)
    assert [(r["vehicle"], r["name"], r["engine"]) for r in old_service] == [(r["vehicle"], r["name"], r["engine"]) for r in new_service[:len(old_service)]]
    differences = [key.decode("ascii") for key in first if first[key] != last[key]]
    if phase in ("civic-query", "civic"):
        civic, = rows["civic"]; assert civic["cost"] == sum(row["cost"] for row in rows["trees"])
        assert civic["cash"] - civic["cost"] >= civic["reserve"] == 150000 and civic["credit"] == 0
        assert civic["trees"] == len(rows["trees"]) and civic["rating"] < civic["target"]
        if phase == "civic-query":
            assert first == last and initial == final and not rows["invoices"]
        else:
            paid, = rows["civic_paid"]; assert paid["cost"] == civic["cost"] == sum(row["cost"] for row in rows["invoices"])
            assert final["company"]["cash"] == initial["company"]["cash"] - paid["cost"] and initial["company"]["debt"] == final["company"]["debt"]
            assert TABLE(output, b"CITY")[0]["ratings"][0] == civic["rating"] + civic["trees"] * 7 >= civic["target"]
            assert first[b"DATE"] == last[b"DATE"] and first[b"VEHS"] == last[b"VEHS"] and first[b"CAPA"] == last[b"CAPA"]
            sites = {row["tile"] for row in rows["trees"]}
            for family, width in ((b"MAPT", 1), (b"MAPO", 1), (b"MAP2", 2), (b"M3LO", 1), (b"M3HI", 1), (b"MAP5", 1), (b"MAPE", 1), (b"MAP7", 1), (b"MAP8", 2)):
                old_map, new_map = first[family][3:], last[family][3:]; assert len(old_map) == len(new_map)
                assert {index // width for index, pair in enumerate(zip(old_map, new_map)) if pair[0] != pair[1]} <= sites, family
            assert set(differences) <= {"MAPT", "MAPO", "MAP2", "M3LO", "M3HI", "MAP5", "MAPE", "MAP7", "MAP8", "PLYR", "CITY"}
    elif phase == "query":
        assert first == last and initial == final and old_service == new_service and not rows["invoices"]
        plan, = rows["plan"]; layout, = rows["layout"]
        assert plan["gross"] == plan["infrastructure"] + 3 * plan["unit"] and plan["infrastructure"] == layout["infrastructure"]
        assert plan["cash"] + plan["credit"] - plan["gross"] >= plan["reserve"] == 150000
        assert plan["debt"] + plan["credit"] <= plan["ceiling"] == 5000000 and plan["projected_fares"] == 0
        assert layout["crossings"] == 0 and layout["depot"] == 2984 and layout["distance"] >= 20
        assert {row["town"] for row in rows["stops"]} == {0, 1} and all(row["new_houses"] > 0 for row in rows["stops"])
    elif phase in ("infrastructure", "buy"):
        invoice = sum(row["cost"] for row in rows["invoices"]); credit = final["company"]["debt"] - initial["company"]["debt"]
        assert final["company"]["cash"] == initial["company"]["cash"] + credit - invoice and final["company"]["cash"] >= 150000
        native_rng(before, output, 2 if phase == "infrastructure" else 3)
        assert first[b"CAPA"] == last[b"CAPA"] and all(first[key] == last[key] for key in (b"INDY", b"LPCR", b"LPCC", b"MAPH"))
        if phase == "infrastructure":
            assert first[b"VEHS"] == last[b"VEHS"] and old_service == new_service
            assert len(TABLE(output, b"STNN")) == len(TABLE(before, b"STNN")) + 2
            layout, = rows["layout"]; assert len(rows["stations"]) == 2
            sites = {row["tile"] for row in rows["roads"] + rows["stations"]}
            # All packed map bytes must stay within the exact admitted native
            # purchase sites. Other towns, freight and launch footprints survive.
            for family, width in ((b"MAPT", 1), (b"MAPO", 1), (b"MAP2", 2), (b"M3LO", 1), (b"M3HI", 1), (b"MAP5", 1), (b"MAPE", 1), (b"MAP7", 1), (b"MAP8", 2)):
                old_map, new_map = first[family][3:], last[family][3:]; assert len(old_map) == len(new_map)
                assert {index // width for index, pair in enumerate(zip(old_map, new_map)) if pair[0] != pair[1]} <= sites, family
            assert set(differences) <= {"MAPT", "MAPO", "MAP2", "M3LO", "M3HI", "MAP5", "MAPE", "MAP7", "MAP8", "PLYR", "CITY", "STNN", "ROAD", "DATE"}
            paid, = rows["infrastructure"]; assert paid["cost"] == invoice == layout["infrastructure"] and paid["credit"] == credit
        else:
            assert len(final["regional_buses"]) == 3 and len(new_service) == 11
            assert all(row["held"] == 0 and row["depot"] and not row["stopped"] and row["tile"] == 2984 and row["engine"] == 119 and row["capacity"] == 40 for row in final["regional_buses"])
            assert len(TABLE(output, b"VEHS")) == len(TABLE(before, b"VEHS")) + 3
            assert set(differences) <= {"VEHS", "ORDL", "PLYR", "DATE"}
            old_vehicles, new_vehicles = TABLE(before, b"VEHS"), TABLE(output, b"VEHS")
            assert all(new_vehicles[key] == row for key, row in old_vehicles.items())
            paid, = rows["purchase"]; assert paid["cost"] == invoice and paid["credit"] == credit
    else:
        earned, = rows["earned"]; profit, = rows["profit"]
        assert earned["earned"] == (27000 if phase == "earn" else 3000)
        assert TABLE(output, b"DATE")[0]["tick_counter"] - TABLE(before, b"DATE")[0]["tick_counter"] == earned["earned"]
        assert sum(row["amount"] for row in rows["expenses"]) == earned["net_expense"] == initial["company"]["cash"] - final["company"]["cash"]
        assert initial["company"]["debt"] == final["company"]["debt"] and not rows["invoices"]
        assert profit["fraction"] == 256 and profit["buses"] == len(rows["vehicle_profit"]) == 3
        assert sum(row["amount"] for row in rows["vehicle_profit"]) == profit["amount"]
    assert identity(source) == original and final["company"]["cash"] > 0 and 0 <= final["company"]["debt"] <= 5000000
    processes = [identity(path) for path in sorted(context.reports.glob("native-*.json"))]; assert processes
    for reference in processes:
        report = json.loads((ROOT / reference["path"]).read_text(encoding="utf-8"))
        assert report["exit_status"] == 0 and report["inputs"] == [original] and report["executable"] == identity(application)
    return dict(receipt=identity(receipt), source=original, before=identity(before), output=identity(output), application=identity(application), terminal=identity(log),
                rows=rows, initial=initial, final=final, service_lives=new_service, source_pause=source_pause, changed_families=differences,
                native_processes=processes, retained_harness=identity(retained),
                sources=[identity(Path(__file__)), identity(ROOT / "src/tests/surface_entry_regional.cpp"), identity(ROOT / "src/tests/surface_regional_layout.h"),
                         identity(ROOT / "src/tests/surface_entry_work.h"), identity(DIRECTORY / "surface_regional_ledger.py"),
                         identity(DIRECTORY / "surface_fuel_refill_ledger.py"), identity(DIRECTORY / "surface_propellant_ledger.py")])


def main():
    """Publish one focused regional boundary; recovery only observes an existing genuine native run."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), required=True)
    parser.add_argument("--phase", choices=("civic-query", "civic", "query", "infrastructure", "buy", "earn", "short"), required=True)
    parser.add_argument("--receipt", type=Path, required=True); parser.add_argument("--recover-run"); parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args(); output = (ROOT / args.output).resolve(); assert output.is_relative_to(ROOT / "logs") and not output.exists()
    operation = execute(args.configuration, args.phase, (ROOT / args.receipt).resolve(), args.recover_run)
    output.write_text(json.dumps(dict(_documentation=__doc__, complete=True, configuration=args.configuration, phase=args.phase, operation=operation), indent=2) + "\n", encoding="utf-8")
    print(output.relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
