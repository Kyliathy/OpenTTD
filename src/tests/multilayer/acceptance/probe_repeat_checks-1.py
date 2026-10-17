"""Execute original paid descent, shell resupply and earned service before repeated mining.

@see docs/zz-reach2/upgrades/2026-10/r2mlvs10-asteroids-and-mining-probes.md Group12
All physical resources and service work retain their original paid native owners.
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
ENTRY = ROOT / "logs/probe-return-verification.json"
ENTRY_SHA = "b1c57dee86998772c8dad1b13a091f432656ee6ee39840ef6d6a25e4e2e47953"
PHASES = ("first-pending", "first-recovered", "second-pending", "exhausted", "resupplied", "service-partial", "service-complete", "third-assigned")
PARENTS = dict(zip(PHASES[1:], PHASES))
SCOPES = {"native-a": PHASES[:4], "native-b": PHASES[4:], "own": ("first-pending", "service-partial", "service-complete"),
          "resume": ("first-recovered", "service-complete", "third-assigned"), "focused": ("focused",)}
TAGS = "[probe-order-actions],[space-logistics-contract],[descent-custody-contract]"


def read(path):
    """Read explicit retained evidence.

    @param path Exact original report/index/selector.
    @return Parsed evidence without newest-directory selection.
    """
    return json.loads(path.read_text(encoding="utf-8"))


def predecessor(configuration):
    """Resolve the accepted actual original paid release-region hold for this configuration.

    @param configuration Matching native producer configuration.
    @return Original index and actual retained-cargo checkpoint identities.
    """
    assert identity(ENTRY)["sha256"] == ENTRY_SHA
    selected, = [c for c in read(ENTRY)["configurations"] if c["configuration"] == configuration]
    saved = selected["checkpoints"]["native/region"]; assert identity(ROOT / saved["path"]) == saved
    return identity(ENTRY), saved


def execute(configuration, scope, phase, parent):
    """Run one real next action, unchanged ordinary resave or focused physical boundary.

    @param configuration Current native test executable.
    @param scope Original sequence, own-resave, resumed next action or focused components.
    @param phase Exact pending/recovered/resupplied/service/rendezvous boundary.
    @param parent Original same-phase or preceding semantic report path.
    @return Exclusive complete semantic proof path.
    """
    dated, source = predecessor(configuration)
    if parent:
        proof = read(parent); assert proof["complete"] and proof["configuration"] == configuration
        assert proof["phase"] == (phase if scope == "own" else PARENTS[phase]); source = proof["after"]; assert identity(ROOT / source["path"]) == source
    context = create_run_context("space", "probe-repeat-" + scope); context.prepare_runtime()
    env = dict(os.environ, OTTD_TEST_CONFIGURATION=configuration); env.pop("OTTD_PROBE_REPEAT_RESAVE", None)
    tag = TAGS if scope == "focused" else "[.probe-repeat-native]"
    if scope != "focused":
        (context.runtime_dir / "scripts/game_start.scr").write_text("probe_repeat_test\n", encoding="utf-8")
        env.update(OTTD_PROBE_REPEAT_SOURCE=str(ROOT / source["path"]), OTTD_PROBE_REPEAT_PHASE=phase)
        if scope == "own":
            env["OTTD_PROBE_REPEAT_RESAVE"] = "1"
    log = context.logs / "probe-repeat.log"
    with log.open("wb") as sink:
        result = run_native(context, [str(ROOT / "build" / configuration / "openttd_test.exe"), tag], cwd=ROOT / "build", env=env,
                            stdout=sink, stderr=subprocess.STDOUT, timeout=110)
    terminal = log.read_text(encoding="utf-8", errors="replace"); match = re.search(r"All tests passed \((\d+) assertions in (\d+) test cases?\)", terminal)
    assert result.returncode == 0 and match, log; after = None
    if scope != "focused":
        assert f"PROBE_REPEAT native=true phase={phase} resave={str(scope == 'own').lower()} added_stock=false" in terminal
        after = identity(context.saves / ("repeat-" + phase + "-438.sav")); assert chunks(ROOT / after["path"])[0] == 438
        if scope == "own":
            AUDIT["compare"](source, after)
    report = context.reports / "probe-repeat.json"
    report.write_text(json.dumps(dict(_documentation=__doc__, complete=True, configuration=configuration, scope=scope, phase=phase,
                                     assertions=int(match[1]), cases=int(match[2]), dated_index=dated, parent=identity(parent) if parent else None,
                                     source=source if scope != "focused" else None, after=after, added_stock=False, terminal=identity(log),
                                     native_reports=[identity(p) for p in sorted(context.reports.glob("native-*.json"))]), indent=2) + "\n", encoding="utf-8")
    return report


def main():
    """Execute at most four independently bounded children in original prerequisite order."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), required=True)
    parser.add_argument("--scope", choices=SCOPES, required=True); parser.add_argument("--parent", type=Path, nargs="+", default=[])
    parser.add_argument("--output", type=Path, required=True); args = parser.parse_args(); assert not args.output.exists()
    parents = {}; reports = []
    # Explicit selectors retain exact action ancestry; original and resumed branches cannot silently select a newest world or replay a committed release.
    for path in args.parent:
        selected = read(path); assert selected["configuration"] == args.configuration and selected["complete"]
        parents.update({read(ROOT / p)["phase"]: ROOT / p for p in selected["reports"]})
    # Each stage consumes its exact preceding custody/service boundary. Ordinary own-resaves perform no new action.
    for phase in SCOPES[args.scope]:
        parent_phase = phase if args.scope == "own" else PARENTS.get(phase); parent = parents.get(parent_phase)
        assert args.scope == "focused" or (parent_phase is None) == (parent is None)
        report = execute(args.configuration, args.scope, phase, parent); reports.append(report.relative_to(ROOT).as_posix())
        if args.scope.startswith("native"):
            parents[phase] = report
        print(f"passed {args.configuration} {args.scope} {phase}", flush=True)
    args.output.write_text(json.dumps(dict(_documentation=__doc__, complete=True, configuration=args.configuration, scope=args.scope, reports=reports), indent=2) + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()
