"""Execute original paid finite extraction and bounded physical participant refusals.

@see docs/zz-reach2/upgrades/2026-10/r2mlvs10-asteroids-and-mining-probes.md Group10
No native input, pose, body reserve, packet or earned work is provided by the observer.
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
PHASES = ("stopped", "partial", "first", "second-partial", "loaded")
PARENTS = dict(zip(PHASES[1:], PHASES))
SCOPES = {"native": PHASES, "own": ("stopped", "second-partial", "loaded"), "resume": ("partial", "loaded"), "focused": ("focused",)}
TAGS = "[probe-extraction],[space-approach-contract],[heavy-mining-runtime],[probe-pursuit]"


def read(path):
    """Read explicit retained evidence.

    @param path Exact original report/index/selector.
    @return Parsed evidence without newest-directory selection.
    """
    return json.loads(path.read_text(encoding="utf-8"))


def predecessor(configuration):
    """Resolve the accepted actual original paid attachment for this configuration.

    @param configuration Matching native producer configuration.
    @return Original index and actual native interception checkpoint identities.
    """
    assert identity(ENTRY)["sha256"] == ENTRY_SHA
    selected, = [c for c in read(ENTRY)["configurations"] if c["configuration"] == configuration]
    saved = selected["checkpoints"]["native/intercepted"]; assert identity(ROOT / saved["path"]) == saved
    return identity(ENTRY), saved


def execute(configuration, scope, phase, parent):
    """Run one real next action, unchanged ordinary resave or focused physical boundary.

    @param configuration Current native test executable.
    @param scope Original sequence, own-resave, resumed next action or focused components.
    @param phase Exact current stopped/work/commit/full-hold checkpoint.
    @param parent Original same-phase or preceding semantic report path.
    @return Exclusive complete semantic proof path.
    """
    dated, source = predecessor(configuration)
    if parent:
        proof = read(parent); assert proof["complete"] and proof["configuration"] == configuration
        assert proof["phase"] == (phase if scope == "own" else PARENTS[phase]); source = proof["after"]; assert identity(ROOT / source["path"]) == source
    context = create_run_context("space", "probe-extraction-" + scope); context.prepare_runtime()
    env = dict(os.environ, OTTD_TEST_CONFIGURATION=configuration); env.pop("OTTD_PROBE_EXTRACTION_RESAVE", None)
    tag = TAGS if scope == "focused" else "[.probe-extraction-native]"
    if scope != "focused":
        (context.runtime_dir / "scripts/game_start.scr").write_text("probe_extraction_test\n", encoding="utf-8")
        env.update(OTTD_PROBE_EXTRACTION_SOURCE=str(ROOT / source["path"]), OTTD_PROBE_EXTRACTION_PHASE=phase)
        if scope == "own":
            env["OTTD_PROBE_EXTRACTION_RESAVE"] = "1"
    log = context.logs / "probe-extraction.log"
    with log.open("wb") as sink:
        result = run_native(context, [str(ROOT / "build" / configuration / "openttd_test.exe"), tag], cwd=ROOT / "build", env=env,
                            stdout=sink, stderr=subprocess.STDOUT, timeout=110)
    terminal = log.read_text(encoding="utf-8", errors="replace"); match = re.search(r"All tests passed \((\d+) assertions in (\d+) test cases?\)", terminal)
    assert result.returncode == 0 and match, log; after = None
    if scope != "focused":
        assert f"PROBE_EXTRACTION native=true phase={phase} resave={str(scope == 'own').lower()} added_stock=false" in terminal
        after = identity(context.saves / ("extraction-" + phase + "-438.sav")); assert chunks(ROOT / after["path"])[0] == 438
        if scope == "own":
            AUDIT["compare"](source, after)
    report = context.reports / "probe-extraction.json"
    report.write_text(json.dumps(dict(_documentation=__doc__, complete=True, configuration=configuration, scope=scope, phase=phase,
                                     assertions=int(match[1]), cases=int(match[2]), dated_index=dated, parent=identity(parent) if parent else None,
                                     source=source if scope != "focused" else None, after=after, added_stock=False, terminal=identity(log),
                                     native_reports=[identity(p) for p in sorted(context.reports.glob("native-*.json"))]), indent=2) + "\n", encoding="utf-8")
    return report


def main():
    """Execute at most five independently bounded children in exact original prerequisite order."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), required=True)
    parser.add_argument("--scope", choices=SCOPES, required=True); parser.add_argument("--parent", type=Path)
    parser.add_argument("--output", type=Path, required=True); args = parser.parse_args(); assert not args.output.exists()
    assert (args.parent is not None) == (args.scope in ("own", "resume")); parents = {}; reports = []
    if args.parent:
        selected = read(args.parent); assert selected["configuration"] == args.configuration and selected["complete"]
        parents = {read(ROOT / p)["phase"]: ROOT / p for p in selected["reports"]}
    # Each original action consumes only its exact predecessor. Own-resaves and resumes retain explicit parents so controls and committed batches cannot be replayed inadvertently.
    for phase in SCOPES[args.scope]:
        parent_phase = phase if args.scope == "own" else PARENTS.get(phase); parent = parents.get(parent_phase)
        assert args.scope == "focused" or (parent_phase is None) == (parent is None)
        report = execute(args.configuration, args.scope, phase, parent); reports.append(report.relative_to(ROOT).as_posix())
        if args.scope == "native":
            parents[phase] = report
        print(f"passed {args.configuration} {args.scope} {phase}", flush=True)
    args.output.write_text(json.dumps(dict(_documentation=__doc__, complete=True, configuration=args.configuration, scope=args.scope, reports=reports), indent=2) + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()
