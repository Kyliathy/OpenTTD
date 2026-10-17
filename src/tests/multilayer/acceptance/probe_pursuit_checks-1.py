"""Execute bounded moving-target pursuit from the exact original paid Earth deployment.

@see docs/zz-reach2/upgrades/2026-10/r2mlvs10-asteroids-and-mining-probes.md Group9
Controlled edge/loss inputs remain isolated components; native route owners come only from accepted paid saves.
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
ENTRY = ROOT / "logs/probe-shell-verification.json"
ENTRY_SHA = "f27cd42c31421ad8f6e5b7ae2e8d807f6b16d8cbf1a4402c2612abfe61fe1ebd"
TAGS = "[probe-pursuit],[probe-shell],[asteroid-motion],[space-approach-contract]"
SCOPES = {"native": ("assigned", "intercepted"), "own": ("assigned",), "resume": ("intercepted",), "focused": ("focused",)}


def read(path):
    """Read explicit retained evidence.

    @param path Original report, index or selector.
    @return Parsed immutable evidence; no newest-directory selection.
    """
    return json.loads(path.read_text(encoding="utf-8"))


def predecessor(configuration):
    """Resolve the exact original accepted Earth-paid deployment for this configuration.

    @param configuration Matching native producer build.
    @return Original producer report and paid deployment identities.
    """
    assert identity(ENTRY)["sha256"] == ENTRY_SHA
    selected, = [c for c in read(ENTRY)["configurations"] if c["configuration"] == configuration]
    producer = selected["report"]; assert identity(ROOT / producer["path"]) == producer
    proof = read(ROOT / producer["path"]); assert proof["configuration"] == configuration
    deployed = proof["deployed"]; assert identity(ROOT / deployed["path"]) == deployed
    return producer, deployed


def execute(configuration, scope, phase, parent):
    """Run one original native action, unchanged save or directly relevant component burst.

    @param configuration Current test executable build.
    @param scope Native route, own-resave, resumed action or isolated components.
    @param phase Exact assigned/intercepted boundary, or focused burst.
    @param parent Original same-phase or prior assigned report identity, absent for original assignment/components.
    @return Original exclusive semantic report path.
    """
    producer, source = predecessor(configuration)
    if parent:
        proof = read(parent); assert proof["configuration"] == configuration and proof["phase"] == "assigned" and proof["complete"]
        source = proof["after"]; assert identity(ROOT / source["path"]) == source
    context = create_run_context("space", "probe-pursuit-" + scope); context.prepare_runtime()
    env = dict(os.environ, OTTD_TEST_CONFIGURATION=configuration); env.pop("OTTD_PROBE_PURSUIT_RESAVE", None)
    tag = TAGS if scope == "focused" else "[.probe-pursuit-native]"
    if scope != "focused":
        (context.runtime_dir / "scripts/game_start.scr").write_text("probe_pursuit_test\n", encoding="utf-8")
        env.update(OTTD_PROBE_PURSUIT_SOURCE=str(ROOT / source["path"]), OTTD_PROBE_PURSUIT_PHASE=phase)
        if scope == "own":
            env["OTTD_PROBE_PURSUIT_RESAVE"] = "1"
    log = context.logs / "probe-pursuit.log"
    with log.open("wb") as sink:
        result = run_native(context, [str(ROOT / "build" / configuration / "openttd_test.exe"), tag], cwd=ROOT / "build", env=env,
                            stdout=sink, stderr=subprocess.STDOUT, timeout=110)
    terminal = log.read_text(encoding="utf-8", errors="replace"); match = re.search(r"All tests passed \((\d+) assertions in (\d+) test cases?\)", terminal)
    assert result.returncode == 0 and match, log; after = None
    if scope != "focused":
        assert f"PROBE_PURSUIT native=true phase={phase} resave={str(scope == 'own').lower()} added_stock=false no_station=true" in terminal
        after = identity(context.saves / ("pursuit-" + phase + "-438.sav")); assert chunks(ROOT / after["path"])[0] == 438
        if scope == "own":
            AUDIT["compare"](source, after)
    report = context.reports / "probe-pursuit.json"
    report.write_text(json.dumps(dict(_documentation=__doc__, complete=True, configuration=configuration, scope=scope, phase=phase,
                                     assertions=int(match[1]), cases=int(match[2]), dated_index=identity(ENTRY), producer=producer,
                                     parent=identity(parent) if parent else None, source=source if scope != "focused" else None, after=after,
                                     added_stock=False, terminal=identity(log), native_reports=[identity(p) for p in sorted(context.reports.glob("native-*.json"))]), indent=2) + "\n", encoding="utf-8")
    return report


def main():
    """Execute at most two original native children in their exact prerequisite order."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), required=True)
    parser.add_argument("--scope", choices=SCOPES, required=True); parser.add_argument("--parent", type=Path)
    parser.add_argument("--output", type=Path, required=True); args = parser.parse_args(); assert not args.output.exists()
    assert (args.parent is not None) == (args.scope in ("own", "resume")); parent = None; reports = []
    if args.parent:
        selected = read(args.parent); assert selected["configuration"] == args.configuration and selected["complete"]
        parent, = [ROOT / p for p in selected["reports"] if read(ROOT / p)["phase"] == "assigned"]
    # Only the original native sequence chains its own actions. Own/resumed scopes retain their explicit original parent and never replay completed assignment controls.
    for phase in SCOPES[args.scope]:
        report = execute(args.configuration, args.scope, phase, parent); reports.append(report.relative_to(ROOT).as_posix())
        if args.scope == "native":
            parent = report
        print(f"passed {args.configuration} {args.scope} {phase}", flush=True)
    args.output.write_text(json.dumps(dict(_documentation=__doc__, complete=True, configuration=args.configuration, scope=args.scope, reports=reports), indent=2) + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()
