"""Execute original paid exit, cancellation and guarded recovery identity boundaries.

@see docs/zz-reach2/upgrades/2026-10/r2mlvs10-asteroids-and-mining-probes.md Group16
Actual paid worlds supply all original cargo, extraction history, shells and service work; no geometry, reserve or progress is installed.
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
    "exhausted": ("logs/probe-repeat-verification.json", "4538c0a58ef7d1440788df02b1ff6355bdf8d02defbab0e67d4e4f3e7b6dc952", "native/exhausted"),
    "approach": ("logs/probe-pursuit-verification.json", "58389fe799980625900bbe16ff068a718b7efbceb7000ed45bcd5555c1007b33", "native/assigned"),
    "partial": ("logs/probe-extraction-verification.json", "6785d60976c3572b3606b54b91bb66c14b523e4bb3586c121e5d39c645f233af", "native/second-partial"),
    "detached": ("logs/probe-extraction-verification.json", "6785d60976c3572b3606b54b91bb66c14b523e4bb3586c121e5d39c645f233af", "native/loaded"),
    "pending": ("logs/probe-repeat-verification.json", "4538c0a58ef7d1440788df02b1ff6355bdf8d02defbab0e67d4e4f3e7b6dc952", "native/first-pending"),
    "home": ("logs/probe-repeat-verification.json", "4538c0a58ef7d1440788df02b1ff6355bdf8d02defbab0e67d4e4f3e7b6dc952", "native/first-pending"),
}
PHASES = tuple(ENTRIES)
SCOPES = {"native": PHASES, "own": PHASES, "focused": ("focused",)}
TAGS = "[probe-boundaries],[probe-exits],[probe-pursuit],[probe-extraction],[descent-custody-contract]"


def read(path):
    """Read explicit retained evidence.

    @param path Exact original report/index/selector.
    @return Parsed evidence without newest-directory selection.
    """
    return json.loads(path.read_text(encoding="utf-8"))


def predecessor(configuration, phase):
    """Resolve this exact phase's original accepted paid producer boundary.

    @param configuration Matching native executable configuration.
    @param phase Explicit paid exit, cancellation or guarded recovery boundary.
    @return Original dated index and actual same-phase input identity.
    """
    relative, sha, checkpoint = ENTRIES["exhausted" if phase == "focused" else phase]
    entry = ROOT / relative; assert identity(entry)["sha256"] == sha
    selected, = [c for c in read(entry)["configurations"] if c["configuration"] == configuration]
    saved = selected["checkpoints"][checkpoint]; assert identity(ROOT / saved["path"]) == saved
    return identity(entry), saved


def execute(configuration, scope, phase, parent):
    """Run one real next action, unchanged ordinary resave or focused physical boundary.

    @param configuration Current native test executable.
    @param scope Original boundary, unchanged own-resave or focused components.
    @param phase Exact spent allowance, approach/mining/detachment/pending cancellation or home guard.
    @param parent Exact same-phase native result for an unchanged ordinary resave.
    @return Exclusive complete semantic proof path.
    """
    dated, source = predecessor(configuration, phase)
    if parent:
        proof = read(parent); assert proof["complete"] and proof["configuration"] == configuration
        assert proof["phase"] == phase; source = proof["after"]; assert identity(ROOT / source["path"]) == source
    context = create_run_context("space", "probe-boundary-" + scope); context.prepare_runtime()
    env = dict(os.environ, OTTD_TEST_CONFIGURATION=configuration); env.pop("OTTD_PROBE_BOUNDARY_RESAVE", None)

    tag = TAGS if scope == "focused" else "[.probe-boundary-native]"
    if scope != "focused":
        (context.runtime_dir / "scripts/game_start.scr").write_text("probe_boundary_test\n", encoding="utf-8")
        env.update(OTTD_PROBE_BOUNDARY_SOURCE=str(ROOT / source["path"]), OTTD_PROBE_BOUNDARY_PHASE=phase)
        if scope == "own":
            env["OTTD_PROBE_BOUNDARY_RESAVE"] = "1"
    log = context.logs / "probe-boundary.log"
    with log.open("wb") as sink:
        result = run_native(context, [str(ROOT / "build" / configuration / "openttd_test.exe"), tag], cwd=ROOT / "build", env=env,
                            stdout=sink, stderr=subprocess.STDOUT, timeout=110)
    terminal = log.read_text(encoding="utf-8", errors="replace"); match = re.search(r"All tests passed \((\d+) assertions in (\d+) test cases?\)", terminal)
    assert result.returncode == 0 and match, log; after = None
    if scope != "focused":
        assert f"PROBE_BOUNDARY native=true phase={phase} resave={str(scope == 'own').lower()} added_stock=false" in terminal
        after = identity(context.saves / ("boundary-" + phase + "-438.sav")); assert chunks(ROOT / after["path"])[0] == 438
        if scope == "own":
            AUDIT["compare"](source, after)
    report = context.reports / "probe-boundary.json"
    report.write_text(json.dumps(dict(_documentation=__doc__, complete=True, configuration=configuration, scope=scope, phase=phase,
                                     assertions=int(match[1]), cases=int(match[2]), dated_index=dated, parent=identity(parent) if parent else None,
                                     source=source if scope != "focused" else None, after=after, added_stock=False, terminal=identity(log),
                                     native_reports=[identity(p) for p in sorted(context.reports.glob("native-*.json"))]), indent=2) + "\n", encoding="utf-8")
    return report


def main():
    """Execute at most six independently bounded children in exact original prerequisite order."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), required=True)
    parser.add_argument("--scope", choices=SCOPES, required=True); parser.add_argument("--parent", type=Path)
    parser.add_argument("--output", type=Path, required=True); args = parser.parse_args(); assert not args.output.exists()
    assert (args.parent is not None) == (args.scope == "own"); parents = {}; reports = []
    if args.parent:
        selected = read(args.parent)
        assert selected["configuration"] == args.configuration and selected["complete"] and selected["scope"] == "native"
        parents = {read(ROOT / p)["phase"]: ROOT / p for p in selected["reports"]}
    # Each native case starts from its own exact producer. Ordinary saves consume that same case's result and cannot repeat cancellation or earn service.
    for phase in SCOPES[args.scope]:
        report = execute(args.configuration, args.scope, phase, parents.get(phase)); reports.append(report.relative_to(ROOT).as_posix())
        print(f"passed {args.configuration} {args.scope} {phase}", flush=True)
    args.output.write_text(json.dumps(dict(_documentation=__doc__, complete=True, configuration=args.configuration, scope=args.scope, reports=reports), indent=2) + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()
