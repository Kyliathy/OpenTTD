"""Run bounded original paid mission twins under native view selection and synchronized command dispatch.

@see docs/zz-reach2/upgrades/2026-10/r2mlvs10-asteroids-and-mining-probes.md Group20
Every packet, work interval, shell, finite reserve and monetary receipt remains under its native owner.
"""
import argparse
import json
import os
from pathlib import Path
import re
import runpy
import subprocess

from space_preview_checks import ROOT, identity, chunks
from artifact_paths import create_run_context
from runner import run_native

HERE = Path(__file__).parent
SURFACE = runpy.run_path(str(HERE / "probe_surface_delivery_checks-1.py"))
AUDIT = SURFACE["AUDIT"]
# Native finite ground reserves, natural source geometry, mine counters, packed map/layer storage and economy/time/RNG owners must converge too; orbital agreement cannot conceal a terrestrial resource or synchronized clock divergence.
WORLD = (b"GEOS", b"GDEP", b"GSPN", b"GVNT", b"LMNH", b"LMNR", b"MLMD", b"ECMY", b"DATE",
         b"MAPS", b"MAPT", b"MAPH", b"MAPO", b"MAP2", b"M3LO", b"M3HI", b"MAP5", b"MAPE", b"MAP7", b"MAP8")
OWNERS = (*AUDIT["PHYSICAL"], *SURFACE["INDUSTRY"], *WORLD)
METADATA = AUDIT["METADATA"]
STAGES = ("assigned", "partial", "loaded", "region", "pending", "recovered", "own")
RECALL = ("recalled", "region", "pending", "recovered", "own")
TAGS = "[probe-mission-generation],[probe-exits],[space-order-contract]"
INDEXES = {"mission": ("probe-shell", "f27cd42c31421ad8f6e5b7ae2e8d807f6b16d8cbf1a4402c2612abfe61fe1ebd"),
           "recall": ("probe-extraction", "6785d60976c3572b3606b54b91bb66c14b523e4bb3586c121e5d39c645f233af")}


def read(path):
    """Read explicit retained evidence.

    @param path Concrete selected index, report or selector.
    @return Parsed evidence without filename recency selection.
    """
    return json.loads(path.read_text(encoding="utf-8"))


def predecessor(configuration, recall=False):
    """Resolve original paid empty deployment or earned two-unit partial extraction.

    @param configuration Matching native producer configuration.
    @param recall Whether the original earned two-unit cancellation input is required.
    @return Exact dated index and original native save, without historical executable/source requalification.
    """
    name, digest = INDEXES["recall" if recall else "mission"]
    index = ROOT / f"logs/{name}-verification.json"
    assert identity(index)["sha256"] == digest
    accepted, = [c for c in read(index)["configurations"] if c["configuration"] == configuration]
    if recall:
        saved = accepted["checkpoints"]["native/second-partial"]
    else:
        original = accepted["report"]
        assert identity(ROOT / original["path"]) == original
        saved = read(ROOT / original["path"])["deployed"]
    assert identity(ROOT / saved["path"]) == saved
    return identity(index), saved


def compare(left, right):
    """Compare original physical, financial and metadata owners byte for byte.

    @param left Exact expected original checkpoint identity.
    @param right Matching actual view, dispatch, peer or unchanged resave identity.
    @return Number of owner-family and metadata comparisons.
    """
    assert identity(ROOT / left["path"]) == left and identity(ROOT / right["path"]) == right
    a, b = chunks(ROOT / left["path"]), chunks(ROOT / right["path"])
    assert a[0] == b[0] == 438
    # Native Industry input/history, pending payment custody and company money join all orbital/Surface packet owners; matching RNG or a displayed hold alone is insufficient.
    for family in (*OWNERS, *METADATA):
        assert a[1][family] == b[1][family], (family, left, right)
    return len(OWNERS), len(METADATA)


def result(log):
    """Require real Catch success and retain exact checkpoint frame witnesses.

    @param log Actual invocation-owned native terminal log.
    @return Passing assertion/case counts and ordered actual frame/tick/field boundaries.
    """
    terminal = log.read_text(encoding="utf-8", errors="replace")
    match = re.search(r"All tests passed \((\d+) assertions in (\d+) test cases?\)", terminal)
    assert match, log
    witnesses = {stage: dict(frame=int(frame), tick=int(tick), elapsed=int(elapsed)) for stage, frame, tick, elapsed in
                 re.findall(r"PROBE_REPLAY checkpoint=(\w+) frame=(\d+) tick=(\d+) elapsed=(\d+)", terminal)}
    return int(match[1]), int(match[2]), witnesses


def execute(configuration, mode, view):
    """Run one directly relevant component burst or complete original paid native view/transport twin.

    @param configuration Current native executable configuration.
    @param mode Focused, local, actual queued dispatch, or queued original partial Recall.
    @param view Actual visible Space or hidden Surface viewport policy.
    @return Complete semantic process proof identity.
    """
    context = create_run_context("space", f"probe-replay-{mode}-{view}")
    context.prepare_runtime()
    env = dict(os.environ, OTTD_TEST_CONFIGURATION=configuration)
    dated = source = None
    if mode != "focused":
        dated, source = predecessor(configuration, mode == "recall-queue")
        (context.runtime_dir / "scripts/game_start.scr").write_text("probe_replay_test\n", encoding="utf-8")
        env.update(OTTD_PROBE_REPLAY_SOURCE=str(ROOT / source["path"]), OTTD_PROBE_REPLAY_MODE=mode, OTTD_PROBE_REPLAY_VIEW=view)
    log = context.logs / "probe-replay.log"
    with log.open("wb") as sink:
        status = run_native(context, [str(ROOT / "build" / configuration / "openttd_test.exe"), TAGS if mode == "focused" else "[.probe-replay-native]"],
                            cwd=ROOT / "build", env=env, stdout=sink, stderr=subprocess.STDOUT, timeout=110)
    assert status.returncode == 0, log
    assertions, cases, witnesses = result(log)
    checkpoints = {}
    if mode != "focused":
        stages = RECALL if mode == "recall-queue" else STAGES
        assert tuple(witnesses) == stages and cases == 1, (log, witnesses)
        checkpoints = {stage: identity(context.saves / f"replay-{stage}-438.sav") for stage in stages}
        compare(checkpoints["recovered"], checkpoints["own"])
        assert len(list(context.images.glob("*.ppm"))) == len(stages) - 1
        assert identity(ROOT / source["path"]) == source
    report = context.reports / "probe-replay.json"
    report.write_text(json.dumps(dict(_documentation=__doc__, complete=True, configuration=configuration, mode=mode, view=view,
                                     dated_index=dated, source=source, checkpoints=checkpoints, witnesses=witnesses,
                                     assertions=assertions, cases=cases, added_stock=False, terminal=identity(log),
                                     captures=[identity(p) for p in sorted(context.images.glob("*.ppm"))],
                                     native_reports=[identity(p) for p in sorted(context.reports.glob("native-*.json"))]), indent=2) + "\n", encoding="utf-8")
    return identity(report)


def main():
    """Execute one bounded scope and freeze its explicit selector only after native success."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), required=True)
    parser.add_argument("--mode", choices=("focused", "local", "queue", "recall-queue"), required=True)
    parser.add_argument("--view", choices=("visible", "hidden"), default="hidden")
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    assert not args.output.exists()
    proof = execute(args.configuration, args.mode, args.view)
    args.output.write_text(json.dumps(dict(_documentation=__doc__, complete=True, configuration=args.configuration,
                                         mode=args.mode, view=args.view, report=proof), indent=2) + "\n", encoding="utf-8")
    print(f"passed {args.configuration} {args.mode} {args.view}", flush=True)


if __name__ == "__main__":
    main()
