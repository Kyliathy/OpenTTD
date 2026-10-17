"""Run bounded native pursuit, request ties and explicit edge geometry over original paid owners.

@see docs/zz-reach2/upgrades/2026-10/r2mlvs10-asteroids-and-mining-probes.md Group15
Geometry forks claim no earned travel, procurement, added stock or artificial work.
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
    "latched": ("logs/probe-extraction-verification.json", "6785d60976c3572b3606b54b91bb66c14b523e4bb3586c121e5d39c645f233af", "native/second-partial"),
}
PHASES = ("approach-stage", "approach-next", "latched-stage", "latched-next")
PARENTS = {"approach-next": "approach-stage", "latched-next": "latched-stage"}
SCOPES = {"native": PHASES, "own": ("approach-stage", "latched-stage"), "resume": tuple(PARENTS), "focused": ("focused",)}
TAGS = "[probe-interception],[asteroid-motion],[space-approach-contract],[probe-pursuit]"


def read(path):
    """Read retained explicit evidence.

    @param path Exact selected report or index.
    @return Parsed evidence without directory recency selection.
    """
    return json.loads(path.read_text(encoding="utf-8"))


def predecessor(configuration, phase):
    """Resolve the original paid approach or partial extraction producer.

    @param configuration Matching native build configuration.
    @param phase Explicit original approach/latched phase, or focused components.
    @return Dated producer index and exact original paid input identities.
    """
    family = "approach" if phase == "focused" else phase.split("-")[0]
    relative, sha, checkpoint = ENTRIES[family]
    entry = ROOT / relative
    assert identity(entry)["sha256"] == sha
    selected, = [c for c in read(entry)["configurations"] if c["configuration"] == configuration]
    saved = selected["checkpoints"][checkpoint]
    assert identity(ROOT / saved["path"]) == saved
    return identity(entry), saved


def execute(configuration, scope, phase, parent):
    """Execute geometry staging, one real native opportunity, an unchanged resave or focused pursuit.

    @param configuration Current native test build.
    @param scope Explicit native, own, resumed or focused evidence boundary.
    @param phase Approach/latched stage or its next native opportunity.
    @param parent Exact prerequisite stage proof; no alternate resource owner is selected.
    @return Complete semantic proof path in an exclusive retained run.
    """
    dated, source = predecessor(configuration, phase)
    if parent:
        proof = read(parent)
        assert proof["complete"] and proof["configuration"] == configuration
        assert proof["phase"] == (phase if scope == "own" else PARENTS[phase])
        source = proof["after"]
        assert identity(ROOT / source["path"]) == source
    context = create_run_context("space", "probe-interception-" + scope)
    context.prepare_runtime()
    env = dict(os.environ, OTTD_TEST_CONFIGURATION=configuration)
    env.pop("OTTD_PROBE_INTERCEPTION_RESAVE", None)
    tag = TAGS if scope == "focused" else "[.probe-interception-native]"
    if scope != "focused":
        (context.runtime_dir / "scripts/game_start.scr").write_text("probe_interception_test\n", encoding="utf-8")
        env.update(OTTD_PROBE_INTERCEPTION_SOURCE=str(ROOT / source["path"]), OTTD_PROBE_INTERCEPTION_PHASE=phase)
        if scope == "own":
            env["OTTD_PROBE_INTERCEPTION_RESAVE"] = "1"
    log = context.logs / "probe-interception.log"
    with log.open("wb") as sink:
        result = run_native(context, [str(ROOT / "build" / configuration / "openttd_test.exe"), tag], cwd=ROOT / "build", env=env,
                            stdout=sink, stderr=subprocess.STDOUT, timeout=110)
    terminal = log.read_text(encoding="utf-8", errors="replace")
    match = re.search(r"All tests passed \((\d+) assertions in (\d+) test cases?\)", terminal)
    # A successful process must also have completed the selected Catch boundary; an early console exit cannot substitute saved-state evidence.
    assert result.returncode == 0 and match, log
    after = None
    if scope != "focused":
        assert f"PROBE_INTERCEPTION native=true phase={phase} resave={str(scope == 'own').lower()} geometry_fixture=true added_stock=false" in terminal
        after = identity(context.saves / ("interception-" + phase + "-438.sav"))
        assert chunks(ROOT / after["path"])[0] == 438
        if scope == "own":
            AUDIT["compare"](source, after)
    report = context.reports / "probe-interception.json"
    report.write_text(json.dumps(dict(_documentation=__doc__, complete=True, configuration=configuration, scope=scope, phase=phase,
                                     assertions=int(match[1]), cases=int(match[2]), dated_index=dated, parent=identity(parent) if parent else None,
                                     source=source if scope != "focused" else None, after=after, geometry_fixture=True, earned_paid_travel=False,
                                     added_stock=False, terminal=identity(log),
                                     native_reports=[identity(p) for p in sorted(context.reports.glob("native-*.json"))]), indent=2) + "\n", encoding="utf-8")
    return report


def main():
    """Execute at most four independently bounded children in exact prerequisite order."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), required=True)
    parser.add_argument("--scope", choices=SCOPES, required=True)
    parser.add_argument("--parent", type=Path)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    assert not args.output.exists()
    assert (args.parent is not None) == (args.scope in ("own", "resume"))
    parents = {}
    reports = []
    if args.parent:
        selected = read(args.parent)
        # Own saves consume native stages, and resumed steps consume only those own stages. This prevents replaying staging in place of genuine continuation.
        assert selected["complete"] and selected["configuration"] == args.configuration and selected["scope"] == ("native" if args.scope == "own" else "own")
        parents = {read(ROOT / p)["phase"]: ROOT / p for p in selected["reports"]}
    # Each family starts at its exact original paid boundary; only an actual produced stage can finance its subsequent reflected native opportunity.
    for phase in SCOPES[args.scope]:
        parent = parents.get(phase if args.scope == "own" else PARENTS.get(phase, ""))
        report = execute(args.configuration, args.scope, phase, parent)
        parents[phase] = report
        reports.append(report.relative_to(ROOT).as_posix())
        print(f"passed {args.configuration} {args.scope} {phase}", flush=True)
    args.output.write_text(json.dumps(dict(_documentation=__doc__, complete=True, configuration=args.configuration, scope=args.scope, reports=reports), indent=2) + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()
