"""Read exact original normal-world files through ordinary native application processes.

@file normal_ground_files.py
@see r2mlvs16-integrated-acceptance-and-balance.md Group6DDC.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Every stage is bounded independently; all126chunks remain comparison authorities.
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
from tube_tbm_files import equal_world


def run_stage(bank_path, bank_sha, origin, reader, stage):
    """Accept pure preview/read/reload and untouched whole-next equality for one actual original."""
    bank_ref = identity(ROOT / bank_path)
    assert bank_ref["sha256"] == bank_sha
    bank = json.loads((ROOT / bank_path).read_text(encoding="utf-8"))
    selected = bank["records"][origin][stage]
    original = verify_reference(selected["original"])
    run_id = datetime.now(timezone.utc).strftime("%Y%m%d-%H%M%S-") + uuid.uuid4().hex[:10]
    context = create_run_context("acceptance", "normal-ground-files", run_id=run_id)
    context.prepare_runtime()
    context.config_path.write_bytes(verify_reference(selected["config"]).read_bytes())
    startup = context.runtime_dir / "scripts/game_start.scr"
    startup.write_text("# Plan16-6DDC;actual ordinary original file reader.\nnormal_ground_file\n", encoding="utf-8")
    executable = ROOT / f"build/{reader}/openttd_test.exe"
    binary = identity(executable)
    baseline = selected.get("baseline")
    cases, inspected = [], None
    for action in ("baseline", "inspect", "next", "reload-next"):
        source_ref = inspected if action == "reload-next" else selected["original"]
        source = verify_reference(source_ref)
        label = f"{origin}-{reader}-{stage}-{action}".lower()
        environment = dict(os.environ, OTTD_NORMAL_GROUND_INPUT=str(source),
                           OTTD_NORMAL_GROUND_FILE_ACTION=action, OTTD_NORMAL_GROUND_FILE_LABEL=label)
        launch = subprocess.STARTUPINFO()
        launch.dwFlags |= subprocess.STARTF_USESHOWWINDOW
        launch.wShowWindow = subprocess.SW_HIDE
        log = context.logs / f"{label}.log"
        child = None
        command = [str(executable), "[.normal-ground-file]"]
        print(json.dumps({"phase": "native_file_started", "origin": origin, "reader": reader, "stage": stage, "action": action}), flush=True)
        try:
            with log.open("wb") as sink:
                child = start_native(context, command, cwd=ROOT / "build", env=environment,
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
            assert identity(source) == source_ref and identity(executable) == binary
        text = log.read_text(encoding="utf-8", errors="replace")
        counts = re.search(r"All tests passed \((\d+) assertions? in (\d+) test cases?\)", text)
        assert counts and int(counts[1]) > 0 and int(counts[2]) == 1, log
        assert f"NORMAL_GROUND_FILE action={action} preview={int(action != 'baseline')}" in text
        saved = {name: context.saves / f"{label}-{name}.sav" for name in ("read", "repeat", "next", "next-repeat")}
        comparisons = {"original_read": equal_world(original, saved["read"]), "pure_read": equal_world(saved["read"], saved["repeat"])}
        if action == "inspect":
            inspected = identity(saved["read"])
        outputs = {name: identity(saved[name]) for name in ("read", "repeat")}
        if action != "inspect":
            if baseline is None:
                assert action == "baseline"
                baseline = identity(saved["next"])
            comparisons["untouched_next"] = equal_world(verify_reference(baseline), saved["next"])
            comparisons["pure_next"] = equal_world(saved["next"], saved["next-repeat"])
            outputs.update({name: identity(saved[name]) for name in ("next", "next-repeat")})
        cases.append({"action": action, "input": source_ref, "outputs": outputs, "comparisons": comparisons,
                      "native": {"log": identity(log), "assertions": int(counts[1]), "cases": 1,
                                 "argv": command, "cwd": "build", "environment_selectors": {
                                     key: environment[key] for key in ("OTTD_NORMAL_GROUND_INPUT", "OTTD_NORMAL_GROUND_FILE_ACTION", "OTTD_NORMAL_GROUND_FILE_LABEL")}},
                      "children_reaped": child.poll() is not None})
    report = {"_documentation": "r2mlvs16-integrated-acceptance-and-balance.md6DDC;all126chunk ordinary preview/read/reload/full-next acceptance of exact normal originals",
              "classification": "normal_start_native_files", "bootstrap_claim": False, "bank": bank_ref, "origin": origin,
              "reader": reader, "stage": stage, "selected": selected, "baseline": baseline, "cases": cases,
              "controller": identity(__file__), "binary": binary, "config": identity(context.config_path), "startup": identity(startup),
              "sources": [identity(ROOT / name) for name in ("src/tests/normal_ground_files.cpp", "src/tests/normal_ground_video_fixture.h",
                          "src/saveload/saveload.cpp", "src/saveload/afterload.cpp", "src/openttd.cpp")],
              "native_cases": len(cases), "native_assertions": sum(row["native"]["assertions"] for row in cases),
              "complete_chunks_equal": 126, "ignored_chunks": []}
    target = context.output("reports", f"normal-files-{origin}-{reader}-{stage}.json")
    target.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"report": identity(target), "native_cases": report["native_cases"], "native_assertions": report["native_assertions"]}), flush=True)
    return target


def main():
    """Run one bounded reader/stage, leaving the immutable bank and every original untouched."""
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--bank", required=True)
    parser.add_argument("--bank-sha", required=True)
    parser.add_argument("--origin", choices=("Debug", "RelWithDebInfo"), required=True)
    parser.add_argument("--reader", choices=("Debug", "RelWithDebInfo"), required=True)
    parser.add_argument("--stage", required=True)
    args = parser.parse_args()
    run_stage(args.bank, args.bank_sha, args.origin, args.reader, args.stage)


if __name__ == "__main__":
    main()
