"""Pay for native modal infrastructure on the actual earned normal-world floor.

@file normal_ground_layout.py
@see r2mlvs16-integrated-acceptance-and-balance.md Group6DCDB.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Construction is a prerequisite;it supplies no useful cargo or bootstrap claim.
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

IRREGULAR_WORK = "logs/acceptance/bootstrap/20261019-native-01/irregular-work-verification.json"
IRREGULAR_WORK_SHA = "fc09a953514dbec4748ed5d9defe77f8b45820cdc3769cd6edf9ad15e326ae54"


def invoices_from(text):
    """Read the actual public command invoices without reconstructing a price authority."""
    return [{key: value if key == "action" else int(value) for key, value in
             (item.split("=", 1) for item in row.split())}
            for row in re.findall(r"NORMAL_FREIGHT_INVOICE ([^\r\n]+)", text)]


def run_one(configuration):
    """Release the exact settled project and pay for actual native facilities without a simulation step."""
    handoff = ROOT / IRREGULAR_WORK
    assert identity(handoff)["sha256"] == IRREGULAR_WORK_SHA
    flow = json.loads(handoff.read_text(encoding="utf-8"))["records"][configuration]
    source_ref = flow["settled_input"]
    source = ROOT / source_ref["path"]
    assert identity(source) == source_ref
    predecessor = flow["records"][-1]["actual"]
    run_id = datetime.now(timezone.utc).strftime("%Y%m%d-%H%M%S-") + uuid.uuid4().hex[:10]
    context = create_run_context("acceptance", "normal-ground-layout", run_id=run_id)
    print(json.dumps({"phase": "preparing_runtime", "run_id": run_id}), flush=True)
    context.prepare_runtime()
    saves = {key: context.saves / name for key, name in (("layout", "modal-layout.sav"), ("repeat", "modal-layout-repeat.sav"))}
    assert all(not path.exists() and path.resolve() != source.resolve() for path in saves.values())
    context.config_path.write_bytes((ROOT / predecessor["config"]["path"]).read_bytes())
    startup = context.runtime_dir / "scripts/game_start.scr"
    startup.write_text("# Owner: r2mlvs16-integrated-acceptance-and-balance.md6DCDB;archi-map-layers.md.\nnormal_ground_layout\n", encoding="utf-8")
    executable = ROOT / f"build/{configuration}/openttd_test.exe"
    binary = identity(executable)
    environment = dict(os.environ, OTTD_NORMAL_GROUND_INPUT=str(source))
    launch = subprocess.STARTUPINFO()
    launch.dwFlags |= subprocess.STARTF_USESHOWWINDOW
    launch.wShowWindow = subprocess.SW_HIDE
    log = context.logs / f"layout-{configuration}.log"
    child = None
    try:
        with log.open("wb") as sink:
            child = start_native(context, [executable, "[.normal-ground-layout]"], cwd=ROOT / "build", env=environment,
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
    layout, cells, invoices = fields(text, "NORMAL_GROUND_LAYOUT"), observations(text, "NORMAL_GROUND_LAYOUT_CELL"), invoices_from(text)
    assert layout["opening_cash"] == flow["closing"]["cash"] and layout["tick"] == flow["closing"]["tick"]
    assert layout["loan"] == 300000 and layout["excavated"] == 112
    cash = layout["opening_cash"]
    for invoice in invoices:
        cash -= invoice["cost"]
        assert cash == invoice["cash"]
    assert cash == layout["cash"] > 0
    assert native_world(saves["layout"]) == native_world(saves["repeat"])
    assert saved_table(saves["layout"], b"DATE")[0]["id"] == saved_table(source, b"DATE")[0]["id"]
    report = {"_documentation": "r2mlvs16-integrated-acceptance-and-balance.md6DCDB;native paid layout on exact actual irregular floor;no useful cargo claim",
              "configuration": configuration, "classification": "normal_start_modal_layout", "bootstrap_claim": False,
              "controller": identity(__file__), "binary": binary, "irregular_work_handoff": identity(handoff),
              "original_input": predecessor["original_input"], "actual_input": source_ref, "config": identity(context.config_path),
              "startup": identity(startup), "layout": layout, "cells": cells, "invoices": invoices,
              "capital_paid": sum(row["cost"] for row in invoices), "saves": {key: identity(path) for key, path in saves.items()},
              "pure_chunks_equal": 126, "native": {"log": identity(log), "assertions": int(counts[1]), "cases": 1},
              "children_reaped": child.poll() is not None,
              "focused_boundaries": [identity(ROOT / f"logs/acceptance/bootstrap/20261019-native-01/{name}") for name in ("useful-floor-boundary.log", "useful-command-boundary.log")],
              "sources": [identity(ROOT / name) for name in ("src/tests/normal_ground_layout_fixture.h", "src/tests/tbm_reuse.cpp", "src/tbm_support.cpp", "src/tbm_controls.cpp", "src/tbm_project.cpp",
                          "src/transport_tube_construction.cpp", "src/rail_cmd.cpp", "src/station_cmd.cpp", "src/layer_portal_work.cpp")],
              "remaining": "6DCDC actual Surface supply/machinery/power;6DCDD actual useful operation;6DD files/recovery;6DE genuine S1/S2 publication."}
    target = context.output("reports", f"normal-layout-{configuration}.json")
    target.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"report": identity(target), "layout": layout, "capital_paid": report["capital_paid"]}), flush=True)
    return target


def main():
    """Run one bounded actual-original native layout case."""
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--config", choices=("Debug", "RelWithDebInfo"), required=True)
    run_one(parser.parse_args().config)


if __name__ == "__main__":
    main()
