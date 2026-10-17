"""Execute one bounded actual resource mission segment or its declared inclusive-boundary geometry control.

@see docs/zz-reach2/upgrades/2026-10/r2mlvs10-asteroids-and-mining-probes.md Group18
Mine/return segments earn all travel/work; boundary controls only existing hull pose and never claims earned travel.
"""
import argparse
import json
import os
from pathlib import Path
import re
import runpy
import subprocess

from space_preview_checks import ROOT, chunks, identity
from artifact_paths import create_run_context
from runner import run_native

AUDIT = runpy.run_path(str(Path(__file__).with_name("orbital_runtime_checks-3.py")))
ENTRY = ROOT / "logs/probe-pursuit-verification.json"
ENTRY_SHA = "58389fe799980625900bbe16ff068a718b7efbceb7000ed45bcd5555c1007b33"
PHASES = {"mine": ("loaded",), "return": ("region", "pending", "recovered"), "boundary": ("region", "pending", "recovered"), "own": ("recovered",)}


def read(path):
    """Read explicit retained evidence.

    @param path Exact selected original index, selector or proof.
    @return Parsed evidence without directory recency selection.
    """
    return json.loads(path.read_text(encoding="utf-8"))


def predecessor(configuration):
    """Resolve the exact original empty paid hull, shells and generated finite resources.

    @param configuration Matching native executable configuration.
    @return Original dated index and actual empty paid approach checkpoint.
    """
    assert identity(ENTRY)["sha256"] == ENTRY_SHA
    selected, = [c for c in read(ENTRY)["configurations"] if c["configuration"] == configuration]
    saved = selected["checkpoints"]["native/assigned"]
    assert identity(ROOT / saved["path"]) == saved
    return identity(ENTRY), saved


def execute(configuration, target, phase, parent):
    """Execute one actual class-specific native segment with its original paid participants.

    @param configuration Current native executable build.
    @param target Original generated starter identity/class0,1,2.
    @param phase Actual mine/return, declared boundary geometry, or unchanged recovered resave.
    @param parent Exact original same-class prerequisite selector.
    @return Complete retained semantic proof identity.
    """
    dated, source = predecessor(configuration)
    parent_proof = None
    if parent:
        selected = read(parent)
        assert selected["complete"] and selected["configuration"] == configuration and selected["target"] == target
        parent_proof = selected["report"]
        assert identity(ROOT / parent_proof["path"]) == parent_proof
        proof = read(ROOT / parent_proof["path"])
        assert proof["phase"] == ("return" if phase == "own" else "mine")
        source = proof["checkpoints"]["recovered" if phase == "own" else "loaded"]
        assert identity(ROOT / source["path"]) == source
    context = create_run_context("space", "probe-class-" + phase)
    context.prepare_runtime()
    (context.runtime_dir / "scripts/game_start.scr").write_text("probe_class_mission_test\n", encoding="utf-8")
    env = dict(os.environ, OTTD_TEST_CONFIGURATION=configuration, OTTD_PROBE_CLASS_SOURCE=str(ROOT / source["path"]),
               OTTD_PROBE_CLASS_TARGET=str(target), OTTD_PROBE_CLASS_PHASE=phase)
    log = context.logs / "probe-class.log"
    with log.open("wb") as sink:
        result = run_native(context, [str(ROOT / "build" / configuration / "openttd_test.exe"), "[.probe-class-mission-native]"],
                            cwd=ROOT / "build", env=env, stdout=sink, stderr=subprocess.STDOUT, timeout=110)
    terminal = log.read_text(encoding="utf-8", errors="replace")
    match = re.search(r"All tests passed \((\d+) assertions in (\d+) test cases?\)", terminal)
    # Completed native assertions and the exact geometry marker distinguish earned route evidence from the separate controlled-boundary experiment.
    assert result.returncode == 0 and match, log
    assert f"PROBE_CLASS native=true target={target} phase={phase} boundary_geometry={str(phase == 'boundary').lower()} added_stock=false" in terminal
    checkpoints = {stage: identity(context.saves / f"class-{target}-{stage}-438.sav") for stage in PHASES[phase]}
    assert all(chunks(ROOT / saved["path"])[0] == 438 for saved in checkpoints.values())
    if phase == "own":
        AUDIT["compare"](source, checkpoints["recovered"])
    report = context.reports / "probe-class.json"
    report.write_text(json.dumps(dict(_documentation=__doc__, complete=True, configuration=configuration, target=target, phase=phase,
                                     dated_index=dated, source=source, parent=parent_proof, checkpoints=checkpoints, boundary_geometry=phase == "boundary",
                                     assertions=int(match[1]), cases=int(match[2]), added_stock=False, terminal=identity(log),
                                     native_reports=[identity(p) for p in sorted(context.reports.glob("native-*.json"))]), indent=2) + "\n", encoding="utf-8")
    return identity(report)


def main():
    """Run exactly one independently bounded native child, keeping long class pursuits out of multi-minute batches."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), required=True)
    parser.add_argument("--target", type=int, choices=(0, 1, 2), required=True)
    parser.add_argument("--phase", choices=PHASES, required=True)
    parser.add_argument("--parent", type=Path)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    assert not args.output.exists()
    assert (args.parent is not None) == (args.phase != "mine")
    report = execute(args.configuration, args.target, args.phase, args.parent)
    args.output.write_text(json.dumps(dict(_documentation=__doc__, complete=True, configuration=args.configuration,
                                         target=args.target, phase=args.phase, report=report), indent=2) + "\n", encoding="utf-8")
    print(f"passed {args.configuration} class={args.target} phase={args.phase}", flush=True)


if __name__ == "__main__":
    main()
