"""Prepare actual Surface Coal supply,retained native carriage and paid tube power.

@file normal_ground_supply.py
@see r2mlvs16-integrated-acceptance-and-balance.md Group6DCDC.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Real native depot trips precede paid construction;useful freight operation remains6DCDD.
"""

import argparse
import json
import os
import re
import subprocess
import sys
import uuid
from datetime import datetime, timezone

sys.dont_write_bytecode = True
from ground_network import ROOT
from runner import create_run_context, start_native
from construction_provider_return import identity
from normal_ground_layout import fields, observations, invoices_from
from normal_ground_start_report import native_world
from save_tables import saved_table

LAYOUT = "logs/acceptance/bootstrap/20261019-native-01/modal-layout-verification.json"
LAYOUT_SHA = "adf17a20f1787d4d9cf8b516ff0afa93eda79d1f75538736db45cf25ddfa4624"


def run_one(configuration):
    """Run one bounded full-native parking and actual paid preparation on an exact original layout."""
    handoff = ROOT / LAYOUT
    assert identity(handoff)["sha256"] == LAYOUT_SHA
    flow = json.loads(handoff.read_text(encoding="utf-8"))["records"][configuration]
    source_ref = flow["layout_input"]
    source = ROOT / source_ref["path"]
    assert identity(source) == source_ref
    predecessor = flow["actual"]
    run_id = datetime.now(timezone.utc).strftime("%Y%m%d-%H%M%S-") + uuid.uuid4().hex[:10]
    context = create_run_context("acceptance", "normal-ground-supply", run_id=run_id)
    print(json.dumps({"phase": "preparing_runtime", "run_id": run_id}), flush=True)
    context.prepare_runtime()
    saves = {key: context.saves / name for key, name in (("ready", "useful-ready.sav"), ("repeat", "useful-ready-repeat.sav"))}
    assert all(not path.exists() and path.resolve() != source.resolve() for path in saves.values())
    context.config_path.write_bytes((ROOT / predecessor["config"]["path"]).read_bytes())
    startup = context.runtime_dir / "scripts/game_start.scr"
    startup.write_text("# Owner: r2mlvs16-integrated-acceptance-and-balance.md6DCDC;archi-map-layers.md.\nnormal_ground_supply\n", encoding="utf-8")
    executable = ROOT / f"build/{configuration}/openttd_test.exe"
    binary = identity(executable)
    environment = dict(os.environ, OTTD_NORMAL_GROUND_INPUT=str(source))
    launch = subprocess.STARTUPINFO()
    launch.dwFlags |= subprocess.STARTF_USESHOWWINDOW
    launch.wShowWindow = subprocess.SW_HIDE
    log = context.logs / f"supply-{configuration}.log"
    child = None
    try:
        with log.open("wb") as sink:
            child = start_native(context, [executable, "[.normal-ground-supply]"], cwd=ROOT / "build", env=environment,
                                 stdout=sink, stderr=subprocess.STDOUT, creationflags=subprocess.CREATE_NO_WINDOW, startupinfo=launch)
            print(json.dumps({"phase": "native_started", "run_id": run_id, "pid": child.pid}), flush=True)
            assert child.wait(timeout=110) == 0, log
    finally:
        if child is not None and child.poll() is None:
            child.terminate()
            try:
                child.wait(timeout=5)
            except subprocess.TimeoutExpired:
                child.kill()
                child.wait(timeout=5)
        assert identity(source) == source_ref and identity(executable) == binary
    text = log.read_text(encoding="utf-8", errors="replace")
    counts = re.search(r"All tests passed \((\d+) assertions? in (\d+) test cases?\)", text)
    assert counts and int(counts[1]) > 0 and int(counts[2]) == 1, log
    ready, quote, machinery = (fields(text, marker) for marker in ("NORMAL_GROUND_SUPPLY_READY", "NORMAL_GROUND_SUPPLY_QUOTE", "NORMAL_GROUND_SUPPLY_MACHINERY"))
    parked, stored, ledger = (observations(text, marker) for marker in ("NORMAL_GROUND_SUPPLY_PARKED", "NORMAL_GROUND_SUPPLY_STORED", "NORMAL_GROUND_SUPPLY_LEDGER"))
    invoices = invoices_from(text)
    assert ready["opening_cash"] == flow["layout"]["cash"] and ready["first_tick"] == flow["layout"]["tick"] < ready["tick"]
    assert ready["complete"] == 1 and ready["loan"] == 300000 and ready["cash"] > 0
    assert len(ledger) == 13 and sorted(row["category"] for row in ledger) == list(range(13))
    assert ready["cash"] == ready["opening_cash"] - sum(row["cost"] for row in ledger)
    assert sum(row["cost"] for row in invoices) == quote["tracks"] + quote["capsule"] + quote["wagon"] + quote["lease"]
    assert ready["cash"] == quote["cash"] - sum(row["cost"] for row in invoices)
    assert len(parked) == 2 and all(row["stored"] == 1 and row["visited"] > 1 for row in parked)
    assert len(stored) == 6 and all(row["count"] == 0 and row["front"] != ready["carrier"] for row in stored)
    assert native_world(saves["ready"]) == native_world(saves["repeat"])
    assert saved_table(saves["ready"], b"DATE")[0]["id"] == saved_table(source, b"DATE")[0]["id"]
    report = {"_documentation": "r2mlvs16-integrated-acceptance-and-balance.md6DCDC;actual native Surface supply/machinery/power readiness;useful operation remains6DCDD",
              "configuration": configuration, "classification": "normal_start_useful_supply", "bootstrap_claim": False,
              "controller": identity(__file__), "binary": binary, "layout_handoff": identity(handoff),
              "original_input": predecessor["original_input"], "actual_input": source_ref, "config": identity(context.config_path),
              "startup": identity(startup), "ready": ready, "quote": quote, "machinery": machinery, "parked": parked, "stored": stored, "ledger": ledger,
              "invoices": invoices, "capital_paid": sum(row["cost"] for row in invoices), "parking_cash_delta": quote["cash"] - ready["opening_cash"],
              "saves": {key: identity(path) for key, path in saves.items()}, "pure_chunks_equal": 126,
              "native": {"log": identity(log), "assertions": int(counts[1]), "cases": 1}, "children_reaped": child.poll() is not None,
              "binding_boundary": identity(ROOT / "logs/acceptance/bootstrap/20261019-native-01/useful-binding-boundary.log"),
              "sources": [identity(ROOT / name) for name in ("src/tests/normal_ground_supply_fixture.h", "src/tests/normal_ground_modal.cpp", "src/tests/normal_ground_freight_fixture.h",
                          "src/tests/CMakeLists.txt", "src/train_cmd.cpp", "src/order_cmd.cpp", "src/transport_tube_cmd.cpp", "src/transport_tube_power.cpp", "src/transport_tube_runtime.cpp")],
              "remaining": "6DCDD actual useful freight operation;6DD native recovery/files;6DE genuine acceptance."}
    target = context.output("reports", f"normal-supply-{configuration}.json")
    target.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"report": identity(target), "ready": ready, "machinery": machinery, "capital_paid": report["capital_paid"]}), flush=True)
    return target


def main():
    """Prepare one actual original,with all participants stopped and no useful operation claim."""
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--config", choices=("Debug", "RelWithDebInfo"), required=True)
    run_one(parser.parse_args().config)


if __name__ == "__main__":
    main()
