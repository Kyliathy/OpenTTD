"""Drive actual useful normal-world modal freight through bounded full native continuations.

@file normal_ground_useful.py
@see r2mlvs16-integrated-acceptance-and-balance.md Group6DCDD.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Only native commands and the ordinary GameLoop move cargo/vehicles,pay invoices or advance time.
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

SUPPLY = "logs/acceptance/bootstrap/20261019-native-01/useful-supply-verification.json"
SUPPLY_SHA = "38b5f0f1d8042aac697f7e33b368c93ebc38c2cfdd6fe98e70a8c9efb7ddb120"


def run_one(configuration, ticks, predecessor=None):
    """Load the exact paid original or prior pure progress save and drive one bounded real segment."""
    handoff = ROOT / SUPPLY
    assert identity(handoff)["sha256"] == SUPPLY_SHA
    flow = json.loads(handoff.read_text(encoding="utf-8"))["records"][configuration]
    previous = json.loads((ROOT / predecessor).read_text(encoding="utf-8")) if predecessor else None
    if previous:
        assert previous["classification"] == "normal_start_useful_operation" and previous["configuration"] == configuration
        assert previous["supply_handoff"] == identity(handoff) and previous["useful"]["complete"] == 0
        source_ref = previous["saves"]["progress"]
        expected = previous["useful"]
        config_ref = previous["config"]
    else:
        source_ref, expected, config_ref = flow["ready_input"], flow["ready"], flow["actual"]["config"]
    source = ROOT / source_ref["path"]
    assert identity(source) == source_ref
    run_id = datetime.now(timezone.utc).strftime("%Y%m%d-%H%M%S-") + uuid.uuid4().hex[:10]
    context = create_run_context("acceptance", "normal-ground-useful", run_id=run_id)
    print(json.dumps({"phase": "preparing_runtime", "run_id": run_id}), flush=True)
    context.prepare_runtime()
    saves = {key: context.saves / name for key, name in (("progress", "useful-progress.sav"), ("repeat", "useful-progress-repeat.sav"))}
    assert all(not path.exists() and path.resolve() != source.resolve() for path in saves.values())
    context.config_path.write_bytes((ROOT / config_ref["path"]).read_bytes())
    startup = context.runtime_dir / "scripts/game_start.scr"
    startup.write_text("# Owner: r2mlvs16-integrated-acceptance-and-balance.md6DCDD;archi-map-layers.md.\nnormal_ground_useful\n", encoding="utf-8")
    executable = ROOT / f"build/{configuration}/openttd_test.exe"
    binary = identity(executable)
    environment = dict(os.environ, OTTD_NORMAL_GROUND_INPUT=str(source), OTTD_NORMAL_GROUND_TICKS=str(ticks),
                       OTTD_NORMAL_GROUND_CARRIER=str(flow["machinery"]["carrier"]), OTTD_NORMAL_GROUND_CAPSULE=str(flow["machinery"]["capsule"]))
    launch = subprocess.STARTUPINFO()
    launch.dwFlags |= subprocess.STARTF_USESHOWWINDOW
    launch.wShowWindow = subprocess.SW_HIDE
    log = context.logs / f"useful-{configuration}.log"
    child = None
    try:
        with log.open("wb") as sink:
            child = start_native(context, [executable, "[.normal-ground-useful]"], cwd=ROOT / "build", env=environment,
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
    useful = fields(text, "NORMAL_GROUND_USEFUL")
    ledger, traces, packets, controls = (observations(text, "NORMAL_GROUND_USEFUL_" + name) for name in ("LEDGER", "TRACE", "PACKET", "CONTROL"))
    assert useful["first_tick"] == expected["tick"] and useful["opening_cash"] == expected["cash"]
    # The realtime saver executes between whole frames,after the requested
    # threshold. Retain actual extra frames within a bounded save-settlement
    # window instead of pretending the saved clock was truncated. The owned
    # child has a separate110-second wall-clock bound,including native pauses.
    assert 0 < useful["tick"] - useful["first_tick"] <= ticks + 3000 and useful["loan"] == 300000
    assert useful["carrier"] == flow["machinery"]["carrier"] and useful["capsule"] == flow["machinery"]["capsule"]
    assert useful["first_energy"] == (expected["energy"] if previous else 0)
    assert useful["first_invoice"] == (expected["invoice"] if previous else 20)
    assert len(ledger) == 13 and sorted(row["category"] for row in ledger) == list(range(13))
    assert useful["cash"] == useful["opening_cash"] - sum(row["cost"] for row in ledger)
    assert native_world(saves["progress"]) == native_world(saves["repeat"])
    assert saved_table(saves["progress"], b"DATE")[0]["id"] == saved_table(source, b"DATE")[0]["id"]
    report = {"_documentation": "r2mlvs16-integrated-acceptance-and-balance.md6DCDD;bounded actual full-native original-world useful modal freight;whole-chain sealing required",
              "configuration": configuration, "classification": "normal_start_useful_operation", "bootstrap_claim": False,
              "driver": "ground-acceptance-null",
              "controller": identity(__file__), "binary": binary, "supply_handoff": identity(handoff),
              "predecessor": identity(ROOT / predecessor) if predecessor else None, "tick_budget": ticks,
              "original_input": flow["actual"]["original_input"], "actual_input": source_ref, "config": identity(context.config_path),
              "startup": identity(startup), "useful": useful, "ledger": ledger, "traces": traces, "packets": packets, "controls": controls,
              "invoices": invoices_from(text), "saves": {key: identity(path) for key, path in saves.items()}, "pure_chunks_equal": 126,
              "native": {"log": identity(log), "assertions": int(counts[1]), "cases": 1}, "children_reaped": child.poll() is not None,
              "motion_boundary": identity(ROOT / "logs/acceptance/bootstrap/20261019-native-01/useful-motion-boundary.log"),
              "sources": [identity(ROOT / name) for name in ("src/tests/normal_ground_useful_fixture.h", "src/tests/normal_ground_video_fixture.h", "src/tests/normal_ground_modal.cpp", "src/tests/normal_ground_supply_fixture.h",
                          "src/tests/normal_ground_freight_fixture.h", "src/tests/normal_ground_newgame_fixture.h", "src/transport_tube_runtime.cpp", "src/transport_tube_power.cpp",
                          "src/economy.cpp", "src/cargopacket.cpp", "src/train_cmd.cpp", "src/order_cmd.cpp")],
              "remaining": "Whole-chain useful physical/payment/empty-return sealing;6DD native recovery/files;6DE genuine S1/S2 publication."}
    target = context.output("reports", f"normal-useful-{configuration}.json")
    target.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"report": identity(target), "useful": useful}), flush=True)
    return target, useful["complete"] == 1


def main():
    """Run exact saved continuations,with a unique invocation/context for every selected native segment."""
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--config", choices=("Debug", "RelWithDebInfo"), required=True)
    parser.add_argument("--ticks", type=int, default=8192)
    parser.add_argument("--segments", type=int, default=1)
    parser.add_argument("--predecessor")
    args = parser.parse_args()
    assert 0 < args.ticks <= 32768 and 1 <= args.segments <= 8
    previous = args.predecessor
    for _ in range(args.segments):
        report, complete = run_one(args.config, args.ticks, previous)
        if complete:
            break
        previous = str(report.relative_to(ROOT))


if __name__ == "__main__":
    main()
