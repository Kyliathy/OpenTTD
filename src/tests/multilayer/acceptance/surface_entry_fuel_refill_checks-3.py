"""Execute one selected original additional Propellant conversion and collection boundary.

@file surface_entry_fuel_refill_checks-3.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15BIEH.2-3.
Native commands, game ticks and original financial/material owners establish
every result. Read-only recovery never repeats a successful purchase or trip.
"""
import argparse
from collections import Counter
import json
import os
from pathlib import Path
import runpy
import re
import subprocess

from artifact_paths import RunContext, create_run_context, identifier
from runner import run_native
from space_preview_checks import ROOT, chunks, identity
from surface_fuel_refill_ledger import TABLE, observe, reserve_identity

FIELDS = runpy.run_path(str(Path(__file__).with_name("surface_entry_chemical_checks-2.py")))["fields"]
REVISION = runpy.run_path(str(Path(__file__).with_name("surface_entry_checks-7.py")))["known_revision"]
RESTORE_PAUSE = runpy.run_path(str(Path(__file__).with_name("surface_entry_fuel_checks-12.py")))["restore_pause"]
FUEL_IDENTITY = runpy.run_path(str(Path(__file__).with_name("surface_entry_propellant_checks-8.py")))["fuel_identity"]
GOODS_IDENTITY = runpy.run_path(str(Path(__file__).with_name("surface_entry_goods_checks-1.py")))["goods_identity"]
PHASES = ("recall", "traffic-work", "chemical-order", "chemical-work", "carrier", "collect-order", "collect-work", "resume", "earn")


def chemical_identity(snapshot):
    """@param snapshot Actual explicit refill world. @return New chemical cargo identity excluding natural packet age."""
    result = dict(snapshot["chemical_feed"])
    result["packets"] = [{key: value for key, value in packet.items() if key != "periods_in_transit"} for packet in result["packets"]]
    return result


def recall_reservations(before, output):
    """Qualify original LeaveStation reservation splits without waiving cargo conservation.

    @param before Actual commercial loading poses before the original recall.
    @param output Actual same-tick original SkipToOrder output.
    @return Quantities whose native reservation split returned them to station custody.
    """
    def cargo_slots(path):
        """@param path Actual complete native save. @return Sole-container native cargo meaning for every packet."""
        result = {}
        # Cargo type belongs to the native container, not CAPA. Follow actual
        # references so two Farm commodities cannot substitute for each other.
        for station in TABLE(path, b"STNN").values():
            for name, references in station.items():
                match = re.fullmatch(r"normal\[0\]\.goods\[(\d+)\]\.cargo\[\d+\]\.second", name)
                if match:
                    for reference in references:
                        assert reference - 1 not in result; result[reference - 1] = int(match.group(1))
        for vehicle in TABLE(path, b"VEHS").values():
            if vehicle["type"] not in (0, 1):
                continue
            prefix = "train[0].common[0]." if vehicle["type"] == 0 else "roadveh[0].common[0]."
            for reference in vehicle[prefix + "cargo.packets"]:
                assert reference - 1 not in result; result[reference - 1] = vehicle[prefix + "cargo_type"]
        assert result.keys() == TABLE(path, b"CAPA").keys(); return result

    old, new = TABLE(before, b"CAPA"), TABLE(output, b"CAPA")
    assert old.keys() <= new.keys()
    for key, row in old.items():
        if row == new[key]:
            continue
        # Original Farm/Goods/Coal commercial loading reservations alone may split
        # here. Existing retained packet identity/travel remains exact; every
        # released slice is accounted by matching source/cargo/age/share below.
        assert row["source_type"] == 0 and row["source_id"] in (0, 5, 7)
        assert {k: v for k, v in row.items() if k not in ("count", "feeder_share")} == {k: v for k, v in new[key].items() if k not in ("count", "feeder_share")}
        assert 0 < new[key]["count"] <= row["count"]
    released = 0
    for key in new.keys() - old.keys():
        row = new[key]; assert row["source_type"] == 0 and row["source_id"] in (0, 5, 7)
        assert row["travelled.x"] == row["travelled.y"] == 0; released += row["count"]
    totals = []
    for path, packets in ((before, old), (output, new)):
        slots = cargo_slots(path); quantities, shares = Counter(), Counter()
        # Split/Merge may change packet IDs and reset only a returned slice's
        # travel. Preserve each cargo/source/origin/age/loaded-at group and its
        # complete feeder share, rather than accepting a company-wide sum.
        for key, row in packets.items():
            group = (slots[key],) + tuple((name, value) for name, value in sorted(row.items()) if name not in ("count", "feeder_share", "travelled.x", "travelled.y"))
            quantities[group] += row["count"]; shares[group] += row["feeder_share"]
        totals.append((quantities, shares))
    assert totals[0] == totals[1]; return released


def fuel_identity(snapshot):
    """@param snapshot Actual native world. @return Additional fuel carrier identity excluding naturally accrued packet age."""
    truck = snapshot["propellant_refill"]["truck"]
    if truck is None:
        return None
    result = dict(truck)
    result["packets"] = [{key: value for key, value in packet.items() if key != "periods_in_transit"} for packet in result["packets"]]
    return result


def execute(configuration, phase, receipt, recover_run=None):
    """@param configuration Actual independently linked focused native build.
    @param phase One published original purchase, order, movement, settlement or replay boundary.
    @param receipt Exact immediate selected original predecessor.
    @param recover_run Successful existing native invocation to inspect without executing again.
    @return Complete actual native lineage, financial settlement and sole material custody.
    """
    assert phase in PHASES
    predecessor = json.loads(receipt.read_text(encoding="utf-8")); assert predecessor["complete"] and predecessor["configuration"] == configuration
    original = predecessor["operation"]["output"]; source = ROOT / original["path"]; assert identity(source) == original
    scenario = "surface-entry-fuel-refill-" + phase
    if recover_run is None:
        context = create_run_context("space", scenario); context.prepare_runtime()
        retained = context.directory / "harness.py"; retained.write_bytes(Path(__file__).read_bytes())
        (context.runtime_dir / "scripts/game_start.scr").write_text("surface_entry_fuel_refill\n", encoding="utf-8")
    else:
        context = RunContext(ROOT, "space", scenario, identifier(recover_run)); retained = context.directory / "harness.py"; assert retained.is_file()
    application = ROOT / "build" / configuration / "openttd_surface_supply_test.exe"; log = context.logs / "native.log"
    if recover_run is None:
        with log.open("wb") as sink:
            result = run_native(context, [str(application), "[.surface-entry-fuel-refill]"], cwd=ROOT / "build", stdout=sink,
                                stderr=subprocess.STDOUT, timeout=110, env=dict(os.environ, OTTD_TEST_CONFIGURATION=configuration,
                                OTTD_SURFACE_ENTRY_SOURCE=str(source), OTTD_SURFACE_FUEL_REFILL_PHASE=phase))
        assert result.returncode == 0, log
    terminal = log.read_text(encoding="utf-8", errors="replace")
    assert "All tests passed" in terminal and f"SURFACE_FUEL_REFILL_COMPLETE phase={phase}" in terminal, log
    before, output = [context.saves / f"surface-fuel-refill-{name}-440.sav" for name in ("before-" + phase, phase)]
    saved, first, final = [chunks(path)[1] for path in (source, before, output)]
    assert saved.keys() == first.keys() == final.keys() and len(first) == 121
    assert all(saved[key] == first[key] for key in saved if key not in (b"GLOG", b"DATE"))
    source_pause = None
    if saved[b"DATE"] != first[b"DATE"]:
        # A native due-worker restart owns only the original65to1pause
        # transition. Every other DATE/RNG/tick field must remain exact.
        assert TABLE(source, b"DATE")[0]["pause_mode"] == 65 and TABLE(before, b"DATE")[0]["pause_mode"] == 1
        source_pause = RESTORE_PAUSE(before, source)
    if saved[b"GLOG"] != first[b"GLOG"]:
        REVISION(source, before)
    assert first[b"GLOG"] == final[b"GLOG"]
    rows = {name: [FIELDS(line) for line in terminal.splitlines() if line.startswith("SURFACE_FUEL_REFILL_" + prefix + " ")]
            for name, prefix in dict(invoices="INVOICE", recall="RECALL", carrier="CARRIER", resumed="RESUMED", road_orders="ROAD_ORDER", custody="CUSTODY",
                                     earned="EARNED", expenses="EXPENSE", delivered="DELIVERED").items()}
    initial, observed = observe(before), observe(output)
    assert reserve_identity(initial) == reserve_identity(observed) and GOODS_IDENTITY(initial) == GOODS_IDENTITY(observed) and FUEL_IDENTITY(initial) == FUEL_IDENTITY(observed)
    old_refill, new_refill = initial["propellant_refill"], observed["propellant_refill"]
    assert observed["plant"]["consumed"] >= initial["plant"]["consumed"]
    assert new_refill["new_produced"] >= old_refill["new_produced"] and new_refill["new_dispatched"] >= old_refill["new_dispatched"]
    assert new_refill["ordinary_loss"] >= old_refill["ordinary_loss"]
    work = phase.endswith("-work") or phase == "earn"
    if work:
        earned, = rows["earned"]; assert 0 <= earned["earned"] <= 3000
        assert TABLE(output, b"DATE")[0]["tick_counter"] - TABLE(before, b"DATE")[0]["tick_counter"] == earned["earned"]
        assert sum(row["amount"] for row in rows["expenses"]) == earned["net_expense"] == initial["company"]["cash"] - observed["company"]["cash"]
        assert initial["company"]["debt"] == observed["company"]["debt"] <= 5000000
        # Only actual native completion enables the stronger physical goal.
        # Partial work retains actual progress without claiming a protected
        # crossing, settled conversion or useful depot fuel prematurely.
        if phase == "traffic-work" and earned["reached"]:
            assert observed["traffic_halted"]
        if phase == "chemical-work" and earned["reached"]:
            feed = observed["chemical_feed"]
            assert feed["held"] == 0 and feed["stopped"] and feed["depot"] and feed["tile"] == 2443
            assert observed["plant"]["consumed"] == observed["plant"]["produced"] == 42 and observed["plant"]["chemicals"] == 0
        if phase == "collect-work" and earned["reached"]:
            truck = new_refill["truck"]; assert truck is not None
            assert truck["held"] >= 10 and truck["stopped"] and truck["depot"] and truck["tile"] == 2443
        if phase == "earn":
            assert earned["earned"] == 3000 and chemical_identity(initial) == chemical_identity(observed)
            assert fuel_identity(initial) == fuel_identity(observed)
    else:
        assert initial["plant"] == observed["plant"]
        assert all(first[key] == final[key] for key in (b"INDY", b"LPCR", b"LPCC", b"MAPH", b"MAPT"))
        if phase == "recall":
            rows["reservation_return"] = [dict(units=recall_reservations(before, output), owner="Vehicle::LeaveStation")]
        else:
            assert first[b"CAPA"] == final[b"CAPA"]
        invoice = sum(row["cost"] for row in rows["invoices"])
        if phase == "carrier":
            carrier, = rows["carrier"]; assert carrier["empty"] and carrier["invoice"] == invoice > 0 and carrier["capacity"] >= 22
            assert old_refill["truck"] is None and new_refill["truck"]["held"] == 0
            assert observed["company"]["cash"] >= 100000
            assert len(TABLE(output, b"VEHS")) == len(TABLE(before, b"VEHS")) + 1
        else:
            assert invoice == 0
        assert observed["company"]["cash"] == initial["company"]["cash"] - invoice and observed["company"]["debt"] == initial["company"]["debt"] <= 5000000
        old_date, new_date = TABLE(before, b"DATE")[0], TABLE(output, b"DATE")[0]
        if phase == "carrier":
            rng = {"random_state[0]", "random_state[1]"}
            assert {k: v for k, v in old_date.items() if k not in rng} == {k: v for k, v in new_date.items() if k not in rng}
            state, other = [old_date[f"random_state[{index}]"] for index in (0, 1)]
            # Exactly one original BuildRoadVehicle body draws a Randomizer
            # value. Preserve every other DATE field and the complete result.
            mixed = other ^ 0x1234567F
            state, other = (state + ((mixed >> 7) | (mixed << 25)) + 1) & 0xFFFFFFFF, (((state >> 3) | (state << 29)) - 1) & 0xFFFFFFFF
            assert [new_date[f"random_state[{index}]"] for index in (0, 1)] == [state, other]
        else:
            assert old_date == new_date
        if phase == "resume":
            assert len(rows["resumed"]) == 3 and not observed["traffic_halted"] and fuel_identity(initial) == fuel_identity(observed)
    if phase not in ("recall", "traffic-work", "resume", "earn"):
        assert initial["traffic_halted"] and observed["traffic_halted"]
    assert observed["company"]["cash"] > 0 and identity(source) == original
    processes = [identity(path) for path in sorted(context.reports.glob("native-*.json"))]; assert processes
    for reference in processes:
        report = json.loads((ROOT / reference["path"]).read_text(encoding="utf-8"))
        assert report["exit_status"] == 0 and report["inputs"] == [original] and report["executable"] == identity(application)
    return dict(receipt=identity(receipt), source=original, before=identity(before), output=identity(output), application=identity(application),
                terminal=identity(log), rows=rows, initial=initial, final=observed, source_pause=source_pause, retained_harness=identity(retained),
                native_processes=processes, sources=[identity(Path(__file__)), identity(ROOT / "src/tests/surface_entry_fuel_refill.cpp"),
                identity(ROOT / "src/tests/surface_entry_work.h"), identity(Path(__file__).with_name("surface_fuel_refill_ledger.py")), identity(Path(__file__).with_name("surface_propellant_ledger.py"))])


def main():
    """Retain one original selected boundary without overwriting any earlier successful receipt."""
    parser = argparse.ArgumentParser(description=__doc__); parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), required=True)
    parser.add_argument("--phase", choices=PHASES, required=True); parser.add_argument("--receipt", type=Path, required=True)
    parser.add_argument("--recover-run"); parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args(); output = (ROOT / args.output).resolve(); assert output.is_relative_to(ROOT / "logs") and not output.exists()
    operation = execute(args.configuration, args.phase, (ROOT / args.receipt).resolve(), args.recover_run)
    output.write_text(json.dumps(dict(_documentation=__doc__, complete=True, configuration=args.configuration, phase=args.phase, operation=operation), indent=2) + "\n", encoding="utf-8")
    print(output.relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
