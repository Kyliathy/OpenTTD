"""Build actual ordinary freight on exact generated normal-start originals.

@file normal_ground_freight.py
@see r2mlvs16-integrated-acceptance-and-balance.md Group6CB.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Every invoice belongs to native public construction/vehicle/order commands.
This predecessor alone does not claim underground readiness or full bootstrap.
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
from save_tables import chunks, read_stream, saved_table
from normal_ground_readiness import START, START_SHA


def fields(text, marker):
    """Read one native observation with exact integer fields."""
    rows = re.findall(rf"{marker} ([^\r\n]+)", text)
    assert len(rows) == 1, (marker, rows)
    return {key: int(value) for key, value in (item.split("=", 1) for item in rows[0].split())}


def run_one(configuration, cargo="COAL", predecessor=None):
    """Pay for a real initial or additional route using the actual original company's available cash."""
    assert cargo in ("COAL", "WOOD", "IORE", "OIL_")
    assert (cargo == "COAL") == (predecessor is None)
    handoff = ROOT / START
    assert identity(handoff)["sha256"] == START_SHA
    selected = json.loads(handoff.read_text(encoding="utf-8"))["records"][configuration]
    source_ref = selected["starting_input"]
    prior_identity = None
    previous = None
    if predecessor is not None:
        prior_path = ROOT / predecessor
        prior_identity = identity(prior_path)
        prior = json.loads(prior_path.read_text(encoding="utf-8"))
        assert prior["configuration"] == configuration and prior["original_input"] == selected["starting_input"]
        assert prior["classification"] in ("normal_start_paid_freight", "normal_start_paid_freight_extension", "normal_start_freight_earnings")
        assert prior["pure_chunks_equal"] == 126 and prior["children_reaped"] is True
        earned = prior["classification"] == "normal_start_freight_earnings"
        source_ref = prior["saves"]["earned" if earned else "ready"]
        previous = prior["earned" if earned else "ready"]
    source = ROOT / source_ref["path"]
    assert identity(source) == source_ref
    run_id = datetime.now(timezone.utc).strftime("%Y%m%d-%H%M%S-") + uuid.uuid4().hex[:10]
    context = create_run_context("acceptance", "normal-ground-freight", run_id=run_id)
    outputs = [context.saves / name for name in ("freight-ready.sav", "freight-ready-repeat.sav")]
    assert all(not target.exists() and target.resolve() != source.resolve() for target in outputs)
    context.prepare_runtime()
    context.config_path.write_bytes((ROOT / selected["actual"]["config"]["path"]).read_bytes())
    startup = context.runtime_dir / "scripts/game_start.scr"
    startup.write_text("# Owner: r2mlvs16-integrated-acceptance-and-balance.md6CB; archi-map-layers.md.\nnormal_ground_freight\n", encoding="utf-8")
    executable = ROOT / f"build/{configuration}/openttd_test.exe"
    binary = identity(executable)
    environment = dict(os.environ, OTTD_NORMAL_GROUND_INPUT=str(source), OTTD_NORMAL_GROUND_CARGO=cargo)
    launch = subprocess.STARTUPINFO()
    launch.dwFlags |= subprocess.STARTF_USESHOWWINDOW
    launch.wShowWindow = subprocess.SW_HIDE
    log = context.logs / f"freight-{configuration}.log"
    child = None
    try:
        with log.open("wb") as sink:
            child = start_native(context, [executable, "[.normal-ground-freight]"], cwd=ROOT / "build", env=environment,
                                 stdout=sink, stderr=subprocess.STDOUT, creationflags=subprocess.CREATE_NO_WINDOW,
                                 startupinfo=launch)
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
    proposal = fields(text, "NORMAL_FREIGHT_PROPOSAL")
    ready = fields(text, "NORMAL_FREIGHT_READY")
    invoices = [{key: value if key == "action" else int(value) for key, value in
                 (item.split("=", 1) for item in row.split())}
                for row in re.findall(r"NORMAL_FREIGHT_INVOICE ([^\r\n]+)", text)]
    assert sum(row["action"] == "rail" for row in invoices) == proposal["rails"]
    assert sum(row["action"] == "station" for row in invoices) == 2
    assert sum(row["action"] == "locomotive" for row in invoices) == 1
    assert sum(row["action"] == "wagon" for row in invoices) == 6
    assert ready["loan"] == 300000
    opening = previous["cash"] if previous is not None else 100000
    assert ready["opening_cash"] == opening
    available = opening if previous is not None else 300000
    assert ready["cash"] == available - sum(row["cost"] for row in invoices) > 0
    assert ready["fleet"] == (previous.get("fleet", 1) + 1 if previous is not None else 1)
    cash = available
    for row in invoices:
        cash -= row["cost"]
        assert row["cash"] == cash
    saves = {name: context.saves / f"freight-ready{suffix}.sav" for name, suffix in (("ready", ""), ("repeat", "-repeat"))}
    states = {}
    for name, path in saves.items():
        version, stream = read_stream(path)
        assert version == 434
        states[name] = {row["name"]: row["raw"] for row in chunks(stream)}
        assert len(states[name]) == 126
        assert saved_table(path, b"DATE")[0]["id"] == saved_table(source, b"DATE")[0]["id"]
    assert states["ready"] == states["repeat"]
    report = {"_documentation": "r2mlvs16-integrated-acceptance-and-balance.md6CB;actual original-world public paid freight predecessor",
              "configuration": configuration, "classification": "normal_start_paid_freight" if previous is None else "normal_start_paid_freight_extension", "bootstrap_claim": False,
              "controller": identity(__file__), "binary": binary, "handoff": identity(handoff), "original_input": selected["starting_input"],
              "actual_input": source_ref, "predecessor": prior_identity, "cargo_label": cargo,
              "config": identity(context.config_path), "startup": identity(startup), "proposal": proposal, "ready": ready,
              "invoices": invoices, "construction_expense": sum(row["cost"] for row in invoices),
              "saves": {name: identity(path) for name, path in saves.items()}, "pure_chunks_equal": 126,
              "native": {"log": identity(log), "assertions": int(counts[1]), "cases": 1}, "children_reaped": child.poll() is not None,
              "sources": [identity(ROOT / name) for name in ("src/tests/normal_ground_freight_fixture.h", "src/tests/tbm_reuse.cpp",
                          "src/rail_cmd.cpp", "src/station_cmd.cpp", "src/train_cmd.cpp", "src/vehicle_cmd.cpp", "src/order_cmd.cpp", "src/misc_cmd.cpp")],
              "remaining": "Real native freight earnings,paid Surface underground yard,hauler/commissioning and normal-start S1/S2."}
    target = context.output("reports", f"normal-freight-{configuration}.json")
    target.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"report": identity(target), "proposal": proposal, "ready": ready, "construction_expense": report["construction_expense"]}), flush=True)
    return target


def main():
    """Run one bounded ordinary freight construction case."""
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--config", choices=("Debug", "RelWithDebInfo"), required=True)
    parser.add_argument("--cargo", choices=("COAL", "WOOD", "IORE", "OIL_"), default="COAL")
    parser.add_argument("--predecessor", help="Accepted paid route or earnings report for a new ordinary route")
    args = parser.parse_args()
    run_one(args.config, args.cargo, args.predecessor)


if __name__ == "__main__":
    main()
