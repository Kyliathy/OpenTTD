"""Provide the shared runtime/capture contract to PowerShell window observers.

@file window_context.py Structured JSON transport; no native process is launched.
@see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup.md Group 5C.
"""

import argparse
import hashlib
import json
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / "src/tests/multilayer/support"))
from runner import current_run_context, native_command


def main():
    """Materialize a manual capture invocation and return its exact path identities.

    @return None; stdout contains one JSON object consumed by PowerShell.
    """
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--feature", default="multilayer-art")
    parser.add_argument("--scenario", default="manual-window")
    parser.add_argument("arguments", nargs=argparse.REMAINDER)
    args = parser.parse_args()
    context = current_run_context(args.feature, args.scenario)
    arguments = args.arguments[1:] if args.arguments[:1] == ["--"] else args.arguments
    if "-c" not in arguments:
        arguments = ["-c", "native-art.cfg", *arguments]
    if "-X" not in arguments:
        arguments = ["-X", *arguments]
    arguments = native_command(context, ["openttd.exe", *arguments])[1:]
    inputs = []
    # A capture's exact config/save bytes identify its world and display setup.
    # Source identities are retained before the interactive process can run.
    for index, option in enumerate(arguments[:-1]):
        if option in ("-c", "-g"):
            path = Path(arguments[index + 1])
            if path.is_file():
                inputs.append({"path": path.relative_to(ROOT).as_posix(), "sha256": hashlib.sha256(path.read_bytes()).hexdigest()})
    paths = {}
    # The mixed manual art observer explicitly selects these features at each
    # capture site. Shared window mechanics do not infer ownership from names.
    for feature in (args.feature, "construction", "geology", "portals", "vehicles"):
        view = context.for_feature(feature, args.scenario)
        paths[feature] = {"images": str(view.images), "reports": str(view.reports)}
    print(json.dumps({"root": str(ROOT), "feature": context.feature, "scenario": context.scenario,
                      "run_id": context.run_id, "runtime": str(context.runtime_dir),
                      "images": str(context.images), "paths": paths, "inputs": inputs,
                      "environment": {key: value for key, value in context.environment({}).items()},
                      "arguments": arguments}))


if __name__ == "__main__":
    main()
