"""Inspect actual native readiness on exact ordinary starting originals.

@file normal_ground_readiness.py
@see r2mlvs16-integrated-acceptance-and-balance.md Group6C.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Pure quote characterization never becomes paid readiness or bootstrap acceptance.
"""

import argparse
import json
import os
import re
import subprocess
import sys

sys.dont_write_bytecode = True
from ground_network import ROOT
from runner import current_run_context, start_native
from construction_provider_return import identity
from save_tables import chunks, read_stream, saved_table

START = "logs/acceptance/bootstrap/20261019-native-01/normal-start-verification.json"
START_SHA = "244583e6c91754ecb51949d23c212a0fcea36f0f07e833a2e9540074ec4d5cdc"
TAG = "[.normal-ground-readiness]"


def run_one(configuration, focus="quote", predecessor=None, entrance=None):
    """Load one exact normal original and inspect supported funding/construction quotes.

    @param configuration Current actual native test build.
    @param focus Original entrance characterization or stable complete-yard selection.
    @return Bounded actual quote/refusal report;no predecessor is declared paid or built.
    """
    handoff = ROOT / START
    assert identity(handoff)["sha256"] == START_SHA
    selected = json.loads(handoff.read_text(encoding="utf-8"))["records"][configuration]
    source_ref = selected["starting_input"]
    prior_identity = None
    if predecessor is not None:
        prior_path = ROOT / predecessor
        prior_identity = identity(prior_path)
        prior = json.loads(prior_path.read_text(encoding="utf-8"))
        assert prior["configuration"] == configuration
        assert prior["original_input"] == selected["starting_input"]
        assert prior["classification"] == "normal_start_freight_earnings"
        assert prior["pure_chunks_equal"] == 126 and prior["children_reaped"] is True
        source_ref = prior["saves"]["earned"]
    source = ROOT / source_ref["path"]
    assert identity(source) == source_ref
    context = current_run_context("acceptance", "normal-ground-readiness")
    context.prepare_runtime()
    context.config_path.write_bytes((ROOT / selected["actual"]["config"]["path"]).read_bytes())
    startup = context.runtime_dir / "scripts/game_start.scr"
    startup.write_text("# Owner: r2mlvs16-integrated-acceptance-and-balance.md6C; archi-map-layers.md.\nnormal_ground_ready\n", encoding="utf-8")
    executable = ROOT / f"build/{configuration}/openttd_test.exe"
    binary = identity(executable)
    entrance = selected["actual"]["starting_owners"]["entrance"] if entrance is None else entrance
    environment = dict(os.environ, OTTD_NORMAL_GROUND_INPUT=str(source),
                       OTTD_NORMAL_GROUND_ENTRANCE=str(entrance),
                       OTTD_NORMAL_GROUND_STAGE=focus)
    launch = subprocess.STARTUPINFO()
    launch.dwFlags |= subprocess.STARTF_USESHOWWINDOW
    launch.wShowWindow = subprocess.SW_HIDE
    log = context.logs / f"readiness-{configuration}-{focus}.log"
    child = None
    try:
        with log.open("wb") as sink:
            child = start_native(context, [executable, TAG], cwd=ROOT / "build", env=environment,
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
    quotes = [{key: value if key == "action" else int(value) for key, value in (item.split("=", 1) for item in line.split())}
              for line in re.findall(r"NORMAL_GROUND_QUOTE ([^\r\n]+)", text)]
    assert len(quotes) == 11
    rows = re.findall(r"NORMAL_GROUND_READINESS ([^\r\n]+)", text)
    assert len(rows) == 1
    readiness = {key: value if key == "stage" else int(value) for key, value in (item.split("=", 1) for item in rows[0].split())}
    assert readiness["stage"] == focus
    if focus == "yard":
        assert readiness["candidates"] > 0
        if readiness["selected"]:
            assert readiness["accepted"] == 1 and all(row["ok"] == 1 for row in quotes)
        else:
            assert readiness["accepted"] == 0 and any(row["ok"] == 0 for row in quotes)
    saves = {name: context.saves / f"readiness-{name}.sav" for name in ("before", "after")}
    states = {}
    for name, path in saves.items():
        version, stream = read_stream(path)
        assert version == 434
        states[name] = {row["name"]: row["raw"] for row in chunks(stream)}
        assert len(states[name]) == 126
    assert states["before"] == states["after"], "Readiness estimates changed native saved owners"
    assert saved_table(saves["after"], b"DATE")[0]["id"] == saved_table(source, b"DATE")[0]["id"]
    models = [{key: int(value) for key, value in (item.split("=", 1) for item in line.split())}
              for line in re.findall(r"NORMAL_GROUND_MODEL ([^\r\n]+)", text)]
    assert models
    receivers = [{key: int(value) for key, value in (item.split("=", 1) for item in line.split())}
                 for line in re.findall(r"NORMAL_GROUND_RECEIVER ([^\r\n]+)", text)]
    assert len(receivers) == 2
    report = {"_documentation": "r2mlvs16-integrated-acceptance-and-balance.md6C;actual ordinary starting-world pure public Surface logistics/finance characterization,not paid readiness or bootstrap",
              "configuration": configuration, "focus": focus, "classification": "normal_start_readiness_quote", "bootstrap_claim": False,
              "controller": identity(__file__), "binary": binary, "handoff": identity(handoff), "original_input": selected["starting_input"],
              "actual_input": source_ref, "predecessor": prior_identity,
              "config": identity(context.config_path), "startup": identity(startup), "requested_entrance": entrance, "quotes": quotes, "models": models,
              "receivers": receivers, "readiness_accepted": bool(readiness["accepted"]),
              "readiness": readiness, "saves": {name: identity(path) for name, path in saves.items()}, "pure_chunks_equal": 126,
              "native": {"log": identity(log), "assertions": int(counts[1]), "cases": 1}, "children_reaped": child.poll() is not None,
              "sources": [identity(ROOT / name) for name in ("src/tests/normal_ground_readiness_fixture.h", "src/tests/tbm_reuse.cpp",
                          "src/industry_cmd.cpp", "src/rail_cmd.cpp", "src/station_cmd.cpp", "src/spoil_receiver.cpp")],
              "remaining": "Actual native financing/paid Surface construction/hauler and commissioning readiness;no quote/refusal supplies a paid predecessor."}
    target = context.output("reports", f"normal-readiness-{configuration}-{focus}.json")
    target.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"report": identity(target), "readiness": readiness, "quotes": quotes, "models": models, "receivers": receivers}), flush=True)
    return target


def main():
    """Run one bounded original-world readiness characterization."""
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--config", choices=("Debug", "RelWithDebInfo"), required=True)
    parser.add_argument("--focus", choices=("quote", "yard"), default="quote")
    parser.add_argument("--predecessor", help="Exact accepted earnings report when quoting the same original world's later prices")
    parser.add_argument("--entrance", type=int, help="Explicit native Surface entrance proposal;estimates never build or prepare terrain")
    args = parser.parse_args()
    run_one(args.config, args.focus, args.predecessor, args.entrance)


if __name__ == "__main__":
    main()
