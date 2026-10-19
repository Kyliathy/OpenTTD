"""Renew only due Steel and rail service with whole original funding and real empty return.

@file surface_entry_coordinated_checks-1.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15BIEP.
Native quotes, movement, wagon recoupling, original books and sole cargo owners
establish every result. Healthy ore and passenger vehicles retain their identities.
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
REFILL = runpy.run_path(str(DIRECTORY / "surface_entry_fuel_refill_checks-3.py"))
GOODS = runpy.run_path(str(DIRECTORY / "surface_entry_goods_checks-1.py"))["goods_identity"]


def healthy(path, snapshot):
    """@param path Actual complete save. @param snapshot Whole-fuel sole-owner observation.
    @return Current ore/passenger identities, engine and capacity, excluding legitimate movement and aging.
    """
    result = [(row["id"], row["name"], row["engine"], row["capacity"]) for row in snapshot["passenger_buses"]]
    prefix = "roadveh[0].common[0]."
    for key, row in TABLE(path, b"VEHS").items():
        if row["type"] != 1:
            continue
        if bytes.fromhex(row[prefix + "name"]).decode("utf-8") != "Surface ore delivery":
            continue
        result.append((key, "Surface ore delivery", row[prefix + "engine_type"], row[prefix + "cargo_cap"]))
    assert len(result) == 4
    return result


def wagons(path, snapshot):
    """@param path Native save. @param snapshot Actual rail identities.
    @return Every original useful wagon identity and its native model/cargo/capacity, grouped by service.
    """
    vehicles = TABLE(path, b"VEHS"); prefix = "train[0].common[0]."; result = {}
    # Current single24fronts are the first part; all useful wagons must
    # survive native engine-only sale and normal recoupling unchanged.
    for train in snapshot["trains"]:
        result[train["name"]] = [(key, vehicles[key][prefix + "engine_type"], vehicles[key][prefix + "cargo_type"], vehicles[key][prefix + "cargo_cap"]) for key in train["chain"][1:]]
    assert sum(len(value) for value in result.values()) == 32
    return result


def execute(configuration, actor, phase, receipt, recover_run=None, observe_fn=observe, with_steel=True):
    """@param configuration Actual focused build. @param actor Native rail fleet or selected due Steel truck.
    @param phase Pure query, original recall, actual work, paid buy, short replay or annual service proof.
    @param receipt Exact immediate selected predecessor. @param recover_run Existing successful native work to inspect only.
    @param observe_fn Explicit current sole-owner material observer; original closed-fuel default remains strict.
    @param with_steel Preserve original coordinated scope by default; false explicitly selects the delivered original three-rail producer.
    @return Original complete financial, physical, healthy-service and native-process evidence.
    """
    assert actor in ("rail", "steel") and phase in ("query", "recall", "work", "buy", "earn", "annual")
    if not with_steel:
        assert actor == "rail"
    if phase == "earn":
        assert actor == "steel"
    if phase == "annual":
        assert actor == "rail"
    predecessor = json.loads(receipt.read_text(encoding="utf-8")); assert predecessor["complete"] and predecessor["configuration"] == configuration
    original = predecessor["operation"]["output"]; source = ROOT / original["path"]; assert identity(source) == original
    road = actor == "steel"; native_phase = dict(work="work" if road else "arrive", buy="renew" if road else "buy", annual="earn").get(phase, phase)
    scenario = "surface-entry-coordinated-" + actor + "-" + phase
    if recover_run is None:
        context = create_run_context("space", scenario); context.prepare_runtime()
        retained = context.directory / "harness.py"; retained.write_bytes(Path(__file__).read_bytes())
        command = "surface_entry_road_renewal" if road else "surface_entry_renewal"
        (context.runtime_dir / "scripts/game_start.scr").write_text(command + "\n", encoding="utf-8")
    else:
        context = RunContext(ROOT, "space", scenario, identifier(recover_run)); retained = context.directory / "harness.py"; assert retained.is_file()
    application = ROOT / "build" / configuration / "openttd_surface_supply_test.exe"; log = context.logs / "native.log"
    if recover_run is None:
        environment = dict(os.environ, OTTD_TEST_CONFIGURATION=configuration, OTTD_SURFACE_ENTRY_SOURCE=str(source),
                           OTTD_SURFACE_ROAD_SCOPE="steel", OTTD_SURFACE_RENEWAL_SCOPE="rail-and-steel")
        if not with_steel:
            environment.pop("OTTD_SURFACE_RENEWAL_SCOPE")
        environment["OTTD_SURFACE_ROAD_PHASE" if road else "OTTD_SURFACE_RENEWAL_PHASE"] = native_phase
        with log.open("wb") as sink:
            result = run_native(context, [str(application), "[.surface-entry-road-renewal]" if road else "[.surface-entry-renewal]"],
                                cwd=ROOT / "build", stdout=sink, stderr=subprocess.STDOUT, timeout=110, env=environment)
        assert result.returncode == 0, log
    terminal = log.read_text(encoding="utf-8", errors="replace"); prefix = "SURFACE_ROAD_RENEWAL_" if road else "SURFACE_RENEWAL_"
    assert "All tests passed" in terminal and prefix + "COMPLETE phase=" + native_phase in terminal, log
    stem = "surface-road-" if road else "surface-renewal-"
    before, output = [context.saves / f"{stem}{name}-440.sav" for name in ("before-" + native_phase, native_phase)]
    saved, first, last = [chunks(path)[1] for path in (source, before, output)]
    assert saved.keys() == first.keys() == last.keys() and len(first) == 121
    assert all(saved[key] == first[key] for key in saved if key not in (b"GLOG", b"DATE"))
    source_pause = None
    if saved[b"DATE"] != first[b"DATE"]:
        # Qualify only original restored due-worker65to1; every other DATE
        # field, RNG seed and actual tick remains exact at this load boundary.
        assert TABLE(source, b"DATE")[0]["pause_mode"] == 65 and TABLE(before, b"DATE")[0]["pause_mode"] == 1
        source_pause = RESTORE_PAUSE(before, source)
    if saved[b"GLOG"] != first[b"GLOG"]:
        REVISION(source, before)
    assert first[b"GLOG"] == last[b"GLOG"]
    rows = {name: [FIELDS(line) for line in terminal.splitlines() if line.startswith(prefix + token + " ")]
            for name, token in dict(plan="PLAN", models="MODEL", selected="SELECTED", extra="EXTRA_ROAD", invoices="INVOICE", recalled="RECALL",
                                    arrival="ARRIVAL", renewed="RENEWED", heads="HEAD", wagons="WAGON", purchase="PURCHASE", custody="CUSTODY",
                                    earned="EARNED", expenses="EXPENSE", delivered="DELIVERED").items()}
    initial, final = observe_fn(before), observe_fn(output)
    assert reserve_identity(initial) == reserve_identity(final) and GOODS(initial) == GOODS(final)
    assert OLD_FUEL(initial) == OLD_FUEL(final) and REFILL["fuel_identity"](initial) == REFILL["fuel_identity"](final)
    assert initial["plant"] == final["plant"] and final["plant"]["produced"] == 42
    assert healthy(before, initial) == healthy(output, final) and wagons(before, initial) == wagons(output, final)
    if phase in ("work", "earn", "annual"):
        limit = 27000 if phase == "annual" else 3000
        earned, = rows["earned"]; assert 0 <= earned["earned"] <= limit
        assert TABLE(output, b"DATE")[0]["tick_counter"] - TABLE(before, b"DATE")[0]["tick_counter"] == earned["earned"]
        assert sum(row["amount"] for row in rows["expenses"]) == earned["net_expense"] == initial["company"]["cash"] - final["company"]["cash"]
        assert initial["company"]["debt"] == final["company"]["debt"] and not rows["invoices"]
        if phase in ("earn", "annual"):
            assert earned["earned"] == limit
    else:
        assert all(first[key] == last[key] for key in (b"INDY", b"LPCR", b"LPCC", b"MAPT", b"MAPH"))
        # Only rail recall leaves a loading station through SkipToOrder;
        # qualify its real reservation splits without weakening road custody.
        if phase == "recall" and not road:
            rows["reservation_return"] = [dict(units=REFILL["recall_reservations"](before, output), owner="Vehicle::LeaveStation")]
        else:
            assert first[b"CAPA"] == last[b"CAPA"]
        invoice = sum(row["cost"] for row in rows["invoices"]); credit = final["company"]["debt"] - initial["company"]["debt"]
        assert final["company"]["cash"] == initial["company"]["cash"] + credit - invoice
        old_date, new_date = TABLE(before, b"DATE")[0], TABLE(output, b"DATE")[0]
        if phase == "buy":
            bodies = 1 if road else 3; assert len(rows["renewed"] if road else rows["heads"]) == bodies
            assert final["company"]["cash"] >= 150000 and len(TABLE(before, b"VEHS")) == len(TABLE(output, b"VEHS"))
            rng = {"random_state[0]", "random_state[1]"}
            assert {k: v for k, v in old_date.items() if k not in rng} == {k: v for k, v in new_date.items() if k not in rng}
            state, other = [old_date[f"random_state[{index}]"] for index in (0, 1)]
            # Only actual paid native single-body builds draw these values;
            # empty sales, order copies and original wagon moves earn no ticks.
            for _ in range(bodies):
                mixed = other ^ 0x1234567F
                state, other = (state + ((mixed >> 7) | (mixed << 25)) + 1) & 0xFFFFFFFF, (((state >> 3) | (state << 29)) - 1) & 0xFFFFFFFF
            assert [new_date[f"random_state[{index}]"] for index in (0, 1)] == [state, other]
        else:
            assert old_date == new_date and credit == invoice == 0
        if phase == "query":
            assert first == last and initial == final
            plan, = rows["plan"]
            if road:
                assert plan["trucks"] == 1 and len(rows["selected"]) == 1
            else:
                assert plan["heads"] == 3 and plan["cash"] + plan["credit"] - plan["whole_gross"] >= 150000
                # Only explicitly coordinated renewal prices due Steel. The
                # original rail-only producer leaves every healthy road role intact.
                if with_steel:
                    extra, = rows["extra"]; assert plan["road_gross"] == extra["cost"] and plan["whole_gross"] == plan["gross"] + extra["cost"]
                else:
                    assert not rows["extra"] and plan["road_gross"] == 0 and plan["whole_gross"] == plan["gross"]
    assert final["company"]["cash"] > 0 and 0 <= final["company"]["debt"] <= 5000000 and identity(source) == original
    processes = [identity(path) for path in sorted(context.reports.glob("native-*.json"))]; assert processes
    for reference in processes:
        report = json.loads((ROOT / reference["path"]).read_text(encoding="utf-8"))
        assert report["exit_status"] == 0 and report["inputs"] == [original] and report["executable"] == identity(application)
    return dict(receipt=identity(receipt), source=original, before=identity(before), output=identity(output), application=identity(application), terminal=identity(log),
                rows=rows, initial=initial, final=final, with_steel=with_steel, source_pause=source_pause, native_processes=processes, retained_harness=identity(retained),
                sources=[identity(Path(__file__)), identity(ROOT / "src/tests/surface_entry_renewal.cpp"), identity(ROOT / "src/tests/surface_entry_road_renewal.cpp"),
                         identity(ROOT / "src/tests/surface_entry_work.h"), identity(DIRECTORY / "surface_fuel_refill_ledger.py"), identity(DIRECTORY / "surface_propellant_ledger.py"),
                         identity(Path(observe_fn.__code__.co_filename))])


def main():
    """Publish one actual original coordinated-service boundary without repeating successful work."""
    parser = argparse.ArgumentParser(description=__doc__); parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), required=True)
    parser.add_argument("--actor", choices=("rail", "steel"), required=True); parser.add_argument("--phase", choices=("query", "recall", "work", "buy", "earn", "annual"), required=True)
    parser.add_argument("--receipt", type=Path, required=True); parser.add_argument("--recover-run"); parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args(); output = (ROOT / args.output).resolve(); assert output.is_relative_to(ROOT / "logs") and not output.exists()
    operation = execute(args.configuration, args.actor, args.phase, (ROOT / args.receipt).resolve(), args.recover_run)
    output.write_text(json.dumps(dict(_documentation=__doc__, complete=True, configuration=args.configuration, actor=args.actor, phase=args.phase, operation=operation), indent=2) + "\n", encoding="utf-8")
    print(output.relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
