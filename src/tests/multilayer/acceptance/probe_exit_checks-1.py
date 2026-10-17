"""Execute original paid loaded/attached recall and retained active service boundaries.

@see docs/zz-reach2/upgrades/2026-10/r2mlvs10-asteroids-and-mining-probes.md Group13
Actual paid worlds supply all original cargo, extraction history, shells and service work.
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
    "loaded": ("logs/probe-return-verification.json", "b1c57dee86998772c8dad1b13a091f432656ee6ee39840ef6d6a25e4e2e47953", "native/region"),
    "partial": ("logs/probe-extraction-verification.json", "6785d60976c3572b3606b54b91bb66c14b523e4bb3586c121e5d39c645f233af", "native/second-partial"),
    "service": ("logs/probe-repeat-verification.json", "4538c0a58ef7d1440788df02b1ff6355bdf8d02defbab0e67d4e4f3e7b6dc952", "native/service-partial"),
}
PHASES = tuple(ENTRIES)
SCOPES = {"native": PHASES, "own": PHASES, "focused": ("focused",)}
TAGS = "[probe-exits],[probe-pursuit],[probe-return],[probe-order-actions],[descent-custody-contract],[heavy-mining-runtime]"


def read(path):
    """Read explicit retained evidence.

    @param path Exact original report/index/selector.
    @return Parsed evidence without newest-directory selection.
    """
    return json.loads(path.read_text(encoding="utf-8"))


def predecessor(configuration, phase):
    """Resolve this exact phase's original accepted paid producer boundary.

    @param configuration Matching native executable configuration.
    @param phase Explicit full hold, partial attachment or retained service phase.
    @return Original dated index and actual same-phase input identity.
    """
    relative, sha, checkpoint = ENTRIES["loaded" if phase == "focused" else phase]
    entry = ROOT / relative; assert identity(entry)["sha256"] == sha
    selected, = [c for c in read(entry)["configurations"] if c["configuration"] == configuration]
    saved = selected["checkpoints"][checkpoint]; assert identity(ROOT / saved["path"]) == saved
    return identity(entry), saved


def execute(configuration, scope, phase, parent):
    """Run one real next action, unchanged ordinary resave or focused physical boundary.

    @param configuration Current native test executable.
    @param scope Original sequence, own-resave, resumed next action or focused components.
    @param phase Original full hold, partial attachment or active service boundary.
    @param parent Exact same-phase native result for an unchanged ordinary resave.
    @return Exclusive complete semantic proof path.
    """
    dated, source = predecessor(configuration, phase)
    if parent:
        proof = read(parent); assert proof["complete"] and proof["configuration"] == configuration
        assert proof["phase"] == phase; source = proof["after"]; assert identity(ROOT / source["path"]) == source
    context = create_run_context("space", "probe-exit-" + scope); context.prepare_runtime()
    env = dict(os.environ, OTTD_TEST_CONFIGURATION=configuration); env.pop("OTTD_PROBE_EXIT_RESAVE", None)

    tag = TAGS if scope == "focused" else "[.probe-exit-native]"
    if scope != "focused":
        (context.runtime_dir / "scripts/game_start.scr").write_text("probe_exit_test\n", encoding="utf-8")
        env.update(OTTD_PROBE_EXIT_SOURCE=str(ROOT / source["path"]), OTTD_PROBE_EXIT_PHASE=phase)
        if scope == "own":
            env["OTTD_PROBE_EXIT_RESAVE"] = "1"
    log = context.logs / "probe-exit.log"
    with log.open("wb") as sink:
        result = run_native(context, [str(ROOT / "build" / configuration / "openttd_test.exe"), tag], cwd=ROOT / "build", env=env,
                            stdout=sink, stderr=subprocess.STDOUT, timeout=110)
    terminal = log.read_text(encoding="utf-8", errors="replace"); match = re.search(r"All tests passed \((\d+) assertions in (\d+) test cases?\)", terminal)
    assert result.returncode == 0 and match, log; after = None
    if scope != "focused":
        assert f"PROBE_EXIT native=true phase={phase} resave={str(scope == 'own').lower()} added_stock=false" in terminal
        after = identity(context.saves / ("exit-" + phase + "-438.sav")); assert chunks(ROOT / after["path"])[0] == 438
        if scope == "own":
            AUDIT["compare"](source, after)
    report = context.reports / "probe-exit.json"
    report.write_text(json.dumps(dict(_documentation=__doc__, complete=True, configuration=configuration, scope=scope, phase=phase,
                                     assertions=int(match[1]), cases=int(match[2]), dated_index=dated, parent=identity(parent) if parent else None,
                                     source=source if scope != "focused" else None, after=after, added_stock=False, terminal=identity(log),
                                     native_reports=[identity(p) for p in sorted(context.reports.glob("native-*.json"))]), indent=2) + "\n", encoding="utf-8")
    return report


def main():
    """Execute at most three independently bounded children in exact original prerequisite order."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), required=True)
    parser.add_argument("--scope", choices=SCOPES, required=True); parser.add_argument("--parent", type=Path)
    parser.add_argument("--output", type=Path, required=True); args = parser.parse_args(); assert not args.output.exists()
    assert (args.parent is not None) == (args.scope == "own"); parents = {}; reports = []
    if args.parent:
        selected = read(args.parent); assert selected["configuration"] == args.configuration and selected["complete"]
        parents = {read(ROOT / p)["phase"]: ROOT / p for p in selected["reports"]}
    # Each native case starts from its own exact producer. Ordinary saves consume that same case's result and cannot repeat a recall or earn service.
    for phase in SCOPES[args.scope]:
        report = execute(args.configuration, args.scope, phase, parents.get(phase)); reports.append(report.relative_to(ROOT).as_posix())
        print(f"passed {args.configuration} {args.scope} {phase}", flush=True)
    args.output.write_text(json.dumps(dict(_documentation=__doc__, complete=True, configuration=args.configuration, scope=args.scope, reports=reports), indent=2) + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()
