"""Quote, buy and genuinely collect additional literal Surface-factory Goods.

@file surface_entry_goods_checks-1.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15BIEF.
Original native factory, packets, vehicles, company and simulation own every
quantity, payment and tick. Read-only recovery never repeats a paid command.
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
from surface_propellant_ledger import TABLE, observe, reserve_identity

FIELDS = runpy.run_path(str(Path(__file__).with_name("surface_entry_chemical_checks-2.py")))["fields"]
REVISION = runpy.run_path(str(Path(__file__).with_name("surface_entry_checks-7.py")))["known_revision"]
FUEL_IDENTITY = runpy.run_path(str(Path(__file__).with_name("surface_entry_propellant_checks-8.py")))["fuel_identity"]
RESTORE_PAUSE = runpy.run_path(str(Path(__file__).with_name("surface_entry_fuel_checks-12.py")))["restore_pause"]


def goods_identity(snapshot):
    """@param snapshot Actual native observed world. @return Additional real Goods identities excluding naturally accrued packet age."""
    result = []
    for truck in snapshot["additional_goods"]:
        row = dict(truck)
        row["packets"] = [{key: value for key, value in packet.items() if key != "periods_in_transit"} for packet in row["packets"]]
        result.append(row)
    return result


def execute(configuration, phase, receipt, recover_run=None):
    """@param configuration Actual independently linked focused native build.
    @param phase Original quote, paid empty purchase, bounded collection or exact replay.
    @param receipt Exact immediate selected predecessor receipt.
    @param recover_run Successful original invocation to inspect without executing again.
    @return Complete original process, material, finance and gameplay evidence.
    """
    predecessor = json.loads(receipt.read_text(encoding="utf-8"))
    assert predecessor["complete"] and predecessor["configuration"] == configuration
    original = predecessor["operation"]["output"]
    source = ROOT / original["path"]; assert identity(source) == original
    scenario = "surface-entry-goods-" + phase
    if recover_run is None:
        context = create_run_context("space", scenario); context.prepare_runtime()
        retained = context.directory / "harness.py"; retained.write_bytes(Path(__file__).read_bytes())
        (context.runtime_dir / "scripts/game_start.scr").write_text("surface_entry_goods\n", encoding="utf-8")
    else:
        context = RunContext(ROOT, "space", scenario, identifier(recover_run)); retained = context.directory / "harness.py"
        assert retained.is_file()
    application = ROOT / "build" / configuration / "openttd_surface_supply_test.exe"
    log = context.logs / "native.log"
    if recover_run is None:
        with log.open("wb") as sink:
            result = run_native(context, [str(application), "[.surface-entry-goods]"], cwd=ROOT / "build", stdout=sink,
                                stderr=subprocess.STDOUT, timeout=110, env=dict(os.environ, OTTD_TEST_CONFIGURATION=configuration,
                                OTTD_SURFACE_ENTRY_SOURCE=str(source), OTTD_SURFACE_GOODS_PHASE=phase))
        assert result.returncode == 0, log
    terminal = log.read_text(encoding="utf-8", errors="replace")
    assert "All tests passed" in terminal and f"SURFACE_GOODS_COMPLETE phase={phase}" in terminal, log
    assert identity(source) == original
    before, output = [context.saves / f"surface-goods-{name}-440.sav" for name in ("before-" + phase, phase)]
    saved, first, final = [chunks(path)[1] for path in (source, before, output)]
    assert saved.keys() == first.keys() == final.keys() and len(first) == 121
    assert all(saved[key] == first[key] for key in saved if key not in (b"GLOG", b"DATE"))
    source_pause = None
    if saved[b"DATE"] != first[b"DATE"]:
        # A restarted actual due-worker job may account for the sole65to1
        # pause transition. Every other original clock/RNG field must match.
        assert TABLE(source, b"DATE")[0]["pause_mode"] == 65 and TABLE(before, b"DATE")[0]["pause_mode"] == 1
        source_pause = RESTORE_PAUSE(before, source)
    if saved[b"GLOG"] != first[b"GLOG"]:
        REVISION(source, before)
    assert first[b"GLOG"] == final[b"GLOG"]
    rows = {name: [FIELDS(line) for line in terminal.splitlines() if line.startswith("SURFACE_GOODS_" + prefix + " ")]
            for name, prefix in dict(plan="PLAN", route="ROUTE", invoices="INVOICE", trucks="TRUCK", custody="CUSTODY",
                                     earned="EARNED", expenses="EXPENSE", delivered="DELIVERED").items()}
    initial, observed = observe(before), observe(output)
    assert reserve_identity(initial) == reserve_identity(observed) and FUEL_IDENTITY(initial) == FUEL_IDENTITY(observed)
    assert initial["plant"] == observed["plant"]
    if phase == "query":
        assert first == final and initial == observed and not observed["additional_goods"]
    if phase in ("query", "buy"):
        plan, = rows["plan"]; route, = rows["route"]
        assert route["source"] == 11 and route["crossings"] == route["infrastructure"] == 0 and route["catchment"]
        assert plan["carriers"] * plan["capacity"] == plan["useful"] >= 34 and plan["minimum"] == 34
        assert plan["gross"] == plan["unit"] * plan["carriers"] and plan["reserve"] == 100000
        assert plan["credit"] % 10000 == 0 and plan["cash"] + plan["credit"] - plan["gross"] >= 100000
        assert plan["debt"] + plan["credit"] <= plan["ceiling"] == 5000000 and not plan["unsold_value_counted"]
        assert plan["orders_admitted"] and plan["full_load"]
    if phase == "buy":
        assert not initial["additional_goods"] and len(observed["additional_goods"]) == plan["carriers"]
        assert len(rows["trucks"]) == plan["carriers"] and all(row["held"] == 0 and row["orders"] == 2 for row in rows["trucks"])
        assert all(row["held"] == 0 and row["capacity"] == plan["capacity"] and row["engine"] == plan["model"] for row in observed["additional_goods"])
        assert observed["company"]["cash"] == initial["company"]["cash"] + plan["credit"] - plan["gross"] >= 100000
        assert observed["company"]["debt"] == initial["company"]["debt"] + plan["credit"]
        assert sum(row["cost"] for row in rows["invoices"]) == plan["gross"]
        assert len(TABLE(output, b"VEHS")) - len(TABLE(before, b"VEHS")) == plan["carriers"]
        assert all(first[key] == final[key] for key in (b"CAPA", b"STNN", b"INDY", b"LPCR", b"LPCC", b"MAPH", b"MAPT"))
        old_date, new_date = TABLE(before, b"DATE")[0], TABLE(output, b"DATE")[0]; rng = {"random_state[0]", "random_state[1]"}
        assert {k: v for k, v in old_date.items() if k not in rng} == {k: v for k, v in new_date.items() if k not in rng}
        state, other = [old_date[f"random_state[{index}]"] for index in (0, 1)]
        # Each native original BuildRoadVehicle body draws exactly one
        # Randomizer value; no broad DATE/RNG exception is accepted.
        for _ in range(plan["carriers"]):
            mixed = other ^ 0x1234567F
            state, other = (state + ((mixed >> 7) | (mixed << 25)) + 1) & 0xFFFFFFFF, (((state >> 3) | (state << 29)) - 1) & 0xFFFFFFFF
        assert [new_date[f"random_state[{index}]"] for index in (0, 1)] == [state, other]
    if phase in ("collect", "earn"):
        earned, = rows["earned"]
        assert 0 <= earned["earned"] <= 3000
        assert TABLE(output, b"DATE")[0]["tick_counter"] - TABLE(before, b"DATE")[0]["tick_counter"] == earned["earned"]
        assert sum(row["amount"] for row in rows["expenses"]) == earned["net_expense"] == initial["company"]["cash"] - observed["company"]["cash"]
        assert initial["company"]["debt"] == observed["company"]["debt"] <= 5000000
        assert len(initial["additional_goods"]) == len(observed["additional_goods"])
        if phase == "earn":
            assert earned["earned"] == 3000 and goods_identity(initial) == goods_identity(observed)
        if earned["reached"] or phase == "earn":
            assert sum(row["held"] for row in observed["additional_goods"]) >= 34
            assert all(row["held"] == row["capacity"] and row["tile"] == 2443 and row["stopped"] and row["depot"] for row in observed["additional_goods"])
    assert observed["company"]["cash"] > 0
    processes = [identity(path) for path in sorted(context.reports.glob("native-*.json"))]; assert processes
    for reference in processes:
        report = json.loads((ROOT / reference["path"]).read_text(encoding="utf-8"))
        assert report["exit_status"] == 0 and report["inputs"] == [original] and report["executable"] == identity(application)
    return dict(receipt=identity(receipt), source=original, before=identity(before), output=identity(output), application=identity(application),
                terminal=identity(log), rows=rows, initial=initial, final=observed, source_pause=source_pause,
                changed_families=[key.decode("ascii") for key in first if first[key] != final[key]], retained_harness=identity(retained),
                native_processes=processes, sources=[identity(Path(__file__)), identity(ROOT / "src/tests/surface_entry_goods.cpp"),
                identity(ROOT / "src/tests/surface_entry_work.h"), identity(Path(__file__).with_name("surface_propellant_ledger.py"))])


def main():
    """Retain one selected original boundary without replacing an earlier successful output."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), required=True)
    parser.add_argument("--phase", choices=("query", "buy", "collect", "earn"), required=True)
    parser.add_argument("--receipt", type=Path, required=True); parser.add_argument("--recover-run")
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args(); output = (ROOT / args.output).resolve()
    assert output.is_relative_to(ROOT / "logs") and not output.exists()
    operation = execute(args.configuration, args.phase, (ROOT / args.receipt).resolve(), args.recover_run)
    output.write_text(json.dumps(dict(_documentation=__doc__, complete=True, configuration=args.configuration,
                                     phase=args.phase, operation=operation), indent=2) + "\n", encoding="utf-8")
    print(output.relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
