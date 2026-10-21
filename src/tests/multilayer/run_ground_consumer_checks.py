"""Build and run one bounded current-ground consumer group with explicit preservation evidence.

@file run_ground_consumer_checks.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Groups79FB-H.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Current consumer cases use their own executable. Original games and completed
packing applications remain exact; each focused native process is capped60seconds.
"""

import argparse
import json
from pathlib import Path
import re
import subprocess
import time

from ground_pack_check_support import ROOT, handoff
from ground_payload_build_checks import execute
from tools.ground_payload_binding import reference
from support.artifact_paths import create_run_context

BASELINE = "logs/graphics/selected-ground-consumers/20261021-types-01/before.json"


def preserved(relative):
    """Require exact current source/payload/application identities outside named owner changes.

    @param relative Explicit repository-relative before snapshot for the active bounded group.
    @return Actual narrowly named existing-file transitions and preservation count.
    """
    before = handoff.read_json(relative)
    changed = []
    # Every old app, retained packing image and source artifact stays exact.
    # Only explicit executing-owner paths may differ; wildcard exemptions are
    # never inferred from a common directory or an old historical baseline.
    for row in before["files"]:
        actual = reference(handoff.bound(row["path"]))
        if actual != row:
            if row["path"] not in before["planned_existing_changes"]:
                raise ValueError("ground consumer changed an unrelated protected file: " + row["path"])
            changed.append({"before": row, "after": actual})
    return changed, len(before["files"])


def main():
    """Compile or execute only the requested current native consumer burst.

    @return None; retain real command/terminal/input/application identities in repository logs.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--mode", choices=("build", "run"), required=True)
    parser.add_argument("--configuration", choices=("Debug", "RelWithDebInfo"), required=True)
    parser.add_argument("--run", required=True, help="Repository logs directory for this exact invocation")
    parser.add_argument("--baseline", default=BASELINE, help="Explicit active-group before snapshot")
    parser.add_argument("--filter", default="[ground-sprite-contract]", help="Only the active group's exact Catch tags")
    args = parser.parse_args()
    output = handoff.bound(args.run)
    if not output.is_relative_to(ROOT / "logs"):
        raise ValueError("ground consumer diagnostics belong in repository logs")
    output.mkdir(parents=True, exist_ok=True)
    transitions, count = preserved(args.baseline)
    cache = dict(re.findall(r"^([^#/:=]+):[^=]+=(.*)$", (ROOT / "build/CMakeCache.txt").read_text(encoding="utf-8"), re.MULTILINE))
    application = ROOT / "build" / args.configuration / "openttd_ground_consumer_test.exe"
    commands, observed = [], None
    if args.mode == "build":
        commands.append(execute([cache["CMAKE_COMMAND"], "-S", str(ROOT), "-B", str(ROOT / "build")], ROOT, output / "configure.log"))
        commands.append(execute([cache["CMAKE_COMMAND"], "--build", str(ROOT / "build"), "--config", args.configuration,
                                 "--target", "openttd_ground_consumer_test", "--parallel", "2", "--", "/nodeReuse:false", "/p:CL_MPCount=2"],
                                ROOT, output / "build.log"))
    else:
        context = create_run_context("multilayer-art", "ground-consumer", run_id="ground-consumer-" + str(time.time_ns()),
                                     root=ROOT, profile="ground-consumers", process=args.configuration.lower())
        terminal = output / "native.log"
        started = time.monotonic()
        with terminal.open("w", encoding="utf-8") as stream:
            result = subprocess.run([str(application), args.filter], cwd=ROOT / "build", env=context.environment(),
                                    stdout=stream, stderr=subprocess.STDOUT, timeout=60, check=False)
        text = terminal.read_text(encoding="utf-8", errors="replace")
        match = re.search(r"All tests passed \(([\d,]+) assertions? in (\d+) test cases?\)", text)
        if result.returncode != 0 or match is None:
            # A zero exit alone cannot prove the requested producer boundary;
            # require the actual filtered Catch completion and exact case count.
            raise ValueError("ground current native consumer burst failed; inspect " + str(terminal))
        observed = {"exit_code": result.returncode, "terminal": reference(terminal), "invocation": context.run_id,
                    "filter": args.filter, "assertions": int(match.group(1).replace(",", "")), "cases": int(match.group(2)),
                    "seconds": round(time.monotonic() - started, 3)}
    transitions, count = preserved(args.baseline)
    inputs = [Path(__file__), ROOT / "src/ground_sprite.h", ROOT / "src/tests/ground_sprite_selection.cpp",
              handoff.bound(args.baseline), ROOT / "src/table/ground_sprite_assets.h"]
    proof = {"_documentation": __doc__, "passed": True, "configuration": args.configuration, "mode": args.mode,
             "application": reference(application), "inputs": [reference(path) for path in inputs],
             "commands": commands, "observation": observed, "preserved_file_count": count, "transitions": transitions,
             "runtime_activated": False}
    (output / (args.mode + ".json")).write_text(json.dumps(proof, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"passed": True, "configuration": args.configuration, "mode": args.mode, "observation": observed}))


if __name__ == "__main__":
    main()
