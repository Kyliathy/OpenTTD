"""Acquire real Standard participants on exact paid normal-world yards.

@file normal_ground_machinery.py
@see r2mlvs16-integrated-acceptance-and-balance.md Group6CDB.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Native commands own every vehicle,order,project and payment;no cutting is driven here.
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
from normal_ground_freight import fields
from normal_ground_start_report import native_world
from save_tables import saved_table

YARD = "logs/acceptance/bootstrap/20261019-native-01/surface-yard-verification.json"
YARD_SHA = "6cd491b51db7a70138868f37082b49e2fe10a67f319d3b6c97540757765631f2"


def run_one(configuration):
    """Buy and configure actual native participants without simulating or opening any rock."""
    handoff = ROOT / YARD
    assert identity(handoff)["sha256"] == YARD_SHA
    basis = json.loads(handoff.read_text(encoding="utf-8"))
    flow = basis["records"][configuration]
    source_ref = flow["paid_input"]
    source = ROOT / source_ref["path"]
    assert identity(source) == source_ref
    yard = flow["yard"]
    run_id = datetime.now(timezone.utc).strftime("%Y%m%d-%H%M%S-") + uuid.uuid4().hex[:10]
    context = create_run_context("acceptance", "normal-ground-machinery", run_id=run_id)
    print(json.dumps({"phase": "preparing_runtime", "run_id": run_id}), flush=True)
    context.prepare_runtime()
    outputs = [context.saves / name for name in ("standard-ready.sav", "standard-ready-repeat.sav")]
    assert all(not target.exists() and target.resolve() != source.resolve() for target in outputs)
    context.config_path.write_bytes((ROOT / flow["actual"]["config"]["path"]).read_bytes())
    startup = context.runtime_dir / "scripts/game_start.scr"
    startup.write_text("# Owner: r2mlvs16-integrated-acceptance-and-balance.md6CDB; archi-map-layers.md.\nnormal_ground_machinery\n", encoding="utf-8")
    executable = ROOT / f"build/{configuration}/openttd_test.exe"
    binary = identity(executable)
    environment = dict(os.environ, OTTD_NORMAL_GROUND_INPUT=str(source),
                       OTTD_NORMAL_GROUND_ENTRANCE=str(yard["portal"]))
    launch = subprocess.STARTUPINFO()
    launch.dwFlags |= subprocess.STARTF_USESHOWWINDOW
    launch.wShowWindow = subprocess.SW_HIDE
    log = context.logs / f"machinery-{configuration}.log"
    child = None
    try:
        with log.open("wb") as sink:
            child = start_native(context, [executable, "[.normal-ground-machinery]"], cwd=ROOT / "build", env=environment,
                                 stdout=sink, stderr=subprocess.STDOUT, creationflags=subprocess.CREATE_NO_WINDOW,
                                 startupinfo=launch)
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
        assert identity(source) == source_ref
        assert identity(executable) == binary
    text = log.read_text(encoding="utf-8", errors="replace")
    counts = re.search(r"All tests passed \((\d+) assertions? in (\d+) test cases?\)", text)
    assert counts and int(counts[1]) > 0 and int(counts[2]) == 1, log
    ready = fields(text, "NORMAL_GROUND_MACHINERY")
    quote = fields(text, "NORMAL_GROUND_COMMISSION_QUOTE")
    engines = [{key: int(value) for key, value in (item.split("=", 1) for item in row.split())}
               for row in re.findall(r"NORMAL_GROUND_ENGINE ([^\r\n]+)", text)]
    assert {row["model"] for row in engines} == {2, 3, 4}
    assert all(row["available"] == 1 for row in engines)
    assert quote["ok"] == 1 and quote["purchase"] > 0
    assert ready["portal"] == yard["portal"] and ready["origin"] == yard["origin"] and ready["depot"] == yard["depot"]
    assert ready["opening_cash"] == yard["cash"] and ready["tick"] == yard["tick"]
    assert ready["loan"] == yard["loan"] == 300000
    assert ready["original_solid"] == ready["solid"] + ready["excavating"] == 31752
    assert ready["reserved"] == ready["excavated"] == 0 and ready["excavating"] > 0
    invoices = [{key: value if key == "action" else int(value) for key, value in
                 (item.split("=", 1) for item in row.split())}
                for row in re.findall(r"NORMAL_FREIGHT_INVOICE ([^\r\n]+)", text)]
    assert sum(row["action"] == "machinery_unit" for row in invoices) == 5
    for action in ("spoil_origin_order", "standard_commission", "assign_hauler", "pause_standard"):
        assert sum(row["action"] == action for row in invoices) == 1
    assert next(row["cost"] for row in invoices if row["action"] == "standard_commission") == quote["purchase"]
    cash = ready["opening_cash"]
    for row in invoices:
        cash -= row["cost"]
        assert row["cash"] == cash
    assert cash == ready["cash"] > 0
    saves = {name: context.saves / basename for name, basename in (("ready", "standard-ready.sav"), ("repeat", "standard-ready-repeat.sav"))}
    assert native_world(saves["ready"]) == native_world(saves["repeat"])
    assert saved_table(saves["ready"], b"DATE")[0]["id"] == saved_table(source, b"DATE")[0]["id"]
    player = saved_table(saves["ready"], b"PLYR")[0]
    assert player["money"] == ready["cash"] and player["current_loan"] == 300000
    report = {"_documentation": "r2mlvs16-integrated-acceptance-and-balance.md6CDB;actual normally earned funds buy native Standard participants;no cuts driven",
              "configuration": configuration, "classification": "normal_start_standard_machinery", "bootstrap_claim": False,
              "controller": identity(__file__), "binary": binary, "yard_handoff": identity(handoff),
              "original_input": flow["actual"]["original_input"], "actual_input": source_ref, "config": identity(context.config_path),
              "startup": identity(startup), "ready": ready, "quote": quote, "engines": engines, "invoices": invoices,
              "capital_paid": sum(row["cost"] for row in invoices),
              "saves": {name: identity(path) for name, path in saves.items()}, "pure_chunks_equal": 126,
              "native": {"log": identity(log), "assertions": int(counts[1]), "cases": 1}, "children_reaped": child.poll() is not None,
              "sources": [identity(ROOT / name) for name in ("src/tests/normal_ground_machinery_fixture.h", "src/tests/normal_ground_yard_fixture.h",
                          "src/tests/normal_ground_freight_fixture.h", "src/tests/tbm_reuse.cpp", "src/tbm_cmd.h", "src/tbm_area_preview.h",
                          "src/tbm_commission.cpp", "src/underground_vehicle.cpp", "src/tbm_exchange.cpp")],
              "remaining": "6CDC paid Standard clearing/disposal/withdrawal and accessible Underground floor;6CDD Deep readiness;6D normal-start S1/S2."}
    target = context.output("reports", f"normal-machinery-{configuration}.json")
    target.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"report": identity(target), "ready": ready, "capital_paid": report["capital_paid"]}), flush=True)
    return target


def main():
    """Run one bounded actual original-world Standard participant acquisition case."""
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--config", choices=("Debug", "RelWithDebInfo"), required=True)
    args = parser.parse_args()
    run_one(args.config)


if __name__ == "__main__":
    main()
