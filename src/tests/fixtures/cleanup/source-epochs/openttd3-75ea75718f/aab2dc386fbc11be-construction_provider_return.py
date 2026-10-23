"""Run one delivered construction provider boundary before actual input integration.

@file construction_provider_return.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs7-underground-construction-and-fleet.md4B.
@see docs/zz-reach2/upgrades/2026-10/r2mlvs3mp-master-plan.md10.8.
Direct callback producer checks establish ownership only; real mouse acceptance is separate.
"""

import argparse
import hashlib
import json
from pathlib import Path
import re
import sys

sys.dont_write_bytecode = True
ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / "src/tests/multilayer/support"))
from runner import current_run_context, run_native

CASES = {"construction": "[transport-tube-controls-construction]", "fleet": "[transport-tube-controls-fleet]",
         "external-power": "[transport-tube-controls]", "external-stale": "[transport-tube-controls-stale]",
         "native-power": "[tube-geothermal-controls-source]", "switch": "[tube-geothermal-controls-switch]",
         "guidance": "[tube-geothermal-controls-guidance]", "native-stale": "[tube-geothermal-controls-stale]"}
SOURCES = ("src/underground_gui.h", "src/underground_gui.cpp", "src/transport_tube.h", "src/transport_tube_gui.h",
           "src/transport_tube_gui.cpp", "src/transport_tube_engine.cpp", "src/transport_tube_cmd.cpp",
           "src/transport_tube_power.cpp", "src/transport_tube_geothermal.cpp", "src/multilayer_content.h",
           "src/widgets/underground_widget.h", "src/tests/transport_tube_controls.cpp", "src/tests/transport_tube_geothermal_controls.cpp")


def identity(path):
    """Identify one repository-owned source, actual binary or native diagnostic.

    @param path Existing file whose exact current bytes belong to this evidence.
    @return Repository-relative path, byte count and SHA256 without runtime setup.
    """
    path = Path(path).resolve()
    return {"path": path.relative_to(ROOT).as_posix(), "bytes": path.stat().st_size,
            "sha256": hashlib.sha256(path.read_bytes()).hexdigest()}


def native(context, configuration, tag, label):
    """Run one bounded current construction boundary without savegame imports.

    @param context Explicit diagnostic/runtime owner for this construction check.
    @param configuration Current Debug or RelWithDebInfo native test executable.
    @param tag Exact single compiled Catch selection, never an aggregate suite.
    @param label Process-local native log basename selected by the owning focus.
    @return Native stdout and independently parsed nonzero passing case counts.
    """
    result = run_native(context, [ROOT / f"build/{configuration}/openttd_test.exe", tag],
                        cwd=ROOT / "build", capture_output=True, timeout=110, check=False)
    output = (result.stdout + result.stderr).decode("utf-8", errors="replace")
    log = context.logs / f"{label}.log"
    log.write_text(output + f"\nNATIVE_EXIT_CODE={result.returncode}\n", encoding="utf-8")
    # Native failure or an empty selection cannot publish a passing boundary,
    # even when the other process diagnostics happen to contain familiar text.
    assert result.returncode == 0 and "All tests passed" in output, log
    counts = re.search(r"All tests passed \((\d+) assertions? in (\d+) test cases?\)", output)
    assert counts is not None, log
    assertions, cases = int(counts[1]), int(counts[2])
    assert min(assertions, cases) > 0, log
    print(f"{label}: {assertions} assertions / {cases} cases", flush=True)
    return output, {"log": identity(log), "assertions": assertions, "cases": cases}


def run_one(configuration, focus):
    """Verify one actual delivered provider obligation in its own native process.

    @param configuration Current Debug or RelWithDebInfo test executable.
    @param focus Exact provider boundary label declared in CASES.
    @return Current report path; failed/empty selections produce no accepted evidence.
    """
    context = current_run_context("construction", "tube-provider-return")
    _, proof = native(context, configuration, CASES[focus], focus)
    report = {"_documentation": "r2mlvs7-underground-construction-and-fleet.md4B.1;actual delivered tube construction/fleet/provider producer boundaries;directcallback tests do not establish real mouse input",
              "configuration": configuration, "focus": focus, "tag": CASES[focus], "native": proof,
              "controller": identity(__file__), "binary": identity(ROOT / f"build/{configuration}/openttd_test.exe"),
              "sources": [identity(ROOT / name) for name in SOURCES], "native_version": 434}
    target = context.output("reports", f"tube-provider-{configuration}-{focus}.json")
    target.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    return target


def main():
    """Select one current native reader and explicit delivered provider focus.

    @return None; shared runner enforces an independent110second native limit.
    """
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--config", choices=("Debug", "RelWithDebInfo"), required=True)
    parser.add_argument("--focus", choices=CASES, required=True)
    args = parser.parse_args()
    print(run_one(args.config, args.focus), flush=True)


if __name__ == "__main__":
    main()
