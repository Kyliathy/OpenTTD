"""Run bounded exact-population native field or probe component measurements.

@see docs/zz-reach2/upgrades/2026-10/r2mlvs10-asteroids-and-mining-probes.md Group21
Declared isolated inputs claim no paid procurement, whole-game frame rate or maximum-map Surface startup measurement.
"""
import argparse
import json
import os
from pathlib import Path
import re
import subprocess

from space_preview_checks import ROOT, identity
from artifact_paths import create_run_context
from runner import run_native

CASES = ("small", "normal", "largest-normal", "largest-dense", "clustered", "narrow-tall", "narrow-wide")
TAGS = "[space-preview],[asteroid-generation],[probe-extraction],[probe-mission-generation]"


def read(path):
    """Read one explicitly selected evidence file.

    @param path Concrete repository evidence path.
    @return Parsed JSON without recency or wildcard selection.
    """
    return json.loads(path.read_text(encoding="utf-8"))


def execute(configuration, domain, case):
    """Run one process under the original invocation-owned native harness.

    @param configuration Current executable build.
    @param domain Actual field/index, native probes, or directly relevant producer tests.
    @param case Exact population/geometry input; unused by focused producer tests.
    @return Hash-bound semantic report identity with the actual native process witness.
    """
    context = create_run_context("space", f"asteroid-work-{domain}-{case}")
    context.prepare_runtime()
    tag = TAGS if domain == "focused" else f"[.asteroid-{'field' if domain == 'field' else 'probe'}-work]"
    log = context.logs / "asteroid-work.log"
    env = dict(os.environ, OTTD_TEST_CONFIGURATION=configuration, OTTD_ASTEROID_WORK_CASE=case)
    with log.open("wb") as sink:
        status = run_native(context, [str(ROOT / "build" / configuration / "openttd_test.exe"), tag], cwd=ROOT / "build",
                            env=env, stdout=sink, stderr=subprocess.STDOUT, timeout=110)
    assert status.returncode == 0, log
    terminal = log.read_text(encoding="utf-8", errors="replace")
    success = re.search(r"All tests passed \((\d+) assertions in (\d+) test cases?\)", terminal)
    assert success, log
    measurement = None
    if domain != "focused":
        measured = context.reports / f"asteroid-work-{domain}.json"
        data = read(measured)
        assert data["case"] == case and data["domain"] == domain and int(success[2]) == 1
        assert f"ASTEROID_WORK case={case} domain={domain} bodies={data['bodies']} native=true paid=false" in terminal
        measurement = identity(measured)
    report = context.reports / "asteroid-work-process.json"
    report.write_text(json.dumps(dict(_documentation=__doc__, complete=True, configuration=configuration, domain=domain, case=case,
                                     assertions=int(success[1]), cases=int(success[2]), measurement=measurement, terminal=identity(log),
                                     native_reports=[identity(p) for p in context.reports.glob("native-*.json")]), indent=2) + "\n", encoding="utf-8")
    return identity(report)


def main():
    """Execute one bounded scope and retain a fresh explicit selector after success."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), required=True)
    parser.add_argument("--domain", choices=("field", "probes", "focused"), required=True)
    parser.add_argument("--case", choices=CASES, default="small")
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    assert not args.output.exists()
    report = execute(args.configuration, args.domain, args.case)
    args.output.write_text(json.dumps(dict(_documentation=__doc__, complete=True, configuration=args.configuration,
                                         domain=args.domain, case=args.case, report=report), indent=2) + "\n", encoding="utf-8")
    print(f"passed {args.configuration} {args.domain} {args.case}", flush=True)


if __name__ == "__main__":
    main()
