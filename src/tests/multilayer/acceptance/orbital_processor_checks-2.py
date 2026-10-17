"""Run bounded paid processor scopes with exact explicit predecessor and own-resave membership.

@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group11
Each scope contains at most eight isolated native children; no newest-file selection or manufactured gameplay input is permitted.
"""
import argparse
import json
from pathlib import Path
import runpy

from space_preview_checks import ROOT, identity

SHARED = runpy.run_path(str(Path(__file__).with_name("orbital_processor_checks-1.py")))
RUNTIME = runpy.run_path(str(Path(__file__).with_name("orbital_runtime_checks-1.py")))
GROUPS = {
    "native-a": ("inspected", "steel", "steel-frame", "steel-active", "miner-deployed", "ore-delivered", "steel-partial"),
    "native-b": ("steel-batch", "steel-stock", "goods-frame", "goods-active", "rare-delivered", "goods-partial", "goods-batch"),
    "native-c": ("refinery-ore", "refinery-stock", "refinery-frame", "refinery-active", "refinery-nore", "refinery-partial", "refinery-metal"),
    "native-d": ("refinery-retarget", "ice-delivered", "ice-partial", "ice-batch", "steel-outage", "steel-restored", "steel-cancelled", "steel-restarted"),
    "own-a": ("steel-partial", "steel-batch", "steel-outage", "steel-cancelled", "steel-restarted", "goods-partial"),
    "own-b": ("goods-batch", "refinery-partial", "refinery-metal", "ice-partial", "ice-batch", "refinery-retarget"),
    "resume": ("steel-batch", "steel-restored", "steel-restarted", "goods-batch", "refinery-metal", "ice-batch", "refinery-retarget"),
    "focused": (),
}


def read(path):
    """Read one exact evidence file, never a latest matching directory.

    @param path Explicit original selector or semantic proof path.
    @return Parsed unchanged evidence.
    """
    return json.loads(path.read_text(encoding="utf-8"))


def execute(configuration, scope, inputs):
    """Execute one bounded actual native phase group or current focused producer boundary.

    @param configuration Matching current native executable configuration.
    @param scope Exact ordered native, own-resave, own-continuation or focused membership.
    @param inputs Explicit preceding scope selectors; their actual reports determine all parent identities.
    @return Exact newly produced semantic proof paths in this invocation's execution order.
    """
    parents = {"native": {}, "resave": {}}
    # Only original explicit input membership may supply a parent. Duplicate phase/operation claims refuse rather than replacing an older owner with a newer matching file.
    for selector in inputs:
        selected = read(selector)
        assert selected["configuration"] == configuration and selected["complete"]
        for name in selected["reports"]:
            path = ROOT / name
            proof = read(path)
            assert proof["configuration"] == configuration and proof["complete"]
            operation, phase = proof["operation"], proof["phase"]
            assert operation in parents and phase not in parents[operation]
            assert identity(ROOT / proof["after"]["path"]) == proof["after"]
            parents[operation][phase] = path
    if scope == "focused":
        assert not inputs
        return [RUNTIME["run"](configuration, "focused", "partial")]
    produced = []
    # Each actual child receives its own exact predecessor and fresh native invocation. A resave cannot repeat a phase's controls; a continuation must load that child's own ordinary output.
    for phase in GROUPS[scope]:
        own = scope.startswith("own-")
        if own:
            parent = parents["native"][phase]
        elif scope == "resume":
            parent = parents["resave"][SHARED["PARENTS"][phase]]
        else:
            parent = parents["native"][SHARED["PARENTS"][phase]] if phase != "inspected" else None
        report = SHARED["inspect"](configuration, phase, parent, own)
        produced.append(report)
        if scope.startswith("native-"):
            assert phase not in parents["native"]
            parents["native"][phase] = report
        print(configuration, scope, phase, "passed", flush=True)
    return produced


def main():
    """Keep each acceptance burst bounded while preserving exact cross-scope process and source identities."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), required=True)
    parser.add_argument("--scope", choices=tuple(GROUPS), required=True)
    parser.add_argument("--input", type=Path, nargs="*", default=[])
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    assert not args.output.exists()
    inputs = [path.resolve() for path in args.input]
    reports = execute(args.configuration, args.scope, inputs)
    args.output.write_text(json.dumps(dict(_documentation=__doc__, configuration=args.configuration, scope=args.scope,
                                          complete=True, inputs=[identity(path) for path in inputs],
                                          reports=[path.relative_to(ROOT).as_posix() for path in reports]), indent=2) + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()
