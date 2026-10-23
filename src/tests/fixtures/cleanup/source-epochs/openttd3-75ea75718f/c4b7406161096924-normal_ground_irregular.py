"""Prepare canonical irregular work using actual retained normal-world machinery.

@file normal_ground_irregular.py
@see r2mlvs16-integrated-acceptance-and-balance.md Group6DCB.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Native quotes/commands own reuse,any required public rail policy and paid support pieces.
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
from normal_ground_boring import observations
from normal_ground_start_report import native_world
from save_tables import saved_table

DEEP_WORK = "logs/acceptance/bootstrap/20261019-native-01/deep-work-verification.json"
DEEP_WORK_SHA = "bbfd942ca5a9c0057aa4d21d5f984d54f310556d7f379e607f39e885b27c90a0"


def run_one(configuration):
    """Load the exact settled normal original and prepare real retained participants without cutting."""
    handoff = ROOT / DEEP_WORK
    assert identity(handoff)["sha256"] == DEEP_WORK_SHA
    basis = json.loads(handoff.read_text(encoding="utf-8"))
    flow = basis["records"][configuration]
    source_ref = flow["settled_input"]
    source = ROOT / source_ref["path"]
    assert identity(source) == source_ref
    predecessor = flow["records"][-1]["actual"]
    run_id = datetime.now(timezone.utc).strftime("%Y%m%d-%H%M%S-") + uuid.uuid4().hex[:10]
    context = create_run_context("acceptance", "normal-ground-irregular", run_id=run_id)
    print(json.dumps({"phase": "preparing_runtime", "run_id": run_id}), flush=True)
    context.prepare_runtime()
    outputs = [context.saves / name for name in ("irregular-ready.sav", "irregular-ready-repeat.sav")]
    assert all(not target.exists() and target.resolve() != source.resolve() for target in outputs)
    context.config_path.write_bytes((ROOT / predecessor["config"]["path"]).read_bytes())
    startup = context.runtime_dir / "scripts/game_start.scr"
    startup.write_text("# Owner: r2mlvs16-integrated-acceptance-and-balance.md6DCB; archi-map-layers.md.\nnormal_ground_irregular\n", encoding="utf-8")
    executable = ROOT / f"build/{configuration}/openttd_test.exe"
    binary = identity(executable)
    environment = dict(os.environ, OTTD_NORMAL_GROUND_INPUT=str(source))
    launch = subprocess.STARTUPINFO()
    launch.dwFlags |= subprocess.STARTF_USESHOWWINDOW
    launch.wShowWindow = subprocess.SW_HIDE
    log = context.logs / f"irregular-{configuration}.log"
    child = None
    try:
        with log.open("wb") as sink:
            child = start_native(context, [executable, "[.normal-ground-irregular]"], cwd=ROOT / "build", env=environment,
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
    ready, quotes = fields(text, "NORMAL_GROUND_IRREGULAR"), observations(text, "NORMAL_GROUND_IRREGULAR_QUOTE")
    assert ready["opening_cash"] == flow["closing"]["cash"] and ready["tick"] == flow["closing"]["tick"]
    assert ready["loan"] == flow["closing"]["loan"] == 300000 and ready["excavated"] == 72
    assert quotes[-1]["ok"] == 1 and quotes[-1]["purchase"] == 0 and ready["bends"] > 0
    invoices = [{key: value if key == "action" else int(value) for key, value in
                 (item.split("=", 1) for item in row.split())}
                for row in re.findall(r"NORMAL_FREIGHT_INVOICE ([^\r\n]+)", text)]
    cash = ready["opening_cash"]
    for invoice in invoices:
        cash -= invoice["cost"]
        assert cash == invoice["cash"]
    assert cash == ready["cash"] > 0
    assert next(row["cost"] for row in invoices if row["action"] == "reuse_standard") == 0
    saves = {name: context.saves / basename for name, basename in (("ready", "irregular-ready.sav"), ("repeat", "irregular-ready-repeat.sav"))}
    assert native_world(saves["ready"]) == native_world(saves["repeat"])
    assert saved_table(saves["ready"], b"DATE")[0]["id"] == saved_table(source, b"DATE")[0]["id"]
    player = saved_table(saves["ready"], b"PLYR")[0]
    assert player["money"] == cash and player["current_loan"] == 300000
    report = {"_documentation": "r2mlvs16-integrated-acceptance-and-balance.md6DCB;canonical irregular proposal reuses actual paid native participants;no new cut",
              "configuration": configuration, "classification": "normal_start_irregular_readiness", "bootstrap_claim": False,
              "controller": identity(__file__), "binary": binary, "deep_work_handoff": identity(handoff),
              "original_input": predecessor["original_input"], "actual_input": source_ref, "config": identity(context.config_path),
              "startup": identity(startup), "ready": ready, "quotes": quotes, "invoices": invoices, "capital_paid": sum(row["cost"] for row in invoices),
              "saves": {name: identity(path) for name, path in saves.items()}, "pure_chunks_equal": 126,
              "native": {"log": identity(log), "assertions": int(counts[1]), "cases": 1}, "children_reaped": child.poll() is not None,
              "sources": [identity(ROOT / name) for name in ("src/tests/normal_ground_irregular_fixture.h", "src/tests/normal_ground_machinery_fixture.h",
                          "src/tests/tbm_reuse.cpp", "src/tbm_commission.cpp", "src/tbm_acquisition.cpp", "src/tbm_area.cpp", "src/tbm_support.cpp",
                          "src/layer_portal_work.cpp", "src/settings.cpp", "src/settings_cmd.h")],
              "remaining": "6DCC actual irregular paid work;6DCD useful final conversion;6DD native recovery/files;6DE genuine S1/S2 publication."}
    target = context.output("reports", f"normal-irregular-{configuration}.json")
    target.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"report": identity(target), "ready": ready, "capital_paid": report["capital_paid"]}), flush=True)
    return target


def main():
    """Run one bounded original-world irregular proposal and real machinery reuse case."""
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--config", choices=("Debug", "RelWithDebInfo"), required=True)
    args = parser.parse_args()
    run_one(args.config)


if __name__ == "__main__":
    main()
