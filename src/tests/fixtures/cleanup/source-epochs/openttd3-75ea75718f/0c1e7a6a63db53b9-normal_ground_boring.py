"""Drive bounded complete native frames for real normally financed Standard,Deep and irregular clearing.

@file normal_ground_boring.py
@see r2mlvs16-integrated-acceptance-and-balance.md Group6CDCB.
@see r2mlvs16-integrated-acceptance-and-balance.md Group6DB.
@see r2mlvs16-integrated-acceptance-and-balance.md Group6DCC.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
The application owns paid work,physical travel,packets,receivers and every expense.
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

MACHINERY = "logs/acceptance/bootstrap/20261019-native-01/standard-machinery-verification.json"
MACHINERY_SHA = "ef5789972ab288e7d94053ad36a260913502f6efcd6185004ff38474a79e1196"
DEEP_MACHINERY = "logs/acceptance/bootstrap/20261019-native-01/deep-machinery-verification.json"
DEEP_MACHINERY_SHA = "7796f578643d6bbb419d479ed6da1f38e5bcd61e2a0d6137734ec8c990a2564a"
IRREGULAR_MACHINERY = "logs/acceptance/bootstrap/20261019-native-01/irregular-readiness-verification.json"
IRREGULAR_MACHINERY_SHA = "db0f76085ab86af039845793a8a6bcd1612126a76baff8d38504f93671c808a5"
WORKING_FILES = "logs/acceptance/bootstrap/20261019-native-01/working-files-verification.json"
WORKING_FILES_SHA = "deabd77e157d3b4e9656550137972cbe47bd4d109dbd4f0c653edf550a872ee8"


def observations(text, marker):
    """Read every native read-only integer observation of one kind in chronological order."""
    return [{key: int(value) for key, value in (item.split("=", 1) for item in row.split())}
            for row in re.findall(rf"{marker} ([^\r\n]+)", text)]


def run_one(configuration, ticks, predecessor=None, variant="standard", capture=False, recover_loaded=False):
    """Continue the exact original native job;each segment owns fresh,nonoverwriting artifacts."""
    scopes = {
        "standard": (MACHINERY, MACHINERY_SHA, "normal-ground-boring", "normal_start_paid_standard_work", "6CDCB"),
        "deep": (DEEP_MACHINERY, DEEP_MACHINERY_SHA, "normal-ground-deep-work", "normal_start_paid_deep_work", "6DB"),
        "irregular": (IRREGULAR_MACHINERY, IRREGULAR_MACHINERY_SHA, "normal-ground-irregular-work", "normal_start_paid_irregular_work", "6DCC"),
    }
    handoff_name, handoff_sha, scenario, classification, group = scopes[variant]
    if capture:
        assert variant == "standard" and predecessor is None
        scenario, group = "normal-ground-checkpoints", "6DDB"
    handoff = ROOT / handoff_name
    assert identity(handoff)["sha256"] == handoff_sha
    basis = json.loads(handoff.read_text(encoding="utf-8"))
    flow = basis["records"][configuration]
    source_ref = flow["ready_input"]
    previous_ref = None
    previous = flow["ready"]
    recovery_ref = None
    config_ref = flow["actual"]["config"]
    if recover_loaded:
        assert variant == "standard" and predecessor is None and not capture
        recovery_ref = identity(ROOT / WORKING_FILES)
        assert recovery_ref["sha256"] == WORKING_FILES_SHA
        recovered = json.loads((ROOT / WORKING_FILES).read_text(encoding="utf-8"))["records"][configuration]
        assert recovered["ready_input"] == source_ref and recovered["original_input"] == flow["actual"]["original_input"]
        source_ref = recovered["checkpoints"]["loaded"]["original"]
        previous = recovered["checkpoints"]["loaded"]["observed"]
        assert previous["hauler_held"] == 162 and previous["returned"] == 1 and previous["resumed"] == 0
        config_ref = recovered["config"]
        classification, group = "normal_start_loaded_recovery", "6DDD"
    if predecessor is not None:
        prior_path = ROOT / predecessor
        previous_ref = identity(prior_path)
        prior = json.loads(prior_path.read_text(encoding="utf-8"))
        assert prior["classification"] == classification and prior["configuration"] == configuration
        assert prior["machinery_handoff"] == identity(handoff) and prior["original_input"] == flow["actual"]["original_input"]
        assert prior["children_reaped"] is True and prior["pure_chunks_equal"] == 126 and prior["bored"]["complete"] == 0
        previous = prior["bored"]
        source_ref = prior["saves"]["progress"]
    source = ROOT / source_ref["path"]
    assert identity(source) == source_ref
    run_id = datetime.now(timezone.utc).strftime("%Y%m%d-%H%M%S-") + uuid.uuid4().hex[:10]
    context = create_run_context("acceptance", scenario, run_id=run_id)
    print(json.dumps({"phase": "preparing_runtime", "run_id": run_id}), flush=True)
    context.prepare_runtime()
    outputs = [context.saves / name for name in ("boring-progress.sav", "boring-progress-repeat.sav")]
    assert all(not target.exists() and target.resolve() != source.resolve() for target in outputs)
    assert identity(ROOT / config_ref["path"]) == config_ref
    context.config_path.write_bytes((ROOT / config_ref["path"]).read_bytes())
    startup = context.runtime_dir / "scripts/game_start.scr"
    startup.write_text(f"# Owner: r2mlvs16-integrated-acceptance-and-balance.md{group}; archi-map-layers.md.\nnormal_ground_boring\n", encoding="utf-8")
    executable = ROOT / f"build/{configuration}/openttd_test.exe"
    binary = identity(executable)
    environment = dict(os.environ, OTTD_NORMAL_GROUND_INPUT=str(source), OTTD_NORMAL_GROUND_TICKS=str(ticks),
                       OTTD_NORMAL_GROUND_RETURNED=str(previous.get("returned", 0)),
                       OTTD_NORMAL_GROUND_RESUMED=str(previous.get("resumed", 0)))
    if capture:
        environment["OTTD_NORMAL_GROUND_CAPTURE"] = "1"
    launch = subprocess.STARTUPINFO()
    launch.dwFlags |= subprocess.STARTF_USESHOWWINDOW
    launch.wShowWindow = subprocess.SW_HIDE
    log = context.logs / f"boring-{configuration}.log"
    child = None
    try:
        with log.open("wb") as sink:
            child = start_native(context, [executable, f"[.{scenario}]"], cwd=ROOT / "build", env=environment,
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
        if previous_ref is not None:
            assert identity(ROOT / previous_ref["path"]) == previous_ref
    text = log.read_text(encoding="utf-8", errors="replace")
    counts = re.search(r"All tests passed \((\d+) assertions? in (\d+) test cases?\)", text)
    assert counts and int(counts[1]) > 0 and int(counts[2]) == 1, log
    bored = fields(text, "NORMAL_GROUND_BORED")
    assert bored["project"] == flow["ready"]["project"] and bored["job"] == flow["ready"]["job"]
    assert bored["opening_cash"] == previous["cash"] and bored["first_tick"] == previous["tick"]
    assert bored["tick"] > bored["first_tick"] and bored["loan"] == previous["loan"] == 300000
    assert bored["first_cursor"] == previous.get("cursor", 0)
    assert bored["first_earth"] == previous.get("earth", 0)
    assert bored["first_rock"] == previous.get("rock", 0)
    ledger = observations(text, "NORMAL_GROUND_BORE_LEDGER")
    assert len(ledger) == 13 and {row["category"] for row in ledger} == set(range(13))
    assert bored["cash"] == bored["opening_cash"] - sum(row["cost"] for row in ledger)
    assert bored["cash"] > 0 and bored["reserved"] == 0
    assert bored["earth_received"] <= bored["earth"] and bored["rock_received"] <= bored["rock"]
    saves = {name: context.saves / basename for name, basename in (("progress", "boring-progress.sav"), ("repeat", "boring-progress-repeat.sav"))}
    before, after = native_world(source), native_world(saves["progress"])
    assert after == native_world(saves["repeat"])
    for owner in ("GDEP", "GSPN", "GVNT"):
        assert before[owner] == after[owner]
    assert saved_table(saves["progress"], b"DATE")[0]["id"] == saved_table(source, b"DATE")[0]["id"]
    player = saved_table(saves["progress"], b"PLYR")[0]
    assert player["money"] == bored["cash"] and player["current_loan"] == 300000
    report = {"_documentation": f"r2mlvs16-integrated-acceptance-and-balance.md{group};actual native frames earn {variant} floor and settle real spoil;bootstrap remains pending",
              "configuration": configuration, "classification": classification, "bootstrap_claim": False,
              "controller": identity(__file__), "binary": binary, "machinery_handoff": identity(handoff),
              "predecessor": previous_ref, "original_input": flow["actual"]["original_input"], "actual_input": source_ref,
              "config": identity(context.config_path), "startup": identity(startup), "tick_budget": ticks, "bored": bored,
              "ledger": ledger, "receivers": observations(text, "NORMAL_GROUND_BORE_RECEIVER"),
              "controls": [{key: value if key == "action" else int(value) for key, value in
                            (item.split("=", 1) for item in row.split())}
                           for row in re.findall(r"NORMAL_FREIGHT_INVOICE ([^\r\n]+)", text)],
              "traces": observations(text, "NORMAL_GROUND_BORE_TRACE"), "sets": observations(text, "NORMAL_GROUND_BORE_SET"),
              "wagons": observations(text, "NORMAL_GROUND_BORE_WAGON"), "packets": observations(text, "NORMAL_GROUND_BORE_PACKET"),
              "parked_haulers": observations(text, "NORMAL_GROUND_BORE_PARKED"),
              "saves": {name: identity(path) for name, path in saves.items()}, "pure_chunks_equal": 126,
              "native": {"log": identity(log), "assertions": int(counts[1]), "cases": 1}, "children_reaped": child.poll() is not None,
              "sources": [identity(ROOT / name) for name in ("src/tests/normal_ground_boring_fixture.h", "src/tests/normal_ground_machinery_fixture.h",
                          "src/tests/normal_ground_freight_fixture.h", "src/tests/tbm_reuse.cpp", "src/tbm_project.cpp", "src/tbm_work.cpp",
                          "src/tbm_exchange.cpp", "src/excavation_cut.cpp", "src/spoil_receiver.cpp", "src/economy.cpp")],
              "remaining": ("Complete current native work if still partial;6DCD useful irregular final conversion;6DD native recovery/files;6DE genuine S1/S2 provenance." if variant != "standard" else
                            "Complete actual Standard work if still partial;6CDCC final release/ordinary access;6CDD Deep readiness;6D full S1/S2.")}
    if recover_loaded:
        report["checkpoint_recovery"] = recovery_ref
        report["checkpoint_opening"] = previous
        report["remaining"] = "6DDD whole loaded-recovery/refusal sealing;6DE genuine original-world runner publication"
    if capture:
        labels = ("working", "working-next", "loaded", "loaded-next")
        report["checkpoint_observations"] = [dict(label=row.split()[0].split("=", 1)[1], **{
            key: int(value) for key, value in (item.split("=", 1) for item in row.split()[1:])})
            for row in re.findall(r"NORMAL_GROUND_CHECKPOINT ([^\r\n]+)", text)]
        assert {row["label"] for row in report["checkpoint_observations"]} == set(labels)
        report["checkpoints"] = {}
        for label in labels:
            path, repeat = (context.saves / f"{label}{suffix}.sav" for suffix in ("", "-repeat"))
            assert native_world(path) == native_world(repeat)
            assert saved_table(path, b"DATE")[0]["id"] == saved_table(source, b"DATE")[0]["id"]
            report["checkpoints"][label] = {"original": identity(path), "repeat": identity(repeat), "pure_chunks_equal": 126}
        report["sources"].extend(identity(ROOT / name) for name in (
            "src/tests/normal_ground_files.cpp", "src/tests/normal_ground_video_fixture.h"))
    target = context.output("reports", f"normal-boring-{configuration}.json")
    target.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"report": identity(target), "bored": bored}), flush=True)
    return target, report


def main():
    """Run sequential bounded native work segments with unique output homes and exact saved lineage."""
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--config", choices=("Debug", "RelWithDebInfo"), required=True)
    parser.add_argument("--variant", choices=("standard", "deep", "irregular"), default="standard")
    parser.add_argument("--ticks", type=int, default=8192)
    parser.add_argument("--segments", type=int, default=1)
    parser.add_argument("--predecessor")
    parser.add_argument("--capture-checkpoints", action="store_true")
    parser.add_argument("--recover-loaded", action="store_true")
    args = parser.parse_args()
    assert 0 < args.ticks <= 65536 and 0 < args.segments <= 8
    predecessor = args.predecessor
    for _ in range(args.segments):
        path, report = run_one(args.config, args.ticks, predecessor, args.variant, args.capture_checkpoints, args.recover_loaded)
        if report["bored"]["complete"]:
            break
        predecessor = path.relative_to(ROOT).as_posix()


if __name__ == "__main__":
    main()
