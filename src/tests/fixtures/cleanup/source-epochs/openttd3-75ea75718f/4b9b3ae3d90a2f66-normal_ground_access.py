"""Release settled native normal-world work and pay for ordinary Underground access.

@file normal_ground_access.py
@see r2mlvs16-integrated-acceptance-and-balance.md Group6CDCC.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
No new floor is opened;actual released support becomes ordinary paid track.
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

WORK = "logs/acceptance/bootstrap/20261019-native-01/standard-work-verification.json"
WORK_SHA = "01fd7e457f48e68dec9233b6572e919f37a4649251a54432d4d6bf79d4cb3245"


def run_one(configuration):
    """Use the exact original actually settled world;native commands own release and payment."""
    handoff = ROOT / WORK
    assert identity(handoff)["sha256"] == WORK_SHA
    basis = json.loads(handoff.read_text(encoding="utf-8"))
    flow = basis["records"][configuration]
    source_ref = flow["settled_input"]
    source = ROOT / source_ref["path"]
    assert identity(source) == source_ref
    original = flow["records"][0]["actual"]
    run_id = datetime.now(timezone.utc).strftime("%Y%m%d-%H%M%S-") + uuid.uuid4().hex[:10]
    context = create_run_context("acceptance", "normal-ground-access", run_id=run_id)
    print(json.dumps({"phase": "preparing_runtime", "run_id": run_id}), flush=True)
    context.prepare_runtime()
    outputs = [context.saves / name for name in ("ordinary-access.sav", "ordinary-access-repeat.sav")]
    assert all(not target.exists() and target.resolve() != source.resolve() for target in outputs)
    context.config_path.write_bytes((ROOT / original["config"]["path"]).read_bytes())
    startup = context.runtime_dir / "scripts/game_start.scr"
    startup.write_text("# Owner: r2mlvs16-integrated-acceptance-and-balance.md6CDCC; archi-map-layers.md.\nnormal_ground_access\n", encoding="utf-8")
    executable = ROOT / f"build/{configuration}/openttd_test.exe"
    binary = identity(executable)
    environment = dict(os.environ, OTTD_NORMAL_GROUND_INPUT=str(source))
    launch = subprocess.STARTUPINFO()
    launch.dwFlags |= subprocess.STARTF_USESHOWWINDOW
    launch.wShowWindow = subprocess.SW_HIDE
    log = context.logs / f"access-{configuration}.log"
    child = None
    try:
        with log.open("wb") as sink:
            child = start_native(context, [executable, "[.normal-ground-access]"], cwd=ROOT / "build", env=environment,
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
    access = fields(text, "NORMAL_GROUND_ACCESS")
    assert access["opening_cash"] == flow["closing"]["cash"] and access["tick"] == flow["closing"]["tick"]
    assert access["loan"] == flow["closing"]["loan"] == 300000
    assert access["state"] == 2 and access["excavated"] == 36
    invoices = [{key: value if key == "action" else int(value) for key, value in
                 (item.split("=", 1) for item in row.split())}
                for row in re.findall(r"NORMAL_FREIGHT_INVOICE ([^\r\n]+)", text)]
    cash = access["opening_cash"]
    for invoice in invoices:
        cash -= invoice["cost"]
        assert cash == invoice["cash"]
    assert cash == access["cash"] > 0
    assert sum(row["cost"] for row in invoices if row["action"] == "ordinary_support") == access["conversion"]
    saves = {name: context.saves / basename for name, basename in (("access", "ordinary-access.sav"), ("repeat", "ordinary-access-repeat.sav"))}
    assert native_world(saves["access"]) == native_world(saves["repeat"])
    assert saved_table(saves["access"], b"DATE")[0]["id"] == saved_table(source, b"DATE")[0]["id"]
    player = saved_table(saves["access"], b"PLYR")[0]
    assert player["money"] == cash and player["current_loan"] == 300000
    report = {"_documentation": "r2mlvs16-integrated-acceptance-and-balance.md6CDCC;native release/paid support conversion connects actual earned floor;no new excavation",
              "configuration": configuration, "classification": "normal_start_ordinary_underground_access", "bootstrap_claim": False,
              "controller": identity(__file__), "binary": binary, "work_handoff": identity(handoff),
              "original_input": original["original_input"], "actual_input": source_ref, "config": identity(context.config_path),
              "startup": identity(startup), "access": access, "invoices": invoices, "capital_paid": sum(row["cost"] for row in invoices),
              "saves": {name: identity(path) for name, path in saves.items()}, "pure_chunks_equal": 126,
              "native": {"log": identity(log), "assertions": int(counts[1]), "cases": 1}, "children_reaped": child.poll() is not None,
              "sources": [identity(ROOT / name) for name in ("src/tests/normal_ground_access_fixture.h", "src/tests/normal_ground_boring_fixture.h",
                          "src/tests/tbm_reuse.cpp", "src/tbm_project.cpp", "src/tbm_exchange.cpp", "src/tbm_support.cpp", "src/layer_portal_work.cpp")],
              "remaining": "6CDD actual Deep machinery readiness on earned accessible floor;6D full normal-start S1/S2 acceptance."}
    target = context.output("reports", f"normal-access-{configuration}.json")
    target.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"report": identity(target), "access": access, "capital_paid": report["capital_paid"]}), flush=True)
    return target


def main():
    """Run one bounded actual settled-world ordinary access case."""
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--config", choices=("Debug", "RelWithDebInfo"), required=True)
    args = parser.parse_args()
    run_one(args.config)


if __name__ == "__main__":
    main()
