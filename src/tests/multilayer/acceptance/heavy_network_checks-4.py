"""Partition completed broad reload membership into bounded per-case scopes without repeating native tests.

@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group9DC
The first Debug reload scope exceeded two minutes. Existing actual process reports remain immutable; future runs use checks-1.py --scope reload-<case> directly.
"""
import argparse
import json
from pathlib import Path
import runpy

from space_preview_checks import ROOT, identity

SHARED = runpy.run_path(str(Path(__file__).with_name("heavy_network_checks-1.py")))


def partition(configuration, selectors, prefix):
    """Preserve exact completed process membership while splitting broad reload scopes by consumer.

    @param configuration Exact original completed configuration.
    @param selectors Explicit completed reload-a/reload-b selector paths in that order.
    @param prefix Repository output prefix; each new per-case selector must not exist.
    """
    # Partition only actually completed four-report scopes. No fabricated assertion count, source checkpoint or newer runtime result can supply a missing original process.
    for selector, scope in zip(selectors, ("reload-a", "reload-b"), strict=True):
        original = SHARED["read"](selector); assert original["configuration"] == configuration and original["complete"] and original["scope"] == scope and len(original["reports"]) == 4
        members = {case: [] for case in SHARED["GROUPS"][scope[-1]]}; modes = {case: set() for case in members}
        for member in original["reports"]:
            proof = SHARED["read"](ROOT / member); case, mode = proof["case"], proof["mode"]
            assert proof["configuration"] == configuration and case in members and mode in SHARED["MODES"] and mode not in modes[case]; members[case].append(member); modes[case].add(mode)
        for case, reports in members.items():
            assert modes[case] == set(SHARED["MODES"]); target = Path(str(prefix) + "-reload-" + case + "-1.json"); assert not target.exists()
            target.write_text(json.dumps(dict(_documentation="Owner: r2mlvs11-orbital-stations-and-manufacturing.md Group9DC; exact per-case partition of a completed broader reload scope. Native process reports are retained unchanged; this operation runs no tests.",
                                             configuration=configuration, scope="reload-" + case, complete=True, partitioned_from=identity(selector.resolve()), reports=reports), indent=2) + "\n", encoding="utf-8")
            print(target.as_posix(), flush=True)


def main():
    """Partition explicit completed selectors only; future native reload invocations already support per-case scopes."""
    parser = argparse.ArgumentParser(description=__doc__); parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), required=True); parser.add_argument("--selector", nargs=2, type=Path, required=True); parser.add_argument("--output-prefix", type=Path, required=True)
    args = parser.parse_args(); partition(args.configuration, args.selector, args.output_prefix)


if __name__ == "__main__":
    main()
