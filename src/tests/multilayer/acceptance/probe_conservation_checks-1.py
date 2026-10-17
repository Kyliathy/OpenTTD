"""Run exact ordinary saves and one real native continuation from five original paid mission phases.

@see docs/zz-reach2/upgrades/2026-10/r2mlvs10-asteroids-and-mining-probes.md Group17
Every resource, phase and remainder originates in accepted paid saves, without fixture installation.
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
ENTRIES = {
    "approach": ("logs/probe-pursuit-verification.json", "58389fe799980625900bbe16ff068a718b7efbceb7000ed45bcd5555c1007b33", "native/assigned"),
    "attach": ("logs/probe-pursuit-verification.json", "58389fe799980625900bbe16ff068a718b7efbceb7000ed45bcd5555c1007b33", "native/intercepted"),
    "mine": ("logs/probe-extraction-verification.json", "6785d60976c3572b3606b54b91bb66c14b523e4bb3586c121e5d39c645f233af", "native/second-partial"),
    "detach": ("logs/probe-extraction-verification.json", "6785d60976c3572b3606b54b91bb66c14b523e4bb3586c121e5d39c645f233af", "native/loaded"),
    "descent": ("logs/probe-repeat-verification.json", "4538c0a58ef7d1440788df02b1ff6355bdf8d02defbab0e67d4e4f3e7b6dc952", "native/first-pending"),
}
PHASES = tuple(ENTRIES)
SCOPES = ("native", "resume")


def read(path):
    """Read explicit retained evidence.

    @param path Exact selected original report or index.
    @return Detached parsed evidence, without recency selection.
    """
    return json.loads(path.read_text(encoding="utf-8"))


def predecessor(configuration, phase):
    """Resolve the matching original paid phase producer boundary.

    @param configuration Exact native build configuration.
    @param phase Original approach, attachment, partial mining, detached or pending descent phase.
    @return Immutable dated index and exact original paid checkpoint identities.
    """
    relative, sha, checkpoint = ENTRIES[phase]
    entry = ROOT / relative
    assert identity(entry)["sha256"] == sha
    selected, = [c for c in read(entry)["configurations"] if c["configuration"] == configuration]
    saved = selected["checkpoints"][checkpoint]
    assert identity(ROOT / saved["path"]) == saved
    return identity(entry), saved


def execute(configuration, scope, phase, parent):
    """Save the actual phase unchanged and earn exactly one original simulation opportunity.

    @param configuration Current native executable build.
    @param scope Original paid input or repeated continuation from its own ordinary save.
    @param phase Exact original saved mission phase.
    @param parent Same-phase native proof whose own save finances the repeated continuation.
    @return Complete original process proof path.
    """
    dated, source = predecessor(configuration, phase)
    if parent:
        proof = read(parent)
        assert proof["complete"] and proof["configuration"] == configuration and proof["phase"] == phase and proof["scope"] == "native"
        source = proof["own"]
        assert identity(ROOT / source["path"]) == source
    context = create_run_context("space", "probe-conservation-" + scope)
    context.prepare_runtime()
    (context.runtime_dir / "scripts/game_start.scr").write_text("probe_conservation_test\n", encoding="utf-8")
    env = dict(os.environ, OTTD_TEST_CONFIGURATION=configuration, OTTD_PROBE_CONSERVATION_SOURCE=str(ROOT / source["path"]), OTTD_PROBE_CONSERVATION_PHASE=phase)
    log = context.logs / "probe-conservation.log"
    with log.open("wb") as sink:
        result = run_native(context, [str(ROOT / "build" / configuration / "openttd_test.exe"), "[.probe-conservation-native]"],
                            cwd=ROOT / "build", env=env, stdout=sink, stderr=subprocess.STDOUT, timeout=110)
    terminal = log.read_text(encoding="utf-8", errors="replace")
    match = re.search(r"All tests passed \((\d+) assertions in (\d+) test cases?\)", terminal)
    # Successful application teardown must accompany completed native assertions and the selected semantic marker; an early console exit is insufficient.
    assert result.returncode == 0 and match, log
    assert f"PROBE_CONSERVATION native=true phase={phase} opportunities=1 added_stock=false" in terminal
    own = identity(context.saves / ("conservation-" + phase + "-own-438.sav"))
    after = identity(context.saves / ("conservation-" + phase + "-next-438.sav"))
    assert chunks(ROOT / own["path"])[0] == chunks(ROOT / after["path"])[0] == 438
    AUDIT["compare"](source, own)
    report = context.reports / "probe-conservation.json"
    report.write_text(json.dumps(dict(_documentation=__doc__, complete=True, configuration=configuration, scope=scope, phase=phase,
                                     assertions=int(match[1]), cases=int(match[2]), dated_index=dated, parent=identity(parent) if parent else None,
                                     source=source, own=own, after=after, added_stock=False, opportunities=1, terminal=identity(log),
                                     native_reports=[identity(p) for p in sorted(context.reports.glob("native-*.json"))]), indent=2) + "\n", encoding="utf-8")
    return report


def main():
    """Run five independently bounded original phases or their exact own-save continuations."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), required=True)
    parser.add_argument("--scope", choices=SCOPES, required=True)
    parser.add_argument("--parent", type=Path)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    assert not args.output.exists()
    assert (args.parent is not None) == (args.scope == "resume")
    parents = {}
    if args.parent:
        selected = read(args.parent)
        assert selected["complete"] and selected["configuration"] == args.configuration and selected["scope"] == "native"
        parents = {read(ROOT / relative)["phase"]: ROOT / relative for relative in selected["reports"]}
        assert set(parents) == set(PHASES)
    reports = []
    # Each phase starts at its own original physical boundary; one phase's cargo, work or elapsed time cannot substitute any other phase.
    for phase in PHASES:
        report = execute(args.configuration, args.scope, phase, parents.get(phase))
        reports.append(report.relative_to(ROOT).as_posix())
        print(f"passed {args.configuration} {args.scope} {phase}", flush=True)
    args.output.write_text(json.dumps(dict(_documentation=__doc__, complete=True, configuration=args.configuration, scope=args.scope, reports=reports), indent=2) + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()
