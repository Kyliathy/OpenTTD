"""Run bounded native freight earnings on the original normal-world lineage.

@file normal_ground_earnings.py
@see r2mlvs16-integrated-acceptance-and-balance.md Group6CB.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Native GameLoop,industry production,vehicles and CargoPayment own every earning.
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
from normal_ground_freight import fields
from normal_ground_readiness import START, START_SHA


def run_one(configuration, predecessor, ticks=32768):
    """Continue one exact original paid service;never supply a replacement cargo or vehicle."""
    assert 0 < ticks <= 65536
    handoff = ROOT / START
    assert identity(handoff)["sha256"] == START_SHA
    starting = json.loads(handoff.read_text(encoding="utf-8"))["records"][configuration]
    prior_path = ROOT / predecessor
    prior_identity = identity(prior_path)
    prior = json.loads(prior_path.read_text(encoding="utf-8"))
    assert prior["configuration"] == configuration
    assert prior["original_input"] == starting["starting_input"]
    assert prior["classification"] in ("normal_start_paid_freight", "normal_start_paid_freight_extension", "normal_start_freight_earnings")
    assert prior["native"]["cases"] == 1 and prior["native"]["assertions"] > 0
    assert prior["pure_chunks_equal"] == 126 and prior["children_reaped"] is True
    earned_predecessor = prior["classification"] == "normal_start_freight_earnings"
    source_ref = prior["saves"]["earned" if earned_predecessor else "ready"]
    source = ROOT / source_ref["path"]
    assert identity(source) == source_ref
    # Every bounded segment is a distinct writer. Sharing the generic imported
    # runner invocation would reuse the previous segment's input/output paths.
    run_id = datetime.now(timezone.utc).strftime("%Y%m%d-%H%M%S-") + uuid.uuid4().hex[:10]
    context = create_run_context("acceptance", "normal-ground-earnings", run_id=run_id)
    print(json.dumps({"phase": "preparing_runtime", "run_id": run_id}), flush=True)
    outputs = [context.saves / name for name in ("freight-earned.sav", "freight-earned-repeat.sav")]
    assert all(not target.exists() and target.resolve() != source.resolve() for target in outputs), "Refuse input/output collision before native launch"
    context.prepare_runtime()
    print(json.dumps({"phase": "runtime_ready", "run_id": run_id}), flush=True)
    context.config_path.write_bytes((ROOT / starting["actual"]["config"]["path"]).read_bytes())
    startup = context.runtime_dir / "scripts/game_start.scr"
    startup.write_text("# Owner: r2mlvs16-integrated-acceptance-and-balance.md6CB; archi-map-layers.md.\nnormal_ground_earn\n", encoding="utf-8")
    executable = ROOT / f"build/{configuration}/openttd_test.exe"
    binary = identity(executable)
    environment = dict(os.environ, OTTD_NORMAL_GROUND_INPUT=str(source), OTTD_NORMAL_GROUND_TICKS=str(ticks))
    launch = subprocess.STARTUPINFO()
    launch.dwFlags |= subprocess.STARTF_USESHOWWINDOW
    launch.wShowWindow = subprocess.SW_HIDE
    log = context.logs / f"earnings-{configuration}.log"
    child = None
    try:
        with log.open("wb") as sink:
            child = start_native(context, [executable, "[.normal-ground-earnings]"], cwd=ROOT / "build", env=environment,
                                 stdout=sink, stderr=subprocess.STDOUT, creationflags=subprocess.CREATE_NO_WINDOW,
                                 startupinfo=launch)
            print(json.dumps({"phase": "native_started", "run_id": run_id, "pid": child.pid, "budget": ticks}), flush=True)
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
        assert identity(prior_path) == prior_identity
    text = log.read_text(encoding="utf-8", errors="replace")
    counts = re.search(r"All tests passed \((\d+) assertions? in (\d+) test cases?\)", text)
    assert counts and int(counts[1]) > 0 and int(counts[2]) == 1, log
    earned = fields(text, "NORMAL_FREIGHT_EARNED")
    previous = prior["earned"] if earned_predecessor else prior["ready"]
    assert earned["first_tick"] == previous["tick"]
    assert ticks <= earned["tick"] - earned["first_tick"] < ticks + 1000
    assert earned["opening_cash"] == previous["cash"]
    assert earned["front"] == previous["front"]
    assert earned["loan"] == previous["loan"] == 300000
    assert earned["income"] > 0 and earned["expenses"] < 0 and earned["delivered"] > 0
    ledger = [{key: int(value) for key, value in (item.split("=", 1) for item in row.split())}
              for row in re.findall(r"NORMAL_FREIGHT_LEDGER ([^\r\n]+)", text)]
    assert [row["category"] for row in ledger] == list(range(13))
    assert earned["cash"] == earned["opening_cash"] - sum(row["cost"] for row in ledger)
    assert earned["income"] == -ledger[7]["cost"]
    assert earned["expenses"] == -sum(ledger[index]["cost"] for index in (2, 6, 11))
    assert earned["overhead"] == ledger[12]["cost"] >= 0
    assert earned["cash"] == earned["opening_cash"] + earned["income"] + earned["expenses"] - earned["overhead"]
    assert earned["visited"] > 20 and earned["peak_load"] > 0
    trains = [{key: int(value) for key, value in (item.split("=", 1) for item in row.split())}
              for row in re.findall(r"NORMAL_FREIGHT_TRAIN ([^\r\n]+)", text)]
    assert trains
    for row in trains:
        moved = row["visited"] > 20 and row["peak_load"] > 0
        waited = row.get("waiting", 0) == 1 and 0 <= row["held"] < row["capacity"]
        assert moved or waited
        if not earned_predecessor:
            assert moved, "The first earnings run must prove every purchased route actually carries cargo"
    if "fleet" in previous:
        assert len(trains) == previous["fleet"]
    deliveries = [{key: int(value) for key, value in (item.split("=", 1) for item in row.split())}
                  for row in re.findall(r"NORMAL_FREIGHT_DELIVERED ([^\r\n]+)", text)]
    assert deliveries and sum(row["amount"] for row in deliveries) == earned["delivered"]
    saves = {name: context.saves / f"freight-earned{suffix}.sav" for name, suffix in (("earned", ""), ("repeat", "-repeat"))}
    states = {}
    for name, path in saves.items():
        version, stream = read_stream(path)
        assert version == 434
        states[name] = {row["name"]: row["raw"] for row in chunks(stream)}
        assert len(states[name]) == 126
        assert saved_table(path, b"DATE")[0]["id"] == saved_table(source, b"DATE")[0]["id"]
    assert states["earned"] == states["repeat"]
    report = {"_documentation": "r2mlvs16-integrated-acceptance-and-balance.md6CB;actual native ordinary freight earnings from original generated world",
              "configuration": configuration, "classification": "normal_start_freight_earnings", "bootstrap_claim": False,
              "controller": identity(__file__), "binary": binary, "handoff": identity(handoff),
              "original_input": prior["original_input"], "predecessor": prior_identity, "actual_input": source_ref,
              "config": identity(context.config_path), "startup": identity(startup), "budget": ticks, "earned": earned, "ledger": ledger,
              "trains": trains, "fleet": len(trains), "deliveries": deliveries,
              "cumulative_income": prior.get("cumulative_income", 0) + earned["income"],
              "cumulative_expenses": prior.get("cumulative_expenses", 0) + earned["expenses"],
              "cumulative_overhead": prior.get("cumulative_overhead", 0) + earned["overhead"],
              "cumulative_delivered": prior.get("cumulative_delivered", 0) + earned["delivered"],
              "saves": {name: identity(path) for name, path in saves.items()}, "pure_chunks_equal": 126,
              "native": {"log": identity(log), "assertions": int(counts[1]), "cases": 1}, "children_reaped": child.poll() is not None,
              "sources": [identity(ROOT / name) for name in ("src/tests/normal_ground_freight_fixture.h", "src/tests/tbm_reuse.cpp",
                          "src/openttd.cpp", "src/video/null_v.cpp", "src/economy.cpp", "src/company_cmd.cpp")],
              "remaining": "Enough genuine earned capital for paid underground readiness;Surface yard/hauler/commissioning and S1/S2 remain pending."}
    target = context.output("reports", f"normal-earnings-{configuration}.json")
    target.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"report": identity(target), "earned": earned, "cumulative_income": report["cumulative_income"],
                      "cumulative_delivered": report["cumulative_delivered"]}), flush=True)
    return target


def main():
    """Run one bounded native economic continuation."""
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--config", choices=("Debug", "RelWithDebInfo"), required=True)
    parser.add_argument("--predecessor", required=True)
    parser.add_argument("--ticks", type=int, default=32768)
    parser.add_argument("--segments", type=int, choices=range(1, 11), default=1,
                        help="Sequential bounded native files;each child must finish below110seconds")
    parser.add_argument("--minimum-cash", type=int, help="Stop this bounded batch after native closing cash reaches this declared target")
    args = parser.parse_args()
    predecessor = args.predecessor
    for _ in range(args.segments):
        predecessor = run_one(args.config, predecessor, args.ticks)
        if args.minimum_cash is not None:
            assert args.minimum_cash > 0
            if json.loads(predecessor.read_text(encoding="utf-8"))["earned"]["cash"] >= args.minimum_cash:
                break


if __name__ == "__main__":
    main()
