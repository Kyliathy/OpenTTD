#!/usr/bin/env python3
"""Rebuild an actually occupied source queue and verify its native file continuation.

@file tbm_occupied_file_check.py
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md native cargo/queue ownership.
@see docs/zz-reach2/upgrades/2026-10/m2o23-2-come-space.md Group23EC.
All evidence goes to a unique test-cache/merge-supply directory. The existing
runtime materializer owns isolated configuration/content under mlm/runtime.
"""

import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "src/tests/multilayer/support"))
from runner import current_run_context
from savegame_tables import owned_chunks, saved_table


def sha256(path):
    """Hash a concrete producer/consumer artifact without rewriting it.

    @param path Existing file selected by this invocation.
    @return Lowercase full SHA256 of its actual bytes.
    """
    return hashlib.sha256(path.read_bytes()).hexdigest()


def run_child(executable, selector, environment, directory, name):
    """Run one bounded native child with complete retained output.

    @param executable Current selected native test executable.
    @param selector Exact producer or hidden ordinary-loader case.
    @param environment Explicit isolated runtime and cache ownership.
    @param directory Fresh cache directory of this invocation.
    @param name Unique report/log basename within that directory.
    @return Combined transcript and compact test report after successful exit.
    """
    report = directory / (name + ".txt")
    log = directory / (name + ".log")
    arguments = [str(executable), selector, "--reporter", "compact",
                 "--durations", "yes", "--out", str(report)]
    with log.open("xb") as output:
        result = subprocess.run(arguments, cwd=ROOT / "build", env=environment,
                                stdout=output, stderr=subprocess.STDOUT,
                                creationflags=subprocess.CREATE_NO_WINDOW if os.name == "nt" else 0,
                                timeout=110)
    transcript = log.read_text(encoding="utf-8", errors="replace")
    evidence = report.read_text(encoding="utf-8", errors="replace") if report.exists() else ""
    assert result.returncode == 0, f"{name} failed; inspect {log} and {report}"
    assert "Passed" in evidence and "No test cases matched" not in evidence, name
    return transcript + "\n" + evidence


def main():
    """Drive a fresh producer and two real application loads to the same FIFO boundary.

    @return None; failure retains exact cache artifacts and never changes retained inputs.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--executable", type=Path, default=ROOT / "build/Debug/openttd_tube_test.exe")
    arguments = parser.parse_args()
    executable = arguments.executable.resolve()
    assert executable.is_file() and executable.is_relative_to(ROOT / "build")
    context = current_run_context("tbm", "merge-occupied-file")
    directory = ROOT / "test-cache/merge-supply" / ("occupied-" + context.run_id)
    directory.mkdir(parents=True, exist_ok=False)
    print(f"Occupied-file evidence: {directory}", flush=True)
    # Preserve the original configuration/content producer. Its retained template
    # catalogue restores cold prerequisites without copying a historical runtime.
    context.prepare_runtime("grftest.cfg")
    from tbm_logistics_save_fixture import verify_initial_owners
    environment = context.environment()
    environment.update(OTTD_TBM_OCCUPIED_CACHE=str(directory), OTTD_TBM_LOGISTICS_RECORD="current-queue")
    run_child(executable, "[tbm-logistics-queue-save]", environment, directory, "producer")
    source = directory / "current-queue-occupied-bay.sav"
    assert source.is_file(), "a passing queue simulation without its actual ordinary file is insufficient"
    original_hash = sha256(source)
    assert len(saved_table(source, b"TBXA")) == 2, "the occupied queue must contain both actual assignments"
    startup = context.runtime_dir / "scripts/game_start.scr"
    original_startup = startup.read_bytes() if startup.exists() else None
    startup.write_text("# @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md; m2o23-2 Group23EC native occupied FIFO\nlogistics_probe\n", encoding="utf-8")
    results = []
    transitions = []
    try:
        # The second process loads the first normalized baseline, proving that
        # replay preserves the saved physical queue rather than regenerating it.
        for index in range(2):
            output = directory / f"reader-{index + 1}"
            loaded_source = source if index == 0 else Path(str(results[0]) + "-loaded.sav")
            child = dict(environment, OTTD_TBM_LOGISTICS_SOURCE=str(loaded_source), OTTD_TBM_LOGISTICS_OUTPUT=str(output))
            evidence = run_child(executable, "[.tbm-logistics-runtime]", child, directory, f"reader-{index + 1}")
            match = re.search(r"LOGISTICS_NEXT_EXCHANGE ticks=(\d+) exchanges=(\d+) phase=(\d+) wait=(\d+)", evidence)
            assert match, "native loader did not reach the actual holder's next legal exchange"
            transitions.append(match.groups())
            results.append(output)
        assert transitions[0] == transitions[1], "loaded FIFO transition timing/cycle changed"
        first_loaded = Path(str(results[0]) + "-loaded.sav")
        second_loaded = Path(str(results[1]) + "-loaded.sav")
        verify_initial_owners(source, first_loaded)
        verify_initial_owners(first_loaded, second_loaded)
        assert saved_table(source, b"VEHS") == saved_table(first_loaded, b"VEHS"), "first load changed actual train physics/custody"
        first_next = Path(str(results[0]) + "-next-exchange.sav")
        second_next = Path(str(results[1]) + "-next-exchange.sav")
        assert owned_chunks(first_next) == owned_chunks(second_next), "loaded FIFO result changed original packet/work/cost owners"
        assert sha256(source) == original_hash, "producer input changed during native continuation"
        report = {
            "_documentation": "archi-map-layers.md; m2o23-2-come-space.md Group23EC; actual ordinary producer and two native loads",
            "executable": str(executable), "executable_sha256": sha256(executable),
            "source": str(source), "source_sha256": original_hash,
            "transition": transitions[0], "runtime_directory": str(context.runtime_dir),
            "artifacts": [{"path": str(path), "sha256": sha256(path)} for path in
                          (first_loaded, second_loaded, first_next, second_next)],
            "passed": True,
        }
        (directory / "verification.json").write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
        print(f"Occupied FIFO passed: identical next legal exchange after {transitions[0][0]} native frames", flush=True)
    finally:
        # Only this invocation's generated startup script is restored; no shared
        # build script, retained world or original fixture is edited or removed.
        if original_startup is not None:
            startup.write_bytes(original_startup)
        else:
            startup.write_text("# @see archi-map-layers.md; m2o23-2-come-space.md Group23EC generated idle startup\n", encoding="utf-8")
        assert sha256(source) == original_hash, "immutable current producer file changed"


if __name__ == "__main__":
    main()
