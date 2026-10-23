"""Build/run only the active native tube terminal producer or cargo checks.

@file tube_terminal_checks.py
@see docs/zz-reach2/upgrades/2026-10/r2fc2-file-cleanup3.md Group20C, retained historical scope and fresh-source refusal.
@see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Group9.
@see docs/zz-reach2/upgrades/2026-10/r2mlvs3mp-master-plan.md task10.5.
Retain exact source/application identities and protect completed ground/Space
evidence. Native processes allow a full minute without agent interaction.
"""

import argparse
import json
from pathlib import Path
import re
import subprocess
import time

from ground_pack_check_support import ROOT, handoff
from ground_payload_build_checks import execute
from run_ground_consumer_checks import preserved
from support.artifact_paths import create_run_context
from tools.ground_payload_binding import reference

BASELINE = "logs/underground-tubes/20261021-terminal-01/before.json"
PRODUCERS = "[transport-tube-model],[transport-tube-construction],[transport-tube-readiness],[transport-tube-meter],[transport-tube-interchange-geometry]"


def main():
    """Build or run one explicit focused group and retain its completed evidence.

    @return None; failures leave their actual diagnostics and raise immediately.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--mode", choices=("build", "run"), required=True)
    parser.add_argument("--configuration", choices=("Debug", "RelWithDebInfo"), required=True)
    parser.add_argument("--run", required=True, help="New repository logs directory")
    parser.add_argument("--filter", default=PRODUCERS, help="Explicit native Catch tags")
    args = parser.parse_args()
    # The legacy driver owns the original model9 terminal epoch. Provisional
    # five-model edits cannot be compiled or executed as a replacement for it.
    from qualify_tube_terminals import require_original_source
    require_original_source(ROOT)
    output = handoff.bound(args.run)
    if not output.is_relative_to(ROOT / "logs"):
        raise ValueError("tube diagnostics belong in repository logs")
    output.mkdir(parents=True, exist_ok=True)
    preserved(BASELINE)
    cache = dict(re.findall(r"^([^#/:=]+):[^=]+=(.*)$", (ROOT / "build/CMakeCache.txt").read_text(encoding="utf-8"), re.MULTILINE))
    application = ROOT / "build" / args.configuration / "openttd_tube_test.exe"
    commands, observation = [], None
    if args.mode == "build":
        commands.append(execute([cache["CMAKE_COMMAND"], "-S", str(ROOT), "-B", str(ROOT / "build")], ROOT, output / "configure.log"))
        commands.append(execute([cache["CMAKE_COMMAND"], "--build", str(ROOT / "build"), "--config", args.configuration,
                                 "--target", "openttd_tube_test", "--parallel", "2", "--", "/nodeReuse:false", "/p:CL_MPCount=2"], ROOT, output / "build.log"))
    else:
        context = create_run_context("underground-tubes", "terminals", run_id="tube-terminal-" + str(time.time_ns()), root=ROOT,
                                     profile="tube-terminals", process=args.configuration.lower())
        terminal = output / "native.log"
        started = time.monotonic()
        with terminal.open("w", encoding="utf-8") as stream:
            result = subprocess.run([str(application), args.filter], cwd=ROOT / "build", env=context.environment(),
                                    stdout=stream, stderr=subprocess.STDOUT, timeout=60, check=False)
        text = terminal.read_text(encoding="utf-8", errors="replace")
        match = re.search(r"All tests passed \(([\d,]+) assertions? in (\d+) test cases?\)", text)
        # A successful process must also prove that its exact selected cases ran;
        # an empty filter or truncated test report cannot satisfy the producer gate.
        if result.returncode != 0 or match is None:
            raise ValueError("native tube checks failed; inspect " + str(terminal))
        observation = {"exit_code": result.returncode, "terminal": reference(terminal), "run_id": context.run_id,
                       "filter": args.filter, "assertions": int(match.group(1).replace(",", "")),
                       "cases": int(match.group(2)), "seconds": round(time.monotonic() - started, 3)}
    transitions, count = preserved(BASELINE)
    inputs = [Path(__file__), ROOT / "CMakeLists.txt", ROOT / "src/CMakeLists.txt", ROOT / "src/cargopacket.cpp",
              ROOT / "src/tests/tube_test_environment.cpp", ROOT / "src/tests/movement_fixture.h",
              *sorted((ROOT / "src").glob("transport_tube*")), *sorted((ROOT / "src/tests").glob("transport_tube*"))]
    record = {"_documentation": __doc__, "passed": True, "mode": args.mode, "configuration": args.configuration,
              "commands": commands, "application": reference(application), "observation": observation,
              "inputs": [reference(path) for path in inputs if path.is_file()], "baseline": reference(handoff.bound(BASELINE)),
              "preserved_file_count": count, "transitions": transitions}
    (output / (args.mode + ".json")).write_text(json.dumps(record, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"passed": True, "mode": args.mode, "configuration": args.configuration, "observation": observation}))


if __name__ == "__main__":
    main()
