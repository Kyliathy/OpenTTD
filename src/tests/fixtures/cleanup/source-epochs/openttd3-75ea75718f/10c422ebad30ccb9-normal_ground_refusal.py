"""Exercise actual normal-world active/final native refusal without changing any saved owner.

@file normal_ground_refusal.py
@see r2mlvs16-integrated-acceptance-and-balance.md Group6DDD.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Exact immutable original file selection and all126chunk comparisons retain physical authority.
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
from ground_network_report import verify_reference
from normal_ground_layout import fields
from tube_tbm_files import equal_world

BANK = "logs/acceptance/bootstrap/20261019-native-01/original-file-bank.json"
BANK_SHA = "fb2c8d058fe5d1ba5be3ab2b60c557a47225908834ce6c7ca873fffb86e16570"


def run_one(configuration, scope):
    """Run public native refusal on an exact ordinarily loaded active or useful final original."""
    bank_ref = identity(ROOT / BANK)
    assert bank_ref["sha256"] == BANK_SHA
    bank = json.loads(verify_reference(bank_ref).read_text(encoding="utf-8"))
    selected = bank["records"][configuration]["working-bore" if scope == "active" else "final-useful"]
    source = verify_reference(selected["original"])
    run_id = datetime.now(timezone.utc).strftime("%Y%m%d-%H%M%S-") + uuid.uuid4().hex[:10]
    context = create_run_context("acceptance", "normal-ground-refusal", run_id=run_id)
    context.prepare_runtime()
    context.config_path.write_bytes(verify_reference(selected["config"]).read_bytes())
    startup = context.runtime_dir / "scripts/game_start.scr"
    startup.write_text("# Plan16-6DDD;actual original-world native refusal.\nnormal_ground_refusal\n", encoding="utf-8")
    executable = ROOT / f"build/{configuration}/openttd_test.exe"
    binary = identity(executable)
    environment = dict(os.environ, OTTD_NORMAL_GROUND_INPUT=str(source), OTTD_NORMAL_GROUND_REFUSAL=scope)
    launch = subprocess.STARTUPINFO()
    launch.dwFlags |= subprocess.STARTF_USESHOWWINDOW
    launch.wShowWindow = subprocess.SW_HIDE
    log = context.logs / f"refusal-{configuration}-{scope}.log"
    child = None
    print(json.dumps({"phase": "native_refusal_started", "configuration": configuration, "scope": scope}), flush=True)
    try:
        with log.open("wb") as sink:
            child = start_native(context, [executable, "[.normal-ground-refusal]"], cwd=ROOT / "build", env=environment,
                                 stdout=sink, stderr=subprocess.STDOUT, creationflags=subprocess.CREATE_NO_WINDOW, startupinfo=launch)
            assert child.wait(timeout=110) == 0, log
    finally:
        if child is not None and child.poll() is None:
            child.terminate()
            try:
                child.wait(timeout=5)
            except subprocess.TimeoutExpired:
                child.kill()
                child.wait(timeout=5)
        assert identity(source) == selected["original"] and identity(executable) == binary
    text = log.read_text(encoding="utf-8", errors="replace")
    counts = re.search(r"All tests passed \((\d+) assertions? in (\d+) test cases?\)", text)
    assert counts and int(counts[1]) > 0 and int(counts[2]) == 1, log
    observed = fields(text, "NORMAL_GROUND_REFUSAL_" + scope.upper())
    saves = {name: context.saves / f"{name}.sav" for name in ("before", "after", "repeat")}
    comparisons = {name: equal_world(source, path) for name, path in saves.items()}
    report = {"_documentation": "r2mlvs16-integrated-acceptance-and-balance.md6DDD;actual native refusal leaves all original saved quantities unchanged",
              "classification": "normal_start_native_refusal", "bootstrap_claim": False,
              "configuration": configuration, "scope": scope, "bank": bank_ref, "selected": selected, "observed": observed,
              "saves": {name: identity(path) for name, path in saves.items()}, "comparisons": comparisons,
              "controller": identity(__file__), "binary": binary, "config": identity(context.config_path), "startup": identity(startup),
              "native": {"log": identity(log), "assertions": int(counts[1]), "cases": 1}, "children_reaped": child.poll() is not None,
              "sources": [identity(ROOT / name) for name in ("src/tests/normal_ground_refusal.cpp", "src/tests/normal_ground_video_fixture.h",
                          "src/tests/normal_ground_newgame_fixture.h", "src/tests/normal_ground_freight_fixture.h",
                          "src/tbm_support.cpp", "src/tbm_project.cpp", "src/layer_portal_work.cpp", "src/rail_cmd.cpp", "src/command.cpp", "src/vehicle_cmd.cpp")]}
    target = context.output("reports", f"normal-refusal-{configuration}-{scope}.json")
    target.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"report": identity(target), "observed": observed}), flush=True)
    return target


def main():
    """Run one independently bounded actual native original-world refusal scope."""
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--config", choices=("Debug", "RelWithDebInfo"), required=True)
    parser.add_argument("--scope", choices=("active", "final"), required=True)
    args = parser.parse_args()
    run_one(args.config, args.scope)


if __name__ == "__main__":
    main()
